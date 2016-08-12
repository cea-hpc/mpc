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

#include "mpcomp_internal.h"

/* REGULAR LOCKS */
 
void 
omp_init_lock(omp_lock_t *lock)
{
  *lock = sctk_malloc(sizeof(sctk_thread_mutex_t));
  sctk_thread_mutex_init(*lock, NULL);
}

void 
omp_destroy_lock(omp_lock_t *lock)
{
  sctk_thread_mutex_destroy(*lock);
  sctk_free(*lock);
}

void omp_set_lock(omp_lock_t *lock) { sctk_thread_mutex_lock(*lock); }

void omp_unset_lock(omp_lock_t *lock) { sctk_thread_mutex_unlock(*lock); }

int 
omp_test_lock(omp_lock_t *lock)
{
  return (sctk_thread_mutex_trylock(*lock) == 0);
}


/* NESTED LOCKS */
void omp_init_nest_lock(omp_nest_lock_t *lock) {
  *lock = sctk_malloc(sizeof(struct omp_nested_lock_s));
  struct omp_nested_lock_s *llock = *lock;
  llock->owner_thread = NULL;
#if MPCOMP_TASK
  llock->owner_task = NULL;
#endif
  llock->nb_nested = 0;
  sctk_thread_mutex_init(&(llock->l), NULL);
}

void omp_destroy_nest_lock(omp_nest_lock_t *lock) {
  sctk_thread_mutex_destroy(&((*lock)->l));
  sctk_free(*lock);
}

void omp_set_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;
  sctk_assert(t->current_task != NULL);

  struct omp_nested_lock_s *llock = *lock;

/* If the current task (thread if implicit task or explicit task)
*     is not the owner of the lock */
#if MPCOMP_TASK
  if (!(((llock->owner_task == t->current_task) &&
         ((llock->owner_thread == (void *)t) || (llock->owner_task != NULL)))))
#else
  if (llock->owner_thread != (void *)t)
#endif
  {
    sctk_thread_mutex_lock(&(llock->l));
    llock->owner_thread = t;
#if MPCOMP_TASK
    llock->owner_task = t->current_task;
#endif
  }
  llock->nb_nested++;
}

void omp_unset_nest_lock(omp_nest_lock_t *lock) {
  struct omp_nested_lock_s *llock = *lock;
  llock->nb_nested--;

  if (llock->nb_nested == 0) {
    llock->owner_thread = NULL;
#if MPCOMP_TASK
    llock->owner_task = NULL;
#endif
    sctk_thread_mutex_unlock(&(llock->l));
    }
}

int omp_test_nest_lock(omp_nest_lock_t *lock) {
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;
  sctk_assert(t->current_task != NULL);

  struct omp_nested_lock_s *llock = *lock;
#if MPCOMP_TASK
            t->current_task 
#else /* MPCOMP_TASK */
    NULL
#endif /* MPCOMP_TASK */
) ;

  sctk_nodebug("omp_test_nest_lock: TEST owner=(%p,%p), thread=(%p,%p)\n",
               llock->owner_thread, llock->owner_task, t, t->current_task);
#if MPCOMP_TASK
  if (!(((llock->owner_task == t->current_task) &&
         ((llock->owner_thread == (void *)t) || (llock->owner_task != NULL)))))
#else
  if (llock->owner_thread != (void *)t)
#endif
  {
    if (sctk_thread_mutex_trylock(&(llock->l)) != 0) {
      return 0;
      }
      llock->owner_thread = (void *)t;
#if MPCOMP_TASK
      llock->owner_task = t->current_task;
#endif
  }
  llock->nb_nested++;

  sctk_nodebug("omp_test_nest_lock: SUCCESS owner=(%p,%p), thread=(%p,%p)\n",
               llock->owner_thread, llock->owner_task, t, t->current_task);
#if MPCOMP_TASK
          t->current_task
#else /* MPCOMP_TASK */
        NULL
#endif /* MPCOMP_TASK */
 ) ;

  return llock->nb_nested;
}
