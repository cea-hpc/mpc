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

#include "sctk_ib_comp_rc_sr.h"
#include "sctk_ib_comp_rc_rdma.h"
#include "sctk_ib_frag.h"
#include "sctk_ib_scheduling.h"
#include "sctk_ib_allocator.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_coll.h"
#include "sctk_ib_lib.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_rpc.h"
#include "sctk_alloc.h"
#include "sctk.h"
#include "sctk_net_tools.h"
#include "sctk_ib_profiler.h"


/* channel selection */
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
extern sctk_net_ibv_com_entry_t* com_entries;
extern sctk_net_ibv_allocator_task_t all_tasks[MAX_NB_TASKS_PER_PROCESS];
extern sctk_spinlock_t all_tasks_lock;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

  sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_sr_create_local(sctk_net_ibv_qp_rail_t* rail)
{
  sctk_net_ibv_qp_local_t* local;
  struct ibv_srq_init_attr srq_init_attr;

  local = sctk_net_ibv_qp_new(rail);

  sctk_net_ibv_pd_init(local);
  sctk_nodebug("New PD %p", local->pd);

  /* completion queues */
  local->send_cq = sctk_net_ibv_cq_init(rail);
  local->recv_cq = sctk_net_ibv_cq_init(rail);

  /* shared receive queues */
  srq_init_attr = sctk_net_ibv_srq_init_attr();
  sctk_net_ibv_srq_init(local, &srq_init_attr);

  return local;
}


void
sctk_net_ibv_comp_rc_sr_send(
    sctk_net_ibv_ibuf_t* ibuf,
    sctk_net_ibv_allocator_request_t req, sctk_net_ibv_ibuf_type_t type)
{
  sctk_net_ibv_ibuf_header_t* msg_header;
  int task_id;
  int thread_id;

  /* get infos from the thread */
  sctk_get_thread_info (&task_id, &thread_id);

  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

  msg_header->ibuf_type   = type;
  msg_header->ptp_type    = req.ptp_type;
  msg_header->src_process = sctk_process_rank;
  msg_header->src_task    = task_id;
  msg_header->dest_task   = req.dest_task;
  msg_header->size        = req.size;
  msg_header->psn         = req.psn;
  msg_header->com_id      = req.com_id;
  msg_header->total_copied= req.total_copied;

  msg_header->payload_size = req.payload_size;
  msg_header->buff_nb     = req.buff_nb;
  msg_header->total_buffs = req.total_buffs;

  sctk_net_ibv_allocator_request_show(&req);

  ibuf->dest_process = req.dest_process;
  /* We have to check if there are free slots for the
   * selected QP
   */
  sctk_net_ibv_qp_send_get_wqe(req.dest_process, ibuf);

  sctk_nodebug("Send frag PTP %lu to %d (task %d) with psn %lu",
      req.size, req.dest_process, req.dest_task, req.psn);
}

  void
sctk_net_ibv_comp_rc_sr_free(sctk_net_ibv_ibuf_t* entry)
{
  sctk_free(entry);
  sctk_ibv_profiler_dec(IBV_MEM_TRACE);
}


  sctk_net_ibv_qp_remote_t*
sctk_net_ibv_comp_rc_sr_check_and_connect(int dest_process)
{
  sctk_net_ibv_qp_remote_t* remote;

  ALLOCATOR_LOCK(dest_process, IBV_CHAN_RC_SR);
  remote = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_SR);

  /* double check for avoinding to take a look if we dont need it */
  //  if (!remote)
  //  {
  //    remote = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_SR);

  if (!remote)
  {
    sctk_nodebug("Connexion to process %d", dest_process);
    char host[256];
    int port;
    sctk_net_ibv_cm_request(dest_process, host, &port);

    remote = sctk_net_ibv_comp_rc_sr_allocate_init(dest_process, rc_sr_local);
    sctk_net_ibv_allocator_register(dest_process, remote, IBV_CHAN_RC_SR);
    ALLOCATOR_UNLOCK(dest_process, IBV_CHAN_RC_SR);

    sctk_net_ibv_cm_client(host, port, dest_process, remote);
  } else {
    ALLOCATOR_UNLOCK(dest_process, IBV_CHAN_RC_SR);
  }

  while(remote->is_connected != 1)
  {
    sctk_thread_yield();
  }
  sctk_nodebug("Send message to process %d", dest_process);

  assume(remote);
  return remote;
}


/**
 *  Send an eager message to a remote process.
 *  The message can be a PTP or a collective.
 *  \param
 *  \return
 */
void
sctk_net_ibv_comp_rc_sr_send_ptp_message (
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_allocator_request_t req) {

  sctk_net_ibv_ibuf_t* ibuf;
  sctk_thread_ptp_message_t* ptp_msg;
  void* payload;

  ibuf = sctk_net_ibv_ibuf_pick(0, 1);
  payload = (void*) RC_SR_PAYLOAD(ibuf->buffer);

  if (sctk_process_rank == 0)
    sctk_nodebug("Send rc_sr message. type %d, size %d, src_process %lu", req.type, req.size, req.dest_process);

  /* if the msg to send is a PTP msg */
  if (req.ptp_type == IBV_PTP)
  {
    ptp_msg = (sctk_thread_ptp_message_t*) req.msg;
    memcpy (payload, ptp_msg, sizeof ( sctk_thread_ptp_message_t ));
    sctk_net_copy_in_buffer(ptp_msg, payload + sizeof ( sctk_thread_ptp_message_t ));
    ptp_msg->completion_flag = 1;

    sctk_nodebug("Send PTP %lu to %d with psn %lu", size, req.dest_process, psn);
  }
  /* if this is a COLLECTIVE msg */
  else
  {
    sctk_nodebug("size : %lu", req.size);
    sctk_net_ibv_allocator_request_show(&req);
    memcpy (payload, req.msg, req.size);
  }

  /* send the message */
  sctk_net_ibv_ibuf_send_init(ibuf, req.size + RC_SR_HEADER_SIZE);
  req.buff_nb     = 1;
  req.total_buffs = 1;
  req.payload_size = req.size;
  sctk_net_ibv_comp_rc_sr_send(ibuf, req, RC_SR_EAGER);


  sctk_nodebug("Eager msg sent to process %lu!", req.dest_process);
}

  static int
sctk_net_ibv_comp_rc_sr_ptp_recv(sctk_net_ibv_ibuf_t* ibuf ,sctk_net_ibv_ibuf_header_t* msg_header, int *release_buffer)
{
  int ret = -1;
  /*
   * If this is a fragmented msg
   */
  if (msg_header->total_buffs != 1)
  {
    sctk_net_ibv_comp_rc_sr_frag_recv(msg_header);
    *release_buffer = 1;
  }
  /*
   * If this is a normal eager msg
   */
  else {

    ret = sctk_net_ibv_sched_sn_check_and_inc(
        msg_header->dest_task,
        msg_header->src_task,
        msg_header->psn);

    /* message not expected */
    if(ret)
    {
      sctk_ibv_profiler_inc(IBV_LOOKUP_UNEXPECTED_MSG_NB);
      sctk_nodebug("LOOKUP UNEXPECTED - Found psn %d from %d (task %d)",
          msg_header->psn, msg_header->src_process, msg_header->src_task);

      sctk_net_ibv_sched_pending_push(
          RC_SR_PAYLOAD(msg_header), msg_header->size, 0,
          IBV_POLL_RC_SR_ORIGIN,
          msg_header->src_process,
          msg_header->src_task,
          msg_header->dest_task,
          msg_header->psn,
          (sctk_thread_ptp_message_t*) RC_SR_PAYLOAD(msg_header),
          (char*) RC_SR_PAYLOAD(msg_header) + sizeof(sctk_thread_ptp_message_t),
          ibuf);

      /* expected message */
    } else {
      sctk_ibv_profiler_inc(IBV_LOOKUP_EXPECTED_MSG_NB);
      sctk_nodebug("LOOKUP EXPECTED - Found psn %d from %d",
          msg_header->psn, msg_header->src_process);

      /* send msg to MPC */
      sctk_net_ibv_send_msg_to_mpc(
          (sctk_thread_ptp_message_t*) RC_SR_PAYLOAD(msg_header),
          (char*) RC_SR_PAYLOAD(msg_header) + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
          IBV_RC_SR_ORIGIN, ibuf);
    }

    /* we don't release the buffer for now.
     * It will be released when the msg will be freed */
    *release_buffer = 0;

    sctk_nodebug("(EAGER) MSG PTP received %lu", msg_header->size);
  }
  return ret;
}


/*-----------------------------------------------------------
 *  POLLING FUNCTIONS
 *----------------------------------------------------------*/

/**
 *  Poll the CQ for sent msg
 *  \param
 *  \return
 */
void
sctk_net_ibv_rc_sr_poll_send(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_local_t* rc_sr_local)
{
  sctk_net_ibv_ibuf_t* ibuf;
  sctk_net_ibv_ibuf_header_t* msg_header;

  ibuf = (sctk_net_ibv_ibuf_t*) wc->wr_id;
  /*
   * free the WQE from the remote QP and try to post the
   * queued WQE.
   */
  sctk_net_ibv_qp_send_free_wqe(ibuf->remote);
  sctk_net_ibv_qp_send_post_pending(ibuf->remote, 1);

  switch (ibuf->flag)
  {
    case BARRIER_IBUF_FLAG:
      break;

    case NORMAL_IBUF_FLAG:
      sctk_net_ibv_ibuf_release(ibuf, 0);
      break;

    case RDMA_READ_IBUF_FLAG:
      msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

      /* if this a RDMA from a RPC call, we send the ack */
      if (msg_header->ibuf_type == RC_SR_RPC_READ)
      {
        sctk_nodebug("\t\tRPC READ received");
        sctk_net_rpc_send_ack(ibuf);
      }
      /* if this is a RDMA from a PTP call */
      else {
        /* we send the ack to the sender... */
        sctk_net_ibv_comp_rc_rdma_send_finish(
            rail, rc_sr_local, rc_rdma_local, ibuf);

        /* ... and we read the recv msg */
        sctk_net_ibv_com_rc_rdma_read_finish(
            ibuf, rc_sr_local);
      }

      sctk_net_ibv_ibuf_release(ibuf, 0);
      break;

    case RDMA_WRITE_IBUF_FLAG:
      not_implemented();
      break;

    default: assume(0); break;
  }
}


/**
 *  Poll the CQ for received msg
 *  \param
 *  \return
 */
void
sctk_net_ibv_rc_sr_poll_recv(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_rail_t      *rail,
    sctk_net_ibv_qp_local_t     *rc_sr_local,
    sctk_net_ibv_qp_local_t     *rc_rdma_local,
    int dest)
{
  sctk_net_ibv_ibuf_t* ibuf;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_ibuf_header_t* msg_header;
  int release_buffer = 1;
  sctk_net_ibv_rpc_ack_t* ack;

  ibuf = (sctk_net_ibv_ibuf_t*) wc->wr_id;
  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

  sctk_nodebug("Message received from proc %d (task %d)",
      msg_header->src_process, msg_header->src_task);

  /* Switch on the type of the eager msg: eager, rdma request */
  switch(msg_header->ibuf_type) {

    case RC_SR_EAGER:
      sctk_nodebug("READ EAGER message %lu from %d (task %d) and size %lu",
          msg_header->psn, msg_header->src_process, msg_header->src_task,
          msg_header->size);

      switch (msg_header->ptp_type)
      {

        case IBV_PTP:
          sctk_net_ibv_comp_rc_sr_ptp_recv(ibuf, msg_header,
              &release_buffer);
          break;

        case IBV_BCAST:
          sctk_nodebug("POLL Broadcast msg received for comm %d",
              msg_header->com_id);
          sctk_net_ibv_collective_push_rc_sr(
              &com_entries[msg_header->com_id].broadcast_fifo, ibuf, &release_buffer, msg_header->ptp_type);
          break;

        case IBV_REDUCE:
          sctk_nodebug("POLL Reduce msg received for comm %d",
              msg_header->com_id);
          sctk_net_ibv_collective_push_rc_sr(
              &com_entries[msg_header->com_id].reduce_fifo, ibuf, &release_buffer, msg_header->ptp_type);
          break;

        case IBV_BCAST_INIT_BARRIER:
          sctk_nodebug("Coll id: %d -> %p", msg_header->com_id, &com_entries[msg_header->com_id].init_barrier_fifo);
          sctk_nodebug("Broadcast barrier msg received from com %d", msg_header->com_id);
          sctk_net_ibv_collective_push_rc_sr(
              &com_entries[msg_header->com_id].init_barrier_fifo, ibuf, &release_buffer, msg_header->ptp_type);
          break;

        case IBV_BARRIER:
          sctk_nodebug("Barrier msg received from com %d", msg_header->com_id);
          sctk_net_ibv_collective_push_rc_sr(
              &com_entries[msg_header->com_id].barrier_fifo, ibuf, &release_buffer, msg_header->ptp_type);
          break;


        default:
          assume(0);
          break;

      }
      break;

    case RC_SR_RDVZ_REQUEST:
      sctk_nodebug("RDVZ REQUEST recv");

      entry_rc_rdma =
        sctk_net_ibv_comp_rc_rdma_analyze_request(
            rail, rc_sr_local, rc_rdma_local,
            ibuf);
      break;

    case RC_SR_RDVZ_DONE:
      sctk_nodebug("RC_SR_RDVZ_DONE");

      sctk_net_ibv_com_rc_rdma_recv_done(rail,
          rc_sr_local, ibuf);
      break;

    case RC_SR_RPC:
      sctk_nodebug("RPC received from %d", msg_header->src_process);
      sctk_net_rpc_receive(ibuf);
      break;

    case RC_SR_RPC_ACK:
      sctk_nodebug("ACK received from %d", msg_header->src_process);
      ack = (sctk_net_ibv_rpc_ack_t*)
        RC_SR_PAYLOAD(ibuf->buffer);
      assume(ack->ack);
      *((int*) ack->ack) = 1;
      break;

    default:
      assume(0);
      break;
  }

  sctk_nodebug("Release : %d %d", release_buffer, msg_header->ptp_type);
  if (release_buffer)
  {
    sctk_net_ibv_ibuf_release(ibuf, 1);
    sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit, 1);
  }
}


/*-----------------------------------------------------------
 *  ALLOCATION
 *----------------------------------------------------------*/

  sctk_net_ibv_qp_remote_t *
sctk_net_ibv_comp_rc_sr_allocate_init(int rank,
    sctk_net_ibv_qp_local_t* local)
{
  sctk_net_ibv_qp_remote_t *remote;

  remote = sctk_malloc(sizeof (sctk_net_ibv_qp_remote_t));
  memset(remote, 0, sizeof(sctk_net_ibv_qp_remote_t));
  assume(remote);
  sctk_net_ibv_qp_allocate_init(rank, local, remote);

  return remote;
}

void
sctk_net_ibv_comp_rc_sr_allocate_send(
    sctk_net_ibv_qp_rail_t* rail,
    sctk_net_ibv_qp_remote_t *remote)
{
  sctk_net_ibv_qp_exchange_send(1, rail, remote);
}

#endif
