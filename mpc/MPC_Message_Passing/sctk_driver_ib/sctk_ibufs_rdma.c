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

/* Linked list of rdma_poll where each remote using the RDMA eager protocol
 * is listed*/
sctk_ibuf_rdma_pool_t *rdma_pool_list = NULL;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/*
 * Check if a remote is connected to the process with a RDMA channel
 */
int
sctk_ibuf_rdma_is_remote_connected(struct sctk_ib_rail_info_s *rail_ib,sctk_ib_qp_t* remote) {

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
  sctk_ibuf_rdma_pool_t* pool;

  sctk_ib_debug("Allocating %d RDMA buffers", nb_ibufs);

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
  DL_APPEND(rdma_pool_list, pool);

  sctk_ib_debug("[rank:%d] Allocation of %d RDMA buffers", remote->rank, nb_ibufs);

  remote->ibuf_rdma = pool;

  return pool;
}

/*
 * Pick a RDMA buffer from the RDMA channel
 */
static inline sctk_ibuf_t *sctk_ibuf_rdma_pick(sctk_ib_rail_info_t* rail_ib, sctk_ib_qp_t* remote) {
  sctk_ibuf_t *head;
  sctk_ibuf_t *tail;
  int *piggyback;

  IBUF_RDMA_LOCK_REGION(remote, REGION_SEND);

  tail = IBUF_RDMA_GET_TAIL(remote, REGION_SEND);

  /* No buffer available */
  piggyback =  &IBUF_GET_EAGER_PIGGYBACK(
      IBUF_RDMA_GET_NEXT(tail)->buffer);

  sctk_nodebug("Piggy back field %d %p", *piggyback, piggyback);

  if (*piggyback != 0) {
    /* Increment the credit for RDMA send */
    remote->ibuf_rdma->send_credit += *piggyback;
    /* Move the tail flag */
    IBUF_RDMA_GET_TAIL(remote, REGION_SEND) =
      IBUF_RDMA_ADD(tail, *piggyback);

    sctk_nodebug("Piggy backed %d %p", *piggyback, piggyback);
  }

  head = IBUF_RDMA_GET_HEAD(remote, REGION_SEND);

  /* If a buffer is available */
  if (remote->ibuf_rdma->send_credit >= 0) {
    /* Update the credit */
    remote->ibuf_rdma->send_credit--;
    /* Move the head flag */
    IBUF_RDMA_GET_HEAD(remote, REGION_SEND) = IBUF_RDMA_GET_NEXT(head);
    IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);

    /* Update ibuf */
    sctk_nodebug("Picked RDMA buffer %p, buffer=%p pb=%p index=%d", head,
        head->buffer, &IBUF_GET_EAGER_PIGGYBACK(head->buffer), head->index);
    head->flag = BUSY_FLAG;

    /* Reset the piggyback on the remote
     * FIXME: we should add an entry in eager and buffered for the piggyback */
    IBUF_GET_EAGER_PIGGYBACK(head->buffer) = 0;

    return (sctk_ibuf_t*) head;
  } else {
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
 * Send a message
 */
sctk_ibuf_t* sctk_ib_rdma_eager_prepare_msg(sctk_ib_rail_info_t* rail_ib,
    sctk_ib_qp_t* remote, sctk_thread_ptp_message_t * msg, size_t size) {
  void* body;
  sctk_ibuf_t *ibuf;
  sctk_ib_eager_t *eager_header;

  body = (char*)msg + sizeof(sctk_thread_ptp_message_t);
  ibuf = sctk_ibuf_rdma_pick(rail_ib, remote);
  assume(ibuf);

  /* Update RDMA buffer description */
  sctk_ib_rdma_set_tail_flag(ibuf, size + IBUF_GET_EAGER_SIZE);
  IBUF_SET_DEST_TASK(ibuf->buffer, msg->sctk_msg_get_glob_destination);
  IBUF_SET_SRC_TASK(ibuf, msg->sctk_msg_get_glob_source);
  IBUF_SET_POISON(ibuf->buffer);

  /* Copy header */
  memcpy(IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer), msg, sizeof(sctk_thread_ptp_message_body_t));
  /* Copy payload */
  sctk_net_copy_in_buffer(msg, IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer));

  sctk_ibuf_prepare(rail_ib, remote, ibuf,
      IBUF_RDMA_GET_SIZE + size + IBUF_GET_EAGER_SIZE);

  eager_header = IBUF_GET_EAGER_HEADER(ibuf->buffer);
  eager_header->payload_size = size - sizeof(sctk_thread_ptp_message_body_t);
  return ibuf;
}

/*
 * Handle a message received
 */
static void __poll_ibuf(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote,
    sctk_ibuf_t* ibuf) {
  sctk_rail_info_t* rail = rail_ib->rail;
  assume(ibuf);

  /* Set the buffer as releasable. Actually, we need to do this reset
   * here.. */
  ibuf->to_release = IBUF_RELEASE;

  const size_t size_flag = *ibuf->size_flag;
  int *tail_flag = IBUF_RDMA_GET_TAIL_FLAG(ibuf->buffer, size_flag);
  assume (*ibuf->head_flag == *tail_flag);
  sctk_nodebug("Buffer size:%d, head flag:%d, tail flag:%d", *ibuf->size_flag, *ibuf->head_flag, *tail_flag);

  sctk_ib_eager_poll_recv(rail, ibuf);
}


/*
 * Walk on each remotes
 */
void sctk_ib_rdma_eager_walk_remotes(sctk_ib_rail_info_t *rail, int (func)(sctk_ib_rail_info_t *rail, sctk_ib_qp_t* remote), int *ret) {
  sctk_ibuf_rdma_pool_t *pool;
  *ret = 0;

  CDL_FOREACH(rdma_pool_list, pool) {
    *ret |= func(rail, pool->remote);
  }
}


/*
 * Poll a specific remote in order to handle new received messages.
 * Returns 1 if a message has been found, 0 else
 */
int
sctk_ib_rdma_eager_poll_remote(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote) {
  /* We return if the remote is not connected to the RDMA channel */
  if (sctk_ibuf_rdma_is_remote_connected(rail_ib, remote) == 0) return 0;

  static sctk_spinlock_t poll_lock = SCTK_SPINLOCK_INITIALIZER;
  sctk_ibuf_t *head = IBUF_RDMA_GET_HEAD(remote, REGION_RECV);

  sctk_nodebug("Head set: %d-%d", *(head->head_flag), IBUF_RDMA_RESET_FLAG);

  if (*(head->head_flag) != IBUF_RDMA_RESET_FLAG) {

    /* Spinlock for preventing two concurrent calls to this function */
    if (sctk_spinlock_trylock(&poll_lock) == 0) {
      /* Recaculate the head because it could be moved */
      head = IBUF_RDMA_GET_HEAD(remote, REGION_RECV);
      /* Double checking */
      if (*(head->head_flag) != IBUF_RDMA_RESET_FLAG) {

        const size_t size_flag = *head->size_flag;
        volatile int volatile * tail_flag = IBUF_RDMA_GET_TAIL_FLAG(head->buffer, size_flag);

        sctk_nodebug("Head set: %d-%d", *(head->head_flag), IBUF_RDMA_RESET_FLAG);

        /* The head has been set. We spin while waiting the tail to be set.
         * We do not release the lock because it is not necessary that a task
         * concurrently polls the same remote.  */
        while (*(head->head_flag) != *(tail_flag)) { /* poll */ };
        /* Move head flag */
        IBUF_RDMA_GET_HEAD(remote, REGION_RECV) =  IBUF_RDMA_GET_NEXT(head);
        sctk_spinlock_unlock(&poll_lock);

        sctk_nodebug("Buffer size:%d, head flag:%d, tail flag:%d", *head->size_flag, *head->head_flag, *tail_flag);

        /* Handle the ibuf */
        __poll_ibuf(rail_ib, remote, head);
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
      /* Reset the head flag */
      sctk_ib_rdma_reset_head_flag(ibuf);

      /* Move to next buffers */
      next_tail = IBUF_RDMA_GET_NEXT(next_tail);
      ibuf = IBUF_RDMA_GET_NEXT(ibuf);
    }

    /* Piggyback the ibuf to the tail addr */
    if (piggyback > 0) {
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

      /* Send the piggyback  */
      sctk_ib_qp_send_ibuf(rail_ib, remote, base, 0);

      /* Move tail */
      IBUF_RDMA_GET_TAIL(remote, REGION_RECV) = IBUF_RDMA_ADD(tail, piggyback);
    }
    /* Unlock the RDMA region */
    IBUF_RDMA_UNLOCK_REGION(remote, REGION_RECV);

  } else if (IBUF_GET_CHANNEL(ibuf) & SEND_CHANNEL) {
    /* Void: nothing to do */
  } else not_reachable();
}

/*
 * Update a remote from the keys received by a peer. This function must be
 * called before sending any message to the RDMA channel
 */
void
sctk_ibuf_rdma_update_remote_addr(sctk_ib_qp_t* remote, sctk_ib_qp_keys_t *key) {
  sctk_nodebug("Set remote addr: send_ptr=%p send_rkey=%u recv_ptr=%p recv_rkey=%u",
      key->rdma.send.ptr, key->rdma.send.rkey, key->rdma.recv.ptr, key->rdma.recv.rkey);

  /* Update keys for send and recv regions. We need to register the send region
   * because of the piggyback */
  remote->ibuf_rdma->remote_region[REGION_SEND] = key->rdma.send.ptr;
  remote->ibuf_rdma->rkey[REGION_SEND] = key->rdma.send.rkey;

  remote->ibuf_rdma->remote_region[REGION_RECV] = key->rdma.recv.ptr;
  remote->ibuf_rdma->rkey[REGION_RECV] = key->rdma.recv.rkey;
}



#endif
