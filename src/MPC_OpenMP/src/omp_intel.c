/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:01 CEST 2021                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Adrien Roussel <adrien.roussel@cea.fr>                             # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */
#include "omp_intel.h"
#include "mpc_omp_abi.h"
#include "mpc_common_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_loop.h"
#include "mpcomp_parallel_region.h"
#include "mpcomp_sync.h"
#include "mpcomp_task.h"
#include "mpcompt_workShare.h"
#include "mpcompt_sync.h"
#include "mpcompt_task.h"
#include "mpcompt_frame.h"

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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> omp_get_max_threads", __func__ );
	return ( kmp_int32 )omp_get_thread_num();
}

kmp_int32 __kmpc_global_num_threads( __UNUSED__ ident_t *loc )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> omp_get_num_threads", __func__ );
	return ( kmp_int32 )omp_get_num_threads();
}

kmp_int32 __kmpc_bound_thread_num( __UNUSED__ ident_t *loc )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> omp_get_thread_num", __func__ );
	return ( kmp_int32 )omp_get_thread_num();
}

kmp_int32 __kmpc_bound_num_threads( __UNUSED__ ident_t *loc )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> omp_get_num_threads", __func__ );
	return ( kmp_int32 )omp_get_num_threads();
}

kmp_int32 __kmpc_in_parallel( __UNUSED__ ident_t *loc )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_pre_init_infos();
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> omp_in_parallel", __func__ );
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	mpc_common_nodebug( "[%d] %s: entering...", t->rank, __func__ );
	mpc_omp_barrier();
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
	mpc_omp_intel_wrapper_t *w = ( mpc_omp_intel_wrapper_t * )arg;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( w );
	mpc_common_nodebug( "%s: f: %p, argc: %d, args: %p", __func__, w->f, w->argc,
	              w->args );
	__kmp_invoke_microtask( w->f, t->rank, 0, w->argc, w->args );
}

/*******************
 * PARALLEL REGION *
 *******************/



kmp_int32 __kmpc_ok_to_fork( __UNUSED__ ident_t *loc )
{
	mpc_common_nodebug( "%s: entering...", __func__ );
	return ( kmp_int32 )1;
}

void __kmpc_fork_call( __UNUSED__ ident_t *loc, kmp_int32 argc, kmpc_micro microtask, ... )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	va_list args;
	int i;
	void **args_copy;
	mpc_omp_intel_wrapper_t *w;
	mpc_omp_thread_t __UNUSED__ *t;
	static mpc_common_spinlock_t lock = MPC_COMMON_SPINLOCK_INITIALIZER;
	mpc_common_nodebug( "__kmpc_fork_call: entering w/ %d arg(s)...", argc );
	mpc_omp_intel_wrapper_t w_noalloc;
	w = &w_noalloc;
	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();

	/* Threadprivate initialisation */
	if ( !__kmp_init_common )
	{
		mpc_common_spinlock_lock( &lock );

		if ( !__kmp_init_common )
		{
			for ( i = 0; i < KMP_HASH_TABLE_SIZE; ++i )
			{
				__kmp_threadprivate_d_table.data[i] = 0;
			}

			__kmp_init_common = 1;
		}

		mpc_common_spinlock_unlock( &lock );
	}

	/* Grab info on the current thread */
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

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
	assert( args_copy );
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
	mpc_common_nodebug( " f: %p, argc: %d, args: %p, num_thread: %d", w->f, w->argc,
	              w->args, t->push_num_threads );
	_mpc_omp_start_parallel_region( __intel_wrapper_func, w,
	                                t->push_num_threads );
	/* restore the number of threads w/ num_threads clause */
	t->push_num_threads = 0;
}

void __kmpc_serialized_parallel( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t, *t_next;
	mpc_omp_parallel_region_t *info;

#ifdef MPC_OMP_FORCE_PARALLEL_REGION_ALLOC
	info = sctk_malloc( sizeof( mpc_omp_parallel_region_t ) );
	assert( info );
#else  /* MPC_OMP_FORCE_PARALLEL_REGION_ALLOC */
	mpc_omp_parallel_region_t noallocate_info;
	info = &noallocate_info;
#endif /* MPC_OMP_FORCE_PARALLEL_REGION_ALLOC */

	mpc_common_nodebug( "%s: entering (%d) ...", __func__, global_tid );

	/* Grab the thread info */
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

    /* No function to call */
	info->func = NULL;
	info->shared = NULL;
	info->num_threads = 1;
	info->new_root = NULL;
	info->icvs = t->info.icvs;
	info->combined_pragma = MPC_OMP_COMBINED_NONE;

	_mpc_omp_internal_begin_parallel_region( info, 1 );

    t_next = t->children_instance->master;

    _mpc_omp_task_tree_init( t_next );

	/* Save the current tls */
	t->children_instance->mvps[0].ptr.mvp->threads[0].father = mpc_omp_tls;
    t_next->father = t;

    /* Set instance for thread executing region-body */
    t->mvp->instance = t->children_instance;
    t_next->instance = t->children_instance;

    /* Set info */
    t_next->info = t_next->instance->team->info;

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_task_schedule( &( MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data ),
                                      ompt_task_switch,
                                      &( MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t_next)->ompt_task_data  ));

    _mpc_omp_ompt_callback_implicit_task( ompt_scope_begin, 1, 0, ompt_task_implicit );
#endif /* OMPT_SUPPORT */

	/* Switch TLS to nested thread for region-body execution */
    mpc_omp_tls = t_next;

	mpc_common_nodebug( "%s: leaving (%d) ...", __func__, global_tid );
}

void __kmpc_end_serialized_parallel( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t, *t_prev;

	mpc_common_nodebug( "%s: entering (%d)...", __func__, global_tid );

	/* Grab the thread info */
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_sync_region( ompt_sync_region_barrier_implicit, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

    mpc_omp_barrier();

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_sync_region( ompt_sync_region_barrier_implicit, ompt_scope_end );
    _mpc_omp_ompt_callback_implicit_task( ompt_scope_end, 0, 0, ompt_task_implicit );
#endif /* OMPT_SUPPORT */

	/* Restore the previous thread info */
    t_prev = t->father;
    t_prev->mvp->threads = t_prev;
    t_prev->mvp->instance = t_prev->instance;

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_task_schedule( &( MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t_prev)->ompt_task_data ),
                                      ompt_task_complete,
                                      &( MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t)->ompt_task_data ));
#endif /* OMPT_SUPPORT */

    //mpc_omp_free(t);
    mpc_omp_tls = t_prev;
    
    _mpc_omp_internal_end_parallel_region( t_prev->children_instance );

	mpc_common_nodebug( "%s: leaving (%d)...", __func__, global_tid );
}

void __kmpc_push_num_threads( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 global_tid,
                              kmp_int32 num_threads )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t;
	mpc_common_debug_warning( "%s: pushing %d thread(s)", __func__, num_threads );
	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();
	/* Grab the thread info */
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_master( ompt_scope_begin );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[%d] %s: entering", t->rank, __func__ );

	return ( t->rank == 0 ) ? ( kmp_int32 )1 : ( kmp_int32 )0;
}

void __kmpc_end_master( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_master( ompt_scope_end );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> None", __func__ );
}

void __kmpc_ordered( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> _mpcomp_ordered_begin", __func__ );
	mpc_omp_ordered_begin();
}

void __kmpc_end_ordered( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> _mpcomp_ordered_end", __func__ );
	mpc_omp_ordered_end();
}

void __kmpc_critical( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                      kmp_critical_name *crit )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> _mpcomp_anonymous_critical_begin",
	                    __func__ );

    mpc_omp_named_critical_begin( (void**) crit );
}

void __kmpc_critical_with_hint( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                                kmp_critical_name *crit,
                                __UNUSED__ uint32_t hint) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

    // TODO: add support of hint
    mpc_common_nodebug( "[REDIRECT KMP]: %s -> _mpcomp_anonymous_critical_begin",
                        __func__ );

    mpc_omp_named_critical_begin( (void**) crit );
}

void __kmpc_end_critical( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                          kmp_critical_name *crit )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> _mpcomp_anonymous_critical_end",
	              __func__ );

    mpc_omp_named_critical_end( (void**) crit );
}

kmp_int32 __kmpc_single( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 global_tid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> _mpcomp_do_single", __func__ );
	return ( kmp_int32 )mpc_omp_do_single();
}

void __kmpc_end_single( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_single_executor, ompt_scope_end, 0 );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> None", __func__ );
}

/*********
 * LOCKS *
 *********/


void __kmpc_init_lock( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock );
	omp_init_lock( user_mpcomp_lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __kmpc_init_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock );
	omp_init_nest_lock( user_mpcomp_nest_lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __kmpc_destroy_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock );
	omp_destroy_lock( user_mpcomp_lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __kmpc_destroy_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock );
	omp_destroy_nest_lock( user_mpcomp_nest_lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __kmpc_set_lock( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock );
	omp_set_lock( user_mpcomp_lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __kmpc_set_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock );
	omp_set_nest_lock( user_mpcomp_nest_lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __kmpc_unset_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock );
	omp_unset_lock( user_mpcomp_lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __kmpc_unset_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock );
	omp_unset_nest_lock( user_mpcomp_nest_lock );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

int __kmpc_test_lock( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_lock_t *user_mpcomp_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_lock = ( omp_lock_t * )( &( user_iomp_lock->lk ) );
	const int ret = omp_test_lock( user_mpcomp_lock );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p try: %d", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_lock, ret );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */

	return ret;
}

int __kmpc_test_nest_lock( __UNUSED__ ident_t *loc, __UNUSED__  kmp_int32 gtid, void **user_lock )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_infos();
    _mpc_omp_ompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

	omp_nest_lock_t *user_mpcomp_nest_lock = NULL;
	iomp_lock_t *user_iomp_lock = ( iomp_lock_t * )user_lock;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	assert( user_iomp_lock );
	user_mpcomp_nest_lock = ( omp_nest_lock_t * )( &( user_iomp_lock->lk ) );
	const int ret = omp_test_nest_lock( user_mpcomp_nest_lock );
	mpc_common_nodebug( "[%d] %s: iomp: %p mpcomp: %p try: %d", t->rank, __func__,
	              user_iomp_lock, user_mpcomp_nest_lock, ret );

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */

	return ret;
}

/*************
 * REDUCTION *
 *************/

int __kmp_determine_reduction_method(
    __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid, __UNUSED__ kmp_int32 num_vars, __UNUSED__ size_t reduce_size,
    void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ),
    __UNUSED__ kmp_critical_name *lck, mpc_omp_thread_t __UNUSED__ *t )
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
		int __UNUSED__ atomic_available, __UNUSED__ tree_available;

		switch ( ( forced_retval = __kmp_force_reduction_method ) )
		{
			case critical_reduce_block:
				assert( lck );

				if ( team_size <= 1 )
				{
					forced_retval = empty_reduce_block;
				}

				break;

			case atomic_reduce_block:
				atomic_available = FAST_REDUCTION_ATOMIC_METHOD_GENERATED;
				assert( atomic_available );
				break;

			case tree_reduce_block:
				tree_available = FAST_REDUCTION_TREE_METHOD_GENERATED;
				assert( tree_available );
				forced_retval = tree_reduce_block;
				break;

			default:
				forced_retval = critical_reduce_block;
				mpc_common_debug( "Unknown reduction method" );
		}

		retval = forced_retval;
	}

	return retval;
}

static inline int __intel_tree_reduce_nowait( void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ), mpc_omp_thread_t __UNUSED__ *t )
{
	mpc_omp_node_t *c, *new_root;
	mpc_omp_mvp_t *mvp = t->mvp;

	if ( t->info.num_threads == 1 )
	{
		return 1;
	}

	assert( mvp );
	assert( mvp->father );
	assert( mvp->father->instance );
	assert( mvp->father->instance->team );
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
						mpc_thread_yield();
						//mpc_thread_yield_wait_for_value( &( c->isArrived[local_rank * cache_size] ), 0 );
					}

					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
						/* waiting for pair thread to arrive */
						mpc_thread_yield();
						//mpc_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
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
						mpc_thread_yield();
						//mpc_thread_yield_wait_for_value( &( c->isArrived[local_rank * cache_size] ), 0 );
					}

					return 0;
				}
				else
				{
					while ( c->isArrived[( local_rank + remaining_threads ) * cache_size] != 1 )
					{
						/* waiting for pair thread to arrive */
						mpc_thread_yield();
						//mpc_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
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

static inline int __intel_tree_reduce( void *reduce_data, void ( *reduce_func )( void *lhs_data, void *rhs_data ), mpc_omp_thread_t __UNUSED__ *t )
{
	mpc_omp_node_t *c, *new_root;
	mpc_omp_mvp_t *mvp = t->mvp;

	if ( t->info.num_threads == 1 )
	{
		return 1;
	}

	assert( mvp );
	assert( mvp->father );
	assert( mvp->father->instance );
	assert( mvp->father->instance->team );
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
						mpc_thread_yield();
						//mpc_thread_yield_wait_for_value( &( c->barrier_done ), b_done + 1);
					}

					while ( c->child_type != MPC_OMP_CHILDREN_LEAF )   /* Go down */
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
						mpc_thread_yield();
						//mpc_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
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
						mpc_thread_yield();
						//mpc_thread_yield_wait_for_value( &( c->barrier_done ), b_done + 1);
					}

					while ( c->child_type != MPC_OMP_CHILDREN_LEAF )   /* Go down */
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
						mpc_thread_yield();
						//mpc_thread_yield_wait_for_value( &( c->isArrived[( local_rank + remaining_threads ) * cache_size] ), 1 );
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

kmp_int32 __kmpc_reduce_nowait( __UNUSED__ ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars,
                                size_t reduce_size, void *reduce_data,
                                void ( *reduce_func )( void *lhs_data, void *rhs_data ),
                                kmp_critical_name *lck )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	int retval = 0;
	int packed_reduction_method;
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_sync_region( ompt_sync_region_reduction, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

	/* get reduction method */
	packed_reduction_method = __kmp_determine_reduction_method( loc, global_tid,
                                                                num_vars, reduce_size,
                                                                reduce_data, reduce_func,
                                                                lck, t );
	t->reduction_method = packed_reduction_method;

	if ( packed_reduction_method == critical_reduce_block )
	{
		mpc_omp_anonymous_critical_begin();
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
		mpc_common_debug_error( "__kmpc_reduce_nowait: Unexpected reduction method %d",
		            packed_reduction_method );
		mpc_common_debug_abort();
	}

	return retval;
}

void __kmpc_end_reduce_nowait( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                               __UNUSED__ kmp_critical_name *lck )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	int packed_reduction_method;

	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

	/* get reduction method */
	packed_reduction_method = t->reduction_method;

	if ( packed_reduction_method == critical_reduce_block )
	{
		mpc_omp_anonymous_critical_end();
		mpc_omp_barrier(); //Reduce nowait...?
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
		mpc_common_debug_error( "__kmpc_end_reduce_nowait: Unexpected reduction method %d",
		            packed_reduction_method );
		mpc_common_debug_abort();
	}

    t->reduction_method = reduction_method_not_defined;

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_sync_region_wait( ompt_sync_region_reduction, ompt_scope_begin );
    _mpc_omp_ompt_callback_sync_region_wait( ompt_sync_region_reduction, ompt_scope_end );
    _mpc_omp_ompt_callback_sync_region( ompt_sync_region_reduction, ompt_scope_end );
#endif /* OMPT_SUPPORT */
}

kmp_int32 __kmpc_reduce( __UNUSED__ ident_t *loc, kmp_int32 global_tid, kmp_int32 num_vars,
                         size_t reduce_size, void *reduce_data,
                         void ( *reduce_func )( void *lhs_data, void *rhs_data ),
                         kmp_critical_name *lck )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	int retval = 0;
	int packed_reduction_method;

	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_sync_region( ompt_sync_region_reduction, ompt_scope_begin );
#endif /* OMPT_SUPPORT */

	/* get reduction method */
	packed_reduction_method = __kmp_determine_reduction_method( loc, global_tid,
                                                                num_vars, reduce_size,
                                                                reduce_data, reduce_func,
                                                                lck, t );
	t->reduction_method = packed_reduction_method;

	if ( packed_reduction_method == critical_reduce_block )
	{
		mpc_omp_anonymous_critical_begin();
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
        /* Issue for threads with retval = 0 */
        if(!retval) t->reduction_method = reduction_method_not_defined;
	}
	else
	{
		mpc_common_debug_error( "__kmpc_reduce_nowait: Unexpected reduction method %d",
		            packed_reduction_method );
		mpc_common_debug_abort();
	}

	return retval;
}

void __kmpc_end_reduce( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid,
                        __UNUSED__ kmp_critical_name *lck )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	int packed_reduction_method;

	mpc_omp_thread_t __UNUSED__ *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

	/* get reduction method */
	packed_reduction_method = t->reduction_method;

	if ( packed_reduction_method == critical_reduce_block )
	{
		mpc_omp_anonymous_critical_end();
        mpc_omp_barrier();
	}
	else if ( packed_reduction_method == atomic_reduce_block )
	{
        mpc_omp_barrier();
	}
	else if ( packed_reduction_method == tree_reduce_block )
	{
		/* For tree reduction algorithm when thread 0 enter __kmpc_end_reduce reduction value is already shared among all threads */
		mpc_omp_mvp_t *mvp = t->mvp;
		mpc_omp_node_t *c = t->instance->root;
		c->barrier_done++;

		/* Go down to release all threads still waiting on __intel_tree_reduce for the reduction to be done, we don't need to do a full barrier  */
		while ( c->child_type != MPC_OMP_CHILDREN_LEAF )
		{
			c = c->children.node[mvp->tree_rank[c->depth]];
			c->barrier_done++;
		}
        /* NOTE: Tree reduction method currently not working with OMPT
         *       because not all threads call reduce_end routine.
         *       Therefore, missing events sync region wait begin/end
         *       and sync region end event. */

        t->reduction_method = reduction_method_not_defined;
	}

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_sync_region( ompt_sync_region_reduction, ompt_scope_end );
#endif /* OMPT_SUPPORT */
}

/**********
 * STATIC *
 **********/



void __kmpc_for_static_init_4( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid, kmp_int32 schedtype,
                               kmp_int32 *plastiter, kmp_int32 *plower,
                               kmp_int32 *pupper, kmp_int32 *pstride,
                               kmp_int32 incr, kmp_int32 chunk )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

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

#if OMPT_SUPPORT
        ompt_work_t ompt_work_type = ompt_work_loop;

        if( loc != NULL ) {
            if( loc->flags & KMP_IDENT_WORK_LOOP ) {
                ompt_work_type = ompt_work_loop;
            }
            else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
                ompt_work_type = ompt_work_sections;
            }
            else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
                ompt_work_type = ompt_work_distribute;
            }
            else {
                mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                    t->rank, __func__, loc->flags );
            }
        }

        _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_begin, *pstride );
#endif /* OMPT_SUPPORT */

		return;
	}

	mpc_common_nodebug( "[%d] %s: <%s> "
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

#if OMPT_SUPPORT
    ompt_work_t ompt_work_type = ompt_work_loop;

    if( loc != NULL ) {
        if( loc->flags & KMP_IDENT_WORK_LOOP ) {
            ompt_work_type = ompt_work_loop;
        }
        else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
            ompt_work_type = ompt_work_sections;
        }
        else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
            ompt_work_type = ompt_work_distribute;
        }
        else {
            mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                t->rank, __func__, loc->flags );
        }
    }

    _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_begin, trip_count );
#endif /* OMPT_SUPPORT */

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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t;
	mpc_omp_loop_gen_info_t *loop_infos;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

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

#if OMPT_SUPPORT
        ompt_work_t ompt_work_type = ompt_work_loop;

        if( loc != NULL ) {
            if( loc->flags & KMP_IDENT_WORK_LOOP ) {
                ompt_work_type = ompt_work_loop;
            }
            else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
                ompt_work_type = ompt_work_sections;
            }
            else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
                ompt_work_type = ompt_work_distribute;
            }
            else {
                mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                    t->rank, __func__, loc->flags );
            }
        }

        _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_begin, *pstride );
#endif /* OMPT_SUPPORT */

		return;
	}

	loop_infos = &( t->info.loop_infos );
	_mpc_omp_loop_gen_infos_init_ull(
	    loop_infos, ( unsigned long long )*plower,
	    ( unsigned long long )*pupper + ( unsigned long long )incr,
	    ( unsigned long long )incr, ( unsigned long long )chunk );
	mpc_common_nodebug( "[%d] __kmpc_for_static_init_4u: <%s> "
	              "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
	              "*plastiter=%d *pstride=%d",
	              ( ( mpc_omp_thread_t * )mpc_omp_tls )->rank, loc->psource,
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

#if OMPT_SUPPORT
    ompt_work_t ompt_work_type = ompt_work_loop;

    if( loc != NULL ) {
        if( loc->flags & KMP_IDENT_WORK_LOOP ) {
            ompt_work_type = ompt_work_loop;
        }
        else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
            ompt_work_type = ompt_work_sections;
        }
        else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
            ompt_work_type = ompt_work_distribute;
        }
        else {
            mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                t->rank, __func__, loc->flags );
        }
    }

    _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_begin, trip_count );
#endif /* OMPT_SUPPORT */

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
			mpc_common_nodebug( "[%d] Results: "
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

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

#if OMPT_SUPPORT
        ompt_work_t ompt_work_type = ompt_work_loop;

        if( loc != NULL ) {
            if( loc->flags & KMP_IDENT_WORK_LOOP ) {
                ompt_work_type = ompt_work_loop;
            }
            else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
                ompt_work_type = ompt_work_sections;
            }
            else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
                ompt_work_type = ompt_work_distribute;
            }
            else {
                mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                    t->rank, __func__, loc->flags );
            }
        }

        _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_begin, *pstride );
#endif /* OMPT_SUPPORT */

		return;
	}

	mpc_common_nodebug( "[%d] __kmpc_for_static_init_8: <%s> "
	              "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
	              "*plastiter=%d *pstride=%d",
	              ( ( mpc_omp_thread_t * )mpc_omp_tls )->rank, loc->psource,
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

#if OMPT_SUPPORT
    ompt_work_t ompt_work_type = ompt_work_loop;

    if( loc != NULL ) {
        if( loc->flags & KMP_IDENT_WORK_LOOP ) {
            ompt_work_type = ompt_work_loop;
        }
        else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
            ompt_work_type = ompt_work_sections;
        }
        else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
            ompt_work_type = ompt_work_distribute;
        }
        else {
            mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                t->rank, __func__, loc->flags );
        }
    }

    _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_begin, trip_count );
#endif /* OMPT_SUPPORT */

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
			mpc_common_nodebug( "[%d] Results: "
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	/* TODO: the same as unsigned long long in GCC... */
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

	unsigned long  rank = t->rank;
	unsigned long num_threads = t->info.num_threads;
	mpc_omp_loop_gen_info_t *loop_infos;
	kmp_uint64 pupper_old = *pupper;

	if ( t->info.num_threads == 1 )
	{
		if ( plastiter != NULL )
		{
			*plastiter = TRUE;
		}

		*pstride =
		    ( incr > 0 ) ? ( *pupper - *plower + 1 ) : ( -( *plower - *pupper + 1 ) );

#if OMPT_SUPPORT
        ompt_work_t ompt_work_type = ompt_work_loop;

        if( loc != NULL ) {
            if( loc->flags & KMP_IDENT_WORK_LOOP ) {
                ompt_work_type = ompt_work_loop;
            }
            else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
                ompt_work_type = ompt_work_sections;
            }
            else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
                ompt_work_type = ompt_work_distribute;
            }
            else {
                mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                    t->rank, __func__, loc->flags );
            }
        }

        _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_begin, *pstride );
#endif /* OMPT_SUPPORT */

		return;
	}

	loop_infos = &( t->info.loop_infos );
	_mpc_omp_loop_gen_infos_init_ull(
	    loop_infos, ( unsigned long long )*plower,
	    ( unsigned long long )*pupper + ( unsigned long long )incr,
	    ( unsigned long long )incr, ( unsigned long long )chunk );
	mpc_common_nodebug( "[%ld] __kmpc_for_static_init_8u: <%s> "
	              "schedtype=%d, %d? %llu -> %llu incl. [%lld], incr=%lld chunk=%lld "
	              "*plastiter=%d *pstride=%lld",
	              ( ( mpc_omp_thread_t * )mpc_omp_tls )->rank, loc->psource,
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

#if OMPT_SUPPORT
    ompt_work_t ompt_work_type = ompt_work_loop;

    if( loc != NULL ) {
        if( loc->flags & KMP_IDENT_WORK_LOOP ) {
            ompt_work_type = ompt_work_loop;
        }
        else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
            ompt_work_type = ompt_work_sections;
        }
        else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
            ompt_work_type = ompt_work_distribute;
        }
        else {
            mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                t->rank, __func__, loc->flags );
        }
    }

    _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_begin, trip_count );
#endif /* OMPT_SUPPORT */

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
			mpc_common_nodebug( "[%d] Results: "
			              "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
			              t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
			              *plastiter );
	}
}

void __kmpc_for_static_fini( __UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 global_tid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_common_nodebug( "[REDIRECT KMP]: %s -> None", __func__ );

#if OMPT_SUPPORT
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

    ompt_work_t ompt_work_type = ompt_work_loop;

    if( loc != NULL ) {
        if( loc->flags & KMP_IDENT_WORK_LOOP ) {
            ompt_work_type = ompt_work_loop;
        }
        else if( loc->flags & KMP_IDENT_WORK_SECTIONS ) {
            ompt_work_type = ompt_work_sections;
        }
        else if( loc->flags & KMP_IDENT_WORK_DISTRIBUTE ) {
            ompt_work_type = ompt_work_distribute;
        }
        else {
            mpc_common_nodebug( "[%d] %s: WARNING! Unknown work type %d",
                                t->rank, __func__, loc->flags );
        }
    }

    _mpc_omp_ompt_callback_work( ompt_work_type, ompt_scope_end, 0 );
#endif /* OMPT_SUPPORT */
}

/************
 * DISPATCH *
 ************/

static inline void __intel_dispatch_init_long( mpc_omp_thread_t __UNUSED__ *t, long lb,
        long b, long incr,
        long chunk )
{
	assert( t );

	switch ( t->schedule_type )
	{
		/* NORMAL AUTO OR STATIC */
		case kmp_sch_auto:
		case kmp_sch_static:
			chunk = 0;
			/* FALLTHRU */
		case kmp_sch_static_chunked:
			mpc_omp_static_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		/* ORDERED AUTO OR STATIC */
		case kmp_ord_auto:
		case kmp_ord_static:
			chunk = 0;
			/* FALLTHRU */
		case kmp_ord_static_chunked:
			mpc_omp_ordered_static_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		/* regular scheduling */
		case kmp_sch_dynamic_chunked:
			mpc_omp_dynamic_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_ord_dynamic_chunked:
			mpc_omp_ordered_dynamic_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_sch_guided_chunked:
			mpc_omp_guided_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_ord_guided_chunked:
			mpc_omp_guided_loop_begin( lb, b, incr, chunk, NULL, NULL );
			break;

		/* runtime */
		case kmp_sch_runtime:
			mpc_omp_runtime_loop_begin( lb, b, incr, NULL, NULL );
			break;

		case kmp_ord_runtime:
			mpc_omp_ordered_runtime_loop_begin( lb, b, incr, NULL, NULL );
			break;

		default:
			mpc_common_debug_error( "Schedule not handled: %d\n", t->schedule_type );
			not_implemented();
	}
}

static inline void __intel_dispatch_init_ull( mpc_omp_thread_t __UNUSED__ *t, bool up,
        unsigned long long lb,
        unsigned long long b,
        unsigned long long incr,
        unsigned long long chunk )
{
	assert( t );

	switch ( t->schedule_type )
	{
		/* NORMAL AUTO OR STATIC */
		case kmp_sch_auto:
		case kmp_sch_static:
			chunk = 0;
			/* FALLTHRU */
		case kmp_sch_static_chunked:
			mpc_omp_static_loop_begin_ull( up, lb, b, incr, chunk, NULL, NULL );
			break;

		/* ORDERED AUTO OR STATIC */
		case kmp_ord_auto:
		case kmp_ord_static:
			chunk = 0;
			/* FALLTHRU */
		case kmp_ord_static_chunked:
			mpc_omp_ordered_static_loop_begin_ull( up, lb, b, incr, chunk, NULL, NULL );
			break;

		/* regular scheduling */
		case kmp_sch_dynamic_chunked:
			mpc_omp_loop_ull_dynamic_begin( up, lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_ord_dynamic_chunked:
			mpc_omp_loop_ull_ordered_dynamic_begin( up, lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_sch_guided_chunked:
			mpc_omp_loop_ull_guided_begin( up, lb, b, incr, chunk, NULL, NULL );
			break;

		case kmp_ord_guided_chunked:
			mpc_omp_loop_ull_guided_begin( up, lb, b, incr, chunk, NULL, NULL );
			break;

		/* runtime */
		case kmp_sch_runtime:
			mpc_omp_loop_ull_runtime_begin( up, lb, b, incr, NULL, NULL );
			break;

		case kmp_ord_runtime:
			mpc_omp_loop_ull_ordered_runtime_begin( up, lb, b, incr, NULL, NULL );
			break;

		default:
			mpc_common_debug_error( "Schedule not handled: %d\n", t->schedule_type );
			not_implemented();
			break;
	}
}

static inline int __intel_dispatch_next_long( mpc_omp_thread_t __UNUSED__ *t,
        long *from, long *to )
{
	int ret;

	switch ( t->schedule_type )
	{
		/* regular scheduling */
		case kmp_sch_auto:
		case kmp_sch_static:
		case kmp_sch_static_chunked:
			ret = mpc_omp_static_loop_next( from, to );
			mpc_common_nodebug( " from: %ld -- to: %ld", *from, *to );
			break;

		case kmp_ord_auto:
		case kmp_ord_static:
		case kmp_ord_static_chunked:
			ret = mpc_omp_ordered_static_loop_next( from, to );
			mpc_common_nodebug( " from: %ld -- to: %ld", *from, *to );
			break;

		/* dynamic */
		case kmp_sch_dynamic_chunked:
			ret = mpc_omp_dynamic_loop_next( from, to );
			break;

		case kmp_ord_dynamic_chunked:
			ret = mpc_omp_ordered_dynamic_loop_next( from, to );
			break;

		/* guided */
		case kmp_sch_guided_chunked:
			ret = mpc_omp_guided_loop_next( from, to );
			break;

		case kmp_ord_guided_chunked:
			ret = mpc_omp_ordered_guided_loop_next( from, to );
			break;

		/* runtime */
		case kmp_sch_runtime:
			ret = mpc_omp_runtime_loop_next( from, to );
			break;

		case kmp_ord_runtime:
			ret = mpc_omp_ordered_runtime_loop_next( from, to );
			break;

		default:
			not_implemented();
			break;
	}

	return ret;
}
static inline int __intel_dispatch_next_ull(
    mpc_omp_thread_t __UNUSED__ *t, unsigned long long *from, unsigned long long *to )
{
	int ret;

	switch ( t->schedule_type )
	{
		/* regular scheduling */
		case kmp_sch_auto:
		case kmp_sch_static:
		case kmp_sch_static_chunked:
			ret = mpc_omp_static_loop_next_ull( from, to );
			break;

		case kmp_ord_auto:
		case kmp_ord_static:
		case kmp_ord_static_chunked:
			ret = mpc_omp_ordered_static_loop_next_ull( from, to );
			break;

		/* dynamic */
		case kmp_sch_dynamic_chunked:
			ret = mpc_omp_loop_ull_dynamic_next( from, to );
			break;

		case kmp_ord_dynamic_chunked:
			ret = mpc_omp_loop_ull_ordered_dynamic_next( from, to );
			break;

		/* guided */
		case kmp_sch_guided_chunked:
			ret = mpc_omp_loop_ull_guided_next( from, to );
			break;

		case kmp_ord_guided_chunked:
			ret = mpc_omp_loop_ull_ordered_guided_next( from, to );
			break;

		/* runtime */
		case kmp_sch_runtime:
			ret = mpc_omp_loop_ull_runtime_next( from, to );
			break;

		case kmp_ord_runtime:
			ret = mpc_omp_loop_ull_ordered_runtime_next( from, to );
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
	mpc_omp_thread_t __UNUSED__ *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );

	switch ( t->schedule_type )
	{
		case kmp_sch_static:
		case kmp_sch_auto:
		case kmp_sch_static_chunked:
			mpc_omp_static_loop_end_nowait();
			break;

		case kmp_ord_static:
		case kmp_ord_auto:
		case kmp_ord_static_chunked:
			mpc_omp_ordered_static_loop_end_nowait();
			break;

		case kmp_sch_dynamic_chunked:
			mpc_omp_dynamic_loop_end_nowait();
			break;

		case kmp_ord_dynamic_chunked:
			mpc_omp_ordered_dynamic_loop_end_nowait();
			break;

		case kmp_sch_guided_chunked:
			mpc_omp_guided_loop_end_nowait();
			break;

		case kmp_ord_guided_chunked:
			mpc_omp_ordered_guided_loop_end_nowait();
			break;

		case kmp_sch_runtime:
			mpc_omp_runtime_loop_end_nowait();
			break;

		case kmp_ord_runtime:
			mpc_omp_ordered_runtime_loop_end_nowait();
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	mpc_common_nodebug( "[%d] %s: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d",
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	mpc_common_nodebug(
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	mpc_common_nodebug( "[%d] %s: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d",
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t );
	mpc_common_nodebug(
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	long from, to;
	mpc_omp_thread_t __UNUSED__ *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );
	assert( t->info.loop_infos.type == MPC_OMP_LOOP_TYPE_LONG );
	const long incr = t->info.loop_infos.loop.mpcomp_long.incr;
	mpc_common_nodebug( "%s: p_lb %ld, p_ub %ld, p_st %ld", __func__, *p_lb, *p_ub,
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t;
	unsigned long long from, to;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );
	assert( t->info.loop_infos.type == MPC_OMP_LOOP_TYPE_ULL );
	const unsigned long long incr = t->info.loop_infos.loop.mpcomp_ull.incr;
	mpc_common_nodebug( "%s: p_lb %llu, p_ub %llu, p_st %llu", __func__, *p_lb, *p_ub,
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	long from, to;
	mpc_omp_thread_t __UNUSED__ *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );
	assert( t->info.loop_infos.type == MPC_OMP_LOOP_TYPE_LONG );
	const long incr = t->info.loop_infos.loop.mpcomp_long.incr;
	mpc_common_nodebug( "%s: p_lb %ld, p_ub %ld, p_st %ld", __func__, *p_lb, *p_ub,
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t;
	unsigned long long from, to;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );
	assert( t->info.loop_infos.type == MPC_OMP_LOOP_TYPE_ULL );
	const unsigned long long incr = t->info.loop_infos.loop.mpcomp_ull.incr;
	mpc_common_nodebug( "%s: p_lb %llu, p_ub %llu, p_st %llu", __func__, *p_lb, *p_ub,
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
kmp_int32
__kmpc_omp_task(__UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, kmp_task_t * icc_task)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_task_t * task = (mpc_omp_task_t *) ((char *)icc_task - sizeof(mpc_omp_task_t));

#if OMPT_SUPPORT
    mpc_omp_thread_t * thread = (mpc_omp_thread_t *) mpc_omp_tls;
    _mpc_omp_ompt_callback_task_create(task, __mpc_omp_ompt_get_task_flags(thread, task), 0);
#endif /* OMPT_SUPPORT */
	return (kmp_int32) _mpc_omp_task_process(task);
}

void
__kmp_omp_task_wrapper(void * kmp_task_ptr)
{
    kmp_task_t * kmp_task = (kmp_task_t *) kmp_task_ptr;
    kmp_task->routine(0, kmp_task);
}

static mpc_omp_task_property_t
___convert_flags(kmp_tasking_flags_t * flags)
{
    mpc_omp_task_property_t properties = 0;

    if (flags->tiedness == 0)       mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_UNTIED);
    if (flags->final)               mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_FINAL);
    if (flags->priority_specified)  mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_PRIORITY);
    if (flags->task_serial)         mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_UNDEFERRED);

    /* MPC_OMP_TASK_PROP_FINAL and MPC_OMP_TASK_PROP_INCLUDED */
    if (mpc_omp_task_property_isset(properties, MPC_OMP_TASK_PROP_FINAL))
    {
        mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_INCLUDED);
        mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_FINAL);
        mpc_omp_task_set_property(&properties, MPC_OMP_TASK_PROP_UNDEFERRED);
    }

    return properties;
}

kmp_task_t *__kmpc_omp_task_alloc( __UNUSED__ ident_t *loc_ref, __UNUSED__  kmp_int32 gtid,
                                   kmp_int32 flags, size_t sizeof_kmp_task_t,
                                   size_t sizeof_shareds,
                                   kmp_routine_entry_t task_entry )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

    /* retrieve current thread */
    mpc_omp_thread_t * thread = ( mpc_omp_thread_t * )mpc_omp_tls;
    assert(thread);

    /* mpc_omp_task_t + kmp_task_t + shared variables) */
    const size_t size = sizeof(mpc_omp_task_t) + sizeof_kmp_task_t + sizeof_shareds;

    /* mpc_omp_task_t */
    kmp_tasking_flags_t * kmp_task_flags = (kmp_tasking_flags_t *) &flags;
    mpc_omp_task_property_t properties = ___convert_flags(kmp_task_flags);
    mpc_omp_task_t * task = _mpc_omp_task_allocate(size);
    assert(task);

    /* kmp_task_t / shared variables*/
    kmp_task_t * kmp_task = (kmp_task_t *)(task + 1);
    kmp_task->shareds = (sizeof_shareds > 0) ? (void *)(kmp_task + 1) : NULL;
    kmp_task->routine = task_entry;
    kmp_task->part_id = 0;

    /* Create new task */
    _mpc_omp_task_init(task, __kmp_omp_task_wrapper, kmp_task, size, properties);

    /* ? */
    thread->task_infos.sizeof_kmp_task_t = sizeof_kmp_task_t;

    /* to handle if0 with deps */
    MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread)->last_task_alloc = task;

    return kmp_task;
}

void __kmpc_omp_task_begin_if0( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                                kmp_task_t *task )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t = ( mpc_omp_thread_t * )mpc_omp_tls;
	struct mpc_omp_task_s *mpcomp_task = ( struct mpc_omp_task_s * ) (
	                                        ( char * )task - sizeof( struct mpc_omp_task_s ) );
	assert( t );
	mpcomp_task->icvs = t->info.icvs;
	mpcomp_task->prev_icvs = t->info.icvs;

    /* Task with if(0) clause, set to undeferred */
    mpc_omp_task_set_property(&(mpcomp_task->property), MPC_OMP_TASK_PROP_UNDEFERRED );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_task_create( mpcomp_task,
                                    __mpc_omp_ompt_get_task_flags( t, mpcomp_task ),
                                    0 );
#endif /* OMPT_SUPPORT */

	/* Because task code is between the current call and the next call
	 * __kmpc_omp_task_complete_if0 */
	MPC_OMP_TASK_THREAD_SET_CURRENT_TASK( t, mpcomp_task );
}

void
__kmpc_omp_task_complete_if0(
    __UNUSED__ ident_t *loc_ref,
    __UNUSED__  kmp_int32 gtid,
    kmp_task_t * kmp_task)
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

	mpc_omp_task_t * task = (mpc_omp_task_t *) ((char *)kmp_task - sizeof(mpc_omp_task_t));
    assert(task);

    /* TODO : unref / delete the task */

	MPC_OMP_TASK_THREAD_SET_CURRENT_TASK(thread, task->parent);
	thread->info.icvs = task->prev_icvs;
}

kmp_int32 __kmpc_omp_task_parts( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid,
                                 kmp_task_t *new_task )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	// TODO: Check if this is the correct way to implement __kmpc_omp_task_parts
	mpc_omp_task_t * task = (mpc_omp_task_t *)((char *)new_task - sizeof(mpc_omp_task_t));
    (void) task;

#if OMPT_SUPPORT
    mpc_omp_thread_t *t = (mpc_omp_thread_t *)mpc_omp_tls;
    _mpc_omp_ompt_callback_task_create(task, __mpc_omp_ompt_get_task_flags(t, task), 0);
#endif /* OMPT_SUPPORT */

	//_mpc_omp_task_process(task);
	return ( kmp_int32 )0;
}

kmp_int32 __kmpc_omp_taskwait( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	_mpc_omp_task_wait();
	return ( kmp_int32 )0;
}

kmp_int32 __kmpc_omp_taskyield( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, __UNUSED__ int end_part )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

    _mpc_omp_task_yield();

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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	_mpc_omp_task_taskgroup_start();
}

void __kmpc_end_taskgroup( __UNUSED__ ident_t *loc, __UNUSED__ int gtid )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	_mpc_omp_task_taskgroup_end();
}

static void
__intel_translate_taskdep_to_gomp(  kmp_int32 ndeps, kmp_depend_info_t *dep_list, kmp_int32 ndeps_noalias, __UNUSED__ kmp_depend_info_t *noalias_dep_list, void **gomp_list_deps )
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

TODO("1) do not translate: directly interpret dependencies");
TODO("2) add support for inoutset, mutexinoutset, and depobj");
kmp_int32 __kmpc_omp_task_with_deps( __UNUSED__ ident_t * loc_ref, __UNUSED__ kmp_int32 gtid,
                                     kmp_task_t * kmp_task, kmp_int32 ndeps,
                                     kmp_depend_info_t * dep_list,
                                     kmp_int32 ndeps_noalias,
                                     kmp_depend_info_t * noalias_dep_list )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

    /* the mpcomp task */
    mpc_omp_task_t * task = (mpc_omp_task_t *) ((char *)kmp_task - sizeof(mpc_omp_task_t));

    /* the user-given priority */
    int priority = (int) kmp_task->data1.priority;

    /* convert llvm dependencies format to gomp */
    void ** depend = (void **)mpc_omp_alloc(sizeof(uintptr_t) * ((int)(ndeps + ndeps_noalias) + 2));
    __intel_translate_taskdep_to_gomp(ndeps, dep_list, ndeps_noalias, noalias_dep_list, depend);
    mpc_omp_task_set_property(&task->property, MPC_OMP_TASK_PROP_DEPEND);

    /* register dependencies and priority */
    _mpc_omp_task_deps(task, depend, priority);

	kmp_int32 r = (kmp_int32) _mpc_omp_task_process(task);

    _mpc_omp_task_deinit(task);

    return r;
}

void __kmpc_omp_wait_deps( __UNUSED__ ident_t *loc_ref, __UNUSED__ kmp_int32 gtid, kmp_int32 ndeps,
                           kmp_depend_info_t *dep_list, kmp_int32 ndeps_noalias,
                           kmp_depend_info_t *noalias_dep_list )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

    mpc_omp_thread_t * thread = (mpc_omp_thread_t *)mpc_omp_tls;
    assert(thread);

    mpc_omp_task_t * current_task = MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread);
    assert(current_task);

    mpc_omp_task_t * task = current_task->last_task_alloc;
    assert(task);

    void ** depend = (void**)sctk_malloc(sizeof(uintptr_t) * ((int)(ndeps + ndeps_noalias)+2));
    __intel_translate_taskdep_to_gomp(ndeps, dep_list, ndeps_noalias, noalias_dep_list, depend);


	/* allocate empty task */
	mpc_omp_task_t * empty = _mpc_omp_task_allocate(sizeof(mpc_omp_task_t));

    /* init empty task */
    _mpc_omp_task_init(empty, NULL, NULL, sizeof(mpc_omp_task_t), current_task->property);

    /* set dependencies (and compute priorities) */
    _mpc_omp_task_deps(empty, depend, current_task->priority);



	while (OPA_load_int(&(empty->dep_node.ref_predecessors)))
    {
        /* Schedule any other task */
        _mpc_omp_task_schedule();
    }

    /* next call should be __kmpc_omp_task_begin_if0 to execute undeferred if0 task */
}

void __kmp_release_deps( __UNUSED__ kmp_int32 gtid, __UNUSED__ kmp_taskdata_t *task )
{
	//not_implemented();
}

typedef void( *p_task_dup_t )( kmp_task_t *, kmp_task_t *, kmp_int32 );

TODO("Check kmp taskloop implementation, maybe refactor this with GOMP one");
void __kmpc_taskloop( __UNUSED__ ident_t *loc, __UNUSED__ int gtid, kmp_task_t *task,
                      __UNUSED__ int if_val, kmp_uint64 *lb, kmp_uint64 *ub, kmp_int64 st,
                      int nogroup, int sched, kmp_uint64 grainsize, void *task_dup )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t = ( mpc_omp_thread_t * )mpc_omp_tls;
    assert(t);

    int lastpriv = 0;
    unsigned long i;
    unsigned long chunk;
    unsigned long trip_count;
    unsigned long num_tasks = 0;
    unsigned long extras = 0;

    size_t lower_offset = (char*)lb - (char*)task;
    size_t upper_offset = (char*)ub - (char*)task;
    unsigned long lower = *lb;
    unsigned long upper = *ub;

    p_task_dup_t ptask_dup = (p_task_dup_t) task_dup;

    mpc_omp_task_t *next_task;
    mpc_omp_task_t *mpcomp_task =
        (mpc_omp_task_t*) ( (char *) task - sizeof(mpc_omp_task_t) );

    if( nogroup == 0 )
      _mpc_omp_task_taskgroup_start();

    if( st == 1 )
      trip_count = upper - lower + 1;
    else if( st < 0 )
      trip_count = ( lower - upper ) / (-st) + 1;
    else
      trip_count = ( upper - lower ) / st + 1;

    mpc_common_nodebug( "[%d]%s: trip_count = %lu, sched = %d, nogroup = %d, taskdup %p",
                        t->rank, __func__, trip_count, sched, nogroup, task_dup );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_taskloop, ompt_scope_begin, trip_count );
    _mpc_omp_ompt_callback_task_create( mpcomp_task,
                                    __mpc_omp_ompt_get_task_flags( t, mpcomp_task ),
                                    0 );
#endif /* OMPT_SUPPORT */

	if ( !trip_count )
	{
#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_work( ompt_work_taskloop, ompt_scope_end, 0 );
#endif /* OMPT_SUPPORT */

		return;
	}

	switch ( sched )
	{
		case 0:
			grainsize = t->info.num_threads * 10;
			/* FALLTHRU */
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
			mpc_common_nodebug( "Unknown scheduling of taskloop" );
	}

    assert( trip_count == num_tasks * grainsize + extras );
    mpc_common_nodebug( "[%d] num tasks = %lu, grainsize = %lu, extra = %lu",
                        t->rank, num_tasks, grainsize, extras );

    if ( num_tasks > 1 ) {
        /* default pading */
        const long align_size = sizeof(void *);
        const long mpcomp_kmp_taskdata = sizeof(mpc_omp_task_t)
                                         + t->task_infos.sizeof_kmp_task_t;
        const long mpcomp_task_info_size =
            _mpc_omp_task_align_single_malloc(mpcomp_kmp_taskdata, align_size);

        /* Task total size */
        long mpc_omp_task_tot_size = mpcomp_task->size;

        for( i = 0; i < num_tasks -1; i++ )
        {
            next_task = (mpc_omp_task_t*) mpc_omp_alloc( mpc_omp_task_tot_size );
            assert( next_task );
            /* Copy from provided task */
            memcpy( next_task, mpcomp_task, mpc_omp_task_tot_size );

            /* Update next task fields */
            kmp_task_t *next_kmp_task = (kmp_task_t*)( (char *) next_task + sizeof(mpc_omp_task_t) );
            next_task->data = next_kmp_task;
            next_kmp_task->shareds = task->shareds ? (void *)((uintptr_t) next_task + mpcomp_task_info_size) : NULL; 

            OPA_incr_int(&(next_task->parent->ref_counter));
            MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(t)->last_task_alloc = next_task;

            if ( ptask_dup != NULL )
                ptask_dup( next_kmp_task, task, lastpriv );

            if ( nogroup == 0 ) {
                _mpc_omp_taskgroup_add_task( next_task );
            }

#if OMPT_SUPPORT
            _mpc_omp_ompt_callback_task_create( next_task, __mpc_omp_ompt_get_task_flags(t, next_task), 0);
#endif /* OMPT_SUPPORT */

            /* Set lower and upper values */
            if ( extras == 0 )
                chunk = grainsize - 1;
            else {
                chunk = grainsize;
                --extras;
            }

            upper = lower + st * chunk;
            *(kmp_uint64*)( (char*) next_kmp_task + lower_offset ) = lower;
            *(kmp_uint64*)( (char*) next_kmp_task + upper_offset ) = upper;

            mpc_common_nodebug( "[%d] Task %p; lb = %lu, ub = %lu",
                                t->rank, next_task, lower, upper );

            //_mpc_omp_task_process( next_task);

            lower = upper + st;
        }

        if ( nogroup == 0 ) {
            _mpc_omp_taskgroup_add_task( mpcomp_task );
        }

        /* Last task */
        lastpriv = 1;
        if ( ptask_dup != NULL )
            ptask_dup( task, task, lastpriv );

        /* Set lower and upper values for last task */
        upper = lower + st * chunk;
        *(kmp_uint64*)( (char*) task + lower_offset ) = lower;
        *(kmp_uint64*)( (char*) task + upper_offset ) = upper;

        mpc_common_nodebug( "[%d] Task %p; lb = %lu, ub = %lu",
                            t->rank, mpcomp_task, lower, upper );

        //_mpc_omp_task_process(mpcomp_task);
    }
    else {
        /* Only one task */
        lastpriv = 1;
        if ( ptask_dup != NULL )
            ptask_dup( task, task, lastpriv );

        *(kmp_uint64*)( (char*) task + lower_offset ) = lower;
        *(kmp_uint64*)( (char*) task + upper_offset ) = upper;

        mpc_common_nodebug( "[%d] Task %p; lb = %lu, ub = %lu",
                            t->rank, mpcomp_task, lower, upper );

        //_mpc_omp_task_process(mpcomp_task);
    }

    if ( nogroup == 0 )
        _mpc_omp_task_taskgroup_end();

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_taskloop, ompt_scope_end, 0 );
#endif /* OMPT_SUPPORT */
}

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
	static mpc_common_spinlock_t lock = MPC_COMMON_SPINLOCK_INITIALIZER;
	mpc_omp_thread_t __UNUSED__ *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	/* critical section */
	mpc_common_spinlock_lock( &lock );
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
	mpc_common_spinlock_unlock( &lock );
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
	mpc_omp_thread_t *t;
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	tn = __kmp_threadprivate_find_task_common( t->th_pri_common, global_tid, data );

	if ( !tn )
	{
		tn = kmp_threadprivate_insert( global_tid, data, data, size );
	}
	else
	{
		if ( ( size_t )size > tn->cmn_size )
		{
			mpc_common_debug_error(
			    "TPCommonBlockInconsist: -> Wong size threadprivate variable found" );
			mpc_common_debug_abort();
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
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_INTEL );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t __UNUSED__ *t; /* Info on the current thread */
	void **data_ptr;
	/* Grab the thread info */
	t = ( mpc_omp_thread_t * )mpc_omp_tls;
	assert( t != NULL );
	/* In this flow path, the number of threads should not be 1 */
	assert( t->info.num_threads > 0 );
	/* Grab the team info */
	assert( t->instance != NULL );
	assert( t->instance->team != NULL );
	data_ptr = &( t->instance->team->single_copyprivate_data );

	if ( didit )
	{
		*data_ptr = cpy_data;
	}

	mpc_omp_barrier();

	if ( !didit )
	{
		( *cpy_func )( cpy_data, *data_ptr );
	}

	mpc_omp_barrier();
}

void *__kmpc_threadprivate_cached( ident_t *loc, kmp_int32 global_tid,
                                   void *data, size_t size, void ***cache )
{
	static mpc_common_spinlock_t lock = MPC_COMMON_SPINLOCK_INITIALIZER;
	int __kmp_tp_capacity = __kmp_default_tp_capacity();

	if ( *cache == 0 )
	{
		mpc_common_spinlock_lock( &lock );

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

		mpc_common_spinlock_unlock( &lock );
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
