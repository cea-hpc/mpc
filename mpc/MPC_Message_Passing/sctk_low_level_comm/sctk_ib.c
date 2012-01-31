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
#include <sctk_route.h>

#define MAX_STRING_SIZE  2048


void sctk_ib_add_static_route(int dest, sctk_route_table_t *tmp, sctk_rail_info_t* rail){
  sctk_add_static_route(dest,tmp,rail);
}

void sctk_ib_add_dynamic_route(int dest, sctk_route_table_t *tmp, sctk_rail_info_t* rail){
  sctk_add_dynamic_route(dest,tmp,rail);
}

void sctk_ib_route_dynamic_set_connected(sctk_route_table_t *tmp, int connected){
  sctk_route_set_connected(tmp, connected);
}

int sctk_ib_route_dynamic_is_connected(sctk_route_table_t *tmp){
  return sctk_route_is_connected(tmp);
}

sctk_route_table_t *
sctk_ib_create_remote(int dest, sctk_rail_info_t* rail){
  sctk_route_table_t* tmp;
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ib_data_t *route_ib;

  tmp = sctk_malloc(sizeof(sctk_route_table_t));
  memset(tmp,0,sizeof(sctk_route_table_t));

  sctk_nodebug("Creating QP for dest %d", dest);
  route_ib=&tmp->data.ib;
  route_ib->remote = sctk_ib_qp_new();
  sctk_ib_qp_allocate_init(rail_ib, dest, route_ib->remote);

  return tmp;
}

void sctk_network_free_msg(sctk_thread_ptp_message_t *msg)
{
#if 0
  sctk_ib_rail_info_t *rail_ib = &rail_0->network.ib;

  switch(msg->tail.ib_protocol) {
    case eager_protocol:
      sctk_ib_sr_free_msg(rail_ib, msg);
      break;
    default: assume(0);
  }
#endif
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
    route_table_dest = sctk_ib_create_remote(dest_rank, rail);
    route_dest=&route_table_dest->data.ib;
    /* create remote for src */
    route_table_src = sctk_ib_create_remote(src_rank, rail);
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
    route_table_dest = sctk_ib_create_remote(dest_rank, rail);
    route_dest=&route_table_dest->data.ib;

    sctk_ib_qp_keys_send(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    /* change state to RTR */
    keys = sctk_ib_qp_keys_recv(route_dest->remote, src_rank);
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

  sctk_get_thread_info (&task_id, &thread_id);
  task = sctk_ib_cp_get_task(task_id);
  stats->matched = CP_PROF_PRINT(task, matched);
  stats->not_matched = CP_PROF_PRINT(task, not_matched);
  stats->poll_own = CP_PROF_PRINT(task, poll_own);
  stats->poll_stolen = CP_PROF_PRINT(task, poll_stolen);
  stats->poll_steals = CP_PROF_PRINT(task, poll_steals);
  stats->time_stolen = task->time_stolen;
  stats->time_steals = task->time_steals;
  stats->time_own = task->time_own;
}
#endif
