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
#include <sctk_thread_keys.h>
#include <sctk_thread_mutex.h>
#include <sctk_thread_scheduler.h>

/***************************************/
/* THREADS                             */
/***************************************/

typedef struct{
  sctk_thread_generic_keys_t keys;
  sctk_thread_generic_scheduler_t sched;
} sctk_thread_generic_p_t;

typedef sctk_thread_generic_p_t* sctk_thread_generic_t;

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
  not_implemented();
  return 0;
}

static int
sctk_thread_generic_mutex_lock (sctk_thread_mutex_t * lock)
{
  not_implemented();
  return 0;
}

static int
sctk_thread_generic_mutex_trylock (sctk_thread_mutex_t * lock)
{
  not_implemented();
  return 0;
}

static int
sctk_thread_generic_mutex_spinlock (sctk_thread_mutex_t * lock)
{
  not_implemented();
  return 0;
}

static int
sctk_thread_generic_mutex_unlock (sctk_thread_mutex_t * lock)
{
  not_implemented();
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

  sctk_multithreading_initialised = 1;

  sctk_thread_data_init ();
}

/***************************************/
/* IMPLEMENTATION SPECIFIC             */
/***************************************/

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
}
