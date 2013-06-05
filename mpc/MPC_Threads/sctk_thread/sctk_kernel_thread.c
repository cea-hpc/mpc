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
#include "sctk_asm.h"
#include "sctk_pthread.h"
#include "sctk_kernel_thread.h"
#include "sctk_spinlock.h"
#include "sctk_tls.h"
#include <string.h>
#include <semaphore.h>
#include "sctk_runtime_config.h"

#ifndef SCTK_KERNEL_THREAD_USE_TLS
int
kthread_key_create (kthread_key_t * key, void (*destr_function) (void *))
{
  return pthread_key_create ((pthread_key_t *) key, destr_function);
}

int
kthread_key_delete (kthread_key_t key)
{
  pthread_key_t keya;
  pthread_key_t *keyp;
  keyp = (pthread_key_t *) & key;
  keya = *keyp;
  return pthread_key_delete (keya);
}

int
kthread_setspecific (kthread_key_t key, const void *pointer)
{
  pthread_key_t keya;
  pthread_key_t *keyp;
  keyp = (pthread_key_t *) & key;
  keya = *keyp;
  return pthread_setspecific (keya, pointer);
}

void *
kthread_getspecific (kthread_key_t key)
{
  pthread_key_t keya;
  pthread_key_t *keyp;
  keyp = (pthread_key_t *) & key;
  keya = *keyp;
  return pthread_getspecific (keya);
}
#endif


typedef void *(*start_routine_t) (void *) ;

typedef struct kthread_create_start_s
{
  volatile start_routine_t start_routine;
  volatile void *arg;
  volatile int started;
  volatile int used;
  volatile struct kthread_create_start_s* next;
  sem_t sem;
} kthread_create_start_t;

static sctk_spinlock_t lock = 0;
static volatile kthread_create_start_t* list = NULL;

static void *
kthread_create_start_routine (void *t_arg)
{
  kthread_create_start_t slot;
  memcpy(&slot,t_arg,sizeof(kthread_create_start_t));
  ((kthread_create_start_t*)t_arg)->started = 1;

  //avoir to create an allocation chain 
  sctk_alloc_posix_plug_on_egg_allocator();

  sem_init(&(slot.sem), 0,0);

  sctk_spinlock_lock(&lock);

  if( list !=  &slot )
    slot.next = list;
  else
    slot.next = NULL;

  list = &slot;
  sctk_spinlock_unlock(&lock);

  while(1){
    void *(*start_routine) (void *);
    void *arg;

    start_routine = (void *(*) (void *))slot.start_routine;
    arg = (void*)slot.arg;

    start_routine (arg);

    slot.used = 0;

    sctk_nodebug("Kernel thread done");

    sem_wait(&(slot.sem));

    while(slot.used != 1){
      sched_yield();
    }

    sctk_nodebug("Kernel thread reuse");
  }
  return NULL;
}

int
kthread_create (kthread_t * thread, void *(*start_routine) (void *),
		void *arg)
{
  kthread_create_start_t* found = NULL;
  kthread_create_start_t* cursor;
  size_t kthread_stack_size = sctk_runtime_config_get()->modules.thread.kthread_stack_size;

  sctk_nodebug("Scan already started kthreads");
  sctk_spinlock_lock(&lock);
  cursor = (kthread_create_start_t*)list;
  while(cursor != NULL){
    if(cursor->used == 0){
      found = (kthread_create_start_t*)cursor;
      found->used = 1;
      break;
    }
    cursor = (kthread_create_start_t*)cursor->next;
  }
  sctk_spinlock_unlock(&lock);
  sctk_nodebug("Scan already started kthreads done found %p",found);

  if(found){
    sctk_nodebug("Reuse old thread %p",found);
    found->arg = arg;
    found->start_routine = start_routine;
    found->started = 0;
    sem_post(&(found->sem));
    return 0;
  } else {
    pthread_attr_t attr;
    int res;
    kthread_create_start_t tmp;
    sctk_nodebug("Create new kernel thread");

    res = pthread_attr_init (&attr);

    sctk_nodebug( "kthread_create: value returned by attr_init %d", res ) ;

    res = pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);

    sctk_nodebug( "kthread_create: value returned by attr_setscope %d", res ) ;

#ifdef PTHREAD_STACK_MIN
    if (PTHREAD_STACK_MIN > kthread_stack_size)
      {
	res = pthread_attr_setstacksize (&attr, PTHREAD_STACK_MIN);

	sctk_nodebug( "kthread_create: value returned by attr_setstacksize(PTHREAD %d) %d",
		      PTHREAD_STACK_MIN, res ) ;
      }
    else
      {
	res = pthread_attr_setstacksize (&attr, kthread_stack_size);

	sctk_nodebug( "kthread_create: value returned by attr_setstacksize(KTHREAD %d) %d",
		      kthread_stack_size, res ) ;

      }
#else
    res = pthread_attr_setstacksize (&attr, kthread_stack_size);

    sctk_nodebug( "kthread_create: value returned by attr_setstacksize(KTHREAD_NO_PTHREAD %d) %d",
		  kthread_stack_size, res ) ;

#endif
    assume (sizeof (kthread_t) == sizeof (pthread_t));

    tmp.started = 0;
    tmp.arg = arg;
    tmp.start_routine = start_routine;
    tmp.used = 1;

    sctk_nodebug( "kthread_create: before pthread_create") ;

    res =
      pthread_create ((pthread_t *) thread, &attr, kthread_create_start_routine,
		      &tmp);

    if(res != 0){
      res = pthread_attr_destroy (&attr);
      res = pthread_attr_init (&attr);
      res = pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);
      res =
	pthread_create ((pthread_t *) thread,  &attr, kthread_create_start_routine,
			&tmp);
    }
    assume(res == 0);

    sctk_nodebug( "kthread_create: after pthread_create with res = %d", res) ;

    pthread_attr_destroy (&attr);

    if ( res != 0 ) {
      sctk_nodebug( "Warning: Creating kernel threads with no attribute" ) ;
      res = pthread_create ((pthread_t *) thread, NULL, kthread_create_start_routine, &tmp);

      assume( res == 0 ) ;

    }

    while (tmp.started != 1)
    {
      sched_yield ();
    }
    return res;
  }
}

int
kthread_join (kthread_t th, void **thread_return)
{
  return pthread_join ((pthread_t) th, thread_return);
}


int
kthread_kill (kthread_t th, int val)
{
  return pthread_kill ((pthread_t) th, val);
}


kthread_t
kthread_self ()
{
  return (kthread_t) pthread_self ();
}

int
kthread_sigmask (int how, const sigset_t * newmask, sigset_t * oldmask)
{
  return pthread_sigmask (how, newmask, oldmask);
}

void
kthread_exit (void *retval)
{
  pthread_exit (retval);
}
