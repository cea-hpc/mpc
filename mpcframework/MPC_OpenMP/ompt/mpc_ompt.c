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
/* #   - BESNARD Jean-Baptiste jbe@prism.uvsq.fr                          # */
/* #                                                                      # */
/* ######################################################################## */

#if 0
/*###########################################################################
 *  INTERNALS
 *##########################################################################*/

/*###########################################################################
 *  Identifiers Handling and inquiry
 *##########################################################################*/

ompt_thread_id_t __ompt_thread_id_new() /* Need to generate a thread ID ? */
{
 return OMPT_gen_thread_id();
}

ompt_parallel_id_t __ompt_parallel_id_new() /* Generate a parallel ID */
{
 return OMPT_gen_parallel_id();
}

ompt_task_id_t __ompt_task_id_new() /* Generate a task ID */
{
 return OMPT_gen_task_id();
}

/*###########################################################################
 * Callbacks
 *##########################################################################*/

 /* IMPORTANT: For now, OMPT_Callbacks is shared between MPI tasks 
 * works with TAU
 * this design should be improved to declare one table OMPT_Callbacks per MPI task
 * */
 ompt_callback_t OMPT_Callbacks[ompt_event_count];

void OMPT_callbacks_clear()
{
    //printf("OMPT_callbacks_clear: ompt_event_count = %d, size of OMPT_Callbacks = %d\n", ompt_event_count, sizeof(OMPT_Callbacks));

	memset( OMPT_Callbacks, 0, ompt_event_count * sizeof( ompt_callback_t ) );
}


/*###########################################################################
 *  Intialization
 *##########################################################################*/

int ____MPC_OMPT_ENABLED = 0;


#define OMPT_TO_STRING(a) #a
#define __OMPT_OMP_VERSION( mi, ma ) mpc OpenMP rev- ## mi ## . ## ma
#define OMPT_OMP_VERSION OMPT_TO_STRING( __OMPT_OMP_VERSION( SCTK_OMP_VERSION_MAJOR, SCTK_OMP_VERSION_MINOR ) )

ompt_set_callback_return_codes OMPT_Event_support[ompt_event_count];

typedef enum tool_setting_e {
  omp_tool_error = 0,
  omp_tool_unset = 1,
  omp_tool_disabled = 2,
  omp_tool_enabled = 3
}tool_setting_t;

void OMPT_init()
{
 char *ompt_env_var = getenv("OMP_TOOL");

 tool_setting_t tool_setting = omp_tool_error;

 if (!ompt_env_var  || !strcmp(ompt_env_var, ""))
   tool_setting = omp_tool_unset;
 else if (!strcmp(ompt_env_var, "disabled"))
   tool_setting = omp_tool_disabled;
 else if (!strcmp(ompt_env_var, "enabled"))
   tool_setting = omp_tool_enabled;

 switch(tool_setting)
 {
       case omp_tool_unset:
       case omp_tool_enabled:
       {
#define OMPT_EVENT(a, b, c, d, e) OMPT_Event_support[c] = d;
#include "ompt-events.h"
#undef OMPT_EVENT

 
         /* Clear callback array 
 	*
 	* Leads to bugs with several MPI tasks
 	* Callback array should be cleared only once
 	* and not at each OMPT initialization
 	* 
	 * */
         //OMPT_callbacks_clear(); 

	//int ret = ompt_initialize( ompt_function_lookup, OMPT_OMP_VERSION, 1 );
	//ompt_initialize( ompt_function_lookup, OMPT_OMP_VERSION, 1 );
        ompt_initialize_t ompt_initialize_fn = ompt_tool();

        //ompt_initialize_t ompt_initialize_fn;
        if (ompt_initialize_fn) {
          fprintf(stdout, "OMPT_init: ompt_tool SUCCESS\n");
          ompt_initialize_fn(ompt_function_lookup, OMPT_OMP_VERSION, 1);
          ____MPC_OMPT_ENABLED = 1;
        } else {
          fprintf(stdout, "OMPT_init: ompt_tool FAILURE\n"); 
	}

	/* Set OMPT state */
        //____MPC_OMPT_ENABLED = ret;

       }

 }
}

void OMPT_release()
{
	OMPT_callbacks_hit( ompt_event_runtime_shutdown, 0, 0, NULL );
	
	____MPC_OMPT_ENABLED = 0;
}

/*###########################################################################
 *  INTERFACE
 *##########################################################################*/

/*###########################################################################
 *  Identifiers Handling and inquiry
 *  (OMPT norm p. 15)
 *##########################################################################*/

ompt_thread_id_t ompt_get_thread_id(void)
{
 mpcomp_thread_t *t;
 mpcomp_mvp_t *mvp;
 ompt_thread_id_t thread_id;
 
 t = (mpcomp_thread_t *) sctk_openmp_thread_tls;

 sctk_assert( t != NULL);	

 mvp = t->mvp;

 //thread_id = mvp->mpc_ompt_thread.thread_id;
 
 return thread_id;
}

ompt_parallel_id_t  ompt_get_parallel_id( int ancestor_level )
{
	
	
}

int ompt_get_parallel_team_size( int ancestor_level) 
{


}

ompt_task_id_t  ompt_get_task_id( int ancestor_level )
{	
 //return ompt_get_task_id_internal(ancestor_level);	
 return NULL;
}

/*###########################################################################
 *  State handling
 *  (OMPT norm p. 25)
 *##########################################################################*/

typedef struct {
  char *state_name;
  ompt_state_t  state_id;
} ompt_state_info_t;


ompt_state_info_t ompt_state_info[] = {
#define OMPT_STATE(state, code, description) { # state, state },
#include "ompt-states.h"
};


ompt_state_t ompt_get_state( ompt_wait_id_t *wait_id )
{	
  ompt_state_t state = ompt_get_state_internal(wait_id);	

  return state;
}

int ompt_enumerate_state( ompt_state_t current_state, ompt_state_t *next_state, const char **next_state_name )
{

#if 1
 const static int len = sizeof(ompt_state_info) / sizeof(ompt_state_info_t);
 int i = 0;

 for(i = 0 ; i < len - 1 ; i++) {
  if(ompt_state_info[i].state_id == current_state) {
   *next_state = ompt_state_info[i+1].state_id;
   *next_state_name = ompt_state_info[i+1].state_name;
   return 1;
  }  
 }
#endif

 return 0; 
 	
}

/*###########################################################################
 *  Runtime Events (Tracing)
 *  (OMPT norm p. 26)
 *##########################################################################*/
 

 
int ompt_set_callback( ompt_event_t event_type, ompt_callback_t callback)
 {

	 /* Handle out of bounds */
	 if( ompt_event_count <= event_type )
	 {
		 return OMPT_CALLBACK_ERROR;
	 }
	 
	 int support_level = OMPT_Event_support[ event_type ];
	 
	 /* Return if missing */
	 if( support_level == OMPT_CALLBACK_MISSING )
		return OMPT_CALLBACK_MISSING;
	 
	 /* Return if no such event */
	 if( support_level == OMPT_CALLBACK_NO_SUCH_EVENT )
		return OMPT_CALLBACK_NO_SUCH_EVENT;	 

	 /* If we are here we can attach */
	 OMPT_Callbacks[ event_type ] = callback;

	 return support_level;
 }
 
 
 int ompt_get_callback( ompt_event_t event_type, ompt_callback_t *callback)
 {
	/* Handle out of bounds */
	 if( ompt_event_count <= event_type )
	 {
		 return 0;
	 }

	 /* No handler set */
	 if( OMPT_Callbacks[ event_type ] == NULL )
	 {
		 return 0;
	 }
	 
	 /* What if we have nowhere to write */
	 if( !callback )
	 {
		 return 1;
	 }
	 
	 /* Write the callback */
	 *callback = OMPT_Callbacks[ event_type ];

	 return 1;
 }


void OMPT_callbacks_hit( ompt_event_t type, uint64_t arg1, uint64_t arg2, void *fct )
{
	/*** TODO ==> HANDLE FRAMES */

	ompt_callback_t cb = OMPT_Callbacks[ type ];
	
	/* Check if there is a CB and that we are enabled */
	if( !cb || !OMPT_enabled() )
	{
        if(!cb)
         printf("OMPT_callbacks_hit: callback uninitialized !!! (cb uninitialized)\n");
        else if(!OMPT_enabled())
         printf("OMPT not enabled\n");
		return;
	}

	/* Ctx variables */
	ompt_task_id_t ompt_parent_task_id = OMPT_UNDEFINED_IDENTIFIER;
	ompt_frame_t * ompt_parent_task_frame = NULL;
	void * function = NULL;
	
	ompt_thread_id_t ompt_thread_id =  OMPT_UNDEFINED_IDENTIFIER;
	ompt_thread_id_t ompt_parallel_id =  OMPT_UNDEFINED_IDENTIFIER;
	ompt_thread_id_t ompt_task_id =  OMPT_UNDEFINED_IDENTIFIER;
	ompt_wait_id_t ompt_wait_id = OMPT_UNDEFINED_IDENTIFIER;

    /* Task switching */
	ompt_task_id_t suspended;                            
	ompt_task_id_t resumed;
	
	/* Control */
	uint64_t command;
	uint64_t modifier;
	

	/* Hit the right CB */
	switch( type )
	{
		/* Thread CB */	
		case ompt_event_thread_begin:
           ((ompt_thread_type_callback_t)cb)(ompt_thread_initial, 1);
        break;
		case ompt_event_thread_end:
		case ompt_event_idle_begin:
		case ompt_event_idle_end:
		case ompt_event_flush:
			/*void (*ompt_thread_callback_t)( ompt_thread_id_t thread_id );*/
		
			ompt_thread_id = ompt_get_thread_id();
			((ompt_thread_callback_t)cb)( ompt_thread_id );
			
		break;
		
		
		/* NEW Parallel CB */
		case ompt_event_parallel_begin:
		case ompt_event_parallel_end:
			/* void (*ompt_new_parallel_callback_t)( ompt_task_id_t parent_task_id,
                                                      ompt_frame_t * parent_task_frame,
                                                      ompt_parallel_id_t parallel_id,
                                                      void *parallel_function); */
			
			ompt_parent_task_id = ompt_get_task_id( 1 );
			ompt_parallel_id = ompt_get_parallel_id( 0 );
			ompt_task_id = ompt_get_task_id( 0 );
			function = fct;
			
			//((ompt_new_parallel_callback_t)cb)( ompt_parent_task_id, ompt_parent_task_frame, 
			//									ompt_parallel_id, function );
            ((ompt_new_parallel_callback_t)cb)(0,NULL,0,1,function);
		break;
			
		/* New task CB */
		case ompt_event_task_begin:
			/*void (*ompt_new_task_callback_t)( ompt_task_id_t parent_task_id,
                                                 ompt_frame_t *parent_task_frame,
                                                 ompt_task_id_t  new_task_id,
                                                 void *task_function ); */
			ompt_parent_task_id = ompt_get_task_id( 1 );
			ompt_task_id = ompt_get_task_id( 0 );
			function = fct;
			
			((ompt_new_task_callback_t)cb)( ompt_parent_task_id, ompt_parent_task_frame, 
												ompt_task_id, function );
		break;
		
		
		/* Task CB */
		case ompt_event_task_end:
			/* void (*ompt_task_callback_t)( ompt_task_id_t task_id ); */
			
			ompt_task_id = ompt_get_task_id( 0 );
			((ompt_task_callback_t)cb)( ompt_task_id );
		break;
		
		/* CB */
		case ompt_event_runtime_shutdown:
			/* void (*ompt_callback_t)(void); */
			
			((ompt_callback_t)cb)();
		break;
		
		/* Workshare CB */
		case ompt_event_loop_begin:
		case ompt_event_loop_end:
		case ompt_event_sections_begin:
		case ompt_event_sections_end:
		case ompt_event_single_in_block_begin:
		case ompt_event_single_in_block_end:
			/* void (*ompt_new_workshare_callback_t)( ompt_task_id_t parent_task_id,
                                                      ompt_parallel_id_t parallel_id,
                                                      void *workshare_function ); */
			ompt_parallel_id = ompt_get_parallel_id( 0 );
			function = fct;
			((ompt_new_workshare_callback_t)cb)( ompt_parent_task_id, ompt_parallel_id, function  );
		
		break;
		
		
		/* Parallel CB */
		case ompt_event_wait_barrier_begin:
		case ompt_event_wait_barrier_end:
		case ompt_event_wait_taskwait_begin:
		case ompt_event_wait_taskwait_end:
		case ompt_event_wait_taskgroup_begin:
		case ompt_event_wait_taskgroup_end:
		case ompt_event_implicit_task_begin:
		case ompt_event_implicit_task_end:
		case ompt_event_initial_task_begin:
		case ompt_event_initial_task_end:
		case ompt_event_single_others_begin:
		case ompt_event_single_others_end:
		case ompt_event_master_begin:
		case ompt_event_master_end:
		case ompt_event_barrier_begin:
		case ompt_event_barrier_end:
		case ompt_event_taskwait_begin:
		case ompt_event_taskwait_end:
		case ompt_event_taskgroup_begin:
		case ompt_event_taskgroup_end:
			/* void (*ompt_parallel_callback_t)( ompt_parallel_id_t parallel_id,
                                                 ompt_task_id_t task_id ); */
			
			ompt_parallel_id = ompt_get_parallel_id( 0 );
			ompt_task_id = ompt_get_task_id( 0 );
			
			((ompt_parallel_callback_t)cb)( ompt_parallel_id, ompt_task_id );
		break;
		
		
		/* Wait CB */
		case ompt_event_release_lock:
		case ompt_event_release_nest_lock_last:
		case ompt_event_release_critical:
		case ompt_event_release_atomic:
		case ompt_event_release_ordered:
		case ompt_event_release_nest_lock_prev:
		case ompt_event_wait_lock:
		case ompt_event_wait_nest_lock:
		case ompt_event_wait_critical:
		case ompt_event_wait_atomic:
		case ompt_event_wait_ordered:
		case ompt_event_acquired_lock:
		case ompt_event_acquired_nest_lock_first:
		case ompt_event_acquired_nest_lock_next:
		case ompt_event_acquired_critical:
		case ompt_event_acquired_atomic:
		case ompt_event_acquired_ordered:
		case ompt_event_init_lock:
		case ompt_event_init_nest_lock:
		case ompt_event_destroy_lock:
		case ompt_event_destroy_nest_lock:
			/* void (*ompt_wait_callback_t)( ompt_wait_id_t wait_id ); */
			
			( (ompt_wait_callback_t)cb)( ompt_wait_id );
		break;
		
		
		/* Task switch */
		case ompt_event_task_switch :
			/* void (*ompt_task_switch_callback_t)( ompt_task_id_t suspended_task_id,
                                                    ompt_task_id_t resumed_task_id ); */

			suspended = arg1;
			resumed = arg2;
		
			( (ompt_task_switch_callback_t)cb)( suspended, resumed );
		break;
		
		/* Control */
		case ompt_event_control:
			/* void (*ompt_control_callback_t) ( uint64_t command, uint64_t modifier ); */
			
			command = arg1;
			modifier = arg2;
			
			( (ompt_control_callback_t)cb)( command, modifier );
		break;
		
	}
}




/*###########################################################################
 *  Frame handling
 *  (OMPT norm p. 16)
 *##########################################################################*/
 
void * ompt_get_idle_frame(void)
{
 mpcomp_thread_t *t;
 mpcomp_mvp_t *mvp;
 void *idle_frame;

 t = (mpcomp_thread_t *) sctk_openmp_thread_tls;

 sctk_assert( t != NULL);
 
 mvp = t->mvp;

 //idle_frame = mvp->mpc_ompt_thread.idle_frame;

 return idle_frame;
}

#if 0
ompt_frame_t * ompt_get_task_frame(int ancestor_level)
{
 //return ompt_get_task_frame_internal(ancestor_level);
 return NULL;
}
#endif

/*###########################################################################
 *  Intialization
 *  (OMPT norm p. 19)
 *##########################################################################*/

ompt_interface_fn_t ompt_function_lookup(const char *func_name)
{
	if( !func_name )
	{
		return NULL;
	}
	
	if( !strcmp( func_name, "ompt_get_thread_id" ) )
	{
		return (ompt_interface_fn_t)ompt_get_thread_id;
	}
	else if( !strcmp( func_name, "ompt_get_parallel_id" ) )
	{
		return (ompt_interface_fn_t)ompt_get_parallel_id;
	}
	else if( !strcmp( func_name, "ompt_get_task_id" ) )
	{
		return (ompt_interface_fn_t)ompt_get_task_id;
	}
	else if( !strcmp( func_name, "ompt_get_state" ) )
	{
		return (ompt_interface_fn_t)ompt_get_state;
	}
	else if( !strcmp( func_name, "ompt_enumerate_state" ) )
	{
		return (ompt_interface_fn_t)ompt_enumerate_state;
	}
	else if( !strcmp( func_name, "ompt_set_callback" ) )
	{
		return (ompt_interface_fn_t)ompt_set_callback;
	}
	else if( !strcmp( func_name, "ompt_get_callback" ) )
	{
		return (ompt_interface_fn_t)ompt_get_callback;
	}
	else if( !strcmp( func_name, "ompt_get_idle_frame" ) )
	{
		return (ompt_interface_fn_t)ompt_get_idle_frame;
	}
	else if( !strcmp( func_name, "ompt_get_task_frame" ) )
	{
		return (ompt_interface_fn_t)ompt_get_task_frame;
	}
	else if( !strcmp( func_name, "ompt_function_lookup" ) )
	{
		return (ompt_interface_fn_t)ompt_function_lookup;
	}	
	else if( !strcmp( func_name, "ompt_control" ) )
	{
		return (ompt_interface_fn_t)ompt_control;
	}
	
	return NULL;
}

//#pragma weak ompt_initialize
//void ompt_initialize( ompt_function_lookup_t ompt_fn_lookup, const char *runtime_version, unsigned int ompt_version )
#pragma weak ompt_tool
ompt_initialize_t ompt_tool()
{
    printf("MPC: runtime side ompt_tool\n"); 
	/* This is the dummy version which returns 0 */
	return 0;
}


/*###########################################################################
 *  Control
 *  (OMPT norm p. 22)
 *##########################################################################*/

void ompt_control( uint64_t command, uint64_t modifier )
{
	/* Handle predefined values */
	OMPT_Control_commands_t c = command;
	switch( c )
	{
		case OMPT_CONTROL_START_MONITORING:
			____MPC_OMPT_ENABLED = 1;
		break;
		case OMPT_CONTROL_PAUSE_MONITORING:
		case OMPT_CONTROL_STOP_MONITORING:
			____MPC_OMPT_ENABLED = 0;
		break;
		case OMPT_CONTROL_FLUSH:
			/* Nothing here yet */
		break;
	}
	
	/* Forward the CB */
	OMPT_callbacks_hit( ompt_event_control, command, modifier, NULL );
	
}
#endif
