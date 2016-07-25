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

#include "sctk_thread_mutex.h"
#include <sctk_thread_generic.h>

int
sctk_thread_generic_mutexes_mutexattr_destroy( sctk_thread_generic_mutexattr_t* attr ){

  /*  
	ERRORS:
	EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL ) return SCTK_EINVAL;

  return 0;
}

int
sctk_thread_generic_mutexes_mutexattr_getpshared( sctk_thread_generic_mutexattr_t* attr,
                    int* pshared ){
 /*  
	ERRORS:
    EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL || pshared == NULL ) return SCTK_EINVAL;

  (*pshared) = ( (attr->attrs >> 2) & 1);

  return 0;
}
/*
int
sctk_thread_generic_mutexes_mutexattr_getprioceiling(sctk_thread_generic_mutexattr_t* attr,
                    int* prioceiling ){

  return 0;
}

int
sctk_thread_generic_mutexes_mutexattr_setprioceiling(sctk_thread_generic_mutexattr_t* attr,
                    int prioceiling ){

  return 0;
}

int
sctk_thread_generic_mutexes_mutexattr_getprotocol(sctk_thread_generic_mutexattr_t* attr,
                    int* protocol ){

  return 0;
}

int
sctk_thread_generic_mutexes_mutexattr_setprotocol(sctk_thread_generic_mutexattr_t* attr,
                    int protocol ){

  return 0;
}
*/
int
sctk_thread_generic_mutexes_mutexattr_gettype( sctk_thread_generic_mutexattr_t* attr,
                    int* kind ){

 /*  
	ERRORS:
    EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL || kind == NULL ) return SCTK_EINVAL;

  (*kind) = (attr->attrs & 3 );

  return 0;
}

int
sctk_thread_generic_mutexes_mutexattr_init( sctk_thread_generic_mutexattr_t* attr ){

  /*
	ERRORS:
    EINVAL The value specified for the argument is not correct
    ENOMEM |> NOT IMPLEMENTED <| Insufficient memory exists to initialize 
           the read-write lock attributes object
	*/

  if( attr == NULL ) return SCTK_EINVAL;

  attr->attrs = ((attr->attrs & ~3) | (0 & 3));
  attr->attrs &= ~( 1 << 2 );

  return 0;
}

int
sctk_thread_generic_mutexes_mutexattr_setpshared( sctk_thread_generic_mutexattr_t* attr,
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
	fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared mutexes\n");
	ret = SCTK_ENOTSUP;
  } else {
	attr->attrs &= ~(1<<2);
  }

  return ret;
}

int
sctk_thread_generic_mutexes_mutexattr_settype( sctk_thread_generic_mutexattr_t* attr,
                    int kind ){

  /*
	ERRORS:
	EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL ) return SCTK_EINVAL;
  if( kind != PTHREAD_MUTEX_NORMAL && kind != PTHREAD_MUTEX_ERRORCHECK 
		  && kind != PTHREAD_MUTEX_RECURSIVE && kind != PTHREAD_MUTEX_DEFAULT ) return SCTK_EINVAL;

  attr->attrs = (attr->attrs & ~3) | (kind & 3);

  return 0;
}

int
sctk_thread_generic_mutexes_mutex_destroy( sctk_thread_generic_mutex_t* lock ){

  /*
	ERRORS:
	EINVAL The value specified for the argument is not correct
	EBUSY  The specified lock is currently owned by a thread or 
		   another thread is currently using the mutex in a cond
	*/

  if( lock == NULL ) return SCTK_EINVAL;
  if( lock->owner != NULL ) return SCTK_EBUSY;

  return 0;
}

int
sctk_thread_generic_mutexes_mutex_init (sctk_thread_generic_mutex_t * lock,
					const  sctk_thread_generic_mutexattr_t* attr,
					sctk_thread_generic_scheduler_t* sched){

  /*
	ERRORS:
	EINVAL The value specified for the argument is not correct
    EBUSY  |>NOT IMPLEMENTED<| The specified lock has already been
		   initialized
	EAGAIN |>NOT IMPLEMENTED<| The system lacked the necessary 
		   resources (other than memory) to initialize another mutex
	ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to 
		   initialize the mutex
	EPERM  |>NOT IMPLEMENTED<| The caller does not have the 
		   privilege to perform the operation
	*/

  if( lock == NULL ) return SCTK_EINVAL;

  int ret = 0;
  sctk_thread_generic_mutex_t local_lock = SCTK_THREAD_GENERIC_MUTEX_INIT;
  sctk_thread_generic_mutex_t* local_ptr = &local_lock;

  if(attr != NULL){
	if( ( (attr->attrs >> 2) & 1) == SCTK_THREAD_PROCESS_SHARED ){
	  fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared mutexes\n");
	  ret = SCTK_ENOTSUP;
	}
	local_ptr->type = ( attr->attrs & 3 );
  }

  *lock = local_lock;

  return ret;
}

int
sctk_thread_generic_mutexes_mutex_lock (sctk_thread_generic_mutex_t * lock,
					sctk_thread_generic_scheduler_t* sched){

  /*
	ERRORS:
	EINVAL  The value specified for the argument is not correct
	EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum 
		    number of recursive locks for mutex has been exceeded
	EDEADLK The current thread already owns the mutex
	*/

  if( lock == NULL ) return SCTK_EINVAL;

  int ret = 0;
  sctk_thread_generic_mutex_cell_t cell;
  void** tmp = (void**) sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;
  sctk_spinlock_lock(&(lock->lock));
  if(lock->owner == NULL){
    lock->owner = sched;
    if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE ) lock->nb_call ++;
    sctk_spinlock_unlock(&(lock->lock));
    //We can force sched_yield here to increase calls to the priority scheduler
    //sctk_thread_generic_sched_yield(sched);
    return ret;
  } else {
    if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE 
			&& lock->owner == sched){
      lock->nb_call ++;
      sctk_spinlock_unlock(&(lock->lock));
      //We can force sched_yield here to increase calls to the priority scheduler
      //sctk_thread_generic_sched_yield(sched);
      return ret;
    } 
    if (lock->type == SCTK_THREAD_MUTEX_ERRORCHECK 
			&& lock->owner == sched ){
	  ret = SCTK_EDEADLK;
	  sctk_spinlock_unlock(&(lock->lock));
	  return ret;
    }

    cell.sched = sched;
    DL_APPEND(lock->blocked,&cell);
	tmp[sctk_thread_generic_mutex] = (void*) lock;
    
    sctk_thread_generic_thread_status(sched,sctk_thread_generic_blocked);
    sctk_nodebug("WAIT MUTEX LOCK sleep %p",sched);
    sctk_thread_generic_register_spinlock_unlock(sched,&(lock->lock));
    sctk_thread_generic_sched_yield(sched);
	tmp[sctk_thread_generic_mutex] = NULL;
  }
  return ret;
}

int
sctk_thread_generic_mutexes_mutex_trylock (sctk_thread_generic_mutex_t * lock,
					   sctk_thread_generic_scheduler_t* sched){

  /*
	ERRORS:
	EINVAL  The value specified for the argument is not correct
	EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum 
		    number of recursive locks for mutex has been exceeded
	EBUSY   the mutex is already owned by another thread or the calling thread
	*/

  if( lock == NULL ) return SCTK_EINVAL;

  int ret = 0;
  if( /*sctk_spinlock_trylock(&(lock->lock)) == 0 */1){
	  sctk_spinlock_lock(&(lock->lock));
	if(lock->owner == NULL){
	  lock->owner = sched;
	  if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE ) lock->nb_call ++;
	  sctk_spinlock_unlock(&(lock->lock));
	  return ret;
	} else {
	  if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE
			&& lock->owner == sched ){
		lock->nb_call ++;
		sctk_spinlock_unlock(&(lock->lock));
		return ret;
	  } 
	  ret = SCTK_EBUSY;
	  sctk_spinlock_unlock(&(lock->lock));
	}
  } else {
	ret = SCTK_EBUSY;
  }

  return ret;
}

int
sctk_thread_generic_mutexes_mutex_timedlock (sctk_thread_generic_mutex_t* lock,
						const struct timespec* restrict time,
                        sctk_thread_generic_scheduler_t* sched){

  /*
	ERRORS:
	EINVAL    The value specified by argument lock does not refer to an initialized 
			  mutex object, or the abs_timeout nanosecond value is less than zero or 
			  greater than or equal to 1000 million
	ETIMEDOUT The lock could not be acquired in specified time
	EAGAIN    |> NOT IMPLEMENTED <| The read lock could not be acquired because the 
			  maximum number of recursive locks for mutex has been exceeded
	*/

  if( lock == NULL || time == NULL ) return SCTK_EINVAL;
  if( time->tv_nsec < 0 || time->tv_nsec >= 1000000000 ) return SCTK_EINVAL;

  int ret = 0;
  struct timespec t_current;

  do{
	if( sctk_spinlock_trylock( &(lock->lock)) == 0 ){
	  if(lock->owner == NULL){
		lock->owner = sched;
    	if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE ) lock->nb_call ++;
	  } else {
		if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE
				&& lock->owner == sched ){
			lock->nb_call ++;
		} 
		ret = SCTK_EBUSY;
	  }
	} else {
	  ret = SCTK_EBUSY;
	}
	sctk_spinlock_unlock(&(lock->lock));
	clock_gettime( CLOCK_REALTIME, &t_current );
  } while( ret != 0 && ( t_current.tv_sec < time->tv_sec ||
			  ( t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec ) ) );

  if( ret != 0 ){
	return SCTK_ETIMEDOUT;
  }

  return ret;
}

int
sctk_thread_generic_mutexes_mutex_spinlock (sctk_thread_generic_mutex_t * lock,
					    sctk_thread_generic_scheduler_t* sched){

  /*
	ERRORS:
	EINVAL  The value specified for the argument is not correct
	EAGAIN  |>NOT IMPLEMENTED<| The mutex could not be acquired because the maximum 
		    number of recursive locks for mutex has been exceeded
	EDEADLK The current thread already owns the mutex
	*/

  /* TODO: FIX BUG when called by "sctk_thread_lock_lock" (same issue with mutex
   unlock when called by "sctk_thread_lock_unlock") with sched null beacause calling
   functions only have one argument instead of two*/
  if( lock == NULL /*|| (lock->m_attrs & 1) != 1*/ ) return SCTK_EINVAL;

  int ret = 0;
  sctk_thread_generic_mutex_cell_t cell;

  sctk_spinlock_lock(&(lock->lock));
  if(lock->owner == NULL){
    lock->owner = sched;
    lock->nb_call = 1;
    sctk_spinlock_unlock(&(lock->lock));
    return ret;
  } else {
    if(lock->type == SCTK_THREAD_MUTEX_RECURSIVE){
      lock->nb_call ++;
      sctk_spinlock_unlock(&(lock->lock));
      return ret;
    } 
    if (lock->type == SCTK_THREAD_MUTEX_ERRORCHECK){
      if(lock->owner == sched){
	ret = SCTK_EDEADLK;
	sctk_spinlock_unlock(&(lock->lock));
	return ret;
      }
    }

    cell.sched = sched;
    DL_APPEND(lock->blocked,&cell);
    
    sctk_spinlock_unlock(&(lock->lock));
    do{
      sctk_thread_generic_sched_yield(sched);
    }while(lock->owner != sched);
  }
  return ret;
}

int
sctk_thread_generic_mutexes_mutex_unlock (sctk_thread_generic_mutex_t * lock,
					  sctk_thread_generic_scheduler_t* sched){

  /*
	ERRORS:
	EINVAL The value specified by argument lock does not refer to an initialized mutex
	EAGAIN |> NOT IMPLEMENTED <| The read lock could not be acquired because the 
		    maximum number of recursive locks for mutex has been exceeded
	EPERM  The current thread does not own the mutex  
	*/

  if( lock == NULL ) return SCTK_EINVAL;

  if (lock->owner != sched){
      return SCTK_EPERM;
    }
  if (lock->type == SCTK_THREAD_MUTEX_RECURSIVE){
      if (lock->nb_call > 1){
	  lock->nb_call --;
	  return 0;
	  }
    }

  sctk_spinlock_lock(&(lock->lock));
  {
    sctk_thread_generic_mutex_cell_t * head;
    head = lock->blocked;
    if(head == NULL){
      lock->owner = NULL;
      lock->nb_call = 0;
    } else {
      lock->owner = head->sched;
      lock->nb_call = 1;
      DL_DELETE(lock->blocked,head);
      if(head->sched->status != sctk_thread_generic_running){
	sctk_nodebug("ADD MUTEX UNLOCK wake %p",head->sched);
	sctk_thread_generic_wake(head->sched);
      }
    }
  }
  sctk_spinlock_unlock(&(lock->lock));
  return 0;
}

void sctk_thread_generic_mutexes_init(){ 
  sctk_thread_generic_check_size (sctk_thread_generic_mutex_t, sctk_thread_mutex_t);
  sctk_thread_generic_check_size (sctk_thread_generic_mutexattr_t, sctk_thread_mutexattr_t);

  {
    static sctk_thread_generic_mutex_t loc = SCTK_THREAD_GENERIC_MUTEX_INIT;
    static sctk_thread_mutex_t glob = SCTK_THREAD_MUTEX_INITIALIZER;
    assume (memcmp (&loc, &glob, sizeof (sctk_thread_generic_mutex_t)) == 0);
  }
 
}
