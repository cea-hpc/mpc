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
#include "sctk_ib.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_sr.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_ib_mmu.h"
#include "sctk_net_tools.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "RDMA"
#include "sctk_ib_toolkit.h"

#define HOSTNAME 2048

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
#define IBUF_GET_RDMA_TYPE(x) ((x)->type)
#define IBUF_SET_RDMA_TYPE(x, __type) ((x)->type = __type)

static inline void sctk_ib_rdma_align_msg(void *addr, size_t  size,
    void **aligned_addr, size_t *aligned_size)
{
 /*XXX Get page_size from device */
  size_t page_size;
  page_size = getpagesize();

  /* align on a page size */
  *aligned_addr = addr -
    (((unsigned long) addr) % page_size);
  *aligned_size = size +
    (((unsigned long) addr) % page_size);

  sctk_nodebug("ptr:%p size:%lu -> ptr:%p size:%lu", addr, size,
      *aligned_addr, *aligned_size);
}

/* Functions to free messages used in transmission and reception */
void sctk_ib_rdma_net_free_send(void* arg) {
  sctk_thread_ptp_message_t * msg = (sctk_thread_ptp_message_t*) arg;

  /* Unregister MMU and free message */
  sctk_ib_mmu_unregister( &msg->tail.ib.rdma.rail->network.ib,
      msg->tail.ib.rdma.local.mmu_entry);
}

void sctk_ib_rdma_net_free_recv(void* arg) {
  sctk_thread_ptp_message_t * msg = (sctk_thread_ptp_message_t*) arg;

  /* Unregister MMU and free message */
  sctk_ib_mmu_unregister( &msg->tail.ib.rdma.rail->network.ib,
      msg->tail.ib.rdma.local.mmu_entry);
  sctk_free(msg);
}

/*-----------------------------------------------------------
 *  SEND
 *----------------------------------------------------------*/
void sctk_ib_rdma_prepare_send_msg (sctk_ib_rail_info_t* rail_ib,
    sctk_thread_ptp_message_t * msg)
{
  void* aligned_addr = NULL;
  size_t aligned_size = 0;

  /* Allocate memory if not contiguous*/
  if (msg->tail.message_type == sctk_message_contiguous)
  {
    sctk_nodebug("Sending contiguous message");
    sctk_ib_rdma_align_msg(msg->tail.message.contiguous.addr,
        msg->tail.message.contiguous.size,
        &aligned_addr, &aligned_size);
    msg->tail.ib.rdma.local.addr = msg->tail.message.contiguous.addr;
    msg->tail.ib.rdma.local.size = msg->tail.message.contiguous.size;
  } else {
    sctk_nodebug("Sending NOT contiguous message");
    not_implemented();
  }

  sctk_reinit_header(msg, sctk_ib_rdma_net_free_send, NULL);
  /* Register MMU */
  msg->tail.ib.rdma.local.mmu_entry =  sctk_ib_mmu_register (
      rail_ib, aligned_addr, aligned_size);
  /* Save addr and size */
  msg->tail.ib.rdma.local.aligned_addr = aligned_addr;
  msg->tail.ib.rdma.local.aligned_size = aligned_size;
  /* Message is now ready ready */
  msg->tail.ib.rdma.local.ready = 1;
}

/*
 * SEND REQUEST
 */
sctk_ibuf_t* sctk_ib_rdma_prepare_req(sctk_rail_info_t* rail,
    sctk_route_table_t* route_table, sctk_thread_ptp_message_t * msg, size_t size) {
  sctk_ibuf_t *ibuf;
  sctk_ib_rdma_t *rdma_header;
  sctk_ib_rdma_req_t *rdma_req;

  ibuf = sctk_ibuf_pick(&rail->network.ib, 1, 0);
  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_req_type);

  /* Initialization of the request */
  rdma_req    = IBUF_GET_RDMA_REQ(ibuf->buffer);
  rdma_req->requested_size = size - sizeof(sctk_thread_ptp_message_body_t);
  rdma_req->dest_msg_header = msg;

  /* Initialization of the msg header */
  memcpy(&rdma_req->msg_header, &msg->body, sizeof(sctk_thread_ptp_message_body_t));
  /* Message not ready: memory not pinned */
  msg->tail.ib.rdma.local.ready = 0;
  msg->tail.ib.rdma.rail = rail;
  msg->tail.ib.rdma.route_table = route_table;
  sctk_nodebug("msg: %p - Rail: %p %p", msg, rail, &msg->tail.ib.rdma);

  /* Initialization of the buffer */
  sctk_ibuf_send_init(ibuf, IBUF_GET_RDMA_REQ_SIZE);
  sctk_ibuf_set_protocol(ibuf, rdma_protocol);

  sctk_nodebug("Req sent (size:%lu, requested:%d, ibuf:%p)", IBUF_GET_RDMA_REQ_SIZE,
      rdma_req->requested_size, ibuf);

  return ibuf;
}

/*
 * SEND RDMA ACK
 */
inline sctk_ibuf_t* sctk_ib_rdma_prepare_ack(sctk_ib_rail_info_t* rail_ib,
  sctk_thread_ptp_message_t* msg) {
  sctk_ibuf_t *ibuf;
  sctk_ib_rdma_t *rdma_header;
  sctk_ib_rdma_ack_t *rdma_ack;

  ibuf = sctk_ibuf_pick(rail_ib, 1, 0);
  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_ack_type);

  rdma_ack    = IBUF_GET_RDMA_ACK(ibuf->buffer);

  rdma_ack->addr = msg->tail.ib.rdma.local.addr;
  rdma_ack->rkey = msg->tail.ib.rdma.local.mmu_entry->mr->rkey;
  rdma_ack->dest_msg_header = msg->tail.ib.rdma.remote.msg_header;
  rdma_ack->src_msg_header = msg;

  /* Initialization of the buffer */
  sctk_ibuf_send_init(ibuf, IBUF_GET_RDMA_ACK_SIZE);
  sctk_ibuf_set_protocol(ibuf, rdma_protocol);

  sctk_nodebug("Send RDMA ACK message: %lu %u", rdma_ack->addr, rdma_ack->rkey);
  return ibuf;
}

/*
 * SEND RDMA DATA
 */
inline void
sctk_ib_rdma_prepare_data_write(sctk_rail_info_t* rail,
    sctk_thread_ptp_message_t *src_msg_header,
    sctk_ibuf_t *ibuf) {
  sctk_ib_rdma_data_write_t *rdma_data_write;
  sctk_ib_rdma_t *rdma_header;

  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);

  rdma_data_write = IBUF_GET_RDMA_DATA_WRITE(ibuf->buffer);
  sctk_nodebug("msg: %p - Rail: %p (%p-%p)", src_msg_header, rail, ibuf, rdma_data_write);
  sctk_nodebug("Write msg %p %lu (mmu:%p)",
      src_msg_header->tail.ib.rdma.remote.addr,
      src_msg_header->tail.ib.rdma.remote.rkey,
      src_msg_header->tail.ib.rdma.local.mmu_entry);
  rdma_data_write->src_msg_header = src_msg_header;
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_data_write_type);
  sctk_ibuf_set_protocol(ibuf, rdma_protocol);

  sctk_nodebug("Write from %p (%lu) to %p (%lu)",
      src_msg_header->tail.ib.rdma.local.addr,
      src_msg_header->tail.ib.rdma.local.size,
      src_msg_header->tail.ib.rdma.remote.addr,
      src_msg_header->tail.ib.rdma.remote.size);


  sctk_ibuf_rdma_write_init(ibuf,
      src_msg_header->tail.ib.rdma.local.addr,
      src_msg_header->tail.ib.rdma.local.mmu_entry->mr->lkey,
      src_msg_header->tail.ib.rdma.remote.addr,
      src_msg_header->tail.ib.rdma.remote.rkey,
      src_msg_header->tail.message.contiguous.size);
}

/*
 * SEND RDMA DONE
 */
inline sctk_thread_ptp_message_t*
sctk_ib_rdma_prepare_done_write(sctk_rail_info_t* rail,
    sctk_ibuf_t *ibuf) {
  sctk_thread_ptp_message_t *src_msg_header;
  sctk_thread_ptp_message_t *dest_msg_header;
  sctk_ib_rdma_t *rdma_header;
  sctk_ib_rdma_done_t *rdma_done;
  sctk_ib_rdma_data_write_t *rdma_data_write;

  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  rdma_data_write = IBUF_GET_RDMA_DATA_WRITE(ibuf->buffer);
  src_msg_header = rdma_data_write->src_msg_header;
  dest_msg_header = src_msg_header->tail.ib.rdma.remote_msg_header;

  /* At this time, we can rewrite informations on the ibuf */
  rdma_done = IBUF_GET_RDMA_DONE(ibuf->buffer);
  rdma_done->dest_msg_header = dest_msg_header;
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_done_type);
  sctk_ibuf_send_init(ibuf, IBUF_GET_RDMA_DONE_SIZE);
  sctk_ibuf_set_protocol(ibuf, rdma_protocol);

  return src_msg_header;
}


void sctk_ib_rdma_free_msg_no_recopy(sctk_thread_ptp_message_t * msg) { }

void sctk_ib_rdma_net_copy(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  sctk_ib_msg_header_t *send_header;

  send = tmp->msg_send;
  recv = tmp->msg_recv;
  send_header = &send->tail.ib;
  send_header->rdma.local.addr  = recv->tail.message.contiguous.addr;
  send_header->rdma.local.size  = recv->tail.message.contiguous.size;

  /* The buffer has been posted and if the message is contiguous,
   * we can initiate a RDMA data transfert */
  sctk_spinlock_lock(&send_header->rdma.lock);

  /* if (send_header->rdma.status != done || 1) { */
    sctk_ibuf_t *ibuf;
    sctk_ib_qp_t *remote;
    sctk_ib_rail_info_t *rail_ib = &send_header->rdma.rail->network.ib;

    if (recv->tail.message_type == sctk_message_contiguous) {
        send_header->rdma.status       = zerocopy;

        sctk_nodebug("Zerocopy. (Send:%p Recv:%p)", send, recv);
        sctk_ib_rdma_align_msg(send_header->rdma.local.addr,
          send_header->rdma.local.size,
          &send_header->rdma.local.aligned_addr,
          &send_header->rdma.local.aligned_size);
    } else { /* not contiguous message */
      send_header->rdma.status       = recopy;
      /*XXX Get page_size from device */
      sctk_error("Not checked");
      assume(0);
      size_t page_size;
      page_size = getpagesize();
      send_header->rdma.local.aligned_size = send_header->rdma.requested_size;
      /* Allocating memory according to the requested size */
      posix_memalign((void**) &send_header->rdma.local.aligned_addr,
          page_size, send_header->rdma.local.aligned_size );
      sctk_nodebug("Allocating memory (size:%lu)", send_header->rdma.requested_size);
    }

    sctk_nodebug("Copy_ptr: %p (free:%p, ptr:%p)", tmp, send->tail.free_memory,
        send_header->rdma.local.aligned_addr);
    send_header->rdma.copy_ptr = tmp;
    /* Register MMU */
    send_header->rdma.local.mmu_entry =  sctk_ib_mmu_register (
        rail_ib, send_header->rdma.local.aligned_addr,
        send_header->rdma.local.aligned_size);

    ibuf = sctk_ib_rdma_prepare_ack(rail_ib, send);

    /* Send message */
    remote = send_header->rdma.route_table->data.ib.remote;
    sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf);
  /*  } else {
    not_implemented();
    // Check if the size requested is the size of the message posted
    // assume(msg->tail.ib.rdma.equested_size == msg->tail.ib.rdma.size_remote);

  } */

  sctk_spinlock_unlock(&send_header->rdma.lock);
}

/*-----------------------------------------------------------
 *  RECV
 *----------------------------------------------------------*/

/*
 * RECV ACK
 */
static inline sctk_thread_ptp_message_t *
sctk_ib_rdma_recv_ack(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_ib_rdma_ack_t *rdma_ack;
  sctk_thread_ptp_message_t *dest_msg_header;
  sctk_thread_ptp_message_t *src_msg_header;

  /* Save the values of the ack because the buffer will be reused */
  rdma_ack = IBUF_GET_RDMA_ACK(ibuf->buffer);
  src_msg_header  = rdma_ack->dest_msg_header;
  dest_msg_header = rdma_ack->src_msg_header;

  /* Wait while the message become ready */
  sctk_thread_wait_for_value((int*) &src_msg_header->tail.ib.rdma.local.ready, 1);

  src_msg_header->tail.ib.rdma.remote.addr = rdma_ack->addr;
  src_msg_header->tail.ib.rdma.remote.rkey = rdma_ack->rkey;
  src_msg_header->tail.ib.rdma.remote_msg_header = dest_msg_header;

  return src_msg_header;
}

/*
 * RECV REQ
 */
static inline sctk_thread_ptp_message_t *
sctk_ib_rdma_recv_req(sctk_rail_info_t* rail, sctk_ib_rdma_req_t *rdma_req) {
  sctk_thread_ptp_message_t *msg;
  sctk_ib_rdma_status_t status;
  sctk_route_table_t* route;

  msg = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
  memcpy(&msg->body, &rdma_req->msg_header, sizeof(sctk_thread_ptp_message_body_t));

  /* determine the route to the source */
  route = sctk_get_route(msg->sctk_msg_get_glob_source, rail);

  msg->body.completion_flag = NULL;
  msg->tail.message_type = sctk_message_network;
  /* Remote addr initially set to NULL */
  msg->tail.ib.rdma.lock        = SCTK_SPINLOCK_INITIALIZER;
  msg->tail.ib.rdma.status      = not_set;
  msg->tail.ib.rdma.requested_size  = rdma_req->requested_size;
  msg->tail.ib.rdma.rail        = rail;
  msg->tail.ib.rdma.route_table = route;
  msg->tail.ib.rdma.local.addr  = NULL;
  msg->tail.ib.rdma.remote.msg_header = rdma_req->dest_msg_header;

  sctk_rebuild_header(msg);
  sctk_reinit_header(msg, sctk_ib_rdma_net_free_recv, sctk_ib_rdma_net_copy);

  /* Send message to MPC. The message can be matched at the end
   * of function. */
  rail->send_message_from_network(msg);

  sctk_spinlock_lock(&msg->tail.ib.rdma.lock);
  status = msg->tail.ib.rdma.status;
  if (status == not_set)
    status = msg->tail.ib.rdma.status = done;
  sctk_spinlock_unlock(&msg->tail.ib.rdma.lock);

#if 0
  if (status == done)
  {
  }
#endif

  sctk_nodebug("End send_message (matching:%p, stats:%d, "
      "requested_size:%lu, required_size:%lu)",
    msg->tail.ib.rdma.local.addr, msg->tail.ib.rdma.status,
    rdma_req->requested_size, msg->tail.ib.rdma.local.size);

  return msg;
}

/*
 * RECV DONE REMOTE
 */
static inline sctk_thread_ptp_message_t *
sctk_ib_rdma_recv_done_remote(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_ib_rdma_done_t *rdma_done;
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  sctk_thread_ptp_message_t *dest_msg_header;

  /* Save the values of the ack because the buffer will be reused */
  rdma_done = IBUF_GET_RDMA_DONE(ibuf->buffer);
  dest_msg_header = rdma_done->dest_msg_header;

  send = dest_msg_header->tail.ib.rdma.copy_ptr->msg_send;
  recv = dest_msg_header->tail.ib.rdma.copy_ptr->msg_recv;

  sctk_nodebug("msg: %p - Rail: %p (%p-%p) copy_ptr:%p (send:%p recv:%p)", dest_msg_header, rail, ibuf, rdma_done, dest_msg_header->tail.ib.rdma.copy_ptr, send, recv);

  sctk_message_completion_and_free(send,recv);
  sctk_nodebug("MSG REMOTE FREE");
  return NULL;
}

/*
 * RECV DONE LOCAL
 */
static inline void
sctk_ib_rdma_recv_done_local(sctk_rail_info_t* rail, sctk_thread_ptp_message_t* msg)
{
  sctk_complete_and_free_message(msg);
  sctk_nodebug("MSG LOCAL FREE");
}

int
sctk_ib_rdma_poll_recv(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_ib_rdma_t *rdma_header;
  sctk_thread_ptp_message_t *header;

  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  /* Switch on the RDMA type of message */
  switch(IBUF_GET_RDMA_TYPE(rdma_header)) {
    case rdma_req_type:
      sctk_nodebug("Poll recv: message RDMA req received");
      sctk_ib_rdma_recv_req(rail, IBUF_GET_RDMA_REQ(ibuf->buffer));
      return 1;
      break;

    case rdma_ack_type:
      sctk_nodebug("Poll recv: message RDMA ack received");
      /* Buffer reused: don't need to free it */
      header = sctk_ib_rdma_recv_ack(rail, ibuf);

      sctk_ib_rdma_prepare_data_write(rail, header, ibuf);
      sctk_ibuf_release_from_srq(&rail->network.ib, ibuf);

      sctk_ib_qp_send_ibuf(&rail->network.ib,
      header->tail.ib.rdma.route_table->data.ib.remote,  ibuf);
      return 0;
      break;

    case rdma_done_type:
      sctk_nodebug("Poll recv: message RDMA done received");
      sctk_ib_rdma_recv_done_remote(rail, ibuf);
      return 1;
      break;

    default:assume(0);
  }
  assume(0);
  return 0;
}

int
sctk_ib_rdma_poll_send(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_ib_rdma_t *rdma_header;
  /* If the buffer *MUST* be released */
  int release_ibuf = 1;
  sctk_thread_ptp_message_t* msg;

  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  /* Switch on the RDMA type of message */
  switch(IBUF_GET_RDMA_TYPE(rdma_header)) {
    case rdma_req_type:
      sctk_nodebug("Poll send: message RDMA req received");
      break;

    case rdma_ack_type:
      sctk_nodebug("Poll send: message RDMA ack received");
      break;

    case rdma_data_write_type:
      sctk_nodebug("Poll send: message RDMA data write received");
      msg = sctk_ib_rdma_prepare_done_write(rail, ibuf);

      sctk_ib_qp_send_ibuf(&rail->network.ib,
        msg->tail.ib.rdma.route_table->data.ib.remote, ibuf);

      sctk_ib_rdma_recv_done_local(rail, msg);

      /* Buffer reused: don't need to free it */
      release_ibuf = 0;
      break;

   case rdma_data_read_type:
      sctk_nodebug("Poll send: message RDMA data read received");
      break;

    case rdma_done_type:
      sctk_nodebug("Poll send: message RDMA done received");
      break;
    default:assume(0);
  }

  return release_ibuf;
}


void sctk_ib_rdma_recv_msg_no_recopy(sctk_message_to_copy_t* tmp){ }

#endif
