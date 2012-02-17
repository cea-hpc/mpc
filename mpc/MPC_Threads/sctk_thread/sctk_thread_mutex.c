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
/* #                                                                      # */
/* ######################################################################## */

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_mutex.h"
#include "sctk_thread_scheduler.h"
#include "sctk_spinlock.h"

int
sctk_thread_generic_mutexes_mutex_init (sctk_thread_generic_mutex_t * lock,
					const  sctk_thread_generic_mutexattr_t* attr,
					sctk_thread_generic_scheduler_t* sched)
{
  sctk_thread_generic_mutex_t local_lock = SCTK_THREAD_GENERIC_MUTEX_INIT;
  if(attr != NULL){
    not_implemented();
  }
  *lock = local_lock;
  return 0;
}

int
sctk_thread_generic_mutexes_mutex_lock (sctk_thread_generic_mutex_t * lock,
					sctk_thread_generic_scheduler_t* sched)
{
  int ret = 0;
  sctk_thread_generic_mutex_cell_t cell;
  sctk_spinlock_lock(&(lock->lock));
  if(lock->owner == NULL){
    lock->owner = sched;
    sctk_spinlock_unlock(&(lock->lock));
    return ret;
  } else {
    if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE){
      lock->nb_call ++;
      sctk_spinlock_unlock(&(lock->lock));
      return ret;
    } 
    if (lock->type == SCTK_THREAD_MUTEX_ERRORCHECK){
      if(lock->owner == sched){
	ret = SCTK_EDEADLK;
	sctk_spinlock_unlock(&(lock->lock));
	return ret;
      }
    }

    cell.sched = sched;
    DL_APPEND(lock->blocked,&cell);
    
    sctk_thread_generic_thread_status(sched,sctk_thread_generic_blocked);
    sctk_debug("WAIT MUTEX LOCK sleep %p",sched);
    sctk_thread_generic_register_spinlock_unlock(sched,&(lock->lock));
    sctk_thread_generic_sched_yield(sched);
  }
  return ret;
}

int
sctk_thread_generic_mutexes_mutex_trylock (sctk_thread_generic_mutex_t * lock,
					   sctk_thread_generic_scheduler_t* sched)
{
  int ret = 0;
  sctk_spinlock_lock(&(lock->lock));
  if(lock->owner == NULL){
    lock->owner = sched;
    sctk_spinlock_unlock(&(lock->lock));
    return ret;
  } else {
    if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE){
      lock->nb_call ++;
      sctk_spinlock_unlock(&(lock->lock));
      return ret;
    } 
    if (lock->type == SCTK_THREAD_MUTEX_ERRORCHECK){
      if(lock->owner == sched){
	ret = SCTK_EDEADLK;
	sctk_spinlock_unlock(&(lock->lock));
	return ret;
      }
    }
    ret = SCTK_EBUSY;
    sctk_spinlock_unlock(&(lock->lock));
  }
  return ret;
}

int
sctk_thread_generic_mutexes_mutex_spinlock (sctk_thread_generic_mutex_t * lock,
					    sctk_thread_generic_scheduler_t* sched)
{
  int ret = 0;
  sctk_thread_generic_mutex_cell_t cell;

  sctk_spinlock_lock(&(lock->lock));
  if(lock->owner == NULL){
    lock->owner = sched;
    lock->nb_call = 1;
    sctk_spinlock_unlock(&(lock->lock));
    return ret;
  } else {
    if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE){
      lock->nb_call ++;
      sctk_spinlock_unlock(&(lock->lock));
      return ret;
    } 
    if (lock->type == SCTK_THREAD_MUTEX_ERRORCHECK){
      if(lock->owner == sched){
	ret = SCTK_EDEADLK;
	sctk_spinlock_unlock(&(lock->lock));
	return ret;
      }
    }

    cell.sched = sched;
    DL_APPEND(lock->blocked,&cell);
    
    sctk_spinlock_unlock(&(lock->lock));
    do{
      sctk_thread_generic_sched_yield(sched);
    }while(lock->owner != sched);
  }
  return ret;
}

int
sctk_thread_generic_mutexes_mutex_unlock (sctk_thread_generic_mutex_t * lock,
					  sctk_thread_generic_scheduler_t* sched)
{
  if (lock->owner != sched)
    {
      return SCTK_EPERM;
    }
  if (lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
    {
      if (lock->nb_call > 1)
	{
	  lock->nb_call --;
	  return 0;
	}
    }

  sctk_spinlock_lock(&(lock->lock));
  {
    sctk_thread_generic_mutex_cell_t * head;
    head = lock->blocked;
    if(head == NULL){
      lock->owner = NULL;
      lock->nb_call = 0;
    } else {
      lock->owner = head->sched;
      lock->nb_call = 1;
      DL_DELETE(lock->blocked,head);
      if(head->sched->status != sctk_thread_generic_running){
	sctk_debug("ADD MUTEX UNLOCK wake %p",head->sched);
	sctk_thread_generic_wake(head->sched);
      }
    }
  }
  sctk_spinlock_unlock(&(lock->lock));
  return 0;
}

void sctk_thread_generic_mutexes_init(){ 
  sctk_thread_generic_check_size (sctk_thread_generic_mutex_t, sctk_thread_mutex_t);
  sctk_thread_generic_check_size (sctk_thread_generic_mutexattr_t, sctk_thread_mutexattr_t);

  {
    static sctk_thread_generic_mutex_t loc = SCTK_THREAD_GENERIC_MUTEX_INIT;
    static sctk_thread_mutex_t glob = SCTK_THREAD_MUTEX_INITIALIZER;
    assume (memcmp (&loc, &glob, sizeof (sctk_thread_generic_mutex_t)) == 0);
  }
 
}
