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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #   - JAEGER Julien julien.jaeger@cea.fr                               # */
/* #                                                                      # */
/* ######################################################################## */

#include "comm_lib.h"


#include <math.h>
#include <mpc_config.h>

#include <mpi_conf.h>
#include <sys/time.h>


#include <mpc_common_flags.h>
#include <mpc_common_helper.h>
#include <mpc_common_profiler.h>
#include <mpc_common_rank.h>
#include <mpc_launch.h>
#include <mpc_launch_pmi.h>
#include <mpc_lowcomm_msg.h>
#include <mpc_topology.h>

#include "mpc_lowcomm_types.h"

#include "mpi_rma_ctrl_msg.h"
#include "mpi_rma_epoch.h"

#include "mpc_reduction.h"

#include "egreq_progress.h"
#include "errh.h"

#ifdef MPC_Threads
	#include <mpc_thread.h>
#endif

#ifdef MPC_USE_DMTCP
	#include "sctk_ft_iface.h"
#endif

#include <sctk_alloc.h>

#include "mpc_lowcomm_workshare.h"
#include <mpc_lowcomm_datatypes.h>

sctk_Op_f sctk_get_common_function(mpc_lowcomm_datatype_t datatype, sctk_Op op);


/************************************************************************/
/*GLOBAL VARIABLES                                                      */
/************************************************************************/

static mpc_thread_keys_t sctk_func_key;


/** \brief Intitializes thread context keys
 * This function is called by mpc_thread_spawn_mpi_tasks
 */
void mpc_cl_init_thread_keys()
{
	mpc_thread_keys_create(&sctk_func_key, NULL);
}

/****************************
* PER COMMUNICATOR STORAGE *
****************************/

static inline mpc_per_communicator_t *_mpc_cl_per_communicator_get_no_lock(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
                                                                           mpc_lowcomm_communicator_t comm)
{
	mpc_per_communicator_t *per_communicator = NULL;

	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	HASH_FIND(hh, task_specific->per_communicator, &comm,
	          sizeof(mpc_lowcomm_communicator_t), per_communicator);
	return per_communicator;
}

mpc_per_communicator_t *_mpc_cl_per_communicator_get(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
                                                     mpc_lowcomm_communicator_t comm)
{
	mpc_per_communicator_t *per_communicator = NULL;

	mpc_common_spinlock_lock(&(task_specific->per_communicator_lock) );
	per_communicator = _mpc_cl_per_communicator_get_no_lock(task_specific, comm);
	mpc_common_spinlock_unlock(&(task_specific->per_communicator_lock) );
	return per_communicator;
}

static inline void __mpc_cl_per_communicator_set(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
                                                 mpc_per_communicator_t *mpc_per_comm,
                                                 mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_common_spinlock_lock(&(task_specific->per_communicator_lock) );
	mpc_per_comm->key = comm;
	HASH_ADD(hh, task_specific->per_communicator, key,
	         sizeof(mpc_lowcomm_communicator_t), mpc_per_comm);
	mpc_common_spinlock_unlock(&(task_specific->per_communicator_lock) );
}

static inline void __mpc_cl_per_communicator_delete(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
                                                    mpc_lowcomm_communicator_t comm)
{
	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	mpc_common_spinlock_lock(&(task_specific->per_communicator_lock) );
	mpc_per_communicator_t *per_communicator = _mpc_cl_per_communicator_get_no_lock(task_specific, comm);

	if(!per_communicator)
	{
		mpc_common_spinlock_unlock(&(task_specific->per_communicator_lock) );
		return;
	}

	assume(per_communicator->key == comm);
	HASH_DELETE(hh, task_specific->per_communicator, per_communicator);
	sctk_free(per_communicator);
	mpc_common_spinlock_unlock(&(task_specific->per_communicator_lock) );
}

static inline mpc_per_communicator_t *__mpc_cl_per_communicator_alloc()
{
	mpc_per_communicator_t *tmp = NULL;
	mpc_common_spinlock_t   lock = MPC_COMMON_SPINLOCK_INITIALIZER;

	tmp = sctk_malloc(sizeof(mpc_per_communicator_t) );
	tmp->err_handler                       = ( MPC_Handler_function * )_mpc_cl_default_error;
	tmp->err_handler_lock                  = lock;
	tmp->mpc_mpi_per_communicator          = NULL;
	tmp->mpc_mpi_per_communicator_copy     = NULL;
	tmp->mpc_mpi_per_communicator_copy_dup = NULL;
	return tmp;
}

static inline void ___mpc_cl_per_communicator_copy(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
                                                   mpc_lowcomm_communicator_t new_comm,
                                                   mpc_per_communicator_t *per_communicator,
                                                   void (*copy_fn)(struct mpc_mpi_per_communicator_s **, struct mpc_mpi_per_communicator_s *) )
{
	if(!per_communicator)
	{
		return;
	}

	mpc_per_communicator_t *per_communicator_new = __mpc_cl_per_communicator_alloc();

	memcpy(per_communicator_new, per_communicator,
	       sizeof(mpc_per_communicator_t) );

	if( (copy_fn) != NULL)
	{
		( copy_fn )(&(per_communicator_new->mpc_mpi_per_communicator),
		            per_communicator->mpc_mpi_per_communicator);
	}

	__mpc_cl_per_communicator_set(task_specific, per_communicator_new, new_comm);
}

static inline void __mpc_cl_per_communicator_alloc_from_existing(
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific, mpc_lowcomm_communicator_t new_comm,
	mpc_lowcomm_communicator_t old_comm)
{
	mpc_per_communicator_t *per_communicator = _mpc_cl_per_communicator_get_no_lock(task_specific, old_comm);

	if(!per_communicator)
	{
		return;
	}
	___mpc_cl_per_communicator_copy(task_specific, new_comm, per_communicator, per_communicator->mpc_mpi_per_communicator_copy);
}

static inline void __mpc_cl_per_communicator_alloc_from_existing_dup(
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific, mpc_lowcomm_communicator_t new_comm,
	mpc_lowcomm_communicator_t old_comm)
{
	mpc_per_communicator_t *per_communicator = _mpc_cl_per_communicator_get_no_lock(task_specific, old_comm);

	if(!per_communicator)
	{
		return;
	}

	___mpc_cl_per_communicator_copy(task_specific, new_comm, per_communicator, per_communicator->mpc_mpi_per_communicator_copy_dup);
}

/************************************************************************/
/* Request Handling                                                     */
/************************************************************************/


void mpc_mpi_cl_egreq_progress_poll();

static inline void __mpc_cl_request_progress(mpc_lowcomm_request_t *request)
{
	if(request->request_type == REQUEST_GENERALIZED)
	{
		/* Try to poll the request */
		mpc_mpi_cl_egreq_progress_poll();
		/* We are done here */
		return;
	}

	struct mpc_lowcomm_ptp_msg_progress_s _wait;

	mpc_lowcomm_ptp_msg_wait_init(&_wait, request, 0);

	mpc_lowcomm_ptp_msg_progress(&_wait);
}

/************************************************************************/
/* MPC per Thread context                                                   */
/************************************************************************/

/* Asyncronous Buffers Storage */

#define MAX_MPC_BUFFERED_MSG    64

typedef struct
{
	mpc_buffered_msg_t    buffer[MAX_MPC_BUFFERED_MSG];
	volatile int          buffer_rank;
	mpc_common_spinlock_t lock;
} __mpc_cl_buffer_t;

typedef struct sctk_thread_buffer_pool_s
{
	__mpc_cl_buffer_t sync;
	__mpc_cl_buffer_t async;
} __mpc_cl_buffer_pool_t;

/* This is the Per VP data-structure */

typedef struct mpc_mpi_cl_per_thread_ctx_s
{
	mpc_mpi_cl_per_mpi_process_ctx_t *per_task_ctx;
} mpc_mpi_cl_per_thread_ctx_t;




/* Per Thread Storage Interface */

extern __thread struct mpc_mpi_cl_per_thread_ctx_s *___mpc_p_per_thread_comm_ctx;

static inline mpc_mpi_cl_per_thread_ctx_t *__mpc_cl_per_thread_ctx_get()
{
	if(!___mpc_p_per_thread_comm_ctx)
	{
		mpc_mpi_cl_per_thread_ctx_init();
	}

	return ( mpc_mpi_cl_per_thread_ctx_t * )___mpc_p_per_thread_comm_ctx;
}

static inline mpc_mpi_cl_per_mpi_process_ctx_t *__mpc_cl_per_thread_ctx_get_task_level()
{
	mpc_mpi_cl_per_thread_ctx_t *ctx = __mpc_cl_per_thread_ctx_get();

	return ctx->per_task_ctx;
}

static inline void __mpc_cl_per_thread_ctx_set_task_level(mpc_mpi_cl_per_mpi_process_ctx_t *per_task)
{
	mpc_mpi_cl_per_thread_ctx_t *ctx = __mpc_cl_per_thread_ctx_get();

	ctx->per_task_ctx = per_task;
}

static inline void __mpc_cl_per_thread_ctx_set(mpc_mpi_cl_per_thread_ctx_t *pointer)
{
	___mpc_p_per_thread_comm_ctx = ( struct mpc_mpi_cl_per_thread_ctx_s * )pointer;
}

void mpc_mpi_cl_per_thread_ctx_init()
{
	if(___mpc_p_per_thread_comm_ctx)
	{
		return;
	}

	/* Allocate */
	mpc_mpi_cl_per_thread_ctx_t *tmp = NULL;

	tmp = sctk_malloc(sizeof(mpc_mpi_cl_per_thread_ctx_t) );
	assume(tmp != NULL);
	memset(tmp, 0, sizeof(mpc_mpi_cl_per_thread_ctx_t) );

	tmp->per_task_ctx = (mpc_mpi_cl_per_mpi_process_ctx_t *)mpc_thread_get_parent_mpi_task_ctx();
	/* Register thread context */
	__mpc_cl_per_thread_ctx_set(tmp);
}

void mpc_mpi_cl_per_thread_ctx_release()
{
	if(!___mpc_p_per_thread_comm_ctx)
	{
		return;
	}

	mpc_mpi_cl_per_thread_ctx_t *thread_specific = ___mpc_p_per_thread_comm_ctx;

	/* Free */
	sctk_free(thread_specific);
	__mpc_cl_per_thread_ctx_set(NULL);
}

/***************************
* DISGUISE SUPPORT IN MPC *
***************************/

static int *__mpc_p_disguise_local_to_global_table = NULL;
static struct mpc_mpi_cl_per_mpi_process_ctx_s **__mpc_p_disguise_costumes = NULL;

int __mpc_p_disguise_init(struct mpc_mpi_cl_per_mpi_process_ctx_s *my_specific)
{
	#ifdef MPC_Threads
	int my_id = mpc_common_get_local_task_rank();

	if(my_id == 0)
	{
		OPA_store_int(&__mpc_p_disguise_flag, 0);
		int local_count = mpc_common_get_local_task_count();
		__mpc_p_disguise_costumes = sctk_malloc(sizeof(struct mpc_mpi_cl_per_mpi_process_ctx_s *) * local_count);

		if(__mpc_p_disguise_costumes == NULL)
		{
			perror("malloc");
			abort();
		}

		__mpc_p_disguise_local_to_global_table = sctk_malloc(sizeof(int) * local_count);

		if(__mpc_p_disguise_local_to_global_table == NULL)
		{
			perror("malloc");
			abort();
		}
	}

	mpc_lowcomm_barrier(MPC_COMM_WORLD);
	__mpc_p_disguise_costumes[my_id] = my_specific;
	__mpc_p_disguise_local_to_global_table[my_id] = mpc_common_get_task_rank();
	mpc_lowcomm_barrier(MPC_COMM_WORLD);
	#endif
	return 0;
}

int MPCX_Disguise(mpc_lowcomm_communicator_t comm, int target_rank)
{
	#ifndef MPC_Threads
	bad_parameter("MPCX_Disguise is not supported when there is no MPC thread support", "");
	#else
	mpc_thread_mpi_disguise_t *th = mpc_thread_disguise_get();

	if(th->my_disguisement)
	{
		/* Sorry I'm already wearing a mask */
		return MPC_ERR_ARG;
	}
	comm = __mpc_lowcomm_communicator_from_predefined(comm);

	/* Retrieve the ctx pointer */
	int cwr         = mpc_lowcomm_communicator_world_rank_of( comm, target_rank);
	int local_count = mpc_common_get_local_task_count();
	int i = 0;

	for(i = 0; i < local_count; ++i)
	{
		if(__mpc_p_disguise_local_to_global_table[i] == cwr)
		{
			OPA_incr_int(&__mpc_p_disguise_flag);
			mpc_thread_disguise_set(__mpc_p_disguise_costumes[i]->thread_data,
			                        (void *)__mpc_p_disguise_costumes[i]);
			return MPC_LOWCOMM_SUCCESS;
		}
	}
	#endif

	return MPC_ERR_ARG;
}

int MPCX_Undisguise()
{
	#ifndef MPC_Threads
	bad_parameter("MPCX_Undisguise is not supported when there is no MPC thread support", "");
	#else
	mpc_thread_mpi_disguise_t *th = mpc_thread_disguise_get();

	if(th->my_disguisement == NULL)
	{
		return MPC_ERR_ARG;
	}

	mpc_thread_disguise_set(NULL, NULL);

	OPA_decr_int(&__mpc_p_disguise_flag);
	#endif
	return MPC_LOWCOMM_SUCCESS;
}

/*******************************
* MPC PER MPI PROCESS CONTEXT *
*******************************/

#ifdef MPC_IN_PROCESS_MODE
struct mpc_mpi_cl_per_mpi_process_ctx_s *___the_process_specific = NULL;
#endif

static inline int __mpc_cl_egreq_progress_init(mpc_mpi_cl_per_mpi_process_ctx_t *tmp);

/** \brief Initalizes a structure of type \ref mpc_mpi_cl_per_mpi_process_ctx_t
 */
static inline void ___mpc_cl_per_mpi_process_ctx_init(mpc_mpi_cl_per_mpi_process_ctx_t *tmp)
{
	/* First empty the whole mpc_mpi_cl_per_mpi_process_ctx_t */
	memset(tmp, 0, sizeof(mpc_mpi_cl_per_mpi_process_ctx_t) );
	tmp->thread_data   = mpc_thread_data_get();
	tmp->progress_list = NULL;
	/* Set task id */
	tmp->task_id = mpc_common_get_task_rank();
	__mpc_cl_egreq_progress_init(tmp);
	/* Initialize Data-type array */
	tmp->datatype_array = _mpc_dt_storage_init();
	/* Set initial per communicator data */
	mpc_per_communicator_t *per_comm_tmp = NULL;

	/* COMM_WORLD */
	per_comm_tmp = __mpc_cl_per_communicator_alloc();
	__mpc_cl_per_communicator_set(tmp, per_comm_tmp, MPC_COMM_WORLD);
	/* COMM_SELF */
	per_comm_tmp = __mpc_cl_per_communicator_alloc();
	__mpc_cl_per_communicator_set(tmp, per_comm_tmp, MPC_COMM_SELF);
	/* Propagate initialization to the thread context */
	mpc_mpi_cl_per_thread_ctx_init();
	/* Set MPI status informations */
	tmp->init_done    = 0;
	tmp->thread_level = -1;
	/* Create the MPI_Info factory */
	MPC_Info_factory_init(&tmp->info_fact);
	/* Create the context class handling structure */
	_mpc_egreq_classes_storage_init(&tmp->grequest_context);
	/* Clear exit handlers */
	tmp->exit_handlers = NULL;
#ifdef MPC_IN_PROCESS_MODE
	___the_process_specific = tmp;
#endif
}

/** \brief Relases a structure of type \ref mpc_mpi_cl_per_mpi_process_ctx_t
 */
static inline void ___mpc_cl_per_mpi_process_ctx_release(mpc_mpi_cl_per_mpi_process_ctx_t *tmp)
{
	/* Release the MPI_Info factory */
	MPC_Info_factory_release(&tmp->info_fact);
	/* Release the context class handling structure */
	_mpc_egreq_classes_storage_release(&tmp->grequest_context);
}

/** \brief Creation point for MPI task context in an \ref mpc_mpi_cl_per_mpi_process_ctx_t
 *
 * This function allocates and initialises
 * an mpc_mpi_cl_per_mpi_process_ctx_t. It also takes care of storing it in the host
 * thread context.
 */
static inline void __mpc_cl_per_mpi_process_ctx_init()
{
	/* Retrieve the task ctx pointer */

	assume(__mpc_cl_per_thread_ctx_get_task_level() == NULL);

	mpc_mpi_cl_per_mpi_process_ctx_t *tmp = NULL;

	/* If not allocate a new mpc_mpi_cl_per_mpi_process_ctx_t */
	tmp = ( mpc_mpi_cl_per_mpi_process_ctx_t * )sctk_malloc(sizeof(mpc_mpi_cl_per_mpi_process_ctx_t) );
	/* And initalize it */
	___mpc_cl_per_mpi_process_ctx_init(tmp);
	/* Set the mpc_mpi_cl_per_mpi_process_ctx key in thread CTX */
	__mpc_cl_per_thread_ctx_set_task_level(tmp);
	/* Initialize common data-types */
	_mpc_dt_init();
	/* Register the task specific in the disguisemement array */
	__mpc_p_disguise_init(tmp);
}

/** \brief Define a function to be called when a task leaves
 *  \arg function Function to be callsed when task exits
 *  \return 1 on error 0 otherwise
 */
int mpc_mpi_cl_per_mpi_process_ctx_at_exit_register(void (*function)(void) )
{
	/* Retrieve the task ctx pointer */
	mpc_mpi_cl_per_mpi_process_ctx_t *tmp = __mpc_cl_per_thread_ctx_get_task_level();

	if(!tmp)
	{
		return 1;
	}

	struct mpc_mpi_cl_per_mpi_process_ctx_atexit_s *new_exit = sctk_malloc(sizeof(struct mpc_mpi_cl_per_mpi_process_ctx_atexit_s) );

	if(!new_exit)
	{
		return 1;
	}

	mpc_common_debug("Registering task-level atexit handler (%p)", function);
	new_exit->func = function;
	new_exit->next = tmp->exit_handlers;
	/* Register the entry */
	tmp->exit_handlers = new_exit;
	return 0;
}

/** \brief Call atexit handlers for the task (mimicking the process behavior)
 */
static inline void __mpc_cl_per_mpi_process_ctx_at_exit_trigger()
{
	/* Retrieve the task ctx pointer */
	mpc_mpi_cl_per_mpi_process_ctx_t *tmp = __mpc_cl_per_thread_ctx_get_task_level();

	if(!tmp)
	{
		return;
	}

	struct mpc_mpi_cl_per_mpi_process_ctx_atexit_s *current = tmp->exit_handlers;

	while(current)
	{
		struct mpc_mpi_cl_per_mpi_process_ctx_atexit_s *to_free = current;

		if(current->func)
		{
			mpc_common_debug("Calling task-level atexit handler (%p)", current->func);
			(current->func)();
		}

		current = current->next;
		sctk_free(to_free);
	}
}

static inline int __mpc_cl_egreq_progress_release(mpc_mpi_cl_per_mpi_process_ctx_t *tmp);

/** \brief Releases and frees task ctx
 *  This function releases MPI task ctx and remove them from host thread keys
 */
static void __mpc_cl_per_mpi_process_ctx_release()
{
	/* Retrieve the ctx pointer */
	mpc_mpi_cl_per_mpi_process_ctx_t *tmp = __mpc_cl_per_thread_ctx_get_task_level();

	/* Clear progress */
	__mpc_cl_egreq_progress_release(tmp);
	/* Free the type array */
	_mpc_dt_storage_release(tmp->datatype_array);
	/* Release common type info */
	_mpc_dt_release();
	/* Call atexit handlers */
	__mpc_cl_per_mpi_process_ctx_at_exit_trigger();
	/* Release the task ctx */
	___mpc_cl_per_mpi_process_ctx_release(tmp);
	/* Free the task ctx */

	/* Remove the ctx reference in the host thread */
	__mpc_cl_per_thread_ctx_set_task_level(NULL);

	sctk_free(tmp);
}

/** \brief Retrieves current thread task specific context
 */

static inline struct mpc_mpi_cl_per_mpi_process_ctx_s *_mpc_cl_per_mpi_process_ctx_get()
{
#ifdef MPC_IN_PROCESS_MODE
	return ___the_process_specific;
#endif
	int maybe_disguised = mpc_common_flags_disguised_get();

	if(maybe_disguised)
	{
		mpc_thread_mpi_disguise_t *th = mpc_thread_disguise_get();

		if(th->ctx_disguisement)
		{
			return th->ctx_disguisement;
		}
	}

	return __mpc_cl_per_thread_ctx_get_task_level();
}

struct mpc_mpi_cl_per_mpi_process_ctx_s *mpc_cl_per_mpi_process_ctx_get()
{
	return _mpc_cl_per_mpi_process_ctx_get();
}

/********************************
* PER PROCESS DATATYPE STORAGE *
********************************/

/**
 * \brief Retrieves the pointer on the datatype array
 *
 * \return The adress of the datatype array
 */
mpc_lowcomm_datatype_t _mpc_cl_per_mpi_process_ctx_user_datatype_array_get(void)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();

	return (mpc_lowcomm_datatype_t)task_specific->datatype_array->general_user_types;
}

/** \brief Retrieves a pointer to a general datatype from its datatype ID
 *
 *  \param task_specific Pointer to current task specific
 *  \param datatype datatype ID to be retrieved
 *
 */
_mpc_lowcomm_general_datatype_t *_mpc_cl_per_mpi_process_ctx_general_datatype_ts_get(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
                                                                                     const size_t datatype_idx)
{
	assert(task_specific != NULL);
	mpc_lowcomm_datatype_t ret = _mpc_dt_storage_get_general_datatype(task_specific->datatype_array, datatype_idx);

	return ret;
}

/** \brief Retrieves a pointer to a general datatype from its datatype ID
 *
 *  \param datatype datatype ID to be retrieved
 *
 */
_mpc_lowcomm_general_datatype_t *_mpc_cl_per_mpi_process_ctx_general_datatype_get(size_t datatype_idx)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();

	assert(task_specific != NULL);
	return _mpc_dt_storage_get_general_datatype(task_specific->datatype_array, datatype_idx);
}


/** \brief This function allows the retrieval of a data-type context
 *  \param datatype Target datatype
 *  \return NULL on error the context otherwise
 */
static inline struct _mpc_dt_footprint *__mpc_cl_datatype_get_ctx(mpc_lowcomm_datatype_t datatype)
{
	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			/* Nothing to do here */
			return NULL;

			break;

		case MPC_DATATYPES_USER:
			/* Get a pointer to the type of interest */
			return datatype->context;

			break;

		case MPC_DATATYPES_UNKNOWN:
			return NULL;

			break;
	}

	return NULL;
}

/** \brief This function spinlock on the datatype storage lock
 *
 * It will be used for convinance when modifying the datatype array
 *
 * \param task_specific Pointer to current task specific
 * \return 0 on success
 */
static inline int  _mpc_cl_per_mpi_process_ctx_datatype_ts_lock(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific)
{
	assume(task_specific != NULL);

	return mpc_common_spinlock_lock(&(task_specific->datatype_array->datatype_lock) );
}

/** \brief This function spinlock on the datatype storage lock of the current task
 *
 * It will be used for convinance when modifying the datatype array
 *
 * \return 0 on success
 */
int _mpc_cl_per_mpi_process_ctx_datatype_lock(void)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();

	assert(task_specific != NULL);
	return _mpc_cl_per_mpi_process_ctx_datatype_ts_lock(task_specific);
}

/** \brief This function releases the datatype storage lock
 *
 * It will be used for convinance when modifying the datatype array
 *
 * \param task_specific Pointer to current task specific
 * \return 0 on success
 */
static inline int  _mpc_cl_per_mpi_process_ctx_datatype_ts_unlock(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific)
{
	assume(task_specific != NULL);

	return mpc_common_spinlock_unlock(&(task_specific->datatype_array->datatype_lock) );
}

/** \brief This function releases the datatype storage lock of the current task
 *
 * It will be used for convinance when modifying the datatype array
 *
 * \return 0 on success
 */
int _mpc_cl_per_mpi_process_ctx_datatype_unlock(void)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();

	assert(task_specific != NULL);
	return _mpc_cl_per_mpi_process_ctx_datatype_ts_unlock(task_specific);
}

/********************
* MPC REDUCE FUNCS *
********************/

#undef MPC_CREATE_INTERN_FUNC
#define MPC_CREATE_INTERN_FUNC(name) \
	const sctk_Op MPC_ ## name = { MPC_ ## name ## _func, NULL }

MPC_CREATE_INTERN_FUNC(SUM);
MPC_CREATE_INTERN_FUNC(MAX);
MPC_CREATE_INTERN_FUNC(MIN);
MPC_CREATE_INTERN_FUNC(PROD);
MPC_CREATE_INTERN_FUNC(LAND);
MPC_CREATE_INTERN_FUNC(BAND);
MPC_CREATE_INTERN_FUNC(LOR);
MPC_CREATE_INTERN_FUNC(BOR);
MPC_CREATE_INTERN_FUNC(LXOR);
MPC_CREATE_INTERN_FUNC(BXOR);
MPC_CREATE_INTERN_FUNC(MINLOC);
MPC_CREATE_INTERN_FUNC(MAXLOC);

/************************************************************************/
/* Error Reporting                                                      */
/************************************************************************/

static inline int __MPC_ERROR_REPORT__(mpc_lowcomm_communicator_t comm, int error, char *message,
                                       char *function, char *file, int line)
{
	mpc_lowcomm_communicator_t comm_id = NULL;
	int error_id = 0;

	comm = __mpc_lowcomm_communicator_from_predefined(comm);
	MPC_Errhandler errh = ( MPC_Errhandler )_mpc_mpi_handle_get_errhandler(
		( sctk_handle )comm, _MPC_MPI_HANDLE_COMM);
	MPC_Handler_function *func = _mpc_mpi_errhandler_resolve(errh);

	comm_id  = __mpc_lowcomm_communicator_to_predefined(comm);
	error_id = error;
	( func )(&comm_id, &error_id, message, function, file, line);

	_mpc_mpi_errhandler_free(errh);
	return error;
}

#define MPC_ERROR_REPORT(comm, error, message) \
	return __MPC_ERROR_REPORT__(comm, error, message, (char *)__FUNCTION__, __FILE__, __LINE__)

#define MPC_ERROR_SUCCESS()    return MPC_LOWCOMM_SUCCESS;



/***************************************
* GENERALIZED REQUEST PROGRESS ENGINE *
***************************************/

static struct _mpc_egreq_progress_pool __mpc_progress_pool;

static inline int __mpc_cl_egreq_progress_init(mpc_mpi_cl_per_mpi_process_ctx_t *tmp)
{
	int my_local_id = mpc_common_get_local_task_rank();

	if(my_local_id == 0)
	{
		int local_count = mpc_common_get_local_task_count();
		_mpc_egreq_progress_pool_init(&__mpc_progress_pool, local_count);
	}

	mpc_lowcomm_barrier(MPC_COMM_WORLD);
	/* Retrieve the ctx pointer */
	tmp->progress_list = _mpc_egreq_progress_pool_join(&__mpc_progress_pool);
	return MPC_LOWCOMM_SUCCESS;
}

static inline int __mpc_cl_egreq_progress_release(mpc_mpi_cl_per_mpi_process_ctx_t *tmp)
{
	static mpc_common_spinlock_t l = MPC_COMMON_SPINLOCK_INITIALIZER;
	static mpc_common_spinlock_t d = MPC_COMMON_SPINLOCK_INITIALIZER;

	tmp->progress_list = NULL;
	int done = 0;

	mpc_common_spinlock_lock(&l);
	done = OPA_load_int(&d);
	OPA_store_int(&d, 1);
	mpc_common_spinlock_unlock(&l);

	if(done)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	_mpc_egreq_progress_pool_release(&__mpc_progress_pool);
	return MPC_LOWCOMM_SUCCESS;
}

static inline void __mpc_cl_egreq_progress_poll_id(int id)
{
	_mpc_egreq_progress_pool_poll(&__mpc_progress_pool, id);
}


void mpc_mpi_cl_egreq_progress_poll()
{
	struct mpc_mpi_cl_per_mpi_process_ctx_s *spe = _mpc_cl_per_mpi_process_ctx_get();

	__mpc_cl_egreq_progress_poll_id(spe->progress_list->id);
}

/************************************************************************/
/* Extended Generalized Requests                                        */
/************************************************************************/

int ___mpc_cl_egreq_disguise_poll(void *arg)
{
	int ret = 0;
	//mpc_common_debug_error("POLL %p", arg);
	int disguised = 0;
	int my_rank   = -1;
	mpc_lowcomm_request_t *req = ( mpc_lowcomm_request_t * )arg;

	if(!req->poll_fn)
	{
		ret = 0;
		goto POLL_DONE_G;
	}

	if(req->completion_flag == MPC_LOWCOMM_MESSAGE_DONE)
	{
		ret = 1;
		goto POLL_DONE_G;
	}

	my_rank = mpc_common_get_task_rank();

	if(my_rank != req->grequest_rank)
	{
		disguised = 1;

		if(MPCX_Disguise(MPC_COMM_WORLD, req->grequest_rank) != MPC_LOWCOMM_SUCCESS)
		{
			ret = 0;
			goto POLL_DONE_G;
		}
	}

	mpc_lowcomm_status_t tmp_status;

	(req->poll_fn)(req->extra_state, &tmp_status);
	ret = (req->completion_flag == MPC_LOWCOMM_MESSAGE_DONE);
POLL_DONE_G:

	if(disguised)
	{
		MPCX_Undisguise();
	}

	return ret;
}

/** \brief This function handles request starting for every generalized request
 * types
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param poll_fn Polling function called by the MPI runtime (CAN BE NULL)
 *  \param wait_fn Wait function called when the runtime waits several requests
 * of the same type (CAN BE NULL)
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
static inline int __mpc_cl_egreq_generic_start(sctk_Grequest_query_function *query_fn,
                                               sctk_Grequest_free_function *free_fn,
                                               sctk_Grequest_cancel_function *cancel_fn,
                                               sctk_Grequest_poll_fn *poll_fn,
                                               sctk_Grequest_wait_fn *wait_fn,
                                               void *extra_state, mpc_lowcomm_request_t *request)
{
	if(request == NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_ARG,
		                 "Bad request passed to MPC_Grequest_start");
	}

	/* Initialized as a NULL request */
	memcpy(request, mpc_lowcomm_request_null(), sizeof(mpc_lowcomm_request_t) );
	/* Change type */
	request->request_type = REQUEST_GENERALIZED;
	/* Set not null as we want to be waited */
	request->is_null       = 0;
	request->grequest_rank = mpc_common_get_task_rank();
	/* Fill in generalized request CTX */
	request->pointer_to_source_request = ( void * )request;
	request->query_fn    = query_fn;
	request->free_fn     = free_fn;
	request->cancel_fn   = cancel_fn;
	request->poll_fn     = poll_fn;
	request->wait_fn     = wait_fn;
	request->extra_state = extra_state;
	/* We set the request as pending */
	request->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;
	/* We now push the request inside the progress list */
	struct mpc_mpi_cl_per_mpi_process_ctx_s *ctx = _mpc_cl_per_mpi_process_ctx_get();

	request->progress_unit = NULL;
	request->progress_unit = ( void * )_mpc_egreq_progress_list_add(ctx->progress_list, ___mpc_cl_egreq_disguise_poll, ( void * )request);
	MPC_ERROR_SUCCESS()
}

int _mpc_cl_egrequest_start(sctk_Grequest_query_function *query_fn,
                            sctk_Grequest_free_function *free_fn,
                            sctk_Grequest_cancel_function *cancel_fn,
                            sctk_Grequest_poll_fn *poll_fn, void *extra_state,
                            mpc_lowcomm_request_t *request)
{
	return __mpc_cl_egreq_generic_start(query_fn, free_fn, cancel_fn, poll_fn,
	                                    NULL, extra_state, request);
}

/************************************************************************/
/* Extended Generalized Requests Request Classes                        */
/************************************************************************/


int _mpc_cl_grequest_class_create(sctk_Grequest_query_function *query_fn,
                                  sctk_Grequest_cancel_function *cancel_fn,
                                  sctk_Grequest_free_function *free_fn,
                                  sctk_Grequest_poll_fn *poll_fn,
                                  sctk_Grequest_wait_fn *wait_fn,
                                  sctk_Request_class *new_class)
{
	/* Retrieve task context */
	struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific = _mpc_cl_per_mpi_process_ctx_get();

	assume(task_specific != NULL);
	struct _mpc_egreq_classes_storage *class_ctx = &task_specific->grequest_context;

	/* Register the class */
	return _mpc_egreq_classes_storage_add_class(class_ctx, query_fn, cancel_fn, free_fn,
	                                            poll_fn, wait_fn, new_class);
}

int _mpc_cl_grequest_class_allocate(sctk_Request_class target_class,
                                    void *extra_state, mpc_lowcomm_request_t *request)
{
	/* Retrieve task context */
	struct mpc_mpi_cl_per_mpi_process_ctx_s *task_specific = _mpc_cl_per_mpi_process_ctx_get();

	assume(task_specific != NULL);
	struct _mpc_egreq_classes_storage *class_ctx = &task_specific->grequest_context;
	/* Retrieve task description */
	MPCX_GRequest_class_t *class_desc =
		_mpc_egreq_classes_storage_get_class(class_ctx, target_class);

	/* Not found */
	if(!class_desc)
	{
		MPC_ERROR_REPORT(
			MPC_COMM_SELF, MPC_ERR_ARG,
			"Bad sctk_Request_class passed to PMPIX_Grequest_class_allocate");
	}

	return __mpc_cl_egreq_generic_start(
		class_desc->query_fn, class_desc->free_fn, class_desc->cancel_fn,
		class_desc->poll_fn, class_desc->wait_fn, extra_state, request);
}

/************************************************************************/
/* Generalized Requests                                                 */
/************************************************************************/


int _mpc_cl_grequest_start(sctk_Grequest_query_function *query_fn,
                           sctk_Grequest_free_function *free_fn,
                           sctk_Grequest_cancel_function *cancel_fn,
                           void *extra_state, mpc_lowcomm_request_t *request)
{
	return __mpc_cl_egreq_generic_start(query_fn, free_fn, cancel_fn, NULL, NULL,
	                                    extra_state, request);
}

int _mpc_cl_grequest_complete(mpc_lowcomm_request_t request)
{
	mpc_lowcomm_request_t *src_req = ( mpc_lowcomm_request_t * )request.pointer_to_source_request;

	if(!src_req)
	{
		MPC_ERROR_SUCCESS()
	}

	struct _mpc_egreq_progress_work_unit *pwu = ( ( struct _mpc_egreq_progress_work_unit * )src_req->progress_unit);

	pwu->done = 1;

	/* We have to do this as request complete takes
	 *     a copy of the request ... but we want
	 *     to modify the original request which is being polled ... */
	src_req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
	MPC_ERROR_SUCCESS()
}

/************************************************************************/
/* Datatype Handling                                                    */
/************************************************************************/

/* Utilities functions for this section are located in mpc_datatypes.[ch] */

/** \brief This fuction releases a datatype
 *
 *  This function call the right release function relatively to the
 *  datatype kind (contiguous or general) and then set the freed
 *  datatype to MPC_DATATYPE_NULL
 *
 */
int _mpc_cl_type_free(mpc_lowcomm_datatype_t *datatype_p)
{
	SCTK_PROFIL_START(MPC_Type_free);
	/* Dereference fo convenience */
	mpc_lowcomm_datatype_t datatype = *datatype_p;
	/* Retrieve task context */

	if(datatype == MPC_DATATYPE_NULL || datatype == MPC_LB ||
	   datatype == MPC_UB)
	{
		return MPC_ERR_TYPE;
	}

	/* Is the datatype PACKED ? */
	if(datatype == MPC_LOWCOMM_PACKED)
	{
		/* ERROR */
		SCTK_PROFIL_END(MPC_Type_free);
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE,
		                 "You tried to free an MPI_PACKED datatype");
	}

	/* Choose what to do in function of the datatype kind */
	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			/* You are not supposed to free predefined datatypes */
			SCTK_PROFIL_END(MPC_Type_free);
			MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE,
			                 "You are not allowed to free a COMMON datatype");
			break;

		case MPC_DATATYPES_USER:
			/* Free a general datatype */
			/* Check if it is really allocated */
			if(datatype->ref_count == 0)
			{
				/* ERROR */
				MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG,
				                 "Tried to release an uninitialized datatype");
			}

			mpc_common_nodebug("Free general [%d] contructor %d", datatype,
			                   general_type_target->context.combiner);

			/* Free it */
			_mpc_dt_general_release(datatype_p);

			break;

		case MPC_DATATYPES_UNKNOWN:
			/* If we are here the type provided must have been erroneous */
			return MPC_ERR_TYPE;

			/* MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, */
			/*                  "An unknown datatype was provided"); */
			break;
	}

	SCTK_PROFIL_END(MPC_Type_free);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_set_name(mpc_lowcomm_datatype_t datatype, const char *name)
{
	if(_mpc_dt_name_set(datatype, name) )
	{
		MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_INTERN, "Could not set name");
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_dup(mpc_lowcomm_datatype_t old_type, mpc_lowcomm_datatype_t *new_type)
{
	/* Set type context */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner = MPC_COMBINER_DUP;
	dtctx.oldtype  = old_type;

	switch(_mpc_dt_get_kind(old_type) )
	{
		case MPC_DATATYPES_COMMON:
			_mpc_cl_type_hcontiguous_ctx(new_type, 1, &old_type, &dtctx);
			break;

		case MPC_DATATYPES_USER:
			_mpc_cl_general_datatype(
				new_type, old_type->begins, old_type->ends,
				old_type->datatypes, old_type->count,
				old_type->lb, old_type->is_lb,
				old_type->ub, old_type->is_ub, &dtctx);
			break;

		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_ARG, "Bad data-type");
			break;
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_get_name(mpc_lowcomm_datatype_t datatype, char *name, int *resultlen)
{
	char *retname = NULL;

	if(mpc_lowcomm_datatype_is_common(datatype) )
	{
		retname = mpc_lowcomm_datatype_common_get_name(datatype);
	}
	else
	{
		retname = _mpc_dt_name_get(datatype);
	}


	if(datatype == MPC_UB)
	{
		retname = "MPI_UB";
	}
	else if(datatype == MPC_LB)
	{
		retname = "MPI_LB";
	}

	if(!retname)
	{
		/* Return an empty string */
		name[0]    = '\0';
		*resultlen = 0;
	}
	else
	{
		(void)sprintf(name, "%s", retname);
		*resultlen = (int)strlen(name);
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_get_envelope(mpc_lowcomm_datatype_t datatype, int *num_integers,
                              int *num_addresses, int *num_datatypes,
                              int *combiner)
{
	*num_integers  = 0;
	*num_addresses = 0;
	*num_datatypes = 0;

	/* Handle the common data-type case */
	if(mpc_lowcomm_datatype_is_common(datatype) ||
	   _mpc_dt_is_boundary(datatype) )
	{
		*combiner      = MPC_COMBINER_NAMED;
		*num_integers  = 0;
		*num_addresses = 0;
		*num_datatypes = 0;
		MPC_ERROR_SUCCESS();
	}

	struct _mpc_dt_footprint *dctx = __mpc_cl_datatype_get_ctx(datatype);

	if(!dctx)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
		                 "This datatype is unknown");
	}

	_mpc_dt_fill_envelope(dctx, num_integers, num_addresses, num_datatypes,
	                      combiner);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_get_contents(mpc_lowcomm_datatype_t datatype, const int max_integers,
                              const int max_addresses, const int max_datatypes,
                              int array_of_integers[],
                              ssize_t array_of_addresses[],
                              mpc_lowcomm_datatype_t array_of_datatypes[])
{
	/* First make sure we were not called on a common type */
	if(mpc_lowcomm_datatype_is_common(datatype) )
	{
		MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_ARG,
		                 "Cannot call MPI_Type_get_contents on a defaut datatype");
	}

	/* Retrieve the context */
	struct _mpc_dt_footprint *dctx = __mpc_cl_datatype_get_ctx(datatype);

	assume(dctx != NULL);

	/* Then make sure the user actually called MPI_Get_envelope
	 * with the same datatype or that we have enough room */
	int n_int  = 0;
	int n_addr = 0;
	int n_type = 0;
	int dummy_combiner = 0;

	_mpc_dt_fill_envelope(dctx, &n_int, &n_addr, &n_type, &dummy_combiner);

	if( (max_integers < n_int) || (max_addresses < n_addr) ||
	    (max_datatypes < n_type) )
	{
		/* Not enough room */
		MPC_ERROR_REPORT(
			MPC_COMM_SELF, MPC_ERR_ARG,
			"Cannot call MPI_Type_get_contents as it would overflow target arrays");
	}

	/* Now we just copy back the content from the context */
	if(array_of_integers)
	{
		memcpy(array_of_integers, dctx->array_of_integers, n_int * sizeof(int) );
	}

	if(array_of_addresses)
	{
		memcpy(array_of_addresses, dctx->array_of_addresses,
		       n_addr * sizeof(size_t) );
	}

	/* Flag the datatypes as duplicated */
	int i = 0;

	for(i = 0; i < n_type; i++)
	{
		_mpc_cl_type_use(dctx->array_of_types[i]);
	}

	if(array_of_datatypes)
	{
		memcpy(array_of_datatypes, dctx->array_of_types,
		       n_type * sizeof(mpc_lowcomm_datatype_t) );
	}

	MPC_ERROR_SUCCESS();
}

/** \brief Retrieves datatype size
 *
 *  Returns the right size field in function of datype kind
 *
 *  \param datatype datatype which size is requested
 *
 *  \return the size of the datatype or aborts
 */
static inline size_t __mpc_cl_datatype_get_size(mpc_lowcomm_datatype_t datatype)
{
	/* Exeptions */
	if(datatype == MPC_DATATYPE_NULL || datatype == MPC_UB || datatype == MPC_LB)
	{
		return 0;
	}
	if(datatype == MPC_LOWCOMM_PACKED)
	{
		return 1;
	}

	/* Common types bypass */
	if(datatype == MPC_LOWCOMM_BYTE)
	{
		return sizeof(char);
	}
	if(datatype == MPC_LOWCOMM_DOUBLE)
	{
		return sizeof(double);
	}
	if(datatype == MPC_LOWCOMM_FLOAT)
	{
		return sizeof(float);
	}
	if(datatype == MPC_LOWCOMM_INT)
	{
		return sizeof(int);
	}

	size_t ret = 0;

	/* Compute the size in function of the datatype */
	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			/* Common datatype sizes */
			return mpc_lowcomm_datatype_common_get_size(datatype);

			break;

		case MPC_DATATYPES_USER:
			/* User datatype size */

			/* Is it initialized ? */
			if(_MPC_DT_USER_IS_FREE(datatype) )
			{
				/* ERROR */
				mpc_common_debug_fatal("Tried to retrieve an uninitialized datatype %d", datatype);
			}

			if(datatype->is_a_padded_struct)
			{
				/* Here we return UB as the size (padded struct) */
				ret = datatype->ub;
			}
			else
			{
				/* Extract the size field */
				ret = datatype->size;
			}

			/* Return */
			return ret;

			break;

		case MPC_DATATYPES_UNKNOWN:
			/* No such datatype */
			/* ERROR */
			MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE,
			                 "An unknown datatype was provided");
			break;
	}

	/* We shall never arrive here ! */
	mpc_common_debug_fatal("This error shall never be reached");
	return MPC_ERR_INTERN;
}

int _mpc_cl_type_get_true_extent(mpc_lowcomm_datatype_t datatype, long *true_lb,
                                 long *true_extent)
{
	long tmp_true_lb = 0;
	long tmp_true_ub = 0;

	/* Special cases */
	if(datatype == MPC_DATATYPE_NULL)
	{
		/* Here we return 0 for data-type null
		 * in order to pass the struct-zero-count test */
		*true_lb     = 0;
		*true_extent = 0;
		MPC_ERROR_SUCCESS();
	}

	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			tmp_true_lb = 0;
			tmp_true_ub = (long)__mpc_cl_datatype_get_size(datatype);
			break;

		case MPC_DATATYPES_USER:
			_mpc_dt_general_true_extend(datatype, &tmp_true_lb,
			                            &tmp_true_ub);
			break;

		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT(MPC_COMM_SELF, MPC_ERR_ARG, "Bad data-type");
			break;
	}

	*true_lb     = tmp_true_lb;
	*true_extent = (tmp_true_ub - tmp_true_lb) + 1;
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_is_allocated(mpc_lowcomm_datatype_t datatype, bool *flag)
{
	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			*flag = 1;
			break;

		case MPC_DATATYPES_USER:
			*flag = _mpc_dt_storage_type_can_be_released(datatype);
			break;

		default:
			*flag = 0;
			break;
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_flag_padded(mpc_lowcomm_datatype_t datatype)
{
	mpc_lowcomm_datatype_t type = _mpc_dt_get_datatype(datatype);

	if(type != MPC_LOWCOMM_DATATYPE_NULL)
	{
		type->is_a_padded_struct = true;

		MPC_ERROR_SUCCESS();
	}

	return MPC_ERR_ARG;
}

int _mpc_cl_type_size(mpc_lowcomm_datatype_t datatype, size_t *size)
{
	SCTK_PROFIL_START(MPC_Type_size);
	*size = __mpc_cl_datatype_get_size(datatype);
	SCTK_PROFIL_END(MPC_Type_size);
	MPC_ERROR_SUCCESS();
}

bool __mpc_cl_type_general_check_context(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
                                         struct _mpc_dt_context *ctx,
                                         mpc_lowcomm_datatype_t *datatype)
{
	unsigned int i = 0;

	/* try to find a data-type with the same footprint in general  */
	for(i = 0; i < SCTK_USER_DATA_TYPES_MAX; i++)
	{
		/* For each datatype */
		_mpc_lowcomm_general_datatype_t *current_user_type =
			_mpc_cl_per_mpi_process_ctx_general_datatype_ts_get(task_specific, i);

		/* Is the slot NOT free ? */
		if(!_MPC_DT_USER_IS_FREE(current_user_type) )
		{
			/* If types match */
			if(_mpc_dt_footprint_match(ctx, current_user_type->context) )
			{
				/* Add a reference to this data-type and we are done */
				_mpc_cl_type_use(current_user_type);
				*datatype = current_user_type;
				return true;
			}
		}
	}

	return false;
}

bool __mpc_cl_type_general_check_footprint(mpc_mpi_cl_per_mpi_process_ctx_t *task_specific,
                                           struct _mpc_dt_footprint *ref,
                                           mpc_lowcomm_datatype_t *datatype)
{
	unsigned int i = 0;

	/* try to find a data-type with the same footprint in general  */
	for(i = 0; i < SCTK_USER_DATA_TYPES_MAX; i++)
	{
		/* For each datatype */
		_mpc_lowcomm_general_datatype_t *current_user_type =
			_mpc_cl_per_mpi_process_ctx_general_datatype_ts_get(task_specific, i);

		/* Is the slot NOT free ? */
		if(_MPC_DT_USER_IS_USED(current_user_type) )
		{
			/* If types match */
			if(_mpc_dt_footprint_check_envelope(ref, current_user_type->context) )
			{
				/* Add a reference to this data-type and we are done */
				_mpc_cl_type_use(current_user_type);
				*datatype = current_user_type;
				return true;
			}
		}
	}

	return false;
}

int _mpc_cl_type_hcontiguous_ctx(mpc_lowcomm_datatype_t *datatype, size_t count,
                                 mpc_lowcomm_datatype_t *data_in,
                                 struct _mpc_dt_context *ctx)
{
	SCTK_PROFIL_START(MPC_Type_hcontiguous);

	size_t size = 0;

	_mpc_cl_type_size(*data_in, &size);

	int ierr = MPC_LOWCOMM_SUCCESS;

	_mpc_dt_contiguous_create(datatype, 0, size, count, *data_in);

	if(*datatype == MPC_LOWCOMM_DATATYPE_NULL)
	{
		SCTK_PROFIL_END(MPC_Type_hcontiguous);
		return MPC_ERR_INTERN;
	}

	_mpc_cl_type_ctx_set(*datatype, ctx);

	ierr = _mpc_dt_general_on_slot(datatype);

	SCTK_PROFIL_END(MPC_Type_hcontiguous);
	return ierr;
}

/** \brief This function is the generic initializer for
 * _mpc_dt_contiguous_t
 *  Creates a contiguous datatypes of count data_in
 *
 *  \param datatype Output datatype to be created
 *  \param count Number of entries of type data_in
 *  \param data_in Type of the entry to be created
 *
 */
int _mpc_cl_type_hcontiguous(mpc_lowcomm_datatype_t *datatype, size_t count,
                             mpc_lowcomm_datatype_t *data_in)
{
	/* HERE WE SET A DEFAULT CONTEXT */
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner = MPC_COMBINER_CONTIGUOUS;
	dtctx.count    = (int)count;
	dtctx.oldtype  = *data_in;
	return _mpc_cl_type_hcontiguous_ctx(datatype, count, data_in, &dtctx);
}

int _mpc_cl_type_debug(mpc_lowcomm_datatype_t datatype)
{
	/* mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get(); */
	/* _mpc_lowcomm_general_datatype_t *   target_general_type; */
	/* _mpc_dt_contiguous_t *contiguous_type; */

	if(datatype == MPC_DATATYPE_NULL)
	{
		mpc_common_debug_error("=============ERROR==================");
		mpc_common_debug_error("MPC_DATATYPE_NULL");
		mpc_common_debug_error("====================================");
		MPC_ERROR_SUCCESS();
	}

	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			mpc_lowcomm_datatype_common_display(datatype);
			break;

		/* case MPC_DATATYPES_CONTIGUOUS: */
		/* 	contiguous_type = */
		/* 	        _mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get(task_specific, datatype); */
		/* 	_mpc_dt_contiguous_display(contiguous_type); */
		/* 	break; */

		case MPC_DATATYPES_USER:
			/* target_general_type = */
			/* _mpc_cl_per_mpi_process_ctx_general_datatype_ts_get(task_specific, datatype); */
			_mpc_dt_general_display(datatype);
			break;

		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
			                 "This datatype is unknown");
			break;
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_commit(mpc_lowcomm_datatype_t *datatype_p)
{
	/* Dereference for convenience */
	mpc_lowcomm_datatype_t datatype = *datatype_p;

	int ierr = MPC_LOWCOMM_SUCCESS;

	/* Error cases */
	if(datatype == MPC_DATATYPE_NULL || datatype == MPC_LB || datatype == MPC_UB)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE,
		                 "You are trying to commit a NULL data-type");
	}
	if(!mpc_dt_is_valid(datatype) )
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE,
		                 "You are trying to commit a non created datatype");
	}

	/* OPTIMIZE */
	ierr = _mpc_dt_general_optimize(*datatype_p);

	/* Commit the datatype */
	(*datatype_p)->is_commited = true;

	return ierr;
}

/** \brief This function increases the refcounter for a datatype
 *
 *  \param datatype Target datatype
 *
 *  \warning We take no lock here thus the datatype lock shall be held
 *  when manipulating such objects
 */
int _mpc_cl_type_use(mpc_lowcomm_datatype_t datatype)
{
	if(_mpc_dt_is_boundary(datatype) )
	{
		MPC_ERROR_SUCCESS();
	}

	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			/* Nothing to do here */
			MPC_ERROR_SUCCESS();
			break;

		case MPC_DATATYPES_USER:
			/* Increment the refcounter */
			datatype->ref_count++;
			mpc_common_nodebug("Type %d Refcount %d", datatype->id,
			                   datatype->ref_count);
			break;

		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
			                 "This datatype is unknown");
			break;
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_general_datatype(mpc_lowcomm_datatype_t *datatype,
                             const long *const begins,
                             const long *const ends,
                             const mpc_lowcomm_datatype_t *const types, const unsigned long count,
                             const long lb, const bool is_lb,
                             const long ub, const bool is_ub,
                             struct _mpc_dt_context *ectx)
{
	SCTK_PROFIL_START(MPC_Create_datatype);

	int ierr = MPC_LOWCOMM_SUCCESS;

	*datatype = _mpc_dt_general_create(0, count, begins, ends, types, lb, is_lb, ub, is_ub);

	if(*datatype == MPC_LOWCOMM_DATATYPE_NULL)
	{
		SCTK_PROFIL_END(MPC_Create_datatype);
		return MPC_ERR_INTERN;
	}

	_mpc_dt_context_set( (*datatype)->context, ectx);
	/* Save it in the task array */
	ierr = _mpc_dt_general_on_slot(datatype);

	SCTK_PROFIL_END(MPC_Create_datatype);
	return ierr;
}

/** \brief Extract general datatype informations
 *  On non general datatypes res is set to 0
 *  and datatype contains garbage
 *
 *  \param datatype Datatype to analyze
 *  \param res set to 0 if not a general datatype, 1 if general
 *  \param output_datatype A general datatype filled with all the informations
 *
 */
int _mpc_cl_general_datatype_try_get_info(mpc_lowcomm_datatype_t datatype, int *res,
                                          _mpc_lowcomm_general_datatype_t *output_datatype)
{
	/* Initialize output argument  assuming it is not a general datatype */
	*res = 0;
	memset(output_datatype, 0, sizeof(_mpc_lowcomm_general_datatype_t) );
	output_datatype->count = 1;

	/* Check whether the datatype ID falls in the general range ID */
	if(!mpc_dt_is_valid(datatype) )
	{
		return MPC_ERR_TYPE;
	}

	datatype = _mpc_dt_get_datatype(datatype);
	bool is_allocated = false;

	_mpc_cl_type_is_allocated(datatype, &is_allocated);
	if(datatype != MPC_LOWCOMM_DATATYPE_NULL && is_allocated)
	{
		mpc_common_nodebug("datatype(%d) - "
		                   "SCTK_USER_DATA_TYPES_MAX(%d) ==  %d-%d-%d == %p",
		                   datatype->id,
		                   SCTK_USER_DATA_TYPES_MAX, datatype->id,
		                   SCTK_USER_DATA_TYPES_MAX,
		                   datatype);
		/* Retrieve the datatype from the array */
		_mpc_lowcomm_general_datatype_t *target_type = datatype;

		if(!target_type)
		{
			MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG,
			                 "Failed to retrieve this general datatype");
		}

		/* We have found a general datatype */
		*res = 1;
		/* Copy its content to out arguments */
		memcpy(output_datatype, target_type, sizeof(_mpc_lowcomm_general_datatype_t) );
	}
	else
	{
		return MPC_ERR_TYPE;
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_type_ctx_set(mpc_lowcomm_datatype_t datatype,
                         struct _mpc_dt_context *dctx)
{
	/* Check if the handle is null */
	if(datatype == MPC_LOWCOMM_DATATYPE_NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE, "NULL datatype handle provided");
	}

	switch(_mpc_dt_get_kind(datatype) )
	{
		case MPC_DATATYPES_COMMON:
			/* Nothing to do here */
			MPC_ERROR_SUCCESS();
			break;

		default:
			/* Check if the datatype is created */
			if(_MPC_DT_USER_IS_FREE(datatype) )
			{
				MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_TYPE, "Unknown datatype provided");
			}

			/* Save the context */
			_mpc_dt_context_set(datatype->context, dctx);
			break;
	}

	MPC_ERROR_SUCCESS();
}

bool mpc_mpi_cl_type_is_contiguous(mpc_lowcomm_datatype_t type)
{
	/* UB or LB are not contiguous */
	if(_mpc_dt_is_boundary(type) || type == MPC_DATATYPE_NULL)
	{
		return false;
	}

	switch(_mpc_dt_get_kind(type) )
	{
		case MPC_DATATYPES_COMMON:
			return true; break;

		case MPC_DATATYPES_USER:
			/* If there is no block (0 size) or one block in the optimized representation
			 * then this data-type is contiguous */
			if( (type->count == 0) || (type->opt_count == 1) )
			{
				return 1;
			}
			break;

		case MPC_DATATYPES_UNKNOWN:
			return false; break;
	}

	return false;
}

/* Datatype  Attribute Handling                                         */

/* KEYVALS */

int _mpc_cl_type_create_keyval(MPC_Type_copy_attr_function *copy,
                               MPC_Type_delete_attr_function *delete,
                               int *type_keyval, void *extra_state)
{
	return _mpc_dt_keyval_create(copy, delete, type_keyval, extra_state);
}

int _mpc_cl_type_free_keyval(int *type_keyval)
{
	return _mpc_dt_keyval_free(type_keyval);
}

/* ATTRS */

int _mpc_cl_type_get_attr(mpc_lowcomm_datatype_t datatype, int type_keyval,
                          void **attribute_val, int *flag)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	struct _mpc_dt_storage *          da            = task_specific->datatype_array;

	if(!da)
	{
		return MPC_ERR_INTERN;
	}

	return _mpc_dt_attr_get(da, datatype, type_keyval, attribute_val, flag);
}

int _mpc_cl_type_set_attr(mpc_lowcomm_datatype_t datatype, int type_keyval,
                          void *attribute_val)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	struct _mpc_dt_storage *          da            = task_specific->datatype_array;

	if(!da)
	{
		return MPC_ERR_INTERN;
	}

	return _mpc_dt_attr_set(da, datatype, type_keyval, attribute_val);
}

int _mpc_cl_type_delete_attr(mpc_lowcomm_datatype_t datatype, int type_keyval)
{
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	struct _mpc_dt_storage *          da            = task_specific->datatype_array;

	if(!da)
	{
		return MPC_ERR_INTERN;
	}

	return _mpc_dt_attr_delete(da, datatype, type_keyval);
}

/** \brief This struct is used to serialize these boundaries all
 * at once instead of having to do it one by one (see two next functions) */
struct inner_lbub
{
	long lb;                 /**< Lower bound offset  */
	int  is_lb;              /**< Does type has a lower bound */
	long ub;                 /**< Upper bound offset */
	int  is_ub;              /**< Does type has an upper bound */
	int  is_a_padded_struct; /**< Was the type padded with UB during construction ?
	                          */
};

#define GUARD_MAGICK 0x77

void *_mpc_cl_general_type_serialize(mpc_lowcomm_datatype_t type, size_t *size,
                                     const size_t header_pad)
{
	assume(mpc_dt_is_valid(type) );
	assume(size != NULL);

	if(_mpc_dt_is_user_defined(type) )
	{
		_mpc_cl_per_mpi_process_ctx_datatype_lock();
	}
	else
	{
		type = _mpc_dt_get_datatype(type);
	}
	_mpc_lowcomm_general_datatype_t *dtype = type;

	assume(dtype != NULL);
	*size = sizeof(size_t) +
	        2 * dtype->count * sizeof(long) +
	        dtype->count * sizeof(mpc_lowcomm_datatype_t) + sizeof(struct inner_lbub) +
	        header_pad + 1;
	void *ret = sctk_malloc(*size);

	if(!ret)
	{
		perror("malloc");
		return NULL;
	}

	ssize_t *count  = ( ssize_t * )(ret + header_pad);
	long *   begins = (count + 1);
	long *   ends   = begins + dtype->count;
	mpc_lowcomm_datatype_t *types = ( mpc_lowcomm_datatype_t * )(ends + dtype->count);
	struct inner_lbub *     lb_ub = ( struct inner_lbub * )(types + dtype->count);
	char *guard = ( ( char * )lb_ub + 1);

	*guard = GUARD_MAGICK;
	*count = (int)dtype->count;
	size_t i = 0;

	for(i = 0; i < dtype->count; i++)
	{
		begins[i] = dtype->begins[i];
		ends[i]   = dtype->ends[i];
		types[i]  = dtype->datatypes[i];
	}

	lb_ub->lb    = dtype->lb;
	lb_ub->is_lb = dtype->is_lb;
	lb_ub->ub    = dtype->ub;
	lb_ub->is_ub = dtype->is_ub;
	lb_ub->is_a_padded_struct = dtype->is_a_padded_struct;
	if(_mpc_dt_is_user_defined(type) )
	{
		_mpc_cl_per_mpi_process_ctx_datatype_unlock();
	}
	assert(*guard == GUARD_MAGICK);
	assert(guard < ( char * )(ret + *size) );
	return ret;
}

mpc_lowcomm_datatype_t _mpc_cl_general_type_deserialize(void *buff, size_t size,
                                                        size_t header_pad)
{
	ssize_t *count  = ( ssize_t * )(buff + header_pad);
	long *   begins = (count + 1);
	long *   ends   = begins + *count;
	mpc_lowcomm_datatype_t *types = ( mpc_lowcomm_datatype_t * )(ends + *count);
	struct inner_lbub *     lb_ub = ( struct inner_lbub * )(types + *count);

	assume(count < ( ssize_t * )(buff + size) );
	assume(count < ( ssize_t * )(begins + size) );
	assume(count < ( ssize_t * )(ends + size) );
	assume(count < ( ssize_t * )(types + size) );
	mpc_lowcomm_datatype_t ret = NULL;
	struct _mpc_dt_context dtctx;

	_mpc_dt_context_clear(&dtctx);
	dtctx.combiner = MPC_COMBINER_DUMMY;
	_mpc_cl_general_datatype(&ret, begins, ends, types, *count, lb_ub->lb,
	                         lb_ub->is_lb, lb_ub->ub, lb_ub->is_ub, &dtctx);

	if(lb_ub->is_a_padded_struct)
	{
		_mpc_cl_type_flag_padded(ret);
	}

	_mpc_cl_type_commit(&ret);
	return ret;
}

mpc_lowcomm_datatype_t _mpc_cl_type_get_inner(mpc_lowcomm_datatype_t type)
{
	if(mpc_lowcomm_datatype_is_common(type) )
	{
		type = _mpc_dt_get_datatype(type);
		return type;
	}

	if(_mpc_dt_is_contig_mem(type) )
	{
		_mpc_lowcomm_general_datatype_t *ctype = type;
		assume(ctype != NULL);
		return ctype->datatypes[0];
	}

	_mpc_lowcomm_general_datatype_t *dtype = type;

	assume(dtype != NULL);
	// _mpc_dt_general_display( dtype );
	mpc_lowcomm_datatype_t itype = MPC_DATATYPE_NULL;
	size_t i = 0;

	for(i = 0; i < dtype->count; i++)
	{
		if(itype == MPC_DATATYPE_NULL)
		{
			itype = dtype->datatypes[i];
		}
		else
		{
			if(itype != dtype->datatypes[i])
			{
				return MPC_DATATYPE_NULL;
			}
		}
	}

	assume(mpc_lowcomm_datatype_is_common(itype) );
	/* if we are here, all types are the same */
	return itype;
}

/************************************************************************/
/* MPC Init and Finalize                                                */
/************************************************************************/


int _mpc_cl_abort(__UNUSED__ mpc_lowcomm_communicator_t comm, int errorcode)
{
	mpc_common_debug_error("MPC_Abort with error %d", errorcode);
	(void)fflush(stderr);
	(void)fflush(stdout);
	mpc_common_debug_abort();
	MPC_ERROR_SUCCESS();
}

int mpc_mpi_cl_get_activity(int nb_item, mpc_mpi_cl_activity_t *tab, double *process_act)
{
	int    i = 0;
	int    nb_proc = 0;
	double proc_act = 0;

	nb_proc = mpc_topology_get_pu_count();
	mpc_common_nodebug("Activity %d/%d:", nb_item, nb_proc);

	for(i = 0; (i < nb_item) && (i < nb_proc); i++)
	{
		double tmp = NAN;
		tab[i].virtual_cpuid = i;
		tmp          = mpc_thread_getactivity(i);
		tab[i].usage = tmp;
		proc_act    += tmp;
		mpc_common_nodebug("\t- cpu %d activity %f\n", tab[i].virtual_cpuid,
		                   tab[i].usage);
	}

	for(; i < nb_item; i++)
	{
		tab[i].virtual_cpuid = -1;
		tab[i].usage         = -1;
	}

	proc_act     = proc_act / ( ( double )nb_proc);
	*process_act = proc_act;
	MPC_ERROR_SUCCESS();
}

/****************************
* MPC MESSAGE PASSING MAIN *
****************************/
/** \brief Function used to create a temporary run directory
 */
static inline void __mpc_cl_enter_tmp_directory()
{
#define TMP_CURRENT_SIZE 500
#define TMP_SIZE 1000

	char *do_move_to_temp = getenv("MPC_MOVE_TO_TEMP");

	if(do_move_to_temp == NULL)
	{
		/* Nothing to do */
		return;
	}

	char *cret = NULL;

	/* Retrieve task rank */
	int  rank = mpc_common_get_task_rank();
	char tmpdir[TMP_SIZE];

	/* If root rank create the temp dir */
	if(rank == 0)
	{
		char currentdir[TMP_CURRENT_SIZE];
		// First enter a sandbox DIR
		cret = getcwd(currentdir, TMP_CURRENT_SIZE);
		if(cret)
		{
			(void)snprintf(tmpdir, TMP_SIZE, "%s/XXXXXX", currentdir);
			cret = mkdtemp(tmpdir);
			if(cret)
			{
				mpc_common_debug_warning("Creating temp directory %s", tmpdir);
			}
			else
			{
				mpc_common_debug_error("Failed to create a tmd directory");
			}
		}
	}

	/* Broadcast the path to all tasks */
	mpc_lowcomm_bcast( ( void * )tmpdir, TMP_SIZE * sizeof(char), 0, MPC_COMM_WORLD);

	/* Only the root of each process does the chdir */
	if(mpc_common_get_local_task_rank() == 0)
	{
		int ret = chdir(tmpdir);

		if(ret < 0)
		{
			mpc_common_debug_warning("Failed entering temporary directory");
		}
	}
}

/************************************************************************/
/* Topology Informations                                                */
/************************************************************************/

int _mpc_cl_comm_rank(mpc_lowcomm_communicator_t comm, int *rank)
{
	SCTK_PROFIL_START(MPC_Comm_rank);

	*rank = mpc_lowcomm_communicator_rank(comm);

	if(*rank == MPC_PROC_NULL)
	{
		*rank = MPI_UNDEFINED;
	}

	SCTK_PROFIL_END(MPC_Comm_rank);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_comm_size(mpc_lowcomm_communicator_t comm, int *size)
{
	SCTK_PROFIL_START(MPC_Comm_size);
	*size = mpc_lowcomm_get_comm_size(comm);
	SCTK_PROFIL_END(MPC_Comm_size);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_comm_remote_size(mpc_lowcomm_communicator_t comm, int *size)
{
	// SCTK_PROFIL_START (MPC_Comm_remote_size);
	*size = mpc_lowcomm_communicator_remote_size(comm);
	// SCTK_PROFIL_END (MPC_Comm_remote_size);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_get_processor_name(char *name, int *resultlen)
{
	SCTK_PROFIL_START(MPC_Get_processor_name);
	gethostname(name, MPI_MAX_PROCESSOR_NAME);
	*resultlen = (int)strlen(name);
	SCTK_PROFIL_END(MPC_Get_processor_name);
	MPC_ERROR_SUCCESS();
}

/************************************************************************/
/* Point to point communications                                        */
/************************************************************************/


int _mpc_cl_isend(const void *buf, mpc_lowcomm_msg_count_t count,
                  mpc_lowcomm_datatype_t datatype, int dest, int tag,
                  mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request)
{

	if(dest == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(request, comm, REQUEST_SEND);
		MPC_ERROR_SUCCESS();
	}

	size_t d_size   = __mpc_cl_datatype_get_size(datatype);
	size_t msg_size = count * d_size;

	mpc_lowcomm_isend(dest, buf, msg_size, tag, comm, request);

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_issend(const void *buf, mpc_lowcomm_msg_count_t count,
                   mpc_lowcomm_datatype_t datatype, int dest, int tag,
                   mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request)
{
	/* Issend is not buffered */
	if(dest == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(request, comm, REQUEST_SEND);
		MPC_ERROR_SUCCESS();
	}

	size_t d_size   = __mpc_cl_datatype_get_size(datatype);
	size_t msg_size = count * d_size;

	mpc_lowcomm_isend(dest, buf, msg_size, tag, comm, request);

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_ibsend(const void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype, int dest,
                   int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request)
{
	return _mpc_cl_isend(buf, count, datatype, dest, tag, comm, request);
}

int _mpc_cl_irsend(const void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype, int dest,
                   int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request)
{
	return _mpc_cl_isend(buf, count, datatype, dest, tag, comm, request);
}

int _mpc_cl_irecv(void *buf, mpc_lowcomm_msg_count_t count,
                  mpc_lowcomm_datatype_t datatype, int source, int tag,
                  mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request)
{

	if(source == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(request, comm, REQUEST_RECV);
		MPC_ERROR_SUCCESS();
	}

	size_t d_size = __mpc_cl_datatype_get_size(datatype);

	mpc_lowcomm_irecv(source, buf, count * d_size, tag, comm, request);

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_ssend(const void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype,
                  int dest, int tag, mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_request_t request;

	if(dest == MPC_PROC_NULL)
	{
		MPC_ERROR_SUCCESS();
	}

	size_t msg_size = count * __mpc_cl_datatype_get_size(datatype);

	mpc_lowcomm_ssend(dest, buf, msg_size, tag, comm, &request);
	mpc_lowcomm_request_wait(&request);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_send(const void *buf, mpc_lowcomm_msg_count_t count,
                 mpc_lowcomm_datatype_t datatype, int dest, int tag, mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_request_t request;

	if(dest == MPC_PROC_NULL)
	{
		MPC_ERROR_SUCCESS();
	}

	size_t msg_size = count * __mpc_cl_datatype_get_size(datatype);

	mpc_lowcomm_isend(dest, buf, msg_size, tag, comm, &request);
	mpc_lowcomm_request_wait(&request);

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_recv(void *buf, mpc_lowcomm_msg_count_t count, mpc_lowcomm_datatype_t datatype, int source,
                 int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_status_t *pstatus)
{
	mpc_lowcomm_status_t  __status;
	mpc_lowcomm_status_t *status = pstatus;

	if(status == MPI_STATUS_IGNORE)
	{
		status = &__status;
	}

	status->MPC_ERROR = MPC_LOWCOMM_SUCCESS;

	if(source == MPC_PROC_NULL)
	{
		if(status != MPC_LOWCOMM_STATUS_NULL)
		{
			status->MPC_SOURCE = MPC_PROC_NULL;
			status->MPC_TAG    = MPC_ANY_TAG;
			status->size       = 0;
		}

		MPC_ERROR_SUCCESS();
	}

	mpc_lowcomm_request_t req;

	int ret = _mpc_cl_irecv(buf, count, datatype, source, tag, comm, &req);

	if(ret != MPI_SUCCESS)
	{
		return ret;
	}

	_mpc_cl_waitall(1, &req, status);

	if(status->MPI_ERROR != MPI_SUCCESS)
	{
		return status->MPI_ERROR;
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_sendrecv(void *sendbuf, mpc_lowcomm_msg_count_t sendcount, mpc_lowcomm_datatype_t sendtype,
                     int dest, int sendtag, void *recvbuf, mpc_lowcomm_msg_count_t recvcount,
                     mpc_lowcomm_datatype_t recvtype, int source, int recvtag, mpc_lowcomm_communicator_t comm,
                     mpc_lowcomm_status_t *status)
{
	mpc_lowcomm_request_t reqs[2];

	reqs[0] = MPC_REQUEST_NULL;
	reqs[1] = MPC_REQUEST_NULL;

	//mpc_common_debug_error("SEND %p CNT %d DEST %d TAG %d", sendbuf, sendcount, dest, sendtag);
	//mpc_common_debug_error("RECV %p CNT %d SRC %d TAG %d", recvbuf, recvcount, source, recvtag);

	_mpc_cl_irecv(recvbuf, recvcount, recvtype, source, recvtag, comm, &reqs[0]);
	_mpc_cl_isend(sendbuf, sendcount, sendtype, dest, sendtag, comm, &reqs[1]);

	MPI_Status st[2];

	_mpc_cl_waitall(2, reqs, st);

	if(status != MPC_LOWCOMM_STATUS_NULL)
	{
		memcpy(status, &st[0], sizeof(MPI_Status) );
	}

	if(st[0].MPI_ERROR != MPI_SUCCESS)
	{
		return st[0].MPI_ERROR;
	}

	if(st[1].MPI_ERROR != MPI_SUCCESS)
	{
		return st[1].MPI_ERROR;
	}

	MPC_ERROR_SUCCESS();
}

/****************
* MESSAGE WAIT *
****************/

static inline int __mpc_cl_test_light(mpc_lowcomm_request_t *request)
{
	if(mpc_lowcomm_request_get_completion(request) != MPC_LOWCOMM_MESSAGE_PENDING)
	{
		return 1;
	}

	__mpc_cl_request_progress(request);

	return mpc_lowcomm_request_get_completion(request);
}

struct wfv_waitall_s
{
	int                     ret;
	mpc_lowcomm_msg_count_t count;
	mpc_lowcomm_request_t **array_of_requests;
	mpc_lowcomm_status_t *  array_of_statuses;
};

static inline void wfv_waitall(void *arg)
{
	struct wfv_waitall_s *args = ( struct wfv_waitall_s * )arg;
	int flag = 1;
	int i = 0;

	for(i = 0; i < args->count; i++)
	{
		flag = flag & __mpc_cl_test_light(args->array_of_requests[i]);
	}

	/* All requests received */
	if(flag)
	{
		for(i = 0; i < args->count; i++)
		{
			mpc_lowcomm_request_t *request = NULL;
			request = args->array_of_requests[i];

			if(args->array_of_statuses != NULL)
			{
				mpc_lowcomm_status_t *status = &(args->array_of_statuses[i]);
				mpc_lowcomm_commit_status_from_request(request, status);
				mpc_common_nodebug("source %d\n", status->MPC_SOURCE);
				mpc_lowcomm_request_set_null(request, 1);
			}
		}

		args->ret = 1;
	}
}

/** \brief This function is used to perform a batch wait over ExGrequest classes
 * It relies on the wait_fn provided at \ref MPCX_GRequest_class_create
 * but in order to use it we must make sure that all the requests are of
 * the same type, to do so we check if they have the same wait_fn.
 *
 * We try not to be intrusive as we are in the waitall critical path
 * therefore we are progressivelly testing for our conditions
 */
static inline int _mpc_cl_waitall_Grequest(mpc_lowcomm_msg_count_t count,
                                           mpc_lowcomm_request_t *parray_of_requests[],
                                           mpc_lowcomm_status_t array_of_statuses[])
{
	int i = 0;
	int all_of_the_same_class       = 0;
	sctk_Grequest_wait_fn *ref_wait = NULL;

	/* Do we have at least two requests ? */
	if(2 < count)
	{
		/* Are we looking at generalized requests ? */
		if(parray_of_requests[0]->request_type == REQUEST_GENERALIZED)
		{
			ref_wait = parray_of_requests[0]->wait_fn;

			/* Are we playing with extended Grequest classes ? */
			if(ref_wait)
			{
				/* Does the first two match ? */
				if(ref_wait == parray_of_requests[1]->wait_fn)
				{
					/* Consider they all match now check the rest */
					all_of_the_same_class = 1;

					for(i = 0; i < count; i++)
					{
						/* Can we find a different one ? */
						if(parray_of_requests[i]->wait_fn != ref_wait)
						{
							all_of_the_same_class = 0;
							break;
						}
					}
				}
			}
		}
	}

	/* Yes we can perform the batch wait */
	if(all_of_the_same_class)
	{
		mpc_lowcomm_status_t tmp_status;
		/* Prepare the state array */
		void **array_of_states = sctk_malloc(sizeof(void *) * count);
		assume(array_of_states != NULL);

		for(i = 0; i < count; i++)
		{
			array_of_states[i] = parray_of_requests[i]->extra_state;
		}

		/* Call the batch wait function */

		/* Here the timeout parameter is not obvious as for example
		 *     ROMIO relies on Wtime which is not constrained by the norm
		 *         is is just monotonous. Whe should have a scaling function
		 *         which depends on the time source to be fair */
		( ref_wait )(count, array_of_states, 1e9, &tmp_status);
		sctk_free(array_of_states);

		/* Update the statuses */
		if(array_of_statuses != MPC_LOWCOMM_STATUS_NULL)
		{
			/* here we do a for loop as we only checked that the wait function
			 * was identical we are not sure that the XGrequests classes werent
			 * different ones */
			for(i = 0; i < count; i++)
			{
				(parray_of_requests[i]->query_fn)(parray_of_requests[i]->extra_state,
				                                  &array_of_statuses[i]);
			}
		}

		/* Free the requests */
		for(i = 0; i < count; i++)
		{
			(parray_of_requests[i]->free_fn)(parray_of_requests[i]->extra_state);
		}

		return 1;
	}

	return 0;
}

/** \brief Internal MPI_Waitall implementation relying on pointer to requests
 *
 *  This function is needed in order to call the MPC interface from
 *  the MPI one without having to rely on a set of MPI_Tests as before
 *  now the progress from MPI_Waitall functions can also be polled
 *
 *  \warning This function is not MPI_Waitall (see the mpc_lowcomm_request_t *
 * parray_of_requests[] argument)
 *
 */

int _mpc_cl_waitallp(mpc_lowcomm_msg_count_t count, mpc_lowcomm_request_t *parray_of_requests[],
                     mpc_lowcomm_status_t array_of_statuses[])
{
	int i = 0;

	/* We have to detect generalized requests as we are not able
	 * to progress them in the pooling function as we have no
	 * MPI context therefore we force a non-pooled progress
	 * when we have generalized requests*/
	int contains_generalized = 0;

	for(i = 0; i < count; i++)
	{
		if(parray_of_requests[i]->request_type == REQUEST_GENERALIZED)
		{
			contains_generalized |= 1;
			break;
		}
	}

	int direct_pass_count = 16;

	/* Can we do a batch wait ? */
	if(contains_generalized)
	{
		if(_mpc_cl_waitall_Grequest(count, parray_of_requests, array_of_statuses) )
		{
			MPC_ERROR_SUCCESS()
		}

		/* Prevent wfv polling as == 0 cannot occur */
		direct_pass_count = -1;
	}

	int flag = 0;

	/* Here we force the local polling because of generalized requests
	*      This will happen only if classical and generalized requests are
	*      mixed or if the wait_fn is not the same for all requests */
	do
	{
		flag = 1;

		for(i = 0; i < count; i++)
		{
			int tmp_flag = 0;
			mpc_lowcomm_status_t * status = NULL;
			mpc_lowcomm_request_t *request = NULL;

			if(array_of_statuses != NULL)
			{
				status = &(array_of_statuses[i]);
			}
			else
			{
				status = MPC_LOWCOMM_STATUS_NULL;
			}

			request = parray_of_requests[i];

			if(mpc_lowcomm_request_is_null(request) )
			{
				continue;
			}

			mpc_lowcomm_test(request, &tmp_flag, status);

			/* We set this flag in order to prevent the status
			 * from being updated repetitivelly in mpc_lowcomm_test */
			if(tmp_flag)
			{
				mpc_lowcomm_request_set_null(request, 1);
			}

			flag = flag & tmp_flag;
		}

		if(flag == 1)
		{
			MPC_ERROR_SUCCESS();
		}
		
					MPC_LOWCOMM_WORKSHARE_CHECK_CONFIG_AND_STEAL();
	

		direct_pass_count--;
	}while( (flag == 0) && (direct_pass_count != 0) );

	/* XXX: Waitall has been ported for using wait_for_value_and_poll
	 * because it provides better results than thread_yield.
	 * It performs well at least on Infiniband on NAS  */
	struct wfv_waitall_s wfv;

	wfv.ret = 0;
	wfv.array_of_requests = parray_of_requests;
	wfv.array_of_statuses = array_of_statuses;
	wfv.count             = count;
	mpc_lowcomm_perform_idle( &(wfv.ret), 1,
	                          (void (*)(void *) ) wfv_waitall, ( void * )&wfv);
	MPC_ERROR_SUCCESS();
}

#define PMPC_WAIT_ALL_STATIC_TRSH    32

int _mpc_cl_waitall(mpc_lowcomm_msg_count_t count, mpc_lowcomm_request_t array_of_requests[],
                    mpc_lowcomm_status_t array_of_statuses[])
{
	int res = 0;
	int i = 0;

	SCTK_PROFIL_START(MPC_Waitall);

	/* Here we are preparing the array of pointer to request
	 * in order to call the _mpc_cl_waitallp function
	 * it might be an extra cost but it allows the use of
	 * the _mpc_cl_waitallp function from MPI_Waitall */
	mpc_lowcomm_request_t **parray_of_requests = NULL;
	mpc_lowcomm_request_t * static_parray_of_requests[PMPC_WAIT_ALL_STATIC_TRSH];

	if(count < PMPC_WAIT_ALL_STATIC_TRSH)
	{
		parray_of_requests = static_parray_of_requests;
	}
	else
	{
		parray_of_requests = sctk_malloc(sizeof(mpc_lowcomm_request_t *) * count);
		assume(parray_of_requests != NULL);
	}

	/* Fill in the array of requests */
	for(i = 0; i < count; i++)
	{
		parray_of_requests[i] = &(array_of_requests[i]);
	}

	/* Call the pointer based waitall */
	res = _mpc_cl_waitallp(count, parray_of_requests, array_of_statuses);

	/* Do we need to free the temporary request pointer array */
	if(PMPC_WAIT_ALL_STATIC_TRSH <= count)
	{
		sctk_free(parray_of_requests);
	}

	SCTK_PROFIL_END(MPC_Waitall);
	return res;
}

int _mpc_cl_waitsome(mpc_lowcomm_msg_count_t incount, mpc_lowcomm_request_t array_of_requests[],
                     mpc_lowcomm_msg_count_t *outcount, mpc_lowcomm_msg_count_t array_of_indices[],
                     mpc_lowcomm_status_t array_of_statuses[])
{
	int i = 0;
	int done = 0;

	SCTK_PROFIL_START(MPC_Waitsome);

	while(done == 0)
	{
		for(i = 0; i < incount; i++)
		{
			if(!mpc_lowcomm_request_is_null(&(array_of_requests[i]) ) )
			{
				int tmp_flag = 0;
				mpc_lowcomm_test(&(array_of_requests[i]), &tmp_flag,
				                 &(array_of_statuses[done]) );

				if(tmp_flag)
				{
					mpc_lowcomm_request_set_null(&(array_of_requests[i]), 1);
					array_of_indices[done] = i;
					done++;
				}
			}
		}

		if(done == 0)
		{
			TODO("wait_for_value_and_poll should be used here")
			mpc_thread_yield();
		}
	}

	*outcount = done;
	SCTK_PROFIL_END(MPC_Waitsome);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_waitany(mpc_lowcomm_msg_count_t count, mpc_lowcomm_request_t array_of_requests[],
                    mpc_lowcomm_msg_count_t *index, mpc_lowcomm_status_t *status)
{
	int i = 0;

	SCTK_PROFIL_START(MPC_Waitany);
	*index = MPC_UNDEFINED;

	while(1)
	{
		for(i = 0; i < count; i++)
		{
			if(mpc_lowcomm_request_is_null(&(array_of_requests[i]) ) != 1)
			{
				int tmp_flag = 0;
				mpc_lowcomm_test(&(array_of_requests[i]), &tmp_flag, status);

				if(tmp_flag)
				{
					mpc_lowcomm_wait(&(array_of_requests[i]), status);
					*index = count;
					SCTK_PROFIL_END(MPC_Waitany);
					MPC_ERROR_SUCCESS();
				}
			}
		}

		TODO("wait_for_value_and_poll should be used here")
		mpc_thread_yield();
	}
}

/*******************
* RANK CONVERSION *
*******************/

int mpc_mpi_cl_world_rank(mpc_lowcomm_communicator_t comm, int rank)
{
	return mpc_lowcomm_communicator_world_rank_of(comm, rank);
}

/************************************************************************/
/* MPI Status Modification and Query                                    */
/************************************************************************/


int _mpc_cl_request_get_status(mpc_lowcomm_request_t request, int *flag, mpc_lowcomm_status_t *status)
{
	mpc_lowcomm_commit_status_from_request(&request, status);
	*flag = (request.completion_flag == MPC_LOWCOMM_MESSAGE_DONE);
	return MPC_LOWCOMM_SUCCESS;
}

int _mpc_cl_status_set_elements_x(mpc_lowcomm_status_t *status, mpc_lowcomm_datatype_t datatype, size_t count)
{
	if(status == MPC_LOWCOMM_STATUS_NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	size_t elem_size = 0;

	_mpc_cl_type_size(datatype, &elem_size);
	status->size = (int)(elem_size * count);
	return MPC_LOWCOMM_SUCCESS;
}

int _mpc_cl_status_set_elements(mpc_lowcomm_status_t *status, mpc_lowcomm_datatype_t datatype, int count)
{
	return _mpc_cl_status_set_elements_x(status, datatype, count);
}

int _mpc_cl_status_get_count(const mpc_lowcomm_status_t *status, mpc_lowcomm_datatype_t datatype, mpc_lowcomm_msg_count_t *count)
{
	int           res = MPC_LOWCOMM_SUCCESS;
	size_t size = 0;
	size_t        data_size = 0;

	if(status == MPC_LOWCOMM_STATUS_NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_IN_STATUS, "Invalid status");
	}

	res = _mpc_cl_type_size(datatype, &data_size);

	if(res != MPC_LOWCOMM_SUCCESS)
	{
		return res;
	}

	if(data_size != 0)
	{
		size = status->size;
		mpc_common_nodebug("Get_count : count %d, data_type %p (size %d)", size, datatype,
		                   data_size);

		if(size % data_size == 0)
		{
			size   = size / data_size;
			*count = (int)size;
		}
		else
		{
			*count = MPC_UNDEFINED;
		}
	}
	else
	{
		*count = 0;
	}

	return res;
}

/*************************
* MPC REDUCE OPERATIONS *
*************************/

int _mpc_cl_op_create(sctk_Op_User_function *function, int commute, sctk_Op *op)
{
	sctk_Op init = SCTK_OP_INIT;

	assume(commute);
	*op        = init;
	op->u_func = function;
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_op_free(sctk_Op *op)
{
	sctk_Op init = SCTK_OP_INIT;

	*op = init;
	MPC_ERROR_SUCCESS();
}

/***************************
* COMMUNICATOR MANAGEMENT *
***************************/

int _mpc_cl_attach_per_comm(mpc_lowcomm_communicator_t comm, mpc_lowcomm_communicator_t new_comm)
{
	if(new_comm != MPC_COMM_NULL)
	{
		mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
		mpc_lowcomm_barrier(new_comm);
		__mpc_cl_per_communicator_alloc_from_existing(task_specific, new_comm, comm);
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_comm_create(mpc_lowcomm_communicator_t comm, mpc_lowcomm_group_t *group, mpc_lowcomm_communicator_t *comm_out)
{
	assert(comm != MPC_COMM_NULL);

	mpc_lowcomm_communicator_t new_comm = MPC_COMM_NULL;


	new_comm = mpc_lowcomm_communicator_from_group(comm, group);

	_mpc_cl_attach_per_comm(comm, new_comm);

	*comm_out = new_comm;

	MPC_ERROR_SUCCESS();
}


int _mpc_cl_intercomm_create(mpc_lowcomm_communicator_t local_comm, int local_leader,
                             mpc_lowcomm_communicator_t peer_comm, int remote_leader,
                             int tag, mpc_lowcomm_communicator_t *newintercomm)
{
	assert(local_comm != MPC_COMM_NULL);

	*newintercomm = mpc_lowcomm_communicator_intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag);

	if(*newintercomm != MPC_COMM_NULL)
	{
		mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
		__mpc_cl_per_communicator_alloc_from_existing(task_specific, *newintercomm, local_comm);
	}

	mpc_lowcomm_barrier(local_comm);

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_comm_free(mpc_lowcomm_communicator_t *comm)
{
	mpc_lowcomm_barrier(*comm);

	if(*comm == MPC_COMM_WORLD)
	{
		MPC_ERROR_SUCCESS();
	}

	__mpc_cl_per_communicator_delete(_mpc_cl_per_mpi_process_ctx_get(), *comm);

	mpc_lowcomm_communicator_free(comm);
	*comm = MPC_COMM_NULL;

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_intercommcomm_merge(mpc_lowcomm_communicator_t intercomm, int high, mpc_lowcomm_communicator_t *newintracomm)
{
	*newintracomm = mpc_lowcomm_intercommunicator_merge(intercomm, high);
	__mpc_cl_per_communicator_alloc_from_existing_dup(_mpc_cl_per_mpi_process_ctx_get(),
	                                                  *newintracomm, intercomm);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_comm_dup(mpc_lowcomm_communicator_t incomm, mpc_lowcomm_communicator_t *outcomm)
{
	*outcomm = mpc_lowcomm_communicator_dup(incomm);
	__mpc_cl_per_communicator_alloc_from_existing_dup(_mpc_cl_per_mpi_process_ctx_get(),
	                                                  *outcomm, incomm);
	MPC_ERROR_SUCCESS();
}

typedef struct
{
	int color;
	int key;
	int rank;
} mpc_comm_split_t;

int _mpc_cl_comm_split(mpc_lowcomm_communicator_t comm, int color, int key, mpc_lowcomm_communicator_t *comm_out)
{
	*comm_out = mpc_lowcomm_communicator_split(comm, color, key);

	mpc_common_debug("Split comm %p col %d key %d == %p == %u", comm, color, key, *comm_out, mpc_lowcomm_communicator_id(comm) );

	if(comm_out != (mpc_lowcomm_communicator_t*)MPC_COMM_NULL)
	{
		__mpc_cl_per_communicator_alloc_from_existing(_mpc_cl_per_mpi_process_ctx_get(),
		                                              *comm_out, comm);
	}

	MPC_ERROR_SUCCESS();
}

/************************************************************************/
/*  Error handling                                                      */
/************************************************************************/

int _mpc_cl_errhandler_create(MPC_Handler_function *function,
                              MPC_Errhandler *errhandler)
{
	return _mpc_mpi_errhandler_register( ( _mpc_mpi_generic_errhandler_func_t )function, ( _mpc_mpi_errhandler_t * )errhandler);
}

int _mpc_cl_errhandler_set(mpc_lowcomm_communicator_t comm, MPC_Errhandler errhandler)
{
	return _mpc_mpi_handle_set_errhandler( ( sctk_handle )comm, _MPC_MPI_HANDLE_COMM, ( _mpc_mpi_errhandler_t )errhandler);
}

int _mpc_cl_errhandler_get(mpc_lowcomm_communicator_t comm, MPC_Errhandler *errhandler)
{
	*errhandler = ( MPC_Errhandler )_mpc_mpi_handle_get_errhandler( ( sctk_handle )comm, _MPC_MPI_HANDLE_COMM);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_errhandler_free(MPC_Errhandler *errhandler)
{
	_mpc_mpi_errhandler_free(*errhandler);
	*errhandler = MPC_ERRHANDLER_NULL;
	MPC_ERROR_SUCCESS();
}

#define err_case_sprintf(code, msg)        \
		case code:                 \
			(void)sprintf(str, msg); \
			break

int mpc_mpi_cl_error_string(int code, char *str, int *len)
{
	str[0] = '\0';

	switch(code)
	{
	err_case_sprintf(MPC_ERR_BUFFER, "Invalid buffer pointer");
	err_case_sprintf(MPC_ERR_COUNT, "Invalid count argument");
	err_case_sprintf(MPC_ERR_TYPE, "Invalid datatype argument");
	err_case_sprintf(MPC_ERR_TAG, "Invalid tag argument");
	err_case_sprintf(MPC_ERR_COMM, "Invalid communicator");
	err_case_sprintf(MPC_ERR_RANK, "Invalid rank");
	err_case_sprintf(MPC_ERR_ROOT, "Invalid root");
	err_case_sprintf(MPC_ERR_TRUNCATE, "Message truncated on receive");
	err_case_sprintf(MPC_ERR_GROUP, "Invalid group");
	err_case_sprintf(MPC_ERR_OP, "Invalid operation");
	err_case_sprintf(MPC_ERR_REQUEST, "Invalid mpc_request handle");
	err_case_sprintf(MPC_ERR_TOPOLOGY, "Invalid topology");
	err_case_sprintf(MPC_ERR_DIMS, "Invalid dimension argument");
	err_case_sprintf(MPC_ERR_ARG, "Invalid argument");
	err_case_sprintf(MPC_ERR_OTHER, "Other error; use Error_string");
	err_case_sprintf(MPC_ERR_UNKNOWN, "Unknown error");
	err_case_sprintf(MPC_ERR_INTERN, "Internal error code");
	err_case_sprintf(MPC_ERR_IN_STATUS, "Look in status for error value");
	err_case_sprintf(MPC_ERR_PENDING, "Pending request");
	err_case_sprintf(MPC_NOT_IMPLEMENTED, "Not implemented");
	err_case_sprintf(MPC_ERR_INFO, "Invalid Status argument");
	err_case_sprintf(MPC_ERR_INFO_KEY, "Provided info key is too large");
	err_case_sprintf(MPC_ERR_INFO_VALUE, "Provided info value is too large");
	err_case_sprintf(MPC_ERR_INFO_NOKEY, "Could not locate a value with this key");

		default:
			mpc_common_debug_warning("%d error code unknown", code);
	}

	*len = (int)strlen(str);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_error_class(int errorcode, int *errorclass)
{
	*errorclass = errorcode;
	MPC_ERROR_SUCCESS();
}

void _mpc_cl_default_error(mpc_lowcomm_communicator_t *comm, const int * error, char *msg, char *function, char *file, int line)
{
	char str[MPI_MAX_NAME_STRING];
	int  i = 0;

	mpc_mpi_cl_error_string(*error, str, &i);

	if(i != 0)
	{
		mpc_common_debug_error("%s: %s [%s] at %s:%d ", function, msg, str, file, line);
	}
	else
	{
		mpc_common_debug_error("Unknown error");
	}

	_mpc_cl_abort(*comm, *error);
}

void _mpc_cl_return_error(mpc_lowcomm_communicator_t *comm, const int *error, char *message, char *function, char *file, int line)
{
	char str[MPI_MAX_NAME_STRING];
	int  i = 0;

	mpc_mpi_cl_error_string(*error, str, &i);

	if(i != 0)
	{
		char extra_info[MPI_MAX_NAME_STRING];
		extra_info[0] = '\0';

		// Are we in debug mode ?
#if !defined(NDEBUG)
		(void)snprintf(extra_info, MPI_MAX_NAME_STRING, " at %s:%d", file, line);
#endif

		mpc_common_debug_error("%s : %s [%s] on comm %d%s", function, message, str, mpc_lowcomm_communicator_id(*comm), extra_info);
	}
}

void _mpc_cl_abort_error(mpc_lowcomm_communicator_t *comm, const int *error, char *message, char *function, char *file,
                         int line)
{
	char str[MPI_MAX_NAME_STRING];
	int  i = 0;

	mpc_mpi_cl_error_string(*error, str, &i);
	mpc_common_debug_error("===================================================");

	if(i != 0)
	{
		mpc_common_debug_error("Error: %s on comm %d : %s", str, mpc_lowcomm_communicator_id(*comm), message);
	}

	mpc_common_debug_error("This occured in %s at %s:%d", function, file, line);
	mpc_common_debug_error("MPC Encountered an Error will now abort");
	mpc_common_debug_error("===================================================");
	mpc_common_debug_abort();
}

int _mpc_cl_error_init()
{
        mpc_mpi_err_init();
        
        //NOTE: next are common to all tasks, so must be initialized only once
        if (mpc_common_get_local_task_rank() == 0) {
                _mpc_mpi_errhandler_register_on_slot( ( _mpc_mpi_generic_errhandler_func_t )_mpc_cl_default_error,
                                                      (int)(long)MPC_ERRHANDLER_NULL);
                _mpc_mpi_errhandler_register_on_slot( ( _mpc_mpi_generic_errhandler_func_t )_mpc_cl_return_error,
                                                      (int)(long)MPC_ERRORS_RETURN);
                _mpc_mpi_errhandler_register_on_slot( ( _mpc_mpi_generic_errhandler_func_t )_mpc_cl_abort_error,
                                                      (int)(long)MPC_ERRORS_ARE_FATAL);
                // TODO(jbbesnard): Not the correct behaviour
                _mpc_mpi_errhandler_register_on_slot( ( _mpc_mpi_generic_errhandler_func_t )_mpc_cl_abort_error,
                                                      (int)(long)MPC_ERRORS_ABORT);
        }

        _mpc_cl_errhandler_set(__mpc_lowcomm_communicator_from_predefined(MPC_COMM_WORLD), MPC_ERRORS_ARE_FATAL);
        _mpc_cl_errhandler_set(__mpc_lowcomm_communicator_from_predefined(MPC_COMM_SELF), MPC_ERRORS_ARE_FATAL);

	MPC_ERROR_SUCCESS();
}

#ifdef MPC_USE_DMTCP
static volatile mpc_lowcomm_checkpoint_state_t global_state = MPC_STATE_ERROR;
#endif

int _mpc_cl_checkpoint(mpc_lowcomm_checkpoint_state_t *state)
{
#ifdef MPC_USE_DMTCP
	if(mpc_common_get_flags()->checkpoint_enabled)
	{
		int total_nbprocs            = mpc_common_get_process_count();
		int local_nbtasks            = mpc_common_get_local_task_count();
		int local_tasknum            = mpc_common_get_local_task_rank();
		int task_rank                = mpc_common_get_task_rank();
		int pmi_rank                 = -1;
		static OPA_int_t init_once   = OPA_INT_T_INITIALIZER(0);
		static OPA_int_t gen_acquire = OPA_INT_T_INITIALIZER(0);
		static OPA_int_t gen_release = OPA_INT_T_INITIALIZER(0);
		static OPA_int_t gen_current = OPA_INT_T_INITIALIZER(0);
		static int *     task_generations;

		/* init once the genration array */
		if(OPA_cas_int(&init_once, 0, 1) == 0)
		{
			task_generations = ( int * )malloc(sizeof(int) * local_nbtasks);
			memset(task_generations, 0, sizeof(int) * local_nbtasks);
			OPA_store_int(&init_once, 2);
		}
		else
		{
			while(OPA_load_int(&init_once) != 2)
			{
				mpc_thread_yield();
			}
		}

		/* ensure there won't be any overlapping betwen different MPC_Checkpoint() calls */
		while(OPA_load_int(&gen_current) < task_generations[local_tasknum])
		{
			mpc_thread_yield();
		}

		/* if I'm the last task to process: do the work */
		if(OPA_fetch_and_incr_int(&gen_acquire) == local_nbtasks - 1)
		{
			/* save the old checkpoint/restart counters */
			sctk_ft_checkpoint_init();
			/* synchronize all processes */
			sctk_ft_disable();

			if(total_nbprocs > 1)
			{
				pmi_rank = mpc_common_get_process_rank();
				mpc_launch_pmi_barrier();
			}
			else
			{
				pmi_rank = 0;
			}

			/* close the network */
			sctk_ft_checkpoint_prepare();
			sctk_ft_enable();

			/* Only one process triggers the checkpoint (=notify the coordinator) */
			if(pmi_rank == 0)
			{
				sctk_ft_checkpoint(); /* we can ignore the return value here, by construction */
			}

			global_state = sctk_ft_checkpoint_wait();
			sctk_ft_checkpoint_finalize();
			/* set gen_release to 0, prepare the end of current generation */
			OPA_store_int(&gen_release, 0);
			/* set gen_aquire to 0: unlock waiting tasks */
			OPA_store_int(&gen_acquire, 0);
		}
		else
		{
			/* waiting tasks */
			while(OPA_load_int(&gen_acquire) != 0)
			{
				mpc_thread_yield();
			}
		}

		/* update the state for all tasks of this process */
		if(state)
		{
			*state = global_state;
		}

		/* re-init the network at task level if necessary */
		sctk_net_init_task_level(task_rank, mpc_topology_get_current_cpu() );
		mpc_lowcomm_terminaison_barrier();

		/* If I'm the last task to reach here, increment the global generation counter */
		if(OPA_fetch_and_incr_int(&gen_release) == local_nbtasks - 1)
		/* the current task finished the work for the current generation */
		{
			task_generations[local_tasknum]++;
		}
	}
	else
	{
		if(state)
		{
			*state = MPC_STATE_IGNORE;
		}
	}
#else
	if(state)
	{
		*state = MPC_STATE_NO_SUPPORT;
	}
#endif
	MPC_ERROR_SUCCESS();
}

/**********
* TIMING *
**********/

double _mpc_cl_wtime()
{
	double res = NAN;

	SCTK_PROFIL_START(MPC_Wtime);
#if SCTK_WTIME_USE_GETTIMEOFDAY
	struct timeval tp;
	gettimeofday(&tp, NULL);
	res = ( double )tp.tv_usec + ( double )tp.tv_sec * ( double )1000000;
	res = res / ( double )1000000;
#else
	res = mpc_arch_get_timestamp();
#endif
	mpc_common_nodebug("Wtime = %f", res);
	SCTK_PROFIL_END(MPC_Wtime);
	return res;
}

double _mpc_cl_wtick()
{
	return mpc_arch_tsc_freq_get();
}

/************************************************************************/
/* Pack managment                                                       */
/************************************************************************/

int mpc_mpi_cl_open_pack(mpc_lowcomm_request_t *request)
{
	SCTK_PROFIL_START(MPC_Open_pack);

	if(request == NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
	}

	memset(&request->header, 0, sizeof(mpc_lowcomm_request_header_t) );
	memset(&request->dt, 0, sizeof(mpc_lowcomm_request_content_t) );

	SCTK_PROFIL_END(MPC_Open_pack);
	MPC_ERROR_SUCCESS();
}

int mpc_mpi_cl_add_pack(void *buf, mpc_lowcomm_msg_count_t count,
                        unsigned long *begins,
                        unsigned long *ends,
                        mpc_lowcomm_datatype_t datatype, mpc_lowcomm_request_t *request)
{

	if(request == NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
	}

	if(begins == NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
	}

	if(ends == NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
	}

	if(count == 0)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
	}

	size_t data_size = __mpc_cl_datatype_get_size(datatype);

	mpc_lowcomm_request_add_pack(request, buf, count, data_size, begins, ends);

	size_t total = 0;

	int i = 0;

	/*Compute message size */
	for(i = 0; i < count; i++)
	{
		total += ends[i] - begins[i] + 1;
	}

	//FIXME: use library function instead of acceding field
	request->header.msg_size += total * data_size;


	MPC_ERROR_SUCCESS();
}

int mpc_mpi_cl_add_pack_absolute(const void *buf, mpc_lowcomm_msg_count_t count,
                                 long *begins,
                                 long *ends,
                                 mpc_lowcomm_datatype_t datatype,
                                 mpc_lowcomm_request_t *request)
{
	size_t data_size = __mpc_cl_datatype_get_size(datatype);

	if(request == NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_REQUEST, "");
	}

	if(begins == NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
	}

	if(ends == NULL)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_ARG, "");
	}

	mpc_lowcomm_request_add_pack_absolute(request, buf, count, data_size, begins, ends);

	size_t total = 0;
	int    i = 0;

	/*Compute message size */
	for(i = 0; i < count; i++)
	{
		total += ends[i] - begins[i] + 1;
	}

	//FIXME: use library function instead of acceding field
	request->header.msg_size += total * data_size;

	MPC_ERROR_SUCCESS();
}

int mpc_mpi_cl_isend_pack(int dest, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request)
{
	SCTK_PROFIL_START(MPC_Isend_pack);
	
	if(dest == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(request, comm, REQUEST_SEND);
		MPC_ERROR_SUCCESS();
	}

	if(request == NULL)
	{
		MPC_ERROR_REPORT(comm, MPC_ERR_REQUEST, "");
	}

	mpc_lowcomm_isend(dest, NULL, request->header.msg_size, tag, comm, request);

	SCTK_PROFIL_END(MPC_Isend_pack);
	MPC_ERROR_SUCCESS();
}

int mpc_mpi_cl_irecv_pack(int source, int tag, mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *request)
{
	SCTK_PROFIL_START(MPC_Irecv_pack);

	if(source == MPC_PROC_NULL)
	{
		mpc_lowcomm_request_init(request, comm, REQUEST_RECV);
		MPC_ERROR_SUCCESS();
	}

	if(request == NULL)
	{
		MPC_ERROR_REPORT(comm, MPC_ERR_REQUEST, "");
	}

	mpc_lowcomm_irecv(source, NULL, request->header.msg_size, tag, comm, request);

	SCTK_PROFIL_END(MPC_Irecv_pack);
	MPC_ERROR_SUCCESS();
}

/************************************************************************/
/* MPI_Info management                                                  */
/************************************************************************/

/* Utilities functions for this section are located in mpc_info.[ch] */

int _mpc_cl_info_create(MPC_Info *info)
{
	/* Retrieve task context */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();

	/* First set to NULL */
	*info = MPC_INFO_NULL;
	/* Create a new entry */
	int new_id = MPC_Info_factory_create(&task_specific->info_fact);

	/* We did not get a new entry */
	if(new_id < 0)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
		                 "Failled to allocate new MPI_Info");
	}

	/* Then set the new ID */
	*info = new_id;
	/* All clear */
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_info_free(MPC_Info *info)
{
	/* Retrieve task context */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	/* Try to delete from factory */
	int ret = MPC_Info_factory_delete(&task_specific->info_fact, ( int )*info);

	if(ret)
	{
		/* Failed to delete no such info */
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
		                 "Failled to delete MPI_Info");
	}
	
			/* Delete was successful */
		*info = MPC_INFO_NULL;
		MPC_ERROR_SUCCESS();

}

int _mpc_cl_info_set(MPC_Info info, const char *key, const char *value)
{
	/* Retrieve task context */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve(&task_specific->info_fact, ( int )info);

	if(!cell)
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO, "Failled to get MPI_Info");
	}

	/* Check for lenght boundaries */
	size_t keylen   = strlen(key);
	size_t valuelen = strlen(value);

	if(MPC_MAX_INFO_KEY <= keylen)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_KEY, "");
	}

	if(MPC_MAX_INFO_VAL <= valuelen)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_VALUE, "");
	}

	/* Now set the key */
	MPC_Info_cell_set(cell, ( char * )key, ( char * )value, 1);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_info_delete(MPC_Info info, const char *key)
{
	/* Retrieve task context */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve(&task_specific->info_fact, ( int )info);

	if(!cell)
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
		                 "Failled to delete MPI_Info");
	}

	int ret = MPC_Info_cell_delete(cell, ( char * )key);

	if(ret)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_NOKEY,
		                 " Failled to delete key");
	}

	MPC_ERROR_SUCCESS();
}

int _mpc_cl_info_get(MPC_Info info, const char *key, int valuelen, char *value,
                     int *flag)
{
	/* Retrieve task context */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve(&task_specific->info_fact, ( int )info);

	if(!cell)
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
		                 "Failled to delete MPI_Info");
	}

	MPC_Info_cell_get(cell, ( char * )key, value, valuelen, flag);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_info_get_nkeys(MPC_Info info, int *nkeys)
{
	/* Retrieve task context */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();

	*nkeys = 0;
	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve(&task_specific->info_fact, ( int )info);

	if(!cell)
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
		                 "Failled to get MPI_Info key count");
	}

	*nkeys = MPC_Info_key_count(cell->keys);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_info_get_count(MPC_Info info, int n, char *key)
{
	/* Retrieve task context */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve(&task_specific->info_fact, ( int )info);

	if(!cell)
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
		                 "Failled to get MPI_Info key count");
	}

	/* Thry to get the nth entry */
	struct MPC_Info_key *keyEntry = MPC_Info_key_get_nth(cell->keys, n);

	/* Not found */
	if(!keyEntry)
	{
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_NOKEY,
		                 "Failled to retrieve an MPI_Info's key");
	}
	
			/* Found just snprintf the key */
		(void)snprintf(key, MPC_MAX_INFO_KEY, "%s", keyEntry->key);


	MPC_ERROR_SUCCESS();
}

int _mpc_cl_info_dup(MPC_Info info, MPC_Info *newinfo)
{
	/* First create a new entry */
	int ret = _mpc_cl_info_create(newinfo);

	if(ret != MPC_LOWCOMM_SUCCESS)
	{
		return ret;
	}

	/* Prepare to copy keys one by one */
	int  num_keys = 0;
	char keybuf[MPC_MAX_INFO_KEY];
	/* This is the buffer for the value */
	char *valbuff = sctk_malloc(MPC_MAX_INFO_VAL * sizeof(char) );

	if(!valbuff)
	{
		perror("sctk_alloc");
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INTERN,
		                 "Failled to allocate temporary value buffer");
	}

	int i = 0;

	_mpc_cl_info_get_nkeys(info, &num_keys);
	int flag = 0;

	/* Copy keys */
	for(i = 0; i < num_keys; i++)
	{
		/* Get key */
		_mpc_cl_info_get_count(info, i, keybuf);
		/* Get value */
		_mpc_cl_info_get(info, keybuf, MPC_MAX_INFO_VAL, valbuff, &flag);

		if(!flag)
		{
			/* Shall not happen */
			sctk_free(valbuff);
			MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO_NOKEY,
			                 "Could not retrieve a key which should have been there");
		}

		/* Store value at key in the newly allocated object */
		_mpc_cl_info_set(*newinfo, keybuf, valbuff);
	}

	sctk_free(valbuff);
	MPC_ERROR_SUCCESS();
}

int _mpc_cl_info_get_valuelen(MPC_Info info, const char *key, int *valuelen, int *flag)
{
	/* Retrieve task context */
	mpc_mpi_cl_per_mpi_process_ctx_t *task_specific = _mpc_cl_per_mpi_process_ctx_get();
	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve(&task_specific->info_fact, ( int )info);

	if(!cell)
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT(MPC_COMM_WORLD, MPC_ERR_INFO,
		                 "Failled to get MPI_Info key count");
	}

	/* Retrieve the cell @ key */
	struct MPC_Info_key *keyEntry = MPC_Info_key_get(cell->keys, ( char * )key);

	if(!keyEntry)
	{
		/* Nothing found, set flag */
		*flag = 0;
	}
	else
	{
		/* We found it ! set length with strlen */
		*flag     = 1;
		*valuelen = (int)strlen(keyEntry->value);
	}

	MPC_ERROR_SUCCESS();
}

/********************
* THREAD MIGRATION *
********************/

int mpc_cl_move_to(int process, int cpuid)
{
#ifdef MPC_Threads
	int proc = mpc_topology_get_current_cpu();

	if(process == mpc_common_get_process_rank() )
	{
		if(proc != cpuid)
		{
			mpc_common_init_trigger("MPC_MPI Force Yield");
			int ret = mpc_thread_migrate_to_core(cpuid);

			if(0 <= ret)
			{
				MPC_ERROR_SUCCESS();
			}
		}
		else
		{
			return MPI_ERR_ARG;
		}
	}
#endif

	mpc_common_debug_warning("Inter process migration Disabled");
	return MPC_ERR_UNSUPPORTED_OPERATION;
}

/*****************************************
* COMM LIB REGISTATION AND CONSTRUCTORS *
*****************************************/

static void __init_basic_checks()
{
	mpc_common_debug_check_type_equal(mpc_lowcomm_msg_count_t, unsigned int);
}

static void __set_thread_trampoline()
{
	mpc_thread_mpi_task_atexit(mpc_mpi_cl_per_mpi_process_ctx_at_exit_register);
	mpc_thread_mpi_ctx_set(mpc_cl_per_mpi_process_ctx_get);
}

static void __set_lowcomm_trampoline()
{
	mpc_lowcomm_egreq_poll_set_trampoline(mpc_mpi_cl_egreq_progress_poll);
	mpc_lowcomm_set_request_completion_trampoline(mpc_MPI_notify_request_counter);
	mpc_lowcomm_rdma_MPC_MPI_notify_src_ctx_set_trampoline(mpc_MPI_Win_notify_src_ctx_counter);
	mpc_lowcomm_rdma_MPC_MPI_notify_dest_ctx_set_trampoline(mpc_MPI_Win_notify_dest_ctx_counter);
}

void mpc_cl_comm_lib_init() __attribute__( (constructor) );

void mpc_cl_comm_lib_init()
{
	MPC_INIT_CALL_ONLY_ONCE

	/* Register MPC MPI Config */
	mpc_common_init_callback_register("Config Sources", "MPC MPI Config Registration", _mpc_mpi_config_init, 256);

	mpc_common_init_callback_register("Config Checks", "MPC MPI Collective Pointers Validation", _mpc_mpi_config_check, 128);

	/* Before Starting MPI tasks */
#ifdef MPC_Threads
	mpc_common_init_callback_register("Before Starting VPs",
	                                  "Register Threading trampoline for MPI",
	                                  __set_thread_trampoline, 21);

	mpc_common_init_callback_register("Before Starting VPs",
	                                  "Init thread keys for MPI",
	                                  mpc_cl_init_thread_keys, 22);
#endif

	mpc_common_init_callback_register("Before Starting VPs",
	                                  "Register lowcomm trampoline for MPI",
	                                  __set_lowcomm_trampoline, 23);

	/* Thread START */


	mpc_common_init_callback_register("Per Thread Init",
	                                  "Per MPI Thread CTX",
	                                  mpc_mpi_cl_per_thread_ctx_init, 22);

	/* Thread END */

	mpc_common_init_callback_register("Per Thread Release",
	                                  "Per MPI Thread CTX Release",
	                                  mpc_mpi_cl_per_thread_ctx_release, 22);


	/* Main START */

	mpc_common_init_callback_register("Starting Main",
	                                  "Basic Type Checks",
	                                  __init_basic_checks, 21);

	mpc_common_init_callback_register("Starting Main",
	                                  "MPI Process CTX Init",
	                                  __mpc_cl_per_mpi_process_ctx_init, 22);

	mpc_common_init_callback_register("Starting Main",
	                                  "Enter TMP Directory",
	                                  __mpc_cl_enter_tmp_directory, 24);

	/* Main END */

	mpc_common_init_callback_register("Ending Main",
	                                  "Per MPI Process CTX Release",
	                                  __mpc_cl_per_mpi_process_ctx_release, 0);
}
