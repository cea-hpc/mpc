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

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_scheduler.h"
#include "sctk_spinlock.h"
#include "sctk_thread_generic.h"
#include "sctk_kernel_thread.h"

static void (*sctk_thread_generic_idle_func)(int) = NULL;
void (*sctk_thread_generic_sched_yield)(sctk_thread_generic_scheduler_t*) = NULL;
void (*sctk_thread_generic_thread_status)(sctk_thread_generic_scheduler_t*,
					  sctk_thread_generic_thread_status_t) = NULL;
void (*sctk_thread_generic_register_spinlock_unlock)(sctk_thread_generic_scheduler_t*,
						     sctk_spinlock_t*) = NULL;
void (*sctk_thread_generic_wake)(sctk_thread_generic_scheduler_t*) = NULL;

void (*sctk_thread_generic_scheduler_init_thread_p)(sctk_thread_generic_scheduler_t* ) = NULL;

void (*sctk_thread_generic_sched_create)(sctk_thread_generic_p_t*) = NULL;
void (*sctk_thread_generic_add_task)(sctk_thread_generic_task_t*) = NULL;
/***************************************/
/* VP MANAGEMENT                       */
/***************************************/
typedef struct {
  sctk_thread_generic_p_t*thread;
  int core;
} sctk_thread_generic_scheduler_task_t;
static __thread int core_id = -1;

static void* sctk_thread_generic_scheduler_idle_task(sctk_thread_generic_scheduler_task_t* arg){
  int core;
  sctk_thread_generic_t th;
  core = arg->core;
  if(arg->core >= 0){
    sctk_bind_to_cpu (arg->core);
  }
  core_id = arg->core;
  sctk_free(arg);
  arg = NULL;
  
  sctk_thread_generic_set_self(th);
  sctk_thread_generic_scheduler_init_thread(&(sctk_thread_generic_self()->sched),th);
  sctk_thread_generic_keys_init_thread(&(sctk_thread_generic_self()->keys));

  sctk_thread_generic_idle_func(core);

  return NULL;
}

static void* sctk_thread_generic_scheduler_bootstrap_task(sctk_thread_generic_scheduler_task_t* arg){
  sctk_thread_generic_p_t*thread;
  if(arg->core >= 0){
    sctk_bind_to_cpu (arg->core);
  }
  core_id = arg->core;
  thread = arg->thread;

  sctk_free(arg);
  arg = NULL;

  sctk_thread_generic_set_self(thread);
  sctk_swapcontext(&(thread->sched.ctx_bootstrap),&(thread->sched.ctx));

  not_implemented();

  return NULL;
}

void sctk_thread_generic_scheduler_create_vp(sctk_thread_generic_p_t*thread,int core){
  void *(*start_routine) (void *);
  sctk_thread_generic_scheduler_task_t * arg;
  kthread_t kthread;

  if(thread == NULL){
    start_routine = (void *(*) (void *))sctk_thread_generic_scheduler_idle_task;
  } else {
    start_routine = (void *(*) (void *))sctk_thread_generic_scheduler_bootstrap_task;
  }

  arg = sctk_malloc(sizeof(sctk_thread_generic_scheduler_task_t));
  arg->thread = thread;
  arg->core = core;

  assume(kthread_create(&kthread,start_routine,arg) == 0);
}

/***************************************/
/* CONTEXT MANAGEMENT                  */
/***************************************/
void sctk_thread_generic_scheduler_swapcontext(sctk_thread_generic_scheduler_t* old_th,
					       sctk_thread_generic_scheduler_t* new_th){
  sctk_thread_generic_set_self(new_th->th);
  sctk_swapcontext(&(old_th->ctx),&(new_th->ctx));
  
}

/***************************************/
/* CENTRALIZED SCHEDULER               */
/***************************************/
static sctk_spinlock_t sctk_centralized_sched_list_lock = SCTK_SPINLOCK_INITIALIZER;
static volatile sctk_thread_generic_scheduler_centralized_t* sctk_centralized_sched_list = NULL;

static volatile sctk_thread_generic_task_t* sctk_centralized_task_list = NULL;
static sctk_spinlock_t sctk_centralized_task_list_lock = SCTK_SPINLOCK_INITIALIZER;

static __thread sctk_thread_generic_task_t* sctk_centralized_delegated_task_list = NULL;
static __thread volatile sctk_thread_generic_scheduler_t* sctk_centralized_delegated_zombie_list = NULL;

static inline void sctk_centralized_add_to_list(sctk_thread_generic_scheduler_t* sched){
  sctk_spinlock_lock(&sctk_centralized_sched_list_lock);
  DL_APPEND(sctk_centralized_sched_list,&(sched->centralized));
  sctk_spinlock_unlock(&sctk_centralized_sched_list_lock);
}

static inline sctk_thread_generic_scheduler_t* sctk_centralized_get_from_list(){
  if(sctk_centralized_sched_list != NULL){
    sctk_thread_generic_scheduler_centralized_t* res;
    sctk_spinlock_lock(&sctk_centralized_sched_list_lock);
    res = sctk_centralized_sched_list;
    DL_DELETE(sctk_centralized_sched_list,res);
    sctk_spinlock_unlock(&sctk_centralized_sched_list_lock);
    return res->sched;
  } else {
    return NULL;
  }
}

static void
sctk_centralized_freeze_thread_on_vp (sctk_thread_mutex_t * lock, void **list)
{
  not_implemented();
}

static void
sctk_centralized_wake_thread_on_vp (void **list)
{
  if(*list != NULL){
    not_implemented();
  }
}

static void sctk_centralized_idle_func(int core){
  not_implemented();
}

static void sctk_centralized_sched_yield(sctk_thread_generic_scheduler_t*sched){
  sctk_thread_generic_scheduler_t* next;

  if(sctk_centralized_delegated_task_list != NULL){ 
    sctk_spinlock_lock(&sctk_centralized_task_list_lock);
    DL_APPEND(sctk_centralized_task_list,sctk_centralized_delegated_task_list);
    sctk_spinlock_unlock(&sctk_centralized_task_list_lock);
    sctk_centralized_delegated_task_list = NULL;
  }

  if(sched->status == sctk_thread_generic_running){
    sctk_centralized_add_to_list(sched);
  } else {
    if(sched->status == sctk_thread_generic_zombie){
      sctk_centralized_delegated_zombie_list = sched;
    }
  }

  next = sctk_centralized_get_from_list();

  if((next != NULL) && (next != sched)){
    sctk_thread_generic_scheduler_swapcontext(sched,next);
  } else {
    not_implemented();
  }

  if(sctk_centralized_delegated_zombie_list != NULL){
    not_implemented();
  }
}

static 
void sctk_centralized_thread_status(sctk_thread_generic_scheduler_t* sched,
				    sctk_thread_generic_thread_status_t status){
  sched->status = status;
}

static void sctk_centralized_register_spinlock_unlock(sctk_thread_generic_scheduler_t* sched,
						      sctk_spinlock_t* lock){
  not_implemented();
}

static void sctk_centralized_wake(sctk_thread_generic_scheduler_t* sched){
  not_implemented();
}

static void sctk_centralized_add_task(sctk_thread_generic_task_t* task){
  if(task->is_blocking){
    sctk_centralized_thread_status(task->sched,sctk_thread_generic_blocked);
    assume(sctk_centralized_delegated_task_list == NULL);
    DL_APPEND(sctk_centralized_delegated_task_list,task);
    sctk_centralized_sched_yield(task->sched);
  } else {
    sctk_spinlock_lock(&sctk_centralized_task_list_lock);
    DL_APPEND(sctk_centralized_task_list,task);
    sctk_spinlock_unlock(&sctk_centralized_task_list_lock);
  }
}

static void sctk_centralized_create(sctk_thread_generic_p_t*thread){
  sctk_thread_generic_scheduler_init_thread(&(thread->sched),thread);

  fprintf(stderr,"Create thread scope %d = %d SYSTEM\n",thread->attr.scope,SCTK_THREAD_SCOPE_SYSTEM);
  if(thread->attr.scope == SCTK_THREAD_SCOPE_SYSTEM){
    thread->sched.centralized.vp_type = 1;
    sctk_thread_generic_scheduler_create_vp(thread,thread->attr.bind_to);
  } else {
    sctk_centralized_add_to_list(&(thread->sched));
  }
}

static void sctk_centralized_scheduler_init_thread(sctk_thread_generic_scheduler_t* sched){
  sched->centralized.vp_type = 0;
  sched->centralized.sched = sched;
  sched->centralized.next = NULL;
  sched->centralized.prev = NULL;
}


/***************************************/
/* INIT                                */
/***************************************/

static int
sctk_thread_generic_scheduler_get_vp ()
{
  return core_id;
}


static char sched_type[4096];
void sctk_thread_generic_scheduler_init(char* scheduler_type, int vp_number){ 
  int i;

  __sctk_ptr_thread_get_vp = sctk_thread_generic_scheduler_get_vp;

  sprintf(sched_type,"%s",scheduler_type);
  core_id = 0;
  sctk_bind_to_cpu (0);
  
  if(strcmp("centralized",scheduler_type) ==0){
    sctk_thread_generic_idle_func = sctk_centralized_idle_func;
    sctk_thread_generic_sched_yield = sctk_centralized_sched_yield;
    sctk_thread_generic_thread_status = sctk_centralized_thread_status;
    sctk_thread_generic_register_spinlock_unlock = sctk_centralized_register_spinlock_unlock;
    sctk_thread_generic_wake = sctk_centralized_wake;
    sctk_thread_generic_scheduler_init_thread_p = sctk_centralized_scheduler_init_thread;
    sctk_thread_generic_sched_create = sctk_centralized_create;
    sctk_thread_generic_add_task = sctk_centralized_add_task;
    sctk_add_func_type (sctk_centralized, freeze_thread_on_vp,
		      void (*)(sctk_thread_mutex_t *, void **));
    sctk_add_func (sctk_centralized, wake_thread_on_vp);
  } else {
    not_reachable();
  }

  for(i = 1; i < vp_number; i++){
    sctk_thread_generic_scheduler_create_vp(NULL,i);
  }
  
}

void sctk_thread_generic_scheduler_init_thread(sctk_thread_generic_scheduler_t* sched,
					       struct sctk_thread_generic_p_s* th){
  sched->th = th;
  sched->status = sctk_thread_generic_running;
  sctk_centralized_scheduler_init_thread(sched);
}

char* sctk_thread_generic_scheduler_get_name(){
  return sched_type;
}
