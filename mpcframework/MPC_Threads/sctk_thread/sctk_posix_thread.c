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

#include <mpcthread.h>
#include "sctk_futex.h"
#include "sctk_thread.h"


/* pthread_atfork */
int mpc_thread_atfork (void (*a) (void), void (*b) (void),
		       void (*c) (void))
{
  return sctk_thread_atfork (a, b, c);
}
/* pthread_attr_destroy */
int mpc_thread_attr_destroy (sctk_thread_attr_t * attr)
{
  return sctk_thread_attr_destroy (attr);
}

/* pthread_attr_getdetachstate */
int mpc_thread_attr_getdetachstate (const
				    sctk_thread_attr_t *
				    attr, int *state)
{
  return sctk_thread_attr_getdetachstate (attr, state);
}

/* pthread_attr_getguardsize */
int mpc_thread_attr_getguardsize (const sctk_thread_attr_t
				  * attr, size_t * size)
{
  return sctk_thread_attr_getguardsize (attr, size);
}

/* pthread_attr_getinheritsched */
int mpc_thread_attr_getinheritsched (const
				     sctk_thread_attr_t *
				     attr, int *type)
{
  return sctk_thread_attr_getinheritsched (attr, type);
}
/* pthread_attr_getschedparam */
int mpc_thread_attr_getschedparam (const
				   sctk_thread_attr_t *
				   attr,
				   struct sched_param *type)
{
  return sctk_thread_attr_getschedparam (attr, type);
}

/* pthread_attr_getschedpolicy */
int mpc_thread_attr_getschedpolicy (const
				    sctk_thread_attr_t *
				    attr, int *policy)
{
  return sctk_thread_attr_getschedpolicy (attr, policy);
}

/* pthread_attr_getscope */
int mpc_thread_attr_getscope (const sctk_thread_attr_t *
			      attr, int *scope)
{
  return sctk_thread_attr_getscope (attr, scope);
}

/* pthread_attr_getstack */
int mpc_thread_attr_getstack (const sctk_thread_attr_t *
			      attr, void **stack,
			      size_t * size)
{
  return sctk_thread_attr_getstack (attr, stack, size);
}

/* pthread_attr_getstackaddr */
int mpc_thread_attr_getstackaddr (const sctk_thread_attr_t
				  * attr, void **stack)
{
  return sctk_thread_attr_getstackaddr (attr, stack);
}

/* pthread_attr_getstacksize */
int mpc_thread_attr_getstacksize (const sctk_thread_attr_t
				  * attr, size_t * size)
{
  return sctk_thread_attr_getstacksize (attr, size);
}

/* pthread_attr_init */
int mpc_thread_attr_init (sctk_thread_attr_t * attr)
{
  return sctk_thread_attr_init (attr);
}


/* pthread_attr_setdetachstate */
int mpc_thread_attr_setdetachstate (sctk_thread_attr_t *
				    attr, int state)
{
  return sctk_thread_attr_setdetachstate (attr, state);
}


/* pthread_attr_setguardsize */
int mpc_thread_attr_setguardsize (sctk_thread_attr_t *
				  attr, size_t size)
{
  return sctk_thread_attr_setguardsize (attr, size);
}


/* pthread_attr_setinheritsched */
int mpc_thread_attr_setinheritsched (sctk_thread_attr_t *
				     attr, int sched)
{
  return sctk_thread_attr_setinheritsched (attr, sched);
}


/* pthread_attr_setschedparam */
int mpc_thread_attr_setschedparam (sctk_thread_attr_t *
				   attr, const struct
				   sched_param *sched)
{
  return sctk_thread_attr_setschedparam (attr, sched);
}
/* pthread_attr_setschedpolicy */
int mpc_thread_attr_setschedpolicy (sctk_thread_attr_t *
				    attr, int policy)
{
  return sctk_thread_attr_setschedpolicy (attr, policy);
}


/* pthread_attr_setscope */
int mpc_thread_attr_setscope (sctk_thread_attr_t * attr,
			      int scope)
{
  return sctk_thread_attr_setscope (attr, scope);
}


/* pthread_attr_setstack */
int mpc_thread_attr_setstack (sctk_thread_attr_t * attr,
			      void *stack, size_t size)
{
  return sctk_thread_attr_setstack (attr, stack, size);
}


/* pthread_attr_setstackaddr */
int mpc_thread_attr_setstackaddr (sctk_thread_attr_t *
				  attr, void *stack)
{
  return sctk_thread_attr_setstackaddr (attr, stack);
}


/* pthread_attr_setstacksize */
int mpc_thread_attr_setstacksize (sctk_thread_attr_t *
				  attr, size_t size)
{
  return sctk_thread_attr_setstacksize (attr, size);
}


/* pthread_barrier_destroy */
int mpc_thread_barrier_destroy (sctk_thread_barrier_t *
				barrier)
{
  return sctk_thread_barrier_destroy (barrier);
}


/* pthread_barrier_init */
int mpc_thread_barrier_init (sctk_thread_barrier_t *
			     barrier, const
			     sctk_thread_barrierattr_t *
			     attr, unsigned i)
{
  return sctk_thread_barrier_init (barrier, attr, i);
}

/* pthread_barrier_wait */
int mpc_thread_barrier_wait (sctk_thread_barrier_t * barrier)
{
  return sctk_thread_barrier_wait (barrier);
}


/* pthread_barrierattr_destroy */
int
mpc_thread_barrierattr_destroy (sctk_thread_barrierattr_t * barrier)
{
  return sctk_thread_barrierattr_destroy (barrier);
}


/* pthread_barrierattr_getpshared */
int mpc_thread_barrierattr_getpshared (const
				       sctk_thread_barrierattr_t
				       * barrier,
				       int *pshared)
{
  return sctk_thread_barrierattr_getpshared (barrier, pshared);
}
/* pthread_barrierattr_init */
int mpc_thread_barrierattr_init (sctk_thread_barrierattr_t
				 * barrier)
{
  return sctk_thread_barrierattr_init (barrier);
}

/* pthread_barrierattr_setpshared */
int
mpc_thread_barrierattr_setpshared (sctk_thread_barrierattr_t * barrier,
				   int pshared)
{
  return sctk_thread_barrierattr_setpshared (barrier, pshared);
}

/* pthread_cancel */
int mpc_thread_cancel (sctk_thread_t thread)
{
  return sctk_thread_cancel (thread);
}

/* pthread_cond_broadcast */
int mpc_thread_cond_broadcast (sctk_thread_cond_t * cond)
{
  return sctk_thread_cond_broadcast (cond);
}

/* pthread_cond_destroy */
int mpc_thread_cond_destroy (sctk_thread_cond_t * cond)
{
  return sctk_thread_cond_destroy (cond);
}

/* pthread_cond_init */
int mpc_thread_cond_init (sctk_thread_cond_t * cond,
			  const sctk_thread_condattr_t * attr)
{
  return sctk_thread_cond_init (cond, attr);
}

/* pthread_cond_signal */
int mpc_thread_cond_signal (sctk_thread_cond_t * cond)
{
  return sctk_thread_cond_signal (cond);
}


/* pthread_cond_timedwait */
int mpc_thread_cond_timedwait (sctk_thread_cond_t * cond,
			       sctk_thread_mutex_t *
			       mutex,
			       const struct timespec *time)
{
  return sctk_thread_cond_timedwait (cond, mutex, time);
}

/* pthread_cond_wait */
int mpc_thread_cond_wait (sctk_thread_cond_t * cond,
			  sctk_thread_mutex_t * mutex)
{
  return sctk_thread_cond_wait (cond, mutex);
}

/* pthread_condattr_destroy */
int mpc_thread_condattr_destroy (sctk_thread_condattr_t *
				 attr)
{
  return sctk_thread_condattr_destroy (attr);
}

/* pthread_condattr_getclock */
int mpc_thread_condattr_getclock (sctk_thread_condattr_t *
				  attr, clockid_t * clock)
{
  return sctk_thread_condattr_getclock (attr, clock);
}


/* pthread_condattr_getpshared */
int mpc_thread_condattr_getpshared (const
				    sctk_thread_condattr_t
				    * attr, int *pshared)
{
  return sctk_thread_condattr_getpshared (attr, pshared);
}


/* pthread_condattr_init */
int mpc_thread_condattr_init (sctk_thread_condattr_t * attr)
{
  return sctk_thread_condattr_init (attr);
}


/* pthread_condattr_setclock */
int mpc_thread_condattr_setclock (sctk_thread_condattr_t *
				  attr, clockid_t clock)
{
  return sctk_thread_condattr_setclock (attr, clock);
}


/* pthread_condattr_setpshared */
int mpc_thread_condattr_setpshared (sctk_thread_condattr_t
				    * attr, int pshared)
{
  return sctk_thread_condattr_setpshared (attr, pshared);
}


/* pthread_create */
int mpc_thread_create (sctk_thread_t * th,
		       const sctk_thread_attr_t * attr,
		       void *(*func) (void *), void *arg)
{
  return sctk_user_thread_create (th, attr, func, arg);
}


/* pthread_detach */
int mpc_thread_detach (sctk_thread_t th)
{
  return sctk_thread_detach (th);
}

/* pthread_equal */
int mpc_thread_equal (sctk_thread_t th1, sctk_thread_t th2)
{
  return sctk_thread_equal (th1, th2);
}


/* pthread_exit */
void mpc_thread_exit (void *result)
{
  sctk_thread_exit (result);
}


/* mpc_thread_getconcurrency */
int mpc_thread_getconcurrency (void)
{
  return sctk_thread_getconcurrency ();
}


/* pthread_getcpuclockid */
int mpc_thread_getcpuclockid (sctk_thread_t th,
			      clockid_t * clock)
{
  return sctk_thread_getcpuclockid (th, clock);
}


/* pthread_getschedparam */
int mpc_thread_getschedparam (sctk_thread_t th, int *a,
			      struct sched_param *b)
{
  return sctk_thread_getschedparam (th, a, b);
}


/* pthread_getspecific */
void *mpc_thread_getspecific (sctk_thread_key_t key)
{
  return sctk_thread_getspecific (key);
}


/* pthread_join */
int mpc_thread_join (sctk_thread_t th, void **ret)
{
  return sctk_thread_join (th, ret);
}

/* pthread_key_create */
int mpc_thread_key_create (sctk_thread_key_t * key,
			   void (*destr) (void *))
{
  return sctk_thread_key_create (key, destr);
}


/* pthread_key_delete */
int mpc_thread_key_delete (sctk_thread_key_t key)
{
  return sctk_thread_key_delete (key);
}


/* pthread_mutex_destroy */
int mpc_thread_mutex_destroy (sctk_thread_mutex_t * mutex)
{
  return sctk_thread_mutex_destroy (mutex);
}


/* pthread_mutex_getprioceiling */
int mpc_thread_mutex_getprioceiling (const
				     sctk_thread_mutex_t *
				     mutex, int *prio)
{
  return sctk_thread_mutex_getprioceiling (mutex, prio);
}


/* pthread_mutex_init */
int mpc_thread_mutex_init (sctk_thread_mutex_t * mutex,
			   const sctk_thread_mutexattr_t *
			   attr)
{
  return sctk_thread_mutex_init (mutex, attr);
}

#ifndef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
int user_sctk_thread_mutex_lock (sctk_thread_mutex_t * mutex);
int user_sctk_thread_mutex_unlock (sctk_thread_mutex_t * mutex);
#endif

/* pthread_mutex_lock */
int mpc_thread_mutex_lock (sctk_thread_mutex_t * mutex)
{
#ifdef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
  return sctk_thread_mutex_lock (mutex);
#else
  return user_sctk_thread_mutex_lock(mutex);
#endif
}


/* pthread_mutex_setprioceiling */
int mpc_thread_mutex_setprioceiling (sctk_thread_mutex_t *
				     mutex, int a, int *b)
{
  return sctk_thread_mutex_setprioceiling (mutex, a, b);
}


/* pthread_mutex_timedlock */
int mpc_thread_mutex_timedlock (sctk_thread_mutex_t *
				mutex,
				const struct timespec *time)
{
  return sctk_thread_mutex_timedlock (mutex, time);
}


/* pthread_mutex_trylock */
int mpc_thread_mutex_trylock (sctk_thread_mutex_t * mutex)
{
  return sctk_thread_mutex_trylock (mutex);
}


/* pthread_mutex_unlock */
int mpc_thread_mutex_unlock (sctk_thread_mutex_t * mutex)
{
#ifdef SCTK_DO_NOT_HAVE_WEAK_SYMBOLS
  return sctk_thread_mutex_unlock (mutex);
#else
  return user_sctk_thread_mutex_unlock (mutex);
#endif
}


/* pthread_mutexattr_destroy */
int mpc_thread_mutexattr_destroy (sctk_thread_mutexattr_t
				  * attr)
{
  return sctk_thread_mutexattr_destroy (attr);
}


/* pthread_mutexattr_getprioceiling */
int mpc_thread_mutexattr_getprioceiling (const
					 sctk_thread_mutexattr_t
					 * attr, int *prio)
{
  return sctk_thread_mutexattr_getprioceiling (attr, prio);
}


/* pthread_mutexattr_getprotocol */
int mpc_thread_mutexattr_getprotocol (const
				      sctk_thread_mutexattr_t
				      * attr, int *protocol)
{
  return sctk_thread_mutexattr_getprotocol (attr, protocol);
}


/* pthread_mutexattr_getpshared */
int mpc_thread_mutexattr_getpshared (const
				     sctk_thread_mutexattr_t
				     * attr, int *pshared)
{
  return sctk_thread_mutexattr_getpshared (attr, pshared);
}


/* pthread_mutexattr_gettype */
int mpc_thread_mutexattr_gettype (const
				  sctk_thread_mutexattr_t
				  * attr, int *type)
{
  return sctk_thread_mutexattr_gettype (attr, type);
}


/* pthread_mutexattr_init */
int mpc_thread_mutexattr_init (sctk_thread_mutexattr_t * attr)
{
  return sctk_thread_mutexattr_init (attr);
}


/* pthread_mutexattr_setprioceiling */
int
mpc_thread_mutexattr_setprioceiling (sctk_thread_mutexattr_t * attr,
				     int prio)
{
  return sctk_thread_mutexattr_setprioceiling (attr, prio);
}


/* pthread_mutexattr_setprotocol */
int
mpc_thread_mutexattr_setprotocol (sctk_thread_mutexattr_t * attr,
				  int protocol)
{
  return sctk_thread_mutexattr_setprotocol (attr, protocol);
}


/* pthread_mutexattr_setpshared */
int
mpc_thread_mutexattr_setpshared (sctk_thread_mutexattr_t * attr,
				 int pshared)
{
  return sctk_thread_mutexattr_setpshared (attr, pshared);
}


/* pthread_mutexattr_settype */
int mpc_thread_mutexattr_settype (sctk_thread_mutexattr_t
				  * attr, int type)
{
  return sctk_thread_mutexattr_settype (attr, type);
}


/* pthread_once */
int mpc_thread_once (sctk_thread_once_t * once,
		     void (*func) (void))
{
  return sctk_thread_once (once, func);
}


/* pthread_rwlock_destroy */
int mpc_thread_rwlock_destroy (sctk_thread_rwlock_t * lock)
{
  return sctk_thread_rwlock_destroy (lock);
}


/* pthread_rwlock_init */
int mpc_thread_rwlock_init (sctk_thread_rwlock_t * lock,
			    const sctk_thread_rwlockattr_t
			    * attr)
{
  return sctk_thread_rwlock_init (lock, attr);
}


/* pthread_rwlock_rdlock */
int mpc_thread_rwlock_rdlock (sctk_thread_rwlock_t * lock)
{
  return sctk_thread_rwlock_rdlock (lock);
}


/* pthread_rwlock_timedrdlock */
int mpc_thread_rwlock_timedrdlock (sctk_thread_rwlock_t *
				   lock,
				   const struct timespec
				   *time)
{
  return sctk_thread_rwlock_timedrdlock (lock, time);
}


/* pthread_rwlock_timedwrlock */
int mpc_thread_rwlock_timedwrlock (sctk_thread_rwlock_t *
				   lock,
				   const struct timespec
				   *time)
{
  return sctk_thread_rwlock_timedwrlock (lock, time);
}


/* pthread_rwlock_tryrdlock */
int mpc_thread_rwlock_tryrdlock (sctk_thread_rwlock_t * lock)
{
  return sctk_thread_rwlock_tryrdlock (lock);
}


/* pthread_rwlock_trywrlock */
int mpc_thread_rwlock_trywrlock (sctk_thread_rwlock_t * lock)
{
  return sctk_thread_rwlock_trywrlock (lock);
}


/* pthread_rwlock_unlock */
int mpc_thread_rwlock_unlock (sctk_thread_rwlock_t * lock)
{
  return sctk_thread_rwlock_unlock (lock);
}


/* pthread_rwlock_wrlock */
int mpc_thread_rwlock_wrlock (sctk_thread_rwlock_t * lock)
{
  return sctk_thread_rwlock_wrlock (lock);
}


/* pthread_rwlockattr_destroy */
int
mpc_thread_rwlockattr_destroy (sctk_thread_rwlockattr_t * attr)
{
  return sctk_thread_rwlockattr_destroy (attr);
}


/* pthread_rwlockattr_getpshared */
int mpc_thread_rwlockattr_getpshared (const
				      sctk_thread_rwlockattr_t
				      * attr, int *pshared)
{
  return sctk_thread_rwlockattr_getpshared (attr, pshared);
}


/* pthread_rwlockattr_init */
int mpc_thread_rwlockattr_init (sctk_thread_rwlockattr_t *
				attr)
{
  return sctk_thread_rwlockattr_init (attr);
}


/* pthread_rwlockattr_setpshared */
int
mpc_thread_rwlockattr_setpshared (sctk_thread_rwlockattr_t * attr,
				  int pshared)
{
  return sctk_thread_rwlockattr_setpshared (attr, pshared);
}


/* pthread_self */
sctk_thread_t mpc_thread_self (void)
{
  return sctk_thread_self ();
}


/* pthread_setcancelstate */
int mpc_thread_setcancelstate (int newarg, int *old)
{
  return sctk_thread_setcancelstate (newarg, old);
}


/* pthread_setcanceltype */
int mpc_thread_setcanceltype (int newarg, int *old)
{
  return sctk_thread_setcanceltype (newarg, old);
}


/* pthread_setconcurrency */
int mpc_thread_setconcurrency (int concurrency)
{
  return sctk_thread_setconcurrency (concurrency);
}


/* pthread_setschedparam */
int mpc_thread_setschedparam (sctk_thread_t th, int a,
			      const struct sched_param *b)
{
  return sctk_thread_setschedparam (th, a, b);
}


/* pthread_setschedprio */
int mpc_thread_setschedprio (sctk_thread_t th, int prio)
{
  return sctk_thread_setschedprio (th, prio);
}


/* pthread_setspecific */
int mpc_thread_setspecific (sctk_thread_key_t key,
			    const void *value)
{
  return sctk_thread_setspecific (key, value);
}


/* pthread_spin_destroy */
int mpc_thread_spin_destroy (sctk_thread_spinlock_t * spin)
{
  return sctk_thread_spin_destroy (spin);
}


/* pthread_spin_init */
int mpc_thread_spin_init (sctk_thread_spinlock_t * spin,
			  int a)
{
  return sctk_thread_spin_init (spin, a);
}


/* pthread_spin_lock */
int mpc_thread_spin_lock (sctk_thread_spinlock_t * spin)
{
  return sctk_thread_spin_lock (spin);
}


/* pthread_spin_trylock */
int mpc_thread_spin_trylock (sctk_thread_spinlock_t * spin)
{
  return sctk_thread_spin_trylock (spin);
}


/* pthread_spin_unlock */
int mpc_thread_spin_unlock (sctk_thread_spinlock_t * spin)
{
  return sctk_thread_spin_unlock (spin);
}


/* pthread_testcancel */
void mpc_thread_testcancel (void)
{
  sctk_thread_testcancel ();
}


/* pthread_yield */
int mpc_thread_yield (void)
{
  return sctk_thread_yield ();
}

int mpc_thread_kill (sctk_thread_t thread, int signo)
{
  return sctk_thread_kill (thread, signo);
}


int mpc_thread_sigmask (int how, const sigset_t * newmask,
			sigset_t * oldmask)
{
  return sctk_thread_sigmask (how, newmask, oldmask);
}

int mpc_thread_getattr_np (sctk_thread_t th,
			   sctk_thread_attr_t * attr)
{
  return sctk_thread_getattr_np (th, attr);
}


/*semaphore.h*/
/* sem_init */
int mpc_thread_sem_init (sctk_thread_sem_t * sem,
			 int pshared, unsigned int value)
{
  return sctk_thread_sem_init (sem, pshared, value);
}


/* sem_ wait*/
int mpc_thread_sem_wait (sctk_thread_sem_t * sem)
{
  return sctk_thread_sem_wait (sem);
}


/* sem_trywait */
int mpc_thread_sem_trywait (sctk_thread_sem_t * sem)
{
  return sctk_thread_sem_trywait (sem);
}


/* sem_post */
int mpc_thread_sem_post (sctk_thread_sem_t * sem)
{
  return sctk_thread_sem_post (sem);
}


/* sem_getvalue */
int mpc_thread_sem_getvalue (sctk_thread_sem_t * sem,
			     int *sval)
{
  return sctk_thread_sem_getvalue (sem, sval);
}


/* sem_destroy */
int mpc_thread_sem_destroy (sctk_thread_sem_t * sem)
{
  return sctk_thread_sem_destroy (sem);
}


/* sem_open */
sctk_thread_sem_t *mpc_thread_sem_open (const char *name,
					int oflag, ...)
{
  if ((oflag & SCTK_O_CREAT))
    {

      va_list ap;
      int value;
      mode_t mode;
      va_start (ap, oflag);
      mode = va_arg (ap, mode_t);
      value = va_arg (ap, int);
      va_end (ap);
      return sctk_thread_sem_open (name, oflag,mode,value);
    }
  else
    {
      return sctk_thread_sem_open (name, oflag);
    }
}


/* sem_close */
int mpc_thread_sem_close (sctk_thread_sem_t * sem)
{
  return sctk_thread_sem_close (sem);
}


/* sem_unlink */
int mpc_thread_sem_unlink (const char *__name)
{
  return sctk_thread_sem_unlink (__name);
}


/* sem_timedwait */
int mpc_thread_sem_timedwait (sctk_thread_sem_t * __sem,
			      const struct timespec
			      *__abstime)
{
  return sctk_thread_sem_timedwait (__sem, __abstime);
}

unsigned long mpc_thread_atomic_add (volatile unsigned long
				     *ptr, unsigned long val)
{
  return sctk_thread_atomic_add (ptr, val);
}

unsigned long mpc_thread_tls_entry_add(unsigned long size,
                                       void (*func)(void *)) {
  return sctk_tls_entry_add(size, func);
}
/* At exit */

int mpc_atexit(void (*function)(void))
{
	return sctk_atexit(function); 
	
}

/* Futexes */

long  mpc_thread_futex(__UNUSED__ int sysop, void *addr1, int op, int val1, 
					  struct timespec *timeout, void *addr2, int val2)
{
	sctk_futex_context_init();
	return sctk_thread_futex(addr1, op, val1, timeout, addr2, val2);
}

long mpc_thread_futex_with_vaargs(int sysop, ...)
{
    va_list ap;
    void* addr1, *addr2;
    int i, op, val1, val2;
    struct timespec *timeout;

    va_start(ap, sysop);
    addr1 = va_arg(ap, void*);
    op = va_arg(ap, int);
    val1 = va_arg(ap, int);
    timeout = va_arg(ap, struct timespec *);
    addr2 = va_arg(ap, void*);
    val2 = va_arg(ap, int);
    va_end(ap);

    return mpc_thread_futex(sysop, addr1, op, val1, timeout, addr2, val2);
}
