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
#include <sctk_low_level_comm.h>
#include <sctk.h>
#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <sctk_asm.h>

#define SCTK_DISABLE_REENTRANCE

  void sctk_cancel_message (sctk_request_t * msg){not_implemented();}

/********************************************************************/
/*Structres                                                         */
/********************************************************************/

typedef struct{
/*   sctk_communicator_t comm; */
  int destination;
}sctk_comm_dest_key_t;


typedef struct {
  sctk_spinlock_t lock;
  volatile sctk_msg_list_t* list;
} sctk_internal_ptp_list_incomming_t;

typedef struct {
  sctk_msg_list_t* list;
} sctk_internal_ptp_list_pending_t;

static inline void sctk_internal_ptp_list_incomming_init(sctk_internal_ptp_list_incomming_t* list){
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;

  list->list = NULL;
  list->lock = lock;
}
static inline void sctk_internal_ptp_list_pending_init(sctk_internal_ptp_list_pending_t* list){
  list->list = NULL;
}
typedef struct {
#ifndef SCTK_DISABLE_REENTRANCE
  sctk_internal_ptp_list_incomming_t incomming_send;
  sctk_internal_ptp_list_incomming_t incomming_recv;
#endif

  sctk_spinlock_t pending_lock;
  sctk_internal_ptp_list_pending_t pending_send;
  sctk_internal_ptp_list_pending_t pending_recv;
}sctk_internal_ptp_message_lists_t;

static inline void sctk_internal_ptp_message_list_init(sctk_internal_ptp_message_lists_t * lists){
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
#ifndef SCTK_DISABLE_REENTRANCE
  sctk_internal_ptp_list_incomming_init(&(lists->incomming_send));
  sctk_internal_ptp_list_incomming_init(&(lists->incomming_recv));
#endif
  
  lists->pending_lock = lock;
  sctk_internal_ptp_list_pending_init(&(lists->pending_send));
  sctk_internal_ptp_list_pending_init(&(lists->pending_recv));
}

typedef struct{
  sctk_comm_dest_key_t key;

  sctk_internal_ptp_message_lists_t lists;

  UT_hash_handle hh;
} sctk_internal_ptp_t;

static inline void sctk_internal_ptp_merge_pending(sctk_internal_ptp_message_lists_t* lists){
#ifndef SCTK_DISABLE_REENTRANCE
  if(lists->incomming_send.list != NULL){
    sctk_spinlock_lock(&(lists->incomming_send.lock));
    DL_CONCAT(lists->pending_send.list,(sctk_msg_list_t*)lists->incomming_send.list);
    lists->incomming_send.list = NULL;
    sctk_spinlock_unlock(&(lists->incomming_send.lock));
  }
  if(lists->incomming_recv.list != NULL){
    sctk_spinlock_lock(&(lists->incomming_recv.lock));
    DL_CONCAT(lists->pending_recv.list,(sctk_msg_list_t*)lists->incomming_recv.list);
    lists->incomming_recv.list = NULL;
    sctk_spinlock_unlock(&(lists->incomming_recv.lock));
  }
#endif
}

static inline void sctk_internal_ptp_lock_pending(sctk_internal_ptp_message_lists_t* lists){
  sctk_spinlock_lock(&(lists->pending_lock));
}

static inline int sctk_internal_ptp_trylock_pending(sctk_internal_ptp_message_lists_t* lists){
  return sctk_spinlock_trylock(&(lists->pending_lock));
}

static inline void sctk_internal_ptp_unlock_pending(sctk_internal_ptp_message_lists_t* lists){
  sctk_spinlock_unlock(&(lists->pending_lock));  
}

#ifndef SCTK_DISABLE_REENTRANCE
static inline void sctk_internal_ptp_add_recv_incomming(sctk_internal_ptp_t* tmp,
							sctk_thread_ptp_message_t * msg){
    msg->tail.distant_list.msg = msg;
    sctk_spinlock_lock(&(tmp->lists.incomming_recv.lock));
    DL_APPEND(tmp->lists.incomming_recv.list, &(msg->tail.distant_list));
    sctk_spinlock_unlock(&(tmp->lists.incomming_recv.lock));
}

static inline void sctk_internal_ptp_add_send_incomming(sctk_internal_ptp_t* tmp,
							sctk_thread_ptp_message_t * msg){
    msg->tail.distant_list.msg = msg;
    sctk_spinlock_lock(&(tmp->lists.incomming_send.lock));
    DL_APPEND(tmp->lists.incomming_send.list, &(msg->tail.distant_list));
    sctk_spinlock_unlock(&(tmp->lists.incomming_send.lock));
}
#else
#warning "Use blocking version of send/recv message"
static inline void sctk_internal_ptp_add_recv_incomming(sctk_internal_ptp_t* tmp,
							sctk_thread_ptp_message_t * msg){
    msg->tail.distant_list.msg = msg;
    sctk_internal_ptp_lock_pending(&(tmp->lists));
    DL_APPEND(tmp->lists.pending_recv.list, &(msg->tail.distant_list));
    sctk_internal_ptp_unlock_pending(&(tmp->lists));
}

static inline void sctk_internal_ptp_add_send_incomming(sctk_internal_ptp_t* tmp,
							sctk_thread_ptp_message_t * msg){
    msg->tail.distant_list.msg = msg;
    sctk_internal_ptp_lock_pending(&(tmp->lists));
    DL_APPEND(tmp->lists.pending_send.list, &(msg->tail.distant_list));
    sctk_internal_ptp_unlock_pending(&(tmp->lists));
}

#endif

/************************************************************************/
/*Data structure accessors                                              */
/************************************************************************/
static sctk_internal_ptp_t* sctk_ptp_table = NULL;
static sctk_spin_rwlock_t sctk_ptp_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

static inline void
sctk_ptp_table_write_lock(){
  if(sctk_migration_mode)
    sctk_spinlock_write_lock(&sctk_ptp_table_lock);
}

static inline void
sctk_ptp_table_write_unlock(){
  if(sctk_migration_mode)
    sctk_spinlock_write_unlock(&sctk_ptp_table_lock);
}

static inline void
sctk_ptp_table_read_lock(){
  if(sctk_migration_mode)
    sctk_spinlock_read_lock(&sctk_ptp_table_lock);
}

static inline void
sctk_ptp_table_read_unlock(){
  if(sctk_migration_mode)
    sctk_spinlock_read_unlock(&sctk_ptp_table_lock);
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

    if(tmp != NULL){
      tmp->msg_send->tail.message_copy(tmp);
    }
  }
}

static inline void sctk_ptp_copy_tasks_insert(sctk_msg_list_t* ptr_recv,
					      sctk_msg_list_t* ptr_send){
  sctk_message_to_copy_t* tmp; 

  tmp = &(ptr_recv->msg->tail.copy_list);
  tmp->msg_send = ptr_send->msg;
  tmp->msg_recv = ptr_recv->msg;

  sctk_spinlock_lock(&sctk_ptp_tasks_lock);
  DL_APPEND(sctk_ptp_task_list,tmp); 
  sctk_spinlock_unlock(&sctk_ptp_tasks_lock);
}


/********************************************************************/
/*copy engine                                                       */
/********************************************************************/

void sctk_complete_and_free_message (sctk_thread_ptp_message_t * msg){
  if(msg->body.completion_flag)
    *(msg->body.completion_flag) = SCTK_MESSAGE_DONE;
  msg->tail.free_memory(msg);  
}

void sctk_message_completion_and_free(sctk_thread_ptp_message_t* send,
				     sctk_thread_ptp_message_t* recv){
  size_t size; 

  if(recv->tail.request){
    size = send->body.header.msg_size;
    if(recv->tail.request->header.msg_size > size){
      recv->tail.request->header.msg_size = size;
    }
  
    recv->tail.request->header.source = send->body.header.source;
    recv->tail.request->header.message_tag = send->body.header.message_tag;
    recv->tail.request->header.msg_size = size;
  }

  sctk_complete_and_free_message(send);
  sctk_complete_and_free_message(recv);
}

inline void sctk_message_copy(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  assume(send->tail.message_type == recv->tail.message_type);

  switch(send->tail.message_type){
  case sctk_message_contiguous: {
    size_t size;
    size = send->tail.message.contiguous.size;
    if(size > recv->tail.message.contiguous.size){
      size = recv->tail.message.contiguous.size;
    }

    memcpy(recv->tail.message.contiguous.addr,send->tail.message.contiguous.addr,
	   size);
    
    sctk_message_completion_and_free(send,recv);
    break;
  }
  default: not_reachable();
  }
}

static inline void
sctk_copy_buffer_std_std (sctk_pack_indexes_t * restrict in_begins,
			  sctk_pack_indexes_t * restrict in_ends,
			  size_t in_sizes,
			  void *restrict in_adress,
			  size_t in_elem_size,
			  sctk_pack_indexes_t * restrict out_begins,
			  sctk_pack_indexes_t * restrict out_ends,
			  size_t out_sizes,
			  void *restrict out_adress, size_t out_elem_size)
{
  sctk_pack_indexes_t tmp_begin[1];
  sctk_pack_indexes_t tmp_end[1];
  if ((in_begins == NULL) && (out_begins == NULL))
    {
      sctk_nodebug ("sctk_copy_buffer_std_std no mpc_pack");
      sctk_nodebug ("%s == %s", out_adress, in_adress);
      memcpy (out_adress, in_adress, in_sizes);
      sctk_nodebug ("%s == %s", out_adress, in_adress);
    }
  else
    {
      unsigned long i;
      unsigned long j;
      unsigned long in_i;
      unsigned long in_j;
      sctk_nodebug ("sctk_copy_buffer_std_std mpc_pack");
      if (in_begins == NULL)
	{
	  in_begins = tmp_begin;
	  in_begins[0] = 0;
	  in_ends = tmp_end;
	  in_ends[0] = in_sizes - 1;
	  in_elem_size = 1;
	  in_sizes = 1;
	}
      if (out_begins == NULL)
	{
	  out_begins = tmp_begin;
	  out_begins[0] = 0;
	  out_ends = tmp_end;
	  out_ends[0] = out_sizes - 1;
	  out_elem_size = 1;
	  out_sizes = 1;
	}
      in_i = 0;
      in_j = in_begins[in_i] * in_elem_size;
      for (i = 0; i < out_sizes; i++)
	{
	  for (j = out_begins[i] * out_elem_size;
	       j <= out_ends[i] * out_elem_size;)
	    {
	      size_t max_length;
	      if (in_j > in_ends[in_i] * in_elem_size)
		{
		  in_i++;
		  if (in_i >= in_sizes)
		    {
		      return;
		    }
		  in_j = in_begins[in_i] * in_elem_size;
		}

	      max_length =
		sctk_min ((out_ends[i] * out_elem_size - j +
			   out_elem_size),
			  (in_ends[in_i] * in_elem_size - in_j +
			   in_elem_size));

	      memcpy (&(((char *) out_adress)[j]),
		      &(((char *) in_adress)[in_j]), max_length);
	      sctk_nodebug ("Copy out[%d-%d]%s == in[%d-%d]%s", j,
			    j + max_length, &(((char *) out_adress)[j]),
			    in_j, in_j + max_length,
			    &(((char *) in_adress)[in_j]));

	      j += max_length;
	      in_j += max_length;

	    }
	}
    }
}

static inline void
sctk_copy_buffer_absolute_absolute (sctk_pack_absolute_indexes_t *
				    restrict in_begins,
				    sctk_pack_absolute_indexes_t *
				    restrict in_ends, size_t in_sizes,
				    void *restrict in_adress,
				    size_t in_elem_size,
				    sctk_pack_absolute_indexes_t *
				    restrict out_begins,
				    sctk_pack_absolute_indexes_t *
				    restrict out_ends, size_t out_sizes,
				    void *restrict out_adress,
				    size_t out_elem_size)
{
  sctk_pack_absolute_indexes_t tmp_begin[1];
  sctk_pack_absolute_indexes_t tmp_end[1];
  if ((in_begins == NULL) && (out_begins == NULL))
    {
      sctk_nodebug ("sctk_copy_buffer_absolute_absolute no mpc_pack");
      sctk_nodebug ("%s == %s", out_adress, in_adress);
      memcpy (out_adress, in_adress, in_sizes);
      sctk_nodebug ("%s == %s", out_adress, in_adress);
    }
  else
    {
      unsigned long i;
      unsigned long j;
      unsigned long in_i;
      unsigned long in_j;
      sctk_nodebug ("sctk_copy_buffer_absolute_absolute mpc_pack");
      if (in_begins == NULL)
	{
	  in_begins = tmp_begin;
	  in_begins[0] = 0;
	  in_ends = tmp_end;
	  in_ends[0] = in_sizes - 1;
	  in_elem_size = 1;
	  in_sizes = 1;
	}
      if (out_begins == NULL)
	{
	  out_begins = tmp_begin;
	  out_begins[0] = 0;
	  out_ends = tmp_end;
	  out_ends[0] = out_sizes - 1;
	  out_elem_size = 1;
	  out_sizes = 1;
	}
      in_i = 0;
      in_j = in_begins[in_i] * in_elem_size;

      for (i = 0; i < out_sizes; i++)
	{
	  for (j = out_begins[i] * out_elem_size;
	       j <= out_ends[i] * out_elem_size;)
	    {
	      size_t max_length;
	      if (in_j > in_ends[in_i] * in_elem_size)
		{
		  in_i++;
		  if (in_i >= in_sizes)
		    {
		      return;
		    }
		  in_j = in_begins[in_i] * in_elem_size;
		}

	      max_length =
		sctk_min ((out_ends[i] * out_elem_size - j +
			   out_elem_size),
			  (in_ends[in_i] * in_elem_size - in_j +
			   in_elem_size));

	      sctk_nodebug ("Copy out[%lu-%lu]%p == in[%lu-%lu]%p", j,
			    j + max_length, &(((char *) out_adress)[j]),
			    in_j, in_j + max_length,
			    &(((char *) in_adress)[in_j]));
	      memcpy (&(((char *) out_adress)[j]),
		      &(((char *) in_adress)[in_j]), max_length);
	      sctk_nodebug ("Copy out[%d-%d]%d == in[%d-%d]%d", j,
			    j + max_length, (((char *) out_adress)[j]),
			    in_j, in_j + max_length,
			    (((char *) in_adress)[in_j]));

	      j += max_length;
	      in_j += max_length;
	    }
	}
    }
}

inline void sctk_message_copy_pack(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  assume(send->tail.message_type == recv->tail.message_type);

  switch(send->tail.message_type){
  case sctk_message_pack: {
    size_t size;
    size_t i;
    for (i = 0; i < send->tail.message.pack.count; i++)
      {	
	sctk_copy_buffer_std_std (send->tail.message.pack.list.std[i].begins,
				  send->tail.message.pack.list.std[i].ends,
				  send->tail.message.pack.list.std[i].count,
				  send->tail.message.pack.list.std[i].addr,
				  send->tail.message.pack.list.std[i].elem_size,
				  recv->tail.message.pack.list.std[i].begins,
				  recv->tail.message.pack.list.std[i].ends,
				  recv->tail.message.pack.list.std[i].count,
				  recv->tail.message.pack.list.std[i].addr,
				  recv->tail.message.pack.list.std[i].elem_size); 
      }

    sctk_message_completion_and_free(send,recv);
    break;
  }
  default: not_reachable();
  }
}

inline void sctk_message_copy_pack_absolute(sctk_message_to_copy_t* tmp){
  sctk_thread_ptp_message_t* send;
  sctk_thread_ptp_message_t* recv;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  assume(send->tail.message_type == recv->tail.message_type);

  switch(send->tail.message_type){
  case sctk_message_pack_absolute: {
    size_t size;
    size_t i;
    for (i = 0; i < send->tail.message.pack.count; i++)
      {	
	sctk_copy_buffer_absolute_absolute (send->tail.message.pack.list.absolute[i].begins,
					    send->tail.message.pack.list.absolute[i].ends,
					    send->tail.message.pack.list.absolute[i].count,
					    send->tail.message.pack.list.absolute[i].addr,
					    send->tail.message.pack.list.absolute[i].elem_size,
					    recv->tail.message.pack.list.absolute[i].begins,
					    recv->tail.message.pack.list.absolute[i].ends,
					    recv->tail.message.pack.list.absolute[i].count,
					    recv->tail.message.pack.list.absolute[i].addr,
					    recv->tail.message.pack.list.absolute[i].elem_size); 
      }

    sctk_message_completion_and_free(send,recv);
    break;
  }
  default: not_reachable();
  }
}

/********************************************************************/
/*INIT                                                              */
/********************************************************************/

/*Init data structures used for task i*/
void sctk_ptp_per_task_init (int i){
  static sctk_spinlock_t lock;
  sctk_internal_ptp_t * tmp;

  sctk_spinlock_lock(&lock);
  tmp = sctk_malloc(sizeof(sctk_internal_ptp_t));
  memset(tmp,0,sizeof(sctk_internal_ptp_t));
/*   tmp->key.comm = SCTK_COMM_WORLD; */
  tmp->key.destination = i;

  sctk_internal_ptp_message_list_init(&(tmp->lists));

  sctk_ptp_table_write_lock(&sctk_ptp_table_lock);
  HASH_ADD(hh,sctk_ptp_table,key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_ptp_table_write_unlock(&sctk_ptp_table_lock);
  sctk_spinlock_unlock(&lock);
}

void sctk_unregister_thread (const int i){
  sctk_thread_mutex_lock (&sctk_total_number_of_tasks_lock);
  sctk_total_number_of_tasks--;
  sctk_thread_mutex_unlock (&sctk_total_number_of_tasks_lock);  
}

/********************************************************************/
/*Message creation                                                  */
/********************************************************************/

void sctk_free_pack(void*); 

void sctk_reinit_header (sctk_thread_ptp_message_t *tmp, void (*free_memory)(void*),
		       void (*message_copy)(sctk_message_to_copy_t*)){

  tmp->tail.free_memory = free_memory;
  tmp->tail.message_copy = message_copy;
}

void sctk_init_header (sctk_thread_ptp_message_t *tmp, const int myself,
		       sctk_message_type_t msg_type, void (*free_memory)(void*),
		       void (*message_copy)(sctk_message_to_copy_t*)){

  /*Init message struct*/
  memset(tmp,0,sizeof(sctk_thread_ptp_message_t));
  
  tmp->tail.message_type = msg_type;

  sctk_reinit_header(tmp,free_memory,message_copy);
  if(tmp->tail.message_type == sctk_message_pack){
    sctk_reinit_header(tmp,sctk_free_pack,sctk_message_copy_pack);
  }
  if(tmp->tail.message_type == sctk_message_pack_absolute){
    sctk_reinit_header(tmp,sctk_free_pack,sctk_message_copy_pack_absolute);
  }
}

sctk_thread_ptp_message_t *sctk_create_header (const int myself,sctk_message_type_t msg_type){
  sctk_thread_ptp_message_t * tmp;
  
#warning "Optimize allocation"
  tmp = sctk_malloc(sizeof(sctk_thread_ptp_message_t));

  sctk_init_header(tmp,myself,msg_type,sctk_free,sctk_message_copy);

  return tmp;
}

static inline void
sctk_add_adress_in_message_contiguous (sctk_thread_ptp_message_t *
			      restrict msg, void *restrict addr,
			      const size_t size){
  msg->tail.message.contiguous.size = size;
  msg->tail.message.contiguous.addr = addr;
}

void
sctk_add_adress_in_message (sctk_thread_ptp_message_t *
			      restrict msg, void *restrict addr,
			      const size_t size){
  switch(msg->tail.message_type){
  case sctk_message_contiguous: {
    sctk_add_adress_in_message_contiguous (msg,addr,size);
    break;
  }
  default: not_reachable();
  }
}

void sctk_set_header_in_message (sctk_thread_ptp_message_t *
				 msg, const int message_tag,
				 const sctk_communicator_t
				 communicator,
				 const int source,
				 const int destination,
				 sctk_request_t * request,
				 const size_t count,
				 specific_message_tag_t specific_message_tag){
  msg->tail.request = request;

  msg->body.header.source = source;
  msg->body.header.destination = destination;

  if(source != MPC_ANY_SOURCE)
    msg->body.header.glob_source = sctk_get_comm_world_rank (communicator,source);
  else 
    msg->body.header.glob_source = -1;

  msg->body.header.glob_destination = sctk_get_comm_world_rank (communicator,destination);
  msg->body.header.communicator = communicator;
  msg->body.header.message_tag = message_tag;
  msg->body.header.specific_message_tag = specific_message_tag;
  msg->body.header.msg_size = count;

  if(request != NULL){
    request->msg = msg;
    
    request->header.source = source;
    request->header.destination = destination;
    request->header.glob_destination = msg->body.header.glob_destination;
    request->header.glob_source = msg->body.header.glob_source;
    request->header.message_tag = message_tag;
    request->header.communicator = communicator;
    request->is_null = 0;
    request->completion_flag = SCTK_MESSAGE_PENDING;

    msg->body.completion_flag = &(request->completion_flag);
  }
}

/********************************************************************/
/*Message pack creation                                             */
/********************************************************************/
#define SCTK_PACK_REALLOC_STEP 10
void sctk_free_pack(void* tmp){
  sctk_thread_ptp_message_t * msg; 

  msg = tmp;

  if(msg->tail.message_type == sctk_message_pack_absolute){
    sctk_free(msg->tail.message.pack.list.absolute);
  } else {
    sctk_free(msg->tail.message.pack.list.std);
  }
  sctk_free(tmp);
}

void
sctk_add_pack_in_message (sctk_thread_ptp_message_t * msg,
			  void *adr, const sctk_count_t nb_items,
			  const size_t elem_size,
			  sctk_pack_indexes_t * begins,
			  sctk_pack_indexes_t * ends){
  int step;
  if(msg->tail.message_type == sctk_message_pack_undefined){
    msg->tail.message_type = sctk_message_pack;
    sctk_reinit_header(msg,sctk_free_pack,sctk_message_copy_pack);
  }
  assume(msg->tail.message_type == sctk_message_pack);

  if(msg->tail.message.pack.count >= msg->tail.message.pack.max_count){
    msg->tail.message.pack.max_count += SCTK_PACK_REALLOC_STEP;
    msg->tail.message.pack.list.std = sctk_realloc(msg->tail.message.pack.list.std,
		 msg->tail.message.pack.max_count*sizeof(sctk_message_pack_std_list_t));
  }

  step = msg->tail.message.pack.count;

  msg->tail.message.pack.list.std[step].count = nb_items;
  msg->tail.message.pack.list.std[step].begins = begins;
  msg->tail.message.pack.list.std[step].ends = ends;
  msg->tail.message.pack.list.std[step].addr = adr;
  msg->tail.message.pack.list.std[step].elem_size = elem_size;

  msg->tail.message.pack.count++;
}
void
sctk_add_pack_in_message_absolute (sctk_thread_ptp_message_t *
				   msg, void *adr,
				   const sctk_count_t nb_items,
				   const size_t elem_size,
				   sctk_pack_absolute_indexes_t *
				   begins,
				   sctk_pack_absolute_indexes_t * ends){
  int step;
  if(msg->tail.message_type == sctk_message_pack_undefined){
    msg->tail.message_type = sctk_message_pack_absolute;
    sctk_reinit_header(msg,sctk_free_pack,sctk_message_copy_pack_absolute);
  }

  assume(msg->tail.message_type == sctk_message_pack_absolute);

  if(msg->tail.message.pack.count >= msg->tail.message.pack.max_count){
    msg->tail.message.pack.max_count += SCTK_PACK_REALLOC_STEP;
    msg->tail.message.pack.list.absolute = sctk_realloc(msg->tail.message.pack.list.absolute,
		 msg->tail.message.pack.max_count*sizeof(sctk_message_pack_absolute_list_t));
  }

  step = msg->tail.message.pack.count;

  msg->tail.message.pack.list.absolute[step].count = nb_items;
  msg->tail.message.pack.list.absolute[step].begins = begins;
  msg->tail.message.pack.list.absolute[step].ends = ends;
  msg->tail.message.pack.list.absolute[step].addr = adr;
  msg->tail.message.pack.list.absolute[step].elem_size = elem_size;

  msg->tail.message.pack.count++;
}

/********************************************************************/
/*Perform messages                                                  */
/********************************************************************/
static inline 
sctk_msg_list_t* sctk_perform_messages_search_matching(sctk_internal_ptp_t* pair,
					  sctk_thread_message_header_t* header){
  sctk_msg_list_t* res = NULL;
  sctk_msg_list_t* ptr_send;
  sctk_msg_list_t* tmp;
  res = NULL;
  DL_FOREACH_SAFE(pair->lists.pending_send.list,ptr_send,tmp){
    sctk_thread_message_header_t* header_send; 
    sctk_assert(ptr_send->msg != NULL);
    header_send = &(ptr_send->msg->body.header);
    if((header->communicator == header_send->communicator) && 
       (header->specific_message_tag == header_send->specific_message_tag) &&
       ((header->source == header_send->source) || (header->source == MPC_ANY_SOURCE))&& 
       ((header->message_tag == header_send->message_tag) || (header->message_tag == MPC_ANY_TAG))){
      DL_DELETE(pair->lists.pending_send.list,ptr_send);
      sctk_nodebug("Match? dest %d (%d,%d,%d) == (%d,%d,%d)",
		 header->destination,
		 header->source,header->message_tag,header->specific_message_tag,
		 header_send->source,header_send->message_tag,header_send->specific_message_tag);
      return ptr_send;
    } 
  }
  return res;
}
static inline 
int sctk_perform_messages_probe_matching(sctk_internal_ptp_t* pair,
					  sctk_thread_message_header_t* header){
  sctk_msg_list_t* res = NULL;
  sctk_msg_list_t* ptr_send;
  sctk_msg_list_t* tmp;
  int remote;
  res = NULL;
  sctk_internal_ptp_merge_pending(&(pair->lists));

  DL_FOREACH_SAFE(pair->lists.pending_send.list,ptr_send,tmp){
    sctk_thread_message_header_t* header_send; 
    sctk_assert(ptr_send->msg != NULL);
    header_send = &(ptr_send->msg->body.header);
    if((header->communicator == header_send->communicator) && 
       (header->specific_message_tag == header_send->specific_message_tag) &&
       ((header->source == header_send->source) || (header->source == MPC_ANY_SOURCE))&& 
       ((header->message_tag == header_send->message_tag) || (header->message_tag == MPC_ANY_TAG))){
      memcpy(header,&(ptr_send->msg->body.header),sizeof(sctk_thread_message_header_t));
      return 1;
    } 
  }

  if(header->source == MPC_ANY_SOURCE){
    sctk_network_notify_any_source_message ();
  } else {
    int remote;
    remote = sctk_get_comm_world_rank (header->communicator,header->source);
    if(remote != sctk_process_rank){
      sctk_network_notify_perform_message (remote);
    }
  }
  return 0;
}
static inline void sctk_perform_messages_for_pair_locked(sctk_internal_ptp_t* pair){ 
  sctk_msg_list_t* ptr_recv;
  sctk_msg_list_t* ptr_send;
  sctk_msg_list_t* tmp;

  sctk_internal_ptp_merge_pending(&(pair->lists));

  DL_FOREACH_SAFE(pair->lists.pending_recv.list,ptr_recv,tmp){  
    sctk_assert(ptr_recv->msg != NULL);
    ptr_send = sctk_perform_messages_search_matching(pair,&(ptr_recv->msg->body.header));
    if(ptr_send != NULL){
      sctk_nodebug("Match %p and %p",ptr_recv,ptr_send);
      DL_DELETE(pair->lists.pending_recv.list,ptr_recv);

      /*Copy message*/
      sctk_ptp_copy_tasks_insert(ptr_recv,ptr_send);
    } else {
      if(ptr_recv->msg->body.header.remote_source){
	sctk_network_notify_matching_message (ptr_recv->msg);
      }
      if(ptr_recv->msg->body.header.source == MPC_ANY_SOURCE){
	sctk_network_notify_any_source_message ();
      }
    }
  }
}

static inline void sctk_perform_messages_for_pair(sctk_internal_ptp_t* pair){
  sctk_internal_ptp_lock_pending(&(pair->lists));
  sctk_perform_messages_for_pair_locked(pair);
  sctk_internal_ptp_unlock_pending(&(pair->lists));
  sctk_ptp_tasks_perform();
}

static inline void sctk_try_perform_messages_for_pair(sctk_internal_ptp_t* pair){
/*   if(sctk_internal_ptp_trylock_pending(&(pair->lists)) == 0){ */
/*     sctk_perform_messages_for_pair_locked(pair); */
/*     sctk_internal_ptp_unlock_pending(&(pair->lists)); */
/*     sctk_ptp_tasks_perform(); */
/*   } */
  sctk_perform_messages_for_pair(pair);
}

void sctk_wait_message (sctk_request_t * request){
  sctk_perform_messages(request);

  if(request->completion_flag != SCTK_MESSAGE_DONE){
    sctk_thread_wait_for_value_and_poll
      ((int *) &(request->completion_flag),SCTK_MESSAGE_DONE ,
       (void(*)(void*))sctk_perform_messages,(void*)request);
  }
}

void sctk_perform_messages(sctk_request_t* request){
  if(request->completion_flag != SCTK_MESSAGE_DONE){
    sctk_comm_dest_key_t key;
    sctk_internal_ptp_t* tmp;
    sctk_comm_dest_key_t send_key;
    sctk_internal_ptp_t* send_tmp;
    
/*     key.comm = request->header.communicator; */
    key.destination = request->header.glob_destination;
    send_key.destination = request->header.glob_source;
    
    sctk_ptp_table_read_lock(&sctk_ptp_table_lock);
    HASH_FIND(hh,sctk_ptp_table,&key,sizeof(sctk_comm_dest_key_t),tmp);
    HASH_FIND(hh,sctk_ptp_table,&send_key,sizeof(sctk_comm_dest_key_t),send_tmp);
    sctk_ptp_table_read_unlock(&sctk_ptp_table_lock);
    
    if(send_tmp == NULL){
      sctk_network_notify_perform_message (request->header.glob_source);
    }

    if(tmp != NULL){
      sctk_try_perform_messages_for_pair(tmp);
    } else {
      sctk_network_notify_perform_message (request->header.glob_destination);
    }
  }
}

void sctk_wait_all (const int task, const sctk_communicator_t com){
  sctk_internal_ptp_t* pair;
  sctk_internal_ptp_t* tmp;
  int i; 

  do{
    i = 0;
    sctk_ptp_table_read_lock(&sctk_ptp_table_lock);
    HASH_ITER(hh,sctk_ptp_table,pair,tmp){
      sctk_msg_list_t* ptr;
      sctk_internal_ptp_lock_pending(&(pair->lists));
      sctk_perform_messages_for_pair_locked(pair);
      DL_FOREACH(pair->lists.pending_recv.list,ptr){
	if((ptr->msg->body.header.destination == task) && 
	   (ptr->msg->body.header.communicator == com)){
	  i++;
	}
      }
      DL_FOREACH(pair->lists.pending_send.list,ptr){
	if((ptr->msg->body.header.source == task) && 
	   (ptr->msg->body.header.communicator == com)){
	  i++;
	}
      }
      sctk_internal_ptp_unlock_pending(&(pair->lists));
    }
    sctk_ptp_table_read_unlock(&sctk_ptp_table_lock);
    sctk_ptp_tasks_perform();

#warning "To optimize"
    sctk_thread_yield();
  } while(i != 0);
}

void sctk_perform_all (){
  sctk_internal_ptp_t* pair;
  sctk_internal_ptp_t* tmp;

  sctk_ptp_table_read_lock(&sctk_ptp_table_lock);
  HASH_ITER(hh,sctk_ptp_table,pair,tmp){
    sctk_try_perform_messages_for_pair(pair);
  }
  sctk_ptp_table_read_unlock(&sctk_ptp_table_lock);
}

void sctk_notify_idle_message (){
  sctk_perform_all ();
  if(sctk_process_number > 1){
    sctk_network_notify_idle_message ();
  }
}
/********************************************************************/
/*Send/Recv messages                                                */
/********************************************************************/

void sctk_send_message (sctk_thread_ptp_message_t * msg){
  sctk_comm_dest_key_t key;
  sctk_internal_ptp_t* tmp;

/*   key.comm = msg->header.communicator; */
  key.destination = msg->body.header.glob_destination;

  if(msg->body.completion_flag != NULL){
    *(msg->body.completion_flag) = SCTK_MESSAGE_PENDING;
  }

  msg->body.header.remote_source = 0;
  msg->body.header.remote_destination = 0;

  sctk_ptp_table_read_lock(&sctk_ptp_table_lock);
  HASH_FIND(hh,sctk_ptp_table,&key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_ptp_table_read_unlock(&sctk_ptp_table_lock);

  if(tmp != NULL){
    sctk_internal_ptp_add_send_incomming(tmp,msg);
  } else {
    /*Entering low level comm*/
    msg->body.header.remote_destination = 1;
    sctk_network_send_message (msg);
  }
}
void sctk_recv_message (sctk_thread_ptp_message_t * msg){
  sctk_comm_dest_key_t key;
  sctk_internal_ptp_t* tmp;
  sctk_comm_dest_key_t send_key;
  sctk_internal_ptp_t* send_tmp = NULL;

/*   key.comm = msg->header.communicator; */
  key.destination = msg->body.header.glob_destination;
  send_key.destination = msg->body.header.glob_source;

  if(msg->body.completion_flag != NULL){
    *(msg->body.completion_flag) = SCTK_MESSAGE_PENDING;
  }

  msg->body.header.remote_source = 0;
  msg->body.header.remote_destination = 0;

  sctk_ptp_table_read_lock(&sctk_ptp_table_lock);
  HASH_FIND(hh,sctk_ptp_table,&key,sizeof(sctk_comm_dest_key_t),tmp);
  if(send_key.destination != -1)
    HASH_FIND(hh,sctk_ptp_table,&send_key,sizeof(sctk_comm_dest_key_t),send_tmp);
  sctk_ptp_table_read_unlock(&sctk_ptp_table_lock);

  if(send_tmp == NULL){
    /*Entering low level comm*/
    msg->body.header.remote_source = 1;
    sctk_network_notify_recv_message (msg);    
  }

  if(tmp != NULL){
    sctk_internal_ptp_add_recv_incomming(tmp,msg);
    sctk_try_perform_messages_for_pair(tmp);
  } else {
    not_reachable();
  }

}

int sctk_is_net_message (int dest){
  sctk_comm_dest_key_t key;
  sctk_internal_ptp_t* tmp;

  key.destination = dest;

  sctk_ptp_table_read_lock(&sctk_ptp_table_lock);
  HASH_FIND(hh,sctk_ptp_table,&key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_ptp_table_read_unlock(&sctk_ptp_table_lock);

  return (tmp == NULL);
}


/********************************************************************/
/*Probe                                                             */
/********************************************************************/

static inline 
void sctk_probe_source_tag_func (int destination, int source,int tag,
				 const sctk_communicator_t comm, int *status,
				 sctk_thread_message_header_t * msg){
  sctk_comm_dest_key_t key;
  sctk_internal_ptp_t* tmp;

  msg->source = source;
  msg->destination = destination;
  msg->message_tag = tag;
  msg->communicator = comm;
  msg->specific_message_tag = pt2pt_specific_message_tag;

  key.destination = sctk_get_comm_world_rank (comm,destination);

  sctk_ptp_table_read_lock(&sctk_ptp_table_lock);
  HASH_FIND(hh,sctk_ptp_table,&key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_ptp_table_read_unlock(&sctk_ptp_table_lock);

  assume(tmp != NULL);
  sctk_internal_ptp_lock_pending(&(tmp->lists));
  *status = sctk_perform_messages_probe_matching(tmp,msg);
  sctk_nodebug("Find source %d tag %d found ?%d",msg->source,msg->message_tag,*status);
  sctk_internal_ptp_unlock_pending(&(tmp->lists));
}
void sctk_probe_source_any_tag (int destination, int source,
				const sctk_communicator_t comm,
				int *status,
				sctk_thread_message_header_t * msg){
  sctk_probe_source_tag_func(destination,source,MPC_ANY_TAG,comm,status,msg);
}

void sctk_probe_any_source_any_tag (int destination,
				    const sctk_communicator_t comm,
				    int *status,
				    sctk_thread_message_header_t * msg){
  sctk_probe_source_tag_func(destination,MPC_ANY_SOURCE,MPC_ANY_TAG,comm,status,msg);
}
void sctk_probe_source_tag (int destination, int source,
			    const sctk_communicator_t comm, int *status,
			    sctk_thread_message_header_t * msg){
  sctk_probe_source_tag_func(destination,source,msg->message_tag,comm,status,msg);
}
void sctk_probe_any_source_tag (int destination,
				const sctk_communicator_t comm,
				int *status,
				sctk_thread_message_header_t * msg){
  sctk_probe_source_tag_func(destination,MPC_ANY_SOURCE,msg->message_tag,comm,status,msg);
}
