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
#include "sctk_alloc.h"
#include "sctk_buffered_fifo.h"
#include "sctk_inter_thread_comm.h"
#include "sctk.h"

/*-----------------------------------------------------------
 *  STATIC DECLARATION
 *----------------------------------------------------------*/
static void
sctk_net_ibv_comp_rc_rdma_modify_qp(sctk_net_ibv_qp_remote_t* remote,
    sctk_net_ibv_qp_exchange_keys_t* keys)
{
  sctk_net_ibv_qp_allocate_recv(remote, keys);
}

void
sctk_net_ibv_comp_rc_rdma_post_recv(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv,
    void **ptr, uint32_t *rkey)
{
  struct ibv_recv_wr *bad_wr = NULL;
  int rc;
  size_t size;
  sctk_net_ibv_mmu_entry_t        *mmu_entry; /* MMU entry */

  size = entry_recv->requested_size;
  sctk_nodebug("Requested size : %lu", size);

  posix_memalign( ptr, sctk_net_ibv_mmu_get_pagesize(), size);
  assume(*ptr);

  mmu_entry = sctk_net_ibv_mmu_register (
      rail->mmu, local,
      *ptr, size);

  entry_recv->mmu_entry   = mmu_entry;
  entry_recv->ptr         = *ptr;

  entry_rc_rdma->list.addr   = (uintptr_t) *ptr;
  entry_rc_rdma->list.length = size;
  entry_rc_rdma->list.lkey   = mmu_entry->mr->lkey;

  entry_rc_rdma->wr.wr_id      = 200;
  entry_rc_rdma->wr.sg_list    = &entry_rc_rdma->list;
  entry_rc_rdma->wr.num_sge    = 1;

  sctk_nodebug("POST RECV for process %lu with psn %lu", entry_recv->src_process, entry_recv->psn);

  /* fill rkey */
  *rkey = mmu_entry->mr->rkey;
  sctk_nodebug("RKEY %lu from %d ptr %p size %lu", *rkey, entry_recv->src_process, *ptr, size);

  sctk_nodebug("there %p", entry_rc_rdma->remote.qp);
  rc = ibv_post_recv(entry_rc_rdma->remote.qp, &(entry_rc_rdma->wr), &bad_wr);
  //FIXME What's appening ?
  //sctk_error("ibv_post_recv returns : %d", rc);
  //assume(rc == 0);

}


static void
sctk_net_ibv_comp_rc_rdma_entry_send_init(
    sctk_net_ibv_rc_rdma_entry_send_t *list_entry,
    sctk_thread_ptp_message_t * msg,
    size_t size,
    sctk_net_ibv_mmu_entry_t* mmu_entry,
    void* entire_msg,
    void* aligned_ptr,
    uint32_t psn)
{
  /* pointer to msg */
  list_entry->msg = msg;
  /* size with alignement */
  list_entry->size = size;
  /* PSN */
  list_entry->psn = psn;

  /* list */
  list_entry->mmu_entry = mmu_entry;
  /*  FIXME: if entire msg or not */
  if (entire_msg)
  {
    list_entry->directly_pinned = 1;
    list_entry->list.addr = (uintptr_t) entire_msg;
  }
  else
  {
    list_entry->directly_pinned = 0;
    list_entry->list.addr = (uintptr_t) aligned_ptr;
    msg->completion_flag = 1;
  }
  list_entry->list.length = size;
  list_entry->list.lkey = mmu_entry->mr->lkey;
  /* wr */
  list_entry->send_wr.wr_id = 1000;
  list_entry->send_wr.sg_list = &(list_entry->list);
  list_entry->send_wr.num_sge = 1;
  list_entry->send_wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
  list_entry->send_wr.send_flags = IBV_SEND_SIGNALED;
  list_entry->send_wr.next = NULL;
}

void
sctk_net_ibv_comp_rc_rdma_prepare_ack(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_rdma_request_ack_t* ack,
    sctk_net_ibv_rc_rdma_entry_recv_t *entry_recv)
{
  ack->if_qp_connection = entry_recv->if_qp_connection;
  ack->src_process = sctk_process_rank;
  ack->psn = entry_recv->psn;

  if (ack->if_qp_connection )
  {
    sctk_nodebug("Connection NEDDED");
    ack->if_qp_connection = 1;

    ack->keys.lid       = rail->lid;
    ack->keys.qp_num    = entry_rc_rdma->remote.qp->qp_num;
    ack->keys.psn       = entry_rc_rdma->remote.psn;
  } else {
    sctk_nodebug("NO Connection NEDDED");
    ack->if_qp_connection = 0;
  }
}

static size_t
sctk_net_ibv_comp_rc_rdma_prepare_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_rdma_request_t* request,
    sctk_thread_ptp_message_t * msg,
    size_t size,
    int need_connection,
    int ready,
    sctk_net_ibv_mmu_entry_t * mmu,
    void* aligned_ptr,
    size_t aligned_size)
{
  size_t size_to_copy;

  /* if a new connection is needed */
  if (need_connection == 1)
  {
    request->type                           = RC_RDMA_REQ_WRITE_WITH_QP_CREATION;
    request->qp_connection.keys.lid         = rail->lid;
    request->qp_connection.keys.qp_num      = entry_rc_rdma->remote.qp->qp_num;
    request->qp_connection.keys.psn         = entry_rc_rdma->remote.psn;
    sctk_nodebug("Need connection to process %d", entry_rc_rdma->remote.rank);
  } else if (ready == 1) {
    request->type             = RC_RDMA_REQ_READ;
    request->read_request.address   = (uintptr_t) aligned_ptr;
    request->read_request.size      = aligned_size;
    request->read_request.rkey      = mmu->mr->rkey;
  } else {
    request->type             = RC_RDMA_REQ_WRITE;
  }

  request->requested_size = size;
  request->src_process    = sctk_process_rank;

  size_to_copy            =
    sizeof(sctk_net_ibv_rc_rdma_request_t);
  return size_to_copy;
}


/*-----------------------------------------------------------
 *  NEW / FREE / INIT
 *----------------------------------------------------------*/
  void
sctk_net_ibv_comp_rc_rdma_free(sctk_net_ibv_rc_rdma_process_t* entry)
{
  sctk_free(entry);
}


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void
sctk_net_ibv_comp_rc_rdma_send_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_sr_buff_t* buff_rc_sr,
    sctk_thread_ptp_message_t * msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    int need_connection)
{
  sctk_net_ibv_rc_sr_entry_t* entry;
  sctk_net_ibv_rc_rdma_process_t* rdma_entry;
  sctk_net_ibv_rc_rdma_request_t* request;
  sctk_net_ibv_rc_rdma_entry_send_t  *list_entry;
  sctk_net_ibv_qp_remote_t* rc_sr_remote;
  sctk_net_ibv_mmu_entry_t* mmu_entry = NULL;
  size_t size_to_copy;
  void* entire_msg = NULL;
  size_t page_size;
  void* aligned_ptr = NULL;
  size_t aligned_size;
  uint32_t psn;
  int ready;

  sctk_nodebug("Send RDVZ request");
  sctk_nodebug("Need connection  to process %d? %d", dest_process, need_connection);

  /* 1 STEP : check if the msg isn't packed.
   * If it's not, we can directly pin the msg */
//  entire_msg = sctk_net_if_one_msg_in_buffer(msg);
//  if (entire_msg)
//  {
//    sctk_nodebug("msg _CAN_ be directly pinned");
//
//    page_size = sctk_net_ibv_mmu_get_pagesize();
//    aligned_ptr = entire_msg - (((unsigned long) entire_msg) % page_size);
//    aligned_size = size + (page_size - (size % page_size));
//    sctk_nodebug("%p->%p (%lu->%lu)", entire_msg, aligned_ptr, size, aligned_size);
//  } else
  {
    sctk_nodebug("Msg cannot be directly pinned (size %lu) %p", size, msg);

    page_size = sctk_net_ibv_mmu_get_pagesize();
    aligned_size = size;
    sctk_posix_memalign(&aligned_ptr, page_size, size);
    assume(aligned_ptr);

    sctk_net_copy_in_buffer(msg, aligned_ptr);
  }

  /* 2 STEP : push a new rdma send entry */
  /* msg not ready for now */
  sctk_nodebug("src process : %d", dest_process);
  rdma_entry = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_RDMA);
  sctk_nodebug("RDMA : %p", rdma_entry);
  assume(rdma_entry);

  sctk_nodebug("PICK UP entry");
  /* 3 STEP : prepare the request msg */
  entry = sctk_net_ibv_comp_rc_sr_pick_header(buff_rc_sr);
  request = (sctk_net_ibv_rc_rdma_request_t* ) &(entry->msg_header->payload);

  /* copy the content of the message */
  memcpy (&request->msg_header, msg,
      sizeof ( sctk_thread_ptp_message_t ));

  /* if the QP are already connected we realize a RDMA read operation */
  ready = entry_rc_rdma->ready;
  ready = 0; //entry_rc_rdma->ready;

  /* check if the remote is already known. If not, we connect to it */
  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(
      dest_process);

  /* we register memory before sending the RC_SR message if
   * we will realize a RDMA read operation */
  if (ready == 1)
  {
    /* finally send the request msg */
    sctk_list_lock(&rdma_entry->recv);

    /* register the memory where the message is stored */
    mmu_entry = sctk_net_ibv_mmu_register (
        rail->mmu, local_rc_rdma, aligned_ptr, aligned_size);

    /* prepare the header */
    size_to_copy = sctk_net_ibv_comp_rc_rdma_prepare_request( rail,
        entry_rc_rdma, request, msg, size, need_connection, ready,
        mmu_entry, aligned_ptr, aligned_size);


    psn = sctk_net_ibv_comp_rc_sr_send(rc_sr_remote, entry, size_to_copy, RC_SR_RDVZ_REQUEST, &request->psn);
    sctk_nodebug("Entry added with PSN %d", psn);

    sctk_list_unlock(&rdma_entry->recv);
  }

  if (!ready) {
    /* 4 STEP : allocate the memory needed and pine the memory */
    list_entry = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_send_t));

    /* prepare the header */
    size_to_copy = sctk_net_ibv_comp_rc_rdma_prepare_request( rail,
        entry_rc_rdma, request, msg, size, need_connection, ready,
        mmu_entry, aligned_ptr, aligned_size);

    /* finally send the request msg */
    sctk_list_lock(&rdma_entry->send);

    sctk_nodebug("Entry added with PSN %d (request %d)", psn, request->type);
    psn = sctk_net_ibv_comp_rc_sr_send(rc_sr_remote, entry, size_to_copy, RC_SR_RDVZ_REQUEST, &request->psn);

    /* register the memory where the message is stored */
    mmu_entry = sctk_net_ibv_mmu_register (
        rail->mmu, local_rc_rdma, aligned_ptr, aligned_size);

    /* initialize the send descriptor */
    sctk_net_ibv_comp_rc_rdma_entry_send_init(
        list_entry, msg, size, mmu_entry, entire_msg,
        aligned_ptr, psn);

    sctk_list_push(&rdma_entry->send, list_entry);
    sctk_list_unlock(&rdma_entry->send);
  }
}


/**
 *  Create a new receive entry and fill it according to the request content
 *  \param
 *  \return
 */
static sctk_net_ibv_rc_rdma_entry_recv_t*
sctk_net_ibv_comp_rc_rdma_entry_recv_new(sctk_net_ibv_rc_rdma_request_t* request)
{
  sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv;

    /* fill the receive entry */
  entry_recv = sctk_malloc(sizeof (sctk_net_ibv_rc_rdma_entry_recv_t));
  assume(entry_recv);
  memcpy(&entry_recv->msg_header, &request->msg_header, sizeof(sctk_thread_ptp_message_t));
  entry_recv->requested_size = request->requested_size;
  entry_recv->src_process = request->src_process;

  return entry_recv;
}

void
sctk_net_ibv_comp_rc_rdma_process_rdma_read(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_rdma_request_t* request)
{
  struct ibv_send_wr    wr;
  struct ibv_send_wr*   bad_wr;
  struct ibv_sge        list;
  sctk_net_ibv_rc_rdma_entry_recv_t                   *entry_recv;
  void* ptr;
  uint32_t rkey;

  /* fill the receive entry */
  entry_recv = sctk_net_ibv_comp_rc_rdma_entry_recv_new(request);

  sctk_list_lock(&entry_rc_rdma->recv);
  sctk_list_push(&entry_rc_rdma->recv, entry_recv);
  sctk_list_unlock(&entry_rc_rdma->recv);

  sctk_net_ibv_comp_rc_rdma_post_recv(
    rail, entry_rc_rdma, local_rc_rdma, entry_recv,
    &ptr, &rkey);

  list.addr   = (uintptr_t) ptr;
  list.length = entry_recv->requested_size;
  list.lkey   = rkey;

  /* wr */
  wr.wr_id = 1001;
  wr.sg_list = &(list);
  wr.num_sge = 1;
  wr.opcode = IBV_WR_RDMA_READ;
  wr.send_flags = IBV_SEND_SIGNALED;
  wr.wr.rdma.remote_addr = request->read_request.address;
  wr.wr.rdma.rkey        = request->read_request.rkey;
  wr.next = NULL;

  ibv_post_send(entry_rc_rdma->remote.qp,
      &wr, &bad_wr);
}


/**
 *  Analize the request received and call the right functions
 *  \param
 *  \return
 */
sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_analyze_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_sr_entry_t* entry)
{
  sctk_net_ibv_rc_rdma_request_t   *request;
  sctk_net_ibv_rc_rdma_process_t   *entry_rc_rdma = NULL;

  request = (sctk_net_ibv_rc_rdma_request_t*)
    &entry->msg_header->payload;

  /* lock the allocator entry for the dest process */
  sctk_net_ibv_allocator_lock(request->src_process, IBV_CHAN_RC_RDMA);

  /* check if we didn't already initiate a connection */
  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(request->src_process, IBV_CHAN_RC_RDMA);

  if (request->type == RC_RDMA_REQ_WRITE_WITH_QP_CREATION)
  {
    if (!entry_rc_rdma)
    {
      sctk_nodebug("LID %d, qp_num %d, psn %d", request->keys.lid, request->keys.qp_num, request->keys.psn);

      /* allocate the structure for the RDMA entry */
      entry_rc_rdma = sctk_net_ibv_comp_rc_rdma_allocate_init(request->src_process, local_rc_rdma);
      assume(entry_rc_rdma);

      /* register the entry to the allocator */
      sctk_net_ibv_allocator_register(request->src_process, entry_rc_rdma, IBV_CHAN_RC_RDMA);
    }

    if(entry_rc_rdma->ready == 0)
    {
      sctk_net_ibv_comp_rc_rdma_modify_qp(&entry_rc_rdma->remote, &request->qp_connection.keys);
      entry_rc_rdma->ready = 1;
      sctk_nodebug("Connected to process %d!", request->src_process);
    }
    sctk_net_ibv_allocator_unlock(request->src_process, IBV_CHAN_RC_RDMA);

  } else {
    sctk_net_ibv_allocator_unlock(request->src_process, IBV_CHAN_RC_RDMA);
  }

  assume(entry_rc_rdma);

  if ( (request->type == RC_RDMA_REQ_WRITE) ||
      (request->type == RC_RDMA_REQ_WRITE_WITH_QP_CREATION)) {
    sctk_nodebug("RC_RDMA_WRITE received");

    sctk_net_ibv_comp_rc_rdma_add_request(
        rail, rc_sr_local, rc_rdma_local,
        entry_rc_rdma, request);

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
 *  Add a new request to the received request list
 *  \param
 *  \return
 */
sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_add_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma,
    sctk_net_ibv_rc_rdma_request_t* request)
{
  sctk_net_ibv_rc_rdma_entry_recv_t                   *entry_recv;

  sctk_nodebug("Recv RDVZ REQUEST message from %d (PSN %d)", request->src_process, request->psn);

  /* fill the receive entry */
  entry_recv = sctk_net_ibv_comp_rc_rdma_entry_recv_new(request);

  if (request->type == RC_RDMA_REQ_WRITE_WITH_QP_CREATION)
  {
    entry_recv->if_qp_connection = 1;
  } else {
    entry_recv->if_qp_connection = 0;
  }

  sctk_nodebug("PSN received : %d (3)", request->psn);
  entry_recv->psn = request->psn;
  entry_recv->used = 0;

  sctk_list_lock(&entry_rc_rdma->recv);
  sctk_list_push(&entry_rc_rdma->recv, entry_recv);
  sctk_list_unlock(&entry_rc_rdma->recv);

  return entry_rc_rdma;
}

/*-----------------------------------------------------------
 *  REQUEST PROCESSING
 *----------------------------------------------------------*/
#define IS_RDMA_WRITE(entry_recv) ((entry_recv->type == RC_RDMA_REQ_WRITE) ||\
    (entry_recv->type == RC_RDMA_REQ_WRITE_WITH_QP_CREATION))
sctk_net_ibv_rc_rdma_entry_recv_t *
sctk_net_ibv_comp_rc_rdma_check_pending_request(
    sctk_net_ibv_rc_rdma_process_t *entry_rc_rdma)
{
  struct sctk_list_elem *tmp                          = entry_rc_rdma->recv.head;
  sctk_net_ibv_rc_rdma_entry_recv_t *entry_recv       = NULL;
  sctk_net_ibv_rc_rdma_entry_recv_t *last_entry_recv  = NULL;

  if (tmp)
  {
    entry_recv = (sctk_net_ibv_rc_rdma_entry_recv_t*) tmp->elem;

    if (entry_recv && IS_RDMA_WRITE(entry_recv))
    {
      if (entry_recv->used == 1) {
        last_entry_recv = NULL;
      } else {
        last_entry_recv = entry_recv;
      }
    }
  }
  return last_entry_recv;
}

sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_send_msg(
    sctk_net_ibv_qp_rail_t  *rail,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_sr_entry_t* entry)
{
  sctk_net_ibv_rc_rdma_request_ack_t* ack;
  sctk_net_ibv_rc_rdma_entry_send_t*  entry_send = NULL;
  sctk_net_ibv_rc_rdma_process_t*              entry_rc_rdma;
  struct  ibv_send_wr* bad_wr = NULL;
  int i;

  ack = (sctk_net_ibv_rc_rdma_request_ack_t*) &(entry->msg_header->payload);

  sctk_nodebug("src process : %d", ack->src_process);

  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(ack->src_process, IBV_CHAN_RC_RDMA);
  assume(entry_rc_rdma);

  /* if a QP creation is needed */
  if (ack->if_qp_connection && (entry_rc_rdma->ready == 0))
  {
    sctk_nodebug("QP creation needed");

    sctk_nodebug("LID : %d qp_num : %d psn : %d", ack->keys.lid, ack->keys.qp_num , ack->keys.psn);

    sctk_net_ibv_comp_rc_rdma_modify_qp(&entry_rc_rdma->remote, &ack->keys);
    entry_rc_rdma->ready = 1;
    sctk_nodebug("Connected to process %d!", ack->src_process);
  }

  i = 0;
  sctk_list_lock(&entry_rc_rdma->send);
  sctk_nodebug("Begin");
  do
  {
    entry_send = (sctk_net_ibv_rc_rdma_entry_send_t*)
      sctk_list_get_from_head(&entry_rc_rdma->send, i);

    if (entry_send)
      sctk_nodebug("Looking for psn %d - Psn found : %d", ack->psn, entry_send->psn);

    if (entry_send && (entry_send->psn == ack->psn) )
      break;

    ++i;
  } while(entry_send);

  sctk_nodebug("End");
  sctk_list_unlock(&entry_rc_rdma->send);
  assume(entry_send);

  sctk_nodebug("RDVZ ACK for process %lu", ack->src_process);

  /* fill the remote addr */
  entry_send->send_wr.wr.rdma.remote_addr = (uintptr_t) ack->dest_ptr;
  entry_send->send_wr.wr.rdma.rkey        = ack->dest_rkey;
  sctk_nodebug("Dest rkey : %lu from %d ptr %p size %lu", ack->dest_rkey, ack->src_process, ack->dest_ptr, entry_send->size);

  sctk_nodebug("Send RDVZ message to %d and size %lu (PSN:%d)",
      ack->src_process, entry_send->size, entry_send->psn);
  /* finally transfert the msg */
  ibv_post_send(entry_rc_rdma->remote.qp,
      &(entry_send->send_wr), &bad_wr);

  return entry_rc_rdma;
}

  sctk_net_ibv_rc_rdma_entry_recv_t*
sctk_net_ibv_comp_rc_rdma_match_read_msg(int src_process)
{
  sctk_net_ibv_rc_rdma_process_t*       entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_recv_t*  entry_recv;
  int i = 0;

  sctk_nodebug("src process : %d", src_process);
  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(src_process, IBV_CHAN_RC_RDMA);
  assume(entry_rc_rdma);
  sctk_nodebug("entry_rc_rdma for process %d: %p ", src_process, entry_rc_rdma);

  sctk_list_lock(&entry_rc_rdma->recv);

  do
  {
    entry_recv = (sctk_net_ibv_rc_rdma_entry_recv_t*)
      sctk_list_get_from_head(&entry_rc_rdma->recv, i);

    if ( (entry_recv) && (entry_recv->used == 1) && IS_RDMA_WRITE(entry_recv)) {
      sctk_list_unlock(&entry_rc_rdma->recv);
      return entry_recv;
    }

    i++;
  } while( (entry_recv != NULL) );

  sctk_list_unlock(&entry_rc_rdma->recv);
  return NULL;
}


/**
 *  Read a MPC msg from a recv entry
 *  \param
 *  \return
 */
void
sctk_net_ibv_comp_rc_rdma_read_msg(
    sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv, int type)
{
  sctk_nodebug("\t\tEntry with PSN %lu set to sent_to_mpc=1", entry_recv->psn);
  /* send the msg to MPC */
  sctk_nodebug("READ RDVZ message %lu %p", entry_recv->psn, entry_recv);
  sctk_net_ibv_send_msg_to_mpc(&entry_recv->msg_header, entry_recv->ptr, entry_recv->src_process, type, entry_recv);
}

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


/*-----------------------------------------------------------
 *  POLLING FUNCTIONS
 *----------------------------------------------------------*/
void
sctk_net_ibv_rc_rdma_poll_send(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    sctk_net_ibv_rc_sr_buff_t* buff_rc_sr)
{
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_send_t *entry_send;
  sctk_net_ibv_rc_rdma_entry_send_t *removed_entry;

  sctk_nodebug("NEW SEND src_qp : %lu", wc->qp_num);

  entry_rc_rdma = sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(wc->qp_num);
  assume(entry_rc_rdma);

  sctk_list_lock(&entry_rc_rdma->send);
  entry_send = (sctk_net_ibv_rc_rdma_entry_send_t *)
    sctk_list_get_from_head(&entry_rc_rdma->send, 0);

  if (entry_send->directly_pinned)
  {
    sctk_nodebug("Message directly pinned");
    /* FIXME */
    entry_send->msg->completion_flag = 1;
  } else {
    sctk_free( (void*) entry_send->list.addr);
  }

  sctk_net_ibv_mmu_unregister (rail->mmu, entry_send->mmu_entry);

  sctk_nodebug("POP from qp_num %d  (PSN:%d)", wc->qp_num, entry_send->psn);
  removed_entry = sctk_list_pop(&entry_rc_rdma->send);
  sctk_list_unlock(&entry_rc_rdma->send);
  free(removed_entry);
}

void
sctk_net_ibv_rc_rdma_poll_recv(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    sctk_net_ibv_rc_sr_buff_t   *rc_sr_recv_buff,
    int lookup_mode)
{
  sctk_net_ibv_rc_sr_entry_t* entry;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv;
  int wc_id;
  int ret;

  sctk_nodebug("NEW SEND src_qp : %lu", wc->qp_num);
  entry_rc_rdma = sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(wc->qp_num);
  assume(entry_rc_rdma);

  entry_recv = sctk_net_ibv_comp_rc_rdma_match_read_msg(entry_rc_rdma->remote.rank);
  assume(entry_recv);

  wc_id = wc->wr_id;
  entry = &(rc_sr_recv_buff->headers[wc_id]);

  sctk_nodebug("RDVZ DONE RECV for process %d (PSN: %d)", entry_rc_rdma->remote.rank, entry_recv->psn);

  if (lookup_mode) {
    //    sctk_nodebug("BEGIN lookup mode");

    ret = sctk_net_ibv_sched_sn_check_and_inc(
        entry_recv->src_process,
        entry_recv->psn);

    if (ret)
    {
      sctk_nodebug("Push RDVZ msg");
      sctk_net_ibv_sched_pending_push(entry_recv,
          sizeof(sctk_net_ibv_rc_rdma_entry_recv_t), 0,
          IBV_CHAN_RC_RDMA);

    } else {
      sctk_nodebug("Read the msg");

      sctk_net_ibv_comp_rc_rdma_read_msg(entry_recv, IBV_RC_RDMA_ORIGIN);

      sctk_nodebug("Recv RDVZ message from %d (PSN:%lu)", entry_recv->src_process, entry_recv->psn);
    }

    sctk_list_lock(&entry_rc_rdma->recv);
    sctk_list_search_and_free(&entry_rc_rdma->recv, entry_recv);
    sctk_list_unlock(&entry_rc_rdma->recv);

  } else {
    ret = sctk_net_ibv_sched_sn_check_and_inc(
        entry_recv->src_process,
        entry_recv->psn);

    if(ret)
    {
      do {
        sctk_net_ibv_allocator_ptp_lookup_all(
            entry_recv->src_process);

        ret = sctk_net_ibv_sched_sn_check_and_inc(
            entry_recv->src_process, entry_recv->psn);

        sctk_thread_yield();

      } while (ret);
    }
    sctk_list_lock(&entry_rc_rdma->recv);
    sctk_list_search_and_free(&entry_rc_rdma->recv, entry_recv);
    sctk_list_unlock(&entry_rc_rdma->recv);

    sctk_net_ibv_comp_rc_rdma_read_msg(entry_recv, IBV_RC_RDMA_ORIGIN);

    sctk_nodebug("Recv RDVZ message from %d (PSN:%lu)", entry_recv->src_process, entry_recv->psn);
  }

  /* process the next RDMA request */
  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(entry_recv->src_process, IBV_CHAN_RC_RDMA);
  sctk_net_ibv_allocator_rc_rdma_process_next_request(entry_rc_rdma);
}


/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/
sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_allocate_init(
    unsigned int rank,
    sctk_net_ibv_qp_local_t *local)
{
  sctk_net_ibv_rc_rdma_process_t* rdma;

  rdma = sctk_malloc(sizeof(sctk_net_ibv_rc_rdma_process_t));
  assume(rdma);

  sctk_net_ibv_qp_allocate_init(rank, local, &rdma->remote);

  sctk_thread_mutex_init(&rdma->lock, NULL);
  sctk_nodebug("SEND");
  sctk_list_new(&(rdma->send), 0);
  sctk_nodebug("RECV");
  sctk_list_new(&(rdma->recv), 0);

  rdma->ready = 0;

  sctk_nodebug("QP registered");
  return rdma;
}


/*-----------------------------------------------------------
 *  ERROR HANDLING
 *----------------------------------------------------------*/
  void
sctk_net_ibv_comp_rc_rdma_print_entry_recv(sctk_net_ibv_rc_rdma_entry_recv_t* entry)
{
  sctk_nodebug(
      "mmu_entry: %p\n"
      "msg_header: %p\n"
      "ptr: %p\n"
      "requested_size: %d\n",
      entry->mmu_entry, entry->msg_header,
      entry->ptr);
}

  void
sctk_net_ibv_comp_rc_rdma_print_entry_send(sctk_net_ibv_rc_rdma_entry_send_t* entry)
{
  sctk_nodebug("\n# - - - - - - - - - -\n"
      "# mmu_entry: %p\n"
      "# msg: %p\n"
      "# size: %d\n"
      "# directly_pinned: %d\n"
      "# psn : %d\n"
      "# - - - - - - - - - -",
      entry->mmu_entry, entry->msg,
      entry->size, entry->directly_pinned,
      entry->psn);
}


  void
sctk_net_ibv_comp_rc_rdma_error_handler_send(struct ibv_wc wc)
{
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_send_t* entry_send;

  entry_rc_rdma = sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(wc.qp_num);
  assume(entry_rc_rdma);

  sctk_nodebug("# qp_num %lu", wc.qp_num);

  entry_send = (sctk_net_ibv_rc_rdma_entry_send_t*)
    sctk_list_get_from_head(&entry_rc_rdma->send, 0);
  assume(entry_send);

  sctk_net_ibv_comp_rc_rdma_print_entry_send(entry_send);
}

  void
sctk_net_ibv_comp_rc_rdma_error_handler_recv(struct ibv_wc wc)
{
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv;

  entry_rc_rdma = sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(wc.qp_num);
  assume(entry_rc_rdma);
  entry_recv = sctk_net_ibv_comp_rc_rdma_match_read_msg(entry_rc_rdma->remote.rank);
  assume(entry_recv);

  sctk_net_ibv_comp_rc_rdma_print_entry_recv(entry_recv);
}
