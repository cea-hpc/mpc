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
#include <mpcomp.h>
#include "mpcomp_internal.h"
#include "mpcmicrothread_internal.h"
#include <sctk_debug.h>

/* lock functions */
void
mpcomp_init_lock (mpcomp_lock_t * lock)
{
  mpcomp_lock_t init = MPCOMP_LOCK_INIT;
  *lock = init;
}

void
mpcomp_destroy_lock (mpcomp_lock_t * lock)
{
  sctk_assert (lock->status == 0);
}



void
mpcomp_set_lock (mpcomp_lock_t * lock)
{
 sctk_spinlock_lock(&lock->lock);

 if (lock->status == 0)
 {
      lock->status = 1;
      sctk_spinlock_unlock(&lock->lock);
 }
 else 
 {
   mpcomp_slot_t slot;
   mpcomp_thread_t *t;
   mpcomp_mvp_t *mvp;
   t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

   slot.t = t;
   slot.next = NULL;

   if (lock->first == NULL)
   {
    lock->first = &slot;
   }
   else
   {
     lock->last->next = &slot;
   }
    
   lock->last = &slot;
   t->is_running = 0;	/* TODO macros */
  
   sctk_spinlock_unlock(&lock->lock); 

   sctk_thread_wait_for_value_and_poll((int *)&(t->is_running), 1 , NULL , NULL);
   sctk_thread_yield();
  }

}

void
mpcomp_unset_lock (mpcomp_lock_t * lock)
{

  sctk_spinlock_lock(&lock->lock);
  if (lock->first == NULL)
  {
   lock->status = 0;
  }
  else
  {
    mpcomp_slot_t *slot;
    mpcomp_thread_t *t;

    slot = (mpcomp_slot_t *) lock->first;
    
    lock->first = slot->next;
      if (lock->first == NULL)
	{
	  lock->last = NULL;
	}
     t = slot->t;

     t->is_running = 1;

  }

  sctk_spinlock_unlock(&lock->lock);
}



int
mpcomp_test_lock (mpcomp_lock_t * lock)
{
/*
  sctk_thread_mutex_lock (&lock->lock);
*/
  not_implemented();
  sctk_spinlock_lock(&lock->lock);
  if (lock->status == 0)
    {
      lock->status = 1;
/*
      sctk_thread_mutex_unlock (&lock->lock);
*/
      sctk_spinlock_unlock(&lock->lock);
      return 1;
    }
  else
    {
/*
      sctk_thread_mutex_unlock (&lock->lock);
*/
      sctk_spinlock_unlock(&lock->lock);
      return 0;
    }
}

/* nestable lock fuctions */
void
mpcomp_init_nest_lock (mpcomp_nest_lock_t * lock)
{
  not_implemented();
  mpcomp_lock_t init = MPCOMP_LOCK_INIT;
  *lock = init;
}

void
mpcomp_destroy_nest_lock (mpcomp_nest_lock_t * lock)
{
  not_implemented();
  sctk_assert (lock->status == 0);
}

void
mpcomp_set_nest_lock (mpcomp_nest_lock_t * lock)
{
  not_implemented();
}

void
mpcomp_unset_nest_lock (mpcomp_nest_lock_t * lock)
{
/*
  sctk_thread_mutex_lock (&lock->lock);
*/
  not_implemented();
}

int
mpcomp_test_nest_lock (mpcomp_nest_lock_t * lock)
{
  not_implemented();
}
