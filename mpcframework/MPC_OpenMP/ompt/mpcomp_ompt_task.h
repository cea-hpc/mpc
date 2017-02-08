#ifndef __MPCOMP_OMPT_TASK_H__
#define __MPCOMP_OMPT_TASK_H__

static inline void __mpcomp_ompt_task_create( mpcomp_task_t* new_task, ompt_task_type_t type, bool has_dependence)
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

static inline void __mpcomp_ompt_task_schedule( mpcomp_task_t* prior_task, mpcomp_task_t* next_task, bool prior_completed )
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

static inline void __mpcomp_ompt_task_implicite_begin( uint32_t thread_num, mpcomp_task_t* task )
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

static inline void __mpcomp_ompt_task_implicite_end( uint32_t thread_num, mpcomp_task_t* task )
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

#endif /* __MPCOMP_OMPT_TASK_H__*/
