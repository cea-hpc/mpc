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
/* Needed to activate the whole interface */
#include "sctk_default_pthread_flags.h"

#include <pthread.h>
#include <semaphore.h>

#include "sctk_posix_pthread.h"
#include "thread_ptr.h"
#include "sctk_posix_ethread_np.h"

void
sctk_posix_pthread()
{
	/*le pthread once */
#ifdef HAVE_PTHREAD_ONCE
	sctk_add_func_type(pthread, once,
	                   int (*)(mpc_thread_once_t *, void (*)(void) ) );
#endif
	/*les attributs des threads */
#ifdef HAVE_PTHREAD_ATTR_SETDETACHSTATE
	sctk_add_func_type(pthread, attr_setdetachstate,
	                   int (*)(mpc_thread_attr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETDETACHSTATE
	sctk_add_func_type(pthread, attr_getdetachstate,
	                   int (*)(const mpc_thread_attr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
	sctk_add_func_type(pthread, attr_setschedpolicy,
	                   int (*)(mpc_thread_attr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSCHEDPOLICY
	sctk_add_func_type(pthread, attr_getschedpolicy,
	                   int (*)(const mpc_thread_attr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETINHERITSCHED
	sctk_add_func_type(pthread, attr_setinheritsched,
	                   int (*)(mpc_thread_attr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETINHERITSCHED
	sctk_add_func_type(pthread, attr_getinheritsched,
	                   int (*)(const mpc_thread_attr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSCOPE
	sctk_add_func_type(pthread, attr_getscope,
	                   int (*)(const mpc_thread_attr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSCOPE
	sctk_add_func_type(pthread, attr_setscope,
	                   int (*)(mpc_thread_attr_t *, int) );
#endif
#ifndef HAVE_PTHREAD_ATTR_GETSTACK
#ifdef HAVE_PTHREAD_ATTR_GETSTACKADDR
	sctk_add_func_type(pthread, attr_getstackaddr,
	                   int (*)(const mpc_thread_attr_t *, void **) );
#endif
#endif
#ifndef  HAVE_PTHREAD_ATTR_SETSTACK
#ifdef HAVE_PTHREAD_ATTR_SETSTACKADDR
	sctk_add_func_type(pthread, attr_setstackaddr,
	                   int (*)(mpc_thread_attr_t *, void *) );
#endif
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSTACKSIZE
	sctk_add_func_type(pthread, attr_getstacksize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
	sctk_add_func_type(pthread, attr_setstacksize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSTACK
	sctk_add_func_type(pthread, attr_getstack,
	                   int (*)(const mpc_thread_attr_t *, void **, size_t *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSTACK
	sctk_add_func_type(pthread, attr_setstack,
	                   int (*)(mpc_thread_attr_t *, void *, size_t) );
#endif
/*mutex attr*/
#ifdef HAVE_PTHREAD_MUTEXATTR_INIT
	sctk_add_func_type(pthread, mutexattr_init,
	                   int (*)(mpc_thread_mutexattr_t *) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_DESTROY
	sctk_add_func_type(pthread, mutexattr_destroy,
	                   int (*)(mpc_thread_mutexattr_t *) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_SETTYPE
	sctk_add_func_type(pthread, mutexattr_settype,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_GETTYPE
	sctk_add_func_type(pthread, mutexattr_gettype,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_SETPSHARED
	sctk_add_func_type(pthread, mutexattr_setpshared,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_GETPSHARED
	sctk_add_func_type(pthread, mutexattr_getpshared,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_DESTROY
	sctk_add_func_type(pthread, mutex_destroy, int (*)(mpc_thread_mutex_t *) );
	/*gestion du cancel */
#endif
#ifdef HAVE_PTHREAD_TESTCANCEL
	sctk_add_func_type(pthread, testcancel, void (*)(void) );
#endif
#ifdef HAVE_PTHREAD_CANCEL
	sctk_add_func_type(pthread, cancel, int (*)(mpc_thread_t) );
#endif
#ifdef HAVE_PTHREAD_SETCANCELSTATE
	sctk_add_func_type(pthread, setcancelstate, int (*)(int, int *) );
#endif
#ifdef HAVE_PTHREAD_SETCANCELTYPE
	sctk_add_func_type(pthread, setcanceltype, int (*)(int, int *) );
#endif

	/* pthread_detach */
#ifdef HAVE_PTHREAD_DETACH
	sctk_add_func_type(pthread, detach, int (*)(mpc_thread_t) );
#endif
	/*les s�maphores  */
#ifdef HAVE_SEM_INIT
	_funcptr_mpc_thread_sem_init = (int (*)(mpc_thread_sem_t *,
	                                      int, unsigned int) )sem_init;
#endif
#ifdef HAVE_SEM_WAIT
	_funcptr_mpc_thread_sem_wait = (int (*)(mpc_thread_sem_t *) )sem_wait;
#endif
#ifdef HAVE_SEM_TRYWAIT
	_funcptr_mpc_thread_sem_trywait = (int (*)(mpc_thread_sem_t *) )sem_trywait;
#endif
#ifdef HAVE_SEM_POST
	_funcptr_mpc_thread_sem_post = (int (*)(mpc_thread_sem_t *) )sem_post;
#endif
#ifdef HAVE_SEM_GETVALUE
	_funcptr_mpc_thread_sem_getvalue =
	        (int (*)(mpc_thread_sem_t *, int *) )sem_getvalue;
#endif
#ifdef HAVE_SEM_DESTROY
	_funcptr_mpc_thread_sem_destroy = (int (*)(mpc_thread_sem_t *) )sem_destroy;
#endif
#ifdef HAVE_SEM_OPEN
	_funcptr_mpc_thread_sem_open =
	        (mpc_thread_sem_t * (*)(const char *, int, ...) )sem_open;
#endif
#ifdef HAVE_SEM_CLOSE
	_funcptr_mpc_thread_sem_close = (int (*)(mpc_thread_sem_t *) )sem_close;
#endif
#ifdef HAVE_SEM_UNLINK
	_funcptr_mpc_thread_sem_unlink = (int (*)(const char *) )sem_unlink;
#endif

/*sched priority*/
	_funcptr_mpc_thread_sched_get_priority_max =
	        (int (*)(int) )sched_get_priority_max;
	_funcptr_mpc_thread_sched_get_priority_min =
	        (int (*)(int) )sched_get_priority_min;


	/*conditions */
#ifdef HAVE_PTHREAD_COND_INIT
	sctk_add_func_type(pthread, cond_init,
	                   int (*)(mpc_thread_cond_t *,
	                           const mpc_thread_condattr_t *) );
#endif
#ifdef HAVE_PTHREAD_COND_DESTROY
	sctk_add_func_type(pthread, cond_destroy, int (*)(mpc_thread_cond_t *) );
#endif
#ifdef HAVE_PTHREAD_COND_WAIT
	sctk_add_func_type(pthread, cond_wait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *) );
#endif
#ifdef HAVE_PTHREAD_COND_TIMEDWAIT
	sctk_add_func_type(pthread, cond_timedwait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *,
	                           const struct timespec *) );
#endif
#ifdef HAVE_PTHREAD_COND_BROADCAST
	sctk_add_func_type(pthread, cond_broadcast, int (*)(mpc_thread_cond_t *) );
#endif
#ifdef HAVE_PTHREAD_COND_SIGNAL
	sctk_add_func_type(pthread, cond_signal, int (*)(mpc_thread_cond_t *) );
#endif
	/*attributs des conditions */
#ifdef HAVE_PTHREAD_CONDATTR_INIT
	sctk_add_func_type(pthread, condattr_init,
	                   int (*)(mpc_thread_condattr_t *) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_DESTROY
	sctk_add_func_type(pthread, condattr_destroy,
	                   int (*)(mpc_thread_condattr_t *) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_GETPSHARED
	sctk_add_func_type(pthread, condattr_getpshared,
	                   int (*)(const mpc_thread_condattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_SETPSHARED
	sctk_add_func_type(pthread, condattr_setpshared,
	                   int (*)(mpc_thread_condattr_t *, int) );
#endif

	/*spinlock */
#ifdef SCTK_USE_PTHREAD_SPINLOCK
#ifdef HAVE_PTHREAD_SPIN_INIT
	sctk_add_func_type(pthread, spin_init,
	                   int (*)(mpc_thread_spinlock_t *, int) );
#endif
#ifdef HAVE_PTHREAD_SPIN_LOCK
	sctk_add_func_type(pthread, spin_lock, int (*)(mpc_thread_spinlock_t *) );
#endif
#ifdef HAVE_PTHREAD_SPIN_TRYLOCK
	sctk_add_func_type(pthread, spin_trylock,
	                   int (*)(mpc_thread_spinlock_t *) );
#endif
#ifdef HAVE_PTHREAD_SPIN_UNLOCK
	sctk_add_func_type(pthread, spin_unlock,
	                   int (*)(mpc_thread_spinlock_t *) );
#endif
#ifdef HAVE_PTHREAD_SPIN_DESTROY
	sctk_add_func_type(pthread, spin_destroy,
	                   int (*)(mpc_thread_spinlock_t *) );
#endif
#endif  /*fin de SCTK_USE_PTHREAD_SPINLOCK */
	/*rwlock */
#ifdef SCTK_USE_PTHREAD_RWLOCK
#ifdef HAVE_PTHREAD_RWLOCK_INIT
	sctk_add_func_type(pthread, rwlock_init,
	                   int (*)(mpc_thread_rwlock_t *,
	                           const mpc_thread_rwlockattr_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_RDLOCK
	sctk_add_func_type(pthread, rwlock_rdlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_TRYRDLOCK
	sctk_add_func_type(pthread, rwlock_tryrdlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_WRLOCK
	sctk_add_func_type(pthread, rwlock_wrlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_TRYWRLOCK
	sctk_add_func_type(pthread, rwlock_trywrlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_UNLOCK
	sctk_add_func_type(pthread, rwlock_unlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_DESTROY
	sctk_add_func_type(pthread, rwlock_destroy,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
/*rwlock_attr*/
#ifdef HAVE_PTHREAD_RWLOCKATTR_INIT
	sctk_add_func_type(pthread, rwlockattr_init,
	                   int (*)(mpc_thread_rwlockattr_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCKATTR_SETPSHARED
	sctk_add_func_type(pthread, rwlockattr_setpshared,
	                   int (*)(mpc_thread_rwlockattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_RWLOCKATTR_GETPSHARED
	sctk_add_func_type(pthread, rwlockattr_getpshared,
	                   int (*)(const mpc_thread_rwlockattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCKATTR_DESTROY
	sctk_add_func_type(pthread, rwlockattr_destroy,
	                   int (*)(mpc_thread_rwlockattr_t *) );
#endif
#endif  /*fin de  SCTK_USE_PTHREAD_RWLOCK */
	/*barrier */
#ifdef SCTK_USE_PTHREAD_BARRIER
#ifdef HAVE_PTHREAD_BARRIER_INIT
	sctk_add_func_type(pthread, barrier_init,
	                   int (*)(mpc_thread_barrier_t *,
	                           const mpc_thread_barrierattr_t *, unsigned) );
#endif
#ifdef HAVE_PTHREAD_BARRIER_WAIT
	sctk_add_func_type(pthread, barrier_wait,
	                   int (*)(mpc_thread_barrier_t *) );
#endif
#ifdef HAVE_PTHREAD_BARRIER_DESTROY
	sctk_add_func_type(pthread, barrier_destroy,
	                   int (*)(mpc_thread_barrier_t *) );
#endif

/*barrier_attr*/
#ifdef HAVE_PTHREAD_BARRIERATTR_INIT
	sctk_add_func_type(pthread, barrierattr_init,
	                   int (*)(mpc_thread_barrierattr_t *) );
#endif
#ifdef HAVE_PTHREAD_BARRIERATTR_SETPSHARED
	sctk_add_func_type(pthread, barrierattr_setpshared,
	                   int (*)(mpc_thread_barrierattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_BARRIERATTR_GETPSHARED
	sctk_add_func_type(pthread, barrierattr_getpshared,
	                   int (*)(const mpc_thread_barrierattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_BARRIERATTR_DESTROY
	sctk_add_func_type(pthread, barrierattr_destroy,
	                   int (*)(mpc_thread_barrierattr_t *) );
#endif
#endif  /*fin de USE_PTHREAD_BARRIER */
/*Non portable */
#ifdef HAVE_PTHREAD_GETATTR_NP
	sctk_add_func_type(pthread, getattr_np,
	                   int (*)(mpc_thread_t, mpc_thread_attr_t *) );
#endif


/*Non impl�ment� dans ethread(_mxn)*/
#ifdef HAVE_PTHREAD_ATTR_GETGUARDSIZE
	sctk_add_func_type(pthread, attr_getguardsize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETGUARDSIZE
	sctk_add_func_type(pthread, attr_setguardsize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSCHEDPARAM
	sctk_add_func_type(pthread, attr_getschedparam,
	                   int (*)(const mpc_thread_attr_t *,
	                           struct sched_param *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPARAM
	sctk_add_func_type(pthread, attr_setschedparam,
	                   int (*)(mpc_thread_attr_t *,
	                           const struct sched_param *) );
#endif

#ifdef HAVE_PTHREAD_ATFORK
	sctk_add_func_type(pthread, atfork,
	                   int (*)(void (*prepare)(void),
	                           void (*parent)(void), void (*child)(void) ) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_GETCLOCK
	sctk_add_func_type(pthread, condattr_getclock,
	                   int (*)(mpc_thread_condattr_t *, clockid_t *) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_SETCLOCK
	sctk_add_func_type(pthread, condattr_setclock,
	                   int (*)(mpc_thread_condattr_t *, clockid_t) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_GETPRIOCEILING
	sctk_add_func_type(pthread, mutexattr_getprioceiling,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_SETPRIOCEILING
	sctk_add_func_type(pthread, mutexattr_setprioceiling,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_GETPROTOCOL
	sctk_add_func_type(pthread, mutexattr_getprotocol,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_SETPROTOCOL
	sctk_add_func_type(pthread, mutexattr_setprotocol,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_TIMEDLOCK
	sctk_add_func_type(pthread, mutex_timedlock,
	                   int (*)(mpc_thread_mutex_t *,
	                           const struct timespec *) );
#endif

#define sctk_add_func_type(newlib, func, t)    _funcptr_mpc_thread_ ## func = (t)newlib ## _ ## func

#ifdef HAVE_PTHREAD_GETCONCURRENCY
	sctk_add_func_type(pthread, getconcurrency, int (*)(void) );
#endif
#ifdef HAVE_PTHREAD_SETCONCURRENCY
	sctk_add_func_type(pthread, setconcurrency, int (*)(int) );
#endif
#ifdef HAVE_PTHREAD_SETSCHEDPRIO

/*
 * sctk_add_func_type (pthread,setschedprio,int (*)(mpc_thread_t , int ));
 */
#endif
}
