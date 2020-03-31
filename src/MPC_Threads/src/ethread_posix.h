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
#ifndef ETHREAD_POSIX_H_
#define ETHREAD_POSIX_H_
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mpc_threads_config.h>
#include <sctk_pthread_compatible_structures.h>
#include <sctk_ethread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************
* PTHREAD ONCE *
****************/

int _mpc_thread_ethread_posix_once(mpc_thread_once_t *once_control,
                                   void (*init_routine)(void) );

/**************
* ATTRIBUTES *
**************/

int _mpc_thread_ethread_posix_attr_setdetachstate(sctk_ethread_attr_t *attr, int detachstate);
int _mpc_thread_ethread_posix_attr_getdetachstate(const sctk_ethread_attr_t *attr,
                                                  int *detachstate);
int _mpc_thread_ethread_posix_attr_setschedpolicy(sctk_ethread_attr_t *attr, int policy);
int _mpc_thread_ethread_posix_attr_getschedpolicy(const sctk_ethread_attr_t *attr,
                                                  int *policy);
int _mpc_thread_ethread_posix_attr_setinheritsched(sctk_ethread_attr_t *attr, int inherit);
int _mpc_thread_ethread_posix_attr_getinheritsched(const sctk_ethread_attr_t *attr,
                                                   int *inherit);
int _mpc_thread_ethread_posix_attr_getscope(const sctk_ethread_attr_t *attr, int *scope);
int _mpc_thread_ethread_posix_attr_setscope(sctk_ethread_attr_t *attr, int scope);
int _mpc_thread_ethread_posix_attr_getstacksize(const sctk_ethread_attr_t *attr,
                                                size_t *stacksize);
int _mpc_thread_ethread_posix_attr_getstackaddr(const sctk_ethread_attr_t *attr, void **addr);
int _mpc_thread_ethread_posix_attr_setstacksize(sctk_ethread_attr_t *attr, size_t stacksize);
int _mpc_thread_ethread_posix_attr_setstackaddr(sctk_ethread_attr_t *attr, void *addr);
int _mpc_thread_ethread_posix_attr_setstack(sctk_ethread_attr_t *attr, void *addr,
                                            size_t stacksize);
int _mpc_thread_ethread_posix_attr_getstack(sctk_ethread_attr_t *attr, void **addr,
                                            size_t *stacksize);
int _mpc_thread_ethread_posix_attr_setguardsize(sctk_ethread_attr_t *attr,
                                                size_t guardsize);
int _mpc_thread_ethread_posix_attr_getguardsize(sctk_ethread_attr_t *attr,
                                                size_t *guardsize);
int _mpc_thread_ethread_posix_attr_destroy(sctk_ethread_attr_t *attr);

/**********
* CANCEL *
**********/

void _mpc_thread_ethread_posix_testcancel(void);
int _mpc_thread_ethread_posix_cancel(sctk_ethread_t target);
int _mpc_thread_ethread_posix_setcancelstate(int state, int *oldstate);
int _mpc_thread_ethread_posix_setcanceltype(int type, int *oldtype);

/**************
* SEMAPHORES *
**************/

int _mpc_thread_ethread_posix_sem_init(sctk_ethread_sem_t *lock,
                                       int pshared, unsigned int value);
int _mpc_thread_ethread_posix_sem_wait(sctk_ethread_sem_t *lock);
int _mpc_thread_ethread_posix_sem_trywait(sctk_ethread_sem_t *lock);
int _mpc_thread_ethread_posix_sem_post(sctk_ethread_sem_t *lock,
                                       void (*return_task)(sctk_ethread_per_thread_t *) );
int _mpc_thread_ethread_posix_sem_getvalue(sctk_ethread_sem_t *sem, int *sval);
int _mpc_thread_ethread_posix_sem_destroy(sctk_ethread_sem_t *sem);

sctk_ethread_sem_t *_mpc_thread_ethread_posix_sem_open(const char *name, int flags, ...);
int _mpc_thread_ethread_posix_sem_close(sctk_ethread_sem_t *sem);
int _mpc_thread_ethread_posix_sem_unlink(const char *name);

/**********
* DETACH *
**********/

int _mpc_thread_ethread_posix_detach(sctk_ethread_t th);

/******************
* SCHED PRIORITY *
******************/

int _mpc_thread_ethread_posix_sched_get_priority_max(int policy);
int _mpc_thread_ethread_posix_sched_get_priority_min(int policy);

/**************
* MUTEX ATTR *
**************/

int _mpc_thread_ethread_posix_mutexattr_init(sctk_ethread_mutexattr_t *attr);
int _mpc_thread_ethread_posix_mutexattr_destroy(sctk_ethread_mutexattr_t *attr);
int _mpc_thread_ethread_posix_mutexattr_gettype(const sctk_ethread_mutexattr_t *attr,
                                                int *kind);
int _mpc_thread_ethread_posix_mutexattr_settype(sctk_ethread_mutexattr_t *attr, int kind);
int _mpc_thread_ethread_posix_mutexattr_getpshared(const sctk_ethread_mutexattr_t *attr,
                                                   int *pshared);
int _mpc_thread_ethread_posix_mutexattr_setpshared(sctk_ethread_mutexattr_t *attr, int pshared);

/*********
* MUTEX *
*********/

int _mpc_thread_ethread_posix_mutex_destroy(sctk_ethread_mutex_t *mutex);

/**************
* CONDITIONS *
**************/

int _mpc_thread_ethread_posix_cond_init(sctk_ethread_cond_t *cond,
                                        const sctk_ethread_condattr_t *attr);
int _mpc_thread_ethread_posix_cond_destroy(sctk_ethread_cond_t *cond);
int _mpc_thread_ethread_posix_cond_wait(sctk_ethread_cond_t *cond,
                                        sctk_ethread_mutex_t *mutex,
                                        void (*return_task)(sctk_ethread_per_thread_t *) );
int _mpc_thread_ethread_posix_cond_timedwait(sctk_ethread_cond_t *cond,
                                             sctk_ethread_mutex_t *mutex,
                                             const struct timespec *abstime,
                                             void (*return_task)(sctk_ethread_per_thread_t *) );
int _mpc_thread_ethread_posix_cond_broadcast(sctk_ethread_cond_t *cond,
                                             void (*return_task)(sctk_ethread_per_thread_t *) );
int _mpc_thread_ethread_posix_cond_signal(sctk_ethread_cond_t *cond,
                                          void (*return_task)(sctk_ethread_per_thread_t *) );

/************************
* CONDITION ATTRIBUTES *
************************/
int _mpc_thread_ethread_posix_condattr_destroy(sctk_ethread_condattr_t *attr);
int _mpc_thread_ethread_posix_condattr_init(sctk_ethread_condattr_t *attr);
int _mpc_thread_ethread_posix_condattr_getpshared(const sctk_ethread_condattr_t *attr,
                                                  int *pshared);
int _mpc_thread_ethread_posix_condattr_setpshared(sctk_ethread_condattr_t *attr, int pshared);

/*************
* SPINLOCKS *
*************/

typedef mpc_common_spinlock_t   sctk_ethread_spinlock_t;

#define SCTK_SPIN_DELAY    10

int _mpc_thread_ethread_posix_spin_init(sctk_ethread_spinlock_t *lock, int pshared);
int _mpc_thread_ethread_posix_spin_destroy(sctk_ethread_spinlock_t *lock);
int _mpc_thread_ethread_posix_spin_lock(sctk_ethread_spinlock_t *lock);
int _mpc_thread_ethread_posix_spin_trylock(sctk_ethread_spinlock_t *lock);
int _mpc_thread_ethread_posix_spin_unlock(sctk_ethread_spinlock_t *lock);

/**********
* RWLOCK *
**********/

int _mpc_thread_ethread_posix_rwlock_init(sctk_ethread_rwlock_t *rwlock,
                                          const sctk_ethread_rwlockattr_t *attr);
int _mpc_thread_ethread_posix_rwlock_destroy(sctk_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_rdlock(sctk_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_tryrdlock(sctk_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_wrlock(sctk_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_trywrlock(sctk_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_unlock(sctk_ethread_rwlock_t *rwlock,
                                            void (*return_task)(sctk_ethread_per_thread_t *) );

/**************
* RWLOCKATTR *
**************/

int _mpc_thread_ethread_posix_rwlockattr_getpshared(const sctk_ethread_rwlockattr_t *attr,
                                                    int *pshared);
int _mpc_thread_ethread_posix_rwlockattr_setpshared(sctk_ethread_rwlockattr_t *attr,
                                                    int pshared);
int _mpc_thread_ethread_posix_rwlockattr_init(sctk_ethread_rwlockattr_t *attr);
int _mpc_thread_ethread_posix_rwlockattr_destroy(sctk_ethread_rwlockattr_t *attr);

/***********
* BARRIER *
***********/
int _mpc_thread_ethread_posix_barrier_init(sctk_ethread_barrier_t *barrier,
                                           const sctk_ethread_barrierattr_t *attr,
                                           unsigned count);
int _mpc_thread_ethread_posix_barrier_destroy(sctk_ethread_barrier_t *barrier);
int _mpc_thread_ethread_posix_barrier_wait(sctk_ethread_barrier_t *barrier,
                                           void (*return_task)(sctk_ethread_per_thread_t *) );

/***************
* BARRIERATTR *
***************/

int _mpc_thread_ethread_posix_barrierattr_getpshared(const sctk_ethread_barrierattr_t *
                                                     attr, int *pshared);
int _mpc_thread_ethread_posix_barrierattr_setpshared(sctk_ethread_barrierattr_t *attr,
                                                     int pshared);
int _mpc_thread_ethread_posix_barrierattr_init(sctk_ethread_barrierattr_t *attr);
int _mpc_thread_ethread_posix_barrierattr_destroy(sctk_ethread_barrierattr_t *attr);

/****************
* NON PORTABLE *
****************/

int _mpc_thread_ethread_posix_getattr_np(sctk_ethread_t th, sctk_ethread_attr_t *attr);

#ifdef __cplusplus
}
#endif

#endif /* ETHREAD_POSIX_H_ */
