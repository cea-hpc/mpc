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
#include <sctk_ib_toolkit.h>
#include <sctk_route.h>
#include <sctk_net_tools.h>
#include <sctk_ib_prof.h>
#include <sctk_ib_fallback.h>
#include "sctk_ib.h"
#include <sctk_ibufs.h>
#include <sctk_ibufs_rdma.h>
#include <sctk_ib_mmu.h>
#include <sctk_ib_config.h>
#include <sctk_ib_fallback_config.h>
#include "sctk_ib_qp.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_async.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_buffered.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"

static void
sctk_network_send_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  sctk_route_table_t* tmp;
  sctk_ib_data_t *route_data;
  sctk_ib_qp_t *remote;
  sctk_ibuf_t *ibuf;
  size_t size;
  char is_control_message = 0;
  specific_message_tag_t tag = msg->body.header.specific_message_tag;

  sctk_nodebug("send message through rail %d to %d",rail->rail_number, msg->sctk_msg_get_destination);
  sctk_nodebug("Send message with tag %d", msg->sctk_msg_get_specific_message_tag);
  /* Determine the route to used */
  if( IS_PROCESS_SPECIFIC_MESSAGE_TAG(tag)) {

    if (IS_PROCESS_SPECIFIC_CONTROL_MESSAGE(tag))
    {
      is_control_message = 1;
      /* send a message with no_ondemand connexion */
      tmp = sctk_get_route_to_process_static(msg->sctk_msg_get_destination,rail);
      sctk_nodebug("Send control message to process %d", msg->sctk_msg_get_destination);
    } else {
      /* send a message to a PROCESS with a possible ondemand connexion if the peer doest not
       * exist.  */
      tmp = sctk_get_route_to_process(msg->sctk_msg_get_destination,rail);
      sctk_nodebug("Send to process %d", msg->sctk_msg_get_destination);
    }
  } else {
    /* send a message to a TASK with a possible ondemand connexion if the peer doest not
     * exist.  */
    sctk_nodebug("Connexion to %d", msg->sctk_msg_get_glob_destination);
    tmp = sctk_get_route(msg->sctk_msg_get_glob_destination,rail);
  }

  route_data=&tmp->data.ib;
  remote=route_data->remote;
  sctk_nodebug("Go to process %d", remote->rank);

  size = msg->body.header.msg_size + sizeof(sctk_thread_ptp_message_body_t);
  if (is_control_message && ((size + IBUF_GET_EAGER_SIZE) > config->ibv_eager_limit) ) {
    sctk_error("MPC tries to send a control message without using the Eager protocol."
        "This is not supported and MPC is going to exit ...");
    sctk_abort();
  }

  /* *
   *
   *  We switch between available protocols
   *
   * */
  if (size+IBUF_GET_EAGER_SIZE <= config->ibv_eager_limit)
  {
    sctk_nodebug("Eager");
    ibuf = sctk_ib_eager_prepare_msg(rail_ib, remote, msg, size, -1, is_control_message);
    /* Actually, it is possible to get a NULL pointer for ibuf. We falback to buffered */
    if (ibuf == NULL) goto buffered;
    /* Send message */
    sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf, is_control_message);
    sctk_complete_and_free_message(msg);
    PROF_INC_RAIL_IB(rail_ib, eager_nb);
    goto exit;
  }

buffered:
  /***** BUFFERED EAGER CHANNEL *****/
  if (size <= config->ibv_frag_eager_limit)  {
    /* Fallback to RDMA if buffered not available or low memory mode */
    if (sctk_ib_buffered_prepare_msg(rail, remote, msg, size) == 1 ) goto error;
    sctk_complete_and_free_message(msg);
    PROF_INC_RAIL_IB(rail_ib, buffered_nb);
    goto exit;
  }

error:
  sctk_error("MPC did not find any channel to use for sending the message."
      "Your job is going to die ...");
  sctk_abort();

exit: {}

}

static int sctk_network_poll_send_ibuf(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf,
    const char from_cp, struct sctk_ib_polling_s* poll) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  int release_ibuf = 1;

  /* Switch on the protocol of the received message */
  switch (IBUF_GET_PROTOCOL(ibuf->buffer)) {
    case eager_protocol:
      release_ibuf = 1;
      break;

    case rdma_protocol:
      release_ibuf = sctk_ib_rdma_poll_send(rail, ibuf);
      sctk_nodebug("Received RMDA write send");
      break;

    case buffered_protocol:
      release_ibuf = 1;
      break;

    default:
      sctk_error ("Got wrong protocol: %d %p", IBUF_GET_PROTOCOL(ibuf->buffer), &IBUF_GET_PROTOCOL(ibuf->buffer));
      assume(0);
      break;
  }

  assume(IBUF_GET_CHANNEL(ibuf) & RC_SR_CHANNEL);
  if(release_ibuf) {
    /* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
    sctk_ibuf_release(rail_ib, ibuf);
  } else {
    sctk_ibuf_release_from_srq(rail_ib, ibuf);
  }

  return 0;
}

static int sctk_network_poll_recv_ibuf(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf,
    const char from_cp, struct sctk_ib_polling_s* poll)
{
  const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL(ibuf->buffer);
  int release_ibuf = 1;
  const struct ibv_wc wc = ibuf->wc;

  /* First we check if the message has an immediate data */
  if (wc.wc_flags & IBV_WC_WITH_IMM) {
    not_reachable();
    sctk_abort();
  } else {

  /* Switch on the protocol of the received message */
    switch (protocol) {
      case eager_protocol:
        sctk_ib_eager_poll_recv(rail, ibuf);
        release_ibuf = 0;
        break;

      case rdma_protocol:
        not_implemented();
        break;

      case buffered_protocol:
        sctk_ib_buffered_poll_recv(rail, ibuf);
        release_ibuf = 1;
        break;

      default:
        assume(0);
    }
  }

  sctk_nodebug("Message received for %d from %d (task:%d), glob_dest:%d",
      sctk_get_process_rank_from_task_rank(msg->sctk_msg_get_glob_source),
      sctk_get_process_rank_from_task_rank(msg->sctk_msg_get_source),
      msg->sctk_msg_get_glob_source,
      msg->sctk_msg_get_glob_destination);

  if (release_ibuf) {
    /* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
    sctk_ibuf_release(&rail->network.ib, ibuf);
  } else {
    sctk_ibuf_release_from_srq(&rail->network.ib, ibuf);
  }

  return 0;
}

static int sctk_network_poll_recv(sctk_rail_info_t* rail, struct ibv_wc* wc,
    struct sctk_ib_polling_s *poll){
  sctk_ibuf_t *ibuf = NULL;
  ibuf = (sctk_ibuf_t*) wc->wr_id;
  assume(ibuf);

  ibuf->wc = *wc;
  return sctk_network_poll_recv_ibuf(rail, ibuf, 0, poll);
}

static int sctk_network_poll_send(sctk_rail_info_t* rail, struct ibv_wc* wc,
    struct sctk_ib_polling_s *poll) {
  sctk_ibuf_t *ibuf = NULL;
  ibuf = (sctk_ibuf_t*) wc->wr_id;
  assume(ibuf);

  /* Decrease the number of pending requests */
  sctk_ib_qp_decr_requests_nb(ibuf->remote);
  sctk_nodebug("Send message released (rank: %d - pending_nb: %d)", ibuf->remote->rank, sctk_ib_qp_get_requests_nb(ibuf->remote));

  ibuf->wc = *wc;
  return sctk_network_poll_send_ibuf(rail, ibuf, 0, poll);
}

#define MAX_TASKS_ALLOWED 1
static OPA_int_t polling_lock;
/* Count how many times the vp is entered to the polling function. We
 * allow recursive calls to the polling function */
static __thread int recursive_polling = 0;
static int sctk_network_poll_all (sctk_rail_info_t* rail) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);
  sctk_ib_polling_t poll;
  POLL_INIT(&poll);

  /* Only one task is allowed to poll new messages from QP */
  int ret = OPA_fetch_and_incr_int(&polling_lock);
  if ( recursive_polling || ret < MAX_TASKS_ALLOWED)
  {
    recursive_polling++;
    /* Poll received messages */
    sctk_ib_cq_poll(rail, device->recv_cq, config->ibv_wc_in_number, &poll, sctk_network_poll_recv);
    /* Poll sent messages */
    sctk_ib_cq_poll(rail, device->send_cq, config->ibv_wc_out_number, &poll, sctk_network_poll_send);
    recursive_polling--;
  }
  OPA_decr_int(&polling_lock);

  return poll.recv_found_own;
}

static int sctk_network_poll_all_and_steal(sctk_rail_info_t *rail) {
  /* POLLING */
  return sctk_network_poll_all(rail);
}

static void
sctk_network_notify_recv_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){ }

static void
sctk_network_notify_matching_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){ }

/* WARNING: This function can be called with dest == sctk_process_rank */
static void
sctk_network_notify_perform_message_ib (int dest, sctk_rail_info_t* rail){
  sctk_network_poll_all_and_steal(rail);
}

static void
sctk_network_notify_idle_message_ib (sctk_rail_info_t* rail){
  /* POLLING */
  sctk_network_poll_all_and_steal(rail);
}

static void
sctk_network_notify_any_source_message_ib (sctk_rail_info_t* rail){
  /* POLLING */
  sctk_network_poll_all_and_steal(rail);
}

static void
sctk_network_connection_to_ib(int from, int to,sctk_rail_info_t* rail){
  sctk_ib_cm_connect_to(from,to,rail);
}

static void
sctk_network_connection_from_ib(int from, int to,sctk_rail_info_t* rail){
  sctk_ib_cm_connect_from(from,to,rail);
}

void sctk_network_init_fallback_ib(sctk_rail_info_t* rail){
  /* XXX: memory never freed */
  char *network_name = sctk_malloc(256);

  OPA_store_int(&polling_lock, 0);
  /* Infiniband Init */
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  memset(rail_ib, 0, sizeof(sctk_ib_rail_info_t));
  /* Profiler init */
  sctk_ib_prof_init(rail_ib);
  sctk_ib_device_t *device;
  struct ibv_srq_init_attr srq_attr;
  /* Open config */
  sctk_ib_fallback_config_init(rail_ib, "fallback");
  /* Open device */
  device = sctk_ib_device_init(rail_ib);
  /* FIXME: pass rail as an arg of sctk_ib_device_init  */
  rail_ib->rail = rail;
  sctk_ib_device_open(rail_ib, 0);
  /* Init Proctection Domain */
  sctk_ib_pd_init(device);
  /* Init Completion Queues */
  device->send_cq = sctk_ib_cq_init(device, rail_ib->config);
  device->recv_cq = sctk_ib_cq_init(device, rail_ib->config);
  /* Init SRQ */
  srq_attr = sctk_ib_srq_init_attr(rail_ib);
  sctk_ib_srq_init(rail_ib, &srq_attr);
  /* Configure all channels */
  sctk_ib_eager_init(rail_ib);
  /* Print config */
  if (sctk_get_verbosity() >= 2) {
    sctk_ib_config_print(rail_ib);
  }
  sctk_ib_mmu_init(rail_ib);
  sctk_ibuf_pool_init(rail_ib);
  /* Fill SRQ with buffers */
  sctk_ibuf_srq_check_and_post(rail_ib, rail_ib->config->ibv_max_srq_ibufs_posted);
  /* Initialize Async thread */
  sctk_ib_async_init(rail);

  LOAD_CONFIG(rail_ib);
  struct ibv_srq_attr mod_attr;
  int rc;
  mod_attr.srq_limit  = config->ibv_srq_credit_thread_limit;
  rc = ibv_modify_srq(device->srq, &mod_attr, IBV_SRQ_LIMIT);
  assume(rc == 0);

  /* Initialize network */
  sprintf(network_name, "IB-MT (v2.0) Fallback %d/%d:%s - %dx %s (%d Gb/s)]",
    device->dev_index+1, device->dev_nb, ibv_get_device_name(device->dev),
    device->link_width, device->link_rate, device->data_rate);

  rail->connect_to = sctk_network_connection_to_ib;
  rail->connect_from = sctk_network_connection_from_ib;
  rail->send_message = sctk_network_send_message_ib;
  rail->notify_recv_message = sctk_network_notify_recv_message_ib;
  rail->notify_matching_message = sctk_network_notify_matching_message_ib;
  rail->notify_perform_message = sctk_network_notify_perform_message_ib;
  rail->notify_idle_message = sctk_network_notify_idle_message_ib;
  rail->notify_any_source_message = sctk_network_notify_any_source_message_ib;
  rail->network_name = network_name;

  sctk_ib_cm_connect_ring(rail, rail->route, rail->route_init);
}
#endif
