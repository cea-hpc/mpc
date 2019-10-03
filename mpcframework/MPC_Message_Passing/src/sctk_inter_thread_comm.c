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

#include <sctk_debug.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <mpc_common_asm.h>
#include <sctk_checksum.h>
#include <sctk_reorder.h>
#include <sctk_control_messages.h>
#include <sctk_rail.h>
#include <sctk_launch.h>

#ifdef SCTK_LIB_MODE
#include "sctk_handle.h"
#endif /* SCTK_LIB_MODE */

#ifdef MPC_MPI
#include <mpc_datatypes.h>
#endif


static inline int _sctk_is_net_message(int dest);



/********************************************************************/
/*Structres                                                         */
/********************************************************************/

typedef struct
{
	sctk_communicator_t comm;
	int rank;
} mpc_comm_dest_key_t;

static inline void _mpc_comm_dest_key_init( mpc_comm_dest_key_t* key, sctk_communicator_t comm, int rank )
{
	key->rank = rank;
	key->comm = comm;
}

typedef struct
{
	mpc_common_spinlock_t lock;
	volatile sctk_msg_list_t *list;
} mpc_comm_ptp_list_incomming_t;

typedef struct
{
	volatile sctk_msg_list_t *list;
} mpc_comm_ptp_list_pending_t;

typedef struct
{
	/* Messages are posted to the 'incoming' lists before being merged to the pending list. */
	mpc_comm_ptp_list_incomming_t incomming_send;
	mpc_comm_ptp_list_incomming_t incomming_recv;

	mpc_common_spinlock_t pending_lock;
	/* Messages in the 'pending' lists are waiting to be matched */
	mpc_comm_ptp_list_pending_t pending_send;
	mpc_comm_ptp_list_pending_t pending_recv;
	/* Flag to indicate that new messages have been merged into the 'pending' list */
	char changed;

} mpc_comm_ptp_message_lists_t;

typedef struct mpc_comm_ptp_s
{
	mpc_comm_dest_key_t key;

	mpc_comm_ptp_message_lists_t lists;

	/* Number of pending messages for the MPI task */
	OPA_int_t pending_nb;

	UT_hash_handle hh;

	/* Reorder table */
	sctk_reorder_list_t reorder;
} mpc_comm_ptp_t;


static inline void __mpc_comm_request_init( sctk_request_t *request,
					    sctk_communicator_t comm,
					    int request_type )
{

	static sctk_request_t the_initial_request = {
		.header.source = SCTK_PROC_NULL,
		.header.destination = SCTK_PROC_NULL,
		.header.source_task = SCTK_PROC_NULL,
		.header.destination_task = SCTK_PROC_NULL,
		.header.message_tag = SCTK_ANY_TAG,
		.header.communicator = SCTK_COMM_NULL,
		.header.msg_size = 0,
		.completion_flag = SCTK_MESSAGE_DONE,
		.request_type = 0,
		.is_null = 0,
		.truncated = 0,
		.msg = NULL,
		.query_fn = NULL,
		.cancel_fn = NULL,
		.wait_fn = NULL,
		.poll_fn = NULL,
		.free_fn = NULL,
		.extra_state = NULL,
		.pointer_to_source_request = NULL,
		.pointer_to_shadow_request = NULL,
		.ptr_to_pin_ctx = NULL};

	if ( request != NULL )
	{
		*request = the_initial_request;
		request->request_type = request_type;
		request->header.communicator = comm;
	}
}


/********************************************************************/
/*Functions                                                         */
/********************************************************************/

/*
 * Initialize the 'incoming' list.
 */

static inline void __mpc_comm_ptp_list_incoming_init(mpc_comm_ptp_list_incomming_t *list)
{
	list->list = NULL;
	list->lock = SCTK_SPINLOCK_INITIALIZER;
}

/*
 * Initialize the 'pending' list
 */
static inline void __mpc_comm_ptp_list_pending_init( mpc_comm_ptp_list_pending_t *list )
{
	list->list = NULL;
}

/*
 * Initialize a PTP the 'incoming' and 'pending' message lists.
 */
static inline void __mpc_comm_ptp_message_list_init( mpc_comm_ptp_message_lists_t *lists )
{
	__mpc_comm_ptp_list_incoming_init( &( lists->incomming_send ) );
	__mpc_comm_ptp_list_incoming_init( &( lists->incomming_recv ) );

	lists->pending_lock = SCTK_SPINLOCK_INITIALIZER;
	__mpc_comm_ptp_list_pending_init( &( lists->pending_send ) );
	__mpc_comm_ptp_list_pending_init( &( lists->pending_recv ) );
}

/*
 * Merge 'incoming' send and receive lists to 'pending' send and receive lists
 */
static inline void __mpc_comm_ptp_message_list_merge_pending( mpc_comm_ptp_message_lists_t *lists )
{

	if ( lists->incomming_send.list != NULL )
	{
		if ( mpc_common_spinlock_trylock( &( lists->incomming_send.lock ) ) == 0 )
		{
			DL_CONCAT( lists->pending_send.list,
					   (sctk_msg_list_t *) lists->incomming_send.list );
			lists->incomming_send.list = NULL;
			mpc_common_spinlock_unlock( &( lists->incomming_send.lock ) );
		}
	}

	if ( lists->incomming_recv.list != NULL )
	{
		if ( mpc_common_spinlock_trylock( &( lists->incomming_recv.lock ) ) == 0 )
		{
			DL_CONCAT( lists->pending_recv.list,
					   (sctk_msg_list_t *) lists->incomming_recv.list );
			lists->incomming_recv.list = NULL;
			mpc_common_spinlock_unlock( &( lists->incomming_recv.lock ) );
		}
	}

	/* Change the flag. New messages have been inserted to the 'pending' lists */
	lists->changed = 1;
}

static inline void __mpc_comm_ptp_message_list_lock_pending( mpc_comm_ptp_message_lists_t *lists )
{
	mpc_common_spinlock_lock_yield( &( lists->pending_lock ) );
}

static inline int __mpc_comm_ptp_message_list_trylock_pending( mpc_comm_ptp_message_lists_t *lists )
{
	return mpc_common_spinlock_trylock( &( lists->pending_lock ) );
}

static inline void __mpc_comm_ptp_message_list_unlock_pending( mpc_comm_ptp_message_lists_t *lists )
{
	mpc_common_spinlock_unlock( &( lists->pending_lock ) );
}

/*
 * Add message into the 'pending' recv list
 */
static inline void __mpc_comm_ptp_message_list_add_pending( mpc_comm_ptp_t *tmp,
						  sctk_thread_ptp_message_t *msg )
{
	msg->tail.internal_ptp = tmp;
	assume( tmp );
	OPA_incr_int( &tmp->pending_nb );
}

/*
 * Add message into the 'incoming' recv list
 */
static inline void __mpc_comm_ptp_message_list_add_incoming_recv( mpc_comm_ptp_t *tmp,
																  sctk_thread_ptp_message_t *msg )
{
	msg->tail.distant_list.msg = msg;
	mpc_common_spinlock_lock( &( tmp->lists.incomming_recv.lock ) );
	DL_APPEND( tmp->lists.incomming_recv.list, &( msg->tail.distant_list ) );
	mpc_common_spinlock_unlock( &( tmp->lists.incomming_recv.lock ) );
}
/*
 * Add message into the 'incoming' send list
 */
static inline void __mpc_comm_ptp_message_list_add_incoming_send( mpc_comm_ptp_t *tmp,
																  sctk_thread_ptp_message_t *msg )
{
	msg->tail.distant_list.msg = msg;
	mpc_common_spinlock_lock( &( tmp->lists.incomming_send.lock ) );
	DL_APPEND( tmp->lists.incomming_send.list, &( msg->tail.distant_list ) );
	mpc_common_spinlock_unlock( &( tmp->lists.incomming_send.lock ) );
}

/************************************************************************/
/*Data structure accessors                                              */
/************************************************************************/

static mpc_comm_ptp_t ***sctk_ptp_array = NULL;
static int sctk_ptp_array_start = 0;
static int sctk_ptp_array_end = 0;

/*
 * Find an internal ptp according the a key (this key represents the task
 * number)
 */

static inline struct mpc_comm_ptp_s *__mpc_comm_ptp_array_get_key( mpc_comm_dest_key_t *key )
{
	int __dest__id = key->rank - sctk_ptp_array_start;

	if ( ( sctk_ptp_array != NULL ) && ( __dest__id >= 0 ) &&
		 ( __dest__id <= sctk_ptp_array_end - sctk_ptp_array_start ) )
	{
		return sctk_ptp_array[__dest__id]
							 [key->comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER];
	}

	return NULL;
}

static inline struct mpc_comm_ptp_s *__mpc_comm_ptp_array_get( sctk_communicator_t comm, int rank )
{
	mpc_comm_dest_key_t key;
	_mpc_comm_dest_key_init( &key, comm, rank );
	return __mpc_comm_ptp_array_get_key( &key );
}

struct mpc_comm_ptp_s *_mpc_comm_ptp_array_get( sctk_communicator_t comm, int rank )
{
	return __mpc_comm_ptp_array_get( comm, rank );
}

sctk_reorder_list_t *_mpc_comm_ptp_array_get_reorder( sctk_communicator_t communicator, int rank )
{
	struct mpc_comm_ptp_s *internal_ptp = __mpc_comm_ptp_array_get( communicator, rank );

	sctk_assert( internal_ptp );

	return &( internal_ptp->reorder );
}

/*
 * Insert a new entry to the PTP table. The function checks if the entry is
 * already prensent
 * and fail in this case
 */
static inline void __mpc_comm_ptp_array_insert( mpc_comm_ptp_t *tmp )
{
	static mpc_common_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
	static volatile int done = 0;

	mpc_common_spinlock_lock( &lock );

	/* Only one task allocates the structures */
	if ( done == 0 )
	{

		sctk_ptp_array_start = sctk_get_first_task_local( SCTK_COMM_WORLD );
		sctk_ptp_array_end = sctk_get_last_task_local( SCTK_COMM_WORLD );
		sctk_ptp_array =
			sctk_malloc( ( sctk_ptp_array_end - sctk_ptp_array_start + 1 ) *
						 sizeof( mpc_comm_ptp_t ** ) );
		memset( sctk_ptp_array, 0,
				( sctk_ptp_array_end - sctk_ptp_array_start + 1 ) *
					sizeof( mpc_comm_ptp_t ** ) );
		done = 1;
	}

	assume( tmp->key.rank >= sctk_ptp_array_start );
	assume( tmp->key.rank <= sctk_ptp_array_end );

	if ( sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] == NULL )
	{
		sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] = sctk_malloc(SCTK_PARALLEL_COMM_QUEUES_NUMBER * sizeof( mpc_comm_ptp_t * ));
		memset( sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start], 0, SCTK_PARALLEL_COMM_QUEUES_NUMBER * sizeof( mpc_comm_ptp_t * ) );
	}

	assume( sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start][tmp->key.comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER] == NULL );

	sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] [tmp->key.comm % SCTK_PARALLEL_COMM_QUEUES_NUMBER] = tmp;

	mpc_common_spinlock_unlock( &lock );
}

/********************************************************************/
/*Task engine                                                       */
/********************************************************************/

/* Messages in the 'sctk_ptp_task_list' have already been
 * matched and are wainting to be copied */
sctk_message_to_copy_t **sctk_ptp_task_list = NULL;
mpc_common_spinlock_t *sctk_ptp_tasks_lock = 0;
int sctk_ptp_tasks_count = 0;

static short sctk_ptp_tasks_init_done = 0;
mpc_common_spinlock_t sctk_ptp_tasks_init_lock = 0;

#define PTP_MAX_TASK_LISTS 32

void sctk_ptp_tasks_init()
{
  mpc_common_spinlock_lock(&sctk_ptp_tasks_init_lock);

  if( sctk_ptp_tasks_init_done )
  {
     mpc_common_spinlock_unlock(&sctk_ptp_tasks_init_lock);
     return;
  }

  if( mpc_common_get_task_count() < PTP_MAX_TASK_LISTS)
  {
    sctk_ptp_tasks_count = mpc_common_get_task_count();
  }
  else
  {
    sctk_ptp_tasks_count = PTP_MAX_TASK_LISTS;
  }

  sctk_ptp_task_list = sctk_malloc(sizeof(sctk_message_to_copy_t*) * sctk_ptp_tasks_count);
  assume(sctk_ptp_task_list);

  sctk_ptp_tasks_lock = sctk_malloc(sizeof(mpc_common_spinlock_t *) * sctk_ptp_tasks_count);
  assume(sctk_ptp_tasks_lock);

  int i;
  for( i = 0 ; i < sctk_ptp_tasks_count; i++)
  {
    sctk_ptp_task_list[i] = NULL;
    sctk_ptp_tasks_lock[i] = 0;
  }

  sctk_ptp_tasks_init_done = 1;
  mpc_common_spinlock_unlock(&sctk_ptp_tasks_init_lock);
}

/*
 * Function which loop on t'he ptp_task_list' in order to copy
 * pending messages which have been already matched.
 * Only called if the task engine is enabled
 */
static inline int _sctk_ptp_tasks_perform(int key, int depth) {
  sctk_message_to_copy_t *tmp;
  int nb_messages_copied = 0; /* Number of messages processed */

  int target_list = key % sctk_ptp_tasks_count;

  /* Each element of this list has already been matched */
  while (sctk_ptp_task_list[target_list] != NULL)
  {
    tmp = NULL;

    if (mpc_common_spinlock_trylock(&(sctk_ptp_tasks_lock[target_list])) == 0) {
      tmp = sctk_ptp_task_list[target_list];

      if (tmp != NULL) {
        /* Message found, we remove it from the list */
        DL_DELETE(sctk_ptp_task_list[target_list], tmp);
      }

      mpc_common_spinlock_unlock(&(sctk_ptp_tasks_lock[target_list]));
    }

    if (tmp != NULL) {
      assume(tmp->msg_send->tail.message_copy);
      /* Call the copy function to copy the message from the network buffer to
       * the matching user buffer */
      tmp->msg_send->tail.message_copy(tmp);
      nb_messages_copied++;
    }
    else
    {
      if(depth)
      {
        return nb_messages_copied;
      }
    }
  }


  if( (nb_messages_copied == 0) && (depth < 3) )
  {
    _sctk_ptp_tasks_perform(key + 1, depth +1);
  }

  return nb_messages_copied;
}

static inline int sctk_ptp_tasks_perform(int key){
  return _sctk_ptp_tasks_perform(key, 0);
}

/*
 * Insert a message to copy. The message to insert has already been matched.
 */
static inline void sctk_ptp_tasks_insert(sctk_message_to_copy_t *tmp,
                                         mpc_comm_ptp_t *pair) {
  int key = pair->key.rank % PTP_MAX_TASK_LISTS;
  mpc_common_spinlock_lock(&(sctk_ptp_tasks_lock[key]));
  DL_APPEND(sctk_ptp_task_list[key], tmp);
  mpc_common_spinlock_unlock(&(sctk_ptp_tasks_lock[key]));
}

/*
 * Insert the message to copy into the pending list
 */
static inline void sctk_ptp_copy_tasks_insert(sctk_msg_list_t *ptr_recv,
                                              sctk_msg_list_t *ptr_send,
                                              mpc_comm_ptp_t *pair) {
  sctk_message_to_copy_t *tmp;
  SCTK_PROFIL_START(MPC_Copy_message);

  tmp = &(ptr_recv->msg->tail.copy_list);
  tmp->msg_send = ptr_send->msg;
  tmp->msg_recv = ptr_recv->msg;


  /* If the message is small just copy */
  if(tmp->msg_send->tail.message_type == SCTK_MESSAGE_CONTIGUOUS )
  {
    if(tmp->msg_send->tail.message.contiguous.size <= 64 )
    {
      assume(tmp->msg_send->tail.message_copy);
      tmp->msg_send->tail.message_copy(tmp);
    }
    else
    {
      sctk_ptp_tasks_insert(tmp, pair);
    }
  }
  else
  {
    sctk_ptp_tasks_insert(tmp, pair);
  }

  SCTK_PROFIL_END(MPC_Copy_message);
}

/********************************************************************/
/*copy engine                                                       */
/********************************************************************/

/*
 * Mark a message received as DONE and call the 'free' function associated
 * to the message
 */

#ifdef MPC_MPI
int mpc_MPI_use_windows();
int mpc_MPI_notify_request_counter(sctk_request_t *req);
#endif

void sctk_complete_and_free_message(sctk_thread_ptp_message_t *msg) {
  sctk_request_t *req = SCTK_MSG_REQUEST(msg);

  if (req) {
#ifdef MPC_MPI
    if(mpc_MPI_use_windows())
    {
      mpc_MPI_notify_request_counter(req);
    }
#endif

    if (req->ptr_to_pin_ctx) {
      sctk_rail_pin_ctx_release((sctk_rail_pin_ctx_t *)req->ptr_to_pin_ctx);
      sctk_free(req->ptr_to_pin_ctx);
      req->ptr_to_pin_ctx = NULL;
    }
  }

  void (*free_memory)(void *);

  free_memory = msg->tail.free_memory;
  assume(free_memory);

  if (msg->tail.internal_ptp) {
    assume(msg->tail.internal_ptp);
    OPA_decr_int(&msg->tail.internal_ptp->pending_nb);
  }

  if (msg->tail.buffer_async) {
    mpc_buffered_msg_t *buffer_async = msg->tail.buffer_async;
    buffer_async->completion_flag = SCTK_MESSAGE_DONE;
  }

  if (SCTK_MSG_COMPLETION_FLAG(msg)) {
    *(SCTK_MSG_COMPLETION_FLAG(msg)) = SCTK_MESSAGE_DONE;
  }

  free_memory(msg);
}

static inline void sctk_message_update_request(sctk_thread_ptp_message_t *recv,
                                               size_t send_size,
                                               size_t recv_size) {
  if (recv->tail.request) {
    /* Update the request with the source, message tag and message size */
    recv->tail.request->truncated = (send_size > recv_size);
    /*    fprintf(stderr,"send_size %lu recv_size %lu\n",send_size,recv_size);*/
  }
}

static inline size_t
sctk_message_deternime_size(sctk_thread_ptp_message_t *msg) {
  return SCTK_MSG_SIZE(msg);
}

void sctk_message_completion_and_free(sctk_thread_ptp_message_t *send,
                                      sctk_thread_ptp_message_t *recv) {
  /* If a recv request is available */
  if (recv->tail.request) {
    size_t send_size;
    size_t recv_size;

    /* Update the request with the source, message tag and message size */

    /* Make sure to do the communicator translation here
     * as messages are sent with their comm_world rank this is required
     * when matching ANY_SOURCE messages in order to fill accordingly
     * the MPI_Status object from the request data */
    recv->tail.request->header.source_task =
        sctk_get_rank(SCTK_MSG_COMMUNICATOR(send), SCTK_MSG_SRC_TASK(send));
    recv->tail.request->header.source = SCTK_MSG_SRC_PROCESS(send);

    recv->tail.request->header.message_tag = SCTK_MSG_TAG(send);
    recv->tail.request->header.msg_size = SCTK_MSG_SIZE(send);
    sctk_nodebug("request->header.msg_size = %d",
                 recv->tail.request->header.msg_size);
    recv->tail.request->msg = NULL;

    send_size = sctk_message_deternime_size(send);
    recv_size = sctk_message_deternime_size(recv);
    sctk_message_update_request(recv, send_size, recv_size);
  }

  /* If a send request is available */
  if (send->tail.request) {
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

inline void sctk_message_copy(sctk_message_to_copy_t *tmp) {
  sctk_thread_ptp_message_t *send;
  sctk_thread_ptp_message_t *recv;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  assume(send->tail.message_type == SCTK_MESSAGE_CONTIGUOUS);

  switch (recv->tail.message_type) {
  case SCTK_MESSAGE_CONTIGUOUS: {
    sctk_nodebug("SCTK_MESSAGE_CONTIGUOUS - SCTK_MESSAGE_CONTIGUOUS");
    size_t size;
    size = mpc_common_min(send->tail.message.contiguous.size,
                    recv->tail.message.contiguous.size);

    memcpy(recv->tail.message.contiguous.addr,
           send->tail.message.contiguous.addr, size);

    sctk_message_completion_and_free(send, recv);
    break;
  }

  case SCTK_MESSAGE_PACK: {
    sctk_nodebug("SCTK_MESSAGE_CONTIGUOUS - SCTK_MESSAGE_PACK size %d",
                 send->tail.message.contiguous.size);
    size_t i;
    ssize_t j;
    size_t size;
    size_t total = 0;
    size_t recv_size = 0;

    if (send->tail.message.contiguous.size > 0) {
      for (i = 0; i < recv->tail.message.pack.count; i++) {
        for (j = 0; j < recv->tail.message.pack.list.std[i].count; j++) {
          size = (recv->tail.message.pack.list.std[i].ends[j] -
                  recv->tail.message.pack.list.std[i].begins[j] + 1) *
                 recv->tail.message.pack.list.std[i].elem_size;
          recv_size += size;
        }
      }

      /* MPI 1.3 : The length of the received message must be less than or equal
       * to the length of the receive buffer */
      assume(send->tail.message.contiguous.size <= recv_size);
      sctk_nodebug("contiguous size : %d, PACK SIZE : %d",
                   send->tail.message.contiguous.size, recv_size);
      char skip = 0;

      for (i = 0; ((i < recv->tail.message.pack.count) && !skip); i++) {
        for (j = 0; ((j < recv->tail.message.pack.list.std[i].count) && !skip);
             j++) {
          size = (recv->tail.message.pack.list.std[i].ends[j] -
                  recv->tail.message.pack.list.std[i].begins[j] + 1) *
                 recv->tail.message.pack.list.std[i].elem_size;

          if (total + size > send->tail.message.contiguous.size) {
            skip = 1;
            size = send->tail.message.contiguous.size - total;
          }

          memcpy((recv->tail.message.pack.list.std[i].addr) +
                     recv->tail.message.pack.list.std[i].begins[j] *
                         recv->tail.message.pack.list.std[i].elem_size,
                 send->tail.message.contiguous.addr, size);
          total += size;
          send->tail.message.contiguous.addr += size;
          assume(total <= send->tail.message.contiguous.size);
        }
      }
    }

    sctk_message_completion_and_free(send, recv);
    break;
  }

  case SCTK_MESSAGE_PACK_ABSOLUTE: {
    sctk_nodebug("SCTK_MESSAGE_CONTIGUOUS - SCTK_MESSAGE_PACK_ABSOLUTE size %d",
                 send->tail.message.contiguous.size);
    size_t i;
    ssize_t j;
    size_t size;
    size_t total = 0;
    size_t recv_size = 0;

    if (send->tail.message.contiguous.size > 0) {
      for (i = 0; i < recv->tail.message.pack.count; i++) {
        for (j = 0; j < recv->tail.message.pack.list.absolute[i].count; j++) {
          size = (recv->tail.message.pack.list.absolute[i].ends[j] -
                  recv->tail.message.pack.list.absolute[i].begins[j] + 1) *
                 recv->tail.message.pack.list.absolute[i].elem_size;
          recv_size += size;
        }
      }

      /* MPI 1.3 : The length of the received message must be less than or equal
       * to the length of the receive buffer */
      assume(send->tail.message.contiguous.size <= recv_size);
      sctk_nodebug("contiguous size : %d, ABSOLUTE SIZE : %d",
                   send->tail.message.contiguous.size, recv_size);
      char skip = 0;

      for (i = 0; ((i < recv->tail.message.pack.count) && !skip); i++) {
        for (j = 0;
             ((j < recv->tail.message.pack.list.absolute[i].count) && !skip);
             j++) {
          size = (recv->tail.message.pack.list.absolute[i].ends[j] -
                  recv->tail.message.pack.list.absolute[i].begins[j] + 1) *
                 recv->tail.message.pack.list.absolute[i].elem_size;

          if (total + size > send->tail.message.contiguous.size) {
            skip = 1;
            size = send->tail.message.contiguous.size - total;
          }

          memcpy((recv->tail.message.pack.list.absolute[i].addr) +
                     recv->tail.message.pack.list.absolute[i].begins[j] *
                         recv->tail.message.pack.list.absolute[i].elem_size,
                 send->tail.message.contiguous.addr, size);
          total += size;
          send->tail.message.contiguous.addr += size;
          assume(total <= send->tail.message.contiguous.size);
        }
      }
    }

    sctk_message_completion_and_free(send, recv);
    break;
  }

  default:
    not_reachable();
  }
}

/*
 * Function without description
 */
static inline void
sctk_copy_buffer_std_std(sctk_pack_indexes_t *restrict in_begins,
                         sctk_pack_indexes_t *restrict in_ends, size_t in_sizes,
                         void *restrict in_adress, size_t in_elem_size,
                         sctk_pack_indexes_t *restrict out_begins,
                         sctk_pack_indexes_t *restrict out_ends,
                         size_t out_sizes, void *restrict out_adress,
                         size_t out_elem_size) {
  sctk_pack_indexes_t tmp_begin[1];
  sctk_pack_indexes_t tmp_end[1];

  if ((in_begins == NULL) && (out_begins == NULL)) {
    sctk_nodebug("sctk_copy_buffer_std_std no mpc_pack");
    sctk_nodebug("%s == %s", out_adress, in_adress);
    memcpy(out_adress, in_adress, in_sizes);
    sctk_nodebug("%s == %s", out_adress, in_adress);
  } else {
    unsigned long i;
    unsigned long j;
    unsigned long in_i;
    unsigned long in_j;
    sctk_nodebug("sctk_copy_buffer_std_std mpc_pack");

    if (in_begins == NULL) {
      in_begins = tmp_begin;
      in_begins[0] = 0;
      in_ends = tmp_end;
      in_ends[0] = in_sizes - 1;
      in_elem_size = 1;
      in_sizes = 1;
    }

    if (out_begins == NULL) {
      out_begins = tmp_begin;
      out_begins[0] = 0;
      out_ends = tmp_end;
      out_ends[0] = out_sizes - 1;
      out_elem_size = 1;
      out_sizes = 1;
    }

    in_i = 0;
    in_j = in_begins[in_i] * in_elem_size;

    for (i = 0; i < out_sizes; i++) {
      for (j = out_begins[i] * out_elem_size;
           j <= out_ends[i] * out_elem_size;) {
        size_t max_length;

        if (in_j > in_ends[in_i] * in_elem_size) {
          in_i++;

          if (in_i >= in_sizes) {
            return;
          }

          in_j = in_begins[in_i] * in_elem_size;
        }

        max_length =
            mpc_common_min((out_ends[i] * out_elem_size - j + out_elem_size),
                     (in_ends[in_i] * in_elem_size - in_j + in_elem_size));

        memcpy(&(((char *)out_adress)[j]), &(((char *)in_adress)[in_j]),
               max_length);
        sctk_nodebug("Copy out[%d-%d]%s == in[%d-%d]%s", j, j + max_length,
                     &(((char *)out_adress)[j]), in_j, in_j + max_length,
                     &(((char *)in_adress)[in_j]));

        j += max_length;
        in_j += max_length;
      }
    }
  }
}

static inline void sctk_copy_buffer_absolute_std(
    sctk_pack_absolute_indexes_t *restrict in_begins,
    sctk_pack_absolute_indexes_t *restrict in_ends, size_t in_sizes,
    void *restrict in_adress, size_t in_elem_size,
    sctk_pack_indexes_t *restrict out_begins,
    sctk_pack_indexes_t *restrict out_ends, size_t out_sizes,
    void *restrict out_adress, size_t out_elem_size) {
  sctk_pack_indexes_t tmp_begin[1];
  sctk_pack_indexes_t tmp_end[1];
  sctk_pack_absolute_indexes_t tmp_begin_abs[1];
  sctk_pack_absolute_indexes_t tmp_end_abs[1];

  if ((in_begins == NULL) && (out_begins == NULL)) {
    sctk_nodebug("sctk_copy_buffer_std_std no mpc_pack");
    sctk_nodebug("%s == %s", out_adress, in_adress);
    memcpy(out_adress, in_adress, in_sizes);
    sctk_nodebug("%s == %s", out_adress, in_adress);
  } else {
    unsigned long i;
    unsigned long j;
    unsigned long in_i;
    unsigned long in_j;
    sctk_nodebug("sctk_copy_buffer_std_std mpc_pack");

    if (in_begins == NULL) {
      in_begins = tmp_begin_abs;
      in_begins[0] = 0;
      in_ends = tmp_end_abs;
      in_ends[0] = in_sizes - 1;
      in_elem_size = 1;
      in_sizes = 1;
    }

    if (out_begins == NULL) {
      out_begins = tmp_begin;
      out_begins[0] = 0;
      out_ends = tmp_end;
      out_ends[0] = out_sizes - 1;
      out_elem_size = 1;
      out_sizes = 1;
    }

    in_i = 0;
    in_j = in_begins[in_i] * in_elem_size;

    for (i = 0; i < out_sizes; i++) {
      for (j = out_begins[i] * out_elem_size;
           j <= out_ends[i] * out_elem_size;) {
        size_t max_length;

        if (in_j > in_ends[in_i] * in_elem_size) {
          in_i++;

          if (in_i >= in_sizes) {
            return;
          }

          in_j = in_begins[in_i] * in_elem_size;
        }

        max_length =
            mpc_common_min((out_ends[i] * out_elem_size - j + out_elem_size),
                     (in_ends[in_i] * in_elem_size - in_j + in_elem_size));

        memcpy(&(((char *)out_adress)[j]), &(((char *)in_adress)[in_j]),
               max_length);
        sctk_nodebug("Copy out[%d-%d]%s == in[%d-%d]%s", j, j + max_length,
                     &(((char *)out_adress)[j]), in_j, in_j + max_length,
                     &(((char *)in_adress)[in_j]));

        j += max_length;
        in_j += max_length;
      }
    }
  }
}

/*
 * Function without description
 */
static inline void sctk_copy_buffer_std_absolute(
    sctk_pack_indexes_t *restrict in_begins,
    sctk_pack_indexes_t *restrict in_ends, size_t in_sizes,
    void *restrict in_adress, size_t in_elem_size,
    sctk_pack_absolute_indexes_t *restrict out_begins,
    sctk_pack_absolute_indexes_t *restrict out_ends, size_t out_sizes,
    void *restrict out_adress, size_t out_elem_size) {
  sctk_pack_indexes_t tmp_begin[1];
  sctk_pack_indexes_t tmp_end[1];
  sctk_pack_absolute_indexes_t tmp_begin_abs[1];
  sctk_pack_absolute_indexes_t tmp_end_abs[1];

  if ((in_begins == NULL) && (out_begins == NULL)) {
    sctk_nodebug("sctk_copy_buffer_absolute_absolute no mpc_pack");
    sctk_nodebug("%s == %s", out_adress, in_adress);
    memcpy(out_adress, in_adress, in_sizes);
    sctk_nodebug("%s == %s", out_adress, in_adress);
  } else {
    unsigned long i;
    unsigned long j;
    unsigned long in_i;
    unsigned long in_j;
    sctk_nodebug("sctk_copy_buffer_absolute_absolute mpc_pack");

    if (in_begins == NULL) {
      in_begins = tmp_begin;
      in_begins[0] = 0;
      in_ends = tmp_end;
      in_ends[0] = in_sizes - 1;
      in_elem_size = 1;
      in_sizes = 1;
    }

    if (out_begins == NULL) {
      out_begins = tmp_begin_abs;
      out_begins[0] = 0;
      out_ends = tmp_end_abs;
      out_ends[0] = out_sizes - 1;
      out_elem_size = 1;
      out_sizes = 1;
    }

    in_i = 0;
    in_j = in_begins[in_i] * in_elem_size;

    for (i = 0; i < out_sizes; i++) {
      for (j = out_begins[i] * out_elem_size;
           j <= out_ends[i] * out_elem_size;) {
        size_t max_length;

        if (in_j > in_ends[in_i] * in_elem_size) {
          in_i++;

          if (in_i >= in_sizes) {
            return;
          }

          in_j = in_begins[in_i] * in_elem_size;
        }

        max_length =
            mpc_common_min((out_ends[i] * out_elem_size - j + out_elem_size),
                     (in_ends[in_i] * in_elem_size - in_j + in_elem_size));

        sctk_nodebug("Copy out[%lu-%lu]%p == in[%lu-%lu]%p", j, j + max_length,
                     &(((char *)out_adress)[j]), in_j, in_j + max_length,
                     &(((char *)in_adress)[in_j]));
        memcpy(&(((char *)out_adress)[j]), &(((char *)in_adress)[in_j]),
               max_length);
        sctk_nodebug("Copy out[%d-%d]%d == in[%d-%d]%d", j, j + max_length,
                     (((char *)out_adress)[j]), in_j, in_j + max_length,
                     (((char *)in_adress)[in_j]));

        j += max_length;
        in_j += max_length;
      }
    }
  }
}

/*
 * Function without description
 */
static inline void sctk_copy_buffer_absolute_absolute(
    sctk_pack_absolute_indexes_t *restrict in_begins,
    sctk_pack_absolute_indexes_t *restrict in_ends, size_t in_sizes,
    void *restrict in_adress, size_t in_elem_size,
    sctk_pack_absolute_indexes_t *restrict out_begins,
    sctk_pack_absolute_indexes_t *restrict out_ends, size_t out_sizes,
    void *restrict out_adress, size_t out_elem_size) {
  sctk_pack_absolute_indexes_t tmp_begin[1];
  sctk_pack_absolute_indexes_t tmp_end[1];

  if ((in_begins == NULL) && (out_begins == NULL)) {
    sctk_nodebug("sctk_copy_buffer_absolute_absolute no mpc_pack");
    sctk_nodebug("%s == %s", out_adress, in_adress);
    memcpy(out_adress, in_adress, in_sizes);
    sctk_nodebug("%s == %s", out_adress, in_adress);
  } else {
    unsigned long i;
    unsigned long j;
    unsigned long in_i;
    unsigned long in_j;
    sctk_nodebug("sctk_copy_buffer_absolute_absolute mpc_pack %p", in_begins);

    /* Empty message */
    if (!in_sizes)
      return;

    if (in_begins == NULL) {
      in_begins = tmp_begin;
      in_begins[0] = 0;
      in_ends = tmp_end;
      in_ends[0] = in_sizes - 1;
      in_elem_size = 1;
      in_sizes = 1;
    }

    if (out_begins == NULL) {
      out_begins = tmp_begin;
      out_begins[0] = 0;
      out_ends = tmp_end;
      out_ends[0] = out_sizes - 1;
      out_elem_size = 1;
      out_sizes = 1;
    }

    in_i = 0;
    in_j = in_begins[in_i] * in_elem_size;

    for (i = 0; i < out_sizes; i++) {
      for (j = out_begins[i] * out_elem_size;
           j <= out_ends[i] * out_elem_size;) {
        size_t max_length;

        if (in_j > in_ends[in_i] * in_elem_size) {
          in_i++;

          if (in_i >= in_sizes) {
            return;
          }

          in_j = in_begins[in_i] * in_elem_size;
        }

        max_length =
            mpc_common_min((out_ends[i] * out_elem_size - j + out_elem_size),
                     (in_ends[in_i] * in_elem_size - in_j + in_elem_size));

        sctk_nodebug("Copy out[%lu-%lu]%p == in[%lu-%lu]%p", j, j + max_length,
                     &(((char *)out_adress)[j]), in_j, in_j + max_length,
                     &(((char *)in_adress)[in_j]));
        memcpy(&(((char *)out_adress)[j]), &(((char *)in_adress)[in_j]),
               max_length);
        sctk_nodebug("Copy out[%d-%d]%d == in[%d-%d]%d", j, j + max_length,
                     (((char *)out_adress)[j]), in_j, in_j + max_length,
                     (((char *)in_adress)[in_j]));

        j += max_length;
        in_j += max_length;
      }
    }
  }
}

/*
 * Function without description
 */
inline void sctk_message_copy_pack(sctk_message_to_copy_t *tmp) {
  sctk_thread_ptp_message_t *send;
  sctk_thread_ptp_message_t *recv;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  assume(send->tail.message_type == SCTK_MESSAGE_PACK);

  switch (recv->tail.message_type) {
  case SCTK_MESSAGE_PACK: {
    sctk_nodebug("SCTK_MESSAGE_PACK - SCTK_MESSAGE_PACK");
    size_t i;

    for (i = 0; i < send->tail.message.pack.count; i++) {
      sctk_copy_buffer_std_std(send->tail.message.pack.list.std[i].begins,
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

    sctk_message_completion_and_free(send, recv);
    break;
  }

  case SCTK_MESSAGE_PACK_ABSOLUTE: {
    sctk_nodebug("SCTK_MESSAGE_PACK - SCTK_MESSAGE_PACK_ABSOLUTE");
    size_t i;

    for (i = 0; i < send->tail.message.pack.count; i++) {
      sctk_copy_buffer_std_absolute(
          send->tail.message.pack.list.std[i].begins,
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

    sctk_message_completion_and_free(send, recv);
    break;
  }

  case SCTK_MESSAGE_CONTIGUOUS: {
    sctk_nodebug("SCTK_MESSAGE_PACK - SCTK_MESSAGE_CONTIGUOUS");
    size_t i;
    ssize_t j;
    size_t size;
    char *body;

    body = recv->tail.message.contiguous.addr;

    sctk_nodebug("COUNT %lu", send->tail.message.pack.count);

    for (i = 0; i < send->tail.message.pack.count; i++) {
      for (j = 0; j < send->tail.message.pack.list.std[i].count; j++) {
        size = (send->tail.message.pack.list.std[i].ends[j] -
                send->tail.message.pack.list.std[i].begins[j] + 1) *
               send->tail.message.pack.list.std[i].elem_size;
        memcpy(body, ((char *)(send->tail.message.pack.list.std[i].addr)) +
                         send->tail.message.pack.list.std[i].begins[j] *
                             send->tail.message.pack.list.std[i].elem_size,
               size);
        body += size;
      }
    }

    sctk_message_completion_and_free(send, recv);
    break;
  }

  default:
    not_reachable();
  }
}

/*
 * Function without description
 */
inline void sctk_message_copy_pack_absolute(sctk_message_to_copy_t *tmp) {
  sctk_thread_ptp_message_t *send;
  sctk_thread_ptp_message_t *recv;

  send = tmp->msg_send;
  recv = tmp->msg_recv;

  assume(send->tail.message_type == SCTK_MESSAGE_PACK_ABSOLUTE);

  switch (recv->tail.message_type) {
  case SCTK_MESSAGE_PACK: {
    sctk_nodebug("SCTK_MESSAGE_PACK_ABSOLUTE - SCTK_MESSAGE_PACK");
    size_t i;

    for (i = 0; i < send->tail.message.pack.count; i++) {
      sctk_copy_buffer_absolute_std(
          send->tail.message.pack.list.absolute[i].begins,
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

    sctk_message_completion_and_free(send, recv);
    break;
  }

  case SCTK_MESSAGE_PACK_ABSOLUTE: {
    sctk_nodebug(
        "SCTK_MESSAGE_PACK_ABSOLUTE - SCTK_MESSAGE_PACK_ABSOLUTE count == %d",
        send->tail.message.pack.count);
    size_t i;

    for (i = 0; i < send->tail.message.pack.count; i++) {
      sctk_copy_buffer_absolute_absolute(
          send->tail.message.pack.list.absolute[i].begins,
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

    sctk_message_completion_and_free(send, recv);
    break;
  }

  case SCTK_MESSAGE_CONTIGUOUS: {
    sctk_nodebug("SCTK_MESSAGE_PACK_ABSOLUTE - SCTK_MESSAGE_CONTIGUOUS");
    size_t i;
    ssize_t j;
    size_t size;
    size_t send_size = 0;

    char *body;

    body = recv->tail.message.contiguous.addr;

    sctk_nodebug("COUNT %lu", send->tail.message.pack.count);

    for (i = 0; i < send->tail.message.pack.count; i++) {
      for (j = 0; j < send->tail.message.pack.list.absolute[i].count; j++) {
        size = (send->tail.message.pack.list.absolute[i].ends[j] -
                send->tail.message.pack.list.absolute[i].begins[j] + 1) *
               send->tail.message.pack.list.absolute[i].elem_size;
        send_size += size;
      }
    }

    sctk_nodebug("msg_size = %d, send_size = %d, recv_size = %d",
                 SCTK_MSG_SIZE(send), send_size,
                 recv->tail.message.contiguous.size);
    /* MPI 1.3 : The length of the received message must be less than or equal
     * to the length of the receive buffer */
    assume(send_size <= recv->tail.message.contiguous.size);

    for (i = 0; (i < send->tail.message.pack.count); i++) {
      for (j = 0; (j < send->tail.message.pack.list.absolute[i].count); j++) {
        size = (send->tail.message.pack.list.absolute[i].ends[j] -
                send->tail.message.pack.list.absolute[i].begins[j] + 1) *
               send->tail.message.pack.list.absolute[i].elem_size;
        memcpy(body, ((char *)(send->tail.message.pack.list.absolute[i].addr)) +
                         send->tail.message.pack.list.absolute[i].begins[j] *
                             send->tail.message.pack.list.absolute[i].elem_size,
               size);
        body += size;
      }
    }

    sctk_message_completion_and_free(send, recv);
    break;
  }

  default:
    not_reachable();
  }
}

/****************
 * BUFFERED PTP *
 ****************/

/* For message creation: a set of buffered ptp_message entries is allocated
 * during init */
#define BUFFERED_PTP_MESSAGE_NUMBER 64
__thread sctk_thread_ptp_message_t *buffered_ptp_message = NULL;

static void __mpc_comm_buffered_ptp_init( void )
{
	/* Initialize the buffered_ptp_message list for the VP */
	if ( buffered_ptp_message == NULL )
	{
		sctk_thread_ptp_message_t *tmp_message;
		tmp_message = sctk_malloc( sizeof( sctk_thread_ptp_message_t ) *
									BUFFERED_PTP_MESSAGE_NUMBER );

		assume( tmp_message );

		int j;
		/* Loop on all buffers and create a list */
		for ( j = 0; j < BUFFERED_PTP_MESSAGE_NUMBER; ++j )
		{
			sctk_thread_ptp_message_t *entry = &tmp_message[j];
			entry->from_buffered = 1;
			/* Add it to the list */
			LL_PREPEND( buffered_ptp_message, entry );
		}
	}
}

/*
 * Free the header. If the header is a from the buffered list, re-add it to
 * the list. Else, free the header.
 */
void __mpc_comm_free_header( void *tmp )
{
	sctk_thread_ptp_message_t *header = (sctk_thread_ptp_message_t *)tmp;
	sctk_nodebug( "Free buffer %p buffered?%d", header, header->from_buffered );

	/* Header is from the buffered list */
	if ( header->from_buffered )
	{
		LL_PREPEND( buffered_ptp_message, header );
	}
	else
	{
		sctk_free( tmp );
	}
}

/*
 * Allocate a free header. Take it from the buffered list is an entry is free.
 * Else,
 * allocate a new header.
 */
static inline sctk_thread_ptp_message_t *__mpc_comm_alloc_header()
{
	sctk_thread_ptp_message_t *tmp = NULL;

	if ( buffered_ptp_message != NULL )
	{
		tmp = buffered_ptp_message;
		assume( tmp->from_buffered );
		LL_DELETE( buffered_ptp_message, buffered_ptp_message );
	} else {
		tmp = sctk_malloc( sizeof( sctk_thread_ptp_message_t ) );
		/* Header must be freed after use */
		tmp->from_buffered = 0;
	}

	return tmp;
}

/********************************************************************/
/*INIT                                                              */
/********************************************************************/

/*
 * Init data structures used for task i. Called only once for each task
 */
void mpc_mp_comm_init_per_task( int rank )
{
	mpc_comm_ptp_t *tmp;

	int comm_id;

	sctk_ptp_tasks_init();

	__mpc_comm_buffered_ptp_init();

	for ( comm_id = 0; comm_id < SCTK_PARALLEL_COMM_QUEUES_NUMBER; comm_id++ )
	{
		tmp = sctk_malloc( sizeof( mpc_comm_ptp_t ) );
		memset( tmp, 0, sizeof( mpc_comm_ptp_t ) );

		_mpc_comm_dest_key_init( &tmp->key, comm_id, rank );

		/* Initialize reordering for the list */
		sctk_reorder_list_init( &tmp->reorder );

		/* Initialize the internal ptp lists */
		__mpc_comm_ptp_message_list_init( &( tmp->lists ) );

		/* And insert them */
		__mpc_comm_ptp_array_insert( tmp );
	}
}

/********************************************************************/
/*Message creation                                                  */
/********************************************************************/
void __mpc_comm_ptp_message_pack_free(void *);

void mpc_mp_comm_ptp_message_header_clear( sctk_thread_ptp_message_t *tmp,
					   sctk_message_type_t msg_type, void ( *free_memory )( void * ),
					   void ( *message_copy )( sctk_message_to_copy_t * ) )
{
	memset( tmp, 0, sizeof( sctk_thread_ptp_message_t ) );
	/*Init message struct*/
	tmp->tail.message_type = msg_type;
	tmp->tail.internal_ptp = NULL;
	tmp->tail.request = NULL;

	switch ( tmp->tail.message_type )
	{
		case SCTK_MESSAGE_PACK:
			_mpc_comm_ptp_message_set_copy_and_free( tmp, __mpc_comm_ptp_message_pack_free, sctk_message_copy_pack );
			break;
		case SCTK_MESSAGE_PACK_ABSOLUTE:
			_mpc_comm_ptp_message_set_copy_and_free( tmp, __mpc_comm_ptp_message_pack_free, sctk_message_copy_pack_absolute );
			break;
		default:
			_mpc_comm_ptp_message_set_copy_and_free( tmp, free_memory, message_copy );
			break;
	}
}

sctk_thread_ptp_message_t *mpc_mp_comm_ptp_message_header_create( sctk_message_type_t msg_type )
{
	sctk_thread_ptp_message_t *tmp;

	tmp = __mpc_comm_alloc_header();
	/* Store if the buffer has been buffered */
	const char from_buffered = tmp->from_buffered;
	/* The copy and free functions will be set after */
	mpc_mp_comm_ptp_message_header_clear( tmp, msg_type, __mpc_comm_free_header, sctk_message_copy );
	/* Restore it */
	tmp->from_buffered = from_buffered;

	return tmp;
}

static inline void __mpc_comm_ptp_message_set_contiguous_addr( sctk_thread_ptp_message_t *restrict msg,
															   void *restrict addr, const size_t size )
{
	msg->tail.message.contiguous.size = size;
	msg->tail.message.contiguous.addr = addr;
}

void mpc_mp_comm_ptp_message_set_contiguous_addr( sctk_thread_ptp_message_t *restrict msg,
												  void *restrict addr, const size_t size )
{
	assert( msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS );
	__mpc_comm_ptp_message_set_contiguous_addr( msg, addr, size );
}

static inline void __mpc_comm_fill_request( sctk_request_t *request,
											int completion,
											sctk_thread_ptp_message_t *msg,
											int request_type )
{
	assume( msg != NULL );

	__mpc_comm_request_init( request, SCTK_MSG_COMMUNICATOR( msg ), request_type );

	request->msg = msg;
	request->header.source = SCTK_MSG_SRC_PROCESS( msg );
	request->header.destination = SCTK_MSG_DEST_PROCESS( msg );
	request->header.destination_task = SCTK_MSG_DEST_TASK( msg );
	request->header.source_task = SCTK_MSG_SRC_TASK( msg );
	request->header.message_tag = SCTK_MSG_TAG( msg );
	request->header.msg_size = SCTK_MSG_SIZE( msg );
	request->is_null = 0;
	request->completion_flag = completion;
	request->status_error = SCTK_SUCCESS;
	request->ptr_to_pin_ctx = NULL;
}

void mpc_mp_comm_ptp_message_header_init( sctk_thread_ptp_message_t *msg,
										  const int message_tag,
										  const sctk_communicator_t communicator,
										  const int source, const int destination,
										  sctk_request_t *request, const size_t count,
										  sctk_message_class_t message_class,
										  sctk_datatype_t datatype,
										  sctk_request_type_t request_type )
{

	msg->tail.request = request;

	/* Save message class */
	SCTK_MSG_SPECIFIC_CLASS_SET( msg, message_class );

	/* PROCESS SPECIFIC MESSAGES */
	if ( sctk_message_class_is_process_specific( message_class ) )
	{
		/* Fill in Source and Dest Process Informations (NO conversion needed)
     */
		SCTK_MSG_SRC_PROCESS_SET( msg, source );
		SCTK_MSG_DEST_PROCESS_SET( msg, destination );

		/* Message does not come from a task source */
		SCTK_MSG_SRC_TASK_SET( msg, mpc_common_get_task_rank() );

		/* In all cases such message goes to a process */
		SCTK_MSG_DEST_TASK_SET( msg, -1 );
	}
	else
	{
		int source_task = -1;
		int dest_task = -1;
		/* Fill in Source and Dest Process Informations (convert from task) */

		if ( (communicator != SCTK_COMM_WORLD) && sctk_is_inter_comm( communicator ) )
		{
			if ( request_type == REQUEST_RECV )
			{
				/* If this is a RECV make sure the translation is done on the source according to remote */
				source_task = sctk_get_remote_comm_world_rank( communicator, source );
				dest_task = sctk_get_comm_world_rank( communicator, destination );
			} else if ( request_type == REQUEST_SEND )
			{
				/* If this is a SEND make sure the translation is done on the dest according to remote */
				source_task = sctk_get_comm_world_rank( communicator, source );
				dest_task = sctk_get_remote_comm_world_rank( communicator, destination );
			}
		}
		else
		{
			/* If we are not using an inter-comm just translate to COMM_WORLD */
			if ( source != MPC_ANY_SOURCE )
			{
				source_task = sctk_get_comm_world_rank( communicator, source );
			}
			else
			{
				source_task = SCTK_ANY_SOURCE;
			}

			dest_task = sctk_get_comm_world_rank( communicator, destination );
		}

		SCTK_MSG_SRC_TASK_SET( msg, source_task );

		if ( source_task != SCTK_ANY_SOURCE )
		{
			SCTK_MSG_SRC_PROCESS_SET( msg, sctk_get_process_rank_from_task_rank( SCTK_MSG_SRC_TASK( msg ) ) );
		}
		else
		{
			SCTK_MSG_SRC_PROCESS_SET( msg, SCTK_ANY_SOURCE );
		}

		SCTK_MSG_DEST_TASK_SET( msg, dest_task );

		SCTK_MSG_DEST_PROCESS_SET( msg, sctk_get_process_rank_from_task_rank( SCTK_MSG_DEST_TASK( msg ) ) );

#if 0
		/* If source matters */
		if ( source != SCTK_ANY_SOURCE )
		{
			/* If the communicator used is the COMM_SELF */
			if ( communicator == SCTK_COMM_SELF )
			{
				/* The world destination is actually ourself :) */
				int world_src = mpc_common_get_task_rank();
				SCTK_MSG_SRC_TASK_SET(
					msg, sctk_get_comm_world_rank( communicator, world_src ) );
			}
			else
			{
				/* Get value from previous resolution according to intercomm */
				SCTK_MSG_SRC_TASK_SET( msg, source_task );
			}
		}
		else
		{
			/* Otherwise source does not matter */
			SCTK_MSG_SRC_TASK_SET( msg, SCTK_ANY_SOURCE );
		}

		/* Fill in dest */

		/* If the communicator used is the COMM_SELF */
		if ( communicator == SCTK_COMM_SELF )
		{
			/* The world destination is actually ourself :) */
			int world_dest = mpc_common_get_task_rank();
			SCTK_MSG_DEST_TASK_SET(
				msg, sctk_get_comm_world_rank( communicator, world_dest ) );
		}
		else
		{
			/* Get value from previous resolution according to intercomm */
			SCTK_MSG_DEST_TASK_SET( msg, dest_task );
		}
#endif
	}

	/* Fill in Message meta-data */
	SCTK_MSG_COMMUNICATOR_SET( msg, communicator );
	SCTK_MSG_DATATYPE_SET(msg, datatype);
	SCTK_MSG_TAG_SET( msg, message_tag );
	SCTK_MSG_SIZE_SET( msg, count );
	SCTK_MSG_USE_MESSAGE_NUMBERING_SET( msg, 1 );
	SCTK_MSG_MATCH_SET( msg, sctk_m_probe_matching_get() );

	/* A message can be sent with a NULL request (see the MPI standard) */
	if ( request )
	{
		__mpc_comm_fill_request( request, SCTK_MESSAGE_PENDING, msg, request_type );
		SCTK_MSG_COMPLETION_FLAG_SET( msg, &( request->completion_flag ) );
	}

}

/********************************************************************/
/*Message pack creation                                             */
/********************************************************************/

#define SCTK_PACK_REALLOC_STEP 10

void __mpc_comm_ptp_message_pack_free( void *tmp )
{
	sctk_thread_ptp_message_t *msg;

	msg = tmp;

	if ( msg->tail.message_type == SCTK_MESSAGE_PACK_ABSOLUTE )
	{
		sctk_free( msg->tail.message.pack.list.absolute );
	}
	else
	{
		sctk_free( msg->tail.message.pack.list.std );
	}

	__mpc_comm_free_header( tmp );
}

void mpc_mp_comm_ptp_message_add_pack( sctk_thread_ptp_message_t *msg, void *adr,
									   const sctk_count_t nb_items,
									   const size_t elem_size,
									   sctk_pack_indexes_t *begins,
									   sctk_pack_indexes_t *ends )
{
	int step;

	if ( msg->tail.message_type == SCTK_MESSAGE_PACK_UNDEFINED )
	{
		msg->tail.message_type = SCTK_MESSAGE_PACK;
		_mpc_comm_ptp_message_set_copy_and_free( msg, __mpc_comm_ptp_message_pack_free, sctk_message_copy_pack );
	}

	assume( msg->tail.message_type == SCTK_MESSAGE_PACK );

	if ( msg->tail.message.pack.count >= msg->tail.message.pack.max_count )
	{
		msg->tail.message.pack.max_count += SCTK_PACK_REALLOC_STEP;
		msg->tail.message.pack.list.std =
			sctk_realloc( msg->tail.message.pack.list.std,
						  msg->tail.message.pack.max_count *
							  sizeof( sctk_message_pack_std_list_t ) );
	}

	step = msg->tail.message.pack.count;

	msg->tail.message.pack.list.std[step].count = nb_items;
	msg->tail.message.pack.list.std[step].begins = begins;
	msg->tail.message.pack.list.std[step].ends = ends;
	msg->tail.message.pack.list.std[step].addr = adr;
	msg->tail.message.pack.list.std[step].elem_size = elem_size;

	msg->tail.message.pack.count++;
}
void mpc_mp_comm_ptp_message_add_pack_absolute( sctk_thread_ptp_message_t *msg,
												void *adr, const sctk_count_t nb_items,
												const size_t elem_size,
												sctk_pack_absolute_indexes_t *begins,
												sctk_pack_absolute_indexes_t *ends )
{
	int step;

	if ( msg->tail.message_type == SCTK_MESSAGE_PACK_UNDEFINED )
	{
		msg->tail.message_type = SCTK_MESSAGE_PACK_ABSOLUTE;
		_mpc_comm_ptp_message_set_copy_and_free( msg, __mpc_comm_ptp_message_pack_free, sctk_message_copy_pack_absolute );
	}

	assume( msg->tail.message_type == SCTK_MESSAGE_PACK_ABSOLUTE );

	if ( msg->tail.message.pack.count >= msg->tail.message.pack.max_count )
	{
		msg->tail.message.pack.max_count += SCTK_PACK_REALLOC_STEP;
		msg->tail.message.pack.list.absolute =
			sctk_realloc( msg->tail.message.pack.list.absolute,
						  msg->tail.message.pack.max_count *
							  sizeof( sctk_message_pack_absolute_list_t ) );
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

static __thread OPA_int_t m_probe_id;
static __thread OPA_int_t m_probe_id_task;

void sctk_m_probe_matching_init()
{
	OPA_store_int( &m_probe_id, 0 );
	OPA_store_int( &m_probe_id_task, -1 );
}

void sctk_m_probe_matching_set( int value )
{
	while ( OPA_cas_int( &m_probe_id, 0, value ) != 0 )
	{
		sctk_nodebug( "CAS %d", OPA_load_int( &m_probe_id ) );
		sctk_thread_yield();
	}

	int task_id;
	int thread_id;
	sctk_get_thread_info( &task_id, &thread_id );

	sctk_nodebug( "THREAD ID %d", thread_id );

	OPA_store_int( &m_probe_id_task, thread_id + 1 );
}

void sctk_m_probe_matching_reset()
{
	OPA_store_int( &m_probe_id, 0 );
	OPA_store_int( &m_probe_id_task, -1 );
}

int sctk_m_probe_matching_get()
{
	int task_id;
	int thread_id;
	sctk_get_thread_info( &task_id, &thread_id );

	if ( OPA_load_int( &m_probe_id_task ) != ( thread_id + 1 ) )
	{
		return -1;
	}

	return OPA_load_int( &m_probe_id );
}

__thread volatile int already_processsing_a_control_message = 0;

static inline sctk_msg_list_t * __mpc_comm_pending_msg_list_search_matching( mpc_comm_ptp_list_pending_t *pending_list,
																	        sctk_thread_message_header_t *header )
{
	sctk_msg_list_t *ptr_found;
	sctk_msg_list_t *tmp;

	/* Loop on all  pending messages */
	DL_FOREACH_SAFE( pending_list->list, ptr_found, tmp )
	{
		sctk_thread_message_header_t *header_found;
		sctk_assert( ptr_found->msg != NULL );
		header_found = &( ptr_found->msg->body.header );

		/* Control Message Handling */
		if ( header_found->message_type.type == SCTK_CONTROL_MESSAGE_TASK )
		{
			if ( !already_processsing_a_control_message )
			{
				DL_DELETE( pending_list->list, ptr_found );
				/* Here we have a pending control message in the list we must take it out in order to avoid deadlocks */
				already_processsing_a_control_message = 1;
				sctk_control_messages_perform( ptr_found->msg, 0 );
				already_processsing_a_control_message = 0;
			}
		}

		/* Match the communicator, the tag, the source and the specific message tag */

		if ( /* Match Communicator */
			 ( header->communicator == header_found->communicator ) &&
			 /* Match message type */
			 ( header->message_type.type == header_found->message_type.type ) &&
			 /* Match source Process */
			 ( ( header->source == header_found->source ) || ( header->source == SCTK_ANY_SOURCE ) ) &&
			 /* Match source task appart for process specific messages which are not matched at task level */
			 ( ( header->source_task == header_found->source_task ) ||
			   ( header->source_task == SCTK_ANY_SOURCE ) ||
			   sctk_message_class_is_process_specific( header->message_type.type ) ) &&
			 /* Match Message Tag while making sure that tags less than 0 are ignored (used for intercomm) */
			 ( ( header->message_tag == header_found->message_tag ) ||
			   ( ( header->message_tag == SCTK_ANY_TAG ) && ( header_found->message_tag >= 0 ) ) ) )
		{
			/* Message found. We delete it  */
			DL_DELETE( pending_list->list, ptr_found );

#ifdef MPC_MPI
			/* update the status with ERR_TYPE if datatypes don't match*/
			if ( header->datatype != header_found->datatype )
			{
				if ( /* See page 33 of 3.0 PACKED and BYTE are exceptions */
					 header->datatype != MPC_PACKED &&
					 header_found->datatype != MPC_PACKED &&
					 header->datatype != MPC_BYTE && header_found->datatype != MPC_BYTE &&
					 header->msg_size > 0 && header_found->msg_size > 0 )
				{
					if ( sctk_datatype_is_common( header->datatype ) &&
						 sctk_datatype_is_common( header_found->datatype ) )
					{

						if ( ptr_found->msg->tail.request == NULL )
						{
							sctk_request_t req;
							ptr_found->msg->tail.request = &req;
						}

						ptr_found->msg->tail.request->status_error = MPC_ERR_TYPE;
					}
				}
			}
#endif

			/* We return the pointer to the request */
			return ptr_found;
		}

		/* Check for canceled send messages*/
		if ( header_found->message_type.type == SCTK_CANCELLED_SEND )
		{
			/* Message found. We delete it  */
			DL_DELETE( pending_list->list, ptr_found );
		}
	}

	/* No message found */
	return NULL;
}

/*
 * Probe for a matching message
 */
static inline int __mpc_comm_ptp_probe( mpc_comm_ptp_t *pair,
										sctk_thread_message_header_t *header )
{

	sctk_msg_list_t *ptr_send;
	sctk_msg_list_t *tmp;

	__mpc_comm_ptp_message_list_merge_pending( &( pair->lists ) );

	DL_FOREACH_SAFE( pair->lists.pending_send.list, ptr_send, tmp )
	{
		sctk_thread_message_header_t *header_send;
		sctk_assert( ptr_send->msg != NULL );
		header_send = &( ptr_send->msg->body.header );

		int send_message_matching_id =
			OPA_load_int( &header_send->matching_id );

		if ( /* Ignore Process Specific */
			 ( sctk_message_class_is_process_specific( header->message_type.type ) == 0 ) &&
			 /* Has either no or the same matching ID */
			 ( ( send_message_matching_id == -1 ) ||
			   ( send_message_matching_id == OPA_load_int( &header->matching_id ) ) ) &&
			 /* Match Communicator */
			 ( ( header->communicator == header_send->communicator ) ||
			   ( header->communicator == SCTK_ANY_COMM ) ) &&
			 /* Match message-type */
			 ( header->message_type.type == header_send->message_type.type ) &&
			 /* Match source task (note that we ignore source process
			    here as probe only come from the MPI layer == Only tasks */
			 ( ( header->source_task == header_send->source_task ) ||
			   ( header->source_task == SCTK_ANY_SOURCE ) ) &&
			 /* Match tag while making sure that tags less than 0 are
			    ignored (used for intercomm) */
			 ( ( header->message_tag == header_send->message_tag ) ||
			   ( ( header->message_tag == SCTK_ANY_TAG ) &&
				 ( header_send->message_tag >= 0 ) ) ) )
		{
			int matching_token = sctk_m_probe_matching_get();

			if ( matching_token != 0 )
			{
				OPA_store_int( &header_send->matching_id, matching_token );
			}

			memcpy( header, &( ptr_send->msg->body.header ),
					sizeof( sctk_thread_message_header_t ) );
			return 1;
		}
	}

	return 0;
}

static inline int __mpc_comm_pending_msg_list_search_matching_from_recv( mpc_comm_ptp_t *pair, sctk_thread_ptp_message_t *msg )
{

	sctk_assert( msg != NULL );

	sctk_msg_list_t *ptr_recv = &msg->tail.distant_list;
	sctk_msg_list_t *ptr_send = __mpc_comm_pending_msg_list_search_matching( &pair->lists.pending_send, &( msg->body.header ) );

	if ( SCTK_MSG_SPECIFIC_CLASS( msg ) == SCTK_CANCELLED_RECV )
	{
		DL_DELETE( pair->lists.pending_recv.list, ptr_recv );
		assume( ptr_send == NULL );
	}

	/* We found a send request which corresponds to the recv request 'ptr_recv' */
	if ( ptr_send != NULL )
	{

		/* Recopy error if present */
		if ( ptr_send->msg->tail.request != NULL )
		{
			if ( ptr_send->msg->tail.request->status_error != SCTK_SUCCESS )
			{
				ptr_recv->msg->tail.request->status_error = ptr_send->msg->tail.request->status_error;
			}
		}

		/* Recv has matched a send, remove from list */
		DL_DELETE( pair->lists.pending_recv.list, ptr_recv );

		/* If the remote source is on a another node, we call the
		   notify matching hook in the inter-process layer. We do it
		   before copying the message to the receive buffer */
		if ( msg->tail.remote_source )
		{
			sctk_network_notify_matching_message( msg );
		}

		/*Insert the matching message to the list of messages that needs to be copied.*/
		sctk_ptp_copy_tasks_insert( ptr_recv, ptr_send, pair );
		return 1;
	}

	return 0;
}

/*
 * Function with loop on the recv 'pending' messages and try to match all
 * messages in this
 * list with the send 'pending' messages. This function must be called protected
 * by locks.
 * The function returns the number of messages copied (implies that this
 * messages where
 * matched).
 */
static inline int __mpc_comm_ptp_perform_msg_pair_locked( mpc_comm_ptp_t *pair )
{
	sctk_msg_list_t *ptr_recv;
	sctk_msg_list_t *tmp;
	int nb_messages_copied = 0;

	__mpc_comm_ptp_message_list_merge_pending( &( pair->lists ) );

	DL_FOREACH_SAFE( pair->lists.pending_recv.list, ptr_recv, tmp )
	{
		nb_messages_copied = __mpc_comm_pending_msg_list_search_matching_from_recv(pair, ptr_recv->msg );
	}

	return nb_messages_copied;
}

static inline int __mpc_comm_ptp_perform_msg_pair( mpc_comm_ptp_t *pair )
{

	if ( ( ( pair->lists.pending_send.list != NULL ) || ( pair->lists.incomming_send.list != NULL )) &&
		 ( ( pair->lists.pending_recv.list != NULL ) || ( pair->lists.incomming_recv.list != NULL )) )
	{

		__mpc_comm_ptp_message_list_lock_pending( &( pair->lists ) );
		__mpc_comm_ptp_perform_msg_pair_locked( pair );
		pair->lists.changed = 0;
		__mpc_comm_ptp_message_list_unlock_pending( &( pair->lists ) );

	}

	/* Call the task engine */
	return sctk_ptp_tasks_perform( pair->key.rank );
}

/*
 * Try to match all received messages with send requests from a internal ptp
 * list
 *
 * Returns:
 *  -1 Lock already taken
 *  >=0 Number of messages matched
 */
static inline int __mpc_comm_ptp_perform_msg_pair_trylock( mpc_comm_ptp_t *pair )
{
	int ret = -1;

	if ( !pair )
		return ret;

	SCTK_PROFIL_START( MPC_Test_message_pair_try );

	/* If the lock has not been taken, we continue */
	if ( pair->lists.pending_lock == 0 )
	{
		return __mpc_comm_ptp_perform_msg_pair( pair );
	}

	SCTK_PROFIL_END( MPC_Test_message_pair_try );
	return ret;
}

void mpc_mp_comm_ptp_msg_wait_init( struct sctk_perform_messages_s *wait,
									sctk_request_t *request, int blocking )
{
	wait->request = request;
	/* If we are in a MPI_Wait function or similar */
	wait->blocking = blocking;

	mpc_comm_ptp_t **recv_ptp = &wait->recv_ptp;
	mpc_comm_ptp_t **send_ptp = &wait->send_ptp;

	/* Searching for the pending list corresponding to the dest task */
	*recv_ptp = __mpc_comm_ptp_array_get( request->header.communicator, request->header.destination_task);
	*send_ptp = __mpc_comm_ptp_array_get( request->header.communicator, request->header.source_task );


	if (request->request_type == REQUEST_SEND) {
		wait->polling_task_id = request->header.source_task;
	} else if (request->request_type == REQUEST_RECV) {
		wait->polling_task_id = request->header.destination_task;
	}
	if (request->request_type == REQUEST_GENERALIZED) {
		wait->polling_task_id = request->header.source_task;
	}

	/* Compute the rank of the remote process */
	if ( request->header.source_task != SCTK_ANY_SOURCE &&
		 request->header.communicator != SCTK_COMM_NULL &&
		 request->header.source_task != SCTK_PROC_NULL )
	{
		/* Convert task rank to process rank */
		wait->source_task_id = sctk_get_rank( request->header.communicator,
										      request->header.source_task );
		wait->remote_process = sctk_get_process_rank_from_task_rank( wait->source_task_id );
	}
	else
	{
		wait->remote_process = -1;
		wait->source_task_id = -1;
	}
}

/*
 *  Function called when the message to receive is already completed
 */
static inline void __mpc_comm_ptp_msg_done( struct sctk_perform_messages_s *wait )
{
	const sctk_request_t *request = wait->request;
	const mpc_comm_ptp_t *recv_ptp = wait->recv_ptp;

	/* The message is marked as done.
	   However, we need to poll if it is a inter-process message
	   and if we are waiting for a SEND request. If we do not do this,
	   we might overflow the number of send buffers waiting to be released */
	if ( request->header.source_task == SCTK_ANY_SOURCE )
	{
		sctk_network_notify_any_source_message( request->header.source_task, 0 );
	}
}

static inline void __mpc_comm_ptp_msg_wait( struct sctk_perform_messages_s *wait );

static void __mpc_comm_perform_msg_wfv( void *a )
{
	struct sctk_perform_messages_s *_wait = (struct sctk_perform_messages_s *) a;

	__mpc_comm_ptp_msg_wait( _wait );

	if ( (volatile int) _wait->request->completion_flag != SCTK_MESSAGE_DONE )
	{
		sctk_network_notify_idle_message();
	}
}

void mpc_mp_comm_perform_idle( volatile int *data, int value,
							  void ( *func )( void * ), void *arg )
{

	if ( *data == value )
	{
		return;
	}

	sctk_thread_wait_for_value_and_poll( data, value, func, arg );
}

#ifdef MPC_MPI
void __MPC_poll_progress();
#endif

void mpc_mp_comm_wait( sctk_request_t *request )
{
	struct sctk_perform_messages_s _wait;

	if ( request->completion_flag == SCTK_MESSAGE_CANCELED )
	{
		return;
	}

	if ( request->request_type == REQUEST_NULL )
	{
		return;
	}

	if ( request->request_type == REQUEST_GENERALIZED )
	{
		mpc_mp_comm_perform_idle( (int *) &( request->completion_flag ),
								  SCTK_MESSAGE_DONE, __MPC_poll_progress, NULL );
	}
	else
	{
		if ( request->completion_flag == SCTK_MESSAGE_DONE )
			return;

		/* Find the PTPs lists */
		mpc_mp_comm_ptp_msg_wait_init( &_wait, request, 1 );

		/* Fastpath try a few times directly before polling */
		int trials = 0;

		do
		{

			__mpc_comm_ptp_msg_wait( &_wait );
			trials++;
		} while ( ( request->completion_flag != SCTK_MESSAGE_DONE ) && ( trials < 3 ) );

		if ( request->completion_flag != SCTK_MESSAGE_DONE )
		{
			mpc_mp_comm_perform_idle(
				(int *) &( _wait.request->completion_flag ), SCTK_MESSAGE_DONE,
				(void ( * )( void * )) __mpc_comm_perform_msg_wfv,
				&_wait );
		}
		else
		{
			__mpc_comm_ptp_msg_done( &_wait );
		}

	}

	/* Propagate finish to parent request if present */
	if ( request->pointer_to_source_request )
	{
		( (MPC_Request *) request->pointer_to_source_request )->completion_flag =
			SCTK_MESSAGE_DONE;
	}

	/* Free the shadow request bound if present (wait over the source ) */
	if ( request->pointer_to_shadow_request )
	{
		( (MPC_Request *) request->pointer_to_shadow_request )
			->pointer_to_source_request = NULL;
	}
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
 * receive (SCTK_ANY_SOURCE or not).
 *
 */

static inline void __mpc_comm_ptp_msg_wait( struct sctk_perform_messages_s *wait )
{

	const sctk_request_t *request = wait->request;
	mpc_comm_ptp_t *recv_ptp = wait->recv_ptp;
	mpc_comm_ptp_t *send_ptp = wait->send_ptp;
	const int remote_process = wait->remote_process;
	const int source_task_id = wait->source_task_id;
	const int polling_task_id = wait->polling_task_id;
	const int blocking = wait->blocking;

	if ( request->completion_flag != SCTK_MESSAGE_DONE )
	{
		/* Check the source of the request. We try to poll the
     		   source in order to retreive messages from the network */

		/* We try to poll for finding a message with a SCTK_ANY_SOURCE source */
		if ( request->header.source_task == SCTK_ANY_SOURCE )
		{
			/* We try to poll for finding a message with a SCTK_ANY_SOURCE source */
			sctk_network_notify_any_source_message( polling_task_id, blocking );
		}
		else if ( ( request->request_type == REQUEST_SEND && !recv_ptp ) ||
			  ( request->request_type == REQUEST_RECV && !send_ptp ) ||
			  ( request->header.source_task == request->header.destination_task ))
		{
			/* We poll the network only if we need it (empty queues) */
			/* If the src and the dest tasks are same, we poll the network.
			   INFO: it is usefull for the overlap benchmark from the
       			   MPC_THREAD_MULTIPLE Test Suite.
       			   An additional thread is created and waiting ob MPI_Recv with src = dest */
			sctk_network_notify_perform_message( remote_process, source_task_id, polling_task_id, blocking );
		}

		if ( ( request->request_type == REQUEST_SEND ) && send_ptp )
		{
			__mpc_comm_ptp_perform_msg_pair_trylock( send_ptp );
		}
		else if ( ( request->request_type == REQUEST_RECV ) && recv_ptp )
		{
			__mpc_comm_ptp_perform_msg_pair_trylock( recv_ptp );
		}
		else
		{
			return;
		}

	}
	else
	{
		__mpc_comm_ptp_msg_done( wait );
	}
}

/* This is the exported version */
void sctk_perform_messages(struct sctk_perform_messages_s *wait) {
  return __mpc_comm_ptp_msg_wait(wait);
}

/*
 * Wait for all message according to a communicator and a task id
 */
void mpc_mp_comm_wait_all_msgs( const int task, const sctk_communicator_t com )
{
	mpc_comm_ptp_t *pair;
	int i;

	/* Get the pending list associated to the task */
	pair = __mpc_comm_ptp_array_get( com, task );
	sctk_assert( pair );

	do
	{
		i = OPA_load_int( &pair->pending_nb );
		sctk_nodebug( "pending = %d", pair->pending_nb );

		if ( i != 0 )
		{
			/* WARNING: The inter-process module *MUST* implement
       * the 'notify_any_source_message function'. If not,
       * the polling will never occur. */
			sctk_network_notify_any_source_message( task, 0 );
			__mpc_comm_ptp_perform_msg_pair_trylock( pair );

			/* Second test to avoid a thread_yield if the message
       * has been handled */
			i = OPA_load_int( &pair->pending_nb );

			if ( i != 0 )
				sctk_thread_yield();
		}
	} while ( i != 0 );
}

void sctk_notify_idle_message() {
  /* sctk_perform_all (); */
  if ( mpc_common_get_process_count() > 1) {
    sctk_network_notify_idle_message();
  }
}

/********************************************************************/
/* Send messages                                                    */
/********************************************************************/

/*
 * Function called when sending a message. The message can be forwarded to
 * another process
 * using the 'sctk_network_send_message' function. If the message
 * matches, we add it to the corresponding pending list
 * */
void sctk_send_message_try_check(sctk_thread_ptp_message_t *msg,
                                 int perform_check) {
  if (( mpc_common_get_process_rank() < 0) ||
      ( mpc_common_get_process_count() <= SCTK_MSG_DEST_PROCESS(msg)))
  {
    sctk_error("BAD RANKS Detected FROM %d SENDING to %d/%d", mpc_common_get_process_rank(), SCTK_MSG_DEST_PROCESS(msg), mpc_common_get_process_count());
    CRASH();
  }
  sctk_debug(
      "!%d!  [ %d -> %d ] [ %d -> %d ] (CLASS %s(%d) SPE %d SIZE %d TAG %d)",
      mpc_common_get_process_rank(), SCTK_MSG_SRC_PROCESS(msg), SCTK_MSG_DEST_PROCESS(msg),
      SCTK_MSG_SRC_TASK(msg), SCTK_MSG_DEST_TASK(msg),
      sctk_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
      SCTK_MSG_SPECIFIC_CLASS(msg),
      sctk_message_class_is_process_specific(SCTK_MSG_SPECIFIC_CLASS(msg)),
      SCTK_MSG_SIZE(msg), SCTK_MSG_TAG(msg));

  /*  Message has not reached its destination */
  if (SCTK_MSG_DEST_PROCESS(msg) != mpc_common_get_process_rank()) {
    if (msg->tail.request != NULL) {
      msg->tail.request->request_type = REQUEST_SEND;
    }

    msg->tail.remote_destination = 1;
    sctk_nodebug("Need to forward the message fom %d to %d",
                 SCTK_MSG_SRC_PROCESS(msg), SCTK_MSG_DEST_PROCESS(msg));
    /* We forward the message */
    sctk_network_send_message(msg);
    return;
  }

  /* The message is a process specific message and the process rank does not
   * match
   * the current process rank */
  if (sctk_message_class_is_process_specific(SCTK_MSG_SPECIFIC_CLASS(msg))) {
    /* If we are on the right process with a control message */

    /* Message is for local process call the handler (such message are
     * never pending)
     * no need to perform a matching with a receive. However, the order is
     * guaranteed
     * by the reorder prior to calling this function */
    sctk_control_messages_incoming(msg);

    /* Done */
    return;

  } else {

    mpc_comm_ptp_t *src_pair, *dest_pair;

    assume(SCTK_MSG_COMMUNICATOR(msg) >= 0);

    if (SCTK_MSG_COMPLETION_FLAG(msg) != NULL) {
      *(SCTK_MSG_COMPLETION_FLAG(msg)) = SCTK_MESSAGE_PENDING;
    }

    if (msg->tail.request != NULL) {
      msg->tail.request->request_type = REQUEST_SEND;
      sctk_nodebug("Request %p %d", msg->tail.request,
                   msg->tail.request->request_type);
    }

    msg->tail.remote_source = 0;
    msg->tail.remote_destination = 0;

    /* We are searching for the corresponding pending list.
     * If we do not find any entry, we forward the message */

    dest_pair = __mpc_comm_ptp_array_get(SCTK_MSG_COMMUNICATOR(msg), SCTK_MSG_DEST_TASK(msg));
    src_pair = __mpc_comm_ptp_array_get(SCTK_MSG_COMMUNICATOR(msg), SCTK_MSG_SRC_TASK(msg));

    if (src_pair == NULL) {
      assume(dest_pair);
      msg->tail.internal_ptp = NULL;
      /*       __mpc_comm_ptp_message_list_add_pending(dest_pair,msg); */
    } else {
      __mpc_comm_ptp_message_list_add_pending(src_pair, msg);
    }

    if (dest_pair != NULL) {
      __mpc_comm_ptp_message_list_add_incoming_send(dest_pair, msg);

      /* If we ask for a checking, we check it */
      if (perform_check) {
        /* Try to match the message for the remote task */
        __UNUSED__ int matched_nb;
        matched_nb = __mpc_comm_ptp_perform_msg_pair_trylock(dest_pair);
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
      /*Entering low level comm and forwarding the message*/
      msg->tail.remote_destination = 1;
      sctk_network_send_message(msg);
    }
  }
}

/*
 * Function called for sending a message (i.e: MPI_Send).
 * Mostly used by the file mpc.c
 * */
void sctk_send_message(sctk_thread_ptp_message_t *msg) {

  sctk_debug(
      "SEND [ %d -> %d ] [ %d -> %d ] (CLASS %s(%d) SPE %d  size %d tag %d)",
      SCTK_MSG_SRC_PROCESS(msg), SCTK_MSG_DEST_PROCESS(msg),
      SCTK_MSG_SRC_TASK(msg), SCTK_MSG_DEST_TASK(msg),
      sctk_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
      SCTK_MSG_SPECIFIC_CLASS(msg),
      sctk_message_class_is_process_specific(SCTK_MSG_SPECIFIC_CLASS(msg)),
      SCTK_MSG_SIZE(msg), SCTK_MSG_TAG(msg));

  int need_check = 0;

  /* We only check if the message is an intra-node message */
  if (SCTK_MSG_DEST_TASK(msg) != -1) {
    if (!_sctk_is_net_message(SCTK_MSG_DEST_TASK(msg))) {
      sctk_nodebug("Request for %d", SCTK_MSG_DEST_TASK(msg));
    }
  }

  sctk_send_message_try_check(msg, need_check);
}

/********************************************************************/
/* Recv messages                                                    */
/********************************************************************/

/*
 * Function called when receiving a message.
 * */
void sctk_recv_message_try_check(sctk_thread_ptp_message_t *msg,
                                 mpc_comm_ptp_t *recv_ptp,
                                 int perform_check) {
  mpc_comm_ptp_t *send_ptp = NULL;

  if (SCTK_MSG_COMPLETION_FLAG(msg) != NULL) {
    *(SCTK_MSG_COMPLETION_FLAG(msg)) = SCTK_MESSAGE_PENDING;
  }

  if (msg->tail.request != NULL) {
    msg->tail.request->request_type = REQUEST_RECV;
  }

  msg->tail.remote_source = 0;
  msg->tail.remote_destination = 0;

  /* We are searching for the list corresponding to the
   * task which receives the message */


  if (recv_ptp == NULL) {
    recv_ptp = __mpc_comm_ptp_array_get(SCTK_MSG_COMMUNICATOR(msg), SCTK_MSG_DEST_TASK(msg));
  }

  if (!sctk_is_process_specific_message(SCTK_MSG_HEADER(msg))) {
    if (SCTK_MSG_SRC_TASK(msg) != -1) {
      send_ptp = __mpc_comm_ptp_array_get(SCTK_MSG_COMMUNICATOR(msg), SCTK_MSG_SRC_TASK(msg));
    }
  }

  /* We assume that the entry is found. If not, the message received is
   * for a task which is not registered on the node. Possible issue. */
  assume(recv_ptp);

  if (send_ptp == NULL) {
    /*Entering low level comm*/
    msg->tail.remote_source = 1;
    sctk_network_notify_recv_message(msg);
  }

  /* We add the message to the pending list */
  sctk_debug("Post recv from PROCESS [%d -> %d] TASK [%d -> %d] (%d) %p",
             SCTK_MSG_SRC_PROCESS(msg), SCTK_MSG_DEST_PROCESS(msg),
             SCTK_MSG_SRC_TASK(msg), SCTK_MSG_DEST_TASK(msg), SCTK_MSG_TAG(msg),
             msg->tail.request);
  __mpc_comm_ptp_message_list_add_pending(recv_ptp, msg);
  __mpc_comm_ptp_message_list_add_incoming_recv(recv_ptp, msg);

  /* Iw we ask for a matching, we run it */
  if (perform_check) {
    __mpc_comm_ptp_perform_msg_pair_trylock(recv_ptp);
  }
}

/*
 * Function called for receiving a message (i.e: MPI_Recv).
 * Mostly used by the file mpc.c
 * */
void sctk_recv_message(sctk_thread_ptp_message_t *msg, mpc_comm_ptp_t *tmp,
                       int need_check) {
  sctk_debug(
      "RECV [ %d -> %d ] [ %d -> %d ] (CLASS %s(%d) SPE %d  size %d tag %d)",
      SCTK_MSG_SRC_PROCESS(msg), SCTK_MSG_DEST_PROCESS(msg),
      SCTK_MSG_SRC_TASK(msg), SCTK_MSG_DEST_TASK(msg),
      sctk_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
      SCTK_MSG_SPECIFIC_CLASS(msg),
      sctk_message_class_is_process_specific(SCTK_MSG_SPECIFIC_CLASS(msg)),
      SCTK_MSG_SIZE(msg), SCTK_MSG_TAG(msg));

  sctk_recv_message_try_check(msg, tmp, need_check);
}

/*
 * Return if the task is on the node or not
 */

struct is_net_previous_answer {
  int ask;
  int answer;
};

static __thread struct is_net_previous_answer __sctk_is_net = {-1, 0};

static inline int _sctk_is_net_message(int dest) {

#ifdef SCTK_PROCESS_MODE
  if (dest != mpc_common_get_task_rank()) {
    return 1;
  }

  return 0;

#else

  if (__sctk_is_net.ask == dest) {
    return __sctk_is_net.answer;
  }

  mpc_comm_ptp_t * tmp = __mpc_comm_ptp_array_get(SCTK_COMM_WORLD, dest);

  __sctk_is_net.ask = dest;
  __sctk_is_net.answer = (tmp == NULL);

  return (tmp == NULL);
#endif
}

/* This is the exported version */ 
int sctk_is_net_message(int dest)
{
  return _sctk_is_net_message(dest);
}

/********************************************************************/
/*Probe                                                             */
/********************************************************************/

static inline void
sctk_probe_source_tag_class_func(int destination, int source, int tag,
                                 sctk_message_class_t class,
                                 const sctk_communicator_t comm, int *status,
                                 sctk_thread_message_header_t *msg) {

  mpc_comm_ptp_t *dest_ptp;
  mpc_comm_ptp_t *src_ptp = NULL;

  int world_source = source;
  int world_destination = destination;

  // sctk_error("%d === %d ( %d )", world_source, world_destination, tag );
  msg->source_task = world_source;
  msg->destination_task = world_destination;
  msg->message_tag = tag;
  msg->communicator = comm;

  msg->message_type.type = class;

  dest_ptp = __mpc_comm_ptp_array_get(comm, world_destination);

  if (source != SCTK_ANY_SOURCE) {
    src_ptp = __mpc_comm_ptp_array_get(comm, world_source);
  }

  if (source != SCTK_ANY_SOURCE && src_ptp == NULL) {
    int src_process = sctk_get_process_rank_from_task_rank(world_source);
    sctk_network_notify_perform_message(src_process, world_source,
                                        world_destination, 0);
  } else if (source == SCTK_ANY_SOURCE) {
    sctk_network_notify_any_source_message(world_destination, 0);
  }

  assume(dest_ptp != NULL);
  __mpc_comm_ptp_message_list_lock_pending(&(dest_ptp->lists));
  *status = __mpc_comm_ptp_probe(dest_ptp, msg);
  sctk_nodebug("Find source %d tag %d found ?%d", msg->source_task,
               msg->message_tag, *status);
  __mpc_comm_ptp_message_list_unlock_pending(&(dest_ptp->lists));
}
void sctk_probe_source_any_tag(int destination, int source,
                               const sctk_communicator_t comm, int *status,
                               sctk_thread_message_header_t *msg) {

  sctk_probe_source_tag_class_func(destination, source, SCTK_ANY_TAG,
                                   SCTK_P2P_MESSAGE, comm, status, msg);
}

void sctk_probe_any_source_any_tag(int destination,
                                   const sctk_communicator_t comm, int *status,
                                   sctk_thread_message_header_t *msg) {
  sctk_probe_source_tag_class_func(destination, SCTK_ANY_SOURCE, SCTK_ANY_TAG,
                                   SCTK_P2P_MESSAGE, comm, status, msg);
}

void sctk_probe_source_tag(int destination, int source,
                           const sctk_communicator_t comm, int *status,
                           sctk_thread_message_header_t *msg) {
  sctk_probe_source_tag_class_func(destination, source, msg->message_tag,
                                   SCTK_P2P_MESSAGE, comm, status, msg);
}

void sctk_probe_any_source_tag(int destination, const sctk_communicator_t comm,
                               int *status, sctk_thread_message_header_t *msg) {
  sctk_probe_source_tag_class_func(destination, SCTK_ANY_SOURCE,
                                   msg->message_tag, SCTK_P2P_MESSAGE, comm,
                                   status, msg);
}

void sctk_probe_any_source_tag_class_comm(int destination, int tag,
                                          sctk_message_class_t class,
                                          const sctk_communicator_t comm,
                                          int *status,
                                          sctk_thread_message_header_t *msg) {
  sctk_probe_source_tag_class_func(destination, SCTK_ANY_SOURCE, tag, class,
                                   comm, status, msg);
}

/********************************************************************/
/*Message Interface                                                 */
/********************************************************************/


void mpc_mp_comm_request_init( sctk_request_t *request, sctk_communicator_t comm,
							   int request_type )
{
	__mpc_comm_request_init( request, comm, request_type );
}

int mpc_mp_comm_cancel_msg( sctk_request_t *msg )
{
	int ret = SCTK_SUCCESS;

	switch ( msg->request_type )
	{
		case REQUEST_GENERALIZED:
			/* Call the cancel handler with a flag telling if the request was completed
     * which is our case is the same as not pending anymore */
			ret = ( msg->cancel_fn )( msg->extra_state,
						  ( msg->completion_flag != SCTK_MESSAGE_PENDING ) );
			break;

		case REQUEST_RECV:
			if ( msg->msg == NULL )
				return ret;

			SCTK_MSG_SPECIFIC_CLASS_SET( msg->msg, SCTK_CANCELLED_RECV );
			break;

		case REQUEST_SEND:
			if ( msg->msg == NULL )
				return ret;

			/* NOTE: cancelling a Send is deprecated */
			if ( _sctk_is_net_message( SCTK_MSG_DEST_PROCESS( msg->msg ) ) )
			{
				sctk_error( "Try to cancel a network message for %d from UNIX process %d",
					    SCTK_MSG_DEST_PROCESS( msg->msg ), mpc_common_get_process_rank() );
				not_implemented();
			}

			SCTK_MSG_SPECIFIC_CLASS_SET( msg->msg, SCTK_CANCELLED_SEND );
			break;

		default:
			not_reachable();
			break;
	}

	msg->completion_flag = SCTK_MESSAGE_CANCELED;

	return ret;
}

void mpc_mp_comm_isend_class_src( int src, int dest, void *data, size_t size,
				   int tag, sctk_communicator_t comm,
				   sctk_message_class_t class,
				   sctk_request_t *req )
{
	if ( dest == SCTK_PROC_NULL )
	{
		mpc_mp_comm_request_init( req, comm, REQUEST_SEND );
		return;
	}

	sctk_thread_ptp_message_t *msg =
		mpc_mp_comm_ptp_message_header_create( SCTK_MESSAGE_CONTIGUOUS );
	mpc_mp_comm_ptp_message_set_contiguous_addr( msg, data, size );
	mpc_mp_comm_ptp_message_header_init( msg, tag, comm, src, dest, req, size, class,
				    SCTK_DATATYPE_IGNORE, REQUEST_SEND );
	sctk_send_message( msg );
}

void mpc_mp_comm_isend_class( int dest, void *data, size_t size, int tag,
							   sctk_communicator_t comm,
							   sctk_message_class_t class, sctk_request_t *req )
{
	mpc_mp_comm_isend_class_src( sctk_get_rank(comm, mpc_common_get_task_rank()),
				      dest,
				      data,
				      size,
				      tag,
				      comm,
				      class,
				      req);
}

void mpc_mp_comm_irecv_class_dest( int src, int dest, void *buffer, size_t size,
			  	    int tag, sctk_communicator_t comm,
				    sctk_message_class_t class,
				    sctk_request_t *req )
{
	if ( src == SCTK_PROC_NULL )
	{
		mpc_mp_comm_request_init( req, comm, REQUEST_RECV );
		return;
	}

	int cw_dest = sctk_get_comm_world_rank( comm, dest );

	struct mpc_comm_ptp_s *ptp_internal = __mpc_comm_ptp_array_get( comm, cw_dest );

	sctk_thread_ptp_message_t *msg =
		mpc_mp_comm_ptp_message_header_create( SCTK_MESSAGE_CONTIGUOUS );
	mpc_mp_comm_ptp_message_set_contiguous_addr( msg, buffer, size );
	mpc_mp_comm_ptp_message_header_init( msg, tag, comm, src, dest, req, size, class,
                                    SCTK_DATATYPE_IGNORE, REQUEST_RECV );
	sctk_recv_message( msg, ptp_internal, 0 );
}

void mpc_mp_comm_irecv_class(  int src, void *buffer, size_t size, int tag,
                                sctk_communicator_t comm,
                                sctk_message_class_t class, sctk_request_t *req )
{

	mpc_mp_comm_irecv_class_dest( src, sctk_get_rank( comm, mpc_common_get_task_rank() ),
                                                           buffer, size, tag, comm, class, req );
}

void mpc_mp_comm_isend( int dest, void *data, size_t size, int tag,
                         sctk_communicator_t comm, sctk_request_t *req )
{
	mpc_mp_comm_isend_class( dest, data, size, tag, comm, SCTK_P2P_MESSAGE, req );
}

void mpc_mp_comm_irecv( int src, void *data, size_t size, int tag,
                         sctk_communicator_t comm, sctk_request_t *req )
{
	mpc_mp_comm_irecv_class( src, data, size, tag, comm, SCTK_P2P_MESSAGE, req );
}

void mpc_mp_comm_wait( sctk_request_t *request );

void mpc_mp_comm_sendrecv( void *sendbuf, size_t size, int dest, int tag, void *recvbuf,
					int src, int comm )
{
	sctk_request_t sendreq, recvreq;

	memset( &sendreq, 0, sizeof( sctk_request_t ) );
	memset( &recvreq, 0, sizeof( sctk_request_t ) );

	mpc_mp_comm_isend( dest, sendbuf, size, tag, comm, &sendreq );
	mpc_mp_comm_irecv( src, recvbuf, size, tag, comm, &recvreq );

	mpc_mp_comm_wait( &sendreq );
	mpc_mp_comm_wait( &recvreq );
}