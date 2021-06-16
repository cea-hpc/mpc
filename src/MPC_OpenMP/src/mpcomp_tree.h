#ifndef MPCOMP_TREE_H_
#define MPCOMP_TREE_H_

#include "mpcomp_types.h"
#include "omp_intel.h"

/***************
 * TREE STRUCT *
 ***************/

#if 0

static inline void mpcomp_task_thread_infos_init( struct mpcomp_thread_s *thread )
{
	assert( thread );

	if ( !MPCOMP_TASK_THREAD_IS_INITIALIZED( thread ) )
	{
		mpcomp_task_t *implicit_task;
		mpcomp_task_list_t *tied_tasks_list;
		/* Allocate the default current task (no func, no data, no parent) */
		implicit_task = ( mpcomp_task_t * ) mpcomp_alloc( sizeof( mpcomp_task_t ) );
		assert( implicit_task );
		MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, NULL );
		_mpc_task_info_init( implicit_task, NULL, NULL, thread );
#ifdef MPCOMP_USE_TASKDEP
		implicit_task->task_dep_infos = ( mpcomp_task_dep_task_infos_t * )
		                                mpcomp_alloc( sizeof( mpcomp_task_dep_task_infos_t ) );
		assert( implicit_task->task_dep_infos );
		memset( implicit_task->task_dep_infos, 0,
		        sizeof( mpcomp_task_dep_task_infos_t ) );
#endif /* MPCOMP_USE_TASKDEP */
#if OMPT_SUPPORT

		if ( _mpc_omp_ompt_is_enabled() )
		{
			ompt_task_type_t task_type = ompt_task_implicit;

			if ( OMPT_Callbacks )
			{
				ompt_callback_task_create_t callback;
				callback = ( ompt_callback_task_create_t ) OMPT_Callbacks[ompt_callback_task_create];

				if ( callback )
				{
					ompt_data_t *task_data = &( implicit_task->ompt_task_data );
					const void *code_ra = __builtin_return_address( 0 );
					callback( NULL, NULL, task_data, task_type, ompt_task_initial, code_ra );
				}
			}
		}

#endif /* OMPT_SUPPORT */
		/* Allocate private task data structures */
		tied_tasks_list = ( mpcomp_task_list_t * ) mpcomp_alloc( sizeof( mpcomp_task_list_t ) );
		assert( tied_tasks_list );
		MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, implicit_task );
		MPCOMP_TASK_THREAD_SET_TIED_TASK_LIST_HEAD( thread, tied_tasks_list );
		OPA_store_int( &( implicit_task->refcount ), 1 );
		/* Change tasking_init_done to avoid multiple thread init */
		MPCOMP_TASK_THREAD_CMPL_INIT( thread );
	}
}


static inline void mpcomp_thread_infos_reset( mpcomp_thread_t *thread )
{
	assert( thread );
	memset( thread, 0, sizeof( mpcomp_thread_t ) );
}


static inline void
mpcomp_common_table_reset( struct common_table *th_pri_common )
{
	assert( th_pri_common );
	memset( th_pri_common, 0, sizeof( struct common_table ) );
}

static inline struct common_table *mpcomp_common_table_allocate( void )
{
	struct common_table *th_pri_common;
	th_pri_common =
	    ( struct common_table * )sctk_malloc( sizeof( struct common_table ) );
	assert( th_pri_common );
	mpcomp_common_table_reset( th_pri_common );
	return th_pri_common;
}

static inline void __mpcomp_thread_infos_init( mpcomp_thread_t *thread,
        mpcomp_local_icv_t icvs,
        __UNUSED__ mpcomp_instance_t *instance,
        __UNUSED__ mpcomp_thread_t *father )
{
	int i;
	/* RESET all thread_infos to NULL 		*/
	mpcomp_thread_infos_reset( thread );
	__mpcomp_parallel_region_infos_init( &( thread->info ) );
	/* SET all non null thread info fields	*/
	//  thread->father = father;
	thread->info.num_threads = 1;
	thread->info.icvs = icvs;
	//  thread->instance = instance;

	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	for ( i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN + 1; i++ )
	{
		OPA_store_int( &( thread->for_dyn_remain[i].i ), -1 );
	}

#if (defined(MPCOMP_TASK) || defined(MPCOMP_OPENMP_3_0))
	mpcomp_task_thread_infos_init( thread );
#endif /* MPCOMP_TASK || MPCOMP_OPENMP_3_0 */
	/* Intel openmp runtime */
#ifdef MPCOMP_USE_INTEL_ABI
	thread->push_num_threads = -1;
	thread->th_pri_common = mpcomp_common_table_allocate();
	thread->reduction_method = 0;
#endif /* MPCOMP_USE_INTEL_ABI */
}

static inline void
__mpcomp_thread_infos_init_and_push(    mpcomp_mvp_t *mvp,
                                        mpcomp_local_icv_t icvs,
                                        mpcomp_instance_t *instance,
                                        mpcomp_thread_t *father_thread )
{
	mpcomp_thread_t *new_thread;
	/* Allocation preserve numa affinity throw first touch mecanism */
	new_thread = ( mpcomp_thread_t * ) malloc( sizeof( mpcomp_thread_t ) );
	assert( new_thread );
	__mpcomp_thread_infos_init( new_thread, icvs, instance, father_thread );
	/* One running thread - One MVP | No need lock */
	new_thread->next = mvp->threads;
	mvp->threads = new_thread;
}

static inline void
__mpcomp_thread_infos_fini_and_pop( mpcomp_mvp_t *mvp )
{
	mpcomp_thread_t *old_thread;
	assert( mvp->threads );
	old_thread = mvp->threads;
	mvp->threads = old_thread->next;
	free( old_thread );
}


static inline void __mpcomp_team_infos_init( mpcomp_team_t *team )
{
	int i;
	assert( team );
	/* SET all non null task info fields */
	memset( team, 0, sizeof( mpcomp_team_t ) );
	OPA_store_int(
	    &( team->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i ),
	    MPCOMP_NOWAIT_STOP_SYMBOL );

	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	for ( i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN; i++ )
	{
		OPA_store_int( &( team->for_dyn_nb_threads_exited[i].i ), 0 );
	}
}

#endif

static inline void
_mpc_common_table_reset( struct common_table *th_pri_common )
{
	assert( th_pri_common );
	memset( th_pri_common, 0, sizeof( struct common_table ) );
}

static inline struct common_table *_mpc_common_table_allocate( void )
{
	struct common_table *th_pri_common;
	th_pri_common =
	    ( struct common_table * )sctk_malloc( sizeof( struct common_table ) );
	assert( th_pri_common );
	_mpc_common_table_reset( th_pri_common );
	return th_pri_common;
}

static inline void __mpcomp_thread_infos_init( mpcomp_thread_t *thread ) {
    assert( thread );

    int i;
	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	for ( i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN + 1; i++ )
	{
		OPA_store_int( &( thread->for_dyn_remain[i].i ), -1 );
	}

#ifdef MPCOMP_USE_INTEL_ABI
	thread->push_num_threads = -1;
	thread->th_pri_common = _mpc_common_table_allocate();
	thread->reduction_method = 0;
#endif /* MPCOMP_USE_INTEL_ABI */
}


/**************
 * TASK UTILS *
 **************/

#if( MPCOMP_TASK || defined( MPCOMP_OPENMP_3_0 ) )

	int mpc_omp_tree_array_get_neighbor( int globalRank, int i );
	int mpc_omp_tree_array_ancestor_get( int globalRank, int depth );

	void _mpc_omp_tree_task_check_neigborhood( void );

#endif /* MPCOMP_TASK */

/**************
 * TREE ARRAY *
 **************/

void __mpcomp_init_seq_region();
void __mpcomp_init_initial_thread( const mpcomp_local_icv_t icvs );
void _mpc_omp_tree_alloc( int *shape, int max_depth, const int *cpus_order, const int place_depth, const int place_size );

int *mpc_omp_tree_array_compute_thread_min_rank( const int *shape, const int max_depth, const int rank, const int core_depth );


#endif /* MPCOMP_TREE_H_ */
