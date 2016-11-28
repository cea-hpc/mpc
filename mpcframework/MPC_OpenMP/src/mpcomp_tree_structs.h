#include "mpcomp_types_def.h"

#ifndef __MPCOMP_TREE_STRUCTS_H__
#define __MPCOMP_TREE_STRUCTS_H__

#include "mpcomp_task.h"
#include "mpcomp_types.h"
#include "mpcomp_task_utils.h"

static inline void
mpcomp_thread_infos_reset( mpcomp_thread_t* thread )
{
	sctk_assert( thread );
	memset( thread, 0, sizeof( mpcomp_thread_t  ));
}

static inline void
mpcomp_common_table_reset( struct common_table* th_pri_common )
{
	sctk_assert( th_pri_common );
	memset( th_pri_common, 0, sizeof( struct common_table ));
}

static inline struct common_table*
mpcomp_common_table_allocate( void )
{
	struct common_table* th_pri_common;
	th_pri_common = (struct common_table *) sctk_malloc(sizeof(struct common_table));
	sctk_assert( th_pri_common );

	mpcomp_common_table_reset( th_pri_common );
	return th_pri_common;
}

static inline void
mpcomp_parallel_region_infos_reset( mpcomp_new_parallel_region_info_t * info )
{
	sctk_assert( info );
	memset( info, sizeof( mpcomp_new_parallel_region_info_t ), 0 );
}

static inline void
mpcomp_parallel_region_infos_init( mpcomp_new_parallel_region_info_t * info )
{
	sctk_assert( info );
	mpcomp_parallel_region_infos_reset( info );
	info->combined_pragma = MPCOMP_COMBINED_NONE;
}

static inline void
mpcomp_thread_infos_init( mpcomp_thread_t* thread, mpcomp_local_icv_t icvs,
					     mpcomp_instance_t *instance, mpcomp_thread_t *father )
{
	int i;

	/* RESET all thread_infos to NULL 		*/
	mpcomp_thread_infos_reset( thread );

	/* SET all non null thread info fields	*/
	thread->father = father ;
	thread->info.num_threads = 1;
	thread->info.icvs = icvs;
	thread->instance = instance;

	mpcomp_parallel_region_infos_init( &( thread->info ) ) ;

 	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
 	for( i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN + 1; i++)
	{
   	sctk_atomics_store_int( &( thread->for_dyn_remain[i].i ), -1 );
	}

#if ( defined( MPCOMP_TASK) || defined( MPCOMP_OPENMP_3_0 ) )
	mpcomp_task_thread_infos_init( thread );
#endif /* MPCOMP_TASK || MPCOMP_OPENMP_3_0 */

	/* Intel openmp runtime */
	thread->push_num_threads = -1 ;
	thread->th_pri_common = mpcomp_common_table_allocate();
  	thread->reduction_method = 0;
}

static inline void
mpcomp_team_infos_init( mpcomp_team_t* team )
{
	int i;
	
	sctk_assert( team );	

	/* SET all non null task info fields */
	memset( team,  0, sizeof( mpcomp_team_t ) );
	sctk_atomics_store_int( &( team->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i ), MPCOMP_NOWAIT_STOP_SYMBOL );

	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	for( i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN; i++) 
	{
		sctk_atomics_store_int( &(team->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i), MPCOMP_NOWAIT_STOP_SYMBOL );
	}
}

#endif /* __MPCOMP_TREE_STRUCTS_H__ */


