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

/***************************************/
/* THREADS                             */
/***************************************/
static inline 
sctk_thread_generic_t sctk_thread_generic_self_default(){
  not_implemented();
  return NULL;
}
static sctk_thread_generic_t (*sctk_thread_generic_self_p)(void) = sctk_thread_generic_self_default;
static inline 
sctk_thread_generic_t sctk_thread_generic_self(){
  return sctk_thread_generic_self_p();
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
/* THREAD CREATION                     */
/***************************************/

static int
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
  not_implemented();
}

static int
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

  ptr = attr->ptr;

  if(ptr == NULL){
    ptr = &init;
  }

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

    sctk_thread_generic_scheduler_init_thread(&(thread_id->sched));
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
    sctk_makecontext (&(thread_id->sched.ctx),
		      (void *) thread_id,
		      __sctk_start_routine, stack, stack_size);
  }


  *threadp = thread_id;
  sctk_thread_generic_create(thread_id);
  return 0;
}

/***************************************/
/* INIT                                */
/***************************************/
static void
sctk_thread_generic_thread_init (char* scheduler_type){
  sctk_only_once ();

  sctk_thread_generic_check_size (sctk_thread_generic_t, sctk_thread_t);
  sctk_add_func_type (sctk_thread_generic, self, sctk_thread_t (*)(void));

  /****** SCHEDULER ******/
  sctk_thread_generic_scheduler_init(scheduler_type);
  sctk_thread_generic_scheduler_init_thread(&(sctk_thread_generic_self()->sched));

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

/********* ETHREAD MXN ************/
static __thread sctk_thread_generic_p_t* ethread_mxn_self_data;
static
sctk_thread_generic_t sctk_thread_ethread_mxn_ng_self(){
  return ethread_mxn_self_data;
}

void
sctk_ethread_mxn_ng_thread_init (void){
  sctk_thread_generic_self_p = sctk_thread_ethread_mxn_ng_self;
  ethread_mxn_self_data = sctk_malloc(sizeof(sctk_thread_generic_p_t));
  assume(ethread_mxn_self_data != NULL);

  sctk_thread_generic_thread_init ("centralized");
  sctk_register_thread_type("ethread_mxn_ng");
}

/********* PTHREAD ************/
static __thread sctk_thread_generic_p_t pthread_self_data;
static
sctk_thread_generic_t sctk_thread_pthread_ng_self(){
  return &(pthread_self_data);
}

void
sctk_pthread_ng_thread_init (void){
  sctk_thread_generic_self_p = sctk_thread_pthread_ng_self;

  sctk_thread_generic_thread_init ("centralized");
  sctk_register_thread_type("pthread_ng");
}
