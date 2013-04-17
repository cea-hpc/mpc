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
#include <sctk_checksum.h>
#include <mpc_common.h>

/* #define SCTK_DISABLE_REENTRANCE */

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
  sctk_msg_list_t* list;
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
  char changed;

  sctk_message_to_copy_t* sctk_ptp_task_list;
  sctk_spinlock_t sctk_ptp_tasks_lock;
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


  lists->sctk_ptp_tasks_lock = lock;
  lists->sctk_ptp_task_list = NULL;
}

typedef struct sctk_internal_ptp_s{
  sctk_comm_dest_key_t key;
  sctk_comm_dest_key_t key_on_vp;

  sctk_internal_ptp_message_lists_t lists;

  UT_hash_handle hh;
  UT_hash_handle hh_on_vp;
} sctk_internal_ptp_t;

static inline void sctk_ptp_tasks_insert(sctk_message_to_copy_t* tmp,
				    sctk_internal_ptp_t* pair){
  sctk_spinlock_lock(&(pair->lists.sctk_ptp_tasks_lock));
  DL_APPEND(pair->lists.sctk_ptp_task_list,tmp);
  sctk_spinlock_unlock(&(pair->lists.sctk_ptp_tasks_lock));
}
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
  lists->changed = 1;
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
#warning "Using blocking version of send/recv message"
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

/* Table where all msg lists for the process are stored */
static sctk_internal_ptp_t* sctk_ptp_table = NULL;
/* Table where all msg lists for the current VP are stored */
__thread sctk_internal_ptp_t* sctk_ptp_table_on_vp = NULL;
static sctk_internal_ptp_t** sctk_ptp_array = NULL;
/* List for process specific messages */
static sctk_internal_ptp_t* sctk_ptp_admin = NULL;
static int sctk_ptp_array_start = 0;
static int sctk_ptp_array_end = 0;
static sctk_spin_rwlock_t sctk_ptp_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

/* Migration disabled */
#define SCTK_MIGRATION_DISABLED

static inline void
sctk_ptp_table_write_lock(){
#ifndef SCTK_MIGRATION_DISABLED
  if(sctk_migration_mode)
    sctk_spinlock_write_lock(&sctk_ptp_table_lock);
#endif
}

static inline void
sctk_ptp_table_write_unlock(){
#ifndef SCTK_MIGRATION_DISABLED
  if(sctk_migration_mode)
    sctk_spinlock_write_unlock(&sctk_ptp_table_lock);
#endif
}

static inline void
sctk_ptp_table_read_lock(){
#ifndef SCTK_MIGRATION_DISABLED
  if(sctk_migration_mode)
    sctk_spinlock_read_lock(&sctk_ptp_table_lock);
#endif
}

static inline void
sctk_ptp_table_read_unlock(){
#ifndef SCTK_MIGRATION_DISABLED
  if(sctk_migration_mode)
    sctk_spinlock_read_unlock(&sctk_ptp_table_lock);
#endif
}

static inline void sctk_ptp_table_insert(sctk_internal_ptp_t * tmp){
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
  static volatile int done = 0;

  if(tmp->key.destination == -1){
    sctk_ptp_admin = tmp;
    return;
  }

  sctk_spinlock_lock(&lock);
  if(done == 0){
    if(!sctk_migration_mode){
      sctk_ptp_array_start = sctk_get_first_task_local (SCTK_COMM_WORLD);
      sctk_ptp_array_end = sctk_get_last_task_local (SCTK_COMM_WORLD);
      sctk_ptp_array = sctk_malloc((sctk_ptp_array_end - sctk_ptp_array_start + 1)*sizeof(sctk_internal_ptp_t*));
      memset(sctk_ptp_array,0,(sctk_ptp_array_end - sctk_ptp_array_start + 1)*sizeof(sctk_internal_ptp_t*));
    }
    done = 1;
  }

  sctk_ptp_table_write_lock();
  HASH_ADD(hh,sctk_ptp_table,key,sizeof(sctk_comm_dest_key_t),tmp);
  HASH_ADD(hh_on_vp,sctk_ptp_table_on_vp,key,sizeof(sctk_comm_dest_key_t),tmp);
  if(sctk_migration_mode){
    if(sctk_ptp_array != NULL){
      sctk_free(sctk_ptp_array);
      sctk_ptp_array = NULL;
    }
  }
  if(sctk_ptp_array){
    assume(tmp->key.destination >= sctk_ptp_array_start);
    assume(tmp->key.destination <= sctk_ptp_array_end);
    assume(sctk_ptp_array[tmp->key.destination - sctk_ptp_array_start] == NULL);
    sctk_ptp_array[tmp->key.destination - sctk_ptp_array_start] = tmp;
  }
  sctk_ptp_table_write_unlock();
  sctk_spinlock_unlock(&lock);
}

#define sctk_ptp_table_find(key,tmp)  do{				\
    if(key.destination == -1){						\
      tmp = sctk_ptp_admin;						\
    } else {								\
      if(sctk_migration_mode){						\
	HASH_FIND(hh,sctk_ptp_table,&(key),sizeof(sctk_comm_dest_key_t),(tmp)); \
      } else {								\
	int __dest__id;							\
	__dest__id = key.destination - sctk_ptp_array_start;		\
	if((sctk_ptp_array != NULL) && (__dest__id >= 0)		\
	   && (__dest__id <= sctk_ptp_array_end- sctk_ptp_array_start)){ \
	  tmp = sctk_ptp_array[__dest__id];				\
	} else {							\
	  tmp = NULL;							\
	}								\
      }									\
    }									\
  }while(0)


/********************************************************************/
/*Task engine                                                       */
/********************************************************************/
#define SCTK_DISABLE_TASK_ENGINE

static inline int sctk_ptp_tasks_perform(sctk_internal_ptp_t* pair){
  sctk_message_to_copy_t* tmp;
  int processed_nb = 0; /* Number of messages processed */

  /* Each element of this list has already been matched */
  while(pair->lists.sctk_ptp_task_list != NULL){
    tmp = NULL;
    if(sctk_spinlock_trylock(&(pair->lists.sctk_ptp_tasks_lock)) == 0){
      tmp = pair->lists.sctk_ptp_task_list;
      if(tmp != NULL){
        DL_DELETE(pair->lists.sctk_ptp_task_list,tmp);
      }
      sctk_spinlock_unlock(&(pair->lists.sctk_ptp_tasks_lock));
    }
    if(tmp != NULL){
      /* Call the copy function to copy the message from the network buffer to the matching user buffer */
      tmp->msg_send->tail.message_copy(tmp);
      processed_nb++;
    }
  }
  return processed_nb;
}

static inline void sctk_ptp_copy_tasks_insert(sctk_msg_list_t* ptr_recv,
					      sctk_msg_list_t* ptr_send,
					      sctk_internal_ptp_t* pair){
  sctk_message_to_copy_t* tmp;
  SCTK_PROFIL_START (MPC_Copy_message);

  tmp = &(ptr_recv->msg->tail.copy_list);
  tmp->msg_send = ptr_send->msg;
  tmp->msg_recv = ptr_recv->msg;

TODO("Add parapmeter to deal with task engine")
#ifdef SCTK_DISABLE_TASK_ENGINE
    tmp->msg_send->tail.message_copy(tmp);
#else
  sctk_ptp_tasks_insert(tmp,pair);
#endif
  SCTK_PROFIL_END (MPC_Copy_message);
}


/********************************************************************/
/*copy engine                                                       */
/********************************************************************/

void sctk_complete_and_free_message (sctk_thread_ptp_message_t * msg){
  void (*free_memory)(void*);

  free_memory = msg->tail.free_memory;
  if (msg->tail.buffer_async) {
    mpc_buffered_msg_t* buffer_async = msg->tail.buffer_async;
    buffer_async->completion_flag = SCTK_MESSAGE_DONE;
  }

  if(msg->body.completion_flag)
    *(msg->body.completion_flag) = SCTK_MESSAGE_DONE;
  free_memory(msg);
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

    recv->tail.request->msg = NULL;
  }

  if(send->tail.request){
    send->tail.request->msg = NULL;
  }

#ifdef SCTK_USE_CHECKSUM
  sctk_checksum_verify(send, recv);
#endif

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

  assume(send->tail.message_type == sctk_message_pack_absolute);

  switch(recv->tail.message_type){
  case sctk_message_pack_absolute: {
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
  case sctk_message_contiguous: {
    size_t i;
    size_t j;
    size_t size;
    char* body;

    body = recv->tail.message.contiguous.addr;

    sctk_nodebug("COUNT %lu",send->tail.message.pack.count);

    for (i = 0; i < send->tail.message.pack.count; i++)
      for (j = 0; j < send->tail.message.pack.list.absolute[i].count; j++)
        {
          size = (send->tail.message.pack.list.absolute[i].ends[j] -
                  send->tail.message.pack.list.absolute[i].begins[j] +
                  1) * send->tail.message.pack.list.absolute[i].elem_size;
          memcpy(body,((char *) (send->tail.message.pack.list.absolute[i].addr)) +
                 send->tail.message.pack.list.absolute[i].begins[j] *
                 send->tail.message.pack.list.absolute[i].elem_size,size);
          sctk_nodebug("COPY %lu in %p %f <- %p %lu %lu %f",size,body,((double*)body)[0],send->tail.message.pack.list.absolute[i].addr,send->tail.message.pack.list.absolute[i].begins[j],send->tail.message.pack.list.absolute[i].elem_size,(double*)(((char *) (send->tail.message.pack.list.absolute[i].addr)) +
                 send->tail.message.pack.list.absolute[i].begins[j] *
                 send->tail.message.pack.list.absolute[i].elem_size)[0]);
          body += size;
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
/* For message creation: a set of buffered ptp_message entries is allocated during init */
#define BUFFERED_PTP_MESSAGE_NUMBER 100
__thread sctk_thread_ptp_message_t* buffered_ptp_message = NULL;
__thread sctk_spinlock_t lock_buffered_ptp_message = SCTK_SPINLOCK_INITIALIZER;

/*Init data structures used for task i*/
void sctk_ptp_per_task_init (int i){
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
  sctk_internal_ptp_t * tmp;
  int j;

  tmp = sctk_malloc(sizeof(sctk_internal_ptp_t));
  memset(tmp,0,sizeof(sctk_internal_ptp_t));
/*   tmp->key.comm = SCTK_COMM_WORLD; */
  tmp->key.destination = i;
  sctk_nodebug("Destination: %d", i);

  sctk_spinlock_lock(&lock);
  sctk_internal_ptp_message_list_init(&(tmp->lists));
  sctk_ptp_table_insert(tmp);
  sctk_spinlock_unlock(&lock);

  if (buffered_ptp_message == NULL) {
    sctk_spinlock_lock(&lock_buffered_ptp_message);

    /* List not already allocated. We create it */
    if (buffered_ptp_message == NULL) {
      sctk_thread_ptp_message_t* tmp;
      tmp = sctk_malloc(sizeof(sctk_thread_ptp_message_t) * BUFFERED_PTP_MESSAGE_NUMBER);
      assume(tmp);

      /* Loop on all buffers and create a list */
      for (j = 0; j < BUFFERED_PTP_MESSAGE_NUMBER; ++j) {
        sctk_thread_ptp_message_t* entry = &tmp[j];
        entry->from_buffered = 1;
        /* Add it to the list */
        LL_PREPEND(buffered_ptp_message, entry);
      }
    }

    sctk_spinlock_unlock(&lock_buffered_ptp_message);
  }
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

static
void sctk_free_header(void* tmp){
  sctk_thread_ptp_message_t *header = (sctk_thread_ptp_message_t*) tmp;
  sctk_nodebug("Free buffer %p buffered?%d", header, header->from_buffered);
  /* Header is from the buffered list */
  if (header->from_buffered) {
    sctk_spinlock_lock(&lock_buffered_ptp_message);
    LL_PREPEND(buffered_ptp_message, header);
    sctk_spinlock_unlock(&lock_buffered_ptp_message);
  } else {
    sctk_free(tmp);
  }
}

static
void* sctk_alloc_header(){
  sctk_thread_ptp_message_t *tmp = NULL;

  /* We first look at the buffered list if a header is available */
  if (buffered_ptp_message != NULL) {
    sctk_spinlock_lock(&lock_buffered_ptp_message);
    if (buffered_ptp_message != NULL) {
      tmp = buffered_ptp_message;
      assume(tmp->from_buffered);
      LL_DELETE(buffered_ptp_message, buffered_ptp_message);
    }
    sctk_spinlock_unlock(&lock_buffered_ptp_message);
  }

  /* If no more entries available in the buffered list, we allocate */
  if (tmp == NULL) {
    tmp = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
    /* Header must be freed after use */
    tmp->from_buffered = 0;
  }
  return tmp;
}

int sctk_determine_src_process_from_header (sctk_thread_ptp_message_body_t * body){
  int src_process;
  int task_number;

  if(IS_PROCESS_SPECIFIC_MESSAGE_TAG(body->header.specific_message_tag)){
    src_process = body->header.source;
  } else {
    if(body->header.source != MPC_ANY_SOURCE) {
      task_number = sctk_get_comm_world_rank (body->header.communicator,
          body->header.source);
    src_process = sctk_get_process_rank_from_task_rank(task_number);
  } else {
      src_process = -1;
    }
  }
  return src_process;
}

void sctk_rebuild_header (sctk_thread_ptp_message_t * msg){
  if(IS_PROCESS_SPECIFIC_MESSAGE_TAG(msg->sctk_msg_get_specific_message_tag)){
    if(msg->sctk_msg_get_source != MPC_ANY_SOURCE)
      msg->sctk_msg_get_glob_source = msg->sctk_msg_get_source;
    else
      msg->sctk_msg_get_glob_source = -1;

    msg->sctk_msg_get_glob_destination = -1;
  } else {
    if(msg->sctk_msg_get_source != MPC_ANY_SOURCE)
      msg->sctk_msg_get_glob_source =
	sctk_get_comm_world_rank (msg->sctk_msg_get_communicator,msg->sctk_msg_get_source);
    else
      msg->sctk_msg_get_glob_source = -1;

    msg->sctk_msg_get_glob_destination =
      sctk_get_comm_world_rank (msg->sctk_msg_get_communicator,msg->sctk_msg_get_destination);
  }
  msg->tail.need_check_in_wait = 1;
  msg->tail.request = NULL;
}

void sctk_reinit_header (sctk_thread_ptp_message_t *tmp, void (*free_memory)(void*),
		       void (*message_copy)(sctk_message_to_copy_t*)){

  tmp->tail.free_memory = free_memory;
  tmp->tail.message_copy = message_copy;
  tmp->tail.buffer_async = NULL;
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

  tmp = sctk_alloc_header();
  /* Store if the buffer has been buffered */
  const char from_buffered = tmp->from_buffered;
  sctk_init_header(tmp,myself,msg_type,sctk_free_header,sctk_message_copy);
  /* Restore it */
  tmp->from_buffered = from_buffered;

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

static inline
void sctk_request_init_request(sctk_request_t * request, int completion,
			       sctk_thread_ptp_message_t * msg,
			       const int source,
			       const int destination,
			       const int message_tag,
			       const sctk_communicator_t
			       communicator,
			       const size_t count){
  assume(msg != NULL);
  request->msg = msg;
  request->header.source = source;
  request->header.destination = destination;
  request->header.glob_destination = msg->sctk_msg_get_glob_destination;
  request->header.glob_source = msg->sctk_msg_get_glob_source;
  request->header.message_tag = message_tag;
  request->header.communicator = communicator;
  request->header.msg_size = count;
  request->is_null = 0;
  request->completion_flag = completion;
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

  msg->sctk_msg_get_source = source;
  msg->sctk_msg_get_destination = destination;

  if(IS_PROCESS_SPECIFIC_MESSAGE_TAG(specific_message_tag)){
    if(source != MPC_ANY_SOURCE) {
      msg->sctk_msg_get_glob_source = source;
    }
    else
      msg->sctk_msg_get_glob_source = -1;

    msg->sctk_msg_get_glob_destination = -1;
  } else {
    if(source != MPC_ANY_SOURCE) {
      /* If the communicator used is the COMM_SELF */
      if (communicator == SCTK_COMM_SELF) {
        /* The world destination is actually ourself :) */
        int world_src = sctk_get_task_rank ();
        msg->sctk_msg_get_glob_source = sctk_get_comm_world_rank (communicator, world_src);
      } else {
        msg->sctk_msg_get_glob_source = sctk_get_comm_world_rank (communicator, source);
      }
    }
    else
      msg->sctk_msg_get_glob_source = -1;

    /* If the communicator used is the COMM_SELF */
    if (communicator == SCTK_COMM_SELF) {
      /* The world destination is actually ourself :) */
      int world_dest = sctk_get_task_rank ();
      msg->sctk_msg_get_glob_destination = sctk_get_comm_world_rank (communicator, world_dest);
    } else {
      msg->sctk_msg_get_glob_destination = sctk_get_comm_world_rank (communicator, destination);
    }
  }

  msg->body.header.communicator = communicator;
  msg->body.header.message_tag = message_tag;
  msg->body.header.specific_message_tag = specific_message_tag;
  msg->body.header.msg_size = count;

  msg->sctk_msg_get_use_message_numbering = 1;

  if(request != NULL){

    sctk_request_init_request(request,SCTK_MESSAGE_PENDING,msg,source,destination,message_tag,communicator,
			      count);

    msg->body.completion_flag = &(request->completion_flag);
  }
  msg->tail.need_check_in_wait = 1;
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
  sctk_free_header(tmp);
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
    SCTK_PROFIL_START (MPC_Test_message_search_matching);
    SCTK_PROFIL_END (MPC_Test_message_search_matching);

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
    int remote_task;
    int remote_process;
    remote_task = sctk_get_comm_world_rank (header->communicator,header->source);
    remote_process = sctk_get_process_rank_from_task_rank(remote_task);

    if(remote_process != sctk_process_rank){
      sctk_network_notify_perform_message (remote_process);
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
    SCTK_PROFIL_START (MPC_Test_message_pair_locked);
    ptr_send = sctk_perform_messages_search_matching(pair,&(ptr_recv->msg->body.header));
    SCTK_PROFIL_END (MPC_Test_message_pair_locked);
    /* The task has posted the send buffer (i.e: MPI_Send)*/
    if(ptr_send != NULL){
      DL_DELETE(pair->lists.pending_recv.list,ptr_recv);

      /*Insert the matching message to the pendint list. At this time,
       * the message is not copied */
      sctk_ptp_copy_tasks_insert(ptr_recv,ptr_send,pair);
    } else {
      if(ptr_recv->msg->tail.remote_source){
        sctk_network_notify_matching_message (ptr_recv->msg);
      }
      if(ptr_recv->msg->body.header.source == MPC_ANY_SOURCE){
        sctk_network_notify_any_source_message ();
      }
    }
  }
}

static inline int sctk_perform_messages_for_pair(sctk_internal_ptp_t* pair){
  SCTK_PROFIL_START (MPC_Test_message_pair);
  if(((pair->lists.pending_send.list != NULL)
#ifndef SCTK_DISABLE_REENTRANCE
      ||(pair->lists.incomming_send.list != NULL)
#endif
      ) && ((pair->lists.pending_recv.list != NULL)
#ifndef SCTK_DISABLE_REENTRANCE
	 || (pair->lists.incomming_recv.list != NULL)
#endif
	 )){
    if(pair->lists.changed || 1
#ifndef SCTK_DISABLE_REENTRANCE
       || (pair->lists.incomming_send.list != NULL)
       || (pair->lists.incomming_recv.list != NULL)
#endif
){

      SCTK_PROFIL_START (MPC_Test_message_pair_try_lock);
      sctk_internal_ptp_lock_pending(&(pair->lists));
      sctk_perform_messages_for_pair_locked(pair);
      pair->lists.changed = 0;
      sctk_internal_ptp_unlock_pending(&(pair->lists));
      SCTK_PROFIL_END (MPC_Test_message_pair_try_lock);
    }
  }
  SCTK_PROFIL_END (MPC_Test_message_pair);
  return sctk_ptp_tasks_perform(pair);
}

/*
 * This function returns the number of messages processed.
 */
static inline int sctk_try_perform_messages_for_pair(sctk_internal_ptp_t* pair){
  int res = 0;
  SCTK_PROFIL_START (MPC_Test_message_pair_try);
  /* If the lock has not been taken, we continue */
  if(pair->lists.pending_lock == 0){
    res = sctk_perform_messages_for_pair(pair);
  }
  SCTK_PROFIL_END (MPC_Test_message_pair_try);
  return res;
}

void sctk_wait_message (sctk_request_t * request){
  sctk_perform_messages(request);

  if(request->completion_flag != SCTK_MESSAGE_DONE){
    sctk_thread_wait_for_value_and_poll
      ((int *) &(request->completion_flag),SCTK_MESSAGE_DONE ,
       (void(*)(void*))sctk_perform_messages,(void*)request);
  }
}

/*
 * Function called for performing message from a specific request.
 * FIXME: Few lines have been commented because they seem useless.
 * We should remove them in a next commit once validated all is
 * working fine
 */
void sctk_perform_messages(sctk_request_t* request){
  if(request->completion_flag != SCTK_MESSAGE_DONE){
     sctk_internal_ptp_t* recv_ptp = NULL;
     sctk_internal_ptp_t* send_ptp;

     {
       sctk_comm_dest_key_t recv_key;
      sctk_comm_dest_key_t send_key;
       /* key.comm = request->header.communicator; */
       recv_key.destination = request->header.glob_destination;
       send_key.destination = request->header.glob_source;

       sctk_ptp_table_read_lock();
       /* Searching for the pending list corresponding to the
        * dest task */
       sctk_ptp_table_find(recv_key, recv_ptp);
       sctk_ptp_table_find(send_key, send_ptp);
       sctk_ptp_table_read_unlock();
     }
     /* We assume that the corresponding pending list has been found.
      * FIXME: not working, why ?*/
     /* assume(recv_ptp); */

    /* Check the source of the request. We try to poll the
     * source in order to retreive messages from the network */
    if(request->header.source == MPC_ANY_SOURCE){
      /* If the source task is not on the current node, we
       * call the low level module */
      if (!send_ptp) sctk_network_notify_any_source_message ();
    } else {
      /* This call can return remote_process == sctk_process.
       * If we use an MPI_Isend followed by a MPI_Test, remote_process
       * is equal to the source of the message, so the current task */

      /* If the source task is not on the current node, we
       * call the low level module */
      if (!send_ptp) {
        int remote_task;
        int remote_process;

        sctk_nodebug("remote process=%d source=%d comm=%d",
            remote_process, request->header.source, request->header.communicator);

        /* Convert task rank to process rank */
        remote_task = sctk_get_comm_world_rank (
            request->header.communicator,request->header.source);
        remote_process = sctk_get_process_rank_from_task_rank(remote_task);

        sctk_network_notify_perform_message (remote_process);
      }
    }

    /* Try to match messages if the user asked for it */
    if(recv_ptp && request->need_check_in_wait == 1) {
    	sctk_try_perform_messages_for_pair(recv_ptp);
    }
  }
}

/*
 * Wait for all message according to a communicator and a task id
 * FIXME: We do not need to loop on all pending lists from
 * sctk_ptp_table.
 */
void sctk_wait_all (const int task, const sctk_communicator_t com){
  sctk_internal_ptp_t* pair;
  int i;

  /* Get the pending list associated to the task */
  pair = sctk_get_internal_ptp(task);
  sctk_assert(pair);

  do{
    i = 0;
    sctk_ptp_table_read_lock();
    sctk_msg_list_t* ptr;
    sctk_internal_ptp_lock_pending(&(pair->lists));
    sctk_perform_messages_for_pair_locked(pair);
    /* We loop on the recv list */
    DL_FOREACH(pair->lists.pending_recv.list,ptr){
      if((ptr->msg->body.header.destination == task) &&
          (ptr->msg->body.header.communicator == com)){
        i++;
      }
    }
    /* We loop on the send list */
    DL_FOREACH(pair->lists.pending_send.list,ptr){
      if((ptr->msg->body.header.source == task) &&
          (ptr->msg->body.header.communicator == com)){
        i++;
      }
    }
    sctk_internal_ptp_unlock_pending(&(pair->lists));
    sctk_ptp_table_read_unlock();

    if (i != 0) sctk_thread_yield();
  } while(i != 0);

  /* FIXME: The following code is the previous implementation of the function. We should remove it in a next commit */
#if 0
  sctk_internal_ptp_t* tmp;
  do{
    i = 0;
    sctk_ptp_table_read_lock();
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
    sctk_ptp_table_read_unlock();

    if (i != 0) sctk_thread_yield();
  } while(i != 0);
#endif
}


void sctk_perform_all (){
  sctk_internal_ptp_t* pair;
  sctk_internal_ptp_t* tmp;
  sctk_ptp_table_read_lock();
  int processed_nb = 0; /* Number of messages processed */

  HASH_ITER(hh_on_vp,sctk_ptp_table_on_vp,pair,tmp){
    processed_nb += sctk_try_perform_messages_for_pair(pair);
  }

  /* Also try to poll the ptp_admin queue if no message
   * has been processed previously */
  if(sctk_ptp_admin && !processed_nb) {
    processed_nb += sctk_try_perform_messages_for_pair(sctk_ptp_admin);
  }

  /* At the end, we try to process messages from other
   * pending lists. We leave when we found a message to
   * handle */

  if( !processed_nb ) {
    HASH_ITER(hh,sctk_ptp_table,pair,tmp){
      processed_nb += sctk_try_perform_messages_for_pair(pair);
      if (processed_nb) break;
    }
  }

  /* FIXME: We only try to match messages for the tasks
   * which are on the same NUMA node than the current thread.
   * We observed *HUGE* performance issues while looping on
   * all tasks on the node. Ideally, we should try to handle messages
   * on the same VP and try to steal messages from another
   * VP.
  HASH_ITER(hh,sctk_ptp_table,pair,tmp){
    sctk_try_perform_messages_for_pair(pair);
  }
  */
  sctk_ptp_table_read_unlock();
}


void sctk_notify_idle_message_inter (){
  if(sctk_process_number > 1){
    sctk_network_notify_idle_message ();
  }
}


void sctk_notify_idle_message (){
  sctk_perform_all ();
  if(sctk_process_number > 1){
    sctk_network_notify_idle_message ();
  }
}

/********************************************************************/
/* Send messages                                                    */
/********************************************************************/

/*
 * Function called when sending a message. The message can be forwarded to another process
 * using the 'sctk_network_send_message' function. If the message
 * matches, we add it to the corresponding pending list
 * */
void sctk_send_message_try_check (sctk_thread_ptp_message_t * msg,int perform_check){
  /* The message is a process specific message and the process rank does not match
   * the current process rank */
  if((IS_PROCESS_SPECIFIC_MESSAGE_TAG(msg->body.header.specific_message_tag)) &&
     (msg->sctk_msg_get_destination != sctk_process_rank)){
    msg->tail.remote_destination = 1;
    /* We forward the message */
    sctk_network_send_message (msg);
  } else {
    sctk_comm_dest_key_t key;
    sctk_internal_ptp_t* tmp;

    /*   key.comm = msg->header.communicator; */
    key.destination = msg->sctk_msg_get_glob_destination;

    assume(msg->sctk_msg_get_communicator >= 0);

    if(msg->body.completion_flag != NULL){
      *(msg->body.completion_flag) = SCTK_MESSAGE_PENDING;
    }

    if(msg->tail.request != NULL){
      msg->tail.request->need_check_in_wait = msg->tail.need_check_in_wait;
    }

    msg->tail.remote_source = 0;
    msg->tail.remote_destination = 0;

    /* We are searching for the corresponding pending list.
     * If we do not find any entry, we forward the message */
    sctk_ptp_table_read_lock();
    sctk_ptp_table_find(key,tmp);
    sctk_ptp_table_read_unlock();

    if(tmp != NULL){
      sctk_internal_ptp_add_send_incomming(tmp,msg);
      /* If we ask for a checking, we check it */
      if(perform_check) sctk_try_perform_messages_for_pair(tmp);
    } else {
      /*Entering low level comm and forwarding the message*/
      msg->tail.remote_destination = 1;
      sctk_network_send_message (msg);
    }
  }
}

/*
 * Function called for sending a message (i.e: MPI_Send).
 * Mostly used by the file mpc.c
 * */
void sctk_send_message (sctk_thread_ptp_message_t * msg){
TODO("To optimize")
  /*
      msg->tail.need_check_in_wait should be optimize by adding self polling in wait and test on myself
   */
  msg->tail.need_check_in_wait = 1;
  sctk_send_message_try_check(msg,0);
}

/********************************************************************/
/* Recv messages                                                    */
/********************************************************************/

/*
 * Function called when receiving a message.
 * */
void sctk_recv_message_try_check (sctk_thread_ptp_message_t * msg,sctk_internal_ptp_t* tmp,int perform_check){
  sctk_comm_dest_key_t send_key;
  sctk_internal_ptp_t* send_tmp = NULL;

  send_key.destination = msg->sctk_msg_get_glob_source;

  if(msg->body.completion_flag != NULL){
    *(msg->body.completion_flag) = SCTK_MESSAGE_PENDING;
  }

  if(msg->tail.request != NULL){
    msg->tail.request->need_check_in_wait = msg->tail.need_check_in_wait;
  }

  msg->tail.remote_source = 0;
  msg->tail.remote_destination = 0;

  /* We are searching for the list corresponding to the
   * task which receives the message */
  sctk_ptp_table_read_lock();
  if(tmp == NULL){
    sctk_comm_dest_key_t key;
    key.destination = msg->sctk_msg_get_glob_destination;
    sctk_ptp_table_find(key,tmp);
  }

  if(!IS_PROCESS_SPECIFIC_MESSAGE_TAG(msg->body.header.specific_message_tag)){
    if(send_key.destination != -1){
      sctk_ptp_table_find(send_key,send_tmp);
    }
  }
  sctk_ptp_table_read_unlock();
  /* We assume that the entry is found. If not, the message received is
   * for a task which is not registered on the node. Possible issue. */
  assume(tmp);

  if(send_tmp == NULL){
    msg->tail.need_check_in_wait = 1;
    /*Entering low level comm*/
    msg->tail.remote_source = 1;
    sctk_network_notify_recv_message (msg);
  }

  /* We add the message to the pending list */
  sctk_internal_ptp_add_recv_incomming(tmp,msg);
  /* Iw we ask for a matching, we run it */
  if(perform_check){
    sctk_try_perform_messages_for_pair(tmp);
  }
}

/*
 * Function called for receiving a message (i.e: MPI_Recv).
 * Mostly used by the file mpc.c
 * */
void sctk_recv_message (sctk_thread_ptp_message_t * msg,sctk_internal_ptp_t* tmp){
  msg->tail.need_check_in_wait = 0;
  sctk_recv_message_try_check(msg,tmp,1);
}

/*
 * Get the internal pending list for a specific task
 */
struct sctk_internal_ptp_s* sctk_get_internal_ptp(int glob_id){
  sctk_internal_ptp_t* tmp = NULL;
  if(!sctk_migration_mode){
    sctk_comm_dest_key_t key;
    key.destination = glob_id;
    sctk_ptp_table_read_lock();
    sctk_ptp_table_find(key,tmp);
    sctk_ptp_table_read_unlock();
  }
  return tmp;
}

/*
 * Return if the task is on the node or not
 */
int sctk_is_net_message (int dest){
  sctk_comm_dest_key_t key;
  sctk_internal_ptp_t* tmp;

  key.destination = dest;

  sctk_ptp_table_read_lock();
  sctk_ptp_table_find(key,tmp);
  sctk_ptp_table_read_unlock();

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

  sctk_ptp_table_read_lock();
  sctk_ptp_table_find(key,tmp);
  sctk_ptp_table_read_unlock();

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
