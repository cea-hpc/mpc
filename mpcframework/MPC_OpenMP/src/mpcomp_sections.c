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
#include <sctk_debug.h>

#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_barrier.h"

#if OMPT_SUPPORT
#include "ompt.h"
extern ompt_callback_t* OMPT_Callbacks;
#endif /* OMPT_SUPPORT */

/*
   This file contains all functions related to SECTIONS constructs in OpenMP
 */

static int __mpcomp_sections_internal_next(mpcomp_thread_t *t,
                                           mpcomp_team_t *team) {
  int r ;
  int success ;

  t->single_sections_current =
      OPA_load_int(&(team->single_sections_last_current));
  sctk_nodebug("[%d] %s: Current = %d (target = %d)", t->rank, __func__,
               t->single_sections_current, t->single_sections_target_current);
  success = 0 ;

  while (t->single_sections_current < t->single_sections_target_current &&
         !success) {
    r = OPA_cas_int(&(team->single_sections_last_current),
                             t->single_sections_current,
                             t->single_sections_current + 1);

    sctk_nodebug("[%d] %s: CAS %d -> %d (res %d)", t->rank, __func__,
                 t->single_sections_current, t->single_sections_current + 1, r);

    if (r != t->single_sections_current) {
      t->single_sections_current =
          OPA_load_int(&(team->single_sections_last_current));
      if (t->single_sections_current > t->single_sections_target_current) {
        t->single_sections_current = t->single_sections_target_current;
      }
    } else {
      success = 1;
    }
  }

  if ( t->single_sections_current < t->single_sections_target_current ) {
    sctk_nodebug("[%d] %s: Success w/ current = %d", t->rank, __func__,
                 t->single_sections_current);
    return t->single_sections_current - t->single_sections_start_current + 1;
  }

  sctk_nodebug("[%d] %s: Fail w/ final current = %d", t->rank, __func__,
               t->single_sections_current);

  return 0 ;
}


void __mpcomp_sections_init( mpcomp_thread_t * t, int nb_sections ) {
  long num_threads ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

  sctk_nodebug("[%d] %s: Entering w/ %d section(s)", t->rank, __func__,
               nb_sections);

  /* Update current sections construct and update
   * the target according to the number of sections */
  t->single_sections_start_current = t->single_sections_current ;
  t->single_sections_target_current = t->single_sections_current + nb_sections ;
  t->nb_sections = nb_sections;

  sctk_nodebug("[%d] %s: Current %d, start %d, target %d", t->rank, __func__,
               t->single_sections_current, t->single_sections_start_current,
               t->single_sections_target_current);

  return ;
}

/* Return >0 to execute corresponding section.
   Return 0 otherwise to leave sections construct
   */
int __mpcomp_sections_begin(int nb_sections) {
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

  sctk_nodebug("[%d] %s: entering w/ %d section(s)", t->rank, __func__,
               nb_sections);

  __mpcomp_sections_init( t, nb_sections ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

#if OMPT_SUPPORT
	if( _mpc_omp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
                ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
                ompt_data_t* task_data = &( t->task_infos.current_task->ompt_task_data );
				const void* code_ra = __builtin_return_address(0);	
				callback( ompt_work_sections, ompt_scope_begin, parallel_data, task_data, nb_sections, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

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

int __mpcomp_sections_next(void) {
  mpcomp_thread_t *t ;	/* Info on the current thread */
  mpcomp_team_t *team ;	/* Info on the team */
  long num_threads ;

  /* Grab the thread info */
  t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* Number of threads in the current team */
  num_threads = t->info.num_threads;
  sctk_assert( num_threads > 0 ) ;

  sctk_nodebug("[%d] %s: Current %d, start %d, target %d", t->rank, __func__,
               t->single_sections_current, t->single_sections_start_current,
               t->single_sections_target_current);

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


void __mpcomp_sections_end_nowait(void) { 

#if OMPT_SUPPORT
	if( _mpc_omp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
  				mpcomp_thread_t *t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
                ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
                ompt_data_t* task_data = &( t->task_infos.current_task->ompt_task_data );
				const void* code_ra = __builtin_return_address(0);	
				callback( ompt_work_sections, ompt_scope_end, parallel_data, task_data, t->nb_sections, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */
}

void __mpcomp_sections_end(void) 
{ 
	__mpcomp_sections_end_nowait();
	__mpcomp_barrier(); 
}

int __mpcomp_sections_coherency_exiting_paralel_region(void) { return 0; }

int __mpcomp_sections_coherency_barrier(void) { return 0; }
