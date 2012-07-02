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
#ifndef __mpcthread__H
#define __mpcthread__H


#include <string.h>
#include <sched.h>
#include "sctk_config.h"
#include "sctk_posix_thread.h"
#include "sctk_thread_api.h"

#ifndef MPC_NO_AUTO_MAIN_REDEF
#undef main
#ifdef __cplusplus
#define main long mpc_user_main_dummy__ (); extern "C" int mpc_user_main__
#else
#define main mpc_user_main__
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define mpc_thread_sem_t sctk_thread_sem_t
#define mpc_thread_t sctk_thread_t
#define mpc_thread_attr_t sctk_thread_attr_t
#define mpc_thread_condattr_t sctk_thread_condattr_t
#define mpc_thread_cond_t sctk_thread_cond_t
#define mpc_thread_mutex_t sctk_thread_mutex_t
#define mpc_thread_mutexattr_t sctk_thread_mutexattr_t
#define mpc_thread_key_t sctk_thread_key_t
#define mpc_thread_once_t sctk_thread_once_t

#define mpc_thread_barrierattr_t sctk_thread_barrierattr_t
#define mpc_thread_barrier_t sctk_thread_barrier_t
#define mpc_thread_spinlock_t sctk_thread_spinlock_t

#define mpc_thread_rwlockattr_t sctk_thread_rwlockattr_t
#define mpc_thread_rwlock_t sctk_thread_rwlock_t

#define MPC_THREAD_RWLOCK_INITIALIZER SCTK_THREAD_RWLOCK_INITIALIZER

#define MPC_THREAD_MUTEX_NORMAL SCTK_THREAD_MUTEX_NORMAL
#define MPC_THREAD_MUTEX_RECURSIVE SCTK_THREAD_MUTEX_RECURSIVE
#define MPC_THREAD_MUTEX_ERRORCHECK SCTK_THREAD_MUTEX_ERRORCHECK
#define MPC_THREAD_MUTEX_DEFAULT SCTK_THREAD_MUTEX_DEFAULT

#define MPC_THREAD_BARRIER_SERIAL_THREAD SCTK_THREAD_BARRIER_SERIAL_THREAD

#define MPC_THREAD_MUTEX_INITIALIZER SCTK_THREAD_MUTEX_INITIALIZER
#define MPC_THREAD_COND_INITIALIZER SCTK_THREAD_COND_INITIALIZER
#define MPC_THREAD_ONCE_INIT SCTK_THREAD_ONCE_INIT
#define MPC_THREAD_KEYS_MAX SCTK_THREAD_KEYS_MAX
#define MPC_THREAD_CREATE_JOINABLE SCTK_THREAD_CREATE_JOINABLE
#define MPC_THREAD_CREATE_DETACHED SCTK_THREAD_CREATE_DETACHED
#define MPC_THREAD_INHERIT_SCHED SCTK_THREAD_INHERIT_SCHED
#define MPC_THREAD_EXPLICIT_SCHED SCTK_THREAD_EXPLICIT_SCHED
#define MPC_THREAD_SCOPE_SYSTEM SCTK_THREAD_SCOPE_SYSTEM
#define MPC_THREAD_SCOPE_PROCESS SCTK_THREAD_SCOPE_PROCESS
#define MPC_THREAD_PROCESS_PRIVATE SCTK_THREAD_PROCESS_PRIVATE
#define MPC_THREAD_PROCESS_SHARED SCTK_THREAD_PROCESS_SHARED
#define MPC_THREAD_CANCEL_ENABLE SCTK_THREAD_CANCEL_ENABLE
#define MPC_THREAD_CANCEL_DISABLE SCTK_THREAD_CANCEL_DISABLE
#define MPC_THREAD_CANCEL_DEFERRED SCTK_THREAD_CANCEL_DEFERRED
#define MPC_THREAD_CANCEL_ASYNCHRONOUS SCTK_THREAD_CANCEL_ASYNCHRONOUS
#define MPC_THREAD_CANCELED SCTK_THREAD_CANCELED

#undef SEM_VALUE_MAX
#define SEM_VALUE_MAX SCTK_SEM_VALUE_MAX
#undef SCTK_SEM_FAILED
#define SEM_FAILED SCTK_SEM_FAILED


/* pthread_atfork */
  static inline int mpc_thread_atfork (void (*a) (void), void (*b) (void),
				       void (*c) (void))
  {
    return sctk_thread_atfork (a, b, c);
  }
/* pthread_attr_destroy */
  static inline int mpc_thread_attr_destroy (sctk_thread_attr_t * attr)
  {
    return sctk_thread_attr_destroy (attr);
  }

/* pthread_attr_getdetachstate */
  static inline int mpc_thread_attr_getdetachstate (const
						    sctk_thread_attr_t *
						    attr, int *state)
  {
    return sctk_thread_attr_getdetachstate (attr, state);
  }

/* pthread_attr_getguardsize */
  static inline int mpc_thread_attr_getguardsize (const sctk_thread_attr_t
						  * attr, size_t * size)
  {
    return sctk_thread_attr_getguardsize (attr, size);
  }

/* pthread_attr_getinheritsched */
  static inline int mpc_thread_attr_getinheritsched (const
						     sctk_thread_attr_t *
						     attr, int *type)
  {
    return sctk_thread_attr_getinheritsched (attr, type);
  }
/* pthread_attr_getschedparam */
  static inline int mpc_thread_attr_getschedparam (const
						   sctk_thread_attr_t *
						   attr,
						   struct sched_param *type)
  {
    return sctk_thread_attr_getschedparam (attr, type);
  }

/* sched_get_priority_max*/
#define sched_get_priority_max(a) sctk_thread_sched_get_priority_max (a)

/* sched_get_priority_min*/
#define sched_get_priority_min(a) sctk_thread_sched_get_priority_min(a)

/* pthread_attr_getschedpolicy */
  static inline int mpc_thread_attr_getschedpolicy (const
						    sctk_thread_attr_t *
						    attr, int *policy)
  {
    return sctk_thread_attr_getschedpolicy (attr, policy);
  }

/* pthread_attr_getscope */
  static inline int mpc_thread_attr_getscope (const sctk_thread_attr_t *
					      attr, int *scope)
  {
    return sctk_thread_attr_getscope (attr, scope);
  }

/* pthread_attr_getstack */
  static inline int mpc_thread_attr_getstack (const sctk_thread_attr_t *
					      attr, void **stack,
					      size_t * size)
  {
    return sctk_thread_attr_getstack (attr, stack, size);
  }

/* pthread_attr_getstackaddr */
  static inline int mpc_thread_attr_getstackaddr (const sctk_thread_attr_t
						  * attr, void **stack)
  {
    return sctk_thread_attr_getstackaddr (attr, stack);
  }

/* pthread_attr_getstacksize */
  static inline int mpc_thread_attr_getstacksize (const sctk_thread_attr_t
						  * attr, size_t * size)
  {
    return sctk_thread_attr_getstacksize (attr, size);
  }

/* pthread_attr_init */
  static inline int mpc_thread_attr_init (sctk_thread_attr_t * attr)
  {
    return sctk_thread_attr_init (attr);
  }


/* pthread_attr_setdetachstate */
  static inline int mpc_thread_attr_setdetachstate (sctk_thread_attr_t *
						    attr, int state)
  {
    return sctk_thread_attr_setdetachstate (attr, state);
  }


/* pthread_attr_setguardsize */
  static inline int mpc_thread_attr_setguardsize (sctk_thread_attr_t *
						  attr, size_t size)
  {
    return sctk_thread_attr_setguardsize (attr, size);
  }


/* pthread_attr_setinheritsched */
  static inline int mpc_thread_attr_setinheritsched (sctk_thread_attr_t *
						     attr, int sched)
  {
    return sctk_thread_attr_setinheritsched (attr, sched);
  }


/* pthread_attr_setschedparam */
  static inline int mpc_thread_attr_setschedparam (sctk_thread_attr_t *
						   attr, const struct
						   sched_param *sched)
  {
    return sctk_thread_attr_setschedparam (attr, sched);
  }
/* pthread_attr_setschedpolicy */
  static inline int mpc_thread_attr_setschedpolicy (sctk_thread_attr_t *
						    attr, int policy)
  {
    return sctk_thread_attr_setschedpolicy (attr, policy);
  }


/* pthread_attr_setscope */
  static inline int mpc_thread_attr_setscope (sctk_thread_attr_t * attr,
					      int scope)
  {
    return sctk_thread_attr_setscope (attr, scope);
  }


/* pthread_attr_setstack */
  static inline int mpc_thread_attr_setstack (sctk_thread_attr_t * attr,
					      void *stack, size_t size)
  {
    return sctk_thread_attr_setstack (attr, stack, size);
  }


/* pthread_attr_setstackaddr */
  static inline int mpc_thread_attr_setstackaddr (sctk_thread_attr_t *
						  attr, void *stack)
  {
    return sctk_thread_attr_setstackaddr (attr, stack);
  }


/* pthread_attr_setstacksize */
  static inline int mpc_thread_attr_setstacksize (sctk_thread_attr_t *
						  attr, size_t size)
  {
    return sctk_thread_attr_setstacksize (attr, size);
  }


/* pthread_barrier_destroy */
  static inline int mpc_thread_barrier_destroy (sctk_thread_barrier_t *
						barrier)
  {
    return sctk_thread_barrier_destroy (barrier);
  }


/* pthread_barrier_init */
  static inline int mpc_thread_barrier_init (sctk_thread_barrier_t *
					     barrier, const
					     sctk_thread_barrierattr_t *
					     attr, unsigned i)
  {
    return sctk_thread_barrier_init (barrier, attr, i);
  }

/* pthread_barrier_wait */
  static inline int mpc_thread_barrier_wait (sctk_thread_barrier_t * barrier)
  {
    return sctk_thread_barrier_wait (barrier);
  }


/* pthread_barrierattr_destroy */
  static inline int
    mpc_thread_barrierattr_destroy (sctk_thread_barrierattr_t * barrier)
  {
    return sctk_thread_barrierattr_destroy (barrier);
  }


/* pthread_barrierattr_getpshared */
  static inline int mpc_thread_barrierattr_getpshared (const
						       sctk_thread_barrierattr_t
						       * barrier,
						       int *pshared)
  {
    return sctk_thread_barrierattr_getpshared (barrier, pshared);
  }
/* pthread_barrierattr_init */
  static inline int mpc_thread_barrierattr_init (sctk_thread_barrierattr_t
						 * barrier)
  {
    return sctk_thread_barrierattr_init (barrier);
  }

/* pthread_barrierattr_setpshared */
  static inline int
    mpc_thread_barrierattr_setpshared (sctk_thread_barrierattr_t * barrier,
				       int pshared)
  {
    return sctk_thread_barrierattr_setpshared (barrier, pshared);
  }

/* pthread_cancel */
  static inline int mpc_thread_cancel (sctk_thread_t thread)
  {
    return sctk_thread_cancel (thread);
  }

/* pthread_cleanup_push */
#define mpc_thread_cleanup_push sctk_thread_cleanup_push

/* pthread_cleanup_pop */
#define mpc_thread_cleanup_pop sctk_thread_cleanup_pop

/* pthread_cond_broadcast */
  static inline int mpc_thread_cond_broadcast (sctk_thread_cond_t * cond)
  {
    return sctk_thread_cond_broadcast (cond);
  }

/* pthread_cond_destroy */
  static inline int mpc_thread_cond_destroy (sctk_thread_cond_t * cond)
  {
    return sctk_thread_cond_destroy (cond);
  }

/* pthread_cond_init */
  static inline int mpc_thread_cond_init (sctk_thread_cond_t * cond,
					  const sctk_thread_condattr_t * attr)
  {
    return sctk_thread_cond_init (cond, attr);
  }

/* pthread_cond_signal */
  static inline int mpc_thread_cond_signal (sctk_thread_cond_t * cond)
  {
    return sctk_thread_cond_signal (cond);
  }


/* pthread_cond_timedwait */
  static inline int mpc_thread_cond_timedwait (sctk_thread_cond_t * cond,
					       sctk_thread_mutex_t *
					       mutex,
					       const struct timespec *time)
  {
    return sctk_thread_cond_timedwait (cond, mutex, time);
  }

/* pthread_cond_wait */
  static inline int mpc_thread_cond_wait (sctk_thread_cond_t * cond,
					  sctk_thread_mutex_t * mutex)
  {
    return sctk_thread_cond_wait (cond, mutex);
  }

/* pthread_condattr_destroy */
  static inline int mpc_thread_condattr_destroy (sctk_thread_condattr_t *
						 attr)
  {
    return sctk_thread_condattr_destroy (attr);
  }

/* pthread_condattr_getclock */
  static inline int mpc_thread_condattr_getclock (sctk_thread_condattr_t *
						  attr, clockid_t * clock)
  {
    return sctk_thread_condattr_getclock (attr, clock);
  }


/* pthread_condattr_getpshared */
  static inline int mpc_thread_condattr_getpshared (const
						    sctk_thread_condattr_t
						    * attr, int *pshared)
  {
    return sctk_thread_condattr_getpshared (attr, pshared);
  }


/* pthread_condattr_init */
  static inline int mpc_thread_condattr_init (sctk_thread_condattr_t * attr)
  {
    return sctk_thread_condattr_init (attr);
  }


/* pthread_condattr_setclock */
  static inline int mpc_thread_condattr_setclock (sctk_thread_condattr_t *
						  attr, clockid_t clock)
  {
    return sctk_thread_condattr_setclock (attr, clock);
  }


/* pthread_condattr_setpshared */
  static inline int mpc_thread_condattr_setpshared (sctk_thread_condattr_t
						    * attr, int pshared)
  {
    return sctk_thread_condattr_setpshared (attr, pshared);
  }


/* pthread_create */
  static inline int mpc_thread_create (sctk_thread_t * th,
				       const sctk_thread_attr_t * attr,
				       void *(*func) (void *), void *arg)
  {
    return sctk_user_thread_create (th, attr, func, arg);
  }


/* pthread_detach */
  static inline int mpc_thread_detach (sctk_thread_t th)
  {
    return sctk_thread_detach (th);
  }

/* pthread_equal */
  static inline int mpc_thread_equal (sctk_thread_t th1, sctk_thread_t th2)
  {
    return sctk_thread_equal (th1, th2);
  }


/* pthread_exit */
  static inline void mpc_thread_exit (void *result)
  {
    sctk_thread_exit (result);
  }


/* mpc_thread_getconcurrency */
  static inline int mpc_thread_getconcurrency (void)
  {
    return sctk_thread_getconcurrency ();
  }


/* pthread_getcpuclockid */
  static inline int mpc_thread_getcpuclockid (sctk_thread_t th,
					      clockid_t * clock)
  {
    return sctk_thread_getcpuclockid (th, clock);
  }


/* pthread_getschedparam */
  static inline int mpc_thread_getschedparam (sctk_thread_t th, int *a,
					      struct sched_param *b)
  {
    return sctk_thread_getschedparam (th, a, b);
  }


/* pthread_getspecific */
  static inline void *mpc_thread_getspecific (sctk_thread_key_t key)
  {
    return sctk_thread_getspecific (key);
  }


/* pthread_join */
  static inline int mpc_thread_join (sctk_thread_t th, void **ret)
  {
    return sctk_thread_join (th, ret);
  }

/* pthread_key_create */
  static inline int mpc_thread_key_create (sctk_thread_key_t * key,
					   void (*destr) (void *))
  {
    return sctk_thread_key_create (key, destr);
  }


/* pthread_key_delete */
  static inline int mpc_thread_key_delete (sctk_thread_key_t key)
  {
    return sctk_thread_key_delete (key);
  }


/* pthread_mutex_destroy */
  static inline int mpc_thread_mutex_destroy (sctk_thread_mutex_t * mutex)
  {
    return sctk_thread_mutex_destroy (mutex);
  }


/* pthread_mutex_getprioceiling */
  static inline int mpc_thread_mutex_getprioceiling (const
						     sctk_thread_mutex_t *
						     mutex, int *prio)
  {
    return sctk_thread_mutex_getprioceiling (mutex, prio);
  }


/* pthread_mutex_init */
  static inline int mpc_thread_mutex_init (sctk_thread_mutex_t * mutex,
					   const sctk_thread_mutexattr_t *
					   attr)
  {
    return sctk_thread_mutex_init (mutex, attr);
  }

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
int user_sctk_thread_mutex_lock (sctk_thread_mutex_t * mutex);
int user_sctk_thread_mutex_unlock (sctk_thread_mutex_t * mutex);
#endif

/* pthread_mutex_lock */
  static inline int mpc_thread_mutex_lock (sctk_thread_mutex_t * mutex)
  {
#ifdef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
    return sctk_thread_mutex_lock (mutex);
#else
    return user_sctk_thread_mutex_lock(mutex);
#endif
  }


/* pthread_mutex_setprioceiling */
  static inline int mpc_thread_mutex_setprioceiling (sctk_thread_mutex_t *
						     mutex, int a, int *b)
  {
    return sctk_thread_mutex_setprioceiling (mutex, a, b);
  }


/* pthread_mutex_timedlock */
  static inline int mpc_thread_mutex_timedlock (sctk_thread_mutex_t *
						mutex,
						const struct timespec *time)
  {
    return sctk_thread_mutex_timedlock (mutex, time);
  }


/* pthread_mutex_trylock */
  static inline int mpc_thread_mutex_trylock (sctk_thread_mutex_t * mutex)
  {
    return sctk_thread_mutex_trylock (mutex);
  }


/* pthread_mutex_unlock */
  static inline int mpc_thread_mutex_unlock (sctk_thread_mutex_t * mutex)
  {
#ifdef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
    return sctk_thread_mutex_unlock (mutex);
#else
    return user_sctk_thread_mutex_unlock (mutex);
#endif
  }


/* pthread_mutexattr_destroy */
  static inline int mpc_thread_mutexattr_destroy (sctk_thread_mutexattr_t
						  * attr)
  {
    return sctk_thread_mutexattr_destroy (attr);
  }


/* pthread_mutexattr_getprioceiling */
  static inline int mpc_thread_mutexattr_getprioceiling (const
							 sctk_thread_mutexattr_t
							 * attr, int *prio)
  {
    return sctk_thread_mutexattr_getprioceiling (attr, prio);
  }


/* pthread_mutexattr_getprotocol */
  static inline int mpc_thread_mutexattr_getprotocol (const
						      sctk_thread_mutexattr_t
						      * attr, int *protocol)
  {
    return sctk_thread_mutexattr_getprotocol (attr, protocol);
  }


/* pthread_mutexattr_getpshared */
  static inline int mpc_thread_mutexattr_getpshared (const
						     sctk_thread_mutexattr_t
						     * attr, int *pshared)
  {
    return sctk_thread_mutexattr_getpshared (attr, pshared);
  }


/* pthread_mutexattr_gettype */
  static inline int mpc_thread_mutexattr_gettype (const
						  sctk_thread_mutexattr_t
						  * attr, int *type)
  {
    return sctk_thread_mutexattr_gettype (attr, type);
  }


/* pthread_mutexattr_init */
  static inline int mpc_thread_mutexattr_init (sctk_thread_mutexattr_t * attr)
  {
    return sctk_thread_mutexattr_init (attr);
  }


/* pthread_mutexattr_setprioceiling */
  static inline int
    mpc_thread_mutexattr_setprioceiling (sctk_thread_mutexattr_t * attr,
					 int prio)
  {
    return sctk_thread_mutexattr_setprioceiling (attr, prio);
  }


/* pthread_mutexattr_setprotocol */
  static inline int
    mpc_thread_mutexattr_setprotocol (sctk_thread_mutexattr_t * attr,
				      int protocol)
  {
    return sctk_thread_mutexattr_setprotocol (attr, protocol);
  }


/* pthread_mutexattr_setpshared */
  static inline int
    mpc_thread_mutexattr_setpshared (sctk_thread_mutexattr_t * attr,
				     int pshared)
  {
    return sctk_thread_mutexattr_setpshared (attr, pshared);
  }


/* pthread_mutexattr_settype */
  static inline int mpc_thread_mutexattr_settype (sctk_thread_mutexattr_t
						  * attr, int type)
  {
    return sctk_thread_mutexattr_settype (attr, type);
  }


/* pthread_once */
  static inline int mpc_thread_once (sctk_thread_once_t * once,
				     void (*func) (void))
  {
    return sctk_thread_once (once, func);
  }


/* pthread_rwlock_destroy */
  static inline int mpc_thread_rwlock_destroy (sctk_thread_rwlock_t * lock)
  {
    return sctk_thread_rwlock_destroy (lock);
  }


/* pthread_rwlock_init */
  static inline int mpc_thread_rwlock_init (sctk_thread_rwlock_t * lock,
					    const sctk_thread_rwlockattr_t
					    * attr)
  {
    return sctk_thread_rwlock_init (lock, attr);
  }


/* pthread_rwlock_rdlock */
  static inline int mpc_thread_rwlock_rdlock (sctk_thread_rwlock_t * lock)
  {
    return sctk_thread_rwlock_rdlock (lock);
  }


/* pthread_rwlock_timedrdlock */
  static inline int mpc_thread_rwlock_timedrdlock (sctk_thread_rwlock_t *
						   lock,
						   const struct timespec
						   *time)
  {
    return sctk_thread_rwlock_timedrdlock (lock, time);
  }


/* pthread_rwlock_timedwrlock */
  static inline int mpc_thread_rwlock_timedwrlock (sctk_thread_rwlock_t *
						   lock,
						   const struct timespec
						   *time)
  {
    return sctk_thread_rwlock_timedwrlock (lock, time);
  }


/* pthread_rwlock_tryrdlock */
  static inline int mpc_thread_rwlock_tryrdlock (sctk_thread_rwlock_t * lock)
  {
    return sctk_thread_rwlock_tryrdlock (lock);
  }


/* pthread_rwlock_trywrlock */
  static inline int mpc_thread_rwlock_trywrlock (sctk_thread_rwlock_t * lock)
  {
    return sctk_thread_rwlock_trywrlock (lock);
  }


/* pthread_rwlock_unlock */
  static inline int mpc_thread_rwlock_unlock (sctk_thread_rwlock_t * lock)
  {
    return sctk_thread_rwlock_unlock (lock);
  }


/* pthread_rwlock_wrlock */
  static inline int mpc_thread_rwlock_wrlock (sctk_thread_rwlock_t * lock)
  {
    return sctk_thread_rwlock_wrlock (lock);
  }


/* pthread_rwlockattr_destroy */
  static inline int
    mpc_thread_rwlockattr_destroy (sctk_thread_rwlockattr_t * attr)
  {
    return sctk_thread_rwlockattr_destroy (attr);
  }


/* pthread_rwlockattr_getpshared */
  static inline int mpc_thread_rwlockattr_getpshared (const
						      sctk_thread_rwlockattr_t
						      * attr, int *pshared)
  {
    return sctk_thread_rwlockattr_getpshared (attr, pshared);
  }


/* pthread_rwlockattr_init */
  static inline int mpc_thread_rwlockattr_init (sctk_thread_rwlockattr_t *
						attr)
  {
    return sctk_thread_rwlockattr_init (attr);
  }


/* pthread_rwlockattr_setpshared */
  static inline int
    mpc_thread_rwlockattr_setpshared (sctk_thread_rwlockattr_t * attr,
				      int pshared)
  {
    return sctk_thread_rwlockattr_setpshared (attr, pshared);
  }


/* pthread_self */
  static inline sctk_thread_t mpc_thread_self (void)
  {
    return sctk_thread_self ();
  }


/* pthread_setcancelstate */
  static inline int mpc_thread_setcancelstate (int newarg, int *old)
  {
    return sctk_thread_setcancelstate (newarg, old);
  }


/* pthread_setcanceltype */
  static inline int mpc_thread_setcanceltype (int newarg, int *old)
  {
    return sctk_thread_setcanceltype (newarg, old);
  }


/* pthread_setconcurrency */
  static inline int mpc_thread_setconcurrency (int concurrency)
  {
    return sctk_thread_setconcurrency (concurrency);
  }


/* pthread_setschedparam */
  static inline int mpc_thread_setschedparam (sctk_thread_t th, int a,
					      const struct sched_param *b)
  {
    return sctk_thread_setschedparam (th, a, b);
  }


/* pthread_setschedprio */
  static inline int mpc_thread_setschedprio (sctk_thread_t th, int prio)
  {
    return sctk_thread_setschedprio (th, prio);
  }


/* pthread_setspecific */
  static inline int mpc_thread_setspecific (sctk_thread_key_t key,
					    const void *value)
  {
    return sctk_thread_setspecific (key, value);
  }


/* pthread_spin_destroy */
  static inline int mpc_thread_spin_destroy (sctk_thread_spinlock_t * spin)
  {
    return sctk_thread_spin_destroy (spin);
  }


/* pthread_spin_init */
  static inline int mpc_thread_spin_init (sctk_thread_spinlock_t * spin,
					  int a)
  {
    return sctk_thread_spin_init (spin, a);
  }


/* pthread_spin_lock */
  static inline int mpc_thread_spin_lock (sctk_thread_spinlock_t * spin)
  {
    return sctk_thread_spin_lock (spin);
  }


/* pthread_spin_trylock */
  static inline int mpc_thread_spin_trylock (sctk_thread_spinlock_t * spin)
  {
    return sctk_thread_spin_trylock (spin);
  }


/* pthread_spin_unlock */
  static inline int mpc_thread_spin_unlock (sctk_thread_spinlock_t * spin)
  {
    return sctk_thread_spin_unlock (spin);
  }


/* pthread_testcancel */
  static inline void mpc_thread_testcancel (void)
  {
    sctk_thread_testcancel ();
  }


/* pthread_yield */
  static inline int mpc_thread_yield (void)
  {
    return sctk_thread_yield ();
  }

  static inline int mpc_thread_kill (sctk_thread_t thread, int signo)
  {
    return sctk_thread_kill (thread, signo);
  }


  static inline int mpc_thread_sigmask (int how, const sigset_t * newmask,
					sigset_t * oldmask)
  {
    return sctk_thread_sigmask (how, newmask, oldmask);
  }

  static inline int mpc_thread_getattr_np (sctk_thread_t th,
					   sctk_thread_attr_t * attr)
  {
    return sctk_thread_getattr_np (th, attr);
  }


/*semaphore.h*/
/* sem_init */
  static inline int mpc_thread_sem_init (sctk_thread_sem_t * sem,
					 int pshared, unsigned int value)
  {
    return sctk_thread_sem_init (sem, pshared, value);
  }


/* sem_ wait*/
  static inline int mpc_thread_sem_wait (sctk_thread_sem_t * sem)
  {
    return sctk_thread_sem_wait (sem);
  }


/* sem_trywait */
  static inline int mpc_thread_sem_trywait (sctk_thread_sem_t * sem)
  {
    return sctk_thread_sem_trywait (sem);
  }


/* sem_post */
  static inline int mpc_thread_sem_post (sctk_thread_sem_t * sem)
  {
    return sctk_thread_sem_post (sem);
  }


/* sem_getvalue */
  static inline int mpc_thread_sem_getvalue (sctk_thread_sem_t * sem,
					     int *sval)
  {
    return sctk_thread_sem_getvalue (sem, sval);
  }


/* sem_destroy */
  static inline int mpc_thread_sem_destroy (sctk_thread_sem_t * sem)
  {
    return sctk_thread_sem_destroy (sem);
  }


/* sem_open */
  static inline sctk_thread_sem_t *mpc_thread_sem_open (const char *name,
							int oflag, ...)
  {
    return sctk_thread_sem_open (name, oflag);
  }


/* sem_close */
  static inline int mpc_thread_sem_close (sctk_thread_sem_t * sem)
  {
    return sctk_thread_sem_close (sem);
  }


/* sem_unlink */
  static inline int mpc_thread_sem_unlink (const char *__name)
  {
    return sctk_thread_sem_unlink (__name);
  }


/* sem_timedwait */
  static inline int mpc_thread_sem_timedwait (sctk_thread_sem_t * __sem,
					      const struct timespec
					      *__abstime)
  {
    return sctk_thread_sem_timedwait (__sem, __abstime);
  }

  static inline unsigned long mpc_thread_atomic_add (volatile unsigned long
						     *ptr, unsigned long val)
  {
    return sctk_thread_atomic_add (ptr, val);
  }

  static inline unsigned long mpc_thread_tls_entry_add (unsigned long size,
							void (*func) (void *))
  {
    return sctk_tls_entry_add (size, func);
  }

#define mpc_thread_sched_yield sctk_thread_yield
#define mpc_thread_raise(a) sctk_thread_kill(sctk_thread_self(), a)

#define mpc_thread_kill  sctk_thread_kill
#define mpc_thread_sigpending sctk_thread_sigpending
#define mpc_thread_sigsuspend sctk_thread_sigsuspend
#define mpc_thread_sigwait  sctk_thread_sigwait

#define mpc_thread_sleep sctk_thread_sleep
#define mpc_thread_usleep sctk_thread_usleep
#define mpc_thread_nanosleep  sctk_thread_nanosleep

#define sched_yield sctk_thread_yield
#define raise(a) sctk_thread_kill(sctk_thread_self(), a)

#define kill  sctk_thread_process_kill
#define sigpending sctk_thread_sigpending
#define sigsuspend sctk_thread_sigsuspend
#define sigwait  sctk_thread_sigwait

#define sleep sctk_thread_sleep
#define usleep sctk_thread_usleep
#define nanosleep  sctk_thread_nanosleep


#ifdef __cplusplus
}
#endif
#endif
