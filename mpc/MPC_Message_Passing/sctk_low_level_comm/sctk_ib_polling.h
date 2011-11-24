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
#ifndef __SCTK__IB_POLLING_H_
#define __SCTK__IB_POLLING_H_

#include <infiniband/verbs.h>
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "sctk_pmi.h"
#include "utlist.h"

int sctk_ib_cq_poll(struct sctk_rail_info_s* rail,
    struct ibv_cq *cq, const int poll_nb, int (*ptr_func)(struct sctk_rail_info_s* rail, struct ibv_wc*));

void
sctk_ib_polling_check_wc(struct sctk_ib_rail_info_s* rail_ib,
    struct ibv_wc wc);

#if 0
/*-----------------------------------------------------------
 *  PROCESS
 *----------------------------------------------------------*/
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
  size_t total_copied;

  /* for collective */
  uint8_t com_id;
  uint32_t psn;
#ifdef SCTK_USE_ADLER
  uint32_t crc_full_payload;
#endif

  int buff_nb;
  int total_buffs;

  struct sctk_net_ibv_allocator_task_s *task;
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

/* pending wc */
struct pending_wc_s
{
  struct ibv_wc wc;
  struct sctk_list_header list_header;
  struct OPA_Queue_element_hdr_t header;
};

/* local tasks */
typedef struct sctk_net_ibv_allocator_task_s
{
  /* ID of the task */
  int task_nb;
  /* vp where the task is located */
  int vp;
  /* node number */
  int node;
  /* task in polling */
  int in_polling;
  /* nb_pending msg*/
  int nb_pending;
  int max_pending;

  /* scheduler */
  sctk_spinlock_t sched_lock;
  sched_sn_t* remote;

  /* one pending list for each local task */
  struct sctk_list_header pending_msg;
  int pending_msg_nb;

  struct sctk_list_header** frag_eager_list;
  sctk_spinlock_t lock;

  /* TODO: new implementation from OPA queues */
  OPA_Queue_info_t* recv_wc;
  sctk_spinlock_t recv_lock;

  /* TODO: utash header */
  UT_hash_handle hh;
} sctk_net_ibv_allocator_task_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

#endif
#endif
#endif
