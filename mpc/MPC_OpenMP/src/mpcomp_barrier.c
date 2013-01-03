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
  mpcomp_thread_t * t ;
  mpcomp_mvp_t * mvp ;

  /* Grab info on the current thread */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_nodebug( "__mpcomp_barrier: thread %p", t ) ;

  if ( t->num_threads == 1 ) {
    return ;
  }

  /* Get the corresponding microVP */
  mvp = t->mvp ;
  sctk_assert( mvp != NULL ) ;

  sctk_nodebug( "__mpcomp_barrier: mvp %p", mvp ) ;


  /* Call the real barrier */
  __mpcomp_internal_full_barrier( mvp ) ;
}


/* Half barrier for the end of a parallel region */
void
__mpcomp_internal_half_barrier ( mpcomp_mvp_t * mvp )
{

  mpcomp_node_t * c ;
  long b ;
  long b_done ;

  sctk_nodebug( "__mpcomp_internal_half_barrier: entering" ) ;

  /* Step 0: TODO finish the barrier whithin the current micro VP */

  /* Step 1: Climb in the tree */
  c = mvp->father ;

  b_done = c->barrier_done ; /* Move out of sync region? */

#if MPCOMP_USE_ATOMICS
  b = sctk_atomics_fetch_and_incr_int( &(c->barrier) ) ;
#else
  sctk_spinlock_lock( &(c->lock) ) ;
  b = c->barrier ;
  c->barrier = b + 1;
  sctk_spinlock_unlock( &(c->lock) ) ;
#endif

  // while ( b == c->barrier_num_threads && c->father != NULL ) 
  while ( (b+1) == c->barrier_num_threads && c->father != NULL ) 
  {
    sctk_nodebug( 
	"__mpcomp_internal_half_barrier: currently %d thread(s), expected %d ", 
	b, c->barrier_num_threads ) ;

#if MPCOMP_USE_ATOMICS
    sctk_atomics_store_int( &(c->barrier), 0 ) ;
#else
    c->barrier = 0 ;
#endif
    c = c->father ;

#if MPCOMP_USE_ATOMICS
    b = sctk_atomics_fetch_and_incr_int( &(c->barrier) ) ;
#else
    sctk_spinlock_lock( &(c->lock) ) ;
    b = c->barrier ;
    c->barrier = b + 1 ;
    sctk_spinlock_unlock( &(c->lock) ) ;
#endif

  }

  sctk_nodebug( "__mpcomp_internal_half_barrier: exiting" ) ;
}

/* Barrier for all threads of the same team */
void
__mpcomp_internal_full_barrier ( mpcomp_mvp_t * mvp )
{

  mpcomp_node_t * c ;
  long b ;
  long b_done ;

  /* TODO: check if we need sctk_atomics_write_barrier() */

  /* Step 0: TODO finish the barrier whithin the current micro VP */

  /* Step 1: Climb in the tree */
  c = mvp->father ;

  b_done = c->barrier_done ; /* Move out of sync region? */

#if MPCOMP_USE_ATOMICS
  b = sctk_atomics_fetch_and_incr_int( &(c->barrier) ) ;
#else
  sctk_spinlock_lock( &(c->lock) ) ;
  b = c->barrier ;
  c->barrier = b + 1 ;
  sctk_spinlock_unlock( &(c->lock) ) ;
#endif

  // sctk_assert( b+1 <= c->barrier_num_threads ) ;

  while ( (b+1) == c->barrier_num_threads && c->father != NULL ) {

#if MPCOMP_USE_ATOMICS
    sctk_atomics_store_int( &(c->barrier), 0 ) ;
#else
    c->barrier = 0 ;
#endif

    c = c->father ;

#if MPCOMP_USE_ATOMICS
    b = sctk_atomics_fetch_and_incr_int( &(c->barrier) ) ;
#else
    sctk_spinlock_lock( &(c->lock) ) ;
    b = c->barrier ;
    c->barrier = b + 1 ;
    sctk_spinlock_unlock( &(c->lock) ) ;
#endif

    // sctk_assert( b+1 <= c->barrier_num_threads ) ;
  }


  /* Step 2 - Wait for the barrier to be done */
  if ( c->father != NULL || 
      ( c->father== NULL && (b+1) != c->barrier_num_threads ) ) {

    /* Wait for c->barrier == c->barrier_num_threads */
    while (b_done == c->barrier_done ) {
      sctk_thread_yield() ;
    }

  } else {
#if MPCOMP_USE_ATOMICS
    sctk_atomics_store_int( &(c->barrier), 0 ) ;
#else
    c->barrier=0;
#endif

    c->barrier_done++ ; /* No need to lock I think... */
  }

  sctk_debug("__mpcomp_internal_full_barrier: node nb children=%d, node depth=%d, mvp address=%p, mvp rank=%d, mvp tree_rank=%d", c->nb_children, c->depth, &mvp, mvp->rank, mvp->tree_rank[c->depth]);

  /* Step 3 - Go down */
  while ( c->child_type != MPCOMP_CHILDREN_LEAF )  {
    c = c->children.node[ mvp->tree_rank[ c->depth ] ] ;
    c->barrier_done++ ; /* No need to lock I think... */
  }

  // sctk_assert( b_done + 1 == mvp->father->barrier_done ) ;
  // sctk_assert( b_done + 1 == mvp->father->father->barrier_done ) ;
}
