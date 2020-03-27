#define _GNU_SOURCE
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
#include "mpc_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_futex.h"

#include <fcntl.h>

static inline void
sctk_touch_ptr(const void *ptr)
{
	assume(ptr == NULL);
}

static inline void
sctk_touch_func(void (*ptr)(void) )
{
	assume(ptr == NULL);
}

static inline void
sctk_touch_func_s(void *(*ptr)(void *) )
{
	assume(ptr == NULL);
}

static inline void
sctk_touch_func_n(void (*ptr)(void *) )
{
	assume(ptr == NULL);
}

static inline void
sctk_touch_thread(const mpc_thread_t ptr)
{
	assume(ptr == NULL);
}

static inline void
sctk_touch_int(const int ptr)
{
	assume(ptr == 0);
}

static inline void
sctk_touch_long(const long ptr)
{
	assume(ptr == 0);
}

static double
sctk_gen_thread_get_activity(__UNUSED__ int i)
{
	sctk_nodebug("sctk_gen_thread_get_activity");
	return -1;
}

void  mpc_launch_init_runtime();

static inline void __base_mpc_init()
{
	static int init_done = 0;

	if(!init_done)
	{
		init_done = 1;
		mpc_launch_init_runtime();
	}
}

static int
sctk_gen_thread_atfork(void (*__prepare)(void), void (*__parent)(void),
                       void (*__child)(void) )
{
	__base_mpc_init();
	assume(sctk_gen_thread_atfork != __sctk_ptr_thread_atfork);
	return __sctk_ptr_thread_atfork(__prepare, __parent, __child);
}

static int
sctk_gen_thread_attr_destroy(mpc_thread_attr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_destroy != __sctk_ptr_thread_attr_destroy);
	return __sctk_ptr_thread_attr_destroy(__attr);
}

static int
sctk_gen_thread_attr_getdetachstate(const mpc_thread_attr_t *__attr,
                                    int *__detachstate)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getdetachstate != __sctk_ptr_thread_attr_getdetachstate);
	return __sctk_ptr_thread_attr_getdetachstate(__attr, __detachstate);
}

static int
sctk_gen_thread_attr_getguardsize(const mpc_thread_attr_t *
                                  restrict __attr,
                                  size_t *restrict __guardsize)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getguardsize != __sctk_ptr_thread_attr_getguardsize);
	return __sctk_ptr_thread_attr_getguardsize(__attr, __guardsize);
}

static int
sctk_gen_thread_attr_getinheritsched(const mpc_thread_attr_t *
                                     restrict __attr,
                                     int *restrict __inherit)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getinheritsched != __sctk_ptr_thread_attr_getinheritsched);
	return __sctk_ptr_thread_attr_getinheritsched(__attr, __inherit);
}

static int
sctk_gen_thread_attr_getschedparam(const mpc_thread_attr_t *
                                   restrict __attr,
                                   struct sched_param *restrict __param)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getschedparam != __sctk_ptr_thread_attr_getschedparam);
	return __sctk_ptr_thread_attr_getschedparam(__attr, __param);
}

static int
sctk_gen_thread_attr_getschedpolicy(const mpc_thread_attr_t *
                                    restrict __attr, int *restrict __policy)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getschedpolicy != __sctk_ptr_thread_attr_getschedpolicy);
	return __sctk_ptr_thread_attr_getschedpolicy(__attr, __policy);
}

static int
sctk_gen_thread_attr_getscope(const mpc_thread_attr_t *restrict __attr,
                              int *restrict __scope)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getscope != __sctk_ptr_thread_attr_getscope);
	return __sctk_ptr_thread_attr_getscope(__attr, __scope);
}

static int
sctk_gen_thread_attr_getstackaddr(const mpc_thread_attr_t *
                                  restrict __attr,
                                  void **restrict __stackaddr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getstackaddr != __sctk_ptr_thread_attr_getstackaddr);
	return __sctk_ptr_thread_attr_getstackaddr(__attr, __stackaddr);
}

static int
sctk_gen_thread_attr_getstack(const mpc_thread_attr_t *restrict __attr,
                              void **restrict __stackaddr,
                              size_t *restrict __stacksize)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getstack != __sctk_ptr_thread_attr_getstack);
	return __sctk_ptr_thread_attr_getstack(__attr, __stackaddr, __stacksize);
}

static int
sctk_gen_thread_attr_getstacksize(const mpc_thread_attr_t *
                                  restrict __attr,
                                  size_t *restrict __stacksize)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getstacksize != __sctk_ptr_thread_attr_getstacksize);
	return __sctk_ptr_thread_attr_getstacksize(__attr, __stacksize);
}

static int
sctk_gen_thread_attr_setbinding(mpc_thread_attr_t *__attr, int __binding)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setbinding != __sctk_ptr_thread_attr_setbinding);
	return __sctk_ptr_thread_attr_setbinding(__attr, __binding);
}

static int
sctk_gen_thread_attr_getbinding(mpc_thread_attr_t *__attr, int *__binding)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_getbinding != __sctk_ptr_thread_attr_getbinding);
	return __sctk_ptr_thread_attr_getbinding(__attr, __binding);
}

static int
sctk_gen_thread_attr_init(mpc_thread_attr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_init != __sctk_ptr_thread_attr_init);
	return __sctk_ptr_thread_attr_init(__attr);
}

static int
sctk_gen_thread_attr_setdetachstate(mpc_thread_attr_t *__attr,
                                    int __detachstate)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setdetachstate != __sctk_ptr_thread_attr_setdetachstate);
	return __sctk_ptr_thread_attr_setdetachstate(__attr, __detachstate);
}

static int
sctk_gen_thread_attr_setguardsize(mpc_thread_attr_t *__attr,
                                  size_t __guardsize)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setguardsize != __sctk_ptr_thread_attr_setguardsize);
	return __sctk_ptr_thread_attr_setguardsize(__attr, __guardsize);
}

static int
sctk_gen_thread_attr_setinheritsched(mpc_thread_attr_t *__attr,
                                     int __inherit)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setinheritsched != __sctk_ptr_thread_attr_setinheritsched);
	return __sctk_ptr_thread_attr_setinheritsched(__attr, __inherit);
}

static int
sctk_gen_thread_attr_setschedparam(mpc_thread_attr_t *restrict __attr,
                                   const struct sched_param *restrict
                                   __param)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setschedparam != __sctk_ptr_thread_attr_setschedparam);
	return __sctk_ptr_thread_attr_setschedparam(__attr, __param);
}

static int
sctk_gen_thread_sched_get_priority_max(int policy)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sched_get_priority_max != __sctk_ptr_thread_sched_get_priority_max);
	return __sctk_ptr_thread_sched_get_priority_max(policy);
}

static int
sctk_gen_thread_sched_get_priority_min(int policy)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sched_get_priority_min != __sctk_ptr_thread_sched_get_priority_min);
	return __sctk_ptr_thread_sched_get_priority_min(policy);
}

static int
sctk_gen_thread_attr_setschedpolicy(mpc_thread_attr_t *__attr,
                                    int __policy)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setschedpolicy != __sctk_ptr_thread_attr_setschedpolicy);
	return __sctk_ptr_thread_attr_setschedpolicy(__attr, __policy);
}

static int
sctk_gen_thread_attr_setscope(mpc_thread_attr_t *__attr, int __scope)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setscope != __sctk_ptr_thread_attr_setscope);
	return __sctk_ptr_thread_attr_setscope(__attr, __scope);
}

static int
sctk_gen_thread_attr_setstackaddr(mpc_thread_attr_t *__attr,
                                  void *__stackaddr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setstackaddr != __sctk_ptr_thread_attr_setstackaddr);
	return __sctk_ptr_thread_attr_setstackaddr(__attr, __stackaddr);
}

static int
sctk_gen_thread_attr_setstack(mpc_thread_attr_t *__attr,
                              void *__stackaddr, size_t __stacksize)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setstack != __sctk_ptr_thread_attr_setstack);
	return __sctk_ptr_thread_attr_setstack(__attr, __stackaddr, __stacksize);
}

static int
sctk_gen_thread_attr_setstacksize(mpc_thread_attr_t *__attr,
                                  size_t __stacksize)
{
	__base_mpc_init();
	assume(sctk_gen_thread_attr_setstacksize != __sctk_ptr_thread_attr_setstacksize);
	return __sctk_ptr_thread_attr_setstacksize(__attr, __stacksize);
}

static int
sctk_gen_thread_barrierattr_destroy(mpc_thread_barrierattr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_barrierattr_destroy != __sctk_ptr_thread_barrierattr_destroy);
	return __sctk_ptr_thread_barrierattr_destroy(__attr);
}

static int
sctk_gen_thread_barrierattr_init(mpc_thread_barrierattr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_barrierattr_init != __sctk_ptr_thread_barrierattr_init);
	return __sctk_ptr_thread_barrierattr_init(__attr);
}

static int
sctk_gen_thread_barrierattr_setpshared(mpc_thread_barrierattr_t *__attr,
                                       int __pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_barrierattr_setpshared != __sctk_ptr_thread_barrierattr_setpshared);
	return __sctk_ptr_thread_barrierattr_setpshared(__attr, __pshared);
}

static int
sctk_gen_thread_barrierattr_getpshared(const mpc_thread_barrierattr_t *
                                       __attr, int *__pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_barrierattr_getpshared != __sctk_ptr_thread_barrierattr_getpshared);
	return __sctk_ptr_thread_barrierattr_getpshared(__attr, __pshared);
}

static int
sctk_gen_thread_barrier_destroy(mpc_thread_barrier_t *__barrier)
{
	__base_mpc_init();
	assume(sctk_gen_thread_barrier_destroy != __sctk_ptr_thread_barrier_destroy);
	return __sctk_ptr_thread_barrier_destroy(__barrier);
}

static int
sctk_gen_thread_barrier_init(mpc_thread_barrier_t *restrict __barrier,
                             const mpc_thread_barrierattr_t *
                             restrict __attr, unsigned int __count)
{
	__base_mpc_init();
	assume(sctk_gen_thread_barrier_init != __sctk_ptr_thread_barrier_init);
	return __sctk_ptr_thread_barrier_init(__barrier, __attr, __count);
}

static int
sctk_gen_thread_barrier_wait(mpc_thread_barrier_t *__barrier)
{
	__base_mpc_init();
	assume(sctk_gen_thread_barrier_wait != __sctk_ptr_thread_barrier_wait);
	return __sctk_ptr_thread_barrier_wait(__barrier);
}

static int
sctk_gen_thread_cancel(mpc_thread_t __cancelthread)
{
	__base_mpc_init();
	assume(sctk_gen_thread_cancel != __sctk_ptr_thread_cancel);
	return __sctk_ptr_thread_cancel(__cancelthread);
}

static int
sctk_gen_thread_condattr_destroy(mpc_thread_condattr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_condattr_destroy != __sctk_ptr_thread_condattr_destroy);
	return __sctk_ptr_thread_condattr_destroy(__attr);
}

static int
sctk_gen_thread_condattr_getpshared(const mpc_thread_condattr_t *
                                    restrict __attr, int *restrict __pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_condattr_getpshared != __sctk_ptr_thread_condattr_getpshared);
	return __sctk_ptr_thread_condattr_getpshared(__attr, __pshared);
}

static int
sctk_gen_thread_condattr_init(mpc_thread_condattr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_condattr_init != __sctk_ptr_thread_condattr_init);
	return __sctk_ptr_thread_condattr_init(__attr);
}

static int
sctk_gen_thread_condattr_setpshared(mpc_thread_condattr_t *__attr,
                                    int __pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_condattr_setpshared != __sctk_ptr_thread_condattr_setpshared);
	return __sctk_ptr_thread_condattr_setpshared(__attr, __pshared);
}

static int
sctk_gen_thread_cond_broadcast(mpc_thread_cond_t *__cond)
{
	__base_mpc_init();
	assume(sctk_gen_thread_cond_broadcast != __sctk_ptr_thread_cond_broadcast);
	return __sctk_ptr_thread_cond_broadcast(__cond);
}

static int
sctk_gen_thread_cond_destroy(mpc_thread_cond_t *__cond)
{
	__base_mpc_init();
	assume(sctk_gen_thread_cond_destroy != __sctk_ptr_thread_cond_destroy);
	return __sctk_ptr_thread_cond_destroy(__cond);
}

static int
sctk_gen_thread_condattr_setclock(mpc_thread_condattr_t *
                                  attr, clockid_t clock_id)
{
	__base_mpc_init();
	assume(sctk_gen_thread_condattr_setclock != __sctk_ptr_thread_condattr_setclock);
	return __sctk_ptr_thread_condattr_setclock(attr, clock_id);
}

static int
sctk_gen_thread_condattr_getclock(mpc_thread_condattr_t *
                                  attr, clockid_t *clock_id)
{
	__base_mpc_init();
	assume(sctk_gen_thread_condattr_getclock != __sctk_ptr_thread_condattr_getclock);
	return __sctk_ptr_thread_condattr_getclock(attr, clock_id);
}

static int
sctk_gen_thread_cond_init(mpc_thread_cond_t *restrict __cond,
                          const mpc_thread_condattr_t *
                          restrict __cond_attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_cond_init != __sctk_ptr_thread_cond_init);
	return __sctk_ptr_thread_cond_init(__cond, __cond_attr);
}

static int
sctk_gen_thread_cond_signal(mpc_thread_cond_t *__cond)
{
	__base_mpc_init();
	assume(sctk_gen_thread_cond_signal != __sctk_ptr_thread_cond_signal);
	return __sctk_ptr_thread_cond_signal(__cond);
}

static int
sctk_gen_thread_cond_timedwait(mpc_thread_cond_t *restrict __cond,
                               mpc_thread_mutex_t *restrict __mutex,
                               const struct timespec *restrict __abstime)
{
	__base_mpc_init();
	assume(sctk_gen_thread_cond_timedwait != __sctk_ptr_thread_cond_timedwait);
	return __sctk_ptr_thread_cond_timedwait(__cond, __mutex, __abstime);
}

static int
sctk_gen_thread_cond_wait(mpc_thread_cond_t *restrict __cond,
                          mpc_thread_mutex_t *restrict __mutex)
{
	__base_mpc_init();
	assume(sctk_gen_thread_cond_wait != __sctk_ptr_thread_cond_wait);
	return __sctk_ptr_thread_cond_wait(__cond, __mutex);
}

static int
sctk_gen_thread_create(mpc_thread_t *restrict __threadp,
                       const mpc_thread_attr_t *restrict __attr,
                       void *(*__start_routine)(void *),
                       void *restrict __arg)
{
	__base_mpc_init();
	assume(sctk_gen_thread_create != __sctk_ptr_thread_create);
	return __sctk_ptr_thread_create(__threadp, __attr, __start_routine, __arg);
}

static int
sctk_gen_thread_user_create(mpc_thread_t *restrict __threadp,
                            const mpc_thread_attr_t *restrict __attr,
                            void *(*__start_routine)(void *),
                            void *restrict __arg)
{
	__base_mpc_init();
	assume(sctk_gen_thread_user_create != __sctk_ptr_thread_user_create);
	return __sctk_ptr_thread_user_create(__threadp, __attr, __start_routine, __arg);
}

static int
sctk_gen_thread_detach(mpc_thread_t __th)
{
	__base_mpc_init();
	assume(sctk_gen_thread_detach != __sctk_ptr_thread_detach);
	return __sctk_ptr_thread_detach(__th);
}

static int
sctk_gen_thread_equal(mpc_thread_t __thread1, mpc_thread_t __thread2)
{
	__base_mpc_init();
	assume(sctk_gen_thread_equal != __sctk_ptr_thread_equal);
	return __sctk_ptr_thread_equal(__thread1, __thread2);
}

static void
sctk_gen_thread_exit(void *__retval)
{
	__base_mpc_init();
	assume(sctk_gen_thread_exit != __sctk_ptr_thread_exit);
	__sctk_ptr_thread_exit(__retval);
}

static int
sctk_gen_thread_getconcurrency(void)
{
	__base_mpc_init();
	assume(sctk_gen_thread_getconcurrency != __sctk_ptr_thread_getconcurrency);
	return __sctk_ptr_thread_getconcurrency();
}

static int
sctk_gen_thread_getcpuclockid(mpc_thread_t __thread_id,
                              clockid_t *__clock_id)
{
	__base_mpc_init();
	assume(sctk_gen_thread_getcpuclockid != __sctk_ptr_thread_getcpuclockid);
	return __sctk_ptr_thread_getcpuclockid(__thread_id, __clock_id);
}

static int
sctk_gen_thread_getschedparam(mpc_thread_t __target_thread,
                              int *restrict __policy,
                              struct sched_param *restrict __param)
{
	__base_mpc_init();
	assume(sctk_gen_thread_getschedparam != __sctk_ptr_thread_getschedparam);
	return __sctk_ptr_thread_getschedparam(__target_thread, __policy, __param);
}

static void *
sctk_gen_thread_getspecific(mpc_thread_keys_t __key)
{
	__base_mpc_init();
	assume(sctk_gen_thread_getspecific != __sctk_ptr_thread_getspecific);
	return __sctk_ptr_thread_getspecific(__key);
}

static int
sctk_gen_thread_join(mpc_thread_t __th, void **__thread_return)
{
	__base_mpc_init();
	assume(sctk_gen_thread_join != __sctk_ptr_thread_join);
	return __sctk_ptr_thread_join(__th, __thread_return);
}

static int
sctk_gen_thread_kill(mpc_thread_t thread, int signo)
{
	__base_mpc_init();
	assume(sctk_gen_thread_kill != __sctk_ptr_thread_kill);
	return __sctk_ptr_thread_kill(thread, signo);
}

static int
sctk_gen_thread_sigsuspend(sigset_t *set)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sigsuspend != __sctk_ptr_thread_sigsuspend);
	return __sctk_ptr_thread_sigsuspend(set);
}

static int
sctk_gen_thread_sigpending(sigset_t *set)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sigpending != __sctk_ptr_thread_sigpending);
	return __sctk_ptr_thread_sigpending(set);
}

static int
sctk_gen_thread_sigmask(int how, const sigset_t *newmask,
                        sigset_t *oldmask)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sigmask != __sctk_ptr_thread_sigmask);
	return __sctk_ptr_thread_sigmask(how, newmask, oldmask);
}

static int
sctk_gen_thread_key_create(mpc_thread_keys_t *__key,
                           void (*__destr_function)(void *) )
{
	__base_mpc_init();
	assume(sctk_gen_thread_key_create != __sctk_ptr_thread_key_create);
	return __sctk_ptr_thread_key_create(__key, __destr_function);
}

static int
sctk_gen_thread_key_delete(mpc_thread_keys_t __key)
{
	__base_mpc_init();
	assume(sctk_gen_thread_key_delete != __sctk_ptr_thread_key_delete);
	return __sctk_ptr_thread_key_delete(__key);
}

static int
sctk_gen_thread_mutexattr_destroy(mpc_thread_mutexattr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_destroy != __sctk_ptr_thread_mutexattr_destroy);
	return __sctk_ptr_thread_mutexattr_destroy(__attr);
}

static int
sctk_gen_thread_mutexattr_getpshared(const mpc_thread_mutexattr_t *
                                     restrict __attr,
                                     int *restrict __pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_getpshared != __sctk_ptr_thread_mutexattr_getpshared);
	return __sctk_ptr_thread_mutexattr_getpshared(__attr, __pshared);
}

static int
sctk_gen_thread_mutexattr_getprioceiling(const mpc_thread_mutexattr_t *
                                         __attr, int *__prioceiling)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_getprioceiling != __sctk_ptr_thread_mutexattr_getprioceiling);
	return __sctk_ptr_thread_mutexattr_getprioceiling(__attr, __prioceiling);
}

static int
sctk_gen_thread_mutexattr_setprioceiling(mpc_thread_mutexattr_t *
                                         __attr, int __prioceiling)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_setprioceiling != __sctk_ptr_thread_mutexattr_setprioceiling);
	return __sctk_ptr_thread_mutexattr_setprioceiling(__attr, __prioceiling);
}

static int
sctk_gen_thread_mutexattr_getprotocol(const mpc_thread_mutexattr_t *
                                      __attr, int *__protocol)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_getprotocol != __sctk_ptr_thread_mutexattr_getprotocol);
	return __sctk_ptr_thread_mutexattr_getprotocol(__attr, __protocol);
}

static int
sctk_gen_thread_mutexattr_setprotocol(mpc_thread_mutexattr_t *
                                      __attr, int __protocol)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_setprotocol != __sctk_ptr_thread_mutexattr_setprotocol);
	return __sctk_ptr_thread_mutexattr_setprotocol(__attr, __protocol);
}

static int
sctk_gen_thread_mutexattr_gettype(const mpc_thread_mutexattr_t *
                                  restrict __attr, int *restrict __kind)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_gettype != __sctk_ptr_thread_mutexattr_gettype);
	return __sctk_ptr_thread_mutexattr_gettype(__attr, __kind);
}

static int
sctk_gen_thread_mutexattr_init(mpc_thread_mutexattr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_init != __sctk_ptr_thread_mutexattr_init);
	return __sctk_ptr_thread_mutexattr_init(__attr);
}

static int
sctk_gen_thread_mutexattr_setpshared(mpc_thread_mutexattr_t *__attr,
                                     int __pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_setpshared != __sctk_ptr_thread_mutexattr_setpshared);
	return __sctk_ptr_thread_mutexattr_setpshared(__attr, __pshared);
}

static int
sctk_gen_thread_mutexattr_settype(mpc_thread_mutexattr_t *__attr,
                                  int __kind)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutexattr_settype != __sctk_ptr_thread_mutexattr_settype);
	return __sctk_ptr_thread_mutexattr_settype(__attr, __kind);
}

static int
sctk_gen_thread_mutex_destroy(mpc_thread_mutex_t *__mutex)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutex_destroy != __sctk_ptr_thread_mutex_destroy);
	return __sctk_ptr_thread_mutex_destroy(__mutex);
}

static int
sctk_gen_thread_mutex_init(mpc_thread_mutex_t *restrict __mutex,
                           const mpc_thread_mutexattr_t *
                           restrict __mutex_attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutex_init != __sctk_ptr_thread_mutex_init);
	return __sctk_ptr_thread_mutex_init(__mutex, __mutex_attr);
}

static int
sctk_gen_thread_mutex_lock(mpc_thread_mutex_t *__mutex)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutex_lock != __sctk_ptr_thread_mutex_lock);
	return __sctk_ptr_thread_mutex_lock(__mutex);
}

static int
sctk_gen_thread_mutex_spinlock(mpc_thread_mutex_t *__mutex)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutex_spinlock != __sctk_ptr_thread_mutex_spinlock);
	return __sctk_ptr_thread_mutex_spinlock(__mutex);
}

static int
sctk_gen_thread_mutex_timedlock(mpc_thread_mutex_t *restrict __mutex,
                                const struct timespec *restrict __abstime)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutex_timedlock != __sctk_ptr_thread_mutex_timedlock);
	return __sctk_ptr_thread_mutex_timedlock(__mutex, __abstime);
}

static int
sctk_gen_thread_mutex_trylock(mpc_thread_mutex_t *__mutex)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutex_trylock != __sctk_ptr_thread_mutex_trylock);
	return __sctk_ptr_thread_mutex_trylock(__mutex);
}

static int
sctk_gen_thread_mutex_unlock(mpc_thread_mutex_t *__mutex)
{
	__base_mpc_init();
	assume(sctk_gen_thread_mutex_unlock != __sctk_ptr_thread_mutex_unlock);
	return __sctk_ptr_thread_mutex_unlock(__mutex);
}

static int
sctk_gen_thread_sem_init(mpc_thread_sem_t *sem, int pshared,
                         unsigned int value)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_init != __sctk_ptr_thread_sem_init);
	return __sctk_ptr_thread_sem_init(sem, pshared, value);
}

static int
sctk_gen_thread_sem_wait(mpc_thread_sem_t *sem)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_wait != __sctk_ptr_thread_sem_wait);
	return __sctk_ptr_thread_sem_wait(sem);
}

static int
sctk_gen_thread_sem_trywait(mpc_thread_sem_t *sem)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_trywait != __sctk_ptr_thread_sem_trywait);
	return __sctk_ptr_thread_sem_trywait(sem);
}

static int
sctk_gen_thread_sem_timedwait(mpc_thread_sem_t *sem,
                              const struct timespec *__abstime)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_timedwait != __sctk_ptr_thread_sem_timedwait);
	return __sctk_ptr_thread_sem_timedwait(sem, __abstime);
}

static int
sctk_gen_thread_sem_post(mpc_thread_sem_t *sem)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_post != __sctk_ptr_thread_sem_post);
	return __sctk_ptr_thread_sem_post(sem);
}

static int
sctk_gen_thread_sem_getvalue(mpc_thread_sem_t *sem, int *sval)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_getvalue != __sctk_ptr_thread_sem_getvalue);
	return __sctk_ptr_thread_sem_getvalue(sem, sval);
}

static int
sctk_gen_thread_sem_destroy(mpc_thread_sem_t *sem)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_destroy != __sctk_ptr_thread_sem_destroy);
	return __sctk_ptr_thread_sem_destroy(sem);
}

static mpc_thread_sem_t *
sctk_gen_thread_sem_open(const char *name, int oflag, ...)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_open != __sctk_ptr_thread_sem_open);
	if( (oflag & O_CREAT) )
	{
		va_list ap;
		int     mode, value;
		va_start(ap, oflag);
		mode  = (int)va_arg(ap, mode_t);
		value = va_arg(ap, int);
		va_end(ap);
		return __sctk_ptr_thread_sem_open(name, oflag, mode, value);
	}
	else
	{
		return __sctk_ptr_thread_sem_open(name, oflag);
	}
}

static int
sctk_gen_thread_sem_close(mpc_thread_sem_t *sem)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_close != __sctk_ptr_thread_sem_close);
	return __sctk_ptr_thread_sem_close(sem);
}

static int
sctk_gen_thread_sem_unlink(const char *name)
{
	__base_mpc_init();
	assume(sctk_gen_thread_sem_unlink != __sctk_ptr_thread_sem_unlink);
	return __sctk_ptr_thread_sem_unlink(name);
}

static int
sctk_gen_thread_once(mpc_thread_once_t *__once_control,
                     void (*__init_routine)(void) )
{
	__base_mpc_init();
	assume(sctk_gen_thread_once != __sctk_ptr_thread_once);
	return __sctk_ptr_thread_once(__once_control, __init_routine);
}

static int
sctk_gen_thread_rwlockattr_destroy(mpc_thread_rwlockattr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlockattr_destroy != __sctk_ptr_thread_rwlockattr_destroy);
	return __sctk_ptr_thread_rwlockattr_destroy(__attr);
}

static int
sctk_gen_thread_rwlockattr_getpshared(const mpc_thread_rwlockattr_t *
                                      restrict __attr,
                                      int *restrict __pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlockattr_getpshared != __sctk_ptr_thread_rwlockattr_getpshared);
	return __sctk_ptr_thread_rwlockattr_getpshared(__attr, __pshared);
}

static int
sctk_gen_thread_rwlockattr_init(mpc_thread_rwlockattr_t *__attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlockattr_init != __sctk_ptr_thread_rwlockattr_init);
	return __sctk_ptr_thread_rwlockattr_init(__attr);
}

static int
sctk_gen_thread_rwlockattr_setpshared(mpc_thread_rwlockattr_t *__attr,
                                      int __pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlockattr_setpshared != __sctk_ptr_thread_rwlockattr_setpshared);
	return __sctk_ptr_thread_rwlockattr_setpshared(__attr, __pshared);
}

static int
sctk_gen_thread_rwlock_destroy(mpc_thread_rwlock_t *__rwlock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_destroy != __sctk_ptr_thread_rwlock_destroy);
	return __sctk_ptr_thread_rwlock_destroy(__rwlock);
}

static int
sctk_gen_thread_rwlock_init(mpc_thread_rwlock_t *restrict __rwlock,
                            const mpc_thread_rwlockattr_t *restrict __attr)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_init != __sctk_ptr_thread_rwlock_init);
	return __sctk_ptr_thread_rwlock_init(__rwlock, __attr);
}

static int
sctk_gen_thread_rwlock_rdlock(mpc_thread_rwlock_t *__rwlock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_rdlock != __sctk_ptr_thread_rwlock_rdlock);
	return __sctk_ptr_thread_rwlock_rdlock(__rwlock);
}

static int
sctk_gen_thread_rwlock_timedrdlock(mpc_thread_rwlock_t *
                                   restrict __rwlock,
                                   const struct timespec *restrict __abstime)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_timedrdlock != __sctk_ptr_thread_rwlock_timedrdlock);
	return __sctk_ptr_thread_rwlock_timedrdlock(__rwlock, __abstime);
}

static int
sctk_gen_thread_rwlock_timedwrlock(mpc_thread_rwlock_t *
                                   restrict __rwlock,
                                   const struct timespec *restrict __abstime)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_timedwrlock != __sctk_ptr_thread_rwlock_timedwrlock);
	return __sctk_ptr_thread_rwlock_timedwrlock(__rwlock, __abstime);
}

static int
sctk_gen_thread_rwlock_tryrdlock(mpc_thread_rwlock_t *__rwlock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_tryrdlock != __sctk_ptr_thread_rwlock_tryrdlock);
	return __sctk_ptr_thread_rwlock_tryrdlock(__rwlock);
}

static int
sctk_gen_thread_rwlock_trywrlock(mpc_thread_rwlock_t *__rwlock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_trywrlock != __sctk_ptr_thread_rwlock_trywrlock);
	return __sctk_ptr_thread_rwlock_trywrlock(__rwlock);
}

static int
sctk_gen_thread_rwlock_unlock(mpc_thread_rwlock_t *__rwlock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_unlock != __sctk_ptr_thread_rwlock_unlock);
	return __sctk_ptr_thread_rwlock_unlock(__rwlock);
}

static int
sctk_gen_thread_rwlock_wrlock(mpc_thread_rwlock_t *__rwlock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlock_wrlock != __sctk_ptr_thread_rwlock_wrlock);
	return __sctk_ptr_thread_rwlock_wrlock(__rwlock);
}

static mpc_thread_t
sctk_gen_thread_self(void)
{
	__base_mpc_init();
	assume(sctk_gen_thread_self != __sctk_ptr_thread_self);
	return __sctk_ptr_thread_self();
}


static int
sctk_gen_thread_setcancelstate(int __state, int *__oldstate)
{
	__base_mpc_init();
	assume(sctk_gen_thread_setcancelstate != __sctk_ptr_thread_setcancelstate);
	return __sctk_ptr_thread_setcancelstate(__state, __oldstate);
}

static int
sctk_gen_thread_setcanceltype(int __type, int *__oldtype)
{
	__base_mpc_init();
	assume(sctk_gen_thread_setcanceltype != __sctk_ptr_thread_setcanceltype);
	return __sctk_ptr_thread_setcanceltype(__type, __oldtype);
}

static int
sctk_gen_thread_setconcurrency(int __level)
{
	__base_mpc_init();
	assume(sctk_gen_thread_setconcurrency != __sctk_ptr_thread_setconcurrency);
	return __sctk_ptr_thread_setconcurrency(__level);
}

static int
sctk_gen_thread_setschedprio(mpc_thread_t __p, int __i)
{
	__base_mpc_init();
	assume(sctk_gen_thread_setschedprio != __sctk_ptr_thread_setschedprio);
	return __sctk_ptr_thread_setschedprio(__p, __i);
}

static int
sctk_gen_thread_setschedparam(mpc_thread_t __target_thread, int __policy,
                              const struct sched_param *__param)
{
	__base_mpc_init();
	assume(sctk_gen_thread_setschedparam != __sctk_ptr_thread_setschedparam);
	return __sctk_ptr_thread_setschedparam(__target_thread, __policy, __param);
}

static int
sctk_gen_thread_setspecific(mpc_thread_keys_t __key, const void *__pointer)
{
	__base_mpc_init();
	assume(sctk_gen_thread_setspecific != __sctk_ptr_thread_setspecific);
	return __sctk_ptr_thread_setspecific(__key, __pointer);
}

static int
sctk_gen_thread_spin_destroy(mpc_thread_spinlock_t *__lock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_spin_destroy != __sctk_ptr_thread_spin_destroy);
	return __sctk_ptr_thread_spin_destroy(__lock);
}

static int
sctk_gen_thread_spin_init(mpc_thread_spinlock_t *__lock, int __pshared)
{
	__base_mpc_init();
	assume(sctk_gen_thread_spin_init != __sctk_ptr_thread_spin_init);
	return __sctk_ptr_thread_spin_init(__lock, __pshared);
}

static int
sctk_gen_thread_spin_lock(mpc_thread_spinlock_t *__lock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_spin_lock != __sctk_ptr_thread_spin_lock);
	return __sctk_ptr_thread_spin_lock(__lock);
}

static int
sctk_gen_thread_spin_trylock(mpc_thread_spinlock_t *__lock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_spin_trylock != __sctk_ptr_thread_spin_trylock);
	return __sctk_ptr_thread_spin_trylock(__lock);
}

static int
sctk_gen_thread_spin_unlock(mpc_thread_spinlock_t *__lock)
{
	__base_mpc_init();
	assume(sctk_gen_thread_spin_unlock != __sctk_ptr_thread_spin_unlock);
	return __sctk_ptr_thread_spin_unlock(__lock);
}

static void
sctk_gen_thread_testcancel(void)
{
	__base_mpc_init();
	assume(sctk_gen_thread_testcancel != __sctk_ptr_thread_testcancel);
	return __sctk_ptr_thread_testcancel();
}

static int
sctk_gen_thread_yield(void)
{
	__base_mpc_init();
	assume(sctk_gen_thread_yield != __sctk_ptr_thread_yield);
	return __sctk_ptr_thread_yield();
}

static int
sctk_gen_thread_dump(char *file)
{
	__base_mpc_init();
	assume(sctk_gen_thread_dump != __sctk_ptr_thread_dump);
	return __sctk_ptr_thread_dump(file);
}

static int
sctk_gen_thread_dump_clean(void)
{
	__base_mpc_init();
	assume(sctk_gen_thread_dump_clean != __sctk_ptr_thread_dump_clean);
	return __sctk_ptr_thread_dump_clean();
}

static int sctk_gen_thread_migrate(void)
{
	__base_mpc_init();
	assume(sctk_gen_thread_migrate != __sctk_ptr_thread_migrate);
	return __sctk_ptr_thread_migrate();
}

static int
sctk_gen_thread_restore(mpc_thread_t thread, char *type, int vp)
{
	__base_mpc_init();
	assume(sctk_gen_thread_restore != __sctk_ptr_thread_restore);
	return __sctk_ptr_thread_restore(thread, type, vp);
}

static void
sctk_gen_thread_wait_for_value(volatile int *data, int value)
{
	__base_mpc_init();
	assume(sctk_gen_thread_wait_for_value != __sctk_ptr_thread_wait_for_value);
	__sctk_ptr_thread_wait_for_value(data, value);
}

static void
sctk_gen_thread_wait_for_value_and_poll(volatile int *data, int value,
                                        void (*func)(void *), void *arg)
{
	__base_mpc_init();
	assume(sctk_gen_thread_wait_for_value_and_poll != __sctk_ptr_thread_wait_for_value_and_poll);
	__sctk_ptr_thread_wait_for_value_and_poll(data, value, func, arg);
}

static void
sctk_gen_thread_freeze_thread_on_vp(mpc_thread_mutex_t *lock, void **list)
{
	__base_mpc_init();
	assume(sctk_gen_thread_freeze_thread_on_vp != __sctk_ptr_thread_freeze_thread_on_vp);
	__sctk_ptr_thread_freeze_thread_on_vp(lock, list);
}

static void
sctk_gen_thread_wake_thread_on_vp(void **list)
{
	__base_mpc_init();
	assume(sctk_gen_thread_wake_thread_on_vp != __sctk_ptr_thread_wake_thread_on_vp);
	__sctk_ptr_thread_wake_thread_on_vp(list);
}


static int sctk_gen_thread_proc_migration(const int cpu)
{
	/* Not implemented */
	return -1;
}

static int
sctk_gen_thread_getattr_np(mpc_thread_t th, mpc_thread_attr_t *attr)
{
	__base_mpc_init();
	/*assume(sctk_gen_thread_getattr_np != __sctk_ptr_thread_getattr_np);*/
	/*return __sctk_ptr_thread_getattr_np(th,attr);*/
	return pthread_getattr_np( (pthread_t)th, (pthread_attr_t *)attr);
}

static int
sctk_gen_thread_rwlockattr_getkind_np(mpc_thread_rwlockattr_t *attr,
                                      int *pref)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlockattr_getkind_np != __sctk_ptr_thread_rwlockattr_getkind_np);
	return __sctk_ptr_thread_rwlockattr_getkind_np(attr, pref);
}

static int
sctk_gen_thread_rwlockattr_setkind_np(mpc_thread_rwlockattr_t *attr,
                                      int pref)
{
	__base_mpc_init();
	assume(sctk_gen_thread_rwlockattr_setkind_np != __sctk_ptr_thread_rwlockattr_setkind_np);
	return __sctk_ptr_thread_rwlockattr_setkind_np(attr, pref);
}

static void
sctk_gen_thread_kill_other_threads_np(void)
{
	__base_mpc_init();
	assume(__sctk_ptr_thread_kill_other_threads_np != sctk_gen_thread_kill_other_threads_np);
	__sctk_ptr_thread_kill_other_threads_np();
}

int sctk_gen_thread_futex(void *addr1, int op, int val1,
                          struct timespec *timeout, void *addr2, int val3)
{
	return sctk_futex(addr1, op, val1, timeout, addr2, val3);
}

double (*__sctk_ptr_thread_get_activity) (int i) =
        sctk_gen_thread_get_activity;

int (*__sctk_ptr_thread_proc_migration) (const int cpu) =
        sctk_gen_thread_proc_migration;

int (*__sctk_ptr_thread_atfork) (void (*__prepare)(void),
                                 void (*__parent)(void),
                                 void (*__child)(void) ) =
        sctk_gen_thread_atfork;

int (*__sctk_ptr_thread_attr_destroy) (mpc_thread_attr_t *__attr) =
        sctk_gen_thread_attr_destroy;
int (*__sctk_ptr_thread_attr_getdetachstate) (const mpc_thread_attr_t *
                                              __attr, int *__detachstate) =
        sctk_gen_thread_attr_getdetachstate;
int (*__sctk_ptr_thread_attr_getguardsize) (const mpc_thread_attr_t *
                                            __attr, size_t *__guardsize) =
        sctk_gen_thread_attr_getguardsize;
int (*__sctk_ptr_thread_attr_getinheritsched) (const mpc_thread_attr_t *
                                               __attr, int *__inherit) =
        sctk_gen_thread_attr_getinheritsched;
int (*__sctk_ptr_thread_attr_getschedparam) (const mpc_thread_attr_t *
                                             __attr,
                                             struct sched_param *
                                             __param) =
        sctk_gen_thread_attr_getschedparam;
int (*__sctk_ptr_thread_attr_getschedpolicy) (const mpc_thread_attr_t *
                                              __attr, int *__policy) =
        sctk_gen_thread_attr_getschedpolicy;
int (*__sctk_ptr_thread_attr_getscope) (const mpc_thread_attr_t *__attr,
                                        int *__scope) =
        sctk_gen_thread_attr_getscope;
int (*__sctk_ptr_thread_attr_getstackaddr) (const mpc_thread_attr_t *
                                            __attr, void **__stackaddr) =
        sctk_gen_thread_attr_getstackaddr;
int (*__sctk_ptr_thread_attr_getstack) (const mpc_thread_attr_t *__attr,
                                        void **__stackaddr,
                                        size_t *__stacksize) =
        sctk_gen_thread_attr_getstack;
int (*__sctk_ptr_thread_attr_getstacksize) (const mpc_thread_attr_t *
                                            __attr, size_t *__stacksize) =
        sctk_gen_thread_attr_getstacksize;
int (*__sctk_ptr_thread_attr_init) (mpc_thread_attr_t *__attr) =
        sctk_gen_thread_attr_init;
int (*__sctk_ptr_thread_attr_setdetachstate) (mpc_thread_attr_t *__attr,
                                              int __detachstate) =
        sctk_gen_thread_attr_setdetachstate;
int (*__sctk_ptr_thread_attr_setguardsize) (mpc_thread_attr_t *__attr,
                                            size_t __guardsize) =
        sctk_gen_thread_attr_setguardsize;
int (*__sctk_ptr_thread_attr_setinheritsched) (mpc_thread_attr_t *__attr,
                                               int __inherit) =
        sctk_gen_thread_attr_setinheritsched;
int (*__sctk_ptr_thread_attr_setschedparam) (mpc_thread_attr_t *__attr,
                                             const struct sched_param *
                                             __param) =
        sctk_gen_thread_attr_setschedparam;
int (*__sctk_ptr_thread_attr_setschedpolicy) (mpc_thread_attr_t *__attr,
                                              int __policy) =
        sctk_gen_thread_attr_setschedpolicy;
int (*__sctk_ptr_thread_attr_setscope) (mpc_thread_attr_t *__attr,
                                        int __scope) =
        sctk_gen_thread_attr_setscope;
int (*__sctk_ptr_thread_attr_setstackaddr) (mpc_thread_attr_t *__attr,
                                            void *__stackaddr) =
        sctk_gen_thread_attr_setstackaddr;
int (*__sctk_ptr_thread_attr_setstack) (mpc_thread_attr_t *__attr,
                                        void *__stackaddr,
                                        size_t __stacksize) =
        sctk_gen_thread_attr_setstack;
int (*__sctk_ptr_thread_attr_setstacksize) (mpc_thread_attr_t *__attr,
                                            size_t __stacksize) =
        sctk_gen_thread_attr_setstacksize;


int
(*__sctk_ptr_thread_attr_setbinding) (mpc_thread_attr_t *__attr,
                                      int __binding) =
        sctk_gen_thread_attr_setbinding;
int (*__sctk_ptr_thread_attr_getbinding) (mpc_thread_attr_t *__attr,
                                          int *__binding) =
        sctk_gen_thread_attr_getbinding;


int (*__sctk_ptr_thread_barrierattr_destroy) (mpc_thread_barrierattr_t *
                                              __attr) =
        sctk_gen_thread_barrierattr_destroy;
int (*__sctk_ptr_thread_barrierattr_init) (mpc_thread_barrierattr_t *
                                           __attr) =
        sctk_gen_thread_barrierattr_init;
int (*__sctk_ptr_thread_barrierattr_setpshared) (mpc_thread_barrierattr_t
                                                 *__attr, int __pshared) =
        sctk_gen_thread_barrierattr_setpshared;
int (*__sctk_ptr_thread_barrierattr_getpshared) (const
                                                 mpc_thread_barrierattr_t
                                                 *__attr,
                                                 int *__pshared) =
        sctk_gen_thread_barrierattr_getpshared;
int (*__sctk_ptr_thread_barrier_destroy) (mpc_thread_barrier_t *
                                          __barrier) =
        sctk_gen_thread_barrier_destroy;
int (*__sctk_ptr_thread_barrier_init) (mpc_thread_barrier_t *
                                       restrict __barrier,
                                       const mpc_thread_barrierattr_t *
                                       restrict __attr,
                                       unsigned int __count) =
        sctk_gen_thread_barrier_init;
int (*__sctk_ptr_thread_barrier_wait) (mpc_thread_barrier_t *__barrier) =
        sctk_gen_thread_barrier_wait;

int (*__sctk_ptr_thread_cancel) (mpc_thread_t __cancelthread) =
        sctk_gen_thread_cancel;
int (*__sctk_ptr_thread_condattr_destroy) (mpc_thread_condattr_t *
                                           __attr) =
        sctk_gen_thread_condattr_destroy;
int (*__sctk_ptr_thread_condattr_getpshared) (const mpc_thread_condattr_t
                                              *__attr, int *__pshared) =
        sctk_gen_thread_condattr_getpshared;
int (*__sctk_ptr_thread_condattr_init) (mpc_thread_condattr_t *__attr) =
        sctk_gen_thread_condattr_init;
int (*__sctk_ptr_thread_condattr_setpshared) (mpc_thread_condattr_t *
                                              __attr, int __pshared) =
        sctk_gen_thread_condattr_setpshared;
int (*__sctk_ptr_thread_condattr_setclock) (mpc_thread_condattr_t *attr,
                                            clockid_t clock_id) =
        sctk_gen_thread_condattr_setclock;
int (*__sctk_ptr_thread_condattr_getclock) (mpc_thread_condattr_t *attr,
                                            clockid_t *clock_id) =
        sctk_gen_thread_condattr_getclock;
int (*__sctk_ptr_thread_cond_broadcast) (mpc_thread_cond_t *__cond) =
        sctk_gen_thread_cond_broadcast;
int (*__sctk_ptr_thread_cond_destroy) (mpc_thread_cond_t *__cond) =
        sctk_gen_thread_cond_destroy;
int (*__sctk_ptr_thread_cond_init) (mpc_thread_cond_t *__cond,
                                    const mpc_thread_condattr_t *
                                    __cond_attr) = sctk_gen_thread_cond_init;
int (*__sctk_ptr_thread_cond_signal) (mpc_thread_cond_t *__cond) =
        sctk_gen_thread_cond_signal;
int (*__sctk_ptr_thread_cond_timedwait) (mpc_thread_cond_t *__cond,
                                         mpc_thread_mutex_t *__mutex,
                                         const struct timespec *
                                         __abstime) =
        sctk_gen_thread_cond_timedwait;
int (*__sctk_ptr_thread_cond_wait) (mpc_thread_cond_t *__cond,
                                    mpc_thread_mutex_t *__mutex) =
        sctk_gen_thread_cond_wait;
int (*__sctk_ptr_thread_create) (mpc_thread_t *__threadp,
                                 const mpc_thread_attr_t *__attr,
                                 void *(*__start_routine)(void *),
                                 void *__arg) = sctk_gen_thread_create;
int (*__sctk_ptr_thread_user_create) (mpc_thread_t *__threadp,
                                      const mpc_thread_attr_t *__attr,
                                      void *(*__start_routine)(void *),
                                      void *__arg) =
        sctk_gen_thread_user_create;
int (*__sctk_ptr_thread_detach) (mpc_thread_t __th) = sctk_gen_thread_detach;
int (*__sctk_ptr_thread_equal) (mpc_thread_t __thread1,
                                mpc_thread_t __thread2) =
        sctk_gen_thread_equal;
void (*__sctk_ptr_thread_exit) (void *__retval) = sctk_gen_thread_exit;
int  (*__sctk_ptr_thread_getconcurrency) (void) =
        sctk_gen_thread_getconcurrency;
int (*__sctk_ptr_thread_getcpuclockid) (mpc_thread_t __thread_id,
                                        clockid_t *__clock_id) =
        sctk_gen_thread_getcpuclockid;
int (*__sctk_ptr_thread_getschedparam) (mpc_thread_t __target_thread,
                                        int *__policy,
                                        struct sched_param *__param) =
        sctk_gen_thread_getschedparam;
int (*__sctk_ptr_thread_sched_get_priority_max) (int policy) =
        sctk_gen_thread_sched_get_priority_max;
int (*__sctk_ptr_thread_sched_get_priority_min) (int policy) =
        sctk_gen_thread_sched_get_priority_min;

void *(*__sctk_ptr_thread_getspecific) (mpc_thread_keys_t __key) =
        sctk_gen_thread_getspecific;
int (*__sctk_ptr_thread_join) (mpc_thread_t __th,
                               void **__thread_return) = sctk_gen_thread_join;
int (*__sctk_ptr_thread_kill) (mpc_thread_t thread, int signo) =
        sctk_gen_thread_kill;
int (*__sctk_ptr_thread_sigsuspend) (sigset_t *set) =
        sctk_gen_thread_sigsuspend;
int (*__sctk_ptr_thread_sigpending) (sigset_t *set) =
        sctk_gen_thread_sigpending;
int (*__sctk_ptr_thread_sigmask) (int how, const sigset_t *newmask,
                                  sigset_t *oldmask) =
        sctk_gen_thread_sigmask;
int (*__sctk_ptr_thread_key_create) (mpc_thread_keys_t *__key,
                                     void (*__destr_function)(void *) ) =
        sctk_gen_thread_key_create;
int (*__sctk_ptr_thread_key_delete) (mpc_thread_keys_t __key) =
        sctk_gen_thread_key_delete;
int (*__sctk_ptr_thread_mutexattr_destroy) (mpc_thread_mutexattr_t *
                                            __attr) =
        sctk_gen_thread_mutexattr_destroy;

int (*__sctk_ptr_thread_mutexattr_getprioceiling) (const
                                                   mpc_thread_mutexattr_t
                                                   *__attr,
                                                   int *__prioceiling) =
        sctk_gen_thread_mutexattr_getprioceiling;
int (*__sctk_ptr_thread_mutexattr_setprioceiling) (mpc_thread_mutexattr_t
                                                   *__attr,
                                                   int __prioceiling) =
        sctk_gen_thread_mutexattr_setprioceiling;
int (*__sctk_ptr_thread_mutexattr_getprotocol) (const
                                                mpc_thread_mutexattr_t *
                                                __attr, int *__protocol) =
        sctk_gen_thread_mutexattr_getprotocol;
int (*__sctk_ptr_thread_mutexattr_setprotocol) (mpc_thread_mutexattr_t *
                                                __attr, int __protocol) =
        sctk_gen_thread_mutexattr_setprotocol;

int (*__sctk_ptr_thread_mutexattr_getpshared) (const
                                               mpc_thread_mutexattr_t *
                                               __attr, int *__pshared) =
        sctk_gen_thread_mutexattr_getpshared;
int (*__sctk_ptr_thread_mutexattr_gettype) (const mpc_thread_mutexattr_t *
                                            __attr, int *__kind) =
        sctk_gen_thread_mutexattr_gettype;
int (*__sctk_ptr_thread_mutexattr_init) (mpc_thread_mutexattr_t *
                                         __attr) =
        sctk_gen_thread_mutexattr_init;
int (*__sctk_ptr_thread_mutexattr_setpshared) (mpc_thread_mutexattr_t *
                                               __attr, int __pshared) =
        sctk_gen_thread_mutexattr_setpshared;
int (*__sctk_ptr_thread_mutexattr_settype) (mpc_thread_mutexattr_t *
                                            __attr, int __kind) =
        sctk_gen_thread_mutexattr_settype;
int (*__sctk_ptr_thread_mutex_destroy) (mpc_thread_mutex_t *__mutex) =
        sctk_gen_thread_mutex_destroy;
int (*__sctk_ptr_thread_mutex_init) (mpc_thread_mutex_t *__mutex,
                                     const mpc_thread_mutexattr_t *
                                     __mutex_attr) =
        sctk_gen_thread_mutex_init;
int (*__sctk_ptr_thread_mutex_lock) (mpc_thread_mutex_t *__mutex) =
        sctk_gen_thread_mutex_lock;
int (*__sctk_ptr_thread_mutex_spinlock) (mpc_thread_mutex_t *__mutex) =
        sctk_gen_thread_mutex_spinlock;
int (*__sctk_ptr_thread_mutex_timedlock) (mpc_thread_mutex_t *__mutex,
                                          const struct timespec *
                                          __abstime) =
        sctk_gen_thread_mutex_timedlock;
int (*__sctk_ptr_thread_mutex_trylock) (mpc_thread_mutex_t *__mutex) =
        sctk_gen_thread_mutex_trylock;
int (*__sctk_ptr_thread_mutex_unlock) (mpc_thread_mutex_t *__mutex) =
        sctk_gen_thread_mutex_unlock;

int (*__sctk_ptr_thread_sem_init) (mpc_thread_sem_t *sem, int pshared,
                                   unsigned int value) =
        sctk_gen_thread_sem_init;
int (*__sctk_ptr_thread_sem_wait) (mpc_thread_sem_t *sem) =
        sctk_gen_thread_sem_wait;
int (*__sctk_ptr_thread_sem_trywait) (mpc_thread_sem_t *sem) =
        sctk_gen_thread_sem_trywait;
int (*__sctk_ptr_thread_sem_timedwait) (mpc_thread_sem_t *sem,
                                        const struct timespec *__abstime) =
        sctk_gen_thread_sem_timedwait;
int (*__sctk_ptr_thread_sem_post) (mpc_thread_sem_t *sem) =
        sctk_gen_thread_sem_post;
int (*__sctk_ptr_thread_sem_getvalue) (mpc_thread_sem_t *sem,
                                       int *sval) =
        sctk_gen_thread_sem_getvalue;
int (*__sctk_ptr_thread_sem_destroy) (mpc_thread_sem_t *sem) =
        sctk_gen_thread_sem_destroy;

mpc_thread_sem_t *(*__sctk_ptr_thread_sem_open) (const char *name,
                                                  int oflag, ...) =
        sctk_gen_thread_sem_open;
int (*__sctk_ptr_thread_sem_close) (mpc_thread_sem_t *sem) =
        sctk_gen_thread_sem_close;
int (*__sctk_ptr_thread_sem_unlink) (const char *name) =
        sctk_gen_thread_sem_unlink;

int (*__sctk_ptr_thread_once) (mpc_thread_once_t *__once_control,
                               void (*__init_routine)(void) ) =
        sctk_gen_thread_once;

int (*__sctk_ptr_thread_rwlockattr_destroy) (mpc_thread_rwlockattr_t *
                                             __attr) =
        sctk_gen_thread_rwlockattr_destroy;
int (*__sctk_ptr_thread_rwlockattr_getpshared) (const
                                                mpc_thread_rwlockattr_t *
                                                restrict __attr,
                                                int *restrict __pshared) =
        sctk_gen_thread_rwlockattr_getpshared;
int (*__sctk_ptr_thread_rwlockattr_init) (mpc_thread_rwlockattr_t *
                                          __attr) =
        sctk_gen_thread_rwlockattr_init;
int (*__sctk_ptr_thread_rwlockattr_setpshared) (mpc_thread_rwlockattr_t *
                                                __attr, int __pshared) =
        sctk_gen_thread_rwlockattr_setpshared;
int (*__sctk_ptr_thread_rwlock_destroy) (mpc_thread_rwlock_t *__rwlock) =
        sctk_gen_thread_rwlock_destroy;
int (*__sctk_ptr_thread_rwlock_init) (mpc_thread_rwlock_t *
                                      restrict __rwlock,
                                      const mpc_thread_rwlockattr_t *
                                      restrict __attr) =
        sctk_gen_thread_rwlock_init;
int (*__sctk_ptr_thread_rwlock_rdlock) (mpc_thread_rwlock_t *__rwlock) =
        sctk_gen_thread_rwlock_rdlock;

int (*__sctk_ptr_thread_rwlock_timedrdlock) (mpc_thread_rwlock_t *
                                             __rwlock,
                                             const struct timespec *
                                             __abstime) =
        sctk_gen_thread_rwlock_timedrdlock;
int (*__sctk_ptr_thread_rwlock_timedwrlock) (mpc_thread_rwlock_t *
                                             __rwlock,
                                             const struct timespec *
                                             __abstime) =
        sctk_gen_thread_rwlock_timedwrlock;

int (*__sctk_ptr_thread_rwlock_tryrdlock) (mpc_thread_rwlock_t *
                                           __rwlock) =
        sctk_gen_thread_rwlock_tryrdlock;
int (*__sctk_ptr_thread_rwlock_trywrlock) (mpc_thread_rwlock_t *
                                           __rwlock) =
        sctk_gen_thread_rwlock_trywrlock;
int (*__sctk_ptr_thread_rwlock_unlock) (mpc_thread_rwlock_t *__rwlock) =
        sctk_gen_thread_rwlock_unlock;
int (*__sctk_ptr_thread_rwlock_wrlock) (mpc_thread_rwlock_t *__rwlock) =
        sctk_gen_thread_rwlock_wrlock;

mpc_thread_t (*__sctk_ptr_thread_self) (void) = sctk_gen_thread_self;

int (*__sctk_ptr_thread_setcancelstate) (int __state, int *__oldstate) =
        sctk_gen_thread_setcancelstate;
int (*__sctk_ptr_thread_setcanceltype) (int __type, int *__oldtype) =
        sctk_gen_thread_setcanceltype;
int (*__sctk_ptr_thread_setconcurrency) (int __level) =
        sctk_gen_thread_setconcurrency;
int (*__sctk_ptr_thread_setschedprio) (mpc_thread_t __p, int __i) =
        sctk_gen_thread_setschedprio;
int (*__sctk_ptr_thread_setschedparam) (mpc_thread_t __target_thread,
                                        int __policy,
                                        const struct sched_param *
                                        __param) =
        sctk_gen_thread_setschedparam;
int (*__sctk_ptr_thread_setspecific) (mpc_thread_keys_t __key,
                                      const void *__pointer) =
        sctk_gen_thread_setspecific;

int (*__sctk_ptr_thread_spin_destroy) (mpc_thread_spinlock_t *__lock) =
        sctk_gen_thread_spin_destroy;
int (*__sctk_ptr_thread_spin_init) (mpc_thread_spinlock_t *__lock,
                                    int __pshared) =
        sctk_gen_thread_spin_init;
int (*__sctk_ptr_thread_spin_lock) (mpc_thread_spinlock_t *__lock) =
        sctk_gen_thread_spin_lock;
int (*__sctk_ptr_thread_spin_trylock) (mpc_thread_spinlock_t *__lock) =
        sctk_gen_thread_spin_trylock;
int (*__sctk_ptr_thread_spin_unlock) (mpc_thread_spinlock_t *__lock) =
        sctk_gen_thread_spin_unlock;

void (*__sctk_ptr_thread_testcancel) (void) = sctk_gen_thread_testcancel;
int  (*__sctk_ptr_thread_yield) (void) = sctk_gen_thread_yield;
int  (*__sctk_ptr_thread_dump) (char *file) = sctk_gen_thread_dump;
int  (*__sctk_ptr_thread_migrate) (void) = sctk_gen_thread_migrate;
int  (*__sctk_ptr_thread_restore) (mpc_thread_t thread, char *type,
                                   int vp) = sctk_gen_thread_restore;
int (*__sctk_ptr_thread_dump_clean) (void) = sctk_gen_thread_dump_clean;

void (*__sctk_ptr_thread_wait_for_value) (volatile int *data, int value) =
        sctk_gen_thread_wait_for_value;
void (*__sctk_ptr_thread_wait_for_value_and_poll) (volatile int *data, int value,
                                                   void (*func)(void *),
                                                   void *arg) =
        sctk_gen_thread_wait_for_value_and_poll;

void (*__sctk_ptr_thread_freeze_thread_on_vp) (mpc_thread_mutex_t *lock,
                                               void **list) =
        sctk_gen_thread_freeze_thread_on_vp;
void (*__sctk_ptr_thread_wake_thread_on_vp) (void **list) =
        sctk_gen_thread_wake_thread_on_vp;


int (*__sctk_ptr_thread_getattr_np) (mpc_thread_t th,
                                     mpc_thread_attr_t *attr) =
        sctk_gen_thread_getattr_np;
int (*__sctk_ptr_thread_rwlockattr_getkind_np) (mpc_thread_rwlockattr_t *
                                                attr, int *ret) =
        sctk_gen_thread_rwlockattr_getkind_np;
int (*__sctk_ptr_thread_rwlockattr_setkind_np) (mpc_thread_rwlockattr_t *
                                                attr, int pref) =
        sctk_gen_thread_rwlockattr_setkind_np;

void (*__sctk_ptr_thread_kill_other_threads_np) (void) =
        sctk_gen_thread_kill_other_threads_np;

int (*__sctk_ptr_thread_futex)(void *addr1, int op, int val1,
                               struct timespec *timeout, void *addr2, int val3) =
        sctk_gen_thread_futex;

void
sctk_gen_thread_init()
{
	__sctk_ptr_thread_atfork               = sctk_gen_thread_atfork;
	__sctk_ptr_thread_attr_destroy         = sctk_gen_thread_attr_destroy;
	__sctk_ptr_thread_attr_getdetachstate  = sctk_gen_thread_attr_getdetachstate;
	__sctk_ptr_thread_attr_getguardsize    = sctk_gen_thread_attr_getguardsize;
	__sctk_ptr_thread_attr_getinheritsched =
	        sctk_gen_thread_attr_getinheritsched;
	__sctk_ptr_thread_attr_getschedparam   = sctk_gen_thread_attr_getschedparam;
	__sctk_ptr_thread_attr_getschedpolicy  = sctk_gen_thread_attr_getschedpolicy;
	__sctk_ptr_thread_attr_getscope        = sctk_gen_thread_attr_getscope;
	__sctk_ptr_thread_attr_getstackaddr    = sctk_gen_thread_attr_getstackaddr;
	__sctk_ptr_thread_attr_getstack        = sctk_gen_thread_attr_getstack;
	__sctk_ptr_thread_attr_getstacksize    = sctk_gen_thread_attr_getstacksize;
	__sctk_ptr_thread_attr_init            = sctk_gen_thread_attr_init;
	__sctk_ptr_thread_attr_setdetachstate  = sctk_gen_thread_attr_setdetachstate;
	__sctk_ptr_thread_attr_setguardsize    = sctk_gen_thread_attr_setguardsize;
	__sctk_ptr_thread_attr_setinheritsched =
	        sctk_gen_thread_attr_setinheritsched;
	__sctk_ptr_thread_attr_setschedparam  = sctk_gen_thread_attr_setschedparam;
	__sctk_ptr_thread_attr_setschedpolicy = sctk_gen_thread_attr_setschedpolicy;
	__sctk_ptr_thread_attr_setscope       = sctk_gen_thread_attr_setscope;
	__sctk_ptr_thread_attr_setstackaddr   = sctk_gen_thread_attr_setstackaddr;
	__sctk_ptr_thread_attr_setstack       = sctk_gen_thread_attr_setstack;
	__sctk_ptr_thread_attr_setstacksize   = sctk_gen_thread_attr_setstacksize;
	__sctk_ptr_thread_attr_setbinding     = sctk_gen_thread_attr_setbinding;
	__sctk_ptr_thread_attr_getbinding     = sctk_gen_thread_attr_getbinding;

	__sctk_ptr_thread_barrierattr_destroy    = sctk_gen_thread_barrierattr_destroy;
	__sctk_ptr_thread_barrierattr_init       = sctk_gen_thread_barrierattr_init;
	__sctk_ptr_thread_barrierattr_setpshared =
	        sctk_gen_thread_barrierattr_setpshared;
	__sctk_ptr_thread_barrierattr_getpshared =
	        sctk_gen_thread_barrierattr_getpshared;
	__sctk_ptr_thread_barrier_destroy = sctk_gen_thread_barrier_destroy;
	__sctk_ptr_thread_barrier_init    = sctk_gen_thread_barrier_init;
	__sctk_ptr_thread_barrier_wait    = sctk_gen_thread_barrier_wait;

	__sctk_ptr_thread_cancel              = sctk_gen_thread_cancel;
	__sctk_ptr_thread_condattr_destroy    = sctk_gen_thread_condattr_destroy;
	__sctk_ptr_thread_condattr_getpshared = sctk_gen_thread_condattr_getpshared;
	__sctk_ptr_thread_condattr_init       = sctk_gen_thread_condattr_init;
	__sctk_ptr_thread_condattr_getclock   = sctk_gen_thread_condattr_getclock;
	__sctk_ptr_thread_condattr_setclock   = sctk_gen_thread_condattr_setclock;
	__sctk_ptr_thread_condattr_setpshared = sctk_gen_thread_condattr_setpshared;
	__sctk_ptr_thread_cond_broadcast      = sctk_gen_thread_cond_broadcast;
	__sctk_ptr_thread_cond_destroy        = sctk_gen_thread_cond_destroy;
	__sctk_ptr_thread_cond_init           = sctk_gen_thread_cond_init;
	__sctk_ptr_thread_cond_signal         = sctk_gen_thread_cond_signal;
	__sctk_ptr_thread_cond_timedwait      = sctk_gen_thread_cond_timedwait;
	__sctk_ptr_thread_cond_wait           = sctk_gen_thread_cond_wait;
	__sctk_ptr_thread_create              = sctk_gen_thread_create;
	__sctk_ptr_thread_detach              = sctk_gen_thread_detach;
	__sctk_ptr_thread_equal                  = sctk_gen_thread_equal;
	__sctk_ptr_thread_exit                   = sctk_gen_thread_exit;
	__sctk_ptr_thread_getconcurrency         = sctk_gen_thread_getconcurrency;
	__sctk_ptr_thread_getcpuclockid          = sctk_gen_thread_getcpuclockid;
	__sctk_ptr_thread_getschedparam          = sctk_gen_thread_getschedparam;
	__sctk_ptr_thread_sched_get_priority_max =
	        sctk_gen_thread_sched_get_priority_max;
	__sctk_ptr_thread_sched_get_priority_min =
	        sctk_gen_thread_sched_get_priority_min;


	__sctk_ptr_thread_getspecific          = sctk_gen_thread_getspecific;
	__sctk_ptr_thread_join                 = sctk_gen_thread_join;
	__sctk_ptr_thread_key_create           = sctk_gen_thread_key_create;
	__sctk_ptr_thread_key_delete           = sctk_gen_thread_key_delete;
	__sctk_ptr_thread_mutexattr_destroy    = sctk_gen_thread_mutexattr_destroy;
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

	__sctk_ptr_thread_mutexattr_gettype    = sctk_gen_thread_mutexattr_gettype;
	__sctk_ptr_thread_mutexattr_init       = sctk_gen_thread_mutexattr_init;
	__sctk_ptr_thread_mutexattr_setpshared =
	        sctk_gen_thread_mutexattr_setpshared;
	__sctk_ptr_thread_mutexattr_settype = sctk_gen_thread_mutexattr_settype;
	__sctk_ptr_thread_mutex_destroy     = sctk_gen_thread_mutex_destroy;
	__sctk_ptr_thread_mutex_init        = sctk_gen_thread_mutex_init;
	__sctk_ptr_thread_mutex_lock        = sctk_gen_thread_mutex_lock;
	__sctk_ptr_thread_mutex_spinlock    = sctk_gen_thread_mutex_spinlock;
	__sctk_ptr_thread_mutex_timedlock   = sctk_gen_thread_mutex_timedlock;
	__sctk_ptr_thread_mutex_trylock     = sctk_gen_thread_mutex_trylock;
	__sctk_ptr_thread_mutex_unlock      = sctk_gen_thread_mutex_unlock;
	__sctk_ptr_thread_once = sctk_gen_thread_once;

	__sctk_ptr_thread_rwlockattr_destroy    = sctk_gen_thread_rwlockattr_destroy;
	__sctk_ptr_thread_rwlockattr_getpshared =
	        sctk_gen_thread_rwlockattr_getpshared;
	__sctk_ptr_thread_rwlockattr_init       = sctk_gen_thread_rwlockattr_init;
	__sctk_ptr_thread_rwlockattr_setpshared =
	        sctk_gen_thread_rwlockattr_setpshared;
	__sctk_ptr_thread_rwlock_destroy = sctk_gen_thread_rwlock_destroy;
	__sctk_ptr_thread_rwlock_init    = sctk_gen_thread_rwlock_init;
	__sctk_ptr_thread_rwlock_rdlock  = sctk_gen_thread_rwlock_rdlock;

	__sctk_ptr_thread_rwlock_timedrdlock = sctk_gen_thread_rwlock_timedrdlock;
	__sctk_ptr_thread_rwlock_timedwrlock = sctk_gen_thread_rwlock_timedwrlock;

	__sctk_ptr_thread_rwlock_tryrdlock = sctk_gen_thread_rwlock_tryrdlock;
	__sctk_ptr_thread_rwlock_trywrlock = sctk_gen_thread_rwlock_trywrlock;
	__sctk_ptr_thread_rwlock_unlock    = sctk_gen_thread_rwlock_unlock;
	__sctk_ptr_thread_rwlock_wrlock    = sctk_gen_thread_rwlock_wrlock;

	__sctk_ptr_thread_self           = sctk_gen_thread_self;
	__sctk_ptr_thread_setcancelstate = sctk_gen_thread_setcancelstate;
	__sctk_ptr_thread_setcanceltype  = sctk_gen_thread_setcanceltype;
	__sctk_ptr_thread_setconcurrency = sctk_gen_thread_setconcurrency;
	__sctk_ptr_thread_setschedprio   = sctk_gen_thread_setschedprio;
	__sctk_ptr_thread_setschedparam  = sctk_gen_thread_setschedparam;
	__sctk_ptr_thread_setspecific    = sctk_gen_thread_setspecific;

	__sctk_ptr_thread_spin_destroy = sctk_gen_thread_spin_destroy;
	__sctk_ptr_thread_spin_init    = sctk_gen_thread_spin_init;
	__sctk_ptr_thread_spin_lock    = sctk_gen_thread_spin_lock;
	__sctk_ptr_thread_spin_trylock = sctk_gen_thread_spin_trylock;
	__sctk_ptr_thread_spin_unlock  = sctk_gen_thread_spin_unlock;

	__sctk_ptr_thread_testcancel = sctk_gen_thread_testcancel;
	__sctk_ptr_thread_yield      = sctk_gen_thread_yield;
	__sctk_ptr_thread_dump       = sctk_gen_thread_dump;
	__sctk_ptr_thread_dump_clean = sctk_gen_thread_dump_clean;
	__sctk_ptr_thread_migrate    = sctk_gen_thread_migrate;
	__sctk_ptr_thread_restore    = sctk_gen_thread_restore;

	__sctk_ptr_thread_wait_for_value          = sctk_gen_thread_wait_for_value;
	__sctk_ptr_thread_wait_for_value_and_poll =
	        sctk_gen_thread_wait_for_value_and_poll;
	__sctk_ptr_thread_freeze_thread_on_vp = sctk_gen_thread_freeze_thread_on_vp;
	__sctk_ptr_thread_wake_thread_on_vp   = sctk_gen_thread_wake_thread_on_vp;


	__sctk_ptr_thread_getattr_np            = sctk_gen_thread_getattr_np;
	__sctk_ptr_thread_rwlockattr_getkind_np =
	        sctk_gen_thread_rwlockattr_getkind_np;
	__sctk_ptr_thread_rwlockattr_setkind_np =
	        sctk_gen_thread_rwlockattr_setkind_np;
	__sctk_ptr_thread_kill_other_threads_np =
	        sctk_gen_thread_kill_other_threads_np;

	__sctk_ptr_thread_futex = sctk_gen_thread_futex;
}
