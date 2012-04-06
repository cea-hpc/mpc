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
#include <sctk_ib_mmu.h>
#include <sctk_ib_config.h>
#include "sctk_ib_qp.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_sr.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_async.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_buffered.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"

OPA_int_t s_rdma;

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

  /* Check if the remote task is in low mem mode */
  low_memory_mode = sctk_route_is_low_memory_mode_remote(tmp);
  size = msg->body.header.msg_size + sizeof(sctk_thread_ptp_message_body_t);
  if (is_control_message && size + IBUF_GET_EAGER_SIZE > config->ibv_eager_limit) {
    sctk_error("MPC tries to send a control message without using the Eager protocol."
        "This is not supported and MPC is going to exit ...");
    sctk_abort();
  }

  if (size+IBUF_GET_EAGER_SIZE < config->ibv_eager_limit)  {
    ibuf = sctk_ib_sr_prepare_msg(rail_ib, remote, msg, size, -1);
    /* Send message */
    sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf, is_control_message);
    sctk_complete_and_free_message(msg);
    PROF_INC_RAIL_IB(rail_ib, eager_nb);
  } else if (!low_memory_mode && size+IBUF_GET_BUFFERED_SIZE < config->ibv_frag_eager_limit)  {
    sctk_nodebug("Sending message to %d (process_destk:%d,process_src;%d,number:%d) (%p)", remote->rank, msg->sctk_msg_get_destination, msg->sctk_msg_get_source,msg->sctk_msg_get_message_number, tmp);
    /* Fallback to RDMA if buffered not available or low memory mode */
    if (sctk_ib_buffered_prepare_msg(rail, tmp, msg, size) == 1 ) goto rdma;
    sctk_complete_and_free_message(msg);
    PROF_INC_RAIL_IB(rail_ib, buffered_nb);
  } else {
rdma:
    sctk_nodebug("Send RDMA message");
    ibuf = sctk_ib_rdma_prepare_req(rail, tmp, msg, size, -1);
    /* Send message */
    sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf, 0);
    sctk_ib_rdma_prepare_send_msg(rail_ib, msg, size);
    PROF_INC_RAIL_IB(rail_ib, rdma_nb);
  }
}

static int __is_specific_mesage_tag(sctk_thread_ptp_message_body_t *msg)
{
  if (IS_PROCESS_SPECIFIC_MESSAGE_TAG(msg->header.specific_message_tag))
    return 1;
  return 0;
}

int sctk_network_poll_send_ibuf(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf,
    const char from_cp, struct sctk_ib_polling_s* poll) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int steal = config->ibv_steal;
  int release_ibuf = 1;

  /* Switch on the protocol of the received message */
  switch (IBUF_GET_PROTOCOL(ibuf->buffer)) {
    case eager_protocol:
      release_ibuf = 1;
      break;

    case rdma_protocol:
      sctk_nodebug("RDMA message received");
      release_ibuf = sctk_ib_rdma_poll_send(rail, ibuf);
      break;

    case buffered_protocol:
      release_ibuf = 1;
      break;

    default: assume(0);
  }

  if(release_ibuf) {
    /* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
    sctk_ibuf_release(&rail->network.ib, ibuf);
  } else {
    sctk_ibuf_release_from_srq(&rail->network.ib, ibuf);
  }
  return 0;
}

int sctk_network_poll_recv_ibuf(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf,
    const char from_cp, struct sctk_ib_polling_s* poll)
{
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int steal = config->ibv_steal;
  sctk_thread_ptp_message_t * msg = NULL;
  sctk_thread_ptp_message_body_t * msg_ibuf = NULL;
  int release_ibuf = 1;
  int recopy = 1;
  int ondemand = 0;
  specific_message_tag_t tag;

  sctk_nodebug("[%d] Recv ibuf:%p", rail->rail_number,ibuf);
  /* Switch on the protocol of the received message */

  switch (IBUF_GET_PROTOCOL(ibuf->buffer)) {
    case eager_protocol:
      msg_ibuf = IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer);
      tag = msg_ibuf->header.specific_message_tag;

      if (IS_PROCESS_SPECIFIC_MESSAGE_TAG(tag)) {

        /* If on demand, handle message and do not send it
         * to high-layers */
        if (IS_PROCESS_SPECIFIC_ONDEMAND(tag)) {
          sctk_nodebug("Received OD message");
          msg = sctk_ib_sr_recv(rail, ibuf, recopy);
          sctk_ib_cm_on_demand_recv(rail, msg, ibuf, recopy);
          goto release;
        } else if (IS_PROCESS_SPECIFIC_LOW_MEM(tag)) {
          sctk_nodebug("Received low mem message");
          msg = sctk_ib_sr_recv(rail, ibuf, recopy);
#warning "Uncomment after commit"
//          sctk_ib_low_mem_recv(rail, msg, ibuf, recopy);

          goto release;
        }
      } else {
        /* Do not recopy message if it is not a process specific message.
         *
         * When there is an intermediate message, we *MUST* recopy the message
         * because MPC does not match the user buffer with the network buffer (the copy function is
         * not performed) */
        recopy = 0;
      }

      /* Normal message: we handle it */
      msg = sctk_ib_sr_recv(rail, ibuf, recopy);
      sctk_ib_sr_recv_free(rail, msg, ibuf, recopy);
      rail->send_message_from_network(msg);

release:
      release_ibuf = 0;
      break;

    case rdma_protocol:
      sctk_nodebug("RDMA message received");
      release_ibuf = sctk_ib_rdma_poll_recv(rail, ibuf);
      break;

    case buffered_protocol:
      sctk_nodebug("Buffered protocol");
      sctk_ib_buffered_poll_recv(rail, ibuf);
      release_ibuf = 1;
      break;

    default:
      sctk_error("Got protocol: %d", IBUF_GET_PROTOCOL(ibuf->buffer));
      assume(0);
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
  int dest_task = -1;
  int ret;

  if (wc->imm_data == IMM_DATA_RENDEZVOUS_WRITE) {
    sctk_debug("Imm data: %u %u", wc->imm_data, IMM_DATA_NULL);
  }

  dest_task = IBUF_GET_DEST_TASK(ibuf);
  ibuf->cq = recv_cq;
  if (sctk_ib_cp_handle_message(rail, ibuf, dest_task, dest_task, recv_cq) == 0) {
    return sctk_network_poll_recv_ibuf(rail, ibuf, 0, poll);
  } else {
    return 0;
  }
}

static int sctk_network_poll_send(sctk_rail_info_t* rail, struct ibv_wc* wc,
    struct sctk_ib_polling_s *poll) {
  sctk_ibuf_t *ibuf = NULL;
  ibuf = (sctk_ibuf_t*) wc->wr_id;
  assume(ibuf);
  int src_task = -1;
  int dest_task = -1;
  int ret;

  src_task = IBUF_GET_SRC_TASK(ibuf);
  dest_task = IBUF_GET_DEST_TASK(ibuf);
  ibuf->cq = send_cq;
  /* Decrease the number of pending requests */
  sctk_ib_qp_decr_requests_nb(ibuf->remote);

  /* We still check the dest_task. If it is -1, this is a process_specific
   * message, so we need to handle the message asap */
  if (sctk_ib_cp_handle_message(rail, ibuf, dest_task, src_task, recv_cq) == 0) {
      return sctk_network_poll_send_ibuf(rail, ibuf, 0, poll);
  } else {
    return 0;
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
  int steal = config->ibv_steal;
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

  POLL_INIT(&poll);
  sctk_ib_cp_poll_global_list(rail, &poll);
  /* Try to poll incomming messages */
  POLL_INIT(&poll);
  sctk_ib_cp_poll(rail, &poll);

  return poll.recv_found_own;
}

int sctk_network_poll_all_and_steal(sctk_rail_info_t *rail) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int steal = config->ibv_steal;
  sctk_ib_polling_t poll;
  POLL_INIT(&poll);
  int nb_found = 0;

  /* POLLING */
  if (sctk_network_poll_all(rail)==0 && steal > 1){
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

static void
sctk_network_notify_perform_message_ib (int remote,sctk_rail_info_t* rail){
  /* POLLING */
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
  /* XXX: do not add polling here */
//  sctk_network_poll_all_and_steal(rail);
}

static void
sctk_network_connection_to_ib(int from, int to,sctk_rail_info_t* rail){
  sctk_ib_cm_connect_to(from,to,rail);
}

static void
sctk_network_connection_from_ib(int from, int to,sctk_rail_info_t* rail){
  sctk_ib_cm_connect_from(from,to,rail);
}

/************ INIT ****************/
/* XXX: polling thread used for 'fully-connected' topology initialization. Because
 * of PMI lib, when a barrier occurs, the IB polling cannot be executed.
 * This thread is disbaled when the route is initialized */
static void* __polling_thread(void *arg) {
  sctk_rail_info_t* rail = (sctk_rail_info_t*) arg;
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  int steal = config->ibv_steal;

  /* XXX hack to disable CP when fully is used */
  config->ibv_steal = -1;
  while(1) {
    if (sctk_route_is_finalized())
    {
      config->ibv_steal = steal;
      break;
    }
    sctk_network_poll_all(rail);
  }
  return NULL;
}

void sctk_network_init_polling_thread (sctk_rail_info_t* rail, char* topology) {
  if (strcmp("fully", topology) == 0)
  {
    sctk_thread_t pidt;
    sctk_thread_attr_t attr;

    sctk_thread_attr_init (&attr);
    sctk_thread_attr_setscope (&attr, SCTK_THREAD_SCOPE_SYSTEM);
    sctk_user_thread_create (&pidt, &attr, __polling_thread, (void*) rail);
  }
}

void sctk_network_init_fallback_ib(sctk_rail_info_t* rail){
  /* XXX: memory never free */
  char *network_name = sctk_malloc(256);

  OPA_store_int(&polling_lock, 0);
  /* Infiniband Init */
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  /* Profiler init */
  sctk_ib_prof_init(rail_ib);
  sctk_ib_device_t *device;
  struct ibv_srq_init_attr srq_attr;
  /* Open config */
  sctk_ib_config_init(rail_ib, "fallback");
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
  assume(rc == 0);


  /* Initialize network */
  sprintf(network_name, "IB-MT (v2.0) fallback %d/%d:%s]",
    device->dev_index, device->dev_nb, ibv_get_device_name(device->dev));
  rail->connect_to = sctk_network_connection_to_ib;
  rail->connect_from = sctk_network_connection_from_ib;
  rail->send_message = sctk_network_send_message_ib;
  rail->notify_recv_message = sctk_network_notify_recv_message_ib;
  rail->notify_matching_message = sctk_network_notify_matching_message_ib;
  rail->notify_perform_message = sctk_network_notify_perform_message_ib;
  rail->notify_idle_message = sctk_network_notify_idle_message_ib;
  rail->notify_any_source_message = sctk_network_notify_any_source_message_ib;
  rail->network_name = network_name;

  sctk_network_init_ib_all(rail, rail->route, rail->route_init);
}
#endif
