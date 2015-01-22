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
#define SCTK_IB_MODULE_NAME "IBUF"
#include "sctk_multirail_ib.h"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_config.h"
#include "sctk_ib_topology.h"
#include "sctk_ib_prof.h"
#include "sctk_ibufs_rdma.h"
#include "utlist.h"

static sctk_ibuf_numa_t *sctk_ibuf_get_closest_node ( struct sctk_ib_rail_info_s *rail_ib )
{
	struct sctk_ib_topology_numa_node_s *closest_node = sctk_ib_topology_get_numa_node ( rail_ib );
	sctk_ibuf_numa_t *node = NULL;

	node = &closest_node->ibufs;

	return node;
}



/* Init a poll of buffers on the numa node pointed by 'node'
 * FIXME: use malloc_on_node instead of memalign
 * Carreful: this function is *NOT* thread-safe */
void sctk_ibuf_init_numa_node ( struct sctk_ib_rail_info_s *rail_ib,
                                struct sctk_ibuf_numa_s *node,
                                int nb_ibufs,
                                char is_initial_allocation )
{
	LOAD_CONFIG ( rail_ib );
	LOAD_POOL ( rail_ib );
	sctk_ib_mmu_t *mmu = node->mmu;
	sctk_ibuf_region_t *region = NULL;
	void *ptr = NULL;
	void *ibuf;
	sctk_ibuf_t *ibuf_ptr;
	int i;
	int free_nb;

	/* If this allocation is done during initialization */
	if ( is_initial_allocation )
	{
		if ( pool->default_node == NULL )
		{
			pool->default_node = node;
		}

		node->lock = SCTK_SPINLOCK_INITIALIZER;
		node->srq_cache_lock = SCTK_SPINLOCK_INITIALIZER;
		node->regions = NULL;
		node->free_entry = NULL;
		node->free_srq_cache = NULL;
		OPA_store_int ( &node->free_srq_nb, 0 );
		node->free_srq_cache_nb = 0;
		OPA_store_int ( &node->free_nb, 0 );
	}

	region = sctk_malloc_on_node ( sizeof ( sctk_ibuf_region_t ), 0 );
	ib_assume ( region );

	/* XXX: replaced by memalign_on_node */
	sctk_posix_memalign ( ( void ** ) &ptr, mmu->page_size, nb_ibufs * config->eager_limit );
	ib_assume ( ptr );
	memset ( ptr, 0, nb_ibufs * config->eager_limit );
	PROF_ADD ( rail_ib->rail, ib_ibuf_sr_size, nb_ibufs * config->eager_limit );

	/* XXX: replaced by memalign_on_node */
	sctk_posix_memalign ( &ibuf, mmu->page_size, nb_ibufs * sizeof ( sctk_ibuf_t ) );
	ib_assume ( ibuf );
	memset ( ibuf, 0, nb_ibufs * sizeof ( sctk_ibuf_t ) );
	PROF_ADD ( rail_ib->rail, ib_ibuf_sr_size, nb_ibufs * sizeof ( sctk_ibuf_t ) );

	region->size_ibufs = config->eager_limit;
	region->list = ibuf;
	region->nb = nb_ibufs;
	region->node = node;
	region->rail = rail_ib;
	region->channel = RC_SR_CHANNEL;
	region->ibuf = ibuf;
	region->allocated_size = ( nb_ibufs * ( config->eager_limit +  sizeof ( sctk_ibuf_t ) ) );
	DL_APPEND ( node->regions, region );

	/* register buffers at once
	* FIXME: Always task on NUMA node 0 which firs-touch all pages... really bad */
	region->mmu_entry = sctk_ib_mmu_register_no_cache ( rail_ib, ptr, nb_ibufs * config->eager_limit );
	sctk_nodebug ( "Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey );

	/* init all buffers - the last one */
	for ( i = 0; i < nb_ibufs; ++i )
	{
		ibuf_ptr = ( sctk_ibuf_t * ) ibuf + i;
		ibuf_ptr->region = region;
		ibuf_ptr->size = 0;
		ibuf_ptr->flag = FREE_FLAG;
		ibuf_ptr->index = i;

		ibuf_ptr->buffer = ( unsigned char * ) ( ( char * ) ptr + ( i * config->eager_limit ) );
		ib_assume ( ibuf_ptr->buffer );
		DL_APPEND ( node->free_entry, ( ( sctk_ibuf_t * ) ibuf + i ) );
	}

	OPA_add_int ( &node->free_nb, nb_ibufs );
	free_nb = OPA_load_int ( &node->free_nb );
	node->nb += nb_ibufs;

	sctk_ib_debug ( "Allocation of %d buffers (free_nb:%u got:%u)", nb_ibufs, free_nb, node->nb - free_nb );
}

void sctk_ibuf_pool_set_node_srq_buffers ( struct sctk_ib_rail_info_s *rail_ib,
                                           sctk_ibuf_numa_t *node )
{
	LOAD_POOL ( rail_ib );
	pool->node_srq_buffers = node;
	node->is_srq_pool = 1;
}

/* Init the pool of buffers on each NUMA node */
void sctk_ibuf_pool_init ( struct sctk_ib_rail_info_s *rail_ib )
{
	sctk_ibuf_pool_t *pool;

	pool = sctk_malloc ( sizeof ( sctk_ibuf_pool_t ) );
	/* WARNING: Do not remove the following memset.
	* Be sure that is_srq_poll is set to 0 !! */
	memset ( pool, 0, sizeof ( sctk_ibuf_pool_t ) );
	pool->post_srq_lock = SCTK_SPINLOCK_INITIALIZER;
	/* update pool buffer */
	rail_ib->pool_buffers = pool;
}

sctk_ibuf_t *sctk_ibuf_pick_send_sr ( struct sctk_ib_rail_info_s *rail_ib )
{
	LOAD_CONFIG ( rail_ib );
	sctk_ibuf_t *ibuf;
	PROF_TIME_START ( rail_ib->rail, ib_pick_send_sr );
	sctk_ibuf_numa_t *node = sctk_ibuf_get_closest_node ( rail_ib );

	sctk_spinlock_t *lock = &node->lock;

	sctk_spinlock_lock ( lock );

	/* Allocate additionnal buffers if no more are available */
	if ( !node->free_entry )
	{
		sctk_ibuf_init_numa_node ( rail_ib, node, config->size_ibufs_chunk, 0 );
	}

	/* update pointers from buffer pool */
	ibuf = node->free_entry;
	DL_DELETE ( node->free_entry, node->free_entry );
	OPA_decr_int ( &node->free_nb );

	sctk_spinlock_unlock ( lock );

	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_NULL_PROTOCOL );

#ifdef DEBUG_IB_BUFS
	assume ( ibuf );

	if ( ibuf->flag != FREE_FLAG )
	{
		sctk_error ( "Wrong flag (%d) got from ibuf", ibuf->flag );
	}

#endif

	sctk_nodebug ( "ibuf: %p, lock:%p, need_lock:%d next free_entryr: %p, nb_free %d, nb_got %d, nb_free_srq %d, node %d)", ibuf, lock, need_lock, node->free_entry, node->nb_free, node->nb_got, node->nb_free_srq, n );

	/* Prepare the buffer for sending */
	IBUF_SET_POISON ( ibuf->buffer );
	PROF_INC ( rail_ib->rail, ib_ibuf_sr_nb );
	PROF_TIME_END ( rail_ib->rail, ib_pick_send_sr );

	return ibuf;
}


/*
 * Prepare a message to send. According to the size and the remote, the function
 * choose between the SR and the RDMA channel.
 * If '*size' == ULONG_MAX, the fonction returns in '*size' the maximum size
 * of the payload for the buffer. The user next needs to manually call sctk_ib_prepare.
 */
sctk_ibuf_t *sctk_ibuf_pick_send ( struct sctk_ib_rail_info_s *rail_ib,
                                   sctk_ib_qp_t *remote,
                                   size_t *size )
{
	LOAD_CONFIG ( rail_ib );
	sctk_ibuf_t *ibuf = NULL;
	size_t s;
	size_t limit;
	sctk_route_state_t state;
	PROF_TIME_START ( rail_ib->rail, ib_pick_send );

	s = *size;
	/***** RDMA CHANNEL *****/
	state = sctk_ibuf_rdma_get_remote_state_rts ( remote );

	if ( state == STATE_CONNECTED )
	{
		/* Double lock checking */
		sctk_spinlock_lock ( &remote->rdma.flushing_lock );
		state = sctk_ibuf_rdma_get_remote_state_rts ( remote );

		if ( state == STATE_CONNECTED )
		{
			/* WARNING: 'free_nb' must be decremented just after
			* checking the state of the RDMA buffer. */
			OPA_incr_int ( &remote->rdma.pool->busy_nb[REGION_SEND] );
			sctk_spinlock_unlock ( &remote->rdma.flushing_lock );

			limit = sctk_ibuf_rdma_get_eager_limit ( remote );
			sctk_nodebug ( "Eager limit: %lu", limit );

			if ( s == ULONG_MAX )
			{
				s = limit - IBUF_RDMA_GET_SIZE;
			}

			if ( ( IBUF_RDMA_GET_SIZE + s ) <= limit )
			{
				sctk_nodebug ( "requested:%lu max:%lu header:%lu", s, limit, IBUF_RDMA_GET_SIZE );
				ibuf = sctk_ibuf_rdma_pick ( rail_ib, remote );
				sctk_nodebug ( "Picked a rdma buffer: %p", ibuf );

				/* A buffer has been picked-up */
				if ( ibuf )
				{
					PROF_INC ( rail_ib->rail, ib_ibuf_rdma_nb );
					sctk_nodebug ( "Picking from RDMA %d", ibuf->index );
#ifdef DEBUG_IB_BUFS
					sctk_route_state_t state = sctk_ibuf_rdma_get_remote_state_rts ( remote );

					if ( ( state != STATE_CONNECTED ) && ( state != STATE_FLUSHING ) )
					{
						sctk_error ( "Got a wrong state : %d", state );
						not_reachable();
					}

#endif
					goto exit;
				}
				else
				{
					OPA_incr_int ( &remote->rdma.miss_nb );
					PROF_INC ( rail_ib->rail, ib_ibuf_rdma_miss_nb );
				}
			}
			else
			{
				OPA_incr_int ( &remote->rdma.miss_nb );
			}

			/* If we cannot pick a buffer from the RDMA channel, we switch to SR */
			OPA_decr_int ( &remote->rdma.pool->busy_nb[REGION_SEND] );
			sctk_ibuf_rdma_check_flush_send ( rail_ib, remote );

		}
		else
		{
			sctk_spinlock_unlock ( &remote->rdma.flushing_lock );
		}
	}

	s = *size;

	/***** SR CHANNEL *****/
	if ( s == ULONG_MAX )
		s = config->eager_limit;

	if ( s <= config->eager_limit )
	{
		sctk_nodebug ( "Picking from SR" );

		sctk_ibuf_numa_t *node = sctk_ibuf_get_closest_node ( rail_ib );
		sctk_spinlock_t *lock = &node->lock;

		sctk_spinlock_lock ( lock );

		/* Allocate additionnal buffers if no more are available */
		if ( !node->free_entry )
		{
			sctk_ibuf_init_numa_node ( rail_ib, node, config->size_ibufs_chunk, 0 );
		}

		/* update pointers from buffer pool */
		ibuf = node->free_entry;
		DL_DELETE ( node->free_entry, node->free_entry );
		OPA_decr_int ( &node->free_nb );

		sctk_spinlock_unlock ( lock );

		IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_NULL_PROTOCOL );
		PROF_INC ( rail_ib->rail, ib_ibuf_sr_nb );

#ifdef DEBUG_IB_BUFS
		assume ( ibuf );

		if ( ibuf->flag != FREE_FLAG )
		{
			sctk_error ( "Wrong flag (%d) got from ibuf", ibuf->flag );
			sctk_abort();
		}

#endif
		ibuf->flag = BUSY_FLAG;

		sctk_nodebug ( "ibuf: %p, lock:%p, need_lock:%d next free_entryr: %p, nb_free %d, nb_got %d, nb_free_srq %d, node %d)", ibuf, lock, need_lock, node->free_entry, node->nb_free, node->nb_got, node->nb_free_srq, n );

	}
	else
	{
		/* We can reach this block if there is no more slots for the RDMA channel and the
		* requested size is larger than the size of a SR slot. At this time, we switch can switch to the
		* buffered protocol */
		PROF_TIME_END ( rail_ib->rail, ib_pick_send );
		return NULL;
	}

exit:

	/* We update the size if it is equal to ULONG_MAX*/
	if ( *size == ULONG_MAX )
	{
		*size = s;
	}
	else
	{
		/* Prepare the buffer for sending */
		sctk_ibuf_prepare ( rail_ib, remote, ibuf, *size );
	}

	IBUF_SET_POISON ( ibuf->buffer );

	PROF_TIME_END ( rail_ib->rail, ib_pick_send );

	sctk_ibuf_rdma_check_remote ( rail_ib, remote );
	return ibuf;
}


/* pick a new buffer from the ibuf list. Function *MUST* be locked to avoid
 * oncurrent calls.
 * - remote: process where picking buffer. It may be NULL. In this case,
 *   we pick a buffer from the SR channel */
static inline sctk_ibuf_t *
sctk_ibuf_pick_recv ( struct sctk_ib_rail_info_s *rail_ib, struct sctk_ibuf_numa_s *node )
{
	LOAD_CONFIG ( rail_ib );
	sctk_ibuf_t *ibuf;
	PROF_TIME_START ( rail_ib->rail, ib_pick_recv );
	assume ( node );

	/* Allocate additionnal buffers if no more are available */
	if ( !node->free_entry )
	{
		sctk_ibuf_init_numa_node ( rail_ib, node, config->size_recv_ibufs_chunk, 0 );
	}

	/* update pointers from buffer pool */
	ibuf = node->free_entry;
	DL_DELETE ( node->free_entry, node->free_entry );
	OPA_decr_int ( &node->free_nb );



#ifdef DEBUG_IB_BUFS
	assume ( ibuf );

	if ( ibuf->flag != FREE_FLAG )
	{
		sctk_error ( "Wrong flag (%d) got from ibuf", ibuf->flag );
	}

	sctk_nodebug ( "ibuf: %p, lock:%p, need_lock:%d next free_entryr: %p, nb_free %d, nb_got %d, nb_free_srq %d, node %d)", ibuf, lock, need_lock, node->free_entry, node->nb_free, node->nb_got, node->nb_free_srq, node->id );
#endif
	PROF_TIME_END ( rail_ib->rail, ib_pick_recv );


	return ibuf;
}

/*
 * Post buffers to the SRQ.
 * FIXME: locks have been removed because useless.
 * Check if it aims to corruption.
 */
static int srq_post ( struct sctk_ib_rail_info_s *rail_ib,
                      sctk_ibuf_numa_t *node,
                      int force )
{
	assume ( node );
	LOAD_DEVICE ( rail_ib );
	LOAD_CONFIG ( rail_ib );
	int i;
	int nb_posted = 0;
	sctk_ibuf_t *ibuf;
	int rc;
	sctk_spinlock_t *lock = &node->lock;
	int nb_ibufs;
	int free_srq_nb;

	/* limit of buffer posted */
	free_srq_nb = OPA_load_int ( &node->free_srq_nb );
	nb_ibufs = config->max_srq_ibufs_posted - free_srq_nb;
	sctk_nodebug ( "TRY Post %d ibufs in SRQ (free:%d max:%d)", nb_ibufs, free_srq_nb, config->max_srq_ibufs_posted );


	if ( nb_ibufs <= 0 )
		return 0;

	PROF_TIME_START ( rail_ib->rail, ib_ibuf_srq_post );

	if ( force )
		sctk_spinlock_lock ( &rail_ib->pool_buffers->post_srq_lock );
	else
		if ( sctk_spinlock_trylock ( &rail_ib->pool_buffers->post_srq_lock ) != 0 )
			goto exit;

	/* Only 1 task can post to the SRQ at the same time. No need more concurrency */
	//  if (sctk_spinlock_trylock(&rail_ib->pool_buffers->post_srq_lock) == 0)
	{
		/* limit of buffer posted */
		free_srq_nb = OPA_load_int ( &node->free_srq_nb );
		nb_ibufs = config->max_srq_ibufs_posted - free_srq_nb;
		sctk_nodebug ( "Post %d ibufs in SRQ (free:%d max:%d force:%d)", nb_ibufs, free_srq_nb, config->max_srq_ibufs_posted, force );

		sctk_spinlock_lock ( lock );

		for ( i = 0; i < nb_ibufs; ++i )
		{
			/* No need lock */
			ibuf = sctk_ibuf_pick_recv ( rail_ib, node );
#ifdef DEBUG_IB_BUFS
			assume ( ibuf );
#endif

			sctk_ibuf_recv_init ( ibuf );
			rc = ibv_post_srq_recv ( device->srq, & ( ibuf->desc.wr.recv ), & ( ibuf->desc.bad_wr.recv ) );

			/* Cannot post more buffers */
			if ( rc != 0 )
			{
				ibuf->flag = FREE_FLAG;
				ibuf->in_srq = 0;

				/* change counters */
				OPA_incr_int ( &node->free_nb );
				DL_PREPEND ( node->free_entry, ibuf );
				break;
			}
			else
				nb_posted++;
		}

		sctk_spinlock_unlock ( lock );

		OPA_add_int ( &node->free_srq_nb, nb_posted );

		sctk_spinlock_unlock ( &rail_ib->pool_buffers->post_srq_lock );
	}
	PROF_TIME_END ( rail_ib->rail, ib_ibuf_srq_post );

exit:
	return nb_posted;
}

/* FIXME: check with buffers near IB card */
int sctk_ibuf_srq_check_and_post ( struct sctk_ib_rail_info_s *rail_ib )
{
	LOAD_POOL ( rail_ib );

	return srq_post ( rail_ib, pool->node_srq_buffers, 1 );
}

static inline void __release_in_srq ( struct sctk_ib_rail_info_s *rail_ib,
                                      sctk_ibuf_numa_t *node,
                                      sctk_ibuf_t *ibuf,
                                      int decr_free_srq_nb )
{

	if ( decr_free_srq_nb )
	{
		/* limit of buffer posted */
		OPA_decr_int ( &node->free_srq_nb );
	}

	srq_post ( rail_ib, node, 0 );
}

void sctk_ibuf_release_from_srq ( struct sctk_ib_rail_info_s *rail_ib, sctk_ibuf_t *ibuf )
{
	sctk_ibuf_numa_t *node = ibuf->region->node;

	__release_in_srq ( rail_ib, node, ibuf, 1 );
}

/* release one buffer given as parameter.
 * is_srq: if the buffer is released from the SRQ */
void sctk_ibuf_release ( struct sctk_ib_rail_info_s *rail_ib,
                         sctk_ibuf_t *ibuf,
                         int decr_free_srq_nb )
{
	ib_assume ( ibuf );

	if ( ibuf->to_release & IBUF_RELEASE )
	{
		if ( IBUF_GET_CHANNEL ( ibuf ) & RDMA_CHANNEL )
		{
			PROF_TIME_START ( rail_ib->rail, ib_ibuf_rdma_release );
			sctk_ibuf_rdma_release ( rail_ib, ibuf );
			PROF_TIME_END ( rail_ib->rail, ib_ibuf_rdma_release );
		}
		else
		{
			ib_assume ( IBUF_GET_CHANNEL ( ibuf ) == RC_SR_CHANNEL );
			sctk_ibuf_numa_t *node = ibuf->region->node;
			sctk_spinlock_t *lock = &node->lock;

			if ( ibuf->in_srq )
			{
				/* If buffer from SRQ */
				PROF_TIME_START ( rail_ib->rail, ib_ibuf_sr_srq_release );
				ibuf->flag = FREE_FLAG;
				IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_NULL_PROTOCOL );

				{
					sctk_ibuf_numa_t *closest_node = sctk_ibuf_get_closest_node ( rail_ib );
					sctk_spinlock_t *srq_cache_lock = &closest_node->srq_cache_lock;

					sctk_spinlock_lock ( srq_cache_lock );
					const int srq_cache_nb = ++ closest_node->free_srq_cache_nb;
					DL_APPEND ( closest_node->free_srq_cache, ibuf );

					TODO ( "Number of SRQ buffers in the cache -> in the configuration" )

					/* If the max number of SRQ ibufs is reached for the current node, we flush
					* the list to the global queue of buffers */
					if ( srq_cache_nb > 40 )
					{
						OPA_add_int ( &node->free_nb, srq_cache_nb );

						sctk_spinlock_lock ( lock );
						DL_CONCAT ( node->free_entry, closest_node->free_srq_cache );
						sctk_spinlock_unlock ( lock );

						closest_node->free_srq_cache_nb = 0;
						closest_node->free_srq_cache = NULL;
					}

					sctk_spinlock_unlock ( srq_cache_lock );
				}

				/* If SRQ, we check and try to post more messages to SRQ */
				__release_in_srq ( rail_ib, node, ibuf, decr_free_srq_nb );
				PROF_TIME_END ( rail_ib->rail, ib_ibuf_sr_srq_release );
			}
			else
			{
				/* TODO: only for debugging */
#if 0
				sctk_ibuf_numa_t *closest_node = sctk_ibuf_get_closest_node ( rail_ib );
				assume ( closest_node == node );
#endif
				PROF_TIME_START ( rail_ib->rail, ib_ibuf_sr_send_release );
				ibuf->flag = FREE_FLAG;
				IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_NULL_PROTOCOL );

				OPA_incr_int ( &node->free_nb );
				sctk_spinlock_lock ( lock );
				DL_APPEND ( node->free_entry, ibuf );
				sctk_spinlock_unlock ( lock );
				PROF_TIME_END ( rail_ib->rail, ib_ibuf_sr_send_release );
			}
		}
	}
	else
	{
		not_reachable();
	}
}

void sctk_ibuf_prepare ( sctk_ib_rail_info_t *rail_ib,
                         sctk_ib_qp_t *remote,
                         sctk_ibuf_t *ibuf,
                         size_t size )
{

	if ( IBUF_GET_CHANNEL ( ibuf ) & RDMA_CHANNEL )
	{
		const sctk_ibuf_region_t *region = IBUF_RDMA_GET_REGION ( remote, REGION_SEND );

		/* Initialization of the buffer */
		sctk_ibuf_rdma_write_init ( ibuf,
		                            IBUF_RDMA_GET_BASE_FLAG ( ibuf ), /* Local addr */
		                            region->mmu_entry->mr->lkey,  /* lkey */
		                            IBUF_RDMA_GET_REMOTE_ADDR ( remote, ibuf ), /* Remote addr */
		                            remote->rdma.pool->rkey[REGION_RECV],  /* rkey */
		                            size + IBUF_RDMA_GET_SIZE, /* size */
		                            IBV_SEND_SIGNALED, IBUF_RELEASE ); /* imm_data: index of the ibuf in the region */

		/* Move tail flag */
		sctk_ib_rdma_set_tail_flag ( ibuf, size );

	}
	else
	{
		ib_assume ( IBUF_GET_CHANNEL ( ibuf ) & RC_SR_CHANNEL );
		sctk_ibuf_send_inline_init ( ibuf, size );
	}
}

/*-----------------------------------------------------------
 *  WR INITIALIZATION
 *----------------------------------------------------------*/
void sctk_ibuf_recv_init ( sctk_ibuf_t *ibuf )
{
	LOAD_CONFIG ( ibuf->region->rail );

	ibuf->to_release = IBUF_RELEASE;
	ibuf->in_srq = 1;
	ibuf->send_imm_data = 0;
	/* For the SRQ, remote is set to 'NULL' as it is shared
	* between all processes */
	ibuf->remote = NULL;

	ibuf->desc.wr.send.next = NULL;
	ibuf->desc.wr.send.wr_id = ( uintptr_t ) ibuf;

	ibuf->desc.wr.send.num_sge = 1;
	ibuf->desc.wr.send.sg_list = & ( ibuf->desc.sg_entry );
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length = config->eager_limit;

	ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
	ibuf->desc.sg_entry.addr = ( uintptr_t ) ( ibuf->buffer );

	ibuf->flag = RECV_IBUF_FLAG;
}

void sctk_ibuf_barrier_send_init ( sctk_ibuf_t *ibuf,
                                   void *local_address,
                                   sctk_uint32_t lkey,
                                   void *remote_address,
                                   sctk_uint32_t rkey,
                                   int len )
{
	ibuf->to_release = IBUF_RELEASE;
	ibuf->in_srq = 0;
	ibuf->send_imm_data = 0;

	ibuf->desc.wr.send.next = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
	ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
	ibuf->desc.wr.send.wr_id = ( uintptr_t ) ibuf;

	ibuf->desc.wr.send.num_sge = 1;
	ibuf->desc.wr.send.wr.rdma.remote_addr = ( uintptr_t ) remote_address;
	ibuf->desc.wr.send.wr.rdma.rkey = rkey;

	ibuf->desc.wr.send.sg_list = & ( ibuf->desc.sg_entry );
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length = len;
	ibuf->desc.sg_entry.lkey = lkey;
	ibuf->desc.sg_entry.addr = ( uintptr_t ) local_address;

	ibuf->flag = BARRIER_IBUF_FLAG;
}

int sctk_ibuf_send_inline_init ( sctk_ibuf_t *ibuf, size_t size )
{
	LOAD_CONFIG ( ibuf->region->rail );
	int is_inlined = 0;

	/* If data may be inlined */
	if ( size <= config->max_inline )
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE | IBV_SEND_SIGNALED;
		ibuf->flag = SEND_INLINE_IBUF_FLAG;
		is_inlined = 1;
	}
	else
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
		ibuf->flag = SEND_IBUF_FLAG;
	}

	ibuf->in_srq = 0;
	ibuf->send_imm_data = 0;
	ibuf->to_release = IBUF_RELEASE;
	ibuf->desc.wr.send.next = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_SEND;
	ibuf->desc.wr.send.wr_id = ( uintptr_t ) ibuf;

	ibuf->desc.wr.send.num_sge = 1;

	ibuf->desc.wr.send.sg_list = & ( ibuf->desc.sg_entry );
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length = size;
	ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
	ibuf->desc.sg_entry.addr = ( uintptr_t ) ( ibuf->buffer );

	return is_inlined;
}

void sctk_ibuf_send_init ( sctk_ibuf_t *ibuf, size_t size )
{
	ibuf->in_srq = 0;
	ibuf->send_imm_data = 0;
	ibuf->to_release = IBUF_RELEASE;
	ibuf->desc.wr.send.next = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_SEND;
	ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
	ibuf->desc.wr.send.wr_id = ( uintptr_t ) ibuf;

	ibuf->desc.wr.send.num_sge = 1;

	ibuf->desc.wr.send.sg_list = & ( ibuf->desc.sg_entry );
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length = size;
	ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
	ibuf->desc.sg_entry.addr = ( uintptr_t ) ( ibuf->buffer );

	ibuf->flag = SEND_IBUF_FLAG;
}

int sctk_ibuf_rdma_write_init ( sctk_ibuf_t *ibuf,
                                void *local_address,
                                sctk_uint32_t lkey,
                                void *remote_address,
                                sctk_uint32_t rkey,
                                int len,
                                int send_flags,
                                char to_release )
{
	LOAD_CONFIG ( ibuf->region->rail );
	int is_inlined = 0;

	/* If data may be inlined */
	if ( len <= config->max_inline )
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE | send_flags;
		ibuf->flag = RDMA_WRITE_INLINE_IBUF_FLAG;
		is_inlined = 1;
	}
	else
	{
		ibuf->desc.wr.send.send_flags = send_flags;
		ibuf->flag = RDMA_WRITE_IBUF_FLAG;
	}

	ibuf->in_srq = 0;
	ibuf->send_imm_data = 0;
	ibuf->to_release = to_release;
	ibuf->desc.wr.send.next = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
	ibuf->desc.wr.send.wr_id = ( uintptr_t ) ibuf;

	ibuf->desc.wr.send.num_sge = 1;
	ibuf->desc.wr.send.wr.rdma.remote_addr = ( uintptr_t ) remote_address;
	ibuf->desc.wr.send.wr.rdma.rkey = rkey;

	ibuf->desc.wr.send.sg_list = & ( ibuf->desc.sg_entry );
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length = len;
	ibuf->desc.sg_entry.lkey = lkey;
	ibuf->desc.sg_entry.addr = ( uintptr_t ) local_address;

	return is_inlined;
}

int sctk_ibuf_rdma_write_with_imm_init ( sctk_ibuf_t *ibuf,
                                         void *local_address,
                                         sctk_uint32_t lkey,
                                         void *remote_address,
                                         sctk_uint32_t rkey,
                                         int len,
                                         char to_release,
                                         sctk_uint32_t imm_data )
{
	LOAD_CONFIG ( ibuf->region->rail );
	int is_inlined = 0;

	/* If data may be inlined */
	if ( len <= config->max_inline )
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE | IBV_SEND_SIGNALED;
		ibuf->flag = RDMA_WRITE_INLINE_IBUF_FLAG;
		is_inlined = 1;
	}
	else
	{
		ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
		ibuf->flag = RDMA_WRITE_IBUF_FLAG;
	}

	ibuf->in_srq = 0;
	ibuf->send_imm_data = imm_data;
	ibuf->to_release = to_release;
	ibuf->desc.wr.send.next = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
	ibuf->desc.wr.send.wr_id = ( uintptr_t ) ibuf;

	ibuf->desc.wr.send.num_sge = 1;
	ibuf->desc.wr.send.wr.rdma.remote_addr = ( uintptr_t ) remote_address;
	ibuf->desc.wr.send.wr.rdma.rkey = rkey;

	ibuf->desc.wr.send.sg_list = & ( ibuf->desc.sg_entry );
	ibuf->desc.wr.send.imm_data = imm_data;
	ibuf->desc.sg_entry.length = len;
	ibuf->desc.sg_entry.lkey = lkey;
	ibuf->desc.sg_entry.addr = ( uintptr_t ) local_address;

	return is_inlined;
}



void sctk_ibuf_rdma_read_init ( sctk_ibuf_t *ibuf,
                                void *local_address,
                                sctk_uint32_t lkey,
                                void *remote_address,
                                sctk_uint32_t rkey,
                                int len,
                                void *supp_ptr )
{
	ibuf->in_srq = 0;
	ibuf->send_imm_data = 0;
	ibuf->to_release = IBUF_RELEASE;
	ibuf->desc.wr.send.next = NULL;
	ibuf->desc.wr.send.opcode = IBV_WR_RDMA_READ;
	ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
	ibuf->desc.wr.send.wr_id = ( uintptr_t ) ibuf;

	ibuf->desc.wr.send.num_sge = 1;
	ibuf->desc.wr.send.wr.rdma.remote_addr = ( uintptr_t ) remote_address;
	ibuf->desc.wr.send.wr.rdma.rkey = rkey;

	ibuf->desc.wr.send.sg_list = & ( ibuf->desc.sg_entry );
	ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
	ibuf->desc.sg_entry.length = len;
	ibuf->desc.sg_entry.lkey = lkey;
	ibuf->desc.sg_entry.addr = ( uintptr_t ) local_address;

	ibuf->flag = RDMA_READ_IBUF_FLAG;
	ibuf->supp_ptr = supp_ptr;
}

void sctk_ibuf_print_rdma ( sctk_ibuf_t *ibuf, char *desc )
{
	sprintf ( desc, 	"region       :%p\n"
	          "buffer       :%p\n"
	          "size         :%lu\n"
	          "flag         :%s\n"
	          "remote       :%p\n"
	          "in_srq       :%d\n"
	          "next         :%p\n"
	          "prev         :%p\n"
	          "sg_entry.length :%u\n"
	          "local addr   :%p\n"
	          "remote addr  :%p\n", 	ibuf->region,
	          ibuf->buffer,
	          ibuf->size,
	          sctk_ibuf_print_flag ( ibuf->flag ),
	          ibuf->remote,
	          ibuf->in_srq,
	          ibuf->next,
	          ibuf->prev,
	          ibuf->desc.sg_entry.length,
	          ( void * ) ibuf->desc.sg_entry.addr,
	          ( void * ) ibuf->desc.wr.send.wr.rdma.remote_addr );
}


void sctk_ibuf_print ( sctk_ibuf_t *ibuf, char *desc )
{
	char *poison = ( IBUF_GET_HEADER ( ibuf->buffer )->poison != IBUF_POISON ) ? "NAK" : "OK";
	char channel[5];
	sctk_ibuf_channel_print ( channel, ibuf->region->channel );

	sprintf ( desc,	  "rail         :%d\n"
	          "region       :%p\n"
	          "channel type : %s\n"
	          "buffer       :%p\n"
	          "size         :%lu\n"
	          "flag         :%s\n"
	          "remote       :%p\n"
	          "in_srq       :%d\n"
	          "next         :%p\n"
	          "prev         :%p\n"
	          "sg_entry.length :%u\n"
	          "poison       :%s", 	  ibuf->region->rail->rail->rail_number,
	          ibuf->region,
	          channel,
	          ibuf->buffer,
	          ibuf->size,
	          sctk_ibuf_print_flag ( ibuf->flag ),
	          ibuf->remote,
	          ibuf->in_srq,
	          ibuf->next,
	          ibuf->prev,
	          ibuf->desc.sg_entry.length,
	          poison );
}

#endif


