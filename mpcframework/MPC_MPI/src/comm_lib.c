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

#include <sctk_launch.h>

#include "mpc_common_helper.h"

#include "mpc_reduction.h"
#include "sctk_inter_thread_comm.h"
#include <sys/time.h>
#include "sctk_handle.h"
#include "mpc_internal_common.h"

#ifdef MPC_Fault_Tolerance
#include "sctk_ft_iface.h"
#endif

sctk_Op_f sctk_get_common_function(mpc_mp_datatype_t datatype, sctk_Op op);


/************************************************************************/
/*GLOBAL VARIABLES                                                      */
/************************************************************************/

sctk_thread_key_t mpc_mpi_m_per_mpi_process_ctx;
static sctk_thread_key_t sctk_func_key;
static int __mpc_cl_buffering_enabled = 1;

const _mpc_cl_group_t mpc_group_empty = {0, NULL};
const _mpc_cl_group_t mpc_group_null = {-1, NULL};
mpc_mp_request_t mpc_request_null;

/** \brief Intitializes thread context keys
 * This function is called in sctk_thread.c:2212
 * by sctk_start_func
 */
void sctk_mpc_init_keys() {
  sctk_thread_key_create(&sctk_func_key, NULL);
  sctk_thread_key_create(&mpc_mpi_m_per_mpi_process_ctx, NULL);
}

/****************************
 * PER COMMUNICATOR STORAGE *
 ****************************/

static inline mpc_per_communicator_t *_mpc_cl_per_communicator_get_no_lock( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
								           mpc_mp_communicator_t comm )
{
	mpc_per_communicator_t *per_communicator;

	HASH_FIND( hh, task_specific->per_communicator, &comm,
			   sizeof( mpc_mp_communicator_t ), per_communicator );

	return per_communicator;
}


mpc_per_communicator_t *_mpc_cl_per_communicator_get( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
								    mpc_mp_communicator_t comm )
{
	mpc_per_communicator_t *per_communicator;
	mpc_common_spinlock_lock( &( task_specific->per_communicator_lock ) );
	per_communicator = _mpc_cl_per_communicator_get_no_lock(task_specific, comm);
	mpc_common_spinlock_unlock( &( task_specific->per_communicator_lock ) );
	return per_communicator;
}

static inline void __mpc_cl_per_communicator_set( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
						mpc_per_communicator_t *mpc_per_comm,
						mpc_mp_communicator_t comm )
{
	mpc_common_spinlock_lock( &( task_specific->per_communicator_lock ) );
	mpc_per_comm->key = comm;

	HASH_ADD( hh, task_specific->per_communicator, key,
			  sizeof( mpc_mp_communicator_t ), mpc_per_comm );

	mpc_common_spinlock_unlock( &( task_specific->per_communicator_lock ) );
	return;
}

static inline void __mpc_cl_per_communicator_delete( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
						    mpc_mp_communicator_t comm )
{
	mpc_common_spinlock_lock( &( task_specific->per_communicator_lock ) );

	mpc_per_communicator_t *per_communicator = _mpc_cl_per_communicator_get_no_lock(task_specific, comm);

	assume( per_communicator != NULL );
	assume( per_communicator->key == comm );

	HASH_DELETE( hh, task_specific->per_communicator, per_communicator );

	sctk_free( per_communicator );
	mpc_common_spinlock_unlock( &( task_specific->per_communicator_lock ) );
}

static inline mpc_per_communicator_t * __mpc_cl_per_communicator_alloc()
{
	mpc_per_communicator_t *tmp;
	mpc_common_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;

	tmp = sctk_malloc( sizeof( mpc_per_communicator_t ) );

	tmp->err_handler = (MPC_Handler_function *) _mpc_cl_default_error;
	tmp->err_handler_lock = lock;
	tmp->mpc_mpi_per_communicator = NULL;
	tmp->mpc_mpi_per_communicator_copy = NULL;
	tmp->mpc_mpi_per_communicator_copy_dup = NULL;

	return tmp;
}

static inline void ___mpc_cl_per_communicator_copy(mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
						  mpc_mp_communicator_t new_comm,
						  mpc_per_communicator_t *per_communicator,
						   void (*copy_fn)(struct mpc_mpi_per_communicator_s**,struct mpc_mpi_per_communicator_s*) )
{
	assume( per_communicator != NULL );

	mpc_per_communicator_t *per_communicator_new = __mpc_cl_per_communicator_alloc();
	memcpy( per_communicator_new, per_communicator,
			sizeof( mpc_per_communicator_t ) );

	if ( (copy_fn) != NULL )
	{
		(copy_fn)(&( per_communicator_new->mpc_mpi_per_communicator ),
			     per_communicator->mpc_mpi_per_communicator );
	}

	__mpc_cl_per_communicator_set( task_specific, per_communicator_new, new_comm );
}

static inline void __mpc_cl_per_communicator_alloc_from_existing(
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific, mpc_mp_communicator_t new_comm,
	mpc_mp_communicator_t old_comm )
{
	mpc_per_communicator_t *per_communicator = _mpc_cl_per_communicator_get_no_lock(task_specific, old_comm);
	assume( per_communicator != NULL );
	___mpc_cl_per_communicator_copy(task_specific, new_comm, per_communicator, per_communicator->mpc_mpi_per_communicator_copy);
}

static inline void __mpc_cl_per_communicator_alloc_from_existing_dup(
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific, mpc_mp_communicator_t new_comm,
	mpc_mp_communicator_t old_comm )
{
	mpc_per_communicator_t *per_communicator = _mpc_cl_per_communicator_get_no_lock(task_specific, old_comm);
	assume( per_communicator != NULL );
	___mpc_cl_per_communicator_copy(task_specific, new_comm, per_communicator, per_communicator->mpc_mpi_per_communicator_copy_dup);
}

/************************************************************************/
/* Request Handling                                                     */
/************************************************************************/

static inline void __mpc_cl_request_commit_status( mpc_mp_request_t *request,
						  mpc_mp_status_t *status )
{
	if ( request->request_type == REQUEST_GENERALIZED )
	{
		mpc_mp_status_t static_status;

		/* You must provide a valid status to the querry function */
		if ( status == SCTK_STATUS_NULL )
			status = &static_status;

		memset( status, 0, sizeof( mpc_mp_status_t ) );
		/* Fill in the status info */
		( request->query_fn )( request->extra_state, status );
		/* Free the request */
		( request->free_fn )( request->extra_state );
	}
	else if ( status != SCTK_STATUS_NULL )
	{
		status->MPC_SOURCE = request->header.source_task;
		status->MPC_TAG = request->header.message_tag;
		status->MPC_ERROR = SCTK_SUCCESS;

		if ( request->truncated )
		{
			status->MPC_ERROR = MPC_ERR_TRUNCATE;
		}

		status->size = request->header.msg_size;

		if ( request->completion_flag == SCTK_MESSAGE_CANCELED )
		{
			status->cancelled = 1;
		}
		else
		{
			status->cancelled = 0;
		}

		if ( request->source_type != request->dest_type )
		{
			if ( /* See page 33 of 3.0 PACKED and BYTE are exceptions */
					request->source_type != MPC_PACKED &&
					request->dest_type != MPC_PACKED &&
					request->source_type != MPC_BYTE && request->dest_type != MPC_BYTE &&
					request->header.msg_size > 0 )
			{
				if ( _mpc_dt_is_common( request->source_type ) &&
				     _mpc_dt_is_common( request->dest_type ) )
				{
					request->status_error = MPC_ERR_TYPE;
				}
			}
		}
	}
}

void mpc_mpi_m_egreq_progress_poll();

static inline void __mpc_cl_request_progress( mpc_mp_request_t *request )
{

	if ( request->request_type == REQUEST_GENERALIZED )
	{
		/* Try to poll the request */
		mpc_mpi_m_egreq_progress_poll();
		/* We are done here */
		return;
	}

	struct mpc_mp_comm_ptp_msg_progress_s _wait;
	mpc_mp_comm_ptp_msg_wait_init( &_wait, request, 0 );
	mpc_mp_comm_ptp_msg_progress( &_wait );
}

static inline void __mpc_cl_request_init_null()
{
	mpc_mp_comm_request_init( &mpc_request_null, SCTK_COMM_NULL, REQUEST_NULL );
	mpc_request_null.is_null = 1;
}

/************************************************************************/
/* MPC per Thread context                                                   */
/************************************************************************/

/* Asyncronous Buffers Storage */

#define MAX_MPC_BUFFERED_MSG 256

typedef struct
{
	mpc_buffered_msg_t buffer[MAX_MPC_BUFFERED_MSG];
	volatile int buffer_rank;
	mpc_common_spinlock_t lock;
} __mpc_cl_buffer_t;

typedef struct sctk_thread_buffer_pool_s
{
	__mpc_cl_buffer_t sync;
	__mpc_cl_buffer_t async;
}__mpc_cl_buffer_pool_t;

/* This is the Per VP data-structure */

typedef struct mpc_mpi_m_per_thread_ctx_s
{
	__mpc_cl_buffer_pool_t buffer_pool;
}mpc_mpi_m_per_thread_ctx_t;


/* Asyncronous Buffers Interface */

static inline void __mpc_cl_thread_buffer_pool_init( __mpc_cl_buffer_pool_t *pool )
{
	int i;

	for ( i = 0; i < MAX_MPC_BUFFERED_MSG; i++ )
	{
		mpc_mp_comm_request_init( &( pool->sync.buffer[i].request ), SCTK_COMM_NULL, REQUEST_NULL );
		mpc_mp_comm_request_init( &( pool->async.buffer[i].request ), SCTK_COMM_NULL, REQUEST_NULL );
	}

	pool->sync.buffer_rank = 0;
	pool->async.buffer_rank = 0;
}

static inline void __mpc_cl_thread_buffer_pool_release( __mpc_cl_buffer_pool_t *pool )
{
	int i;

	for ( i = 0; i < MAX_MPC_BUFFERED_MSG; i++ )
	{
		sctk_nodebug( "WAIT message %d type %d src %d dest %d tag %d size %ld", i,
					  tmp->sync.buffer[i].request.request_type,
					  tmp->sync.buffer[i].request.header.source,
					  tmp->sync.buffer[i].request.header.destination,
					  tmp->sync.buffer[i].request.header.message_tag,
					  tmp->sync.buffer[i].request.header.msg_size );

		mpc_mp_comm_request_wait( &( pool->sync.buffer[i].request ) );
	}

	for ( i = 0; i < MAX_MPC_BUFFERED_MSG; i++ )
	{
		sctk_nodebug( "WAIT message ASYNC %d", i );
		mpc_mp_comm_request_wait( &( pool->async.buffer[i].request ) );
	}
}

static inline mpc_buffered_msg_t *__mpc_cl_thread_buffer_pool_acquire_sync( mpc_mpi_m_per_thread_ctx_t *thread_spec )
{
	__mpc_cl_buffer_pool_t *pool = &( thread_spec->buffer_pool );

	mpc_buffered_msg_t *ret = NULL;

	int buffer_rank = pool->sync.buffer_rank;
	ret = &( pool->sync.buffer[buffer_rank] );

	return ret;
}

static inline void __mpc_cl_thread_buffer_pool_step_sync( mpc_mpi_m_per_thread_ctx_t *thread_spec )
{
	__mpc_cl_buffer_pool_t *pool = &( thread_spec->buffer_pool );

	int buffer_rank = pool->sync.buffer_rank;
	pool->sync.buffer_rank = ( buffer_rank + 1 ) % MAX_MPC_BUFFERED_MSG;
}

static inline mpc_mpi_m_per_thread_ctx_t * __mpc_cl_per_thread_ctx_get();

static inline mpc_buffered_msg_t *__mpc_cl_thread_buffer_pool_acquire_async()
{
	__mpc_cl_buffer_pool_t *pool = &( __mpc_cl_per_thread_ctx_get()->buffer_pool );

	mpc_buffered_msg_t *ret = NULL;

	int buffer_rank = pool->async.buffer_rank;
	ret = &( pool->async.buffer[buffer_rank] );

	return ret;
}

static inline void __mpc_cl_thread_buffer_pool_step_async()
{
	__mpc_cl_buffer_pool_t *pool = &( __mpc_cl_per_thread_ctx_get()->buffer_pool );

	int buffer_rank = pool->async.buffer_rank;
	pool->async.buffer_rank = ( buffer_rank + 1 ) % MAX_MPC_BUFFERED_MSG;
}

/* Per Thread Storage Interface */

__thread struct mpc_mpi_m_per_thread_ctx_s *___mpc_p_per_VP_comm_ctx;

static inline mpc_mpi_m_per_thread_ctx_t *__mpc_cl_per_thread_ctx_get()
{
	if ( !___mpc_p_per_VP_comm_ctx )
		mpc_mpi_m_per_thread_ctx_init();

	return (mpc_mpi_m_per_thread_ctx_t *) ___mpc_p_per_VP_comm_ctx;
}

static inline void __mpc_cl_per_thread_ctx_set( mpc_mpi_m_per_thread_ctx_t *pointer )
{
	___mpc_p_per_VP_comm_ctx = (struct mpc_mpi_m_per_thread_ctx_s *) pointer;
}

void mpc_mpi_m_per_thread_ctx_init()
{
	if ( ___mpc_p_per_VP_comm_ctx )
		return;

	/* Allocate */

	mpc_mpi_m_per_thread_ctx_t *tmp = NULL;
	tmp = sctk_malloc( sizeof( mpc_mpi_m_per_thread_ctx_t ) );
	assume( tmp != NULL );
	memset( tmp, 0, sizeof( mpc_mpi_m_per_thread_ctx_t ) );

	/* Initialize */
	__mpc_cl_thread_buffer_pool_init( &tmp->buffer_pool );

	/* Register thread context */
	__mpc_cl_per_thread_ctx_set( tmp );
}

void mpc_mpi_m_per_thread_ctx_release()
{
	if ( !___mpc_p_per_VP_comm_ctx )
		return;

	mpc_mpi_m_per_thread_ctx_t *thread_specific = ___mpc_p_per_VP_comm_ctx;

	/* Release */
	__mpc_cl_thread_buffer_pool_release( &thread_specific->buffer_pool );

	/* Free */
	sctk_free( thread_specific );
	__mpc_cl_per_thread_ctx_set( NULL );
}

/***************************
 * DISGUISE SUPPORT IN MPC *
 ***************************/

static int *__mpc_p_disguise_local_to_global_table = NULL;
static struct mpc_mpi_m_per_mpi_process_ctx_s **__mpc_p_disguise_costumes = NULL;
OPA_int_t __mpc_p_disguise_flag;

int __mpc_p_disguise_init( struct mpc_mpi_m_per_mpi_process_ctx_s *my_specific )
{

	int my_id = mpc_common_get_local_task_rank();

	if ( my_id == 0 )
	{
		OPA_store_int( &__mpc_p_disguise_flag, 0 );
		int local_count = mpc_common_get_local_task_count();
		__mpc_p_disguise_costumes = sctk_malloc( sizeof( struct mpc_mpi_m_per_mpi_process_ctx_s * ) * local_count );

		if ( __mpc_p_disguise_costumes == NULL )
		{
			perror( "malloc" );
			abort();
		}

		__mpc_p_disguise_local_to_global_table = sctk_malloc( sizeof( int ) * local_count );

		if ( __mpc_p_disguise_local_to_global_table == NULL )
		{
			perror( "malloc" );
			abort();
		}
	}

	mpc_mp_barrier( SCTK_COMM_WORLD );

	__mpc_p_disguise_costumes[my_id] = my_specific;
	__mpc_p_disguise_local_to_global_table[my_id] = mpc_common_get_task_rank();

	mpc_mp_barrier( SCTK_COMM_WORLD );

	return 0;
}

int MPCX_Disguise( mpc_mp_communicator_t comm, int target_rank )
{

	sctk_thread_data_t *th = __sctk_thread_data_get( 1 );

	if ( th->my_disguisement )
	{
		/* Sorry I'm already wearing a mask */
		return MPC_ERR_ARG;
	}

	/* Retrieve the ctx pointer */
	int cwr = sctk_get_comm_world_rank( (mpc_mp_communicator_t) comm, target_rank );

	int local_count = mpc_common_get_local_task_count();

	int i;

	for ( i = 0; i < local_count; ++i )
	{
		if ( __mpc_p_disguise_local_to_global_table[i] == cwr )
		{
			OPA_incr_int( &__mpc_p_disguise_flag );
			th->my_disguisement = __mpc_p_disguise_costumes[i]->thread_data;
			th->ctx_disguisement = (void *) __mpc_p_disguise_costumes[i];
			return SCTK_SUCCESS;
		}
	}

	return MPC_ERR_ARG;
}

int MPCX_Undisguise()
{
	sctk_thread_data_t *th = __sctk_thread_data_get( 1 );

	if ( th->my_disguisement == NULL )
		return MPC_ERR_ARG;

	th->my_disguisement = NULL;
	th->ctx_disguisement = NULL;
	OPA_decr_int( &__mpc_p_disguise_flag );

	return SCTK_SUCCESS;
}

/*******************************
 * MPC PER MPI PROCESS CONTEXT *
 *******************************/

#ifdef SCTK_PROCESS_MODE
struct mpc_mpi_m_per_mpi_process_ctx_s *___the_process_specific = NULL;
#endif

static inline int __mpc_cl_egreq_progress_init( mpc_mpi_m_per_mpi_process_ctx_t *tmp );

/** \brief Initalizes a structure of type \ref mpc_mpi_m_per_mpi_process_ctx_t
 */
static inline void ___mpc_cl_per_mpi_process_ctx_init( mpc_mpi_m_per_mpi_process_ctx_t *tmp )
{
	/* First empty the whole mpc_mpi_m_per_mpi_process_ctx_t */
	memset( tmp, 0, sizeof( mpc_mpi_m_per_mpi_process_ctx_t ) );

	tmp->thread_data = sctk_thread_data_get();
	tmp->progress_list = NULL;

	/* Set task id */
	tmp->task_id = mpc_common_get_task_rank();

	__mpc_cl_egreq_progress_init( tmp );

	mpc_mp_barrier( (mpc_mp_communicator_t) SCTK_COMM_WORLD );


	/* Initialize Data-type array */
	tmp->datatype_array = _mpc_dt_storage_init();

	/* Set initial per communicator data */
	mpc_per_communicator_t *per_comm_tmp;

	/* COMM_WORLD */
	per_comm_tmp = __mpc_cl_per_communicator_alloc();
	__mpc_cl_per_communicator_set( tmp, per_comm_tmp, SCTK_COMM_WORLD );

	/* COMM_SELF */
	per_comm_tmp = __mpc_cl_per_communicator_alloc();
	__mpc_cl_per_communicator_set( tmp, per_comm_tmp, SCTK_COMM_SELF );

	/* Propagate initialization to the thread context */
	mpc_mpi_m_per_thread_ctx_init();

	/* Set MPI status informations */
	tmp->init_done = 0;
	tmp->thread_level = -1;

	/* Create the MPI_Info factory */
	MPC_Info_factory_init( &tmp->info_fact );

	/* Create the context class handling structure */
	_mpc_egreq_classes_storage_init( &tmp->grequest_context );

	/* Clear exit handlers */
	tmp->exit_handlers = NULL;

#ifdef SCTK_PROCESS_MODE
	___the_process_specific = tmp;
#endif
}

/** \brief Relases a structure of type \ref mpc_mpi_m_per_mpi_process_ctx_t
 */
static inline void ___mpc_cl_per_mpi_process_ctx_release( mpc_mpi_m_per_mpi_process_ctx_t *tmp )
{
	/* Release the MPI_Info factory */
	MPC_Info_factory_release( &tmp->info_fact );

	/* Release the context class handling structure */
	_mpc_egreq_classes_storage_release( &tmp->grequest_context );
}

/** \brief Creation point for MPI task context in an \ref mpc_mpi_m_per_mpi_process_ctx_t
 *
 * Called from \ref mpc_mpi_m_mpi_process_main this function allocates and initialises
 * an mpc_mpi_m_per_mpi_process_ctx_t. It also takes care of storing it in the host
 * thread context.
 */
static inline void __mpc_cl_per_mpi_process_ctx_init()
{
	/* Retrieve the task ctx pointer */
	mpc_mpi_m_per_mpi_process_ctx_t *tmp;
	tmp = (mpc_mpi_m_per_mpi_process_ctx_t *) sctk_thread_getspecific( mpc_mpi_m_per_mpi_process_ctx );

	/* Make sure that it is not already present */
	sctk_assert( tmp == NULL );

	/* If not allocate a new mpc_mpi_m_per_mpi_process_ctx_t */
	tmp = (mpc_mpi_m_per_mpi_process_ctx_t *) sctk_malloc( sizeof( mpc_mpi_m_per_mpi_process_ctx_t ) );

	/* And initalize it */
	___mpc_cl_per_mpi_process_ctx_init( tmp );

	/* Set the mpc_mpi_m_per_mpi_process_ctx key in thread CTX */
	sctk_thread_setspecific( mpc_mpi_m_per_mpi_process_ctx, tmp );

	/* Initialize commond data-types */
	_mpc_dt_init();

	/* Register the task specific in the disguisemement array */
	__mpc_p_disguise_init( tmp );
}

/** \brief Define a function to be called when a task leaves
 *  \arg function Function to be callsed when task exits
 *  \return 1 on error 0 otherwise
 */
int mpc_mpi_m_per_mpi_process_ctx_at_exit_register( void ( *function )( void ) )
{
	/* Retrieve the task ctx pointer */
	mpc_mpi_m_per_mpi_process_ctx_t *tmp;
	tmp = (mpc_mpi_m_per_mpi_process_ctx_t *) sctk_thread_getspecific( mpc_mpi_m_per_mpi_process_ctx );

	if ( !tmp )
		return 1;

	struct mpc_mpi_m_per_mpi_process_ctx_atexit_s *new_exit =
		sctk_malloc( sizeof( struct mpc_mpi_m_per_mpi_process_ctx_atexit_s ) );

	if ( !new_exit )
	{
		return 1;
	}

	sctk_info( "Registering task-level atexit handler (%p)", function );

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
	mpc_mpi_m_per_mpi_process_ctx_t *tmp;
	tmp = (mpc_mpi_m_per_mpi_process_ctx_t *) sctk_thread_getspecific( mpc_mpi_m_per_mpi_process_ctx );

	if ( !tmp )
		return;

	struct mpc_mpi_m_per_mpi_process_ctx_atexit_s *current = tmp->exit_handlers;

	while ( current )
	{
		struct mpc_mpi_m_per_mpi_process_ctx_atexit_s *to_free = current;

		if ( current->func )
		{
			sctk_info( "Calling task-level atexit handler (%p)", current->func );
			( current->func )();
		}

		current = current->next;

		sctk_free( to_free );
	}
}

static inline int __mpc_cl_egreq_progress_release( mpc_mpi_m_per_mpi_process_ctx_t *tmp );

/** \brief Releases and frees task ctx
 *  Also called from  \ref mpc_mpi_m_mpi_process_main this function releases
 *  MPI task ctx and remove them from host thread keys
 */
static void __mpc_cl_per_mpi_process_ctx_release()
{
	/* Retrieve the ctx pointer */
	mpc_mpi_m_per_mpi_process_ctx_t *tmp;
	tmp = (mpc_mpi_m_per_mpi_process_ctx_t *) sctk_thread_getspecific( mpc_mpi_m_per_mpi_process_ctx );

	/* Clear progress */
	__mpc_cl_egreq_progress_release( tmp );


	/* Free the type array */
	_mpc_dt_storage_release( tmp->datatype_array );

	/* Release common type info */
	_mpc_dt_release();

	/* Call atexit handlers */
	__mpc_cl_per_mpi_process_ctx_at_exit_trigger();

	/* Remove the ctx reference in the host thread */
	sctk_thread_setspecific( mpc_mpi_m_per_mpi_process_ctx, NULL );

	/* Release the task ctx */
	___mpc_cl_per_mpi_process_ctx_release( tmp );

	/* Free the task ctx */
	sctk_free( tmp );
}

/** \brief Set current thread task specific context
 *  \param tmp New value to be set
 */
void __MPC_reinit_task_specific( struct mpc_mpi_m_per_mpi_process_ctx_s *tmp )
{
	sctk_thread_setspecific( mpc_mpi_m_per_mpi_process_ctx, tmp );
}

/** \brief Retrieves current thread task specific context
 */

static inline struct mpc_mpi_m_per_mpi_process_ctx_s *__mpc_cl_per_mpi_process_ctx_get()
{
#ifdef SCTK_PROCESS_MODE
	return ___the_process_specific;
#endif

	struct mpc_mpi_m_per_mpi_process_ctx_s *ret = NULL;
	int maybe_disguised = __MPC_Maybe_disguised();

	if ( maybe_disguised )
	{
		sctk_thread_data_t *th = __sctk_thread_data_get( 1 );
		if ( th->ctx_disguisement )
		{
			return th->ctx_disguisement;
		}
	}

	static __thread int last_rank = -2;
	static __thread struct mpc_mpi_m_per_mpi_process_ctx_s *last_specific = NULL;

	if ( last_rank == mpc_common_get_task_rank() )
	{
		if ( last_specific )
			return last_specific;
	}

	ret = (struct mpc_mpi_m_per_mpi_process_ctx_s *) sctk_thread_getspecific(mpc_mpi_m_per_mpi_process_ctx );

	last_specific = ret;
	last_rank = mpc_common_get_task_rank();

	return ret;
}

struct mpc_mpi_m_per_mpi_process_ctx_s *_mpc_cl_per_mpi_process_ctx_get()
{
	return __mpc_cl_per_mpi_process_ctx_get();
}


/********************************
 * PER PROCESS DATATYPE STORAGE *
 ********************************/


/** \brief Retrieves a pointer to a contiguous datatype from its datatype ID
 *
 *  \param task_specific Pointer to current task specific
 *  \param datatype datatype ID to be retrieved
 *
 */
_mpc_dt_contiguout_t * _mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
									         mpc_mp_datatype_t datatype )
{
	sctk_assert( task_specific != NULL );
	/* Return the pointed _mpc_dt_contiguout_t */
	return _mpc_dt_storage_get_contiguous_datatype( task_specific->datatype_array, datatype );
}

/** \brief Retrieves a pointer to a contiguous datatype from its datatype ID
 *
 *  \param datatype datatype ID to be retrieved
 *
 */
_mpc_dt_contiguout_t * _mpc_cl_per_mpi_process_ctx_contiguous_datatype_get( mpc_mp_datatype_t datatype )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	return _mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get(task_specific, datatype);
}

/** \brief Retrieves a pointer to a derived datatype from its datatype ID
 *
 *  \param task_specific Pointer to current task specific
 *  \param datatype datatype ID to be retrieved
 *
 */
_mpc_dt_derived_t * _mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
										 mpc_mp_datatype_t datatype )
{
	sctk_assert( task_specific != NULL );
	return _mpc_dt_storage_get_derived_datatype( task_specific->datatype_array, datatype );
}

/** \brief Retrieves a pointer to a derived datatype from its datatype ID
 *
 *  \param datatype datatype ID to be retrieved
 *
 */
_mpc_dt_derived_t *_mpc_cl_per_mpi_process_ctx_derived_datatype_get( mpc_mp_datatype_t datatype )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	sctk_assert( task_specific != NULL );
	return _mpc_dt_storage_get_derived_datatype( task_specific->datatype_array, datatype );
}


/** \brief Removed a derived datatype from the datatype array
 *
 *  \param task_specific Pointer to current task specific
 *  \param datatype datatype ID to be removed from datatype array
 */
static inline void __mpc_cl_per_mpi_process_ctx_derived_datatype_set(
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific, mpc_mp_datatype_t datatype,
	_mpc_dt_derived_t *value )
{
	sctk_assert( task_specific != NULL );
	_mpc_dt_storage_set_derived_datatype( task_specific->datatype_array, datatype,  value );
}

/** \brief This function allows the retrieval of a data-type context
 *  \param datatype Target datatype
 *  \return NULL on error the context otherwise
 */
static inline struct _mpc_dt_footprint *__mpc_cl_datatype_get_ctx( mpc_mp_datatype_t datatype )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	_mpc_dt_contiguout_t *target_contiguous_type;
	_mpc_dt_derived_t *target_derived_type;

	switch ( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			/* Nothing to do here */
			return NULL;
			break;

		case MPC_DATATYPES_CONTIGUOUS:

			/* Get a pointer to the type of interest */
			target_contiguous_type =
				_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get( task_specific, datatype );
			return &target_contiguous_type->context;
			break;

		case MPC_DATATYPES_DERIVED:

			/* Get a pointer to the type of interest */
			target_derived_type =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );
			return &target_derived_type->context;
			break;

		case MPC_DATATYPES_UNKNOWN:
			return NULL;
			break;
	}

	return NULL;
}

/********************
 * MPC REDUCE FUNCS *
 ********************/

#undef MPC_CREATE_INTERN_FUNC
#define MPC_CREATE_INTERN_FUNC( name ) \
	const sctk_Op MPC_##name = {MPC_##name##_func, NULL}

MPC_CREATE_INTERN_FUNC( SUM );
MPC_CREATE_INTERN_FUNC( MAX );
MPC_CREATE_INTERN_FUNC( MIN );
MPC_CREATE_INTERN_FUNC( PROD );
MPC_CREATE_INTERN_FUNC( LAND );
MPC_CREATE_INTERN_FUNC( BAND );
MPC_CREATE_INTERN_FUNC( LOR );
MPC_CREATE_INTERN_FUNC( BOR );
MPC_CREATE_INTERN_FUNC( LXOR );
MPC_CREATE_INTERN_FUNC( BXOR );
MPC_CREATE_INTERN_FUNC( MINLOC );
MPC_CREATE_INTERN_FUNC( MAXLOC );

/************************************************************************/
/* Error Reporting                                                      */
/************************************************************************/

static inline int __MPC_ERROR_REPORT__( mpc_mp_communicator_t comm, int error, char *message,
										char *file, int line )
{
	mpc_mp_communicator_t comm_id;
	int error_id;

	MPC_Errhandler errh = (MPC_Errhandler) sctk_handle_get_errhandler(
		(sctk_handle) comm, SCTK_HANDLE_COMM );

	MPC_Handler_function *func = sctk_errhandler_resolve( errh );
	error_id = error;
	( func )( &comm_id, &error_id, message, file, line );

	return error;
}

#define MPC_ERROR_REPORT( comm, error, message ) \
	return __MPC_ERROR_REPORT__( comm, error, message, __FILE__, __LINE__ )

#define MPC_ERROR_SUCESS() return SCTK_SUCCESS;


/*******************
 * VARIOUS GETTERS *
 *******************/


static void __mpc_cl_disable_buffering()
{
	if ( mpc_common_get_task_rank() == 0 )
	{
		sctk_warning( "Message Buffering has been disabled in configuration" );
	}
	__mpc_cl_buffering_enabled = 0;
}


/***************************************
 * GENERALIZED REQUEST PROGRESS ENGINE *
 ***************************************/

static struct _mpc_egreq_progress_pool __mpc_progress_pool;

static inline int __mpc_cl_egreq_progress_init( mpc_mpi_m_per_mpi_process_ctx_t *tmp )
{

	int my_local_id = mpc_common_get_local_task_rank();
	if ( my_local_id == 0 )
	{
		int local_count = mpc_common_get_local_task_count();
		_mpc_egreq_progress_pool_init( &__mpc_progress_pool, local_count );
	}

	mpc_mp_barrier( SCTK_COMM_WORLD );

	/* Retrieve the ctx pointer */
	tmp->progress_list = _mpc_egreq_progress_pool_join( &__mpc_progress_pool );

	return SCTK_SUCCESS;
}

static inline int __mpc_cl_egreq_progress_release( mpc_mpi_m_per_mpi_process_ctx_t *tmp )
{
	static mpc_common_spinlock_t l = 0;
	static mpc_common_spinlock_t d = 0;

	tmp->progress_list = NULL;

	int done = 0;
	mpc_common_spinlock_lock( &l );
	done = d;
	d = 1;
	mpc_common_spinlock_unlock( &l );

	if ( done )
		return SCTK_SUCCESS;

	_mpc_egreq_progress_pool_release( &__mpc_progress_pool );
	return SCTK_SUCCESS;
}

static inline void __mpc_cl_egreq_progress_poll_id( int id )
{
	_mpc_egreq_progress_pool_poll( &__mpc_progress_pool, id );
}

static inline struct mpc_mpi_m_per_mpi_process_ctx_s *__mpc_cl_per_mpi_process_ctx_get();

void mpc_mpi_m_egreq_progress_poll()
{
	struct mpc_mpi_m_per_mpi_process_ctx_s *spe = __mpc_cl_per_mpi_process_ctx_get();
	__mpc_cl_egreq_progress_poll_id( spe->progress_list->id );
}


/************************************************************************/
/* Extended Generalized Requests                                        */
/************************************************************************/

int ___mpc_cl_egreq_disguise_poll( void *arg )
{
	int ret = 0;
	//sctk_error("POLL %p", arg);
	int disguised = 0;
	int my_rank = -1;
	mpc_mp_request_t *req = (mpc_mp_request_t *) arg;

	if ( !req->poll_fn )
	{
		ret = 0;
		goto POLL_DONE_G;
	}

	if ( req->completion_flag == SCTK_MESSAGE_DONE )
	{
		ret = 1;
		goto POLL_DONE_G;
	}

	my_rank = mpc_common_get_task_rank();

	if ( my_rank != req->grequest_rank )
	{
		disguised = 1;
		if ( MPCX_Disguise( SCTK_COMM_WORLD, req->grequest_rank ) != SCTK_SUCCESS )
		{
			ret = 0;
			goto POLL_DONE_G;
		}
	}

	mpc_mp_status_t tmp_status;
	( req->poll_fn )( req->extra_state, &tmp_status );

	ret = ( req->completion_flag == SCTK_MESSAGE_DONE );

POLL_DONE_G:
	if ( disguised )
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
static inline int __mpc_cl_egreq_generic_start( MPC_Grequest_query_function *query_fn,
						MPC_Grequest_free_function *free_fn,
						MPC_Grequest_cancel_function *cancel_fn,
						MPCX_Grequest_poll_fn *poll_fn,
						MPCX_Grequest_wait_fn *wait_fn,
						void *extra_state, mpc_mp_request_t *request )
{
	if ( request == NULL )
		MPC_ERROR_REPORT( SCTK_COMM_SELF, MPC_ERR_ARG,
						  "Bad request passed to MPC_Grequest_start" );

	/* Initialized as a NULL request */
	memcpy( request, &mpc_request_null, sizeof( mpc_mp_request_t ) );

	/* Change type */
	request->request_type = REQUEST_GENERALIZED;
	/* Set not null as we want to be waited */
	request->is_null = 0;

	request->grequest_rank = mpc_common_get_task_rank();

	/* Fill in generalized request CTX */
	request->pointer_to_source_request = (void *) request;
	request->query_fn = query_fn;
	request->free_fn = free_fn;
	request->cancel_fn = cancel_fn;
	request->poll_fn = poll_fn;
	request->wait_fn = wait_fn;
	request->extra_state = extra_state;

	/* We set the request as pending */
	request->completion_flag = SCTK_MESSAGE_PENDING;

	/* We now push the request inside the progress list */
	struct mpc_mpi_m_per_mpi_process_ctx_s *ctx = __mpc_cl_per_mpi_process_ctx_get();
	request->progress_unit = NULL;
	request->progress_unit = (void *) _mpc_egreq_progress_list_add( ctx->progress_list, ___mpc_cl_egreq_disguise_poll, (void *) request );

	MPC_ERROR_SUCESS()
}

/** \brief Starts a generalized request with a polling function
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param poll_fn Polling function called by the MPI runtime
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
int _mpc_cl_egrequest_start( MPC_Grequest_query_function *query_fn,
			MPC_Grequest_free_function *free_fn,
			MPC_Grequest_cancel_function *cancel_fn,
			MPCX_Grequest_poll_fn *poll_fn, void *extra_state,
			mpc_mp_request_t *request )
{
	return __mpc_cl_egreq_generic_start( query_fn, free_fn, cancel_fn, poll_fn,
					    NULL, extra_state, request );
}
/************************************************************************/
/* Extended Generalized Requests Request Classes                        */
/************************************************************************/

/** \brief This creates a request class which can be referred to later on
 *
 *  \param query_fn Function used to fill in the status information
 *  \param free_fn Function used to free the extra_state dummy object
 *  \param cancel_fn Function called when the request is cancelled
 *  \param poll_fn Polling function called by the MPI runtime (CAN BE NULL)
 *  \param wait_fn Wait function called when the runtime waits several requests
 * of the same type (CAN BE NULL)
 *  \param new_class The identifier of the class we are creating (output)
 */
int _mpc_cl_grequest_class_create( MPC_Grequest_query_function *query_fn,
								 MPC_Grequest_cancel_function *cancel_fn,
								 MPC_Grequest_free_function *free_fn,
								 MPCX_Grequest_poll_fn *poll_fn,
								 MPCX_Grequest_wait_fn *wait_fn,
								 MPCX_Request_class *new_class )
{
	/* Retrieve task context */
	struct mpc_mpi_m_per_mpi_process_ctx_s *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	assume( task_specific != NULL );
	struct _mpc_egreq_classes_storage *class_ctx = &task_specific->grequest_context;

	/* Register the class */
	return _mpc_egreq_classes_storage_add_class( class_ctx, query_fn, cancel_fn, free_fn,
					   poll_fn, wait_fn, new_class );
}

/** \brief Create a request linked to an \ref MPCX_Request_class type
 *
 *  \param target_class Identifier of the class of work we launch as created by
 * \ref PMPIX_GRequest_class_create
 *  \param extra_state Extra pointer passed to every handler
 *  \param request The request object we are creating (output)
 */
int _mpc_cl_grequest_class_allocate( MPCX_Request_class target_class,
								   void *extra_state, mpc_mp_request_t *request )
{
	/* Retrieve task context */
	struct mpc_mpi_m_per_mpi_process_ctx_s *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	assume( task_specific != NULL );
	struct _mpc_egreq_classes_storage *class_ctx = &task_specific->grequest_context;

	/* Retrieve task description */
	MPCX_GRequest_class_t *class_desc =
		_mpc_egreq_classes_storage_get_class( class_ctx, target_class );

	/* Not found */
	if ( !class_desc )
		MPC_ERROR_REPORT(
			SCTK_COMM_SELF, MPC_ERR_ARG,
			"Bad MPCX_Request_class passed to PMPIX_Grequest_class_allocate" );

	return __mpc_cl_egreq_generic_start(
		class_desc->query_fn, class_desc->free_fn, class_desc->cancel_fn,
		class_desc->poll_fn, class_desc->wait_fn, extra_state, request );
}

/************************************************************************/
/* Generalized Requests                                                 */
/************************************************************************/

/** \brief This function starts a generic request
*
* \param query_fn Query function called to fill the status object
* \param free_fn Free function which frees extra arg
* \param cancel_fn Function called when the request is canceled
* \param extra_state Extra context passed to every handlers
* \param request New request to be created
*
* \warning Generalized Requests are not progressed by MPI the user are
*          in charge of providing the progress mechanism.
*
* Once the operation completes, the user has to call \ref _mpc_cl_grequest_complete
*
*/
int _mpc_cl_grequest_start( MPC_Grequest_query_function *query_fn,
						 MPC_Grequest_free_function *free_fn,
						 MPC_Grequest_cancel_function *cancel_fn,
						 void *extra_state, mpc_mp_request_t *request )
{
	return __mpc_cl_egreq_generic_start( query_fn, free_fn, cancel_fn, NULL, NULL,
					    extra_state, request );
}

/** \brief Flag a Generalized Request as finished
* \param request Request we want to finish
*/
int _mpc_cl_grequest_complete( mpc_mp_request_t request )
{

	mpc_mp_request_t *src_req = (mpc_mp_request_t *) request.pointer_to_source_request;

	struct _mpc_egreq_progress_work_unit *pwu = ( (struct _mpc_egreq_progress_work_unit *) src_req->progress_unit );

	pwu->done = 1;

	/* We have to do this as request complete takes
           a copy of the request ... but we want
           to modify the original request which is being polled ... */
	src_req->completion_flag = SCTK_MESSAGE_DONE;

	MPC_ERROR_SUCESS()
}

/************************************************************************/
/* Datatype Handling                                                    */
/************************************************************************/

/* Utilities functions for this section are located in mpc_datatypes.[ch] */

/** \brief This fuction releases a datatype
 *
 *  This function call the right release function relatively to the
 *  datatype kind (contiguous or derived) and then set the freed
 *  datatype to MPC_DATATYPE_NULL
 *
 */
int _mpc_cl_type_free( mpc_mp_datatype_t *datatype_p )
{
	/* Dereference the datatype pointer for convenience */
	mpc_mp_datatype_t datatype = *datatype_p;

	SCTK_PROFIL_START( MPC_Type_free );

	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	if ( datatype == MPC_DATATYPE_NULL || datatype == MPC_LB ||
		 datatype == MPC_UB )
	{
		return MPC_ERR_TYPE;
	}

	/* Is the datatype NULL or PACKED ? */
	if ( datatype == MPC_PACKED )
	{
		/* ERROR */
		SCTK_PROFIL_END( MPC_Type_free );
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_TYPE,
						  "You tried to free an MPI_PACKED datatype" );
	}

	_mpc_dt_derived_t *derived_type_target = NULL;
	_mpc_dt_contiguout_t *continuous_type_target = NULL;
	/* Choose what to do in function of the datatype kind */
	switch ( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			/* You are not supposed to free predefined datatypes */
			SCTK_PROFIL_END( MPC_Type_free );
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_TYPE,
							  "You are not allowed to free a COMMON datatype" );
			break;

		case MPC_DATATYPES_CONTIGUOUS:
			/* Free a contiguous datatype */


			/* Retrieve a pointer to the type to be freed */
			continuous_type_target =
				_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get( task_specific, datatype );

			/* Free it */
			_mpc_dt_contiguous_release( continuous_type_target );

			break;

		case MPC_DATATYPES_DERIVED:
			/* Free a derived datatype */


			/* Retrieve a pointer to the type to be freed */
			derived_type_target =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );


			/* Check if it is really allocated */
			if ( !derived_type_target )
			{
				/* ERROR */
				MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG,
								  "Tried to release an uninitialized datatype" );
			}

			sctk_nodebug( "Free derived [%d] contructor %d", datatype,
						  derived_type_target->context.combiner );

			/* Free it  (not that the container is also freed in this function */
			if ( _mpc_dt_derived_release( derived_type_target ) )
			{
				/* Set the pointer to the freed datatype to NULL in the derived datatype
       * array */
				__mpc_cl_per_mpi_process_ctx_derived_datatype_set( task_specific, datatype, NULL );
			}

			break;

		case MPC_DATATYPES_UNKNOWN:
			/* If we are here the type provided must have been erroneous */
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG,
							  "An unknown datatype was provided" );
			break;
	}

	/* Set the source datatype as freed */
	*datatype_p = MPC_DATATYPE_NULL;

	SCTK_PROFIL_END( MPC_Type_free );
	MPC_ERROR_SUCESS();
}

/** \brief Set a name to an mpc_mp_datatype_t
 *  \param datatype Datatype to be named
 *  \param name Name to be set
 */
int _mpc_cl_type_set_name( mpc_mp_datatype_t datatype, char *name )
{
	if ( sctk_datype_set_name( datatype, name ) )
	{
		MPC_ERROR_REPORT( SCTK_COMM_SELF, MPC_ERR_INTERN, "Could not set name" );
	}

	MPC_ERROR_SUCESS();
}

/** \brief Duplicate a  datatype
 *  \param old_type Type to be duplicated
 *  \param new_type Copy of old_type
 */
int _mpc_cl_type_dup( mpc_mp_datatype_t old_type, mpc_mp_datatype_t *new_type )
{
	_mpc_dt_derived_t *derived_type_target;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Set type context */
	struct _mpc_dt_context dtctx;
	_mpc_dt_context_clear( &dtctx );
	dtctx.combiner = MPC_COMBINER_DUP;
	dtctx.oldtype = old_type;

	switch ( _mpc_dt_get_kind( old_type ) )
	{
		case MPC_DATATYPES_COMMON:
		/* We dup common datatypes in contiguous ones in order
   * to have a context where to put the DUP combiner */
		case MPC_DATATYPES_CONTIGUOUS:
			/* Create a type consisting in one entry of the contiguous block
       here as the combiner is set to be a dup it wont reuse the previous
       contiguous that we are dupping from unless it has already been duped */
			_mpc_cl_type_hcontiguous_ctx( new_type, 1, &old_type, &dtctx );
			break;
		case MPC_DATATYPES_DERIVED:
			/* Here we just build a copy of the derived data-type */
			derived_type_target =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, old_type );
			_mpc_cl_derived_datatype(
				new_type, derived_type_target->begins, derived_type_target->ends,
				derived_type_target->datatypes, derived_type_target->count,
				derived_type_target->lb, derived_type_target->is_lb,
				derived_type_target->ub, derived_type_target->is_ub, &dtctx );
			break;
		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT( SCTK_COMM_SELF, MPC_ERR_ARG, "Bad data-type" );
			break;
	}

	MPC_ERROR_SUCESS();
}

/** \brief Set a name to an mpc_mp_datatype_t
 *  \param datatype Datatype to get the name of
 *  \param name Datatype name (OUT)
 *  \param resultlen Maximum length of the target buffer (OUT)
 */
int _mpc_cl_type_get_name( mpc_mp_datatype_t datatype, char *name, int *resultlen )
{
	char *retname = sctk_datype_get_name( datatype );

	if ( datatype == MPC_UB )
	{
		retname = "MPI_UB";
	}
	else if ( datatype == MPC_LB )
	{
		retname = "MPI_LB";
	}

	if ( !retname )
	{
		/* Return an empty string */
		name[0] = '\0';
		*resultlen = 0;
	}
	else
	{
		sprintf( name, "%s", retname );
		*resultlen = strlen( name );
	}

	MPC_ERROR_SUCESS();
}

/** \brief This call is used to fill the envelope of an MPI type
 *
 *  \param ctx Source context
 *  \param num_integers Number of input integers [OUT]
 *  \param num_addresses Number of input addresses [OUT]
 *  \param num_datatypes Number of input datatypes [OUT]
 *  \param combiner Combiner used to build the datatype [OUT]
 *
 *
 */
int _mpc_cl_type_get_envelope( mpc_mp_datatype_t datatype, int *num_integers,
				int *num_addresses, int *num_datatypes,
				int *combiner )
{
	*num_integers = 0;
	*num_addresses = 0;
	*num_datatypes = 0;

	/* Handle the common data-type case */
	if ( _mpc_dt_is_common( datatype ) ||
		 _mpc_dt_is_boundary( datatype ) )
	{
		*combiner = MPC_COMBINER_NAMED;
		*num_integers = 0;
		*num_addresses = 0;
		*num_datatypes = 0;
		MPC_ERROR_SUCESS();
	}

	struct _mpc_dt_footprint *dctx = __mpc_cl_datatype_get_ctx( datatype );

	if ( !dctx )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN,
						  "This datatype is unknown" );
	}

	_mpc_dt_fill_envelope( dctx, num_integers, num_addresses, num_datatypes,
								 combiner );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_type_get_contents( mpc_mp_datatype_t datatype, int max_integers,
							int max_addresses, int max_datatypes,
							int array_of_integers[],
							size_t array_of_addresses[],
							mpc_mp_datatype_t array_of_datatypes[] )
{
	/* First make sure we were not called on a common type */
	if ( _mpc_dt_is_common( datatype ) )
	{
		MPC_ERROR_REPORT( SCTK_COMM_SELF, MPC_ERR_ARG,
						  "Cannot call MPI_Type_get_contents on a defaut datatype" );
	}

	/* Retrieve the context */
	struct _mpc_dt_footprint *dctx = __mpc_cl_datatype_get_ctx( datatype );

	assume( dctx != NULL );

	/* Then make sure the user actually called MPI_Get_envelope
  * with the same datatype or that we have enough room */
	int n_int = 0;
	int n_addr = 0;
	int n_type = 0;
	int dummy_combiner;

	_mpc_dt_fill_envelope( dctx, &n_int, &n_addr, &n_type, &dummy_combiner );

	if ( ( max_integers < n_int ) || ( max_addresses < n_addr ) ||
		 ( max_datatypes < n_type ) )
	{
		/* Not enough room */
		MPC_ERROR_REPORT(
			SCTK_COMM_SELF, MPC_ERR_ARG,
			"Cannot call MPI_Type_get_contents as it would overflow target arrays" );
	}

	/* Now we just copy back the content from the context */
	if ( array_of_integers )
		memcpy( array_of_integers, dctx->array_of_integers, n_int * sizeof( int ) );

	if ( array_of_addresses )
		memcpy( array_of_addresses, dctx->array_of_addresses,
				n_addr * sizeof( size_t ) );

	/* Flag the datatypes as duplicated */
	int i;
	for ( i = 0; i < n_type; i++ )
		_mpc_cl_type_use( dctx->array_of_types[i] );

	if ( array_of_datatypes )
		memcpy( array_of_datatypes, dctx->array_of_types,
				n_type * sizeof( mpc_mp_datatype_t ) );

	MPC_ERROR_SUCESS();
}

/** \brief Retrieves datatype size
 *
 *  Returns the right size field in function of datype kind
 *
 *  \param datatype datatype which size is requested
 *  \param task_specific a pointer to task CTX
 *
 *  \return the size of the datatype or aborts
 */
static inline size_t __mpc_cl_datatype_get_size( mpc_mp_datatype_t datatype,
						mpc_mpi_m_per_mpi_process_ctx_t *task_specific )
{

	/* Common types bypass */
	switch(datatype)
	{
		/* FASTPATH */
		case MPC_BYTE:
			return sizeof(char);
		case MPC_DOUBLE:
			return sizeof(double);
		case MPC_FLOAT:
			return sizeof(float);
		case MPC_INT:
			return sizeof(int);

		/* Exceptions */

		case MPC_DATATYPE_NULL:
		/* Here we return 0 for data-type null
                   in order to pass the struct-zero-count test */
		case MPC_UB:
		case MPC_LB:
			return 0;
		case MPC_PACKED:
			return 1;

	}

	size_t ret;
	_mpc_dt_contiguout_t *contiguous_type_target;
	_mpc_dt_derived_t *derived_type_target;

	/* Compute the size in function of the datatype */
	switch ( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			/* Common datatype sizes */

			return _mpc_dt_common_get_size( datatype );
			break;

		case MPC_DATATYPES_CONTIGUOUS:
			/* Contiguous datatype size */

			/* Get a pointer to the type of interest */
			contiguous_type_target =
				_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get( task_specific, datatype );
			/* Extract its size field */
			ret = contiguous_type_target->size;

			/* Return */
			return ret;
			break;

		case MPC_DATATYPES_DERIVED:
			/* Derived datatype size */

			/* Get a pointer to the object of interest */
			derived_type_target =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );

			/* Is it allocated ? */
			if ( !derived_type_target )
			{
				/* ERROR */
				sctk_fatal( "Tried to retrieve an uninitialized datatype %d", datatype );
			}

			if ( derived_type_target->is_a_padded_struct )
			{

				/* Here we return UB as the size (padded struct) */
				ret = derived_type_target->ub;
			}
			else
			{
				/* Extract the size field */
				ret = derived_type_target->size;
			}

			/* Return */
			return ret;
			break;

		case MPC_DATATYPES_UNKNOWN:
			/* No such datatype */
			/* ERROR */
			sctk_fatal( "An unknown datatype was provided" );
			break;
	}

	/* We shall never arrive here ! */
	sctk_fatal( "This error shall never be reached" );
	return MPC_ERR_INTERN;
}

int _mpc_cl_type_get_true_extent( mpc_mp_datatype_t datatype, size_t *true_lb,
							   size_t *true_extent )
{
	_mpc_dt_derived_t *derived_type_target;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	mpc_pack_absolute_indexes_t tmp_true_lb;
	mpc_pack_absolute_indexes_t tmp_true_ub;

	/* Special cases */
	if ( datatype == MPC_DATATYPE_NULL )
	{
		/* Here we return 0 for data-type null
     * in order to pass the struct-zero-count test */
		*true_lb = 0;
		*true_extent = 0;
		MPC_ERROR_SUCESS();
	}

	switch ( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
		case MPC_DATATYPES_CONTIGUOUS:
			tmp_true_lb = 0;
			tmp_true_ub = __mpc_cl_datatype_get_size( datatype, task_specific );
			break;
		case MPC_DATATYPES_DERIVED:
			derived_type_target =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );
			_mpc_dt_derived_true_extend( derived_type_target, &tmp_true_lb,
											   &tmp_true_ub );
			break;
		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT( SCTK_COMM_SELF, MPC_ERR_ARG, "Bad data-type" );
			break;
	}

	*true_lb = tmp_true_lb;
	*true_extent = ( tmp_true_ub - tmp_true_lb ) + 1;

	MPC_ERROR_SUCESS();
}

/** \brief Checks if a datatype has already been released
 *  \param datatype target datatype
 *  \param flag 1 if the type is allocated [OUT]
 */
int _mpc_cl_type_is_allocated( mpc_mp_datatype_t datatype, int *flag )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	*flag = _mpc_dt_storage_type_can_be_released(task_specific->datatype_array, datatype);

	MPC_ERROR_SUCESS();
}

/** \brief Set a Struct datatype as a padded one to return the extent instead of
 * the size
 *  \param datatype to be flagged as padded
 */
int _mpc_cl_type_flag_padded( mpc_mp_datatype_t datatype )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	_mpc_dt_derived_t *derived_type_target = NULL;

	switch ( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
		case MPC_DATATYPES_CONTIGUOUS:
		case MPC_DATATYPES_UNKNOWN:
			sctk_fatal( "Only Common datatypes can be flagged" );
			break;
		case MPC_DATATYPES_DERIVED:
			derived_type_target =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );

			if ( derived_type_target )
			{
				derived_type_target->is_a_padded_struct = 1;
			}
			else
			{
				return MPC_ERR_ARG;
			}

			break;
	}

	MPC_ERROR_SUCESS();
}

/** \brief Compute the size of a \ref mpc_mp_datatype_t
 *  \param datatype target datatype
 *  \param size where to write the size of datatype
 */
int _mpc_cl_type_size( mpc_mp_datatype_t datatype, size_t *size )
{
	SCTK_PROFIL_START( MPC_Type_size );

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	*size = __mpc_cl_datatype_get_size( datatype, task_specific );

	SCTK_PROFIL_END( MPC_Type_size );
	MPC_ERROR_SUCESS();
}

static inline int __mpc_cl_type_contiguous_check( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
						struct _mpc_dt_context *ctx,
						mpc_mp_datatype_t *datatype )
{
	int i;
	/* see if such a datatype is not already allocated */
	for ( i = 0; i < SCTK_USER_DATA_TYPES_MAX; i++ )
	{
		/* For each contiguous type slot */
		_mpc_dt_contiguout_t *current_type =
			_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get(
				task_specific, _MPC_DT_MAP_FROM_CONTIGUOUS( i ) );

		/* We are only interested in allocated types */
		if ( _MPC_DT_CONTIGUOUS_IS_FREE( current_type ) )
			continue;

		/* If types match */
		if ( _mpc_dt_footprint_match( ctx, &current_type->context ) )
		{
			/* Add a reference to this data-type and we are done */
			_mpc_cl_type_use( _MPC_DT_MAP_FROM_CONTIGUOUS( i ) );

			*datatype = _MPC_DT_MAP_FROM_CONTIGUOUS( i );

			return 1;
		}
	}

	return 0;
}

static inline int __mpc_cl_type_derived_check( mpc_mpi_m_per_mpi_process_ctx_t *task_specific,
						struct _mpc_dt_context *ctx,
						mpc_mp_datatype_t *datatype )
{
	int i;

	/* try to find a data-type with the same footprint in derived  */
	for ( i = MPC_STRUCT_DATATYPE_COUNT; i < SCTK_USER_DATA_TYPES_MAX; i++ )
	{
		/* For each datatype */
		_mpc_dt_derived_t *current_user_type =
			_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific,
									    _MPC_DT_MAP_FROM_DERIVED( i ) );

		/* Is the slot NOT free ? */
		if ( current_user_type != NULL )
		{

			/* If types match */
			if ( _mpc_dt_footprint_match( ctx, &current_user_type->context ) )
			{
				/* Add a reference to this data-type and we are done */
				_mpc_cl_type_use( _MPC_DT_MAP_FROM_DERIVED( i ) );

				*datatype = _MPC_DT_MAP_FROM_DERIVED( i );

				return 1;
			}
		}
	}

	return 0;
}

/** \brief This function is the generic initializer for
 * _mpc_dt_contiguout_t
 *  Creates a contiguous datatypes of count data_in while checking for unicity
 *
 *  \param datatype Output datatype to be created
 *  \param count Number of entries of type data_in
 *  \param data_in Type of the entry to be created
 *  \param ctx Context of the new data-type in order to allow unicity check
 *
 */
int _mpc_cl_type_hcontiguous_ctx( mpc_mp_datatype_t *datatype, size_t count,
				mpc_mp_datatype_t *data_in,
				struct _mpc_dt_context *ctx )
{
	SCTK_PROFIL_START( MPC_Type_hcontiguous );

	/* Retrieve task specific context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Set the datatype to NULL in case we do not find a slot */
	*datatype = MPC_DATATYPE_NULL;


	/* Retrieve size per element */
	size_t size;
	_mpc_cl_type_size( *data_in, &size );

	int i;

	if ( __mpc_cl_type_contiguous_check( task_specific, ctx, datatype ) )
	{
		MPC_ERROR_SUCESS();
	}

	/* We did not find a previously defined type with the same footprint
	   Now lets try to find a free slot in the array */
	for ( i = 0; i < SCTK_USER_DATA_TYPES_MAX; i++ )
	{
		/* For each contiguous type slot */
		_mpc_dt_contiguout_t *current_type =
			_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get(
				task_specific, _MPC_DT_MAP_FROM_CONTIGUOUS( i ) );

		/* Are you free ? */
		if ( _MPC_DT_CONTIGUOUS_IS_FREE( current_type ) )
		{
			/* Yes !*/
			/* Here we create an id falling in the continuous datatype range */
			int new_id = _MPC_DT_MAP_FROM_CONTIGUOUS( i );

			/* Set the news datatype to the continuous ID we have just allocated */
			*datatype = new_id;

			/* Initialize the datatype */
			_mpc_dt_contiguous_init( current_type, new_id, size, count,
										   *data_in );

			/* Set context */
			_mpc_cl_type_ctx_set( *datatype, ctx );

			/* Increment target datatype refcounter here we do it once as there is
			    only a single datatype */
			// _mpc_cl_type_use( *data_in );


			SCTK_PROFIL_END( MPC_Type_hcontiguous );

			/* We are done return */
			MPC_ERROR_SUCESS();
		}
	}

	/* If we are here we did not find any slot so we abort you might think of
   * increasing
   * SCTK_USER_DATA_TYPES_MAX if you app needs more than 265 datatypes =) */
	sctk_fatal( "Not enough datatypes allowed : you requested to many contiguous "
				"types (forgot to free ?)" );

	return -1;
}

/** \brief This function is the generic initializer for
 * _mpc_dt_contiguout_t
 *  Creates a contiguous datatypes of count data_in
 *
 *  \param datatype Output datatype to be created
 *  \param count Number of entries of type data_in
 *  \param data_in Type of the entry to be created
 *
 */
int _mpc_cl_type_hcontiguous( mpc_mp_datatype_t *datatype, size_t count,
						   mpc_mp_datatype_t *data_in )
{
	/* HERE WE SET A DEFAULT CONTEXT */
	struct _mpc_dt_context dtctx;
	_mpc_dt_context_clear( &dtctx );
	dtctx.combiner = MPC_COMBINER_CONTIGUOUS;
	dtctx.count = count;
	dtctx.oldtype = *data_in;

	return _mpc_cl_type_hcontiguous_ctx( datatype, count, data_in, &dtctx );
}

int _mpc_cl_type_commit( mpc_mp_datatype_t *datatype )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	_mpc_dt_derived_t *target_derived_type;

	if ( *datatype == MPC_DATATYPE_NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG,
						  "You are trying to commit a NULL data-type" );
	}

	switch ( _mpc_dt_get_kind( *datatype ) )
	{
		case MPC_DATATYPES_COMMON:
		case MPC_DATATYPES_CONTIGUOUS:
			/* Nothing to do here */
			MPC_ERROR_SUCESS();
			break;

		case MPC_DATATYPES_DERIVED:

			/* Get a pointer to the type of interest */
			target_derived_type =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, *datatype );

			/* OPTIMIZE */
			_mpc_dt_derived_optimize( target_derived_type );

			MPC_ERROR_SUCESS();
			break;

		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN,
							  "This datatype is unknown" );
			break;
	}

	MPC_ERROR_SUCESS();
}

int _mpc_cl_type_debug( mpc_mp_datatype_t datatype )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	_mpc_dt_derived_t *target_derived_type;
	_mpc_dt_contiguout_t *contiguous_type;

	if ( datatype == MPC_DATATYPE_NULL )
	{
		sctk_error( "=============ERROR==================" );
		sctk_error( "MPC_DATATYPE_NULL" );
		sctk_error( "====================================" );
		MPC_ERROR_SUCESS();
	}

	switch ( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			_mpc_dt_common_display( datatype );
			break;
		case MPC_DATATYPES_CONTIGUOUS:
			contiguous_type =
				_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get( task_specific, datatype );
			_mpc_dt_contiguous_display( contiguous_type );
			break;

		case MPC_DATATYPES_DERIVED:
			target_derived_type =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );
			_mpc_dt_derived_display( target_derived_type );
			break;

		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN,
							  "This datatype is unknown" );
			break;
	}

	MPC_ERROR_SUCESS();
}

/** \brief This function increases the refcounter for a datatype
 *
 *  \param datatype Target datatype
 *
 *  \warning We take no lock here thus the datatype lock shall be held
 *  when manipulating such objects
 */
int _mpc_cl_type_use( mpc_mp_datatype_t datatype )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	_mpc_dt_contiguout_t *target_contiguous_type;
	_mpc_dt_derived_t *target_derived_type;

	if ( _mpc_dt_is_boundary( datatype ) )
		MPC_ERROR_SUCESS();

	switch ( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			/* Nothing to do here */
			MPC_ERROR_SUCESS();
			break;

		case MPC_DATATYPES_CONTIGUOUS:

			/* Get a pointer to the type of interest */
			target_contiguous_type =
				_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get( task_specific, datatype );
			/* Increment the refcounter */
			target_contiguous_type->ref_count++;
			sctk_nodebug( "Type %d Refcount %d", datatype,
						  target_contiguous_type->ref_count );

			break;

		case MPC_DATATYPES_DERIVED:

			/* Get a pointer to the type of interest */
			target_derived_type =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );
			/* Increment the refcounter */
			target_derived_type->ref_count++;
			sctk_nodebug( "Type %d Refcount %d", datatype,
						  target_derived_type->ref_count );

			break;

		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN,
							  "This datatype is unknown" );
			break;
	}

	MPC_ERROR_SUCESS();
}

int _mpc_cl_derived_datatype_on_slot( int id, mpc_pack_absolute_indexes_t *begins,
				mpc_pack_absolute_indexes_t *ends,
				mpc_mp_datatype_t *types, unsigned long count,
				mpc_pack_absolute_indexes_t lb, int is_lb,
				mpc_pack_absolute_indexes_t ub, int is_ub )
{
	SCTK_PROFIL_START( MPC_Derived_datatype );

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Here we allocate the new derived datatype */
	_mpc_dt_derived_t *new_type =
		(_mpc_dt_derived_t *) sctk_malloc( sizeof( _mpc_dt_derived_t ) );

	if ( !new_type )
	{
		sctk_fatal( "Failled to allocate a new derived type" );
	}

	/* And we call the datatype initializer */
	_mpc_dt_derived_init( new_type, id, count, begins, ends, types, lb,
								is_lb, ub, is_ub );

	/* Now we register the datatype pointer in the derived datatype array */
	__mpc_cl_per_mpi_process_ctx_derived_datatype_set( task_specific, id, new_type );

	SCTK_PROFIL_END( MPC_Derived_datatype );
	MPC_ERROR_SUCESS();
}

/** \brief Derived datatype constructor
 *
 * This function allocates a derived datatype slot and fills it
 * with a datatype according to the parameters.
 *
 * \param datatype Datatype to build
 * \param begins list of starting offsets in the datatype
 * \param ends list of end offsets in the datatype
 * \param count number of offsets to store
 * \param lb offset of type lower bound
 * \param is_lb tells if the type has a lowerbound
 * \param ub offset for type upper bound
 * \param is_b tells if the type has an upper bound
 *
 */
int _mpc_cl_derived_datatype( mpc_mp_datatype_t *datatype,
				mpc_pack_absolute_indexes_t *begins,
				mpc_pack_absolute_indexes_t *ends,
				mpc_mp_datatype_t *types, unsigned long count,
				mpc_pack_absolute_indexes_t lb, int is_lb,
				mpc_pack_absolute_indexes_t ub, int is_ub,
				struct _mpc_dt_context *ectx )
{
	SCTK_PROFIL_START( MPC_Derived_datatype );

	/* First the the target datatype to NULL if we do not manage to allocate a new slot */
	*datatype = MPC_DATATYPE_NULL;

	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();


	int i;

	/* Here we check against contiguous as in some case a derived datatype can be stored directly as a contiguous one */
	if ( __mpc_cl_type_contiguous_check( task_specific, ectx, datatype ) )
	{
		MPC_ERROR_SUCESS();
	}

	/* We did not find it lets now look at contiguous */
	if ( __mpc_cl_type_derived_check( task_specific, ectx, datatype ) )
	{
		MPC_ERROR_SUCESS();
	}

	/* Here we jump the first MPC_STRUCT_DATATYPE_COUNT items as they are reserved for common datatypes which are actually derived ones */
	for ( i = MPC_STRUCT_DATATYPE_COUNT; i < SCTK_USER_DATA_TYPES_MAX; i++ )
	{
		/* For each datatype */
		_mpc_dt_derived_t *current_user_type =
			_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific,
									_MPC_DT_MAP_FROM_DERIVED( i ) );

		/* Is the slot free ? */
		if ( current_user_type == NULL )
		{
			/* Here we compute an ID falling in the derived datatype range this range it the converted back to a local id using _MPC_DT_MAP_TO_DERIVED( datatype) */
			int new_id = _MPC_DT_MAP_FROM_DERIVED( i );

			/* Set the new ID in the target datatype */
			*datatype = new_id;

			int ret = _mpc_cl_derived_datatype_on_slot( new_id, begins, ends, types,
								count, lb, is_lb, ub, is_ub );
			_mpc_cl_type_ctx_set( new_id, ectx );

			return ret;
		}
	}

	/* If we are here we did not find any slot so we abort you might think of  increasing SCTK_USER_DATA_TYPES_MAX if you app needs more than 265 datatypes =) */
	sctk_fatal( "Not enough datatypes allowed : you requested too many derived "
				"types (forgot to free ?)" );
	return MPC_ERR_INTERN;
}

/** \brief this function is used to convert a datatype to a derived datatype
 *
 *  \param in_datatype Input datatype
 *  \param out_datatype Output datatype expressed as a derived datatype
 *
 */
int _mpc_cl_type_convert_to_derived( mpc_mp_datatype_t in_datatype,
								  mpc_mp_datatype_t *out_datatype )
{
	/* Is it already a derived datatype ? */
	if ( _mpc_dt_is_derived( in_datatype ) )
	{
		/* Yes nothing to do here just copy it to the new type */
		*out_datatype = in_datatype;
		MPC_ERROR_SUCESS();
	}
	else
	{
		/* Its either a derived or common datatype */

		/* First allocate desriptive arrays for derived datatype */
		mpc_pack_absolute_indexes_t *begins_out =
			sctk_malloc( sizeof( mpc_pack_absolute_indexes_t ) );
		mpc_pack_absolute_indexes_t *ends_out =
			sctk_malloc( sizeof( mpc_pack_absolute_indexes_t ) );
		mpc_mp_datatype_t *datatypes_out = sctk_malloc( sizeof( mpc_mp_datatype_t ) );

		/* We have a single contiguous block here all fo the same type starting at offset 0 */
		begins_out[0] = 0;

		/* Compute the datatype block end with contiguous size */
		size_t type_size;
		_mpc_cl_type_size( in_datatype, &type_size );
		ends_out[0] = type_size - 1;

		/* Retrieve previous datatype if the type is contiguous */
		datatypes_out[0] = in_datatype;

		/* FILL ctx */
		struct _mpc_dt_context dtctx;
		_mpc_dt_context_clear( &dtctx );
		dtctx.combiner = MPC_COMBINER_CONTIGUOUS;
		dtctx.count = 1;
		dtctx.oldtype = in_datatype;

		/* Lets now initialize the new derived datatype */
		_mpc_cl_derived_datatype( out_datatype, begins_out, ends_out, datatypes_out, 1,
							   0, 0, 0, 0, &dtctx );

		/* Free temporary buffers */
		sctk_free( begins_out );
		sctk_free( ends_out );
		sctk_free( datatypes_out );
	}

	MPC_ERROR_SUCESS();
}

/** \brief Pack a derived datatype in a contiguous buffer
 *  \param inbuffer The input buffer
 *  \param outbuffer The output buffer (has to be allocated to size !)
 *  \param datatype Datatype to be copied
 */
int PMPC_Copy_in_buffer( void *inbuffer, void *outbuffer,
						 mpc_mp_datatype_t datatype )
{
	/* Only meaningful if its a derived datatype */
	if ( _mpc_dt_is_derived( datatype ) )
	{
		unsigned int j;
		char *tmp;

		/* Retrieve task ctx */
		mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

		tmp = (char *) outbuffer;

		/* Get a pointer to the target type */
		_mpc_dt_derived_t *t =
			_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );

		if ( !t )
		{
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG,
							  "Failed to retrieve this derived datatype" );
		}

		/* Copy ach block one by one in the pack */
		for ( j = 0; j < t->opt_count; j++ )
		{
			size_t size;
			/* Sizeof block */
			size = t->opt_ends[j] - t->opt_begins[j] + 1;
			memcpy( tmp, ( (char *) inbuffer ) + t->begins[j], size );
			/* Increment offset in packed block */
			tmp += size;
		}
	}
	else
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN, "" );
	}
	MPC_ERROR_SUCESS();
}

/** \brief Unpack a derived datatype from contiguous buffer
 *  \param inbuffer The input buffer
 *  \param outbuffer The output buffer (has to be allocated to size !)
 *  \param datatype Datatype to be unpacked
 */
int PMPC_Copy_from_buffer( void *inbuffer, void *outbuffer,
						   mpc_mp_datatype_t datatype )
{
	/* Only meaningful if its a derived datatype */
	if ( _mpc_dt_is_derived( datatype ) )
	{
		unsigned int j;
		char *tmp;

		/* Retrieve task ctx */
		mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

		tmp = (char *) inbuffer;

		/* Get a pointer to the target type */
		_mpc_dt_derived_t *t =
			_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );

		if ( !t )
		{
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG,
							  "Failed to retrieve this derived datatype" );
		}

		/* Unpack each block at the correct offset */
		for ( j = 0; j < t->opt_count; j++ )
		{
			size_t size;
			/* Sizeof block */
			size = t->opt_ends[j] - t->opt_begins[j] + 1;
			/* Copy at offset */
			memcpy( ( (char *) outbuffer ) + t->begins[j], tmp, size );
			/* Move in the contiguous block */
			tmp += size;
		}
	}
	else
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN, "" );
	}

	MPC_ERROR_SUCESS();
}

/** \brief Extract derived datatype informations
 *  On non derived datatypes res is set to 0
 *  and datatype contains garbage
 *
 *  \param datatype Datatype to analyze
 *  \param res set to 0 if not a derived datatype, 1 if derived
 *  \param output_datatype A derived datatype filled with all the informations
 *
 */
int _mpc_cl_derived_datatype_try_get_info( mpc_mp_datatype_t datatype, int *res,
					  _mpc_dt_derived_t *output_datatype )
{
	/* Initialize output argument  assuming it is not a derived datatype */
	*res = 0;
	memset( output_datatype, 0, sizeof( _mpc_dt_derived_t ) );
	output_datatype->count = 1;

	/* Check whether the datatype ID falls in the derived range ID */
	if ( _mpc_dt_is_derived( datatype ) )
	{
		/* Retrieve task specific context */
		mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
		task_specific = __mpc_cl_per_mpi_process_ctx_get();

		sctk_nodebug( "datatype(%d) - SCTK_COMMON_DATA_TYPE_COUNT(%d) - "
					  "SCTK_USER_DATA_TYPES_MAX(%d) ==  %d-%d-%d == %d",
					  datatype, SCTK_COMMON_DATA_TYPE_COUNT,
					  SCTK_USER_DATA_TYPES_MAX, datatype,
					  SCTK_COMMON_DATA_TYPE_COUNT, SCTK_USER_DATA_TYPES_MAX,
					  _MPC_DT_MAP_TO_DERIVED( datatype ) );

		/* Retrieve the datatype from the array */
		_mpc_dt_derived_t *target_type =
			_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );

		if ( !target_type )
		{
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG,
							  "Failed to retrieve this derived datatype" );
		}

		/* We have found a derived datatype */
		*res = 1;

		/* Copy its content to out arguments */
		memcpy( output_datatype, target_type, sizeof( _mpc_dt_derived_t ) );
	}

	MPC_ERROR_SUCESS();
}

int _mpc_cl_type_ctx_set( mpc_mp_datatype_t datatype,
				struct _mpc_dt_context *dctx )
{

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	_mpc_dt_contiguout_t *target_contiguous_type;
	_mpc_dt_derived_t *target_derived_type;

	switch ( _mpc_dt_get_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			/* Nothing to do here */
			MPC_ERROR_SUCESS();
			break;

		case MPC_DATATYPES_CONTIGUOUS:
			/* Get a pointer to the type of interest */
			target_contiguous_type =
				_mpc_cl_per_mpi_process_ctx_contiguous_datatype_ts_get( task_specific, datatype );
			/* Save the context */
			_mpc_dt_context_set( &target_contiguous_type->context, dctx );
			break;

		case MPC_DATATYPES_DERIVED:
			/* Get a pointer to the type of interest */
			target_derived_type =
				_mpc_cl_per_mpi_process_ctx_derived_datatype_ts_get( task_specific, datatype );
			/* Save the context */
			_mpc_dt_context_set( &target_derived_type->context, dctx );
			break;

		case MPC_DATATYPES_UNKNOWN:
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN,
							  "This datatype is unknown" );
			break;
	}

	MPC_ERROR_SUCESS();
}

/* Datatype  Attribute Handling                                         */

/* KEYVALS */

int _mpc_cl_type_create_keyval( MPC_Type_copy_attr_function *copy,
				MPC_Type_delete_attr_function *delete,
				int *type_keyval, void *extra_state )
{
	return _mpc_dt_keyval_create( copy, delete, type_keyval, extra_state );
}

int _mpc_cl_type_free_keyval( int *type_keyval )
{
	return _mpc_dt_keyval_free( type_keyval );
}

/* ATTRS */

int _mpc_cl_type_get_attr( mpc_mp_datatype_t datatype, int type_keyval,
						  void *attribute_val, int *flag )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	struct _mpc_dt_storage *da = task_specific->datatype_array;

	if ( !da )
	{
		return MPC_ERR_INTERN;
	}

	return sctk_type_get_attr( da, datatype, type_keyval, attribute_val, flag );
}

int _mpc_cl_type_set_attr( mpc_mp_datatype_t datatype, int type_keyval,
						  void *attribute_val )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	struct _mpc_dt_storage *da = task_specific->datatype_array;

	if ( !da )
	{
		return MPC_ERR_INTERN;
	}

	return sctk_type_set_attr( da, datatype, type_keyval, attribute_val );
}

int _mpc_cl_type_delete_attr( mpc_mp_datatype_t datatype, int type_keyval )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	struct _mpc_dt_storage *da = task_specific->datatype_array;

	if ( !da )
	{
		return MPC_ERR_INTERN;
	}

	return sctk_type_delete_attr( da, datatype, type_keyval );
}

/** \brief This struct is used to serialize these boundaries all
 * at once instead of having to do it one by one (see two next functions) */
struct inner_lbub
{
	mpc_pack_absolute_indexes_t lb; /**< Lower bound offset  */
	int is_lb;						/**< Does type has a lower bound */
	mpc_pack_absolute_indexes_t ub; /**< Upper bound offset */
	int is_ub;						/**< Does type has an upper bound */
	int is_a_padded_struct;			/**< Was the type padded with UB during construction ?
                             */
};

void *_mpc_cl_derived_type_serialize( mpc_mp_datatype_t type, size_t *size,
									  size_t header_pad )
{
	assume( _mpc_dt_is_derived( type ) );
	assume( size );

	_mpc_dt_derived_t *dtype = _mpc_cl_per_mpi_process_ctx_derived_datatype_get( type );

	assume( dtype != NULL );

	*size = sizeof( size_t ) +
			2 * dtype->count * sizeof( mpc_pack_absolute_indexes_t ) +
			dtype->count * sizeof( mpc_mp_datatype_t ) + sizeof( struct inner_lbub ) +
			header_pad + 1;

	void *ret = sctk_malloc( *size );

	if ( !ret )
	{
		perror( "malloc" );
		return NULL;
	}

	ssize_t *count = (ssize_t *) ( ret + header_pad );
	mpc_pack_absolute_indexes_t *begins = ( count + 1 );
	mpc_pack_absolute_indexes_t *ends = begins + dtype->count;
	mpc_mp_datatype_t *types = (mpc_mp_datatype_t *) ( ends + dtype->count );
	struct inner_lbub *lb_ub = (struct inner_lbub *) ( types + dtype->count );
	char *guard = ( (char *) lb_ub + 1 );

	*guard = 77;

	*count = dtype->count;

	size_t i;

	for ( i = 0; i < dtype->count; i++ )
	{
		begins[i] = dtype->begins[i];
		ends[i] = dtype->ends[i];
		types[i] = dtype->datatypes[i];
	}

	lb_ub->lb = dtype->lb;
	lb_ub->is_lb = dtype->is_lb;
	lb_ub->ub = dtype->ub;
	lb_ub->is_ub = dtype->is_ub;
	lb_ub->is_a_padded_struct = dtype->is_a_padded_struct;

	assert( *guard == 77 );
	assert( guard < (char *) ( ret + *size ) );

	return ret;
}

mpc_mp_datatype_t _mpc_cl_derived_type_deserialize( void *buff, size_t size,
													size_t header_pad )
{
	ssize_t *count = (ssize_t *) ( buff + header_pad );
	mpc_pack_absolute_indexes_t *begins = ( count + 1 );
	mpc_pack_absolute_indexes_t *ends = begins + *count;
	mpc_mp_datatype_t *types = (mpc_mp_datatype_t *) ( ends + *count );
	struct inner_lbub *lb_ub = (struct inner_lbub *) ( types + *count );

	assume( count < (ssize_t *) ( buff + size ) );
	assume( count < (ssize_t *) ( begins + size ) );
	assume( count < (ssize_t *) ( ends + size ) );
	assume( count < (ssize_t *) ( types + size ) );

	mpc_mp_datatype_t ret;

	struct _mpc_dt_context dtctx;
	_mpc_dt_context_clear( &dtctx );
	dtctx.combiner = MPC_COMBINER_DUMMY;

	_mpc_cl_derived_datatype( &ret, begins, ends, types, *count, lb_ub->lb,
							  lb_ub->is_lb, lb_ub->ub, lb_ub->is_ub, &dtctx );

	if ( lb_ub->is_a_padded_struct )
	{
		_mpc_cl_type_flag_padded( ret );
	}

	_mpc_cl_type_commit( &ret );

	return ret;
}

mpc_mp_datatype_t _mpc_cl_type_get_inner( mpc_mp_datatype_t type )
{
	assume( !_mpc_dt_is_common( type ) );

	if ( _mpc_dt_is_struct( type ) )
		return type;

	if ( _mpc_dt_is_contiguous( type ) )
	{
		_mpc_dt_contiguout_t *ctype = _mpc_cl_per_mpi_process_ctx_contiguous_datatype_get( type );
		assume( ctype != NULL );
		return ctype->datatype;
	}

	_mpc_dt_derived_t *dtype = _mpc_cl_per_mpi_process_ctx_derived_datatype_get( type );

	assume( dtype != NULL );

	// _mpc_dt_derived_display( dtype );

	mpc_mp_datatype_t itype = -1;

	size_t i;

	for ( i = 0; i < dtype->count; i++ )
	{
		if ( itype < 0 )
		{
			itype = dtype->datatypes[i];
		}
		else
		{
			if ( itype != dtype->datatypes[i] )
				return -1;
		}
	}

	assume( _mpc_dt_is_common( itype ) );

	/* if we are here, all types are the same */
	return itype;
}

/************************************************************************/
/* MPC Init and Finalize                                                */
/************************************************************************/

int _mpc_cl_init( __UNUSED__ int *argc, __UNUSED__ char ***argv )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	SCTK_PROFIL_START( MPC_Init );

	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* If the task calls MPI_Init() a second time */
	if ( task_specific->init_done == 2 )
	{
		return MPC_ERR_OTHER;
	}

	task_specific->init_done = 1;
	task_specific->thread_level = MPC_THREAD_MULTIPLE;

	SCTK_PROFIL_END( MPC_Init );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_init_thread( int *argc, char ***argv, int required, int *provided )
{
	const int res = _mpc_cl_init( argc, argv );

	if ( res == SCTK_SUCCESS )
	{
		mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
		task_specific->thread_level = required;
		*provided = required;
	}

	return res;
}

int _mpc_cl_initialized( int *flag )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	SCTK_PROFIL_START( MPC_Initialized );
	task_specific = __mpc_cl_per_mpi_process_ctx_get();
	*flag = task_specific->init_done;
	SCTK_PROFIL_END( MPC_Initialized );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_barrier( mpc_mp_communicator_t comm );

int _mpc_cl_finalize( void )
{
	SCTK_PROFIL_START( MPC_Finalize );

	_mpc_cl_barrier( SCTK_COMM_WORLD );

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	task_specific->init_done = 2;

	fflush( stderr );
	fflush( stdout );
	SCTK_PROFIL_END( MPC_Finalize );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_query_thread( int *provided )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	if ( task_specific->thread_level == -1 )
	{
		return MPC_ERR_OTHER;
	}

	*provided = task_specific->thread_level;
	MPC_ERROR_SUCESS();
}

int _mpc_cl_get_activity( int nb_item, MPC_Activity_t *tab, double *process_act )
{
	int i;
	int nb_proc;
	double proc_act = 0;
	nb_proc = mpc_common_get_pu_count();
	sctk_nodebug( "Activity %d/%d:", nb_item, nb_proc );
	for ( i = 0; ( i < nb_item ) && ( i < nb_proc ); i++ )
	{
		double tmp;
		tab[i].virtual_cpuid = i;
		tmp = sctk_thread_get_activity( i );
		tab[i].usage = tmp;
		proc_act += tmp;
		sctk_nodebug( "\t- cpu %d activity %f\n", tab[i].virtual_cpuid,
					  tab[i].usage );
	}
	for ( ; i < nb_item; i++ )
	{
		tab[i].virtual_cpuid = -1;
		tab[i].usage = -1;
	}
	proc_act = proc_act / ( (double) nb_proc );
	*process_act = proc_act;
	MPC_ERROR_SUCESS();
}

/****************************
 * MPC MESSAGE PASSING MAIN *
 ****************************/

int _mpc_cl_bcast( void *buffer, mpc_mp_msg_count_t count,
		  mpc_mp_datatype_t datatype, int root, mpc_mp_communicator_t comm);

/** \brief Function used to create a temporary run directory
 */
static inline void __mpc_cl_enter_tmp_directory()
{
	char *do_move_to_temp = getenv( "MPC_MOVE_TO_TEMP" );

	if ( do_move_to_temp == NULL )
	{
		/* Nothing to do */
		return;
	}

	/* Retrieve task rank */
	int rank;
	_mpc_cl_comm_rank( SCTK_COMM_WORLD, &rank );

	char tmpdir[1000];

	/* If root rank create the temp dir */
	if ( rank == 0 )
	{
		char currentdir[800];
		// First enter a sandbox DIR
		getcwd( currentdir, 800 );
		snprintf( tmpdir, 1000, "%s/XXXXXX", currentdir );
		mkdtemp( tmpdir );
		sctk_warning( "Creating temp directory %s", tmpdir );
	}

	/* Broadcast the path to all tasks */
	_mpc_cl_bcast( (void *) tmpdir, 1000, MPC_CHAR, 0, SCTK_COMM_WORLD );

	/* Only the root of each process does the chdir */
	if ( mpc_common_get_local_task_rank() == 0 )
	{
		chdir( tmpdir );
	}
}

#ifdef HAVE_ENVIRON_VAR
#include <stdlib.h>
#include <stdio.h>
extern char **environ;
#endif

/* main (int argc, char **argv) */
int mpc_mpi_m_mpi_process_main( int argc, char **argv )
{
	int result;

	__mpc_cl_request_init_null();

	sctk_size_checking_eq( SCTK_COMM_WORLD, SCTK_COMM_WORLD, "SCTK_COMM_WORLD",  "SCTK_COMM_WORLD", __FILE__, __LINE__ );
	sctk_size_checking_eq( SCTK_COMM_SELF, SCTK_COMM_SELF, "SCTK_COMM_SELF",  "SCTK_COMM_SELF", __FILE__, __LINE__ );

	sctk_check_equal_types( mpc_mp_datatype_t, mpc_mp_datatype_t );
	sctk_check_equal_types( mpc_mp_communicator_t, mpc_mp_communicator_t );
	sctk_check_equal_types( sctk_pack_indexes_t, mpc_pack_indexes_t );
	sctk_check_equal_types( sctk_pack_absolute_indexes_t, mpc_pack_absolute_indexes_t );
	sctk_check_equal_types( sctk_count_t, mpc_mp_msg_count_t );
	sctk_check_equal_types( sctk_thread_key_t, mpc_thread_key_t );

	__mpc_cl_per_mpi_process_ctx_init();

	if ( sctk_runtime_config_get()->modules.mpc.disable_message_buffering )
	{
		__mpc_cl_disable_buffering();
	}

	__mpc_cl_enter_tmp_directory();

#ifdef HAVE_ENVIRON_VAR
	result = mpc_user_main( argc, argv, environ );
#else
	result = mpc_user_main( argc, argv );
#endif

#ifdef MPC_Profiler
	sctk_internal_profiler_render();
#endif

	mpc_mpi_m_per_thread_ctx_release();

	mpc_mp_barrier( (mpc_mp_communicator_t) SCTK_COMM_WORLD );

	__mpc_cl_per_mpi_process_ctx_release();

	return result;
}

/************************************************************************/
/* Topology Informations                                                */
/************************************************************************/

int _mpc_cl_comm_rank( mpc_mp_communicator_t comm, int *rank )
{
	SCTK_PROFIL_START( MPC_Comm_rank );
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	
	task_specific = __mpc_cl_per_mpi_process_ctx_get();
	*rank = mpc_mp_communicator_rank( comm, task_specific->task_id );
	SCTK_PROFIL_END( MPC_Comm_rank );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_comm_size( mpc_mp_communicator_t comm, int *size )
{
	SCTK_PROFIL_START( MPC_Comm_size );
	
	*size = mpc_mp_communicator_size( comm );
	SCTK_PROFIL_END( MPC_Comm_size );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_comm_remote_size( mpc_mp_communicator_t comm, int *size )
{
	// SCTK_PROFIL_START (MPC_Comm_remote_size);
	
	*size = mpc_mp_communicator_remote_size( comm );
	// SCTK_PROFIL_END (MPC_Comm_remote_size);
	MPC_ERROR_SUCESS();
}

int _mpc_cl_get_processor_name( char *name, int *resultlen )
{
	SCTK_PROFIL_START( MPC_Get_processor_name );
	gethostname( name, 128 );
	*resultlen = strlen( name );
	SCTK_PROFIL_END( MPC_Get_processor_name );
	MPC_ERROR_SUCESS();
}

/************************************************************************/
/* Point to point communications                                        */
/************************************************************************/

static void sctk_no_free_header(__UNUSED__ void *tmp) {}

int _mpc_cl_isend( void *buf, mpc_mp_msg_count_t count,
		mpc_mp_datatype_t datatype, int dest, int tag,
		mpc_mp_communicator_t comm, mpc_mp_request_t *request )
{
	int com_size = mpc_mp_communicator_size( comm );
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	int src = mpc_mp_communicator_rank( comm, task_specific->task_id );

	if ( dest == SCTK_PROC_NULL )
	{
		mpc_mp_comm_request_init( request, comm, REQUEST_SEND );
		MPC_ERROR_SUCESS();
	}

	size_t d_size = __mpc_cl_datatype_get_size( datatype, task_specific );
	size_t msg_size = count * d_size;

	mpc_mp_ptp_message_t *msg = NULL;

	/* Here we see if we can get a slot to buffer small message content
	   otherwise we simply get a regular message header */

	if ( ( msg_size > MAX_MPC_BUFFERED_SIZE ) || !__mpc_cl_buffering_enabled )
	{
	FALLBACK_TO_UNBUFERED_ISEND:
		msg = mpc_mp_comm_ptp_message_header_create( SCTK_MESSAGE_CONTIGUOUS );
		mpc_mp_comm_ptp_message_header_init( msg, tag, comm, src, dest, request, msg_size,
						     SCTK_P2P_MESSAGE, datatype, REQUEST_SEND );
		mpc_mp_comm_ptp_message_set_contiguous_addr( msg, buf, msg_size );
	}
	else
	{
		mpc_buffered_msg_t *tmp_buf = __mpc_cl_thread_buffer_pool_acquire_async();

		if ( tmp_buf->completion_flag == SCTK_MESSAGE_DONE )
		{
			/* We set the buffer as busy */
			tmp_buf->completion_flag = SCTK_MESSAGE_PENDING;

			/* Use the header from the slot */
			msg = &tmp_buf->header;

			/* We move asynchronous buffer pool head ahead */
			__mpc_cl_thread_buffer_pool_step_async();

			/* Initialize the header */
			mpc_mp_comm_ptp_message_header_clear( msg, SCTK_MESSAGE_CONTIGUOUS, sctk_no_free_header, mpc_mp_comm_ptp_message_copy );

			/* Copy message content in the buffer */
			memcpy( tmp_buf->buf, buf, msg_size );

			mpc_mp_comm_ptp_message_header_init( msg, tag, comm, src, dest, request,
							     msg_size, SCTK_P2P_MESSAGE, datatype, REQUEST_SEND );

			mpc_mp_comm_ptp_message_set_contiguous_addr( msg, tmp_buf->buf, msg_size );

			/* Register the async buffer to release the wait immediately */
			msg->tail.buffer_async = tmp_buf;
		}
		else
		{
			/* Here we are starving: the oldest async buffer is still in use therefore no buffering */
			goto FALLBACK_TO_UNBUFERED_ISEND;
		}
	}

	mpc_mp_comm_ptp_message_send( msg );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_issend( void *buf, mpc_mp_msg_count_t count,
		   mpc_mp_datatype_t datatype, int dest, int tag,
	 	   mpc_mp_communicator_t comm, mpc_mp_request_t *request )
{
	/* Issend is not buffered */

	if ( dest == SCTK_PROC_NULL )
	{
		mpc_mp_comm_request_init( request, comm, REQUEST_SEND );
		MPC_ERROR_SUCESS();
	}

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	int myself = mpc_mp_communicator_rank( comm, task_specific->task_id );

	mpc_mp_ptp_message_t *msg = mpc_mp_comm_ptp_message_header_create( SCTK_MESSAGE_CONTIGUOUS );
	size_t d_size = __mpc_cl_datatype_get_size( datatype, task_specific );
	size_t msg_size = count * d_size;

	mpc_mp_comm_ptp_message_header_init( msg, tag, comm, myself, dest, request, msg_size,
					     SCTK_P2P_MESSAGE, datatype, REQUEST_SEND );
	mpc_mp_comm_ptp_message_set_contiguous_addr( msg, buf, msg_size );

	mpc_mp_comm_ptp_message_send( msg );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_ibsend(void *buf, mpc_mp_msg_count_t count, mpc_mp_datatype_t datatype, int dest,
                int tag, mpc_mp_communicator_t comm, mpc_mp_request_t *request)
{
  return _mpc_cl_isend(buf, count, datatype, dest, tag, comm, request);
}


int _mpc_cl_irsend(void *buf, mpc_mp_msg_count_t count, mpc_mp_datatype_t datatype, int dest,
                int tag, mpc_mp_communicator_t comm, mpc_mp_request_t *request) {
  return _mpc_cl_isend(buf, count, datatype, dest, tag, comm, request);
}

int _mpc_cl_irecv( void *buf, mpc_mp_msg_count_t count,
		  mpc_mp_datatype_t datatype, int source, int tag,
		  mpc_mp_communicator_t comm, mpc_mp_request_t *request )
{

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	if ( source == SCTK_PROC_NULL )
	{
		mpc_mp_comm_request_init( request, comm, REQUEST_RECV );
		MPC_ERROR_SUCESS();
	}


	int myself = mpc_mp_communicator_rank( comm, task_specific->task_id );


	mpc_mp_ptp_message_t * msg = mpc_mp_comm_ptp_message_header_create( SCTK_MESSAGE_CONTIGUOUS );

	size_t d_size = __mpc_cl_datatype_get_size( datatype, task_specific );

	mpc_mp_comm_ptp_message_header_init( msg, tag, comm, source, myself, request,
					    count * d_size, SCTK_P2P_MESSAGE, datatype, REQUEST_RECV );
	mpc_mp_comm_ptp_message_set_contiguous_addr( msg, buf, count * d_size );
	mpc_mp_comm_ptp_message_recv(msg);

	MPC_ERROR_SUCESS();
}

int _mpc_cl_ssend( void *buf, mpc_mp_msg_count_t count, mpc_mp_datatype_t datatype,
		  int dest, int tag, mpc_mp_communicator_t comm )
{
	mpc_mp_request_t request;

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	if ( dest == SCTK_PROC_NULL )
	{
		MPC_ERROR_SUCESS();
	}

	size_t msg_size = count * __mpc_cl_datatype_get_size( datatype, task_specific );

	mpc_mp_comm_isend( dest, buf, msg_size, tag, comm, &request );
	mpc_mp_comm_request_wait( &request );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_send( void *restrict buf, mpc_mp_msg_count_t count,
				 mpc_mp_datatype_t datatype, int dest, int tag, mpc_mp_communicator_t comm )
{
	mpc_mp_request_t request;

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	int src = mpc_mp_communicator_rank( comm, task_specific->task_id );


	if ( dest == SCTK_PROC_NULL )
	{
		MPC_ERROR_SUCESS();
	}

	size_t msg_size = count * __mpc_cl_datatype_get_size( datatype, task_specific );

	mpc_mp_ptp_message_t *msg = NULL;

	mpc_mp_ptp_message_t header;

	if ( ( msg_size >= MAX_MPC_BUFFERED_SIZE ) || !__mpc_cl_buffering_enabled )
	{
	FALLBACK_TO_BLOCKING_SEND:

		msg = &header;
		mpc_mp_comm_isend( dest, buf, msg_size, tag, comm, &request);
		mpc_mp_comm_request_wait( &request );
	}
	else
	{
		mpc_mpi_m_per_thread_ctx_t *thread_spec = __mpc_cl_per_thread_ctx_get();
		mpc_buffered_msg_t *tmp_buf = __mpc_cl_thread_buffer_pool_acquire_sync( thread_spec );

		if ( mpc_mp_comm_request_get_completion( &( tmp_buf->request ) ) != SCTK_MESSAGE_DONE )
		{
			goto FALLBACK_TO_BLOCKING_SEND;
		}
		else
		{
			/* Move the buffer head */
			__mpc_cl_thread_buffer_pool_step_sync(thread_spec);
			msg = &( tmp_buf->header );

			msg->tail.message_type = SCTK_MESSAGE_CONTIGUOUS;

			/* Init the header */
			mpc_mp_comm_ptp_message_header_clear( msg, SCTK_MESSAGE_CONTIGUOUS, sctk_no_free_header,mpc_mp_comm_ptp_message_copy );

			mpc_mp_comm_ptp_message_header_init( msg, tag, comm, src, dest,
							     &( tmp_buf->request ), msg_size,
							     SCTK_P2P_MESSAGE, datatype, REQUEST_SEND );
			mpc_mp_comm_ptp_message_set_contiguous_addr( msg, tmp_buf->buf, msg_size );

			/* Copy message in buffer */
			memcpy( tmp_buf->buf, buf, msg_size );

			/* Register async buffer */
			msg->tail.buffer_async = tmp_buf;

			/* Send but do not wait */
			mpc_mp_comm_ptp_message_send( msg );
		}
	}

	MPC_ERROR_SUCESS();
}

int _mpc_cl_recv( void *buf, mpc_mp_msg_count_t count, mpc_mp_datatype_t datatype, int source,
		 int tag, mpc_mp_communicator_t comm, mpc_mp_status_t *status )
{
	mpc_mp_request_t request;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	if ( source == SCTK_PROC_NULL )
	{
		if ( status != SCTK_STATUS_NULL )
		{
			status->MPC_SOURCE = SCTK_PROC_NULL;
			status->MPC_TAG = SCTK_ANY_TAG;
			status->MPC_ERROR = SCTK_SUCCESS;
			status->size = 0;
		}
		MPC_ERROR_SUCESS();
	}

	size_t msg_size = count * __mpc_cl_datatype_get_size( datatype, task_specific );

	mpc_mp_comm_irecv( source, buf, msg_size, tag, comm, &request );
	mpc_mp_comm_request_wait( &request );

	if ( request.status_error != SCTK_SUCCESS )
	{
		if ( status != NULL )
		{
			status->MPC_ERROR = request.status_error;
		}

		return request.status_error;
	}

	__mpc_cl_request_commit_status( &request, status );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_sendrecv( void *sendbuf, mpc_mp_msg_count_t sendcount, mpc_mp_datatype_t sendtype,
				   int dest, int sendtag, void *recvbuf, mpc_mp_msg_count_t recvcount,
				   mpc_mp_datatype_t recvtype, int source, int recvtag, mpc_mp_communicator_t comm,
				   mpc_mp_status_t *status )
{
	mpc_mp_request_t sendreq;
	mpc_mp_request_t recvreq;

	sendreq = MPC_REQUEST_NULL;
	recvreq = MPC_REQUEST_NULL;

	_mpc_cl_irecv( recvbuf, recvcount, recvtype, source, recvtag, comm, &recvreq );
	_mpc_cl_isend( sendbuf, sendcount, sendtype, dest, sendtag, comm, &sendreq );

	_mpc_cl_wait( &sendreq, status );
	_mpc_cl_wait( &recvreq, status );

	MPC_ERROR_SUCESS();
}



/****************
 * MESSAGE WAIT *
 ****************/

int _mpc_cl_wait( mpc_mp_request_t *request, mpc_mp_status_t *status )
{
	if ( mpc_mp_comm_request_get_source( request ) == SCTK_PROC_NULL )
	{
		mpc_mp_comm_request_set_null( request, 1 );
	}

	if ( !mpc_mp_comm_request_is_null( request ) )
	{
		mpc_mp_comm_request_wait( request );
		mpc_mp_comm_request_set_null( request, 1 );
	}

	__mpc_cl_request_commit_status( request, status );
	MPC_ERROR_SUCESS();
}


static inline int __mpc_cl_test_light( mpc_mp_request_t *request )
{
	if ( mpc_mp_comm_request_get_completion( request ) != SCTK_MESSAGE_PENDING )
	{
		return 1;
	}

	__mpc_cl_request_progress( request );

	return mpc_mp_comm_request_get_completion( request );
}

static inline int __mpc_cl_test_no_progress( mpc_mp_request_t *request, int *flag,
					   mpc_mp_status_t *status )
{
	*flag = 0;
	if ( mpc_mp_comm_request_get_completion( request ) != SCTK_MESSAGE_PENDING )
	{
		*flag = 1;
		__mpc_cl_request_commit_status( request, status );
	}
	else
	{
		if ( ( status != SCTK_STATUS_NULL ) && ( *flag == 0 ) )
		{
			status->MPC_ERROR = MPC_ERR_PENDING;
		}
	}

	MPC_ERROR_SUCESS();
}


int _mpc_cl_test( mpc_mp_request_t *request, int *flag,
	         mpc_mp_status_t *status )
{
	*flag = 0;

	if ( mpc_mp_comm_request_is_null( request ) )
	{
		*flag = 1;
		__mpc_cl_request_commit_status( request, status );
		MPC_ERROR_SUCESS();
	}

	if ( mpc_mp_comm_request_get_completion( request ) == SCTK_MESSAGE_PENDING )
	{
		__mpc_cl_request_progress( request );
	}

	__mpc_cl_test_no_progress(request, flag, status);

	MPC_ERROR_SUCESS();
}

struct wfv_waitall_s
{
	int ret;
	mpc_mp_msg_count_t count;
	mpc_mp_request_t **array_of_requests;
	mpc_mp_status_t *array_of_statuses;
};

static inline void wfv_waitall( void *arg )
{
	struct wfv_waitall_s *args = (struct wfv_waitall_s *) arg;
	int flag = 1;
	int i;

	for ( i = 0; i < args->count; i++ )
	{
		flag = flag & __mpc_cl_test_light( args->array_of_requests[i] );
	}

	/* All requests received */
	if ( flag )
	{
		for ( i = 0; i < args->count; i++ )
		{
			mpc_mp_request_t *request;

			request = args->array_of_requests[i];

			if ( args->array_of_statuses != NULL )
			{
				mpc_mp_status_t *status = &( args->array_of_statuses[i] );
				__mpc_cl_request_commit_status( request, status );
				sctk_nodebug( "source %d\n", status->MPC_SOURCE );
				mpc_mp_comm_request_set_null( request, 1 );
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
static inline int _mpc_cl_waitall_Grequest( mpc_mp_msg_count_t count,
					   mpc_mp_request_t *parray_of_requests[],
					   mpc_mp_status_t array_of_statuses[] )
{
	int i;
	int all_of_the_same_class = 0;
	MPCX_Grequest_wait_fn *ref_wait = NULL;

	/* Do we have at least two requests ? */
	if ( 2 < count )
	{
		/* Are we looking at generalized requests ? */
		if ( parray_of_requests[0]->request_type == REQUEST_GENERALIZED )
		{
			ref_wait = parray_of_requests[0]->wait_fn;

			/* Are we playing with extended Grequest classes ? */
			if ( ref_wait )
			{
				/* Does the first two match ? */
				if ( ref_wait == parray_of_requests[1]->wait_fn )
				{
					/* Consider they all match now check the rest */
					all_of_the_same_class = 1;

					for ( i = 0; i < count; i++ )
					{
						/* Can we find a different one ? */
						if ( parray_of_requests[i]->wait_fn != ref_wait )
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
	if ( all_of_the_same_class )
	{
		mpc_mp_status_t tmp_status;
		/* Prepare the state array */
		void **array_of_states = sctk_malloc( sizeof( void * ) * count );
		assume( array_of_states != NULL );
		for ( i = 0; i < count; i++ )
			array_of_states[i] = parray_of_requests[i]->extra_state;

		/* Call the batch wait function */
		/* Here the timeout parameter is not obvious as for example
	           ROMIO relies on Wtime which is not constrained by the norm
                   is is just monotonous. Whe should have a scaling function
                   which depends on the time source to be fair */
		( ref_wait )( count, array_of_states, 1e9, &tmp_status );

		sctk_free( array_of_states );

		/* Update the statuses */
		if ( array_of_statuses != SCTK_STATUS_NULL )
		{
			/* here we do a for loop as we only checked that the wait function
			 was identical we are not sure that the XGrequests classes werent
			different ones */
			for ( i = 0; i < count; i++ )
			{
				( parray_of_requests[i]->query_fn )( parray_of_requests[i]->extra_state,
													 &array_of_statuses[i] );
			}
		}

		/* Free the requests */
		for ( i = 0; i < count; i++ )
			( parray_of_requests[i]->free_fn )( parray_of_requests[i]->extra_state );

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
 *  \warning This function is not MPI_Waitall (see the mpc_mp_request_t *
 * parray_of_requests[] argument)
 *
 */

int _mpc_cl_waitallp( mpc_mp_msg_count_t count, mpc_mp_request_t *parray_of_requests[],
					 mpc_mp_status_t array_of_statuses[] )
{
	int i;

	sctk_nodebug( "waitall count %d\n", count );

	/* We have to detect generalized requests as we are not able
	to progress them in the pooling function as we have no
	MPI context therefore we force a non-pooled progress
	when we have generalized requests*/
	int contains_generalized = 0;

	for ( i = 0; i < count; i++ )
	{
		if ( parray_of_requests[i]->request_type == REQUEST_GENERALIZED )
		{
			contains_generalized |= 1;
			break;
		}
	}

	if( contains_generalized )
	{
		/* Can we do a batch wait ? */
		if(contains_generalized)
		{
			if ( _mpc_cl_waitall_Grequest( count, parray_of_requests, array_of_statuses ) )
			{
				MPC_ERROR_SUCESS()
			}
		}

		int flag = 0;

		/* Here we force the local polling because of generalized requests
			This will happen only if classical and generalized requests are
			mixed or if the wait_fn is not the same for all requests */
		while ( flag == 0 )
		{
			flag = 1;

			for ( i = 0; i < count; i++ )
			{
				int tmp_flag = 0;

				mpc_mp_status_t *status;
				mpc_mp_request_t *request;

				if ( array_of_statuses != NULL )
				{
					status = &( array_of_statuses[i % count] );
				}
				else
				{
					status = SCTK_STATUS_NULL;
				}

				request = parray_of_requests[i % count];

				if ( mpc_mp_comm_request_is_null(request) )
					continue;

				_mpc_cl_test( request, &tmp_flag, status );

				/* We set this flag in order to prevent the status
				   from being updated repetitivelly in __mpc_cl_test */
				if ( tmp_flag )
				{
					mpc_mp_comm_request_set_null( request, 1 );
				}

				flag = flag & tmp_flag;
			}

			if ( flag == 1 )
				MPC_ERROR_SUCESS();

			sctk_thread_yield();
		}
	}

	/* XXX: Waitall has been ported for using wait_for_value_and_poll
  * because it provides better results than thread_yield.
  * It performs well at least on Infiniband on NAS  */
	struct wfv_waitall_s wfv;
	wfv.ret = 0;
	wfv.array_of_requests = parray_of_requests;
	wfv.array_of_statuses = array_of_statuses;
	wfv.count = count;
	mpc_mp_comm_perform_idle( (int *) &( wfv.ret ), 1,
				  (void ( * )( void * )) wfv_waitall, (void *) &wfv );

	MPC_ERROR_SUCESS();
}

#define PMPC_WAIT_ALL_STATIC_TRSH 32

int _mpc_cl_waitall( mpc_mp_msg_count_t count, mpc_mp_request_t array_of_requests[],
					mpc_mp_status_t array_of_statuses[] )
{
	int res;
	int i;

	SCTK_PROFIL_START( MPC_Waitall );

	/* Here we are preparing the array of pointer to request
	   in order to call the _mpc_cl_waitallp function
	   it might be an extra cost but it allows the use of
	   the _mpc_cl_waitallp function from MPI_Waitall */
	mpc_mp_request_t **parray_of_requests;
	mpc_mp_request_t *static_parray_of_requests[PMPC_WAIT_ALL_STATIC_TRSH];

	if ( count < PMPC_WAIT_ALL_STATIC_TRSH )
	{
		parray_of_requests = static_parray_of_requests;
	}
	else
	{
		parray_of_requests = sctk_malloc( sizeof( mpc_mp_request_t * ) * count );
		assume( parray_of_requests != NULL );
	}

	/* Fill in the array of requests */
	for ( i = 0; i < count; i++ )
		parray_of_requests[i] = &( array_of_requests[i] );

	/* Call the pointer based waitall */
	res = _mpc_cl_waitallp( count, parray_of_requests, array_of_statuses );

	/* Do we need to free the temporary request pointer array */
	if ( PMPC_WAIT_ALL_STATIC_TRSH <= count )
	{
		sctk_free( parray_of_requests );
	}

	SCTK_PROFIL_END( MPC_Waitall );
	return res;
}

int _mpc_cl_waitsome( mpc_mp_msg_count_t incount, mpc_mp_request_t array_of_requests[],
					 mpc_mp_msg_count_t *outcount, mpc_mp_msg_count_t array_of_indices[],
					 mpc_mp_status_t array_of_statuses[] )
{
	int i;
	int done = 0;
	SCTK_PROFIL_START( MPC_Waitsome );
	while ( done == 0 )
	{
		for ( i = 0; i < incount; i++ )
		{
			if ( !mpc_mp_comm_request_is_null( &( array_of_requests[i] ) ) )
			{
				int tmp_flag = 0;
				_mpc_cl_test( &( array_of_requests[i] ), &tmp_flag,
						 &( array_of_statuses[done] ) );
				if ( tmp_flag )
				{
					mpc_mp_comm_request_set_null( &( array_of_requests[i] ), 1 );
					array_of_indices[done] = i;
					done++;
				}
			}
		}
		if ( done == 0 )
		{
			TODO( "wait_for_value_and_poll should be used here" )
			sctk_thread_yield();
		}
	}
	*outcount = done;
	SCTK_PROFIL_END( MPC_Waitsome );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_waitany( mpc_mp_msg_count_t count, mpc_mp_request_t array_of_requests[],
					mpc_mp_msg_count_t *index, mpc_mp_status_t *status )
{
	int i;
	SCTK_PROFIL_START( MPC_Waitany );
	*index = MPC_UNDEFINED;
	while ( 1 )
	{
		for ( i = 0; i < count; i++ )
		{
			if ( mpc_mp_comm_request_is_null( &( array_of_requests[i] ) ) != 1 )
			{
				int tmp_flag = 0;
				_mpc_cl_test( &( array_of_requests[i] ), &tmp_flag, status );
				if ( tmp_flag )
				{
					_mpc_cl_wait( &( array_of_requests[i] ), status );
					*index = count;
					SCTK_PROFIL_END( MPC_Waitany );
					MPC_ERROR_SUCESS();
				}
			}
		}
		TODO( "wait_for_value_and_poll should be used here" )
		sctk_thread_yield();
	}
}

int _mpc_cl_wait_pending( mpc_mp_communicator_t comm )
{
	int src;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();
	

	src = mpc_mp_communicator_rank( comm, task_specific->task_id );

	mpc_mp_comm_request_wait_all_msgs( src, comm );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_wait_pending_all_comm()
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	SCTK_PROFIL_START( MPC_Wait_pending_all_comm );

	mpc_per_communicator_t *per_communicator;
	mpc_per_communicator_t *per_communicator_tmp;

	mpc_common_spinlock_lock( &( task_specific->per_communicator_lock ) );
	HASH_ITER( hh, task_specific->per_communicator, per_communicator,
			   per_communicator_tmp )
	{
		int j = per_communicator->key;
		if ( sctk_is_valid_comm( j ) )
			_mpc_cl_wait_pending( j );
	}

	mpc_common_spinlock_unlock( &( task_specific->per_communicator_lock ) );

	SCTK_PROFIL_END( MPC_Wait_pending_all_comm );

	MPC_ERROR_SUCESS();
}

/************************************************************************/
/* MPI Status Modification and Query                                    */
/************************************************************************/


int _mpc_cl_request_get_status( mpc_mp_request_t request, int *flag, mpc_mp_status_t *status )
{
	__mpc_cl_request_commit_status( &request, status );

	*flag = ( request.completion_flag == SCTK_MESSAGE_DONE );

	return SCTK_SUCCESS;
}

int _mpc_cl_status_set_elements_x( mpc_mp_status_t *status, mpc_mp_datatype_t datatype, size_t count )
{
	if ( status == SCTK_STATUS_NULL )
		return SCTK_SUCCESS;

	size_t elem_size = 0;
	_mpc_cl_type_size( datatype, &elem_size );
	status->size = elem_size * count;

	return SCTK_SUCCESS;
}

int _mpc_cl_status_set_elements( mpc_mp_status_t *status, mpc_mp_datatype_t datatype, int count )
{
	return _mpc_cl_status_set_elements_x( status, datatype, count );
}


int _mpc_cl_status_get_count( mpc_mp_status_t *status, mpc_mp_datatype_t datatype, mpc_mp_msg_count_t *count )
{
	int res = SCTK_SUCCESS;
	unsigned long size;
	size_t data_size;

	if ( status == SCTK_STATUS_NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_IN_STATUS, "Invalid status" );
	}

	res = _mpc_cl_type_size( datatype, &data_size );

	if ( res != SCTK_SUCCESS )
	{
		return res;
	}

	if ( data_size != 0 )
	{
		size = status->size;
		sctk_nodebug( "Get_count : count %d, data_type %d (size %d)", size, datatype,
					  data_size );

		if ( size % data_size == 0 )
		{
			size = size / data_size;
			*count = size;
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


/*******************
 * MESSAGE PROBING *
 *******************/

int MPC_Iprobe_inter( const int source, const int destination, const int tag,
					  const mpc_mp_communicator_t comm, int *flag, mpc_mp_status_t *status )
{
	sctk_thread_message_header_t msg;
	memset( &msg, 0, sizeof( sctk_thread_message_header_t ) );

	mpc_mp_status_t status_init = SCTK_STATUS_INIT;

	

	int has_status = 1;

	if ( status == SCTK_STATUS_NULL )
	{
		has_status = 0;
	}
	else
	{
		*status = status_init;
	}

	*flag = 0;

	/*handler for SCTK_PROC_NULL*/
	if ( source == SCTK_PROC_NULL )
	{
		*flag = 1;

		if ( has_status )
		{
			status->MPC_SOURCE = SCTK_PROC_NULL;
			status->MPC_TAG = SCTK_ANY_TAG;
			status->size = 0;
			status->MPC_ERROR = SCTK_SUCCESS;
		}

		MPC_ERROR_SUCESS();
	}

	/* Value to check that the case was handled by
   * one of the if in this function */
	int __did_process = 0;

	if ( ( source == SCTK_ANY_SOURCE ) && ( tag == SCTK_ANY_TAG ) )
	{
		mpc_mp_comm_message_probe_any_source_any_tag( destination, comm, flag, &msg );
		__did_process = 1;
	}
	else if ( ( source != SCTK_ANY_SOURCE ) && ( tag != SCTK_ANY_TAG ) )
	{
		msg.message_tag = tag;
		mpc_mp_comm_message_probe( destination, source, comm, flag, &msg );
		__did_process = 1;
	}
	else if ( ( source != SCTK_ANY_SOURCE ) && ( tag == SCTK_ANY_TAG ) )
	{
		mpc_mp_comm_message_probe_any_tag( destination, source, comm, flag, &msg );
		__did_process = 1;
	}
	else if ( ( source == SCTK_ANY_SOURCE ) && ( tag != SCTK_ANY_TAG ) )
	{
		msg.message_tag = tag;
		mpc_mp_comm_message_probe_any_source( destination, comm, flag, &msg );
		__did_process = 1;
	}

	if ( *flag )
	{
		if ( has_status )
		{
			status->MPC_SOURCE = mpc_mp_communicator_rank( comm, msg.source_task );
			status->MPC_TAG = msg.message_tag;
			status->size = (mpc_mp_msg_count_t) msg.msg_size;
			status->MPC_ERROR = MPC_ERR_PENDING;
		}
		MPC_ERROR_SUCESS();
	}

	if ( !__did_process )
	{
		fprintf( stderr, "source = %d tag = %d\n", source, tag );
		not_reachable();
		MPC_ERROR_SUCESS();
	}

	MPC_ERROR_SUCESS();
}

int PMPC_Iprobe( int source, int tag, mpc_mp_communicator_t comm, int *flag,
				 mpc_mp_status_t *status )
{
	int destination;
	int res;

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	SCTK_PROFIL_START( MPC_Iprobe );

	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	

	destination = mpc_mp_communicator_rank( comm, task_specific->task_id );

	/* Translate ranks */

	if ( source != SCTK_ANY_SOURCE )
	{
		if ( sctk_is_inter_comm( comm ) )
		{
			source = sctk_get_remote_comm_world_rank( comm, source );
		}
		else
		{
			source = sctk_get_comm_world_rank( comm, source );
		}
	}
	else
	{
		source = SCTK_ANY_SOURCE;
	}

	destination = sctk_get_comm_world_rank( comm, destination );

	res = MPC_Iprobe_inter( source, destination, tag, comm, flag, status );

	SCTK_PROFIL_END( MPC_Iprobe );
	return res;
}

typedef struct
{
	int flag;
	int source;
	int destination;
	int tag;
	mpc_mp_communicator_t comm;
	mpc_mp_status_t *status;
} MPC_probe_struct_t;

static void MPC_Probe_poll( MPC_probe_struct_t *arg )
{
	MPC_Iprobe_inter( arg->source, arg->destination, arg->tag, arg->comm,
					  &( arg->flag ), arg->status );
}

static int __MPC_Probe( int source, int tag, mpc_mp_communicator_t comm, mpc_mp_status_t *status,
						mpc_mpi_m_per_mpi_process_ctx_t *task_specific )
{
	MPC_probe_struct_t probe_struct;
	int comm_rank = mpc_mp_communicator_rank( comm, task_specific->task_id );

	if ( source != SCTK_ANY_SOURCE )
	{
		if ( sctk_is_inter_comm( comm ) )
		{
			probe_struct.source = sctk_get_remote_comm_world_rank( comm, source );
		}
		else
		{
			probe_struct.source = sctk_get_comm_world_rank( comm, source );
		}
	}
	else
	{
		probe_struct.source = SCTK_ANY_SOURCE;
	}

	probe_struct.destination = sctk_get_comm_world_rank( comm, comm_rank );

	probe_struct.tag = tag;
	probe_struct.comm = comm;
	probe_struct.status = status;

	MPC_Iprobe_inter( probe_struct.source, probe_struct.destination, tag, comm,
					  &probe_struct.flag, status );

	if ( probe_struct.flag != 1 )
	{
		mpc_mp_comm_perform_idle(
			&probe_struct.flag, 1, (void ( * )( void * )) MPC_Probe_poll, &probe_struct );
	}
	MPC_ERROR_SUCESS();
}

int PMPC_Probe( int source, int tag, mpc_mp_communicator_t comm, mpc_mp_status_t *status )
{
	int res;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	SCTK_PROFIL_START( MPC_Probe );
	task_specific = __mpc_cl_per_mpi_process_ctx_get();
	
	res = __MPC_Probe( source, tag, comm, status, task_specific );
	SCTK_PROFIL_END( MPC_Probe );
	return res;
}

/*************************
 * MPC REDUCE OPERATIONS *
 *************************/

int _mpc_cl_op_create(sctk_Op_User_function *function, int commute, sctk_Op *op) {
  sctk_Op init = SCTK_OP_INIT;
  assume(commute);
  *op = init;
  op->u_func = function;

  MPC_ERROR_SUCESS();
}

int _mpc_cl_op_free(sctk_Op *op) {
  sctk_Op init = SCTK_OP_INIT;
  *op = init;
  MPC_ERROR_SUCESS();
}

/************************************************************************/
/* Collective operations                                                */
/************************************************************************/

int _mpc_cl_barrier( mpc_mp_communicator_t comm )
{
	int size;
	_mpc_cl_comm_size( comm, &size );
	

	if ( sctk_is_inter_comm( comm ) )
	{
		int buf = 0, rank;
		mpc_mpi_m_per_mpi_process_ctx_t *task_specific;

		task_specific = __mpc_cl_per_mpi_process_ctx_get();

		rank = mpc_mp_communicator_rank( comm, task_specific->task_id );

		/* Sync Local */
		if ( size > 1 )
			_mpc_cl_barrier( sctk_get_local_comm_id( comm ) );

		/* Sync A-B */
		if ( rank == 0 )
		{
			_mpc_cl_sendrecv( &buf, 1, MPC_INT, 0, MPC_BARRIER_TAG, &buf, 1, MPC_INT, 0, MPC_BARRIER_TAG, comm, SCTK_STATUS_NULL );
		}

		/* Sync Local */
		if ( size > 1 )
			_mpc_cl_barrier( sctk_get_local_comm_id( comm ) );
	}
	else
	{

		if ( size > 1 )
			mpc_mp_barrier( (mpc_mp_communicator_t) comm );
	}
	MPC_ERROR_SUCESS();
}

int _mpc_cl_bcast( void *buffer, mpc_mp_msg_count_t count,
				  mpc_mp_datatype_t datatype, int root, mpc_mp_communicator_t comm)
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	int size, rank;
	mpc_mp_status_t status;
	
	size = mpc_mp_communicator_size( comm );
	
	
	

	if ( sctk_is_inter_comm( comm ) )
	{
		if ( root == SCTK_PROC_NULL )
		{
			MPC_ERROR_SUCESS();
		}
		else if ( root == MPC_ROOT )
		{
			_mpc_cl_send( buffer, count, datatype, 0, MPC_BROADCAST_TAG, comm );
		}
		else
		{
			rank = mpc_mp_communicator_rank( comm, task_specific->task_id );

			if ( rank == 0 )
			{
				_mpc_cl_recv( buffer, count, datatype, root, MPC_BROADCAST_TAG, comm,
							 &status );
			}
			_mpc_cl_bcast( buffer, count, datatype, 0, sctk_get_local_comm_id( comm ) );
		}
	}
	else
	{
		if ( ( size == 0 ) || ( count == 0 ) )
		{
			MPC_ERROR_SUCESS();
		}
		else
		{
			mpc_mp_bcast( buffer,
				      count * __mpc_cl_datatype_get_size( datatype, task_specific ),
				      root, comm );
		}
	}
	MPC_ERROR_SUCESS();
}


#define MPC_MAX_CONCURENT 100

int _mpc_cl_gather( void *sendbuf, mpc_mp_msg_count_t sendcnt,
				   mpc_mp_datatype_t sendtype, void *recvbuf,
				   mpc_mp_msg_count_t recvcount, mpc_mp_datatype_t recvtype,
				   int root, mpc_mp_communicator_t comm )
{
	int i;
	int j;
	int rank;
	int size;
	mpc_mp_request_t request;
	size_t dsize;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	mpc_mp_request_t *recvrequest =
		sctk_malloc( sizeof( mpc_mp_request_t ) * MPC_MAX_CONCURENT );

	assume( recvrequest != NULL );

	
	size = mpc_mp_communicator_size( comm );
	rank = mpc_mp_communicator_rank( comm, task_specific->task_id );

	if ( rank != root )
	{
		
		
		
	}
	else
	{
		
		
		
	}
	

	if ( sctk_is_inter_comm( comm ) )
	{
		if ( root == SCTK_PROC_NULL )
		{
			MPC_ERROR_SUCESS();
		}
		else if ( root == MPC_ROOT )
		{
			i = 0;
			dsize = __mpc_cl_datatype_get_size( recvtype, task_specific );
			while ( i < size )
			{
				for ( j = 0; ( i < size ) && ( j < MPC_MAX_CONCURENT ); )
				{
					_mpc_cl_irecv( ( (char *) recvbuf ) + ( i * recvcount * dsize ), recvcount,
								  recvtype, i, MPC_GATHER_TAG, comm, &( recvrequest[j] ) );
					i++;
					j++;
				}
				j--;
				for ( ; j >= 0; j-- )
				{
					_mpc_cl_wait( &( recvrequest[j] ), SCTK_STATUS_NULL );
				}
			}
		}
		else
		{
			_mpc_cl_isend( sendbuf, sendcnt, sendtype, root, MPC_GATHER_TAG, comm,
						  &request );
			_mpc_cl_wait( &( request ), SCTK_STATUS_NULL );
		}
	}
	else
	{

		size = mpc_mp_communicator_size( comm );
		rank = mpc_mp_communicator_rank( comm, task_specific->task_id );

		if ( ( sendbuf == SCTK_IN_PLACE ) && ( rank == root ) )
		{
			request = mpc_request_null;
		}
		else
		{
			_mpc_cl_isend( sendbuf, sendcnt, sendtype, root, MPC_GATHER_TAG, comm,
						  &request );
		}

		if ( rank == root )
		{
			i = 0;
			dsize = __mpc_cl_datatype_get_size( recvtype, task_specific );
			while ( i < size )
			{
				for ( j = 0; ( i < size ) && ( j < MPC_MAX_CONCURENT ); )
				{
					if ( ( sendbuf == SCTK_IN_PLACE ) && ( i == root ) )
					{
						recvrequest[j] = mpc_request_null;
					}
					else
					{
						_mpc_cl_irecv( ( (char *) recvbuf ) + ( i * recvcount * dsize ), recvcount,
									  recvtype, i, MPC_GATHER_TAG, comm, &( recvrequest[j] ) );
					}
					i++;
					j++;
				}
				j--;
				_mpc_cl_waitall( j + 1, recvrequest, SCTK_STATUS_NULL );
			}
		}
		_mpc_cl_wait( &( request ), SCTK_STATUS_NULL );
	}

	sctk_free( recvrequest );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_allgather( void *sendbuf, mpc_mp_msg_count_t sendcount,
					  mpc_mp_datatype_t sendtype, void *recvbuf,
					  mpc_mp_msg_count_t recvcount,
					  mpc_mp_datatype_t recvtype, mpc_mp_communicator_t comm )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();
	int size, rank, remote_size;
	int root = 0;
	void *tmp_buf;
	
	
	
	
	
	
	if ( sendbuf != SCTK_IN_PLACE )
		

	size = mpc_mp_communicator_size( comm );
	rank = mpc_mp_communicator_rank( comm, task_specific->task_id );

	remote_size = mpc_mp_communicator_remote_size( comm );

	if ( sctk_is_inter_comm( comm ) )
	{
		if ( rank == 0 && sendcount > 0 )
		{
			tmp_buf = (void *) sctk_malloc( sendcount * size * sizeof( void * ) );
		}
		else
		{
			tmp_buf = NULL;
		}


		_mpc_cl_gather( sendbuf, sendcount, sendtype, tmp_buf, sendcount, sendtype, 0,
					   sctk_get_local_comm_id( comm ) );

		if ( sctk_is_in_local_group( comm ) )
		{
			if ( sendcount != 0 )
			{
				root = ( rank == 0 ) ? MPC_ROOT : SCTK_PROC_NULL;
				sctk_nodebug( "bcast size %d to the left", size * sendcount );
				_mpc_cl_bcast( tmp_buf, size * sendcount, sendtype, root, comm );
			}

			if ( recvcount != 0 )
			{
				root = 0;
				sctk_nodebug( "bcast size %d from the left", remote_size * recvcount );
				_mpc_cl_bcast( recvbuf, remote_size * recvcount, recvtype, root, comm );
			}
		}
		else
		{
			if ( recvcount != 0 )
			{
				root = 0;
				sctk_nodebug( "bcast size %d from the right", remote_size * recvcount );
				_mpc_cl_bcast( recvbuf, remote_size * recvcount, recvtype, root, comm );
			}

			if ( sendcount != 0 )
			{
				sctk_nodebug( "bcast size %d to the right", size * sendcount );
				root = ( rank == 0 ) ? MPC_ROOT : SCTK_PROC_NULL;
				_mpc_cl_bcast( tmp_buf, size * sendcount, sendtype, root, comm );
			}
		}
	}
	else
	{
		if ( SCTK_IN_PLACE == sendbuf )
		{
			size_t extent;
			_mpc_cl_type_size( recvtype, &extent );
			sendbuf = ( (char *) recvbuf ) + ( rank * extent * recvcount );
			sendtype = recvtype;
			sendcount = recvcount;
		}
		_mpc_cl_gather( sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
					   root, comm );
		_mpc_cl_bcast( recvbuf, size * recvcount, recvtype, root, comm );
	}
	MPC_ERROR_SUCESS();
}

/********************
 * GROUP MANAGEMENT *
 ********************/

int _mpc_cl_comm_group( mpc_mp_communicator_t comm, _mpc_cl_group_t **group )
{
	int size;
	int i;
	size = mpc_mp_communicator_size( comm );

	sctk_nodebug( "MPC_Comm_group" );

	*group = (_mpc_cl_group_t *) sctk_malloc( sizeof( _mpc_cl_group_t ) );

	( *group )->task_nb = size;
	( *group )->task_list_in_global_ranks = (int *) sctk_malloc( size * sizeof( int ) );

	for ( i = 0; i < size; i++ )
	{
		( *group )->task_list_in_global_ranks[i] = sctk_get_comm_world_rank( comm, i );
	}
	MPC_ERROR_SUCESS();
}


int _mpc_cl_comm_remote_group( mpc_mp_communicator_t comm, _mpc_cl_group_t **group )
{
	int size;
	int i;
	size = mpc_mp_communicator_remote_size( comm );
	sctk_nodebug( "MPC_Comm_group" );

	*group = (_mpc_cl_group_t *) sctk_malloc( sizeof( _mpc_cl_group_t ) );

	( *group )->task_nb = size;
	( *group )->task_list_in_global_ranks = (int *) sctk_malloc( size * sizeof( int ) );

	for ( i = 0; i < size; i++ )
	{
		( *group )->task_list_in_global_ranks[i] =
			sctk_get_remote_comm_world_rank( comm, i );
	}
	MPC_ERROR_SUCESS();
}


int _mpc_cl_group_free( _mpc_cl_group_t **group )
{
	if ( *group != MPC_GROUP_NULL )
	{
		sctk_free( ( *group )->task_list_in_global_ranks );
		sctk_free( *group );
		*group = MPC_GROUP_NULL;
	}
	MPC_ERROR_SUCESS();
}


int _mpc_cl_group_incl( _mpc_cl_group_t *group, int n, int *ranks, _mpc_cl_group_t **newgroup )
{
	int i;

	( *newgroup ) = (_mpc_cl_group_t *) sctk_malloc( sizeof( _mpc_cl_group_t ) );
	assume( ( *newgroup ) != NULL );

	( *newgroup )->task_nb = n;
	( *newgroup )->task_list_in_global_ranks = (int *) sctk_malloc( n * sizeof( int ) );
	assume( ( ( *newgroup )->task_list_in_global_ranks ) != NULL );

	for ( i = 0; i < n; i++ )
	{
		( *newgroup )->task_list_in_global_ranks[i] =
			group->task_list_in_global_ranks[ranks[i]];
		sctk_nodebug( "%d", group->task_list_in_global_ranks[ranks[i]] );
	}

	MPC_ERROR_SUCESS();
}

int _mpc_cl_group_difference( _mpc_cl_group_t *group1, _mpc_cl_group_t *group2, _mpc_cl_group_t **newgroup )
{
	int size;
	int i, j, k;
	int not_in;

	for ( i = 0; i < group1->task_nb; i++ )
		sctk_nodebug( "group1[%d] = %d", i, group1->task_list_in_global_ranks[i] );
	for ( i = 0; i < group2->task_nb; i++ )
		sctk_nodebug( "group2[%d] = %d", i, group2->task_list_in_global_ranks[i] );
	/* get the size of newgroup */
	size = 0;
	for ( i = 0; i < group1->task_nb; i++ )
	{
		not_in = 1;
		for ( j = 0; j < group2->task_nb; j++ )
		{
			if ( group1->task_list_in_global_ranks[i] ==
				 group2->task_list_in_global_ranks[j] )
				not_in = 0;
		}
		if ( not_in )
			size++;
	}

	/* allocation */
	*newgroup = (_mpc_cl_group_t *) sctk_malloc( sizeof( _mpc_cl_group_t ) );

	( *newgroup )->task_nb = size;
	if ( size == 0 )
	{
		MPC_ERROR_SUCESS();
	}

	( *newgroup )->task_nb = size;
	( *newgroup )->task_list_in_global_ranks =
		(int *) sctk_malloc( size * sizeof( int ) );

	/* fill the newgroup */
	k = 0;
	for ( i = 0; i < group1->task_nb; i++ )
	{
		not_in = 1;
		for ( j = 0; j < group2->task_nb; j++ )
		{
			if ( group1->task_list_in_global_ranks[i] ==
				 group2->task_list_in_global_ranks[j] )
				not_in = 0;
		}
		if ( not_in )
		{
			( *newgroup )->task_list_in_global_ranks[k] =
				group1->task_list_in_global_ranks[i];
			k++;
		}
	}

	MPC_ERROR_SUCESS();
}

/***************************
 * COMMUNICATOR MANAGEMENT *
 ***************************/

int _mpc_cl_comm_create_from_intercomm( mpc_mp_communicator_t comm,
				    _mpc_cl_group_t *group,
				    mpc_mp_communicator_t *comm_out )
{
	int rank;
	int i;
	int present = 0;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();
	
	rank = mpc_mp_communicator_rank( SCTK_COMM_WORLD, task_specific->task_id );

	sctk_assert( comm != SCTK_COMM_NULL );

	_mpc_cl_barrier( comm );

	for ( i = 0; i < group->task_nb; i++ )
	{
		if ( group->task_list_in_global_ranks[i] == rank )
		{
			sctk_nodebug( "%d present", rank );
			present = 1;
			break;
		}
	}

	for ( i = 0; i < group->task_nb; i++ )
	{
		sctk_nodebug( "task_list[%d] = %d", i, group->task_list_in_global_ranks[i] );
	}

	if ( sctk_is_in_local_group( comm ) )
		( *comm_out ) = sctk_create_communicator_from_intercomm(
			comm, group->task_nb, group->task_list_in_global_ranks );
	else
		( *comm_out ) = sctk_create_communicator_from_intercomm(
			comm, group->task_nb, group->task_list_in_global_ranks );
	sctk_nodebug( "\tnew comm from intercomm %d", *comm_out );
	_mpc_cl_barrier( comm );

	if ( present == 1 )
	{
		sctk_nodebug( "from intercomm barrier comm %d", *comm_out );
		_mpc_cl_barrier( *comm_out );
		sctk_nodebug( "sortie barrier" );
		__mpc_cl_per_communicator_alloc_from_existing( task_specific, *comm_out, comm );
	}
	sctk_nodebug( "comm %d created from intercomm %d", *comm_out, comm );
	MPC_ERROR_SUCESS();
}


static inline int __mpc_cl_comm_create( mpc_mp_communicator_t comm, _mpc_cl_group_t *group,
					mpc_mp_communicator_t *comm_out, int is_inter_comm )
{
	int grank, rank;
	int i;
	int present = 0;
	mpc_mp_status_t status;
	mpc_mp_communicator_t intra_comm;
	int remote_leader;
	int rleader;
	int local_leader = 0;
	int tag = 729;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();
	
	rank = mpc_mp_communicator_rank( SCTK_COMM_WORLD, task_specific->task_id );
	grank = mpc_mp_communicator_rank( comm, task_specific->task_id );

	sctk_assert( comm != SCTK_COMM_NULL );
	_mpc_cl_barrier( comm );

	for ( i = 0; i < group->task_nb; i++ )
	{
		if ( group->task_list_in_global_ranks[i] == rank )
		{
			present = 1;
			break;
		}
	}

	if ( is_inter_comm )
	{
		rleader = sctk_get_remote_leader( comm );
		/* get remote_leader */
		if ( grank == local_leader )
		{
			int tmp = group->task_list_in_global_ranks[local_leader];
			_mpc_cl_sendrecv( &tmp, 1, MPC_INT, rleader, tag, &remote_leader, 1, MPC_INT,
							 rleader, tag, sctk_get_peer_comm( comm ), &status );
		}
		_mpc_cl_bcast( &remote_leader, 1, MPC_INT, local_leader,
					  sctk_get_local_comm_id( comm ) );

		/* create local_comm */
		__mpc_cl_comm_create( sctk_get_local_comm_id( comm ), group, &intra_comm, 0 );

		_mpc_cl_barrier( comm );

		if ( intra_comm != SCTK_COMM_NULL )
		{
			/* create intercomm */
			( *comm_out ) = sctk_create_intercommunicator_from_intercommunicator(
				comm, remote_leader,
				intra_comm );
			sctk_nodebug( "\trank %d: new intercomm -> %d", rank, *comm_out );
		}
		else
			( *comm_out ) = SCTK_COMM_NULL;
	}
	else
	{
		( *comm_out ) = sctk_create_communicator( comm, group->task_nb,
												  group->task_list_in_global_ranks );
		sctk_nodebug( "\trank %d: new intracomm -> %d", rank, *comm_out );
	}

	_mpc_cl_barrier( comm );

	if ( present == 1 )
	{
		_mpc_cl_barrier( *comm_out );
		__mpc_cl_per_communicator_alloc_from_existing( task_specific, *comm_out, comm );
	}
	MPC_ERROR_SUCESS();
}

int _mpc_cl_comm_create( mpc_mp_communicator_t comm, _mpc_cl_group_t *group, mpc_mp_communicator_t *comm_out )
{
	return __mpc_cl_comm_create( comm, group, comm_out, sctk_is_inter_comm( comm ) );
}

static inline int *__mpc_cl_comm_task_list( mpc_mp_communicator_t comm, int size )
{
	int i;
	int *task_list = sctk_malloc( size * sizeof( int ) );
	for ( i = 0; i < size; i++ )
	{
		task_list[i] = sctk_get_comm_world_rank( comm, i );
	}
	return task_list;
}

int _mpc_cl_intercomm_create( mpc_mp_communicator_t local_comm, int local_leader,
				mpc_mp_communicator_t peer_comm, int remote_leader,
				int tag, mpc_mp_communicator_t *newintercomm )
{
	int rank, grank;
	int i;
	int present = 0;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();
	
	
	int *task_list;
	int size;
	int other_leader;
	int first = 0;
	mpc_mp_status_t status;

	rank = mpc_mp_communicator_rank( SCTK_COMM_WORLD, task_specific->task_id );
	grank = mpc_mp_communicator_rank( local_comm, task_specific->task_id );

	size = mpc_mp_communicator_size( local_comm );
	sctk_assert( local_comm != SCTK_COMM_NULL );

	task_list = __mpc_cl_comm_task_list( local_comm, size );

	if ( grank == local_leader )
	{
		_mpc_cl_sendrecv( &remote_leader, 1, MPC_INT, remote_leader, tag, &other_leader,
						 1, MPC_INT, remote_leader, tag, peer_comm, &status );
		if ( other_leader < remote_leader )
			first = 1;
		else
			first = 0;
	}

	_mpc_cl_bcast( &first, 1, MPC_INT, local_leader, local_comm );

	( *newintercomm ) = sctk_create_intercommunicator(
		local_comm, local_leader, peer_comm, remote_leader, tag, first );
	sctk_nodebug( "new intercomm %d", *newintercomm );

	_mpc_cl_barrier( local_comm );

	for ( i = 0; i < size; i++ )
	{
		if ( task_list[i] == rank )
		{
			present = 1;
			break;
		}
	}
	if ( present )
	{
		__mpc_cl_per_communicator_alloc_from_existing(
			task_specific, *newintercomm, local_comm );
		_mpc_cl_barrier( local_comm );
	}

	MPC_ERROR_SUCESS();
}


int _mpc_cl_comm_free( mpc_mp_communicator_t *comm )
{
	_mpc_cl_barrier(*comm);

	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	if ( *comm == SCTK_COMM_WORLD )
	{
		MPC_ERROR_SUCESS();
	}

	__mpc_cl_per_communicator_delete( task_specific, *comm );

	MPC_ERROR_SUCESS();
	mpc_mp_communicator_t old_comm = *comm;

	TODO("THIS CODE IS BAD LEADS TO DEADLOCKS");
	sctk_delete_communicator( old_comm );

	*comm = SCTK_COMM_NULL;

	MPC_ERROR_SUCESS();
}

int _mpc_cl_comm_dup( mpc_mp_communicator_t comm, mpc_mp_communicator_t *comm_out )
{
	sctk_nodebug( "duplicate comm %d", comm );
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	int rank;
	

	task_specific = __mpc_cl_per_mpi_process_ctx_get();
	rank = mpc_mp_communicator_rank( comm, task_specific->task_id );

	*comm_out = sctk_duplicate_communicator( comm, sctk_is_inter_comm( comm ), rank );
	if ( sctk_is_inter_comm( comm ) )
		sctk_nodebug( "intercomm %d duplicate --> newintercomm %d", comm, *comm_out );
	else
		sctk_nodebug( "comm %d duplicate --> newcomm %d", comm, *comm_out );

	__mpc_cl_per_communicator_alloc_from_existing_dup( task_specific,
							  *comm_out, comm );
	MPC_ERROR_SUCESS();
}

typedef struct
{
	int color;
	int key;
	int rank;
} mpc_comm_split_t;

int _mpc_cl_comm_split( mpc_mp_communicator_t comm, int color, int key, mpc_mp_communicator_t *comm_out )
{
	mpc_comm_split_t *tab;
	mpc_comm_split_t tab_this;
	mpc_comm_split_t tab_tmp;
	int size;
	int group_size;
	int i, j, k;
	int color_number;
	int *color_tab;
	int rank;
	_mpc_cl_group_t group;

	mpc_mp_communicator_t comm_res = SCTK_COMM_NULL;
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific;
	task_specific = __mpc_cl_per_mpi_process_ctx_get();

	

	sctk_nodebug( "MPC_Comm_split color=%d key=%d out_comm=%p", color, key,
				  comm_out );

	size = mpc_mp_communicator_size( comm );
	rank = mpc_mp_communicator_rank( comm, task_specific->task_id );

	sctk_nodebug( "rank %d : _mpc_cl_comm_split with comm %d, color %d and key %d",
				  rank, comm, color, key );
	tab = (mpc_comm_split_t *) sctk_malloc( size * sizeof( mpc_comm_split_t ) );
	color_tab = (int *) sctk_malloc( size * sizeof( int ) );

	tab_this.rank = rank;

	tab_this.color = color;
	tab_this.key = key;
	sctk_nodebug( "Allgather..." );
	_mpc_cl_allgather( &tab_this, sizeof( mpc_comm_split_t ), MPC_CHAR, tab,
					  sizeof( mpc_comm_split_t ), MPC_CHAR, comm );

	sctk_nodebug( "done" );
	/*Sort the new tab */
	for ( i = 0; i < size; i++ )
	{
		for ( j = 0; j < size - 1; j++ )
		{
			if ( tab[j].color > tab[j + 1].color )
			{
				tab_tmp = tab[j + 1];
				tab[j + 1] = tab[j];
				tab[j] = tab_tmp;
			}
			else
			{
				if ( ( tab[j].color == tab[j + 1].color ) &&
					 ( tab[j].key > tab[j + 1].key ) )
				{
					tab_tmp = tab[j + 1];
					tab[j + 1] = tab[j];
					tab[j] = tab_tmp;
				}
			}
		}
	}

	color_number = 0;
	for ( i = 0; i < size; i++ )
	{
		for ( j = 0; j < color_number; j++ )
		{
			if ( color_tab[j] == tab[i].color )
			{
				break;
			}
		}
		if ( j == color_number )
		{
			color_tab[j] = tab[i].color;
			color_number++;
		}
		sctk_nodebug( "rank %d color %d", i, tab[i].color, tab[i].rank );
	}

	sctk_nodebug( "%d colors", color_number );

	/*We need on comm_create per color */
	for ( k = 0; k < color_number; k++ )
	{
		int tmp_color = color_tab[k];
		if ( tmp_color != MPC_UNDEFINED )
		{
			group_size = 0;
			for ( i = 0; i < size; i++ )
			{
				if ( tab[i].color == tmp_color )
				{
					group_size++;
				}
			}

			group.task_nb = group_size;
			group.task_list_in_global_ranks =
				(int *) sctk_malloc( group_size * sizeof( int ) );

			j = 0;
			for ( i = 0; i < size; i++ )
			{
				if ( tab[i].color == tmp_color )
				{
					group.task_list_in_global_ranks[j] =
						sctk_get_comm_world_rank( comm, tab[i].rank );
					sctk_nodebug( "Thread %d color (%d) size %d on %d rank %d", tmp_color,
								  k, j, group_size, tab[i].rank );
					j++;
				}
			}

			__mpc_cl_comm_create( comm, &group, comm_out, sctk_is_inter_comm( comm ) );
			if ( *comm_out != SCTK_COMM_NULL )
			{
				comm_res = *comm_out;
				sctk_nodebug( "Split done -> new %s %d",
							  ( sctk_is_inter_comm( comm ) ) ? "intercomm" : "intracom",
							  comm_res );
			}

			sctk_free( group.task_list_in_global_ranks );
			sctk_nodebug( "Split color %d done", tmp_color );
		}
	}

	sctk_free( color_tab );
	sctk_free( tab );
	sctk_nodebug( "Split done" );
	*comm_out = comm_res;
	MPC_ERROR_SUCESS();
}

/************************************************************************/
/*  Error handling                                                      */
/************************************************************************/

int _mpc_cl_errhandler_create( MPC_Handler_function *function,
							MPC_Errhandler *errhandler )
{
	sctk_errhandler_register( (sctk_generic_handler) function,   (sctk_errhandler_t *) errhandler );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_errhandler_set( mpc_mp_communicator_t comm, MPC_Errhandler errhandler )
{
	sctk_handle_set_errhandler( (sctk_handle) comm, SCTK_HANDLE_COMM, (sctk_errhandler_t) errhandler );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_errhandler_get( mpc_mp_communicator_t comm, MPC_Errhandler *errhandler )
{
	*errhandler = (MPC_Errhandler) sctk_handle_get_errhandler( (sctk_handle) comm, SCTK_HANDLE_COMM );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_errhandler_free( MPC_Errhandler *errhandler )
{
	sctk_errhandler_free( *errhandler );
	*errhandler = (MPC_Errhandler) MPC_ERRHANDLER_NULL;
	MPC_ERROR_SUCESS();
}

#define err_case_sprintf( code, msg ) \
	case code:                                \
		sprintf( str, msg );                  \
		break

int _mpc_cl_error_string( int code, char *str, int *len )
{
	size_t lngt;
	str[0] = '\0';
	switch ( code )
	{
		err_case_sprintf( MPC_ERR_BUFFER, "Invalid buffer pointer" );
		err_case_sprintf( MPC_ERR_COUNT, "Invalid count argument" );
		err_case_sprintf( MPC_ERR_TYPE, "Invalid datatype argument" );
		err_case_sprintf( MPC_ERR_TAG, "Invalid tag argument" );
		err_case_sprintf( MPC_ERR_COMM, "Invalid communicator" );
		err_case_sprintf( MPC_ERR_RANK, "Invalid rank" );
		err_case_sprintf( MPC_ERR_ROOT, "Invalid root" );
		err_case_sprintf( MPC_ERR_TRUNCATE, "Message truncated on receive" );
		err_case_sprintf( MPC_ERR_GROUP, "Invalid group" );
		err_case_sprintf( MPC_ERR_OP, "Invalid operation" );
		err_case_sprintf( MPC_ERR_REQUEST, "Invalid mpc_request handle" );
		err_case_sprintf( MPC_ERR_TOPOLOGY, "Invalid topology" );
		err_case_sprintf( MPC_ERR_DIMS, "Invalid dimension argument" );
		err_case_sprintf( MPC_ERR_ARG, "Invalid argument" );
		err_case_sprintf( MPC_ERR_OTHER, "Other error; use Error_string" );
		err_case_sprintf( MPC_ERR_UNKNOWN, "Unknown error" );
		err_case_sprintf( MPC_ERR_INTERN, "Internal error code" );
		err_case_sprintf( MPC_ERR_IN_STATUS, "Look in status for error value" );
		err_case_sprintf( MPC_ERR_PENDING, "Pending request" );
		err_case_sprintf( MPC_NOT_IMPLEMENTED, "Not implemented" );
		err_case_sprintf( MPC_ERR_INFO, "Invalid Status argument" );
		err_case_sprintf( MPC_ERR_INFO_KEY, "Provided info key is too large" );
		err_case_sprintf( MPC_ERR_INFO_VALUE, "Provided info value is too large" );
		err_case_sprintf( MPC_ERR_INFO_NOKEY, "Could not locate a value with this key" );

		default:
			sctk_warning( "%d error code unknown", code );
	}

	*len = strlen( str );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_error_class( int errorcode, int *errorclass )
{
	*errorclass = errorcode;
	MPC_ERROR_SUCESS();
}

int _mpc_cl_abort( mpc_mp_communicator_t comm, int errorcode )
{
	SCTK_PROFIL_START( MPC_Abort );
	
	sctk_error( "MPC_Abort with error %d", errorcode );
	fflush( stderr );
	fflush( stdout );
	sctk_abort();
	SCTK_PROFIL_END( MPC_Abort );
	MPC_ERROR_SUCESS();
}

void _mpc_cl_default_error( mpc_mp_communicator_t *comm, int *error, char *msg, char *file,
						 int line )
{
	char str[1024];
	int i;
	_mpc_cl_error_string( *error, str, &i );
	if ( i != 0 )
		sctk_error( "%s file %s line %d %s", str, file, line, msg );
	else
		sctk_error( "Unknown error" );
	_mpc_cl_abort( *comm, *error );
}

void _mpc_cl_return_error( mpc_mp_communicator_t *comm, int *error, ... )
{
	char str[1024];
	int i;
	_mpc_cl_error_string( *error, str, &i );
	if ( i != 0 )
		sctk_error( "Warning: %s on comm %d", str, (int) *comm );
}

void _mpc_cl_abort_error( mpc_mp_communicator_t *comm, int *error, char *message, char *file,
					   int line )
{
	char str[1024];
	int i;
	_mpc_cl_error_string( *error, str, &i );

	sctk_error( "===================================================" );
	if ( i != 0 )
		sctk_error( "Error: %s on comm %d", str, (int) *comm, message );

	sctk_error( "This occured at %s:%d", file, line );

	sctk_error( "MPC Encountered an Error will now abort" );
	sctk_error( "===================================================" );
	sctk_abort();
}


static volatile int error_init_done = 0;

int _mpc_cl_error_init()
{
	if ( error_init_done == 0 )
	{
		error_init_done = 1;

		sctk_errhandler_register_on_slot( (sctk_generic_handler) _mpc_cl_default_error,
						MPC_ERRHANDLER_NULL );
		sctk_errhandler_register_on_slot( (sctk_generic_handler) _mpc_cl_return_error,
						MPC_ERRORS_RETURN );
		sctk_errhandler_register_on_slot( (sctk_generic_handler) _mpc_cl_abort_error,
						MPC_ERRORS_ARE_FATAL );

		_mpc_cl_errhandler_set( SCTK_COMM_WORLD, MPC_ERRORS_ARE_FATAL );
		_mpc_cl_errhandler_set( SCTK_COMM_SELF, MPC_ERRORS_ARE_FATAL );
	}

	MPC_ERROR_SUCESS();
}


#ifdef MPC_Fault_Tolerance
static volatile MPC_Checkpoint_state global_state = MPC_STATE_ERROR;
#else
static volatile MPC_Checkpoint_state global_state = '\0';
#endif

/**
 * Trigger a checkpoint for the whole application.
 *
 * We are aware this functino implements somehow a barrier through three "expensive" atomics
 * But keep in mind that creating a checkpoint for the whole application is far more expensive
 * than that.
 *
 * This function sets "state" depending on the application state:
 *   - If we reach this function, it is a simple checkpoint -> MPC_STATE_CHECKPOINT
 *   - If we restart from a snapshot, it is a restart -> MPC_STATE_RESTART
 *
 * \param[out] state The state after the procedure.
 */
int _mpc_cl_checkpoint( MPC_Checkpoint_state *state )
{
#ifdef MPC_Fault_Tolerance
	if ( sctk_checkpoint_mode )
	{
		int total_nbprocs = mpc_common_get_process_count();
		int local_nbtasks = mpc_common_get_local_task_count();
		int local_tasknum = mpc_common_get_local_task_rank();
		int task_rank = mpc_common_get_task_rank();
		int pmi_rank = -1;
		static OPA_int_t init_once = OPA_INT_T_INITIALIZER( 0 );
		static OPA_int_t gen_acquire = OPA_INT_T_INITIALIZER( 0 );
		static OPA_int_t gen_release = OPA_INT_T_INITIALIZER( 0 );
		static OPA_int_t gen_current = OPA_INT_T_INITIALIZER( 0 );
		static int *task_generations;

		/* init once the genration array */
		if ( OPA_cas_int( &init_once, 0, 1 ) == 0 )
		{
			task_generations = (int *) malloc( sizeof( int ) * local_nbtasks );
			memset( task_generations, 0, sizeof( int ) * local_nbtasks );
			OPA_store_int( &init_once, 2 );
		}
		else
		{
			while ( OPA_load_int( &init_once ) != 2 )
				sctk_thread_yield();
		}

		/* ensure there won't be any overlapping betwen different MPC_Checkpoint() calls */
		while ( OPA_load_int( &gen_current ) < task_generations[local_tasknum] )
			sctk_thread_yield();

		/* if I'm the last task to process: do the work */
		if ( OPA_fetch_and_incr_int( &gen_acquire ) == local_nbtasks - 1 )
		{
			/* save the old checkpoint/restart counters */
			sctk_ft_checkpoint_init();

			/* synchronize all processes */
			sctk_ft_disable();
			if ( total_nbprocs > 1 )
			{
				mpc_common_get_process_rank( &pmi_rank );
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
			if ( pmi_rank == 0 )
			{
				sctk_ft_checkpoint(); /* we can ignore the return value here, by construction */
			}

			global_state = sctk_ft_checkpoint_wait();

			sctk_ft_checkpoint_finalize();

			/* set gen_release to 0, prepare the end of current generation */
			OPA_store_int( &gen_release, 0 );
			/* set gen_aquire to 0: unlock waiting tasks */
			OPA_store_int( &gen_acquire, 0 );
		}
		else
		{
			/* waiting tasks */
			while ( OPA_load_int( &gen_acquire ) != 0 )
				sctk_thread_yield();
		}

		/* update the state for all tasks of this process */
		if ( state )
			*state = global_state;

		/* re-init the network at task level if necessary */
		sctk_net_init_task_level( task_rank, mpc_common_topo_get_current_cpu() );
		mpc_mp_terminaison_barrier( task_rank );

		/* If I'm the last task to reach here, increment the global generation counter */
		if ( OPA_fetch_and_incr_int( &gen_release ) == local_nbtasks - 1 )

			/* the current task finished the work for the current generation */
			task_generations[local_tasknum]++;
	}
	else
	{
		if ( state )
			*state = MPC_STATE_IGNORE;
	}
#else
	if ( state )
		*state = MPC_STATE_NO_SUPPORT;
#endif

	MPC_ERROR_SUCESS();
}


/**********
 * TIMING *
 **********/

double _mpc_cl_wtime()
{
	double res;
	SCTK_PROFIL_START( MPC_Wtime );
#if SCTK_WTIME_USE_GETTIMEOFDAY
	struct timeval tp;

	gettimeofday( &tp, NULL );
	res = (double) tp.tv_usec + (double) tp.tv_sec * (double) 1000000;
	res = res / (double) 1000000;

#else
	res = sctk_atomics_get_timestamp_tsc();
#endif
	sctk_nodebug( "Wtime = %f", res );
	SCTK_PROFIL_END( MPC_Wtime );
	return res;
}

double _mpc_cl_wtick()
{
	return 10e-6;
}

/************************************************************************/
/* Pack managment                                                       */
/************************************************************************/

int _mpc_cl_open_pack( mpc_mp_request_t *request )
{
	SCTK_PROFIL_START( MPC_Open_pack );

	if ( request == NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_REQUEST, "" );
	}

	mpc_mp_ptp_message_t *msg = mpc_mp_comm_ptp_message_header_create( SCTK_MESSAGE_PACK_UNDEFINED );

	mpc_mp_comm_request_set_msg( request, msg );
	mpc_mp_comm_request_set_size( request );
	SCTK_PROFIL_END( MPC_Open_pack );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_add_pack( void *buf, mpc_mp_msg_count_t count,
			mpc_pack_indexes_t *begins,
			mpc_pack_indexes_t *ends,
			mpc_mp_datatype_t datatype, mpc_mp_request_t *request )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	if ( request == NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_REQUEST, "" );
	}
	if ( begins == NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG, "" );
	}
	if ( ends == NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG, "" );
	}
	if ( count == 0 )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG, "" );
	}

	size_t data_size = __mpc_cl_datatype_get_size( datatype, task_specific );

	mpc_mp_ptp_message_t *msg = mpc_mp_comm_request_get_msg( request );

	mpc_mp_comm_ptp_message_add_pack( msg, buf, count, data_size, begins, ends );

	mpc_mp_comm_request_set_msg( request, msg );

	size_t total = 0;
	int i;
	/*Compute message size */
	for ( i = 0; i < count; i++ )
	{
		total += ends[i] - begins[i] + 1;
	}
	mpc_mp_comm_request_inc_size( request, total * data_size );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_add_pack_absolute( void *buf, mpc_mp_msg_count_t count,
					mpc_pack_absolute_indexes_t *begins,
					mpc_pack_absolute_indexes_t *ends,
					mpc_mp_datatype_t datatype,
					mpc_mp_request_t *request )
{
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	size_t data_size = __mpc_cl_datatype_get_size( datatype, task_specific );

	if ( request == NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_REQUEST, "" );
	}
	if ( begins == NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG, "" );
	}
	if ( ends == NULL )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_ARG, "" );
	}

	mpc_mp_ptp_message_t *msg = mpc_mp_comm_request_get_msg( request );

	mpc_mp_comm_ptp_message_add_pack_absolute( msg, buf, count, data_size, begins, ends );

	mpc_mp_comm_request_set_msg( request, msg );

	size_t total = 0;
	int i;
	/*Compute message size */
	for ( i = 0; i < count; i++ )
	{
		total += ends[i] - begins[i] + 1;
	}

	mpc_mp_comm_request_inc_size( request, total * data_size );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_isend_pack( int dest, int tag, mpc_mp_communicator_t comm, mpc_mp_request_t *request )
{
	SCTK_PROFIL_START( MPC_Isend_pack );
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	int src = mpc_mp_communicator_rank( comm, task_specific->task_id );

	if ( dest == SCTK_PROC_NULL )
	{
		mpc_mp_comm_request_init( request, comm, REQUEST_SEND );
		MPC_ERROR_SUCESS();
	}

	if ( request == NULL )
	{
		MPC_ERROR_REPORT( comm, MPC_ERR_REQUEST, "" );
	}

	mpc_mp_ptp_message_t *msg = mpc_mp_comm_request_get_msg( request );

	mpc_mp_comm_ptp_message_header_init( msg, tag, comm, src, dest, request,
					mpc_mp_comm_request_get_size( request ),
					SCTK_P2P_MESSAGE, MPC_PACKED, REQUEST_SEND );
	mpc_mp_comm_ptp_message_send( msg );
	SCTK_PROFIL_END( MPC_Isend_pack );
	MPC_ERROR_SUCESS();
}

int _mpc_cl_irecv_pack( int source, int tag, mpc_mp_communicator_t comm, mpc_mp_request_t *request )
{

	SCTK_PROFIL_START( MPC_Irecv_pack );
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	int src = mpc_mp_communicator_rank( comm, task_specific->task_id );

	if ( source == SCTK_PROC_NULL )
	{
		mpc_mp_comm_request_init( request, comm, REQUEST_RECV );
		MPC_ERROR_SUCESS();
	}

	if ( request == NULL )
	{
		MPC_ERROR_REPORT( comm, MPC_ERR_REQUEST, "" );
	}

	mpc_mp_ptp_message_t *msg = mpc_mp_comm_request_get_msg( request );

	mpc_mp_comm_ptp_message_header_init( msg, tag, comm, source, src, request,
						mpc_mp_comm_request_get_size( request ),
						SCTK_P2P_MESSAGE, MPC_PACKED, REQUEST_RECV );

	mpc_mp_comm_ptp_message_recv( msg );
	SCTK_PROFIL_END( MPC_Irecv_pack );
	MPC_ERROR_SUCESS();
}

/************************************************************************/
/* MPI_Info management                                                  */
/************************************************************************/

/* Utilities functions for this section are located in mpc_info.[ch] */

int _mpc_cl_info_create( MPC_Info *info )
{
	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* First set to NULL */
	*info = MPC_INFO_NULL;

	/* Create a new entry */
	int new_id = MPC_Info_factory_create( &task_specific->info_fact );

	/* We did not get a new entry */
	if ( new_id < 0 )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN,
						  "Failled to allocate new MPI_Info" );
	}

	/* Then set the new ID */
	*info = new_id;

	/* All clear */
	MPC_ERROR_SUCESS();
}

int _mpc_cl_info_free( MPC_Info *info )
{
	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Try to delete from factory */
	int ret = MPC_Info_factory_delete( &task_specific->info_fact, (int) *info );

	if ( ret )
	{
		/* Failed to delete no such info */
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO,
						  "Failled to delete MPI_Info" );
	}
	else
	{
		/* Delete was successful */
		*info = MPC_INFO_NULL;
		MPC_ERROR_SUCESS();
	}
}

int _mpc_cl_info_set( MPC_Info info, const char *key, const char *value )
{
	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve( &task_specific->info_fact, (int) info );

	if ( !cell )
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO, "Failled to get MPI_Info" );
	}

	/* Check for lenght boundaries */
	int keylen = strlen( key );
	int valuelen = strlen( value );

	if ( MPC_MAX_INFO_KEY <= keylen )
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO_KEY, "" );

	if ( MPC_MAX_INFO_VAL <= valuelen )
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO_VALUE, "" );

	/* Now set the key */
	MPC_Info_cell_set( cell, (char *) key, (char *) value, 1 );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_info_delete( MPC_Info info, const char *key )
{
	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve( &task_specific->info_fact, (int) info );

	if ( !cell )
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO,
						  "Failled to delete MPI_Info" );
	}

	int ret = MPC_Info_cell_delete( cell, (char *) key );

	if ( ret )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO_NOKEY,
						  " Failled to delete key" );
	}

	MPC_ERROR_SUCESS();
}

int _mpc_cl_info_get( MPC_Info info, const char *key, int valuelen, char *value,
					 int *flag )
{
	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve( &task_specific->info_fact, (int) info );

	if ( !cell )
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO,
						  "Failled to delete MPI_Info" );
	}

	MPC_Info_cell_get( cell, (char *) key, value, valuelen, flag );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_info_get_nkeys( MPC_Info info, int *nkeys )
{
	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	*nkeys = 0;

	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve( &task_specific->info_fact, (int) info );

	if ( !cell )
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO,
						  "Failled to get MPI_Info key count" );
	}

	*nkeys = MPC_Info_key_count( cell->keys );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_info_get_count( MPC_Info info, int n, char *key )
{
	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve( &task_specific->info_fact, (int) info );

	if ( !cell )
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO,
						  "Failled to get MPI_Info key count" );
	}

	/* Thry to get the nth entry */
	struct MPC_Info_key *keyEntry = MPC_Info_key_get_nth( cell->keys, n );

	/* Not found */
	if ( !keyEntry )
	{
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO_NOKEY,
						  "Failled to retrieve an MPI_Info's key" );
	}
	else
	{
		/* Found just snprintf the key */
		snprintf( key, MPC_MAX_INFO_KEY, "%s", keyEntry->key );
	}

	MPC_ERROR_SUCESS();
}

int _mpc_cl_info_dup( MPC_Info info, MPC_Info *newinfo )
{
	/* First create a new entry */
	int ret = _mpc_cl_info_create( newinfo );

	if ( ret != SCTK_SUCCESS )
		return ret;

	/* Prepare to copy keys one by one */
	int num_keys = 0;
	char keybuf[MPC_MAX_INFO_KEY];
	/* This is the buffer for the value */
	char *valbuff = sctk_malloc( MPC_MAX_INFO_VAL * sizeof( char ) );

	if ( !valbuff )
	{
		perror( "sctk_alloc" );
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INTERN,
						  "Failled to allocate temporary value buffer" );
	}

	int i;

	_mpc_cl_info_get_nkeys( info, &num_keys );

	int flag = 0;

	/* Copy keys */
	for ( i = 0; i < num_keys; i++ )
	{
		/* Get key */
		_mpc_cl_info_get_count( info, i, keybuf );
		/* Get value */
		_mpc_cl_info_get( info, keybuf, MPC_MAX_INFO_VAL, valbuff, &flag );

		if ( !flag )
		{
			/* Shall not happen */
			sctk_free( valbuff );
			MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO_NOKEY,
							  "Could not retrieve a key which should have been there" );
		}

		/* Store value at key in the newly allocated object */
		_mpc_cl_info_set( *newinfo, keybuf, valbuff );
	}

	sctk_free( valbuff );

	MPC_ERROR_SUCESS();
}

int _mpc_cl_info_get_valuelen( MPC_Info info, char *key, int *valuelen, int *flag )
{
	/* Retrieve task context */
	mpc_mpi_m_per_mpi_process_ctx_t *task_specific = __mpc_cl_per_mpi_process_ctx_get();

	/* Locate the cell */
	struct MPC_Info_cell *cell =
		MPC_Info_factory_resolve( &task_specific->info_fact, (int) info );

	if ( !cell )
	{
		/* We failed to locate the info */
		MPC_ERROR_REPORT( SCTK_COMM_WORLD, MPC_ERR_INFO,
						  "Failled to get MPI_Info key count" );
	}

	/* Retrieve the cell @ key */
	struct MPC_Info_key *keyEntry = MPC_Info_key_get( cell->keys, (char *) key );

	if ( !keyEntry )
	{
		/* Nothing found, set flag */
		*flag = 0;
	}
	else
	{
		/* We found it ! set length with strlen */
		*flag = 1;
		*valuelen = strlen( keyEntry->value );
	}

	MPC_ERROR_SUCESS();
}
