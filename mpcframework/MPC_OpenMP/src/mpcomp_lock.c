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

#include <mpcomp.h>
 
void mpcomp_init_lock(mpcomp_lock_t *lock)
{
  sctk_thread_mutex_init(lock,NULL);
}

void mpcomp_init_nest_lock(mpcomp_nest_lock_t *lock)
{

 sctk_thread_mutexattr_t attr; 
 sctk_thread_mutexattr_init(&attr);
 sctk_thread_mutexattr_settype(&attr,SCTK_THREAD_MUTEX_RECURSIVE);
 sctk_thread_mutex_init(lock,&attr);
 sctk_thread_mutexattr_destroy(&attr);
}

void mpcomp_destroy_lock(mpcomp_lock_t *lock)
{
  sctk_thread_mutex_destroy(lock);
}

void mpcomp_destroy_nest_lock(mpcomp_nest_lock_t *lock) 
{

 sctk_thread_mutex_destroy(lock);

}

void mpcomp_set_lock(mpcomp_lock_t *lock)
{
 sctk_thread_mutex_lock(lock);
}

void mpcomp_set_nest_lock(mpcomp_nest_lock_t *lock)
{
 sctk_thread_mutex_lock(lock);
}

void mpcomp_unset_lock(mpcomp_lock_t *lock)
{
 sctk_thread_mutex_unlock(lock);
}

void mpcomp_unset_nest_lock(mpcomp_nest_lock_t *lock)
{ 
 sctk_thread_mutex_unlock(lock);
}

int mpcomp_test_lock(mpcomp_lock_t *lock)
{
 return (sctk_thread_mutex_trylock(lock) == 0);
}

int mpcomp_test_nest_lock(mpcomp_nest_lock_t *lock)
{
 return (sctk_thread_mutex_trylock(lock) == 0);
}
