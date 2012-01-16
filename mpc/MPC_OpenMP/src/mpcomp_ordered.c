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

#include "sctk.h"
#include "sctk_debug.h"
#include "sctk_topology.h"
#include "sctk_atomics.h"
#include <mpcomp.h>
#include <mpcomp_internal.h>

void __mpcomp_ordered_begin() {

  mpcomp_thread_t *t;
  mpcomp_thread_team_t *team;

  __mpcomp_init ();

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  team = t->team ;

  /* First iteration of the loop -> initialize 'next_ordered_offset' */
  if ( t->current_ordered_iteration == t->loop_lb ) {
    team->next_ordered_offset = 0 ;
  } else {
    /* Do we have to wait for the right iteration? */
    if ( t->current_ordered_iteration != 
	(t->loop_lb + t->loop_incr * team->next_ordered_offset) ) {

      sctk_nodebug( "__mpcomp_ordered_begin[%d]: Waiting to schedule iteration %d",
	  t->rank, t->current_ordered_iteration ) ;

      /* Spin while the condition is not satisfied */
      while ( t->current_ordered_iteration != 
	  (t->loop_lb + t->loop_incr * team->next_ordered_offset) ) {
	if ( t->current_ordered_iteration != 
	    (t->loop_lb + t->loop_incr * team->next_ordered_offset) ) {
	  sctk_thread_yield ();
	}
      }
    }
  }

  sctk_nodebug( "__mpcomp_ordered_begin[%d]: Allowed to schedule iteration %d",
      t->rank, t->current_ordered_iteration ) ;
}

void __mpcomp_ordered_end() {

  mpcomp_thread_t *t;
  mpcomp_thread_team_t *team;

   /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  team = t->team ;

  t->current_ordered_iteration += t->loop_incr ;
  if ( (t->loop_incr > 0 && t->current_ordered_iteration >= t->loop_b) ||
      (t->loop_incr < 0 && t->current_ordered_iteration <= t->loop_b) ) {
    team->next_ordered_offset = -1 ;
  } else {
    team->next_ordered_offset++ ;
  }

}

