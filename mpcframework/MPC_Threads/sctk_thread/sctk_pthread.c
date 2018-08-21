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

#include "sctk_config_pthread.h"

#if HAVE_PTHREAD_ATTR_SETAFFINITY_NP
#define _GNU_SOURCE
#include <sched.h>
#endif /* HAVE_PTHREAD_ATTR_SETAFFINITY_NP */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "sctk_pthread.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_tls.h"
#include "sctk_internal_thread.h"
#include "sctk_posix_pthread.h"
#include "sctk_kernel_thread.h"
#include <semaphore.h>
#ifdef MPC_Message_Passing
#include <sctk_inter_thread_comm.h>
#include <sctk_communicator.h>
#include "sctk_asm.h"
#endif
#include "sctk_asm.h"


#if !defined(HAVE_PTHREAD_CREATE)
#error "Bad pthread detection"
#endif

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

extern volatile int sctk_online_program;

static void
pthread_wait_for_value_and_poll( volatile int *data, int value,
								 void ( *func )( void * ), void *arg )
{
	int i = 0;
	while ( ( *data ) != value )
	{
		if ( func != NULL )
		{
			func( arg );
		}
		/*       sctk_thread_yield(); */
		if ( ( *data ) != value )
		{
			if ( i >= 100 )
			{
#ifdef MPC_Message_Passing
  #ifndef SCTK_ENABLE_SPINNING
          sctk_notify_idle_message();
          kthread_usleep( 10 );
  #endif
#else
				kthread_usleep( 10 );
#endif
				i = 0;
			}
			else
			{
				/* If the spinning is enable, we slow down the the progression thread */
#ifdef SCTK_ENABLE_SPINNING
				sleep( 1 );
#endif
				sctk_cpu_relax();
			}
			i++;
		}
	}
}

typedef struct sctk_cell_s
{
  int val;
  struct sctk_cell_s *next;
} sctk_cell_t;

static void
pthread_freeze_thread_on_vp (sctk_thread_mutex_t * lock, void **list)
{
  sctk_cell_t cell;

  cell.val = 0;
  cell.next = (sctk_cell_t *) * list;
  *list = &cell;
  sctk_thread_mutex_unlock (lock);
  sctk_thread_wait_for_value (&(cell.val), 1);
}

static void
pthread_wake_thread_on_vp (void **list)
{
  sctk_cell_t *tmp;
  sctk_cell_t *cur;
  cur = (sctk_cell_t *) * list;
  while (cur != NULL)
    {
      tmp = cur->next;
      cur->val = 1;
      cur = tmp;
    }
  *list = NULL;
}

static int
sctk_pthread_mutex_init (sctk_thread_mutex_t * mutex,
			 const sctk_thread_mutexattr_t * mutex_attr)
{
  int res;
  res =
    pthread_mutex_init ((pthread_mutex_t *) mutex,
			(pthread_mutexattr_t *) mutex_attr);
  return res;
}

static int
sctk_pthread_get_vp ()
{
  int cpu = sctk_get_cpu();
  return cpu;
}

static sem_t sctk_pthread_user_create_sem;
static void *(*sctk_pthread_user_create_start_routine) (void *);

static void
sctk_thread_exit_cleanup_def (void *arg)
{
  sctk_thread_exit_cleanup ();
  assume (arg == NULL);
}

typedef struct
{
  void *arg;
  void *(*start_routine) (void *);
  void *tls;
} tls_start_routine_arg_t;


static void *
tls_start_routine (void *arg)
{
  tls_start_routine_arg_t *tmp;
  void *res;
  tmp = arg;
#if defined(MPC_USE_EXTLS)
  /**((extls_ctx_t**)extls_get_context_storage_addr()) = tmp->tls;*/
  extls_ctx_restore((extls_ctx_t *)tmp->tls);
#endif

  res = tmp->start_routine (tmp->arg);

  sctk_free (tmp);
  return res;
}

static void *
init_tls_start_routine_arg (void *(*start_routine) (void *), void *arg)
{
  tls_start_routine_arg_t *tmp;
  tmp = sctk_malloc (sizeof (tls_start_routine_arg_t));

  tmp->arg = arg;
  tmp->start_routine = start_routine;
#ifdef MPC_USE_EXTLS
  extls_ctx_t *ctx = sctk_calloc(1, sizeof(extls_ctx_t));
  extls_ctx_herit(*((extls_ctx_t**)extls_get_context_storage_addr()), ctx, LEVEL_THREAD);
  tmp->tls = (void*)ctx;
#endif
  return tmp;
}

static void *
def_start_routine (void *arg)
{
  void *tmp;
  void *(*sctk_pthread_user_create_start_routine_local) (void *);
  sctk_pthread_user_create_start_routine_local =
    sctk_pthread_user_create_start_routine;
  sem_post (&sctk_pthread_user_create_sem);

  pthread_cleanup_push (sctk_thread_exit_cleanup_def, NULL);
  tmp = sctk_pthread_user_create_start_routine_local (arg);
  pthread_cleanup_pop (0);


  return tmp;
}

static int
pthread_user_create (pthread_t * thread, pthread_attr_t * attr,
		     void *(*start_routine) (void *), void *arg)
{
  size_t size;
  sem_wait (&sctk_pthread_user_create_sem);
  sctk_pthread_user_create_start_routine = start_routine;

  if (sctk_is_in_fortran == 1)
    size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
  else
    size = SCTK_ETHREAD_STACK_SIZE;

  size += sctk_extls_size();


  if ((attr == NULL) && (sctk_is_in_fortran != 1))
    {
      pthread_attr_t tmp_attr;
      int res;
      pthread_attr_init (&tmp_attr);

#ifdef PTHREAD_STACK_MIN
      if (PTHREAD_STACK_MIN > size)
	{
	  res = pthread_attr_setstacksize (&tmp_attr, PTHREAD_STACK_MIN);
	}
      else
	{
	  res = pthread_attr_setstacksize (&tmp_attr, size);
	}
#else
      res = pthread_attr_setstacksize (&tmp_attr, size);
#endif

      if(res != 0){
	perror("pthread_attr_setstacksize: ");
	assume(res == 0);
      }

      res =
	pthread_create (thread, &tmp_attr, tls_start_routine,
			init_tls_start_routine_arg (def_start_routine, arg));
      if(res != 0){
	perror("pthread_create: ");
	assume(res == 0);
      }
      pthread_attr_destroy (&tmp_attr);
      return res;
    }
  else
  {
    int res;

    if(attr == NULL){
      pthread_attr_t tmp_attr;
      pthread_attr_init (&tmp_attr);
      attr=&tmp_attr;
    }


#ifdef PTHREAD_STACK_MIN
      if (PTHREAD_STACK_MIN > size)
	{
	  res = pthread_attr_setstacksize (attr, PTHREAD_STACK_MIN);
	}
      else
	{
	  res = pthread_attr_setstacksize (attr, size);
	}
#else
      res = pthread_attr_setstacksize (attr, size);
#endif

      if(res != 0){
	perror("pthread_attr_setstacksize: ");
	assume(res == 0);
      }

      res = pthread_create (thread, attr, tls_start_routine,
			    init_tls_start_routine_arg (def_start_routine,
							arg));
      /* Sylvain: see file sctk_thread.c for more details about
       * the following commented block */
#if 0
      res = sctk_real_pthread_create (thread, attr, tls_start_routine,
			    init_tls_start_routine_arg (def_start_routine,
							arg));
#endif
      if(res != 0){
	perror("pthread_create: ");
	assume(res == 0);
      }
      return res;
    }
}

#define pthread_check_size(a,b) sctk_size_checking(sizeof(a),sizeof(b),SCTK_STRING(a),SCTK_STRING(b),__FILE__,__LINE__)
#define pthread_check_size_eq(a,b) sctk_size_checking_eq(sizeof(a),sizeof(b),SCTK_STRING(a),SCTK_STRING(b),__FILE__,__LINE__)

static int
pthread_mutex_spinlock (pthread_mutex_t * lock)
{
  return pthread_mutex_lock (lock);
}

static inline pthread_t
pthread_self_check ()
{
  return pthread_self ();
}

static int
local_pthread_create (pthread_t * restrict thread,
		      const pthread_attr_t * restrict attr,
		      void *(*start_routine) (void *), void *restrict arg)
{
  if (attr == NULL)
    {
      pthread_attr_t tmp_attr;
      int res;
      size_t size;
      char * env;
      pthread_attr_init (&tmp_attr);

      /* We bind the MPI tasks in a round robin manner */
      env = getenv("MPC_ENABLE_PTHREAD_PINNING");
      if ( env != NULL ){
        sctk_thread_data_t * data = (sctk_thread_data_t*) arg;
        int cpu_number = sctk_get_cpu_number ();

        sctk_debug("Bind VP to core %d\n", data->local_task_id % cpu_number);
        sctk_bind_to_cpu (data->local_task_id);
      }

      if (sctk_is_in_fortran == 1)
	size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
      else
	size = SCTK_ETHREAD_STACK_SIZE;

      size += sctk_extls_size();

#ifdef PTHREAD_STACK_MIN
      if (PTHREAD_STACK_MIN > size)
	{
	  res = pthread_attr_setstacksize (&tmp_attr, PTHREAD_STACK_MIN);
	}
      else
	{
	  res = pthread_attr_setstacksize (&tmp_attr, size);
	}
#else
      res = pthread_attr_setstacksize (&tmp_attr, size);
#endif
      if(res != 0){
	perror("pthread_attr_setstacksize: ");
	assume(res == 0);
      }

      res =
	pthread_create (thread, &tmp_attr, tls_start_routine,
			init_tls_start_routine_arg (start_routine, arg));
      if(res != 0){
	perror("pthread_create: ");
	assume(res == 0);
      }
      pthread_attr_destroy (&tmp_attr);
      return res;
    }
  else
    {
      int res;
      res = pthread_create (thread, attr, start_routine, arg);

      if(res != 0){
	perror("pthread_create: ");
	assume(res == 0);
      }
      return res;
    }
}

static int
sctk_pthread_thread_attr_setbinding (sctk_thread_attr_t * __attr, int __binding)
{
#if HAVE_PTHREAD_ATTR_SETAFFINITY_NP
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(__binding, &mask);
    pthread_attr_setaffinity_np(__attr, sizeof(cpu_set_t), &mask);
#endif /* HAVE_PTHREAD_ATTR_SETAFFINITY_NP */
    return 0;
}

static int
sctk_pthread_thread_attr_getbinding (sctk_thread_attr_t * __attr, int *__binding)
{
  return 0;
}

#if SCTK_FUTEX_SUPPORTED

#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>

int pthread_futex(void *addr1, int op, int val1, 
					 struct timespec *timeout, void *addr2, int val3)
{
	/* Here we call the actual futex implementation */
	return syscall( SYS_futex, addr1, op, val1, timeout, addr2, val3 );
}
#else
int pthread_futex(void *addr1, int op, int val1, 
					 struct timespec *timeout, void *addr2, int val3)
{
	not_implemented();
}
#endif

static void sctk_pthread_at_fork_child() {
  sem_post(&sctk_pthread_user_create_sem);
}

static void sctk_pthread_at_fork_parent() {
  sem_post(&sctk_pthread_user_create_sem);
}

static void sctk_pthread_at_fork_prepare() {
  sem_wait(&sctk_pthread_user_create_sem);
}

void
sctk_pthread_thread_init (void)

{
/*   pthread_mutex_t loc = PTHREAD_MUTEX_INITIALIZER; */
/*   sctk_thread_mutex_t glob = SCTK_THREAD_MUTEX_INITIALIZER; */
  sctk_only_once ();

  sem_init (&sctk_pthread_user_create_sem, 0, 1);

  pthread_atfork(sctk_pthread_at_fork_prepare, sctk_pthread_at_fork_parent,
                 sctk_pthread_at_fork_child);

  sctk_print_version ("Init Pthread", SCTK_LOCAL_VERSION_MAJOR,
		      SCTK_LOCAL_VERSION_MINOR);

  pthread_check_size (pthread_mutex_t, sctk_thread_mutex_t);
  pthread_check_size (pthread_mutexattr_t, sctk_thread_mutexattr_t);

  __sctk_ptr_thread_attr_setbinding = sctk_pthread_thread_attr_setbinding;
  __sctk_ptr_thread_attr_getbinding = sctk_pthread_thread_attr_getbinding;

/*   assume (memcmp (&loc, &glob, sizeof (pthread_mutex_t)) == 0); */

/*   sctk_add_func (pthread, yield); */
  __sctk_ptr_thread_yield = sched_yield;
  sctk_add_func_type (local_pthread, create, int (*)(sctk_thread_t *,
						     const sctk_thread_attr_t
						     *, void *(*)(void *),
						     void *));
  sctk_add_func_type (pthread, user_create,
		      int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
			      void *(*)(void *), void *));
  sctk_add_func_type (pthread, join, int (*)(sctk_thread_t, void **));
  sctk_add_func_type (pthread, attr_init, int (*)(sctk_thread_attr_t *));
  sctk_add_func_type (pthread, attr_destroy, int (*)(sctk_thread_attr_t *));
  sctk_add_func_type (pthread, attr_setscope,
		      int (*)(sctk_thread_attr_t *, int));
  sctk_add_func_type (pthread, self, sctk_thread_t (*)(void));
  sctk_add_func_type (pthread, self_check, sctk_thread_t (*)(void));
  sctk_add_func_type (pthread, mutex_lock, int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (pthread, mutex_spinlock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (pthread, mutex_trylock, int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (pthread, mutex_unlock, int (*)(sctk_thread_mutex_t *));

  __sctk_ptr_thread_mutex_init = sctk_pthread_mutex_init;

  sctk_add_func_type (pthread, setspecific, int (*)(sctk_thread_key_t,
						    const void *));
  sctk_add_func_type (pthread, getspecific, void *(*)(sctk_thread_key_t));
  sctk_add_func_type (pthread, key_create, int (*)(sctk_thread_key_t *,
						   void (*)(void *)));
  sctk_add_func_type (pthread, key_delete, int (*)(sctk_thread_key_t));

  sctk_add_func (pthread, wait_for_value_and_poll);
  sctk_add_func (pthread, freeze_thread_on_vp);
  sctk_add_func (pthread, wake_thread_on_vp);
  sctk_add_func (sctk_pthread, get_vp);
  sctk_add_func_type (pthread, equal, int (*)(sctk_thread_t, sctk_thread_t));
  sctk_add_func_type (pthread, exit, void (*)(void *));
  
  sctk_add_func_type (pthread, futex, int (*)(void *, int , int ,  struct timespec *, void *, int ));
  
#ifdef HAVE_PTHREAD_KILL
  sctk_add_func_type (pthread, kill, int (*)(sctk_thread_t, int));
#endif

#ifndef WINDOWS_SYS
  __sctk_ptr_thread_sigpending = (int (*)(sigset_t *)) sigpending;
  __sctk_ptr_thread_sigsuspend = (int (*)(sigset_t *)) sigsuspend;
#endif

#ifdef HAVE_PTHREAD_SIGMASK
  sctk_add_func_type (pthread, sigmask,
		      int (*)(int, const sigset_t *, sigset_t *));
#endif

  sctk_posix_pthread ();

  sctk_multithreading_initialised = 1;

  sctk_thread_data_init ();

  sctk_free (sctk_malloc (5));

#ifdef MPC_Message_Passing
  sctk_set_net_migration_available (0);
#endif
}
