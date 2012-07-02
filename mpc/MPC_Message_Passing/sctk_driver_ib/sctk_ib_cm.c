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
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;
  state_before_connexion = sctk_route_get_state(route_table);

  switch ( sctk_route_get_origin(route_table) ) {

    case route_origin_static: /* Static route */
      sctk_route_set_state(route_table, state_connected);

      sctk_ib_debug("[%d] Static QP connected to process %d", rail->rail_number,remote->rank);
      break;

    case route_origin_dynamic: /* Dynamic route */
      sctk_route_set_state(route_table, state_connected);

      if (state_before_connexion == state_reconnecting)
        sctk_ib_debug("[%d] OD QP reconnected to process %d", rail->rail_number,remote->rank);
      else
        sctk_ib_debug("[%d] OD QP connected to process %d", rail->rail_number,remote->rank);

      /* Only reccord dynamically created QPs */
      sctk_ib_prof_qp_write(remote->rank, 0,
        sctk_get_time_stamp(), PROF_QP_CREAT);

      break;

    default: not_reachable(); sctk_abort(); /* Not reachable */
  }

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
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  struct sctk_ib_qp_s *remote;
  sctk_ib_cm_qp_connection_t send_keys;
  sctk_ib_cm_qp_connection_t recv_keys;
  sctk_ib_cm_done_t done;
  sctk_nodebug("Connection TO from %d to %d", from, to);

  /* create remote for dest */
  route_table = sctk_ib_create_remote(from, rail, 0);
  remote = route_table->data.ib.remote;
  sctk_ib_nodebug("[%d] QP connection request to process %d", rail->rail_number, remote->rank);

  sctk_route_messages_recv(from,to,ondemand_specific_message_tag, 0,&recv_keys, sizeof(sctk_ib_cm_qp_connection_t));
  sctk_ib_qp_allocate_rtr(rail_ib, remote, &recv_keys);

  sctk_ib_qp_key_fill(&send_keys, remote, device->port_attr.lid,
      remote->qp->qp_num, remote->psn);

  sctk_route_messages_send(to,from,ondemand_specific_message_tag, 0, &send_keys,sizeof(sctk_ib_cm_qp_connection_t));
  sctk_route_messages_recv(from,to,ondemand_specific_message_tag, 0,&done,
      sizeof(sctk_ib_cm_done_t));
  sctk_ib_qp_allocate_rts(rail_ib, remote);
  /* Add route */
  sctk_add_static_route(from, route_table, rail);
  /* Change to connected */
  sctk_ib_cm_change_state(rail, route_table, state_connected);
}

void sctk_ib_cm_connect_from(int from, int to,sctk_rail_info_t* rail){
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
  route_table = sctk_ib_create_remote(to, rail, 0);
  remote = route_table->data.ib.remote;
  sctk_ib_nodebug("[%d] QP connection  request to process %d", rail->rail_number, remote->rank);

  sctk_ib_qp_key_fill(&send_keys, remote, device->port_attr.lid,
      remote->qp->qp_num, remote->psn);

  sctk_route_messages_send(from,to,ondemand_specific_message_tag, 0,&send_keys, sizeof(sctk_ib_cm_qp_connection_t));
  sctk_route_messages_recv(to,from,ondemand_specific_message_tag, 0,&recv_keys, sizeof(sctk_ib_cm_qp_connection_t));
  sctk_ib_qp_allocate_rtr(rail_ib, remote, &recv_keys);
  sctk_ib_qp_allocate_rts(rail_ib, remote);
  sctk_nodebug("FROM: Ready to send to %d", to);

  sctk_route_messages_send(from,to,ondemand_specific_message_tag, 0, &done,
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
    sctk_ib_debug("[%d] OD QP connexion request to process %d",
        rail_sign->rail_number, remote->rank);
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

  /* If we are the first to access the route and if the state
   * is deconnected, so we can proceed to a connection*/
  if (send_request) {

    if (sctk_ibuf_rdma_is_connectable(rail_ib_targ, remote)) {  /* Can connect to RDMA */
      sctk_ib_cm_rdma_connection_t send_keys;
      sctk_nodebug("Can connect to remote %d", remote->rank);

      send_keys.connected = 1;
      /* We fill the request and we save how many slots are requested as well
       * as the size of each slot */
      remote->od_request.nb = send_keys.nb = entry_nb;
      remote->od_request.size_ibufs = send_keys.size = entry_size;

      sctk_ib_debug("[%d] OD QP RDMA connexion requested to %d (size:%d nb:%d rdma_connections:%d)",
        rail_targ->rail_number, remote->rank, send_keys.size, send_keys.nb, device->eager_rdma_connections);

      sctk_route_messages_send(sctk_process_rank,remote->rank,ondemand_specific_message_tag,
        CM_OD_RDMA_REQ_TAG+CM_MASK_TAG,
        &send_keys, sizeof(sctk_ib_cm_rdma_connection_t));

    } else { /* Cannot connect to RDMA */
      sctk_nodebug("[%d] Cannot connect to remote %d", rail->rail_number, remote->rank);
      /* We reset the state to deconnected */
      /* FIXME: state to reset */
      sctk_ibuf_rdma_set_remote_state_rts(remote, state_reset);
    }
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

  sctk_ib_debug("[%d] OD QP connexion DONE REQ received from process %d (%p:%u)", rail_targ->rail_number, src,
      recv_keys->addr, recv_keys->rkey);

  ib_assume(recv_keys->connected == 1);
  /* Update the RDMA regions */
  sctk_ibuf_rdma_update_remote_addr(rail_ib_targ, remote, recv_keys, REGION_SEND);
  sctk_ibuf_rdma_set_remote_state_rtr(remote, state_connected);
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

  sctk_ib_debug("[%d] OD QP connexion ACK received from process %d (%p:%u)", rail_targ->rail_number, src,
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

  sctk_ibuf_rdma_set_remote_state_rts(remote, state_connected);

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
  LOAD_DEVICE(rail_ib_targ);
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
  sctk_ib_debug("[%d] OD RDMA connexion REQUEST to process %d (connected:%d size:%d nb:%d rdma_connections:%d)",
      rail_targ->rail_number, remote->rank, recv_keys->connected, recv_keys->size, recv_keys->nb,
      device->eager_rdma_connections);

  /* We do not send a request if we do not want to be connected
   * using RDMA. This is stupid :-) */
  ib_assume(recv_keys->connected == 1);

  /* We check if we can also be connected using RDMA */
  if (sctk_ibuf_rdma_is_connectable(rail_ib_targ, remote)) {  /* Can connect to RDMA */

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
  sctk_ib_debug("[%d] OD QP ack to process %d (%p:%u)", rail_targ->rail_number, src,
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
 * - number of slots
 *
 *   FIXME: take 'remote' as argument insteed of dest
 */
int sctk_ib_cm_resizing_rdma_request(sctk_rail_info_t* rail_targ,
    struct sctk_ib_qp_s *remote, int entry_size, int entry_nb){

  /* Assume that the route is in a flushed state */
  ib_assume (sctk_ibuf_rdma_get_remote_state_rts(remote) == state_flushed);
  /* Assume there is no more pending messages */
  ib_assume(OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]) == 0);

  sctk_ib_cm_rdma_connection_t send_keys;

  send_keys.connected = 1;
  remote->rdma.pool->resizing_request.send_keys.nb   = send_keys.nb = entry_nb;
  remote->rdma.pool->resizing_request.send_keys.size = send_keys.size = entry_size;

  sctk_ib_nodebug("[%d] Sending RDMA RESIZING request to %d (size:%d nb:%d)",
      rail_targ->rail_number, remote->rank, send_keys.size, send_keys.nb);

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

  /* Change the state of the route */
  sctk_route_state_t state;
  state = sctk_ibuf_rdma_cas_remote_state_rtr(remote, state_flushed, state_connected);
  assume(state == state_flushed);
  sctk_ib_debug("RTR with process %d (nb:%d, size:%lu)", remote->rank,
      remote->rdma.pool->resizing_request.recv_keys.nb,
      remote->rdma.pool->resizing_request.recv_keys.size );
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

  send_keys->connected = 1;
  sctk_ibuf_rdma_fill_remote_addr(rail_ib_targ, remote, send_keys, REGION_SEND);

  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag,
      CM_RESIZING_RDMA_DONE_TAG+CM_MASK_TAG,
      send_keys,sizeof(sctk_ib_cm_rdma_connection_t));

  /* Change the state of the route */
  sctk_route_state_t state;
  state = sctk_ibuf_rdma_cas_remote_state_rts(remote, state_flushed, state_connected);
  assume(state == state_flushed);
  sctk_ib_debug("RTS with process %d (nb:%d, size:%lu)", remote->rank,
      remote->rdma.pool->resizing_request.send_keys.nb,
      remote->rdma.pool->resizing_request.send_keys.size );
}

/*
 * Receive a request for a source process
 * Data exchanged:
 * - size of slots
 * - number of slots
 * - Address of the send region
 * - Address of the recv region
 */
static inline sctk_ib_cm_resizing_rdma_recv_request(RAIL_ARGS, void* request, int src) {
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
 *  On demand QP deconnexion
 *----------------------------------------------------------*/

/* Request msg */
struct __deco_msg_s {
  /* Empty structure */
  int ack;
};

/* Ack msg */
struct __deco_ack_msg_s {
  int ack;
};


/*-----------------------------------------------------------
 *  Deconnexion recv
 *----------------------------------------------------------*/
void sctk_ib_cm_deco_request_recv(sctk_rail_info_t *rail, void* payload, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_route_table_t *route_table;

  sctk_ib_nodebug("[%d] QP deconnexion REQUEST RECV from %d", rail->rail_number, src);

  route_table = sctk_route_dynamic_search(src, rail);

  /* Deconnect the victim */
  sctk_ib_qp_deco_victim(rail_ib, route_table);

  return;
}

void sctk_ib_cm_deco_ack_recv(sctk_rail_info_t *rail, void* ack, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  struct __deco_ack_msg_s * deco_ack = ack;
  /* We can proceed to a QP deconnexion */
  sctk_route_table_t *route_table;

  route_table = sctk_route_dynamic_search(src, rail);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;

  sctk_ib_nodebug("[%d] QP deconnexion ACK RECV from %d", rail->rail_number, src);

  /* Wait until the flushing is terminated.
   * CAREFULL: this function may block the polling */
  while( sctk_ib_qp_get_local_flush_ack(remote) == ACK_UNSET)
    sctk_thread_yield();

  if (deco_ack->ack == ACK_OK) {
    /* We can destroy the QP */
    sctk_ib_qp_allocate_reset(rail_ib, remote);
    sctk_ib_qp_free(remote);
    /* ... and reinit all the structures */
    sctk_ib_qp_set_local_flush_ack(remote, ACK_OK);
    sctk_route_set_state(route_table, state_reset);
    /* FIXME: missing reset */
  } else {
    /* We cannot proceed to a QP deconnexion */
    not_implemented();
  }

  /* send a deconnexion done request */
  sctk_ib_cm_deco_done_request_send(rail, route_table);

  return;
}

void sctk_ib_cm_deco_done_ack_recv(sctk_rail_info_t *rail, void* ack, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  struct __deco_ack_msg_s * deco_ack = ack;
  /* We can proceed to a QP deconnexion */
  sctk_route_table_t *route_table;

  route_table = sctk_route_dynamic_search(src, rail);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;

  sctk_ib_nodebug("[%d] QP deconnexion DONE ACK RECV from %d", rail->rail_number, src);

  /* Release the lock */
  remote->deco_lock = 0;
  return;
}

void sctk_ib_cm_deco_done_request_recv(sctk_rail_info_t *rail, void* ack, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  struct __deco_msg_s * deco_ack = ack;
  /* We can proceed to a QP deconnexion */
  sctk_route_table_t *route_table;

  route_table = sctk_route_dynamic_search(src, rail);
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;

  sctk_ib_nodebug("[%d] QP deconnexion DONE REQUEST RECV from %d", rail->rail_number, src);

  /* The flushing has already been terminated here. We do not need to check it */

  if (deco_ack->ack == ACK_OK) {
    /* We can destroy the QP */
    sctk_ib_qp_allocate_reset(rail_ib, remote);
    sctk_ib_qp_free(remote);
    /* ... and reinit all the structures */
    sctk_ib_qp_set_local_flush_ack(remote, ACK_OK);
    sctk_route_set_state(route_table, state_reset);
    /* FIXME: missing reset */
  } else {
    /* We cannot proceed to a QP deconnexion */
    not_implemented();
  }

  /* send a deconnexion done request */
  sctk_ib_cm_deco_done_ack_send(rail, route_table, ACK_OK);

  /* Release the lock */
  remote->deco_lock = 0;
  return;
}

/*-----------------------------------------------------------
 *  Deconnexion send
 *----------------------------------------------------------*/

void sctk_ib_cm_deco_request_send(sctk_rail_info_t* rail,
    sctk_route_table_t* route_table){
  sctk_ib_data_t *route;
  /* Message to exchange to the peer */
  struct __deco_msg_s msg;
  int dest;

  route=&route_table->data.ib;
  dest = route->remote->rank;

  sctk_ib_nodebug("[%d] QP deconnexion REQUEST SEND from %d", rail->rail_number, dest);

  sctk_route_messages_send(sctk_process_rank,
      dest,ondemand_specific_message_tag,
      ONDEMAND_DECO_REQ_TAG+CM_MASK_TAG,
      &msg,sizeof(struct __deco_msg_s));

  return;
}

void sctk_ib_cm_deco_done_request_send(sctk_rail_info_t* rail,
    sctk_route_table_t* route_table){
  sctk_ib_data_t *route;
  /* Message to exchange to the peer */
  struct __deco_msg_s msg;
  int dest;

  route=&route_table->data.ib;
  dest = route->remote->rank;
  /* FIXME: Change here if deconnexion canceled */
  msg.ack = ACK_OK;

  sctk_ib_nodebug("[%d] QP deconnexion DONE REQUEST SEND from %d", rail->rail_number, dest);

  sctk_route_messages_send(sctk_process_rank,
      dest,ondemand_specific_message_tag,
      ONDEMAND_DECO_DONE_REQ_TAG+CM_MASK_TAG,
      &msg,sizeof(struct __deco_msg_s));

  return;
}


/*
 * Send a deco ack message to the remote.
 * 'ack' contains the answer:
 * 0 -> deconnexion accepted
 * 1 -> deconnexion canceled
 */
void sctk_ib_cm_deco_ack_send(sctk_rail_info_t* rail,
    sctk_route_table_t* route_table, int ack){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ib_data_t *route;
  LOAD_DEVICE(rail_ib);
  /* Message to exchange to the peer */
  struct __deco_ack_msg_s msg;
  int dest;
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;

  msg.ack = ack;
  dest = remote->rank;

  sctk_ib_nodebug("[%d] QP deconnexion ACK SEND from %d", rail->rail_number, dest);

  sctk_route_messages_send(sctk_process_rank,
      dest,ondemand_specific_message_tag,
      ONDEMAND_DECO_ACK_TAG+CM_MASK_TAG,
      &msg,sizeof(struct __deco_ack_msg_s));
  return;
}

void sctk_ib_cm_deco_done_ack_send(sctk_rail_info_t* rail,
    sctk_route_table_t* route_table, int ack){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ib_data_t *route;
  LOAD_DEVICE(rail_ib);
  /* Message to exchange to the peer */
  struct __deco_ack_msg_s msg;
  int dest;
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;

  msg.ack = ack;
  dest = remote->rank;

  sctk_ib_nodebug("[%d] QP deconnexion DONE ACK SEND from %d", rail->rail_number, dest);

  sctk_route_messages_send(sctk_process_rank,
      dest,ondemand_specific_message_tag,
      ONDEMAND_DECO_DONE_ACK_TAG+CM_MASK_TAG,
      &msg,sizeof(struct __deco_ack_msg_s));
  return;
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
#warning "OD connections only work with rail number 0! There is no support for multirail now"
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

      /* QP deconnection */
      case ONDEMAND_DECO_REQ_TAG:
      sctk_ib_cm_deco_request_recv(rail_targ, payload, process_src);
      break;

      case ONDEMAND_DECO_ACK_TAG:
      sctk_ib_cm_deco_ack_recv(rail_targ, payload, process_src);
      break;

      case ONDEMAND_DECO_DONE_REQ_TAG:
      sctk_ib_cm_deco_done_request_recv(rail_targ, payload, process_src);
      break;

      case ONDEMAND_DECO_DONE_ACK_TAG:
      sctk_ib_cm_deco_done_ack_recv(rail_targ, payload, process_src);
      break;

      default:
      sctk_error("Invalid CM request: %d (%d - %d)", msg->body.header.message_tag ^ CM_MASK_TAG, msg->body.header.message_tag & CM_MASK_TAG, msg->body.header.message_tag);
      not_reachable();
      break;

    }
    sctk_ibuf_release(&rail->network.ib, ibuf);
    PROF_INC(rail, free_mem);
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
