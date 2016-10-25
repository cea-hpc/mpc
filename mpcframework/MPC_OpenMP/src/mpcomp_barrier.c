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

#include "mpcomp.h"
#include "mpcomp_internal.h"


/*
   OpenMP barrier.
   All threads of the same team must meet.
   This barrier uses some optimizations for threads inside the same microVP.
 */
void
__mpcomp_barrier (void)
{
     mpcomp_thread_t *t;
     mpcomp_mvp_t *mvp;

	 /* Handle orphaned directive (initialize OpenMP environment) */
	 __mpcomp_init() ;

     /* Grab info on the current thread */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     sctk_nodebug( "[%d] __mpcomp_barrier: Entering w/ %d thread(s)", 
	 t->rank, t->info.num_threads ) ;
     
	if (t->info.num_threads == 1) 
	{
		return;
   }
    
     /* Get the corresponding microVP */
     mvp = t->mvp;
     sctk_assert(mvp != NULL);
	 

    sctk_nodebug( "[%d] __mpcomp_barrier: t->mvp = %p", t->rank, t->mvp ) ;

   /* Call the real barrier */
   __mpcomp_internal_full_barrier(mvp);
}


/* Half barrier for the end of a parallel region */
void
__mpcomp_internal_half_barrier (mpcomp_mvp_t *mvp)
{

  mpcomp_node_t *c;
  long b;
  long b_done;
  mpcomp_node_t * new_root ;

  c = mvp->father;
  sctk_assert( c != NULL ) ;

  new_root = c->instance->team->info.new_root ;
  sctk_assert( new_root != NULL ) ;

  sctk_nodebug("__mpcomp_internal_half_barrier: entering");

#if MPCOMP_TASK
  //sctk_debug("__mpcomp_internal_half_barrier: full barrier for tasks");
  ///* Wait for all tasks to be done */
  //__mpcomp_internal_full_barrier(mvp);
#endif /* MPCOMP_TASK */

  /* Step 0: TODO finish the barrier whithin the current micro VP */

  /* Step 1: Climb in the tree */
  b_done = c->barrier_done; /* Move out of sync region? */
  b = sctk_atomics_fetch_and_incr_int(&(c->barrier));

  sctk_nodebug("__mpcomp_internal_half_barrier: incrementing first node %d -> %d out of %d",
b, b+1, c->barrier_num_threads );

    while ((b+1) == c->barrier_num_threads && c != new_root ) 
    {
       sctk_nodebug("__mpcomp_internal_half_barrier: currently %d thread(s), expected %d ", 
		    b, c->barrier_num_threads);
       
        sctk_atomics_store_int(&(c->barrier), 0);
        c = c->father;
        b = sctk_atomics_fetch_and_incr_int(&(c->barrier));
    }

  sctk_nodebug("__mpcomp_internal_half_barrier: exiting");
}

/* Barrier for all threads of the same team */
void
__mpcomp_internal_full_barrier (mpcomp_mvp_t *mvp)
{
	mpcomp_node_t *c;
	long b;
	long b_done;
	mpcomp_node_t * new_root ;

	c = mvp->father;
	sctk_assert( c != NULL ) ;

	new_root = c->instance->team->info.new_root ;
	sctk_assert( new_root != NULL ) ;

	/* TODO: check if we need sctk_atomics_write_barrier() */

	/* Step 0: TODO finish the barrier whithin the current micro VP */

	/* Step 1: Climb in the tree */
	b_done = c->barrier_done; /* Move out of sync region? */
	b = sctk_atomics_fetch_and_incr_int(&(c->barrier));

	while ((b+1) == c->barrier_num_threads && c != new_root ) {
		sctk_atomics_store_int(&(c->barrier), 0);
		c = c->father;
		b = sctk_atomics_fetch_and_incr_int(&(c->barrier));
	}

	/* Step 2 - Wait for the barrier to be done */
	if (c != new_root || (c == new_root && (b+1) != c->barrier_num_threads)) {	  
		/* Wait for c->barrier == c->barrier_num_threads */
		while (b_done == c->barrier_done) {
			sctk_thread_yield();
		}
	} else {

		sctk_atomics_store_int(&(c->barrier), 0);

#if MPCOMP_COHERENCY_CHECKING
		__mpcomp_for_dyn_coherency_end_barrier() ;
		__mpcomp_single_coherency_end_barrier() ;
#endif

		c->barrier_done++ ; /* No need to lock I think... */
	}

	/* Step 3 - Go down */
	while ( c->child_type != MPCOMP_CHILDREN_LEAF ) {
		c = c->children.node[mvp->tree_rank[c->depth]];
		c->barrier_done++; /* No need to lock I think... */
	}

#if MPCOMP_COHERENCY_CHECKING
#if MPCOMP_TASK
        __mpcomp_task_coherency_barrier();
#endif /* MPCOMP_TASK */
#endif
}
