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

#include "sctk_thread_rwlock.h"
#include "sctk_thread_generic.h"


void sctk_thread_generic_rwlocks_init(){ 
  sctk_thread_generic_check_size (sctk_thread_generic_rwlock_t, sctk_thread_rwlock_t);
  sctk_thread_generic_check_size (sctk_thread_generic_rwlockattr_t, sctk_thread_rwlockattr_t);
  {
	static sctk_thread_generic_rwlock_t loc = SCTK_THREAD_GENERIC_RWLOCK_INIT;
	static sctk_thread_rwlock_t glob = SCTK_THREAD_RWLOCK_INITIALIZER;
	assume (memcmp (&loc, &glob, sizeof (sctk_thread_generic_rwlock_t)) == 0);
  }
}

void sctk_thread_generic_rwlocks_init_cell(
    sctk_thread_generic_rwlock_cell_t *cell) {
  cell->sched = NULL;
  cell->type = -1;
  cell->prev = NULL;
  cell->next = NULL;
}

int sctk_thread_generic_rwlocks_store_rwlock(
    sctk_thread_generic_rwlock_t *_rwlock,
    sctk_thread_generic_scheduler_t *sched) {

  /*  
	  ERRORS:
      ENOMEM Insufficient memory exists to initialize the structure used for
	  		 the hash table
	*/

  sctk_thread_rwlock_in_use_t* tmp = (sctk_thread_rwlock_in_use_t*) 
	  sctk_malloc( sizeof(sctk_thread_rwlock_in_use_t));

  if( tmp == NULL ) return SCTK_ENOMEM;

  tmp->rwlock = _rwlock;
  HASH_ADD(hh,(sched->th->attr.rwlocks_owned),rwlock,sizeof(void*),tmp);

  return 0;
}

int sctk_thread_generic_rwlocks_retrieve_rwlock(
    sctk_thread_generic_rwlock_t *_rwlock,
    sctk_thread_generic_scheduler_t *sched) {

  /*  
	  ERRORS:
      ENOMEM Insufficient memory exists to initialize the structure used for
	  		 the hash table
	*/

  sctk_thread_rwlock_in_use_t* tmp;

  HASH_FIND(hh,(sched->th->attr.rwlocks_owned),(&_rwlock),sizeof(void*),tmp);

  if( tmp == NULL ) return SCTK_EPERM;

  HASH_DELETE(hh,(sched->th->attr.rwlocks_owned),tmp);

  return 0;
}

static inline int 
__sctk_thread_generic_rwlocks_rwlockattr_destroy( sctk_thread_generic_rwlockattr_t* attr ){

  /*  
	  ERRORS:
      EINVAL The value specified for the argument is not correct
	*/
	  
  if( attr == NULL ) return SCTK_EINVAL;

  return 0;
}

static inline int 
__sctk_thread_generic_rwlocks_rwlockattr_getpshared( const sctk_thread_generic_rwlockattr_t* attr,
					int* val ){

  /*  
      ERRORS:
      EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL || val == NULL ) return SCTK_EINVAL;

  (*val) = attr->pshared;

  return 0;
}

static inline int
__sctk_thread_generic_rwlocks_rwlockattr_init( sctk_thread_generic_rwlockattr_t* attr ){

  /*
     ERRORS:
     EINVAL The value specified for the argument is not correct
     ENOMEM |> NOT IMPLEMENTED <| Insufficient memory exists to initialize 
            the read-write lock attributes object
   */

  if( attr == NULL ) return SCTK_EINVAL;

  attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;

  return 0;
}

static inline int
__sctk_thread_generic_rwlocks_rwlockattr_setpshared( sctk_thread_generic_rwlockattr_t* attr,
                    int val ){

  /*
     ERRORS:
     EINVAL The value specified for the argument is not correct
    */

  if( attr == NULL ) return SCTK_EINVAL;
  if( val != SCTK_THREAD_PROCESS_PRIVATE && val != SCTK_THREAD_PROCESS_SHARED ) return SCTK_EINVAL;

  int ret = 0;
  if( val == SCTK_THREAD_PROCESS_SHARED ){
	fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared rwlokcs\n");
	ret = SCTK_ENOTSUP;
  }

  attr->pshared = val;

  return ret;
}

static inline int
__sctk_thread_generic_rwlocks_rwlock_destroy( sctk_thread_generic_rwlock_t* lock ){

  /*
     ERRORS:
     EINVAL The value specified for the argument is not correct
     EBUSY  The specified lock is currently owned by a thread
    */

  if( lock == NULL ) return SCTK_EINVAL;
  if( lock->count > 0 ) return SCTK_EBUSY;

  lock->status = SCTK_DESTROYED;

  return 0;
}

static inline int
__sctk_thread_generic_rwlocks_rwlock_init( sctk_thread_generic_rwlock_t* lock,
                    const sctk_thread_generic_rwlockattr_t* attr,
                    sctk_thread_generic_scheduler_t* sched ){
 /*
 	 ERRORS:
     EINVAL The value specified for the argument is not correct
     EAGAIN |> NOT IMPLEMENTED <| The system lacked the necessary resources 
            (other than memory) to initialize another read-write lock
     ENOMEM |> NOT IMPLEMENTED <| Insufficient memory exists to initialize 
            the read-write lock
     EPERM  |> NOT IMPLEMENTED <| The caller does not have the privilege 
            to perform the operation
     EBUSY  The implementation has detected an attempt to reinitialize 
            the object referenced by lock, a previously initialized 
            but not yet destroyed read-write lock
   */

  if( lock == NULL ) return SCTK_EINVAL;
  if( lock->status == SCTK_INITIALIZED ) return SCTK_EBUSY;

  int ret = 0;
  sctk_thread_generic_rwlock_t local = SCTK_THREAD_GENERIC_RWLOCK_INIT;

  if( attr != NULL ){
	  if( attr->pshared == SCTK_THREAD_PROCESS_SHARED ){
		  fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared rwlokcs\n");
		  ret = SCTK_ENOTSUP;
	  }
  }

  local.status = SCTK_INITIALIZED;
  (*lock) = local;

  return ret;
}

static inline int
__sctk_thread_generic_rwlocks_rwlock_lock( sctk_thread_generic_rwlock_t* lock,
					unsigned int type,
                    sctk_thread_generic_scheduler_t* sched ){
  /*
     ERRORS:
     EINVAL  The value specified for the argument is not correct
     EDEADLK The current thread already owns the lock for writing
     EBUSY   The lock is not avaible for a tryrdlock or trywrlock type
     EAGAIN  |> NOT IMPLEMENTED <| The read lock could not be acquired 
             because the maximum number of read locks for rwlock has been exceeded
    */

  if( lock == NULL ) return SCTK_EINVAL;
  if( lock->status != SCTK_INITIALIZED ) return SCTK_EINVAL;

  sctk_thread_generic_rwlock_cell_t cell;
  sctk_thread_generic_rwlocks_init_cell( &cell );
  int ret;
  void** tmp = (void**) sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  unsigned int ltype = type;
  if( type != SCTK_RWLOCK_READ && type != SCTK_RWLOCK_WRITE ){
	  if( type == SCTK_RWLOCK_TRYREAD ){
		  if( sctk_spinlock_trylock ( &(lock->lock) )) return SCTK_EBUSY;
		  ltype = SCTK_RWLOCK_READ;
	  }
	  else if( type == SCTK_RWLOCK_TRYWRITE ){
		  if( sctk_spinlock_trylock ( &(lock->lock) )) return SCTK_EBUSY;
		  ltype = SCTK_RWLOCK_WRITE;
	  } else {
		  return SCTK_EINVAL;
	  }
  } else {
	  sctk_spinlock_lock ( &(lock->lock) );
  }
  lock->count++;

  sctk_nodebug( " rwlock : \nlock = %d\ncurrent = %d\nwait = %d\ntype : %d\n",
		  lock->lock, lock->current, lock->wait, type );

  if( ltype == SCTK_RWLOCK_READ ){
	  if( (lock->current == SCTK_RWLOCK_READ || lock->current == SCTK_RWLOCK_ALONE)
			  && lock->wait == SCTK_RWLOCK_NO_WR_WAITING ){
		  lock->current = SCTK_RWLOCK_READ;
		  lock->reads_count++;

		  /* We add the lock to a hash table owned by the current thread*/
		  if( (ret = sctk_thread_generic_rwlocks_store_rwlock( lock, sched )) != 0 ){
			lock->count--;
			sctk_spinlock_unlock ( &(lock->lock) );
			return ret;
		  }

		  sctk_spinlock_unlock ( &(lock->lock) );
		  return 0;
	  }
	  else if( sched == lock->writer ){
		  lock->count--;
		  sctk_spinlock_unlock ( &(lock->lock) );
		  return SCTK_EDEADLK;
	  }
	  /* We cannot take the lock for reading and we prepare to block */
  }
  else if( ltype == SCTK_RWLOCK_WRITE ){
	  if( lock->current == SCTK_RWLOCK_ALONE
			  && lock->wait == SCTK_RWLOCK_NO_WR_WAITING ){
		  lock->current = SCTK_RWLOCK_WRITE;
		  lock->writer = sched;
  
		  /* We add the lock to a hash table owned by the current thread*/
		  if( (ret = sctk_thread_generic_rwlocks_store_rwlock( lock, sched )) != 0 ){
			lock->count--;
			sctk_spinlock_unlock ( &(lock->lock) );
			return ret;
		  }

		  sctk_spinlock_unlock ( &(lock->lock) );
		  return 0;
	  }
	  else if( sched == lock->writer ){
		  lock->count--;
		  sctk_spinlock_unlock ( &(lock->lock) );
		  return SCTK_EDEADLK;
	  }
	  lock->wait = SCTK_RWLOCK_WR_WAITING;
	  /* We cannot take the lock for writing and we prepare to block */
  }
  else{
	  not_reachable();
  }

  /* We test if we were in a tryread or in a trywrite */
  if( type != SCTK_RWLOCK_READ && type != SCTK_RWLOCK_WRITE ){
	  lock->count--;
	  sctk_spinlock_unlock ( &(lock->lock) );
	  return SCTK_EBUSY;
  }

  /* We add the lock to a hash table owned by the current thread*/
  if( (ret = sctk_thread_generic_rwlocks_store_rwlock( lock, sched )) != 0 ){
	lock->count--;
	sctk_spinlock_unlock ( &(lock->lock) );
	return ret;
  }

  cell.sched = sched;
  cell.type = ltype;
  DL_APPEND( lock->waiting, &cell );
  sctk_nodebug ( "blocked on %p", lock );

  sctk_thread_generic_thread_status( sched, sctk_thread_generic_blocked );
  sctk_nodebug( "WAIT RWLOCK LOCK sleep %p", sched );
  tmp[sctk_thread_generic_rwlock] = (void*) lock;
  sctk_thread_generic_register_spinlock_unlock( sched, &(lock->lock) );
  sctk_thread_generic_sched_yield( sched );
  tmp[sctk_thread_generic_rwlock] = NULL;
 
  /* test cancel */
  sctk_thread_generic_check_signals( 0 );
  return 0;
}

static inline int
__sctk_thread_generic_rwlocks_rwlock_unlock( sctk_thread_generic_rwlock_t* lock,
                    sctk_thread_generic_scheduler_t* sched ){
  /*
     ERRORS:
     EINVAL The value specified for the argument is not correct
     EPERM The current thread does not hold a lock on the read-write lock
    */

  if( lock == NULL ) return SCTK_EINVAL;
  if( lock->status != SCTK_INITIALIZED ) return SCTK_EINVAL;

  int ret = sctk_thread_generic_rwlocks_retrieve_rwlock( lock, sched  );
  if( ret != 0 ) return ret;

  sctk_spinlock_lock( &(lock->lock) );
  lock->count--;
  if( lock->reads_count > 0 ) lock->reads_count--;

  sctk_thread_generic_rwlock_cell_t* cell;
  sctk_thread_generic_scheduler_t* lsched;
  if( lock->waiting != NULL && lock->reads_count == 0 ){
	  if( lock->waiting->type == SCTK_RWLOCK_WRITE ){
		  cell = lock->waiting;
		  DL_DELETE( lock->waiting, cell );
		  lsched = cell->sched;
		  lock->current = SCTK_RWLOCK_WRITE;
		  lock->writer = lsched;
		  if( lock->waiting == NULL ) lock->wait = SCTK_RWLOCK_NO_WR_WAITING;
		  if( lsched->status != sctk_thread_generic_running ){
			  sctk_nodebug( "ADD RWLOCK UNLOCK wake %p", lsched );
			  sctk_thread_generic_wake( lsched );
		  }
	  }
	  else if( lock->waiting->type == SCTK_RWLOCK_READ ){
		  lock->current = SCTK_RWLOCK_READ;
		  lock->writer = NULL;
		  while( lock->waiting != NULL && lock->waiting->type == SCTK_RWLOCK_READ ){
			  cell = lock->waiting;
			  DL_DELETE( lock->waiting, cell );
			  lsched = cell->sched;
			  if( lsched->status != sctk_thread_generic_running ){
				  sctk_nodebug( "ADD RWLOCK UNLOCK wake %p", lsched );
				  sctk_thread_generic_wake( lsched );
			  }
			  lock->reads_count++;
		  }
		  if( lock->waiting == NULL ) lock->wait = SCTK_RWLOCK_NO_WR_WAITING;
	  }
	  else{
		  not_reachable();
	  }
  }

  if( lock->count <= 0 ){
	  if( lock->count < 0 ) abort();
	  lock->wait = SCTK_RWLOCK_NO_WR_WAIT;
	  lock->current = SCTK_RWLOCK_ALONE;
	  lock->writer = NULL;
  }
  sctk_spinlock_unlock( &(lock->lock) );

  return 0;
}

static inline int
__sctk_thread_generic_rwlocks_rwlock_timedrdlock( sctk_thread_generic_rwlock_t* lock,
                    const struct timespec* restrict time,
                    sctk_thread_generic_scheduler_t* sched ){
  /*
     ERRORS:
     EINVAL    The value specified by argument lock does not refer to an initialized 
               read-write lock object, or the abs_timeout nanosecond value is less 
               than zero or greater than or equal to 1000 million
     ETIMEDOUT The lock could not be acquired in the time specified
     EAGAIN    |> NOT IMPLEMENTED <| The read lock could not be acquired 
               because the maximum number of read locks for lock would be exceeded
     EDEADLK   The calling thread already holds a write lock on argument lock
    */

  if( lock == NULL || time == NULL ) return SCTK_EINVAL;
  if( time->tv_nsec < 0 || time->tv_nsec >= 1000000000 ) return SCTK_EINVAL;

  int ret;
  struct timespec t_current;

  do{
	  ret = __sctk_thread_generic_rwlocks_rwlock_lock( lock, SCTK_RWLOCK_TRYREAD, sched );
	  if( ret != SCTK_EBUSY ){
		  return ret;
	  }
	  clock_gettime( CLOCK_REALTIME, &t_current );
  } while( ret != 0 && ( t_current.tv_sec < time->tv_sec ||
			  ( t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec ) ) );

  if( ret != 0 ){
	  return SCTK_ETIMEDOUT;
  }

  return 0;
}

static inline int
__sctk_thread_generic_rwlocks_rwlock_timedwrlock( sctk_thread_generic_rwlock_t* lock,
                    const struct timespec* restrict time,
                    sctk_thread_generic_scheduler_t* sched ){

  /*
     ERRORS:
     EINVAL    The value specified by argument lock does not refer to an initialized 
               read-write lock object, or the abs_timeout nanosecond value is less 
               than zero or greater than or equal to 1000 million
     ETIMEDOUT The lock could not be acquired in the specified time
     EDEADLK   The calling thread already holds the argument lock for writing
  */

  if( lock == NULL || time == NULL ) return SCTK_EINVAL;
  if( time->tv_nsec < 0 || time->tv_nsec >= 1000000000 ) return SCTK_EINVAL;

  int ret;
  struct timespec t_current;

  do{
	  ret = __sctk_thread_generic_rwlocks_rwlock_lock( lock, SCTK_RWLOCK_TRYWRITE, sched );
	  if( ret != SCTK_EBUSY ){
		  return ret;
	  }
	  clock_gettime( CLOCK_REALTIME, &t_current );
  } while( ret != 0 && ( t_current.tv_sec < time->tv_sec ||
			  ( t_current.tv_sec == time->tv_sec && t_current.tv_nsec < time->tv_nsec ) ) );

  if( ret != 0 ){
	  return SCTK_ETIMEDOUT;
  }

  return 0;
}

int
sctk_thread_generic_rwlocks_rwlockattr_destroy( sctk_thread_generic_rwlockattr_t* attr ){
  return __sctk_thread_generic_rwlocks_rwlockattr_destroy( attr );
}

int
sctk_thread_generic_rwlocks_rwlockattr_getpshared( const sctk_thread_generic_rwlockattr_t* attr,
                    int* val ){
  return __sctk_thread_generic_rwlocks_rwlockattr_getpshared( attr, val );
}

int
sctk_thread_generic_rwlocks_rwlockattr_init( sctk_thread_generic_rwlockattr_t* attr ){
  return __sctk_thread_generic_rwlocks_rwlockattr_init( attr );
}

int
sctk_thread_generic_rwlocks_rwlockattr_setpshared( sctk_thread_generic_rwlockattr_t* attr,
                    int val ){
  return __sctk_thread_generic_rwlocks_rwlockattr_setpshared( attr, val );
}

int
sctk_thread_generic_rwlocks_rwlock_destroy( sctk_thread_generic_rwlock_t* lock ){
  return __sctk_thread_generic_rwlocks_rwlock_destroy( lock );
}

int
sctk_thread_generic_rwlocks_rwlock_init( sctk_thread_generic_rwlock_t* lock,
                    const sctk_thread_generic_rwlockattr_t* attr,
                    sctk_thread_generic_scheduler_t* sched ){
  return __sctk_thread_generic_rwlocks_rwlock_init( lock, attr, sched );
}

int
sctk_thread_generic_rwlocks_rwlock_rdlock( sctk_thread_generic_rwlock_t* lock,
                    sctk_thread_generic_scheduler_t* sched ){
  return __sctk_thread_generic_rwlocks_rwlock_lock( lock, SCTK_RWLOCK_READ, sched );
}

int
sctk_thread_generic_rwlocks_rwlock_wrlock( sctk_thread_generic_rwlock_t* lock,
                    sctk_thread_generic_scheduler_t* sched ){
  return __sctk_thread_generic_rwlocks_rwlock_lock( lock, SCTK_RWLOCK_WRITE, sched );
}

int
sctk_thread_generic_rwlocks_rwlock_timedrdlock( sctk_thread_generic_rwlock_t* lock,
                    const struct timespec* restrict time,
                    sctk_thread_generic_scheduler_t* sched ){
  return __sctk_thread_generic_rwlocks_rwlock_timedrdlock( lock, time, sched );
}

int
sctk_thread_generic_rwlocks_rwlock_timedwrlock( sctk_thread_generic_rwlock_t* lock,
                    const struct timespec* restrict time,
                    sctk_thread_generic_scheduler_t* sched ){
  return __sctk_thread_generic_rwlocks_rwlock_timedwrlock( lock, time, sched );
}

int
sctk_thread_generic_rwlocks_rwlock_tryrdlock( sctk_thread_generic_rwlock_t* lock,
                    sctk_thread_generic_scheduler_t* sched ){
  return __sctk_thread_generic_rwlocks_rwlock_lock( lock, SCTK_RWLOCK_TRYREAD, sched );
}

int
sctk_thread_generic_rwlocks_rwlock_trywrlock( sctk_thread_generic_rwlock_t* lock,
                    sctk_thread_generic_scheduler_t* sched ){
  return __sctk_thread_generic_rwlocks_rwlock_lock( lock, SCTK_RWLOCK_TRYWRITE, sched );
}

int
sctk_thread_generic_rwlocks_rwlock_unlock( sctk_thread_generic_rwlock_t* lock,
                    sctk_thread_generic_scheduler_t* sched ){
  return __sctk_thread_generic_rwlocks_rwlock_unlock( lock, sched );
}

int
sctk_thread_generic_rwlocks_rwlockattr_getkind_np( sctk_thread_generic_rwlockattr_t * attr,
                    int* pref){
  not_implemented ();
  return 0;
}

int
sctk_thread_generic_rwlocks_rwlockattr_setkind_np( sctk_thread_generic_rwlockattr_t * attr,
                    int pref){
  not_implemented ();
  return 0;
}

