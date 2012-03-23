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

/* TODO values for different single construct are located on the same cache line
   Need to create padding for single_nb_threads_entered array
 */

/* 
   Perform a single construct.
   This function handles the 'nowait' clause.
   Return '1' if the next single construct has to be executed, '0' otherwise 
 */

#if MPCOMP_USE_ATOMICS

#if 1
#warning "SINGLE: Old version with 2 increments"
/* Old version with initial increment */
int
__mpcomp_do_single (void)
{
  mpcomp_thread *t ;	/* Info on the current thread */
  mpcomp_team_info *team_info ;	/* Info on the team */
  int index;
  int num_threads;
  int nb_entered_threads;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */
  if (num_threads == 1) {
    return 1;
  }

  /* Get the team info */
  team_info = t->team ;
  sctk_assert (team_info != NULL);

  /* Compute the index of the current single construct */
  index = (t->single_current + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);

  nb_entered_threads = sctk_atomics_fetch_and_incr_int(
      &(team_info->single_nb_threads_entered[index].i) ) ;

  if ( nb_entered_threads + 1 >= MPCOMP_MAX_THREADS) {

    while ( sctk_atomics_load_int( 
	  &(team_info->single_nb_threads_entered[index].i) ) >= MPCOMP_MAX_THREADS ) {
      sctk_thread_yield() ;
    }

    nb_entered_threads = sctk_atomics_fetch_and_incr_int(
	&(team_info->single_nb_threads_entered[index].i) ) ;
  }

  /*
  sctk_assert( nb_entered_threads >= 0 ) ;
  sctk_assert( nb_entered_threads < num_threads ) ;
  */

  t->single_current = index;

  if ( nb_entered_threads == 0 ) {
    return 1;
  }

  if ( nb_entered_threads + 1 == num_threads ) {
    int previous_index;

    sctk_atomics_store_int( 
	&(team_info->single_nb_threads_entered[index].i), MPCOMP_MAX_THREADS ) ;

    // sctk_atomics_write_barrier() ;

    previous_index = (index - 1 + MPCOMP_MAX_ALIVE_SINGLE + 1) %
      (MPCOMP_MAX_ALIVE_SINGLE + 1);

    /*
    sctk_assert( sctk_atomics_load_int(
	  &(team_info->single_nb_threads_entered[previous_index].i) ) >= MPCOMP_MAX_THREADS ) ;
	  */

    sctk_atomics_store_int( 
	&(team_info->single_nb_threads_entered[previous_index].i), 0 ) ;
  }
  
  return 0 ;

}
#else
#warning "SINGLE: new version with 1 increment"

/* New version with only one increment operation (March 1st 2012) */
int
__mpcomp_do_single (void)
{
  mpcomp_thread *t ;	/* Info on the current thread */
  mpcomp_team_info *team_info ;	/* Info on the team */
  int index;
  int num_threads;
  int nb_entered_threads;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */
  if (num_threads == 1) {
    return 1;
  }

  /* Get the team info */
  team_info = t->team ;
  sctk_assert (team_info != NULL);

  /* Compute the index of the current single construct */
  index = (t->single_current + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);

  nb_entered_threads = sctk_atomics_load_int( 
      &(team_info->single_nb_threads_entered[index].i) ) ;

  if ( nb_entered_threads == MPCOMP_MAX_THREADS) {

    while ( sctk_atomics_load_int( 
	  &(team_info->single_nb_threads_entered[index].i) ) == MPCOMP_MAX_THREADS ) {
      sctk_thread_yield() ;
    }

  }

  nb_entered_threads = sctk_atomics_fetch_and_incr_int(
      &(team_info->single_nb_threads_entered[index].i) ) ;

  /*
  sctk_assert( nb_entered_threads >= 0 ) ;
  sctk_assert( nb_entered_threads < num_threads ) ;
  */

  t->single_current = index;

  if ( nb_entered_threads == 0 ) {
    return 1;
  }

  if ( nb_entered_threads + 1 == num_threads ) {
    int previous_index;

    sctk_atomics_store_int( 
	&(team_info->single_nb_threads_entered[index].i), MPCOMP_MAX_THREADS ) ;

    // sctk_atomics_write_barrier() ;

    previous_index = (index - 1 + MPCOMP_MAX_ALIVE_SINGLE + 1) %
      (MPCOMP_MAX_ALIVE_SINGLE + 1);

    /*
    sctk_assert( sctk_atomics_load_int(
	  &(team_info->single_nb_threads_entered[previous_index].i) ) == MPCOMP_MAX_THREADS ) ;
	  */

    sctk_atomics_store_int( 
	&(team_info->single_nb_threads_entered[previous_index].i), 0 ) ;
  }
  
  return 0 ;

}
#endif

#else
int
__mpcomp_do_single (void)
{
  mpcomp_thread *t ;	/* Info on the current thread */
  mpcomp_team_info *team_info ;	/* Info on the team */
  int index;
  int num_threads;
  int nb_entered_threads;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */
  if (num_threads == 1) {
    return 1;
  }

  /* Get the team info */
  team_info = t->team ;
  sctk_assert (team_info != NULL);
   */
  /* Get the rank of the current thread */

     rank, index, self->vp, MPCOMP_MAX_ALIVE_SINGLE);


  /* Compute the index of the current single construct */
  index = (t->single_current + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);

  /*
  sctk_nodebug( "__mpcomp_do_single[%d]: entering single %d ", 
      t->rank, index ) ;
   */

  sctk_spinlock_lock (&(team_info->single_lock_enter[index]));

  nb_entered_threads = team_info->single_nb_threads_entered[index];

  /* MPCOMP_NOWAIT_STOP_SYMBOL in the next workshare structure means that the
   * buffer is full (too many alive single constructs).
   * Therefore, the threads of this team have to wait. */
  if (nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL) {

    sctk_spinlock_unlock (&(team_info->single_lock_enter[index]));

  /*
    sctk_nodebug( "__mpcomp_do_single[%d]: waiting for index %d to change",
	t->rank, index ) ;
   */

    t->single_current = index;

    /* Wait until the value changes */
    while ( team_info->single_nb_threads_entered[index] == MPCOMP_NOWAIT_STOP_SYMBOL ) {
      sctk_thread_yield() ;
    }

    sctk_spinlock_lock (&(team_info->single_lock_enter[index]));

    nb_entered_threads = team_info->single_nb_threads_entered[index];
  }

  /*
  sctk_assert( nb_entered_threads >= 0 ) ;
  sctk_assert( nb_entered_threads < num_threads ) ;
  */

  /* Am I the first one? */
  if (nb_entered_threads == 0) {
    /* Yes => execute the single block */

    /*
    sctk_assert( team_info->single_nb_threads_entered[index] != MPCOMP_NOWAIT_STOP_SYMBOL ) ;
    sctk_assert( team_info->single_nb_threads_entered[index] == nb_entered_threads ) ;
    */

    team_info->single_nb_threads_entered[index] = 1;
    
    /*
    sctk_assert( team_info->single_nb_threads_entered[index] == 1 ) ;
    */

    sctk_spinlock_unlock (&(team_info->single_lock_enter[index]));

    t->single_current = index;

    return 1;
  }

  /* Otherwise, just check if I'm not the last one and do not execute the
     single block */

  /*
  sctk_assert( team_info->single_nb_threads_entered[index] != MPCOMP_NOWAIT_STOP_SYMBOL ) ;
  */

  if ( nb_entered_threads + 1 == num_threads ) {
    team_info->single_nb_threads_entered[index] = MPCOMP_NOWAIT_STOP_SYMBOL ;
  } else {
    /*
    sctk_assert( team_info->single_nb_threads_entered[index] == nb_entered_threads ) ;
    */
    team_info->single_nb_threads_entered[index] = nb_entered_threads + 1;
    /*
    sctk_assert( team_info->single_nb_threads_entered[index] == nb_entered_threads + 1 ) ;
    */
  }

  sctk_spinlock_unlock (&(team_info->single_lock_enter[index]));

  t->single_current = index;


  /* If I'm the last thread to exit */
  if (nb_entered_threads + 1 == num_threads) {
    int previous_index;
    int previous_nb_entered_threads;

    /* Update the info on the previous loop */
    previous_index = (index - 1 + MPCOMP_MAX_ALIVE_SINGLE + 1) %
      (MPCOMP_MAX_ALIVE_SINGLE + 1);

    /*
    sctk_nodebug( "__mpcomp_do_single[%d]: last one in %d, checking %d", 
	t->rank, index, previous_index) ;
     */

    sctk_spinlock_lock (&(team_info->single_lock_enter[previous_index]));

    /*
    sctk_nodebug( "__mpcomp_do_single[%d]: previous index %d -> value %d", 
	t->rank, previous_index, team_info->single_nb_threads_entered[previous_index] ) ;

    sctk_assert( team_info->single_nb_threads_entered[previous_index] 
	== MPCOMP_NOWAIT_STOP_SYMBOL ) ;

    sctk_assert( team_info->single_nb_threads_entered[index] 
	== MPCOMP_NOWAIT_STOP_SYMBOL ) ;
     */

    team_info->single_nb_threads_entered[previous_index] = 0 ;

    sctk_spinlock_unlock (&(team_info->single_lock_enter[previous_index]));
  }

  return 0;
}
#endif

void *
__mpcomp_do_single_copyprivate_begin (void)
{
  /* TODO */
  not_implemented() ;
}

void
__mpcomp_do_single_copyprivate_end (void *data)
{
  /* TODO */
  not_implemented() ;
}
