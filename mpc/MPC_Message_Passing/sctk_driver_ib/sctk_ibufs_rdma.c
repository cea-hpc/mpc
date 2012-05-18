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
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_sr.h"
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

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/*
 * Check if a remote is connected to the process with a RDMA channel
 */
int
sctk_ibuf_rdma_is_remote_connected(sctk_ib_qp_t* remote) {
  /* FIXME: Change the test */
  return (remote->ibuf_rdma != NULL);
}

static inline void
sctk_ibuf_rdma_region_init(struct sctk_ib_rail_info_s *rail_ib,sctk_ib_qp_t* remote,
    sctk_ibuf_region_t *region, enum sctk_ibuf_channel channel, int nb_ibufs) {
  LOAD_CONFIG(rail_ib);
  LOAD_MMU(rail_ib);
  void* ptr = NULL;
  void* ibuf;
  sctk_ibuf_t* ibuf_ptr;
  int i;

  /* XXX: replaced by memalign_on_node */
  sctk_posix_memalign( (void**) &ptr, mmu->page_size, nb_ibufs * config->ibv_eager_rdma_limit);
  assume(ptr);
  memset(ptr, 0, nb_ibufs * config->ibv_eager_rdma_limit);

  /* XXX: replaced by memalign_on_node */
   sctk_posix_memalign(&ibuf, mmu->page_size, nb_ibufs * sizeof(sctk_ibuf_t));
  assume(ibuf);
  memset (ibuf, 0, nb_ibufs * sizeof(sctk_ibuf_t));

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
   * FIXME: Always task on NUMA node 0 which firs-touch all pages... really bad */
  region->mmu_entry = sctk_ib_mmu_register_no_cache(rail_ib, ptr,
      nb_ibufs * config->ibv_eager_rdma_limit);
  sctk_nodebug("Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey);

  /* init all buffers */
  for(i=0; i < nb_ibufs; ++i) {
    const char *buffer = (char*) ((char*) ptr + (i*config->ibv_eager_rdma_limit));
;
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
    /* Set headers value */
    *(ibuf_ptr->head_flag) = IBUF_RDMA_RESET_FLAG;

    DL_APPEND(region->list, ((sctk_ibuf_t*) ibuf + i));
    sctk_nodebug("ibuf=%p, index=%d, buffer=%p, head_flag=%p, size_flag=%p",
        ibuf_ptr, i, ibuf_ptr->buffer, ibuf_ptr->head_flag, ibuf_ptr->size_flag);
  }

  /* Setup tail and head pointers */
  region->head = ibuf;
  region->tail = ibuf_ptr;

  sctk_nodebug("Head=%p(%d) Tail=%p(%d)", region->head, region->head->index,
      region->tail, region->tail->index);
}

/*
 * Create a region of ibufs for RDMA
 */
sctk_ibuf_rdma_pool_t*
sctk_ibuf_rdma_pool_init(struct sctk_ib_rail_info_s *rail_ib, sctk_ib_qp_t* remote, int nb_ibufs) {
  LOAD_DEVICE(rail_ib);
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_rdma_pool_t* pool = NULL;
  static sctk_spinlock_t rdma_lock = SCTK_SPINLOCK_INITIALIZER;

  /* Check if we can allocate an RDMA channel */
  sctk_spinlock_lock(&rdma_lock);
  if (config->ibv_max_rdma_connections >= (device->eager_rdma_connections + 1)) {
    ++device->eager_rdma_connections;
    sctk_spinlock_unlock(&rdma_lock);

    sctk_ib_debug("Allocating %d RDMA buffers (connections: %d)", nb_ibufs, device->eager_rdma_connections);

    pool = sctk_malloc(sizeof(sctk_ibuf_rdma_pool_t));
    assume(pool);
    memset(pool, 0, sizeof(sctk_ibuf_rdma_pool_t));

    /* Initialize regions for send and recv */
    sctk_ibuf_rdma_region_init(rail_ib, remote, &pool->region[REGION_SEND],
        RDMA_CHANNEL | SEND_CHANNEL, nb_ibufs);
    sctk_ibuf_rdma_region_init(rail_ib, remote, &pool->region[REGION_RECV],
        RDMA_CHANNEL | RECV_CHANNEL, nb_ibufs);

    /* Setup local addr */
    pool->remote_region[REGION_SEND] = NULL;
    pool->remote_region[REGION_RECV] = NULL;
    pool->send_credit = nb_ibufs;
    pool->remote = remote;
    sctk_ibuf_rdma_set_state_remote(pool->remote, state_deconnected);

    /* Add the entry to the list */
    sctk_spinlock_lock(&rdma_pool_list_lock);
    DL_APPEND(rdma_pool_list_to_merge, pool);
    sctk_spinlock_unlock(&rdma_pool_list_lock);

    sctk_ib_debug("[rank:%d] Allocation of %d RDMA buffers", remote->rank, nb_ibufs);

    remote->ibuf_rdma = pool;
  } else {
    sctk_spinlock_unlock(&rdma_lock);
    sctk_debug("Cannot no more allocate RDMA channels (connections: %d)", device->eager_rdma_connections);
  }

  return pool;
}

/*
 * Delete all buffers
 */
static inline void
sctk_ibuf_rdma_region_free(struct sctk_ib_rail_info_s *rail_ib,
    sctk_ibuf_region_t *region) {
  void* ptr = NULL;
  void* ibuf;
  sctk_ibuf_t* ibuf_ptr;
  int i;

  assume(region->buffer_addr);
  assume(region->ibuf);

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
  LOAD_DEVICE(rail_ib);
  LOAD_CONFIG(rail_ib);
  static sctk_spinlock_t rdma_lock = SCTK_SPINLOCK_INITIALIZER;

  /* Check if we can allocate an RDMA channel */
  sctk_spinlock_lock(&rdma_lock);
  if (sctk_ibuf_rdma_is_remote_connected(remote)) {
    sctk_ibuf_rdma_pool_t* pool = remote->ibuf_rdma;
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
    free(remote->ibuf_rdma);
    remote->ibuf_rdma = NULL;

    sctk_ib_debug("[rank:%d] Free RDMA buffers", remote->rank);
  } else {
    sctk_spinlock_unlock(&rdma_lock);
  }
}


/*
 * Pick a RDMA buffer from the RDMA channel
 */
inline sctk_ibuf_t *sctk_ibuf_rdma_pick(sctk_ib_rail_info_t* rail_ib, sctk_ib_qp_t* remote) {
  sctk_ibuf_t *tail;
  int *piggyback;

  IBUF_RDMA_LOCK_REGION(remote, REGION_SEND);

  tail = IBUF_RDMA_GET_TAIL(remote, REGION_SEND);

  /* No buffer available */
  piggyback =  &IBUF_GET_EAGER_PIGGYBACK(
      IBUF_RDMA_GET_NEXT(tail)->buffer);

  sctk_nodebug("Piggy back field %d %p", *piggyback, piggyback);

  if (*piggyback > 0) {
    /* Increment the credit for RDMA send */
    remote->ibuf_rdma->send_credit += *piggyback;
    /* Move the tail flag */
    IBUF_RDMA_GET_TAIL(remote, REGION_SEND) =
      IBUF_RDMA_ADD(tail, *piggyback);

    sctk_nodebug("Piggy backed %d %p", *piggyback, piggyback);
  }

  /* If a buffer is available */
  if (remote->ibuf_rdma->send_credit > 0) {
    sctk_ibuf_t *head;

    head = IBUF_RDMA_GET_HEAD(remote, REGION_SEND);
    /* Update the credit */
    remote->ibuf_rdma->send_credit--;
    /* Move the head flag */
    IBUF_RDMA_GET_HEAD(remote, REGION_SEND) = IBUF_RDMA_GET_NEXT(head);
    /* Reset the piggyback on the remote */
    IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);

    /* Update ibuf */
    sctk_nodebug("Picked RDMA buffer %p, buffer=%p pb=%p index=%dn credit=%d", head,
        head->buffer, &IBUF_GET_EAGER_PIGGYBACK(head->buffer), head->index, remote->ibuf_rdma->send_credit);
    head->flag = BUSY_FLAG;
    IBUF_GET_EAGER_PIGGYBACK(head->buffer) = -1;

    return (sctk_ibuf_t*) head;
  }
  else {
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
  assume(ibuf);
  int release_ibuf;

  /* Set the buffer as releasable. Actually, we need to do this reset
   * here.. */
  ibuf->to_release = IBUF_RELEASE;

  const size_t size_flag = *ibuf->size_flag;
  int *tail_flag = IBUF_RDMA_GET_TAIL_FLAG(ibuf->buffer, size_flag);
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
      assume(0);
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
    /* We check if the remote is connected. If not, do not pool */
    if (sctk_ibuf_rdma_get_state_remote(pool->remote) == state_connected) {
      *ret |= func(rail, pool->remote);
    }
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
      if (sctk_ibuf_rdma_get_state_remote(pool->remote) == state_connected) {
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
 * Returns 1 if a message has been found, 0 else
 */
int
sctk_ib_rdma_eager_poll_remote(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote) {
  /* We return if the remote is not connected to the RDMA channel */
  if (sctk_ibuf_rdma_is_remote_connected(remote) == 0) return 0;
  static sctk_spinlock_t poll_lock = SCTK_SPINLOCK_INITIALIZER;
  sctk_ibuf_t *head;

//retry:
  head = IBUF_RDMA_GET_HEAD(remote, REGION_RECV);

  if (*(head->head_flag) != IBUF_RDMA_RESET_FLAG) {

    /* Spinlock for preventing two concurrent calls to this function */
    if (sctk_spinlock_trylock(&poll_lock) == 0) {
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
        sctk_spinlock_unlock(&poll_lock);

        sctk_nodebug("Buffer size:%d, head flag:%d, tail flag:%d", *head->size_flag, *head->head_flag, *tail_flag);

        if (head->flag != FREE_FLAG) {
          sctk_error("GET %p (%d)", head, head->flag);
          assume(0);
        }
        /* Handle the ibuf */
        __poll_ibuf(rail_ib, remote, head);
        /* We retry to poll a message.
         * FIXME: only retry if the sequence number of the message is not
         * expected */
//        goto retry;
        return 1;
      } else {
        sctk_spinlock_unlock(&poll_lock);
      }
    }
  }
  return 0;
}

void
sctk_ib_rdma_eager_poll_send(sctk_ib_rail_info_t *rail_ib, sctk_ibuf_t* ibuf) {
  sctk_ibuf_rdma_release(rail_ib, ibuf);
}


/* FIXME: Ideally, the following function should be merged with the
 * function in the polling */
void
sctk_ib_rdma_eager_poll_recv(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote, int index) {
#if 0
  sctk_rail_info_t* rail = rail_ib->rail;
  sctk_ibuf_t *ibuf = IBUF_RDMA_GET_ADDR_FROM_INDEX(remote, REGION_RECV, index);
  assume(ibuf);
  __poll_ibuf(rail_ib, remote, ibuf);
#endif

  sctk_ib_rdma_eager_poll_remote(rail_ib, remote);
}

/*
 * Release a buffer from the RDMA channel
 */
void sctk_ibuf_rdma_release(sctk_ib_rail_info_t* rail_ib, sctk_ibuf_t* ibuf) {
  sctk_ib_qp_t * remote = ibuf->region->remote;
  LOAD_CONFIG(rail_ib);

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
    sctk_ibuf_t *next_tail = IBUF_RDMA_GET_NEXT(tail);

    /* Mark the buffer as polled */
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
    while (ibuf->flag == EAGER_RDMA_POLLED && ibuf == next_tail) {
      piggyback ++;
      /* Put the buffer as free */
      ibuf->flag = FREE_FLAG;
      sctk_nodebug("Set %p as FREE", ibuf);

      /* Move to next buffers */
      next_tail = IBUF_RDMA_GET_NEXT(next_tail);
      ibuf = IBUF_RDMA_GET_NEXT(ibuf);
    }

    /* Piggyback the ibuf to the tail addr */
    if (piggyback > 0) {
      /* Move tail */
      IBUF_RDMA_GET_TAIL(remote, REGION_RECV) = IBUF_RDMA_ADD(tail, piggyback);
      /* Unlock the RDMA region */
      IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);

      IBUF_GET_EAGER_PIGGYBACK(base->buffer) = piggyback;

      sctk_nodebug("Piggy back to %p pb:%d", IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote, REGION_SEND, base), piggyback);

      /* Prepare the piggyback msg.
       * FIXME: there is an issue when IBV_SEND_SIGNALED missing.
       * If not present, at a certain time we cannot post ibufs
       * no more to the QP. This is bad because IBV_SEND_SIGNALED generates
       * an useless event into the CQ and could be omitted. */
      sctk_ibuf_rdma_write_init(base,
          &IBUF_GET_EAGER_PIGGYBACK(base->buffer), /* Local addr */
          base->region->mmu_entry->mr->lkey,  /* lkey */
          IBUF_RDMA_GET_REMOTE_PIGGYBACK(remote, REGION_SEND, base),  /* Remote addr */
          remote->ibuf_rdma->rkey[REGION_SEND],  /* rkey */
          sizeof(int), IBV_SEND_SIGNALED,
          IBUF_DO_NOT_RELEASE);

      /* Put the buffer as free */
      base->flag = FREE_FLAG;

      /* Send the piggyback  */
#warning "Is a control message"
      sctk_ib_qp_send_ibuf(rail_ib, remote, base, 1);
    }
    else {
      /* Unlock the RDMA region */
      IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);
    }

  } else if (IBUF_GET_CHANNEL(ibuf) & SEND_CHANNEL) {
    /* Void: nothing to do */
  } else not_reachable();
}

void
sctk_ibuf_rdma_set_state_remote(sctk_ib_qp_t* remote, sctk_route_state_t state) {

  if (sctk_ibuf_rdma_is_remote_connected(remote)) {
    remote->ibuf_rdma_state = state;
  }
}

sctk_route_state_t
sctk_ibuf_rdma_get_state_remote(sctk_ib_qp_t* remote) {

  if (sctk_ibuf_rdma_is_remote_connected(remote)) {
    return remote->ibuf_rdma_state;
  } else state_deconnected;
}


/*
 * Update a remote from the keys received by a peer. This function must be
 * called before sending any message to the RDMA channel
 */
void
sctk_ibuf_rdma_update_remote_addr(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t* remote, sctk_ib_qp_keys_t *key) {

  if (sctk_ibuf_rdma_is_remote_connected(remote)) {


    sctk_debug("Set remote addr: send_ptr=%p send_rkey=%u recv_ptr=%p recv_rkey=%u (connected: %d)",
        key->rdma.send.ptr, key->rdma.send.rkey, key->rdma.recv.ptr, key->rdma.recv.rkey, key->connected);

    if (key->connected == 0)  {
      sctk_ibuf_rdma_pool_free(rail_ib, remote);
      return;
    }

    /* Update keys for send and recv regions. We need to register the send region
     * because of the piggyback */
    remote->ibuf_rdma->remote_region[REGION_SEND] = key->rdma.send.ptr;
    remote->ibuf_rdma->rkey[REGION_SEND] = key->rdma.send.rkey;

    remote->ibuf_rdma->remote_region[REGION_RECV] = key->rdma.recv.ptr;
    remote->ibuf_rdma->rkey[REGION_RECV] = key->rdma.recv.rkey;
  }
}



#endif
