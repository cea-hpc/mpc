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


#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_lib.h"
#include "sctk_infiniband_ibufs.h"
#include "sctk_infiniband_config.h"
#include "sctk_alloc.h"
#include "sctk_buffered_fifo.h"
#include "sctk_inter_thread_comm.h"
#include "sctk_infiniband_profiler.h"
#include "sctk.h"

#define WAIT_READY(x) \
  while(!x->ready) sctk_thread_yield()

/* channel selection */
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
extern struct sctk_list broadcast_fifo;
struct sctk_list  reduce_fifo;

/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
  sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_rdma_create_local(sctk_net_ibv_qp_rail_t* rail)
{
  sctk_net_ibv_qp_local_t* local;

  local = sctk_net_ibv_qp_new(rail);

  sctk_net_ibv_pd_init(local);
  sctk_nodebug("New PD %p", local->pd);

  /* completion queues */
  local->send_cq = sctk_net_ibv_cq_init(rail);
  local->recv_cq = sctk_net_ibv_cq_init(rail);

  return local;
}


sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_allocate_init(
    unsigned int rank,
    sctk_net_ibv_qp_local_t *local)
{
  sctk_net_ibv_rc_rdma_process_t* rdma;

  rdma = sctk_malloc(sizeof(sctk_net_ibv_rc_rdma_process_t));
  assume(rdma);

  sctk_thread_mutex_init(&rdma->lock, NULL);
  sctk_nodebug("SEND");
  sctk_list_new(&(rdma->send), 0, 0);
  sctk_nodebug("RECV");
  sctk_list_new(&(rdma->recv), 0, 0);

  rdma->ready = 0;

  return rdma;
}

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void
sctk_net_ibv_comp_rc_rdma_send_coll_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_ibuf_msg_type_t type)
{
  sctk_net_ibv_ibuf_t* ibuf;
  size_t page_size;
  sctk_net_ibv_rc_rdma_coll_request_t* request;
  sctk_net_ibv_mmu_entry_t* mmu_entry = NULL;
  sctk_net_ibv_qp_remote_t* rc_sr_remote;
  size_t size_to_copy;

  void *aligned_msg;
  size_t aligned_size;

  /* list entries */
  sctk_net_ibv_rc_rdma_entry_t *entry;

  ibuf = sctk_net_ibv_ibuf_pick(0, 1);
  request = (sctk_net_ibv_rc_rdma_coll_request_t* ) RC_SR_PAYLOAD(ibuf->buffer);

  page_size = sctk_net_ibv_mmu_get_pagesize();
  aligned_size = size;
  sctk_posix_memalign(&aligned_msg, page_size, size);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  memcpy(aligned_msg, msg, size);

//  offset = (((unsigned long) msg) % page_size);
//  aligned_msg =  msg - offset;
//  aligned_size = size + offset;

  /* we register the memory where the message is stored, whatever the
   * RDVZ protocol used */
  mmu_entry = sctk_net_ibv_mmu_register (
      rail, local_rc_sr, aligned_msg, aligned_size);

  /* check if the connection is opened. If not, we connect processes */
  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);

  size_to_copy = sizeof(sctk_net_ibv_rc_rdma_coll_request_t);
  switch (ibv_rdvz_protocol)
  {
    case IBV_RDVZ_READ_PROTOCOL:
      sctk_nodebug("Initialize READ request");

      entry = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
      assume(entry);
      sctk_ibv_profiler_inc(IBV_MEM_TRACE);
      entry->ready = 0;
      /* add entry to the list */
      sctk_list_lock(&entry_rc_rdma->recv);
      entry->list_elem = sctk_list_push(&entry_rc_rdma->recv, entry);
      sctk_list_unlock(&entry_rc_rdma->recv);

      request->protocol = RC_RDMA_REQ_READ;

      request->read_req.address = (uintptr_t) aligned_msg;
      request->read_req.rkey  = mmu_entry->mr->rkey;
      request->entry_rc_rdma  = entry_rc_rdma;
      request->entry          = entry;
      request->msg_type       = type;
      sctk_nodebug("Send entry 1: %p", entry);
      request->requested_size = size;
      request->src_process    = sctk_process_rank;

      sctk_nodebug("%p->%p (%lu->%lu)", msg, aligned_msg, size, aligned_size);
      sctk_net_ibv_ibuf_send_init(ibuf, size_to_copy + RC_SR_HEADER_SIZE);
      sctk_net_ibv_comp_rc_sr_send(
          rc_sr_remote, -1, ibuf, size_to_copy, size_to_copy, RC_SR_RDVZ_REQUEST, NULL, 1, 1);

      entry->mmu_entry = mmu_entry;
      entry->msg_header_ptr = aligned_msg;
      entry->requested_size = size;
      entry->src_process = dest_process;
      entry->entry_rc_rdma  = entry_rc_rdma;
      entry->msg_type  = type;

      entry->msg_payload_aligned_ptr = aligned_msg;
      entry->ready = 1;

      sctk_nodebug("PSN: %lu", request->psn);
      break;

    case IBV_RDVZ_WRITE_PROTOCOL:
      sctk_nodebug("Initialize WRITE request");
      entry = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
      assume(entry);
      sctk_ibv_profiler_inc(IBV_MEM_TRACE);

      sctk_list_lock(&entry_rc_rdma->recv);
      entry->list_elem = sctk_list_push(&entry_rc_rdma->recv, entry);
      sctk_list_unlock(&entry_rc_rdma->recv);

      request->protocol = RC_RDMA_REQ_WRITE;
      request->requested_size = size;
      request->src_process    = sctk_process_rank;
      break;
  }

  sctk_nodebug("Request sent");
}


void
sctk_net_ibv_comp_rc_rdma_prepare_ptp(
    sctk_thread_ptp_message_t * msg,
    size_t size,
    int* directly_pinned,
    sctk_thread_ptp_message_t *msg_header,
    sctk_thread_ptp_message_t **msg_header_ptr,
    void  **msg_payload_aligned_ptr,
    size_t* aligned_size)
{
  void* entire_msg = NULL;
  size_t page_size;

  /* 1 STEP : check if the msg isn't packed.
   * If it's not, we can directly pin the msg and avoid an
   * allocation */
  page_size = sctk_net_ibv_mmu_get_pagesize();
  entire_msg = sctk_net_if_one_msg_in_buffer(msg);

  if (entire_msg)
  {
    sctk_nodebug("msg _CAN_ be directly pinned");

    /*TODO: use an offset */
    *msg_payload_aligned_ptr =
      entire_msg - (((unsigned long) entire_msg) % page_size);

    *aligned_size =
      size + (((unsigned long) entire_msg) % page_size);
    *directly_pinned = 1;

    sctk_nodebug("%p->%p (%lu->%lu)", entire_msg, *msg_payload_aligned_ptr, size, aligned_size);
    *msg_header_ptr = entire_msg;
  } else {
    sctk_nodebug("The msg cannot be directly pinned (size %lu) %p", size, msg);

    *aligned_size = size;
    sctk_posix_memalign(msg_payload_aligned_ptr, page_size, *aligned_size);
    assume(*msg_payload_aligned_ptr);
    sctk_ibv_profiler_inc(IBV_MEM_TRACE);

    *msg_header_ptr = *msg_payload_aligned_ptr;
    *directly_pinned = 0;

    sctk_net_copy_in_buffer(msg, *msg_payload_aligned_ptr);
  }

  /* we copy the MPC header of the message in the request */
  memcpy (msg_header, msg,
      sizeof ( sctk_thread_ptp_message_t ));
}

static int first = 1;
void
sctk_net_ibv_comp_rc_rdma_send_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_thread_ptp_message_t * msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma)
{
  sctk_net_ibv_ibuf_t* ibuf;
  size_t aligned_size;
  sctk_net_ibv_rc_rdma_request_t* request;
  sctk_net_ibv_mmu_entry_t* mmu_entry = NULL;
  sctk_net_ibv_qp_remote_t* rc_sr_remote;
  size_t size_to_copy;
  uint32_t psn;
  int directly_pinned = 0;
  sctk_thread_ptp_message_t *msg_header_ptr;
  void *msg_payload_aligned_ptr;
  sctk_net_ibv_ibuf_header_t* msg_header;

  /* list entries */
  sctk_net_ibv_rc_rdma_entry_t *entry;

  ibuf = sctk_net_ibv_ibuf_pick(0, 1);
  request = (sctk_net_ibv_rc_rdma_request_t* ) RC_SR_PAYLOAD(ibuf->buffer);
  msg_header = RC_SR_HEADER(ibuf->buffer);

  sctk_net_ibv_comp_rc_rdma_prepare_ptp(
    msg, size, &directly_pinned, &request->msg_header,
    &msg_header_ptr, &msg_payload_aligned_ptr, &aligned_size);

  /* we register the memory where the message is stored, whatever the
   * RDVZ protocol used */
  mmu_entry = sctk_net_ibv_mmu_register (
      rail, local_rc_sr, msg_payload_aligned_ptr, aligned_size);

  /* check if the connection is opened. If not, we connect processes */
  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);


  size_to_copy = sizeof(sctk_net_ibv_rc_rdma_request_t);
  switch (ibv_rdvz_protocol)
  {
    case IBV_RDVZ_READ_PROTOCOL:
      sctk_nodebug("Initialize READ request");

      entry = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
      assume(entry);
      sctk_ibv_profiler_inc(IBV_MEM_TRACE);
      entry->ready = 0;
      /* add entry to the list */
      sctk_list_lock(&entry_rc_rdma->recv);
      entry->list_elem = sctk_list_push(&entry_rc_rdma->recv, entry);
      sctk_list_unlock(&entry_rc_rdma->recv);

      request->protocol = RC_RDMA_REQ_READ;
      request->read_req.address = (uintptr_t) msg_header_ptr;
      request->read_req.rkey  = mmu_entry->mr->rkey;
      request->entry_rc_rdma  = entry_rc_rdma;
      request->entry          = entry;
      request->msg_type         = RC_SR_RDVZ_DONE;
      sctk_nodebug("Send entry 1: %p", entry);
      request->requested_size = size;

      sctk_net_ibv_ibuf_send_init(ibuf, size_to_copy + RC_SR_HEADER_SIZE);
      psn = sctk_net_ibv_comp_rc_sr_send(
          rc_sr_remote, msg->header.destination, ibuf, size_to_copy, size_to_copy, RC_SR_RDVZ_REQUEST, &msg_header->psn, 1, 1);

      entry->mmu_entry = mmu_entry;
      entry->msg_header_ptr = msg;
      entry->requested_size = size;
      entry->src_process = dest_process;
      entry->entry_rc_rdma  = entry_rc_rdma;
      entry->psn = psn;
      entry->msg_payload_aligned_ptr = msg_payload_aligned_ptr;
      entry->directly_pinned = directly_pinned;
      entry->msg_type  = RC_SR_RDVZ_DONE;
      entry->ready = 1;

      sctk_nodebug("PSN: %lu", request->psn);
      break;

    case IBV_RDVZ_WRITE_PROTOCOL:
      sctk_nodebug("Initialize WRITE request");
      entry = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
      assume(entry);
      sctk_ibv_profiler_inc(IBV_MEM_TRACE);

      sctk_list_lock(&entry_rc_rdma->recv);
      entry->list_elem = sctk_list_push(&entry_rc_rdma->recv, entry);
      sctk_list_unlock(&entry_rc_rdma->recv);

      request->protocol = RC_RDMA_REQ_WRITE;
      request->requested_size = size;
//      request->src_process    = sctk_process_rank;
      break;
  }
}

/* TODO: request vs coll_request */
static sctk_net_ibv_rc_rdma_entry_t*
sctk_net_ibv_comp_rc_rdma_entry_send_new(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_ibuf_header_t* msg_header,
    sctk_net_ibv_rc_rdma_request_t* request)
{
  sctk_net_ibv_rc_rdma_entry_t* entry_send;
  sctk_net_ibv_mmu_entry_t* mmu_entry;
  size_t page_size = sctk_net_ibv_mmu_get_pagesize();

  /* fill the receive entry */
  entry_send = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
  assume(entry_send);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  entry_send->creation_timestamp = MPC_Wtime();

  sctk_posix_memalign((void**) &(entry_send->msg_payload_ptr),
      page_size, request->requested_size);
  assume(entry_send->msg_payload_ptr);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  mmu_entry = sctk_net_ibv_mmu_register (
      rail, local_rc_sr,
      entry_send->msg_payload_ptr, request->requested_size);
  entry_send->mmu_entry = mmu_entry;
  entry_send->entry_rc_rdma = entry_rc_rdma;

  memcpy(&entry_send->msg_header, &request->msg_header, sizeof(sctk_thread_ptp_message_t));

  entry_send->requested_size = request->requested_size;
  entry_send->src_process = msg_header->src_process;
  entry_send->src_task = entry_send->msg_header.header.source;
  entry_send->dest_task = entry_send->msg_header.header.destination;
  entry_send->psn = msg_header->psn;
  entry_send->remote_entry = request->entry;
  sctk_nodebug("request entry : %p", entry_send->remote_entry);
  entry_send->msg_type = request->msg_type;

  return entry_send;
}

void
sctk_net_ibv_comp_rc_rdma_send_finish(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_ibuf_t   *ibuf_f;
  sctk_net_ibv_rc_rdma_done_t* finish;
  sctk_net_ibv_rc_rdma_entry_t* entry;
  sctk_net_ibv_qp_remote_t* rc_sr_remote;

  entry = (sctk_net_ibv_rc_rdma_entry_t*) ibuf->supp_ptr;

  /* fill the receive entry */
  ibuf_f = sctk_net_ibv_ibuf_pick(0, 1);
  finish = (sctk_net_ibv_rc_rdma_done_t*) RC_SR_PAYLOAD(ibuf_f->buffer);

//  finish->psn = entry->psn;
//  finish->src_process = sctk_process_rank;
  finish->entry_rc_rdma = entry->entry_rc_rdma;
  finish->entry = entry->remote_entry;
  sctk_nodebug("entry : %p", finish->entry);

  sctk_nodebug("Send entry 2: %p", finish->entry);

  sctk_nodebug("Send finish with psn %lu to process %d", entry->psn, entry->src_process);

  /* check if the connection is opened. If not, we connect processes */
  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(entry->src_process);

  sctk_net_ibv_ibuf_send_init(ibuf_f, sizeof(sctk_net_ibv_rc_rdma_done_t) + RC_SR_HEADER_SIZE);
  sctk_net_ibv_comp_rc_sr_send(
      rc_sr_remote, entry->src_task, ibuf_f, sizeof(sctk_net_ibv_rc_rdma_done_t), sizeof(sctk_net_ibv_rc_rdma_done_t), RC_SR_RDVZ_DONE, NULL, 1, 1);
}


static void
sctk_net_ibv_comp_rc_rdma_process_rdma_read(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_ibuf_header_t* msg_header,
    sctk_net_ibv_rc_rdma_request_t* request)
{
  sctk_net_ibv_ibuf_t   *ibuf;
  sctk_net_ibv_rc_rdma_entry_t* entry;
  sctk_net_ibv_qp_remote_t* rc_sr_remote;

  /* fill the receive entry */
  ibuf = sctk_net_ibv_ibuf_pick(0, 1);

  entry = sctk_net_ibv_comp_rc_rdma_entry_send_new(
      rail, local_rc_sr, entry_rc_rdma, msg_header, request);
  entry->ready = 0;
  /* check if the connection is opened. If not, we connect processes */
  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(entry->src_process);

  sctk_list_lock(&entry_rc_rdma->send);
  entry->list_elem = sctk_list_push(&entry_rc_rdma->send, entry);
  sctk_nodebug("PUSH SEND %p", entry);
  sctk_list_unlock(&entry_rc_rdma->send);

  sctk_net_ibv_ibuf_rdma_read_init(
      ibuf, entry->msg_payload_ptr,
      entry->mmu_entry->mr->lkey,
      (void*) request->read_req.address, request->read_req.rkey,
      entry->requested_size, entry);

  sctk_nodebug("Posting RC_RDMA READ with size %lu", request->requested_size);

  sctk_net_ibv_qp_send_get_wqe(rc_sr_remote, ibuf);

  ibuf->supp_ptr = entry;

//  sctk_net_ibv_comp_rc_rdma_send_finish(
//      rail, rc_sr_local, rc_rdma_local,
//      ibuf);

  entry->ready = 1;
}


sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_analyze_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_rc_rdma_request_t   *request;
  sctk_net_ibv_rc_rdma_process_t   *entry_rc_rdma = NULL;
  sctk_net_ibv_ibuf_header_t* msg_header;

  request = (sctk_net_ibv_rc_rdma_request_t*) RC_SR_PAYLOAD(ibuf->buffer);
  msg_header = RC_SR_HEADER(ibuf->buffer);
  sctk_nodebug("src_process : %lu", msg_header->src_process);

  /* lock the allocator entry for the dest process */
  ALLOCATOR_LOCK(msg_header->src_process, IBV_CHAN_RC_RDMA);

  /* check if we didn't already initiate a connection */
  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(msg_header->src_process, IBV_CHAN_RC_RDMA);

  if (!entry_rc_rdma)
  {
    /* allocate the structure for the RDMA entry */
    entry_rc_rdma = sctk_net_ibv_comp_rc_rdma_allocate_init(msg_header->src_process, local_rc_rdma);
    assume(entry_rc_rdma);

    entry_rc_rdma->ready = 1;

    /* register the entry to the allocator */
    sctk_net_ibv_allocator_register(msg_header->src_process, entry_rc_rdma, IBV_CHAN_RC_RDMA);
  }
  ALLOCATOR_UNLOCK(msg_header->src_process, IBV_CHAN_RC_RDMA);

  if ( request->protocol == RC_RDMA_REQ_WRITE)
  {
    sctk_nodebug("RC_RDMA_WRITE received");

    not_implemented();

    return entry_rc_rdma;
  }

  if (request->protocol == RC_RDMA_REQ_READ)
  {
    sctk_nodebug("RC_RDMA_READ received");

    sctk_net_ibv_comp_rc_rdma_process_rdma_read(
        rail, rc_sr_local, rc_rdma_local,
        entry_rc_rdma, msg_header, request);

    return entry_rc_rdma;
  }

  assume(0);
  return entry_rc_rdma;
}

/**
 *  Read a MPC msg from a recv entry
 *  \param
 *  \return
 */
void
sctk_net_ibv_comp_rc_rdma_read_msg(
    sctk_net_ibv_rc_rdma_entry_t* entry, int type)
{
  sctk_nodebug("\t\tEntry with PSN %lu set to sent_to_mpc=1 (%p-%p)", entry->psn, &entry->msg_header, entry->sg_payload_ptr);
  /* send the msg to MPC */
  sctk_nodebug("READ RDVZ message %lu %p", entry->psn, entry);
  sctk_net_ibv_send_msg_to_mpc(&entry->msg_header, entry->msg_payload_ptr, entry->src_process, type, entry);
}

void
sctk_net_ibv_com_rc_rdma_recv_done(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    sctk_net_ibv_ibuf_t* ibuf)
{
  sctk_net_ibv_rc_rdma_done_t* done;
  sctk_net_ibv_rc_rdma_entry_t* entry;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  void* rc;


  done = (sctk_net_ibv_rc_rdma_done_t*) RC_SR_PAYLOAD(ibuf->buffer);
  entry = done->entry;
  WAIT_READY(entry);

  entry_rc_rdma = entry->entry_rc_rdma;
  sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);

  switch (entry->msg_type)
  {
    case RC_SR_BCAST:
    case RC_SR_REDUCE:
    case RC_SR_BCAST_INIT_BARRIER:
    case RC_SR_RDVZ_READ:
      sctk_free(entry->msg_payload_aligned_ptr);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      sctk_list_lock(&entry_rc_rdma->recv);
      rc = sctk_list_remove(&entry_rc_rdma->recv, entry->list_elem);
      sctk_list_unlock(&entry_rc_rdma->recv);
      /*   TODO: fixme */
      assume(rc);

      sctk_free(entry);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;

    case RC_SR_RDVZ_DONE:
      /* FIXME */
      entry->msg_header_ptr->completion_flag = 1;

      /* if the message isnt directly pinned we can
       * free the memory */
      if (!entry->directly_pinned)
      {
        sctk_free(entry->msg_payload_aligned_ptr);
        sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      }

      sctk_list_lock(&entry_rc_rdma->recv);
      rc = sctk_list_remove(&entry_rc_rdma->recv, entry->list_elem);
      sctk_list_unlock(&entry_rc_rdma->recv);
      /*   TODO: fixme */
      assume(rc);

      sctk_free(entry);
      sctk_ibv_profiler_dec(IBV_MEM_TRACE);
      break;

    default: assume(0); break;
  }

  sctk_nodebug("recv done finished");
}

void
sctk_net_ibv_com_rc_rdma_read_finish(
    sctk_net_ibv_ibuf_t* ibuf,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    int lookup_mode)
{
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_t* entry;
  int ret;
  struct sctk_list_elem* rc;

  entry = (sctk_net_ibv_rc_rdma_entry_t*)
    ibuf->supp_ptr;
  entry_rc_rdma = entry->entry_rc_rdma;
  WAIT_READY(entry);

  sctk_nodebug("RDVZ DONE RECV for process %d (PSN: %d), requested_size %lu", entry->src_process, entry->psn, entry->requested_size);

  switch (entry->msg_type)
  {
    case RC_SR_BCAST:
      sctk_nodebug("BCAST");
      sctk_net_ibv_collective_push_rc_rdma(&broadcast_fifo, entry);
      sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);
      break;

    case RC_SR_REDUCE:
      sctk_net_ibv_collective_push_rc_rdma(&reduce_fifo, entry);
      sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);
      break;

    case RC_SR_BCAST_INIT_BARRIER:
      not_reachable();
      break;

    default:
      if (lookup_mode) {
        sctk_nodebug("BEGIN lookup mode");

        ret = sctk_net_ibv_sched_sn_check_and_inc(
            entry->src_process,
            entry->src_task,
            entry->psn);

        if (ret)
        {
          sctk_nodebug("Push RDVZ msg");
          sctk_net_ibv_sched_pending_push(entry,
              sizeof(sctk_net_ibv_rc_rdma_entry_t), 0,
              IBV_POLL_RC_RDMA_ORIGIN,
              entry->src_process,
              entry->src_task,
              entry->dest_task,
              entry->psn,
              &entry->msg_header,
              entry->msg_payload_ptr,
              entry);

        } else {
          sctk_net_ibv_comp_rc_rdma_read_msg(entry, IBV_RC_RDMA_ORIGIN);
          sctk_nodebug("Recv RDVZ message from %d (PSN:%lu)", entry->src_process, entry->psn);
        }

      } else {
        ret = sctk_net_ibv_sched_sn_check_and_inc(
            entry->src_process,
            entry->src_task,
            entry->psn);

        if(ret)
        {

          sctk_nodebug("EXPECTED %lu but GOT %lu from process %d", sctk_net_ibv_sched_get_esn(entry->src_process), entry->psn, entry->src_process);
          do {
            sctk_net_ibv_allocator_ptp_lookup_all(
                entry->src_process);

            ret = sctk_net_ibv_sched_sn_check_and_inc(
                entry->src_process, entry->src_task, entry->psn);

            sctk_net_ibv_allocator_ptp_poll_all();
//            sctk_thread_yield();

          } while (ret);
        }

        sctk_nodebug("Recv RDVZ message from %d (PSN:%lu) size:%lu", entry->src_process, entry->psn, entry->requested_size);
        sctk_net_ibv_comp_rc_rdma_read_msg(entry, IBV_RC_RDMA_ORIGIN);
      }
      break;
  }
}
