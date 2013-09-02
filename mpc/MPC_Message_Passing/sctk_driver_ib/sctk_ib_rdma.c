/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include "sctk_ib.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_ib_mmu.h"
#include "sctk_net_tools.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
//#define SCTK_IB_MODULE_DEBUG
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
  /* We do not need to align pointers by hand */

  *aligned_addr = addr;
  *aligned_size = size;
#if 0
  size_t page_size;
 /*XXX Get page_size from device */
  page_size = getpagesize();

  /* align on a page size */
  *aligned_addr = addr -
    (((unsigned long) addr) % page_size);
  *aligned_size = size +
    (((unsigned long) addr) % page_size);

  sctk_nodebug("ptr:%p size:%lu -> ptr:%p size:%lu", addr, size,
      *aligned_addr, *aligned_size);
#endif
}

/*-----------------------------------------------------------
 *  FREE RECV MESSAGE
 *----------------------------------------------------------*/
void sctk_ib_rdma_net_free_recv(void* arg) {
  sctk_thread_ptp_message_t * msg = (sctk_thread_ptp_message_t*) arg;
  sctk_rail_info_t* rail = msg->tail.ib.rdma.rail;
  /* Unregister MMU and free message */
  sctk_nodebug("Free MMu for msg: %p", msg);
  assume(msg->tail.ib.rdma.local.mmu_entry);
  sctk_ib_mmu_unregister( &rail->network.ib,
      msg->tail.ib.rdma.local.mmu_entry);
    sctk_nodebug("FREE: %p", msg);
  sctk_free(msg);
  PROF_INC(rail, free_mem);
}

/*-----------------------------------------------------------
 *  SEND
 *----------------------------------------------------------*/
void sctk_ib_rdma_prepare_send_msg (sctk_ib_rail_info_t* rail_ib,
    sctk_thread_ptp_message_t * msg, size_t size)
{
  sctk_ib_header_rdma_t * rdma = &msg->tail.ib.rdma;
  void* aligned_addr = NULL;
  size_t aligned_size = 0;

  /* Do not allocate memory if contiguous message */
  if (msg->tail.message_type == sctk_message_contiguous) {

    sctk_ib_rdma_align_msg(msg->tail.message.contiguous.addr,
        msg->tail.message.contiguous.size,
        &aligned_addr, &aligned_size);

    rdma->local.addr = msg->tail.message.contiguous.addr;
    rdma->local.size = msg->tail.message.contiguous.size;
    rdma->local.status = zerocopy;
  } else {
    size_t page_size;
    aligned_size = size - sizeof(sctk_thread_ptp_message_body_t);
    page_size = getpagesize();

    posix_memalign((void**) &aligned_addr, page_size, aligned_size);
    PROF_INC_RAIL_IB(rail_ib, alloc_mem);
    sctk_net_copy_in_buffer(msg, aligned_addr);

    sctk_nodebug("Sending NOT contiguous message %p of size: %lu, add:%p, type:%d (src cs:%lu, dest cs:%lu)", msg, aligned_size, aligned_addr, msg->tail.message_type, msg->body.checksum, sctk_checksum_buffer(aligned_addr,msg));

    rdma->local.addr = aligned_addr;
    rdma->local.size = aligned_size;
    rdma->local.status = recopy;
  }

  /* Register MMU */
  rdma->local.mmu_entry =  sctk_ib_mmu_register (
      rail_ib, aligned_addr, aligned_size);

  /* Save addr and size */
  rdma->local.aligned_addr = aligned_addr;
  rdma->local.aligned_size = aligned_size;
  rdma->glob_source = msg->sctk_msg_get_glob_source;
  rdma->glob_destination = msg->sctk_msg_get_glob_destination;
  rdma->local.msg_header = msg;
  /* Message is now ready ready */
  rdma->local.ready = 1;
}

/*
 * SEND REQUEST
 */
sctk_ibuf_t* sctk_ib_rdma_prepare_req(sctk_rail_info_t* rail,
    sctk_ib_qp_t *remote, sctk_thread_ptp_message_t * msg, size_t size, int low_memory_mode) {
  LOAD_RAIL(rail);
  sctk_ib_header_rdma_t * rdma = &msg->tail.ib.rdma;
  sctk_ibuf_t *ibuf;
  sctk_ib_rdma_t *rdma_header;
  sctk_ib_rdma_req_t *rdma_req;
  size_t ibuf_size = IBUF_GET_RDMA_REQ_SIZE;

  ibuf = sctk_ibuf_pick_send(rail_ib, remote, &ibuf_size,
      task_node_number);
  assume(ibuf);
  IBUF_SET_DEST_TASK(ibuf->buffer, msg->sctk_msg_get_glob_destination);
  IBUF_SET_SRC_TASK(ibuf, msg->sctk_msg_get_glob_source);
  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_req_type);

  /* Get the buffer */
  rdma_req    = IBUF_GET_RDMA_REQ(ibuf->buffer);

  /* Initialization of the msg header */
  memcpy(&rdma_req->msg_header, msg, sizeof(sctk_thread_ptp_message_body_t));
  /* Message not ready: memory not pinned */
  rdma->local.ready = 0;
  rdma->rail = rail;
  rdma->remote_peer = remote;

  /* Initialization of the request */
  rdma_req->requested_size = size - sizeof(sctk_thread_ptp_message_body_t);
  rdma_req->dest_msg_header = msg;
  /* Register the type of the message */
  rdma_req->message_type = msg->tail.message_type;
  rdma_req->msg_number = rdma->ht_msg_number;

  /* Initialization of the buffer */
  IBUF_SET_PROTOCOL(ibuf->buffer, rdma_protocol);

  sctk_nodebug("Request size: %d", IBUF_GET_RDMA_REQ_SIZE);
  sctk_nodebug("Req sent (size:%lu, requested:%d, ibuf:%p)", IBUF_GET_RDMA_REQ_SIZE,
      rdma_req->requested_size, ibuf);

  return ibuf;
}

/*
 * SEND RDMA ACK
 */
static inline sctk_ibuf_t* sctk_ib_rdma_prepare_ack(sctk_rail_info_t* rail,
  sctk_thread_ptp_message_t* msg) {
  sctk_ib_header_rdma_t * rdma = &msg->tail.ib.rdma;
  LOAD_RAIL(rail);
  sctk_ibuf_t *ibuf;
  sctk_ib_rdma_t *rdma_header;
  sctk_ib_rdma_ack_t *rdma_ack;
  size_t ibuf_size = IBUF_GET_RDMA_ACK_SIZE;

  ibuf = sctk_ibuf_pick_send(rail_ib, rdma->remote_peer, &ibuf_size,
      task_node_number);
  assume(ibuf);
  IBUF_SET_DEST_TASK(ibuf->buffer, msg->tail.ib.rdma.glob_destination);
  IBUF_SET_SRC_TASK(ibuf, msg->tail.ib.rdma.glob_source);
  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);

  rdma_ack    = IBUF_GET_RDMA_ACK(ibuf->buffer);

  rdma_ack->addr = rdma->local.addr;
  rdma_ack->size = rdma->local.size;
  rdma_ack->rkey = rdma->local.mmu_entry->mr->rkey;
  rdma_ack->dest_msg_header = rdma->remote.msg_header;
  rdma_ack->src_msg_header = msg;

  /* Initialization of the buffer */
  IBUF_SET_PROTOCOL(ibuf->buffer, rdma_protocol);
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_ack_type);

  sctk_nodebug("Send RDMA ACK message: %lu %u", rdma_ack->addr, rdma_ack->rkey);
  return ibuf;
}

/*
 * SEND RDMA DATA
 */
inline void
sctk_ib_rdma_prepare_data_write(sctk_rail_info_t* rail,
    sctk_thread_ptp_message_t *src_msg_header) {
  LOAD_RAIL(rail);
  sctk_ib_header_rdma_t * rdma = &src_msg_header->tail.ib.rdma;
  sctk_ib_rdma_data_write_t *rdma_data_write;
  sctk_ib_rdma_t *rdma_header;
  sctk_ibuf_t* ibuf;

  ibuf = sctk_ibuf_pick_send_sr(rail_ib, task_node_number);
  assume(ibuf);

  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  rdma_data_write = IBUF_GET_RDMA_DATA_WRITE(ibuf->buffer);
  rdma_data_write->src_msg_header = src_msg_header;

  IBUF_SET_DEST_TASK(ibuf->buffer, rdma->glob_destination);
  IBUF_SET_SRC_TASK(ibuf, rdma->glob_source);
  IBUF_SET_PROTOCOL(ibuf->buffer, rdma_protocol);
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_data_write_type);

  sctk_nodebug("Write from %p (%lu) to %p (%lu)",
      rdma->local.addr,
      rdma->local.size,
      rdma->remote.addr,
      rdma->remote.size);

  sctk_ibuf_rdma_write_init(ibuf,
      rdma->local.addr,
      rdma->local.mmu_entry->mr->lkey,
      rdma->remote.addr,
      rdma->remote.rkey,
      rdma->local.size,
      IBV_SEND_SIGNALED, IBUF_RELEASE);

  sctk_ib_qp_send_ibuf(rail_ib,
      rdma->remote_peer, ibuf, 0);
}

/*
 * SEND RDMA DONE
 */
static inline sctk_thread_ptp_message_t*
sctk_ib_rdma_prepare_done_write(sctk_rail_info_t* rail, sctk_ibuf_t *incoming_ibuf) {
  LOAD_RAIL(rail);
  sctk_ib_header_rdma_t * rdma;
  sctk_thread_ptp_message_t *src_msg_header;
  sctk_thread_ptp_message_t *dest_msg_header;
  sctk_ib_rdma_t *rdma_header;
  sctk_ib_rdma_done_t *rdma_done;
  sctk_ib_rdma_data_write_t *rdma_data_write;
  sctk_ibuf_t *ibuf;
  size_t ibuf_size = IBUF_GET_RDMA_DONE_SIZE;


  rdma_header = IBUF_GET_RDMA_HEADER(incoming_ibuf->buffer);
  rdma_data_write = IBUF_GET_RDMA_DATA_WRITE(incoming_ibuf->buffer);
  src_msg_header = rdma_data_write->src_msg_header;
  rdma = &src_msg_header->tail.ib.rdma;
  dest_msg_header = rdma->remote.msg_header;

  /* Initialize & send the buffer */
  ibuf = sctk_ibuf_pick_send(rail_ib, rdma->remote_peer, &ibuf_size,
      task_node_number);
  assume(ibuf);
  rdma_done = IBUF_GET_RDMA_DONE(ibuf->buffer);
  rdma_done->dest_msg_header = dest_msg_header;
  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  IBUF_SET_DEST_TASK(ibuf->buffer, rdma->glob_destination);
  IBUF_SET_SRC_TASK(ibuf, rdma->glob_source);

  IBUF_SET_PROTOCOL(ibuf->buffer, rdma_protocol);
  IBUF_SET_RDMA_TYPE(rdma_header, rdma_done_type);

  sctk_ib_qp_send_ibuf(rail_ib, rdma->remote_peer, ibuf, 0);

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
  size_t page_size;

  page_size = getpagesize();
  send_header = &msg->tail.ib;

  /* Allocating memory according to the requested size */
  posix_memalign((void**) &send_header->rdma.local.aligned_addr,
      page_size, send_header->rdma.requested_size );
  PROF_INC(rail, alloc_mem);

  send_header->rdma.local.aligned_size  = send_header->rdma.requested_size;
  send_header->rdma.local.size          = send_header->rdma.requested_size;
  send_header->rdma.local.addr          = send_header->rdma.local.aligned_addr;

  sctk_nodebug("Allocating memory (size:%lu,ptr:%p, msg:%p)", send_header->rdma.requested_size, send_header->rdma.local.aligned_addr, msg);
}

static void sctk_ib_rdma_send_ack(sctk_rail_info_t* rail, sctk_thread_ptp_message_t* msg) {
  sctk_ibuf_t *ibuf;
  LOAD_RAIL(rail);
  sctk_ib_qp_t *remote;
  sctk_ib_msg_header_t *send_header;

  send_header = &msg->tail.ib;
  /* Register MMU */
  send_header->rdma.local.mmu_entry =  sctk_ib_mmu_register (
      rail_ib, send_header->rdma.local.aligned_addr,
      send_header->rdma.local.aligned_size);
  sctk_nodebug("MMU registered for msg %p", send_header);

  ibuf = sctk_ib_rdma_prepare_ack(rail, msg);

  /* Send message */
  remote = send_header->rdma.remote_peer;
  sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf, 0);
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

      /* XXX: Check if the size requested is the size of the message posted */
      assume(send_header->rdma.requested_size == recv->tail.message.contiguous.size);

      send_header->rdma.local.addr  = recv->tail.message.contiguous.addr;
      send_header->rdma.local.size  = recv->tail.message.contiguous.size;
      send_header->rdma.local.status       = zerocopy;
      sctk_nodebug("Zerocopy. (Send:%p Recv:%p)", send, recv);
      sctk_ib_rdma_prepare_recv_zerocopy(send_header->rdma.rail, send);
      sctk_ib_rdma_send_ack(send_header->rdma.rail, send);
      send_header->rdma.copy_ptr = tmp;
      send_header->rdma.local.ready = 1;
    } else { /* not contiguous message */
      send_header->rdma.local.status       = recopy;
      sctk_ib_rdma_prepare_recv_recopy(send_header->rdma.rail, send);
      sctk_nodebug("Msg %p recopied from buffer %p", tmp->msg_send, send_header->rdma.local.addr);
      send_header->rdma.copy_ptr = tmp;
      sctk_ib_rdma_send_ack(send_header->rdma.rail, send);
      send_header->rdma.local.ready = 1;
    }

    sctk_nodebug("Copy_ptr: %p (free:%p, ptr:%p)", tmp, send->tail.free_memory,
        send_header->rdma.local.addr);

  } else if (send_header->rdma.local.status == recopy) {
      send_header->rdma.copy_ptr = tmp;
      send_header->rdma.local.ready = 1;
  } else not_reachable();
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
  sctk_ib_header_rdma_t *rdma;

  /* Save the values of the ack because the buffer will be reused */
  rdma_ack = IBUF_GET_RDMA_ACK(ibuf->buffer);
  src_msg_header  = rdma_ack->dest_msg_header;
  rdma = &src_msg_header->tail.ib.rdma;
  dest_msg_header = rdma_ack->src_msg_header;

  /* Wait while the message becomes ready */
  sctk_thread_wait_for_value((int*) &rdma->local.ready, 1);

  sctk_nodebug("Remote addr: %p", rdma_ack->addr);
  rdma->remote.addr = rdma_ack->addr;
  rdma->remote.size = rdma_ack->size;
  rdma->remote.rkey = rdma_ack->rkey;
  rdma->remote.msg_header = dest_msg_header;

  return src_msg_header;
}

/*
 * RECV REQ
 */
static inline sctk_thread_ptp_message_t *
sctk_ib_rdma_recv_req(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_ib_header_rdma_t *rdma;
  sctk_thread_ptp_message_t *msg;
  sctk_route_table_t* route;
  sctk_ib_rdma_req_t *rdma_req = IBUF_GET_RDMA_REQ(ibuf->buffer);
  int src_process;

  msg = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
  PROF_INC(rail, alloc_mem);
  memcpy(&msg->body, &rdma_req->msg_header, sizeof(sctk_thread_ptp_message_body_t));

  /* We reinit header before calculating the source */
  sctk_rebuild_header(msg);
  sctk_reinit_header(msg, sctk_ib_rdma_net_free_recv, sctk_ib_rdma_net_copy);
  msg->tail.ib.protocol = rdma_protocol;
  rdma = &msg->tail.ib.rdma;

  src_process = sctk_determine_src_process_from_header(&msg->body);
  assume(src_process != -1);
  route = sctk_get_route_to_process(src_process, rail);
  assume(route);

  msg->body.completion_flag = NULL;
  msg->tail.message_type = sctk_message_network;
  /* Remote addr initially set to NULL */
  rdma->lock        = SCTK_SPINLOCK_INITIALIZER;
  rdma->local.status      = not_set;
  rdma->requested_size  = rdma_req->requested_size;
  rdma->rail        = rail;
  rdma->remote_peer = route->data.ib.remote;
  rdma->local.addr  = NULL;
  rdma->remote.msg_header = rdma_req->dest_msg_header;
  rdma->local.msg_header = msg;
  rdma->local.ready = 0;
  rdma->ht_msg_number = rdma_req->msg_number;
  /* XXX: Only for collaborative polling */
  rdma->glob_source = IBUF_GET_SRC_TASK(ibuf);
  rdma->glob_destination = IBUF_GET_DEST_TASK(ibuf->buffer);

  /* Send message to MPC. The message can be matched at the end
   * of function. */
  rail->send_message_from_network(msg);

  sctk_nodebug("End send_message (matching:%p, stats:%d, "
      "requested_size:%lu, required_size:%lu)",
    rdma->local.addr, rdma->local.status,
    rdma_req->requested_size, rdma->local.size);

  return msg;
}

/*
 * RECV DONE REMOTE
 */
static inline sctk_thread_ptp_message_t *
sctk_ib_rdma_recv_done_remote(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_ib_rdma_done_t *rdma_done;
  sctk_ib_header_rdma_t *rdma;
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  sctk_thread_ptp_message_t *dest_msg_header;

  /* Save the values of the ack because the buffer will be reused */
  rdma_done = IBUF_GET_RDMA_DONE(ibuf->buffer);
  dest_msg_header = rdma_done->dest_msg_header;
  rdma = &dest_msg_header->tail.ib.rdma;

  sctk_thread_wait_for_value((int*) &dest_msg_header->tail.ib.rdma.local.ready, 1);

  send = rdma->copy_ptr->msg_send;
  recv = rdma->copy_ptr->msg_recv;

  sctk_nodebug("msg: %p - Rail: %p (%p-%p) copy_ptr:%p (send:%p recv:%p)", dest_msg_header, rail, ibuf, rdma_done, rdma->copy_ptr, send, recv);

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
    PROF_INC(rail, free_mem);
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
    PROF_INC(rail, free_mem);
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

  IBUF_CHECK_POISON(ibuf->buffer);
  rdma_header = IBUF_GET_RDMA_HEADER(ibuf->buffer);
  /* Switch on the RDMA type of message */
  switch(IBUF_GET_RDMA_TYPE(rdma_header)) {
    case rdma_req_type:
      sctk_nodebug("Poll recv: message RDMA req received");
      sctk_ib_rdma_recv_req(rail, ibuf);
      return 1;
      break;

    case rdma_ack_type:
      sctk_nodebug("Poll recv: message RDMA ack received");
      /* Buffer reused: don't need to free it */
      header = sctk_ib_rdma_recv_ack(rail, ibuf);
      sctk_ib_rdma_prepare_data_write(rail, header);
      return 1;
      break;

    case rdma_done_type:
      sctk_nodebug("Poll recv: message RDMA done received");
      sctk_ib_rdma_recv_done_remote(rail, ibuf);
      return 1;
      break;

    default:
      sctk_error("RDMA type %d not found!!!", IBUF_GET_RDMA_TYPE(rdma_header));
      not_reachable();
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

  IBUF_CHECK_POISON(ibuf->buffer);
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
      sctk_ib_rdma_recv_done_local(rail, msg);
      break;

   case rdma_data_read_type:
      sctk_nodebug("Poll send: message RDMA data read received");
      break;

    case rdma_done_type:
      sctk_nodebug("Poll send: message RDMA done received");
      break;
    default:
      sctk_error("Got RDMA type:%d, payload_size;%lu", IBUF_GET_RDMA_TYPE(rdma_header), rdma_header->payload_size);
      char ibuf_desc[4096];
      sctk_error("BEGIN ERROR");
      sctk_ibuf_print(ibuf, ibuf_desc);
      sctk_error ("\033[1;31m\nIB - FATAL ERROR FROM PROCESS %d\n"
          "\033[1;31m######### IBUF DESC ############\033[0m\n"
          "%s\n"
          "\033[1;31m################################\033[0m",
          sctk_process_rank, ibuf_desc);
      sctk_error("END ERROR");
//      not_reachable();
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
      "remote_peer: %p\n"
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
    h->rdma.remote_peer,
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
