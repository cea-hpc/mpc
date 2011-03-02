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

#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_scheduling.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_cm.h"
#include "sctk_infiniband_coll.h"
#include "sctk_infiniband_lib.h"
#include "sctk_infiniband_qp.h"
#include "sctk_infiniband_ibufs.h"
#include "sctk_alloc.h"
#include "sctk.h"
#include "sctk_net_tools.h"

static uint32_t wr_current = 0;

/* collective */
extern struct sctk_list broadcast_fifo;
extern struct sctk_list reduce_fifo;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

  sctk_net_ibv_qp_local_t*
sctk_net_ibv_comp_rc_sr_create_local(sctk_net_ibv_qp_rail_t* rail)
{
  sctk_net_ibv_qp_local_t* local;
  struct ibv_srq_init_attr srq_init_attr;

  local = sctk_net_ibv_qp_new(rail, 1);

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


static void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc)
{
  sctk_net_ibv_rc_sr_poll_send(wc);
}

#if 0
  sctk_net_ibv_rc_sr_entry_t*
sctk_net_ibv_comp_rc_sr_pick_header(sctk_net_ibv_ibuf_t* buff)
{
  int i;
  int random;

  for (;;)
  {
    random = rand() % buff->slot_nb;

    /* Garbage collector */
    if (buff->current_nb > buff->ceiling)
    {
      int nb_freed;
      int total_freed = 0;
//      sctk_warning("IB is running out of send buffers.\nStarting Garbage Collector...");
      do
      {
        nb_freed = sctk_net_ibv_cq_garbage_collector(rc_sr_local->send_cq, SCTK_PENDING_OUT_NUMBER, sctk_net_ibv_rc_sr_send_cq, IBV_CHAN_RC_SR | IBV_CHAN_SEND);
        total_freed+=nb_freed;
      } while(nb_freed > 0);
//      sctk_warning("Garbage Collector done.\n - Number of buffers freed: %d\n - Number of busy buffers: %d", total_freed, buff->current_nb);
    }


    if (sctk_thread_mutex_trylock(&buff->lock) == 0)
    {
      for (i = random; i < buff->slot_nb; ++i)
      {
        sctk_nodebug("Slot %d (used:%d)", i, buff->headers[i].used);
        if ( buff->headers[i].used == 0 )
       {
          sctk_nodebug("RC_SR slot %d found ! (%p) ", i, &buff->headers[i]);
          buff->headers[i].used = 1;
          buff->current_nb++;
          if (buff->current_nb > 280 )
            sctk_warning("PICK Remaining buffers : %d", buff->current_nb);
          sctk_thread_mutex_unlock(&buff->lock);
          return &(buff->headers[i]);
        }
      }
      sctk_nodebug("No header found");

      sctk_thread_mutex_unlock(&buff->lock);
    }

    sctk_thread_yield();
  }
  assume(0);
}
#endif

uint32_t
sctk_net_ibv_comp_rc_sr_send(
    sctk_net_ibv_qp_remote_t* remote,
    sctk_net_ibv_ibuf_t* ibuf, size_t size,
    sctk_net_ibv_rc_sr_msg_type_t type, uint32_t* psn)
{
  uint32_t ret_psn = 0;
  int rc;
  sctk_net_ibv_rc_sr_msg_header_t* msg_header;

  sctk_net_ibv_ibuf_send_init(ibuf, size + RC_SR_HEADER_SIZE);
  msg_header = ((sctk_net_ibv_rc_sr_msg_header_t*) ibuf->buffer);
  msg_header->msg_type = type;
  msg_header->src_process = sctk_process_rank;
  msg_header->size = size + RC_SR_HEADER_SIZE;

  /* If the psn variable is != NULL, the PSN has to be computed before
   * sending the message */
  if (psn != NULL)
  {
    sctk_net_ibv_sched_lock();
    *psn = sctk_net_ibv_sched_psn_inc(remote->rank);
    ret_psn = *psn;
    msg_header->psn = ret_psn;

    rc = ibv_post_send(remote->qp , &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
    assume (rc == 0);
    sctk_net_ibv_sched_unlock();
    sctk_nodebug("Send EAGER message to %d and size %lu (PSN:%d)",
        remote->rank, entry->msg_header->size, *psn);

  } else {
    sctk_nodebug("QP %p dest %d size %lu", remote->qp, remote->rank, size);
//    sctk_debug("lkey : %lu", ibuf->desc.sg_entry.lkey);
    rc = ibv_post_send(remote->qp , &(ibuf->desc.wr.send), &(ibuf->desc.bad_wr.send));
    assume (rc == 0);
  }

  if (type == RC_SR_RDVZ_REQUEST)
  {
    sctk_nodebug("PSN : %d(%p)", *psn, psn);
  }

  return ret_psn;
}

  void
sctk_net_ibv_comp_rc_sr_free(sctk_net_ibv_ibuf_t* entry)
{
  sctk_free(entry);
}

  sctk_net_ibv_qp_remote_t*
sctk_net_ibv_comp_rc_sr_check_and_connect(int dest_process)
{
  sctk_net_ibv_qp_remote_t* remote;
  sctk_net_ibv_allocator_lock(dest_process, IBV_CHAN_RC_SR);

  remote = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_SR);

  //FIXME Check connection
  if (!remote)
  {
    sctk_nodebug("Connexion to process %d", dest_process);
    char host[256];
    int port;

    remote = sctk_net_ibv_comp_rc_sr_allocate_init(dest_process, rc_sr_local);
    sctk_net_ibv_allocator_register(dest_process, remote, IBV_CHAN_RC_SR);
    sctk_net_ibv_allocator_unlock(dest_process, IBV_CHAN_RC_SR);

    sctk_net_ibv_cm_request(dest_process, remote, host, &port);
    sctk_net_ibv_cm_client(host, port, dest_process, remote);

  } else {
    sctk_net_ibv_allocator_unlock(dest_process, IBV_CHAN_RC_SR);

    while(remote->is_connected != 1)
    {
      sctk_thread_yield();
    }
    sctk_nodebug("Remote known");
  }
  assume(remote);

  sctk_nodebug("Send message to process %d", dest_process);

  return remote;
}

void
sctk_net_ibv_comp_rc_sr_send_ptp_message (
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void* msg, int dest_process, size_t size, sctk_net_ibv_rc_sr_msg_type_t type) {


  sctk_net_ibv_ibuf_t* ibuf;
  sctk_net_ibv_qp_remote_t* remote;
  sctk_thread_ptp_message_t* ptp_msg;
  void* payload;
  uint32_t psn;

  ibuf = sctk_net_ibv_ibuf_pick();
  payload = RC_SR_PAYLOAD(ibuf);

  sctk_nodebug("Send rc_sr message. type %d, size %d, src_process %lu", type, size);

  /* check if the TCP connection is active. If not, connect peers */
  remote = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);

  /* if the msg to send is a PTP msg */
  if (type == RC_SR_EAGER)
  {
    ptp_msg = (sctk_thread_ptp_message_t*) msg;
    memcpy (payload, ptp_msg, sizeof ( sctk_thread_ptp_message_t ));
    sctk_net_copy_in_buffer(ptp_msg, payload + sizeof ( sctk_thread_ptp_message_t ));
    ptp_msg->completion_flag = 1;
    size += sizeof(sctk_thread_ptp_message_t);

    /* lock to be sure that the message that we send has the good
     * PSN */
    sctk_net_ibv_comp_rc_sr_send(remote, ibuf, size, type, &psn);
    sctk_nodebug("Send PTP %lu to %d with psn %lu", size, dest_process, psn);
  }
  /* if this is a COLLECTIVE msg */
  else if ( (type == RC_SR_BCAST) ||
      (type == RC_SR_REDUCE) ||
      (type == RC_SR_BCAST_INIT_BARRIER))
  {
    memcpy (payload, msg, size);
    sctk_net_ibv_comp_rc_sr_send(remote, ibuf, size, type, NULL);
  }
  else
  {
    sctk_error("Type of message not known");
    assume(0);
  }

  sctk_nodebug("Eager msg sent to process %lu!", dest_process);
}

  void*
sctk_net_ibv_comp_rc_sr_prepare_recv(void* msg, size_t size)
{
  void* ptr;
  sctk_nodebug("SIZE : %lu", size);

  ptr = sctk_malloc(size);
  memcpy(ptr, msg, size);

  return ptr;
}

/*-----------------------------------------------------------
 *  POLLING FUNCTIONS
 *----------------------------------------------------------*/
void
sctk_net_ibv_rc_sr_poll_send(
    struct ibv_wc* wc)
{
  sctk_net_ibv_ibuf_t* ibuf;

  ibuf = (sctk_net_ibv_ibuf_t*) wc->wr_id;
  sctk_nodebug("New Send cq %p", ibuf);

  switch (ibuf->flag)
  {
    case BARRIER_IBUF_FLAG:
      break;

    case NORMAL_IBUF_FLAG:
      sctk_net_ibv_ibuf_release(ibuf);
      break;

    case RDMA_READ_IBUF_FLAG:
      break;

    case RDMA_WRITE_IBUF_FLAG:
      break;

    default: assume(0); break;

  }
}

void
sctk_net_ibv_rc_sr_poll_recv(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_rail_t      *rail,
    sctk_net_ibv_qp_local_t     *rc_sr_local,
    sctk_net_ibv_qp_local_t     *rc_rdma_local,
    int lookup_mode,
    int dest)
{
  sctk_net_ibv_ibuf_t* ibuf;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  sctk_net_ibv_rc_sr_msg_type_t msg_type;
  sctk_net_ibv_rc_sr_msg_header_t* msg_header;
  int ret;
  void* msg;

  ibuf = (sctk_net_ibv_ibuf_t*) wc->wr_id;
  msg_header = ((sctk_net_ibv_rc_sr_msg_header_t*) ibuf->buffer);

  msg_type = msg_header->msg_type;

  if (lookup_mode)
  {
    sctk_nodebug("Lookup mode for dest %d and ESN %lu", dest, sctk_net_ibv_sched_get_esn(dest));
  }

  switch(msg_type) {

    case RC_SR_EAGER:
      sctk_nodebug("EAGER recv");

      sctk_nodebug("READ EAGER message %lu from %d and size %lu",
          msg_header->psn, msg_header->src_process, msg_header->size);

      if (lookup_mode)
      {
        ret = sctk_net_ibv_sched_sn_check_and_inc(
            msg_header->src_process,
            msg_header->psn);

        /* message not expected */
        if(ret)
        {
          sctk_nodebug("LOOKUP UNEXPECTED - Found psn %d from %d",
              msg_header->psn, msg_header->src_process);

          sctk_net_ibv_sched_pending_push(
              msg_header, msg_header->size, 1,
              IBV_CHAN_RC_SR);

          /* expected message */
        } else {
          sctk_nodebug("LOOKUP EXPECTED - Found psn %d from %d",
              msg_header->psn, msg_header->src_process);

          /* copy the message to buffer */
          msg = sctk_net_ibv_comp_rc_sr_prepare_recv(
              &(msg_header->payload),
              msg_header->size);

          /* send msg to MPC */
          sctk_net_ibv_send_msg_to_mpc(
              (sctk_thread_ptp_message_t*) msg,
              (char*) msg + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
              IBV_RC_SR_ORIGIN, NULL);
        }

        /* normal mode */
      } else {
        sctk_nodebug("NORMAL mode");
        ret = sctk_net_ibv_sched_sn_check_and_inc(msg_header->src_process, msg_header->psn);

        if (ret)
        {
          sctk_nodebug("EXPECTED %lu but GOT %lu from process %d", sctk_net_ibv_sched_get_esn(msg_header->src_process), msg_header->psn, msg_header->src_process);
          do {
            /* enter to the lookup mode */
            sctk_net_ibv_allocator_ptp_lookup_all(
                msg_header->src_process);

            ret = sctk_net_ibv_sched_sn_check_and_inc(
                msg_header->src_process, msg_header->psn);

            sctk_thread_yield();
          } while (ret);
        }

        sctk_nodebug("Recv EAGER message from %d (PSN %d)", msg_header->src_process, msg_header->psn);

        /* copy the message to buffer */
        msg = sctk_net_ibv_comp_rc_sr_prepare_recv(
            &(msg_header->payload),
            msg_header->size);

        /* send msg to MPC */
        sctk_net_ibv_send_msg_to_mpc(
            (sctk_thread_ptp_message_t*) msg,
            (char*) msg + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
            IBV_RC_SR_ORIGIN, NULL);

      }
      /* reset buffers */
//      restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);

      sctk_nodebug("(EAGER) MSG PTP received %lu", msg_header->size);
      break;
#if 0
    case RC_SR_RDVZ_REQUEST:
      sctk_nodebug("RDVZ REQUEST recv");

      entry_rc_rdma =
        sctk_net_ibv_comp_rc_rdma_analyze_request(
            rail, rc_sr_local, rc_rdma_local,
            entry);

      sctk_net_ibv_allocator_rc_rdma_process_next_request(entry_rc_rdma);

      restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
      sctk_nodebug("exit");
      break;

    case RC_SR_RDVZ_ACK:
      sctk_nodebug("RDVZ ACK recv");
      sctk_net_ibv_comp_rc_rdma_send_msg(rail, rc_rdma_local,
          entry);

      restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
      break;

    case RC_SR_RDVZ_DONE:
      assume(0);

#endif

    case RC_SR_BCAST:
      sctk_nodebug("Broadcast msg received");
      sctk_net_ibv_collective_push(&broadcast_fifo, msg_header);
      break;

    case RC_SR_REDUCE:
      sctk_nodebug("Reduce msg received");
      sctk_net_ibv_collective_push(&reduce_fifo, msg_header);
      break;

    case RC_SR_BCAST_INIT_BARRIER:
      sctk_nodebug("Broadcast barrier msg received");
      sctk_net_ibv_collective_push(&init_barrier_fifo, msg_header);
      break;
  }

  --ibuf_free_srq_nb;
  sctk_nodebug("Buffer %d posted", ibuf_free_srq_nb);
  sctk_net_ibv_ibuf_release(ibuf);
  sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local);

  if (lookup_mode)
    sctk_nodebug("Finished Lookup mode");
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

void
sctk_net_ibv_comp_rc_sr_allocate_recv(
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_qp_remote_t *remote,
    int rank)
{
  sctk_net_ibv_qp_exchange_keys_t keys;
  /* set the rank of the remote QP */
  keys = sctk_net_ibv_qp_exchange_recv(1, local, rank);
  sctk_nodebug("Keys received from process %d", rank);

  sctk_net_ibv_qp_allocate_recv(remote, &keys);
}

/*-----------------------------------------------------------
 *  ERROR HANDLING
 *----------------------------------------------------------*/
  void
sctk_net_ibv_comp_rc_sr_error_handler(struct ibv_wc wc)
{

}
