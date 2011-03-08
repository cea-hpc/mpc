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

#ifndef __SCTK__INFINIBAND_IBUFS_H_
#define __SCTK__INFINIBAND_IBUFS_H_

#include "sctk_config.h"
#include "sctk_spinlock.h"
#include "stdint.h"
#include "infiniband/verbs.h"
#include "sctk_infiniband_mmu.h"

/* must be a multiple of 64 bytes */
#define IBUF_SIZE (8 * 1024)

/* type of ibuf */
#define RDMA_READ_IBUF_FLAG (111)
#define RDMA_WRITE_IBUF_FLAG (222)
#define NORMAL_IBUF_FLAG (333)
#define BARRIER_IBUF_FLAG (444)
#define BUSY_FLAG (1)
#define FREE_FLAG (0)

static char* sctk_net_ibv_ibuf_print_flag (int flag)
{
  switch(flag) {
    case RDMA_READ_IBUF_FLAG:   return "RDMA_READ_IBUF_FLAG";break;
    case RDMA_WRITE_IBUF_FLAG:  return "RDMA_WRITE_IBUF_FLAG";break;
    case NORMAL_IBUF_FLAG:      return "NORMAL_IBUF_FLAG";break;
    case BARRIER_IBUF_FLAG:     return "BARRIER_IBUF_FLAG";break;
    case BUSY_FLAG:             return "BUSY_FLAG";break;
    case FREE_FLAG:             return "FREE_FLAG";break;
  }
  return NULL;
}

extern uint32_t                     ibuf_free_ibuf_nb;
extern uint32_t                     ibuf_got_ibuf_nb;
extern uint32_t                     ibuf_free_srq_nb;
extern uint32_t                     ibuf_got_srq_nb;


typedef struct sctk_net_ibv_ibuf_region_s
{
  sctk_net_ibv_mmu_entry_t* mmu_entry;

  uint16_t nb;

  struct sctk_net_ibv_ibuf_s* ibuf;
  struct sctk_net_ibv_ibuf_region_s* next_region;
} sctk_net_ibv_ibuf_region_t;


typedef struct sctk_net_ibv_ibuf_desc_s
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
  struct sctk_net_ibv_ibuf_s* next;
} sctk_net_ibv_ibuf_desc_t;


typedef struct sctk_net_ibv_ibuf_s
{
  /* / ! \ must be first in the structure for data alignment */
  struct sctk_net_ibv_ibuf_desc_s desc;
  sctk_net_ibv_ibuf_region_t* region;

  unsigned char* buffer;
  size_t size;
  int flag;

  /* the following infos aren't transmitted by the network */
  sctk_net_ibv_qp_remote_t*    remote;
  void* supp_ptr;

} sctk_net_ibv_ibuf_t;


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_net_ibv_ibuf_new();

void sctk_net_ibv_ibuf_init( sctk_net_ibv_qp_rail_t *rail,
    sctk_net_ibv_qp_local_t* local, int nb_ibufs);

sctk_net_ibv_ibuf_t* sctk_net_ibv_ibuf_pick(int return_on_null);

void sctk_net_ibv_ibuf_release(sctk_net_ibv_ibuf_t* ibuf, int is_srq);

void sctk_net_ibv_ibuf_recv_init(sctk_net_ibv_ibuf_t* ibuf);

void sctk_net_ibv_ibuf_send_init(
    sctk_net_ibv_ibuf_t* ibuf, size_t size);

void sctk_net_ibv_ibuf_rdma_write_init(
    sctk_net_ibv_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len);

void sctk_net_ibv_ibuf_rdma_read_init(
    sctk_net_ibv_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len, void* supp_ptr);

int sctk_net_ibv_ibuf_srq_check_and_post(
    sctk_net_ibv_qp_local_t* local);

int sctk_net_ibv_ibuf_srq_post(
    sctk_net_ibv_qp_local_t* local,
    int nb_ibufs);

void sctk_net_ibv_ibuf_barrier_send_init(
    sctk_net_ibv_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len);

#endif
