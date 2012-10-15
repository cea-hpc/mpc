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
#include "sctk_ib.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_prof.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_route.h"
#include "sctk_asm.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "CM"
#include "sctk_ib_toolkit.h"

/*-----------------------------------------------------------
 *  Interface for Infiniband connexions.
 *
 *  - RACE CONDITITONS: for avoind race conditions between peers, a
 *  process responds to a request with a positive ack only when
 *  it has not initiated a request or its rank is higher than the
 *  source rank
 *----------------------------------------------------------*/

struct sctk_ib_qp_s;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/* Change the state of a remote process */
static void sctk_ib_cm_change_state(sctk_rail_info_t* rail,
    sctk_route_table_t *route_table, sctk_route_state_t state) {

  sctk_route_state_t state_before_connexion;
  state_before_connexion = sctk_route_get_state(route_table);

  switch ( sctk_route_get_origin(route_table) ) {

    case route_origin_static: /* Static route */
      ROUTE_LOCK(route_table);
      sctk_route_set_state(route_table, state_connected);
      ROUTE_UNLOCK(route_table);

      sctk_ib_nodebug("[%d] Static QP connected to process %d", rail->rail_number,remote->rank);
      break;

    case route_origin_dynamic: /* Dynamic route */
      ROUTE_LOCK(route_table);
      sctk_route_set_state(route_table, state_connected);
      ROUTE_UNLOCK(route_table);

      if (state_before_connexion == state_reconnecting)
        sctk_ib_nodebug("[%d] OD QP reconnected to process %d", rail->rail_number,remote->rank);
      else
        sctk_ib_nodebug("[%d] OD QP connected to process %d", rail->rail_number,remote->rank);

      /* Only reccord dynamically created QPs */
      sctk_ib_prof_qp_write(remote->rank, 0,
        sctk_get_time_stamp(), PROF_QP_CREAT);

      break;

    default: not_reachable(); sctk_abort(); /* Not reachable */
  }
}

/* Change the state of a remote process */
static void sctk_ib_cm_change_state_to_rtr(sctk_rail_info_t* rail,
    sctk_route_table_t *route_table, enum sctk_ib_cm_change_state_type_e type) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib)
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  sctk_route_state_t state;
  char txt[256];

  if (type == CONNECTION) {
      state = sctk_ibuf_rdma_cas_remote_state_rtr(remote, state_connecting, state_connected);
      sprintf(txt, SCTK_COLOR_GREEN(RTR CONNECTED));
      assume(state == state_connecting);
      sctk_ib_prof_qp_write(remote->rank, 0, sctk_get_time_stamp(), PROF_QP_RDMA_CONNECTED);
  } else if (type == RESIZING) {
    if (remote->rdma.pool->resizing_request.recv_keys.nb == 0) {  /* We deconnect */
      /* Change the state of the route */
      state = sctk_ibuf_rdma_cas_remote_state_rtr(remote, state_flushed, state_deconnected);
      sprintf(txt, SCTK_COLOR_RED(RTR DECONNECTED));
      sctk_ib_prof_qp_write(remote->rank, 0, sctk_get_time_stamp(), PROF_QP_RDMA_DISCONNECTED);
    } else { /* We connect */
      state = sctk_ibuf_rdma_cas_remote_state_rtr(remote, state_flushed, state_connected);
      sprintf(txt, SCTK_COLOR_BLUE(RTR RESIZED));
      sctk_ib_prof_qp_write(remote->rank, 0, sctk_get_time_stamp(), PROF_QP_RDMA_RESIZING);
    }
    assume(state == state_flushed);
  }else not_reachable();

  sctk_ib_debug("%s with process %d (nb:%d->%d, size:%d->%d rdma_connections:%d allocated:%.0fko)", txt,
      remote->rank,
      remote->rdma.pool->region[REGION_RECV].nb_previous,
      remote->rdma.pool->region[REGION_RECV].nb,
      remote->rdma.pool->region[REGION_RECV].size_ibufs_previous,
      remote->rdma.pool->region[REGION_RECV].size_ibufs,
      device->eager_rdma_connections,
      (float) sctk_ibuf_rdma_get_regions_get_allocate_size(remote) / 1024.0);
}

/* Change the state of a remote process */
static void sctk_ib_cm_change_state_to_rts(sctk_rail_info_t* rail,
    sctk_route_table_t *route_table, enum sctk_ib_cm_change_state_type_e type) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib)
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  sctk_route_state_t state;
  char txt[256];

  if (type == CONNECTION) {
    state = sctk_ibuf_rdma_cas_remote_state_rts(remote, state_connecting, state_connected);
    sprintf(txt, SCTK_COLOR_GREEN(RTS CONNECTED));
    assume(state == state_connecting);
    sctk_ib_prof_qp_write(remote->rank, 0, sctk_get_time_stamp(), PROF_QP_RDMA_CONNECTED);
  } else if (type == RESIZING) {
    if (remote->rdma.pool->resizing_request.send_keys.nb == 0) {  /* We deconnect */
      state = sctk_ibuf_rdma_cas_remote_state_rts(remote, state_flushed, state_deconnected);
      sprintf(txt, SCTK_COLOR_RED(RTS DISCONNECTED));
      sctk_ib_prof_qp_write(remote->rank, 0, sctk_get_time_stamp(), PROF_QP_RDMA_DISCONNECTED);
    } else { /* We connect */
      state = sctk_ibuf_rdma_cas_remote_state_rts(remote, state_flushed, state_connected);
      sprintf(txt, SCTK_COLOR_BLUE(RTS RESIZED));
      sctk_ib_prof_qp_write(remote->rank, 0, sctk_get_time_stamp(), PROF_QP_RDMA_RESIZING);
    }
    assume(state == state_flushed);
  }else not_reachable();

  /* Change the state of the route */
  sctk_ib_debug("%s with process %d (nb:%d->%d, size:%d->%d, rdma_connections:%d  allocated:%0.fko ts:%f)", txt,
      remote->rank,
      remote->rdma.pool->region[REGION_SEND].nb_previous,
      remote->rdma.pool->region[REGION_SEND].nb,
      remote->rdma.pool->region[REGION_SEND].size_ibufs_previous,
      remote->rdma.pool->region[REGION_SEND].size_ibufs,
      device->eager_rdma_connections,
      (float) sctk_ibuf_rdma_get_regions_get_allocate_size(remote) / 1024,
      sctk_get_time_stamp() - remote->rdma.creation_timestamp);

}

/*-----------------------------------------------------------
 *  STATIC CONNEXIONS : intialization to a ring topology
 *  Messages are exchange in a string format. This is because
 *  we are using the PMI library.
 *----------------------------------------------------------*/
void sctk_ib_cm_connect_ring (sctk_rail_info_t* rail,
			       int (*route)(int , sctk_rail_info_t* ),
			       void(*route_init)(sctk_rail_info_t*)){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  int dest_rank;
  int src_rank;
  sctk_route_table_t *route_table_src, *route_table_dest;
  sctk_ib_data_t *route_dest, *route_src;
  sctk_ib_cm_qp_connection_t keys;

  ib_assume(rail->send_message_from_network != NULL);

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
    keys = sctk_ib_qp_keys_recv(rail_ib, route_dest->remote, src_rank);
    sctk_ib_qp_allocate_rtr(rail_ib, route_src->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_src->remote);
    sctk_ib_qp_keys_send(rail_ib, route_src->remote);
    sctk_pmi_barrier();

    keys = sctk_ib_qp_keys_recv(rail_ib, route_src->remote, dest_rank);
    sctk_ib_qp_allocate_rtr(rail_ib, route_dest->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    sctk_add_static_route(dest_rank, route_table_dest, rail);
    sctk_add_static_route(src_rank, route_table_src, rail);

    /* Change to connected */
    sctk_ib_cm_change_state(rail, route_table_dest, state_connected);
    sctk_ib_cm_change_state(rail, route_table_src, state_connected);
  } else {
    sctk_nodebug("Send msg to rail %d", rail->rail_number);
    /* create remote for dest */
    route_table_dest = sctk_ib_create_remote();
    sctk_ib_init_remote(dest_rank, rail, route_table_dest, 0);
    route_dest=&route_table_dest->data.ib;

    sctk_ib_qp_keys_send(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    /* change state to RTR */
    keys = sctk_ib_qp_keys_recv(rail_ib, route_dest->remote, src_rank);
    sctk_ib_qp_allocate_rtr(rail_ib, route_dest->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    sctk_add_static_route(dest_rank, route_table_dest, rail);

    /* Change to connected */
    sctk_ib_cm_change_state(rail, route_table_dest, state_connected);
  }

  sctk_nodebug("Recv from %d, send to %d", src_rank, dest_rank);
}


/*-----------------------------------------------------------
 *  STATIC CONNEXIONS : process to process connexions during intialization
 *  time
 *  Messages are sent using raw data (not like ring where messages are converted into
 *  string).
 *----------------------------------------------------------*/
void sctk_ib_cm_connect_to(int from, int to,sctk_rail_info_t* rail){
  /* We assume that the node we want to connect to is not the current  */
  assume(from != to);
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  struct sctk_ib_qp_s *remote;
  sctk_ib_cm_qp_connection_t send_keys;
  sctk_ib_cm_qp_connection_t recv_keys;
  sctk_ib_cm_done_t done;
  sctk_nodebug("Connection TO from %d to %d", from, to);

  /* create remote for dest */
  route_table = sctk_ib_create_remote();
  sctk_ib_init_remote(from, rail, route_table, 0);
  remote = route_table->data.ib.remote;
  sctk_ib_nodebug("[%d] QP connection request to process %d", rail->rail_number, remote->rank);

  sctk_route_messages_recv(from,to,ondemand_specific_message_tag, CM_OD_STATIC_TAG+CM_MASK_TAG,&recv_keys, sizeof(sctk_ib_cm_qp_connection_t));
  sctk_ib_qp_allocate_rtr(rail_ib, remote, &recv_keys);

  sctk_ib_qp_key_fill(&send_keys, remote, device->port_attr.lid,
      remote->qp->qp_num, remote->psn);

  sctk_route_messages_send(to,from,ondemand_specific_message_tag, CM_OD_STATIC_TAG+CM_MASK_TAG,&send_keys,sizeof(sctk_ib_cm_qp_connection_t));
  sctk_route_messages_recv(from,to,ondemand_specific_message_tag, CM_OD_STATIC_TAG+CM_MASK_TAG,&done,
      sizeof(sctk_ib_cm_done_t));
  sctk_ib_qp_allocate_rts(rail_ib, remote);
  /* Add route */
  sctk_add_static_route(from, route_table, rail);
  /* Change to connected */
  sctk_ib_cm_change_state(rail, route_table, state_connected);
}

void sctk_ib_cm_connect_from(int from, int to,sctk_rail_info_t* rail){
  /* We assume that the node we want to connect to is not the current  */
  assume(from != to);
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  struct sctk_ib_qp_s *remote;
  sctk_ib_cm_qp_connection_t send_keys;
  sctk_ib_cm_qp_connection_t recv_keys;
  /* Message to exchange to the peer */
  sctk_ib_cm_done_t done = {
    .done = 1,
  };
  sctk_nodebug("Connection FROM from %d to %d", from, to);

  /* create remote for dest */
  route_table = sctk_ib_create_remote();
  sctk_ib_init_remote(to, rail, route_table, 0);
  remote = route_table->data.ib.remote;
  sctk_ib_nodebug("[%d] QP connection  request to process %d", rail->rail_number, remote->rank);

  assume(remote->qp);
  sctk_ib_qp_key_fill(&send_keys, remote, device->port_attr.lid,
      remote->qp->qp_num, remote->psn);

  sctk_route_messages_send(from,to,ondemand_specific_message_tag, CM_OD_STATIC_TAG+CM_MASK_TAG,&send_keys, sizeof(sctk_ib_cm_qp_connection_t));
  sctk_route_messages_recv(to,from,ondemand_specific_message_tag, CM_OD_STATIC_TAG+CM_MASK_TAG,&recv_keys, sizeof(sctk_ib_cm_qp_connection_t));
  sctk_ib_qp_allocate_rtr(rail_ib, remote, &recv_keys);
  sctk_ib_qp_allocate_rts(rail_ib, remote);
  sctk_nodebug("FROM: Ready to send to %d", to);

  sctk_route_messages_send(from,to,ondemand_specific_message_tag, CM_OD_STATIC_TAG+CM_MASK_TAG,&done,
      sizeof(sctk_ib_cm_done_t));
  /* Add route */
  sctk_add_static_route(to, route_table, rail);
  /* Change to connected */
  sctk_ib_cm_change_state(rail, route_table, state_connected);
}

/* Helper macro: I'm bored to write always the same lines :-) */
#define RAIL_ARGS sctk_rail_info_t *rail_targ, sctk_rail_info_t *rail_sign
#define LOAD_SIGN() sctk_ib_rail_info_t *rail_ib_sign = &rail_sign->network.ib
#define LOAD_TARG() sctk_ib_rail_info_t *rail_ib_targ = &rail_targ->network.ib

/*-----------------------------------------------------------
 *  ON DEMAND CONNEXIONS: process to process connexions during run time.
 *  Messages are sent using raw data (not like ring where messages are converted into
 *  string).
 *----------------------------------------------------------*/
static inline void sctk_ib_cm_on_demand_recv_done(RAIL_ARGS, void* done, int src) {
  LOAD_TARG();

  sctk_route_table_t *route_table = sctk_route_dynamic_search(src, rail_targ);
  ib_assume(route_table);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);

  ib_assume(sctk_ib_qp_allocate_get_rts(remote) == 0);
  sctk_ib_qp_allocate_rts(rail_ib_targ, remote);

  /* Change to connected */
  sctk_ib_cm_change_state(rail_targ, route_table, state_connected);
}

static inline void sctk_ib_cm_on_demand_recv_ack(RAIL_ARGS, void* ack, int src) {
  LOAD_TARG();

  sctk_ib_cm_qp_connection_t recv_keys;
  sctk_ib_cm_done_t done = {
    .done = 1,
  };

  sctk_ib_nodebug("OD QP connexion ACK received from process %d %s", src, ack);
  sctk_route_table_t *route_table = sctk_route_dynamic_search(src, rail_targ);
  ib_assume(route_table);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);
  memcpy(&recv_keys, ack, sizeof(sctk_ib_cm_qp_connection_t));

  ib_assume (sctk_ib_qp_allocate_get_rtr(remote) == 0);
  sctk_ib_qp_allocate_rtr(rail_ib_targ, remote, &recv_keys);

  ib_assume (sctk_ib_qp_allocate_get_rts(remote) == 0);
  sctk_ib_qp_allocate_rts(rail_ib_targ, remote);

  /* SEND MESSAGE */
  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag,
      CM_OD_DONE_TAG+CM_MASK_TAG, &done,sizeof(sctk_ib_cm_done_t));

  /* Change to connected */
  sctk_ib_cm_change_state(rail_targ, route_table, state_connected);
}

int sctk_ib_cm_on_demand_recv_request(RAIL_ARGS, void* request, int src) {
  LOAD_TARG();
  LOAD_DEVICE(rail_ib_targ);
  sctk_ib_cm_qp_connection_t send_keys;
  sctk_ib_cm_qp_connection_t recv_keys;
  int added;

  /* create remote for source */
  sctk_route_table_t *route_table = sctk_route_dynamic_safe_add(src, rail_targ, sctk_ib_create_remote, sctk_ib_init_remote, &added, 0);
  ib_assume(route_table);

  ROUTE_LOCK(route_table);
  if (sctk_route_get_state(route_table) == state_deconnected) {
    sctk_route_set_state(route_table, state_connecting);
  }
  ROUTE_UNLOCK(route_table);

  /* RACE CONDITION AVOIDING -> positive ACK */
  if ( sctk_route_get_is_initiator(route_table) == 0 || sctk_process_rank > src ) {
    struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
    sctk_ib_debug("[%d] OD QP connexion request to process %d (initiator:%d)",
        rail_sign->rail_number, remote->rank, sctk_route_get_is_initiator(route_table) );
    memcpy(&recv_keys, request, sizeof(sctk_ib_cm_qp_connection_t));

    ib_assume(sctk_ib_qp_allocate_get_rtr(remote) == 0);
    sctk_ib_qp_allocate_rtr(rail_ib_targ, remote, &recv_keys);

    sctk_ib_qp_key_fill(&send_keys, remote, device->port_attr.lid,
        remote->qp->qp_num, remote->psn);

    /* Send ACK */
    sctk_ib_nodebug("OD QP ack to process %d: (tag:%d)", src, CM_OD_ACK_TAG+CM_MASK_TAG);

    sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag, CM_OD_ACK_TAG+CM_MASK_TAG,
        &send_keys, sizeof(sctk_ib_cm_qp_connection_t));
  }
  return 0;
}


/*
 * Send a connexion request to the process 'dest'
 */
sctk_route_table_t *sctk_ib_cm_on_demand_request(int dest,sctk_rail_info_t* rail_targ){
  LOAD_TARG();
  LOAD_DEVICE(rail_ib_targ);
  sctk_route_table_t *route_table;
  sctk_ib_cm_qp_connection_t send_keys;
  /* Message to exchange to the peer */
  int added;
  /* If we need to send the request */
  int send_request = 0;

  /* create remote for dest if it does not exist */
  route_table = sctk_route_dynamic_safe_add(dest, rail_targ, sctk_ib_create_remote, sctk_ib_init_remote, &added, 1);
  ib_assume(route_table);

  ROUTE_LOCK(route_table);
  if (sctk_route_get_state(route_table) == state_deconnected) {
    sctk_route_set_state(route_table, state_connecting);
    send_request = 1;
  }
  ROUTE_UNLOCK(route_table);

  /* If we are the first to access the route and if the state
   * is deconnected, so we can proceed to a connection*/
  if (send_request) {
    struct sctk_ib_qp_s *remote = route_table->data.ib.remote;

    sctk_ib_qp_key_fill(&send_keys, remote, device->port_attr.lid,
        remote->qp->qp_num, remote->psn);

    sctk_ib_debug("[%d] OD QP connexion requested to %d", rail_targ->rail_number, remote->rank);
    sctk_route_messages_send(sctk_process_rank,dest,ondemand_specific_message_tag,
        CM_OD_REQ_TAG+CM_MASK_TAG,
        &send_keys, sizeof(sctk_ib_cm_qp_connection_t));
  }

  return route_table;
}


/*-----------------------------------------------------------
 *  ON DEMAND RDMA CONNEXIONS:
 *  Messages are sent using raw data (not like ring where messages are converted into
 *  string).
 *----------------------------------------------------------*/

/* Function which returns if a remote can be connected using RDMA */
int sctk_ib_cm_on_demand_rdma_check_request(
    sctk_rail_info_t* rail_targ, struct sctk_ib_qp_s *remote) {
  int send_request = 0;
  ib_assume(sctk_route_get_state(remote->route_table) == state_connected);

  /* If several threads call this function, only 1 will send a request to
   * the remote process */
  ROUTE_LOCK(remote->route_table);
  if (sctk_ibuf_rdma_get_remote_state_rts(remote) == state_deconnected) {
    sctk_ibuf_rdma_set_remote_state_rts(remote, state_connecting);
    send_request = 1;
  }
  ROUTE_UNLOCK(remote->route_table);

  return send_request;
}

/*
 * Send a connexion request to the process 'dest'.
 * Data exchanged:
 * - size of slots
 * - number of slots
 */
int sctk_ib_cm_on_demand_rdma_request(
    sctk_rail_info_t* rail_targ, struct sctk_ib_qp_s *remote,
    int entry_size, int entry_nb){
  LOAD_TARG();
  LOAD_DEVICE(rail_ib_targ);
  /* If we need to send the request */
  ib_assume(sctk_route_get_state(remote->route_table) == state_connected);


  /* If we are the first to access the route and if the state
   * is deconnected, so we can proceed to a connection*/

  if (sctk_ibuf_rdma_is_connectable(rail_ib_targ, remote, entry_nb, entry_size)) {  /* Can connect to RDMA */

    sctk_ib_cm_rdma_connection_t send_keys;
    sctk_nodebug("Can connect to remote %d", remote->rank);

    send_keys.connected = 1;
    /* We fill the request and we save how many slots are requested as well
     * as the size of each slot */
    remote->od_request.nb = send_keys.nb = entry_nb;
    remote->od_request.size_ibufs = send_keys.size = entry_size;

    sctk_ib_debug("[%d] OD QP RDMA connexion requested to %d (size:%d nb:%d rdma_connections:%d rdma_cancel:%d)",
        rail_targ->rail_number, remote->rank, send_keys.size, send_keys.nb, device->eager_rdma_connections, remote->rdma.cancel_nb);

    sctk_route_messages_send(sctk_process_rank,remote->rank,ondemand_specific_message_tag,
        CM_OD_RDMA_REQ_TAG+CM_MASK_TAG,
        &send_keys, sizeof(sctk_ib_cm_rdma_connection_t));

  } else { /* Cannot connect to RDMA */
    sctk_nodebug("[%d] Cannot connect to remote %d", rail_targ->rail_number, remote->rank);
    /* We reset the state to deconnected */
    /* FIXME: state to reset */
    sctk_ibuf_rdma_set_remote_state_rts(remote, state_deconnected);
  }

  return 1;
}

static inline void sctk_ib_cm_on_demand_rdma_done_recv(RAIL_ARGS, void* done, int src) {
  LOAD_TARG();
  sctk_ib_cm_rdma_connection_t *recv_keys = (sctk_ib_cm_rdma_connection_t*) done;

  /* get the route to process */
  sctk_route_table_t *route_table = sctk_get_route_to_process_no_route(src, rail_targ);
  ib_assume(route_table);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);

  sctk_ib_nodebug("[%d] OD QP connexion DONE REQ received from process %d (%p:%u)", rail_targ->rail_number, src,
      recv_keys->addr, recv_keys->rkey);

  ib_assume(recv_keys->connected == 1);
  /* Update the RDMA regions */
  sctk_ibuf_rdma_update_remote_addr(rail_ib_targ, remote, recv_keys, REGION_SEND);
  sctk_ib_cm_change_state_to_rtr(rail_targ, route_table, CONNECTION);
}

static inline void sctk_ib_cm_on_demand_rdma_recv_ack(RAIL_ARGS, void* ack, int src) {
  LOAD_TARG();
  sctk_ib_cm_rdma_connection_t send_keys;
  sctk_ib_cm_rdma_connection_t *recv_keys = (sctk_ib_cm_rdma_connection_t*) ack;

  /* get the route to process */
  sctk_route_table_t *route_table = sctk_get_route_to_process_no_route(src, rail_targ);
  ib_assume(route_table);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);

  sctk_ib_nodebug("[%d] OD QP connexion ACK received from process %d (%p:%u)", rail_targ->rail_number, src,
      recv_keys->addr, recv_keys->rkey);

  /* If the remote peer is connectable */
  if (recv_keys->connected == 1) {
    /* Allocate the buffer */
    sctk_ibuf_rdma_pool_init(rail_ib_targ, remote);
    /* We create the SEND region */
    sctk_ibuf_rdma_region_init(rail_ib_targ, remote,
        &remote->rdma.pool->region[REGION_SEND],
        RDMA_CHANNEL | SEND_CHANNEL, remote->od_request.nb, remote->od_request.size_ibufs);

    /* Update the RDMA regions */
    sctk_ibuf_rdma_update_remote_addr(rail_ib_targ, remote, recv_keys, REGION_RECV);
    /* Fill the keys */
    sctk_ibuf_rdma_fill_remote_addr(rail_ib_targ, remote, &send_keys, REGION_SEND);

    /* Send the message */
    send_keys.connected = 1;
  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag,
      CM_OD_RDMA_DONE_TAG+CM_MASK_TAG,
      &send_keys,sizeof(sctk_ib_cm_rdma_connection_t));

  sctk_ib_cm_change_state_to_rts(rail_targ, route_table, CONNECTION);

  } else { /* cannot connect */
    /* We cancel the RDMA connection */
    sctk_ibuf_rdma_connection_cancel(rail_ib_targ, remote);
  }
}

/*
 * Receive a request for a source process
 * Data exchanged:
 * - size of slots
 * - number of slots
 * - Address of the send region
 * - Address of the recv region
 */
static inline void sctk_ib_cm_on_demand_rdma_recv_request(RAIL_ARGS, void* request, int src) {
  LOAD_TARG();
  sctk_ib_cm_rdma_connection_t send_keys;
  memset(&send_keys, 0, sizeof(sctk_ib_cm_rdma_connection_t));

  /* get the route to process */
  sctk_route_table_t *route_table = sctk_get_route_to_process_no_route(src, rail_targ);
  ib_assume(route_table);
  /* We assume the route is connected */
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(sctk_route_get_state(route_table) == state_connected);


  ROUTE_LOCK(route_table);
  if (sctk_ibuf_rdma_get_remote_state_rtr(remote) == state_deconnected) {
    sctk_ibuf_rdma_set_remote_state_rtr(remote, state_connecting);
  }
  ROUTE_UNLOCK(route_table);

  sctk_ib_cm_rdma_connection_t *recv_keys = (sctk_ib_cm_rdma_connection_t*) request;
  sctk_ib_nodebug("[%d] OD RDMA connexion REQUEST to process %d (connected:%d size:%d nb:%d rdma_connections:%d)",
      rail_targ->rail_number, remote->rank, recv_keys->connected, recv_keys->size, recv_keys->nb,
      device->eager_rdma_connections);

  /* We do not send a request if we do not want to be connected
   * using RDMA. This is stupid :-) */
  ib_assume(recv_keys->connected == 1);

  /* We check if we can also be connected using RDMA */
  if (sctk_ibuf_rdma_is_connectable(rail_ib_targ, remote, recv_keys->nb, recv_keys->size)) {  /* Can connect to RDMA */

    sctk_nodebug("[%d] Can connect to remote %d", rail_ib->rail_number, remote->rank);

    /* We can change to RTR because we are RDMA connectable */
    send_keys.connected = 1;

    /* We firstly allocate the main structure. 'ibuf_rdma_pool_init'
     * implicitely does not allocate memory if already created */
    sctk_ibuf_rdma_pool_init(rail_ib_targ, remote);
    /* We create the RECV region */
    sctk_ibuf_rdma_region_init(rail_ib_targ, remote,
        &remote->rdma.pool->region[REGION_RECV],
        RDMA_CHANNEL | RECV_CHANNEL, recv_keys->nb, recv_keys->size);

    /* Fill the keys */
    sctk_ibuf_rdma_fill_remote_addr(rail_ib_targ, remote, &send_keys, REGION_RECV);
  } else { /* Cannot connect to RDMA */
    sctk_nodebug("Cannot connect to remote %d", remote->rank);

    send_keys.connected = 0;
    send_keys.size = 0;
    send_keys.nb = 0;
  }

  /* Send ACK */
  sctk_ib_nodebug("[%d] OD QP ack to process %d (%p:%u)", rail_targ->rail_number, src,
      send_keys.addr, send_keys.rkey);

  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag, CM_OD_RDMA_ACK_TAG+CM_MASK_TAG,
      &send_keys, sizeof(sctk_ib_cm_rdma_connection_t));
}


/*-----------------------------------------------------------
 *  ON DEMAND RDMA RESIZING:
 *  Messages are sent using raw data (not like ring where messages are converted into
 *  string).
 *----------------------------------------------------------*/

/*
 * Send a connexion request to the process 'dest'.
 * Data exchanged:
 * - size of slots
 * - number of slots. If 0, the remote needs to be deconnected
 *
 */
int sctk_ib_cm_resizing_rdma_request(sctk_rail_info_t* rail_targ,
    struct sctk_ib_qp_s *remote, int entry_size, int entry_nb){

  /* Assume that the route is in a flushed state */
  ib_assume (sctk_ibuf_rdma_get_remote_state_rts(remote) == state_flushed);
  /* Assume there is no more pending messages */
  ib_assume(OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]) == 0);

  sctk_ib_cm_rdma_connection_t send_keys;

  send_keys.connected = 1;
  send_keys.nb = entry_nb;
  send_keys.size = entry_size;

  sctk_ib_debug("[%d] Sending RDMA RESIZING request to %d (size:%d nb:%d resizing_nb:%d)",
      rail_targ->rail_number, remote->rank, send_keys.size, send_keys.nb,
      remote->rdma.resizing_nb);

  sctk_route_messages_send(sctk_process_rank,remote->rank,ondemand_specific_message_tag,
      CM_RESIZING_RDMA_REQ_TAG+CM_MASK_TAG,
      &send_keys, sizeof(sctk_ib_cm_rdma_connection_t));

  return 1;
}

void sctk_ib_cm_resizing_rdma_ack(sctk_rail_info_t* rail_targ,  struct sctk_ib_qp_s *remote,
    sctk_ib_cm_rdma_connection_t* send_keys){

  ib_assume(sctk_route_get_state(remote->route_table) == state_connected);
  /* Assume that the route is in a flushed state */
  ib_assume (sctk_ibuf_rdma_get_remote_state_rtr(remote) == state_flushed);
  /* Assume there is no more pending messages */
  ib_assume(OPA_load_int(&remote->rdma.pool->busy_nb[REGION_RECV]) == 0);

  sctk_ib_nodebug("[%d] Sending RDMA RESIZING ACK to %d (addr:%p - rkey:%u)",
      rail_targ->rail_number, remote->rank, send_keys->addr, send_keys->rkey);

  sctk_route_messages_send(sctk_process_rank,remote->rank,ondemand_specific_message_tag,
      CM_RESIZING_RDMA_ACK_TAG+CM_MASK_TAG,
      send_keys, sizeof(sctk_ib_cm_rdma_connection_t));
}

static inline void sctk_ib_cm_resizing_rdma_done_recv(RAIL_ARGS, void* done, int src) {
  LOAD_TARG();
  sctk_ib_cm_rdma_connection_t *recv_keys = (sctk_ib_cm_rdma_connection_t*) done;

  /* get the route to process */
  sctk_route_table_t *route_table = sctk_get_route_to_process_no_route(src, rail_targ);
  ib_assume(route_table);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);

  sctk_ib_nodebug("[%d] RDMA RESIZING done req received from process %d (%p:%u)", rail_targ->rail_number, src,
      recv_keys->addr, recv_keys->rkey);

  ib_assume(recv_keys->connected == 1);

  /* Update the RDMA regions */
  sctk_ibuf_rdma_update_remote_addr(rail_ib_targ, remote, recv_keys, REGION_SEND);

  sctk_ib_cm_change_state_to_rtr(rail_targ, route_table, RESIZING);
}


static inline void sctk_ib_cm_resizing_rdma_ack_recv(RAIL_ARGS, void* ack, int src) {
  LOAD_TARG();
  sctk_ib_cm_rdma_connection_t *recv_keys = (sctk_ib_cm_rdma_connection_t*) ack;

  /* get the route to process */
  sctk_route_table_t *route_table = sctk_get_route_to_process_no_ondemand(src, rail_targ);
  ib_assume(route_table);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);

  sctk_ib_nodebug("[%d] RDMA RESIZING ACK received from process %d (addr:%p rkey:%u)", rail_targ->rail_number, src, recv_keys->addr, recv_keys->rkey);

  /* Update the RDMA regions */
  /* FIXME: the rail number should be determinated */
  sctk_ibuf_rdma_update_remote_addr(rail_ib_targ, remote, recv_keys, REGION_RECV);

  /* If the remote peer is connectable */
  sctk_ib_cm_rdma_connection_t *send_keys =
    &remote->rdma.pool->resizing_request.send_keys;
  /* FIXME: do some cool stuff here */
  /* Resizing the RDMA buffer */
  sctk_ibuf_rdma_region_reinit(rail_ib_targ, remote,
      &remote->rdma.pool->region[REGION_SEND],
      RDMA_CHANNEL | SEND_CHANNEL,
      send_keys->nb, send_keys->size);

  OPA_incr_int(&remote->rdma.resizing_nb);
  send_keys->connected = 1;
  sctk_ibuf_rdma_fill_remote_addr(rail_ib_targ, remote, send_keys, REGION_SEND);

  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag,
      CM_RESIZING_RDMA_DONE_TAG+CM_MASK_TAG,
      send_keys,sizeof(sctk_ib_cm_rdma_connection_t));

  sctk_ib_cm_change_state_to_rts(rail_targ, route_table, RESIZING);
}

/*
 * Receive a request for a source process
 * Data exchanged:
 * - size of slots
 * - number of slots
 * - Address of the send region
 * - Address of the recv region
 */
static inline int sctk_ib_cm_resizing_rdma_recv_request(RAIL_ARGS, void* request, int src) {
  LOAD_TARG();
  sctk_ib_cm_rdma_connection_t send_keys;
  memset(&send_keys, 0, sizeof(sctk_ib_cm_rdma_connection_t));

  /* get the route to process */
  sctk_route_table_t *route_table = sctk_get_route_to_process_no_ondemand(src, rail_targ);
  ib_assume(route_table);
  /* We assume the route is connected */
  ib_assume(sctk_route_get_state(route_table) == state_connected);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);

  sctk_ib_cm_rdma_connection_t *recv_keys = (sctk_ib_cm_rdma_connection_t*) request;
  ib_assume(recv_keys->connected == 1);
  remote->rdma.pool->resizing_request.recv_keys.nb   = recv_keys->nb;
  remote->rdma.pool->resizing_request.recv_keys.size = recv_keys->size;

  sctk_ib_nodebug("[%d] Receiving RDMA RESIZING request to process %d (connected:%d size:%d nb:%d)",
      rail_targ->rail_number, remote->rank, recv_keys->connected, recv_keys->size, recv_keys->nb);

  /* Start flushing */
  sctk_ibuf_rdma_flush_recv(rail_ib_targ, remote);

  return 0;
}


/*-----------------------------------------------------------
 *  RDMA deconnection
 *----------------------------------------------------------*/
int sctk_ib_cm_resizing_rdma_deco_request(sctk_rail_info_t* rail_targ,
    struct sctk_ib_qp_s *remote){

  sctk_ib_cm_rdma_connection_t send_keys;

  send_keys.connected = 1;
  send_keys.nb = 0;
  send_keys.size = 0;

  sctk_ib_debug("[%d] Sending RDMA DECONNECTION request to %d",
      rail_targ->rail_number, remote->rank);

  sctk_route_messages_send(sctk_process_rank,remote->rank,ondemand_specific_message_tag,
      CM_RESIZING_RDMA_DECO_REQ_TAG+CM_MASK_TAG,
      &send_keys, sizeof(sctk_ib_cm_rdma_connection_t));

  return 1;
}


/*
 * Receive a request for a source process
 * Data exchanged:
 * - size of slots
 * - number of slots
 * - Address of the send region
 * - Address of the recv region
 */
static inline sctk_ib_cm_resizing_rdma_recv_deco_request(RAIL_ARGS, void* request, int src) {
  LOAD_TARG();
  sctk_ib_cm_rdma_connection_t send_keys;
  memset(&send_keys, 0, sizeof(sctk_ib_cm_rdma_connection_t));

  /* get the route to process */
  sctk_route_table_t *route_table = sctk_get_route_to_process_no_ondemand(src, rail_targ);
  ib_assume(route_table);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  ib_assume(remote);

  /* If the route is connected */
  if (sctk_route_get_state(route_table) == state_connected) {
    /* Try to change the state to flushing.
     * By changing the state of the remote to 'flushing', we automaticaly switch to SR */
    sctk_spinlock_lock(&remote->rdma.flushing_lock);
    sctk_route_state_t ret =
      sctk_ibuf_rdma_cas_remote_state_rts(remote, state_connected, state_flushing);

    if (ret == state_connected) {
      /* Update the slots values requested to 0 -> means that we want to disconnect */
      remote->rdma.pool->resizing_request.send_keys.nb   = 0;
      remote->rdma.pool->resizing_request.send_keys.size = 0;
      sctk_spinlock_unlock(&remote->rdma.flushing_lock);

      sctk_ib_nodebug("Resizing the RMDA buffer for remote %d", remote->rank);
      sctk_ibuf_rdma_check_flush_send(rail_ib_targ, remote);
    } else {
      sctk_spinlock_unlock(&remote->rdma.flushing_lock);
    }
  }

  return 0;
}

/*-----------------------------------------------------------
 *  Handler of OD connexions
 *----------------------------------------------------------*/
int sctk_ib_cm_on_demand_recv(sctk_rail_info_t *rail,
    sctk_thread_ptp_message_t *msg, sctk_ibuf_t* ibuf, int recopy) {
  int process_dest;
  int process_src;
  void* payload;
  sctk_rail_info_t *rail_targ;
  sctk_rail_info_t *rail_sign;

  payload = IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer);
  TODO("OD connections only work with rail number 0! There is no support for multirail now")
  rail_targ = sctk_route_get_rail(0);
  rail_sign = sctk_route_get_rail(1);

  process_dest = msg->sctk_msg_get_destination;
  process_src = msg->sctk_msg_get_source;
  /* If destination of the message */
  if (process_dest == sctk_process_rank) {
    sctk_nodebug("[%d] Receiving OD connexion from %d to %d (%d)", rail_targ->rail_number, process_src, process_dest, msg->body.header.message_tag ^ CM_MASK_TAG);
    switch (msg->body.header.message_tag ^ CM_MASK_TAG) {

      /* QP connection */
      case CM_OD_REQ_TAG:
      sctk_ib_cm_on_demand_recv_request(rail_targ, rail_sign, payload, process_src);
      break;

      case CM_OD_ACK_TAG:
      sctk_ib_cm_on_demand_recv_ack(rail_targ, rail_sign, payload, process_src);
      break;

      case CM_OD_DONE_TAG:
      sctk_ib_cm_on_demand_recv_done(rail_targ, rail_sign, payload, process_src);
      break;

      /* RDMA connection */
      case CM_OD_RDMA_REQ_TAG:
      sctk_ib_cm_on_demand_rdma_recv_request(rail_targ, rail_sign, payload, process_src);
      break;

      case CM_OD_RDMA_ACK_TAG:
      sctk_ib_cm_on_demand_rdma_recv_ack(rail_targ, rail_sign, payload, process_src);
      break;

      case CM_OD_RDMA_DONE_TAG:
      sctk_ib_cm_on_demand_rdma_done_recv(rail_targ, rail_sign, payload, process_src);
      break;

      /* RDMA resizing */
      case CM_RESIZING_RDMA_REQ_TAG:
      sctk_ib_cm_resizing_rdma_recv_request(rail_targ, rail_sign, payload, process_src);
      break;

      case CM_RESIZING_RDMA_ACK_TAG:
      sctk_ib_cm_resizing_rdma_ack_recv(rail_targ, rail_sign, payload, process_src);
      break;

      case CM_RESIZING_RDMA_DONE_TAG:
      sctk_ib_cm_resizing_rdma_done_recv(rail_targ, rail_sign, payload, process_src);
      break;

      case CM_RESIZING_RDMA_DECO_REQ_TAG:
      sctk_ib_cm_resizing_rdma_recv_deco_request(rail_targ, rail_sign, payload, process_src);
      break;

      default:
      sctk_error("Invalid CM request: %d (%d - %d)", msg->body.header.message_tag ^ CM_MASK_TAG, msg->body.header.message_tag & CM_MASK_TAG, msg->body.header.message_tag);
      not_reachable();
      break;

    }
    sctk_ibuf_release(&rail->network.ib, ibuf);
    PROF_INC(rail, ib_free_mem);
    sctk_free(msg);
    return 1;
  } else {
    sctk_nodebug("Forward request to process %d for process %d", process_dest, process_src);
    sctk_ib_eager_recv_free(rail, msg, ibuf, recopy);
    rail->send_message_from_network(msg);
    return 0;
  }
}

#endif
