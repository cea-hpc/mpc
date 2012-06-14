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

#include "sctk_ibufs_rdma.h"
#include "sctk_ibufs.h"

#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "IBUFR"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_asm.h"
#include "sctk_ib_prof.h"
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
 *  TODO: use an additional set of QP for RDMA where no buffers are
 *  posted in receive.
 */

/* FIXME: the following variables *SHOULD* be in the rail */
/* Linked list of rdma_poll where each remote using the RDMA eager protocol
 * is listed*/
sctk_ibuf_rdma_pool_t *rdma_pool_list = NULL;
/* Elements to merge to the rdma_pool_list */
sctk_ibuf_rdma_pool_t *rdma_pool_list_to_merge = NULL;
/* Lock when adding and accessing the concat list */
static sctk_spinlock_t rdma_pool_list_lock = SCTK_SPINLOCK_INITIALIZER;

static sctk_spinlock_t rdma_lock = SCTK_SPINLOCK_INITIALIZER;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/*
 * Init the remote for the RDMA connection
 */
void sctk_ibuf_rdma_remote_init(sctk_ib_qp_t* remote) {
  sctk_ibuf_rdma_set_remote_state_rtr(remote, state_deconnected);
  sctk_ibuf_rdma_set_remote_state_rts(remote, state_deconnected);
  remote->rdma.mean_size_lock = SCTK_SPINLOCK_INITIALIZER;
  remote->rdma.mean_size = 0;
  remote->rdma.mean_iter = 0;
  remote->rdma.max_pending_requests = 0;
  remote->rdma.lock = SCTK_SPINLOCK_INITIALIZER;
  remote->rdma.polling_lock = SCTK_SPINLOCK_INITIALIZER;
  /* Counters */
  OPA_store_int(&remote->rdma.miss_nb, 0);
  OPA_store_int(&remote->rdma.hits_nb, 0);
  /* Lock for resizing */
  remote->rdma.flushing_lock = SCTK_SPINLOCK_INITIALIZER;
}

static inline void __init_rdma_slots(sctk_ibuf_region_t *region,
    void* ptr, void* ibuf, int nb_ibufs, int size_ibufs) {
  int i;
  sctk_ibuf_t* ibuf_ptr = NULL;

  for(i=0; i < nb_ibufs; ++i) {
    const char *buffer = (char*) ((char*) ptr + (i*size_ibufs));
    ibuf_ptr = (sctk_ibuf_t*) ibuf + i;
    ibuf_ptr->region = region;
    ibuf_ptr->size = 0;
    ibuf_ptr->flag = FREE_FLAG;
    ibuf_ptr->index = i;
    ibuf_ptr->to_release = IBUF_RELEASE;

    /* Set flags  */
    ibuf_ptr->buffer = IBUF_RDMA_GET_PAYLOAD_FLAG(buffer);
    ibuf_ptr->head_flag = IBUF_RDMA_GET_HEAD_FLAG(buffer);
    ibuf_ptr->size_flag = IBUF_RDMA_GET_SIZE_FLAG(buffer);
    ibuf_ptr->poison_flag_head = IBUF_RDMA_GET_POISON_FLAG_HEAD(buffer);
    /* Reset piggyback */
    IBUF_GET_EAGER_PIGGYBACK(ibuf_ptr->buffer) = -1;
    /* Set headers value */
    *(ibuf_ptr->head_flag) = IBUF_RDMA_RESET_FLAG;
    /* Set poison value */
    *(ibuf_ptr->poison_flag_head) = IBUF_RDMA_POISON_HEAD;

    DL_APPEND(region->list, ((sctk_ibuf_t*) ibuf + i));
    sctk_nodebug("ibuf=%p, index=%d, buffer=%p, head_flag=%p, size_flag=%p",
        ibuf_ptr, i, ibuf_ptr->buffer, ibuf_ptr->head_flag, ibuf_ptr->size_flag);
  }
}

/*
 * Reinitialize a RDMA region with a different number of ibufs and a different size of ibufs
 */
void
sctk_ibuf_rdma_region_reinit(struct sctk_ib_rail_info_s *rail_ib,sctk_ib_qp_t* remote,
    sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel, int nb_ibufs, int size_ibufs) {
  void* ptr = NULL;
  void* ibuf;
  sctk_ibuf_t* ibuf_ptr = NULL;
  int i;

  ib_assume(remote->rdma.pool);
  /* Because we are reinitializing a RDMA region, be sure that a previous allocation
   * has been made */
  assume(region->buffer_addr);

  /* Check if all ibufs are really free */
#warning "Only for debug"
  int busy;
  if (channel == (RDMA_CHANNEL | SEND_CHANNEL) ) { /* SEND CHANNEL */
    if ( (busy = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND])) != 0) {
      sctk_error("Buffer with wrong number in SEND channel (busy_nb:%d)", busy);
      not_reachable();
    }
  } else if (channel == (RDMA_CHANNEL | RECV_CHANNEL) ) { /* RECV CHANNEL */
    if ( (busy = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_RECV])) != 0) {
      sctk_error("Buffer with wrong numer in RECV channel (busy_nb:%d)", busy);
      not_reachable();
    }
  }

  for(i=0; i < region->nb ; ++i) {
    ibuf_ptr = (sctk_ibuf_t*) region->ibuf + i;
    if (ibuf_ptr->flag != FREE_FLAG) {
      if (channel == (RDMA_CHANNEL | SEND_CHANNEL) ) { /* SEND CHANNEL */
        sctk_error("Buffer with wrong flag: %d in SEND channel (busy_nb:%d)", ibuf_ptr->flag,
           OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]));
      } else if (channel == (RDMA_CHANNEL | RECV_CHANNEL) ) { /* RECV CHANNEL */
        sctk_error("Buffer with wrong flag: %d in RECV channel (busy_nb:%d)", ibuf_ptr->flag,
           OPA_load_int(&remote->rdma.pool->busy_nb[REGION_RECV]));
      }
      not_reachable();
    }
  }

  /* Unregister the memory.
   * FIXME: ideally, we should use ibv_reregister() but this call is currently not
   * supported by the libverb. */
  sctk_ib_mmu_unregister(rail_ib, region->mmu_entry);

  /* Realloc a new memory region.
   * FIXME: Using realloc is a little bit useless here because we get an additional
   * memory copy not used in this case */
  assume(region->buffer_addr);
  ptr = realloc(region->buffer_addr, nb_ibufs * size_ibufs);
  ib_assume(ptr);
  /* FIXME: is the memset here really usefull? */
  memset(ptr, 0, nb_ibufs * size_ibufs);

  assume(region->ibuf);
  ibuf = realloc(region->ibuf, nb_ibufs * sizeof(sctk_ibuf_t));
  ib_assume(ibuf);
  /* FIXME: is the memset here really usefull? */
  memset (ibuf, 0, nb_ibufs * sizeof(sctk_ibuf_t));

  region->size_ibufs = size_ibufs;
  region->buffer_addr = ptr;
  region->nb = nb_ibufs;
  region->rail = rail_ib;
  region->list = NULL;
  region->channel = channel;
  region->ibuf = ibuf;
  region->remote = remote;
  region->lock = SCTK_SPINLOCK_INITIALIZER;

  sctk_nodebug("Channel: %d", region->channel);

  /* register buffers at once
   * FIXME: ideally, we should use ibv_reregister() but this call is currently not
   * supported by the libverb. */
  region->mmu_entry = sctk_ib_mmu_register_no_cache(rail_ib, ptr,
      nb_ibufs * size_ibufs);
  sctk_nodebug("[%d] Reg %p registered for rank %d (channel:%d). lkey : %lu", rail_ib->rail->rail_number, ptr, remote->rank, channel, region->mmu_entry->mr->lkey);

  /* Init all RDMA slots */
  __init_rdma_slots(region, ptr, ibuf, nb_ibufs, size_ibufs);

  /* Setup tail and head pointers */
  region->head = ibuf;
  region->tail = ibuf;

  if (channel == (RDMA_CHANNEL | SEND_CHANNEL) ) { /* SEND CHANNEL */
    /* Send the number of credits */
    remote->rdma.pool->send_credit = nb_ibufs;
    OPA_store_int(&remote->rdma.pool->busy_nb[REGION_SEND], 0);

    sctk_ib_debug("RESIZING SEND channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d (send_credit:%d)",
        remote->rank, nb_ibufs, size_ibufs, remote->rdma.pool->send_credit);

  } else if (channel == (RDMA_CHANNEL | RECV_CHANNEL) ) { /* RECV CHANNEL */
    sctk_ib_debug("RESIZING RECV channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d)",
        remote->rank, nb_ibufs, size_ibufs);

    /* Add the entry to the pooling list */
    sctk_spinlock_lock(&rdma_pool_list_lock);
    DL_APPEND(rdma_pool_list_to_merge, remote->rdma.pool);
    sctk_spinlock_unlock(&rdma_pool_list_lock);
    OPA_store_int(&remote->rdma.pool->busy_nb[REGION_RECV], 0);

  } else not_reachable();

  sctk_nodebug("Head=%p(%d) Tail=%p(%d)", region->head, region->head->index,
      region->tail, region->tail->index);
}


/*
 * Initialize a new RDMA buffer region
 */
void
sctk_ibuf_rdma_region_init(struct sctk_ib_rail_info_s *rail_ib,sctk_ib_qp_t* remote,
    sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel, int nb_ibufs, int size_ibufs) {
  LOAD_MMU(rail_ib);
  void* ptr = NULL;
  void* ibuf;

  ib_assume(remote->rdma.pool);

  /* XXX: replace by memalign_on_node */
  sctk_posix_memalign( (void**) &ptr, mmu->page_size, nb_ibufs * size_ibufs);
  ib_assume(ptr);
  memset(ptr, 0, nb_ibufs * size_ibufs);

  /* XXX: replace by memalign_on_node */
   sctk_posix_memalign(&ibuf, mmu->page_size, nb_ibufs * sizeof(sctk_ibuf_t));
  ib_assume(ibuf);
  memset (ibuf, 0, nb_ibufs * sizeof(sctk_ibuf_t));

  region->size_ibufs = size_ibufs;
  region->buffer_addr = ptr;
  region->nb = nb_ibufs;
  region->rail = rail_ib;
  region->list = NULL;
  region->channel = channel;
  region->ibuf = ibuf;
  region->remote = remote;
  region->lock = SCTK_SPINLOCK_INITIALIZER;

  sctk_nodebug("Channel: %d", region->channel);

  /* register buffers at once */
  region->mmu_entry = sctk_ib_mmu_register_no_cache(rail_ib, ptr,
      nb_ibufs * size_ibufs);
  sctk_nodebug("Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey);

  /* Init all RDMA slots */
  __init_rdma_slots(region, ptr, ibuf, nb_ibufs, size_ibufs);

  /* Setup tail and head pointers */
  region->head = ibuf;
  region->tail = ibuf;

  if (channel == (RDMA_CHANNEL | SEND_CHANNEL) ) { /* SEND CHANNEL */
    /* Send the number of credits */
    remote->rdma.pool->send_credit = nb_ibufs;
    OPA_store_int(&remote->rdma.pool->busy_nb[REGION_SEND], 0);

    sctk_ib_debug("SEND channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d (send_credit:%d)",
        remote->rank, nb_ibufs, size_ibufs, remote->rdma.pool->send_credit);

  } else if (channel == (RDMA_CHANNEL | RECV_CHANNEL) ) { /* RECV CHANNEL */
    sctk_ib_debug("RECV channel initialized for remote %d (nb_ibufs=%d, size_ibufs=%d",
        remote->rank, nb_ibufs, size_ibufs);

    /* Add the entry to the pooling list */
    sctk_spinlock_lock(&rdma_pool_list_lock);
    DL_APPEND(rdma_pool_list_to_merge, remote->rdma.pool);
    sctk_spinlock_unlock(&rdma_pool_list_lock);
    OPA_store_int(&remote->rdma.pool->busy_nb[REGION_RECV], 0);

  } else not_reachable();

  sctk_nodebug("Head=%p(%d) Tail=%p(%d)", region->head, region->head->index,
      region->tail, region->tail->index);
}

/*
 * Create a region of ibufs for RDMA
 */
sctk_ibuf_rdma_pool_t*
sctk_ibuf_rdma_pool_init(struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t* remote) {
  sctk_ibuf_rdma_pool_t* pool = NULL;

  /* We lock during the memmory allocating */
  sctk_spinlock_lock(&remote->rdma.lock);
  /* If no allocation has been done */
  if (remote->rdma.pool == NULL) {
    pool = sctk_malloc(sizeof(sctk_ibuf_rdma_pool_t));
    ib_assume(pool);
    memset(pool, 0, sizeof(sctk_ibuf_rdma_pool_t));

    /* Setup local addr */
    pool->remote_region[REGION_SEND] = NULL;
    pool->remote_region[REGION_RECV] = NULL;
    pool->remote = remote;

    /* Finaly update the pool pointer */
    remote->rdma.pool = pool;
  }
  sctk_spinlock_unlock(&remote->rdma.lock);

  /* Initialize regions for send and recv */
  /* sctk_ibuf_rdma_region_init(rail_ib, remote, &pool->region[REGION_SEND],
      RDMA_CHANNEL | SEND_CHANNEL, send_nb_ibufs, send_size_ibufs);
  sctk_ibuf_rdma_region_init(rail_ib, remote, &pool->region[REGION_RECV],
      RDMA_CHANNEL | RECV_CHANNEL, recv_nb_ibufs, recv_size_ibufs); */

  /* sctk_ib_debug("Allocating (|%p|%d:%d-|%p|%d:%d) RDMA buffers (connections: %d)",
      pool->region[REGION_SEND].buffer_addr,
      send_nb_ibufs, send_size_ibufs,
      pool->region[REGION_RECV].buffer_addr,
      recv_nb_ibufs, recv_size_ibufs,
      device->eager_rdma_connections); */


  return pool;
}

/*
 * Delete all buffers
 */
static inline void
sctk_ibuf_rdma_region_free(struct sctk_ib_rail_info_s *rail_ib,
    sctk_ibuf_region_t *region) {

  ib_assume(region->buffer_addr);
  ib_assume(region->ibuf);

  free(region->buffer_addr);
  free(region->ibuf);

  region->buffer_addr = NULL;
  region->ibuf = NULL;
}


/*
 * Delete a RDMA poll from a remote
 */
void
sctk_ibuf_rdma_pool_free(struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t* remote) {
/* Needs to be re-written */
  not_implemented();
#if 0
  LOAD_DEVICE(rail_ib);
  /* Check if we can allocate an RDMA channel */
  sctk_spinlock_lock(&rdma_lock);
  if (sctk_ibuf_rdma_is_remote_connected(remote)) {
    sctk_ibuf_rdma_pool_t* pool = remote->rdma.pool;
    --device->eager_rdma_connections;
    sctk_spinlock_unlock(&rdma_lock);

    sctk_ib_debug("Freeing RDMA buffers (connections: %d)", device->eager_rdma_connections);

    /* Free regions for send and recv */
    sctk_ibuf_rdma_region_free(rail_ib, &pool->region[REGION_SEND]);
    sctk_ibuf_rdma_region_free(rail_ib, &pool->region[REGION_RECV]);

    sctk_spinlock_lock(&rdma_pool_list_lock);
    DL_DELETE(rdma_pool_list_to_merge, pool);
    sctk_spinlock_unlock(&rdma_pool_list_lock);

    /* Free pool */
    free(remote->rdma.pool);
    remote->rdma.pool = NULL;

    sctk_ib_debug("[rank:%d] Free RDMA buffers", remote->rank);
  } else {
    sctk_spinlock_unlock(&rdma_lock);
  }
#endif
}


/*
 * Pick a RDMA buffer from the RDMA channel
 */
inline sctk_ibuf_t *sctk_ibuf_rdma_pick(sctk_ib_rail_info_t* rail_ib, sctk_ib_qp_t* remote) {
  sctk_ibuf_t *tail;
  int piggyback;

  IBUF_RDMA_LOCK_REGION(remote, REGION_SEND);

  tail = IBUF_RDMA_GET_TAIL(remote, REGION_SEND);
  piggyback =  IBUF_GET_EAGER_PIGGYBACK(tail->buffer);

  sctk_nodebug("Piggy back field %d %p", piggyback, &IBUF_GET_EAGER_PIGGYBACK(tail->buffer));

  if (piggyback > 0) {
    int i;
    /* Reinit the piggyback of each buffer */
    sctk_ibuf_t *ptr = tail;
    /* For each slot freed, we reset the piggyback filed */
    for (i = 0; i < piggyback; ++i) {
      IBUF_GET_EAGER_PIGGYBACK(ptr->buffer) = -1;
      ptr = IBUF_RDMA_GET_NEXT(ptr);
    }
    /* Upgrade the send credit and move the tail flag */
    remote->rdma.pool->send_credit += piggyback;
    IBUF_RDMA_GET_TAIL(remote, REGION_SEND) = IBUF_RDMA_ADD(tail, piggyback);

    sctk_nodebug("Got pg %p %d (prev:%p next:%p)", piggyback, *piggyback, tail, IBUF_RDMA_GET_TAIL(remote, REGION_SEND));
  }

  /* If a buffer is available */
  if (remote->rdma.pool->send_credit > 0) {
    sctk_ibuf_t *head;

    head = IBUF_RDMA_GET_HEAD(remote, REGION_SEND);

    /* If the buffer is not free for the moment, we skip the
     * RDMA */
    if (head->flag != FREE_FLAG) {
      IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);
      return NULL;
    }

    /* Update the credit */
    remote->rdma.pool->send_credit--;
    /* Move the head flag */
    IBUF_RDMA_GET_HEAD(remote, REGION_SEND) = IBUF_RDMA_GET_NEXT(head);
    IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);

    sctk_nodebug("Picked RDMA buffer %p, buffer=%p pb=%p index=%d credit=%d size:%d", head,
        head->buffer, &IBUF_GET_EAGER_PIGGYBACK(head->buffer), head->index, remote->rdma.pool->send_credit,
        remote->rdma.pool->region[REGION_SEND].size_ibufs);

    head->flag = BUSY_FLAG;

    return (sctk_ibuf_t*) head;
  }
  else {
    sctk_nodebug("Cannot pick RDMA buffer");
    IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);
  }

  return NULL;
}

/*
 * Reset the head flag after a message has been successfully received
 */
void sctk_ib_rdma_reset_head_flag(sctk_ibuf_t* ibuf) {
  *(ibuf->head_flag) = IBUF_RDMA_RESET_FLAG;
}

/*
 * Set the tail flag to the ibuf
 */
void sctk_ib_rdma_set_tail_flag(sctk_ibuf_t* ibuf, size_t size) {
  /* Head, payload and Tail */
  int* ibuf_tail;

  ibuf_tail = IBUF_RDMA_GET_TAIL_FLAG(ibuf->buffer, size);

  *(ibuf->size_flag) = size;

  if (*ibuf_tail == IBUF_RDMA_FLAG_1) {
    *ibuf_tail = IBUF_RDMA_FLAG_2;
    *(ibuf->head_flag) = IBUF_RDMA_FLAG_2;
  } else {
    *ibuf_tail =IBUF_RDMA_FLAG_1;
    *(ibuf->head_flag) = IBUF_RDMA_FLAG_1;
  }
}

/*
 * Handle a message received
 */
static void __poll_ibuf(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote,
    sctk_ibuf_t* ibuf) {
  sctk_rail_info_t* rail = rail_ib->rail;
  ib_assume(ibuf);
  int release_ibuf = 1;

  /* Set the buffer as releasable. Actually, we need to do this reset
   * here.. */
  ibuf->to_release = IBUF_RELEASE;

  /* const size_t size_flag = *ibuf->size_flag; */
  /* int *tail_flag = IBUF_RDMA_GET_TAIL_FLAG(ibuf->buffer, size_flag); */
  sctk_nodebug("Buffer size:%d, head flag:%d, tail flag:%d", *ibuf->size_flag, *ibuf->head_flag, *tail_flag);

  const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL(ibuf->buffer);
  switch (protocol) {
    case eager_protocol:
      sctk_ib_eager_poll_recv(rail, ibuf);
      release_ibuf = 0;
      break;

    case rdma_protocol:
      sctk_nodebug("RDMA received from RDMA channel");
      release_ibuf = sctk_ib_rdma_poll_recv(rail, ibuf);
      break;

    case buffered_protocol:
      sctk_ib_buffered_poll_recv(rail, ibuf);
      release_ibuf = 1;
      break;

    default:
      not_reachable();
  }

  if (release_ibuf) {
    sctk_ibuf_release(rail_ib, ibuf);
  }
}


/*
 * Walk on each remotes
 */
void sctk_ib_rdma_eager_walk_remotes(sctk_ib_rail_info_t *rail, int (func)(sctk_ib_rail_info_t *rail, sctk_ib_qp_t* remote), int *ret) {
  sctk_ibuf_rdma_pool_t *pool;
  *ret = 0;
  static sctk_spin_rwlock_t rw_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

  sctk_spinlock_read_lock(&rw_lock);
  DL_FOREACH(rdma_pool_list, pool) {
    /* 'func' needs to check if the remote is in a RTR mode */
    *ret |= func(rail, pool->remote);
  }
  sctk_spinlock_read_unlock(&rw_lock);

  /* Check if they are remotes to merge. We need to merge here because
   * we cannot do this in the DL_FOREACH just before. It aims
   * to deadlocks. */
  if (rdma_pool_list_to_merge != NULL) {
    sctk_spinlock_write_lock(&rw_lock);
    /* We only add entries of connected processes */
    sctk_spinlock_lock(&rdma_pool_list_lock);
    DL_FOREACH(rdma_pool_list_to_merge, pool) {
      if (sctk_ibuf_rdma_get_remote_state_rtr(pool->remote) == state_connected) {
        sctk_nodebug("List merged for pooling !!");
        /* Remove the entry from the merge list... */
        DL_DELETE(rdma_pool_list_to_merge, pool);
        /* ... add it to the global list */
        DL_APPEND(rdma_pool_list, pool);
      }
    }
    sctk_spinlock_unlock(&rdma_pool_list_lock);
    sctk_spinlock_write_unlock(&rw_lock);
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
sctk_ib_rdma_eager_poll_remote(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote) {
  /* We return if the remote is not connected to the RDMA channel */
  sctk_route_state_t state = sctk_ibuf_rdma_get_remote_state_rtr(remote);
  if (state != state_connected) return -1;
  sctk_ibuf_t *head;

  head = IBUF_RDMA_GET_HEAD(remote, REGION_RECV);

  if (*(head->head_flag) != IBUF_RDMA_RESET_FLAG) {

    /* Spinlock for preventing two concurrent calls to this function */
    if (IBUF_RDMA_TRYLOCK_REGION(remote, REGION_RECV) == 0) {

      /* Read the state of the connection and return if necessary.
       * If we do not this, the RDMA buffer may be read while the RDMA buffer
       * is in a 'flushed' state */
      state = sctk_ibuf_rdma_get_remote_state_rtr(remote);
      if (state != state_connected) {
        IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
        return -1;
      }

      /* Recaculate the head because it could be moved */
      head = IBUF_RDMA_GET_HEAD(remote, REGION_RECV);
      /* Double checking */
      if (*(head->head_flag) != IBUF_RDMA_RESET_FLAG) {

        sctk_nodebug("Head set: %d-%d", *(head->head_flag), IBUF_RDMA_RESET_FLAG);

        const size_t size_flag = *head->size_flag;
        volatile int volatile * tail_flag = IBUF_RDMA_GET_TAIL_FLAG(head->buffer, size_flag);

        sctk_nodebug("Head set: %d-%d", *(head->head_flag), IBUF_RDMA_RESET_FLAG);

        /* The head has been set. We spin while waiting the tail to be set.
         * We do not release the lock because it is not necessary that a task
         * concurrently polls the same remote.  */
        while (*(head->head_flag) != *(tail_flag)) { /* poll */ };
        /* Reset the head flag */
        sctk_ib_rdma_reset_head_flag(head);
        /* Move head flag */
        IBUF_RDMA_GET_HEAD(remote, REGION_RECV) =  IBUF_RDMA_GET_NEXT(head);
        /* Increase the number of buffers busy */
        OPA_incr_int(&remote->rdma.pool->busy_nb[REGION_RECV]);
        IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
        IBUF_RDMA_CHECK_POISON_FLAG(head);

        sctk_nodebug("Buffer size:%d, ibuf:%p, head flag:%d, tail flag:%d %p new_head:%p (%d-%d)", *head->size_flag, head, *head->head_flag, *tail_flag, head->buffer, IBUF_RDMA_GET_HEAD(remote, REGION_RECV)->buffer, *(head->head_flag), *(tail_flag));

        if (head->flag != FREE_FLAG) {
          sctk_error ("Got a wrong flag, it seems there is a problem with MPC: %d", head->flag);
          not_reachable();
        }
        /* Set the slot as busy */
        head->flag = BUSY_FLAG;

        /* Handle the ibuf */
        __poll_ibuf(rail_ib, remote, head);
        sctk_nodebug("End of RDMA handler");
        /* We retry to poll a message.
         * FIXME: only retry if the sequence number of the message is not
         * expected */
        return 1;
      } else {
        IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
        return 0;
      }
    } else {
      return -1;
    }
  }
  return 0;
}

/*
 * Release a buffer from the RDMA channel
 */
void sctk_ibuf_rdma_release(sctk_ib_rail_info_t* rail_ib, sctk_ibuf_t* ibuf) {
  sctk_ib_qp_t * remote = ibuf->region->remote;

  if (IBUF_GET_CHANNEL(ibuf) & RECV_CHANNEL) {
    int piggyback = 0;

    sctk_nodebug("Freeing a RECV RDMA buffer (channel:%d head:%p - ibuf:%p - tail:%p)", IBUF_GET_CHANNEL(ibuf),
      ibuf->region->head, ibuf, ibuf->region->tail);

    /* Firstly lock the region */
    IBUF_RDMA_LOCK_REGION(remote, REGION_RECV);

    /* Get the tail */
    sctk_ibuf_t *tail = IBUF_RDMA_GET_TAIL(remote, REGION_RECV);
    /* Buffer which will be piggybacked */
    sctk_ibuf_t *base = ibuf;

    /* Mark the buffer as polled */
    if (ibuf->flag != BUSY_FLAG) {
      sctk_error ("Got a wrong flag, it seems there is a problem with MPC: %d", ibuf->flag);
      not_reachable();
    }
    ibuf->flag = EAGER_RDMA_POLLED;
    sctk_nodebug("Set %p as RDMA_POLLED", ibuf);

    /* Check if the buffer is located at the tail adress */
    sctk_nodebug("Flag=%d tail=(%p-%p-%d) next_tail=(%p-%p-%d)",ibuf->flag,
        tail, tail->buffer, tail->index,
        IBUF_RDMA_GET_NEXT(tail),
        IBUF_RDMA_GET_NEXT(tail)->buffer,
        IBUF_RDMA_GET_NEXT(tail)->index);

    /* While a buffer free is found, we increase the piggy back and
     * we reset each buffer */
    if (ibuf == tail) {
      while (ibuf->flag == EAGER_RDMA_POLLED) {
        piggyback ++;
        /* Put the buffer as free */
        ibuf->flag = FREE_FLAG;
        /* Move to next buffer */
        ibuf = IBUF_RDMA_GET_NEXT(ibuf);
      }
    }

    /* Piggyback the ibuf to the tail addr */
    if (piggyback > 0) {
      ib_assume(base == tail);
      ib_assume(base->flag == FREE_FLAG);
      /* Move tail */
      IBUF_RDMA_GET_TAIL(remote, REGION_RECV) = IBUF_RDMA_ADD(tail, piggyback);
      /* Unlock the RDMA region */
      IBUF_GET_EAGER_PIGGYBACK(base->buffer) = piggyback;
      IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);


      sctk_nodebug("Piggy back to %p pb:%d size:%d", IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote, base), piggyback, remote->rdma.pool->region[REGION_RECV].size_ibufs);

      /* Prepare the piggyback msg. The sent message is SIGNALED */
#if 0
       sctk_ibuf_rdma_write_init(base,
          &IBUF_GET_EAGER_PIGGYBACK(base->buffer), /* Local addr */
          base->region->mmu_entry->mr->lkey,  /* lkey */
          IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote, base),  /* Remote addr */
          remote->rdma.pool->rkey[REGION_SEND],  /* rkey */
          sizeof(int), 0,
          IBUF_DO_NOT_RELEASE);
#endif
       sctk_ibuf_rdma_write_with_imm_init(base,
          &IBUF_GET_EAGER_PIGGYBACK(base->buffer), /* Local addr */
          base->region->mmu_entry->mr->lkey,  /* lkey */
          IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote, base),  /* Remote addr */
          remote->rdma.pool->rkey[REGION_SEND],  /* rkey */
          sizeof(int), IMM_DATA_RDMA_PIGGYBACK);


      /* Put the buffer as free */
      base->flag = FREE_FLAG;

      /* Send the piggyback  */
#warning "Is a control message"
      sctk_ib_qp_send_ibuf(rail_ib, remote, base, 1);
      sctk_ib_qp_decr_requests_nb(remote);

      /* Check if we are in a flush state */
      OPA_decr_int(&remote->rdma.pool->busy_nb[REGION_RECV]);
      sctk_ibuf_rdma_check_flush_recv(rail_ib, remote);

    } else {
      /* Unlock the RDMA region */
      IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
      /* Increase the number of buffers busy */
      OPA_decr_int(&remote->rdma.pool->busy_nb[REGION_RECV]);
      sctk_ibuf_rdma_check_flush_recv(rail_ib, remote);
    }

  } else if (IBUF_GET_CHANNEL(ibuf) & SEND_CHANNEL) {
    /* We set the buffer as free */
    ibuf->flag = FREE_FLAG;
    OPA_decr_int(&remote->rdma.pool->busy_nb[REGION_SEND]);

    /* Check if we are in a flush state */
    sctk_ibuf_rdma_check_flush_send(rail_ib, remote);
  } else not_reachable();
}


/*
 * Check if the need to connect the remote using RDMA.
 */
#define IBUF_RDMA_SAMPLES 1000
#define IBUF_RDMA_MIN_SIZE (16 * 1024)
#define IBUF_RDMA_MAX_SIZE (16 * 1024)
#define IBUF_RDMA_MIN_NB (12)
#define IBUF_RDMA_MAX_NB (12)
/* Maximum number of miss before resizing RDMA */
#define IBUF_RDMA_MAX_MISS 100


/*
 * Check if a remote can be connected using RDMA.
 * The number of connections is automatically increased
 */
int sctk_ibuf_rdma_is_connectable(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote) {
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);
  int ret = 0;

  sctk_spinlock_lock(&rdma_lock);
  if (config->ibv_max_rdma_connections >= (device->eager_rdma_connections + 1)) {
    ret = ++device->eager_rdma_connections;
  }
  sctk_spinlock_unlock(&rdma_lock);

  return ret;
}

void sctk_ibuf_rdma_connection_cancel(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote) {
  LOAD_DEVICE(rail_ib);

  sctk_spinlock_lock(&rdma_lock);
  --device->eager_rdma_connections;
  sctk_spinlock_unlock(&rdma_lock);

  /* We reset the state to deconnected */
  sctk_ibuf_rdma_set_remote_state_rts(remote, state_deconnected);
}


/*
 * Update the maximum number of pending requests.
 * Should be called just before calling incr_requests_nb
 */
void sctk_ibuf_rdma_update_max_pending_requests(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote)
{
  int current_pending;

  sctk_spinlock_lock(&remote->rdma.mean_size_lock);
  current_pending = sctk_ib_qp_get_requests_nb(remote);
  if (current_pending > remote->rdma.max_pending_requests) {
    remote->rdma.max_pending_requests = current_pending;
  }
  sctk_spinlock_unlock(&remote->rdma.mean_size_lock);
}

void sctk_ibuf_rdma_check_remote(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote, size_t size) {
  /* We profile only when the RDMA route is deconnected */
  if (sctk_ibuf_rdma_get_remote_state_rts(remote) == state_deconnected) {
    float mean = (float) (size / IBUF_RDMA_SAMPLES);
    float total;
    int iter;
    int determined_size;
    int determined_nb;

    /* FIXME: This locks are really costly. Is there any tip for
     * using atomics with float ? */
    {
      sctk_spinlock_lock(&remote->rdma.mean_size_lock);
      determined_nb = remote->rdma.max_pending_requests;
      total = (remote->rdma.mean_size += mean);
      iter  = (++remote->rdma.mean_iter);
      sctk_spinlock_unlock(&remote->rdma.mean_size_lock);
    }

    /* Check if we need to connect using RDMA */
    if ( iter >= IBUF_RDMA_SAMPLES ) {
      /* Reajust the size according to the statistics */
      determined_size = (int) total;
      if (determined_size > IBUF_RDMA_MAX_SIZE) determined_size = IBUF_RDMA_MAX_SIZE;
      if (determined_size < IBUF_RDMA_MIN_SIZE) determined_size = IBUF_RDMA_MIN_SIZE;
      if (determined_nb > IBUF_RDMA_MAX_NB) determined_nb = IBUF_RDMA_MAX_NB;
      if (determined_nb < IBUF_RDMA_MIN_NB) determined_nb = IBUF_RDMA_MIN_NB;
      determined_size = ALIGN_ON_64 (determined_size + IBUF_GET_EAGER_SIZE + IBUF_RDMA_GET_SIZE);

      sctk_nodebug("Trying to connect remote %d using RDMA (mean size: %f, determined: %d, pending: %d, iter: %d)", remote->rank, total, determined_size,
          determined_nb, iter);

      /* Sending the request. If the request has been sent, we reinit */
      if (sctk_ib_cm_on_demand_rdma_request(rail_ib->rail, remote,
            determined_size, determined_nb) == 1) {
        /* Reset counters */
        sctk_spinlock_lock(&remote->rdma.mean_size_lock);
        remote->rdma.mean_size = 0;
        remote->rdma.mean_iter = 0;
        sctk_spinlock_unlock(&remote->rdma.mean_size_lock);
      }
    }
  } else if (sctk_ibuf_rdma_get_remote_state_rts(remote) == state_connected) {

    /* Check if we need the resize the RDMA */
    if( OPA_load_int(&remote->rdma.miss_nb) > IBUF_RDMA_MAX_MISS) {
      sctk_debug("MAX MISS REACHED");
      /* Try to change the state to flushing.
       * By changing the state of the remote to 'flushing', we automaticaly switch to SR */
      sctk_spinlock_lock(&remote->rdma.flushing_lock);
      sctk_route_state_t ret =
        sctk_ibuf_rdma_cas_remote_state_rts(remote, state_connected, state_flushing);
      sctk_spinlock_unlock(&remote->rdma.flushing_lock);
      /* If we are allowed to resize */
      if( (ret != state_connected) && (ret != state_flushing)) {
        sctk_error("Got a wrong state: %d", ret);
//        not_implemented();
      }

      if (ret == state_connected) {
        /* Reset the counter */
        OPA_store_int(&remote->rdma.miss_nb, 0);
        sctk_ib_debug("SEND Trying to flush RDMA for for remote %d", remote->rank);
        sctk_ibuf_rdma_check_flush_send(rail_ib, remote);
      }
    }
  }
}

/*
 * Fill RDMA connection keys with the configuration of a send and receive region
 */
void
sctk_ibuf_rdma_fill_remote_addr(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t* remote,
    sctk_ib_cm_rdma_connection_t *keys, int region) {

  ib_assume(remote->rdma.pool);
  keys->addr = remote->rdma.pool->region[region].buffer_addr;
  keys->rkey = remote->rdma.pool->region[region].mmu_entry->mr->rkey;

  sctk_nodebug("Filled keys addr=%p rkey=%u",
      keys->addr, keys->rkey, keys->connected);
}

/*
 * Update a remote from the keys received by a peer. This function must be
 * called before sending any message to the RDMA channel
 */
void
sctk_ibuf_rdma_update_remote_addr(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t* remote, sctk_ib_cm_rdma_connection_t *key, int region) {
  sctk_nodebug("REMOTE addr updated:: addr=%p rkey=%u (connected: %d)",
      key->addr, key->rkey, key->connected);

  ib_assume(remote->rdma.pool);
  /* Update keys for send and recv regions. We need to register the send region
   * because of the piggyback */
  remote->rdma.pool->remote_region[region] = key->addr;
  remote->rdma.pool->rkey[region] = key->rkey;
}

/*
 * Return the RDMA eager ibufs limit for
 * sending a message
 */
size_t sctk_ibuf_rdma_get_eager_limit(sctk_ib_qp_t *remote) {
  return (remote->rdma.pool->region[REGION_SEND].size_ibufs);
}


/*-----------------------------------------------------------
 *                          FLUSH
 *----------------------------------------------------------*/

/*
 * Check if a remote is in a flushing state. If it is, it
 * sends a flush REQ to the process
 */
int sctk_ibuf_rdma_check_flush_send(sctk_ib_rail_info_t* rail_ib, sctk_ib_qp_t *remote) {
  /* Check if the RDMA is in flushing mode and if all messages
   * have be flushed to the network */
  sctk_route_state_t ret = sctk_ibuf_rdma_get_remote_state_rts(remote);

  /* If we are in a flushing mode */
  if (ret == state_flushing) {
    int busy_nb = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]);
    if ( busy_nb == 0) {
      ret = sctk_ibuf_rdma_cas_remote_state_rts(remote, state_flushing, state_flushed);
      if (ret == state_flushing) {
        sctk_ib_debug("SEND DONE Trying to flush RDMA for for remote %d", remote->rank);
        /* All the requests have been flushed. We can now send the resizing request.
         * NOTE: we are sure that only one thread call the resizing request */
        sctk_ib_nodebug("Sending a FLUSH message to remote %d", remote->rank);

        /* FIXME: for now, we multiply the number of buffers by 2 */
        sctk_ib_cm_resizing_rdma_request(rail_ib->rail, remote,
            remote->rdma.pool->region[REGION_SEND].size_ibufs,
            remote->rdma.pool->region[REGION_SEND].nb);
        return 1;
      }
    } else {
      sctk_nodebug("Nb pending: %d", busy_nb);
    }
  }
  return 0;
}


/*
 * Check if a remote is in a flushing state. If it is, it
 * sends a flush REQ to the process
 */
int sctk_ibuf_rdma_check_flush_recv(sctk_ib_rail_info_t* rail_ib, sctk_ib_qp_t *remote) {
  /* Check if the RDMA is in flushing mode and if all messages
   * have be flushed to the network */
  sctk_route_state_t ret = sctk_ibuf_rdma_get_remote_state_rtr(remote);

  /* If we are in a flushing mode */
  if (ret == state_flushing) {
    int busy_nb = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_RECV]);
    ib_assume(busy_nb >= 0);
    sctk_nodebug("Number pending: %d", busy_nb);
    if ( busy_nb == 0) {
      /* We lock the region during the state modification to 'flushed'.
       * If we do not this, the RDMA buffer may be read while the RDMA buffer
       * is in a 'flushed' state */
      IBUF_RDMA_LOCK_REGION(remote, REGION_RECV);
      ret = sctk_ibuf_rdma_cas_remote_state_rtr(remote, state_flushing, state_flushed);
      IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
      if (ret == state_flushing) {
        sctk_ib_debug("RECV DONE Trying to flush RDMA for for remote %d", remote->rank);
        /* All the requests have been flushed. We can now send the resizing request.
         * NOTE: we are sure that only one thread call the resizing request */

        /* Resizing the RDMA buffer */
        sctk_ibuf_rdma_region_reinit(rail_ib, remote,
          &remote->rdma.pool->region[REGION_RECV],
          RDMA_CHANNEL | RECV_CHANNEL,
          remote->rdma.pool->resizing_request.recv_keys.nb,
          remote->rdma.pool->resizing_request.recv_keys.size);

        /* Fill the keys */
        sctk_ib_cm_rdma_connection_t *send_keys = &remote->rdma.pool->resizing_request.send_keys;
        sctk_ibuf_rdma_fill_remote_addr(rail_ib, remote, send_keys, REGION_RECV);
        send_keys->connected = 1;

        sctk_ib_nodebug("Sending a FLUSH ACK message to remote %d", remote->rank);
        sctk_ib_cm_resizing_rdma_ack(rail_ib->rail, remote, send_keys);
        return 1;
      }
    } else {
      sctk_nodebug("Nb pending: %d", busy_nb);
    }
  }
  return 0;
}



void sctk_ibuf_rdma_flush_recv(sctk_ib_rail_info_t* rail_ib, sctk_ib_qp_t *remote) {
  int ret = -1;
  sctk_route_state_t state;
  sctk_ib_debug("RECV Trying to flush RDMA for for remote %d", remote->rank);

  /*
   * Flush the RDMA buffers.
   */
  do {
    ret = sctk_ib_rdma_eager_poll_remote(rail_ib, remote);
    sctk_debug("Poll");
  } while (ret != 0);

  /* We need to change the state AFTER flushing the buffers */
  state = sctk_ibuf_rdma_cas_remote_state_rtr(remote, state_connected, state_flushing);
  assume(state == state_connected);
  sctk_ibuf_rdma_check_flush_recv(rail_ib, remote);
}

#endif
