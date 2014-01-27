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
#ifndef __mpcomp__H
#define __mpcomp__H

#ifdef __cplusplus
extern "C"
{
#endif

#include <sctk_config.h>
#include <errno.h>
#include <mpc.h>

/* Macros for OpenMP compliance */
#define omp_set_num_threads mpcomp_set_num_threads
#define omp_get_num_threads mpcomp_get_num_threads
#define omp_get_max_threads mpcomp_get_max_threads
#define omp_get_thread_num mpcomp_get_thread_num
#define omp_get_num_procs mpcomp_get_num_procs
#define omp_in_parallel mpcomp_in_parallel
#define omp_get_level mpcomp_get_level
#define omp_get_active_level mpcomp_get_active_level
#define omp_set_dynamic mpcomp_set_dynamic
#define omp_get_dynamic mpcomp_get_dynamic
#define omp_set_nested mpcomp_set_nested
#define omp_get_nested mpcomp_get_nested
#define omp_get_team_size mpcomp_get_team_size
#define omp_get_ancestor_thread_num mpcomp_get_ancestor_thread_num

#define omp_lock_t mpcomp_lock_t
#define omp_nest_lock_t mpcomp_nest_lock_t

#define omp_init_lock mpcomp_init_lock
#define omp_destroy_lock mpcomp_destroy_lock
#define omp_set_lock mpcomp_set_lock
#define omp_unset_lock mpcomp_unset_lock
#define omp_test_lock mpcomp_test_lock

#define omp_init_nest_lock mpcomp_init_nest_lock
#define omp_destroy_nest_lock mpcomp_destroy_nest_lock
#define omp_set_nest_lock mpcomp_set_nest_lock
#define omp_unset_nest_lock mpcomp_unset_nest_lock
#define omp_test_nest_lock mpcomp_test_nest_lock

#define omp_get_wtime mpcomp_get_wtime
#define omp_get_wtick mpcomp_get_wtick


/* OpenMP 2.5 API */
  void mpcomp_set_num_threads (int num_threads);
  int mpcomp_get_num_threads (void);
  int mpcomp_get_max_threads (void);
  int mpcomp_get_thread_num (void);
  int mpcomp_get_num_procs (void);
  int mpcomp_in_parallel (void);
  void mpcomp_set_dynamic (int dynamic_threads);
  int mpcomp_get_dynamic (void);
  void mpcomp_set_nested (int nested);
  int mpcomp_get_nested (void);

  struct mpcomp_lock_s
  {
    volatile long status;
    volatile long iter;
    volatile struct mpcomp_thread_info_s *owner;
    volatile struct mpcomp_slot_s *first;
    volatile struct mpcomp_slot_s *last;
    /* sctk_thread_mutex_t lock; */
    sctk_thread_spinlock_t lock;
  };


  //typedef struct mpcomp_lock_s mpcomp_lock_t;
  //typedef struct mpcomp_lock_s mpcomp_nest_lock_t;

  typedef sctk_thread_mutex_t mpcomp_lock_t;
  typedef sctk_thread_mutex_t mpcomp_nest_lock_t;

/* Lock Functions */
  void mpcomp_init_lock (mpcomp_lock_t * lock);
  void mpcomp_destroy_lock (mpcomp_lock_t * lock);
  void mpcomp_set_lock (mpcomp_lock_t * lock);
  void mpcomp_unset_lock (mpcomp_lock_t * lock);
  int mpcomp_test_lock (mpcomp_lock_t * lock);


/* Nestable Lock Fuctions */
  void mpcomp_init_nest_lock (mpcomp_nest_lock_t * lock);
  void mpcomp_destroy_nest_lock (mpcomp_nest_lock_t * lock);
  void mpcomp_set_nest_lock (mpcomp_nest_lock_t * lock);
  void mpcomp_unset_nest_lock (mpcomp_nest_lock_t * lock);
  int mpcomp_test_nest_lock (mpcomp_nest_lock_t * lock);

/* Timing Routines */
  double mpcomp_get_wtime (void);
  double mpcomp_get_wtick (void);

/* OpenMP 3.0 API */
  typedef enum omp_sched_t {
    omp_sched_static = 1,
    omp_sched_dynamic = 2,
    omp_sched_guided = 3,
    omp_sched_auto = 4,
  } omp_sched_t ;
  void omp_set_schedule (omp_sched_t kind, int modifier);
  void omp_get_schedule (omp_sched_t * kind, int *modifier);
#if 0
  int omp_get_thread_limit (void);
  void omp_set_max_active_levels (int max_levels);
  int omp_get_max_active_levels (void);
  int omp_get_level (void);
  int omp_get_ancestor_thread_num (int level);
  int omp_get_team_size (int level);
  int omp_get_active_level (void);
#endif

#ifdef __cplusplus
}
#endif

#endif
