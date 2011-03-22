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
#include "sctk_infiniband_profiler.h"

static uint32_t wr_current = 0;

/* collective */
extern struct sctk_list broadcast_fifo;
extern struct sctk_list reduce_fifo;

/* channel selection */
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_net_ibv_comp_rc_sr_frag_allocate(int src,
    size_t size, uint32_t psn, sctk_net_ibv_rc_sr_msg_header_t* msg_header)
{
  sctk_net_ibv_frag_eager_entry_t* entry;
  struct sctk_list* list;
  list = &sctk_net_ibv_allocator->entry[src].frag_eager;

  entry = sctk_malloc((char*) sizeof(sctk_net_ibv_frag_eager_entry_t) +
      size);
  assume(entry);

  entry->msg = (char*) entry + sizeof(sctk_net_ibv_frag_eager_entry_t);
  entry->psn = psn;
  entry->current_copied = 0;
  entry->src = src;

  sctk_list_lock(list);
  entry->list_elem = sctk_list_push(list, entry);
  sctk_list_unlock(list);

  sctk_nodebug("New entry allocated for msg (size: %lu, PSN %lu, alloc %p)", size, psn, entry);

}

 void
sctk_net_ibv_comp_rc_sr_free_frag_msg(sctk_net_ibv_frag_eager_entry_t* entry)
{
  struct sctk_list* list;
  list = &sctk_net_ibv_allocator->entry[entry->src].frag_eager;

  sctk_list_lock(list);
  sctk_list_remove(list, entry->list_elem);
  sctk_list_unlock(list);
  sctk_nodebug("Free msg %p", entry);
  sctk_free(entry);
}

 sctk_net_ibv_frag_eager_entry_t*
sctk_net_ibv_comp_rc_sr_copy_msg(void* buffer, int src, size_t size, uint32_t psn)
{
  struct sctk_list* list;
  struct sctk_list_elem *tmp = NULL;
  sctk_net_ibv_frag_eager_entry_t* entry;

  list = &sctk_net_ibv_allocator->entry[src].frag_eager;

  sctk_list_lock(list);
  tmp = list->head;

  while (tmp) {

    entry = (sctk_net_ibv_frag_eager_entry_t*) tmp->elem;


    if (psn == entry->psn)
    {
    sctk_nodebug("Found %lu", entry->psn);
      memcpy( (char*) entry->msg + entry->current_copied, buffer, size);
      entry->current_copied+=size;

      sctk_nodebug("Frag msg copied (size: %lu, psn %lu, copied %lu)", size, psn, entry->current_copied);

      sctk_list_unlock(list);
      return entry;
    }

    tmp = tmp->p_next;
  }
  sctk_list_unlock(list);
  return NULL;
}



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

uint32_t
sctk_net_ibv_comp_rc_sr_send(
    sctk_net_ibv_qp_remote_t* remote,
    sctk_net_ibv_ibuf_t* ibuf, size_t size, size_t buffer_size,
    sctk_net_ibv_rc_sr_msg_type_t type, uint32_t* psn, const int buff_nb, const int total_buffs)
{
  uint32_t ret_psn = 0;
  int rc;
  int locked = 0;
  sctk_net_ibv_rc_sr_msg_header_t* msg_header;

  msg_header = ((sctk_net_ibv_rc_sr_msg_header_t*) ibuf->buffer);
  msg_header->msg_type = type;
  msg_header->src_process = sctk_process_rank;
  msg_header->size = size + RC_SR_HEADER_SIZE;
  msg_header->buffer_size = size + RC_SR_HEADER_SIZE;

  msg_header->payload_size = buffer_size;
  msg_header->buff_nb = buff_nb;
  msg_header->total_buffs = total_buffs;

  /* If the psn variable is != NULL, the PSN has to be computed before
   * sending the message */
  if (psn != NULL)
  {
    /* if this is the first buffer from a serial of buffer,
     * we set the PSN to the right value*/
    if (buff_nb == 1)
    {
      sctk_net_ibv_sched_lock();
      locked = 1;
      *psn = sctk_net_ibv_sched_psn_inc(remote->rank);
      ret_psn = *psn;
      msg_header->psn = ret_psn;
    } else {
      /* in the other case, we just copy the current
       * value of psn */
      locked = 0;
      ret_psn = *psn;
      msg_header->psn = ret_psn;
    }

    /*
     * We have to check if there are free slots for the
     * selected QP
     */
    sctk_net_ibv_qp_send_get_wqe(remote, ibuf);

    if (locked) {
      sctk_net_ibv_sched_unlock();
    }

  } else {
    sctk_nodebug("QP %p dest %d size %lu", remote->qp, remote->rank, size);
    //    sctk_nodebug("lkey : %lu", ibuf->desc.sg_entry.lkey);
    sctk_net_ibv_qp_send_get_wqe(remote, ibuf);
  }

  return ret_psn;
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
sctk_net_ibv_comp_rc_sr_send_frag_ptp_message(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void* msg, int dest_process, size_t size, sctk_net_ibv_rc_sr_msg_type_t type)
{
  sctk_net_ibv_ibuf_t* ibuf;
  sctk_net_ibv_qp_remote_t* remote;
  sctk_thread_ptp_message_t* ptp_msg;
  void* payload;
  uint32_t psn;
  size_t offset_buffer = 0;
  size_t offset_copied = 0;
  size_t allowed_copy_size = 0;
  int   nb_buffers = 1;
  int   total_buffs = 0;
  size_t size_payload = 0;

  ptp_msg = (sctk_thread_ptp_message_t*) msg;

  size += sizeof(sctk_thread_ptp_message_t);

  /* check if the TCP connection is active. If not, connect peers */
  remote = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);

  total_buffs = ceilf( (float)
      size/ (ibv_eager_threshold - RC_SR_HEADER_SIZE));

  sctk_nodebug("Send rc_sr message. type %d, size %d, src_process %lu, nb buffs %d", type, size, dest_process, total_buffs);


  /* while it reamins slots to copy */
  while (nb_buffers <= total_buffs)
  {
    ibuf = sctk_net_ibv_ibuf_pick(0, 1);
    payload = RC_SR_PAYLOAD(ibuf);

    /* if this is the first buffer, we  */
    if (nb_buffers == 1)
    {
      sctk_nodebug("Copy header");
      memcpy (payload, ptp_msg, sizeof ( sctk_thread_ptp_message_t ));
      offset_buffer = sizeof ( sctk_thread_ptp_message_t);
    } else {
      offset_buffer = 0;
    }

    allowed_copy_size = ibv_eager_threshold
      - RC_SR_HEADER_SIZE
      - offset_buffer;

    sctk_nodebug("%d - Allowed size to be copied : %lu", nb_buffers, allowed_copy_size);

    size_payload = offset_copied;

    offset_copied = sctk_net_copy_frag_msg(ptp_msg, payload + offset_buffer, offset_copied, allowed_copy_size);

    size_payload = (offset_copied - size_payload) + offset_buffer;

    sctk_nodebug("Msg copied in buffer");

    /* PSN */
    sctk_net_ibv_ibuf_send_init(ibuf, size_payload + RC_SR_HEADER_SIZE);

    sctk_net_ibv_comp_rc_sr_send(remote, ibuf, size, size_payload, type, &psn, nb_buffers, total_buffs);

    sctk_nodebug("Send frag PTP %lu to %d with psn %lu", size_payload, dest_process, psn);

    nb_buffers++;
  }

  ptp_msg->completion_flag = 1;

  sctk_nodebug("Eager msg sent to process %lu!", dest_process);
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

  ibuf = sctk_net_ibv_ibuf_pick(0, 1);
  payload = RC_SR_PAYLOAD(ibuf);

  if (sctk_process_rank == 0)
    sctk_nodebug("Send rc_sr message. type %d, size %d, src_process %lu", type, size, dest_process);

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
    sctk_net_ibv_ibuf_send_init(ibuf, size + RC_SR_HEADER_SIZE);
    sctk_net_ibv_comp_rc_sr_send(remote, ibuf, size, size, type, &psn, 1, 1);
    sctk_nodebug("Send PTP %lu to %d with psn %lu", size, dest_process, psn);
  }
  /* if this is a COLLECTIVE msg */
  else if ( (type == RC_SR_BCAST) ||
      (type == RC_SR_REDUCE) ||
      (type == RC_SR_BCAST_INIT_BARRIER))
  {
    memcpy (payload, msg, size);
    sctk_net_ibv_ibuf_send_init(ibuf, size + RC_SR_HEADER_SIZE);
    sctk_net_ibv_comp_rc_sr_send(remote, ibuf, size, size, type, NULL, 1, 1);
  }
  else
  {
    sctk_error("Type of message not known");
    assume(0);
  }

  sctk_nodebug("Eager msg sent to process %lu!", dest_process);
}

/*-----------------------------------------------------------
 *  POLLING FUNCTIONS
 *----------------------------------------------------------*/
void
sctk_net_ibv_rc_sr_poll_send(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_local_t* rc_sr_local,
    int lookup_mode)
{
  sctk_net_ibv_ibuf_t* ibuf;

  ibuf = (sctk_net_ibv_ibuf_t*) wc->wr_id;
  /*
   * free the WQE from the remote QP and try to post the
   * queued WQE.
   */
  sctk_net_ibv_qp_send_free_wqe(ibuf->remote);
  sctk_net_ibv_qp_send_post_pending(ibuf->remote);

  if (sctk_process_rank == 0)
    sctk_nodebug("New Send cq %p, flag %d", ibuf, ibuf->flag);

  switch (ibuf->flag)
  {
    case BARRIER_IBUF_FLAG:
      break;

    case NORMAL_IBUF_FLAG:
      sctk_net_ibv_ibuf_release(ibuf, 0);
      break;

    case RDMA_READ_IBUF_FLAG:
      sctk_net_ibv_com_rc_rdma_read_finish(
          ibuf, rc_sr_local, lookup_mode);

      /* there */
      sctk_net_ibv_comp_rc_rdma_send_finish(
          rail, rc_sr_local, rc_rdma_local,
          ibuf);

      sctk_net_ibv_ibuf_release(ibuf, 0);
      break;

    case RDMA_WRITE_IBUF_FLAG:
      assume(0);
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
  int release_buffer = 1;

  ibuf = (sctk_net_ibv_ibuf_t*) wc->wr_id;
  msg_header = ((sctk_net_ibv_rc_sr_msg_header_t*) ibuf->buffer);

  sctk_nodebug("New Recv cq %p, flag %d", ibuf, ibuf->flag);

  msg_type = msg_header->msg_type;

  if (lookup_mode)
  {
    sctk_nodebug("Lookup mode for dest %d and ESN %lu", dest, sctk_net_ibv_sched_get_esn(dest));
  }

  sctk_nodebug("MSG TYPE : %d", msg_type);
  switch(msg_type) {

    case RC_SR_EAGER:
      sctk_nodebug("EAGER recv");

      sctk_nodebug("READ EAGER message %lu from %d and size %lu",
          msg_header->psn, msg_header->src_process, msg_header->size);


      /* if the buffer received isn't the first, we
       * create the buffer and copy the message to it */
      if (msg_header->total_buffs != 1)
      {

        sctk_nodebug("buff %d on %d (sizepayload %lu)", msg_header->buff_nb, msg_header->total_buffs, msg_header->payload_size);

        sctk_net_ibv_frag_eager_entry_t* entry;

        if (msg_header->buff_nb == 1)
        {
          sctk_net_ibv_comp_rc_sr_frag_allocate(msg_header->src_process, msg_header->size, msg_header->psn, msg_header);

          entry = sctk_net_ibv_comp_rc_sr_copy_msg(&msg_header->payload,
              msg_header->src_process, msg_header->payload_size,
              msg_header->psn);
          assume(entry);
        } else {
          entry = sctk_net_ibv_comp_rc_sr_copy_msg(&msg_header->payload,
              msg_header->src_process, msg_header->payload_size,
              msg_header->psn);
          assume(entry);
        }

        if (msg_header->buff_nb == msg_header->total_buffs)
        {

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
                  entry, msg_header->size, 0,
                  IBV_CHAN_RC_SR_FRAG);

              /* expected message */
            } else {
              sctk_nodebug("LOOKUP EXPECTED - Found psn %d from %d",
                  msg_header->psn, msg_header->src_process);

              sctk_net_ibv_send_msg_to_mpc(
                  (sctk_thread_ptp_message_t*) entry->msg,
                  (char*) entry->msg + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
                  IBV_RC_SR_FRAG_ORIGIN, entry);
            }
          } else {
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

//                sctk_thread_yield();
              } while (ret);
            }

            sctk_net_ibv_send_msg_to_mpc(
                (sctk_thread_ptp_message_t*) entry->msg,
                (char*) entry->msg + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
                IBV_RC_SR_FRAG_ORIGIN, entry);

          }
        }
      }
      /* if this is the last buffer of a message */
      else {

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

            /* send msg to MPC */
            sctk_net_ibv_send_msg_to_mpc(
                (sctk_thread_ptp_message_t*) &(msg_header->payload),
                (char*) &(msg_header->payload) + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
                IBV_RC_SR_ORIGIN, ibuf);

            /* we don't release the buffer for now.
             * It will be released when the msg will be freed */
            release_buffer = 0;
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

            } while (ret);
          }

          sctk_nodebug("Recv EAGER message from %d (PSN %d)", msg_header->src_process, msg_header->psn);

          /* send msg to MPC */
          sctk_net_ibv_send_msg_to_mpc(
              (sctk_thread_ptp_message_t*) &(msg_header->payload),
              (char*) &(msg_header->payload) + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
              IBV_RC_SR_ORIGIN, ibuf);

          /* we don't release the buffer for now.
           * It will be released when the msg will be freed */
          release_buffer = 0;
        }

        sctk_nodebug("(EAGER) MSG PTP received %lu", msg_header->size);
      }
      break;

    case RC_SR_RDVZ_REQUEST:
      sctk_nodebug("RDVZ REQUEST recv");

      entry_rc_rdma =
        sctk_net_ibv_comp_rc_rdma_analyze_request(
            rail, rc_sr_local, rc_rdma_local,
            ibuf);

      break;

#if 0
    case RC_SR_RDVZ_ACK:
      sctk_nodebug("RDVZ ACK recv");
      sctk_net_ibv_comp_rc_rdma_send_msg(rail, rc_rdma_local,
          entry);

      restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
      break;
#endif

    case RC_SR_RDVZ_DONE:
      sctk_nodebug("RC_SR_RDVZ_DONE");

      sctk_net_ibv_com_rc_rdma_recv_done(rail,
          rc_sr_local, ibuf);
      break;


    case RC_SR_BCAST:
      sctk_nodebug("Broadcast msg received");
      sctk_net_ibv_collective_push_rc_sr(&broadcast_fifo, msg_header);
      break;

    case RC_SR_REDUCE:
      sctk_nodebug("Reduce msg received");
      sctk_net_ibv_collective_push_rc_sr(&reduce_fifo, msg_header);
      break;

    case RC_SR_BCAST_INIT_BARRIER:
      sctk_nodebug("Broadcast barrier msg received");
      sctk_net_ibv_collective_push_rc_sr(&init_barrier_fifo, msg_header);
      break;
  }

  if (release_buffer)
  {
    sctk_nodebug("Buffer %d posted", ibuf_free_srq_nb);
    sctk_net_ibv_ibuf_release(ibuf, 1);
    sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local);
  }

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
  sctk_net_ibv_ibuf_t* ibuf;

  ibuf = (sctk_net_ibv_ibuf_t*) wc.wr_id;
  sctk_nodebug("New Send cq %p, flag %d", ibuf, ibuf->flag);
}
