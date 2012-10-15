/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */
#ifdef MPC_USE_INFINIBAND
#include <sctk_debug.h>
#include <sctk_net_tools.h>
#include <sctk_ib.h>
#include <sctk_pmi.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sctk.h>
#include <sctk_asm.h>
#include <netdb.h>
#include <sctk_spinlock.h>
#include <sctk_net_tools.h>
#include <sctk_route.h>
#include <sctk_ib.h>
#include <sctk_ib_qp.h>
#include <sctk_ib_cp.h>
#include <sctk_ib_toolkit.h>
#include <sctk_ib_rdma.h>
#include <sctk_ib_eager.h>
#include <sctk_ib_prof.h>
#include <sctk_route.h>
#include <sctk_ibufs_rdma.h>
#include <sctk_multirail_ib.h>

#define MAX_STRING_SIZE  2048

/* Initialize a new route table */
void
sctk_ib_init_remote(int dest, sctk_rail_info_t* rail, struct sctk_route_table_s* route_table, int ondemand){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ib_data_t *route_ib;

  route_ib=&route_table->data.ib;

  sctk_nodebug("Initializing QP for dest %d", dest);
  sctk_ib_qp_allocate_init(rail_ib, dest, route_ib->remote, ondemand, route_table);
  return;
}

/* Create a new route table */
sctk_route_table_t *
sctk_ib_create_remote(){
  sctk_route_table_t* tmp;
  sctk_ib_data_t *route_ib;

  tmp = sctk_malloc(sizeof(sctk_route_table_t));
  memset(tmp,0,sizeof(sctk_route_table_t));

  route_ib=&tmp->data.ib;
  route_ib->remote = sctk_ib_qp_new();

  return tmp;
}

char *sctk_ib_print_procotol(sctk_ib_protocol_t p)
{
  switch (p) {
    case eager_protocol:
      return "eager_protocol";
    case rdma_protocol:
      return "rdma_protocol";
    case buffered_protocol:
      return "buffered_protocol";
    default: not_reachable();
  }
  return NULL;
}

void sctk_ib_print_msg(sctk_thread_ptp_message_t *msg) {
  sctk_error("IB protocol: %s", sctk_ib_print_procotol(msg->tail.ib.protocol));
  switch (msg->tail.ib.protocol) {
    case eager_protocol:
      break;
    case rdma_protocol:
      sctk_ib_rdma_print(msg);
      break;
    case buffered_protocol:
      break;
    default: not_reachable();
  }

}

void sctk_network_stats_ib (struct MPC_Network_stats_s* stats) {
#if 0
  sctk_ib_cp_task_t *task = NULL;
  int task_id;
  int thread_id;
  sctk_rail_info_t** rails = sctk_network_get_rails();
  sctk_rail_info_t* r;

  sctk_get_thread_info (&task_id, &thread_id);
  task = sctk_ib_cp_get_task(task_id);
  stats->int_t.matched = CP_PROF_PRINT(task, matched);
  stats->int_t.not_matched = CP_PROF_PRINT(task, not_matched);

  if (rails) {
    if (rails[0]) {
      r = rails[0];

      stats->long_t.poll_own              = PROF_LOAD(r, poll_own);
      stats->long_t.poll_own_failed       = PROF_LOAD(r, poll_own_failed);
      stats->long_t.poll_own_success      = PROF_LOAD(r, poll_own_success);
      stats->long_t.poll_steals           = PROF_LOAD(r,poll_steals);
      stats->long_t.poll_steals_failed    = PROF_LOAD(r,poll_steals_failed);
      stats->long_t.poll_steals_success   = PROF_LOAD(r,poll_steals_success);
      stats->long_t.poll_steal_same_node  = PROF_LOAD(r,poll_steal_same_node);
      stats->long_t.poll_steal_other_node = PROF_LOAD(r,poll_steal_other_node);
      stats->long_t.call_to_polling       = PROF_LOAD(r,call_to_polling);
      stats->long_t.poll_cq               = PROF_LOAD(r,poll_cq);

      stats->double_t.ibuf_release        = PROF_LOAD(r, ibuf_release);
      stats->double_t.resize_rdma         = PROF_LOAD(r, resize_rdma);
      stats->double_t.post_send           = PROF_LOAD(r, post_send);
      stats->double_t.time_steals         = PROF_LOAD(r, time_steals);
      stats->double_t.time_own            = PROF_LOAD(r, time_own);
      stats->double_t.time_own            = PROF_LOAD(r, time_own);
      stats->double_t.time_poll_cq        = PROF_LOAD(r, time_poll_cq);
      stats->double_t.time_ptp            = PROF_LOAD(r, time_ptp);
      stats->double_t.time_coll           = PROF_LOAD(r, time_coll);

      stats->int_t.alloc_mem              = PROF_LOAD(r, alloc_mem);
      stats->int_t.free_mem               = PROF_LOAD(r, free_mem);
      stats->int_t.qp_created             = PROF_LOAD(r, qp_created);

      stats->int_t.eager_nb               = PROF_LOAD(r, eager_nb);
      stats->int_t.buffered_nb            = PROF_LOAD(r, buffered_nb);
      stats->int_t.rdma_nb                = PROF_LOAD(r, rdma_nb);

      stats->int_t.rdma_connection        = PROF_LOAD(r, rdma_connection);
      stats->int_t.rdma_resizing          = PROF_LOAD(r, rdma_resizing);
      stats->int_t.rdma_deconnection      = PROF_LOAD(r, rdma_deconnection);

      stats->int_t.ibuf_sr_nb             = PROF_LOAD(r, ibuf_sr_nb);
      stats->int_t.ibuf_rdma_nb           = PROF_LOAD(r, ibuf_rdma_nb);
      stats->int_t.ibuf_rdma_miss_nb      = PROF_LOAD(r, ibuf_rdma_miss_nb);
    }
  }
#endif
}


void sctk_network_deco_neighbor_ib (int process) {
  sctk_rail_info_t ** rails;
  sctk_ib_rail_info_t* rail_ib;
;

  /* Select the first rail */
  rails = sctk_network_get_rails();
  rail_ib = &rails[0]->network.ib;

  /* get the route to process */
  sctk_route_table_t *route_table = sctk_get_route_to_process_no_route(process, rails[0]);
  ib_assume(route_table);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);

  sctk_debug("Rank: %d -> %d", remote->rank, process);

  sctk_spinlock_lock(&remote->rdma.flushing_lock);
  sctk_route_state_t ret =
    sctk_ibuf_rdma_cas_remote_state_rts(remote, state_connected, state_flushing);

  /* If we are allowed to deconnect */
  if (ret == state_connected) {
    /* Update the slots values requested to 0 -> means that we want to disconnect */
    remote->rdma.pool->resizing_request.send_keys.nb   = 0;
    remote->rdma.pool->resizing_request.send_keys.size = 0;
    remote->rdma.creation_timestamp = sctk_get_time_stamp();
    sctk_spinlock_unlock(&remote->rdma.flushing_lock);

    int busy_nb = OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]);
    sctk_debug("DECONNECTING the RMDA buffer for remote %d busy: %d", remote->rank, busy_nb);

    sctk_ibuf_rdma_check_flush_send(rail_ib, remote);
  } else {
    sctk_debug("DECONNECTING ABORTED %d", remote->rank);
    sctk_spinlock_unlock(&remote->rdma.flushing_lock);
  }
}
#endif
