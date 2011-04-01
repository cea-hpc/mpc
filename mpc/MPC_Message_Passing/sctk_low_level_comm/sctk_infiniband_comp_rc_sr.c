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


/* collective */
extern struct sctk_list broadcast_fifo;
extern struct sctk_list reduce_fifo;

/* channel selection */
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/


/*-----------------------------------------------------------
 *  FRAG MSG
 *----------------------------------------------------------*/


/**
 *  Allocate memory for a new fragmented msg
 *  \param
 *  \return
 */
void sctk_net_ibv_comp_rc_sr_frag_allocate(
    sctk_net_ibv_ibuf_header_t* msg_header)
{
  sctk_net_ibv_frag_eager_entry_t* entry;
  struct sctk_list* list;
  int local_nb = -1;

  local_nb = LOOKUP_LOCAL_THREAD_ENTRY(msg_header->dest_task);
  sctk_nodebug("Allocate a new list for task %d (local:%d)", msg_header->dest_task, local_nb);

  if (!all_tasks[local_nb].frag_eager_list[msg_header->src_task])
  {
    all_tasks[local_nb].frag_eager_list[msg_header->src_task] =
      sctk_malloc(sizeof(struct sctk_list));
    assume(all_tasks[local_nb].frag_eager_list[msg_header->src_task]);
    sctk_list_new(all_tasks[local_nb].frag_eager_list[msg_header->src_task], 0, 0);
  }

  list = all_tasks[local_nb].frag_eager_list[msg_header->src_task];

  entry = sctk_malloc(sizeof(sctk_net_ibv_frag_eager_entry_t) +
      msg_header->size);
  assume(entry);

  entry->msg = (char*) entry + sizeof(sctk_net_ibv_frag_eager_entry_t);
  entry->psn          = msg_header->psn;
  entry->current_copied = 0;
  entry->src_process  = msg_header->src_process;
  entry->src_task     = msg_header->src_task;
  entry->dest_task    = msg_header->dest_task;

  sctk_list_lock(list);
  entry->list_elem = sctk_list_push(list, entry);
  sctk_list_unlock(list);

  sctk_nodebug("\t\tNew entry allocated for msg (size: %lu, PSN %lu, alloc %p src_task %d, list %p)", msg_header->size, msg_header->psn, entry, msg_header->src_task, list);
}


/**
 *  Remove a frag msg entry from the frag msg list.
 *  \param
 *  \return
 */
 void
sctk_net_ibv_comp_rc_sr_free_frag_msg(sctk_net_ibv_frag_eager_entry_t* entry)
{
  struct sctk_list* list;
  int local_nb =-1;

  local_nb = LOOKUP_LOCAL_THREAD_ENTRY(entry->dest_task);
  list = all_tasks[local_nb].frag_eager_list[entry->src_task];

  sctk_list_lock(list);
  sctk_list_remove(list, entry->list_elem);
  sctk_list_unlock(list);
  sctk_nodebug("Free msg %p (src_task %d, psn %lu)", entry, entry->src_task, entry->psn);
  sctk_free(entry);
}

 sctk_net_ibv_frag_eager_entry_t*
sctk_net_ibv_comp_rc_sr_copy_msg(void* buffer, int src_task, int dest_task, size_t size, uint32_t psn)
{
  struct sctk_list* list;
  struct sctk_list_elem *tmp = NULL;
  sctk_net_ibv_frag_eager_entry_t* entry;
  int local_nb = -1;

  local_nb = LOOKUP_LOCAL_THREAD_ENTRY(dest_task);
  list = all_tasks[local_nb].frag_eager_list[src_task];

  sctk_nodebug("Copy for msg (dest_task:%d, local_nb%d", dest_task, local_nb);

  sctk_list_lock(list);
  tmp = list->head;
  while (tmp) {

    entry = (sctk_net_ibv_frag_eager_entry_t*) tmp->elem;

    if (psn == entry->psn)
    {
      sctk_nodebug("FOUND %lu", entry->psn);
      memcpy( (char*) entry->msg + entry->current_copied, buffer, size);
      entry->current_copied+=size;

      sctk_nodebug("Frag msg copied (size: %lu, psn %lu, copied %lu task %d)", size, psn, entry->current_copied, dest_task);

      sctk_list_unlock(list);
      return entry;
    }

    tmp = tmp->p_next;
  }
  sctk_list_unlock(list);

  sctk_nodebug("Entry with PSN %lu not found !", psn);
  return NULL;
}


/**
 *  Retreive a frag msg
 *  \param
 *  \return
 */
static void
sctk_net_ibv_comp_rc_sr_frag_recv(sctk_net_ibv_ibuf_header_t* msg_header, int lookup_mode)
{
  sctk_nodebug("buff %d on %d (sizepayload %lu)", msg_header->buff_nb, msg_header->total_buffs, msg_header->payload_size);

  sctk_net_ibv_frag_eager_entry_t* entry;
  int ret;

  if (msg_header->buff_nb == 1)
  {
    sctk_net_ibv_comp_rc_sr_frag_allocate(msg_header);

    entry = sctk_net_ibv_comp_rc_sr_copy_msg(RC_SR_PAYLOAD(msg_header),
        msg_header->src_task, msg_header->dest_task,
        msg_header->payload_size, msg_header->psn);
    assume(entry);
  } else {
    entry = sctk_net_ibv_comp_rc_sr_copy_msg(RC_SR_PAYLOAD(msg_header),
        msg_header->src_task, msg_header->dest_task,
        msg_header->payload_size, msg_header->psn);
    assume(entry);
  }

  if (msg_header->buff_nb == msg_header->total_buffs)
  {

    if (lookup_mode)
    {
      ret = sctk_net_ibv_sched_sn_check_and_inc(
          msg_header->dest_task,
          msg_header->src_task,
          msg_header->psn);

      /* message not expected */
      if(ret)
      {
        sctk_ibv_profiler_inc(IBV_LOOKUP_UNEXPECTED_MSG_NB);
        sctk_nodebug("LOOKUP UNEXPECTED - Found psn %d from %d",
            msg_header->psn, msg_header->src_process);

        sctk_net_ibv_sched_pending_push(
            entry, msg_header->size, 0,
            IBV_POLL_RC_SR_FRAG_ORIGIN,
            msg_header->src_process,
            msg_header->src_task,
            msg_header->dest_task,
            msg_header->psn,
            entry->msg,
            (char*) entry->msg + sizeof(sctk_thread_ptp_message_t),
            entry);

        /* expected message */
      } else {
        sctk_ibv_profiler_inc(IBV_LOOKUP_EXPECTED_MSG_NB);
        sctk_nodebug("LOOKUP EXPECTED - Found psn %d from %d",
            msg_header->psn, msg_header->src_process);

        sctk_nodebug("READ %lu", entry->psn);

        sctk_net_ibv_send_msg_to_mpc(
            (sctk_thread_ptp_message_t*) entry->msg,
            (char*) entry->msg + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
            IBV_RC_SR_FRAG_ORIGIN, entry);

      }
    } else {
      ret = sctk_net_ibv_sched_sn_check_and_inc(
          msg_header->dest_task, msg_header->src_task, msg_header->psn);

      if (ret)
      {
        sctk_ibv_profiler_inc(IBV_UNEXPECTED_MSG_NB);
        sctk_nodebug("EXPECTED %lu but GOT %lu from process %d", sctk_net_ibv_sched_get_esn(
              msg_header->dest_task, msg_header->src_task), msg_header->psn, msg_header->src_process);

        do {
          /* enter to the lookup mode */
          sctk_net_ibv_allocator_ptp_lookup_all(
              msg_header->src_process);

          ret = sctk_net_ibv_sched_sn_check_and_inc(
              msg_header->dest_task, msg_header->src_task, msg_header->psn);

        } while (ret);
      } else {
        sctk_ibv_profiler_inc(IBV_EXPECTED_MSG_NB);
      }

      sctk_nodebug("READ %lu", entry->psn);

      sctk_net_ibv_send_msg_to_mpc(
          (sctk_thread_ptp_message_t*) entry->msg,
          (char*) entry->msg + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
          IBV_RC_SR_FRAG_ORIGIN, entry);
    }
  }
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


/**
 *  Send a collective message to a specific process.
 *  We include the communicator id of the message in
 *  the header
 *  \param
 *  \return
 */
uint32_t
sctk_net_ibv_comp_rc_sr_send_coll(
    int dest_process,
    int com_id,
    sctk_net_ibv_ibuf_t* ibuf, size_t size, size_t buffer_size,
    sctk_net_ibv_ibuf_type_t type,
    sctk_net_ibv_ibuf_ptp_type_t ptp_type,
    uint32_t* psn, const int buff_nb, const int total_buffs)
{
  uint32_t ret_psn = 0;
  sctk_net_ibv_ibuf_header_t* msg_header;

  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

  msg_header->ibuf_type = type;
  msg_header->ptp_type = ptp_type;

  msg_header->com_id = com_id;

  msg_header->src_process = sctk_process_rank;

  msg_header->size = size + RC_SR_HEADER_SIZE;
  msg_header->payload_size = buffer_size;
  msg_header->buff_nb = buff_nb;
  msg_header->total_buffs = total_buffs;

  /* We have to check if there are free slots for the
   * selected QP
   */
  sctk_net_ibv_qp_send_get_wqe(dest_process, ibuf);

  sctk_nodebug("Send frag PTP %lu to %d (task %d) with psn %lu",
      size, dest_process, dest_task, ret_psn);
  return ret_psn;
}

uint32_t
sctk_net_ibv_comp_rc_sr_send(
    int dest_process,
    int dest_task,
    sctk_net_ibv_ibuf_t* ibuf, size_t size, size_t buffer_size,
    sctk_net_ibv_ibuf_type_t type,
    sctk_net_ibv_ibuf_ptp_type_t ptp_type,
    uint32_t* psn, const int buff_nb, const int total_buffs)
{
  uint32_t ret_psn = 0;
  sctk_net_ibv_ibuf_header_t* msg_header;
  int task_id;
  int thread_id;

  /* get infos from the thread */
  sctk_get_thread_info (&task_id, &thread_id);

  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);
  msg_header->ibuf_type = type;
  msg_header->ptp_type = ptp_type;
  msg_header->src_process = sctk_process_rank;
  msg_header->src_task = task_id;
  msg_header->dest_task = dest_task;
  msg_header->size = size + RC_SR_HEADER_SIZE;

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
      *psn = sctk_net_ibv_sched_psn_inc(task_id, dest_task);
      ret_psn = *psn;
      msg_header->psn = ret_psn;
    } else {
      /* in the other case, we just copy the current
       * value of psn */
      ret_psn = *psn;
      msg_header->psn = ret_psn;
    }
  }

  /* We have to check if there are free slots for the
   * selected QP
   */
  sctk_net_ibv_qp_send_get_wqe(dest_process, ibuf);

  sctk_nodebug("Send frag PTP %lu to %d (task %d) with psn %lu",
      size, dest_process, dest_task, ret_psn);
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
  ALLOCATOR_LOCK(dest_process, IBV_CHAN_RC_SR);

  remote = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_SR);

  //FIXME Check connection
  if (!remote)
  {
    sctk_nodebug("Connexion to process %d", dest_process);
    char host[256];
    int port;

    remote = sctk_net_ibv_comp_rc_sr_allocate_init(dest_process, rc_sr_local);
    sctk_net_ibv_allocator_register(dest_process, remote, IBV_CHAN_RC_SR);
    ALLOCATOR_UNLOCK(dest_process, IBV_CHAN_RC_SR);

    sctk_net_ibv_cm_request(dest_process, remote, host, &port);
    sctk_net_ibv_cm_client(host, port, dest_process, remote);

  } else {
    ALLOCATOR_UNLOCK(dest_process, IBV_CHAN_RC_SR);

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


/**
 *  Send a collective frag msg
 *  \param
 *  \return
 */
void
sctk_net_ibv_comp_rc_sr_send_coll_frag_ptp_message(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_allocator_request_t req)
{
  sctk_net_ibv_ibuf_t* ibuf;
  void* coll_msg;
  size_t size;
  void* payload;
  size_t allowed_copy_size = 0;
  int   nb_buffers = 1;
  int   total_buffs = 0;
  int size_payload = 0;
  int remaining_size;
  int size_to_copy = 0;
  int copied_size = 0;

  coll_msg = (sctk_thread_ptp_message_t*) req.msg;
  size = req.size;
  remaining_size = req.size;

  total_buffs = ceilf( (float)
      size/ (ibv_eager_threshold - RC_SR_HEADER_SIZE));

  sctk_nodebug("Send rc_sr message. size %d, src_process %lu, nb buffs %d",  size, req.dest_process, total_buffs);

  allowed_copy_size = ibv_eager_threshold
      - RC_SR_HEADER_SIZE;

  /* while it reamins slots to copy */
  while (nb_buffers <= total_buffs)
  {
    ibuf = sctk_net_ibv_ibuf_pick(0, 1);
    payload = RC_SR_PAYLOAD(ibuf->buffer);

    sctk_nodebug("there");

    sctk_nodebug("%d - Allowed size to be copied : %d %d", nb_buffers, remaining_size, allowed_copy_size);

    /* compute the size to copy */
    if ( remaining_size > allowed_copy_size)
    {
      size_to_copy = allowed_copy_size;
    } else {
      size_to_copy = remaining_size;
    }

    sctk_nodebug("Size to copy : %lu", size_to_copy);
    memcpy(payload, (char*) coll_msg + copied_size, size_to_copy);
    copied_size += size_to_copy;
    remaining_size -= size_to_copy;

    sctk_net_ibv_ibuf_send_init(ibuf, size_payload + RC_SR_HEADER_SIZE);
    sctk_nodebug("Send message (size:%lu, remaining:%lu)", copied_size, remaining_size);

    sctk_net_ibv_comp_rc_sr_send(req.dest_process, -1, ibuf, size, size_payload, req.ibuf_type, req.ptp_type, NULL, nb_buffers, total_buffs);

    nb_buffers++;
  }

  sctk_nodebug("Eager msg sent to process %lu!", req.dest_process);
}

void
sctk_net_ibv_comp_rc_sr_send_frag_ptp_message(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_allocator_request_t req)
{
  sctk_net_ibv_ibuf_t* ibuf;
  sctk_thread_ptp_message_t* ptp_msg;
  size_t size;
  void* payload;
  uint32_t psn;
  size_t offset_buffer = 0;
  size_t offset_copied = 0;
  size_t allowed_copy_size = 0;
  int   nb_buffers = 1;
  int   total_buffs = 0;
  size_t size_payload = 0;

  ptp_msg = (sctk_thread_ptp_message_t*) req.msg;
  size = req.size;
  size += sizeof(sctk_thread_ptp_message_t);

  total_buffs = ceilf( (float)
      size/ (ibv_eager_threshold - RC_SR_HEADER_SIZE));

  sctk_nodebug("Send rc_sr message. type %d, size %d, src_process %lu, nb buffs %d", type, size, dest_process, total_buffs);


  /* while it reamins slots to copy */
  while (nb_buffers <= total_buffs)
  {
    ibuf = sctk_net_ibv_ibuf_pick(0, 1);
    payload = RC_SR_PAYLOAD(ibuf->buffer);

    /* if this is the first buffer, we copy the PTP header */
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

    sctk_net_ibv_comp_rc_sr_send(req.dest_process, ptp_msg->header.destination, ibuf, size, size_payload, RC_SR_EAGER, req.ptp_type, &psn, nb_buffers, total_buffs);

    nb_buffers++;
  }

  ptp_msg->completion_flag = 1;

  sctk_nodebug("Eager msg sent to process %lu!", dest_process);
}

void
sctk_net_ibv_comp_rc_sr_send_ptp_message (
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_allocator_request_t req) {

  sctk_net_ibv_ibuf_t* ibuf;
  sctk_thread_ptp_message_t* ptp_msg;
  void* payload;
  uint32_t psn;
  size_t size;

  ibuf = sctk_net_ibv_ibuf_pick(0, 1);
  payload = RC_SR_PAYLOAD(ibuf->buffer);
  size = req.size;

  if (sctk_process_rank == 0)
    sctk_nodebug("Send rc_sr message. type %d, size %d, src_process %lu", req.type, size, req.dest_process);

  /* if the msg to send is a PTP msg */
  if (req.ptp_type == IBV_PTP)
  {
    ptp_msg = (sctk_thread_ptp_message_t*) req.msg;
    memcpy (payload, ptp_msg, sizeof ( sctk_thread_ptp_message_t ));
    sctk_net_copy_in_buffer(ptp_msg, payload + sizeof ( sctk_thread_ptp_message_t ));
    ptp_msg->completion_flag = 1;
    size += sizeof(sctk_thread_ptp_message_t);

    sctk_net_ibv_ibuf_send_init(ibuf, size + RC_SR_HEADER_SIZE);
    sctk_net_ibv_comp_rc_sr_send(req.dest_process, req.dest_task, ibuf, size, size, RC_SR_EAGER, req.ptp_type, &psn, 1, 1);

    sctk_nodebug("Send PTP %lu to %d with psn %lu", size, req.dest_process, psn);
  }
  /* if this is a COLLECTIVE msg */
  else if ( (req.ptp_type == IBV_BCAST) ||
      (req.ptp_type == IBV_REDUCE) )
  {
    memcpy (payload, req.msg, size);
    sctk_net_ibv_ibuf_send_init(ibuf, size + RC_SR_HEADER_SIZE);
    sctk_net_ibv_comp_rc_sr_send(req.dest_process, req.dest_task, ibuf, size, size, RC_SR_EAGER, req.ptp_type, NULL, 1, 1);

  } else if ((req.ptp_type == IBV_BCAST_INIT_BARRIER)) {
    memcpy (payload, req.msg, size);
    sctk_net_ibv_ibuf_send_init(ibuf, size + RC_SR_HEADER_SIZE);
    sctk_net_ibv_comp_rc_sr_send(req.dest_process, req.dest_task, ibuf, size, size, RC_SR_EAGER, req.ptp_type, NULL, 1, 1);
  } else  {
    sctk_error("Type of message not known");
    assume(0);
  }

  sctk_nodebug("Eager msg sent to process %lu!", req.dest_process);
}

  static int
sctk_net_ibv_comp_rc_sr_ptp_recv(sctk_net_ibv_ibuf_t* ibuf ,sctk_net_ibv_ibuf_header_t* msg_header, int lookup_mode, int *release_buffer)
{
  int ret = -1;
  /*
   * If this is a fragmented msg
   */
  if (msg_header->total_buffs != 1)
  {
    sctk_net_ibv_comp_rc_sr_frag_recv(msg_header,lookup_mode);
  }
  /*
   * If this is a normal eager msg
   */
  else {

    if (lookup_mode)
    {
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


      /* normal mode */
    } else {
      sctk_nodebug("NORMAL mode");
      ret = sctk_net_ibv_sched_sn_check_and_inc(
          msg_header->dest_task, msg_header->src_task, msg_header->psn);

      if (ret)
      {
        sctk_ibv_profiler_inc(IBV_UNEXPECTED_MSG_NB);
        sctk_nodebug("EXPECTED %lu but GOT %lu from process %d", sctk_net_ibv_sched_get_esn(
              msg_header->dest_task, msg_header->src_task), msg_header->psn, msg_header->src_process);

        do {
          /* enter to the lookup mode */
          sctk_net_ibv_allocator_ptp_lookup_all(
              msg_header->src_process);

          ret = sctk_net_ibv_sched_sn_check_and_inc(
              msg_header->dest_task, msg_header->src_task,
              msg_header->psn);

        } while (ret);
      } else {
        sctk_ibv_profiler_inc(IBV_EXPECTED_MSG_NB);
      }

      sctk_nodebug("Recv EAGER message from %d (PSN %d)", msg_header->src_process, msg_header->psn);

      /* send msg to MPC */
      sctk_net_ibv_send_msg_to_mpc(
          (sctk_thread_ptp_message_t*) RC_SR_PAYLOAD(msg_header),
          (char*) RC_SR_PAYLOAD(msg_header) + sizeof(sctk_thread_ptp_message_t), msg_header->src_process,
          IBV_RC_SR_ORIGIN, ibuf);

      /* we don't release the buffer for now.
       * It will be released when the msg will be freed */
      *release_buffer = 0;
    }

    sctk_nodebug("(EAGER) MSG PTP received %lu", msg_header->size);
  }
  return ret;
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
      /* there */
      sctk_net_ibv_comp_rc_rdma_send_finish(
          rail, rc_sr_local, rc_rdma_local,
          ibuf);

      sctk_net_ibv_com_rc_rdma_read_finish(
          ibuf, rc_sr_local, lookup_mode);

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
  sctk_net_ibv_ibuf_header_t* msg_header;
  int release_buffer = 1;

  ibuf = (sctk_net_ibv_ibuf_t*) wc->wr_id;
  msg_header = ((sctk_net_ibv_ibuf_header_t*) ibuf->buffer);

  sctk_nodebug("Message received from proc %d (task %d)", msg_header->src_process, msg_header->src_task);

  if (lookup_mode)
  {
    sctk_nodebug("Lookup mode for dest %d and ESN %lu", dest, sctk_net_ibv_sched_get_esn(dest));
  }

  /* Switch on the type of the eager msg: eager, rdma request */
  switch(msg_header->ibuf_type) {

    case RC_SR_EAGER:
      sctk_nodebug("READ EAGER message %lu from %d (task %d) and size %lu",
          msg_header->psn, msg_header->src_process, msg_header->src_task,
          msg_header->size);

      switch (msg_header->ptp_type)
      {
        case IBV_PTP:
          sctk_net_ibv_comp_rc_sr_ptp_recv(ibuf, msg_header, lookup_mode, &release_buffer);
          break;

        case IBV_BCAST:
          sctk_nodebug("POLL Broadcast msg received");
          sctk_net_ibv_collective_push_rc_sr(&broadcast_fifo, ibuf);
          release_buffer = 0;
          break;

        case IBV_REDUCE:
          sctk_nodebug("POLL Reduce msg received");
          sctk_net_ibv_collective_push_rc_sr(&reduce_fifo, ibuf);
          release_buffer = 0;
          break;

        case IBV_BCAST_INIT_BARRIER:
          sctk_nodebug("Broadcast barrier msg received");
          sctk_net_ibv_collective_push_rc_sr(&init_barrier_fifo, ibuf);
          release_buffer = 0;
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

    default:
      assume(0);
      break;
  }

  if (release_buffer)
  {
    sctk_nodebug("Buffer %d posted", ibuf_free_srq_nb);
    sctk_net_ibv_ibuf_release(ibuf, 1);
    sctk_net_ibv_ibuf_srq_check_and_post(rc_sr_local, ibv_srq_credit_limit);
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
