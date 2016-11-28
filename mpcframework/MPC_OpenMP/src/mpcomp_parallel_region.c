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

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_core.h"
#include "mpcomp_barrier.h"

#include "mpcomp_parallel_region.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_structs.h"

void mpcomp_internal_begin_parallel_region( mpcomp_new_parallel_region_info_t* info, unsigned arg_num_threads ) 
{
  mpcomp_thread_t * t ;
  unsigned num_threads ;
  mpcomp_instance_t * instance ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

#if MPCOMP_COHERENCY_CHECKING
  __mpcomp_single_coherency_entering_parallel_region() ;
#if MPCOMP_TASK
  __mpcomp_task_coherency_entering_parallel_region();
#endif // MPCOMP_TASK
#endif

    /* Check if the children instance exists */
    if ( t->children_instance == NULL ) 
    {
	    mpcomp_team_t * new_team ;
        sctk_nodebug("%s: children instance is NULL, allocating...", __func__, t->info.num_threads);

        new_team = (mpcomp_team_t *)sctk_malloc(sizeof(mpcomp_team_t));
        sctk_assert(new_team != NULL);
        mpcomp_team_infos_init(new_team);

        t->children_instance = ( mpcomp_instance_t* ) sctk_malloc(sizeof(mpcomp_instance_t));
        sctk_assert(t->children_instance != NULL);
        mpcomp_instance_init(t->children_instance, 0, new_team);
    }

    /* Grab the instance for the future parallel region */
    instance = t->children_instance;
    sctk_assert(instance != NULL);
    sctk_assert(instance->team != NULL);

    /* Compute the number of threads for this parallel region */
    num_threads = (arg_num_threads == 0) ? t->info.icvs.nthreads_var: arg_num_threads;
    num_threads = sctk_min( num_threads, t->children_instance->nb_mvps ); 
    sctk_assert(num_threads > 0);

    sctk_nodebug("%s: Number of threads %d (default %d, #mvps %d arg:%d)", __func__,
                     num_threads, t->info->icvs.nthreads_var,
                     t->children_instance->nb_mvps, arg_num_threads);

    /* Fill information for the team */
    instance->team->info.func = info->func;
    instance->team->info.shared = info->shared;
    instance->team->info.num_threads = num_threads;
    instance->team->info.new_root = instance->root;
    /* Do not touch to single_sections_current_save and for_dyn_current_save */
    instance->team->info.combined_pragma = info->combined_pragma;
    instance->team->info.icvs = t->info.icvs;
    /* Update active_levels_var and levels_var accordingly */
    instance->team->info.icvs.levels_var = t->info.icvs.levels_var + 1;
    if (num_threads > 1) 
    {
        instance->team->info.icvs.active_levels_var += t->info.icvs.active_levels_var + 1;
    }

	instance->team->info.loop_lb = info->loop_lb ;
	instance->team->info.loop_b = info->loop_b ;
	instance->team->info.loop_incr = info->loop_incr ;
	instance->team->info.loop_chunk_size = info->loop_chunk_size ;
	instance->team->info.nb_sections = info->nb_sections ;

    sctk_nodebug( "__mpcomp_internal_begin_parallel_region: "
            "Level %d -> %d, Active level %d -> %d",
            t->info.icvs.levels_var, instance->team->info.icvs.levels_var,
            t->info.icvs.active_levels_var, instance->team->info.icvs.active_levels_var
            ) ;


	/* Compute depth */
	instance->team->depth = t->instance->team->depth + 1 ;

	if ( num_threads != 1 ) {
		mpcomp_node_t * n ;			/* Node for tree traversal */

		/* Get the root node of the main tree */
		n = instance->root ;
		sctk_assert( n != NULL ) ;

		/* Root is awake -> propagate the values to children */

#if MPCOMP_TRANSFER_INFO_ON_NODES
		n->info = instance->team->info ;
                sctk_nodebug("__mpcomp_internal_begin_parallel_region: "
                             "Node transfers ON");
#else
    sctk_nodebug("__mpcomp_internal_begin_parallel_region: "
                 "Node transfers OFF");
#endif

		/* Wake up children nodes (as the master) */
		n = __mpcomp_wakeup_node(1, n, num_threads, instance) ;

		/* Wake up children leaf */
		n = __mpcomp_wakeup_leaf( n, num_threads, instance ) ;

		__mpcomp_wakeup_mvp( n->children.leaf[0], n ) ;
	} else {

		sctk_nodebug( 
				"__mpcomp_internal_begin_parallel_region: "
				"1 thread -> fille MVPs 0 out of %d",
				instance->nb_mvps ) ;


		instance->mvps[0]->info = instance->team->info ;

		__mpcomp_wakeup_mvp(instance->mvps[0], NULL ) ;

	}

	return ;
}

void mpcomp_internal_end_parallel_region( mpcomp_instance_t * instance )
{
	mpcomp_node_t * root ;
	mpcomp_thread_t * master ;

  sctk_nodebug( 
		  "mpcomp_internal_end_parallel_region: entering..."
			) ;

	/* Grab the master thread of the ending parallel region */
	master = &(instance->mvps[0]->threads[0]) ;

	/* Grab the tree root of the ending parallel region */
	root = master->info.new_root ;

	__mpcomp_sendtosleep_mvp( instance->mvps[0] ) ;

	/* Someone to wait... */
	if ( instance->team->info.num_threads > 1 ) {

    mpcomp_thread_t* prev = sctk_openmp_thread_tls;
    sctk_openmp_thread_tls = master; 

    /* Implicit barrier */
    mpcomp_internal_half_barrier( instance->mvps[0] ) ;
    int nb_call = 0;
    /* Finish the half barrier by spinning on the root value */
    while (sctk_atomics_load_int( &(root->barrier) ) !=
					root->barrier_num_threads ) 
    {
	    sctk_thread_yield() ;
        mpcomp_task_schedule( 0 ); 
    }
    sctk_atomics_store_int( &(root->barrier) , 0 ) ;

    sctk_openmp_thread_tls = prev;
    sctk_nodebug( "%s: final barrier done...", __func__ );
	}

    /* Update team info for last values */
	__mpcomp_save_team_info( instance->team, master ) ;

#if MPCOMP_COHERENCY_CHECKING
		__mpcomp_for_dyn_coherency_end_parallel_region( instance ) ;
		__mpcomp_single_coherency_end_barrier() ;
#if MPCOMP_TASK
                __mpcomp_task_coherency_ending_parallel_region();
#endif // MPCOMP_TASK
#endif

}

void 
mpcomp_start_parallel_region(void (*func) (void *), void *shared, unsigned arg_num_threads ) 
{
    mpcomp_thread_t * t ;
	mpcomp_new_parallel_region_info_t* info;

    sctk_nodebug( "%s: === ENTER PARALLEL REGION ===", __func__ ); 

#ifdef MPCOMP_FORCE_PARALLEL_REGION_ALLOC
    info = sctk_malloc( sizeof( mpcomp_new_parallel_region_info_t ) );
    assert( info );
#else   /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */
    mpcomp_new_parallel_region_info_t noallocate_info;
    info = &noallocate_info;
#endif  /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */

    /* Initialize OpenMP environment */
    mpcomp_init();

    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL ) ;

	mpcomp_parallel_region_infos_init( info ) ;
    info->func = (void*(*)(void*)) func;
    info->shared = shared;
    info->icvs = t->info.icvs ;
    info->combined_pragma = MPCOMP_COMBINED_NONE;

    t->schedule_type = ( t->schedule_is_forced ) ? t->schedule_type : MPCOMP_COMBINED_NONE;  
    t->schedule_is_forced = 1;

    mpcomp_internal_begin_parallel_region( info, arg_num_threads );

    /* Start scheduling */
    mpcomp_in_order_scheduler(t->children_instance->mvps[0]) ;

	mpcomp_internal_end_parallel_region( t->children_instance ) ;

    sctk_nodebug( "%s: === EXIT PARALLEL REGION ===", __func__ );
}

/**
  Entry point for microVP in charge of passing information to other microVPs.
  Spinning on a specific node to wake up
 */
void * mpcomp_slave_mvp_node( void * arg ) {
  int index ;
  mpcomp_thread_t * t;
  mpcomp_mvp_t * mvp ; /* Current microVP */
  mpcomp_node_t * n ; /* Spinning node + Traversing node */
  int num_threads ;

  sctk_nodebug("%s: Entering...", __func__);

  /* Get the current microVP */
  mvp = (mpcomp_mvp_t *)arg ;
  sctk_assert( mvp != NULL ) ;

  while (mvp->enable) {
    int i ;

	/* Capture the node to spin on */
	n = mvp->to_run ;

    /* Spinning on the designed node */
    sctk_thread_wait_for_value_and_poll( (int*)&(n->slave_running), 1, NULL, NULL ) ;
    n->slave_running = 0 ;

	sctk_nodebug( "mpcomp_slave_mvp_node: +++ START parallel region +++" ) ;

#if MPCOMP_TRANSFER_INFO_ON_NODES
	sctk_assert( n->father != NULL ) ;
	num_threads = n->father->info.num_threads ;
#else
	sctk_assert( n->instance != NULL ) ;
	num_threads = n->instance->team->info.num_threads ;
#endif
	sctk_assert( num_threads > 0 ) ;


	/* Wake up children nodes */
	n = __mpcomp_wakeup_node(0, n, num_threads, n->instance) ;

    /* Wake up children leaf */
	n = __mpcomp_wakeup_leaf( n, num_threads, n->instance ) ;

	__mpcomp_wakeup_mvp( mvp, n ) ;

    /* Run */
    mpcomp_in_order_scheduler( mvp ) ;

    sctk_nodebug( "mpcomp_slave_mvp_node: end of in-order scheduling" ) ;

    __mpcomp_sendtosleep_mvp( mvp ) ;

    /* Implicit barrier */
    mpcomp_internal_half_barrier( mvp ) ;

  }

  return NULL ;
}

/**
  Entry point for microVP working on their own
  Spinning on a variables inside the microVP.
 */
void * mpcomp_slave_mvp_leaf( void * arg ) {
  mpcomp_mvp_t * mvp ;

  sctk_nodebug( "mpcomp_slave_mvp_leaf: Entering..." ) ;

  /* Grab the current microVP */
  mvp = (mpcomp_mvp_t *)arg ;

  /* Spin while this microVP is alive */
  while (mvp->enable) {
    int i ;
    int num_threads_mvp ;

    /* Spin for new parallel region */
    sctk_thread_wait_for_value_and_poll( (int*)&(mvp->slave_running), 1, NULL, NULL ) ;
    mvp->slave_running = 0 ;

	sctk_nodebug( "mpcomp_slave_mvp_leaf: +++ START parallel region +++" ) ;

	__mpcomp_wakeup_mvp( mvp, mvp->father ) ;

    /* Run */
    mpcomp_in_order_scheduler( mvp ) ;

    sctk_nodebug( "mpcomp_slave_mvp_leaf: +++ STOP +++" ) ;

    __mpcomp_sendtosleep_mvp( mvp ) ;

    /* Half barrier */
    mpcomp_internal_half_barrier( mvp ) ;
  }

  return NULL ;
}

/****
 *
 * COMBINED VERSION 
 *
 *
 *****/

void
mpcomp_start_sections_parallel_region( void (*func) (void *), void *shared, unsigned arg_num_threads, unsigned nb_sections )
{
    mpcomp_thread_t *t ;
	mpcomp_new_parallel_region_info_t* info;

    sctk_nodebug("%s: === ENTER PARALLEL REGION w/ %d SECTION(s) ===", __func__, nb_sections);

#ifdef MPCOMP_FORCE_PARALLEL_REGION_ALLOC
    info = sctk_malloc( sizeof( mpcomp_new_parallel_region_info_t ) );
    assert( info );
#else   /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */
    mpcomp_new_parallel_region_info_t noallocate_info;
    info = &noallocate_info;
#endif  /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */

    /* Initialize OpenMP environment */
    mpcomp_init();

    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    mpcomp_parallel_region_infos_init( info );
    info->func = (void* (*) (void*)) func;
    info->shared = shared;
    info->icvs = t->info.icvs;
    info->combined_pragma = MPCOMP_COMBINED_SECTION;
    info->nb_sections = nb_sections;

    mpcomp_internal_begin_parallel_region(info, arg_num_threads );

    /* Start scheduling */
    mpcomp_in_order_scheduler(t->children_instance->mvps[0]);

    mpcomp_internal_end_parallel_region(t->children_instance);

    sctk_nodebug("%s: === EXIT PARALLEL REGION w/ %d SECTION(s) ===\n", __func__, nb_sections );
}

void
mpcomp_start_parallel_dynamic_loop( void (*func) (void *), void *shared, unsigned arg_num_threads,
				      long lb, long b, long incr, long chunk_size)
{
    mpcomp_thread_t* t ;
	mpcomp_new_parallel_region_info_t* info ;

    sctk_nodebug( "%s: === ENTER PARALLEL REGION w/ DYN LOOP %ld -> %ld [%ld] cs:%ld ===", __func__, lb, b, incr, chunk_size);

#ifdef MPCOMP_FORCE_PARALLEL_REGION_ALLOC
    info = sctk_malloc( sizeof( mpcomp_new_parallel_region_info_t ) );
    assert( info );
#else   /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */
    mpcomp_new_parallel_region_info_t noallocate_info;
    info = &noallocate_info;
#endif  /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */

    /* Initialize OpenMP environment */
    mpcomp_init();

    /* Grab the thread info */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    mpcomp_parallel_region_infos_init( info );
    info->func = (void* (*) (void*)) func;
    info->shared = shared;
    info->icvs = t->info.icvs ;
    info->combined_pragma = MPCOMP_COMBINED_DYN_LOOP;
    info->loop_lb = lb;
    info->loop_b = b;
    info->loop_incr = incr;
    info->loop_chunk_size = chunk_size;

    mpcomp_internal_begin_parallel_region(info, arg_num_threads);

    /* Start scheduling */
    mpcomp_in_order_scheduler(t->children_instance->mvps[0]);

    mpcomp_internal_end_parallel_region(t->children_instance);

    sctk_nodebug( "%s: === EXIT PARALLEL REGION w/ DYN LOOP %ld -> %ld [%ld] cs:%ld ===", __func__, lb, b, incr, chunk_size);
}

void
mpcomp_start_parallel_static_loop( void (*func) (void *), void *shared, unsigned arg_num_threads,
				      long lb, long b, long incr, long chunk_size)
{
    mpcomp_thread_t* t ;
	mpcomp_new_parallel_region_info_t* info ;

    sctk_nodebug( "%s: === ENTER PARALLEL REGION w/ STATIC LOOP %ld -> %ld [%ld] cs:%ld ===", __func__, lb, b, incr, chunk_size);

#ifdef MPCOMP_FORCE_PARALLEL_REGION_ALLOC
    info = sctk_malloc( sizeof( mpcomp_new_parallel_region_info_t ) );
    assert( info );
#else   /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */
    mpcomp_new_parallel_region_info_t noallocate_info;
    info = &noallocate_info;
#endif  /* MPCOMP_FORCE_PARALLEL_REGION_ALLOC */

 
    /* Initialize OpenMP environment */
    mpcomp_init();

    /* Grab the thread info */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    mpcomp_parallel_region_infos_init( info );
    info->func = (void*(*)(void*)) func;
    info->shared = shared;
    info->icvs = t->info.icvs ;
    info->combined_pragma = MPCOMP_COMBINED_STATIC_LOOP;
    info->loop_lb = lb;
    info->loop_b = b;
    info->loop_incr = incr;
    info->loop_chunk_size = chunk_size;

    mpcomp_internal_begin_parallel_region( info, arg_num_threads );

    /* Start scheduling */
    mpcomp_in_order_scheduler(t->children_instance->mvps[0]);

    mpcomp_internal_end_parallel_region(t->children_instance);

    sctk_nodebug( "%s: === EXIT PARALLEL REGION w/ STATIC LOOP %ld -> %ld [%ld] cs:%ld ===", __func__, lb, b, incr, chunk_size);
}


void
mpcomp_start_parallel_guided_loop ( void (*func) (void *), void *shared, unsigned arg_num_threads,
				      long lb, long b, long incr, long chunk_size)
{
    mpcomp_start_parallel_dynamic_loop(func, shared, arg_num_threads, lb, b, incr, chunk_size);
}

void
mpcomp_start_parallel_runtime_loop( void (*func) (void *), void *shared, unsigned arg_num_threads,
		long lb, long b, long incr, long chunk_size)
{
	mpcomp_thread_t *t ;	/* Info on the current thread */

	/* Handle orphaned directive (initialize OpenMP environment) */
	mpcomp_init();

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	switch( t->info.icvs.run_sched_var ) {
		case omp_sched_static:
			mpcomp_start_parallel_static_loop( func, shared, arg_num_threads, lb, b, incr, chunk_size );
			break ;
		case omp_sched_dynamic:
			mpcomp_start_parallel_dynamic_loop( func, shared, arg_num_threads, lb, b, incr, chunk_size) ;
			break ;
		case omp_sched_guided:
			mpcomp_start_parallel_guided_loop( func, shared, arg_num_threads, lb, b, incr, chunk_size) ;
            break;
		default:
			not_reachable();
	}
}

/* GOMP OPTIMIZED_4_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
    __asm__(".symver mpcomp_start_parallel_region,          GOMP_parallel@@GOMP_4.0"); 
    __asm__(".symver mpcomp_start_sections_parallel_region, GOMP_parallel_sections@@GOMP_4.0"); 
    __asm__(".symver mpcomp_start_parallel_static_loop,     GOMP_parallel_loop_static@@GOMP_4.0"); 
    __asm__(".symver mpcomp_start_parallel_dynamic_loop,    GOMP_parallel_loop_dynamic@@GOMP_4.0"); 
    __asm__(".symver mpcomp_start_parallel_guided_loop,     GOMP_parallel_loop_guided@@GOMP_4.0"); 
    __asm__(".symver mpcomp_start_parallel_runtime_loop,    GOMP_parallel_loop_runtime@@GOMP_4.0"); 
#endif /* OPTIMIZED_GOMP_API_SUPPORT */
