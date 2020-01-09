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

#include "mpcomp_loop.h"
#include "mpcomp_barrier.h"
#include "mpcomp_loop_static.h"
#include "mpcomp_loop_dyn.h"
#include "mpcomp_loop_static.h"
#include "mpcomp_loop_dyn_utils.h"

#if OMPT_SUPPORT
#include "ompt.h"
#include "mpcomp_ompt_general.h"
#endif /* OMPT_SUPPORT */

static int __mpcomp_dynamic_loop_get_chunk_from_rank(mpcomp_thread_t *t,
                                                     mpcomp_thread_t *target,
                                                     long *from, long *to) {

  int cur;

  if ( !target ) return 0;

  const long rank = (long)target->rank;
  const long num_threads = (long)t->info.num_threads;

  sctk_assert(t->info.loop_infos.type == MPCOMP_LOOP_TYPE_LONG);
  mpcomp_loop_long_iter_t *loop = &(t->info.loop_infos.loop.mpcomp_long);

  cur = __mpcomp_loop_dyn_get_chunk_from_target(t, target);

  if (cur < 0) {
    return 0;
  }


  __mpcomp_static_schedule_get_specific_chunk(rank, num_threads, loop,
                                              cur, from, to);

  return 1;
}

void __mpcomp_dynamic_loop_init(mpcomp_thread_t *t, long lb, long b, long incr,
                                long chunk_size) {
  mpcomp_team_t *team_info; /* Info on the team */
                            /* Get the team info */
  sctk_assert(t->instance != NULL);
  team_info = t->instance->team;
  sctk_assert(team_info != NULL);
  /* WORKAROUND (pr35196.c)
   * Reinitialize the flag for the last iteration of the loop */
  t->for_dyn_last_loop_iteration = 0;

  const long num_threads = (long)t->info.num_threads;
  const int index = __mpcomp_loop_dyn_get_for_dyn_index(t);

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_DYN_LOOP;
  t->schedule_is_forced = 0;

  /* Stop if the maximum number of alive loops is reached */
  while (
      sctk_atomics_load_int(&(team_info->for_dyn_nb_threads_exited[index].i)) ==
      MPCOMP_NOWAIT_STOP_SYMBOL) {
    sctk_thread_yield();
  }

  /* Fill private info about the loop */
  __mpcomp_loop_gen_infos_init(&(t->info.loop_infos), lb, b, incr, chunk_size);

  /* Try to change the number of remaining chunks */
  t->dyn_last_target = t ;
  t->for_dyn_total[index] = __mpcomp_get_static_nb_chunks_per_rank(
      t->rank, num_threads, &(t->info.loop_infos.loop.mpcomp_long));
  (void) sctk_atomics_cas_int(&(t->for_dyn_remain[index].i), -1, t->for_dyn_total[index]);
}

int __mpcomp_dynamic_loop_begin(long lb, long b, long incr, long chunk_size,
                                long *from, long *to) {
  	mpcomp_thread_t *t; /* Info on the current thread */

  	/* Handle orphaned directive (initialize OpenMP environment) */
  	__mpcomp_init();

  	/* Grab the thread info */
  	t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  	sctk_assert(t != NULL);

    if( !( t->instance->root ) )
    {
        *from = lb;
        *to = b;
  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_DYN_LOOP;
  t->schedule_is_forced = 0;
        return 1;
    }
  	/* Initialization of loop internals */
  	t->for_dyn_last_loop_iteration = 0;
  	__mpcomp_dynamic_loop_init(t, lb, b, incr, chunk_size);

#if OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
   	    if( OMPT_Callbacks )
   	    {
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
				uint64_t ompt_iter_count =
				  __mpcomp_internal_loop_get_num_iters_gen(&(t->info.loop_infos));
				ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
                ompt_data_t* task_data = &( t->task_infos.current_task->ompt_task_data );
				const void* code_ra = __ompt_get_return_address(2);
				callback( ompt_work_loop, ompt_scope_begin, parallel_data, task_data, ompt_iter_count, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  return (!from && !to) ? -1 : __mpcomp_dynamic_loop_next(from, to);
}

int __mpcomp_dynamic_loop_next(long *from, long *to) 
{
    mpcomp_mvp_t* target_mvp;
    mpcomp_thread_t *t, *target;
    int found, target_idx, barrier_nthreads, ret;

    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert( t != NULL );

    /* Stop the stealing at the following depth.
     * Nodes above this depth will not be traversed */
    const int num_threads = t->info.num_threads;

    /* 1 thread => no stealing, directly get a chunk */
    if (num_threads == 1)
        return __mpcomp_dynamic_loop_get_chunk_from_rank(t, t, from, to);

    /* Check that the target is allocated */
    __mpcomp_loop_dyn_target_init(t);


    /*
     * WORKAROUND (pr35196.c):
     * Stop if the current thread already executed
     * the last iteration of the current loop
     */
    if (t->for_dyn_last_loop_iteration == 1) {
        __mpcomp_loop_dyn_target_reset(t);
        return 0;
    }

    int *tree_base = t->instance->tree_base + 1;
    const int tree_depth = t->instance->tree_depth -1;
    const int max_depth = t->instance->root->depth;

    /* Compute the index of the target */
    target = t->dyn_last_target;
    found = 1;
    ret = 0;

    /* While it is not possible to get a chunk */
    while(!__mpcomp_dynamic_loop_get_chunk_from_rank( t, target, from, to)) 
    {

      do_increase:

      if( ret ) 
      {
          found = 0;
          break;
      }

      ret = __mpcomp_loop_dyn_dynamic_increase(t->for_dyn_shift, tree_base, tree_depth, max_depth, 1);
      /* Add t->for_dyn_shift and t->mvp->tree_rank[i] w/out carry over and store it into t->for_dyn_target */
      __mpcomp_loop_dyn_dynamic_add(t->for_dyn_target, t->for_dyn_shift, t->mvp->tree_rank, tree_base, tree_depth, max_depth, 0);

      /* Compute the index of the target */
      target_idx = __mpcomp_loop_dyn_get_victim_rank(t);
      target_mvp = t->instance->mvps[target_idx].ptr.mvp;
      target = ( target_mvp ) ? target_mvp->threads : NULL;

      if( !( target ) ) goto do_increase;
      barrier_nthreads = target_mvp->father->barrier_num_threads;

      /* Stop if the target is not a launch thread */
      if (t->for_dyn_target[tree_depth - 1] >= barrier_nthreads)
        goto do_increase;
    }
    
  /* Did we exit w/ or w/out a chunk? */
  if (found) {
    /* WORKAROUND (pr35196.c):
     * to avoid issue w/ lastprivate and GCC code generation,
     * check if this is the last chunk to avoid further execution
     * for this thread.
     */
    if (*to == t->info.loop_b) {
      t->for_dyn_last_loop_iteration = 1;
    }
    t->dyn_last_target = target;
    return 1;
  }

  return 0;
}

void __mpcomp_dynamic_loop_end_nowait(void) {
  int nb_threads_exited;
  mpcomp_thread_t *t;       /* Info on the current thread */
  mpcomp_team_t *team_info; /* Info on the team */

  /* Grab the thread info */
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Number of threads in the current team */
  const int num_threads = t->info.num_threads;

  /* Compute the index of the dynamic for construct */
  const int index = __mpcomp_loop_dyn_get_for_dyn_index(t);
  sctk_assert(index >= 0 && index < MPCOMP_MAX_ALIVE_FOR_DYN + 1);

#if OMPT_SUPPORT
	/* Avoid double call during runtime schedule policy */
	if( mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
				uint64_t ompt_iter_count = 
				  __mpcomp_internal_loop_get_num_iters_gen(&(t->info.loop_infos));
				ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
                ompt_data_t* task_data = &( t->task_infos.current_task->ompt_task_data );
				const void* code_ra = __builtin_return_address(0);	
				callback( ompt_work_loop, ompt_scope_end, parallel_data, task_data, ompt_iter_count, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */
  /* In case of 1 thread, re-initialize the number of remaining chunks
       * but do not increase the current index */
  if (num_threads == 1) {
    sctk_atomics_store_int(&(t->for_dyn_remain[index].i), -1);
    return;
  }

  /* Get the team info */
  sctk_assert(t->instance != NULL);
  team_info = t->instance->team;
  sctk_assert(team_info != NULL);

  /* WARNING: the following order is important */
  sctk_spinlock_lock(&(t->info.update_lock));
  sctk_atomics_incr_int(&(t->for_dyn_ull_current));
  sctk_atomics_store_int(&(t->for_dyn_remain[index].i), -1);
  sctk_spinlock_unlock(&(t->info.update_lock));

  /* Update the number of threads which ended this loop */
  nb_threads_exited = sctk_atomics_fetch_and_incr_int(
      &(team_info->for_dyn_nb_threads_exited[index].i)) + 1;
  sctk_assert(nb_threads_exited > 0 && nb_threads_exited <= num_threads);

  if (nb_threads_exited == num_threads ) 
  {
    
    const int previous_index = __mpcomp_loop_dyn_get_for_dyn_prev_index(t);
    sctk_assert(previous_index >= 0 &&
                previous_index < MPCOMP_MAX_ALIVE_FOR_DYN + 1);

    sctk_atomics_store_int(&(team_info->for_dyn_nb_threads_exited[index].i),
                           MPCOMP_NOWAIT_STOP_SYMBOL);
    sctk_atomics_swap_int(
        &(team_info->for_dyn_nb_threads_exited[previous_index].i), 0);
     
  }

}

void __mpcomp_dynamic_loop_end(void) {
  __mpcomp_dynamic_loop_end_nowait();
  __mpcomp_barrier();
}

int __mpcomp_dynamic_loop_next_ignore_nowait(__UNUSED__ long *from, __UNUSED__ long *to) {
  not_implemented();
  return 0;
}

/****
 *
 * ORDERED LONG VERSION
 *
 *
 *****/

int __mpcomp_ordered_dynamic_loop_begin(long lb, long b, long incr,
                                        long chunk_size, long *from, long *to) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  __mpcomp_loop_gen_infos_init(&(t->info.loop_infos), lb, b, incr, chunk_size);
  const int ret =
      __mpcomp_dynamic_loop_begin(lb, b, incr, chunk_size, from, to);
  if (from) {
    t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
  }
  return ret;
}

int __mpcomp_ordered_dynamic_loop_next(long *from, long *to) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  const int ret = __mpcomp_dynamic_loop_next(from, to);
  t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
  return ret;
}

void __mpcomp_ordered_dynamic_loop_end() { __mpcomp_dynamic_loop_end(); }

void __mpcomp_ordered_dynamic_loop_end_nowait() { __mpcomp_dynamic_loop_end(); }

void __mpcomp_for_dyn_coherency_end_parallel_region(
    mpcomp_instance_t *instance) {
  mpcomp_thread_t *t_first;
  mpcomp_team_t *team;
  int i;
  int n;

  team = instance->team;
  sctk_assert(team != NULL);

  const int tree_depth = instance->tree_depth -1;
  sctk_nodebug("[X] __mpcomp_for_dyn_coherency_checking_end_barrier: "
               "Checking for %d thread(s)...",
               team->info.num_threads);

  /* Check team coherency */
  n = 0;
  for (i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN + 1; i++) {
    switch (sctk_atomics_load_int(&team->for_dyn_nb_threads_exited[i].i)) {
    case 0:
      break;
    case MPCOMP_NOWAIT_STOP_SYMBOL:
      n++;
      break;
    default:
      /* This array should contain only '0' and
             * one MPCOMP_MAX_ALIVE_FOR_DYN */
      not_reachable();
      break;
    }

    sctk_nodebug("[X] __mpcomp_for_dyn_coherency_checking_end_barrier: "
                 "TEAM - FOR_DYN - nb_threads_exited[%d] = %d",
                 i, team->for_dyn_nb_threads_exited[i].i);
  }

  if (n != 1) {
    /* This array should contain exactly 1 MPCOMP_MAX_ALIVE_FOR_DYN */
    not_reachable();
  }

  /* Check tree coherency */

  /* Check thread coherency */
  t_first = &(instance->mvps[0].ptr.mvp->threads[0]);
  for (i = 0; i < team->info.num_threads; i++) {
    mpcomp_thread_t *target_t;
    int j;

    target_t = &(instance->mvps[i].ptr.mvp->threads[0]);

    sctk_nodebug("[X] __mpcomp_for_dyn_coherency_checking_end_barrier: "
                 "Checking thread %d...",
                 target_t->rank);

    /* Checking for_dyn_current */
    if (t_first->for_dyn_current != target_t->for_dyn_current) {
      not_reachable();
    }

    /* Checking for_dyn_remain */
    for (j = 0; j < MPCOMP_MAX_ALIVE_FOR_DYN + 1; j++) {
      if (sctk_atomics_load_int(&(target_t->for_dyn_remain[j].i)) != -1) {
        not_reachable();
      }
    }

    /* Checking for_dyn_target */
    /* for_dyn_tarfer can be NULL if there were no dynamic loop
           * since the beginning of the program
           */
    if (target_t->for_dyn_target != NULL) {
      for (j = 0; j < tree_depth; j++) {
        if (target_t->for_dyn_target[j] != target_t->mvp->tree_rank[j]) {
          sctk_nodebug("[X] __mpcomp_for_dyn_coherency_checking_end_barrier: "
                       "error rank %d has target[%d] = %d (depth: %d, "
                       "max_depth: %d, for_dyn_current=%d)",
                       target_t->rank, j, target_t->for_dyn_target[j],
                       tree_depth, team->info.new_root->depth,
                       target_t->for_dyn_current);

          not_reachable();
        }
      }
    }

    /* Checking for_dyn_shift */
    /* for_dyn_shift can be NULL if there were no dynamic loop
           * since the beginning of the program
           */
    if (target_t->for_dyn_shift != NULL) {
      // for ( j = t->info.new_root->depth ; j < instance->tree_depth ; j++ )
      for (j = 0; j < tree_depth; j++) {
        if (target_t->for_dyn_shift[j] != 0) {
          sctk_nodebug("[X] __mpcomp_for_dyn_coherency_checking_end_barrier: "
                       "error rank %d has shift[%d] = %d (depth: %d, "
                       "max_depth: %d, for_dyn_current=%d)",
                       target_t->rank, j, target_t->for_dyn_shift[j],
                       tree_depth, team->info.new_root->depth,
                       target_t->for_dyn_current);

          not_reachable();
        }
      }
    }
  }
}
