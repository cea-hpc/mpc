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


#include <sctk_debug.h>
#include <sctk_ib_toolkit.h>
#include <sctk_route.h>
#include <sctk_net_tools.h>

#include <sctk_ib_fallback.h>
#include "sctk_ib.h"
#include <sctk_ibufs.h>
#include <sctk_ib_mmu.h>
#include <sctk_ib_config.h>
#include "sctk_ib_qp.h"
#include "sctk_ib_sr.h"
#include "sctk_ib_polling.h"

static void
sctk_network_send_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_route_table_t* tmp;
  sctk_ib_data_t *route_data;
  sctk_ib_qp_t *remote;
  sctk_ibuf_t *ibuf;

  sctk_nodebug("send message through rail %d",rail->rail_number);

  if(msg->body.header.specific_message_tag == process_specific_message_tag){
    tmp = sctk_get_route_to_process(msg->sctk_msg_get_destination,rail);
  } else {
    tmp = sctk_get_route(msg->sctk_msg_get_glob_destination,rail);
  }

  route_data=&tmp->data.ib;
  remote=route_data->remote;
  sctk_debug("Sending message to %d for %d (task:%d,number:%d) (%p)", remote->rank,
      sctk_get_process_rank_from_task_rank(msg->sctk_msg_get_glob_destination),
      msg->sctk_msg_get_glob_destination, msg->sctk_msg_get_message_number, tmp);
  /* XXX: switch on message sending protocols */

  ibuf = sctk_ib_sr_prepare_msg(rail_ib, remote, msg);
  sctk_ib_qp_send_ibuf(remote, ibuf);

  sctk_complete_and_free_message(msg);
}

static int sctk_network_poll(sctk_rail_info_t* rail, struct ibv_wc* wc)
{
  sctk_ibuf_t *ibuf = NULL;
  size_t size;
  sctk_ib_sr_t *sr_header;
  sctk_thread_ptp_message_t * msg;
  void* body;

  ibuf = (sctk_ibuf_t*) wc->wr_id;
  assume(ibuf);

  sr_header = IBUF_SR_HEADER(ibuf->buffer);
  size = sr_header->eager.payload_size;
  size = size - sizeof(sctk_thread_ptp_message_body_t) +
      sizeof(sctk_thread_ptp_message_t);
  sctk_debug("Malloc size :%lu", size);
  msg = sctk_malloc(size);
  assume(msg);
  body = (char*)msg + sizeof(sctk_thread_ptp_message_t);

  /* Copy the header of the message */
  sctk_debug("Read header %d",sizeof(sctk_thread_ptp_message_body_t));
  memcpy(msg, IBUF_MSG_HEADER(ibuf->buffer), sizeof(sctk_thread_ptp_message_body_t));

  msg->body.completion_flag = NULL;
  msg->tail.message_type = sctk_message_network;

  /* Copy the body of the message */
  size = size - sizeof(sctk_thread_ptp_message_t);
  sctk_debug("Read body %d",size);
  memcpy(body, IBUF_MSG_PAYLOAD(ibuf->buffer), size);

  sctk_debug("Message received for %d from %d (task:%d,size:%lu), glob_dest:%d",
      sctk_get_process_rank_from_task_rank(msg->sctk_msg_get_glob_source),
      sctk_get_process_rank_from_task_rank(msg->sctk_msg_get_source),
      msg->sctk_msg_get_glob_source,size,
      msg->sctk_msg_get_glob_destination);
  sctk_rebuild_header(msg);
  sctk_reinit_header(msg,sctk_free,sctk_net_message_copy);
  rail->send_message_from_network(msg);

  return 0;
}

static void
sctk_network_notify_recv_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);

  sctk_nodebug("Recv_message");
  sctk_ib_cq_poll(rail_ib, device->recv_cq, config->ibv_wc_in_number, sctk_network_poll);
}

static void
sctk_network_notify_matching_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  sctk_nodebug("Matching");
}

static void
sctk_network_notify_perform_message_ib (int remote,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);

  sctk_ib_cq_poll(rail, device->recv_cq, config->ibv_wc_in_number, sctk_network_poll);
}

static void
sctk_network_notify_idle_message_ib (sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);

  sctk_ib_cq_poll(rail, device->recv_cq, config->ibv_wc_in_number, sctk_network_poll);
}

static void
sctk_network_notify_any_source_message_ib (sctk_rail_info_t* rail){

}

static void
sctk_network_connection_to_ib(int from, int to,sctk_rail_info_t* rail){
  sctk_debug("Connection TO from %d to %d", from, to);
}

static void
sctk_network_connection_from_ib(int from, int to,sctk_rail_info_t* rail){
  sctk_debug("Connection FROM from %d to %d", from, to);
}

/************ INIT ****************/
void sctk_network_init_ib(sctk_rail_info_t* rail){
  rail->connect_to = sctk_network_connection_to_ib;
  rail->connect_from = sctk_network_connection_from_ib;
  rail->send_message = sctk_network_send_message_ib;
  rail->notify_recv_message = sctk_network_notify_recv_message_ib;
  rail->notify_matching_message = sctk_network_notify_matching_message_ib;
  rail->notify_perform_message = sctk_network_notify_perform_message_ib;
  rail->notify_idle_message = sctk_network_notify_idle_message_ib;
  rail->notify_any_source_message = sctk_network_notify_any_source_message_ib;
  rail->network_name = "IB";

  /* Infiniband Init */
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ib_device_t *device;
  struct ibv_srq_init_attr srq_attr;
  /* Open config */
  sctk_ib_config_init(rail_ib);
  /* Open device */
  device = sctk_ib_device_open(rail_ib, 0);
  /* Init Proctection Domain */
  sctk_ib_pd_init(device);
  /* Init Completion Queues */
  device->send_cq = sctk_ib_cq_init(device, rail_ib->config);
  device->recv_cq = sctk_ib_cq_init(device, rail_ib->config);
  /* Init SRQ */
  srq_attr = sctk_ib_srq_init_attr(rail_ib);
  sctk_ib_srq_init(rail_ib, &srq_attr);
  /* Print config */
  sctk_ib_config_print(rail_ib);
  sctk_ib_mmu_init(rail_ib);
  sctk_ibuf_pool_init(rail_ib);
  /* Fill SRQ with buffers */
  sctk_ibuf_srq_check_and_post(rail_ib, rail_ib->config->ibv_max_srq_ibufs);

  /* Initialize network */
  sctk_network_init_ib_all(rail, rail->route, rail->route_init);
}
