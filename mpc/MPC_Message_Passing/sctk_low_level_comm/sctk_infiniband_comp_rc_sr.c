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
#include "sctk_alloc.h"
#include "sctk.h"
#include "sctk_net_tools.h"

static uint32_t wr_current = 0;

/* collective */
extern struct sctk_list broadcast_fifo;
extern struct sctk_list reduce_fifo;

static restore_buffers(
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_sr_buff_t* buff,
    int id)
{
  sctk_net_ibv_comp_rc_sr_post_recv(local, buff, id);
  sctk_net_ibv_comp_rc_sr_free_header(buff,
      &buff->headers[id]);
}


/*-----------------------------------------------------------
 *  NEW / FREE
 *----------------------------------------------------------*/

  sctk_net_ibv_rc_sr_buff_t*
sctk_net_ibv_comp_rc_sr_new( int slot_nb, int slot_size)
{
  sctk_net_ibv_rc_sr_buff_t* buff = NULL;

  buff = sctk_malloc(sizeof(sctk_net_ibv_rc_sr_buff_t));
  assume(buff);
  buff->headers = sctk_malloc( slot_nb * sizeof(sctk_net_ibv_rc_sr_entry_t));
  assume(buff->headers);

  sctk_nodebug("Size : %lu",  slot_size * sizeof(sctk_net_ibv_rc_sr_entry_t));
  buff->slot_nb = slot_nb;
  buff->current_nb = 0;
  buff->slot_size = slot_size;
  sctk_thread_mutex_init(&buff->lock, NULL);

  /* set wr begin and wr end*/
  buff->wr_begin  = wr_current;
  wr_current      += slot_nb;
  buff->wr_end    = wr_current;

  //sctk_nodebug("Wr begin %d Wr end %d", buff->wr_begin, buff->wr_end);

  return buff;
}


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

  void
sctk_net_ibv_comp_rc_sr_free_header(sctk_net_ibv_rc_sr_buff_t* buff, sctk_net_ibv_rc_sr_entry_t* entry)
{
  sctk_thread_mutex_lock(&buff->lock);
  entry->used = 0;
  buff->current_nb--;
  sctk_thread_mutex_unlock(&buff->lock);
}


  sctk_net_ibv_rc_sr_entry_t*
sctk_net_ibv_comp_rc_sr_pick_header(sctk_net_ibv_rc_sr_buff_t* buff)
{
  int i;

  while (1)
  {
    sctk_thread_mutex_lock(&buff->lock);
    for (i = 0; i < buff->slot_nb; ++i)
    {
      if ( buff->headers[i].used == 0 )
      {
        sctk_nodebug("RC_SR slot %d found !", i);
        buff->headers[i].used = 1;
        buff->current_nb++;
        sctk_thread_mutex_unlock(&buff->lock);
        return &(buff->headers[i]);
      }
    }
    sctk_nodebug("No header found");

    sctk_thread_mutex_unlock(&buff->lock);
    sctk_thread_yield();
  }
  assume(0);
}

void
sctk_net_ibv_comp_rc_sr_send(
    sctk_net_ibv_qp_remote_t* remote,
    sctk_net_ibv_rc_sr_entry_t* entry,
    size_t size, sctk_net_ibv_rc_sr_msg_type_t type, uint32_t* psn)
{
  int rc;
  struct  ibv_send_wr* bad_wr = NULL;

  entry->list.length          = MSG_HEADER_SIZE + size;
  entry->msg_header->msg_type = type;
  entry->msg_header->size     = MSG_HEADER_SIZE + size;

  /* If the psn variable is != NULL, the PSN has to be computed before
   * sending the message */
  if (psn != NULL)
  {
    sctk_net_ibv_sched_lock();
    *psn = sctk_net_ibv_sched_psn_inc(remote->rank);
    rc = ibv_post_send(remote->qp , &(entry->wr.send_wr), &bad_wr);
    assume (rc == 0);
    sctk_net_ibv_sched_unlock();
    sctk_nodebug("Send EAGER message to %d and size %lu (PSN:%d)",
        remote->rank, entry->msg_header->size, *psn);

  } else {
    sctk_nodebug("QP %p dest %d", remote->qp, remote->rank);
    rc = ibv_post_send(remote->qp , &(entry->wr.send_wr), &bad_wr);
    assume (rc == 0);
  }
}

int
sctk_net_ibv_comp_rc_sr_post_recv(
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_sr_buff_t* buff, int i)
{
  struct ibv_recv_wr *bad_wr;
  sctk_net_ibv_rc_sr_entry_t* entry = NULL;
  int rc;

  entry = &(buff->headers[i]);
  assume(entry);
  sctk_nodebug("Header : %p %d", entry, i);

  sctk_nodebug("OK rank %d/%d %p", sctk_process_rank, sctk_process_number, &local->srq->context->ops);

  sctk_nodebug("SRQ:%p | RECV_WR:%p, BAD_WR:%p, HEADER:%p",local->srq, &(entry->wr.recv_wr), &bad_wr, entry->msg_header);

  rc = ibv_post_srq_recv(local->srq, &(entry->wr.recv_wr), &bad_wr);
  assume(rc == 0);

  return 0;
}

void
sctk_net_ibv_comp_rc_sr_init(
    sctk_net_ibv_qp_rail_t  *rail,
    sctk_net_ibv_qp_local_t* local,
    sctk_net_ibv_rc_sr_buff_t* buff, sctk_net_ibv_rc_sr_wr_type_t type)
{
  int i;
  sctk_net_ibv_rc_sr_entry_t* entry = NULL;
  unsigned long page_size;
  sctk_net_ibv_mmu_entry_t *mmu_entry = NULL;
  size_t size;

  page_size = sctk_net_ibv_mmu_get_pagesize();

  for (i = 0; i < buff->slot_nb; ++i) {

    entry = &(buff->headers[i]);

    size = MSG_HEADER_SIZE + buff->slot_size;
    sctk_nodebug("SIZE : %lu", size);

    posix_memalign((void*) &(entry->msg_header), page_size, size );
    sctk_nodebug("PTR 2: %p", entry->msg_header);

    mmu_entry = sctk_net_ibv_mmu_register (
        rail->mmu, local,
        entry->msg_header, size);

    entry->mmu_entry = mmu_entry;
    entry->id = i + buff->wr_begin;
    entry->used = 0;

    entry->list.addr                =  (uintptr_t) entry->msg_header;
    entry->list.length              =  size;
    entry->list.lkey                =  mmu_entry->mr->lkey;
    entry->msg_header->src_process  =  sctk_process_rank;

    if (type == RC_SR_WR_SEND)
    {
      sctk_nodebug("Send");
      /* user assigned work request ID */
      entry->wr.send_wr.wr_id = entry->id;
      /* scatter/gather array for this WR */
      entry->wr.send_wr.sg_list = &(entry->list);
      /* number of entries in sq_list */
      entry->wr.send_wr.num_sge = 1;
      entry->wr.send_wr.opcode = IBV_WR_SEND;
      /* pointer to next WR, NULL if last one */
      entry->wr.send_wr.send_flags = IBV_SEND_SIGNALED;
      entry->wr.send_wr.next = NULL;
    } else if (type == RC_SR_WR_RECV) {
      sctk_nodebug("recv");
      /* user assigned work request ID */
      entry->wr.recv_wr.wr_id = entry->id;
      /* scatter/gather array for this WR */
      entry->wr.recv_wr.sg_list = &(entry->list);
      /* number of entries in sq_list */
      entry->wr.recv_wr.num_sge = 1;
    }
  }
}

  sctk_net_ibv_rc_sr_entry_t*
sctk_net_ibv_comp_rc_sr_match_entry(sctk_net_ibv_rc_sr_buff_t* buff, void* ptr)
{
  int i;

  for (i = 0; i < buff->slot_nb; ++i)
  {
    if ( &(buff->headers[i].msg_header->payload) == ptr )
    {
      sctk_nodebug("RC_SR slot %d match ptr %p !", i, ptr);
      return &(buff->headers[i]);
    }
  }
  return NULL;
}

  void
sctk_net_ibv_comp_rc_sr_free(sctk_net_ibv_rc_sr_buff_t* entry)
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
    remote = sctk_net_ibv_comp_rc_sr_allocate_init(dest_process, rc_sr_local);
    sctk_net_ibv_allocator_register(dest_process, remote, IBV_CHAN_RC_SR);
  } else {
    sctk_nodebug("Remote known");
  }
  assume(remote);

  if (remote->is_connected == 0)
  {
    char host[256];
    int port;

    sctk_net_ibv_allocator_unlock(dest_process, IBV_CHAN_RC_SR);
    sctk_net_ibv_cm_request(dest_process, remote, host, &port);
    sctk_net_ibv_cm_client(host, port, dest_process, remote);
  } else  {
    sctk_net_ibv_allocator_unlock(dest_process, IBV_CHAN_RC_SR);
  }

  return remote;
}


void
sctk_net_ibv_comp_rc_sr_send_ptp_message (
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_rc_sr_buff_t* buff,
    void* msg, int dest_process, size_t size, sctk_net_ibv_rc_sr_msg_type_t type) {


  sctk_net_ibv_rc_sr_entry_t* entry;
  sctk_net_ibv_qp_remote_t* remote;
  sctk_thread_ptp_message_t* ptp_msg;
  void* payload;

  entry = sctk_net_ibv_comp_rc_sr_pick_header(buff);
  payload = &(entry->msg_header->payload);
  /* check if the TCP connection is active. If not, connect peers */
  remote = sctk_net_ibv_comp_rc_sr_check_and_connect(dest_process);

  /* if the msg to send is a PTP msg */
  if (type == RC_SR_EAGER)
  {
    ptp_msg = (sctk_thread_ptp_message_t*) msg;
    memcpy (payload, ptp_msg, sizeof ( sctk_thread_ptp_message_t ));
    sctk_net_copy_in_buffer(ptp_msg, payload + sizeof ( sctk_thread_ptp_message_t ));
    ptp_msg->completion_flag = 1;
    size = size + sizeof(sctk_thread_ptp_message_t);

    /* lock to be sure that the message that we send has the good
     * PSN */
    sctk_net_ibv_comp_rc_sr_send(remote, entry, size, type, &entry->msg_header->psn);
    sctk_nodebug("Send PTP %lu to %d with psn %lu", size, dest_process, psn);
  }
  /* if this is a COLLECTIVE msg */
  else if ( (type == RC_SR_BCAST) || (type == RC_SR_REDUCE))
  {
    memcpy (payload, msg, size);
    sctk_net_ibv_comp_rc_sr_send(remote, entry, size, type, NULL);
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

  ptr = sctk_malloc(size);
  memcpy(ptr, msg, size);

  return ptr;
}

/*-----------------------------------------------------------
 *  POLLING FUNCTIONS
 *----------------------------------------------------------*/
void
sctk_net_ibv_rc_sr_poll_send(
    struct ibv_wc* wc,
    sctk_net_ibv_rc_sr_buff_t* ptp_buff,
    sctk_net_ibv_rc_sr_buff_t* coll_buff)
{
  sctk_nodebug("New Send cq %d", wc->wr_id);

  /* if PTP msg */
  if ( (wc->wr_id >= ptp_buff->wr_begin)
      && (wc->wr_id < ptp_buff->wr_end) )
  {
    sctk_nodebug("ptp msg");
    sctk_net_ibv_comp_rc_sr_free_header(ptp_buff, &ptp_buff->headers[wc->wr_id - ptp_buff->wr_begin]);
    /* if COLLECTIVE msg */
  } else if ( (wc->wr_id >= coll_buff->wr_begin)
      && (wc->wr_id < coll_buff->wr_end) )
  {
    sctk_nodebug("coll msg");
    sctk_net_ibv_comp_rc_sr_free_header(coll_buff, &coll_buff->headers[wc->wr_id - coll_buff->wr_begin]);
  } else {
    assume(0);
  }
  sctk_nodebug("Send cq done");
}

int
sctk_net_ibv_rc_sr_poll_recv(
    struct ibv_wc* wc,
    sctk_net_ibv_qp_rail_t      *rail,
    sctk_net_ibv_qp_local_t     *rc_sr_local,
    sctk_net_ibv_rc_sr_buff_t   *rc_sr_recv_buff,
    sctk_net_ibv_qp_local_t     *rc_rdma_local,
    int lookup_mode,
    int dest)
{
  sctk_net_ibv_rc_sr_entry_t* entry;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  int wc_id;
  sctk_net_ibv_rc_sr_msg_type_t msg_type;
  sctk_net_ibv_rc_rdma_entry_recv_t* entry_recv;
  int* limit;
  int ret;
  uint32_t psn_got;
  void* msg;

  wc_id = wc->wr_id;
  entry = &(rc_sr_recv_buff->headers[wc_id]);
  msg_type = entry->msg_header->msg_type;

  if (lookup_mode)
  {
    sctk_nodebug("Lookup mode for dest %d and ESN %lu", dest, sctk_net_ibv_sched_get_esn(dest));
  }

  switch(msg_type) {

    case RC_SR_EAGER:
      sctk_nodebug("EAGER recv");

      sctk_nodebug("READ EAGER message %lu from %d and size %lu",
          entry->msg_header->psn, entry->msg_header->src_process, entry->msg_header->size);

      if (lookup_mode)
      {
        ret = sctk_net_ibv_sched_sn_check_and_inc(
            entry->msg_header->src_process,
            entry->msg_header->psn);

        /* message not expected */
        if(ret)
        {
          sctk_nodebug("LOOKUP UNEXPECTED - Found psn %d from %d",
              entry->msg_header->psn, entry->msg_header->src_process);

          sctk_net_ibv_allocator_pending_push(
              entry->msg_header, entry->msg_header->size, 1,
              IBV_CHAN_RC_SR);

          /* expected message */
        } else {
          sctk_nodebug("LOOKUP EXPECTED - Found psn %d from %d",
              entry->msg_header->psn, entry->msg_header->src_process);

          /* copy the message to buffer */
          msg = sctk_net_ibv_comp_rc_sr_prepare_recv(
              &(entry->msg_header->payload),
              entry->msg_header->size);

          /* send msg to MPC */
          sctk_net_ibv_send_msg_to_mpc(
              (sctk_thread_ptp_message_t*) msg,
              (char*) msg + sizeof(sctk_thread_ptp_message_t), entry->msg_header->src_process,
              IBV_RC_SR_ORIGIN, NULL);
        }
        /* reset buffers */
        restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);

        /* normal mode */
      } else {
        ret = sctk_net_ibv_sched_sn_check_and_inc(entry->msg_header->src_process, entry->msg_header->psn);

        if (ret)
        {
          sctk_nodebug("EXPECTED %lu but GOT %lu from process %d", sctk_net_ibv_sched_get_esn(entry->msg_header->src_process), entry->msg_header->psn, entry->msg_header->src_process);
          do {
            /* enter to the lookup mode */
            sctk_net_ibv_allocator_ptp_lookup_all(
                entry->msg_header->src_process);

            ret = sctk_net_ibv_sched_sn_check_and_inc(
                entry->msg_header->src_process, entry->msg_header->psn);

            sctk_thread_yield();
          } while (ret);
        }

        sctk_nodebug("Recv EAGER message from %d (PSN %d)", entry->msg_header->src_process, entry->msg_header->psn);

        /* copy the message to buffer */
        msg = sctk_net_ibv_comp_rc_sr_prepare_recv(
            &(entry->msg_header->payload),
            entry->msg_header->size);

        /* send msg to MPC */
        sctk_net_ibv_send_msg_to_mpc(
            (sctk_thread_ptp_message_t*) msg,
            (char*) msg + sizeof(sctk_thread_ptp_message_t), entry->msg_header->src_process,
            IBV_RC_SR_ORIGIN, NULL);

        /* reset buffers */
        restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
      }
      sctk_nodebug("(EAGER) MSG PTP received %lu", entry->msg_header->size);
      break;

    case RC_SR_RDVZ_REQUEST:
      sctk_nodebug("RDVZ REQUEST recv");

      entry_rc_rdma =
        sctk_net_ibv_comp_rc_rdma_add_request(
            rail, rc_sr_local, rc_rdma_local,
            entry);
      assume(entry_rc_rdma);

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
//      sctk_nodebug("RDVZ DONE recv");
//      entry_recv = sctk_net_ibv_comp_rc_rdma_match_read_msg(
//          entry);
//      assume(entry_recv);
//
//      sctk_nodebug("RDVZ DONE RECV for process %lu with psn %lu", entry_recv->src_process, entry_recv->psn);
//
//      if (lookup_mode) {
//
//        ret = sctk_net_ibv_sched_sn_check_and_inc(
//            entry_recv->src_process,
//            entry_recv->psn);
//
//        if (ret)
//        {
//          sctk_nodebug("Push RDVZ msg");
//          sctk_net_ibv_allocator_pending_push(entry_recv,
//              sizeof(sctk_net_ibv_rc_rdma_entry_recv_t), 0,
//              IBV_CHAN_RC_RDMA);
//          //TODO surround by lock
//          entry_recv->sent_to_mpc = 1;
//
//          restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
//        } else {
//          sctk_nodebug("Read the msg");
//          sctk_net_ibv_comp_rc_rdma_read_msg(entry_recv);
//
//          sctk_nodebug("Recv RDVZ message from %d (PSN:%lu)", entry_recv->src_process, entry_recv->psn);
//
//          restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
//
//          sctk_nodebug("PSN %d FOUND for process %d", entry_recv->psn, entry_recv->src_process);
//        }
//
//      } else {
//        ret = sctk_net_ibv_sched_sn_check_and_inc(
//            entry_recv->src_process,
//            entry_recv->psn);
//
//        sctk_nodebug("RET : %d", ret);
//
//        if(ret)
//        {
//          do {
//            sctk_net_ibv_allocator_ptp_lookup_all(
//                entry_recv->src_process);
//
//            ret = sctk_net_ibv_sched_sn_check_and_inc(
//                entry_recv->src_process, entry_recv->psn);
//
//            sctk_thread_yield();
//
//          } while (ret);
//        }
//
//        sctk_net_ibv_comp_rc_rdma_read_msg(entry_recv);
//
//        sctk_nodebug("Recv RDVZ message from %d (PSN:%lu)", entry_recv->src_process, entry_recv->psn);
//
//        restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
//      }
//
//      /* process the next RDMA request */
//      entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
//        sctk_net_ibv_allocator_get(entry_recv->src_process, IBV_CHAN_RC_RDMA);
//      sctk_net_ibv_allocator_rc_rdma_process_next_request(entry_rc_rdma);
//      break;
//
    case RC_SR_BCAST:
      sctk_nodebug("Broadcast msg received from %d", entry_recv->src_process);
      sctk_net_ibv_collective_push(&broadcast_fifo, entry->msg_header);
      restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
      break;

    case RC_SR_REDUCE:
      sctk_nodebug("Reduce msg received from %d", entry_recv->src_process);
      sctk_net_ibv_collective_push(&reduce_fifo, entry->msg_header);
      restore_buffers(rc_sr_local, rc_sr_recv_buff, wc_id);
      break;
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
