#ifndef __MPCOMP_OMPT_PARALLEL_REGIONS_H__
#define __MPCOMP_OMPT_PARELLEL_REGIONS_H__

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
	
#endif /* __MPCOMP_OMPT_PARELLEL_REGIONS_H__ */
