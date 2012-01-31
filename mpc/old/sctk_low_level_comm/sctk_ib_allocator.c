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
#include "sctk_ib_allocator.h"
#include "sctk_ib_comp_rc_rdma.h"
#include "sctk_ib_const.h"
#include "sctk_ib_config.h"
#include "sctk_ib_profiler.h"
#include "sctk_ib_scheduling.h"
#include "sctk_ib_frag.h"
#include "sctk_bootstrap.h"
#include "sctk.h"
#include "sctk_thread.h"
#include "sctk_config.h"

sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

sctk_net_ibv_allocator_task_t all_tasks[MAX_NB_TASKS_PER_PROCESS];
sctk_spinlock_t all_tasks_lock = SCTK_SPINLOCK_INITIALIZER;

/*-----------------------------------------------------------
 *  TASKS
 *----------------------------------------------------------*/
/*
 * lookup for a local thread id. If the entry isn't found, we lock the
 * array and create the entry */
int LOOKUP_LOCAL_THREAD_ENTRY(int id) {
  int i;

  for (i=0; i<MAX_NB_TASKS_PER_PROCESS; ++i) {
    sctk_nodebug("(i:%d) Search for entry %d: %d", i, id, all_tasks[i].task_nb);

    /* We take the lock and verify if the entry hasn't
     * been modified before locking */
    if (all_tasks[i].task_nb == -1) {

      sctk_spinlock_lock(&all_tasks_lock);
      if (all_tasks[i].task_nb == -1) {

        /* init list for pending frag eager */
        all_tasks[i].frag_eager_list =
          sctk_calloc(sctk_get_total_tasks_number(), sizeof(struct sctk_list_header*));
        assume(all_tasks[i].frag_eager_list);

        /* init list for pendint messages */
        SCTK_LIST_HEAD_INIT(&all_tasks[i].pending_msg);

        all_tasks[i].remote = sctk_calloc(sizeof(sched_sn_t),
            sctk_get_total_tasks_number());
        assume(all_tasks[i].remote);

        /* finily, we set the task_nb. Must be done
         * at the end */
        all_tasks[i].task_nb = id;
        sctk_nodebug("ENTRY CREATED : %d <-> %d (lock:%p)", i, id, &all_tasks_lock);
      }
      sctk_spinlock_unlock(&all_tasks_lock);
    }

    if (all_tasks[i].task_nb == id)
    {
      return i;
      break;
    }
  }

  /* If we are there, the local entry nb has not been found.
   * We exit... */
  sctk_error("Local thread entry %d not found !", id);
  assume(0);

  return -1;
}


  sctk_net_ibv_allocator_t*
sctk_net_ibv_allocator_new()
{
  int i;

  /* FIXME: change to HashTables */
  size_t size = sctk_process_number * sizeof(sctk_net_ibv_allocator_entry_t);

  sctk_net_ibv_allocator = sctk_malloc(sizeof(sctk_net_ibv_allocator_t) + size);

  sctk_net_ibv_allocator->entry = (sctk_net_ibv_allocator_entry_t*)
    ((char*) sctk_net_ibv_allocator + sizeof(sctk_net_ibv_allocator_t));
  assume(sctk_net_ibv_allocator->entry);

  sctk_nodebug("creation : %p", sctk_net_ibv_allocator->entry);
  memset(sctk_net_ibv_allocator->entry, 0, size);

  /* set up procs list */
  for (i=0; i < sctk_process_number; ++i)
  {
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_sr_lock, NULL);
    sctk_thread_mutex_init(&sctk_net_ibv_allocator->entry[i].rc_rdma_lock, NULL);
    sctk_net_ibv_allocator->entry[i].sched_lock = SCTK_SPINLOCK_INITIALIZER;
  }

  /* set up tasks list */
  for(i=0; i < MAX_NB_TASKS_PER_PROCESS; ++i)
  {
    all_tasks[i].task_nb = -1;
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
 *  POLLING
 *----------------------------------------------------------*/
static void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int dest)
{
  sctk_net_ibv_rc_sr_poll_send(wc, rc_sr_local);
}

static void sctk_net_ibv_rc_sr_recv_cq(struct ibv_wc* wc, int dest)
{
  sctk_net_ibv_rc_sr_poll_recv(wc, rail, rc_sr_local,
      rc_rdma_local, dest);
}

/* TODO: for now, we restrict the adaptive polling to
 * Linux Systems. It could be cool to port it to other systems */
#if defined(Linux_SYS)
/* Begin of experimental */

#define MIN_POLL_PERIOD (1e-7*cpu_frequency)
#define MAX_POLL_PERIOD (1e-5*cpu_frequency)
#define CPU_RELAX_NB 1e4

#define POLL_UP(period) \
  period*=2; \
  if (period > MAX_POLL_PERIOD) period = MAX_POLL_PERIOD;

#define POLL_DOWN(period) \
  period/=2; \
  if (period < MIN_POLL_PERIOD) period = MIN_POLL_PERIOD;

/* for storing times */
__thread double poll_period = 0;
__thread double poll_last_found = 0;
/* store counters */
__thread int poll_nb_found = 0;
__thread int poll_nb_not_found = 0;
__thread double poll_time_found = 0;
__thread double poll_time_not_found = 0;
#else
#warning "System not supported for adaptive polling"
#endif

/* End of experimental */

//static sctk_spinlock_t ptp_send_lock = SCTK_SPINLOCK_INITIALIZER;
//static sctk_spinlock_t ptp_recv_lock = SCTK_SPINLOCK_INITIALIZER;
void sctk_net_ibv_allocator_ptp_poll_all()
{
  int ret;
  int i = 0,j;
  double stamp;
  uint8_t res, found = 0;
  /* if thread must be activated */
  uint8_t activate = 1;

#if defined(Linux_SYS)
  if (ibv_adaptive_polling) {
    /* FIXME: find another place to init poll_period
     * (for example, after thread initialization.) */
    if (poll_period == 0)
      poll_period = MIN_POLL_PERIOD;

    stamp = sctk_get_time_stamp();
    if (poll_last_found == 0)
      poll_last_found = sctk_get_time_stamp();

    if ((stamp - poll_last_found) > poll_period)
      activate = 1;
    else activate = 0;
  } else activate = 1;

  if (activate)
  {
#endif
    while((all_tasks[i].task_nb != -1) && (i < MAX_NB_TASKS_PER_PROCESS))
    {
      sctk_nodebug("tasks %d", all_tasks[i].task_nb);
      ret = sctk_net_ibv_sched_poll_pending_msg(i);
      ++i;
    }

    /* FIXME: For now, we leave the lock because of message corruption. This lock will be removed soon. */
    if (sctk_spinlock_trylock(&ptp_recv_lock) == 0)
    {
      res = sctk_net_ibv_cq_poll(rc_sr_local->recv_cq, ibv_wc_in_number, sctk_net_ibv_rc_sr_recv_cq, IBV_CHAN_RECV);
      sctk_spinlock_unlock(&ptp_recv_lock);
      found = found || res;
    }

    res = sctk_net_ibv_cq_poll(rc_sr_local->send_cq, ibv_wc_out_number, sctk_net_ibv_rc_sr_send_cq, IBV_CHAN_SEND);
    found = found || res;

#if defined(Linux_SYS)
    if (ibv_adaptive_polling) {
      if (found)
      {
        ++poll_nb_found;
        poll_last_found = sctk_get_time_stamp();
        POLL_DOWN(poll_period);
      } else {
        ++poll_nb_not_found;
        POLL_UP(poll_period);
        if (poll_period >= MAX_POLL_PERIOD)
        {
          /* We wait for a while... */
          for(j=0; j<CPU_RELAX_NB; ++j)
            sctk_cpu_relax();
        }
      }
    }
  }
#endif
}

/*-----------------------------------------------------------
 *  PTP SENDING
 *----------------------------------------------------------*/


/**
 *  Function which sends a PTP message to a dest process. The
 *  channel to use is determinated according to the size of
 *  the message to send
 *  \param
 *  \return
 */
void
sctk_net_ibv_allocator_send_ptp_message ( sctk_thread_ptp_message_t * msg,
    int dest_process ) {
  DBG_S(1);
  size_t size;
  size_t size_with_header;
  sctk_net_ibv_allocator_request_t req;
  int task_id;
  int thread_id;

  /* determine the message number and the
   * thread ID of the current thread */
  size = msg->header.msg_size;
  size_with_header = size + sizeof(sctk_thread_ptp_message_t);
  sctk_get_thread_info (&task_id, &thread_id);

  /* profile if message is contigous or not (if we can
   * use the zerocopy method or not) */
#if IBV_ENABLE_PROFILE == 1
  void* entire_msg = NULL;
  entire_msg = sctk_net_if_one_msg_in_buffer(msg);
  if (entire_msg) sctk_ibv_profiler_inc(IBV_MSG_DIRECTLY_PINNED);
  else sctk_ibv_profiler_inc(IBV_MSG_NOT_DIRECTLY_PINNED);
  sctk_ibv_profiler_add(IBV_PTP_SIZE, size_with_header);
  sctk_ibv_profiler_inc(IBV_PTP_NB);
#endif

  /* fill the request */
  req.msg = msg;
  req.dest_process = dest_process;
  req.dest_task = ((sctk_thread_ptp_message_t*) msg)->header.destination;
  req.ptp_type = IBV_PTP;
  req.psn = sctk_net_ibv_sched_psn_inc(task_id,
      req.dest_task);

  if ( size_with_header +
      RC_SR_HEADER_SIZE <= ibv_eager_limit) {
    sctk_ibv_profiler_inc(IBV_EAGER_NB);

    /*
     * EAGER MSG
     */
    req.size = size_with_header;
    req.channel = IBV_CHAN_RC_SR;

    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, req);

  } else if ( size <= ibv_frag_eager_limit) {
    sctk_ibv_profiler_inc(IBV_FRAG_EAGER_NB);
    sctk_ibv_profiler_add(IBV_FRAG_EAGER_SIZE, size_with_header);

    /*
     * FRAG MSG: we don't consider the size of the headers
     */
    req.size = size_with_header;
    req.channel = IBV_CHAN_RC_SR_FRAG;

    sctk_net_ibv_comp_rc_sr_send_frag_ptp_message (rc_sr_local, req);
  } else {
    sctk_ibv_profiler_inc(IBV_RDVZ_READ_NB);

    /*
     * RENDEZVOUS
     */
    req.size = size;
    req.channel = IBV_CHAN_RC_RDMA;

    sctk_net_ibv_comp_rc_rdma_send_request (
        rail, rc_sr_local, req, 1);
  }

  sctk_net_ibv_allocator->entry[dest_process].nb_ptp_msg_transfered++;

  DBG_E(1);
}

/**
 *  Function which sends a PTP collective message to a dest process. The
 *  channel to use is determinated according to the size of
 *  the message to send
 *  \param
 *  \return
 */
void
sctk_net_ibv_allocator_send_coll_message (
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_collective_communications_t * com,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_ibuf_ptp_type_t type,
    uint32_t psn)
{
  DBG_S(1);
  sctk_net_ibv_allocator_request_t req;

  req.ptp_type = type;
  req.size = size;
  req.msg   = msg;
  req.dest_process  = dest_process;
  req.dest_task     = -1;
  req.psn = psn;


  /* copy the comm id if this is a message from a bcast or
   * a reduce */
  if (com)
  {
    req.com_id = com->id;
  } else {
    req.com_id = 0;
  }

  sctk_nodebug("Send collective message with size %lu", size);

  if ( (size + RC_SR_HEADER_SIZE) <= ibv_eager_limit) {
    sctk_ibv_profiler_inc(IBV_EAGER_NB);
    /*
     * EAGER MSG
     */
    req.channel = IBV_CHAN_RC_SR;
    sctk_net_ibv_comp_rc_sr_send_ptp_message (
        rc_sr_local, req);
  } else if ( (size  + RC_SR_HEADER_SIZE )<= ibv_frag_eager_limit) {
    sctk_ibv_profiler_inc(IBV_FRAG_EAGER_NB);
    sctk_ibv_profiler_add(IBV_FRAG_EAGER_SIZE, size + RC_SR_HEADER_SIZE);

    /*
     * FRAG MSG
     */
    req.channel = IBV_CHAN_RC_SR_FRAG;
    sctk_net_ibv_comp_rc_sr_send_coll_frag_ptp_message (rc_sr_local, req);
  }
  else
  {
    sctk_ibv_profiler_inc(IBV_RDVZ_READ_NB);

    /*
     * RENDEZVOUS
     */
    req.channel = IBV_CHAN_RC_RDMA;
    sctk_net_ibv_comp_rc_rdma_send_request (
        rail, rc_sr_local, req, 0);
  }

  DBG_E(1);
}
#endif
