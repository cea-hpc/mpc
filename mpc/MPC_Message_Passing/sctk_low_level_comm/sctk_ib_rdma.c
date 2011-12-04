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
#include "sctk_checksum.h"

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

/*-----------------------------------------------------------
 *  FREE RECV MESSAGE
 *----------------------------------------------------------*/
void sctk_ib_rdma_net_free_recv(void* arg) {
  sctk_thread_ptp_message_t * msg = (sctk_thread_ptp_message_t*) arg;
  /* Unregister MMU and free message */
  sctk_nodebug("Free MMu for msg: %p", msg);
  assume(msg->tail.ib.rdma.local.mmu_entry);
  sctk_ib_mmu_unregister( &msg->tail.ib.rdma.rail->network.ib,
      msg->tail.ib.rdma.local.mmu_entry);
    sctk_nodebug("FREE: %p", msg);
  sctk_free(msg);
}

/*-----------------------------------------------------------
 *  SEND
 *----------------------------------------------------------*/
void sctk_ib_rdma_prepare_send_msg (sctk_ib_rail_info_t* rail_ib,
    sctk_thread_ptp_message_t * msg, size_t size)
{
  void* aligned_addr = NULL;
  size_t aligned_size = 0;

  /* Allocate memory if not contiguous*/
  if (msg->tail.message_type == sctk_message_contiguous)
  {
    sctk_nodebug("Sending contiguous message; %p (%lu-%lu)", msg->tail.message.contiguous.addr,
        sctk_checksum_message(msg), msg->body.checksum);
    sctk_ib_rdma_align_msg(msg->tail.message.contiguous.addr,
        msg->tail.message.contiguous.size,
        &aligned_addr, &aligned_size);

    msg->tail.ib.rdma.local.addr = msg->tail.message.contiguous.addr;
    msg->tail.ib.rdma.local.size = msg->tail.message.contiguous.size;
    msg->tail.ib.rdma.local.status = zerocopy;
  } else {
    size_t page_size;
    aligned_size = size - sizeof(sctk_thread_ptp_message_body_t);
    page_size = getpagesize();
    /* XXX: NOT FREE for the moment */
    posix_memalign((void**) &aligned_addr, page_size, aligned_size);
    sctk_net_copy_in_buffer(msg, aligned_addr);
    sctk_nodebug("Sending NOT contiguous message %p of size: %lu, add:%p, type:%d (src cs:%lu, dest cs:%lu)", msg, aligned_size, aligned_addr, msg->tail.message_type, msg->body.checksum, sctk_checksum_buffer(aligned_addr,msg));

    msg->tail.ib.rdma.local.addr = aligned_addr;
    msg->tail.ib.rdma.local.size = aligned_size;
    msg->tail.ib.rdma.local.status = recopy;
  }

  msg->tail.ib.rdma.local.mmu_entry =  sctk_ib_mmu_register (
      rail_ib, aligned_addr, aligned_size);
  sctk_nodebug("MMU registered to %p for msg:%p", msg->tail.ib.rdma.local.mmu_entry, msg);
  /* Register MMU */
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
  memcpy(&rdma_req->msg_header, msg, sizeof(sctk_thread_ptp_message_body_t));
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
  rdma_ack->size = msg->tail.ib.rdma.local.size;
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
      src_msg_header->tail.ib.rdma.local.size);
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
  dest_msg_header = src_msg_header->tail.ib.rdma.remote.msg_header;

  /* At this time, we can rewrite informations on the ibuf */
  rdma_done = IBUF_GET_RDMA_DONE(ibuf->buffer);
  rdma_done->dest_msg_header = dest_msg_header;
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_done_type);
  sctk_ibuf_send_init(ibuf, IBUF_GET_RDMA_DONE_SIZE);
  sctk_ibuf_set_protocol(ibuf, rdma_protocol);

  return src_msg_header;
}


static void sctk_ib_rdma_prepare_recv_zerocopy(sctk_rail_info_t* rail, sctk_thread_ptp_message_t* msg) {
  sctk_ib_msg_header_t *send_header;

  send_header = &msg->tail.ib;
  sctk_ib_rdma_align_msg(send_header->rdma.local.addr,
      send_header->rdma.local.size,
      &send_header->rdma.local.aligned_addr,
      &send_header->rdma.local.aligned_size);

}

static void sctk_ib_rdma_prepare_recv_recopy(sctk_rail_info_t* rail, sctk_thread_ptp_message_t* msg) {
  sctk_ib_msg_header_t *send_header;
  sctk_ib_qp_t *remote;
  size_t page_size;

  page_size = getpagesize();
  send_header = &msg->tail.ib;

  /* Allocating memory according to the requested size */
  posix_memalign((void**) &send_header->rdma.local.aligned_addr,
      page_size, send_header->rdma.requested_size );

  send_header->rdma.local.aligned_size  = send_header->rdma.requested_size;
  send_header->rdma.local.size          = send_header->rdma.requested_size;
  send_header->rdma.local.addr          = send_header->rdma.local.aligned_addr;

  sctk_nodebug("Allocating memory (size:%lu,ptr:%p, msg:%p)", send_header->rdma.requested_size, send_header->rdma.local.aligned_addr, msg);
}

static void sctk_ib_rdma_send_ack(sctk_rail_info_t* rail, sctk_thread_ptp_message_t* msg) {
  sctk_ibuf_t *ibuf;
  /*XXX Get page_size from device */
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ib_qp_t *remote;
  sctk_ib_msg_header_t *send_header;

  send_header = &msg->tail.ib;
  /* Register MMU */
  send_header->rdma.local.mmu_entry =  sctk_ib_mmu_register (
      rail_ib, send_header->rdma.local.aligned_addr,
      send_header->rdma.local.aligned_size);
  sctk_nodebug("MMU registered for msg %p", send_header);

  ibuf = sctk_ib_rdma_prepare_ack(rail_ib, msg);

  /* Send message */
  remote = send_header->rdma.route_table->data.ib.remote;
  sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf);
}

void sctk_ib_rdma_net_copy(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  sctk_ib_msg_header_t *send_header;

  send = tmp->msg_send;
  recv = tmp->msg_recv;
  send_header = &send->tail.ib;

  /* The buffer has been posted and if the message is contiguous,
   * we can initiate a RDMA data transfert */
  sctk_spinlock_lock(&send_header->rdma.lock);
  /* If the message has not yet been handled */
  if (send_header->rdma.local.status == not_set) {

    if (recv->tail.message_type == sctk_message_contiguous) {
      send_header->rdma.local.addr  = recv->tail.message.contiguous.addr;
      send_header->rdma.local.size  = recv->tail.message.contiguous.size;
      send_header->rdma.local.status       = zerocopy;
      sctk_nodebug("Zerocopy. (Send:%p Recv:%p)", send, recv);
      sctk_ib_rdma_prepare_recv_zerocopy(send_header->rdma.rail, send);

    } else { /* not contiguous message */
      send_header->rdma.local.status       = recopy;
      sctk_ib_rdma_prepare_recv_recopy(send_header->rdma.rail, send);
      sctk_nodebug("Msg %p recopied from buffer %p", tmp->msg_send, send_header->rdma.local.addr);
    }

    sctk_nodebug("Copy_ptr: %p (free:%p, ptr:%p)", tmp, send->tail.free_memory,
        send_header->rdma.local.addr);
    send_header->rdma.copy_ptr = tmp;
    sctk_ib_rdma_send_ack(send_header->rdma.rail, send);
    /* XXX: Check if the size requested is the size of the message posted */
    // assume(send_header->rdma.requested_size == msg->tail.ib.rdma.size_remote);
  } else {
    assume(0);
    sctk_nodebug("Message recopy");
    sctk_net_message_copy_from_buffer(send_header->rdma.local.addr, tmp, 0);
    sctk_spinlock_unlock(&send_header->rdma.lock);
  }
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

  /* Wait while the message becomes ready */
  sctk_thread_wait_for_value((int*) &src_msg_header->tail.ib.rdma.local.ready, 1);

  src_msg_header->tail.ib.rdma.remote.addr = rdma_ack->addr;
  src_msg_header->tail.ib.rdma.remote.size = rdma_ack->size;
  src_msg_header->tail.ib.rdma.remote.rkey = rdma_ack->rkey;
  src_msg_header->tail.ib.rdma.remote.msg_header = dest_msg_header;

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
  sctk_nodebug("Checksum:%lu", msg->body.checksum);

  /* We reinit header before calculating the source */
  sctk_rebuild_header(msg);
  sctk_reinit_header(msg, sctk_ib_rdma_net_free_recv, sctk_ib_rdma_net_copy);
  msg->tail.ib.protocol = rdma_protocol;
  /* determine the route to the source */
  route = sctk_get_route(msg->sctk_msg_get_glob_source, rail);

  msg->body.completion_flag = NULL;
  msg->tail.message_type = sctk_message_network;
  /* Remote addr initially set to NULL */
  msg->tail.ib.rdma.lock        = SCTK_SPINLOCK_INITIALIZER;
  msg->tail.ib.rdma.local.status      = not_set;
  msg->tail.ib.rdma.requested_size  = rdma_req->requested_size;
  msg->tail.ib.rdma.rail        = rail;
  msg->tail.ib.rdma.route_table = route;
  msg->tail.ib.rdma.local.addr  = NULL;
  msg->tail.ib.rdma.remote.msg_header = rdma_req->dest_msg_header;

  /* Send message to MPC. The message can be matched at the end
   * of function. */
  rail->send_message_from_network(msg);
  /* XXX: Deal with the fact that buffer not been posted */
#if 0
  sctk_spinlock_lock(&msg->tail.ib.rdma.lock);
  status = msg->tail.ib.rdma.local.status;
  if (status == not_set)
  {
    sctk_nodebug("NOT zero-copy msg");
    sctk_ib_rdma_prepare_recv_recopy(rail, msg);
    sctk_ib_rdma_send_ack(rail, msg);
    msg->tail.ib.rdma.local.status = recopy;
  }
  sctk_spinlock_unlock(&msg->tail.ib.rdma.lock);
#endif

  sctk_nodebug("End send_message (matching:%p, stats:%d, "
      "requested_size:%lu, required_size:%lu)",
    msg->tail.ib.rdma.local.addr, msg->tail.ib.rdma.local.status,
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


  /* If recopy, we delete the temporary msg copy */
  sctk_nodebug("MSG DONE REMOTE %p ", dest_msg_header);
  if (dest_msg_header->tail.ib.rdma.local.status == recopy) {
  sctk_nodebug("MSG with addr %p completed, recopy to %p (checksum:%lu)", send->tail.ib.rdma.local.addr,
      recv->tail.message.contiguous.addr,
      sctk_checksum_buffer(dest_msg_header->tail.ib.rdma.local.addr,dest_msg_header->tail.ib.rdma.copy_ptr->msg_recv));
    sctk_net_message_copy_from_buffer(dest_msg_header->tail.ib.rdma.local.addr,
        dest_msg_header->tail.ib.rdma.copy_ptr , 0);
    sctk_nodebug("FREE: %p", dest_msg_header->tail.ib.rdma.local.addr);
    /* If we recopy, we can delete the temp buffer */
    sctk_free(dest_msg_header->tail.ib.rdma.local.addr);
   }
  sctk_message_completion_and_free(send,recv);

  return NULL;
}

/*
 * RECV DONE LOCAL
 */
static inline void
sctk_ib_rdma_recv_done_local(sctk_rail_info_t* rail, sctk_thread_ptp_message_t* msg)
{
  assume(msg->tail.ib.rdma.local.mmu_entry);
  sctk_ib_mmu_unregister( &msg->tail.ib.rdma.rail->network.ib,
    msg->tail.ib.rdma.local.mmu_entry);

  if (msg->tail.ib.rdma.local.status == recopy){
    /* Unregister MMU and free message */
    sctk_nodebug("FREE PTR: %p", msg->tail.ib.rdma.local.addr);
    sctk_free(msg->tail.ib.rdma.local.addr);
  }

  sctk_nodebug("MSG LOCAL FREE %p", msg);
  sctk_complete_and_free_message(msg);
}

/*-----------------------------------------------------------
 *  POLLING SEND / RECV
 *----------------------------------------------------------*/
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
      sctk_ib_qp_send_ibuf(&rail->network.ib,
      header->tail.ib.rdma.route_table->data.ib.remote,  ibuf);
      return 0;
      break;

    case rdma_done_type:
      sctk_nodebug("Poll recv: message RDMA done received");
      sctk_ib_rdma_recv_done_remote(rail, ibuf);
      return 1;
      break;

    default:not_reachable();
  }
  not_reachable();
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
      sctk_nodebug("Checksum after write:%lu", sctk_checksum_message(msg));
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

/*-----------------------------------------------------------
 *  PRINT
 *----------------------------------------------------------*/
void sctk_ib_rdma_print(sctk_thread_ptp_message_t* msg) {
  sctk_ib_msg_header_t* h = &msg->tail.ib;

  sctk_error("MSG INFOS\n"
      "requested_size: %d\n"
      "rail: %p\n"
      "route_table: %p\n"
      "copy_ptr: %p\n"
      "--- LOCAL ---\n"
      "status: %d\n"
      "mmu_entry: %p\n"
      "addr: %p\n"
      "size: %lu\n"
      "aligned_addr: %p\n"
      "aligned_size: %lu\n"
      "ready: %d\n"
      "--- REMOTE ---\n"
      "msg_header: %p\n"
      "addr: %p\n"
      "size: %lu\n"
      "rkey: %u\n"
      "aligned_addr: %p\n"
      "aligned_size: %lu\n",
    h->rdma.requested_size,
    h->rdma.rail,
    h->rdma.route_table,
    h->rdma.copy_ptr,
    h->rdma.local.status,
    h->rdma.local.mmu_entry,
    h->rdma.local.addr,
    h->rdma.local.size,
    h->rdma.local.aligned_addr,
    h->rdma.local.aligned_size,
    h->rdma.local.ready,
    h->rdma.remote.msg_header,
    h->rdma.remote.addr,
    h->rdma.remote.size,
    h->rdma.remote.rkey,
    h->rdma.remote.aligned_addr,
    h->rdma.remote.aligned_size);
}

#endif
