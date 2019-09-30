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

#include "sctk_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_barrier.h"
#include "mpcomp_openmp_tls.h"

#include "ompt.h"
#include "mpcomp_ompt_general.h"

#if OMPT_SUPPORT
extern ompt_callback_t* OMPT_Callbacks; 
#endif /* OMPT_SUPPORT */

/* 
   Perform a single construct.
   This function handles the 'nowait' clause.
   Return '1' if the next single construct has to be executed, '0' otherwise 
 */

int __mpcomp_do_single(void) 
{
	int retval = 0;
  	mpcomp_team_t *team ;	/* Info on the team */

  	/* Handle orphaned directive (initialize OpenMP environment) */
  	__mpcomp_init() ;

  	/* Grab the thread info */
 	mpcomp_thread_t *t = mpcomp_get_thread_tls();

  	/* Number of threads in the current team */
  	const long num_threads = t->info.num_threads;
  	sctk_assert( num_threads > 0 ) ;

  	/* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */
  	if( num_threads == 1	) 
	{
		retval = 1;
	}
	else
	{
  		/* Grab the team info */
  		sctk_assert( t->instance != NULL ) ;
  		team = t->instance->team ;
  		sctk_assert( team != NULL ) ;

  		int current ;

  		/* Catch the current construct
   	    * and prepare the next single/section construct */
  		current = t->single_sections_current ;
  		t->single_sections_current++ ;

  		sctk_nodebug("[%d]%s : Entering with current %d...", __func__, t->rank,
               current);
  		sctk_nodebug("[%d]%s : team current is %d", __func__, t->rank,
               OPA_load_int(&(team->single_sections_last_current)));

  		if (current == OPA_load_int(&(team->single_sections_last_current))) 
		{
    		const int res = OPA_cas_int(&(team->single_sections_last_current),
                                         current, current + 1);
    		sctk_nodebug("[%d]%s: incr team %d -> %d ==> %d", __func__, t->rank,
                 current, current + 1, res);
    		/* Success => execute the single block */
    		if (res == current) 
			{
				retval = 1;
			}
  		}
	}

#if OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
                t->info.in_single = 1;
                if( retval ) team->info.doing_single = t->rank;
                ompt_data_t* parallel_data = &(team->info.ompt_region_data);
                ompt_data_t* task_data = &( t->task_infos.current_task->ompt_task_data );
				ompt_work_t wstype = (retval) ? ompt_work_single_executor : ompt_work_single_other;
				const void* code_ra = __builtin_return_address(0);	
				callback( wstype, ompt_scope_begin, parallel_data, task_data, 1, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */
	/* 0 means not execute */
 	return retval ;
}

void *__mpcomp_do_single_copyprivate_begin(void) {
  mpcomp_team_t *team; /* Info on the team */

  if (__mpcomp_do_single())
    return NULL;

 
  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  /* In this flow path, the number of threads should not be 1 */
  sctk_assert(t->info.num_threads > 0);

  /* Grab the team info */
  sctk_assert(t->instance != NULL);
  team = t->instance->team;
  sctk_assert(team != NULL);

  __mpcomp_barrier();

  return team->single_copyprivate_data;
}

void __mpcomp_do_single_copyprivate_end(void *data) {
  mpcomp_team_t *team; /* Info on the team */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  if( t->info.num_threads == 1 ) return;
  /* Grab the team info */
  sctk_assert(t->instance != NULL);
  team = t->instance->team;
  sctk_assert(team != NULL);

  team->single_copyprivate_data = data;

  __mpcomp_barrier();
}

void __mpcomp_single_coherency_entering_parallel_region(void) {}

void __mpcomp_single_coherency_end_barrier(void) {
  int i;

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  /* Number of threads in the current team */
  const long num_threads = t->info.num_threads;

  sctk_assert(num_threads > 0);

  for (i = 0; i < num_threads; i++) {
    /* mpcomp_thread_t *target_t = &(t->instance->mvps[i].ptr.mvp->threads[0]);
    sctk_debug("%s: thread %d single_sections_current:%d "
                 "single_sections_last_current:%d",
                 __func__, target_t->rank, target_t->single_sections_current,
                 OPA_load_int(
                     &(t->instance->team->single_sections_last_current))); */
  }
}

/* GOMP OPTIMIZED_1_0_WRAPPING */
#ifndef NO_OPTIMIZED_GOMP_4_0_API_SUPPORT
__asm__(".symver __mpcomp_do_single_copyprivate_end, "
        "GOMP_single_copy_end@@GOMP_1.0");
__asm__(".symver __mpcomp_do_single_copyprivate_begin, "
        "GOMP_single_copy_start@@GOMP_1.0");
__asm__(".symver __mpcomp_do_single, GOMP_single_start@@GOMP_1.0");
#endif /* OPTIMIZED_GOMP_API_SUPPORT */
