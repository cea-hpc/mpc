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
#include <mpcomp.h>
#include "mpcomp_internal.h"
#include <sctk.h>
#include <sctk_debug.h>

/*
   This file contains all functions related to SECTIONS constructs in OpenMP
 */

static int __mpcomp_sections_internal_next( mpcomp_thread_t * t,
		mpcomp_team_t * team ) 
{
  int r ;
  int success ;

  t->single_sections_current = sctk_atomics_load_int( 
		  &(team->single_sections_last_current) ) ;

  sctk_debug( "[%d] __mpcomp_sections_internal_next: Current = %d (target = %d)",
		  t->rank, t->single_sections_current, t->single_sections_target_current ) ;

  success = 0 ;

  while ( t->single_sections_current < t->single_sections_target_current &&
		  !success ) {
	  r = sctk_atomics_cas_int( &(team->single_sections_last_current),
			  t->single_sections_current, t->single_sections_current + 1 ) ;

	  sctk_debug( "[%d] __mpcomp_sections_internal_next: CAS %d -> %d (res %d)",
			  t->rank, t->single_sections_current,
			 t->single_sections_current + 1, r ) ;

	  if ( r != t->single_sections_current ) {
		  t->single_sections_current = sctk_atomics_load_int( 
				  &(team->single_sections_last_current) ) ;
		  if ( t->single_sections_current > t->single_sections_target_current ) {
			  t->single_sections_current = t->single_sections_target_current ;
		  }
	  } else {
		  success = 1 ;
	  }
  }

  if ( t->single_sections_current < t->single_sections_target_current ) {
	  sctk_debug( "[%d] __mpcomp_sections_internal_next: Success w/ current = %d",
			  t->rank, t->single_sections_current ) ;
	  return t->single_sections_current - t->single_sections_start_current + 1 ;
  }

  sctk_debug( "[%d] __mpcomp_sections_internal_next: Fail w/ final current = %d",
		  t->rank, t->single_sections_current ) ;

  return 0 ;
}


void __mpcomp_sections_init( mpcomp_thread_t * t, int nb_sections ) {
  mpcomp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

  sctk_debug( "[%d] __mpomp_sections_init: Entering w/ %d section(s)",
		  t->rank, nb_sections ) ;

  /* Update current sections construct and update
   * the target according to the number of sections */
  t->single_sections_start_current = t->single_sections_current ;
  t->single_sections_target_current = t->single_sections_current + nb_sections ;

  sctk_debug( "[%d] __mpomp_sections_init: Current %d, start %d, target %d",
		  t->rank, t->single_sections_current, 
		  t->single_sections_start_current, t->single_sections_target_current ) ;

  return ;
}

/* Return >0 to execute corresponding section.
   Return 0 otherwise to leave sections construct
   */
int
__mpcomp_sections_begin (int nb_sections)
{
  mpcomp_thread_t *t ;	/* Info on the current thread */
  mpcomp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Leave if the number of sections is wrong */
  if ( nb_sections <= 0 ) {
	  return 0 ;
  }

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  __mpcomp_sections_init( t, nb_sections ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute all sections 
   */
  if (num_threads == 1) {
	  /* Start with the first section */
	  t->single_sections_current++ ;
	  return 1 ;
  }

  /* Grab the team info */
  sctk_assert( t->instance != NULL ) ;
  team = t->instance->team ;
  sctk_assert( team != NULL ) ;

  return __mpcomp_sections_internal_next( t, team ) ;
}

int
__mpcomp_sections_next ()
{
  mpcomp_thread_t *t ;	/* Info on the current thread */
  mpcomp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;


  sctk_debug( "[%d] __mpcomp_sections_next: Current %d, start %d, target %d",
		  t->rank, t->single_sections_current, 
		  t->single_sections_start_current, t->single_sections_target_current ) ;

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute all sections 
   */
  if (num_threads == 1) {
	  /* Proceed to the next section if available */
	  if ( t->single_sections_current >= t->single_sections_target_current ) {
		  /* Update current section construct */
		  t->single_sections_current++ ;
		  return 0 ;
	  }
	  /* Update current section construct */
	  t->single_sections_current++ ;
	  return t->single_sections_current - t->single_sections_start_current ;
  }

  /* Grab the team info */
  sctk_assert( t->instance != NULL ) ;
  team = t->instance->team ;
  sctk_assert( team != NULL ) ;

  return __mpcomp_sections_internal_next( t, team ) ;
}

void
__mpcomp_sections_end ()
{
  __mpcomp_barrier() ;
}

void
__mpcomp_sections_end_nowait ()
{
  /* Nothing to do */
}


int __mpcomp_sections_coherency_exiting_paralel_region() {
  return 0 ;
}

int __mpcomp_sections_coherency_barrier() {
  return 0 ;
}
