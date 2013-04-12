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
#include <sctk_communicator.h>
#include <sctk.h>
#include <sctk_debug.h>
#include <sctk_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <sctk_asm.h>
#include <sctk_checksum.h>
#include <mpc_common.h>
#include <sctk_reorder.h>

/********************************************************************/
/* Macros for configuration                                         */
/********************************************************************/
/* Reentrance enabled */
/*  #define SCTK_DISABLE_REENTRANCE */
/* Migration disabled */
#define SCTK_MIGRATION_DISABLED
/* Task engine disabled */
#define SCTK_DISABLE_TASK_ENGINE
/* Defines if we are in a full MPI mode */
//~ #define SCTK_ENABLE_FULL_MPI

TODO("sctk_cancel_message: need to be implemented")
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
  volatile sctk_msg_list_t* list;
} sctk_internal_ptp_list_pending_t;

typedef struct {
 /* Messages are posted to the 'incoming' lists before being merge to
    the pending list. */
#ifndef SCTK_DISABLE_REENTRANCE
  sctk_internal_ptp_list_incomming_t incomming_send;
  sctk_internal_ptp_list_incomming_t incomming_recv;
#endif

  sctk_spinlock_t pending_lock;
  /* Messages in the 'pending' lists are waiting to be
   * matched */
  sctk_internal_ptp_list_pending_t pending_send;
  sctk_internal_ptp_list_pending_t pending_recv;
  /* Flag to indicate that new messages have been merged into the 'pending' list */
  char changed;

  /* Messages in the 'sctk_ptp_task_list' have already been
   * matched and are wainting to be copied */
  volatile sctk_message_to_copy_t* sctk_ptp_task_list;
  sctk_spinlock_t sctk_ptp_tasks_lock;
}sctk_internal_ptp_message_lists_t;

typedef struct sctk_internal_ptp_s{
  sctk_comm_dest_key_t key;
  sctk_comm_dest_key_t key_on_vp;

  sctk_internal_ptp_message_lists_t lists;

  /* Number of pending messages for the MPI task */
  OPA_int_t pending_nb;

  UT_hash_handle hh;
  UT_hash_handle hh_on_vp;

  /* Reorder table / ! \
   * This table is now at a task level !*/
  sctk_reorder_list_t reorder;

} sctk_internal_ptp_t;

/********************************************************************/
/*Functions                                                         */
/********************************************************************/

static void sctk_show_requests(sctk_request_t* request, int req_nb) {
  int i;

//  for (i=0; i< req_nb; ++i) {
//    sctk_request_t* request = &requests[req_nb];

    sctk_debug("Request %p from %d to %d (glob=from %d to %d type=%d msg=%p)",
        request,
        request->header.source,
        request->header.destination,
        request->header.glob_source,
        request->header.glob_destination,
        request->request_type,
        request->msg);

    if (request->msg) {
      sctk_debug("Ceck in wait: %d %p", request->msg->tail.need_check_in_wait, request->msg->tail.request);
TODO("Fill with infos from the message");
    }
//  }
}

/*
 * Initialize the 'incoming' list.
 */
static inline void sctk_internal_ptp_list_incomming_init(sctk_internal_ptp_list_incomming_t* list){
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;

  list->list = NULL;
  list->lock = lock;
}
/*
 * Initialize the 'pending' list
 */
static inline void sctk_internal_ptp_list_pending_init(sctk_internal_ptp_list_pending_t* list){
  list->list = NULL;
}

/*
 * Initialize a PTP the 'incoming' and 'pending' message lists.
 */
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

/*
 * Insert a message to copy. The message to insert has already been matched.
 */
static inline void sctk_ptp_tasks_insert(sctk_message_to_copy_t* tmp,
				    sctk_internal_ptp_t* pair){
  sctk_spinlock_lock(&(pair->lists.sctk_ptp_tasks_lock));
  DL_APPEND(pair->lists.sctk_ptp_task_list,tmp);
  sctk_spinlock_unlock(&(pair->lists.sctk_ptp_tasks_lock));
}

/*
 * Merge 'incoming' send and receive lists to 'pending' send and receive lists
 */
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
  /* Change the flag. New messages have been inserted to the 'pending' lists */
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

/*
 * Add message into the 'incoming' recv list
 */
static inline void sctk_internal_ptp_add_pending(sctk_internal_ptp_t* tmp,
							sctk_thread_ptp_message_t * msg){
    msg->tail.internal_ptp = tmp;
    OPA_incr_int(&tmp->pending_nb);
}

#ifndef SCTK_DISABLE_REENTRANCE
/*
 * Add message into the 'incoming' recv list
 */
static inline void sctk_internal_ptp_add_recv_incomming(sctk_internal_ptp_t* tmp,
							sctk_thread_ptp_message_t * msg){
    msg->tail.distant_list.msg = msg;
    sctk_spinlock_lock(&(tmp->lists.incomming_recv.lock));
    DL_APPEND(tmp->lists.incomming_recv.list, &(msg->tail.distant_list));
    sctk_spinlock_unlock(&(tmp->lists.incomming_recv.lock));
}
/*
 * Add message into the 'incoming' send list
 */
static inline void sctk_internal_ptp_add_send_incomming(sctk_internal_ptp_t* tmp,
							sctk_thread_ptp_message_t * msg){
    msg->tail.distant_list.msg = msg;
    sctk_spinlock_lock(&(tmp->lists.incomming_send.lock));
    DL_APPEND(tmp->lists.incomming_send.list, &(msg->tail.distant_list));
    sctk_spinlock_unlock(&(tmp->lists.incomming_send.lock));
}
#else
#warning "Using blocking version of send/recv message"
/*
 * No 'incoming' recv list. We directly add the message into the 'pending' recv list
 */
static inline void sctk_internal_ptp_add_recv_incomming(sctk_internal_ptp_t* tmp,
							sctk_thread_ptp_message_t * msg){
    msg->tail.distant_list.msg = msg;
    sctk_internal_ptp_lock_pending(&(tmp->lists));
    DL_APPEND(tmp->lists.pending_recv.list, &(msg->tail.distant_list));
    sctk_internal_ptp_unlock_pending(&(tmp->lists));
}
/*
 * No 'incoming' send list. We directly add the message into the 'pending' send list
 */
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

/*
 * Loop on all internal ptp on the node, including the 'sctk_admin_ptp' list
 * The 'func' function is called for each element found.
 * The 'return_on_found' indicates if the function returns if an element has been found.
 */
static int sctk_ptp_table_loop( int (*func)(sctk_internal_ptp_t* pair), char return_on_found ) {
  sctk_internal_ptp_t* pair;
  sctk_internal_ptp_t* tmp;
  int ret;

  sctk_ptp_table_read_lock();
  if (sctk_migration_mode) {
    HASH_ITER(hh,sctk_ptp_table,pair,tmp){
      ret += func(tmp);
      if ( (ret > 0) && return_on_found) break;
    }
  } else {
    if (sctk_ptp_array == NULL) {
      sctk_ptp_table_read_unlock();
      return;
    }
    int i;
    int max = (sctk_ptp_array_end - sctk_ptp_array_start + 1);
    for (i=0; i < max; ++i) {
      if (sctk_ptp_array[i] == NULL) continue;
      ret += func(sctk_ptp_array[i]);
      if ( (ret > 0) && return_on_found) break;
    }
  }
  sctk_ptp_table_read_unlock();
}

/*
 * Find an internal ptp according the a key (this key represents the task number)
 */
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

sctk_reorder_list_t * sctk_ptp_get_reorder_from_destination(int task) {
  struct sctk_internal_ptp_s *internal_ptp;
  sctk_comm_dest_key_t key;
  key.destination = task;

  sctk_ptp_table_read_lock();
  sctk_ptp_table_find(key, internal_ptp);
  sctk_ptp_table_read_unlock();

  return &(internal_ptp->reorder);
}


/*
 * Insert a new entry to the PTP table. The function checks if the entry is already prensent
 * and fail in this case
 */
static inline void sctk_ptp_table_insert(sctk_internal_ptp_t * tmp){
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
  static volatile int done = 0;

  /* If the destination is -1, the message is for the 'sctk_ptp_admin' list */
  if(tmp->key.destination == -1){
    sctk_ptp_admin = tmp;
    /* Do not needed to be added */
    return;
  }

  { /* Check if the entry has not been already added */
    sctk_internal_ptp_t * internal_ptp;
    sctk_ptp_table_find(tmp->key,internal_ptp);
    assume(internal_ptp == NULL);
  }

  sctk_spinlock_lock(&lock);
  /* Only one task allocate the structures */
  if(done == 0){
    /* If migration disabled, we use the 'sctk_ptp_array' structure.*/
    if(!sctk_migration_mode){
      sctk_ptp_array_start = sctk_get_first_task_local (SCTK_COMM_WORLD);
      sctk_ptp_array_end = sctk_get_last_task_local (SCTK_COMM_WORLD);
      sctk_ptp_array = sctk_malloc((sctk_ptp_array_end - sctk_ptp_array_start + 1)*sizeof(sctk_internal_ptp_t*));
      memset(sctk_ptp_array,0,(sctk_ptp_array_end - sctk_ptp_array_start + 1)*sizeof(sctk_internal_ptp_t*));
    }
    done = 1;
  }

  sctk_ptp_table_write_lock();
  /* If migration enabled, we add the current list to sctk_ptp_table */
  if (sctk_migration_mode) {
    HASH_ADD(hh,sctk_ptp_table,key,sizeof(sctk_comm_dest_key_t),tmp);
  } else {
    assume(tmp->key.destination >= sctk_ptp_array_start);
    assume(tmp->key.destination <= sctk_ptp_array_end);
    assume(sctk_ptp_array[tmp->key.destination - sctk_ptp_array_start] == NULL);
    /* Last thing which has to be done */
    sctk_ptp_array[tmp->key.destination - sctk_ptp_array_start] = tmp;
  }
  HASH_ADD(hh_on_vp,sctk_ptp_table_on_vp,key,sizeof(sctk_comm_dest_key_t),tmp);
  sctk_ptp_table_write_unlock();
  sctk_spinlock_unlock(&lock);
}

/********************************************************************/
/*Task engine                                                       */
/********************************************************************/

#ifndef SCTK_DISABLE_TASK_ENGINE
/*
 * Function which loop on t'he ptp_task_list' in order to copy
 * pending messages which have been already matched.
 * Only called if the task engine is enabled
 */
static inline int sctk_ptp_tasks_perform(sctk_internal_ptp_t* pair){
  sctk_message_to_copy_t* tmp;
  int nb_messages_copied = 0; /* Number of messages processed */

  /* Each element of this list has already been matched */
  while(pair->lists.sctk_ptp_task_list != NULL){
    tmp = NULL;
    if(sctk_spinlock_trylock(&(pair->lists.sctk_ptp_tasks_lock)) == 0){
      tmp = pair->lists.sctk_ptp_task_list;
      if(tmp != NULL){
        /* Message found, we remove it from the list */
        DL_DELETE(pair->lists.sctk_ptp_task_list,tmp);
      }
      sctk_spinlock_unlock(&(pair->lists.sctk_ptp_tasks_lock));
    }

    if(tmp != NULL){
      assume(tmp->msg_send->tail.message_copy);
      /* Call the copy function to copy the message from the network buffer to the matching user buffer */
      tmp->msg_send->tail.message_copy(tmp);
      nb_messages_copied++;
    }
  }
  return nb_messages_copied;
}
#endif

/*
 * Insert the message to copy into the pending list
 */
static inline void sctk_ptp_copy_tasks_insert(sctk_msg_list_t* ptr_recv,
					      sctk_msg_list_t* ptr_send,
					      sctk_internal_ptp_t* pair){
  sctk_message_to_copy_t* tmp;

  tmp = &(ptr_recv->msg->tail.copy_list);
  tmp->msg_send = ptr_send->msg;
  tmp->msg_recv = ptr_recv->msg;
TODO("Add parapmeter to deal with task engine")
#ifdef SCTK_DISABLE_TASK_ENGINE
  assume(tmp->msg_send->tail.message_copy);
  tmp->msg_send->tail.message_copy(tmp);
#else
  sctk_ptp_tasks_insert(tmp,pair);
#endif
}


/********************************************************************/
/*copy engine                                                       */
/********************************************************************/

/*
 * Mark a message received as DONE and call the 'free' function associated
 * to the message
 */
void sctk_complete_and_free_message (sctk_thread_ptp_message_t * msg){
  void (*free_memory)(void*);

  free_memory = msg->tail.free_memory;
  assume(free_memory);

  if (msg->tail.buffer_async) {
    mpc_buffered_msg_t* buffer_async = msg->tail.buffer_async;
    buffer_async->completion_flag = SCTK_MESSAGE_DONE;
  }

  if(msg->body.completion_flag) {
    *(msg->body.completion_flag) = SCTK_MESSAGE_DONE;
  }

  if(msg->tail.internal_ptp) {
    OPA_decr_int(&msg->tail.internal_ptp->pending_nb);
  }

  free_memory(msg);
}

void sctk_message_completion_and_free(sctk_thread_ptp_message_t* send,
				     sctk_thread_ptp_message_t* recv){
  size_t size;

  /* If a recv request is available */
  if(recv->tail.request){

TODO("Uselss code")
#if 0
    /* If the receive request has more bytes than the
     * send request, we reajust the size
     */
    size = send->body.header.msg_size;
    if(recv->tail.request->header.msg_size > size){
      recv->tail.request->header.msg_size = size;
    }
#endif

    /* Update the request with the source, message tag and message size */
    recv->tail.request->header.source = send->body.header.source;
    recv->tail.request->header.message_tag = send->body.header.message_tag;
    recv->tail.request->header.msg_size = send->body.header.msg_size;

    recv->tail.request->msg = NULL;
  }

  /* If a send request is available */
  if(send->tail.request){
    send->tail.request->msg = NULL;
  }

#ifdef SCTK_USE_CHECKSUM
  /* Verify the checksum of the received message */
  sctk_checksum_verify(send, recv);
#endif

  /* Complete messages: mark messages as done and mark them as DONE */
  sctk_complete_and_free_message(send);
  sctk_complete_and_free_message(recv);
}

inline void sctk_message_copy(sctk_message_to_copy_t* tmp)
{
	sctk_thread_ptp_message_t* send;
	sctk_thread_ptp_message_t* recv;

	send = tmp->msg_send;
	recv = tmp->msg_recv;
	
	assume(send->tail.message_type == sctk_message_contiguous);	
	
	switch(recv->tail.message_type)
	{
		case sctk_message_contiguous: 
		{
			sctk_nodebug("sctk_message_contiguous - sctk_message_contiguous");
			size_t size;
			size = send->tail.message.contiguous.size;
			if(size > recv->tail.message.contiguous.size)
			{
				size = recv->tail.message.contiguous.size;
			}
			memcpy(recv->tail.message.contiguous.addr,send->tail.message.contiguous.addr, size);

			sctk_message_completion_and_free(send,recv);
			break;
		}
		case sctk_message_pack: 
		{
			sctk_nodebug("sctk_message_contiguous - sctk_message_pack");
			size_t i;
			size_t j;
			size_t size;
			if(send->tail.message.contiguous.size > 0)
			{
				for (i = 0; i < recv->tail.message.pack.count; i++)
				{
					for (j = 0; j < recv->tail.message.pack.list.std[i].count; j++)
					{
						size = (recv->tail.message.pack.list.std[i].ends[j] - recv->tail.message.pack.list.std[i].begins[j] + 1) * 
							recv->tail.message.pack.list.std[i].elem_size;
						memcpy((recv->tail.message.pack.list.std[i].addr) + recv->tail.message.pack.list.std[i].begins[j] * 
							recv->tail.message.pack.list.std[i].elem_size,send->tail.message.contiguous.addr,size);
						send->tail.message.contiguous.addr += size;
					}
				}
			}
			sctk_message_completion_and_free(send,recv);
			break;
		}
		case sctk_message_pack_absolute: 
		{
			sctk_nodebug("sctk_message_contiguous - sctk_message_pack_absolute");
			size_t i;
			size_t j;
			size_t size;
			if(send->tail.message.contiguous.size > 0)
			{
				for (i = 0; i < recv->tail.message.pack.count; i++)
				{
					for (j = 0; j < recv->tail.message.pack.list.absolute[i].count; j++)
					{
						size = (recv->tail.message.pack.list.absolute[i].ends[j] - recv->tail.message.pack.list.absolute[i].begins[j] + 1) * 
							recv->tail.message.pack.list.absolute[i].elem_size;
						memcpy((recv->tail.message.pack.list.absolute[i].addr) + recv->tail.message.pack.list.absolute[i].begins[j] *
							recv->tail.message.pack.list.absolute[i].elem_size,send->tail.message.contiguous.addr,size);
						send->tail.message.contiguous.addr += size;
					}
				}
			}
			sctk_message_completion_and_free(send,recv);
			break;
		}
		default: not_reachable();
	}
}

/*
 * Function without description
 */
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
sctk_copy_buffer_absolute_std (sctk_pack_absolute_indexes_t * restrict in_begins,
			  sctk_pack_absolute_indexes_t * restrict in_ends,
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
  sctk_pack_absolute_indexes_t tmp_begin_abs[1];
  sctk_pack_absolute_indexes_t tmp_end_abs[1];
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
	  in_begins = tmp_begin_abs;
	  in_begins[0] = 0;
	  in_ends = tmp_end_abs;
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

/*
 * Function without description
 */
static inline void
sctk_copy_buffer_std_absolute (sctk_pack_indexes_t * restrict in_begins,
							   sctk_pack_indexes_t * restrict in_ends, 
							   size_t in_sizes,
							   void *restrict in_adress,
							   size_t in_elem_size,
							   sctk_pack_absolute_indexes_t * restrict out_begins,
							   sctk_pack_absolute_indexes_t * restrict out_ends, 
							   size_t out_sizes,
							   void *restrict out_adress,
							   size_t out_elem_size)
{
  sctk_pack_indexes_t tmp_begin[1];
  sctk_pack_indexes_t tmp_end[1];
  sctk_pack_absolute_indexes_t tmp_begin_abs[1];
  sctk_pack_absolute_indexes_t tmp_end_abs[1];
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
      sctk_debug ("sctk_copy_buffer_absolute_absolute mpc_pack");
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
	  out_begins = tmp_begin_abs;
	  out_begins[0] = 0;
	  out_ends = tmp_end_abs;
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

/*
 * Function without description
 */
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

/*
 * Function without description
 */
inline void sctk_message_copy_pack(sctk_message_to_copy_t* tmp)
{
	sctk_thread_ptp_message_t* send;
	sctk_thread_ptp_message_t* recv;

	send = tmp->msg_send;
	recv = tmp->msg_recv;

	assume(send->tail.message_type == sctk_message_pack);

	switch(recv->tail.message_type)
	{
		case sctk_message_pack: 
		{
			sctk_nodebug("sctk_message_pack - sctk_message_pack");
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
		case sctk_message_pack_absolute: 
		{
			sctk_nodebug("sctk_message_pack - sctk_message_pack_absolute");
			size_t i;
			for (i = 0; i < send->tail.message.pack.count; i++)
			{
					sctk_copy_buffer_std_absolute  (send->tail.message.pack.list.std[i].begins,
													send->tail.message.pack.list.std[i].ends,
													send->tail.message.pack.list.std[i].count,
													send->tail.message.pack.list.std[i].addr,
													send->tail.message.pack.list.std[i].elem_size,
													recv->tail.message.pack.list.absolute[i].begins,
													recv->tail.message.pack.list.absolute[i].ends,
													recv->tail.message.pack.list.absolute[i].count,
													recv->tail.message.pack.list.absolute[i].addr,
													recv->tail.message.pack.list.absolute[i].elem_size);
			}
			sctk_message_completion_and_free(send,recv);
			break;
		}
		case sctk_message_contiguous: 
		{
			sctk_nodebug("sctk_message_pack - sctk_message_contiguous");
			size_t i;
			size_t j;
			size_t size;
			char* body;

			body = recv->tail.message.contiguous.addr;

			sctk_nodebug("COUNT %lu",send->tail.message.pack.count);

			for (i = 0; i < send->tail.message.pack.count; i++)
			{
				for (j = 0; j < send->tail.message.pack.list.std[i].count; j++)
				{
					size = (send->tail.message.pack.list.std[i].ends[j] - send->tail.message.pack.list.std[i].begins[j] + 1) * 
						send->tail.message.pack.list.std[i].elem_size;
					memcpy(body,((char *) (send->tail.message.pack.list.std[i].addr)) + send->tail.message.pack.list.std[i].begins[j] * 
						send->tail.message.pack.list.std[i].elem_size,size);
					body += size;
				}
			}
			sctk_message_completion_and_free(send,recv);
			break;
		}
		default: not_reachable();
	}
}

/*
 * Function without description
 */
inline void sctk_message_copy_pack_absolute(sctk_message_to_copy_t* tmp)
{
	sctk_thread_ptp_message_t* send;
	sctk_thread_ptp_message_t* recv;

	send = tmp->msg_send;
	recv = tmp->msg_recv;

	assume(send->tail.message_type == sctk_message_pack_absolute);

	switch(recv->tail.message_type)
	{
		case sctk_message_pack: 
		{
			sctk_nodebug("sctk_message_pack_absolute - sctk_message_pack");
			size_t i;
			for (i = 0; i < send->tail.message.pack.count; i++)
			{
				sctk_copy_buffer_absolute_std (send->tail.message.pack.list.absolute[i].begins,
										  send->tail.message.pack.list.absolute[i].ends,
										  send->tail.message.pack.list.absolute[i].count,
										  send->tail.message.pack.list.absolute[i].addr,
										  send->tail.message.pack.list.absolute[i].elem_size,
										  recv->tail.message.pack.list.std[i].begins,
										  recv->tail.message.pack.list.std[i].ends,
										  recv->tail.message.pack.list.std[i].count,
										  recv->tail.message.pack.list.std[i].addr,
										  recv->tail.message.pack.list.std[i].elem_size);
			}
			sctk_message_completion_and_free(send,recv);
			break;
		}
		case sctk_message_pack_absolute: 
		{
			sctk_nodebug("sctk_message_pack_absolute - sctk_message_pack_absolute");
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
		case sctk_message_contiguous: 
		{
			sctk_nodebug("sctk_message_pack_absolute - sctk_message_contiguous");
			size_t i;
			size_t j;
			size_t size;
			char* body;

			body = recv->tail.message.contiguous.addr;

			sctk_nodebug("COUNT %lu",send->tail.message.pack.count);

			for (i = 0; i < send->tail.message.pack.count; i++)
			{
				for (j = 0; j < send->tail.message.pack.list.absolute[i].count; j++)
				{
					size = (send->tail.message.pack.list.absolute[i].ends[j] - send->tail.message.pack.list.absolute[i].begins[j] + 1) * 
						send->tail.message.pack.list.absolute[i].elem_size;
					memcpy(body,((char *) (send->tail.message.pack.list.absolute[i].addr)) + send->tail.message.pack.list.absolute[i].begins[j] * 
						send->tail.message.pack.list.absolute[i].elem_size,size);
					body += size;
				}
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

/*
 * Init data structures used for task i. Called only once for each task
 */
void sctk_ptp_per_task_init (int i){
  sctk_internal_ptp_t * tmp;
  int j;

  tmp = sctk_malloc(sizeof(sctk_internal_ptp_t));
  memset(tmp,0,sizeof(sctk_internal_ptp_t));
/*   tmp->key.comm = SCTK_COMM_WORLD; */
  tmp->key.destination = i;
  sctk_nodebug("Destination: %d", i);

  /* Initialize reordering for the list */
  sctk_reorder_list_init(&tmp->reorder);

  /* Initialize the internal ptp lists */
  sctk_internal_ptp_message_list_init(&(tmp->lists));
  /* And insert them */
  sctk_ptp_table_insert(tmp);

  /* Initialize the buffered_ptp_message list for the VP */
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

/*
 * Free the header. If the header is a from the buffered list, re-add it to
 * the list. Else, free the header.
 */
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

/*
 * Allocate a free header. Take it from the buffered list is an entry is free. Else,
 * allocate a new header.
 */
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

/*
 * Determine the source process from the 'body' of a message
 */
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

/*  Determine what is the global source and destination of one message */
void sctk_determine_glob_source_and_destination_from_header (sctk_thread_ptp_message_body_t* body,
    int *glob_source, int *glob_destination) {
  if(IS_PROCESS_SPECIFIC_MESSAGE_TAG(body->header.specific_message_tag)){
    if(body->header.source != MPC_ANY_SOURCE)
      *glob_source = body->header.source;
    else
      *glob_source = -1;
    *glob_destination = -1;
  } else {
    if(body->header.source != MPC_ANY_SOURCE)
      *glob_source = sctk_get_comm_world_rank (body->header.communicator, body->header.source);
    else
      *glob_source = -1;
    *glob_destination = sctk_get_comm_world_rank (body->header.communicator, body->header.destination);
  }
}

/*
 * Rebuild the header of a received message. Set the source, destination, global source and
 * global destination.
 */
void sctk_rebuild_header (sctk_thread_ptp_message_t * msg){
  if(IS_PROCESS_SPECIFIC_MESSAGE_TAG(msg->sctk_msg_get_specific_message_tag)){
    if(msg->sctk_msg_get_source != MPC_ANY_SOURCE)
      msg->sctk_msg_get_glob_source = msg->sctk_msg_get_source;
    else
      msg->sctk_msg_get_glob_source = -1;

    msg->sctk_msg_get_glob_destination = -1;
  } else {
    if(msg->sctk_msg_get_source != MPC_ANY_SOURCE) {
      /* Convert the MPI source rank to MPI_COMM_WORLD according to the communicator */
      //~ msg->sctk_msg_get_glob_source = sctk_get_comm_world_rank (msg->sctk_msg_get_communicator,msg->sctk_msg_get_source);
    } else {
      /* Source task not available */
      msg->sctk_msg_get_glob_source = -1;
    }

    /* Convert the MPI source rank to MPI_COMM_WORLD according to the communicator */
    //~ msg->sctk_msg_get_glob_destination = sctk_get_comm_world_rank (msg->sctk_msg_get_communicator,msg->sctk_msg_get_destination);
  }

  msg->tail.need_check_in_wait = 1;
  msg->tail.request = NULL;
}

/*
 * Reinit the header of the message. Set the 'free_memory' and 'message_copy' funcs.
 */
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
  tmp->tail.internal_ptp = NULL;

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
  /* The copy and free functions will be set after */
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
			       const size_t count,
			       int request_type){
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
  request->request_type = request_type;
}

void sctk_set_header_in_message (sctk_thread_ptp_message_t *
				 msg, const int message_tag,
				 const sctk_communicator_t communicator,
				 const int source,
				 const int destination,
				 sctk_request_t * request,
				 const size_t count,
				 specific_message_tag_t specific_message_tag)
{
	msg->tail.request = request;
	msg->sctk_msg_get_source = source;
	msg->sctk_msg_get_destination = destination;
  
	if(IS_PROCESS_SPECIFIC_MESSAGE_TAG(specific_message_tag))
	{
		if(source != MPC_ANY_SOURCE) 
		{
			msg->sctk_msg_get_glob_source = source;
		}
		else
			msg->sctk_msg_get_glob_source = -1;

		msg->sctk_msg_get_glob_destination = -1;
	} 
	else 
	{
		if(source != MPC_ANY_SOURCE) 
		{
			/* If the communicator used is the COMM_SELF */
			if (communicator == SCTK_COMM_SELF) 
			{
				/* The world destination is actually ourself :) */
				int world_src = sctk_get_task_rank ();
				msg->sctk_msg_get_glob_source = sctk_get_comm_world_rank (communicator, world_src);
			} 
			else 
			{
				if((sctk_is_inter_comm(communicator)) && ((request->request_type == REQUEST_RECV) || (request->request_type == REQUEST_RECV_COLL)))
				{
					sctk_nodebug("recv : get comm world rank of rank %d in comm %d", source, communicator);
					msg->sctk_msg_get_glob_source = sctk_get_remote_comm_world_rank (communicator, source);
				}
				else
					msg->sctk_msg_get_glob_source = sctk_get_comm_world_rank (communicator, source);
			}
		}
		else
			msg->sctk_msg_get_glob_source = -1;

		/* If the communicator used is the COMM_SELF */
		if (communicator == SCTK_COMM_SELF) 
		{
			/* The world destination is actually ourself :) */
			int world_dest = sctk_get_task_rank ();
			msg->sctk_msg_get_glob_destination = sctk_get_comm_world_rank (communicator, world_dest);
		} else 
		{
			if((sctk_is_inter_comm(communicator)) && ((request->request_type == REQUEST_SEND) || (request->request_type == REQUEST_SEND_COLL)))
			{
				sctk_nodebug("send : get comm world rank of rank %d in comm %d", destination, communicator);
				msg->sctk_msg_get_glob_destination = sctk_get_remote_comm_world_rank (communicator, destination);
			}
			else
				msg->sctk_msg_get_glob_destination = sctk_get_comm_world_rank (communicator, destination);
		}
	}
    
	msg->body.header.communicator = communicator;
	msg->body.header.message_tag = message_tag;
	msg->body.header.specific_message_tag = specific_message_tag;
	msg->body.header.msg_size = count;

	msg->sctk_msg_get_use_message_numbering = 1;
/*  
	if((request->request_type == REQUEST_SEND) || (request->request_type == REQUEST_SEND_COLL))
		sctk_debug("Send comm %d : rang %d envoie message a rang %d", communicator, msg->sctk_msg_get_glob_source, msg->sctk_msg_get_glob_destination);
	else if((request->request_type == REQUEST_RECV) || (request->request_type == REQUEST_RECV_COLL))
		sctk_debug("Recv comm %d : rang %d recoit message de rang %d", communicator, msg->sctk_msg_get_glob_destination, msg->sctk_msg_get_glob_source);
*/	
	/* A message can be sent with a NULL request (see the MPI standard) */
	if (request) 
	{
		sctk_request_init_request(request,SCTK_MESSAGE_PENDING,msg,source,destination,message_tag,communicator, count, request->request_type);
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
/* Searching functions                                              */
/********************************************************************/

/*
 * Function which tries to match a send request ('header' parameter)
 * from a list of 'send' pending messages for a recv request
 */
static inline
sctk_msg_list_t* sctk_perform_messages_search_matching(
    sctk_internal_ptp_list_pending_t *pending_list, sctk_thread_message_header_t* header){
  sctk_msg_list_t* ptr_found;
  sctk_msg_list_t* tmp;

  /* Loop on all  pending messages */
  DL_FOREACH_SAFE(pending_list->list,ptr_found,tmp){
	sctk_thread_message_header_t* header_found;
	sctk_assert(ptr_found->msg != NULL);
	header_found = &(ptr_found->msg->body.header);
	
	/* Match the communicator, the tag, the source and the specific message tag */
	if((header->communicator == header_found->communicator) &&
	   (header->specific_message_tag == header_found->specific_message_tag) &&
	   ((header->source == header_found->source) || (header->source == MPC_ANY_SOURCE))&&
	   ((header->message_tag == header_found->message_tag) || (header->message_tag == MPC_ANY_TAG))){
	  /* Message found. We delete it  */
	  DL_DELETE(pending_list->list,ptr_found);
	  /* We return the pointer to the request */
	  return ptr_found;
	}
  }
  return NULL;
}

/*
 * Probe for a matching message
 */
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

  TODO("Add another function pointer to indicate that we have matched a message from known process")
#if 0
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
#endif
  return 0;
}

static inline int sctk_perform_messages_matching_from_recv_msg(sctk_internal_ptp_t* pair,
    sctk_thread_ptp_message_t * msg){
  sctk_msg_list_t* ptr_recv;
  sctk_msg_list_t* ptr_send;

  sctk_assert(msg != NULL);
  ptr_recv = &msg->tail.distant_list;
  ptr_send = sctk_perform_messages_search_matching(
      &pair->lists.pending_send, &(msg->body.header));

  /* We found a send request which corresponds to the recv request 'ptr_recv' */
  if(ptr_send != NULL){
    DL_DELETE(pair->lists.pending_recv.list, ptr_recv);

    /* If the remote source is on a another node, we call the
     * notify matching hook in the inter-process layer. We do it
     * before copying the message to the receive buffer */
    if(msg->tail.remote_source){
      sctk_network_notify_matching_message (msg);
    }

    /*Insert the matching message to the list of messages that needs to be copies.
     * The message is copied inside the copy_tasks_insert function if the task engine
     * is disabled */
    sctk_ptp_copy_tasks_insert(ptr_recv,ptr_send,pair);
    return 1;
  }
  return 0;
}

/*
 * Function with loop on the recv 'pending' messages and try to match all messages in this
 * list with the send 'pending' messages. This function must be called protected by locks.
 * The function returns the number of messages copied (implies that this messages where
 * matched).
 */
static inline int sctk_perform_messages_for_pair_locked(sctk_internal_ptp_t* pair){
  sctk_msg_list_t* ptr_recv;
  sctk_msg_list_t* ptr_send;
  sctk_msg_list_t* tmp;
  int nb_messages_copied = 0;

  sctk_internal_ptp_merge_pending(&(pair->lists));

  DL_FOREACH_SAFE(pair->lists.pending_recv.list,ptr_recv,tmp){
    nb_messages_copied +=
      sctk_perform_messages_matching_from_recv_msg(pair, ptr_recv->msg);
  }
  return nb_messages_copied;
}

static inline int sctk_perform_messages_for_pair(sctk_internal_ptp_t* pair){
  int nb_messages_copied = 0;

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
      sctk_internal_ptp_lock_pending(&(pair->lists));
      nb_messages_copied = sctk_perform_messages_for_pair_locked(pair);
        pair->lists.changed = 0;
      sctk_internal_ptp_unlock_pending(&(pair->lists));
      }
    }
#ifndef SCTK_DISABLE_TASK_ENGINE
  /* Call the task engine if it is not diabled */
  return sctk_ptp_tasks_perform(pair);
#else
  return nb_messages_copied;
#endif
}

/*
 * Try to match all received messages with send requests from a internal ptp list
 *
 * Returns:
 *  -1 Lock already taken
 *  >=0 Number of messages matched
 */
static inline int sctk_try_perform_messages_for_pair(sctk_internal_ptp_t* pair){
  /* If the lock has not been taken, we continue */
  if (pair->lists.pending_lock == 0) {
    return sctk_perform_messages_for_pair(pair);
  } else {
    return -1;
  }
  return 0;
}

void sctk_perform_messages_wait_init(
    struct sctk_perform_messages_s * wait, sctk_request_t * request) {
  wait->request = request;
  int * remote_process = &wait->remote_process;
  int * source_task_id = &wait->source_task_id;
  sctk_internal_ptp_t **recv_ptp = &wait->recv_ptp;
  sctk_internal_ptp_t **send_ptp = &wait->send_ptp;
  sctk_comm_dest_key_t recv_key;
  sctk_comm_dest_key_t send_key;
  int remote_task;
  /* key.comm = request->header.communicator; */
  recv_key.destination = request->header.glob_destination;
  send_key.destination = request->header.glob_source;

  sctk_ptp_table_read_lock();
  /* Searching for the pending list corresponding to the
   * dest task */
  sctk_ptp_table_find(recv_key, *recv_ptp);
  sctk_ptp_table_find(send_key, *send_ptp);
  sctk_ptp_table_read_unlock();

  /* Compute the rank of the remote process */
  if(request->header.source != MPC_ANY_SOURCE && request->header.communicator != MPC_COMM_NULL){
    /* Convert task rank to process rank */
    *source_task_id = sctk_get_comm_world_rank (request->header.communicator,request->header.source);
    *remote_process = sctk_get_process_rank_from_task_rank(*source_task_id);
    sctk_nodebug("remote process=%d source=%d comm=%d", *remote_process, request->header.source, request->header.communicator);
  } else {
    *remote_process = -1;
    *source_task_id = -1;
  }
}

/*
 *  Function called when the message to receive is already completed
 */
static void sctk_perform_messages_done(struct sctk_perform_messages_s * wait) {
    const sctk_request_t* request = wait->request;
    const sctk_internal_ptp_t *recv_ptp = wait->recv_ptp;
    const int remote_process = wait->remote_process;
    const int source_task_id = wait->source_task_id;

  /* The message is marked as done.
   * However, we need to poll if it is a inter-process message
   * and if we are wating for a SEND request. If we do not do this,
   * we might overflow the number of send buffers waiting to be released
   */
  if (request->header.source == MPC_ANY_SOURCE) {
    sctk_network_notify_any_source_message (request->header.glob_source);
  } else if (request->request_type == REQUEST_SEND && !recv_ptp) {
    sctk_network_notify_perform_message (remote_process, source_task_id,
        request->header.glob_source);
  }
}


static void sctk_perform_messages_wait_for_value_and_poll(void* a){
  struct sctk_perform_messages_s *_wait = (struct sctk_perform_messages_s*) a;

  sctk_perform_messages(_wait);

  if ((volatile int) _wait->request->completion_flag != SCTK_MESSAGE_DONE) {
    sctk_network_notify_idle_message();
  }

}

void sctk_perform_messages_wait_init_request_type(struct sctk_perform_messages_s * wait) {
  sctk_request_t* request = wait->request;
    /* INFO: The polling task id may be -1. For example when the fully connected mode
     * is enabled */
  int * polling_task_id = &wait->polling_task_id;

  if (request->request_type == REQUEST_SEND) {
      *polling_task_id = request->header.glob_source;
    } else if (request->request_type == REQUEST_RECV) {
      *polling_task_id = request->header.glob_destination;
    } else {
      not_reachable();
    };
}

void sctk_wait_message (sctk_request_t * request)
{
  struct sctk_perform_messages_s _wait;

  sctk_perform_messages_wait_init(&_wait, request);

  /* Find the PTPs lists */
  if(request->completion_flag != SCTK_MESSAGE_DONE){

    sctk_perform_messages_wait_init_request_type(&_wait);

    sctk_nodebug("Wait from %d to %d (req %p %d) (%p - %p) %d",
        request->header.glob_source, request->header.glob_destination, request, request->request_type, _wait.send_ptp, _wait.recv_ptp, request->header.message_tag);

#ifdef SCTK_ENABLE_FULL_MPI
    while ((volatile int) _wait.request->completion_flag != SCTK_MESSAGE_DONE) {
      sctk_perform_messages_wait_for_value_and_poll(&_wait);
      /* Relax if message not found */
      if ((volatile int) _wait.request->completion_flag != SCTK_MESSAGE_DONE) {
        sctk_cpu_relax();
      }
    }
#else
  /* Wait until the message is received */
  if(_wait.request->completion_flag != SCTK_MESSAGE_DONE){
    sctk_thread_wait_for_value_and_poll
      ((int *) &(_wait.request->completion_flag),SCTK_MESSAGE_DONE ,
       (void(*)(void*))sctk_perform_messages_wait_for_value_and_poll, &_wait);
  }
#endif
  } else {
    sctk_perform_messages_done(&_wait);
  }


  sctk_nodebug("Wait DONE from %d to %d (req %p %d) (%p - %p)",
      request->header.glob_source, request->header.glob_destination, request, request->request_type, _wait.send_ptp, _wait.recv_ptp);
}

/*
 * Function called for performing message from a specific request.
 *
 * Firstly, we check if the message to receive is an intra-node message.
 * If it is, we walk on the pending list of the receive task in order
 * to find a message to match.
 *
 * If the message is an inter-node message, we call the inter-node
 * polling function according to the source of the message to
 * receive (MPC_ANY_SOURCE or not).
 *
 */
 
void sctk_perform_messages(struct sctk_perform_messages_s * wait){
    const sctk_request_t* request = wait->request;
    sctk_internal_ptp_t *recv_ptp = wait->recv_ptp;
    sctk_internal_ptp_t *send_ptp = wait->send_ptp;
    const int remote_process = wait->remote_process;
    const int source_task_id = wait->source_task_id;
    const int polling_task_id = wait->polling_task_id;

  if(request->completion_flag != SCTK_MESSAGE_DONE){
    /* Check the source of the request. We try to poll the
     * source in order to retreive messages from the network */
    if(request->header.source == MPC_ANY_SOURCE){
      /* We try to poll for finding a message with a MPC_ANY_SOURCE source */
      sctk_network_notify_any_source_message (polling_task_id);
    } else if ( (request->request_type == REQUEST_SEND && !recv_ptp) ||
        (request->request_type == REQUEST_RECV && !send_ptp) ) {
      /* We poll the network only if we need it */
      sctk_network_notify_perform_message (remote_process, source_task_id, polling_task_id);
    } else if (request->header.source == request->header.destination) {
      /* If the src and the dest tasks are same, we pool the network.
       * INFO: it is usefull for the overlap benchmark from the MPC_THREAD_MULTIPLE Test Suite.
       * An additional thread is created and waiting ob MPI_Recv with src = dest */
      sctk_network_notify_perform_message (remote_process, source_task_id, polling_task_id);
    }

    if(request->request_type == REQUEST_SEND) {
      sctk_try_perform_messages_for_pair(send_ptp);
    } else if (request->request_type == REQUEST_RECV) {
      sctk_try_perform_messages_for_pair(recv_ptp);
    } else not_reachable();
  } else {
    sctk_perform_messages_done(wait);
  }
}

/*
 * Wait for all message according to a communicator and a task id
 */
TODO("We should add an option in the MPC config to choose if we support "
      "the wait_all function. If not, we could delete the useless locks")
void sctk_wait_all (const int task, const sctk_communicator_t com){
  sctk_internal_ptp_t* pair;
  int i;

  /* Get the pending list associated to the task */
  pair = sctk_get_internal_ptp(task);
  sctk_assert(pair);

  do{
    i = OPA_load_int(&pair->pending_nb);

    if (i != 0) {
      /* WARNING: The inter-process module *MUST* implement
       * the 'notify_any_source_message function'. If not,
       * the polling will never occur. */
      sctk_network_notify_any_source_message (task);
      sctk_try_perform_messages_for_pair(pair);

      /* Second test to avoid a thread_yield if the message
       * has been handled */
      i = OPA_load_int(&pair->pending_nb);
      if (i != 0) sctk_thread_yield();
    }
  } while(i != 0);
}

void sctk_notify_idle_message (){
  /* sctk_perform_all (); */
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

    if(msg->tail.request != NULL){
      msg->tail.request->request_type = REQUEST_SEND;
    }

    msg->tail.remote_destination = 1;
    sctk_nodebug("Need to forward the message to %d", msg->sctk_msg_get_destination);
    /* We forward the message */
    sctk_network_send_message (msg);
  } else {
    sctk_comm_dest_key_t src_key, dest_key;
    sctk_internal_ptp_t * src_pair, * dest_pair;

    dest_key.destination = msg->sctk_msg_get_glob_destination;
    src_key.destination = msg->sctk_msg_get_glob_source;
	
    assume(msg->sctk_msg_get_communicator >= 0);

    if(msg->body.completion_flag != NULL){
      *(msg->body.completion_flag) = SCTK_MESSAGE_PENDING;
    }

    if(msg->tail.request != NULL){
      msg->tail.request->need_check_in_wait = msg->tail.need_check_in_wait;
      msg->tail.request->request_type = REQUEST_SEND;
      sctk_nodebug("Request %p %d", msg->tail.request, msg->tail.request->request_type);
    }

    msg->tail.remote_source = 0;
    msg->tail.remote_destination = 0;

    /* We are searching for the corresponding pending list.
     * If we do not find any entry, we forward the message */
    sctk_ptp_table_read_lock();
    sctk_ptp_table_find(dest_key,dest_pair);
    sctk_ptp_table_find(src_key,src_pair);
    sctk_ptp_table_read_unlock();
    
    if (src_pair == NULL) {
      assume(dest_pair);
      sctk_internal_ptp_add_pending(dest_pair,msg);
    } else {
      sctk_internal_ptp_add_pending(src_pair,msg);
    }

    if(dest_pair != NULL){
      sctk_internal_ptp_add_send_incomming(dest_pair,msg);
      /* If we ask for a checking, we check it */
      if(perform_check) {
        /* Try to match the message for the remote task */
        int matched_nb = sctk_try_perform_messages_for_pair(dest_pair);
#ifdef MPC_Profiler
        switch (matched_nb) {
          case -1:
            SCTK_COUNTER_INC(matching_locked, 1);
            break;
          case 0:
            SCTK_COUNTER_INC(matching_not_found, 1);
            break;
          default:
            SCTK_COUNTER_INC(matching_found, 1);
            break;
        }
#endif
      }
    } else {
      sctk_nodebug("Need to forward the message");
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
  int need_check = 1;
  /* TODO: need to check in wait */
#warning "disable checking in wait"
  msg->tail.need_check_in_wait = 1;
  /* We only check if the message is an intra-node message */
  if ( msg->sctk_msg_get_glob_destination != -1 ) {
    if(!sctk_is_net_message(msg->sctk_msg_get_glob_destination) ) {
      sctk_nodebug("Request for %d", msg->sctk_msg_get_glob_destination);
      msg->tail.need_check_in_wait = 0;
    }
  }
  sctk_send_message_try_check(msg,need_check);
}

/********************************************************************/
/* Recv messages                                                    */
/********************************************************************/

/*
 * Function called when receiving a message.
 * */
void sctk_recv_message_try_check (sctk_thread_ptp_message_t * msg,sctk_internal_ptp_t* recv_ptp,int perform_check){
  sctk_comm_dest_key_t send_key;
  sctk_internal_ptp_t* send_ptp = NULL;

  send_key.destination = msg->sctk_msg_get_glob_source;

  if(msg->body.completion_flag != NULL){
    *(msg->body.completion_flag) = SCTK_MESSAGE_PENDING;
  }

  if(msg->tail.request != NULL){
    msg->tail.request->request_type = REQUEST_RECV;
    msg->tail.request->need_check_in_wait = msg->tail.need_check_in_wait;
  }

  msg->tail.remote_source = 0;
  msg->tail.remote_destination = 0;

  /* We are searching for the list corresponding to the
   * task which receives the message */
  sctk_ptp_table_read_lock();
  if(recv_ptp == NULL){
    sctk_comm_dest_key_t key;
    key.destination = msg->sctk_msg_get_glob_destination;
    sctk_ptp_table_find(key,recv_ptp);
  }

  if(!IS_PROCESS_SPECIFIC_MESSAGE_TAG(msg->body.header.specific_message_tag)){
    if(send_key.destination != -1){
      sctk_ptp_table_find(send_key,send_ptp);
    }
  }
  sctk_ptp_table_read_unlock();
  /* We assume that the entry is found. If not, the message received is
   * for a task which is not registered on the node. Possible issue. */
  assume(recv_ptp);

  if(send_ptp == NULL){
    msg->tail.need_check_in_wait = 1;
    /*Entering low level comm*/
    msg->tail.remote_source = 1;
    sctk_network_notify_recv_message (msg);
  }

  /* We add the message to the pending list */
  sctk_nodebug("Post recv from %d to %d (%d) %p",
          msg->sctk_msg_get_glob_source, msg->sctk_msg_get_glob_destination, msg->sctk_msg_get_message_tag, msg->tail.request);
  sctk_internal_ptp_add_pending(recv_ptp,msg);
  sctk_internal_ptp_add_recv_incomming(recv_ptp,msg);
  /* Iw we ask for a matching, we run it */
  if(perform_check){
    sctk_try_perform_messages_for_pair(recv_ptp);
  }
}

/*
 * Function called for receiving a message (i.e: MPI_Recv).
 * Mostly used by the file mpc.c
 * */
void sctk_recv_message (sctk_thread_ptp_message_t * msg,sctk_internal_ptp_t* tmp,
    int need_check){
  msg->tail.need_check_in_wait = 1;
  sctk_recv_message_try_check(msg,tmp, need_check);
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
  sctk_comm_dest_key_t dest_key, src_key;
  sctk_internal_ptp_t* dest_ptp;
  sctk_internal_ptp_t* src_ptp = NULL;
  int world_source;
  int world_destination;

  msg->source = source;
  msg->destination = destination;
  msg->message_tag = tag;
  msg->communicator = comm;
  msg->specific_message_tag = pt2pt_specific_message_tag;

  world_destination = sctk_get_comm_world_rank (comm,destination);
  dest_key.destination = world_destination;
  if (source != MPC_ANY_SOURCE) {
    world_source = sctk_get_comm_world_rank (comm, source);
    src_key.destination = world_source;
  }

  sctk_ptp_table_read_lock();
  sctk_ptp_table_find(dest_key,dest_ptp);
  if (source != MPC_ANY_SOURCE) {
    sctk_ptp_table_find(src_key,src_ptp);
  }
  sctk_ptp_table_read_unlock();

  if (source == MPC_ANY_SOURCE || src_ptp == NULL) {
    int src_process = sctk_get_process_rank_from_task_rank(world_source);
    sctk_network_notify_perform_message (src_process, world_source, world_destination);
  }

  assume(dest_ptp != NULL);
  sctk_internal_ptp_lock_pending(&(dest_ptp->lists));
  *status = sctk_perform_messages_probe_matching(dest_ptp,msg);
  sctk_nodebug("Find source %d tag %d found ?%d",msg->source,msg->message_tag,*status);
  sctk_internal_ptp_unlock_pending(&(dest_ptp->lists));
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
