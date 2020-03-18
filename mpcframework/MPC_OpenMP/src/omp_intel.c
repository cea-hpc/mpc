#include "omp_intel.h"

#include "sctk_debug.h"

#include "mpcomp_core.h"
#include "mpcomp_loop.h"

#include "mpcomp_parallel_region.h"

#include "mpcomp_sync.h"

#include "mpcomp_task.h"


/********************
 * GLOBAL VARIABLES *
 ********************/

int __kmp_force_reduction_method = reduction_method_not_defined;
int __kmp_init_common = 0;
kmp_cached_addr_t *__kmp_threadpriv_cache_list = NULL; /* list for cleaning */
struct shared_table __kmp_threadprivate_d_table;

/***********************
 * RUNTIME BEGIN & END *
 ***********************/

void __kmpc_begin( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 flags )
{
	/* nothing to do */
}

void __kmpc_end( __UNUSED__ ident_t *loc )
{
	/* nothing to do */
}

/*************
 * INTEL API *
 *************/

kmp_int32 __kmpc_global_thread_num( __UNUSED__ ident_t *loc )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> omp_get_max_threads", __func__ );
	return ( kmp_int32 )omp_get_thread_num();
}

kmp_int32 __kmpc_global_num_threads( __UNUSED__ ident_t *loc )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> omp_get_num_threads", __func__ );
	return ( kmp_int32 )omp_get_num_threads();
}

kmp_int32 __kmpc_bound_thread_num( __UNUSED__ ident_t *loc )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> omp_get_thread_num", __func__ );
	return ( kmp_int32 )omp_get_thread_num();
}

kmp_int32 __kmpc_bound_num_threads( __UNUSED__ ident_t *loc )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> omp_get_num_threads", __func__ );
	return ( kmp_int32 )omp_get_num_threads();
}

kmp_int32 __kmpc_in_parallel( __UNUSED__ ident_t *loc )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> omp_in_parallel", __func__ );
	return ( kmp_int32 )omp_in_parallel();
}

void __kmpc_flush( __UNUSED__ ident_t *loc, ... )
{
	/* TODO depending on the compiler, call the right function
	 * Maybe need to do the same for mpcomp_flush...
	 */
	__sync_synchronize();
}

/***********
 * BARRIER *
 * ********/

void __kmpc_barrier( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_nodebug( "[%d] %s: entering...", t->rank, __func__ );
	__mpcomp_barrier();
}

kmp_int32 __kmpc_barrier_master( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	not_implemented();
	return ( kmp_int32 )0;
}

void __kmpc_end_barrier_master( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	not_implemented();
}

kmp_int32 __kmpc_barrier_master_nowait( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	not_implemented();
	return ( kmp_int32 )0;
}


/*******************
 * WRAPPER FUNTION *
 *******************/

void __intel_wrapper_func( void *arg )
{
	mpcomp_intel_wrapper_t *w = ( mpcomp_intel_wrapper_t * )arg;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( w );
	sctk_nodebug( "%s: f: %p, argc: %d, args: %p", __func__, w->f, w->argc,
	              w->args );
	__kmp_invoke_microtask( w->f, t->rank, 0, w->argc, w->args );
}

/*******************
 * PARALLEL REGION *
 *******************/



kmp_int32 __kmpc_ok_to_fork( __UNUSED__ ident_t *loc )
{
	sctk_nodebug( "%s: entering...", __func__ );
	return ( kmp_int32 )1;
}

void __kmpc_fork_call( __UNUSED__ ident_t *loc, kmp_int32 argc, kmpc_micro microtask, ... )
{
	va_list args;
	int i;
	void **args_copy;
	mpcomp_intel_wrapper_t *w;
	mpcomp_thread_t *t;
	static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
	sctk_nodebug( "__kmpc_fork_call: entering w/ %d arg(s)...", argc );
	mpcomp_intel_wrapper_t w_noalloc;
	w = &w_noalloc;
	/* Handle orphaned directive (initialize OpenMP environment) */
	__mpcomp_init();

	/* Threadprivate initialisation */
	if ( !__kmp_init_common )
	{
		sctk_thread_mutex_lock( &lock );

		if ( !__kmp_init_common )
		{
			for ( i = 0; i < KMP_HASH_TABLE_SIZE; ++i )
			{
				__kmp_threadprivate_d_table.data[i] = 0;
			}

			__kmp_init_common = 1;
		}

		sctk_thread_mutex_unlock( &lock );
	}

	/* Grab info on the current thread */
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );

	if ( t->args_copy == NULL || argc > t->temp_argc )
	{
		if ( t->args_copy != NULL )
		{
			free( t->args_copy );
		}

		t->args_copy = ( void ** ) malloc( argc * sizeof( void * ) );
		t->temp_argc = argc;
	}

	args_copy = t->args_copy;
	sctk_assert( args_copy );
	va_start( args, microtask );

	for ( i = 0; i < argc; i++ )
	{
		args_copy[i] = va_arg( args, void * );
	}

	va_end( args );
	w->f = microtask;
	w->argc = argc;
	w->args = args_copy;
	/* translate for gcc value */
	t->push_num_threads = ( t->push_num_threads <= 0 ) ? 0 : t->push_num_threads;
	sctk_nodebug( " f: %p, argc: %d, args: %p, num_thread: %d", w->f, w->argc,
	              w->args, t->push_num_threads );
	__mpcomp_start_parallel_region( __intel_wrapper_func, w,
	                                t->push_num_threads );
	/* restore the number of threads w/ num_threads clause */
	t->push_num_threads = 0;
}

void __kmpc_serialized_parallel( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	mpcomp_thread_t *t;
	mpcomp_parallel_region_t *info;
#ifdef MPCOMP_FORCE_PARALLEL_REGION_ALLOC
	info = sctk_malloc( sizeof( mpcomp_parallel_region_t ) );
	assert( info );
#else  /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */
	mpcomp_parallel_region_t noallocate_info;
	info = &noallocate_info;
#endif /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */
	sctk_nodebug( "%s: entering (%d) ...", __func__, global_tid );
	/* Grab the thread info */
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	info->func = NULL; /* No function to call */
	info->shared = NULL;
	info->num_threads = 1;
	info->new_root = NULL;
	info->icvs = t->info.icvs;
	info->combined_pragma = MPCOMP_COMBINED_NONE;
	__mpcomp_internal_begin_parallel_region( info, 1 );
	/* Save the current tls */
	t->children_instance->mvps[0].ptr.mvp->threads[0].father = sctk_openmp_thread_tls;
	/* Switch TLS to nested thread for region-body execution */
	sctk_openmp_thread_tls = &( t->children_instance->mvps[0].ptr.mvp->threads[0] );
	sctk_nodebug( "%s: leaving (%d) ...", __func__, global_tid );
}

void __kmpc_end_serialized_parallel( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	mpcomp_thread_t *t;
	sctk_nodebug( "%s: entering (%d)...", __func__, global_tid );
	/* Grab the thread info */
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	/* Restore the previous thread info */
	sctk_openmp_thread_tls = t->father;
	__mpcomp_internal_end_parallel_region( t->instance );
	sctk_nodebug( "%s: leaving (%d)...", __func__, global_tid );
}

void __kmpc_push_num_threads( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 global_tid,
                              kmp_int32 num_threads )
{
	mpcomp_thread_t *t;
	sctk_warning( "%s: pushing %d thread(s)", __func__, num_threads );
	/* Handle orphaned directive (initialize OpenMP environment) */
	__mpcomp_init();
	/* Grab the thread info */
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	t->push_num_threads = num_threads;
}

void __kmpc_push_num_teams( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                            __UNUSED__ kmp_int32 num_teams, __UNUSED__ kmp_int32 num_threads )
{
	not_implemented();
}

void __kmpc_fork_teams( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 argc, __UNUSED__  kmpc_micro microtask,
                        ... )
{
	not_implemented();
}

/*******************
 * SYNCHRONIZATION *
 *******************/



kmp_int32 __kmpc_master( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	sctk_nodebug( "[%d] %s: entering", t->rank, __func__ );
	return ( t->rank == 0 ) ? ( kmp_int32 )1 : ( kmp_int32 )0;
}

void __kmpc_end_master( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> None", __func__ );
}

void __kmpc_ordered( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> __mpcomp_ordered_begin", __func__ );
	__mpcomp_ordered_begin();
}

void __kmpc_end_ordered( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> __mpcomp_ordered_end", __func__ );
	__mpcomp_ordered_end();
}

void __kmpc_critical( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                      __UNUSED__ kmp_critical_name *crit )
{
	/* TODO Handle named critical */
	sctk_nodebug( "[REDIRECT KMP]: %s -> __mpcomp_anonymous_critical_begin",
	              __func__ );
	__mpcomp_anonymous_critical_begin();
}

void __kmpc_end_critical( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                          __UNUSED__ kmp_critical_name *crit )
{
	/* TODO Handle named critical */
	sctk_nodebug( "[REDIRECT KMP]: %s -> __mpcomp_anonymous_critical_end",
	              __func__ );
	__mpcomp_anonymous_critical_end();
}

kmp_int32 __kmpc_single( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 global_tid )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> __mpcomp_do_single", __func__ );
	return ( kmp_int32 )__mpcomp_do_single();
}

void __kmpc_end_single( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> None", __func__ );
}

/*********
 * LOCKS *
 *********/


void __kmpc_init_lock( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid, void **user_lock )
{
	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock );
	omp_init_lock( user_mpcomp_lock );
}

void __kmpc_init_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid, void **user_lock )
{
	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock );
	omp_init_nest_lock( user_mpcomp_nest_lock );
}

void __kmpc_destroy_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock );
	omp_destroy_lock( user_mpcomp_lock );
}

void __kmpc_destroy_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock );
	omp_destroy_nest_lock( user_mpcomp_nest_lock );
}

void __kmpc_set_lock( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid, void **user_lock )
{
	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock );
	omp_set_lock( user_mpcomp_lock );
}

void __kmpc_set_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock );
	omp_set_nest_lock( user_mpcomp_nest_lock );
}

void __kmpc_unset_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock );
	omp_unset_lock( user_mpcomp_lock );
}

void __kmpc_unset_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock );
	omp_unset_nest_lock( user_mpcomp_nest_lock );
}

int __kmpc_test_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	const int ret = omp_test_lock( user_mpcomp_lock );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p try: %d", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock, ret );
	return ret;
}

int __kmpc_test_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid, void **user_lock )
{
	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	const int ret = omp_test_nest_lock( user_mpcomp_nest_lock );
	sctk_nodebug( "[%d] %s: iomp: %p mpcomp: %p try: %d", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock, ret );
	return ret;
}

/*************
 * REDUCTION *
 *************/

int __kmp_determine_reduction_method(
    ident_t *loc, __UNUSED__ kmp_int32 global_tid, __UNUSED__ kmp_int32 num_vars, __UNUSED__ size_t reduce_size,
    void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ),
    kmp_critical_name *lck, mpcomp_thread_t *t )
{
	int retval = critical_reduce_block;
	int team_size;
	int teamsize_cutoff = 4;
	team_size = t->instance->team->info.num_threads;

	if ( team_size == 1 )
	{
		return empty_reduce_block;
	}
	else
	{
		int atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
		int tree_available = FAST_REDUCTION_TREE_METHOD_GENERATED;

		if ( tree_available )
		{
			if ( team_size <= teamsize_cutoff )
			{
				if ( atomic_available )
				{
					retval = atomic_reduce_block;
				}
			}
			else
			{
				retval = tree_reduce_block;
			}
		}
		else if ( atomic_available )
		{
			retval = atomic_reduce_block;
		}
	}

	/* force reduction method */
	if ( __kmp_force_reduction_method != reduction_method_not_defined )
	{
		int forced_retval;
		int atomic_available, tree_available;

		switch ( ( forced_retval = __kmp_force_reduction_method ) )
		{
			case critical_reduce_block:
				sctk_assert( lck );

				if ( team_size <= 1 )
				{
					forced_retval = empty_reduce_block;
				}

				break;

			case atomic_reduce_block:
				atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
				sctk_assert( atomic_available );
				break;

			case tree_reduce_block:
				tree_available = FAST_REDUCTION_TREE_METHOD_GENERATED;
				sctk_assert( tree_available );
				forced_retval = tree_reduce_block;
				break;

			default:
				forced_retval = critical_reduce_block;
				sctk_debug( "Unknown reduction method" );
		}

		retval = forced_retval;
	}

	return retval;
}

static inline int __intel_tree_reduce_nowait( void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ), mpcomp_thread_t *t )
{
	mpcomp_node_t *c, *new_root;
	mpcomp_mvp_t *mvp = t->mvp;

	if ( t->info.num_threads == 1 )
	{
		return 1;
	}

	sctk_assert( mvp );
	sctk_assert( mvp->father );
	sctk_assert( mvp->father->instance );
	sctk_assert( mvp->father->instance->team );
	c = mvp->father;
	new_root = c->instance->root;
	int local_rank = t->rank % c->barrier_num_threads; /* rank of mvp on the node */
	int remaining_threads = c->barrier_num_threads; /* number of threads of the node still on reduction */
	int cache_size = 64;

	while ( 1 )
	{
		while ( remaining_threads > 1 ) /* do reduction of the node */
		{
			if ( remaining_threads % 2 != 0 )
			{
				remaining_threads = ( remaining_threads / 2 ) + 1;

				if ( local_rank == remaining_threads - 1 )
				{
					c->reduce_data[local_rank * cache_size] = reduce_data;
				}
				else if ( local_rank >= remaining_threads )
				{
					c->reduce_data[local_rank * cache_size] = reduce_data;
					c->isArrived[local_rank * cache_size] = 1;

					while ( c->isArrived[local_rank * cache_size] != 0 )
					{
						/* need to wait for the pair thread doing the reduction operation before exiting function because compiler will delete reduce_data after */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[local_rank * cache_size] ), 0 );
					}

					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
						/* waiting for pair thread to arrive */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
					}

					( *reduce_func )( reduce_data, c->reduce_data[( local_rank + remaining_threads ) * cache_size] ); // result on reduce_data
					c->isArrived[( local_rank + remaining_threads ) * cache_size] = 0;
				}
			}
			else
			{
				remaining_threads = remaining_threads / 2;

				if ( local_rank >= remaining_threads )
				{
					c->reduce_data[local_rank * cache_size] = reduce_data;
					c->isArrived[local_rank * cache_size] = 1;

					while ( c->isArrived[local_rank * cache_size] != 0 )
					{
						/* need to wait for the thread doing the reduction operation before exiting function because compiler will delete reduce_data after */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[local_rank * cache_size] ), 0 );
					}

					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
						/* waiting for pair thread to arrive */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
					}

					( *reduce_func )( reduce_data, c->reduce_data[( local_rank + remaining_threads ) * cache_size] ); // result on reduce_data
					c->isArrived[( local_rank + remaining_threads ) * cache_size] = 0;
				}
			}
		}

		if ( c == new_root )
		{
			return 1;
		}

		local_rank = c->rank;
		c = c->father;
		remaining_threads = c->barrier_num_threads;
		/* Climb on the tree */
	}
}

static inline int __intel_tree_reduce( void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ), mpcomp_thread_t *t )
{
	mpcomp_node_t *c, *new_root;
	mpcomp_mvp_t *mvp = t->mvp;

	if ( t->info.num_threads == 1 )
	{
		return 1;
	}

	sctk_assert( mvp );
	sctk_assert( mvp->father );
	sctk_assert( mvp->father->instance );
	sctk_assert( mvp->father->instance->team );
	c = mvp->father;
	new_root = c->instance->root;
	int local_rank = t->rank % c->barrier_num_threads; /* rank of mvp on the node */
	int remaining_threads = c->barrier_num_threads; /* number of threads of the node still on reduction */
	long b_done;
	int cache_size = 64;

	while ( 1 )
	{
		while ( remaining_threads > 1 ) /* do reduction of the node */
		{
			if ( remaining_threads % 2 != 0 )
			{
				remaining_threads = ( remaining_threads / 2 ) + 1;

				if ( local_rank == remaining_threads - 1 )
				{
					/* one thread put his contribution and go next step if remaining threads is odd */
					c->reduce_data[local_rank * cache_size] = reduce_data;
				}
				else if ( local_rank >= remaining_threads )
				{
					b_done = c->barrier_done;
					c->reduce_data[local_rank * cache_size] = reduce_data;
					c->isArrived[local_rank * cache_size] = 1;

					while ( b_done == c->barrier_done )
						/* wait for master thread to end and share the result, this is done when it enters in __kmpc_end_reduce function */
					{
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->barrier_done ), b_done + 1);
					}

					while ( c->child_type != MPCOMP_CHILDREN_LEAF )   /* Go down */
					{
						c = c->children.node[mvp->tree_rank[c->depth]];
						c->barrier_done++;
					}

					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
						/* waiting for pair thread to arrive */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
					}

					( *reduce_func )( reduce_data, c->reduce_data[( local_rank + remaining_threads ) * cache_size] ); // result on reduce_data
					c->isArrived[( local_rank + remaining_threads ) * cache_size] = 0;
				}
			}
			else
			{
				remaining_threads = remaining_threads / 2;

				if ( local_rank >= remaining_threads )
				{
					b_done = c->barrier_done;
					c->reduce_data[local_rank * cache_size] = reduce_data;
					c->isArrived[local_rank * cache_size] = 1;

					while ( b_done == c->barrier_done )
						/* wait for master thread to end and share the result, this is done when it enters in __kmpc_end_reduce function */
					{
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->barrier_done ), b_done + 1);
					}

					while ( c->child_type != MPCOMP_CHILDREN_LEAF )   /* Go down */
					{
						c = c->children.node[mvp->tree_rank[c->depth]];
						c->barrier_done++;
					}

					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
						/* waiting for pair thread to arrive */
						sctk_thread_yield();
						//sctk_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
					}

					( *reduce_func )( reduce_data, c->reduce_data[( local_rank + remaining_threads ) * cache_size] ); // result on reduce_data
					c->isArrived[( local_rank + remaining_threads ) * cache_size] = 0;
				}
			}
		}

		if ( c == new_root )
		{
			return 1;
		}

		local_rank = c->rank;
		c = c->father;
		remaining_threads = c->barrier_num_threads;
		/* Climb on the tree */
	}
}

kmp_int32 __kmpc_reduce_nowait( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars,
                                size_t reduce_size, void *reduce_data,
                                void ( *reduce_func )( void *lhs_data, void *rhs_data ),
                                kmp_critical_name *lck )
{
	int retval = 0;
	int packed_reduction_method;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	/* get reduction method */
	packed_reduction_method = __kmp_determine_reduction_method(
	                              loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck, t );
	t->reduction_method = packed_reduction_method;

	if ( packed_reduction_method == critical_reduce_block )
	{
		__mpcomp_anonymous_critical_begin();
		retval = 1;
	}
	else if ( packed_reduction_method == empty_reduce_block )
	{
		retval = 1;
	}
	else if ( packed_reduction_method == atomic_reduce_block )
	{
		retval = 2;
	}
	else if ( packed_reduction_method == tree_reduce_block )
	{
		retval = __intel_tree_reduce_nowait( reduce_data, reduce_func, t );
		/* retval = 1 for thread with reduction value, 0 for others */
	}
	else
	{
		sctk_error( "__kmpc_reduce_nowait: Unexpected reduction method %d",
		            packed_reduction_method );
		sctk_abort();
	}

	return retval;
}

void __kmpc_end_reduce_nowait( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                               __UNUSED__ kmp_critical_name *lck )
{
	int packed_reduction_method;
	/* get reduction method */
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	packed_reduction_method = t->reduction_method;

	if ( packed_reduction_method == critical_reduce_block )
	{
		__mpcomp_anonymous_critical_end();
		__mpcomp_barrier();
	}
	else if ( packed_reduction_method == empty_reduce_block )
	{
	}
	else if ( packed_reduction_method == atomic_reduce_block )
	{
	}
	else if ( packed_reduction_method == tree_reduce_block )
	{
	}
	else
	{
		sctk_error( "__kmpc_end_reduce_nowait: Unexpected reduction method %d",
		            packed_reduction_method );
		sctk_abort();
	}
}

kmp_int32 __kmpc_reduce( ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars,
                         size_t reduce_size, void *reduce_data,
                         void ( *reduce_func )( void *lhs_data, void *rhs_data ),
                         kmp_critical_name *lck )
{
	int retval = 0;
	int packed_reduction_method;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	/* get reduction method */
	packed_reduction_method = __kmp_determine_reduction_method(
	                              loc, global_tid, num_vars, reduce_size, reduce_data, reduce_func, lck, t );
	t->reduction_method = packed_reduction_method;

	if ( packed_reduction_method == critical_reduce_block )
	{
		__mpcomp_anonymous_critical_begin();
		retval = 1;
	}
	else if ( packed_reduction_method == empty_reduce_block )
	{
		retval = 1;
	}
	else if ( packed_reduction_method == atomic_reduce_block )
	{
		retval = 2;
	}
	else if ( packed_reduction_method == tree_reduce_block )
	{
		retval = __intel_tree_reduce( reduce_data, reduce_func, t );
		/* retval = 1 for thread with reduction value (thread 0), 0 for others. When retval = 0 the thread doesn't enter __kmpc_end_reduce function */
	}
	else
	{
		sctk_error( "__kmpc_reduce_nowait: Unexpected reduction method %d",
		            packed_reduction_method );
		sctk_abort();
	}

	return retval;
}

void __kmpc_end_reduce( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                        __UNUSED__ kmp_critical_name *lck )
{
	int packed_reduction_method;
	/* get reduction method */
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	packed_reduction_method = t->reduction_method;

	if ( packed_reduction_method == critical_reduce_block )
	{
		__mpcomp_anonymous_critical_end();
		__mpcomp_barrier();
	}
	else if ( packed_reduction_method == atomic_reduce_block )
	{
		__mpcomp_barrier();
	}
	else if ( packed_reduction_method == tree_reduce_block )
	{
		/* For tree reduction algorithm when thread 0 enter __kmpc_end_reduce reduction value is already shared among all threads */
		mpcomp_mvp_t *mvp = t->mvp;
		mpcomp_node_t *c = t->instance->root;
		c->barrier_done++;

		/* Go down to release all threads still waiting on __intel_tree_reduce for the reduction to be done, we don't need to do a full barrier  */
		while ( c->child_type != MPCOMP_CHILDREN_LEAF )
		{
			c = c->children.node[mvp->tree_rank[c->depth]];
			c->barrier_done++;
		}
	}
}

/**********
 * STATIC *
 **********/



void __kmpc_for_static_init_4( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, kmp_int32 schedtype,
                               kmp_int32 *plastiter, kmp_int32 *plower,
                               kmp_int32 *pupper, kmp_int32 *pstride,
                               kmp_int32 incr, kmp_int32 chunk )
{
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	int rank = t->rank;
	int num_threads = t->info.num_threads;
	// save old pupper
	int pupper_old = *pupper;

	if ( num_threads == 1 )
	{
		if ( plastiter != NULL )
		{
			*plastiter = TRUE;
		}

		*pstride =
		    ( incr > 0 ) ? ( *pupper - *plower + 1 ) : ( -( *plower - *pupper + 1 ) );
		return;
	}

	sctk_nodebug( "[%d] %s: <%s> "
	              "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
	              "*plastiter=%d *pstride=%d",
	              t->rank, __func__, loc->psource, schedtype, *plastiter, *plower,
	              *pupper, *pstride, incr, chunk, *plastiter, *pstride );
	int trip_count;

	if ( incr > 0 )
	{
		trip_count = ( ( *pupper - *plower ) / incr ) + 1;
	}
	else
	{
		trip_count = ( ( *plower - *pupper ) / ( -incr ) ) + 1;
	}

	switch ( schedtype )
	{
		case kmp_sch_static:
		{
			if ( trip_count <= num_threads )
			{
				if ( rank < trip_count )
				{
					*pupper = *plower = *plower + rank * incr;
				}
				else
				{
					*plower = *pupper + incr;
					return;
				}

				if ( plastiter != NULL )
				{
					*plastiter = ( rank == trip_count - 1 );
				}

				return ;
			}
			else
			{
				int chunk_size = trip_count / num_threads;
				int extras = trip_count % num_threads;

				if ( rank < extras )
				{
					/* The first part is homogeneous with a chunk size a little bit larger */
					*pupper = *plower + ( rank + 1 ) * ( chunk_size + 1 ) * incr - incr;
					*plower = *plower + rank * ( chunk_size + 1 ) * incr;
				}
				else
				{
					/* The final part has a smaller chunk_size, therefore the computation is
					   splitted in 2 parts */
					*pupper = *plower + extras * ( chunk_size + 1 ) * incr +
					          ( rank + 1 - extras ) * chunk_size * incr - incr;
					*plower = *plower + extras * ( chunk_size + 1 ) * incr +
					          ( rank - extras ) * chunk_size * incr;
				}

				/* Remarks:
				   - MPC chunk has not-inclusive upper bound while Intel runtime includes
				   upper bound for calculation
				   - Need to cast from long to int because MPC handles everything has a
				   long
				   (like GCC) while Intel creates different functions
				   */
				if ( plastiter != NULL )
				{
					if ( incr > 0 )
					{
						*plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
					}
					else
					{
						*plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
					}
				}
			}

			break;
		}

		case kmp_sch_static_chunked:
		{
			chunk = ( chunk < 1 ) ? 1 : chunk;
			*pstride = ( chunk * incr ) * t->info.num_threads;
			*plower = *plower + ( ( chunk * incr ) * t->rank );
			*pupper = *plower + ( chunk * incr ) - incr;

			// plastiter computation
			if ( plastiter != NULL )
			{
				*plastiter = ( t->rank == ( ( trip_count - 1 ) / chunk ) % t->info.num_threads );
			}

			break;
		}

		default:
			not_implemented();
			break;
	}
}

void __kmpc_for_static_init_4u( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid,
                                kmp_int32 schedtype, kmp_int32 *plastiter,
                                kmp_uint32 *plower, kmp_uint32 *pupper,
                                kmp_int32 *pstride, kmp_int32 incr,
                                kmp_int32 chunk )
{
	mpcomp_thread_t *t;
	mpcomp_loop_gen_info_t *loop_infos;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	unsigned long rank = ( unsigned long )t->rank;
	unsigned int num_threads = ( unsigned int )t->info.num_threads;
	// save old pupper
	kmp_uint32 pupper_old = *pupper;

	if ( t->info.num_threads == 1 )
	{
		if ( plastiter != NULL )
		{
			*plastiter = TRUE;
		}

		*pstride =
		    ( incr > 0 ) ? ( *pupper - *plower + 1 ) : ( -( *plower - *pupper + 1 ) );
		return;
	}

	loop_infos = &( t->info.loop_infos );
	__mpcomp_loop_gen_infos_init_ull(
	    loop_infos, ( unsigned long long )*plower,
	    ( unsigned long long )*pupper + ( unsigned long long )incr,
	    ( unsigned long long )incr, ( unsigned long long )chunk );
	sctk_nodebug( "[%d] __kmpc_for_static_init_4u: <%s> "
	              "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
	              "*plastiter=%d *pstride=%d",
	              ( ( mpcomp_thread_t * )sctk_openmp_thread_tls )->rank, loc->psource,
	              schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
	              *plastiter, *pstride );
	unsigned long long trip_count;

	if ( incr > 0 )
	{
		trip_count = ( ( *pupper - *plower ) / incr ) + 1;
	}
	else
	{
		trip_count = ( ( *plower - *pupper ) / ( -incr ) ) + 1;
	}

	switch ( schedtype )
	{
		case kmp_sch_static:
		{
			if ( trip_count <= num_threads )
			{
				if ( rank < trip_count )
				{
					*pupper = *plower = *plower + rank * incr;
				}
				else
				{
					*plower = *pupper + incr;
					return;
				}

				if ( plastiter != NULL )
				{
					*plastiter = ( rank == trip_count - 1 );
				}

				return ;
			}
			else
			{
				int chunk_size = trip_count / num_threads;
				unsigned long int extras = trip_count % num_threads;

				if ( rank < extras )
				{
					/* The first part is homogeneous with a chunk size a little bit larger */
					*pupper = *plower + ( rank + 1 ) * ( chunk_size + 1 ) * incr - incr;
					*plower = *plower + rank * ( chunk_size + 1 ) * incr;
				}
				else
				{
					/* The final part has a smaller chunk_size, therefore the computation is
					   splitted in 2 parts */
					*pupper = *plower + extras * ( chunk_size + 1 ) * incr +
					          ( rank + 1 - extras ) * chunk_size * incr - incr;
					*plower = *plower + extras * ( chunk_size + 1 ) * incr +
					          ( rank - extras ) * chunk_size * incr;
				}

				/* Remarks:
				   - MPC chunk has not-inclusive upper bound while Intel runtime includes
				   upper bound for calculation
				   - Need to cast from long to int because MPC handles everything has a
				   long
				   (like GCC) while Intel creates different functions
				   */
				if ( plastiter != NULL )
				{
					if ( incr > 0 )
					{
						*plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
					}
					else
					{
						*plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
					}
				}
			}

			break;
		}

		case kmp_sch_static_chunked:
		{
			chunk = ( chunk < 1 ) ? 1 : chunk;
			*pstride = ( chunk * incr ) * t->info.num_threads;
			*plower = *plower + ( ( chunk * incr ) * t->rank );
			*pupper = *plower + ( chunk * incr ) - incr;

			// plastiter computation
			if ( plastiter != NULL )
			{
				*plastiter = ( ( unsigned long )t->rank == ( ( trip_count - 1 ) / chunk ) % t->info.num_threads );
			}

			break;
		}

		default:
			not_implemented();
			break;
			sctk_nodebug( "[%d] Results: "
			              "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
			              t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
			              *plastiter );
	}
}

void __kmpc_for_static_init_8( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, kmp_int32 schedtype,
                               kmp_int32 *plastiter, kmp_int64 *plower,
                               kmp_int64 *pupper, kmp_int64 *pstride,
                               kmp_int64 incr, kmp_int64 chunk )
{
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	int rank = t->rank;
	int num_threads = t->info.num_threads;
	// save old pupper
	kmp_int64 pupper_old = *pupper;

	if ( t->info.num_threads == 1 )
	{
		if ( plastiter != NULL )
		{
			*plastiter = TRUE;
		}

		*pstride =
		    ( incr > 0 ) ? ( *pupper - *plower + 1 ) : ( -( *plower - *pupper + 1 ) );
		return;
	}

	sctk_nodebug( "[%d] __kmpc_for_static_init_8: <%s> "
	              "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
	              "*plastiter=%d *pstride=%d",
	              ( ( mpcomp_thread_t * )sctk_openmp_thread_tls )->rank, loc->psource,
	              schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
	              *plastiter, *pstride );
	long long trip_count;

	if ( incr > 0 )
	{
		trip_count = ( ( *pupper - *plower ) / incr ) + 1;
	}
	else
	{
		trip_count = ( ( *plower - *pupper ) / ( -incr ) ) + 1;
	}

	switch ( schedtype )
	{
		case kmp_sch_static:
		{
			if ( trip_count <= num_threads )
			{
				if ( rank < trip_count )
				{
					*pupper = *plower = *plower + rank * incr;
				}
				else
				{
					*plower = *pupper + incr;
					return;
				}

				if ( plastiter != NULL )
				{
					*plastiter = ( rank == trip_count - 1 );
				}

				return ;
			}
			else
			{
				long long chunk_size = trip_count / num_threads;
				int extras = trip_count % num_threads;

				if ( rank < extras )
				{
					/* The first part is homogeneous with a chunk size a little bit larger */
					*pupper = *plower + ( rank + 1 ) * ( chunk_size + 1 ) * incr - incr;
					*plower = *plower + rank * ( chunk_size + 1 ) * incr;
				}
				else
				{
					/* The final part has a smaller chunk_size, therefore the computation is
					   splitted in 2 parts */
					*pupper = *plower + extras * ( chunk_size + 1 ) * incr +
					          ( rank + 1 - extras ) * chunk_size * incr - incr;
					*plower = *plower + extras * ( chunk_size + 1 ) * incr +
					          ( rank - extras ) * chunk_size * incr;
				}

				/* Remarks:
				   - MPC chunk has not-inclusive upper bound while Intel runtime includes
				   upper bound for calculation
				   - Need to cast from long to int because MPC handles everything has a
				   long
				   (like GCC) while Intel creates different functions
				   */
				if ( plastiter != NULL )
				{
					if ( incr > 0 )
					{
						*plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
					}
					else
					{
						*plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
					}
				}
			}

			break;
		}

		case kmp_sch_static_chunked:
		{
			chunk = ( chunk < 1 ) ? 1 : chunk;
			*pstride = ( chunk * incr ) * t->info.num_threads;
			*plower = *plower + ( ( chunk * incr ) * t->rank );
			*pupper = *plower + ( chunk * incr ) - incr;

			// plastiter computation
			if ( plastiter != NULL )
			{
				*plastiter = ( t->rank == ( ( trip_count - 1 ) / chunk ) % t->info.num_threads );
			}

			break;
		}

		default:
			not_implemented();
			break;
			sctk_nodebug( "[%d] Results: "
			              "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
			              t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
			              *plastiter );
	}
}

void __kmpc_for_static_init_8u( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid,
                                kmp_int32 schedtype, kmp_int32 *plastiter,
                                kmp_uint64 *plower, kmp_uint64 *pupper,
                                kmp_int64 *pstride, kmp_int64 incr,
                                kmp_int64 chunk )
{
	/* TODO: the same as unsigned long long in GCC... */
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	unsigned long  rank = t->rank;
	unsigned long num_threads = t->info.num_threads;
	mpcomp_loop_gen_info_t *loop_infos;
	kmp_uint64 pupper_old = *pupper;

	if ( t->info.num_threads == 1 )
	{
		if ( plastiter != NULL )
		{
			*plastiter = TRUE;
		}

		*pstride =
		    ( incr > 0 ) ? ( *pupper - *plower + 1 ) : ( -( *plower - *pupper + 1 ) );
		return;
	}

	loop_infos = &( t->info.loop_infos );
	__mpcomp_loop_gen_infos_init_ull(
	    loop_infos, ( unsigned long long )*plower,
	    ( unsigned long long )*pupper + ( unsigned long long )incr,
	    ( unsigned long long )incr, ( unsigned long long )chunk );
	sctk_nodebug( "[%ld] __kmpc_for_static_init_8u: <%s> "
	              "schedtype=%d, %d? %llu -> %llu incl. [%lld], incr=%lld chunk=%lld "
	              "*plastiter=%d *pstride=%lld",
	              ( ( mpcomp_thread_t * )sctk_openmp_thread_tls )->rank, loc->psource,
	              schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
	              *plastiter, *pstride );
	unsigned long long trip_count;

	if ( incr > 0 )
	{
		trip_count = ( ( *pupper - *plower ) / incr ) + 1;
	}
	else
	{
		trip_count = ( ( *plower - *pupper ) / ( -incr ) ) + 1;
	}

	switch ( schedtype )
	{
		case kmp_sch_static:
		{
			if ( trip_count <= num_threads )
			{
				if ( rank < trip_count )
				{
					*pupper = *plower = *plower + rank * incr;
				}
				else
				{
					*plower = *pupper + incr;
					return;
				}

				if ( plastiter != NULL )
				{
					*plastiter = ( rank == trip_count - 1 );
				}

				return ;
			}
			else
			{
				unsigned long long chunk_size = trip_count / num_threads;
				unsigned long extras = trip_count % num_threads;

				if ( rank < extras )
				{
					/* The first part is homogeneous with a chunk size a little bit larger */
					*pupper = *plower + ( rank + 1 ) * ( chunk_size + 1 ) * incr - incr;
					*plower = *plower + rank * ( chunk_size + 1 ) * incr;
				}
				else
				{
					/* The final part has a smaller chunk_size, therefore the computation is
					   splitted in 2 parts */
					*pupper = *plower + extras * ( chunk_size + 1 ) * incr +
					          ( rank + 1 - extras ) * chunk_size * incr - incr;
					*plower = *plower + extras * ( chunk_size + 1 ) * incr +
					          ( rank - extras ) * chunk_size * incr;
				}

				/* Remarks:
				   - MPC chunk has not-inclusive upper bound while Intel runtime includes
				   upper bound for calculation
				   - Need to cast from long to int because MPC handles everything has a
				   long
				   (like GCC) while Intel creates different functions
				   */
				if ( plastiter != NULL )
				{
					if ( incr > 0 )
					{
						*plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
					}
					else
					{
						*plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
					}
				}
			}

			break;
		}

		case kmp_sch_static_chunked:
		{
			chunk = ( chunk < 1 ) ? 1 : chunk;
			*pstride = ( chunk * incr ) * t->info.num_threads;
			*plower = *plower + ( ( chunk * incr ) * t->rank );
			*pupper = *plower + ( chunk * incr ) - incr;

			// plastiter computation
			if ( plastiter != NULL )
			{
				*plastiter = ( ( unsigned long )t->rank == ( ( trip_count - 1 ) / chunk ) % t->info.num_threads );
			}

			break;
		}

		default:
			not_implemented();
			break;
			sctk_nodebug( "[%d] Results: "
			              "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
			              t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
			              *plastiter );
	}
}

void __kmpc_for_static_fini( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
	sctk_nodebug( "[REDIRECT KMP]: %s -> None", __func__ );
}

/************
 * DISPATCH *
 ************/

static inline void __intel_dispatch_init_long( mpcomp_thread_t *t, long lb,
        long b, long incr,
        long chunk )
{
	sctk_assert( t );

	switch ( t->schedule_type )
	{
		/* NORMAL AUTO OR STATIC */
		case kmp_sch_auto:
		case kmp_sch_static:
			chunk = 0;

		case kmp_sch_static_chunked:
			__mpcomp_static_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		/* ORDERED AUTO OR STATIC */
		case kmp_ord_auto:
		case kmp_ord_static:
			chunk = 0;

		case kmp_ord_static_chunked:
			__mpcomp_ordered_static_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		/* regular scheduling */
		case kmp_sch_dynamic_chunked:
			__mpcomp_dynamic_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_ord_dynamic_chunked:
			__mpcomp_ordered_dynamic_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_sch_guided_chunked:
			__mpcomp_guided_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_ord_guided_chunked:
			__mpcomp_guided_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		/* runtime */
		case kmp_sch_runtime:
			__mpcomp_runtime_loop_begin( lb, b, incr, NULL, NULL );
			break;

		case kmp_ord_runtime:
			__mpcomp_ordered_runtime_loop_begin( lb, b, incr, NULL, NULL );
			break;

		default:
			sctk_error( "Schedule not handled: %d\n", t->schedule_type );
			not_implemented();
	}
}

static inline void __intel_dispatch_init_ull( mpcomp_thread_t *t, bool up,
        unsigned long long lb,
        unsigned long long b,
        unsigned long long incr,
        unsigned long long chunk )
{
	sctk_assert( t );

	switch ( t->schedule_type )
	{
		/* NORMAL AUTO OR STATIC */
		case kmp_sch_auto:
		case kmp_sch_static:
			chunk = 0;

		case kmp_sch_static_chunked:
			__mpcomp_static_loop_begin_ull( up, lb, b, incr, chunk, NULL, NULL );
			break;

		/* ORDERED AUTO OR STATIC */
		case kmp_ord_auto:
		case kmp_ord_static:
			chunk = 0;

		case kmp_ord_static_chunked:
			__mpcomp_ordered_static_loop_begin_ull( up, lb, b, incr, chunk, NULL, NULL );
			break;

		/* regular scheduling */
		case kmp_sch_dynamic_chunked:
			__mpcomp_loop_ull_dynamic_begin( up, lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_ord_dynamic_chunked:
			__mpcomp_loop_ull_ordered_dynamic_begin( up, lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_sch_guided_chunked:
			__mpcomp_loop_ull_guided_begin( up, lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_ord_guided_chunked:
			__mpcomp_loop_ull_guided_begin( up, lb, b, incr, chunk, NULL, NULL );
			break;

		/* runtime */
		case kmp_sch_runtime:
			__mpcomp_loop_ull_runtime_begin( up, lb, b, incr, NULL, NULL );
			break;

		case kmp_ord_runtime:
			__mpcomp_loop_ull_ordered_runtime_begin( up, lb, b, incr, NULL, NULL );
			break;

		default:
			sctk_error( "Schedule not handled: %d\n", t->schedule_type );
			not_implemented();
			break;
	}
}

static inline int __intel_dispatch_next_long( mpcomp_thread_t *t,
        long *from, long *to )
{
	int ret;

	switch ( t->schedule_type )
	{
		/* regular scheduling */
		case kmp_sch_auto:
		case kmp_sch_static:
		case kmp_sch_static_chunked:
			ret = __mpcomp_static_loop_next( from, to );
			sctk_nodebug( " from: %ld -- to: %ld", *from, *to );
			break;

		case kmp_ord_auto:
		case kmp_ord_static:
		case kmp_ord_static_chunked:
			ret = __mpcomp_ordered_static_loop_next( from, to );
			sctk_nodebug( " from: %ld -- to: %ld", *from, *to );
			break;

		/* dynamic */
		case kmp_sch_dynamic_chunked:
			ret = __mpcomp_dynamic_loop_next( from, to );
			break;

		case kmp_ord_dynamic_chunked:
			ret = __mpcomp_ordered_dynamic_loop_next( from, to );
			break;

		/* guided */
		case kmp_sch_guided_chunked:
			ret = __mpcomp_guided_loop_next( from, to );
			break;

		case kmp_ord_guided_chunked:
			ret = __mpcomp_ordered_guided_loop_next( from, to );
			break;

		/* runtime */
		case kmp_sch_runtime:
			ret = __mpcomp_runtime_loop_next( from, to );
			break;

		case kmp_ord_runtime:
			ret = __mpcomp_ordered_runtime_loop_next( from, to );
			break;

		default:
			not_implemented();
			break;
	}

	return ret;
}
static inline int __intel_dispatch_next_ull(
    mpcomp_thread_t *t, unsigned long long *from, unsigned long long *to )
{
	int ret;

	switch ( t->schedule_type )
	{
		/* regular scheduling */
		case kmp_sch_auto:
		case kmp_sch_static:
		case kmp_sch_static_chunked:
			ret = __mpcomp_static_loop_next_ull( from, to );
			break;

		case kmp_ord_auto:
		case kmp_ord_static:
		case kmp_ord_static_chunked:
			ret = __mpcomp_ordered_static_loop_next_ull( from, to );
			break;

		/* dynamic */
		case kmp_sch_dynamic_chunked:
			ret = __mpcomp_loop_ull_dynamic_next( from, to );
			break;

		case kmp_ord_dynamic_chunked:
			ret = __mpcomp_loop_ull_ordered_dynamic_next( from, to );
			break;

		/* guided */
		case kmp_sch_guided_chunked:
			ret = __mpcomp_loop_ull_guided_next( from, to );
			break;

		case kmp_ord_guided_chunked:
			ret = __mpcomp_loop_ull_ordered_guided_next( from, to );
			break;

		/* runtime */
		case kmp_sch_runtime:
			ret = __mpcomp_loop_ull_runtime_next( from, to );
			break;

		case kmp_ord_runtime:
			ret = __mpcomp_loop_ull_ordered_runtime_next( from, to );
			break;

		default:
			not_implemented();
			break;
	}

	return ret;
}

static inline void __intel_dispatch_fini_gen( __UNUSED__ ident_t *loc,
        __UNUSED__ kmp_int32 gtid )
{
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );

	switch ( t->schedule_type )
	{
		case kmp_sch_static:
		case kmp_sch_auto:
		case kmp_sch_static_chunked:
			__mpcomp_static_loop_end_nowait();
			break;

		case kmp_ord_static:
		case kmp_ord_auto:
		case kmp_ord_static_chunked:
			__mpcomp_ordered_static_loop_end_nowait();
			break;

		case kmp_sch_dynamic_chunked:
			__mpcomp_dynamic_loop_end_nowait();
			break;

		case kmp_ord_dynamic_chunked:
			__mpcomp_ordered_dynamic_loop_end_nowait();
			break;

		case kmp_sch_guided_chunked:
			__mpcomp_guided_loop_end_nowait();
			break;

		case kmp_ord_guided_chunked:
			__mpcomp_ordered_guided_loop_end_nowait();
			break;

		case kmp_sch_runtime:
			__mpcomp_runtime_loop_end_nowait();
			break;

		case kmp_ord_runtime:
			__mpcomp_ordered_runtime_loop_end_nowait();
			break;

		default:
			not_implemented();
			break;
	}
}

/* END STAT ST */


void __kmpc_dispatch_init_4( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid,
                             enum sched_type schedule, kmp_int32 lb,
                             kmp_int32 ub, kmp_int32 st, kmp_int32 chunk )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_nodebug( "[%d] %s: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d",
	              t->rank, __func__, lb, ub, ub + st, st, chunk, schedule );
	/* add to sync with MPC runtime bounds */
	const long add = ( ( ub - lb ) % st == 0 ) ? st : st - ( ( ub - lb ) % st );
	const long b = ( long )ub + add;
	t->schedule_type = schedule;
	t->schedule_is_forced = 1;
	__intel_dispatch_init_long( t, lb, b, ( long )st, ( long )chunk );
}

void __kmpc_dispatch_init_4u( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid,
                              enum sched_type schedule, kmp_uint32 lb,
                              kmp_uint32 ub, kmp_int32 st, kmp_int32 chunk )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_nodebug(
	    "[%d] %s: enter %llu -> %llud incl, %llu excl [%llu] ck:%llud sch:%d",
	    t->rank, __func__, lb, ub, ub + st, st, chunk, schedule );
	/* add to sync with MPC runtime bounds */
	const long long add = ( ( ub - lb ) % st == 0 ) ? st : st - ( kmp_int32 )( ( ub - lb ) % st );
	const unsigned long long b = ( unsigned long long )ub + add;
	const bool up = ( st > 0 );
	const unsigned long long st_ull = ( up ) ? st : -st;
	t->schedule_type = schedule;
	t->schedule_is_forced = 1;
	__intel_dispatch_init_ull( t, up, lb, b, ( unsigned long long )st_ull,
	                           ( unsigned long long )chunk );
}

void __kmpc_dispatch_init_8( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid,
                             enum sched_type schedule, kmp_int64 lb,
                             kmp_int64 ub, kmp_int64 st, kmp_int64 chunk )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_nodebug( "[%d] %s: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d",
	              t->rank, __func__, lb, ub, ub + st, st, chunk, schedule );
	/* add to sync with MPC runtime bounds */
	const long add = ( ( ub - lb ) % st == 0 ) ? st : st - ( ( ub - lb ) % st );
	const long b = ( long )ub + add;
	t->schedule_type = schedule;
	t->schedule_is_forced = 1;
	__intel_dispatch_init_long( t, lb, b, ( long )st, ( long )chunk );
}

void __kmpc_dispatch_init_8u( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid,
                              enum sched_type schedule, kmp_uint64 lb,
                              kmp_uint64 ub, kmp_int64 st, kmp_int64 chunk )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t );
	sctk_nodebug(
	    "[%d] %s: enter %llu -> %llud incl, %llu excl [%llu] ck:%llud sch:%d",
	    t->rank, __func__, lb, ub, ub + st, st, chunk, schedule );
	/* add to sync with MPC runtime bounds */
	const bool up = ( st > 0 );
	const unsigned long long st_ull = ( up ) ? st : -st;
	const long long add = ( ( ub - lb ) % st == 0 ) ? st : st - ( kmp_int32 )( ( ub - lb ) % st );
	const unsigned long long b = ( unsigned long long )ub + add;
	t->schedule_type = schedule;
	t->schedule_is_forced = 1;
	__intel_dispatch_init_ull( t, up, lb, b, ( unsigned long long )st_ull,
	                           ( unsigned long long )chunk );
}

int __kmpc_dispatch_next_4( ident_t *loc, kmp_int32 gtid, __UNUSED__ kmp_int32 *p_last,
                            kmp_int32 *p_lb, kmp_int32 *p_ub, __UNUSED__  kmp_int32 *p_st )
{
	long from, to;
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	sctk_assert( t->info.loop_infos.type == MPCOMP_LOOP_TYPE_LONG );
	const long incr = t->info.loop_infos.loop.mpcomp_long.incr;
	sctk_nodebug( "%s: p_lb %ld, p_ub %ld, p_st %ld", __func__, *p_lb, *p_ub,
	              *p_st );
	const int ret = __intel_dispatch_next_long( t, &from, &to );
	/* sync with intel runtime (A->B excluded with gcc / A->B included with intel)
	 */
	*p_lb = ( kmp_int32 )from;
	*p_ub = ( kmp_int32 )to - ( kmp_int32 )incr;

	if ( !ret )
	{
		__intel_dispatch_fini_gen( loc, gtid );
	}

	return ret;
}

int __kmpc_dispatch_next_4u( ident_t *loc, kmp_int32 gtid, __UNUSED__ kmp_int32 *p_last,
                             kmp_uint32 *p_lb, kmp_uint32 *p_ub,
                             __UNUSED__ kmp_int32 *p_st )
{
	mpcomp_thread_t *t;
	unsigned long long from, to;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	sctk_assert( t->info.loop_infos.type == MPCOMP_LOOP_TYPE_ULL );
	const unsigned long long incr = t->info.loop_infos.loop.mpcomp_ull.incr;
	sctk_nodebug( "%s: p_lb %llu, p_ub %llu, p_st %llu", __func__, *p_lb, *p_ub,
	              *p_st );
	const int ret = __intel_dispatch_next_ull( t, &from, &to );
	/* sync with intel runtime (A->B excluded with gcc / A->B included with intel)
	 */
	*p_lb = ( kmp_uint32 )from;
	*p_ub = ( kmp_uint32 )to - ( kmp_uint32 )incr;

	if ( !ret )
	{
		__intel_dispatch_fini_gen( loc, gtid );
	}

	return ret;
}

int __kmpc_dispatch_next_8( ident_t *loc, kmp_int32 gtid, __UNUSED__ kmp_int32 *p_last,
                            kmp_int64 *p_lb, kmp_int64 *p_ub, __UNUSED__ kmp_int64 *p_st )
{
	long from, to;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	sctk_assert( t->info.loop_infos.type == MPCOMP_LOOP_TYPE_LONG );
	const long incr = t->info.loop_infos.loop.mpcomp_long.incr;
	sctk_nodebug( "%s: p_lb %ld, p_ub %ld, p_st %ld", __func__, *p_lb, *p_ub,
	              *p_st );
	const int ret = __intel_dispatch_next_long( t, &from, &to );
	/* sync with intel runtime (A->B excluded with gcc / A->B included with intel)
	 */
	*p_lb = ( kmp_int64 )from;
	*p_ub = ( kmp_int64 )to - ( kmp_int64 )incr;

	if ( !ret )
	{
		__intel_dispatch_fini_gen( loc, gtid );
	}

	return ret;
}

int __kmpc_dispatch_next_8u( ident_t *loc, kmp_int32 gtid, __UNUSED__ kmp_int32 *p_last,
                             kmp_uint64 *p_lb, kmp_uint64 *p_ub,
                             __UNUSED__ kmp_int64 *p_st )
{
	mpcomp_thread_t *t;
	unsigned long long from, to;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	sctk_assert( t->info.loop_infos.type == MPCOMP_LOOP_TYPE_ULL );
	const unsigned long long incr = t->info.loop_infos.loop.mpcomp_ull.incr;
	sctk_nodebug( "%s: p_lb %llu, p_ub %llu, p_st %llu", __func__, *p_lb, *p_ub,
	              *p_st );
	const int ret = __intel_dispatch_next_ull( t, &from, &to );
	/* sync with intel runtime (A->B excluded with gcc / A->B included with intel)
	 */
	*p_lb = from;
	*p_ub = to - ( kmp_uint64 )incr;

	if ( !ret )
	{
		__intel_dispatch_fini_gen( loc, gtid );
	}

	return ret;
}

void __kmpc_dispatch_fini_4( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid ) {}

void __kmpc_dispatch_fini_8( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid ) {}

void __kmpc_dispatch_fini_4u( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid ) {}

void __kmpc_dispatch_fini_8u( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid ) {}

/*********
 * ALLOC *
 *********/

void *kmpc_malloc( __UNUSED__ size_t size )
{
	not_implemented();
	return NULL;
}

void *kmpc_calloc( __UNUSED__ size_t nelem, __UNUSED__ size_t elsize )
{
	not_implemented();
	return NULL;
}

void *kmpc_realloc( __UNUSED__ void *ptr, __UNUSED__ size_t size )
{
	not_implemented();
	return NULL;
}

void kmpc_free( __UNUSED__ void *ptr )
{
	not_implemented();
}


/*********
 * TASKQ *
 *********/

kmpc_thunk_t *__kmpc_taskq( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                            __UNUSED__  kmpc_task_t taskq_task, __UNUSED__ size_t sizeof_thunk,
                            __UNUSED__  size_t sizeof_shareds, __UNUSED__ kmp_int32 flags,
                            __UNUSED__ kmpc_shared_vars_t **shareds )
{
	not_implemented();
	return ( kmpc_thunk_t * )NULL;
}

void __kmpc_end_taskq( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 global_tid, __UNUSED__ kmpc_thunk_t *thunk )
{
	not_implemented();
}

kmp_int32 __kmpc_task( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid, __UNUSED__ kmpc_thunk_t *thunk )
{
	not_implemented();
	return ( kmp_int32 )0;
}

void __kmpc_taskq_task( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid, __UNUSED__ kmpc_thunk_t *thunk,
                        __UNUSED__ kmp_int32 status )
{
	not_implemented();
}

void __kmpc_end_taskq_task( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                            __UNUSED__ kmpc_thunk_t *thunk )
{
	not_implemented();
}

kmpc_thunk_t *__kmpc_task_buffer( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                                  __UNUSED__ kmpc_thunk_t *taskq_thunk, __UNUSED__ kmpc_task_t task )
{
	not_implemented();
	return ( kmpc_thunk_t * )NULL;
}

/***********
 * TASKING *
 ***********/

#if MPCOMP_TASK
kmp_int32 __kmpc_omp_task( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                           kmp_task_t *new_task )
{
	struct mpcomp_task_s *mpcomp_task =
	    ( struct mpcomp_task_s * ) ( ( char * )new_task - sizeof( struct mpcomp_task_s ) );
	// TODO: Handle final clause
	_mpc_task_process( mpcomp_task, true );
	return ( kmp_int32 )0;
}

void __kmp_omp_task_wrapper( void *task_ptr )
{
	kmp_task_t *kmp_task_ptr = ( kmp_task_t * )task_ptr;
	kmp_task_ptr->routine( 0, kmp_task_ptr );
}

kmp_task_t *__kmpc_omp_task_alloc( __UNUSED__ ident_t *loc_ref, __UNUSED__  kmp_int32 gtid,
                                   kmp_int32 flags, size_t sizeof_kmp_task_t,
                                   size_t sizeof_shareds,
                                   kmp_routine_entry_t task_entry )
{
	sctk_assert( sctk_openmp_thread_tls );
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	__mpcomp_init();
	/* default pading */
	const long align_size = sizeof( void * );
	// mpcomp_task + arg_size
	const long mpcomp_kmp_taskdata = sizeof( mpcomp_task_t ) + sizeof_kmp_task_t;
	const long mpcomp_task_info_size =
	    _mpc_task_align_single_malloc( mpcomp_kmp_taskdata, align_size );
	const long mpcomp_task_data_size =
	    _mpc_task_align_single_malloc( sizeof_shareds, align_size );
	/* Compute task total size */
	long mpcomp_task_tot_size = mpcomp_task_info_size;
	sctk_assert( MPCOMP_OVERFLOW_SANITY_CHECK( ( unsigned long )mpcomp_task_tot_size,
	             ( unsigned long )mpcomp_task_data_size ) );
	mpcomp_task_tot_size += mpcomp_task_data_size;
	struct mpcomp_task_s *new_task ;

	/* Reuse another task if we can to avoid alloc overhead */
	if ( t->task_infos.one_list_per_thread )
	{
		if ( mpcomp_task_tot_size > t->task_infos.max_task_tot_size )
			/* Tasks stored too small, free them */
		{
			t->task_infos.max_task_tot_size = mpcomp_task_tot_size;
			int i;

			for ( i = 0; i < t->task_infos.nb_reusable_tasks; i++ )
			{
				sctk_free( t->task_infos.reusable_tasks[i] );
				t->task_infos.reusable_tasks[i] = NULL;
			}

			t->task_infos.nb_reusable_tasks = 0;
		}

		if ( t->task_infos.nb_reusable_tasks == 0 )
		{
			new_task = ( struct mpcomp_task_s * ) mpcomp_alloc( t->task_infos.max_task_tot_size );
		}
		/* Reuse last task stored */
		else
		{
			sctk_assert( t->task_infos.reusable_tasks[t->task_infos.nb_reusable_tasks - 1] );
			new_task = ( mpcomp_task_t * ) t->task_infos.reusable_tasks[t->task_infos.nb_reusable_tasks - 1];
			t->task_infos.nb_reusable_tasks --;
		}
	}
	else
	{
		new_task = ( struct mpcomp_task_s * ) mpcomp_alloc( mpcomp_task_tot_size );
	}

	sctk_assert( new_task != NULL );
	void *task_data = ( sizeof_shareds > 0 )
	                  ? ( void * )( ( uintptr_t )new_task + mpcomp_task_info_size )
	                  : NULL;
	/* Create new task */
	kmp_task_t *compiler_infos =
	    ( kmp_task_t * )( ( char * )new_task + sizeof( struct mpcomp_task_s ) );
	_mpc_task_info_init( new_task, __kmp_omp_task_wrapper, compiler_infos, t );
	/* MPCOMP_USE_TASKDEP */
	compiler_infos->shareds = task_data;
	compiler_infos->routine = task_entry;
	compiler_infos->part_id = 0;
	/* taskgroup */
	mpcomp_task_t *current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( t );
	mpcomp_taskgroup_add_task( new_task );
	_mpc_task_ref_parent_task( new_task );
	new_task->is_stealed = false;
	new_task->task_size = t->task_infos.max_task_tot_size;

	if ( new_task->depth % MPCOMP_TASKS_DEPTH_JUMP == 2 )
	{
		new_task->far_ancestor = new_task->parent;
	}
	else
	{
		new_task->far_ancestor = new_task->parent->far_ancestor;
	}

	t->task_infos.sizeof_kmp_task_t = sizeof_kmp_task_t;

	/* If its parent task is final, the new task must be final too */
	if ( _mpc_task_is_final( flags, new_task->parent ) )
	{
		mpcomp_task_set_property( &( new_task->property ), MPCOMP_TASK_FINAL );
	}

	/* to handle if0 with deps */
	current_task->last_task_alloc = new_task;
	return compiler_infos;
}

void __kmpc_omp_task_begin_if0( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                                kmp_task_t *task )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	struct mpcomp_task_s *mpcomp_task = ( struct mpcomp_task_s * ) (
	                                        ( char * )task - sizeof( struct mpcomp_task_s ) );
	sctk_assert( t );
	mpcomp_task->icvs = t->info.icvs;
	mpcomp_task->prev_icvs = t->info.icvs;
	/* Because task code is between the current call and the next call
	 * __kmpc_omp_task_complete_if0 */
	MPCOMP_TASK_THREAD_SET_CURRENT_TASK( t, mpcomp_task );
}

void __kmpc_omp_task_complete_if0( __UNUSED__ ident_t *loc_ref, __UNUSED__  kmp_int32 gtid,
                                   kmp_task_t *task )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	struct mpcomp_task_s *mpcomp_task = ( struct mpcomp_task_s * )
	                                    ( ( char * )task - sizeof( struct mpcomp_task_s ) );
	sctk_assert( t );
	_mpc_task_dep_new_finalize( mpcomp_task ); /* if task if0 with deps */
	mpcomp_taskgroup_del_task( mpcomp_task );
	_mpc_task_unref_parent_task( mpcomp_task );
	MPCOMP_TASK_THREAD_SET_CURRENT_TASK( t, mpcomp_task->parent );
	t->info.icvs = mpcomp_task->prev_icvs;
}

kmp_int32 __kmpc_omp_task_parts( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                                 kmp_task_t *new_task )
{
	// TODO: Check if this is the correct way to implement __kmpc_omp_task_parts
	struct mpcomp_task_s *mpcomp_task = ( struct mpcomp_task_s * )
	                                    ( ( char * )new_task - sizeof( struct mpcomp_task_s ) );
	_mpc_task_process( mpcomp_task, true );
	return ( kmp_int32 )0;
}

kmp_int32 __kmpc_omp_taskwait( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid )
{
	_mpc_task_wait();
	return ( kmp_int32 )0;
}

kmp_int32 __kmpc_omp_taskyield( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, __UNUSED__ int end_part )
{
	//not_implemented();
	return ( kmp_int32 )0;
}

/* Following 2 functions should be enclosed by "if TASK_UNUSED" */

void __kmpc_omp_task_begin( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, __UNUSED__ kmp_task_t *task )
{
	//not_implemented();
}

void __kmpc_omp_task_complete( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                               __UNUSED__ kmp_task_t *task )
{
	//not_implemented();
}

/* FOR OPENMP 4 */

void __kmpc_taskgroup( __UNUSED__ ident_t *loc, __UNUSED__ int gtid )
{
	_mpc_task_taskgroup_start();
}

void __kmpc_end_taskgroup( __UNUSED__ ident_t *loc, __UNUSED__ int gtid )
{
	_mpc_task_taskgroup_end();
}

static void mpcomp_intel_translate_taskdep_to_gomp(  kmp_int32 ndeps, kmp_depend_info_t *dep_list, kmp_int32 ndeps_noalias, __UNUSED__ kmp_depend_info_t *noalias_dep_list, void **gomp_list_deps )
{
	int i;
	long long int num_out_dep = 0;
	int num_in_dep = 0;
	long long int nd = ( ndeps + ndeps_noalias );
	gomp_list_deps[0] = ( void * )( uintptr_t )nd;
	uintptr_t dep_not_out[nd];

	for ( i = 0; i < nd ; i++ )
	{
		if ( dep_list[i].base_addr != 0 )
		{
			if ( dep_list[i].flags.out )
			{
				long long int ibaseaddr = dep_list[i].base_addr;
				gomp_list_deps[num_out_dep + 2] = ( void * )ibaseaddr;
				num_out_dep++;
			}
			else
			{
				dep_not_out[num_in_dep] = ( uintptr_t )dep_list[i].base_addr;
				num_in_dep++;
			}
		}
	}

	gomp_list_deps[1] = ( void * )num_out_dep;

	for ( i = 0; i < num_in_dep; i++ )
	{
		gomp_list_deps[2 + num_out_dep + i] = ( void * )dep_not_out[i];
	}

	return;
}

kmp_int32 __kmpc_omp_task_with_deps( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                                     kmp_task_t *new_task, kmp_int32 ndeps,
                                     kmp_depend_info_t *dep_list,
                                     kmp_int32 ndeps_noalias,
                                     kmp_depend_info_t *noalias_dep_list )
{
	struct mpcomp_task_s *mpcomp_task =
	    ( struct mpcomp_task_s * ) ( ( char * )new_task - sizeof( struct mpcomp_task_s ) );
	void *data = ( void * )mpcomp_task->data;
	long arg_size = 0;
	long arg_align = 0;
	bool if_clause = true; /* if0 task doesn't go here */
	unsigned flags = 8; /* dep flags */
	void **depend;
	depend = ( void ** )sctk_malloc( sizeof( uintptr_t ) * ( ( int )( ndeps + ndeps_noalias ) + 2 ) );
	mpcomp_intel_translate_taskdep_to_gomp( ndeps, dep_list, ndeps_noalias, noalias_dep_list, depend );
	sctk_nodebug( "[Redirect mpcomp_GOMP]%s:\tBegin", __func__ );
	_mpc_task_new_with_deps( mpcomp_task->func, data, NULL, arg_size, arg_align,
	                       if_clause, flags, depend, true, mpcomp_task );
	sctk_nodebug( "[Redirect mpcomp_GOMP]%s:\tEnd", __func__ );
	return ( kmp_int32 )0;
}

void __kmpc_omp_wait_deps( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, kmp_int32 ndeps,
                           kmp_depend_info_t *dep_list, kmp_int32 ndeps_noalias,
                           kmp_depend_info_t *noalias_dep_list )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	mpcomp_task_t *current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( t );
	mpcomp_task_t *task = current_task->last_task_alloc;
	long arg_size = 0;
	long arg_align = 0;
	bool if_clause = false; /* if0 task here */
	unsigned flags = 8; /* dep flags */
	void **depend = ( void ** )sctk_malloc( sizeof( uintptr_t ) * ( ( int )( ndeps + ndeps_noalias ) + 2 ) );
	mpcomp_intel_translate_taskdep_to_gomp( ndeps, dep_list, ndeps_noalias, noalias_dep_list, depend );
	sctk_nodebug( "[Redirect mpcomp_GOMP]%s:\tBegin", __func__ );
	_mpc_task_new_with_deps( NULL, NULL, NULL, arg_size, arg_align,
	                       if_clause, flags, depend, true, task ); /* create the dep node and set the task to the node then return */
	/* next call should be __kmpc_omp_task_begin_if0 to execute undeferred if0 task */
	sctk_nodebug( "[Redirect mpcomp_GOMP]%s:\tEnd", __func__ );
}

void __kmp_release_deps( __UNUSED__ kmp_int32 gtid, __UNUSED__ kmp_taskdata_t *task )
{
	//not_implemented();
}

typedef void( *p_task_dup_t )( kmp_task_t *, kmp_task_t *, kmp_int32 );
void __kmpc_taskloop( ident_t *loc, int gtid, kmp_task_t *task, __UNUSED__ int if_val,
                      kmp_uint64 *lb, kmp_uint64 *ub, kmp_int64 st,
                      int nogroup, int sched, kmp_uint64 grainsize, void *task_dup )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	unsigned long i, trip_count, num_tasks = 0, extras = 0, lower = *lb, upper = *ub;
	kmp_task_t *next_task;
	int lastpriv;
	kmp_taskdata_t *taskdata_src, *taskdata;
	size_t lower_offset = ( char * )lb - ( char * )task;
	size_t upper_offset = ( char * )ub - ( char * )task;
	size_t shareds_offset, sizeof_kmp_task_t, sizeof_shareds;
	p_task_dup_t ptask_dup = ( p_task_dup_t )task_dup;

	if ( nogroup == 0 )
	{
		_mpc_task_taskgroup_start();
	}

	if ( st == 1 )     // most common case
	{
		trip_count = upper - lower + 1;
	}
	else if ( st < 0 )
	{
		trip_count = ( lower - upper ) / ( -st ) + 1;
	}
	else           // st > 0
	{
		trip_count = ( upper - lower ) / st + 1;
	}

	if ( !( trip_count ) )
	{
		return;
	}

	switch ( sched )
	{
		case 0:
			grainsize = t->info.num_threads * 10;

		case 2:
			if ( grainsize > trip_count )
			{
				num_tasks = trip_count;
				grainsize = 1;
				extras = 0;
			}
			else
			{
				num_tasks = grainsize;
				grainsize = trip_count / num_tasks;
				extras = trip_count % num_tasks;
			}

			break;

		case 1:
			if ( grainsize > trip_count )
			{
				num_tasks = 1;
				grainsize = trip_count;
				extras = 0;
			}
			else
			{
				num_tasks = trip_count / grainsize;
				grainsize = trip_count / num_tasks;
				extras = trip_count % num_tasks;
			}

			break;

		default:
			sctk_nodebug( "Unknown scheduling of taskloop" );
	}

	unsigned long chunk;

	if ( extras == 0 )
	{
		chunk = grainsize - 1;
	}
	else
	{
		chunk = grainsize;
		--extras;
	}

	sizeof_shareds = sizeof( kmp_task_t );
	sizeof_kmp_task_t = t->task_infos.sizeof_kmp_task_t;
	taskdata_src = ( ( kmp_taskdata_t * ) task ) - 1;
	shareds_offset = ( char * )task->shareds - ( char * )taskdata_src;
	taskdata = sctk_malloc( sizeof_shareds + shareds_offset );
	memcpy( taskdata, taskdata_src, sizeof_shareds + shareds_offset );

	for ( i = 0; i < num_tasks; ++i )
	{
		upper = lower + st * chunk;

		if ( i == num_tasks - 1 )
		{
			lastpriv = 1;
		}

		next_task = __kmpc_omp_task_alloc( loc, gtid, 1,
		                                   sizeof_kmp_task_t,
		                                   sizeof_shareds, task->routine );

		if ( task->shareds != NULL )  // need setup shareds pointer
		{
			next_task->shareds = &( ( char * )taskdata )[shareds_offset];
		}

		if ( ptask_dup != NULL )
		{
			ptask_dup( next_task, task, lastpriv );
		}

		*( kmp_uint64 * )( ( char * )next_task + lower_offset ) = lower; // adjust task-specific bounds
		*( kmp_uint64 * )( ( char * )next_task + upper_offset ) = upper;
		struct mpcomp_task_s *mpcomp_task =
		    ( struct mpcomp_task_s * ) ( ( char * )next_task - sizeof( struct mpcomp_task_s ) );
		_mpc_task_process( mpcomp_task, true );
		lower = upper + st;
	}

	// Free pattern task without executing it
	struct mpcomp_task_s *mpcomp_taskloop_task =
	    ( struct mpcomp_task_s * ) ( ( char * )task - sizeof( struct mpcomp_task_s ) );
	mpcomp_taskgroup_del_task( mpcomp_taskloop_task );
	_mpc_task_unref_parent_task( mpcomp_taskloop_task );

	if ( nogroup == 0 )
	{
		_mpc_task_taskgroup_end();
	}
}
#endif

/******************
 * THREAD PRIVATE *
 ******************/

struct private_common *
__kmp_threadprivate_find_task_common( struct common_table *tbl, __UNUSED__ int gtid,
                                      void *pc_addr )
{
	struct private_common *tn;

	for ( tn = tbl->data[KMP_HASH( pc_addr )]; tn; tn = tn->next )
	{
		if ( tn->gbl_addr == pc_addr )
		{
			return tn;
		}
	}

	return 0;
}

struct shared_common *__kmp_find_shared_task_common( struct shared_table *tbl,
        __UNUSED__ int gtid, void *pc_addr )
{
	struct shared_common *tn;

	for ( tn = tbl->data[KMP_HASH( pc_addr )]; tn; tn = tn->next )
	{
		if ( tn->gbl_addr == pc_addr )
		{
			return tn;
		}
	}

	return 0;
}

struct private_common *kmp_threadprivate_insert( int gtid, void *pc_addr,
        __UNUSED__ void *data_addr,
        size_t pc_size )
{
	struct private_common *tn, **tt;
	static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	/* critical section */
	sctk_thread_mutex_lock( &lock );
	tn = ( struct private_common * )sctk_malloc( sizeof( struct private_common ) );
	memset( tn, 0, sizeof( struct private_common ) );
	tn->gbl_addr = pc_addr;
	tn->cmn_size = pc_size;

	if ( gtid == 0 )
	{
		tn->par_addr = ( void * )pc_addr;
	}
	else
	{
		tn->par_addr = ( void * )sctk_malloc( tn->cmn_size );
	}

	memcpy( tn->par_addr, pc_addr, pc_size );
	sctk_thread_mutex_unlock( &lock );
	/* end critical section */
	tt = &( t->th_pri_common->data[KMP_HASH( pc_addr )] );
	tn->next = *tt;
	*tt = tn;
	tn->link = t->th_pri_head;
	t->th_pri_head = tn;
	return tn;
}

void *__kmpc_threadprivate( __UNUSED__ ident_t *loc, kmp_int32 global_tid, void *data,
                            size_t size )
{
	void *ret = NULL;
	struct private_common *tn;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	tn = __kmp_threadprivate_find_task_common( t->th_pri_common, global_tid, data );

	if ( !tn )
	{
		tn = kmp_threadprivate_insert( global_tid, data, data, size );
	}
	else
	{
		if ( ( size_t )size > tn->cmn_size )
		{
			sctk_error(
			    "TPCommonBlockInconsist: -> Wong size threadprivate variable found" );
			sctk_abort();
		}
	}

	ret = tn->par_addr;
	return ret;
}

int __kmp_default_tp_capacity()
{
	char *env;
	int nth = 128;
	int __kmp_xproc = 0, req_nproc = 0, r = 0, __kmp_max_nth = 32 * 1024;
	r = sysconf( _SC_NPROCESSORS_ONLN );
	__kmp_xproc = ( r > 0 ) ? r : 2;
	env = getenv( "OMP_NUM_THREADS" );

	if ( env != NULL )
	{
		req_nproc = strtol( env, NULL, 10 );
	}

	if ( nth < ( 4 * req_nproc ) )
	{
		nth = ( 4 * req_nproc );
	}

	if ( nth < ( 4 * __kmp_xproc ) )
	{
		nth = ( 4 * __kmp_xproc );
	}

	if ( nth > __kmp_max_nth )
	{
		nth = __kmp_max_nth;
	}

	return nth;
}

void __kmpc_copyprivate( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid, __UNUSED__ size_t cpy_size,
                         void *cpy_data, void ( *cpy_func )( void *, void * ),
                         kmp_int32 didit )
{
	mpcomp_thread_t *t; /* Info on the current thread */
	void **data_ptr;
	/* Grab the thread info */
	t = ( mpcomp_thread_t * )sctk_openmp_thread_tls;
	sctk_assert( t != NULL );
	/* In this flow path, the number of threads should not be 1 */
	sctk_assert( t->info.num_threads > 0 );
	/* Grab the team info */
	sctk_assert( t->instance != NULL );
	sctk_assert( t->instance->team != NULL );
	data_ptr = &( t->instance->team->single_copyprivate_data );

	if ( didit )
	{
		*data_ptr = cpy_data;
	}

	__mpcomp_barrier();

	if ( !didit )
	{
		( *cpy_func )( cpy_data, *data_ptr );
	}

	__mpcomp_barrier();
}

void *__kmpc_threadprivate_cached( ident_t *loc, kmp_int32 global_tid,
                                   void *data, size_t size, void ***cache )
{
	static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
	int __kmp_tp_capacity = __kmp_default_tp_capacity();

	if ( *cache == 0 )
	{
		sctk_thread_mutex_lock( &lock );

		if ( *cache == 0 )
		{
			// handle cache to be dealt with later
			void **my_cache;
			my_cache = ( void ** )malloc( sizeof( void * ) * __kmp_tp_capacity +
			                              sizeof( kmp_cached_addr_t ) );
			memset( my_cache, 0,
			        sizeof( void * ) * __kmp_tp_capacity + sizeof( kmp_cached_addr_t ) );
			kmp_cached_addr_t *tp_cache_addr;
			tp_cache_addr = ( kmp_cached_addr_t * )( ( char * )my_cache +
			                sizeof( void * ) * __kmp_tp_capacity );
			tp_cache_addr->addr = my_cache;
			tp_cache_addr->next = __kmp_threadpriv_cache_list;
			__kmp_threadpriv_cache_list = tp_cache_addr;
			*cache = my_cache;
		}

		sctk_thread_mutex_unlock( &lock );
	}

	void *ret = NULL;

	if ( ( ret = ( *cache )[global_tid] ) == 0 )
	{
		ret = __kmpc_threadprivate( loc, global_tid, data, ( size_t )size );
		( *cache )[global_tid] = ret;
	}

	return ret;
}

void __kmpc_threadprivate_register( __UNUSED__ ident_t *loc, void *data, kmpc_ctor ctor,
                                    kmpc_cctor cctor, kmpc_dtor dtor )
{
	struct shared_common *d_tn, **lnk_tn;
	/* Only the global data table exists. */
	d_tn = __kmp_find_shared_task_common( &__kmp_threadprivate_d_table, -1, data );

	if ( d_tn == 0 )
	{
		d_tn = ( struct shared_common * )sctk_malloc( sizeof( struct shared_common ) );
		memset( d_tn, 0, sizeof( struct shared_common ) );
		d_tn->gbl_addr = data;
		d_tn->ct.ctor = ctor;
		d_tn->cct.cctor = cctor;
		d_tn->dt.dtor = dtor;
		lnk_tn = &( __kmp_threadprivate_d_table.data[KMP_HASH( data )] );
		d_tn->next = *lnk_tn;
		*lnk_tn = d_tn;
	}
}

void __kmpc_threadprivate_register_vec( __UNUSED__ ident_t *loc, void *data,
                                        kmpc_ctor_vec ctor, kmpc_cctor_vec cctor,
                                        kmpc_dtor_vec dtor,
                                        size_t vector_length )
{
	struct shared_common *d_tn, **lnk_tn;
	d_tn = __kmp_find_shared_task_common(
	           &__kmp_threadprivate_d_table, -1,
	           data ); /* Only the global data table exists. */

	if ( d_tn == 0 )
	{
		d_tn = ( struct shared_common * )sctk_malloc( sizeof( struct shared_common ) );
		memset( d_tn, 0, sizeof( struct shared_common ) );
		d_tn->gbl_addr = data;
		d_tn->ct.ctorv = ctor;
		d_tn->cct.cctorv = cctor;
		d_tn->dt.dtorv = dtor;
		d_tn->is_vec = TRUE;
		d_tn->vec_len = ( size_t )vector_length;
		lnk_tn = &( __kmp_threadprivate_d_table.data[KMP_HASH( data )] );
		d_tn->next = *lnk_tn;
		*lnk_tn = d_tn;
	}
}
