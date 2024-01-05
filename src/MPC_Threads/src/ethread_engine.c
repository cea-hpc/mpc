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
#define _GNU_SOURCE
#include <sched.h>

#include "mpc_common_debug.h"
#include "ethread_engine.h"
#include "ethread_engine_internal.h"
#include "mpc_thread.h"
#include "sctk_alloc.h"
#include "thread_ptr.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mpc_topology.h"

#include "ethread_posix.h"

/*Key data structures shared with ethread*/
int _mpc_thread_ethread_key_pos = 0;
stck_ethread_key_destr_function_t
        _mpc_thread_ethread_destr_func_tab[SCTK_THREAD_KEYS_MAX];
_mpc_thread_ethread_per_thread_t _mpc_thread_ethread_main_thread;

_mpc_thread_ethread_virtual_processor_t virtual_processor = SCTK_ETHREAD_VP_INIT;

/*Thread management*/
#ifdef SCTK_KERNEL_THREAD_USE_TLS
__thread void *_mpc_thread_ethread_key;
#else
mpc_thread_keys_t _mpc_thread_ethread_key;
#endif

inline _mpc_thread_ethread_t _mpc_thread_ethread_self()
{
	_mpc_thread_ethread_virtual_processor_t *vp = NULL;
	_mpc_thread_ethread_per_thread_t *       task = NULL;

	vp   = _mpc_thread_kthread_getspecific(_mpc_thread_ethread_key);
	task = (_mpc_thread_ethread_t)vp->current;
	return task;
}

static inline void _mpc_thread_ethread_self_all(_mpc_thread_ethread_virtual_processor_t **vp,
                                                _mpc_thread_ethread_per_thread_t **task)
{
	*vp   = _mpc_thread_kthread_getspecific(_mpc_thread_ethread_key);
	*task = (_mpc_thread_ethread_t)(*vp)->current;
	assert( (*task)->vp == *vp);
}

static inline void _mpc_thread_ethread_place_task_on_vp(_mpc_thread_ethread_virtual_processor_t *vp,
                                                        _mpc_thread_ethread_per_thread_t *task)
{
	task->vp = vp;
	mpc_common_nodebug("Place %p on %d", task, vp->rank);
	mpc_common_spinlock_lock(&vp->spinlock);
	task->status = ethread_ready;
	___mpc_thread_ethread_enqueue_task(task,
	                                   (_mpc_thread_ethread_per_thread_t **)&(vp->
	                                                                          incomming_queue),
	                                   (_mpc_thread_ethread_per_thread_t **)&(vp->
	                                                                          incomming_queue_tail) );
	mpc_common_spinlock_unlock(&vp->spinlock);
}

void _mpc_thread_ethread_return_task(_mpc_thread_ethread_per_thread_t *task)
{
	if(task->status == ethread_blocked)
	{
		task->status = ethread_ready;
		_mpc_thread_ethread_place_task_on_vp
		        ( (_mpc_thread_ethread_virtual_processor_t *)task->vp, task);
	}
	else
	{
		mpc_common_nodebug("status %d %p", task->status, task);
	}
}

static int _mpc_thread_ethread_sched_yield()
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_sched_yield_vp(current_vp, current);
}

/* int */
/* _mpc_thread_ethread_sched_yield_to_idle () */
/* { */
/*   _mpc_thread_ethread_per_thread_t *current; */
/*   _mpc_thread_ethread_virtual_processor_t *current_vp; */
/*   _mpc_thread_ethread_self_all (&current_vp, &current); */

/*   return ___mpc_thread_ethread_sched_yield_to_idle (current_vp, current); */
/* } */

static int _mpc_thread_ethread_sched_dump(char *file)
{
	_mpc_thread_ethread_t self = NULL;

	self               = _mpc_thread_ethread_self();
	self->status       = ethread_dump;
	self->file_to_dump = file;
	return
	        ___mpc_thread_ethread_sched_yield_vp( (_mpc_thread_ethread_virtual_processor_t *)
	                                              self->vp, self);
}

static int _mpc_thread_ethread_sched_migrate()
{
	_mpc_thread_ethread_t self = NULL;

	self                     = _mpc_thread_ethread_self();
	self->status             = ethread_dump;
	self->dump_for_migration = 1;
	return
	        ___mpc_thread_ethread_sched_yield_vp( (_mpc_thread_ethread_virtual_processor_t *)
	                                              self->vp, self);
}

static int _mpc_thread_ethread_sched_restore(mpc_thread_t thread, char *type, int vp)
{
	struct sctk_alloc_chain *tls = NULL;

	assume(vp == 0);

	mpc_common_nodebug("Try to restore %p %s", thread, type);
	__sctk_restore_tls(&tls, type);

	/*Reinit status */
	( (_mpc_thread_ethread_t)thread)->vp     = &virtual_processor;
	( (_mpc_thread_ethread_t)thread)->status = ethread_ready;

/*   _mpc_thread_ethread_print_task((_mpc_thread_ethread_t)thread); */

	/*Place in ready queue */
	_mpc_thread_ethread_enqueue_task(&virtual_processor, thread);


	return 0;
}

static int _mpc_thread_ethread_sched_dump_clean()
{
	return 0;
}

static double
_mpc_thread_ethread_get_activity(int i)
{
	assume(i == 0);
	return virtual_processor.usage;
}

/*Thread polling*/
static void _mpc_thread_ethread_wait_for_value_and_poll(volatile int *data, int value,
                                                        void (*func)(void *), void *arg)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	mpc_common_nodebug("wait real : %d", current_vp->rank);
	___mpc_thread_ethread_wait_for_value_and_poll(current_vp,
	                                              current, data, value, func, arg);
}

static int _mpc_thread_ethread_join(_mpc_thread_ethread_t th, void **val)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_join(current_vp, current, th, val);
}

static void _mpc_thread_ethread_exit(void *retval)
{
	_mpc_thread_ethread_per_thread_t *current = NULL;

	current = _mpc_thread_ethread_self();
	___mpc_thread_ethread_exit(retval, current);
}

/*Mutex management*/
static int _mpc_thread_ethread_mutex_init(mpc_thread_mutex_t *lock,
                                          const mpc_thread_mutexattr_t *attr)
{
	return ___mpc_thread_ethread_mutex_init( (_mpc_thread_ethread_mutex_t *)lock,
	                                         (_mpc_thread_ethread_mutexattr_t *)attr);
}

static int _mpc_thread_ethread_mutex_lock(_mpc_thread_ethread_mutex_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_mutex_lock(current_vp, current, lock);
}

static int _mpc_thread_ethread_mutex_spinlock(_mpc_thread_ethread_mutex_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_mutex_lock(current_vp, current, lock);
}

static int _mpc_thread_ethread_mutex_trylock(_mpc_thread_ethread_mutex_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_mutex_trylock(current_vp, current, lock);
}

static int _mpc_thread_ethread_mutex_unlock(_mpc_thread_ethread_mutex_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_mutex_unlock(current_vp,
	                                          current,
	                                          _mpc_thread_ethread_return_task, lock);
}

/*Key management*/
static int _mpc_thread_ethread_key_create(int *key,
                                          stck_ethread_key_destr_function_t destr_func)
{
	return ___mpc_thread_ethread_key_create(key, destr_func);
}

static int _mpc_thread_ethread_key_delete(int key)
{
	return ___mpc_thread_ethread_key_delete(_mpc_thread_ethread_self(), key);
}

static int _mpc_thread_ethread_setspecific(int key, void *val)
{
	return ___mpc_thread_ethread_setspecific(_mpc_thread_ethread_self(), key, val);
}

static void *
_mpc_thread_ethread_getspecific(int key)
{
	return ___mpc_thread_ethread_getspecific(_mpc_thread_ethread_self(), key);
}

/*Attribut creation*/
static int _mpc_thread_ethread_attr_init(_mpc_thread_ethread_attr_t *attr)
{
	attr->ptr = (_mpc_thread_ethread_attr_intern_t *)
	            sctk_malloc(sizeof(_mpc_thread_ethread_attr_intern_t) );
	return ___mpc_thread_ethread_attr_init(attr->ptr);
}

static int _mpc_thread_ethread_attr_destroy(_mpc_thread_ethread_attr_t *attr)
{
	sctk_free(attr->ptr);
	return 0;
}

static int _mpc_thread_ethread_create(_mpc_thread_ethread_t *threadp,
                                      _mpc_thread_ethread_attr_t *attr,
                                      void *(*start_routine)(void *), void *arg)
{
	if(attr != NULL)
	{
		return ___mpc_thread_ethread_create(ethread_ready, &virtual_processor,
		                                    _mpc_thread_ethread_self(),
		                                    threadp, attr->ptr, start_routine, arg);
	}

	return ___mpc_thread_ethread_create(ethread_ready, &virtual_processor,
												_mpc_thread_ethread_self(),
												threadp, NULL, start_routine, arg);

}

static int _mpc_thread_ethread_user_create(_mpc_thread_ethread_t *threadp,
                                           _mpc_thread_ethread_attr_t *attr,
                                           void *(*start_routine)(void *), void *arg)
{
	if(attr != NULL)
	{
		return ___mpc_thread_ethread_create(ethread_ready, &virtual_processor,
		                                    _mpc_thread_ethread_self(),
		                                    threadp, attr->ptr, start_routine, arg);
	}

	return ___mpc_thread_ethread_create(ethread_ready, &virtual_processor,
												_mpc_thread_ethread_self(),
												threadp, NULL, start_routine, arg);

}

static void _mpc_thread_ethread_freeze_thread_on_vp(_mpc_thread_ethread_mutex_t *lock,
                                                    void **list_tmp)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	___mpc_thread_ethread_freeze_thread_on_vp(current_vp,
	                                          current,
	                                          _mpc_thread_ethread_mutex_unlock,
	                                          lock, (volatile void **)list_tmp);
}

static void _mpc_thread_ethread_wake_thread_on_vp(void **list_tmp)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_self_all(&current_vp, &current);

	___mpc_thread_ethread_wake_thread_on_vp(current_vp, list_tmp);
}

static int _mpc_thread_ethread_equal(mpc_thread_t a, mpc_thread_t b)
{
	return a == b;
}

static int _mpc_thread_ethread_kill(_mpc_thread_ethread_per_thread_t *thread, int val)
{
	return ___mpc_thread_ethread_kill(thread, val);
}

static int _mpc_thread_ethread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask)
{
	_mpc_thread_ethread_per_thread_t *cur = NULL;

	cur = _mpc_thread_ethread_self();
	return ___mpc_thread_ethread_sigmask(cur, how, newmask, oldmask);
}

static int _mpc_thread_ethread_sigpending(sigset_t *set)
{
	int res = 0;
	_mpc_thread_ethread_per_thread_t *cur = NULL;

	cur = _mpc_thread_ethread_self();
	res = ___mpc_thread_ethread_sigpending(cur, set);
	return res;
}

static int _mpc_thread_ethread_sigsuspend(sigset_t *set)
{
	int res = 0;
	_mpc_thread_ethread_per_thread_t *cur = NULL;

	cur = _mpc_thread_ethread_self();
	res = ___mpc_thread_ethread_sigsuspend(cur, set);
	return res;
}

static int _mpc_thread_ethread_thread_attr_setbinding(__UNUSED__ mpc_thread_attr_t *__attr, __UNUSED__ int __binding)
{
	return 0;
}

static int _mpc_thread_ethread_thread_attr_getbinding(__UNUSED__ mpc_thread_attr_t *__attr, __UNUSED__ int *__binding)
{
	return 0;
}

/* These are wrappers around the Ethread Posix interface
 *  providing the per thread engine task function */

static int _mpc_thread_ethread_sem_post(_mpc_thread_ethread_sem_t *sem)
{
	return _mpc_thread_ethread_posix_sem_post(sem, _mpc_thread_ethread_return_task);
}

static int _mpc_thread_ethread_cond_wait(_mpc_thread_ethread_cond_t *cond,
                                         _mpc_thread_ethread_mutex_t *mutex)
{
	return _mpc_thread_ethread_posix_cond_wait(cond, mutex, _mpc_thread_ethread_return_task);
}

static int _mpc_thread_ethread_cond_timedwait(_mpc_thread_ethread_cond_t *cond,
                                              _mpc_thread_ethread_mutex_t *mutex,
                                              const struct timespec *abstime)
{
	return _mpc_thread_ethread_posix_cond_timedwait(cond, mutex, abstime, _mpc_thread_ethread_return_task);
}

static int _mpc_thread_ethread_cond_broadcast(_mpc_thread_ethread_cond_t *cond)
{
	return _mpc_thread_ethread_posix_cond_broadcast(cond, _mpc_thread_ethread_return_task);
}

static int _mpc_thread_ethread_cond_signal(_mpc_thread_ethread_cond_t *cond)
{
	return _mpc_thread_ethread_posix_cond_signal(cond, _mpc_thread_ethread_return_task);
}

static int _mpc_thread_ethread_rwlock_unlock(_mpc_thread_ethread_rwlock_t *rwlock)
{
	return _mpc_thread_ethread_posix_rwlock_unlock(rwlock, _mpc_thread_ethread_return_task);
}

static int _mpc_thread_ethread_barrier_wait(_mpc_thread_ethread_barrier_t *barrier)
{
	return _mpc_thread_ethread_posix_barrier_wait(barrier, _mpc_thread_ethread_return_task);
}


/****************
* NON PORTABLE *
****************/

int _mpc_thread_ethread_engine_setaffinity_np(_mpc_thread_ethread_t thread __UNUSED__, size_t cpusetsize __UNUSED__,
                              const mpc_cpu_set_t *cpuset __UNUSED__)
{
	mpc_common_debug_warning("setaffinity_np not supported");
	return EINVAL;
}

int _mpc_thread_ethread_engine_getaffinity_np(_mpc_thread_ethread_t thread __UNUSED__, size_t cpusetsize,
                              mpc_cpu_set_t *cpuset)
{
	CPU_ZERO_S(cpusetsize, (cpu_set_t*) cpuset);
	int pu = mpc_thread_get_pu();
	CPU_SET_S(pu, cpusetsize, (cpu_set_t*) cpuset);
	return 0;
}

/********************************
* ENGINE REGISTRATION FUNCTION *
********************************/

void mpc_thread_ethread_engine_init(void)
{
	mpc_common_debug_only_once();

	/* Size Checks */
	_mpc_thread_ethread_check_size_eq(int, _mpc_thread_ethread_status_t);

	_mpc_thread_ethread_check_size(_mpc_thread_ethread_mutex_t, mpc_thread_mutex_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_mutexattr_t, mpc_thread_mutexattr_t);

	{
		static _mpc_thread_ethread_mutex_t loc  = SCTK_ETHREAD_MUTEX_INIT;
		static mpc_thread_mutex_t          glob = SCTK_THREAD_MUTEX_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(_mpc_thread_ethread_mutex_t) ) == 0);
	}
	{
		_mpc_thread_ethread_cond_t loc  = SCTK_ETHREAD_COND_INIT;
		mpc_thread_cond_t          glob = SCTK_THREAD_COND_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(_mpc_thread_ethread_cond_t) ) == 0);
	}

	_mpc_thread_ethread_check_size(_mpc_thread_ethread_t, mpc_thread_t);
	_mpc_thread_ethread_check_size(mpc_thread_once_t, mpc_thread_once_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_attr_t, mpc_thread_attr_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_sem_t, mpc_thread_sem_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_cond_t, mpc_thread_cond_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_spinlock_t, mpc_thread_spinlock_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_rwlock_t, mpc_thread_rwlock_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_rwlockattr_t,
	                               mpc_thread_rwlockattr_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_condattr_t, mpc_thread_condattr_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_barrier_t, mpc_thread_barrier_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_barrierattr_t,
	                               mpc_thread_barrierattr_t);

	/* Initialize Ethread */

	sctk_enable_lib_thread_db();

	sctk_init_default_sigset();
	_mpc_thread_kthread_key_create(&_mpc_thread_ethread_key, NULL);
	_mpc_thread_kthread_setspecific(_mpc_thread_ethread_key, &virtual_processor);

	int i = 0;

	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		_mpc_thread_ethread_destr_func_tab[i] = NULL;
	}

	_funcptr_mpc_thread_yield      = _mpc_thread_ethread_sched_yield;
	_funcptr_mpc_thread_dump       = _mpc_thread_ethread_sched_dump;
	_funcptr_mpc_thread_migrate    = _mpc_thread_ethread_sched_migrate;
	_funcptr_mpc_thread_restore    = _mpc_thread_ethread_sched_restore;
	_funcptr_mpc_thread_dump_clean = _mpc_thread_ethread_sched_dump_clean;

	_funcptr_mpc_thread_attr_setbinding = _mpc_thread_ethread_thread_attr_setbinding;
	_funcptr_mpc_thread_attr_getbinding = _mpc_thread_ethread_thread_attr_getbinding;

	sctk_add_func_type(_mpc_thread_ethread, create, int (*)(mpc_thread_t *,
	                                                        const
	                                                        mpc_thread_attr_t *,
	                                                        void *(*)(void *),
	                                                        void *) );
	sctk_add_func_type(_mpc_thread_ethread, user_create,
	                   int (*)(mpc_thread_t *, const mpc_thread_attr_t *,
	                           void *(*)(void *), void *) );


	_mpc_thread_ethread_check_size(_mpc_thread_ethread_attr_t, mpc_thread_attr_t);
	sctk_add_func_type(_mpc_thread_ethread, attr_init, int (*)(mpc_thread_attr_t *) );
	sctk_add_func_type(_mpc_thread_ethread, attr_destroy,
	                   int (*)(mpc_thread_attr_t *) );

	_mpc_thread_ethread_check_size(_mpc_thread_ethread_t, mpc_thread_t);
	sctk_add_func_type(_mpc_thread_ethread, self, mpc_thread_t (*)(void) );
	sctk_add_func_type(_mpc_thread_ethread, equal,
	                   int (*)(mpc_thread_t, mpc_thread_t) );
	sctk_add_func_type(_mpc_thread_ethread, join, int (*)(mpc_thread_t, void **) );
	sctk_add_func_type(_mpc_thread_ethread, exit, void (*)(void *) );


	sctk_add_func_type(_mpc_thread_ethread, mutex_lock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_thread_ethread, mutex_spinlock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_thread_ethread, mutex_trylock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_thread_ethread, mutex_unlock,
	                   int (*)(mpc_thread_mutex_t *) );
	_funcptr_mpc_thread_mutex_init = _mpc_thread_ethread_mutex_init;


	_mpc_thread_ethread_check_size(int, mpc_thread_keys_t);
	_mpc_thread_ethread_check_size(stck_ethread_key_destr_function_t,
	                               void (*)(void *) );
	sctk_add_func_type(_mpc_thread_ethread, key_create,
	                   int (*)(mpc_thread_keys_t *, void (*)(void *) ) );
	sctk_add_func_type(_mpc_thread_ethread, key_delete, int (*)(mpc_thread_keys_t) );
	sctk_add_func_type(_mpc_thread_ethread, setspecific,
	                   int (*)(mpc_thread_keys_t, const void *) );
	sctk_add_func_type(_mpc_thread_ethread, getspecific,
	                   void *(*)(mpc_thread_keys_t) );
	sctk_add_func_type(_mpc_thread_ethread, kill, int (*)(mpc_thread_t, int) );
	sctk_add_func_type(_mpc_thread_ethread, sigpending, int (*)(sigset_t *) );
	sctk_add_func_type(_mpc_thread_ethread, sigsuspend, int (*)(sigset_t *) );


	sctk_add_func(_mpc_thread_ethread, wait_for_value_and_poll);
	sctk_add_func_type(_mpc_thread_ethread, freeze_thread_on_vp,
	                   void (*)(mpc_thread_mutex_t *, void **) );
	sctk_add_func_type(_mpc_thread_ethread, wake_thread_on_vp,
	                   void (*)(void **) );

	sctk_add_func_type(_mpc_thread_ethread, get_activity, double (*)(int) );

	sctk_add_func_type(_mpc_thread_ethread, sigmask,
	                   int (*)(int, const sigset_t *, sigset_t *) );

	/*Init main thread and virtual processor */

	_mpc_thread_ethread_init_data(&_mpc_thread_ethread_main_thread);
	virtual_processor.current          = &_mpc_thread_ethread_main_thread;
	_mpc_thread_ethread_main_thread.vp = &virtual_processor;

	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		_mpc_thread_ethread_main_thread.tls[i] = NULL;
		_mpc_thread_ethread_destr_func_tab[i]  = NULL;
	}

	{
		_mpc_thread_ethread_t idle = NULL;
		___mpc_thread_ethread_create(ethread_idle,
		                             &virtual_processor,
		                             &_mpc_thread_ethread_main_thread,
		                             &idle, NULL, NULL, NULL);
	}


	/* Wrapper for task return function */
	sctk_add_func_type(_mpc_thread_ethread, sem_post, int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread, cond_wait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_thread_ethread, cond_timedwait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *,
	                           const struct timespec *) );
	sctk_add_func_type(_mpc_thread_ethread, cond_broadcast,
	                   int (*)(mpc_thread_cond_t *) );
	sctk_add_func_type(_mpc_thread_ethread, cond_signal,
	                   int (*)(mpc_thread_cond_t *) );
	sctk_add_func_type(_mpc_thread_ethread, rwlock_unlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread, barrier_wait,
	                   int (*)(mpc_thread_barrier_t *) );

	/* From Common Ethread Iface */

	/*le pthread once */
	sctk_add_func_type(_mpc_thread_ethread_posix, once,
	                   int (*)(mpc_thread_once_t *, void (*)(void) ) );

	/*les attributs des threads */
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_setdetachstate,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_getdetachstate,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_setschedpolicy,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_getschedpolicy,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_setinheritsched,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_getinheritsched,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_getscope,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_setscope,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_getstackaddr,
	                   int (*)(const mpc_thread_attr_t *, void **) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_setstackaddr,
	                   int (*)(mpc_thread_attr_t *, void *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_getstacksize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_setstacksize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_getstack,
	                   int (*)(const mpc_thread_attr_t *, void **, size_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_setstack,
	                   int (*)(mpc_thread_attr_t *, void *, size_t) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_setguardsize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_getguardsize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );

	/*gestion du cancel */
	sctk_add_func_type(_mpc_thread_ethread_posix, testcancel, void (*)(void) );
	sctk_add_func_type(_mpc_thread_ethread_posix, cancel, int (*)(mpc_thread_t) );
	sctk_add_func_type(_mpc_thread_ethread_posix, setcancelstate, int (*)(int, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, setcanceltype, int (*)(int, int *) );

	/*pthread_detach */
	sctk_add_func_type(_mpc_thread_ethread_posix, detach, int (*)(mpc_thread_t) );

	/*les semaphores */
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_init,
	                   int (*)(mpc_thread_sem_t *lock,
	                           int pshared, unsigned int value) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_wait, int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_trywait,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_getvalue,
	                   int (*)(mpc_thread_sem_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_destroy,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_open,
	                   mpc_thread_sem_t * (*)(const char *, int, ...) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_close, int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_unlink, int (*)(const char *) );



	/*sched prio */
	sctk_add_func_type(_mpc_thread_ethread_posix, sched_get_priority_min, int (*)(int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sched_get_priority_max, int (*)(int) );


	/*mutex attr */
	sctk_add_func_type(_mpc_thread_ethread_posix, mutexattr_init,
	                   int (*)(mpc_thread_mutexattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, mutexattr_destroy,
	                   int (*)(mpc_thread_mutexattr_t *) );

	sctk_add_func_type(_mpc_thread_ethread_posix, mutexattr_settype,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, mutexattr_gettype,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, mutexattr_setpshared,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, mutexattr_getpshared,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, mutex_destroy,
	                   int (*)(mpc_thread_mutex_t *) );

	/*conditions*/
	sctk_add_func_type(_mpc_thread_ethread_posix, cond_init,
	                   int (*)(mpc_thread_cond_t *,
	                           const mpc_thread_condattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, cond_destroy,
	                   int (*)(mpc_thread_cond_t *) );


	/*attributs des conditions */
	sctk_add_func_type(_mpc_thread_ethread_posix, condattr_init,
	                   int (*)(mpc_thread_condattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, condattr_destroy,
	                   int (*)(mpc_thread_condattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, condattr_getpshared,
	                   int (*)(const mpc_thread_condattr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, condattr_setpshared,
	                   int (*)(mpc_thread_condattr_t *, int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, attr_destroy,
	                   int (*)(mpc_thread_attr_t *) );

	/*spinlock */
	sctk_add_func_type(_mpc_thread_ethread_posix, spin_init,
	                   int (*)(mpc_thread_spinlock_t *, int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, spin_destroy,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, spin_lock,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, spin_trylock,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, spin_unlock,
	                   int (*)(mpc_thread_spinlock_t *) );

	/*rwlock */
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlock_init,
	                   int (*)(mpc_thread_rwlock_t *,
	                           const mpc_thread_rwlockattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlock_rdlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlock_tryrdlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlock_wrlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlock_trywrlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlock_destroy,
	                   int (*)(mpc_thread_rwlock_t *) );

	/*rwlock attr */
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlockattr_init,
	                   int (*)(mpc_thread_rwlockattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlockattr_destroy,
	                   int (*)(mpc_thread_rwlockattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlockattr_getpshared,
	                   int (*)(const mpc_thread_rwlockattr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, rwlockattr_setpshared,
	                   int (*)(mpc_thread_rwlockattr_t *, int) );

	/*barrier */
	sctk_add_func_type(_mpc_thread_ethread_posix, barrier_init,
	                   int (*)(mpc_thread_barrier_t *,
	                           const mpc_thread_barrierattr_t *, unsigned) );
	sctk_add_func_type(_mpc_thread_ethread_posix, barrier_destroy,
	                   int (*)(mpc_thread_barrier_t *) );
	/*barrier attr */
	sctk_add_func_type(_mpc_thread_ethread_posix, barrierattr_init,
	                   int (*)(mpc_thread_barrierattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, barrierattr_destroy,
	                   int (*)(mpc_thread_barrierattr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, barrierattr_getpshared,
	                   int (*)(const mpc_thread_barrierattr_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, barrierattr_setpshared,
	                   int (*)(mpc_thread_barrierattr_t *, int) );
	/*non portable */
	sctk_add_func_type(_mpc_thread_ethread_posix, getattr_np,
	                   int (*)(mpc_thread_t, mpc_thread_attr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_engine, setaffinity_np,
	                   int (*)(mpc_thread_t, size_t, const mpc_cpu_set_t *) );
	sctk_add_func_type(_mpc_thread_ethread_engine, getaffinity_np,
	                   int (*)(mpc_thread_t, size_t, mpc_cpu_set_t *) );

	/* Current */
	_mpc_thread_ethread_debug_status(ethread_ready);

	sctk_multithreading_initialised = 1;
	sctk_free(sctk_malloc(5) );

	_mpc_thread_data_init();
	mpc_topology_set_pu_count(1);
}
