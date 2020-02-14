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
#ifndef __SCTK_INTERNAL_THREADS_H_
#define __SCTK_INTERNAL_THREADS_H_

#include <stdlib.h>
#include <signal.h>
#include "sctk_debug.h"
#include "mpcthread_config.h"
#include "sctk_thread_api.h"

#ifdef __cplusplus
extern "C"
{
#endif
  struct sched_param;

  extern double (*__sctk_ptr_thread_get_activity) (int i);

  extern int (*__sctk_ptr_thread_get_vp) (void);
  extern int (*__sctk_ptr_thread_atfork) (void (*__prepare) (void),
					  void (*__parent) (void),
					  void (*__child) (void));
  extern int (*__sctk_ptr_thread_attr_destroy) (sctk_thread_attr_t * __attr);
  extern int (*__sctk_ptr_thread_attr_getdetachstate) (const
						       sctk_thread_attr_t
						       * __attr,
						       int *__detachstate);
  extern int (*__sctk_ptr_thread_attr_getguardsize) (const
						     sctk_thread_attr_t *
						     __attr,
						     size_t * __guardsize);
  extern int (*__sctk_ptr_thread_attr_getinheritsched) (const
							sctk_thread_attr_t
							* __attr,
							int *__inherit);
  extern int (*__sctk_ptr_thread_attr_getschedparam) (const
						      sctk_thread_attr_t
						      * __attr,
						      struct sched_param
						      * __param);
  extern int (*__sctk_ptr_thread_attr_getschedpolicy) (const
						       sctk_thread_attr_t
						       * __attr,
						       int *__policy);
  extern int (*__sctk_ptr_thread_attr_getscope) (const sctk_thread_attr_t
						 * __attr, int *__scope);
  extern int (*__sctk_ptr_thread_attr_getstackaddr) (const
						     sctk_thread_attr_t *
						     __attr,
						     void **__stackaddr);
  extern int (*__sctk_ptr_thread_attr_getstack) (const sctk_thread_attr_t
						 * __attr,
						 void **__stackaddr,
						 size_t * __stacksize);
  extern int (*__sctk_ptr_thread_attr_getstacksize) (const
						     sctk_thread_attr_t *
						     __attr,
						     size_t * __stacksize);
  extern int (*__sctk_ptr_thread_attr_init) (sctk_thread_attr_t * __attr);
  extern int (*__sctk_ptr_thread_attr_setdetachstate) (sctk_thread_attr_t
						       * __attr,
						       int __detachstate);
  extern int (*__sctk_ptr_thread_attr_setguardsize) (sctk_thread_attr_t *
						     __attr,
						     size_t __guardsize);
  extern int (*__sctk_ptr_thread_attr_setinheritsched) (sctk_thread_attr_t *
						   __attr, int __inherit);
  extern int (*__sctk_ptr_thread_attr_setschedparam) (sctk_thread_attr_t
						      * __attr,
						      const struct
						      sched_param * __param);
  extern int (*__sctk_ptr_thread_sched_get_priority_max) (int policy);
  extern int (*__sctk_ptr_thread_sched_get_priority_min) (int policy);

  extern int (*__sctk_ptr_thread_attr_setschedpolicy) (sctk_thread_attr_t
						       * __attr,
						       int __policy);
  extern int (*__sctk_ptr_thread_attr_setscope) (sctk_thread_attr_t *
						 __attr, int __scope);
  extern int (*__sctk_ptr_thread_attr_setstackaddr) (sctk_thread_attr_t *
						     __attr,
						     void *__stackaddr);
  extern int (*__sctk_ptr_thread_attr_setstack) (sctk_thread_attr_t *
						 __attr,
						 void *__stackaddr,
						 size_t __stacksize);
  extern int (*__sctk_ptr_thread_attr_setstacksize) (sctk_thread_attr_t *
						     __attr,
						     size_t __stacksize);

  extern int
    (*__sctk_ptr_thread_attr_setbinding) (sctk_thread_attr_t * __attr,
					  int __binding);
  extern int
    (*__sctk_ptr_thread_attr_getbinding) (sctk_thread_attr_t * __attr,
					  int *__binding);

  extern
    int (*__sctk_ptr_thread_barrierattr_destroy) (sctk_thread_barrierattr_t
						  * __attr);
  extern
    int (*__sctk_ptr_thread_barrierattr_init)
    (sctk_thread_barrierattr_t * __attr);
  extern
    int (*__sctk_ptr_thread_barrierattr_setpshared)
    (sctk_thread_barrierattr_t * __attr, int __pshared);
  extern
    int (*__sctk_ptr_thread_barrierattr_getpshared) (const
						     sctk_thread_barrierattr_t
						     * __attr,
						     int *__pshared);
  extern int (*__sctk_ptr_thread_barrier_destroy) (sctk_thread_barrier_t
						   * __barrier);
  extern int (*__sctk_ptr_thread_barrier_init) (sctk_thread_barrier_t *
						restrict __barrier,
						const
						sctk_thread_barrierattr_t
						* restrict __attr,
						unsigned int __count);
  extern int (*__sctk_ptr_thread_barrier_wait) (sctk_thread_barrier_t *
						__barrier);

  extern int (*__sctk_ptr_thread_cancel) (sctk_thread_t __cancelthread);
  extern
    int (*__sctk_ptr_thread_condattr_destroy) (sctk_thread_condattr_t *
					       __attr);
  extern int (*__sctk_ptr_thread_condattr_getpshared) (const
						       sctk_thread_condattr_t
						       * __attr,
						       int *__pshared);
  extern int (*__sctk_ptr_thread_condattr_init) (sctk_thread_condattr_t *
						 __attr);
  extern
    int (*__sctk_ptr_thread_condattr_setpshared)
    (sctk_thread_condattr_t * __attr, int __pshared);
  extern
    int (*__sctk_ptr_thread_condattr_setclock) (sctk_thread_condattr_t
						* attr, clockid_t clock_id);
  extern
    int (*__sctk_ptr_thread_condattr_getclock) (sctk_thread_condattr_t
						* attr, clockid_t * clock_id);
  extern int (*__sctk_ptr_thread_cond_broadcast) (sctk_thread_cond_t *
						  __cond);
  extern int (*__sctk_ptr_thread_cond_destroy) (sctk_thread_cond_t * __cond);
  extern int (*__sctk_ptr_thread_cond_init) (sctk_thread_cond_t * __cond,
					     const sctk_thread_condattr_t
					     * __cond_attr);
  extern int (*__sctk_ptr_thread_cond_signal) (sctk_thread_cond_t * __cond);
  extern int (*__sctk_ptr_thread_cond_timedwait) (sctk_thread_cond_t *
						  __cond,
						  sctk_thread_mutex_t *
						  __mutex,
						  const struct timespec *
						  __abstime);
  extern int (*__sctk_ptr_thread_cond_wait) (sctk_thread_cond_t * restrict __cond,
					     sctk_thread_mutex_t * restrict __mutex);
  extern int (*__sctk_ptr_thread_create) (sctk_thread_t * __threadp,
					  const sctk_thread_attr_t *
					  __attr,
					  void *(*__start_routine) (void
								    *),
					  void *__arg);
  extern int (*__sctk_ptr_thread_user_create) (sctk_thread_t * __threadp,
					       const sctk_thread_attr_t *
					       __attr,
					       void
					       *(*__start_routine) (void
								    *),
					       void *__arg);
  extern int (*__sctk_ptr_thread_detach) (sctk_thread_t __th);
  extern int (*__sctk_ptr_thread_equal) (sctk_thread_t __thread1,
					 sctk_thread_t __thread2);
  extern void (*__sctk_ptr_thread_exit) (void *__retval);
  extern int (*__sctk_ptr_thread_getconcurrency) (void);
  extern int (*__sctk_ptr_thread_setschedprio) (sctk_thread_t __p, int __i);
  extern int (*__sctk_ptr_thread_getcpuclockid) (sctk_thread_t
						 __thread_id,
						 clockid_t * __clock_id);
  extern int (*__sctk_ptr_thread_getschedparam) (sctk_thread_t
						 __target_thread,
						 int *__policy,
						 struct sched_param *
						 __param);
  extern void *(*__sctk_ptr_thread_getspecific) (sctk_thread_key_t __key);
  extern int (*__sctk_ptr_thread_join) (sctk_thread_t __th,
					void **__thread_return);
  extern int (*__sctk_ptr_thread_kill) (sctk_thread_t thread, int signo);
  extern int (*__sctk_ptr_thread_sigsuspend) (sigset_t * set);
  extern int (*__sctk_ptr_thread_sigpending) (sigset_t * set);
  extern int (*__sctk_ptr_thread_sigmask) (int how,
					   const sigset_t * newmask,
					   sigset_t * oldmask);
  extern int (*__sctk_ptr_thread_key_create) (sctk_thread_key_t * __key,
					      void (*__destr_function)
					      (void *));
  extern int (*__sctk_ptr_thread_key_delete) (sctk_thread_key_t __key);
  extern
    int (*__sctk_ptr_thread_mutexattr_destroy) (sctk_thread_mutexattr_t
						* __attr);

  extern
    int (*__sctk_ptr_thread_mutexattr_getprioceiling) (const
						       sctk_thread_mutexattr_t
						       * __attr,
						       int *__prioceiling);
  extern
    int (*__sctk_ptr_thread_mutexattr_setprioceiling)
    (sctk_thread_mutexattr_t * __attr, int __prioceiling);

  extern
    int (*__sctk_ptr_thread_mutexattr_getprotocol) (const
						    sctk_thread_mutexattr_t
						    * __attr,
						    int *__protocol);
  extern
    int (*__sctk_ptr_thread_mutexattr_setprotocol) (sctk_thread_mutexattr_t
						    * __attr, int __protocol);

  extern
    int (*__sctk_ptr_thread_mutexattr_getpshared) (const
						   sctk_thread_mutexattr_t
						   * __attr, int *__pshared);
  extern int (*__sctk_ptr_thread_mutexattr_gettype) (const
						     sctk_thread_mutexattr_t
						     * __attr, int *__kind);
  extern int (*__sctk_ptr_thread_mutexattr_init) (sctk_thread_mutexattr_t
						  * __attr);
  extern
    int (*__sctk_ptr_thread_mutexattr_setpshared) (sctk_thread_mutexattr_t
						   * __attr, int __pshared);
  extern
    int (*__sctk_ptr_thread_mutexattr_settype) (sctk_thread_mutexattr_t
						* __attr, int __kind);
  extern int (*__sctk_ptr_thread_mutex_destroy) (sctk_thread_mutex_t *
						 __mutex);
  extern int (*__sctk_ptr_thread_mutex_init) (sctk_thread_mutex_t *
					      __mutex,
					      const
					      sctk_thread_mutexattr_t *
					      __mutex_attr);
  extern int (*__sctk_ptr_thread_mutex_lock) (sctk_thread_mutex_t * __mutex);
  extern int (*__sctk_ptr_thread_mutex_spinlock) (sctk_thread_mutex_t *
						  __mutex);
  extern int (*__sctk_ptr_thread_mutex_timedlock) (sctk_thread_mutex_t *
						   __mutex,
						   const struct timespec
						   * __abstime);
  extern int (*__sctk_ptr_thread_mutex_trylock) (sctk_thread_mutex_t *
						 __mutex);
  extern int (*__sctk_ptr_thread_mutex_unlock) (sctk_thread_mutex_t *
						__mutex);
  extern int (*__sctk_ptr_thread_sem_init) (sctk_thread_sem_t * sem,
					    int pshared, unsigned int value);
  extern int (*__sctk_ptr_thread_sem_wait) (sctk_thread_sem_t * sem);
  extern int (*__sctk_ptr_thread_sem_trywait) (sctk_thread_sem_t * sem);
  extern int (*__sctk_ptr_thread_sem_timedwait) (sctk_thread_sem_t * sem,
		  				  const struct timespec* __abstime );
  extern int (*__sctk_ptr_thread_sem_post) (sctk_thread_sem_t * sem);
  extern int (*__sctk_ptr_thread_sem_getvalue) (sctk_thread_sem_t * sem,
						int *sval);
  extern int (*__sctk_ptr_thread_sem_destroy) (sctk_thread_sem_t * sem);
  extern sctk_thread_sem_t *(*__sctk_ptr_thread_sem_open) (const char
							   *name,
							   int oflag, ...);
  extern int (*__sctk_ptr_thread_sem_close) (sctk_thread_sem_t * sem);
  extern int (*__sctk_ptr_thread_sem_unlink) (const char *name);

  extern int (*__sctk_ptr_thread_once) (sctk_thread_once_t *
					__once_control,
					void (*__init_routine) (void));

  extern
    int (*__sctk_ptr_thread_rwlockattr_destroy)
    (sctk_thread_rwlockattr_t * __attr);
  extern
    int (*__sctk_ptr_thread_rwlockattr_getpshared) (const
						    sctk_thread_rwlockattr_t
						    * restrict __attr,
						    int *restrict __pshared);
  extern
    int (*__sctk_ptr_thread_rwlockattr_init) (sctk_thread_rwlockattr_t
					      * __attr);
  extern
    int (*__sctk_ptr_thread_rwlockattr_setpshared)
    (sctk_thread_rwlockattr_t * __attr, int __pshared);
  extern int (*__sctk_ptr_thread_rwlock_destroy) (sctk_thread_rwlock_t *
						  __rwlock);
  extern int (*__sctk_ptr_thread_rwlock_init) (sctk_thread_rwlock_t *
					       restrict __rwlock,
					       const
					       sctk_thread_rwlockattr_t *
					       restrict __attr);
  extern int (*__sctk_ptr_thread_rwlock_rdlock) (sctk_thread_rwlock_t *
						 __rwlock);

  extern
    int (*__sctk_ptr_thread_rwlock_timedrdlock) (sctk_thread_rwlock_t *
						 __rwlock,
						 const struct timespec
						 * __abstime);
  extern
    int (*__sctk_ptr_thread_rwlock_timedwrlock) (sctk_thread_rwlock_t *
						 __rwlock,
						 const struct timespec
						 * __abstime);

  extern int (*__sctk_ptr_thread_rwlock_tryrdlock) (sctk_thread_rwlock_t
						    * __rwlock);
  extern int (*__sctk_ptr_thread_rwlock_trywrlock) (sctk_thread_rwlock_t
						    * __rwlock);
  extern int (*__sctk_ptr_thread_rwlock_unlock) (sctk_thread_rwlock_t *
						 __rwlock);
  extern int (*__sctk_ptr_thread_rwlock_wrlock) (sctk_thread_rwlock_t *
						 __rwlock);

  extern sctk_thread_t (*__sctk_ptr_thread_self) (void);
  extern sctk_thread_t (*__sctk_ptr_thread_self_check) (void);
  extern int (*__sctk_ptr_thread_setcancelstate) (int __state,
						  int *__oldstate);
  extern int (*__sctk_ptr_thread_setcanceltype) (int __type, int *__oldtype);
  extern int (*__sctk_ptr_thread_setconcurrency) (int __level);
  extern int (*__sctk_ptr_thread_setschedparam) (sctk_thread_t
						 __target_thread,
						 int __policy,
						 const struct sched_param
						 * __param);
  extern int (*__sctk_ptr_thread_setspecific) (sctk_thread_key_t __key,
					       const void *__pointer);

  extern int (*__sctk_ptr_thread_spin_destroy) (sctk_thread_spinlock_t *
						__lock);
  extern int (*__sctk_ptr_thread_spin_init) (sctk_thread_spinlock_t *
					     __lock, int __pshared);
  extern int (*__sctk_ptr_thread_spin_lock) (sctk_thread_spinlock_t * __lock);
  extern int (*__sctk_ptr_thread_spin_trylock) (sctk_thread_spinlock_t *
						__lock);
  extern int (*__sctk_ptr_thread_spin_unlock) (sctk_thread_spinlock_t *
					       __lock);

  extern void (*__sctk_ptr_thread_testcancel) (void);
  extern int (*__sctk_ptr_thread_yield) (void);
  extern int (*__sctk_ptr_thread_dump) (char *file);
  extern int (*__sctk_ptr_thread_dump_clean) (void);
  extern int (*__sctk_ptr_thread_migrate) (void);
  extern int (*__sctk_ptr_thread_restore) (sctk_thread_t thread,
					   char *type, int vp);

  extern void (*__sctk_ptr_thread_wait_for_value) (volatile int *data, int value);
  extern void (*__sctk_ptr_thread_wait_for_value_and_poll) (volatile int *data,
							    int value,
							    void (*func)
							    (void *),
							    void *arg);

  extern
    void (*__sctk_ptr_thread_freeze_thread_on_vp) (sctk_thread_mutex_t
						   * lock, void **list);
  extern void (*__sctk_ptr_thread_wake_thread_on_vp) (void **list);

  extern int (*__sctk_ptr_thread_proc_migration) (const int cpu);
  extern int (*__sctk_ptr_thread_getattr_np) (sctk_thread_t th,
					      sctk_thread_attr_t * attr);
  extern int (*__sctk_ptr_thread_rwlockattr_getkind_np) (sctk_thread_rwlockattr_t *
							 attr, int *ret);
  extern int (*__sctk_ptr_thread_rwlockattr_setkind_np) (sctk_thread_rwlockattr_t *
							 attr, int pref);
  extern void (*__sctk_ptr_thread_kill_other_threads_np) (void);

  extern int (*__sctk_ptr_thread_futex)(void *addr1, int op, int val1, 
								struct timespec *timeout, void *addr2, int val3);

#define sctk_add_func(newlib,func) __sctk_ptr_thread_##func = newlib##_##func
#define sctk_add_func_type(newlib,func,t) __sctk_ptr_thread_##func = (t)newlib##_##func
#define sctk_remove_func(func) __sctk_ptr_thread_##func = sctk_gen_thread_##func


#define sctk_thread_generic_check_size(a,b) sctk_size_checking(sizeof(a),sizeof(b),SCTK_STRING(a),SCTK_STRING(b),__FILE__,__LINE__)

#ifdef __cplusplus
}
#endif
#endif