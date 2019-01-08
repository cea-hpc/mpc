#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ompt.h"
#include "mpcomp_abi.h"
#include "mpcomp_types.h"
#include "mpcomp_ompt_init.h"
#include "mpcomp_openmp_tls.h"
#include "mpcomp_ompt_entry_point.h"
#include "mpcomp_ompt_types_misc.h"
#include "mpcomp_ompt_macro_inquiry.h"

/**
 * 	macros
 *  	Non standard / LLVM COMPLIANCE
 */

#define ompt_get_callback_success 1
#define ompt_get_callback_failure 0 

#define no_tool_present 0

#define OMPT_API_ROUTINE static

#ifndef OMPT_STR_MATCH
// compare two string ignoring case
#define OMPT_STR_MATCH( string0, string1 ) \
	!strcasecmp( string0, string1 )
#endif /* OMPT_STR_MATCH */

/**
 *	types
 * Non standard / LLVM COMPLIANCE or MPC specific 
 */

typedef struct ompt_enumerate_info_s {
	const char* name;
	unsigned long id;
} ompt_enumerate_info_t;

typedef struct ompt_callback_info_s
{
	const char *name;
	ompt_callbacks_t id;
	ompt_set_result_t status;					
} ompt_callback_info_t;

typedef enum tool_setting_e {
	omp_tool_error,
	omp_tool_unset,
	omp_tool_disabled,
	omp_tool_enabled
} tool_setting_t;

typedef int (*mpcomp_ompt_initialize_t)
(
   ompt_function_lookup_t lookup, 
   struct ompt_fns_t* fns
);

typedef void (*mpcomp_ompt_finalize_t)
(
   struct ompt_fns_t* fns
);

/**** 
 * global variables
 */

static ompt_enumerate_info_t ompt_state_info[] = {
#define ompt_state_macro(state, code, desc) { # state, code },
   FOREACH_OMPT_STATE(ompt_state_macro)
#undef ompt_state_macro
};

static ompt_enumerate_info_t ompt_mutex_impls_info[] = {
#define ompt_mutex_impl_macro(mutex_impl, code, desc) { # mutex_impl, mutex_impl },
   FOREACH_OMPT_MUTEX_IMPL(ompt_mutex_impl_macro)
#undef ompt_mutex_impl_macro
};

static ompt_callback_info_t ompt_callback_info[] = {
#define ompt_callback_macro(callback, code, status) { # callback, code, status },
	FOREACH_OMPT_CALLBACK(ompt_callback_macro)
#undef ompt_callback_macro
};

static ompt_enumerate_info_t ompt_set_result_info[] = {
#define ompt_set_result_macro(name, code)	{ # name, name },
	 FOREACH_OMPT_SET_RESULT(ompt_set_result_macro)
#undef ompt_set_result_macro
};

//TODO: should be task specific value
static int mpcomp_ompt_enabled = 0;

//TODO: should be task specific value
ompt_callback_t *OMPT_Callbacks = NULL;

//TODO: should be task specific value
static ompt_fns_t* mpcomp_ompt_fns = NULL; 

const ompt_data_t ompt_data_none = (ompt_data_t) {.value=0};

/**
 * Forward declaration 
 */

static ompt_interface_fn_t ompt_fn_lookup( const char* name );

int mpcomp_ompt_is_enabled(void)
{
	return mpcomp_ompt_enabled;
}

ompt_callback_t mpcomp_ompt_get_callback(int callback_type)
{
	return OMPT_Callbacks[callback_type];
}
/*
 * Initialisation / Finalization
 */

#pragma weak ompt_start_tool
ompt_fns_t* ompt_start_tool( __UNUSED__ unsigned int omp_version, __UNUSED__ const char *runtime_version )
{
   sctk_info("No ompt_start_tool function from tool");
   return NULL;
}

void mpcomp_ompt_pre_init( void )
{
	//TODO: Should be task specific ONCE
	static int ompt_pre_init_once_processus = 0;

	if( ompt_pre_init_once_processus )
	{
		return;
	}

	// Parse OMP_TOOL env variable
	const char* ompt_env_var = getenv("OMP_TOOL");
	tool_setting_t tool_setting = omp_tool_error;

	if( !ompt_env_var || !strcmp(ompt_env_var, ""))
	{
		tool_setting = omp_tool_unset;
	}	
	else if( OMPT_STR_MATCH(ompt_env_var,"disabled") ) 
	{
		tool_setting = omp_tool_disabled;	
	}
	else if( OMPT_STR_MATCH(ompt_env_var,"enabled") )
	{
		tool_setting = omp_tool_enabled;
	}
	else
	{
		// Unexpected value
		tool_setting = omp_tool_error;
	}

	switch(tool_setting) 
	{
		/* OMPT DISABLED */
		case omp_tool_error:
			fprintf(stderr, "Warning: OMPT_TOOL has invalid value");		
		case omp_tool_disabled:
			break;

		/* OMPT ENABLED */
		case omp_tool_unset:
		case omp_tool_enabled:
			mpcomp_ompt_fns = ompt_start_tool(_OPENMP, "mpc-omp");
			if( mpcomp_ompt_fns )
			{
				sctk_error("OMPT is ENABLED ...");
				mpcomp_ompt_enabled = 1;
			} 	
	}
}

void mpcomp_ompt_post_init(void)
{
	if( mpcomp_ompt_enabled && mpcomp_ompt_fns )
		mpcomp_ompt_fns->initialize(ompt_fn_lookup, mpcomp_ompt_fns);
}

/***
 * Enumerate STATE
 */ 

OMPT_API_ROUTINE int 
ompt_enumerate_states(int cur_state, int* next_state, const char** next_state_name)
{
	int i;
	static const int len = sizeof(ompt_state_info)/sizeof(ompt_enumerate_info_t);
	assert( len > 0 );

	if( cur_state == ompt_state_undefined )
	{
		i = -1; /* next state -> i = 0 */
	}
	else
	{
		/* Find current state in ompt_state_info tabular */
		for( i = 0; i < len - 1; i++) 
		{
			if( ompt_state_info[i].id != cur_state)
				continue;
			break;
		}
	}

	/* get next value */
	i++;

	if( i < len - 1 )	/* Found */
	{
		*next_state = ompt_state_info[i].id;
		*next_state_name = ompt_state_info[i].name;
	}

	return ( i < len - 1 ) ? 1 : 0;
}

/***
 * Enumerate MUTEX 
 */ 

OMPT_API_ROUTINE int 
ompt_enumerate_mutex_impls( int current_mutex, int* next_mutex, const char** next_name)
{
	uint64_t i;
	static const uint64_t len = sizeof(ompt_mutex_impls_info)/sizeof(ompt_enumerate_info_t);
	assert( len > 0 );

	/* Find current state in ompt_state_info tabular */
	for( i = 0; i < len - 1; i++) 
	{
		if( ompt_mutex_impls_info[i].id != current_mutex )
			continue;
		break;
	}

	if( i < len - 1 )	/* Found */
	{
		*next_mutex = ompt_mutex_impls_info[i+1].id;
		*next_name = ompt_mutex_impls_info[i+1].name;
	}

	return ( i < len - 1 ) ? 1 : 0;
}

/****
 * Set/Get Callback
 */
#if 0
static int mpcomp_ompt_set_result_to_str( ompt_set_result_t status, const char **string )
{
	uint64_t i;
	const uint64_t len = sizeof(ompt_set_result_info) / sizeof(ompt_enumerate_info_t);
	assert( len > 0 );

	for( i = 0; i < len -1; i++ )
	{
		if( ompt_set_result_info[i].id != status )
			continue;
		break;	
	}

	if( i < len -1 )
	{
		*string =  ompt_set_result_info[i].name;
	}

	return ( i < len -1 ) ? 1 : 0;
}


static int mpcomp_ompt_callback_to_str( ompt_callbacks_t callback_type, const char ** string )
{
	uint64_t i;
	const uint64_t len = sizeof(ompt_callback_info) / sizeof(ompt_callback_info_t);
	assert( len > 0 );

	for( i = 0; i < len -1; i++ )
	{
		if( ompt_callback_info[i].id != callback_type )
			continue;
		break;	
	}

	if( i < len -1 )
	{
		*string =  ompt_callback_info[i].name;
	}

	return ( i < len -1 ) ? 1 : 0;
}
#endif

static ompt_set_result_t mpcomp_ompt_get_callback_status( ompt_callbacks_t callback_type )
{
	uint64_t i;
	const uint64_t len = sizeof(ompt_callback_info) / sizeof(ompt_callback_info_t);	
	assert( len > 0 );

	ompt_set_result_t retval = ompt_set_error;	

	for( i = 0; i < len - 1; i++ )
	{
		if( ompt_callback_info[i].id != callback_type )
			continue;
		retval = ompt_callback_info[i].status;
		break;
	}

	return retval; 
}

OMPT_API_ROUTINE
int ompt_set_callback(ompt_callbacks_t callback_type, ompt_callback_t callback)
{
	ompt_set_result_t status;
	// callbacks begin with value 1 
	const uint64_t len = sizeof(ompt_callback_info) / sizeof(ompt_callback_info_t) + 1;
	assert( callback_type > 0 && callback_type < len );	

	status = mpcomp_ompt_get_callback_status( callback_type );
	if( status != ompt_set_error )
	{
		if( !OMPT_Callbacks )
		{
			OMPT_Callbacks = (ompt_callback_t*)malloc( len* sizeof(ompt_callback_t) );
			assert( OMPT_Callbacks );
			memset( OMPT_Callbacks, 0, len* sizeof(ompt_callback_t) );
		}
		OMPT_Callbacks[callback_type] = callback;
	}
	return status;
}

OMPT_API_ROUTINE int 
ompt_get_callback( ompt_callbacks_t callback_type, ompt_callback_t* callback )
{
	ompt_set_result_t status;

	int retval = ompt_get_callback_failure;
	status = mpcomp_ompt_get_callback_status( callback_type );
	

	if( status != ompt_set_error && OMPT_Callbacks )
	{
		*callback = OMPT_Callbacks[callback_type];
		retval = ompt_get_callback_success;
	} 
	
	return retval; 
}

OMPT_API_ROUTINE ompt_data_t* 
ompt_get_thread_data(void)
{
	mpcomp_thread_t *thread_infos = mpcomp_get_thread_tls();
	return ( thread_infos ) ? &(thread_infos->ompt_thread_data) : NULL; 
}

OMPT_API_ROUTINE ompt_state_t
ompt_get_state( __UNUSED__ ompt_wait_id_t* wait_id)
{
	mpcomp_thread_t *thread_infos = mpcomp_get_thread_tls();
	return ( thread_infos ) ? thread_infos->state : ompt_state_undefined; 
} 

OMPT_API_ROUTINE int
ompt_get_parallel_info( int ancestor_level, ompt_data_t** parallel_data, int* team_size)
{
	mpcomp_thread_t *thread_infos = mpcomp_get_thread_tls();

	if( ancestor_level != 0 ) // NO NESTED REGION
	{
		*parallel_data = NULL;
		*team_size = 0;
		return 0;
	}

	*parallel_data = (thread_infos) ? &(thread_infos->info.ompt_region_data) : NULL;
	*team_size = (thread_infos) ? thread_infos->info.num_threads : 0;
	return 1;
} 

OMPT_API_ROUTINE int
ompt_get_task_info( 	int ancestor_level, 
							ompt_task_type_t *type, 
							ompt_data_t **task_data, 
							ompt_frame_t **task_frame, 
							ompt_data_t **parallel_data,
							int *thread_num )
{
#if MPCOMP_TASK
	int i;
	mpcomp_task_t* current_task = NULL;
	mpcomp_thread_t *thread_infos = mpcomp_get_thread_tls();
	
	if( !thread_infos ) return 0;

	current_task = thread_infos->task_infos.current_task;
	
	for( i = 0; i < ancestor_level; i++)
	{
		current_task = current_task->parent;
		if( current_task == NULL ) break;
	} 

	if( !current_task )
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
		*task_data = &(current_task->ompt_task_data);
		*task_frame = &(current_task->ompt_task_frame);
		if( !ompt_get_parallel_info( 0, parallel_data, thread_num))
		{
			*parallel_data = NULL;
			*thread_num = 0;
		}
	}
	
	return (current_task) ? 1 : 0;
#endif /* MPCOMP_TASK */
    return 0;
}

static ompt_interface_fn_t ompt_fn_lookup(const char* name)
{
#define ompt_interface_fn(fn) 												\
	if(strcmp(name, #fn ) == 0) return (ompt_interface_fn_t) fn;
	FOREACH_OMPT_INQUIRY_FN(ompt_interface_fn)
#undef ompt_interface_fn
	return (ompt_interface_fn_t) 0;
}

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
