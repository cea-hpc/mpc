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

#include <sctk.h>
#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_reorder.h>
#include <sctk_communicator.h>
#include <sctk_spinlock.h>
#include <uthash.h>
#include <opa_primitives.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_ib_buffered.h>

/*
 * Initialize the reordering for the list
 */
void sctk_reorder_list_init( sctk_reorder_list_t * reorder) {
  reorder->lock = SCTK_SPINLOCK_INITIALIZER;
  reorder->table = NULL;
}

/*
 * Add a reorder entry to the reorder table and initialize it
 */
sctk_reorder_table_t * sctk_init_task_to_reorder(int dest){
  sctk_reorder_table_t* tmp;

  tmp = sctk_malloc(sizeof(sctk_reorder_table_t));
  OPA_store_int(&(tmp->message_number_src),0);
  OPA_store_int(&(tmp->message_number_dest),0);
  tmp->lock = SCTK_SPINLOCK_INITIALIZER;
  tmp->buffer = NULL;
  tmp->key.destination = dest;

  return tmp;
}

/*
 * Get an entry from the reorder. Add it if not exist
 */
sctk_reorder_table_t* sctk_get_task_from_reorder(int dest,
    sctk_reorder_list_t * reorder){
  sctk_reorder_key_t key;
  sctk_reorder_table_t* tmp;

  key.destination = dest;

  sctk_spinlock_lock(&reorder->lock);
  HASH_FIND(hh,reorder->table,&key,sizeof(sctk_reorder_key_t),tmp);
  if (tmp == NULL) {
    tmp = sctk_init_task_to_reorder(dest);
    /* Add the entry */
    HASH_ADD(hh,reorder->table,key,sizeof(sctk_reorder_key_t),tmp);
    sctk_nodebug("Entry for task %d added to %p!!",dest, reorder->table);
  }
  sctk_spinlock_unlock(&reorder->lock);
  assume(tmp);

  return tmp;
}

/* Try to handle all messages with an expected sequence number */
static inline int __send_pending_messages(sctk_reorder_table_t* tmp) {
  int ret = REORDER_FOUND_NOT_EXPECTED;

  if(tmp->buffer != NULL){
    sctk_reorder_buffer_t* reorder;
    int key;
    do{
      sctk_spinlock_lock(&(tmp->lock));
      key = OPA_load_int(&(tmp->message_number_src));
      HASH_FIND(hh,tmp->buffer,&key,sizeof(int),reorder);
      if(reorder != NULL){
        sctk_nodebug("Pending Send %d for %p",reorder->msg->sctk_msg_get_message_number, tmp);
        HASH_DELETE(hh, tmp->buffer, reorder);
        sctk_spinlock_unlock(&(tmp->lock));
        /* We can not keep the lock during sending the message to MPC or the
         * code will deadlock */
        sctk_send_message_try_check(reorder->msg,1);

        OPA_fetch_and_incr_int(&(tmp->message_number_src));
        ret = REORDER_FOUND_EXPECTED;
      } else sctk_spinlock_unlock(&(tmp->lock));
    } while(reorder != NULL);
  }
  return ret;
}


/*
 * Receive message from network
 * */
int sctk_send_message_from_network_reorder (sctk_thread_ptp_message_t * msg){
  const int src_task  = msg->sctk_msg_get_glob_source;
  const int dest_task = msg->sctk_msg_get_glob_destination;

  sctk_nodebug("Recv message from %d to %d (number: %d)", src_task, dest_task, msg->sctk_msg_get_message_number);

  int dest_process;
  int number;
  sctk_reorder_table_t* tmp = NULL;

  if(msg->sctk_msg_get_use_message_numbering == 0){
    /* Renumbering unused */
    return REORDER_NO_NUMBERING;
  } else if(IS_PROCESS_SPECIFIC_MESSAGE_TAG(msg->body.header.specific_message_tag)){
      /* Process specific message tag */
    if (IS_PROCESS_SPECIFIC_MESSAGE_TAG_WITH_ORDERING(msg->body.header.specific_message_tag) == 0) {
      /* Without message reordering */
      msg->sctk_msg_get_use_message_numbering = 0;
      return REORDER_NO_NUMBERING;
    }
    assume(0);
#if 0
    /* With message reordering */
    dest_process = msg->sctk_msg_get_destination;
    sctk_nodebug("Receives process specific with message reordering from %d to %d %d", msg->sctk_msg_get_source, dest_process, msg->sctk_msg_get_message_number);

    sctk_reorder_list_t * list = sctk_ptp_get_reorder_from_destination(-1);
    tmp = sctk_get_task_from_reorder(dest_process, list);
    assume(tmp);
#if 0
    if (tmp == NULL) {
      /* Without message reordering */
      msg->sctk_msg_get_use_message_numbering = 0;
      return REORDER_NO_NUMBERING;
    }
#endif

    number = OPA_load_int(&(tmp->message_number_src));

    if(number == msg->sctk_msg_get_message_number){
      sctk_nodebug("PS recv %d to %d (msg ng:%d)", msg->sctk_msg_get_source, msg->sctk_msg_get_destination, msg->sctk_msg_get_message_number);
      sctk_send_message_try_check(msg,1);
      OPA_fetch_and_incr_int(&(tmp->message_number_src));

      /*Search for pending messages*/
      __send_pending_messages(tmp);
      return REORDER_FOUND_EXPECTED;
    } else {
      sctk_reorder_buffer_t* reorder;

      reorder = &(msg->tail.reorder);
      reorder->key = msg->sctk_msg_get_message_number;
      reorder->msg = msg;

      sctk_spinlock_lock(&(tmp->lock));
      HASH_ADD(hh,tmp->buffer,key,sizeof(int),reorder);
      sctk_spinlock_unlock(&(tmp->lock));
      sctk_nodebug("PS recv %d to %d - delay expecting %d recv %d (glob_source:%d))", msg->sctk_msg_get_source, msg->sctk_msg_get_destination,
          number,msg->sctk_msg_get_message_number, msg->sctk_msg_get_glob_source);

      /* XXX: we *MUST* check once again if there is no pending messages. During
       * adding a msg to the pending list, a new message with a good PSN could be
       * received. If we omit this line, the code can deadlock */
      return __send_pending_messages(tmp);
    }

    not_reachable();
#endif

  } else {
    assume(src_task != -1);
    assume(dest_task != -1);
    sctk_reorder_list_t * list = sctk_ptp_get_reorder_from_destination(dest_task);
    tmp = sctk_get_task_from_reorder(src_task, list);
    assume(tmp != NULL);
    sctk_nodebug("LIST %p ENTRY %p src %d dest %d", list, tmp, src_task, dest_task);

    dest_process = sctk_get_process_rank_from_task_rank(dest_task);
    sctk_nodebug("Recv message from %d to %d (number:%d)",
        msg->sctk_msg_get_glob_source,
		 msg->sctk_msg_get_glob_destination, msg->sctk_msg_get_message_number);

    /* Indirect messages, we do not check PSN */
    if(sctk_process_rank != dest_process){
      return REORDER_NO_NUMBERING;
    }

    number = OPA_load_int(&(tmp->message_number_src));
    sctk_nodebug("wait for %d recv %d",number,msg->sctk_msg_get_message_number);

    if(number == msg->sctk_msg_get_message_number){
      sctk_nodebug("Direct Send %d from %p",msg->sctk_msg_get_message_number, tmp);

      sctk_send_message_try_check(msg,1);

      OPA_fetch_and_incr_int(&(tmp->message_number_src));

      /*Search for pending messages*/
      __send_pending_messages(tmp);
      return REORDER_FOUND_EXPECTED;

    } else {
      sctk_reorder_buffer_t* reorder;

      reorder = &(msg->tail.reorder);

      reorder->key = msg->sctk_msg_get_message_number;
      reorder->msg = msg;

      sctk_spinlock_lock(&(tmp->lock));
      HASH_ADD(hh,tmp->buffer,key,sizeof(int),reorder);
      sctk_spinlock_unlock(&(tmp->lock));
      sctk_nodebug("recv %d to %d - delay wait for %d recv %d (expecting:%d, tmp:%p)", msg->sctk_msg_get_source, msg->sctk_msg_get_destination,
          number,msg->sctk_msg_get_message_number, number, tmp);

      /* XXX: we *MUST* check once again if there is no pending messages. During
       * adding a msg to the pending list, a new message with a good PSN could be
        * received. If we omit this line, the code can deadlock */
      return __send_pending_messages(tmp);
    }
    return 0;
  }
  return 0;
}

/*
 * Send message to network
 * */
int sctk_prepare_send_message_to_network_reorder (sctk_thread_ptp_message_t * msg){
  sctk_reorder_table_t* tmp;
  const int src_task  = msg->sctk_msg_get_glob_source;
  const int dest_task = msg->sctk_msg_get_glob_destination;

  sctk_nodebug("Send message from %d to %d", src_task, dest_task);

  sctk_nodebug("Send message with tag: %d %d %d", msg->body.header.specific_message_tag, (MASK_PROCESS_SPECIFIC | MASK_PROCESS_SPECIFIC_W_ORDERING) & msg->body.header.specific_message_tag, MASK_PROCESS_SPECIFIC | MASK_PROCESS_SPECIFIC_W_ORDERING);

    /* Process specific message tag */
  if(IS_PROCESS_SPECIFIC_MESSAGE_TAG(msg->body.header.specific_message_tag)){
    if (IS_PROCESS_SPECIFIC_MESSAGE_TAG_WITH_ORDERING(msg->body.header.specific_message_tag) == 0) {
      /* Without message reordering */
      msg->sctk_msg_get_use_message_numbering = 0;
      return 1;
    }
    assume(0);
#if 0
    /* With message reordering */
    /* We get the admin list */
    int dest_process;
    sctk_reorder_list_t * list = sctk_ptp_get_reorder_from_destination(-1);
    tmp = sctk_get_task_from_reorder(dest_process, list);
    assume(tmp);

    /* Local numbering */
    msg->sctk_msg_get_use_message_numbering = 1;
    msg->sctk_msg_get_message_number = OPA_fetch_and_incr_int(&(tmp->message_number_dest));

    sctk_nodebug("PS send %d to %d (number:%d)", msg->sctk_msg_get_source, dest_process, msg->sctk_msg_get_message_number);
    return 0;
#endif
  }


  /* Indirect messages */
  int src_process;
  src_process = sctk_get_process_rank_from_task_rank(src_task);
  if(sctk_process_rank != src_process){
    return 0;
  }

  /* Std messages */
  assume(dest_task != -1);
  assume(src_task != -1);
  sctk_nodebug("msg->sctk_msg_get_use_message_numbering %d",msg->sctk_msg_get_use_message_numbering);

  sctk_reorder_list_t * list = sctk_ptp_get_reorder_from_destination(src_task);
  tmp = sctk_get_task_from_reorder(dest_task, list);
  assume(tmp);

  /* Local numbering */
  msg->sctk_msg_get_use_message_numbering = 1;
  msg->sctk_msg_get_message_number = OPA_fetch_and_incr_int(&(tmp->message_number_dest));
  sctk_nodebug("send %d to %d (number:%d)", msg->sctk_msg_get_glob_source,
      msg->sctk_msg_get_glob_destination, msg->sctk_msg_get_message_number);

  return 0;
}
