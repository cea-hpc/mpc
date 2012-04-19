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
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_sr.h"
#include "utlist.h"

/**
 *  RDMA for eager messages. This implementation is based on the
 *  paper 'High Performance RDMA-Based MPI Implementation
 *  over Infiniband' (RDMA FP). We use the CQ notification because polling each
 *  RDMA channel is not scalable.
 *  TODO: use an additional set of QP for RDMA where no buffers are
 *  posted in receive.
 */

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

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
  // memset(ptr, 0, nb_ibufs * config->ibv_eager_rdma_limit);

  /* XXX: replaced by memalign_on_node */
   sctk_posix_memalign(&ibuf, mmu->page_size, nb_ibufs * sizeof(sctk_ibuf_t));
  assume(ibuf);
  // memset (ibuf, 0, nb_ibufs * sizeof(sctk_ibuf_t));

  region->buffer_addr = ptr;
  region->nb = nb_ibufs;
  region->rail = rail_ib;
  region->list = NULL;
  region->channel = channel;
  region->remote = remote;
  region->lock = SCTK_SPINLOCK_INITIALIZER;
  region->ibuf = ibuf;

  /* register buffers at once
   * FIXME: Always task on NUMA node 0 which firs-touch all pages... really bad */
  region->mmu_entry = sctk_ib_mmu_register_no_cache(rail_ib, ptr,
      nb_ibufs * config->ibv_eager_rdma_limit);
  sctk_nodebug("Reg %p registered. lkey : %lu", ptr, region->mmu_entry->mr->lkey);

  /* init all buffers */
  for(i=0; i < nb_ibufs; ++i) {
    ibuf_ptr = (sctk_ibuf_t*) ibuf + i;
    ibuf_ptr->region = region;
    ibuf_ptr->size = 0;
    ibuf_ptr->flag = FREE_FLAG;
    ibuf_ptr->index = i;

    ibuf_ptr->buffer = (unsigned char*) ((char*) ptr + (i*config->ibv_eager_rdma_limit));
    DL_APPEND(region->list, ((sctk_ibuf_t*) ibuf + i));
    sctk_debug("ibuf=%p, index=%d, buffer=%p", ibuf_ptr, i, ibuf_ptr->buffer);
  }

  /* Setup tail and head pointers */
  region->head = region->list;
  region->tail = region->list->prev;
  /* Cyclic list */
  region->tail->next = region->head;

  sctk_debug("Head=%p(%d) Tail=%p(%d)", region->head, region->head->index,
      region->tail, region->tail->index);
}

/* Create a region of ibufs for RDMA */
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
  pool->local_addr = pool->region[REGION_SEND].buffer_addr;
  pool->remote_addr = NULL;

  sctk_ib_debug("[rank:%d] Allocation of %d RDMA buffers (local:%p)", remote->rank, nb_ibufs, pool->local_addr);

  remote->ibuf_rdma = pool;

  return pool;
}

/* Pick a RDMA buffer */
static inline sctk_ibuf_t *sctk_ibuf_rdma_pick(sctk_ib_rail_info_t* rail_ib, sctk_ib_qp_t* remote) {
  sctk_ibuf_t *head = IBUF_RDMA_GET_HEAD(remote, REGION_SEND);
  const sctk_ibuf_t *tail = IBUF_RDMA_GET_TAIL(remote, REGION_SEND);
  const sctk_ibuf_t *next = head->next;

  if (next == tail) return NULL;

  IBUF_RDMA_LOCK_REGION(remote, REGION_SEND);
  /* No buffer available */
  if (next == tail) {
    IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);
    return NULL;
  } else {
    IBUF_RDMA_GET_HEAD(remote, REGION_SEND) = next;
    IBUF_RDMA_UNLOCK_REGION(remote, REGION_SEND);

    /* Update ibuf */
    sctk_debug("Picked RDMA buffer %p, buffer=%p", head, head->buffer);
    assume(head->flag == FREE_FLAG);
    head->flag = BUSY_FLAG;
    return (sctk_ibuf_t*) head;
  }
  not_reachable();
}


sctk_ibuf_t* sctk_ib_rdma_eager_prepare_msg(sctk_ib_rail_info_t* rail_ib,
    sctk_ib_qp_t* remote, sctk_thread_ptp_message_t * msg, size_t size) {
  LOAD_CONFIG(rail_ib);
  void* body;
  sctk_ibuf_t *ibuf;
  sctk_ib_eager_t *eager_header;
  const sctk_ibuf_region_t* region = IBUF_RDMA_GET_REGION(remote, REGION_SEND);

  body = (char*)msg + sizeof(sctk_thread_ptp_message_t);
  ibuf = sctk_ibuf_rdma_pick(rail_ib, remote);
  assume(ibuf);
  IBUF_SET_DEST_TASK(ibuf, msg->sctk_msg_get_glob_destination);
  IBUF_SET_SRC_TASK(ibuf, msg->sctk_msg_get_glob_source);

  /* Copy header */
  memcpy(IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer), msg, sizeof(sctk_thread_ptp_message_body_t));
  /* Copy payload */
  sctk_net_copy_in_buffer(msg, IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer));

  /* Initialization of the buffer */
  sctk_debug("Send msg with imm_data: %d", ibuf->index);
  sctk_ibuf_rdma_write_with_imm_init(ibuf,
      ibuf->buffer, /* Local addr */
      region->mmu_entry->mr->lkey,  /* lkey */
      IBUF_RDMA_GET_REMOTE_ADDR(remote, ibuf),  /* Remote addr */
      remote->ibuf_rdma->rkey,  /* rkey */
      IBUF_GET_EAGER_SIZE + size, /* size */
      ibuf->index | IMM_DATA_EAGER_RDMA);  /* imm_data: index of the ibuf in the region */

  IBUF_SET_PROTOCOL(ibuf->buffer, eager_rdma_protocol);

  eager_header = IBUF_GET_EAGER_HEADER(ibuf->buffer);
  eager_header->payload_size = size - sizeof(sctk_thread_ptp_message_body_t);
  sctk_debug("RDMA eager ready");

  return ibuf;
}

void
sctk_ib_rdma_eager_poll_send(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote, int index) {
  LOAD_CONFIG(rail_ib);
  sctk_ibuf_t *ibuf = IBUF_RDMA_GET_ADDR_FROM_INDEX(remote, REGION_SEND, index);

  sctk_ibuf_release(rail_ib, ibuf);
}


/* FIXME: Ideally, the following function should be merged with the
 * function in the polling */
void
sctk_ib_rdma_eager_poll_recv(sctk_ib_rail_info_t *rail_ib, sctk_ib_qp_t *remote, int index) {
  LOAD_CONFIG(rail_ib);
  sctk_rail_info_t* rail = rail_ib->rail;
  sctk_ibuf_t *ibuf = IBUF_RDMA_GET_ADDR_FROM_INDEX(remote, REGION_RECV, index);
  assume(ibuf);
  const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL(ibuf->buffer);
  sctk_thread_ptp_message_t * msg = NULL;
  sctk_thread_ptp_message_body_t * msg_ibuf = NULL;
  specific_message_tag_t tag;
  int release_ibuf = 1;
  int recopy = 1;

  msg_ibuf = IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer);
  tag = msg_ibuf->header.specific_message_tag;

  if (IS_PROCESS_SPECIFIC_MESSAGE_TAG(tag)) {
    /* If on demand, handle message and do not send it
     * to high-layers */
    if (IS_PROCESS_SPECIFIC_ONDEMAND(tag)) {
      sctk_nodebug("Received OD message");
      msg = sctk_ib_sr_recv(rail, ibuf, recopy, protocol);
      sctk_ib_cm_on_demand_recv(rail, msg, ibuf, recopy);
      goto release;
    } else if (IS_PROCESS_SPECIFIC_LOW_MEM(tag)) {
      sctk_nodebug("Received low mem message");
      msg = sctk_ib_sr_recv(rail, ibuf, recopy, protocol);

      goto release;
    }
  } else {
    /* Do not recopy message if it is not a process specific message.
     *
     * When there is an intermediate message, we *MUST* recopy the message
     * because MPC does not match the user buffer with the network buffer (the copy function is
     * not performed) */
    recopy = 0;
  }

  /* Normal message: we handle it */
  msg = sctk_ib_sr_recv(rail, ibuf, recopy, protocol);
  sctk_ib_sr_recv_free(rail, msg, ibuf, recopy);
  rail->send_message_from_network(msg);

release:
  release_ibuf = 0;

  /* We do not need to release the buffer to SRQ in RDMA */
  if (release_ibuf) {
    /* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
    sctk_ibuf_release(&rail->network.ib, ibuf);
  }
}


void sctk_ibuf_rdma_release(sctk_ib_rail_info_t* rail_ib, sctk_ibuf_t* ibuf) {
  const sctk_ib_qp_t const * remote = ibuf->region->remote;
  sctk_ibuf_t *tail = IBUF_RDMA_GET_TAIL(remote, REGION_RECV);
  const sctk_ibuf_t const *next = ibuf->next;

  sctk_debug("Freeing a RDMA buffer (head:%p - ibuf:%p - tail:%p)", ibuf->region->head, ibuf, ibuf->region->tail);

  if (IBUF_GET_CHANNEL(ibuf) & RECV_CHANNEL) {
    sctk_debug("Recv channel");
    /* First mark the buffer as polled */
    ibuf->flag = EAGER_RDMA_POLLED;
    /* Check if the buffer is located at the tail adress */
    while (ibuf->flag == EAGER_RDMA_POLLED && ibuf == tail) {
      sctk_debug("Head == tail");
    }
  } else if (IBUF_GET_CHANNEL(ibuf) & SEND_CHANNEL) {
    sctk_debug("Send channel");

  } else not_implemented();

  /* Piggyback the message */

  ibuf->flag = FREE_FLAG;
}


void
sctk_ibuf_rdma_update_remote_addr(sctk_ib_qp_t* remote, sctk_ib_qp_keys_t *key) {
  remote->ibuf_rdma->remote_addr = key->rdma.ptr;
  remote->ibuf_rdma->rkey = key->rdma.rkey;
}



#endif
