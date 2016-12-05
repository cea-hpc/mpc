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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_LOCK_H__
#define __MPCOMP_LOCK_H__

#include "mpcomp.h"
#include "sctk_alloc.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "sctk_debug.h"

/* REGULAR LOCKS */
void omp_init_lock(omp_lock_t *lock);
void omp_destroy_lock(omp_lock_t *lock);
void omp_set_lock(omp_lock_t *lock);
void omp_unset_lock(omp_lock_t *lock);
int omp_test_lock(omp_lock_t *lock);

/* NESTED LOCKS */
void omp_init_nest_lock(omp_nest_lock_t *lock);
void omp_set_nest_lock(omp_nest_lock_t *lock);
void omp_unset_nest_lock(omp_nest_lock_t *lock);
int omp_test_nest_lock(omp_nest_lock_t *lock);

/* If the current task (thread if implicit task or explicit task)
*     is not the owner of the lock */
static inline int
mpcomp_nest_lock_test_task(mpcomp_thread_t *thread,
                           mpcomp_nest_lock_t *mpcomp_user_nest_lock) {
#if MPCOMP_TASK
  struct mpcomp_task_s *current_task =
      MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
  sctk_assert(current_task);
  const bool is_task_owner =
      (mpcomp_user_nest_lock->owner_task == current_task);
  const bool is_thread_owned =
      (mpcomp_user_nest_lock->owner_thread == (void *)thread);
  const bool have_task_owner = (mpcomp_user_nest_lock->owner_task != NULL);
  return !(is_task_owner && (is_thread_owned || have_task_owner));
#endif /* MPCOMP_TASK */
}

#endif /* __MPCOMP_LOCK_H__ */
