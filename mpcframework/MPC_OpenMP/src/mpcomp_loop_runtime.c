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
#include "sctk_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_loop_runtime.h"
#include "mpcomp_openmp_tls.h"

TODO(runtime schedule: ICVs are not well transfered!)

/****
 *
 * LONG VERSION
 *
 *
 *****/
int __mpcomp_runtime_loop_begin(long lb, long b, long incr, long *from,
                                long *to) {
  int ret;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_RUNTIME_LOOP;
  t->schedule_is_forced = 1;

  const int run_sched_var = t->info.icvs.run_sched_var;
  const long chunk_size = t->info.icvs.modifier_sched_var;

  switch (run_sched_var) {
  case omp_sched_static:
    ret = __mpcomp_static_loop_begin(lb, b, incr, chunk_size, from, to);
    break;
  case omp_sched_dynamic:
    ret = __mpcomp_dynamic_loop_begin(lb, b, incr, chunk_size, from, to);
    break;
  case omp_sched_guided:
    ret = __mpcomp_guided_loop_begin(lb, b, incr, chunk_size, from, to);
    break;
  default:
    not_reachable();
  }

  return ret;
}

int __mpcomp_runtime_loop_next(long *from, long *to) {
  int ret;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int run_sched_var = t->info.icvs.run_sched_var;

  switch (run_sched_var) {
  case omp_sched_static:
    ret = __mpcomp_static_loop_next(from, to);
    break;
  case omp_sched_dynamic:
    ret = __mpcomp_dynamic_loop_next(from, to);
    break;
  case omp_sched_guided:
    ret = __mpcomp_guided_loop_next(from, to);
    break;
  default:
    not_reachable();
  }

  return ret;
}

void __mpcomp_runtime_loop_end(void) {
  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int run_sched_var = t->info.icvs.run_sched_var;

  switch (run_sched_var) {
  case omp_sched_static:
    __mpcomp_static_loop_end();
    break;
  case omp_sched_dynamic:
    __mpcomp_dynamic_loop_end();
    break;
  case omp_sched_guided:
    __mpcomp_guided_loop_end();
    break;
  default:
    not_reachable();
  }
}

void __mpcomp_runtime_loop_end_nowait() {
  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int run_sched_var = t->info.icvs.run_sched_var;

  switch (run_sched_var) {
  case omp_sched_static:
    __mpcomp_static_loop_end_nowait();
    break;
  case omp_sched_dynamic:
    __mpcomp_dynamic_loop_end_nowait();
    break;
  case omp_sched_guided:
    __mpcomp_guided_loop_end_nowait();
    break;
  default:
    not_reachable();
  }
}

/****
 *
 * ORDERED LONG VERSION 
 *
 *
 *****/

int __mpcomp_ordered_runtime_loop_begin(long lb, long b, long incr, long *from,
                                        long *to) {
  int ret;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_RUNTIME_LOOP;
  t->schedule_is_forced = 1;

  const int run_sched_var = t->info.icvs.run_sched_var;
  const long chunk_size = t->info.icvs.modifier_sched_var;

  switch (run_sched_var) {
  case omp_sched_static:
    ret = __mpcomp_ordered_static_loop_begin(lb, b, incr, chunk_size, from, to);
    break;
  case omp_sched_dynamic:
    ret =
        __mpcomp_ordered_dynamic_loop_begin(lb, b, incr, chunk_size, from, to);
    break;
  case omp_sched_guided:
    ret = __mpcomp_ordered_guided_loop_begin(lb, b, incr, chunk_size, from, to);
    break;
  default:
    not_reachable();
  }

  return ret;
}

int __mpcomp_ordered_runtime_loop_next(long *from, long *to) {
  int ret;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int run_sched_var = t->info.icvs.run_sched_var;

  switch (run_sched_var) {
  case omp_sched_static:
    ret = __mpcomp_ordered_static_loop_next(from, to);
    break;
  case omp_sched_dynamic:
    ret = __mpcomp_ordered_dynamic_loop_next(from, to);
    break;
  case omp_sched_guided:
    ret = __mpcomp_ordered_guided_loop_next(from, to);
    break;
  default:
    not_reachable();
  }

  return ret;
}

void __mpcomp_ordered_runtime_loop_end(void) { __mpcomp_runtime_loop_end(); }

void __mpcomp_ordered_runtime_loop_end_nowait(void) {
  __mpcomp_runtime_loop_end();
}

/****
 *
 * ULL VERSION
 *
 *
 *****/
int __mpcomp_loop_ull_runtime_begin(bool up, unsigned long long lb,
                                    unsigned long long b,
                                    unsigned long long incr,
                                    unsigned long long *from,
                                    unsigned long long *to) {
  int ret;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_RUNTIME_LOOP;
  t->schedule_is_forced = 1;

  const int run_sched_var = t->info.icvs.run_sched_var;
  const unsigned long long chunk_size =
      (unsigned long long)t->info.icvs.modifier_sched_var;

  switch (t->info.icvs.run_sched_var) {
  case omp_sched_static:
    ret = __mpcomp_static_loop_begin_ull(up, lb, b, incr, chunk_size, from, to);
    break;
  case omp_sched_dynamic:
    ret =
        __mpcomp_loop_ull_dynamic_begin(up, lb, b, incr, chunk_size, from, to);
    break;
  case omp_sched_guided:
    ret = __mpcomp_loop_ull_guided_begin(up, lb, b, incr, chunk_size, from, to);
    break;
  default:
    not_reachable();
  }

  return ret;
}

int __mpcomp_loop_ull_runtime_next(unsigned long long *from,
                                   unsigned long long *to) {
  int ret;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  switch (t->info.icvs.run_sched_var) {
  case omp_sched_static:
    ret = __mpcomp_static_loop_next_ull(from, to);
    break;
  case omp_sched_dynamic:
    ret = __mpcomp_loop_ull_dynamic_next(from, to);
    break;
  case omp_sched_guided:
    ret = __mpcomp_loop_ull_guided_next(from, to);
    break;
  default:
    not_reachable();
  }

  return ret;
}

/****
 *
 * ORDERED ULL VERSION
 *
 *
 *****/
int __mpcomp_loop_ull_ordered_runtime_begin(bool up, unsigned long long lb,
                                            unsigned long long b,
                                            unsigned long long incr,
                                            unsigned long long *from,
                                            unsigned long long *to) {
  int ret;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_RUNTIME_LOOP;
  t->schedule_is_forced = 1;

  const int run_sched_var = t->info.icvs.run_sched_var;
  const unsigned long long chunk_size =
      (unsigned long long)t->info.icvs.modifier_sched_var;

  sctk_nodebug("%s: value of schedule %d", __func__, run_sched_var);

  switch (run_sched_var) {
  case omp_sched_static:
    ret = __mpcomp_ordered_static_loop_begin_ull(up, lb, b, incr, chunk_size,
                                                 from, to);
    break;
  case omp_sched_dynamic:
    ret = __mpcomp_loop_ull_ordered_dynamic_begin(up, lb, b, incr, chunk_size,
                                                  from, to);
    break;
  case omp_sched_guided:
    ret = __mpcomp_loop_ull_ordered_guided_begin(up, lb, b, incr, chunk_size,
                                                 from, to);
    break;
  default:
    not_reachable();
  }

  return ret;
}

int __mpcomp_loop_ull_ordered_runtime_next(unsigned long long *from,
                                           unsigned long long *to) {
  int ret;

  /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

  /* Grab the thread info */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int run_sched_var = t->info.icvs.run_sched_var;

  switch (run_sched_var) {
  case omp_sched_static:
    ret = __mpcomp_ordered_static_loop_next_ull(from, to);
    break;
  case omp_sched_dynamic:
    ret = __mpcomp_loop_ull_ordered_dynamic_next(from, to);
    break;
  case omp_sched_guided:
    ret = __mpcomp_loop_ull_ordered_guided_next(from, to);
    break;
  default:
    not_reachable();
  }

  return ret;
}
