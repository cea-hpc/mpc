/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
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
#ifndef __SCTK__INFINIBAND_IBUFS_RDMA_H_
#define __SCTK__INFINIBAND_IBUFS_RDMA_H_

#include "infiniband/verbs.h"

#include "mpc_common_spinlock.h"
#include "sctk_ib_mmu.h"
#include "sctk_ib.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_cm.h"
#include "mpc_common_types.h"

struct sctk_ib_rail_info_s;

#define IBUF_RDMA_GET_HEAD(remote,ptr) (remote->rdma.pool->region[ptr].head)
#define IBUF_RDMA_GET_TAIL(remote,ptr) (remote->rdma.pool->region[ptr].tail)
#define IBUF_RDMA_GET_NEXT(i) \
  ( ( (i->index + 1) >= i->region->nb) ? \
    (sctk_ibuf_t*) i->region->ibuf : \
    (sctk_ibuf_t*) i->region->ibuf + (i->index + 1))

#define IBUF_RDMA_ADD(i, x) \
  ( (sctk_ibuf_t*) ( (i->region->ibuf + ( ( i->index + x  ) % (i->region->nb) ) ) ) )

#define IBUF_RDMA_GET_ADDR_FROM_INDEX(remote,ptr,index) \
  ((sctk_ibuf_t*) remote->rdma.pool->region[ptr].ibuf + index)

#define IBUF_RDMA_GET_REGION(remote,ptr) (&remote->rdma.pool->region[ptr])

#define IBUF_RDMA_LOCK_REGION(remote,ptr) (mpc_common_spinlock_lock(&remote->rdma.pool->region[ptr].lock))
#define IBUF_RDMA_TRYLOCK_REGION(remote,ptr) (mpc_common_spinlock_trylock(&remote->rdma.pool->region[ptr].lock))
#define IBUF_RDMA_UNLOCK_REGION(remote,ptr) (mpc_common_spinlock_unlock(&remote->rdma.pool->region[ptr].lock))

#define IBUF_RDMA_LOCK_POLLING(remote) (mpc_common_spinlock_lock(&remote->rdma.polling_lock))
#define IBUF_RDMA_TRYLOCK_POLLING(remote) (mpc_common_spinlock_trylock(&remote->rdma.polling_lock))
#define IBUF_RDMA_UNLOCK_POLLING(remote) (mpc_common_spinlock_unlock(&remote->rdma.polling_lock))

#define IBUF_RDMA_GET_REMOTE_ADDR(remote,ibuf) \
  ((char*) remote->rdma.pool->remote_region[REGION_RECV] + (ibuf->index * \
    remote->rdma.pool->region[REGION_SEND].size_ibufs))

#define IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote,ibuf) \
  &(IBUF_GET_EAGER_PIGGYBACK( \
    (IBUF_RDMA_GET_PAYLOAD_FLAG( \
      (char*) remote->rdma.pool->remote_region[REGION_SEND] + (ibuf->index * \
        remote->rdma.pool->region[REGION_RECV].size_ibufs) \
    )) \
  ))

/*
 * This figure discribes how the buffer is
 * decomposed with the size of each filed.
 *  _____________
 * |             |    <--- BASE_FLAG
 * |POISON  (int)|
 * |_____________|
 * |             |
 * |SIZE (size_t)|
 * |_____________|
 * |             |
 * | HEAD (int)  |
 * |_____________|    <--- ibuf->buffer
 * |             |
 * |    DATA     |
 * |             |
 * |_____________|
 * |             |
 * | TAIL (int)  |
 * |_____________|
 *
 */
#define IBUF_RDMA_RESET_FLAG   120983
#define IBUF_RDMA_FLAG_1 989898
#define IBUF_RDMA_FLAG_2 434343
#define ALIGNED_RDMA_BUFFER (sizeof(size_t) + 2*sizeof(int))
//#define ALIGNED_RDMA_BUFFER ALIGN_ON((sizeof(size_t) + 2*sizeof(int)), 8)

/* ATTENTION: this macro *MUST* point to the
 * base of the ibuf */
#define IBUF_RDMA_GET_BASE_FLAG(ibuf) \
  ((void*) ibuf->poison_flag_head)

#define IBUF_RDMA_POISON_HEAD (~ (0<<16))
#define IBUF_RDMA_GET_POISON_FLAG_HEAD(buffer) \
  ((void*) ((char*) buffer))

#define IBUF_RDMA_CHECK_POISON_FLAG(ibuf) do {\
   if ( *(ibuf->poison_flag_head) != IBUF_RDMA_POISON_HEAD) {  \
      sctk_error("Got a wrong Poison flag. Aborting");  \
      sctk_abort(); \
    } \
}while(0)\
 
#define IBUF_RDMA_GET_SIZE_FLAG(buffer) \
  ((void*) ((char*) buffer + sizeof(int)))

#define IBUF_RDMA_GET_HEAD_FLAG(buffer) \
  ((void*) ((char*) buffer + sizeof(size_t) + sizeof(int)))

#define IBUF_RDMA_GET_PAYLOAD_FLAG(buffer) \
  ((void*) ((char*) buffer + ALIGNED_RDMA_BUFFER ))

/* 's' is the size of the data.
 * XXX: We only start to count from the buffer. */
#define IBUF_RDMA_GET_TAIL_FLAG(buffer, s) \
  ((void*) ((char*) buffer + s))

/* The payload size must be set before using this macro */
#define IBUF_RDMA_GET_SIZE \
 (ALIGNED_RDMA_BUFFER + sizeof(int) )

/* Pool of ibufs */
#define REGION_SEND 0
#define REGION_RECV 1

typedef struct sctk_ibuf_rdma_pool_s
{
	/* Pointer to the RDMA regions: send and recv */
	struct sctk_ibuf_region_s region[2];

	/* Remote addr of the buffers pool */
	struct sctk_ibuf_region_s *remote_region[2];

	/* Rkey of the remote buffer pool */
	uint32_t rkey[2];

	/* Number of pending entries busy in the RDMA buffer.
	 * Only for send region */
	OPA_int_t            busy_nb[2];

	/* Request for resizing */
	struct
	{
		sctk_ib_cm_rdma_connection_t send_keys;
		sctk_ib_cm_rdma_connection_t recv_keys;
	} resizing_request;

	/* The maximum number of data pending */
	size_t max_data_pending;
	int send_credit;

	/* Pointer to the remote */
	sctk_ib_qp_t *remote;
	/* Pointer to next and tail for list */
	struct sctk_ibuf_rdma_pool_s *next;
	struct sctk_ibuf_rdma_pool_s *prev;

} sctk_ibuf_rdma_pool_t;

/* Description of an ibuf */
typedef struct sctk_ibuf_rdma_desc_s
{
	union
	{
		struct ibv_recv_wr recv;
		struct ibv_send_wr send;
	} wr;
	union
	{
		struct ibv_recv_wr *recv;
		struct ibv_send_wr *send;
	} bad_wr;

	struct ibv_sge sg_entry;
} sctk_ibuf_rdma_desc_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void sctk_ibuf_rdma_remote_init ( sctk_ib_qp_t *remote );

int sctk_ibuf_rdma_is_connectable ( sctk_ib_rail_info_t *rail_ib);

void sctk_ibuf_rdma_update_remote ( sctk_ib_qp_t *remote, size_t size );

void sctk_ibuf_rdma_check_remote ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote );

sctk_ibuf_rdma_pool_t *
sctk_ibuf_rdma_pool_init ( sctk_ib_qp_t *remote );

void
sctk_ibuf_rdma_update_remote_addr ( sctk_ib_qp_t *remote, sctk_ib_cm_rdma_connection_t *key, int region );

void sctk_ibuf_rdma_release ( sctk_ib_rail_info_t *rail_ib, sctk_ibuf_t *ibuf );

int
sctk_ib_rdma_eager_poll_remote ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote );

void sctk_ib_rdma_eager_walk_remotes ( sctk_ib_rail_info_t *rail, int ( func ) ( sctk_ib_rail_info_t *rail, sctk_ib_qp_t *remote ), int *ret );

void sctk_ibuf_rdma_update_max_pending_requests ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote );

void sctk_ib_rdma_eager_poll_recv ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote, int index );

sctk_ibuf_t *sctk_ibuf_rdma_pick ( sctk_ib_qp_t *remote );

void sctk_ib_rdma_set_tail_flag ( sctk_ibuf_t *ibuf, size_t size );

void sctk_ibuf_rdma_region_init ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote,
                                  sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel, int nb_ibufs, int size_ibufs );
void sctk_ibuf_rdma_region_reinit ( struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote,
                                    sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel, int nb_ibufs, int size_ibufs );

size_t sctk_ibuf_rdma_get_eager_limit ( sctk_ib_qp_t *remote );

void sctk_ibuf_rdma_determine_config_from_sample ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote, int *nb,
                                                   int *size_ibufs );

void sctk_ibuf_rdma_connection_cancel ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote );

void sctk_ibuf_rdma_fill_remote_addr ( sctk_ib_qp_t *remote,
                                       sctk_ib_cm_rdma_connection_t *keys, int region );

/*-----------------------------------------------------------
 *  ACESSORS
 *----------------------------------------------------------*/

static __UNUSED__ void
sctk_ibuf_rdma_set_remote_state_rtr ( sctk_ib_qp_t *remote, sctk_endpoint_state_t state )
{
	OPA_store_int ( &remote->rdma.state_rtr, ( int ) state );
}

static __UNUSED__ sctk_endpoint_state_t
sctk_ibuf_rdma_get_remote_state_rtr ( sctk_ib_qp_t *remote )
{
	return ( sctk_endpoint_state_t ) OPA_load_int ( &remote->rdma.state_rtr );
}

/* Return the old value */
static __UNUSED__ sctk_endpoint_state_t
sctk_ibuf_rdma_cas_remote_state_rtr ( sctk_ib_qp_t *remote,
                                      sctk_endpoint_state_t oldv, sctk_endpoint_state_t newv )
{
	return ( sctk_endpoint_state_t ) OPA_cas_int ( &remote->rdma.state_rtr, oldv, newv );
}


static __UNUSED__ void
sctk_ibuf_rdma_set_remote_state_rts ( sctk_ib_qp_t *remote, sctk_endpoint_state_t state )
{
	OPA_store_int ( &remote->rdma.state_rts, ( int ) state );
}

static __UNUSED__ sctk_endpoint_state_t
sctk_ibuf_rdma_get_remote_state_rts ( sctk_ib_qp_t *remote )
{
	return ( sctk_endpoint_state_t ) OPA_load_int ( &remote->rdma.state_rts );
}

/* Return the old value */
static __UNUSED__ sctk_endpoint_state_t
sctk_ibuf_rdma_cas_remote_state_rts ( sctk_ib_qp_t *remote,
                                      sctk_endpoint_state_t oldv, sctk_endpoint_state_t newv )
{
	return ( sctk_endpoint_state_t ) OPA_cas_int ( &remote->rdma.state_rts, oldv, newv );
}

int sctk_ibuf_rdma_check_send_flush ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote );

void sctk_ibuf_rdma_flush_recv ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote );
int sctk_ibuf_rdma_check_flush_send ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote );
int sctk_ibuf_rdma_check_flush_recv ( sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote );
size_t sctk_ibuf_rdma_get_regions_get_allocate_size ( sctk_ib_qp_t *remote );
void sctk_ibuf_rdma_update_max_pending_data ( sctk_ib_qp_t *remote, int current_pending );
#endif
#endif
