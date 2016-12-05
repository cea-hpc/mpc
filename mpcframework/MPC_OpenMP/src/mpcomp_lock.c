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

#include "mpcomp.h"
#include "sctk_alloc.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "sctk_debug.h"
#include "mpcomp_openmp_tls.h"
#include "mpcomp_lock.h"

/**
 *  \fn void omp_init_lock(omp_lock_t *lock)
 *  \brief The effect of these routines is to initialize the lock to the
 * unlocked state; that is, no task owns the lock. (OpenMP 4.5 p272)
 *  \param omp_lock_t adress of a lock address (\see mpcomp.h)
 */
void omp_init_lock(omp_lock_t *lock) {
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  mpcomp_user_lock = (mpcomp_lock_t *)sctk_malloc(sizeof(mpcomp_lock_t));
  sctk_assert(mpcomp_user_lock);
  memset(mpcomp_user_lock, 0, sizeof(mpcomp_lock_t));
  sctk_thread_mutex_init(&(mpcomp_user_lock->lock), 0);
  sctk_assert(lock);
  *lock = mpcomp_user_lock;
}

/**
 *  \fn void omp_init_lock_with_hint( omp_lock_t *lock, omp_lock_hint_t hint)
 *  \brief These routines initialize an OpenMP lock with a hint. The effect of
 * the hint is implementation-defined. The OpenMP implementation can ignore the
 * hint without changing program semantics.
 */
void omp_init_lock_with_hint(omp_lock_t *lock, omp_lock_hint_t hint) {
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  omp_init_lock(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;
  mpcomp_user_lock->hint = hint;
}

void omp_destroy_lock(omp_lock_t *lock) {
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;
  sctk_thread_mutex_destroy(&(mpcomp_user_lock->lock));
  sctk_free(mpcomp_user_lock);
  *lock = NULL;
}

void omp_set_lock(omp_lock_t *lock) {
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;
  sctk_thread_mutex_lock(&(mpcomp_user_lock->lock));
}

void omp_unset_lock(omp_lock_t *lock) {
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;
  sctk_thread_mutex_unlock(&(mpcomp_user_lock->lock));
}

int omp_test_lock(omp_lock_t *lock) {
  mpcomp_lock_t *mpcomp_user_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_lock = (mpcomp_lock_t *)*lock;
  return !sctk_thread_mutex_trylock(&(mpcomp_user_lock->lock));
}

/**
 *  \fn void omp_init_nest_lock(omp_lock_t *lock)
 *  \brief The effect of these routines is to initialize the lock to the
 * unlocked state; that is, no task owns the lock. (OpenMP 4.5 p272)
 * \param omp_lock_t adress of a lock address (\see mpcomp.h)
 */
void omp_init_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
  mpcomp_user_nest_lock =
      (mpcomp_nest_lock_t *)sctk_malloc(sizeof(mpcomp_nest_lock_t));
  sctk_assert(mpcomp_user_nest_lock);
  memset(mpcomp_user_nest_lock, 0, sizeof(mpcomp_nest_lock_t));
  sctk_thread_mutex_init(&(mpcomp_user_nest_lock->lock), 0);
  sctk_assert(lock);
  *lock = mpcomp_user_nest_lock;
}

void omp_init_nest_lock_with_hint(omp_nest_lock_t *lock, omp_lock_hint_t hint) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
  omp_init_nest_lock(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;
  mpcomp_user_nest_lock->hint = hint;
}

void omp_destroy_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;
  sctk_thread_mutex_destroy(&(mpcomp_user_nest_lock->lock));
  sctk_free(mpcomp_user_nest_lock);
  *lock = NULL;
}

void omp_set_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock;
  __mpcomp_init();
  mpcomp_thread_t *thread = mpcomp_get_thread_tls();
  sctk_assert(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;

#if MPCOMP_TASK
  if (mpcomp_nest_lock_test_task(thread, mpcomp_user_nest_lock))
#else
  if (mpcomp_user_nest_lock->owner_thread != (void *)thread)
#endif
  {
    sctk_thread_mutex_lock(&(mpcomp_user_nest_lock->lock));
    mpcomp_user_nest_lock->owner_thread = thread;
#if MPCOMP_TASK
    mpcomp_user_nest_lock->owner_task =
        MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
#endif
  }
  mpcomp_user_nest_lock->nb_nested += 1;
}

void omp_unset_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock = NULL;
  sctk_assert(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;

  mpcomp_user_nest_lock->nb_nested -= 1;

  if (mpcomp_user_nest_lock->nb_nested == 0) {
    mpcomp_user_nest_lock->owner_thread = NULL;
#if MPCOMP_TASK
    mpcomp_user_nest_lock->owner_task = NULL;
#endif
    sctk_thread_mutex_unlock(&(mpcomp_user_nest_lock->lock));
  }
}

int omp_test_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_nest_lock_t *mpcomp_user_nest_lock;
  __mpcomp_init();
  mpcomp_thread_t *thread = mpcomp_get_thread_tls();
  sctk_assert(lock);
  mpcomp_user_nest_lock = (mpcomp_nest_lock_t *)*lock;

#if MPCOMP_TASK
  if (mpcomp_nest_lock_test_task(thread, mpcomp_user_nest_lock))
#else
  if (mpcomp_user_nest_lock->owner_thread != (void *)t)
#endif
  {
    if (sctk_thread_mutex_trylock(&(mpcomp_user_nest_lock->lock))) {
      return 0;
    }
    mpcomp_user_nest_lock->owner_thread = (void *)thread;
#if MPCOMP_TASK
    mpcomp_user_nest_lock->owner_task =
        MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
#endif
  }
  mpcomp_user_nest_lock->nb_nested += 1;
  return mpcomp_user_nest_lock->nb_nested;
}
