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
#include "sctk_buffered_fifo.h"

extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
struct sctk_list rc_sr_pending;
struct sctk_list rc_rdma_pending;

struct sctk_buffered_fifo sctk_net_ibv_pending_rc_sr;
static sctk_thread_mutex_t lock;

void
sctk_net_ibv_sched_init() {
  int i;

  for(i=0; i < sctk_process_number; ++i)
  {
    sctk_net_ibv_allocator->entry[i].esn = 0;
    sctk_net_ibv_allocator->entry[i].psn = 0;
  }

  sctk_thread_mutex_init(&lock, NULL);

  sctk_buffered_fifo_new(&sctk_net_ibv_pending_rc_sr,
      sizeof(sctk_net_ibv_rc_sr_msg_header_t), 32, 0);
}

void
sctk_net_ibv_sched_queue_msg(void* msg, sctk_net_ibv_allocator_type_t type)
{
  switch (type) {
    case IBV_CHAN_RC_SR:
      sctk_buffered_fifo_push(&sctk_net_ibv_pending_rc_sr, msg);
      break;

    case IBV_CHAN_RC_RDMA:
      break;

    default: assume(0); break;
  }
}


/*-----------------------------------------------------------
 *  PSN
 *----------------------------------------------------------*/
  uint32_t
sctk_net_ibv_sched_rc_sr_get_tail_psn(int* ret)
{
  sctk_net_ibv_rc_rdma_entry_recv_t*  entry_recv;

  entry_recv = sctk_buffered_fifo_get_elem_from_tail(&sctk_net_ibv_pending_rc_sr, 0);
  if (entry_recv)
  {
    *ret = 0;
    return entry_recv->psn;
  }
  else
  {
    *ret = 1;
    return 0;
  }
}

//  uint32_t
//sctk_net_ibv_sched_rc_rdma_get_tail_psn(
//    struct sctk_list *rc_rdma, int rank, int* ret)
//{
//  sctk_net_ibv_rc_rdma_process_t*     entry_rc_rdma;
//  sctk_net_ibv_rc_rdma_entry_recv_t*  entry_recv;
//
//  entry_rc_rdma = sctk_net_ibv_comp_rc_rdma_find_from_rank(
//      rc_rdma, rank);
//
//  entry_recv = sctk_buffered_fifo_get_elem_from_tail(&entry_rc_rdma->recv, 0);
//
//  if ( (entry_recv) && (entry_recv->used == 1)
//      && (entry_recv->sent_to_mpc == 0) ) {
//
//    *ret = 0;
//    return entry_recv->psn;
//
//  }
//  *ret = 1;
//  return 0;
//}

void
sctk_net_ibv_sched_pending_init(
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_list_new(&rc_sr_pending, 0);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_list_new(&rc_rdma_pending, 0);
      break;

    default: assume(0); break;
  }
}

void
sctk_net_ibv_sched_pending_push(
    void* ptr,
    size_t size,
    int allocation_needed,
    sctk_net_ibv_allocator_type_t type)
{
  void* msg;

  if (allocation_needed)
  {
    msg = sctk_malloc(size);
    memcpy(msg, ptr, size);
  } else {
    msg = ptr;
  }

  switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_nodebug("New Message RC_SR pushed %p", msg);
      sctk_list_lock(&rc_sr_pending);
      sctk_list_push(&rc_sr_pending, msg);
      sctk_list_unlock(&rc_sr_pending);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_nodebug("New Message RC_RDMA pushed ");
      sctk_list_lock(&rc_rdma_pending);
      sctk_list_push(&rc_rdma_pending, msg);
      sctk_list_unlock(&rc_rdma_pending);
      break;

    default: assume(0); break;
  }
}


  void
  sctk_net_ibv_sched_rc_sr_poll_pending()
{
  struct sctk_list_elem* tmp = NULL;
  sctk_net_ibv_rc_sr_msg_header_t* msg;
  int ret;

  sctk_list_lock(&rc_sr_pending);
  tmp = rc_sr_pending.head;
  while(tmp)
  {
    msg = tmp->elem;
    ret = sctk_net_ibv_sched_sn_check_and_inc(msg->src_process, msg->psn);

    if (!ret)
    {
      sctk_nodebug("PENDING - Recv EAGER message from %d (PSN %d)", msg->src_process, msg->psn );

      sctk_net_ibv_send_msg_to_mpc(
          (sctk_thread_ptp_message_t*) &(msg->payload),
          (char*) &(msg->payload)
          + sizeof(sctk_thread_ptp_message_t), msg->src_process,
          IBV_POLL_RC_SR_ORIGIN, msg);

      //THERE
      sctk_list_remove(&rc_sr_pending, tmp);
      sctk_list_unlock(&rc_sr_pending);
      return;
    }

    tmp = tmp->p_next;
  }
  sctk_list_unlock(&rc_sr_pending);
}

#if 0
  void
sctk_net_ibv_sched_rc_rdma_poll_pending()
{
  struct sctk_list_elem* tmp = NULL;
  sctk_net_ibv_rc_rdma_entry_recv_t* msg;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  int ret;
  int src_process;

  sctk_list_lock(&rc_rdma_pending);
  tmp = rc_rdma_pending.head;
  while(tmp)
  {
    msg = tmp->elem;
    ret = sctk_net_ibv_sched_sn_check_and_inc(msg->src_process, msg->psn);

    if (!ret)
    {

      sctk_nodebug("PENDING - Recv RDVZ message from %d (PSN %d) (%p)", msg->src_process, msg->psn, msg);

      sctk_list_remove(&rc_rdma_pending, tmp);

      src_process = msg->src_process;
      sctk_net_ibv_comp_rc_rdma_read_msg(msg, IBV_POLL_RC_RDMA_ORIGIN);
      sctk_list_unlock(&rc_rdma_pending);

      entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
        sctk_net_ibv_allocator_get(src_process, IBV_CHAN_RC_RDMA);
      sctk_net_ibv_allocator_rc_rdma_process_next_request(entry_rc_rdma);
      return;
    }

    tmp = tmp->p_next;
  }
  sctk_list_unlock(&rc_rdma_pending);
}
#endif

  void
sctk_net_ibv_sched_poll_pending()
{
  sctk_net_ibv_sched_rc_sr_poll_pending();
//  sctk_net_ibv_sched_rc_rdma_poll_pending();

}

  uint32_t
sctk_net_ibv_sched_get_esn(int dest)
{
  return sctk_net_ibv_allocator->entry[dest].esn;
}

  uint32_t
sctk_net_ibv_sched_psn_inc (int dest)
{
  uint32_t i;

  //  sctk_net_ibv_sched_lock();
  i = sctk_net_ibv_allocator->entry[dest].psn;
  sctk_net_ibv_allocator->entry[dest].psn++;
  //  sctk_net_ibv_sched_unlock();

  return i;
}

  uint32_t
sctk_net_ibv_sched_esn_inc (int dest)
{
  uint32_t i;
  i = sctk_net_ibv_allocator->entry[dest].esn;
  sctk_net_ibv_allocator->entry[dest].esn++;
  return i;
}


  uint32_t
sctk_net_ibv_sched_sn_check(int dest, uint64_t num)
{
  if ( sctk_net_ibv_allocator->entry[dest].esn == num )
    return 1;
  else
    return 0;
}

  int
sctk_net_ibv_sched_sn_check_and_inc(int dest, uint64_t num)
{
  //  int i;

  if (!sctk_net_ibv_sched_sn_check(dest, num)) {
    //    i = sctk_net_ibv_allocator->entry[dest].esn;
    //    sctk_error("Wrong Sequence Number received from %d. Expected %d got %d", dest, i, num);
    return 1;
  }
  else
  {
    sctk_net_ibv_sched_lock();
    sctk_net_ibv_sched_esn_inc (dest);
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

