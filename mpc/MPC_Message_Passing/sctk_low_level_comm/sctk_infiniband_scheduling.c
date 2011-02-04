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
#include "sctk_list.h"
#include "sctk_buffered_fifo.h"

extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
extern struct sctk_list rc_sr_pending;
extern struct sctk_list rc_rdma_pending;

struct sctk_buffered_fifo sctk_net_ibv_pending_rc_sr;
static sctk_spinlock_t lock;

void
sctk_net_ibv_sched_init() {
  int i;

  for(i=0; i < sctk_process_number; ++i)
  {
    sctk_net_ibv_allocator->entry[i].esn = 0;
    sctk_net_ibv_allocator->entry[i].psn = 0;
  }

  lock = SCTK_SPINLOCK_INITIALIZER;

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


  int
  sctk_net_ibv_sched_rc_sr_free_pending_msg(sctk_thread_ptp_message_t * item )
{
  struct sctk_list_elem* tmp = rc_sr_pending.head;
  sctk_net_ibv_rc_sr_msg_header_t* msg;
  int ret;

  while(tmp)
  {
    msg = tmp->elem;

    if (&(msg->payload) == (char*) item)
    {
      sctk_nodebug("FREE EAGER message");
      sctk_list_lock(&rc_sr_pending);
      sctk_list_remove(&rc_sr_pending, tmp);
      sctk_list_unlock(&rc_sr_pending);
      free(msg);
      return 1;
    }

    tmp = tmp->p_next;
  }
  return 0;
}


  void
  sctk_net_ibv_sched_rc_sr_poll_pending()
{
  struct sctk_list_elem* tmp = rc_sr_pending.head;
  sctk_net_ibv_rc_sr_msg_header_t* msg;
  int ret;

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

//      sctk_list_remove(&rc_sr_pending, tmp);
      return;
    }

    tmp = tmp->p_next;
  }
}

  void
sctk_net_ibv_sched_rc_rdma_poll_pending()
{
  struct sctk_list_elem* tmp = rc_rdma_pending.head;
  sctk_net_ibv_rc_rdma_entry_recv_t* msg;
  sctk_net_ibv_rc_rdma_process_t* entry_rc_rdma;
  int ret;

  while(tmp)
  {
    msg = tmp->elem;

    ret = sctk_net_ibv_sched_sn_check_and_inc(msg->src_process, msg->psn);

    if (!ret)
    {
      sctk_nodebug("PENDING - Recv RDVZ message from %d (PSN %d)", msg->src_process, msg->psn);

      sctk_net_ibv_comp_rc_rdma_read_msg(msg);

      entry_rc_rdma = (sctk_net_ibv_rc_rdma_process_t*)
        sctk_net_ibv_allocator_get(msg->src_process, IBV_CHAN_RC_RDMA);
      sctk_net_ibv_allocator_rc_rdma_process_next_request(entry_rc_rdma);

      return;
    }

    tmp = tmp->p_next;
  }
}

  void
sctk_net_ibv_sched_poll_pending()
{
  sctk_net_ibv_sched_rc_sr_poll_pending();
  sctk_net_ibv_sched_rc_rdma_poll_pending();

}

  int
sctk_net_ibv_sched_get_esn(int dest)
{
  return sctk_net_ibv_allocator->entry[dest].esn;
}

  int
sctk_net_ibv_sched_psn_inc (int dest)
{
  int i;

  //  sctk_net_ibv_sched_lock();
  i = sctk_net_ibv_allocator->entry[dest].psn;
  sctk_net_ibv_allocator->entry[dest].psn++;
  //  sctk_net_ibv_sched_unlock();

  return i;
}

  int
sctk_net_ibv_sched_esn_inc (int dest)
{
  int i;
  i = sctk_net_ibv_allocator->entry[dest].esn;
  sctk_net_ibv_allocator->entry[dest].esn++;
  return i;
}


  int
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
  sctk_spinlock_lock(&lock);
}

  void
sctk_net_ibv_sched_unlock()
{
  sctk_spinlock_unlock(&lock);
}

