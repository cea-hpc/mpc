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
     
     /* Grab info on the current thread */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert(t != NULL);
     
     if (t->num_threads == 1) {
#if MPCOMP_TASK
	  __mpcomp_task_schedule();   /* Look for tasks remaining */
#endif /* MPCOMP_TASK */
	  return;
     }

     /* Get the corresponding microVP */
     mvp = t->mvp;
     sctk_assert(mvp != NULL);
     
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

  sctk_nodebug("__mpcomp_internal_half_barrier: entering");

  /* Step 0: TODO finish the barrier whithin the current micro VP */

  /* Step 1: Climb in the tree */
  c = mvp->father;
  b_done = c->barrier_done; /* Move out of sync region? */
  b = sctk_atomics_fetch_and_incr_int(&(c->barrier));

  while ((b+1) == c->barrier_num_threads && c->father != NULL) {
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
     
     /* TODO: check if we need sctk_atomics_write_barrier() */
     
     /* Step 0: TODO finish the barrier whithin the current micro VP */
     
     /* Step 1: Climb in the tree */
     c = mvp->father;
     b_done = c->barrier_done; /* Move out of sync region? */
     b = sctk_atomics_fetch_and_incr_int(&(c->barrier));
  
     while ((b+1) == c->barrier_num_threads && c->father != NULL) {
	  sctk_atomics_store_int(&(c->barrier), 0);
	  c = c->father;
	  b = sctk_atomics_fetch_and_incr_int(&(c->barrier));
     }
     
     /* Step 2 - Wait for the barrier to be done */
     if (c->father != NULL || 
	 (c->father == NULL && (b+1) != c->barrier_num_threads)) {	  
	  /* Wait for c->barrier == c->barrier_num_threads */
	  while (b_done == c->barrier_done) {
#if MPCOMP_TASK
	       __mpcomp_task_schedule(); /* Look for tasks remaining */
#endif /* MPCOMP_TASK */
	       sctk_thread_yield();
	  }
     } else {
	  sctk_atomics_store_int(&(c->barrier), 0);

	  c->barrier_done++ ; /* No need to lock I think... */
#if MPCOMP_TASK
	  __mpcomp_task_schedule(); /* Look for tasks remaining */
#endif /* MPCOMP_TASK */
     }

     /* Step 3 - Go down */
     while ( c->child_type != MPCOMP_CHILDREN_LEAF ) {
	  c = c->children.node[mvp->tree_rank[c->depth]];
	  c->barrier_done++; /* No need to lock I think... */
     }
}
