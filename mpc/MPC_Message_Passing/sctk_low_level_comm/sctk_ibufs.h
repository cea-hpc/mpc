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
#ifndef __SCTK__INFINIBAND_IBUFS_H_
#define __SCTK__INFINIBAND_IBUFS_H_

#include "stdint.h"
#include "infiniband/verbs.h"

#include "sctk_spinlock.h"
#include "sctk_ib_mmu.h"

struct sctk_ib_rail_info_s;

/*-----------------------------------------------------------
 *  STRUCTURES
 *----------------------------------------------------------*/

/* Description of an ibuf */
typedef struct sctk_ibuf_desc_s
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
  /* ibufs can be chained together. */
  struct sctk_net_ibv_ibuf_s* next;
} sctk_ibuf_desc_t;

/* NUMA aware pool of buffers */
typedef struct sctk_ibuf_numa_s
{
  /* id of the region */
  int id;
  /* DL list of regions */
  struct sctk_ibuf_region_s  *regions;
  /* flag to the first free header */
  struct sctk_ibuf_s         *free_header;
  /* lock when moving pointers */
  sctk_spinlock_t lock;
  /* Number of buffers created and free in total,
   * and free for srq */
  unsigned int nb;
  unsigned int free_nb;
  unsigned int free_srq_nb;
} sctk_ibuf_numa_t
/* FIXME: only with GCC and ICC. Padd to the size of a cache
 * for avoind false sharing */
__attribute__((__aligned__(64)));

/* Region of buffers where several buffers
 * are allocated */
typedef struct sctk_ibuf_region_s
{
  struct sctk_ibuf_region_s* next;
  struct sctk_ibuf_region_s* prev;
  /* Number of buffer for the region */
  uint16_t nb;
  /* A region is associated to a rail */
  struct sctk_ib_rail_info_s* rail;
  /* MMU entry */
  struct sctk_ib_mmu_entry_s* mmu_entry;

  struct sctk_ibuf_numa_s *node;

  struct sctk_ibuf_s* ibuf;
} sctk_ibuf_region_t;

/* Poll of ibufs */
typedef struct sctk_ibuf_pool_s
{
  /* number of NUMA nodes */
  unsigned int nodes_nb;
  /* NUMA nodes */
  sctk_ibuf_numa_t *nodes;

} sctk_ibuf_pool_t;

/* type of an ibuf */
enum sctk_ibuf_status
{
  BUSY_FLAG             = 111,
  FREE_FLAG             = 222,
  RDMA_READ_IBUF_FLAG   = 333,
  RDMA_WRITE_IBUF_FLAG  = 444,
  NORMAL_IBUF_FLAG      = 555,
  BARRIER_IBUF_FLAG     = 666
};

static char* sctk_ibuf_print_flag (enum sctk_ibuf_status flag)
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

/* Describes an ibuf */
typedef struct sctk_ibuf_s
{
  /* XXX: desc must be first in the structure
   * for data alignment troubles */
  struct sctk_ibuf_desc_s desc;
  sctk_ibuf_region_t* region;

  /* pointer to the buffer and its size */
  unsigned char* buffer;
  size_t size;
  enum sctk_ibuf_status flag;

  /* the following infos aren't transmitted by the network */
  struct sctk_ib_qp_t*    remote;
  void* supp_ptr;
  int dest_process;

  struct sctk_ibuf_s* next;
  struct sctk_ibuf_s* prev;
} sctk_ibuf_t;


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void sctk_ibuf_pool_init(struct sctk_ib_rail_info_s *rail);

sctk_ibuf_t*
sctk_ibuf_pick(struct sctk_ib_rail_info_s *rail_ib,
    int need_lock, int n);

int sctk_ibuf_srq_check_and_post(
    struct sctk_ib_rail_info_s *rail_ib, int limit);

/*-----------------------------------------------------------
 *  WR INITIALIZATION
 *----------------------------------------------------------*/
void sctk_ibuf_recv_init(sctk_ibuf_t* ibuf);

void sctk_ibuf_rdma_recv_init(sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey);

void sctk_ibuf_barrier_send_init(sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len);

void sctk_ibuf_send_init(
    sctk_ibuf_t* ibuf, size_t size);

void sctk_ibuf_rdma_write_init(
    sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len);

void sctk_ibuf_rdma_read_init(
    sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len, void* supp_ptr, int dest_process);

void sctk_ibuf_release(
    struct sctk_ib_rail_info_s *rail_ib,
    sctk_ibuf_t* ibuf, int is_srq);

#endif
#endif
