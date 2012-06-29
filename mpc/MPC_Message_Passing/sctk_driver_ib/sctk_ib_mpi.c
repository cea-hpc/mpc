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
#include <sctk_ib_toolkit.h>
#include <sctk_route.h>
#include <sctk_net_tools.h>

#include <sctk_ib_fallback.h>
#include "sctk_ib.h"
#include <sctk_ibufs.h>
#include <sctk_ibufs_rdma.h>
#include <sctk_ib_mmu.h>
#include <sctk_ib_config.h>
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
  int low_memory_mode = 0;
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

  /* Check if the remote task is in low mem mode */
  low_memory_mode = sctk_route_is_low_memory_mode_remote(tmp);
  size = msg->body.header.msg_size + sizeof(sctk_thread_ptp_message_body_t);
  if (is_control_message && size + IBUF_GET_EAGER_SIZE > config->ibv_eager_limit) {
    sctk_error("MPC tries to send a control message without using the Eager protocol."
        "This is not supported and MPC is going to exit ...");
    sctk_abort();
  }

  /* *
   *
   *  We switch between available protocols
   *
   * */
  if ( ( (sctk_ibuf_rdma_get_remote_state_rts(remote) == state_connected) &&
        (size+IBUF_GET_EAGER_SIZE+IBUF_RDMA_GET_SIZE <= sctk_ibuf_rdma_get_eager_limit(remote) ) )
      || (size+IBUF_GET_EAGER_SIZE <= config->ibv_eager_limit) )
  {
    sctk_nodebug("Eager");
    ibuf = sctk_ib_eager_prepare_msg(rail_ib, remote, msg, size, -1, is_control_message);
    /* Actually, it is possible to get a NULL pointer for ibuf. We falback to buffered */
    if (ibuf == NULL) goto buffered;
    /* Send message */
    sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf, is_control_message);
    sctk_complete_and_free_message(msg);
    PROF_INC_RAIL_IB(rail_ib, eager_nb);

    /* Remote profiling */
    sctk_ibuf_rdma_check_remote(rail_ib, remote, size);

    goto exit;

  }

buffered:
  /***** BUFFERED EAGER CHANNEL *****/
  if (!low_memory_mode && size <= config->ibv_frag_eager_limit)  {
    sctk_nodebug("Buffered");
    /* Fallback to RDMA if buffered not available or low memory mode */
    if (sctk_ib_buffered_prepare_msg(rail, remote, msg, size) == 1 ) goto rdma;
    sctk_complete_and_free_message(msg);
    PROF_INC_RAIL_IB(rail_ib, buffered_nb);

    /* Remote profiling */
    sctk_ibuf_rdma_check_remote(rail_ib, remote, size);

    goto exit;
  }

  /***** RDMA RENDEZVOUS CHANNEL *****/
rdma:
  ibuf = sctk_ib_rdma_prepare_req(rail, remote, msg, size, -1);
  /* Send message */
  sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf, 0);
  sctk_ib_rdma_prepare_send_msg(rail_ib, msg, size);
  PROF_INC_RAIL_IB(rail_ib, rdma_nb);

exit: {}

}

int sctk_network_poll_send_ibuf(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf,
    const char from_cp, struct sctk_ib_polling_s* poll) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  int release_ibuf = 1;
  const struct ibv_wc wc = ibuf->wc;

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
      not_reachable();
      break;
  }

  /* We do not need to release the buffer with the RDMA channel */
  if (IBUF_GET_CHANNEL(ibuf) & RDMA_CHANNEL) {
    sctk_ibuf_release(rail_ib, ibuf);
    return 0;
  }

  ib_assume(IBUF_GET_CHANNEL(ibuf) & RC_SR_CHANNEL);
  if(release_ibuf) {
    /* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
    sctk_ibuf_release(rail_ib, ibuf);
  } else {
    sctk_ibuf_release_from_srq(rail_ib, ibuf);
  }

  return 0;
}

int sctk_network_poll_recv_ibuf(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf,
    const char from_cp, struct sctk_ib_polling_s* poll)
{
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL(ibuf->buffer);
  int release_ibuf = 1;
  const struct ibv_wc wc = ibuf->wc;

  sctk_nodebug("[%d] Recv ibuf:%p", rail->rail_number,ibuf);
#ifdef IB_DEBUG
  sctk_nodebug("Protocol received: %s", sctk_ib_protocol_print(protocol));
#endif


  /* First we check if the message has an immediate data */
  if (wc.wc_flags == IBV_WC_WITH_IMM) {
    if (wc.imm_data & IMM_DATA_RDMA_PIGGYBACK) {
      /* We actually do nothing here */
    } else {
      not_reachable();
#if 0
      sctk_ib_qp_t *remote = sctk_ib_qp_ht_find(rail_ib, wc.qp_num);
      assume(remote);
      /* Find the remote which has received the msg */
      sctk_nodebug("received an RDMA message from rank %d, ibuf index: %u", remote->rank, index);
      sctk_ib_rdma_eager_poll_remote(rail_ib, remote);
#endif
    }

    /* We release the buffer */
    release_ibuf = 1;
  } else {

    /* Switch on the protocol of the received message */
    switch (protocol) {
      case eager_protocol:
        sctk_ib_eager_poll_recv(rail, ibuf);
        release_ibuf = 0;
        break;

      case rdma_protocol:
        release_ibuf = sctk_ib_rdma_poll_recv(rail, ibuf);
        break;

      case buffered_protocol:
        sctk_ib_buffered_poll_recv(rail, ibuf);
        release_ibuf = 1;
        break;

      default:
        not_reachable();
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
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ibuf_t *ibuf = NULL;
  ibuf = (sctk_ibuf_t*) wc->wr_id;
  ib_assume(ibuf);
  int dest_task = -1;
  /* Get the remote associated to the ibuf */
  const sctk_ib_qp_t const *remote = sctk_ib_qp_ht_find(rail_ib, wc->qp_num);
  ib_assume(remote);
  sctk_ib_prof_qp_write(remote->rank, wc->byte_len, sctk_get_time_stamp(), PROF_QP_RECV);

  /* We *MUST* recopy some informations to the ibuf */
  ibuf->wc = *wc;
  ibuf->cq = recv_cq;

  if (IBUF_GET_CHANNEL(ibuf) & RC_SR_CHANNEL) {
    dest_task = IBUF_GET_DEST_TASK(ibuf->buffer);
    ibuf->cq = recv_cq;
    if (sctk_ib_cp_handle_message(rail, ibuf, dest_task, dest_task, recv_cq) == 0) {
      return sctk_network_poll_recv_ibuf(rail, ibuf, 0, poll);
    } else {
      return 0;
    }
  } else {
    return sctk_network_poll_recv_ibuf(rail, ibuf, 0, poll);
  }
}

static int sctk_network_poll_send(sctk_rail_info_t* rail, struct ibv_wc* wc,
    struct sctk_ib_polling_s *poll) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  sctk_ibuf_t *ibuf = NULL;
  ibuf = (sctk_ibuf_t*) wc->wr_id;
  ib_assume(ibuf);
  int src_task = -1;
  int dest_task = -1;

  /* First we check if the message has an immediate data.
   * If it is, we decrease the number of pending requests and we check
   * if the remote is in a 'flushing' state*/
  if (wc->wc_flags & IBV_WC_WITH_IMM) {
    sctk_ib_qp_decr_requests_nb(ibuf->remote);

    /* Check if we are in a flush state */
    OPA_decr_int(&ibuf->remote->rdma.pool->busy_nb[REGION_RECV]);
    sctk_ibuf_rdma_check_flush_recv(rail_ib, ibuf->remote);

    return 0;
  }

  /* We *MUST* recopy some informations to the ibuf */
  ibuf->wc = *wc;
  ibuf->cq = send_cq;

  /* Decrease the number of pending requests */
  sctk_ibuf_rdma_update_max_pending_requests(rail_ib, ibuf->remote);
  sctk_ib_qp_decr_requests_nb(ibuf->remote);
  /* The following assume is false as we can call 'sctk_ib_qp_inc_requests_nb' before
   * 'sctk_ib_qp_get_requests_nb' */
  /* assume(sctk_ib_qp_get_requests_nb(ibuf->remote) >= 0); */

  if (IBUF_GET_CHANNEL(ibuf) & RC_SR_CHANNEL) {
    src_task = IBUF_GET_SRC_TASK(ibuf);
    dest_task = IBUF_GET_DEST_TASK(ibuf->buffer);

    /* We still check the dest_task. If it is -1, this is a process_specific
     * message, so we need to handle the message asap */
    if (sctk_ib_cp_handle_message(rail, ibuf, dest_task, src_task, recv_cq) == 0) {
      return sctk_network_poll_send_ibuf(rail, ibuf, 0, poll);
    } else {
      return 0;
    }
  } else {
    return sctk_network_poll_send_ibuf(rail, ibuf, 0, poll);
  }
}

#define MAX_TASKS_ALLOWED 1
static OPA_int_t polling_lock;
/* Count how many times the vp is entered to the polling function. We
 * allow recursive calls to the polling function */
__thread int recursive_polling = 0;
int sctk_network_poll_all (sctk_rail_info_t* rail) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  LOAD_DEVICE(rail_ib);
  sctk_ib_polling_t poll;
  POLL_INIT(&poll);

  /* Only one task is allowed to poll new messages from QP */
  int ret = OPA_fetch_and_incr_int(&polling_lock);
  if ( recursive_polling || ret < MAX_TASKS_ALLOWED)
  {
#ifdef SCTK_IB_PROFILER
    double e, s;
    if (recursive_polling == 0)
      s = sctk_get_time_stamp();
    poll_cq++;
#endif
    recursive_polling++;
    /* Poll received messages */
    sctk_ib_cq_poll(rail, device->recv_cq, config->ibv_wc_in_number, &poll, sctk_network_poll_recv);
    /* Poll sent messages */
    sctk_ib_cq_poll(rail, device->send_cq, config->ibv_wc_out_number, &poll, sctk_network_poll_send);
    recursive_polling--;
#ifdef SCTK_IB_PROFILER
    if (recursive_polling == 0) {
      e = sctk_get_time_stamp();
      time_poll_cq += (e - s);
    }
#endif
  }
  OPA_decr_int(&polling_lock);

  POLL_INIT(&poll);
  sctk_ib_cp_poll_global_list(rail, &poll);
/* Try to poll incomming messages */
  POLL_INIT(&poll);
  sctk_ib_cp_poll(rail, &poll);

  return poll.recv_found_own;
}

static int sctk_network_poll_all_and_steal(sctk_rail_info_t *rail) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int steal = config->ibv_steal;
  sctk_ib_polling_t poll;
  POLL_INIT(&poll);
  int nb_found = 0;
  int ret;

  call_to_polling++;

  /* POLLING */
  ret = sctk_network_poll_all(rail);
  if (ret == 0 && steal > 1){
    /* If no message has been found -> steal*/
    nb_found += sctk_ib_cp_steal(rail, &poll);
  }
  return nb_found;
}

static void
sctk_network_notify_recv_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  /* POLLING */
  /* XXX: do not add polling here */
//  sctk_network_poll_all_and_steal(rail);
}

static void
sctk_network_notify_matching_message_ib (sctk_thread_ptp_message_t * msg,sctk_rail_info_t* rail){
  /* POLLING */
  /* XXX: do not add polling here */
//  sctk_network_poll_all_and_steal(rail);
}

/* WARNING: This function can be called with dest == sctk_process_rank */
static void
sctk_network_notify_perform_message_ib (int dest, sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  int ret;

  /* If the dest is another process than the current process */
  if (dest != sctk_process_rank) {
    sctk_ib_qp_t *remote;
    sctk_ib_data_t *route_data;
    sctk_route_table_t *route =  sctk_get_route_to_process_no_ondemand(dest, rail);
    ib_assume(route);

    route_data=&route->data.ib;
    remote=route_data->remote;

    /* Poll messages fistly on RDMA. If no message has been found,
     * we continue to poll SR channel */
    ret = sctk_ib_rdma_eager_poll_remote(rail_ib, remote);
    sctk_network_poll_all_and_steal(rail);
  } else {
    /* Else we simply pool all other channels */
    sctk_network_poll_all_and_steal(rail);
  }
}

static void
sctk_network_notify_idle_message_ib (sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  int ret;

  /* POLLING */
  sctk_ib_rdma_eager_walk_remotes(rail_ib, sctk_ib_rdma_eager_poll_remote, &ret);
  sctk_network_poll_all_and_steal(rail);
}

static void
sctk_network_notify_any_source_message_ib (sctk_rail_info_t* rail){
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  int ret;

  /* POLLING */
  sctk_ib_rdma_eager_walk_remotes(rail_ib, sctk_ib_rdma_eager_poll_remote, &ret);
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

void sctk_network_init_mpi_ib(sctk_rail_info_t* rail){
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
  sctk_ib_config_init(rail_ib, "MPI");
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
  /* Initialize collaborative polling */
  sctk_ib_cp_init(rail_ib);

  LOAD_CONFIG(rail_ib);
  struct ibv_srq_attr mod_attr;
  int rc;
  mod_attr.srq_limit  = config->ibv_srq_credit_thread_limit;
  rc = ibv_modify_srq(device->srq, &mod_attr, IBV_SRQ_LIMIT);
  ib_assume(rc == 0);

  /* Initialize network */
  sprintf(network_name, "IB-MT (v2.0) MPI      %d/%d:%s - %dx %s (%d Gb/s)]",
    device->dev_index+1, device->dev_nb, ibv_get_device_name(device->dev),
    device->link_width, device->link_rate, device->data_rate);

#ifdef IB_DEBUG
  if ( sctk_process_rank == 0)  {
    fprintf(stderr, SCTK_COLOR_RED_BOLD(WARNING: MPC debug mode activated. Your job *MAY* be *VERY* slow!)"\n");
  }
#endif

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
