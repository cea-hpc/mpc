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

#include "mpcomp_internal.h"


static inline void 
__mpcomp_save_team_info(
		mpcomp_team_t * team,
		mpcomp_thread_t * t ) 
{

	sctk_debug( "__mpcomp_save_team_info: saving single/section %d and for dyn %d",
			t->single_sections_current, t->for_dyn_current ) ;

	team->info.single_sections_current_save = t->single_sections_current ;
	team->info.for_dyn_current_save = t->for_dyn_current ;
}

static inline void 
__mpcomp_wakeup_mvp(
		mpcomp_mvp_t * mvp,
		mpcomp_node_t * n
		) 
{

	/* TODO check that this is correct (only 1 thread) */
	if ( n == NULL ) {
		/* mvp->info should be ok */

	} else {

	sctk_assert( n == mvp->father ) ;

#if MPCOMP_TRANSFER_INFO_ON_NODES
	mvp->info = n->info ;
#else
	mvp->info = n->instance->team->info ;
#endif
	}

    /* Update the total number of threads for this microVP */
    mvp->nb_threads = 1 ;

	mvp->threads[0].info = mvp->info ;
	mvp->threads[0].rank = mvp->min_index[mpcomp_global_icvs.affinity] ;
	mvp->threads[0].single_sections_current = 
		mvp->info.single_sections_current_save ;
	mvp->threads[0].for_dyn_current = 
		mvp->info.for_dyn_current_save ;

	sctk_nodebug( "__mpcomp_wakeup_mvp: "
			"value of single_sections_current %d and for_dyn_current %d",
			mvp->info.single_sections_current_save,
		 mvp->info.for_dyn_current_save ) ;

#if 0
	switch( mvp->threads[0].info.combined_pragma ) {
		case MPCOMP_COMBINED_NONE:
			sctk_debug( "__mpcomp_wakeup_mvp: No combined parallel" ) ;
			break ;
		case MPCOMP_COMBINED_SECTION:
			sctk_debug( "__mpcomp_wakeup_mvp: Combined parallel/sections w/ %d section(s)",
				   mvp->threads[0].info.nb_sections	) ;
			__mpcomp_sections_init( 
					&(mvp->threads[0]),
					mvp->threads[0].info.nb_sections ) ;
			break ;
		case MPCOMP_COMBINED_STATIC_LOOP:
			sctk_debug( "__mpcomp_wakeup_mvp: Combined parallel/loop" ) ;
			__mpcomp_static_loop_init(
					&(mvp->threads[0]),
					mvp->threads[0].info.loop_lb,
					mvp->threads[0].info.loop_b,
					mvp->threads[0].info.loop_incr,
					mvp->threads[0].info.loop_chunk_size
					) ;
			break ;
		case MPCOMP_COMBINED_DYN_LOOP:
			sctk_debug( "__mpcomp_wakeup_mvp: Combined parallel/loop" ) ;
			__mpcomp_dynamic_loop_init(
					&(mvp->threads[0]),
					mvp->threads[0].info.loop_lb,
					mvp->threads[0].info.loop_b,
					mvp->threads[0].info.loop_incr,
					mvp->threads[0].info.loop_chunk_size
					) ;
			break ;
		default:
			not_implemented() ;
			break ;
	}
#endif

}

static inline void
 __mpcomp_sendtosleep_mvp(
     mpcomp_mvp_t * mvp
     )
{
#if 0
	switch( mvp->threads[0].info.combined_pragma ) {
		case MPCOMP_COMBINED_NONE:
			sctk_debug( "__mpcomp_sendtosleep_mvp: No combined parallel" ) ;
			break ;
		case MPCOMP_COMBINED_SECTION:
			sctk_debug( "__mpcomp_sendtosleep_mvp: Combined parallel/sections w/ %d section(s)",
					mvp->threads[0].info.nb_sections	) ;
			break ;
		case MPCOMP_COMBINED_STATIC_LOOP:
			sctk_debug( "__mpcomp_sendtosleep_mvp: Combined parallel/loop" ) ;
			break ;
		case MPCOMP_COMBINED_DYN_LOOP:
			sctk_debug( "__mpcomp_sendtosleep_mvp: Combined parallel/loop" ) ;
			__mpcomp_dynamic_loop_end_nowait(
					&(mvp->threads[0])
					) ;
			break ;
		default:
			not_implemented() ;
			break ;
	}
#endif
}


static inline mpcomp_node_t * __mpcomp_wakeup_node(
		int master,
		mpcomp_node_t * start_node,
		int num_threads,
		mpcomp_instance_t * instance
		) {
	int i ;
	mpcomp_node_t * n ;

	n = start_node ;

	while ( n->child_type != MPCOMP_CHILDREN_LEAF ) {
		int nb_children_involved = 1 ;

		/* This part should keep two different loops because we first need to
		 * update barrier_num_threads before waking up the children to avoid
		 * one child to complete a barrier before waking up all children and
		 * getting a wrong barrier_num_threads value */

		/* Compute the number of children that will be involved for the barrier
		 */
		for ( i = 1 ; i < n->nb_children ; i++ ) {
			if ( n->children.node[i]->min_index[mpcomp_global_icvs.affinity] < 
                    num_threads ) {
				nb_children_involved++ ;
			}
		}

		sctk_nodebug( "__mpcomp_wakeup_node: nb_children_involved=%d", 
				nb_children_involved ) ;

		/* Update the number of threads for the barrier */
		n->barrier_num_threads = nb_children_involved ;

		if ( nb_children_involved == 1 ) {
            /* Bypass algorithm are ok with threads only with compact affinity
               Otherwise, with more than 1 thread, multiple subtree of the root
               will be used
             */
			if ( master && mpcomp_global_icvs.affinity == MPCOMP_AFFINITY_COMPACT ) {
				/* Only one subtree => bypass */
				instance->team->info.new_root = n->children.node[0] ;
				sctk_debug( "__mpcomp_wakeup_node: Bypassing root from depth %d to %d",
                       n->depth, n->depth+1 ) ;
			}
		} else {
			/* Wake up children and transfer information */
			for ( i = 1 ; i < n->nb_children ; i++ ) {
				if ( n->children.node[i]->min_index[mpcomp_global_icvs.affinity] 
                        < num_threads ) {
#if MPCOMP_TRANSFER_INFO_ON_NODES
					n->children.node[i]->info = n->info ;
#endif
					n->children.node[i]->slave_running = 1 ;
				}
			}
		}

		/* Transfer information for the first child */
#if MPCOMP_TRANSFER_INFO_ON_NODES
		n->children.node[0]->info = n->info ;
#endif

		/* Go down towards the leaves */
		n = n->children.node[0] ;
	}

	return n ;
}

static inline mpcomp_node_t * __mpcomp_wakeup_leaf(
		mpcomp_node_t * start_node,
		int num_threads,
		mpcomp_instance_t * instance
		) {

	int i ;
	mpcomp_node_t * n ;

	n = start_node ;

    /* Wake up children leaf */
    sctk_assert( n->child_type == MPCOMP_CHILDREN_LEAF ) ;
	int nb_children_involved = 1 ;
    for ( i = 1 ; i < n->nb_children ; i++ ) {
		if ( n->children.leaf[i]->min_index[mpcomp_global_icvs.affinity] < num_threads ) {
			nb_children_involved++ ;
		}
	}
	sctk_nodebug( "__mpcomp_wakeup_leaf: nb_leaves_involved=%d", 
			nb_children_involved ) ;
    n->barrier_num_threads = nb_children_involved ;
    for ( i = 1 ; i < n->nb_children ; i++ ) {
		if ( n->children.leaf[i]->min_index[mpcomp_global_icvs.affinity] < num_threads ) {
#if MPCOMP_TRANSFER_INFO_ON_NODES
			n->children.leaf[i]->info = n->info ;
#else
			n->children.leaf[i]->info = instance->team->info ;
#endif

			sctk_nodebug( "__mpcomp_wakeup_leaf: waking up leaf %d", 
					i ) ;

			n->children.leaf[i]->slave_running = 1 ;
		}
	}

	return n ;
}

inline void 
__mpcomp_internal_begin_parallel_region(int arg_num_threads, 
		mpcomp_new_parallel_region_info_t info )
{
  mpcomp_thread_t * t ;
  int num_threads ;
	mpcomp_instance_t * instance ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

#if MPCOMP_COHERENCY_CHECKING
  __mpcomp_single_coherency_entering_parallel_region() ;
#endif

	/* Check if the children instance exists */
	if ( t->children_instance == NULL ) {
		mpcomp_team_t * new_team ;

		sctk_debug( "__mpcomp_internal_begin_parallel_region: "
				"children instance is NULL, allocating...",
				t->info.num_threads	) ;

		new_team = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
		sctk_assert( new_team != NULL ) ;
		__mpcomp_team_init( new_team ) ;

		t->children_instance = 
			(mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
		sctk_assert( t->children_instance != NULL ) ;
		__mpcomp_instance_init( t->children_instance, 0, new_team ) ;
	}

	/* Grab the instance for the future parallel region */
	instance = t->children_instance ;
	sctk_assert( instance != NULL ) ;
	sctk_assert( instance->team != NULL ) ;

  /* Compute the number of threads for this parallel region */
	/* Default number of threads */
	num_threads = arg_num_threads ;

	/* -1 is default, and < 0 too */
	if ( num_threads < 0 ) {
		num_threads = t->info.icvs.nthreads_var ;
	}

	/* If number of threads too large, set to the max
	 * no oversubscribing */
	if ( num_threads > t->children_instance->nb_mvps ) {
		num_threads = t->children_instance->nb_mvps ;
	}

	sctk_assert( num_threads > 0 ) ;

  sctk_nodebug( 
		  "__mpcomp_internal_begin_parallel_region: "
		  "Number of threads %d (default %d, #mvps %d arg:%d)",
		  num_threads, 
		  t->info.icvs.nthreads_var, 
		  t->children_instance->nb_mvps, 
		  arg_num_threads ) ;

	/* Fill information for the team */
	instance->team->info.func = info.func ;
	instance->team->info.shared = info.shared ;
	instance->team->info.num_threads = num_threads ;
	instance->team->info.new_root = instance->root ;
	/* Do not touch to single_sections_current_save and for_dyn_current_save */
	instance->team->info.combined_pragma = info.combined_pragma ;
	instance->team->info.icvs = t->info.icvs ;
    /* Update active_levels_var and levels_var accordingly */
    instance->team->info.icvs.levels_var = t->info.icvs.levels_var+1;
    if ( num_threads > 1 ) 
    {
        instance->team->info.icvs.active_levels_var = 
            t->info.icvs.active_levels_var+1;
    }
	instance->team->info.loop_lb = info.loop_lb ;
	instance->team->info.loop_b = info.loop_b ;
	instance->team->info.loop_incr = info.loop_incr ;
	instance->team->info.loop_chunk_size = info.loop_chunk_size ;
	instance->team->info.nb_sections = info.nb_sections ;

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
		sctk_debug( "__mpcomp_internal_begin_parallel_region: "
				"Node transfers ON" ) ;
#else
		sctk_debug( "__mpcomp_internal_begin_parallel_region: "
				"Node transfers OFF" ) ;
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

#if 0
		instance->mvps[0]->nb_threads = 1 ;

		instance->mvps[0]->threads[0].info = info ;
		instance->mvps[0]->threads[0].info.num_threads = num_threads ;
		instance->mvps[0]->threads[0].info.new_root = instance->root ;
#endif


	}

	return ;
}

inline void 
__mpcomp_internal_end_parallel_region( mpcomp_instance_t * instance )
{
	mpcomp_node_t * root ;
	mpcomp_thread_t * master ;

  sctk_nodebug( 
		  "__mpcomp_internal_end_parallel_region: entering..."
			) ;

	/* Grab the master thread of the ending parallel region */
	master = &(instance->mvps[0]->threads[0]) ;

	/* Grab the tree root of the ending parallel region */
	root = master->info.new_root ;

	__mpcomp_sendtosleep_mvp( instance->mvps[0] ) ;

	/* Someone to wait... */
	if ( instance->team->info.num_threads > 1 ) {

    /* Implicit barrier */
    __mpcomp_internal_half_barrier( instance->mvps[0] ) ;

    /* Finish the half barrier by spinning on the root value */
    while (sctk_atomics_load_int( &(root->barrier) ) !=
					root->barrier_num_threads ) 
    {
#if MPCOMP_TASK
	 __mpcomp_task_schedule(); /* Look for tasks remaining */
#endif //MPCOMP_TASK
	 sctk_thread_yield() ;
    }
    sctk_atomics_store_int( &(root->barrier) , 0 ) ;

  sctk_nodebug( 
		  "__mpcomp_internal_end_parallel_region: final barrier done..."
			) ;

#if MPCOMP_TASK
    __mpcomp_task_schedule();
#endif //MPCOMP_TASK
	}

    /* Update team info for last values */
	__mpcomp_save_team_info( instance->team, master ) ;

#if MPCOMP_COHERENCY_CHECKING
		__mpcomp_for_dyn_coherency_end_parallel_region( instance ) ;
		__mpcomp_single_coherency_end_barrier() ;
#endif

}

void 
__mpcomp_start_parallel_region(int arg_num_threads, void *(*func)
    (void *), void *shared) 
{
  mpcomp_thread_t * t ;
	mpcomp_new_parallel_region_info_t info ;

  sctk_nodebug( 
      "__mpcomp_start_parallel_region: === ENTER PARALLEL REGION ===" ) ;

  /* Initialize OpenMP environment */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	__mpcomp_new_parallel_region_info_init( &info ) ;
	info.func = func ;
	info.shared = shared ;
	// info.icvs = t->info.icvs ; 
	info.combined_pragma = MPCOMP_COMBINED_NONE;

	__mpcomp_internal_begin_parallel_region( arg_num_threads, info ) ;

  sctk_nodebug( 
		  "__mpcomp_start_parallel_region: calling in order scheduler..."
			) ;

	/* Start scheduling */
	in_order_scheduler(t->children_instance->mvps[0]) ;

  sctk_nodebug( 
		  "__mpcomp_start_parallel_region: finish in order scheduler..."
			) ;

	__mpcomp_internal_end_parallel_region( t->children_instance ) ;

  sctk_nodebug( 
      "__mpcomp_start_parallel_region: === EXIT PARALLEL REGION ===\n" ) ;

}

void
__mpcomp_start_sections_parallel_region (int arg_num_threads,
		void *(*func) (void *), void *shared, int nb_sections)
{
  mpcomp_thread_t * t ;
	mpcomp_new_parallel_region_info_t info ;

  sctk_debug( 
      "__mpcomp_start_parallel_region: "
			"=== ENTER PARALLEL REGION w/ %d SECTION(s) ===", 
			nb_sections ) ;

  /* Initialize OpenMP environment */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	__mpcomp_new_parallel_region_info_init( &info ) ;
	info.func = func ;
	info.shared = shared ;
	// info.icvs = t->info.icvs ; 
	info.combined_pragma = MPCOMP_COMBINED_SECTION ;
	info.nb_sections = nb_sections ;

	__mpcomp_internal_begin_parallel_region( arg_num_threads, info ) ;

	/* Start scheduling */
	in_order_scheduler(t->children_instance->mvps[0]) ;

	__mpcomp_internal_end_parallel_region( t->children_instance ) ;

  sctk_debug( 
      "__mpcomp_start_parallel_region: "
			"=== EXIT PARALLEL REGION w/ %d SECTION(s) ===\n", 
			nb_sections ) ;
}

void
__mpcomp_start_parallel_dynamic_loop (int arg_num_threads,
				      void *(*func) (void *), void *shared,
				      long lb, long b, long incr, long chunk_size)
{
  mpcomp_thread_t * t ;
	mpcomp_new_parallel_region_info_t info ;

  sctk_debug( 
		  "__mpcomp_start_parallel_dynamic_loop:"
		  " === ENTER PARALLEL REGION w/ DYN LOOP %ld -> %ld [%ld] cs:%ld ===",
		  lb, b, incr, chunk_size ) ;

  /* Initialize OpenMP environment */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	__mpcomp_new_parallel_region_info_init( &info ) ;
	info.func = func ;
	info.shared = shared ;
	// info.icvs = t->info.icvs ; 
	info.combined_pragma = MPCOMP_COMBINED_DYN_LOOP ;
	info.loop_lb = lb ;
	info.loop_b = b ;
	info.loop_incr = incr ;
	info.loop_chunk_size = chunk_size ;

	__mpcomp_internal_begin_parallel_region( arg_num_threads, info ) ;

	/* Start scheduling */
	in_order_scheduler(t->children_instance->mvps[0]) ;

	__mpcomp_internal_end_parallel_region( t->children_instance ) ;

  sctk_debug( 
		  "__mpcomp_start_parallel_dynamic_loop:"
		  " === EXIT PARALLEL REGION w/ DYN LOOP %ld -> %ld [%ld] cs:%ld ===\n",
		  lb, b, incr, chunk_size ) ;
}

void
__mpcomp_start_parallel_static_loop (int arg_num_threads,
				      void *(*func) (void *), void *shared,
				      long lb, long b, long incr, long chunk_size)
{
  mpcomp_thread_t * t ;
	mpcomp_new_parallel_region_info_t info ;

  sctk_debug( 
		  "__mpcomp_start_parallel_static_loop:"
		  " === ENTER PARALLEL REGION w/ STATIC LOOP %ld -> %ld [%ld] cs:%ld ===",
		  lb, b, incr, chunk_size ) ;

  /* Initialize OpenMP environment */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	__mpcomp_new_parallel_region_info_init( &info ) ;
	info.func = func ;
	info.shared = shared ;
	// info.icvs = t->info.icvs ; 
	info.combined_pragma = MPCOMP_COMBINED_STATIC_LOOP ;
	info.loop_lb = lb ;
	info.loop_b = b ;
	info.loop_incr = incr ;
	info.loop_chunk_size = chunk_size ;

	__mpcomp_internal_begin_parallel_region( arg_num_threads, info ) ;

	/* Start scheduling */
	in_order_scheduler(t->children_instance->mvps[0]) ;

	__mpcomp_internal_end_parallel_region( t->children_instance ) ;

  sctk_debug( 
		  "__mpcomp_start_parallel_static_loop:"
		  " === EXIT PARALLEL REGION w/ STATIC LOOP %ld -> %ld [%ld] cs:%ld ===\n",
		  lb, b, incr, chunk_size ) ;
}

void 
OLD__mpcomp_start_parallel_region(int arg_num_threads, void *(*func)
    (void *), void *shared) {
  mpcomp_thread_t * t ;
  int num_threads ;

	 // printf( "!!!!!!!!!!! __mpcomp_start_parallel_region !!!!!!!!!!!!!!!\n" ) ;

  /* Initialize OpenMP environment */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_debug( 
		  "__mpcomp_start_parallel_region: === ENTER PARALLEL REGION ===" ) ;

#if MPCOMP_COHERENCY_CHECKING
  __mpcomp_single_coherency_entering_parallel_region() ;
#endif

  TODO("put #mvps as per-thread ICV and test it for the number of threads")

  /* Compute the number of threads for this parallel region */
  if ( t->children_instance == NULL  ){
	  /* Nested => 1 thread */
	  num_threads = 1 ;
  } else {
	  /* Default number of threads */
	  num_threads = arg_num_threads ;

	  /* -1 is default, and < 0 too */
	  if ( num_threads < 0 ) {
		  num_threads = t->info.icvs.nthreads_var ;
	  }

	  /* If number of threads too large, set to the max
	   * no oversubscribing */
	  if ( num_threads > t->children_instance->nb_mvps ) {
		  num_threads = t->children_instance->nb_mvps ;
	  }
  }

  sctk_debug( 
		  "__mpcomp_start_parallel_region: "
		  "Number of threads %d (default %d, #mvps %d arg:%d)",
		  num_threads, 
		  t->info.icvs.nthreads_var, 
		  (t->children_instance == NULL)?1:t->children_instance->nb_mvps, 
		  arg_num_threads ) ;


  /* First level of parallel region (no nesting and more than 1 thread) */
  if ( num_threads != 1 && t->instance->team->depth == 0 ) {
    mpcomp_instance_t * instance ;
    mpcomp_node_t * n ;			/* Temp node */
	int i ;

	/* Get the OpenMP instance already allocated during the initialization
	 * (mpcomp_init) */
    instance = t->children_instance ;
    sctk_assert( instance != NULL ) ;
    sctk_assert( instance->team != NULL ) ;

	/* Fill info for transfer */
	instance->team->info.func = func ;
	instance->team->info.shared = shared ;
	instance->team->info.num_threads = num_threads ;
	instance->team->info.new_root = instance->root ;
	instance->team->info.icvs = t->info.icvs ;
	instance->team->depth = t->instance->team->depth + 1 ;

    /* Get the root node of the main tree */
    n = instance->root ;
    sctk_assert( n != NULL ) ;

    /* Root is awake -> propagate the values to children */

#if MPCOMP_TRANSFER_INFO_ON_NODES
	n->info = instance->team->info ;
	sctk_debug( "__mpcomp_start_parallel_region: Node transfers ON" ) ;
#else
	sctk_debug( "__mpcomp_start_parallel_region: Node transfers OFF" ) ;
#endif

	/* Wake up children nodes (as the master) */
	n = __mpcomp_wakeup_node(1, n, num_threads, instance) ;

    /* Wake up children leaf */
	n = __mpcomp_wakeup_leaf( n, num_threads, instance ) ;

	__mpcomp_wakeup_mvp( n->children.leaf[0], n ) ;

    /* Start scheduling */
    in_order_scheduler(instance->mvps[0]) ;

    sctk_nodebug( 
	"[0] __mpcomp_start_parallel_region: end of in-order scheduling" ) ;

    /* Implicit barrier */
    __mpcomp_internal_half_barrier( instance->mvps[0] ) ;

    /* Finish the half barrier by spinning on the root value */
    while (sctk_atomics_load_int(
				&(n->children.leaf[0]->threads[0].info.new_root->barrier)) != 
			n->children.leaf[0]->threads[0].info.new_root->barrier_num_threads ) 
    {
#if MPCOMP_TASK
	 __mpcomp_task_schedule(); /* Look for tasks remaining */
#endif //MPCOMP_TASK
	 sctk_thread_yield() ;
    }
    sctk_atomics_store_int(
			&(n->children.leaf[0]->threads[0].info.new_root->barrier), 0) ;

#if MPCOMP_TASK
    __mpcomp_task_schedule();
#endif //MPCOMP_TASK

    /* Update team info for last values */
	__mpcomp_save_team_info( instance->team, &(n->children.leaf[0]->threads[0]) ) ;

    /* Restore the previous OpenMP info */
    sctk_openmp_thread_tls = t ;

#if 0
    /* Sequential check of tree coherency */
    sctk_assert( root->barrier == 0 ) ;
    for ( i = 0 ; i < root->nb_children ; i++ ) {
      sctk_assert( root->children.node[i]->barrier == 0 ) ;
    }
#endif

  } else {

    if ( mpcomp_global_icvs.warn_nested && t->instance->team->depth > 0 ) {
      mpcomp_warn_nested() ;
    }

	  /* Check whether the number of current thread is 1 or not */
#if 0
	  if ( t->info.num_threads == 1 ) {
		  mpcomp_instance_t * instance ;

		  sctk_debug( "__mpcomp_start_parallel_region: nested 1 thread within 1 thread" ) ;

		  instance = t->instance ;
		  sctk_assert( instance != NULL ) ;
		  sctk_assert( instance->team != NULL ) ;

		  instance->team->depth++ ;

		  func(shared) ;

		  instance->team->depth-- ;

	  } else 
#endif
	  {
		  mpcomp_instance_t * instance ;

		  sctk_debug( "__mpcomp_start_parallel_region: nested 1 thread within %d threads",
				  t->info.num_threads	) ;

		  /* Check if the children instance exists */
		  if ( t->children_instance == NULL ) {
			  mpcomp_team_t * new_team ;

			  sctk_debug( "__mpcomp_start_parallel_region: children instance is NULL, allocating...",
					  t->info.num_threads	) ;

			  new_team = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
			  sctk_assert( new_team != NULL ) ;
			  __mpcomp_team_init( new_team ) ;

			  t->children_instance = 
				  (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
			  sctk_assert( t->children_instance != NULL ) ;
			  __mpcomp_instance_init( t->children_instance, 0, new_team ) ;
		  }

		  instance = t->children_instance ;
		  sctk_assert( instance != NULL ) ;
		  sctk_assert( instance->team != NULL ) ;

		  /* Fill information for the team */
		  instance->team->info.func = func ;
		  instance->team->info.shared = shared ;
		  instance->team->info.num_threads = 1 ;
		  instance->team->info.new_root = NULL ;
		  instance->team->info.icvs = t->info.icvs ;
		  instance->team->depth = t->instance->team->depth + 1 ;

		  mpcomp_thread_t * target_t ;

		  sctk_debug( "__mpcomp_start_parallel_region: #mVPs for target instance = %d\n", instance->nb_mvps ) ;

		  target_t = &(instance->mvps[0]->threads[0]);
		  sctk_assert( target_t != NULL ) ;

		  /* Fill information for the target thread */
		  target_t->info = instance->team->info ;
		  target_t->rank = 0 ;

		  sctk_openmp_thread_tls = target_t ;

		  func(shared) ;

		  sctk_openmp_thread_tls = t;

	  }
  }

  sctk_nodebug( 
		  "__mpcomp_start_parallel_region: === EXIT PARALLEL REGION ===" ) ;

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

  sctk_debug( "mpcomp_slave_mvp_node: Entering..." ) ;

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
    in_order_scheduler( mvp ) ;

    sctk_nodebug( "mpcomp_slave_mvp_node: end of in-order scheduling" ) ;

    __mpcomp_sendtosleep_mvp( mvp ) ;

    /* Implicit barrier */
    __mpcomp_internal_half_barrier( mvp ) ;

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
    in_order_scheduler( mvp ) ;

    sctk_nodebug( "mpcomp_slave_mvp_leaf: +++ STOP +++" ) ;

    __mpcomp_sendtosleep_mvp( mvp ) ;

    /* Half barrier */
    __mpcomp_internal_half_barrier( mvp ) ;
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
OLD__mpcomp_start_sections_parallel_region (int arg_num_threads,
		void *(*func) (void *), void *shared, int nb_sections)
{


  mpcomp_thread_t * t ;
  int num_threads ;

  /* Initialize OpenMP environment */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

	sctk_debug( 
			"__mpcomp_start_sections_parallel_region:"
			" === ENTER PARALLEL REGION w/ %d SECTION(S) ===",
		   nb_sections	) ;

	// not_implemented() ;

#if MPCOMP_COHERENCY_CHECKING
  __mpcomp_single_coherency_entering_parallel_region() ;
#endif

  TODO("put #mvps as per-thread ICV and test it for the number of threads")

  /* Compute the number of threads for this parallel region */
  if ( t->children_instance == NULL  ){
	  /* Nested => 1 thread */
	  num_threads = 1 ;
  } else {
	  /* Default number of threads */
	  num_threads = arg_num_threads ;

	  /* -1 is default, and < 0 too */
	  if ( num_threads < 0 ) {
		  num_threads = t->info.icvs.nthreads_var ;
	  }

	  /* If number of threads too large, set to the max
	   * no oversubscribing */
	  if ( num_threads > t->children_instance->nb_mvps ) {
		  num_threads = t->children_instance->nb_mvps ;
	  }
  }

  sctk_debug( 
		  "__mpcomp_start_sections_parallel_region:"
		  " Number of threads %d (default %d)",
		  num_threads, t->info.icvs.nthreads_var ) ;


  /* First level of parallel region (no nesting and more than 1 thread) */
  if ( num_threads != 1 && t->instance->team->depth == 0 ) {
    mpcomp_instance_t * instance ;
    mpcomp_node_t * n ;			/* Temp node */
	int i ;

	/* Get the OpenMP instance already allocated during the initialization
	 * (mpcomp_init) */
    instance = t->children_instance ;
    sctk_assert( instance != NULL ) ;
    sctk_assert( instance->team != NULL ) ;

	/* Fill info for transfer */
	instance->team->info.func = func ;
	instance->team->info.shared = shared ;
	instance->team->info.num_threads = num_threads ;
	instance->team->info.new_root = instance->root ;
	instance->team->info.icvs = t->info.icvs ;
	instance->team->info.combined_pragma = MPCOMP_COMBINED_SECTION ;
	instance->team->info.nb_sections = nb_sections ;
	instance->team->depth = t->instance->team->depth + 1 ;


    /* Get the root node of the main tree */
    n = instance->root ;
    sctk_assert( n != NULL ) ;

    /* Root is awake -> propagate the values to children */

#if MPCOMP_TRANSFER_INFO_ON_NODES
	n->info = instance->team->info ;
	sctk_debug( "__mpcomp_start_sections_parallel_region: Node transfers ON" ) ;
#else
	sctk_debug( "__mpcomp_start_sections_parallel_region: Node transfers OFF" ) ;
#endif

	/* Wake up children nodes (as the master) */
	n = __mpcomp_wakeup_node(1, n, num_threads, instance) ;

    /* Wake up children leaf */
	n = __mpcomp_wakeup_leaf( n, num_threads, instance ) ;

	__mpcomp_wakeup_mvp( n->children.leaf[0], n ) ;

    /* Start scheduling */
    in_order_scheduler(instance->mvps[0]) ;

    sctk_nodebug( 
			"__mpcomp_start_sections_parallel_region: end of in-order scheduling" ) ;

    /* Implicit barrier */
    __mpcomp_internal_half_barrier( instance->mvps[0] ) ;

    /* Finish the half barrier by spinning on the root value */
    while (sctk_atomics_load_int(
				&(n->children.leaf[0]->threads[0].info.new_root->barrier)) != 
			n->children.leaf[0]->threads[0].info.new_root->barrier_num_threads ) 
    {
#if MPCOMP_TASK
	 __mpcomp_task_schedule(); /* Look for tasks remaining */
#endif //MPCOMP_TASK
	 sctk_thread_yield() ;
    }
    sctk_atomics_store_int(
			&(n->children.leaf[0]->threads[0].info.new_root->barrier), 0) ;

#if MPCOMP_TASK
    __mpcomp_task_schedule();
#endif //MPCOMP_TASK

    /* Update team info for last values */
	__mpcomp_save_team_info( instance->team, &(n->children.leaf[0]->threads[0]) ) ;

    /* Restore the previous OpenMP info */
    sctk_openmp_thread_tls = t ;

#if 0
    /* Sequential check of tree coherency */
    sctk_assert( root->barrier == 0 ) ;
    for ( i = 0 ; i < root->nb_children ; i++ ) {
      sctk_assert( root->children.node[i]->barrier == 0 ) ;
    }
#endif

  } else {

    if ( mpcomp_global_icvs.warn_nested && t->instance->team->depth > 0 ) {
      mpcomp_warn_nested() ;
    }

	  /* Check whether the number of current thread is 1 or not */
#if 0
	  if ( t->info.num_threads == 1 ) {
		  mpcomp_instance_t * instance ;

		  sctk_debug( "__mpcomp_start_sections_parallel_region:"
				 " nested 1 thread within 1 thread" ) ;

		  instance = t->instance ;
		  sctk_assert( instance != NULL ) ;
		  sctk_assert( instance->team != NULL ) ;

		  instance->team->depth++ ;

		  __mpcomp_sections_init(t,nb_sections) ;
		  func(shared) ;

		  instance->team->depth-- ;

	  } else 
#endif
	  {
		  mpcomp_instance_t * instance ;

		  sctk_debug( "__mpcomp_start_sections_parallel_region:"
				  " nested 1 thread within %d threads",
				  t->info.num_threads	) ;

		  /* Check if the children instance exists */
		  if ( t->children_instance == NULL ) {
			  mpcomp_team_t * new_team ;

			  sctk_debug( "__mpcomp_start_sections_parallel_region:"
					  " children instance is NULL, allocating...",
					  t->info.num_threads	) ;

			  new_team = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
			  sctk_assert( new_team != NULL ) ;
			  __mpcomp_team_init( new_team ) ;

			  t->children_instance = 
				  (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
			  sctk_assert( t->children_instance != NULL ) ;
			  __mpcomp_instance_init( t->children_instance, 0, new_team ) ;
		  }

		  instance = t->children_instance ;
		  sctk_assert( instance != NULL ) ;
		  sctk_assert( instance->team != NULL ) ;

		  /* Fill information for the team */
		  instance->team->info.func = func ;
		  instance->team->info.shared = shared ;
		  instance->team->info.num_threads = 1 ;
		  instance->team->info.new_root = NULL ;
		  instance->team->info.icvs = t->info.icvs ;
		  instance->team->depth = t->instance->team->depth + 1 ;

		  mpcomp_thread_t * target_t ;

		  sctk_debug( "__mpcomp_start_sections_parallel_region:"
				  " #mVPs for target instance = %d", instance->nb_mvps ) ;

		  target_t = &(instance->mvps[0]->threads[0]);
		  sctk_assert( target_t != NULL ) ;

		  /* Fill information for the target thread */
		  target_t->info = instance->team->info ;
		  target_t->rank = 0 ;

		  sctk_openmp_thread_tls = target_t ;

		  __mpcomp_sections_init(target_t,nb_sections) ;
		  func(shared) ;

		  sctk_openmp_thread_tls = t;

	  }
  }

  sctk_nodebug( 
		  "__mpcomp_start_sections_parallel_region:"
		  " === EXIT PARALLEL REGION ===" ) ;

}

void
OLD__mpcomp_start_parallel_dynamic_loop (int arg_num_threads,
				      void *(*func) (void *), void *shared,
				      long lb, long b, long incr, long chunk_size)
{
  mpcomp_thread_t * t ;
  int num_threads ;

  /* Initialize OpenMP environment */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_debug( 
		  "__mpcomp_start_parallel_dynamic_loop:"
		  " === ENTER PARALLEL REGION w/ DYN LOOP %ld -> %ld [%ld] cs:%ld ===",
		  lb, b, incr, chunk_size ) ;

#if MPCOMP_COHERENCY_CHECKING
  __mpcomp_single_coherency_entering_parallel_region() ;
#endif

  TODO("put #mvps as per-thread ICV and test it for the number of threads")

  /* Compute the number of threads for this parallel region */
  if ( t->children_instance == NULL  ){
	  /* Nested => 1 thread */
	  num_threads = 1 ;
  } else {
	  /* Default number of threads */
	  num_threads = arg_num_threads ;

	  /* -1 is default, and < 0 too */
	  if ( num_threads < 0 ) {
		  num_threads = t->info.icvs.nthreads_var ;
	  }

	  /* If number of threads too large, set to the max
	   * no oversubscribing */
	  if ( num_threads > t->children_instance->nb_mvps ) {
		  num_threads = t->children_instance->nb_mvps ;
	  }
  }

  sctk_debug( 
		  "__mpcomp_start_parallel_dynamic_loop:"
		  " Number of threads %d (default %d)",
		  num_threads, t->info.icvs.nthreads_var ) ;


  /* First level of parallel region (no nesting and more than 1 thread) */
  if ( num_threads != 1 && t->instance->team->depth == 0 ) {
    mpcomp_instance_t * instance ;
    mpcomp_node_t * n ;			/* Temp node */
	int i ;

	/* Get the OpenMP instance already allocated during the initialization
	 * (mpcomp_init) */
    instance = t->children_instance ;
    sctk_assert( instance != NULL ) ;
    sctk_assert( instance->team != NULL ) ;

	/* Fill info for transfer */
	instance->team->info.func = func ;
	instance->team->info.shared = shared ;
	instance->team->info.num_threads = num_threads ;
	instance->team->info.new_root = instance->root ;
	instance->team->info.icvs = t->info.icvs ;
	instance->team->info.combined_pragma = MPCOMP_COMBINED_DYN_LOOP ;
	instance->team->info.loop_lb = lb ;
	instance->team->info.loop_b = b ;
	instance->team->info.loop_incr = incr ;
	instance->team->info.loop_chunk_size = chunk_size ;
	instance->team->depth = t->instance->team->depth + 1 ;


    /* Get the root node of the main tree */
    n = instance->root ;
    sctk_assert( n != NULL ) ;

    /* Root is awake -> propagate the values to children */

#if MPCOMP_TRANSFER_INFO_ON_NODES
	n->info = instance->team->info ;
	sctk_debug( "__mpcomp_start_parallel_dynamic_loop: Node transfers ON" ) ;
#else
	sctk_debug( "__mpcomp_start_parallel_dynamic_loop: Node transfers OFF" ) ;
#endif

	/* Wake up children nodes (as the master) */
	n = __mpcomp_wakeup_node(1, n, num_threads, instance) ;

    /* Wake up children leaf */
	n = __mpcomp_wakeup_leaf( n, num_threads, instance ) ;

	__mpcomp_wakeup_mvp( n->children.leaf[0], n ) ;

    /* Start scheduling */
    in_order_scheduler(instance->mvps[0]) ;

    sctk_nodebug( 
			"__mpcomp_start_parallel_dynamic_loop: end of in-order scheduling" ) ;

    /* Implicit barrier */
    __mpcomp_internal_half_barrier( instance->mvps[0] ) ;

    /* Finish the half barrier by spinning on the root value */
    while (sctk_atomics_load_int(
				&(n->children.leaf[0]->threads[0].info.new_root->barrier)) != 
			n->children.leaf[0]->threads[0].info.new_root->barrier_num_threads ) 
    {

#if MPCOMP_TASK
	 __mpcomp_task_schedule(); /* Look for tasks remaining */
#endif //MPCOMP_TASK
	 sctk_thread_yield() ;
    }
    sctk_atomics_store_int(
			&(n->children.leaf[0]->threads[0].info.new_root->barrier), 0) ;

#if MPCOMP_TASK
    __mpcomp_task_schedule();
#endif //MPCOMP_TASK

    /* Update team info for last values */
	__mpcomp_save_team_info( instance->team, &(n->children.leaf[0]->threads[0]) ) ;

	__mpcomp_dynamic_loop_end_nowait() ;

    /* Restore the previous OpenMP info */
    sctk_openmp_thread_tls = t ;

#if 0
    /* Sequential check of tree coherency */
    sctk_assert( root->barrier == 0 ) ;
    for ( i = 0 ; i < root->nb_children ; i++ ) {
      sctk_assert( root->children.node[i]->barrier == 0 ) ;
    }
#endif

  } else {

    if ( mpcomp_global_icvs.warn_nested && t->instance->team->depth > 0 ) {
      mpcomp_warn_nested() ;
    }

	  /* Check whether the number of current thread is 1 or not */
#if 0
	  if ( t->info.num_threads == 1 ) {
		  mpcomp_instance_t * instance ;

		  sctk_debug( "__mpcomp_start_parallel_dynamic_loop:"
				 " nested 1 thread within 1 thread" ) ;

		  instance = t->instance ;
		  sctk_assert( instance != NULL ) ;
		  sctk_assert( instance->team != NULL ) ;

		  instance->team->depth++ ;

		  __mpcomp_dynamic_loop_init(t,lb,b,incr,chunk_size);
		  func(shared) ;

		  instance->team->depth-- ;

	  } else 
#endif
	  {
		  mpcomp_instance_t * instance ;

		  sctk_debug( "__mpcomp_start_parallel_dynamic_loop:"
				  " nested 1 thread within %d threads",
				  t->info.num_threads	) ;

		  /* Check if the children instance exists */
		  if ( t->children_instance == NULL ) {
			  mpcomp_team_t * new_team ;

			  sctk_debug( "__mpcomp_start_parallel_dynamic_loop:"
					  " children instance is NULL, allocating...",
					  t->info.num_threads	) ;

			  new_team = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
			  sctk_assert( new_team != NULL ) ;
			  __mpcomp_team_init( new_team ) ;

			  t->children_instance = 
				  (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
			  sctk_assert( t->children_instance != NULL ) ;
			  __mpcomp_instance_init( t->children_instance, 0, new_team ) ;
		  }

		  instance = t->children_instance ;
		  sctk_assert( instance != NULL ) ;
		  sctk_assert( instance->team != NULL ) ;

		  /* Fill information for the team */
		  instance->team->info.func = func ;
		  instance->team->info.shared = shared ;
		  instance->team->info.num_threads = 1 ;
		  instance->team->info.new_root = NULL ;
		  instance->team->info.icvs = t->info.icvs ;
		  instance->team->depth = t->instance->team->depth + 1 ;

		  mpcomp_thread_t * target_t ;

		  sctk_debug( "__mpcomp_start_parallel_dynamic_loop:"
				  " #mVPs for target instance = %d\n", instance->nb_mvps ) ;

		  target_t = &(instance->mvps[0]->threads[0]);
		  sctk_assert( target_t != NULL ) ;

		  /* Fill information for the target thread */
		  target_t->info = instance->team->info ;
		  target_t->rank = 0 ;

		  sctk_openmp_thread_tls = target_t ;

		  __mpcomp_dynamic_loop_init(target_t,lb,b,incr,chunk_size);
		  func(shared) ;

		  __mpcomp_dynamic_loop_end_nowait();
		  sctk_openmp_thread_tls = t;

	  }
  }

  sctk_debug( 
		  "__mpcomp_start_parallel_dynamic_loop:"
		  " === EXIT PARALLEL REGION ===" ) ;

}

void
OLD__mpcomp_start_parallel_static_loop (int arg_num_threads,
				      void *(*func) (void *), void *shared,
				      long lb, long b, long incr, long chunk_size)
{
  mpcomp_thread_t * t ;
  int num_threads ;

  /* Initialize OpenMP environment */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_debug( 
		  "__mpcomp_start_parallel_static_loop:"
		  " === ENTER PARALLEL REGION w/ STATIC LOOP %ld -> %ld [%ld] cs:%ld ===",
		  lb, b, incr, chunk_size ) ;

#if MPCOMP_COHERENCY_CHECKING
  __mpcomp_single_coherency_entering_parallel_region() ;
#endif

  TODO("put #mvps as per-thread ICV and test it for the number of threads")

  /* Compute the number of threads for this parallel region */
  if ( t->children_instance == NULL  ){
	  /* Nested => 1 thread */
	  num_threads = 1 ;
  } else {
	  /* Default number of threads */
	  num_threads = arg_num_threads ;

	  /* -1 is default, and < 0 too */
	  if ( num_threads < 0 ) {
		  num_threads = t->info.icvs.nthreads_var ;
	  }

	  /* If number of threads too large, set to the max
	   * no oversubscribing */
	  if ( num_threads > t->children_instance->nb_mvps ) {
		  num_threads = t->children_instance->nb_mvps ;
	  }
  }

  sctk_debug( 
		  "__mpcomp_start_parallel_static_loop:"
		  " Number of threads %d (default %d)",
		  num_threads, t->info.icvs.nthreads_var ) ;


  /* First level of parallel region (no nesting and more than 1 thread) */
  if ( num_threads != 1 && t->instance->team->depth == 0 ) {
    mpcomp_instance_t * instance ;
    mpcomp_node_t * n ;			/* Temp node */
	int i ;

	/* Get the OpenMP instance already allocated during the initialization
	 * (mpcomp_init) */
    instance = t->children_instance ;
    sctk_assert( instance != NULL ) ;
    sctk_assert( instance->team != NULL ) ;

	/* Fill info for transfer */
	instance->team->info.func = func ;
	instance->team->info.shared = shared ;
	instance->team->info.num_threads = num_threads ;
	instance->team->info.new_root = instance->root ;
	instance->team->info.icvs = t->info.icvs ;
	instance->team->info.combined_pragma = MPCOMP_COMBINED_STATIC_LOOP ;
	instance->team->info.loop_lb = lb ;
	instance->team->info.loop_b = b ;
	instance->team->info.loop_incr = incr ;
	instance->team->info.loop_chunk_size = chunk_size ;
	instance->team->depth = t->instance->team->depth + 1 ;


    /* Get the root node of the main tree */
    n = instance->root ;
    sctk_assert( n != NULL ) ;

    /* Root is awake -> propagate the values to children */

#if MPCOMP_TRANSFER_INFO_ON_NODES
	n->info = instance->team->info ;
	sctk_debug( "__mpcomp_start_parallel_static_loop: Node transfers ON" ) ;
#else
	sctk_debug( "__mpcomp_start_parallel_static_loop: Node transfers OFF" ) ;
#endif

	/* Wake up children nodes (as the master) */
	n = __mpcomp_wakeup_node(1, n, num_threads, instance) ;

    /* Wake up children leaf */
	n = __mpcomp_wakeup_leaf( n, num_threads, instance ) ;

	__mpcomp_wakeup_mvp( n->children.leaf[0], n ) ;

    /* Start scheduling */
    in_order_scheduler(instance->mvps[0]) ;

    sctk_nodebug( 
			"__mpcomp_start_parallel_static_loop: end of in-order scheduling" ) ;

    /* Implicit barrier */
    __mpcomp_internal_half_barrier( instance->mvps[0] ) ;

    /* Finish the half barrier by spinning on the root value */
    while (sctk_atomics_load_int(
				&(n->children.leaf[0]->threads[0].info.new_root->barrier)) != 
			n->children.leaf[0]->threads[0].info.new_root->barrier_num_threads ) 
    {
#if MPCOMP_TASK
	 __mpcomp_task_schedule(); /* Look for tasks remaining */
#endif //MPCOMP_TASK
	 sctk_thread_yield() ;
    }
    sctk_atomics_store_int(
			&(n->children.leaf[0]->threads[0].info.new_root->barrier), 0) ;

#if MPCOMP_TASK
    __mpcomp_task_schedule();
#endif //MPCOMP_TASK

    /* Update team info for last values */
	__mpcomp_save_team_info( instance->team, &(n->children.leaf[0]->threads[0]) ) ;

    /* Restore the previous OpenMP info */
    sctk_openmp_thread_tls = t ;

#if 0
    /* Sequential check of tree coherency */
    sctk_assert( root->barrier == 0 ) ;
    for ( i = 0 ; i < root->nb_children ; i++ ) {
      sctk_assert( root->children.node[i]->barrier == 0 ) ;
    }
#endif

  } else {

    if ( mpcomp_global_icvs.warn_nested && t->instance->team->depth > 0 ) {
      mpcomp_warn_nested() ;
    }

	  /* Check whether the number of current thread is 1 or not */
#if 0
	  if ( t->info.num_threads == 1 ) {
		  mpcomp_instance_t * instance ;

		  sctk_debug( "__mpcomp_start_parallel_static_loop:"
				 " nested 1 thread within 1 thread" ) ;

		  instance = t->instance ;
		  sctk_assert( instance != NULL ) ;
		  sctk_assert( instance->team != NULL ) ;

		  instance->team->depth++ ;

		  __mpcomp_static_loop_init(t,lb,b,incr,chunk_size);
		  func(shared) ;

		  instance->team->depth-- ;

	  } else 
#endif
	  {
		  mpcomp_instance_t * instance ;

		  sctk_debug( "__mpcomp_start_parallel_static_loop:"
				  " nested 1 thread within %d threads",
				  t->info.num_threads	) ;

		  /* Check if the children instance exists */
		  if ( t->children_instance == NULL ) {
			  mpcomp_team_t * new_team ;

			  sctk_debug( "__mpcomp_start_parallel_static_loop:"
					  " children instance is NULL, allocating...",
					  t->info.num_threads	) ;

			  new_team = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
			  sctk_assert( new_team != NULL ) ;
			  __mpcomp_team_init( new_team ) ;

			  t->children_instance = 
				  (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
			  sctk_assert( t->children_instance != NULL ) ;
			  __mpcomp_instance_init( t->children_instance, 0, new_team ) ;
		  }

		  instance = t->children_instance ;
		  sctk_assert( instance != NULL ) ;
		  sctk_assert( instance->team != NULL ) ;

		  /* Fill information for the team */
		  instance->team->info.func = func ;
		  instance->team->info.shared = shared ;
		  instance->team->info.num_threads = 1 ;
		  instance->team->info.new_root = NULL ;
		  instance->team->info.icvs = t->info.icvs ;
		  instance->team->depth = t->instance->team->depth + 1 ;

		  mpcomp_thread_t * target_t ;

		  sctk_debug( "__mpcomp_start_parallel_static_loop:"
				  " #mVPs for target instance = %d\n", instance->nb_mvps ) ;

		  target_t = &(instance->mvps[0]->threads[0]);
		  sctk_assert( target_t != NULL ) ;

		  /* Fill information for the target thread */
		  target_t->info = instance->team->info ;
		  target_t->rank = 0 ;

		  sctk_openmp_thread_tls = target_t ;

		  __mpcomp_static_loop_init(target_t,lb,b,incr,chunk_size);
		  func(shared) ;

		  sctk_openmp_thread_tls = t;

	  }
  }

  sctk_debug( 
		  "__mpcomp_start_parallel_static_loop:"
		  " === EXIT PARALLEL REGION ===" ) ;


}

void
__mpcomp_start_parallel_guided_loop (int arg_num_threads,
				      void *(*func) (void *), void *shared,
				      long lb, long b, long incr, long chunk_size)
{
     __mpcomp_start_parallel_dynamic_loop(arg_num_threads, func, shared,
					  lb, b, incr, chunk_size);
}

void
__mpcomp_start_parallel_runtime_loop (int arg_num_threads,
		void *(*func) (void *), void *shared,
		long lb, long b, long incr, long chunk_size)
{
	mpcomp_thread_t *t ;	/* Info on the current thread */

	/* Handle orphaned directive (initialize OpenMP environment) */
	__mpcomp_init();

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	switch( t->info.icvs.run_sched_var ) {
		case omp_sched_static:
			__mpcomp_start_parallel_static_loop (arg_num_threads, func, shared,
					lb, b, incr, chunk_size) ;
			break ;
		case omp_sched_dynamic:
			__mpcomp_start_parallel_dynamic_loop (arg_num_threads, func, shared,
					lb, b, incr, chunk_size) ;
			break ;
		case omp_sched_guided:
			__mpcomp_start_parallel_guided_loop (arg_num_threads, func, shared,
					lb, b, incr, chunk_size) ;
			break ;
		default:
			not_reachable();
			break ;
	}
}
