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

#ifndef __MPCOMP_MPCOMP_LOOP_DYN_UTILS_H__
#define __MPCOMP_MPCOMP_LOOP_DYN_UTILS_H__

#include "sctk_debug.h"
#include "mpcomp_types.h"

/* dest = src1+src2 in base 'base' of size 'depth' up to dimension 'max_depth'
 */
static inline int __mpcomp_loop_dyn_dynamic_add(int *dest, int *src1, int *src2,
                                                int *base, int depth,
                                                int max_depth,
                                                int include_carry_over) {
  int i, ret, carry_over;

  ret = 1;
  /* Step to the next target */
  for (i = depth - 1; i >= max_depth; i--) {
    const int value = src1[i] + src2[i];
    dest[i] = value;
    carry_over = 0;

    if (value >= base[i]) {
      ret = (i == max_depth) ? 0 : ret;
      carry_over = (include_carry_over) ? value / base[i] : carry_over;
      dest[i] = (value % base[i]);
    }
  }

  return ret;
}

/* Return 1 if overflow, otherwise 0 */
static inline int __mpcomp_loop_dyn_dynamic_increase(int *target, int *base,
                                                     int depth, int max_depth,
                                                     int include_carry_over) {
  int i, carry_over, ret;

  ret = 0;
  carry_over = 1;

  /* Step to the next target */
  for (i = depth - 1; i >= max_depth; i--) {
    const int value = target[i] + carry_over;
    carry_over = 0;
    target[i] = value;

    if (value >= base[i]) {
      ret = (i == max_depth) ? 1 : ret;
      carry_over = (include_carry_over) ? value / base[i] : carry_over;
      target[i] = value % base[i];
    }
  }

  return ret;
}

static inline int
__mpcomp_loop_dyn_get_for_dyn_current(mpcomp_thread_t *thread) {
  return sctk_atomics_load_int(&(thread->for_dyn_ull_current));
}

static inline int
__mpcomp_loop_dyn_get_for_dyn_prev_index(mpcomp_thread_t *thread) {
  const int for_dyn_current = __mpcomp_loop_dyn_get_for_dyn_current(thread);
  return (for_dyn_current + MPCOMP_MAX_ALIVE_FOR_DYN - 1) %
         (MPCOMP_MAX_ALIVE_FOR_DYN + 1);
}

static inline int __mpcomp_loop_dyn_get_for_dyn_index(mpcomp_thread_t *thread) {
  const int for_dyn_current = __mpcomp_loop_dyn_get_for_dyn_current(thread);
  return (for_dyn_current % (MPCOMP_MAX_ALIVE_FOR_DYN + 1));
}


static inline void
__mpcomp_loop_dyn_init_target_chunk_ull(mpcomp_thread_t *thread,
                                        mpcomp_thread_t *target,
                                        unsigned int num_threads) {
  /* Compute the index of the dynamic for construct */
  const int index = __mpcomp_loop_dyn_get_for_dyn_index(thread);
  int cur = sctk_atomics_load_int(&(target->for_dyn_remain[index].i));

  if (cur < 0 )
	{
    if( !sctk_spinlock_trylock(&(target->info.update_lock))) {
      /* Get the current id of remaining chunk for the target */
      int cur = sctk_atomics_load_int(&(target->for_dyn_remain[index].i));
      if (cur < 0 && (__mpcomp_loop_dyn_get_for_dyn_current(thread) >
                      __mpcomp_loop_dyn_get_for_dyn_current(target) || __mpcomp_loop_dyn_get_for_dyn_current(target) == 0)) {
          target->for_dyn_total[index] = __mpcomp_get_static_nb_chunks_per_rank_ull(
          (unsigned long long) target->rank, (unsigned long long) num_threads, &(thread->info.loop_infos.loop.mpcomp_ull));
          int ret = sctk_atomics_cas_int(&(target->for_dyn_remain[index].i), -1,
                                     target->for_dyn_total[index]);
      }
      sctk_spinlock_unlock(&(target->info.update_lock));
    }
	}
}

static inline void
__mpcomp_loop_dyn_init_target_chunk(mpcomp_thread_t *thread,
                                    mpcomp_thread_t *target,
                                    unsigned int num_threads) {
  /* Compute the index of the dynamic for construct */
  const int index = __mpcomp_loop_dyn_get_for_dyn_index(thread);
  int cur = sctk_atomics_load_int(&(target->for_dyn_remain[index].i));

  if (cur < 0 )
	{
	  if( !sctk_spinlock_trylock(&(target->info.update_lock))) {
      /* Get the current id of remaining chunk for the target */
      int cur = sctk_atomics_load_int(&(target->for_dyn_remain[index].i));
      if (cur < 0 && (__mpcomp_loop_dyn_get_for_dyn_current(thread) >
                    __mpcomp_loop_dyn_get_for_dyn_current(target) || __mpcomp_loop_dyn_get_for_dyn_current(target) == 0)) {
        target->for_dyn_total[index] = __mpcomp_get_static_nb_chunks_per_rank(
        target->rank, num_threads, &(thread->info.loop_infos.loop.mpcomp_long));
        int ret = sctk_atomics_cas_int(&(target->for_dyn_remain[index].i), -1,
                                   target->for_dyn_total[index]);
      }
      sctk_spinlock_unlock(&(target->info.update_lock));
	  }
  }
}


static inline int
__mpcomp_loop_dyn_get_chunk_from_target(mpcomp_thread_t *thread,
                                        mpcomp_thread_t *target) {
  int prev, cur;

  /* Compute the index of the dynamic for construct */
  const int index = __mpcomp_loop_dyn_get_for_dyn_index(target);
  cur = sctk_atomics_load_int(&(target->for_dyn_remain[index].i));

  /* Init target chunk if it is not */
  if(cur < 0) {
    if(thread->info.loop_infos.type == MPCOMP_LOOP_TYPE_LONG)
    {
      __mpcomp_loop_dyn_init_target_chunk(thread,target,thread->info.num_threads);
    }
    else
    {
      __mpcomp_loop_dyn_init_target_chunk_ull(thread,target,thread->info.num_threads);
    }
    cur = sctk_atomics_load_int(&(target->for_dyn_remain[index].i));
  }

  if (cur <= 0) {
    return 0;
  }

  int success = 0;
  do {
    if (thread == target) {
      prev = cur;
      cur = sctk_atomics_cas_int(&(target->for_dyn_remain[index].i), prev,
                                 prev - 1);
      success = (cur == prev);
    } else {
      if (__mpcomp_loop_dyn_get_for_dyn_current(thread) >=
          __mpcomp_loop_dyn_get_for_dyn_current(target) || __mpcomp_loop_dyn_get_for_dyn_current(target) == 0) {
        if (!sctk_spinlock_trylock(&(target->info.update_lock))) {
          cur = sctk_atomics_load_int(&(target->for_dyn_remain[index].i));
          if ((__mpcomp_loop_dyn_get_for_dyn_current(thread) >=
               __mpcomp_loop_dyn_get_for_dyn_current(target) || __mpcomp_loop_dyn_get_for_dyn_current(target) == 0) && cur > 0) {
            prev = cur;
            cur = sctk_atomics_cas_int(&(target->for_dyn_remain[index].i), prev,
                                       prev - 1);
            success = (cur == prev);
          } else {
            /* NO MORE STEAL */
            cur = 0;
          }
          sctk_spinlock_unlock(&(target->info.update_lock));
        } else {
          success = 0;
        }
      } else {
        /* NO MORE STEAL */
        cur = 0;
      }
    }
  } while (cur > 0 && !success);

  return (cur > 0 && success) ? cur : 0;
}


static inline int __mpcomp_loop_dyn_get_victim_rank(mpcomp_thread_t *thread) {
    int i, target;
    int* tree_cumulative;

    sctk_assert(thread);
    sctk_assert(thread->instance);

    tree_cumulative = thread->instance->tree_cumulative +1;
    const int tree_depth = thread->instance->tree_depth -1;
    

    sctk_assert(thread->for_dyn_target);
    sctk_assert(thread->instance->tree_cumulative);

    for (i = 0, target = 0; i < tree_depth; i++) {
        target += thread->for_dyn_target[i] * tree_cumulative[i];
    }

	target = ( thread->rank + target ) % thread->instance->nb_mvps;

    sctk_assert(target >= 0);
    sctk_assert(target < thread->instance->nb_mvps);
	//fprintf(stderr, "[%d] ::: %s ::: Get Victim  %d\n", thread->rank, __func__, target );

  return target;
}

static inline void __mpcomp_loop_dyn_target_reset(mpcomp_thread_t *thread) {
  int i;
  sctk_assert(thread);

  sctk_assert(thread->instance);
  const int tree_depth = thread->instance->tree_depth -1;

  sctk_assert(thread->mvp);
  sctk_assert(thread->mvp->tree_rank);

  sctk_assert(thread->for_dyn_target);
  sctk_assert(thread->for_dyn_shift);

  /* Re-initialize the target to steal */
  for (i = 0; i < tree_depth; i++) {
    thread->for_dyn_shift[i] = 0;
    thread->for_dyn_target[i] = thread->mvp->tree_rank[i];
  }
}

static inline void __mpcomp_loop_dyn_target_init(mpcomp_thread_t *thread) {
  sctk_assert(thread);

  if (thread->for_dyn_target)
    return;

  sctk_assert(thread->instance);
  const int tree_depth = thread->instance->tree_depth -1;

  thread->for_dyn_target = (int *)malloc(tree_depth * sizeof(int));
  sctk_assert(thread->for_dyn_target);

  thread->for_dyn_shift = (int *)malloc(tree_depth * sizeof(int));
  sctk_assert(thread->for_dyn_shift);

  sctk_assert(thread->mvp);
  sctk_assert(thread->mvp->tree_rank);

  __mpcomp_loop_dyn_target_reset(thread);

  sctk_nodebug("[%d] %s: initialization of target and shift", thread->rank,
               __func__);
}

#endif /* __MPCOMP_MPCOMP_LOOP_DYN_UTILS_H__ */
