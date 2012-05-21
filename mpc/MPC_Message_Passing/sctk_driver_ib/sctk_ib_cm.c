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

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "CM"
#include "sctk_ib_toolkit.h"

#define MSG_STRING_SIZE 256

struct sctk_ib_qp_s;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/* Change the state of a remote process */
static void sctk_ib_cm_change_state(sctk_rail_info_t* rail,
    sctk_route_table_t *route_table, sctk_route_state_t state) {

  sctk_route_state_t state_before_connexion;
  struct sctk_ib_qp_s *remote = route_table->data.ib.remote;

  switch ( sctk_route_get_origin(route_table) ) {

    case route_origin_static: /* Static route */
      sctk_route_set_state(route_table, state_connected);
      sctk_ibuf_rdma_set_state_remote(remote, state_connected);

      sctk_ib_debug("[%d] Static QP connected to process %d", rail->rail_number,remote->rank);
      break;

    case route_origin_dynamic: /* Dynamic route */
      sctk_route_set_state(route_table, state_connected);
      sctk_ibuf_rdma_set_state_remote(remote, state_connected);

      if (state_before_connexion == state_reconnecting)
        sctk_ib_debug("[%d] OD QP reconnected to process %d", rail->rail_number,remote->rank);
      else
        sctk_ib_debug("[%d] OD QP connected to process %d", rail->rail_number,remote->rank);
      break;

    default: not_reachable(); sctk_abort(); /* Not reachable */
  }
}

/*-----------------------------------------------------------
 *  STATIC CONNEXIONS : intialization to a ring topology
 *----------------------------------------------------------*/
void sctk_ib_cm_connect_ring (sctk_rail_info_t* rail,
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
    sctk_ibuf_rdma_update_remote_addr(rail_ib, route_src->remote, &keys);
    sctk_ib_qp_allocate_rtr(rail_ib, route_src->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_src->remote);
    sctk_ib_qp_keys_send(rail_ib, route_src->remote);
    sctk_pmi_barrier();

    keys = sctk_ib_qp_keys_recv(route_src->remote, dest_rank);
    sctk_ibuf_rdma_update_remote_addr(rail_ib, route_dest->remote, &keys);
    sctk_ib_qp_allocate_rtr(rail_ib, route_dest->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    sctk_ib_add_static_route(dest_rank, route_table_dest, rail);
    sctk_ib_add_static_route(src_rank, route_table_src, rail);

    /* Change to connected */
    sctk_ib_cm_change_state(rail, route_table_dest, state_connected);
    sctk_ib_cm_change_state(rail, route_table_src, state_connected);
  } else {
    /* create remote for dest */
    route_table_dest = sctk_ib_create_remote();
    sctk_ib_init_remote(dest_rank, rail, route_table_dest, 0);
    route_dest=&route_table_dest->data.ib;

    sctk_ib_qp_keys_send(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    /* change state to RTR */
    keys = sctk_ib_qp_keys_recv(route_dest->remote, src_rank);
    sctk_ibuf_rdma_update_remote_addr(rail_ib, route_dest->remote, &keys);
    sctk_ib_qp_allocate_rtr(rail_ib, route_dest->remote, &keys);
    sctk_ib_qp_allocate_rts(rail_ib, route_dest->remote);
    sctk_pmi_barrier();

    sctk_ib_add_static_route(dest_rank, route_table_dest, rail);

    /* Change to connected */
    sctk_ib_cm_change_state(rail, route_table_dest, state_connected);
  }

  sctk_nodebug("Recv from %d, send to %d", src_rank, dest_rank);
}


/*-----------------------------------------------------------
 *  STATIC CONNEXIONS : process to process connexions during intialization
 *  time
 *----------------------------------------------------------*/
void sctk_ib_cm_connect_to(int from, int to,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  char msg[MSG_STRING_SIZE];
  struct sctk_ib_qp_s *remote;
  char *done;
  sctk_nodebug("Connection TO from %d to %d", from, to);

  /* create remote for dest */
  route_table = sctk_ib_create_remote(from, rail, 0);
  remote = route_table->data.ib.remote;
  sctk_ib_nodebug("[%d] QP connection request to process %d", rail->rail_number, remote->rank);

  sctk_route_messages_recv(from,to,ondemand_specific_message_tag, 0,msg,MSG_STRING_SIZE);
  remote->recv_keys = sctk_ib_qp_keys_convert(msg);
  sctk_ib_qp_allocate_rtr(rail_ib, remote, &remote->recv_keys);
  sctk_ibuf_rdma_update_remote_addr(rail_ib, remote, &remote->recv_keys);

  sctk_ib_qp_key_fill(&remote->send_keys, remote, device->port_attr.lid,
      remote->qp->qp_num, remote->psn);

  sctk_ib_qp_key_create_value(msg, MSG_STRING_SIZE, &remote->send_keys);
  sctk_route_messages_send(to,from,ondemand_specific_message_tag, 0,msg,MSG_STRING_SIZE);
  sctk_route_messages_recv(from,to,ondemand_specific_message_tag, 0,&done,sizeof(char));
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
  /* Message to exchange to the peer */
  char msg[MSG_STRING_SIZE];
  char done = 1;
  sctk_nodebug("Connection FROM from %d to %d", from, to);

  /* create remote for dest */
  route_table = sctk_ib_create_remote(to, rail, 0);
  remote = route_table->data.ib.remote;
  sctk_ib_nodebug("[%d] QP connection  request to process %d", rail->rail_number, remote->rank);

  sctk_ib_qp_key_fill(&remote->send_keys, remote, device->port_attr.lid,
      remote->qp->qp_num, remote->psn);

  sctk_ib_qp_key_create_value(msg, MSG_STRING_SIZE, &remote->send_keys);

  sctk_route_messages_send(from,to,ondemand_specific_message_tag, 0,msg,MSG_STRING_SIZE);
  sctk_route_messages_recv(to,from,ondemand_specific_message_tag, 0,msg,MSG_STRING_SIZE);
  remote->recv_keys = sctk_ib_qp_keys_convert(msg);
  sctk_ib_qp_allocate_rtr(rail_ib, remote, &remote->recv_keys);
  sctk_ibuf_rdma_update_remote_addr(rail_ib, remote, &remote->recv_keys);

  sctk_ib_qp_allocate_rts(rail_ib, remote);
  sctk_nodebug("FROM: Ready to send to %d", to);
  sctk_route_messages_send(from,to,ondemand_specific_message_tag, 0,&done,sizeof(char));
  /* Add route */
  sctk_add_static_route(to, route_table, rail);
  /* Change to connected */
  sctk_ib_cm_change_state(rail, route_table, state_connected);
}

/*-----------------------------------------------------------
 *  ON DEMAND CONNEXIONS: process to process connexions during run time
 *----------------------------------------------------------*/
int sctk_ib_cm_on_demand_recv_done(sctk_rail_info_t *rail, void* done, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_route_table_t *route_table;
  struct sctk_ib_qp_s *remote;

  route_table = sctk_route_dynamic_search(src, rail);
  remote = route_table->data.ib.remote;
  sctk_spinlock_lock(&remote->lock_rts);
  if (sctk_ib_qp_allocate_get_rts(remote) == 0){
    sctk_ib_qp_allocate_rts(rail_ib, remote);
  }
  sctk_spinlock_unlock(&remote->lock_rts);

  /* Change to connected */
  sctk_ib_cm_change_state(rail, route_table, state_connected);
  return 0;
}

int sctk_ib_cm_on_demand_recv_ack(sctk_rail_info_t *rail, void* ack, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_route_table_t *route_table;
  char done=1;
  struct sctk_ib_qp_s *remote;

  sctk_ib_nodebug("OD QP connexion ACK received from process %d %s", src, ack);
  route_table = sctk_route_dynamic_search(src, rail);
  assume(route_table);
  remote = route_table->data.ib.remote;
  remote->recv_keys = sctk_ib_qp_keys_convert(ack);
  sctk_ibuf_rdma_update_remote_addr(rail_ib, remote, &remote->recv_keys);

  sctk_spinlock_lock(&remote->lock_rtr);
  if (sctk_ib_qp_allocate_get_rtr(remote) == 0){
    sctk_ib_qp_allocate_rtr(rail_ib, remote, &remote->recv_keys);
  }
  sctk_spinlock_unlock(&remote->lock_rtr);

  sctk_spinlock_lock(&remote->lock_rts);
  if (sctk_ib_qp_allocate_get_rts(remote) == 0){
    sctk_ib_qp_allocate_rts(rail_ib, remote);
  }
  sctk_spinlock_unlock(&remote->lock_rts);

  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag, ONDEMAND_DONE_TAG+ONDEMAND_MASK_TAG,
      &done,sizeof(char));

  /* Change to connected */
  sctk_ib_cm_change_state(rail, route_table, state_connected);
  return 0;
}

int sctk_ib_cm_on_demand_recv_request(sctk_rail_info_t *rail, void* request, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  char msg[MSG_STRING_SIZE];
  int added;
  struct sctk_ib_qp_s *remote;

  /* create remote for source */
  route_table = sctk_route_dynamic_safe_add(src, rail, sctk_ib_create_remote, sctk_ib_init_remote, &added);
  assume(route_table);
  remote = route_table->data.ib.remote;

  sctk_ib_nodebug("[%d] OD QP connexion request to process %d: %s",
      rail->rail_number, remote->rank, request);
  remote->recv_keys = sctk_ib_qp_keys_convert(request);
  sctk_ibuf_rdma_update_remote_addr(rail_ib, remote, &remote->recv_keys);

  sctk_spinlock_lock(&remote->lock_rtr);
  if (sctk_ib_qp_allocate_get_rtr(remote) == 0)  {
    sctk_ib_qp_allocate_rtr(rail_ib, remote, &remote->recv_keys);
  }
  sctk_spinlock_unlock(&remote->lock_rtr);

  sctk_ib_qp_key_fill(&remote->send_keys, remote, device->port_attr.lid,
      remote->qp->qp_num, remote->psn);

  sctk_ib_qp_key_create_value(msg, MSG_STRING_SIZE, &remote->send_keys);

  /* Send ACK */
  sctk_ib_nodebug("OD QP ack to process %d: %s (tag:%d)",
      src, msg, ONDEMAND_ACK_TAG+ONDEMAND_MASK_TAG);
  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag, ONDEMAND_ACK_TAG+ONDEMAND_MASK_TAG, msg,MSG_STRING_SIZE);
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
      ONDEMAND_DECO_REQ_TAG+ONDEMAND_MASK_TAG,
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
      ONDEMAND_DECO_DONE_REQ_TAG+ONDEMAND_MASK_TAG,
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
      ONDEMAND_DECO_ACK_TAG+ONDEMAND_MASK_TAG,
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
      ONDEMAND_DECO_DONE_ACK_TAG+ONDEMAND_MASK_TAG,
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

  payload = IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer);
  process_dest = msg->sctk_msg_get_destination;
  process_src = msg->sctk_msg_get_source;
  /* If destination of the message */
  if (process_dest == sctk_process_rank) {
    sctk_nodebug("[%d] Receiving OD connexion from %d to %d (%d)", rail->rail_number, process_src, process_dest, msg->body.header.message_tag ^ ONDEMAND_MASK_TAG);
    switch (msg->body.header.message_tag ^ ONDEMAND_MASK_TAG) {

      case ONDEMAND_REQ_TAG:
      sctk_ib_cm_on_demand_recv_request(rail, payload, process_src);
      break;

      case ONDEMAND_ACK_TAG:
      sctk_ib_cm_on_demand_recv_ack(rail, payload, process_src);
      break;

      case ONDEMAND_DONE_TAG:
      sctk_ib_cm_on_demand_recv_done(rail, payload, process_src);
      break;

      case ONDEMAND_DECO_REQ_TAG:
      sctk_ib_cm_deco_request_recv(rail, payload, process_src);
      break;

      case ONDEMAND_DECO_ACK_TAG:
      sctk_ib_cm_deco_ack_recv(rail, payload, process_src);
      break;

      case ONDEMAND_DECO_DONE_REQ_TAG:
      sctk_ib_cm_deco_done_request_recv(rail, payload, process_src);
      break;

      case ONDEMAND_DECO_DONE_ACK_TAG:
      sctk_ib_cm_deco_done_ack_recv(rail, payload, process_src);
      break;

      default:
      sctk_error("Invalid CM request: %d (%d - %d)", msg->body.header.message_tag ^ ONDEMAND_MASK_TAG, msg->body.header.message_tag & ONDEMAND_MASK_TAG, msg->body.header.message_tag );
      assume(0);
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

int sctk_ib_cm_on_demand_recv_check(sctk_thread_ptp_message_body_t *msg) {
  /* If on demand, handle message before sending it to high-layers */
  if (msg->header.message_tag & ONDEMAND_MASK_TAG)
  {
   return 1;
  }
  return 0;
}

sctk_route_table_t *sctk_ib_cm_on_demand_request(int dest,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  sctk_ib_data_t *route;
  /* Message to exchange to the peer */
  char msg[MSG_STRING_SIZE];
  int added;
  struct sctk_ib_qp_s *remote;

  /* create remote for dest */
  route_table = sctk_route_dynamic_safe_add(dest, rail, sctk_ib_create_remote, sctk_ib_init_remote, &added);
  remote = route_table->data.ib.remote;

  assume(route_table);
  if (added == 0) return route_table;
  route=&route_table->data.ib;

  sctk_ib_qp_key_fill(&remote->send_keys, route->remote, device->port_attr.lid,
      route->remote->qp->qp_num, route->remote->psn);

  sctk_ib_qp_key_create_value(msg, MSG_STRING_SIZE, &remote->send_keys);
  sctk_ib_nodebug("[%d] OD QP connexion requested to %d", rail->rail_number, route->remote->rank);
  sctk_route_messages_send(sctk_process_rank,dest,ondemand_specific_message_tag,
      ONDEMAND_REQ_TAG+ONDEMAND_MASK_TAG,
      msg,MSG_STRING_SIZE);
  return route_table;
}

#endif
