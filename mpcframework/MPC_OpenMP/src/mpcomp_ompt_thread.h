#ifndef __MPCOMP_OMPT_THREAD_H__
#define __MPCOMP_OMPT_THREAD_H__

static inline void __mpcomp_ompt_thread_begin( ompt_thread_type_t type )
{
#if OMPT_SUPPORT
	mpcomp_thread_t *thread;
	ompt_data_t* thread_data;
	ompt_thread_begin_callback_t callback;

	if( !OMPT_enabled() )  
		return;                                                                                         
	
	if( ompt_get_callback(ompt_event_thread_begin, &(callback) ))
	{
  		/* Grab info on the current thread */
  		thread = mpcomp_get_thread_tls();
		/* Compute callback arguments */ 	
		thread_data = &( thead->ompt_thread_info.thread_data );
		callback( type, thread_data );
	}	
	
#endif /* OMPT_SUPPORT */ 
}

static inline void __mpcomp_ompt_thread_end( void )
{
#if OMPT_SUPPORT
	mpcomp_thread_t *thread;
	ompt_data_t* thread_data;
	ompt_thread_end_callback_t callback;

	if( !OMPT_enabled() )  
		return;                                                                                         
	
	if( ompt_get_callback(ompt_event_thread_end, &(callback) ))
	{
  		/* Grab info on the current thread */
  		thread = mpcomp_get_thread_tls();
		/* Compute callback arguments */ 	
		thread_data = &( thead->ompt_thread_info.thread_data );
		callback( thread_data );
	}
#endif /* OMPT_SUPPORT */ 
} 

#endif /* __MPCOMP_OMPT_THREAD_H__ */

