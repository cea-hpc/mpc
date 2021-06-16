/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "mpc_common_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_spinning_core.h"
#include "mpcomp_parallel_region.h"
#include "mpcomp_loop.h"
#include "mpcomp_task.h"
#include "mpcompt_parallel.h"
#include "mpcompt_frame.h"

/* Add header for spinning core */
mpcomp_instance_t* __mpcomp_tree_array_instance_init( mpcomp_thread_t*, const int);

void 
__mpcomp_internal_begin_parallel_region( mpcomp_parallel_region_t *info, const unsigned expected_num_threads ) 
{
    mpcomp_thread_t *t;

    unsigned real_num_threads;

    mpcomp_parallel_region_t* instance_info;

    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    assert(t != NULL);
   

    /* Compute new num threads value */
    if( t->root )
    {
        const unsigned int max_threads = t->root->tree_cumulative[0];
        if(expected_num_threads){
            real_num_threads = (expected_num_threads < max_threads ) ? expected_num_threads : max_threads;
        }
        else{
           real_num_threads = (t->info.icvs.nthreads_var < max_threads) ? t->info.icvs.nthreads_var : max_threads;
        }
    }
    else
    {
        real_num_threads = 1;
    }

    assert(real_num_threads > 0);
   
    if( !t->children_instance ||
        (t->children_instance && 
        t->children_instance->nb_mvps != real_num_threads ) )
    {
        t->children_instance = __mpcomp_tree_array_instance_init( t, real_num_threads );
        assert( t->children_instance );
    }
    else
    {
        t->mvp->threads = t->children_instance->master;
    }

    /* Switch to instance thread */
    mpcomp_instance_t* instance = t->children_instance;

#if OMPT_SUPPORT
    __mpcompt_callback_parallel_begin( real_num_threads,
                                       ompt_parallel_invoker_program | ompt_parallel_team );

#if MPCOMPT_HAS_FRAME_SUPPORT
    /* Copy to transfert frame infos */
    instance->team->frame_infos = t->frame_infos;
#endif
#endif /* OMPT_SUPPORT */

    instance_info = &( instance->team->info );

    instance_info->func = info->func;
    instance_info->shared = info->shared;
    instance_info->num_threads = real_num_threads;

    instance_info->combined_pragma = info->combined_pragma;
    instance_info->icvs = info->icvs;
    instance_info->icvs.levels_var = t->info.icvs.levels_var + 1;

    if( real_num_threads > 1 ) 
        instance_info->icvs.active_levels_var = t->info.icvs.active_levels_var + 1;

    __mpcomp_loop_gen_loop_infos_cpy( &(info->loop_infos), &(instance_info->loop_infos) );
    instance_info->nb_sections = info->nb_sections;

    instance->team->depth = t->instance->team->depth + 1;
    int i;
    for(i=0;i<MPCOMP_MAX_ALIVE_FOR_DYN + 1;i++)
      instance->team->is_first[i] = 1;

    if( !t->children_instance->buffered ) 
        __mpcomp_instance_tree_array_root_init( t->root, t->children_instance, instance_info->num_threads );

    _mpc_spin_node_wakeup( t->root );    

	return ;
}

void __mpcomp_internal_end_parallel_region( __UNUSED__ mpcomp_instance_t *instance) 
{
#if 0
    /* Grab the thread info */
    mpcomp_thread_t *t;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    assert(t != NULL);

    if( instance->team->info.num_threads > 1 ) {

      mpcomp_node_t *root;
      mpcomp_thread_t *master;
//    mpcomp_thread_t *prev = sctk_openmp_thread_tls;
//    sctk_openmp_thread_tls = master;

    /* Implicit barrier */
    //__mpcomp_internal_half_barrier( instance->mvps[0] ) ;
    //int nb_call = 0;

    fprintf(stderr, "[<>] Waitting for %d threads on node %p\n", root->barrier_num_threads, root );  

    /* Finish the half barrier by spinning on the root value */
    while (OPA_load_int(&(root->barrier)) !=
           root->barrier_num_threads) {
      mpc_thread_yield();
#ifdef MPCOMP_TASK
//      _mpc_task_schedule();
#endif /* MPCOMP_TASK */
    }

    OPA_store_int(&(root->barrier), 0);

    mpc_common_nodebug("%s: final barrier done...", __func__);

  }
  #endif

  /* Update team info for last values */
//  __mpcomp_save_team_info(instance->team, master);

#if MPCOMP_COHERENCY_CHECKING
//__mpcomp_for_dyn_coherency_end_parallel_region( instance ) ;
//__mpcomp_single_coherency_end_barrier() ;
#if MPCOMP_TASK
                _mpc_task_new_coherency_ending_parallel_region();
#endif // MPCOMP_TASK
#endif

#if OMPT_SUPPORT 
    __mpcompt_callback_parallel_end( ompt_parallel_invoker_program | ompt_parallel_team );
#endif /* OMPT_SUPPORT */
}

typedef void*(*mpcomp_start_func_t)(void*);

void __mpcomp_start_parallel_region(void (*func)(void *), void *shared,
                                    unsigned arg_num_threads) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
    __mpcompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

 	mpcomp_thread_t *t;
  	mpcomp_parallel_region_t info;

	mpcomp_start_func_t start = ( mpcomp_start_func_t ) func;
	assert( start );

  	__mpcomp_init();


  	t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
  	assert(t != NULL);

  	__mpcomp_parallel_region_infos_init(&info);
  	__mpcomp_parallel_set_specific_infos(&info, start, shared, t->info.icvs, MPCOMP_COMBINED_NONE);

	if( !( t->schedule_is_forced ) )
		 t->schedule_type = MPCOMP_COMBINED_NONE;
  	t->schedule_is_forced = 0;

  	__mpcomp_internal_begin_parallel_region(&info, arg_num_threads);
  	t->mvp->instance = t->children_instance;
  	__mpcomp_start_openmp_thread( t->mvp );  
  	__mpcomp_internal_end_parallel_region(t->children_instance);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

/****
 *
 * COMBINED VERSION 
 *
 *
 *****/

void __mpcomp_start_sections_parallel_region(void (*func)(void *), void *shared,
                                             unsigned arg_num_threads,
                                             unsigned nb_sections) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
    __mpcompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  	mpcomp_thread_t *t;
  	mpcomp_parallel_region_t info;

  	__mpcomp_init();

	mpcomp_start_func_t start = ( mpcomp_start_func_t ) func;
	assert( start );

  	t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  	assert(t != NULL);

  	__mpcomp_parallel_region_infos_init(&info);
  	__mpcomp_parallel_set_specific_infos(&info,start,shared,t->info.icvs,MPCOMP_COMBINED_SECTION);
  	info.nb_sections = nb_sections;

	if( !( t->schedule_is_forced ) )
		 t->schedule_type = MPCOMP_COMBINED_SECTION;
  	t->schedule_is_forced = 0;

  	__mpcomp_internal_begin_parallel_region(&info, arg_num_threads);
  	t->mvp->instance = t->children_instance;
  	__mpcomp_start_openmp_thread( t->mvp );  
  	__mpcomp_internal_end_parallel_region(t->children_instance);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __mpcomp_start_parallel_dynamic_loop(void (*func)(void *), void *shared,
                                          unsigned arg_num_threads, long lb,
                                          long b, long incr, long chunk_size) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
    __mpcompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  	mpcomp_thread_t *t;
 	mpcomp_parallel_region_t info;

	mpcomp_start_func_t start = ( mpcomp_start_func_t ) func;
	assert( start );

  	__mpcomp_init();

  	t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  	assert(t != NULL);

  	__mpcomp_parallel_region_infos_init(&info);
  	__mpcomp_parallel_set_specific_infos(&info,start, shared,
                                       t->info.icvs, MPCOMP_COMBINED_DYN_LOOP);

    assert( info.combined_pragma == MPCOMP_COMBINED_DYN_LOOP );
  	__mpcomp_loop_gen_infos_init(&(info.loop_infos), lb, b, incr, chunk_size);

	if( !( t->schedule_is_forced ) )
		 t->schedule_type = MPCOMP_COMBINED_DYN_LOOP;
  	t->schedule_is_forced = 0;

  	__mpcomp_internal_begin_parallel_region(&info, arg_num_threads);
  	t->mvp->instance = t->children_instance;
  	__mpcomp_start_openmp_thread( t->mvp );  
  	__mpcomp_internal_end_parallel_region(t->children_instance);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __mpcomp_start_parallel_static_loop(void (*func)(void *), void *shared,
                                         unsigned arg_num_threads, long lb,
                                         long b, long incr, long chunk_size) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
    __mpcompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  mpcomp_thread_t *t;
  mpcomp_parallel_region_t info;

	mpcomp_start_func_t start = ( mpcomp_start_func_t ) func;
	assert( start );

  	__mpcomp_init();

  	t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  	assert(t != NULL);

  	__mpcomp_parallel_region_infos_init(&info);
  	__mpcomp_parallel_set_specific_infos(&info, start, shared,
                                       t->info.icvs,
                                       MPCOMP_COMBINED_STATIC_LOOP);
  	__mpcomp_loop_gen_infos_init(&(info.loop_infos), lb, b, incr, chunk_size);

	if( !( t->schedule_is_forced ) )
		 t->schedule_type = MPCOMP_COMBINED_STATIC_LOOP;
  	t->schedule_is_forced = 0;

  	__mpcomp_internal_begin_parallel_region(&info, arg_num_threads);
  	t->mvp->instance = t->children_instance;
  	__mpcomp_start_openmp_thread( t->mvp );  
  	__mpcomp_internal_end_parallel_region(t->children_instance);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __mpcomp_start_parallel_guided_loop(void (*func)(void *), void *shared,
                                         unsigned arg_num_threads, long lb,
                                         long b, long incr, long chunk_size) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
    __mpcompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  mpcomp_thread_t *t;
 	mpcomp_parallel_region_t info;

	mpcomp_start_func_t start = ( mpcomp_start_func_t ) func;
	assert( start );

  	__mpcomp_init();

  	t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  	assert(t != NULL);

  	__mpcomp_parallel_region_infos_init(&info);
  	__mpcomp_parallel_set_specific_infos(&info,start, shared,
                                       t->info.icvs, MPCOMP_COMBINED_GUIDED_LOOP);

    assert( info.combined_pragma == MPCOMP_COMBINED_GUIDED_LOOP );
  	__mpcomp_loop_gen_infos_init(&(info.loop_infos), lb, b, incr, chunk_size);

	if( !( t->schedule_is_forced ) )
		 t->schedule_type = MPCOMP_COMBINED_GUIDED_LOOP;
  	t->schedule_is_forced = 0;

  	__mpcomp_internal_begin_parallel_region(&info, arg_num_threads);
  	t->mvp->instance = t->children_instance;
  	__mpcomp_start_openmp_thread( t->mvp );  
  	__mpcomp_internal_end_parallel_region(t->children_instance);

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_unset_no_reentrant();
#endif /* OMPT_SUPPORT */
}

void __mpcomp_start_parallel_runtime_loop(void (*func)(void *), void *shared,
                                          unsigned arg_num_threads, long lb,
                                          long b, long incr, long chunk_size) {
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    __mpcompt_frame_get_wrapper_infos( MPCOMP_GOMP );
    __mpcompt_frame_set_no_reentrant();
#endif /* OMPT_SUPPORT */

  mpcomp_thread_t *t; /* Info on the current thread */

  __mpcomp_init();

  /* Grab the thread info */
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert(t != NULL);

  switch (t->info.icvs.run_sched_var) {
  case omp_sched_static:
    __mpcomp_start_parallel_static_loop(func, shared, arg_num_threads, lb, b,
                                        incr, chunk_size);
    break;
  case omp_sched_dynamic:
    __mpcomp_start_parallel_dynamic_loop(func, shared, arg_num_threads, lb, b,
                                         incr, chunk_size);
    break;
  case omp_sched_guided:
    __mpcomp_start_parallel_guided_loop(func, shared, arg_num_threads, lb, b,
                                        incr, chunk_size);
    break;
  default:
    not_reachable();
  }
}

