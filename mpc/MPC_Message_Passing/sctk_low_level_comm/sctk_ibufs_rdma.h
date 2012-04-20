/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__INFINIBAND_IBUFS_RDMA_H_
#define __SCTK__INFINIBAND_IBUFS_RDMA_H_

#include "stdint.h"
#include "infiniband/verbs.h"

#include "sctk_spinlock.h"
#include "sctk_ib_mmu.h"
#include "sctk_ib.h"
#include "sctk_ib_qp.h"

struct sctk_ib_rail_info_s;

#define IBUF_RDMA_GET_HEAD(remote,ptr) (remote->ibuf_rdma->region[ptr].head)
#define IBUF_RDMA_GET_TAIL(remote,ptr) (remote->ibuf_rdma->region[ptr].tail)
#define IBUF_RDMA_GET_NEXT(i) \
  ( ( (i->index + 1) >= i->region->nb) ? \
    (sctk_ibuf_t*) i->region->ibuf : \
    (sctk_ibuf_t*) i->region->ibuf + (i->index + 1))

#define IBUF_RDMA_GET_ADDR_FROM_INDEX(remote,ptr,index) \
  ((sctk_ibuf_t*) remote->ibuf_rdma->region[ptr].ibuf + index)

#define IBUF_RDMA_GET_REGION(remote,ptr) (&remote->ibuf_rdma->region[ptr])

#define IBUF_RDMA_LOCK_REGION(remote,ptr) (sctk_spinlock_lock(&remote->ibuf_rdma->region[ptr].lock))
#define IBUF_RDMA_UNLOCK_REGION(remote,ptr) (sctk_spinlock_unlock(&remote->ibuf_rdma->region[ptr].lock))

#define IBUF_RDMA_GET_REMOTE_ADDR(remote,ptr,ibuf) \
  ((char*) remote->ibuf_rdma->remote_region[ptr] + (ibuf->index * config->ibv_eager_rdma_limit))

#define IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote,ptr,ibuf) \
  ((char*) remote->ibuf_rdma->remote_region[ptr] + (ibuf->index * config->ibv_eager_rdma_limit))

/* Pool of ibufs */
#define REGION_SEND 0
#define REGION_RECV 1
typedef struct sctk_ibuf_rdma_pool_s
{
  /* Pointer to the RDMA regions: send and recv */
  struct sctk_ibuf_region_s region[2];

  /* Remote addr of the buffers pool */
  struct sctk_ibuf_region_s* remote_region[2];

  /* Rkey of the remote buffer pool */
  uint32_t rkey[2];

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
    struct ibv_recv_wr* recv;
    struct ibv_send_wr* send;
  } bad_wr;

  struct ibv_sge sg_entry;
} sctk_ibuf_rdma_desc_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
sctk_ibuf_rdma_pool_t*
sctk_ibuf_rdma_pool_init(struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t* remote, int nb_ibufs);

void
sctk_ibuf_rdma_update_remote_addr(sctk_ib_qp_t* remote, sctk_ib_qp_keys_t *key);

void sctk_ibuf_rdma_release(sctk_ib_rail_info_t* rail_ib, sctk_ibuf_t* ibuf);

#endif
#endif
