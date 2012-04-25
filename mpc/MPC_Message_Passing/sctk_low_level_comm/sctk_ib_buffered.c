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
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"

#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "BUFF"
#include "sctk_ib_toolkit.h"
#include "math.h"

/* XXX: Modifications required:
 * - copy in user buffer if the message has already been posted - DONE
 * - Support of fragmented copy
 *
 * */

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

int sctk_ib_buffered_prepare_msg(sctk_rail_info_t* rail,
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

  /* Sometimes it should be interresting to fallback to RDMA :-) */
  if (msg->tail.message_type != sctk_message_contiguous) {
    return 1;
  }
  payload = msg->tail.message.contiguous.addr;
  route_data=&route_table->data.ib;
  remote=route_data->remote;

  sctk_nodebug("Sending buffered message (buffer:nb:%lu, size:%lu)", buffer_nb, size);
  /* While it reamins slots to copy */
  for (buffer_index = 0; buffer_index < buffer_nb; ++buffer_index) {
    ibuf = sctk_ibuf_pick(rail_ib, 1, task_node_number);
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
    IBUF_SET_PROTOCOL(ibuf->buffer, buffered_protocol);
    msg_copied += payload_size;

    IBUF_SET_DEST_TASK(ibuf->buffer, msg->sctk_msg_get_glob_destination);
    IBUF_SET_SRC_TASK(ibuf, msg->sctk_msg_get_glob_source);
    sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf, 0);
  }
  return 0;
}

void sctk_ib_buffered_free_msg(void* arg) {
  sctk_thread_ptp_message_t *msg = (sctk_thread_ptp_message_t*) arg;
  sctk_ib_buffered_entry_t *entry = NULL;
  sctk_rail_info_t* rail;

  entry = msg->tail.ib.buffered.entry;
  rail = entry->msg.tail.ib.buffered.rail;
  assume(entry);

  switch(entry->status & MASK_BASE) {
    case recopy:
      sctk_nodebug("Free payload %p from entry %p", entry->payload, entry);
      sctk_free(entry->payload);
      PROF_INC(rail, free_mem);
      break;

    case zerocopy:
    /* Nothing to do */
      break;

    default: not_reachable();
  }
}

void sctk_ib_buffered_copy(sctk_message_to_copy_t* tmp){
  sctk_ib_buffered_entry_t *entry = NULL;
  sctk_rail_info_t* rail;
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;

  recv = tmp->msg_recv;
  send = tmp->msg_send;
  rail = send->tail.ib.buffered.rail;
  entry = send->tail.ib.buffered.entry;
  assume(entry);
  assume(recv->tail.message_type == sctk_message_contiguous);

  sctk_spinlock_lock(&entry->lock);
  entry->copy_ptr = tmp;
  sctk_nodebug("Copy status (%p): %d", entry, entry->status);
  switch (entry->status & MASK_BASE) {
    case not_set:
      sctk_nodebug("Message directly copied (entry:%p)", entry);
      entry->payload = recv->tail.message.contiguous.addr;
      /* Add matching OK */
      entry->status = zerocopy | match;
      sctk_spinlock_unlock(&entry->lock);
      break;
    case recopy:
      sctk_nodebug("Message directly recopied");
      /* transfer done */
      if ( (entry->status & MASK_DONE) == done) {
        sctk_spinlock_unlock(&entry->lock);
        /* The message is done. All buffers have been received */
        sctk_nodebug("Message recopied free from copy %d (%p)", entry->status, entry);
        sctk_net_message_copy_from_buffer(entry->payload, tmp, 1);
        sctk_free(entry);
        PROF_INC(rail, free_mem);
      } else {
        /* Add matching OK */
        entry->status |= match;
        sctk_spinlock_unlock(&entry->lock);
      }
      break;
    default:
      sctk_error("Got mask %d", entry->status);
      not_reachable();
  }
}

  void
sctk_ib_buffered_poll_recv(sctk_rail_info_t* rail, sctk_ibuf_t *ibuf) {
  sctk_thread_ptp_message_t *msg;
  sctk_ib_buffered_t *buffered;
  sctk_route_table_t* route_table;
  sctk_ib_buffered_entry_t *entry = NULL;
  sctk_ib_qp_t *remote;
  int key;
  int current;
  int src_process;

  buffered = IBUF_GET_BUFFERED_HEADER(ibuf->buffer);
  msg = &buffered->msg;

  src_process = sctk_determine_src_process_from_header(msg);
  assume(src_process != -1);
  route_table = sctk_get_route_to_process(src_process, rail);
  assume(route_table);
  remote = route_table->data.ib.remote;
  key = msg->sctk_msg_get_message_number;

  sctk_spinlock_lock(&remote->ib_buffered.lock);
  HASH_FIND(hh,remote->ib_buffered.entries,&key,sizeof(int),entry);
  if (!entry) {
    /* Allocate memory for message header */
    entry = sctk_malloc(sizeof(sctk_ib_buffered_entry_t));
    assume(entry);
    PROF_INC(rail, alloc_mem);
    /* Copy message header */
    memcpy(&entry->msg, msg, sizeof(sctk_thread_ptp_message_body_t));
    entry->msg.tail.ib.protocol = buffered_protocol;
    entry->msg.tail.ib.buffered.entry = entry;
    entry->msg.tail.ib.buffered.rail = rail;
    /* Prepare matching */
    entry->msg.body.completion_flag = NULL;
    entry->msg.tail.message_type = sctk_message_network;
    sctk_rebuild_header(&entry->msg);
    sctk_reinit_header(&entry->msg, sctk_ib_buffered_free_msg,
        sctk_ib_buffered_copy);
    OPA_store_int(&entry->current, 0);
    /* Add msg to hashtable */
    entry->key = key;
    entry->total = buffered->nb;
    entry->status = not_set;
    sctk_nodebug("Not set: %d (%p)", entry->status, entry);
    entry->lock = SCTK_SPINLOCK_INITIALIZER;
    entry->payload = NULL;
    entry->copy_ptr = NULL;
    HASH_ADD(hh,remote->ib_buffered.entries,key,sizeof(int),entry);
    /* Send message to high level MPC */

    sctk_nodebug("Read msg with number %d", msg->sctk_msg_get_message_number);
    rail->send_message_from_network(&entry->msg);

    sctk_spinlock_lock(&entry->lock);
    /* Should be 'not_set' or 'zerocopy' */
    if ( (entry->status & MASK_BASE) == not_set) {
      entry->payload = sctk_malloc(msg->sctk_msg_get_msg_size);
      assume(entry->payload);
      PROF_INC(rail, alloc_mem);
      entry->status = recopy;
    } else if ( (entry->status & MASK_BASE) != zerocopy) not_reachable();
    sctk_spinlock_unlock(&entry->lock);
  }
  sctk_spinlock_unlock(&remote->ib_buffered.lock);

  sctk_nodebug("Copy frag %d on %d (size:%lu copied:%lu)", buffered->index, buffered->nb, buffered->payload_size, buffered->copied);
  /* Copy the message payload */
  memcpy((char*) entry->payload + buffered->copied,IBUF_GET_BUFFERED_PAYLOAD(ibuf->buffer),
   buffered->payload_size);

  /* If last entry, we send it to MPC */
  /* XXX: horrible use of locks. but we do not have the choice */
  current = OPA_fetch_and_incr_int(&(entry->current));
  sctk_nodebug("%p - %d on %d", entry, current, entry->total);
  if (current == entry->total-1) {
    /* remove entry from HT.
     * XXX: We have to do this before marking message as done */
    sctk_spinlock_lock(&remote->ib_buffered.lock);
    HASH_DEL(remote->ib_buffered.entries, entry);
    sctk_spinlock_unlock(&remote->ib_buffered.lock);

    sctk_spinlock_lock(&entry->lock);
    switch(entry->status & MASK_BASE) {
      case recopy:
        /* Message matched */
        if ( (entry->status & MASK_MATCH) == match) {
          sctk_spinlock_unlock(&entry->lock);
          assume(entry->copy_ptr);
          /* The message is done. All buffers have been received */
          sctk_net_message_copy_from_buffer(entry->payload, entry->copy_ptr, 1);
          sctk_nodebug("Message recopied free from done");
          sctk_free(entry);
          PROF_INC(rail, free_mem);
        } else {
          sctk_nodebug("Free done:%p", entry);
          entry->status |= done;
          sctk_spinlock_unlock(&entry->lock);
        }
        break;
      case zerocopy:
        /* Message matched */
        if ( (entry->status & MASK_MATCH) == match) {
          sctk_spinlock_unlock(&entry->lock);
          assume(entry->copy_ptr);
          sctk_message_completion_and_free(entry->copy_ptr->msg_send,
              entry->copy_ptr->msg_recv);
          sctk_free(entry);
          PROF_INC(rail, free_mem);
          sctk_nodebug("Message zerocpy free from done");
        } else {
          sctk_spinlock_unlock(&entry->lock);
          not_reachable();
        }
        break;
      default: not_reachable();
    }
    sctk_nodebug("Free done:%p", entry);
  }
}
#endif
