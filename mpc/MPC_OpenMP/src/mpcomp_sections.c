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

/* Return >0 to execute corresponding section.
   Return 0 otherwise to leave sections construct
   */
int
__mpcomp_sections_begin (int nb_sections)
{
  mpcomp_thread_t *t ;  /* Info on the current thread */
  mpcomp_team_t *team_info ;    /* Info on the team */
  int index ;
  int num_threads ;
  int nb_entered_threads ;

  /* Quit if no sections */
  if ( nb_sections <= 0 ) {
    return 0 ;
  }

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init() ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->num_threads;

  /* Store the total number of sections inside private thread info */
  t->nb_sections = nb_sections ;

  /* If alone in parallel region -> execute first section */
  if ( num_threads == 1 ) {
    t->previous_section = 1 ;
    return 1 ;
  }

  /* Get the team info */
  team_info = t->team ;
  sctk_assert (team_info != NULL);

  /* Grab the current index for sections */
  index = t->sections_current ;

#if MPCOMP_USE_ATOMICS
  /* If too far ==> Wait until the last thread of first in-flight sections has finished */
  if ( sctk_atomics_load_int(
        &(team_info->sections_nb_threads_entered[index].i) )
      == MPCOMP_NOWAIT_STOP_SYMBOL ) {

    while ( sctk_atomics_load_int(
          &(team_info->sections_nb_threads_entered[index].i) )
        == MPCOMP_NOWAIT_STOP_SYMBOL ) {
      sctk_thread_yield() ;
    }
  }
#else
  sctk_spinlock_lock (&(team_info->sections_lock_enter[index]));

  /* If too far ==> Wait until the last thread of first in-flight sections has finished */  
  if ( team_info->sections_nb_threads_entered[index] 
       == MPCOMP_NOWAIT_STOP_SYMBOL ) {       
       while ( team_info->sections_nb_threads_entered[index] 
	       == MPCOMP_NOWAIT_STOP_SYMBOL ) {
	    sctk_spinlock_unlock (&(team_info->sections_lock_enter[index]));
	    sctk_thread_yield() ;
	    sctk_spinlock_lock (&(team_info->sections_lock_enter[index]));
       }
  }
  sctk_spinlock_unlock (&(team_info->sections_lock_enter[index]));
#endif

  /* Increase the value of the current sections index */
#if MPCOMP_USE_ATOMICS
  nb_entered_threads = sctk_atomics_fetch_and_incr_int(
      &(team_info->sections_nb_threads_entered[index].i) ) ;
#else
  sctk_spinlock_lock (&(team_info->sections_lock_enter[index]));
  nb_entered_threads = team_info->sections_nb_threads_entered[index];
  team_info->sections_nb_threads_entered[index] = nb_entered_threads + 1;
  sctk_spinlock_unlock (&(team_info->sections_lock_enter[index]));
#endif

  /* Between 1 to nb_sections => execute the corresponding section */
  if ( nb_entered_threads < nb_sections ) {
    return nb_entered_threads + 1 ;
  }

  /* Else, sections done, increment current sections index */

  if ( nb_entered_threads == nb_sections + num_threads - 1 ) {
    int previous_index ;

#if MPCOMP_USE_ATOMICS
    sctk_atomics_store_int(
        &(team_info->sections_nb_threads_entered[index].i),
        MPCOMP_NOWAIT_STOP_SYMBOL
        ) ;
#else
    sctk_spinlock_lock (&(team_info->sections_lock_enter[index]));
    team_info->sections_nb_threads_entered[index] = MPCOMP_NOWAIT_STOP_SYMBOL;
    sctk_spinlock_unlock (&(team_info->sections_lock_enter[index]));
#endif

    previous_index = (index-1+MPCOMP_MAX_ALIVE_SECTIONS+1)%(MPCOMP_MAX_ALIVE_SECTIONS+1) ;

#if MPCOMP_USE_ATOMICS
    sctk_atomics_store_int(
        &(team_info->sections_nb_threads_entered[previous_index].i),
        0
        ) ;
#else
    sctk_spinlock_lock (&(team_info->sections_lock_enter[previous_index]));
    team_info->sections_nb_threads_entered[previous_index] = 0;
    sctk_spinlock_unlock (&(team_info->sections_lock_enter[previous_index]));
#endif


  }

  t->sections_current = (index + 1)%(MPCOMP_MAX_ALIVE_SECTIONS+1) ;

  return 0 ;

}

int
__mpcomp_sections_next ()
{
  mpcomp_thread_t *t ;  /* Info on the current thread */
  mpcomp_team_t *team_info ;    /* Info on the team */
  int index ;
  int num_threads ;
  int nb_entered_threads ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->num_threads;

  /* If alone in parallel region -> execute next section or finish */
  if ( num_threads == 1 ) {
    t->previous_section++ ;
    if ( t->previous_section <= t->nb_sections ) {
      return t->previous_section ;
    }
    return 0 ;
  }

  /* Get the team info */
  team_info = t->team ;
  sctk_assert (team_info != NULL);

  /* Grab the current index for sections */
  index = t->sections_current ;

  /* Increase the value of the current sections index */
#if MPCOMP_USE_ATOMICS
  nb_entered_threads = sctk_atomics_fetch_and_incr_int(
      &(team_info->sections_nb_threads_entered[index].i) ) ;
#else
    sctk_spinlock_lock (&(team_info->sections_lock_enter[index]));
    nb_entered_threads = team_info->sections_nb_threads_entered[index];
    sctk_spinlock_unlock (&(team_info->sections_lock_enter[index]));
#endif

  /* Between 1 to nb_sections => execute the corresponding section */
  if ( nb_entered_threads < t->nb_sections ) {
    return nb_entered_threads + 1 ;
  }

  /* Else, sections done, increment current sections index */

  if ( nb_entered_threads == t->nb_sections + num_threads - 1 ) {
    int previous_index ;

#if MPCOMP_USE_ATOMICS
    sctk_atomics_store_int(
        &(team_info->sections_nb_threads_entered[index].i),
        MPCOMP_NOWAIT_STOP_SYMBOL
        ) ;
#else
    sctk_spinlock_lock (&(team_info->sections_lock_enter[index]));
    team_info->sections_nb_threads_entered[index] = MPCOMP_NOWAIT_STOP_SYMBOL;
    sctk_spinlock_unlock (&(team_info->sections_lock_enter[index]));
#endif

    previous_index = (index-1+MPCOMP_MAX_ALIVE_SECTIONS+1)%(MPCOMP_MAX_ALIVE_SECTIONS+1) ;

#if MPCOMP_USE_ATOMICS
    sctk_atomics_store_int(
        &(team_info->sections_nb_threads_entered[previous_index].i),
        0
        ) ;
#else
    sctk_spinlock_lock (&(team_info->sections_lock_enter[previous_index]));
    team_info->sections_nb_threads_entered[previous_index] = 0;
    sctk_spinlock_unlock (&(team_info->sections_lock_enter[previous_index]));
#endif

  }

  t->sections_current = (index + 1)%(MPCOMP_MAX_ALIVE_SECTIONS+1) ;

  return 0 ;

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


/* COHERENCY FUNCTIONS */

int __mpcomp_sections_coherency_entering_paralel_region() {
  int i ;
  mpcomp_thread_t *t ;  /* Info on the current thread */
  mpcomp_team_t *team_info ;    /* Info on the team */
  int error ;
  int nb_stop ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Get the team info */
  team_info = t->team ;
  sctk_assert (team_info != NULL);

  error = 0 ;
  nb_stop = 0 ;
  for ( i = 0 ; i < MPCOMP_MAX_ALIVE_SINGLE + 1 ; i++ ) {
#if MPCOMP_USE_ATOMICS
    switch ( sctk_atomics_load_int(
          &(team_info->sections_nb_threads_entered[i].i) ) ) {
      case 0:
        break ;
      case MPCOMP_NOWAIT_STOP_SYMBOL:
        nb_stop++ ;
        break ;
      default:
        error = 1 ;
    }
#else
    sctk_spinlock_lock (&(team_info->sections_lock_enter[i]));    
    switch (team_info->sections_nb_threads_entered[i]) {
      case 0:
        break ;
      case MPCOMP_NOWAIT_STOP_SYMBOL:
        nb_stop++ ;
        break ;
      default:
        error = 1 ;
    }
    sctk_spinlock_unlock (&(team_info->sections_lock_enter[i]));    
#endif

  }

  if ( nb_stop != 1 ) {
    error = 1 ;
  }

  return error ;
}

int __mpcomp_sections_coherency_exiting_paralel_region() {
  return 0 ;
}

int __mpcomp_sections_coherency_barrier() {
  return 0 ;
}
