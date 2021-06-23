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
#include <stdio.h>

#include <mpc_thread.h>
#include <ng_engine.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "thread.h"
#include "kthread.h"

#include <mpc_topology.h>
#include <string.h>
#include <mpc_common_flags.h>

#define SCTK_BLOCKING_LOCK_TABLE_SIZE    6

/***************************************/
/* THREADS                             */
/***************************************/

static __thread _mpc_threads_ng_engine_p_t *_mpc_threads_ng_engine_self_data;

_mpc_threads_ng_engine_t _mpc_threads_ng_engine_self()
{
	return _mpc_threads_ng_engine_self_data;
}

void _mpc_threads_ng_engine_self_set(_mpc_threads_ng_engine_t th)
{
	_mpc_threads_ng_engine_self_data = th;
}

/***********************
* THREAD KIND SETTERS *
***********************/

void _mpc_threads_ng_engine_kind_set_self(_mpc_threads_ng_engine_kind_t kind)
{
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched = &(_mpc_threads_ng_engine_self()->sched);
	sched->th->attr.kind = kind;
}

void mpc_threads_generic_kind_mask_self(unsigned int kind_mask)
{
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched = &(_mpc_threads_ng_engine_self()->sched);
	sched->th->attr.kind.mask = kind_mask;
}

void mpc_threads_generic_kind_priority(int priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		_mpc_threads_ng_engine_scheduler_t *sched;
		sched = &(_mpc_threads_ng_engine_self()->sched);
		sched->th->attr.kind.priority = priority;
	}
}

void mpc_threads_generic_kind_basic_priority(int basic_priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		_mpc_threads_ng_engine_scheduler_t *sched;
		sched = &(_mpc_threads_ng_engine_self()->sched);
		sched->th->attr.basic_priority = basic_priority;
	}
}

void mpc_threads_generic_kind_current_priority(int current_priority)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		_mpc_threads_ng_engine_scheduler_t *sched;
		sched = &(_mpc_threads_ng_engine_self()->sched);
		sched->th->attr.current_priority = current_priority;
	}
}

/***********************
* THREAD KIND GETTERS *
***********************/

_mpc_threads_ng_engine_kind_t _mpc_threads_ng_engine_kind_get()
{
	_mpc_threads_ng_engine_kind_t       kind;
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched = &(_mpc_threads_ng_engine_self()->sched);
	kind  = sched->th->attr.kind;
	return kind;
}

unsigned int mpc_threads_generic_kind_mask_get()
{
	unsigned int kind_mask;
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched     = &(_mpc_threads_ng_engine_self()->sched);
	kind_mask = sched->th->attr.kind.mask;
	return kind_mask;
}

int mpc_threads_generic_kind_basic_priority_get()
{
	int priority;
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched    = &(_mpc_threads_ng_engine_self()->sched);
	priority = sched->th->attr.basic_priority;
	return priority;
}

int mpc_threads_generic_kind_current_priority_get()
{
	int priority;
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched    = &(_mpc_threads_ng_engine_self()->sched);
	priority = sched->th->attr.kind.priority;
	return priority;
}

int mpc_threads_generic_kind_priority_get()
{
	int priority;
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched    = &(_mpc_threads_ng_engine_self()->sched);
	priority = sched->th->attr.kind.priority;
	return priority;
}

void mpc_threads_generic_kind_mask_add(unsigned int kind_mask)
{
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		_mpc_threads_ng_engine_scheduler_t *sched;
		sched = &(_mpc_threads_ng_engine_self()->sched);
		sched->th->attr.kind.mask |= kind_mask;
	}
}

void mpc_threads_generic_kind_mask_remove(unsigned int kind_mask)
{
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched = &(_mpc_threads_ng_engine_self()->sched);
	sched->th->attr.kind.mask &= ~kind_mask;
}

/***************************************/
/* KEYS                                */
/***************************************/

/* Globals */

static mpc_common_spinlock_t __keys_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static char __keys_in_use[SCTK_THREAD_KEYS_MAX + 1];


typedef void (*__keys_destructors_t) (void *);

static __keys_destructors_t __keys_destructors[SCTK_THREAD_KEYS_MAX + 1];

/* Interface */

static int _mpc_threads_ng_engine_setspecific(mpc_thread_keys_t __key, const void *__pointer)
{
	if(__keys_in_use[__key] == 1)
	{
		const void **keys = _mpc_threads_ng_engine_self()->keys.keys;
		keys[__key] = __pointer;
		return 0;
	}
	else
	{
		return EINVAL;
	}
}

static void *_mpc_threads_ng_engine_getspecific(mpc_thread_keys_t __key)
{
	if(__keys_in_use[__key] == 1)
	{
		const void **keys = _mpc_threads_ng_engine_self()->keys.keys;
		return (void *)keys[__key];
	}
	else
	{
		return NULL;
	}
}

static int _mpc_threads_ng_engine_key_create(mpc_thread_keys_t *__key,
                                           void (*__destr_function)(void *) )
{
	int i;

	mpc_common_spinlock_lock(&__keys_lock);
	for(i = 1; i < SCTK_THREAD_KEYS_MAX + 1; i++)
	{
		if(__keys_in_use[i] == 0)
		{
			__keys_in_use[i]      = 1;
			__keys_destructors[i] = __destr_function;
			*__key = i;
			break;
		}
	}
	mpc_common_spinlock_unlock(&__keys_lock);
	if(i == SCTK_THREAD_KEYS_MAX)
	{
		return EAGAIN;
	}

	return 0;
}

static int _mpc_threads_ng_engine_key_delete(mpc_thread_keys_t __key)
{
	if(__keys_in_use[__key] == 1)
	{
		if(__keys_destructors[__key] != NULL)
		{
			const void **keys = _mpc_threads_ng_engine_self()->keys.keys;
			__keys_destructors[__key] (&keys[__key]);
			keys[__key] = NULL;
		}
	}

	return 0;
}

void _mpc_threads_ng_engine_key_init_thread(_mpc_threads_ng_engine_keys_t *keys)
{
	int i;

	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		keys->keys[i] = NULL;
	}
}

/* Init and release */

static inline void __keys_init()
{
	int i;

	_mpc_threads_ng_engine_check_size(int, mpc_thread_keys_t);
	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		__keys_in_use[i]      = 0;
		__keys_destructors[i] = NULL;
	}
}

static inline void __keys_delete_all(_mpc_threads_ng_engine_keys_t *keys)
{
	int i;

	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		if(__keys_in_use[i] == 1)
		{
			if(__keys_destructors[i] != NULL)
			{
				__keys_destructors[i] (&keys->keys[i]);
				keys->keys[i] = NULL;
			}
		}
	}
}

/***************************************/
/* MUTEX                               */
/***************************************/

/* Interface */

static int _mpc_threads_ng_engine_mutexattr_destroy(mpc_thread_mutexattr_t *attr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	return 0;
}

static int _mpc_threads_ng_engine_mutexattr_getpshared(mpc_thread_mutexattr_t *pattr, int *pshared)
{
	/*
	 *     ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */
	_mpc_threads_ng_engine_mutexattr_t *attr = (_mpc_threads_ng_engine_mutexattr_t *)pattr;

	if(attr == NULL || pshared == NULL)
	{
		return EINVAL;
	}

	(*pshared) = ( (attr->attrs >> 2) & 1);

	return 0;
}

/*
 * static int
 * _mpc_threads_ng_engine_mutexattr_getprioceiling( mpc_thread_mutexattr_t* attr,
 *                              int* prioceiling ){
 * return _mpc_threads_ng_engine_mutexes_mutexattr_getprioceiling((_mpc_threads_ng_engine_mutexattr_t *) attr,
 *                                              prioceiling );
 * }
 *
 * static int
 * _mpc_threads_ng_engine_mutexattr_setprioceiling( mpc_thread_mutexattr_t* attr,
 *                              int prioceiling ){
 * return _mpc_threads_ng_engine_mutexes_mutexattr_setprioceiling((_mpc_threads_ng_engine_mutexattr_t *) attr,
 *                                              prioceiling );
 * }
 *
 * static int
 * _mpc_threads_ng_engine_mutexattr_getprotocol( mpc_thread_mutexattr_t* attr,
 *                              int* protocol ){
 * return _mpc_threads_ng_engine_mutexes_mutexattr_getprotocol((_mpc_threads_ng_engine_mutexattr_t *) attr,
 *                                              protocol );
 * }
 *
 * static int
 * _mpc_threads_ng_engine_mutexattr_setprotocol( mpc_thread_mutexattr_t* attr,
 *                              int protocol ){
 * return _mpc_threads_ng_engine_mutexes_mutexattr_setprotocol((_mpc_threads_ng_engine_mutexattr_t *) attr,
 *                                              protocol );
 * }
 */

static int _mpc_threads_ng_engine_mutexattr_gettype(mpc_thread_mutexattr_t *pattr, int *kind)
{
	/*
	 *     ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */
	_mpc_threads_ng_engine_mutexattr_t *attr = (_mpc_threads_ng_engine_mutexattr_t *)pattr;


	if(attr == NULL || kind == NULL)
	{
		return EINVAL;
	}

	(*kind) = (attr->attrs & 3);

	return 0;
}

static int _mpc_threads_ng_engine_mutexattr_init(mpc_thread_mutexattr_t *pattr)
{
	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 * ENOMEM |> NOT IMPLEMENTED <| Insufficient memory exists to initialize
	 *       the read-write lock attributes object
	 */
	_mpc_threads_ng_engine_mutexattr_t *attr = (_mpc_threads_ng_engine_mutexattr_t *)pattr;


	if(attr == NULL)
	{
		return EINVAL;
	}

	attr->attrs  = ( (attr->attrs & ~3) | (0 & 3) );
	attr->attrs &= ~(1 << 2);

	return 0;
}

static int _mpc_threads_ng_engine_mutexattr_setpshared(mpc_thread_mutexattr_t *pattr, int pshared)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */
	_mpc_threads_ng_engine_mutexattr_t *attr = (_mpc_threads_ng_engine_mutexattr_t *)pattr;


	if(attr == NULL)
	{
		return EINVAL;
	}
	if(pshared != SCTK_THREAD_PROCESS_PRIVATE && pshared != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}

	int ret = 0;
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		attr->attrs |= (1 << 2);
		fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared mutexes\n");
		ret = ENOTSUP;
	}
	else
	{
		attr->attrs &= ~(1 << 2);
	}

	return ret;
}

static int _mpc_threads_ng_engine_mutexattr_settype(mpc_thread_mutexattr_t *pattr, int kind)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	_mpc_threads_ng_engine_mutexattr_t *attr = (_mpc_threads_ng_engine_mutexattr_t *)pattr;

	if(attr == NULL)
	{
		return EINVAL;
	}
	if(kind != PTHREAD_MUTEX_NORMAL && kind != PTHREAD_MUTEX_ERRORCHECK &&
	   kind != PTHREAD_MUTEX_RECURSIVE && kind != PTHREAD_MUTEX_DEFAULT)
	{
		return EINVAL;
	}

	attr->attrs = (attr->attrs & ~3) | (kind & 3);

	return 0;
}

static int _mpc_threads_ng_engine_mutex_destroy(mpc_thread_mutex_t *plock)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 *    EBUSY  The specified lock is currently owned by a thread or
	 *               another thread is currently using the mutex in a cond
	 */
	_mpc_threads_ng_engine_mutex_t *lock = (_mpc_threads_ng_engine_mutex_t *)plock;


	if(lock == NULL)
	{
		return EINVAL;
	}
	if(lock->owner != NULL)
	{
		return EBUSY;
	}

	return 0;
}

static int _mpc_threads_ng_engine_mutex_init(mpc_thread_mutex_t *lock,
                                           const mpc_thread_mutexattr_t *pattr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 * EBUSY  |>NOT IMPLEMENTED<| The specified lock has already been
	 *               initialized
	 *    EAGAIN |>NOT IMPLEMENTED<| The system lacked the necessary
	 *               resources (other than memory) to initialize another mutex
	 *    ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to
	 *               initialize the mutex
	 *    EPERM  |>NOT IMPLEMENTED<| The caller does not have the
	 *               privilege to perform the operation
	 */

	_mpc_threads_ng_engine_mutexattr_t *attr = (_mpc_threads_ng_engine_mutexattr_t *)pattr;

	if(lock == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	_mpc_threads_ng_engine_mutex_t  local_lock = SCTK_THREAD_GENERIC_MUTEX_INIT;
	_mpc_threads_ng_engine_mutex_t *local_ptr  = &local_lock;

	if(attr != NULL)
	{
		if( ( (attr->attrs >> 2) & 1) == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared mutexes\n");
			ret = ENOTSUP;
		}
		local_ptr->type = (attr->attrs & 3);
	}

	memcpy(lock, &local_lock, sizeof(_mpc_threads_ng_engine_mutex_t) );

	return ret;
}

static int _mpc_threads_ng_engine_mutex_lock(mpc_thread_mutex_t *plock)
{
	/*
	 *    ERRORS:
	 *    EINVAL  The value specified for the argument is not correct
	 *    EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum
	 *                number of recursive locks for mutex has been exceeded
	 *    EDEADLK The current thread already owns the mutex
	 */

	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);
	_mpc_threads_ng_engine_mutex_t *    lock  = (_mpc_threads_ng_engine_mutex_t *)plock;


	if(lock == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	_mpc_threads_ng_engine_mutex_cell_t cell;
	void **tmp = (void **)sched->th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table;
	mpc_common_spinlock_lock(&(lock->lock) );
	if(lock->owner == NULL)
	{
		lock->owner = sched;
		if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
		{
			lock->nb_call++;
		}
		mpc_common_spinlock_unlock(&(lock->lock) );
		// We can force sched_yield here to increase calls to the priority scheduler
		// _mpc_threads_ng_engine_sched_yield(sched);
		return ret;
	}
	else
	{
		if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE &&
		   lock->owner == sched)
		{
			lock->nb_call++;
			mpc_common_spinlock_unlock(&(lock->lock) );
			// We can force sched_yield here to increase calls to the priority
			// scheduler
			// _mpc_threads_ng_engine_sched_yield(sched);
			return ret;
		}
		if(lock->type == SCTK_THREAD_MUTEX_ERRORCHECK &&
		   lock->owner == sched)
		{
			ret = EDEADLK;
			mpc_common_spinlock_unlock(&(lock->lock) );
			return ret;
		}

		cell.sched = sched;
		DL_APPEND(lock->blocked, &cell);
		tmp[MPC_THREADS_GENERIC_MUTEX] = (void *)lock;

		_mpc_threads_ng_engine_thread_status(sched, _mpc_threads_ng_engine_blocked);
		mpc_common_nodebug("WAIT MUTEX LOCK sleep %p", sched);
		_mpc_threads_ng_engine_register_spinlock_unlock(sched, &(lock->lock) );
		_mpc_threads_ng_engine_sched_yield(sched);
		tmp[MPC_THREADS_GENERIC_MUTEX] = NULL;
	}
	return ret;
}

static int _mpc_threads_ng_engine_mutex_trylock(mpc_thread_mutex_t *plock)
{
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);
	_mpc_threads_ng_engine_mutex_t *    lock  = (_mpc_threads_ng_engine_mutex_t *)plock;

	/*
	 *    ERRORS:
	 *    EINVAL  The value specified for the argument is not correct
	 *    EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum
	 *                number of recursive locks for mutex has been exceeded
	 *    EBUSY   the mutex is already owned by another thread or the calling thread
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	if(/*mpc_common_spinlock_trylock(&(lock->lock)) == 0 */ 1)
	{
		mpc_common_spinlock_lock(&(lock->lock) );
		if(lock->owner == NULL)
		{
			lock->owner = sched;
			if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
			{
				lock->nb_call++;
			}
			mpc_common_spinlock_unlock(&(lock->lock) );
			return ret;
		}
		else
		{
			if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE &&
			   lock->owner == sched)
			{
				lock->nb_call++;
				mpc_common_spinlock_unlock(&(lock->lock) );
				return ret;
			}
			ret = EBUSY;
			mpc_common_spinlock_unlock(&(lock->lock) );
		}
	}
	else
	{
		ret = EBUSY;
	}

	return ret;
}

static int _mpc_threads_ng_engine_mutex_timedlock(mpc_thread_mutex_t *plock,
                                                const struct timespec *restrict time)
{
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);
	_mpc_threads_ng_engine_mutex_t *    lock  = (_mpc_threads_ng_engine_mutex_t *)plock;

	/*
	 *    ERRORS:
	 *    EINVAL    The value specified by argument lock does not refer to an initialized
	 *                      mutex object, or the abs_timeout nanosecond value is less than zero or
	 *                      greater than or equal to 1000 million
	 *    ETIMEDOUT The lock could not be acquired in specified time
	 *    EAGAIN    |> NOT IMPLEMENTED <| The read lock could not be acquired because the
	 *                      maximum number of recursive locks for mutex has been exceeded
	 */

	if(lock == NULL || time == NULL)
	{
		return EINVAL;
	}
	if(time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
	{
		return EINVAL;
	}

	int             ret = 0;
	struct timespec t_current;

	do
	{
		if(mpc_common_spinlock_trylock(&(lock->lock) ) == 0)
		{
			if(lock->owner == NULL)
			{
				lock->owner = sched;
				if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
				{
					lock->nb_call++;
				}
			}
			else
			{
				if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE &&
				   lock->owner == sched)
				{
					lock->nb_call++;
				}
				ret = EBUSY;
			}
		}
		else
		{
			ret = EBUSY;
		}
		mpc_common_spinlock_unlock(&(lock->lock) );
		clock_gettime(CLOCK_REALTIME, &t_current);
	} while(ret != 0 && (t_current.tv_sec < time->tv_sec ||
	                     (t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec) ) );

	if(ret != 0)
	{
		return ETIMEDOUT;
	}

	return ret;
}

static int _mpc_threads_ng_engine_mutex_spinlock(mpc_thread_mutex_t *plock)
{
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);
	_mpc_threads_ng_engine_mutex_t *    lock  = (_mpc_threads_ng_engine_mutex_t *)plock;

	/*
	 *    ERRORS:
	 *    EINVAL  The value specified for the argument is not correct
	 *    EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum
	 *                number of recursive locks for mutex has been exceeded
	 *    EDEADLK The current thread already owns the mutex
	 */

	/* TODO: FIX BUG when called by "sctk_thread_lock_lock" (same issue with mutex
	 * unlock when called by "sctk_thread_lock_unlock") with sched null beacause calling
	 * functions only have one argument instead of two*/
	if(lock == NULL /*|| (lock->m_attrs & 1) != 1*/)
	{
		return EINVAL;
	}

	int ret = 0;
	_mpc_threads_ng_engine_mutex_cell_t cell;

	mpc_common_spinlock_lock(&(lock->lock) );
	if(lock->owner == NULL)
	{
		lock->owner   = sched;
		lock->nb_call = 1;
		mpc_common_spinlock_unlock(&(lock->lock) );
		return ret;
	}
	else
	{
		if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
		{
			lock->nb_call++;
			mpc_common_spinlock_unlock(&(lock->lock) );
			return ret;
		}
		if(lock->type == SCTK_THREAD_MUTEX_ERRORCHECK)
		{
			if(lock->owner == sched)
			{
				ret = EDEADLK;
				mpc_common_spinlock_unlock(&(lock->lock) );
				return ret;
			}
		}

		cell.sched = sched;
		DL_APPEND(lock->blocked, &cell);

		mpc_common_spinlock_unlock(&(lock->lock) );
		do
		{
			_mpc_threads_ng_engine_sched_yield(sched);
		} while(lock->owner != sched);
	}
	return ret;
}

static int _mpc_threads_ng_engine_mutex_unlock(mpc_thread_mutex_t *plock)
{
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);
	_mpc_threads_ng_engine_mutex_t *    lock  = (_mpc_threads_ng_engine_mutex_t *)plock;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified by argument lock does not refer to an initialized mutex
	 *    EAGAIN |> NOT IMPLEMENTED <| The read lock could not be acquired because the
	 *                maximum number of recursive locks for mutex has been exceeded
	 *    EPERM  The current thread does not own the mutex
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}

	if(lock->owner != sched)
	{
		return EPERM;
	}
	if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE)
	{
		if(lock->nb_call > 1)
		{
			lock->nb_call--;
			return 0;
		}
	}

	mpc_common_spinlock_lock(&(lock->lock) );
	{
		_mpc_threads_ng_engine_mutex_cell_t *head;
		head = lock->blocked;
		if(head == NULL)
		{
			lock->owner   = NULL;
			lock->nb_call = 0;
		}
		else
		{
			lock->owner   = head->sched;
			lock->nb_call = 1;
			DL_DELETE(lock->blocked, head);
			if(head->sched->status != _mpc_threads_ng_engine_running)
			{
				mpc_common_nodebug("ADD MUTEX UNLOCK wake %p", head->sched);
				_mpc_threads_ng_engine_wake(head->sched);
			}
		}
	}
	mpc_common_spinlock_unlock(&(lock->lock) );
	return 0;
}

/* Init */

void __mutex_init()
{
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_mutex_t, mpc_thread_mutex_t);
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_mutexattr_t, mpc_thread_mutexattr_t);

	{
		static _mpc_threads_ng_engine_mutex_t loc  = SCTK_THREAD_GENERIC_MUTEX_INIT;
		static mpc_thread_mutex_t           glob = SCTK_THREAD_MUTEX_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(_mpc_threads_ng_engine_mutex_t) ) == 0);
	}
}

/***************************************/
/* THREAD SIGNALS                      */
/***************************************/

static sigset_t sctk_thread_default_set;
static int      main_thread_sigs_initialized = 0;
int             errno;

static inline void __default_sigset_init()
{
#ifdef WINDOWS_SYS
	return;
#endif
	sigset_t set;
	sigemptyset(&set);

	if(!main_thread_sigs_initialized)
	{
		main_thread_sigs_initialized++;
		_mpc_thread_kthread_sigmask(SIG_SETMASK, &set, &sctk_thread_default_set);
		_mpc_thread_kthread_sigmask(SIG_SETMASK, &sctk_thread_default_set, NULL);
	}
	else
	{
		_mpc_thread_kthread_sigmask(SIG_SETMASK, (sigset_t *)&(_mpc_threads_ng_engine_self()->attr.thread_sigset), &set);
		_mpc_thread_kthread_sigmask(SIG_SETMASK, &set, &sctk_thread_default_set);
	}
}

static inline void ___check_signals(_mpc_threads_ng_engine_t threadp)
{
#ifdef WINDOWS_SYS
	return;
#endif
	_mpc_threads_ng_engine_p_t *th = threadp;
	int        i, done = 0;
	static int nb_inner_calls = 0;
	sigset_t   set, current_set;
	sigemptyset(&set);
	if(nb_inner_calls == 0)
	{
		th->attr.old_thread_sigset = th->attr.thread_sigset;
	}
	_mpc_thread_kthread_sigmask(SIG_SETMASK, &set, &current_set);
	_mpc_thread_kthread_sigmask(SIG_SETMASK, &current_set, NULL);
	if( (*( (unsigned long *)&(th->attr.sa_sigset_mask) ) > 0) || (*( (unsigned long *)&(current_set) ) > 0) )
	{
		_mpc_thread_kthread_sigmask(SIG_SETMASK, (sigset_t *)&(th->attr.thread_sigset), NULL);
		_mpc_thread_kthread_sigmask(SIG_BLOCK, &current_set, NULL);
		_mpc_thread_kthread_sigmask(SIG_BLOCK, (sigset_t *)&(th->attr.sa_sigset_mask), NULL);
		_mpc_thread_kthread_sigmask(SIG_SETMASK, &current_set, (sigset_t *)&(th->attr.thread_sigset) );
	}

	if(&(th->attr.spinlock) != &(_mpc_threads_ng_engine_self()->attr.spinlock) )
	{
		mpc_common_spinlock_lock(&(th->attr.spinlock) );

		for(i = 0; i < SCTK_NSIG; i++)
		{
			if(expect_false(th->attr.thread_sigpending[i] != 0) )
			{
				if(sigismember( (sigset_t *)&(th->attr.thread_sigset), i + 1) == 0)
				{
					th->attr.thread_sigpending[i] = 0;
					th->attr.nb_sig_pending--;
					done++;
					nb_inner_calls++;

					_mpc_thread_kthread_kill(_mpc_thread_kthread_self(), i + 1);
					nb_inner_calls--;

					if(nb_inner_calls == 0)
					{
						th->attr.thread_sigset = th->attr.old_thread_sigset;
						sigemptyset( (sigset_t *)&(th->attr.sa_sigset_mask) );
					}
					th->attr.nb_sig_treated += done;
				}
			}
		}

		th->attr.nb_sig_treated += done;
		mpc_common_spinlock_unlock(&(th->attr.spinlock) );
	}
	else
	{
		for(i = 0; i < SCTK_NSIG; i++)
		{
			if(expect_false(th->attr.thread_sigpending[i] != 0) )
			{
				if(sigismember( (sigset_t *)&(th->attr.thread_sigset), i + 1) == 0)
				{
					th->attr.thread_sigpending[i] = 0;
					th->attr.nb_sig_pending--;
					done++;
					nb_inner_calls++;

					_mpc_thread_kthread_kill(_mpc_thread_kthread_self(), i + 1);
					nb_inner_calls--;

					if(nb_inner_calls == 0)
					{
						th->attr.thread_sigset = th->attr.old_thread_sigset;
						sigemptyset( (sigset_t *)&(th->attr.sa_sigset_mask) );
					}
					th->attr.nb_sig_treated += done;
				}
			}
		}
	}
}

void _mpc_threads_ng_engine_check_signals(int select)
{
	_mpc_threads_ng_engine_scheduler_t *sched;
	_mpc_threads_ng_engine_p_t *        current;

	/* Get the current thread */
	sched = &(_mpc_threads_ng_engine_self()->sched);
	mpc_common_nodebug("_mpc_threads_ng_engine_check_signals %p", sched);
	current = sched->th;
	assert(&current->sched == sched);

	if(expect_false(current->attr.nb_sig_pending > 0) )
	{
		___check_signals(current);
	}

	if(expect_false(current->attr.cancel_status > 0) )
	{
		mpc_common_debug(
		        "%p %d %d", current,
		        (current->attr.cancel_state != SCTK_THREAD_CANCEL_DISABLE),
		        ( (current->attr.cancel_type != SCTK_THREAD_CANCEL_DEFERRED) && select) );

		if( (current->attr.cancel_state != SCTK_THREAD_CANCEL_DISABLE) &&
		    ( (current->attr.cancel_type != SCTK_THREAD_CANCEL_DEFERRED) ||
		      !select) )
		{
			mpc_common_debug("Exit Thread %p", current);
			current->attr.cancel_status = 0;
			current->attr.return_value  = ( (void *)SCTK_THREAD_CANCELED);
			_mpc_thread_exit_cleanup();
			mpc_common_debug("thread %p key liberation", current);
			__keys_delete_all(&(current->keys) );
			mpc_common_debug("thread %p key liberation done", current);
			mpc_common_debug("thread %p ends", current);

			_mpc_threads_ng_engine_thread_status(&(current->sched),
			                                   _mpc_threads_ng_engine_zombie);
			_mpc_threads_ng_engine_sched_yield(&(current->sched) );
		}
	}
}

static inline void __attr_init_signals(const _mpc_threads_ng_engine_attr_t *attr)
{
	int i;

	for(i = 0; i < SCTK_NSIG; i++)
	{
		attr->ptr->thread_sigpending[i] = 0;
	}

	attr->ptr->thread_sigset = sctk_thread_default_set;
	sigemptyset( (sigset_t *)&(attr->ptr->sa_sigset_mask) );
}

static int _mpc_threads_ng_engine_sigpending(sigset_t *set)
{
	/*
	 *    ERRORS:
	 *    EFAULT |>NOT IMPLEMENTED<|
	 */

#ifdef WINDOWS_SYS
	return 0;
#endif
	int i;

	sigemptyset(set);
	_mpc_threads_ng_engine_p_t *th = _mpc_threads_ng_engine_self();

	mpc_common_spinlock_lock(&(th->attr.spinlock) );

	for(i = 0; i < SCTK_NSIG; i++)
	{
		if(th->attr.thread_sigpending[i] != 0)
		{
			sigaddset(set, i + 1);
		}
	}

	mpc_common_spinlock_unlock(&(th->attr.spinlock) );

	return 0;
}

static inline int __thread_sigmask(_mpc_threads_ng_engine_t threadp, int how,
                                   const sigset_t *newmask, sigset_t *oldmask)
{
	int res = -1;

	if(how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK)
	{
		return EINVAL;
	}
	_mpc_threads_ng_engine_p_t *th = threadp;
	sigset_t set;

	_mpc_thread_kthread_sigmask(SIG_SETMASK, (sigset_t *)&(th->attr.thread_sigset), &set);
	res = _mpc_thread_kthread_sigmask(how, newmask, oldmask);
	_mpc_thread_kthread_sigmask(SIG_SETMASK, &set, (sigset_t *)&(th->attr.thread_sigset) );

	_mpc_threads_ng_engine_check_signals(1);

	return res;
}

static int _mpc_threads_ng_engine_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask)
{
	/*
	 *    ERRORS:
	 *    EINVAL  The value specified in how was invalid
	 */
	int res = -1;

#ifndef WINDOWS_SYS
	_mpc_threads_ng_engine_p_t *th = _mpc_threads_ng_engine_self();
	res = __thread_sigmask(th, how, newmask, oldmask);
#endif
	return res;
}

static int _mpc_threads_ng_engine_sigsuspend(const sigset_t *mask)
{
	/*
	 *    ERRORS:
	 *    EINTR  The call was interrupted by a signal
	 *    EFAULT |>NOT IMPLEMENTED<| mask points to memory which is not a valid
	 *               part of the process address space
	 */

#ifdef WINDOWS_SYS
	return 0;
#endif
	_mpc_threads_ng_engine_p_t *th = _mpc_threads_ng_engine_self();
	sigset_t oldmask;
	sigset_t pending;
	int      i;

	th->attr.nb_sig_treated = 0;
	__thread_sigmask(th, SIG_SETMASK, mask, &oldmask);

	while(th->attr.nb_sig_treated == 0)
	{
		_mpc_threads_ng_engine_sched_yield(&(th->sched) );
		sigpending(&pending);

		for(i = 0; i < SCTK_NSIG; i++)
		{
			if( (sigismember( (sigset_t *)&(th->attr.thread_sigset), i + 1) == 1) &&
			    (sigismember(&pending, i + 1) == 1) )
			{
				_mpc_thread_kthread_kill(_mpc_thread_kthread_self(), i + 1);
				th->attr.nb_sig_treated = 1;
			}
		}
	}

	__thread_sigmask(th, SIG_SETMASK, &oldmask, NULL);
	errno = EINTR;

	return -1;
}

/***************************************/
/* CONDITIONS                          */
/***************************************/

static int _mpc_threads_ng_engine_condattr_destroy(mpc_thread_condattr_t *pattr)
{
	_mpc_threads_ng_engine_condattr_t *attr = (_mpc_threads_ng_engine_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	return 0;
}

static int _mpc_threads_ng_engine_condattr_getpshared(mpc_thread_condattr_t *pattr,
                                                    int *pshared)
{
	_mpc_threads_ng_engine_condattr_t *attr = (_mpc_threads_ng_engine_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL || pshared == NULL)
	{
		return EINVAL;
	}

	(*pshared) = ( (attr->attrs >> 2) & 1);

	return 0;
}

static int _mpc_threads_ng_engine_condattr_init(mpc_thread_condattr_t *pattr)
{
	_mpc_threads_ng_engine_condattr_t *attr = (_mpc_threads_ng_engine_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 *    ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the condition
	 *               variable attributes object
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	attr->attrs &= ~(1 << 2);
	attr->attrs  = ( (attr->attrs & ~3) | (0 & 3) );

	return 0;
}

static int _mpc_threads_ng_engine_condattr_setpshared(mpc_thread_condattr_t *pattr,
                                                    int pshared)
{
	_mpc_threads_ng_engine_condattr_t *attr = (_mpc_threads_ng_engine_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}
	if(pshared != SCTK_THREAD_PROCESS_PRIVATE && pshared != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}

	int ret = 0;
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		attr->attrs |= (1 << 2);
		fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared conds\n");
		ret = ENOTSUP;
	}
	else
	{
		attr->attrs &= ~(1 << 2);
	}

	return ret;
}

static int _mpc_threads_ng_engine_condattr_setclock(mpc_thread_condattr_t *pattr,
                                                  clockid_t clock_id)
{
	_mpc_threads_ng_engine_condattr_t *attr = (_mpc_threads_ng_engine_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}
	if(clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC &&
	   clock_id != CLOCK_PROCESS_CPUTIME_ID && clock_id != CLOCK_THREAD_CPUTIME_ID)
	{
		return EINVAL;
	}
	if(clock_id == CLOCK_PROCESS_CPUTIME_ID || clock_id == CLOCK_THREAD_CPUTIME_ID)
	{
		return EINVAL;
	}

	attr->attrs = ( (attr->attrs & ~3) | (clock_id & 3) );

	return 0;
}

static int _mpc_threads_ng_engine_condattr_getclock(mpc_thread_condattr_t *pattr,
                                                  clockid_t *clock_id)
{
	_mpc_threads_ng_engine_condattr_t *attr = (_mpc_threads_ng_engine_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL || clock_id == NULL)
	{
		return EINVAL;
	}

	(*clock_id) = (attr->attrs & 3);

	return 0;
}

static int _mpc_threads_ng_engine_cond_destroy(mpc_thread_cond_t *plock)
{
	_mpc_threads_ng_engine_cond_t *lock = (_mpc_threads_ng_engine_cond_t *)plock;

	/*
	 *    ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 *    EBUSY  The lock argument is currently used
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}
	if(lock->blocked != NULL)
	{
		return EBUSY;
	}

	lock->clock_id = -1;

	return 0;
}

static int _mpc_threads_ng_engine_cond_init(mpc_thread_cond_t *plock,
                                          const mpc_thread_condattr_t *pattr)
{
	_mpc_threads_ng_engine_cond_t *    lock = (_mpc_threads_ng_engine_cond_t *)plock;
	_mpc_threads_ng_engine_condattr_t *attr = (_mpc_threads_ng_engine_condattr_t *)pattr;

	/*
	 *    ERRORS:
	 *    EAGAIN |>NOT IMPLEMENTED<| The system lacked the necessary resources
	 *               (other than memory) to initialize another condition variable
	 *    ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize
	 *               the condition variable
	 *    EBUSY  |>NOT IMPLEMENTED<| The argument lock is already initialize
	 *               and must be destroy before reinitializing it
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	_mpc_threads_ng_engine_cond_t  local = SCTK_THREAD_GENERIC_COND_INIT;
	_mpc_threads_ng_engine_cond_t *ptrl  = &local;

	if(attr != NULL)
	{
		if( ( (attr->attrs >> 2) & 1) == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared conds\n");
			ret = ENOTSUP;
		}
		ptrl->clock_id = (attr->attrs & 3);
	}

	(*lock) = local;

	return ret;
}

static int _mpc_threads_ng_engine_cond_wait(mpc_thread_cond_t *pcond,
                                          mpc_thread_mutex_t *pmutex)
{
	_mpc_threads_ng_engine_cond_t *     cond  = (_mpc_threads_ng_engine_cond_t *)pcond;
	_mpc_threads_ng_engine_mutex_t *    mutex = (_mpc_threads_ng_engine_mutex_t *)pmutex;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 *    ERRORS:
	 *    EINVAL    The value specified by argument cond, mutex, or time is invalid
	 *                      or different mutexes were supplied for concurrent pthread_cond_wait()
	 *                      operations on the same condition variable
	 *    EPERM     The mutex was not owned by the current thread
	 */

	if(cond == NULL || mutex == NULL)
	{
		return EINVAL;
	}

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	int ret = 0;
	_mpc_threads_ng_engine_cond_cell_t cell;
	void **tmp = (void **)sched->th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table;
	mpc_common_spinlock_lock(&(cond->lock) );
	if(cond->blocked == NULL)
	{
		if(sched != mutex->owner)
		{
			return EPERM;
		}
		cell.binded = mutex;
	}
	else
	{
		if(cond->blocked->binded != mutex)
		{
			return EINVAL;
		}
		if(sched != mutex->owner)
		{
			return EPERM;
		}
		cell.binded = mutex;
	}
	_mpc_threads_ng_engine_mutex_unlock( (mpc_thread_mutex_t *)mutex);
	cell.sched = sched;
	DL_APPEND(cond->blocked, &cell);
	tmp[MPC_THREADS_GENERIC_COND] = (void *)cond;

	mpc_common_nodebug("WAIT on %p", sched);

	_mpc_threads_ng_engine_thread_status(sched, _mpc_threads_ng_engine_blocked);
	_mpc_threads_ng_engine_register_spinlock_unlock(sched, &(cond->lock) );
	_mpc_threads_ng_engine_sched_yield(sched);

	tmp[MPC_THREADS_GENERIC_COND] = NULL;
	_mpc_threads_ng_engine_mutex_lock( (mpc_thread_mutex_t *)mutex);

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);
	return ret;
}

static int _mpc_threads_ng_engine_cond_signal(mpc_thread_cond_t *pcond)
{
	_mpc_threads_ng_engine_cond_t *cond = (_mpc_threads_ng_engine_cond_t *)pcond;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(cond == NULL)
	{
		return EINVAL;
	}

	_mpc_threads_ng_engine_cond_cell_t *task;
	mpc_common_spinlock_lock(&(cond->lock) );
	task = cond->blocked;
	if(task != NULL)
	{
		DL_DELETE(cond->blocked, task);
		_mpc_threads_ng_engine_wake(task->sched);
	}
	mpc_common_spinlock_unlock(&(cond->lock) );
	return 0;
}

struct __timedwait_arg_s
{
	const struct timespec *restrict       timedout;
	int *                                 timeout;
	_mpc_threads_ng_engine_scheduler_t *    sched;
	_mpc_threads_ng_engine_cond_t *restrict cond;
};

void __timedwait_arg_init(
        struct __timedwait_arg_s *arg,
        const struct timespec *restrict timedout, int *timeout,
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_cond_t *restrict cond)
{
	arg->timedout = timedout;
	arg->timeout  = timeout;
	arg->sched    = sched;
	arg->cond     = cond;
}

void __timedwait_task_init(_mpc_threads_ng_engine_task_t *task,
                           volatile int *data, int value,
                           void (*func)(void *), void *arg)
{
	task->is_blocking = 0;
	task->data        = data;
	task->value       = value;
	task->func        = func;
	task->arg         = arg;
	task->sched       = NULL;
	task->free_func   = sctk_free;
	task->prev        = NULL;
	task->next        = NULL;
}

static void __timedwait_test_timeout(void *args)
{
	struct __timedwait_arg_s *        arg       = (struct __timedwait_arg_s *)args;
	_mpc_threads_ng_engine_cond_cell_t *lcell     = NULL;
	_mpc_threads_ng_engine_cond_cell_t *lcell_tmp = NULL;
	struct timespec t_current;

	if(mpc_common_spinlock_trylock(&(arg->cond->lock) ) == 0)
	{
		clock_gettime(arg->cond->clock_id, &t_current);

		if(t_current.tv_sec > arg->timedout->tv_sec ||
		   (t_current.tv_sec == arg->timedout->tv_sec && t_current.tv_nsec > arg->timedout->tv_nsec) )
		{
			DL_FOREACH_SAFE(arg->cond->blocked, lcell, lcell_tmp)
			{
				if(lcell->sched == arg->sched)
				{
					*(arg->timeout) = 1;
					DL_DELETE(arg->cond->blocked, lcell);
					lcell->next = NULL;
					_mpc_threads_ng_engine_wake(lcell->sched);
				}
			}
		}
		mpc_common_spinlock_unlock(&(arg->cond->lock) );
	}
}

static int _mpc_threads_ng_engine_cond_timedwait(mpc_thread_cond_t *pcond,
                                               mpc_thread_mutex_t *pmutex,
                                               const struct timespec *restrict time)
{
	_mpc_threads_ng_engine_cond_t *     cond  = (_mpc_threads_ng_engine_cond_t *)pcond;
	_mpc_threads_ng_engine_mutex_t *    mutex = (_mpc_threads_ng_engine_mutex_t *)pmutex;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 *    ERRORS:
	 *    EINVAL    The value specified by argument cond, mutex, or time is invalid
	 *                  or different mutexes were supplied for concurrent pthread_cond_timedwait() or
	 *                      pthread_cond_wait() operations on the same condition variable
	 *    EPERM     The mutex was not owned by the current thread
	 *    ETIMEDOUT The time specified by time has passed
	 *    ENOMEM    Lack of memory
	 */

	if(cond == NULL || mutex == NULL || time == NULL)
	{
		return EINVAL;
	}
	if(time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
	{
		return EINVAL;
	}
	if(sched != mutex->owner)
	{
		return EPERM;
	}

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	int             timeout = 0;
	struct timespec t_current;
	_mpc_threads_ng_engine_thread_status_t *status;
	_mpc_threads_ng_engine_cond_cell_t      cell;
	_mpc_threads_ng_engine_task_t *         cond_timedwait_task;
	struct __timedwait_arg_s *            args;
	void **tmp = (void **)sched->th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table;

	cond_timedwait_task = (_mpc_threads_ng_engine_task_t *)sctk_malloc(sizeof(_mpc_threads_ng_engine_task_t) );
	if(cond_timedwait_task == NULL)
	{
		return ENOMEM;
	}
	args = (struct __timedwait_arg_s *)sctk_malloc(sizeof(struct __timedwait_arg_s) );
	if(args == NULL)
	{
		return ENOMEM;
	}

	mpc_common_spinlock_lock(&(cond->lock) );
	if(cond->blocked == NULL)
	{
		if(sched != mutex->owner)
		{
			return EPERM;
		}
		cell.binded = mutex;
	}
	else
	{
		if(cond->blocked->binded != mutex)
		{
			return EINVAL;
		}
		if(sched != mutex->owner)
		{
			return EPERM;
		}
		cell.binded = mutex;
	}
	_mpc_threads_ng_engine_mutex_unlock( (mpc_thread_mutex_t *)mutex);
	cell.sched = sched;
	DL_APPEND(cond->blocked, &cell);
	tmp[MPC_THREADS_GENERIC_COND] = (void *)cond;

	mpc_common_nodebug("WAIT on %p", sched);

	status = (_mpc_threads_ng_engine_thread_status_t *)&(sched->status);

	__timedwait_arg_init(args,
	                     time, &timeout, sched, cond);

	__timedwait_task_init(cond_timedwait_task,
	                      (volatile int *)status,
	                      _mpc_threads_ng_engine_running,
	                      __timedwait_test_timeout,
	                      (void *)args);

	clock_gettime(cond->clock_id, &t_current);
	if(t_current.tv_sec > time->tv_sec ||
	   (t_current.tv_sec == time->tv_sec && t_current.tv_nsec > time->tv_nsec) )
	{
		sctk_free(args);
		sctk_free(cond_timedwait_task);
		DL_DELETE(cond->blocked, &cell);
		tmp[MPC_THREADS_GENERIC_COND] = NULL;
		_mpc_threads_ng_engine_mutex_lock( (mpc_thread_mutex_t *)mutex);
		mpc_common_spinlock_unlock(&(cond->lock) );
		return ETIMEDOUT;
	}

	_mpc_threads_ng_engine_thread_status(sched, _mpc_threads_ng_engine_blocked);
	_mpc_threads_ng_engine_register_spinlock_unlock(sched, &(cond->lock) );
	_mpc_threads_ng_engine_add_task(cond_timedwait_task);
	_mpc_threads_ng_engine_sched_yield(sched);

	tmp[MPC_THREADS_GENERIC_COND] = NULL;
	_mpc_threads_ng_engine_mutex_lock( (mpc_thread_mutex_t *)mutex);

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);
	if(timeout)
	{
		return ETIMEDOUT;
	}

	return 0;
}

static int _mpc_threads_ng_engine_cond_broadcast(mpc_thread_cond_t *pcond)
{
	_mpc_threads_ng_engine_cond_t *cond = (_mpc_threads_ng_engine_cond_t *)pcond;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(cond == NULL)
	{
		return EINVAL;
	}

	_mpc_threads_ng_engine_cond_cell_t *task;
	_mpc_threads_ng_engine_cond_cell_t *task_tmp;
	mpc_common_spinlock_lock(&(cond->lock) );
	DL_FOREACH_SAFE(cond->blocked, task, task_tmp)
	{
		DL_DELETE(cond->blocked, task);
		mpc_common_nodebug("ADD BCAST cond wake %p from %p", task->sched, sched);
		_mpc_threads_ng_engine_wake(task->sched);
	}
	mpc_common_spinlock_unlock(&(cond->lock) );
	return 0;
}

/* Init */


static inline void __cond_init()
{
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_cond_t, mpc_thread_cond_t);
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_condattr_t, mpc_thread_condattr_t);

	{
		static _mpc_threads_ng_engine_cond_t loc  = SCTK_THREAD_GENERIC_COND_INIT;
		static mpc_thread_cond_t           glob = SCTK_THREAD_COND_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(_mpc_threads_ng_engine_cond_t) ) == 0);
	}
}

/***************************************/
/* SEMAPHORES                          */
/***************************************/

/* Semaphore Interface */

static int _mpc_threads_ng_engine_sem_init(mpc_thread_sem_t *psem, int pshared, unsigned int value)
{
	_mpc_threads_ng_engine_sem_t *sem = (_mpc_threads_ng_engine_sem_t *)psem;

	/*
	 *    ERRORS:
	 *    EINVAL value exceeds SEM_VALUE_MAX
	 *    ENOSYS pshared is nonzero, but the system does not support
	 *               process-shared semaphores
	 */

	if(value > SCTK_SEM_VALUE_MAX)
	{
		errno = EINVAL;
		return -1;
	}

	_mpc_threads_ng_engine_sem_t  local     = SCTK_THREAD_GENERIC_SEM_INIT;
	_mpc_threads_ng_engine_sem_t *local_ptr = &local;

	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		errno = ENOTSUP;
		return -1;
	}

	local_ptr->lock = value;
	(*sem)          = local;

	return 0;
}

static int _mpc_threads_ng_engine_sem_wait(mpc_thread_sem_t *psem)
{
	_mpc_threads_ng_engine_sem_t *      sem   = (_mpc_threads_ng_engine_sem_t *)psem;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 *    EINTR  |>NOT IMPLEMENTED<| The call was interrupted by a signal handler
	 */

	if(sem == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	int    ret = 0;
	void **tmp = (void **)sched->th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table;
	_mpc_threads_ng_engine_mutex_cell_t cell;

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	mpc_common_spinlock_lock(&(sem->spinlock) );
	if(sem->lock > 0)
	{
		sem->lock--;
		mpc_common_spinlock_unlock(&(sem->spinlock) );
		return ret;
	}

	cell.sched = sched;
	DL_APPEND(sem->list, &cell);
	_mpc_threads_ng_engine_thread_status(sched, _mpc_threads_ng_engine_blocked);
	mpc_common_nodebug("WAIT SEM LOCK sleep %p", sched);
	tmp[MPC_THREADS_GENERIC_SEM] = (void *)sem;
	_mpc_threads_ng_engine_register_spinlock_unlock(sched, &(sem->spinlock) );
	_mpc_threads_ng_engine_sched_yield(sched);
	tmp[MPC_THREADS_GENERIC_SEM] = NULL;

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	return ret;
}

static int _mpc_threads_ng_engine_sem_trywait(mpc_thread_sem_t *psem)
{
	_mpc_threads_ng_engine_sem_t *sem = (_mpc_threads_ng_engine_sem_t *)psem;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 *    EAGAIN The operation could not be performed without blocking
	 */

	if(sem == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	int ret = 0;

	if(mpc_common_spinlock_trylock(&(sem->spinlock) ) == 0)
	{
		if(sem->lock > 0)
		{
			sem->lock--;
		}
		else
		{
			errno = EAGAIN;
			ret   = -1;
		}
		mpc_common_spinlock_unlock(&(sem->spinlock) );
	}
	else
	{
		errno = EAGAIN;
		ret   = -1;
	}

	return ret;
}

static int _mpc_threads_ng_engine_sem_timedwait(mpc_thread_sem_t *psem,
                                              const struct timespec *time)
{
	_mpc_threads_ng_engine_sem_t *      sem   = (_mpc_threads_ng_engine_sem_t *)psem;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct or the value
	 *               of nsecs in time argument is less than 0, or greater than or equal
	 *               to 1000 million
	 *    ETIMEDOUT The call timed out before the semaphore could be locked
	 *    EINTR  |>NOT IMPLEMENTED<| The call was interrupted by a signal handler
	 */

	if(sem == NULL || time == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if(time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
	{
		errno = EINVAL;
		return -1;
	}

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	int             ret = 0;
	struct timespec t_current;

	do
	{
		if(mpc_common_spinlock_trylock(&(sem->spinlock) ) == 0)
		{
			if(sem->lock > 0)
			{
				sem->lock--;
				errno = 0;
				ret   = 0;
				mpc_common_spinlock_unlock(&(sem->spinlock) );
				return ret;
			}
			else
			{
				errno = EAGAIN;
				ret   = -1;
			}
			mpc_common_spinlock_unlock(&(sem->spinlock) );
		}
		else
		{
			errno = EAGAIN;
			ret   = -1;
		}
		/* test cancel */
		_mpc_threads_ng_engine_check_signals(0);
		clock_gettime(CLOCK_REALTIME, &t_current);
	} while(ret != 0 && (t_current.tv_sec < time->tv_sec ||
	                     (t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec) ) );

	if(ret != 0)
	{
		errno = ETIMEDOUT;
	}

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	if(sched->th->attr.nb_sig_pending > 0)
	{
		errno = EINTR;
	}

	return ret;
}

static int _mpc_threads_ng_engine_sem_post(mpc_thread_sem_t *psem)
{
	_mpc_threads_ng_engine_sem_t *sem = (_mpc_threads_ng_engine_sem_t *)psem;

	/*
	 *    ERRORS:
	 *    EINVAL     The value specified for the argument is not correct
	 *    EOVERFLOW  The maximum allowable value for a semaphore would be exceeded
	 */

	if(sem == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	int ret = 0;
	_mpc_threads_ng_engine_mutex_cell_t *head;

	mpc_common_spinlock_lock(&(sem->spinlock) );
	if(sem->list != NULL)
	{
		head = sem->list;
		DL_DELETE(sem->list, head);
		if(head->sched->status != _mpc_threads_ng_engine_running)
		{
			mpc_common_nodebug("ADD SEM UNLOCK wake %p", head->sched);
			_mpc_threads_ng_engine_wake(head->sched);
		}
	}
	else
	{
		sem->lock++;
		if(sem->lock > SCTK_SEM_VALUE_MAX)
		{
			errno = EOVERFLOW;
			ret   = -1;
		}
	}
	mpc_common_spinlock_unlock(&(sem->spinlock) );

	return ret;
}

static int _mpc_threads_ng_engine_sem_getvalue(mpc_thread_sem_t *psem, int *sval)
{
	_mpc_threads_ng_engine_sem_t *sem = (_mpc_threads_ng_engine_sem_t *)psem;

	/*
	 *    ERRORS:
	 *    EINVAL     The value specified for the argument is not correct
	 */

	if(sem == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	(*sval) = sem->lock;

	return 0;
}

static int _mpc_threads_ng_engine_sem_destroy(mpc_thread_sem_t *psem)
{
	_mpc_threads_ng_engine_sem_t *sem = (_mpc_threads_ng_engine_sem_t *)psem;

	/*
	 *    ERRORS:
	 *    EINVAL     The value specified for the argument is not correct
	 *    EBUSY      Threads are currently waiting on the semaphore argument
	 */

	if(sem == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if(sem->list != NULL)
	{
		errno = EBUSY;
		return -1;
	}

	return 0;
}

/* Named semaphore globals */

static mpc_common_spinlock_t __named_semaphore_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static _mpc_threads_ng_engine_sem_named_list_t *__named_semaphore_list = NULL;

/* Named semaphore Interface */

static mpc_thread_sem_t *_mpc_threads_ng_engine_sem_open(const char *name, int oflag, ...)
{
	/*
	 *    ERRORS:
	 *    EACCES The semaphore exists, but the caller does not have permission to open it
	 *    EEXIST Both O_CREAT and O_EXCL were specified in oflag, but a semaphore
	 *               with this name already exists
	 *    EINVAL value was greater than SEM_VALUE_MAX or name consists of just "/",
	 *               followed by no other characters
	 *    EMFILE |>NOT IMPLEMENTED<| The process already has the maximum number of files
	 *               and open
	 *    ENAMETOOLONG name was too long
	 *    ENFILE |>NOT IMPLEMENTED<| The system limit on the total number of open files
	 *               has been reached
	 *    ENOENT The O_CREAT flag was not specified in oflag and no semaphore with
	 *               this name exists; or, O_CREAT was specified, but name wasn't well formed
	 *    ENOMEM Insufficient memory
	 */

	if(name == NULL)
	{
		errno = EINVAL;
		return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
	}

	if(name[0] != '/' || (name[0] == '/' && name[1] == '\0') )
	{
		errno = EINVAL;
		return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
	}

	int length = strlen(name);

	if(length > NAME_MAX - 4)
	{
		errno = ENAMETOOLONG;
		return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
	}

	char *_name = (char *)sctk_malloc(length * sizeof(char) );
	if(_name == NULL)
	{
		errno = ENOMEM;
		return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
	}

	strncpy(_name, name + 1, length);
	_name[length] = '\0';

	_mpc_threads_ng_engine_sem_t *           sem_tmp;
	_mpc_threads_ng_engine_sem_named_list_t *sem_named_tmp;

	mpc_common_spinlock_lock(&__named_semaphore_lock);
	_mpc_threads_ng_engine_sem_named_list_t *list = __named_semaphore_list;

	if(list != NULL)
	{
		DL_FOREACH(__named_semaphore_list, list)
		{
			if(strcmp(list->name, _name) == 0 &&
			   list->unlink == 0)
			{
				break;
			}
		}
	}

	if(list != NULL && ( (oflag & O_CREAT) &&
	                     (oflag & O_EXCL) ) )
	{
		mpc_common_spinlock_unlock(&__named_semaphore_lock);
		sctk_free(_name);
		errno = EEXIST;
		return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
	}

	if(list != NULL)
	{
		if(oflag & O_CREAT)
		{
			va_list args;
			va_start(args, oflag);
			mode_t mode = va_arg(args, mode_t);
			va_end(args);
			if( (list->mode & mode) != mode)
			{
				sctk_free(_name);
				errno = EACCES;
				mpc_common_spinlock_unlock(&__named_semaphore_lock);
				return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
			}
		}
		list->nb++;
		mpc_common_spinlock_unlock(&__named_semaphore_lock);
		sctk_free(_name);
		return (mpc_thread_sem_t *)list->sem;
	}
	else if(oflag & O_CREAT)
	{
		va_list args;
		va_start(args, oflag);
		mode_t       mode  = va_arg(args, mode_t);
		unsigned int value = va_arg(args, int);
		va_end(args);

		sem_tmp = (_mpc_threads_ng_engine_sem_t *)
		          sctk_malloc(sizeof(_mpc_threads_ng_engine_sem_t) );

		if(sem_tmp == NULL)
		{
			errno = ENOMEM;
			sctk_free(_name);
			mpc_common_spinlock_unlock(&__named_semaphore_lock);
			return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
		}

		sem_named_tmp = (_mpc_threads_ng_engine_sem_named_list_t *)
		                sctk_malloc(sizeof(_mpc_threads_ng_engine_sem_named_list_t) );

		if(sem_named_tmp == NULL)
		{
			errno = ENOMEM;
			sctk_free(_name);
			sctk_free(sem_tmp);
			mpc_common_spinlock_unlock(&__named_semaphore_lock);
			return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
		}

		int ret = _mpc_threads_ng_engine_sem_init( (mpc_thread_sem_t *)sem_tmp, 0, value);

		if(ret != 0)
		{
			mpc_common_spinlock_unlock(&__named_semaphore_lock);
			return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
		}

		sem_named_tmp->name   = _name;
		sem_named_tmp->nb     = 1;
		sem_named_tmp->unlink = 0;
		sem_named_tmp->mode   = mode;
		sem_named_tmp->sem    = sem_tmp;

		DL_APPEND(__named_semaphore_list, sem_named_tmp);

		mpc_common_spinlock_unlock(&__named_semaphore_lock);
	}
	else
	{
		sctk_free(_name);
		errno = ENOENT;
		mpc_common_spinlock_unlock(&__named_semaphore_lock);
		return (mpc_thread_sem_t *)SCTK_SEM_FAILED;
	}

	mpc_common_nodebug("sem vaux %p avec une valeur de %d", sem_named_tmp->sem, sem_named_tmp->sem->lock);

	return (mpc_thread_sem_t *)sem_named_tmp->sem;
}

static int _mpc_threads_ng_engine_sem_close(mpc_thread_sem_t *psem)
{
	_mpc_threads_ng_engine_sem_t *sem = (_mpc_threads_ng_engine_sem_t *)psem;

	/*
	 *    ERRORS:
	 *    EINVAL     The value specified for the argument is not correct
	 */

	if(sem == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	mpc_common_spinlock_lock(&__named_semaphore_lock);
	_mpc_threads_ng_engine_sem_named_list_t *list = __named_semaphore_list;

	if(list != NULL)
	{
		DL_FOREACH(__named_semaphore_list, list)
		{
			if(sem == list->sem)
			{
				break;
			}
		}
	}

	if(list == NULL)
	{
		errno = EINVAL;
		mpc_common_spinlock_unlock(&__named_semaphore_lock);
		return -1;
	}

	list->nb--;

	if(list->nb == 0 && list->unlink != 0)
	{
		DL_DELETE(__named_semaphore_list, list);
		sctk_free(list->sem);
		sctk_free(list);
	}

	mpc_common_spinlock_unlock(&__named_semaphore_lock);

	return 0;
}

static int _mpc_threads_ng_engine_sem_unlink(const char *name)
{
	/*
	 *    ERRORS:
	 *    EACCES  |>NOT IMPLEMENTED<| The caller does not have permission to unlink this semaphore
	 *    ENAMETOOLONG  name was too long
	 *    ENOENT  There is no semaphore with the given name
	 */

	int length;

	if(name == NULL)
	{
		errno = ENOENT;
		return -1;
	}

	length = strlen(name);

	if(length > NAME_MAX - 4)
	{
		errno = ENAMETOOLONG;
		return -1;
	}

	mpc_common_spinlock_lock(&__named_semaphore_lock);
	_mpc_threads_ng_engine_sem_named_list_t *list = __named_semaphore_list;

	if(list != NULL)
	{
		DL_FOREACH(__named_semaphore_list, list)
		{
			if(strcmp(list->name, name + 1) == 0 &&
			   list->unlink == 0)
			{
				break;
			}
		}
	}

	if(list == NULL)
	{
		errno = ENOENT;
		mpc_common_spinlock_unlock(&__named_semaphore_lock);
		return -1;
	}
	else if(list->nb == 0)
	{
		DL_DELETE(__named_semaphore_list, list);
		sctk_free(list->sem);
		sctk_free(list);
	}
	else
	{
		list->unlink = 1;
	}

	mpc_common_spinlock_unlock(&__named_semaphore_lock);

	return 0;
}

/* Semaphore Init */


static inline void __semaphore_init()
{
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_sem_t, mpc_thread_cond_t);

	{
		static _mpc_threads_ng_engine_sem_t loc = SCTK_THREAD_GENERIC_SEM_INIT;
		static mpc_thread_sem_t           glob;
		assume(memcmp(&loc, &glob, sizeof(_mpc_threads_ng_engine_sem_t) ) == 0);
	}
}

/***************************************/
/* READ/WRITE LOCKS                    */
/***************************************/

/* Common Helpers */

static inline int __rwlock_store(_mpc_threads_ng_engine_rwlock_t *_rwlock,
                                 _mpc_threads_ng_engine_scheduler_t *sched)
{
	/*
	 *      ERRORS:
	 *  ENOMEM Insufficient memory exists to initialize the structure used for
	 *                     the hash table
	 */

	mpc_thread_rwlock_in_use_t *tmp = (mpc_thread_rwlock_in_use_t *)
	                                  sctk_malloc(sizeof(mpc_thread_rwlock_in_use_t) );

	if(tmp == NULL)
	{
		return ENOMEM;
	}

	tmp->rwlock = _rwlock;
	HASH_ADD(hh, (sched->th->attr.rwlocks_owned), rwlock, sizeof(void *), tmp);

	return 0;
}

static inline void __rwlock_init_cell(_mpc_threads_ng_engine_rwlock_cell_t *cell)
{
	cell->sched = NULL;
	cell->type  = -1;
	cell->prev  = NULL;
	cell->next  = NULL;
}

static inline int __rwlock_retrieve(_mpc_threads_ng_engine_rwlock_t *_rwlock,
                                    _mpc_threads_ng_engine_scheduler_t *sched)
{
	/*
	 *      ERRORS:
	 *  ENOMEM Insufficient memory exists to initialize the structure used for
	 *                     the hash table
	 */

	mpc_thread_rwlock_in_use_t *tmp;

	HASH_FIND(hh, (sched->th->attr.rwlocks_owned), (&_rwlock), sizeof(void *), tmp);

	if(tmp == NULL)
	{
		return EPERM;
	}

	HASH_DELETE(hh, (sched->th->attr.rwlocks_owned), tmp);

	return 0;
}

static inline int __rwlock_lock(_mpc_threads_ng_engine_rwlock_t *lock,
                                unsigned int type,
                                _mpc_threads_ng_engine_scheduler_t *sched)
{
	/*
	 * ERRORS:
	 * EINVAL  The value specified for the argument is not correct
	 * EDEADLK The current thread already owns the lock for writing
	 * EBUSY   The lock is not avaible for a tryrdlock or trywrlock type
	 * EAGAIN  |> NOT IMPLEMENTED <| The read lock could not be acquired
	 *         because the maximum number of read locks for rwlock has been exceeded
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}
	if(lock->status != SCTK_INITIALIZED)
	{
		return EINVAL;
	}

	_mpc_threads_ng_engine_rwlock_cell_t cell;
	__rwlock_init_cell(&cell);
	int    ret;
	void **tmp = (void **)sched->th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table;

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	unsigned int ltype = type;
	if(type != SCTK_RWLOCK_READ && type != SCTK_RWLOCK_WRITE)
	{
		if(type == SCTK_RWLOCK_TRYREAD)
		{
			if(mpc_common_spinlock_trylock(&(lock->lock) ) )
			{
				return EBUSY;
			}
			ltype = SCTK_RWLOCK_READ;
		}
		else if(type == SCTK_RWLOCK_TRYWRITE)
		{
			if(mpc_common_spinlock_trylock(&(lock->lock) ) )
			{
				return EBUSY;
			}
			ltype = SCTK_RWLOCK_WRITE;
		}
		else
		{
			return EINVAL;
		}
	}
	else
	{
		mpc_common_spinlock_lock(&(lock->lock) );
	}
	lock->count++;

	mpc_common_nodebug(" rwlock : \nlock = %d\ncurrent = %d\nwait = %d\ntype : %d\n",
	             lock->lock, lock->current, lock->wait, type);

	if(ltype == SCTK_RWLOCK_READ)
	{
		if( (lock->current == SCTK_RWLOCK_READ || lock->current == SCTK_RWLOCK_ALONE) &&
		    lock->wait == SCTK_RWLOCK_NO_WR_WAITING)
		{
			lock->current = SCTK_RWLOCK_READ;
			lock->reads_count++;

			/* We add the lock to a hash table owned by the current thread*/
			if( (ret = __rwlock_store(lock, sched) ) != 0)
			{
				lock->count--;
				mpc_common_spinlock_unlock(&(lock->lock) );
				return ret;
			}

			mpc_common_spinlock_unlock(&(lock->lock) );
			return 0;
		}
		else if(sched == lock->writer)
		{
			lock->count--;
			mpc_common_spinlock_unlock(&(lock->lock) );
			return EDEADLK;
		}
		/* We cannot take the lock for reading and we prepare to block */
	}
	else if(ltype == SCTK_RWLOCK_WRITE)
	{
		if(lock->current == SCTK_RWLOCK_ALONE &&
		   lock->wait == SCTK_RWLOCK_NO_WR_WAITING)
		{
			lock->current = SCTK_RWLOCK_WRITE;
			lock->writer  = sched;

			/* We add the lock to a hash table owned by the current thread*/
			if( (ret = __rwlock_store(lock, sched) ) != 0)
			{
				lock->count--;
				mpc_common_spinlock_unlock(&(lock->lock) );
				return ret;
			}

			mpc_common_spinlock_unlock(&(lock->lock) );
			return 0;
		}
		else if(sched == lock->writer)
		{
			lock->count--;
			mpc_common_spinlock_unlock(&(lock->lock) );
			return EDEADLK;
		}
		lock->wait = SCTK_RWLOCK_WR_WAITING;
		/* We cannot take the lock for writing and we prepare to block */
	}
	else
	{
		not_reachable();
	}

	/* We test if we were in a tryread or in a trywrite */
	if(type != SCTK_RWLOCK_READ && type != SCTK_RWLOCK_WRITE)
	{
		lock->count--;
		mpc_common_spinlock_unlock(&(lock->lock) );
		return EBUSY;
	}

	/* We add the lock to a hash table owned by the current thread*/
	if( (ret = __rwlock_store(lock, sched) ) != 0)
	{
		lock->count--;
		mpc_common_spinlock_unlock(&(lock->lock) );
		return ret;
	}

	cell.sched = sched;
	cell.type  = ltype;
	DL_APPEND(lock->waiting, &cell);
	mpc_common_nodebug("blocked on %p", lock);

	_mpc_threads_ng_engine_thread_status(sched, _mpc_threads_ng_engine_blocked);
	mpc_common_nodebug("WAIT RWLOCK LOCK sleep %p", sched);
	tmp[MPC_THREADS_GENERIC_RWLOCK] = (void *)lock;
	_mpc_threads_ng_engine_register_spinlock_unlock(sched, &(lock->lock) );
	_mpc_threads_ng_engine_sched_yield(sched);
	tmp[MPC_THREADS_GENERIC_RWLOCK] = NULL;

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);
	return 0;
}

/* Interface for RW Locks */

static int _mpc_threads_ng_engine_rwlockattr_destroy(mpc_thread_rwlockattr_t *pattr)
{
	_mpc_threads_ng_engine_rwlockattr_t *attr = (_mpc_threads_ng_engine_rwlockattr_t *)pattr;

	/*
	 *      ERRORS:
	 *  EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	return 0;
}

static int _mpc_threads_ng_engine_rwlockattr_getpshared(const mpc_thread_rwlockattr_t *pattr, int *val)
{
	_mpc_threads_ng_engine_rwlockattr_t *attr = (_mpc_threads_ng_engine_rwlockattr_t *)pattr;

	/*
	 *  ERRORS:
	 *  EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL || val == NULL)
	{
		return EINVAL;
	}

	(*val) = attr->pshared;

	return 0;
}

static int _mpc_threads_ng_engine_rwlockattr_init(mpc_thread_rwlockattr_t *pattr)
{
	_mpc_threads_ng_engine_rwlockattr_t *attr = (_mpc_threads_ng_engine_rwlockattr_t *)pattr;

	/*
	 * ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 * ENOMEM |> NOT IMPLEMENTED <| Insufficient memory exists to initialize
	 *        the read-write lock attributes object
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;

	return 0;
}

static int _mpc_threads_ng_engine_rwlockattr_setpshared(mpc_thread_rwlockattr_t *pattr, int val)
{
	_mpc_threads_ng_engine_rwlockattr_t *attr = (_mpc_threads_ng_engine_rwlockattr_t *)pattr;

	/*
	 * ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}
	if(val != SCTK_THREAD_PROCESS_PRIVATE && val != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}

	int ret = 0;
	if(val == SCTK_THREAD_PROCESS_SHARED)
	{
		fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared rwlokcs\n");
		ret = ENOTSUP;
	}

	attr->pshared = val;

	return ret;
}

static int _mpc_threads_ng_engine_rwlock_destroy(mpc_thread_rwlock_t *plock)
{
	_mpc_threads_ng_engine_rwlock_t *lock = (_mpc_threads_ng_engine_rwlock_t *)plock;

	/*
	 * ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 * EBUSY  The specified lock is currently owned by a thread
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}
	if(lock->count > 0)
	{
		return EBUSY;
	}

	lock->status = SCTK_DESTROYED;

	return 0;
}

static int _mpc_threads_ng_engine_rwlock_init(mpc_thread_rwlock_t *plock, mpc_thread_rwlockattr_t *pattr)
{
	_mpc_threads_ng_engine_rwlock_t *    lock = (_mpc_threads_ng_engine_rwlock_t *)plock;
	_mpc_threads_ng_engine_rwlockattr_t *attr = (_mpc_threads_ng_engine_rwlockattr_t *)pattr;

	/*
	 *      ERRORS:
	 *  EINVAL The value specified for the argument is not correct
	 *  EAGAIN |> NOT IMPLEMENTED <| The system lacked the necessary resources
	 *         (other than memory) to initialize another read-write lock
	 *  ENOMEM |> NOT IMPLEMENTED <| Insufficient memory exists to initialize
	 *         the read-write lock
	 *  EPERM  |> NOT IMPLEMENTED <| The caller does not have the privilege
	 *         to perform the operation
	 *  EBUSY  The implementation has detected an attempt to reinitialize
	 *         the object referenced by lock, a previously initialized
	 *         but not yet destroyed read-write lock
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}
	if(lock->status == SCTK_INITIALIZED)
	{
		return EBUSY;
	}

	int ret = 0;
	_mpc_threads_ng_engine_rwlock_t local = SCTK_THREAD_GENERIC_RWLOCK_INIT;

	if(attr != NULL)
	{
		if(attr->pshared == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared rwlokcs\n");
			ret = ENOTSUP;
		}
	}

	local.status = SCTK_INITIALIZED;
	(*lock)      = local;

	return ret;
}

static int _mpc_threads_ng_engine_rwlock_rdlock(mpc_thread_rwlock_t *plock)
{
	_mpc_threads_ng_engine_rwlock_t *   lock  = (_mpc_threads_ng_engine_rwlock_t *)plock;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	return __rwlock_lock(lock, SCTK_RWLOCK_READ, sched);
}

static int _mpc_threads_ng_engine_rwlock_wrlock(mpc_thread_rwlock_t *plock)
{
	_mpc_threads_ng_engine_rwlock_t *   lock  = (_mpc_threads_ng_engine_rwlock_t *)plock;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	return __rwlock_lock(lock, SCTK_RWLOCK_WRITE, sched);
}

static int _mpc_threads_ng_engine_rwlock_tryrdlock(mpc_thread_rwlock_t *plock)
{
	_mpc_threads_ng_engine_rwlock_t *   lock  = (_mpc_threads_ng_engine_rwlock_t *)plock;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	return __rwlock_lock(lock, SCTK_RWLOCK_TRYREAD, sched);
}

static int _mpc_threads_ng_engine_rwlock_trywrlock(mpc_thread_rwlock_t *plock)
{
	_mpc_threads_ng_engine_rwlock_t *   lock  = (_mpc_threads_ng_engine_rwlock_t *)plock;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	return __rwlock_lock(lock, SCTK_RWLOCK_TRYWRITE, sched);
}

static int _mpc_threads_ng_engine_rwlock_unlock(mpc_thread_rwlock_t *plock)
{
	_mpc_threads_ng_engine_rwlock_t *   lock  = (_mpc_threads_ng_engine_rwlock_t *)plock;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 * ERRORS:
	 * EINVAL The value specified for the argument is not correct
	 * EPERM The current thread does not hold a lock on the read-write lock
	 */

	if(lock == NULL)
	{
		return EINVAL;
	}
	if(lock->status != SCTK_INITIALIZED)
	{
		return EINVAL;
	}

	int ret = __rwlock_retrieve(lock, sched);
	if(ret != 0)
	{
		return ret;
	}

	mpc_common_spinlock_lock(&(lock->lock) );
	lock->count--;
	if(lock->reads_count > 0)
	{
		lock->reads_count--;
	}

	_mpc_threads_ng_engine_rwlock_cell_t *cell;
	_mpc_threads_ng_engine_scheduler_t *  lsched;
	if(lock->waiting != NULL && lock->reads_count == 0)
	{
		if(lock->waiting->type == SCTK_RWLOCK_WRITE)
		{
			cell = lock->waiting;
			DL_DELETE(lock->waiting, cell);
			lsched        = cell->sched;
			lock->current = SCTK_RWLOCK_WRITE;
			lock->writer  = lsched;
			if(lock->waiting == NULL)
			{
				lock->wait = SCTK_RWLOCK_NO_WR_WAITING;
			}
			if(lsched->status != _mpc_threads_ng_engine_running)
			{
				mpc_common_nodebug("ADD RWLOCK UNLOCK wake %p", lsched);
				_mpc_threads_ng_engine_wake(lsched);
			}
		}
		else if(lock->waiting->type == SCTK_RWLOCK_READ)
		{
			lock->current = SCTK_RWLOCK_READ;
			lock->writer  = NULL;
			while(lock->waiting != NULL && lock->waiting->type == SCTK_RWLOCK_READ)
			{
				cell = lock->waiting;
				DL_DELETE(lock->waiting, cell);
				lsched = cell->sched;
				if(lsched->status != _mpc_threads_ng_engine_running)
				{
					mpc_common_nodebug("ADD RWLOCK UNLOCK wake %p", lsched);
					_mpc_threads_ng_engine_wake(lsched);
				}
				lock->reads_count++;
			}
			if(lock->waiting == NULL)
			{
				lock->wait = SCTK_RWLOCK_NO_WR_WAITING;
			}
		}
		else
		{
			not_reachable();
		}
	}

	if(lock->count == 0)
	{
		lock->wait    = SCTK_RWLOCK_NO_WR_WAITING;
		lock->current = SCTK_RWLOCK_ALONE;
		lock->writer  = NULL;
	}
	mpc_common_spinlock_unlock(&(lock->lock) );

	return 0;
}

static int _mpc_threads_ng_engine_rwlock_timedrdlock(mpc_thread_rwlock_t *plock, const struct timespec *restrict time)
{
	_mpc_threads_ng_engine_rwlock_t *   lock  = (_mpc_threads_ng_engine_rwlock_t *)plock;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 * ERRORS:
	 * EINVAL    The value specified by argument lock does not refer to an initialized
	 *           read-write lock object, or the abs_timeout nanosecond value is less
	 *           than zero or greater than or equal to 1000 million
	 * ETIMEDOUT The lock could not be acquired in the time specified
	 * EAGAIN    |> NOT IMPLEMENTED <| The read lock could not be acquired
	 *           because the maximum number of read locks for lock would be exceeded
	 * EDEADLK   The calling thread already holds a write lock on argument lock
	 */

	if(lock == NULL || time == NULL)
	{
		return EINVAL;
	}
	if(time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
	{
		return EINVAL;
	}

	int             ret;
	struct timespec t_current;

	do
	{
		ret = __rwlock_lock(lock, SCTK_RWLOCK_TRYREAD, sched);
		if(ret != EBUSY)
		{
			return ret;
		}
		clock_gettime(CLOCK_REALTIME, &t_current);
	} while(ret != 0 && (t_current.tv_sec < time->tv_sec ||
	                     (t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec) ) );

	if(ret != 0)
	{
		return ETIMEDOUT;
	}

	return 0;
}

static int _mpc_threads_ng_engine_rwlock_timedwrlock(mpc_thread_rwlock_t *plock, const struct timespec *restrict time)
{
	_mpc_threads_ng_engine_rwlock_t *   lock  = (_mpc_threads_ng_engine_rwlock_t *)plock;
	_mpc_threads_ng_engine_scheduler_t *sched = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 * ERRORS:
	 * EINVAL    The value specified by argument lock does not refer to an initialized
	 *           read-write lock object, or the abs_timeout nanosecond value is less
	 *           than zero or greater than or equal to 1000 million
	 * ETIMEDOUT The lock could not be acquired in the specified time
	 * EDEADLK   The calling thread already holds the argument lock for writing
	 */

	if(lock == NULL || time == NULL)
	{
		return EINVAL;
	}
	if(time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
	{
		return EINVAL;
	}

	int             ret;
	struct timespec t_current;

	do
	{
		ret = __rwlock_lock(lock, SCTK_RWLOCK_TRYWRITE, sched);
		if(ret != EBUSY)
		{
			return ret;
		}
		clock_gettime(CLOCK_REALTIME, &t_current);
	} while(ret != 0 && (t_current.tv_sec < time->tv_sec ||
	                     (t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec) ) );

	if(ret != 0)
	{
		return ETIMEDOUT;
	}

	return 0;
}

static int _mpc_threads_ng_engine_rwlockattr_setkind_np(__UNUSED__ mpc_thread_rwlockattr_t *attr, __UNUSED__ int pref)
{
	not_implemented();
	return 0;
}

static int _mpc_threads_ng_engine_rwlockattr_getkind_np(__UNUSED__ mpc_thread_rwlockattr_t *attr, __UNUSED__ int *pref)
{
	not_implemented();
	return 0;
}

/* Init */


static inline void __rwlock_init()
{
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_rwlock_t, mpc_thread_rwlock_t);
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_rwlockattr_t, mpc_thread_rwlockattr_t);
	{
		static _mpc_threads_ng_engine_rwlock_t loc  = SCTK_THREAD_GENERIC_RWLOCK_INIT;
		static mpc_thread_rwlock_t           glob = SCTK_THREAD_RWLOCK_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(_mpc_threads_ng_engine_rwlock_t) ) == 0);
	}
}

/***************************************/
/* THREAD BARRIER                      */
/***************************************/

static int _mpc_threads_ng_engine_barrierattr_destroy(mpc_thread_barrierattr_t *attr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	return 0;
}

static int _mpc_threads_ng_engine_barrierattr_init(mpc_thread_barrierattr_t *pattr)
{
	_mpc_threads_ng_engine_barrierattr_t *attr = (_mpc_threads_ng_engine_barrierattr_t *)pattr;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 *    ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the barrier
	 *               attributes object
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;

	return 0;
}

static int _mpc_threads_ng_engine_barrierattr_getpshared(const mpc_thread_barrierattr_t *restrict pattr,
                                                       int *restrict pshared)
{
	_mpc_threads_ng_engine_barrierattr_t *attr = (_mpc_threads_ng_engine_barrierattr_t *)pattr;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL || pshared == NULL)
	{
		return EINVAL;
	}

	(*pshared) = attr->pshared;

	return 0;
}

static int _mpc_threads_ng_engine_barrierattr_setpshared(mpc_thread_barrierattr_t *pattr, int pshared)
{
	_mpc_threads_ng_engine_barrierattr_t *attr = (_mpc_threads_ng_engine_barrierattr_t *)pattr;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}
	if(pshared != SCTK_THREAD_PROCESS_PRIVATE &&
	   pshared != SCTK_THREAD_PROCESS_SHARED)
	{
		return EINVAL;
	}

	int ret = 0;
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared barriers\n");
		ret = ENOTSUP;
	}
	attr->pshared = pshared;

	return ret;
}

static int _mpc_threads_ng_engine_barrier_destroy(mpc_thread_barrier_t *pbarrier)
{
	_mpc_threads_ng_engine_barrier_t *barrier = (_mpc_threads_ng_engine_barrier_t *)pbarrier;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct
	 *    EBUSY  The implementation has detected an attempt to destroy a barrier while it is in use
	 */

	if(barrier == NULL)
	{
		return EINVAL;
	}
	if(barrier->blocked != NULL)
	{
		return EBUSY;
	}

	return 0;
}

static int _mpc_threads_ng_engine_barrier_init(mpc_thread_barrier_t *restrict pbarrier,
                                             const mpc_thread_barrierattr_t *restrict pattr, unsigned count)
{
	_mpc_threads_ng_engine_barrier_t *    barrier = (_mpc_threads_ng_engine_barrier_t *)pbarrier;
	_mpc_threads_ng_engine_barrierattr_t *attr    = (_mpc_threads_ng_engine_barrierattr_t *)pattr;

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct or count is equal to zero
	 *    EAGAIN |>NOT IMPLEMENTED<| The system lacks the necessary resources to initialize
	 *               another barrier
	 *    ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the barrier
	 *    EBUSY  |>NOT IMPLEMENTED<| The implementation has detected an attempt to destroy a
	 *               barrier while it is in use
	 */

	if(barrier == NULL || count == 0)
	{
		return EINVAL;
	}

	_mpc_threads_ng_engine_barrier_t  loc     = SCTK_THREAD_GENERIC_BARRIER_INIT;
	_mpc_threads_ng_engine_barrier_t *ptr_loc = &loc;

	int ret = 0;
	if(attr != NULL)
	{
		if(attr->pshared == SCTK_THREAD_PROCESS_SHARED)
		{
			fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared barriers\n");
			ret = ENOTSUP;
		}
	}
	ptr_loc->nb_current = count;
	ptr_loc->nb_max     = count;
	(*barrier)          = loc;

	return ret;
}

static int _mpc_threads_ng_engine_barrier_wait(mpc_thread_barrier_t *pbarrier)
{
	_mpc_threads_ng_engine_barrier_t *  barrier = (_mpc_threads_ng_engine_barrier_t *)pbarrier;
	_mpc_threads_ng_engine_scheduler_t *sched   = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 *    ERRORS:
	 *    EINVAL The value specified for the argument is not correct or count is equal to zero
	 */

	if(barrier == NULL)
	{
		return EINVAL;
	}

	int ret = 0;
	_mpc_threads_ng_engine_barrier_cell_t  cell;
	_mpc_threads_ng_engine_barrier_cell_t *list;
	void **tmp = sched->th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table;

	mpc_common_spinlock_lock(&(barrier->lock) );
	barrier->nb_current--;
	if(barrier->nb_current > 0)
	{
		cell.sched = sched;
		DL_APPEND(barrier->blocked, &cell);
		_mpc_threads_ng_engine_thread_status(sched, _mpc_threads_ng_engine_blocked);
		tmp[MPC_THREADS_GENERIC_BARRIER] = (void *)barrier;
		mpc_common_nodebug("blocked on %p", barrier);
		_mpc_threads_ng_engine_register_spinlock_unlock(sched, &(barrier->lock) );
		_mpc_threads_ng_engine_sched_yield(sched);
		tmp[MPC_THREADS_GENERIC_BARRIER] = NULL;
	}
	else
	{
		list = barrier->blocked;
		while(list != NULL)
		{
			DL_DELETE(barrier->blocked, list);
			if(list->sched->status != _mpc_threads_ng_engine_running)
			{
				mpc_common_nodebug("Sched %p pass barrier %p", list->sched, barrier);
				_mpc_threads_ng_engine_wake(list->sched);
			}
			list = barrier->blocked;
		}
		barrier->nb_current = barrier->nb_max;
		ret = SCTK_THREAD_BARRIER_SERIAL_THREAD;
		mpc_common_spinlock_unlock(&(barrier->lock) );
	}

	return ret;
}

static inline void __barrier_init()
{
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_barrier_t, mpc_thread_barrier_t);
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_barrierattr_t, mpc_thread_barrierattr_t);

	{
		static _mpc_threads_ng_engine_barrier_t loc = SCTK_THREAD_GENERIC_BARRIER_INIT;
		static mpc_thread_barrier_t           glob;
		assume(memcmp(&loc, &glob, sizeof(_mpc_threads_ng_engine_barrier_t) ) == 0);
	}
}

/***************************************/
/* THREAD SPINLOCK                     */
/***************************************/

/* Interface */

static int  _mpc_threads_ng_engine_spin_destroy(mpc_thread_spinlock_t *pspinlock)
{
	_mpc_threads_ng_engine_spinlock_t *spinlock = (_mpc_threads_ng_engine_spinlock_t *)pspinlock;

	/*
	 *      ERRORS:
	 *      EBUSY  The implementation has detected an attempt to destroy a spin lock while
	 *                     it is in use
	 *      EINVAL The value of the spinlock argument is invalid
	 */

	printf("%d %d\n", sctk_spin_destroyed, spinlock->state);
	if(spinlock == NULL || spinlock->state != sctk_spin_initialized)
	{
		return EINVAL;
	}
	mpc_common_spinlock_t *p_lock = &(spinlock->lock);
	if(OPA_load_int(p_lock) != 0)
	{
		return EBUSY;
	}

	spinlock->state = sctk_spin_destroyed;
	return 0;
}

static int _mpc_threads_ng_engine_spin_init(mpc_thread_spinlock_t *pspinlock, int pshared)
{
	_mpc_threads_ng_engine_spinlock_t *spinlock = (_mpc_threads_ng_engine_spinlock_t *)pspinlock;

	/*
	 *      ERRORS:
	 *      EBUSY  The implementation has detected an attempt to initialize a spin lock while
	 *                     it is in use
	 *      EINVAL The value of the spinlock argument is invalid
	 *      EAGAIN |>NOT IMPLEMENTED<| The system lacks the necessary resources to initialize
	 *                     another spin lock
	 *      ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the lock
	 */

	if(spinlock == NULL)
	{
		return EINVAL;
	}
	mpc_common_spinlock_t *p_lock = &(spinlock->lock);
	if( (spinlock->state == sctk_spin_initialized || spinlock->state == sctk_spin_destroyed) &&
	    (OPA_load_int(p_lock) != 0) )
	{
		return EBUSY;
	}
	if(pshared == SCTK_THREAD_PROCESS_SHARED)
	{
		fprintf(stderr, "Invalid pshared value in attr, MPC doesn't handle process shared spinlocks\n");
		return ENOTSUP;
	}

	_mpc_threads_ng_engine_spinlock_t  local   = SCTK_THREAD_GENERIC_SPINLOCK_INIT;
	_mpc_threads_ng_engine_spinlock_t *p_local = &local;
	p_local->state = sctk_spin_initialized;
	(*spinlock)    = local;
	printf("init:%d\n", spinlock->state);

	return 0;
}

static int _mpc_threads_ng_engine_spin_lock(mpc_thread_spinlock_t *pspinlock)
{
	_mpc_threads_ng_engine_spinlock_t * spinlock = (_mpc_threads_ng_engine_spinlock_t *)pspinlock;
	_mpc_threads_ng_engine_scheduler_t *sched    = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 *      ERRORS:
	 *      EINVAL  The value of the spinlock argument is invalid
	 *      EDEADLK The calling thread already holds the lock
	 */

	if(spinlock == NULL)
	{
		return EINVAL;
	}
	if(spinlock->owner == sched)
	{
		return EDEADLK;
	}

	long i = SCTK_SPIN_DELAY;
	mpc_common_spinlock_t *p_lock = &(spinlock->lock);
	while(mpc_common_spinlock_trylock(&(spinlock->lock) ) )
	{
		while(expect_true(OPA_load_int(p_lock) != 0) )
		{
			i--;
			if(expect_false(i <= 0) )
			{
				_mpc_threads_ng_engine_sched_yield(sched);
				i = SCTK_SPIN_DELAY;
			}
		}
	}
	spinlock->owner = sched;
	printf("in lock:%d\n", spinlock->state);
	return 0;
}

static int _mpc_threads_ng_engine_spin_trylock(mpc_thread_spinlock_t *pspinlock)
{
	_mpc_threads_ng_engine_spinlock_t * spinlock = (_mpc_threads_ng_engine_spinlock_t *)pspinlock;
	_mpc_threads_ng_engine_scheduler_t *sched    = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 *      ERRORS:
	 *      EINVAL  The value of the spinlock argument is invalid
	 *      EDEADLK The calling thread already holds the lock
	 *      EBUSY   Another thread currently holds the lock
	 */

	if(spinlock == NULL)
	{
		return EINVAL;
	}
	if(spinlock->owner == sched)
	{
		return EDEADLK;
	}

	if(mpc_common_spinlock_trylock(&(spinlock->lock) ) != 0)
	{
		return EBUSY;
	}
	else
	{
		spinlock->owner = sched;
		return 0;
	}
}

static int _mpc_threads_ng_engine_spin_unlock(mpc_thread_spinlock_t *pspinlock)
{
	_mpc_threads_ng_engine_spinlock_t * spinlock = (_mpc_threads_ng_engine_spinlock_t *)pspinlock;
	_mpc_threads_ng_engine_scheduler_t *sched    = &(_mpc_threads_ng_engine_self()->sched);

	/*
	 *      ERRORS:
	 *      EINVAL The value of the spinlock argument is invalid
	 *      EPERM  The calling thread does not hold the lock
	 */

	if(spinlock == NULL)
	{
		return EINVAL;
	}
	if(spinlock->owner != sched)
	{
		return EPERM;
	}

	spinlock->owner = NULL;
	mpc_common_spinlock_unlock(&(spinlock->lock) );
	printf("in unlock:%d\n", spinlock->state);
	return 0;
}

/* Init */

static inline void __spinlock_init()
{
	_mpc_threads_ng_engine_check_size(mpc_thread_spinlock_t, _mpc_threads_ng_engine_spinlock_t);

	//printf("TOTO %d - %d \n", sizeof(mpc_thread_spinlock_t), sizeof(mpc_thread_spinlock_t));
	/*static mpc_thread_spinlock_t loc = */
}

/***************************************/
/* THREAD KILL                         */
/***************************************/

static inline void ___wake_on_barrier(_mpc_threads_ng_engine_scheduler_t *sched,
                                      void *lock, int remove_from_lock_list)
{
	_mpc_threads_ng_engine_barrier_t *     barrier = (_mpc_threads_ng_engine_barrier_t *)lock;
	_mpc_threads_ng_engine_barrier_cell_t *list;

	mpc_common_spinlock_lock(&(barrier->lock) );
	DL_FOREACH(barrier->blocked, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(barrier->blocked, list);
		barrier->nb_current -= 1;
		if(list->sched->th->attr.cancel_type != PTHREAD_CANCEL_DEFERRED)
		{
			_mpc_threads_ng_engine_register_spinlock_unlock(&(_mpc_threads_ng_engine_self()->sched),
			                                              &(barrier->lock) );
			sctk_generic_swap_to_sched(sched);
		}
		else
		{
			mpc_common_spinlock_unlock(&(barrier->lock) );
			_mpc_threads_ng_engine_wake(sched);
		}
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(barrier->lock) );
	}
}

static inline void ___wake_on_cond(_mpc_threads_ng_engine_scheduler_t *sched,
                                   void *lock, int remove_from_lock_list)
{
	_mpc_threads_ng_engine_cond_t *     cond = (_mpc_threads_ng_engine_cond_t *)lock;
	_mpc_threads_ng_engine_cond_cell_t *list;

	mpc_common_spinlock_lock(&(cond->lock) );
	DL_FOREACH(cond->blocked, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(cond->blocked, list);
		mpc_common_spinlock_unlock(&(cond->lock) );
		_mpc_threads_ng_engine_wake(sched);
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(cond->lock) );
	}
}

static inline void ___wake_on_mutex(_mpc_threads_ng_engine_scheduler_t *sched,
                                    void *lock, int remove_from_lock_list)
{
	_mpc_threads_ng_engine_mutex_t *     mu = (_mpc_threads_ng_engine_mutex_t *)lock;
	_mpc_threads_ng_engine_mutex_cell_t *list;

	mpc_common_spinlock_lock(&(mu->lock) );
	DL_FOREACH(mu->blocked, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(mu->blocked, list);
		if(list->sched->th->attr.cancel_type != PTHREAD_CANCEL_DEFERRED)
		{
			_mpc_threads_ng_engine_register_spinlock_unlock(&(_mpc_threads_ng_engine_self()->sched),
			                                              &(mu->lock) );
			sctk_generic_swap_to_sched(sched);
		}
		else
		{
			mpc_common_spinlock_unlock(&(mu->lock) );
			_mpc_threads_ng_engine_wake(sched);
		}
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(mu->lock) );
	}
}

static inline void ___wake_on_rwlock(_mpc_threads_ng_engine_scheduler_t *sched,
                                     void *lock, int remove_from_lock_list)
{
	_mpc_threads_ng_engine_rwlock_t *     rw = (_mpc_threads_ng_engine_rwlock_t *)lock;
	_mpc_threads_ng_engine_rwlock_cell_t *list;

	mpc_common_spinlock_lock(&(rw->lock) );
	DL_FOREACH(rw->waiting, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(rw->waiting, list);
		rw->count--;
		if(list->sched->th->attr.cancel_type != PTHREAD_CANCEL_DEFERRED)
		{
			_mpc_threads_ng_engine_register_spinlock_unlock(&(_mpc_threads_ng_engine_self()->sched),
			                                              &(rw->lock) );
			sctk_generic_swap_to_sched(sched);
			/* Maybe need to remove the lock from taken thread s rwlocks list*/
		}
		else
		{
			mpc_common_spinlock_unlock(&(rw->lock) );
			_mpc_threads_ng_engine_wake(sched);
		}
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(rw->lock) );
	}
}

static inline void ___wake_on_semaphore(_mpc_threads_ng_engine_scheduler_t *sched,
                                        void *lock, int remove_from_lock_list)
{
	_mpc_threads_ng_engine_sem_t *       sem = (_mpc_threads_ng_engine_sem_t *)lock;
	_mpc_threads_ng_engine_mutex_cell_t *list;

	mpc_common_spinlock_lock(&(sem->spinlock) );
	DL_FOREACH(sem->list, list)
	{
		if(list->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && list)
	{
		DL_DELETE(sem->list, list);
		mpc_common_spinlock_unlock(&(sem->spinlock) );
		_mpc_threads_ng_engine_wake(sched);
	}
	else
	{
		if(list)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&(sem->spinlock) );
	}
}

static inline void __handle_blocked_threads(_mpc_threads_ng_engine_t threadp,
                                            int remove_from_lock_list)
{
	_mpc_threads_ng_engine_p_t *th = threadp;
	int    i;
	void **list = (void **)th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table;

	for(i = 0; i <= SCTK_BLOCKING_LOCK_TABLE_SIZE; i++)
	{
		if( (i < SCTK_BLOCKING_LOCK_TABLE_SIZE) &&
		    (list[i] != NULL) )
		{
			break;
		}
	}

	switch(i)
	{
		case MPC_THREADS_GENERIC_BARRIER:
			___wake_on_barrier(&(th->sched), list[i], remove_from_lock_list);
			break;

		case MPC_THREADS_GENERIC_COND:
			___wake_on_cond(&(th->sched), list[i], remove_from_lock_list);
			break;

		case MPC_THREADS_GENERIC_MUTEX:
			___wake_on_mutex(&(th->sched), list[i], remove_from_lock_list);
			break;

		case MPC_THREADS_GENERIC_RWLOCK:
			___wake_on_rwlock(&(th->sched), list[i], remove_from_lock_list);
			break;

		case MPC_THREADS_GENERIC_SEM:
			___wake_on_semaphore(&(th->sched), list[i], remove_from_lock_list);
			break;

		case MPC_THREADS_GENERIC_TASK_LOCK:
			_mpc_threads_ng_engine_wake_on_task_lock(&(th->sched), remove_from_lock_list);
			break;

		default:
			mpc_common_nodebug("No saved lock, thread %p has been wakened", th);
	}
}

static int _mpc_threads_ng_engine_kill(_mpc_threads_ng_engine_t threadp, int val)
{
	/*
	 *    ERRORS:
	 *    ESRCHH No thread with the ID threadp could be found
	 *    EINVAL An invalid signal was specified
	 */

	mpc_common_nodebug("_mpc_threads_ng_engine_kill %p %d set", threadp, val);

	_mpc_threads_ng_engine_p_t *th = threadp;
	if(th->sched.status == _mpc_threads_ng_engine_joined ||
	   th->sched.status == _mpc_threads_ng_engine_zombie)
	{
		return ESRCH;
	}

	if(val == 0)
	{
		return 0;
	}
	val--;
	if(val < 0 || val > SCTK_NSIG)
	{
		errno = EINVAL;
		return EINVAL;
	}

	if( (&(th->attr.spinlock) ) != (&(_mpc_threads_ng_engine_self()->attr.spinlock) ) )
	{
		mpc_common_spinlock_lock(&(th->attr.spinlock) );
		if(th->attr.thread_sigpending[val] == 0)
		{
			th->attr.thread_sigpending[val] = 1;
			th->attr.nb_sig_pending++;
		}
		mpc_common_spinlock_unlock(&(th->attr.spinlock) );
	}
	else
	{
		if(th->attr.thread_sigpending[val] == 0)
		{
			th->attr.thread_sigpending[val] = 1;
			th->attr.nb_sig_pending++;
		}
	}

	sigset_t set;
	_mpc_thread_kthread_sigmask(SIG_SETMASK, (sigset_t *)&(th->attr.sa_sigset_mask), &set);
	_mpc_thread_kthread_sigmask(SIG_BLOCK, &set, NULL);
	_mpc_thread_kthread_sigmask(SIG_SETMASK, &set, (sigset_t *)&(th->attr.sa_sigset_mask) );

	if(th->sched.status == _mpc_threads_ng_engine_blocked)
	{
		__handle_blocked_threads(threadp, 0);
	}

	return 0;
}

/*
 * defined but not used
 *
 * static int
 * _mpc_threads_ng_engine_kill_other_threads_np(){
 * return 0;
 * }
 */

/***************************************/
/* THREAD CREATION                     */
/***************************************/

static inline void __alloc_blocking_lock_table(const _mpc_threads_ng_engine_attr_t *attr)
{
	int    i;
	void **lock_table = (void **)sctk_malloc(SCTK_BLOCKING_LOCK_TABLE_SIZE * sizeof(void *) );

	if(lock_table == NULL)
	{
		abort();
	}
	for(i = 0; i < SCTK_BLOCKING_LOCK_TABLE_SIZE; i++)
	{
		lock_table[i] = NULL;
	}
	attr->ptr->_mpc_threads_ng_engine_pthread_blocking_lock_table = (void *)lock_table;
}

static inline void __attr_init(_mpc_threads_ng_engine_intern_attr_t *attr)
{
	memset(attr, 0, sizeof(_mpc_threads_ng_engine_intern_attr_t) );

	attr->scope        = SCTK_THREAD_SCOPE_PROCESS;
	attr->detachstate  = SCTK_THREAD_CREATE_JOINABLE;
	attr->inheritsched = SCTK_THREAD_EXPLICIT_SCHED;
	attr->cancel_state = PTHREAD_CANCEL_ENABLE;
	attr->cancel_type  = PTHREAD_CANCEL_DEFERRED;
	attr->bind_to      = -1;
	mpc_common_spinlock_init(&attr->spinlock, 0);
	_mpc_threads_ng_engine_kind_t knd = _mpc_threads_ng_engine_kind_init;
	attr->kind                = knd;
	attr->basic_priority      = 10;
	attr->current_priority    = 10;
	attr->timestamp_threshold = 1;
	attr->timestamp_base      = -1;
}

int _mpc_threads_ng_engine_attr_init(_mpc_threads_ng_engine_attr_t *attr)
{
	_mpc_threads_ng_engine_intern_attr_t init;

	__attr_init(&init);
	attr->ptr = (_mpc_threads_ng_engine_intern_attr_t *)
	            sctk_malloc(sizeof(_mpc_threads_ng_engine_intern_attr_t) );

	*(attr->ptr) = init;
	__attr_init_signals(attr);
	__alloc_blocking_lock_table(attr);
	return 0;
}

static int _mpc_threads_ng_engine_getattr_np(_mpc_threads_ng_engine_t threadp,
                                           _mpc_threads_ng_engine_attr_t *attr)
{
	/*
	 *    ERRORS:
	 *    EINVAL
	 *    ESRCH
	 */

	if(threadp == NULL || attr == NULL)
	{
		return EINVAL;
	}
	_mpc_threads_ng_engine_p_t *th = threadp;
	if(th->sched.status == _mpc_threads_ng_engine_joined ||
	   th->sched.status == _mpc_threads_ng_engine_zombie)
	{
		return ESRCH;
	}

	/* if(attr->ptr == NULL){ */
	_mpc_threads_ng_engine_attr_init(attr);
	/* } */
	*(attr->ptr) = th->attr;
	return 0;
}

int _mpc_threads_ng_engine_attr_destroy(_mpc_threads_ng_engine_attr_t *attr)
{
	sctk_free(attr->ptr);
	attr->ptr = NULL;
	return 0;
}

static int _mpc_threads_ng_engine_attr_getscope(const _mpc_threads_ng_engine_attr_t *attr, int *scope)
{
	_mpc_threads_ng_engine_intern_attr_t init;

	__attr_init(&init);
	_mpc_threads_ng_engine_intern_attr_t *ptr;

	ptr = attr->ptr;

	if(ptr == NULL)
	{
		ptr = &init;
		/* __attr_init_signals( attr ); */
		/* __alloc_blocking_lock_table( attr ); */
	}
	*scope = ptr->scope;
	return 0;
}

static int _mpc_threads_ng_engine_attr_setscope(_mpc_threads_ng_engine_attr_t *attr, int scope)
{
	if(scope != PTHREAD_SCOPE_SYSTEM && scope != PTHREAD_SCOPE_PROCESS)
	{
		return EINVAL;
	}
	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}
	attr->ptr->scope = scope;
	return 0;
}

static int _mpc_threads_ng_engine_attr_setbinding(_mpc_threads_ng_engine_attr_t *attr, int binding)
{
	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}
	attr->ptr->bind_to = binding;
	return 0;
}

static int _mpc_threads_ng_engine_attr_getbinding(_mpc_threads_ng_engine_attr_t *attr, int *binding)
{
	_mpc_threads_ng_engine_intern_attr_t init;

	__attr_init(&init);
	_mpc_threads_ng_engine_intern_attr_t *ptr;

	ptr = attr->ptr;

	if(ptr == NULL)
	{
		ptr = &init;
		__attr_init_signals(attr);
		__alloc_blocking_lock_table(attr);
	}
	*binding = ptr->bind_to;
	return 0;
}

static int _mpc_threads_ng_engine_attr_getstacksize(_mpc_threads_ng_engine_attr_t *attr,
                                                  size_t *stacksize)
{
	/*
	 *    ERRORS:
	 *    EINVAL The stacksize argument is not valide
	 */

	if(stacksize == NULL)
	{
		return EINVAL;
	}

	_mpc_threads_ng_engine_intern_attr_t init;
	__attr_init(&init);
	_mpc_threads_ng_engine_intern_attr_t *ptr;

	ptr = attr->ptr;

	if(ptr == NULL)
	{
		ptr = &init;
		__attr_init_signals(attr);
		__alloc_blocking_lock_table(attr);
	}
	*stacksize = ptr->stack_size;
	return 0;
}

static int _mpc_threads_ng_engine_attr_setstacksize(_mpc_threads_ng_engine_attr_t *attr,
                                                  size_t stacksize)
{
	/*
	 *    ERRORS:
	 *    EINVAL The stacksize value is less than PTHREAD_STACK_MIN
	 */

	if(stacksize < SCTK_THREAD_STACK_MIN)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}
	attr->ptr->stack_size = stacksize;
	return 0;
}

static int _mpc_threads_ng_engine_attr_getstackaddr(_mpc_threads_ng_engine_attr_t *attr,
                                                  void **addr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The addr argument is not valide
	 */

	if(addr == NULL)
	{
		return EINVAL;
	}

	_mpc_threads_ng_engine_intern_attr_t init;
	__attr_init(&init);
	_mpc_threads_ng_engine_intern_attr_t *ptr;

	ptr = attr->ptr;

	if(ptr == NULL)
	{
		ptr = &init;
		__attr_init_signals(attr);
		__alloc_blocking_lock_table(attr);
	}

	*addr = ptr->stack;
	return 0;
}

static int _mpc_threads_ng_engine_attr_setstackaddr(_mpc_threads_ng_engine_attr_t *attr, void *addr)
{
	/*
	 *    ERRORS:
	 *    EINVAL The addr argument is not valide
	 */

	if(addr == NULL)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}
	attr->ptr->stack      = addr;
	attr->ptr->user_stack = attr->ptr->stack;
	return 0;
}

static int _mpc_threads_ng_engine_attr_getstack(const _mpc_threads_ng_engine_attr_t *attr,
                                              void **stackaddr, size_t *stacksize)
{
	/*
	 *    ERRORS:
	 *    EINVAL Argument is not valide
	 */

	if(attr == NULL || stackaddr == NULL ||
	   stacksize == NULL)
	{
		return EINVAL;
	}

	*stackaddr = (void *)attr->ptr->stack;
	*stacksize = attr->ptr->stack_size;

	return 0;
}

static int _mpc_threads_ng_engine_attr_setstack(_mpc_threads_ng_engine_attr_t *attr,
                                              void *stackaddr, size_t stacksize)
{
	/*
	 *    ERRORS:
	 *    EINVAL Argument is not valide or the stacksize value is less
	 *               than PTHREAD_STACK_MIN
	 */

	if(stackaddr == NULL || stacksize < SCTK_THREAD_STACK_MIN)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}

	attr->ptr->stack      = stackaddr;
	attr->ptr->user_stack = attr->ptr->stack;
	attr->ptr->stack_size = stacksize;

	return 0;
}

static int _mpc_threads_ng_engine_attr_getguardsize(_mpc_threads_ng_engine_attr_t *attr,
                                                  size_t *guardsize)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid arguments for the function
	 */

	if(attr == NULL || guardsize == NULL)
	{
		return EINVAL;
	}
	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}
	(*guardsize) = attr->ptr->stack_guardsize;

	return 0;
}

static int _mpc_threads_ng_engine_attr_setguardsize(_mpc_threads_ng_engine_attr_t *attr,
                                                  size_t guardsize)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid arguments for the function
	 */

	if(attr == NULL)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}

	attr->ptr->stack_guardsize = guardsize;

	return 0;
}

struct sched_param;

static int _mpc_threads_ng_engine_attr_getschedparam(_mpc_threads_ng_engine_attr_t *attr,
                                                   struct sched_param *param)
{
	/*
	 *    ERRORS:
	 *    EINVAL  Arguments of function are invalid
	 */

	if(attr == NULL || param == NULL)
	{
		return EINVAL;
	}
	param->__sched_priority = 0;
	return 0;
}

static int _mpc_threads_ng_engine_attr_setschedparam(_mpc_threads_ng_engine_attr_t *attr,
                                                   const struct sched_param *param)
{
	/*
	 *    ERRORS:
	 *    EINVAL  Arguments of function are invalid
	 *    ENOTSUP Only priority 0 (default) is supported
	 */

	if(attr == NULL || param == NULL)
	{
		return EINVAL;
	}
	if(param->__sched_priority != 0)
	{
		return ENOTSUP;
	}
	return 0;
}

static int _mpc_threads_ng_engine_attr_setschedpolicy(_mpc_threads_ng_engine_attr_t *attr,
                                                    int policy)
{
	if(policy == SCTK_SCHED_FIFO ||
	   policy == SCTK_SCHED_RR)
	{
		return ENOTSUP;
	}
	if(policy != SCTK_SCHED_OTHER)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}

	attr->ptr->schedpolicy = policy;

	return 0;
}

static int _mpc_threads_ng_engine_attr_getschedpolicy(_mpc_threads_ng_engine_attr_t *attr,
                                                    int *policy)
{
	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}

	(*policy) = attr->ptr->schedpolicy;

	return 0;
}

static int _mpc_threads_ng_engine_attr_getinheritsched(_mpc_threads_ng_engine_attr_t *attr,
                                                     int *inheritsched)
{
	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}

	(*inheritsched) = attr->ptr->inheritsched;

	return 0;
}

static int _mpc_threads_ng_engine_attr_setinheritsched(_mpc_threads_ng_engine_attr_t *attr,
                                                     int inheritsched)
{
	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}
	if(inheritsched != SCTK_THREAD_INHERIT_SCHED &&
	   inheritsched != SCTK_THREAD_EXPLICIT_SCHED)
	{
		return EINVAL;
	}

	attr->ptr->inheritsched = inheritsched;

	return 0;
}

static int _mpc_threads_ng_engine_attr_getdetachstate(_mpc_threads_ng_engine_attr_t *attr,
                                                    int *detachstate)
{
	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}

	(*detachstate) = attr->ptr->detachstate;

	return 0;
}

static int _mpc_threads_ng_engine_attr_setdetachstate(_mpc_threads_ng_engine_attr_t *attr,
                                                    int detachstate)
{
	if(detachstate != PTHREAD_CREATE_DETACHED && detachstate != PTHREAD_CREATE_JOINABLE)
	{
		return EINVAL;
	}

	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}

	attr->ptr->detachstate = detachstate;

	return 0;
}

static void ___threads_generic_start_func(void *arg)
{
	_mpc_threads_ng_engine_p_t *thread;

	thread = arg;

	mpc_common_nodebug("Before yield %p", &(thread->sched) );
	/*It is mandatory to have two yields for pthread mode*/
	_mpc_threads_ng_engine_self_set(thread);
	_mpc_threads_ng_engine_sched_yield(&(thread->sched) );
	_mpc_threads_ng_engine_sched_yield(&(thread->sched) );

	mpc_common_nodebug("Start %p %p", &(thread->sched), thread->attr.arg);
	thread->attr.return_value = thread->attr.start_routine(thread->attr.arg);
	mpc_common_nodebug("End %p %p", &(thread->sched), thread->attr.arg);

	/* Handel Exit */
	if(thread->attr.scope == SCTK_THREAD_SCOPE_SYSTEM)
	{
		sctk_swapcontext(&(thread->sched.ctx), &(thread->sched.ctx_bootstrap) );
	}
	else
	{
		_mpc_threads_ng_engine_thread_status(&(thread->sched), _mpc_threads_ng_engine_zombie);
		_mpc_threads_ng_engine_sched_yield(&(thread->sched) );
	}
	not_reachable();
}

int _mpc_threads_ng_engine_user_create(_mpc_threads_ng_engine_t *threadp,
                                     _mpc_threads_ng_engine_attr_t *attr,
                                     void *(*start_routine)(void *), void *arg)
{
	_mpc_threads_ng_engine_intern_attr_t init;

	__attr_init(&init);
	_mpc_threads_ng_engine_intern_attr_t *ptr;
	sctk_thread_data_t     tmp;
	_mpc_threads_ng_engine_t thread_id;
	char * stack;
	size_t stack_size;
	size_t stack_guardsize;

	if(attr == NULL)
	{
		ptr = &init;
	}
	else
	{
		ptr = attr->ptr;

		if(ptr == NULL)
		{
			ptr = &init;
		}
	}
	_mpc_threads_ng_engine_attr_t lattr;
	lattr.ptr = ptr;

	/****** SIGNALS ******/
	__default_sigset_init();
	__attr_init_signals(&lattr);
	__alloc_blocking_lock_table(&lattr);

	mpc_common_nodebug("Create %p", arg);

	if(arg != NULL)
	{
		tmp = *( (sctk_thread_data_t *)arg);
		if(tmp.tls == NULL)
		{
			tmp.tls = mpc_thread_tls;
		}
	}
	else
	{
		tmp.tls = mpc_thread_tls;
	}

	/*Create data struct*/
	{
		thread_id =
		        __sctk_malloc(sizeof(_mpc_threads_ng_engine_p_t), tmp.tls);

		thread_id->attr = *ptr;

		_mpc_threads_ng_engine_scheduler_init_thread(&(thread_id->sched), thread_id);
		_mpc_threads_ng_engine_key_init_thread(&(thread_id->keys) );
	}

	/*Allocate stack*/
	{
		stack           = thread_id->attr.stack;
		stack_size      = thread_id->attr.stack_size;
		stack_guardsize = thread_id->attr.stack_guardsize;
		size_t old_stack_size      = 0;
		size_t old_stack_guardsize = 0;

		if(stack == NULL)
		{
			if(mpc_common_get_flags()->is_fortran == 1 && stack_size <= 0)
			{
				stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
			}
			else if(stack_size <= 0)
			{
				stack_size = SCTK_ETHREAD_STACK_SIZE;
			}
			if(stack_guardsize > 0)
			{
				long _page_size_ = 0;
				int  ret;
#ifndef _SC_PAGESIZE
  #ifndef PAGE_SIZE
	#error "Unknown system macro for pagesize"
  #else
				_page_size_ = sysconf(PAGESIZE);
  #endif
#else
				_page_size_ = sysconf(_SC_PAGESIZE);
#endif
				old_stack_guardsize = stack_guardsize;
				stack_guardsize     = (stack_guardsize + _page_size_ - 1) & ~(_page_size_ - 1);
				stack_size          = (stack_size + _page_size_ - 1) & ~(_page_size_ - 1);
				old_stack_size      = stack_size;
				stack_size          = stack_size + stack_guardsize;
				int ret_mem = posix_memalign( (void *)&stack, _page_size_, stack_size);
				if(stack == NULL || ret_mem != 0)
				{
					sctk_free(thread_id);
					return EAGAIN;
				}
				if( (ret = mprotect(stack, stack_guardsize, PROT_NONE) ) != 0)
				{
					sctk_free(stack);
					sctk_free(thread_id);
					return ret;
				}
			}
			if(stack == NULL)
			{
				stack = (char *)__sctk_malloc(stack_size + 8, tmp.tls);
				if(stack == NULL)
				{
					sctk_free(thread_id);
					return EAGAIN;
				}
			}
		}
		else if(stack_size <= 0)
		{
			sctk_free(thread_id);
			return EINVAL;
		}

		if(stack_guardsize > 0)
		{
			thread_id->attr.user_stack = stack;
			thread_id->attr.stack      = stack + old_stack_guardsize;
			thread_id->attr.stack_size = old_stack_size;
		}
		else
		{
			thread_id->attr.stack      = stack;
			thread_id->attr.stack_size = stack_size;
		}
	}


	/*Create context*/
	{
		thread_id->attr.start_routine = start_routine;
		thread_id->attr.arg           = arg;
		mpc_common_nodebug("STACK %p STACK SIZE %lu", stack, stack_size);
		sctk_makecontext(&(thread_id->sched.ctx),
		                 (void *)thread_id,
		                 ___threads_generic_start_func, stack, stack_size);
	}

	thread_id->attr.nb_wait_for_join = 0;

	*threadp = thread_id;

	_mpc_threads_ng_engine_sched_create(thread_id);
	return 0;
}

static int _mpc_threads_ng_engine_create(_mpc_threads_ng_engine_t *threadp,
                                       _mpc_threads_ng_engine_attr_t *attr,
                                       void *(*start_routine)(void *), void *arg)
{
	static unsigned int pos = 0;
	int res;
	_mpc_threads_ng_engine_attr_t tmp_attr;

	tmp_attr.ptr = NULL;

	if(attr == NULL)
	{
		attr = &tmp_attr;
	}
	if(attr->ptr == NULL)
	{
		_mpc_threads_ng_engine_attr_init(attr);
	}

	if(attr->ptr->bind_to == -1)
	{
		attr->ptr->bind_to = mpc_thread_get_task_placement(pos);
		pos++;
	}

	res = _mpc_threads_ng_engine_user_create(threadp, attr, start_routine, arg);

	_mpc_threads_ng_engine_attr_destroy(&tmp_attr);

	return res;
}

static int _mpc_threads_ng_engine_atfork(__UNUSED__ void (*prepare)(void), __UNUSED__ void (*parent)(void),
                                       __UNUSED__ void (*child)(void) )
{
	not_implemented();
	return 0;
}

static int _mpc_threads_ng_engine_sched_get_priority_min(int sched_policy)
{
	/*
	 *    ERRORS:
	 *    EINVAL the value of the argument is not valid
	 */

	if(sched_policy != SCTK_SCHED_FIFO && sched_policy != SCTK_SCHED_RR &&
	   sched_policy != SCTK_SCHED_OTHER)
	{
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int _mpc_threads_ng_engine_sched_get_priority_max(int sched_policy)
{
	/*
	 *    ERRORS:
	 *    EINVAL the value of the argument is not valid
	 */

	if(sched_policy != SCTK_SCHED_FIFO && sched_policy != SCTK_SCHED_RR &&
	   sched_policy != SCTK_SCHED_OTHER)
	{
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int _mpc_threads_ng_engine_getschedparam(_mpc_threads_ng_engine_t threadp,
                                              int *policy, struct sched_param *param)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid arguments for the function call
	 *    ESRCH No thread with the ID threadp could be found
	 */

	if(threadp == NULL || policy == NULL || param == NULL)
	{
		return EINVAL;
	}
	_mpc_threads_ng_engine_p_t *th = threadp;
	if(th->sched.status == _mpc_threads_ng_engine_zombie ||
	   th->sched.status == _mpc_threads_ng_engine_joined)
	{
		return ESRCH;
	}

	param->__sched_priority = 0;
	(*policy) = SCHED_OTHER;

	return 0;
}

static int _mpc_threads_ng_engine_setschedparam(_mpc_threads_ng_engine_t threadp,
                                              int policy, const struct sched_param *param)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid arguments for the function call
	 *    ESRCH No thread with the ID threadp could be found
	 *    EPERM |>NOT IMPLEMENTED<| The caller does not have appropriate
	 *    privileges to set the specified scheduling policy and parameters
	 */

	if(threadp == NULL || param == NULL)
	{
		return EINVAL;
	}
	if(policy != SCTK_SCHED_OTHER)
	{
		return EINVAL;
	}
	_mpc_threads_ng_engine_p_t *th = threadp;
	if(th->sched.status == _mpc_threads_ng_engine_zombie ||
	   th->sched.status == _mpc_threads_ng_engine_joined)
	{
		return ESRCH;
	}
	if(param->__sched_priority != 0)
	{
		return EINVAL;
	}

	return 0;
}

/***************************************/
/* THREAD CANCELATION                  */
/***************************************/


static int _mpc_threads_ng_engine_cancel(_mpc_threads_ng_engine_t threadp)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid value for the argument
	 */

	_mpc_threads_ng_engine_p_t *th = threadp;

	mpc_common_debug("thread to cancel: %p\n", th);

	if(th == NULL)
	{
		return EINVAL;
	}
	if(th->sched.status == _mpc_threads_ng_engine_zombie ||
	   th->sched.status == _mpc_threads_ng_engine_joined)
	{
		return ESRCH;
	}

	if(th->attr.cancel_state == PTHREAD_CANCEL_ENABLE)
	{
		th->attr.cancel_status = 1;
		mpc_common_debug("thread %p canceled\n", th);
	}

	if(th->sched.status == _mpc_threads_ng_engine_blocked)
	{
		__handle_blocked_threads(threadp, 1);
	}

	return 0;
}

static int _mpc_threads_ng_engine_setcancelstate(int state, int *oldstate)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid value for state
	 */

	_mpc_threads_ng_engine_attr_t attr;

	attr.ptr = &(_mpc_threads_ng_engine_self()->attr);

	if(oldstate != NULL)
	{
		(*oldstate) = attr.ptr->cancel_state;
	}

	if(state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE)
	{
		return EINVAL;
	}

	attr.ptr->cancel_state = state;

	return 0;
}

static int _mpc_threads_ng_engine_setcanceltype(int type, int *oldtype)
{
	/*
	 *    ERRORS:
	 *    EINVAL Invalid value for type
	 */

	_mpc_threads_ng_engine_attr_t attr;

	attr.ptr = &(_mpc_threads_ng_engine_self()->attr);

	if(oldtype != NULL)
	{
		(*oldtype) = attr.ptr->cancel_type;
	}

	if(type != PTHREAD_CANCEL_DEFERRED &&
	   type != PTHREAD_CANCEL_ASYNCHRONOUS)
	{
		return EINVAL;
	}

	attr.ptr->cancel_type = type;

	return 0;
}

static int _mpc_threads_ng_engine_setschedprio(_mpc_threads_ng_engine_t threadp, int prio)
{
	/*
	 *    ERRORS
	 *    EINVAL prio is not valid for the scheduling policy of the specified thread
	 *    EPERM  |>NOT IMPLEMENTED<| The caller does not have appropriate privileges
	 *               to set the specified priority
	 *    ESRCH  No thread with the ID thread could be found
	 */

	if(threadp == NULL || prio != 0)
	{
		return EINVAL;
	}

	_mpc_threads_ng_engine_p_t *th = threadp;
	if(th->sched.status == _mpc_threads_ng_engine_joined ||
	   th->sched.status == _mpc_threads_ng_engine_zombie)
	{
		return ESRCH;
	}

	mpc_common_nodebug("Only Priority 0 handled in current version of MPC, priority remains the same as before the call");

	return 0;
}

static void _mpc_threads_ng_engine_testcancel()
{
	_mpc_threads_ng_engine_check_signals(0);
}

/***************************************/
/* THREAD POLLING                      */
/***************************************/

void _mpc_threads_ng_engine_wait_for_value_and_poll(volatile int *data, int value,
                                                  void (*func)(void *), void *arg)
{
	_mpc_threads_ng_engine_task_t task;

	if(func)
	{
		func(arg);
	}
	if(*data == value)
	{
		return;
	}

	task.sched       = &(_mpc_threads_ng_engine_self()->sched);
	task.func        = func;
	task.value       = value;
	task.arg         = arg;
	task.data        = data;
	task.is_blocking = 1;
	task.free_func   = NULL;
	task.next        = NULL;
	task.prev        = NULL;

	_mpc_threads_ng_engine_add_task(&task);
}

/***************************************/
/* THREAD JOIN                         */
/***************************************/

static inline void __zombies_handle(_mpc_threads_ng_engine_scheduler_generic_t *th)
{
	_mpc_threads_ng_engine_scheduler_generic_t *schedg = th;


	mpc_common_nodebug("th %p to be freed", th->sched->th);
	if(schedg->sched->th->attr.user_stack == NULL)
	{
		sctk_free(schedg->sched->th->attr.stack);
	}
	sctk_free(schedg->sched->th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table);
	sctk_free(schedg->sched->th);
}

static int _mpc_threads_ng_engine_join(_mpc_threads_ng_engine_t threadp, void **val)
{
	/*
	 *    ERRORS:
	 *    ESRCH  No  thread could be found corresponding to that specified by th.
	 *    EINVAL The th thread has been detached.
	 *    EINVAL Another thread is already waiting on termination of th.
	 *    EDEADLK The th argument refers to the calling thread.
	 */

	_mpc_threads_ng_engine_p_t *            th = threadp;
	_mpc_threads_ng_engine_scheduler_t *    sched;
	_mpc_threads_ng_engine_p_t *            current;
	_mpc_threads_ng_engine_thread_status_t *status;

	/* Get the current thread */
	sched   = &(_mpc_threads_ng_engine_self()->sched);
	current = sched->th;
	assert(&current->sched == sched);

	mpc_common_nodebug("Join Thread %p", th);

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	if(th != current)
	{
		if(th->sched.status == _mpc_threads_ng_engine_joined)
		{
			return ESRCH;
		}
		if(th->attr.detachstate != 0)
		{
			return EINVAL;
		}

		th->attr.nb_wait_for_join++;
		if(th->attr.nb_wait_for_join != 1)
		{
			return EINVAL;
		}

		status = (_mpc_threads_ng_engine_thread_status_t *)&(th->sched.status);
		mpc_common_nodebug("TO Join Thread %p", th);
		_mpc_threads_ng_engine_wait_for_value_and_poll( (volatile int *)status,
		                                              _mpc_threads_ng_engine_zombie, NULL, NULL);

		/* test cancel */
		_mpc_threads_ng_engine_check_signals(0);

		mpc_common_nodebug("Joined Thread %p", th);

		if(val != NULL)
		{
			*val = th->attr.return_value;
		}
		th->sched.status = _mpc_threads_ng_engine_joined;

		/* Free thread memory */
		__zombies_handle(&(th->sched.generic) );
	}
	else
	{
		return EDEADLK;
	}

	return 0;
}

/***************************************/
/* THREAD EXIT                         */
/***************************************/

static void _mpc_threads_ng_engine_exit(void *retval)
{
	_mpc_threads_ng_engine_scheduler_t *sched;
	_mpc_threads_ng_engine_p_t *        current;

	/* test cancel */
	_mpc_threads_ng_engine_check_signals(0);

	/* Get the current thread */
	sched   = &(_mpc_threads_ng_engine_self()->sched);
	current = sched->th;
	assert(&current->sched == sched);

	current->attr.return_value = retval;

	mpc_common_nodebug("thread %p key liberation", current);
	/*key liberation */
	__keys_delete_all(&(current->keys) );

	mpc_common_nodebug("thread %p key liberation done", current);

	mpc_common_nodebug("thread %p ends", current);

	_mpc_threads_ng_engine_thread_status(&(current->sched), _mpc_threads_ng_engine_zombie);
	_mpc_threads_ng_engine_sched_yield(&(current->sched) );
	not_reachable();
}

/***************************************/
/* THREAD EQUAL                        */
/***************************************/

static int _mpc_threads_ng_engine_equal(_mpc_threads_ng_engine_t th1, _mpc_threads_ng_engine_t th2)
{
	return th1 == th2;
}

/***************************************/
/* THREAD DETACH                       */
/***************************************/

static int _mpc_threads_ng_engine_detach(_mpc_threads_ng_engine_t threadp)
{
	/*
	 *    ERRORS:
	 *    EINVAL threadp is not a joinable thread or already detach
	 *    ESRCH  No thread with the ID threadp could be found
	 */

	if(threadp == NULL)
	{
		return ESRCH;
	}
	_mpc_threads_ng_engine_p_t *th = threadp;

	if(th->sched.status == _mpc_threads_ng_engine_joined /*||
	                                                    * th->sched.status == _mpc_threads_ng_engine_zombie*/)
	{
		return ESRCH;
	}
	if(th->attr.detachstate == SCTK_THREAD_CREATE_DETACHED)
	{
		return EINVAL;
	}

	th->attr.detachstate = SCTK_THREAD_CREATE_DETACHED;

	return 0;
}

/***************************************/
/* THREAD CONCURRENCY                  */
/***************************************/

static int _mpc_threads_ng_engine_setconcurrency(int new_level)
{
	if(new_level < 0)
	{
		return EINVAL;
	}
	if(new_level != 0)
	{
		return ENOTSUP;
	}
	return 0;
}

static int _mpc_threads_ng_engine_getconcurrency(void)
{
	return 0;
}

/***************************************/
/* THREAD CPUCLOCKID                   */
/***************************************/

static int _mpc_threads_ng_engine_getcpuclockid(_mpc_threads_ng_engine_t threadp,
                                              clockid_t *clock_id)
{
	/*
	 *    ERRORS:
	 *    ENOENT |>NOT IMPLEMENTED<| Per-thread CPU time clocks
	 *               are not supported by the system
	 *    ESRCH  No thread with the ID thread could be found
	 *    EINVAL Invalid arguments for function call
	 */

	if(threadp == NULL || clock_id == NULL)
	{
		return EINVAL;
	}
	_mpc_threads_ng_engine_p_t *th = threadp;
	if(th->sched.status == _mpc_threads_ng_engine_zombie ||
	   th->sched.status == _mpc_threads_ng_engine_joined)
	{
		return ESRCH;
	}

	int ret = 0;
#ifdef _POSIX_THREAD_CPUTIME
	(*clock_id) = CLOCK_THREAD_CPUTIME_ID;
#else
	ret = ENOENT;
#endif

	return ret;
}

/***************************************/
/* THREAD ONCE                         */
/***************************************/

static inline int __once_init(mpc_thread_once_t *once_control)
{
#ifdef mpc_thread_once_t_is_contiguous_int
	return *( (mpc_thread_once_t *)once_control) == SCTK_THREAD_ONCE_INIT;

#else
	mpc_thread_once_t once_init = SCTK_THREAD_ONCE_INIT;
	return memcpy
	               ( (void *)&once_init, (void *)once_control,
	               sizeof(mpc_thread_once_t) ) == 0;
#endif
}

static int _mpc_threads_ng_engine_once(mpc_thread_once_t *once_control,
                                     void (*init_routine)(void) )
{
	static mpc_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

	if(__once_init(once_control) )
	{
		mpc_thread_mutex_lock(&lock);
		if(__once_init(once_control) )
		{
			init_routine();
#ifdef mpc_thread_once_t_is_contiguous_int
			*once_control = !SCTK_THREAD_ONCE_INIT;
#else
			once_control[0] = 1;
#endif
		}
		mpc_thread_mutex_unlock(&lock);
	}
	return 0;
}

/***************************************/
/* YIELD                               */
/***************************************/
static int _mpc_threads_ng_engine_yield()
{
	_mpc_threads_ng_engine_sched_yield(&(_mpc_threads_ng_engine_self()->sched) );
	return 0;
}

/***************************************/
/* INIT                                */
/***************************************/
static void _mpc_threads_ng_engine_init(char *thread_type, char *scheduler_type, int vp_number)
{
	mpc_common_debug_only_once();
	_mpc_threads_ng_engine_self_data = sctk_malloc(sizeof(_mpc_threads_ng_engine_p_t) );

	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_t, mpc_thread_t);
	sctk_add_func_type(_mpc_threads_ng_engine, self, mpc_thread_t (*)(void) );

	/****** SIGNALS ******/
	__default_sigset_init();


	/****** SCHEDULER ******/
	_mpc_threads_ng_engine_scheduler_init(thread_type, scheduler_type, vp_number);
	_mpc_threads_ng_engine_scheduler_init_thread(&(_mpc_threads_ng_engine_self()->sched),
	                                           _mpc_threads_ng_engine_self() );
	{
		_mpc_threads_ng_engine_attr_t         lattr;
		_mpc_threads_ng_engine_intern_attr_t *ptr;
		_mpc_threads_ng_engine_intern_attr_t  init;
		__attr_init(&init);
		_mpc_threads_ng_engine_scheduler_t *sched;
		_mpc_threads_ng_engine_p_t *        current;

		ptr       = &init;
		lattr.ptr = ptr;
		__attr_init_signals(&lattr);
		__alloc_blocking_lock_table(&lattr);
		sched         = &(_mpc_threads_ng_engine_self()->sched);
		current       = sched->th;
		current->attr = *ptr;

		mpc_common_debug("%d", current->attr.cancel_status);
	}

	/****** KEYS ******/
	__keys_init();

	sctk_add_func_type(_mpc_threads_ng_engine, key_create,
	                   int (*)(mpc_thread_keys_t *, void (*)(void *) ) );
	sctk_add_func_type(_mpc_threads_ng_engine, key_delete,
	                   int (*)(mpc_thread_keys_t) );
	sctk_add_func_type(_mpc_threads_ng_engine, setspecific,
	                   int (*)(mpc_thread_keys_t, const void *) );
	sctk_add_func_type(_mpc_threads_ng_engine, getspecific,
	                   void *(*)(mpc_thread_keys_t) );
	_mpc_threads_ng_engine_key_init_thread(&(_mpc_threads_ng_engine_self()->keys) );

	/****** MUTEX ******/
	__mutex_init();

	sctk_add_func_type(_mpc_threads_ng_engine, mutexattr_destroy,
	                   int (*)(mpc_thread_mutexattr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutexattr_setpshared,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutexattr_getpshared,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutexattr_settype,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutexattr_gettype,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutexattr_init,
	                   int (*)(mpc_thread_mutexattr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutex_lock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutex_spinlock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutex_trylock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutex_timedlock,
	                   int (*)(mpc_thread_mutex_t *, const struct timespec *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutex_unlock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutex_destroy,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, mutex_init,
	                   int (*)(mpc_thread_mutex_t *, const mpc_thread_mutexattr_t *) );

	/****** THREAD SIGNALS ******/
	sctk_add_func_type(_mpc_threads_ng_engine, sigsuspend,
	                   int (*)(sigset_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sigpending,
	                   int (*)(sigset_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sigmask,
	                   int (*)(int, const sigset_t *, sigset_t *) );

	/****** COND ******/
	__cond_init();

	sctk_add_func_type(_mpc_threads_ng_engine, cond_init,
	                   int (*)(mpc_thread_cond_t *, const mpc_thread_condattr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, condattr_destroy,
	                   int (*)(mpc_thread_condattr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, condattr_getpshared,
	                   int (*)(const mpc_thread_condattr_t *, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, condattr_init,
	                   int (*)(mpc_thread_condattr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, condattr_setpshared,
	                   int (*)(mpc_thread_condattr_t *, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, condattr_setclock,
	                   int (*)(mpc_thread_condattr_t *, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, condattr_getclock,
	                   int (*)(mpc_thread_condattr_t *, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, cond_destroy,
	                   int (*)(mpc_thread_cond_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, cond_wait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, cond_signal,
	                   int (*)(mpc_thread_cond_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, cond_timedwait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *, const struct timespec *) );
	sctk_add_func_type(_mpc_threads_ng_engine, cond_broadcast,
	                   int (*)(mpc_thread_cond_t *) );

	/****** SEMAPHORE ******/
	__semaphore_init();

	sctk_add_func_type(_mpc_threads_ng_engine, sem_init,
	                   int (*)(mpc_thread_sem_t *, int, unsigned int) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_wait,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_trywait,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_timedwait,
	                   int (*)(mpc_thread_sem_t *, const struct timespec *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_post,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_getvalue,
	                   int (*)(mpc_thread_sem_t *, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_destroy,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_open,
	                   mpc_thread_sem_t * (*)(const char *, int, ...) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_close,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, sem_unlink,
	                   int (*)(const char *) );

	/****** RWLOCK ******/
	__rwlock_init();

	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_init,
	                   int (*)(mpc_thread_rwlock_t *, const mpc_thread_rwlockattr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlockattr_destroy,
	                   int (*)(mpc_thread_rwlockattr_t *attr) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlockattr_getpshared,
	                   int (*)(const mpc_thread_rwlockattr_t *attr, int *val) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlockattr_init,
	                   int (*)(mpc_thread_rwlockattr_t *attr) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlockattr_setpshared,
	                   int (*)(mpc_thread_rwlockattr_t *attr, int val) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_destroy,
	                   int (*)(mpc_thread_rwlock_t *lock) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_rdlock,
	                   int (*)(mpc_thread_rwlock_t *lock) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_wrlock,
	                   int (*)(mpc_thread_rwlock_t *lock) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_tryrdlock,
	                   int (*)(mpc_thread_rwlock_t *lock) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_trywrlock,
	                   int (*)(mpc_thread_rwlock_t *lock) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_unlock,
	                   int (*)(mpc_thread_rwlock_t *lock) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_timedrdlock,
	                   int (*)(mpc_thread_rwlock_t *lock, const struct timespec *time) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlock_timedwrlock,
	                   int (*)(mpc_thread_rwlock_t *lock, const struct timespec *time) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlockattr_setkind_np,
	                   int (*)(mpc_thread_rwlockattr_t *attr, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, rwlockattr_getkind_np,
	                   int (*)(mpc_thread_rwlockattr_t *attr, int *) );


	/****** THREAD BARRIER *******/
	__barrier_init();

	sctk_add_func_type(_mpc_threads_ng_engine, barrier_init,
	                   int (*)(mpc_thread_barrier_t *restrict,
	                           const mpc_thread_barrierattr_t *restrict, unsigned) );
	sctk_add_func_type(_mpc_threads_ng_engine, barrierattr_destroy,
	                   int (*)(mpc_thread_barrierattr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, barrierattr_init,
	                   int (*)(mpc_thread_barrierattr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, barrierattr_getpshared,
	                   int (*)(const mpc_thread_barrierattr_t *restrict, int *restrict) );
	sctk_add_func_type(_mpc_threads_ng_engine, barrierattr_setpshared,
	                   int (*)(mpc_thread_barrierattr_t *, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, barrier_destroy,
	                   int (*)(mpc_thread_barrier_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, barrier_init,
	                   int (*)(mpc_thread_barrier_t *restrict,
	                           const mpc_thread_barrierattr_t *restrict, unsigned) );
	sctk_add_func_type(_mpc_threads_ng_engine, barrier_wait,
	                   int (*)(mpc_thread_barrier_t *) );

	/****** THREAD SPINLOCK ******/
	__spinlock_init();
	sctk_add_func_type(_mpc_threads_ng_engine, spin_init,
	                   int (*)(mpc_thread_spinlock_t *, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, spin_destroy,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, spin_lock,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, spin_trylock,
	                   int (*)(mpc_thread_spinlock_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, spin_unlock,
	                   int (*)(mpc_thread_spinlock_t *) );

	/****** THREAD KILL ******/
	sctk_add_func_type(_mpc_threads_ng_engine, kill,
	                   int (*)(mpc_thread_t, int) );

	/****** THREAD CREATION ******/
	_mpc_threads_ng_engine_check_size(_mpc_threads_ng_engine_attr_t, mpc_thread_attr_t);
	sctk_add_func_type(_mpc_threads_ng_engine, attr_init,
	                   int (*)(mpc_thread_attr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_destroy,
	                   int (*)(mpc_thread_attr_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, getattr_np,
	                   int (*)(mpc_thread_t, mpc_thread_attr_t *) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_getscope,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_setscope,
	                   int (*)(mpc_thread_attr_t *, int) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_getbinding,
	                   int (*)(mpc_thread_attr_t *, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_setbinding,
	                   int (*)(mpc_thread_attr_t *, int) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_getstacksize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_setstacksize,
	                   int (*)(mpc_thread_attr_t *, size_t) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_getstackaddr,
	                   int (*)(const mpc_thread_attr_t *, void **) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_setstackaddr,
	                   int (*)(mpc_thread_attr_t *, void *) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_setstack,
	                   int (*)(mpc_thread_attr_t *, void *, size_t) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_getstack,
	                   int (*)(const mpc_thread_attr_t *, void **, size_t *) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_setguardsize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_getguardsize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_setschedparam,
	                   int (*)(mpc_thread_attr_t *, const struct sched_param *) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_getschedparam,
	                   int (*)(const mpc_thread_attr_t *, struct sched_param *) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_getschedpolicy,
	                   int (*)(const mpc_thread_attr_t *, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_setschedpolicy,
	                   int (*)(mpc_thread_attr_t *, int) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_setinheritsched,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_getinheritsched,
	                   int (*)(const mpc_thread_attr_t *, int *) );

	sctk_add_func_type(_mpc_threads_ng_engine, attr_setdetachstate,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, attr_getdetachstate,
	                   int (*)(const mpc_thread_attr_t *, int *) );

	sctk_add_func_type(_mpc_threads_ng_engine, user_create,
	                   int (*)(mpc_thread_t *, const mpc_thread_attr_t *,
	                           void *(*)(void *), void *) );
	sctk_add_func_type(_mpc_threads_ng_engine, create,
	                   int (*)(mpc_thread_t *, const mpc_thread_attr_t *,
	                           void *(*)(void *), void *) );

	sctk_add_func_type(_mpc_threads_ng_engine, atfork,
	                   int (*)(void (*)(void),
	                           void (*)(void),
	                           void (*)(void) ) );
	sctk_add_func_type(_mpc_threads_ng_engine, sched_get_priority_min,
	                   int (*)(int) );
	sctk_add_func_type(_mpc_threads_ng_engine, sched_get_priority_max,
	                   int (*)(int) );

	sctk_add_func_type(_mpc_threads_ng_engine, getschedparam,
	                   int (*)(mpc_thread_t, int *, struct sched_param *) );
	sctk_add_func_type(_mpc_threads_ng_engine, setschedparam,
	                   int (*)(mpc_thread_t, int, const struct sched_param *) );

	/****** THREAD CANCEL ******/
	sctk_add_func_type(_mpc_threads_ng_engine, cancel,
	                   int (*)(mpc_thread_t) );
	sctk_add_func_type(_mpc_threads_ng_engine, setcancelstate,
	                   int (*)(int, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, setcanceltype,
	                   int (*)(int, int *) );
	sctk_add_func_type(_mpc_threads_ng_engine, setschedprio,
	                   int (*)(mpc_thread_t, int) );
	sctk_add_func_type(_mpc_threads_ng_engine, testcancel,
	                   void (*)() );

	/****** THREAD POLLING ******/
	sctk_add_func(_mpc_threads_ng_engine, wait_for_value_and_poll);

	/****** JOIN ******/
	sctk_add_func_type(_mpc_threads_ng_engine, join,
	                   int (*)(mpc_thread_t, void **) );

	/****** EXIT ******/
	sctk_add_func_type(_mpc_threads_ng_engine, exit,
	                   void (*)(void *) );

	/****** EQUAL ******/
	sctk_add_func_type(_mpc_threads_ng_engine, equal,
	                   int (*)(mpc_thread_t, mpc_thread_t) );

	/****** DETACH ******/
	sctk_add_func_type(_mpc_threads_ng_engine, detach,
	                   int (*)(mpc_thread_t) );

	/****** CONCURRENCY ******/
	sctk_add_func_type(_mpc_threads_ng_engine, getconcurrency,
	                   int (*)(void) );
	sctk_add_func_type(_mpc_threads_ng_engine, setconcurrency,
	                   int (*)(int) );

	/****** CPUCLOCKID ******/
	sctk_add_func_type(_mpc_threads_ng_engine, getcpuclockid,
	                   int (*)(mpc_thread_t, clockid_t *) );

	/****** THREAD ONCE ******/
	_mpc_threads_ng_engine_check_size(mpc_thread_once_t, mpc_thread_once_t);
	sctk_add_func_type(_mpc_threads_ng_engine, once,
	                   int (*)(mpc_thread_once_t *, void (*)(void) ) );

	/****** YIELD ******/
	sctk_add_func_type(_mpc_threads_ng_engine, yield,
	                   int (*)() );

	sctk_multithreading_initialised = 1;

	_mpc_thread_data_init();
	_mpc_threads_ng_engine_polling_init(vp_number);
}

/***************************************/
/* IMPLEMENTATION SPECIFIC             */
/***************************************/

static inline int __get_cpu_count()
{
	int   cpu_number;
	char *env;

	cpu_number = mpc_topology_get_pu_count();
	env        = getenv("SCTK_SET_CORE_NUMBER");
	if(env != NULL)
	{
		cpu_number = atoi(env);
		assume(cpu_number > 0);
	}
	return cpu_number;
}

/********* ETHREAD MXN ************/
void mpc_thread_ethread_mxn_ng_engine_init(void)
{
	mpc_common_get_flags()->new_scheduler_engine_enabled = 1;
	_mpc_threads_ng_engine_init("ethread_mxn",
	                          "generic/multiple_queues_with_priority",
	                          __get_cpu_count() );
}

/********* ETHREAD ************/
void mpc_thread_ethread_ng_engine_init(void)
{
	mpc_common_get_flags()->new_scheduler_engine_enabled = 1;
	_mpc_threads_ng_engine_init("ethread_mxn", "generic/multiple_queues", 1);
}

/********* PTHREAD ************/
void mpc_thread_pthread_ng_engine_init(void)
{
	mpc_common_get_flags()->new_scheduler_engine_enabled = 1;
	_mpc_threads_ng_engine_init("pthread", "generic/multiple_queues", __get_cpu_count() );
}
