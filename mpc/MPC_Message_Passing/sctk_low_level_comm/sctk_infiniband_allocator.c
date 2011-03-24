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

#include "sctk_list.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_const.h"
#include "sctk_infiniband_config.h"
#include "sctk_infiniband_profiler.h"
#include "sctk_infiniband_scheduling.h"
#include "sctk_bootstrap.h"
#include "sctk.h"
#include "sctk_thread.h"
#include "sctk_config.h"

sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;
extern pending_entry_t pending[MAX_NB_TASKS_PER_PROCESS];

  sctk_net_ibv_allocator_t*
sctk_net_ibv_allocator_new()
{
  int i;

  /* FIXME: change to HashTables */
  size_t size = sctk_process_number * sizeof(sctk_net_ibv_allocator_entry_t);

  sctk_net_ibv_allocator = sctk_malloc(sizeof(sctk_net_ibv_allocator_t));

  sctk_net_ibv_allocator->entry = sctk_malloc(size);
  sctk_nodebug("creation : %p", sctk_net_ibv_allocator->entry);
  memset(sctk_net_ibv_allocator->entry, 0, size);

  for (i=0; i < sctk_process_number; ++i)
  {
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_sr_lock, NULL);
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_rdma_lock, NULL);
  }

  return sctk_net_ibv_allocator;
}

void
sctk_net_ibv_allocator_register(
    unsigned int rank,
    void* entry,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      if (sctk_net_ibv_allocator->entry[rank].rc_sr)
      {
        sctk_error("Process %d already registered to sctk_net_ibv_allocator RC_SR", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_sr = entry;

      sctk_ibv_profiler_inc(IBV_QP_CONNECTED);
      break;

    case IBV_CHAN_RC_RDMA:
      if (sctk_net_ibv_allocator->entry[rank].rc_rdma)
      {
        sctk_error("Process %d already registered to chan RC_RDMA", rank);
        sctk_abort();
      }
      sctk_net_ibv_allocator->entry[rank].rc_rdma = entry;
      sctk_nodebug("Registering %p", &sctk_net_ibv_allocator->entry[rank]);
      break;

    default: assume(0); break;
  }
}

void
sctk_net_ibv_allocator_lock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
   switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);
      break;

    default: assume(0); break;
  }
}

void
sctk_net_ibv_allocator_unlock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
   switch(type) {
    case IBV_CHAN_RC_SR:
      sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);
      break;

    case IBV_CHAN_RC_RDMA:
      sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);
      break;

    default: assume(0); break;
  }
}

void*
sctk_net_ibv_allocator_get(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type)
{
  switch(type) {
    case IBV_CHAN_RC_SR:
      return sctk_net_ibv_allocator->entry[rank].rc_sr;
      break;

    case IBV_CHAN_RC_RDMA:
      return sctk_net_ibv_allocator->entry[rank].rc_rdma;
      break;

    default: assume(0); break;
  }
  return NULL;
}

/*-----------------------------------------------------------
 *  POLLING RC SR
 *----------------------------------------------------------*/
void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_send(wc, rc_sr_local, lookup);
}

void sctk_net_ibv_rc_sr_recv_cq(struct ibv_wc* wc, int lookup, int dest)
{
  sctk_net_ibv_rc_sr_poll_recv(wc, rail, rc_sr_local,
      rc_rdma_local, lookup, dest);
}

/*-----------------------------------------------------------
 *  POLLING
 *----------------------------------------------------------*/
void
sctk_net_ibv_allocator_ptp_lookup(int dest, sctk_net_ibv_allocator_type_t type)
{
  switch (type) {
    case IBV_CHAN_RC_SR :
      sctk_net_ibv_cq_lookup(rc_sr_local->recv_cq, 1, sctk_net_ibv_rc_sr_recv_cq, dest, type | IBV_CHAN_RECV);
      sctk_net_ibv_cq_lookup(rc_sr_local->send_cq, ibv_wc_out_number, sctk_net_ibv_rc_sr_send_cq, dest, type | IBV_CHAN_SEND);
      break;

    default: assume(0); break;
  }
}

static sctk_spinlock_t ptp_lock = SCTK_SPINLOCK_INITIALIZER;
void sctk_net_ibv_allocator_ptp_poll_all()
{
  int ret;
  int i = 0;
  if (sctk_spinlock_trylock(&ptp_lock) == 0)
  {
//    sctk_net_ibv_allocator_ptp_poll(IBV_CHAN_RC_SR);
    sctk_net_ibv_cq_poll(rc_sr_local->recv_cq, ibv_wc_in_number, sctk_net_ibv_rc_sr_recv_cq, IBV_CHAN_RC_SR | IBV_CHAN_RECV);
    sctk_net_ibv_cq_poll(rc_sr_local->send_cq, ibv_wc_out_number, sctk_net_ibv_rc_sr_send_cq, IBV_CHAN_RC_SR | IBV_CHAN_SEND);
#if 0
    while((pending[i].task_nb != -1) && (i < MAX_NB_TASKS_PER_PROCESS))
    {
      do {
        ret = sctk_net_ibv_sched_poll_pending(pending[i].task_nb);
      } while(ret);
      ++i;
    }
#endif
    sctk_spinlock_unlock(&ptp_lock);
  }
}


void sctk_net_ibv_allocator_ptp_lookup_all(int dest)
{
  int ret;
  int tmp, restart = 0;
  int i = 0;

#if 0
  while( (pending[i].task_nb != -1) && (i < MAX_NB_TASKS_PER_PROCESS))
  {
    do {
      /* poll pending messages */
      ret = sctk_net_ibv_sched_poll_pending(pending[i].task_nb);
    } while(ret);
    ++i;
  }
#endif

  sctk_net_ibv_allocator_ptp_lookup(dest, IBV_CHAN_RC_SR);
}

/**
 *  \brief Fonction which sends a PTP message
 */
void
sctk_net_ibv_allocator_send_ptp_message ( sctk_thread_ptp_message_t * msg,
    int dest_process ) {
  DBG_S(1);
  size_t size;
  sctk_net_ibv_rc_rdma_process_t*     rc_rdma_entry = NULL;

  /* profile if message is contigous or not (if we can
   * use the zerocopy method of not) */
#if IBV_ENABLE_PROFILE == 1
  void* entire_msg = NULL;
  entire_msg = sctk_net_if_one_msg_in_buffer(msg);
  if (entire_msg) sctk_ibv_profiler_inc(IBV_MSG_DIRECTLY_PINNED);
  else sctk_ibv_profiler_inc(IBV_MSG_NOT_DIRECTLY_PINNED);
  sctk_ibv_profiler_add(IBV_PTP_SIZE, (size + sizeof(sctk_thread_ptp_message_t)));
  sctk_ibv_profiler_inc(IBV_PTP_NB);
#endif

 /* determine message number */
  size = sctk_net_determine_message_size(msg);

  sctk_net_ibv_allocator->entry[dest_process].nb_ptp_msg_transfered++;

  /* EAGER MODE */
  if ( (size + sizeof(sctk_thread_ptp_message_t)) + RC_SR_HEADER_SIZE <= ibv_eager_threshold)
  {
    sctk_ibv_profiler_inc(IBV_EAGER_NB);

    sctk_nodebug("Send EAGER message to %d and size %lu", dest_process, size);
    sctk_nodebug("Destination : %d", ((sctk_thread_ptp_message_t*) msg)->header.local_source);

    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, msg,
        dest_process, ((sctk_thread_ptp_message_t*) msg)->header.destination, size, RC_SR_EAGER);

    /* FRAG EAGER MODE */
  } else if ( (size + sizeof(sctk_thread_ptp_message_t)) <= ibv_frag_eager_threshold) {
    sctk_ibv_profiler_inc(IBV_FRAG_EAGER_NB);
    sctk_nodebug("Send FRAG EAGER message to %d (task %d) and size %lu", dest_process, msg->header.destination, size);

    sctk_net_ibv_comp_rc_sr_send_frag_ptp_message (
        rc_sr_local, msg,
        dest_process, size, RC_SR_EAGER);

    /* RDVZ MODE */
  } else {

    sctk_ibv_profiler_inc(IBV_RDVZ_READ_NB);
    sctk_net_ibv_allocator_lock(dest_process, IBV_CHAN_RC_RDMA);
    rc_rdma_entry = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_RDMA);

    /*
     * check if the dest process exists in the RDVZ remote list.
     * If it doesn't, we create a structure for the RC_RDMA entry
     */
    if (!rc_rdma_entry)
    {
      sctk_nodebug("Need connection to process %d", dest_process);

      rc_rdma_entry = sctk_net_ibv_comp_rc_rdma_allocate_init(dest_process, rc_rdma_local);

      sctk_net_ibv_allocator_register(dest_process, rc_rdma_entry, IBV_CHAN_RC_RDMA);
    }
    sctk_net_ibv_allocator_unlock(dest_process, IBV_CHAN_RC_RDMA);

    sctk_nodebug("Send RENDEZVOUS message to %d and size %lu", dest_process, size);

    sctk_net_ibv_comp_rc_rdma_send_request (
        rail,
        rc_sr_local,
        rc_rdma_local,
        msg,
        dest_process, size, rc_rdma_entry);
  }
  DBG_E(1);
}


/**
 *  Send a collective message to a specific process. According to the size of
 *  the process, the message will use eager or rdvz protocol
 *  \param
 *  \return
 */
void
sctk_net_ibv_allocator_send_coll_message(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_rc_sr_msg_type_t type)
{
  if (type == RC_SR_REDUCE)
  {
    sctk_ibv_profiler_add(IBV_REDUCE_SIZE, size);
    sctk_ibv_profiler_inc(IBV_REDUCE_NB);
  } else if (type == RC_SR_BCAST)
  {
    sctk_ibv_profiler_add(IBV_BCAST_SIZE, size);
    sctk_ibv_profiler_inc(IBV_BCAST_NB);
  }
  sctk_ibv_profiler_add(IBV_COLL_SIZE, size);
  sctk_ibv_profiler_inc(IBV_COLL_NB);

  /* EAGER MODE */
  if ( (size + sizeof(sctk_thread_ptp_message_t)) + RC_SR_HEADER_SIZE <= ibv_eager_threshold)
  {
    sctk_nodebug("Send EAGER collective");

    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, msg,
        dest_process, -1, size, type);

  } else { /* RDVZ MODE */
    sctk_nodebug("Send RDVZ collective");
    sctk_net_ibv_rc_rdma_process_t* rc_rdma_entry;

    assume(0);

    sctk_net_ibv_allocator_lock(dest_process, IBV_CHAN_RC_RDMA);
    rc_rdma_entry = sctk_net_ibv_allocator_get(dest_process, IBV_CHAN_RC_RDMA);
    /*
     * check if the dest process exists in the RDVZ remote list.
     * If it doesn't, we create a structure for the RC_RDMA entry
     */
    if (!rc_rdma_entry)
    {
      rc_rdma_entry = sctk_net_ibv_comp_rc_rdma_allocate_init(dest_process, rc_rdma_local);
      sctk_net_ibv_allocator_register(dest_process, rc_rdma_entry, IBV_CHAN_RC_RDMA);
    }
    sctk_net_ibv_allocator_unlock(dest_process, IBV_CHAN_RC_RDMA);

    sctk_net_ibv_comp_rc_rdma_send_coll_request(
        rail, local_rc_sr, msg,
        dest_process, size, rc_rdma_entry, type);
  }
  DBG_E(1);
}
