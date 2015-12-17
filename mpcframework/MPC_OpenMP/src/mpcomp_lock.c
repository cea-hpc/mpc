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
  sctk_thread_mutex_init(lock,NULL);
}

void 
omp_destroy_lock(omp_lock_t *lock)
{
  sctk_thread_mutex_destroy(lock);
}

void 
omp_set_lock(omp_lock_t *lock)
{
 sctk_thread_mutex_lock(lock);
}

void 
omp_unset_lock(omp_lock_t *lock)
{
 sctk_thread_mutex_unlock(lock);
}

int 
omp_test_lock(omp_lock_t *lock)
{
 return (sctk_thread_mutex_trylock(lock) == 0);
}


/* NESTED LOCKS */

void 
omp_init_nest_lock(omp_nest_lock_t *lock)
{
 lock->owner_thread = NULL ;
#if MPCOMP_TASK
 lock->owner_task = NULL ;
#endif
 lock->nb_nested = 0 ;
 sctk_thread_mutex_init(&(lock->l),NULL);
}

void 
omp_destroy_nest_lock(omp_nest_lock_t *lock) 
{
 sctk_thread_mutex_destroy(&(lock->l));
}

void 
omp_set_nest_lock(omp_nest_lock_t *lock)
{
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  /* If the current task (thread if implicit task or explicit task)
     is not the owner of the lock */
#if MPCOMP_TASK
  if (
          !(
              (
               (lock->owner_task == t->current_task)
               &&
               (
                (lock->owner_thread == (void *)t)
                ||
                (lock->owner_task!=NULL)
               )
              )
           )
     )
#else
  if ( lock->owner_thread != (void *)t )
#endif
  {
      sctk_thread_mutex_lock(&(lock->l));
      lock->owner_thread = t ;
#if MPCOMP_TASK
      lock->owner_task = t->current_task ;
#endif
  }
  lock->nb_nested++ ;
}

void 
omp_unset_nest_lock(omp_nest_lock_t *lock)
{ 
    lock->nb_nested-- ;

    if ( lock->nb_nested == 0 ) {
        lock->owner_thread = NULL ;
#if MPCOMP_TASK
      lock->owner_task = NULL ;
#endif
        sctk_thread_mutex_unlock(&(lock->l));
    }
}

int 
omp_test_nest_lock(omp_nest_lock_t *lock)
{
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  sctk_nodebug( "omp_test_nest_lock: TEST owner=(%p,%p), thread=(%p,%p)\n", 
          lock->owner_thread, lock->owner_task,
          t, t->current_task ) ;

#if MPCOMP_TASK
  if (
          !(
              (
               (lock->owner_task == t->current_task)
               &&
               (
                (lock->owner_thread == (void *)t)
                ||
                (lock->owner_task!=NULL)
               )
              )
           )
     )
#else
  if ( lock->owner_thread != (void *)t )
#endif
  {
      if ( sctk_thread_mutex_trylock(&(lock->l)) != 0 ) 
      {
          return 0 ;
      }
      lock->owner_thread = (void *)t ;
#if MPCOMP_TASK
      lock->owner_task = t->current_task ;
#endif
  }
  lock->nb_nested++ ;

  sctk_nodebug( "omp_test_nest_lock: SUCCESS owner=(%p,%p), thread=(%p,%p)\n", 
          lock->owner_thread, lock->owner_task,
          t, t->current_task ) ;

  return lock->nb_nested ; 
}
