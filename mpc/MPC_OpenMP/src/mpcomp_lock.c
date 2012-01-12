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
//#include "mpcomp_internal.h"
#include "mpcomp_structures.h"
#include "mpcmicrothread_internal.h"
#include <sctk_debug.h>

/* lock functions */
void
mpcomp_init_lock (mpcomp_lock_t * lock)
{
  //not_implemented();
  mpcomp_lock_t init = MPCOMP_LOCK_INIT;
  *lock = init;
}

void
mpcomp_destroy_lock (mpcomp_lock_t * lock)
{
  //not_implemented();
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

   //mvp = t->mvp;
   //printf("[mpcomp_set_lock] t address=%p..\n", &t);
   //assert (mvp != NULL);

   slot.t = t;
   slot.next = NULL;

   if (lock->first == NULL)
   {
    printf("[mpcomp_set_lock] lock first NULL..\n");
    lock->first = &slot;
   }
   else
   {
     //printf("[mpcomp_set_lock] lock first not NULL..\n");     
     lock->last->next = &slot;
   }
    
   lock->last = &slot;
   t->is_running = 0;	/* TODO macros */
   //mvp->slave_running[0] = 0;
  
   sctk_spinlock_unlock(&lock->lock); 

   sctk_thread_wait_for_value_and_poll((int *)&(t->is_running), 1 , NULL , NULL);
   sctk_thread_yield();
   /*
   while(mvp->slave_running[0] == 0) 
   {
     sctk_thread_yield();
   }
   */
 }

}


#if 0
void
mpcomp_set_lock (mpcomp_lock_t * lock)
{
  sctk_nodebug ("mpcomp_set_lock: Req lock");
  not_implemented();
  sctk_spinlock_lock(&lock->lock);
  if (lock->status == 0)
    {
      lock->status = 1;
      sctk_spinlock_unlock(&lock->lock);
    }
  else
    {
      mpcomp_slot_t slot;
      mpcomp_thread_info_t *info;
      sctk_microthread_vp_t *my_vp;

      sctk_nodebug ("mpcomp_set_lock: Blocked");

      info = sctk_thread_getspecific (mpcomp_thread_info_key);

      /* If the lock is not available and we are currently executing a
         sequential part, this is a deadlock */
#if 0
      if (info->depth == 0)
	{
	  sctk_error
	    ("Deadlock detected: OpenMP lock unavailable inside a sequential part");
	  sctk_abort ();
	}
#endif

      my_vp = &(info->task->__list[info->vp]);

      slot.thread = info;
      slot.next = NULL;

      /* Create contextes only in case of overloading */
      if ( my_vp->to_do_list > 1 ) {
	mpcomp_fork_when_blocked (my_vp, info->step);
      }

      if (lock->first == NULL)
	{
	  lock->first = &slot;
	}
      else
	{
	  lock->last->next = &slot;
	}
      lock->last = &slot;
      info->is_running = 0;	/* TODO macros */

/*
      sctk_thread_mutex_unlock (&lock->lock);
*/
      sctk_spinlock_unlock(&lock->lock);
if ( my_vp->to_do_list > 1 ) {
      mpcomp_macro_scheduler (my_vp, info->step);
      while (info->is_running == 0)	/* TODO macro */
	{
	  mpcomp_macro_scheduler (my_vp, info->step);
	  if (info->is_running == 0)	/* TODO macro */
	    sctk_thread_yield ();
	}
} else {
      while (info->is_running == 0)	
	{
	  sctk_thread_yield ();
	}
    }
}
}
#endif


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
    //mpcomp_mvp_t *mvp;

    slot = (mpcomp_slot_t *) lock->first;
    
    lock->first = slot->next;
      if (lock->first == NULL)
	{
	  lock->last = NULL;
	}
     t = slot->t;
     //mvp = t->mvp;

     //printf("[mpcomp_unset_lock] t address=%p..\n", &t);       

     //mvp->slave_running[0] = 1;  
     t->is_running = 1;

  }

  sctk_spinlock_unlock(&lock->lock);
}


#if 0
void
mpcomp_unset_lock (mpcomp_lock_t * lock)
{
/*
  sctk_thread_mutex_lock (&lock->lock);
*/
  not_implemented();
  sctk_spinlock_lock(&lock->lock);
  if (lock->first == NULL)
    {
      lock->status = 0;
    }
  else
    {
      mpcomp_slot_t *slot;
      mpcomp_thread_info_t *info;
      slot = (mpcomp_slot_t *) lock->first;

      lock->first = slot->next;
      if (lock->first == NULL)
	{
	  lock->last = NULL;
	}
      info = slot->thread;
      info->is_running = 1;
    }
/*
  sctk_thread_mutex_unlock (&lock->lock);
*/
  sctk_spinlock_unlock(&lock->lock);
}
#endif

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
//#endif

//#if 0
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
  #if 0
  not_implemented();
  mpcomp_thread_info_t *info;
  info = sctk_thread_getspecific (mpcomp_thread_info_key);
/*
  sctk_thread_mutex_lock (&lock->lock);
*/
  sctk_spinlock_lock(&lock->lock);
  if (lock->status == 0)
    {
      lock->status = 1;
      lock->owner = info;
/*
      sctk_thread_mutex_unlock (&lock->lock);
*/
      sctk_spinlock_unlock(&lock->lock);
    }
  else
    {
      if (lock->owner == info)
	{
	  lock->iter++;
/*
	  sctk_thread_mutex_unlock (&lock->lock);
*/
	  sctk_spinlock_unlock(&lock->lock);
	}
      else
	{
	  mpcomp_slot_t slot;
	  sctk_microthread_vp_t *my_vp;

	  /* If the lock is not available and we are currently executing a
	     sequential part, then this is a deadlock */
	  if (info->depth == 0)
	    {
	      sctk_error
		("Deadlock detected: OpenMP nested lock unavailable inside a sequential part");
	      sctk_abort ();
	    }

	  my_vp = &(info->task->__list[info->vp]);

	  slot.thread = info;
	  slot.next = NULL;

	  mpcomp_fork_when_blocked (my_vp, info->step);

	  if (lock->first == NULL)
	    {
	      lock->first = &slot;
	    }
	  else
	    {
	      lock->last->next = &slot;
	    }
	  lock->last = &slot;
	  info->is_running = 0;

/*
	  sctk_thread_mutex_unlock (&lock->lock);
*/
	  sctk_spinlock_unlock(&lock->lock);
	  mpcomp_macro_scheduler (my_vp, info->step);
	  while (info->is_running == 0)
	    {
	      mpcomp_macro_scheduler (my_vp, info->step);
	      if (info->is_running == 0)
		sctk_thread_yield ();
	    }

	}
    }
#endif
}

void
mpcomp_unset_nest_lock (mpcomp_nest_lock_t * lock)
{
/*
  sctk_thread_mutex_lock (&lock->lock);
*/
 #if 0
  not_implemented();
  sctk_spinlock_lock(&lock->lock);
  if ((lock->first == NULL) && (lock->iter == 0))
    {
      lock->status = 0;
    }
  else
    {
      if (lock->iter != 0)
	{
	  lock->iter--;
	}
      else
	{
	  mpcomp_slot_t *slot;
	  mpcomp_thread_info_t *info;
	  slot = (mpcomp_slot_t *) lock->first;

	  lock->first = slot->next;
	  if (lock->first == NULL)
	    {
	      lock->last = NULL;
	    }
	  info = slot->thread;
	  lock->owner = info;
	  info->is_running = 1;
	}
    }
/*
  sctk_thread_mutex_unlock (&lock->lock);
*/
  sctk_spinlock_unlock(&lock->lock);
#endif
}

int
mpcomp_test_nest_lock (mpcomp_nest_lock_t * lock)
{
#if 0
  not_implemented();
  mpcomp_thread_info_t *info;
  info = sctk_thread_getspecific (mpcomp_thread_info_key);
/*
  sctk_thread_mutex_lock (&lock->lock);
*/
  sctk_spinlock_lock(&lock->lock);
  if (lock->status == 0)
    {
      lock->status = 1;
      lock->owner = info;
/*
      sctk_thread_mutex_unlock (&lock->lock);
*/
      sctk_spinlock_unlock(&lock->lock);
      return 1;
    }
  else
    {
      if (lock->owner == info)
	{
	  lock->iter++;
/*
	  sctk_thread_mutex_unlock (&lock->lock);
*/
	  sctk_spinlock_unlock(&lock->lock);
	  return 1;
	}
/*
      sctk_thread_mutex_unlock (&lock->lock);
*/
      sctk_spinlock_unlock(&lock->lock);
      return 0;
    }
#endif
}
//#endif
