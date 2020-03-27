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
#include "sctk_posix_ethread.h"
#include "sctk_posix_ethread_internal.h"
#include "sctk_internal_thread.h"
#include "sctk_posix_ethread_np.h"

static sctk_ethread_sem_head_list __sctk_head_sem = SCTK_SEM_HEAD_INITIALIZER;


static int
sctk_ethread_once(mpc_thread_once_t *once_control,
                  void (*init_routine)(void) )
{
	return __sctk_ethread_once(once_control, init_routine);
}

static int
sctk_ethread_attr_setdetachstate(sctk_ethread_attr_t *attr, int detachstate)
{
	return __sctk_ethread_attr_setdetachstate(attr, detachstate);
}

static int
sctk_ethread_attr_getdetachstate(const sctk_ethread_attr_t *attr,
                                 int *detachstate)
{
	return __sctk_ethread_attr_getdetachstate(attr, detachstate);
}

static int
sctk_ethread_attr_setschedpolicy(sctk_ethread_attr_t *attr, int policy)
{
	return __sctk_ethread_attr_setschedpolicy(attr, policy);
}

static int
sctk_ethread_attr_getschedpolicy(const sctk_ethread_attr_t *attr,
                                 int *policy)
{
	return __sctk_ethread_attr_getschedpolicy(attr, policy);
}

static int
sctk_ethread_attr_setinheritsched(sctk_ethread_attr_t *attr, int inherit)
{
	return __sctk_ethread_attr_setinheritsched(attr, inherit);
}

static int
sctk_ethread_attr_getinheritsched(const sctk_ethread_attr_t *attr,
                                  int *inherit)
{
	return __sctk_ethread_attr_getinheritsched(attr, inherit);
}

static int
sctk_ethread_attr_getscope(const sctk_ethread_attr_t *attr, int *scope)
{
	return __sctk_ethread_attr_getscope(attr, scope);
}

static int
sctk_ethread_attr_setscope(sctk_ethread_attr_t *attr, int scope)
{
	return __sctk_ethread_attr_setscope2(attr, scope);
}

static int
sctk_ethread_attr_getstacksize(const sctk_ethread_attr_t *attr,
                               size_t *stacksize)
{
	return __sctk_ethread_attr_getstacksize(attr, stacksize);
}

static int
sctk_ethread_attr_getstackaddr(const sctk_ethread_attr_t *attr, void **addr)
{
	return __sctk_ethread_attr_getstackaddr(attr, addr);
}

static int
sctk_ethread_attr_setstacksize(sctk_ethread_attr_t *attr, size_t stacksize)
{
	return __sctk_ethread_attr_setstacksize(attr, stacksize);
}

static int
sctk_ethread_attr_setstackaddr(sctk_ethread_attr_t *attr, void *addr)
{
	return __sctk_ethread_attr_setstackaddr(attr, addr);
}

static int
sctk_ethread_attr_setstack(sctk_ethread_attr_t *attr, void *addr,
                           size_t stacksize)
{
	return __sctk_ethread_attr_setstack(attr, addr, stacksize);
}

static int
sctk_ethread_attr_getstack(sctk_ethread_attr_t *attr, void **addr,
                           size_t *stacksize)
{
	return __sctk_ethread_attr_getstack(attr, addr, stacksize);
}

static int
sctk_ethread_attr_setguardsize(sctk_ethread_attr_t *attr,
                               size_t guardsize)
{
	return __sctk_ethread_attr_setguardsize(attr, guardsize);
}

static int
sctk_ethread_attr_getguardsize(sctk_ethread_attr_t *attr,
                               size_t *guardsize)
{
	return __sctk_ethread_attr_getguardsize(attr, guardsize);
}

static int
sctk_ethread_attr_destroy(sctk_ethread_attr_t *attr)
{
	int ret;

	ret       = __sctk_ethread_attr_destroy(attr->ptr);
	attr->ptr = NULL;
	return ret;
}

static void
sctk_ethread_testcancel(void)
{
	sctk_ethread_per_thread_t *current;

	current = sctk_ethread_self();
	__sctk_ethread_testcancel(current);
}

static int
sctk_ethread_cancel(sctk_ethread_t target)
{
	return __sctk_ethread_cancel(target);
}

static int
sctk_ethread_setcancelstate(int state, int *oldstate)
{
	sctk_ethread_per_thread_t *current;

	current = sctk_ethread_self();
	return __sctk_ethread_setcancelstate(current, state, oldstate);
}

static int
sctk_ethread_setcanceltype(int type, int *oldtype)
{
	sctk_ethread_per_thread_t *current;

	current = sctk_ethread_self();
	return __sctk_ethread_setcanceltype(current, type, oldtype);
}

static int
sctk_ethread_sem_init(sctk_ethread_sem_t *lock,
                      int pshared, unsigned int value)
{
	return __sctk_ethread_sem_init(lock, pshared, value);
}

static int
sctk_ethread_sem_wait(sctk_ethread_sem_t *lock)
{
	sctk_ethread_per_thread_t *       current;
	sctk_ethread_virtual_processor_t *current_vp;

	current    = sctk_ethread_self();
	current_vp = (sctk_ethread_virtual_processor_t *)current->vp;

	return __sctk_ethread_sem_wait(current_vp, current, lock);
}

static int
sctk_ethread_sem_trywait(sctk_ethread_sem_t *lock)
{
	return __sctk_ethread_sem_trywait(lock);
}

static int
sctk_ethread_sem_post(sctk_ethread_sem_t *lock)
{
	return __sctk_ethread_sem_post(sctk_ethread_return_task, lock);
}

static int
sctk_ethread_sem_getvalue(sctk_ethread_sem_t *sem, int *sval)
{
	return __sctk_ethread_sem_getvalue(sem, sval);
}

static int
sctk_ethread_sem_destroy(sctk_ethread_sem_t *sem)
{
	return __sctk_ethread_sem_destroy(sem);
}

static sctk_ethread_sem_t *
sctk_ethread_sem_open(const char *name, int oflag, ...)
{
	int value = 0;
	int mode  = 0;

	if(oflag & O_CREAT)
	{
		va_list ap;
		va_start(ap, oflag);
		mode  = (int)va_arg(ap, mode_t);
		value = va_arg(ap, int);
		va_end(ap);
	}
	return __sctk_ethread_sem_open(name, oflag, &__sctk_head_sem, mode, value);
}

static int
sctk_ethread_sem_close(sctk_ethread_sem_t *sem)
{
	return __sctk_ethread_sem_close(sem, &__sctk_head_sem);
}

static int
sctk_ethread_sem_unlink(const char *name)
{
	return __sctk_ethread_sem_unlink(name, &__sctk_head_sem);
}

static int
sctk_ethread_detach(sctk_ethread_t th)
{
	return __sctk_ethread_detach(th);
}

/*sched priority*/
static int
sctk_ethread_sched_get_priority_max(int policy)
{
	return __sctk_ethread_sched_get_priority_max(policy);
}

static int
sctk_ethread_sched_get_priority_min(int policy)
{
	return __sctk_ethread_sched_get_priority_min(policy);
}

/*mutex attr*/
static int
sctk_ethread_mutexattr_init(sctk_ethread_mutexattr_t *attr)
{
	return __sctk_ethread_mutexattr_init(attr);
}

static int
sctk_ethread_mutexattr_destroy(sctk_ethread_mutexattr_t *attr)
{
	return __sctk_ethread_mutexattr_destroy(attr);
}

static int
sctk_ethread_mutexattr_gettype(const sctk_ethread_mutexattr_t *attr,
                               int *type)
{
	return __sctk_ethread_mutexattr_gettype(attr, type);
}

static int
sctk_ethread_mutexattr_settype(sctk_ethread_mutexattr_t *attr, int type)
{
	return __sctk_ethread_mutexattr_settype(attr, (unsigned char)type);
}

static int
sctk_ethread_mutexattr_getpshared(const sctk_ethread_mutexattr_t *attr,
                                  int *type)
{
	return __sctk_ethread_mutexattr_getpshared(attr, type);
}

static int
sctk_ethread_mutexattr_setpshared(sctk_ethread_mutexattr_t *attr, int type)
{
	return __sctk_ethread_mutexattr_setpshared(attr, type);
}

static int
sctk_ethread_mutex_destroy(sctk_ethread_mutex_t *attr)
{
	return __sctk_ethread_mutex_destroy(attr);
}

/*conditions*/
static int
sctk_ethread_cond_init(sctk_ethread_cond_t *cond,
                       const sctk_ethread_condattr_t *attr)
{
	return __sctk_ethread_cond_init(cond, attr);
}

static int
sctk_ethread_cond_destroy(sctk_ethread_cond_t *cond)
{
	return __sctk_etheread_cond_destroy(cond);
}

static int
sctk_ethread_cond_wait(sctk_ethread_cond_t *cond,
                       sctk_ethread_mutex_t *mutex)
{
	sctk_ethread_per_thread_t *       current;
	sctk_ethread_virtual_processor_t *current_vp;

	current    = sctk_ethread_self();
	current_vp = (sctk_ethread_virtual_processor_t *)current->vp;
	return __sctk_ethread_cond_wait(cond, current_vp, current,
	                                sctk_ethread_return_task, mutex);
}

static int
sctk_ethread_cond_timedwait(sctk_ethread_cond_t *cond,
                            sctk_ethread_mutex_t *mutex,
                            const struct timespec *abstime)
{
	sctk_ethread_per_thread_t *       current;
	sctk_ethread_virtual_processor_t *current_vp;

	current    = sctk_ethread_self();
	current_vp = (sctk_ethread_virtual_processor_t *)current->vp;
	return __sctk_ethread_cond_timedwait(cond, current_vp, current,
	                                     sctk_ethread_return_task, mutex,
	                                     abstime);
}

static int
sctk_ethread_cond_broadcast(sctk_ethread_cond_t *cond)
{
	return __sctk_ethread_cond_broadcast(cond, sctk_ethread_return_task);
}

static int
sctk_ethread_cond_signal(sctk_ethread_cond_t *cond)
{
	return __sctk_ethread_cond_signal(cond, sctk_ethread_return_task);
}

/*attributs des conditions*/
static int
sctk_ethread_condattr_destroy(sctk_ethread_condattr_t *attr)
{
	return __sctk_ethread_condattr_destroy(attr);
}

static int
sctk_ethread_condattr_init(sctk_ethread_condattr_t *attr)
{
	return __sctk_ethread_condattr_init(attr);
}

static int
sctk_ethread_condattr_getpshared(const sctk_ethread_condattr_t *attr,
                                 int *pshared)
{
	return __sctk_ethread_condattr_getpshared(attr, pshared);
}

static int
sctk_ethread_condattr_setpshared(sctk_ethread_condattr_t *attr, int pshared)
{
	return __sctk_ethread_condattr_setpshared(attr, pshared);
}

/*spinlock*/
static int
sctk_ethread_spin_init(sctk_ethread_spinlock_t *lock, int pshared)
{
	return __sctk_ethread_spin_init(lock, pshared);
}

static int
sctk_ethread_spin_destroy(sctk_ethread_spinlock_t *lock)
{
	return __sctk_ethread_spin_destroy(lock);
}

static int
sctk_ethread_spin_lock(sctk_ethread_spinlock_t *lock)
{
	return __sctk_ethread_spin_lock(lock);
}

static int
sctk_ethread_spin_trylock(sctk_ethread_spinlock_t *lock)
{
	return __sctk_ethread_spin_trylock(lock);
}

static int
sctk_ethread_spin_unlock(sctk_ethread_spinlock_t *lock)
{
	return __sctk_ethread_spin_unlock(lock);
}

/*rwlock*/
static int
sctk_ethread_rwlock_init(sctk_ethread_rwlock_t *rwlock,
                         const sctk_ethread_rwlockattr_t *attr)
{
	return __sctk_ethread_rwlock_init(rwlock, attr);
}

static int
sctk_ethread_rwlock_destroy(sctk_ethread_rwlock_t *rwlock)
{
	return __sctk_ethread_rwlock_destroy(rwlock);
}

static int
sctk_ethread_rwlock_rdlock(sctk_ethread_rwlock_t *rwlock)
{
	sctk_ethread_per_thread_t *       current;
	sctk_ethread_virtual_processor_t *current_vp;

	current    = sctk_ethread_self();
	current_vp = (sctk_ethread_virtual_processor_t *)current->vp;
	return __sctk_ethread_rwlock_lock(rwlock, SCTK_RWLOCK_READ,
	                                  current_vp, current);
}

static int
sctk_ethread_rwlock_tryrdlock(sctk_ethread_rwlock_t *rwlock)
{
	return __sctk_ethread_rwlock_trylock(rwlock, SCTK_RWLOCK_READ);
}

static int
sctk_ethread_rwlock_wrlock(sctk_ethread_rwlock_t *rwlock)
{
	sctk_ethread_per_thread_t *       current;
	sctk_ethread_virtual_processor_t *current_vp;

	current    = sctk_ethread_self();
	current_vp = (sctk_ethread_virtual_processor_t *)current->vp;
	return __sctk_ethread_rwlock_lock(rwlock, SCTK_RWLOCK_WRITE,
	                                  current_vp, current);
}

static int
sctk_ethread_rwlock_trywrlock(sctk_ethread_rwlock_t *rwlock)
{
	return __sctk_ethread_rwlock_trylock(rwlock, SCTK_RWLOCK_WRITE);
}

static int
sctk_ethread_rwlock_unlock(sctk_ethread_rwlock_t *rwlock)
{
	return __sctk_ethread_rwlock_unlock(sctk_ethread_return_task, rwlock);
}

/*rwlockattr*/
static int
sctk_ethread_rwlockattr_getpshared(const sctk_ethread_rwlockattr_t *attr,
                                   int *type)
{
	return __sctk_ethread_rwlockattr_getpshared(attr, type);
}

static int
sctk_ethread_rwlockattr_setpshared(sctk_ethread_rwlockattr_t *attr,
                                   int type)
{
	return __sctk_ethread_rwlockattr_setpshared(attr, type);
}

static int
sctk_ethread_rwlockattr_init(sctk_ethread_rwlockattr_t *attr)
{
	return __sctk_ethread_rwlockattr_init(attr);
}

static int
sctk_ethread_rwlockattr_destroy(sctk_ethread_rwlockattr_t *attr)
{
	return __sctk_ethread_rwlockattr_destroy(attr);
}

/*barrier*/
static int
sctk_ethread_barrier_init(sctk_ethread_barrier_t *barrier,
                          const sctk_ethread_barrierattr_t *attr,
                          unsigned count)
{
	return __sctk_ethread_barrier_init(barrier, attr, count);
}

static int
sctk_ethread_barrier_destroy(sctk_ethread_barrier_t *barrier)
{
	return __sctk_ethread_barrier_destroy(barrier);
}

static int
sctk_ethread_barrier_wait(sctk_ethread_barrier_t *barrier)
{
	sctk_ethread_per_thread_t *       current;
	sctk_ethread_virtual_processor_t *current_vp;

	current    = sctk_ethread_self();
	current_vp = (sctk_ethread_virtual_processor_t *)current->vp;
	return __sctk_ethread_barrier_wait(barrier, current_vp, current,
	                                   sctk_ethread_return_task);
}

/*barrierattr*/
static int
sctk_ethread_barrierattr_getpshared(const sctk_ethread_barrierattr_t *
                                    attr, int *type)
{
	return __sctk_ethread_barrierattr_getpshared(attr, type);
}

static int
sctk_ethread_barrierattr_setpshared(sctk_ethread_barrierattr_t *attr,
                                    int type)
{
	return __sctk_ethread_barrierattr_setpshared(attr, type);
}

static int
sctk_ethread_barrierattr_init(sctk_ethread_barrierattr_t *attr)
{
	return __sctk_ethread_barrierattr_init(attr);
}

static int
sctk_ethread_barrierattr_destroy(sctk_ethread_barrierattr_t *attr)
{
	return __sctk_ethread_barrierattr_destroy(attr);
}

/*non portable*/
static int
sctk_ethread_getattr_np(sctk_ethread_t th, sctk_ethread_attr_t *attr)
{
	int ret;

	attr->ptr = (sctk_ethread_attr_intern_t *)
	            sctk_malloc(sizeof(sctk_ethread_attr_intern_t) );
	ret = __sctk_ethread_getattr_np(th, attr->ptr);
	sctk_nodebug("sctk_getattr :%p  %d", attr->ptr, ret);
	return ret;
}

void
sctk_posix_ethread()
{
	sctk_ethread_check_size(sctk_ethread_t, mpc_thread_t);
	sctk_ethread_check_size(mpc_thread_once_t, mpc_thread_once_t);
	sctk_ethread_check_size(sctk_ethread_attr_t, mpc_thread_attr_t);
	sctk_ethread_check_size(sctk_ethread_sem_t, mpc_thread_sem_t);
	sctk_ethread_check_size(sctk_ethread_cond_t, mpc_thread_cond_t);
	sctk_ethread_check_size(sctk_ethread_spinlock_t, mpc_thread_spinlock_t);
	sctk_ethread_check_size(sctk_ethread_rwlock_t, mpc_thread_rwlock_t);
	sctk_ethread_check_size(sctk_ethread_rwlockattr_t,
	                        mpc_thread_rwlockattr_t);
	sctk_ethread_check_size(sctk_ethread_condattr_t, mpc_thread_condattr_t);
	sctk_ethread_check_size(sctk_ethread_barrier_t, mpc_thread_barrier_t);
	sctk_ethread_check_size(sctk_ethread_barrierattr_t,
	                        mpc_thread_barrierattr_t);
	{
		sctk_ethread_cond_t loc  = SCTK_ETHREAD_COND_INIT;
		mpc_thread_cond_t  glob = SCTK_THREAD_COND_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(sctk_ethread_cond_t) ) == 0);
	}


	/*le pthread once */
	sctk_add_func_type(sctk_ethread, once,
	                   int (*)(mpc_thread_once_t *, void (*)(void) ) );

	/*les attributs des threads */
	sctk_add_func_type(sctk_ethread, attr_setdetachstate,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(sctk_ethread, attr_getdetachstate,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(sctk_ethread, attr_setschedpolicy,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(sctk_ethread, attr_getschedpolicy,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(sctk_ethread, attr_setinheritsched,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(sctk_ethread, attr_getinheritsched,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(sctk_ethread, attr_getscope,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(sctk_ethread, attr_setscope,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(sctk_ethread, attr_getstackaddr,
	                   int (*)(const mpc_thread_attr_t *, void **) );
	sctk_add_func_type(sctk_ethread, attr_setstackaddr,
	                   int (*)(mpc_thread_attr_t *, void *) );
	sctk_add_func_type(sctk_ethread, attr_getstacksize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );
	sctk_add_func_type(sctk_ethread, attr_setstacksize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
	sctk_add_func_type(sctk_ethread, attr_getstack,
	                   int (*)(const mpc_thread_attr_t *, void **, size_t *) );
	sctk_add_func_type(sctk_ethread, attr_setstack,
	                   int (*)(mpc_thread_attr_t *, void *, size_t) );
	sctk_add_func_type(sctk_ethread, attr_setguardsize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
	sctk_add_func_type(sctk_ethread, attr_getguardsize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );


	/*gestion du cancel */
	sctk_add_func_type(sctk_ethread, testcancel, void (*)(void) );
	sctk_add_func_type(sctk_ethread, cancel, int (*)(mpc_thread_t) );
	sctk_add_func_type(sctk_ethread, setcancelstate, int (*)(int, int *) );
	sctk_add_func_type(sctk_ethread, setcanceltype, int (*)(int, int *) );

	/*pthread_detach */
	sctk_add_func_type(sctk_ethread, detach, int (*)(mpc_thread_t) );

	/*les sémaphores */
	sctk_add_func_type(sctk_ethread, sem_init,
	                   int (*)(mpc_thread_sem_t *lock,
	                           int pshared, unsigned int value) );
	sctk_add_func_type(sctk_ethread, sem_wait, int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(sctk_ethread, sem_trywait,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(sctk_ethread, sem_post, int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(sctk_ethread, sem_getvalue,
	                   int (*)(mpc_thread_sem_t *, int *) );
	sctk_add_func_type(sctk_ethread, sem_destroy,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(sctk_ethread, sem_open,
	                   mpc_thread_sem_t * (*)(const char *, int, ...) );
	sctk_add_func_type(sctk_ethread, sem_close, int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(sctk_ethread, sem_unlink, int (*)(const char *) );



	/*sched prio */
	sctk_add_func_type(sctk_ethread, sched_get_priority_min, int (*)(int) );
	sctk_add_func_type(sctk_ethread, sched_get_priority_max, int (*)(int) );


	/*mutex attr */
	sctk_add_func_type(sctk_ethread, mutexattr_init,
	                   int (*)(mpc_thread_mutexattr_t *) );
	sctk_add_func_type(sctk_ethread, mutexattr_destroy,
	                   int (*)(mpc_thread_mutexattr_t *) );

	sctk_add_func_type(sctk_ethread, mutexattr_settype,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
	sctk_add_func_type(sctk_ethread, mutexattr_gettype,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
	sctk_add_func_type(sctk_ethread, mutexattr_setpshared,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
	sctk_add_func_type(sctk_ethread, mutexattr_getpshared,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
	sctk_add_func_type(sctk_ethread, mutex_destroy,
	                   int (*)(mpc_thread_mutex_t *) );

/*conditions*/
	sctk_add_func_type(sctk_ethread, cond_init,
	                   int (*)(mpc_thread_cond_t *,
	                           const mpc_thread_condattr_t *) );
	sctk_add_func_type(sctk_ethread, cond_destroy,
	                   int (*)(mpc_thread_cond_t *) );
	sctk_add_func_type(sctk_ethread, cond_wait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *) );
	sctk_add_func_type(sctk_ethread, cond_timedwait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *,
	                           const struct timespec *) );
	sctk_add_func_type(sctk_ethread, cond_broadcast,
	                   int (*)(mpc_thread_cond_t *) );
	sctk_add_func_type(sctk_ethread, cond_signal,
	                   int (*)(mpc_thread_cond_t *) );

	/*attributs des conditions */
	sctk_add_func_type(sctk_ethread, condattr_init,
	                   int (*)(mpc_thread_condattr_t *) );
	sctk_add_func_type(sctk_ethread, condattr_destroy,
	                   int (*)(mpc_thread_condattr_t *) );
	sctk_add_func_type(sctk_ethread, condattr_getpshared,
	                   int (*)(const mpc_thread_condattr_t *, int *) );
	sctk_add_func_type(sctk_ethread, condattr_setpshared,
	                   int (*)(mpc_thread_condattr_t *, int) );
	sctk_add_func_type(sctk_ethread, attr_destroy,
	                   int (*)(mpc_thread_attr_t *) );

	/*spinlock */
	sctk_add_func_type(sctk_ethread, spin_init,
	                   int (*)(mpc_thread_spinlock_t *, int) );
	sctk_add_func_type(sctk_ethread, spin_destroy,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(sctk_ethread, spin_lock,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(sctk_ethread, spin_trylock,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(sctk_ethread, spin_unlock,
	                   int (*)(mpc_thread_spinlock_t *) );

	/*rwlock */
	sctk_add_func_type(sctk_ethread, rwlock_init,
	                   int (*)(mpc_thread_rwlock_t *,
	                           const mpc_thread_rwlockattr_t *) );
	sctk_add_func_type(sctk_ethread, rwlock_rdlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(sctk_ethread, rwlock_tryrdlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(sctk_ethread, rwlock_wrlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(sctk_ethread, rwlock_trywrlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(sctk_ethread, rwlock_unlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(sctk_ethread, rwlock_destroy,
	                   int (*)(mpc_thread_rwlock_t *) );

	/*rwlock attr */
	sctk_add_func_type(sctk_ethread, rwlockattr_init,
	                   int (*)(mpc_thread_rwlockattr_t *) );
	sctk_add_func_type(sctk_ethread, rwlockattr_destroy,
	                   int (*)(mpc_thread_rwlockattr_t *) );
	sctk_add_func_type(sctk_ethread, rwlockattr_getpshared,
	                   int (*)(const mpc_thread_rwlockattr_t *, int *) );
	sctk_add_func_type(sctk_ethread, rwlockattr_setpshared,
	                   int (*)(mpc_thread_rwlockattr_t *, int) );

	/*barrier */
	sctk_add_func_type(sctk_ethread, barrier_init,
	                   int (*)(mpc_thread_barrier_t *,
	                           const mpc_thread_barrierattr_t *, unsigned) );
	sctk_add_func_type(sctk_ethread, barrier_wait,
	                   int (*)(mpc_thread_barrier_t *) );
	sctk_add_func_type(sctk_ethread, barrier_destroy,
	                   int (*)(mpc_thread_barrier_t *) );
	/*barrier attr */
	sctk_add_func_type(sctk_ethread, barrierattr_init,
	                   int (*)(mpc_thread_barrierattr_t *) );
	sctk_add_func_type(sctk_ethread, barrierattr_destroy,
	                   int (*)(mpc_thread_barrierattr_t *) );
	sctk_add_func_type(sctk_ethread, barrierattr_getpshared,
	                   int (*)(const mpc_thread_barrierattr_t *, int *) );
	sctk_add_func_type(sctk_ethread, barrierattr_setpshared,
	                   int (*)(mpc_thread_barrierattr_t *, int) );
	/*non portable */
	sctk_add_func_type(sctk_ethread, getattr_np,
	                   int (*)(mpc_thread_t, mpc_thread_attr_t *) );
}
