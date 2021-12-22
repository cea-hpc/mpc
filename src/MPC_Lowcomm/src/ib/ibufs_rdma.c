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

#include "ibufs.h"

//#define MPC_LOWCOMM_IB_MODULE_DEBUG
#define MPC_LOWCOMM_IB_MODULE_NAME    "IBUFR"
#include "ibtoolkit.h"
#include "ibufs_rdma.h"
#include "ib.h"
#include "mpc_common_asm.h"
#include "ibeager.h"
#include "ibrdma.h"
#include "ibconfig.h"
#include "qp.h"
#include "ibeager.h"
#include "sctk_net_tools.h"
#include "utlist.h"

#include <sctk_alloc.h>


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
_mpc_lowcomm_ib_ibuf_rdma_pool_t *rdma_pool_list = NULL;
/* Elements to merge to the rdma_pool_list */
_mpc_lowcomm_ib_ibuf_rdma_pool_t *rdma_pool_list_to_merge = NULL;
/* Lock when adding and accessing the concat list */
static mpc_common_spinlock_t rdma_pool_list_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/* Linked list of regions */
static _mpc_lowcomm_ib_ibuf_region_t *rdma_region_list      = NULL;
static mpc_common_rwlock_t rdma_region_list_lock = MPC_COMMON_SPIN_RWLOCK_INITIALIZER;
/* Pointer for the clock algorithm */
static _mpc_lowcomm_ib_ibuf_region_t *clock_pointer = NULL;

static mpc_common_spinlock_t rdma_lock         = MPC_COMMON_SPINLOCK_INITIALIZER;
static mpc_common_rwlock_t   rdma_polling_lock = MPC_COMMON_SPIN_RWLOCK_INITIALIZER;

/*-----------------------------------------------------------
*  FUNCTIONS
*----------------------------------------------------------*/

/*
 * Init the remote for the RDMA connection
 */
//#warning "To reinit when disconnected"
void _mpc_lowcomm_ib_ibuf_rdma_remote_init(_mpc_lowcomm_ib_qp_t *remote)
{
	_mpc_lowcomm_ib_ibuf_rdma_set_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_DECONNECTED);
	_mpc_lowcomm_ib_ibuf_rdma_set_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_DECONNECTED);
	mpc_common_spinlock_init(&remote->rdma.pending_data_lock, 0);
	mpc_common_spinlock_init(&remote->rdma.lock, 0);
	mpc_common_spinlock_init(&remote->rdma.polling_lock, 0);
	/* Counters */
	remote->rdma.messages_nb      = 0;
	remote->rdma.max_pending_data = 0;
	OPA_store_int(&remote->rdma.miss_nb, 0);
	OPA_store_int(&remote->rdma.hits_nb, 0);
	OPA_store_int(&remote->rdma.resizing_nb, 0);
	OPA_store_int(&remote->rdma.cancel_nb, 0);
	/* Lock for resizing */
	mpc_common_spinlock_init(&remote->rdma.flushing_lock, 0);
}

static inline void __init_rdma_slots(_mpc_lowcomm_ib_ibuf_region_t *region,
                                     void *ptr, void *ibuf, int nb_ibufs, int size_ibufs)
{
	int          i;
	_mpc_lowcomm_ib_ibuf_t *ibuf_ptr = NULL;

	for(i = 0; i < nb_ibufs; ++i)
	{
		const char *buffer = ( char * )( ( char * )ptr + (i * size_ibufs) );
		ibuf_ptr             = ( _mpc_lowcomm_ib_ibuf_t * )ibuf + i;
		ibuf_ptr->region     = region;
		ibuf_ptr->size       = 0;
		ibuf_ptr->flag       = FREE_FLAG;
		ibuf_ptr->index      = i;
		ibuf_ptr->to_release = IBUF_RELEASE;

		/* Set flags  */
		ibuf_ptr->buffer           = IBUF_RDMA_GET_PAYLOAD_FLAG(buffer);
		ibuf_ptr->head_flag        = IBUF_RDMA_GET_HEAD_FLAG(buffer);
		ibuf_ptr->size_flag        = IBUF_RDMA_GET_SIZE_FLAG(buffer);
		ibuf_ptr->poison_flag_head = IBUF_RDMA_GET_POISON_FLAG_HEAD(buffer);
		/* Reset piggyback */
		IBUF_GET_EAGER_PIGGYBACK(ibuf_ptr->buffer) = -1;
		/* Set headers value */
		*(ibuf_ptr->head_flag) = IBUF_RDMA_RESET_FLAG;
		/* Set poison value */
		*(ibuf_ptr->poison_flag_head) = IBUF_RDMA_POISON_HEAD;

		DL_APPEND(region->list, ( ( _mpc_lowcomm_ib_ibuf_t * )ibuf + i) );
		mpc_common_nodebug("ibuf=%p, index=%d, buffer=%p, head_flag=%p, size_flag=%p",
		                   ibuf_ptr, i, ibuf_ptr->buffer, ibuf_ptr->head_flag, ibuf_ptr->size_flag);
	}
}

void __rdma_region_resize(struct _mpc_lowcomm_ib_rail_info_s *rail_ib, _mpc_lowcomm_ib_qp_t *remote,
                          _mpc_lowcomm_ib_ibuf_region_t *region, enum _mpc_lowcomm_ib_ibuf_channel_t channel, int nb_ibufs, int size_ibufs)
{
	void *ptr = NULL;
	void *ibuf;

	ib_assume(nb_ibufs > 0);
	ib_assume(remote->rdma.pool);

	/* Because we are reinitializing a RDMA region, be sure that a previous allocation
	 * has been made */
	assume(region->buffer_addr);

	/* Check if all ibufs are really free */
#ifdef IB_DEBUG
	int busy;
    int i;
    _mpc_lowcomm_ib_ibuf_t * ibuf_ptr;

	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
	/* SEND CHANNEL */
	{
		if( (busy = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]) ) != 0)
		{
			mpc_common_debug_error("Buffer with wrong number in SEND channel (busy_nb:%d)", busy);
			not_reachable();
		}
	}
	else
	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
	/* RECV CHANNEL */
	{
		if( (busy = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_RECV]) ) != 0)
		{
			mpc_common_debug_error("Buffer with wrong numer in RECV channel (busy_nb:%d)", busy);
			not_reachable();
		}
	}

	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
	/* SEND CHANNEL */
	{
		for(i = 0; i < region->nb; ++i)
		{
			ibuf_ptr = ( _mpc_lowcomm_ib_ibuf_t * )region->ibuf + i;

			if(ibuf_ptr->flag != FREE_FLAG)
			{
				mpc_common_debug_error("Buffer %d (tail:%d) with wrong flag: %d in RECV channel (busy_nb:%d)", ibuf_ptr->index, region->tail->index,
				                       ibuf_ptr->flag,
				                       OPA_load_int(&remote->rdma.pool->busy_nb[REGION_RECV]) );
			}

			assume(*(ibuf_ptr->head_flag) == IBUF_RDMA_RESET_FLAG);
		}
	}
	else
	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
	/* SEND CHANNEL */
	{
		for(i = 0; i < region->nb; ++i)
		{
			ibuf_ptr = ( _mpc_lowcomm_ib_ibuf_t * )region->ibuf + i;

			if(ibuf_ptr->flag != FREE_FLAG)
			{
				mpc_common_debug_error("Buffer with wrong flag: %d in SEND channel (busy_nb:%d)", ibuf_ptr->flag,
				                       OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]) );
			}
		}
	}
	else
	{
		not_reachable();
	}
#endif


	/* Unregister the memory.
	 * FIXME: ideally, we should use ibv_reregister() but this call is currently not
	 * supported by the libverb. */
	_mpc_lowcomm_ib_mmu_entry_release(region->mmu_entry);

	/* Realloc a new memory region.
	 * FIXME: Using realloc is a little bit useless here because we get an additional
	 * memory copy not used in this case */
	ib_assume(region->buffer_addr);
	ptr = realloc(region->buffer_addr, nb_ibufs * size_ibufs);
	ib_assume(ptr);
	/* FIXME: is the memset here really usefull? */
	/* memset(ptr, 0, nb_ibufs * size_ibufs); */

	ib_assume(region->ibuf);
	ibuf = realloc(region->ibuf, nb_ibufs * sizeof(_mpc_lowcomm_ib_ibuf_t) );
	ib_assume(ibuf);
	/* FIXME: is the memset here really usefull? */
	/* memset (ibuf, 0, nb_ibufs * sizeof(_mpc_lowcomm_ib_ibuf_t)); */

	ib_assume(nb_ibufs > 0);
	/* save previous values */
	region->size_ibufs_previous = region->size_ibufs;
	region->nb_previous         = region->nb;
	/* Store new values */
	region->size_ibufs  = size_ibufs;
	region->buffer_addr = ptr;
	region->nb          = nb_ibufs;
	ib_assume( (region->size_ibufs != region->size_ibufs_previous) ||
	           region->nb != region->nb_previous);
	region->rail    = rail_ib;
	region->list    = NULL;
	region->channel = channel;
	region->ibuf    = ibuf;
	region->remote  = remote;
	mpc_common_spinlock_init(&region->lock, 0);
	region->polled_nb      = 0;
	region->allocated_size = (nb_ibufs * (size_ibufs + sizeof(_mpc_lowcomm_ib_ibuf_t) ) );

	mpc_common_nodebug("Channel: %d", region->channel);

	/* register buffers at once
	 * FIXME: ideally, we should use ibv_reregister() but this call is currently not
	 * supported by the libverb. */
	region->mmu_entry = _mpc_lowcomm_ib_mmu_entry_new(rail_ib, ptr,
	                                          nb_ibufs * size_ibufs);
	mpc_common_nodebug("Size_ibufs: %lu", size_ibufs);
	mpc_common_nodebug("[%d] Reg %p registered for rank %d (channel:%d). lkey : %lu", rail_ib->rail->rail_number, ptr, remote->rank, channel, region->mmu_entry->mr->lkey);

	/* Init all RDMA slots */
	__init_rdma_slots(region, ptr, ibuf, nb_ibufs, size_ibufs);

	/* Setup tail and head pointers */
	region->head = ibuf;
	region->tail = ibuf;

	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
	/* SEND CHANNEL */
	{
		/* Send the number of credits */
		remote->rdma.pool->send_credit = nb_ibufs;
		OPA_store_int(&remote->rdma.pool->busy_nb[REGION_SEND], 0);

		_mpc_lowcomm_ib_nodebug("RESIZING SEND channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d (send_credit:%d)",
		                remote->rank, nb_ibufs, size_ibufs, remote->rdma.pool->send_credit);
	}
	else
	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
	/* RECV CHANNEL */
	{
		_mpc_lowcomm_ib_nodebug("RESIZING RECV channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d)",
		                remote->rank, nb_ibufs, size_ibufs);

		/* Add the entry to the pooling list */
		OPA_store_int(&remote->rdma.pool->busy_nb[REGION_RECV], 0);
	}
	else
	{
		not_reachable();
	}

	mpc_common_nodebug("Head=%p(%d) Tail=%p(%d)", region->head, region->head->index,
	                   region->tail, region->tail->index);
}

void __rdma_region_free(struct _mpc_lowcomm_ib_rail_info_s *rail_ib, _mpc_lowcomm_ib_qp_t *remote,
                        _mpc_lowcomm_ib_ibuf_region_t *region, enum _mpc_lowcomm_ib_ibuf_channel_t channel)
{
	LOAD_DEVICE(rail_ib);


	{
		/* Free the buffers */
		ib_assume(region->buffer_addr);
		ib_assume(region->ibuf);

		free(region->buffer_addr);
		free(region->ibuf);

		region->buffer_addr    = NULL;
		region->ibuf           = NULL;
		region->allocated_size = 0;
	}

	mpc_common_spinlock_lock(&rdma_lock);
	--device->eager_rdma_connections;
	mpc_common_spinlock_unlock(&rdma_lock);

	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
	/* SEND CHANNEL */
	{
		/* Reinit counters */
		remote->rdma.messages_nb   = 0;
		remote->rdma.messages_size = 0;
		mpc_common_spinlock_init(&remote->rdma.stats_lock, 0);
		remote->rdma.max_pending_data  = 0;
		remote->rdma.pool->send_credit = 0;
		OPA_store_int(&remote->rdma.miss_nb, 0);
		OPA_store_int(&remote->rdma.hits_nb, 0);
		OPA_store_int(&remote->rdma.resizing_nb, 0);
	}
	else
	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
	/* RECV CHANNEL */
	{
		_mpc_lowcomm_ib_ibuf_rdma_pool_t *pool = remote->rdma.pool;
//#warning "We should lock during DELETING the element."
		mpc_common_spinlock_write_lock(&rdma_polling_lock);
		DL_DELETE(rdma_pool_list, pool);
		mpc_common_spinlock_write_unlock(&rdma_polling_lock);

		mpc_common_spinlock_write_lock(&rdma_region_list_lock);

		/* Shift clock pointer */
		if(clock_pointer == region)
		{
			clock_pointer = region->next;
		}

		CDL_DELETE(rdma_region_list, region);
		mpc_common_spinlock_write_unlock(&rdma_region_list_lock);
	}
	else
	{
		not_reachable();
	}
}

/*
 * Reinitialize a RDMA region with a different number of ibufs and a different size of ibufs
 */
void _mpc_lowcomm_ib_ibuf_rdma_region_reinit(struct _mpc_lowcomm_ib_rail_info_s *rail_ib, _mpc_lowcomm_ib_qp_t *remote,
                                             _mpc_lowcomm_ib_ibuf_region_t *region, enum _mpc_lowcomm_ib_ibuf_channel_t channel, int nb_ibufs, int size_ibufs)
{
	/* If we need to free the region */
	if(nb_ibufs == 0)
	{
		__rdma_region_free(rail_ib, remote, region, channel);
	}
	else
	/* If we need to resize the region */
	{
//    PROF_TIME_START(rail_ib->rail, ib_resize_rdma);
		__rdma_region_resize(rail_ib, remote, region, channel, nb_ibufs, size_ibufs);
//    PROF_TIME_END(rail_ib->rail, ib_resize_rdma);
	}
}

/*
 * Initialize a new RDMA buffer region
 */
void _mpc_lowcomm_ib_ibuf_rdma_region_init(struct _mpc_lowcomm_ib_rail_info_s *rail_ib, _mpc_lowcomm_ib_qp_t *remote,
                                           _mpc_lowcomm_ib_ibuf_region_t *region, enum _mpc_lowcomm_ib_ibuf_channel_t channel, int nb_ibufs, int size_ibufs)
{
	void *ptr = NULL;
	void *ibuf;

	ib_assume(nb_ibufs > 0);
	ib_assume(remote->rdma.pool);

	/* XXX: replace by memalign_on_node */
	sctk_posix_memalign( ( void ** )&ptr, getpagesize(), nb_ibufs * size_ibufs);
	ib_assume(ptr);
	memset(ptr, 0, nb_ibufs * size_ibufs);

	/* XXX: replace by memalign_on_node */
	sctk_posix_memalign(&ibuf, getpagesize(), nb_ibufs * sizeof(_mpc_lowcomm_ib_ibuf_t) );
	ib_assume(ibuf);
	memset(ibuf, 0, nb_ibufs * sizeof(_mpc_lowcomm_ib_ibuf_t) );

	region->size_ibufs_previous = 0;
	region->nb_previous         = 0;
	region->size_ibufs          = size_ibufs;
	region->nb = nb_ibufs;
	ib_assume( (region->size_ibufs != region->size_ibufs_previous) ||
	           region->nb != region->nb_previous);
	region->buffer_addr = ptr;
	region->rail        = rail_ib;
	region->list        = NULL;
	region->channel     = channel;
	region->ibuf        = ibuf;
	region->remote      = remote;
	mpc_common_spinlock_init(&region->lock, 0);
	region->polled_nb      = 0;
	region->allocated_size = (nb_ibufs * (size_ibufs + sizeof(_mpc_lowcomm_ib_ibuf_t) ) );

	mpc_common_nodebug("Channel: %d", region->channel);

	/* register buffers at once */
	region->mmu_entry = _mpc_lowcomm_ib_mmu_entry_new(rail_ib, ptr,
	                                          nb_ibufs * size_ibufs);
	mpc_common_nodebug("Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey);
	mpc_common_nodebug("Size_ibufs: %lu", size_ibufs);

	/* Init all RDMA slots */
	__init_rdma_slots(region, ptr, ibuf, nb_ibufs, size_ibufs);

	/* Setup tail and head pointers */
	region->head = ibuf;
	region->tail = ibuf;

	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
	/* SEND CHANNEL */
	{
		/* Send the number of credits */
		remote->rdma.pool->send_credit = nb_ibufs;
		OPA_store_int(&remote->rdma.pool->busy_nb[REGION_SEND], 0);

		_mpc_lowcomm_ib_nodebug("SEND channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d (send_credit:%d)",
		                remote->rank, nb_ibufs, size_ibufs, remote->rdma.pool->send_credit);
	}
	else
	if(channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
	/* RECV CHANNEL */
	{
		_mpc_lowcomm_ib_nodebug("RECV channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d",
		                remote->rank, nb_ibufs, size_ibufs);

		/* Add the entry to the pooling list */
		mpc_common_spinlock_lock(&rdma_pool_list_lock);
		DL_APPEND(rdma_pool_list_to_merge, remote->rdma.pool);
		mpc_common_spinlock_unlock(&rdma_pool_list_lock);
		OPA_store_int(&remote->rdma.pool->busy_nb[REGION_RECV], 0);
	}
	else
	{
		not_reachable();
	}

	mpc_common_spinlock_write_lock(&rdma_region_list_lock);

	/* Set clock-pointer */
	if(clock_pointer == NULL)
	{
		clock_pointer = region;
	}

	CDL_PREPEND(rdma_region_list, region);
	mpc_common_spinlock_write_unlock(&rdma_region_list_lock);

	mpc_common_nodebug("Head=%p(%d) Tail=%p(%d)", region->head, region->head->index,
	                   region->tail, region->tail->index);
}

/*
 * Create a region of ibufs for RDMA
 */
_mpc_lowcomm_ib_ibuf_rdma_pool_t *_mpc_lowcomm_ib_ibuf_rdma_pool_init(_mpc_lowcomm_ib_qp_t *remote)
{
	_mpc_lowcomm_ib_ibuf_rdma_pool_t *pool;

	/* We lock during the memmory allocating */
	mpc_common_spinlock_lock(&remote->rdma.lock);
	pool = remote->rdma.pool;

	/* If no allocation has been done */
	if(remote->rdma.pool == NULL)
	{
		pool = sctk_malloc(sizeof(_mpc_lowcomm_ib_ibuf_rdma_pool_t) );
		ib_assume(pool);
		memset(pool, 0, sizeof(_mpc_lowcomm_ib_ibuf_rdma_pool_t) );

		/* Setup local addr */
		pool->remote_region[REGION_SEND] = NULL;
		pool->remote_region[REGION_RECV] = NULL;
		pool->remote = remote;

		/* Finaly update the pool pointer */
		remote->rdma.pool = pool;
	}

	mpc_common_spinlock_unlock(&remote->rdma.lock);

	return pool;
}

/*
 * Delete a RDMA poll from a remote
 */
void _mpc_lowcomm_ib_ibuf_rdma_pool_free(__UNUSED__ struct _mpc_lowcomm_ib_rail_info_s *rail_ib, __UNUSED__ _mpc_lowcomm_ib_qp_t *remote)
{
/* Needs to be re-written */
#if 0
	LOAD_DEVICE(rail_ib);
	/* Check if we can allocate an RDMA channel */
	mpc_common_spinlock_lock(&rdma_lock);

	if(_mpc_lowcomm_ib_ibuf_rdma_is_remote_connected(remote) )
	{
		_mpc_lowcomm_ib_ibuf_rdma_pool_t *pool = remote->rdma.pool;
		--device->eager_rdma_connections;
		mpc_common_spinlock_unlock(&rdma_lock);

		_mpc_lowcomm_ib_debug("Freeing RDMA buffers (connections: %d)", device->eager_rdma_connections);

		/* Free regions for send and recv */
		__rdma_region_free(rail_ib, &pool->region[REGION_SEND]);
		__rdma_region_free(rail_ib, &pool->region[REGION_RECV]);

		mpc_common_spinlock_lock(&rdma_pool_list_lock);
		DL_DELETE(rdma_pool_list_to_merge, pool);
		mpc_common_spinlock_unlock(&rdma_pool_list_lock);

		/* Free pool */
		free(remote->rdma.pool);
		remote->rdma.pool = NULL;

		_mpc_lowcomm_ib_debug("[rank:%d] Free RDMA buffers", remote->rank);
	}
	else
	{
		mpc_common_spinlock_unlock(&rdma_lock);
	}
#endif
}

/*
 * Pick a RDMA buffer from the RDMA channel
 */
inline _mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_ibuf_rdma_pick(_mpc_lowcomm_ib_qp_t *remote)
{
	_mpc_lowcomm_ib_ibuf_t *tail;
	int          piggyback;

	IBUF_RDMA_LOCK_REGION(remote, REGION_SEND);

	tail      = IBUF_RDMA_GET_TAIL(remote, REGION_SEND);
	piggyback = IBUF_GET_EAGER_PIGGYBACK(tail->buffer);

	mpc_common_nodebug("Piggy back field %d %p", piggyback, &IBUF_GET_EAGER_PIGGYBACK(tail->buffer) );

	if(piggyback > 0)
	{
		int i;
		/* Reinit the piggyback of each buffer */
		_mpc_lowcomm_ib_ibuf_t *ptr = tail;

		/* For each slot freed, we reset the piggyback filed */
		for(i = 0; i < piggyback; ++i)
		{
			IBUF_GET_EAGER_PIGGYBACK(ptr->buffer) = -1;
			ptr = IBUF_RDMA_GET_NEXT(ptr);
		}

		/* Upgrade the send credit and move the tail flag */
		remote->rdma.pool->send_credit         += piggyback;
		IBUF_RDMA_GET_TAIL(remote, REGION_SEND) = IBUF_RDMA_ADD(tail, piggyback);

		mpc_common_nodebug("Got pg %d (prev:%p next:%p)", piggyback, tail, IBUF_RDMA_GET_TAIL(remote, REGION_SEND) );
	}

	mpc_common_nodebug("Send credit: %d", remote->rdma.pool->send_credit);

	/* If a buffer is available */
	if(remote->rdma.pool->send_credit > 0)
	{
		_mpc_lowcomm_ib_ibuf_t *head;

		head = IBUF_RDMA_GET_HEAD(remote, REGION_SEND);

		/* If the buffer is not free for the moment, we skip the
		 * RDMA */
//    if (head->flag != FREE_FLAG) {
//      mpc_common_debug_error("Buffer %p not free", head);
//      IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);
//      return NULL;
//    }

		/* Update the credit */
		remote->rdma.pool->send_credit--;
		/* Move the head flag */
		IBUF_RDMA_GET_HEAD(remote, REGION_SEND) = IBUF_RDMA_GET_NEXT(head);
		IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);

		mpc_common_nodebug("Picked RDMA buffer %p, buffer=%p pb=%p index=%d credit=%d size:%d", head,
		                   head->buffer, &IBUF_GET_EAGER_PIGGYBACK(head->buffer), head->index, remote->rdma.pool->send_credit,
		                   remote->rdma.pool->region[REGION_SEND].size_ibufs);
#ifdef IB_DEBUG
		if(head->flag != FREE_FLAG)
		{
			mpc_common_debug_error("Wrong flag (%d) got from ibuf", head->flag);
			not_implemented();
		}
#endif
		head->flag = BUSY_FLAG;

		remote->rdma.pool->region[REGION_SEND].R_bit = 1;
		return ( _mpc_lowcomm_ib_ibuf_t * )head;
	}
	else
	{
		mpc_common_nodebug("Cannot pick RDMA buffer: %d", remote->rdma.pool->send_credit);
		IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);
	}

	return NULL;
}

/*
 * Reset the head flag after a message has been successfully received
 */
static inline void __reset_head_flag(_mpc_lowcomm_ib_ibuf_t *ibuf)
{
	*(ibuf->head_flag) = IBUF_RDMA_RESET_FLAG;
}

/*
 * Set the tail flag to the ibuf
 */
void _mpc_lowcomm_ib_ibuf_rdma_set_tail_flag(_mpc_lowcomm_ib_ibuf_t *ibuf, size_t size)
{
	/* Head, payload and Tail */
	int *ibuf_tail;

	ibuf_tail = IBUF_RDMA_GET_TAIL_FLAG(ibuf->buffer, size);

	*(ibuf->size_flag) = size;

	if(*ibuf_tail == IBUF_RDMA_FLAG_1)
	{
		*ibuf_tail         = IBUF_RDMA_FLAG_2;
		*(ibuf->head_flag) = IBUF_RDMA_FLAG_2;
	}
	else
	{
		*ibuf_tail         = IBUF_RDMA_FLAG_1;
		*(ibuf->head_flag) = IBUF_RDMA_FLAG_1;
	}
}

/*
 * Handle a message received
 */
static inline void __poll_ibuf(_mpc_lowcomm_ib_rail_info_t *rail_ib,
                               _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	sctk_rail_info_t *rail = rail_ib->rail;

	ib_assume(ibuf);
	int release_ibuf = 1;


	/* Set the buffer as releasable. Actually, we need to do this reset
	 * here.. */
	ibuf->to_release = IBUF_RELEASE;

	const _mpc_lowcomm_ib_protocol_t protocol = IBUF_GET_PROTOCOL(ibuf->buffer);

	switch(protocol)
	{
		case MPC_LOWCOMM_IB_EAGER_PROTOCOL:
			_mpc_lowcomm_ib_eager_poll_recv(rail, ibuf);
			release_ibuf = 0;
			break;

		case MPC_LOWCOMM_IB_RDMA_PROTOCOL:
			mpc_common_nodebug("RDMA received from RDMA channel");
			release_ibuf = _mpc_lowcomm_ib_rdma_poll_recv(rail, ibuf);
			break;

		case MPC_LOWCOMM_IB_BUFFERED_PROTOCOL:
			_mpc_lowcomm_ib_buffered_poll_recv(rail, ibuf);
			release_ibuf = 1;
			break;

		default:
			not_reachable();
	}

	if(release_ibuf)
	{
		mpc_common_nodebug("Release RDMA buffer %p");
		_mpc_lowcomm_ib_ibuf_release(rail_ib, ibuf, 1);
	}

	return;
}

/*
 * Walk on each remotes
 */
void _mpc_lowcomm_ib_ibuf_rdma_eager_walk_remote(_mpc_lowcomm_ib_rail_info_t *rail, int(func) (_mpc_lowcomm_ib_rail_info_t * rail, _mpc_lowcomm_ib_qp_t * remote), int *ret)
{
	_mpc_lowcomm_ib_ibuf_rdma_pool_t *pool, *tmp_pool;
	int tmp_ret;

	/* Set the default value */
	*ret = _MPC_LOWCOMM_REORDER_UNDEFINED;

	mpc_common_spinlock_read_lock(&rdma_polling_lock);
	DL_FOREACH(rdma_pool_list, pool)
	{
		/* 'func' needs to check if the remote is in a RTR mode */
		tmp_ret = func(rail, pool->remote);

		if(tmp_ret == _MPC_LOWCOMM_REORDER_FOUND_EXPECTED)
		{
			*ret = tmp_ret;
			mpc_common_spinlock_read_unlock(&rdma_polling_lock);
			return;
		}
	}
	mpc_common_spinlock_read_unlock(&rdma_polling_lock);

	/* Check if they are remotes to merge. We need to merge here because
	 * we cannot do this in the DL_FOREACH just before. It aims
	 * to deadlocks. */
	if(rdma_pool_list_to_merge != NULL)
	{
		mpc_common_spinlock_write_lock(&rdma_polling_lock);
		/* We only add entries of connected processes */
		mpc_common_spinlock_lock(&rdma_pool_list_lock);
		DL_FOREACH_SAFE(rdma_pool_list_to_merge, pool, tmp_pool)
		{
			_mpc_lowcomm_endpoint_state_t state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(pool->remote);

			if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
			{
				/* Remove the entry from the merge list... */
				DL_DELETE(rdma_pool_list_to_merge, pool);
				/* ... add it to the global list */
				DL_APPEND(rdma_pool_list, pool);
			}
		}
		mpc_common_spinlock_unlock(&rdma_pool_list_lock);
		mpc_common_spinlock_write_unlock(&rdma_polling_lock);
	}
}

/*
 * Poll a specific remote in order to handle new received messages.
 * Returns :
 *  1 if a message has been found
 *  -1 if the lock had not been taken
 *  0 if no message has been found
 */
static inline int __eager_poll_remote(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_qp_t *remote)
{
	/* We return if the remote is not connected to the RDMA channel */
	_mpc_lowcomm_endpoint_state_t state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(remote);

	if( (state != _MPC_LOWCOMM_ENDPOINT_CONNECTED) && (state != _MPC_LOWCOMM_ENDPOINT_REQUESTING) )
	{
		return _MPC_LOWCOMM_REORDER_UNDEFINED;
	}

	_mpc_lowcomm_ib_ibuf_t *head;

	/* Spinlock for preventing two concurrent calls to this function */
retry:

	if(IBUF_RDMA_TRYLOCK_REGION(remote, REGION_RECV) == 0)
	{
		/* Read the state of the connection and return if necessary.
		 * If we do not this, the RDMA buffer may be read while the RDMA buffer
		 * is in a 'flushed' state */
		state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(remote);

		if( (state != _MPC_LOWCOMM_ENDPOINT_CONNECTED) && (state != _MPC_LOWCOMM_ENDPOINT_REQUESTING) )
		{
			IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
			return _MPC_LOWCOMM_REORDER_UNDEFINED;
		}

		/* Recaculate the head because it could be moved */
		head = IBUF_RDMA_GET_HEAD(remote, REGION_RECV);

		/* Double checking */
		if(*(head->head_flag) != IBUF_RDMA_RESET_FLAG)
		{
			mpc_common_nodebug("Head set: %d-%d", *(head->head_flag), IBUF_RDMA_RESET_FLAG);

			const size_t           size_flag = *head->size_flag;
			volatile int *tail_flag = IBUF_RDMA_GET_TAIL_FLAG(head->buffer, size_flag);

			mpc_common_nodebug("Head set: %d-%d", *(head->head_flag), IBUF_RDMA_RESET_FLAG);

			/* The head has been set. We spin while waiting the tail to be set.
			 * We do not release the lock because it is not necessary that a task
			 * concurrently polls the same remote.  */
			int i = 0;

			while( (*(head->head_flag) != *(tail_flag) ) && (i++ < 10) )
			{
				/* poll */
			}
			;

			if(*(head->head_flag) != *(tail_flag) )
			{
				IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
				return _MPC_LOWCOMM_REORDER_NOT_FOUND;
			}

			/* Reset the head flag */
			__reset_head_flag(head);
			/* Move head flag */
			IBUF_RDMA_GET_HEAD(remote, REGION_RECV) = IBUF_RDMA_GET_NEXT(head);
			/* Increase the number of buffers busy */
			OPA_incr_int(&remote->rdma.pool->busy_nb[REGION_RECV]);
			IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
			IBUF_RDMA_CHECK_POISON_FLAG(head);

			mpc_common_nodebug("Buffer size:%d, ibuf:%p, head flag:%d, tail flag:%d %p new_head:%p (%d-%d)", *head->size_flag, head, *head->head_flag, *tail_flag, head->buffer, IBUF_RDMA_GET_HEAD(remote, REGION_RECV)->buffer, *(head->head_flag), *(tail_flag) );

			if(head->flag != FREE_FLAG)
			{
				mpc_common_debug_error("Got a wrong flag, it seems there is a problem with MPC: %d %p", head->flag,
				                       head);
				not_reachable();
			}

			/* Set the slot as busy */
			head->flag = BUSY_FLAG;
			mpc_common_nodebug("Set %p as busy", head);

			/* Handle the ibuf */
			__poll_ibuf(rail_ib, head);
			goto retry;
		}
		else
		{
			IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
			return _MPC_LOWCOMM_REORDER_NOT_FOUND;
		}
	}
	else
	{
		return _MPC_LOWCOMM_REORDER_UNDEFINED;
	}
}

/*
 * Poll a specific remote in order to handle new received messages.
 * Returns :
 *  1 if a message has been found
 *  -1 if the lock had not been taken
 *  0 if no message has been found
 */
int _mpc_lowcomm_ib_ibuf_rdma_eager_poll_remote(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_qp_t *remote)
{
	/* We return if the remote is not connected to the RDMA channel */
	_mpc_lowcomm_endpoint_state_t state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(remote);

	if( (state != _MPC_LOWCOMM_ENDPOINT_CONNECTED) && (state != _MPC_LOWCOMM_ENDPOINT_REQUESTING) )
	{
		return _MPC_LOWCOMM_REORDER_UNDEFINED;
	}

	_mpc_lowcomm_ib_ibuf_t *head;

	head = IBUF_RDMA_GET_HEAD(remote, REGION_RECV);

	if(*(head->head_flag) != IBUF_RDMA_RESET_FLAG)
	{
		if(__eager_poll_remote(rail_ib, remote) == _MPC_LOWCOMM_REORDER_FOUND_EXPECTED)
		{
			return _MPC_LOWCOMM_REORDER_FOUND_EXPECTED;
		}
	}

	return _MPC_LOWCOMM_REORDER_UNDEFINED;
}

/* / ! \ This function requires the region to be previously locked !! */
static inline void __check_piggyback(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_ibuf_region_t *region)
{
	_mpc_lowcomm_ib_qp_t *remote = region->remote;
	int           ret;
	int           piggyback = 0;
	_mpc_lowcomm_ib_ibuf_t * ibuf;

	/* Get the tail */
	_mpc_lowcomm_ib_ibuf_t *tail = IBUF_RDMA_GET_TAIL(remote, REGION_RECV);

	/* Buffer which will be piggybacked */
	ibuf = tail;

	while(ibuf->flag == EAGER_RDMA_POLLED)
	{
		piggyback++;
		/* Put the buffer as free */
		ibuf->flag = FREE_FLAG;
		mpc_common_nodebug("Set %p as free", ibuf);
		/* Move to next buffer */
		ibuf = IBUF_RDMA_GET_NEXT(ibuf);
	}

	/* Piggyback the ibuf to the tail addr */
	if(piggyback > 0)
	{
		ib_assume(tail->flag == FREE_FLAG);
		ib_assume(tail->region);
		ib_assume(tail->region->ibuf);
		ib_assume(region->nb > 0);

		/* Move tail */
		IBUF_RDMA_GET_TAIL(remote, REGION_RECV) = IBUF_RDMA_ADD(tail, piggyback);
		/* Unlock the RDMA region */
		IBUF_GET_EAGER_PIGGYBACK(tail->buffer) = piggyback;
		region->polled_nb = 0;
		IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);


		mpc_common_nodebug("Piggy back to %p pb:%d size:%d", IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote, tail), piggyback, remote->rdma.pool->region[REGION_RECV].size_ibufs);

		/* Prepare the piggyback msg. The sent message is SIGNALED */
		mpc_common_nodebug("ACK %d messages", piggyback);
		ret = _mpc_lowcomm_ib_ibuf_write_with_imm_init(tail,
		                                               &IBUF_GET_EAGER_PIGGYBACK(tail->buffer),      /* Local addr */
		                                               region->mmu_entry->mr->lkey,                  /* lkey */
		                                               IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote, tail), /* Remote addr */
		                                               remote->rdma.pool->rkey[REGION_SEND],         /* rkey */
		                                               sizeof(int), IBUF_DO_NOT_RELEASE, IMM_DATA_RDMA_PIGGYBACK + piggyback);
		assume(ret == 1);

		/* Put the buffer as free */
		tail->flag = FREE_FLAG;

		/* Send the piggyback. This event will generate an event to
		 * the CQ */
		ret = _mpc_lowcomm_ib_qp_send_ibuf(rail_ib, remote, tail);
		assume(ret == 1);
	}
	else
	{
		/* Unlock the RDMA region */
		IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);

		/* Increase the number of buffers busy */
		OPA_decr_int(&remote->rdma.pool->busy_nb[REGION_RECV]);
		_mpc_lowcomm_ib_ibuf_rdma_check_flush_recv(rail_ib, remote);
	}
}

//#define PIGGYBACK_THRESHOLD (50.0 / 100.0)
#define PIGGYBACK_THRESHOLD    (0)

/*
 * Release a buffer from the RDMA channel
 */
void _mpc_lowcomm_ib_ibuf_rdma_release(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	_mpc_lowcomm_ib_ibuf_region_t *region = ibuf->region;
	_mpc_lowcomm_ib_qp_t *      remote = ibuf->region->remote;


	if(IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_RECV_CHANNEL)
	{
		mpc_common_nodebug("Freeing a RECV RDMA buffer (channel:%d head:%p - ibuf:%p - tail:%p)", IBUF_GET_CHANNEL(ibuf),
		                   ibuf->region->head, ibuf, ibuf->region->tail);

		/* Firstly lock the region */
		IBUF_RDMA_LOCK_REGION(remote, REGION_RECV);


		/* Mark the buffer as polled */
		if(ibuf->flag != BUSY_FLAG)
		{
			mpc_common_debug_error("Got a wrong flag, it seems there is a problem with MPC: %d", ibuf->flag);
			not_reachable();
		}

		ibuf->flag    = EAGER_RDMA_POLLED;
		region->R_bit = 1;
		mpc_common_nodebug("Set %p as RDMA_POLLED", ibuf);


		/* While a buffer free is found, we increase the piggy back and
		 * we reset each buffer */
		region->polled_nb++;

		if(region->polled_nb > region->nb * PIGGYBACK_THRESHOLD /* 0 */)
		{
			__check_piggyback(rail_ib, region);
		}
		else
		{
			/* Unlock the RDMA region */
			IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);

			/* Increase the number of buffers busy */
			OPA_decr_int(&remote->rdma.pool->busy_nb[REGION_RECV]);
			_mpc_lowcomm_ib_ibuf_rdma_check_flush_recv(rail_ib, remote);
		}
	}
	else
	if(IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_SEND_CHANNEL)
	{
		/* We set the buffer as free */
//    ibuf->flag = FREE_FLAG;
//    OPA_decr_int(&remote->rdma.pool->busy_nb[REGION_SEND]);

		/* Check if we are in a flush state */
//    _mpc_lowcomm_ib_ibuf_rdma_flush_send(rail_ib, remote);
	}
	else
	{
		not_reachable();
	}
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
#define IBV_RDMA_MAX_MISS     500
//#define IBV_RDMA_MAX_MISS 1
/* It is better if it is higher than IBV_RDMA_SAMPLES */
#define IBV_RDMA_THRESHOLD    100
//#define IBV_RDMA_THRESHOLD 1


/*
 * Check if a remote can be connected using RDMA.
 * The number of connections is automatically increased
 */
int _mpc_lowcomm_ib_ibuf_rdma_is_connectable(_mpc_lowcomm_ib_rail_info_t *rail_ib)
{
	LOAD_CONFIG(rail_ib);
	LOAD_DEVICE(rail_ib);
	int ret = 0;

	/* Check if the memory used is correct */
#if 0
	if( (_mpc_lowcomm_ib_prof_get_mem_used() + entry_nb * entry_size) > IBV_MEM_USED_LIMIT)
	{
		mpc_common_nodebug("Cannot connect with RDMA because max memory used reached");
		return 0;
	}
#endif

	/* Check if we do not have reached the maximum number of RDMA connections */
	mpc_common_spinlock_lock(&rdma_lock);

	if(config->max_rdma_connections >= (device->eager_rdma_connections + 1) )
	{
		ret = ++device->eager_rdma_connections;
	}

	mpc_common_spinlock_unlock(&rdma_lock);

	return ret;
}

void _mpc_lowcomm_ib_ibuf_rdma_connexion_cancel(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_qp_t *remote)
{
	LOAD_DEVICE(rail_ib);

	mpc_common_spinlock_lock(&rdma_lock);
	--device->eager_rdma_connections;
	mpc_common_spinlock_unlock(&rdma_lock);

	/* We reset the state to 'reset' because we do not support the RDMA deconnection for now.
	 * In a next release, we need to change to deconnected */
	remote->rdma.messages_nb   = 0;
	remote->rdma.messages_size = 0;
	mpc_common_spinlock_init(&remote->rdma.stats_lock, 0);
	OPA_incr_int(&remote->rdma.cancel_nb);
	_mpc_lowcomm_ib_ibuf_rdma_set_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_DECONNECTED);

	_mpc_lowcomm_ib_debug("[%d] OD QP RDMA connexion canceled to %d (rdma_connections:%d rdma_cancel:%d)",
	              rail_ib->rail->rail_number, remote->rank, device->eager_rdma_connections, remote->rdma.cancel_nb);
}

/*
 * Return the size allocated by the RDMA channel for a
 * remote and for a specific region (send or recv)
 */
static inline size_t __get_region_size(_mpc_lowcomm_ib_qp_t *remote, int reg)
{
	_mpc_lowcomm_ib_ibuf_region_t *region = NULL;

	if(reg == REGION_RECV)
	{
		_mpc_lowcomm_endpoint_state_t state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(remote);

		if(state != _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
		{
			region = &remote->rdma.pool->region[REGION_RECV];
		}
	}
	else
	if(reg == REGION_SEND)
	{
		_mpc_lowcomm_endpoint_state_t state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote);

		if(state != _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
		{
			region = &remote->rdma.pool->region[REGION_SEND];
		}
	}
	else
	{
		not_reachable();
	}

	/* If there is a region to analyze */
	if(region)
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
size_t _mpc_lowcomm_ib_ibuf_rdma_region_size_get(_mpc_lowcomm_ib_qp_t *remote)
{
	size_t sum = 0;

	sum += __get_region_size(remote, REGION_SEND);
	sum += __get_region_size(remote, REGION_RECV);
	return sum;
}

/*
 * Update the maximum number of pending requests.
 * Should be called just before calling incr_requests_nb
 */
void _mpc_lowcomm_ib_ibuf_rdma_max_pending_data_update(_mpc_lowcomm_ib_qp_t *remote, int current_pending)
{
	mpc_common_spinlock_lock(&remote->rdma.pending_data_lock);

	if(current_pending > remote->rdma.max_pending_data)
	{
		remote->rdma.max_pending_data = current_pending;
	}

	mpc_common_spinlock_unlock(&remote->rdma.pending_data_lock);
}

static inline int __determine_config(_mpc_lowcomm_ib_rail_info_t *rail_ib,
                                           _mpc_lowcomm_ib_qp_t *remote, unsigned int *determined_size, int *determined_nb, char resizing)
{
	LOAD_CONFIG(rail_ib);
	/* Compute the mean size of messages */

	float mean = 0;
	int   max_pending_data, max_pending_data_before_realign;

	max_pending_data = remote->rdma.max_pending_data;

	if(resizing == 1)
	{
		if(max_pending_data <= remote->rdma.previous_max_pending_data)
		{
			return 0;
		}
	}

	mpc_common_spinlock_lock(&remote->rdma.stats_lock);
	mean = (remote->rdma.messages_size) / ( float )(remote->rdma.messages_nb);
	mpc_common_spinlock_unlock(&remote->rdma.stats_lock);

	/* Reajust the size according to the statistics */
	*determined_size = ( int )mean;

	/* We determine the size of the slots */
	if(resizing == 0)
	{
		if(*determined_size > config->rdma_max_size)
		{
			*determined_size = config->rdma_max_size;
		}
		else
		if(*determined_size < config->rdma_min_size)
		{
			*determined_size = config->rdma_min_size;
		}
	}
	else
	/* Buffer resizing */
	{
		if(*determined_size > config->rdma_resizing_max_size)
		{
			*determined_size = config->rdma_resizing_max_size;
		}
		else
		if(*determined_size < config->rdma_resizing_min_size)
		{
			*determined_size = config->rdma_resizing_min_size;
		}
	}

	/* Realign */
	max_pending_data_before_realign = max_pending_data;
	max_pending_data = ALIGN_ON(max_pending_data, *determined_size);
	*determined_nb   = (max_pending_data / ( float )(*determined_size) );
//  *determined_nb *= 1.1;

	/* Realign on 64 bits and adjust with the size of the headers */
	*determined_size = ALIGN_ON( (*determined_size + sizeof(mpc_lowcomm_ptp_message_body_t) +
	                              IBUF_GET_EAGER_SIZE + IBUF_RDMA_GET_SIZE), 1024);

	if(resizing == 0)
	{
		if(*determined_nb > config->rdma_max_nb)
		{
			*determined_nb = config->rdma_max_nb;
		}
		else
		if(*determined_nb < config->rdma_min_nb)
		{
			*determined_nb = config->rdma_min_nb;
		}
	}
	else
	/* Buffer resizing */
	{
		if(*determined_nb > config->rdma_resizing_max_nb)
		{
			*determined_nb = config->rdma_resizing_max_nb;
		}
		else
		if(*determined_nb < config->rdma_resizing_min_nb)
		{
			*determined_nb = config->rdma_resizing_min_nb;
		}
	}

	if(resizing == 1)
	{
		unsigned int previous_size = remote->rdma.pool->region[REGION_SEND].size_ibufs;
		int          previous_nb   = remote->rdma.pool->region[REGION_SEND].nb;

		if(previous_nb > *determined_nb)
		{
			*determined_nb = previous_nb;
		}

		if(previous_size > *determined_size)
		{
			*determined_size = previous_size;
		}

		mpc_common_nodebug("Max pending: %d->%d", remote->rdma.previous_max_pending_data, max_pending_data_before_realign);
		//mpc_common_debug("Mean message size to remote %d kB, pending:%d->%dkB, previous max pending:%dkB (size:%d->%d nb:%d->%d)", (int) mean / 1024, max_pending_data_before_realign / 1024, max_pending_data / 1024,
		// remote->rdma.previous_max_pending_data/1024, previous_size/1024, *determined_size/1024, previous_nb,  *determined_nb);

		if(previous_size >= *determined_size && previous_nb >= *determined_nb)
		{
			return 0;
		}
	}

	remote->rdma.previous_max_pending_data = max_pending_data_before_realign;
	return 1;
}

void _mpc_lowcomm_ib_ibuf_rdma_remote_update(_mpc_lowcomm_ib_qp_t *remote, size_t size)
{
	mpc_common_spinlock_lock(&remote->rdma.stats_lock);
	size_t old_messages_size = remote->rdma.messages_size;
	size_t new_messages_size = old_messages_size + size;
	remote->rdma.messages_size += size;
	remote->rdma.messages_nb++;

	/* Reset counters */
	if(expect_false(new_messages_size < old_messages_size) )
	{
		remote->rdma.messages_nb   = 1;
		remote->rdma.messages_size = size;
	}

	mpc_common_spinlock_unlock(&remote->rdma.stats_lock);
}

/*
 * Check if we need to change the RDMA state of a remote
 */
void _mpc_lowcomm_ib_ibuf_rdma_remote_check(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_qp_t *remote)
{
	LOAD_CONFIG(rail_ib);


	unsigned int determined_size;
	int          determined_nb;
	/* We first get the state of the route */
	const _mpc_lowcomm_endpoint_state_t state_rts = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote);

	/* We profile only when the RDMA route is deconnected */
	if(config->max_rdma_connections != 0 && state_rts == _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
	{
		size_t messages_nb;
		mpc_common_spinlock_lock(&remote->rdma.stats_lock);
		messages_nb = remote->rdma.messages_nb;
		mpc_common_spinlock_unlock(&remote->rdma.stats_lock);

		/* Check if we need to connect using RDMA */
		if(messages_nb >= IBV_RDMA_THRESHOLD)
		{
			/* Check if we can connect using RDMA. If == 1, only one thread can have an access to
			 * this code's part */
			if(_mpc_lowcomm_ib_cm_on_demand_rdma_check_request(rail_ib->rail, remote) == 1)
			{
				__determine_config(rail_ib, remote, &determined_size, &determined_nb, 0);

				mpc_common_nodebug("Trying to connect remote %d using RDMA ( messages_nb: %d, determined: %d, pending: %d, iter: %d)", remote->rank, messages_nb, determined_size,
				                   determined_nb);

				/* Sending the request. If the request has been sent, we reinit */
				_mpc_lowcomm_ib_cm_on_demand_rdma_request(rail_ib->rail, remote,
				                                  determined_size, determined_nb);
			}
		}
	}
	else
	if(config->rdma_resizing && state_rts == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
	{
		_mpc_lowcomm_endpoint_state_t ret;

		/* Check if we need to resize the RDMA */
		if(OPA_load_int(&remote->rdma.miss_nb) > IBV_RDMA_MAX_MISS)
		{
			mpc_common_nodebug("MAX MISS REACHED busy:%d", OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]) );

			/* Try to change the state to flushing.
			 * By changing the state of the remote to 'flushing', we automaticaly switch to SR */
			mpc_common_spinlock_lock(&remote->rdma.flushing_lock);
			ret = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTED, _MPC_LOWCOMM_ENDPOINT_FLUSHING);

			if(ret == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
			{
				/* Compute the next slots values */
				unsigned int next_size;
				int          next_nb;
				/* Reset the counter */
				OPA_store_int(&remote->rdma.miss_nb, 0);

				if(__determine_config(rail_ib, remote, &next_size, &next_nb, 1) == 1)
				{
					/* Update the slots values requested */
					remote->rdma.pool->resizing_request.send_keys.nb   = next_nb;
					remote->rdma.pool->resizing_request.send_keys.size = next_size;
					mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);

					_mpc_lowcomm_ib_ibuf_rdma_flush_send(rail_ib, remote);
				}
				else
				{
					/* We reset the connection to connected */
					ret = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_FLUSHING, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
					assume(ret == _MPC_LOWCOMM_ENDPOINT_FLUSHING);
					mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);
				}
			}
			else
			{
				mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);
			}
		}
	}
}

/*
 * Fill RDMA connection keys with the configuration of a send and receive region
 */
void
_mpc_lowcomm_ib_ibuf_rdma_fill_remote_addr(_mpc_lowcomm_ib_qp_t *remote,
                                _mpc_lowcomm_ib_cm_rdma_connection_t *keys, int region)
{
	ib_assume(remote->rdma.pool);
	keys->addr = remote->rdma.pool->region[region].buffer_addr;
	keys->rkey = remote->rdma.pool->region[region].mmu_entry->mr->rkey;

	mpc_common_nodebug("Filled keys addr=%p rkey=%u",
	                   keys->addr, keys->rkey, keys->connected);
}

/*
 * Update a remote from the keys received by a peer. This function must be
 * called before sending any message to the RDMA channel
 */
void _mpc_lowcomm_ib_ibuf_rdma_update_remote_addr(_mpc_lowcomm_ib_qp_t *remote, _mpc_lowcomm_ib_cm_rdma_connection_t *key, int region)
{
	mpc_common_nodebug("REMOTE addr updated:: addr=%p rkey=%u (connected: %d)",
	                   key->addr, key->rkey, key->connected);

	ib_assume(remote->rdma.pool);

	/* Update keys for send and recv regions. We need to register the send region
	 * because of the piggyback */
	remote->rdma.pool->remote_region[region] = key->addr;
	remote->rdma.pool->rkey[region]          = key->rkey;
}

/*
 * Return the RDMA eager ibufs limit for
 * sending a message
 */
size_t _mpc_lowcomm_ib_ibuf_rdma_eager_limit_get(_mpc_lowcomm_ib_qp_t *remote)
{
	return remote->rdma.pool->region[REGION_SEND].size_ibufs;
}

/*-----------------------------------------------------------
*                          FLUSH
*----------------------------------------------------------*/

/*
 * Check if a remote is in a flushing state. If it is, it
 * sends a flush REQ to the process
 */
int _mpc_lowcomm_ib_ibuf_rdma_flush_send(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_qp_t *remote)
{
	/* Check if the RDMA is in flushing mode and if all messages
	 * have be flushed to the network */
	_mpc_lowcomm_endpoint_state_t ret = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote);

	/* If we are in a flushing mode */
	if(ret == _MPC_LOWCOMM_ENDPOINT_FLUSHING)
	{
		int busy_nb = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]);

		if(busy_nb == 0)
		{
			mpc_common_spinlock_lock(&remote->rdma.flushing_lock);
			ret = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_FLUSHING, _MPC_LOWCOMM_ENDPOINT_FLUSHED);

			if(ret == _MPC_LOWCOMM_ENDPOINT_FLUSHING)
			{
				_mpc_lowcomm_ib_debug("SEND DONE Trying to flush RDMA for for remote %d", remote->rank);

				/* All the requests have been flushed. We can now send the resizing request.
				 * NOTE: we are sure that only one thread call the resizing request */
				_mpc_lowcomm_ib_nodebug("Sending a FLUSH message to remote %d", remote->rank);

				/* Send the request. We need to lock in order to be sure that we do not
				 * emit a resizing request before changing the size_slots and the size and
				 * the number of slots */
				int size_ibufs = remote->rdma.pool->resizing_request.send_keys.size;
				int nb         = remote->rdma.pool->resizing_request.send_keys.nb;
				mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);

				_mpc_lowcomm_ib_cm_resizing_rdma_request(rail_ib->rail, remote,
				                                 size_ibufs, nb);
				return 1;
			}
			else
			{
				mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);
			}
		}
		else
		{
			mpc_common_nodebug("Nb pending: %d", busy_nb);
		}
	}

	return 0;
}

/*
 * Check if a remote is in a flushing state. If it is, it
 * sends a flush REQ to the process
 */
int _mpc_lowcomm_ib_ibuf_rdma_check_flush_recv(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_qp_t *remote)
{
	/* Check if the RDMA is in flushing mode and if all messages
	 * have be flushed to the network */
	_mpc_lowcomm_endpoint_state_t ret = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(remote);

	/* If we are in a flushing mode */
	if(ret == _MPC_LOWCOMM_ENDPOINT_FLUSHING)
	{
		int busy_nb = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_RECV]);
		ib_assume(busy_nb >= 0);
		mpc_common_nodebug("Number pending: %d", busy_nb);

		if(busy_nb == 0)
		{
			/* We lock the region during the state modification to 'flushed'.
			 * If we do not this, the RDMA buffer may be read while the RDMA buffer
			 * is in a 'flushed' state */
			IBUF_RDMA_LOCK_REGION(remote, REGION_RECV);
			ret = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_FLUSHING, _MPC_LOWCOMM_ENDPOINT_FLUSHED);
			IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);

			if(ret == _MPC_LOWCOMM_ENDPOINT_FLUSHING)
			{
				_mpc_lowcomm_ib_nodebug("RECV DONE Trying to flush RDMA for for remote %d", remote->rank);

				/* All the requests have been flushed. We can now send the resizing request.
				 * NOTE: we are sure that only one thread call the resizing request */

				/* Resizing the RDMA buffer */
				_mpc_lowcomm_ib_ibuf_rdma_region_reinit(rail_ib, remote,
				                                        &remote->rdma.pool->region[REGION_RECV],
				                                        MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL,
				                                        remote->rdma.pool->resizing_request.recv_keys.nb,
				                                        remote->rdma.pool->resizing_request.recv_keys.size);

				/* Fill the keys */
				_mpc_lowcomm_ib_cm_rdma_connection_t *send_keys = &remote->rdma.pool->resizing_request.send_keys;
				_mpc_lowcomm_ib_ibuf_rdma_fill_remote_addr(remote, send_keys, REGION_RECV);
				send_keys->connected = 1;

				_mpc_lowcomm_ib_nodebug("Sending a FLUSH ACK message to remote %d", remote->rank);
				_mpc_lowcomm_ib_cm_resizing_rdma_ack(rail_ib->rail, remote, send_keys);
				return 1;
			}
		}
		else
		{
			mpc_common_nodebug("Nb pending: %d", busy_nb);
		}
	}

	return 0;
}

void _mpc_lowcomm_ib_ibuf_rdma_flush_recv(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_qp_t *remote)
{
	int ret = -1;

	_mpc_lowcomm_ib_nodebug("RECV Trying to flush RDMA for for remote %d", remote->rank);

	/*
	 * Flush the RDMA buffers.
	 */
	do
	{
		ret = __eager_poll_remote(rail_ib, remote);
	} while(ret != _MPC_LOWCOMM_REORDER_NOT_FOUND);

	/* We need to change the state AFTER flushing the buffers */
	_mpc_lowcomm_endpoint_state_t state;
	state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTED, _MPC_LOWCOMM_ENDPOINT_FLUSHING);

	/* If we are in a requesting state, we swap to _MPC_LOWCOMM_ENDPOINT_FLUSHING mode */
	if(state != _MPC_LOWCOMM_ENDPOINT_FLUSHING)
	{
		state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_REQUESTING, _MPC_LOWCOMM_ENDPOINT_FLUSHING);
	}

	_mpc_lowcomm_ib_ibuf_rdma_check_flush_recv(rail_ib, remote);
}

/*-----------------------------------------------------------
*  RDMA disconnection
*----------------------------------------------------------*/

/*
 * DANGER: no lock is taken during this call. The parent calling function
 * must take locks
 */
static inline void __get_size_from_all_remotes(
        size_t *allocated_size,
        int *regions_nb)
{
	_mpc_lowcomm_ib_ibuf_region_t *  region;
	_mpc_lowcomm_endpoint_state_t state;

	*allocated_size = 0;
	*regions_nb     = 0;

	if(rdma_region_list != NULL)
	{
		region = rdma_region_list;

		CDL_FOREACH(rdma_region_list, region)
		{
			/* Get the state associated to the region */
			if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
			{
				state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(region->remote);
			}
			else
			if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
			{
				state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(region->remote);
			}
			else
			{
				not_reachable();
			}

			/* If state connected */
			if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
			{
				*allocated_size += region->allocated_size;

				/* pas utilisé
				 * regions_nb++;
				 */
			}
		}
	}
}

/*
 *  Normalize RDMA buffers
 */
int _mpc_lowcomm_ib_ibuf_rdma_normalize(_mpc_lowcomm_ib_rail_info_t *rail_ib, size_t mem_to_save)
{
	size_t allocated_size;
	int    regions_nb;
	double average_size;
	_mpc_lowcomm_ib_ibuf_region_t *  region;
	_mpc_lowcomm_endpoint_state_t state;
	_mpc_lowcomm_ib_qp_t *        remote;

	mpc_common_spinlock_read_lock(&rdma_region_list_lock);
	__get_size_from_all_remotes(&allocated_size,
	                                                   &regions_nb);

	/* If we want to save more memory than
	 * the memory used for RDMA buffers, we should disconnect every remote */
	if(mem_to_save > allocated_size)
	{
		mpc_common_spinlock_read_unlock(&rdma_region_list_lock);
		return 0;
	}

	average_size = (allocated_size - mem_to_save) / ( float )regions_nb;

	if(rdma_region_list != NULL)
	{
		region = rdma_region_list;

		CDL_FOREACH(rdma_region_list, region)
		{
			remote = region->remote;

			/* Get the state associated to the region */
			if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
			{
				state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote);

				if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED && (average_size < region->allocated_size) )
				{
					/* Try to change the state to flushing.
					 * By changing the state of the remote to 'flushing', we automaticaly switch to SR */
					mpc_common_spinlock_lock(&remote->rdma.flushing_lock);
					state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTED, _MPC_LOWCOMM_ENDPOINT_FLUSHING);

					/* If we are allowed to deconnect */
					if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
					{
						/* Update the slots values requested to 0 -> means that we want to disconnect */
						remote->rdma.pool->resizing_request.send_keys.nb   = 0;
						remote->rdma.pool->resizing_request.send_keys.size = 0;
						mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);

						_mpc_lowcomm_ib_debug("Resizing the RMDA buffer for remote %d", remote->rank);
						_mpc_lowcomm_ib_ibuf_rdma_flush_send(rail_ib, remote);
					}
					else
					{
						mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);
					}
				}
			}
			else
			if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
			{
				state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(remote);

				if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED && (average_size < region->allocated_size) )
				{
					/* Change the state to 'requesting' */
					state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTED, _MPC_LOWCOMM_ENDPOINT_REQUESTING);
				}
				else
				{
					not_reachable();
				}
			}
		}
	}

	mpc_common_spinlock_read_unlock(&rdma_region_list_lock);
	return 1;
}

static inline _mpc_lowcomm_ib_ibuf_region_t *__get_remote_lru(char *name)
{
	_mpc_lowcomm_ib_ibuf_region_t *  region;
	_mpc_lowcomm_ib_ibuf_region_t *  elected_region = NULL;
	_mpc_lowcomm_endpoint_state_t state;

	sprintf(name, "LRU");

	mpc_common_spinlock_read_lock(&rdma_region_list_lock);

	if(rdma_region_list != NULL)
	{
		region = rdma_region_list;

		/* Infinite loops are so nice */
		while(1)
		{
			char found_connected = 0;
			CDL_FOREACH(rdma_region_list, region)
			{
				/* Get the state associated to the region */
				if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
				{
					state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(region->remote);
				}
				else
				if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
				{
					state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(region->remote);
				}
				else
				{
					not_reachable();
				}

				/* If state connected */
				if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
				{
					found_connected = 1;

					/* The RDMA buffer can be evicted */
					if(region->R_bit == 0)
					{
						elected_region = region;
						goto exit;
					}
					else /* == 1 */
					{
						region->R_bit = 0;
						region        = region->next;
					}
				}
			}

			/* There is no remote connected. So we can leave... */
			if(found_connected == 0)
			{
				goto exit;
			}
		}
	}

exit:
	mpc_common_spinlock_read_unlock(&rdma_region_list_lock);

	/* Return the elected region */
	return elected_region;
}

/*
 * Return the remote which is the most memory consumming in RDMA.
 * It may return NULL
 * Returns: the remote to disconnect
 * memory_used: total memory used for the RDMA connection (in kB)
 * */
static inline _mpc_lowcomm_ib_ibuf_region_t *__get_max_allocated_size(char *name)
{
	_mpc_lowcomm_ib_ibuf_region_t *region;
	size_t max = 0;
	_mpc_lowcomm_ib_ibuf_region_t *  max_region = NULL;
	_mpc_lowcomm_endpoint_state_t state;

	sprintf(name, "EMERGENCY");

	mpc_common_spinlock_read_lock(&rdma_region_list_lock);
	CDL_FOREACH(rdma_region_list, region)
	{
		size_t current = (~0);

		/* Get the state associated to the region */
		if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
		{
			state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(region->remote);
		}
		else
		if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
		{
			state = _mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(region->remote);
		}
		else
		{
			not_reachable();
		}

		/* If the region is connected, we can deconnect */
		if(state == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
		{
			current = region->allocated_size;

			if(current > max)
			{
				max        = current;
				max_region = region;
			}
		}
	}
	mpc_common_spinlock_read_unlock(&rdma_region_list_lock);

	return max_region;
}

/*
 *  Disconnect RDMA buffers
 */
size_t _mpc_lowcomm_ib_ibuf_rdma_remote_disconnect(_mpc_lowcomm_ib_rail_info_t *rail_ib)
{
	size_t memory_used = (~0);
	char   name[256]   = "";
	_mpc_lowcomm_ib_ibuf_region_t *region;

	/* MAX MEM */
	//region = __get_max_allocated_size( name);
	/* LRU */
	region = __get_remote_lru(name);
	/* NORMALIZATION */

	/* If we find a process to deconnect, we deconnect it */
	if(region != NULL)
	{
		_mpc_lowcomm_ib_qp_t *remote;
		mpc_common_nodebug("%s %d -> %.02f",
		                   name, region->remote->rank, region->allocated_size);

		remote      = region->remote;
		memory_used = region->allocated_size;

		if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL) )
		{
			/* Try to change the state to flushing.
			 * By changing the state of the remote to 'flushing', we automaticaly switch to SR */
			mpc_common_spinlock_lock(&remote->rdma.flushing_lock);
			_mpc_lowcomm_endpoint_state_t ret =
			        _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTED, _MPC_LOWCOMM_ENDPOINT_FLUSHING);

			/* If we are allowed to deconnect */
			if(ret == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
			{
				/* Update the slots values requested to 0 -> means that we want to disconnect */
				remote->rdma.pool->resizing_request.send_keys.nb   = 0;
				remote->rdma.pool->resizing_request.send_keys.size = 0;
				mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);

				_mpc_lowcomm_ib_debug("Resizing the RMDA buffer for remote %d", remote->rank);
				_mpc_lowcomm_ib_ibuf_rdma_flush_send(rail_ib, remote);
			}
			else
			{
				mpc_common_spinlock_unlock(&remote->rdma.flushing_lock);
				memory_used = 0;
			}
		}
		else
		if(region->channel == (MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL) )
		{
			/* Change the state to 'requesting' */
			_mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTED, _MPC_LOWCOMM_ENDPOINT_REQUESTING);
			memory_used = 0;
		}
		else
		{
			not_reachable();
		}
	}

	return memory_used;
}
