#ifndef __MPCOMP_OMPT_BARRIER_H__
#define __MPCOMP_OMPT_BARRIER_H__

static inline void __mpcomp_ompt_barrier_begin( bool wait )
{
#if OMPT_SUPPORT
	ompt_event_t event;
	mpcomp_task_t* task;
	mpcomp_thread_t *thread;
	ompt_sync_region_kind_t kind;
	ompt_data_t* parallel_data, *task_data;
	ompt_scoped_sync_region_callback_t barrier_cb;

	if( !OMPT_enabled() )  
		return;                                                                                         

	event = ( wait ) ? ompt_scoped_sync_wait_region : ompt_scoped_sync_region;
	
	if( ompt_get_callback(event, &(barrier_cb) ))
	{
  		/* Grab info on the current thread */
  		thread = mpcomp_get_thread_tls();
		task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
		/* Compute callback arguments */
		const void* code_ra = __builtin_return_address( 1 );
		kind = ompt_sync_region_barrier;
		parallel_data = &( thread->instance.team.ompt_team_info.parallel_data );
		task_data = &( task->ompt_task_info.task_data );
		barrier_cb( kind, ompt_scope_begin, parallel_data, task_data, code_ra ); 
	}
#endif /* OMPT_SUPPORT */
}

static inline void __mpcomp_ompt_barrier_end( bool wait )
{
#if OMPT_SUPPORT
	ompt_event_t event;
	mpcomp_task_t* task;
	mpcomp_thread_t *thread;
	ompt_sync_region_kind_t kind;
	ompt_data_t* parallel_data, *task_data;
	ompt_scoped_sync_region_callback_t barrier_cb;

	if( !OMPT_enabled() )  
		return;                                                                                         


	event = ( wait ) ? ompt_scoped_sync_wait_region : ompt_scoped_sync_region;

	if( ompt_get_callback(ompt_scoped_sync_region, &(barrier_cb) ))
	{
  		/* Grab info on the current thread */
  		thread = mpcomp_get_thread_tls();
		task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
		/* Compute callback arguments */
		kind = ompt_sync_region_barrier;
		parallel_data = &( thread->instance.team.ompt_team_info.parallel_data );
		task_data = &( task->ompt_task_info.task_data );
		barrier_cb( kind, ompt_scope_end, parallel_data, task_data, NULL ); 
	}
#endif /* OMPT_SUPPORT */
}

#endif /*  __MPCOMP_OMPT_BARRIER_H__ */
