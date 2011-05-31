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
#ifndef __SCTK__INFINIBAND_SCHEDULING_H_
#define __SCTK__INFINIBAND_SCHEDULING_H_

#include <sctk.h>
#include <stdint.h>
#include "sctk_ib_const.h"
#include "sctk_ib_list.h"
#include "sctk_ib_comp_rc_sr.h"
#include "sctk_ib_comp_rc_rdma.h"

#define SCHED_LOCK {                \
  sctk_thread_mutex_lock(&lock);}   \

#define SCHED_UNLOCK {              \
  sctk_thread_mutex_unlock(&lock); }\

typedef struct {
    uint32_t                esn;    /* expected sequence number */
    uint32_t                psn;    /* packet sequence number */
} sched_sn_t;

/* TODO: change it */
#define MAX_NB_TASKS_PER_PROCESS 32
typedef struct sctk_net_ibv_sched_entry_s
{
  int                     task_nb;/* task number */

  sched_sn_t eager;
} sctk_net_ibv_sched_entry_t;

#define INIT_SCHED_ENTRY { -1, 0, 0 }

/*
 * Header of pending msg
 **/
typedef struct
{
  /* type of msg pending */
  sctk_net_ibv_allocator_type_t type;

  uint64_t src_process;
  uint64_t src_task;
  uint64_t dest_task;
  uint32_t psn;
  sctk_thread_ptp_message_t* msg_header;
  void* msg_payload;
  void* struct_ptr;
  struct sctk_list_header list_header;
} sctk_net_ibv_sched_pending_header;


void sctk_net_ibv_sched_init();

/*
 * Sequence number for PTP
 */
uint32_t
sctk_net_ibv_sched_psn_inc (int dest, int task_nb);
uint32_t
sctk_net_ibv_sched_esn_inc (int dest, int task_nb);
  uint32_t
sctk_net_ibv_sched_get_esn(int dest, int task_nb);
  uint32_t
sctk_net_ibv_sched_sn_check(int dest, int task_nb, uint64_t num);
int
sctk_net_ibv_sched_sn_check_and_inc(int dest, int task_nb, uint64_t num);

/*
 * Sequence number for COLL
 */
uint32_t
sctk_net_ibv_sched_coll_psn_inc (sctk_net_ibv_ibuf_ptp_type_t type, int com_id);
uint32_t
sctk_net_ibv_sched_coll_esn_inc (int com_id);
uint32_t
sctk_net_ibv_sched_coll_get_esn(int com_id);
uint32_t
sctk_net_ibv_sched_coll_sn_check(int com_id);
int
sctk_net_ibv_sched_coll_sn_check_and_inc(int com_id);

/*
 * Pending messages
 */
int
  sctk_net_ibv_sched_rc_sr_free_pending_msg(sctk_thread_ptp_message_t * item );

void
sctk_net_ibv_sched_pending_init(
    sctk_net_ibv_allocator_type_t type, int entry_nb);

void
sctk_net_ibv_sched_pending_push(
    void* ptr,
    size_t size,
    int allocation_needed,
    sctk_net_ibv_allocator_type_t type,
    int src_process,
    uint64_t src_task,
    uint64_t dest_task,
    uint32_t psn,
    sctk_thread_ptp_message_t* msg_header,
    void* msg_payload,
    void* struct_ptr);

  int
sctk_net_ibv_sched_poll_pending_msg(int task_nb);

void
sctk_net_ibv_sched_initialize_threads();

  void
sctk_net_ibv_sched_coll_reset (int com_id);

#endif
#endif
