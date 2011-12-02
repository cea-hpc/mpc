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

static int dynamic_reordering_buffer_creation = 0;

static sctk_reorder_table_t* sctk_dynamic_reorder_table = NULL;
static sctk_reorder_table_t* sctk_static_reorder_table = NULL;
static sctk_spin_rwlock_t sctk_reorder_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

/*NOT THREAD SAFE use to add a reorder at initialisation time*/
void sctk_add_static_reorder_buffer(int dest){
  sctk_reorder_table_t* tmp;
  sctk_reorder_key_t key;

  key.destination = dest;

  HASH_FIND(hh,sctk_static_reorder_table,&key,sizeof(sctk_reorder_key_t),tmp);
  if(tmp == NULL){
    tmp = sctk_malloc(sizeof(sctk_reorder_table_t));

    tmp->key.destination = dest;
    OPA_store_int(&(tmp->message_number_src),0);
    OPA_store_int(&(tmp->message_number_dest),0);
    tmp->lock = SCTK_SPINLOCK_INITIALIZER;
    tmp->buffer = NULL;
#ifdef MPC_USE_INFINIBAND
    tmp->ib_buffered.entries = NULL;
    tmp->ib_buffered.lock = SCTK_SPINLOCK_INITIALIZER;
#endif
    HASH_ADD(hh,sctk_static_reorder_table,key,sizeof(sctk_reorder_key_t),tmp);
  }
}

/*THREAD SAFE use to add a reorder at compute time*/
void sctk_add_dynamic_reorder_buffer(int dest){
  sctk_reorder_table_t* tmp;
  sctk_reorder_key_t key;

  key.destination = dest;

  sctk_spinlock_write_lock(&sctk_reorder_table_lock);
  HASH_FIND(hh,sctk_dynamic_reorder_table,&key,sizeof(sctk_reorder_key_t),tmp);
  if(tmp == NULL){
    tmp = sctk_malloc(sizeof(sctk_reorder_table_t));

    tmp->key.destination = dest;
    OPA_store_int(&(tmp->message_number_src),0);
    OPA_store_int(&(tmp->message_number_dest),0);
    tmp->lock = SCTK_SPINLOCK_INITIALIZER;
    tmp->buffer = NULL;
#ifdef MPC_USE_INFINIBAND
    tmp->ib_buffered.entries = NULL;
    tmp->ib_buffered.lock = SCTK_SPINLOCK_INITIALIZER;
#endif

    HASH_ADD(hh,sctk_dynamic_reorder_table,key,sizeof(sctk_reorder_key_t),tmp);
  }
  sctk_spinlock_write_unlock(&sctk_reorder_table_lock);
}

static
sctk_reorder_table_t* sctk_get_reorder_to_process(int dest){
  sctk_reorder_key_t key;
  sctk_reorder_table_t* tmp;

  key.destination = dest;

  HASH_FIND(hh,sctk_static_reorder_table,&key,sizeof(sctk_reorder_key_t),tmp);
  if(tmp == NULL){
    sctk_spinlock_read_lock(&sctk_reorder_table_lock);
    HASH_FIND(hh,sctk_dynamic_reorder_table,&key,sizeof(sctk_reorder_key_t),tmp);
    sctk_spinlock_read_unlock(&sctk_reorder_table_lock);
  }

  if((tmp == NULL) && (dynamic_reordering_buffer_creation == 1)){
    sctk_add_dynamic_reorder_buffer(dest);
    sctk_nodebug("Create reordering buffer for %d",dest);
    tmp = sctk_get_reorder_to_process(dest);
    assume(tmp != NULL);
  }
  return tmp;
}

sctk_reorder_table_t* sctk_get_reorder(int dest){
  sctk_reorder_table_t* tmp;
  int process;

  process = sctk_get_process_rank_from_task_rank(dest);

  tmp = sctk_get_reorder_to_process(process);

  return tmp;
}

inline void __send_pending_messages(sctk_reorder_table_t* tmp) {
  if(tmp->buffer != NULL){
    sctk_reorder_buffer_t* reorder;
    int key;
    sctk_nodebug("Proceed pending messages");
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
      } else sctk_spinlock_unlock(&(tmp->lock));
    } while(reorder != NULL);
  }
}

int sctk_send_message_from_network_reorder (sctk_thread_ptp_message_t * msg){
  if(msg->sctk_msg_get_use_message_numbering == 0){
    /* Renumbering unused */
    return 1;
  } else if(msg->body.header.specific_message_tag == process_specific_message_tag){
    /* Process specific messages */
    return 1;
  } else {
    sctk_reorder_table_t* tmp;
    int number;
    int process;
    tmp = sctk_get_reorder(msg->sctk_msg_get_glob_source);
    sctk_nodebug("Recv message from %d to %d (number:%d)",msg->sctk_msg_get_glob_source,
		 msg->sctk_msg_get_glob_destination, msg->sctk_msg_get_message_number);

    process = sctk_get_process_rank_from_task_rank(msg->sctk_msg_get_glob_destination);

    /* Indirect messages, we do not check PSN */
    if(sctk_process_rank != process){
      return 1;
    }

    assume(tmp != NULL);

    number = OPA_load_int(&(tmp->message_number_src));
    sctk_nodebug("wait for %d recv %d",number,msg->sctk_msg_get_message_number);

    if(number == msg->sctk_msg_get_message_number){
      sctk_nodebug("Direct Send %d from %p",msg->sctk_msg_get_message_number, tmp);
      sctk_send_message_try_check(msg,1);
      OPA_fetch_and_incr_int(&(tmp->message_number_src));
      msg = NULL;

      /*Search for pending messages*/
      __send_pending_messages(tmp);
    } else {
      sctk_reorder_buffer_t* reorder;

      reorder = &(msg->tail.reorder);

      reorder->key = msg->sctk_msg_get_message_number;
      reorder->msg = msg;

      sctk_spinlock_lock(&(tmp->lock));
      HASH_ADD(hh,tmp->buffer,key,sizeof(int),reorder);
      sctk_spinlock_unlock(&(tmp->lock));
      sctk_nodebug("Delay wait for %d recv %d (expecting:%d, tmp:%p)",number,msg->sctk_msg_get_message_number, number, tmp);

      /* XXX: we *MUST* check once again if there is no pending messages. During
       * adding a msg to the pending list, a new message with a good PSN could be
        * received. If we omit this line, the code can deadlock */
      __send_pending_messages(tmp);
    }
    return 0;
  }
}

int sctk_prepare_send_message_to_network_reorder (sctk_thread_ptp_message_t * msg){
  sctk_reorder_table_t* tmp;
  int process;

  /* Process specific messages */
  if(msg->body.header.specific_message_tag == process_specific_message_tag){
    msg->sctk_msg_get_use_message_numbering = 0;
    return 1;
  }

  /* Indirect messages */
  process = sctk_get_process_rank_from_task_rank(msg->sctk_msg_get_glob_source);
  if(sctk_process_rank != process){
    return 0;
  }

  /* Std messages */
  process = sctk_get_process_rank_from_task_rank(msg->sctk_msg_get_glob_destination);
  sctk_nodebug("msg->sctk_msg_get_use_message_numbering %d",msg->sctk_msg_get_use_message_numbering);

  tmp = sctk_get_reorder_to_process(process);

  /* Unable to find local numbering */
  if(tmp == NULL){
    msg->sctk_msg_get_use_message_numbering = 0;
    return 1;
  }

  /* Local numbering */
  msg->sctk_msg_get_use_message_numbering = 1;
  msg->sctk_msg_get_message_number = OPA_fetch_and_incr_int(&(tmp->message_number_dest));
  sctk_nodebug("msg to %d (number:%d)", process,msg->sctk_msg_get_message_number);
  return 0;
}

void sctk_set_dynamic_reordering_buffer_creation(){
  dynamic_reordering_buffer_creation = 1;
}
