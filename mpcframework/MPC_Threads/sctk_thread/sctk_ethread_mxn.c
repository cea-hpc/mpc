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
#include <pthread.h>
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_ethread.h"
#include "sctk_ethread_internal.h"
#include "sctk_alloc.h"
#include "sctk_internal_thread.h"
#include "sctk_context.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_topology.h"
#include "sctk_posix_ethread_mxn.h"

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 2

static volatile unsigned int sctk_nb_vp_initialized = 1;
mpc_common_spinlock_t sctk_ethread_key_spinlock = SCTK_SPINLOCK_INITIALIZER;
static sctk_ethread_virtual_processor_t **sctk_ethread_mxn_vp_list = NULL;
static int use_ethread_mxn = 0;

void
sctk_ethread_mxn_init_kethread ()
{
  if (use_ethread_mxn != 0)
    kthread_setspecific (sctk_ethread_key, &virtual_processor);
}

inline sctk_ethread_t
sctk_ethread_mxn_self ()
{
  sctk_ethread_virtual_processor_t *vp;
  sctk_ethread_per_thread_t *task;
  vp = kthread_getspecific (sctk_ethread_key);
  if (vp == NULL)
    return NULL;
  task = (sctk_ethread_t) vp->current;
  return task;
}

sctk_ethread_t
sctk_ethread_mxn_self_check ()
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
sctk_ethread_mxn_self_all (sctk_ethread_virtual_processor_t ** vp,
			   sctk_ethread_per_thread_t ** task)
{
  *vp = kthread_getspecific (sctk_ethread_key);
  *task = (sctk_ethread_t) (*vp)->current;
  sctk_assert ((*task)->vp == *vp);
}

static inline void
sctk_ethread_mxn_place_task_on_vp (sctk_ethread_virtual_processor_t * vp,
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
sctk_ethread_mxn_return_task (sctk_ethread_per_thread_t * task)
{
  if (task->status == ethread_blocked)
    {
      task->status = ethread_ready;
      sctk_ethread_mxn_place_task_on_vp
	((sctk_ethread_virtual_processor_t *) task->vp, task);
    }
  else
    {
      sctk_nodebug ("status %d %p", task->status, task);
    }
}

static void
sctk_ethread_mxn_migration_task (sctk_ethread_per_thread_t * task)
{
  sctk_nodebug ("sctk_ethread_mxn_migration_task %p", task);
  sctk_ethread_mxn_place_task_on_vp (task->migrate_to, task);
  sctk_nodebug ("sctk_ethread_mxn_migration_task %p done", task);
}

static void
sctk_ethread_mxn_migration_task_relocalise (sctk_ethread_per_thread_t * task)
{
  sctk_nodebug ("sctk_ethread_mxn_migration_task rel %p", task);
  sctk_alloc_posix_numa_migrate_chain (task->tls_mem);
  sctk_ethread_mxn_place_task_on_vp (task->migrate_to, task);
  sctk_nodebug ("sctk_ethread_mxn_migration_task rel %p done", task);
}

static int
sctk_ethread_mxn_sched_yield ()
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  return __sctk_ethread_sched_yield_vp (current_vp, current);
}

int
sctk_ethread_check_state ()
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  if (current->status == ethread_zombie)
    return 0;
  if (current->status == ethread_joined)
    return 0;
  if (current->status == ethread_idle)
    return 0;

  return 1;
}

static int
sctk_ethread_mxn_sched_dump (char *file)
{
  sctk_ethread_t self;
  self = sctk_ethread_mxn_self ();
  self->status = ethread_dump;
  self->file_to_dump = file;
  return
    __sctk_ethread_sched_yield_vp ((sctk_ethread_virtual_processor_t *)
				   self->vp, self);
}
static int
sctk_ethread_mxn_sched_migrate ()
{
  int res;
  sctk_ethread_t self;
  self = sctk_ethread_mxn_self ();
  self->status = ethread_dump;
  self->dump_for_migration = 1;
  sctk_nodebug("Start migration sctk_ethread_mxn_sched_migrate");
  res =
    __sctk_ethread_sched_yield_vp ((sctk_ethread_virtual_processor_t *)
				   self->vp, self);
  sctk_nodebug("Start migration sctk_ethread_mxn_sched_migrate DONE");
  return res;
}
static int
sctk_ethread_mxn_sched_restore (sctk_thread_t thread, char *type, int vp)
{
  struct sctk_alloc_chain *tls;
  sctk_ethread_virtual_processor_t *cpu;
  sctk_nodebug ("Try to restore %p on vp %d", thread, vp);
  __sctk_restore_tls (&tls, type);
  sctk_nodebug ("Try to restore %p on vp %d DONE", thread, vp);

  /*Reinit status */
  cpu = sctk_ethread_mxn_vp_list[vp];
  ((sctk_ethread_t) thread)->vp = sctk_ethread_mxn_vp_list[vp];

  /*Place in ready queue */
  sctk_ethread_mxn_place_task_on_vp (cpu, thread);

  sctk_nodebug ("Restored");

  /*Free migration request */
/*   sprintf (name, "%s/mig_task", sctk_store_dir); */
/*   if (strncmp (type, name, strlen (name)) == 0) */
/*     { */
/*       remove (type); */
/*       sctk_nodebug ("%s removed Restored",name); */
/*     } */
  sctk_nodebug ("All done Restored");
  return 0;
}
static int
sctk_ethread_mxn_sched_dump_clean ()
{
  /*
  sctk_ethread_t self;
  char name[SCTK_MAX_FILENAME_SIZE];
  unsigned long step = 0;
  FILE *file;
  self = sctk_ethread_mxn_self ();
  */
/*   sprintf (name, "%s/task_%p_%lu", sctk_store_dir, self, step); */
/*   file = fopen (name, "r"); */
/*   while (file != NULL) */
/*     { */
/*       fclose (file); */
/*       sctk_nodebug ("Clean file %s", name); */
/*       remove (name); */
/*       step++; */
/*       sprintf (name, "%s/task_%p_%lu", sctk_store_dir, self, step); */
/*       file = fopen (name, "r"); */
/*     } */

  return 0;
}

/*Thread polling*/
static void
sctk_ethread_mxn_wait_for_value_and_poll (volatile int *data, int value,
					  void (*func) (void *), void *arg)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;

  sctk_ethread_mxn_self_all (&current_vp, &current);

  sctk_nodebug ("wait real : %d", current_vp->rank);
  __sctk_ethread_wait_for_value_and_poll (current_vp,
					  current, data, value, func, arg);
}

static int
sctk_ethread_mxn_join (sctk_ethread_t th, void **val)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  return __sctk_ethread_join (current_vp, current, th, val);
}
static void
sctk_ethread_mxn_exit (void *retval)
{
  sctk_ethread_per_thread_t *current;
  current = sctk_ethread_mxn_self ();
  __sctk_ethread_exit (retval, current);
}


/*Mutex management*/
static int
sctk_ethread_mxn_mutex_init (sctk_thread_mutex_t * lock,
			     const sctk_thread_mutexattr_t * attr)
{
  return __sctk_ethread_mutex_init ((sctk_ethread_mutex_t *) lock,
				    (sctk_ethread_mutexattr_t *) attr);
}
static int
sctk_ethread_mxn_mutex_lock (sctk_ethread_mutex_t * lock)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  return __sctk_ethread_mutex_lock (current_vp, current, lock);
}
static int
sctk_ethread_mxn_mutex_trylock (sctk_ethread_mutex_t * lock)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  return __sctk_ethread_mutex_trylock (current_vp, current, lock);
}
static int
sctk_ethread_mxn_mutex_spinlock (sctk_ethread_mutex_t * lock)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  int res;
  int i;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  for (i = 0; i < 20; i++)
    {
      res = __sctk_ethread_mutex_trylock (current_vp, current, lock);
      if (res == 0)
	{
	  return res;
	}
    }
  return __sctk_ethread_mutex_lock (current_vp, current, lock);
}

static int
sctk_ethread_mxn_mutex_unlock (sctk_ethread_mutex_t * lock)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  return __sctk_ethread_mutex_unlock (current_vp,
				      current,
				      sctk_ethread_mxn_return_task, lock);
}

/*Key management*/
static int
sctk_ethread_mxn_key_create (int *key,
			     stck_ethread_key_destr_function_t destr_func)
{
  return __sctk_ethread_key_create (key, destr_func);
}

static int
sctk_ethread_mxn_key_delete (int key)
{
  return __sctk_ethread_key_delete (sctk_ethread_mxn_self (), key);
}

static int
sctk_ethread_mxn_setspecific (int key, void *val)
{
  sctk_ethread_per_thread_t *current;
  current = sctk_ethread_mxn_self ();

  return __sctk_ethread_setspecific (current, key, val);
}

static void *
sctk_ethread_mxn_getspecific (int key)
{
  sctk_ethread_per_thread_t *current;
  current = sctk_ethread_mxn_self ();
  if(current == NULL)
    return NULL;
  return __sctk_ethread_getspecific (current, key);
}

/*Attribut creation*/
static int
sctk_ethread_mxn_attr_init (sctk_ethread_attr_t * attr)
{
  attr->ptr = (sctk_ethread_attr_intern_t *)
    sctk_malloc (sizeof (sctk_ethread_attr_intern_t));
  return __sctk_ethread_attr_init (attr->ptr);
}

static int
sctk_ethread_mxn_attr_destroy (sctk_ethread_attr_t * attr)
{
  sctk_free (attr->ptr);
  return 0;
}

static int
sctk_ethread_mxn_user_create (sctk_ethread_t * threadp,
			      sctk_ethread_attr_t * attr,
			      void *(*start_routine) (void *), void *arg)
{
  static unsigned int pos = 0;
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  current = sctk_ethread_mxn_self ();

  current_vp = sctk_ethread_mxn_vp_list[pos];

  assume (pos < sctk_nb_vp_initialized);
  assume (sctk_ethread_mxn_vp_list[pos] != NULL);

  sctk_nodebug ("Put %d", pos);
  pos = (pos + 1) % sctk_nb_vp_initialized;

  if (attr != NULL){
    if(attr->ptr->binding != (unsigned int)-1){
      sctk_nodebug("Thread pinned to core %d", attr->ptr->binding);
      assume(attr->ptr->binding < sctk_nb_vp_initialized);
      current_vp = sctk_ethread_mxn_vp_list[attr->ptr->binding];
    }
    return __sctk_ethread_create (ethread_ready, current_vp,
				  current, threadp, attr->ptr,
				  start_routine, arg);
  }
  else{
    return __sctk_ethread_create (ethread_ready, current_vp,
				  current, threadp, NULL, start_routine, arg);
  }
}

// hmt
/**
 * @brief threads placement on numa nodes
 *
 * @param pos current thread position
 *
 * @return cpu to bind current thread to have a numa placement
 */
// int sctk_ethread_get_init_vp_numa(int pos){
//    int nb_cpu_per_node = mpc_common_topo_get_pu_count();
//    int nb_numa_node_per_node = mpc_common_topo_get_numa_node_count();
//    int nb_cpu_per_numa_node = nb_cpu_per_node / nb_numa_node_per_node;
//    int current_cpu = mpc_common_topo_get_current_cpu();
//
//
//    //TODO algo de bind numa et il faudrais verifier aussi comment les threads
//    //sont placer au sein d un noeud numa
//
//
//
//}
// hmt

// sctk_thread_mutex_t hmtlock = SCTK_THREAD_MUTEX_INITIALIZER;

static int
sctk_ethread_mxn_create (sctk_ethread_t * threadp,
			 sctk_ethread_attr_t * attr,
			 void *(*start_routine) (void *), void *arg)
{
  int new_binding;

  // pos is a useless variable now we have the patch which uses the arg
  // structure which contain
  // the new_binding for the thread
  // static unsigned int pos = 0;
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  current = sctk_ethread_mxn_self ();

  // new_binding= sctk_get_init_vp(pos); //bind de mpc par defaut

  // PATCH : we dont need rerun sctk_get_init_vp because you have the result in
  // the arg structure
  sctk_thread_data_t *tmp = (sctk_thread_data_t *)arg;
  new_binding = tmp->bind_to;

  current_vp = sctk_ethread_mxn_vp_list[new_binding];

  // DEBUG
  // char hostname[1024];
  // gethostname(hostname,1024);
  // FILE *hostname_fd = fopen(strcat(hostname,".log"),"a");
  // fprintf(hostname_fd,"SCTK_ETHREAD current_vp->bind_to %d mpc_common_topo_get_current_cpu() %d
  // new_binding %d pos
  // %d\n",current_vp[new_binding].bind_to,mpc_common_topo_get_current_cpu(),new_binding, pos);
  // fflush(hostname_fd);
  // fclose(hostname_fd);
  // DEBUG

  // pos is a useless variable now we have the patch which uses the arg
  // structure which contain
  // the new_binding for the thread
  // pos++;

  if (attr != NULL)
    return __sctk_ethread_create (ethread_ready, current_vp,
				  current, threadp, attr->ptr,
				  start_routine, arg);
  else
    return __sctk_ethread_create (ethread_ready, current_vp,
				  current, threadp, NULL, start_routine, arg);
}

static void
sctk_ethread_mxn_freeze_thread_on_vp (sctk_ethread_mutex_t * lock,
				      void **list_tmp)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  __sctk_ethread_freeze_thread_on_vp (current_vp,
				      current,
				      sctk_ethread_mxn_mutex_unlock,
				      lock, (volatile void **) list_tmp);
}

static void
sctk_ethread_mxn_wake_thread_on_vp (void **list_tmp)
{
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);

  __sctk_ethread_wake_thread_on_vp (current_vp, list_tmp);
}

static void *
sctk_ethread_mxn_func_kernel_thread (void *arg)
{
  sctk_ethread_per_thread_t th_data;
  sctk_ethread_virtual_processor_t *vp;
  sctk_thread_data_t tmp = SCTK_THREAD_DATA_INIT;
  vp = (sctk_ethread_virtual_processor_t *) arg;
  sctk_ethread_init_data (&th_data);
  vp->current = &th_data;
  kthread_setspecific (sctk_ethread_key, vp);
  th_data.vp = vp;
  vp->idle = &th_data;
  th_data.status = ethread_idle;

  tmp.__start_routine = sctk_ethread_mxn_func_kernel_thread;
  tmp.virtual_processor = vp->rank;

  sctk_thread_data_set (&tmp);
  sctk_set_tls (NULL);

  //avoid to create an allocation chain
  sctk_alloc_posix_plug_on_egg_allocator();

  mpc_common_spinlock_lock (&sctk_ethread_key_spinlock);
  sctk_nb_vp_initialized++;
  mpc_common_spinlock_unlock (&sctk_ethread_key_spinlock);

  if (vp != NULL)
    {
      mpc_common_topo_bind_to_cpu (vp->bind_to);
/*     fprintf(stderr,"bind thread to %d\n",vp->bind_to); */
    }
  sctk_nodebug ("%d %d", mpc_common_topo_get_current_cpu (), vp->bind_to);
  __sctk_ethread_idle_task (&th_data);
  return NULL;
}

static void *
sctk_ethread_gen_func_kernel_thread (void *arg)
{
  sctk_ethread_per_thread_t th_data;
  sctk_ethread_virtual_processor_t *vp;
  int i;
  sctk_thread_data_t tmp = SCTK_THREAD_DATA_INIT;
  vp = (sctk_ethread_virtual_processor_t *) arg;
  mpc_common_topo_bind_to_cpu (vp->bind_to);
  sctk_ethread_init_data (&th_data);
  for (i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
    {
      th_data.tls[i] = NULL;
    }
  vp->current = &th_data;
  kthread_setspecific (sctk_ethread_key, vp);
  th_data.vp = vp;
  vp->idle = &th_data;
  th_data.status = ethread_idle;

  tmp.__start_routine = sctk_ethread_gen_func_kernel_thread;
  tmp.virtual_processor = vp->rank;

  sctk_thread_data_set (&tmp);
  sctk_set_tls (NULL);

  sctk_nodebug ("Entering in kernel idle");


  __sctk_ethread_kernel_idle_task (&th_data);
  return NULL;
}

static void
sctk_ethread_mxn_start_kernel_thread (int pos)
{
  kthread_t pid;
  sctk_ethread_virtual_processor_t tmp_init = SCTK_ETHREAD_VP_INIT;
  sctk_ethread_virtual_processor_t *tmp;
  mpc_common_topo_bind_to_cpu (pos);
  if (pos != 0)
    {
      tmp = (sctk_ethread_virtual_processor_t *)
	__sctk_malloc_new (sizeof (sctk_ethread_virtual_processor_t),
			   sctk_thread_tls);
      *tmp = tmp_init;
      tmp->rank = pos;
      sctk_ethread_mxn_vp_list[pos] = tmp;
    }
  sctk_ethread_mxn_vp_list[pos]->bind_to = pos;
  sctk_nodebug ("BIND to %d", pos);

  kthread_create (&pid,
		  sctk_ethread_mxn_func_kernel_thread,
		  sctk_ethread_mxn_vp_list[pos]);
/*   mpc_common_topo_bind_to_cpu(0); */
}

sctk_ethread_virtual_processor_t *
sctk_ethread_start_kernel_thread ()
{
  kthread_t pid;
  sctk_ethread_virtual_processor_t tmp_init = SCTK_ETHREAD_VP_INIT;
  sctk_ethread_virtual_processor_t *tmp;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  sctk_nodebug ("Create Kthread");

  sctk_thread_mutex_lock (&lock);

  tmp = (sctk_ethread_virtual_processor_t *)
    __sctk_malloc_new (sizeof (sctk_ethread_virtual_processor_t),
		       sctk_thread_tls);
  *tmp = tmp_init;
  tmp->rank = -1;

  kthread_create (&pid, sctk_ethread_gen_func_kernel_thread, tmp);
  sctk_nodebug ("Create Kthread done");

  sctk_thread_mutex_unlock (&lock);
  return tmp;
}

static void
sctk_ethread_mxn_init_vp (sctk_ethread_per_thread_t * th_data,
			  sctk_ethread_virtual_processor_t * vp)
{
  int i;
  /*Init thread and virtual processor */
  sctk_ethread_init_data (th_data);
  for (i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
    {
      th_data->tls[i] = NULL;
    }

  vp->current = th_data;
  kthread_setspecific (sctk_ethread_key, vp);

  th_data->vp = vp;
  for (i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
    {
      sctk_ethread_main_thread.tls[i] = NULL;
      sctk_ethread_destr_func_tab[i] = NULL;
    }

  {
    sctk_ethread_t idle;
    __sctk_ethread_create (ethread_idle,
			   vp, th_data, &idle, NULL, NULL, NULL);
  }

}

static void
sctk_ethread_mxn_init_virtual_processors ()
{
  unsigned int cpu_number;
  unsigned int i;

  use_ethread_mxn = 1;
  kthread_key_create (&sctk_ethread_key, NULL);

  /*Init main thread and virtual processor */
  sctk_ethread_mxn_init_vp (&sctk_ethread_main_thread, &virtual_processor);

  cpu_number = mpc_common_topo_get_pu_count ();

  sctk_nodebug ("starts %d virtual processors", cpu_number);

  sctk_ethread_mxn_vp_list = (sctk_ethread_virtual_processor_t **)
    __sctk_malloc (cpu_number *
		   sizeof (sctk_ethread_virtual_processor_t *),
		   sctk_thread_tls);
  sctk_ethread_mxn_vp_list[0] = &virtual_processor;

  for (i = 1; i < cpu_number; i++)
    {
      sctk_ethread_mxn_start_kernel_thread (i);
    }
  mpc_common_topo_bind_to_cpu (0);

  sctk_nodebug ("I'am %p", sctk_ethread_mxn_self ());
  while (cpu_number != sctk_nb_vp_initialized)
    {
      assume (sctk_nb_vp_initialized <= cpu_number);
      sched_yield ();
    }

  for (i = 0; i < cpu_number; i++)
    {
      assume (sctk_ethread_mxn_vp_list[i] != NULL);
    }
}

static int
sctk_ethread_mxn_get_vp ()
{
  /* Be careful the polling thread might
   * end up here we need to tell him that mxn
   * does not know where he is (-1) */
  sctk_ethread_per_thread_t *cur = sctk_ethread_mxn_self ();

  if( !cur )
        return -1;

  if( !cur->vp )
        return -1;

  return cur->vp->rank;
}

static int
sctk_ethread_mxn_thread_attr_setbinding (sctk_thread_attr_t * __attr, int __binding)
{
  sctk_ethread_attr_t * attr;

  attr = (sctk_ethread_attr_t *)__attr;
  if(attr == NULL){
    return SCTK_EINVAL;
  }
  if(attr->ptr == NULL){
      return SCTK_EINVAL;
    }


  attr->ptr->binding = __binding;
  return 0;
}

static int
sctk_ethread_mxn_thread_attr_getbinding (sctk_thread_attr_t * __attr, int *__binding)
{
  sctk_ethread_attr_t * attr;

  attr = (sctk_ethread_attr_t *)__attr;
  if(attr == NULL){
    return SCTK_EINVAL;
  }
  if(attr->ptr == NULL){
      return SCTK_EINVAL;
    }


  *__binding = attr->ptr->binding;

  return 0;
}

static int
sctk_ethread_mxn_thread_proc_migration (const int cpu)
{
  int last;
  sctk_ethread_per_thread_t *current;
  sctk_ethread_virtual_processor_t *current_vp;
  sctk_ethread_mxn_self_all (&current_vp, &current);
  last = current_vp->rank;

  sctk_assert (current->vp == current_vp);
  sctk_assert (last == sctk_thread_get_vp ());
  assume (cpu >= 0);
  assume (cpu < mpc_common_topo_get_pu_count ());
  assume (last >= 0);
  assume (last < mpc_common_topo_get_pu_count ());

  sctk_nodebug ("task %p Jump from %d to %d", current, last, cpu);

  if (cpu != last)
    {
      current->status = ethread_migrate;
      current->migration_func = sctk_ethread_mxn_migration_task;
      current->migrate_to = sctk_ethread_mxn_vp_list[cpu];
      __sctk_ethread_sched_yield_vp (current_vp, current);

      sctk_ethread_mxn_self_all (&current_vp, &current);
      current->status = ethread_migrate;
      current->migration_func = sctk_ethread_mxn_migration_task_relocalise;
      current->migrate_to = sctk_ethread_mxn_vp_list[cpu];
      __sctk_ethread_sched_yield_vp (current_vp, current);

      current->migration_func = NULL;
      current->migrate_to = NULL;
    }

  sctk_nodebug ("task %p Jump from %d to %d(%d) done", current, last, cpu,
		(sctk_ethread_virtual_processor_t
		 *) (sctk_ethread_mxn_self ()->vp)->rank);

  sctk_assert (current->vp == sctk_ethread_mxn_vp_list[cpu]);
  return last;
}

static double
sctk_ethread_mxn_get_activity (int i)
{
  return sctk_ethread_mxn_vp_list[i]->usage;
}


static int
sctk_ethread_mxn_equal (sctk_thread_t a, sctk_thread_t b)
{
  return (a == b);
}

static int
sctk_ethread_mxn_kill (sctk_ethread_per_thread_t * thread, int val)
{
  return __sctk_ethread_kill (thread, val);
}

static int
sctk_ethread_mxn_sigmask (int how, const sigset_t * newmask,
			  sigset_t * oldmask)
{
  sctk_ethread_per_thread_t *cur;
  cur = sctk_ethread_mxn_self ();
  return __sctk_ethread_sigmask (cur, how, newmask, oldmask);
}


static int
sctk_ethread_mxn_sigpending (sigset_t * set)
{
  int res;
  sctk_ethread_per_thread_t *cur;
  cur = sctk_ethread_mxn_self ();
  res = __sctk_ethread_sigpending (cur, set);
  return res;
}

static int
sctk_ethread_mxn_sigsuspend (sigset_t * set)
{
  int res;
  sctk_ethread_per_thread_t *cur;
  cur = sctk_ethread_mxn_self ();
  res = __sctk_ethread_sigsuspend (cur, set);
  return res;
}

static void sctk_ethread_mxn_at_fork_child(){
  /* sctk_error("Unable to fork with user threads child"); */
  /* sctk_abort(); */
}

static void sctk_ethread_mxn_at_fork_parent(){
  /* sctk_error("Unable to fork with user threads parent"); */
  /* sctk_abort(); */
}


static void sctk_ethread_mxn_at_fork_prepare(){
  /* sctk_error("Unable to fork with user threads prepare"); */
  /* sctk_abort(); */
}

void
sctk_ethread_mxn_thread_init (void)
{
  int i;
  sctk_only_once ();

  pthread_atfork(sctk_ethread_mxn_at_fork_prepare,sctk_ethread_mxn_at_fork_parent,sctk_ethread_mxn_at_fork_child);

  sctk_init_default_sigset ();

  sctk_ethread_check_size_eq (int, sctk_ethread_status_t);

  sctk_print_version ("Init Ethread_mxn", SCTK_LOCAL_VERSION_MAJOR,
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

  __sctk_ptr_thread_yield = sctk_ethread_mxn_sched_yield;
  __sctk_ptr_thread_dump = sctk_ethread_mxn_sched_dump;
  __sctk_ptr_thread_migrate = sctk_ethread_mxn_sched_migrate;
  __sctk_ptr_thread_restore = sctk_ethread_mxn_sched_restore;
  __sctk_ptr_thread_dump_clean = sctk_ethread_mxn_sched_dump_clean;

  __sctk_ptr_thread_attr_setbinding = sctk_ethread_mxn_thread_attr_setbinding;
  __sctk_ptr_thread_attr_getbinding = sctk_ethread_mxn_thread_attr_getbinding;

  __sctk_ptr_thread_proc_migration = sctk_ethread_mxn_thread_proc_migration;

  sctk_add_func_type (sctk_ethread_mxn, create, int (*)(sctk_thread_t *,
							const
							sctk_thread_attr_t
							*,
							void *(*)(void *),
							void *));
  sctk_add_func_type (sctk_ethread_mxn, user_create,
		      int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
			      void *(*)(void *), void *));

  sctk_ethread_check_size (sctk_ethread_attr_t, sctk_thread_attr_t);
  sctk_add_func_type (sctk_ethread_mxn, attr_init,
		      int (*)(sctk_thread_attr_t *));
  sctk_add_func_type (sctk_ethread_mxn, attr_destroy,
		      int (*)(sctk_thread_attr_t *));

  sctk_ethread_check_size (sctk_ethread_t, sctk_thread_t);
  sctk_add_func_type (sctk_ethread_mxn, self, sctk_thread_t (*)(void));
  sctk_add_func_type (sctk_ethread_mxn, self_check, sctk_thread_t (*)(void));
  sctk_add_func_type (sctk_ethread_mxn, equal,
		      int (*)(sctk_thread_t, sctk_thread_t));
  sctk_add_func_type (sctk_ethread_mxn, join,
		      int (*)(sctk_thread_t, void **));
  sctk_add_func_type (sctk_ethread_mxn, exit, void (*)(void *));
  sctk_add_func_type (sctk_ethread_mxn, kill, int (*)(sctk_thread_t, int));
  sctk_add_func_type (sctk_ethread_mxn, sigpending, int (*)(sigset_t *));
  sctk_add_func_type (sctk_ethread_mxn, sigsuspend, int (*)(sigset_t *));


  sctk_add_func_type (sctk_ethread_mxn, mutex_lock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_ethread_mxn, mutex_spinlock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_ethread_mxn, mutex_trylock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_ethread_mxn, mutex_unlock,
		      int (*)(sctk_thread_mutex_t *));
  __sctk_ptr_thread_mutex_init = sctk_ethread_mxn_mutex_init;


  sctk_ethread_check_size (int, sctk_thread_key_t);
  sctk_ethread_check_size (stck_ethread_key_destr_function_t,
			   void (*)(void *));
  sctk_add_func_type (sctk_ethread_mxn, key_create,
		      int (*)(sctk_thread_key_t *, void (*)(void *)));
  sctk_add_func_type (sctk_ethread_mxn, key_delete,
		      int (*)(sctk_thread_key_t));
  sctk_add_func_type (sctk_ethread_mxn, setspecific,
		      int (*)(sctk_thread_key_t, const void *));
  sctk_add_func_type (sctk_ethread_mxn, getspecific,
		      void *(*)(sctk_thread_key_t));

  sctk_add_func_type (sctk_ethread_mxn, get_activity, double (*)(int));

  sctk_add_func (sctk_ethread_mxn, wait_for_value_and_poll);
  sctk_add_func_type (sctk_ethread_mxn, freeze_thread_on_vp,
		      void (*)(sctk_thread_mutex_t *, void **));
  sctk_add_func (sctk_ethread_mxn, wake_thread_on_vp);
  sctk_add_func (sctk_ethread_mxn, get_vp);

  sctk_add_func_type (sctk_ethread_mxn, sigmask,
		      int (*)(int, const sigset_t *, sigset_t *));

  /** ** **/
  sctk_enable_lib_thread_db () ;
  /** **/
  sctk_ethread_mxn_init_virtual_processors ();

  sctk_posix_ethread_mxn ();

  sctk_multithreading_initialised = 1;
  sctk_nodebug ("FORCE allocation");
  sctk_free (sctk_malloc (5));
  sctk_nodebug ("FORCE allocation done %p %p", sctk_get_tls (),
		sctk_ethread_mxn_self ());
  sctk_thread_data_init ();

  sctk_ethread_mxn_sched_yield ();
#ifdef MPC_PosixAllocator
  sctk_add_global_var (&sctk_ethread_key_pos, sizeof (int));
#endif
}


#ifndef MPC_MPI
#ifndef SCTK_LIB_MODE

#ifdef HAVE_ENVIRON_VAR
#include <stdlib.h>
#include <stdio.h>
extern char ** environ;
#endif


int
/* main (int argc, char **argv) */
mpc_mpi_m_mpi_process_main (int argc, char **argv)
{
  int result;

#ifdef HAVE_ENVIRON_VAR
  result = mpc_user_main (argc, argv,environ);
#else
  result = mpc_user_main (argc, argv);
#endif

  return result;
}
#endif /* MPC_MPI */
#endif /* SCTK_LIB_MODE */
