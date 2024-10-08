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
#include "lowcomm.h"
#include <mpc_common_helper.h>
#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <mpc_common_asm.h>
#include <checksum.h>
#include <reorder.h>
#include "mpc_keywords.h"
#include "rail.h"
#include <mpc_launch.h>
#include <mpc_common_profiler.h>
#include <sctk_alloc.h>
#include <coll.h>
#include <mpc_common_flags.h>
#include <mpc_topology.h>

#include "alloc_mem.h"

#include "communicator.h"
#include "datatypes_common.h"

#include "mpc_lowcomm_types.h"
#include <mpc_lowcomm_datatypes.h>
#include <mpc_common_progress.h>

#include <lcp.h>

#include "mpc_lowcomm_workshare.h"
#include "lowcomm_config.h"
#include "monitor.h"
#include "pset.h"

/* Forward declaration. */
static int mpc_comm_progress();

/********************************************************************/
/*Structures                                                        */
/********************************************************************/

typedef struct
{
	mpc_lowcomm_communicator_id_t comm_id;
	int                           rank;
} mpc_comm_dest_key_t;

#if 0
static inline void _mpc_comm_dest_key_init(mpc_comm_dest_key_t *key, mpc_lowcomm_communicator_id_t comm, int rank)
{
	key->rank    = rank;
	key->comm_id = comm;
}

typedef struct mpc_comm_ptp_s
{
	mpc_comm_dest_key_t             key;

	mpc_lowcomm_ptp_message_lists_t lists;


	UT_hash_handle                  hh;

	/* Reorder table */
	_mpc_lowcomm_reorder_list_t     reorder;
} mpc_comm_ptp_t;

static inline void __mpc_comm_request_init(mpc_lowcomm_request_t *request,
                                           mpc_lowcomm_communicator_t comm,
                                           int request_type)
{
	if(request != NULL)
	{
		request->comm                      = comm;
		request->completion_flag           = MPC_LOWCOMM_MESSAGE_DONE;
		request->header.source             = mpc_lowcomm_monitor_local_uid_of(MPC_PROC_NULL);
		request->header.destination        = mpc_lowcomm_monitor_local_uid_of(MPC_PROC_NULL);
		request->header.source_task        = MPC_PROC_NULL;
		request->header.destination_task   = MPC_PROC_NULL;
		request->header.message_tag        = MPC_ANY_TAG;
		request->header.communicator_id    = _mpc_lowcomm_communicator_id(comm);
		request->truncated                 = 0;
		request->request_type              = request_type;
		request->pointer_to_shadow_request = NULL;
		request->pointer_to_source_request = NULL;
		request->msg = NULL;
		TODO("Setting it breaks add_pack there is a mismatch in initialization");
		//request->dt_magic = 0;
	}
}
#endif

/********************************************************************/
/*Functions                                                         */
/********************************************************************/

#define CHECK_STATUS_ERROR(status)    do{                       \
		if( (status)->MPC_ERROR != MPC_LOWCOMM_SUCCESS) \
		{                                               \
			return (status)->MPC_ERROR;             \
		}                                               \
}                                                               \
	while(0)

int mpc_lowcomm_check_type_compat(mpc_lowcomm_datatype_t src, mpc_lowcomm_datatype_t dest)
{
	if(src != dest)
	{
		if( (src != MPC_DATATYPE_IGNORE) && (dest != MPC_DATATYPE_IGNORE) )
		{
			/* See page 33 of 3.0 PACKED and BYTE are exceptions */
			if( (src != MPC_LOWCOMM_PACKED) && (dest != MPC_LOWCOMM_PACKED) )
			{
				if( (src != MPC_LOWCOMM_BYTE) && (dest != MPC_LOWCOMM_BYTE) )
				{
					if(mpc_lowcomm_datatype_is_common(src) &&
					   mpc_lowcomm_datatype_is_common(dest) )
					{
						return MPC_LOWCOMM_ERR_TYPE;
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

		/* You must provide a valid status to the query function */
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
		status->MPC_SOURCE = request->recv_info.src;
		status->MPC_TAG    = request->recv_info.tag;
		status->MPC_ERROR  = request->status_error;
		status->size       = (int)request->recv_info.length;

		if(request->completion_flag == MPC_LOWCOMM_MESSAGE_CANCELED)
		{
			status->cancelled = 1;
		}
		else
		{
			status->cancelled = 0;
		}
	}

	return MPC_LOWCOMM_SUCCESS;
}

#if 0
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

	if(flush)
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

	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	_mpc_comm_dest_key_init(&key, comm->id, rank);
	return __mpc_comm_ptp_array_get_key(&key);
}
#endif

int _mpc_lowcomm_comm_get_lists(int rank, mpc_lowcomm_ptp_message_lists_t **lists, int *list_count)
{
        UNUSED(rank);
        UNUSED(lists);
        UNUSED(list_count);
        not_reachable();
#if 0
	int i;
	int ret_cnt = 0;

	for(i = 0 ; (i < SCTK_PARALLEL_COMM_QUEUES_NUMBER) && (i < *list_count); i++)
	{
		mpc_comm_dest_key_t key;
		key.comm_id = i;
		key.rank    = rank;

		struct mpc_comm_ptp_s *ptp = __mpc_comm_ptp_array_get_key(&key);

		if(ptp)
		{
			(lists)[ret_cnt] = &ptp->lists;
			ret_cnt++;
		}
	}

	*list_count = ret_cnt;



	if(mpc_common_get_local_process_count() <= *list_count)
	{
		return MPC_LOWCOMM_ERR_TRUNCATE;
	}

#endif
	return MPC_LOWCOMM_SUCCESS;
}

#if 0
struct mpc_comm_ptp_s *_mpc_comm_ptp_array_get(mpc_lowcomm_communicator_t comm, int rank)
{
	return __mpc_comm_ptp_array_get(comm, rank);
}
#endif

_mpc_lowcomm_reorder_list_t *_mpc_comm_ptp_array_get_reorder(mpc_lowcomm_communicator_t communicator, int rank)
{
        UNUSED(communicator);
        UNUSED(rank);
#if 0
	struct mpc_comm_ptp_s *internal_ptp = __mpc_comm_ptp_array_get(communicator, rank);

	assert(internal_ptp);
	return &(internal_ptp->reorder);
#endif
        return NULL;
}


void _mpc_comm_ptp_message_reinit_comm(mpc_lowcomm_ptp_message_t *msg)
{
        UNUSED(msg);
#if 0
	assert(msg != NULL);
	mpc_lowcomm_communicator_id_t comm_id = SCTK_MSG_COMMUNICATOR_ID(msg);

	mpc_lowcomm_communicator_t comm = mpc_lowcomm_get_communicator_from_id(comm_id);

	if(!comm)
	{
		mpc_common_debug_error("No such local comm %llu", comm_id);
		assume(comm != NULL);
	}


	SCTK_MSG_COMMUNICATOR_SET(msg, comm);
#endif
}

#if 0
/*
 * Insert a new entry to the PTP table. The function checks if the entry is
 * already present
 * and fail in this case
 *
 * FIXME: This function is currently unused: the corresponding call in
 * mpc_lowcomm_init_per_task was commented out in dc52abdf8b.
 */
__UNUSED__ static inline void __mpc_comm_ptp_array_insert(mpc_comm_ptp_t *tmp)
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
		memset(sctk_ptp_array, 0, (size_t)(sctk_ptp_array_end - sctk_ptp_array_start + 1) * sizeof(mpc_comm_ptp_t * *) );
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
#endif

/* This is the exported version */
int mpc_lowcomm_is_remote_rank(int dest)
{
        UNUSED(dest);
        not_reachable();
#if 0
	return _mpc_comm_is_remote_rank(dest);
#endif
        return 0;
}

#if 0
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
	while(1)
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
		else
		{
			break;
		}
	}

	if(!nb_messages_copied && (depth < 2) )
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
#endif

/********************************************************************/
/*copy engine                                                       */
/********************************************************************/

/*
 * Mark a message received as DONE and call the 'free' function associated
 * to the message
 */
int ___mpi_windows_in_use = 0;

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
        UNUSED(msg);
#if 0
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
		if(req->request_completion_fn)
		{
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
#endif
}

void _mpc_comm_ptp_message_commit_request(mpc_lowcomm_ptp_message_t *send,
                                          mpc_lowcomm_ptp_message_t *recv)
{
        UNUSED(send);
        UNUSED(recv);
#if 0
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
		if(0 <= SCTK_MSG_SRC_TASK(send) )
		{
			recv->tail.request->header.source_task = mpc_lowcomm_communicator_rank_of_as(SCTK_MSG_COMMUNICATOR(send), SCTK_MSG_SRC_TASK(send), SCTK_MSG_SRC_TASK(send), SCTK_MSG_SRC_PROCESS_UID(send) );
			//assume(recv->tail.request->header.source_task  != MPC_PROC_NULL);
		}
		else
		{
			/* This is a ctrl msg do not attempt to resolve */
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
		if(send->tail.request->status_error != MPC_LOWCOMM_SUCCESS && recv->tail.request)
		{
			recv->tail.request->status_error = send->tail.request->status_error;
		}

		send->tail.request->msg = NULL;
	}

#ifdef SCTK_USE_CHECKSUM
	/* Verify the checksum of the received message */
	_mpc_lowcomm_checksum_verify(send, recv);
#endif
	/* Complete messages: mark messages as done and mark them as DONE */
	mpc_lowcomm_ptp_message_complete_and_free(send);
	mpc_lowcomm_ptp_message_complete_and_free(recv);
#endif
}

#if 0
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
						                 (long)recv->tail.message.pack.list.absolute[i].elem_size);
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
						size = (size_t)( (recv->tail.message.pack.list.absolute[i].ends[j] -
						                  recv->tail.message.pack.list.absolute[i].begins[j] + 1) *
						                 (long)recv->tail.message.pack.list.absolute[i].elem_size);

						if(total + size > send->tail.message.contiguous.size)
						{
							skip = 1;
							size = send->tail.message.contiguous.size - total;
						}

						memcpy( (char *)(recv->tail.message.pack.list.absolute[i].addr) +
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
		memcpy( (void *)out_adress, in_adress, in_sizes);
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
		memcpy( (void *)out_adress, in_adress, in_sizes);
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

	//__mpc_comm_ptp_task_init();
	//__mpc_comm_buffered_ptp_init();

	_mpc_lowcomm_datatype_init_common();

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
		//__mpc_comm_ptp_array_insert(tmp);
	}
}

void mpc_lowcomm_release_per_task(int task_rank)
{
	mpc_lowcomm_allocmem_pool_release();

	sctk_net_finalize_task_level(task_rank, mpc_topology_get_current_cpu() );
}
#endif

/********************************************************************/
/*Message creation                                                  */
/********************************************************************/
void __mpc_comm_ptp_message_pack_free(void *);

void mpc_lowcomm_ptp_message_header_clear(mpc_lowcomm_ptp_message_t *tmp,
                                          mpc_lowcomm_ptp_message_type_t msg_type, void (*free_memory)(void *),
                                          void (*message_copy)(mpc_lowcomm_ptp_message_content_to_copy_t *) )
{
        UNUSED(tmp);
        UNUSED(msg_type);
        UNUSED(free_memory);
        UNUSED(message_copy);
        not_reachable();
#if 0
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
#endif
}

__attribute__((deprecated("Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
mpc_lowcomm_ptp_message_t *mpc_lowcomm_ptp_message_header_create(mpc_lowcomm_ptp_message_type_t msg_type)
{
        UNUSED(msg_type);
        not_reachable();
#if 0
	mpc_lowcomm_ptp_message_t *tmp;

	tmp = __mpc_comm_alloc_header();
	/* Store if the buffer has been buffered */
	const char from_buffered = tmp->from_buffered;

	/* The copy and free functions will be set after */
	mpc_lowcomm_ptp_message_header_clear(tmp, msg_type, __mpc_comm_free_header, mpc_lowcomm_ptp_message_copy);
	/* Restore it */
	tmp->from_buffered = from_buffered;
	return tmp;
#endif
        return NULL;
}

__attribute__((deprecated("Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
void mpc_lowcomm_ptp_message_set_contiguous_addr(mpc_lowcomm_ptp_message_t *restrict msg,
                                                 const void *restrict addr, const size_t size)
{
        UNUSED(msg);
        UNUSED(addr);
        UNUSED(size);
        not_reachable();
#if 0
	assert(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS);
	msg->tail.message.contiguous.size = size;
	msg->tail.message.contiguous.addr = (void *)addr;
#endif
}

#if 0
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
#endif

__attribute__((deprecated("Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
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
        UNUSED(msg);
        UNUSED(message_tag);
        UNUSED(communicator);
        UNUSED(source);
        UNUSED(destination);
        UNUSED(request);
        UNUSED(count);
        UNUSED(message_class);
        UNUSED(datatype);
        UNUSED(request_type);
#if 0
	/* This function can fill the header for both process-based messages
	 * and regular MPI messages this is the reason why destination and source
	 * are UIDs. In the case of regular messages this is downcasted to an int */

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
		SCTK_MSG_DEST_TASK_SET(msg, _mpc_lowcomm_communicator_world_first_local_task() );
	}
	else
	{
		int is_intercomm = mpc_lowcomm_communicator_is_intercomm(communicator);

		mpc_lowcomm_peer_uid_t source_process = 0;
		mpc_lowcomm_peer_uid_t dest_process   = 0;

		int source_task = -1;
		int dest_task   = -1;
		/* Fill in Source and Dest Process Information (convert from task) */

		/* SOURCE */
		int isource = mpc_lowcomm_peer_get_rank(source);

		if(mpc_lowcomm_peer_get_rank(source) != MPC_ANY_SOURCE)
		{
			if(is_intercomm)
			{
				if(request_type == REQUEST_RECV)
				{
					/* If this is a RECV make sure the translation is done on the source according to remote */
					source_task    = mpc_lowcomm_communicator_remote_world_rank(communicator, isource);
					source_process = mpc_lowcomm_communicator_remote_uid(communicator, isource);
				}
				else if(request_type == REQUEST_SEND)
				{
					/* If this is a SEND make sure the translation is done on the dest according to remote */
					source_task    = mpc_lowcomm_communicator_world_rank_of(communicator, isource);
					source_process = mpc_lowcomm_communicator_uid(communicator, isource);
				}
			}
			else
			{
				source_task    = mpc_lowcomm_communicator_world_rank_of(communicator, isource);
				source_process = mpc_lowcomm_communicator_uid(communicator, isource);
			}
		}
		else
		{
			source_task    = MPC_ANY_SOURCE;
			source_process = mpc_lowcomm_monitor_local_uid_of(MPC_ANY_SOURCE);
		}

		/* DEST Handling */
		int idestination = mpc_lowcomm_peer_get_rank(destination);

		if(is_intercomm)
		{
			if(request_type == REQUEST_RECV)
			{
				/* If this is a RECV make sure the translation is done on the source according to remote */
				dest_task    = mpc_lowcomm_communicator_world_rank_of(communicator, idestination);
				dest_process = mpc_lowcomm_communicator_uid(communicator, idestination);
			}
			else if(request_type == REQUEST_SEND)
			{
				/* If this is a SEND make sure the translation is done on the dest according to remote */
				dest_task    = mpc_lowcomm_communicator_remote_world_rank(communicator, idestination);
				dest_process = mpc_lowcomm_communicator_remote_uid(communicator, idestination);
			}
		}
		else
		{
			dest_task    = mpc_lowcomm_communicator_world_rank_of(communicator, idestination);
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
			(mpc_lowcomm_peer_get_set(SCTK_MSG_DEST_PROCESS_UID(msg) ) != mpc_lowcomm_monitor_get_uid() ) ||
			/* or does overflows task count */
			( ((unsigned int) source_task < mpc_common_get_task_count() ) && ((unsigned int) dest_task < mpc_common_get_task_count() ) )
			);


		assert(
			/* Is in another set set */
			(mpc_lowcomm_peer_get_set(SCTK_MSG_DEST_PROCESS_UID(msg) ) != mpc_lowcomm_monitor_get_uid() ) ||
			/* or does overflows process count */
			( (SCTK_MSG_DEST_PROCESS(msg) < mpc_common_get_process_count() ) && (SCTK_MSG_SRC_PROCESS(msg) < mpc_common_get_process_count() ) )
			);

		/* Avoid buffer overflow in mpc_lowcomm_ptp_message_class_name */
		assert(message_class < MPC_LOWCOMM_MESSAGE_CLASS_COUNT);

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
#endif
}

/********************************************************************/
/*Message pack creation                                             */
/********************************************************************/

#if 0
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
#endif

void mpc_lowcomm_request_pack(void *request, void *buffer)
{
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

	switch(req->dt_type)
	{
		case MPC_LOWCOMM_MESSAGE_PACK:
		{
			size_t i;
			size_t j;
			size_t size;
			size = req->header.msg_size;

			if(size > 0)
			{
				for( i = 0; i < req->dt.count; i++ )
				{
					for( j = 0; j < req->dt.list.std[i].count; j++ )
					{
						size = (req->dt.list.std[i].ends[j] -
						        req->dt.list.std[i].begins[j] +
						        1) * req->dt.list.std[i].elem_size;
						memcpy(buffer, ( ( char * )(req->dt.list.std[i].addr) ) +
						       req->dt.list.std[i].begins[j] *
						       req->dt.list.std[i].elem_size, size);
						buffer += size;
					}
				}
			}

			break;
		}

		case MPC_LOWCOMM_MESSAGE_PACK_ABSOLUTE:
		{
			size_t i;
			size_t j;
			size_t size;
			size = req->header.msg_size;

			if(size > 0)
			{
				for( i = 0; i < req->dt.count; i++ )
				{
					for( j = 0; j < req->dt.list.absolute[i].count; j++ )
					{
						size = (req->dt.list.absolute[i].ends[j] -
						        req->dt.list.absolute[i].begins[j] +
						        1) * req->dt.list.absolute[i].elem_size;
						memcpy(buffer, ( ( char * )(req->dt.list.absolute[i].addr) ) +
						       req->dt.list.absolute[i].begins[j] *
						       req->dt.list.absolute[i].elem_size, size);
						buffer += size;
					}
				}
			}

			break;
		}

		default:
			not_reachable();
	}
}

void mpc_lowcomm_request_unpack(void *request, void *buffer)
{
	size_t i, j;
	size_t size;
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

	switch(req->dt_type)
	{
		case MPC_LOWCOMM_REQUEST_PACK:
			size = req->header.msg_size;
			if(size > 0)
			{
				size_t total = 0;
				char   skip  = 0;

				for(i = 0; i < req->dt.count; i++)
				{
					for(j = 0; (j < req->dt.list.std[i].count) && !skip; j++)
					{
						size = (req->dt.list.std[i].ends[j] -
						        req->dt.list.std[i].begins[j] +
						        1) * req->dt.list.std[i].elem_size;

						mpc_common_nodebug("%p - %p \n", req->dt.list.std[i].begins[j],
						                   req->dt.list.std[i].ends[j]);
						if(total + size > req->header.msg_size)
						{
							skip = 1;
							size = req->header.msg_size - total;
						}

						memcpy( ( (char *)(req->dt.list.std[i].addr) ) +
						        req->dt.list.std[i].begins[j] *
						        req->dt.list.std[i].elem_size, buffer, size);
						buffer += size;
						total  += size;
						assume(total <= req->header.msg_size);
					}
				}
			}
			break;

		case MPC_LOWCOMM_REQUEST_PACK_ABSOLUTE:
			size = req->header.msg_size;
			if(size > 0)
			{
				size_t total = 0;
				char   skip  = 0;

				for(i = 0; i < req->dt.count; i++)
				{
					for(j = 0; (j < req->dt.list.absolute[i].count) && !skip; j++)
					{
						size = (req->dt.list.absolute[i].ends[j] -
						        req->dt.list.absolute[i].begins[j] +
						        1) * req->dt.list.absolute[i].elem_size;

						mpc_common_nodebug("%p - %p \n", req->dt.list.std[i].begins[j],
						                   req->dt.list.std[i].ends[j]);
						if(total + size > req->header.msg_size)
						{
							skip = 1;
							size = req->header.msg_size - total;
						}

						memcpy( ( ( char * )(req->dt.list.absolute[i].addr) ) +
						        req->dt.list.absolute[i].begins[j] *
						        req->dt.list.absolute[i].elem_size, buffer, size);
						buffer += size;
						total  += size;
						assume(total <= req->header.msg_size);
					}
				}
			}
			break;

		default:
			not_reachable();
	}
}

#define SCTK_PACK_REALLOC_STEP    10
void mpc_lowcomm_request_add_pack(mpc_lowcomm_request_t *req, void *adr,
                                  const unsigned int nb_items,
                                  const size_t elem_size,
                                  unsigned long *begins,
                                  unsigned long *ends)
{
	size_t step;

	req->dt_type  = MPC_LOWCOMM_REQUEST_PACK;
	req->dt_magic = 0xDDDDDDDD;
	if(req->dt.count >= req->dt.max_count)
	{
		req->dt.max_count += SCTK_PACK_REALLOC_STEP;
		req->dt.list.std   =
			sctk_realloc(req->dt.list.std,
			             req->dt.max_count *
			             sizeof(mpc_lowcomm_request_pack_std_list_t) );
	}

	step = req->dt.count;
	req->dt.list.std[step].count     = nb_items;
	req->dt.list.std[step].begins    = begins;
	req->dt.list.std[step].ends      = ends;
	req->dt.list.std[step].addr      = adr;
	req->dt.list.std[step].elem_size = elem_size;
	req->dt.count++;
}

void mpc_lowcomm_request_add_pack_absolute(mpc_lowcomm_request_t *req,
                                           const void *adr, const unsigned int nb_items,
                                           const size_t elem_size,
                                           long *begins,
                                           long *ends)
{
	size_t step;

	req->dt_type  = MPC_LOWCOMM_REQUEST_PACK_ABSOLUTE;
	req->dt_magic = 0xDDDDDDDD;
	if(req->dt.count >= req->dt.max_count)
	{
		req->dt.max_count    += SCTK_PACK_REALLOC_STEP;
		req->dt.list.absolute =
			sctk_realloc(req->dt.list.absolute,
			             req->dt.max_count *
			             sizeof(mpc_lowcomm_request_pack_absolute_list_t) );
	}

	step = req->dt.count;
	req->dt.list.absolute[step].count     = nb_items;
	req->dt.list.absolute[step].begins    = begins;
	req->dt.list.absolute[step].ends      = ends;
	req->dt.list.absolute[step].addr      = adr;
	req->dt.list.absolute[step].elem_size = elem_size;
	req->dt.count++;
}

#if 0
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
#endif

/********************************************************************/
/* Searching functions                                              */
/********************************************************************/

static __thread int       did_mprobe = 0;
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

#if 0
static inline mpc_lowcomm_msg_list_t *__mpc_comm_pending_msg_list_search_matching(mpc_lowcomm_ptp_list_pending_t *pending_list,
                                                                                  mpc_lowcomm_ptp_message_t *msg)
{
	mpc_lowcomm_ptp_message_header_t *header = &(msg->body.header);

	mpc_lowcomm_msg_list_t *ptr_found = NULL;
	mpc_lowcomm_msg_list_t *tmp       = NULL;

	/* Loop on all  pending messages */
	DL_FOREACH_SAFE(pending_list->list, ptr_found, tmp)
	{
		mpc_lowcomm_ptp_message_header_t *header_found;

		assert(ptr_found->msg != NULL);
		header_found = &(ptr_found->msg->body.header);

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
		                 mpc_lowcomm_ptp_message_class_name[(int)header->message_type],
		                 mpc_lowcomm_ptp_message_class_name[(int)header_found->message_type]);
#endif

		if(  /* Match Communicator */
			(header->communicator_id == header_found->communicator_id) &&
			/* Match message type */
			(header->message_type == header_found->message_type) &&
			/* Match source Process */
			( (header->source == header_found->source) || (mpc_lowcomm_peer_get_rank(header->source) == MPC_ANY_SOURCE) ) &&
			/* Match source task appart for process specific messages which are not matched at task level */
			( (header->source_task == header_found->source_task) ||
			  (header->source_task == MPC_ANY_SOURCE) ||
			  _mpc_comm_ptp_message_is_for_process(header->message_type) ) &&
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
		if(header_found->message_type == MPC_LOWCOMM_CANCELLED_SEND)
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
			(_mpc_comm_ptp_message_is_for_process(header->message_type) == 0) &&
			/* Has either no or the same matching ID */
			( (send_message_matching_id == -1) ||
			  (send_message_matching_id == OPA_load_int(&tail_send->matching_id) ) ) &&
			/* Match Communicator */
			( (header->communicator_id == header_send->communicator_id) ||
			  (header->communicator_id == MPC_ANY_COMM) ) &&
			/* Match message-type */
			(header->message_type == header_send->message_type) &&

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
	mpc_lowcomm_msg_list_t *ptr_send = __mpc_comm_pending_msg_list_search_matching(&pair->lists.pending_send, msg);

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
		if(__mpc_comm_ptp_message_list_trylock_pending(&(pair->lists) ) == 0)
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
#endif

void mpc_lowcomm_ptp_msg_wait_init(struct mpc_lowcomm_ptp_msg_progress_s *wait,
                                   mpc_lowcomm_request_t *request, int blocking)
{
        UNUSED(wait);
        UNUSED(request);
        UNUSED(blocking);
        not_reachable();
#if 0
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
#endif
}

//NOTE: cannot be static since needed in mpi_partitioned.c
lcp_context_h lcp_ctx_loc = NULL;
struct mpi_request_info {
        size_t size;
        void (*request_init_func)(void *request);
};
struct mpi_request_info mpi_req_info = {0};

lcp_manager_h lcp_mngr_loc = NULL;
int mpc_lowcomm_request_complete(mpc_lowcomm_request_t *request);

#if 0
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
		lcp_progress(lcp_mngr_loc);
	}

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
#endif

//static inline void __mpc_comm_ptp_msg_wait(struct mpc_lowcomm_ptp_msg_progress_s *wait);

static void __mpc_comm_perform_req_wfv(void *a)
{
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)a;

	if( ( volatile int )req->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
	{
		mpc_common_progress();
		MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}
}

#if 0
static void __mpc_comm_perform_msg_wfv(void *a)
{
	struct mpc_lowcomm_ptp_msg_progress_s *_wait = ( struct mpc_lowcomm_ptp_msg_progress_s * )a;

	__mpc_comm_ptp_msg_wait(_wait);

	if( ( volatile int )_wait->request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
	{
		lcp_progress(lcp_mngr_loc);
		MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	}
}
#endif

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
	if(!request)
	{
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
	if(__request_is_null_or_cancelled(request) )
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
		int trials = 0;
		do
		{
                        mpc_common_progress();
			trials++;
		} while( (request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE) && (trials < 16) );

		if(request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
		{
			mpc_lowcomm_perform_idle(
				( int * )&(request->completion_flag), MPC_LOWCOMM_MESSAGE_DONE,
				(void (*)(void *) ) __mpc_comm_perform_req_wfv,
				request);
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

#if 0
static inline void __mpc_comm_ptp_msg_wait(struct mpc_lowcomm_ptp_msg_progress_s *wait)
{
	const mpc_lowcomm_request_t *request        = wait->request;
	mpc_comm_ptp_t *recv_ptp                    = wait->recv_ptp;
	mpc_comm_ptp_t *send_ptp                    = wait->send_ptp;

	if(request->completion_flag != MPC_LOWCOMM_MESSAGE_DONE)
	{
		/* Check the source of the request. We try to poll the
		 *         source in order to retrieve messages from the network */

		/* We try to poll for finding a message with a MPC_ANY_SOURCE source
		 * also in case we are blocked we punctually poll any-source */
		lcp_progress(lcp_mngr_loc);


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
#endif

/* This is the exported version */
void mpc_lowcomm_ptp_msg_progress(struct mpc_lowcomm_ptp_msg_progress_s *wait)
{
        UNUSED(wait);
        not_reachable();
#if 0
	return __mpc_comm_ptp_msg_wait(wait);
#endif
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
__attribute__((deprecated("Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
void _mpc_comm_ptp_message_send_check(mpc_lowcomm_ptp_message_t *msg, int poll_receiver_end)
{
        UNUSED(msg);
        UNUSED(poll_receiver_end);
#if 0

	assert(mpc_common_get_process_rank() >= 0);
	assert(
		/* Does not overflow */
		(mpc_common_get_process_count() >= SCTK_MSG_DEST_PROCESS(msg) ) ||
		/* Is different process set */
		(mpc_lowcomm_peer_get_set(SCTK_MSG_DEST_PROCESS_UID(msg) ) != mpc_lowcomm_monitor_get_gid() )
		);

	/* Flag msg's request as a send one */
	if(msg->tail.request != NULL)
	{
		msg->tail.request->request_type = REQUEST_SEND;
	}

	mpc_common_nodebug("%llu != %llu ?", SCTK_MSG_DEST_PROCESS_UID(msg), mpc_lowcomm_monitor_get_uid() );
	/*  Message has not reached its destination */
	if(SCTK_MSG_DEST_PROCESS_UID(msg) != mpc_lowcomm_monitor_get_uid() )
	{
		/* We forward the message */
		int rc;
		mpc_lowcomm_peer_uid_t uid = SCTK_MSG_DEST_PROCESS_UID(msg);
		if(SCTK_DATATYPE(msg) == MPC_LOWCOMM_PACKED)
		{
			mpc_common_debug_fatal("LCP: Packed not supported");
		}
		lcp_ep_h ep;

		if(!(ep = lcp_ep_get(lcp_mngr_loc, uid)))
		{
			rc = lcp_ep_create(lcp_mngr_loc, &ep, uid, 0);
			if(rc != MPC_LOWCOMM_SUCCESS)
			{
				mpc_common_debug_fatal("Could not create endpoint "
				                       "for %lu.", uid);
			}
		}

		/* Set sequence number before sending */
		//_mpc_lowcomm_reorder_msg_register(msg);

		msg->tail.request->request_completion_fn =
			mpc_lowcomm_request_complete;
		rc = lcp_tag_send_nb(ep, NULL, msg->tail.message.contiguous.addr,
		                     msg->body.header.msg_size, NULL, NULL);
		if(rc != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("Could not send message %lu.", uid);
		}
		return;
	}

	if(_mpc_comm_ptp_message_is_for_control(SCTK_MSG_SPECIFIC_CLASS(msg) ) )
	{
		/* If we are on the right process with a control message */

		/* Message is for local process call the handler (such message are never pending)
		 *     no need to perform a matching with a receive. However, the order is guaranteed
		 *         by the reorder prior to calling this function */


		/* Done */
		return;
	}

	if(SCTK_MSG_COMPLETION_FLAG(msg) != NULL)
	{
		*(SCTK_MSG_COMPLETION_FLAG(msg) ) = MPC_LOWCOMM_MESSAGE_PENDING;
	}

	mpc_common_debug("SEND msg from %llu to %llu HERE %llu (FT %d TT %d)", SCTK_MSG_SRC_PROCESS_UID(msg), SCTK_MSG_DEST_PROCESS_UID(msg), mpc_lowcomm_monitor_get_uid(), SCTK_MSG_SRC_TASK(msg), SCTK_MSG_DEST_TASK(msg) );

	/* If the message is a UNIVERSE message it targets the process not a given task
	 * we then use the matching queue from the first local process as a convention */
	if(SCTK_MSG_SPECIFIC_CLASS(msg) == MPC_LOWCOMM_MESSAGE_UNIVERSE)
	{
		SCTK_MSG_DEST_TASK_SET(msg, _mpc_lowcomm_communicator_world_first_local_task() );
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
#endif
}

/*
 * Function called for sending a message (i.e: MPI_Send).
 * Mostly used by the file mpc.c
 * */
__attribute__((deprecated("Legacy function that should be removed once PTP messages are fully unused (deprecated by the LCP rework)")))
void mpc_lowcomm_ptp_message_send(mpc_lowcomm_ptp_message_t *msg)
{
        UNUSED(msg);
#if 0
	mpc_common_debug("SEND to %d (size %ld) %s COMM %llu T %d", SCTK_MSG_DEST_TASK(msg),
	                 SCTK_MSG_SIZE(msg),
	                 mpc_lowcomm_ptp_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
	                 SCTK_MSG_COMMUNICATOR_ID(msg),
	                 SCTK_MSG_TAG(msg) );


	int need_check = !_mpc_comm_is_remote_rank(SCTK_MSG_DEST_TASK(msg) );

	_mpc_comm_ptp_message_send_check(msg, need_check);
#endif
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
        UNUSED(msg);
        UNUSED(perform_check);
#if 0
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
		if(SCTK_DATATYPE(msg) == MPC_LOWCOMM_PACKED)
		{
			mpc_common_debug_fatal("LCP: Packed not supported");
		}
		msg->tail.request->request_completion_fn =
			mpc_lowcomm_request_complete;
		lcp_tag_recv_nb(NULL, NULL, msg->tail.message.contiguous.addr,
		                msg->body.header.msg_size, NULL, 0, 0, NULL);
		return;
	}
	else
	{
		msg->tail.remote_source = 0;
	}

	/* We add the message to the pending list */
	__mpc_comm_ptp_message_list_add_incoming_recv(recv_ptp, msg);

	/* If we ask for a matching, we run it */
	if(perform_check)
	{
		__mpc_comm_ptp_perform_msg_pair_trylock(recv_ptp);
	}
#endif
}

/*
 * Function called for receiving a message (i.e: MPI_Recv).
 * Mostly used by the file mpc.c
 * */
void mpc_lowcomm_ptp_message_recv(mpc_lowcomm_ptp_message_t *msg)
{
        UNUSED(msg);
#if 0
	int need_check = !_mpc_comm_is_remote_rank(SCTK_MSG_DEST_TASK(msg) );

	mpc_common_debug("RECV from %d (size %ld) %s COMM %llu T %d", SCTK_MSG_SRC_TASK(msg),
	                 SCTK_MSG_SIZE(msg),
	                 mpc_lowcomm_ptp_message_class_name[(int)SCTK_MSG_SPECIFIC_CLASS(msg)],
	                 SCTK_MSG_COMMUNICATOR_ID(msg),
	                 SCTK_MSG_TAG(msg) );

	_mpc_comm_ptp_message_recv_check(msg, need_check);
#endif
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
        UNUSED(destination);
        UNUSED(tag);
        UNUSED(class);
        UNUSED(comm);
        UNUSED(status);
        UNUSED(msg);
        UNUSED(source);
#if 0
	int world_source      = source;
	int world_destination = destination;

	msg->source_task      = world_source;
	msg->destination_task = world_destination;
	msg->message_tag      = tag;
	msg->communicator_id  = (__mpc_lowcomm_communicator_from_predefined(comm) )->id;
	msg->message_type     = class;

	switch(*status)
	{
		case 0: /* all rails supported probing request AND not found */
		case 1: /* found a match ! */
			return;

			break;

		default: /* not found AND at least one rail does not support low-level probing */
			*status       = 0;
			msg->msg_size = 0;
			break;
	}

	mpc_comm_ptp_t *dest_ptp = __mpc_comm_ptp_array_get(comm, world_destination);

	if(source != MPC_ANY_SOURCE)
	{
		mpc_comm_ptp_t *src_ptp = __mpc_comm_ptp_array_get(comm, world_source);

		if(src_ptp == NULL)
		{
			lcp_progress(lcp_mngr_loc);
		}
	}
	else
	{
		lcp_progress(lcp_mngr_loc);
	}

	assert(dest_ptp != NULL);
	__mpc_comm_ptp_message_list_lock_pending(&(dest_ptp->lists) );
	*status = __mpc_comm_ptp_probe(dest_ptp, msg);
	__mpc_comm_ptp_message_list_unlock_pending(&(dest_ptp->lists) );
#endif
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
#if 0
void mpc_lowcomm_request_init_struct(mpc_lowcomm_request_t *request,
                                     mpc_lowcomm_communicator_t comm,
                                     int request_type, int src, int dest,
                                     int tag, lcp_send_callback_func_t cb)
{
	__mpc_comm_request_init(request, comm, request_type);

	int is_intercomm = mpc_lowcomm_communicator_is_intercomm(comm);

	mpc_lowcomm_peer_uid_t src_uid  = 0;
	mpc_lowcomm_peer_uid_t dest_uid = 0;

	int src_task  = -1;
	int dest_task = -1;

	/* Fill in Source and Dest Process Informations (convert from task) */

	/* SOURCE */
	if(src != MPC_ANY_SOURCE)
	{
		if(is_intercomm)
		{
			if(request_type == REQUEST_RECV)
			{
				/* If this is a RECV make sure the translation is done on the source according to remote */
				src_task = mpc_lowcomm_communicator_remote_world_rank(comm, src);
				src_uid  = mpc_lowcomm_communicator_remote_uid(comm, src);
			}
			else if(request_type == REQUEST_SEND)
			{
				/* If this is a SEND make sure the translation is done on the dest according to remote */
				src_task = mpc_lowcomm_communicator_world_rank_of(comm, src);
				src_uid  = mpc_lowcomm_communicator_uid(comm, src);
			}
		}
		else
		{
			src_task = mpc_lowcomm_communicator_world_rank_of(comm, src);
			src_uid  = mpc_lowcomm_communicator_uid(comm, src);
		}
	}
	else
	{
		src_task = MPC_ANY_SOURCE;
		src_uid  = mpc_lowcomm_monitor_local_uid_of(MPC_ANY_SOURCE);
	}

	/* DEST Handling */
	if(is_intercomm)
	{
		if(request_type == REQUEST_RECV)
		{
			/* If this is a RECV make sure the translation is done on the source according to remote */
			dest_task = mpc_lowcomm_communicator_world_rank_of(comm, dest);
			dest_uid  = mpc_lowcomm_communicator_uid(comm, dest);
		}
		else if(request_type == REQUEST_SEND)
		{
			/* If this is a SEND make sure the translation is done on the dest according to remote */
			dest_task = mpc_lowcomm_communicator_remote_world_rank(comm, dest);
			dest_uid  = mpc_lowcomm_communicator_remote_uid(comm, dest);
		}
	}
	else
	{
		dest_task = mpc_lowcomm_communicator_world_rank_of(comm, dest);
		dest_uid  = mpc_lowcomm_communicator_uid(comm, dest);
	}

	request->header.source           = src_uid;
	request->header.destination      = dest_uid;
	request->header.destination_task = dest_task;
	request->header.source_task      = src_task;
	request->header.message_tag      = tag;
	request->header.communicator_id  = mpc_lowcomm_communicator_id(comm);
	request->is_null = 0;

	request->request_completion_fn = cb;
	request->completion_flag       = MPC_LOWCOMM_MESSAGE_PENDING;
	request->ptr_to_pin_ctx        = NULL;
}
#endif

void mpc_lowcomm_request_init(mpc_lowcomm_request_t *request,
                              int count, mpc_lowcomm_datatype_t datatype,
                              mpc_lowcomm_complete_callback_func_t cb,
                              unsigned flags)
{
        request->datatype         = datatype;
        request->count            = count;
        request->request_complete = cb;
        request->flags            = flags;
}

int mpc_lowcomm_request_cancel(mpc_lowcomm_request_t *msg)
{
        UNUSED(msg);
#if 0
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
#endif
	return MPC_LOWCOMM_SUCCESS;
}

static inline int __mpc_lowcomm_isend_class_src(int src, int dest, const void *data, size_t size,
                                                int tag, mpc_lowcomm_communicator_t comm,
                                                mpc_lowcomm_ptp_message_class_t class,
                                                mpc_lowcomm_request_t *req)
{
        UNUSED(src);
        UNUSED(dest);
        UNUSED(data);
        UNUSED(size);
        UNUSED(tag);
        UNUSED(comm);
        UNUSED(class);
        UNUSED(req);
#if 0
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

#endif
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
        UNUSED(src);
        UNUSED(dest);
        UNUSED(buffer);
        UNUSED(size);
        UNUSED(tag);
        UNUSED(comm);
        UNUSED(class);
        UNUSED(req);
#if 0
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

#endif
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
void *mpc_lowcomm_request_alloc()
{
        mpc_lowcomm_request_t *request = NULL;
	int tid = mpc_common_get_task_rank();
        lcp_task_h task;

	task = lcp_context_task_get(lcp_ctx_loc, tid);
	if(task == NULL)
	{
		mpc_common_debug_fatal("COMM: Could not find task with tid %d.",
		                       tid);
	}

        request = (mpc_lowcomm_request_t *)lcp_request_alloc(task);
        if (request == NULL) {
                return request;
        }
        request->flags = 0;

        return request + 1;
}

void mpc_lowcomm_set_packed_buf(mpc_lowcomm_request_t *request, void *buf,
                                size_t packed_size)
{
       request->flags      |= MPC_LOWCOMM_REQUEST_PACKED;
       request->packed_buf  = buf;
       request->packed_size = packed_size;
}

//FIXME: this function needs to be revised. Is she really useful?
int mpc_lowcomm_request_complete(mpc_lowcomm_request_t *request)
{
	mpc_lowcomm_request_set_null(request, 1);
        return MPC_LOWCOMM_SUCCESS;
}

void mpc_lowcomm_request_free(mpc_lowcomm_request_t *req)
{
        if (req->flags & MPC_LOWCOMM_REQUEST_PACKED) {
                sctk_free(req->packed_buf);
        }
        lcp_request_free(req);
}

#if 0
static int mpc_lowcomm_init_request_header(const int tag,
                                           const mpc_lowcomm_communicator_t comm,
                                           const int src,
                                           const int dest,
                                           const size_t count,
                                           mpc_lowcomm_datatype_t dt,
                                           mpc_lowcomm_request_type_t request_type,
                                           mpc_lowcomm_request_t *req)
{
        UNUSED(dt);
	int is_intercomm = mpc_lowcomm_communicator_is_intercomm(comm);

	mpc_lowcomm_peer_uid_t source_process = 0;
	mpc_lowcomm_peer_uid_t dest_process   = 0;

	int source_task = -1;
	int dest_task   = -1;
	/* Fill in Source and Dest Process Informations (convert from task) */

	/* SOURCE */
	int isource = src;

	if(mpc_lowcomm_peer_get_rank(src) != MPC_ANY_SOURCE)
	{
		if(is_intercomm)
		{
			if(request_type == REQUEST_RECV)
			{
				/* If this is a RECV make sure the translation is done on the source according to remote */
				source_task    = mpc_lowcomm_communicator_remote_world_rank(comm, isource);
				source_process = mpc_lowcomm_communicator_remote_uid(comm, isource);
			}
			else if(request_type == REQUEST_SEND)
			{
				/* If this is a SEND make sure the translation is done on the dest according to remote */
				source_task    = mpc_lowcomm_communicator_world_rank_of(comm, isource);
				source_process = mpc_lowcomm_communicator_uid(comm, isource);
			}
		}
		else
		{
			source_task    = mpc_lowcomm_communicator_world_rank_of(comm, isource);
			source_process = mpc_lowcomm_communicator_uid(comm, isource);
		}
	}
	else
	{
		source_task    = MPC_ANY_SOURCE;
		source_process = mpc_lowcomm_monitor_local_uid_of(MPC_ANY_SOURCE);
	}

	/* DEST Handling */
	int idestination = dest;

	if(is_intercomm)
	{
		if(request_type == REQUEST_RECV)
		{
			/* If this is a RECV make sure the translation is done on the source according to remote */
			dest_task    = mpc_lowcomm_communicator_world_rank_of(comm, idestination);
			dest_process = mpc_lowcomm_communicator_uid(comm, idestination);
		}
		else if(request_type == REQUEST_SEND)
		{
			/* If this is a SEND make sure the translation is done on the dest according to remote */
			dest_task    = mpc_lowcomm_communicator_remote_world_rank(comm, idestination);
			dest_process = mpc_lowcomm_communicator_remote_uid(comm, idestination);
		}
	}
	else
	{
		dest_task    = mpc_lowcomm_communicator_world_rank_of(comm, idestination);
		dest_process = mpc_lowcomm_communicator_uid(comm, idestination);
	}

	if(request_type == REQUEST_SEND)
	{
		__mpc_comm_request_init(req, comm, REQUEST_SEND);
	}
	else
	{
		__mpc_comm_request_init(req, comm, REQUEST_RECV);
	}

	req->header.destination      = dest_process;
	req->header.destination_task = dest_task;
	req->header.source           = source_process;
	req->header.source_task      = source_task;
	req->header.message_tag      = tag;
	req->header.msg_size         = count;
	req->completion_flag         = MPC_LOWCOMM_MESSAGE_PENDING;
	req->is_null               = 0;
	req->status_error          = MPC_LOWCOMM_SUCCESS;
	req->ptr_to_pin_ctx        = NULL;
	req->request_completion_fn = mpc_lowcomm_request_complete;

	return 0;
}
#endif

static mpc_lowcomm_request_t __request_null;

static inline void __init_request_null()
{
        mpc_lowcomm_request_init(&__request_null,
                                 0, NULL, NULL, 0);
	__request_null.is_null = 1;
        __request_null.request_type = REQUEST_NULL;
        __request_null.comm    = MPC_COMM_NULL;
        __request_null.completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
}

mpc_lowcomm_request_t * mpc_lowcomm_request_null(void)
{
	return &__request_null;
}

static inline void _mpc_lowcomm_build_send_tag_info(lcp_tag_info_t *tag_info,
                                                    mpc_lowcomm_communicator_t comm,
                                                    int dest, int tag,
                                                    mpc_lowcomm_request_t *req)
{
        tag_info->tag = tag;
        tag_info->src_uid = mpc_lowcomm_monitor_get_uid();
        tag_info->comm_id = (uint16_t)mpc_lowcomm_communicator_id(comm);

        if (mpc_lowcomm_communicator_is_intercomm(comm)) {
                tag_info->src      = mpc_lowcomm_communicator_rank(comm);
                tag_info->dest_uid = mpc_lowcomm_communicator_remote_uid(comm, dest);
                tag_info->dest     = mpc_lowcomm_communicator_remote_world_rank(comm, dest);
        } else {
                tag_info->src      = mpc_lowcomm_communicator_rank_fast(comm);
                tag_info->dest_uid = mpc_lowcomm_communicator_uid(comm, dest);
                tag_info->dest     = mpc_lowcomm_communicator_world_rank_of(comm, dest);
        }

        //FIXME: see fixme below.
        req->recv_info.src = tag_info->src;
        req->recv_info.tag = tag;
}

static int _mpc_lowcomm_request_send_complete(int status, void *request, size_t length)
{
        mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

        assert(req->completion_flag != MPC_LOWCOMM_MESSAGE_DONE);
        if (status != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_fatal("COMM: request failed with status=%d.",
                                       status);
        }

        //FIXME: MPI standard does not require the size, tag and src fields to
        //       be filled but some MPC test in the CI requires send, so set
        //       them from header info.
        req->status_error     = status;
        req->recv_info.length = length;

        if (req->flags & MPC_LOWCOMM_REQUEST_CALLBACK) {
                req->request_complete(req);
        }

        req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;

        return MPC_LOWCOMM_SUCCESS;
}

int _mpc_lowcomm_isend(int dest, const void *data, size_t size, int tag,
                       mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req,
                       int synchronized)
{
        lcp_tag_info_t tag_info;
        lcp_ep_h ep;
        lcp_task_h task;
        lcp_status_ptr_t request;

        /* Initialize needed lowcomm request fields. */
        mpc_lowcomm_request_set_null(req, 0);
        req->request_type    = REQUEST_SEND;
        req->comm            = comm;
        req->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;

        /* Initialize tag information based on comm: src, src_uid, tag,... */
        _mpc_lowcomm_build_send_tag_info(&tag_info, comm, dest, tag, req);

        /* Try to get cached endpoint. */
	ep = mpc_lowcomm_communicator_lookup(comm, dest);
	if(ep == NULL)
	{
                int rc = lcp_ep_get_or_create(lcp_mngr_loc, tag_info.dest_uid,
                                              &ep, 0);
                if (rc != 0) {
                        mpc_common_debug_fatal("Could not create endpoint.");
                }
                /* Add endpoint to communicator cache. */
                mpc_lowcomm_communicator_add_ep(comm, dest, ep);
	}

        /* Get MPI rank data store. */
	task = lcp_context_task_get(lcp_ctx_loc, mpc_common_get_task_rank());
	if(task == NULL)
	{
		mpc_common_debug_fatal("LCP: Could not find task with tid %d.",
		                       tag_info.src);
	}

	/* Fill up request parameters. */
	lcp_request_param_t param =
	{
		.field_mask = (synchronized ? LCP_REQUEST_TAG_SYNC : 0) |
                        LCP_REQUEST_SEND_CALLBACK,
                // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
                .datatype   = req->dt_magic == (int)0xDDDDDDDD ? LCP_DATATYPE_DERIVED : LCP_DATATYPE_CONTIGUOUS,
                .request    = req,
                .send_cb    = _mpc_lowcomm_request_send_complete,
	};

	/* Request has been allocated through mpc_lowcomm_request_alloc. */
	if ( req->is_allocated == MPC_LOWCOMM_REQUEST_ALLOC )
	{
		param.field_mask |= LCP_REQUEST_USER_REQUEST;
        } else {
                req->flags = 0;
        }

	request = lcp_tag_send_nb(ep, task, data, size, &tag_info, &param);

        if (LCP_PTR_IS_ERR(request)) {
                return MPC_LOWCOMM_ERROR;
        } else {
                return MPC_LOWCOMM_SUCCESS;
        }
}

int mpc_lowcomm_ssend(int dest, const void *data, size_t size, int tag,
                      mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req)
{
	return _mpc_lowcomm_isend(dest, data, size, tag, comm, req, 1);
}

int mpc_lowcomm_isend(int dest, const void *data, size_t size, int tag,
                      mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req)
{
	return _mpc_lowcomm_isend(dest, data, size, tag, comm, req, 0);
}

static int _mpc_lowcomm_request_recv_complete(int status, void *request,
                                              lcp_tag_recv_info_t *tag_info)
{
        mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

        assert(status == MPC_LOWCOMM_SUCCESS);

        req->recv_info.length = tag_info->length;
        req->recv_info.src    = tag_info->src;
        req->recv_info.tag    = tag_info->tag;
        req->status_error     = status;

        if (req->flags & MPC_LOWCOMM_REQUEST_CALLBACK) {
                req->request_complete(req);
        }

        req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;

        return MPC_LOWCOMM_SUCCESS;
}

static inline void _mpc_lowcomm_build_recv_tag_info(lcp_tag_info_t *tag_info,
                                                    mpc_lowcomm_communicator_t comm,
                                                    int src, int tag)
{
        tag_info->tag = tag;
	tag_info->src = src;
	tag_info->dest = mpc_common_get_task_rank();
        tag_info->comm_id = (uint16_t)mpc_lowcomm_communicator_id(comm);
}

int mpc_lowcomm_irecv(int src, void *data, size_t size, int tag,
                      mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req)
{
        lcp_status_ptr_t request;
        lcp_tag_info_t tag_info;
        lcp_task_h task;

        /* Initialize needed lowcomm request fields. */
        mpc_lowcomm_request_set_null(req, 0);
        req->request_type    = REQUEST_RECV;
        req->comm            = comm;
        req->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;

        /* Initialize tag information based on comm: src, src_uid, tag,... */
        _mpc_lowcomm_build_recv_tag_info(&tag_info, comm, src, tag);

	task = lcp_context_task_get(lcp_ctx_loc, tag_info.dest);
	if(task == NULL) {
		mpc_common_debug_fatal("LCP: Could not find task with tid %d.",
		                       tag_info.src);
	}

	/* Fill up request parameters. */
	lcp_request_param_t param = {
                .field_mask   = LCP_REQUEST_RECV_CALLBACK,
                // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
                .datatype   = req->dt_magic == (int)0xDDDDDDDD ? LCP_DATATYPE_DERIVED : LCP_DATATYPE_CONTIGUOUS,
                .request      = req,
                .recv_cb      = _mpc_lowcomm_request_recv_complete,
        };
        if (req->is_allocated == MPC_LOWCOMM_REQUEST_ALLOC) {
                param.field_mask |= LCP_REQUEST_USER_REQUEST;
        } else {
                req->flags = 0;
        }

        request = lcp_tag_recv_nb(lcp_mngr_loc, task, data, size, &tag_info,
                                  src == MPC_ANY_SOURCE ? 0 : ~0,
                                  tag == MPC_ANY_TAG ? 0 : ~0,
                                  &param);

        if (LCP_PTR_IS_ERR(request)) {
                return MPC_LOWCOMM_ERROR;
        } else {
                return MPC_LOWCOMM_SUCCESS;
        }
}

int mpc_lowcomm_sendrecv(const void *sendbuf, size_t size, int dest, int tag, void *recvbuf,
                         int src, mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_request_t sendreq, recvreq;

        mpc_lowcomm_request_init(&sendreq, size, NULL,
                                 NULL, 0);
        mpc_lowcomm_request_init(&recvreq, size, NULL,
                                 NULL, 0);

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
        mpc_lowcomm_request_complete(request);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_test(mpc_lowcomm_request_t *request, int *completed, mpc_lowcomm_status_t *status)
{
        int rc = MPC_LOWCOMM_SUCCESS;
	*completed = 0;

	/* Always make sure to start on clean grounds */
	if(status)
	{
		status->MPC_ERROR = MPC_LOWCOMM_SUCCESS;
	}

	if(__request_is_null_or_cancelled(request) )
	{
		*completed = 1;
		return MPC_LOWCOMM_SUCCESS;
	}

        rc = mpc_common_progress();
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

	*completed = (request->completion_flag == MPC_LOWCOMM_MESSAGE_DONE);

	if(*completed)
	{
		mpc_lowcomm_commit_status_from_request(request, status);
                mpc_lowcomm_request_complete(request);
	}

err:
        return rc;
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
        int rc = MPC_LOWCOMM_SUCCESS;
	mpc_lowcomm_request_t req;

        mpc_lowcomm_request_init(&req, size, NULL, NULL, 0);

	mpc_lowcomm_isend(dest, data, size, tag, comm, &req);
	mpc_lowcomm_status_t status;

	mpc_lowcomm_wait(&req, &status);
	CHECK_STATUS_ERROR(&status);

	return rc;
}

int mpc_lowcomm_recv(int src, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm)
{
        int rc = MPC_LOWCOMM_SUCCESS;
	mpc_lowcomm_request_t req;

        mpc_lowcomm_request_init(&req, size, NULL, NULL, 0);

	mpc_lowcomm_irecv(src, buffer, size, tag, comm, &req);
	mpc_lowcomm_status_t status;

	mpc_lowcomm_wait(&req, &status);
	CHECK_STATUS_ERROR(&status);

	return rc;
}

int mpc_lowcomm_iprobe_src_dest(const int world_source, const int world_destination, const int tag,
                                const mpc_lowcomm_communicator_t comm, int *flag, mpc_lowcomm_status_t *status)
{
	//FIXME: move code to mpc_lowcomm_iprobe
	int                  rc          = MPC_LOWCOMM_SUCCESS;
	lcp_task_h           task        = NULL;
	lcp_tag_recv_info_t  recv_info   = { 0 };
	mpc_lowcomm_status_t status_init = MPC_LOWCOMM_STATUS_INIT;
	int                  has_status  = 1;
        mpc_lowcomm_communicator_id_t comm_id = mpc_lowcomm_communicator_id(comm);

	if(status == MPC_LOWCOMM_STATUS_NULL)
	{
		has_status = 0;
	}
	else
	{
		*status = status_init;
	}

	task = lcp_context_task_get(lcp_ctx_loc, world_destination);
	if(task == NULL)
	{
		mpc_common_debug_fatal("LCP: Could not find task with tid %d.",
		                       world_destination);
	}

	rc = lcp_tag_probe_nb(lcp_mngr_loc, task, world_source, tag,
	                      comm_id, &recv_info);

        //FIXME: should it be before probe?
	/* Progress communications to check if event has been raised. */
	mpc_common_progress();

	if( (*flag = recv_info.found) )
	{
		if(has_status)
		{
			status->MPC_SOURCE = recv_info.src;
			status->MPC_TAG    = recv_info.tag;
			status->size       = recv_info.length;
			status->MPC_ERROR  = MPC_LOWCOMM_SUCCESS;
		}

		return rc;
	}

	return rc;
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
			&probe_struct.flag, 1, (void (*)(void *) ) __mpc_probe_poll, &probe_struct);
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
                mpc_lowcomm_request_init(req, size, NULL, NULL, 0);
		return MPC_LOWCOMM_SUCCESS;
	}

	if(!mpc_lowcomm_monitor_peer_exists(dest) )
	{
		mpc_common_debug_warning("%s : peer %s does not appear to exist",
		                         __FUNCTION__,
		                         mpc_lowcomm_peer_format(dest) );
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
                mpc_lowcomm_request_init(req, size, NULL, NULL, 0);
		return MPC_LOWCOMM_SUCCESS;
	}

	if(mpc_lowcomm_peer_get_rank(src) != MPC_ANY_SOURCE)
	{
		if(!mpc_lowcomm_monitor_peer_exists(src) )
		{
			mpc_common_debug_warning("%s : peer %s does not appear to exist",
			                         __FUNCTION__,
			                         mpc_lowcomm_peer_format(src) );
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

int mpc_lowcomm_open_port(char *port_name, int port_name_len)
{
	return mpc_lowcomm_monitor_open_port(port_name, port_name_len);
}

int mpc_lowcomm_close_port(const char *port_name)
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

	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_naming(mpc_lowcomm_monitor_uid_of(mpc_lowcomm_monitor_get_gid(), 0),
	                                                                 cmd,
	                                                                 mpc_lowcomm_monitor_get_uid(),
	                                                                 service_name,
	                                                                 port_name,
	                                                                 &mret);

	mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

	int err = 0;


	if( (content->naming.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS) ||
	    (mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS) )
	{
		err = 1;
	}

	mpc_lowcomm_monitor_response_free(resp);

	return err?MPC_LOWCOMM_ERROR:MPC_LOWCOMM_SUCCESS;
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
	mpc_lowcomm_monitor_retcode_t mret = MPC_LOWCOMM_MONITOR_RET_ERROR;


	for(i = 0 ; i < roots_count && mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS; i++)
	{
		/* Now try to resolve starting from ourselves */
		mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_naming(set_roots[i],
		                                                                 MPC_LOWCOMM_MONITOR_NAMING_GET,
		                                                                 mpc_lowcomm_monitor_get_uid(),
		                                                                 service_name,
		                                                                 "",
		                                                                 &mret);

		if (mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
		{
			continue;
		}

		mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

		if (content->naming.retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
		{
			mret = content->naming.retcode;
			continue;
		}

		snprintf(port_name, port_name_len, "%s", content->naming.port_name);
		mpc_lowcomm_monitor_response_free(resp);
	}

	_mpc_lowcomm_free_set_roots(set_roots);

	if (mret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_warning("%s could not find any matching peer for service %s",
				__func__, service_name);

		return MPC_LOWCOMM_ERROR;
	}

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

		//mpc_lowcomm_init_per_task(task_rank);

                _mpc_lowcomm_datatype_init_common();

                lcp_task_h task;
		int rc = lcp_task_create(lcp_ctx_loc, task_rank, &task);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_fatal("LCP: could not create task "
			                       "tid=%d", task_rank);
		}

                rc = lcp_task_associate(task, lcp_mngr_loc);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_fatal("LCP: could not associate "
                                               "task. tid=%d.", task_rank);
                }

                _mpc_lowcomm_communicator_init_task(task_rank);
		mpc_lowcomm_terminaison_barrier();

		_mpc_lowcomm_pset_bootstrap();

		mpc_lowcomm_allocmem_pool_init();

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
		//mpc_lowcomm_release_per_task(task_rank);

                lcp_task_h task = lcp_context_task_get(lcp_ctx_loc, task_rank);
                if(task == NULL)
                {
                        mpc_common_debug_fatal("LCP: Could not find task with tid %d.",
                                               task_rank);
                }
                lcp_task_dissociate(task, lcp_mngr_loc);

		mpc_common_nodebug("mpc_lowcomm_terminaison_barrier done");
		_mpc_lowcomm_monitor_teardown_per_task();
	}
	else
	{
		not_reachable();
	}
}

/********************************************************************/
/* Memory Allocator                                                 */
/********************************************************************/

#ifdef MPC_Allocator

static size_t __mpc_memory_allocation_hook(size_t size_origin)
{
	UNUSED(size_origin);
	return 0;
}

void __mpc_memory_free_hook(void *ptr, size_t size)
{
	//mpc_common_debug_error("FREE %p size %ld", ptr, size);

	if(mpc_lowcomm_allocmem_is_in_pool(ptr))
	{
		//mpc_common_debug_error("FREE pool");
		mpc_lowcomm_allocmem_pool_free_size(ptr, size);
	}

	UNUSED(ptr);
	UNUSED(size);
}

#endif

int mpc_lowcomm_pass_mpi_request_info(size_t request_size, void (*request_init_func)(void *))
{
        mpi_req_info.size = request_size;
        mpi_req_info.request_init_func = request_init_func;

        return MPC_LOWCOMM_SUCCESS;
}

void mpc_lowcomm_request_initialize(void *request) {
        mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

        req->is_allocated = MPC_LOWCOMM_REQUEST_ALLOC;
        mpc_lowcomm_request_set_null(request, 0);

        mpi_req_info.request_init_func(req + 1);
}

static int mpc_comm_progress()
{
      return lcp_progress(lcp_mngr_loc);
}

static void __initialize_drivers()
{


#ifdef MPC_Allocator
	sctk_net_memory_allocation_hook_register(__mpc_memory_allocation_hook);
	sctk_net_memory_free_hook_register(__mpc_memory_free_hook);
#endif


	int rc;

	_mpc_lowcomm_monitor_setup();
	_mpc_lowcomm_checksum_init();

        //FIXME: task count is number of MPI task while ctx->num_tasks should be
        //       the number of local tasks. At this point,
        //       mpc_common_get_local_task_count() is not setup yet.
	lcp_context_param_t ctx_params =
	{
                .field_mask     = LCP_CONTEXT_DATATYPE_OPS  |
                        LCP_CONTEXT_NUM_TASKS               |
                        LCP_CONTEXT_PROCESS_UID             |
                        LCP_CONTEXT_REQUEST_SIZE            |
                        LCP_CONTEXT_REQUEST_CB,
		.dt_ops         =
		{
			.pack   = mpc_lowcomm_request_pack,
			.unpack = mpc_lowcomm_request_unpack,
		},
                .num_tasks      = mpc_common_get_task_count(),
		.process_uid    = mpc_lowcomm_monitor_get_uid(),
                .request_init   = mpc_lowcomm_request_initialize,
                .request_size   = mpi_req_info.size + sizeof(mpc_lowcomm_request_t),
	};
	rc = lcp_context_create(&lcp_ctx_loc, &ctx_params);
	if(rc != MPC_LOWCOMM_SUCCESS)
	{
	mpc_common_debug_fatal("COMM: context creation failed.");
	}

        lcp_manager_param_t mngr_params = {
                .field_mask     = LCP_MANAGER_ESTIMATED_EPS | LCP_MANAGER_COMM_MODEL,
                .estimated_eps  = mpc_common_get_process_count(),
                .flags          = LCP_MANAGER_TSC_MODEL,
        };
        rc = lcp_manager_create(lcp_ctx_loc, &lcp_mngr_loc, &mngr_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_fatal("COMM: manager creation failed.");
        }

        mpc_common_progress_register(mpc_comm_progress);

	__init_request_null();

	_mpc_lowcomm_communicator_init();
}

static void __finalize_driver()
{
        //FIXME: error checking
        lcp_manager_fini(lcp_mngr_loc);
	lcp_context_fini(lcp_ctx_loc);
	_mpc_lowcomm_communicator_release();
	_mpc_lowcomm_monitor_teardown();
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
