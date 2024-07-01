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
#ifndef THREAD_PTR_H_
#define THREAD_PTR_H_

#include <stdlib.h>
#include <signal.h>

#include "mpc_thread.h"

struct sched_param;

extern double (*_funcptr_mpc_thread_get_activity) (int i);

extern int (*_funcptr_mpc_thread_atfork) (void (*__prepare)(void),
                                          void (*__parent)(void),
                                          void (*__child)(void) );
extern int (*_funcptr_mpc_thread_attr_destroy) (mpc_thread_attr_t *__attr);
extern int (*_funcptr_mpc_thread_attr_getdetachstate) (const
                                                       mpc_thread_attr_t
                                                       *__attr,
                                                       int *__detachstate);
extern int (*_funcptr_mpc_thread_attr_getguardsize) (const
                                                     mpc_thread_attr_t *
                                                     __attr,
                                                     size_t *__guardsize);
extern int (*_funcptr_mpc_thread_attr_getinheritsched) (const
                                                        mpc_thread_attr_t
                                                        *__attr,
                                                        int *__inherit);
extern int (*_funcptr_mpc_thread_attr_getschedparam) (const
                                                      mpc_thread_attr_t
                                                      *__attr,
                                                      struct sched_param
                                                      *__param);
extern int (*_funcptr_mpc_thread_attr_getschedpolicy) (const
                                                       mpc_thread_attr_t
                                                       *__attr,
                                                       int *__policy);
extern int ( *_funcptr_mpc_thread_setaffinity_np)(mpc_thread_t,
                                                 size_t,
                                                 const mpc_cpu_set_t *);
extern int ( *_funcptr_mpc_thread_getaffinity_np)(mpc_thread_t,
                                                 size_t,
                                                 mpc_cpu_set_t *);
extern int (*_funcptr_mpc_thread_attr_getscope) (const mpc_thread_attr_t
                                                 *__attr, int *__scope);
extern int (*_funcptr_mpc_thread_attr_getstackaddr) (const
                                                     mpc_thread_attr_t *
                                                     __attr,
                                                     void **__stackaddr);
extern int (*_funcptr_mpc_thread_attr_getstack) (const mpc_thread_attr_t
                                                 *__attr,
                                                 void **__stackaddr,
                                                 size_t *__stacksize);
extern int (*_funcptr_mpc_thread_attr_getstacksize) (const
                                                     mpc_thread_attr_t *
                                                     __attr,
                                                     size_t *__stacksize);
extern int (*_funcptr_mpc_thread_attr_init) (mpc_thread_attr_t *__attr);
extern int (*_funcptr_mpc_thread_attr_setdetachstate) (mpc_thread_attr_t
                                                       *__attr,
                                                       int __detachstate);
extern int (*_funcptr_mpc_thread_attr_setguardsize) (mpc_thread_attr_t *
                                                     __attr,
                                                     size_t __guardsize);
extern int (*_funcptr_mpc_thread_attr_setinheritsched) (mpc_thread_attr_t *
                                                        __attr, int __inherit);
extern int (*_funcptr_mpc_thread_attr_setschedparam) (mpc_thread_attr_t
                                                      *__attr,
                                                      const struct
                                                      sched_param *__param);
extern int (*_funcptr_mpc_thread_sched_get_priority_max) (int policy);
extern int (*_funcptr_mpc_thread_sched_get_priority_min) (int policy);

extern int (*_funcptr_mpc_thread_attr_setschedpolicy) (mpc_thread_attr_t
                                                       *__attr,
                                                       int __policy);
extern int (*_funcptr_mpc_thread_attr_setscope) (mpc_thread_attr_t *
                                                 __attr, int __scope);
extern int (*_funcptr_mpc_thread_attr_setstackaddr) (mpc_thread_attr_t *
                                                     __attr,
                                                     void *__stackaddr);
extern int (*_funcptr_mpc_thread_attr_setstack) (mpc_thread_attr_t *
                                                 __attr,
                                                 void *__stackaddr,
                                                 size_t __stacksize);
extern int (*_funcptr_mpc_thread_attr_setstacksize) (mpc_thread_attr_t *
                                                     __attr,
                                                     size_t __stacksize);

extern int
(*_funcptr_mpc_thread_attr_setbinding) (mpc_thread_attr_t *__attr,
                                        int __binding);
extern int
(*_funcptr_mpc_thread_attr_getbinding) (mpc_thread_attr_t *__attr,
                                        int *__binding);

extern
int (*_funcptr_mpc_thread_barrierattr_destroy) (mpc_thread_barrierattr_t
                                                *__attr);
extern
int (*_funcptr_mpc_thread_barrierattr_init)
(mpc_thread_barrierattr_t * __attr);
extern
int (*_funcptr_mpc_thread_barrierattr_setpshared)
(mpc_thread_barrierattr_t * __attr, int __pshared);
extern
int (*_funcptr_mpc_thread_barrierattr_getpshared) (const
                                                   mpc_thread_barrierattr_t
                                                   *__attr,
                                                   int *__pshared);
extern int (*_funcptr_mpc_thread_barrier_destroy) (mpc_thread_barrier_t
                                                   *__barrier);
extern int (*_funcptr_mpc_thread_barrier_init) (mpc_thread_barrier_t *
                                                restrict __barrier,
                                                const
                                                mpc_thread_barrierattr_t
                                                *restrict __attr,
                                                unsigned int __count);
extern int (*_funcptr_mpc_thread_barrier_wait) (mpc_thread_barrier_t *
                                                __barrier);

extern int (*_funcptr_mpc_thread_cancel) (mpc_thread_t __cancelthread);
extern
int (*_funcptr_mpc_thread_condattr_destroy) (mpc_thread_condattr_t *
                                             __attr);
extern int (*_funcptr_mpc_thread_condattr_getpshared) (const
                                                       mpc_thread_condattr_t
                                                       *__attr,
                                                       int *__pshared);
extern int (*_funcptr_mpc_thread_condattr_init) (mpc_thread_condattr_t *
                                                 __attr);
extern
int (*_funcptr_mpc_thread_condattr_setpshared)
(mpc_thread_condattr_t * __attr, int __pshared);
extern
int (*_funcptr_mpc_thread_condattr_setclock) (mpc_thread_condattr_t
                                              *attr, clockid_t clock_id);
extern
int (*_funcptr_mpc_thread_condattr_getclock) (mpc_thread_condattr_t
                                              *attr, clockid_t *clock_id);
extern int (*_funcptr_mpc_thread_cond_broadcast) (mpc_thread_cond_t *
                                                  __cond);
extern int (*_funcptr_mpc_thread_cond_destroy) (mpc_thread_cond_t *__cond);
extern int (*_funcptr_mpc_thread_cond_init) (mpc_thread_cond_t *__cond,
                                             const mpc_thread_condattr_t
                                             *__cond_attr);
extern int (*_funcptr_mpc_thread_cond_signal) (mpc_thread_cond_t *__cond);
extern int (*_funcptr_mpc_thread_cond_timedwait) (mpc_thread_cond_t *
                                                  __cond,
                                                  mpc_thread_mutex_t *
                                                  __mutex,
                                                  const struct timespec *
                                                  __abstime);
extern int (*_funcptr_mpc_thread_cond_clockwait) (mpc_thread_cond_t * __cond,
                                                  mpc_thread_mutex_t * __mutex,
                                                  clockid_t __clock_id,
                                                  const struct timespec * __abstime);
extern int (*_funcptr_mpc_thread_cond_wait) (mpc_thread_cond_t *restrict __cond,
                                             mpc_thread_mutex_t *restrict __mutex);
extern int (*_funcptr_mpc_thread_create) (mpc_thread_t *__threadp,
                                          const mpc_thread_attr_t *
                                          __attr,
                                          void *(*__start_routine)(void
                                                                   *),
                                          void *__arg);
extern int (*_funcptr_mpc_thread_user_create) (mpc_thread_t *__threadp,
                                               const mpc_thread_attr_t *
                                               __attr,
                                               void
                                               *(*__start_routine)(void
                                                                   *),
                                               void *__arg);
extern int (*_funcptr_mpc_thread_detach) (mpc_thread_t __th);
extern int (*_funcptr_mpc_thread_equal) (mpc_thread_t __thread1,
                                         mpc_thread_t __thread2);
extern void (*_funcptr_mpc_thread_exit) (void *__retval);
extern int  (*_funcptr_mpc_thread_getconcurrency) (void);
extern int  (*_funcptr_mpc_thread_setschedprio) (mpc_thread_t __p, int __i);
extern int  (*_funcptr_mpc_thread_getcpuclockid) (mpc_thread_t
                                                  __thread_id,
                                                  clockid_t *__clock_id);
extern int (*_funcptr_mpc_thread_getschedparam) (mpc_thread_t
                                                 __target_thread,
                                                 int *__policy,
                                                 struct sched_param *
                                                 __param);
extern void *(*_funcptr_mpc_thread_getspecific) (mpc_thread_keys_t __key);
extern int   (*_funcptr_mpc_thread_join) (mpc_thread_t __th,
                                          void **__thread_return);
extern int (*_funcptr_mpc_thread_kill) (mpc_thread_t thread, int signo);
extern int (*_funcptr_mpc_thread_sigsuspend) (sigset_t *set);
extern int (*_funcptr_mpc_thread_sigpending) (sigset_t *set);
extern int (*_funcptr_mpc_thread_sigmask) (int how,
                                           const sigset_t *newmask,
                                           sigset_t *oldmask);
extern int (*_funcptr_mpc_thread_key_create) (mpc_thread_keys_t *__key,
                                              void (*__destr_function)
                                              (void *) );
extern int (*_funcptr_mpc_thread_key_delete) (mpc_thread_keys_t __key);
extern
int (*_funcptr_mpc_thread_mutexattr_destroy) (mpc_thread_mutexattr_t
                                              *__attr);

extern
int (*_funcptr_mpc_thread_mutexattr_getprioceiling) (const
                                                     mpc_thread_mutexattr_t
                                                     *__attr,
                                                     int *__prioceiling);
extern
int (*_funcptr_mpc_thread_mutexattr_setprioceiling)
(mpc_thread_mutexattr_t * __attr, int __prioceiling);

extern
int (*_funcptr_mpc_thread_mutexattr_getprotocol) (const
                                                  mpc_thread_mutexattr_t
                                                  *__attr,
                                                  int *__protocol);
extern
int (*_funcptr_mpc_thread_mutexattr_setprotocol) (mpc_thread_mutexattr_t
                                                  *__attr, int __protocol);

extern
int (*_funcptr_mpc_thread_mutexattr_getpshared) (const
                                                 mpc_thread_mutexattr_t
                                                 *__attr, int *__pshared);
extern int (*_funcptr_mpc_thread_mutexattr_gettype) (const
                                                     mpc_thread_mutexattr_t
                                                     *__attr, int *__kind);
extern int (*_funcptr_mpc_thread_mutexattr_init) (mpc_thread_mutexattr_t
                                                  *__attr);
extern
int (*_funcptr_mpc_thread_mutexattr_setpshared) (mpc_thread_mutexattr_t
                                                 *__attr, int __pshared);
extern
int (*_funcptr_mpc_thread_mutexattr_settype) (mpc_thread_mutexattr_t
                                              *__attr, int __kind);
extern int (*_funcptr_mpc_thread_mutex_destroy) (mpc_thread_mutex_t *
                                                 __mutex);
extern int (*_funcptr_mpc_thread_mutex_init) (mpc_thread_mutex_t *
                                              __mutex,
                                              const
                                              mpc_thread_mutexattr_t *
                                              __mutex_attr);
extern int (*_funcptr_mpc_thread_mutex_lock) (mpc_thread_mutex_t *__mutex);
extern int (*_funcptr_mpc_thread_mutex_spinlock) (mpc_thread_mutex_t *
                                                  __mutex);
extern int (*_funcptr_mpc_thread_mutex_timedlock) (mpc_thread_mutex_t *
                                                   __mutex,
                                                   const struct timespec
                                                   *__abstime);
extern int (*_funcptr_mpc_thread_mutex_clocklock) (mpc_thread_mutex_t *__mutex,
                                                   clockid_t __clock_id,
                                                   const struct timespec *__abstime);
extern int (*_funcptr_mpc_thread_mutex_trylock) (mpc_thread_mutex_t *
                                                 __mutex);
extern int (*_funcptr_mpc_thread_mutex_unlock) (mpc_thread_mutex_t *
                                                __mutex);
extern int (*_funcptr_mpc_thread_sem_init) (mpc_thread_sem_t *sem,
                                            int pshared, unsigned int value);
extern int (*_funcptr_mpc_thread_sem_wait) (mpc_thread_sem_t *sem);
extern int (*_funcptr_mpc_thread_sem_trywait) (mpc_thread_sem_t *sem);
extern int (*_funcptr_mpc_thread_sem_timedwait) (mpc_thread_sem_t *sem,
                                                 const struct timespec *__abstime);
extern int (*_funcptr_mpc_thread_sem_post) (mpc_thread_sem_t *sem);
extern int (*_funcptr_mpc_thread_sem_getvalue) (mpc_thread_sem_t *sem,
                                                int *sval);
extern int (*_funcptr_mpc_thread_sem_destroy) (mpc_thread_sem_t *sem);
extern mpc_thread_sem_t *(*_funcptr_mpc_thread_sem_open) (const char
                                                          *name,
                                                          int oflag, ...);
extern int (*_funcptr_mpc_thread_sem_close) (mpc_thread_sem_t *sem);
extern int (*_funcptr_mpc_thread_sem_unlink) (const char *name);

extern int (*_funcptr_mpc_thread_once) (mpc_thread_once_t *
                                        __once_control,
                                        void (*__init_routine)(void) );

extern
int (*_funcptr_mpc_thread_rwlockattr_destroy)
(mpc_thread_rwlockattr_t * __attr);
extern
int (*_funcptr_mpc_thread_rwlockattr_getpshared) (const
                                                  mpc_thread_rwlockattr_t
                                                  *restrict __attr,
                                                  int *restrict __pshared);
extern
int (*_funcptr_mpc_thread_rwlockattr_init) (mpc_thread_rwlockattr_t
                                            *__attr);
extern
int (*_funcptr_mpc_thread_rwlockattr_setpshared)
(mpc_thread_rwlockattr_t * __attr, int __pshared);
extern int (*_funcptr_mpc_thread_rwlock_destroy) (mpc_thread_rwlock_t *
                                                  __rwlock);
extern int (*_funcptr_mpc_thread_rwlock_init) (mpc_thread_rwlock_t *
                                               restrict __rwlock,
                                               const
                                               mpc_thread_rwlockattr_t *
                                               restrict __attr);
extern int (*_funcptr_mpc_thread_rwlock_rdlock) (mpc_thread_rwlock_t *
                                                 __rwlock);

extern
int (*_funcptr_mpc_thread_rwlock_timedrdlock) (mpc_thread_rwlock_t *
                                               __rwlock,
                                               const struct timespec
                                               *__abstime);
extern
int (*_funcptr_mpc_thread_rwlock_timedwrlock) (mpc_thread_rwlock_t *
                                               __rwlock,
                                               const struct timespec
                                               *__abstime);
extern
int (*_funcptr_mpc_thread_rwlock_clockrdlock) (mpc_thread_rwlock_t *__rwlock,
                                               clockid_t __clock_id,
                                               const struct timespec *__abstime);
extern
int (*_funcptr_mpc_thread_rwlock_clockwrlock) (mpc_thread_rwlock_t * __rwlock,
                                               clockid_t __clock_id,
                                               const struct timespec *__abstime);

extern int (*_funcptr_mpc_thread_rwlock_tryrdlock) (mpc_thread_rwlock_t
                                                    *__rwlock);
extern int (*_funcptr_mpc_thread_rwlock_trywrlock) (mpc_thread_rwlock_t
                                                    *__rwlock);
extern int (*_funcptr_mpc_thread_rwlock_unlock) (mpc_thread_rwlock_t *
                                                 __rwlock);
extern int (*_funcptr_mpc_thread_rwlock_wrlock) (mpc_thread_rwlock_t *
                                                 __rwlock);

extern mpc_thread_t (*_funcptr_mpc_thread_self) (void);

extern int (*_funcptr_mpc_thread_setcancelstate) (int __state,
                                                  int *__oldstate);
extern int (*_funcptr_mpc_thread_setcanceltype) (int __type, int *__oldtype);
extern int (*_funcptr_mpc_thread_setconcurrency) (int __level);
extern int (*_funcptr_mpc_thread_setschedparam) (mpc_thread_t
                                                 __target_thread,
                                                 int __policy,
                                                 const struct sched_param
                                                 *__param);
extern int (*_funcptr_mpc_thread_setspecific) (mpc_thread_keys_t __key,
                                               const void *__pointer);

extern int (*_funcptr_mpc_thread_spin_destroy) (mpc_thread_spinlock_t *
                                                __lock);
extern int (*_funcptr_mpc_thread_spin_init) (mpc_thread_spinlock_t *
                                             __lock, int __pshared);
extern int (*_funcptr_mpc_thread_spin_lock) (mpc_thread_spinlock_t *__lock);
extern int (*_funcptr_mpc_thread_spin_trylock) (mpc_thread_spinlock_t *
                                                __lock);
extern int (*_funcptr_mpc_thread_spin_unlock) (mpc_thread_spinlock_t *
                                               __lock);

extern void (*_funcptr_mpc_thread_testcancel) (void);
extern int  (*_funcptr_mpc_thread_yield) (void);
extern int  (*_funcptr_mpc_thread_dump) (char *file);
extern int  (*_funcptr_mpc_thread_dump_clean) (void);
extern int  (*_funcptr_mpc_thread_migrate) (void);
extern int  (*_funcptr_mpc_thread_restore) (mpc_thread_t thread,
                                            char *type, int vp);

extern void (*_funcptr_mpc_thread_wait_for_value) (volatile int *data, int value);
extern void (*_funcptr_mpc_thread_wait_for_value_and_poll) (volatile int *data,
                                                            int value,
                                                            void (*func)
                                                            (void *),
                                                            void *arg);

extern
void (*_funcptr_mpc_thread_freeze_thread_on_vp) (mpc_thread_mutex_t
                                                 *lock, void **list);
extern void (*_funcptr_mpc_thread_wake_thread_on_vp) (void **list);

extern int (*_funcptr_mpc_thread_proc_migration) (const int cpu);
extern int (*_funcptr_mpc_thread_getattr_np) (mpc_thread_t th,
                                              mpc_thread_attr_t *attr);
extern int (*_funcptr_mpc_thread_rwlockattr_getkind_np) (mpc_thread_rwlockattr_t *
                                                         attr, int *ret);
extern int (*_funcptr_mpc_thread_rwlockattr_setkind_np) (mpc_thread_rwlockattr_t *
                                                         attr, int pref);
extern void (*_funcptr_mpc_thread_kill_other_threads_np) (void);

extern int (*_funcptr_mpc_thread_futex)(void *addr1, int op, int val1,
                                        struct timespec *timeout, void *addr2, int val3);

#define sctk_add_func(newlib, func)                _funcptr_mpc_thread_ ## func = newlib ## _ ## func
#define sctk_add_func_type(newlib, func, t)        _funcptr_mpc_thread_ ## func = (t)newlib ## _ ## func
#define sctk_remove_func(func)                     _funcptr_mpc_thread_ ## func = sctk_gen_thread_ ## func

#endif /* THREAD_PTR_H_ */
