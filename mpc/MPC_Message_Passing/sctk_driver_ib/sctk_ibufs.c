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

#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "IBUF"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "sctk_ibufs_rdma.h"
#include "utlist.h"
/**
   Description: NUMA aware buffers poll interface.
 *  Buffers are stored together in regions. Buffers are
 *  deallocated region per region.
 *  TODO: Use HWLOC for detecting which core is the closest to the
 *  IB card.
 */
#define CLOSEST_NODE_FROM_NIC 0

/* Init a poll of buffers on the numa node pointed by 'node'
 * FIXME: use malloc_on_node instead of memalign
 * Carreful: this function is *NOT* thread-safe */
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
  int free_nb;

  sctk_ib_debug("Allocating %d buffers", nb_ibufs);

  region = sctk_malloc_on_node(sizeof(sctk_ibuf_region_t), 0);
  assume(region);

  /* XXX: replaced by memalign_on_node */
  sctk_posix_memalign( (void**) &ptr, mmu->page_size, nb_ibufs * config->ibv_eager_limit);
  assume(ptr);
  // memset(ptr, 0, nb_ibufs * config->ibv_eager_limit);

  /* XXX: replaced by memalign_on_node */
   sctk_posix_memalign(&ibuf, mmu->page_size, nb_ibufs * sizeof(sctk_ibuf_t));
  assume(ibuf);
  // memset (ibuf, 0, nb_ibufs * sizeof(sctk_ibuf_t));

  region->list = ibuf;
  region->nb = nb_ibufs;
  region->node = node;
  region->rail = rail_ib;
  region->channel = RC_SR_CHANNEL;
  region->ibuf = ibuf;
  DL_APPEND(node->regions, region);

  /* register buffers at once
   * FIXME: Always task on NUMA node 0 which firs-touch all pages... really bad */
  region->mmu_entry = sctk_ib_mmu_register_no_cache(rail_ib, ptr,
      nb_ibufs * config->ibv_eager_limit);
  sctk_nodebug("Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey);

  /* init all buffers - the last one */
  for(i=0; i < nb_ibufs; ++i) {
    ibuf_ptr = (sctk_ibuf_t*) ibuf + i;
    ibuf_ptr->region = region;
    ibuf_ptr->size = 0;
    ibuf_ptr->flag = FREE_FLAG;
    ibuf_ptr->index = i;

    ibuf_ptr->buffer = (unsigned char*) ((char*) ptr + (i*config->ibv_eager_limit));
    assume(ibuf_ptr->buffer);
    DL_APPEND(node->free_entry, ((sctk_ibuf_t*) ibuf + i));
  }

  OPA_add_int(&node->free_nb, nb_ibufs);
  free_nb = OPA_load_int(&node->free_nb);
  node->nb += nb_ibufs;

  sctk_ib_debug("[node:%d] Allocation of %d buffers (free_nb:%u got:%u)", node->id, nb_ibufs, free_nb, node->nb - free_nb);
}

/* Init the pool of buffers on each NUMA node */
void
sctk_ibuf_pool_init(struct sctk_ib_rail_info_s *rail_ib)
{
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_pool_t *pool;
  int i;
  int alloc_nb;
  unsigned int nodes_nb;

  nodes_nb = sctk_get_numa_node_number();
 /* FIXME: get_numa_node_number may returns 1 when SMP */
  nodes_nb = (nodes_nb == 0) ? 1 : nodes_nb;
  alloc_nb = nodes_nb;

  /* We malloc the entries */
  pool = sctk_malloc(sizeof(sctk_ibuf_pool_t) +
      alloc_nb * sizeof(sctk_ibuf_numa_t));
  memset(pool, 0, sizeof(sctk_ibuf_pool_t) +
      alloc_nb * sizeof(sctk_ibuf_numa_t));
  assume (pool);
  /* Number of NUMA nodes */
  pool->nodes_nb = nodes_nb;
  pool->nodes = (sctk_ibuf_numa_t*)
    ((char*) pool + sizeof(sctk_ibuf_pool_t));

  /* loop on nodes and create buffers */
  for (i=0; i < alloc_nb; ++i) {
    sctk_ibuf_numa_t *node = &pool->nodes[i];

    node->id = i;
    node->lock = SCTK_SPINLOCK_INITIALIZER;
    OPA_store_int(&node->free_srq_nb, 0);
    OPA_store_int(&node->free_nb, 0);
    init_node(rail_ib, &pool->nodes[i], config->ibv_init_ibufs);
  }
  /* update pool buffer */
  rail_ib->pool_buffers = pool;
}

sctk_ibuf_t*
sctk_ibuf_pick_send_sr(struct sctk_ib_rail_info_s *rail_ib, int n)
{
  LOAD_POOL(rail_ib);
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_t* ibuf;

#ifdef DEBUG_IB_BUFS
    if (n != -1)  {
      assume(n <= pool->nodes_nb);
    }
#endif

    if (n == -1) n = CLOSEST_NODE_FROM_NIC;
    sctk_ibuf_numa_t *node = &pool->nodes[n];
    sctk_spinlock_t *lock = &node->lock;

    if (OPA_load_int(&node->free_nb) < 100) {
      sctk_ib_low_mem_broadcast(rail_ib->rail);
    }

    sctk_spinlock_lock(lock);

    /* Allocate additionnal buffers if no more are available */
    if ( !node->free_entry) {
      init_node(rail_ib, node, config->ibv_size_ibufs_chunk);
    }

    /* update pointers from buffer pool */
    ibuf = node->free_entry;
    DL_DELETE(node->free_entry, node->free_entry);
    OPA_decr_int(&node->free_nb);

    sctk_spinlock_unlock(lock);

    IBUF_SET_PROTOCOL(ibuf->buffer, null_protocol);

#ifdef DEBUG_IB_BUFS
    assume(ibuf);
    if (ibuf->flag != FREE_FLAG)
    {sctk_error("Wrong flag (%d) got from ibuf", ibuf->flag);}
#endif

    sctk_nodebug("ibuf: %p, lock:%p, need_lock:%d next free_entryr: %p, nb_free %d, nb_got %d, nb_free_srq %d, node %d)", ibuf, lock, need_lock, node->free_entry, node->nb_free, node->nb_got, node->nb_free_srq, n);

    /* Prepare the buffer for sending */
  IBUF_SET_POISON(ibuf->buffer);

  return ibuf;
}

/*
 * Prepare a message to send. According to the size and the remote, the function
 * choose between the SR and the RDMA channel.
 * If '*size' == ULONG_MAX, the fonction returns in '*size' the maximum size
 * of the payload for the buffer. The user next needs to manually call sctk_ib_prepare.
 */
sctk_ibuf_t*
sctk_ibuf_pick_send(struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t *remote,
    size_t *size, int n)
{
  LOAD_POOL(rail_ib);
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_t* ibuf = NULL;
  size_t s;

  s = *size;
  /***** RDMA CHANNEL *****/
  if (sctk_ibuf_rdma_is_remote_connected(remote)) {

    if (s == ULONG_MAX) s = config->ibv_eager_rdma_limit - IBUF_RDMA_GET_SIZE;

    if ( (IBUF_RDMA_GET_SIZE + s) <= config->ibv_eager_rdma_limit) {
      sctk_nodebug("requested:%lu max:%lu header:%lu", s, config->ibv_eager_rdma_limit, IBUF_RDMA_GET_SIZE);
      ibuf = sctk_ibuf_rdma_pick(rail_ib, remote);

      /* If we cannot pick a buffer from the RDMA channel, we switch to SR */
      if (ibuf) {
        sctk_nodebug("Picking from RDMA %d", ibuf->index);
        goto exit;
      }
    }
  }

  s = *size;
  /***** SR CHANNEL *****/
  if (s == ULONG_MAX) s = config->ibv_eager_limit;

  if (s <= config->ibv_eager_limit) {
    sctk_nodebug("Picking from SR");
#ifdef DEBUG_IB_BUFS
    if (n != -1)  {
      assume(n <= pool->nodes_nb);
    }
#endif

    if (n == -1) n = CLOSEST_NODE_FROM_NIC;
    sctk_ibuf_numa_t *node = &pool->nodes[n];
    sctk_spinlock_t *lock = &node->lock;

    if (OPA_load_int(&node->free_nb) < 100) {
      sctk_ib_low_mem_broadcast(rail_ib->rail);
    }

    sctk_spinlock_lock(lock);

    /* Allocate additionnal buffers if no more are available */
    if ( !node->free_entry) {
      init_node(rail_ib, node, config->ibv_size_ibufs_chunk);
    }

    /* update pointers from buffer pool */
    ibuf = node->free_entry;
    DL_DELETE(node->free_entry, node->free_entry);
    OPA_decr_int(&node->free_nb);

    sctk_spinlock_unlock(lock);

    IBUF_SET_PROTOCOL(ibuf->buffer, null_protocol);

#ifdef DEBUG_IB_BUFS
    assume(ibuf);
    if (ibuf->flag != FREE_FLAG)
    {sctk_error("Wrong flag (%d) got from ibuf", ibuf->flag);}
#endif

    sctk_nodebug("ibuf: %p, lock:%p, need_lock:%d next free_entryr: %p, nb_free %d, nb_got %d, nb_free_srq %d, node %d)", ibuf, lock, need_lock, node->free_entry, node->nb_free, node->nb_got, node->nb_free_srq, n);

  } else {
    sctk_error("The size associated to one eager buffer is too small to allocate a"
        "eager buffer. Please increase the value of XXXXX. (Requested=%lu max_rdma=%lu max_sr=%lu)", s,
        config->ibv_eager_rdma_limit - IBUF_RDMA_GET_SIZE, config->ibv_eager_limit);
    sctk_abort();
    return NULL;
  }
exit:

  /* We update the size if it is equal to ULONG_MAX*/
  if (*size == ULONG_MAX)  {
    *size = s;
  } else {
    /* Prepare the buffer for sending */
    sctk_ibuf_prepare(rail_ib, remote, ibuf, *size);
  }

  IBUF_SET_POISON(ibuf->buffer);

  return ibuf;
}


/* pick a new buffer from the ibuf list. Function *MUST* be locked to avoid
 * oncurrent calls.
 * - remote: process where picking buffer. It may be NULL. In this case,
 *   we pick a buffer from the SR channel */
static inline sctk_ibuf_t*
sctk_ibuf_pick_recv(struct sctk_ib_rail_info_s *rail_ib,
    int need_lock, int n)
{
  LOAD_POOL(rail_ib);
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_t* ibuf;
#ifdef DEBUG_IB_BUFS
  if (n != -1)  {
    assume(n <= pool->nodes_nb);
  }
#endif

  if (n == -1) n = CLOSEST_NODE_FROM_NIC;
  sctk_ibuf_numa_t *node = &pool->nodes[n];
  sctk_spinlock_t *lock = &node->lock;

  if (OPA_load_int(&node->free_nb) < 100) {
    sctk_ib_low_mem_broadcast(rail_ib->rail);
  }

  if (need_lock) sctk_spinlock_lock(lock);

  /* Allocate additionnal buffers if no more are available */
  if ( !node->free_entry) {
      init_node(rail_ib, node, config->ibv_size_ibufs_chunk);
  }

  /* update pointers from buffer pool */
  ibuf = node->free_entry;
  DL_DELETE(node->free_entry, node->free_entry);
  OPA_decr_int(&node->free_nb);

 if (need_lock) sctk_spinlock_unlock(lock);

#ifdef DEBUG_IB_BUFS
  assume(ibuf);
  if (ibuf->flag != FREE_FLAG)
  {sctk_error("Wrong flag (%d) got from ibuf", ibuf->flag);}
#endif

  sctk_nodebug("ibuf: %p, lock:%p, need_lock:%d next free_entryr: %p, nb_free %d, nb_got %d, nb_free_srq %d, node %d)", ibuf, lock, need_lock, node->free_entry, node->nb_free, node->nb_got, node->nb_free_srq, n);

  return ibuf;
}

/*
 * Post buffers to the SRQ.
 * FIXME: locks have been removed because useless.
 * Check if it aims to corruption.
 */
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

  for (i=0; i < nb_ibufs; ++i)
  {
    ibuf = sctk_ibuf_pick_recv(rail_ib, need_lock, node->id);
#ifdef DEBUG_IB_BUFS
    assume(ibuf);
#endif

    sctk_ibuf_recv_init(ibuf);

    rc = ibv_post_srq_recv(device->srq, &(ibuf->desc.wr.recv), &(ibuf->desc.bad_wr.recv));

    if (rc != 0)
    {
      ibuf->flag = FREE_FLAG;
      ibuf->in_srq = 0;

      /* change counters */
      OPA_incr_int(&node->free_nb);

      if(need_lock) sctk_spinlock_lock(lock);
      DL_PREPEND(node->free_entry, ibuf);
      if (need_lock) sctk_spinlock_unlock(lock);
      break;
    }
    else nb_posted++;
  }

  OPA_add_int(&node->free_srq_nb, nb_posted);

  return nb_posted;
}

/* FIXME: check with buffers near IB card */
int sctk_ibuf_srq_check_and_post(
    struct sctk_ib_rail_info_s *rail_ib,
    int limit)
{
  LOAD_POOL(rail_ib);
  int node = CLOSEST_NODE_FROM_NIC;

  return srq_post(rail_ib, limit, &pool->nodes[node], 1);
}

static inline void __release_in_srq(
    struct sctk_ib_rail_info_s *rail_ib,
    sctk_ibuf_t* ibuf, int need_lock)
{
  assume(ibuf);
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_numa_t *node = ibuf->region->node;
  int limit;
  int free_srq_nb;

  ibuf->in_srq = 0;
  /* limit of buffer posted */
  free_srq_nb = OPA_fetch_and_decr_int(&node->free_srq_nb) - 1;
  limit = config->ibv_max_srq_ibufs_posted - free_srq_nb;
  sctk_nodebug("Post new buffer %d (%d - %d)", limit, config->ibv_max_srq_ibufs_posted, free_srq_nb);
  if (limit > 0) {
    srq_post(rail_ib, limit, node, 1);
  }
  sctk_nodebug("Buffer %p free (%d)", ibuf, is_srq);
}

void sctk_ibuf_release_from_srq(
    struct sctk_ib_rail_info_s *rail_ib,
    sctk_ibuf_t* ibuf)
{
  __release_in_srq(rail_ib, ibuf, 1);
}

/* release one buffer given as parameter.
 * is_srq: if the buffer is released from the SRQ */
void sctk_ibuf_release(
    struct sctk_ib_rail_info_s *rail_ib,
    sctk_ibuf_t* ibuf)
{
  assume(ibuf);

  if (ibuf->to_release & IBUF_RELEASE) {
    if (IBUF_GET_CHANNEL(ibuf) & RDMA_CHANNEL) {
      sctk_ibuf_rdma_release(rail_ib, ibuf);
    } else {
      assume(IBUF_GET_CHANNEL(ibuf) == RC_SR_CHANNEL);
      sctk_ibuf_numa_t *node = ibuf->region->node;
      sctk_spinlock_t *lock = &node->lock;

      ibuf->flag = FREE_FLAG;
      IBUF_SET_PROTOCOL(ibuf->buffer, null_protocol);

      OPA_incr_int(&node->free_nb);
      sctk_spinlock_lock(lock);
      DL_APPEND(node->free_entry, ibuf);
      sctk_spinlock_unlock(lock);

      /* If SRQ, we check and try to post more messages to SRQ */
      if (ibuf->in_srq) {
        __release_in_srq(rail_ib, ibuf, 0);
      }
      sctk_nodebug("Buffer %p free (%d)", ibuf, ibuf->in_srq);
    }
  }
}

void sctk_ibuf_prepare(sctk_ib_rail_info_t* rail_ib, sctk_ib_qp_t *remote,
    sctk_ibuf_t* ibuf, size_t size) {
  LOAD_CONFIG(rail_ib);

  if (IBUF_GET_CHANNEL(ibuf) & RDMA_CHANNEL) {
    const sctk_ibuf_region_t* region = IBUF_RDMA_GET_REGION(remote, REGION_SEND);
    /* We do not emit an immediate data */
#if 0
    /* Initialization of the buffer */
    sctk_ibuf_rdma_write_with_imm_init(ibuf,
        IBUF_RDMA_GET_BASE_FLAG(ibuf), /* Local addr */
        region->mmu_entry->mr->lkey,  /* lkey */
        IBUF_RDMA_GET_REMOTE_ADDR(remote, REGION_RECV, ibuf),  /* Remote addr */
        remote->ibuf_rdma->rkey[REGION_RECV],  /* rkey */
        IBUF_RDMA_GET_SIZE + size + IBUF_GET_EAGER_SIZE, /* size */
        ibuf->index | IMM_DATA_EAGER_RDMA);  /* imm_data: index of the ibuf in the region */
#endif
    /* Initialization of the buffer */
    sctk_ibuf_rdma_write_init(ibuf,
        IBUF_RDMA_GET_BASE_FLAG(ibuf), /* Local addr */
        region->mmu_entry->mr->lkey,  /* lkey */
        IBUF_RDMA_GET_REMOTE_ADDR(remote, REGION_RECV, ibuf),  /* Remote addr */
        remote->ibuf_rdma->rkey[REGION_RECV],  /* rkey */
        size + IBUF_RDMA_GET_SIZE, /* size */
        IBV_SEND_SIGNALED, IBUF_DO_NOT_RELEASE);  /* imm_data: index of the ibuf in the region */

    /* Move tail flag */
    sctk_ib_rdma_set_tail_flag(ibuf, size);

  } else {
    assume (IBUF_GET_CHANNEL(ibuf) & RC_SR_CHANNEL);
    sctk_ibuf_send_inline_init(ibuf, size);
  }
}

/*-----------------------------------------------------------
 *  WR INITIALIZATION
 *----------------------------------------------------------*/
void sctk_ibuf_recv_init(sctk_ibuf_t* ibuf)
{
  LOAD_CONFIG(ibuf->region->rail);

  ibuf->to_release = IBUF_RELEASE;
  ibuf->in_srq = 1;

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
  ibuf->desc.sg_entry.length = config->ibv_eager_limit;

  ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) (ibuf->buffer);

  ibuf->flag = RECV_IBUF_FLAG;
}

void sctk_ibuf_barrier_send_init(sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len)
{

  ibuf->to_release = IBUF_RELEASE;
  ibuf->in_srq = 0;

  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = BARRIER_IBUF_FLAG;
}

int sctk_ibuf_send_inline_init(
    sctk_ibuf_t* ibuf, size_t size)
{
  LOAD_CONFIG(ibuf->region->rail);
  int is_inlined = 0;

  /* If data may be inlined */
  if(size <= config->ibv_max_inline) {
    ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE | IBV_SEND_SIGNALED;
    ibuf->flag = SEND_INLINE_IBUF_FLAG;
    is_inlined = 1;
  } else {
    ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
    ibuf->flag = SEND_IBUF_FLAG;
  }

  ibuf->in_srq = 0;
  ibuf->to_release = IBUF_RELEASE;
  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_SEND;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
  ibuf->desc.sg_entry.length = size;
  ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) (ibuf->buffer);

  return is_inlined;
}

void sctk_ibuf_send_init(
    sctk_ibuf_t* ibuf, size_t size)
{
  ibuf->in_srq = 0;
  ibuf->to_release = IBUF_RELEASE;
  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_SEND;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
  ibuf->desc.sg_entry.length = size;
  ibuf->desc.sg_entry.lkey = ibuf->region->mmu_entry->mr->lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) (ibuf->buffer);

  ibuf->flag = SEND_IBUF_FLAG;
}

int sctk_ibuf_rdma_write_init(
    sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len, int send_flags, char to_release)
{
  LOAD_CONFIG(ibuf->region->rail);
  int is_inlined = 0;

  /* If data may be inlined */
  if(len <= config->ibv_max_inline) {
    ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE | send_flags;
    ibuf->flag = RDMA_WRITE_INLINE_IBUF_FLAG;
    is_inlined = 1;
  } else {
    ibuf->desc.wr.send.send_flags = send_flags;
    ibuf->flag = RDMA_WRITE_IBUF_FLAG;
  }

  ibuf->in_srq = 0;
  ibuf->to_release = to_release;
  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  return is_inlined;
}

int sctk_ibuf_rdma_write_with_imm_init(
    sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len, uint32_t imm_data)
{
  LOAD_CONFIG(ibuf->region->rail);
  int is_inlined = 0;

  /* If data may be inlined */
  if(len <= config->ibv_max_inline) {
    ibuf->desc.wr.send.send_flags = IBV_SEND_INLINE;
    ibuf->flag = RDMA_WRITE_INLINE_IBUF_FLAG;
    is_inlined = 1;
  } else {
    ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
    ibuf->flag = RDMA_WRITE_IBUF_FLAG;
  }

  ibuf->in_srq = 0;

  ibuf->to_release = IBUF_RELEASE;
  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.wr.send.imm_data = imm_data;
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  return is_inlined;
}



void sctk_ibuf_rdma_read_init(
    sctk_ibuf_t* ibuf, void* local_address,
    uint32_t lkey, void* remote_address, uint32_t rkey,
    int len, void* supp_ptr, int dest_process)
{
  ibuf->in_srq = 0;
  ibuf->to_release = IBUF_RELEASE;
  ibuf->desc.wr.send.next = NULL;
  ibuf->desc.wr.send.opcode = IBV_WR_RDMA_READ;
  ibuf->desc.wr.send.send_flags = IBV_SEND_SIGNALED;
  ibuf->desc.wr.send.wr_id = (uintptr_t) ibuf;

  ibuf->desc.wr.send.num_sge = 1;
  ibuf->desc.wr.send.wr.rdma.remote_addr = (uintptr_t) remote_address;
  ibuf->desc.wr.send.wr.rdma.rkey = rkey;

  ibuf->desc.wr.send.sg_list = &(ibuf->desc.sg_entry);
  ibuf->desc.wr.send.imm_data = IMM_DATA_NULL;
  ibuf->desc.sg_entry.length = len;
  ibuf->desc.sg_entry.lkey = lkey;
  ibuf->desc.sg_entry.addr = (uintptr_t) local_address;

  ibuf->flag = RDMA_READ_IBUF_FLAG;
  ibuf->supp_ptr = supp_ptr;
  ibuf->dest_process = dest_process;
}

void sctk_ibuf_print_rdma(sctk_ibuf_t *ibuf, char* desc) {
  sprintf(desc,
      "region       :%p\n"
      "buffer       :%p\n"
      "size         :%lu\n"
      "flag         :%s\n"
      "remote       :%p\n"
      "dest_process :%d\n"
      "in_srq       :%d\n"
      "next         :%p\n"
      "prev         :%p\n"
      "sg_entry.length :%u\n"
      "local addr   :%p\n"
      "remote addr  :%p\n",
      ibuf->region,
      ibuf->buffer,
      ibuf->size,
      sctk_ibuf_print_flag(ibuf->flag),
      ibuf->remote,
      ibuf->dest_process,
      ibuf->in_srq,
      ibuf->next,
      ibuf->prev,
      ibuf->desc.sg_entry.length,
      ibuf->desc.sg_entry.addr,
      ibuf->desc.wr.send.wr.rdma.remote_addr);
}


void sctk_ibuf_print(sctk_ibuf_t *ibuf, char* desc) {
  sprintf(desc,
      "region       :%p\n"
      "buffer       :%p\n"
      "size         :%lu\n"
      "flag         :%s\n"
      "remote       :%p\n"
      "dest_process :%d\n"
      "in_srq       :%d\n"
      "next         :%p\n"
      "prev         :%p\n"
      "sg_entry.length :%u\n",
      ibuf->region,
      ibuf->buffer,
      ibuf->size,
      sctk_ibuf_print_flag(ibuf->flag),
      ibuf->remote,
      ibuf->dest_process,
      ibuf->in_srq,
      ibuf->next,
      ibuf->prev,
      ibuf->desc.sg_entry.length);
}
#endif
