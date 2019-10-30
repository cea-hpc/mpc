#include <stdio.h>
#include "ompt.h"

#include "mpcomp_ompt_types_misc.h"

#if 0
OMPT_API ompt_data_t *ompt_get_thread_data( void )
{
#if SUPPORT_OMPT
	mpcomp_thread_t* thread = NULL;
	
	if( !sctk_openmp_thread_tls )
		return NULL;
	
 	thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
	return thread->ompt_thread_info.thread_data;
#else /* SUPPORT_OMPT */
	return NULL;
#endif /* SUPPORT_OMPT */
}

#if 0
OMPT_API omp_state_t ompt_get_state( ompt_wait_id_t* wait_id )
{
#if SUPPORT_OMPT
	mpcomp_thread_t* thread = NULL;
	
	if( !sctk_openmp_thread_tls )
		return omp_state_undefined;
	
 	thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
	//TODO check if wait_id is NULL if no wait state
	if( thread->ompt_thread_info.wait_id )
		*wait_id = thread->ompt_thread_info.wait_id;

	return thread->ompt_thread_info.state;	
#else /* SUPPORT_OMPT */
	return 0;
#endif /* SUPPORT_OMPT */
}
#endif

OMPT_API _Bool ompt_get_parallel_info( int ancestor_level, ompt_data_t **parallel_data, int *team_size )
{
#if SUPPORT_OMPT
	mpcomp_thread_t* thread = NULL;

	/** Not in parallel **/
	if( !sctk_openmp_thread_tls )
	{
		*parallel_data = NULL;
		return 0;
	}

	thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
	const int current_depth = thread->instance.team.depth;		

	/** No paralle at this level **/
	if( ancestor_level > current_depth )
	{
		*parallel_data = NULL;
		return 0; 
	}
#else /* SUPPORT_OMPT */
	return 0;
#endif /* SUPPORT_OMPT */
}

OMPT_API _Bool ompt_get_task_info( int ancestor_level, ompt_task_type_t *type, ompt_data_t** task_data, ompt_frame_t **task_frame, ompt_data_t **parallel_data, uint32_t *thread_num )
{
#if SUPPORT_OMPT

#else /* SUPPORT_OMPT */
	return 0;
#endif /* SUPPORT_OMPT */
}
#endif 
