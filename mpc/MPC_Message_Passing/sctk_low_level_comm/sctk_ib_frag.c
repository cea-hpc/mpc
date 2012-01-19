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
#include "sctk_ib_scheduling.h"
#include "sctk_ib_allocator.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_list.h"
#include "sctk_ib_coll.h"
#include "sctk_ib_lib.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_ibufs.h"
#include "sctk_ib_rpc.h"
#include "sctk_alloc.h"
#include "sctk.h"
#include "sctk_net_tools.h"
#include "sctk_ib_profiler.h"
#include "sctk_ib_frag.h"

extern sctk_net_ibv_allocator_task_t all_tasks[MAX_NB_TASKS_PER_PROCESS];
extern sctk_spinlock_t all_tasks_lock;

/*-----------------------------------------------------------
 *  PTP
 *----------------------------------------------------------*/

/**
 *  Send a PTP fragmented message (into several eager buffers)
 *  \param
 *  \return
 */
void
sctk_net_ibv_comp_rc_sr_send_frag_ptp_message(
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_net_ibv_allocator_request_t req)
{
  sctk_net_ibv_ibuf_t* ibuf;
  sctk_thread_ptp_message_t* ptp_msg;
  void* payload;
  size_t offset_buffer = 0;
  size_t offset_copied = 0;
  size_t total_copied = 0;
  size_t allowed_copy_size = 0;
  int   nb_buffers = 1;
  int   total_buffs = 0;
  size_t size_payload = 0;

  ptp_msg = (sctk_thread_ptp_message_t*) req.msg;

  /* compute the number of slots needed */
  total_buffs = ceilf( (float)
      req.size / (ibv_eager_limit - RC_SR_HEADER_SIZE));

  sctk_nodebug("Send rc_sr message. type %d, size %d, src_process %lu, nb buffs %d", type, req.size, dest_process, total_buffs);

  /* while it reamins slots to copy */
  for (nb_buffers = 1; nb_buffers <= total_buffs; ++nb_buffers)
  {
    ibuf = sctk_net_ibv_ibuf_pick(0, 1);
    payload = (void*) RC_SR_PAYLOAD(ibuf->buffer);

    /* if this is the first buffer, we copy the PTP header */
    if (nb_buffers == 1)
    {
      sctk_nodebug("Copy header");
      memcpy (payload, ptp_msg, sizeof ( sctk_thread_ptp_message_t ));
      offset_buffer = sizeof ( sctk_thread_ptp_message_t);
      total_copied = 0;
    } else {
      offset_buffer = 0;
      total_copied = offset_copied + sizeof ( sctk_thread_ptp_message_t);
    }

    /* compute the allowed size for the current buffer */
    allowed_copy_size = ibv_eager_limit
      - RC_SR_HEADER_SIZE
      - offset_buffer;

    sctk_nodebug("%d - Allowed size to be copied : %lu", nb_buffers, allowed_copy_size);

    size_payload = offset_copied;

    /* copy the message to the ib buffer */
    offset_copied = sctk_net_copy_frag_msg(ptp_msg, payload + offset_buffer, offset_copied, allowed_copy_size);

    size_payload = (offset_copied - size_payload) + offset_buffer;

    /* fill the buffer and request */
    sctk_net_ibv_ibuf_send_init(ibuf, size_payload + RC_SR_HEADER_SIZE);
    /* total number of buffers used for the message */
    req.total_buffs   = total_buffs;
    req.payload_size  = size_payload;
    /* total size already copied (header + payload) */
    req.total_copied  = total_copied;
    sctk_net_ibv_comp_rc_sr_send(ibuf, req, RC_SR_EAGER);
    sctk_nodebug("Total copied: %lu, payload: %lu", total_copied, size_payload);
  }

  /* we set the message as completed */
  ptp_msg->completion_flag = 1;
  sctk_nodebug("Eager msg sent to process %lu!", dest_process);
}

/**
 *  Allocate memory for a new fragmented msg. This entry is read when a new
 *  fragment of the msg is received.
 *  \param
 *  \return
 */
sctk_net_ibv_msg_entry_t* sctk_net_ibv_frag_allocate(
    sctk_net_ibv_ibuf_header_t* msg_header)
{
  sctk_net_ibv_msg_entry_t* entry;

  entry = sctk_malloc(sizeof(sctk_net_ibv_msg_entry_t) +
      msg_header->size);
  assume(entry);
  sctk_nodebug("allocate for size %lu", msg_header->size);
  sctk_ibv_profiler_inc(IBV_MEM_TRACE);

  entry->payload = (char*) entry + sizeof(sctk_net_ibv_msg_entry_t);
  entry->psn          = msg_header->psn;
  entry->src_process  = msg_header->src_process;
  entry->src_task     = msg_header->src_task;
  entry->dest_task    = msg_header->dest_task;
  entry->total_buffs  = msg_header->total_buffs;
  entry->size         = msg_header->size;
  entry->type         = RC_SR_FRAG_EAGER;
  entry->buff_nb      = 0;
  entry->ready        = 0;

  sctk_nodebug("New entry allocated for msg (size: %lu, PSN %lu, alloc %p,"
    "src_task %d, dest_task %d)", msg_header->size, msg_header->psn, entry,
      msg_header->src_task, msg_header->dest_task);

  return entry;
}


/**
 *  Remove a frag msg entry from the frag msg list.
 *  \param
 *  \return
 */
 void
sctk_net_ibv_frag_free_msg(sctk_net_ibv_msg_entry_t* entry)
{
/* The entry is now removed when the msg is sent to MPC */
  sctk_free(entry);
  sctk_ibv_profiler_dec(IBV_MEM_TRACE);
}



/**
 *  Print a frag eager list. Usefull for debug.
 *  When the expected msg is not found in the list, we read
 *  the content of the list
 *  \param
 *  \return
 */
void sctk_net_ibv_frag_eager_print_list(struct sctk_list_header *list)
{
  struct sctk_list_header *tmp = NULL;
  sctk_net_ibv_msg_entry_t* entry;

  sctk_ib_list_foreach(tmp, list)
  {
    entry = sctk_ib_list_get_entry(tmp, sctk_net_ibv_msg_entry_t, list_header);

    sctk_warning("PSN FOUND %lu (total buffs:%d, buff_nb:%d, src:%d, dest:%d) next:%p", entry->psn,
        entry->total_buffs, entry->buff_nb, entry->src_task, entry->dest_task, tmp->p_next);
  }
}

/**
 *  Search to the framgmented msg list for a msg with a specific PSN
 *  \param
 *  \return
 */
  sctk_net_ibv_msg_entry_t*
sctk_net_ibv_frag_search_msg(sctk_net_ibv_ibuf_header_t* msg_header, struct sctk_list_header *list)
{
  struct sctk_list_header *tmp = NULL;
  sctk_net_ibv_msg_entry_t* entry = NULL;

  sctk_ib_list_foreach(tmp, list)
  {
    entry = sctk_ib_list_get_entry(tmp, sctk_net_ibv_msg_entry_t, list_header);
    sctk_nodebug("PSN FOUND %lu", entry->psn);
    if ( (msg_header->psn == entry->psn) &&
        (msg_header->src_process == entry->src_process))
    {
      return entry;
    }
  }
  sctk_nodebug("Entry with PSN %lu not found for task !", msg_header->psn);
  return NULL;
}


/**
 *  Search to the framgmented msg list for a msg with a specific PSN
 *  \param
 *  \return
 */
  int
sctk_net_ibv_frag_copy_msg(sctk_net_ibv_ibuf_header_t* msg_header,
    struct sctk_list_header* list, sctk_net_ibv_msg_entry_t* entry)
{
  int ret;

  memcpy( (char*) (entry->payload + msg_header->total_copied),
      RC_SR_PAYLOAD(msg_header), msg_header->payload_size);

  sctk_ib_list_lock(list);
  entry->buff_nb++;
  ret =  entry->buff_nb;
  sctk_ib_list_unlock(list);

  assume (ret <= msg_header->total_buffs);

  sctk_nodebug("Frag msg copied (size: %lu, psn %lu, task %d)",
      msg_header->payload_size, msg_header->psn, msg_header->dest_task);

  return ret;
}


/**
 *  Retreive a frag msg
 *  \param
 *  \return
 */
static sctk_spinlock_t frag_lock = SCTK_SPINLOCK_INITIALIZER;

  void
sctk_net_ibv_comp_rc_sr_frag_recv(sctk_net_ibv_ibuf_header_t* msg_header)
{
  sctk_net_ibv_msg_entry_t* entry;
  int ret;
  int local_nb = -1;
  struct sctk_list_header* list;

  sctk_nodebug("buff %d on %d (sizepayload %lu, psn %lu, src_task %lu, dest_task %lu, total_copied:%lu)", msg_header->buff_nb, msg_header->total_buffs, msg_header->payload_size, msg_header->psn, msg_header->src_task, msg_header->dest_task, msg_header->total_copied);

  local_nb = LOOKUP_LOCAL_THREAD_ENTRY(msg_header->dest_task);

  list =  all_tasks[local_nb].frag_eager_list[msg_header->src_task];
  if (!list)
  {
    sctk_spinlock_lock(&all_tasks_lock);
    list =  all_tasks[local_nb].frag_eager_list[msg_header->src_task];

    if (!list)
    {
      list =  sctk_malloc(sizeof(struct sctk_list_header));
      memset(list, 0, sizeof(struct sctk_list_header));
      assume(list);

      all_tasks[local_nb].frag_eager_list[msg_header->src_task] = list;
      SCTK_LIST_HEAD_INIT(list);
    }
    sctk_spinlock_unlock(&all_tasks_lock);
  }

  sctk_nodebug("all_tasks:%p, List:%p, local_nb:%d, src_task:%d", all_tasks, list, local_nb, msg_header->src_task);

  /*
   * We are looking into the list where the entry is located.
   * If the entry isn't found, we create a new list element.
   */
  sctk_ib_list_lock(list);
  entry = sctk_net_ibv_frag_search_msg(msg_header, list);
  if (!entry) {
    entry = sctk_net_ibv_frag_allocate(msg_header);
    sctk_ib_list_push_tail(list, &entry->list_header);
  }
  sctk_ib_list_unlock(list);
  assume(entry);

  /*
   * We copy the payload of the msg
   */
  ret = sctk_net_ibv_frag_copy_msg(msg_header, list, entry);

  /* all parts of the message have been received -> we
   * use it */
  if (ret == msg_header->total_buffs)
  {
    /* first we remove the entry from the list */
    sctk_ib_list_lock(list);
    sctk_ib_list_remove(&entry->list_header);
    sctk_ib_list_unlock(list);

    ret = sctk_net_ibv_sched_sn_check_and_inc(
        entry->dest_task,
        entry->src_task,
        entry->psn);

    /* message not expected */
    if(ret)
    {
      sctk_ibv_profiler_inc(IBV_LOOKUP_UNEXPECTED_MSG_NB);
      sctk_nodebug("LOOKUP UNEXPECTED - Found psn %d from %d",
          entry->psn, entry->src_process);

      sctk_net_ibv_sched_pending_push(
          entry, entry->size, 0,
          IBV_POLL_RC_SR_FRAG_ORIGIN,
          entry->src_process,
          entry->src_task,
          entry->dest_task,
          entry->psn,
          entry->payload,
          (char*) entry->payload + sizeof(sctk_thread_ptp_message_t),
          entry);

      /* expected message */
    } else {
      sctk_ibv_profiler_inc(IBV_LOOKUP_EXPECTED_MSG_NB);
      sctk_nodebug("LOOKUP EXPECTED - Found psn %d from %d",
          entry->psn, entry->src_process);

      sctk_net_ibv_send_msg_to_mpc(
          (sctk_thread_ptp_message_t*) entry->payload,
          (char*) entry->payload + sizeof(sctk_thread_ptp_message_t),
          entry->src_process,
          IBV_RC_SR_FRAG_ORIGIN, entry);
    }
    sctk_nodebug("READ %lu for task %d", entry->psn, entry->dest_task);
  }
}

/*-----------------------------------------------------------
 *  COLLECTIVE
 *----------------------------------------------------------*/
/**
 *  Send a collective fragmented msg. The msg is divided into several
 *  eager buffers and bufers are send 1 by 1.
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
  size_t remaining_size;
  int size_to_copy = 0;
  int copied_size = 0;

  coll_msg = (sctk_thread_ptp_message_t*) req.msg;
  size = req.size;
  remaining_size = req.size;

  total_buffs = ceilf( (float)
      size/ (ibv_eager_limit - RC_SR_HEADER_SIZE));

  sctk_nodebug("Send rc_sr message. size %d, src_process %lu, nb buffs %d",
      size, req.dest_process, total_buffs);

  allowed_copy_size = ibv_eager_limit
    - RC_SR_HEADER_SIZE;

  /* while it reamins slots to copy */
  while (nb_buffers <= total_buffs)
  {
    ibuf = sctk_net_ibv_ibuf_pick(0, 1);
    payload = (void*) RC_SR_PAYLOAD(ibuf->buffer);

    sctk_nodebug("%d - Allowed size to be copied : %d %d", nb_buffers, remaining_size, allowed_copy_size);

    /* compute the size to copy */
    if ( remaining_size > allowed_copy_size)
    {
      size_to_copy = allowed_copy_size;
    } else {
      size_to_copy = remaining_size;
    }

    sctk_net_ibv_ibuf_send_init(ibuf, size_to_copy + RC_SR_HEADER_SIZE);
    memcpy(payload, (char*) coll_msg + copied_size, size_to_copy);

    req.size = size;
    req.payload_size = size_to_copy;
    req.buff_nb = nb_buffers;
    req.total_copied = copied_size;
    req.total_buffs = total_buffs;
    sctk_net_ibv_comp_rc_sr_send(ibuf, req, RC_SR_EAGER);

    copied_size += size_to_copy;
    remaining_size -= size_to_copy;

    sctk_nodebug("Send message  to %d (size:%lu, remaining:%lu, nb_buffers:%lu, total_buffers:%lu, psn:%u, total_copied:%lu, size:%lu)", req.dest_process, copied_size, remaining_size, nb_buffers, total_buffs, req.psn, req.total_copied, size);

    nb_buffers++;
  }

  sctk_nodebug("Eager msg sent to process %lu!", req.dest_process);
}

#endif
