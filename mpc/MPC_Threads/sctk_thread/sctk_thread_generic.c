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
#include <sctk_config.h>
#include <sctk_thread.h>
#include <sctk_thread_generic.h>
#include <stdlib.h>

/***************************************/
/* THREADS                             */
/***************************************/
static __thread sctk_thread_generic_p_t* sctk_thread_generic_self_data;

inline
sctk_thread_generic_t sctk_thread_generic_self(){
  return sctk_thread_generic_self_data;
}

void sctk_thread_generic_set_self(sctk_thread_generic_t th){
  sctk_thread_generic_self_data = th;
}
/***************************************/
/* KEYS                                */
/***************************************/
static int
sctk_thread_generic_setspecific (sctk_thread_key_t __key, const void *__pointer)
{
  return sctk_thread_generic_keys_setspecific(__key,__pointer,&(sctk_thread_generic_self()->keys));
}

static void *
sctk_thread_generic_getspecific (sctk_thread_key_t __key)
{
  return sctk_thread_generic_keys_getspecific(__key,&(sctk_thread_generic_self()->keys));
}

static int
sctk_thread_generic_key_create (sctk_thread_key_t * __key,
				void (*__destr_function) (void *))
{
  return sctk_thread_generic_keys_key_create(__key,__destr_function,&(sctk_thread_generic_self()->keys));
}

static int
sctk_thread_generic_key_delete (sctk_thread_key_t __key)
{
  return  sctk_thread_generic_keys_key_delete(__key,&(sctk_thread_generic_self()->keys));
}


/***************************************/
/* MUTEX                               */
/***************************************/
static int
sctk_thread_generic_mutex_init (sctk_thread_mutex_t * lock,
				const sctk_thread_mutexattr_t * attr)
{
  return sctk_thread_generic_mutexes_mutex_init((sctk_thread_generic_mutex_t *)lock,
						(sctk_thread_generic_mutexattr_t*)attr,
						&(sctk_thread_generic_self()->sched));
}

static int
sctk_thread_generic_mutex_lock (sctk_thread_mutex_t * lock)
{
  return sctk_thread_generic_mutexes_mutex_lock((sctk_thread_generic_mutex_t *)lock,
						&(sctk_thread_generic_self()->sched));
}

static int
sctk_thread_generic_mutex_trylock (sctk_thread_mutex_t * lock)
{
  return sctk_thread_generic_mutexes_mutex_trylock((sctk_thread_generic_mutex_t *)lock,
						&(sctk_thread_generic_self()->sched));
}

static int
sctk_thread_generic_mutex_spinlock (sctk_thread_mutex_t * lock)
{
  return sctk_thread_generic_mutexes_mutex_spinlock((sctk_thread_generic_mutex_t *)lock,
						&(sctk_thread_generic_self()->sched));
}

static int
sctk_thread_generic_mutex_unlock (sctk_thread_mutex_t * lock)
{
  return sctk_thread_generic_mutexes_mutex_unlock((sctk_thread_generic_mutex_t *)lock,
						&(sctk_thread_generic_self()->sched));
}

/***************************************/
/* CONDITIONS                          */
/***************************************/
static int
sctk_thread_generic_cond_init (sctk_thread_cond_t * lock,
				const sctk_thread_condattr_t * attr)
{
  return sctk_thread_generic_conds_cond_init((sctk_thread_generic_cond_t *)lock,
						(sctk_thread_generic_condattr_t*)attr,
						&(sctk_thread_generic_self()->sched));
}

static int
sctk_thread_generic_cond_wait (sctk_thread_cond_t * cond,
				sctk_thread_mutex_t * mutex)
{
  return sctk_thread_generic_conds_cond_wait((sctk_thread_generic_cond_t *)cond,
						(sctk_thread_generic_mutex_t*)mutex,
						&(sctk_thread_generic_self()->sched));
}

static int
sctk_thread_generic_cond_signal (sctk_thread_cond_t * lock)
{
  return sctk_thread_generic_conds_cond_signal((sctk_thread_generic_cond_t *)lock,
						&(sctk_thread_generic_self()->sched));
}

static int
sctk_thread_generic_cond_broadcast (sctk_thread_cond_t * lock)
{
  return sctk_thread_generic_conds_cond_broadcast((sctk_thread_generic_cond_t *)lock,
						&(sctk_thread_generic_self()->sched));
}

/***************************************/
/* THREAD CREATION                     */
/***************************************/

int
sctk_thread_generic_attr_init (sctk_thread_generic_attr_t * attr)
{
  sctk_thread_generic_intern_attr_t init = sctk_thread_generic_intern_attr_init;
  attr->ptr = (sctk_thread_generic_intern_attr_t *)
    sctk_malloc (sizeof (sctk_thread_generic_intern_attr_t));

  *(attr->ptr) = init;
  return 0;
}

static int
sctk_thread_generic_attr_destroy (sctk_thread_generic_attr_t * attr)
{
  sctk_free (attr->ptr);
  return 0;
}

static int
sctk_thread_generic_attr_getscope (const sctk_thread_generic_attr_t * attr, int *scope)
{
  sctk_thread_generic_intern_attr_t init = sctk_thread_generic_intern_attr_init;
  sctk_thread_generic_intern_attr_t * ptr;

  ptr = attr->ptr;

  if(ptr == NULL){
    ptr = &init;
  }
  *scope = ptr->scope;
  return 0;
}

static int
sctk_thread_generic_attr_setscope (sctk_thread_generic_attr_t * attr, int scope)
{
  if(attr->ptr == NULL){
    sctk_thread_generic_attr_init(attr);
  }
  attr->ptr->scope = scope;
  return 0;
}

static  void __sctk_start_routine (void * arg){
  sctk_thread_generic_p_t*thread;

  thread = arg;

  sctk_debug("Before yield %p",&(thread->sched));

  sctk_thread_generic_sched_yield(&(thread->sched));

  sctk_debug("Start %p %p",&(thread->sched),thread->attr.arg);
  thread->attr.return_value = thread->attr.start_routine(thread->attr.arg);
  sctk_debug("End %p %p",&(thread->sched),thread->attr.arg);

  /* Handel Exit */
  if(thread->attr.scope == SCTK_THREAD_SCOPE_SYSTEM){
    sctk_swapcontext(&(thread->sched.ctx),&(thread->sched.ctx_bootstrap));
  } else {
    sctk_thread_generic_thread_status(&(thread->sched),sctk_thread_generic_zombie);
    sctk_thread_generic_sched_yield(&(thread->sched));
  }
  not_reachable();
}

int
sctk_thread_generic_user_create (sctk_thread_generic_t * threadp,
				 sctk_thread_generic_attr_t * attr,
				 void *(*start_routine) (void *), void *arg)
{
  sctk_thread_generic_intern_attr_t init = sctk_thread_generic_intern_attr_init;
  sctk_thread_generic_intern_attr_t * ptr;
  sctk_thread_data_t tmp;
  sctk_thread_generic_t thread_id;
  char* stack;
  size_t stack_size;

  if(attr == NULL){
    ptr = &init;
  } else {
    ptr = attr->ptr;
    
    if(ptr == NULL){
      ptr = &init;
    }
  }

  sctk_debug("Create %p",arg);

  if (arg != NULL)
    {
      tmp = *((sctk_thread_data_t *) arg);
      if (tmp.tls == NULL)
	{
	  tmp.tls = sctk_thread_tls;
	}
    }
  else
    {
      tmp.tls = sctk_thread_tls;
    }

  /*Create data struct*/
  {
    thread_id = 
      __sctk_malloc (sizeof (sctk_thread_generic_p_t), tmp.tls);

    thread_id->attr = *ptr;

    sctk_thread_generic_scheduler_init_thread(&(thread_id->sched),thread_id);
    sctk_thread_generic_keys_init_thread(&(thread_id->keys));
  }

  /*Allocate stack*/
  {
    stack = thread_id->attr.stack;
    stack_size = thread_id->attr.stack_size;
    
    if (stack == NULL)
      {
	if (sctk_is_in_fortran == 1 && stack_size <= 0)
	  stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	else if (stack_size <= 0)
	  stack_size = SCTK_ETHREAD_STACK_SIZE;
	if (stack == NULL)
	  {
	    stack = (char *) __sctk_malloc (stack_size + 8, tmp.tls);
	    if (stack == NULL)
	      {
		sctk_free(thread_id);
		return SCTK_EAGAIN;
	      }
	  }
	stack[stack_size] = 123;

      }
    else if (stack_size <= 0)
      {
	sctk_free(thread_id);
	return SCTK_EINVAL;
      }

    thread_id->attr.stack = stack;
    thread_id->attr.stack_size = stack_size;
  }
  

  /*Create context*/
  {
    thread_id->attr.start_routine = start_routine;
    thread_id->attr.arg = arg;
    sctk_debug("STACK %p STACK SIZE %lu",stack,stack_size);
    sctk_makecontext (&(thread_id->sched.ctx),
		      (void *) thread_id,
		      __sctk_start_routine, stack, stack_size);
  }


  *threadp = thread_id;
  
  sctk_thread_generic_sched_create(thread_id);
  return 0;
}
static int
sctk_thread_generic_create (sctk_thread_generic_t * threadp,
				 sctk_thread_generic_attr_t * attr,
				 void *(*start_routine) (void *), void *arg)
{
  static unsigned int pos = 0;
  int res;
  sctk_thread_generic_attr_t tmp_attr;

  tmp_attr.ptr = NULL;

  if(attr == NULL){
    attr = &tmp_attr;
  }
  if(attr->ptr == NULL){
    sctk_thread_generic_attr_init(attr);
  }

  attr->ptr->bind_to = sctk_get_init_vp (pos);
  pos++;

  res = sctk_thread_generic_user_create(threadp,attr,start_routine,arg);

  sctk_thread_generic_attr_destroy(&tmp_attr);

  return res;
}

/***************************************/
/* THREAD POLLING                      */
/***************************************/

static void
sctk_thread_generic_wait_for_value_and_poll (volatile int *data, int value,
					     void (*func) (void *), void *arg)
{
  sctk_thread_generic_task_t task;
  if(func){
    func(arg);
  }
  if(*data == value){
    return;
  } 

  task.sched = &(sctk_thread_generic_self()->sched);
  task.func = func;
  task.value = value;
  task.arg = arg;
  task.data = data;
  task.is_blocking = 1;

  sctk_thread_generic_add_task(&task); 

} 
/***************************************/
/* THREAD ONCE                         */
/***************************************/
typedef sctk_spinlock_t sctk_thread_generic_once_t;

  static inline int __sctk_thread_generic_once_initialized (sctk_thread_generic_once_t *
						     once_control)
  {
#ifdef sctk_thread_once_t_is_contiguous_int
    return (*((sctk_thread_once_t *) once_control) == SCTK_THREAD_ONCE_INIT);
#else
    sctk_thread_once_t once_init = SCTK_THREAD_ONCE_INIT;
    return (memcpy
	    ((void *) &once_init, (void *) once_control,
	     sizeof (sctk_thread_once_t)) == 0);
#endif
  }

static int
sctk_thread_generic_once (sctk_thread_generic_once_t * once_control,
			  void (*init_routine) (void))
{
    static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
    if (__sctk_thread_generic_once_initialized (once_control))
      {
	sctk_thread_mutex_lock (&lock);
	if (__sctk_thread_generic_once_initialized (once_control))
	  {
#ifdef MPC_Allocator
#ifdef SCTK_USE_TLS
	    sctk_add_global_var ((void *) once_control,
				 sizeof (sctk_thread_generic_once_t));
#else
#warning "Once backup disabled"
#endif
#endif
	    init_routine ();
#ifdef sctk_thread_once_t_is_contiguous_int
	    *once_control = !SCTK_THREAD_ONCE_INIT;
#else
	    once_control[0] = 1;
#endif
	  }
	sctk_thread_mutex_unlock (&lock);
      }
  return 0;
}

/***************************************/
/* YIELD                               */
/***************************************/
static int
sctk_thread_generic_thread_sched_yield ()
{
  sctk_thread_generic_sched_yield(&(sctk_thread_generic_self()->sched));
  return 0;
}

/***************************************/
/* INIT                                */
/***************************************/
static void
sctk_thread_generic_thread_init (char* thread_type,char* scheduler_type, int vp_number){
  sctk_only_once ();
  sctk_thread_generic_self_data = sctk_malloc(sizeof(sctk_thread_generic_p_t));

  sctk_thread_generic_check_size (sctk_thread_generic_t, sctk_thread_t);
  sctk_add_func_type (sctk_thread_generic, self, sctk_thread_t (*)(void));

  /****** SCHEDULER ******/
  sctk_thread_generic_scheduler_init(thread_type,scheduler_type,vp_number);
  sctk_thread_generic_scheduler_init_thread(&(sctk_thread_generic_self()->sched),
					    sctk_thread_generic_self());

  /****** KEYS ******/
  sctk_thread_generic_keys_init();
  sctk_add_func_type (sctk_thread_generic, key_create,
		      int (*)(sctk_thread_key_t *, void (*)(void *)));
  sctk_add_func_type (sctk_thread_generic, key_delete,
		      int (*)(sctk_thread_key_t));
  sctk_add_func_type (sctk_thread_generic, setspecific,
		      int (*)(sctk_thread_key_t, const void *));
  sctk_add_func_type (sctk_thread_generic, getspecific,
		      void *(*)(sctk_thread_key_t));
  sctk_thread_generic_keys_init_thread(&(sctk_thread_generic_self()->keys));

  /****** MUTEX ******/
  sctk_thread_generic_mutexes_init(); 
  sctk_add_func_type (sctk_thread_generic, mutex_lock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_thread_generic, mutex_spinlock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_thread_generic, mutex_trylock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_thread_generic, mutex_unlock,
		      int (*)(sctk_thread_mutex_t *));
  __sctk_ptr_thread_mutex_init = sctk_thread_generic_mutex_init;

  /****** COND ******/
  sctk_thread_generic_conds_init();
  __sctk_ptr_thread_cond_init = sctk_thread_generic_cond_init;
   sctk_add_func_type (sctk_thread_generic, cond_wait,
		      int (*)(sctk_thread_cond_t*,sctk_thread_mutex_t *));
   sctk_add_func_type (sctk_thread_generic, cond_signal,
		      int (*)(sctk_thread_cond_t *));
   sctk_add_func_type (sctk_thread_generic, cond_broadcast,
		      int (*)(sctk_thread_cond_t *));
#warning "ADD destroy and timedwait"

  /****** THREAD CREATION ******/  
  sctk_thread_generic_check_size (sctk_thread_generic_attr_t, sctk_thread_attr_t);
  sctk_add_func_type (sctk_thread_generic, attr_init,
		      int (*)(sctk_thread_attr_t *));
  sctk_add_func_type (sctk_thread_generic, attr_destroy,
		      int (*)(sctk_thread_attr_t *));
  sctk_add_func_type (sctk_thread_generic, attr_getscope,
		      int (*)(const sctk_thread_attr_t *, int *));
  sctk_add_func_type (sctk_thread_generic, attr_setscope,
		      int (*)(sctk_thread_attr_t *, int));
  sctk_add_func_type (sctk_thread_generic, user_create,
		      int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
			      void *(*)(void *), void *));
  sctk_add_func_type (sctk_thread_generic, create,
		      int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
			      void *(*)(void *), void *));

  /****** THREAD POLLING ******/  
  sctk_add_func (sctk_thread_generic, wait_for_value_and_poll);
  
  /****** THREAD ONCE ******/ 
  sctk_thread_generic_check_size (sctk_thread_generic_once_t, sctk_thread_once_t);
  sctk_add_func_type (sctk_thread_generic, once,
		      int (*)(sctk_thread_once_t *, void (*)(void)));

  /****** YIELD ******/ 
  __sctk_ptr_thread_yield = sctk_thread_generic_thread_sched_yield;

  sctk_multithreading_initialised = 1;

  sctk_thread_data_init ();
}

/***************************************/
/* IMPLEMENTATION SPECIFIC             */
/***************************************/
static char sched_type[4096];
void sctk_register_thread_type(char* type){
  sprintf(sched_type,"[%s:%s]",type,sctk_thread_generic_scheduler_get_name());
  sctk_multithreading_mode = sched_type;
}

int sctk_get_env_cpu_nuber(){
  int cpu_number; 
  char* env;
  cpu_number = sctk_get_cpu_number ();
  env = getenv("SCTK_SET_CORE_NUMBER");
  if(env != NULL){
    cpu_number = atoi(env);
    assume(cpu_number > 0);
  }
  return cpu_number;
}

/********* ETHREAD MXN ************/
void
sctk_ethread_mxn_ng_thread_init (void){
  
  sctk_thread_generic_thread_init ("ethread_mxn","generic/centralized",sctk_get_env_cpu_nuber());
  sctk_register_thread_type("ethread_mxn_ng");
}

/********* ETHREAD ************/
void
sctk_ethread_ng_thread_init (void){
  sctk_thread_generic_thread_init ("ethread_mxn","generic/centralized",1);
  sctk_register_thread_type("ethread_ng");
}

/********* PTHREAD ************/
void
sctk_pthread_ng_thread_init (void){
  sctk_thread_generic_thread_init ("pthread","generic/centralized",sctk_get_env_cpu_nuber());
  sctk_register_thread_type("pthread_ng");
}
