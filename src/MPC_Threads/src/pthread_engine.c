//* ############################# MPC License ############################## */
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



#if HAVE_PTHREAD_ATTR_SETAFFINITY_NP
#include <sched.h>
#endif /* HAVE_PTHREAD_ATTR_SETAFFINITY_NP */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "pthread_engine.h"

#include "mpc_common_debug.h"
#include "mpc_thread.h"
#include "tls.h"
#include "thread_ptr.h"

#include "thread.h"
#include "kthread.h"

#include <semaphore.h>
#include <mpc_common_flags.h>
#include <mpc_common_asm.h>

#include <mpc_topology.h>


#if !defined(HAVE_PTHREAD_CREATE)
#error "Bad pthread detection"
#endif

static void _mpc_thread_pthread_engine_wait_for_value_and_poll(volatile int *data, int value,
                                                               void (*func)(void *), void *arg)
{
	int i = 0;

	while( (*data) != value)
	{
		if(func != NULL)
		{
			func(arg);
		}

		if( (*data) != value)
		{
			if(i >= 2048)
			{
				kthread_usleep(i >> 10);
			}
			else
			{
				sctk_cpu_relax();
			}

			i++;
		}
	}
}

typedef struct sctk_cell_s
{
	int                 val;
	struct sctk_cell_s *next;
} sctk_cell_t;

static void _mpc_thread_pthread_engine_freeze_thread_on_vp(mpc_thread_mutex_t *lock, void **list)
{
	sctk_cell_t cell;

	cell.val  = 0;
	cell.next = (sctk_cell_t *)*list;
	*list     = &cell;
	mpc_thread_mutex_unlock(lock);
	mpc_thread_wait_for_value(&(cell.val), 1);
}

static void _mpc_thread_pthread_engine_wake_thread_on_vp(void **list)
{
	sctk_cell_t *tmp;
	sctk_cell_t *cur;

	cur = (sctk_cell_t *)*list;
	while(cur != NULL)
	{
		tmp      = cur->next;
		cur->val = 1;
		cur      = tmp;
	}
	*list = NULL;
}

static int _mpc_thread_pthread_engine_mutex_init(mpc_thread_mutex_t *mutex, const mpc_thread_mutexattr_t *mutex_attr)
{
	int res;

	res =
	        pthread_mutex_init( (pthread_mutex_t *)mutex,
	                            (pthread_mutexattr_t *)mutex_attr);
	return res;
}

static sem_t _mpc_thread_pthread_engine_user_create_sem;
static void *(*_mpc_thread_pthread_engine_user_create_start_routine) (void *);

static void _mpc_thread_exit_cleanup_def(void *arg)
{
	_mpc_thread_exit_cleanup();
	assume(arg == NULL);
}

typedef struct
{
	void *arg;
	void *(*start_routine)(void *);
	void *tls;
} tls_start_routine_arg_t;


static void *tls_start_routine(void *arg)
{
	tls_start_routine_arg_t *tmp;
	void *res;

	tmp = arg;
#if defined(MPC_USE_EXTLS)
	extls_ctx_restore( (extls_ctx_t *)tmp->tls);
#endif

	res = tmp->start_routine(tmp->arg);

	sctk_free(tmp);
	return res;
}

static void *init_tls_start_routine_arg(void *(*start_routine)(void *), void *arg)
{
	tls_start_routine_arg_t *tmp;

	tmp = sctk_malloc(sizeof(tls_start_routine_arg_t) );

	tmp->arg           = arg;
	tmp->start_routine = start_routine;
#ifdef MPC_USE_EXTLS
	tmp->tls = *(void **)sctk_get_ctx_addr();
#endif
	return tmp;
}

static void *def_start_routine(void *arg)
{
	void *tmp;
	void *(*_mpc_thread_pthread_engine_user_create_start_routine_local) (void *);

	_mpc_thread_pthread_engine_user_create_start_routine_local =
	        _mpc_thread_pthread_engine_user_create_start_routine;
	sem_post(&_mpc_thread_pthread_engine_user_create_sem);

	pthread_cleanup_push(_mpc_thread_exit_cleanup_def, NULL);
	tmp = _mpc_thread_pthread_engine_user_create_start_routine_local(arg);
	pthread_cleanup_pop(0);


	return tmp;
}

static int _mpc_thread_pthread_engine_user_create(pthread_t *thread, pthread_attr_t *attr,
                                                  void *(*start_routine)(void *), void *arg)
{
	size_t size;

	sem_wait(&_mpc_thread_pthread_engine_user_create_sem);
	_mpc_thread_pthread_engine_user_create_start_routine = start_routine;

	if(mpc_common_get_flags()->is_fortran == 1)
	{
		size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	}
	else
	{
		size = SCTK_ETHREAD_STACK_SIZE;
	}

	size += sctk_extls_size();


	if( (attr == NULL) && (mpc_common_get_flags()->is_fortran != 1) )
	{
		pthread_attr_t tmp_attr;
		int            res;
		pthread_attr_init(&tmp_attr);

#ifdef PTHREAD_STACK_MIN
		if(PTHREAD_STACK_MIN > size)
		{
			res = pthread_attr_setstacksize(&tmp_attr, PTHREAD_STACK_MIN);
		}
		else
		{
			res = pthread_attr_setstacksize(&tmp_attr, size);
		}
#else
		res = pthread_attr_setstacksize(&tmp_attr, size);
#endif

		if(res != 0)
		{
			perror("pthread_attr_setstacksize: ");
			assume(res == 0);
		}

		res =
		        mpc_thread_kthread_pthread_create(thread, &tmp_attr, tls_start_routine,
		                                          init_tls_start_routine_arg(def_start_routine, arg) );
		if(res != 0)
		{
			perror("mpc_thread_kthread_pthread_create: ");
			assume(res == 0);
		}
		pthread_attr_destroy(&tmp_attr);
		return res;
	}
	else
	{
		int res;

		if(attr == NULL)
		{
			pthread_attr_t tmp_attr;
			pthread_attr_init(&tmp_attr);
			attr = &tmp_attr;
		}


#ifdef PTHREAD_STACK_MIN
		if(PTHREAD_STACK_MIN > size)
		{
			res = pthread_attr_setstacksize(attr, PTHREAD_STACK_MIN);
		}
		else
		{
			res = pthread_attr_setstacksize(attr, size);
		}
#else
		res = pthread_attr_setstacksize(attr, size);
#endif

		if(res != 0)
		{
			perror("pthread_attr_setstacksize: ");
			assume(res == 0);
		}

		res = mpc_thread_kthread_pthread_create(thread, attr, tls_start_routine,
		                                        init_tls_start_routine_arg(def_start_routine,
		                                                                   arg) );

		if(res != 0)
		{
			perror("pthread_create: ");
			assume(res == 0);
		}
		return res;
	}
}

#define pthread_check_size(a, b)       mpc_common_debug_check_large_enough(sizeof(a), sizeof(b), MPC_STRING(a), MPC_STRING(b), __FILE__, __LINE__)
#define pthread_check_size_eq(a, b)    mpc_common_debug_check_size_equal(sizeof(a), sizeof(b), MPC_STRING(a), MPC_STRING(b), __FILE__, __LINE__)

static int _mpc_thread_pthread_mutex_spinlock(pthread_mutex_t *lock)
{
	return pthread_mutex_lock(lock);
}

static int _mpc_thread_pthread_engine_create(pthread_t *restrict thread,
                                             const pthread_attr_t *restrict attr,
                                             void *(*start_routine)(void *), void *restrict arg)
{
	if(attr == NULL)
	{
		pthread_attr_t tmp_attr;
		int            res;
		size_t         size;
		char *         env;
		pthread_attr_init(&tmp_attr);

		/* We bind the MPI tasks in a round robin manner */
		env = getenv("MPC_ENABLE_PTHREAD_PINNING");
		if(env != NULL)
		{
			sctk_thread_data_t *data = (sctk_thread_data_t *)arg;
			mpc_common_debug("Bind VP to core %d\n", data->mpi_task.local_rank % mpc_topology_get_pu_count() );
			mpc_topology_bind_to_cpu(data->mpi_task.local_rank);
		}

		if(mpc_common_get_flags()->is_fortran == 1)
		{
			size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
		}
		else
		{
			size = SCTK_ETHREAD_STACK_SIZE;
		}

		size += sctk_extls_size();

#ifdef PTHREAD_STACK_MIN
		if(PTHREAD_STACK_MIN > size)
		{
			res = pthread_attr_setstacksize(&tmp_attr, PTHREAD_STACK_MIN);
		}
		else
		{
			res = pthread_attr_setstacksize(&tmp_attr, size);
		}
#else
		res = pthread_attr_setstacksize(&tmp_attr, size);
#endif
		if(res != 0)
		{
			perror("pthread_attr_setstacksize: ");
			assume(res == 0);
		}

		res =
		        mpc_thread_kthread_pthread_create(thread, &tmp_attr, tls_start_routine,
		                                          init_tls_start_routine_arg(start_routine, arg) );
		if(res != 0)
		{
			perror("pthread_create: ");
			assume(res == 0);
		}
		pthread_attr_destroy(&tmp_attr);
		return res;
	}
	else
	{
		int res;
		res = mpc_thread_kthread_pthread_create(thread, attr, start_routine, arg);

		if(res != 0)
		{
			perror("pthread_create: ");
			assume(res == 0);
		}
		return res;
	}
}

static int _mpc_thread_pthread_engine_attr_setbinding(__UNUSED__ mpc_thread_attr_t *__attr, __UNUSED__ int __binding)
{
#if HAVE_PTHREAD_ATTR_SETAFFINITY_NP
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(__binding, &mask);
	pthread_attr_setaffinity_np( (pthread_attr_t *)__attr, sizeof(cpu_set_t), &mask);
#endif  /* HAVE_PTHREAD_ATTR_SETAFFINITY_NP */
	return 0;
}

static int _mpc_thread_pthread_engine_attr_getbinding(__UNUSED__ mpc_thread_attr_t *__attr, __UNUSED__ int *__binding)
{
	return 0;
}

#if SCTK_FUTEX_SUPPORTED

#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>

int _mpc_thread_pthread_futex(void *addr1, int op, int val1,
                              struct timespec *timeout, void *addr2, int val3)
{
	/* Here we call the actual futex implementation */
	return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

#else
int _mpc_thread_pthread_futex(__UNUSED__ void *addr1, __UNUSED__ int op, __UNUSED__ int val1,
                              __UNUSED__ struct timespec *timeout, __UNUSED__ void *addr2, __UNUSED__ int val3)
{
	not_implemented();
}
#endif

static void _mpc_thread_pthread_engine_at_fork_child()
{
	sem_post(&_mpc_thread_pthread_engine_user_create_sem);
}

static void _mpc_thread_pthread_engine_at_fork_parent()
{
	sem_post(&_mpc_thread_pthread_engine_user_create_sem);
}

static void _mpc_thread_pthread_engine_at_fork_prepare()
{
	sem_wait(&_mpc_thread_pthread_engine_user_create_sem);
}

int _mpc_thread_pthread_engine_proc_migration(__UNUSED__ const int cpu)
{
	return -1;
}

void mpc_thread_pthread_engine_init(void)

{
/*   pthread_mutex_t loc = PTHREAD_MUTEX_INITIALIZER; */
/*   mpc_thread_mutex_t glob = SCTK_THREAD_MUTEX_INITIALIZER; */
	mpc_common_debug_only_once();

	sem_init(&_mpc_thread_pthread_engine_user_create_sem, 0, 1);

	pthread_atfork(_mpc_thread_pthread_engine_at_fork_prepare, _mpc_thread_pthread_engine_at_fork_parent,
	               _mpc_thread_pthread_engine_at_fork_child);

	pthread_check_size(pthread_mutex_t, mpc_thread_mutex_t);
	pthread_check_size(pthread_mutexattr_t, mpc_thread_mutexattr_t);

	sctk_add_func(_mpc_thread_pthread_engine, attr_setbinding);
	sctk_add_func(_mpc_thread_pthread_engine, attr_getbinding);

	sctk_add_func_type(_mpc_thread_pthread, mutex_spinlock,
	                   int (*)(mpc_thread_mutex_t *) );

	sctk_add_func(_mpc_thread_pthread_engine, mutex_init);

	sctk_add_func(_mpc_thread_pthread_engine, wait_for_value_and_poll);
	sctk_add_func(_mpc_thread_pthread_engine, freeze_thread_on_vp);
	sctk_add_func(_mpc_thread_pthread_engine, wake_thread_on_vp);
	sctk_add_func(_mpc_thread_pthread_engine, proc_migration);

	sctk_add_func_type(_mpc_thread_pthread_engine, create, int (*)(mpc_thread_t *,
	                                                               const mpc_thread_attr_t
	                                                               *, void *(*)(void *),
	                                                               void *) );

	sctk_add_func_type(_mpc_thread_pthread_engine, user_create,
	                   int (*)(mpc_thread_t *, const mpc_thread_attr_t *,
	                           void *(*)(void *), void *) );

/*   assume (memcmp (&loc, &glob, sizeof (pthread_mutex_t)) == 0); */

/*   sctk_add_func (pthread, yield); */
	_funcptr_mpc_thread_yield = sched_yield;

	sctk_add_func_type(pthread, join, int (*)(mpc_thread_t, void **) );
	sctk_add_func_type(pthread, attr_init, int (*)(mpc_thread_attr_t *) );
	sctk_add_func_type(pthread, attr_destroy, int (*)(mpc_thread_attr_t *) );
	sctk_add_func_type(pthread, attr_setscope,
	                   int (*)(mpc_thread_attr_t *, int) );
	sctk_add_func_type(pthread, self, mpc_thread_t (*)(void) );
	sctk_add_func_type(pthread, mutex_lock, int (*)(mpc_thread_mutex_t *) );

	sctk_add_func_type(pthread, mutex_trylock, int (*)(mpc_thread_mutex_t *) );
	sctk_add_func_type(pthread, mutex_unlock, int (*)(mpc_thread_mutex_t *) );


	sctk_add_func_type(pthread, equal, int (*)(mpc_thread_t, mpc_thread_t) );
	sctk_add_func_type(pthread, exit, void (*)(void *) );

	sctk_add_func_type(_mpc_thread_pthread, futex, int (*)(void *, int, int, struct timespec *, void *, int) );


	/* Regular Pthread iface */

	sctk_add_func_type(pthread, setspecific, int (*)(mpc_thread_keys_t,
	                                                 const void *) );
	sctk_add_func_type(pthread, getspecific, void *(*)(mpc_thread_keys_t) );

	sctk_add_func_type(pthread, key_create, int (*)(mpc_thread_keys_t *,
	                                                void (*)(void *) ) );
	sctk_add_func_type(pthread, key_delete, int (*)(mpc_thread_keys_t) );


#ifdef HAVE_PTHREAD_KILL
	sctk_add_func_type(pthread, kill, int (*)(mpc_thread_t, int) );
#endif

#ifndef WINDOWS_SYS
	_funcptr_mpc_thread_sigpending = (int (*)(sigset_t *) )sigpending;
	_funcptr_mpc_thread_sigsuspend = (int (*)(sigset_t *) )sigsuspend;
#endif

#ifdef HAVE_PTHREAD_SIGMASK
	sctk_add_func_type(pthread, sigmask,
	                   int (*)(int, const sigset_t *, sigset_t *) );
#endif

	/*le pthread once */
#ifdef HAVE_PTHREAD_ONCE
	sctk_add_func_type(pthread, once,
	                   int (*)(mpc_thread_once_t *, void (*)(void) ) );
#endif
	/*les attributs des threads */
#ifdef HAVE_PTHREAD_ATTR_SETDETACHSTATE
	sctk_add_func_type(pthread, attr_setdetachstate,
	                   int (*)(mpc_thread_attr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETDETACHSTATE
	sctk_add_func_type(pthread, attr_getdetachstate,
	                   int (*)(const mpc_thread_attr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPOLICY
	sctk_add_func_type(pthread, attr_setschedpolicy,
	                   int (*)(mpc_thread_attr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSCHEDPOLICY
	sctk_add_func_type(pthread, attr_getschedpolicy,
	                   int (*)(const mpc_thread_attr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
	sctk_add_func_type(pthread, setaffinity_np,
	                  int (*)(mpc_thread_t , size_t ,const mpc_cpu_set_t *) );
#endif
#ifdef HAVE_PTHREAD_GETAFFINITY_NP
	sctk_add_func_type(pthread, getaffinity_np,
	                  int (*)(mpc_thread_t , size_t , mpc_cpu_set_t *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETINHERITSCHED
	sctk_add_func_type(pthread, attr_setinheritsched,
	                   int (*)(mpc_thread_attr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETINHERITSCHED
	sctk_add_func_type(pthread, attr_getinheritsched,
	                   int (*)(const mpc_thread_attr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSCOPE
	sctk_add_func_type(pthread, attr_getscope,
	                   int (*)(const mpc_thread_attr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSCOPE
	sctk_add_func_type(pthread, attr_setscope,
	                   int (*)(mpc_thread_attr_t *, int) );
#endif
#ifndef HAVE_PTHREAD_ATTR_GETSTACK
#ifdef HAVE_PTHREAD_ATTR_GETSTACKADDR
	sctk_add_func_type(pthread, attr_getstackaddr,
	                   int (*)(const mpc_thread_attr_t *, void **) );
#endif
#endif
#ifndef  HAVE_PTHREAD_ATTR_SETSTACK
#ifdef HAVE_PTHREAD_ATTR_SETSTACKADDR
	sctk_add_func_type(pthread, attr_setstackaddr,
	                   int (*)(mpc_thread_attr_t *, void *) );
#endif
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSTACKSIZE
	sctk_add_func_type(pthread, attr_getstacksize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
	sctk_add_func_type(pthread, attr_setstacksize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSTACK
	sctk_add_func_type(pthread, attr_getstack,
	                   int (*)(const mpc_thread_attr_t *, void **, size_t *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSTACK
	sctk_add_func_type(pthread, attr_setstack,
	                   int (*)(mpc_thread_attr_t *, void *, size_t) );
#endif
/*mutex attr*/
#ifdef HAVE_PTHREAD_MUTEXATTR_INIT
	sctk_add_func_type(pthread, mutexattr_init,
	                   int (*)(mpc_thread_mutexattr_t *) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_DESTROY
	sctk_add_func_type(pthread, mutexattr_destroy,
	                   int (*)(mpc_thread_mutexattr_t *) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_SETTYPE
	sctk_add_func_type(pthread, mutexattr_settype,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_GETTYPE
	sctk_add_func_type(pthread, mutexattr_gettype,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_SETPSHARED
	sctk_add_func_type(pthread, mutexattr_setpshared,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_GETPSHARED
	sctk_add_func_type(pthread, mutexattr_getpshared,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_DESTROY
	sctk_add_func_type(pthread, mutex_destroy, int (*)(mpc_thread_mutex_t *) );
	/*gestion du cancel */
#endif
#ifdef HAVE_PTHREAD_TESTCANCEL
	sctk_add_func_type(pthread, testcancel, void (*)(void) );
#endif
#ifdef HAVE_PTHREAD_CANCEL
	sctk_add_func_type(pthread, cancel, int (*)(mpc_thread_t) );
#endif
#ifdef HAVE_PTHREAD_SETCANCELSTATE
	sctk_add_func_type(pthread, setcancelstate, int (*)(int, int *) );
#endif
#ifdef HAVE_PTHREAD_SETCANCELTYPE
	sctk_add_func_type(pthread, setcanceltype, int (*)(int, int *) );
#endif

	/* pthread_detach */
#ifdef HAVE_PTHREAD_DETACH
	sctk_add_func_type(pthread, detach, int (*)(mpc_thread_t) );
#endif
	/*les s�maphores  */
#ifdef HAVE_SEM_INIT
	_funcptr_mpc_thread_sem_init = (int (*)(mpc_thread_sem_t *,
	                                        int, unsigned int) )sem_init;
#endif
#ifdef HAVE_SEM_WAIT
	_funcptr_mpc_thread_sem_wait = (int (*)(mpc_thread_sem_t *) )sem_wait;
#endif
#ifdef HAVE_SEM_TRYWAIT
	_funcptr_mpc_thread_sem_trywait = (int (*)(mpc_thread_sem_t *) )sem_trywait;
#endif
#ifdef HAVE_SEM_POST
	_funcptr_mpc_thread_sem_post = (int (*)(mpc_thread_sem_t *) )sem_post;
#endif
#ifdef HAVE_SEM_GETVALUE
	_funcptr_mpc_thread_sem_getvalue =
	        (int (*)(mpc_thread_sem_t *, int *) )sem_getvalue;
#endif
#ifdef HAVE_SEM_DESTROY
	_funcptr_mpc_thread_sem_destroy = (int (*)(mpc_thread_sem_t *) )sem_destroy;
#endif
#ifdef HAVE_SEM_OPEN
	_funcptr_mpc_thread_sem_open =
	        (mpc_thread_sem_t * (*)(const char *, int, ...) )sem_open;
#endif
#ifdef HAVE_SEM_CLOSE
	_funcptr_mpc_thread_sem_close = (int (*)(mpc_thread_sem_t *) )sem_close;
#endif
#ifdef HAVE_SEM_UNLINK
	_funcptr_mpc_thread_sem_unlink = (int (*)(const char *) )sem_unlink;
#endif

/*sched priority*/
	_funcptr_mpc_thread_sched_get_priority_max =
	        (int (*)(int) )sched_get_priority_max;
	_funcptr_mpc_thread_sched_get_priority_min =
	        (int (*)(int) )sched_get_priority_min;


	/*conditions */
#ifdef HAVE_PTHREAD_COND_INIT
	sctk_add_func_type(pthread, cond_init,
	                   int (*)(mpc_thread_cond_t *,
	                           const mpc_thread_condattr_t *) );
#endif
#ifdef HAVE_PTHREAD_COND_DESTROY
	sctk_add_func_type(pthread, cond_destroy, int (*)(mpc_thread_cond_t *) );
#endif
#ifdef HAVE_PTHREAD_COND_WAIT
	sctk_add_func_type(pthread, cond_wait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *) );
#endif
#ifdef HAVE_PTHREAD_COND_TIMEDWAIT
	sctk_add_func_type(pthread, cond_timedwait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *,
	                           const struct timespec *) );
#endif
#ifdef HAVE_PTHREAD_COND_CLOCKWAIT
	sctk_add_func_type(pthread, cond_clockwait,
	                   int (*)(mpc_thread_cond_t *, mpc_thread_mutex_t *,
	                           const struct timespec *) );
#endif
#ifdef HAVE_PTHREAD_COND_BROADCAST
	sctk_add_func_type(pthread, cond_broadcast, int (*)(mpc_thread_cond_t *) );
#endif
#ifdef HAVE_PTHREAD_COND_SIGNAL
	sctk_add_func_type(pthread, cond_signal, int (*)(mpc_thread_cond_t *) );
#endif
	/*attributs des conditions */
#ifdef HAVE_PTHREAD_CONDATTR_INIT
	sctk_add_func_type(pthread, condattr_init,
	                   int (*)(mpc_thread_condattr_t *) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_DESTROY
	sctk_add_func_type(pthread, condattr_destroy,
	                   int (*)(mpc_thread_condattr_t *) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_GETPSHARED
	sctk_add_func_type(pthread, condattr_getpshared,
	                   int (*)(const mpc_thread_condattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_SETPSHARED
	sctk_add_func_type(pthread, condattr_setpshared,
	                   int (*)(mpc_thread_condattr_t *, int) );
#endif

	/*spinlock */
#ifdef SCTK_USE_PTHREAD_SPINLOCK
#ifdef HAVE_PTHREAD_SPIN_INIT
	sctk_add_func_type(pthread, spin_init,
	                   int (*)(mpc_thread_spinlock_t *, int) );
#endif
#ifdef HAVE_PTHREAD_SPIN_LOCK
	sctk_add_func_type(pthread, spin_lock, int (*)(mpc_thread_spinlock_t *) );
#endif
#ifdef HAVE_PTHREAD_SPIN_TRYLOCK
	sctk_add_func_type(pthread, spin_trylock,
	                   int (*)(mpc_thread_spinlock_t *) );
#endif
#ifdef HAVE_PTHREAD_SPIN_UNLOCK
	sctk_add_func_type(pthread, spin_unlock,
	                   int (*)(mpc_thread_spinlock_t *) );
#endif
#ifdef HAVE_PTHREAD_SPIN_DESTROY
	sctk_add_func_type(pthread, spin_destroy,
	                   int (*)(mpc_thread_spinlock_t *) );
#endif
#endif  /*fin de SCTK_USE_PTHREAD_SPINLOCK */
	/*rwlock */
#ifdef SCTK_USE_PTHREAD_RWLOCK
#ifdef HAVE_PTHREAD_RWLOCK_INIT
	sctk_add_func_type(pthread, rwlock_init,
	                   int (*)(mpc_thread_rwlock_t *,
	                           const mpc_thread_rwlockattr_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_RDLOCK
	sctk_add_func_type(pthread, rwlock_rdlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_TRYRDLOCK
	sctk_add_func_type(pthread, rwlock_tryrdlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_TIMEDRDLOCK
	sctk_add_func_type(pthread, rwlock_timedrdlock,
	                   int (*)(mpc_thread_rwlock_t *,
	                           const struct timespec *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_CLOCKRDLOCK
	sctk_add_func_type(pthread, rwlock_clockrdlock,
	                   int (*)(mpc_thread_rwlock_t *,
	                           clockid_t,
	                           const struct timespec *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_WRLOCK
	sctk_add_func_type(pthread, rwlock_wrlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_TRYWRLOCK
	sctk_add_func_type(pthread, rwlock_trywrlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_TIMEDWRLOCK
	sctk_add_func_type(pthread, rwlock_timedwrlock,
	                   int (*)(mpc_thread_rwlock_t *,
	                           const struct timespec *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_CLOCKWRLOCK
	sctk_add_func_type(pthread, rwlock_timedwrlock,
	                   int (*)(mpc_thread_rwlock_t *,
	                           clockid_t,
	                           const struct timespec *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_UNLOCK
	sctk_add_func_type(pthread, rwlock_unlock,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCK_DESTROY
	sctk_add_func_type(pthread, rwlock_destroy,
	                   int (*)(mpc_thread_rwlock_t *) );
#endif
/*rwlock_attr*/
#ifdef HAVE_PTHREAD_RWLOCKATTR_INIT
	sctk_add_func_type(pthread, rwlockattr_init,
	                   int (*)(mpc_thread_rwlockattr_t *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCKATTR_SETPSHARED
	sctk_add_func_type(pthread, rwlockattr_setpshared,
	                   int (*)(mpc_thread_rwlockattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_RWLOCKATTR_GETPSHARED
	sctk_add_func_type(pthread, rwlockattr_getpshared,
	                   int (*)(const mpc_thread_rwlockattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_RWLOCKATTR_DESTROY
	sctk_add_func_type(pthread, rwlockattr_destroy,
	                   int (*)(mpc_thread_rwlockattr_t *) );
#endif
#endif  /*fin de  SCTK_USE_PTHREAD_RWLOCK */
	/*barrier */
#ifdef SCTK_USE_PTHREAD_BARRIER
#ifdef HAVE_PTHREAD_BARRIER_INIT
	sctk_add_func_type(pthread, barrier_init,
	                   int (*)(mpc_thread_barrier_t *,
	                           const mpc_thread_barrierattr_t *, unsigned) );
#endif
#ifdef HAVE_PTHREAD_BARRIER_WAIT
	sctk_add_func_type(pthread, barrier_wait,
	                   int (*)(mpc_thread_barrier_t *) );
#endif
#ifdef HAVE_PTHREAD_BARRIER_DESTROY
	sctk_add_func_type(pthread, barrier_destroy,
	                   int (*)(mpc_thread_barrier_t *) );
#endif

/*barrier_attr*/
#ifdef HAVE_PTHREAD_BARRIERATTR_INIT
	sctk_add_func_type(pthread, barrierattr_init,
	                   int (*)(mpc_thread_barrierattr_t *) );
#endif
#ifdef HAVE_PTHREAD_BARRIERATTR_SETPSHARED
	sctk_add_func_type(pthread, barrierattr_setpshared,
	                   int (*)(mpc_thread_barrierattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_BARRIERATTR_GETPSHARED
	sctk_add_func_type(pthread, barrierattr_getpshared,
	                   int (*)(const mpc_thread_barrierattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_BARRIERATTR_DESTROY
	sctk_add_func_type(pthread, barrierattr_destroy,
	                   int (*)(mpc_thread_barrierattr_t *) );
#endif
#endif  /*fin de USE_PTHREAD_BARRIER */
/*Non portable */
#ifdef HAVE_PTHREAD_GETATTR_NP
	sctk_add_func_type(pthread, getattr_np,
	                   int (*)(mpc_thread_t, mpc_thread_attr_t *) );
#endif


/*Non impl�ment� dans ethread(_mxn)*/
#ifdef HAVE_PTHREAD_ATTR_GETGUARDSIZE
	sctk_add_func_type(pthread, attr_getguardsize,
	                   int (*)(const mpc_thread_attr_t *, size_t *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETGUARDSIZE
	sctk_add_func_type(pthread, attr_setguardsize,
	                   int (*)(mpc_thread_attr_t *, size_t) );
#endif
#ifdef HAVE_PTHREAD_ATTR_GETSCHEDPARAM
	sctk_add_func_type(pthread, attr_getschedparam,
	                   int (*)(const mpc_thread_attr_t *,
	                           struct sched_param *) );
#endif
#ifdef HAVE_PTHREAD_ATTR_SETSCHEDPARAM
	sctk_add_func_type(pthread, attr_setschedparam,
	                   int (*)(mpc_thread_attr_t *,
	                           const struct sched_param *) );
#endif

#ifdef HAVE_PTHREAD_ATFORK
	sctk_add_func_type(pthread, atfork,
	                   int (*)(void (*prepare)(void),
	                           void (*parent)(void), void (*child)(void) ) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_GETCLOCK
	sctk_add_func_type(pthread, condattr_getclock,
	                   int (*)(mpc_thread_condattr_t *, clockid_t *) );
#endif
#ifdef HAVE_PTHREAD_CONDATTR_SETCLOCK
	sctk_add_func_type(pthread, condattr_setclock,
	                   int (*)(mpc_thread_condattr_t *, clockid_t) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_GETPRIOCEILING
	sctk_add_func_type(pthread, mutexattr_getprioceiling,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_SETPRIOCEILING
	sctk_add_func_type(pthread, mutexattr_setprioceiling,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_GETPROTOCOL
	sctk_add_func_type(pthread, mutexattr_getprotocol,
	                   int (*)(const mpc_thread_mutexattr_t *, int *) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_SETPROTOCOL
	sctk_add_func_type(pthread, mutexattr_setprotocol,
	                   int (*)(mpc_thread_mutexattr_t *, int) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_TIMEDLOCK
	sctk_add_func_type(pthread, mutex_timedlock,
	                   int (*)(mpc_thread_mutex_t *,
	                           const struct timespec *) );
#endif
#ifdef HAVE_PTHREAD_MUTEX_CLOCKLOCK
	sctk_add_func_type(pthread, mutex_clocklock,
	                   int (*)(mpc_thread_mutex_t *,
	                           clockid_t,
	                           const struct timespec *) );
#endif

#define sctk_add_func_type(newlib, func, t)    _funcptr_mpc_thread_ ## func = (t)newlib ## _ ## func

#ifdef HAVE_PTHREAD_GETCONCURRENCY
	sctk_add_func_type(pthread, getconcurrency, int (*)(void) );
#endif
#ifdef HAVE_PTHREAD_SETCONCURRENCY
	sctk_add_func_type(pthread, setconcurrency, int (*)(int) );
#endif
#ifdef HAVE_PTHREAD_SETSCHEDPRIO

/*
 * sctk_add_func_type (pthread,setschedprio,int (*)(mpc_thread_t , int ));
 */
#endif

	sctk_multithreading_initialised = 1;

	_mpc_thread_data_init();

	sctk_free(sctk_malloc(5) );

#ifdef MPC_Lowcomm
	sctk_set_net_migration_available(0);
#endif
}
