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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_barrier.h"
#include "mpcomp_task_utils.h"

int mpcomp_task_all_task_executed( void );

/** Barrier for all threads of the same team */
static void __mpcomp_internal_full_barrier(mpcomp_mvp_t *mvp)
{
	long b, b_done;
	mpcomp_node_t *c, *new_root ;

    sctk_assert( mvp );
    sctk_assert( mvp->father );
    sctk_assert( mvp->father->instance );
    sctk_assert( mvp->father->instance->team );

	c = mvp->father;
	new_root = c->instance->team->info.new_root; 

#if MPCOMP_TASK
    mpcomp_taskwait();
#endif /* MPCOMP_TASK */

	/* Step 1: Climb in the tree */
	b_done = c->barrier_done; /* Move out of sync region? */
	b = sctk_atomics_fetch_and_incr_int(&(c->barrier)) + 1;

	while( b == c->barrier_num_threads && c != new_root ) 
    {
		sctk_atomics_store_int(&(c->barrier), 0);
		c = c->father;
        b_done = c->barrier_done;
		b = sctk_atomics_fetch_and_incr_int(&(c->barrier)) + 1;
	}


	/* Step 2 - Wait for the barrier to be done */
	if (c != new_root || (c == new_root && b != c->barrier_num_threads)) 
    {	  
		/* Wait for c->barrier == c->barrier_num_threads */
		while (b_done == c->barrier_done) {
			sctk_thread_yield();
#if MPCOMP_TASK
			mpcomp_task_schedule( 0 );
#endif /* MPCOMP_TASK */
		}
	} else {

		sctk_atomics_store_int(&(c->barrier), 0);

#if MPCOMP_COHERENCY_CHECKING
		mpcomp_for_dyn_coherency_end_barrier() ;
		mpcomp_single_coherency_end_barrier() ;
#endif

		c->barrier_done++ ; /* No need to lock I think... */
	}

	/* Step 3 - Go down */
	while ( c->child_type != MPCOMP_CHILDREN_LEAF ) {
		c = c->children.node[mvp->tree_rank[c->depth]];
		c->barrier_done++; /* No need to lock I think... */
	}

#if MPCOMP_TASK
#if MPCOMP_COHERENCY_CHECKING
        //mpcomp_task_coherency_barrier();
#endif
#endif /* MPCOMP_TASK */
}

/* Half barrier for the end of a parallel region */
void __mpcomp_internal_half_barrier ( mpcomp_mvp_t *mvp)
{
  long b;
  mpcomp_node_t *c, *new_root;

  sctk_assert( mvp );
  sctk_assert( mvp->father );
  sctk_assert( mvp->father->instance );
  sctk_assert( mvp->father->instance->team );

  c = mvp->father;
  new_root = c->instance->team->info.new_root ;
  sctk_assert( new_root != NULL ) ;

  sctk_nodebug("%s: entering", __func__ );

#if MPCOMP_TASK
    mpcomp_thread_t* save_tls = sctk_openmp_thread_tls;
    sctk_openmp_thread_tls = &( mvp->threads[0] );
    mpcomp_thread_t* thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;

	sctk_assert( thread->instance );
	sctk_assert( thread->instance->team );

    __mpcomp_internal_full_barrier(mvp);
    sctk_openmp_thread_tls = save_tls;
#endif /* MPCOMP_TASK */

    /* Step 1: Climb in the tree */
    b = sctk_atomics_fetch_and_incr_int(&(c->barrier)) + 1;
    while( b == c->barrier_num_threads && c != new_root ) 
    {
        sctk_nodebug("%s: currently %d thread(s), expected %d", __func__, b - 1, c->barrier_num_threads);
        sctk_atomics_store_int(&(c->barrier), 0);
        c = c->father;
        b = sctk_atomics_fetch_and_incr_int(&(c->barrier)) + 1;
    }

    sctk_nodebug("%s: exiting", __func__);
}

/*
   OpenMP barrier.
   All threads of the same team must meet.
   This barrier uses some optimizations for threads inside the same microVP.
 */
void __mpcomp_barrier(void)
{
    mpcomp_thread_t *t;

	/* Handle orphaned directive (initialize OpenMP environment) */
	__mpcomp_init() ;

    /* Grab info on the current thread */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    sctk_nodebug( "[%d] %s: Entering w/ %d thread(s)", t->rank, __func__, t->info.num_threads ) ;
     
	if (t->info.num_threads == 1) return;
    
    /* Get the corresponding microVP */
    mpcomp_mvp_t* mvp = t->mvp;
    sctk_assert(mvp != NULL);
    sctk_nodebug( "[%d] %s: t->mvp = %p", t->rank, __func__, t->mvp ) ;

    /* Call the real barrier */
    __mpcomp_internal_full_barrier( mvp );
}

/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
    __asm__(".symver __mpcomp_barrier, GOMP_barrier@@GOMP_1.0"); 
#endif /* OPTIMIZED_GOMP_API_SUPPORT */
