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
#include "sctk_thread_mutex.h"
#include "sctk_thread_cond.h"
#include "sctk_thread_scheduler.h"
#include "sctk_spinlock.h"
#include "sctk_thread_generic.h"

int
sctk_thread_generic_conds_condattr_destroy( sctk_thread_generic_condattr_t* attr ){

  /*
	ERRORS:
    EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL ) return SCTK_EINVAL;

  return 0;
}

int
sctk_thread_generic_conds_condattr_getpshared( sctk_thread_generic_condattr_t* attr,
                    int* pshared ){

  /*  
	ERRORS:
    EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL || pshared == NULL ) return SCTK_EINVAL;

  (*pshared) = ( (attr->attrs >> 2) & 1);

  return 0;
}

int
sctk_thread_generic_conds_condattr_init( sctk_thread_generic_condattr_t* attr ){

  /*
	ERRORS:
    EINVAL The value specified for the argument is not correct
	ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the condition 
		   variable attributes object
	*/

  if( attr == NULL ) return SCTK_EINVAL;

  attr->attrs &= ~( 1 << 2 );
  attr->attrs = ((attr->attrs & ~3) | (0 & 3));

  return 0;
}

int
sctk_thread_generic_conds_condattr_setpshared( sctk_thread_generic_condattr_t* attr,
					int pshared ){

  /*
	ERRORS:
    EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL ) return SCTK_EINVAL;
  if( pshared != SCTK_THREAD_PROCESS_PRIVATE && pshared != SCTK_THREAD_PROCESS_SHARED ) return SCTK_EINVAL;

  int ret = 0;
  if( pshared == SCTK_THREAD_PROCESS_SHARED ){
	attr->attrs |= (1<<2);
	fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared conds\n");
	ret = SCTK_ENOTSUP;
  } else {
	attr->attrs &= ~(1<<2);
  }

  return ret;
}

int
sctk_thread_generic_conds_condattr_setclock( sctk_thread_generic_condattr_t* attr,
                    clockid_t clock_id ){

  /*
	ERRORS:
    EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL ) return SCTK_EINVAL;
  if(  clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC
		&& clock_id != CLOCK_PROCESS_CPUTIME_ID && clock_id != CLOCK_THREAD_CPUTIME_ID ) return SCTK_EINVAL;
  if( clock_id == CLOCK_PROCESS_CPUTIME_ID || clock_id == CLOCK_THREAD_CPUTIME_ID ) return SCTK_EINVAL;

  attr->attrs = ((attr->attrs & ~3) | (clock_id & 3));

  return 0;
}

int
sctk_thread_generic_conds_condattr_getclock( sctk_thread_generic_condattr_t* restrict attr,
                    clockid_t* restrict clock_id ){

  /*
	ERRORS:
    EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL || clock_id == NULL ) return SCTK_EINVAL;

  (*clock_id) = (attr->attrs & 3 );

  return 0;
}

int
sctk_thread_generic_conds_cond_destroy( sctk_thread_generic_cond_t* lock ){

  /*  
	ERRORS:
    EINVAL The value specified for the argument is not correct
	EBUSY  The lock argument is currently used
	*/

  if( lock == NULL ) return SCTK_EINVAL;
  if( lock->blocked != NULL ) return SCTK_EBUSY;

  lock->clock_id = -1;

  return 0;
}

int
sctk_thread_generic_conds_cond_init (sctk_thread_generic_cond_t * lock,
					const  sctk_thread_generic_condattr_t* attr,
					sctk_thread_generic_scheduler_t* sched)
{

  /*
	ERRORS:
	EAGAIN |>NOT IMPLEMENTED<| The system lacked the necessary resources 
		   (other than memory) to initialize another condition variable
	ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize 
		   the condition variable
	EBUSY  |>NOT IMPLEMENTED<| The argument lock is already initialize 
		   and must be destroy before reinitializing it
	EINVAL The value specified for the argument is not correct
	*/

  if( lock == NULL ) return SCTK_EINVAL;

  int ret = 0;
  sctk_thread_generic_cond_t local = SCTK_THREAD_GENERIC_COND_INIT;
  sctk_thread_generic_cond_t* ptrl = &local;

  if( attr != NULL ){
	if( ((attr->attrs >> 2) & 1) == SCTK_THREAD_PROCESS_SHARED ){
	  fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared conds\n");
	  ret = SCTK_ENOTSUP;
	}
	ptrl->clock_id = ( attr->attrs & 3 );
  }

  (*lock) = local;

  return ret;
}

int sctk_thread_generic_conds_cond_wait (sctk_thread_generic_cond_t* restrict cond,
                                         sctk_thread_generic_mutex_t* restrict mutex,
                                         sctk_thread_generic_scheduler_t* sched)
{

  /*
	ERRORS:
	EINVAL    The value specified by argument cond, mutex, or time is invalid
			  or different mutexes were supplied for concurrent pthread_cond_wait() 
			  operations on the same condition variable
	EPERM     The mutex was not owned by the current thread
	*/

  if( cond == NULL || mutex == NULL ) return SCTK_EINVAL;

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  int ret = 0;
  sctk_thread_generic_cond_cell_t cell;
  void** tmp = (void**) sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;
  sctk_spinlock_lock(&(cond->lock));
  if( cond->blocked == NULL ){
	if( sched != mutex->owner ) return SCTK_EPERM;
	cell.binded = mutex;
  } else {
	if( cond->blocked->binded != mutex ) return SCTK_EINVAL;
	if( sched != mutex->owner ) return SCTK_EPERM;
	cell.binded = mutex;
  }
  sctk_thread_generic_mutexes_mutex_unlock(mutex,sched);
  cell.sched = sched;
  DL_APPEND(cond->blocked,&cell);
  tmp[sctk_thread_generic_cond] = (void*) cond;
    
  sctk_nodebug("WAIT on %p",sched);

  sctk_thread_generic_thread_status(sched,sctk_thread_generic_blocked);
  sctk_thread_generic_register_spinlock_unlock(sched,&(cond->lock));
  sctk_thread_generic_sched_yield(sched);

  tmp[sctk_thread_generic_cond] = NULL;
  sctk_thread_generic_mutexes_mutex_lock(mutex,sched);

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );
  return ret;
}


int sctk_thread_generic_conds_cond_signal (sctk_thread_generic_cond_t* cond,
                                         sctk_thread_generic_scheduler_t* sched)
{

  /*
	ERRORS:
	EINVAL The value specified for the argument is not correct
	*/

  if( cond == NULL ) return SCTK_EINVAL;

  sctk_thread_generic_cond_cell_t* task;
  sctk_spinlock_lock(&(cond->lock));
  task = cond->blocked; 
  if(task != NULL){
    DL_DELETE(cond->blocked,task);
    sctk_thread_generic_wake(task->sched); 
  }
  sctk_spinlock_unlock(&(cond->lock));
  return 0;
}

struct sctk_cond_timedwait_args_s{
  const struct timespec* restrict timedout;
  int* timeout;
  sctk_thread_generic_scheduler_t* sched;
  sctk_thread_generic_cond_t* restrict cond;
};

inline void
sctk_thread_conds_init_timedwait_args( struct sctk_cond_timedwait_args_s* arg,
		const struct timespec* restrict timedout,
		int* timeout,
		sctk_thread_generic_scheduler_t* sched,
		sctk_thread_generic_cond_t* restrict cond ){
  arg->timedout = timedout;
  arg->timeout = timeout;
  arg->sched = sched;
  arg->cond = cond;
}

inline void
sctk_thread_conds_init_timedwait_task( sctk_thread_generic_task_t* task,
		volatile int *data,
		int value,
		void (*func) (void *),
		void *arg ){
  task->is_blocking = 0;
  task->data = data;
  task->value = value;
  task->func = func;
  task->arg = arg;
  task->sched = NULL;
  task->free_func = sctk_free;
  task->prev = NULL;
  task->next = NULL;
}

void
sctk_conds_cond_timedwait_test_timeout( void* args ){
  struct sctk_cond_timedwait_args_s* arg = (struct sctk_cond_timedwait_args_s*) args;
  sctk_thread_generic_cond_cell_t* lcell = NULL;
  sctk_thread_generic_cond_cell_t* lcell_tmp = NULL ;
  struct timespec t_current;

  if( sctk_spinlock_trylock( &(arg->cond->lock )) == 0 ){
	clock_gettime( arg->cond->clock_id, &t_current );

	if( t_current.tv_sec > arg->timedout->tv_sec ||
			( t_current.tv_sec == arg->timedout->tv_sec && t_current.tv_nsec > arg->timedout->tv_nsec )){
	  DL_FOREACH_SAFE( arg->cond->blocked, lcell, lcell_tmp ){
		if(lcell->sched == arg->sched ){
		  *(arg->timeout) = 1;
		  DL_DELETE( arg->cond->blocked, lcell );
		  lcell->next = NULL;
		  sctk_thread_generic_wake( lcell->sched );
		}
	  }
	}
  sctk_spinlock_unlock( &(arg->cond->lock) );
  }
}

int
sctk_thread_generic_conds_cond_timedwait( sctk_thread_generic_cond_t* restrict cond,
                    sctk_thread_generic_mutex_t* restrict mutex,
                    const struct timespec* restrict time,
                    sctk_thread_generic_scheduler_t* sched ){

  /*
	ERRORS:
	EINVAL    The value specified by argument cond, mutex, or time is invalid
		      or different mutexes were supplied for concurrent pthread_cond_timedwait() or 
			  pthread_cond_wait() operations on the same condition variable
	EPERM     The mutex was not owned by the current thread
	ETIMEDOUT The time specified by time has passed
	ENOMEM    Lack of memory
	*/

  if( cond == NULL || mutex == NULL || time == NULL ) return SCTK_EINVAL;
  if( time->tv_nsec < 0 || time->tv_nsec >= 1000000000 ) return SCTK_EINVAL;
  if( sched != mutex->owner ) return SCTK_EPERM;

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  int timeout = 0;
  struct timespec t_current;
  sctk_thread_generic_thread_status_t* status;
  sctk_thread_generic_cond_cell_t cell;
  sctk_thread_generic_task_t* cond_timedwait_task;
  struct sctk_cond_timedwait_args_s* args;
  void** tmp = (void**) sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;

  cond_timedwait_task = (sctk_thread_generic_task_t*) sctk_malloc( sizeof( sctk_thread_generic_task_t ));
  if( cond_timedwait_task == NULL ) return SCTK_ENOMEM;
  args = (struct sctk_cond_timedwait_args_s*) sctk_malloc( sizeof( struct sctk_cond_timedwait_args_s ));
  if( args == NULL ) return SCTK_ENOMEM;

  sctk_spinlock_lock(&(cond->lock));
  if( cond->blocked == NULL ){
	if( sched != mutex->owner ) return SCTK_EPERM;
	cell.binded = mutex;
  }
  else {
	if( cond->blocked->binded != mutex ) return SCTK_EINVAL;
	if( sched != mutex->owner ) return SCTK_EPERM;
	cell.binded = mutex;
  }
  sctk_thread_generic_mutexes_mutex_unlock(mutex,sched);
  cell.sched = sched;
  DL_APPEND( cond->blocked, &cell );
  tmp[sctk_thread_generic_cond] = (void*) cond;

  sctk_nodebug("WAIT on %p",sched);

  status = (sctk_thread_generic_thread_status_t*) &(sched->status);

  sctk_thread_conds_init_timedwait_args( args,
		  time, &timeout, sched, cond );

  sctk_thread_conds_init_timedwait_task( cond_timedwait_task,
		  (volatile int*) status,
		  sctk_thread_generic_running,
		  sctk_conds_cond_timedwait_test_timeout,
		  (void*) args );

  clock_gettime( cond->clock_id, &t_current );
  if( t_current.tv_sec > time->tv_sec ||
		  ( t_current.tv_sec == time->tv_sec && t_current.tv_nsec > time->tv_nsec )){
	sctk_free( args );
	sctk_free( cond_timedwait_task );
	DL_DELETE( cond->blocked, &cell );
	tmp[sctk_thread_generic_cond] = NULL;
	sctk_thread_generic_mutexes_mutex_lock( mutex, sched );
	sctk_spinlock_unlock( &(cond->lock ));
	return SCTK_ETIMEDOUT;
  }

  sctk_thread_generic_thread_status(sched,sctk_thread_generic_blocked);
  sctk_thread_generic_register_spinlock_unlock(sched,&(cond->lock));
  sctk_thread_generic_add_task( cond_timedwait_task );
  sctk_thread_generic_sched_yield(sched);

  tmp[sctk_thread_generic_cond] = NULL;
  sctk_thread_generic_mutexes_mutex_lock(mutex,sched);

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );
  if( timeout ) return SCTK_ETIMEDOUT;

  return 0;
}

int sctk_thread_generic_conds_cond_broadcast (sctk_thread_generic_cond_t * cond,
					      sctk_thread_generic_scheduler_t* sched)
{
  /*
	ERRORS:
	EINVAL The value specified for the argument is not correct
	*/

  if( cond == NULL ) return SCTK_EINVAL;

  sctk_thread_generic_cond_cell_t* task;
  sctk_thread_generic_cond_cell_t* task_tmp;
  sctk_spinlock_lock(&(cond->lock));
  DL_FOREACH_SAFE(cond->blocked,task,task_tmp){
    DL_DELETE(cond->blocked,task);
    sctk_nodebug("ADD BCAST cond wake %p from %p",task->sched,sched);
    sctk_thread_generic_wake(task->sched);    
  }
  sctk_spinlock_unlock(&(cond->lock));
  return 0;
}



void sctk_thread_generic_conds_init(){ 
  sctk_thread_generic_check_size (sctk_thread_generic_cond_t, sctk_thread_cond_t);
  sctk_thread_generic_check_size (sctk_thread_generic_condattr_t, sctk_thread_condattr_t);

  {
    static sctk_thread_generic_cond_t loc = SCTK_THREAD_GENERIC_COND_INIT;
    static sctk_thread_cond_t glob = SCTK_THREAD_COND_INITIALIZER;
    assume (memcmp (&loc, &glob, sizeof (sctk_thread_generic_cond_t)) == 0);
  }
 
}
