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
#include "stdarg.h"

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
#define MPC_THREAD_MUTEX_RECURSIVE_INITIALIZER SCTK_THREAD_MUTEX_RECURSIVE_INITIALIZER

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
#undef SEM_FAILED
#define SEM_FAILED SCTK_SEM_FAILED


/* pthread_atfork */
   int mpc_thread_atfork (void (*a) (void), void (*b) (void),
				       void (*c) (void));
/* pthread_attr_destroy */
   int mpc_thread_attr_destroy (sctk_thread_attr_t * attr);

/* pthread_attr_getdetachstate */
   int mpc_thread_attr_getdetachstate (const
						    sctk_thread_attr_t *
						    attr, int *state);

/* pthread_attr_getguardsize */
   int mpc_thread_attr_getguardsize (const sctk_thread_attr_t
						  * attr, size_t * size);

/* pthread_attr_getinheritsched */
   int mpc_thread_attr_getinheritsched (const
						     sctk_thread_attr_t *
						     attr, int *type);
/* pthread_attr_getschedparam */
   int mpc_thread_attr_getschedparam (const
						   sctk_thread_attr_t *
						   attr,
						   struct sched_param *type);

/* sched_get_priority_max*/
#define sched_get_priority_max(a) sctk_thread_sched_get_priority_max (a)

/* sched_get_priority_min*/
#define sched_get_priority_min(a) sctk_thread_sched_get_priority_min(a)

/* pthread_attr_getschedpolicy */
   int mpc_thread_attr_getschedpolicy (const
						    sctk_thread_attr_t *
						    attr, int *policy);

/* pthread_attr_getscope */
   int mpc_thread_attr_getscope (const sctk_thread_attr_t *
					      attr, int *scope);

/* pthread_attr_getstack */
   int mpc_thread_attr_getstack (const sctk_thread_attr_t *
					      attr, void **stack,
					      size_t * size);

/* pthread_attr_getstackaddr */
   int mpc_thread_attr_getstackaddr (const sctk_thread_attr_t
						  * attr, void **stack);

/* pthread_attr_getstacksize */
   int mpc_thread_attr_getstacksize (const sctk_thread_attr_t
						  * attr, size_t * size);

/* pthread_attr_init */
   int mpc_thread_attr_init (sctk_thread_attr_t * attr);


/* pthread_attr_setdetachstate */
   int mpc_thread_attr_setdetachstate (sctk_thread_attr_t *
						    attr, int state);


/* pthread_attr_setguardsize */
   int mpc_thread_attr_setguardsize (sctk_thread_attr_t *
						  attr, size_t size);


/* pthread_attr_setinheritsched */
   int mpc_thread_attr_setinheritsched (sctk_thread_attr_t *
						     attr, int sched);


/* pthread_attr_setschedparam */
   int mpc_thread_attr_setschedparam (sctk_thread_attr_t *
						   attr, const struct
						   sched_param *sched);
/* pthread_attr_setschedpolicy */
   int mpc_thread_attr_setschedpolicy (sctk_thread_attr_t *
						    attr, int policy);


/* pthread_attr_setscope */
   int mpc_thread_attr_setscope (sctk_thread_attr_t * attr,
					      int scope);


/* pthread_attr_setstack */
   int mpc_thread_attr_setstack (sctk_thread_attr_t * attr,
					      void *stack, size_t size);


/* pthread_attr_setstackaddr */
   int mpc_thread_attr_setstackaddr (sctk_thread_attr_t *
						  attr, void *stack);


/* pthread_attr_setstacksize */
   int mpc_thread_attr_setstacksize (sctk_thread_attr_t *
						  attr, size_t size);


/* pthread_barrier_destroy */
   int mpc_thread_barrier_destroy (sctk_thread_barrier_t *
						barrier);


/* pthread_barrier_init */
   int mpc_thread_barrier_init (sctk_thread_barrier_t *
					     barrier, const
					     sctk_thread_barrierattr_t *
					     attr, unsigned i);

/* pthread_barrier_wait */
   int mpc_thread_barrier_wait (sctk_thread_barrier_t * barrier);


/* pthread_barrierattr_destroy */
   int
    mpc_thread_barrierattr_destroy (sctk_thread_barrierattr_t * barrier);


/* pthread_barrierattr_getpshared */
   int mpc_thread_barrierattr_getpshared (const
						       sctk_thread_barrierattr_t
						       * barrier,
						       int *pshared);
/* pthread_barrierattr_init */
   int mpc_thread_barrierattr_init (sctk_thread_barrierattr_t
						 * barrier);

/* pthread_barrierattr_setpshared */
   int
    mpc_thread_barrierattr_setpshared (sctk_thread_barrierattr_t * barrier,
				       int pshared);

/* pthread_cancel */
  int mpc_thread_cancel (sctk_thread_t thread);

/* pthread_cleanup_push */
#define mpc_thread_cleanup_push sctk_thread_cleanup_push

/* pthread_cleanup_pop */
#define mpc_thread_cleanup_pop sctk_thread_cleanup_pop

/* pthread_cond_broadcast */
   int mpc_thread_cond_broadcast (sctk_thread_cond_t * cond);

/* pthread_cond_destroy */
   int mpc_thread_cond_destroy (sctk_thread_cond_t * cond);

/* pthread_cond_init */
   int mpc_thread_cond_init (sctk_thread_cond_t * cond,
					  const sctk_thread_condattr_t * attr);

/* pthread_cond_signal */
   int mpc_thread_cond_signal (sctk_thread_cond_t * cond);


/* pthread_cond_timedwait */
   int mpc_thread_cond_timedwait (sctk_thread_cond_t * cond,
					       sctk_thread_mutex_t *
					       mutex,
					       const struct timespec *time);

/* pthread_cond_wait */
   int mpc_thread_cond_wait (sctk_thread_cond_t * cond,
					  sctk_thread_mutex_t * mutex);

/* pthread_condattr_destroy */
   int mpc_thread_condattr_destroy (sctk_thread_condattr_t *
						 attr);

/* pthread_condattr_getclock */
   int mpc_thread_condattr_getclock (sctk_thread_condattr_t *
						  attr, clockid_t * clock);


/* pthread_condattr_getpshared */
   int mpc_thread_condattr_getpshared (const
						    sctk_thread_condattr_t
						    * attr, int *pshared);


/* pthread_condattr_init */
   int mpc_thread_condattr_init (sctk_thread_condattr_t * attr);


/* pthread_condattr_setclock */
   int mpc_thread_condattr_setclock (sctk_thread_condattr_t *
						  attr, clockid_t clock);


/* pthread_condattr_setpshared */
   int mpc_thread_condattr_setpshared (sctk_thread_condattr_t
						    * attr, int pshared);


/* pthread_create */
   int mpc_thread_create (sctk_thread_t * th,
				       const sctk_thread_attr_t * attr,
				       void *(*func) (void *), void *arg);


/* pthread_detach */
   int mpc_thread_detach (sctk_thread_t th);

/* pthread_equal */
   int mpc_thread_equal (sctk_thread_t th1, sctk_thread_t th2);


/* pthread_exit */
  void mpc_thread_exit (void *result);


/* mpc_thread_getconcurrency */
   int mpc_thread_getconcurrency (void);


/* pthread_getcpuclockid */
   int mpc_thread_getcpuclockid (sctk_thread_t th,
					      clockid_t * clock);


/* pthread_getschedparam */
   int mpc_thread_getschedparam (sctk_thread_t th, int *a,
					      struct sched_param *b);


/* pthread_getspecific */
   void *mpc_thread_getspecific (sctk_thread_key_t key);


/* pthread_join */
   int mpc_thread_join (sctk_thread_t th, void **ret);

/* pthread_key_create */
   int mpc_thread_key_create (sctk_thread_key_t * key,
					   void (*destr) (void *));


/* pthread_key_delete */
   int mpc_thread_key_delete (sctk_thread_key_t key);


/* pthread_mutex_destroy */
   int mpc_thread_mutex_destroy (sctk_thread_mutex_t * mutex);


/* pthread_mutex_getprioceiling */
   int mpc_thread_mutex_getprioceiling (const
						     sctk_thread_mutex_t *
						     mutex, int *prio);


/* pthread_mutex_init */
   int mpc_thread_mutex_init (sctk_thread_mutex_t * mutex,
					   const sctk_thread_mutexattr_t *
					   attr);

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
int user_sctk_thread_mutex_lock (sctk_thread_mutex_t * mutex);
int user_sctk_thread_mutex_unlock (sctk_thread_mutex_t * mutex);
#endif

/* pthread_mutex_lock */
int mpc_thread_mutex_lock (sctk_thread_mutex_t * mutex);


/* pthread_mutex_setprioceiling */
   int mpc_thread_mutex_setprioceiling (sctk_thread_mutex_t *
						     mutex, int a, int *b);


/* pthread_mutex_timedlock */
   int mpc_thread_mutex_timedlock (sctk_thread_mutex_t *
						mutex,
						const struct timespec *time);


/* pthread_mutex_trylock */
   int mpc_thread_mutex_trylock (sctk_thread_mutex_t * mutex);


/* pthread_mutex_unlock */
int mpc_thread_mutex_unlock (sctk_thread_mutex_t * mutex);


/* pthread_mutexattr_destroy */
   int mpc_thread_mutexattr_destroy (sctk_thread_mutexattr_t
						  * attr);


/* pthread_mutexattr_getprioceiling */
   int mpc_thread_mutexattr_getprioceiling (const
							 sctk_thread_mutexattr_t
							 * attr, int *prio);


/* pthread_mutexattr_getprotocol */
   int mpc_thread_mutexattr_getprotocol (const
						      sctk_thread_mutexattr_t
						      * attr, int *protocol);


/* pthread_mutexattr_getpshared */
   int mpc_thread_mutexattr_getpshared (const
						     sctk_thread_mutexattr_t
						     * attr, int *pshared);


/* pthread_mutexattr_gettype */
   int mpc_thread_mutexattr_gettype (const
						  sctk_thread_mutexattr_t
						  * attr, int *type);


/* pthread_mutexattr_init */
   int mpc_thread_mutexattr_init (sctk_thread_mutexattr_t * attr);


/* pthread_mutexattr_setprioceiling */
   int
    mpc_thread_mutexattr_setprioceiling (sctk_thread_mutexattr_t * attr,
					 int prio);


/* pthread_mutexattr_setprotocol */
   int
    mpc_thread_mutexattr_setprotocol (sctk_thread_mutexattr_t * attr,
				      int protocol);


/* pthread_mutexattr_setpshared */
   int
    mpc_thread_mutexattr_setpshared (sctk_thread_mutexattr_t * attr,
				     int pshared);


/* pthread_mutexattr_settype */
   int mpc_thread_mutexattr_settype (sctk_thread_mutexattr_t
						  * attr, int type);


/* pthread_once */
   int mpc_thread_once (sctk_thread_once_t * once,
				     void (*func) (void));


/* pthread_rwlock_destroy */
   int mpc_thread_rwlock_destroy (sctk_thread_rwlock_t * lock);


/* pthread_rwlock_init */
   int mpc_thread_rwlock_init (sctk_thread_rwlock_t * lock,
					    const sctk_thread_rwlockattr_t
					    * attr);


/* pthread_rwlock_rdlock */
   int mpc_thread_rwlock_rdlock (sctk_thread_rwlock_t * lock);


/* pthread_rwlock_timedrdlock */
   int mpc_thread_rwlock_timedrdlock (sctk_thread_rwlock_t *
						   lock,
						   const struct timespec
						   *time);


/* pthread_rwlock_timedwrlock */
   int mpc_thread_rwlock_timedwrlock (sctk_thread_rwlock_t *
						   lock,
						   const struct timespec
						   *time);


/* pthread_rwlock_tryrdlock */
   int mpc_thread_rwlock_tryrdlock (sctk_thread_rwlock_t * lock);


/* pthread_rwlock_trywrlock */
   int mpc_thread_rwlock_trywrlock (sctk_thread_rwlock_t * lock);


/* pthread_rwlock_unlock */
   int mpc_thread_rwlock_unlock (sctk_thread_rwlock_t * lock);


/* pthread_rwlock_wrlock */
   int mpc_thread_rwlock_wrlock (sctk_thread_rwlock_t * lock);


/* pthread_rwlockattr_destroy */
   int
    mpc_thread_rwlockattr_destroy (sctk_thread_rwlockattr_t * attr);


/* pthread_rwlockattr_getpshared */
   int mpc_thread_rwlockattr_getpshared (const
						      sctk_thread_rwlockattr_t
						      * attr, int *pshared);


/* pthread_rwlockattr_init */
   int mpc_thread_rwlockattr_init (sctk_thread_rwlockattr_t *
						attr);


/* pthread_rwlockattr_setpshared */
   int
    mpc_thread_rwlockattr_setpshared (sctk_thread_rwlockattr_t * attr,
				      int pshared);


/* pthread_self */
   sctk_thread_t mpc_thread_self (void);


/* pthread_setcancelstate */
   int mpc_thread_setcancelstate (int newarg, int *old);


/* pthread_setcanceltype */
   int mpc_thread_setcanceltype (int newarg, int *old);


/* pthread_setconcurrency */
   int mpc_thread_setconcurrency (int concurrency);


/* pthread_setschedparam */
   int mpc_thread_setschedparam (sctk_thread_t th, int a,
					      const struct sched_param *b);


/* pthread_setschedprio */
   int mpc_thread_setschedprio (sctk_thread_t th, int prio);


/* pthread_setspecific */
   int mpc_thread_setspecific (sctk_thread_key_t key,
					    const void *value);


/* pthread_spin_destroy */
   int mpc_thread_spin_destroy (sctk_thread_spinlock_t * spin);


/* pthread_spin_init */
   int mpc_thread_spin_init (sctk_thread_spinlock_t * spin,
					  int a);


/* pthread_spin_lock */
   int mpc_thread_spin_lock (sctk_thread_spinlock_t * spin);


/* pthread_spin_trylock */
   int mpc_thread_spin_trylock (sctk_thread_spinlock_t * spin);


/* pthread_spin_unlock */
   int mpc_thread_spin_unlock (sctk_thread_spinlock_t * spin);


/* pthread_testcancel */
  void mpc_thread_testcancel (void);


/* pthread_yield */
   int mpc_thread_yield (void);

   int mpc_thread_kill (sctk_thread_t thread, int signo);


   int mpc_thread_sigmask (int how, const sigset_t * newmask,
					sigset_t * oldmask);

   int mpc_thread_getattr_np (sctk_thread_t th,
					   sctk_thread_attr_t * attr);


/*semaphore.h*/
/* sem_init */
   int mpc_thread_sem_init (sctk_thread_sem_t * sem,
					 int pshared, unsigned int value);


/* sem_ wait*/
   int mpc_thread_sem_wait (sctk_thread_sem_t * sem);


/* sem_trywait */
   int mpc_thread_sem_trywait (sctk_thread_sem_t * sem);


/* sem_post */
   int mpc_thread_sem_post (sctk_thread_sem_t * sem);


/* sem_getvalue */
   int mpc_thread_sem_getvalue (sctk_thread_sem_t * sem,
					     int *sval);


/* sem_destroy */
   int mpc_thread_sem_destroy (sctk_thread_sem_t * sem);


/* sem_open */
   sctk_thread_sem_t *mpc_thread_sem_open (const char *name,
					   int oflag, ...);


/* sem_close */
   int mpc_thread_sem_close (sctk_thread_sem_t * sem);


/* sem_unlink */
   int mpc_thread_sem_unlink (const char *__name);


/* sem_timedwait */
   int mpc_thread_sem_timedwait (sctk_thread_sem_t * __sem,
					      const struct timespec
					      *__abstime);

   unsigned long mpc_thread_atomic_add (volatile unsigned long
						     *ptr, unsigned long val);

    unsigned long mpc_thread_tls_entry_add (unsigned long size,
							void (*func) (void *));

#define mpc_thread_sched_yield sctk_thread_yield
#define mpc_thread_raise(a) sctk_thread_kill(sctk_thread_self(), a)

/* #define mpc_thread_kill  sctk_thread_kill */
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

#ifdef MPC_AIO_ENABLED

/* Make sure that we have at least POSIX.1-2001
 * (see FEATURE_TEST_MACROS(7)) to handle the sigevent
 * data-structure required by AIO (otherwise do not include) */
#if (_POSIX_C_SOURCE - 0) >= 200112L

#include <signal.h>
#include <time.h>
#include <aio.h>

/* Declare the MPC aio interface if enabled */
int sctk_aio_read( struct aiocb * cb );
int sctk_aio_write( struct aiocb * cb );
int sctk_aio_fsync( int op, struct aiocb * cb );
int sctk_aio_error( struct aiocb * cb );
ssize_t sctk_aio_return( struct aiocb * cb );
int sctk_aio_cancel( int fd, struct aiocb * cb );
int sctk_aio_suspend( const struct aiocb * const aiocb_list[], int nitems, const struct timespec * timeout );
int sctk_aio_lio_listio( int mode , struct aiocb * const aiocb_list[], int nitems, struct sigevent *sevp );

#endif

#endif

#ifdef MPC_GETOPT_ENABLED
/* Getopt Support */

#define _GETOPT_H
#include <unistd.h>
#include <getopt.h>

/* Getopt variables */
extern char * sctk_optarg;
extern int sctk_optind, sctk_opterr, sctk_optopt, sctk_optreset, sctk_optpos;

/* Define the option struct */
struct option
{
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

/* Redefine the getopt modifiers */
#define no_argument        0
#define required_argument  1
#define optional_argument  2

/* SCTK getopt implementation */
int sctk_getopt(int, char * const [], const char *);
int sctk_getopt_long(int, char *const *, const char *, const struct option *, int *);
int sctk_getopt_long_only(int, char *const *, const char *, const struct option *, int *);

/* Rewrite getopt variables */
#define optarg sctk_optarg
#define optind sctk_optind
#define opterr sctk_opterr
#define optopt sctk_optopt
#define optreset sctk_optreset
#define optpos sctk_optpos

/* Rewrite getopt functions */
#define getopt_long sctk_getopt_long
#define getopt_long_only sctk_getopt_long_only
#define getopt sctk_getopt
#define __getopt_msg sctk___getopt_msg

/* End of getopt support */
#endif

#ifdef __cplusplus
}
#endif
#endif
