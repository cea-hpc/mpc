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

static void (*sctk_thread_generic_sched_idle_start)(void);

void (*sctk_thread_generic_sched_yield)(sctk_thread_generic_scheduler_t*) = NULL;
void (*sctk_thread_generic_thread_status)(sctk_thread_generic_scheduler_t*,
					  sctk_thread_generic_thread_status_t) = NULL;
void (*sctk_thread_generic_register_spinlock_unlock)(sctk_thread_generic_scheduler_t*,
						     sctk_spinlock_t*) = NULL;
void (*sctk_thread_generic_wake)(sctk_thread_generic_scheduler_t*) = NULL;

void (*sctk_thread_generic_scheduler_init_thread_p)(sctk_thread_generic_scheduler_t* ) = NULL;

void (*sctk_thread_generic_sched_create)(sctk_thread_generic_p_t*) = NULL;
void (*sctk_thread_generic_add_task)(sctk_thread_generic_task_t*) = NULL;
static void* (*sctk_thread_generic_polling_func)(void*) = NULL;
/***************************************/
/* VP MANAGEMENT                       */
/***************************************/
typedef struct {
  sctk_thread_generic_p_t*thread;
  int core;
} sctk_thread_generic_scheduler_task_t;
static __thread int core_id = -1;
static int sctk_thread_generic_scheduler_use_binding = 1;

static void sctk_thread_generic_scheduler_bind_to_cpu (int core){
  if(sctk_thread_generic_scheduler_use_binding == 1){
    sctk_bind_to_cpu (core);
  }
}

static void* sctk_thread_generic_scheduler_idle_task(sctk_thread_generic_scheduler_task_t* arg){
  int core;
  sctk_thread_generic_p_t p_th;
  sctk_thread_generic_t th;
  core = arg->core;
  if(arg->core >= 0){
    sctk_thread_generic_scheduler_bind_to_cpu (arg->core);
  }
  core_id = arg->core;
  sctk_free(arg);
  arg = NULL;

  th = &p_th;
  
  sctk_thread_generic_set_self(th);
  sctk_thread_generic_scheduler_init_thread(&(sctk_thread_generic_self()->sched),th);
  sctk_thread_generic_keys_init_thread(&(sctk_thread_generic_self()->keys));

  /* Start Idle*/
  sctk_thread_generic_sched_idle_start();

  sctk_debug("End vp");
  
#warning "Handle zombies"
  not_implemented();
  return NULL;
}

static void* sctk_thread_generic_scheduler_bootstrap_task(sctk_thread_generic_scheduler_task_t* arg){
  sctk_thread_generic_p_t*thread;
  if(arg->core >= 0){
    sctk_thread_generic_scheduler_bind_to_cpu (arg->core);
  }
  core_id = arg->core;
  thread = arg->thread;

  sctk_free(arg);
  arg = NULL;

  sctk_thread_generic_set_self(thread);
  sctk_swapcontext(&(thread->sched.ctx_bootstrap),&(thread->sched.ctx));

  sctk_debug("End vp");

#warning "Handle zombies"
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
#include <pthread.h>
void sctk_thread_generic_scheduler_swapcontext(sctk_thread_generic_scheduler_t* old_th,
					       sctk_thread_generic_scheduler_t* new_th){
  sctk_thread_generic_set_self(new_th->th);
  sctk_debug("SWAP %p %p -> %p",pthread_self(),&(old_th->ctx),&(new_th->ctx));
  sctk_spinlock_unlock(&(old_th->debug_lock));
  assume(sctk_spinlock_trylock(&(new_th->debug_lock)) == 0);
  sctk_swapcontext(&(old_th->ctx),&(new_th->ctx));
}
void sctk_thread_generic_scheduler_setcontext(sctk_thread_generic_scheduler_t* new_th){
  sctk_thread_generic_set_self(new_th->th);
  sctk_debug("SET %p",&(new_th->ctx));
  sctk_setcontext(&(new_th->ctx));
   assume(sctk_spinlock_trylock(&(new_th->debug_lock)) == 0);
	
}

/***************************************/
/* TASKS MANAGEMENT                    */
/***************************************/
static inline int sctk_thread_generic_scheduler_check_task(sctk_thread_generic_task_t* task){
  if(*(task->data) == task->value){
    return 1;
  } 
  if(task->func){
    task->func(task->arg);
    if(*(task->data) == task->value){
      return 1;
    } 
  }
  return 0;
}

/***************************************/
/* CENTRALIZED SCHEDULER               */
/***************************************/
static sctk_spinlock_t sctk_centralized_sched_list_lock = SCTK_SPINLOCK_INITIALIZER;
static volatile sctk_thread_generic_scheduler_generic_t* sctk_centralized_sched_list = NULL;

static volatile sctk_thread_generic_task_t* sctk_centralized_task_list = NULL;
static sctk_spinlock_t sctk_centralized_task_list_lock = SCTK_SPINLOCK_INITIALIZER;

static void sctk_centralized_add_to_list(sctk_thread_generic_scheduler_t* sched){
  assume(sched->generic.sched == sched);
  sctk_spinlock_lock(&sctk_centralized_sched_list_lock);
  DL_APPEND(sctk_centralized_sched_list,&(sched->generic));
  sctk_debug("ADD Thread %p",sched);
  sctk_spinlock_unlock(&sctk_centralized_sched_list_lock);
}

static 
void sctk_centralized_concat_to_list(sctk_thread_generic_scheduler_t* sched,
				     sctk_thread_generic_scheduler_generic_t*s_list){
    sctk_spinlock_lock(&sctk_centralized_sched_list_lock);
    DL_CONCAT(sctk_centralized_sched_list,s_list);
    sctk_spinlock_unlock(&sctk_centralized_sched_list_lock);    
}

static sctk_thread_generic_scheduler_t* sctk_centralized_get_from_list(){
  if(sctk_centralized_sched_list != NULL){
    sctk_thread_generic_scheduler_generic_t* res;
    sctk_spinlock_lock(&sctk_centralized_sched_list_lock);
    res = sctk_centralized_sched_list;
    if(res != NULL){
      DL_DELETE(sctk_centralized_sched_list,res);
    }
    sctk_spinlock_unlock(&sctk_centralized_sched_list_lock);
    if(res != NULL){
      sctk_debug("REMOVE Thread %p",res->sched);
      return res->sched;
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

static sctk_thread_generic_scheduler_t* sctk_centralized_get_from_list_pthread_init(){
  if(sctk_centralized_sched_list != NULL){
    sctk_thread_generic_scheduler_generic_t* res_tmp;
    sctk_thread_generic_scheduler_generic_t* res;
    sctk_spinlock_lock(&sctk_centralized_sched_list_lock);
    DL_FOREACH_SAFE(sctk_centralized_sched_list,res,res_tmp){    
    if((res != NULL) && (res->vp_type == 4)){
      DL_DELETE(sctk_centralized_sched_list,res);
      break;
    }
    }
    sctk_spinlock_unlock(&sctk_centralized_sched_list_lock);
    if(res != NULL){
      sctk_debug("REMOVE Thread %p",res->sched);
      return res->sched;
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}



static sctk_thread_generic_task_t* sctk_centralized_get_task(){
  sctk_thread_generic_task_t* task = NULL;
  if(sctk_centralized_task_list != NULL){
    sctk_spinlock_lock(&sctk_centralized_task_list_lock);
    if(sctk_centralized_task_list != NULL){
      task = sctk_centralized_task_list;
      DL_DELETE(sctk_centralized_task_list,task);
    }
    sctk_spinlock_unlock(&sctk_centralized_task_list_lock);
  }
  return task;
}

static void sctk_centralized_add_task_to_threat(sctk_thread_generic_task_t* task){
    sctk_spinlock_lock(&sctk_centralized_task_list_lock);
    DL_APPEND(sctk_centralized_task_list,task);
    sctk_spinlock_unlock(&sctk_centralized_task_list_lock);
}

static 
void sctk_centralized_poll_tasks(sctk_thread_generic_scheduler_t* sched){
  sctk_thread_generic_task_t* task;
  sctk_thread_generic_task_t* task_tmp;
  
  if(sctk_spinlock_trylock(&sctk_centralized_task_list_lock) == 0){
  /* Exec polling */
  DL_FOREACH_SAFE(sctk_centralized_task_list,task,task_tmp){
    if(sctk_thread_generic_scheduler_check_task(task) == 1){
      DL_DELETE(sctk_centralized_task_list,task);
      sctk_debug("WAKE task %p",task);
      sctk_spinlock_lock(&(sched->generic.lock));
      sctk_thread_generic_wake(task->sched);
      sctk_spinlock_unlock(&(sched->generic.lock));
    }
  }
  sctk_spinlock_unlock(&sctk_centralized_task_list_lock);  
}
}

/***************************************/
/* GENERIC SCHEDULER                   */
/***************************************/
static void (*sctk_generic_add_to_list_intern)(sctk_thread_generic_scheduler_t* ) = NULL;
static sctk_thread_generic_scheduler_t* (*sctk_generic_get_from_list)(void) = NULL;
static sctk_thread_generic_scheduler_t* (*sctk_generic_get_from_list_pthread_init)(void) = NULL;
static sctk_thread_generic_task_t* (*sctk_generic_get_task)(void) = NULL;
static void (*sctk_generic_add_task_to_threat)(sctk_thread_generic_task_t* ) = NULL;
static void (*sctk_generic_concat_to_list)(sctk_thread_generic_scheduler_t* ,
					   sctk_thread_generic_scheduler_generic_t*) = NULL;
static void (*sctk_generic_poll_tasks)(sctk_thread_generic_scheduler_t* ) = NULL;


static __thread volatile sctk_thread_generic_task_t* sctk_generic_delegated_task_list = NULL;
static __thread volatile sctk_thread_generic_scheduler_t* sctk_generic_delegated_zombie_list = NULL;
static __thread volatile sctk_thread_generic_scheduler_t* sctk_generic_delegated_add = NULL;
static __thread volatile sctk_spinlock_t* sctk_generic_delegated_spinlock = NULL;


static void sctk_generic_add_to_list(sctk_thread_generic_scheduler_t* sched, int is_idle_mode){
  assume(sched->status == sctk_thread_generic_running);
  sctk_debug("ADD TASK %p idle mode %d",sched,is_idle_mode);
  if(is_idle_mode == 0){
    sctk_generic_add_to_list_intern(sched);
  } 
}

static void sctk_generic_add_task(sctk_thread_generic_task_t* task){
  sctk_debug("ADD task %p FROM %p",task,task->sched);
  if(task->is_blocking){
    sctk_thread_generic_thread_status(task->sched,sctk_thread_generic_blocked);
    assume(sctk_generic_delegated_task_list == NULL);
    DL_APPEND(sctk_generic_delegated_task_list,task);
    sctk_spinlock_lock(&(task->sched->generic.lock));
    sctk_thread_generic_register_spinlock_unlock(task->sched,&(task->sched->generic.lock));
    sctk_thread_generic_sched_yield(task->sched);
  } else {
    sctk_generic_add_task_to_threat(task);
  }
}

static void sctk_generic_sched_yield(sctk_thread_generic_scheduler_t*sched);

static void sctk_generic_sched_idle_start(){
  sctk_thread_generic_scheduler_t* next;
  do{
    sched_yield();
    next = sctk_generic_get_from_list();
  }while(next == NULL);
  sctk_debug("Launch %p",next);
  sctk_swapcontext(&(next->ctx_bootstrap),&(next->ctx));
  not_reachable();
}
static void sctk_generic_sched_idle_start_pthread(){
  sctk_thread_generic_scheduler_t* next;

  sctk_generic_delegated_task_list = NULL;
  sctk_generic_delegated_zombie_list = NULL;
  sctk_generic_delegated_add = NULL;
  
  do{
    sched_yield();
    next = sctk_generic_get_from_list_pthread_init();
  }while(next == NULL);
  sctk_debug("Launch PTHREAD %p",next);
  sctk_swapcontext(&(next->ctx_bootstrap),&(next->ctx));
  not_reachable();
}

static void sctk_generic_sched_yield(sctk_thread_generic_scheduler_t*sched){
  sctk_thread_generic_scheduler_t* next;

  if(sched->generic.vp_type == 0){
    int have_spinlock_registered = 0;
    static __thread sctk_thread_generic_scheduler_t*sched_idle = NULL;
    static __thread sctk_spinlock_t*registered_spin_unlock = NULL;

    if(sctk_generic_delegated_add){
      sctk_generic_add_to_list(sctk_generic_delegated_add,sctk_generic_delegated_add->generic.is_idle_mode);
      sctk_generic_delegated_add = NULL;
    }

    if(sched->status == sctk_thread_generic_running){
      assume(sctk_generic_delegated_add == NULL);
      sctk_generic_delegated_add = sched;

      if(sctk_generic_delegated_task_list != NULL){ 
	sctk_generic_add_task_to_threat(sctk_generic_delegated_task_list);
	sctk_generic_delegated_task_list = NULL;
      }

    } else {
      sctk_debug("TASK %p status %d type %d",sched,sched->status,sched->generic.vp_type);
      if(sched->status == sctk_thread_generic_zombie){
	assume(sctk_generic_delegated_zombie_list == NULL);
	sctk_generic_delegated_zombie_list = sched;
	sctk_debug("Zombie %p",sched);
      }
    }

    sched->generic.is_idle_mode = 0;

  retry:
    next = sctk_generic_get_from_list();
    
    if((next == NULL) && (sched->status == sctk_thread_generic_running)){
      sctk_generic_delegated_add = NULL;
      next = sched;
    } 
    if(next != sched){
      if(next != NULL){
	sctk_thread_generic_scheduler_swapcontext(sched,next);
      } else {
	/* Idle function */

	if(sctk_generic_delegated_task_list != NULL){
	  sched->generic.is_idle_mode = 1;
          sctk_generic_add_task_to_threat(sctk_generic_delegated_task_list);
          sctk_generic_delegated_task_list = NULL;
  	}

	if(sctk_generic_delegated_spinlock != NULL){
          sched->generic.is_idle_mode = 1;

	  have_spinlock_registered = 1;
	  assume(registered_spin_unlock == NULL);

	  sctk_debug("REGISTER delegated IDLE %p",sched);
	  sched_idle = sched;
	  registered_spin_unlock = sctk_generic_delegated_spinlock;

	  sctk_spinlock_unlock(sctk_generic_delegated_spinlock);
	  sctk_generic_delegated_spinlock = NULL;
	}

	sctk_cpu_relax ();
	goto retry;
      }
    }

    sched->generic.is_idle_mode = 0;

    if(sched_idle != NULL){
      if(sched_idle != sched){
	if(registered_spin_unlock != NULL){
	  sctk_spinlock_lock(registered_spin_unlock);
	  sched_idle->generic.is_idle_mode = 0;
	  if(sched_idle->status == sctk_thread_generic_running){
	    sctk_debug("ADD FROM delegated spinlock %p",sched_idle);
	    sctk_generic_add_to_list(sched_idle,sched_idle->generic.is_idle_mode);
	  }
	  sctk_spinlock_unlock(registered_spin_unlock);	
	}
      }
      registered_spin_unlock = NULL;
      sched_idle = NULL;
    }

    if(sctk_generic_delegated_spinlock != NULL){
      sctk_spinlock_unlock(sctk_generic_delegated_spinlock);
      sctk_generic_delegated_spinlock = NULL;
    }

    /* Deal with zombie threads */
    if(sctk_generic_delegated_zombie_list != NULL){
      (void)(0);
#warning "Handel Zombies"
      sctk_generic_delegated_zombie_list = NULL;
    }

    if(sctk_generic_delegated_task_list != NULL){ 
      sctk_generic_add_task_to_threat(sctk_generic_delegated_task_list);
      sctk_generic_delegated_task_list = NULL;
    }

    if(sctk_generic_delegated_add){
      sctk_generic_add_to_list(sctk_generic_delegated_add,sctk_generic_delegated_add->generic.is_idle_mode);
      sctk_generic_delegated_add = NULL;
    }

  } else {
    if(sctk_generic_delegated_task_list != NULL){ 
      sctk_generic_add_task_to_threat(sctk_generic_delegated_task_list);
      sctk_generic_delegated_task_list = NULL;
    }
    if(sctk_generic_delegated_spinlock != NULL){
      sctk_spinlock_unlock(sctk_generic_delegated_spinlock);
      sctk_generic_delegated_spinlock = NULL;
    }


    if(sched->generic.vp_type == 1){
      if(sched->status == sctk_thread_generic_zombie){
	sctk_swapcontext(&(sched->ctx),&(sched->ctx_bootstrap));
	not_reachable();
      }

      if(sched->status == sctk_thread_generic_blocked){
	not_implemented();
      }
    } else {
      if(sched->generic.vp_type == 4){
	sched->generic.vp_type = 3;
	sem_wait(&(sched->generic.sem));	
      }

      if(sched->status == sctk_thread_generic_running){
	sctk_generic_add_to_list(sched,sched->generic.is_idle_mode);
      }

    retry_system:
      next = sctk_generic_get_from_list();
      
      if(next != sched){
	if(next != NULL){
	  sctk_debug("SLEEP %p WAKE %p",sched,next);
	  sem_post(&(next->generic.sem));
	  sem_wait(&(sched->generic.sem));
	} else {
	  /* Idle function */
	  sctk_cpu_relax ();
	  goto retry_system;
	}	
      }
      if(sched->status == sctk_thread_generic_zombie){
	sctk_swapcontext(&(sched->ctx),&(sched->ctx_bootstrap));
	not_reachable();
      }
    }
  }
}

static 
void sctk_generic_thread_status(sctk_thread_generic_scheduler_t* sched,
				sctk_thread_generic_thread_status_t status){
  sched->status = status;
}

static void sctk_generic_register_spinlock_unlock(sctk_thread_generic_scheduler_t* sched,
						  sctk_spinlock_t* lock){
  sctk_generic_delegated_spinlock = lock;
}

static void sctk_generic_wake(sctk_thread_generic_scheduler_t* sched){
  int is_idle_mode;

  is_idle_mode = sched->generic.is_idle_mode;

  if(sched->generic.vp_type == 0){
    sched->status = sctk_thread_generic_running;
    sctk_generic_add_to_list(sched,is_idle_mode);
  } else {
    sched->status = sctk_thread_generic_running;
    if(sched->generic.vp_type == 1){
      sem_post(&(sched->generic.sem));
    } else {
      sctk_generic_add_to_list(sched,is_idle_mode);
    }
  }
}

static void
sctk_generic_freeze_thread_on_vp (sctk_thread_mutex_t * lock, void **list)
{
  sctk_thread_generic_scheduler_t*sched;
  sctk_thread_generic_scheduler_generic_t*s_list;

  sched = &(sctk_thread_generic_self()->sched);

  s_list = *list;
  DL_APPEND(s_list,&(sched->generic));
  *list = s_list;

  sctk_thread_mutex_unlock(lock);
  sctk_generic_thread_status(sched,sctk_thread_generic_blocked);
  sctk_generic_sched_yield(sched);
}

static void
sctk_generic_wake_thread_on_vp (void **list)
{
  if(*list != NULL){
    sctk_thread_generic_scheduler_generic_t*s_list;
    sctk_thread_generic_scheduler_t*sched;
    s_list = *list;
    sched = &(sctk_thread_generic_self()->sched);
    sctk_generic_concat_to_list(sched,s_list);
    *list = NULL; 
  }
}

static inline 
void sctk_generic_create_common(sctk_thread_generic_p_t*thread){
  if(thread->attr.scope == SCTK_THREAD_SCOPE_SYSTEM){
    thread->sched.generic.vp_type = 1;
   
    sctk_debug("Create thread scope %d (%d SYSTEM) vp _type %d",
	       thread->attr.scope,SCTK_THREAD_SCOPE_SYSTEM,thread->sched.generic.vp_type);
    sctk_thread_generic_scheduler_create_vp(thread,thread->attr.bind_to);
  } else {
    sctk_debug("Create thread scope %d (%d SYSTEM) vp _type %d",
	       thread->attr.scope,SCTK_THREAD_SCOPE_SYSTEM,thread->sched.generic.vp_type);
    sctk_generic_add_to_list(&(thread->sched),(&(thread->sched))->generic.is_idle_mode);
  }
}


static void sctk_generic_create(sctk_thread_generic_p_t*thread){
  sctk_thread_generic_scheduler_init_thread(&(thread->sched),thread);
  sctk_generic_create_common(thread);
}

static void sctk_generic_create_pthread(sctk_thread_generic_p_t*thread){
  sctk_thread_generic_scheduler_init_thread(&(thread->sched),thread);
  thread->sched.generic.vp_type = 4;

  if(thread->attr.scope != SCTK_THREAD_SCOPE_SYSTEM){
    sctk_thread_generic_scheduler_create_vp(thread,thread->attr.bind_to); 
    sctk_generic_add_to_list(&(thread->sched),(&(thread->sched))->generic.is_idle_mode);   
  } else {
    sctk_generic_create_common(thread);
  }
}

static void* sctk_generic_polling_func(void*arg){
  sctk_thread_generic_scheduler_t*sched;

  assume(arg == NULL); 

  sched = &(sctk_thread_generic_self()->sched);

  sctk_debug("Start polling func %p",sched);

  do{
    sctk_generic_poll_tasks(sched);
    sctk_generic_sched_yield(sched);
  }while(1);
}

static void sctk_generic_scheduler_init_thread_common(sctk_thread_generic_scheduler_t* sched){
  sched->generic.sched = sched;
  sched->generic.next = NULL;
  sched->generic.prev = NULL;
  sched->generic.is_idle_mode = 0;
  sched->generic.lock = SCTK_SPINLOCK_INITIALIZER;
  sem_init(&(sched->generic.sem),0,0);
}

static void sctk_generic_scheduler_init_thread(sctk_thread_generic_scheduler_t* sched){
  sched->generic.vp_type = 0;
  sctk_generic_scheduler_init_thread_common(sched);
}

static void sctk_generic_scheduler_init_pthread(sctk_thread_generic_scheduler_t* sched){
  sched->generic.vp_type = 3;
  sctk_generic_scheduler_init_thread_common(sched);
}


/***************************************/
/* INIT                                */
/***************************************/

static int
sctk_thread_generic_scheduler_get_vp ()
{
  return core_id;
}

static void* sctk_thread_generic_polling_func_bootstrap(void* attr){
  return sctk_thread_generic_polling_func(attr);
}

static char sched_type[4096];
void sctk_thread_generic_scheduler_init(char* thread_type,char* scheduler_type, int vp_number){ 
  int i;

  if(vp_number > sctk_get_cpu_number ()){
    sctk_thread_generic_scheduler_use_binding = 0;
  }

  __sctk_ptr_thread_get_vp = sctk_thread_generic_scheduler_get_vp;

  sprintf(sched_type,"%s",scheduler_type);
  core_id = 0;
  sctk_thread_generic_scheduler_bind_to_cpu (0);
  
  if(strcmp("generic/centralized",scheduler_type) == 0){
    sctk_generic_add_to_list_intern = sctk_centralized_add_to_list;
    sctk_generic_get_from_list = sctk_centralized_get_from_list;
    sctk_generic_get_task = sctk_centralized_get_task;
    sctk_generic_add_task_to_threat = sctk_centralized_add_task_to_threat;
    sctk_generic_concat_to_list = sctk_centralized_concat_to_list;
    sctk_generic_poll_tasks = sctk_centralized_poll_tasks;

    sctk_thread_generic_sched_idle_start = sctk_generic_sched_idle_start;
    sctk_thread_generic_sched_yield = sctk_generic_sched_yield;
    sctk_thread_generic_thread_status = sctk_generic_thread_status;
    sctk_thread_generic_register_spinlock_unlock = sctk_generic_register_spinlock_unlock;
    sctk_thread_generic_wake = sctk_generic_wake;
    sctk_thread_generic_scheduler_init_thread_p = sctk_generic_scheduler_init_thread;
    sctk_thread_generic_sched_create = sctk_generic_create;
    sctk_thread_generic_add_task = sctk_generic_add_task;
    sctk_add_func_type (sctk_generic, freeze_thread_on_vp,
		      void (*)(sctk_thread_mutex_t *, void **));
    sctk_add_func (sctk_generic, wake_thread_on_vp);
    sctk_thread_generic_polling_func = sctk_generic_polling_func;
    if(strcmp("pthread",thread_type) == 0){
      sctk_thread_generic_sched_create = sctk_generic_create_pthread;
      sctk_thread_generic_scheduler_init_thread_p = sctk_generic_scheduler_init_pthread;
      sctk_generic_get_from_list_pthread_init = sctk_centralized_get_from_list_pthread_init;
      sctk_thread_generic_sched_idle_start = sctk_generic_sched_idle_start_pthread;

    }
  } else {
    not_reachable();
  }

  if(strcmp("pthread",thread_type) != 0){
    for(i = 1; i < vp_number; i++){
      sctk_thread_generic_scheduler_create_vp(NULL,i);
    }
  }
  
  sctk_set_cpu_number (vp_number);

  {
    sctk_thread_generic_attr_t attr;
    sctk_thread_generic_t threadp;
    sctk_thread_generic_attr_init(&attr);

    attr.ptr->polling = 1;
    attr.ptr->stack_size = 8*1024;
    for(i = 0; i < vp_number; i++){
      attr.ptr->bind_to = i;
      sctk_thread_generic_user_create(&threadp,&attr,sctk_thread_generic_polling_func_bootstrap,NULL);
    }
  }
}

void sctk_thread_generic_scheduler_init_thread(sctk_thread_generic_scheduler_t* sched,
					       struct sctk_thread_generic_p_s* th){
  sched->th = th;
  sched->status = sctk_thread_generic_running;
  sched->debug_lock = SCTK_SPINLOCK_INITIALIZER;
  sctk_thread_generic_scheduler_init_thread_p(sched);
}

char* sctk_thread_generic_scheduler_get_name(){
  return sched_type;
}
