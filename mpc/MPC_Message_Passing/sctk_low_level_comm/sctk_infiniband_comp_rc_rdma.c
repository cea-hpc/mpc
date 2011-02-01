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
#include "sctk_net_tools.h"
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
    void* aligned_ptr)
{
  /* pointer to msg */
  list_entry->msg = msg;
  /* size with alignement */
  list_entry->size = size;

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
  list_entry->send_wr.opcode = IBV_WR_RDMA_WRITE;
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
    sctk_net_ibv_rc_rdma_request_with_qp_connection_t* request,
    sctk_thread_ptp_message_t * msg,
    size_t size,
    int need_connection,
    uint32_t psn)
{
  size_t size_to_copy;

  /* if a new connection is needed */
  if (need_connection == 1)
  {
    size_to_copy = sizeof(sctk_net_ibv_rc_rdma_request_with_qp_connection_t);

    //    sctk_nodebug("Connection requested LID: %d, qp_num %d, psn %d!", rail->lid, entry_rc_rdma->remote.qp->qp_num, entry_rc_rdma->remote.psn);

    request->keys.lid         = rail->lid;
    request->keys.qp_num      = entry_rc_rdma->remote.qp->qp_num;
    request->keys.psn         = entry_rc_rdma->remote.psn;
    request->if_qp_connection = 1;
  } else {

    size_to_copy = sizeof(sctk_net_ibv_rc_rdma_request_t);
    request->if_qp_connection = 0;
  }

  request->requested_size = size;
  request->src_process    = sctk_process_rank;
  request->psn            = psn;
  memcpy (&request->msg_header, msg, sizeof ( sctk_thread_ptp_message_t ));

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
    int need_connection,
    uint32_t psn)
{
  sctk_net_ibv_rc_sr_entry_t* entry;
  sctk_net_ibv_rc_rdma_process_t* rdma_entry;
  sctk_net_ibv_rc_rdma_request_with_qp_connection_t* request;
  sctk_net_ibv_rc_rdma_entry_send_t entry_send, *list_entry;
  sctk_net_ibv_qp_remote_t* rc_sr_remote;
  sctk_net_ibv_mmu_entry_t* mmu_entry = NULL;
  size_t size_to_copy;
  void* entire_msg;
  size_t page_size;
  void* aligned_ptr = NULL;
  size_t aligned_size;

  sctk_nodebug("Send RDVZ request");

  /* 1 STEP : check if the msg isn't packed.
   * If it's not, we can directly pin the msg */
  entire_msg = sctk_net_if_one_msg_in_buffer(msg);
//  if (entire_msg)
//  {
//    sctk_nodebug("msg _CAN_ be directly pinned");
//
//    page_size = sctk_net_ibv_mmu_get_pagesize();
//    aligned_ptr = entire_msg - (((unsigned long) entire_msg) % page_size);
//    aligned_size = size + (page_size - (size % page_size));
//    sctk_nodebug("Size sent : %lu", size);
//  } else {
    {
    void** ptr;
    entire_msg = NULL;
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

  /* 3 STEP : prepare the request msg */
  entry = sctk_net_ibv_comp_rc_sr_pick_header(buff_rc_sr);
  request = (sctk_net_ibv_rc_rdma_request_with_qp_connection_t* ) &(entry->msg_header->payload);

  size_to_copy = sctk_net_ibv_comp_rc_rdma_prepare_request( rail, entry_rc_rdma,
      request, msg, size, need_connection, psn);
  sctk_nodebug("SIZE : %lu", size);

  rc_sr_remote = sctk_net_ibv_comp_rc_sr_check_and_connect(
      dest_process);
  /* finally send the REQUEST msg */
  sctk_net_ibv_comp_rc_sr_send(rc_sr_remote, entry, size_to_copy, RC_SR_RDVZ_REQUEST);


  /* 4 STEP : allocate the memory needed and pine the memory */
  sctk_spinlock_lock(&rdma_entry->lock);
  list_entry = sctk_buffered_fifo_push(&rdma_entry->send, &entry_send);
  sctk_spinlock_unlock(&rdma_entry->lock);

  mmu_entry = sctk_net_ibv_mmu_register (
      rail->mmu, local_rc_rdma, aligned_ptr, aligned_size);

  sctk_net_ibv_comp_rc_rdma_entry_send_init(
      list_entry, msg, size, mmu_entry, entire_msg,
      aligned_ptr);
}


sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_add_request(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_qp_local_t* local_rc_rdma,
    sctk_net_ibv_rc_sr_entry_t* entry)
{
  sctk_net_ibv_rc_rdma_entry_recv_t                   entry_recv;
  sctk_net_ibv_rc_rdma_entry_send_t                   *second_entry_recv;
  sctk_net_ibv_rc_rdma_request_with_qp_connection_t   *request;
  sctk_net_ibv_rc_rdma_process_t                      *entry_rc_rdma = NULL;
  size_t                                              size;

  request = (sctk_net_ibv_rc_rdma_request_with_qp_connection_t*) &(entry->msg_header->payload);
  size = request->requested_size;

  /* check if we didn't already initiate a connection */
  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(request->src_process, IBV_CHAN_RC_RDMA);

  /* check if a connection is needed by the remote peer */
  if (request->if_qp_connection)
  {
    if (!entry_rc_rdma)
    {
      sctk_nodebug("CONNECTION requested");
      sctk_nodebug("LID %d, qp_num %d, psn %d", request->keys.lid, request->keys.qp_num, request->keys.psn);

      entry_rc_rdma = sctk_net_ibv_comp_rc_rdma_allocate_init(request->src_process, local_rc_rdma);
      assume(entry_rc_rdma);

      sctk_net_ibv_allocator_register(request->src_process, entry_rc_rdma, IBV_CHAN_RC_RDMA);
      sctk_nodebug("return entry_rc_rdma for process %d: %p", request->src_process, entry_rc_rdma);
    }

    if(entry_rc_rdma->ready == 0)
    {
      sctk_net_ibv_comp_rc_rdma_modify_qp(&entry_rc_rdma->remote, &request->keys);
      entry_rc_rdma->ready = 1;
    }
    sctk_nodebug("CONNECTED !");
  } else
  {
    sctk_nodebug("CONNECTION already established");
    entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
      sctk_net_ibv_allocator_get(request->src_process, IBV_CHAN_RC_RDMA);
  }

  sctk_nodebug("Request received from process %d with psn %d", request->src_process, request->psn);

  /*-----------------------------------------------------------
   *  REUSE AN ENTRY
   *----------------------------------------------------------*/
  //  second_entry_recv = sctk_buffered_fifo_get_elem_from_tail(&entry_rc_rdma->recv, 1);
  //
  //  if (second_entry_recv != NULL)
  //  {
  //    sctk_nodebug("request pending");
  //  }

  sctk_nodebug("Requested size : %lu", size);
  entry_recv.requested_size = size;
  memcpy(&entry_recv.msg_header, &request->msg_header, sizeof(sctk_thread_ptp_message_t));
  entry_recv.src_process = request->src_process;
  entry_recv.if_qp_connection = request->if_qp_connection;
  entry_recv.psn = request->psn;
  entry_recv.used = 0;
  entry_recv.sent_to_mpc = 0;

  sctk_spinlock_lock(&entry_rc_rdma->lock);
  sctk_buffered_fifo_push(&entry_rc_rdma->recv, &entry_recv);
  sctk_spinlock_unlock(&entry_rc_rdma->lock);

  return entry_rc_rdma;
}


/*-----------------------------------------------------------
 *  REQUEST PROCESSING
 *----------------------------------------------------------*/
sctk_net_ibv_rc_rdma_entry_recv_t *
sctk_net_ibv_comp_rc_rdma_check_pending_request(
    sctk_net_ibv_rc_rdma_process_t *entry_rc_rdma)
{
  sctk_net_ibv_rc_rdma_entry_recv_t *entry_recv = NULL,
                                    *last_entry_recv = NULL;
  int i = 0;

  do
  {
    entry_recv = sctk_buffered_fifo_get_elem_from_tail(&entry_rc_rdma->recv, i);

    if ( (entry_recv) && (entry_recv->used == 1) && (entry_recv->sent_to_mpc == 0) ) {
      //FIXME
      //      sctk_warning("Entry not processed, sent_to_mpc : %d", entry_recv->sent_to_mpc);
      last_entry_recv = NULL;
      break;
    }

    if ( (entry_recv) && (entry_recv->used == 0))
    {
      last_entry_recv = entry_recv;
      break;
    }

    sctk_nodebug("inc %d", i);
    i++;
  } while( (entry_recv != NULL) );

  return last_entry_recv;
}

sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_send_msg(
    sctk_net_ibv_qp_rail_t  *rail,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_sr_entry_t* entry)
{
  sctk_net_ibv_rc_rdma_request_ack_t* ack;
  sctk_net_ibv_rc_rdma_entry_send_t*  entry_send;
  sctk_net_ibv_rc_rdma_process_t*              entry_rc_rdma;
  struct  ibv_send_wr* bad_wr = NULL;

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
  }

  entry_send = sctk_buffered_fifo_get_elem_from_tail(&entry_rc_rdma->send, 0);
  assume(entry_send);

  sctk_nodebug("RDVZ ACK for process %lu", ack->src_process);

  /* fill the remote addr */
  entry_send->send_wr.wr.rdma.remote_addr = (uintptr_t) ack->dest_ptr;
  entry_send->send_wr.wr.rdma.rkey        = ack->dest_rkey;
  sctk_nodebug("Dest rkey : %lu from %d ptr %p size %lu", ack->dest_rkey, ack->src_process, ack->dest_ptr, entry_send->size);


  /* finally transfert the msg */
  ibv_post_send(entry_rc_rdma->remote.qp,
      &(entry_send->send_wr), &bad_wr);

  sctk_nodebug("sent");
  return entry_rc_rdma;
}

void
sctk_net_ibv_comp_rc_rdma_done(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_rc_sr_buff_t  *buff_rc_sr,
    sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma)
{
  sctk_net_ibv_rc_sr_entry_t*   entry;
  sctk_net_ibv_rc_rdma_done_t*  done;
  sctk_net_ibv_qp_remote_t*     remote_rc_sr;

  sctk_nodebug("Call rc_rdma_done");
  entry = sctk_net_ibv_comp_rc_sr_pick_header(buff_rc_sr);

  done = (sctk_net_ibv_rc_rdma_done_t* ) &(entry->msg_header->payload);
  done->src_process = sctk_process_rank;

  sctk_nodebug("\t\t SEND DONE MESSAGE to process %lu", entry_rc_rdma->remote.rank);

  remote_rc_sr = sctk_net_ibv_comp_rc_sr_check_and_connect(
      entry_rc_rdma->remote.rank);

  sctk_net_ibv_comp_rc_sr_send(remote_rc_sr, entry,
      sizeof(sctk_net_ibv_rc_rdma_done_t), RC_SR_RDVZ_DONE);
  sctk_nodebug("Done SENT");
}

sctk_net_ibv_rc_rdma_entry_recv_t*
sctk_net_ibv_comp_rc_rdma_match_read_msg(
    sctk_net_ibv_rc_sr_entry_t* entry)
{
  sctk_net_ibv_rc_rdma_done_t*  done;
  sctk_net_ibv_rc_rdma_process_t*       entry_rc_rdma;
  sctk_net_ibv_rc_rdma_entry_recv_t*  entry_recv;
  int i = 0;

  done = (sctk_net_ibv_rc_rdma_done_t*) &(entry->msg_header->payload);

  sctk_nodebug("src process : %d", done->src_process);
  entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
    sctk_net_ibv_allocator_get(done->src_process, IBV_CHAN_RC_RDMA);
  assume(entry_rc_rdma);
  sctk_nodebug("entry_rc_rdma for process %d: %p ", done->src_process, entry_rc_rdma);

  do
  {
    entry_recv = sctk_buffered_fifo_get_elem_from_tail(&entry_rc_rdma->recv, i);

    sctk_nodebug("Entry found %p %d", entry_recv, entry_rc_rdma->recv.elem_count);
    if ( (entry_recv) && (entry_recv->used == 1) && (entry_recv->sent_to_mpc == 0) ) {
      return entry_recv;
    }
    i++;
  } while( (entry_recv != NULL) );
  return NULL;
}

void
sctk_net_ibv_comp_rc_rdma_read_msg(
    sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv)
{
  entry_recv->sent_to_mpc = 1;
  /* send the msg to MPC */

  sctk_nodebug("READ RDVZ message %lu", entry_recv->psn);
  sctk_net_ibv_send_msg_to_mpc(&entry_recv->msg_header, entry_recv->ptr, entry_recv->src_process);
}

sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_comp_rc_rdma_free_msg(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_thread_ptp_message_t * item)
{
  sctk_net_ibv_qp_remote_t        *remote;
  sctk_net_ibv_rc_rdma_process_t          *entry_rdma = NULL;
  sctk_net_ibv_rc_rdma_entry_recv_t *entry_recv;
  int i;

  for (i=0; i < sctk_process_number; ++i)
  {
    entry_rdma = (sctk_net_ibv_rc_rdma_process_t*)
      sctk_net_ibv_allocator_get(i, IBV_CHAN_RC_RDMA);

    if(entry_rdma)
    {
      entry_recv = sctk_buffered_fifo_get_elem_from_tail(&entry_rdma->recv, 0);
      sctk_nodebug("List counter : %d", entry_rdma->recv.elem_count);

      sctk_nodebug("msg %p <-> %p item", entry_recv, item);

      if ( &(entry_recv->msg_header) == item)
      {
        sctk_nodebug("FOUND process %d %p", entry_rdma->remote.rank, entry_recv);

        sctk_nodebug("FREE PSN FROM %lu with psn %lu", entry_recv->src_process, entry_recv->psn);

        /* free the memory */
        free(entry_recv->ptr);

        sctk_net_ibv_mmu_unregister (rail->mmu, entry_recv->mmu_entry);

        sctk_spinlock_lock(&entry_rdma->lock);
        sctk_buffered_fifo_pop_elem(&entry_rdma->recv, NULL);
        sctk_spinlock_unlock(&entry_rdma->lock);

        return entry_rdma;
      }
    }

  }
  return NULL;
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

  sctk_nodebug("NEW SEND src_qp : %lu", wc->qp_num);

  entry_rc_rdma = sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(wc->qp_num);
  entry_send = sctk_buffered_fifo_get_elem_from_tail(&entry_rc_rdma->send, 0);

  if (entry_send->directly_pinned)
  {
    sctk_nodebug("Message directly pinned");
    /* FIXME */
    entry_send->msg->completion_flag = 1;
  } else {
    sctk_free(entry_send->list.addr);
  }

  sctk_net_ibv_comp_rc_rdma_done(
      rc_sr_local,
      buff_rc_sr,
      entry_rc_rdma);

  sctk_net_ibv_mmu_unregister (rail->mmu, entry_send->mmu_entry);

  sctk_spinlock_lock(&entry_rc_rdma->lock);
  sctk_buffered_fifo_pop_elem(&entry_rc_rdma->send, NULL);
  sctk_spinlock_unlock(&entry_rc_rdma->lock);
}

  void
sctk_net_ibv_rc_rdma_poll_recv()
{
  /* VOID */
  assume(0);
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

  rdma->lock = 0;
  sctk_nodebug("SEND");
  sctk_buffered_fifo_new(&(rdma->send), sizeof(sctk_net_ibv_rc_rdma_entry_send_t), 32, 0);
  sctk_nodebug("RECV");
  sctk_buffered_fifo_new(&(rdma->recv), sizeof(sctk_net_ibv_rc_rdma_entry_recv_t), 32, 0);

  rdma->ready = 0;

  sctk_nodebug("QP registered");
  return rdma;
}

