#include "omp_gomp.h"
#include "mpc_common_types.h"
#include "mpc_common_debug.h"
#include "mpc_omp_abi.h"
#include "mpcomp_sync.h"
#include "mpcomp_core.h"
#include "mpcomp_spinning_core.h"
#include "mpcomp_loop.h"
#include "mpcomp_task.h"
#include "mpcomp_parallel_region.h"
#include "mpcompt_task.h"
#include "mpcompt_sync.h"
#include "mpcompt_frame.h"

/***********
 * ORDERED *
 ***********/

void mpc_omp_GOMP_ordered_start( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_ordered_begin();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_ordered_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_ordered_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/***********
 * BARRIER *
 ***********/

void mpc_omp_GOMP_barrier( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_barrier();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

bool mpc_omp_GOMP_barrier_cancel( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return false;
}

/**********
 * SINGLE *
 **********/

bool mpc_omp_GOMP_single_start( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_do_single() ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd -- ret: %s", __func__,
	              ( ret == true ) ? "true" : "false" );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */

	return ret;
}

void *mpc_omp_GOMP_single_copy_start( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	void *ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = mpc_omp_do_single_copyprivate_begin();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpc_omp_GOMP_single_copy_end( void *data )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_do_single_copyprivate_end( data );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return;
}

/********************
 * WRAPPER FUNCTION *
 ********************/

/**
 * \fn void *mpc_omp_GOMP_wrapper_fn(void* args)
 * \brief Transform void(*fn)(void*) GOMP args to void*(*fn)(void*) \
 * unpack mpcomp_gomp_wrapper_t.
 * \param arg opaque type
 */
static inline void * mpc_omp_GOMP_wrapper_fn( void *args )
{
	mpc_omp_GOMP_wrapper_t *w = ( mpc_omp_GOMP_wrapper_t * ) args;
	assert( w );
	assert( w->fn );
	w->fn( w->data );
    return NULL;
}

/*******************
 * PARALLEL REGION *
 *******************/

static inline void __gomp_in_order_scheduler_master_begin( mpc_omp_thread_t *t )
{
    assert( t != NULL );
	assert( t->instance != NULL );
	assert( t->instance->team != NULL );
	assert( t->info.func != NULL );

	t->schedule_type = ( long ) t->info.combined_pragma;

	/* Handle beginning of combined parallel region */
	switch ( t->info.combined_pragma )
	{
		case MPC_OMP_COMBINED_NONE:
			mpc_common_nodebug( "%s:\nBEGIN - No combined parallel", __func__ );
			break;

		case MPC_OMP_COMBINED_SECTION:
			mpc_common_nodebug( "%s:\n BEGIN - Combined parallel/sections w/ %d section(s)",
			              __func__, t->info.nb_sections );
			_mpc_omp_sections_init( t, t->info.nb_sections );
			break;

		case MPC_OMP_COMBINED_STATIC_LOOP:
			mpc_common_nodebug( "%s:\tBEGIN - Combined parallel/loop", __func__ );
			_mpc_omp_static_loop_init( t, t->info.loop_lb, t->info.loop_b,
			                           t->info.loop_incr, t->info.loop_chunk_size );
			break;

		case MPC_OMP_COMBINED_DYN_LOOP:
			mpc_common_nodebug( "%s:\tBEGIN - Combined parallel/loop", __func__ );
			_mpc_omp_dynamic_loop_init( t, t->info.loop_lb, t->info.loop_b,
			                            t->info.loop_incr, t->info.loop_chunk_size );
			break;

		default:
			not_implemented();
			break;
	}
}

static inline void __gomp_in_order_scheduler_master_end( void )
{
    /* Nothing to do */
}

static inline void __gomp_start_parallel( void ( *fn )( void * ), void *data,
        unsigned num_threads,
        __UNUSED__ unsigned int flags )
{
	mpc_omp_start_parallel_region( ( void ( * )( void * ) ) fn, data, num_threads );
}

static inline void __gomp_start_parallel_region( void ( *fn )( void * ), void *data,
        unsigned num_threads )
{
	mpc_omp_thread_t *t, *next_thread;
	mpc_omp_parallel_region_t *info;

	/* Initialize OpenMP environment */
	mpc_omp_init();

	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
    mpc_omp_mvp_t *mvp = t->mvp;
    assert( mvp );

	/* Prepare MPC OpenMP parallel region infos */
	info = sctk_malloc( sizeof( mpc_omp_parallel_region_t ) );
	assert( info );
	_mpc_omp_parallel_region_infos_init( info );

	info->func = (void *( * )( void * )) fn;
	info->shared = data;
    info->icvs = t->info.icvs;
	info->combined_pragma = MPC_OMP_COMBINED_NONE;

	/* Begin scheduling */
	_mpc_omp_internal_begin_parallel_region( info, num_threads );

    t->mvp->instance = t->children_instance;
    next_thread = __mvp_wakeup( mvp );
    assert( next_thread->mvp );

    mpc_omp_tls = (void*) next_thread;

    __scatter_instance_post_init( next_thread );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_task_schedule(
        &MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data,
        ompt_task_switch,
        &MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(next_thread)->ompt_task_data );

    _mpc_omp_ompt_callback_implicit_task( ompt_scope_begin,
                                      t->instance->nb_mvps,
                                      t->rank,
                                      ompt_task_implicit );
#endif /* OMPT_SUPPORT */

	__gomp_in_order_scheduler_master_begin( next_thread );

    sctk_free( info );
    info = NULL;
}

static inline void __gomp_end_parallel_region( void )
{
	__gomp_in_order_scheduler_master_end();

    volatile int * spin_status;
    mpc_omp_thread_t *t_prev;
	mpc_omp_thread_t *t = mpc_omp_tls;

	assert( t != NULL );
    mpc_omp_mvp_t *mvp = t->mvp;
    assert(mvp != NULL);

    /* Must be set before barrier for thread safety*/
    spin_status = ( mvp->spin_node ) ? &( mvp->spin_node->spin_status ) : &( mvp->spin_status );
    *spin_status = MPC_OMP_MVP_STATE_SLEEP;

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_sync_region( ompt_sync_region_barrier_implicit, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

    mpc_omp_barrier();

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_sync_region( ompt_sync_region_barrier_implicit, ompt_scope_end );
    _mpc_omp_ompt_callback_implicit_task( ompt_scope_end,
                                      0,
                                      t->rank,
                                      ompt_task_implicit );
#endif /* OMPT_SUPPORT */

    mpc_omp_free(t);

    t_prev = mvp->threads->next;
    assert(t_prev != NULL);
    assert(t_prev->instance != NULL);

    if( t_prev ) {
#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_task_schedule(
        &MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data,
        ompt_task_complete,
        &MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t_prev)->ompt_task_data );
#endif /* OMPT_SUPPORT */

        mvp->threads = t_prev;
        //mpc_omp_free( t );
    }

    mvp->instance = t_prev->instance;
    mpc_omp_tls = t_prev;

    _mpc_omp_internal_end_parallel_region(t_prev->instance);
}

/* GOMP Parallel Region */

void mpc_omp_GOMP_parallel( void ( *fn )( void * ), void *data, unsigned num_threads,
                           unsigned flags )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_start_parallel( fn, data, num_threads, flags );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_parallel_start( void ( *fn )( void * ), void *data,
                                 unsigned num_threads )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_start_parallel_region( fn, data, num_threads );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void mpc_omp_GOMP_parallel_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_end_parallel_region();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpc_omp_GOMP_cancellation_point( __UNUSED__ int which )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return false;
}

bool mpc_omp_GOMP_cancel( __UNUSED__ int which, __UNUSED__ bool do_cancel )
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
	mpc_omp_thread_t *t, *next_thread;
	mpc_omp_parallel_region_t *info;

	/* Initialize OpenMP environment */
	mpc_omp_init();

	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
    mpc_omp_mvp_t *mvp = t->mvp;
    assert( mvp );

	info = sctk_malloc( sizeof( mpc_omp_parallel_region_t ) );
	assert( info );

	_mpc_omp_parallel_region_infos_init( info );
	_mpc_omp_parallel_set_specific_infos( info, ( void *( * ) ( void * ) ) fn, data,
	                                      t->info.icvs, combined_pragma );

	_mpc_omp_loop_gen_infos_init( &( t->info.loop_infos ), start, end, incr,
	                              chunk_size );

	_mpc_omp_internal_begin_parallel_region( info, num_threads );

	/* Start scheduling */
    t->mvp->instance = t->children_instance;
    next_thread = __mvp_wakeup( mvp );
    assert( next_thread->mvp );

    mpc_omp_tls = (void*) next_thread;

    __scatter_instance_post_init( next_thread );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_task_schedule(
        &MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data,
        ompt_task_switch,
        &MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(next_thread)->ompt_task_data );
#endif /* OMPT_SUPPORT */

	__gomp_in_order_scheduler_master_begin( next_thread );

    sctk_free( info );
    info = NULL;
}

/* Gomp Loop Definition */

/* Ordered Loop */

static inline bool __gomp_loop_runtime_start( long start, long end, long incr,
        long *istart, long *iend )
{
	return ( mpc_omp_runtime_loop_begin( start, end, incr, istart, iend ) ) ? true
	       : false;
}

bool mpc_omp_GOMP_loop_runtime_start( long istart, long iend, long incr,
                                     long *start, long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

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
	return ( mpc_omp_ordered_runtime_loop_begin( start, end, incr, istart, iend ) )
	       ? true
	       : false;
}

bool mpc_omp_GOMP_loop_ordered_runtime_start( long istart, long iend, long incr,
        long *start, long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_ordered_runtime_start( istart, iend, incr,
	        start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ordered_runtime_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = mpc_omp_ordered_runtime_loop_next( start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ordered_static_start( long start, long end,
        long incr, long chunk_size,
        long *istart, long *iend )
{
	return ( mpc_omp_ordered_static_loop_begin( start, end, incr, chunk_size,
	         istart, iend ) )
	       ? true
	       : false;
}

bool mpc_omp_GOMP_loop_ordered_static_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_ordered_static_start( istart, iend, incr,
	                                        chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ordered_static_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_ordered_static_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ordered_dynamic_start( long start, long end,
        long incr, long chunk_size,
        long *istart, long *iend )
{
	return ( mpc_omp_ordered_dynamic_loop_begin( start, end, incr, chunk_size,
	         istart, iend ) )
	       ? true
	       : false;
}

bool mpc_omp_GOMP_loop_ordered_dynamic_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_ordered_dynamic_start( istart, iend, incr,
	        chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ordered_dynamic_next( long *start, long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_ordered_dynamic_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

static inline bool __gomp_loop_ordered_guided_start( long start, long end,
        long incr, long chunk_size,
        long *istart, long *iend )
{
	return ( mpc_omp_ordered_guided_loop_begin( start, end, incr, chunk_size,
	         istart, iend ) )
	       ? true
	       : false;
}

bool mpc_omp_GOMP_loop_ordered_guided_start( long istart, long iend, long incr,
        long chunk_size, long *start,
        long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_ordered_guided_start( istart, iend, incr,
	                                        chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ordered_guided_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_ordered_guided_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* Static */

void mpc_omp_GOMP_parallel_loop_static_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	__gomp_parallel_loop_generic_start(
	    fn, data, num_threads, start, end, incr, chunk_size,
	    ( long ) MPC_OMP_COMBINED_STATIC_LOOP );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

static inline bool __gomp_loop_static_start( long start, long end, long incr,
        long chunk_size, long *istart,
        long *iend )
{
	return ( mpc_omp_static_loop_begin( start, end, incr, chunk_size, istart,
	                                     iend ) )
	       ? true
	       : false;
}

bool mpc_omp_GOMP_loop_static_start( long istart, long iend, long incr,
                                    long chunk_size, long *start, long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_static_start( istart, iend, incr, chunk_size,
	                                start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_static_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_static_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* Dynamic */

void mpc_omp_GOMP_parallel_loop_dynamic_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin\tSTART:%ld", __func__, start );
	__gomp_parallel_loop_generic_start(
	    fn, data, num_threads, start, end, incr, chunk_size,
	    ( long ) MPC_OMP_COMBINED_DYN_LOOP );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

static inline bool __gomp_loop_dynamic_start( long start, long end, long incr,
        long chunk_size, long *istart,
        long *iend )
{
	return ( mpc_omp_dynamic_loop_begin( start, end, incr, chunk_size, istart,
	                                      iend ) )
	       ? true
	       : false;
}

bool mpc_omp_GOMP_loop_dynamic_start( long istart, long iend, long incr,
                                     long chunk_size, long *start, long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_dynamic_start( istart, iend, incr, chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_dynamic_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_dynamic_loop_next( start, end ) ) ? true : false;
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
	    ( long ) MPC_OMP_COMBINED_GUIDED_LOOP );
}

void mpc_omp_GOMP_parallel_loop_guided_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr,
        long chunk_size )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_parallel_loop_guided_start( fn, data, num_threads, start,
	                                   end, incr, chunk_size );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

static inline bool __gomp_loop_guided_start( long start, long end, long incr,
        long chunk_size, long *istart,
        long *iend )
{
	return ( mpc_omp_guided_loop_begin( start, end, incr, chunk_size, istart,
	                                     iend ) )
	       ? true
	       : false;
}

bool mpc_omp_GOMP_loop_guided_start( long istart, long iend, long incr,
                                    long chunk_size, long *start, long *end )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_guided_start( istart, iend, incr, chunk_size,
	                                start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_guided_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_guided_loop_next( start, end ) ) ? true : false;
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
	mpc_omp_thread_t *t; /* Info on the current thread */

	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();

	/* Grab the thread info */
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );

	switch ( t->info.icvs.run_sched_var )
	{
		case omp_sched_static:
			combined_pragma = ( long ) MPC_OMP_COMBINED_STATIC_LOOP;
			break;

		case omp_sched_dynamic:
		case omp_sched_guided:
			combined_pragma = ( long ) MPC_OMP_COMBINED_GUIDED_LOOP;
			break;

		default:
			not_reachable();
			break;
	}

	chunk_size = t->info.icvs.modifier_sched_var;
	__gomp_parallel_loop_generic_start(
	    fn, data, num_threads, start, end, incr, chunk_size, combined_pragma );
}

void mpc_omp_GOMP_parallel_loop_runtime_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, long start,
        long end, long incr )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_parallel_loop_runtime_start( fn, data, num_threads, start,
	                                    end, incr );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/* loop nonmonotonic */

void mpc_omp_GOMP_parallel_loop_nonmonotonic_dynamic(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, __UNUSED__ unsigned flags )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	_mpc_omp_start_parallel_dynamic_loop( fn, data, num_threads, start, end, incr,
			chunk_size );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpc_omp_GOMP_loop_nonmonotonic_dynamic_start( long istart, long iend,
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

void mpc_omp_GOMP_parallel_loop_nonmonotonic_guided(
    void ( *fn )( void * ), void *data, unsigned num_threads, long start, long end,
    long incr, long chunk_size, __UNUSED__ unsigned flags )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	_mpc_omp_start_parallel_guided_loop( fn, data, num_threads, start, end, incr,
			chunk_size );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpc_omp_GOMP_loop_nonmonotonic_guided_start( long istart, long iend,
        long incr, long chunk_size,
        long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = __gomp_loop_guided_start( istart, iend, incr, chunk_size, start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_nonmonotonic_dynamic_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_dynamic_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_nonmonotonic_guided_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = ( mpc_omp_guided_loop_next( start, end ) ) ? true : false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/* Common loop ops */

static inline void __gomp_loop_end( void )
{
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );

	switch ( t->schedule_type )
	{
		case MPC_OMP_COMBINED_STATIC_LOOP:
		{
			mpc_omp_static_loop_end();
			break;
		}

		case MPC_OMP_COMBINED_DYN_LOOP:
		{
			mpc_omp_dynamic_loop_end();
			break;
		}

		case MPC_OMP_COMBINED_GUIDED_LOOP:
		{
			mpc_omp_guided_loop_end();
			break;
		}

		case MPC_OMP_COMBINED_RUNTIME_LOOP:
		{
			mpc_omp_runtime_loop_end();
			break;
		}

		default:
		{
			not_implemented();
			//mpc_omp_static_loop_end();
			//mpc_omp_dynamic_loop_end();
		}
	}
}

static inline void __gomp_loop_end_nowait( void )
{
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );

	switch ( t->schedule_type )
	{
		case MPC_OMP_COMBINED_STATIC_LOOP:
		{
			mpc_omp_static_loop_end_nowait();
			break;
		}

		case MPC_OMP_COMBINED_DYN_LOOP:
		{
			mpc_omp_dynamic_loop_end_nowait();
			break;
		}

		case MPC_OMP_COMBINED_GUIDED_LOOP:
		{
			mpc_omp_guided_loop_end_nowait();
			break;
		}

		case MPC_OMP_COMBINED_RUNTIME_LOOP:
		{
			mpc_omp_runtime_loop_end_nowait();
			break;
		}

		default:
		{
			not_implemented();
			//mpc_omp_static_loop_end_nowait();
		}
	}
}

bool mpc_omp_GOMP_loop_runtime_next( long *start, long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = mpc_omp_runtime_loop_next( start, end );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpc_omp_GOMP_loop_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_loop_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

bool mpc_omp_GOMP_loop_end_cancel( void )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpc_omp_GOMP_loop_end_nowait( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_loop_end_nowait();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/* Doacross */

bool mpc_omp_GOMP_loop_doacross_static_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	MPC_OMP_GOMP_UNUSED_VAR( ncounts );
	MPC_OMP_GOMP_UNUSED_VAR( counts );
	MPC_OMP_GOMP_UNUSED_VAR( chunk_size );
	MPC_OMP_GOMP_UNUSED_VAR( start );
	MPC_OMP_GOMP_UNUSED_VAR( end );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_doacross_dynamic_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	MPC_OMP_GOMP_UNUSED_VAR( ncounts );
	MPC_OMP_GOMP_UNUSED_VAR( counts );
	MPC_OMP_GOMP_UNUSED_VAR( chunk_size );
	MPC_OMP_GOMP_UNUSED_VAR( start );
	MPC_OMP_GOMP_UNUSED_VAR( end );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_doacross_guided_start( unsigned ncounts, long *counts,
        long chunk_size, long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	MPC_OMP_GOMP_UNUSED_VAR( ncounts );
	MPC_OMP_GOMP_UNUSED_VAR( counts );
	MPC_OMP_GOMP_UNUSED_VAR( chunk_size );
	MPC_OMP_GOMP_UNUSED_VAR( start );
	MPC_OMP_GOMP_UNUSED_VAR( end );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_doacross_runtime_start( unsigned ncounts, long *counts,
        long *start,
        long *end )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	MPC_OMP_GOMP_UNUSED_VAR( ncounts );
	MPC_OMP_GOMP_UNUSED_VAR( counts );
	MPC_OMP_GOMP_UNUSED_VAR( start );
	MPC_OMP_GOMP_UNUSED_VAR( end );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

/** OPENMP 4.0 **/

void mpc_omp_GOMP_parallel_loop_static( void ( *fn )( void * ), void *data,
                                       unsigned num_threads, long start,
                                       long end, long incr, long chunk_size,
                                       __UNUSED__ unsigned flags )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	_mpc_omp_start_parallel_static_loop( fn, data, num_threads, start, end, incr,
	                                     chunk_size );
}

void mpc_omp_GOMP_parallel_loop_dynamic( void ( *fn )( void * ), void *data,
                                        unsigned num_threads, long start,
                                        long end, long incr, long chunk_size,
                                        __UNUSED__ unsigned flags )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	_mpc_omp_start_parallel_dynamic_loop( fn, data, num_threads, start, end, incr,
	                                      chunk_size );
}

void mpc_omp_GOMP_parallel_loop_guided( void ( *fn )( void * ), void *data,
                                       unsigned num_threads, long start,
                                       long end, long incr, long chunk_size,
                                       __UNUSED__ unsigned flags )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	_mpc_omp_start_parallel_guided_loop( fn, data, num_threads, start, end, incr,
	                                     chunk_size );
}

void mpc_omp_GOMP_parallel_loop_runtime( void ( *fn )( void * ), void *data,
                                        unsigned num_threads, long start,
                                        long end, long incr, __UNUSED__ unsigned flags )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	_mpc_omp_start_parallel_runtime_loop( fn, data, num_threads, start, end, incr, 0 );
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
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type = MPC_OMP_COMBINED_RUNTIME_LOOP;
	ret = ( mpc_omp_loop_ull_runtime_begin( up, start, end, incr, istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpc_omp_GOMP_loop_ull_runtime_start( bool up, unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = mpc_omp_loop_ull_runtime_begin( up, start, end, incr, istart, iend );
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
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type = MPC_OMP_COMBINED_RUNTIME_LOOP;
	ret = ( mpc_omp_loop_ull_ordered_runtime_begin( up, start, end, incr, istart,
	        iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpc_omp_GOMP_loop_ull_ordered_runtime_start( bool up,
        unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long *istart,
        unsigned long long *iend )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

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

bool mpc_omp_GOMP_loop_ull_doacross_runtime_start( __UNUSED__ unsigned ncounts,
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

bool mpc_omp_GOMP_loop_ull_runtime_next( unsigned long long *istart,
                                        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = mpc_omp_loop_ull_runtime_next( istart, iend );
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_ordered_runtime_next( unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = mpc_omp_loop_ull_ordered_runtime_next( istart, iend );
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
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type = MPC_OMP_COMBINED_STATIC_LOOP;
	ret = ( mpc_omp_static_loop_begin_ull( up, start, end, incr, chunk_size,
	                                        istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpc_omp_GOMP_loop_ull_static_start( bool up, unsigned long long start,
                                        unsigned long long end,
                                        unsigned long long incr,
                                        unsigned long long chunk_size,
                                        unsigned long long *istart,
                                        unsigned long long *iend )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

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
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type = MPC_OMP_COMBINED_DYN_LOOP;
	ret = ( mpc_omp_loop_ull_dynamic_begin( up, start, end, incr, chunk_size,
	        istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpc_omp_GOMP_loop_ull_dynamic_start( bool up, unsigned long long start,
        unsigned long long end,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *istart,
        unsigned long long *iend )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
#ifdef MPC_SUPPORT_ULL_LOOP
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin - start: %llu - end: %llu - step: "
	              "%llu - up: %d",
	              __func__, start, end, incr, up );
	TODO( "CHECK why this was changed" );
	ret = mpc_omp_loop_ull_dynamic_begin( up, start, end, incr, chunk_size,
	                                       istart, iend );
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
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type = MPC_OMP_COMBINED_GUIDED_LOOP;
	ret = ( mpc_omp_loop_ull_guided_begin( up, start, end, incr, chunk_size,
	                                        istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpc_omp_GOMP_loop_ull_guided_start( bool up, unsigned long long start,
                                        unsigned long long end,
                                        unsigned long long incr,
                                        unsigned long long chunk_size,
                                        unsigned long long *istart,
                                        unsigned long long *iend )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	TODO( "CHECK why GUIDED schedule is not used from previous function" );
	ret = mpc_omp_loop_ull_guided_begin( up, start, end, incr, chunk_size, istart,
	                                      iend );
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_nonmonotonic_dynamic_start(
     bool up,  unsigned long long start,  unsigned long long end,
     unsigned long long incr,  unsigned long long chunk_size,
     unsigned long long *istart,  unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = __gomp_loop_ull_dynamic_start( up, start, end, incr, chunk_size, istart, iend );
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_nonmonotonic_guided_start(
     bool up,  unsigned long long start,  unsigned long long end,
		 unsigned long long incr,  unsigned long long chunk_size,
		 unsigned long long *istart,  unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
		ret = __gomp_loop_ull_guided_start( up, start, end, incr, chunk_size, istart, iend );
#endif /* MPC_SUPPORT_ULL_LOOP */
		mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
		return ret;
		}

static inline bool __gomp_loop_ull_ordered_static_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
	bool ret;
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type = MPC_OMP_COMBINED_STATIC_LOOP;
	ret = ( mpc_omp_ordered_static_loop_begin_ull( up, start, end, incr,
	        chunk_size, istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpc_omp_GOMP_loop_ull_ordered_static_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

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
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type = MPC_OMP_COMBINED_DYN_LOOP;
	ret = ( mpc_omp_loop_ull_ordered_dynamic_begin( up, start, end, incr,
	        chunk_size, istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpc_omp_GOMP_loop_ull_ordered_dynamic_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

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
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type = MPC_OMP_COMBINED_GUIDED_LOOP;
	ret = ( mpc_omp_loop_ull_ordered_guided_begin( up, start, end, incr,
	        chunk_size, istart, iend ) )
	      ? true
	      : false;
	return ret;
}

bool mpc_omp_GOMP_loop_ull_ordered_guided_start(
    bool up, unsigned long long start, unsigned long long end,
    unsigned long long incr, unsigned long long chunk_size,
    unsigned long long *istart, unsigned long long *iend )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

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

bool mpc_omp_GOMP_loop_ull_doacross_static_start( __UNUSED__ unsigned ncounts,
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

bool mpc_omp_GOMP_loop_ull_doacross_dynamic_start( __UNUSED__ unsigned ncounts,
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

bool mpc_omp_GOMP_loop_ull_doacross_guided_start( __UNUSED__ unsigned ncounts,
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

bool mpc_omp_GOMP_loop_ull_static_next( unsigned long long *istart,
                                       unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( mpc_omp_static_loop_next_ull( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_dynamic_next( unsigned long long *istart,
                                        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( mpc_omp_loop_ull_dynamic_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_guided_next( unsigned long long *istart,
                                       unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( mpc_omp_loop_ull_guided_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_nonmonotonic_dynamic_next(  unsigned long long *istart,
         unsigned long long *iend )
{
	bool ret = false;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( mpc_omp_loop_ull_dynamic_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_nonmonotonic_guided_next(  unsigned long long *istart,
         unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( mpc_omp_loop_ull_guided_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_ordered_static_next( unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( mpc_omp_ordered_static_loop_next_ull( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_ordered_dynamic_next( unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( mpc_omp_loop_ull_ordered_dynamic_next( istart, iend ) ) ? true : false;
#else  /* MPC_SUPPORT_ULL_LOOP */
	ret = false;
	not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

bool mpc_omp_GOMP_loop_ull_ordered_guided_next( unsigned long long *istart,
        unsigned long long *iend )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#ifdef MPC_SUPPORT_ULL_LOOP
	ret = ( mpc_omp_loop_ull_ordered_guided_next( istart, iend ) ) ? true : false;
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

unsigned mpc_omp_GOMP_sections_start( unsigned count )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	unsigned ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = mpc_omp_sections_begin( count );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpc_omp_GOMP_sections_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
    mpc_omp_sections_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

unsigned mpc_omp_GOMP_sections_next( void )
{
	unsigned ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = mpc_omp_sections_next();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpc_omp_GOMP_parallel_sections( void ( *fn )( void * ), void *data,
                                    unsigned num_threads, unsigned count,
                                    __UNUSED__ unsigned flags )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	_mpc_omp_start_sections_parallel_region( fn, data, num_threads, count );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

static inline void __gomp_parallel_section_start( void ( *fn )( void * ),
        void *data,
        unsigned num_threads,
        unsigned count )
{
	mpc_omp_thread_t *t, *next_thread;
	mpc_omp_parallel_region_t *info;
	mpc_omp_GOMP_wrapper_t *wrapper_gomp;

	/* Initialize OpenMP environment */
	mpc_omp_init();

	t = ( mpc_omp_thread_t * ) mpc_omp_tls;
	assert( t != NULL );
    mpc_omp_mvp_t *mvp = t->mvp;
    assert( mvp );

	info = sctk_malloc( sizeof( mpc_omp_parallel_region_t ) );
	assert( info );
	_mpc_omp_parallel_region_infos_init( info );

	wrapper_gomp = sctk_malloc( sizeof( mpc_omp_GOMP_wrapper_t ) );
	assert( wrapper_gomp );
	wrapper_gomp->fn = fn;
	wrapper_gomp->data = data;

	info->func = mpc_omp_GOMP_wrapper_fn;
	info->shared = wrapper_gomp;
    info->icvs = t->info.icvs;
	info->combined_pragma = MPC_OMP_COMBINED_SECTION;
	info->nb_sections = count;

	num_threads = ( num_threads == 0 ) ? 1 : num_threads;
	_mpc_omp_internal_begin_parallel_region( info, num_threads );

	/* Begin scheduling */
    t->mvp->instance = t->children_instance;
    next_thread = __mvp_wakeup( mvp );
    assert( next_thread->mvp );

    mpc_omp_tls = (void*) next_thread;

    __scatter_instance_post_init( next_thread );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_task_schedule(
        &MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data,
        ompt_task_switch,
        &MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(next_thread)->ompt_task_data );
#endif /* OMPT_SUPPORT */

	__gomp_in_order_scheduler_master_begin( next_thread );

    sctk_free( info );
    info = NULL;
}

void mpc_omp_GOMP_parallel_sections_start( void ( *fn )( void * ), void *data,
        unsigned num_threads, unsigned count )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	__gomp_parallel_section_start( fn, data, num_threads, count );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

bool mpc_omp_GOMP_sections_end_cancel( void )
{
	bool ret;
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	ret = false;
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
	return ret;
}

void mpc_omp_GOMP_sections_end_nowait( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_sections_end_nowait();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

/************
 * CRITICAL *
 ************/

void mpc_omp_GOMP_critical_start( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_anonymous_critical_begin();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void mpc_omp_GOMP_critical_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_anonymous_critical_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void mpc_omp_GOMP_critical_name_start( void **pptr )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_named_critical_begin( pptr );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void mpc_omp_GOMP_critical_name_end( void **pptr )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_named_critical_end( pptr );
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void mpc_omp_GOMP_atomic_start( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_atomic_begin();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void mpc_omp_GOMP_atomic_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	mpc_omp_atomic_end();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

/*********
 * TASKS *
 *********/

/**
 * See task constructor and constant definitions
 *
 * https://github.com/gcc-mirror/gcc/blob/master/libgomp/task.c#L348
 * https://github.com/gcc-mirror/gcc/blob/master/include/gomp-constants.h#L204
 */
static inline mpc_omp_task_property_t
___gomp_convert_flags(bool if_clause, int flags)
{
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * current_task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(current_task);

    TODO("This can be slightly optimized by using the same bits for GOMP and MPC tasks");

    mpc_omp_task_property_t properties = 0;

# define __CONVERT_ONE_BIT(X, Y) if (flags & X) mpc_omp_task_set_property(&properties, Y)
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_UNTIED,    MPC_OMP_TASK_PROP_UNTIED);
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_FINAL,     MPC_OMP_TASK_PROP_FINAL);
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_MERGEABLE, MPC_OMP_TASK_PROP_MERGEABLE);
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_DEPEND,    MPC_OMP_TASK_PROP_DEPEND);
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_PRIORITY,  MPC_OMP_TASK_PROP_PRIORITY);
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_UP,        MPC_OMP_TASK_PROP_UP);
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_GRAINSIZE, MPC_OMP_TASK_PROP_GRAINSIZE);
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_IF,        MPC_OMP_TASK_PROP_IF);
    __CONVERT_ONE_BIT(GOMP_TASK_FLAG_NOGROUP,   MPC_OMP_TASK_PROP_NOGROUP);
# undef  __CONVERT_ONE_BIT

    /* MPC_OMP_TASK_PROP_FINAL and MPC_OMP_TASK_PROP_INCLUDED */
    if (current_task && mpc_omp_task_property_isset(current_task->property, MPC_OMP_TASK_PROP_FINAL))
    {
        mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_INCLUDED);
        mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_FINAL);
        mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_UNDEFERRED);
    }
    /* MPC_OMP_TASK_PROP_UNDEFERRED */
    else if (!if_clause || !omp_in_parallel())
    {
        mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_UNDEFERRED);
    }

    return properties;
}

static inline void
__task_data_copy(void (*cpyfn)(void *, void *), void * data_storage, void * data, size_t arg_size)
{
    assert(data_storage);
    assert(arg_size == 0 || data);
    if (arg_size > 0)
    {
        if (cpyfn)
        {
            cpyfn(data_storage, data);
        }
        else
        {
            memcpy(data_storage, data, arg_size);
        }
    }
}

/**
 * See https://github.com/gcc-mirror/gcc/blob/master/libgomp/task.c#L351
 *
 * void
 * GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
 *              long arg_size, long arg_align, bool if_clause, unsigned flags,
 *              void **depend, int priority_hint, void *detach)
 *
 * >= 6 -> priority
 * >= 11 -> detach
 */
void
mpc_omp_GOMP_task( void ( *fn )( void * ), void *data,
                       void ( *cpyfn )( void *, void * ), long arg_size,
                       long arg_align, bool if_clause, unsigned flags,
                       void **depend, int priority)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tBegin", __func__ );

    /* convert GOMP flags to MPC-OpenMP task properties */
    mpc_omp_task_property_t properties = ___gomp_convert_flags(if_clause, flags);

    /* compute task size, data alignement, and allocate the task */
    if (arg_align == 0) arg_align = sizeof(void *);
    const size_t size = _mpc_omp_task_align_single_malloc(sizeof(mpc_omp_task_t) + arg_size, arg_align);
    mpc_omp_task_t * task = _mpc_omp_task_allocate(size);

    /* retrieve task data storage (for shared variables) */
    void * data_storage = (void *) (task + 1);
    __task_data_copy(cpyfn, data_storage, data, arg_size);

    /* set task fields */
    _mpc_omp_task_init(task, fn, data_storage, size, properties);

    /* set dependencies (and compute priorities) */
    _mpc_omp_task_deps(task, depend, priority);

    /* process the task (differ or run it) */
    _mpc_omp_task_process(task);

    mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_taskwait( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tBegin", __func__ );
	_mpc_omp_task_wait();
	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_taskyield( void )
{
	/* Nothing at the moment.  */
	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tBegin", __func__ );
	_mpc_omp_task_yield();
	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_taskgroup_start( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tBegin", __func__ );
	_mpc_omp_task_taskgroup_start();
	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void mpc_omp_GOMP_taskgroup_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tBegin", __func__ );
	_mpc_omp_task_taskgroup_end();
	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tEnd", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}


void mpc_omp_GOMP_taskloop( void (*fn)(void *), void *data,
                           void (*cpyfn)(void *, void *), long arg_size, long arg_align,
                           unsigned flags, unsigned long num_tasks, int priority,
                           long start, long end, long step )
{
    mpc_omp_init();

    TODO("task loop implementation does not support 'if' clause, and private/shared variables");
    (void)cpyfn;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

    mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tBegin", __func__ );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

    long taskstep;
    unsigned long extra_chunk, i;
    const long num_iters = _mpc_omp_task_loop_compute_num_iters(start, end, step);

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_taskloop, ompt_scope_begin, num_iters );
#endif /* OMPT_SUPPORT */

    if (num_iters)
    {
        if (!(flags & GOMP_TASK_FLAG_GRAINSIZE))
        {
            num_tasks = _mpc_task_loop_compute_loop_value(num_iters, num_tasks, step, &taskstep, &extra_chunk);
        }
        else
        {
            num_tasks = _mpc_task_loop_compute_loop_value_grainsize(num_iters, num_tasks, step, &taskstep, &extra_chunk);
            taskstep = (num_tasks == 1) ? end - start : taskstep;
        }

        if (!(flags & GOMP_TASK_FLAG_NOGROUP))
        {
            _mpc_omp_task_taskgroup_start();
        }

        /* tasks size and properties */
        if (arg_align == 0) arg_align = sizeof(void *);
        const size_t size = _mpc_omp_task_align_single_malloc(sizeof(mpc_omp_task_t) + arg_size, arg_align);
        mpc_omp_task_property_t properties = ___gomp_convert_flags(1, flags);

        /* task instantiations */
        for (i = 0; i < num_tasks; i++)
        {
            mpc_omp_task_t * task = _mpc_omp_task_allocate(size);
            long * data_storage = (long *) (task + 1);
            data_storage[0] = start;
            data_storage[1] = start + taskstep;
            _mpc_omp_task_init(task, fn, data, size, properties);
            _mpc_omp_task_deps(task, NULL, priority);
            _mpc_omp_task_process(task);

            start += taskstep;
            taskstep -= (i == extra_chunk) ? step : 0;
        }

        if (!(flags & GOMP_TASK_FLAG_NOGROUP))
        {
            _mpc_omp_task_taskgroup_end();
        }
    }

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_taskloop, ompt_scope_end, 0 );
#if MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* MPCOMPT_HAS_FRAME_SUPPORT */
#endif /* OMPT_SUPPORT */

    TODO("no barrier after a taskloop construct, right ?");

    mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_taskloop_ull( void (*fn)(void *), void *data,
                               void (*cpyfn)(void *, void *), long arg_size, long arg_align,
                               unsigned flags, unsigned long num_tasks, int priority,
                               unsigned long long start, unsigned long long end,
                               unsigned long long step )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tBegin", __func__ );
//    mpc_omp_task_loop_ull( fn, data, cpyfn, arg_size, arg_align,
//                         flags, num_tasks, priority, start, end, step );
    TODO("implement 'mpc_omp_GOMP_taskloop_ull'");
	mpc_common_nodebug( "[Redirect mpc_omp_GOMP]%s:\tEnd", __func__ );
}

/***********
 * TARGETS *
 ***********/

void mpc_omp_GOMP_offload_register_ver( __UNUSED__ unsigned version, __UNUSED__ const void *host_table,
                                       __UNUSED__ int target_type,
                                       __UNUSED__ const void *target_data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_offload_register( __UNUSED__ const void *host_table, __UNUSED__ int target_type,
                                   __UNUSED__ const void *target_data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_offload_unregister_ver( __UNUSED__ unsigned version,
        __UNUSED__ const void *host_table, __UNUSED__ int target_type,
        __UNUSED__ const void *target_data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_offload_unregister( __UNUSED__ const void *host_table, __UNUSED__ int target_type,
                                     __UNUSED__ const void *target_data )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_target( __UNUSED__ int device, __UNUSED__ void ( *fn )( void * ), __UNUSED__ const void *unused,
                         __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                         __UNUSED__ unsigned char *kinds )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_target_ext( __UNUSED__ int device, __UNUSED__  void ( *fn )( void * ), __UNUSED__ size_t mapnum,
                             __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                             __UNUSED__ unsigned short *kinds, __UNUSED__  unsigned int flags,
                             __UNUSED__ void **depend, __UNUSED__ void **args )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_target_data( __UNUSED__ int device, __UNUSED__ const void *unused, __UNUSED__  size_t mapnum,
                              __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                              __UNUSED__ unsigned char *kinds )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_target_data_ext( __UNUSED__ int device, __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs,
                                  __UNUSED__ size_t *sizes, __UNUSED__ unsigned short *kinds )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_target_end_data( void )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_target_update( __UNUSED__ int device, __UNUSED__ const void *unused, __UNUSED__ size_t mapnum,
                                __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
                                __UNUSED__ unsigned char *kinds )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_target_update_ext( __UNUSED__ int device, __UNUSED__ size_t mapnum, __UNUSED__ void **hostaddrs,
                                    __UNUSED__  size_t *sizes, __UNUSED__  unsigned short *kinds,
                                    __UNUSED__ unsigned int flags, __UNUSED__ void **depend )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_target_enter_exit_data( __UNUSED__ int device, __UNUSED__ size_t mapnum,
        __UNUSED__ void **hostaddrs, __UNUSED__ size_t *sizes,
        __UNUSED__ unsigned short *kinds,
        __UNUSED__ unsigned int flags, __UNUSED__  void **depend )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_teams( __UNUSED__ unsigned int num_teams, __UNUSED__ unsigned int thread_limit )
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_doacross_post (__UNUSED__ long *counts)
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_doacross_ull_post (__UNUSED__ unsigned long long *counts)
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_doacross_wait (__UNUSED__ long *first, ...)
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

void mpc_omp_GOMP_doacross_ull_wait (__UNUSED__ unsigned long long *first, ...)
{
	mpc_common_nodebug( "[Redirect GOMP]%s:\tBegin", __func__ );
	not_implemented();
	mpc_common_nodebug( "[Redirect GOMP]%s:\tEnd", __func__ );
}

