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

typedef struct{
  int destination;
}sctk_reorder_key_t;

typedef struct sctk_reorder_table_s{
  sctk_reorder_key_t key;

  OPA_int_t message_number_src;
  OPA_int_t message_number_dest;

  sctk_spinlock_t lock;
  volatile sctk_reorder_buffer_t* buffer;

  UT_hash_handle hh;
} sctk_reorder_table_t;

static sctk_reorder_table_t* sctk_dynamic_reorder_table = NULL;
static sctk_reorder_table_t* sctk_static_reorder_table = NULL;
static sctk_spin_rwlock_t sctk_reorder_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

/*NOT THREAD SAFE use to add a reorder at initialisation time*/
void sctk_add_static_reorder_buffer(int dest){
  sctk_reorder_table_t* tmp;

  tmp = sctk_malloc(sizeof(sctk_reorder_table_t));

  tmp->key.destination = dest;
  OPA_store_int(&(tmp->message_number_src),0);
  OPA_store_int(&(tmp->message_number_dest),0);
  tmp->lock = SCTK_SPINLOCK_INITIALIZER;

  HASH_ADD(hh,sctk_static_reorder_table,key,sizeof(sctk_reorder_key_t),tmp); 
}

/*THREAD SAFE use to add a reorder at compute time*/
void sctk_add_dynamic_reorder_buffer(int dest){ 
  sctk_reorder_table_t* tmp;

  tmp = sctk_malloc(sizeof(sctk_reorder_table_t));

  tmp->key.destination = dest;
  OPA_store_int(&(tmp->message_number_src),0);
  OPA_store_int(&(tmp->message_number_dest),0);
  tmp->lock = SCTK_SPINLOCK_INITIALIZER;

  sctk_spinlock_write_lock(&sctk_reorder_table_lock);
  HASH_ADD(hh,sctk_dynamic_reorder_table,key,sizeof(sctk_reorder_key_t),tmp);
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
    sctk_spinlock_read_lock(&sctk_reorder_table_lock);
  }
  
  return tmp;
}

static 
sctk_reorder_table_t* sctk_get_reorder(int dest){
  sctk_reorder_table_t* tmp;
  int process;

  process = sctk_get_process_rank_from_task_rank(dest);

  tmp = sctk_get_reorder_to_process(process);

  return tmp;
}

int sctk_send_message_from_network_reorder (sctk_thread_ptp_message_t * msg){
  sctk_reorder_table_t* tmp;
  int number;
  
  tmp = sctk_get_reorder(msg->sctk_msg_get_glob_source);

  if(msg->sctk_msg_get_use_message_numbering){
    assume(tmp != NULL);
    
    number = OPA_load_int(&(tmp->message_number_src));
    sctk_nodebug("wait for %d recv %d",number,msg->sctk_msg_get_message_number);
    
    if(number == msg->sctk_msg_get_message_number){
      sctk_nodebug("Send %d",msg->sctk_msg_get_message_number);
      sctk_send_message(msg);
      OPA_fetch_and_incr_int(&(tmp->message_number_src));
      msg = NULL;
      
      /*Search for pending messages*/
      if(tmp->buffer != NULL){
	sctk_reorder_buffer_t* reorder;
	int key;
	
	sctk_nodebug("Proceed pending messages");
	
	sctk_spinlock_lock(&(tmp->lock));
	do{
	  key = OPA_load_int(&(tmp->message_number_src));
	  HASH_FIND(hh,tmp->buffer,&key,sizeof(int),reorder);
	  if(reorder != NULL){
	    sctk_nodebug("Send %d",reorder->msg->sctk_msg_get_message_number);
	    sctk_send_message(reorder->msg);
	    OPA_fetch_and_incr_int(&(tmp->message_number_src));
	  }
	} while(reorder != NULL);
	sctk_spinlock_unlock(&(tmp->lock));
      }
    } else {
      sctk_reorder_buffer_t* reorder;
      
      sctk_nodebug("Delay wait for %d recv %d",number,msg->sctk_msg_get_message_number);
      
      reorder = &(msg->tail.reorder);
      
      reorder->key = msg->sctk_msg_get_message_number;
      reorder->msg = msg;
      
      sctk_spinlock_lock(&(tmp->lock));
      HASH_ADD(hh,tmp->buffer,key,sizeof(int),reorder);    
      sctk_spinlock_unlock(&(tmp->lock));    
    }
    return 0;
  } else {
    return 1;
  }
}

int sctk_prepare_send_message_to_network_reorder (sctk_thread_ptp_message_t * msg){
  sctk_reorder_table_t* tmp;

  if(msg->sctk_msg_get_use_message_numbering == 0){
    return 1;
  }
  tmp = sctk_get_reorder(msg->sctk_msg_get_glob_destination);
  if(tmp == NULL){
    msg->sctk_msg_get_use_message_numbering = 0;
    return 1;
  }
  msg->sctk_msg_get_use_message_numbering = 1;
  msg->sctk_msg_get_message_number = OPA_fetch_and_incr_int(&(tmp->message_number_dest));
  return 0;
}
