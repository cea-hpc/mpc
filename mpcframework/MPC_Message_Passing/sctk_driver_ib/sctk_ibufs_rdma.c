/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */
#ifdef MPC_USE_INFINIBAND

#include "sctk_ibufs.h"

//#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "IBUFR"
#include "sctk_ib_toolkit.h"
#include "sctk_ib_prof.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_ib.h"
#include "sctk_asm.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_eager.h"
#include "sctk_net_tools.h"
#include "utlist.h"

/**
 *  RDMA for eager messages. This implementation is based on the
 *  paper 'High Performance RDMA-Based MPI Implementation
 *  over Infiniband' (RDMA FP). We use the CQ notification because polling each
 *  RDMA channel is not scalable.
 *  TODO: use an additional set of QPs for RDMA where no buffers are
 *  posted in receive. We use buffers in recv when we piggyback the RDMA slot
 */

/* FIXME: the following variables *SHOULD* be in the rail */
/* Linked list of rdma_poll where each remote using the RDMA eager protocol
 * is listed*/
sctk_ibuf_rdma_pool_t *rdma_pool_list = NULL;
/* Elements to merge to the rdma_pool_list */
sctk_ibuf_rdma_pool_t *rdma_pool_list_to_merge = NULL;
/* Lock when adding and accessing the concat list */
static sctk_spinlock_t rdma_pool_list_lock = SCTK_SPINLOCK_INITIALIZER;

/* Linked list of regions */
static sctk_ibuf_region_t *rdma_region_list = NULL;
static sctk_spin_rwlock_t rdma_region_list_lock = SCTK_SPIN_RWLOCK_INITIALIZER;
/* Pointer for the clock algorithm */
static sctk_ibuf_region_t *clock_pointer = NULL;

static sctk_spinlock_t rdma_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_spin_rwlock_t rdma_polling_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/*
 * Init the remote for the RDMA connection
 */
//#warning "To reinit when disconnected"
void sctk_ibuf_rdma_remote_init ( sctk_ib_qp_t *remote )
{
	sctk_ibuf_rdma_set_remote_state_rtr ( remote, STATE_DECONNECTED );
	sctk_ibuf_rdma_set_remote_state_rts ( remote, STATE_DECONNECTED );
	remote->rdma.pending_data_lock = SCTK_SPINLOCK_INITIALIZER;
	remote->rdma.lock = SCTK_SPINLOCK_INITIALIZER;
	remote->rdma.polling_lock = SCTK_SPINLOCK_INITIALIZER;
	/* Counters */
	remote->rdma.messages_nb = 0;
	remote->rdma.max_pending_data = 0;
	OPA_store_int ( &remote->rdma.miss_nb, 0 );
	OPA_store_int ( &remote->rdma.hits_nb, 0 );
	OPA_store_int ( &remote->rdma.resizing_nb, 0 );
	OPA_store_int ( &remote->rdma.cancel_nb, 0 );
	/* Lock for resizing */
	remote->rdma.flushing_lock = SCTK_SPINLOCK_INITIALIZER;
}

static inline void __init_rdma_slots ( sctk_ibuf_region_t *region,
                                       void *ptr, void *ibuf, int nb_ibufs, int size_ibufs )
{
	int i;
	sctk_ibuf_t *ibuf_ptr = NULL;

	for ( i = 0; i < nb_ibufs; ++i )
	{
		const char *buffer = ( char * ) ( ( char * ) ptr + ( i * size_ibufs ) );
		ibuf_ptr = ( sctk_ibuf_t * ) ibuf + i;
		ibuf_ptr->region = region;
		ibuf_ptr->size = 0;
		ibuf_ptr->flag = FREE_FLAG;
		ibuf_ptr->index = i;
		ibuf_ptr->to_release = IBUF_RELEASE;

		/* Set flags  */
		ibuf_ptr->buffer = IBUF_RDMA_GET_PAYLOAD_FLAG ( buffer );
		ibuf_ptr->head_flag = IBUF_RDMA_GET_HEAD_FLAG ( buffer );
		ibuf_ptr->size_flag = IBUF_RDMA_GET_SIZE_FLAG ( buffer );
		ibuf_ptr->poison_flag_head = IBUF_RDMA_GET_POISON_FLAG_HEAD ( buffer );
		/* Reset piggyback */
		IBUF_GET_EAGER_PIGGYBACK ( ibuf_ptr->buffer ) = -1;
		/* Set headers value */
		* ( ibuf_ptr->head_flag ) = IBUF_RDMA_RESET_FLAG;
		/* Set poison value */
		* ( ibuf_ptr->poison_flag_head ) = IBUF_RDMA_POISON_HEAD;

		DL_APPEND ( region->list, ( ( sctk_ibuf_t * ) ibuf + i ) );
		sctk_nodebug ( "ibuf=%p, index=%d, buffer=%p, head_flag=%p, size_flag=%p",
		               ibuf_ptr, i, ibuf_ptr->buffer, ibuf_ptr->head_flag, ibuf_ptr->size_flag );
	}
}

void
sctk_ibuf_rdma_region_resize ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote,
                               sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel, int nb_ibufs, int size_ibufs )
{
	void *ptr = NULL;
	void *ibuf;

	ib_assume ( nb_ibufs > 0 );
	ib_assume ( remote->rdma.pool );
	/* Because we are reinitializing a RDMA region, be sure that a previous allocation
	 * has been made */
	assume ( region->buffer_addr );

	PROF_DECR_GLOB ( rail_ib->rail, SCTK_IB_IBUS_RDMA_SIZE, ( region->nb * sizeof ( sctk_ibuf_t ) ) );
	PROF_DECR_GLOB ( rail_ib->rail, SCTK_IB_IBUS_RDMA_SIZE, ( region->nb * region->size_ibufs ) );

	/* Check if all ibufs are really free */
#ifdef IB_DEBUG
	int busy;

	if ( channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
		/* SEND CHANNEL */
	{
		if ( ( busy = OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_SEND] ) ) != 0 )
		{
			sctk_error ( "Buffer with wrong number in SEND channel (busy_nb:%d)", busy );
			not_reachable();
		}
	}
	else
		if ( channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
			/* RECV CHANNEL */
		{
			if ( ( busy = OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_RECV] ) ) != 0 )
			{
				sctk_error ( "Buffer with wrong numer in RECV channel (busy_nb:%d)", busy );
				not_reachable();
			}
		}

	if ( channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
		/* SEND CHANNEL */
	{
		for ( i = 0; i < region->nb ; ++i )
		{
			ibuf_ptr = ( sctk_ibuf_t * ) region->ibuf + i;

			if ( ibuf_ptr->flag != FREE_FLAG )
			{
				sctk_error ( "Buffer %d (tail:%d) with wrong flag: %d in RECV channel (busy_nb:%d)", ibuf_ptr->index, region->tail->index,
				             ibuf_ptr->flag,
				             OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_RECV] ) );
			}

			assume ( * ( ibuf_ptr->head_flag ) == IBUF_RDMA_RESET_FLAG );
		}
	}
	else
		if ( channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
			/* SEND CHANNEL */
		{
			for ( i = 0; i < region->nb ; ++i )
			{
				ibuf_ptr = ( sctk_ibuf_t * ) region->ibuf + i;

				if ( ibuf_ptr->flag != FREE_FLAG )
				{
					sctk_error ( "Buffer with wrong flag: %d in SEND channel (busy_nb:%d)", ibuf_ptr->flag,
					             OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_SEND] ) );
				}
			}
		}
		else
			not_reachable();

#endif


	/* Unregister the memory.
	 * FIXME: ideally, we should use ibv_reregister() but this call is currently not
	 * supported by the libverb. */
	sctk_ib_mmu_entry_release ( region->mmu_entry );

	/* Realloc a new memory region.
	 * FIXME: Using realloc is a little bit useless here because we get an additional
	 * memory copy not used in this case */
	ib_assume ( region->buffer_addr );
	ptr = realloc ( region->buffer_addr, nb_ibufs * size_ibufs );
	ib_assume ( ptr );
	/* FIXME: is the memset here really usefull? */
	/* memset(ptr, 0, nb_ibufs * size_ibufs); */
	PROF_ADD_GLOB ( rail_ib->rail, SCTK_IB_IBUS_RDMA_SIZE, nb_ibufs * size_ibufs );

	ib_assume ( region->ibuf );
	ibuf = realloc ( region->ibuf, nb_ibufs * sizeof ( sctk_ibuf_t ) );
	ib_assume ( ibuf );
	/* FIXME: is the memset here really usefull? */
	/* memset (ibuf, 0, nb_ibufs * sizeof(sctk_ibuf_t)); */
	PROF_ADD_GLOB ( rail_ib->rail, SCTK_IB_IBUS_RDMA_SIZE, nb_ibufs * sizeof ( sctk_ibuf_t ) );

	ib_assume ( nb_ibufs > 0 );
	/* save previous values */
	region->size_ibufs_previous = region->size_ibufs;
	region->nb_previous = region->nb;
	/* Store new values */
	region->size_ibufs = size_ibufs;
	region->buffer_addr = ptr;
	region->nb = nb_ibufs;
	ib_assume ( ( region->size_ibufs != region->size_ibufs_previous ) ||
	            region->nb != region->nb_previous );
	region->rail = rail_ib;
	region->list = NULL;
	region->channel = channel;
	region->ibuf = ibuf;
	region->remote = remote;
	region->lock = SCTK_SPINLOCK_INITIALIZER;
	region->polled_nb = 0;
	region->allocated_size = ( nb_ibufs * ( size_ibufs +  sizeof ( sctk_ibuf_t ) ) );

	sctk_nodebug ( "Channel: %d", region->channel );

	/* register buffers at once
	 * FIXME: ideally, we should use ibv_reregister() but this call is currently not
	 * supported by the libverb. */
	region->mmu_entry = sctk_ib_mmu_entry_new( rail_ib, ptr,
	                                                    nb_ibufs * size_ibufs );
	sctk_nodebug ( "Size_ibufs: %lu", size_ibufs );
	sctk_nodebug ( "[%d] Reg %p registered for rank %d (channel:%d). lkey : %lu", rail_ib->rail->rail_number, ptr, remote->rank, channel, region->mmu_entry->mr->lkey );

	/* Init all RDMA slots */
	__init_rdma_slots ( region, ptr, ibuf, nb_ibufs, size_ibufs );

	/* Setup tail and head pointers */
	region->head = ibuf;
	region->tail = ibuf;

	if ( channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
		/* SEND CHANNEL */
	{
		/* Send the number of credits */
		remote->rdma.pool->send_credit = nb_ibufs;
		OPA_store_int ( &remote->rdma.pool->busy_nb[REGION_SEND], 0 );

		sctk_ib_nodebug ( "RESIZING SEND channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d (send_credit:%d)",
		                  remote->rank, nb_ibufs, size_ibufs, remote->rdma.pool->send_credit );

	}
	else
		if ( channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
			/* RECV CHANNEL */
		{
			sctk_ib_nodebug ( "RESIZING RECV channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d)",
			                  remote->rank, nb_ibufs, size_ibufs );

			/* Add the entry to the pooling list */
			OPA_store_int ( &remote->rdma.pool->busy_nb[REGION_RECV], 0 );

		}
		else
			not_reachable();

	sctk_nodebug ( "Head=%p(%d) Tail=%p(%d)", region->head, region->head->index,
	               region->tail, region->tail->index );
}

void
sctk_ibuf_rdma_region_free ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote,
                             sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel )
{
	LOAD_DEVICE ( rail_ib );

	PROF_DECR_GLOB ( rail_ib->rail, SCTK_IB_IBUS_RDMA_SIZE, ( region->nb * sizeof ( sctk_ibuf_t ) ) );
	PROF_DECR_GLOB ( rail_ib->rail, SCTK_IB_IBUS_RDMA_SIZE, ( region->nb * region->size_ibufs ) );

	{
		/* Free the buffers */
		ib_assume ( region->buffer_addr );
		ib_assume ( region->ibuf );

		free ( region->buffer_addr );
		free ( region->ibuf );

		region->buffer_addr = NULL;
		region->ibuf = NULL;
		region->allocated_size = 0;
	}

	sctk_spinlock_lock ( &rdma_lock );
	--device->eager_rdma_connections;
	sctk_spinlock_unlock ( &rdma_lock );

	if ( channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
		/* SEND CHANNEL */
	{
		sctk_ib_nodebug ( "FREEING SEND channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d)",
		                  remote->rank, nb_ibufs, size_ibufs, remote->rdma.pool->send_credit );

		/* Reinit counters */
		remote->rdma.messages_nb = 0;
		remote->rdma.messages_size = 0;
		remote->rdma.stats_lock = SCTK_SPINLOCK_INITIALIZER;
		remote->rdma.max_pending_data = 0;
		remote->rdma.pool->send_credit = 0;
		OPA_store_int ( &remote->rdma.miss_nb, 0 );
		OPA_store_int ( &remote->rdma.hits_nb, 0 );
		OPA_store_int ( &remote->rdma.resizing_nb, 0 );

	}
	else
		if ( channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
			/* RECV CHANNEL */
		{
			sctk_ib_nodebug ( "FREEING RECV channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d)",
			                  remote->rank, nb_ibufs, size_ibufs );

			sctk_ibuf_rdma_pool_t *pool = remote->rdma.pool;
//#warning "We should lock during DELETING the element."
			//    sctk_spinlock_write_lock(&rdma_polling_lock);
			DL_DELETE ( rdma_pool_list, pool );
			//    sctk_spinlock_write_unlock(&rdma_polling_lock);

			sctk_spinlock_write_lock ( &rdma_region_list_lock );

			/* Shift clock pointer */
			if ( clock_pointer == region )
				clock_pointer = region->next;

			CDL_DELETE ( rdma_region_list, region );
			sctk_spinlock_write_unlock ( &rdma_region_list_lock );
		}
		else
			not_reachable();
}


/*
 * Reinitialize a RDMA region with a different number of ibufs and a different size of ibufs
 */
void
sctk_ibuf_rdma_region_reinit ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote,
                               sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel, int nb_ibufs, int size_ibufs )
{

	/* If we need to free the region */
	if ( nb_ibufs == 0 )
	{
		sctk_ibuf_rdma_region_free ( rail_ib, remote, region, channel );
		PROF_INC_GLOB ( rail_ib->rail, SCTK_IB_RDMA_DECONNECTION );
	}
	else
		/* If we need to resize the region */
	{
//    PROF_TIME_START(rail_ib->rail, ib_resize_rdma);
		sctk_ibuf_rdma_region_resize ( rail_ib, remote, region, channel, nb_ibufs, size_ibufs );
//    PROF_TIME_END(rail_ib->rail, ib_resize_rdma);
		PROF_INC_GLOB ( rail_ib->rail, SCTK_IB_RDMA_RESIZING );
	}
}


/*
 * Initialize a new RDMA buffer region
 */
void
sctk_ibuf_rdma_region_init ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote,
                             sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel, int nb_ibufs, int size_ibufs )
{
	void *ptr = NULL;
	void *ibuf;

	ib_assume ( nb_ibufs > 0 );
	ib_assume ( remote->rdma.pool );

	/* XXX: replace by memalign_on_node */
	sctk_posix_memalign ( ( void ** ) &ptr, getpagesize(), nb_ibufs * size_ibufs );
	ib_assume ( ptr );
	memset ( ptr, 0, nb_ibufs * size_ibufs );
	PROF_ADD_GLOB ( rail_ib->rail, SCTK_IB_IBUS_RDMA_SIZE, nb_ibufs * size_ibufs );

	/* XXX: replace by memalign_on_node */
	sctk_posix_memalign ( &ibuf, getpagesize(), nb_ibufs * sizeof ( sctk_ibuf_t ) );
	ib_assume ( ibuf );
	memset ( ibuf, 0, nb_ibufs * sizeof ( sctk_ibuf_t ) );
	PROF_ADD_GLOB ( rail_ib->rail, SCTK_IB_IBUS_RDMA_SIZE, nb_ibufs * sizeof ( sctk_ibuf_t ) );

	region->size_ibufs_previous = 0;
	region->nb_previous = 0;
	region->size_ibufs = size_ibufs;
	region->nb = nb_ibufs;
	ib_assume ( ( region->size_ibufs != region->size_ibufs_previous ) ||
	            region->nb != region->nb_previous );
	region->buffer_addr = ptr;
	region->rail = rail_ib;
	region->list = NULL;
	region->channel = channel;
	region->ibuf = ibuf;
	region->remote = remote;
	region->lock = SCTK_SPINLOCK_INITIALIZER;
	region->polled_nb = 0;
	region->allocated_size = ( nb_ibufs * ( size_ibufs +  sizeof ( sctk_ibuf_t ) ) );

	sctk_nodebug ( "Channel: %d", region->channel );

	/* register buffers at once */
	region->mmu_entry = sctk_ib_mmu_entry_new ( rail_ib, ptr,
	                                                    nb_ibufs * size_ibufs );
	sctk_nodebug ( "Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey );
	sctk_nodebug ( "Size_ibufs: %lu", size_ibufs );

	/* Init all RDMA slots */
	__init_rdma_slots ( region, ptr, ibuf, nb_ibufs, size_ibufs );

	/* Setup tail and head pointers */
	region->head = ibuf;
	region->tail = ibuf;

	if ( channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
		/* SEND CHANNEL */
	{
		/* Send the number of credits */
		remote->rdma.pool->send_credit = nb_ibufs;
		OPA_store_int ( &remote->rdma.pool->busy_nb[REGION_SEND], 0 );

		sctk_ib_nodebug ( "SEND channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d (send_credit:%d)",
		                  remote->rank, nb_ibufs, size_ibufs, remote->rdma.pool->send_credit );

	}
	else
		if ( channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
			/* RECV CHANNEL */
		{
			sctk_ib_nodebug ( "RECV channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d",
			                  remote->rank, nb_ibufs, size_ibufs );

			/* Add the entry to the pooling list */
			sctk_spinlock_lock ( &rdma_pool_list_lock );
			DL_APPEND ( rdma_pool_list_to_merge, remote->rdma.pool );
			sctk_spinlock_unlock ( &rdma_pool_list_lock );
			OPA_store_int ( &remote->rdma.pool->busy_nb[REGION_RECV], 0 );

		}
		else
			not_reachable();

	sctk_spinlock_write_lock ( &rdma_region_list_lock );

	/* Set clock-pointer */
	if ( clock_pointer == NULL )
		clock_pointer = region;

	CDL_PREPEND ( rdma_region_list, region );
	PROF_INC_GLOB ( rail_ib->rail, SCTK_IB_RDMA_CONNECTION );
	sctk_spinlock_write_unlock ( &rdma_region_list_lock );

	sctk_nodebug ( "Head=%p(%d) Tail=%p(%d)", region->head, region->head->index,
	               region->tail, region->tail->index );
}

/*
 * Create a region of ibufs for RDMA
 */
sctk_ibuf_rdma_pool_t *
sctk_ibuf_rdma_pool_init ( sctk_ib_qp_t *remote )
{
	sctk_ibuf_rdma_pool_t *pool;

	/* We lock during the memmory allocating */
	sctk_spinlock_lock ( &remote->rdma.lock );
	pool = remote->rdma.pool;

	/* If no allocation has been done */
	if ( remote->rdma.pool == NULL )
	{
		pool = sctk_malloc ( sizeof ( sctk_ibuf_rdma_pool_t ) );
		ib_assume ( pool );
		memset ( pool, 0, sizeof ( sctk_ibuf_rdma_pool_t ) );

		/* Setup local addr */
		pool->remote_region[REGION_SEND] = NULL;
		pool->remote_region[REGION_RECV] = NULL;
		pool->remote = remote;

		/* Finaly update the pool pointer */
		remote->rdma.pool = pool;
	}

	sctk_spinlock_unlock ( &remote->rdma.lock );

	return pool;
}

/*
 * Delete a RDMA poll from a remote
 */
void
sctk_ibuf_rdma_pool_free ( __UNUSED__ struct sctk_ib_rail_info_s *rail_ib, __UNUSED__ sctk_ib_qp_t *remote )
{
/* Needs to be re-written */
#if 0
	LOAD_DEVICE ( rail_ib );
	/* Check if we can allocate an RDMA channel */
	sctk_spinlock_lock ( &rdma_lock );

	if ( sctk_ibuf_rdma_is_remote_connected ( remote ) )
	{
		sctk_ibuf_rdma_pool_t *pool = remote->rdma.pool;
		--device->eager_rdma_connections;
		sctk_spinlock_unlock ( &rdma_lock );

		sctk_ib_debug ( "Freeing RDMA buffers (connections: %d)", device->eager_rdma_connections );

		/* Free regions for send and recv */
		sctk_ibuf_rdma_region_free ( rail_ib, &pool->region[REGION_SEND] );
		sctk_ibuf_rdma_region_free ( rail_ib, &pool->region[REGION_RECV] );

		sctk_spinlock_lock ( &rdma_pool_list_lock );
		DL_DELETE ( rdma_pool_list_to_merge, pool );
		sctk_spinlock_unlock ( &rdma_pool_list_lock );

		/* Free pool */
		free ( remote->rdma.pool );
		remote->rdma.pool = NULL;

		sctk_ib_debug ( "[rank:%d] Free RDMA buffers", remote->rank );
	}
	else
	{
		sctk_spinlock_unlock ( &rdma_lock );
	}

#endif
}


/*
 * Pick a RDMA buffer from the RDMA channel
 */
inline sctk_ibuf_t *sctk_ibuf_rdma_pick ( sctk_ib_qp_t *remote )
{
	sctk_ibuf_t *tail;
	int piggyback;

	IBUF_RDMA_LOCK_REGION ( remote, REGION_SEND );

	tail = IBUF_RDMA_GET_TAIL ( remote, REGION_SEND );
	piggyback =  IBUF_GET_EAGER_PIGGYBACK ( tail->buffer );

	sctk_nodebug ( "Piggy back field %d %p", piggyback, &IBUF_GET_EAGER_PIGGYBACK ( tail->buffer ) );

	if ( piggyback > 0 )
	{
		int i;
		/* Reinit the piggyback of each buffer */
		sctk_ibuf_t *ptr = tail;

		/* For each slot freed, we reset the piggyback filed */
		for ( i = 0; i < piggyback; ++i )
		{
			IBUF_GET_EAGER_PIGGYBACK ( ptr->buffer ) = -1;
			ptr = IBUF_RDMA_GET_NEXT ( ptr );
		}

		/* Upgrade the send credit and move the tail flag */
		remote->rdma.pool->send_credit += piggyback;
		IBUF_RDMA_GET_TAIL ( remote, REGION_SEND ) = IBUF_RDMA_ADD ( tail, piggyback );

		sctk_nodebug ( "Got pg %d (prev:%p next:%p)", piggyback, tail, IBUF_RDMA_GET_TAIL ( remote, REGION_SEND ) );
	}

	sctk_nodebug ( "Send credit: %d", remote->rdma.pool->send_credit );

	/* If a buffer is available */
	if ( remote->rdma.pool->send_credit > 0 )
	{
		sctk_ibuf_t *head;

		head = IBUF_RDMA_GET_HEAD ( remote, REGION_SEND );

		/* If the buffer is not free for the moment, we skip the
		 * RDMA */
//    if (head->flag != FREE_FLAG) {
//      sctk_error("Buffer %p not free", head);
//      IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);
//      return NULL;
//    }

		/* Update the credit */
		remote->rdma.pool->send_credit--;
		/* Move the head flag */
		IBUF_RDMA_GET_HEAD ( remote, REGION_SEND ) = IBUF_RDMA_GET_NEXT ( head );
		IBUF_RDMA_UNLOCK_REGION ( remote, REGION_SEND );

		sctk_nodebug ( "Picked RDMA buffer %p, buffer=%p pb=%p index=%d credit=%d size:%d", head,
		               head->buffer, &IBUF_GET_EAGER_PIGGYBACK ( head->buffer ), head->index, remote->rdma.pool->send_credit,
		               remote->rdma.pool->region[REGION_SEND].size_ibufs );
#ifdef IB_DEBUG

		if ( head->flag != FREE_FLAG )
		{
			sctk_error ( "Wrong flag (%d) got from ibuf", head->flag );
			not_implemented();
		}

#endif
		head->flag = BUSY_FLAG;

		remote->rdma.pool->region[REGION_SEND].R_bit = 1;
		return ( sctk_ibuf_t * ) head;
	}
	else
	{
		sctk_nodebug ( "Cannot pick RDMA buffer: %d", remote->rdma.pool->send_credit );
		IBUF_RDMA_UNLOCK_REGION ( remote, REGION_SEND );
	}

	return NULL;
}

/*
 * Reset the head flag after a message has been successfully received
 */
void sctk_ib_rdma_reset_head_flag ( sctk_ibuf_t *ibuf )
{
	* ( ibuf->head_flag ) = IBUF_RDMA_RESET_FLAG;
}

/*
 * Set the tail flag to the ibuf
 */
void sctk_ib_rdma_set_tail_flag ( sctk_ibuf_t *ibuf, size_t size )
{
	/* Head, payload and Tail */
	int *ibuf_tail;

	ibuf_tail = IBUF_RDMA_GET_TAIL_FLAG ( ibuf->buffer, size );

	* ( ibuf->size_flag ) = size;

	if ( *ibuf_tail == IBUF_RDMA_FLAG_1 )
	{
		*ibuf_tail = IBUF_RDMA_FLAG_2;
		* ( ibuf->head_flag ) = IBUF_RDMA_FLAG_2;
	}
	else
	{
		*ibuf_tail = IBUF_RDMA_FLAG_1;
		* ( ibuf->head_flag ) = IBUF_RDMA_FLAG_1;
	}
}

/*
 * Handle a message received
 */
static void __poll_ibuf ( sctk_ib_rail_info_t *rail_ib,
                          sctk_ibuf_t *ibuf )
{
	sctk_rail_info_t *rail = rail_ib->rail;
	ib_assume ( ibuf );
	int release_ibuf = 1;
	int ret = REORDER_UNDEFINED;

	/* Set the buffer as releasable. Actually, we need to do this reset
	 * here.. */
	ibuf->to_release = IBUF_RELEASE;

	/* const size_t size_flag = *ibuf->size_flag; */
	/* int *tail_flag = IBUF_RDMA_GET_TAIL_FLAG(ibuf->buffer, size_flag); */
	sctk_nodebug ( "Buffer size:%d, head flag:%d, tail flag:%d", *ibuf->size_flag, *ibuf->head_flag, *tail_flag );

	const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL ( ibuf->buffer );

	switch ( protocol )
	{
		case SCTK_IB_EAGER_PROTOCOL:
			sctk_ib_eager_poll_recv ( rail, ibuf );
			release_ibuf = 0;
			break;

		case SCTK_IB_RDMA_PROTOCOL:
			sctk_nodebug ( "RDMA received from RDMA channel" );
			release_ibuf = sctk_ib_rdma_poll_recv ( rail, ibuf );
			break;

		case SCTK_IB_BUFFERED_PROTOCOL:
			sctk_ib_buffered_poll_recv ( rail, ibuf );
			release_ibuf = 1;
			break;

		default:
			not_reachable();
	}

	if ( release_ibuf )
	{
		sctk_nodebug ( "Release RDMA buffer %p" );
		sctk_ibuf_release ( rail_ib, ibuf, 1 );
	}

	return;
}


/*
 * Walk on each remotes
 */
void sctk_ib_rdma_eager_walk_remotes ( sctk_ib_rail_info_t *rail, int ( func ) ( sctk_ib_rail_info_t *rail, sctk_ib_qp_t *remote ), int *ret )
{
	sctk_ibuf_rdma_pool_t *pool, *tmp_pool;
	int tmp_ret;

	/* Set the default value */
	*ret = REORDER_UNDEFINED;

	sctk_spinlock_read_lock ( &rdma_polling_lock );
	DL_FOREACH ( rdma_pool_list, pool )
	{
		/* 'func' needs to check if the remote is in a RTR mode */
		tmp_ret = func ( rail, pool->remote );

		if ( tmp_ret == REORDER_FOUND_EXPECTED )
		{
			*ret = tmp_ret;
			sctk_spinlock_read_unlock ( &rdma_polling_lock );
			return;
		}
	}
	sctk_spinlock_read_unlock ( &rdma_polling_lock );

	/* Check if they are remotes to merge. We need to merge here because
	 * we cannot do this in the DL_FOREACH just before. It aims
	 * to deadlocks. */
	if ( rdma_pool_list_to_merge != NULL )
	{
		sctk_spinlock_write_lock ( &rdma_polling_lock );
		/* We only add entries of connected processes */
		sctk_spinlock_lock ( &rdma_pool_list_lock );
		DL_FOREACH_SAFE ( rdma_pool_list_to_merge, pool, tmp_pool )
		{
			sctk_endpoint_state_t state = sctk_ibuf_rdma_get_remote_state_rtr ( pool->remote );

			if ( ( state == STATE_CONNECTED ) )
			{
				/* Remove the entry from the merge list... */
				DL_DELETE ( rdma_pool_list_to_merge, pool );
				/* ... add it to the global list */
				DL_APPEND ( rdma_pool_list, pool );
			}
		}
		sctk_spinlock_unlock ( &rdma_pool_list_lock );
		sctk_spinlock_write_unlock ( &rdma_polling_lock );
	}
}


/*
 * Poll a specific remote in order to handle new received messages.
 * Returns :
 *  1 if a message has been found
 *  -1 if the lock had not been taken
 *  0 if no message has been found
 */
static int
sctk_ib_rdma_eager_poll_remote_locked ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote )
{
	/* We return if the remote is not connected to the RDMA channel */
	sctk_endpoint_state_t state = sctk_ibuf_rdma_get_remote_state_rtr ( remote );

	if ( ( state != STATE_CONNECTED ) && ( state != STATE_REQUESTING ) )
		return REORDER_UNDEFINED;

	sctk_ibuf_t *head;

	/* Spinlock for preventing two concurrent calls to this function */
retry:

	if ( IBUF_RDMA_TRYLOCK_REGION ( remote, REGION_RECV ) == 0 )
	{

		/* Read the state of the connection and return if necessary.
		 * If we do not this, the RDMA buffer may be read while the RDMA buffer
		 * is in a 'flushed' state */
		state = sctk_ibuf_rdma_get_remote_state_rtr ( remote );

		if ( ( state != STATE_CONNECTED ) && ( state != STATE_REQUESTING ) )
		{
			IBUF_RDMA_UNLOCK_REGION ( remote, REGION_RECV );
			return REORDER_UNDEFINED;
		}

		/* Recaculate the head because it could be moved */
		head = IBUF_RDMA_GET_HEAD ( remote, REGION_RECV );

		/* Double checking */
		if ( * ( head->head_flag ) != IBUF_RDMA_RESET_FLAG )
		{

			sctk_nodebug ( "Head set: %d-%d", * ( head->head_flag ), IBUF_RDMA_RESET_FLAG );

			const size_t size_flag = *head->size_flag;
			volatile int volatile *tail_flag = IBUF_RDMA_GET_TAIL_FLAG ( head->buffer, size_flag );

			sctk_nodebug ( "Head set: %d-%d", * ( head->head_flag ), IBUF_RDMA_RESET_FLAG );

			/* The head has been set. We spin while waiting the tail to be set.
			 * We do not release the lock because it is not necessary that a task
			 * concurrently polls the same remote.  */
			int i = 0;

			while ( ( * ( head->head_flag ) != * ( tail_flag ) ) && ( i++ < 10 ) )
			{
				/* poll */
			};

			if ( * ( head->head_flag ) != * ( tail_flag ) )
			{
				IBUF_RDMA_UNLOCK_REGION ( remote, REGION_RECV );
				return REORDER_NOT_FOUND;
			}

			/* Reset the head flag */
			sctk_ib_rdma_reset_head_flag ( head );
			/* Move head flag */
			IBUF_RDMA_GET_HEAD ( remote, REGION_RECV ) =  IBUF_RDMA_GET_NEXT ( head );
			/* Increase the number of buffers busy */
			OPA_incr_int ( &remote->rdma.pool->busy_nb[REGION_RECV] );
			IBUF_RDMA_UNLOCK_REGION ( remote, REGION_RECV );
			IBUF_RDMA_CHECK_POISON_FLAG ( head );

			sctk_nodebug ( "Buffer size:%d, ibuf:%p, head flag:%d, tail flag:%d %p new_head:%p (%d-%d)", *head->size_flag, head, *head->head_flag, *tail_flag, head->buffer, IBUF_RDMA_GET_HEAD ( remote, REGION_RECV )->buffer, * ( head->head_flag ), * ( tail_flag ) );

			if ( head->flag != FREE_FLAG )
			{
				sctk_error ( "Got a wrong flag, it seems there is a problem with MPC: %d %p", head->flag,
				             head );
				not_reachable();
			}

			/* Set the slot as busy */
			head->flag = BUSY_FLAG;
			sctk_nodebug ( "Set %p as busy", head );

			/* Handle the ibuf */
			__poll_ibuf ( rail_ib, head );
			goto retry;
		}
		else
		{
			IBUF_RDMA_UNLOCK_REGION ( remote, REGION_RECV );
			return REORDER_NOT_FOUND;
		}
	}
	else
	{
		return REORDER_UNDEFINED;
	}
}



/*
 * Poll a specific remote in order to handle new received messages.
 * Returns :
 *  1 if a message has been found
 *  -1 if the lock had not been taken
 *  0 if no message has been found
 */
int
sctk_ib_rdma_eager_poll_remote ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote )
{
	/* We return if the remote is not connected to the RDMA channel */
	sctk_endpoint_state_t state = sctk_ibuf_rdma_get_remote_state_rtr ( remote );

	if ( ( state != STATE_CONNECTED ) && ( state != STATE_REQUESTING ) )
		return REORDER_UNDEFINED;

	sctk_ibuf_t *head;

	head = IBUF_RDMA_GET_HEAD ( remote, REGION_RECV );

	if ( * ( head->head_flag ) != IBUF_RDMA_RESET_FLAG )
	{
		if ( sctk_ib_rdma_eager_poll_remote_locked ( rail_ib, remote ) == REORDER_FOUND_EXPECTED )
		{
			return REORDER_FOUND_EXPECTED;
		}
	}

	return REORDER_UNDEFINED;
}

/* / ! \ This function requires the region to be previously locked !! */
void sctk_ibuf_rdma_check_piggyback ( sctk_ib_rail_info_t *rail_ib, sctk_ibuf_region_t *region )
{
	sctk_ib_qp_t *remote = region->remote;
	int ret;
	int piggyback = 0;
	sctk_ibuf_t *ibuf;

	sctk_nodebug ( "polled nb: %d -> %f", region->polled_nb, region->nb * ( PIGGYBACK_THRESHOLD ) );

	/* Get the tail */
	sctk_ibuf_t *tail = IBUF_RDMA_GET_TAIL ( remote, REGION_RECV );

	/* Buffer which will be piggybacked */
	ibuf = tail;

	while ( ibuf->flag == EAGER_RDMA_POLLED )
	{
		piggyback ++;
		/* Put the buffer as free */
		ibuf->flag = FREE_FLAG;
		sctk_nodebug ( "Set %p as free", ibuf );
		/* Move to next buffer */
		ibuf = IBUF_RDMA_GET_NEXT ( ibuf );
	}

	/* Piggyback the ibuf to the tail addr */
	if ( piggyback > 0 )
	{
		ib_assume ( tail->flag == FREE_FLAG );
		ib_assume ( tail->region );
		ib_assume ( tail->region->ibuf );
		ib_assume ( region->nb > 0 );

		/* Move tail */
		IBUF_RDMA_GET_TAIL ( remote, REGION_RECV ) = IBUF_RDMA_ADD ( tail, piggyback );
		/* Unlock the RDMA region */
		IBUF_GET_EAGER_PIGGYBACK ( tail->buffer ) = piggyback;
		region->polled_nb = 0;
		IBUF_RDMA_UNLOCK_REGION ( remote, REGION_RECV );


		sctk_nodebug ( "Piggy back to %p pb:%d size:%d", IBUF_RDMA_GET_REMOTE_PIGGYBACK ( remote, tail ), piggyback, remote->rdma.pool->region[REGION_RECV].size_ibufs );

		/* Prepare the piggyback msg. The sent message is SIGNALED */
		sctk_nodebug ( "ACK %d messages", piggyback );
		ret = sctk_ibuf_rdma_write_with_imm_init ( tail,
		                                           &IBUF_GET_EAGER_PIGGYBACK ( tail->buffer ), /* Local addr */
		                                           region->mmu_entry->mr->lkey,  /* lkey */
		                                           IBUF_RDMA_GET_REMOTE_PIGGYBACK ( remote, tail ), /* Remote addr */
		                                           remote->rdma.pool->rkey[REGION_SEND],  /* rkey */
		                                           sizeof ( int ), IBUF_DO_NOT_RELEASE, IMM_DATA_RDMA_PIGGYBACK + piggyback );
		assume ( ret == 1 );

		/* Put the buffer as free */
		tail->flag = FREE_FLAG;

		/* Send the piggyback. This event will generate an event to
		 * the CQ */
		ret = sctk_ib_qp_send_ibuf ( rail_ib, remote, tail);
		assume ( ret == 1 );
	}
	else
	{
		/* Unlock the RDMA region */
		IBUF_RDMA_UNLOCK_REGION ( remote, REGION_RECV );

		/* Increase the number of buffers busy */
		OPA_decr_int ( &remote->rdma.pool->busy_nb[REGION_RECV] );
		sctk_ibuf_rdma_check_flush_recv ( rail_ib, remote );
	}
}

//#define PIGGYBACK_THRESHOLD (50.0 / 100.0)
#define PIGGYBACK_THRESHOLD (0)
/*
 * Release a buffer from the RDMA channel
 */
void sctk_ibuf_rdma_release ( sctk_ib_rail_info_t *rail_ib, sctk_ibuf_t *ibuf )
{
	sctk_ibuf_region_t *region = ibuf->region;
	sctk_ib_qp_t *remote = ibuf->region->remote;
	int ret;

	if ( IBUF_GET_CHANNEL ( ibuf ) & RECV_CHANNEL )
	{
		int piggyback = 0;

		sctk_nodebug ( "Freeing a RECV RDMA buffer (channel:%d head:%p - ibuf:%p - tail:%p)", IBUF_GET_CHANNEL ( ibuf ),
		               ibuf->region->head, ibuf, ibuf->region->tail );

		/* Firstly lock the region */
		IBUF_RDMA_LOCK_REGION ( remote, REGION_RECV );


		/* Mark the buffer as polled */
		if ( ibuf->flag != BUSY_FLAG )
		{
			sctk_error ( "Got a wrong flag, it seems there is a problem with MPC: %d", ibuf->flag );
			not_reachable();
		}

		ibuf->flag = EAGER_RDMA_POLLED;
		region->R_bit = 1;
		sctk_nodebug ( "Set %p as RDMA_POLLED", ibuf );

		/* Check if the buffer is located at the tail adress */
		sctk_nodebug ( "Flag=%d tail=(%p-%p-%d) next_tail=(%p-%p-%d)", ibuf->flag,
		               tail, tail->buffer, tail->index,
		               IBUF_RDMA_GET_NEXT ( tail ),
		               IBUF_RDMA_GET_NEXT ( tail )->buffer,
		               IBUF_RDMA_GET_NEXT ( tail )->index );

		/* While a buffer free is found, we increase the piggy back and
		 * we reset each buffer */
		region->polled_nb++;

		if ( region->polled_nb >  region->nb * PIGGYBACK_THRESHOLD /* 0 */ )
		{
			sctk_ibuf_rdma_check_piggyback ( rail_ib, region );
		}
		else
		{
			/* Unlock the RDMA region */
			IBUF_RDMA_UNLOCK_REGION ( remote, REGION_RECV );

			/* Increase the number of buffers busy */
			OPA_decr_int ( &remote->rdma.pool->busy_nb[REGION_RECV] );
			sctk_ibuf_rdma_check_flush_recv ( rail_ib, remote );
		}

	}
	else
		if ( IBUF_GET_CHANNEL ( ibuf ) & SEND_CHANNEL )
		{
			/* We set the buffer as free */
//    ibuf->flag = FREE_FLAG;
//    OPA_decr_int(&remote->rdma.pool->busy_nb[REGION_SEND]);

			/* Check if we are in a flush state */
//    sctk_ibuf_rdma_check_flush_send(rail_ib, remote);
		}
		else
			not_reachable();
}


/* Number of messages needed to be exchanged before connecting peers
 * using RMDA */

/* FOR PAPER */
//#define IBV_RDMA_MIN_SIZE (2 * 1024)
//#define IBV_RDMA_MAX_SIZE (12 * 1024)
//#define IBV_RDMA_MIN_NB (16)
//#define IBV_RDMA_MAX_NB (1024)

/* FOR PAPER */
//#define IBV_RDMA_RESIZING_MIN_SIZE (2 * 1024)
//#define IBV_RDMA_RESIZING_MAX_SIZE (12 * 1024)
//#define IBV_RDMA_RESIZING_MIN_NB (16)
//#define IBV_RDMA_RESIZING_MAX_NB (1024)

/* Maximum number of miss before resizing RDMA */
#define IBV_RDMA_MAX_MISS 500
//#define IBV_RDMA_MAX_MISS 1
/* It is better if it is higher than IBV_RDMA_SAMPLES */
#define IBV_RDMA_THRESHOLD 100
//#define IBV_RDMA_THRESHOLD 1


/*
 * Check if a remote can be connected using RDMA.
 * The number of connections is automatically increased
 */
int sctk_ibuf_rdma_is_connectable ( sctk_ib_rail_info_t *rail_ib )
{
	LOAD_CONFIG ( rail_ib );
	LOAD_DEVICE ( rail_ib );
	int ret = 0;

	/* Check if the memory used is correct */
#if 0

	if ( ( sctk_ib_prof_get_mem_used() + entry_nb * entry_size ) > IBV_MEM_USED_LIMIT )
	{
		sctk_nodebug ( "Cannot connect with RDMA because max memory used reached" );
		return 0;
	}

#endif

	/* Check if we do not have reached the maximum number of RDMA connections */
	sctk_spinlock_lock ( &rdma_lock );

	if ( config->max_rdma_connections >= ( device->eager_rdma_connections + 1 ) )
	{
		ret = ++device->eager_rdma_connections;
	}

	sctk_spinlock_unlock ( &rdma_lock );

	return ret;
}

void sctk_ibuf_rdma_connection_cancel ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote )
{
	LOAD_DEVICE ( rail_ib );

	sctk_spinlock_lock ( &rdma_lock );
	--device->eager_rdma_connections;
	sctk_spinlock_unlock ( &rdma_lock );

	/* We reset the state to 'reset' because we do not support the RDMA deconnection for now.
	 * In a next release, we need to change to deconnected */
	remote->rdma.messages_nb = 0;
	remote->rdma.messages_size = 0;
	remote->rdma.stats_lock = SCTK_SPINLOCK_INITIALIZER;
	OPA_incr_int ( &remote->rdma.cancel_nb );
	sctk_ibuf_rdma_set_remote_state_rts ( remote, STATE_DECONNECTED );

	sctk_ib_debug ( "[%d] OD QP RDMA connexion canceled to %d (rdma_connections:%d rdma_cancel:%d)",
	                rail_ib->rail->rail_number, remote->rank, device->eager_rdma_connections, remote->rdma.cancel_nb );

}

/*
 * Return the size allocated by the RDMA channel for a
 * remote and for a specific region (send or recv)
 */
size_t sctk_ibuf_rdma_region_get_allocated_size ( sctk_ib_qp_t *remote, int reg )
{

	sctk_ibuf_region_t *region = NULL;

	if ( reg == REGION_RECV )
	{
		sctk_endpoint_state_t state = sctk_ibuf_rdma_get_remote_state_rtr ( remote );

		if ( state != STATE_DECONNECTED )
		{
			region = &remote->rdma.pool->region[REGION_RECV];
		}
	}
	else
		if ( reg == REGION_SEND )
		{
			sctk_endpoint_state_t state = sctk_ibuf_rdma_get_remote_state_rts ( remote );

			if ( state != STATE_DECONNECTED )
			{
				region = &remote->rdma.pool->region[REGION_SEND];
			}
		}
		else
		{
			not_reachable();
		}

	/* If there is a region to analyze */
	if ( region )
	{
		return region->allocated_size;
	}

	return -1;
}
/*
 * Return the size allocated by the RDMA channel for a
 * remote (This function cummulate the size allocated
 * for the send AND for the receive regions)
 */
size_t sctk_ibuf_rdma_get_regions_get_allocate_size ( sctk_ib_qp_t *remote )
{
	size_t sum = 0;

	sum += sctk_ibuf_rdma_region_get_allocated_size ( remote, REGION_SEND );
	sum += sctk_ibuf_rdma_region_get_allocated_size ( remote, REGION_RECV );
	return sum;
}

/*
 * Update the maximum number of pending requests.
 * Should be called just before calling incr_requests_nb
 */
void sctk_ibuf_rdma_update_max_pending_data ( sctk_ib_qp_t *remote, int current_pending )
{
	sctk_spinlock_lock ( &remote->rdma.pending_data_lock );

	if ( current_pending > remote->rdma.max_pending_data )
	{
		remote->rdma.max_pending_data = current_pending;
	}

	sctk_spinlock_unlock ( &remote->rdma.pending_data_lock );
}

static int sctk_ibuf_rdma_determine_config ( sctk_ib_rail_info_t *rail_ib,
                                             sctk_ib_qp_t *remote, unsigned int *determined_size, int *determined_nb, char resizing )
{
	LOAD_CONFIG ( rail_ib );
	/* Compute the mean size of messages */
	int i, max;
	float mean = 0;
	int max_pending_data, max_pending_data_before_realign;

	max_pending_data = remote->rdma.max_pending_data;

	if ( resizing == 1 )
	{
		if ( max_pending_data <= remote->rdma.previous_max_pending_data )
		{
			return 0;
		}
	}

	sctk_spinlock_lock ( &remote->rdma.stats_lock );
	mean = ( remote->rdma.messages_size ) / ( float ) ( remote->rdma.messages_nb );
	sctk_spinlock_unlock ( &remote->rdma.stats_lock );

	/* Reajust the size according to the statistics */
	*determined_size = ( int ) mean;

	/* We determine the size of the slots */
	if ( resizing == 0 )
	{
		if ( *determined_size > config->rdma_max_size )
			*determined_size = config->rdma_max_size;
		else
			if ( *determined_size < config->rdma_min_size )
				*determined_size = config->rdma_min_size;
	}
	else
		/* Buffer resizing */
	{
		if ( *determined_size > config->rdma_resizing_max_size )
			*determined_size = config->rdma_resizing_max_size;
		else
			if ( *determined_size < config->rdma_resizing_min_size )
				*determined_size = config->rdma_resizing_min_size;
	}

	/* Realign */
	max_pending_data_before_realign = max_pending_data;
	max_pending_data = ALIGN_ON ( max_pending_data, *determined_size );
	*determined_nb = ( max_pending_data / ( float ) ( *determined_size ) );
//  *determined_nb *= 1.1;

	/* Realign on 64 bits and adjust with the size of the headers */
	*determined_size = ALIGN_ON ( ( *determined_size + sizeof ( sctk_thread_ptp_message_body_t ) +
	                                IBUF_GET_EAGER_SIZE + IBUF_RDMA_GET_SIZE ), 1024 );

	if ( resizing == 0 )
	{
		if ( *determined_nb > config->rdma_max_nb )
			*determined_nb = config->rdma_max_nb;
		else
			if ( *determined_nb < config->rdma_min_nb )
				*determined_nb = config->rdma_min_nb;
	}
	else
		/* Buffer resizing */
	{
		if ( *determined_nb > config->rdma_resizing_max_nb )
			*determined_nb = config->rdma_resizing_max_nb;
		else
			if ( *determined_nb < config->rdma_resizing_min_nb )
				*determined_nb = config->rdma_resizing_min_nb;
	}

	if ( resizing == 1 )
	{
		unsigned int previous_size = remote->rdma.pool->region[REGION_SEND].size_ibufs;
		int previous_nb   = remote->rdma.pool->region[REGION_SEND].nb;

		if ( previous_nb > *determined_nb )
			*determined_nb = previous_nb;

		if ( previous_size > *determined_size )
			*determined_size = previous_size;

		sctk_nodebug ( "Max pending: %d->%d", remote->rdma.previous_max_pending_data, max_pending_data_before_realign );
		//sctk_debug("Mean message size to remote %d kB, pending:%d->%dkB, previous max pending:%dkB (size:%d->%d nb:%d->%d)", (int) mean / 1024, max_pending_data_before_realign / 1024, max_pending_data / 1024,
		// remote->rdma.previous_max_pending_data/1024, previous_size/1024, *determined_size/1024, previous_nb,  *determined_nb);

		if ( previous_size >= *determined_size && previous_nb >= *determined_nb )
		{
			return 0;
		}
	}

	remote->rdma.previous_max_pending_data = max_pending_data_before_realign;
	return 1;
}

void sctk_ibuf_rdma_update_remote ( sctk_ib_qp_t *remote, size_t size )
{

	size_t messages_nb;
	sctk_spinlock_lock ( &remote->rdma.stats_lock );
	size_t old_messages_size = remote->rdma.messages_size;
	size_t new_messages_size = old_messages_size + size;
	remote->rdma.messages_size += size;
	messages_nb = ++remote->rdma.messages_nb;

	/* Reset counters */
	if ( expect_false ( new_messages_size < old_messages_size ) )
	{
		remote->rdma.messages_nb = 1;
		remote->rdma.messages_size = size;
	}

	sctk_spinlock_unlock ( &remote->rdma.stats_lock );
}

/*
* Check if we need to change the RDMA state of a remote
*/
void sctk_ibuf_rdma_check_remote ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote )
{
	LOAD_CONFIG ( rail_ib );
	int res;
	int iter;
	unsigned int determined_size;
	int determined_nb;
	/* We first get the state of the route */
	const sctk_endpoint_state_t state_rts = sctk_ibuf_rdma_get_remote_state_rts ( remote );

	/* We profile only when the RDMA route is deconnected */
	if ( config->max_rdma_connections != 0 && state_rts == STATE_DECONNECTED )
	{
		size_t messages_nb;
		sctk_spinlock_lock ( &remote->rdma.stats_lock );
		messages_nb = remote->rdma.messages_nb;
		sctk_spinlock_unlock ( &remote->rdma.stats_lock );

		/* Check if we need to connect using RDMA */
		if ( messages_nb >= IBV_RDMA_THRESHOLD )
		{

			/* Check if we can connect using RDMA. If == 1, only one thread can have an access to
			 * this code's part */
			if ( sctk_ib_cm_on_demand_rdma_check_request ( rail_ib->rail, remote ) == 1 )
			{

				sctk_ibuf_rdma_determine_config ( rail_ib, remote, &determined_size, &determined_nb, 0 );

				sctk_nodebug ( "Trying to connect remote %d using RDMA ( messages_nb: %d, determined: %d, pending: %d, iter: %d)", remote->rank, messages_nb, determined_size,
				               determined_nb );

				/* Sending the request. If the request has been sent, we reinit */
				sctk_ib_cm_on_demand_rdma_request ( rail_ib->rail, remote,
				                                    determined_size, determined_nb );
			}
		}
	}
	else
		if ( config->rdma_resizing && state_rts == STATE_CONNECTED )
		{
			sctk_endpoint_state_t ret;

			/* Check if we need to resize the RDMA */
			if ( OPA_load_int ( &remote->rdma.miss_nb ) > IBV_RDMA_MAX_MISS )
			{
				sctk_nodebug ( "MAX MISS REACHED busy:%d", OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_SEND] ) );
				/* Try to change the state to flushing.
				 * By changing the state of the remote to 'flushing', we automaticaly switch to SR */
				sctk_spinlock_lock ( &remote->rdma.flushing_lock );
				ret = sctk_ibuf_rdma_cas_remote_state_rts ( remote, STATE_CONNECTED, STATE_FLUSHING );

				if ( ret == STATE_CONNECTED )
				{
					/* Compute the next slots values */
					unsigned int next_size;
					int next_nb, previous_size, previous_nb;
					/* Reset the counter */
					OPA_store_int ( &remote->rdma.miss_nb, 0 );

					if ( sctk_ibuf_rdma_determine_config ( rail_ib, remote, &next_size, &next_nb, 1 ) == 1 )
					{
						/* Update the slots values requested */
						remote->rdma.pool->resizing_request.send_keys.nb   = next_nb;
						remote->rdma.pool->resizing_request.send_keys.size = next_size;
						sctk_nodebug ( "Resizing the RMDA buffer for remote %d (%d->%d %d->%d)", remote->rank, previous_nb, next_nb, previous_size, next_size );
						sctk_spinlock_unlock ( &remote->rdma.flushing_lock );

						sctk_ibuf_rdma_check_flush_send ( rail_ib, remote );
					}
					else
					{
						sctk_nodebug ( "NO MODIFICATION Resizing the RMDA buffer for remote %d (%d->%d %d->%d)", remote->rank, previous_nb, next_nb, previous_size, next_size );
						/* We reset the connection to connected */
						ret = sctk_ibuf_rdma_cas_remote_state_rts ( remote, STATE_FLUSHING, STATE_CONNECTED );
						assume ( ret == STATE_FLUSHING );
						sctk_spinlock_unlock ( &remote->rdma.flushing_lock );
					}
				}
				else
				{
					sctk_spinlock_unlock ( &remote->rdma.flushing_lock );
				}
			}
		}
}

/*
 * Fill RDMA connection keys with the configuration of a send and receive region
 */
void
sctk_ibuf_rdma_fill_remote_addr ( sctk_ib_qp_t *remote,
                                  sctk_ib_cm_rdma_connection_t *keys, int region )
{

	ib_assume ( remote->rdma.pool );
	keys->addr = remote->rdma.pool->region[region].buffer_addr;
	keys->rkey = remote->rdma.pool->region[region].mmu_entry->mr->rkey;

	sctk_nodebug ( "Filled keys addr=%p rkey=%u",
	               keys->addr, keys->rkey, keys->connected );
}

/*
 * Update a remote from the keys received by a peer. This function must be
 * called before sending any message to the RDMA channel
 */
void
sctk_ibuf_rdma_update_remote_addr ( sctk_ib_qp_t *remote, sctk_ib_cm_rdma_connection_t *key, int region )
{
	sctk_nodebug ( "REMOTE addr updated:: addr=%p rkey=%u (connected: %d)",
	               key->addr, key->rkey, key->connected );

	ib_assume ( remote->rdma.pool );
	/* Update keys for send and recv regions. We need to register the send region
	 * because of the piggyback */
	remote->rdma.pool->remote_region[region] = key->addr;
	remote->rdma.pool->rkey[region] = key->rkey;
}

/*
 * Return the RDMA eager ibufs limit for
 * sending a message
 */
size_t sctk_ibuf_rdma_get_eager_limit ( sctk_ib_qp_t *remote )
{
	return ( remote->rdma.pool->region[REGION_SEND].size_ibufs );
}


/*-----------------------------------------------------------
 *                          FLUSH
 *----------------------------------------------------------*/

/*
 * Check if a remote is in a flushing state. If it is, it
 * sends a flush REQ to the process
 */
int sctk_ibuf_rdma_check_flush_send ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote )
{
	/* Check if the RDMA is in flushing mode and if all messages
	 * have be flushed to the network */
	sctk_endpoint_state_t ret = sctk_ibuf_rdma_get_remote_state_rts ( remote );

	/* If we are in a flushing mode */
	if ( ret == STATE_FLUSHING )
	{
		int busy_nb = OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_SEND] );

		if ( busy_nb == 0 )
		{
			sctk_spinlock_lock ( &remote->rdma.flushing_lock );
			ret = sctk_ibuf_rdma_cas_remote_state_rts ( remote, STATE_FLUSHING, STATE_FLUSHED );

			if ( ret == STATE_FLUSHING )
			{
				sctk_ib_debug ( "SEND DONE Trying to flush RDMA for for remote %d", remote->rank );
				/* All the requests have been flushed. We can now send the resizing request.
				 * NOTE: we are sure that only one thread call the resizing request */
				sctk_ib_nodebug ( "Sending a FLUSH message to remote %d", remote->rank );

				/* Send the request. We need to lock in order to be sure that we do not
				 * emit a resizing request before changing the size_slots and the size and
				 * the number of slots */
				int size_ibufs = remote->rdma.pool->resizing_request.send_keys.size;
				int nb = remote->rdma.pool->resizing_request.send_keys.nb;
				sctk_spinlock_unlock ( &remote->rdma.flushing_lock );

				sctk_ib_cm_resizing_rdma_request ( rail_ib->rail, remote,
				                                   size_ibufs, nb );
				return 1;
			}
			else
			{
				sctk_spinlock_unlock ( &remote->rdma.flushing_lock );
			}
		}
		else
		{
			sctk_nodebug ( "Nb pending: %d", busy_nb );
		}
	}

	return 0;
}


/*
 * Check if a remote is in a flushing state. If it is, it
 * sends a flush REQ to the process
 */
int sctk_ibuf_rdma_check_flush_recv ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote )
{
	/* Check if the RDMA is in flushing mode and if all messages
	 * have be flushed to the network */
	sctk_endpoint_state_t ret = sctk_ibuf_rdma_get_remote_state_rtr ( remote );

	/* If we are in a flushing mode */
	if ( ret == STATE_FLUSHING )
	{
		int busy_nb = OPA_load_int ( &remote->rdma.pool->busy_nb[REGION_RECV] );
		ib_assume ( busy_nb >= 0 );
		sctk_nodebug ( "Number pending: %d", busy_nb );

		if ( busy_nb == 0 )
		{
			/* We lock the region during the state modification to 'flushed'.
			 * If we do not this, the RDMA buffer may be read while the RDMA buffer
			 * is in a 'flushed' state */
			IBUF_RDMA_LOCK_REGION ( remote, REGION_RECV );
			ret = sctk_ibuf_rdma_cas_remote_state_rtr ( remote, STATE_FLUSHING, STATE_FLUSHED );
			IBUF_RDMA_UNLOCK_REGION ( remote, REGION_RECV );

			if ( ret == STATE_FLUSHING )
			{
				sctk_ib_nodebug ( "RECV DONE Trying to flush RDMA for for remote %d", remote->rank );
				/* All the requests have been flushed. We can now send the resizing request.
				 * NOTE: we are sure that only one thread call the resizing request */

				/* Resizing the RDMA buffer */
				sctk_ibuf_rdma_region_reinit ( rail_ib, remote,
				                               &remote->rdma.pool->region[REGION_RECV],
				                               RDMA_CHANNEL | RECV_CHANNEL,
				                               remote->rdma.pool->resizing_request.recv_keys.nb,
				                               remote->rdma.pool->resizing_request.recv_keys.size );

				/* Fill the keys */
				sctk_ib_cm_rdma_connection_t *send_keys = &remote->rdma.pool->resizing_request.send_keys;
				sctk_ibuf_rdma_fill_remote_addr ( remote, send_keys, REGION_RECV );
				send_keys->connected = 1;

				sctk_ib_nodebug ( "Sending a FLUSH ACK message to remote %d", remote->rank );
				sctk_ib_cm_resizing_rdma_ack ( rail_ib->rail, remote, send_keys );
				return 1;
			}
		}
		else
		{
			sctk_nodebug ( "Nb pending: %d", busy_nb );
		}
	}

	return 0;
}



void sctk_ibuf_rdma_flush_recv ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote )
{
	int ret = -1;
	sctk_ib_nodebug ( "RECV Trying to flush RDMA for for remote %d", remote->rank );

	/*
	 * Flush the RDMA buffers.
	 */
	do
	{
		ret = sctk_ib_rdma_eager_poll_remote_locked ( rail_ib, remote );
	}
	while ( ret != REORDER_NOT_FOUND );

	/* We need to change the state AFTER flushing the buffers */
	sctk_endpoint_state_t state;
	state = sctk_ibuf_rdma_cas_remote_state_rtr ( remote, STATE_CONNECTED, STATE_FLUSHING );

	/* If we are in a requesting state, we swap to STATE_FLUSHING mode */
	if ( state != STATE_FLUSHING )
	{
		state = sctk_ibuf_rdma_cas_remote_state_rtr ( remote, STATE_REQUESTING, STATE_FLUSHING );
	}

	sctk_ibuf_rdma_check_flush_recv ( rail_ib, remote );
}


/*-----------------------------------------------------------
 *  RDMA disconnection
 *----------------------------------------------------------*/
/*
 * DANGER: no lock is taken during this call. The parent calling function
 * must take locks
 */
void sctk_ibuf_rdma_get_allocated_size_from_all_remotes (
    size_t *allocated_size,
    int *regions_nb )
{
	sctk_ibuf_region_t *region;
	sctk_endpoint_state_t state;

	*allocated_size = 0;
	*regions_nb      = 0;

	if ( rdma_region_list != NULL )
	{
		region = rdma_region_list;

		CDL_FOREACH ( rdma_region_list, region )
		{
			/* Get the state associated to the region */
			if ( region->channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
			{
				state = sctk_ibuf_rdma_get_remote_state_rts ( region->remote );
			}
			else
				if ( region->channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
				{
					state = sctk_ibuf_rdma_get_remote_state_rtr ( region->remote );
				}
				else
					not_reachable();

			/* If state connected */
			if ( state == STATE_CONNECTED )
			{
				*allocated_size += region->allocated_size;
				/* pas utilisé
					*regions_nb++;
				*/
			}
		}
	}
}

/*
 *  Normalize RDMA buffers
 */
int sctk_ibuf_rdma_remote_normalize ( sctk_ib_rail_info_t *rail_ib, size_t mem_to_save )
{
	size_t allocated_size;
	int regions_nb;
	double average_size;
	sctk_ibuf_region_t *region;
	sctk_endpoint_state_t state;
	sctk_ib_qp_t *remote;

	sctk_spinlock_read_lock ( &rdma_region_list_lock );
	sctk_ibuf_rdma_get_allocated_size_from_all_remotes ( &allocated_size,
	                                                     &regions_nb );

	/* If we want to save more memory than
	 * the memory used for RDMA buffers, we should disconnect every remote */
	if ( mem_to_save > allocated_size )
	{
		sctk_spinlock_read_unlock ( &rdma_region_list_lock );
		return 0;
	}

	average_size = ( allocated_size - mem_to_save ) / ( float ) regions_nb;

	if ( rdma_region_list != NULL )
	{
		region = rdma_region_list;

		CDL_FOREACH ( rdma_region_list, region )
		{
			remote = region->remote;

			/* Get the state associated to the region */
			if ( region->channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
			{
				state = sctk_ibuf_rdma_get_remote_state_rts ( remote );

				if ( state == STATE_CONNECTED && ( average_size < region->allocated_size ) )
				{
					/* Try to change the state to flushing.
					 * By changing the state of the remote to 'flushing', we automaticaly switch to SR */
					sctk_spinlock_lock ( &remote->rdma.flushing_lock );
					state = sctk_ibuf_rdma_cas_remote_state_rts ( remote, STATE_CONNECTED, STATE_FLUSHING );

					/* If we are allowed to deconnect */
					if ( state == STATE_CONNECTED )
					{
						/* Update the slots values requested to 0 -> means that we want to disconnect */
						remote->rdma.pool->resizing_request.send_keys.nb   = 0;
						remote->rdma.pool->resizing_request.send_keys.size = 0;
						sctk_spinlock_unlock ( &remote->rdma.flushing_lock );

						sctk_ib_debug ( "Resizing the RMDA buffer for remote %d", remote->rank );
						sctk_ibuf_rdma_check_flush_send ( rail_ib, remote );
					}
					else
					{
						sctk_spinlock_unlock ( &remote->rdma.flushing_lock );
					}
				}

			}
			else
				if ( region->channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
				{
					state = sctk_ibuf_rdma_get_remote_state_rtr ( remote );

					if ( state == STATE_CONNECTED && ( average_size < region->allocated_size ) )
					{
						/* Change the state to 'requesting' */
						state = sctk_ibuf_rdma_cas_remote_state_rtr ( remote, STATE_CONNECTED, STATE_REQUESTING );

					}
					else
						not_reachable();
				}
		}
	}

	sctk_spinlock_read_unlock ( &rdma_region_list_lock );
	return 1;
}



sctk_ibuf_region_t *sctk_ibuf_rdma_remote_get_lru ( char *name )
{
	sctk_ibuf_region_t *region;
	sctk_ibuf_region_t *elected_region = NULL;
	sctk_endpoint_state_t state;
	sprintf ( name, "LRU" );

	sctk_spinlock_read_lock ( &rdma_region_list_lock );

	if ( rdma_region_list != NULL )
	{
		region = rdma_region_list;

		/* Infinite loops are so nice */
		while ( 1 )
		{
			char found_connected = 0;
			CDL_FOREACH ( rdma_region_list, region )
			{
				/* Get the state associated to the region */
				if ( region->channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
				{
					state = sctk_ibuf_rdma_get_remote_state_rts ( region->remote );
				}
				else
					if ( region->channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
					{
						state = sctk_ibuf_rdma_get_remote_state_rtr ( region->remote );
					}
					else
						not_reachable();

				/* If state connected */
				if ( state == STATE_CONNECTED )
				{
					found_connected = 1;

					/* The RDMA buffer can be evicted */
					if ( region->R_bit == 0 )
					{
						elected_region = region;
						goto exit;
					}
					else /* == 1 */
					{
						region->R_bit = 0;
						region = region->next;
					}
				}
			}

			/* There is no remote connected. So we can leave... */
			if ( found_connected == 0 )
				goto exit;
		}
	}

exit:
	sctk_spinlock_read_unlock ( &rdma_region_list_lock );

	/* Return the elected region */
	return elected_region;
}



/*
 * Return the remote which is the most memory consumming in RDMA.
 * It may return NULL
 * Returns: the remote to disconnect
 * memory_used: total memory used for the RDMA connection (in kB)
 * */
sctk_ibuf_region_t *sctk_ibuf_rdma_remote_get_max_allocated_size ( char *name )
{
	sctk_ibuf_region_t *region;
	size_t max = 0;
	sctk_ibuf_region_t *max_region = NULL;
	sctk_endpoint_state_t state;
	sprintf ( name, "EMERGENCY" );

	sctk_spinlock_read_lock ( &rdma_region_list_lock );
	CDL_FOREACH ( rdma_region_list, region )
	{
		size_t current = ( ~0 );

		/* Get the state associated to the region */
		if ( region->channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
		{
			state = sctk_ibuf_rdma_get_remote_state_rts ( region->remote );
		}
		else
			if ( region->channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
			{
				state = sctk_ibuf_rdma_get_remote_state_rtr ( region->remote );
			}
			else
				not_reachable();

		/* If the region is connected, we can deconnect */
		if ( state == STATE_CONNECTED )
		{
			current = region->allocated_size;

			if ( current > max )
			{
				max = current;
				max_region = region;
			}
		}
	}
	sctk_spinlock_read_unlock ( &rdma_region_list_lock );

	return max_region;
}


/*
 *  Disconnect RDMA buffers
 */
size_t sctk_ibuf_rdma_remote_disconnect ( sctk_ib_rail_info_t *rail_ib )
{
	size_t memory_used = ( ~0 );
	char name[256] = "";
	sctk_ibuf_region_t *region;

	/* MAX MEM */
	//region = sctk_ibuf_rdma_remote_get_max_allocated_size( name);
	/* LRU */
	region = sctk_ibuf_rdma_remote_get_lru ( name );
	/* NORMALIZATION */

	/* If we find a process to deconnect, we deconnect it */
	if ( region != NULL )
	{
		sctk_ib_qp_t *remote;
		sctk_nodebug ( "%s %d -> %.02f",
		               name, region->remote->rank, region->allocated_size );

		remote = region->remote;
		memory_used = region->allocated_size;

		if ( region->channel == ( RDMA_CHANNEL | SEND_CHANNEL ) )
		{
			/* Try to change the state to flushing.
			 * By changing the state of the remote to 'flushing', we automaticaly switch to SR */
			sctk_spinlock_lock ( &remote->rdma.flushing_lock );
			sctk_endpoint_state_t ret =
			    sctk_ibuf_rdma_cas_remote_state_rts ( remote, STATE_CONNECTED, STATE_FLUSHING );

			/* If we are allowed to deconnect */
			if ( ret == STATE_CONNECTED )
			{
				/* Update the slots values requested to 0 -> means that we want to disconnect */
				remote->rdma.pool->resizing_request.send_keys.nb   = 0;
				remote->rdma.pool->resizing_request.send_keys.size = 0;
				sctk_spinlock_unlock ( &remote->rdma.flushing_lock );

				sctk_ib_debug ( "Resizing the RMDA buffer for remote %d", remote->rank );
				sctk_ibuf_rdma_check_flush_send ( rail_ib, remote );
			}
			else
			{
				sctk_spinlock_unlock ( &remote->rdma.flushing_lock );
				memory_used = 0;
			}
		}
		else
			if ( region->channel == ( RDMA_CHANNEL | RECV_CHANNEL ) )
			{
				/* Change the state to 'requesting' */
				sctk_endpoint_state_t ret =
				    sctk_ibuf_rdma_cas_remote_state_rtr ( remote, STATE_CONNECTED, STATE_REQUESTING );
				memory_used = 0;
			}
			else
				not_reachable();
	}

	return memory_used;
}



#endif
