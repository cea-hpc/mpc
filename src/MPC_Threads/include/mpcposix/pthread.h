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

#include <mpc.h>
#include <mpc_thread.h>
#include <sched.h>

#define sem_t                                  mpc_thread_sem_t
#define pthread_t                              mpc_thread_t
#define pthread_attr_t                         mpc_thread_attr_t
#define pthread_condattr_t                     mpc_thread_condattr_t
#define pthread_cond_t                         mpc_thread_cond_t
#define pthread_mutex_t                        mpc_thread_mutex_t
#define pthread_mutexattr_t                    mpc_thread_mutexattr_t
#define pthread_key_t                          mpc_thread_keys_t
#define pthread_once_t                         mpc_thread_once_t

#define pthread_barrierattr_t                  mpc_thread_barrierattr_t
#define pthread_barrier_t                      mpc_thread_barrier_t
#define pthread_spinlock_t                     mpc_thread_spinlock_t

#define pthread_rwlockattr_t                   mpc_thread_rwlockattr_t
#define pthread_rwlock_t                       mpc_thread_rwlock_t

#undef PTHREAD_RWLOCK_INITIALIZER
#define PTHREAD_RWLOCK_INITIALIZER             SCTK_THREAD_RWLOCK_INITIALIZER

#undef PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_NORMAL                   SCTK_THREAD_MUTEX_NORMAL
#undef PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_RECURSIVE                SCTK_THREAD_MUTEX_RECURSIVE
#undef PTHREAD_MUTEX_ERRORCHECK
#define PTHREAD_MUTEX_ERRORCHECK               SCTK_THREAD_MUTEX_ERRORCHECK
#undef PTHREAD_MUTEX_DEFAULT
#define PTHREAD_MUTEX_DEFAULT                  SCTK_THREAD_MUTEX_DEFAULT

#undef PTHREAD_BARRIER_SERIAL_THREAD
#define PTHREAD_BARRIER_SERIAL_THREAD          SCTK_THREAD_BARRIER_SERIAL_THREAD

#undef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER              SCTK_THREAD_MUTEX_INITIALIZER
#undef PTHREAD_MUTEX_RECURSIVE_INITIALIZER
#define PTHREAD_MUTEX_RECURSIVE_INITIALIZER    SCTK_THREAD_MUTEX_RECURSIVE_INITIALIZER
#undef PTHREAD_COND_INITIALIZER
#define PTHREAD_COND_INITIALIZER               SCTK_THREAD_COND_INITIALIZER
#undef PTHREAD_ONCE_INIT
#define PTHREAD_ONCE_INIT                      SCTK_THREAD_ONCE_INIT

#undef PTHREAD_KEYS_MAX
#define PTHREAD_KEYS_MAX                       SCTK_THREAD_KEYS_MAX
#undef PTHREAD_CREATE_JOINABLE
#define PTHREAD_CREATE_JOINABLE                SCTK_THREAD_CREATE_JOINABLE
#undef PTHREAD_CREATE_DETACHED
#define PTHREAD_CREATE_DETACHED                SCTK_THREAD_CREATE_DETACHED
#undef PTHREAD_INHERIT_SCHED
#define PTHREAD_INHERIT_SCHED                  SCTK_THREAD_INHERIT_SCHED
#undef PTHREAD_EXPLICIT_SCHED
#define PTHREAD_EXPLICIT_SCHED                 SCTK_THREAD_EXPLICIT_SCHED
#undef PTHREAD_SCOPE_SYSTEM
#define PTHREAD_SCOPE_SYSTEM                   SCTK_THREAD_SCOPE_SYSTEM
#undef PTHREAD_SCOPE_PROCESS
#define PTHREAD_SCOPE_PROCESS                  SCTK_THREAD_SCOPE_PROCESS
#undef PTHREAD_PROCESS_PRIVATE
#define PTHREAD_PROCESS_PRIVATE                SCTK_THREAD_PROCESS_PRIVATE
#undef PTHREAD_PROCESS_SHARED
#define PTHREAD_PROCESS_SHARED                 SCTK_THREAD_PROCESS_SHARED
#undef PTHREAD_CANCEL_ENABLE
#define PTHREAD_CANCEL_ENABLE                  SCTK_THREAD_CANCEL_ENABLE
#undef PTHREAD_CANCEL_DISABLE
#define PTHREAD_CANCEL_DISABLE                 SCTK_THREAD_CANCEL_DISABLE
#undef PTHREAD_CANCEL_DEFERRED
#define PTHREAD_CANCEL_DEFERRED                SCTK_THREAD_CANCEL_DEFERRED
#undef PTHREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_CANCEL_ASYNCHRONOUS            SCTK_THREAD_CANCEL_ASYNCHRONOUS
#undef PTHREAD_CANCELED
#define PTHREAD_CANCELED                       SCTK_THREAD_CANCELED

#undef SEM_VALUE_MAX
#define SEM_VALUE_MAX                          SCTK_SEM_VALUE_MAX
#undef SEM_FAILED
#define SEM_FAILED                             SCTK_SEM_FAILED

#undef PTHREAD_PRIO_NONE
#define PTHREAD_PRIO_NONE                      SCTK_THREAD_PRIO_NONE
#undef PTHREAD_PRIO_INHERIT
#define PTHREAD_PRIO_INHERIT                   SCTK_THREAD_PRIO_INHERIT
#undef PTHREAD_PRIO_PROTECT
#define PTHREAD_PRIO_PROTECT                   SCTK_THREAD_PRIO_PROTECT

/* pthread_atfork */
#define pthread_atfork                         mpc_thread_atfork

/* pthread_attr_destroy */
#define pthread_attr_destroy                   mpc_thread_attr_destroy

/* pthread_attr_getdetachstate */
#define pthread_attr_getdetachstate            mpc_thread_attr_getdetachstate

/* pthread_attr_getguardsize */
#define pthread_attr_getguardsize              mpc_thread_attr_getguardsize

/* pthread_attr_getinheritsched */
#define pthread_attr_getinheritsched           mpc_thread_attr_getinheritsched

/* pthread_attr_getschedparam */
#define pthread_attr_getschedparam             mpc_thread_attr_getschedparam

/* pthread_attr_getschedpolicy */
#define pthread_attr_getschedpolicy            mpc_thread_attr_getschedpolicy

/* pthread_setaffinity_np */
#define pthread_setaffinity_np                 mpc_thread_setaffinity_np

/* pthread_getaffinity_np */
#define pthread_getaffinity_np                 mpc_thread_getaffinity_np

/* OpenMP Compat: sched_setaffinity */
#define sched_setaffinity                 	   mpc_thread_setaffinity_np

/* OpenMP Compat: sched_getaffinity */
#define sched_getaffinity                      mpc_thread_getaffinity_np

/* pthread_attr_getscope */
#define pthread_attr_getscope                  mpc_thread_attr_getscope

/* pthread_attr_getstack */
#define pthread_attr_getstack                  mpc_thread_attr_getstack

/* pthread_attr_getstackaddr */
#define pthread_attr_getstackaddr              mpc_thread_attr_getstackaddr

/* pthread_attr_getstacksize */
#define pthread_attr_getstacksize              mpc_thread_attr_getstacksize

/* pthread_attr_init */
#define pthread_attr_init                      mpc_thread_attr_init


/* pthread_attr_setdetachstate */
#define pthread_attr_setdetachstate    mpc_thread_attr_setdetachstate


/* pthread_attr_setguardsize */
#define pthread_attr_setguardsize    mpc_thread_attr_setguardsize


/* pthread_attr_setinheritsched */
#define pthread_attr_setinheritsched    mpc_thread_attr_setinheritsched


/* pthread_attr_setschedparam */
#define pthread_attr_setschedparam     mpc_thread_attr_setschedparam

/* pthread_attr_setschedpolicy */
#define pthread_attr_setschedpolicy    mpc_thread_attr_setschedpolicy


/* pthread_attr_setscope */
#define pthread_attr_setscope    mpc_thread_attr_setscope


/* pthread_attr_setstack */
#define pthread_attr_setstack    mpc_thread_attr_setstack


/* pthread_attr_setstackaddr */
#define pthread_attr_setstackaddr    mpc_thread_attr_setstackaddr


/* pthread_attr_setstacksize */
#define pthread_attr_setstacksize    mpc_thread_attr_setstacksize


/* pthread_barrier_destroy */
#define pthread_barrier_destroy    mpc_thread_core_barrier_destroy


/* pthread_barrier_init */
#define pthread_barrier_init    mpc_thread_core_barrier_init

/* pthread_barrier_wait */
#define pthread_barrier_wait    mpc_thread_core_barrier_wait


/* pthread_barrierattr_destroy */
#define pthread_barrierattr_destroy    mpc_thread_barrierattr_destroy


/* pthread_barrierattr_getpshared */
#define pthread_barrierattr_getpshared    mpc_thread_barrierattr_getpshared

/* pthread_barrierattr_init */
#define pthread_barrierattr_init          mpc_thread_barrierattr_init

/* pthread_barrierattr_setpshared */
#define pthread_barrierattr_setpshared    mpc_thread_barrierattr_setpshared

/* pthread_cancel */
#define pthread_cancel                    mpc_thread_cancel

/* pthread_cleanup_push */
#undef pthread_cleanup_push
#define pthread_cleanup_push(routine, arg)            \
	{ struct _sctk_thread_cleanup_buffer _buffer; \
	  mpc_thread_cleanup_push(&_buffer, (routine), (arg) );

/* pthread_cleanup_pop */
#undef pthread_cleanup_pop
#define pthread_cleanup_pop(execute) \
	mpc_thread_cleanup_pop(&_buffer, (execute) ); }

/* pthread_cond_broadcast */
#define pthread_cond_broadcast    mpc_thread_cond_broadcast

/* pthread_cond_destroy */
#define pthread_cond_destroy      mpc_thread_cond_destroy

/* pthread_cond_init */
#define pthread_cond_init         mpc_thread_cond_init

/* pthread_cond_signal */
#define pthread_cond_signal       mpc_thread_cond_signal


/* pthread_cond_timedwait */
#define pthread_cond_timedwait         mpc_thread_cond_timedwait

/* pthread_cond_clockwait */
#define pthread_cond_clockwait         mpc_thread_cond_clockwait

/* pthread_cond_wait */
#define pthread_cond_wait              mpc_thread_cond_wait

/* pthread_condattr_destroy */
#define pthread_condattr_destroy       mpc_thread_condattr_destroy

/* pthread_condattr_getclock */
#define pthread_condattr_getclock      mpc_thread_condattr_getclock

/* pthread_condattr_getpshared */
#define pthread_condattr_getpshared    mpc_thread_condattr_getpshared


/* pthread_condattr_init */
#define pthread_condattr_init    mpc_thread_condattr_init


/* pthread_condattr_setclock */
#define pthread_condattr_setclock    mpc_thread_condattr_setclock


/* pthread_condattr_setpshared */
#define pthread_condattr_setpshared    mpc_thread_condattr_setpshared


/* pthread_create */
#define pthread_create    mpc_thread_core_thread_create


/* pthread_detach */
#define pthread_detach    mpc_thread_detach

/* pthread_equal */
#define pthread_equal     mpc_thread_equal


/* pthread_exit */
#define pthread_exit    mpc_thread_exit


/* pthread_getconcurrency */
#define pthread_getconcurrency    mpc_thread_getconcurrency


/* pthread_getcpuclockid */
#define pthread_getcpuclockid    mpc_thread_getcpuclockid


/* pthread_getschedparam */
#define pthread_getschedparam    mpc_thread_getschedparam


/* pthread_getspecific */
#define pthread_getspecific    mpc_thread_getspecific


/* pthread_join */
#define pthread_join          mpc_thread_join

/* pthread_key_create */
#define pthread_key_create    mpc_thread_keys_create


/* pthread_key_delete */
#define pthread_key_delete    mpc_thread_keys_delete


/* pthread_mutex_destroy */
#define pthread_mutex_destroy    mpc_thread_mutex_destroy


/* pthread_mutex_getprioceiling */
#define pthread_mutex_getprioceiling    mpc_thread_mutex_getprioceiling


/* pthread_mutex_init */
#define pthread_mutex_init    mpc_thread_mutex_init

/* pthread_mutex_lock */
#define pthread_mutex_lock    mpc_thread_mutex_lock


/* pthread_mutex_setprioceiling */
#define pthread_mutex_setprioceiling    mpc_thread_mutex_setprioceiling


/* pthread_mutex_timedlock */
#define pthread_mutex_timedlock    mpc_thread_mutex_timedlock

/* pthread_mutex_clocklock */
#define pthread_mutex_clocklock    mpc_thread_mutex_clocklock


/* pthread_mutex_trylock */
#define pthread_mutex_trylock    mpc_thread_mutex_trylock

/* pthread_mutex_unlock */
#define pthread_mutex_unlock     mpc_thread_mutex_unlock


/* pthread_mutexattr_destroy */
#define pthread_mutexattr_destroy           mpc_thread_mutexattr_destroy

/* pthread_mutexattr_getprioceiling */
#define pthread_mutexattr_getprioceiling    mpc_thread_mutexattr_getprioceiling


/* pthread_mutexattr_getprotocol */
#define pthread_mutexattr_getprotocol    mpc_thread_mutexattr_getprotocol


/* pthread_mutexattr_getpshared */
#define pthread_mutexattr_getpshared    mpc_thread_mutexattr_getpshared


/* pthread_mutexattr_gettype */
#define pthread_mutexattr_gettype    mpc_thread_mutexattr_gettype


/* pthread_mutexattr_init */
#define pthread_mutexattr_init    mpc_thread_mutexattr_init


/* pthread_mutexattr_setprioceiling */
#define pthread_mutexattr_setprioceiling    mpc_thread_mutexattr_setprioceiling


/* pthread_mutexattr_setprotocol */
#define pthread_mutexattr_setprotocol    mpc_thread_mutexattr_setprotocol


/* pthread_mutexattr_setpshared */
#define pthread_mutexattr_setpshared    mpc_thread_mutexattr_setpshared


/* pthread_mutexattr_settype */
#define pthread_mutexattr_settype    mpc_thread_mutexattr_settype


/* pthread_once */
#define pthread_once    mpc_thread_once


/* pthread_rwlock_destroy */
#define pthread_rwlock_destroy    mpc_thread_rwlock_destroy


/* pthread_rwlock_init */
#define pthread_rwlock_init    mpc_thread_rwlock_init


/* pthread_rwlock_rdlock */
#define pthread_rwlock_rdlock    mpc_thread_rwlock_rdlock


/* pthread_rwlock_timedrdlock */
#define pthread_rwlock_timedrdlock    mpc_thread_rwlock_timedrdlock

/* pthread_rwlock_clockrdlock */
#define pthread_rwlock_clockrdlock    mpc_thread_rwlock_clockrdlock


/* pthread_rwlock_timedwrlock */
#define pthread_rwlock_timedwrlock    mpc_thread_rwlock_timedwrlock

/* pthread_rwlock_clockwrlock */
#define pthread_rwlock_clockwrlock    mpc_thread_rwlock_clockwrlock


/* pthread_rwlock_tryrdlock */
#define pthread_rwlock_tryrdlock    mpc_thread_rwlock_tryrdlock


/* pthread_rwlock_trywrlock */
#define pthread_rwlock_trywrlock    mpc_thread_rwlock_trywrlock

/* pthread_rwlock_unlock */
#define pthread_rwlock_unlock       mpc_thread_rwlock_unlock


/* pthread_rwlock_wrlock */
#define pthread_rwlock_wrlock    mpc_thread_rwlock_wrlock


/* pthread_rwlockattr_destroy */
#define pthread_rwlockattr_destroy       mpc_thread_rwlockattr_destroy

/* pthread_rwlockattr_getpshared */
#define pthread_rwlockattr_getpshared    mpc_thread_rwlockattr_getpshared


/* pthread_rwlockattr_init */
#define pthread_rwlockattr_init    mpc_thread_rwlockattr_init


/* pthread_rwlockattr_setpshared */
#define pthread_rwlockattr_setpshared    mpc_thread_rwlockattr_setpshared

/* pthread_self */
#define pthread_self                     mpc_thread_self


/* pthread_setcancelstate */
#define pthread_setcancelstate    mpc_thread_setcancelstate


/* pthread_setcanceltype */
#define pthread_setcanceltype    mpc_thread_setcanceltype


/* pthread_setconcurrency */
#define pthread_setconcurrency    mpc_thread_setconcurrency


/* pthread_setschedparam */
#define pthread_setschedparam    mpc_thread_setschedparam


/* pthread_setschedprio */
#define pthread_setschedprio    mpc_thread_setschedprio


/* pthread_setspecific */
#define pthread_setspecific    mpc_thread_setspecific


/* pthread_spin_destroy */
#define pthread_spin_destroy    mpc_thread_spin_destroy


/* pthread_spin_init */
#define pthread_spin_init    mpc_thread_spin_init


/* pthread_spin_lock */
#define pthread_spin_lock    mpc_thread_spin_lock


/* pthread_spin_trylock */
#define pthread_spin_trylock    mpc_thread_spin_trylock


/* pthread_spin_unlock */
#define pthread_spin_unlock    mpc_thread_spin_unlock


/* pthread_testcancel */
#define pthread_testcancel    mpc_thread_testcancel


/* pthread_yield */
#define pthread_yield         mpc_thread_yield

#define pthread_kill          mpc_thread_kill

#define pthread_sigmask       mpc_thread_sigmask

#define pthread_getattr_np    mpc_thread_getattr_np


/*semaphore.h*/

/* sem_init */
#define sem_init    mpc_thread_sem_init


/* sem_ wait*/
#define sem_wait    mpc_thread_sem_wait


/* sem_trywait */
#define sem_trywait    mpc_thread_sem_trywait


/* sem_post */
#define sem_post    mpc_thread_sem_post


/* sem_getvalue */
#define sem_getvalue    mpc_thread_sem_getvalue


/* sem_destroy */
#define sem_destroy    mpc_thread_sem_destroy


/* sem_open */
#define sem_open    mpc_thread_sem_open


/* sem_close */
#define sem_close    mpc_thread_sem_close


/* sem_unlink */
#define sem_unlink    mpc_thread_sem_unlink


/* sem_timedwait */
#define sem_timedwait    mpc_thread_sem_timedwait


/**********************************
* OTHER THREAD RELATED REDIRECTS *
**********************************/

#define sched_yield    mpc_thread_yield
#define raise(a)    mpc_thread_kill(mpc_thread_self(), a)

#ifndef SCTK_DONOT_REDEFINE_KILL
#define kill          mpc_thread_process_kill
#endif

#define sigpending    mpc_thread_sigpending
#define sigsuspend    mpc_thread_sigsuspend
#define sigwait       mpc_thread_sigwait

#define sleep         mpc_thread_sleep
#define usleep        mpc_thread_usleep
#define nanosleep     mpc_thread_nanosleep

/* Futex */

/************************************************************************/
/* Futex Wrapping                                                       */
/************************************************************************/

#ifdef SCTK_FUTEX_SUPPORTED

#include <linux/futex.h>
#include <sys/syscall.h>

/* Futex Opcodes */

#undef FUTEX_WAIT
#define FUTEX_WAIT SCTK_FUTEX_WAIT
#undef FUTEX_WAKE
#define FUTEX_WAKE SCTK_FUTEX_WAKE
#undef FUTEX_REQUEUE
#define FUTEX_REQUEUE SCTK_FUTEX_REQUEUE
#undef FUTEX_CMP_REQUEUE
#define FUTEX_CMP_REQUEUE SCTK_FUTEX_CMP_REQUEUE
#undef FUTEX_WAKE_OP
#define FUTEX_WAKE_OP SCTK_FUTEX_WAKE_OP
#undef FUTEX_WAIT_BITSET
#define FUTEX_WAIT_BITSET SCTK_FUTEX_WAIT_BITSET
#undef FUTEX_WAKE_BITSET
#define FUTEX_WAKE_BITSET SCTK_FUTEX_WAKE_BITSET
#undef FUTEX_LOCK_PI
#define FUTEX_LOCK_PI SCTK_FUTEX_LOCK_PI
#undef FUTEX_TRYLOCK_PI
#define FUTEX_TRYLOCK_PI SCTK_FUTEX_TRYLOCK_PI
#undef FUTEX_UNLOCK_PI
#define FUTEX_UNLOCK_PI SCTK_FUTEX_UNLOCK_PI
#undef FUTEX_CMP_REQUEUE_PI
#define FUTEX_CMP_REQUEUE_PI SCTK_FUTEX_CMP_REQUEUE_PI
#undef FUTEX_WAIT_REQUEUE_PI
#define FUTEX_WAIT_REQUEUE_PI SCTK_FUTEX_WAIT_REQUEUE_PI

/* WAITERS */
#undef FUTEX_WAITERS
#define FUTEX_WAITERS SCTK_FUTEX_WAITERS

/* OPS */

#undef FUTEX_OP_SET
#define FUTEX_OP_SET SCTK_FUTEX_OP_SET
#undef FUTEX_OP_ADD
#define FUTEX_OP_ADD SCTK_FUTEX_OP_ADD
#undef FUTEX_OP_OR
#define FUTEX_OP_OR SCTK_FUTEX_OP_OR
#undef FUTEX_OP_ANDN
#define FUTEX_OP_ANDN SCTK_FUTEX_OP_ANDN
#undef FUTEX_OP_XOR
#define FUTEX_OP_XOR SCTK_FUTEX_OP_XOR
#undef FUTEX_OP_ARG_SHIFT
#define FUTEX_OP_ARG_SHIFT SCTK_FUTEX_OP_ARG_SHIFT

/* CMP */

#undef FUTEX_OP_CMP_EQ
#define FUTEX_OP_CMP_EQ SCTK_FUTEX_OP_CMP_EQ
#undef FUTEX_OP_CMP_NE
#define FUTEX_OP_CMP_NE SCTK_FUTEX_OP_CMP_NE
#undef FUTEX_OP_CMP_LT
#define FUTEX_OP_CMP_LT SCTK_FUTEX_OP_CMP_LT
#undef FUTEX_OP_CMP_LE
#define FUTEX_OP_CMP_LE SCTK_FUTEX_OP_CMP_LE
#undef FUTEX_OP_CMP_GT
#define FUTEX_OP_CMP_GT SCTK_FUTEX_OP_CMP_GT
#undef FUTEX_OP_CMP_GE
#define FUTEX_OP_CMP_GE SCTK_FUTEX_OP_CMP_GE

/* Futex Redirect */

#ifdef __cplusplus

static inline long int __sctk_cpp_scope_dereferencing_proxy(long int a)
{
	return a;
}

/* In C++ sometimes you get :
 * ::syscall( SYS_futex,futex,1,2147483647,__null,__null,0 );
 * So we need to use a forwarding function.
 */

#define syscall(op, ...) \
	__sctk_cpp_scope_dereferencing_proxy( ( (op == SYS_futex) ? mpc_thread_futex_with_vaargs(op, ## __VA_ARGS__) : syscall(op, ## __VA_ARGS__) ) )
#else
#define syscall(op, ...) \
	( (op == SYS_futex) ? mpc_thread_futex_with_vaargs(op, ## __VA_ARGS__) : syscall(op, ## __VA_ARGS__) )
#endif /* __cplusplus */

#endif /* SCTK_FUTEX_SUPPORTED */

#ifdef __cplusplus
}
#endif
#endif
