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
#include <ethread_pthread_struct.h>
#include <ethread_engine.h>

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

int _mpc_thread_ethread_posix_attr_setdetachstate(_mpc_thread_ethread_attr_t *attr, int detachstate);
int _mpc_thread_ethread_posix_attr_getdetachstate(const _mpc_thread_ethread_attr_t *attr,
                                                  int *detachstate);
int _mpc_thread_ethread_posix_attr_setschedpolicy(_mpc_thread_ethread_attr_t *attr, int policy);
int _mpc_thread_ethread_posix_attr_getschedpolicy(const _mpc_thread_ethread_attr_t *attr,
                                                  int *policy);
int _mpc_thread_ethread_posix_attr_setinheritsched(_mpc_thread_ethread_attr_t *attr, int inherit);
int _mpc_thread_ethread_posix_attr_getinheritsched(const _mpc_thread_ethread_attr_t *attr,
                                                   int *inherit);
int _mpc_thread_ethread_posix_attr_getscope(const _mpc_thread_ethread_attr_t *attr, int *scope);
int _mpc_thread_ethread_posix_attr_setscope(_mpc_thread_ethread_attr_t *attr, int scope);
int _mpc_thread_ethread_posix_attr_getstacksize(const _mpc_thread_ethread_attr_t *attr,
                                                size_t *stacksize);
int _mpc_thread_ethread_posix_attr_getstackaddr(const _mpc_thread_ethread_attr_t *attr, void **addr);
int _mpc_thread_ethread_posix_attr_setstacksize(_mpc_thread_ethread_attr_t *attr, size_t stacksize);
int _mpc_thread_ethread_posix_attr_setstackaddr(_mpc_thread_ethread_attr_t *attr, void *addr);
int _mpc_thread_ethread_posix_attr_setstack(_mpc_thread_ethread_attr_t *attr, void *addr,
                                            size_t stacksize);
int _mpc_thread_ethread_posix_attr_getstack(_mpc_thread_ethread_attr_t *attr, void **addr,
                                            size_t *stacksize);
int _mpc_thread_ethread_posix_attr_setguardsize(_mpc_thread_ethread_attr_t *attr,
                                                size_t guardsize);
int _mpc_thread_ethread_posix_attr_getguardsize(_mpc_thread_ethread_attr_t *attr,
                                                size_t *guardsize);
int _mpc_thread_ethread_posix_attr_destroy(_mpc_thread_ethread_attr_t *attr);

/**********
* CANCEL *
**********/

void _mpc_thread_ethread_posix_testcancel(void);
int _mpc_thread_ethread_posix_cancel(_mpc_thread_ethread_t target);
int _mpc_thread_ethread_posix_setcancelstate(int state, int *oldstate);
int _mpc_thread_ethread_posix_setcanceltype(int type, int *oldtype);

/**************
* SEMAPHORES *
**************/

int _mpc_thread_ethread_posix_sem_init(_mpc_thread_ethread_sem_t *lock,
                                       int pshared, unsigned int value);
int _mpc_thread_ethread_posix_sem_wait(_mpc_thread_ethread_sem_t *lock);
int _mpc_thread_ethread_posix_sem_trywait(_mpc_thread_ethread_sem_t *lock);
int _mpc_thread_ethread_posix_sem_post(_mpc_thread_ethread_sem_t *lock,
                                       void (*return_task)(_mpc_thread_ethread_per_thread_t *) );
int _mpc_thread_ethread_posix_sem_getvalue(_mpc_thread_ethread_sem_t *sem, int *sval);
int _mpc_thread_ethread_posix_sem_destroy(_mpc_thread_ethread_sem_t *sem);

_mpc_thread_ethread_sem_t *_mpc_thread_ethread_posix_sem_open(const char *name, int flags, ...);
int _mpc_thread_ethread_posix_sem_close(_mpc_thread_ethread_sem_t *sem);
int _mpc_thread_ethread_posix_sem_unlink(const char *name);

/**********
* DETACH *
**********/

int _mpc_thread_ethread_posix_detach(_mpc_thread_ethread_t th);

/******************
* SCHED PRIORITY *
******************/

int _mpc_thread_ethread_posix_sched_get_priority_max(int policy);
int _mpc_thread_ethread_posix_sched_get_priority_min(int policy);

/**************
* MUTEX ATTR *
**************/

int _mpc_thread_ethread_posix_mutexattr_init(_mpc_thread_ethread_mutexattr_t *attr);
int _mpc_thread_ethread_posix_mutexattr_destroy(_mpc_thread_ethread_mutexattr_t *attr);
int _mpc_thread_ethread_posix_mutexattr_gettype(const _mpc_thread_ethread_mutexattr_t *attr,
                                                int *kind);
int _mpc_thread_ethread_posix_mutexattr_settype(_mpc_thread_ethread_mutexattr_t *attr, int kind);
int _mpc_thread_ethread_posix_mutexattr_getpshared(const _mpc_thread_ethread_mutexattr_t *attr,
                                                   int *pshared);
int _mpc_thread_ethread_posix_mutexattr_setpshared(_mpc_thread_ethread_mutexattr_t *attr, int pshared);

/*********
* MUTEX *
*********/

int _mpc_thread_ethread_posix_mutex_destroy(_mpc_thread_ethread_mutex_t *mutex);

/**************
* CONDITIONS *
**************/

int _mpc_thread_ethread_posix_cond_init(_mpc_thread_ethread_cond_t *cond,
                                        const _mpc_thread_ethread_condattr_t *attr);
int _mpc_thread_ethread_posix_cond_destroy(_mpc_thread_ethread_cond_t *cond);
int _mpc_thread_ethread_posix_cond_wait(_mpc_thread_ethread_cond_t *cond,
                                        _mpc_thread_ethread_mutex_t *mutex,
                                        void (*return_task)(_mpc_thread_ethread_per_thread_t *) );
int _mpc_thread_ethread_posix_cond_timedwait(_mpc_thread_ethread_cond_t *cond,
                                             _mpc_thread_ethread_mutex_t *mutex,
                                             const struct timespec *abstime,
                                             void (*return_task)(_mpc_thread_ethread_per_thread_t *) );
int _mpc_thread_ethread_posix_cond_broadcast(_mpc_thread_ethread_cond_t *cond,
                                             void (*return_task)(_mpc_thread_ethread_per_thread_t *) );
int _mpc_thread_ethread_posix_cond_signal(_mpc_thread_ethread_cond_t *cond,
                                          void (*return_task)(_mpc_thread_ethread_per_thread_t *) );

/************************
* CONDITION ATTRIBUTES *
************************/
int _mpc_thread_ethread_posix_condattr_destroy(_mpc_thread_ethread_condattr_t *attr);
int _mpc_thread_ethread_posix_condattr_init(_mpc_thread_ethread_condattr_t *attr);
int _mpc_thread_ethread_posix_condattr_getpshared(const _mpc_thread_ethread_condattr_t *attr,
                                                  int *pshared);
int _mpc_thread_ethread_posix_condattr_setpshared(_mpc_thread_ethread_condattr_t *attr, int pshared);

/*************
* SPINLOCKS *
*************/

typedef mpc_common_spinlock_t   _mpc_thread_ethread_spinlock_t;

#define SCTK_SPIN_DELAY    10

int _mpc_thread_ethread_posix_spin_init(_mpc_thread_ethread_spinlock_t *lock, int pshared);
int _mpc_thread_ethread_posix_spin_destroy(_mpc_thread_ethread_spinlock_t *lock);
int _mpc_thread_ethread_posix_spin_lock(_mpc_thread_ethread_spinlock_t *lock);
int _mpc_thread_ethread_posix_spin_trylock(_mpc_thread_ethread_spinlock_t *lock);
int _mpc_thread_ethread_posix_spin_unlock(_mpc_thread_ethread_spinlock_t *lock);

/**********
* RWLOCK *
**********/

int _mpc_thread_ethread_posix_rwlock_init(_mpc_thread_ethread_rwlock_t *rwlock,
                                          const _mpc_thread_ethread_rwlockattr_t *attr);
int _mpc_thread_ethread_posix_rwlock_destroy(_mpc_thread_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_rdlock(_mpc_thread_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_tryrdlock(_mpc_thread_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_wrlock(_mpc_thread_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_trywrlock(_mpc_thread_ethread_rwlock_t *rwlock);
int _mpc_thread_ethread_posix_rwlock_unlock(_mpc_thread_ethread_rwlock_t *rwlock,
                                            void (*return_task)(_mpc_thread_ethread_per_thread_t *) );

/**************
* RWLOCKATTR *
**************/

int _mpc_thread_ethread_posix_rwlockattr_getpshared(const _mpc_thread_ethread_rwlockattr_t *attr,
                                                    int *pshared);
int _mpc_thread_ethread_posix_rwlockattr_setpshared(_mpc_thread_ethread_rwlockattr_t *attr,
                                                    int pshared);
int _mpc_thread_ethread_posix_rwlockattr_init(_mpc_thread_ethread_rwlockattr_t *attr);
int _mpc_thread_ethread_posix_rwlockattr_destroy(_mpc_thread_ethread_rwlockattr_t *attr);

/***********
* BARRIER *
***********/
int _mpc_thread_ethread_posix_barrier_init(_mpc_thread_ethread_barrier_t *barrier,
                                           const _mpc_thread_ethread_barrierattr_t *attr,
                                           unsigned count);
int _mpc_thread_ethread_posix_barrier_destroy(_mpc_thread_ethread_barrier_t *barrier);
int _mpc_thread_ethread_posix_barrier_wait(_mpc_thread_ethread_barrier_t *barrier,
                                           void (*return_task)(_mpc_thread_ethread_per_thread_t *) );

/***************
* BARRIERATTR *
***************/

int _mpc_thread_ethread_posix_barrierattr_getpshared(const _mpc_thread_ethread_barrierattr_t *
                                                     attr, int *pshared);
int _mpc_thread_ethread_posix_barrierattr_setpshared(_mpc_thread_ethread_barrierattr_t *attr,
                                                     int pshared);
int _mpc_thread_ethread_posix_barrierattr_init(_mpc_thread_ethread_barrierattr_t *attr);
int _mpc_thread_ethread_posix_barrierattr_destroy(_mpc_thread_ethread_barrierattr_t *attr);

/****************
* NON PORTABLE *
****************/

int _mpc_thread_ethread_posix_getattr_np(_mpc_thread_ethread_t th, _mpc_thread_ethread_attr_t *attr);

#ifdef __cplusplus
}
#endif

#endif /* ETHREAD_POSIX_H_ */
