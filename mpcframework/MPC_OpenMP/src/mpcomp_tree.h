#ifndef MPCOMP_TREE_H_
#define MPCOMP_TREE_H_

#include "mpcomp_types.h"

/***************
 * TREE STRUCT *
 ***************/

#if 0
static inline void mpcomp_thread_infos_reset( mpcomp_thread_t *thread )
{
	sctk_assert( thread );
	memset( thread, 0, sizeof( mpcomp_thread_t ) );
}


static inline void
mpcomp_common_table_reset( struct common_table *th_pri_common )
{
	sctk_assert( th_pri_common );
	memset( th_pri_common, 0, sizeof( struct common_table ) );
}

static inline struct common_table *mpcomp_common_table_allocate( void )
{
	struct common_table *th_pri_common;
	th_pri_common =
	    ( struct common_table * )sctk_malloc( sizeof( struct common_table ) );
	sctk_assert( th_pri_common );
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
	sctk_assert( team );
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

void _mpc_omp_tree_alloc( int *shape, int max_depth, const int *cpus_order, const int place_depth, const int place_size, const mpcomp_local_icv_t *icvs );

int *mpc_omp_tree_array_compute_thread_min_rank( const int *shape, const int max_depth, const int rank, const int core_depth );


#endif /* MPCOMP_TREE_H_ */