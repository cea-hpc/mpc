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
#include "sctk_net_tools.h"
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "RDMA"
#include "sctk_ib_toolkit.h"

#define HOSTNAME 2048

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
#define IBUF_GET_RDMA_TYPE(x) ((x)->type)
#define IBUF_SET_RDMA_TYPE(x, __type) ((x)->type = __type)

static inline void sctk_ib_rdma_align_msg(const void *addr_remote, const size_t  size_remote,
    void **aligned_addr_remote, size_t *aligned_size_remote)
{
 /*XXX Get page_size from device */
  size_t page_size;

  page_size = getpagesize();

  /* align on a page size */
  *aligned_addr_remote = addr_remote -
    (((unsigned long) addr_remote) % page_size);
  *aligned_size_remote = size_remote +
    (((unsigned long) addr_remote) % page_size);

  sctk_debug("ptr:%p size:%lu -> ptr:%p size:%lu", addr_remote, size_remote,
      *aligned_addr_remote, *aligned_size_remote);
}

sctk_ibuf_t* sctk_ib_rdma_prepare_msg(sctk_ib_rail_info_t* rail_ib,
    sctk_ib_qp_t* route_data, sctk_thread_ptp_message_t * msg, size_t size) {
  sctk_ibuf_t *ibuf;
  sctk_ib_rdma_t *rdma_header;
  sctk_ib_rdma_req_t *rdma_req;

  ibuf = sctk_ibuf_pick(rail_ib, 1, 0);
  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_req_type);

  rdma_req    = IBUF_GET_RDMA_REQ(ibuf->buffer);
  rdma_req->requested_size = size - sizeof(sctk_thread_ptp_message_body_t);

  /* Only copy the header to the buffer */
  memcpy(&rdma_req->msg_header, msg, sizeof(sctk_thread_ptp_message_body_t));

  /* Initialization of the buffer */
  sctk_ibuf_send_init(ibuf, IBUF_GET_RDMA_REQ_SIZE);
  sctk_ibuf_set_protocol(ibuf, rdma_protocol);

  sctk_debug("Req sent (size:%lu, requested:%d, ibuf:%p)", IBUF_GET_RDMA_REQ_SIZE,
      rdma_req->requested_size, ibuf);


#if 0
  sctk_ib_sr_t *sr_header;


  /* Copy header */
  memcpy(IBUF_MSG_HEADER(ibuf->buffer), msg, sizeof(sctk_thread_ptp_message_body_t));
  /* Copy payload */
  sctk_net_copy_in_buffer(msg, IBUF_MSG_PAYLOAD(ibuf->buffer));

  /* Initialization of the buffer */
  sctk_ibuf_send_init(ibuf, size);

  sr_header = IBUF_SR_HEADER(ibuf->buffer);
  sr_header->protocol = rdma_protocol;
  /* Size without msg header*/
  sr_header->eager.payload_size = size - sizeof(sctk_thread_ptp_message_body_t);
#endif

  return ibuf;
}

void sctk_ib_rdma_free_msg_no_recopy(sctk_thread_ptp_message_t * msg) {

}

void sctk_ib_rdma_net_free(sctk_thread_ptp_message_t * msg) {

}

void sctk_ib_rdma_net_copy(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  int recopy;
  sctk_ibuf_t *ibuf;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  /* The buffer has been posted and if the message is contiguous,
   * we can initiate a RDMA data transfert */
  if (recv->tail.message_type == sctk_message_contiguous)
  {
    send->tail.ib.rdma.addr_remote = recv;
    send->tail.ib.rdma.size_remote = recv->tail.message.contiguous.size;
  }

  sctk_debug("MATCHED %p", recv);
}

static inline sctk_thread_ptp_message_t *
sctk_ib_rdma_recv_req(sctk_rail_info_t* rail, sctk_ib_rdma_req_t *rdma_req) {
  sctk_thread_ptp_message_t *msg;
  sctk_ib_rdma_status_t status;

  msg = sctk_malloc(sizeof(sctk_thread_ptp_message_body_t));
  memcpy(msg, &rdma_req->msg_header, sizeof(sctk_thread_ptp_message_body_t));

  msg->body.completion_flag = NULL;
  msg->tail.message_type = sctk_message_network;
  /* Remote addr initially set to NULL */
  msg->tail.ib.rdma.addr_remote = NULL;
  msg->tail.ib.rdma.lock        = SCTK_SPINLOCK_INITIALIZER;
  msg->tail.ib.rdma.status      = not_set;

  sctk_rebuild_header(msg);
  sctk_reinit_header(msg, sctk_ib_rdma_net_free, sctk_ib_rdma_net_copy);

  rail->send_message_from_network(msg);
  /* Set the msg as a zero-copy msg*/
  sctk_spinlock_lock(&msg->tail.ib.rdma.lock);
  if (msg->tail.ib.rdma.status == not_set) {
    status = msg->tail.ib.rdma.status = zerocopy;
  }
  sctk_spinlock_unlock(&msg->tail.ib.rdma.lock);

  sctk_debug("End send_message (matching:%p, stats:%d, "
      "requested_size:%lu, required_size:%lu)",
    msg->tail.ib.rdma.addr_remote, msg->tail.ib.rdma.status,
    rdma_req->requested_size, msg->tail.ib.rdma.size_remote);

  /* Zerocopy asked */
  if (status == zerocopy)
  {
    /* Check if the size requested is the size of the message posted */
    assume(rdma_req->requested_size == msg->tail.ib.rdma.size_remote);

    sctk_ib_rdma_align_msg(msg->tail.ib.rdma.addr_remote,
      msg->tail.ib.rdma.size_remote,
      &msg->tail.ib.rdma.aligned_addr_remote,
      &msg->tail.ib.rdma.aligned_size_remote);
  }
  return msg;
}

sctk_thread_ptp_message_t *
sctk_ib_rdma_recv(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_ib_rdma_t *rdma_header;

  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  /* Switch on the RDMA type of message */
  switch(IBUF_GET_RDMA_TYPE(rdma_header)) {
    case rdma_req_type:
      sctk_debug("Message RDMA req received");
      return sctk_ib_rdma_recv_req(rail, IBUF_GET_RDMA_REQ(ibuf->buffer));
      break;

    case rdma_ack_type:
      sctk_debug("Message RDMA ack received");
      break;

    case rdma_done_type:
      sctk_debug("Message RDMA done received");
      break;
    default:assume(0);
  }

  return NULL;
}

void sctk_ib_rdma_recv_msg_no_recopy(sctk_message_to_copy_t* tmp){
#if 0
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  int recopy;
  sctk_ibuf_t *ibuf;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  /* Assume msg not recopies */
  assume(!send->tail.ib.eager.recopied);

  ibuf = send->tail.ib.eager.ibuf;
  assume(ibuf);
  sctk_net_message_copy_from_buffer(tmp, IBUF_MSG_PAYLOAD(ibuf->buffer));
#endif
}

#endif
