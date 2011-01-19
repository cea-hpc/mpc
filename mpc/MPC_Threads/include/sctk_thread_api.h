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
#ifndef __SCTK_THREADS_API_H_
#define __SCTK_THREADS_API_H_

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include "sctk_config.h"
#if defined(SunOS_SYS) || defined(AIX_SYS) || defined(HP_UX_SYS)
/* typedef clockid_t __clockid_t; */
struct timespec;
#endif

#ifdef __cplusplus
extern "C"
{
#endif


  struct sched_param;

  int sctk_thread_atfork (void (*__prepare) (void),
			  void (*__parent) (void), void (*__child) (void));
  int sctk_thread_attr_destroy (sctk_thread_attr_t * __attr);
  int sctk_thread_attr_getdetachstate (const sctk_thread_attr_t * __attr,
				       int *__detachstate);
  int sctk_thread_attr_getguardsize (const sctk_thread_attr_t *
				     __attr, size_t * __guardsize);
  int sctk_thread_attr_getinheritsched (const sctk_thread_attr_t *
					__attr, int *__inherit);
  int sctk_thread_attr_getschedparam (const sctk_thread_attr_t *
				      __attr, struct sched_param *__param);
  int sctk_thread_attr_getschedpolicy (const sctk_thread_attr_t * __attr,
				       int *__policy);
  int sctk_thread_attr_getscope (const sctk_thread_attr_t * __attr,
				 int *__scope);
  int sctk_thread_attr_getstackaddr (const sctk_thread_attr_t * __attr,
				     void **__stackaddr);
  int sctk_thread_attr_getstack (const sctk_thread_attr_t * __attr,
				 void **__stackaddr, size_t * __stacksize);
  int sctk_thread_attr_getstacksize (const sctk_thread_attr_t * __attr,
				     size_t * __stacksize);
  int sctk_thread_attr_init (sctk_thread_attr_t * __attr);
  int sctk_thread_attr_setdetachstate (sctk_thread_attr_t * __attr,
				       int __detachstate);
  int sctk_thread_attr_setguardsize (sctk_thread_attr_t * __attr,
				     size_t __guardsize);
  int sctk_thread_attr_setinheritsched (sctk_thread_attr_t * __attr,
					int __inherit);
  int sctk_thread_attr_setschedparam (sctk_thread_attr_t * __attr,
				      const struct sched_param *__param);
  int sctk_thread_attr_setschedpolicy (sctk_thread_attr_t * __attr,
				       int __policy);
  int sctk_thread_attr_setscope (sctk_thread_attr_t * __attr, int __scope);
  int sctk_thread_attr_setstackaddr (sctk_thread_attr_t * __attr,
				     void *__stackaddr);
  int sctk_thread_attr_setstack (sctk_thread_attr_t * __attr,
				 void *__stackaddr, size_t __stacksize);
  int sctk_thread_attr_setstacksize (sctk_thread_attr_t * __attr,
				     size_t __stacksize);
  int
    sctk_thread_attr_setbinding (sctk_thread_attr_t * __attr, int __binding);
  int
    sctk_thread_attr_getbinding (sctk_thread_attr_t * __attr, int *__binding);


  int sctk_thread_barrierattr_destroy (sctk_thread_barrierattr_t * __attr);
  int sctk_thread_barrierattr_init (sctk_thread_barrierattr_t * __attr);
  int sctk_thread_barrierattr_setpshared (sctk_thread_barrierattr_t *
					  __attr, int __pshared);
  int sctk_thread_barrier_destroy (sctk_thread_barrier_t * __barrier);
  int sctk_thread_barrier_init (sctk_thread_barrier_t * __barrier,
				const sctk_thread_barrierattr_t *
				__attr, unsigned int __count);
  int sctk_thread_barrier_wait (sctk_thread_barrier_t * __barrier);

  int sctk_thread_cancel (sctk_thread_t __cancelthread);
  int sctk_thread_condattr_destroy (sctk_thread_condattr_t * __attr);
  int sctk_thread_condattr_getpshared (const sctk_thread_condattr_t *
				       __attr, int *__pshared);
  int sctk_thread_condattr_init (sctk_thread_condattr_t * __attr);
  int sctk_thread_condattr_setpshared (sctk_thread_condattr_t * __attr,
				       int __pshared);
  int sctk_thread_condattr_setclock (sctk_thread_condattr_t *
				     attr, clockid_t clock_id);
  int sctk_thread_condattr_getclock (sctk_thread_condattr_t *
				     attr, clockid_t * clock_id);
  int sctk_thread_cond_broadcast (sctk_thread_cond_t * __cond);
  int sctk_thread_cond_destroy (sctk_thread_cond_t * __cond);
  int sctk_thread_cond_init (sctk_thread_cond_t * __cond,
			     const sctk_thread_condattr_t * __cond_attr);
  int sctk_thread_cond_signal (sctk_thread_cond_t * __cond);
  int sctk_thread_cond_timedwait (sctk_thread_cond_t * __cond,
				  sctk_thread_mutex_t * __mutex,
				  const struct timespec *__abstime);
  int sctk_thread_cond_wait (sctk_thread_cond_t * __cond,
			     sctk_thread_mutex_t * __mutex);
  int sctk_thread_create (sctk_thread_t * __threadp,
			  const sctk_thread_attr_t * __attr,
			  void *(*__start_routine) (void *),
			  void *__arg, long task_id);
  int
    sctk_user_thread_create (sctk_thread_t * __threadp,
			     const sctk_thread_attr_t * __attr,
			     void *(*__start_routine) (void *), void *__arg);

  int sctk_thread_detach (sctk_thread_t __th);
  int sctk_thread_equal (sctk_thread_t __thread1, sctk_thread_t __thread2);
  void sctk_thread_exit (void *__retval);
  int sctk_thread_getconcurrency (void);
  int sctk_thread_getcpuclockid (sctk_thread_t __thread_id,
				 clockid_t * __clock_id);
  int sctk_thread_getschedparam (sctk_thread_t __target_thread,
				 int *__policy, struct sched_param *__param);
  void *sctk_thread_getspecific (sctk_thread_key_t __key);
  int sctk_thread_join (sctk_thread_t __th, void **__thread_return);
  int sctk_thread_kill (sctk_thread_t thread, int signo);
  int sctk_thread_sigsuspend (sigset_t * set);
  int sctk_thread_process_kill (pid_t pid, int sig);
  int sctk_thread_sigpending (sigset_t * set);
  int sctk_thread_sigmask (int how, const sigset_t * newmask,
			   sigset_t * oldmask);
  int sctk_thread_sigwait (const sigset_t * set, int *sig);
  int sctk_thread_key_create (sctk_thread_key_t * __key,
			      void (*__destr_function) (void *));
  int sctk_thread_key_delete (sctk_thread_key_t __key);
  int sctk_thread_mutexattr_destroy (sctk_thread_mutexattr_t * __attr);
  int sctk_thread_mutexattr_getpshared (const sctk_thread_mutexattr_t *
					__attr, int *__pshared);
  int sctk_thread_mutexattr_gettype (const sctk_thread_mutexattr_t *
				     __attr, int *__kind);
  int sctk_thread_mutexattr_init (sctk_thread_mutexattr_t * __attr);
  int sctk_thread_mutexattr_setpshared (sctk_thread_mutexattr_t * __attr,
					int __pshared);
  int sctk_thread_mutexattr_settype (sctk_thread_mutexattr_t * __attr,
				     int __kind);
  int sctk_thread_mutex_destroy (sctk_thread_mutex_t * __mutex);
  int sctk_thread_mutex_init (sctk_thread_mutex_t * __mutex,
			      const sctk_thread_mutexattr_t * __mutex_attr);
  int sctk_thread_mutex_lock (sctk_thread_mutex_t * __mutex);
  int sctk_thread_mutex_spinlock (sctk_thread_mutex_t * __mutex);
  int sctk_thread_mutex_timedlock (sctk_thread_mutex_t * __mutex,
				   const struct timespec *__abstime);
  int sctk_thread_mutex_trylock (sctk_thread_mutex_t * __mutex);
  int sctk_thread_mutex_unlock (sctk_thread_mutex_t * __mutex);

  int sctk_thread_sem_init (sctk_thread_sem_t * sem, int pshared,
			    unsigned int value);
  int sctk_thread_sem_wait (sctk_thread_sem_t * sem);
  int sctk_thread_sem_trywait (sctk_thread_sem_t * sem);
  int sctk_thread_sem_post (sctk_thread_sem_t * sem);
  int sctk_thread_sem_getvalue (sctk_thread_sem_t * sem, int *sval);
  int sctk_thread_sem_destroy (sctk_thread_sem_t * sem);
  sctk_thread_sem_t *sctk_thread_sem_open (const char *__name,
					   int __oflag, ...);
  int sctk_thread_sem_close (sctk_thread_sem_t * __sem);
  int sctk_thread_sem_unlink (const char *__name);
  int sctk_thread_sem_timedwait (sctk_thread_sem_t * __sem,
				 const struct timespec *__abstime);

  int sctk_thread_once (sctk_thread_once_t * __once_control,
			void (*__init_routine) (void));

  int sctk_thread_rwlockattr_destroy (sctk_thread_rwlockattr_t * __attr);
  int sctk_thread_rwlockattr_getpshared (const sctk_thread_rwlockattr_t *
					 __attr, int *__pshared);
  int sctk_thread_rwlockattr_init (sctk_thread_rwlockattr_t * __attr);
  int sctk_thread_rwlockattr_setpshared (sctk_thread_rwlockattr_t *
					 __attr, int __pshared);
  int sctk_thread_rwlock_destroy (sctk_thread_rwlock_t * __rwlock);
  int sctk_thread_rwlock_init (sctk_thread_rwlock_t * __rwlock,
			       const sctk_thread_rwlockattr_t * __attr);
  int sctk_thread_rwlock_rdlock (sctk_thread_rwlock_t * __rwlock);

  int sctk_thread_rwlock_timedrdlock (sctk_thread_rwlock_t *
				      __rwlock,
				      const struct timespec *__abstime);
  int sctk_thread_rwlock_timedwrlock (sctk_thread_rwlock_t *
				      __rwlock,
				      const struct timespec *__abstime);

  int sctk_thread_rwlock_tryrdlock (sctk_thread_rwlock_t * __rwlock);
  int sctk_thread_rwlock_trywrlock (sctk_thread_rwlock_t * __rwlock);
  int sctk_thread_rwlock_unlock (sctk_thread_rwlock_t * __rwlock);
  int sctk_thread_rwlock_wrlock (sctk_thread_rwlock_t * __rwlock);

  sctk_thread_t sctk_thread_self (void);
  sctk_thread_t sctk_thread_self_check (void);
  int sctk_thread_setcancelstate (int __state, int *__oldstate);
  int sctk_thread_setcanceltype (int __type, int *__oldtype);
  int sctk_thread_setconcurrency (int __level);
  int sctk_thread_setschedparam (sctk_thread_t __target_thread,
				 int __policy,
				 const struct sched_param *__param);
  int sctk_thread_setspecific (sctk_thread_key_t __key,
			       const void *__pointer);

  int sctk_thread_spin_destroy (sctk_thread_spinlock_t * __lock);
  int sctk_thread_spin_init (sctk_thread_spinlock_t * __lock, int __pshared);
  int sctk_thread_spin_lock (sctk_thread_spinlock_t * __lock);
  int sctk_thread_spin_trylock (sctk_thread_spinlock_t * __lock);
  int sctk_thread_spin_unlock (sctk_thread_spinlock_t * __lock);

  void sctk_thread_testcancel (void);
  int sctk_thread_yield (void);
  unsigned int sctk_thread_sleep (unsigned int seconds);
  int sctk_thread_usleep (unsigned int seconds);
  int sctk_thread_nanosleep (const struct timespec *req,
			     struct timespec *rem);

  int sctk_thread_sched_get_priority_max (int policy);
  int sctk_thread_sched_get_priority_min (int policy);
  int sctk_thread_attr_getschedpolicy (const sctk_thread_attr_t *, int *);
  int sctk_thread_barrierattr_getpshared (const sctk_thread_barrierattr_t
					  *, int *);
  int sctk_thread_mutex_getprioceiling (const sctk_thread_mutex_t *, int *);
  int sctk_thread_mutex_setprioceiling (sctk_thread_mutex_t *, int, int *);
  int sctk_thread_mutexattr_getprioceiling (const sctk_thread_mutexattr_t
					    *, int *);
  int sctk_thread_mutexattr_setprioceiling (sctk_thread_mutexattr_t *, int);
  int sctk_thread_mutexattr_getprotocol (const sctk_thread_mutexattr_t *
					 attr, int *protocol);
  int sctk_thread_mutexattr_setprotocol (sctk_thread_mutexattr_t *, int);
  int sctk_thread_setschedprio (sctk_thread_t, int);
  int sctk_thread_getattr_np (sctk_thread_t th, sctk_thread_attr_t * attr);


  unsigned long sctk_thread_atomic_add (volatile unsigned long *ptr,
					unsigned long val);
  unsigned long sctk_tls_entry_add (unsigned long size,
				    void (*func) (void *));
  void sctk_tls_init_key (unsigned long key, void (*func) (void *));

#ifdef __cplusplus
}
#endif
#endif
