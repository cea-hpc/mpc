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
#ifndef __SCTK_POSIX_ETHREAD_INTERNAL_H_
#define __SCTK_POSIX_ETHREAD_INTERNAL_H_
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_ethread_internal.h"
#include "sctk_spinlock.h"
#include "sctk_asm.h"
#include "sctk_pthread_compatible_structures.h"
#include "sctk_ethread.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define sctk_ethread_check_attr(attr) do{if(attr == NULL){return SCTK_EINVAL;}if(attr->ptr == NULL){return SCTK_EINVAL;}}while(0)

  /**pthread_attr*/
  static inline
    int __sctk_ethread_attr_destroy (sctk_ethread_attr_intern_t * attr)
  {
    sctk_free (attr);
    return 0;
  }
  /*les différents attributs */ static inline
  int __sctk_ethread_attr_setdetachstate (sctk_ethread_attr_t * attr,
					  int detachstate)
  {
    /*attributs posix
     *SCTK_THREAD_CREATE_JOINABLE 
     *SCTK_THREAD_CREATE_DETACHED
     */
    sctk_ethread_check_attr (attr);

    if (detachstate != SCTK_THREAD_CREATE_JOINABLE
	&& detachstate != SCTK_THREAD_CREATE_DETACHED)
      return SCTK_EINVAL;
    else
      {
	(attr->ptr)->detached = detachstate;
      }
    return 0;
  }

  static inline
    int __sctk_ethread_attr_getdetachstate (const sctk_ethread_attr_t *
					    attr, int *detachstate)
  {
    sctk_ethread_check_attr (attr);
    *detachstate = attr->ptr->detached;
    return 0;
  }
  static inline
    int __sctk_ethread_attr_setschedpolicy (sctk_ethread_attr_t * attr,
					    int policy)
  {
    /*attributs posix
     *SCTK_SCHED_OTHER (normal)
     *SCTK_SCHED_RR (real time, round robin)
     *SCTK_SCHED_FIFO (realtime, first-in first-out)
     */
    sctk_ethread_check_attr (attr);
    if (policy == SCTK_SCHED_FIFO || policy == SCTK_SCHED_RR)
      return SCTK_ENOTSUP;
    else if (policy != SCTK_SCHED_OTHER)
      return SCTK_EINVAL;
    else
      attr->ptr->schedpolicy = policy;
    return 0;
  }

  static inline
    int __sctk_ethread_attr_getschedpolicy (const sctk_ethread_attr_t *
					    attr, int *policy)
  {
    sctk_ethread_check_attr (attr);
    *policy = attr->ptr->schedpolicy;
    return 0;
  }
  static inline
    int __sctk_ethread_attr_setinheritsched (sctk_ethread_attr_t * attr,
					     int inherit)
  {
    /*attributs posix
     *SCTK_THREAD_EXPLICIT_SCHED
     *SCTK_THREAD_INHERIT_SCHED
     */
    sctk_ethread_check_attr (attr);
    if (inherit != SCTK_THREAD_INHERIT_SCHED
	&& inherit != SCTK_THREAD_EXPLICIT_SCHED)
      return SCTK_EINVAL;
    else
      attr->ptr->inheritsched = inherit;
    return 0;
  }

  static inline
    int __sctk_ethread_attr_getinheritsched (const sctk_ethread_attr_t *
					     attr, int *inherit)
  {
    sctk_ethread_check_attr (attr);
    *inherit = attr->ptr->inheritsched;
    return 0;
  }
  static inline
    int __sctk_ethread_attr_setstackaddr (sctk_ethread_attr_t * attr,
					  void *stackaddr)
  {
    sctk_ethread_check_attr (attr);
    if (attr == NULL)
      return SCTK_ESRCH;	/*la norme ne précise pas le signal a envoyer */
    sctk_ethread_check_attr (attr);
    attr->ptr->stack = (char *) stackaddr;
    return 0;
  }

  static inline
    int __sctk_ethread_attr_getstackaddr (const sctk_ethread_attr_t *
					  attr, void **stackaddr)
  {
    if (attr == NULL || stackaddr == NULL)
      return SCTK_EINVAL;
    sctk_ethread_check_attr (attr);
    *stackaddr = (void *) attr->ptr->stack;
    return 0;
  }
  static inline
    int __sctk_ethread_attr_setstacksize (sctk_ethread_attr_t * attr,
					  size_t stacksize)
  {
    if (stacksize < SCTK_THREAD_STACK_MIN)
      return SCTK_EINVAL;

    else if (attr == NULL)
      return SCTK_ESRCH;
    sctk_ethread_check_attr (attr);
    attr->ptr->stack_size = (int) stacksize;
    return 0;
  }
  static inline
    int __sctk_ethread_attr_getstacksize (const sctk_ethread_attr_t *
					  attr, size_t * stacksize)
  {
    if (attr == NULL || stacksize == NULL)
      return SCTK_ESRCH;
    else
      {
	sctk_ethread_check_attr (attr);
	*stacksize = attr->ptr->stack_size;
      }
    return 0;
  }


  static inline int __sctk_ethread_attr_getstack (const
						  sctk_ethread_attr_t *
						  attr, void **stackaddr,
						  size_t * stacksize)
  {
    sctk_ethread_check_attr (attr);
    if (stackaddr != NULL)
      *stackaddr = (void *) attr->ptr->stack;
    if (stacksize != NULL)
      *stacksize = attr->ptr->stack_size;
    return 0;
  }
  static inline int __sctk_ethread_attr_setstack (sctk_ethread_attr_t *
						  attr, void *stackaddr,
						  size_t stacksize)
  {
    sctk_ethread_check_attr (attr);
    if (attr == NULL || stacksize < SCTK_THREAD_STACK_MIN)	/*on effectue pas  le test d'alignement */
      return SCTK_EINVAL;
    attr->ptr->stack_size = (int) stacksize;
    attr->ptr->stack = (char *) stackaddr;
    return 0;
  }

  static inline
    int __sctk_ethread_attr_getscope (const sctk_ethread_attr_t * attr,
				      int *scope)
  {
    sctk_ethread_check_attr (attr);
    *scope = attr->ptr->scope;
    return 0;
  }

  static inline int
    __sctk_ethread_attr_setscope2 (sctk_ethread_attr_t * attr, int scope)
  {
    sctk_ethread_check_attr (attr);
    if (attr == NULL)
      return SCTK_EINVAL;
    if (scope != SCTK_ETHREAD_SCOPE_SYSTEM
	&& scope != SCTK_ETHREAD_SCOPE_PROCESS)
      return SCTK_EINVAL;
/*
    if (scope == SCTK_ETHREAD_SCOPE_SYSTEM)
      return SCTK_ENOTSUP;
*/
    attr->ptr->scope = scope;
    return 0;
  }
  /*la gestion du cancel */
  static inline int __sctk_ethread_cancel (sctk_ethread_per_thread_t * target)
  {
    sctk_nodebug ("target vaux : %d", target);
    if (target == NULL)
      return SCTK_ESRCH;
    if (target->status == ethread_joined)
      return SCTK_ESRCH;

    if (target->cancel_state != SCTK_THREAD_CANCEL_DISABLE)
      target->cancel_status = 1;
    return 0;
  }

  static inline
    int __sctk_ethread_setcancelstate (sctk_ethread_per_thread_t *
				       owner, int state, int *oldstate)
  {
    if (oldstate != NULL)
      {
	*oldstate = owner->cancel_state;
      }
    if (state != SCTK_THREAD_CANCEL_DISABLE
	&& state != SCTK_THREAD_CANCEL_ENABLE)
      {
	return SCTK_EINVAL;
      }
    else
      {
	owner->cancel_state = (char) state;
      }
    return 0;
  }

  static inline
    int __sctk_ethread_setcanceltype (sctk_ethread_per_thread_t * owner,
				      int type, int *oldtype)
  {
    sctk_nodebug ("%d %d", type, owner->cancel_type);
    if (oldtype != NULL)
      {
	*oldtype = owner->cancel_type;
      }
    if (type != SCTK_THREAD_CANCEL_DEFERRED
	&& type != SCTK_THREAD_CANCEL_ASYNCHRONOUS)
      {
	sctk_nodebug ("%d %d %d", type != SCTK_THREAD_CANCEL_DEFERRED,
		      type != SCTK_THREAD_CANCEL_ASYNCHRONOUS, type);
	return SCTK_EINVAL;
      }
    else
      {
	owner->cancel_type = (char) type;
      }
    return 0;
  }



  /**pthread_once**/
  typedef sctk_spinlock_t sctk_ethread_once_t;

  static inline int __sctk_ethread_once_initialized (sctk_ethread_once_t *
						     once_control)
  {
#ifdef sctk_thread_once_t_is_contiguous_int
    return (*((sctk_thread_once_t *) once_control) == SCTK_THREAD_ONCE_INIT);
#else
    sctk_thread_once_t once_init = SCTK_THREAD_ONCE_INIT;
    return (memcpy
	    ((void *) &once_init, (void *) once_control,
	     sizeof (sctk_thread_once_t)) == 0);
#endif
  }

  static inline
    int __sctk_ethread_once (sctk_ethread_once_t * once_control,
			     void (*init_routine) (void))
  {
    static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

    if (__sctk_ethread_once_initialized (once_control))
      {
	sctk_thread_mutex_lock (&lock);
	if (__sctk_ethread_once_initialized (once_control))
	  {
#ifdef MPC_Allocator
#ifdef SCTK_USE_TLS
	    sctk_add_global_var ((void *) once_control,
				 sizeof (sctk_ethread_once_t));
#else
#warning "Once backup disabled"
#endif
#endif
	    init_routine ();
#ifdef sctk_thread_once_t_is_contiguous_int
	    *once_control = !SCTK_THREAD_ONCE_INIT;
#else
	    once_control[0] = 1;
#endif
	  }
	sctk_thread_mutex_unlock (&lock);
      }
    return 0;
  }


  /**conditions**/

  static inline int __sctk_ethread_cond_init (sctk_ethread_cond_t * cond,
					      const sctk_ethread_condattr_t *
					      attr)
  {

    if (cond == NULL)
      return SCTK_EINVAL;
    if (attr != NULL)
      {
	if (attr->pshared == SCTK_THREAD_PROCESS_SHARED)
	  {
	    fprintf (stderr, "Invalid pshared value in attr\n");
	    abort ();
	  }
      }
    cond->is_init = 0;
    sctk_spinlock_init (&(cond->lock), 0);
    cond->list = NULL;
    cond->list_tail = NULL;
    return 0;
  }

  static inline int __sctk_etheread_cond_destroy (sctk_ethread_cond_t * cond)
  {
    sctk_nodebug (" %p %p %d\n", cond->list, cond->list_tail, cond->is_init);
    if (cond->is_init != 0)
      return SCTK_EINVAL;
    if (cond->list != NULL && cond->list_tail != NULL)
      return SCTK_EBUSY;
    return 0;
  }

  /*
     on ne gère pas encore la condition comme point de cancel
   */
  static inline int __sctk_ethread_cond_wait (sctk_ethread_cond_t * cond,
					      sctk_ethread_virtual_processor_t
					      * vp,
					      sctk_ethread_per_thread_t *
					      owner, void (*retrun_task)
					      (sctk_ethread_per_thread_t
					       * task),
					      sctk_ethread_mutex_t * mutex)
  {
    sctk_ethread_mutex_cell_t cell;
    int ret;
    __sctk_ethread_testcancel (owner);
    sctk_spinlock_lock (&(cond->lock));
    if (cond == NULL || mutex == NULL)
      return SCTK_EINVAL;
    cell.my_self = owner;
    cell.next = NULL;
    cell.wake = 0;
    if (cond->list == NULL)
      {
	cond->list = &cell;
	cond->list_tail = &cell;
      }
    else
      {
	cond->list_tail->next = &cell;
	cond->list_tail = &cell;
      }
/*    sctk_nodebug("%p blocked on %p", owner, lock);*/
    if (owner->status == ethread_ready)
      {
	owner->status = ethread_blocked;
	owner->no_auto_enqueue = 1;
	ret = __sctk_ethread_mutex_unlock (vp, owner, retrun_task, mutex);
	if (ret != 0)
	  {
	    cond->list = cond->list->next;
	    if (cond->list == NULL)
	      {
		cond->list_tail = NULL;
	      }
	    owner->status = ethread_ready;
	    owner->no_auto_enqueue = 0;
	    sctk_spinlock_unlock (&(cond->lock));
	    return SCTK_EPERM;
	  }
	sctk_spinlock_unlock (&(cond->lock));
	__sctk_ethread_sched_yield_vp_poll (vp, owner);
      }
    else
      {
	ret = __sctk_ethread_mutex_unlock (vp, owner, retrun_task, mutex);
	if (ret != 0)
	  {
	    cond->list = cond->list->next;
	    if (cond->list == NULL)
	      {
		cond->list_tail = NULL;
	      }
	    sctk_spinlock_unlock (&(cond->lock));
	    return SCTK_EPERM;
	  }
	sctk_spinlock_unlock (&(cond->lock));
	while (cell.wake != 1)
	  {
	    __sctk_ethread_sched_yield_vp (vp, owner);
	  }
      }
    __sctk_ethread_mutex_lock (vp, owner, mutex);
    return 0;
  }

#if 0
  static inline int __sctk_ethread_cond_timedwait (sctk_ethread_cond_t *
						   cond,
						   sctk_ethread_virtual_processor_t
						   * vp,
						   sctk_ethread_per_thread_t
						   * owner,
						   void (*retrun_task)
						   (sctk_ethread_per_thread_t
						    * task),
						   sctk_ethread_mutex_t *
						   mutex,
						   const struct timespec
						   *abstime)
  {
    sctk_ethread_mutex_cell_t cell;
    struct timeval tv;
    int ret;
    __sctk_ethread_testcancel (owner);
    if (cond == NULL || mutex == NULL)
      return SCTK_EINVAL;
    gettimeofday (&tv, NULL);
    sctk_nodebug ("temps : %d %d  =  %d %d ", tv.tv_sec,
		  tv.tv_usec * 1000, abstime->tv_sec, abstime->tv_nsec);
    if (tv.tv_sec > abstime->tv_sec)
      if (tv.tv_usec * 1000 > (abstime->tv_nsec))
	{
	  sctk_nodebug ("sortie direct");
	  return SCTK_ETIMEDOUT;
	}

    sctk_spinlock_lock (&(cond->lock));
    cell.my_self = owner;
    cell.next = NULL;
    cell.wake = -1;
    if (cond->list == NULL)
      {
	cond->list = &cell;
	cond->list_tail = &cell;
      }
    else
      {
	cond->list_tail->next = &cell;
	cond->list_tail = &cell;
      }
    ret = __sctk_ethread_mutex_unlock (vp, owner, retrun_task, mutex);
    if (ret != 0)
      {
	cond->list = cond->list->next;
	if (cond->list == NULL)
	  {
	    cond->list_tail = NULL;
	  }
	owner->status = ethread_ready;
	owner->no_auto_enqueue = 0;
	sctk_spinlock_unlock (&(cond->lock));
	sctk_nodebug ("sortie eperm");
	return SCTK_EPERM;
      }
    sctk_nodebug ("cond ->list = %p", cond->list);
    sctk_spinlock_unlock (&(cond->lock));
    ret = 0;
    while (cell.wake != 1)
      {
	__sctk_ethread_sched_yield_vp (vp, owner);
	gettimeofday (&tv, NULL);
	if (tv.tv_sec > abstime->tv_sec)
	  if (tv.tv_usec * 1000 > (abstime->tv_nsec))
	    {
	      sctk_ethread_cond_t *ptr;
	      ptr = cond;

	      sctk_spinlock_lock (&(cond->lock));

	      cell.wake = 1;
	      owner->status = ethread_ready;
	      sctk_nodebug ("cond ->list 2 = %p", cond->list);
	      /*
	       *on reparcours toute la liste pour savoir ou on est
	       *on peut etre absent (apres un broadcast qui c'est produit 
	       *entre le début du while et ici  par exemple          
	       */
	      while (ptr->list != NULL)
		{
		  if (ptr->list->next == &cell)
		    {
		      if (cond->list_tail == &cell)
			{
			  cond->list_tail = ptr->list;
			  ptr->list->next = NULL;
			}
		      else
			ptr->list->next = ptr->list->next->next;
		    }
		  ptr->list = ptr->list->next;
		}
	      /*cond->list=cond->list->next; */
	      if (cond->list == NULL)
		cond->list_tail = NULL;

	      ret = SCTK_ETIMEDOUT;
	      sctk_nodebug ("cond ->list 3 = %p", cond->list);
	      sctk_spinlock_unlock (&(cond->lock));
	    }
      }

    sctk_nodebug ("le mutex a été récupéré");
    __sctk_ethread_mutex_lock (vp, owner, mutex);
    return ret;
  }
#else
  static inline int __sctk_ethread_cond_timedwait (sctk_ethread_cond_t *
						   cond,
						   sctk_ethread_virtual_processor_t
						   * vp,
						   sctk_ethread_per_thread_t
						   * owner,
						   void (*retrun_task)
						   (sctk_ethread_per_thread_t
						    * task),
						   sctk_ethread_mutex_t *
						   mutex,
						   const struct timespec
						   *abstime)
  {
    not_implemented ();
    return 0;
  }
#endif



  static inline
    int __sctk_ethread_cond_broadcast (sctk_ethread_cond_t * cond,
				       void (*retrun_task)
				       (sctk_ethread_per_thread_t * task))
  {

    sctk_ethread_per_thread_t *to_wake;
    sctk_ethread_mutex_cell_t *cell, *cell_next;
    if (cond == NULL)
      return SCTK_EINVAL;
    sctk_spinlock_lock (&(cond->lock));
    while (cond->list != NULL)
      {
	cell_next = cond->list->next;
	cell = (sctk_ethread_mutex_cell_t *) cond->list;
	to_wake = (sctk_ethread_per_thread_t *) cell->my_self;
	if (cell->wake == -1)
	  cell->wake = 1;
	else
	  {
	    cell->wake = 1;
	    retrun_task (to_wake);
	  }

	cond->list = cell_next;
      }
    cond->list_tail = NULL;
    sctk_spinlock_unlock (&(cond->lock));
    return 0;
  }
  static inline
    int __sctk_ethread_cond_signal (sctk_ethread_cond_t * cond,
				    void (*retrun_task)
				    (sctk_ethread_per_thread_t * task))
  {

    sctk_ethread_per_thread_t *to_wake;
    sctk_ethread_mutex_cell_t *cell, *cell_next;
    if (cond == NULL)
      return SCTK_EINVAL;
    sctk_spinlock_lock (&(cond->lock));
    if (cond->list != NULL)
      {
	cell_next = cond->list->next;
	cell = (sctk_ethread_mutex_cell_t *) cond->list;
	to_wake = (sctk_ethread_per_thread_t *) cell->my_self;
	if (cell->wake == -1)
	  cell->wake = 1;
	else
	  {
	    cell->wake = 1;
	    retrun_task (to_wake);
	  }
	cond->list = cell_next;
      }
    if (cond->list == NULL)
      cond->list_tail = NULL;
    sctk_spinlock_unlock (&(cond->lock));
    return 0;
  }


  /*les attributs des condtions */
  static inline
    int __sctk_ethread_condattr_destroy (sctk_ethread_condattr_t * attr)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    return 0;
  }
  static inline
    int __sctk_ethread_condattr_init (sctk_ethread_condattr_t * attr)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;
    attr->clock = 0;
    return 0;
  }

  static inline int __sctk_ethread_condattr_getpshared (const
							sctk_ethread_condattr_t
							* attr, int *pshared)
  {
    if (attr == NULL || pshared == NULL)
      return SCTK_EINVAL;
    *pshared = attr->pshared;
    return 0;
  }
  static inline
    int __sctk_ethread_condattr_setpshared (sctk_ethread_condattr_t *
					    attr, int pshared)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    if (pshared != SCTK_THREAD_PROCESS_PRIVATE
	&& pshared != SCTK_THREAD_PROCESS_SHARED)
      return SCTK_EINVAL;
    if (pshared == SCTK_THREAD_PROCESS_SHARED)
      return SCTK_ENOTSUP;
    attr->pshared = (char) pshared;
    return 0;
  }



  /*pthread_detach */
  static inline int __sctk_ethread_detach (sctk_ethread_t th)
  {
    if (th == NULL)
      return SCTK_ESRCH;
    else if (th->status == ethread_joined)
      return SCTK_ESRCH;
    else if (th->attr.detached == SCTK_ETHREAD_CREATE_DETACHED)
      return SCTK_EINVAL;
    else
      return 0;
  }

  /*Semaphore management */
  static inline int __sctk_ethread_sem_init (sctk_ethread_sem_t * lock,
					     int pshared, unsigned int value)
  {
    sctk_ethread_sem_t lock_tmp = STCK_ETHREAD_SEM_INIT;
    lock_tmp.lock = (int) value;
    if (value > SCTK_SEM_VALUE_MAX)
      {
	return SCTK_EINVAL;
      }
    if (pshared)
      {
	return -1;
      }
    *lock = lock_tmp;
    return 0;
  }

  static inline int
    __sctk_ethread_sem_wait (sctk_ethread_virtual_processor_t * vp,
			     sctk_ethread_per_thread_t * owner,
			     sctk_ethread_sem_t * lock)
  {

    sctk_ethread_mutex_cell_t cell;

    sctk_nodebug ("%p lock on %p %d", owner, lock, lock->lock);
    __sctk_ethread_testcancel (owner);
    sctk_spinlock_lock (&lock->spinlock);
    lock->lock--;
    if (lock->lock < 0)
      {
	cell.my_self = owner;
	cell.next = NULL;
	cell.wake = 0;
	if (lock->list == NULL)
	  {
	    lock->list = &cell;
	    lock->list_tail = &cell;
	  }
	else
	  {
	    lock->list_tail->next = &cell;
	    lock->list_tail = &cell;
	  }
	sctk_nodebug ("%p blocked on %p", owner, lock);
	if (owner->status == ethread_ready)
	  {
	    owner->status = ethread_blocked;
	    owner->no_auto_enqueue = 1;
	    sctk_spinlock_unlock (&lock->spinlock);
	    __sctk_ethread_sched_yield_vp_poll (vp, owner);
	    owner->status = ethread_ready;
	  }
	else
	  {
	    while (cell.wake != 1)
	      {
		sctk_spinlock_unlock (&lock->spinlock);
		__sctk_ethread_sched_yield_vp (vp, owner);
		sctk_spinlock_lock (&lock->spinlock);
	      }
	    sctk_spinlock_unlock (&lock->spinlock);
	  }
      }
    else
      {
	/*lock->lock--; effectué au début */
	sctk_spinlock_unlock (&lock->spinlock);
      }

    return 0;
  }

  static inline int __sctk_ethread_sem_trywait (sctk_ethread_sem_t * lock)
  {
/*    sctk_nodebug ("%p lock on %p %d", owner, lock, lock->lock);*/

    if (lock->lock <= 0)
      {
	sctk_nodebug ("try_wait : sem occupé");
	errno = SCTK_EAGAIN;
	return -1;
      }
    sctk_spinlock_lock (&lock->spinlock);

    if (lock->lock <= 0)
      {
	sctk_nodebug ("try_wait : sem occupé");
	sctk_spinlock_unlock (&lock->spinlock);
	errno = SCTK_EAGAIN;
	return -1;
      }
    else
      {
	lock->lock--;
	sctk_nodebug ("try_wait : sem libre");
	sctk_spinlock_unlock (&lock->spinlock);
      }

    return 0;
  }

  static inline int
    __sctk_ethread_sem_post (void (*retrun_task)
			     (sctk_ethread_per_thread_t * task),
			     sctk_ethread_sem_t * lock)
  {
    sctk_ethread_mutex_cell_t *cell;
    sctk_ethread_per_thread_t *to_wake_up;
    sctk_spinlock_lock (&lock->spinlock);
    if (lock->list != NULL)
      {
	sctk_ethread_per_thread_t *to_wake;
	cell = (sctk_ethread_mutex_cell_t *) lock->list;
	to_wake_up = cell->my_self;
	lock->list = lock->list->next;
	if (lock->list == NULL)
	  {
	    lock->list_tail = NULL;
	  }
	to_wake = (sctk_ethread_per_thread_t *) to_wake_up;
	cell->wake = 1;
	retrun_task (to_wake);
      }
    lock->lock++;
    sctk_spinlock_unlock (&lock->spinlock);
/*    sctk_nodebug ("%p unlock on %p %d", owner, lock, lock->lock);*/

    return 0;
  }
  static inline
    int __sctk_ethread_sem_getvalue (sctk_ethread_sem_t * sem, int *sval)
  {
    if (sval != NULL)
      *sval = sem->lock;
    return 0;
  }
  static inline int __sctk_ethread_sem_destroy (sctk_ethread_sem_t * sem)
  {
    if (sem->list != NULL)
      return SCTK_EBUSY;
    else
      return 0;
  }


  typedef struct sctk_ethread_sem_name_s
  {
    char *name;
    volatile int nb;
    volatile int unlink;
    sctk_ethread_sem_t *sem;
    volatile struct sctk_ethread_sem_name_s *next;
  } sctk_ethread_sem_name_t;

  typedef struct
  {
    sctk_spinlock_t spinlock;
    volatile sctk_ethread_sem_name_t *next;
  } sctk_ethread_sem_head_list;
#define SCTK_SEM_HEAD_INITIALIZER {SCTK_SPINLOCK_INITIALIZER,NULL}

  static inline sctk_ethread_sem_t *__sctk_ethread_sem_open (const char
							     *name,
							     int flags,
							     sctk_ethread_sem_head_list
							     * head,
							     mode_t mode,
							     int value)
  {
    int length;
    sctk_ethread_sem_t *sem = NULL;
    volatile sctk_ethread_sem_name_t *tmp, *sem_struct;
    char *_name;
    sem_struct = head->next;
    if (name == NULL)
      {
	errno = SCTK_EINVAL;
	return (sctk_ethread_sem_t *) SCTK_SEM_FAILED;
      }

    length = strlen (name);
    _name = (char *) sctk_malloc ((length + 1) * sizeof (char));
    if (name[0] == '/')
      {
	strcpy (_name, name + 1);
	_name[length - 1] = '\0';
      }
    else
      {
	strcpy (_name, name);
	_name[length] = '\0';
      }
    /*on recherche le sémaphore */
    tmp = sem_struct;
    sctk_spinlock_lock (&head->spinlock);
    while (tmp != NULL)
      {
	if (strcmp (tmp->name, _name) == 0 && tmp->unlink == 0)
	  break;
	else
	  {
	    sem_struct = tmp;
	    tmp = tmp->next;
	  }
      }
    sctk_nodebug
      ("la recherche a trouvé : %p , la création est à  : %d \n",
       tmp, flags & SCTK_O_CREAT);

    if ((tmp != NULL) && ((flags & SCTK_O_CREAT) && (flags & SCTK_O_EXCL)))
      {
	errno = EEXIST;
	sctk_spinlock_unlock (&head->spinlock);
	return (sctk_ethread_sem_t *) SCTK_SEM_FAILED;
      }
    else if (tmp != NULL)
      {
	sctk_free (_name);
	tmp->nb++;
	sctk_spinlock_unlock (&head->spinlock);
	return tmp->sem;
      }
    if (tmp == NULL && (flags & SCTK_O_CREAT))
      {
	sctk_ethread_sem_name_t *maillon;
	int ret;
	sem = (sctk_ethread_sem_t *)
	  sctk_malloc (sizeof (sctk_ethread_sem_t));
	if (sem == NULL)
	  {
	    sctk_free (_name);
	    errno = SCTK_ENOSPC;
	    sctk_spinlock_unlock (&head->spinlock);
	    return (sctk_ethread_sem_t *) SCTK_SEM_FAILED;
	  }
	ret = __sctk_ethread_sem_init (sem, 0, value);
	if (ret != 0)
	  {
	    sctk_free (_name);
	    sctk_free (sem);
	    errno = ret;
	    sctk_spinlock_unlock (&head->spinlock);
	    return (sctk_ethread_sem_t *) SCTK_SEM_FAILED;
	  }
	maillon = (sctk_ethread_sem_name_t *)
	  sctk_malloc (sizeof (sctk_ethread_sem_name_t));
	sctk_nodebug ("%p pointe sur %p avec maillon vaux %p\n",
		      sem_struct, sem_struct, maillon);
	if (maillon == NULL)
	  {
	    sctk_free (_name);
	    sctk_free (sem);
	    errno = SCTK_ENOSPC;
	    sctk_spinlock_unlock (&head->spinlock);
	    return (sctk_ethread_sem_t *) SCTK_SEM_FAILED;
	  }
	if (sem_struct != NULL)
	  {
	    maillon->next = sem_struct->next;
	    sem_struct->next = maillon;
	  }
	else
	  {
	    head->next = maillon;
	  }
	maillon->name = _name;
	maillon->sem = sem;
	maillon->nb = 1;
	maillon->unlink = 0;
	sctk_nodebug ("head %p pointe sur %p avec maillon vaux %p\n",
		      head, head->next, maillon);

      }
    if (tmp == NULL && !(flags & SCTK_O_CREAT))
      {
	sctk_free (_name);
	errno = SCTK_ENOENT;
	sctk_spinlock_unlock (&head->spinlock);
	return (sctk_ethread_sem_t *) SCTK_SEM_FAILED;
      }
    sctk_nodebug ("sem vaux %p avec une valeur de %d", sem, sem->lock);
    sctk_spinlock_unlock (&head->spinlock);
    return sem;
  }

  static inline void __sctk_ethread_sem_clean (volatile
					       sctk_ethread_sem_name_t *
					       sem_struct)
  {
    sctk_free (sem_struct->name);
    sem_struct->name = NULL;
    sctk_free (sem_struct->sem);
    sem_struct->sem = NULL;
  }



  static inline
    int __sctk_ethread_sem_unlink (const char *name,
				   sctk_ethread_sem_head_list * head)
  {
    char *_name;
    volatile sctk_ethread_sem_name_t *tmp, *sem_struct;
    int length;
    sem_struct = head->next;
    if (name == NULL)
      {
	errno = SCTK_ENOENT;
	return -1;
      }

    length = strlen (name);
    _name = (char *) sctk_malloc ((length + 1) * sizeof (char));
    if (name[0] == '/')
      {
	strcpy (_name, name + 1);
	_name[length - 1] = '\0';
      }
    else
      {
	strcpy (_name, name);
	_name[length] = '\0';
      }
    /*on recherche le sémaphore */
    tmp = sem_struct;
    sctk_spinlock_lock (&head->spinlock);
    while (tmp != NULL)
      {
	if (strcmp (tmp->name, _name) == 0 && tmp->unlink == 0)
	  break;
	else
	  {
	    sem_struct = tmp;
	    tmp = tmp->next;
	  }
      }
    if (tmp == NULL)
      {
	sctk_free (_name);
	errno = SCTK_ENOENT;
	sctk_spinlock_unlock (&head->spinlock);
	return -1;
      }
    else if (tmp->nb == 0)
      {				/*tout le monde a déja fermé ce sémaphore, on le détruit */
	sctk_nodebug
	  ("unlink détruit le maillon %p avec le sémaphore %p", tmp,
	   tmp->sem);
	if (head->next == tmp)
	  head->next = tmp->next;
	else
	  sem_struct->next = tmp->next;

	__sctk_ethread_sem_clean (tmp);
	sctk_free ((sctk_ethread_sem_name_t *) tmp);
      }
    else
      {
	tmp->unlink = 1;
      }
    sctk_spinlock_unlock (&head->spinlock);
    sctk_free (_name);
    return 0;
  }

  static inline int __sctk_ethread_sem_close (sctk_ethread_sem_t * sem,
					      sctk_ethread_sem_head_list *
					      head)
  {
    volatile sctk_ethread_sem_name_t *tmp, *sem_struct;
    sem_struct = head->next;
    /*on recherche le sémaphore */
    tmp = sem_struct;
    sctk_spinlock_lock (&head->spinlock);
    while (tmp != NULL && sem != tmp->sem)
      {
	sem_struct = tmp;
	tmp = tmp->next;
      }

    sctk_nodebug ("la recherche a trouvé : %p pour le sémaphore %p\n",
		  tmp, sem);
    if (tmp == NULL)
      {
	errno = SCTK_EINVAL;
	sctk_spinlock_unlock (&head->spinlock);
	return -1;
      }
    tmp->nb--;
    if (tmp->nb == 0 && tmp->unlink != 0)
      {
	sctk_nodebug
	  ("close détruit le maillon %p avec le sémaphore %p", tmp, tmp->sem);
	sctk_nodebug ("sem_struct : %p , tmp : %p ", sem_struct, tmp);
	if (head->next == tmp)
	  head->next = tmp->next;
	else
	  sem_struct->next = tmp->next;

	__sctk_ethread_sem_clean (tmp);
	sctk_free ((sctk_ethread_sem_name_t *) tmp);
      }
    sctk_spinlock_unlock (&head->spinlock);
    return 0;
  }


  /*sched priority 
   *on ne gère que 0 pour l'instant*/
  static inline int __sctk_ethread_sched_get_priority_max (int policy)
  {
    if (policy != SCTK_SCHED_FIFO && policy != SCTK_SCHED_RR
	&& policy != SCTK_SCHED_OTHER)
      return SCTK_EINVAL;
    else
      return 0;
  }
  static inline int __sctk_ethread_sched_get_priority_min (int policy)
  {
    if (policy != SCTK_SCHED_FIFO && policy != SCTK_SCHED_RR
	&& policy != SCTK_SCHED_OTHER)
      return SCTK_EINVAL;
    else
      return 0;
  }



  /**attributs des mutex*/
  static inline
    int __sctk_ethread_mutexattr_init (sctk_ethread_mutexattr_t * attr)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    attr->kind = SCTK_THREAD_MUTEX_DEFAULT;
    return 0;
  }
  static inline
    int __sctk_ethread_mutexattr_destroy (sctk_ethread_mutexattr_t * attr)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    return 0;
  }
  static inline
    int __sctk_ethread_mutexattr_settype (sctk_ethread_mutexattr_t *
					  attr, unsigned short kind)
  {
    if ((attr == NULL)
	|| (kind != SCTK_THREAD_MUTEX_DEFAULT
	    && kind != SCTK_THREAD_MUTEX_RECURSIVE
	    && kind != SCTK_THREAD_MUTEX_ERRORCHECK
	    && kind != SCTK_THREAD_MUTEX_NORMAL))
      return SCTK_EINVAL;
    attr->kind = kind;
    return 0;
  }
  static inline
    int __sctk_ethread_mutexattr_gettype (const sctk_ethread_mutexattr_t
					  * attr, int *kind)
  {
    if (attr == NULL || kind == NULL)
      return SCTK_EINVAL;
    *kind = attr->kind;
    return 0;
  }
  static inline int __sctk_ethread_mutexattr_getpshared (const
							 sctk_ethread_mutexattr_t
							 * attr, int *pshared)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    *pshared = SCTK_THREAD_PROCESS_PRIVATE;
    return 0;
  }
  static inline
    int __sctk_ethread_mutexattr_setpshared (sctk_ethread_mutexattr_t *
					     attr, int pshared)
  {
    if (attr == NULL
	|| (pshared != SCTK_THREAD_PROCESS_SHARED
	    && pshared != SCTK_THREAD_PROCESS_PRIVATE))
      return SCTK_EINVAL;
    if (pshared == SCTK_THREAD_PROCESS_SHARED)
      return SCTK_ENOTSUP;	/*pas dans la norme mais on le supporte pas */
    return 0;
  }

  /*mutex destroy */
  static inline
    int __sctk_ethread_mutex_destroy (sctk_ethread_mutex_t * mutex)
  {
    if (mutex->lock > 0)
      return SCTK_EBUSY;
    return 0;
  }



  /*Spinlock */
  typedef sctk_spinlock_t sctk_ethread_spinlock_t;
  static inline
    int __sctk_ethread_spin_init (sctk_ethread_spinlock_t * lock, int pshared)
  {
    *lock = SCTK_SPINLOCK_INITIALIZER;
    if (pshared == SCTK_THREAD_PROCESS_SHARED)
      return SCTK_ENOTSUP;	/*pas dans la norme mais on le supporte pas */
    return 0;
  }
  static inline
    int __sctk_ethread_spin_destroy (sctk_ethread_spinlock_t * lock)
  {
    if (*lock != 0)
      {
	return SCTK_EBUSY;
      }
    return 0;
  }
  /*on ne supporte pas le EDEADLOCK
   *La norme n'est pas précise à ce sujet
   */
  int sctk_thread_yield (void);
#define SCTK_SPIN_DELAY 10
  static inline int __sctk_ethread_spin_lock (sctk_ethread_spinlock_t * lock)
  {
    /*sctk_spinlock_lock(lock); */
    long i = SCTK_SPIN_DELAY;
    while (sctk_spinlock_trylock (lock))
      {
	while (expect_true (*lock != 0))
	  {
	    i--;
	    if (expect_false (i <= 0))
	      {
		sctk_thread_yield ();
		i = SCTK_SPIN_DELAY;
	      }
	  }
      }
    return 0;
  }
  static inline
    int __sctk_ethread_spin_trylock (sctk_ethread_spinlock_t * lock)
  {
    if (sctk_spinlock_trylock (lock) != 0)
      return SCTK_EBUSY;
    else
      return 0;
  }
  static inline
    int __sctk_ethread_spin_unlock (sctk_ethread_spinlock_t * lock)
  {
    sctk_spinlock_unlock (lock);
    return 0;
  }

  /* RWlock */
  /*on ne supporte pas les attributs dans le sens où on ne supporte pas le fait 
   *qu'on soit sur plusieurs processus*/
  static inline int
    __sctk_ethread_rwlock_init (sctk_ethread_rwlock_t * rwlock,
				const sctk_ethread_rwlockattr_t * attr)
  {
    sctk_ethread_rwlock_t rwlock2 = SCTK_ETHREAD_RWLOCK_INIT;
    if (attr != NULL)
      {
	if (attr->pshared == SCTK_THREAD_PROCESS_SHARED)
	  {
	    fprintf (stderr, "Invalid pshared value in attr\n");
	    abort ();
	  }
      }
    if (rwlock == NULL)
      return SCTK_EINVAL;
    *rwlock = rwlock2;
    return 0;
  }
  static inline
    int __sctk_ethread_rwlock_destroy (sctk_ethread_rwlock_t * rwlock)
  {
    if (rwlock == NULL)
      return SCTK_EINVAL;
    return 0;
  }
  static inline int __sctk_ethread_rwlock_lock (sctk_ethread_rwlock_t *
						rwlock,
						unsigned char type,
						sctk_ethread_virtual_processor_t
						* vp,
						sctk_ethread_per_thread_t
						* owner)
  {
    sctk_ethread_rwlock_cell_t cell;
    sctk_spinlock_lock (&rwlock->spinlock);
    rwlock->lock++;

    sctk_nodebug
      (" rwlock : \nlock = %d\ncurrent = %d\nwait = %d\ntype : %d\n",
       rwlock->lock, rwlock->current, rwlock->wait, type);

    if (type == SCTK_RWLOCK_READ)
      {
	/* reader */
	if ((rwlock->current == SCTK_RWLOCK_READ
	     || rwlock->current == SCTK_RWLOCK_ALONE)
	    && (rwlock->wait == SCTK_RWLOCK_NO_WR_WAIT))
	  {
	    rwlock->current = SCTK_RWLOCK_READ;
	    sctk_spinlock_unlock (&rwlock->spinlock);
	    return 0;
	  }
      }
    else if (type == SCTK_RWLOCK_WRITE)
      {
	/* writer */
	if (rwlock->current == SCTK_RWLOCK_ALONE
	    && rwlock->wait == SCTK_RWLOCK_NO_WR_WAIT)
	  {
	    rwlock->current = SCTK_RWLOCK_WRITE;
	    sctk_spinlock_unlock (&rwlock->spinlock);
	    return 0;
	  }

	rwlock->wait = SCTK_RWLOCK_WR_WAIT;
      }
    else
      {
	not_reachable ();
      }

    cell.my_self = owner;
    cell.next = NULL;
    cell.wake = 0;
    cell.type = type;
    if (rwlock->list == NULL)
      {
	rwlock->list = &cell;
	rwlock->list_tail = &cell;
      }
    else
      {
	rwlock->list_tail->next = &cell;
	rwlock->list_tail = &cell;
      }
    sctk_nodebug ("blocked on %p", rwlock);
    if (owner->status == ethread_ready)
      {
	owner->status = ethread_blocked;
	owner->no_auto_enqueue = 1;
	sctk_spinlock_unlock (&rwlock->spinlock);
	__sctk_ethread_sched_yield_vp_poll (vp, owner);
	owner->status = ethread_ready;
	sctk_spinlock_lock (&rwlock->spinlock);
      }
    else
      {
	while (cell.wake != 1)
	  {
	    sctk_spinlock_unlock (&rwlock->spinlock);
	    __sctk_ethread_sched_yield_vp (vp, owner);
	    sctk_spinlock_lock (&rwlock->spinlock);
	  }
      }

    sctk_spinlock_unlock (&rwlock->spinlock);
    return 0;
  }



  static inline int
    __sctk_ethread_rwlock_unlock (void (*retrun_task)
				  (sctk_ethread_per_thread_t * task),
				  sctk_ethread_rwlock_t * lock)
  {
    sctk_ethread_rwlock_cell_t *cell;
    sctk_ethread_per_thread_t *to_wake_up;
    int first = 1;
    sctk_spinlock_lock (&(lock->spinlock));
    sctk_nodebug ("unlock : %p", lock->list);
    lock->lock--;
    if (lock->list != NULL)
      {
	lock->wait = SCTK_RWLOCK_NO_WR_WAIT;
	/*
	   Wake all readers
	 */
	do
	  {
	    sctk_ethread_per_thread_t *to_wake;
	    cell = (sctk_ethread_rwlock_cell_t *) lock->list;

	    lock->current = cell->type;

	    if (cell->type == SCTK_RWLOCK_WRITE)
	      {
		lock->wait = SCTK_RWLOCK_WR_WAIT;
	      }

	    if (first == 1 || lock->wait == SCTK_RWLOCK_NO_WR_WAIT)
	      {
		lock->list = lock->list->next;
		to_wake_up = cell->my_self;
		to_wake = (sctk_ethread_per_thread_t *) to_wake_up;
		cell->wake = 1;
		retrun_task (to_wake);
		first = 0;
	      }
	  }
	while (lock->list != NULL && lock->wait == SCTK_RWLOCK_NO_WR_WAIT);
      }
    if (lock->list == NULL)
      {
	lock->list_tail = NULL;
      }
    if (lock->lock <= 0)
      {
	lock->wait = SCTK_RWLOCK_NO_WR_WAIT;
	lock->current = SCTK_RWLOCK_ALONE;
      }
    sctk_spinlock_unlock (&(lock->spinlock));

    return 0;
  }

  static inline int
    __sctk_ethread_rwlock_trylock (sctk_ethread_rwlock_t * rwlock, int type)
  {
    sctk_spinlock_lock (&rwlock->spinlock);
    /*si on n'est pas sur de devoir attendre */
    if (rwlock->current != SCTK_RWLOCK_WRITE
	&& rwlock->wait != SCTK_RWLOCK_WR_WAIT)
      {
	if (type == SCTK_RWLOCK_READ)
	  {
	    rwlock->current = SCTK_RWLOCK_READ;
	    rwlock->lock++;
	    sctk_spinlock_unlock (&rwlock->spinlock);
	    return 0;
	  }
      }
    /*si il y a aucune attente , alors on peut écrire directement */
    if (type == SCTK_RWLOCK_WRITE
	&& rwlock->current == SCTK_RWLOCK_ALONE
	&& rwlock->wait == SCTK_RWLOCK_NO_WR_WAIT)
      {
	rwlock->current = SCTK_RWLOCK_WRITE;
	rwlock->lock++;
	sctk_spinlock_unlock (&rwlock->spinlock);
	return 0;
      }
    sctk_spinlock_unlock (&rwlock->spinlock);
    return SCTK_EBUSY;
  }
  /*les attributs des rwlock */
  static inline
    int __sctk_ethread_rwlockattr_destroy (sctk_ethread_rwlockattr_t * attr)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    return 0;
  }
  static inline
    int __sctk_ethread_rwlockattr_init (sctk_ethread_rwlockattr_t * attr)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;
    return 0;
  }

  static inline int __sctk_ethread_rwlockattr_getpshared (const
							  sctk_ethread_rwlockattr_t
							  * attr,
							  int *pshared)
  {
    if (attr == NULL || pshared == NULL)
      return SCTK_EINVAL;
    *pshared = attr->pshared;
    return 0;
  }
  static inline
    int __sctk_ethread_rwlockattr_setpshared (sctk_ethread_rwlockattr_t
					      * attr, int pshared)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    if (pshared != SCTK_THREAD_PROCESS_PRIVATE
	&& pshared != SCTK_THREAD_PROCESS_SHARED)
      return SCTK_EINVAL;
    if (pshared == SCTK_THREAD_PROCESS_SHARED)
      return SCTK_ENOTSUP;
    attr->pshared = pshared;
    return 0;
  }






  /* Barrier */
  /*on ne supporte pas les attributs dans le sens où on ne supporte pas le fait 
   *qu'on soit sur plusieurs processus*/
  static inline int
    __sctk_ethread_barrier_init (sctk_ethread_barrier_t * barrier,
				 const sctk_ethread_barrierattr_t * attr,
				 unsigned count)
  {
    if (barrier == NULL)
      return SCTK_EINVAL;
    if (count == 0)
      return SCTK_EINVAL;
    if (attr != NULL)
      {
	if (attr->pshared == SCTK_THREAD_PROCESS_SHARED)
	  {
	    fprintf (stderr, "Invalid pshared value in attr\n");
	    abort ();
	  }
      }
    barrier->spinlock = SCTK_SPINLOCK_INITIALIZER;
    barrier->list = NULL;
    barrier->list_tail = NULL;
    barrier->nb_max = count;
    barrier->lock = count;
    return 0;
  }
  static inline int
    __sctk_ethread_barrier_destroy (sctk_ethread_barrier_t * barrier)
  {
    if (barrier == NULL)
      return SCTK_EINVAL;
    return 0;
  }
  static inline int __sctk_ethread_barrier_wait (sctk_ethread_barrier_t *
						 barrier,
						 sctk_ethread_virtual_processor_t
						 * vp,
						 sctk_ethread_per_thread_t
						 * owner,
						 void (*retrun_task)
						 (sctk_ethread_per_thread_t *
						  task))
  {
    sctk_ethread_mutex_cell_t cell;
    int ret;
    ret = 0;
    cell.my_self = owner;
    cell.next = NULL;
    cell.wake = 0;

    sctk_spinlock_lock (&barrier->spinlock);

    barrier->lock--;

    if (barrier->lock > 0)
      {
	if (barrier->list == NULL)
	  {
	    barrier->list = &cell;
	    barrier->list_tail = &cell;
	  }
	else
	  {
	    barrier->list_tail->next = &cell;
	    barrier->list_tail = &cell;
	  }
	sctk_nodebug ("blocked on %p", barrier);
	if (owner->status == ethread_ready)
	  {
	    owner->status = ethread_blocked;
	    owner->no_auto_enqueue = 1;
	    sctk_spinlock_unlock (&barrier->spinlock);
	    __sctk_ethread_sched_yield_vp_poll (vp, owner);
	    owner->status = ethread_ready;
	    sctk_spinlock_lock (&barrier->spinlock);
	  }
	else
	  {
	    sctk_spinlock_unlock (&barrier->spinlock);
	    while (cell.wake != 1)
	      {
		__sctk_ethread_sched_yield_vp (vp, owner);
	      }
	  }
      }
    else
      {
	sctk_ethread_mutex_cell_t *list;

	list = (sctk_ethread_mutex_cell_t *)barrier->list;

	barrier->list_tail = NULL;
	barrier->lock = barrier->nb_max;
	barrier->list = NULL;
	sctk_spinlock_unlock (&barrier->spinlock);

	while (list != NULL)
	  {
	    sctk_ethread_mutex_cell_t *cell2;
	    sctk_ethread_per_thread_t *to_wake;
	    sctk_ethread_per_thread_t *to_wake_up;
	    cell2 = (sctk_ethread_mutex_cell_t *) list;
	    list = list->next;
	    to_wake_up = cell2->my_self;
	    to_wake = (sctk_ethread_per_thread_t *) to_wake_up;
	    cell2->wake = 1;
	    retrun_task (to_wake);
	  }
	ret = SCTK_THREAD_BARRIER_SERIAL_THREAD;
      }
    sctk_spinlock_unlock (&barrier->spinlock);
    return ret;
  }


  /*les attributs des barrier */
  static inline
    int __sctk_ethread_barrierattr_destroy (sctk_ethread_barrierattr_t * attr)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    return 0;
  }
  static inline
    int __sctk_ethread_barrierattr_init (sctk_ethread_barrierattr_t * attr)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;
    return 0;
  }

  static inline int __sctk_ethread_barrierattr_getpshared (const
							   sctk_ethread_barrierattr_t
							   * attr,
							   int *pshared)
  {
    if (attr == NULL || pshared == NULL)
      return SCTK_EINVAL;
    *pshared = attr->pshared;
    return 0;
  }
  static inline int
    __sctk_ethread_barrierattr_setpshared (sctk_ethread_barrierattr_t *
					   attr, int pshared)
  {
    if (attr == NULL)
      return SCTK_EINVAL;
    if (pshared != SCTK_THREAD_PROCESS_PRIVATE
	&& pshared != SCTK_THREAD_PROCESS_SHARED)
      return SCTK_EINVAL;
    if (pshared == SCTK_THREAD_PROCESS_SHARED)
      return SCTK_ENOTSUP;
    attr->pshared = pshared;
    return 0;
  }

#ifdef __cplusplus
}
#endif
#endif
