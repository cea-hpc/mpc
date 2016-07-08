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
#ifndef __MPC_PTHREAD_API_H_
#define __MPC_PTHREAD_API_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <mpc.h>
#include <mpcthread.h>
#include <sched.h>
/* #include <pthread.h>  */

#define sem_t mpc_thread_sem_t
#define pthread_t mpc_thread_t
#define pthread_attr_t mpc_thread_attr_t
#define pthread_condattr_t mpc_thread_condattr_t
#define pthread_cond_t mpc_thread_cond_t
#define pthread_mutex_t mpc_thread_mutex_t
#define pthread_mutexattr_t mpc_thread_mutexattr_t
#define pthread_key_t mpc_thread_key_t
#define pthread_once_t mpc_thread_once_t

#define pthread_barrierattr_t mpc_thread_barrierattr_t
#define pthread_barrier_t mpc_thread_barrier_t
#define pthread_spinlock_t mpc_thread_spinlock_t

#define pthread_rwlockattr_t mpc_thread_rwlockattr_t
#define pthread_rwlock_t mpc_thread_rwlock_t


#define PTHREAD_RWLOCK_INITIALIZER MPC_THREAD_RWLOCK_INITIALIZER

#define PTHREAD_MUTEX_NORMAL MPC_THREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_RECURSIVE MPC_THREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_ERRORCHECK MPC_THREAD_MUTEX_ERRORCHECK
#define PTHREAD_MUTEX_DEFAULT MPC_THREAD_MUTEX_DEFAULT

#define PTHREAD_BARRIER_SERIAL_THREAD MPC_THREAD_BARRIER_SERIAL_THREAD

#define PTHREAD_MUTEX_INITIALIZER MPC_THREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_RECURSIVE_INITIALIZER MPC_THREAD_MUTEX_RECURSIVE_INITIALIZER
#define PTHREAD_COND_INITIALIZER MPC_THREAD_COND_INITIALIZER
#define PTHREAD_ONCE_INIT MPC_THREAD_ONCE_INIT
#undef PTHREAD_KEYS_MAX
#define PTHREAD_KEYS_MAX MPC_THREAD_KEYS_MAX
#define PTHREAD_CREATE_JOINABLE MPC_THREAD_CREATE_JOINABLE
#define PTHREAD_CREATE_DETACHED MPC_THREAD_CREATE_DETACHED
#define PTHREAD_INHERIT_SCHED MPC_THREAD_INHERIT_SCHED
#define PTHREAD_EXPLICIT_SCHED MPC_THREAD_EXPLICIT_SCHED
#define PTHREAD_SCOPE_SYSTEM MPC_THREAD_SCOPE_SYSTEM
#define PTHREAD_SCOPE_PROCESS MPC_THREAD_SCOPE_PROCESS
#define PTHREAD_PROCESS_PRIVATE MPC_THREAD_PROCESS_PRIVATE
#define PTHREAD_PROCESS_SHARED MPC_THREAD_PROCESS_SHARED
#define PTHREAD_CANCEL_ENABLE MPC_THREAD_CANCEL_ENABLE
#define PTHREAD_CANCEL_DISABLE MPC_THREAD_CANCEL_DISABLE
#define PTHREAD_CANCEL_DEFERRED MPC_THREAD_CANCEL_DEFERRED
#define PTHREAD_CANCEL_ASYNCHRONOUS MPC_THREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_CANCELED MPC_THREAD_CANCELED

#define SEM_VALUE_MAX SCTK_SEM_VALUE_MAX
#define SEM_FAILED SCTK_SEM_FAILED


#define PTHREAD_PRIO_NONE MPC_THREAD_PRIO_NONE
#define PTHREAD_PRIO_INHERIT MPC_THREAD_PRIO_INHERIT
#define PTHREAD_PRIO_PROTECT MPC_THREAD_PRIO_PROTECT

/* pthread_atfork */
#define pthread_atfork mpc_thread_atfork

/* pthread_attr_destroy */
#define pthread_attr_destroy mpc_thread_attr_destroy

/* pthread_attr_getdetachstate */
#define pthread_attr_getdetachstate mpc_thread_attr_getdetachstate

/* pthread_attr_getguardsize */
#define pthread_attr_getguardsize mpc_thread_attr_getguardsize

/* pthread_attr_getinheritsched */
#define pthread_attr_getinheritsched mpc_thread_attr_getinheritsched

/* pthread_attr_getschedparam */
#define pthread_attr_getschedparam mpc_thread_attr_getschedparam

#if 0
/* sched_get_priority_max*/
#undef sched_get_priority_max
#define sched_get_priority_max(a) mpc_thread_sched_get_priority_max (a)


/* sched_get_priority_min*/
#undef sched_get_priority_min
#define sched_get_priority_min(a) mpc_thread_sched_get_priority_min(a)
#endif

/* pthread_attr_getschedpolicy */
#define pthread_attr_getschedpolicy mpc_thread_attr_getschedpolicy

/* pthread_attr_getscope */
#define pthread_attr_getscope mpc_thread_attr_getscope

/* pthread_attr_getstack */
#define pthread_attr_getstack mpc_thread_attr_getstack

/* pthread_attr_getstackaddr */
#define pthread_attr_getstackaddr mpc_thread_attr_getstackaddr

/* pthread_attr_getstacksize */
#define pthread_attr_getstacksize mpc_thread_attr_getstacksize

/* pthread_attr_init */
#define pthread_attr_init mpc_thread_attr_init


/* pthread_attr_setdetachstate */
#define pthread_attr_setdetachstate mpc_thread_attr_setdetachstate


/* pthread_attr_setguardsize */
#define pthread_attr_setguardsize mpc_thread_attr_setguardsize


/* pthread_attr_setinheritsched */
#define pthread_attr_setinheritsched mpc_thread_attr_setinheritsched


/* pthread_attr_setschedparam */
#define pthread_attr_setschedparam mpc_thread_attr_setschedparam

/* pthread_attr_setschedpolicy */
#define pthread_attr_setschedpolicy mpc_thread_attr_setschedpolicy


/* pthread_attr_setscope */
#define pthread_attr_setscope mpc_thread_attr_setscope


/* pthread_attr_setstack */
#define pthread_attr_setstack mpc_thread_attr_setstack


/* pthread_attr_setstackaddr */
#define pthread_attr_setstackaddr mpc_thread_attr_setstackaddr


/* pthread_attr_setstacksize */
#define pthread_attr_setstacksize mpc_thread_attr_setstacksize


/* pthread_barrier_destroy */
#define pthread_barrier_destroy mpc_thread_barrier_destroy


/* pthread_barrier_init */
#define pthread_barrier_init mpc_thread_barrier_init

/* pthread_barrier_wait */
#define pthread_barrier_wait mpc_thread_barrier_wait


/* pthread_barrierattr_destroy */
#define pthread_barrierattr_destroy mpc_thread_barrierattr_destroy


/* pthread_barrierattr_getpshared */
#define pthread_barrierattr_getpshared mpc_thread_barrierattr_getpshared

/* pthread_barrierattr_init */
#define pthread_barrierattr_init mpc_thread_barrierattr_init

/* pthread_barrierattr_setpshared */
#define pthread_barrierattr_setpshared mpc_thread_barrierattr_setpshared

/* pthread_cancel */
#define pthread_cancel mpc_thread_cancel

/* pthread_cleanup_push */
#undef pthread_cleanup_push
#define pthread_cleanup_push mpc_thread_cleanup_push


/* pthread_cleanup_pop */
#undef pthread_cleanup_pop
#define pthread_cleanup_pop mpc_thread_cleanup_pop


/* pthread_cond_broadcast */
#define pthread_cond_broadcast mpc_thread_cond_broadcast

/* pthread_cond_destroy */
#define pthread_cond_destroy mpc_thread_cond_destroy

/* pthread_cond_init */
#define pthread_cond_init mpc_thread_cond_init

/* pthread_cond_signal */
#define pthread_cond_signal mpc_thread_cond_signal


/* pthread_cond_timedwait */
#define pthread_cond_timedwait mpc_thread_cond_timedwait

/* pthread_cond_wait */
#define pthread_cond_wait mpc_thread_cond_wait

/* pthread_condattr_destroy */
#define pthread_condattr_destroy mpc_thread_condattr_destroy

/* pthread_condattr_getclock */
#define pthread_condattr_getclock mpc_thread_condattr_getclock

/* pthread_condattr_getpshared */
#define pthread_condattr_getpshared mpc_thread_condattr_getpshared


/* pthread_condattr_init */
#define pthread_condattr_init mpc_thread_condattr_init


/* pthread_condattr_setclock */
#define pthread_condattr_setclock mpc_thread_condattr_setclock


/* pthread_condattr_setpshared */
#define pthread_condattr_setpshared mpc_thread_condattr_setpshared


/* pthread_create */
#define pthread_create sctk_user_thread_create


/* pthread_detach */
#define pthread_detach mpc_thread_detach

/* pthread_equal */
#define pthread_equal mpc_thread_equal


/* pthread_exit */
#define pthread_exit mpc_thread_exit


/* pthread_getconcurrency */
#define pthread_getconcurrency mpc_thread_getconcurrency


/* pthread_getcpuclockid */
#define pthread_getcpuclockid mpc_thread_getcpuclockid


/* pthread_getschedparam */
#define pthread_getschedparam mpc_thread_getschedparam


/* pthread_getspecific */
#define pthread_getspecific mpc_thread_getspecific


/* pthread_join */
#define pthread_join mpc_thread_join

/* pthread_key_create */
#define pthread_key_create mpc_thread_key_create


/* pthread_key_delete */
#define pthread_key_delete mpc_thread_key_delete


/* pthread_mutex_destroy */
#define pthread_mutex_destroy mpc_thread_mutex_destroy


/* pthread_mutex_getprioceiling */
#define pthread_mutex_getprioceiling mpc_thread_mutex_getprioceiling


/* pthread_mutex_init */
#define pthread_mutex_init mpc_thread_mutex_init

/* pthread_mutex_lock */
#define pthread_mutex_lock mpc_thread_mutex_lock


/* pthread_mutex_setprioceiling */
#define pthread_mutex_setprioceiling mpc_thread_mutex_setprioceiling


/* pthread_mutex_timedlock */
#define pthread_mutex_timedlock mpc_thread_mutex_timedlock


/* pthread_mutex_trylock */
#define pthread_mutex_trylock mpc_thread_mutex_trylock

/* pthread_mutex_unlock */
#define pthread_mutex_unlock mpc_thread_mutex_unlock


/* pthread_mutexattr_destroy */
#define pthread_mutexattr_destroy mpc_thread_mutexattr_destroy

/* pthread_mutexattr_getprioceiling */
#define pthread_mutexattr_getprioceiling mpc_thread_mutexattr_getprioceiling


/* pthread_mutexattr_getprotocol */
#define pthread_mutexattr_getprotocol mpc_thread_mutexattr_getprotocol


/* pthread_mutexattr_getpshared */
#define pthread_mutexattr_getpshared mpc_thread_mutexattr_getpshared


/* pthread_mutexattr_gettype */
#define pthread_mutexattr_gettype mpc_thread_mutexattr_gettype


/* pthread_mutexattr_init */
#define pthread_mutexattr_init mpc_thread_mutexattr_init


/* pthread_mutexattr_setprioceiling */
#define pthread_mutexattr_setprioceiling mpc_thread_mutexattr_setprioceiling


/* pthread_mutexattr_setprotocol */
#define pthread_mutexattr_setprotocol mpc_thread_mutexattr_setprotocol


/* pthread_mutexattr_setpshared */
#define pthread_mutexattr_setpshared mpc_thread_mutexattr_setpshared


/* pthread_mutexattr_settype */
#define pthread_mutexattr_settype mpc_thread_mutexattr_settype


/* pthread_once */
#define pthread_once mpc_thread_once


/* pthread_rwlock_destroy */
#define pthread_rwlock_destroy mpc_thread_rwlock_destroy


/* pthread_rwlock_init */
#define pthread_rwlock_init mpc_thread_rwlock_init


/* pthread_rwlock_rdlock */
#define pthread_rwlock_rdlock mpc_thread_rwlock_rdlock


/* pthread_rwlock_timedrdlock */
#define pthread_rwlock_timedrdlock mpc_thread_rwlock_timedrdlock


/* pthread_rwlock_timedwrlock */
#define pthread_rwlock_timedwrlock mpc_thread_rwlock_timedwrlock


/* pthread_rwlock_tryrdlock */
#define pthread_rwlock_tryrdlock mpc_thread_rwlock_tryrdlock


/* pthread_rwlock_trywrlock */
#define pthread_rwlock_trywrlock mpc_thread_rwlock_trywrlock

/* pthread_rwlock_unlock */
#define pthread_rwlock_unlock mpc_thread_rwlock_unlock


/* pthread_rwlock_wrlock */
#define pthread_rwlock_wrlock mpc_thread_rwlock_wrlock


/* pthread_rwlockattr_destroy */
#define pthread_rwlockattr_destroy mpc_thread_rwlockattr_destroy

/* pthread_rwlockattr_getpshared */
#define pthread_rwlockattr_getpshared mpc_thread_rwlockattr_getpshared


/* pthread_rwlockattr_init */
#define pthread_rwlockattr_init mpc_thread_rwlockattr_init


/* pthread_rwlockattr_setpshared */
#define pthread_rwlockattr_setpshared mpc_thread_rwlockattr_setpshared

/* pthread_self */
#define pthread_self mpc_thread_self


/* pthread_setcancelstate */
#define pthread_setcancelstate mpc_thread_setcancelstate


/* pthread_setcanceltype */
#define pthread_setcanceltype mpc_thread_setcanceltype


/* pthread_setconcurrency */
#define pthread_setconcurrency mpc_thread_setconcurrency


/* pthread_setschedparam */
#define pthread_setschedparam mpc_thread_setschedparam


/* pthread_setschedprio */
#define pthread_setschedprio mpc_thread_setschedprio


/* pthread_setspecific */
#define pthread_setspecific mpc_thread_setspecific


/* pthread_spin_destroy */
#define pthread_spin_destroy mpc_thread_spin_destroy


/* pthread_spin_init */
#define pthread_spin_init mpc_thread_spin_init


/* pthread_spin_lock */
#define pthread_spin_lock mpc_thread_spin_lock


/* pthread_spin_trylock */
#define pthread_spin_trylock mpc_thread_spin_trylock


/* pthread_spin_unlock */
#define pthread_spin_unlock mpc_thread_spin_unlock


/* pthread_testcancel */
#define pthread_testcancel mpc_thread_testcancel


/* pthread_yield */
#define pthread_yield mpc_thread_yield

#define pthread_kill mpc_thread_kill

#define pthread_sigmask mpc_thread_sigmask

#define pthread_getattr_np mpc_thread_getattr_np


/*semaphore.h*/

/* sem_init */
#define sem_init mpc_thread_sem_init


/* sem_ wait*/
#define sem_wait mpc_thread_sem_wait


/* sem_trywait */
#define sem_trywait mpc_thread_sem_trywait


/* sem_post */
#define sem_post mpc_thread_sem_post


/* sem_getvalue */
#define sem_getvalue mpc_thread_sem_getvalue


/* sem_destroy */
#define sem_destroy mpc_thread_sem_destroy


/* sem_open */
#define sem_open mpc_thread_sem_open


/* sem_close */
#define sem_close mpc_thread_sem_close


/* sem_unlink */
#define sem_unlink mpc_thread_sem_unlink


/* sem_timedwait */
#define sem_timedwait mpc_thread_sem_timedwait


/* atexit wrapping is done in mpc_main.h
 * file generated by the configure
 * 
 * in mpc_main.h:
 * 
 * #ifdef MPC_MODULE_MPC_Threads
 *   #define atexit mpc_atexit
 * #endif
 * 
 **/


/* Futex */

/************************************************************************/
/* Futex Wrapping                                                       */
/************************************************************************/

#include "sctk_futex_def.h"

#ifdef SCTK_FUTEX_SUPPORTED

#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>

#if 0
#ifdef __cplusplus

static inline long int __sctk_cpp_scope_dereferencing_proxy( long int a )
{
	return a;
}

/* In C++ sometimes you get :
 * ::syscall( SYS_futex,futex,1,2147483647,__null,__null,0 );
 * So we need to use a forwarding function.
 */

#define syscall(op, ...) \
    __sctk_cpp_scope_dereferencing_proxy((( op == SYS_futex )?mpc_thread_futex( op, ##__VA_ARGS__):syscall(op, ##__VA_ARGS__)))
#else
#define syscall(op, ...) \
    (( op == SYS_futex )?mpc_thread_futex( op, ##__VA_ARGS__):syscall(op, ##__VA_ARGS__))
#endif /* __cplusplus */

#endif

#endif /* SCTK_FUTEX_SUPPORTED */

#ifdef __cplusplus
}
#endif
#endif
