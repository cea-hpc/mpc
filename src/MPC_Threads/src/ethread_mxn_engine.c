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
#include "mpc_common_spinlock.h"
#include "mpc_thread.h"
#include "mpc_topology.h"
#include "sctk_alloc.h"
#include "thread_ptr.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ethread_posix.h"

static volatile unsigned int sctk_nb_vp_initialized           = 1;
mpc_common_spinlock_t        _mpc_thread_ethread_key_spinlock = MPC_COMMON_SPINLOCK_INITIALIZER;
static _mpc_thread_ethread_virtual_processor_t **_mpc_thread_ethread_mxn_engine_vp_list = NULL;
static int use_ethread_mxn = 0;

void
_mpc_thread_ethread_mxn_engine_init_kethread()
{
	if(use_ethread_mxn != 0)
	{
		_mpc_thread_kthread_setspecific(_mpc_thread_ethread_key, &virtual_processor);
	}
}

inline _mpc_thread_ethread_t
_mpc_thread_ethread_mxn_engine_self()
{
	_mpc_thread_ethread_virtual_processor_t *vp = NULL;
	_mpc_thread_ethread_per_thread_t *       task = NULL;

	vp = _mpc_thread_kthread_getspecific(_mpc_thread_ethread_key);
	if(vp == NULL)
	{
		return NULL;
	}
	task = (_mpc_thread_ethread_t)vp->current;
	return task;
}

static inline void
_mpc_thread_ethread_mxn_engine_self_all(_mpc_thread_ethread_virtual_processor_t **vp,
                                        _mpc_thread_ethread_per_thread_t **task)
{
	*vp   = _mpc_thread_kthread_getspecific(_mpc_thread_ethread_key);
	*task = (_mpc_thread_ethread_t)(*vp)->current;
	assert( (*task)->vp == *vp);
}

static inline void
_mpc_thread_ethread_mxn_engine_place_task_on_vp(_mpc_thread_ethread_virtual_processor_t *vp,
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

void
_mpc_thread_ethread_mxn_engine_return_task(_mpc_thread_ethread_per_thread_t *task)
{
	if(task->status == ethread_blocked)
	{
		task->status = ethread_ready;
		_mpc_thread_ethread_mxn_engine_place_task_on_vp
		        ( (_mpc_thread_ethread_virtual_processor_t *)task->vp, task);
	}
	else
	{
		mpc_common_nodebug("status %d %p", task->status, task);
	}
}

static void _mpc_thread_ethread_mxn_engine_migration_task(_mpc_thread_ethread_per_thread_t *task)
{
	mpc_common_nodebug("_mpc_thread_ethread_mxn_engine_migration_task %p", task);
	_mpc_thread_ethread_mxn_engine_place_task_on_vp(task->migrate_to, task);
	mpc_common_nodebug("_mpc_thread_ethread_mxn_engine_migration_task %p done", task);
}

static void _mpc_thread_ethread_mxn_engine_migration_task_relocalise(_mpc_thread_ethread_per_thread_t *task)
{
	mpc_common_nodebug("_mpc_thread_ethread_mxn_engine_migration_task rel %p", task);
	sctk_alloc_posix_numa_migrate_chain(task->tls_mem);
	_mpc_thread_ethread_mxn_engine_place_task_on_vp(task->migrate_to, task);
	mpc_common_nodebug("_mpc_thread_ethread_mxn_engine_migration_task rel %p done", task);
}

static int _mpc_thread_ethread_mxn_engine_sched_yield()
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_sched_yield_vp(current_vp, current);
}

int _mpc_thread_ethread_check_state()
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	if(current->status == ethread_zombie)
	{
		return 0;
	}
	if(current->status == ethread_joined)
	{
		return 0;
	}
	if(current->status == ethread_idle)
	{
		return 0;
	}

	return 1;
}

static int _mpc_thread_ethread_mxn_engine_sched_dump(char *file)
{
	_mpc_thread_ethread_t self = NULL;

	self               = _mpc_thread_ethread_mxn_engine_self();
	self->status       = ethread_dump;
	self->file_to_dump = file;
	return
	        ___mpc_thread_ethread_sched_yield_vp( (_mpc_thread_ethread_virtual_processor_t *)
	                                              self->vp, self);
}

static int _mpc_thread_ethread_mxn_engine_sched_migrate()
{
	int res = 0;
	_mpc_thread_ethread_t self = NULL;

	self                     = _mpc_thread_ethread_mxn_engine_self();
	self->status             = ethread_dump;
	self->dump_for_migration = 1;
	mpc_common_nodebug("Start migration _mpc_thread_ethread_mxn_engine_sched_migrate");
	res =
		___mpc_thread_ethread_sched_yield_vp( (_mpc_thread_ethread_virtual_processor_t *)
		                                      self->vp, self);
	mpc_common_nodebug("Start migration _mpc_thread_ethread_mxn_engine_sched_migrate DONE");
	return res;
}

static int _mpc_thread_ethread_mxn_engine_sched_restore(mpc_thread_t thread, char *type, int vp)
{
	struct sctk_alloc_chain *tls = NULL;
	_mpc_thread_ethread_virtual_processor_t *cpu = NULL;

	mpc_common_nodebug("Try to restore %p on vp %d", thread, vp);
	__sctk_restore_tls(&tls, type);
	mpc_common_nodebug("Try to restore %p on vp %d DONE", thread, vp);

	/*Reinit status */
	cpu = _mpc_thread_ethread_mxn_engine_vp_list[vp];
	( (_mpc_thread_ethread_t)thread)->vp = _mpc_thread_ethread_mxn_engine_vp_list[vp];

	/*Place in ready queue */
	_mpc_thread_ethread_mxn_engine_place_task_on_vp(cpu, thread);

	mpc_common_nodebug("Restored");

	/*Free migration request */
/*   sprintf (name, "%s/mig_task", sctk_store_dir); */
/*   if (strncmp (type, name, strlen (name)) == 0) */
/*     { */
/*       remove (type); */
/*       mpc_common_nodebug ("%s removed Restored",name); */
/*     } */
	mpc_common_nodebug("All done Restored");
	return 0;
}

static int _mpc_thread_ethread_mxn_engine_sched_dump_clean()
{
	/*
	 * _mpc_thread_ethread_t self;
	 * char name[MPC_COMMON_MAX_FILENAME_SIZE];
	 * unsigned long step = 0;
	 * FILE *file;
	 * self = _mpc_thread_ethread_mxn_engine_self ();
	 */
/*   sprintf (name, "%s/task_%p_%lu", sctk_store_dir, self, step); */
/*   file = fopen (name, "r"); */
/*   while (file != NULL) */
/*     { */
/*       fclose (file); */
/*       mpc_common_nodebug ("Clean file %s", name); */
/*       remove (name); */
/*       step++; */
/*       sprintf (name, "%s/task_%p_%lu", sctk_store_dir, self, step); */
/*       file = fopen (name, "r"); */
/*     } */

	return 0;
}

/*Thread polling*/
static void _mpc_thread_ethread_mxn_engine_wait_for_value_and_poll(volatile int *data, int value,
                                                                   void (*func)(void *), void *arg)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	mpc_common_nodebug("wait real : %d", current_vp->rank);
	___mpc_thread_ethread_wait_for_value_and_poll(current_vp,
	                                              current, data, value, func, arg);
}

static int _mpc_thread_ethread_mxn_engine_join(_mpc_thread_ethread_t th, void **val)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_join(current_vp, current, th, val);
}

static void _mpc_thread_ethread_mxn_engine_exit(void *retval)
{
	_mpc_thread_ethread_per_thread_t *current = NULL;

	current = _mpc_thread_ethread_mxn_engine_self();
	___mpc_thread_ethread_exit(retval, current);
}

/*Mutex management*/
static int _mpc_thread_ethread_mxn_engine_mutex_init(mpc_thread_mutex_t *lock,
                                                     const mpc_thread_mutexattr_t *attr)
{
	return ___mpc_thread_ethread_mutex_init( (_mpc_thread_ethread_mutex_t *)lock,
	                                         (_mpc_thread_ethread_mutexattr_t *)attr);
}

static int _mpc_thread_ethread_mxn_engine_mutex_lock(_mpc_thread_ethread_mutex_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_mutex_lock(current_vp, current, lock);
}

static int _mpc_thread_ethread_mxn_engine_mutex_trylock(_mpc_thread_ethread_mutex_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_mutex_trylock(current_vp, current, lock);
}

static int _mpc_thread_ethread_mxn_engine_mutex_spinlock(_mpc_thread_ethread_mutex_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;
	int res = 0;
	int i = 0;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	for(i = 0; i < 20; i++)
	{
		res = ___mpc_thread_ethread_mutex_trylock(current_vp, current, lock);
		if(res == 0)
		{
			return res;
		}
	}
	return ___mpc_thread_ethread_mutex_lock(current_vp, current, lock);
}

static int _mpc_thread_ethread_mxn_engine_mutex_unlock(_mpc_thread_ethread_mutex_t *lock)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	return ___mpc_thread_ethread_mutex_unlock(current_vp,
	                                          current,
	                                          _mpc_thread_ethread_mxn_engine_return_task, lock);
}

/*Key management*/
static int _mpc_thread_ethread_mxn_engine_key_create(int *key,
                                                     stck_ethread_key_destr_function_t destr_func)
{
	return ___mpc_thread_ethread_key_create(key, destr_func);
}

static int _mpc_thread_ethread_mxn_engine_key_delete(int key)
{
	return ___mpc_thread_ethread_key_delete(_mpc_thread_ethread_mxn_engine_self(), key);
}

static int _mpc_thread_ethread_mxn_engine_setspecific(int key, void *val)
{
	_mpc_thread_ethread_per_thread_t *current = NULL;

	current = _mpc_thread_ethread_mxn_engine_self();

	return ___mpc_thread_ethread_setspecific(current, key, val);
}

static void *
_mpc_thread_ethread_mxn_engine_getspecific(int key)
{
	_mpc_thread_ethread_per_thread_t *current = NULL;

	current = _mpc_thread_ethread_mxn_engine_self();
	if(current == NULL)
	{
		return NULL;
	}
	return ___mpc_thread_ethread_getspecific(current, key);
}

/*Attribut creation*/
static int _mpc_thread_ethread_mxn_engine_attr_init(_mpc_thread_ethread_attr_t *attr)
{
	attr->ptr = (_mpc_thread_ethread_attr_intern_t *)
	            sctk_malloc(sizeof(_mpc_thread_ethread_attr_intern_t) );
	return ___mpc_thread_ethread_attr_init(attr->ptr);
}

static int _mpc_thread_ethread_mxn_engine_attr_destroy(_mpc_thread_ethread_attr_t *attr)
{
	sctk_free(attr->ptr);
	return 0;
}

static int _mpc_thread_ethread_mxn_engine_user_create(_mpc_thread_ethread_t *threadp,
                                                      _mpc_thread_ethread_attr_t *attr,
                                                      void *(*start_routine)(void *), void *arg)
{
	/* OpenMP Compat: pos should start at 1. */
	static unsigned int pos = 0;
	pos = (pos + 1) % sctk_nb_vp_initialized;

	assume(pos < sctk_nb_vp_initialized);
	assume(_mpc_thread_ethread_mxn_engine_vp_list[pos] != NULL);

	_mpc_thread_ethread_per_thread_t *current = _mpc_thread_ethread_mxn_engine_self();

	/* This is where Threads are scattered in round-robin when doing
	   MxN Scheduling */
	_mpc_thread_ethread_virtual_processor_t *current_vp = _mpc_thread_ethread_mxn_engine_vp_list[pos];

	mpc_common_debug("Non VP thread creation on VP %d / %d", pos, sctk_nb_vp_initialized);

	if(attr != NULL)
	{
		if(attr->ptr->binding != (unsigned int)-1)
		{
			assume(attr->ptr->binding < sctk_nb_vp_initialized);
			current_vp = _mpc_thread_ethread_mxn_engine_vp_list[attr->ptr->binding];
		}
		return ___mpc_thread_ethread_create(ethread_ready, current_vp,
		                                    current, threadp, attr->ptr,
		                                    start_routine, arg);
	}

	return ___mpc_thread_ethread_create(ethread_ready, current_vp,
												current, threadp, NULL, start_routine, arg);

}

// hmt

/**
 * @brief threads placement on numa nodes
 *
 * @param pos current thread position
 *
 * @return cpu to bind current thread to have a numa placement
 */
// int _mpc_thread_ethread_get_init_vp_numa(int pos){
//    int nb_cpu_per_node = mpc_topology_get_pu_count();
//    int nb_numa_node_per_node = mpc_topology_get_numa_node_count();
//    int nb_cpu_per_numa_node = nb_cpu_per_node / nb_numa_node_per_node;
//    int current_cpu = mpc_topology_get_current_cpu();
//
//
//    //TODO algo de bind numa et il faudrais verifier aussi comment les threads
//    //sont placer au sein d un noeud numa
//
//
//
//}
// hmt

// mpc_thread_mutex_t hmtlock = SCTK_THREAD_MUTEX_INITIALIZER;

static int _mpc_thread_ethread_mxn_engine_create(_mpc_thread_ethread_t *threadp,
                                                 _mpc_thread_ethread_attr_t *attr,
                                                 void *(*start_routine)(void *), void *arg)
{
	int new_binding = 0;

	// pos is a useless variable now we have the patch which uses the arg
	// structure which contain
	// the new_binding for the thread
	// static unsigned int pos = 0;
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	current = _mpc_thread_ethread_mxn_engine_self();

	// new_binding= mpc_thread_get_task_placement(pos); //bind de mpc par defaut

	// PATCH : we dont need rerun mpc_thread_get_task_placement because you have the result in
	// the arg structure
	sctk_thread_data_t *tmp = (sctk_thread_data_t *)arg;
	new_binding = (int)tmp->bind_to;

	current_vp = _mpc_thread_ethread_mxn_engine_vp_list[new_binding];

	// DEBUG
	// char hostname[1024];
	// gethostname(hostname,1024);
	// FILE *hostname_fd = fopen(strcat(hostname,".log"),"a");
	// fprintf(hostname_fd,"SCTK_ETHREAD current_vp->bind_to %d mpc_topology_get_current_cpu() %d
	// new_binding %d pos
	// %d\n",current_vp[new_binding].bind_to,mpc_topology_get_current_cpu(),new_binding, pos);
	// fflush(hostname_fd);
	// fclose(hostname_fd);
	// DEBUG

	// pos is a useless variable now we have the patch which uses the arg
	// structure which contain
	// the new_binding for the thread
	// pos++;

	if(attr != NULL)
	{
		return ___mpc_thread_ethread_create(ethread_ready, current_vp,
		                                    current, threadp, attr->ptr,
		                                    start_routine, arg);
	}

	return ___mpc_thread_ethread_create(ethread_ready, current_vp,
												current, threadp, NULL, start_routine, arg);

}

static void _mpc_thread_ethread_mxn_engine_freeze_thread_on_vp(_mpc_thread_ethread_mutex_t *lock,
                                                               void **list_tmp)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	___mpc_thread_ethread_freeze_thread_on_vp(current_vp,
	                                          current,
	                                          _mpc_thread_ethread_mxn_engine_mutex_unlock,
	                                          lock, (volatile void **)list_tmp);
}

static void _mpc_thread_ethread_mxn_engine_wake_thread_on_vp(void **list_tmp)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);

	___mpc_thread_ethread_wake_thread_on_vp(current_vp, list_tmp);
}

static void *
_mpc_thread_ethread_mxn_engine_func_kernel_thread(void *arg)
{
	_mpc_thread_ethread_per_thread_t         th_data;
	_mpc_thread_ethread_virtual_processor_t *vp = NULL;
	sctk_thread_data_t tmp = SCTK_THREAD_DATA_INIT;

	vp = (_mpc_thread_ethread_virtual_processor_t *)arg;
	_mpc_thread_ethread_init_data(&th_data);
	vp->current = &th_data;
	_mpc_thread_kthread_setspecific(_mpc_thread_ethread_key, vp);
	th_data.vp     = vp;
	vp->idle       = &th_data;
	th_data.status = ethread_idle;

	tmp.__start_routine   = _mpc_thread_ethread_mxn_engine_func_kernel_thread;
	tmp.virtual_processor = vp->rank;

	_mpc_thread_data_set(&tmp);
	sctk_set_tls(NULL);

	//avoid to create an allocation chain
	sctk_alloc_posix_plug_on_egg_allocator();

	mpc_common_spinlock_lock(&_mpc_thread_ethread_key_spinlock);
	sctk_nb_vp_initialized++;
	mpc_common_spinlock_unlock(&_mpc_thread_ethread_key_spinlock);

	if(vp != NULL)
	{
		mpc_topology_bind_to_cpu(vp->bind_to);
/*     fprintf(stderr,"bind thread to %d\n",vp->bind_to); */
	}
	mpc_common_nodebug("%d %d", mpc_topology_get_current_cpu(), vp->bind_to);
	___mpc_thread_ethread_idle_task(&th_data);
	return NULL;
}

static void *
_mpc_thread_ethread_gen_func_kernel_thread(void *arg)
{
	_mpc_thread_ethread_per_thread_t         th_data;
	_mpc_thread_ethread_virtual_processor_t *vp = NULL;
	int i = 0;
	sctk_thread_data_t tmp = SCTK_THREAD_DATA_INIT;

	vp = (_mpc_thread_ethread_virtual_processor_t *)arg;
	mpc_topology_bind_to_cpu(vp->bind_to);
	_mpc_thread_ethread_init_data(&th_data);
	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		th_data.tls[i] = NULL;
	}
	vp->current = &th_data;
	_mpc_thread_kthread_setspecific(_mpc_thread_ethread_key, vp);
	th_data.vp     = vp;
	vp->idle       = &th_data;
	th_data.status = ethread_idle;

	tmp.__start_routine   = _mpc_thread_ethread_gen_func_kernel_thread;
	tmp.virtual_processor = vp->rank;

	_mpc_thread_data_set(&tmp);
	sctk_set_tls(NULL);

	mpc_common_nodebug("Entering in kernel idle");


	___mpc_thread_ethread_kernel_idle_task(&th_data);
	return NULL;
}

static void _mpc_thread_ethread_mxn_engine_start_kernel_thread(int pos)
{
	mpc_thread_kthread_t pid = NULL;
	_mpc_thread_ethread_virtual_processor_t  tmp_init = SCTK_ETHREAD_VP_INIT;
	_mpc_thread_ethread_virtual_processor_t *tmp = NULL;

	mpc_topology_bind_to_cpu(pos);
	if(pos != 0)
	{
		tmp = (_mpc_thread_ethread_virtual_processor_t *)
		      __sctk_malloc_new(sizeof(_mpc_thread_ethread_virtual_processor_t),
		                        mpc_thread_tls);
		*tmp      = tmp_init;
		tmp->rank = pos;
		_mpc_thread_ethread_mxn_engine_vp_list[pos] = tmp;
	}
	_mpc_thread_ethread_mxn_engine_vp_list[pos]->bind_to = pos;
	mpc_common_nodebug("BIND to %d", pos);

	_mpc_thread_kthread_create(&pid,
	                           _mpc_thread_ethread_mxn_engine_func_kernel_thread,
	                           _mpc_thread_ethread_mxn_engine_vp_list[pos]);
/*   mpc_topology_bind_to_cpu(0); */
}

_mpc_thread_ethread_virtual_processor_t *
_mpc_thread_ethread_start_kernel_thread()
{
	mpc_thread_kthread_t pid = NULL;
	_mpc_thread_ethread_virtual_processor_t  tmp_init = SCTK_ETHREAD_VP_INIT;
	_mpc_thread_ethread_virtual_processor_t *tmp = NULL;
	static mpc_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

	mpc_common_nodebug("Create Kthread");

	mpc_thread_mutex_lock(&lock);

	tmp = (_mpc_thread_ethread_virtual_processor_t *)
	      __sctk_malloc_new(sizeof(_mpc_thread_ethread_virtual_processor_t),
	                        mpc_thread_tls);
	*tmp      = tmp_init;
	tmp->rank = -1;

	_mpc_thread_kthread_create(&pid, _mpc_thread_ethread_gen_func_kernel_thread, tmp);
	mpc_common_nodebug("Create Kthread done");

	mpc_thread_mutex_unlock(&lock);
	return tmp;
}

static void _mpc_thread_ethread_mxn_engine_init_vp(_mpc_thread_ethread_per_thread_t *th_data,
                                                   _mpc_thread_ethread_virtual_processor_t *vp)
{
	int i = 0;

	/*Init thread and virtual processor */
	_mpc_thread_ethread_init_data(th_data);
	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		th_data->tls[i] = NULL;
	}

	vp->current = th_data;
	_mpc_thread_kthread_setspecific(_mpc_thread_ethread_key, vp);

	th_data->vp = vp;
	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		_mpc_thread_ethread_main_thread.tls[i] = NULL;
		_mpc_thread_ethread_destr_func_tab[i]  = NULL;
	}

	{
		_mpc_thread_ethread_t idle = NULL;
		___mpc_thread_ethread_create(ethread_idle,
		                             vp, th_data, &idle, NULL, NULL, NULL);
	}
}

static void _mpc_thread_ethread_mxn_engine_init_virtual_processors()
{
	unsigned int cpu_number = 0;
	unsigned int i = 0;

	use_ethread_mxn = 1;
	_mpc_thread_kthread_key_create(&_mpc_thread_ethread_key, NULL);

	/*Init main thread and virtual processor */
	_mpc_thread_ethread_mxn_engine_init_vp(&_mpc_thread_ethread_main_thread, &virtual_processor);

	cpu_number = mpc_topology_get_pu_count();

	mpc_common_nodebug("starts %d virtual processors", cpu_number);

	_mpc_thread_ethread_mxn_engine_vp_list = (_mpc_thread_ethread_virtual_processor_t **)
	                                         __sctk_malloc(cpu_number *
	                                                       sizeof(_mpc_thread_ethread_virtual_processor_t *),
	                                                       mpc_thread_tls);
	_mpc_thread_ethread_mxn_engine_vp_list[0] = &virtual_processor;

	for(i = 1; i < cpu_number; i++)
	{
		_mpc_thread_ethread_mxn_engine_start_kernel_thread((int)i);
	}
	mpc_topology_bind_to_cpu(0);

	mpc_common_nodebug("I'am %p", _mpc_thread_ethread_mxn_engine_self() );
	while(cpu_number != sctk_nb_vp_initialized)
	{
		assume(sctk_nb_vp_initialized <= cpu_number);
		sched_yield();
	}

	for(i = 0; i < cpu_number; i++)
	{
		assume(_mpc_thread_ethread_mxn_engine_vp_list[i] != NULL);
	}
}

static int _mpc_thread_ethread_mxn_engine_thread_attr_setbinding(mpc_thread_attr_t *__attr, int __binding)
{
	_mpc_thread_ethread_attr_t *attr = NULL;

	attr = (_mpc_thread_ethread_attr_t *)__attr;
	if(attr == NULL)
	{
		return EINVAL;
	}
	if(attr->ptr == NULL)
	{
		return EINVAL;
	}


	attr->ptr->binding = __binding;
	return 0;
}

static int _mpc_thread_ethread_mxn_engine_thread_attr_getbinding(mpc_thread_attr_t *__attr, int *__binding)
{
	_mpc_thread_ethread_attr_t *attr = NULL;

	attr = (_mpc_thread_ethread_attr_t *)__attr;
	if(attr == NULL)
	{
		return EINVAL;
	}
	if(attr->ptr == NULL)
	{
		return EINVAL;
	}


	*__binding = (int)attr->ptr->binding;

	return 0;
}

static int _mpc_thread_ethread_mxn_engine_thread_proc_migration(const int cpu)
{
	int last = 0;
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;

	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);
	last = current_vp->rank;

	assert(current->vp == current_vp);
	assert(last == mpc_topology_get_current_cpu() );
	assume(cpu >= 0);
	assume(cpu < mpc_topology_get_pu_count() );
	assume(last >= 0);
	assume(last < mpc_topology_get_pu_count() );

	mpc_common_nodebug("task %p Jump from %d to %d", current, last, cpu);

	if(cpu != last)
	{
		current->status         = ethread_migrate;
		current->migration_func = _mpc_thread_ethread_mxn_engine_migration_task;
		current->migrate_to     = _mpc_thread_ethread_mxn_engine_vp_list[cpu];
		___mpc_thread_ethread_sched_yield_vp(current_vp, current);

		_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);
		current->status         = ethread_migrate;
		current->migration_func = _mpc_thread_ethread_mxn_engine_migration_task_relocalise;
		current->migrate_to     = _mpc_thread_ethread_mxn_engine_vp_list[cpu];
		___mpc_thread_ethread_sched_yield_vp(current_vp, current);

		current->migration_func = NULL;
		current->migrate_to     = NULL;
	}

	mpc_common_nodebug("task %p Jump from %d to %d(%d) done", current, last, cpu,
	                   (_mpc_thread_ethread_virtual_processor_t
	                    *)(_mpc_thread_ethread_mxn_engine_self()->vp)->rank);

	assert(current->vp == _mpc_thread_ethread_mxn_engine_vp_list[cpu]);
	return last;
}

static double
_mpc_thread_ethread_mxn_engine_get_activity(int i)
{
	return _mpc_thread_ethread_mxn_engine_vp_list[i]->usage;
}

static int _mpc_thread_ethread_mxn_engine_equal(mpc_thread_t a, mpc_thread_t b)
{
	return a == b;
}

static int _mpc_thread_ethread_mxn_engine_kill(_mpc_thread_ethread_per_thread_t *thread, int val)
{
	return ___mpc_thread_ethread_kill(thread, val);
}

static int _mpc_thread_ethread_mxn_engine_sigmask(int how, const sigset_t *newmask,
                                                  sigset_t *oldmask)
{
	_mpc_thread_ethread_per_thread_t *cur = NULL;

	cur = _mpc_thread_ethread_mxn_engine_self();
	return ___mpc_thread_ethread_sigmask(cur, how, newmask, oldmask);
}

static int _mpc_thread_ethread_mxn_engine_sigpending(sigset_t *set)
{
	int res = 0;
	_mpc_thread_ethread_per_thread_t *cur = NULL;

	cur = _mpc_thread_ethread_mxn_engine_self();
	res = ___mpc_thread_ethread_sigpending(cur, set);
	return res;
}

static int _mpc_thread_ethread_mxn_engine_sigsuspend(sigset_t *set)
{
	int res = 0;
	_mpc_thread_ethread_per_thread_t *cur = NULL;

	cur = _mpc_thread_ethread_mxn_engine_self();
	res = ___mpc_thread_ethread_sigsuspend(cur, set);
	return res;
}

static void _mpc_thread_ethread_mxn_engine_at_fork_child()
{
	/* mpc_common_debug_error("Unable to fork with user threads child"); */
	/* mpc_common_debug_abort(); */
}

static void _mpc_thread_ethread_mxn_engine_at_fork_parent()
{
	/* mpc_common_debug_error("Unable to fork with user threads parent"); */
	/* mpc_common_debug_abort(); */
}

static void _mpc_thread_ethread_mxn_engine_at_fork_prepare()
{
	/* mpc_common_debug_error("Unable to fork with user threads prepare"); */
	/* mpc_common_debug_abort(); */
}

/* These are wrappers around the Ethread Posix interface
 *  providing the per thread engine task function */

static int _mpc_thread_ethread_mxn_engine_sem_post(_mpc_thread_ethread_sem_t *sem)
{
	return _mpc_thread_ethread_posix_sem_post(sem, _mpc_thread_ethread_mxn_engine_return_task);
}

static int _mpc_thread_ethread_mxn_engine_cond_wait(_mpc_thread_ethread_cond_t *cond,
                                                    _mpc_thread_ethread_mutex_t *mutex)
{
	return _mpc_thread_ethread_posix_cond_wait(cond, mutex, _mpc_thread_ethread_mxn_engine_return_task);
}

static int _mpc_thread_ethread_mxn_engine_cond_timedwait(_mpc_thread_ethread_cond_t *cond,
                                                         _mpc_thread_ethread_mutex_t *mutex,
                                                         const struct timespec *abstime)
{
	return _mpc_thread_ethread_posix_cond_timedwait(cond, mutex, abstime, _mpc_thread_ethread_mxn_engine_return_task);
}

static int _mpc_thread_ethread_mxn_engine_cond_broadcast(_mpc_thread_ethread_cond_t *cond)
{
	return _mpc_thread_ethread_posix_cond_broadcast(cond, _mpc_thread_ethread_mxn_engine_return_task);
}

static int _mpc_thread_ethread_mxn_engine_cond_signal(_mpc_thread_ethread_cond_t *cond)
{
	return _mpc_thread_ethread_posix_cond_signal(cond, _mpc_thread_ethread_mxn_engine_return_task);
}

static int _mpc_thread_ethread_mxn_engine_rwlock_unlock(_mpc_thread_ethread_rwlock_t *rwlock)
{
	return _mpc_thread_ethread_posix_rwlock_unlock(rwlock, _mpc_thread_ethread_mxn_engine_return_task);
}

static int _mpc_thread_ethread_mxn_engine_barrier_wait(_mpc_thread_ethread_barrier_t *barrier)
{
	return _mpc_thread_ethread_posix_barrier_wait(barrier, _mpc_thread_ethread_mxn_engine_return_task);
}

/****************
* NON PORTABLE *
****************/


/* OpenMP Compat */
int _mpc_thread_ethread_mxn_engine_setaffinity_np(_mpc_thread_ethread_t thread, size_t cpusetsize,
                              const mpc_cpu_set_t *cpuset)
{
	_mpc_thread_ethread_per_thread_t *       current = NULL;
	_mpc_thread_ethread_virtual_processor_t *current_vp = NULL;
	cpu_set_t *_cpuset = NULL;

	if (thread != 0)
	{
		//mpc_common_debug_warning("setaffinity_np not supported");
		return 0; /* OpenMP Compat */
	}

	_cpuset = (cpu_set_t *) cpuset;

	// avoid changing vp if we already are on a valide one.
	_mpc_thread_ethread_mxn_engine_self_all(&current_vp, &current);
	if (CPU_ISSET_S(current_vp->rank, cpusetsize, _cpuset))
		return 0;

	unsigned int i = 0;
	for (i = 0 ; i < cpusetsize ; i++)
	{
		if(CPU_ISSET_S(i, cpusetsize, _cpuset))
		{
			(_funcptr_mpc_thread_proc_migration)((int)i);
			break;
		}
	}

	return 0;
}

/* OpenMP Compat */
int _mpc_thread_ethread_mxn_engine_getaffinity_np(_mpc_thread_ethread_t thread __UNUSED__, size_t cpusetsize,
                              mpc_cpu_set_t *cpuset)
{
	cpu_set_t *_cpuset = NULL;

	_cpuset = (cpu_set_t *) cpuset;

	CPU_ZERO_S(cpusetsize, _cpuset);

	for(size_t i = 0; i < sctk_nb_vp_initialized; ++i)
		CPU_SET_S(_mpc_thread_ethread_mxn_engine_vp_list[i]->rank, cpusetsize, _cpuset);

	return 0;
}

void mpc_thread_ethread_mxn_engine_init(void)
{
	mpc_common_debug_only_once();

	pthread_atfork(_mpc_thread_ethread_mxn_engine_at_fork_prepare, _mpc_thread_ethread_mxn_engine_at_fork_parent, _mpc_thread_ethread_mxn_engine_at_fork_child);

	sctk_init_default_sigset();

	_mpc_thread_ethread_check_size_eq(int, _mpc_thread_ethread_status_t);

	_mpc_thread_ethread_check_size(_mpc_thread_ethread_mutex_t, mpc_thread_mutex_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_mutexattr_t, mpc_thread_mutexattr_t);

	{
		static _mpc_thread_ethread_mutex_t loc  = SCTK_ETHREAD_MUTEX_INIT;
		static mpc_thread_mutex_t          glob = SCTK_THREAD_MUTEX_INITIALIZER;
		assume(memcmp(&loc, &glob, sizeof(_mpc_thread_ethread_mutex_t) ) == 0);
	}

	_mpc_thread_ethread_check_size(_mpc_thread_ethread_t, mpc_thread_t);
	_mpc_thread_ethread_check_size(mpc_thread_once_t, mpc_thread_once_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_attr_t, mpc_thread_attr_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_t, mpc_thread_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_sem_t, mpc_thread_sem_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_condattr_t, mpc_thread_condattr_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_spinlock_t, mpc_thread_spinlock_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_rwlock_t, mpc_thread_rwlock_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_rwlockattr_t,
	                               mpc_thread_rwlockattr_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_barrier_t, mpc_thread_barrier_t);
	_mpc_thread_ethread_check_size(_mpc_thread_ethread_barrierattr_t,
	                               mpc_thread_barrierattr_t);

	int i = 0;

	for(i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
	{
		_mpc_thread_ethread_destr_func_tab[i] = NULL;
	}

	_funcptr_mpc_thread_yield      = _mpc_thread_ethread_mxn_engine_sched_yield;
	_funcptr_mpc_thread_dump       = _mpc_thread_ethread_mxn_engine_sched_dump;
	_funcptr_mpc_thread_migrate    = _mpc_thread_ethread_mxn_engine_sched_migrate;
	_funcptr_mpc_thread_restore    = _mpc_thread_ethread_mxn_engine_sched_restore;
	_funcptr_mpc_thread_dump_clean = _mpc_thread_ethread_mxn_engine_sched_dump_clean;

	_funcptr_mpc_thread_attr_setbinding = _mpc_thread_ethread_mxn_engine_thread_attr_setbinding;
	_funcptr_mpc_thread_attr_getbinding = _mpc_thread_ethread_mxn_engine_thread_attr_getbinding;

	_funcptr_mpc_thread_proc_migration = _mpc_thread_ethread_mxn_engine_thread_proc_migration;

	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, create, int (*)(mpc_thread_t *,
	                                                                   const
	                                                                   mpc_thread_attr_t
	                                                                   *,
	                                                                   void *(*)(void *),
	                                                                   void *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, user_create,
	                   int (*)(mpc_thread_t *, const mpc_thread_attr_t *,
	                           void *(*)(void *), void *) );

	_mpc_thread_ethread_check_size(_mpc_thread_ethread_attr_t, mpc_thread_attr_t);
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, attr_init,
	                   int (*)(mpc_thread_attr_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, attr_destroy,
	                   int (*)(mpc_thread_attr_t *) );

	_mpc_thread_ethread_check_size(_mpc_thread_ethread_t, mpc_thread_t);
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, self, mpc_thread_t (*)(void) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, equal,
	                   int (*)(mpc_thread_t, mpc_thread_t) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, join,
	                   int (*)(mpc_thread_t, void **) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, exit, void (*)(void *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, kill, int (*)(mpc_thread_t, int) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, sigpending, int (*)(sigset_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, sigsuspend, int (*)(sigset_t *) );


	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, mutex_lock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, mutex_spinlock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, mutex_trylock,
	                   int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, mutex_unlock,
	                   int (*)(mpc_thread_mutex_t *) );
	_funcptr_mpc_thread_mutex_init = _mpc_thread_ethread_mxn_engine_mutex_init;


	_mpc_thread_ethread_check_size(int, mpc_thread_keys_t);
	_mpc_thread_ethread_check_size(stck_ethread_key_destr_function_t,
	                               void (*)(void *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, key_create,
	                   int (*)(mpc_thread_keys_t *, void (*)(void *) ) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, key_delete,
	                   int (*)(mpc_thread_keys_t) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, setspecific,
	                   int (*)(mpc_thread_keys_t, const void *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, getspecific,
	                   void *(*)(mpc_thread_keys_t) );

	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, get_activity, double (*)(int) );

	sctk_add_func(_mpc_thread_ethread_mxn_engine, wait_for_value_and_poll);
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, freeze_thread_on_vp,
	                   void (*)(mpc_thread_mutex_t *, void **) );
	sctk_add_func(_mpc_thread_ethread_mxn_engine, wake_thread_on_vp);

	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, sigmask,
	                   int (*)(int, const sigset_t *, sigset_t *) );



	/** ** **/
	sctk_enable_lib_thread_db();
	/** **/
	_mpc_thread_ethread_mxn_engine_init_virtual_processors();


	/* Wrapper for task return function */
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, sem_post, int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, cond_wait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, cond_timedwait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *,
	                           const struct timespec *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, cond_broadcast,
	                   int (*)(mpc_thread_cond_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, cond_signal,
	                   int (*)(mpc_thread_cond_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, rwlock_unlock,
	                   int (*)(mpc_thread_rwlock_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, barrier_wait,
	                   int (*)(mpc_thread_barrier_t *) );

	/* From common Ethread interface */

	/*le pthread_once */
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
	/*les s�maphores */
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_init,
	                   int (*)(mpc_thread_sem_t *lock,
	                           int pshared, unsigned int value) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_wait,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_trywait,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_getvalue,
	                   int (*)(mpc_thread_sem_t *, int *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_destroy,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_open,
	                   mpc_thread_sem_t * (*)(const char *, int, ...) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_close,
	                   int (*)(mpc_thread_sem_t *) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sem_unlink, int (*)(const char *) );

	/*sched prio */
	sctk_add_func_type(_mpc_thread_ethread_posix, sched_get_priority_min, int (*)(int) );
	sctk_add_func_type(_mpc_thread_ethread_posix, sched_get_priority_max, int (*)(int) );

	/* sctk_add_func_type(_mpc_thread_ethread_posix, attr_setschedparam,
	 *                 int (*)(mpc_thread_attr_t *restrict,
	 *                         const struct sched_param *restrict) ); */


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

	/*conditions */
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
	/*barrier*/
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
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, setaffinity_np,
	                   int (*)(mpc_thread_t, size_t, const mpc_cpu_set_t *) );
	sctk_add_func_type(_mpc_thread_ethread_mxn_engine, getaffinity_np,
	                   int (*)(mpc_thread_t, size_t, mpc_cpu_set_t *) );

	/* Current */


	sctk_multithreading_initialised = 1;
	mpc_common_nodebug("FORCE allocation");
	sctk_free(sctk_malloc(5) );

	_mpc_thread_data_init();


	TODO("UNDERSTAND WHY THIS YIELD TERMINATED US");
	//_mpc_thread_ethread_mxn_engine_sched_yield ();
}
