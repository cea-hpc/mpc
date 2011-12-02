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
#include "sctk_ib_buffered.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_route.h"
#include "sctk_ib_mmu.h"
#include "sctk_net_tools.h"
#include "sctk_ib_sr.h"

#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "BUFF"
#include "sctk_ib_toolkit.h"
#include "math.h"

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_ib_buffered_prepare_msg(sctk_rail_info_t* rail,
    sctk_route_table_t* route_table, sctk_thread_ptp_message_t * msg, size_t size) {
  sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
  LOAD_CONFIG(rail_ib);
  /* Maximum size for an eager buffer */
  size = size - sizeof(sctk_thread_ptp_message_body_t);
  const size_t buffer_size = config->ibv_eager_limit - IBUF_GET_BUFFERED_SIZE;
  /* Maximum number of buffers */
  int    buffer_nb = ceilf( (float) size / buffer_size);
  int    buffer_index;
  size_t msg_copied=0;
  size_t payload_size;
  sctk_ibuf_t* ibuf;
  sctk_ib_buffered_t *buffered;
  void *payload;
  sctk_ib_data_t *route_data;
  sctk_ib_qp_t *remote;

  if (msg->tail.message_type != sctk_message_contiguous) not_implemented();

  payload = msg->tail.message.contiguous.addr;
  route_data=&route_table->data.ib;
  remote=route_data->remote;

  sctk_nodebug("Sending buffered message (buffer:nb:%lu, size:%lu)", buffer_nb, size);
  /* While it reamins slots to copy */
  for (buffer_index = 0; buffer_index < buffer_nb; ++buffer_index) {
    ibuf = sctk_ibuf_pick(rail_ib, 1, 0);
    buffered = IBUF_GET_BUFFERED_HEADER(ibuf->buffer);

    assume(buffer_size >= sizeof( sctk_thread_ptp_message_body_t));
    memcpy(&buffered->msg, msg, sizeof(sctk_thread_ptp_message_body_t));
    sctk_nodebug("Send number %d, src:%d, dest:%d", msg->sctk_msg_get_message_number, msg->sctk_msg_get_glob_source, msg->sctk_msg_get_glob_destination);

    if (msg_copied + buffer_size > size) {
      payload_size = size - msg_copied;
    } else {
      payload_size = buffer_size;
    }

    memcpy(IBUF_GET_BUFFERED_PAYLOAD(ibuf->buffer), (char*) payload+msg_copied,
      payload_size);

    sctk_nodebug("Send message %d on %d (msg_copied:%lu pyload_size:%lu header:%lu, buffer_size:%lu",
        buffer_index, buffer_nb, msg_copied, payload_size, IBUF_GET_BUFFERED_SIZE, buffer_size);
    /* Initialization of the buffer */
    buffered->index = buffer_index;
    buffered->payload_size = payload_size;
    buffered->copied = msg_copied;
    buffered->nb = buffer_nb;
    sctk_ibuf_send_init(ibuf, IBUF_GET_BUFFERED_SIZE + buffer_size);
    sctk_ibuf_set_protocol(ibuf, buffered_protocol);
    msg_copied += payload_size;

    sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf);
  }
}

void sctk_ib_buffered_free_msg(void* arg) {
  sctk_thread_ptp_message_t *msg = (sctk_thread_ptp_message_t*) arg;
  assume(msg->tail.ib.buffered.entry);
  assume(msg->tail.ib.buffered.reorder_table);

  sctk_reorder_table_t* reorder_table = msg->tail.ib.buffered.reorder_table;

  sctk_spinlock_lock(&reorder_table->ib_buffered.lock);
  HASH_DEL(msg->tail.ib.buffered.reorder_table->ib_buffered.entries,
     msg->tail.ib.buffered.entry);
  sctk_spinlock_unlock(&reorder_table->ib_buffered.lock);
  sctk_free(msg->tail.ib.buffered.entry);
}

void sctk_ib_buffered_copy(sctk_message_to_copy_t* tmp){
  sctk_ib_buffered_entry_t *entry = NULL;
  sctk_rail_info_t* rail;
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;
  void* body;

  recv = tmp->msg_recv;
  send = tmp->msg_send;
  /* Assume msg not recopied */
  rail = send->tail.ib.buffered.rail;
  entry = send->tail.ib.buffered.entry;

  assume(entry);
  body = entry->payload;
  sctk_net_message_copy_from_buffer(body, tmp, 1);
}


  void
sctk_ib_buffered_poll_recv(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_thread_ptp_message_t *msg;
  sctk_ib_buffered_t *buffered;
  sctk_reorder_table_t* reorder_table;
  sctk_ib_buffered_entry_t *entry = NULL;
	int key;
  int current;

  buffered = IBUF_GET_BUFFERED_HEADER(ibuf->buffer);
  msg = &buffered->msg;
  reorder_table = sctk_get_reorder(msg->sctk_msg_get_source);
  assume(reorder_table);
  key = msg->sctk_msg_get_message_number;

  sctk_spinlock_lock(&reorder_table->ib_buffered.lock);
  HASH_FIND(hh,reorder_table->ib_buffered.entries,&key,sizeof(int),entry);
  if (!entry) {
    sctk_nodebug("Allocating memory %lu", msg->sctk_msg_get_msg_size);
    entry = sctk_malloc(msg->sctk_msg_get_msg_size + sizeof(sctk_ib_buffered_entry_t));
    assume(entry);
    entry->key = key;
    entry->total = buffered->nb;
    OPA_store_int(&entry->current, 0);
    entry->payload = (char*) entry + sizeof(sctk_ib_buffered_entry_t);
    HASH_ADD(hh,reorder_table->ib_buffered.entries,key,sizeof(int),entry);
  }
  sctk_spinlock_unlock(&reorder_table->ib_buffered.lock);

  sctk_nodebug("Copy frag %d on %d (size:%lu copied:%lu)", buffered->index, buffered->nb, buffered->payload_size, buffered->copied);
  /* Copy the message payload */
  memcpy((char*) entry->payload + buffered->copied,IBUF_GET_BUFFERED_PAYLOAD(ibuf->buffer),
   buffered->payload_size);

  /* If last entry, we send it to MPC */
  current = OPA_fetch_and_incr_int(&(entry->current));
  if (current == entry->total-1) {
    /* Copy the message header */
    memcpy(&entry->msg, msg, sizeof(sctk_thread_ptp_message_body_t));
    entry->msg.tail.ib.buffered.entry = entry;
    entry->msg.tail.ib.buffered.rail = rail;
    entry->msg.tail.ib.buffered.reorder_table = reorder_table;

    entry->msg.body.completion_flag = NULL;
    entry->msg.tail.message_type = sctk_message_network;

    sctk_rebuild_header(&entry->msg);
    sctk_reinit_header(&entry->msg, sctk_ib_buffered_free_msg,
        sctk_ib_buffered_copy);
  rail->send_message_from_network(&entry->msg);
  }
}
#endif
