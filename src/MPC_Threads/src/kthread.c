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
#include <dlfcn.h>

#include "mpc_common_asm.h"
#include "pthread_engine.h"
#include "kthread.h"
#include "mpc_common_spinlock.h"
#include "mpc_topology.h"
#include <string.h>
#include <semaphore.h>

#include "thread.h"

#include "sctk_alloc.h"
#include "tls.h"

#ifndef SCTK_KERNEL_THREAD_USE_TLS

int _mpc_thread_kthread_key_create(mpc_thread_keys_t *key, void (*destr_function)(void *) )
{
	return pthread_key_create( (pthread_key_t *)key, destr_function);
}

int _mpc_thread_kthread_key_delete(mpc_thread_keys_t key)
{
	pthread_key_t  keya;
	pthread_key_t *keyp;

	keyp = (pthread_key_t *)&key;
	keya = *keyp;
	return pthread_key_delete(keya);
}

int _mpc_thread_kthread_setspecific(mpc_thread_keys_t key, const void *pointer)
{
	pthread_key_t  keya;
	pthread_key_t *keyp;

	keyp = (pthread_key_t *)&key;
	keya = *keyp;
	return pthread_setspecific(keya, pointer);
}

void *_mpc_thread_kthread_getspecific(mpc_thread_keys_t key)
{
	pthread_key_t  keya;
	pthread_key_t *keyp;

	keyp = (pthread_key_t *)&key;
	keya = *keyp;
	return pthread_getspecific(keya);
}
#endif /* SCTK_KERNEL_THREAD_USE_TLS */

typedef void *(*start_routine_t) (void *);

typedef struct _mpc_thread_kthread_create_start_s
{
	volatile start_routine_t                            start_routine;
	volatile void *                                     arg;
	volatile int                                        started;
	volatile int                                        used;
	volatile struct _mpc_thread_kthread_create_start_s *next;
	sem_t                                               sem;
} _mpc_thread_kthread_create_start_t;

static mpc_common_spinlock_t __thread_list_lock = { 0 };
static volatile _mpc_thread_kthread_create_start_t *__thread_list = NULL;

static void *__kthread_start_func(void *t_arg)
{
	_mpc_thread_kthread_create_start_t slot;

	mpc_topology_clear_cpu_pinning_cache();

	memcpy(&slot, t_arg, sizeof(_mpc_thread_kthread_create_start_t) );
	( (_mpc_thread_kthread_create_start_t *)t_arg)->started = 1;

	sctk_alloc_posix_plug_on_egg_allocator();

	sem_init(&(slot.sem), 0, 0);

	mpc_common_spinlock_lock(&__thread_list_lock);

	if(__thread_list)
	{
		slot.next = __thread_list;
	}
	else
	{
		slot.next = NULL;
	}

	__thread_list = &slot;
	mpc_common_spinlock_unlock(&__thread_list_lock);

	while(1)
	{
		void *(*start_routine) (void *);
		void *arg;

		start_routine = (void *(*)(void *) )slot.start_routine;
		arg           = (void *)slot.arg;

		start_routine(arg);

		slot.used = 0;

		mpc_common_nodebug("Kernel thread done");

		sem_wait(&(slot.sem) );

		while(slot.used != 1)
		{
			sched_yield();
		}

		mpc_common_nodebug("Kernel thread reuse");
	}
	return NULL;
}

int (*real_pthread_create)(pthread_t *, const pthread_attr_t *,
                           void *(*)(void *), void *) = NULL;

int mpc_thread_kthread_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                                      void *(*start_routine)(void *), void *arg)
{
	pthread_attr_t  local_attr;
	pthread_attr_t *r_attr;
	size_t          kthread_stack_size;
	int             res;

	r_attr = (pthread_attr_t *)attr;

	if(r_attr == NULL)
	{
		r_attr = &local_attr;
		res    = pthread_attr_init(r_attr);
		if(res != 0)
		{
			return res;
		}
	}

	res = pthread_attr_getstacksize(r_attr, &kthread_stack_size);
	if(res != 0)
	{
		return res;
	}

	kthread_stack_size += sctk_extls_size();

	res = pthread_attr_setstacksize(r_attr, kthread_stack_size);
	if(res != 0)
	{
		return res;
	}

	res = pthread_create(thread, r_attr, start_routine, arg);
	return res;
}

int _mpc_thread_kthread_create(mpc_thread_kthread_t *thread, void *(*start_routine)(void *),
                               void *arg)
{
	_mpc_thread_kthread_create_start_t *found = NULL;
	_mpc_thread_kthread_create_start_t *cursor;
	size_t kthread_stack_size = _mpc_thread_config_get()->kthread_stack_size;
	
	mpc_common_nodebug("Scan already started kthreads");
	mpc_common_spinlock_lock(&__thread_list_lock);
	cursor = (_mpc_thread_kthread_create_start_t *)__thread_list;
	while(cursor != NULL)
	{
		if(cursor->used == 0)
		{
			found       = (_mpc_thread_kthread_create_start_t *)cursor;
			found->used = 1;
			break;
		}
		cursor = (_mpc_thread_kthread_create_start_t *)cursor->next;
	}
	mpc_common_spinlock_unlock(&__thread_list_lock);
	mpc_common_nodebug("Scan already started kthreads done found %p", found);

	if(found)
	{
		mpc_common_nodebug("Reuse old thread %p", found);
		found->arg           = arg;
		found->start_routine = start_routine;
		found->started       = 0;
		sem_post(&(found->sem) );
		return 0;
	}
	else
	{
		pthread_attr_t attr;
		int            res;
		_mpc_thread_kthread_create_start_t tmp;
		mpc_common_nodebug("Create new kernel thread");

		pthread_attr_init(&attr);
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

//#warning "Move it to the XML configuration file"
		char *env;
		if( (env = getenv("MPC_KTHREAD_STACK_SIZE") ) != NULL)
		{
			kthread_stack_size = atoll(env) /* + sctk_extls_size() */;
		}

#ifdef PTHREAD_STACK_MIN
		if(PTHREAD_STACK_MIN > kthread_stack_size)
		{
			pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
		}
		else
		{
			pthread_attr_setstacksize(&attr, kthread_stack_size);
		}
#else
		pthread_attr_setstacksize(&attr, kthread_stack_size);
#endif
		assume(sizeof(mpc_thread_kthread_t) == sizeof(pthread_t) );

		tmp.started       = 0;
		tmp.arg           = arg;
		tmp.start_routine = start_routine;
		tmp.used          = 1;

		mpc_common_nodebug("_mpc_thread_kthread_create: before pthread_create");

		res = mpc_thread_kthread_pthread_create( (pthread_t *)thread, &attr, __kthread_start_func, &tmp);

		if(res != 0)
		{
			pthread_attr_destroy(&attr);
			pthread_attr_init(&attr);
			pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
			res = mpc_thread_kthread_pthread_create( (pthread_t *)thread, &attr, __kthread_start_func, &tmp);
		}

		mpc_common_nodebug("_mpc_thread_kthread_create: after pthread_create with res = %d", res);

		pthread_attr_destroy(&attr);

		if(res != 0)
		{
			mpc_common_nodebug("Warning: Creating kernel threads with no attribute");
			res = mpc_thread_kthread_pthread_create( (pthread_t *)thread, NULL, __kthread_start_func, &tmp);

			assume(res == 0);
		}

		while(tmp.started != 1)
		{
			sched_yield();
		}

		return res;
	}
}

int _mpc_thread_kthread_join(mpc_thread_kthread_t th, void **thread_return)
{
	return pthread_join( (pthread_t)th, thread_return);
}

int _mpc_thread_kthread_kill(mpc_thread_kthread_t th, int val)
{
	return pthread_kill( (pthread_t)th, val);
}

mpc_thread_kthread_t _mpc_thread_kthread_self()
{
	return (mpc_thread_kthread_t)pthread_self();
}

int _mpc_thread_kthread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask)
{
	return pthread_sigmask(how, newmask, oldmask);
}

void _mpc_thread_kthread_exit(void *retval)
{
	pthread_exit(retval);
}
