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

/* OpenMP 2.5 API - For backward compatibility with old patched GCC */
  int mpcomp_get_num_threads (void);
  int mpcomp_get_thread_num (void);

/* OpenMP 2.5 API */
  void omp_set_num_threads (int num_threads);
  int omp_get_num_threads (void);
  int omp_get_max_threads (void);
  int omp_get_thread_num (void);
  int omp_get_num_procs (void);
  int omp_in_parallel (void);
  void omp_set_dynamic (int dynamic_threads);
  int omp_get_dynamic (void);
  void omp_set_nested (int nested);
  int omp_get_nested (void);

    typedef enum omp_lock_hint_t 
    {
        omp_lock_hint_none = 0,
        omp_lock_hint_uncontended = 1,
        omp_lock_hint_contended = 2,
        omp_lock_hint_nonspeculative = 4,
        omp_lock_hint_speculative = 8
    } omp_lock_hint_t;
  
    typedef struct mpcomp_lock_s
    {
        omp_lock_hint_t hint;
        sctk_thread_mutex_t lock;
    } mpcomp_lock_t; 
    
    typedef mpcomp_lock_t* omp_lock_t;

    typedef struct mpcomp_nest_lock_s
    {
        int nb_nested ; /* Number of times this lock is held */
        void * owner_thread ; /* Owner of the lock */
        void * owner_task; /* Owner of the lock */
        omp_lock_hint_t hint;
        sctk_thread_mutex_t lock;
    } mpcomp_nest_lock_t;
 
    typedef mpcomp_nest_lock_t* omp_nest_lock_t;

  /* Lock Functions */
  void omp_init_lock (omp_lock_t* lock);
  void omp_init_lock_with_hint (omp_lock_t* lock, omp_lock_hint_t hint);
  void omp_destroy_lock (omp_lock_t* lock);
  void omp_set_lock (omp_lock_t* lock);
  void omp_unset_lock (omp_lock_t* lock);
  int omp_test_lock (omp_lock_t* lock);


/* Nestable Lock Fuctions */
  void omp_init_nest_lock (omp_nest_lock_t* lock);
  void omp_init_nest_lock_with_hint (omp_nest_lock_t* lock, omp_lock_hint_t hint);
  void omp_destroy_nest_lock (omp_nest_lock_t* lock);
  void omp_set_nest_lock (omp_nest_lock_t* lock);
  void omp_unset_nest_lock (omp_nest_lock_t* lock);
  int omp_test_nest_lock (omp_nest_lock_t* lock);

/* Timing Routines */
  double omp_get_wtime (void);
  double omp_get_wtick (void);

/* OpenMP 3.0 API */
  typedef enum omp_sched_t {
    omp_sched_static = 1,
    omp_sched_dynamic = 2,
    omp_sched_guided = 3,
    omp_sched_auto = 4,
  } omp_sched_t ;
  void omp_set_schedule (omp_sched_t kind, int modifier);
  void omp_get_schedule (omp_sched_t * kind, int *modifier);
  int omp_get_thread_limit (void);
  void omp_set_max_active_levels (int max_levels);
  int omp_get_max_active_levels (void);
  int omp_get_level (void);
  int omp_get_ancestor_thread_num (int level);
  int omp_get_team_size (int level);
  int omp_get_active_level (void);
  int omp_in_final ();

#ifdef __cplusplus
}
#endif

#endif
