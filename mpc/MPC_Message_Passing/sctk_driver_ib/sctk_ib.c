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
#include <netdb.h>
#include <sctk_spinlock.h>
#include <sctk_net_tools.h>
#include <sctk_route.h>
#include <sctk_ib.h>
#include <sctk_ib_qp.h>
#include <sctk_ib_cp.h>
#include <sctk_ib_toolkit.h>
#include <sctk_ib_rdma.h>
#include <sctk_ib_sr.h>
#include <sctk_ib_prof.h>
#include <sctk_route.h>
#include <sctk_ibufs_rdma.h>
#include <sctk_multirail_ib.h>

#define MAX_STRING_SIZE  2048


void sctk_ib_add_static_route(int dest, sctk_route_table_t *tmp, sctk_rail_info_t* rail){
  sctk_add_static_route(dest,tmp,rail);
}

void sctk_ib_add_dynamic_route(int dest, sctk_route_table_t *tmp, sctk_rail_info_t* rail){
  sctk_add_dynamic_route(dest,tmp,rail);
}

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
  sctk_debug("IB protocol: %s", sctk_ib_print_procotol(msg->tail.ib.protocol));
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

void sctk_network_init_ib_all(sctk_rail_info_t* rail,
			       int (*route)(int , sctk_rail_info_t* ),
			       void(*route_init)(sctk_rail_info_t*)){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int dest_rank;
  int src_rank;
  sctk_route_table_t *route_table_src, *route_table_dest;
  sctk_ib_data_t *route_dest, *route_src;
  sctk_ib_qp_keys_t keys;

  assume(rail->send_message_from_network != NULL);

  dest_rank = (sctk_process_rank + 1) % sctk_process_number;
  src_rank = (sctk_process_rank + sctk_process_number - 1) % sctk_process_number;

  if (sctk_process_number > 2)
  {
  /* XXX: Set QP in a Ready-To-Send mode. Ideally, we should check that
   * the remote QP has sent an ack */

    /* create remote for dest */
    route_table_dest = sctk_ib_create_remote();
    sctk_ib_init_remote(dest_rank, rail, route_table_dest, 0);
    route_dest=&route_table_dest->data.ib;
    /* create remote for src */
    route_table_src = sctk_ib_create_remote();
    sctk_ib_init_remote(src_rank, rail, route_table_src, 0);
    route_src=&route_table_src->data.ib;

    sctk_ib_qp_keys_send(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    /* change state to RTR */
    keys = sctk_ib_qp_keys_recv(route_dest->remote, src_rank);
    sctk_ib_qp_allocate_rtr(rail_ib, route_src->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_src->remote);
    sctk_ib_qp_keys_send(rail_ib, route_src->remote);
    sctk_pmi_barrier();

    keys = sctk_ib_qp_keys_recv(route_src->remote, dest_rank);
    sctk_ib_qp_allocate_rtr(rail_ib, route_dest->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    sctk_ib_add_static_route(dest_rank, route_table_dest, rail);
    sctk_ib_add_static_route(src_rank, route_table_src, rail);
  } else {
    /* create remote for dest */
    route_table_dest = sctk_ib_create_remote();
    sctk_ib_init_remote(dest_rank, rail, route_table_dest, 0);
    route_dest=&route_table_dest->data.ib;
    /* TODO: User may choose the number of ibufs */
    sctk_ibuf_rdma_pool_init(rail_ib, route_dest->remote, config->ibv_max_rdma_ibufs);

    sctk_ib_qp_keys_send(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    /* change state to RTR */
    keys = sctk_ib_qp_keys_recv(route_dest->remote, src_rank);
    /* TODO: for RDMA */
    sctk_ibuf_rdma_update_remote_addr(route_dest->remote, &keys);
    sctk_ib_qp_allocate_rtr(rail_ib, route_dest->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    sctk_ib_add_static_route(dest_rank, route_table_dest, rail);
  }

  sctk_nodebug("Recv from %d, send to %d", src_rank, dest_rank);
}

void sctk_network_stats_ib (struct MPC_Network_stats_s* stats) {
  sctk_ib_cp_task_t *task = NULL;
  int task_id;
  int thread_id;
  sctk_rail_info_t** rails = sctk_network_get_rails();
  sctk_ib_rail_info_t* r;

  sctk_get_thread_info (&task_id, &thread_id);
  task = sctk_ib_cp_get_task(task_id);
  stats->matched = CP_PROF_PRINT(task, matched);
  stats->not_matched = CP_PROF_PRINT(task, not_matched);

#if 0
  stats->poll_own = CP_PROF_PRINT(task, poll_own);
  stats->poll_own_failed = CP_PROF_PRINT(task, poll_own_failed);
  stats->poll_steals = CP_PROF_PRINT(task, poll_steals);
  stats->poll_steals_failed = CP_PROF_PRINT(task, poll_steals_failed);
  stats->poll_steal_same_node = CP_PROF_PRINT(task, poll_steal_same_node);
  stats->poll_steal_other_node = CP_PROF_PRINT(task, poll_steal_other_node);
#endif
  stats->poll_own = poll_own;
  stats->poll_own_failed = poll_own_failed;
  stats->poll_own_success = poll_own_success;
  stats->poll_steals = poll_steals;
  stats->poll_steals_failed = poll_steals_failed;
  stats->poll_steals_success = poll_steals_success;
  stats->poll_steal_same_node = poll_steal_same_node;
  stats->poll_steal_other_node = poll_steal_other_node;
  stats->call_to_polling = call_to_polling;
  stats->poll_cq = poll_cq;

  stats->time_steals = time_steals;
  stats->time_own = time_own;
  stats->time_own = time_own;
  stats->time_poll_cq = time_poll_cq;
  stats->time_ptp = time_ptp;
  stats->time_coll = time_coll;

  if (rails) {
    if (rails[0]) {
      r = &rails[0]->network.ib;

      stats->alloc_mem = PROF_LOAD(r, alloc_mem);
      stats->free_mem = PROF_LOAD(r, free_mem);
      stats->qp_created = PROF_LOAD(r, qp_created);

      stats->eager_nb = PROF_LOAD(r, eager_nb);
      stats->buffered_nb = PROF_LOAD(r, buffered_nb);
      stats->rdma_nb = PROF_LOAD(r, rdma_nb);
    }
  }
}


void sctk_network_deco_neighbors_ib () {
  sctk_rail_info_t ** rails;
  sctk_ib_rail_info_t* r;

  /* Select the first rail */
  rails = sctk_network_get_rails();
  r = &rails[0]->network.ib;
  sctk_ib_qp_select_victim(r);

}
#endif
