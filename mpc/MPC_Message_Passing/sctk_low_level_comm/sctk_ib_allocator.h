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
#ifndef __SCTK__INFINIBAND_ALLOCATOR_H_
#define __SCTK__INFINIBAND_ALLOCATOR_H_

#include "sctk_list.h"
#include "sctk_launch.h"
#include "sctk_ib_allocator.h"
#include "sctk_ib_scheduling.h"
#include "sctk_ib_comp_rc_rdma.h"
#include "sctk_ib_comp_rc_sr.h"
#include "sctk_ib_qp.h"
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

typedef struct sctk_net_ibv_allocator_request_s
{
  sctk_net_ibv_ibuf_type_t      ibuf_type;
  sctk_net_ibv_ibuf_ptp_type_t  ptp_type;
  sctk_net_ibv_allocator_type_t channel;

  int dest_process;
  int dest_task;
  void* msg;
  void* msg_header;
  size_t size;
  size_t payload_size;

  /* for collective */
  uint8_t com_id;
  uint32_t psn;

  int buff_nb;
  int total_buffs;
} sctk_net_ibv_allocator_request_t;


UNUSED static void sctk_net_ibv_allocator_request_show(sctk_net_ibv_allocator_request_t* h)
{
  sctk_nodebug("#### Request ####"
      "ibuf_type : %d\n"
      "ptp_type : %d\n"
      "dest_process : %d\n"
      "dest_task : %d\n"
      "size : %lu\n"
      "psn : %lu\n"
      "payload_size : %lu\n"
      "buff_nb : %d\n"
      "total_buffs : %d\n"
      " com_id : %d\n",
      h->ibuf_type, h->ptp_type, h->dest_process,
      h->dest_task, h->size, h->psn, h->payload_size,
      h->buff_nb, h->total_buffs, h->com_id);
}

/* lookup for a remote thread entry. The only requirement is to know the
 * process where the task is located. Once, we check in the allocator list.
 * The task entry is created if it doesn't exist*/
extern sctk_net_ibv_allocator_t* sctk_net_ibv_allocator;



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

  sched_sn_t* remote;
  /* one pending list for each local task */
  struct sctk_list pending_msg;
  struct sctk_list** frag_eager_list;
} sctk_net_ibv_allocator_task_t;



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
sctk_net_ibv_allocator_send_coll_message (
    sctk_net_ibv_qp_rail_t   *rail,
    sctk_net_ibv_qp_local_t* local_rc_sr,
    sctk_collective_communications_t * com,
    void *msg,
    int dest_process,
    size_t size,
    sctk_net_ibv_ibuf_ptp_type_t type,
    uint32_t);

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

char*
sctk_net_ibv_tcp_request(int process, sctk_net_ibv_qp_remote_t *remote, char* host, int* port);

int sctk_net_ibv_tcp_client(char* host, int port, int dest, sctk_net_ibv_qp_remote_t *remote);

void sctk_net_ibv_tcp_server();

void
sctk_net_ibv_allocator_initialize_threads();

#endif
#endif
