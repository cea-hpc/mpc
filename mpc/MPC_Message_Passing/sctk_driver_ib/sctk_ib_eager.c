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
#include "sctk_route.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_net_tools.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_low_mem.h"
#include "sctk_ib_topology.h"
#define HOSTNAME 2048

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
/* Number of buffered MPC headers. With the eager protocol, we
 * need to proceed to an dynamic allocation to copy the MPC header with
 * its tail. The body is transfered by the network */
#define EAGER_BUFFER_SIZE 100

/*
 * Initialize the eager protocol
 */
void sctk_ib_eager_init(sctk_ib_rail_info_t* rail_ib) {
  int i;

  rail_ib->eager_buffered_ptp_message = NULL;
  rail_ib->eager_lock_buffered_ptp_message = SCTK_SPINLOCK_INITIALIZER;

  sctk_thread_ptp_message_t *tmp = sctk_malloc(
      EAGER_BUFFER_SIZE * sizeof(sctk_thread_ptp_message_t));

  for (i=0; i < EAGER_BUFFER_SIZE; ++i) {
    sctk_thread_ptp_message_t* entry = &tmp[i];
    entry->from_buffered = 1;
    /* Add the entry to the list */
    LL_PREPEND(rail_ib->eager_buffered_ptp_message, entry);
  }
}

/*
 * Try to pick a buffered MPC header or
 * if not found, allocate one
 */
static inline sctk_thread_ptp_message_t*
sctk_ib_eager_pick_buffered_ptp_message(sctk_rail_info_t *rail) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

  sctk_thread_ptp_message_t *tmp = NULL;
  if (rail_ib->eager_buffered_ptp_message) {
    sctk_spinlock_lock(&rail_ib->eager_lock_buffered_ptp_message);
    if (rail_ib->eager_buffered_ptp_message) {
      tmp = rail_ib->eager_buffered_ptp_message;
      LL_DELETE(rail_ib->eager_buffered_ptp_message, rail_ib->eager_buffered_ptp_message);
    }
    sctk_spinlock_unlock(&rail_ib->eager_lock_buffered_ptp_message);
  }
  /* If no more entries are available in the buffered list, we allocate */
  if (tmp == NULL) {
    PROF_INC(rail, ib_alloc_mem);
    tmp = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
    /* This header must be freed after use */
    tmp->from_buffered = 0;
  }
  return tmp;
}

/*
 * Release an MPC header according to its origin (buffered or allocated)
 */
void sctk_ib_eager_release_buffered_ptp_message(sctk_rail_info_t* rail,
    sctk_thread_ptp_message_t* msg) {
  if (msg->from_buffered) {
    sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

    sctk_spinlock_lock(&rail_ib->eager_lock_buffered_ptp_message);
    LL_PREPEND(rail_ib->eager_buffered_ptp_message, msg);
    sctk_spinlock_unlock(&rail_ib->eager_lock_buffered_ptp_message);
  } else {
    /* We can simply free the buffer because it was malloc'ed :-) */
    PROF_INC(rail, ib_free_mem);
    sctk_free(msg);
  }
}


sctk_ibuf_t* sctk_ib_eager_prepare_msg(sctk_ib_rail_info_t* rail_ib,
    sctk_ib_qp_t* remote, sctk_thread_ptp_message_t * msg, size_t size, int low_memory_mode, char is_control_message) {
  sctk_ibuf_t *ibuf;
  sctk_ib_eager_t *eager_header;
  void* body;
  size_t ibuf_size = size + IBUF_GET_EAGER_SIZE;

  body = (char*)msg + sizeof(sctk_thread_ptp_message_t);
  if (is_control_message) {
    ibuf = sctk_ibuf_pick_send_sr(rail_ib);
    sctk_ibuf_prepare(rail_ib, remote, ibuf, ibuf_size);
  } else {
    ibuf = sctk_ibuf_pick_send(rail_ib, remote, &ibuf_size);
  }
  /* We cannot pick an ibuf. We should try the buffered eager protocol */
  if (ibuf == NULL) return NULL;

  IBUF_SET_DEST_TASK(ibuf->buffer, msg->sctk_msg_get_glob_destination);
  IBUF_SET_SRC_TASK(ibuf->buffer, msg->sctk_msg_get_glob_source);
  IBUF_SET_POISON(ibuf->buffer);

  PROF_TIME_START(rail_ib->rail, ib_ibuf_memcpy);
  /* Copy header */
  memcpy(IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer), msg, sizeof(sctk_thread_ptp_message_body_t));
  /* Copy payload */
  sctk_net_copy_in_buffer(msg, IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer));
  PROF_TIME_END(rail_ib->rail, ib_ibuf_memcpy);

  eager_header = IBUF_GET_EAGER_HEADER(ibuf->buffer);
  eager_header->payload_size = size - sizeof(sctk_thread_ptp_message_body_t);
  IBUF_SET_PROTOCOL(ibuf->buffer, eager_protocol);

  return ibuf;
}


/*-----------------------------------------------------------
 *  Internal free functions
 *----------------------------------------------------------*/
static void __free_no_recopy(void* arg) {
  sctk_thread_ptp_message_t *msg = (sctk_thread_ptp_message_t*) arg;
  sctk_ibuf_t *ibuf = NULL;

  /* Assume msg not recopies */
  ib_assume(!msg->tail.ib.eager.recopied);
  ibuf = msg->tail.ib.eager.ibuf;
  sctk_ibuf_release(ibuf->region->rail, ibuf, 0);
  sctk_ib_eager_release_buffered_ptp_message(ibuf->region->rail->rail, msg);
}

static void __free_with_recopy(void *arg) {
  sctk_thread_ptp_message_t *msg = (sctk_thread_ptp_message_t*) arg;
  sctk_ibuf_t *ibuf = NULL;

  ibuf = msg->tail.ib.eager.ibuf;
  sctk_ib_eager_release_buffered_ptp_message(ibuf->region->rail->rail, msg);
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
    sctk_ibuf_release(&rail->network.ib, ibuf, 0);
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
    PROF_INC(rail, ib_alloc_mem);
    ib_assume(msg);

    body = (char*)msg + sizeof(sctk_thread_ptp_message_t);
    /* Copy the header of the message */
    memcpy(msg, IBUF_GET_EAGER_MSG_HEADER(ibuf->buffer), sizeof(sctk_thread_ptp_message_body_t));

    /* Copy the body of the message */
    memcpy(body, IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer), size);
  } else {
    msg = sctk_ib_eager_pick_buffered_ptp_message(rail);
    ib_assume(msg);

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
  ib_assume(!send->tail.ib.eager.recopied);
  ibuf = send->tail.ib.eager.ibuf;
  ib_assume(ibuf);
  body = IBUF_GET_EAGER_MSG_PAYLOAD(ibuf->buffer);

  sctk_net_message_copy_from_buffer(body, tmp, 1);
}

int
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
      sctk_nodebug("Message tag: %d", (msg_ibuf->header.message_tag ^ CM_MASK_TAG) );
      if ( (msg_ibuf->header.message_tag ^ CM_MASK_TAG) != CM_OD_STATIC_TAG) {
        sctk_nodebug("Received OD message");
        msg = sctk_ib_eager_recv(rail, ibuf, recopy, protocol);
        sctk_ib_cm_on_demand_recv(rail, msg, ibuf, recopy);
        return REORDER_FOUND_EXPECTED;
      }
    }
  } else {
    /* Do not recopy message if it is not a process specific message.
     *
     * When there is an intermediate message, we *MUST* recopy the message
     * because MPC does not match the user buffer with the network buffer (the copy function is
     * not performed) */
    recopy = 0;
  }

  sctk_nodebug("Received IBUF %p %d", ibuf, IBUF_GET_CHANNEL(ibuf));

  /* Normal message: we handle it */
  msg = sctk_ib_eager_recv(rail, ibuf, recopy, protocol);
  sctk_ib_eager_recv_free(rail, msg, ibuf, recopy);
  return rail->send_message_from_network(msg);
}

#endif
