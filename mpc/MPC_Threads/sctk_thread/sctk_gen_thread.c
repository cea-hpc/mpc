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
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"

#define not_initialized()  sctk_formated_dbg_print_abort(SCTK_DBG_INFO,"You have to initialize the thread library before using MPC thread API!")

static inline void
sctk_touch_ptr (const void *ptr)
{
  assume (ptr == NULL);
}

static inline void
sctk_touch_func (void (*ptr) (void))
{
  assume (ptr == NULL);
}

static inline void
sctk_touch_func_s (void *(*ptr) (void *))
{
  assume (ptr == NULL);
}
static inline void
sctk_touch_func_n (void (*ptr) (void *))
{
  assume (ptr == NULL);
}

static inline void
sctk_touch_thread (const sctk_thread_t ptr)
{
  assume (ptr == NULL);
}

static inline void
sctk_touch_int (const int ptr)
{
  assume (ptr == 0);
}

static inline void
sctk_touch_long (const long ptr)
{
  assume (ptr == 0);
}

static double
sctk_gen_thread_get_activity (int i)
{
  sctk_nodebug ("sctk_gen_thread_get_activity");
  return -1;
}

static int
sctk_gen_thread_atfork (void (*__prepare) (void), void (*__parent) (void),
			void (*__child) (void))
{
  not_initialized ();
  sctk_touch_func (__prepare);
  sctk_touch_func (__parent);
  sctk_touch_func (__child);
  return 0;
}

static int
sctk_gen_thread_attr_destroy (sctk_thread_attr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_attr_getdetachstate (const sctk_thread_attr_t * __attr,
				     int *__detachstate)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__detachstate);
  return 0;
}

static int
sctk_gen_thread_attr_getguardsize (const sctk_thread_attr_t *
				   restrict __attr,
				   size_t * restrict __guardsize)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__guardsize);
  return 0;
}

static int
sctk_gen_thread_attr_getinheritsched (const sctk_thread_attr_t *
				      restrict __attr,
				      int *restrict __inherit)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__inherit);
  return 0;
}

static int
sctk_gen_thread_attr_getschedparam (const sctk_thread_attr_t *
				    restrict __attr,
				    struct sched_param *restrict __param)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__param);
  return 0;
}

static int
sctk_gen_thread_attr_getschedpolicy (const sctk_thread_attr_t *
				     restrict __attr, int *restrict __policy)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__policy);
  return 0;
}

static int
sctk_gen_thread_attr_getscope (const sctk_thread_attr_t * restrict __attr,
			       int *restrict __scope)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__scope);
  return 0;
}

static int
sctk_gen_thread_attr_getstackaddr (const sctk_thread_attr_t *
				   restrict __attr,
				   void **restrict __stackaddr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__stackaddr);
  return 0;
}

static int
sctk_gen_thread_attr_getstack (const sctk_thread_attr_t * restrict __attr,
			       void **restrict __stackaddr,
			       size_t * restrict __stacksize)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__stackaddr);
  sctk_touch_ptr (__stacksize);
  return 0;
}

static int
sctk_gen_thread_attr_getstacksize (const sctk_thread_attr_t *
				   restrict __attr,
				   size_t * restrict __stacksize)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__stacksize);
  return 0;
}

static int
sctk_gen_thread_attr_setbinding (sctk_thread_attr_t * __attr, int __binding)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__binding);
  return 0;
}

static int
sctk_gen_thread_attr_getbinding (sctk_thread_attr_t * __attr, int *__binding)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__binding);
  return 0;
}

static int
sctk_gen_thread_attr_init (sctk_thread_attr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_attr_setdetachstate (sctk_thread_attr_t * __attr,
				     int __detachstate)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__detachstate);
  return 0;
}

static int
sctk_gen_thread_attr_setguardsize (sctk_thread_attr_t * __attr,
				   size_t __guardsize)
{
  #warning "Calls to pthread_attr_setguardsize () allowed but without effect"
  //not_initialized ();
  //sctk_touch_ptr (__attr);
  //sctk_touch_long (__guardsize);
  return 0;
}

static int
sctk_gen_thread_attr_setinheritsched (sctk_thread_attr_t * __attr,
				      int __inherit)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__inherit);
  return 0;
}

static int
sctk_gen_thread_attr_setschedparam (sctk_thread_attr_t * restrict __attr,
				    const struct sched_param *restrict
				    __param)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__param);
  return 0;
}

static int
sctk_gen_thread_sched_get_priority_max (int policy)
{
  not_initialized ();
  sctk_touch_int (policy);
  return 0;
}
static int
sctk_gen_thread_sched_get_priority_min (int policy)
{
  not_initialized ();
  sctk_touch_int (policy);
  return 0;
}

static int
sctk_gen_thread_attr_setschedpolicy (sctk_thread_attr_t * __attr,
				     int __policy)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__policy);
  return 0;
}

static int
sctk_gen_thread_attr_setscope (sctk_thread_attr_t * __attr, int __scope)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__scope);
  return 0;
}

static int
sctk_gen_thread_attr_setstackaddr (sctk_thread_attr_t * __attr,
				   void *__stackaddr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__stackaddr);
  return 0;
}

static int
sctk_gen_thread_attr_setstack (sctk_thread_attr_t * __attr,
			       void *__stackaddr, size_t __stacksize)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_long (__stacksize);
  sctk_touch_ptr (__stackaddr);
  return 0;
}

static int
sctk_gen_thread_attr_setstacksize (sctk_thread_attr_t * __attr,
				   size_t __stacksize)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_long (__stacksize);
  return 0;
}

static int
sctk_gen_thread_barrierattr_destroy (sctk_thread_barrierattr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_barrierattr_init (sctk_thread_barrierattr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_barrierattr_setpshared (sctk_thread_barrierattr_t * __attr,
					int __pshared)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__pshared);
  return 0;
}
static int
sctk_gen_thread_barrierattr_getpshared (const sctk_thread_barrierattr_t *
					__attr, int *__pshared)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__pshared);
  return 0;
}
static int
sctk_gen_thread_barrier_destroy (sctk_thread_barrier_t * __barrier)
{
  not_initialized ();
  sctk_touch_ptr (__barrier);
  return 0;
}

static int
sctk_gen_thread_barrier_init (sctk_thread_barrier_t * restrict __barrier,
			      const sctk_thread_barrierattr_t *
			      restrict __attr, unsigned int __count)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__count);
  sctk_touch_ptr (__barrier);
  return 0;
}

static int
sctk_gen_thread_barrier_wait (sctk_thread_barrier_t * __barrier)
{
  not_initialized ();
  sctk_touch_ptr (__barrier);
  return 0;
}

static int
sctk_gen_thread_cancel (sctk_thread_t __cancelthread)
{
  not_initialized ();
  sctk_touch_ptr (__cancelthread);
  return 0;
}

static int
sctk_gen_thread_condattr_destroy (sctk_thread_condattr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_condattr_getpshared (const sctk_thread_condattr_t *
				     restrict __attr, int *restrict __pshared)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__pshared);
  return 0;
}

static int
sctk_gen_thread_condattr_init (sctk_thread_condattr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_condattr_setpshared (sctk_thread_condattr_t * __attr,
				     int __pshared)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__pshared);
  return 0;
}

static int
sctk_gen_thread_cond_broadcast (sctk_thread_cond_t * __cond)
{
  not_initialized ();
  sctk_touch_ptr (__cond);
  return 0;
}

static int
sctk_gen_thread_cond_destroy (sctk_thread_cond_t * __cond)
{
  not_initialized ();
  sctk_touch_ptr (__cond);
  return 0;
}
static int
sctk_gen_thread_condattr_setclock (sctk_thread_condattr_t *
				   attr, clockid_t clock_id)
{
  not_initialized ();
  sctk_touch_ptr (attr);
  sctk_touch_int (clock_id);
  return 0;
}
static int
sctk_gen_thread_condattr_getclock (sctk_thread_condattr_t *
				   attr, clockid_t * clock_id)
{
  not_initialized ();
  sctk_touch_ptr (attr);
  sctk_touch_ptr (clock_id);
  return 0;
}
static int
sctk_gen_thread_cond_init (sctk_thread_cond_t * restrict __cond,
			   const sctk_thread_condattr_t *
			   restrict __cond_attr)
{
  not_initialized ();
  sctk_touch_ptr (__cond);
  sctk_touch_ptr (__cond_attr);
  return 0;
}

static int
sctk_gen_thread_cond_signal (sctk_thread_cond_t * __cond)
{
  not_initialized ();
  sctk_touch_ptr (__cond);
  return 0;
}

static int
sctk_gen_thread_cond_timedwait (sctk_thread_cond_t * restrict __cond,
				sctk_thread_mutex_t * restrict __mutex,
				const struct timespec *restrict __abstime)
{
  not_initialized ();
  sctk_touch_ptr (__cond);
  sctk_touch_ptr (__mutex);
  sctk_touch_ptr (__abstime);
  return 0;
}

static int
sctk_gen_thread_cond_wait (sctk_thread_cond_t * restrict __cond,
			   sctk_thread_mutex_t * restrict __mutex)
{
  not_initialized ();
  sctk_touch_ptr (__cond);
  sctk_touch_ptr (__mutex);
  return 0;
}

static int
sctk_gen_thread_create (sctk_thread_t * restrict __threadp,
			const sctk_thread_attr_t * restrict __attr,
			void *(*__start_routine) (void *),
			void *restrict __arg)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__threadp);
  sctk_touch_func_s (__start_routine);
  sctk_touch_ptr (__arg);
  return 0;
}
static int
sctk_gen_thread_user_create (sctk_thread_t * restrict __threadp,
			     const sctk_thread_attr_t * restrict __attr,
			     void *(*__start_routine) (void *),
			     void *restrict __arg)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__threadp);
  sctk_touch_func_s (__start_routine);
  sctk_touch_ptr (__arg);
  return 0;
}

static int
sctk_gen_thread_detach (sctk_thread_t __th)
{
  not_initialized ();
  sctk_touch_ptr (__th);
  return 0;
}

static int
sctk_gen_thread_equal (sctk_thread_t __thread1, sctk_thread_t __thread2)
{
  not_initialized ();
  sctk_touch_ptr (__thread1);
  sctk_touch_ptr (__thread2);
  return 0;
}

static void
sctk_gen_thread_exit (void *__retval)
{
  not_initialized ();
  sctk_touch_ptr (__retval);

}

static int
sctk_gen_thread_getconcurrency (void)
{
  not_initialized ();
  return 0;
}

static int
sctk_gen_thread_getcpuclockid (sctk_thread_t __thread_id,
			       clockid_t * __clock_id)
{
  not_initialized ();
  sctk_touch_ptr (__thread_id);
  sctk_touch_ptr (__clock_id);
  return 0;
}

static int
sctk_gen_thread_getschedparam (sctk_thread_t __target_thread,
			       int *restrict __policy,
			       struct sched_param *restrict __param)
{
  not_initialized ();
  sctk_touch_ptr (__target_thread);
  sctk_touch_ptr (__policy);
  sctk_touch_ptr (__param);
  return 0;
}

static void *
sctk_gen_thread_getspecific (sctk_thread_key_t __key)
{
  not_initialized ();
  sctk_touch_int (__key);
  return 0;
}

static int
sctk_gen_thread_join (sctk_thread_t __th, void **__thread_return)
{
  not_initialized ();
  sctk_touch_ptr (__th);
  sctk_touch_ptr (__thread_return);
  return 0;
}

static int
sctk_gen_thread_kill (sctk_thread_t thread, int signo)
{
  not_initialized ();
  sctk_touch_ptr (thread);
  sctk_touch_int (signo);
  return 0;
}

static int
sctk_gen_thread_sigsuspend (sigset_t * set)
{
  not_initialized ();
  sctk_touch_ptr (set);
  return 0;
}

static int
sctk_gen_thread_sigpending (sigset_t * set)
{
  not_initialized ();
  sctk_touch_ptr (set);
  return 0;
}

static int
sctk_gen_thread_sigmask (int how, const sigset_t * newmask,
			 sigset_t * oldmask)
{
  not_initialized ();
  sctk_touch_int (how);
  sctk_touch_ptr (newmask);
  sctk_touch_ptr (oldmask);
  return 0;
}

static int
sctk_gen_thread_key_create (sctk_thread_key_t * __key,
			    void (*__destr_function) (void *))
{
  not_initialized ();
  sctk_touch_ptr (__key);
  sctk_touch_func_n (__destr_function);
  return 0;
}

static int
sctk_gen_thread_key_delete (sctk_thread_key_t __key)
{
  not_initialized ();
  sctk_touch_int (__key);
  return 0;
}

static int
sctk_gen_thread_mutexattr_destroy (sctk_thread_mutexattr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_mutexattr_getpshared (const sctk_thread_mutexattr_t *
				      restrict __attr,
				      int *restrict __pshared)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__pshared);
  return 0;
}

static int
sctk_gen_thread_mutexattr_getprioceiling (const sctk_thread_mutexattr_t *
					  __attr, int *__prioceiling)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__prioceiling);
  return 0;
}

static int
sctk_gen_thread_mutexattr_setprioceiling (sctk_thread_mutexattr_t *
					  __attr, int __prioceiling)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__prioceiling);
  return 0;
}

static int
sctk_gen_thread_mutexattr_getprotocol (const sctk_thread_mutexattr_t *
				       __attr, int *__protocol)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__protocol);
  return 0;
}

static int
sctk_gen_thread_mutexattr_setprotocol (sctk_thread_mutexattr_t *
				       __attr, int __protocol)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__protocol);
  return 0;
}

static int
sctk_gen_thread_mutexattr_gettype (const sctk_thread_mutexattr_t *
				   restrict __attr, int *restrict __kind)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__kind);
  return 0;
}

static int
sctk_gen_thread_mutexattr_init (sctk_thread_mutexattr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_mutexattr_setpshared (sctk_thread_mutexattr_t * __attr,
				      int __pshared)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__pshared);
  return 0;
}

static int
sctk_gen_thread_mutexattr_settype (sctk_thread_mutexattr_t * __attr,
				   int __kind)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__kind);
  return 0;
}

static int
sctk_gen_thread_mutex_destroy (sctk_thread_mutex_t * __mutex)
{
  not_initialized ();
  sctk_touch_ptr (__mutex);
  return 0;
}

static int
sctk_gen_thread_mutex_init (sctk_thread_mutex_t * restrict __mutex,
			    const sctk_thread_mutexattr_t *
			    restrict __mutex_attr)
{
  not_initialized ();
  sctk_touch_ptr (__mutex);
  sctk_touch_ptr (__mutex_attr);
  return 0;
}

static int
sctk_gen_thread_mutex_lock (sctk_thread_mutex_t * __mutex)
{
  not_initialized ();
  sctk_touch_ptr (__mutex);
  return 0;
}

static int
sctk_gen_thread_mutex_spinlock (sctk_thread_mutex_t * __mutex)
{
  not_initialized ();
  sctk_touch_ptr (__mutex);
  return 0;
}

static int
sctk_gen_thread_mutex_timedlock (sctk_thread_mutex_t * restrict __mutex,
				 const struct timespec *restrict __abstime)
{
  not_initialized ();
  sctk_touch_ptr (__mutex);
  sctk_touch_ptr (__abstime);
  return 0;
}

static int
sctk_gen_thread_mutex_trylock (sctk_thread_mutex_t * __mutex)
{
  not_initialized ();
  sctk_touch_ptr (__mutex);
  return 0;
}

static int
sctk_gen_thread_mutex_unlock (sctk_thread_mutex_t * __mutex)
{
  not_initialized ();
  sctk_touch_ptr (__mutex);
  return 0;
}

static int
sctk_gen_thread_sem_init (sctk_thread_sem_t * sem, int pshared,
			  unsigned int value)
{
  not_initialized ();
  sctk_touch_ptr (sem);
  sctk_touch_int (pshared);
  sctk_touch_int (value);
  return 0;
}
static int
sctk_gen_thread_sem_wait (sctk_thread_sem_t * sem)
{
  not_initialized ();
  sctk_touch_ptr (sem);
  return 0;
}
static int
sctk_gen_thread_sem_trywait (sctk_thread_sem_t * sem)
{
  not_initialized ();
  sctk_touch_ptr (sem);
  return 0;
}
static int
sctk_gen_thread_sem_post (sctk_thread_sem_t * sem)
{
  not_initialized ();
  sctk_touch_ptr (sem);
  return 0;
}
static int
sctk_gen_thread_sem_getvalue (sctk_thread_sem_t * sem, int *sval)
{
  not_initialized ();
  sctk_touch_ptr (sem);
  sctk_touch_ptr (sval);
  return 0;
}
static int
sctk_gen_thread_sem_destroy (sctk_thread_sem_t * sem)
{
  not_initialized ();
  sctk_touch_ptr (sem);
  return 0;
}

static sctk_thread_sem_t *
sctk_gen_thread_sem_open (const char *name, int oflag, ...)
{
  not_initialized ();
  sctk_touch_ptr (name);
  sctk_touch_int (oflag);
  return NULL;
}
static int
sctk_gen_thread_sem_close (sctk_thread_sem_t * sem)
{
  not_initialized ();
  sctk_touch_ptr (sem);
  return 0;
}
static int
sctk_gen_thread_sem_unlink (const char *name)
{
  not_initialized ();
  sctk_touch_ptr (name);
  return 0;
}

static int
sctk_gen_thread_once (sctk_thread_once_t * __once_control,
		      void (*__init_routine) (void))
{
  not_initialized ();
  sctk_touch_ptr (__once_control);
  sctk_touch_func (__init_routine);
  return 0;
}

static int
sctk_gen_thread_rwlockattr_destroy (sctk_thread_rwlockattr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_rwlockattr_getpshared (const sctk_thread_rwlockattr_t *
				       restrict __attr,
				       int *restrict __pshared)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__pshared);
  return 0;
}

static int
sctk_gen_thread_rwlockattr_init (sctk_thread_rwlockattr_t * __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  return 0;
}

static int
sctk_gen_thread_rwlockattr_setpshared (sctk_thread_rwlockattr_t * __attr,
				       int __pshared)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_int (__pshared);
  return 0;
}

static int
sctk_gen_thread_rwlock_destroy (sctk_thread_rwlock_t * __rwlock)
{
  not_initialized ();
  sctk_touch_ptr (__rwlock);
  return 0;
}

static int
sctk_gen_thread_rwlock_init (sctk_thread_rwlock_t * restrict __rwlock,
			     const sctk_thread_rwlockattr_t * restrict __attr)
{
  not_initialized ();
  sctk_touch_ptr (__attr);
  sctk_touch_ptr (__rwlock);
  return 0;
}

static int
sctk_gen_thread_rwlock_rdlock (sctk_thread_rwlock_t * __rwlock)
{
  not_initialized ();
  sctk_touch_ptr (__rwlock);
  return 0;
}

static int
sctk_gen_thread_rwlock_timedrdlock (sctk_thread_rwlock_t *
				    restrict __rwlock,
				    const struct timespec *restrict __abstime)
{
  not_initialized ();
  sctk_touch_ptr (__rwlock);
  sctk_touch_ptr (__abstime);
  return 0;
}

static int
sctk_gen_thread_rwlock_timedwrlock (sctk_thread_rwlock_t *
				    restrict __rwlock,
				    const struct timespec *restrict __abstime)
{
  not_initialized ();
  sctk_touch_ptr (__rwlock);
  sctk_touch_ptr (__abstime);
  return 0;
}

static int
sctk_gen_thread_rwlock_tryrdlock (sctk_thread_rwlock_t * __rwlock)
{
  not_initialized ();
  sctk_touch_ptr (__rwlock);
  return 0;
}

static int
sctk_gen_thread_rwlock_trywrlock (sctk_thread_rwlock_t * __rwlock)
{
  not_initialized ();
  sctk_touch_ptr (__rwlock);
  return 0;
}

static int
sctk_gen_thread_rwlock_unlock (sctk_thread_rwlock_t * __rwlock)
{
  not_initialized ();
  sctk_touch_ptr (__rwlock);
  return 0;
}

static int
sctk_gen_thread_rwlock_wrlock (sctk_thread_rwlock_t * __rwlock)
{
  not_initialized ();
  sctk_touch_ptr (__rwlock);
  return 0;
}

static sctk_thread_t
sctk_gen_thread_self (void)
{
  not_initialized ();
  return 0;
}

static sctk_thread_t
sctk_gen_thread_self_check (void)
{
  not_initialized ();
  return 0;
}

static int
sctk_gen_thread_setcancelstate (int __state, int *__oldstate)
{
  not_initialized ();
  sctk_touch_int (__state);
  sctk_touch_ptr (__oldstate);
  return 0;
}

static int
sctk_gen_thread_setcanceltype (int __type, int *__oldtype)
{
  not_initialized ();
  sctk_touch_int (__type);
  sctk_touch_ptr (__oldtype);
  return 0;
}

static int
sctk_gen_thread_setconcurrency (int __level)
{
  #warning "Calls to pthread_setconcurrency () allowed but without effect"
  //not_initialized ();
  //sctk_touch_int (__level);
  return 0;
}

static int
sctk_gen_thread_setschedprio (sctk_thread_t __p, int __i)
{
  not_initialized ();
  sctk_touch_int (__i);
  sctk_touch_thread (__p);
  return 0;
}

static int
sctk_gen_thread_setschedparam (sctk_thread_t __target_thread, int __policy,
			       const struct sched_param *__param)
{
  not_initialized ();
  sctk_touch_thread (__target_thread);
  sctk_touch_int (__policy);
  sctk_touch_ptr (__param);
  return 0;
}

static int
sctk_gen_thread_setspecific (sctk_thread_key_t __key, const void *__pointer)
{
  not_initialized ();
  sctk_touch_int (__key);
  sctk_touch_ptr (__pointer);
  return 0;
}

static int
sctk_gen_thread_spin_destroy (sctk_thread_spinlock_t * __lock)
{
  not_initialized ();
  sctk_touch_ptr (__lock);
  return 0;
}

static int
sctk_gen_thread_spin_init (sctk_thread_spinlock_t * __lock, int __pshared)
{
  not_initialized ();
  sctk_touch_ptr (__lock);
  sctk_touch_int (__pshared);
  return 0;
}

static int
sctk_gen_thread_spin_lock (sctk_thread_spinlock_t * __lock)
{
  not_initialized ();
  sctk_touch_ptr (__lock);
  return 0;
}

static int
sctk_gen_thread_spin_trylock (sctk_thread_spinlock_t * __lock)
{
  not_initialized ();
  sctk_touch_ptr (__lock);
  return 0;
}

static int
sctk_gen_thread_spin_unlock (sctk_thread_spinlock_t * __lock)
{
  not_initialized ();
  sctk_touch_ptr (__lock);
  return 0;
}

static void
sctk_gen_thread_testcancel (void)
{
  not_initialized ();
}

static int
sctk_gen_thread_yield (void)
{
  not_initialized ();
  return 0;
}

static int
sctk_gen_thread_dump (char *file)
{
  not_initialized ();
  sctk_touch_ptr (file);
  return 0;
}
static int
sctk_gen_thread_dump_clean (void)
{
  not_initialized ();
  return 0;
}
static int
sctk_gen_thread_migrate (void)
{
  not_initialized ();
  return 0;
}

static int
sctk_gen_thread_restore (sctk_thread_t thread, char *type, int vp)
{
  not_initialized ();
  sctk_touch_int (vp);
  sctk_touch_ptr (type);
  sctk_touch_ptr (thread);
  return 0;
}

static void
sctk_gen_thread_wait_for_value (int *data, int value)
{
  sctk_thread_wait_for_value_and_poll (data, value, NULL, NULL);
}

static void
sctk_gen_thread_wait_for_value_and_poll (int *data, int value,
					 void (*func) (void *), void *arg)
{
  not_initialized ();
  sctk_touch_ptr (data);
  sctk_touch_func_n (func);
  sctk_touch_ptr (arg);
  sctk_touch_int (value);
}

static void
sctk_gen_thread_freeze_thread_on_vp (sctk_thread_mutex_t * lock, void **list)
{
  not_initialized ();
  sctk_touch_ptr (lock);
  sctk_touch_ptr (list);

}

static void
sctk_gen_thread_wake_thread_on_vp (void **list)
{
  not_initialized ();
  sctk_touch_ptr (list);

}

static int
sctk_gen_thread_get_vp ()
{
  return -1;
}

static int
sctk_gen_thread_proc_migration (const int cpu)
{
  assume (cpu >= 0);
  return sctk_thread_get_vp ();
}

static int
sctk_gen_thread_getattr_np (sctk_thread_t th, sctk_thread_attr_t * attr)
{
  not_initialized ();
  sctk_touch_ptr (th);
  sctk_touch_ptr (attr);
  return 0;
}
static int
sctk_gen_thread_rwlockattr_getkind_np (sctk_thread_rwlockattr_t * attr,
				       int *pref)
{
  not_initialized ();
  sctk_touch_ptr (attr);
  sctk_touch_ptr (pref);
  return 0;
}
static int
sctk_gen_thread_rwlockattr_setkind_np (sctk_thread_rwlockattr_t * attr,
				       int pref)
{
  not_initialized ();
  sctk_touch_ptr (attr);
  sctk_touch_int (pref);
  return 0;
}
static void
sctk_gen_thread_kill_other_threads_np (void)
{
  not_initialized ();
}

double (*__sctk_ptr_thread_get_activity) (int i) =
  sctk_gen_thread_get_activity;

int (*__sctk_ptr_thread_proc_migration) (const int cpu) =
  sctk_gen_thread_proc_migration;

int (*__sctk_ptr_thread_get_vp) (void) = sctk_gen_thread_get_vp;

int (*__sctk_ptr_thread_atfork) (void (*__prepare) (void),
				 void (*__parent) (void),
				 void (*__child) (void)) =
  sctk_gen_thread_atfork;

int (*__sctk_ptr_thread_attr_destroy) (sctk_thread_attr_t * __attr) =
  sctk_gen_thread_attr_destroy;
int (*__sctk_ptr_thread_attr_getdetachstate) (const sctk_thread_attr_t *
					      __attr, int *__detachstate) =
  sctk_gen_thread_attr_getdetachstate;
int (*__sctk_ptr_thread_attr_getguardsize) (const sctk_thread_attr_t *
					    __attr, size_t * __guardsize) =
  sctk_gen_thread_attr_getguardsize;
int (*__sctk_ptr_thread_attr_getinheritsched) (const sctk_thread_attr_t *
					       __attr, int *__inherit) =
  sctk_gen_thread_attr_getinheritsched;
int (*__sctk_ptr_thread_attr_getschedparam) (const sctk_thread_attr_t *
					     __attr,
					     struct sched_param *
					     __param) =
  sctk_gen_thread_attr_getschedparam;
int (*__sctk_ptr_thread_attr_getschedpolicy) (const sctk_thread_attr_t *
					      __attr, int *__policy) =
  sctk_gen_thread_attr_getschedpolicy;
int (*__sctk_ptr_thread_attr_getscope) (const sctk_thread_attr_t * __attr,
					int *__scope) =
  sctk_gen_thread_attr_getscope;
int (*__sctk_ptr_thread_attr_getstackaddr) (const sctk_thread_attr_t *
					    __attr, void **__stackaddr) =
  sctk_gen_thread_attr_getstackaddr;
int (*__sctk_ptr_thread_attr_getstack) (const sctk_thread_attr_t * __attr,
					void **__stackaddr,
					size_t * __stacksize) =
  sctk_gen_thread_attr_getstack;
int (*__sctk_ptr_thread_attr_getstacksize) (const sctk_thread_attr_t *
					    __attr, size_t * __stacksize) =
  sctk_gen_thread_attr_getstacksize;
int (*__sctk_ptr_thread_attr_init) (sctk_thread_attr_t * __attr) =
  sctk_gen_thread_attr_init;
int (*__sctk_ptr_thread_attr_setdetachstate) (sctk_thread_attr_t * __attr,
					      int __detachstate) =
  sctk_gen_thread_attr_setdetachstate;
int (*__sctk_ptr_thread_attr_setguardsize) (sctk_thread_attr_t * __attr,
					    size_t __guardsize) =
  sctk_gen_thread_attr_setguardsize;
int (*__sctk_ptr_thread_attr_setinheritsched) (sctk_thread_attr_t * __attr,
					       int __inherit) =
  sctk_gen_thread_attr_setinheritsched;
int (*__sctk_ptr_thread_attr_setschedparam) (sctk_thread_attr_t * __attr,
					     const struct sched_param *
					     __param) =
  sctk_gen_thread_attr_setschedparam;
int (*__sctk_ptr_thread_attr_setschedpolicy) (sctk_thread_attr_t * __attr,
					      int __policy) =
  sctk_gen_thread_attr_setschedpolicy;
int (*__sctk_ptr_thread_attr_setscope) (sctk_thread_attr_t * __attr,
					int __scope) =
  sctk_gen_thread_attr_setscope;
int (*__sctk_ptr_thread_attr_setstackaddr) (sctk_thread_attr_t * __attr,
					    void *__stackaddr) =
  sctk_gen_thread_attr_setstackaddr;
int (*__sctk_ptr_thread_attr_setstack) (sctk_thread_attr_t * __attr,
					void *__stackaddr,
					size_t __stacksize) =
  sctk_gen_thread_attr_setstack;
int (*__sctk_ptr_thread_attr_setstacksize) (sctk_thread_attr_t * __attr,
					    size_t __stacksize) =
  sctk_gen_thread_attr_setstacksize;


int
  (*__sctk_ptr_thread_attr_setbinding) (sctk_thread_attr_t * __attr,
					int __binding) =
  sctk_gen_thread_attr_setbinding;
int (*__sctk_ptr_thread_attr_getbinding) (sctk_thread_attr_t * __attr,
					  int *__binding) =
  sctk_gen_thread_attr_getbinding;


int (*__sctk_ptr_thread_barrierattr_destroy) (sctk_thread_barrierattr_t *
					      __attr) =
  sctk_gen_thread_barrierattr_destroy;
int (*__sctk_ptr_thread_barrierattr_init) (sctk_thread_barrierattr_t *
					   __attr) =
  sctk_gen_thread_barrierattr_init;
int (*__sctk_ptr_thread_barrierattr_setpshared) (sctk_thread_barrierattr_t
						 * __attr, int __pshared) =
  sctk_gen_thread_barrierattr_setpshared;
int (*__sctk_ptr_thread_barrierattr_getpshared) (const
						 sctk_thread_barrierattr_t
						 * __attr,
						 int *__pshared) =
  sctk_gen_thread_barrierattr_getpshared;
int (*__sctk_ptr_thread_barrier_destroy) (sctk_thread_barrier_t *
					  __barrier) =
  sctk_gen_thread_barrier_destroy;
int (*__sctk_ptr_thread_barrier_init) (sctk_thread_barrier_t *
				       restrict __barrier,
				       const sctk_thread_barrierattr_t *
				       restrict __attr,
				       unsigned int __count) =
  sctk_gen_thread_barrier_init;
int (*__sctk_ptr_thread_barrier_wait) (sctk_thread_barrier_t * __barrier) =
  sctk_gen_thread_barrier_wait;

int (*__sctk_ptr_thread_cancel) (sctk_thread_t __cancelthread) =
  sctk_gen_thread_cancel;
int (*__sctk_ptr_thread_condattr_destroy) (sctk_thread_condattr_t *
					   __attr) =
  sctk_gen_thread_condattr_destroy;
int (*__sctk_ptr_thread_condattr_getpshared) (const sctk_thread_condattr_t
					      * __attr, int *__pshared) =
  sctk_gen_thread_condattr_getpshared;
int (*__sctk_ptr_thread_condattr_init) (sctk_thread_condattr_t * __attr) =
  sctk_gen_thread_condattr_init;
int (*__sctk_ptr_thread_condattr_setpshared) (sctk_thread_condattr_t *
					      __attr, int __pshared) =
  sctk_gen_thread_condattr_setpshared;
int (*__sctk_ptr_thread_condattr_setclock) (sctk_thread_condattr_t * attr,
					    clockid_t clock_id) =
  sctk_gen_thread_condattr_setclock;
int (*__sctk_ptr_thread_condattr_getclock) (sctk_thread_condattr_t * attr,
					    clockid_t * clock_id) =
  sctk_gen_thread_condattr_getclock;
int (*__sctk_ptr_thread_cond_broadcast) (sctk_thread_cond_t * __cond) =
  sctk_gen_thread_cond_broadcast;
int (*__sctk_ptr_thread_cond_destroy) (sctk_thread_cond_t * __cond) =
  sctk_gen_thread_cond_destroy;
int (*__sctk_ptr_thread_cond_init) (sctk_thread_cond_t * __cond,
				    const sctk_thread_condattr_t *
				    __cond_attr) = sctk_gen_thread_cond_init;
int (*__sctk_ptr_thread_cond_signal) (sctk_thread_cond_t * __cond) =
  sctk_gen_thread_cond_signal;
int (*__sctk_ptr_thread_cond_timedwait) (sctk_thread_cond_t * __cond,
					 sctk_thread_mutex_t * __mutex,
					 const struct timespec *
					 __abstime) =
  sctk_gen_thread_cond_timedwait;
int (*__sctk_ptr_thread_cond_wait) (sctk_thread_cond_t * __cond,
				    sctk_thread_mutex_t * __mutex) =
  sctk_gen_thread_cond_wait;
int (*__sctk_ptr_thread_create) (sctk_thread_t * __threadp,
				 const sctk_thread_attr_t * __attr,
				 void *(*__start_routine) (void *),
				 void *__arg) = sctk_gen_thread_create;
int (*__sctk_ptr_thread_user_create) (sctk_thread_t * __threadp,
				      const sctk_thread_attr_t * __attr,
				      void *(*__start_routine) (void *),
				      void *__arg) =
  sctk_gen_thread_user_create;
int (*__sctk_ptr_thread_detach) (sctk_thread_t __th) = sctk_gen_thread_detach;
int (*__sctk_ptr_thread_equal) (sctk_thread_t __thread1,
				sctk_thread_t __thread2) =
  sctk_gen_thread_equal;
void (*__sctk_ptr_thread_exit) (void *__retval) = sctk_gen_thread_exit;
int (*__sctk_ptr_thread_getconcurrency) (void) =
  sctk_gen_thread_getconcurrency;
int (*__sctk_ptr_thread_getcpuclockid) (sctk_thread_t __thread_id,
					clockid_t * __clock_id) =
  sctk_gen_thread_getcpuclockid;
int (*__sctk_ptr_thread_getschedparam) (sctk_thread_t __target_thread,
					int *__policy,
					struct sched_param * __param) =
  sctk_gen_thread_getschedparam;
int (*__sctk_ptr_thread_sched_get_priority_max) (int policy) =
  sctk_gen_thread_sched_get_priority_max;
int (*__sctk_ptr_thread_sched_get_priority_min) (int policy) =
  sctk_gen_thread_sched_get_priority_min;


void *(*__sctk_ptr_thread_getspecific) (sctk_thread_key_t __key) =
  sctk_gen_thread_getspecific;
int (*__sctk_ptr_thread_join) (sctk_thread_t __th,
			       void **__thread_return) = sctk_gen_thread_join;
int (*__sctk_ptr_thread_kill) (sctk_thread_t thread, int signo) =
  sctk_gen_thread_kill;
int (*__sctk_ptr_thread_sigsuspend) (sigset_t * set) =
  sctk_gen_thread_sigsuspend;
int (*__sctk_ptr_thread_sigpending) (sigset_t * set) =
  sctk_gen_thread_sigpending;
int (*__sctk_ptr_thread_sigmask) (int how, const sigset_t * newmask,
				  sigset_t * oldmask) =
  sctk_gen_thread_sigmask;
int (*__sctk_ptr_thread_key_create) (sctk_thread_key_t * __key,
				     void (*__destr_function) (void *)) =
  sctk_gen_thread_key_create;
int (*__sctk_ptr_thread_key_delete) (sctk_thread_key_t __key) =
  sctk_gen_thread_key_delete;
int (*__sctk_ptr_thread_mutexattr_destroy) (sctk_thread_mutexattr_t *
					    __attr) =
  sctk_gen_thread_mutexattr_destroy;

int (*__sctk_ptr_thread_mutexattr_getprioceiling) (const
						   sctk_thread_mutexattr_t
						   * __attr,
						   int *__prioceiling) =
  sctk_gen_thread_mutexattr_getprioceiling;
int (*__sctk_ptr_thread_mutexattr_setprioceiling) (sctk_thread_mutexattr_t
						   * __attr,
						   int __prioceiling) =
  sctk_gen_thread_mutexattr_setprioceiling;
int (*__sctk_ptr_thread_mutexattr_getprotocol) (const
						sctk_thread_mutexattr_t *
						__attr, int *__protocol) =
  sctk_gen_thread_mutexattr_getprotocol;
int (*__sctk_ptr_thread_mutexattr_setprotocol) (sctk_thread_mutexattr_t *
						__attr, int __protocol) =
  sctk_gen_thread_mutexattr_setprotocol;

int (*__sctk_ptr_thread_mutexattr_getpshared) (const
					       sctk_thread_mutexattr_t *
					       __attr, int *__pshared) =
  sctk_gen_thread_mutexattr_getpshared;
int (*__sctk_ptr_thread_mutexattr_gettype) (const sctk_thread_mutexattr_t *
					    __attr, int *__kind) =
  sctk_gen_thread_mutexattr_gettype;
int (*__sctk_ptr_thread_mutexattr_init) (sctk_thread_mutexattr_t *
					 __attr) =
  sctk_gen_thread_mutexattr_init;
int (*__sctk_ptr_thread_mutexattr_setpshared) (sctk_thread_mutexattr_t *
					       __attr, int __pshared) =
  sctk_gen_thread_mutexattr_setpshared;
int (*__sctk_ptr_thread_mutexattr_settype) (sctk_thread_mutexattr_t *
					    __attr, int __kind) =
  sctk_gen_thread_mutexattr_settype;
int (*__sctk_ptr_thread_mutex_destroy) (sctk_thread_mutex_t * __mutex) =
  sctk_gen_thread_mutex_destroy;
int (*__sctk_ptr_thread_mutex_init) (sctk_thread_mutex_t * __mutex,
				     const sctk_thread_mutexattr_t *
				     __mutex_attr) =
  sctk_gen_thread_mutex_init;
int (*__sctk_ptr_thread_mutex_lock) (sctk_thread_mutex_t * __mutex) =
  sctk_gen_thread_mutex_lock;
int (*__sctk_ptr_thread_mutex_spinlock) (sctk_thread_mutex_t * __mutex) =
  sctk_gen_thread_mutex_spinlock;
int (*__sctk_ptr_thread_mutex_timedlock) (sctk_thread_mutex_t * __mutex,
					  const struct timespec *
					  __abstime) =
  sctk_gen_thread_mutex_timedlock;
int (*__sctk_ptr_thread_mutex_trylock) (sctk_thread_mutex_t * __mutex) =
  sctk_gen_thread_mutex_trylock;
int (*__sctk_ptr_thread_mutex_unlock) (sctk_thread_mutex_t * __mutex) =
  sctk_gen_thread_mutex_unlock;

int (*__sctk_ptr_thread_sem_init) (sctk_thread_sem_t * sem, int pshared,
				   unsigned int value) =
  sctk_gen_thread_sem_init;
int (*__sctk_ptr_thread_sem_wait) (sctk_thread_sem_t * sem) =
  sctk_gen_thread_sem_wait;
int (*__sctk_ptr_thread_sem_trywait) (sctk_thread_sem_t * sem) =
  sctk_gen_thread_sem_trywait;
int (*__sctk_ptr_thread_sem_post) (sctk_thread_sem_t * sem) =
  sctk_gen_thread_sem_post;
int (*__sctk_ptr_thread_sem_getvalue) (sctk_thread_sem_t * sem,
				       int *sval) =
  sctk_gen_thread_sem_getvalue;
int (*__sctk_ptr_thread_sem_destroy) (sctk_thread_sem_t * sem) =
  sctk_gen_thread_sem_destroy;

sctk_thread_sem_t *(*__sctk_ptr_thread_sem_open) (const char *name,
						  int oflag, ...) =
  sctk_gen_thread_sem_open;
int (*__sctk_ptr_thread_sem_close) (sctk_thread_sem_t * sem) =
  sctk_gen_thread_sem_close;
int (*__sctk_ptr_thread_sem_unlink) (const char *name) =
  sctk_gen_thread_sem_unlink;

int (*__sctk_ptr_thread_once) (sctk_thread_once_t * __once_control,
			       void (*__init_routine) (void)) =
  sctk_gen_thread_once;

int (*__sctk_ptr_thread_rwlockattr_destroy) (sctk_thread_rwlockattr_t *
					     __attr) =
  sctk_gen_thread_rwlockattr_destroy;
int (*__sctk_ptr_thread_rwlockattr_getpshared) (const
						sctk_thread_rwlockattr_t *
						restrict __attr,
						int *restrict __pshared) =
  sctk_gen_thread_rwlockattr_getpshared;
int (*__sctk_ptr_thread_rwlockattr_init) (sctk_thread_rwlockattr_t *
					  __attr) =
  sctk_gen_thread_rwlockattr_init;
int (*__sctk_ptr_thread_rwlockattr_setpshared) (sctk_thread_rwlockattr_t *
						__attr, int __pshared) =
  sctk_gen_thread_rwlockattr_setpshared;
int (*__sctk_ptr_thread_rwlock_destroy) (sctk_thread_rwlock_t * __rwlock) =
  sctk_gen_thread_rwlock_destroy;
int (*__sctk_ptr_thread_rwlock_init) (sctk_thread_rwlock_t *
				      restrict __rwlock,
				      const sctk_thread_rwlockattr_t *
				      restrict __attr) =
  sctk_gen_thread_rwlock_init;
int (*__sctk_ptr_thread_rwlock_rdlock) (sctk_thread_rwlock_t * __rwlock) =
  sctk_gen_thread_rwlock_rdlock;

int (*__sctk_ptr_thread_rwlock_timedrdlock) (sctk_thread_rwlock_t *
					     __rwlock,
					     const struct timespec *
					     __abstime) =
  sctk_gen_thread_rwlock_timedrdlock;
int (*__sctk_ptr_thread_rwlock_timedwrlock) (sctk_thread_rwlock_t *
					     __rwlock,
					     const struct timespec *
					     __abstime) =
  sctk_gen_thread_rwlock_timedwrlock;

int (*__sctk_ptr_thread_rwlock_tryrdlock) (sctk_thread_rwlock_t *
					   __rwlock) =
  sctk_gen_thread_rwlock_tryrdlock;
int (*__sctk_ptr_thread_rwlock_trywrlock) (sctk_thread_rwlock_t *
					   __rwlock) =
  sctk_gen_thread_rwlock_trywrlock;
int (*__sctk_ptr_thread_rwlock_unlock) (sctk_thread_rwlock_t * __rwlock) =
  sctk_gen_thread_rwlock_unlock;
int (*__sctk_ptr_thread_rwlock_wrlock) (sctk_thread_rwlock_t * __rwlock) =
  sctk_gen_thread_rwlock_wrlock;

sctk_thread_t (*__sctk_ptr_thread_self) (void) = sctk_gen_thread_self;
sctk_thread_t (*__sctk_ptr_thread_self_check) (void) =
  sctk_gen_thread_self_check;
int (*__sctk_ptr_thread_setcancelstate) (int __state, int *__oldstate) =
  sctk_gen_thread_setcancelstate;
int (*__sctk_ptr_thread_setcanceltype) (int __type, int *__oldtype) =
  sctk_gen_thread_setcanceltype;
int (*__sctk_ptr_thread_setconcurrency) (int __level) =
  sctk_gen_thread_setconcurrency;
int (*__sctk_ptr_thread_setschedprio) (sctk_thread_t __p, int __i) =
  sctk_gen_thread_setschedprio;
int (*__sctk_ptr_thread_setschedparam) (sctk_thread_t __target_thread,
					int __policy,
					const struct sched_param *
					__param) =
  sctk_gen_thread_setschedparam;
int (*__sctk_ptr_thread_setspecific) (sctk_thread_key_t __key,
				      const void *__pointer) =
  sctk_gen_thread_setspecific;

int (*__sctk_ptr_thread_spin_destroy) (sctk_thread_spinlock_t * __lock) =
  sctk_gen_thread_spin_destroy;
int (*__sctk_ptr_thread_spin_init) (sctk_thread_spinlock_t * __lock,
				    int __pshared) =
  sctk_gen_thread_spin_init;
int (*__sctk_ptr_thread_spin_lock) (sctk_thread_spinlock_t * __lock) =
  sctk_gen_thread_spin_lock;
int (*__sctk_ptr_thread_spin_trylock) (sctk_thread_spinlock_t * __lock) =
  sctk_gen_thread_spin_trylock;
int (*__sctk_ptr_thread_spin_unlock) (sctk_thread_spinlock_t * __lock) =
  sctk_gen_thread_spin_unlock;

void (*__sctk_ptr_thread_testcancel) (void) = sctk_gen_thread_testcancel;
int (*__sctk_ptr_thread_yield) (void) = sctk_gen_thread_yield;
int (*__sctk_ptr_thread_dump) (char *file) = sctk_gen_thread_dump;
int (*__sctk_ptr_thread_migrate) (void) = sctk_gen_thread_migrate;
int (*__sctk_ptr_thread_restore) (sctk_thread_t thread, char *type,
				  int vp) = sctk_gen_thread_restore;
int (*__sctk_ptr_thread_dump_clean) (void) = sctk_gen_thread_dump_clean;

void (*__sctk_ptr_thread_wait_for_value) (int *data, int value) =
  sctk_gen_thread_wait_for_value;
void (*__sctk_ptr_thread_wait_for_value_and_poll) (int *data, int value,
						   void (*func) (void *),
						   void *arg) =
  sctk_gen_thread_wait_for_value_and_poll;

void (*__sctk_ptr_thread_freeze_thread_on_vp) (sctk_thread_mutex_t * lock,
					       void **list) =
  sctk_gen_thread_freeze_thread_on_vp;
void (*__sctk_ptr_thread_wake_thread_on_vp) (void **list) =
  sctk_gen_thread_wake_thread_on_vp;


int (*__sctk_ptr_thread_getattr_np) (sctk_thread_t th,
				     sctk_thread_attr_t * attr) =
  sctk_gen_thread_getattr_np;
int (*__sctk_ptr_thread_rwlockattr_getkind_np) (sctk_thread_rwlockattr_t *
						attr, int *ret) =
  sctk_gen_thread_rwlockattr_getkind_np;
int (*__sctk_ptr_thread_rwlockattr_setkind_np) (sctk_thread_rwlockattr_t *
						attr, int pref) =
  sctk_gen_thread_rwlockattr_setkind_np;
void (*__sctk_ptr_thread_kill_other_threads_np) (void) =
  sctk_gen_thread_kill_other_threads_np;

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

void
sctk_gen_thread_init ()
{
  sctk_print_version ("Init Gen", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);
  __sctk_ptr_thread_atfork = sctk_gen_thread_atfork;
  __sctk_ptr_thread_attr_destroy = sctk_gen_thread_attr_destroy;
  __sctk_ptr_thread_attr_getdetachstate = sctk_gen_thread_attr_getdetachstate;
  __sctk_ptr_thread_attr_getguardsize = sctk_gen_thread_attr_getguardsize;
  __sctk_ptr_thread_attr_getinheritsched =
    sctk_gen_thread_attr_getinheritsched;
  __sctk_ptr_thread_attr_getschedparam = sctk_gen_thread_attr_getschedparam;
  __sctk_ptr_thread_attr_getschedpolicy = sctk_gen_thread_attr_getschedpolicy;
  __sctk_ptr_thread_attr_getscope = sctk_gen_thread_attr_getscope;
  __sctk_ptr_thread_attr_getstackaddr = sctk_gen_thread_attr_getstackaddr;
  __sctk_ptr_thread_attr_getstack = sctk_gen_thread_attr_getstack;
  __sctk_ptr_thread_attr_getstacksize = sctk_gen_thread_attr_getstacksize;
  __sctk_ptr_thread_attr_init = sctk_gen_thread_attr_init;
  __sctk_ptr_thread_attr_setdetachstate = sctk_gen_thread_attr_setdetachstate;
  __sctk_ptr_thread_attr_setguardsize = sctk_gen_thread_attr_setguardsize;
  __sctk_ptr_thread_attr_setinheritsched =
    sctk_gen_thread_attr_setinheritsched;
  __sctk_ptr_thread_attr_setschedparam = sctk_gen_thread_attr_setschedparam;
  __sctk_ptr_thread_attr_setschedpolicy = sctk_gen_thread_attr_setschedpolicy;
  __sctk_ptr_thread_attr_setscope = sctk_gen_thread_attr_setscope;
  __sctk_ptr_thread_attr_setstackaddr = sctk_gen_thread_attr_setstackaddr;
  __sctk_ptr_thread_attr_setstack = sctk_gen_thread_attr_setstack;
  __sctk_ptr_thread_attr_setstacksize = sctk_gen_thread_attr_setstacksize;
  __sctk_ptr_thread_attr_setbinding = sctk_gen_thread_attr_setbinding;
  __sctk_ptr_thread_attr_getbinding = sctk_gen_thread_attr_getbinding;

  __sctk_ptr_thread_barrierattr_destroy = sctk_gen_thread_barrierattr_destroy;
  __sctk_ptr_thread_barrierattr_init = sctk_gen_thread_barrierattr_init;
  __sctk_ptr_thread_barrierattr_setpshared =
    sctk_gen_thread_barrierattr_setpshared;
  __sctk_ptr_thread_barrierattr_getpshared =
    sctk_gen_thread_barrierattr_getpshared;
  __sctk_ptr_thread_barrier_destroy = sctk_gen_thread_barrier_destroy;
  __sctk_ptr_thread_barrier_init = sctk_gen_thread_barrier_init;
  __sctk_ptr_thread_barrier_wait = sctk_gen_thread_barrier_wait;

  __sctk_ptr_thread_cancel = sctk_gen_thread_cancel;
  __sctk_ptr_thread_condattr_destroy = sctk_gen_thread_condattr_destroy;
  __sctk_ptr_thread_condattr_getpshared = sctk_gen_thread_condattr_getpshared;
  __sctk_ptr_thread_condattr_init = sctk_gen_thread_condattr_init;
  __sctk_ptr_thread_condattr_getclock = sctk_gen_thread_condattr_getclock;
  __sctk_ptr_thread_condattr_setclock = sctk_gen_thread_condattr_setclock;
  __sctk_ptr_thread_condattr_setpshared = sctk_gen_thread_condattr_setpshared;
  __sctk_ptr_thread_cond_broadcast = sctk_gen_thread_cond_broadcast;
  __sctk_ptr_thread_cond_destroy = sctk_gen_thread_cond_destroy;
  __sctk_ptr_thread_cond_init = sctk_gen_thread_cond_init;
  __sctk_ptr_thread_cond_signal = sctk_gen_thread_cond_signal;
  __sctk_ptr_thread_cond_timedwait = sctk_gen_thread_cond_timedwait;
  __sctk_ptr_thread_cond_wait = sctk_gen_thread_cond_wait;
  __sctk_ptr_thread_create = sctk_gen_thread_create;
  __sctk_ptr_thread_detach = sctk_gen_thread_detach;
  __sctk_ptr_thread_equal = sctk_gen_thread_equal;
  __sctk_ptr_thread_exit = sctk_gen_thread_exit;
  __sctk_ptr_thread_getconcurrency = sctk_gen_thread_getconcurrency;
  __sctk_ptr_thread_getcpuclockid = sctk_gen_thread_getcpuclockid;
  __sctk_ptr_thread_getschedparam = sctk_gen_thread_getschedparam;
  __sctk_ptr_thread_sched_get_priority_max =
    sctk_gen_thread_sched_get_priority_max;
  __sctk_ptr_thread_sched_get_priority_min =
    sctk_gen_thread_sched_get_priority_min;


  __sctk_ptr_thread_getspecific = sctk_gen_thread_getspecific;
  __sctk_ptr_thread_join = sctk_gen_thread_join;
  __sctk_ptr_thread_key_create = sctk_gen_thread_key_create;
  __sctk_ptr_thread_key_delete = sctk_gen_thread_key_delete;
  __sctk_ptr_thread_mutexattr_destroy = sctk_gen_thread_mutexattr_destroy;
  __sctk_ptr_thread_mutexattr_getpshared =
    sctk_gen_thread_mutexattr_getpshared;

  __sctk_ptr_thread_mutexattr_getprioceiling =
    sctk_gen_thread_mutexattr_getprioceiling;
  __sctk_ptr_thread_mutexattr_setprioceiling =
    sctk_gen_thread_mutexattr_setprioceiling;
  __sctk_ptr_thread_mutexattr_getprotocol =
    sctk_gen_thread_mutexattr_getprotocol;
  __sctk_ptr_thread_mutexattr_setprotocol =
    sctk_gen_thread_mutexattr_setprotocol;

  __sctk_ptr_thread_mutexattr_gettype = sctk_gen_thread_mutexattr_gettype;
  __sctk_ptr_thread_mutexattr_init = sctk_gen_thread_mutexattr_init;
  __sctk_ptr_thread_mutexattr_setpshared =
    sctk_gen_thread_mutexattr_setpshared;
  __sctk_ptr_thread_mutexattr_settype = sctk_gen_thread_mutexattr_settype;
  __sctk_ptr_thread_mutex_destroy = sctk_gen_thread_mutex_destroy;
  __sctk_ptr_thread_mutex_init = sctk_gen_thread_mutex_init;
  __sctk_ptr_thread_mutex_lock = sctk_gen_thread_mutex_lock;
  __sctk_ptr_thread_mutex_spinlock = sctk_gen_thread_mutex_spinlock;
  __sctk_ptr_thread_mutex_timedlock = sctk_gen_thread_mutex_timedlock;
  __sctk_ptr_thread_mutex_trylock = sctk_gen_thread_mutex_trylock;
  __sctk_ptr_thread_mutex_unlock = sctk_gen_thread_mutex_unlock;
  __sctk_ptr_thread_once = sctk_gen_thread_once;

  __sctk_ptr_thread_rwlockattr_destroy = sctk_gen_thread_rwlockattr_destroy;
  __sctk_ptr_thread_rwlockattr_getpshared =
    sctk_gen_thread_rwlockattr_getpshared;
  __sctk_ptr_thread_rwlockattr_init = sctk_gen_thread_rwlockattr_init;
  __sctk_ptr_thread_rwlockattr_setpshared =
    sctk_gen_thread_rwlockattr_setpshared;
  __sctk_ptr_thread_rwlock_destroy = sctk_gen_thread_rwlock_destroy;
  __sctk_ptr_thread_rwlock_init = sctk_gen_thread_rwlock_init;
  __sctk_ptr_thread_rwlock_rdlock = sctk_gen_thread_rwlock_rdlock;

  __sctk_ptr_thread_rwlock_timedrdlock = sctk_gen_thread_rwlock_timedrdlock;
  __sctk_ptr_thread_rwlock_timedwrlock = sctk_gen_thread_rwlock_timedwrlock;

  __sctk_ptr_thread_rwlock_tryrdlock = sctk_gen_thread_rwlock_tryrdlock;
  __sctk_ptr_thread_rwlock_trywrlock = sctk_gen_thread_rwlock_trywrlock;
  __sctk_ptr_thread_rwlock_unlock = sctk_gen_thread_rwlock_unlock;
  __sctk_ptr_thread_rwlock_wrlock = sctk_gen_thread_rwlock_wrlock;

  __sctk_ptr_thread_self = sctk_gen_thread_self;
  __sctk_ptr_thread_self_check = sctk_gen_thread_self_check;
  __sctk_ptr_thread_setcancelstate = sctk_gen_thread_setcancelstate;
  __sctk_ptr_thread_setcanceltype = sctk_gen_thread_setcanceltype;
  __sctk_ptr_thread_setconcurrency = sctk_gen_thread_setconcurrency;
  __sctk_ptr_thread_setschedprio = sctk_gen_thread_setschedprio;
  __sctk_ptr_thread_setschedparam = sctk_gen_thread_setschedparam;
  __sctk_ptr_thread_setspecific = sctk_gen_thread_setspecific;

  __sctk_ptr_thread_spin_destroy = sctk_gen_thread_spin_destroy;
  __sctk_ptr_thread_spin_init = sctk_gen_thread_spin_init;
  __sctk_ptr_thread_spin_lock = sctk_gen_thread_spin_lock;
  __sctk_ptr_thread_spin_trylock = sctk_gen_thread_spin_trylock;
  __sctk_ptr_thread_spin_unlock = sctk_gen_thread_spin_unlock;

  __sctk_ptr_thread_testcancel = sctk_gen_thread_testcancel;
  __sctk_ptr_thread_yield = sctk_gen_thread_yield;
  __sctk_ptr_thread_dump = sctk_gen_thread_dump;
  __sctk_ptr_thread_dump_clean = sctk_gen_thread_dump_clean;
  __sctk_ptr_thread_migrate = sctk_gen_thread_migrate;
  __sctk_ptr_thread_restore = sctk_gen_thread_restore;

  __sctk_ptr_thread_wait_for_value = sctk_gen_thread_wait_for_value;
  __sctk_ptr_thread_wait_for_value_and_poll =
    sctk_gen_thread_wait_for_value_and_poll;
  __sctk_ptr_thread_freeze_thread_on_vp = sctk_gen_thread_freeze_thread_on_vp;
  __sctk_ptr_thread_wake_thread_on_vp = sctk_gen_thread_wake_thread_on_vp;


  __sctk_ptr_thread_getattr_np = sctk_gen_thread_getattr_np;
  __sctk_ptr_thread_rwlockattr_getkind_np =
    sctk_gen_thread_rwlockattr_getkind_np;
  __sctk_ptr_thread_rwlockattr_setkind_np =
    sctk_gen_thread_rwlockattr_setkind_np;
  __sctk_ptr_thread_kill_other_threads_np =
    sctk_gen_thread_kill_other_threads_np;



}
