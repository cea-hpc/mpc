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
#include "sctk_infiniband_const.h"
#include "sctk_infiniband_profiler.h"
#include "sctk_buffered_fifo.h"

#define LOOK_AND_CREATE_ENTRY(process, task) \
{  int entry_nb = -1;  \
  int i = 0;          \
  while(entry_nb == -1)  \
  {                   \
    if (sctk_net_ibv_allocator->entry[process].sched[i].task_nb == task) \
    { entry_nb = i; } \
    if (sctk_net_ibv_allocator->entry[process].sched[i].task_nb == -1) \
    { entry_nb = 1;   \
      sctk_net_ibv_allocator->entry[process].sched[i].task_nb = task_nb; \
      sctk_nodebug("New task %d created for process %d", task, process); \
    } \
    i++;            \
  }}

extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
static sctk_thread_mutex_t lock;

pending_entry_t pending[MAX_NB_TASKS_PER_PROCESS];
sctk_spinlock_t pending_lock = SCTK_SPINLOCK_INITIALIZER;
int pending_task_entry;

void
sctk_net_ibv_sched_init() {
  int i, j;

  for(i=0; i < sctk_process_number; ++i)
  {
    for (j=0; j < MAX_NB_TASKS_PER_PROCESS; ++j)
    {
      sctk_net_ibv_allocator->entry[i].sched[j].task_nb = -1;
      sctk_net_ibv_allocator->entry[i].sched[j].esn = 0;
      sctk_net_ibv_allocator->entry[i].sched[j].psn = 0;
    }
  }

  memset(&pending, -1, MAX_NB_TASKS_PER_PROCESS * sizeof(pending_entry_t));

  sctk_thread_mutex_init(&lock, NULL);
}

/*-----------------------------------------------------------
 *  PSN
 *----------------------------------------------------------*/
void
sctk_net_ibv_sched_pending_init(
    sctk_net_ibv_allocator_type_t type, int entry_nb)
{

  switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_list_new(&pending[entry_nb].rc_sr, 0);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_list_new(&pending[entry_nb].rc_rdma, 0);
      break;

    case IBV_CHAN_RC_SR_FRAG:
      sctk_list_new(&pending[entry_nb].rc_sr_frag, 0);
      break;


    default: assume(0); break;
  }
}

#define LOCK_AND_PUSH(list, msg) \
  sctk_list_lock(list); \
sctk_list_push(list, msg); \
sctk_list_unlock(list);


void
sctk_net_ibv_sched_pending_push(
    void* ptr,
    size_t size,
    int allocation_needed,
    sctk_net_ibv_allocator_type_t type)
{
  void* msg;
  int task_id;
  int thread_id;
  int entry_nb = -1;
  int i = 0;
  sctk_get_thread_info (&task_id, &thread_id);

  if (allocation_needed)
  {
    msg = sctk_malloc(size);
    sctk_ibv_profiler_inc(IBV_MEM_TRACE);
    memcpy(msg, ptr, size);
  } else {
    msg = ptr;
  }

  switch(type) {
    case IBV_CHAN_RC_SR_FRAG:
      sctk_nodebug("New Message RC_SR pushed %p", msg);
      LOCK_AND_PUSH(&pending[task_id].rc_sr_frag, msg);
      break;

    case IBV_CHAN_RC_SR:
      sctk_nodebug("New Message RC_SR pushed %p", msg);
      LOCK_AND_PUSH(&pending[task_id].rc_sr, msg);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_nodebug("New Message RC_RDMA pushed ");
      LOCK_AND_PUSH(&pending[task_id].rc_rdma, msg);
      break;

    default: assume(0); break;
  }
}

  int
sctk_net_ibv_sched_rc_sr_poll_pending(int task_nb)
{
  struct sctk_list_elem* tmp = NULL;
  sctk_net_ibv_rc_sr_msg_header_t* msg;
  int ret;

  sctk_list_lock(&pending[task_nb].rc_sr);
  tmp = pending[task_nb].rc_sr.head;
  while(tmp)
  {
    msg = tmp->elem;
    ret = sctk_net_ibv_sched_sn_check_and_inc(msg->src_process,
        msg->src_task, msg->psn);

    if (!ret)
    {
      sctk_nodebug("PENDING - Recv EAGER message from %d (PSN %d)", msg->src_process, msg->psn );

      sctk_net_ibv_send_msg_to_mpc(
          (sctk_thread_ptp_message_t*) RC_SR_PAYLOAD(msg),
          (char*) RC_SR_PAYLOAD(msg)
          + sizeof(sctk_thread_ptp_message_t), msg->src_process,
          IBV_POLL_RC_SR_ORIGIN, msg);

      //THERE
      sctk_list_remove(&pending[task_nb].rc_sr, tmp);
      sctk_list_unlock(&pending[task_nb].rc_sr);
      return 1;
    }

    tmp = tmp->p_next;
  }
  sctk_list_unlock(&pending[task_nb].rc_sr);
  return 0;
}

  int
sctk_net_ibv_sched_rc_sr_frag_poll_pending(int task_nb)
{
  struct sctk_list_elem* tmp = NULL;
  sctk_net_ibv_frag_eager_entry_t* msg;
  int ret;

  sctk_list_lock(&pending[task_nb].rc_sr_frag);
  tmp = pending[task_nb].rc_sr_frag.head;
  while(tmp)
  {
    msg = tmp->elem;
    ret = sctk_net_ibv_sched_sn_check_and_inc(msg->src_process, msg->src_task, msg->psn);

    if (!ret)
    {
      sctk_nodebug("PENDING - Recv FRAG EAGER message from %d (PSN %d)", msg->src_process, msg->psn );

      sctk_net_ibv_send_msg_to_mpc(
          (sctk_thread_ptp_message_t*) msg->msg,
          (char*) msg->msg
          + sizeof(sctk_thread_ptp_message_t), msg->src_process,
          IBV_POLL_RC_SR_FRAG_ORIGIN, msg);

      //THERE
      sctk_list_remove(&pending[task_nb].rc_sr_frag, tmp);
      sctk_list_unlock(&pending[task_nb].rc_sr_frag);
      return 1;
    }

    tmp = tmp->p_next;
  }
  sctk_list_unlock(&pending[task_nb].rc_sr_frag);
  return 0;
}

  int
sctk_net_ibv_sched_rc_rdma_poll_pending(int task_nb)
{
  struct sctk_list_elem* tmp = NULL;
  sctk_net_ibv_rc_rdma_entry_t* msg;
  int ret;
  int src_process;

  sctk_list_lock(&pending[task_nb].rc_rdma);
  tmp = pending[task_nb].rc_rdma.head;
  while(tmp)
  {
    msg = tmp->elem;
    ret = sctk_net_ibv_sched_sn_check_and_inc(msg->src_process, msg->src_task, msg->psn);

    if (!ret)
    {

      sctk_nodebug("PENDING - Recv RDVZ message from %d (PSN %d) (%p)", msg->src_process, msg->psn, msg);

      sctk_list_remove(&pending[task_nb].rc_rdma, tmp);

      src_process = msg->src_process;
      sctk_net_ibv_comp_rc_rdma_read_msg(msg, IBV_POLL_RC_RDMA_ORIGIN);
      sctk_list_unlock(&pending[task_nb].rc_rdma);

      return 1;
    }

    tmp = tmp->p_next;
  }
  sctk_list_unlock(&pending[task_nb].rc_rdma);

  return 0;
}

  int
sctk_net_ibv_sched_poll_pending(int task_nb)
{
  int tmp, restart = 0;

  tmp = sctk_net_ibv_sched_rc_sr_poll_pending(task_nb);
  if (tmp == 1)
    restart = 1;

  tmp = sctk_net_ibv_sched_rc_sr_frag_poll_pending(task_nb);
  if (tmp == 1)
    restart = 1;

  tmp = sctk_net_ibv_sched_rc_rdma_poll_pending(task_nb);
  if (tmp == 1)
    restart = 1;

  return restart;
}

  uint32_t
sctk_net_ibv_sched_get_esn(int dest, int task_nb)
{
  LOOK_AND_CREATE_ENTRY(dest, task_nb);
  sctk_nodebug("ESN : %lu", sctk_net_ibv_allocator->entry[dest].esn);
  return sctk_net_ibv_allocator->entry[dest].sched[task_nb].esn;
}

  uint32_t
sctk_net_ibv_sched_psn_inc (int dest, int task_nb)
{
  uint32_t i;

  LOOK_AND_CREATE_ENTRY(dest, task_nb);
  //  sctk_net_ibv_sched_lock();
  i = sctk_net_ibv_allocator->entry[dest].sched[task_nb].psn;
  sctk_net_ibv_allocator->entry[dest].sched[task_nb].psn++;
  //  sctk_net_ibv_sched_unlock();

  return i;
}

  uint32_t
sctk_net_ibv_sched_esn_inc (int dest, int task_nb)
{
  uint32_t i;

  LOOK_AND_CREATE_ENTRY(dest, task_nb);
  i = sctk_net_ibv_allocator->entry[dest].sched[task_nb].esn;
  sctk_net_ibv_allocator->entry[dest].sched[task_nb].esn++;
  sctk_nodebug("INC for process %d: %lu", dest, i);
  return i;
}


  uint32_t
sctk_net_ibv_sched_sn_check(int dest, int task_nb, uint64_t num)
{
  LOOK_AND_CREATE_ENTRY(dest, task_nb);
  if ( sctk_net_ibv_allocator->entry[dest].sched[task_nb].esn == num )
    return 1;
  else
    return 0;
}

  int
sctk_net_ibv_sched_sn_check_and_inc(int dest, int task_nb, uint64_t num)
{
  int i;

  if (!sctk_net_ibv_sched_sn_check(dest, task_nb, num)) {
    //        i = sctk_net_ibv_allocator->entry[dest].esn;
    //        sctk_error("Wrong Sequence Number received from %d. Expected %d got %d", dest, i, num);
    return 1;
  }
  else
  {
    sctk_net_ibv_sched_lock();
    sctk_net_ibv_sched_esn_inc (dest, task_nb);
    sctk_net_ibv_sched_unlock();
    return 0;
  }
}


  void
sctk_net_ibv_sched_lock()
{
  sctk_thread_mutex_lock(&lock);
}

  void
sctk_net_ibv_sched_unlock()
{
  sctk_thread_mutex_unlock(&lock);
}

  void
sctk_net_ibv_sched_initialize_threads()
{
  int task_id;
  int thread_id;
  int i;
  int entry_nb = -1;
  sctk_get_thread_info (&task_id, &thread_id);

  sctk_spinlock_lock(&pending_lock);
  for (i=0; i < MAX_NB_TASKS_PER_PROCESS; ++i)
  {
    sctk_nodebug("Task nb : %d", pending[i].task_nb);

    if (pending[i].task_nb == -1)
    {
      entry_nb = i;
      pending[entry_nb].task_nb = task_id;
      sctk_nodebug("%p - Found entry %d -> %d", &pending[entry_nb], i, task_id);
      break;
    }
  }
//  assume (entry_nb != -1);
  sctk_spinlock_unlock(&pending_lock);

  sctk_net_ibv_sched_pending_init(IBV_CHAN_RC_SR, entry_nb);
  sctk_net_ibv_sched_pending_init(IBV_CHAN_RC_SR_FRAG, entry_nb);
  sctk_net_ibv_sched_pending_init(IBV_CHAN_RC_RDMA, entry_nb);
  sctk_nodebug("Initializing thread %d with entry %d", task_id, entry_nb);
}
