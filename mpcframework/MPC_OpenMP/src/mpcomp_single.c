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
#include <sctk_debug.h>


/* 
   Perform a single construct.
   This function handles the 'nowait' clause.
   Return '1' if the next single construct has to be executed, '0' otherwise 
 */

int
__mpcomp_do_single (void)
{
  mpcomp_thread_t *t ;	/* Info on the current thread */
  mpcomp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */
  if (num_threads == 1) {
    return 1;
  }

  /* Grab the team info */
  sctk_assert( t->instance != NULL ) ;
  team = t->instance->team ;
  sctk_assert( team != NULL ) ;

  int current ;

  /* Catch the current construct
   * and prepare the next single/section construct */
  current = t->single_sections_current ;
  t->single_sections_current++ ;

  sctk_debug( "[%d]__mpcomp_do_single: Entering with current %d...", 
		  t->rank, current ) ;
  sctk_debug( "[%d]__mpcomp_do_single:   team current is %d",
		  t->rank, sctk_atomics_load_int( &(team->single_sections_last_current) ) ) ;

  if ( current == sctk_atomics_load_int( &(team->single_sections_last_current) ) ) {
	  int res ;
	  res = sctk_atomics_cas_int( &(team->single_sections_last_current),
			  current, current+1 ) ;

	  sctk_debug( "[%d]__mpcomp_do_single: incr team %d -> %d ==> %d",
			  t->rank, current, current+1, res ) ;

	  /* Success => execute the single block */
	  if ( res == current ) {
		  return 1 ;
	  }
  }

  /* Do not execute the single block */
  return 0 ;
}

void *
__mpcomp_do_single_copyprivate_begin (void)
{
	mpcomp_thread_t *t ;	/* Info on the current thread */
	mpcomp_team_t *team ;	/* Info on the team */

	if ( __mpcomp_do_single() ) {
		return NULL ;
	}
	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	/* In this flow path, the number of threads should not be 1 */
	sctk_assert( t->info.num_threads > 0 ) ;

	/* Grab the team info */
	sctk_assert( t->instance != NULL ) ;
	team = t->instance->team ;
	sctk_assert( team != NULL ) ;

	__mpcomp_barrier() ;

	return team->single_copyprivate_data ;

}

void
__mpcomp_do_single_copyprivate_end (void *data)
{
	mpcomp_thread_t *t ;	/* Info on the current thread */
	mpcomp_team_t *team ;	/* Info on the team */


	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	/* Grab the team info */
	sctk_assert( t->instance != NULL ) ;
	team = t->instance->team ;
	sctk_assert( team != NULL ) ;

	team->single_copyprivate_data = data ;

	__mpcomp_barrier() ;
}


void __mpcomp_single_coherency_entering_parallel_region( ) {
}

void __mpcomp_single_coherency_end_barrier( ) {
	int i ;
  mpcomp_thread_t *t ;	/* Info on the current thread */
  long num_threads ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

	for ( i = 0 ; i < num_threads ; i++ ) {
  mpcomp_thread_t *target_t ;	

	target_t = &(t->instance->mvps[i]->threads[0]) ;
	sctk_debug( 
				"__mpcomp_single_coherency_entering_parallel_region: "
				"thread %d single_sections_current:%d single_sections_last_current:%d",
				target_t->rank,
				target_t->single_sections_current,
				sctk_atomics_load_int( &(t->instance->team->single_sections_last_current) ) ) ;

	
	}
}

