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

#include "sctk_ibufs.h"

#define SCTK_IB_MODULE_NAME "IBUF"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "utlist.h"
/**
   Description: NUMA aware buffers poll interface.
 *  Buffers are stored together in regions. Buffers are
 *  deallocated region per region.
 */

/* FIXME: use malloc_on_node instead of memalign
 * This function is *NOT* thread-safe */
static inline void
init_node(struct sctk_ib_rail_info_s *rail_ib,
    struct sctk_ibuf_numa_s* node, int nb_ibufs)
{
  LOAD_CONFIG(rail_ib);
  LOAD_MMU(rail_ib);
  sctk_ibuf_region_t *region = NULL;
  void* ptr = NULL;
  void* ibuf;
  sctk_ibuf_t* ibuf_ptr;
  int i;

  sctk_ib_debug("Allocating %d buffers", nb_ibufs);

  region = sctk_malloc_on_node(sizeof(sctk_ibuf_region_t), 0);
  assume(region);

  /* XXX: replaced by memalign_on_node */
  sctk_posix_memalign( (void**) &ptr, mmu->page_size, nb_ibufs * config->ibv_eager_limit);
  assume(ptr);
  memset(ptr, 0, nb_ibufs * config->ibv_eager_limit);

  /* XXX: replaced by memalign_on_node */
   sctk_posix_memalign(&ibuf, mmu->page_size, nb_ibufs * sizeof(sctk_ibuf_t));
  assume(ibuf);
  memset (ibuf, 0, nb_ibufs * sizeof(sctk_ibuf_t));
  /* XXX: memset disabled for optimizing launch
   * memset (ptr, 0, nb_ibufs * ibv_eager_limit); */

  region->ibuf = ibuf;
  region->nb = nb_ibufs;
  region->node = node;
  region->rail = rail_ib;
  DL_APPEND(node->regions, region);

  /* register buffers at once */
  region->mmu_entry = sctk_ib_mmu_register(rail_ib, ptr,
      nb_ibufs * config->ibv_eager_limit);
  sctk_nodebug("Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey);

  /* init all buffers - the last one */
  for(i=0; i < nb_ibufs; ++i) {
    ibuf_ptr = (sctk_ibuf_t*) ibuf + i;
    ibuf_ptr->region = region;
    ibuf_ptr->size = 0;
    ibuf_ptr->flag = FREE_FLAG;

    ibuf_ptr->buffer = (unsigned char*) ((char*) ptr + (i*config->ibv_eager_limit));
    assume(ibuf_ptr->buffer);
    DL_APPEND(node->free_header, ((sctk_ibuf_t*) ibuf + i));
  }

  node->free_nb += nb_ibufs;
  node->nb += nb_ibufs;

  sctk_ib_debug("[node:%d] Allocation of %d buffers (free_nb:%u got:%u)", node->id, nb_ibufs, node->free_nb, node->nb - node->free_nb);
}

void
sctk_ibuf_pool_init(struct sctk_ib_rail_info_s *rail_ib)
{
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_pool_t *pool;
  int i;
  unsigned int nodes_nb = sctk_get_numa_node_number();
  /* FIXME: get_numa_node_number which returns 0 when SMP */
  nodes_nb = (nodes_nb == 0) ? 1 : nodes_nb;

  pool = sctk_malloc(sizeof(sctk_ibuf_pool_t) +
      nodes_nb * sizeof(sctk_ibuf_numa_t));
  memset(pool, 0, sizeof(sctk_ibuf_pool_t) +
      nodes_nb * sizeof(sctk_ibuf_numa_t));
  assume (pool);
  pool->nodes_nb = nodes_nb;
  pool->nodes = (sctk_ibuf_numa_t*)
    ((char*) pool + sizeof(sctk_ibuf_pool_t));

  /* loop on nodes and create buffers */
  for (i=0; i < nodes_nb; ++i) {
    pool->nodes[i].id = i;
    pool->nodes[i].lock = SCTK_SPINLOCK_INITIALIZER;
    init_node(rail_ib, &pool->nodes[i], config->ibv_init_ibufs);
  }
  /* update pool buffer */
  rail_ib->pool_buffers = pool;
}

/* pick a new buffer from the ibuf list. Function *MUST* be locked to avoid
 * multiple calls*/
sctk_ibuf_t*
sctk_ibuf_pick(struct sctk_ib_rail_info_s *rail_ib,
    int need_lock, int n)
{
  LOAD_POOL(rail_ib);
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_t* ibuf;
  sctk_ibuf_numa_t *node = &pool->nodes[n];
  sctk_spinlock_t *lock = &node->lock;

  if (need_lock) sctk_spinlock_lock(lock);

  if (!node->free_header)
      init_node(rail_ib, node, config->ibv_size_ibufs_chunk);

  /* update pointers from buffer pool */
  ibuf = node->free_header;
  DL_DELETE(node->free_header, node->free_header);
  node->free_nb--;

 if (need_lock) sctk_spinlock_unlock(lock);

#ifdef DEBUG_IB_BUFS
  assume(ibuf);
  if (ibuf->flag != FREE_FLAG)
  {sctk_error("Wrong flag (%d) got from ibuf", ibuf->flag);}
#endif

  sctk_nodebug("ibuf: %p, lock:%p, need_lock:%d next free_header: %p, nb_free %d, nb_got %d, nb_free_srq %d, node %d)", ibuf, lock, need_lock, node->free_header, node->nb_free, node->nb_got, node->nb_free_srq, n);

  return ibuf;
}

static int srq_post(
    struct sctk_ib_rail_info_s *rail_ib,
    int nb_ibufs, sctk_ibuf_numa_t *node, int need_lock)
{
  LOAD_DEVICE(rail_ib);
  int i;
  int nb_posted = 0;
  sctk_ibuf_t* ibuf;
  int rc;
  sctk_spinlock_t *lock = &node->lock;

  if(need_lock) sctk_spinlock_lock(lock);
  for (i=0; i < nb_ibufs; ++i)
  {
    ibuf = sctk_ibuf_pick(rail_ib, 0, node->id);
#ifdef DEBUG_IB_BUFS
    assume(ibuf);
#endif

    sctk_ibuf_recv_init(ibuf);

    rc = ibv_post_srq_recv(device->srq, &(ibuf->desc.wr.recv), &(ibuf->desc.bad_wr.recv));

    if (rc != 0)
    {
      ibuf->flag = FREE_FLAG;

      /* change counters */
      node->free_nb++;
      DL_PREPEND(node->free_header, ibuf);
      break;
    }
    else nb_posted++;
  }

  node->free_srq_nb+=nb_posted;
  if (need_lock) sctk_spinlock_unlock(lock);

  sctk_nodebug("posted: %d on nb_ibufs %d (free_srq %d, got_srq %d)", nb_posted, nb_ibufs, node->nb_free_srq, node->nb_got_srq);
  return nb_posted;
}

/* FIXME: check with buffers near IB card */
int sctk_ibuf_srq_check_and_post(
    struct sctk_ib_rail_info_s *rail_ib,
    int limit)
{
  LOAD_POOL(rail_ib);
  int node = 0;

  return srq_post(rail_ib, limit, &pool->nodes[node], 1);
}

/* release one buffer given as parameter.
 * is_srq: if the buffer is released from the SRQ */
void sctk_ibuf_release(
    struct sctk_ib_rail_info_s *rail_ib,
    sctk_ibuf_t* ibuf, int is_srq)
{
  assume(ibuf);
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_numa_t *node = ibuf->region->node;
  sctk_spinlock_t *lock = &node->lock;

  ibuf->flag = FREE_FLAG;

  sctk_spinlock_lock(lock);
  node->free_nb++;
  DL_APPEND(node->free_header, ibuf);
  /* If SRQ, we check and try to post more messages to SRQ */
  if (is_srq) {
    /* limit of buffer posted */
    int limit;

    node->free_srq_nb--;

    limit = config->ibv_max_srq_ibufs - node->free_srq_nb;
    sctk_debug("Post new buffer !!!!!!!!!!! : %d (%d - %d)", limit, config->ibv_max_srq_ibufs, node->free_srq_nb);
    if (limit > 0) {
      srq_post(rail_ib, limit, node, 0);
    }
  }
  sctk_debug("Buffer %p free (%d)", ibuf, is_srq);
  sctk_spinlock_unlock(lock);
}


#if 0
sctk_net_ibv_ibuf_numa_t* get_numa_node(const int core_id)
{
  return &ibuf_on_node[sctk_get_node_from_cpu(core_id)];
}


#endif

/*-----------------------------------------------------------
 *  WR INITIALIZATION
 *----------------------------------------------------------*/
void sctk_ibuf_recv_init(sctk_ibuf_t* ibuf)
{
  sctk_assert(ibuf);
  LOAD_CONFIG(ibuf->region->rail);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = config->ibv_eager_limit;

  ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) (ibuf->buffer);

  ibuf->flag = NORMAL_IBUF_FLAG;
}

void sctk_ibuf_rdma_recv_init(sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey)
{
  sctk_assert(ibuf);
  LOAD_CONFIG(ibuf->region->rail);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = config->ibv_eager_limit;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = NORMAL_IBUF_FLAG;
}

void sctk_ibuf_barrier_send_init(sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = BARRIER_IBUF_FLAG;
}

void sctk_ibuf_send_init(
    sctk_ibuf_t* ibuf, size_t size)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_SEND;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = size;
  ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) (ibuf->buffer);

  ibuf->flag = NORMAL_IBUF_FLAG;
}

void sctk_ibuf_rdma_write_init(
    sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = RDMA_WRITE_IBUF_FLAG;
}

void sctk_ibuf_rdma_read_init(
    sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len, void* supp_ptr, int dest_process)
{
  sctk_assert(ibuf);

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_READ;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = RDMA_READ_IBUF_FLAG;
  ibuf->supp_ptr = supp_ptr;
  ibuf->dest_process = dest_process;
}
#endif
