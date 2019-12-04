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
#include <unistd.h>
#include <string.h>
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_ethread.h"
#include "sctk_ethread_internal.h"
#include "sctk_alloc.h"
#include "sctk_internal_thread.h"
#include "sctk_context.h"
#include "sctk_posix_ethread.h"
#include "mpc_topology.h"

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 2

/*Key data structures shared with ethread*/
int sctk_ethread_key_pos = 0;
stck_ethread_key_destr_function_t
  sctk_ethread_destr_func_tab[SCTK_THREAD_KEYS_MAX];
sctk_ethread_per_thread_t sctk_ethread_main_thread;

sctk_ethread_virtual_processor_t virtual_processor = SCTK_ETHREAD_VP_INIT;

/*Thread management*/
#ifdef SCTK_KERNEL_THREAD_USE_TLS
__thread void *sctk_ethread_key;
#else
kthread_key_t sctk_ethread_key;
#endif

inline sctk_ethread_t
sctk_ethread_self ()
{
  sctk_ethread_virtual_processor_t *vp;
  sctk_ethread_per_thread_t *task;
  vp = kthread_getspecific (sctk_ethread_key);
  task = (sctk_ethread_t) vp->current;
  return task;
}

sctk_ethread_t
sctk_ethread_self_check ()
{
  sctk_ethread_virtual_processor_t *vp;
  sctk_ethread_per_thread_t *task;
  vp = kthread_getspecific (sctk_ethread_key);
  if (vp == NULL)
    return NULL;
  task = (sctk_ethread_t) vp->current;
  return task;
}

static inline void
sctk_ethread_self_all (sctk_ethread_virtual_processor_t ** vp,
		       sctk_ethread_per_thread_t ** task)
{
  *vp = kthread_getspecific (sctk_ethread_key);
  *task = (sctk_ethread_t) (*vp)->current;
  sctk_assert ((*task)->vp == *vp);
}

static inline void
sctk_ethread_place_task_on_vp (sctk_ethread_virtual_processor_t * vp,
			       sctk_ethread_per_thread_t * task)
{
  task->vp = vp;
  sctk_nodebug ("Place %p on %d", task, vp->rank);
  mpc_common_spinlock_lock (&vp->spinlock);
  task->status = ethread_ready;
  __sctk_ethread_enqueue_task (task,
			       (sctk_ethread_per_thread_t **) & (vp->
								 incomming_queue),
			       (sctk_ethread_per_thread_t **) & (vp->
								 incomming_queue_tail));
  mpc_common_spinlock_unlock (&vp->spinlock);
}

void
sctk_ethread_return_task (sctk_ethread_per_thread_t * task)
{
  if (task->status == ethread_blocked)
    {
      task->status = ethread_ready;
      sctk_ethread_place_task_on_vp
	((sctk_ethread_virtual_processor_t *) task->vp, task);
    }
  else
    {
      sctk_nodebug ("status %d %p", task->status, task);
    }
}

static int
sctk_ethread_sched_yield ()
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_self_all (&current_vp, &current);

  return __sctk_ethread_sched_yield_vp (current_vp, current);
}

/* int */
/* sctk_ethread_sched_yield_to_idle () */
/* { */
/*   sctk_ethread_per_thread_t *current; */
/*   sctk_ethread_virtual_processor_t *current_vp; */
/*   sctk_ethread_self_all (&current_vp, &current); */

/*   return __sctk_ethread_sched_yield_to_idle (current_vp, current); */
/* } */

static int
sctk_ethread_sched_dump (char *file)
{
  sctk_ethread_t self;
  self = sctk_ethread_self ();
  self->status = ethread_dump;
  self->file_to_dump = file;
  return
    __sctk_ethread_sched_yield_vp ((sctk_ethread_virtual_processor_t *)
				   self->vp, self);
}
static int
sctk_ethread_sched_migrate ()
{
  sctk_ethread_t self;
  self = sctk_ethread_self ();
  self->status = ethread_dump;
  self->dump_for_migration = 1;
  return
    __sctk_ethread_sched_yield_vp ((sctk_ethread_virtual_processor_t *)
				   self->vp, self);
}
static int
sctk_ethread_sched_restore (sctk_thread_t thread, char *type, int vp)
{
  struct sctk_alloc_chain *tls;

  assume (vp == 0);

  sctk_nodebug ("Try to restore %p %s", thread, type);
  __sctk_restore_tls (&tls, type);

  /*Reinit status */
  ((sctk_ethread_t) thread)->vp = &virtual_processor;
  ((sctk_ethread_t) thread)->status = ethread_ready;

/*   sctk_ethread_print_task((sctk_ethread_t)thread); */

  /*Place in ready queue */
  sctk_ethread_enqueue_task (&virtual_processor, thread);


  return 0;
}
static int
sctk_ethread_sched_dump_clean ()
{
  return 0;
}

static double
sctk_ethread_get_activity (int i)
{
  assume (i == 0);
  return virtual_processor.usage;
}

/*Thread polling*/
static void
sctk_ethread_wait_for_value_and_poll (volatile int *data, int value,
				      void (*func) (void *), void *arg)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;

  sctk_ethread_self_all (&current_vp, &current);

  sctk_nodebug ("wait real : %d", current_vp->rank);
  __sctk_ethread_wait_for_value_and_poll (current_vp,
					  current, data, value, func, arg);
}

static int
sctk_ethread_join (sctk_ethread_t th, void **val)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_self_all (&current_vp, &current);

  return __sctk_ethread_join (current_vp, current, th, val);
}
static void
sctk_ethread_exit (void *retval)
{
  sctk_ethread_per_thread_t *current;
  current = sctk_ethread_self ();
  __sctk_ethread_exit (retval, current);
}


/*Mutex management*/
static int
sctk_ethread_mutex_init (sctk_thread_mutex_t * lock,
			 const sctk_thread_mutexattr_t * attr)
{
  return __sctk_ethread_mutex_init ((sctk_ethread_mutex_t *) lock,
				    (sctk_ethread_mutexattr_t *) attr);
}
static int
sctk_ethread_mutex_lock (sctk_ethread_mutex_t * lock)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_self_all (&current_vp, &current);

  return __sctk_ethread_mutex_lock (current_vp, current, lock);
}
static int
sctk_ethread_mutex_spinlock (sctk_ethread_mutex_t * lock)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_self_all (&current_vp, &current);

  return __sctk_ethread_mutex_lock (current_vp, current, lock);
}
static int
sctk_ethread_mutex_trylock (sctk_ethread_mutex_t * lock)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_self_all (&current_vp, &current);

  return __sctk_ethread_mutex_trylock (current_vp, current, lock);
}

static int
sctk_ethread_mutex_unlock (sctk_ethread_mutex_t * lock)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_self_all (&current_vp, &current);

  return __sctk_ethread_mutex_unlock (current_vp,
				      current,
				      sctk_ethread_return_task, lock);
}

/*Key management*/
static int
sctk_ethread_key_create (int *key,
			 stck_ethread_key_destr_function_t destr_func)
{
  return __sctk_ethread_key_create (key, destr_func);
}

static int
sctk_ethread_key_delete (int key)
{
  return __sctk_ethread_key_delete (sctk_ethread_self (), key);
}

static int
sctk_ethread_setspecific (int key, void *val)
{
  return __sctk_ethread_setspecific (sctk_ethread_self (), key, val);
}

static void *
sctk_ethread_getspecific (int key)
{
  return __sctk_ethread_getspecific (sctk_ethread_self (), key);
}

/*Attribut creation*/
static int
sctk_ethread_attr_init (sctk_ethread_attr_t * attr)
{
  attr->ptr = (sctk_ethread_attr_intern_t *)
    sctk_malloc (sizeof (sctk_ethread_attr_intern_t));
  return __sctk_ethread_attr_init (attr->ptr);
}

static int
sctk_ethread_attr_destroy (sctk_ethread_attr_t * attr)
{
  sctk_free (attr->ptr);
  return 0;
}

static int
sctk_ethread_create (sctk_ethread_t * threadp,
		     sctk_ethread_attr_t * attr,
		     void *(*start_routine) (void *), void *arg)
{
  if (attr != NULL)
    return __sctk_ethread_create (ethread_ready, &virtual_processor,
				  sctk_ethread_self (),
				  threadp, attr->ptr, start_routine, arg);
  else
    return __sctk_ethread_create (ethread_ready, &virtual_processor,
				  sctk_ethread_self (),
				  threadp, NULL, start_routine, arg);
}

static int
sctk_ethread_user_create (sctk_ethread_t * threadp,
			  sctk_ethread_attr_t * attr,
			  void *(*start_routine) (void *), void *arg)
{
  if (attr != NULL)
    return __sctk_ethread_create (ethread_ready, &virtual_processor,
				  sctk_ethread_self (),
				  threadp, attr->ptr, start_routine, arg);
  else
    return __sctk_ethread_create (ethread_ready, &virtual_processor,
				  sctk_ethread_self (),
				  threadp, NULL, start_routine, arg);
}

static void
sctk_ethread_freeze_thread_on_vp (sctk_ethread_mutex_t * lock,
				  void **list_tmp)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_self_all (&current_vp, &current);

  __sctk_ethread_freeze_thread_on_vp (current_vp,
				      current,
				      sctk_ethread_mutex_unlock,
				      lock, (volatile void **) list_tmp);
}

static void
sctk_ethread_wake_thread_on_vp (void **list_tmp)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_self_all (&current_vp, &current);

  __sctk_ethread_wake_thread_on_vp (current_vp, list_tmp);
}

static int
sctk_ethread_get_vp ()
{
  return 0;
}

static int
sctk_ethread_equal (sctk_thread_t a, sctk_thread_t b)
{
  return (a == b);
}

static int
sctk_ethread_kill (sctk_ethread_per_thread_t * thread, int val)
{
  return __sctk_ethread_kill (thread, val);
}

static int
sctk_ethread_sigmask (int how, const sigset_t * newmask, sigset_t * oldmask)
{
  sctk_ethread_per_thread_t *cur;
  cur = sctk_ethread_self ();
  return __sctk_ethread_sigmask (cur, how, newmask, oldmask);
}


static int
sctk_ethread_sigpending (sigset_t * set)
{
  int res;
  sctk_ethread_per_thread_t *cur;
  cur = sctk_ethread_self ();
  res = __sctk_ethread_sigpending (cur, set);
  return res;
}


static int
sctk_ethread_sigsuspend (sigset_t * set)
{
  int res;
  sctk_ethread_per_thread_t *cur;
  cur = sctk_ethread_self ();
  res = __sctk_ethread_sigsuspend (cur, set);
  return res;
}


static int
sctk_ethread_thread_attr_setbinding (__UNUSED__ sctk_thread_attr_t * __attr, __UNUSED__  int __binding)
{
  return 0;
}

static int
sctk_ethread_thread_attr_getbinding (__UNUSED__  sctk_thread_attr_t * __attr, __UNUSED__  int *__binding)
{
  return 0;
}

void
sctk_ethread_thread_init (void)
{
  int i;
  sctk_only_once ();

  /** ** **/
  sctk_enable_lib_thread_db() ;
  /** **/
  
  sctk_init_default_sigset ();
  kthread_key_create (&sctk_ethread_key, NULL);
  kthread_setspecific (sctk_ethread_key, &virtual_processor);

  sctk_ethread_check_size_eq (int, sctk_ethread_status_t);

  sctk_print_version ("Init Ethread", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);

  sctk_ethread_check_size (sctk_ethread_mutex_t, sctk_thread_mutex_t);
  sctk_ethread_check_size (sctk_ethread_mutexattr_t, sctk_thread_mutexattr_t);

  {
    static sctk_ethread_mutex_t loc = SCTK_ETHREAD_MUTEX_INIT;
    static sctk_thread_mutex_t glob = SCTK_THREAD_MUTEX_INITIALIZER;
    assume (memcmp (&loc, &glob, sizeof (sctk_ethread_mutex_t)) == 0);
  }

  for (i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
    {
      sctk_ethread_destr_func_tab[i] = NULL;
    }

  __sctk_ptr_thread_yield = sctk_ethread_sched_yield;
  __sctk_ptr_thread_dump = sctk_ethread_sched_dump;
  __sctk_ptr_thread_migrate = sctk_ethread_sched_migrate;
  __sctk_ptr_thread_restore = sctk_ethread_sched_restore;
  __sctk_ptr_thread_dump_clean = sctk_ethread_sched_dump_clean;

  __sctk_ptr_thread_attr_setbinding = sctk_ethread_thread_attr_setbinding;
  __sctk_ptr_thread_attr_getbinding = sctk_ethread_thread_attr_getbinding;

  sctk_add_func_type (sctk_ethread, create, int (*)(sctk_thread_t *,
						    const
						    sctk_thread_attr_t *,
						    void *(*)(void *),
						    void *));
  sctk_add_func_type (sctk_ethread, user_create,
		      int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
			      void *(*)(void *), void *));


  sctk_ethread_check_size (sctk_ethread_attr_t, sctk_thread_attr_t);
  sctk_add_func_type (sctk_ethread, attr_init, int (*)(sctk_thread_attr_t *));
  sctk_add_func_type (sctk_ethread, attr_destroy,
		      int (*)(sctk_thread_attr_t *));

  sctk_ethread_check_size (sctk_ethread_t, sctk_thread_t);
  sctk_add_func_type (sctk_ethread, self, sctk_thread_t (*)(void));
  sctk_add_func_type (sctk_ethread, self_check, sctk_thread_t (*)(void));
  sctk_add_func_type (sctk_ethread, equal,
		      int (*)(sctk_thread_t, sctk_thread_t));
  sctk_add_func_type (sctk_ethread, join, int (*)(sctk_thread_t, void **));
  sctk_add_func_type (sctk_ethread, exit, void (*)(void *));


  sctk_add_func_type (sctk_ethread, mutex_lock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_ethread, mutex_spinlock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_ethread, mutex_trylock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_ethread, mutex_unlock,
		      int (*)(sctk_thread_mutex_t *));
  __sctk_ptr_thread_mutex_init = sctk_ethread_mutex_init;


  sctk_ethread_check_size (int, sctk_thread_key_t);
  sctk_ethread_check_size (stck_ethread_key_destr_function_t,
			   void (*)(void *));
  sctk_add_func_type (sctk_ethread, key_create,
		      int (*)(sctk_thread_key_t *, void (*)(void *)));
  sctk_add_func_type (sctk_ethread, key_delete, int (*)(sctk_thread_key_t));
  sctk_add_func_type (sctk_ethread, setspecific,
		      int (*)(sctk_thread_key_t, const void *));
  sctk_add_func_type (sctk_ethread, getspecific,
		      void *(*)(sctk_thread_key_t));
  sctk_add_func_type (sctk_ethread, kill, int (*)(sctk_thread_t, int));
  sctk_add_func_type (sctk_ethread, sigpending, int (*)(sigset_t *));
  sctk_add_func_type (sctk_ethread, sigsuspend, int (*)(sigset_t *));


  sctk_add_func (sctk_ethread, wait_for_value_and_poll);
  sctk_add_func_type (sctk_ethread, freeze_thread_on_vp,
		      void (*)(sctk_thread_mutex_t *, void **));
  sctk_add_func (sctk_ethread, wake_thread_on_vp);
  sctk_add_func (sctk_ethread, get_vp);
  sctk_add_func_type (sctk_ethread, get_activity, double (*)(int));

  sctk_add_func_type (sctk_ethread, sigmask,
		      int (*)(int, const sigset_t *, sigset_t *));

  /*Init main thread and virtual processor */
  sctk_ethread_init_data (&sctk_ethread_main_thread);
  virtual_processor.current = &sctk_ethread_main_thread;
  sctk_ethread_main_thread.vp = &virtual_processor;

  for (i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
    {
      sctk_ethread_main_thread.tls[i] = NULL;
      sctk_ethread_destr_func_tab[i] = NULL;
    }

  {
    sctk_ethread_t idle;
    __sctk_ethread_create (ethread_idle,
			   &virtual_processor,
			   &sctk_ethread_main_thread,
			   &idle, NULL, NULL, NULL);
  }

  sctk_posix_ethread ();

  sctk_ethread_debug_status (ethread_ready);

  sctk_multithreading_initialised = 1;
  sctk_free (sctk_malloc (5));

  sctk_thread_data_init ();
  mpc_topology_set_pu_count (1);
#ifdef MPC_PosixAllocator
  sctk_add_global_var (&sctk_ethread_key_pos, sizeof (int));
#endif
}
