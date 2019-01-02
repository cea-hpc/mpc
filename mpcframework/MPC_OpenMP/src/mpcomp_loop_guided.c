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
#include "mpcomp_abi.h"
#include "mpcomp_barrier.h"
#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_loop_dyn.h"
#include "mpcomp_loop_dyn_utils.h"

#include "mpcomp_ompt_general.h"

/* 
   This file includes the function related to the 'guided' schedule of a shared
   for loop.
*/

/****
 *
 * LONG VERSION
 *
 *
 *****/

int __mpcomp_guided_loop_begin(long lb, long b, long incr, long chunk_size,
                               long *from, long *to) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_GUIDED_LOOP;
  t->schedule_is_forced = 1; 

  const int index = __mpcomp_loop_dyn_get_for_dyn_index(t);

  /* if current thread is too much ahead */
  while ( sctk_atomics_load_int(&(t->instance->team->for_dyn_nb_threads_exited[index].i)) ==
      MPCOMP_NOWAIT_STOP_SYMBOL) {
    sctk_thread_yield();
  }
  __mpcomp_loop_gen_infos_init(&(t->info.loop_infos), lb, b, incr, chunk_size);
  sctk_spinlock_lock(&(t->instance->team->lock));

  /* First thread store shared first iteration value */
  if(t->instance->team->is_first[index] == 1) {
    t->instance->team->is_first[index] = 0;
    long long int llb = lb;
    sctk_atomics_store_ptr(&(t->instance->team->guided_from[index]),(void *)llb);
  }
  long long int ret = (long long int)sctk_atomics_load_ptr(&(t->instance->team->guided_from[index]));
  sctk_spinlock_unlock(&(t->instance->team->lock));

  return (!from && !to) ? -1 : __mpcomp_guided_loop_next(from, to);
}

int __mpcomp_guided_loop_next(long *from, long *to) {

  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);
  mpcomp_loop_long_iter_t *loop = &(t->info.loop_infos.loop.mpcomp_long);
  const long num_threads = (long)t->info.num_threads;
  long long int ret = 0, anc_from,chunk_size,new_from;
  const int index = __mpcomp_loop_dyn_get_for_dyn_index(t);
  while(ret == 0) /* will loop again if another thread changed from value at the same time */
  {
    anc_from = (long long int)sctk_atomics_load_ptr(&(t->instance->team->guided_from[index]));
    chunk_size = (loop->b - anc_from) / (2*loop->incr * num_threads);
    if(chunk_size < 0) chunk_size = - chunk_size;
    if(loop->chunk_size > chunk_size) chunk_size = loop->chunk_size;

    if((loop->b - anc_from - chunk_size * loop->incr < 0 && loop->incr > 0) || (loop->b - anc_from - chunk_size * loop->incr > 0 && loop->incr < 0)) // Last iteration 
    {
      new_from = loop->b;
    }
    else
    {
      new_from = anc_from + chunk_size * loop->incr;
    }
    ret = (long long int)sctk_atomics_cas_ptr(&(t->instance->team->guided_from[index]), (void *)anc_from, (void *)new_from);
    ret = (ret == anc_from) ? 1 : 0;
  }
  *from = anc_from;
  *to = new_from;

  if(*from < *to && loop->incr > 0 || *from > *to && loop->incr < 0)
    return 1;
  else
    return 0;
}

void __mpcomp_guided_loop_end_nowait() 
{
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
				uint64_t ompt_iter_count = 0;
				ompt_iter_count = __mpcomp_internal_loop_get_num_iters_gen(&(t->info.loop_infos));
				ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
				const void* code_ra = __builtin_return_address(0);	
				callback( ompt_worksharing_loop, ompt_scope_end, parallel_data, NULL, ompt_iter_count, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  /* Get the team info */
  sctk_assert(t->instance != NULL);
  team_info = t->instance->team;
  sctk_assert(team_info != NULL);

  /* WARNING: the following order is important */
  sctk_spinlock_lock(&(t->info.update_lock));
  sctk_atomics_incr_int(&(t->for_dyn_ull_current));
  sctk_spinlock_unlock(&(t->info.update_lock));

  /* Update the number of threads which ended this loop */
  nb_threads_exited = sctk_atomics_fetch_and_incr_int(
      &(team_info->for_dyn_nb_threads_exited[index].i)) + 1;
  sctk_assert(nb_threads_exited > 0 && nb_threads_exited <= num_threads);

  if (nb_threads_exited == num_threads )
  {
    t->instance->team->is_first[index] = 1;

    const int previous_index = __mpcomp_loop_dyn_get_for_dyn_prev_index(t);
    sctk_assert(previous_index >= 0 &&
                previous_index < MPCOMP_MAX_ALIVE_FOR_DYN + 1);

    sctk_atomics_store_int(&(team_info->for_dyn_nb_threads_exited[index].i),
                           MPCOMP_NOWAIT_STOP_SYMBOL);
    int prev = sctk_atomics_swap_int(
        &(team_info->for_dyn_nb_threads_exited[previous_index].i), 0);
  }
}

void __mpcomp_guided_loop_end() {
  __mpcomp_guided_loop_end_nowait();
  __mpcomp_barrier();
}

/* Start a loop shared by the team w/ a guided schedule.
   !WARNING! This function assumes that there is no loops w/ guided schedule
   and nowait clause previously executed in the same parallel region 
*/
int __mpcomp_guided_loop_begin_ignore_nowait(long lb, long b, long incr,
                                             long chunk_size, long *from,
                                             long *to) {
  not_implemented();
  return 0;
}


int
__mpcomp_guided_loop_next_ignore_nowait (long *from, long *to)
{
     return __mpcomp_dynamic_loop_next_ignore_nowait(from, to);
}

/****
 *
 * ORDERED LONG VERSION
 *
 *
 *****/

int __mpcomp_ordered_guided_loop_begin(long lb, long b, long incr,
                                       long chunk_size, long *from, long *to) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  const int ret = __mpcomp_guided_loop_begin(lb, b, incr, chunk_size, from, to);
  if (from) {
    t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
  }
  return ret;
}

int __mpcomp_ordered_guided_loop_next(long *from, long *to) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  const int ret = __mpcomp_guided_loop_next(from, to);
  t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
  return ret;
}

void __mpcomp_ordered_guided_loop_end() { __mpcomp_guided_loop_end(); }

void __mpcomp_ordered_guided_loop_end_nowait() { __mpcomp_guided_loop_end(); }

/****
 *
 * ULL VERSION
 *
 *
 *****/
int __mpcomp_loop_ull_guided_begin(bool up, unsigned long long lb,
                                   unsigned long long b,
                                   unsigned long long incr,
                                   unsigned long long chunk_size,
                                   unsigned long long *from,
                                   unsigned long long *to) {

  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_GUIDED_LOOP;
  t->schedule_is_forced = 1;

  const int index = __mpcomp_loop_dyn_get_for_dyn_index(t);
  while ( sctk_atomics_load_int(&(t->instance->team->for_dyn_nb_threads_exited[index].i)) ==
      MPCOMP_NOWAIT_STOP_SYMBOL) {
    sctk_thread_yield();
  }
  __mpcomp_loop_gen_infos_init_ull(&(t->info.loop_infos), lb, b, incr, chunk_size);
  mpcomp_loop_ull_iter_t *loop = &(t->info.loop_infos.loop.mpcomp_ull);
  loop->up = up;

  sctk_spinlock_lock(&(t->instance->team->lock));

  if(t->instance->team->is_first[index] == 1) {
    t->instance->team->is_first[index] = 0;
    sctk_atomics_store_ptr(&(t->instance->team->guided_from[index]), (void *)lb);
  }
  unsigned long long ret = (unsigned long long)sctk_atomics_load_ptr(&(t->instance->team->guided_from[index]));
  sctk_spinlock_unlock(&(t->instance->team->lock));

  return (!from && !to) ? -1 : __mpcomp_loop_ull_guided_next(from, to);
}

int __mpcomp_loop_ull_guided_next(unsigned long long *from,
                                  unsigned long long *to) {

  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);
  mpcomp_loop_ull_iter_t *loop = &(t->info.loop_infos.loop.mpcomp_ull);
  const unsigned long long num_threads = (unsigned long long)t->info.num_threads;
  unsigned long long ret = 0, anc_from,chunk_size,new_from;
  const int index = __mpcomp_loop_dyn_get_for_dyn_index(t);

  while(ret == 0) /* will loop again if another thread changed from value at the same time */
  {
    anc_from = (unsigned long long)sctk_atomics_load_ptr(&(t->instance->team->guided_from[index]));
    if(loop->up)
    {
      chunk_size = ((loop->b - anc_from) / (loop->incr)) / (2*num_threads);
    }
    else 
    {
      chunk_size =  ((anc_from - loop->b) / (-loop->incr)) / (2*num_threads);
    }
    if(loop->chunk_size > chunk_size) chunk_size = loop->chunk_size;
    
    if(((loop->b - anc_from) / chunk_size < loop->incr && loop->up) || ((anc_from - loop->b) / chunk_size < (- loop->incr) && !loop->up)) // Last iteration 
    {
      new_from = loop->b;
    }
    else
    {
      new_from = anc_from + chunk_size * loop->incr;
    }
    ret = (unsigned long long)sctk_atomics_cas_ptr(&(t->instance->team->guided_from[index]), (void *)anc_from, (void *)new_from);
    ret = (ret == anc_from) ? 1 : 0;
  }
  *from = anc_from;
  *to = new_from;

  if(*from != *to)
    return 1;
  else
    return 0;

}

void __mpcomp_guided_loop_ull_end() {
  __mpcomp_guided_loop_end();
}

void __mpcomp_guided_loop_ull_end_nowait() {
  __mpcomp_guided_loop_end_nowait();
}

/****
 *
 * ULL ORDERED VERSION
 *
 *
 *****/
int __mpcomp_loop_ull_ordered_guided_begin(bool up, unsigned long long lb,
                                           unsigned long long b,
                                           unsigned long long incr,
                                           unsigned long long chunk_size,
                                           unsigned long long *from,
                                           unsigned long long *to) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  const int ret =
      __mpcomp_loop_ull_guided_begin(up, lb, b, incr, chunk_size, from, to);
  if (from) {
    t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
  }
  return ret;
}

int __mpcomp_loop_ull_ordered_guided_next(unsigned long long *from,
                                          unsigned long long *to) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  const int ret = __mpcomp_loop_ull_guided_next(from, to);
  t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
  return ret;
}
