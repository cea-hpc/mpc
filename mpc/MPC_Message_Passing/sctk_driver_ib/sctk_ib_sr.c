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
#include "sctk_route.h"
#include "sctk_ib_sr.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_net_tools.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"

#define HOSTNAME 2048

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
/* Number of buffered MPC headers. With the eager protocol, we
 * need to proceed to an dynamic allocation to copy the MPC header with
 * its tail. The body is transfered by the network */
#define EAGER_BUFFER_SIZE 100
static sctk_thread_ptp_message_t *eager_buffered_ptp_message = NULL;
static sctk_spinlock_t eager_lock_buffered_ptp_message = SCTK_SPINLOCK_INITIALIZER;

/*
 * Initialize the eager protocol
 */
void sctk_ib_eager_init(sctk_ib_rail_info_t* rail_ib) {
  int i;

  sctk_thread_ptp_message_t *tmp = sctk_malloc(
      EAGER_BUFFER_SIZE * sizeof(sctk_thread_ptp_message_t));

  for (i=0; i < EAGER_BUFFER_SIZE; ++i) {
    sctk_thread_ptp_message_t* entry = &tmp[i];
    entry->from_buffered = 1;
    /* Add the entry to the list */
    LL_PREPEND(eager_buffered_ptp_message, entry);
  }
}

/*
 * Try to pick a buffered MPC header or
 * if not found, allocate one
 */
static inline sctk_thread_ptp_message_t*
sctk_ib_eager_pick_buffered_ptp_message() {
  sctk_thread_ptp_message_t *tmp = NULL;
  if (eager_buffered_ptp_message) {
    sctk_spinlock_lock(&eager_lock_buffered_ptp_message);
    if (eager_buffered_ptp_message) {
      tmp = eager_buffered_ptp_message;
      LL_DELETE(eager_buffered_ptp_message, eager_buffered_ptp_message);
    }
    sctk_spinlock_unlock(&eager_lock_buffered_ptp_message);
  }
  /* If no more entries are available in the buffered list, we allocate */
  if (tmp == NULL) {
    tmp = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
    /* This header must be freed after use */
    tmp->from_buffered = 0;
  }
  return tmp;
}

/*
 * Release an MPC header according to its origin (buffered or allocated)
 */
void sctk_ib_eager_release_buffered_ptp_message(sctk_thread_ptp_message_t* msg) {
  if (msg->from_buffered) {

    sctk_spinlock_lock(&eager_lock_buffered_ptp_message);
    LL_PREPEND(eager_buffered_ptp_message, msg);
    sctk_spinlock_unlock(&eager_lock_buffered_ptp_message);
  } else {
    /* We can simply free the buffer because it was malloc'ed :-) */
    sctk_free(msg);
  }
}


sctk_ibuf_t* sctk_ib_eager_prepare_msg(sctk_ib_rail_info_t* rail_ib,
    sctk_ib_qp_t* remote, sctk_thread_ptp_message_t * msg, size_t size, int low_memory_mode) {
  sctk_ibuf_t *ibuf;
  sctk_ib_eager_t *eager_header;
  void* body;

  body = (char*)msg + sizeof(sctk_thread_ptp_message_t);
  ibuf = sctk_ibuf_pick(rail_ib, remote, 1, task_node_number);
  IBUF_SET_DEST_TASK(ibuf->buffer, msg->sctk_msg_get_glob_destination);
  IBUF_SET_SRC_TASK(ibuf, msg->sctk_msg_get_glob_source);
  IBUF_SET_POISON(ibuf->buffer);

  /* Copy header */
  memcpy(IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer), msg, sizeof(sctk_thread_ptp_message_body_t));
  /* Copy payload */
  sctk_net_copy_in_buffer(msg, IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer));

  /* Initialization of the buffer */
  sctk_ibuf_prepare(rail_ib, remote, ibuf,
      size + IBUF_GET_EAGER_SIZE);

  eager_header = IBUF_GET_EAGER_HEADER(ibuf->buffer);
  eager_header->payload_size = size - sizeof(sctk_thread_ptp_message_body_t);

  return ibuf;
}


/*-----------------------------------------------------------
 *  Internal free functions
 *----------------------------------------------------------*/
static void __free_no_recopy(void* arg) {
  sctk_thread_ptp_message_t *msg = (sctk_thread_ptp_message_t*) arg;
  sctk_ibuf_t *ibuf = NULL;

  /* Assume msg not recopies */
  assume(!msg->tail.ib.eager.recopied);
  ibuf = msg->tail.ib.eager.ibuf;
  sctk_ibuf_release(ibuf->region->rail, ibuf);
  PROF_INC_RAIL_IB(ibuf->region->rail, free_mem);
  sctk_ib_eager_release_buffered_ptp_message(msg);
}

static void __free_with_recopy(void *arg) {
  sctk_thread_ptp_message_t *msg = (sctk_thread_ptp_message_t*) arg;
  sctk_ibuf_t *ibuf = NULL;

  ibuf = msg->tail.ib.eager.ibuf;
  PROF_INC_RAIL_IB(ibuf->region->rail, free_mem);
  sctk_ib_eager_release_buffered_ptp_message(msg);
}

/*-----------------------------------------------------------
 *  Ibuf free function
 *----------------------------------------------------------*/
void
sctk_ib_eager_recv_free(sctk_rail_info_t* rail, sctk_thread_ptp_message_t *msg,
    sctk_ibuf_t *ibuf, int recopy) {
  /* Read from recopied buffer */
  if (recopy){
    sctk_reinit_header(msg,__free_with_recopy,sctk_net_message_copy);
    sctk_ibuf_release(&rail->network.ib, ibuf);
    /* Read from network buffer  */
  } else {
    sctk_reinit_header(msg,__free_no_recopy, sctk_ib_eager_recv_msg_no_recopy);
  }
}

static sctk_thread_ptp_message_t*
sctk_ib_eager_recv(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf, int recopy,
    sctk_ib_protocol_t protocol) {
  size_t size;
  sctk_thread_ptp_message_t * msg;
  void* body;
  IBUF_CHECK_POISON(ibuf->buffer);
  /* XXX: select if a recopy is needed for the message */
  /* XXX: recopy is not compatible with CM */

   /* If recopy required */
  if (recopy)
  {
    sctk_ib_eager_t *eager_header;
    eager_header = IBUF_GET_EAGER_HEADER(ibuf->buffer);

    size = eager_header->payload_size;
    msg = sctk_malloc(size + sizeof(sctk_thread_ptp_message_t));
    PROF_INC(rail, alloc_mem);
    assume(msg);

    body = (char*)msg + sizeof(sctk_thread_ptp_message_t);
    /* Copy the header of the message */
    memcpy(msg, IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer), sizeof(sctk_thread_ptp_message_body_t));

    /* Copy the body of the message */
    memcpy(body, IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer), size);
  } else {
    msg = sctk_ib_eager_pick_buffered_ptp_message();
    assume(msg);
    PROF_INC(rail, alloc_mem);

    /* Copy the header of the message */
    memcpy(msg, IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer), sizeof(sctk_thread_ptp_message_body_t));
  }

  msg->body.completion_flag = NULL;
  msg->tail.message_type = sctk_message_network;
  msg->tail.ib.eager.recopied = recopy;
  msg->tail.ib.eager.ibuf = ibuf;
  msg->tail.ib.protocol = protocol;

  sctk_rebuild_header(msg);
  return msg;
}


void sctk_ib_eager_recv_msg_no_recopy(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  sctk_ibuf_t *ibuf;
  void* body;

  recv = tmp->msg_recv;
  send = tmp->msg_send;
  /* Assume msg not recopied */
  assume(!send->tail.ib.eager.recopied);
  ibuf = send->tail.ib.eager.ibuf;
  assume(ibuf);
  body = IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer);

  sctk_net_message_copy_from_buffer(body, tmp, 1);
}


/* *******
 * Specific to eager
 * ********/

void
sctk_ib_eager_poll_recv(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_thread_ptp_message_body_t * msg_ibuf = IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer);
;
  const specific_message_tag_t tag = msg_ibuf->header.specific_message_tag;
  const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL(ibuf->buffer);
  int recopy = 1;
  sctk_thread_ptp_message_t * msg = NULL;

  if (IS_PROCESS_SPECIFIC_MESSAGE_TAG(tag)) {
    /* If on demand, handle message and do not send it
     * to high-layers */
    if (IS_PROCESS_SPECIFIC_ONDEMAND(tag)) {
      sctk_nodebug("Received OD message");
      msg = sctk_ib_eager_recv(rail, ibuf, recopy, protocol);
      sctk_ib_cm_on_demand_recv(rail, msg, ibuf, recopy);
      return;
    } else if (IS_PROCESS_SPECIFIC_LOW_MEM(tag)) {
      sctk_nodebug("Received low mem message");
      msg = sctk_ib_eager_recv(rail, ibuf, recopy, protocol);
#warning "Uncomment after commit"
      //          sctk_ib_low_mem_recv(rail, msg, ibuf, recopy);

      return;
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
  msg = sctk_ib_eager_recv(rail, ibuf, recopy, protocol);
  sctk_ib_eager_recv_free(rail, msg, ibuf, recopy);
  rail->send_message_from_network(msg);
}

#endif
