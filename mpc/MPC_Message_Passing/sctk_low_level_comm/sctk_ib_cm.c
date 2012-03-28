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
#include "sctk_ib_sr.h"
#include "sctk_ib_prof.h"
#include "sctk_route.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "CM"
#include "sctk_ib_toolkit.h"

#define MSG_STRING_SIZE 256

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/


/*-----------------------------------------------------------
 *  STATIC CONNEXIONS : process to process connexions during intialization
 *  time
 *----------------------------------------------------------*/
void sctk_ib_cm_connect_to(int from, int to,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  sctk_ib_data_t *route;
  char msg[MSG_STRING_SIZE];
  sctk_ib_qp_keys_t keys;
  char *done;
  sctk_nodebug("Connection TO from %d to %d", from, to);

  /* create remote for dest */
  route_table = sctk_ib_create_remote(from, rail, 0);
  route=&route_table->data.ib;
  sctk_ib_debug("[%d] QP connection request to process %d", rail->rail_number, route->remote->rank);

  sctk_route_messages_recv(from,to,ondemand_specific_message_tag, 0,msg,MSG_STRING_SIZE);
  sctk_nodebug("TO: Recv %s", msg);
  keys = sctk_ib_qp_keys_convert(msg);
  sctk_ib_qp_allocate_rtr(rail_ib, route->remote, &keys);

  snprintf(msg, MSG_STRING_SIZE,
      "%05"SCNu16":%010"SCNu32":%010"SCNu32,
      device->port_attr.lid, route->remote->qp->qp_num, route->remote->psn);
  sctk_route_messages_send(to,from,ondemand_specific_message_tag, 0,msg,MSG_STRING_SIZE);
  sctk_nodebug("TO: Send %s", msg);
  sctk_route_messages_recv(from,to,ondemand_specific_message_tag, 0,&done,sizeof(char));
  sctk_nodebug("TO: Received done from %d",from);
  sctk_ib_qp_allocate_rts(rail_ib, route->remote);
  /* Add route */
  sctk_add_static_route(from, route_table, rail);
  sctk_ib_debug("[%d] QP connected to process %d", rail->rail_number,route->remote->rank);
}

void sctk_ib_cm_connect_from(int from, int to,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  sctk_ib_data_t *route;
  /* Message to exchange to the peer */
  char msg[MSG_STRING_SIZE];
  char done = 1;
  sctk_ib_qp_keys_t keys;
  sctk_nodebug("Connection FROM from %d to %d", from, to);

  /* create remote for dest */
  route_table = sctk_ib_create_remote(to, rail, 0);
  route=&route_table->data.ib;
  sctk_ib_debug("[%d] QP connection  request to process %d", rail->rail_number, route->remote->rank);

  snprintf(msg, MSG_STRING_SIZE,
      "%05"SCNu16":%010"SCNu32":%010"SCNu32,
      device->port_attr.lid, route->remote->qp->qp_num, route->remote->psn);

  sctk_nodebug("FROM: Send %s", msg);
  sctk_route_messages_send(from,to,ondemand_specific_message_tag, 0,msg,MSG_STRING_SIZE);
  sctk_route_messages_recv(to,from,ondemand_specific_message_tag, 0,msg,MSG_STRING_SIZE);
  sctk_nodebug("FROM: Recv %s", msg);
  keys = sctk_ib_qp_keys_convert(msg);
  sctk_ib_qp_allocate_rtr(rail_ib, route->remote, &keys);
  sctk_ib_qp_allocate_rts(rail_ib, route->remote);
  sctk_nodebug("FROM: Ready to send to %d", to);
  sctk_route_messages_send(from,to,ondemand_specific_message_tag, 0,&done,sizeof(char));
  /* Add route */
  sctk_add_static_route(to, route_table, rail);
  sctk_ib_debug("[%d] QP connected to process %d", rail->rail_number,route->remote->rank);
}

/*-----------------------------------------------------------
 *  ON DEMAND CONNEXIONS: process to process connexions during run time
 *----------------------------------------------------------*/
int sctk_ib_cm_on_demand_recv_done(sctk_rail_info_t *rail, void* done, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_route_table_t *route_table;
  sctk_ib_data_t *route;

  route_table = sctk_route_dynamic_search(src, rail);
  route=&route_table->data.ib;
  sctk_spinlock_lock(&route->remote->lock_rts);
  if (sctk_ib_qp_allocate_get_rts(route->remote) == 0){
    sctk_ib_qp_allocate_rts(rail_ib, route->remote);
  }
  sctk_spinlock_unlock(&route->remote->lock_rts);
  sctk_route_set_state(route_table, state_connected);
  sctk_ib_debug("[%d] OD QP connected to process %d", rail->rail_number,src);

  return 0;
}

int sctk_ib_cm_on_demand_recv_ack(sctk_rail_info_t *rail, void* ack, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_route_table_t *route_table;
  sctk_ib_data_t *route;
  sctk_ib_qp_keys_t keys;
  char done=1;

  sctk_ib_debug("OD QP connexion ACK received from process %d %s", src, ack);
  route_table = sctk_route_dynamic_search(src, rail);
  assume(route_table);
  route=&route_table->data.ib;
  assume(route);
  keys = sctk_ib_qp_keys_convert(ack);

  sctk_spinlock_lock(&route->remote->lock_rtr);
  if (sctk_ib_qp_allocate_get_rtr(route->remote) == 0){
  sctk_ib_qp_allocate_rtr(rail_ib, route->remote, &keys);
  }
  sctk_spinlock_unlock(&route->remote->lock_rtr);
  sctk_spinlock_lock(&route->remote->lock_rts);
  if (sctk_ib_qp_allocate_get_rts(route->remote) == 0){
    sctk_ib_qp_allocate_rts(rail_ib, route->remote);
  }
  sctk_spinlock_unlock(&route->remote->lock_rts);

  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag, ONDEMAND_DONE_TAG+ONDEMAND_MASK_TAG,
      &done,sizeof(char));
  sctk_route_set_state(route_table, state_connected);
  sctk_ib_debug("[%d] OD QP connected to process %d", rail->rail_number,src);

  return 0;
}

int sctk_ib_cm_on_demand_recv_request(sctk_rail_info_t *rail, void* request, int src) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_DEVICE(rail_ib);
  sctk_route_table_t *route_table;
  sctk_ib_data_t *route;
  sctk_ib_qp_keys_t keys;
  char msg[MSG_STRING_SIZE];
  int added;

  sctk_nodebug("[%d] HERE", rail->rail_number);
  /* create remote for source */
  route_table = sctk_route_dynamic_safe_add(src, rail, sctk_ib_create_remote, &added);
  assume(route_table);
//  if (added == 0) return 1;
  route=&route_table->data.ib;

  sctk_ib_debug("[%d] OD QP connexion request to process %d: %s",
      rail->rail_number, route->remote->rank, request);
  keys = sctk_ib_qp_keys_convert(request);

  sctk_spinlock_lock(&route->remote->lock_rtr);
  if (sctk_ib_qp_allocate_get_rtr(route->remote) == 0)  {
    sctk_ib_qp_allocate_rtr(rail_ib, route->remote, &keys);
  }
  sctk_spinlock_unlock(&route->remote->lock_rtr);

    snprintf(msg, MSG_STRING_SIZE,
        "%05"SCNu16":%010"SCNu32":%010"SCNu32,
        device->port_attr.lid, route->remote->qp->qp_num, route->remote->psn);

  /* Send ACK */
  sctk_ib_nodebug("OD QP ack to process %d: %s (tag:%d)",
      src, msg, ONDEMAND_ACK_TAG+ONDEMAND_MASK_TAG);
  sctk_route_messages_send(sctk_process_rank,src,ondemand_specific_message_tag, ONDEMAND_ACK_TAG+ONDEMAND_MASK_TAG, msg,MSG_STRING_SIZE);
  return 0;
}

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
    sctk_ib_sr_recv_free(rail, msg, ibuf, recopy);
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

  /* create remote for dest */
  route_table = sctk_route_dynamic_safe_add(dest, rail, sctk_ib_create_remote, &added);
  assume(route_table);
  if (added == 0) return route_table;
  route=&route_table->data.ib;

  snprintf(msg, MSG_STRING_SIZE,
      "%05"SCNu16":%010"SCNu32":%010"SCNu32,
      device->port_attr.lid, route->remote->qp->qp_num, route->remote->psn);
  sctk_ib_debug("[%d] OD QP connexion requested to %d", rail->rail_number, route->remote->rank);
  sctk_route_messages_send(sctk_process_rank,dest,ondemand_specific_message_tag,
      ONDEMAND_REQ_TAG+ONDEMAND_MASK_TAG,
      msg,MSG_STRING_SIZE);
  return route_table;
}


/*-----------------------------------------------------------
 *  On demand QP deconnexion
 *----------------------------------------------------------*/
void sctk_ib_cm_deco_all_dynamic_routes() {

}

#endif
