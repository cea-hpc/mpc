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
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */

#include <sctk_inter_thread_comm.h>
#include <sctk.h>
#include <sctk_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>

void sctk_perform_messages(sctk_request_t* request){not_implemented();}
sctk_thread_ptp_message_t
* sctk_add_pack_in_message (sctk_thread_ptp_message_t * msg,
			    void *adr, const sctk_count_t nb_items,
			    const size_t elem_size,
			    sctk_pack_indexes_t * begins,
			    sctk_pack_indexes_t * ends){not_implemented();}
sctk_thread_ptp_message_t
* sctk_add_pack_in_message_absolute (sctk_thread_ptp_message_t *
				     msg, void *adr,
				     const sctk_count_t nb_items,
				     const size_t elem_size,
				     sctk_pack_absolute_indexes_t *
				     begins,
				     sctk_pack_absolute_indexes_t * ends){not_implemented();}

void sctk_wait_all (const int task, const sctk_communicator_t com){not_implemented();}
void sctk_probe_source_any_tag (int destination, int source,
				const sctk_communicator_t comm,
				int *status,
				sctk_thread_message_header_t * msg){not_implemented();}
void sctk_probe_any_source_any_tag (int destination,
				    const sctk_communicator_t comm,
				    int *status,
				    sctk_thread_message_header_t * msg){not_implemented();}
void sctk_probe_source_tag (int destination, int source,
			    const sctk_communicator_t comm, int *status,
			    sctk_thread_message_header_t * msg){not_implemented();}
void sctk_probe_any_source_tag (int destination,
				const sctk_communicator_t comm,
				int *status,
				sctk_thread_message_header_t * msg){not_implemented();}

  void sctk_cancel_message (sctk_request_t * msg){not_implemented();}

/********************************************************************/
/*Structres                                                         */
/********************************************************************/

typedef struct{
  sctk_communicator_t comm;
  int destination;
}sctk_comm_dest_key_t;

typedef struct{
  sctk_comm_dest_key_t key;

  sctk_spinlock_t lock;
  /*list used for send/recv messages*/
  sctk_msg_list_t* send_message_list;
  sctk_msg_list_t* recv_message_list;

  UT_hash_handle hh;
} sctk_internal_ptp_t;

/********************************************************************/
/*copy engine                                                       */
/********************************************************************/
static inline void sctk_message_copy(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  assume(send->message_type == recv->message_type);

  switch(send->message_type){
  case sctk_message_contiguous: {
    size_t size;
    size = send->message.contiguous.size;
    if(size > recv->message.contiguous.size){
      size = recv->message.contiguous.size;
    }

    memcpy(recv->message.contiguous.addr,send->message.contiguous.addr,
	   size);
    
    if(recv->request){
      recv->request->header.msg_size = size;
    }
    
    *(recv->completion_flag) = SCTK_MESSAGE_DONE;
    *(send->completion_flag) = SCTK_MESSAGE_DONE;
    
    /*Free messages*/
    send->free_memory(send);
    recv->free_memory(recv);

    break;
  }
  default: not_reachable();
  }
}

/********************************************************************/
/*Task engine                                                       */
/********************************************************************/
#warning "To optimize to deal with NUMA effects"
static sctk_message_to_copy_t* sctk_ptp_task_list = NULL;
static sctk_spinlock_t sctk_ptp_tasks_lock = SCTK_SPINLOCK_INITIALIZER;

static inline void sctk_ptp_tasks_perform(){
  sctk_message_to_copy_t* tmp;

  while(sctk_ptp_task_list != NULL){
    sctk_spinlock_lock(&sctk_ptp_tasks_lock);
    tmp = sctk_ptp_task_list;
    if(tmp != NULL){
      DL_DELETE(sctk_ptp_task_list,tmp);
    }
    sctk_spinlock_unlock(&sctk_ptp_tasks_lock);

    tmp->msg_send->message_copy(tmp);
  }
}

static inline void sctk_ptp_copy_tasks_insert(sctk_msg_list_t* ptr_recv,
					      sctk_msg_list_t* ptr_send){
  sctk_message_to_copy_t* tmp; 

  tmp = &(ptr_recv->msg->copy_list);
  tmp->msg_send = ptr_send->msg;
  tmp->msg_recv = ptr_recv->msg;

  sctk_spinlock_lock(&sctk_ptp_tasks_lock);
  DL_APPEND(sctk_ptp_task_list,tmp); 
  sctk_spinlock_unlock(&sctk_ptp_tasks_lock);
}


/********************************************************************/
/*INIT                                                              */
/********************************************************************/

static sctk_internal_ptp_t* sctk_ptp_table = NULL;
static sctk_spin_rwlock_t sctk_ptp_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

/*Init data structures used for task i*/
void sctk_ptp_per_task_init (int i){
  sctk_internal_ptp_t * tmp;
  tmp = sctk_malloc(sizeof(sctk_internal_ptp_t));
  memset(tmp,0,sizeof(sctk_internal_ptp_t));
  tmp->key.comm = SCTK_COMM_WORLD;
  tmp->key.destination = i;

  tmp->recv_message_list = NULL;
  tmp->send_message_list = NULL;

  sctk_spinlock_write_lock(&sctk_ptp_table_lock);
  HASH_ADD(hh,sctk_ptp_table,key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_spinlock_write_unlock(&sctk_ptp_table_lock);
}

void sctk_unregister_thread (const int i){
  sctk_thread_mutex_lock (&sctk_total_number_of_tasks_lock);
  sctk_total_number_of_tasks--;
  sctk_thread_mutex_unlock (&sctk_total_number_of_tasks_lock);  
}

/********************************************************************/
/*Message creation                                                  */
/********************************************************************/

void sctk_reinit_header (sctk_thread_ptp_message_t *tmp, void (*free_memory)(void*),
		       void (*message_copy)(sctk_message_to_copy_t*)){

  tmp->free_memory = free_memory;
  tmp->message_copy = message_copy;
}

void sctk_init_header (sctk_thread_ptp_message_t *tmp, const int myself,
		       sctk_message_type_t msg_type, void (*free_memory)(void*),
		       void (*message_copy)(sctk_message_to_copy_t*)){

  /*Init message struct*/
  memset(tmp,0,sizeof(sctk_thread_ptp_message_t));
  
  tmp->message_type = msg_type;

  sctk_reinit_header(tmp,free_memory,message_copy);
}

sctk_thread_ptp_message_t *sctk_create_header (const int myself,sctk_message_type_t msg_type){
  sctk_thread_ptp_message_t * tmp;
  
#warning "Optimize allocation"
  tmp = sctk_malloc(sizeof(sctk_thread_ptp_message_t));

  sctk_init_header(tmp,myself,msg_type,sctk_free,sctk_message_copy);

  return tmp;
}

static inline sctk_thread_ptp_message_t
* sctk_add_adress_in_message_contiguous (sctk_thread_ptp_message_t *
			      restrict msg, void *restrict addr,
			      const size_t size){
  msg->message.contiguous.size = size;
  msg->message.contiguous.addr = addr;
  return msg;
}

sctk_thread_ptp_message_t
* sctk_add_adress_in_message (sctk_thread_ptp_message_t *
			      restrict msg, void *restrict addr,
			      const size_t size){
  switch(msg->message_type){
  case sctk_message_contiguous: {
    msg = sctk_add_adress_in_message_contiguous (msg,addr,size);
    break;
  }
  default: not_reachable();
  }
  return msg;
}

void sctk_set_header_in_message (sctk_thread_ptp_message_t *
				 msg, const int message_tag,
				 const sctk_communicator_t
				 communicator,
				 const int source,
				 const int destination,
				 sctk_request_t * request,
				 const size_t count){
  msg->request = request;
  request->msg = msg;

  request->header.source = source;
  request->header.destination = destination;
  request->header.message_tag = message_tag;
  request->header.communicator = communicator;

  msg->completion_flag = &(request->completion_flag);

  msg->header.source = source;
  msg->header.destination = destination;
  msg->header.glob_source = sctk_get_comm_world_rank (communicator,source);
  msg->header.glob_destination = sctk_get_comm_world_rank (communicator,destination);
  msg->header.communicator = communicator;
  msg->header.message_tag = message_tag;
  msg->header.msg_size = count;
}

/********************************************************************/
/*Perform messages                                                  */
/********************************************************************/
static inline 
sctk_msg_list_t* sctk_perform_messages_search_matching(sctk_internal_ptp_t* pair,
					  sctk_thread_message_header_t* header){
  sctk_msg_list_t* res = (sctk_msg_list_t*)(-1);
  if(header->source == MPC_ANY_SOURCE){
    not_implemented();
  } else if((header->message_tag == MPC_ANY_TAG)){
    not_implemented();
  }else if((header->source == MPC_ANY_SOURCE) && (header->message_tag == MPC_ANY_TAG)){
    not_implemented();
  }else {
    sctk_msg_list_t*ptr_send;
    sctk_msg_list_t* tmp;
    res = NULL;
    DL_FOREACH_SAFE(pair->send_message_list,ptr_send,tmp){
      sctk_thread_message_header_t* header_send;
      header_send = &(ptr_send->msg->header);
      sctk_debug("Match? (%d,%d) ?= (%d,%d)",header->source,header->message_tag,
		 header_send->source,header_send->message_tag);
      if((header->source == header_send->source) && 
	 (header->message_tag == header_send->message_tag)){
	DL_DELETE(pair->send_message_list,ptr_send);
	return ptr_send;
      }
    }
  }
  return res;
}
static inline void sctk_perform_messages_for_pair_locked(sctk_internal_ptp_t* pair){ 
  sctk_msg_list_t* ptr_recv;
  sctk_msg_list_t* ptr_send;
  sctk_msg_list_t* tmp;

  DL_FOREACH_SAFE(pair->recv_message_list,ptr_recv,tmp){    
    ptr_send = sctk_perform_messages_search_matching(pair,&(ptr_recv->msg->header));
    if(ptr_send != NULL){
      sctk_debug("Match %p and %p",ptr_recv,ptr_send);
      DL_DELETE(pair->recv_message_list,ptr_recv);

      /*Copy message*/
      sctk_ptp_copy_tasks_insert(ptr_recv,ptr_send);
    }
  }
  sctk_ptp_tasks_perform();
}

static inline void sctk_perform_messages_for_pair(sctk_internal_ptp_t* pair){
    sctk_spinlock_lock(&(pair->lock));
    sctk_perform_messages_for_pair_locked(pair);
    sctk_spinlock_unlock(&(pair->lock));
}

static inline void sctk_try_perform_messages_for_pair(sctk_internal_ptp_t* pair){
  if(sctk_spinlock_trylock(&(pair->lock)) == 0){
    sctk_perform_messages_for_pair_locked(pair);
    sctk_spinlock_unlock(&(pair->lock));
  }
}

void sctk_wait_message (sctk_request_t * request){
  if(request->completion_flag != SCTK_MESSAGE_DONE){
    sctk_comm_dest_key_t key;
    sctk_internal_ptp_t* tmp;
    sctk_thread_ptp_message_t* msg;

    msg = request->msg;
    
    key.comm = msg->header.communicator;
    key.destination = msg->header.glob_destination;
    
    sctk_spinlock_read_lock(&sctk_ptp_table_lock);
    HASH_FIND(hh,sctk_ptp_table,&key,sizeof(sctk_comm_dest_key_t),tmp);
    sctk_spinlock_read_unlock(&sctk_ptp_table_lock);
    
    if(tmp != NULL){
      sctk_try_perform_messages_for_pair(tmp);
    }
    
    sctk_thread_wait_for_value_and_poll
      ((int *) &(msg->completion_flag),SCTK_MESSAGE_DONE ,
       NULL,NULL);
  }
}

/********************************************************************/
/*Send/Recv messages                                                */
/********************************************************************/

void sctk_send_message (sctk_thread_ptp_message_t * msg){
  sctk_comm_dest_key_t key;
  sctk_internal_ptp_t* tmp;

  key.comm = msg->header.communicator;
  key.destination = msg->header.glob_destination;

  if(msg->completion_flag != NULL){
    *(msg->completion_flag) = SCTK_MESSAGE_PENDING;
  }

  sctk_spinlock_read_lock(&sctk_ptp_table_lock);
  HASH_FIND(hh,sctk_ptp_table,&key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_spinlock_read_unlock(&sctk_ptp_table_lock);

  if(tmp != NULL){
    msg->distant_list.msg = msg;

    sctk_spinlock_lock(&(tmp->lock));
    DL_APPEND(tmp->send_message_list, &(msg->distant_list));
    sctk_spinlock_unlock(&(tmp->lock));
  } else {
    not_implemented();
  }
}
void sctk_recv_message (sctk_thread_ptp_message_t * msg){
  sctk_comm_dest_key_t key;
  sctk_internal_ptp_t* tmp;

  key.comm = msg->header.communicator;
  key.destination = msg->header.glob_destination;

  if(msg->completion_flag != NULL){
    *(msg->completion_flag) = SCTK_MESSAGE_PENDING;
  }

  sctk_spinlock_read_lock(&sctk_ptp_table_lock);
  HASH_FIND(hh,sctk_ptp_table,&key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_spinlock_read_unlock(&sctk_ptp_table_lock);

  if(tmp != NULL){
    msg->distant_list.msg = msg;

    sctk_spinlock_lock(&(tmp->lock));
    DL_APPEND(tmp->recv_message_list, &(msg->distant_list));
    sctk_perform_messages_for_pair_locked(tmp);
    sctk_spinlock_unlock(&(tmp->lock));
  } else {
    not_reachable();
  }
}
