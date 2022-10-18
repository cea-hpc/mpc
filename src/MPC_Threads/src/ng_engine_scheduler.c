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
/* #   - TABOADA Hugo hugo.taboada@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include "mpc_thread.h"

#include <mpc_arch.h>

#include "mpc_common_debug.h"
#include "thread_ptr.h"
#include "kthread.h"
#include "mpc_common_spinlock.h"
#include "ng_engine.h"
#include "ng_engine_scheduler.h"
#include "mpc_topology.h"
#include "thread.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpc_common_flags.h>

#include <sctk_alloc.h>

//#define SCTK_DEBUG_SCHEDULER

static void (*_mpc_threads_ng_engine_sched_idle_start)(void);

void (*_mpc_threads_ng_engine_sched_yield)(_mpc_threads_ng_engine_scheduler_t *) = NULL;
void (*_mpc_threads_ng_engine_thread_status)(_mpc_threads_ng_engine_scheduler_t *,
                                          _mpc_threads_ng_engine_thread_status_t) = NULL;
void (*_mpc_threads_ng_engine_register_spinlock_unlock)(_mpc_threads_ng_engine_scheduler_t *,
                                                     mpc_common_spinlock_t *) = NULL;
void (*_mpc_threads_ng_engine_wake)(_mpc_threads_ng_engine_scheduler_t *) = NULL;

void (*_mpc_threads_ng_engine_scheduler_init_thread_p)(_mpc_threads_ng_engine_scheduler_t *) = NULL;

void          (*_mpc_threads_ng_engine_sched_create)(_mpc_threads_ng_engine_p_t *) = NULL;
void          (*_mpc_threads_ng_engine_add_task)(_mpc_threads_ng_engine_task_t *) = NULL;
static void * (*_mpc_threads_ng_engine_polling_func)(void *) = NULL;
/***************************************/
/* VP MANAGEMENT                       */
/***************************************/
typedef struct
{
	_mpc_threads_ng_engine_p_t *thread;
	int                      core;
} _mpc_threads_ng_engine_scheduler_task_t;
static __thread int core_id = -1;
static int          _mpc_threads_ng_engine_scheduler_use_binding = 1;

static void _mpc_threads_ng_engine_scheduler_bind_to_cpu(int core)
{
	if(_mpc_threads_ng_engine_scheduler_use_binding == 1)
	{
		mpc_topology_bind_to_cpu(core);
	}
}

static void *_mpc_threads_ng_engine_scheduler_idle_task(_mpc_threads_ng_engine_scheduler_task_t *arg)
{
	_mpc_threads_ng_engine_p_t    p_th;
	_mpc_threads_ng_engine_t      th;
	_mpc_threads_ng_engine_attr_t attr;

	if(arg->core >= 0)
	{
		_mpc_threads_ng_engine_scheduler_bind_to_cpu(arg->core);
	}
	core_id = arg->core;
	sctk_free(arg);
	arg = NULL;

	_mpc_threads_ng_engine_attr_init(&attr);
	p_th.attr = (*(attr.ptr) );
	th        = &p_th;

	_mpc_threads_ng_engine_self_set(th);
	_mpc_threads_ng_engine_scheduler_init_thread(&(_mpc_threads_ng_engine_self()->sched), th);
	_mpc_threads_ng_engine_key_init_thread(&(_mpc_threads_ng_engine_self()->keys) );

	/* Start Idle*/
	_mpc_threads_ng_engine_sched_idle_start();

	mpc_common_nodebug("End vp");

//#warning "Handle zombies"
	not_implemented();
	return NULL;
}

static void *_mpc_threads_ng_engine_scheduler_bootstrap_task(_mpc_threads_ng_engine_scheduler_task_t *arg)
{
	_mpc_threads_ng_engine_p_t *thread;

	if(arg->core >= 0)
	{
		_mpc_threads_ng_engine_scheduler_bind_to_cpu(arg->core);
	}
	core_id = arg->core;
	thread  = arg->thread;

	sctk_free(arg);
	arg = NULL;

	_mpc_threads_ng_engine_self_set(thread);
	sctk_swapcontext(&(thread->sched.ctx_bootstrap), &(thread->sched.ctx) );

	mpc_common_nodebug("End vp");

//#warning "Handle zombies"

	return NULL;
}

void _mpc_threads_ng_engine_scheduler_create_vp(_mpc_threads_ng_engine_p_t *thread, int core)
{
	void *(*start_routine) (void *);
	_mpc_threads_ng_engine_scheduler_task_t *arg;
	mpc_thread_kthread_t kthread;

	if(thread == NULL)
	{
		start_routine = (void *(*)(void *) )_mpc_threads_ng_engine_scheduler_idle_task;
	}
	else
	{
		start_routine = (void *(*)(void *) )_mpc_threads_ng_engine_scheduler_bootstrap_task;
	}

	arg         = sctk_malloc(sizeof(_mpc_threads_ng_engine_scheduler_task_t) );
	arg->thread = thread;
	arg->core   = core;

	assume(_mpc_thread_kthread_create(&kthread,
	                      start_routine, arg) == 0);
}

/***************************************/
/* CONTEXT MANAGEMENT                  */
/***************************************/
#include <pthread.h>
void _mpc_threads_ng_engine_scheduler_swapcontext(_mpc_threads_ng_engine_scheduler_t *old_th,
                                               _mpc_threads_ng_engine_scheduler_t *new_th)
{
	_mpc_threads_ng_engine_self_set(new_th->th);
	mpc_common_nodebug("SWAP %p %p -> %p", pthread_self(), &(old_th->ctx), &(new_th->ctx) );
	mpc_common_spinlock_unlock(&(old_th->debug_lock) );
	assume(mpc_common_spinlock_trylock(&(new_th->debug_lock) ) == 0);
	sctk_swapcontext(&(old_th->ctx), &(new_th->ctx) );
}

void _mpc_threads_ng_engine_scheduler_setcontext(_mpc_threads_ng_engine_scheduler_t *new_th)
{
	_mpc_threads_ng_engine_self_set(new_th->th);
	mpc_common_nodebug("SET %p", &(new_th->ctx) );
	sctk_setcontext(&(new_th->ctx) );
	assume(mpc_common_spinlock_trylock(&(new_th->debug_lock) ) == 0);
}

/***************************************/
/* TASKS MANAGEMENT                    */
/***************************************/
static int
_mpc_threads_ng_engine_scheduler_check_task(_mpc_threads_ng_engine_task_t *task)
{
	if(*(task->data) == task->value)
	{
		return 1;
	}
	if(task->func)
	{
		task->func(task->arg);
		if(*(task->data) == task->value)
		{
			return 1;
		}
	}
	return 0;
}

/***************************************/
/* CENTRALIZED SCHEDULER               */
/***************************************/
static mpc_common_spinlock_t sctk_centralized_sched_list_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static _mpc_threads_ng_engine_scheduler_generic_t *sctk_centralized_sched_list = NULL;

static _mpc_threads_ng_engine_task_t *sctk_centralized_task_list      = NULL;
static mpc_common_spinlock_t       sctk_centralized_task_list_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

static void sctk_centralized_add_to_list(_mpc_threads_ng_engine_scheduler_t *sched)
{
	assume(sched->generic.sched == sched);
	mpc_common_spinlock_lock(&sctk_centralized_sched_list_lock);
	DL_APPEND(sctk_centralized_sched_list, &(sched->generic) );
	mpc_common_nodebug("ADD Thread %p", sched);
	mpc_common_spinlock_unlock(&sctk_centralized_sched_list_lock);
}

static
void sctk_centralized_concat_to_list(__UNUSED__ _mpc_threads_ng_engine_scheduler_t *sched,
                                     _mpc_threads_ng_engine_scheduler_generic_t *s_list)
{
	mpc_common_spinlock_lock(&sctk_centralized_sched_list_lock);
	DL_CONCAT(sctk_centralized_sched_list, s_list);
	mpc_common_spinlock_unlock(&sctk_centralized_sched_list_lock);
}

static _mpc_threads_ng_engine_scheduler_t *sctk_centralized_get_from_list()
{
	if(sctk_centralized_sched_list != NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		mpc_common_spinlock_lock(&sctk_centralized_sched_list_lock);
		res = sctk_centralized_sched_list;
		if(res != NULL)
		{
			DL_DELETE(sctk_centralized_sched_list, res);
		}
		mpc_common_spinlock_unlock(&sctk_centralized_sched_list_lock);
		if(res != NULL)
		{
			mpc_common_nodebug("REMOVE Thread %p", res->sched);
			return res->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

static _mpc_threads_ng_engine_scheduler_t *sctk_centralized_get_from_list_pthread_init()
{
	if(sctk_centralized_sched_list != NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res_tmp;
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		mpc_common_spinlock_lock(&sctk_centralized_sched_list_lock);
		DL_FOREACH_SAFE(sctk_centralized_sched_list, res, res_tmp)
		{
			if( (res != NULL) && (res->vp_type == 4) )
			{
				DL_DELETE(sctk_centralized_sched_list, res);
				break;
			}
		}
		mpc_common_spinlock_unlock(&sctk_centralized_sched_list_lock);
		if(res != NULL)
		{
			mpc_common_nodebug("REMOVE Thread %p", res->sched);
			return res->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

static _mpc_threads_ng_engine_task_t *sctk_centralized_get_task()
{
	_mpc_threads_ng_engine_task_t *task = NULL;

	if(sctk_centralized_task_list != NULL)
	{
		mpc_common_spinlock_lock(&sctk_centralized_task_list_lock);
		if(sctk_centralized_task_list != NULL)
		{
			task = sctk_centralized_task_list;
			DL_DELETE(sctk_centralized_task_list, task);
		}
		mpc_common_spinlock_unlock(&sctk_centralized_task_list_lock);
	}
	return task;
}

static void sctk_centralized_add_task_to_proceed(_mpc_threads_ng_engine_task_t *task)
{
	mpc_common_spinlock_lock(&sctk_centralized_task_list_lock);
	DL_APPEND(sctk_centralized_task_list, task);
	mpc_common_spinlock_unlock(&sctk_centralized_task_list_lock);
}

static
void sctk_centralized_poll_tasks(_mpc_threads_ng_engine_scheduler_t *sched)
{
	_mpc_threads_ng_engine_task_t *     task;
	_mpc_threads_ng_engine_task_t *     task_tmp;
	_mpc_threads_ng_engine_scheduler_t *lsched = NULL;

	if(mpc_common_spinlock_trylock(&sctk_centralized_task_list_lock) == 0)
	{
		/* Exec polling */
		DL_FOREACH_SAFE(sctk_centralized_task_list, task, task_tmp)
		{
			if(_mpc_threads_ng_engine_scheduler_check_task(task) == 1)
			{
				DL_DELETE(sctk_centralized_task_list, task);
				mpc_common_nodebug("WAKE task %p", task);

				mpc_common_spinlock_lock(&(sched->generic.lock) );
				lsched = task->sched;
				if(task->free_func)
				{
					task->free_func(task->arg);
					task->free_func(task);
				}
				if(lsched)
				{
					void **tmp = NULL;
					if(task->sched && task->sched->th && &(task->sched->th->attr) )
					{
						tmp = (void **)task->sched->th->attr._mpc_threads_ng_engine_pthread_blocking_lock_table;
					}
					if(tmp)
					{
						tmp[MPC_THREADS_GENERIC_TASK_LOCK] = NULL;
					}
					_mpc_threads_ng_engine_wake(lsched);
				}
				mpc_common_spinlock_unlock(&(sched->generic.lock) );
			}
		}
		mpc_common_spinlock_unlock(&sctk_centralized_task_list_lock);
	}
}

void
sctk_centralized_thread_generic_wake_on_task_lock(_mpc_threads_ng_engine_scheduler_t *sched,
                                                  int remove_from_lock_list)
{
	_mpc_threads_ng_engine_task_t *task_tmp;

	mpc_common_spinlock_lock(&sctk_centralized_task_list_lock);
	DL_FOREACH(sctk_centralized_task_list, task_tmp)
	{
		if(task_tmp->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && task_tmp)
	{
		*(task_tmp->data) = task_tmp->value;
		mpc_common_spinlock_unlock(&sctk_centralized_task_list_lock);
	}
	else
	{
		if(task_tmp)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&sctk_centralized_task_list_lock);
	}
}

/***************************************/
/* MULTIPLE_QUEUES SCHEDULER           */
/***************************************/

typedef struct
{
	mpc_common_spinlock_t                    sctk_multiple_queues_sched_list_lock;
	/*Mettre un pointeur directement sur le thread de progression des NBC */
	_mpc_threads_ng_engine_scheduler_t *        sctk_multiple_queues_sched_NBC_Pthread_sched;
	_mpc_threads_ng_engine_scheduler_generic_t *sctk_multiple_queues_sched_list;
/* TODO Optimize to have data locality*/
	char                                     pad[4096];
} sctk_multiple_queues_sched_list_t;
#define SCTK_MULTIPLE_QUEUES_SCHED_LIST_INIT \
	{ MPC_COMMON_SPINLOCK_INITIALIZER, NULL, NULL, .pad = { 0 } }

static sctk_multiple_queues_sched_list_t *sctk_multiple_queues_sched_lists = NULL;

typedef struct
{
	// list of task which are able to be poll by the polling mpc thread
	_mpc_threads_ng_engine_task_t *sctk_multiple_queues_task_list;
	mpc_common_spinlock_t       sctk_multiple_queues_task_list_lock;
	int                         task_nb; // number of task in sctk_multiple_queues_task_list
	/*Mettre un pointeur directement sur le thread de polling mpc*/
	_mpc_threads_ng_engine_scheduler_t
	*                           sctk_multiple_queues_task_polling_thread_sched;
	char                        pad[4096];
}sctk_multiple_queues_task_list_t;
#define SCTK_MULTIPLE_QUEUES_TASK_LIST_INIT \
	{ NULL, MPC_COMMON_SPINLOCK_INITIALIZER, 0, NULL, .pad = { 0 } }
static sctk_multiple_queues_task_list_t *sctk_multiple_queues_task_lists = NULL;

////////////////////////////////////////
// generic/multiple_queues
//
// add_to_list
// concat_to_list
// get_from_list
//

// add_to_list
static void sctk_multiple_queues_add_to_list(_mpc_threads_ng_engine_scheduler_t *sched)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}


	assume(sched->generic.sched == sched);

	mpc_common_spinlock_lock(&(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list_lock) );
	DL_APPEND( (sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list), &(sched->generic) );
	mpc_common_nodebug("ADD Thread %p", sched);
	mpc_common_spinlock_unlock(&(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list_lock) );
}

// concat_to_list
static
void sctk_multiple_queues_concat_to_list(_mpc_threads_ng_engine_scheduler_t *sched,
                                         _mpc_threads_ng_engine_scheduler_generic_t *s_list)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}

	mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list_lock);
	DL_CONCAT(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list, s_list);
	mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list_lock);
}

// get_from_list
mpc_common_spinlock_t debug_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static _mpc_threads_ng_engine_scheduler_t *sctk_multiple_queues_get_from_list()
{
	int core;

	core = core_id;
	assert(core >= 0);

	char hostname[128];
	gethostname(hostname, 128);

	FILE *fd = fopen(hostname, "a");

	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list != NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		_mpc_threads_ng_engine_scheduler_generic_t *res_tmp;

		mpc_common_spinlock_lock(&(sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock) );
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		if(res != NULL)
		{
			DL_FOREACH_SAFE(sctk_multiple_queues_sched_lists[core]
			                .sctk_multiple_queues_sched_list,
			                res, res_tmp)
			{
				fprintf(fd, "hostname=%s core=%d res->sched=%p,scope=%d user_stack=%p "
				            "stack=%p bind_to %d polling %d kind %d priority %d \n",
				        hostname, core, res->sched, res->sched->th->attr.scope,
				        res->sched->th->attr.user_stack, res->sched->th->attr.stack,
				        res->sched->th->attr.bind_to, res->sched->th->attr.polling,
				        res->sched->th->attr.kind.mask,
				        res->sched->th->attr.kind.priority);
			}
			fprintf(fd, "\n\n\n");
		}
		mpc_common_spinlock_unlock(&(sctk_multiple_queues_sched_lists[core]
		                             .sctk_multiple_queues_sched_list_lock) );
	}

	fclose(fd);
	// endhmt

	// est ce que la liste existe et si ya un thread a prendre dedans
	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list !=
	   NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		if(res != NULL)
		{
			DL_DELETE(sctk_multiple_queues_sched_lists[core]
			          .sctk_multiple_queues_sched_list,
			          res);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);
		if(res != NULL)
		{
			mpc_common_nodebug("REMOVE Thread %p", res->sched);
			return res->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

////////////////////////////////////////
// generic/multiple_queues dynamic
//
// add_to_list
// concat_to_list
// get_from_list
//
#if 0
// add_to_list
static void sctk_multiple_queues_with_priority_dynamic_add_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}
	assume(sched->generic.sched == sched);

	mpc_common_spinlock_lock(&(sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock) );
	DL_APPEND(
	        (sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list),
	        &(sched->generic) );
	mpc_common_nodebug("ADD Thread %p", sched);
	mpc_common_spinlock_unlock(&(sctk_multiple_queues_sched_lists[core]
	                             .sctk_multiple_queues_sched_list_lock) );
}

// concat_to_list
static void sctk_multiple_queues_with_priority_dynamic_concat_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}
	assume(sched->generic.sched == sched);

	mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
	                         .sctk_multiple_queues_sched_list_lock);
	DL_CONCAT(
	        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list,
	        s_list);
	mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock);
}

// get_from_list with priority_dyn_sorted_list
static _mpc_threads_ng_engine_scheduler_t *
sctk_multiple_queues_with_priority_dynamic_get_from_list()
{
	int core;

	core = core_id;
	assert(core >= 0);

	_mpc_threads_ng_engine_scheduler_t *sched;
	sched = &(_mpc_threads_ng_engine_self()->sched);

#ifdef SCTK_DEBUG_SCHEDULER
	{
		char hostname[128];
		char hostname2[128];
		char current_vp[5];
		gethostname(hostname, 128);
		strncpy(hostname2, hostname, 128);
		sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
		strcat(hostname2, current_vp);

		FILE *fd = fopen(hostname2, "a");
		fprintf(fd, "current %p[%2d-%2d|%2d,%2d]\nlist[", sched,
		        sched->th->attr.kind.mask, sched->status,
		        sched->th->attr.current_priority, sched->th->attr.basic_priority);

		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);

		if(sctk_multiple_queues_sched_lists[core]
		   .sctk_multiple_queues_sched_list != NULL)
		{
			_mpc_threads_ng_engine_scheduler_generic_t *res;
			res = sctk_multiple_queues_sched_lists[core]
			      .sctk_multiple_queues_sched_list;
			DL_FOREACH(sctk_multiple_queues_sched_lists[core]
			           .sctk_multiple_queues_sched_list,
			           res)
			{
				fprintf(fd, "%p[%2d-%2d|%2d,%2d] ", res->sched,
				        res->sched->th->attr.kind.mask, sched->status,
				        res->sched->th->attr.current_priority,
				        res->sched->th->attr.basic_priority);
			}
			fprintf(fd, "] task_nb=%d\n",
			        sctk_multiple_queues_task_lists[core].task_nb);
		}
		else
		{
			fprintf(fd, "null ] task_nb=%d\n",
			        sctk_multiple_queues_task_lists[core].task_nb);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);

		fflush(fd);
		fclose(fd);
	}
#endif

	// check if the list exists and if it contains threads
	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list !=
	   NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		if(res != NULL)
		{
			DL_DELETE(sctk_multiple_queues_sched_lists[core]
			          .sctk_multiple_queues_sched_list,
			          res);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);
		if(res != NULL)
		{
			mpc_common_nodebug("REMOVE Thread %p", res->sched);
			return res->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

////////////////////////////////////////
// generic/multiple_queues_with_priority_default
//
// add_to_list
// concat_to_list
// get_from_list
//

// add_to_list with priority
static void sctk_multiple_queues_with_priority_default_add_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}

	assume(sched->generic.sched == sched);

	mpc_common_spinlock_lock(&(sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock) );
	DL_APPEND(
	        (sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list),
	        &(sched->generic) );
	mpc_common_nodebug("ADD Thread %p", sched);
	mpc_common_spinlock_unlock(&(sctk_multiple_queues_sched_lists[core]
	                             .sctk_multiple_queues_sched_list_lock) );
}

// concat_to_list with priority
static void sctk_multiple_queues_with_priority_default_concat_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}

	mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
	                         .sctk_multiple_queues_sched_list_lock);
	DL_CONCAT(
	        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list,
	        s_list);
	mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock);
}

// get_from_list with priority
static _mpc_threads_ng_engine_scheduler_t *
sctk_multiple_queues_with_priority_default_get_from_list()
{
	int core;

	core = core_id;
	assert(core >= 0);

	// //hmt
	// int i;
	// char hostname[128];
	// gethostname(hostname,128);

	// FILE* fd=fopen(hostname ,"a");

	// if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list
	// != NULL){
	//     _mpc_threads_ng_engine_scheduler_generic_t* res;
	//     _mpc_threads_ng_engine_scheduler_generic_t* res_tmp;

	//     mpc_common_spinlock_lock(
	//     &(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list_lock));
	//     res =
	//     sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
	//     if(res != NULL){
	//         DL_FOREACH_SAFE(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list,
	//                 res,res_tmp){
	//             fprintf(fd,"hostname=%s core=%d res->sched=%p,scope=%d
	//             user_stack=%p stack=%p bind_to %d polling %d kind %d priority
	//             %d \n",
	//                     hostname,
	//                     core,
	//                     res->sched,
	//                     res->sched->th->attr.scope,
	//                     res->sched->th->attr.user_stack,
	//                     res->sched->th->attr.stack,
	//                     res->sched->th->attr.bind_to,
	//                     res->sched->th->attr.polling,
	//                     res->sched->th->attr.kind.mask,
	//                     res->sched->th->attr.kind.priority);

	//         }
	//         fprintf(fd,"\n\n\n");
	//     }
	//     mpc_common_spinlock_unlock(
	//     &(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list_lock));
	// }

	// fclose(fd);
	// //endhmt

	// est ce que la liste existe et si ya un thread a prendre dedans
	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list !=
	   NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		if(res != NULL)
		{
			DL_DELETE(sctk_multiple_queues_sched_lists[core]
			          .sctk_multiple_queues_sched_list,
			          res);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);
		if(res != NULL)
		{
			mpc_common_nodebug("REMOVE Thread %p", res->sched);
			return res->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

////////////////////////////////////////
// generic/multiple_queues_with_priority_omp
//
// add_to_list
// concat_to_list
// get_from_list
//

// add_to_list with priority_omp
static void sctk_multiple_queues_with_priority_omp_add_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}

	assume(sched->generic.sched == sched);

	mpc_common_spinlock_lock(&(sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock) );
	DL_APPEND(
	        (sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list),
	        &(sched->generic) );
	mpc_common_nodebug("ADD Thread %p", sched);
	mpc_common_spinlock_unlock(&(sctk_multiple_queues_sched_lists[core]
	                             .sctk_multiple_queues_sched_list_lock) );
}

// concat_to_list with priority_omp
static void sctk_multiple_queues_with_priority_omp_concat_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}

	mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
	                         .sctk_multiple_queues_sched_list_lock);
	DL_CONCAT(
	        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list,
	        s_list);
	mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock);
}

// get_from_list with priority_omp
static _mpc_threads_ng_engine_scheduler_t *
sctk_multiple_queues_with_priority_omp_get_from_list()
{
	int core;

	core = core_id;
	assert(core >= 0);

	// check if the list exists and if it contains threads
	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list !=
	   NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		_mpc_threads_ng_engine_scheduler_generic_t *res_tmp;
		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);

		// look for omp threads to schedule them in priority
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		if(res != NULL)
		{
			// look for a thread OMP to schedule in priority
			DL_FOREACH_SAFE(sctk_multiple_queues_sched_lists[core]
			                .sctk_multiple_queues_sched_list,
			                res, res_tmp)
			{
				if(res->sched->th->attr.kind.mask & KIND_MASK_OMP)
				{
					printf("canard %d %d %d\n", res->sched->th->attr.kind.mask,
					       mpc_threads_generic_kind_mask_get(), mpc_topology_get_current_cpu() );
					fflush(stdout);
					// remove the omp thread from the list
					DL_DELETE(sctk_multiple_queues_sched_lists[core]
					          .sctk_multiple_queues_sched_list,
					          res);
					mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
					                           .sctk_multiple_queues_sched_list_lock);
					// return this thread
					return res->sched;
				}
			}
		}

		// if we dont have omp threads, do the default algorithm
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		if(res != NULL)
		{
			DL_DELETE(sctk_multiple_queues_sched_lists[core]
			          .sctk_multiple_queues_sched_list,
			          res);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);
		if(res != NULL)
		{
			mpc_common_nodebug("REMOVE Thread %p", res->sched);
			return res->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

////////////////////////////////////////
// generic/multiple_queues_with_priority_nofamine
//
// add_to_list
// concat_to_list
// get_from_list
//

// add_to_list with priority_nofamine
static void sctk_multiple_queues_with_priority_nofamine_add_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}

	assume(sched->generic.sched == sched);

	mpc_common_spinlock_lock(&(sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock) );
	DL_APPEND(
	        (sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list),
	        &(sched->generic) );
	mpc_common_nodebug("ADD Thread %p", sched);
	mpc_common_spinlock_unlock(&(sctk_multiple_queues_sched_lists[core]
	                             .sctk_multiple_queues_sched_list_lock) );
}

// concat_to_list with priority_nofamine
static void sctk_multiple_queues_with_priority_nofamine_concat_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}

	mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
	                         .sctk_multiple_queues_sched_list_lock);
	DL_CONCAT(
	        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list,
	        s_list);
	mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock);
}

// get_from_list with priority_nofamine
static _mpc_threads_ng_engine_scheduler_t *
sctk_multiple_queues_with_priority_nofamine_get_from_list()
{
	int core;

	core = core_id;
	assert(core >= 0);

	// check if the list exists and if it contains threads
	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list !=
	   NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		_mpc_threads_ng_engine_scheduler_generic_t *res_tmp;
		_mpc_threads_ng_engine_scheduler_generic_t *ret;
		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);
		// look for omp threads to schedule them in priority
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		ret = res;
		if(res != NULL)
		{
			// look for a thread OMP to schedule in priority
			DL_FOREACH_SAFE(sctk_multiple_queues_sched_lists[core]
			                .sctk_multiple_queues_sched_list,
			                res, res_tmp)
			{
				if(res->sched->th->attr.current_priority >
				   ret->sched->th->attr.current_priority)
				{
					ret = res;
				}
			}
			if(ret->sched->th->attr.current_priority < 0)
			{
				ret->sched->th->attr.current_priority =
				        ret->sched->th->attr.basic_priority;
			}
			else
			{
				ret->sched->th->attr.current_priority--;
			}
			DL_DELETE(sctk_multiple_queues_sched_lists[core]
			          .sctk_multiple_queues_sched_list,
			          ret);
			mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
			                           .sctk_multiple_queues_sched_list_lock);
			return ret->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}
#endif
////////////////////////////////////////
// generic/multiple_queues_with_priority_dyn_sorted_list
//
// add_to_list
// concat_to_list
// get_from_list
//

// function to be used on DL_SORT macro to sort the
// sctk_multiple_queues_sched_list
// using the current_priority of each thread in the list
int sctk_multiple_queues_sched_list_sort(void *cmp1, void *cmp2)
{
	_mpc_threads_ng_engine_scheduler_generic_t *elm1 =
	        (_mpc_threads_ng_engine_scheduler_generic_t *)cmp1;
	_mpc_threads_ng_engine_scheduler_generic_t *elm2 =
	        (_mpc_threads_ng_engine_scheduler_generic_t *)cmp2;

	if(elm1->sched->th->attr.current_priority ==
	   elm2->sched->th->attr.current_priority)
	{
		return 0;
	}
	else if(elm1->sched->th->attr.current_priority >
	        elm2->sched->th->attr.current_priority)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

// init NBC hook
void sctk_multiple_queues_with_priority_dyn_sorted_list_sched_NBC_Pthread_sched_init()
{
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched = &(_mpc_threads_ng_engine_self()->sched);
	sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
	.sctk_multiple_queues_sched_NBC_Pthread_sched = sched;
}

// increase priority of the NBC polling threads
void sctk_multiple_queues_priority_dyn_sorted_list_sched_NBC_Pthread_sched_increase_priority()
{
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched = &(_mpc_threads_ng_engine_self()->sched);
	assert(sched->th->attr.bind_to >= 0);

	sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
	.sctk_multiple_queues_sched_NBC_Pthread_sched->th->attr
	.current_priority += _mpc_thread_config_get()->scheduler_nbc_current_prio;
	sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
	.sctk_multiple_queues_sched_NBC_Pthread_sched->th->attr.basic_priority += _mpc_thread_config_get()->scheduler_nbc_basic_prio;
	// sort the list to have a sorted ready list
	mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
	                         .sctk_multiple_queues_sched_list_lock);
	DL_SORT(sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
	        .sctk_multiple_queues_sched_list,
	        sctk_multiple_queues_sched_list_sort);
	mpc_common_spinlock_unlock(
	        &sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
	        .sctk_multiple_queues_sched_list_lock);
}

// decrease priority of the NBC polling threads
// this function are called from the NBC thread, so we dont need to sort the
// list
// because we are running and we sort the list when we add element on the list
void sctk_multiple_queues_with_priority_dyn_sorted_list_sched_NBC_Pthread_sched_decrease_priority()
{
	_mpc_threads_ng_engine_scheduler_t *sched;

	sched = &(_mpc_threads_ng_engine_self()->sched);
	assert(sched->th->attr.bind_to >= 0);
	sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
	.sctk_multiple_queues_sched_NBC_Pthread_sched->th->attr
	.current_priority -= _mpc_thread_config_get()->scheduler_nbc_current_prio;
	sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
	.sctk_multiple_queues_sched_NBC_Pthread_sched->th->attr.basic_priority -=
	        _mpc_thread_config_get()->scheduler_nbc_step_prio;
}

// increase priority of the mpc polling threads
void sctk_multiple_queues_priority_dyn_sorted_list_task_polling_thread_sched_increase_priority(
        int bind_to)
{
	if(sctk_multiple_queues_task_lists[bind_to]
	   .sctk_multiple_queues_task_polling_thread_sched != NULL)
	{
		sctk_multiple_queues_task_lists[bind_to]
		.sctk_multiple_queues_task_polling_thread_sched->th->attr
		.current_priority += _mpc_thread_config_get()->scheduler_polling_current_prio;
		sctk_multiple_queues_task_lists[bind_to]
		.sctk_multiple_queues_task_polling_thread_sched->th->attr
		.basic_priority += _mpc_thread_config_get()->scheduler_polling_step_prio;

		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[bind_to]
		                         .sctk_multiple_queues_sched_list_lock);
		DL_SORT(sctk_multiple_queues_sched_lists[bind_to]
		        .sctk_multiple_queues_sched_list,
		        sctk_multiple_queues_sched_list_sort);
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[bind_to]
		                           .sctk_multiple_queues_sched_list_lock);


#ifdef SCTK_DEBUG_SCHEDULER
		{
			_mpc_threads_ng_engine_scheduler_t *sched;
			sched = &(_mpc_threads_ng_engine_self()->sched);
			char hostname[128];
			char hostname2[128];
			char current_vp[5];
			gethostname(hostname, 128);
			strncpy(hostname2, hostname, 128);
			sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
			strcat(hostname2, current_vp);

			FILE *fd = fopen(hostname2, "a");
			fprintf(fd, "multiple_queues_task add SCHED=%p %d %d\n\n", sched,
			        sctk_multiple_queues_task_lists[bind_to].task_nb,
			        sctk_multiple_queues_task_lists[bind_to]
			        .sctk_multiple_queues_task_polling_thread_sched->th->attr
			        .basic_priority);

			// fprintf(fd,"add task SCHED=%p %d\n\n",
			//        sched,
			//        sctk_multiple_queues_task_lists[bind_to].task_nb);
			fflush(fd);
			fclose(fd);
		}
#endif
	}
}

// decrease priority of the mpc polling threads
void sctk_multiple_queues_priority_dyn_sorted_list_task_polling_thread_sched_decrease_priority(
        int core)
{
	if(sctk_multiple_queues_task_lists[core]
	   .sctk_multiple_queues_task_polling_thread_sched != NULL &&
	   sctk_multiple_queues_task_lists[core]
	   .sctk_multiple_queues_task_polling_thread_sched->th->attr
	   .basic_priority != 1)
	{
		sctk_multiple_queues_task_lists[core]
		.sctk_multiple_queues_task_polling_thread_sched->th->attr
		.current_priority -= _mpc_thread_config_get()->scheduler_polling_current_prio;
		sctk_multiple_queues_task_lists[core]
		.sctk_multiple_queues_task_polling_thread_sched->th->attr
		.basic_priority -= _mpc_thread_config_get()->scheduler_polling_step_prio;

#ifdef SCTK_DEBUG_SCHEDULER
		{
			_mpc_threads_ng_engine_scheduler_t *sched;
			sched = &(_mpc_threads_ng_engine_self()->sched);
			char hostname[128];
			char hostname2[128];
			char current_vp[5];
			gethostname(hostname, 128);
			strncpy(hostname2, hostname, 128);
			sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
			strcat(hostname2, current_vp);

			FILE *fd = fopen(hostname2, "a");
			fprintf(fd, "multiple_queues_task poll SCHED=%p %d %d\n\n", sched,
			        sctk_multiple_queues_task_lists[core].task_nb,
			        sctk_multiple_queues_task_lists[core]
			        .sctk_multiple_queues_task_polling_thread_sched->th->attr
			        .basic_priority);
			fflush(fd);
			fclose(fd);
		}
#endif
	}
}

// add_to_list with priority_dyn_sorted_list
static void sctk_multiple_queues_with_priority_dyn_sorted_list_add_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}
	assume(sched->generic.sched == sched);

	_mpc_threads_ng_engine_scheduler_generic_t *res;
	mpc_common_spinlock_lock(&(sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock) );
	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list !=
	   NULL)
	{
		DL_FOREACH(
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list,
		        res)
		{
			if(sched->th->attr.current_priority >
			   res->sched->th->attr.current_priority)
			{
				if(sched->th->attr.current_priority < 0)
				{
					sched->th->attr.current_priority = sched->th->attr.basic_priority;
				}

				if(sched->th->attr.timestamp_count >
				   sched->th->attr.timestamp_threshold)
				{
					sched->th->attr.current_priority--;
					sched->th->attr.timestamp_count = 0;
				}

				DL_PREPEND_ELEM( (sctk_multiple_queues_sched_lists[core]
				                  .sctk_multiple_queues_sched_list),
				                 &(res->sched->generic), &(sched->generic) );
				mpc_common_nodebug("ADD Thread %p", sched);
				goto unlock;
			}
		}
	}
	if(sched->th->attr.current_priority < 0)
	{
		sched->th->attr.current_priority = sched->th->attr.basic_priority;
	}
	if(sched->th->attr.timestamp_count > sched->th->attr.timestamp_threshold)
	{
		sched->th->attr.current_priority--;
		sched->th->attr.timestamp_count = 0;
	}
	DL_APPEND(
	        (sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list),
	        &(sched->generic) );
unlock:
	mpc_common_spinlock_unlock(&(sctk_multiple_queues_sched_lists[core]
	                             .sctk_multiple_queues_sched_list_lock) );
}

// concat_to_list with priority_dyn_sorted_list
static void sctk_multiple_queues_with_priority_dyn_sorted_list_concat_to_list(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list)
{
	int core;

	core = sched->th->attr.bind_to;
	if(core < 0)
	{
		core = 0;
	}

	mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
	                         .sctk_multiple_queues_sched_list_lock);
	DL_CONCAT(
	        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list,
	        s_list);
	mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
	                           .sctk_multiple_queues_sched_list_lock);
}

#if 0
// get_from_list with priority_dyn_sorted_list
static _mpc_threads_ng_engine_scheduler_t *
sctk_multiple_queues_with_priority_dyn_sorted_list_get_from_list()
{
	int core;

	core = core_id;
	assert(core >= 0);

	_mpc_threads_ng_engine_scheduler_t *sched;
	sched = &(_mpc_threads_ng_engine_self()->sched);

#ifdef SCTK_DEBUG_SCHEDULER
	{
		char hostname[128];
		char hostname2[128];
		char current_vp[5];
		gethostname(hostname, 128);
		strncpy(hostname2, hostname, 128);
		sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
		strcat(hostname2, current_vp);

		FILE *fd = fopen(hostname2, "a");
		fprintf(fd, "current %p[%2d-%2d|%2d,%2d] list[", sched,
		        sched->th->attr.kind.mask, sched->status,
		        sched->th->attr.current_priority, sched->th->attr.basic_priority);

		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);

		if(sctk_multiple_queues_sched_lists[core]
		   .sctk_multiple_queues_sched_list != NULL)
		{
			_mpc_threads_ng_engine_scheduler_generic_t *res;
			res = sctk_multiple_queues_sched_lists[core]
			      .sctk_multiple_queues_sched_list;
			DL_FOREACH(sctk_multiple_queues_sched_lists[core]
			           .sctk_multiple_queues_sched_list,
			           res)
			{
				fprintf(fd, "%p[%2d-%2d|%2d,%2d] ", res->sched,
				        res->sched->th->attr.kind.mask, sched->status,
				        res->sched->th->attr.current_priority,
				        res->sched->th->attr.basic_priority);
			}
			fprintf(fd, "] task_nb=%d\n",
			        sctk_multiple_queues_task_lists[core].task_nb);
		}
		else
		{
			fprintf(fd, "null ] task_nb=%d\n",
			        sctk_multiple_queues_task_lists[core].task_nb);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);

		fflush(fd);
		fclose(fd);
	}
#endif

	// check if the list exists and if it contains threads
	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list !=
	   NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		if(res != NULL)
		{
			if(res->sched->th->attr.current_priority >=
			   sched->th->attr.current_priority)
			{
				DL_DELETE(sctk_multiple_queues_sched_lists[core]
				          .sctk_multiple_queues_sched_list,
				          res);
			}
		}

		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);

		if(res != NULL)
		{
			if(res->sched->th->attr.current_priority >=
			   sched->th->attr.current_priority)
			{
				mpc_common_nodebug("REMOVE Thread %p", res->sched);
				return res->sched;
			}
			else
			{
				sched->th->attr.current_priority--;
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}
#endif

// get_from_list with priority_dyn_sorted_list old
static _mpc_threads_ng_engine_scheduler_t *
sctk_multiple_queues_with_priority_dyn_sorted_list_get_from_list_old()
{
	int core;

	core = core_id;
	assert(core >= 0);



#ifdef SCTK_DEBUG_SCHEDULER
	{
		_mpc_threads_ng_engine_scheduler_t *sched;
		sched = &(_mpc_threads_ng_engine_self()->sched);
		char hostname[128];
		char hostname2[128];
		char current_vp[5];
		gethostname(hostname, 128);
		strncpy(hostname2, hostname, 128);
		sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
		strcat(hostname2, current_vp);

		FILE *fd = fopen(hostname2, "a");
		fprintf(fd, "current %p[%2d-%2d|%2d,%2d]\nlist[", sched,
		        sched->th->attr.kind.mask, sched->status,
		        sched->th->attr.current_priority, sched->th->attr.basic_priority);

		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);

		if(sctk_multiple_queues_sched_lists[core]
		   .sctk_multiple_queues_sched_list != NULL)
		{
			_mpc_threads_ng_engine_scheduler_generic_t *res;
			res = sctk_multiple_queues_sched_lists[core]
			      .sctk_multiple_queues_sched_list;
			DL_FOREACH(sctk_multiple_queues_sched_lists[core]
			           .sctk_multiple_queues_sched_list,
			           res)
			{
				fprintf(fd, "%p[%2d-%2d|%2d,%2d] ", res->sched,
				        res->sched->th->attr.kind.mask, sched->status,
				        res->sched->th->attr.current_priority,
				        res->sched->th->attr.basic_priority);
			}
			fprintf(fd, "] task_nb=%d\n",
			        sctk_multiple_queues_task_lists[core].task_nb);
		}
		else
		{
			fprintf(fd, "null ] task_nb=%d\n",
			        sctk_multiple_queues_task_lists[core].task_nb);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);

		fflush(fd);
		fclose(fd);
	}
#endif

	// check if the list exists and if it contains threads
	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list !=
	   NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core]
		                         .sctk_multiple_queues_sched_list_lock);
		res =
		        sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list;
		if(res != NULL)
		{
			DL_DELETE(sctk_multiple_queues_sched_lists[core]
			          .sctk_multiple_queues_sched_list,
			          res);
		}

		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core]
		                           .sctk_multiple_queues_sched_list_lock);

		if(res != NULL)
		{
			mpc_common_nodebug("REMOVE Thread %p", res->sched);
			return res->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

////////////////////////////////////////
// Function pointers : allow choice of multifile schedulers
//

//#define MULTIFILE_DYNAMIC
#define MULTIFILE_DEFAULT
///#define MULTIFILE_OMP
//#define MULTIFILE_NOFAMINE
//#define MULTIFILE_DYN_SORTED_LIST

#ifdef MULTIFILE_DYNAMIC
void (*_mpc_threads_ng_engine_scheduler_increase_prio_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_decrease_prio_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_sched_init_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_task_decrease_prio_ptr)(
        int core) = NULL;
// add_to_list
static void (*sctk_multiple_queues_with_priority_add_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched) =
        sctk_multiple_queues_with_priority_dynamic_add_to_list;
// concat_to_list
static void (*sctk_multiple_queues_with_priority_concat_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list) =
        sctk_multiple_queues_with_priority_dynamic_concat_to_list;
// get_from_list
static _mpc_threads_ng_engine_scheduler_t *(
*sctk_multiple_queues_with_priority_get_from_list)() =
        sctk_multiple_queues_with_priority_dynamic_get_from_list;
#endif

#ifdef MULTIFILE_DEFAULT
void (*_mpc_threads_ng_engine_scheduler_increase_prio_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_decrease_prio_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_sched_init_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_task_decrease_prio_ptr)(
        int core) = NULL;
// add_to_list
static void (*sctk_multiple_queues_with_priority_add_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched) =
        sctk_multiple_queues_with_priority_default_add_to_list;
// concat_to_list
static void (*sctk_multiple_queues_with_priority_concat_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list) =
        sctk_multiple_queues_with_priority_default_concat_to_list;
// get_from_list
static _mpc_threads_ng_engine_scheduler_t *(
*sctk_multiple_queues_with_priority_get_from_list)() =
        sctk_multiple_queues_with_priority_default_get_from_list;
#endif

#ifdef MULTIFILE_OMP
void (*_mpc_threads_ng_engine_scheduler_increase_prio_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_decrease_prio_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_sched_init_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_task_decrease_prio_ptr)(
        int core) = NULL;
// add_to_list
static void (*sctk_multiple_queues_with_priority_add_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched) =
        sctk_multiple_queues_with_priority_omp_add_to_list;
// concat_to_list
static void (*sctk_multiple_queues_with_priority_concat_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list) =
        sctk_multiple_queues_with_priority_omp_concat_to_list;
// get_from_list
static _mpc_threads_ng_engine_scheduler_t *(
*sctk_multiple_queues_with_priority_get_from_list)() =
        sctk_multiple_queues_with_priority_omp_get_from_list;
#endif

#ifdef MULTIFILE_NOFAMINE
void (*_mpc_threads_ng_engine_scheduler_increase_prio_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_decrease_prio_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_sched_init_ptr)() = NULL;
void (*_mpc_threads_ng_engine_scheduler_task_decrease_prio_ptr)(
        int core) = NULL;

void (*_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr)(
        int bind_to) = NULL;

// add_to_list
static void (*sctk_multiple_queues_with_priority_add_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched) =
        sctk_multiple_queues_with_priority_nofamine_add_to_list;
// concat_to_list
static void (*sctk_multiple_queues_with_priority_concat_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list) =
        sctk_multiple_queues_with_priority_nofamine_concat_to_list;
// get_from_list
static _mpc_threads_ng_engine_scheduler_t *(
*sctk_multiple_queues_with_priority_get_from_list)() =
        sctk_multiple_queues_with_priority_nofamine_get_from_list;
#endif

#ifdef MULTIFILE_DYN_SORTED_LIST
void (*_mpc_threads_ng_engine_scheduler_sched_init_ptr)() =
        sctk_multiple_queues_with_priority_dyn_sorted_list_sched_NBC_Pthread_sched_init;

void (*_mpc_threads_ng_engine_scheduler_increase_prio_ptr)() =
        sctk_multiple_queues_priority_dyn_sorted_list_sched_NBC_Pthread_sched_increase_priority;

void (*_mpc_threads_ng_engine_scheduler_decrease_prio_ptr)() =
        sctk_multiple_queues_with_priority_dyn_sorted_list_sched_NBC_Pthread_sched_decrease_priority;

void (*_mpc_threads_ng_engine_scheduler_task_decrease_prio_ptr)(
        int core) =
        sctk_multiple_queues_priority_dyn_sorted_list_task_polling_thread_sched_decrease_priority;

void (*_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr)(
        int bind_to) =
        sctk_multiple_queues_priority_dyn_sorted_list_task_polling_thread_sched_increase_priority;

// add_to_list
static void (*sctk_multiple_queues_with_priority_add_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched) =
        sctk_multiple_queues_with_priority_dyn_sorted_list_add_to_list;
// concat_to_list
static void (*sctk_multiple_queues_with_priority_concat_to_list)(
        _mpc_threads_ng_engine_scheduler_t *sched,
        _mpc_threads_ng_engine_scheduler_generic_t *s_list) =
        sctk_multiple_queues_with_priority_dyn_sorted_list_concat_to_list;
// get_from_list
static _mpc_threads_ng_engine_scheduler_t *(
*sctk_multiple_queues_with_priority_get_from_list)() =
        // sctk_multiple_queues_with_priority_dyn_sorted_list_get_from_list;
        sctk_multiple_queues_with_priority_dyn_sorted_list_get_from_list_old;
#endif
////////////////////////////////////////
// END
//

static _mpc_threads_ng_engine_scheduler_t *sctk_multiple_queues_get_from_list_pthread_init()
{
	int core;

	core = core_id;



	if(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list != NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *res_tmp;
		_mpc_threads_ng_engine_scheduler_generic_t *res;
		mpc_common_spinlock_lock(&sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list_lock);
		DL_FOREACH_SAFE(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list, res, res_tmp)
		{
			if( (res != NULL) && (res->vp_type == 4) )
			{
				DL_DELETE(sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list, res);
				break;
			}
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_sched_lists[core].sctk_multiple_queues_sched_list_lock);
		if(res != NULL)
		{
			mpc_common_nodebug("REMOVE Thread %p", res->sched);
			return res->sched;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

static _mpc_threads_ng_engine_task_t *sctk_multiple_queues_get_task()
{
	int core;
	_mpc_threads_ng_engine_task_t *task = NULL;

	core = core_id;


	if(sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list != NULL)
	{
		mpc_common_spinlock_lock(&sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list_lock);
		if(sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list != NULL)
		{
			task = sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list;
			DL_DELETE(sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list, task);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list_lock);
	}
	return task;
}

static void sctk_multiple_queues_add_task_to_proceed(_mpc_threads_ng_engine_task_t *task)
{
	int core;

	core = core_id;

	mpc_common_spinlock_lock(&sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list_lock);
	DL_APPEND(sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list, task);
	// we already do the increment of task_nb line 1321 after we call
	// sctk_multiple_queues_add_task_to_proceed
	////hmt
	// sctk_multiple_queues_task_lists[core].task_nb+=vp_data.delegated_task_nb;
	// vp_data.delegated_task_nb=0;

	// hmt
	// increment task_nb
	//
	// sctk_multiple_queues_task_lists[task->sched->th->attr.bind_to].task_nb+=vp_data.delegated_task_nb;
	// vp_data.delegated_task_nb=0;
	// //we increment the polling thread priority when we add a task on the list

	// _mpc_threads_ng_engine_scheduler_t* sched;
	// sched = &(_mpc_threads_ng_engine_self()->sched);

	// #ifdef SCTK_DEBUG_SCHEDULER
	// {
	//     char hostname[128];
	//     char hostname2[128];
	//     char current_vp[5];
	//     gethostname(hostname,128);
	//     strncpy(hostname2,hostname,128);
	//     sprintf(current_vp,"_%03d",mpc_topology_get_current_cpu ());
	//     strcat(hostname2,current_vp);

	//     FILE* fd=fopen(hostname2,"a");
	//     fprintf(fd,"add task SCHED=%p %d\n\n",
	//             sched,
	//             sctk_multiple_queues_task_lists[sched->th->attr.bind_to].task_nb);
	//     fflush(fd);
	//     fclose(fd);
	// }
	// #endif

	// //endhmt

	////endhmt
	mpc_common_spinlock_unlock(&sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list_lock);
}

static
void sctk_multiple_queues_poll_tasks(_mpc_threads_ng_engine_scheduler_t *sched)
{
	_mpc_threads_ng_engine_task_t *     task;
	_mpc_threads_ng_engine_task_t *     task_tmp;
	_mpc_threads_ng_engine_scheduler_t *lsched = NULL;
	int core;

	core = core_id;
	mpc_common_spinlock_lock(&sctk_multiple_queues_task_lists[core]
	                         .sctk_multiple_queues_task_list_lock);
	{
		/* Exec polling */
		DL_FOREACH_SAFE(
		        sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list,
		        task, task_tmp)
		{
			if(_mpc_threads_ng_engine_scheduler_check_task(task) == 1)
			{
				DL_DELETE(sctk_multiple_queues_task_lists[core]
				          .sctk_multiple_queues_task_list,
				          task);
				// hmt

				sctk_multiple_queues_task_lists[core].task_nb--;
				// we decrement the polling thread priority when we remove a
				// task on the list
				if(_mpc_threads_ng_engine_scheduler_task_decrease_prio_ptr !=
				   NULL)
				{
					_mpc_threads_ng_engine_scheduler_task_decrease_prio_ptr(
					        core);
				}

				// endhmt
				mpc_common_nodebug("WAKE task %p", task);

				mpc_common_spinlock_lock(&(sched->generic.lock) );
				lsched = task->sched;
				if(task->free_func)
				{
					task->free_func(task->arg);
					task->free_func(task);
				}
				if(lsched)
				{
					void **tmp = NULL;
					if(task->sched && task->sched->th && &(task->sched->th->attr) )
					{
						tmp = (void **)task->sched->th->attr
						      ._mpc_threads_ng_engine_pthread_blocking_lock_table;
					}
					if(tmp)
					{
						tmp[MPC_THREADS_GENERIC_TASK_LOCK] = NULL;
					}
					_mpc_threads_ng_engine_wake(lsched);
				}
				mpc_common_spinlock_unlock(&(sched->generic.lock) );
			}
		}
	}
	mpc_common_spinlock_unlock(&sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list_lock);
}

void
sctk_multiple_queues_thread_generic_wake_on_task_lock(_mpc_threads_ng_engine_scheduler_t *sched,
                                                      int remove_from_lock_list)
{
	_mpc_threads_ng_engine_task_t *task_tmp;
	int core;

	core = core_id;

	mpc_common_spinlock_lock(&sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list_lock);
	DL_FOREACH(sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list, task_tmp)
	{
		if(task_tmp->sched == sched)
		{
			break;
		}
	}
	if(remove_from_lock_list && task_tmp)
	{
		*(task_tmp->data) = task_tmp->value;
		mpc_common_spinlock_unlock(&sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list_lock);
	}
	else
	{
		if(task_tmp)
		{
			sctk_generic_swap_to_sched(sched);
		}
		mpc_common_spinlock_unlock(&sctk_multiple_queues_task_lists[core].sctk_multiple_queues_task_list_lock);
	}
}

/***************************************/
/* GENERIC SCHEDULER                   */
/***************************************/
static void (*sctk_generic_add_to_list_intern)(_mpc_threads_ng_engine_scheduler_t *) = NULL;
static _mpc_threads_ng_engine_scheduler_t * (*sctk_generic_get_from_list)(void) = NULL;
static _mpc_threads_ng_engine_scheduler_t * (*sctk_generic_get_from_list_pthread_init)(void) = NULL;
static _mpc_threads_ng_engine_task_t *      (*sctk_generic_get_task)(void) = NULL;
static void (*sctk_generic_add_task_to_proceed)(_mpc_threads_ng_engine_task_t *) = NULL;
static void (*sctk_generic_concat_to_list)(_mpc_threads_ng_engine_scheduler_t *,
                                           _mpc_threads_ng_engine_scheduler_generic_t *) = NULL;
static void (*sctk_generic_poll_tasks)(_mpc_threads_ng_engine_scheduler_t *) = NULL;


static void (*_mpc_threads_ng_engine_wake_on_task_lock_p)(_mpc_threads_ng_engine_scheduler_t *sched,
                                                       int remove_from_lock_list) = NULL;

void _mpc_threads_ng_engine_wake_on_task_lock(
        _mpc_threads_ng_engine_scheduler_t *sched, int remove_from_lock_list)
{
	_mpc_threads_ng_engine_wake_on_task_lock_p(sched, remove_from_lock_list);
}

typedef struct sctk_per_vp_data_s
{
	_mpc_threads_ng_engine_task_t *             sctk_generic_delegated_task_list;
	// int delegated_task_nb;
	_mpc_threads_ng_engine_scheduler_generic_t *sctk_generic_delegated_zombie_detach_thread;
	_mpc_threads_ng_engine_scheduler_t *        sctk_generic_delegated_add;
	mpc_common_spinlock_t *                  sctk_generic_delegated_spinlock;
	_mpc_threads_ng_engine_scheduler_t *        sched_idle;
	mpc_common_spinlock_t *                  registered_spin_unlock;
	_mpc_threads_ng_engine_scheduler_t *        swap_to_sched;
} sctk_per_vp_data_t;

//#define SCTK_PER_VP_DATA_INIT {NULL,0,NULL,NULL,NULL,NULL,NULL,NULL}
#define SCTK_PER_VP_DATA_INIT    { NULL, NULL, NULL, NULL, NULL, NULL, NULL }

static __thread int delegated_task_nb = 0;

static __thread sctk_per_vp_data_t vp_data = SCTK_PER_VP_DATA_INIT;

static int sctk_sem_wait(sem_t *sem)
{
	int res;

	do
	{
		res = sem_wait(sem);
	} while(res != 0);
	mpc_common_nodebug("SEM WAIT %d", res);
	if(res != 0)
	{
		perror("SEM WAIT");
	}
	return res;
}

void _mpc_threads_ng_engine_scheduler_swapcontext_pthread(_mpc_threads_ng_engine_scheduler_t *old_th,
                                                       _mpc_threads_ng_engine_scheduler_t *new_th,
                                                       sctk_per_vp_data_t *vp)
{
	int val;

	assume(new_th->status == _mpc_threads_ng_engine_running);
	assume(sem_getvalue(&(old_th->generic.sem), &val) == 0);
	mpc_common_nodebug("SLEEP %p (%d) WAKE %p %d (%d)", old_th, old_th->status, new_th, val, new_th->status);
	mpc_common_nodebug("Register spinunlock %p execute SWAP", vp->sctk_generic_delegated_spinlock);

	new_th->generic.vp = vp;
	old_th->generic.vp = NULL;
	mpc_common_spinlock_unlock(&(old_th->debug_lock) );
	assume(mpc_common_spinlock_trylock(&(new_th->debug_lock) ) == 0);

	assume(sem_getvalue(&(new_th->generic.sem), &val) == 0);
	assume(val == 0);
	assume(sem_post(&(new_th->generic.sem) ) == 0);

	assume(sctk_sem_wait(&(old_th->generic.sem) ) == 0);
	assume(sem_getvalue(&(old_th->generic.sem), &val) == 0);
	assume(val == 0);

	assume(old_th->status == _mpc_threads_ng_engine_running);
	assume(old_th->generic.vp != NULL);
	memcpy(&vp_data, old_th->generic.vp, sizeof(sctk_per_vp_data_t) );
	mpc_common_nodebug("Register spinunlock %p execute SWAP NEW", old_th->generic.vp->sctk_generic_delegated_spinlock);

	mpc_common_nodebug("RESTART %p", old_th);
}

void _mpc_threads_ng_engine_scheduler_swapcontext_ethread(_mpc_threads_ng_engine_scheduler_t *old_th,
                                                       _mpc_threads_ng_engine_scheduler_t *new_th,
                                                       sctk_per_vp_data_t *vp)
{
	new_th->generic.vp = vp;
	_mpc_threads_ng_engine_scheduler_swapcontext(old_th, new_th);
}

void _mpc_threads_ng_engine_scheduler_swapcontext_nothing(__UNUSED__ _mpc_threads_ng_engine_scheduler_t *old_th,
                                                       __UNUSED__ _mpc_threads_ng_engine_scheduler_t *new_th,
                                                       __UNUSED__ sctk_per_vp_data_t *vp)
{
}

void _mpc_threads_ng_engine_scheduler_swapcontext_none(__UNUSED__ _mpc_threads_ng_engine_scheduler_t *old_th,
                                                    __UNUSED__ _mpc_threads_ng_engine_scheduler_t *new_th,
                                                    __UNUSED__ sctk_per_vp_data_t *vp)
{
	not_reachable();
}

static void sctk_generic_add_to_list(_mpc_threads_ng_engine_scheduler_t *sched, int is_idle_mode)
{
	assume(sched->status == _mpc_threads_ng_engine_running);
	mpc_common_nodebug("ADD TASK %p idle mode %d", sched, is_idle_mode);
	if(is_idle_mode == 0)
	{
		sctk_generic_add_to_list_intern(sched);
	}
}

static void sctk_generic_add_task(_mpc_threads_ng_engine_task_t *task)
{
	void **tmp = NULL;

	if(task->sched && task->sched->th && &(task->sched->th->attr) )
	{
		tmp = (void **)task->sched->th->attr
		      ._mpc_threads_ng_engine_pthread_blocking_lock_table;
	}
	if(tmp)
	{
		tmp[MPC_THREADS_GENERIC_TASK_LOCK] = (void *)1;
	}
	mpc_common_nodebug("ADD task %p FROM %p", task, task->sched);
	if(task->is_blocking)
	{
		_mpc_threads_ng_engine_thread_status(task->sched, _mpc_threads_ng_engine_blocked);
		assume(vp_data.sctk_generic_delegated_task_list == NULL);
		DL_APPEND(vp_data.sctk_generic_delegated_task_list, task);
		// hmt
		// increment delegated_task_nb
		delegated_task_nb++;
		// endhmt
		mpc_common_spinlock_lock(&(task->sched->generic.lock) );
		_mpc_threads_ng_engine_register_spinlock_unlock(task->sched,
		                                             &(task->sched->generic.lock) );
		_mpc_threads_ng_engine_sched_yield(task->sched);
	}
	else
	{
		sctk_generic_add_task_to_proceed(task);
		// hmt
		// increment task_nb
		sctk_multiple_queues_task_lists[task->sched->th->attr.bind_to].task_nb +=
		        delegated_task_nb;
		delegated_task_nb = 0;
		// we increment the polling thread priority when we add a task on the list
		if(_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr !=
		   NULL)
		{
			_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr(
			        task->sched->th->attr.bind_to);
		}
		// endhmt
	}
	mpc_common_nodebug("ADD task %p FROM %p DONE", task, task->sched);
}

static void sctk_generic_sched_yield(_mpc_threads_ng_engine_scheduler_t *sched);

static void sctk_generic_sched_idle_start()
{
	_mpc_threads_ng_engine_scheduler_t *next;

	do
	{
		sched_yield();
		next = sctk_generic_get_from_list();
	} while(next == NULL);
	mpc_common_nodebug("Launch %p", next);
	sctk_swapcontext(&(next->ctx_bootstrap), &(next->ctx) );
	not_reachable();
}

static void sctk_generic_sched_idle_start_pthread()
{
	_mpc_threads_ng_engine_scheduler_t *next;

	vp_data.sctk_generic_delegated_task_list            = NULL;
	vp_data.sctk_generic_delegated_zombie_detach_thread = NULL;
	vp_data.sctk_generic_delegated_add = NULL;

	do
	{
		sched_yield();
		next = sctk_generic_get_from_list_pthread_init();
	} while(next == NULL);
	mpc_common_nodebug("Launch PTHREAD %p", next);
	sctk_swapcontext(&(next->ctx_bootstrap), &(next->ctx) );
	not_reachable();
}

static _mpc_threads_ng_engine_scheduler_t *sctk_generic_get_from_list_none()
{
	return NULL;
}

static void sctk_generic_sched_yield_intern(
        _mpc_threads_ng_engine_scheduler_t *sched,
        void (*swap)(_mpc_threads_ng_engine_scheduler_t *old_th,
                     _mpc_threads_ng_engine_scheduler_t *new_th,
                     sctk_per_vp_data_t *vp),
        _mpc_threads_ng_engine_scheduler_t *(*get_from_list)() )
{
#ifdef SCTK_DEBUG_SCHEDULER
	{
		char hostname[128];
		char hostname2[128];
		char current_vp[5];
		gethostname(hostname, 128);
		strncpy(hostname2, hostname, 128);
		sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
		strcat(hostname2, current_vp);

		FILE *fd = fopen(hostname2, "a");
		fprintf(fd, "\nsctk_generic_sched_yield_intern %p[%2d-%2d|%2d,%2d] --> %p "
		            "%p hook_mpc_polling %p hook_NBC %p\n",
		        sched, sched->th->attr.kind.mask, sched->status,
		        sched->th->attr.current_priority, sched->th->attr.basic_priority,
		        sched->generic.sched, sched->th,
		        sctk_multiple_queues_task_lists[sched->th->attr.bind_to]
		        .sctk_multiple_queues_task_polling_thread_sched
		        ? sctk_multiple_queues_task_lists[sched->th->attr.bind_to]
		        .sctk_multiple_queues_task_polling_thread_sched
			: NULL,
		        sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
		        .sctk_multiple_queues_sched_NBC_Pthread_sched
		        ? sctk_multiple_queues_sched_lists[sched->th->attr.bind_to]
		        .sctk_multiple_queues_sched_NBC_Pthread_sched
			: NULL);
		fflush(fd);
		fclose(fd);
	}
#endif

	// next thread to schedule
	_mpc_threads_ng_engine_scheduler_t *next;


	// bypass get_from_list ?
	if(vp_data.swap_to_sched != NULL)
	{
		next = vp_data.swap_to_sched;
		vp_data.swap_to_sched = NULL;
		goto quick_swap;
	}

	////hmt TODO //attention quand sched->th->attr.bind_to vaut -1
	// if(sctk_multiple_queues_sched_lists[sched->th->attr.bind_to].sctk_multiple_queues_sched_list
	// != NULL){
	//    sched->th->attr.current_priority = 10;

	//    _mpc_threads_ng_engine_scheduler_generic_t* res;
	//    res =
	//    sctk_multiple_queues_sched_lists[sched->th->attr.bind_to].sctk_multiple_queues_sched_list;
	//    res->sched->th->attr.current_priority = 10;
	//}
	////endhmt

	// Register the current thread for a futur insertion into the ready thread
	// list
	if(vp_data.sctk_generic_delegated_add)
	{
#ifdef SCTK_DEBUG_SCHEDULER
		{
			char hostname[128];
			char hostname2[128];
			char current_vp[5];
			gethostname(hostname, 128);
			strncpy(hostname2, hostname, 128);
			sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
			strcat(hostname2, current_vp);

			FILE *fd = fopen(hostname2, "a");
			fprintf(fd,
			        "if(vp_data.sctk_generic_delegated_add) 1 add_to_list %p %p\n",
			        sched, vp_data.sctk_generic_delegated_add);
			fflush(fd);
			fclose(fd);
		}
#endif

		sctk_generic_add_to_list(
		        vp_data.sctk_generic_delegated_add,
		        vp_data.sctk_generic_delegated_add->generic.is_idle_mode);
		vp_data.sctk_generic_delegated_add = NULL;
	}

	if(sched->status == _mpc_threads_ng_engine_running)
	{
		assume(vp_data.sctk_generic_delegated_add == NULL);
		vp_data.sctk_generic_delegated_add = sched;

		// Register a polling task for futur execution
		if(vp_data.sctk_generic_delegated_task_list != NULL)
		{
			sctk_generic_add_task_to_proceed(
			        vp_data.sctk_generic_delegated_task_list);
			// hmt
			sctk_multiple_queues_task_lists[sched->th->attr.bind_to].task_nb +=
			        delegated_task_nb;
			delegated_task_nb = 0;
			// we increment the polling thread priority when we add a task on the list
			if(_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr !=
			   NULL)
			{
				_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr(
				        sched->th->attr.bind_to);
			}
			// endhmt
			vp_data.sctk_generic_delegated_task_list = NULL;
		}
	}
	else
	{
		mpc_common_nodebug("TASK %p status %d type %d %d %d", sched, sched->status,
		             sched->generic.vp_type, _mpc_threads_ng_engine_zombie,
		             sched->th->attr.cancel_status);
		// Register the current thread for a futur destruction (zombie state)
		if(sched->status == _mpc_threads_ng_engine_zombie &&
		   sched->th->attr.detachstate == SCTK_THREAD_CREATE_DETACHED)
		{
			assume(vp_data.sctk_generic_delegated_zombie_detach_thread == NULL);
			vp_data.sctk_generic_delegated_zombie_detach_thread = &(sched->generic);
			mpc_common_nodebug("Detached Zombie %p", sched);
		}

		if(sched->status == _mpc_threads_ng_engine_zombie)
		{
			mpc_common_nodebug("Attached Zombie %p", sched);
		}
	}

	sched->generic.is_idle_mode = 0;

retry:
	next = get_from_list();

	if( (next == NULL) && (sched->status == _mpc_threads_ng_engine_running) )
	{
#ifdef SCTK_DEBUG_SCHEDULER
		{
			char hostname[128];
			char hostname2[128];
			char current_vp[5];
			gethostname(hostname, 128);
			strncpy(hostname2, hostname, 128);
			sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
			strcat(hostname2, current_vp);

			FILE *fd = fopen(hostname2, "a");
			fprintf(fd, "if((next == NULL) && (sched->status == "
			            "_mpc_threads_ng_engine_running)) %p\n\n",
			        sched);
			fflush(fd);
			fclose(fd);
		}
#endif
		vp_data.sctk_generic_delegated_add = NULL;
		next = sched;
	}

quick_swap:

	//#ifdef SCTK_DEBUG_SCHEDULER
	//{
	//    char hostname[128];
	//    char hostname2[128];
	//    char current_vp[5];
	//    gethostname(hostname,128);
	//    strncpy(hostname2,hostname,128);
	//    sprintf(current_vp,"_%03d",mpc_topology_get_current_cpu ());
	//    strcat(hostname2,current_vp);

	//    FILE* fd=fopen(hostname2,"a");
	//    fprintf(fd,"label quick_swap next=%p sched=%p\n\n",next, sched);
	//    fflush(fd);
	//    fclose(fd);
	//}
	//#endif
	if(next != sched)
	{
		if(next != NULL)
		{
			mpc_common_nodebug("SWAP from %p to %p", sched, next);
			mpc_common_nodebug("SLEEP TASK %p status %d type %d %d cancel status %d", sched,
			             sched->status, sched->generic.vp_type,
			             _mpc_threads_ng_engine_zombie, sched->th->attr.cancel_status);

			// timers
			if(sched->th->attr.timestamp_base != -1)
			{
				sched->th->attr.timestamp_end    = mpc_arch_get_timestamp_gettimeofday();
				sched->th->attr.timestamp_count +=
				        sched->th->attr.timestamp_end - sched->th->attr.timestamp_begin;

#ifdef SCTK_DEBUG_SCHEDULER
				char hostname[128];
				char hostname2[128];
				char current_vp[5];
				gethostname(hostname, 128);
				strncpy(hostname2, hostname, 128);
				sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
				strcat(hostname2, current_vp);

				FILE *fd = fopen(hostname2, "a");
				mpc_common_debug_log_file(fd, "%p[%2d-%2d|%2d,%2d] to %p[%2d-%2d|%2d,%2d] t=[%f "
				                              "count(%f)]\n",
				                          sched, sched->th->attr.kind.mask, sched->status,
				                          sched->th->attr.current_priority,
				                          sched->th->attr.basic_priority,

				                          next, next->th->attr.kind.mask, sched->status,
				                          next->th->attr.current_priority, next->th->attr.basic_priority,

				                          // sched->th->attr.timestamp_base,
				                          // sched->th->attr.timestamp_begin,
				                          // sched->th->attr.timestamp_end,
				                          sched->th->attr.timestamp_end - sched->th->attr.timestamp_begin,
				                          sched->th->attr.timestamp_count);
				fflush(fd);
				fclose(fd);
// endhmt
#endif                          // SCTK_DEBUG_SCHEDULER
			}
			// endtimers

			/////////////////////////////////////////////////////////////
			swap(sched, next, &vp_data);
			/////////////////////////////////////////////////////////////

			// timers
			if(sched->th->attr.timestamp_base == -1)
			{
				sched->th->attr.timestamp_threshold = _mpc_thread_config_get()->scheduler_quantum;
				sched->th->attr.timestamp_base  = mpc_arch_get_timestamp_gettimeofday();
				sched->th->attr.timestamp_begin = sched->th->attr.timestamp_base;
			}
			else
			{
				sched->th->attr.timestamp_begin = mpc_arch_get_timestamp_gettimeofday();
			}
			// endtimers

			mpc_common_nodebug("Enter %p", sched);
			mpc_common_nodebug("WAKE TASK %p status %d type %d %d cancel status %d", sched,
			             sched->status, sched->generic.vp_type,
			             _mpc_threads_ng_engine_zombie, sched->th->attr.cancel_status);
		}
		else
		{
			/* Idle function */

			if(vp_data.sctk_generic_delegated_task_list != NULL)
			{
				sched->generic.is_idle_mode = 1;
				sctk_generic_add_task_to_proceed(
				        vp_data.sctk_generic_delegated_task_list);
				// hmt
				sctk_multiple_queues_task_lists[sched->th->attr.bind_to].task_nb += delegated_task_nb;
				delegated_task_nb = 0;
				// we increment the polling thread priority when we add a task on the
				// list
				if(_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr !=
				   NULL)
				{
					_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr(
					        sched->th->attr.bind_to);
				}
				// endhmt
				vp_data.sctk_generic_delegated_task_list = NULL;
			}

			if(vp_data.sctk_generic_delegated_spinlock != NULL)
			{
				sched->generic.is_idle_mode = 1;


				assume(vp_data.registered_spin_unlock == NULL);

				mpc_common_nodebug("REGISTER delegated IDLE %p", sched);
				vp_data.sched_idle             = sched;
				vp_data.registered_spin_unlock =
				        vp_data.sctk_generic_delegated_spinlock;

				mpc_common_nodebug("Register spinunlock %p execute IDLE",
				             vp_data.sctk_generic_delegated_spinlock);
				mpc_common_spinlock_unlock(vp_data.sctk_generic_delegated_spinlock);
				vp_data.sctk_generic_delegated_spinlock = NULL;
			}

			sctk_cpu_relax();

			//#ifdef SCTK_DEBUG_SCHEDULER
			//{
			//    char hostname[128];
			//    char hostname2[128];
			//    char current_vp[5];
			//    gethostname(hostname,128);
			//    strncpy(hostname2,hostname,128);
			//    sprintf(current_vp,"_%03d",mpc_topology_get_current_cpu ());
			//    strcat(hostname2,current_vp);

			//    FILE* fd=fopen(hostname2,"a");
			//    fprintf(fd,"before retry sched=%p\n", sched);
			//    fflush(fd);
			//    fclose(fd);
			//}
			//#endif
			goto retry;
		}
	}
	/*New thread if next != nil*/

	sched->generic.is_idle_mode = 0;

	if(vp_data.sched_idle != NULL)
	{
		if(vp_data.sched_idle != sched)
		{
			if(vp_data.registered_spin_unlock != NULL)
			{
				mpc_common_spinlock_lock(vp_data.registered_spin_unlock);
				vp_data.sched_idle->generic.is_idle_mode = 0;
				if(vp_data.sched_idle->status == _mpc_threads_ng_engine_running)
				{
					mpc_common_nodebug("ADD FROM delegated spinlock %p", vp_data.sched_idle);

#ifdef SCTK_DEBUG_SCHEDULER
					// hmt
					printf("if(vp_data.sched_idle->status == "
					       "_mpc_threads_ng_engine_running) add_to_list\n");
					fflush(stdout);
// endhmt
#endif
					sctk_generic_add_to_list(vp_data.sched_idle,
					                         vp_data.sched_idle->generic.is_idle_mode);
				}
				mpc_common_spinlock_unlock(vp_data.registered_spin_unlock);
			}
		}
		vp_data.registered_spin_unlock = NULL;
		vp_data.sched_idle             = NULL;
	}

	if(vp_data.sctk_generic_delegated_spinlock != NULL)
	{
		mpc_common_nodebug("Register spinunlock %p execute",
		             vp_data.sctk_generic_delegated_spinlock);
		mpc_common_spinlock_unlock(vp_data.sctk_generic_delegated_spinlock);
		vp_data.sctk_generic_delegated_spinlock = NULL;
	}

	mpc_common_nodebug("TASK %p status %d type %d _mpc_threads_ng_engine_check_signals",
	             sched, sched->status, sched->generic.vp_type);
	_mpc_threads_ng_engine_check_signals(1);

	mpc_common_nodebug("TASK %p status %d type %d _mpc_threads_ng_engine_check_signals done %p ",
	        sched, sched->status, sched->generic.vp_type, vp_data.swap_to_sched);

	/* To ensure that blocked threads in synchronizaton locks are still able to
	 * receive
	 * and treat signals for the pthread api, we save the calling thread and
	 * schedule the
	 * receiving thread. Once signal has been treated, calling thread is scheduled
	 * again.
	 */
	if(vp_data.swap_to_sched != NULL)
	{
		next = vp_data.swap_to_sched;
		vp_data.swap_to_sched = NULL;
		goto quick_swap;
	}

	mpc_common_nodebug("TASK %p status %d type %d Deal with zombie threads", sched,
	             sched->status, sched->generic.vp_type);
	/* Deal with zombie threads */
	if(vp_data.sctk_generic_delegated_zombie_detach_thread != NULL)
	{
		// _mpc_threads_ng_engine_handle_zombies(
		// vp_data.sctk_generic_delegated_zombie_detach_thread );
		//#warning "Handel Zombies"
		vp_data.sctk_generic_delegated_zombie_detach_thread = NULL;
	}

	if(vp_data.sctk_generic_delegated_task_list != NULL)
	{
		sctk_generic_add_task_to_proceed(vp_data.sctk_generic_delegated_task_list);
		// hmt
		sctk_multiple_queues_task_lists[sched->th->attr.bind_to].task_nb += delegated_task_nb;
		delegated_task_nb = 0;
		// we increment the polling thread priority when we add a task on the list
		if(_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr != NULL)
		{
			_mpc_threads_ng_engine_scheduler_task_increase_prio_ptr(
			        sched->th->attr.bind_to);
		}
		// endhmt
		vp_data.sctk_generic_delegated_task_list = NULL;
	}

	if(vp_data.sctk_generic_delegated_add)
	{
#ifdef SCTK_DEBUG_SCHEDULER
		{
			char hostname[128];
			char hostname2[128];
			char current_vp[5];
			gethostname(hostname, 128);
			strncpy(hostname2, hostname, 128);
			sprintf(current_vp, "_%03d", mpc_topology_get_current_cpu() );
			strcat(hostname2, current_vp);

			FILE *fd = fopen(hostname2, "a");
			fprintf(fd,
			        "if(vp_data.sctk_generic_delegated_add) 2 add_to_list %p %p\n\n",
			        sched, vp_data.sctk_generic_delegated_add);
			fflush(fd);
			fclose(fd);
		}
#endif

		sctk_generic_add_to_list(
		        vp_data.sctk_generic_delegated_add,
		        vp_data.sctk_generic_delegated_add->generic.is_idle_mode);
		vp_data.sctk_generic_delegated_add = NULL;
	}
}

static void sctk_generic_sched_yield(_mpc_threads_ng_engine_scheduler_t *sched)
{
	if(sched->generic.vp_type == 0)
	{
		sctk_generic_sched_yield_intern(sched, _mpc_threads_ng_engine_scheduler_swapcontext_ethread,
		                                sctk_generic_get_from_list);
	}
	else
	{
		if(sched->generic.vp_type == 1)
		{
			sctk_generic_sched_yield_intern(sched, _mpc_threads_ng_engine_scheduler_swapcontext_none,
			                                sctk_generic_get_from_list_none);
		}
		else
		{
			if(sched->generic.vp_type == 4)
			{
				sched->generic.vp_type = 3;
				mpc_common_spinlock_unlock(&(sched->debug_lock) );
				assume(sctk_sem_wait(&(sched->generic.sem) ) == 0);
				memcpy(&vp_data, sched->generic.vp, sizeof(sctk_per_vp_data_t) );
				mpc_common_nodebug("Register spinunlock %p execute SWAP NEW FIRST", sched->generic.vp->sctk_generic_delegated_spinlock);
				return;
			}
			sctk_generic_sched_yield_intern(sched, _mpc_threads_ng_engine_scheduler_swapcontext_pthread,
			                                sctk_generic_get_from_list);
		}
	}
}

void sctk_generic_swap_to_sched(_mpc_threads_ng_engine_scheduler_t *sched)
{
	_mpc_threads_ng_engine_scheduler_t *old_sched = &(_mpc_threads_ng_engine_self()->sched);

	vp_data.swap_to_sched = old_sched;

	if(sched->generic.vp_type == 0)
	{
		_mpc_threads_ng_engine_scheduler_swapcontext_ethread(old_sched,
		                                                  sched, &vp_data);
	}
	else
	{
		_mpc_threads_ng_engine_scheduler_swapcontext_pthread(old_sched,
		                                                  sched, &vp_data);
	}
}

static
void sctk_generic_thread_status(_mpc_threads_ng_engine_scheduler_t *sched,
                                _mpc_threads_ng_engine_thread_status_t status)
{
	sched->status = status;
}

static void sctk_generic_register_spinlock_unlock(__UNUSED__ _mpc_threads_ng_engine_scheduler_t *sched,
                                                  mpc_common_spinlock_t *lock)
{
	mpc_common_nodebug("Register spinunlock %p", lock);
	vp_data.sctk_generic_delegated_spinlock = lock;
}

static void sctk_generic_wake(_mpc_threads_ng_engine_scheduler_t *sched)
{
	int is_idle_mode;

	is_idle_mode = sched->generic.is_idle_mode;

	mpc_common_nodebug("WAKE %p", sched);

	if(sched->generic.vp_type == 0)
	{
		sched->status = _mpc_threads_ng_engine_running;
		sctk_generic_add_to_list(sched, is_idle_mode);
	}
	else
	{
		sched->status = _mpc_threads_ng_engine_running;
		if(sched->generic.vp_type == 1)
		{
			sem_post(&(sched->generic.sem) );
		}
		else
		{
			sctk_generic_add_to_list(sched, is_idle_mode);
		}
	}
}

static void
sctk_generic_freeze_thread_on_vp(mpc_thread_mutex_t *lock, void **list)
{
	_mpc_threads_ng_engine_scheduler_t *        sched;
	_mpc_threads_ng_engine_scheduler_generic_t *s_list;

	sched = &(_mpc_threads_ng_engine_self()->sched);

	s_list = *list;
	DL_APPEND(s_list, &(sched->generic) );
	*list = s_list;

	mpc_thread_mutex_unlock(lock);
	sctk_generic_thread_status(sched, _mpc_threads_ng_engine_blocked);
	sctk_generic_sched_yield(sched);
}

static void
sctk_generic_wake_thread_on_vp(void **list)
{
	if(*list != NULL)
	{
		_mpc_threads_ng_engine_scheduler_generic_t *s_list;
		_mpc_threads_ng_engine_scheduler_t *        sched;
		s_list = *list;
		sched  = &(_mpc_threads_ng_engine_self()->sched);
		sctk_generic_concat_to_list(sched, s_list);
		*list = NULL;
	}
}

static void sctk_generic_create_common(_mpc_threads_ng_engine_p_t *thread)
{
	if(thread->attr.scope == SCTK_THREAD_SCOPE_SYSTEM)
	{
		thread->sched.generic.vp_type = 1;

		mpc_common_nodebug("Create thread scope %d (%d SYSTEM) vp _type %d",
		             thread->attr.scope, SCTK_THREAD_SCOPE_SYSTEM, thread->sched.generic.vp_type);
		_mpc_threads_ng_engine_scheduler_create_vp(thread, thread->attr.bind_to);
	}
	else
	{
		mpc_common_nodebug("Create thread scope %d (%d SYSTEM) vp _type %d",
		             thread->attr.scope, SCTK_THREAD_SCOPE_SYSTEM, thread->sched.generic.vp_type);
		sctk_generic_add_to_list(&(thread->sched), (&(thread->sched) )->generic.is_idle_mode);
	}
}

static void sctk_generic_create(_mpc_threads_ng_engine_p_t *thread)
{
	_mpc_threads_ng_engine_scheduler_init_thread(&(thread->sched), thread);
	sctk_generic_create_common(thread);
}

static void sctk_generic_create_pthread(_mpc_threads_ng_engine_p_t *thread)
{
	_mpc_threads_ng_engine_scheduler_init_thread(&(thread->sched), thread);
	thread->sched.generic.vp_type = 4;

	if(thread->attr.scope != SCTK_THREAD_SCOPE_SYSTEM)
	{
		_mpc_threads_ng_engine_scheduler_create_vp(thread, thread->attr.bind_to);
		sctk_generic_add_to_list(&(thread->sched), (&(thread->sched) )->generic.is_idle_mode);
	}
	else
	{
		sctk_generic_create_common(thread);
	}
}

static void *sctk_generic_polling_func(void *arg)
{
	_mpc_threads_ng_engine_scheduler_t *sched;

	assume(arg == NULL);

	sched = &(_mpc_threads_ng_engine_self()->sched);

	// set a hook on sctk_multiple_queues_task_lists[sched->th->attr.bind_to]
	// allowing accesibility to the polling thread from all threads
	sctk_multiple_queues_task_lists[sched->th->attr.bind_to]
    .sctk_multiple_queues_task_polling_thread_sched = sched;

	mpc_common_nodebug("Start polling func %p", sched);

	do
	{
		sctk_generic_poll_tasks(sched);
		sctk_generic_sched_yield(sched);
	} while(1);

	return (void *)0;
}

static void sctk_generic_scheduler_init_thread_common(_mpc_threads_ng_engine_scheduler_t *sched)
{
	sched->generic.sched        = sched;
	sched->generic.next         = NULL;
	sched->generic.prev         = NULL;
	sched->generic.is_idle_mode = 0;
	mpc_common_spinlock_init(&sched->generic.lock, 0);

	assume(sem_init(&(sched->generic.sem), 0, 0) == 0);
	mpc_common_nodebug("INIT DONE FOR TASK %p status %d type %d %d cancel status %d %p", sched, sched->status, sched->generic.vp_type, _mpc_threads_ng_engine_zombie, sched->th->attr.cancel_status, sched->th);
	/* assume(sched->th->attr.cancel_status == 0); */
}

static void sctk_generic_scheduler_init_thread(_mpc_threads_ng_engine_scheduler_t *sched)
{
	sched->generic.vp_type = 0;
	sctk_generic_scheduler_init_thread_common(sched);
}

static void sctk_generic_scheduler_init_pthread(_mpc_threads_ng_engine_scheduler_t *sched)
{
	sched->generic.vp_type = 3;
	sctk_generic_scheduler_init_thread_common(sched);
}

/***************************************/
/* INIT                                */
/***************************************/

static void *_mpc_threads_ng_engine_polling_func_bootstrap(void *attr)
{
	return _mpc_threads_ng_engine_polling_func(attr);
}

// Initialization of function pointers which are used for the scheduling on yield funtion
static char sched_type[4096];
void _mpc_threads_ng_engine_scheduler_init(char *thread_type, char *scheduler_type, int vp_number)
{
	int i;

	if(vp_number > mpc_topology_get_pu_count() )
	{
		_mpc_threads_ng_engine_scheduler_use_binding = 0;
	}

	sprintf(sched_type, "%s", scheduler_type);
	core_id = 0;
	_mpc_threads_ng_engine_scheduler_bind_to_cpu(0);

	///////////////////////////
	// update function pointers
  //

	// generic/centralized scheduler
	if(strcmp("generic/centralized", scheduler_type) == 0)
	{
		fprintf(stderr, "SCHEDULER : generic/centralized\n");
		sctk_generic_add_to_list_intern                   = sctk_centralized_add_to_list;    // add to list
		sctk_generic_get_from_list                        = sctk_centralized_get_from_list;  // get from list
		sctk_generic_concat_to_list                       = sctk_centralized_concat_to_list; // concat to list
		sctk_generic_get_task                             = sctk_centralized_get_task;
		sctk_generic_add_task_to_proceed                  = sctk_centralized_add_task_to_proceed;
		sctk_generic_poll_tasks                           = sctk_centralized_poll_tasks;

		_mpc_threads_ng_engine_wake_on_task_lock_p        = sctk_centralized_thread_generic_wake_on_task_lock;
		_mpc_threads_ng_engine_sched_idle_start           = sctk_generic_sched_idle_start;
		_mpc_threads_ng_engine_sched_yield                = sctk_generic_sched_yield;
		_mpc_threads_ng_engine_thread_status              = sctk_generic_thread_status;
		_mpc_threads_ng_engine_register_spinlock_unlock   = sctk_generic_register_spinlock_unlock;
		_mpc_threads_ng_engine_wake                       = sctk_generic_wake;
		_mpc_threads_ng_engine_scheduler_init_thread_p    = sctk_generic_scheduler_init_thread;
		_mpc_threads_ng_engine_sched_create               = sctk_generic_create;
		_mpc_threads_ng_engine_add_task                   = sctk_generic_add_task;
    _mpc_threads_ng_engine_polling_func               = sctk_generic_polling_func;

		sctk_add_func_type(sctk_generic, freeze_thread_on_vp, void (*)(mpc_thread_mutex_t *, void **) );
		sctk_add_func(sctk_generic, wake_thread_on_vp);

		if(strcmp("pthread", thread_type) == 0)
		{
			sctk_generic_get_from_list_pthread_init         = sctk_centralized_get_from_list_pthread_init;

			_mpc_threads_ng_engine_sched_create             = sctk_generic_create_pthread;
			_mpc_threads_ng_engine_scheduler_init_thread_p  = sctk_generic_scheduler_init_pthread;
			_mpc_threads_ng_engine_sched_idle_start         = sctk_generic_sched_idle_start_pthread;
		}
	}

	// generic/multiple_queues scheduler
	else if(strcmp("generic/multiple_queues", scheduler_type) == 0)
	{
		fprintf(stderr, "SCHEDULER : generic/multiple_queues\n");
		sctk_generic_add_to_list_intern                   = sctk_multiple_queues_add_to_list;    // add to list
		sctk_generic_get_from_list                        = sctk_multiple_queues_get_from_list;  // get from list
		sctk_generic_concat_to_list                       = sctk_multiple_queues_concat_to_list; // concat to list
		sctk_generic_get_task                             = sctk_multiple_queues_get_task;
		sctk_generic_add_task_to_proceed                  = sctk_multiple_queues_add_task_to_proceed;
		sctk_generic_poll_tasks                           = sctk_multiple_queues_poll_tasks;

		_mpc_threads_ng_engine_wake_on_task_lock_p        = sctk_multiple_queues_thread_generic_wake_on_task_lock;
		_mpc_threads_ng_engine_sched_idle_start           = sctk_generic_sched_idle_start;
		_mpc_threads_ng_engine_sched_yield                = sctk_generic_sched_yield;
		_mpc_threads_ng_engine_thread_status              = sctk_generic_thread_status;
		_mpc_threads_ng_engine_register_spinlock_unlock   = sctk_generic_register_spinlock_unlock;
		_mpc_threads_ng_engine_wake                       = sctk_generic_wake;
		_mpc_threads_ng_engine_scheduler_init_thread_p    = sctk_generic_scheduler_init_thread;
		_mpc_threads_ng_engine_sched_create               = sctk_generic_create;
		_mpc_threads_ng_engine_add_task                   = sctk_generic_add_task;

		sctk_add_func_type(sctk_generic, freeze_thread_on_vp, void (*)(mpc_thread_mutex_t *, void **) );
		sctk_add_func(sctk_generic, wake_thread_on_vp); _mpc_threads_ng_engine_polling_func = sctk_generic_polling_func;

		if(strcmp("pthread", thread_type) == 0)
		{
			sctk_generic_get_from_list_pthread_init        = sctk_multiple_queues_get_from_list_pthread_init;

			_mpc_threads_ng_engine_sched_create            = sctk_generic_create_pthread;
			_mpc_threads_ng_engine_scheduler_init_thread_p = sctk_generic_scheduler_init_pthread;
			_mpc_threads_ng_engine_sched_idle_start        = sctk_generic_sched_idle_start_pthread;
		}

		sctk_multiple_queues_sched_lists = malloc(vp_number * sizeof(sctk_multiple_queues_sched_list_t) );

		for(i = 0; i < vp_number; i++)
		{
			sctk_multiple_queues_sched_list_t init = SCTK_MULTIPLE_QUEUES_SCHED_LIST_INIT;
			sctk_multiple_queues_sched_lists[i]    = init;
		}

		sctk_multiple_queues_task_lists = malloc(vp_number * sizeof(sctk_multiple_queues_task_list_t) );

		for(i = 0; i < vp_number; i++)
		{
			sctk_multiple_queues_task_list_t init  = SCTK_MULTIPLE_QUEUES_TASK_LIST_INIT;
			sctk_multiple_queues_task_lists[i]     = init;
		}
		//////////////////////////////////////////////
		// we can add others scheduling algorithm  here
		//
	}
	else if(strcmp("generic/multiple_queues_with_priority", scheduler_type) == 0)
	{
		fprintf(stderr, "SCHEDULER : generic/multiple_queues_with_priority\n");
		sctk_generic_add_to_list_intern                   = sctk_multiple_queues_with_priority_add_to_list;   // add to list
		sctk_generic_get_from_list                        = sctk_multiple_queues_with_priority_get_from_list; // get from list
		sctk_generic_concat_to_list                       = sctk_multiple_queues_with_priority_concat_to_list; // concat to list
		sctk_generic_get_task                             = sctk_multiple_queues_get_task;
		sctk_generic_add_task_to_proceed                  = sctk_multiple_queues_add_task_to_proceed;
		sctk_generic_poll_tasks                           = sctk_multiple_queues_poll_tasks;

		_mpc_threads_ng_engine_wake_on_task_lock_p        = sctk_multiple_queues_thread_generic_wake_on_task_lock;
		_mpc_threads_ng_engine_sched_idle_start           = sctk_generic_sched_idle_start;
		_mpc_threads_ng_engine_sched_yield                = sctk_generic_sched_yield;
		_mpc_threads_ng_engine_thread_status              = sctk_generic_thread_status;
		_mpc_threads_ng_engine_register_spinlock_unlock   = sctk_generic_register_spinlock_unlock;
		_mpc_threads_ng_engine_wake                       = sctk_generic_wake;
		_mpc_threads_ng_engine_scheduler_init_thread_p    = sctk_generic_scheduler_init_thread;
		_mpc_threads_ng_engine_sched_create               = sctk_generic_create;
		_mpc_threads_ng_engine_add_task                   = sctk_generic_add_task;
		_mpc_threads_ng_engine_polling_func               = sctk_generic_polling_func;

		sctk_add_func_type(sctk_generic, freeze_thread_on_vp, void (*)(mpc_thread_mutex_t *, void **) );
		sctk_add_func(sctk_generic, wake_thread_on_vp);

		if(strcmp("pthread", thread_type) == 0)
		{
			sctk_generic_get_from_list_pthread_init         = sctk_multiple_queues_get_from_list_pthread_init;
			_mpc_threads_ng_engine_sched_create             = sctk_generic_create_pthread;
			_mpc_threads_ng_engine_scheduler_init_thread_p  = sctk_generic_scheduler_init_pthread;
			_mpc_threads_ng_engine_sched_idle_start         = sctk_generic_sched_idle_start_pthread;
		}

		sctk_multiple_queues_sched_lists = malloc(vp_number * sizeof(sctk_multiple_queues_sched_list_t) );

		for(i = 0; i < vp_number; i++)
		{
			sctk_multiple_queues_sched_list_t init = SCTK_MULTIPLE_QUEUES_SCHED_LIST_INIT;
			sctk_multiple_queues_sched_lists[i]    = init;
		}

		sctk_multiple_queues_task_lists = malloc(vp_number * sizeof(sctk_multiple_queues_task_list_t) );

		for(i = 0; i < vp_number; i++)
		{
			sctk_multiple_queues_task_list_t init = SCTK_MULTIPLE_QUEUES_TASK_LIST_INIT;
			sctk_multiple_queues_task_lists[i]    = init;
		}

		//////////////////////////////////////////////
		// End of new schedulers
		//
	}
	else
	{
		not_reachable();
	}

	if(strcmp("pthread", thread_type) != 0)
	{
		for(i = 1; i < vp_number; i++)
		{
			_mpc_threads_ng_engine_scheduler_create_vp(NULL, i);
		}
	}

	mpc_topology_set_pu_count(vp_number);
}

void _mpc_threads_ng_engine_polling_init(int vp_number)
{
	int i;
	mpc_thread_attr_t attr;
	mpc_thread_t      threadp;

	mpc_thread_attr_init(&attr);

	_mpc_threads_ng_engine_attr_t *attr_intern;
	attr_intern                  = (_mpc_threads_ng_engine_attr_t *)&attr;
	attr_intern->ptr->polling    = 1;
	attr_intern->ptr->stack_size = 8 * 1024;

	// add kind in the attr of the POLLING THREADS
	if(mpc_common_get_flags()->new_scheduler_engine_enabled)
	{
		attr_intern->ptr->kind.mask        = KIND_MASK_MPC_POLLING_THREAD;
		attr_intern->ptr->basic_priority   = _mpc_thread_config_get()->scheduler_polling_basic_prio;
		attr_intern->ptr->kind.priority    = _mpc_thread_config_get()->scheduler_polling_basic_prio;
		attr_intern->ptr->current_priority = _mpc_thread_config_get()->scheduler_polling_basic_prio;
	}

	for(i = 0; i < vp_number; i++)
	{
		attr_intern->ptr->bind_to = i;
		mpc_thread_core_thread_create(&threadp, &attr, _mpc_threads_ng_engine_polling_func_bootstrap, NULL);
	}

	mpc_thread_attr_destroy(&attr);
}

void _mpc_threads_ng_engine_scheduler_init_thread(_mpc_threads_ng_engine_scheduler_t *sched,
                                               struct _mpc_threads_ng_engine_p_s *th)
{
	sched->th     = th;
	sched->status = _mpc_threads_ng_engine_running;
	mpc_common_spinlock_init(&sched->debug_lock, 0);
	_mpc_threads_ng_engine_scheduler_init_thread_p(sched);
}

char *_mpc_threads_ng_engine_scheduler_get_name()
{
	return sched_type;
}
