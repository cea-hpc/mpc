#include "omp_gomp.h"

#include "mpc_common_types.h"
#include "mpc_common_debug.h"

#include "mpcomp_abi.h"
#include "mpcomp_sync.h"
#include "mpcomp_core.h"

#include "mpcomp_loop.h"

#include "mpcomp_task.h"
#include "mpcomp_parallel_region.h"


/***********
 * ORDERED *
 ***********/

void mpcomp_GOMP_ordered_start( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_ordered_begin();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_ordered_end( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_ordered_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/***********
 * BARRIER *
 ***********/

void mpcomp_GOMP_barrier( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_barrier();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool GOMP_barrier_cancel( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return false;
}

/**********
 * SINGLE *
 **********/

bool mpcomp_GOMP_single_start( void )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_do_single() ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd -- ret: %s", __func__,
	              ( ret == true ) ? "true" : "false" );
	return ret;
}

void *mpcomp_GOMP_single_copy_start( void )
{
	void *ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __mpcomp_do_single_copyprivate_begin();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpcomp_GOMP_single_copy_end( void *data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_do_single_copyprivate_end( data );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return;
}

/********************
 * WRAPPER FUNCTION *
 ********************/

/**
 * \fn void* mpcomp_GOMP_wrapper_fn(void* args)
 * \brief Transform void(*fn)(void*) GOMP args to void*(*fn)(void*) \
 * unpack mpcomp_gomp_wrapper_t.
 * \param arg opaque type
 */
static inline void *mpcomp_GOMP_wrapper_fn( void *args )
{
	mpcomp_GOMP_wrapper_t *w = ( mpcomp_GOMP_wrapper_t * ) args;
	assert( w );
	assert( w->fn );
	w->fn( w->data );
	return NULL;
}

/*******************
 * PARALLEL REGION *
 *******************/

static inline void __gomp_in_order_scheduler_master_begin( mpcomp_mvp_t *mvp )
{
	mpcomp_thread_t *t;
	/* Switch OpenMP TLS */
	sctk_openmp_thread_tls = &( mvp->threads[0] );
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t->instance != NULL );
	assert( t->instance->team != NULL );
	assert( t->info.func != NULL );
	t->schedule_type = ( long ) t->info.combined_pragma;

	/* Handle beginning of combined parallel region */
	switch ( t->info.combined_pragma )
	{
		case MPCOMP_COMBINED_NONE:
			mpc_common_nodebug( "%s:\nBEGIN - No combined parallel", __func__ );
			break;

		case MPCOMP_COMBINED_SECTION:
			mpc_common_nodebug( "%s:\n BEGIN - Combined parallel/sections w/ %d section(s)",
			              __func__, t->info.nb_sections );
			__mpcomp_sections_init( t, t->info.nb_sections );
			break;

		case MPCOMP_COMBINED_STATIC_LOOP:
			mpc_common_nodebug( "%s:\tBEGIN - Combined parallel/loop", __func__ );
			__mpcomp_static_loop_init( t, t->info.loop_lb, t->info.loop_b,
			                           t->info.loop_incr, t->info.loop_chunk_size );
			break;

		case MPCOMP_COMBINED_DYN_LOOP:
			mpc_common_nodebug( "%s:\tBEGIN - Combined parallel/loop", __func__ );
			__mpcomp_dynamic_loop_init( t, t->info.loop_lb, t->info.loop_b,
			                            t->info.loop_incr, t->info.loop_chunk_size );
			break;

		default:
			not_implemented();
			break;
	}
}

static inline void __gomp_in_order_scheduler_master_end( void )
{
	mpcomp_thread_t *t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );

	/* Handle ending of combined parallel region */
	switch ( t->info.combined_pragma )
	{
		case MPCOMP_COMBINED_NONE:
			mpc_common_nodebug( "%s:\tEND - No combined parallel", __func__ );
			break;

		case MPCOMP_COMBINED_SECTION:
			mpc_common_nodebug( "%s:\tEND - Combined parallel/sections w/ %d section(s)",
			              __func__, t->info.nb_sections );
			break;

		case MPCOMP_COMBINED_STATIC_LOOP:
			mpc_common_nodebug( "%s:\tEND - Combined parallel/loop", __func__ );
			break;

		case MPCOMP_COMBINED_DYN_LOOP:
			mpc_common_nodebug( "%s:\tEND - Combined parallel/loop", __func__ );
			__mpcomp_dynamic_loop_end_nowait();
			break;

		default:
			not_implemented();
			break;
	}

	//t->done = 1;
	// _mpc_task_wait();
	/* Restore previous TLS */
	sctk_openmp_thread_tls = t->father;
}

static inline void __gomp_start_parallel( void ( *fn )( void * ), void *data,
        unsigned num_threads,
        __UNUSED__ unsigned int flags )
{
	__mpcomp_start_parallel_region( ( void ( * )( void * ) ) fn, data, num_threads );
}

static inline void __gomp_start_parallel_region( void ( *fn )( void * ), void *data,
        unsigned num_threads )
{
	mpcomp_thread_t *t;
	mpcomp_parallel_region_t *info;
	/* Initialize OpenMP environment */
	__mpcomp_init();
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	/* Prepare MPC OpenMP parallel region infos */
	info = sctk_malloc( sizeof( mpcomp_parallel_region_t ) );
	assert( info );
	__mpcomp_parallel_region_infos_init( info );
	info->func = ( void *( * ) ( void * ) ) fn;
	info->shared = data;
	info->combined_pragma = MPCOMP_COMBINED_NONE;
	/* Begin scheduling */
	__mpcomp_internal_begin_parallel_region( info, num_threads );
	/* Start scheduling */
	mpcomp_mvp_t *mvp = t->children_instance->mvps[0].ptr.mvp;
	__gomp_in_order_scheduler_master_begin( mvp );
	return;
}

static inline void __gomp_end_parallel_region( void )
{
	__gomp_in_order_scheduler_master_end();
	mpcomp_thread_t *t = sctk_openmp_thread_tls;
	assert( t != NULL );
	mpcomp_instance_t *instance = t->children_instance;
	assert( instance != NULL );
	__mpcomp_internal_end_parallel_region( instance );
}

/* GOMP Parallel Region */

void mpcomp_GOMP_parallel( void ( *fn )( void * ), void *data, unsigned num_threads,
                           unsigned flags )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_start_parallel( fn, data, num_threads, flags );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_parallel_start( void ( *fn )( void * ), void *data,
                                 unsigned num_threads )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_start_parallel_region( fn, data, num_threads );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_parallel_end( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_end_parallel_region();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpcomp_GOMP_cancellation_point( __UNUSED__ int which )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return false;
}

bool mpcomp_GOMP_cancel( __UNUSED__ int which, __UNUSED__ bool do_cancel )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return false;
}

/********
 * LOOP *
 ********/

static inline void __gomp_parallel_loop_generic_start(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, long combined_pragma )
{
	mpcomp_thread_t *t;
	mpcomp_parallel_region_t *info;
	/* Initialize OpenMP environment */
	__mpcomp_init();
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	info = sctk_malloc( sizeof( mpcomp_parallel_region_t ) );
	assert( info );
	__mpcomp_parallel_region_infos_init( info );
	__mpcomp_parallel_set_specific_infos( info, ( void *( * ) ( void * ) ) fn, data,
	                                      t->info.icvs, combined_pragma );
	__mpcomp_loop_gen_infos_init( &( t->info.loop_infos ), start, end, incr,
	                              chunk_size );
	__mpcomp_internal_begin_parallel_region( info, num_threads );
	/* Start scheduling */
	mpcomp_mvp_t *mvp = t->children_instance->mvps[0].ptr.mvp;
	__gomp_in_order_scheduler_master_begin( mvp );
	return;
}

/* Gomp Loop Definition */

/* Ordered Loop */

static inline bool __gomp_loop_runtime_start( long start, long end, long incr,
        long *istart, long *iend )
{
	return ( __mpcomp_runtime_loop_begin( start, end, incr, istart, iend ) ) ? true
	       : false;
}

bool mpcomp_GOMP_loop_runtime_start( long istart, long iend, long incr,
                                     long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_runtime_start( istart, iend, incr, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ordered_runtime_start( long start, long end,
        long incr, long *istart,
        long *iend )
{
	return ( __mpcomp_ordered_runtime_loop_begin( start, end, incr, istart, iend ) )
	       ? true
	       : false;
}

bool mpcomp_GOMP_loop_ordered_runtime_start( long istart, long iend, long incr,
        long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_ordered_runtime_start( istart, iend, incr,
	        start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ordered_runtime_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __mpcomp_ordered_runtime_loop_next( start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ordered_static_start( long start, long end,
        long incr, long chunk_size,
        long *istart, long *iend )
{
	return ( __mpcomp_ordered_static_loop_begin( start, end, incr, chunk_size,
	         istart, iend ) )
	       ? true
	       : false;
}

bool mpcomp_GOMP_loop_ordered_static_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_ordered_static_start( istart, iend, incr,
	                                        chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ordered_static_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_ordered_static_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ordered_dynamic_start( long start, long end,
        long incr, long chunk_size,
        long *istart, long *iend )
{
	return ( __mpcomp_ordered_dynamic_loop_begin( start, end, incr, chunk_size,
	         istart, iend ) )
	       ? true
	       : false;
}

bool mpcomp_GOMP_loop_ordered_dynamic_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_ordered_dynamic_start( istart, iend, incr,
	        chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ordered_dynamic_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_ordered_dynamic_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ordered_guided_start( long start, long end,
        long incr, long chunk_size,
        long *istart, long *iend )
{
	return ( __mpcomp_ordered_guided_loop_begin( start, end, incr, chunk_size,
	         istart, iend ) )
	       ? true
	       : false;
}

bool mpcomp_GOMP_loop_ordered_guided_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_ordered_guided_start( istart, iend, incr,
	                                        chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ordered_guided_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_ordered_guided_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* Static */

void mpcomp_GOMP_parallel_loop_static_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size )
{
	__gomp_parallel_loop_generic_start(
	    fn, data, num_threads, start, end, incr, chunk_size,
	    ( long ) MPCOMP_COMBINED_STATIC_LOOP );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

static inline bool __gomp_loop_static_start( long start, long end, long incr,
        long chunk_size, long *istart,
        long *iend )
{
	return ( __mpcomp_static_loop_begin( start, end, incr, chunk_size, istart,
	                                     iend ) )
	       ? true
	       : false;
}

bool mpcomp_GOMP_loop_static_start( long istart, long iend, long incr,
                                    long chunk_size, long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_static_start( istart, iend, incr, chunk_size,
	                                start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_static_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_static_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* Dynamic */

void mpcomp_GOMP_parallel_loop_dynamic_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin\tSTART:%ld", __func__, start );
	__gomp_parallel_loop_generic_start(
	    fn, data, num_threads, start, end, incr, chunk_size,
	    ( long ) MPCOMP_COMBINED_DYN_LOOP );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

static inline bool __gomp_loop_dynamic_start( long start, long end, long incr,
        long chunk_size, long *istart,
        long *iend )
{
	return ( __mpcomp_dynamic_loop_begin( start, end, incr, chunk_size, istart,
	                                      iend ) )
	       ? true
	       : false;
}

bool mpcomp_GOMP_loop_dynamic_start( long istart, long iend, long incr,
                                     long chunk_size, long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_dynamic_start( istart, iend, incr, chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_dynamic_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_dynamic_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* Guided */

static inline void __gomp_parallel_loop_guided_start(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size )
{
	// TODO Implemented guided algorithm, Guided fallback to dynamic loop in
	// mpcomp
	__gomp_parallel_loop_generic_start(
	    fn, data, num_threads, start, end, incr, chunk_size,
	    ( long ) MPCOMP_COMBINED_GUIDED_LOOP );
}

void mpcomp_GOMP_parallel_loop_guided_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_parallel_loop_guided_start( fn, data, num_threads, start,
	                                   end, incr, chunk_size );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

static inline bool __gomp_loop_guided_start( long start, long end, long incr,
        long chunk_size, long *istart,
        long *iend )
{
	return ( __mpcomp_guided_loop_begin( start, end, incr, chunk_size, istart,
	                                     iend ) )
	       ? true
	       : false;
}

bool mpcomp_GOMP_loop_guided_start( long istart, long iend, long incr,
                                    long chunk_size, long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_guided_start( istart, iend, incr, chunk_size,
	                                start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_guided_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_guided_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* Parallel loop */

static inline void __gomp_parallel_loop_runtime_start( void ( *fn )( void * ),
        void *data,
        unsigned num_threads,
        long start, long end,
        long incr )
{
	long combined_pragma, chunk_size;
	mpcomp_thread_t *t; /* Info on the current thread */
	/* Handle orphaned directive (initialize OpenMP environment) */
	__mpcomp_init();
	/* Grab the thread info */
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );

	switch ( t->info.icvs.run_sched_var )
	{
		case omp_sched_static:
			combined_pragma = ( long ) MPCOMP_COMBINED_STATIC_LOOP;
			break;

		case omp_sched_dynamic:
		case omp_sched_guided:
			combined_pragma = ( long ) MPCOMP_COMBINED_GUIDED_LOOP;
			break;

		default:
			not_reachable();
			break;
	}

	chunk_size = t->info.icvs.modifier_sched_var;
	__gomp_parallel_loop_generic_start(
	    fn, data, num_threads, start, end, incr, chunk_size, combined_pragma );
}

void mpcomp_GOMP_parallel_loop_runtime_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_parallel_loop_runtime_start( fn, data, num_threads, start,
	                                    end, incr );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/* loop nonmonotonic */

void mpcomp_GOMP_parallel_loop_nonmonotonic_dynamic(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, unsigned flags )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_start_parallel_dynamic_loop( fn, data, num_threads, start, end, incr,
			chunk_size );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpcomp_GOMP_loop_nonmonotonic_dynamic_start( long istart, long iend,
        long incr, long chunk_size,
        long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_dynamic_start( istart, iend, incr, chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* loop nonmonotonic */

void mpcomp_GOMP_parallel_loop_nonmonotonic_guided(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, unsigned flags )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_start_parallel_guided_loop( fn, data, num_threads, start, end, incr,
			chunk_size );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpcomp_GOMP_loop_nonmonotonic_guided_start( long istart, long iend,
        long incr, long chunk_size,
        long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_guided_start( istart, iend, incr, chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_nonmonotonic_dynamic_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_dynamic_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_nonmonotonic_guided_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( __mpcomp_guided_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* Common loop ops */

static inline void __gomp_loop_end( void )
{
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );

	switch ( t->schedule_type )
	{
		case MPCOMP_COMBINED_STATIC_LOOP:
		{
			__mpcomp_static_loop_end();
			break;
		}

		case MPCOMP_COMBINED_DYN_LOOP:
		{
			__mpcomp_dynamic_loop_end();
			break;
		}

		case MPCOMP_COMBINED_GUIDED_LOOP:
		{
			__mpcomp_guided_loop_end();
			break;
		}

		case MPCOMP_COMBINED_RUNTIME_LOOP:
		{
			__mpcomp_runtime_loop_end();
			break;
		}

		default:
		{
			not_implemented();
			//__mpcomp_static_loop_end();
			//__mpcomp_dynamic_loop_end();
		}
	}
}

static inline void __gomp_loop_end_nowait( void )
{
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );

	switch ( t->schedule_type )
	{
		case MPCOMP_COMBINED_STATIC_LOOP:
		{
			__mpcomp_static_loop_end_nowait();
			break;
		}

		case MPCOMP_COMBINED_DYN_LOOP:
		{
			__mpcomp_dynamic_loop_end_nowait();
			break;
		}

		case MPCOMP_COMBINED_GUIDED_LOOP:
		{
			__mpcomp_guided_loop_end_nowait();
			break;
		}

		case MPCOMP_COMBINED_RUNTIME_LOOP:
		{
			__mpcomp_runtime_loop_end_nowait();
			break;
		}

		default:
		{
			not_implemented();
			//__mpcomp_static_loop_end_nowait();
		}
	}
}

bool mpcomp_GOMP_loop_runtime_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __mpcomp_runtime_loop_next( start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpcomp_GOMP_loop_end( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_loop_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpcomp_GOMP_loop_end_cancel( void )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpcomp_GOMP_loop_end_nowait( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_loop_end_nowait();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/* Doacross */

bool mpcomp_GOMP_loop_doacross_static_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	MPCOMP_GOMP_UNUSED_VAR( ncounts );
	MPCOMP_GOMP_UNUSED_VAR( counts );
	MPCOMP_GOMP_UNUSED_VAR( chunk_size );
	MPCOMP_GOMP_UNUSED_VAR( start );
	MPCOMP_GOMP_UNUSED_VAR( end );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_doacross_dynamic_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	MPCOMP_GOMP_UNUSED_VAR( ncounts );
	MPCOMP_GOMP_UNUSED_VAR( counts );
	MPCOMP_GOMP_UNUSED_VAR( chunk_size );
	MPCOMP_GOMP_UNUSED_VAR( start );
	MPCOMP_GOMP_UNUSED_VAR( end );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_doacross_guided_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	MPCOMP_GOMP_UNUSED_VAR( ncounts );
	MPCOMP_GOMP_UNUSED_VAR( counts );
	MPCOMP_GOMP_UNUSED_VAR( chunk_size );
	MPCOMP_GOMP_UNUSED_VAR( start );
	MPCOMP_GOMP_UNUSED_VAR( end );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/** OPENMP 4.0 **/

void mpcomp_GOMP_parallel_loop_static( void ( *fn )( void * ), void *data,
                                       unsigned num_threads, long start,
                                       long end, long incr, long chunk_size,
                                       __UNUSED__ unsigned flags )
{
	__mpcomp_start_parallel_static_loop( fn, data, num_threads, start, end, incr,
	                                     chunk_size );
}

void mpcomp_GOMP_parallel_loop_dynamic( void ( *fn )( void * ), void *data,
                                        unsigned num_threads, long start,
                                        long end, long incr, long chunk_size,
                                        __UNUSED__ unsigned flags )
{
	__mpcomp_start_parallel_dynamic_loop( fn, data, num_threads, start, end, incr,
	                                      chunk_size );
}

void mpcomp_GOMP_parallel_loop_guided( void ( *fn )( void * ), void *data,
                                       unsigned num_threads, long start,
                                       long end, long incr, long chunk_size,
                                       __UNUSED__ unsigned flags )
{
	__mpcomp_start_parallel_guided_loop( fn, data, num_threads, start, end, incr,
	                                     chunk_size );
}

void mpcomp_GOMP_parallel_loop_runtime( void ( *fn )( void * ), void *data,
                                        unsigned num_threads, long start,
                                        long end, long incr, __UNUSED__ unsigned flags )
{
	__mpcomp_start_parallel_runtime_loop( fn, data, num_threads, start, end, incr, 0 );
}

/************
 * LOOP ULL *
 ************/

/* Ull loop Gomp interface */

#define MPC_SUPPORT_ULL_LOOP

static inline bool __gomp_loop_ull_runtime_start( bool up,
        unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	t->schedule_type = MPCOMP_COMBINED_RUNTIME_LOOP;
	ret = ( __mpcomp_loop_ull_runtime_begin( up, start, end, incr, istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpcomp_GOMP_loop_ull_runtime_start( bool up, unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = __mpcomp_loop_ull_runtime_begin( up, start, end, incr, istart, iend );
	TODO( "CHECK why correct schedule is not used" );
	//ret = __gomp_loop_ull_runtime_start(up, start, end, incr, istart, iend);
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ull_ordered_runtime_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long *istart,
    unsigned long long *iend )
{
	bool ret;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	t->schedule_type = MPCOMP_COMBINED_RUNTIME_LOOP;
	ret = ( __mpcomp_loop_ull_ordered_runtime_begin( up, start, end, incr, istart,
	        iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_runtime_start( bool up,
        unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = __gomp_loop_ull_ordered_runtime_start( up, start, end,
	        incr, istart, iend );
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_doacross_runtime_start( __UNUSED__ unsigned ncounts,
        __UNUSED__ unsigned long long *counts,
        __UNUSED__ unsigned long long *istart,
        __UNUSED__ unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = false;
	not_implemented();
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_runtime_next( unsigned long long *istart,
                                        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = __mpcomp_loop_ull_runtime_next( istart, iend );
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_runtime_next( unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = __mpcomp_loop_ull_ordered_runtime_next( istart, iend );
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ull_static_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	t->schedule_type = MPCOMP_COMBINED_STATIC_LOOP;
	ret = ( __mpcomp_static_loop_begin_ull( up, start, end, incr, chunk_size,
	                                        istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpcomp_GOMP_loop_ull_static_start( bool up, unsigned long long start,
                                        unsigned long long end,
                                        unsigned long long incr,
                                        unsigned long long chunk_size,
                                        unsigned long long *istart,
                                        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = __gomp_loop_ull_static_start( up, start, end, incr,
	                                    chunk_size, istart, iend );
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ull_dynamic_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	t->schedule_type = MPCOMP_COMBINED_DYN_LOOP;
	ret = ( __mpcomp_loop_ull_dynamic_begin( up, start, end, incr, chunk_size,
	        istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpcomp_GOMP_loop_ull_dynamic_start( bool up, unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
#ifdef MPC_SUPPORT_ULL_LOOP
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin - start: %llu - end: %llu - step: "
	              "%llu - up: %d",
	              __func__, start, end, incr, up );
	TODO( "CHECK why this was changed" );
	ret = __mpcomp_loop_ull_dynamic_begin( up, start, end, incr, chunk_size,
	                                       istart, iend );
	// ret =
	// __gomp_loop_ull_dynamic_start(up,start,end,incr,chunk_size,istart,iend);
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	return ret;
}

static inline bool __gomp_loop_ull_guided_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	t->schedule_type = MPCOMP_COMBINED_GUIDED_LOOP;
	ret = ( __mpcomp_loop_ull_guided_begin( up, start, end, incr, chunk_size,
	                                        istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpcomp_GOMP_loop_ull_guided_start( bool up, unsigned long long start,
                                        unsigned long long end,
                                        unsigned long long incr,
                                        unsigned long long chunk_size,
                                        unsigned long long *istart,
                                        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	TODO( "CHECK why GUIDED schedule is not used from previous function" );
	ret = __mpcomp_loop_ull_guided_begin( up, start, end, incr, chunk_size, istart,
	                                      iend );
	/*ret = __gomp_loop_ull_guided_start(up, start, end, incr, chunk_size, istart,
	                                   iend);*/
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_start(
    __UNUSED__ bool up, __UNUSED__ unsigned long long start, __UNUSED__ unsigned long long end,
    __UNUSED__ unsigned long long incr, __UNUSED__ unsigned long long chunk_size,
    __UNUSED__ unsigned long long *istart, __UNUSED__ unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#if ( defined( MPC_SUPPORT_ULL_LOOP ) && defined( MPC_SUPPORT_NONMONOTONIC ) )
	ret = false;
	not_implemented();
#else  /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_nonmonotonic_guided_start(
    __UNUSED__ bool up, __UNUSED__ unsigned long long start, __UNUSED__ unsigned long long end,
    __UNUSED__ unsigned long long incr, __UNUSED__ unsigned long long chunk_size,
    __UNUSED__ unsigned long long *istart, __UNUSED__ unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#if ( defined( MPC_SUPPORT_ULL_LOOP ) && defined( MPC_SUPPORT_NONMONOTONIC ) )
	ret = false;
	not_implemented();
#else  /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ull_ordered_static_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	t->schedule_type = MPCOMP_COMBINED_STATIC_LOOP;
	ret = ( __mpcomp_ordered_static_loop_begin_ull( up, start, end, incr,
	        chunk_size, istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_static_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __gomp_loop_ull_ordered_static_start(
	            up, start, end, incr, chunk_size, istart, iend ) )
	      ? true
	      : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ull_ordered_dynamic_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	t->schedule_type = MPCOMP_COMBINED_DYN_LOOP;
	ret = ( __mpcomp_loop_ull_ordered_dynamic_begin( up, start, end, incr,
	        chunk_size, istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_dynamic_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __gomp_loop_ull_ordered_dynamic_start(
	            up, start, end, incr, chunk_size, istart, iend ) )
	      ? true
	      : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ull_ordered_guided_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpcomp_thread_t *t;
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	t->schedule_type = MPCOMP_COMBINED_GUIDED_LOOP;
	ret = ( __mpcomp_loop_ull_ordered_guided_begin( up, start, end, incr,
	        chunk_size, istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_guided_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __gomp_loop_ull_ordered_guided_start(
	            up, start, end, incr, chunk_size, istart, iend ) )
	      ? true
	      : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_doacross_static_start( __UNUSED__ unsigned ncounts,
        __UNUSED__ unsigned long long *counts,
        __UNUSED__ unsigned long long chunk_size,
        __UNUSED__ unsigned long long *istart,
        __UNUSED__ unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = false;
	not_implemented();
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_doacross_dynamic_start( __UNUSED__ unsigned ncounts,
        __UNUSED__ unsigned long long *counts,
        __UNUSED__ unsigned long long chunk_size,
        __UNUSED__ unsigned long long *istart,
        __UNUSED__ unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#if ( defined( MPC_SUPPORT_ULL_LOOP ) && defined( MPC_SUPPORT_DOACCROSS ) )
	ret = false;
	not_implemented();
#else  /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_DOACCROSS */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_DOACCROSS */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_doacross_guided_start( __UNUSED__ unsigned ncounts,
        __UNUSED__ unsigned long long *counts,
        __UNUSED__ unsigned long long chunk_size,
        __UNUSED__ unsigned long long *istart,
        __UNUSED__ unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#if ( defined( MPC_SUPPORT_ULL_LOOP ) && defined( MPC_SUPPORT_DOACCROSS ) )
	ret = false;
	not_implemented();
#else  /* MPC_SUPPORT_ULL_LOOP  && MPC_SUPPORT_DOACCROSS */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_DOACCROSS */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_static_next( unsigned long long *istart,
                                       unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __mpcomp_static_loop_next_ull( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_dynamic_next( unsigned long long *istart,
                                        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __mpcomp_loop_ull_dynamic_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_guided_next( unsigned long long *istart,
                                       unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __mpcomp_loop_ull_guided_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_next( __UNUSED__ unsigned long long *istart,
        __UNUSED__ unsigned long long *iend )
{
	bool ret = false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#if ( defined( MPC_SUPPORT_ULL_LOOP ) && defined( MPC_SUPPORT_NONMONOTONIC ) )
	not_implemented();
#else  /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_nonmonotonic_guided_next( __UNUSED__ unsigned long long *istart,
        __UNUSED__ unsigned long long *iend )
{
	bool ret = false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#if ( defined( MPC_SUPPORT_ULL_LOOP ) && defined( MPC_SUPPORT_NONMONOTONIC ) )
	not_implemented();
#else  /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_static_next( unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __mpcomp_ordered_static_loop_next_ull( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_dynamic_next( unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __mpcomp_loop_ull_ordered_dynamic_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_guided_next( unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( __mpcomp_loop_ull_ordered_guided_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/***********
 * SECTION *
 ***********/

unsigned mpcomp_GOMP_sections_start( unsigned count )
{
	unsigned ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __mpcomp_sections_begin( count );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpcomp_GOMP_sections_end( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	/* Nothing to do */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

unsigned mpcomp_GOMP_sections_next( void )
{
	unsigned ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __mpcomp_sections_next();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpcomp_GOMP_parallel_sections( void ( *fn )( void * ), void *data,
                                    unsigned num_threads, unsigned count,
                                    __UNUSED__ unsigned flags )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_start_sections_parallel_region( fn, data, num_threads, count );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

static inline void __gomp_parallel_section_start( void ( *fn )( void * ),
        void *data,
        unsigned num_threads,
        unsigned count )
{
	mpcomp_thread_t *t;
	mpcomp_parallel_region_t *info;
	mpcomp_GOMP_wrapper_t *wrapper_gomp;
	/* Initialize OpenMP environment */
	__mpcomp_init();
	t = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	assert( t != NULL );
	info = sctk_malloc( sizeof( mpcomp_parallel_region_t ) );
	assert( info );
	__mpcomp_parallel_region_infos_init( info );
	wrapper_gomp = sctk_malloc( sizeof( mpcomp_GOMP_wrapper_t ) );
	assert( wrapper_gomp );
	wrapper_gomp->fn = fn;
	wrapper_gomp->data = data;
	info->func = mpcomp_GOMP_wrapper_fn;
	info->shared = wrapper_gomp;
	info->combined_pragma = MPCOMP_COMBINED_SECTION;
	info->nb_sections = count;
	/* Begin scheduling */
	num_threads = ( num_threads == 0 ) ? 1 : num_threads;
	__mpcomp_internal_begin_parallel_region( info, num_threads );
	/* Start scheduling */
	mpcomp_mvp_t *mvp = t->children_instance->mvps[0].ptr.mvp;
	__gomp_in_order_scheduler_master_begin( mvp );
	return;
}

void mpcomp_GOMP_parallel_sections_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, unsigned count )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_parallel_section_start( fn, data, num_threads, count );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpcomp_GOMP_sections_end_cancel( void )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpcomp_GOMP_sections_end_nowait( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	/* Nothing to do */
	__mpcomp_sections_end_nowait();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/************
 * CRITICAL *
 ************/

void mpcomp_GOMP_critical_start( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_anonymous_critical_begin();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_critical_end( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_anonymous_critical_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_critical_name_start( void **pptr )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_named_critical_begin( pptr );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_critical_name_end( void **pptr )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_named_critical_end( pptr );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_atomic_start( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_atomic_begin();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_atomic_end( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__mpcomp_atomic_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/*********
 * TASKS *
 *********/

#if MPCOMP_USE_TASKDEP
void mpcomp_GOMP_task( void ( *fn )( void * ), void *data,
                       void ( *cpyfn )( void *, void * ), long arg_size,
                       long arg_align, bool if_clause, unsigned flags,
                       void **depend )
#else
void mpcomp_GOMP_task( void ( *fn )( void * ), void *data,
                       void ( *cpyfn )( void *, void * ), long arg_size,
                       long arg_align, bool if_clause, unsigned flags,
                       void **depend, int priority )
#endif
{
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tBegin", __func__ );
#if MPCOMP_USE_TASKDEP
	_mpc_task_new_with_deps( ( void ( * )( void * ) ) fn, data, cpyfn, arg_size, arg_align,
	                       if_clause, flags, depend, false, NULL );
#else
	_mpc_task_new( ( void *( * ) ( void * ) ) fn, data, cpyfn, arg_size, arg_align,
	               if_clause, flags );
#endif
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_taskwait( void )
{
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tBegin", __func__ );
	_mpc_task_wait();
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_taskyield( void )
{
	/* Nothing at the moment.  */
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tBegin", __func__ );
	_mpc_taskyield();
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tEnd", __func__ );
}

#if MPCOMP_TASK
void mpcomp_GOMP_taskgroup_start( void )
{
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tBegin", __func__ );
	_mpc_task_taskgroup_start();
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_taskgroup_end( void )
{
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tBegin", __func__ );
	_mpc_task_taskgroup_end();
	mpc_common_nodebug( "[Redirect mpcomp_GOMP]%s:\tEnd", __func__ );
}
#endif /* MPCOMP_TASK */

/***********
 * TARGETS *
 ***********/

void mpcomp_GOMP_offload_register_ver( __UNUSED__ unsigned version, __UNUSED__ const void *host_table,
                                       __UNUSED__ int target_type,
                                       __UNUSED__ const void *target_data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_offload_register( __UNUSED__ const void *host_table, __UNUSED__ int target_type,
                                   __UNUSED__ const void *target_data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_offload_unregister_ver( __UNUSED__ unsigned version,
        __UNUSED__ const void *host_table, __UNUSED__ int target_type,
        __UNUSED__ const void *target_data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_offload_unregister( __UNUSED__ const void *host_table, __UNUSED__ int target_type,
                                     __UNUSED__ const void *target_data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_target( __UNUSED__ int device, __UNUSED__ void ( *fn )( void * ), __UNUSED__ const void *unused,
                         __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                         __UNUSED__ unsigned char *kinds )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_target_ext( __UNUSED__ int device, __UNUSED__  void ( *fn )( void * ), __UNUSED__ size_t mapnum,
                             __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                             __UNUSED__ unsigned short *kinds, __UNUSED__  unsigned int flags,
                             __UNUSED__ void **depend, __UNUSED__ void **args )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_target_data( __UNUSED__ int device, __UNUSED__ const void *unused, __UNUSED__  size_t mapnum,
                              __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                              __UNUSED__ unsigned char *kinds )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_target_data_ext( __UNUSED__ int device, __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs,
                                  __UNUSED__ size_t *sizes, __UNUSED__ unsigned short *kinds )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_target_end_data( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_target_update( __UNUSED__ int device, __UNUSED__ const void *unused, __UNUSED__ size_t mapnum,
                                __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                                __UNUSED__ unsigned char *kinds )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_target_update_ext( __UNUSED__ int device, __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs,
                                    __UNUSED__  size_t *sizes, __UNUSED__  unsigned short *kinds,
                                    __UNUSED__ unsigned int flags, __UNUSED__ void **depend )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_target_enter_exit_data( __UNUSED__ int device, __UNUSED__ size_t mapnum,
        __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
        __UNUSED__ unsigned short *kinds,
        __UNUSED__ unsigned int flags, __UNUSED__  void **depend )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpcomp_GOMP_teams( __UNUSED__ unsigned int num_teams, __UNUSED__ unsigned int thread_limit )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

