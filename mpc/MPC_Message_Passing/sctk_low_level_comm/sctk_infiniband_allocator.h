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

#ifndef __SCTK__INFINIBAND_ALLOCATOR_H_
#define __SCTK__INFINIBAND_ALLOCATOR_H_

#include "sctk_list.h"
#include "sctk_launch.h"
#include "sctk_infiniband_allocator.h"
#include "sctk_infiniband_scheduling.h"
#include "sctk_infiniband_comp_rc_rdma.h"
#include "sctk_infiniband_comp_rc_sr.h"
#include "sctk_infiniband_qp.h"
#include "sctk.h"

sctk_net_ibv_qp_rail_t   *rail;

/* RC SR structures */
sctk_net_ibv_qp_local_t *rc_sr_local;

/* RC RDMA structures */
sctk_net_ibv_qp_local_t   *rc_rdma_local;


/*-----------------------------------------------------------
 *  PROCESS
 *----------------------------------------------------------*/

typedef struct
{
  sctk_net_ibv_rc_sr_process_t        *rc_sr;
  sctk_net_ibv_rc_rdma_process_t      *rc_rdma;

  /* sequence numbers */
  sctk_net_ibv_sched_entry_t          *sched;
  sctk_spinlock_t sched_lock;

  /* for debug */
  uint32_t nb_ptp_msg_transfered;
  uint32_t nb_ptp_msg_received;

  /* locks */
  sctk_thread_mutex_t rc_sr_lock;
  sctk_thread_mutex_t rc_rdma_lock;

} sctk_net_ibv_allocator_entry_t;

typedef struct
{
  sctk_net_ibv_allocator_entry_t* entry;
} sctk_net_ibv_allocator_t;

/*-----------------------------------------------------------
 *  TASKS
 *----------------------------------------------------------*/
/* lookup for a local thread id */
#define LOOKUP_LOCAL_THREAD_ENTRY(id, entry_nb) { \
  int i;\
  for (i=0; i<MAX_NB_TASKS_PER_PROCESS && all_tasks[i].task_nb != -1; ++i) { \
    sctk_nodebug("Search entry : %d", all_tasks[i].task_nb); \
    if (all_tasks[i].task_nb == id) {*entry_nb = i; break;} \
  }; if(*entry_nb == -1) {\
    sctk_nodebug("Thread entry %d not found !", id); abort();};} \

/* lookup for a remote thread entry. The only requirement is to know the
 * process where the task is located. Once, we check in the allocator list.
 * The task entry is created if it doesn't exist*/
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;

  static inline int
LOOK_AND_CREATE_REMOTE_ENTRY(int process, int task)
{
  int i;
  int rc = -1;

  sctk_spinlock_lock(&sctk_net_ibv_allocator->entry[process].sched_lock);

  if (sctk_net_ibv_allocator->entry[process].sched == NULL) {
    sctk_net_ibv_allocator->entry[process].sched =
      sctk_malloc(sctk_get_total_tasks_number() * sizeof(sctk_net_ibv_sched_entry_t));

    sctk_nodebug("Number of tasks : %d", sctk_get_total_tasks_number());
    for (i=0; i < sctk_get_total_tasks_number(); ++i) {
      sctk_net_ibv_allocator->entry[process].sched[i].task_nb = -1;
      sctk_net_ibv_allocator->entry[process].sched[i].esn = 0;
      sctk_net_ibv_allocator->entry[process].sched[i].psn = 0;
    }
  }
  sctk_nodebug("Number of tasks : %d", sctk_get_total_tasks_number());
  sctk_nodebug("process %d, task %d", process, task);

  for (i=0; i < sctk_get_total_tasks_number(); ++i)
  {
    sctk_nodebug("[%d->%d]Search (process %d, task %d) - found %d", i, sctk_get_total_tasks_number(), process, task, sctk_net_ibv_allocator->entry[process].sched[i].task_nb);
    if (sctk_net_ibv_allocator->entry[process].sched[i].task_nb == task)
    {
      rc = i;
      sctk_nodebug("New task %d FOUND for process %d (entry %d)", task, process, i);
      break;
    }

    if (sctk_net_ibv_allocator->entry[process].sched[i].task_nb == -1)
    {
      rc = i;
      sctk_net_ibv_allocator->entry[process].sched[i].task_nb = task;
      sctk_nodebug("New task %d created for process %d (entry %d)", task, process, i);
      break;
    }
  }
  sctk_spinlock_unlock(&sctk_net_ibv_allocator->entry[process].sched_lock);
  assume(rc != -1);

  return rc;
}



#define ALLOCATOR_LOCK(rank, type)                                                  \
{                                                                                   \
  switch(type) {                                                                   \
    case IBV_CHAN_RC_SR:                                                            \
                                                                                    sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);      \
    break;                                                                        \
    case IBV_CHAN_RC_RDMA:                                                          \
                                                                                    sctk_thread_mutex_lock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);    \
    break;                                                                        \
    default: assume(0); break;                                                      \
  }                                                                                 \
}                                                                                   \

#define ALLOCATOR_UNLOCK(rank, type)                                                \
{                                                                                   \
  switch(type) {                                                                   \
    case IBV_CHAN_RC_SR:                                                            \
                                                                                    sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_sr_lock);    \
    break;                                                                        \
    case IBV_CHAN_RC_RDMA:                                                          \
                                                                                    sctk_thread_mutex_unlock(&sctk_net_ibv_allocator->entry[rank].rc_rdma_lock);  \
    break;                                                                        \
    default: assume(0); break;                                                      \
  }                                                                                 \
}                                                                                   \

/* local tasks */
typedef struct
{
  /* local task nb */
  int task_nb;

  /* one pending list for each local task */
  struct sctk_list pending_msg;

} sctk_net_ibv_allocator_task_t;


sctk_net_ibv_allocator_task_t all_tasks[MAX_NB_TASKS_PER_PROCESS];
static sctk_spinlock_t all_tasks_lock = SCTK_SPINLOCK_INITIALIZER;


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

sctk_net_ibv_allocator_t *sctk_net_ibv_allocator_new();

void
sctk_net_ibv_allocator_register(
    unsigned int rank,
    void* entry, sctk_net_ibv_allocator_type_t type);

void*
sctk_net_ibv_allocator_get(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void
sctk_net_ibv_allocator_rc_rdma_process_next_request(
    sctk_net_ibv_rc_rdma_process_t *entry_rc_rdma);

void
sctk_net_ibv_allocator_send_ptp_message ( sctk_thread_ptp_message_t * msg,
    int dest_process );

void
sctk_net_ibv_allocator_send_coll_message(
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_ibuf_msg_type_t type);

/*-----------------------------------------------------------
 *  SEARCH FUNCTIONS
 *----------------------------------------------------------*/

sctk_net_ibv_rc_sr_process_t*
sctk_net_ibv_alloc_rc_sr_find_from_qp_num(uint32_t qp_num);

sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_alloc_rc_rdma_find_from_qp_num(uint32_t qp_num);

sctk_net_ibv_rc_rdma_process_t*
sctk_net_ibv_alloc_rc_rdma_find_from_rank(int rank);

/*-----------------------------------------------------------
 *  POLLING RC SR
 *----------------------------------------------------------*/
void sctk_net_ibv_allocator_ptp_poll_all();

void sctk_net_ibv_allocator_ptp_lookup_all(int dest);

void
sctk_net_ibv_allocator_unlock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void
sctk_net_ibv_allocator_lock(
    unsigned int rank,
    sctk_net_ibv_allocator_type_t type);

void sctk_net_ibv_rc_sr_send_cq(struct ibv_wc* wc, int lookup, int dest);

char*
sctk_net_ibv_tcp_request(int process, sctk_net_ibv_qp_remote_t *remote, char* host, int* port);

int sctk_net_ibv_tcp_client(char* host, int port, int dest, sctk_net_ibv_qp_remote_t *remote);

void sctk_net_ibv_tcp_server();

void
sctk_net_ibv_allocator_initialize_threads();

#endif
