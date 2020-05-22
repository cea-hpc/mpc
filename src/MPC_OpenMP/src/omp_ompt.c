#include "omp_ompt.h"


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include<execinfo.h>


#include "ompt.h"
#include "mpcomp_abi.h"
#include "mpcomp_types.h"



#include "mpcomp_openmp_tls.h"

/***********************
 * CALLBACK DEFINITION *
 ***********************/

typedef void ( *ompt_callback_t )( void );

typedef int ( *ompt_enumerate_states_t )(
    int current_state,
    int *next_state,
    const char **next_state_name
);

typedef int ( *ompt_enumerate_mutex_impls_t )(
    int current_impl,
    int *next_impl,
    const char **next_impl_name
);

typedef struct ompt_fns_t ompt_fns_t;
typedef void ( *ompt_interface_fn_t )( void );

/**************
 * OMPT TYPES *
 **************/

typedef struct ompt_enumerate_info_s
{
	const char *name;
	unsigned long id;
} ompt_enumerate_info_t;

typedef struct __ompt_callback_info_s
{
	const char *name;
	ompt_callbacks_t id;
	ompt_set_result_t status;
} __ompt_callback_info_t;

typedef enum tool_setting_e
{
	omp_tool_error,
	omp_tool_unset,
	omp_tool_disabled,
	omp_tool_enabled
} tool_setting_t;

typedef int ( *mpcomp_ompt_initialize_t )
(
    ompt_function_lookup_t lookup,
    struct ompt_fns_t *fns
);

typedef void ( *mpcomp_ompt_finalize_t )
(
    struct ompt_fns_t *fns
);

/********************
 * GLOBAL VARIABLES *
 ********************/

static ompt_enumerate_info_t __ompt_state_info[] =
{
#define ompt_state_macro(state, code, desc) { # state, code },
	FOREACH_OMPT_STATE( ompt_state_macro )
#undef ompt_state_macro
};

static ompt_enumerate_info_t __ompt_mutex_impls_info[] =
{
#define ompt_mutex_impl_macro(mutex_impl, code, desc) { # mutex_impl, mutex_impl },
	FOREACH_OMPT_MUTEX_IMPL( ompt_mutex_impl_macro )
#undef ompt_mutex_impl_macro
};

static __ompt_callback_info_t __ompt_callback_info[] =
{
#define ompt_callback_macro(callback, code, status) { # callback, code, status },
	FOREACH_OMPT_CALLBACK( ompt_callback_macro )
#undef ompt_callback_macro
};

__UNUSED__ static ompt_enumerate_info_t __ompt_set_result_info[] =
{
#define ompt_set_result_macro(name, code)	{ # name, name },
	FOREACH_OMPT_SET_RESULT( ompt_set_result_macro )
#undef ompt_set_result_macro
};

//TODO: should be task specific value
int _omp_ompt_enabled_flag = 0;

//TODO: should be task specific value
ompt_callback_t *OMPT_Callbacks = NULL;

//TODO: should be task specific value
static ompt_fns_t *__ompt_fns = NULL;

const ompt_data_t ompt_data_none = ( ompt_data_t )
{
	.value = 0
};

#pragma weak ompt_start_tool
ompt_fns_t* ompt_start_tool( __UNUSED__ unsigned int omp_version, __UNUSED__ const char *runtime_version )
{
   mpc_common_debug("No ompt_start_tool function from tool");
   return NULL;
}


void _mpc_ompt_pre_init( void )
{
	//TODO: Should be task specific ONCE
	static int ompt_pre_init_once_processus = 0;

	if ( ompt_pre_init_once_processus )
	{
		return;
	}

	// Parse OMP_TOOL env variable
	const char *ompt_env_var = getenv( "OMP_TOOL" );
	tool_setting_t tool_setting = omp_tool_error;

	if ( !ompt_env_var || !strcmp( ompt_env_var, "" ) )
	{
		tool_setting = omp_tool_unset;
	}
	else if ( !strcasecmp( ompt_env_var, "disabled" ) )
	{
		tool_setting = omp_tool_disabled;
	}
	else if ( !strcasecmp( ompt_env_var, "enabled" ) )
	{
		tool_setting = omp_tool_enabled;
	}
	else
	{
		// Unexpected value
		tool_setting = omp_tool_error;
	}

	switch ( tool_setting )
	{
		/* OMPT DISABLED */
		case omp_tool_error:
			fprintf( stderr, "Warning: OMPT_TOOL has invalid value" );

		case omp_tool_disabled:
			break;

		/* OMPT ENABLED */
		case omp_tool_unset:
		case omp_tool_enabled:
			__ompt_fns = ompt_start_tool( _OPENMP, "mpc-omp" );

			if ( __ompt_fns )
			{
				mpc_common_debug_error( "OMPT is ENABLED ..." );
				_omp_ompt_enabled_flag = 1;
			}
	}
}

static ompt_interface_fn_t __ompt_fn_lookup( const char *name );

void _mpc_ompt_post_init( void )
{
	if ( _omp_ompt_enabled_flag && __ompt_fns )
	{
		__ompt_fns->initialize( __ompt_fn_lookup, __ompt_fns );
	}
}


/*******************
 * ENUMERATE STATE *
 *******************/


static int ompt_enumerate_states( int cur_state, int *next_state, const char **next_state_name )
{
	int i;
	static const int len = sizeof( __ompt_state_info ) / sizeof( ompt_enumerate_info_t );
	assert( len > 0 );

	if ( cur_state == ompt_state_undefined )
	{
		i = -1; /* next state -> i = 0 */
	}
	else
	{
		/* Find current state in __ompt_state_info tabular */
		for ( i = 0; i < len - 1; i++ )
		{
			if ( __ompt_state_info[i].id != ( unsigned long )cur_state )
			{
				continue;
			}

			break;
		}
	}

	/* get next value */
	i++;

	if ( i < len - 1 )	/* Found */
	{
		*next_state = __ompt_state_info[i].id;
		*next_state_name = __ompt_state_info[i].name;
	}

	return ( i < len - 1 ) ? 1 : 0;
}


/*******************
 * ENUMERATE MUTEX *
 *******************/


static int ompt_enumerate_mutex_impls( int current_mutex, int *next_mutex, const char **next_name )
{
	uint64_t i;
	static const uint64_t len = sizeof( __ompt_mutex_impls_info ) / sizeof( ompt_enumerate_info_t );
	assert( len > 0 );

	/* Find current state in __ompt_state_info tabular */
	for ( i = 0; i < len - 1; i++ )
	{
		if ( __ompt_mutex_impls_info[i].id != ( unsigned long )current_mutex )
		{
			continue;
		}

		break;
	}

	if ( i < len - 1 )	/* Found */
	{
		*next_mutex = __ompt_mutex_impls_info[i + 1].id;
		*next_name = __ompt_mutex_impls_info[i + 1].name;
	}

	return ( i < len - 1 ) ? 1 : 0;
}


/********************
 * SET/GET CALLBACK *
 ********************/


static ompt_set_result_t mpcomp_ompt_get_callback_status( ompt_callbacks_t callback_type )
{
	uint64_t i;
	const uint64_t len = sizeof( __ompt_callback_info ) / sizeof( __ompt_callback_info_t );
	assert( len > 0 );
	ompt_set_result_t retval = ompt_set_error;

	for ( i = 0; i < len - 1; i++ )
	{
		if ( __ompt_callback_info[i].id != callback_type )
		{
			continue;
		}

		retval = __ompt_callback_info[i].status;
		break;
	}

	return retval;
}

static int ompt_set_callback( ompt_callbacks_t callback_type, ompt_callback_t callback )
{
	ompt_set_result_t status;
	// callbacks begin with value 1
	const uint64_t len = sizeof( __ompt_callback_info ) / sizeof( __ompt_callback_info_t ) + 1;
	assert( callback_type > 0 && callback_type < len );
	status = mpcomp_ompt_get_callback_status( callback_type );

	if ( status != ompt_set_error )
	{
		if ( !OMPT_Callbacks )
		{
			OMPT_Callbacks = ( ompt_callback_t * )malloc( len * sizeof( ompt_callback_t ) );
			assert( OMPT_Callbacks );
			memset( OMPT_Callbacks, 0, len * sizeof( ompt_callback_t ) );
		}

		OMPT_Callbacks[callback_type] = callback;
	}

	return status;
}

#define ompt_get_callback_success 1
#define ompt_get_callback_failure 0

static int ompt_get_callback( ompt_callbacks_t callback_type, ompt_callback_t *callback )
{
	ompt_set_result_t status;
	int retval = ompt_get_callback_failure;
	status = mpcomp_ompt_get_callback_status( callback_type );

	if ( status != ompt_set_error && OMPT_Callbacks )
	{
		*callback = OMPT_Callbacks[callback_type];
		retval = ompt_get_callback_success;
	}

	return retval;
}

static ompt_data_t *ompt_get_thread_data( void )
{
	mpcomp_thread_t *thread_infos = mpcomp_get_thread_tls();
	return ( thread_infos ) ? &( thread_infos->ompt_thread_data ) : NULL;
}

static ompt_state_t ompt_get_state( __UNUSED__ ompt_wait_id_t *wait_id )
{
	mpcomp_thread_t *thread_infos = mpcomp_get_thread_tls();
	return ( thread_infos ) ? thread_infos->state : ompt_state_undefined;
}

static int ompt_get_parallel_info( int ancestor_level, ompt_data_t **parallel_data, int *team_size )
{
	mpcomp_thread_t *thread_infos = mpcomp_get_thread_tls();

	if ( ancestor_level != 0 ) // NO NESTED REGION
	{
		*parallel_data = NULL;
		*team_size = 0;
		return 0;
	}

	*parallel_data = ( thread_infos ) ? &( thread_infos->info.ompt_region_data ) : NULL;
	*team_size = ( thread_infos ) ? thread_infos->info.num_threads : 0;
	return 1;
}


static int ompt_get_task_info( 	int ancestor_level,
                                ompt_task_flag_t *type,
                                ompt_data_t **task_data,
                                ompt_frame_t **task_frame,
                                ompt_data_t **parallel_data,
                                int *thread_num )
{
#if MPCOMP_TASK
	int i;
	mpcomp_task_t *current_task = NULL;
	mpcomp_thread_t *thread_infos = mpcomp_get_thread_tls();

	if ( !thread_infos )
	{
		return 0;
	}

	current_task = thread_infos->task_infos.current_task;

	for ( i = 0; i < ancestor_level; i++ )
	{
		current_task = current_task->parent;

		if ( current_task == NULL )
		{
			break;
		}
	}

	if ( !current_task )
	{
		*type = ompt_task_undefined;
		*task_data = NULL;
		*task_frame = NULL;
		*parallel_data = NULL;
		*thread_num = 0;
	}
	else
	{
		*type = ompt_task_undefined;
		*task_data = &( current_task->ompt_task_data );
		*task_frame = &( current_task->ompt_task_frame );

		if ( !ompt_get_parallel_info( 0, parallel_data, thread_num ) )
		{
			*parallel_data = NULL;
			*thread_num = 0;
		}
	}

	return ( current_task ) ? 1 : 0;
#endif /* MPCOMP_TASK */
	return 0;
}


#if OMPT_SUPPORT
static ompt_interface_fn_t ompt_fn_lookup(const char* name)
{
#define ompt_interface_fn(fn) 												\
	if(strcmp(name, #fn ) == 0) return (ompt_interface_fn_t) fn;
	FOREACH_OMPT_INQUIRY_FN(ompt_interface_fn)
#undef ompt_interface_fn
	return (ompt_interface_fn_t) 0;
}
#endif

static ompt_interface_fn_t __ompt_fn_lookup( const char *name )
{
#define ompt_interface_fn(fn) 												\
	if(strcmp(name, #fn ) == 0) return (ompt_interface_fn_t) fn;
	FOREACH_OMPT_INQUIRY_FN( ompt_interface_fn )
#undef ompt_interface_fn
	return ( ompt_interface_fn_t ) 0;
}

void *__ompt_get_return_address( unsigned int level )
{
	const int real_level = level + 2;
	void **array = ( void ** ) malloc( real_level * sizeof( void * ) );
	const int size = backtrace( array, real_level );
	void *retval = ( size == real_level ) ? array[real_level - 1] : NULL;
	free( array );
	return retval;
}


static OPA_int_t mpcomp_OMPT_wait_id = OPA_INT_T_INITIALIZER( 1 );
static OPA_int_t mpcomp_OMPT_task_id = OPA_INT_T_INITIALIZER( 1 );
static OPA_int_t mpcomp_OMPT_thread_id = OPA_INT_T_INITIALIZER( 1 );

ompt_wait_id_t mpcomp_OMPT_gen_wait_id( void )
{
	const int prev = OPA_fetch_and_incr_int( &( mpcomp_OMPT_wait_id ) );
	return ( ompt_wait_id_t ) prev;
}

ompt_task_id_t mpcomp_OMPT_gen_task_id( void )
{
	const int prev = OPA_fetch_and_incr_int( &( mpcomp_OMPT_task_id ) );
	return ( ompt_task_id_t ) prev;
}

ompt_thread_id_t mpcomp_OMPT_gen_thread_id( void )
{
	const int prev = OPA_fetch_and_incr_int( &( mpcomp_OMPT_thread_id ) );
	return ( ompt_thread_id_t ) prev;
}

ompt_parallel_id_t mpcomp_OMPT_gen_parallel_id( void )
{
	const int prev = OPA_fetch_and_incr_int( &( mpcomp_OMPT_thread_id ) );
	return ( ompt_thread_id_t ) prev;
}

ompt_state_t ompt_get_state_internal( __UNUSED__ ompt_wait_id_t *wait_id )
{
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	sctk_assert( t != NULL );

	if ( t )
	{
		return t->state;
	}
	else
	{
		return ompt_state_undefined;
	}
}

ompt_frame_t *ompt_get_task_frame( int ancestor_level )
{
	int i;
	mpcomp_thread_t *t;
	mpcomp_mvp_t * __UNUSED__  mvp;
	//mpc_ompt_team_t *team;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	mvp = t->mvp;
	sctk_assert( mvp != NULL );

	//team = mvp->mpc_ompt_thread.team;

	for ( i = 0; i < ancestor_level; i++ )
	{
		//team = team->parent;
	}

	return NULL;
}


static inline void __mpcomp_ompt_task_create( __UNUSED__  mpcomp_task_t* new_task, __UNUSED__  ompt_task_flag_t type, __UNUSED__  bool has_dependence)
{
#if OMPT_SUPPORT
	ompt_frame_t* parent_frame;
	ompt_data_t* new_task_data, *parent_task_data;
	
	ompt_task_create_callback_t callback;

	if( !OMPT_enabled() )  
		return;                                                                                         
	
	if( ompt_get_callback(ompt_event_task_create, &(callback) ))
	{
		new_task_data = &( new_task->ompt_thread_info.task_data );
		parent_task_data = &( new_task->parent->ompt_thread_info.task_data );
		const void* code_ra = __builtin_return_address( 1 );		
		callback( parent_task_data, parent_frame, new_task_data, type, has_dependence, code_ra );
	}
#endif /* OMPT_SUPPORT */ 
}

static inline void __mpcomp_ompt_task_schedule(__UNUSED__  mpcomp_task_t* prior_task, __UNUSED__ mpcomp_task_t* next_task, __UNUSED__ bool prior_completed )
{
#if OMPT_SUPPORT
	ompt_data_t* prior_task_data, *next_task_data;
	ompt_task_schedule_callback_t callback;

	if( !OMPT_enabled() )  
		return;                                                                                         
	
	if( ompt_get_callback(ompt_event_task_schedule, &(callback) ))
	{
		prior_task_data = &( prior_task->ompt_thread_info.task_data );
		next_task_data = &( next_task->ompt_thread_info.task_data );
		callback( prior_task_data, prior_completed, next_task_data );	
	}
#endif /* OMPT_SUPPORT */ 
}

static inline void __mpcomp_ompt_task_implicite_begin(__UNUSED__  uint32_t thread_num,__UNUSED__  mpcomp_task_t* task )
{
#if OMPT_SUPPORT
	mpcomp_thread_t* thread;
	ompt_data_t* parallel_data, *task_data;
	ompt_event_scoped_implicit_callback_t callback;

	if( !OMPT_enabled() )  
		return;                                                                                         
	
	if( ompt_get_callback(ompt_event_implicit_task, &(callback) ))
	{
  		/* Grab info on the current thread */
  		thread = mpcomp_get_thread_tls();
		task_data = &( task->ompt_thread_info.task_data );
		parallel_data = &( thread->instance.team.ompt_team_info.parallel_data );
		callback( ompt_scoped_begin, parallel_data, task_data, thread_num ); 
	}
#endif /* OMPT_SUPPORT */ 
}

static inline void __mpcomp_ompt_task_implicite_end( __UNUSED__ uint32_t thread_num, __UNUSED__ mpcomp_task_t* task )
{
#if OMPT_SUPPORT
	mpcomp_thread_t* thread;
	ompt_data_t* parallel_data, *task_data;
	ompt_event_scoped_implicit_callback_t callback;

	if( !OMPT_enabled() )  
		return;                                                                                         
	
	if( ompt_get_callback(ompt_event_implicit_task, &(callback) ))
	{
  		/* Grab info on the current thread */
  		thread = mpcomp_get_thread_tls();
		task_data = &( task->ompt_thread_info.task_data );
		parallel_data = &( thread->instance.team.ompt_team_info.parallel_data );
		callback( ompt_scoped_end, parallel_data, task_data, thread_num ); 
	}
#endif /* OMPT_SUPPORT */ 
}

#if 0
#if OMPT_SUPPORT
mpc_ompt_task_t ompt_get_task( mpc_ompt_taskteam_t *ompt_team )
{
	return ompt_team->mpc_ompt_task;
}

ompt_frame_t ompt_get_task_frame_internal( int depth )
{
	mpc_ompt_taskteam_t *ompt_team;
	mpc_ompt_task_t ompt_task;

	//ompt_team = ompt_get_team(depth);

	if ( ompt_team == NULL )
	{
		return;
	}

	//ompt_task = ompt_get_task(&ompt_team);
	//return ompt_task.frame;
	return;
}

ompt_task_id_t ompt_get_task_id_internal( int depth )
{
	mpc_ompt_taskteam_t *ompt_team;
	mpc_ompt_task_t ompt_task;
	//ompt_team = ompt_get_team(depth);
	//ompt_task = ompt_get_task(ompt_team);
	return ompt_task.task_id;
}

ompt_task_id_t ompt_get_parent_task_id( mpcomp_thread_t *t )
{
	mpc_ompt_taskteam_t *team;
	team = t->mpc_ompt_thread.taskteam;

	if ( team == NULL )
	{
		//fprintf(stderr,"ompt_get_parent_task_id: team NULL (@ %p), t @ %p\n", team, t);
		return;
	}

	return t->mpc_ompt_thread.taskteam->parent->mpc_ompt_task.task_id;
}

ompt_frame_t ompt_get_parent_task_frame( mpcomp_thread_t *t )
{
	return;
}

void ompt_taskteam_init( mpc_ompt_taskteam_t *taskteam, ompt_parallel_id_t parallel_id, ompt_task_id_t task_id )
{
	taskteam->parallel_id =  parallel_id;
	taskteam->mpc_ompt_task.task_id = task_id;
	taskteam->mpc_ompt_task.frame.reenter_runtime_frame = 0;
	taskteam->mpc_ompt_task.frame.exit_runtime_frame = 0;
	taskteam->parent = 0;
}

void ompt_push_taskteam( mpc_ompt_taskteam_t *taskteam, mpcomp_thread_t *t )
{
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	sctk_assert( t != NULL );

	if ( t->mpc_ompt_thread.taskteam == NULL )
	{
		return;
	}

	mpc_ompt_taskteam_t *team_parent = t->mpc_ompt_thread.taskteam;
	taskteam->parent = team_parent;
	t->mpc_ompt_thread.taskteam = taskteam;

	if ( t->mpc_ompt_thread.taskteam == NULL )
	{
		return;
	}

	//fprintf(stderr, "[TEST] ompt_push_taskteam: task_id = %d\n", t->mpc_ompt_thread.taskteam->parent->mpc_ompt_task.task_id);
}

void ompt_pull_taskteam( mpc_ompt_taskteam_t *taskteam, mpcomp_thread_t *t )
{
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	t->mpc_ompt_thread.taskteam = taskteam->parent;
}

mpc_ompt_team_t *ompt_get_team( int ancestor_level )
{
	int i;
	mpcomp_thread_t *t;
	mpcomp_mvp_t *mvp;
	mpc_ompt_team_t *team;
	fprintf( stderr, "ompt_get_team: ancestor_level = %d\n", ancestor_level );
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	//mvp = t->mvp;
	sctk_assert( mvp != NULL );

	//team = mvp->mpc_ompt_thread.team;

	for ( i = 0; i < ancestor_level; i++ )
	{
		//fprintf(stderr, "ompt_get_team: getting team parent\n");
		team = team->parent;
	}

	return team;
}

void ompt_parallel_region_init()
{
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	mpc_ompt_parallel_t ompt_parallel = t->mpc_ompt_thread.ompt_parallel_info;

	if ( ompt_parallel != NULL )
	{
		return;
	}
	else
	{
		t->mpc_ompt_thread.ompt_parallel_info.parallel_id = OMPT_gen_parallel_id();
	}
}
#endif


/** Mandatory Events ( page 6, TR 2016 ) */

static inline void __mpcomp_ompt_parallel_regions_begin( unsigned int threads_num, ompt_invoker_t invoker)
{
#if OMPT_SUPPORT
#endif /* OMPT_SUPPORT */ 
}

static inline void __mpcomp_ompt_parallel_regions_end(ompt_invoker_t invoker)
{
#if OMPT_SUPPORT
	mpcomp_task_t* task;
	mpcomp_thread_t *thread;
	ompt_data_t* parallel_data, *parent_task_data;
	ompt_parallel_end_callback_t callback;

	if( !OMPT_enabled() )  
		return;                                                                                         
	
	if( ompt_get_callback(ompt_event_parallel_begin, &(callback) ))
	{
  		/* Grab info on the current thread */
  		thread = mpcomp_get_thread_tls();
		task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
		/* Compute callback arguments */ 	
		const void* code_ra = __builtin_return_address( 1 );
		task_data = &( task->ompt_thread_info.task_data );
		parallel_data = &( thread->instance.team.ompt_team_info.parallel_data );
		callback( parallel_data, task_data, invoker, code_ra );
	}	
#endif /* OMPT_SUPPORT */ 
}




#endif


#ifdef COMPILE_FILE_FOR_TEST
int main(int argc, char* argv[])
{
	const char *name;

	printf("ENUMERATE STATE:\n");
	ompt_enumerate_states_t states_enumerate_fn = NULL;
	states_enumerate_fn = (ompt_enumerate_states_t) ompt_fn_lookup("ompt_enumerate_states");
	assert( states_enumerate_fn );

	int state = ompt_state_undefined;
	while(states_enumerate_fn( state, &state, &name))
	{
		printf("%04d: %s\n", state, name);
	}

	printf("---------------------------\n");

	printf("ENUMERATE MUTEX:\n");
	ompt_enumerate_mutex_impls_t mutex_impl_enumerate_fn = NULL;
	mutex_impl_enumerate_fn = (ompt_enumerate_mutex_impls_t) ompt_fn_lookup("ompt_enumerate_mutex_impls");
	assert(mutex_impl_enumerate_fn);

	int mutex_impl = ompt_mutex_impl_unknown;	
	while( mutex_impl_enumerate_fn( mutex_impl, &mutex_impl, &name ) )
	{
		printf("%04d: %s\n", mutex_impl, name);
	}
	
	printf("---------------------------\n");

	printf("SET CALLBACK:\n");
	ompt_set_result_t status  = ompt_set_error;
	ompt_callback_t cb = (ompt_callback_t) 0x1234;
	ompt_callback_set_t callback_set_fn = NULL;		
	callback_set_fn = (ompt_callback_set_t) ompt_fn_lookup( "ompt_set_callback" );
	assert( callback_set_fn );	
	status = callback_set_fn( ompt_callback_parallel_begin, (ompt_callback_t) cb); 	
	assert( status == ompt_set_always ); 
	printf("PASSED !\n");

	printf("GET CALLBACK:\n");
	int retval;
	ompt_callback_t register_cb;
	ompt_callback_get_t callback_get_fn = NULL;
	callback_get_fn = (ompt_callback_get_t) ompt_fn_lookup( "ompt_get_callback" );
	assert( callback_get_fn );	
	retval = callback_get_fn( ompt_callback_parallel_begin, &register_cb); 	
	assert( retval == 1 ); 
	assert( (char*) register_cb == (char*) cb );
	printf("PASSED !\n");
	
	return 0;
}
#endif /* COMPILE_FILE_FOR_TEST */
