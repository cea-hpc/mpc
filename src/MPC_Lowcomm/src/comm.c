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

#include <comm.h>

#include <mpc_arch.h>
#include <sctk_low_level_comm.h>
#include <mpc_common_helper.h>
#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <mpc_common_asm.h>
#include <sctk_checksum.h>
#include <reorder.h>
#include <sctk_control_messages.h>
#include <sctk_rail.h>
#include <mpc_launch.h>
#include <sctk_window.h>
#include <mpc_common_profiler.h>
#include <sctk_alloc.h>
#include <coll.h>
#include <mpc_common_flags.h>
#include <mpc_topology.h>

#include "communicator.h"
#include <mpc_lowcomm_datatypes.h>
#ifdef MPC_USE_INFINIBAND
#include <ibdevice.h>
#endif

#include "mpc_lowcomm_workshare.h"
#include "lowcomm_config.h"
#include "monitor.h"
#include "pset.h"

/********************************************************************/
/*Structures                                                        */
/********************************************************************/

typedef struct
{
	mpc_lowcomm_communicator_id_t comm_id;
	int      rank;
} mpc_comm_dest_key_t;

static inline void _mpc_comm_dest_key_init(mpc_comm_dest_key_t *key, mpc_lowcomm_communicator_id_t comm, int rank)
{
	key->rank = rank;
	key->comm_id = comm;
}

typedef struct mpc_comm_ptp_s
{
	mpc_comm_dest_key_t          key;

	mpc_lowcomm_ptp_message_lists_t lists;


	UT_hash_handle               hh;

	/* Reorder table */
	_mpc_lowcomm_reorder_list_t          reorder;
} mpc_comm_ptp_t;

static inline void __mpc_comm_request_init(mpc_lowcomm_request_t *request,
                                           mpc_lowcomm_communicator_t comm,
                                           int request_type)
{
	if(request != NULL)
	{
        memset(request, 0, sizeof(mpc_lowcomm_request_t));

		request->comm = comm;
		request->completion_flag         = MPC_LOWCOMM_MESSAGE_DONE;
		request->header.source           = mpc_lowcomm_monitor_local_uid_of(MPC_PROC_NULL);
		request->header.destination      = mpc_lowcomm_monitor_local_uid_of(MPC_PROC_NULL);
		request->header.source_task      = MPC_PROC_NULL;
		request->header.destination_task = MPC_PROC_NULL;
		request->header.message_tag      = MPC_ANY_TAG;
		request->header.communicator_id = _mpc_lowcomm_communicator_id(comm);

		request->request_type        = request_type;
	}
}

/********************************************************************/
/*Functions                                                         */
/********************************************************************/

#define CHECK_STATUS_ERROR(status) do{\
	if((status)->MPC_ERROR  != MPC_LOWCOMM_SUCCESS)\
	{\
		return (status)->MPC_ERROR ;\
	}\
	}\
	while(0)

static int (*mpc_lowcomm_type_is_common)(mpc_lowcomm_datatype_t datatype) = NULL;

void mpc_lowcomm_register_type_is_common( int (*type_ptr)(mpc_lowcomm_datatype_t) )
{
	assume(type_ptr != NULL);
	mpc_lowcomm_type_is_common = type_ptr;
}

int mpc_lowcomm_check_type_compat(mpc_lowcomm_datatype_t src, mpc_lowcomm_datatype_t dest)
{
	//mpc_common_debug_error("COMPAT %d %d", src, dest);

	if(mpc_lowcomm_type_is_common != NULL)
	{
		TODO("THIS WILL BE FIXED WHEN DT will be at the right place");
		if(src != dest)
		{
			if( (src != MPC_DATATYPE_IGNORE) && (dest != MPC_DATATYPE_IGNORE))
			{
				/* See page 33 of 3.0 PACKED and BYTE are exceptions */
				if((src != MPC_LOWCOMM_PACKED) && (dest != MPC_LOWCOMM_PACKED))
				{
					if((src != MPC_LOWCOMM_BYTE) && (dest != MPC_LOWCOMM_BYTE))
					{
						if((mpc_lowcomm_type_is_common)(src) &&
						   (mpc_lowcomm_type_is_common)(dest) )
						{

							return MPC_LOWCOMM_ERR_TYPE;
						}
					}
				}
			}
		}

	}

	return MPC_LOWCOMM_SUCCESS;
}


int mpc_lowcomm_commit_status_from_request(mpc_lowcomm_request_t *request,
                                           mpc_lowcomm_status_t *status)
{
	if(!request)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	if(request->request_type == REQUEST_GENERALIZED)
	{
		mpc_lowcomm_status_t static_status;

		/* You must provide a valid status to the querry function */
		if(status == MPC_LOWCOMM_STATUS_NULL)
		{
			status = &static_status;
		}

		memset(status, 0, sizeof(mpc_lowcomm_status_t) );
		/* Fill in the status info */
		(request->query_fn)(request->extra_state, status);
		/* Free the request */
		(request->free_fn)(request->extra_state);
	}
	else if(status != MPC_LOWCOMM_STATUS_NULL)
	{
		status->MPC_SOURCE = request->header.source_task;
		status->MPC_TAG    = request->header.message_tag;
		status->MPC_ERROR  = request->status_error;

		status->size = (int)request->header.msg_size;

		if(request->completion_flag == MPC_LOWCOMM_MESSAGE_CANCELED)
		{
			status->cancelled = 1;
		}
		else
		{
			status->cancelled = 0;
		}


#ifdef MPC_MPI
		if(request->truncated)
		{
			status->MPC_ERROR = MPC_LOWCOMM_ERR_TRUNCATE;
		}
#endif

	}

	return MPC_LOWCOMM_SUCCESS;
}


/*
 * Initialize the 'incoming' list.
 */

static inline void __mpc_comm_ptp_list_incoming_init(mpc_lowcomm_ptp_list_incomming_t *list)
{
	list->list = NULL;
	mpc_common_spinlock_init(&list->lock, 0);
}

/*
 * Initialize the 'pending' list
 */
static inline void __mpc_comm_ptp_list_pending_init(mpc_lowcomm_ptp_list_pending_t *list)
{
	list->list = NULL;
}

/*
 * Initialize a PTP the 'incoming' and 'pending' message lists.
 */
static inline void __mpc_comm_ptp_message_list_init(mpc_lowcomm_ptp_message_lists_t *lists)
{
	__mpc_comm_ptp_list_incoming_init(&(lists->incomming_send) );
	__mpc_comm_ptp_list_incoming_init(&(lists->incomming_recv) );
	mpc_common_spinlock_init(&lists->pending_lock, 0);
	__mpc_comm_ptp_list_pending_init(&(lists->pending_send) );
	__mpc_comm_ptp_list_pending_init(&(lists->pending_recv) );
}

/*
 * Merge 'incoming' send and receive lists to 'pending' send and receive lists
 */
static inline void __mpc_comm_ptp_message_list_merge_pending(mpc_lowcomm_ptp_message_lists_t *lists)
{
    int flush = 0;

	if(lists->incomming_send.list != NULL)
	{
		if(mpc_common_spinlock_trylock(&(lists->incomming_send.lock) ) == 0)
		{
			DL_CONCAT(lists->pending_send.list,
			          ( mpc_lowcomm_msg_list_t * )lists->incomming_send.list);
			lists->incomming_send.list = NULL;
            flush = 1;
			mpc_common_spinlock_unlock(&(lists->incomming_send.lock) );
		}
	}

	if(lists->incomming_recv.list != NULL)
	{
		if(mpc_common_spinlock_trylock(&(lists->incomming_recv.lock) ) == 0)
		{
			DL_CONCAT(lists->pending_recv.list,
			          ( mpc_lowcomm_msg_list_t * )lists->incomming_recv.list);
			lists->incomming_recv.list = NULL;
            flush = 1;
			mpc_common_spinlock_unlock(&(lists->incomming_recv.lock) );
		}
	}

    if( flush )
    {
        OPA_write_barrier();
    }
}

static inline void __mpc_comm_ptp_message_list_lock_pending(mpc_lowcomm_ptp_message_lists_t *lists)
{
	mpc_common_spinlock_lock_yield(&(lists->pending_lock) );
}

static inline int __mpc_comm_ptp_message_list_trylock_pending(mpc_lowcomm_ptp_message_lists_t *lists)
{
	return mpc_common_spinlock_trylock(&(lists->pending_lock) );
}

static inline void __mpc_comm_ptp_message_list_unlock_pending(mpc_lowcomm_ptp_message_lists_t *lists)
{
	mpc_common_spinlock_unlock(&(lists->pending_lock) );
}


/*
 * Add message into the 'incoming' recv list
 */
static inline void __mpc_comm_ptp_message_list_add_incoming_recv(mpc_comm_ptp_t *tmp,
                                                                 mpc_lowcomm_ptp_message_t *msg)
{
	assume(tmp != NULL);
	msg->tail.distant_list.msg = msg;
	mpc_common_spinlock_lock(&(tmp->lists.incomming_recv.lock) );
	DL_APPEND(tmp->lists.incomming_recv.list, &(msg->tail.distant_list) );
    mpc_common_spinlock_unlock(&(tmp->lists.incomming_recv.lock) );
}

/*
 * Add message into the 'incoming' send list
 */
static inline void __mpc_comm_ptp_message_list_add_incoming_send(mpc_comm_ptp_t *tmp,
                                                                 mpc_lowcomm_ptp_message_t *msg)
{
	msg->tail.distant_list.msg = msg;

	assume(tmp != NULL);
	mpc_common_spinlock_lock(&(tmp->lists.incomming_send.lock) );
	DL_APPEND(tmp->lists.incomming_send.list, &(msg->tail.distant_list) );
	mpc_common_spinlock_unlock(&(tmp->lists.incomming_send.lock) );
}

/************************************************************************/
/*Data structure accessors                                              */
/************************************************************************/

static mpc_comm_ptp_t ***sctk_ptp_array = NULL;
static int sctk_ptp_array_start         = 0;
static int sctk_ptp_array_end           = 0;

/*
 * Find an internal ptp according the a key (this key represents the task
 * number)
 */

static inline struct mpc_comm_ptp_s *__mpc_comm_ptp_array_get_key(mpc_comm_dest_key_t *key)
{
	int __dest__id = key->rank - sctk_ptp_array_start;

	if( (sctk_ptp_array != NULL) && (__dest__id >= 0) &&
	    (__dest__id <= sctk_ptp_array_end - sctk_ptp_array_start) )
	{
		return sctk_ptp_array[__dest__id][key->comm_id % SCTK_PARALLEL_COMM_QUEUES_NUMBER];
	}

	return NULL;
}

static inline struct mpc_comm_ptp_s *__mpc_comm_ptp_array_get(mpc_lowcomm_communicator_t comm, int rank)
{
	mpc_comm_dest_key_t key;

	_mpc_comm_dest_key_init(&key, comm->id, rank);
	return __mpc_comm_ptp_array_get_key(&key);
}

int _mpc_lowcomm_comm_get_lists(int rank, mpc_lowcomm_ptp_message_lists_t **lists, int *list_count)
{
	int i;
	int ret_cnt = 0;

	for(i = 0 ; (i < SCTK_PARALLEL_COMM_QUEUES_NUMBER)  && (i < *list_count);i++)
	{
		mpc_comm_dest_key_t key;
		key.comm_id = i;
		key.rank = rank;

		struct mpc_comm_ptp_s * ptp = __mpc_comm_ptp_array_get_key(&key);

		if(ptp)
		{
			(lists)[ret_cnt] = &ptp->lists;
			ret_cnt++;
		} 
	}

	*list_count = ret_cnt;



	if( mpc_common_get_local_process_count() <= *list_count)
	{
		return MPC_LOWCOMM_ERR_TRUNCATE;
	}

	return MPC_LOWCOMM_SUCCESS;
}


struct mpc_comm_ptp_s *_mpc_comm_ptp_array_get(mpc_lowcomm_communicator_t comm, int rank)
{
	return __mpc_comm_ptp_array_get(comm, rank);
}

_mpc_lowcomm_reorder_list_t *_mpc_comm_ptp_array_get_reorder(mpc_lowcomm_communicator_t communicator, int rank)
{
	struct mpc_comm_ptp_s *internal_ptp = __mpc_comm_ptp_array_get(communicator, rank);

	assert(internal_ptp);
	return &(internal_ptp->reorder);
}

void _mpc_comm_ptp_message_reinit_comm(mpc_lowcomm_ptp_message_t *msg)
{
	assert(msg != NULL);
	mpc_lowcomm_communicator_id_t comm_id = SCTK_MSG_COMMUNICATOR_ID(msg);

	mpc_lowcomm_communicator_t comm = mpc_lowcomm_get_communicator_from_id(comm_id);

	if(!comm)
	{
		mpc_common_debug_error("No such local comm %llu", comm_id);
		assume(comm != NULL);
	}


	SCTK_MSG_COMMUNICATOR_SET(msg, comm);
}


/*
 * Insert a new entry to the PTP table. The function checks if the entry is
 * already prensent
 * and fail in this case
 */
static inline void __mpc_comm_ptp_array_insert(mpc_comm_ptp_t *tmp)
{
	static mpc_common_spinlock_t lock = MPC_COMMON_SPINLOCK_INITIALIZER;
	static volatile int          done = 0;

	mpc_common_spinlock_lock(&lock);

	/* Only one task allocates the structures */
	if(done == 0)
	{
		sctk_ptp_array_start = _mpc_lowcomm_communicator_world_first_local_task(MPC_COMM_WORLD);
		sctk_ptp_array_end   = _mpc_lowcomm_communicator_world_last_local_task(MPC_COMM_WORLD);
		sctk_ptp_array       = sctk_malloc( (size_t)(sctk_ptp_array_end - sctk_ptp_array_start + 1) * sizeof(mpc_comm_ptp_t * *) );
		memset(sctk_ptp_array, 0,  (size_t)(sctk_ptp_array_end - sctk_ptp_array_start + 1) * sizeof(mpc_comm_ptp_t * *) );
		done = 1;
	}

	assume(tmp->key.rank >= sctk_ptp_array_start);
	assume(tmp->key.rank <= sctk_ptp_array_end);

	if(sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] == NULL)
	{
		sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] = sctk_malloc(SCTK_PARALLEL_COMM_QUEUES_NUMBER * sizeof(mpc_comm_ptp_t *) );
		memset(sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start], 0, SCTK_PARALLEL_COMM_QUEUES_NUMBER * sizeof(mpc_comm_ptp_t *) );
	}

	assume(sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start][tmp->key.comm_id % SCTK_PARALLEL_COMM_QUEUES_NUMBER] == NULL);
	sctk_ptp_array[tmp->key.rank - sctk_ptp_array_start] [tmp->key.comm_id % SCTK_PARALLEL_COMM_QUEUES_NUMBER] = tmp;
	mpc_common_spinlock_unlock(&lock);
}

/*
 * Return if the task is on the node or not
 */

struct is_net_previous_answer
{
	int ask;
	int answer;
};

static __thread struct is_net_previous_answer __sctk_is_net = { -1, 0 };

static inline int _mpc_comm_is_remote_rank(int dest)
{
#ifdef MPC_IN_PROCESS_MODE
	if(dest != mpc_common_get_task_rank() )
	{
		return 1;
	}

	return 0;
#endif

	if(__sctk_is_net.ask == dest)
	{
		return __sctk_is_net.answer;
	}

	mpc_comm_ptp_t *tmp = __mpc_comm_ptp_array_get(MPC_COMM_WORLD, dest);
	__sctk_is_net.ask    = dest;
	__sctk_is_net.answer = (tmp == NULL);
	return tmp == NULL;
}

/* This is the exported version */
int mpc_lowcomm_is_remote_rank(int dest)
{
	return _mpc_comm_is_remote_rank(dest);
}

/********************************************************************/
/*Task engine                                                       */
/********************************************************************/

/* Messages in the '__mpc_ptp_task_list' have already been
 * matched and are wainting to be copied */

static short          __mpc_comm_ptp_task_init_done = 0;
mpc_common_spinlock_t __mpc_comm_ptp_task_init_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

/* MUST BE POWER of 2 as we use masking for modulos */
#define PTP_TASKING_QUEUE_COUNT    0xF

struct __mpc_lowcomm_copy_task_s
{
	mpc_common_spinlock_t                      lock;
	mpc_lowcomm_ptp_message_content_to_copy_t *cpy;
	char                                       pad[128];
};

struct __mpc_lowcomm_copy_task_s __mpc_ptp_task_list[PTP_TASKING_QUEUE_COUNT + 1];

static inline void __mpc_comm_ptp_task_init()
{
	mpc_common_spinlock_lock(&__mpc_comm_ptp_task_init_lock);

	if(__mpc_comm_ptp_task_init_done)
	{
		mpc_common_spinlock_unlock(&__mpc_comm_ptp_task_init_lock);
		return;
	}

	int i;

	for(i = 0; i <= PTP_TASKING_QUEUE_COUNT; i++)
	{
		__mpc_ptp_task_list[i].cpy = NULL;
		mpc_common_spinlock_init(&__mpc_ptp_task_list[i].lock, 0);
	}

	__mpc_comm_ptp_task_init_done = 1;
	mpc_common_spinlock_unlock(&__mpc_comm_ptp_task_init_lock);
}

/*
 * Function which loop on t'he ptp_task_list' in order to copy
 * pending messages which have been already matched.
 * Only called if the task engine is enabled
 */
static inline int ___mpc_comm_ptp_task_perform(unsigned int key, unsigned int depth)
{
	int nb_messages_copied = 0; /* Number of messages processed */
	int target_list        = key & PTP_TASKING_QUEUE_COUNT;

	/* Each element of this list has already been matched */
	while(__mpc_ptp_task_list[target_list].cpy != NULL )
	{
		mpc_lowcomm_ptp_message_content_to_copy_t *tmp = NULL;

		if(mpc_common_spinlock_trylock(&(__mpc_ptp_task_list[target_list].lock) ) == 0)
		{
			tmp = __mpc_ptp_task_list[target_list].cpy;

			if(tmp != NULL)
			{
				/* Message found, we remove it from the list */
				DL_DELETE(__mpc_ptp_task_list[target_list].cpy, tmp);
			}

			mpc_common_spinlock_unlock(&(__mpc_ptp_task_list[target_list].lock) );
		}

		if(tmp != NULL)
		{
			assert(tmp->msg_send->tail.message_copy);

			/* Call the copy function to copy the message from the network buffer to
			 * the matching user buffer */
			tmp->msg_send->tail.message_copy(tmp);
			nb_messages_copied++;
		}
	}

	if( !nb_messages_copied && (depth < 2) )
	{
		___mpc_comm_ptp_task_perform(key + depth - 1, depth + 1);
	}


	return nb_messages_copied;
}

static inline int _mpc_comm_ptp_task_perform(unsigned int key)
{
	return ___mpc_comm_ptp_task_perform(key, 0);
}

/*
 * Insert a message to copy. The message to insert has already been matched.
 */
static inline void _mpc_comm_ptp_task_insert(mpc_lowcomm_ptp_message_content_to_copy_t *tmp,
                                             mpc_comm_ptp_t *pair)
{
	int key = pair->key.rank & PTP_TASKING_QUEUE_COUNT;

	mpc_common_spinlock_lock(&(__mpc_ptp_task_list[key].lock) );
	DL_APPEND(__mpc_ptp_task_list[key].cpy, tmp);
	mpc_common_spinlock_unlock(&(__mpc_ptp_task_list[key].lock) );
}

/*
 * Insert the message to copy into the pending list
 */
static inline int _mpc_comm_ptp_copy_task_insert(mpc_lowcomm_msg_list_t *ptr_recv,
                                                 mpc_lowcomm_msg_list_t *ptr_send,
                                                 mpc_comm_ptp_t *pair)
{
	mpc_lowcomm_ptp_message_content_to_copy_t *tmp;

	SCTK_PROFIL_START(MPC_Copy_message);
	tmp           = &(ptr_recv->msg->tail.copy_list);
	tmp->msg_send = ptr_send->msg;
	tmp->msg_recv = ptr_recv->msg;

	int new_task = 0;

	/* If the message is small just copy */
	if(tmp->msg_send->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
	{
		if(tmp->msg_send->tail.message.contiguous.size <= 128)
		{
			assert(tmp->msg_send->tail.message_copy);
			tmp->msg_send->tail.message_copy(tmp);
		}
		else
		{
			_mpc_comm_ptp_task_insert(tmp, pair);
			new_task = 1;
		}
	}
	else
	{
		_mpc_comm_ptp_task_insert(tmp, pair);
	}

	SCTK_PROFIL_END(MPC_Copy_message);

	return new_task;
}

/********************************************************************/
/*copy engine                                                       */
/********************************************************************/

/*
 * Mark a message received as DONE and call the 'free' function associated
 * to the message
 */
int ___mpi_windows_in_use = 0;

static inline int __MPC_MPI_windows_in_use()
{
	return ___mpi_windows_in_use;
}

void mpc_lowcomm_rdma_MPI_windows_in_use(void)
{
	___mpi_windows_in_use |= 1;
}

static int (*___notify_request_completion_trampoline)(mpc_lowcomm_request_t *req) = NULL;

void mpc_lowcomm_set_request_completion_trampoline(int trampoline(mpc_lowcomm_request_t *req) )
{
	___notify_request_completion_trampoline = trampoline;
}

void mpc_lowcomm_set_request_completion_callback(mpc_lowcomm_request_t *req, int trampoline(mpc_lowcomm_request_t *req) )
{
	req->request_completion_fn = trampoline;
}

int mpc_lowcomm_notify_request_completion(mpc_lowcomm_request_t *req)
{
	if(___notify_request_completion_trampoline)
	{
		return (___notify_request_completion_trampoline)(req);
	}

	return 0;
}

void mpc_lowcomm_ptp_message_complete_and_free(mpc_lowcomm_ptp_message_t *msg)
{
	mpc_lowcomm_request_t *req = SCTK_MSG_REQUEST(msg);

	mpc_common_debug("COMPLETE AND FREE Request %p", req);

	if(req)
	{
#ifdef MPC_MPI
		if(__MPC_MPI_windows_in_use() )
		{
			mpc_lowcomm_notify_request_completion(req);
		}
#endif
                //mpc_lowcomm_notify_request_completion(req);
                if (req->request_completion_fn) {
                        req->request_completion_fn(req);
                }

		if(req->ptr_to_pin_ctx)
		{
			sctk_rail_pin_ctx_release( ( sctk_rail_pin_ctx_t * )req->ptr_to_pin_ctx);
			sctk_free(req->ptr_to_pin_ctx);
			req->ptr_to_pin_ctx = NULL;
		}
	}

	void ( *free_memory )(void *);
	free_memory = msg->tail.free_memory;
	assume(free_memory);

	if(SCTK_MSG_COMPLETION_FLAG(msg) )
	{
		*(SCTK_MSG_COMPLETION_FLAG(msg) ) = MPC_LOWCOMM_MESSAGE_DONE;
	}

	( free_memory )(msg);
}

void _mpc_comm_ptp_message_commit_request(mpc_lowcomm_ptp_message_t *send,
                                          mpc_lowcomm_ptp_message_t *recv)
{
	/* If a recv request is available */
	if(recv->tail.request)
	{
		/* Update the request with the source, message tag and message size */

		/* Make sure to do the communicator translation here
		 * as messages are sent with their comm_world rank this is required
		 * when matching ANY_SOURCE messages in order to fill accordingly
		 * the MPI_Status object from the request data 
		 * therefore we translate the rank as seen from the source (for intercomm)
		 * for intracomm as there is a single group it is not important */
		if(0 <= SCTK_MSG_SRC_TASK(send))
        {
            recv->tail.request->header.source_task = mpc_lowcomm_communicator_rank_of_as(SCTK_MSG_COMMUNICATOR(send), SCTK_MSG_SRC_TASK(send),  SCTK_MSG_SRC_TASK(send), SCTK_MSG_SRC_PROCESS_UID(send) );
		    //assume(recv->tail.request->header.source_task  != MPC_PROC_NULL);
        }
        else
        {
            /* This is a ctrl msg do not attemp to resolve */
            recv->tail.request->header.source_task = SCTK_MSG_SRC_TASK(send);
        }

		recv->tail.request->header.source      = SCTK_MSG_SRC_PROCESS_UID(send);
		recv->tail.request->header.message_tag = SCTK_MSG_TAG(send);
		recv->tail.request->header.msg_size    = SCTK_MSG_SIZE(send);
		recv->tail.request->msg       = NULL;
		recv->tail.request->truncated = (SCTK_MSG_SIZE(send) > SCTK_MSG_SIZE(recv) );
	}

	/* If a send request is available */
	if(send->tail.request)
	{
		/* Recopy error if present */
		if(send->tail.request->status_error != MPC_LOWCOMM_SUCCESS)
		{
			recv->tail.request->status_error = send->tail.request->status_error;
		}

		send->tail.request->msg = NULL;
	}

#ifdef SCTK_USE_CHECKSUM
	/* Verify the checksum of the received message */
	sctk_checksum_verify(send, recv);
#endif
	/* Complete messages: mark messages as done and mark them as DONE */
	mpc_lowcomm_ptp_message_complete_and_free(send);
	mpc_lowcomm_ptp_message_complete_and_free(recv);
}

inline void mpc_lowcomm_ptp_message_copy(mpc_lowcomm_ptp_message_content_to_copy_t *tmp)
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;

	send = tmp->msg_send;
	recv = tmp->msg_recv;
	assert(send->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS);

	switch(recv->tail.message_type)
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			mpc_common_nodebug("MPC_LOWCOMM_MESSAGE_CONTIGUOUS - MPC_LOWCOMM_MESSAGE_CONTIGUOUS");
			size_t size;
			size = mpc_common_min(send->tail.message.contiguous.size,
			                      recv->tail.message.contiguous.size);
			memcpy(recv->tail.message.contiguous.addr,
			       send->tail.message.contiguous.addr, size);
			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			mpc_common_nodebug("MPC_LOWCOMM_MESSAGE_CONTIGUOUS - MPC_LOWCOMM_MESSAGE_PACK size %d",
			                   send->tail.message.contiguous.size);
			size_t  i;
			ssize_t j;
			size_t  size;
			size_t  total     = 0;
			size_t  recv_size = 0;

			if(send->tail.message.contiguous.size > 0)
			{
				for(i = 0; i < recv->tail.message.pack.count; i++)
				{
					for(j = 0; j < recv->tail.message.pack.list.std[i].count; j++)
					{
						size = (recv->tail.message.pack.list.std[i].ends[j] -
						        recv->tail.message.pack.list.std[i].begins[j] + 1) *
						       recv->tail.message.pack.list.std[i].elem_size;
						recv_size += size;
					}
				}

				/* MPI 1.3 : The length of the received message must be less than or equal
				 * to the length of the receive buffer */
				assert(send->tail.message.contiguous.size <= recv_size);
				mpc_common_nodebug("contiguous size : %d, PACK SIZE : %d",
				                   send->tail.message.contiguous.size, recv_size);
				char skip = 0;

				for(i = 0; ( (i < recv->tail.message.pack.count) && !skip); i++)
				{
					for(j = 0; ( (j < recv->tail.message.pack.list.std[i].count) && !skip);
					    j++)
					{
						size = (recv->tail.message.pack.list.std[i].ends[j] -
						        recv->tail.message.pack.list.std[i].begins[j] + 1) *
						       recv->tail.message.pack.list.std[i].elem_size;

						if(total + size > send->tail.message.contiguous.size)
						{
							skip = 1;
							size = send->tail.message.contiguous.size - total;
						}

						memcpy( (recv->tail.message.pack.list.std[i].addr) +
						        recv->tail.message.pack.list.std[i].begins[j] *
						        recv->tail.message.pack.list.std[i].elem_size,
						        send->tail.message.contiguous.addr, size);
						total += size;
						send->tail.message.contiguous.addr += size;
						assert(total <= send->tail.message.contiguous.size);
					}
				}
			}

			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			mpc_common_nodebug("MPC_LOWCOMM_MESSAGE_CONTIGUOUS - MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE size %d",
			                   send->tail.message.contiguous.size);
			size_t  i;
			ssize_t j;
			size_t  size;
			size_t  total     = 0;
			size_t  recv_size = 0;

			if(send->tail.message.contiguous.size > 0)
			{
				for(i = 0; i < recv->tail.message.pack.count; i++)
				{
					for(j = 0; j < recv->tail.message.pack.list.absolute[i].count; j++)
					{
						size = (size_t)( (recv->tail.message.pack.list.absolute[i].ends[j] -
						        recv->tail.message.pack.list.absolute[i].begins[j] + 1) *
						       (long)recv->tail.message.pack.list.absolute[i].elem_size );
						recv_size += size;
					}
				}

				/* MPI 1.3 : The length of the received message must be less than or equal
				 * to the length of the receive buffer */
				assert(send->tail.message.contiguous.size <= recv_size);
				mpc_common_nodebug("contiguous size : %d, ABSOLUTE SIZE : %d",
				                   send->tail.message.contiguous.size, recv_size);
				char skip = 0;

				for(i = 0; ( (i < recv->tail.message.pack.count) && !skip); i++)
				{
					for(j = 0;
					    ( (j < recv->tail.message.pack.list.absolute[i].count) && !skip);
					    j++)
					{
						size = (size_t)((recv->tail.message.pack.list.absolute[i].ends[j] -
						        recv->tail.message.pack.list.absolute[i].begins[j] + 1) *
						       (long)recv->tail.message.pack.list.absolute[i].elem_size);

						if(total + size > send->tail.message.contiguous.size)
						{
							skip = 1;
							size = send->tail.message.contiguous.size - total;
						}

						memcpy( (void*)(recv->tail.message.pack.list.absolute[i].addr) +
						        recv->tail.message.pack.list.absolute[i].begins[j] *
						        (long)recv->tail.message.pack.list.absolute[i].elem_size,
						        send->tail.message.contiguous.addr, size);
						total += size;
						send->tail.message.contiguous.addr += size;
						assert(total <= send->tail.message.contiguous.size);
					}
				}
			}

			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		default:
			not_reachable();
	}
}

static inline void __mpc_comm_copy_buffer_pack_pack(unsigned long *restrict in_begins,
                                                    unsigned long *restrict in_ends, size_t in_sizes,
                                                    void *restrict in_adress, size_t in_elem_size,
                                                    unsigned long *restrict out_begins,
                                                    unsigned long *restrict out_ends,
                                                    size_t out_sizes, void *restrict out_adress,
                                                    size_t out_elem_size)
{
	unsigned long tmp_begin[1];
	unsigned long tmp_end[1];

	if( (in_begins == NULL) && (out_begins == NULL) )
	{
		mpc_common_nodebug("__mpc_comm_copy_buffer_pack_pack no mpc_pack");
		mpc_common_nodebug("%s == %s", out_adress, in_adress);
		memcpy(out_adress, in_adress, in_sizes);
		mpc_common_nodebug("%s == %s", out_adress, in_adress);
	}
	else
	{
		unsigned long i;
		unsigned long j;
		unsigned long in_i;
		unsigned long in_j;
		mpc_common_nodebug("__mpc_comm_copy_buffer_pack_pack mpc_pack");

		if(in_begins == NULL)
		{
			in_begins    = tmp_begin;
			in_begins[0] = 0;
			in_ends      = tmp_end;
			in_ends[0]   = in_sizes - 1;
			in_elem_size = 1;
			in_sizes     = 1;
		}

		if(out_begins == NULL)
		{
			out_begins    = tmp_begin;
			out_begins[0] = 0;
			out_ends      = tmp_end;
			out_ends[0]   = out_sizes - 1;
			out_elem_size = 1;
			out_sizes     = 1;
		}

		in_i = 0;
		in_j = in_begins[in_i] * in_elem_size;

		for(i = 0; i < out_sizes; i++)
		{
			for(j = out_begins[i] * out_elem_size;
			    j <= out_ends[i] * out_elem_size; )
			{
				size_t max_length;

				if(in_j > in_ends[in_i] * in_elem_size)
				{
					in_i++;

					if(in_i >= in_sizes)
					{
						return;
					}

					in_j = in_begins[in_i] * in_elem_size;
				}

				max_length =
				        mpc_common_min( (out_ends[i] * out_elem_size - j + out_elem_size),
				                        (in_ends[in_i] * in_elem_size - in_j + in_elem_size) );
				memcpy(&( ( ( char * )out_adress)[j]), &( ( ( char * )in_adress)[in_j]),
				       max_length);
				mpc_common_nodebug("Copy out[%d-%d]%s == in[%d-%d]%s", j, j + max_length,
				                   &( ( ( char * )out_adress)[j]), in_j, in_j + max_length,
				                   &( ( ( char * )in_adress)[in_j]) );
				j    += max_length;
				in_j += max_length;
			}
		}
	}
}

static inline void __mpc_comm_copy_buffer_absolute_pack(long *restrict in_begins,
                                                        long *restrict in_ends, size_t in_sizes,
                                                        const void *restrict in_adress, size_t in_elem_size,
                                                        unsigned long *restrict out_begins,
                                                        unsigned long *restrict out_ends, size_t out_sizes,
                                                        void *restrict out_adress, size_t out_elem_size)
{
	unsigned long tmp_begin[1];
	unsigned long tmp_end[1];
	long          tmp_begin_abs[1];
	long          tmp_end_abs[1];

	if( (in_begins == NULL) && (out_begins == NULL) )
	{
		mpc_common_nodebug("__mpc_comm_copy_buffer_pack_pack no mpc_pack");
		mpc_common_nodebug("%s == %s", out_adress, in_adress);
		memcpy(out_adress, in_adress, in_sizes);
		mpc_common_nodebug("%s == %s", out_adress, in_adress);
	}
	else
	{
		unsigned long i;
		unsigned long j;
		unsigned long in_i;
		unsigned long in_j;
		mpc_common_nodebug("__mpc_comm_copy_buffer_pack_pack mpc_pack");

		if(in_begins == NULL)
		{
			in_begins    = tmp_begin_abs;
			in_begins[0] = 0;
			in_ends      = tmp_end_abs;
			in_ends[0]   = (long)in_sizes - 1;
			in_elem_size = 1;
			in_sizes     = 1;
		}

		if(out_begins == NULL)
		{
			out_begins    = tmp_begin;
			out_begins[0] = 0;
			out_ends      = tmp_end;
			out_ends[0]   = (unsigned long)out_sizes - 1;
			out_elem_size = 1;
			out_sizes     = 1;
		}

		in_i = 0;
		in_j = (unsigned long)in_begins[in_i] * in_elem_size;

		for(i = 0; i < out_sizes; i++)
		{
			for(j = out_begins[i] * out_elem_size;
			    j <= out_ends[i] * out_elem_size; )
			{
				size_t max_length;

				if(in_j > in_ends[in_i] * in_elem_size)
				{
					in_i++;

					if(in_i >= in_sizes)
					{
						return;
					}

					in_j = in_begins[in_i] * in_elem_size;
				}

				max_length =
				        mpc_common_min( (out_ends[i] * out_elem_size - j + out_elem_size),
				                        (in_ends[in_i] * in_elem_size - in_j + in_elem_size) );
				memcpy(&( ( ( char * )out_adress)[j]), &( ( ( char * )in_adress)[in_j]),
				       max_length);
				mpc_common_nodebug("Copy out[%d-%d]%s == in[%d-%d]%s", j, j + max_length,
				                   &( ( ( char * )out_adress)[j]), in_j, in_j + max_length,
				                   &( ( ( char * )in_adress)[in_j]) );
				j    += max_length;
				in_j += max_length;
			}
		}
	}
}

/*
 * Function without description
 */
static inline void __mpc_comm_copy_buffer_pack_absolute(
        unsigned long *restrict in_begins,
        unsigned long *restrict in_ends, size_t in_sizes,
        void *restrict in_adress, size_t in_elem_size,
        long *restrict out_begins,
        long *restrict out_ends, size_t out_sizes,
        const void *restrict out_adress, size_t out_elem_size)
{
	unsigned long tmp_begin[1];
	unsigned long tmp_end[1];
	long          tmp_begin_abs[1];
	long          tmp_end_abs[1];

	if( (in_begins == NULL) && (out_begins == NULL) )
	{
		mpc_common_nodebug("__mpc_comm_copy_buffer_absolute_absolute no mpc_pack");
		mpc_common_nodebug("%s == %s", out_adress, in_adress);
		memcpy((void*)out_adress, in_adress, in_sizes);
		mpc_common_nodebug("%s == %s", out_adress, in_adress);
	}
	else
	{
		unsigned long i;
		unsigned long j;
		unsigned long in_i;
		unsigned long in_j;
		mpc_common_nodebug("__mpc_comm_copy_buffer_absolute_absolute mpc_pack");

		if(in_begins == NULL)
		{
			in_begins    = tmp_begin;
			in_begins[0] = 0;
			in_ends      = tmp_end;
			in_ends[0]   = in_sizes - 1;
			in_elem_size = 1;
			in_sizes     = 1;
		}

		if(out_begins == NULL)
		{
			out_begins    = tmp_begin_abs;
			out_begins[0] = 0;
			out_ends      = tmp_end_abs;
			out_ends[0]   = out_sizes - 1;
			out_elem_size = 1;
			out_sizes     = 1;
		}

		in_i = 0;
		in_j = in_begins[in_i] * in_elem_size;

		for(i = 0; i < out_sizes; i++)
		{
			for(j = out_begins[i] * out_elem_size;
			    j <= out_ends[i] * out_elem_size; )
			{
				size_t max_length;

				if(in_j > in_ends[in_i] * in_elem_size)
				{
					in_i++;

					if(in_i >= in_sizes)
					{
						return;
					}

					in_j = in_begins[in_i] * in_elem_size;
				}

				max_length =
				        mpc_common_min( (out_ends[i] * out_elem_size - j + out_elem_size),
				                        (in_ends[in_i] * in_elem_size - in_j + in_elem_size) );
				mpc_common_nodebug("Copy out[%lu-%lu]%p == in[%lu-%lu]%p", j, j + max_length,
				                   &( ( ( char * )out_adress)[j]), in_j, in_j + max_length,
				                   &( ( ( char * )in_adress)[in_j]) );
				memcpy(&( ( ( char * )out_adress)[j]), &( ( ( char * )in_adress)[in_j]),
				       max_length);
				mpc_common_nodebug("Copy out[%d-%d]%d == in[%d-%d]%d", j, j + max_length,
				                   ( ( ( char * )out_adress)[j]), in_j, in_j + max_length,
				                   ( ( ( char * )in_adress)[in_j]) );
				j    += max_length;
				in_j += max_length;
			}
		}
	}
}

/*
 * Function without description
 */
static inline void __mpc_comm_copy_buffer_absolute_absolute(
        long *restrict in_begins,
        long *restrict in_ends, size_t in_sizes,
        const void *restrict in_adress, size_t in_elem_size,
        long *restrict out_begins,
        long *restrict out_ends, size_t out_sizes,
        const void *restrict out_adress, size_t out_elem_size)
{
	long tmp_begin[1];
	long tmp_end[1];

	if( (in_begins == NULL) && (out_begins == NULL) )
	{
		mpc_common_nodebug("__mpc_comm_copy_buffer_absolute_absolute no mpc_pack");
		mpc_common_nodebug("%s == %s", out_adress, in_adress);
		memcpy((void*)out_adress, in_adress, in_sizes);
		mpc_common_nodebug("%s == %s", out_adress, in_adress);
	}
	else
	{
		unsigned long i;
		unsigned long j;
		unsigned long in_i;
		unsigned long in_j;
		mpc_common_nodebug("__mpc_comm_copy_buffer_absolute_absolute mpc_pack %p", in_begins);

		/* Empty message */
		if(!in_sizes)
		{
			return;
		}

		if(in_begins == NULL)
		{
			in_begins    = tmp_begin;
			in_begins[0] = 0;
			in_ends      = tmp_end;
			in_ends[0]   = in_sizes - 1;
			in_elem_size = 1;
			in_sizes     = 1;
		}

		if(out_begins == NULL)
		{
			out_begins    = tmp_begin;
			out_begins[0] = 0;
			out_ends      = tmp_end;
			out_ends[0]   = out_sizes - 1;
			out_elem_size = 1;
			out_sizes     = 1;
		}

		in_i = 0;
		in_j = in_begins[in_i] * in_elem_size;

		for(i = 0; i < out_sizes; i++)
		{
			for(j = out_begins[i] * out_elem_size;
			    j <= out_ends[i] * out_elem_size; )
			{
				size_t max_length;

				if(in_j > in_ends[in_i] * in_elem_size)
				{
					in_i++;

					if(in_i >= in_sizes)
					{
						return;
					}

					in_j = in_begins[in_i] * in_elem_size;
				}

				max_length =
				        mpc_common_min( (out_ends[i] * out_elem_size - j + out_elem_size),
				                        (in_ends[in_i] * in_elem_size - in_j + in_elem_size) );
				mpc_common_nodebug("Copy out[%lu-%lu]%p == in[%lu-%lu]%p", j, j + max_length,
				                   &( ( ( char * )out_adress)[j]), in_j, in_j + max_length,
				                   &( ( ( char * )in_adress)[in_j]) );
				memcpy(&( ( ( char * )out_adress)[j]), &( ( ( char * )in_adress)[in_j]),
				       max_length);
				mpc_common_nodebug("Copy out[%d-%d]%d == in[%d-%d]%d", j, j + max_length,
				                   ( ( ( char * )out_adress)[j]), in_j, in_j + max_length,
				                   ( ( ( char * )in_adress)[in_j]) );
				j    += max_length;
				in_j += max_length;
			}
		}
	}
}

/*
 * Function without description
 */
inline void mpc_lowcomm_ptp_message_copy_pack(mpc_lowcomm_ptp_message_content_to_copy_t *tmp)
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;

	send = tmp->msg_send;
	recv = tmp->msg_recv;
	assert(send->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK);

	switch(recv->tail.message_type)
	{
		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			mpc_common_nodebug("MPC_LOWCOMM_MESSAGE_PACK - MPC_LOWCOMM_MESSAGE_PACK");
			size_t i;

			for(i = 0; i < send->tail.message.pack.count; i++)
			{
				__mpc_comm_copy_buffer_pack_pack(send->tail.message.pack.list.std[i].begins,
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

			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			mpc_common_nodebug("MPC_LOWCOMM_MESSAGE_PACK - MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE");
			size_t i;

			for(i = 0; i < send->tail.message.pack.count; i++)
			{
				__mpc_comm_copy_buffer_pack_absolute(
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

			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			mpc_common_nodebug("MPC_LOWCOMM_MESSAGE_PACK - MPC_LOWCOMM_MESSAGE_CONTIGUOUS");
			size_t  i;
			ssize_t j;
			size_t  size;
			char *  body;
			body = recv->tail.message.contiguous.addr;
			mpc_common_nodebug("COUNT %lu", send->tail.message.pack.count);

			for(i = 0; i < send->tail.message.pack.count; i++)
			{
				for(j = 0; j < send->tail.message.pack.list.std[i].count; j++)
				{
					size = (send->tail.message.pack.list.std[i].ends[j] -
					        send->tail.message.pack.list.std[i].begins[j] + 1) *
					       send->tail.message.pack.list.std[i].elem_size;
					memcpy(body, ( ( char * )(send->tail.message.pack.list.std[i].addr) ) +
					       send->tail.message.pack.list.std[i].begins[j] *
					       send->tail.message.pack.list.std[i].elem_size,
					       size);
					body += size;
				}
			}

			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		default:
			not_reachable();
	}
}

/*
 * Function without description
 */
inline void mpc_lowcomm_ptp_message_copy_pack_absolute(mpc_lowcomm_ptp_message_content_to_copy_t *tmp)
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;

	send = tmp->msg_send;
	recv = tmp->msg_recv;
	assert(send->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE);

	switch(recv->tail.message_type)
	{
		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			mpc_common_nodebug("MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE - MPC_LOWCOMM_MESSAGE_PACK");
			size_t i;

			for(i = 0; i < send->tail.message.pack.count; i++)
			{
				__mpc_comm_copy_buffer_absolute_pack(
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

			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			mpc_common_nodebug(
			        "MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE - MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE count == %d",
			        send->tail.message.pack.count);
			size_t i;

			for(i = 0; i < send->tail.message.pack.count; i++)
			{
				__mpc_comm_copy_buffer_absolute_absolute(
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

			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		{
			mpc_common_nodebug("MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE - MPC_LOWCOMM_MESSAGE_CONTIGUOUS");
			size_t  i;
			ssize_t j;
			size_t  size;
			size_t  send_size = 0;
			char *  body;
			body = recv->tail.message.contiguous.addr;
			mpc_common_nodebug("COUNT %lu", send->tail.message.pack.count);

			for(i = 0; i < send->tail.message.pack.count; i++)
			{
				for(j = 0; j < send->tail.message.pack.list.absolute[i].count; j++)
				{
					size = (send->tail.message.pack.list.absolute[i].ends[j] -
					        send->tail.message.pack.list.absolute[i].begins[j] + 1) *
					       send->tail.message.pack.list.absolute[i].elem_size;
					send_size += size;
				}
			}

			mpc_common_nodebug("msg_size = %d, send_size = %d, recv_size = %d",
			                   SCTK_MSG_SIZE(send), send_size,
			                   recv->tail.message.contiguous.size);

			/* MPI 1.3 : The length of the received message must be less than or equal
			 * to the length of the receive buffer */
			assert(send_size <= recv->tail.message.contiguous.size);

			for(i = 0; (i < send->tail.message.pack.count); i++)
			{
				for(j = 0; (j < send->tail.message.pack.list.absolute[i].count); j++)
				{
					size = (send->tail.message.pack.list.absolute[i].ends[j] -
					        send->tail.message.pack.list.absolute[i].begins[j] + 1) *
					       send->tail.message.pack.list.absolute[i].elem_size;
					memcpy(body, ( ( char * )(send->tail.message.pack.list.absolute[i].addr) ) +
					       send->tail.message.pack.list.absolute[i].begins[j] *
					       send->tail.message.pack.list.absolute[i].elem_size,
					       size);
					body += size;
				}
			}

			_mpc_comm_ptp_message_commit_request(send, recv);
			break;
		}

		default:
			not_reachable();
	}
}

/****************
* BUFFERED PTP *
****************/

/* For message creation: a set of buffered ptp_message entries is allocated during init */
#define BUFFERED_PTP_MESSAGE_NUMBER    2048

__thread mpc_lowcomm_ptp_message_t *buffered_ptp_message = NULL;

static void __mpc_comm_buffered_ptp_init(void)
{
	/* Initialize the buffered_ptp_message list for the VP */
	if(buffered_ptp_message == NULL)
	{
			mpc_lowcomm_ptp_message_t *tmp_message;
			tmp_message = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t) *
						BUFFERED_PTP_MESSAGE_NUMBER);
			assume(tmp_message);
			int j;

			/* Loop on all buffers and create a list */
			for(j = 0; j < BUFFERED_PTP_MESSAGE_NUMBER; ++j)
			{
				mpc_lowcomm_ptp_message_t *entry = &tmp_message[j];
				entry->from_buffered = 1;
				/* Add it to the list */
				LL_PREPEND(buffered_ptp_message, entry);
			}
	}
}

/*
 * If the list grows it means that there was a header scarcity at one point
 * so we keep them considering potential repetitive behavior
 */
void __mpc_comm_free_header(void *tmp)
{
	mpc_lowcomm_ptp_message_t *header = ( mpc_lowcomm_ptp_message_t * )tmp;

	mpc_common_nodebug("Free buffer %p buffered?%d", header, header->from_buffered);

	/* Always keep headers if we had to alloc */
	header->from_buffered = 1;
	LL_PREPEND(buffered_ptp_message, header);
}

/*
 * Allocate a free header. Take it from the buffered list is an entry is free.
 * Else,
 * allocate a new header.
 */
static inline mpc_lowcomm_ptp_message_t *__mpc_comm_alloc_header()
{
	mpc_lowcomm_ptp_message_t *tmp = NULL;


	if(buffered_ptp_message != NULL)
	{
		tmp = buffered_ptp_message;
		assert(tmp->from_buffered);
		LL_DELETE(buffered_ptp_message, buffered_ptp_message);
	}


	if(!tmp)
	{
		tmp = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t) );
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
void mpc_lowcomm_init_per_task(int rank)
{
	sctk_net_init_task_level(rank, mpc_topology_get_current_cpu() );

	int comm_id;

	__mpc_comm_ptp_task_init();
	__mpc_comm_buffered_ptp_init();

	for(comm_id = 0; comm_id < SCTK_PARALLEL_COMM_QUEUES_NUMBER; comm_id++)
	{
		mpc_comm_ptp_t *tmp = sctk_malloc(sizeof(mpc_comm_ptp_t) );
		memset(tmp, 0, sizeof(mpc_comm_ptp_t) );
		_mpc_comm_dest_key_init(&tmp->key, (mpc_lowcomm_communicator_id_t)comm_id, rank);
		/* Initialize reordering for the list */
		_mpc_lowcomm_reorder_list_init(&tmp->reorder);
		/* Initialize the internal ptp lists */
		__mpc_comm_ptp_message_list_init(&(tmp->lists) );
		/* And insert them */
		__mpc_comm_ptp_array_insert(tmp);
	}
}

void mpc_lowcomm_release_per_task(int task_rank)
{
	sctk_net_finalize_task_level(task_rank, mpc_topology_get_current_cpu() );
}

/********************************************************************/
/*Message creation                                                  */
/********************************************************************/
void __mpc_comm_ptp_message_pack_free(void *);

void mpc_lowcomm_ptp_message_header_clear(mpc_lowcomm_ptp_message_t *tmp,
                                          mpc_lowcomm_ptp_message_type_t msg_type, void (*free_memory)(void *),
                                          void (*message_copy)(mpc_lowcomm_ptp_message_content_to_copy_t *) )
{
	/* This memset costs 0.03 usecs in pingpong */
	memset(&tmp->tail, 0, sizeof(mpc_lowcomm_ptp_message_tail_t) );
	tmp->tail.message_type = msg_type;

	switch(tmp->tail.message_type)
	{
		case MPC_LOWCOMM_MESSAGE_PACK:
			_mpc_comm_ptp_message_set_copy_and_free(tmp, __mpc_comm_ptp_message_pack_free, mpc_lowcomm_ptp_message_copy_pack);
			break;

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
			_mpc_comm_ptp_message_set_copy_and_free(tmp, __mpc_comm_ptp_message_pack_free, mpc_lowcomm_ptp_message_copy_pack_absolute);
			break;

		default:
			_mpc_comm_ptp_message_set_copy_and_free(tmp, free_memory, message_copy);
			break;
	}
}

mpc_lowcomm_ptp_message_t *mpc_lowcomm_ptp_message_header_create(mpc_lowcomm_ptp_message_type_t msg_type)
{
	mpc_lowcomm_ptp_message_t *tmp;

	tmp = __mpc_comm_alloc_header();
	/* Store if the buffer has been buffered */
	const char from_buffered = tmp->from_buffered;
	/* The copy and free functions will be set after */
	mpc_lowcomm_ptp_message_header_clear(tmp, msg_type, __mpc_comm_free_header, mpc_lowcomm_ptp_message_copy);
	/* Restore it */
	tmp->from_buffered = from_buffered;
	return tmp;
}

void mpc_lowcomm_ptp_message_set_contiguous_addr(mpc_lowcomm_ptp_message_t *restrict msg,
                                                 const void *restrict addr, const size_t size)
{
	assert(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS);
	msg->tail.message.contiguous.size = size;
	msg->tail.message.contiguous.addr = (void *)addr;
}

static inline void __mpc_comm_fill_request(mpc_lowcomm_request_t *request,
										   mpc_lowcomm_communicator_t comm,
                                           int completion,
                                           mpc_lowcomm_ptp_message_t *msg,
                                           int request_type)
{
	assume(msg != NULL);
	__mpc_comm_request_init(request, comm, request_type);
	request->msg                     = msg;
	request->header.source           = SCTK_MSG_SRC_PROCESS_UID(msg);
	request->header.destination      = SCTK_MSG_DEST_PROCESS_UID(msg);
	request->header.destination_task = SCTK_MSG_DEST_TASK(msg);
	request->header.source_task      = SCTK_MSG_SRC_TASK(msg);
	request->header.message_tag      = SCTK_MSG_TAG(msg);
	request->header.msg_size         = SCTK_MSG_SIZE(msg);
	request->is_null                 = 0;
	request->completion_flag         = completion;
	request->status_error            = MPC_LOWCOMM_SUCCESS;
	request->ptr_to_pin_ctx          = NULL;
}

void mpc_lowcomm_ptp_message_header_init(mpc_lowcomm_ptp_message_t *msg,
                                         const int message_tag,
                                         const mpc_lowcomm_communicator_t communicator,
                                         const mpc_lowcomm_peer_uid_t source,
										 const mpc_lowcomm_peer_uid_t destination,
                                         mpc_lowcomm_request_t *request,
										 const size_t count,
                                         mpc_lowcomm_ptp_message_class_t message_class,
                                         mpc_lowcomm_datatype_t datatype,
                                         mpc_lowcomm_request_type_t request_type)
{
	/* This function can fill the header for both process-based messages
	and regular MPI messages this is the reason why desitnation and source
	are UIDs. In the case of regular messages this is downcasted to an int */

	msg->tail.request = request;

	/* Save message class */
	SCTK_MSG_SPECIFIC_CLASS_SET(msg, message_class);

	/* PROCESS SPECIFIC MESSAGES */
	if(_mpc_comm_ptp_message_is_for_process(message_class) )
	{
		/* Fill in Source and Dest Process Informations (NO conversion needed)
		 */

		/* Note ANY_SOURCE will use truncation in matching with mpc_lowcomm_peer_get_rank(source) */
		SCTK_MSG_SRC_PROCESS_UID_SET(msg, source);
		SCTK_MSG_DEST_PROCESS_UID_SET(msg, destination);
		/* Message does not come from a task source */
		SCTK_MSG_SRC_TASK_SET(msg, mpc_common_get_task_rank() );
		/* In all cases such message goes to a process we set the id the first local task */
		SCTK_MSG_DEST_TASK_SET(msg, _mpc_lowcomm_communicator_world_first_local_task());
	}
	else
	{
		int is_intercomm = mpc_lowcomm_communicator_is_intercomm(communicator);

		mpc_lowcomm_peer_uid_t source_process = 0;
		mpc_lowcomm_peer_uid_t dest_process = 0;

		int source_task = -1;
		int dest_task   = -1;
		/* Fill in Source and Dest Process Informations (convert from task) */

		/* SOURCE */
		int isource = mpc_lowcomm_peer_get_rank(source);

		if(mpc_lowcomm_peer_get_rank(source) != MPC_ANY_SOURCE)
		{
			if( is_intercomm )
			{
				if(request_type == REQUEST_RECV)
				{
					/* If this is a RECV make sure the translation is done on the source according to remote */
					source_task = mpc_lowcomm_communicator_remote_world_rank(communicator, isource);
					source_process = mpc_lowcomm_communicator_remote_uid(communicator, isource);
				}
				else if(request_type == REQUEST_SEND)
				{
					/* If this is a SEND make sure the translation is done on the dest according to remote */
					source_task = mpc_lowcomm_communicator_world_rank_of(communicator, isource);
					source_process = mpc_lowcomm_communicator_uid(communicator, isource);
				}
			}
			else
			{
				source_task = mpc_lowcomm_communicator_world_rank_of(communicator, isource);
				source_process = mpc_lowcomm_communicator_uid(communicator, isource);
			}

		}
		else
		{
			source_task = MPC_ANY_SOURCE;
			source_process = mpc_lowcomm_monitor_local_uid_of(MPC_ANY_SOURCE);
		}

		/* DEST Handling */
		int idestination = mpc_lowcomm_peer_get_rank(destination);

		if( is_intercomm )
		{
			if(request_type == REQUEST_RECV)
			{
				/* If this is a RECV make sure the translation is done on the source according to remote */
				dest_task   = mpc_lowcomm_communicator_world_rank_of(communicator, idestination);
				dest_process = mpc_lowcomm_communicator_uid(communicator, idestination);
			}
			else if(request_type == REQUEST_SEND)
			{
				/* If this is a SEND make sure the translation is done on the dest according to remote */
				dest_task   = mpc_lowcomm_communicator_remote_world_rank(communicator, idestination);
				dest_process = mpc_lowcomm_communicator_remote_uid(communicator, idestination);
			}
		}
		else
		{
			dest_task = mpc_lowcomm_communicator_world_rank_of(communicator, idestination);
			dest_process = mpc_lowcomm_communicator_uid(communicator, idestination);
		}

		assert(source_process != 0);
		SCTK_MSG_SRC_PROCESS_UID_SET(msg, source_process);

		assert(dest_process != 0);
		SCTK_MSG_DEST_PROCESS_UID_SET(msg, dest_process);

		SCTK_MSG_SRC_TASK_SET(msg, source_task);
		assert(dest_task != -1);
		SCTK_MSG_DEST_TASK_SET(msg, dest_task);

		assert(
			/* Is in another set set */
			( mpc_lowcomm_peer_get_set(SCTK_MSG_DEST_PROCESS_UID(msg)) != mpc_lowcomm_monitor_get_uid() ) ||
			/* or does overflows task count */
			( (source_task < mpc_common_get_task_count()) && (dest_task < mpc_common_get_task_count()) )
		);


		assert(
			/* Is in another set set */
			( mpc_lowcomm_peer_get_set(SCTK_MSG_DEST_PROCESS_UID(msg)) != mpc_lowcomm_monitor_get_uid() ) ||
			/* or does overflows process count */
			( (SCTK_MSG_DEST_PROCESS(msg) < mpc_common_get_process_count()) && (SCTK_MSG_SRC_PROCESS(msg) < mpc_common_get_process_count()) )
		);

		mpc_common_debug("%s [T(%d -> %d) P(%lu -> %lu) C %llu CL %s TA %d REQ %p]",
		                 mpc_lowcomm_request_type_name[request_type],
		                 source_task,
		                 dest_task,
		                 SCTK_MSG_SRC_PROCESS_UID(msg),
		                 SCTK_MSG_DEST_PROCESS_UID(msg),
		                 mpc_lowcomm_communicator_id(communicator),
		                 mpc_lowcomm_ptp_message_class_name[message_class],
		                 message_tag,
				 request);
	}

	/* Fill in Message meta-data */
	SCTK_MSG_COMMUNICATOR_SET(msg, communicator);
	SCTK_MSG_DATATYPE_SET(msg, datatype);
	SCTK_MSG_TAG_SET(msg, message_tag);
	SCTK_MSG_SIZE_SET(msg, count);
	SCTK_MSG_USE_MESSAGE_NUMBERING_SET(msg, 1);
	SCTK_MSG_MATCH_SET(msg, sctk_m_probe_matching_get() );

	/* A message can be sent with a NULL request (see the MPI standard) */
	if(request)
	{
		__mpc_comm_fill_request(request, communicator, MPC_LOWCOMM_MESSAGE_PENDING, msg, request_type);
		SCTK_MSG_COMPLETION_FLAG_SET(msg, &(request->completion_flag) );
	}
}

/********************************************************************/
/*Message pack creation                                             */
/********************************************************************/

#define SCTK_PACK_REALLOC_STEP    10

void __mpc_comm_ptp_message_pack_free(void *tmp)
{
	mpc_lowcomm_ptp_message_t *msg;

	msg = tmp;

	if(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE)
	{
		sctk_free(msg->tail.message.pack.list.absolute);
	}
	else
	{
		sctk_free(msg->tail.message.pack.list.std);
	}

	__mpc_comm_free_header(tmp);
}

void mpc_lowcomm_ptp_message_add_pack(mpc_lowcomm_ptp_message_t *msg, void *adr,
                                      const unsigned int nb_items,
                                      const size_t elem_size,
                                      unsigned long *begins,
                                      unsigned long *ends)
{
	size_t step;

	if(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_UNDEFINED)
	{
		msg->tail.message_type = MPC_LOWCOMM_MESSAGE_PACK;
		_mpc_comm_ptp_message_set_copy_and_free(msg, __mpc_comm_ptp_message_pack_free, mpc_lowcomm_ptp_message_copy_pack);
	}

	assert(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK);

	if(msg->tail.message.pack.count >= msg->tail.message.pack.max_count)
	{
		msg->tail.message.pack.max_count += SCTK_PACK_REALLOC_STEP;
		msg->tail.message.pack.list.std   =
		        sctk_realloc(msg->tail.message.pack.list.std,
		                     msg->tail.message.pack.max_count *
		                     sizeof(mpc_lowcomm_ptp_message_pack_std_list_t) );
	}

	step = msg->tail.message.pack.count;
	msg->tail.message.pack.list.std[step].count     = nb_items;
	msg->tail.message.pack.list.std[step].begins    = begins;
	msg->tail.message.pack.list.std[step].ends      = ends;
	msg->tail.message.pack.list.std[step].addr      = adr;
	msg->tail.message.pack.list.std[step].elem_size = elem_size;
	msg->tail.message.pack.count++;
}

void mpc_lowcomm_ptp_message_add_pack_absolute(mpc_lowcomm_ptp_message_t *msg,
                                               const void *adr, const unsigned int nb_items,
                                               const size_t elem_size,
                                               long *begins,
                                               long *ends)
{
	size_t step;

	if(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_UNDEFINED)
	{
		msg->tail.message_type = MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE;
		_mpc_comm_ptp_message_set_copy_and_free(msg, __mpc_comm_ptp_message_pack_free, mpc_lowcomm_ptp_message_copy_pack_absolute);
	}

	assert(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE);

	if(msg->tail.message.pack.count >= msg->tail.message.pack.max_count)
	{
		msg->tail.message.pack.max_count    += SCTK_PACK_REALLOC_STEP;
		msg->tail.message.pack.list.absolute =
		        sctk_realloc(msg->tail.message.pack.list.absolute,
		                     msg->tail.message.pack.max_count *
		                     sizeof(mpc_lowcomm_ptp_message_pack_absolute_list_t) );
	}

	step = msg->tail.message.pack.count;
	msg->tail.message.pack.list.absolute[step].count     = nb_items;
	msg->tail.message.pack.list.absolute[step].begins    = begins;
	msg->tail.message.pack.list.absolute[step].ends      = ends;
	msg->tail.message.pack.list.absolute[step].addr      = adr;
	msg->tail.message.pack.list.absolute[step].elem_size = elem_size;
	msg->tail.message.pack.count++;
}

/********************************************************************/
/* Searching functions                                              */
/********************************************************************/

static __thread int did_mprobe = 0;
static __thread OPA_int_t m_probe_id;
static __thread OPA_int_t m_probe_id_task;

void sctk_m_probe_matching_init()
{
	OPA_store_int(&m_probe_id, 0);
	OPA_store_int(&m_probe_id_task, -1);
}

void sctk_m_probe_matching_set(int value)
{
	did_mprobe = 1;

	while(OPA_cas_int(&m_probe_id, 0, value) != 0)
	{
		mpc_common_nodebug("CAS %d", OPA_load_int(&m_probe_id) );
		mpc_thread_yield();
	}

	int thread_id = mpc_common_get_thread_id();

	mpc_common_nodebug("THREAD ID %d", thread_id);
	OPA_store_int(&m_probe_id_task, thread_id + 1);
}

void sctk_m_probe_matching_reset()
{
	OPA_store_int(&m_probe_id, 0);
	OPA_store_int(&m_probe_id_task, -1);
}

int sctk_m_probe_matching_get()
{
	if(!did_mprobe)
	{
		return -1;
	}

	int thread_id = mpc_common_get_thread_id();

	if(OPA_load_int(&m_probe_id_task) != (thread_id + 1) )
	{
		return -1;
	}

	return OPA_load_int(&m_probe_id);
}

__thread volatile int already_processsing_a_control_message = 0;

static inline mpc_lowcomm_msg_list_t *__mpc_comm_pending_msg_list_search_matching(mpc_lowcomm_ptp_list_pending_t *pending_list,
                                                                                  mpc_lowcomm_ptp_message_t * msg)
{
	mpc_lowcomm_ptp_message_header_t *header = &(msg->body.header);

	mpc_lowcomm_msg_list_t *ptr_found = NULL;
	mpc_lowcomm_msg_list_t *tmp = NULL;

	/* Loop on all  pending messages */
	DL_FOREACH_SAFE(pending_list->list, ptr_found, tmp)
	{
		mpc_lowcomm_ptp_message_header_t *header_found;

		assert(ptr_found->msg != NULL);
		header_found = &(ptr_found->msg->body.header);

		/* Control Message Handling */
		if(header_found->message_type.type == MPC_LOWCOMM_CONTROL_MESSAGE_TASK)
		{
			if(!already_processsing_a_control_message)
			{
				DL_DELETE(pending_list->list, ptr_found);
				/* Here we have a pending control message in the list we must take it out in order to avoid deadlocks */
				already_processsing_a_control_message = 1;
				sctk_control_messages_perform(ptr_found->msg, 0);
				already_processsing_a_control_message = 0;
			}
		}

		/* Match the communicator, the tag, the source and the specific message tag */
#if 0
		mpc_common_debug("MATCH COMM [%d-%d] STASK [%d-%d] SPROC [%d-%d] TAG [%d-%d] TYPE [%s-%s]",
		                 header->communicator_id,
						 header_found->communicator_id,
		                 header->source_task,
						 header_found->source_task,
		                 header->source,
						 header_found->source,
		                 header->message_tag,
						 header_found->message_tag,
						 mpc_lowcomm_ptp_message_class_name[(int)header->message_type.type],
						 mpc_lowcomm_ptp_message_class_name[(int)header_found->message_type.type]);
#endif

		if(  /* Match Communicator */
		        (header->communicator_id == header_found->communicator_id) &&
		     /* Match message type */
		        (header->message_type.type == header_found->message_type.type) &&
		     /* Match source Process */
		        ( (header->source == header_found->source) || (mpc_lowcomm_peer_get_rank(header->source) == MPC_ANY_SOURCE) ) &&
		     /* Match source task appart for process specific messages which are not matched at task level */
		        ( (header->source_task == header_found->source_task) ||
		          (header->source_task == MPC_ANY_SOURCE) ||
		          _mpc_comm_ptp_message_is_for_process(header->message_type.type) ) &&
		        /* Match Message Tag while making sure that tags less than 0 are ignored (used for intercomm) */
		        ( (header->message_tag == header_found->message_tag) ||
		          ( (header->message_tag == MPC_ANY_TAG) && (header_found->message_tag >= 0) ) ) )
		{
			/* Message found. We delete it  */
			DL_DELETE(pending_list->list, ptr_found);

			/* Save matching datatypes inside the request */
			if(ptr_found->msg->tail.request)
			{
				ptr_found->msg->tail.request->source_type = header->datatype;
				ptr_found->msg->tail.request->dest_type   = header_found->datatype;
			}

			/* This handles mismatching types in send and recv */
			if(header->msg_size > 0)
			{
				int err = mpc_lowcomm_check_type_compat(header->datatype, header_found->datatype);
				if(err != MPC_LOWCOMM_SUCCESS)
				{
					if(ptr_found->msg->tail.request)
					{
						ptr_found->msg->tail.request->status_error = err;
					}

					if(ptr_found->msg->tail.request)
					{
						ptr_found->msg->tail.request->status_error = err;
					}

					if(msg->tail.request)
					{
						msg->tail.request->status_error = err;
					}
				}
			}

			/* We return the pointer to the request */
			return ptr_found;
		}

		/* Check for canceled send messages*/
		if(header_found->message_type.type == MPC_LOWCOMM_CANCELLED_SEND)
		{
			/* Message found. We delete it  */
			DL_DELETE(pending_list->list, ptr_found);
		}
	}
	/* No message found */
	return NULL;
}

/*
 * Probe for a matching message
 */
static inline int __mpc_comm_ptp_probe(mpc_comm_ptp_t *pair,
                                       mpc_lowcomm_ptp_message_header_t *header)
{
	mpc_lowcomm_msg_list_t *ptr_send;
	mpc_lowcomm_msg_list_t *tmp;

	__mpc_comm_ptp_message_list_merge_pending(&(pair->lists) );
	DL_FOREACH_SAFE(pair->lists.pending_send.list, ptr_send, tmp)
	{
		assert(ptr_send->msg != NULL);
		mpc_lowcomm_ptp_message_header_t *header_send = &(ptr_send->msg->body.header);
		mpc_lowcomm_ptp_message_tail_t *  tail_send   = &(ptr_send->msg->tail);
		int send_message_matching_id =
		        OPA_load_int(&tail_send->matching_id);

		if(  /* Ignore Process Specific */
		        (_mpc_comm_ptp_message_is_for_process(header->message_type.type) == 0) &&
		     /* Has either no or the same matching ID */
		        ( (send_message_matching_id == -1) ||
		          (send_message_matching_id == OPA_load_int(&tail_send->matching_id) ) ) &&
		        /* Match Communicator */
		        ( (header->communicator_id == header_send->communicator_id) ||
		          (header->communicator_id == MPC_ANY_COMM) ) &&
		        /* Match message-type */
		        (header->message_type.type == header_send->message_type.type) &&

		        /* Match source task (note that we ignore source process
		         * here as probe only come from the MPI layer == Only tasks */
		        ( (header->source_task == header_send->source_task) ||
		          (header->source_task == MPC_ANY_SOURCE) ) &&

		        /* Match tag while making sure that tags less than 0 are
		         * ignored (used for intercomm) */
		        ( (header->message_tag == header_send->message_tag) ||
		          ( (header->message_tag == MPC_ANY_TAG) &&
		            (header_send->message_tag >= 0) ) ) )
		{
			int matching_token = sctk_m_probe_matching_get();

			if(matching_token != 0)
			{
				OPA_store_int(&tail_send->matching_id, matching_token);
			}

			memcpy(header, &(ptr_send->msg->body.header),
			       sizeof(mpc_lowcomm_ptp_message_header_t) );
			return 1;
		}
	}
	return 0;
}

static inline int __mpc_comm_pending_msg_list_search_matching_from_recv(mpc_comm_ptp_t *pair, mpc_lowcomm_ptp_message_t *msg)
{
	assert(msg != NULL);
	mpc_lowcomm_msg_list_t *ptr_recv = &msg->tail.distant_list;
	mpc_lowcomm_msg_list_t *ptr_send = __mpc_comm_pending_msg_list_search_matching(&pair->lists.pending_send, msg );

	if(SCTK_MSG_SPECIFIC_CLASS(msg) == MPC_LOWCOMM_CANCELLED_RECV)
	{
		DL_DELETE(pair->lists.pending_recv.list, ptr_recv);
		assert(ptr_send == NULL);
	}

	/* We found a send request which corresponds to the recv request 'ptr_recv' */
	if(ptr_send != NULL)
	{
		/* Recv has matched a send, remove from list */
		DL_DELETE(pair->lists.pending_recv.list, ptr_recv);

		/* If the remote source is on a another node, we call the
		 * notify matching hook in the inter-process layer. We do it
		 * before copying the message to the receive buffer */
		if(msg->tail.remote_source)
		{
			_mpc_lowcomm_multirail_notify_matching(msg);
		}

		/*Insert the matching message to the list of messages that needs to be copied.*/
		return _mpc_comm_ptp_copy_task_insert(ptr_recv, ptr_send, pair);
	}

	return 0;
}

static inline int __mpc_comm_ptp_perform_msg_pair(mpc_comm_ptp_t *pair)
{
	int nb_messages_copied = 0;


	if( ( (pair->lists.pending_send.list != NULL) ||
	      (pair->lists.incomming_send.list != NULL)
	      ) &&
	    ( (pair->lists.pending_recv.list != NULL) ||
	      (pair->lists.incomming_recv.list != NULL)
	    ) )
	{
			if(__mpc_comm_ptp_message_list_trylock_pending(&(pair->lists)) == 0)
            {


			__mpc_comm_ptp_message_list_merge_pending(&(pair->lists) );

			mpc_lowcomm_msg_list_t *ptr_recv;
			mpc_lowcomm_msg_list_t *tmp;

			DL_FOREACH_SAFE(pair->lists.pending_recv.list, ptr_recv, tmp)
			{
				nb_messages_copied += __mpc_comm_pending_msg_list_search_matching_from_recv(pair, ptr_recv->msg);
			}

			__mpc_comm_ptp_message_list_unlock_pending(&(pair->lists) );
			return _mpc_comm_ptp_task_perform(pair->key.rank);
            }

	}

	return nb_messages_copied;
}

/*
 * Try to match all received messages with send requests from a internal ptp
 * list
 *
 * Returns:
 *  -1 Lock already taken
 *  >=0 Number of messages matched
 */
static inline int __mpc_comm_ptp_perform_msg_pair_trylock(mpc_comm_ptp_t *pair)
{
	int ret = -1;

	if(!pair)
	{
		return ret;
	}

	SCTK_PROFIL_START(MPC_Test_message_pair_try);

	/* If the lock has not been taken, we continue */
	if(OPA_load_int(&pair->lists.pending_lock) == 0)
	{
		return __mpc_comm_ptp_perform_msg_pair(pair);
	}

	SCTK_PROFIL_END(MPC_Test_message_pair_try);
	return ret;
}

void mpc_lowcomm_ptp_msg_wait_init(struct mpc_lowcomm_ptp_msg_progress_s *wait,
                                   mpc_lowcomm_request_t *request, int blocking)
{
	wait->request = request;
	/* If we are in a MPI_Wait function or similar */
	wait->blocking = blocking;
	mpc_comm_ptp_t **recv_ptp = &wait->recv_ptp;
	mpc_comm_ptp_t **send_ptp = &wait->send_ptp;
	/* Searching for the pending list corresponding to the dest task */
	*recv_ptp = __mpc_comm_ptp_array_get(request->comm, request->header.destination_task);
	*send_ptp = __mpc_comm_ptp_array_get(request->comm, request->header.source_task);

	if(request->request_type == REQUEST_SEND)
	{
		wait->polling_task_id = request->header.source_task;
	}
	else if(request->request_type == REQUEST_RECV)
	{
		wait->polling_task_id = request->header.destination_task;
	}

	if(request->request_type == REQUEST_GENERALIZED)
	{
		wait->polling_task_id = request->header.source_task;
	}

	/* Compute the rank of the remote process */
	if(request->header.source_task != MPC_ANY_SOURCE &&
	   request->comm != MPC_COMM_NULL &&
	   request->header.source_task != MPC_PROC_NULL)
	{
		/* Convert task rank to process rank */
		wait->remote_process = mpc_lowcomm_monitor_local_uid_of(MPC_PROC_NULL);
		//mpc_lowcomm_communicator_uid_as(request->comm, request->header.source_task, request->header.destination_task, mpc_lowcomm_monitor_get_uid());
	}
	else
	{
		wait->remote_process = mpc_lowcomm_monitor_local_uid_of(MPC_PROC_NULL);;
		wait->source_task_id = -1;
	}
}

/*
 *  Function called when the message to receive is already completed
 */
static inline void __mpc_comm_ptp_msg_done(struct mpc_lowcomm_ptp_msg_progress_s *wait)
{
	const mpc_lowcomm_request_t *request = wait->request;
#if 0
  	const mpc_comm_ptp_t *recv_ptp = wait->recv_ptp;
#endif

	/* The message is marked as done.
	 * However, we need to poll if it is a inter-process message
	 * and if we are waiting for a SEND request. If we do not do this,
	 * we might overflow the number of send buffers waiting to be released */
	if(request->header.source_task == MPC_ANY_SOURCE)
	{
		_mpc_lowcomm_multirail_notify_anysource(request->header.source_task, 0);
	}
#if 0
    else if (request->request_type == REQUEST_SEND && !recv_ptp) {
		const int remote_process = wait->remote_process;
		const int source_task_id = wait->source_task_id;
		/* This call may INCREASE the latency in the send... */
		_mpc_lowcomm_multirail_notify_perform (remote_process, source_task_id, request->header.source_task, 0);
 	}
#endif

	/* Propagate finish to parent request if present */
	if(request->pointer_to_source_request)
	{
		( ( mpc_lowcomm_request_t * )request->pointer_to_source_request)->completion_flag =
		        MPC_LOWCOMM_MESSAGE_DONE;
	}

	/* Free the shadow request bound if present (wait over the source ) */
	if(request->pointer_to_shadow_request)
	{
		( ( mpc_lowcomm_request_t * )request->pointer_to_shadow_request)
		->pointer_to_source_request = NULL;
	}

}

static inline void __mpc_comm_ptp_msg_wait(struct mpc_lowcomm_ptp_msg_progress_s *wait);

static void __mpc_comm_perform_msg_wfv(void *a)
{
	struct mpc_lowcomm_ptp_msg_progress_s *_wait = ( struct mpc_lowcomm_ptp_msg_progress_s * )a;

	__mpc_comm_ptp_msg_wait(_wait);

	if( ( volatile int )_wait->request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
  {
		_mpc_lowcomm_multirail_notify_idle();
    MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
  }
}

void mpc_lowcomm_perform_idle(volatile int *data, int value,
                              void (*func)(void *), void *arg)
{
	if(*data == value)
	{
		return;
	}

#ifndef MPC_Threads
	while(*data != value)
	{
		func(arg);
    MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}
#else
	mpc_thread_wait_for_value_and_poll(data, value, func, arg);
#endif
}

static void (*__egreq_poll_trampoline)(void) = NULL;

void mpc_lowcomm_egreq_poll_set_trampoline(void (*trampoline)(void) )
{
	__egreq_poll_trampoline = trampoline;
}

static inline void __egreq_poll()
{
	if(__egreq_poll_trampoline)
	{
		(__egreq_poll_trampoline)();
	}
}

static inline int __request_is_null_or_cancelled(mpc_lowcomm_request_t *request)
{
	if(!request){
		return 1;
	}

	if(mpc_lowcomm_request_is_null(request) )
	{
		return 1;
	}

	if(request->completion_flag == MPC_LOWCOMM_MESSAGE_CANCELED)
	{
		return 1;
	}

	if(request->request_type == REQUEST_NULL)
	{
		return 1;
	}

	return 0;
}

int mpc_lowcomm_request_wait(mpc_lowcomm_request_t *request)
{
	if(__request_is_null_or_cancelled(request))
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	if(mpc_lowcomm_request_get_source(request) == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_set_null(request, 1);
	}

#ifdef MPC_MPI
	if(request->request_type == REQUEST_GENERALIZED)
	{
		mpc_lowcomm_perform_idle( ( int * )&(request->completion_flag),
		                          MPC_LOWCOMM_MESSAGE_DONE, __egreq_poll, NULL);
	}
	else
#endif
	{
		struct mpc_lowcomm_ptp_msg_progress_s _wait;


		/* Find the PTPs lists */
		mpc_lowcomm_ptp_msg_wait_init(&_wait, request, 1);
		/* Fastpath try a few times directly before polling */
		int trials = 0;

		do
		{
		    __mpc_comm_ptp_msg_wait(&_wait);
			trials++;
		} while( (request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE) && (trials < 16) );

		if(request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
		{
			mpc_lowcomm_perform_idle(
			        ( int * )&(_wait.request->completion_flag), MPC_LOWCOMM_MESSAGE_DONE,
			        (void (*)(void *) )__mpc_comm_perform_msg_wfv,
			        &_wait);
		}
		else
		{
			__mpc_comm_ptp_msg_done(&_wait);
		}
	}

	return MPC_LOWCOMM_SUCCESS;
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

static inline void __mpc_comm_ptp_msg_wait(struct mpc_lowcomm_ptp_msg_progress_s *wait)
{
	const mpc_lowcomm_request_t *request = wait->request;
	mpc_comm_ptp_t *recv_ptp             = wait->recv_ptp;
	mpc_comm_ptp_t *send_ptp             = wait->send_ptp;
	const mpc_lowcomm_peer_uid_t remote_process = wait->remote_process;
	const int       source_task_id       = wait->source_task_id;
	const int       polling_task_id      = wait->polling_task_id;
	const int       blocking             = wait->blocking;

	if(request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
	{
		/* Check the source of the request. We try to poll the
		 *         source in order to retreive messages from the network */

		/* We try to poll for finding a message with a MPC_ANY_SOURCE source
		 * also in case we are blocked we punctually poll any-source */
		if(request->header.source_task == MPC_ANY_SOURCE)
		{
			/* We try to poll for finding a message with a MPC_ANY_SOURCE source */
			_mpc_lowcomm_multirail_notify_anysource(polling_task_id, blocking);
		}
		else if( (!recv_ptp) || (!send_ptp) )
		{
			/* We poll the network only if we need it (empty queues) */
			_mpc_lowcomm_multirail_notify_perform(remote_process, source_task_id, polling_task_id, blocking);
		}


		if( (request->request_type == REQUEST_SEND) && send_ptp)
		{
			__mpc_comm_ptp_perform_msg_pair_trylock(send_ptp);
		}
		else if( (request->request_type == REQUEST_RECV) && recv_ptp)
		{
			__mpc_comm_ptp_perform_msg_pair_trylock(recv_ptp);
		}

	}
	else
	{
		__mpc_comm_ptp_msg_done(wait);
	}
}

/* This is the exported version */
void mpc_lowcomm_ptp_msg_progress(struct mpc_lowcomm_ptp_msg_progress_s *wait)
{
	return __mpc_comm_ptp_msg_wait(wait);
}

/********************************************************************/
/* Send messages                                                    */
/********************************************************************/

/*
 * Function called when sending a message. The message can be forwarded to
 * another process
 * using the '_mpc_lowcomm_multirail_send_message' function. If the message
 * matches, we add it to the corresponding pending list
 * */
void _mpc_comm_ptp_message_send_check(mpc_lowcomm_ptp_message_t *msg, int poll_receiver_end)
{
	assert(mpc_common_get_process_rank() >= 0);
	assert( 
		/* Does not overflow */
		(mpc_common_get_process_count() >= SCTK_MSG_DEST_PROCESS(msg)) ||
		/* Is different process set */
		( mpc_lowcomm_peer_get_set(SCTK_MSG_DEST_PROCESS_UID(msg)) != mpc_lowcomm_monitor_get_gid() )
	 );

	/* Flag msg's request as a send one */
	if(msg->tail.request != NULL)
	{
		msg->tail.request->request_type = REQUEST_SEND;
	}

    mpc_common_nodebug("%llu != %llu ?", SCTK_MSG_DEST_PROCESS_UID(msg), mpc_lowcomm_monitor_get_uid());
	/*  Message has not reached its destination */
	if(SCTK_MSG_DEST_PROCESS_UID(msg) != mpc_lowcomm_monitor_get_uid() )
	{
		/* We forward the message */
		_mpc_lowcomm_multirail_send_message(msg);
		return;
	}

	if(_mpc_comm_ptp_message_is_for_control(SCTK_MSG_SPECIFIC_CLASS(msg) ) )
	{
		/* If we are on the right process with a control message */

		/* Message is for local process call the handler (such message are never pending)
		 *     no need to perform a matching with a receive. However, the order is guaranteed
		 *         by the reorder prior to calling this function */
		sctk_control_messages_incoming(msg);
		/* Done */
		return;
	}

	if(SCTK_MSG_COMPLETION_FLAG(msg) != NULL)
	{
		*(SCTK_MSG_COMPLETION_FLAG(msg) ) = MPC_LOWCOMM_MESSAGE_PENDING;
	}

	mpc_common_debug("SEND msg from %llu to %llu HERE %llu (FT %d TT %d)", SCTK_MSG_SRC_PROCESS_UID(msg), SCTK_MSG_DEST_PROCESS_UID(msg), mpc_lowcomm_monitor_get_uid(), SCTK_MSG_SRC_TASK(msg), SCTK_MSG_DEST_TASK(msg) );

	/* If the message is a UNIVERSE message it targets the process not a given task
	  we then use the matching queue from the first local process as a convention */
	if(SCTK_MSG_SPECIFIC_CLASS(msg) == MPC_LOWCOMM_MESSAGE_UNIVERSE)
	{
		SCTK_MSG_DEST_TASK_SET(msg, _mpc_lowcomm_communicator_world_first_local_task());
	}

	/* Flag the Message as all local */
	msg->tail.remote_source = 0;
	/* We are searching for the corresponding pending list. If we do not find any entry, we forward the message */
	mpc_comm_ptp_t *src_pair  = __mpc_comm_ptp_array_get(SCTK_MSG_COMMUNICATOR(msg), SCTK_MSG_SRC_TASK(msg) );
	mpc_comm_ptp_t *dest_pair = __mpc_comm_ptp_array_get(SCTK_MSG_COMMUNICATOR(msg), SCTK_MSG_DEST_TASK(msg) );

	if(!src_pair)
	{
		/* Message comes from network */
		msg->tail.internal_ptp = NULL;
	}

	assert(dest_pair != NULL);
	__mpc_comm_ptp_message_list_add_incoming_send(dest_pair, msg);

	/* If we ask for a checking, we check it */
	if(poll_receiver_end)
	{
		__UNUSED__ int matched_nb;

		matched_nb = __mpc_comm_ptp_perform_msg_pair_trylock(dest_pair);
#ifdef MPC_Profiler
		switch(matched_nb)
		{
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
}

/*
 * Function called for sending a message (i.e: MPI_Send).
 * Mostly used by the file mpc.c
 * */
void mpc_lowcomm_ptp_message_send(mpc_lowcomm_ptp_message_t *msg)
{
	mpc_common_debug("SEND to %d (size %ld) %s COMM %llu T %d", SCTK_MSG_DEST_TASK(msg),
	                 SCTK_MSG_SIZE(msg),
	                 mpc_lowcomm_ptp_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
	                 SCTK_MSG_COMMUNICATOR_ID(msg),
	                 SCTK_MSG_TAG(msg) );


	int need_check = !_mpc_comm_is_remote_rank(SCTK_MSG_DEST_TASK(msg) );

	_mpc_comm_ptp_message_send_check(msg, need_check);
}

/********************************************************************/
/* Recv messages                                                    */
/********************************************************************/

/*
 * Function called when receiving a message.
 * */
void _mpc_comm_ptp_message_recv_check(mpc_lowcomm_ptp_message_t *msg,
                                      int perform_check)
{
	if(SCTK_MSG_COMPLETION_FLAG(msg) != NULL)
	{
		*(SCTK_MSG_COMPLETION_FLAG(msg) ) = MPC_LOWCOMM_MESSAGE_PENDING;
	}

	if(msg->tail.request != NULL)
	{
		msg->tail.request->request_type = REQUEST_RECV;
	}

	/* We are searching for the list corresponding to the task which receives the message */
	mpc_comm_ptp_t *send_ptp = __mpc_comm_ptp_array_get(SCTK_MSG_COMMUNICATOR(msg), SCTK_MSG_SRC_TASK(msg) );
	mpc_comm_ptp_t *recv_ptp = __mpc_comm_ptp_array_get(SCTK_MSG_COMMUNICATOR(msg), SCTK_MSG_DEST_TASK(msg) );
	/* We assume that the entry is found. If not, the message received is for a task which is not registered on the node. Possible issue. */
	assert(recv_ptp);

	if(send_ptp == NULL)
	{
		/* This receive expects a message from a remote process */
		msg->tail.remote_source = 1;
		_mpc_lowcomm_multirail_notify_receive(msg);
	}
	else
	{
		msg->tail.remote_source = 0;
	}

	/* We add the message to the pending list */
	__mpc_comm_ptp_message_list_add_incoming_recv(recv_ptp, msg);

	/* Iw we ask for a matching, we run it */
	if(perform_check)
	{
		__mpc_comm_ptp_perform_msg_pair_trylock(recv_ptp);
	}
}

/*
 * Function called for receiving a message (i.e: MPI_Recv).
 * Mostly used by the file mpc.c
 * */
void mpc_lowcomm_ptp_message_recv(mpc_lowcomm_ptp_message_t *msg)
{
	int need_check = !_mpc_comm_is_remote_rank(SCTK_MSG_DEST_TASK(msg) );

	mpc_common_debug("RECV from %d (size %ld) %s COMM %llu T %d", SCTK_MSG_SRC_TASK(msg),
	                 SCTK_MSG_SIZE(msg),
	                 mpc_lowcomm_ptp_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
	                 SCTK_MSG_COMMUNICATOR_ID(msg),
	                 SCTK_MSG_TAG(msg) );

	_mpc_comm_ptp_message_recv_check(msg, need_check);
}

/********************************************************************/
/*Probe                                                             */
/********************************************************************/

static inline void
__mpc_comm_probe_source_tag_class_func(int destination, int source, int tag,
                                       mpc_lowcomm_ptp_message_class_t class,
                                       const mpc_lowcomm_communicator_t comm,
                                       int *status,
                                       mpc_lowcomm_ptp_message_header_t *msg)
{
	int world_source      = source;
	int world_destination = destination;

	msg->source_task       = world_source;
	msg->destination_task  = world_destination;
	msg->message_tag       = tag;
	msg->communicator_id   = comm->id;
	msg->message_type.type = class;

	_mpc_lowcomm_multirail_notify_probe(msg, status);

	switch ( *status )
	{
		case 0: /* all rails supported probing request AND not found */
		case 1: /* found a match ! */
			return;
			break;
		default: /* not found AND at least one rail does not support low-level probing */
			*status = 0;
			msg->msg_size = 0;
			break;
	}

	mpc_comm_ptp_t *dest_ptp = __mpc_comm_ptp_array_get(comm, world_destination);

	if(source != MPC_ANY_SOURCE)
	{
		mpc_comm_ptp_t *src_ptp = __mpc_comm_ptp_array_get(comm, world_source);

		if(src_ptp == NULL)
		{
			int src_process = mpc_lowcomm_group_process_rank_from_world(world_source);
			_mpc_lowcomm_multirail_notify_perform(src_process, world_source, world_destination, 0);
		}
	}
	else
	{
		_mpc_lowcomm_multirail_notify_anysource(world_destination, 0);
	}

	assert(dest_ptp != NULL);
	__mpc_comm_ptp_message_list_lock_pending(&(dest_ptp->lists) );
	*status = __mpc_comm_ptp_probe(dest_ptp, msg);
	__mpc_comm_ptp_message_list_unlock_pending(&(dest_ptp->lists) );
}

void mpc_lowcomm_message_probe_any_tag(int destination, int source,
                                       const mpc_lowcomm_communicator_t comm, int *status,
                                       mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, source, MPC_ANY_TAG,
	                                       MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg);
}

void mpc_lowcomm_message_probe_any_source(int destination, const mpc_lowcomm_communicator_t comm,
                                          int *status, mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, MPC_ANY_SOURCE,
	                                       msg->message_tag, MPC_LOWCOMM_P2P_MESSAGE, comm,
	                                       status, msg);
}

void mpc_lowcomm_message_probe_any_source_any_tag(int destination,
                                                  const mpc_lowcomm_communicator_t comm, int *status,
                                                  mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, MPC_ANY_SOURCE, MPC_ANY_TAG,
	                                       MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg);
}

void mpc_lowcomm_message_probe(int destination, int source,
                               const mpc_lowcomm_communicator_t comm, int *status,
                               mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, source, msg->message_tag,
	                                       MPC_LOWCOMM_P2P_MESSAGE, comm, status, msg);
}

void mpc_lowcomm_message_probe_any_source_class_comm(int destination, int tag,
                                                     mpc_lowcomm_ptp_message_class_t class,
                                                     const mpc_lowcomm_communicator_t comm,
                                                     int *status,
                                                     mpc_lowcomm_ptp_message_header_t *msg)
{
	__mpc_comm_probe_source_tag_class_func(destination, MPC_ANY_SOURCE, tag, class,
	                                       comm, status, msg);
}

/********************************************************************/
/*Message Interface                                                 */
/********************************************************************/


void mpc_lowcomm_request_init(mpc_lowcomm_request_t *request, mpc_lowcomm_communicator_t comm, int request_type)
{
	__mpc_comm_request_init(request, comm, request_type);
}

int mpc_lowcomm_request_cancel(mpc_lowcomm_request_t *msg)
{
	int ret = MPC_LOWCOMM_SUCCESS;

	switch(msg->request_type)
	{
		case REQUEST_GENERALIZED:

			/* Call the cancel handler with a flag telling if the request was completed
			 * which is our case is the same as not pending anymore */
			ret = (msg->cancel_fn)(msg->extra_state,
			                       (msg->completion_flag != MPC_LOWCOMM_MESSAGE_PENDING) );
			break;

		case REQUEST_RECV:
			if(msg->msg == NULL)
			{
				return ret;
			}

			SCTK_MSG_SPECIFIC_CLASS_SET(msg->msg, MPC_LOWCOMM_CANCELLED_RECV);
			break;

		case REQUEST_SEND:
			if(msg->msg == NULL)
			{
				return ret;
			}

			/* NOTE: cancelling a Send is deprecated */
			if(_mpc_comm_is_remote_rank(SCTK_MSG_DEST_PROCESS(msg->msg) ) )
			{
				mpc_common_debug_error("Try to cancel a network message for %d from UNIX process %d",
				                       SCTK_MSG_DEST_PROCESS(msg->msg), mpc_common_get_process_rank() );
				not_implemented();
			}

			SCTK_MSG_SPECIFIC_CLASS_SET(msg->msg, MPC_LOWCOMM_CANCELLED_SEND);
			break;

		default:
			not_reachable();
			break;
	}

	msg->completion_flag = MPC_LOWCOMM_MESSAGE_CANCELED;
	return ret;
}

static inline int __mpc_lowcomm_isend_class_src(int src, int dest, const void *data, size_t size,
                                                 int tag, mpc_lowcomm_communicator_t comm,
                                                 mpc_lowcomm_ptp_message_class_t class,
                                                 mpc_lowcomm_request_t *req)
{
	if(dest == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(req, comm, REQUEST_SEND);
		return MPC_LOWCOMM_SUCCESS;
	}

	mpc_lowcomm_ptp_message_t *msg =
	        mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);
	mpc_lowcomm_ptp_message_set_contiguous_addr(msg, data, size);
	mpc_lowcomm_ptp_message_header_init(msg, tag, comm, src, dest, req, size, class,
	                                    MPC_DATATYPE_IGNORE, REQUEST_SEND);
	mpc_lowcomm_ptp_message_send(msg);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_isend_class_src(int src, int dest, const void *data, size_t size,
                                 int tag, mpc_lowcomm_communicator_t comm,
                                 mpc_lowcomm_ptp_message_class_t class,
                                 mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_isend_class_src(src, dest, data, size, tag, comm, class, req);
}

static inline int __mpc_lowcomm_isend_class(int dest, const void *data, size_t size, int tag,
                                             mpc_lowcomm_communicator_t comm,
                                             mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_isend_class_src(mpc_lowcomm_communicator_rank_of(comm, mpc_common_get_task_rank() ),
	                              dest,
	                              data,
	                              size,
	                              tag,
	                              comm,
	                              class,
	                              req);
}

int mpc_lowcomm_isend_class(int dest, const void *data, size_t size, int tag,
                             mpc_lowcomm_communicator_t comm,
                             mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_isend_class(dest,
	                          data,
	                          size,
	                          tag,
	                          comm,
	                          class,
	                          req);
}

static inline int __mpc_lowcomm_irecv_class_dest(int src, int dest, void *buffer, size_t size,
                                                  int tag, mpc_lowcomm_communicator_t comm,
                                                  mpc_lowcomm_ptp_message_class_t class,
                                                  mpc_lowcomm_request_t *req)
{
	if(src == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(req, comm, REQUEST_RECV);
		return MPC_LOWCOMM_SUCCESS;
	}

	mpc_lowcomm_ptp_message_t *msg =
	mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);
	mpc_lowcomm_ptp_message_set_contiguous_addr(msg, buffer, size);
	mpc_lowcomm_ptp_message_header_init(msg, tag, comm, src, dest, req, size, class,
	                                    MPC_DATATYPE_IGNORE, REQUEST_RECV);
	mpc_lowcomm_ptp_message_recv(msg);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_irecv_class_dest(int src, int dest, void *buffer, size_t size,
                                  int tag, mpc_lowcomm_communicator_t comm,
                                  mpc_lowcomm_ptp_message_class_t class,
                                  mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_irecv_class_dest(src, dest, buffer, size, tag, comm, class, req);
}

static inline int __mpc_lowcomm_irecv_class(int src, void *buffer, size_t size, int tag,
                                             mpc_lowcomm_communicator_t comm,
                                             mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_irecv_class_dest(src, mpc_lowcomm_communicator_rank_of(comm, mpc_common_get_task_rank() ),
	                               buffer, size, tag, comm, class, req);
}

int mpc_lowcomm_irecv_class(int src, void *buffer, size_t size, int tag,
                             mpc_lowcomm_communicator_t comm,
                             mpc_lowcomm_ptp_message_class_t class, mpc_lowcomm_request_t *req)
{
	return __mpc_lowcomm_irecv_class(src, buffer, size, tag, comm, class, req);
}

/*************************
 * PUBLIC COMM INTERFACE *
 *************************/

/**************
 * RANK QUERY *
 **************/

int mpc_lowcomm_get_rank()
{
	return mpc_common_get_task_rank();
}

int mpc_lowcomm_get_size()
{
	return mpc_common_get_task_count();
}

int mpc_lowcomm_get_comm_rank(const mpc_lowcomm_communicator_t communicator)
{
	return mpc_lowcomm_communicator_rank_of(communicator, mpc_lowcomm_get_rank() );
}

int mpc_lowcomm_get_comm_size(const mpc_lowcomm_communicator_t communicator)
{
	return mpc_lowcomm_communicator_size(communicator);
}

int mpc_lowcomm_get_process_count()
{
	return mpc_common_get_process_count();
}

int mpc_lowcomm_get_process_rank()
{
	return mpc_common_get_process_rank();
}

/************
 * MESSAGES *
 ************/

static mpc_lowcomm_request_t __request_null;

static inline void __init_request_null()
{
	mpc_lowcomm_request_init(&__request_null, MPC_COMM_NULL, REQUEST_NULL);
	__request_null.is_null = 1;
}
mpc_lowcomm_request_t* mpc_lowcomm_request_null(void)
{
	return &__request_null;
}

int mpc_lowcomm_isend(int dest, const void *data, size_t size, int tag,
                       mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req)
{
	return mpc_lowcomm_isend_class(dest, data, size, tag, comm, MPC_LOWCOMM_P2P_MESSAGE, req);
}

int mpc_lowcomm_irecv(int src, void *data, size_t size, int tag,
                       mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req)
{
	return mpc_lowcomm_irecv_class(src, data, size, tag, comm, MPC_LOWCOMM_P2P_MESSAGE, req);
}

int mpc_lowcomm_sendrecv(void *sendbuf, size_t size, int dest, int tag, void *recvbuf,
                          int src, mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_request_t sendreq, recvreq;

	mpc_lowcomm_isend(dest, sendbuf, size, tag, comm, &sendreq);
	mpc_lowcomm_irecv(src, recvbuf, size, tag, comm, &recvreq);

	mpc_lowcomm_status_t sts, str;

	mpc_lowcomm_wait(&sendreq, &sts);
	mpc_lowcomm_wait(&recvreq, &str);

	CHECK_STATUS_ERROR(&sts);
	CHECK_STATUS_ERROR(&str);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_wait(mpc_lowcomm_request_t *request, mpc_lowcomm_status_t *status)
{
	mpc_lowcomm_request_wait(request);
	mpc_lowcomm_commit_status_from_request(request, status);
	mpc_lowcomm_request_set_null(request, 1);
	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_test(mpc_lowcomm_request_t * request, int * completed, mpc_lowcomm_status_t *status)
{
	*completed = 0;

	/* Always make sure to start on clean grounds */
	if(status)
	{
		status->MPC_ERROR=MPC_LOWCOMM_SUCCESS;
	}

	TODO("UNDERSTAND why this happens");
	if(!request->comm)
	{
		/* We stop here as if we go waiting we crash */
		*completed = (request->completion_flag == MPC_LOWCOMM_MESSAGE_DONE);
		return MPC_LOWCOMM_SUCCESS;
	}

	struct mpc_lowcomm_ptp_msg_progress_s wait;

	if(__request_is_null_or_cancelled(request))
	{
		*completed = 1;
		return MPC_LOWCOMM_SUCCESS;
	}

	mpc_lowcomm_ptp_msg_wait_init(&wait, request, 1);

	__mpc_comm_ptp_msg_wait(&wait);

	*completed = (request->completion_flag == MPC_LOWCOMM_MESSAGE_DONE);

	if(*completed)
	{
		mpc_lowcomm_commit_status_from_request(request, status);
		mpc_lowcomm_request_set_null(request, 1);
		__mpc_comm_ptp_msg_done(&wait);
	}
	else
	{
		_mpc_lowcomm_multirail_notify_idle();
	}


	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_waitall(int count, mpc_lowcomm_request_t *requests, mpc_lowcomm_status_t *statuses)
{
	int all_done = 0;
	int i;

	while(!all_done)
	{
		all_done = 1;
		for( i = 0; i < count ; i++)
		{
			int done = 0;

			mpc_lowcomm_status_t *status = NULL;
			if(statuses)
			{
				status = &statuses[i];
			}

			mpc_lowcomm_test(&requests[i], &done, status);
			all_done &= done;
		}

		if(!all_done)
		{
			/* Yield for coroutines */
			mpc_thread_yield();
		}
	}

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_send(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_request_t req;

	mpc_lowcomm_isend(dest, data, size, tag, comm, &req);
	mpc_lowcomm_status_t status;
	mpc_lowcomm_wait(&req, &status);
	CHECK_STATUS_ERROR(&status);
	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_recv(int src, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_request_t req;

	mpc_lowcomm_irecv(src, buffer, size, tag, comm, &req);
	mpc_lowcomm_status_t status;
	mpc_lowcomm_wait(&req, &status);
	CHECK_STATUS_ERROR(&status);
	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_iprobe_src_dest(const int world_source, const int world_destination, const int tag,
                                const mpc_lowcomm_communicator_t comm, int *flag, mpc_lowcomm_status_t *status)
{
	mpc_lowcomm_ptp_message_header_t msg;

	memset(&msg, 0, sizeof(mpc_lowcomm_ptp_message_header_t) );
	mpc_lowcomm_status_t status_init = MPC_LOWCOMM_STATUS_INIT;
	int has_status = 1;

	if(status == MPC_LOWCOMM_STATUS_NULL)
	{
		has_status = 0;
	}
	else
	{
		*status = status_init;
	}

	*flag = 0;

	/*handler for MPC_PROC_NULL*/
	if(world_source == MPC_PROC_NULL)
	{
		*flag = 1;

		if(has_status)
		{
			status->MPC_SOURCE = MPC_PROC_NULL;
			status->MPC_TAG    = MPC_ANY_TAG;
			status->size       = 0;
			status->MPC_ERROR  = MPC_LOWCOMM_SUCCESS;
		}

		return MPC_LOWCOMM_SUCCESS;
	}

	/* Value to check that the case was handled by
	 * one of the if in this function */
	int __did_process = 0;

	if( (world_source == MPC_ANY_SOURCE) && (tag == MPC_ANY_TAG) )
	{
		mpc_lowcomm_message_probe_any_source_any_tag(world_destination, comm, flag, &msg);
		__did_process = 1;
	}
	else if( (world_source != MPC_ANY_SOURCE) && (tag != MPC_ANY_TAG) )
	{
		msg.message_tag = tag;
		mpc_lowcomm_message_probe(world_destination, world_source, comm, flag, &msg);
		__did_process = 1;
	}
	else if( (world_source != MPC_ANY_SOURCE) && (tag == MPC_ANY_TAG) )
	{
		mpc_lowcomm_message_probe_any_tag(world_destination, world_source, comm, flag, &msg);
		__did_process = 1;
	}
	else if( (world_source == MPC_ANY_SOURCE) && (tag != MPC_ANY_TAG) )
	{
		msg.message_tag = tag;
		mpc_lowcomm_message_probe_any_source(world_destination, comm, flag, &msg);
		__did_process = 1;
	}

	if(*flag)
	{
		if(has_status)
		{
			status->MPC_SOURCE = mpc_lowcomm_communicator_rank_of_as(comm, msg.source_task, msg.source_task, msg.source);
			status->MPC_TAG    = msg.message_tag;
			status->size       = ( mpc_lowcomm_msg_count_t )msg.msg_size;
			status->MPC_ERROR = MPC_LOWCOMM_SUCCESS;
#ifdef MPC_MPI
			status->MPC_ERROR  = MPC_LOWCOMM_ERR_TYPE;
#endif
		}

		return MPC_LOWCOMM_SUCCESS;
	}

	if(!__did_process)
	{
		mpc_common_debug_error("source = %d dest = %d tag = %d\n", world_source, world_destination, tag);
		not_reachable();
	}

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_iprobe(int source, int tag, mpc_lowcomm_communicator_t comm, int *flag, mpc_lowcomm_status_t *status)
{
	/* Translate ranks */

	if(source != MPC_ANY_SOURCE)
	{
		if(mpc_lowcomm_communicator_is_intercomm(comm) )
		{
			source = mpc_lowcomm_communicator_remote_world_rank(comm, source);
		}
		else
		{
			source = mpc_lowcomm_communicator_world_rank_of(comm, source);
		}
	}
	else
	{
		source = MPC_ANY_SOURCE;
	}

	return mpc_lowcomm_iprobe_src_dest(source, mpc_common_get_task_rank(), tag, comm, flag, status);
}

typedef struct
{
	int                        flag;
	int                        source;
	int                        destination;
	int                        tag;
	mpc_lowcomm_communicator_t comm;
	mpc_lowcomm_status_t *     status;
} __mpc_probe_t;

static void __mpc_probe_poll(__mpc_probe_t *arg)
{
	mpc_lowcomm_iprobe_src_dest(arg->source, arg->destination, arg->tag, arg->comm, &(arg->flag), arg->status);
}

int mpc_lowcomm_probe(int source, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_status_t *status)
{
	__mpc_probe_t probe_struct;

	if(source != MPC_ANY_SOURCE)
	{
		if(mpc_lowcomm_communicator_is_intercomm(comm) )
		{
			probe_struct.source = mpc_lowcomm_communicator_remote_world_rank(comm, source);
		}
		else
		{
			probe_struct.source = mpc_lowcomm_communicator_world_rank_of(comm, source);
		}
	}
	else
	{
		probe_struct.source = MPC_ANY_SOURCE;
	}

	probe_struct.destination = mpc_common_get_task_rank();
	probe_struct.tag         = tag;
	probe_struct.comm        = comm;
	probe_struct.status      = status;
	mpc_lowcomm_iprobe_src_dest(probe_struct.source, probe_struct.destination, tag, comm,
	                 &probe_struct.flag, status);

	if(probe_struct.flag != 1)
	{
		mpc_lowcomm_perform_idle(
		        &probe_struct.flag, 1, (void (*)(void *) )__mpc_probe_poll, &probe_struct);
	}

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_get_req_source(mpc_lowcomm_request_t *req)
{
	return mpc_lowcomm_peer_get_rank(req->header.source);
}

size_t mpc_lowcomm_get_req_size(mpc_lowcomm_request_t *req)
{
   return req->header.msg_size;
}

/**********************************
 * UNIVERSE COMMUNICATION SUPPORT *
 **********************************/

int mpc_lowcomm_universe_isend(mpc_lowcomm_peer_uid_t dest,
							   const void *data,
							   size_t size,
							   int tag,
							   mpc_lowcomm_communicator_t remote_world,
							   mpc_lowcomm_request_t *req)
{

	if(mpc_lowcomm_peer_get_rank(dest) == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(req, MPC_COMM_WORLD, REQUEST_SEND);
		return MPC_LOWCOMM_SUCCESS;
	}

	if(!mpc_lowcomm_monitor_peer_exists(dest))
	{
		mpc_common_debug_warning("%s : peer %s does not appear to exist",
								__FUNCTION__,
								mpc_lowcomm_peer_format(dest));
		return MPC_LOWCOMM_ERROR;
	}

	mpc_lowcomm_ptp_message_t *msg = mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);
	mpc_lowcomm_ptp_message_set_contiguous_addr(msg, data, size);


	mpc_lowcomm_ptp_message_header_init(msg,
										tag,
										remote_world,
										mpc_lowcomm_monitor_get_uid(),
										dest,
										req,
										size,
										MPC_LOWCOMM_MESSAGE_UNIVERSE,
	                                    MPC_DATATYPE_IGNORE,
										REQUEST_SEND);

	//mpc_common_debug_error("MSG TO comm %llu (LOCAL %llu)", SCTK_MSG_COMMUNICATOR_ID(msg), mpc_lowcomm_get_comm_world_id());

	mpc_lowcomm_ptp_message_send(msg);

	return MPC_LOWCOMM_SUCCESS;
}


int mpc_lowcomm_universe_irecv(mpc_lowcomm_peer_uid_t src,
							   const void *data,
							   size_t size,
							   int tag,
							   mpc_lowcomm_request_t *req)
{
	if(mpc_lowcomm_peer_get_rank(src) == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(req, MPC_COMM_WORLD, REQUEST_SEND);
		return MPC_LOWCOMM_SUCCESS;
	}

	if(mpc_lowcomm_peer_get_rank(src) != MPC_ANY_SOURCE)
	{
		if(!mpc_lowcomm_monitor_peer_exists(src))
		{
			mpc_common_debug_warning("%s : peer %s does not appear to exist",
									__FUNCTION__,
									mpc_lowcomm_peer_format(src));
			return MPC_LOWCOMM_ERROR;
		}
	}

	mpc_lowcomm_ptp_message_t *msg = mpc_lowcomm_ptp_message_header_create(MPC_LOWCOMM_MESSAGE_CONTIGUOUS);
	mpc_lowcomm_ptp_message_set_contiguous_addr(msg, data, size);


	mpc_lowcomm_ptp_message_header_init(msg,
										tag,
										MPC_COMM_WORLD,
										src,
										mpc_lowcomm_monitor_get_uid(),
										req,
										size,
										MPC_LOWCOMM_MESSAGE_UNIVERSE,
	                                    MPC_DATATYPE_IGNORE,
										REQUEST_RECV);

	mpc_lowcomm_ptp_message_recv(msg);

	return MPC_LOWCOMM_SUCCESS;
}

/*******************
 * PORT MANAGEMENT *
 *******************/

int mpc_lowcomm_open_port(char * port_name, int port_name_len)
{
	return mpc_lowcomm_monitor_open_port(port_name, port_name_len);
}

int mpc_lowcomm_close_port(const char * port_name)
{
	return mpc_lowcomm_monitor_close_port(port_name);
}

/*********************
 * Name Publishing   *
 *********************/

static inline int __publish_op(mpc_lowcomm_monitor_command_naming_t cmd,
							   const char *service_name,
							   const char *port_name)
{
	/* Publishing is setting a value in the root rank of my process set */
	mpc_lowcomm_monitor_retcode_t mret;

	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_naming(mpc_lowcomm_monitor_uid_of(  mpc_lowcomm_monitor_get_gid() , 0),
																	cmd,
																	mpc_lowcomm_monitor_get_uid(),
																    service_name,
																    port_name,
																    &mret);

	mpc_lowcomm_monitor_args_t * content = mpc_lowcomm_monitor_response_get_content(resp);

	int err = 0;


	if((content->naming.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS) ||
	   (mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS) )
	{
		err = 1;
	}

	mpc_lowcomm_monitor_response_free(resp);

	return (err?MPC_LOWCOMM_ERROR:MPC_LOWCOMM_SUCCESS);
}


int mpc_lowcomm_publish_name(const char *service_name,
                             const char *port_name)
{
	return __publish_op(MPC_LOWCOMM_MONITOR_NAMING_PUT, service_name, port_name);
}

int mpc_lowcomm_unpublish_name(const char *service_name,
                               __UNUSED__ const char *port_name)
{
	return __publish_op(MPC_LOWCOMM_MONITOR_NAMING_DEL, service_name, "");
}

int mpc_lowcomm_lookup_name(const char *service_name,
                            char *port_name,
                            int port_name_len)
{
	int roots_count = 0;
	mpc_lowcomm_peer_uid_t *set_roots = _mpc_lowcomm_get_set_roots(&roots_count);

	int i;

	for(i = 0 ; i < roots_count ; i++)
	{
		/* Now try to resolve starting from ourselves */
		mpc_lowcomm_monitor_retcode_t mret;

		mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_naming(set_roots[i],
																		 MPC_LOWCOMM_MONITOR_NAMING_GET,
																		 mpc_lowcomm_monitor_get_uid(),
																		 service_name,
																		 "",
																		&mret);

		mpc_lowcomm_monitor_args_t * content = mpc_lowcomm_monitor_response_get_content(resp);

		int err = 0;


		if((content->naming.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS) ||
		(mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS) )
		{
			err = 1;
		}

		if(!err)
		{
			snprintf(port_name, port_name_len, "%s", content->naming.port_name);
		}

		mpc_lowcomm_monitor_response_free(resp);

		if(!err)
		{
			break;
		}
	}

	_mpc_lowcomm_free_set_roots(set_roots);

	return MPC_LOWCOMM_SUCCESS;
}

/********************
 * SETUP & TEARDOWN *
 ********************/

void mpc_launch_init_runtime();

void mpc_lowcomm_init()
{
#ifndef MPC_Threads
	mpc_launch_init_runtime();
	mpc_common_init_trigger("VP Thread Start");
#endif
	mpc_lowcomm_barrier(MPC_COMM_WORLD);
}

void mpc_lowcomm_release()
{
	mpc_lowcomm_barrier(MPC_COMM_WORLD);
#ifndef MPC_Threads
	mpc_common_init_trigger("VP Thread End");
	mpc_common_init_trigger("After Ending VPs");
	mpc_launch_release_runtime();
#endif
}

/* Old interface for retro-compat */

void mpc_lowcomm_libmode_init()
{
	mpc_lowcomm_init();
}

void mpc_lowcomm_libmode_release()
{
	mpc_lowcomm_release();
}


static void __lowcomm_init_per_task()
{
	int task_rank = mpc_common_get_task_rank();

	/* We call for all threads as some
	 * progress threads may need buffered headers */
	if(task_rank >= 0)
	{
		mpc_lowcomm_terminaison_barrier();

		mpc_lowcomm_init_per_task(task_rank);

        _mpc_lowcomm_monitor_setup_per_task();

		mpc_lowcomm_terminaison_barrier();

		_mpc_lowcomm_pset_bootstrap();
	}
}

static void __lowcomm_release_per_task()
{
	int task_rank = mpc_common_get_task_rank();

	if(task_rank >= 0)
	{
		mpc_common_nodebug("mpc_lowcomm_terminaison_barrier");
		mpc_lowcomm_terminaison_barrier();
		_mpc_lowcomm_release_psets();
		mpc_lowcomm_release_per_task(task_rank);
		mpc_common_nodebug("mpc_lowcomm_terminaison_barrier done");
		_mpc_lowcomm_monitor_teardown_per_task();
	}
	else
	{
		not_reachable();
	}
}

static void __initialize_drivers()
{
	_mpc_lowcomm_monitor_setup();

	if(mpc_common_get_process_count() > 1)
	{
		sctk_net_init_driver(mpc_common_get_flags()->network_driver_name);
	}

	__init_request_null();

 	mpc_lowcomm_rdma_window_init_ht();

	_mpc_lowcomm_communicator_init();
}

static void __finalize_driver()
{
	_mpc_lowcomm_monitor_teardown();
	mpc_lowcomm_rdma_window_release_ht();
	_mpc_lowcomm_communicator_release();
}

#ifdef MPC_USE_DMTCP
static void __initialize_ft(void)
{
	sctk_ft_init();
}
#endif


void mpc_lowcomm_registration() __attribute__( (constructor) );

void mpc_lowcomm_registration()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("Base Runtime Init Done", "LowComm Init", __initialize_drivers, 16);

	mpc_common_init_callback_register("Base Runtime Finalize", "Release LowComm", __finalize_driver, 16);

	mpc_common_init_callback_register("Base Runtime Init Done", "Block sigpipe", mpc_common_helper_ignore_sigpipe, 0);

	mpc_common_init_callback_register("VP Thread Start", "MPC Message Passing Init per Task", __lowcomm_init_per_task, 0);

	mpc_common_init_callback_register("VP Thread End", "MPC Message Passing Release", __lowcomm_release_per_task, 0);

	mpc_common_init_callback_register("Config Sources", "MPC Lowcomm Config Registration", _mpc_lowcomm_config_register, 128);

	mpc_common_init_callback_register("Config Checks", "MPC Lowcomm Config Validation", _mpc_lowcomm_config_validate, 128);
#ifdef MPC_USE_DMTCP
	mpc_common_init_callback_register("Base Runtime Init with Config", "Initialize Fault-Tolerance", __initialize_ft, 77);
#endif
}
