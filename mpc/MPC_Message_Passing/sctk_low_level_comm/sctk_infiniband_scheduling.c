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

#include "sctk_infiniband_lib.h"
#include <sctk_infiniband_scheduling.h>
#include <sctk_infiniband_allocator.h>
#include <sctk_infiniband_coll.h>
#include "sctk_infiniband_const.h"
#include "sctk_infiniband_profiler.h"
#include "sctk_buffered_fifo.h"

extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
extern sctk_net_ibv_com_entry_t* com_entries;
/* TODO: can we delete it ? */
sctk_thread_mutex_t lock;

void
sctk_net_ibv_sched_init() {
  sctk_thread_mutex_init(&lock, NULL);
}

/*-----------------------------------------------------------
 *  PSN
 *----------------------------------------------------------*/

/**
 *  Push a msg to the pending list. Also fill the header of the
 *  pending message according to its type.
 *  \param
 *  \return
 */
void
sctk_net_ibv_sched_pending_push(
    void* ptr,
    size_t size,
    int allocation_needed,
    sctk_net_ibv_allocator_type_t type,
    uint64_t src_process,
    uint64_t src_task,
    uint64_t dest_task,
    uint64_t psn,
    sctk_thread_ptp_message_t* msg_header,
    void* msg_payload,
    void* struct_ptr)
{
  void* msg;
  int entry_nb = -1;
  sctk_net_ibv_sched_pending_header header;

  sctk_nodebug("[task %lu] Pushing msg (src_process:%lu, src_task:%lu, psn:%lu, type %d) to pending list",
      dest_task, src_process, src_task, psn, type);

  /* fill header for msg payload */
  header.src_process = src_process;
  header.src_task = src_task;
  header.psn = psn;
  header.type = type;

  if (allocation_needed)
  {
    sctk_nodebug("Allocate memory");
    msg = sctk_malloc(size);
    sctk_ibv_profiler_inc(IBV_MEM_TRACE);
    memcpy(msg, ptr, size);

    header.msg_header = msg;
    header.msg_payload = (char*) msg + sizeof(sctk_thread_ptp_message_t);
    header.struct_ptr = msg;
  } else {
    sctk_nodebug("No memory allocated");
    msg = ptr;
    header.msg_header = msg_header;
    header.msg_payload = msg_payload;
    header.struct_ptr = struct_ptr;
  }


  /* lookup the local thread entry */
  entry_nb = LOOKUP_LOCAL_THREAD_ENTRY(dest_task);
  sctk_nodebug("Entry nb : %d (header %p, payload %p, size header %lu)", entry_nb, header.msg_header, header.msg_payload, sizeof(header));

  sctk_list_lock(&all_tasks[entry_nb].pending_msg);
  sctk_list_push(&all_tasks[entry_nb].pending_msg, &header);
  sctk_list_unlock(&all_tasks[entry_nb].pending_msg);
}

  int
sctk_net_ibv_sched_poll_pending_msg(int task_nb)
{
  struct sctk_list_elem* tmp = NULL;
  sctk_net_ibv_sched_pending_header* header;
  int ret;

  sctk_list_lock(&all_tasks[task_nb].pending_msg);
  tmp = all_tasks[task_nb].pending_msg.head;

  while(tmp)
  {
    header = tmp->elem;

    ret = sctk_net_ibv_sched_sn_check_and_inc(
        header->src_process,
        header->src_task, header->psn);

    if (!ret)
    {
      sctk_nodebug("[task %lu] PENDING - Recv EAGER message from %d, task %d (PSN %d, struct ptr %p, type %d, header %p, payload %p)", all_tasks[task_nb].task_nb, header->src_process, header->src_task, header->psn, header->struct_ptr, header->type, header->msg_header, header->msg_payload);

      sctk_net_ibv_send_msg_to_mpc(
          header->msg_header,
          header->msg_payload,
          header->src_process,
          header->type, header->struct_ptr);

      //THERE
      sctk_list_remove(&all_tasks[task_nb].pending_msg, tmp);
      sctk_list_unlock(&all_tasks[task_nb].pending_msg);
      /*  FIXME there */
      return 0;
    }

    tmp = tmp->p_next;
  }
  sctk_list_unlock(&all_tasks[task_nb].pending_msg);
  return 0;
}

/*-----------------------------------------------------------
 *  PTP SEQUENCE NUMBERS
 *----------------------------------------------------------*/

  static sched_sn_t*
sched_get_sn(int src_task, int dest_task)
{
  return &all_tasks[src_task].remote[dest_task];
}

  uint32_t
sctk_net_ibv_sched_get_esn(int src_task, int dest_task)
{
  sched_sn_t* sn;
  int local_nb = -1;

  local_nb = LOOKUP_LOCAL_THREAD_ENTRY(src_task);
  sn = sched_get_sn(local_nb, dest_task);

  sctk_nodebug("ESN : %lu", sn->esn);
  return sn->esn;
}

  uint32_t
sctk_net_ibv_sched_psn_inc (int src_task, int dest_task)
{
  uint32_t i;
  sched_sn_t* sn;
  int local_nb = -1;

  local_nb = LOOKUP_LOCAL_THREAD_ENTRY(src_task);
  sn = sched_get_sn(local_nb, dest_task);

  i = sn->psn;
  sn->psn++;
  return i;
}

  uint32_t
sctk_net_ibv_sched_esn_inc (int src_task, int dest_task)
{
  uint32_t i;
  sched_sn_t* sn;
  int local_nb = -1;

  local_nb = LOOKUP_LOCAL_THREAD_ENTRY(src_task);
  sn = sched_get_sn(local_nb, dest_task);

  i = sn->esn;
  sn->esn++;
  sctk_nodebug("INC for process %d (task %d): %lu", dest, entry_nb, i);
  return i;
}


  uint32_t
sctk_net_ibv_sched_sn_check(int src_task, int dest_task, uint64_t num)
{
  sched_sn_t* sn;
  int local_nb = -1;

  local_nb = LOOKUP_LOCAL_THREAD_ENTRY(src_task);
  sn = sched_get_sn(local_nb, dest_task);

  if ( sn->esn == num )
    return 1;
  else
    return 0;
}

  int
sctk_net_ibv_sched_sn_check_and_inc(int src_task, int dest_task, uint64_t num)
{

  if (!sctk_net_ibv_sched_sn_check(src_task, dest_task, num)) {
    return 1;
  }
  else
  {
    SCHED_LOCK;
    sctk_net_ibv_sched_esn_inc (src_task, dest_task);
    SCHED_UNLOCK;
    return 0;
  }
}

/*-----------------------------------------------------------
 *  COLL SEQUENCE NUMBERS
 *----------------------------------------------------------*/

uint32_t
sctk_net_ibv_sched_coll_psn_inc (sctk_net_ibv_ibuf_ptp_type_t type, int com_id)
{
  int ret;

  switch (type) {
    case IBV_BCAST:
      ret = com_entries[com_id].bcast.psn++;
      return ret;
      break;

    case IBV_REDUCE:
      ret = com_entries[com_id].reduce.psn++;
      return ret;
      break;

    default:
      assume(0);
      break;
  }
  return 0;
}

uint32_t
sctk_net_ibv_sched_coll_esn_inc (int com_id)
{
  return 0;
}

uint32_t
sctk_net_ibv_sched_coll_get_esn(int com_id)
{
  return 0;
}

uint32_t
sctk_net_ibv_sched_coll_sn_check(int com_id)
{

  return 0;
}

int
sctk_net_ibv_sched_coll_sn_check_and_inc(int com_id)
{

  return 0;
}

