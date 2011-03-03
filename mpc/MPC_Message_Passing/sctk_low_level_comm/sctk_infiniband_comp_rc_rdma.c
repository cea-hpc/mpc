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
#include "sctk.h"

#define WAIT_READY(x) \
  while(!x->ready) sctk_thread_yield()

/* channel selection */
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
  sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_rdma_create_local(sctk_net_ibv_qp_rail_t* rail)
{
  sctk_net_ibv_qp_local_t* local;

  local = sctk_net_ibv_qp_new(rail, 0);

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
  sctk_list_new(&(rdma->send), 0);
  sctk_nodebug("RECV");
  sctk_list_new(&(rdma->recv), 0);

  rdma->ready = 0;

  return rdma;
}

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
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
  size_t page_size;
  void* aligned_ptr = NULL;
  void* entire_msg = NULL;
  void* msg_ptr = NULL;
  size_t aligned_size;
  sctk_net_ibv_rc_rdma_request_t* request;
  sctk_net_ibv_mmu_entry_t* mmu_entry = NULL;
  sctk_net_ibv_qp_remote_t* rc_sr_remote;
  size_t size_to_copy;
  uint32_t psn;
  int directly_pinned = 0;

  /* list entries */
  sctk_net_ibv_rc_rdma_entry_t *entry;

  sctk_net_ibv_rc_sr_msg_header_t* msg_header;

//  sctk_nodebug("Pick 3");
  ibuf = sctk_net_ibv_ibuf_pick(0);
  request = (sctk_net_ibv_rc_rdma_request_t* ) RC_SR_PAYLOAD(ibuf);

  /* TODO: 1 STEP : check if the msg isn't packed.
   * If it's not, we can directly pin the msg and avoid an
   * allocation */
  entire_msg = sctk_net_if_one_msg_in_buffer(msg);
  if (entire_msg)
  {
    sctk_nodebug("msg _CAN_ be directly pinned");

    page_size = sctk_net_ibv_mmu_get_pagesize();
    aligned_ptr = entire_msg - (((unsigned long) entire_msg) % page_size);
    aligned_size = size + (((unsigned long) entire_msg) % page_size); //(page_size - (size % page_size));
    directly_pinned = 1;
    sctk_nodebug("%p->%p (%lu->%lu)", entire_msg, aligned_ptr, size, aligned_size);
    msg_ptr = entire_msg;
  } else
  {
    sctk_nodebug("The msg cannot be directly pinned (size %lu) %p", size, msg);

    page_size = sctk_net_ibv_mmu_get_pagesize();
    aligned_size = size;
    sctk_posix_memalign(&aligned_ptr, page_size, aligned_size);
    assume(aligned_ptr);
    msg_ptr = aligned_ptr;

    sctk_net_copy_in_buffer(msg, aligned_ptr);
  }

  /* we copy the MPC header of the message in the request */
  memcpy (&request->msg_header, msg,
      sizeof ( sctk_thread_ptp_message_t ));

  /* we register the memory where the message is stored, whatever the
   * RDVZ protocol used */
  mmu_entry = sctk_net_ibv_mmu_register (
        rail->mmu, local_rc_sr, aligned_ptr, aligned_size);

  /* check if the connection is opened. If not, we connect processes */
  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);

  size_to_copy = sizeof(sctk_net_ibv_rc_rdma_request_t);
//  sctk_nodebug("there");
  switch (ibv_rdvz_protocol)
  {
    case IBV_RDVZ_READ_PROTOCOL:
      sctk_nodebug("Initialize READ request");

      entry = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
      assume(entry);
      entry->ready = 0;
      /* add entry to the list */
      sctk_list_lock(&entry_rc_rdma->recv);
      sctk_list_push(&entry_rc_rdma->recv, entry);
      sctk_list_unlock(&entry_rc_rdma->recv);

      request->type = RC_RDMA_REQ_READ;
      request->read_req.address = (uintptr_t) msg_ptr;
      request->read_req.rkey  = mmu_entry->mr->rkey;
      request->entry_rc_rdma  = entry_rc_rdma;
      request->entry          = entry;
      sctk_nodebug("Send entry 1: %p", entry);
      request->requested_size = size;
      request->src_process    = sctk_process_rank;

      sctk_net_ibv_ibuf_send_init(ibuf, size_to_copy + RC_SR_HEADER_SIZE);
      psn = sctk_net_ibv_comp_rc_sr_send(
          rc_sr_remote, ibuf, size_to_copy, RC_SR_RDVZ_REQUEST, &request->psn);

      entry->mmu_entry = mmu_entry;
      entry->msg_ptr = msg;
      entry->requested_size = size;
      entry->src_process = dest_process;
      entry->entry_rc_rdma  = entry_rc_rdma;
      entry->psn = psn;
      entry->aligned_ptr = aligned_ptr;
      entry->ready = 1;
      entry->directly_pinned = directly_pinned;

      sctk_nodebug("MSG: %p", msg);


      sctk_nodebug("PSN: %lu", request->psn);
      break;

    case IBV_RDVZ_WRITE_PROTOCOL:
      sctk_nodebug("Initialize WRITE request");
      entry = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
      assume(entry);

      sctk_list_lock(&entry_rc_rdma->recv);
      sctk_list_push(&entry_rc_rdma->recv, entry);
      sctk_list_unlock(&entry_rc_rdma->recv);

      request->type = RC_RDMA_REQ_WRITE;
      request->requested_size = size;
      request->src_process    = sctk_process_rank;
      break;
  }

  sctk_nodebug("Request sent");
}

  static sctk_net_ibv_rc_rdma_entry_t*
sctk_net_ibv_comp_rc_rdma_entry_send_new(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_rdma_request_t* request)
{
  sctk_net_ibv_rc_rdma_entry_t* entry_send;
  sctk_net_ibv_mmu_entry_t* mmu_entry;
  size_t page_size = sctk_net_ibv_mmu_get_pagesize();

  /* fill the receive entry */
  entry_send = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_t));
  assume(entry_send);

  sctk_posix_memalign((void**) &(entry_send->ptr),
      page_size, request->requested_size);
  assume(entry_send->ptr);

  mmu_entry = sctk_net_ibv_mmu_register (
      rail->mmu, local_rc_sr,
      entry_send->ptr, request->requested_size);
  entry_send->mmu_entry = mmu_entry;
  entry_send->entry_rc_rdma = entry_rc_rdma;

  memcpy(&entry_send->msg_header, &request->msg_header, sizeof(sctk_thread_ptp_message_t));

  entry_send->requested_size = request->requested_size;
  entry_send->src_process = request->src_process;
  entry_send->psn = request->psn;
  entry_send->remote_entry = request->entry;

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
  ibuf_f = sctk_net_ibv_ibuf_pick(0);
  finish = (sctk_net_ibv_rc_rdma_done_t*) RC_SR_PAYLOAD(ibuf_f);

  finish->psn = entry->psn;
  finish->src_process = sctk_process_rank;
  finish->entry_rc_rdma = entry->entry_rc_rdma;
  sctk_nodebug("entry : %p", entry);
  finish->entry = entry->remote_entry;

  sctk_nodebug("Send entry 2: %p", finish->entry);

  sctk_nodebug("Send finish with psn %lu to process %d", entry->psn, entry->src_process);

  /* check if the connection is opened. If not, we connect processes */
  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(entry->src_process);

  sctk_net_ibv_ibuf_send_init(ibuf_f, sizeof(sctk_net_ibv_rc_rdma_done_t) + RC_SR_HEADER_SIZE);
  sctk_net_ibv_comp_rc_sr_send(
    rc_sr_remote, ibuf_f, sizeof(sctk_net_ibv_rc_rdma_done_t), RC_SR_RDVZ_DONE, NULL);
}


static void
sctk_net_ibv_comp_rc_rdma_process_rdma_read(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_rdma_request_t* request)
{
  sctk_net_ibv_ibuf_t   *ibuf, *ibuf_recv;
  sctk_net_ibv_rc_rdma_entry_t* entry;
  sctk_net_ibv_qp_remote_t* rc_sr_remote;
  int rc;

  /* fill the receive entry */
  ibuf = sctk_net_ibv_ibuf_pick(0);

  entry = sctk_net_ibv_comp_rc_rdma_entry_send_new(
      rail, local_rc_sr, entry_rc_rdma, request);
  entry->ready = 0;
  /* check if the connection is opened. If not, we connect processes */
  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(entry->src_process);

  sctk_list_lock(&entry_rc_rdma->send);
  sctk_list_push(&entry_rc_rdma->send, entry);
  sctk_list_unlock(&entry_rc_rdma->send);

  sctk_net_ibv_ibuf_rdma_read_init(
    ibuf, entry->ptr,
    entry->mmu_entry->mr->lkey,
    (void*) request->read_req.address, request->read_req.rkey,
    entry->requested_size, entry);

  sctk_nodebug("Posting RC_RDMA READ with size %lu", request->requested_size);

  rc = ibv_post_send(rc_sr_remote->qp , &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
  assume (rc == 0);

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

  request = (sctk_net_ibv_rc_rdma_request_t*) RC_SR_PAYLOAD(ibuf);

  /* lock the allocator entry for the dest process */
  sctk_net_ibv_allocator_lock(request->src_process, IBV_CHAN_RC_RDMA);

  /* check if we didn't already initiate a connection */
  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(request->src_process, IBV_CHAN_RC_RDMA);

  if (!entry_rc_rdma)
  {
    /* allocate the structure for the RDMA entry */
    entry_rc_rdma = sctk_net_ibv_comp_rc_rdma_allocate_init(request->src_process, local_rc_rdma);
    assume(entry_rc_rdma);

    entry_rc_rdma->ready = 1;

    /* register the entry to the allocator */
    sctk_net_ibv_allocator_register(request->src_process, entry_rc_rdma, IBV_CHAN_RC_RDMA);
  }

  sctk_net_ibv_allocator_unlock(request->src_process, IBV_CHAN_RC_RDMA);


  if ( request->type == RC_RDMA_REQ_WRITE)
  {
    sctk_nodebug("RC_RDMA_WRITE received");

//    sctk_net_ibv_comp_rc_rdma_add_request(
//        rail, rc_sr_local, rc_rdma_local,
//        entry_rc_rdma, request);

    return entry_rc_rdma;
  }

  if (request->type == RC_RDMA_REQ_READ)
  {
    sctk_nodebug("RC_RDMA_READ received");

    sctk_net_ibv_comp_rc_rdma_process_rdma_read(
        rail, rc_sr_local, rc_rdma_local,
        entry_rc_rdma, request);

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
  sctk_nodebug("\t\tEntry with PSN %lu set to sent_to_mpc=1 (%p-%p)", entry->psn, &entry->msg_header, entry->ptr);
  /* send the msg to MPC */
  sctk_nodebug("READ RDVZ message %lu %p", entry->psn, entry);
  sctk_net_ibv_send_msg_to_mpc(&entry->msg_header, entry->ptr, entry->src_process, type, entry);
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


  done = (sctk_net_ibv_rc_rdma_done_t*) RC_SR_PAYLOAD(ibuf);
  entry = done->entry;
  WAIT_READY(entry);

  entry_rc_rdma = entry->entry_rc_rdma;

  /* FIXME */
  sctk_net_ibv_mmu_unregister (rail->mmu, entry->mmu_entry);

  entry->msg_ptr->completion_flag = 1;

  sctk_list_lock(&entry_rc_rdma->send);
  sctk_list_search_and_free(&entry_rc_rdma->send, entry);
  sctk_list_unlock(&entry_rc_rdma->send);

  /* FIXME: change there*/
  if (!entry->directly_pinned)
  {
    sctk_free(entry->aligned_ptr);
  }
  sctk_free(entry);
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

  entry = (sctk_net_ibv_rc_rdma_entry_t*)
    ibuf->supp_ptr;
  entry_rc_rdma = entry->entry_rc_rdma;
  WAIT_READY(entry);

  sctk_nodebug("RDVZ DONE RECV for process %d (PSN: %d), requested_size %lu", entry->src_process, entry->psn, entry->requested_size);

  if (lookup_mode) {
    sctk_nodebug("BEGIN lookup mode");

    ret = sctk_net_ibv_sched_sn_check_and_inc(
        entry->src_process,
        entry->psn);

    if (ret)
    {
      sctk_nodebug("Push RDVZ msg");
      sctk_net_ibv_sched_pending_push(entry,
          sizeof(sctk_net_ibv_rc_rdma_entry_t), 0,
          IBV_CHAN_RC_RDMA);

    } else {
      sctk_net_ibv_comp_rc_rdma_read_msg(entry, IBV_RC_RDMA_ORIGIN);

      sctk_nodebug("Recv RDVZ message from %d (PSN:%lu)", entry->src_process, entry->psn);
    }

    sctk_list_lock(&entry_rc_rdma->recv);
    sctk_list_search_and_free(&entry_rc_rdma->recv, entry);
    sctk_list_unlock(&entry_rc_rdma->recv);

  } else {

    ret = sctk_net_ibv_sched_sn_check_and_inc(
        entry->src_process,
        entry->psn);

    if(ret)
    {

      sctk_nodebug("EXPECTED %lu but GOT %lu from process %d", sctk_net_ibv_sched_get_esn(entry_send->src_process), entry_send->psn, entry_send->src_process);
      do {
        sctk_net_ibv_allocator_ptp_lookup_all(
            entry->src_process);

        ret = sctk_net_ibv_sched_sn_check_and_inc(
            entry->src_process, entry->psn);

        sctk_thread_yield();

      } while (ret);
    }
    sctk_list_lock(&entry_rc_rdma->send);
    sctk_list_search_and_free(&entry_rc_rdma->send, entry);
    sctk_list_unlock(&entry_rc_rdma->send);

    sctk_nodebug("Recv RDVZ message from %d (PSN:%lu) size:%lu", entry->src_process, entry->psn, entry->requested_size);
    sctk_net_ibv_comp_rc_rdma_read_msg(entry, IBV_RC_RDMA_ORIGIN);
  }
}
