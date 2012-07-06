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
#include <fcntl.h>
#include <errno.h>
#include <sctk_kernel_thread.h>
#include <sctk_topology.h>
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
sctk_thread_generic_mutexattr_destroy( sctk_thread_mutexattr_t* attr ){
  return sctk_thread_generic_mutexes_mutexattr_destroy( (sctk_thread_generic_mutexattr_t*) attr );
}

static int
sctk_thread_generic_mutexattr_getpshared( sctk_thread_mutexattr_t* attr, int* pshared ){
  return sctk_thread_generic_mutexes_mutexattr_getpshared( (sctk_thread_generic_mutexattr_t*) attr,
		  				pshared );
}
/*
static int
sctk_thread_generic_mutexattr_getprioceiling( sctk_thread_mutexattr_t* attr,
				int* prioceiling ){
  return sctk_thread_generic_mutexes_mutexattr_getprioceiling((sctk_thread_generic_mutexattr_t *) attr,
		  				prioceiling );
}

static int
sctk_thread_generic_mutexattr_setprioceiling( sctk_thread_mutexattr_t* attr,
				int prioceiling ){
  return sctk_thread_generic_mutexes_mutexattr_setprioceiling((sctk_thread_generic_mutexattr_t *) attr,
		  				prioceiling );
}

static int
sctk_thread_generic_mutexattr_getprotocol( sctk_thread_mutexattr_t* attr,
				int* protocol ){
  return sctk_thread_generic_mutexes_mutexattr_getprotocol((sctk_thread_generic_mutexattr_t *) attr,
		  				protocol );
}

static int
sctk_thread_generic_mutexattr_setprotocol( sctk_thread_mutexattr_t* attr,
				int protocol ){
  return sctk_thread_generic_mutexes_mutexattr_setprotocol((sctk_thread_generic_mutexattr_t *) attr,
		  				protocol );
}
*/
static int
sctk_thread_generic_mutexattr_gettype( sctk_thread_mutexattr_t* attr, int* kind ){
  return sctk_thread_generic_mutexes_mutexattr_gettype( (sctk_thread_generic_mutexattr_t*) attr,
		  				kind );
}

static int
sctk_thread_generic_mutexattr_init( sctk_thread_mutexattr_t* attr ){
  return sctk_thread_generic_mutexes_mutexattr_init( (sctk_thread_generic_mutexattr_t*) attr );
}

static int
sctk_thread_generic_mutexattr_setpshared( sctk_thread_mutexattr_t* attr, int pshared ){
  return sctk_thread_generic_mutexes_mutexattr_setpshared( (sctk_thread_generic_mutexattr_t*) attr,
		  				pshared );
}

static int
sctk_thread_generic_mutexattr_settype( sctk_thread_mutexattr_t* attr, int kind ){
  return sctk_thread_generic_mutexes_mutexattr_settype( (sctk_thread_generic_mutexattr_t*) attr,
		  				kind );
}

static int
sctk_thread_generic_mutex_destroy( sctk_thread_mutex_t* lock )
{
  return sctk_thread_generic_mutexes_mutex_destroy( (sctk_thread_generic_mutex_t*) lock );
}

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
sctk_thread_generic_mutex_timedlock(sctk_thread_mutex_t* lock,
						const struct timespec* restrict time ){
  return sctk_thread_generic_mutexes_mutex_timedlock((sctk_thread_generic_mutex_t*)lock,
  						time, &(sctk_thread_generic_self()->sched));
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
sctk_thread_generic_condattr_destroy( sctk_thread_condattr_t* attr ){
  return sctk_thread_generic_conds_condattr_destroy( (sctk_thread_generic_condattr_t*) attr );
}

static int
sctk_thread_generic_condattr_getpshared( sctk_thread_condattr_t* attr,
				int* pshared ){
  return sctk_thread_generic_conds_condattr_getpshared( (sctk_thread_generic_condattr_t*) attr,
		  				pshared );
}

static int
sctk_thread_generic_condattr_init( sctk_thread_condattr_t* attr ){
  return sctk_thread_generic_conds_condattr_init( (sctk_thread_generic_condattr_t*) attr );
}

static int
sctk_thread_generic_condattr_setpshared( sctk_thread_condattr_t* attr,
				int pshared ){
  return sctk_thread_generic_conds_condattr_setpshared( (sctk_thread_generic_condattr_t*) attr,
		  				pshared );
}

static int
sctk_thread_generic_condattr_setclock( sctk_thread_condattr_t* attr,
				clockid_t clock_id ){
  return sctk_thread_generic_conds_condattr_setclock( (sctk_thread_generic_condattr_t*) attr,
		  				clock_id );
}

static int
sctk_thread_generic_condattr_getclock( sctk_thread_condattr_t* attr,
				clockid_t* clock_id ){
  return sctk_thread_generic_conds_condattr_getclock( (sctk_thread_generic_condattr_t*) attr,
		  				clock_id );
}

static int
sctk_thread_generic_cond_destroy( sctk_thread_cond_t * lock ){
  return sctk_thread_generic_conds_cond_destroy( (sctk_thread_generic_cond_t*)lock );
}

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
sctk_thread_generic_cond_timedwait (sctk_thread_cond_t* cond,
				sctk_thread_mutex_t* mutex,
				const struct timespec* restrict time ){
  return sctk_thread_generic_conds_cond_timedwait( (sctk_thread_generic_cond_t*) cond,
						(sctk_thread_generic_mutex_t*) mutex, time,
						&(sctk_thread_generic_self()->sched));
}

static int
sctk_thread_generic_cond_broadcast (sctk_thread_cond_t * lock)
{
  return sctk_thread_generic_conds_cond_broadcast((sctk_thread_generic_cond_t *)lock,
						&(sctk_thread_generic_self()->sched));
}

/***************************************/
/* READ/WRITE LOCKS                    */
/***************************************/

static int
sctk_thread_generic_rwlockattr_destroy( sctk_thread_rwlockattr_t* attr ){
  return sctk_thread_generic_rwlocks_rwlockattr_destroy( (sctk_thread_generic_rwlockattr_t*) attr );
}

static int
sctk_thread_generic_rwlockattr_getpshared( const sctk_thread_rwlockattr_t* attr, int* val ){
  return sctk_thread_generic_rwlocks_rwlockattr_getpshared( (sctk_thread_generic_rwlockattr_t*) attr,
		  				val );
}

static int
sctk_thread_generic_rwlockattr_init( sctk_thread_rwlockattr_t* attr ){
  return sctk_thread_generic_rwlocks_rwlockattr_init( (sctk_thread_generic_rwlockattr_t*) attr );
}

static int
sctk_thread_generic_rwlockattr_setpshared( sctk_thread_rwlockattr_t* attr, int val ){
  return sctk_thread_generic_rwlocks_rwlockattr_setpshared( (sctk_thread_generic_rwlockattr_t*) attr,
		  				val );
}

static int
sctk_thread_generic_rwlock_destroy( sctk_thread_rwlock_t* lock ){
  return sctk_thread_generic_rwlocks_rwlock_destroy( (sctk_thread_generic_rwlock_t*) lock );
}

static int
sctk_thread_generic_rwlock_init( sctk_thread_rwlock_t* lock, sctk_thread_rwlockattr_t* attr ){
  return sctk_thread_generic_rwlocks_rwlock_init( (sctk_thread_generic_rwlock_t*) lock,
		  				(sctk_thread_generic_rwlockattr_t*) attr, &(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_rdlock( sctk_thread_rwlock_t* lock ){
  return sctk_thread_generic_rwlocks_rwlock_rdlock( (sctk_thread_generic_rwlock_t*) lock,
		  				&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_wrlock( sctk_thread_rwlock_t* lock ){
  return sctk_thread_generic_rwlocks_rwlock_wrlock( (sctk_thread_generic_rwlock_t*) lock,
		  				&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_tryrdlock( sctk_thread_rwlock_t* lock ){
  return sctk_thread_generic_rwlocks_rwlock_tryrdlock( (sctk_thread_generic_rwlock_t*) lock,
		  				&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_trywrlock( sctk_thread_rwlock_t* lock ){
  return sctk_thread_generic_rwlocks_rwlock_trywrlock( (sctk_thread_generic_rwlock_t*) lock,
		  				&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_unlock( sctk_thread_rwlock_t* lock ){
  return sctk_thread_generic_rwlocks_rwlock_unlock( (sctk_thread_generic_rwlock_t*) lock,
		  				&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_timedrdlock( sctk_thread_rwlock_t* lock, const struct timespec* restrict time ){
  return sctk_thread_generic_rwlocks_rwlock_timedrdlock( (sctk_thread_generic_rwlock_t*) lock,
		  				time, &(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlock_timedwrlock( sctk_thread_rwlock_t* lock, const struct timespec* restrict time ){
  return sctk_thread_generic_rwlocks_rwlock_timedwrlock( (sctk_thread_generic_rwlock_t*) lock,
		  				time, &(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_rwlockattr_setkind_np( sctk_thread_rwlockattr_t* attr, int pref ){
  return sctk_thread_generic_rwlocks_rwlockattr_setkind_np( (sctk_thread_generic_rwlockattr_t*) attr,
		  				pref );
}

static int
sctk_thread_generic_rwlockattr_getkind_np( sctk_thread_rwlockattr_t* attr, int* pref ){
  return sctk_thread_generic_rwlocks_rwlockattr_getkind_np( (sctk_thread_generic_rwlockattr_t*) attr,
		  				pref );
}

/***************************************/
/* SEMAPHORES                          */
/***************************************/

static int
sctk_thread_generic_sem_init( sctk_thread_sem_t* sem, int pshared, unsigned int value ){
  return sctk_thread_generic_sems_sem_init( (sctk_thread_generic_sem_t*) sem,
						pshared, value );
}

static int
sctk_thread_generic_sem_wait( sctk_thread_sem_t* sem ){
  return sctk_thread_generic_sems_sem_wait( (sctk_thread_generic_sem_t*) sem,
		  				&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_sem_trywait( sctk_thread_sem_t* sem ){
  return sctk_thread_generic_sems_sem_trywait( (sctk_thread_generic_sem_t*) sem,
		  				&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_sem_timedwait( sctk_thread_sem_t* sem,
						const struct timespec* time ){
  return sctk_thread_generic_sems_sem_timedwait( (sctk_thread_generic_sem_t*) sem,
		  				time,
						&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_sem_post( sctk_thread_sem_t* sem ){
  return sctk_thread_generic_sems_sem_post( (sctk_thread_generic_sem_t*) sem,
		  				&(sctk_thread_generic_self()->sched) );
}

static int
sctk_thread_generic_sem_getvalue( sctk_thread_sem_t* sem, int* sval ){
  return sctk_thread_generic_sems_sem_getvalue( (sctk_thread_generic_sem_t*) sem,
		  				sval );
}

static int
sctk_thread_generic_sem_destroy( sctk_thread_sem_t* sem ){
  return sctk_thread_generic_sems_sem_destroy( (sctk_thread_generic_sem_t*) sem );
}

static sctk_thread_sem_t*
sctk_thread_generic_sem_open( const char* name, int oflag, ...){
  if( oflag & O_CREAT ){
  	va_list args;
	va_start( args, oflag );
	mode_t mode = va_arg( args, mode_t );
	unsigned int value = va_arg( args, int );
	va_end( args );
	return (sctk_thread_sem_t*) sctk_thread_generic_sems_sem_open( name, oflag, mode, value );
  } else {
	return (sctk_thread_sem_t*) sctk_thread_generic_sems_sem_open( name, oflag );
  }
}

static int
sctk_thread_generic_sem_close( sctk_thread_sem_t* sem ){
  return sctk_thread_generic_sems_sem_close( (sctk_thread_generic_sem_t*) sem );
}

static int
sctk_thread_generic_sem_unlink( const char* name ){
  return sctk_thread_generic_sems_sem_unlink( name );
}

/***************************************/
/* THREAD BARRIER                      */
/***************************************/

static int
sctk_thread_generic_barrierattr_destroy( sctk_thread_barrierattr_t* attr ){
  return sctk_thread_generic_barriers_barrierattr_destroy( (sctk_thread_generic_barrierattr_t*) attr );
}

static int
sctk_thread_generic_barrierattr_init( sctk_thread_barrierattr_t* attr ){
  return sctk_thread_generic_barriers_barrierattr_init( (sctk_thread_generic_barrierattr_t*) attr );
}

static int
sctk_thread_generic_barrierattr_getpshared( const sctk_thread_barrierattr_t* restrict attr,
					int* restrict pshared ){
  return sctk_thread_generic_barriers_barrierattr_getpshared( (sctk_thread_generic_barrierattr_t*) attr,
		  				pshared );
}

static int
sctk_thread_generic_barrierattr_setpshared( sctk_thread_barrierattr_t* attr,
					int pshared ){
  return sctk_thread_generic_barriers_barrierattr_setpshared( (sctk_thread_generic_barrierattr_t*) attr,
		  				pshared );
}

static int
sctk_thread_generic_barrier_destroy( sctk_thread_barrier_t* barrier ){
  return sctk_thread_generic_barriers_barrier_destroy( (sctk_thread_generic_barrier_t*) barrier );
}

static int
sctk_thread_generic_barrier_init( sctk_thread_barrier_t* restrict barrier,
					const sctk_thread_barrierattr_t* restrict attr, unsigned count ){
  return sctk_thread_generic_barriers_barrier_init( (sctk_thread_generic_barrier_t*) barrier,
		  				(sctk_thread_generic_barrierattr_t*) attr, count );
}

static int
sctk_thread_generic_barrier_wait( sctk_thread_barrier_t* barrier ){
  return sctk_thread_generic_barriers_barrier_wait( (sctk_thread_generic_barrier_t*) barrier,
		  				&(sctk_thread_generic_self()->sched) );
}

/***************************************/
/* THREAD SIGNALS                      */
/***************************************/

static sigset_t sctk_thread_default_set;
int errno;

static inline void
sctk_thread_generic_init_default_sigset(){
#ifndef WINDOWS_SYS
  sigset_t set;
  sigemptyset (&set);
  kthread_sigmask (SIG_SETMASK, &set, &sctk_thread_default_set);
  kthread_sigmask (SIG_SETMASK, &sctk_thread_default_set, NULL);
#endif
}

static inline void
sctk_thread_generic_attr_init_sigs( const sctk_thread_generic_attr_t* attr){
  int i;

  for( i=0; i<SCTK_NSIG; i++ ) attr->ptr->thread_sigpending[i] = 0;

  attr->ptr->thread_sigset = sctk_thread_default_set;
}

static inline void
sctk_thread_generic_treat_signals( sctk_thread_generic_t threadp ){
#ifndef WINDOWS_SYS
  sctk_thread_generic_p_t* th = threadp;
  int i, done = 0;

  sctk_spinlock_lock ( &(th->attr.spinlock ));

  for( i = 0; i < SCTK_NSIG; i++ ){
	if( expect_false( th->attr.thread_sigpending[i] != 0 )){
	  if( sigismember( (sigset_t*) &(th->attr.thread_sigset ), i + 1 ) == 0 ){
		th->attr.thread_sigpending[i] = 0;
		th->attr.nb_sig_pending--;
		done++;
		kthread_kill ( kthread_self(), i + 1);
	  }
	}
  }

  th->attr.nb_sig_treated += done;
  sctk_spinlock_unlock ( &(th->attr.spinlock ));
#endif
}

inline int
__sctk_thread_generic_sigpending( sctk_thread_generic_t threadp,
					sigset_t* set ){
  int i;
  sigemptyset (set);
  sctk_thread_generic_p_t* th = threadp;

  sctk_spinlock_lock ( &(th->attr.spinlock ));

  for( i = 0; i < SCTK_NSIG; i++ ){
	if( th->attr.thread_sigpending[i] != 0 ) 
		sigaddset ( set, i + 1 );
  }

  sctk_spinlock_unlock ( &(th->attr.spinlock ));
  return 0;
}

static int
sctk_thread_generic_sigpending( sigset_t * set){

  /*
	ERRORS:
	EFAULT |>NOT IMPLEMENTED<| 
	*/

#ifndef WINDOWS_SYS
  sctk_thread_generic_p_t* th = sctk_thread_generic_self();
  return __sctk_thread_generic_sigpending( th, set );
#endif
  return 0;
}

inline int
__sctk_thread_generic_sigmask( sctk_thread_generic_t threadp, int how, 
					const sigset_t* newmask, sigset_t* oldmask){
  int res = -1;
  if( how != SIG_BLOCK && how != SIG_UNBLOCK && how != SIG_SETMASK ){
	errno = SCTK_EINVAL;
	return res;
  }
  sctk_thread_generic_p_t* th = threadp;
  sigset_t set;

  kthread_sigmask( SIG_SETMASK, (sigset_t*) &(th->attr.thread_sigset ), &set );
  res = kthread_sigmask( how, newmask, oldmask);
  kthread_sigmask( SIG_SETMASK, &set, (sigset_t*) &(th->attr.thread_sigset ));

  sctk_thread_generic_check_signals( 1 );

  if( res > 0 ){
	errno = res;
	res = -1;
  }

  return res;
}

static int
sctk_thread_generic_sigmask( int how, const sigset_t* newmask, sigset_t* oldmask){

  /*
	ERRORS:
	EINVAL  The value specified in how was invalid
	*/
  int res = -1;
#ifndef WINDOWS_SYS
  sctk_thread_generic_p_t* th = sctk_thread_generic_self();
  res = __sctk_thread_generic_sigmask( th, how, newmask, oldmask );
#endif
  return res;
}

inline int
__sctk_thread_generic_sigsuspend( sctk_thread_generic_t threadp,
						const sigset_t* mask ){
  sigset_t oldmask;
  sigset_t pending;
  int i;
  sctk_thread_generic_p_t* th = threadp;

  th->attr.nb_sig_treated = 0;
  __sctk_thread_generic_sigmask( th, SIG_SETMASK, mask, &oldmask);

  while( th->attr.nb_sig_treated == 0 ){
	sctk_thread_generic_sched_yield( &(th->sched ));
	sigpending (&pending);

	for( i = 0; i < SCTK_NSIG; i++ ){
	  if( ( sigismember( (sigset_t *) &(th->attr.thread_sigset ), i + 1) == 1)
			  && ( sigismember( &pending, i + 1) == 1)){
		kthread_kill (kthread_self (), i + 1);
		th->attr.nb_sig_treated = 1;
	  }
	}
  }

  __sctk_thread_generic_sigmask( th, SIG_SETMASK, &oldmask, NULL );
  errno = SCTK_EINTR;
  return -1;
}

static int
sctk_thread_generic_sigsuspend( const sigset_t* mask ){

  /*
	ERRORS:
	EINTR  The call was interrupted by a signal
	EFAULT |>NOT IMPLEMENTED<| mask points to memory which is not a valid 
		   part of the process address space
	*/

#ifndef WINDOWS_SYS
  sctk_thread_generic_p_t* th = sctk_thread_generic_self();
  return __sctk_thread_generic_sigsuspend( th, mask );
#endif
  return -1;
}

/***************************************/
/* THREAD KILL                         */
/***************************************/

static int
sctk_thread_generic_kill( sctk_thread_generic_t threadp, int val ){

  /*
	ERRORS:
	ESRCHH No thread with the ID threadp could be found
	EINVAL An invalid signal was specified
	*/

  sctk_nodebug ("sctk_thread_generic_kill %p %d set", thread, val);

  if( val == 0 ) return 0;
  val--;
  if( val < 0 || val > SCTK_NSIG ) return SCTK_EINVAL;

  sctk_thread_generic_p_t* th = threadp;
  if( th->sched.status == sctk_thread_generic_joined 
		  || th->sched.status == sctk_thread_generic_zombie )
	  return SCTK_ESRCH;

  sctk_spinlock_lock ( &(th->attr.spinlock ));

  if( th->attr.thread_sigpending[val] == 0 ){
	th->attr.thread_sigpending[val] = 1;
	th->attr.nb_sig_pending++;
  }

  sctk_spinlock_unlock ( &(th->attr.spinlock ));

  return 0;
}

static int
sctk_thread_generic_kill_other_threads_np(){
  return 0;
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
  sctk_thread_generic_attr_init_sigs( attr );
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
	sctk_thread_generic_attr_init_sigs( attr );
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

  sctk_nodebug("Before yield %p",&(thread->sched));
  /*It is mandatory to have to yield for pthread mode*/
  sctk_thread_generic_sched_yield(&(thread->sched));
  sctk_thread_generic_sched_yield(&(thread->sched));

  sctk_nodebug("Start %p %p",&(thread->sched),thread->attr.arg);
  thread->attr.return_value = thread->attr.start_routine(thread->attr.arg);
  sctk_nodebug("End %p %p",&(thread->sched),thread->attr.arg);

  /* Handel Exit */
  if(thread->attr.scope == SCTK_THREAD_SCOPE_SYSTEM){
    sctk_swapcontext(&(thread->sched.ctx),&(thread->sched.ctx_bootstrap));
  } else {
    sctk_thread_generic_thread_status(&(thread->sched),sctk_thread_generic_zombie);
    sctk_thread_generic_sched_yield(&(thread->sched));
  }
  not_reachable();
}

static int
sctk_thread_generic_attr_setbinding (sctk_thread_generic_attr_t * attr, int binding)
{
  
  if(attr->ptr == NULL){
    sctk_thread_generic_attr_init(attr);
  }
  attr->ptr->bind_to = binding;
  return 0;
}

static int
sctk_thread_generic_attr_getbinding (sctk_thread_generic_attr_t * attr, int *binding)
{
  sctk_thread_generic_intern_attr_t init = sctk_thread_generic_intern_attr_init;
  sctk_thread_generic_intern_attr_t * ptr;

  ptr = attr->ptr;

  if(ptr == NULL){
    ptr = &init;
	sctk_thread_generic_attr_init_sigs( attr );
  }
  *binding = ptr->bind_to;
  return 0;
}

static int
sctk_thread_generic_attr_getstacksize (sctk_thread_generic_attr_t * attr,
				    size_t * stacksize)
{

  /*
	ERRORS:
	EINVAL The stacksize argument is not valide
	*/

  if( stacksize == NULL ) return SCTK_EINVAL;

  sctk_thread_generic_intern_attr_t init = sctk_thread_generic_intern_attr_init;
  sctk_thread_generic_intern_attr_t * ptr;

  ptr = attr->ptr;

  if(ptr == NULL){
    ptr = &init;
	sctk_thread_generic_attr_init_sigs( attr );
  }
  *stacksize = ptr->stack_size;
  return 0;
}
static int
sctk_thread_generic_attr_setstacksize (sctk_thread_generic_attr_t * attr,
				    size_t stacksize)
{

  /*
	ERRORS:
	EINVAL The stacksize value is less than PTHREAD_STACK_MIN
	*/

  if( stacksize < SCTK_THREAD_STACK_MIN ) return SCTK_EINVAL;

  if(attr->ptr == NULL){
    sctk_thread_generic_attr_init(attr);
  }
  attr->ptr->stack_size = stacksize;
  return 0;
}

static int
sctk_thread_generic_attr_getstackaddr (sctk_thread_generic_attr_t * attr,
				    void **addr)
{

  /*
	ERRORS:
	EINVAL The addr argument is not valide 
	*/

  if( addr == NULL ) return SCTK_EINVAL;

  sctk_thread_generic_intern_attr_t init = sctk_thread_generic_intern_attr_init;
  sctk_thread_generic_intern_attr_t * ptr;

  ptr = attr->ptr;

  if(ptr == NULL){
    ptr = &init;
	sctk_thread_generic_attr_init_sigs( attr );
  }

  *addr = ptr->stack;
  return 0;
}
static int
sctk_thread_generic_attr_setstackaddr (sctk_thread_generic_attr_t * attr, void *addr)
{

  /*
	ERRORS:
	EINVAL The addr argument is not valide 
	*/

  if( addr == NULL ) return SCTK_EINVAL;

  if(attr->ptr == NULL){
    sctk_thread_generic_attr_init(attr);
  }
  attr->ptr->stack = addr;
  attr->ptr->user_stack = attr->ptr->stack;
  return 0;
}

static int
sctk_thread_generic_attr_getstack( const sctk_thread_generic_attr_t* attr,
					void** stackaddr, size_t* stacksize ){

  /*
	ERRORS:
	EINVAL Argument is not valide 
	*/

  if( attr == NULL ||stackaddr == NULL 
		  || stacksize == NULL ) return SCTK_EINVAL;

  *stackaddr = (void *) attr->ptr->stack;
  *stacksize = attr->ptr->stack_size;

  return 0;
}

static int
sctk_thread_generic_attr_setstack( sctk_thread_generic_attr_t* attr,
					void* stackaddr, size_t stacksize ){

  /*
	ERRORS:
	EINVAL Argument is not valide or the stacksize value is less 
		   than PTHREAD_STACK_MIN
	*/

  if( stackaddr == NULL || stacksize < SCTK_THREAD_STACK_MIN )
	  return SCTK_EINVAL;

  if(attr->ptr == NULL){
    sctk_thread_generic_attr_init(attr);
  }

  attr->ptr->stack = stackaddr;
  attr->ptr->user_stack = attr->ptr->stack;
  attr->ptr->stack_size = stacksize;

  return 0;
}

static int
sctk_thread_generic_attr_getdetachstate( sctk_thread_generic_attr_t* attr,
					int* detachstate ){

  if(attr->ptr == NULL){
	sctk_thread_generic_attr_init(attr);
  }

  (*detachstate) = attr->ptr->detachstate;

  return 0;
}

static int
sctk_thread_generic_attr_setdetachstate( sctk_thread_generic_attr_t* attr,
					int detachstate ){

  if( detachstate != PTHREAD_CREATE_DETACHED && detachstate != PTHREAD_CREATE_JOINABLE )
	  return SCTK_EINVAL;

  if(attr->ptr == NULL){
	sctk_thread_generic_attr_init(attr);
  }

  attr->ptr->detachstate = detachstate;

  return 0;
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
  sctk_thread_generic_attr_t lattr;
  lattr.ptr = ptr;
  sctk_thread_generic_attr_init_sigs( &lattr );

  sctk_nodebug("Create %p",arg);

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
    sctk_nodebug("STACK %p STACK SIZE %lu",stack,stack_size);
    sctk_makecontext (&(thread_id->sched.ctx),
		      (void *) thread_id,
		      __sctk_start_routine, stack, stack_size);
  }

  thread_id->attr.nb_wait_for_join = 0;

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

  if(attr->ptr->bind_to == -1){
    attr->ptr->bind_to = sctk_get_init_vp (pos);
    pos++;
  }

  res = sctk_thread_generic_user_create(threadp,attr,start_routine,arg);

  sctk_thread_generic_attr_destroy(&tmp_attr);

  return res;
}

static int
sctk_thread_generic_sched_get_priority_min( int sched_policy ){

  /*
	ERRORS:
	EINVAL the value of the argument is not valide
	*/

  if( sched_policy != SCTK_SCHED_FIFO && sched_policy != SCTK_SCHED_RR
		  && sched_policy != SCTK_SCHED_OTHER )
			return SCTK_EINVAL;

  return 0;
}

static int
sctk_thread_generic_sched_get_priority_max( int sched_policy ){

  /*
	ERRORS:
	EINVAL the value of the argument is not valide
	*/

  if( sched_policy != SCTK_SCHED_FIFO && sched_policy != SCTK_SCHED_RR
		  && sched_policy != SCTK_SCHED_OTHER )
			return SCTK_EINVAL;

  return 0;
}

/***************************************/
/* THREAD CANCELATION                  */
/***************************************/

inline void
sctk_thread_generic_check_signals( int select ){

	sctk_thread_generic_scheduler_t* sched;
	sctk_thread_generic_p_t* current;

	/* Get the current thread */
	sched = &(sctk_thread_generic_self()->sched);
	current = sched->th;
	sctk_assert( current->sched == sched );

	if( expect_false( current->attr.nb_sig_pending > 0))
			sctk_thread_generic_treat_signals( current );

	if( expect_false( current->attr.cancel_status > 0 )){
		//fprintf(stderr, "cancel_status:%d,cancelstate:%d,canceltype:%d,select :%d sched:%p, vp_type:%d\n", current->attr.cancel_status,current->attr.cancel_state,current->attr.cancel_type, select, sched, sched->generic.vp_type);
		sctk_nodebug ("%p %d %d", current,
				( current->attr.cancel_state != SCTK_THREAD_CANCEL_DISABLE),
				(( current->attr.cancel_type != SCTK_THREAD_CANCEL_DEFERRED)
				 && select ));

		if(( current->attr.cancel_state != SCTK_THREAD_CANCEL_DISABLE )
				&& (( current->attr.cancel_type != SCTK_THREAD_CANCEL_DEFERRED )
					|| !select )){
			sctk_nodebug ("Exit Thread %p", current );
			current->attr.cancel_status = 0;
			current->attr.return_value = ((void*) SCTK_THREAD_CANCELED );
			sctk_thread_exit_cleanup ();
			sctk_nodebug ("thread %p key liberation", current);
			sctk_thread_generic_keys_key_delete_all( &(current->keys) );
			sctk_nodebug ("thread %p key liberation done", current);
			sctk_nodebug ("thread %p ends", current);

			sctk_thread_generic_thread_status(&(current->sched),sctk_thread_generic_zombie);
			sctk_thread_generic_sched_yield(&(current->sched));
		}
	}
}

static int
sctk_thread_generic_cancel( sctk_thread_generic_t threadp ){

  /*
	ERRORS:
	EINVAL Invalid value for the argument
	*/

  sctk_thread_generic_p_t* th = threadp;
  sctk_nodebug ("thread to cancel: %p\n", th);

  if( th == NULL ) return SCTK_EINVAL;
  if( th->sched.status == sctk_thread_generic_zombie
		  || th->sched.status == sctk_thread_generic_joined ) 
	  return SCTK_ESRCH;

  if( th->attr.cancel_state == PTHREAD_CANCEL_ENABLE ){
	th->attr.cancel_status = 1;
	sctk_nodebug ("thread %p canceled\n", th);
  }

  return 0;
}

static int
sctk_thread_generic_setcancelstate( int state, int* oldstate ){

  /*
	ERRORS:
	EINVAL Invalid value for state
	*/

  sctk_thread_generic_attr_t attr;
  attr.ptr = &(sctk_thread_generic_self()->attr);

  if( oldstate != NULL ) 
	  (*oldstate) = attr.ptr->cancel_state;

  if( state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE )
	  return SCTK_EINVAL;

  attr.ptr->cancel_state = state;

  return 0;
}

static int
sctk_thread_generic_setcanceltype( int type, int* oldtype ){

  /*
	ERRORS:
	EINVAL Invalid value for type
	*/

  sctk_nodebug ("%d %d\n", type, sctk_thread_generic_self()->cancel_type);
  sctk_thread_generic_attr_t attr;
  attr.ptr = &(sctk_thread_generic_self()->attr);

  if( oldtype != NULL ) 
	  (*oldtype) = attr.ptr->cancel_type;

  if( type != PTHREAD_CANCEL_DEFERRED 
		  && type != PTHREAD_CANCEL_ASYNCHRONOUS ) return SCTK_EINVAL;

  attr.ptr->cancel_type = type;

  return 0;
}

static void
sctk_thread_generic_testcancel(){
  sctk_thread_generic_check_signals( 0 );
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
  task.free_func = NULL;
  task.next = NULL;
  task.prev = NULL;

  sctk_thread_generic_add_task(&task); 

}

/***************************************/
/* THREAD JOIN                         */
/***************************************/

static int
sctk_thread_generic_join ( sctk_thread_generic_t threadp, void** val ){
  /*
   	ERRORS:
   	ESRCH  No  thread could be found corresponding to that specified by th.
   	EINVAL The th thread has been detached.
   	EINVAL Another thread is already waiting on termination of th.
   	EDEADLK The th argument refers to the calling thread.
   */

  sctk_thread_generic_p_t* th = threadp;
  sctk_thread_generic_scheduler_t* sched;
  sctk_thread_generic_p_t* current;
  sctk_thread_generic_thread_status_t* status;

  /* Get the current thread */
  sched = &(sctk_thread_generic_self()->sched);
  current = sched->th;
  sctk_assert( current->sched == sched );
  
  sctk_nodebug ("Join Thread %p", th );
  //printf("in join: vp = %p\n",th->sched.generic.vp);

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  if( th != current ){
	if( th->sched.status == sctk_thread_generic_joined ) return SCTK_ESRCH;
	if( th->attr.detachstate != 0 ) return SCTK_EINVAL;

	th->attr.nb_wait_for_join++;
	if (th->attr.nb_wait_for_join != 1) return SCTK_EINVAL;

	status = (sctk_thread_generic_thread_status_t*) &(th->sched.status);
	sctk_nodebug ("TO Join Thread %p", th);
	sctk_thread_generic_wait_for_value_and_poll( (volatile int *) status,
			sctk_thread_generic_zombie, NULL, NULL );
	sctk_nodebug ("Joined Thread %p", th);

	if( val != NULL ) *val = th->attr.return_value;
	th->sched.status = sctk_thread_generic_joined;

	/* TODO: handle zombies */
	//printf("In join: th %p\n",th);
	//sctk_thread_generic_handle_zombies( th->sched.generic.vp );

  }else{
    return SCTK_EDEADLK;
  }

  return 0;
}

/***************************************/
/* THREAD EXIT                         */
/***************************************/

static void
sctk_thread_generic_exit( void* retval ){

  sctk_thread_generic_scheduler_t* sched;
  sctk_thread_generic_p_t* current;

  /* test cancel */
  sctk_thread_generic_check_signals( 0 );

  sctk_nodebug ("Exit Thread %p", th );

  /* Get the current thread */
  sched = &(sctk_thread_generic_self()->sched);
  current = sched->th;
  sctk_assert( current->sched == sched );

  current->attr.return_value = retval;

  sctk_nodebug ("thread %p key liberation", current);
  /*key liberation */
  sctk_thread_generic_keys_key_delete_all( &(current->keys) );

  sctk_nodebug ("thread %p key liberation done", current);

  sctk_nodebug ("thread %p ends", current);

  sctk_thread_generic_thread_status(&(current->sched),sctk_thread_generic_zombie);
  sctk_thread_generic_sched_yield(&(current->sched));
  not_reachable ();
}

/***************************************/
/* THREAD EQUAL                        */
/***************************************/

static int
sctk_thread_generic_equal( sctk_thread_generic_t th1, sctk_thread_generic_t th2 ){
  return th1 == th2;
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

  /****** SIGNALS ******/
  sctk_thread_generic_init_default_sigset();

  /****** SCHEDULER ******/
  sctk_thread_generic_scheduler_init(thread_type,scheduler_type,vp_number);
  sctk_thread_generic_scheduler_init_thread(&(sctk_thread_generic_self()->sched),
					    sctk_thread_generic_self());

  /****** JOIN ******/
  sctk_add_func_type (sctk_thread_generic, join,
			  int (*)(sctk_thread_generic_t, void **));

  /****** EXIT ******/
  sctk_add_func_type (sctk_thread_generic, exit,
		  	  void (*)(int *));

  /****** EQUAL ******/
  sctk_add_func_type (sctk_thread_generic, equal,
		  	  int (*)(sctk_thread_generic_t, sctk_thread_generic_t ));
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
  sctk_add_func_type (sctk_thread_generic, mutexattr_destroy,
		      int (*)(sctk_thread_mutexattr_t *));
  sctk_add_func_type (sctk_thread_generic, mutexattr_setpshared,
		      int (*)(sctk_thread_mutexattr_t *, int));
  sctk_add_func_type (sctk_thread_generic, mutexattr_getpshared,
		      int (*)(sctk_thread_mutexattr_t *, int *));
  sctk_add_func_type (sctk_thread_generic, mutexattr_settype,
		      int (*)(sctk_thread_mutexattr_t *, int));
  sctk_add_func_type (sctk_thread_generic, mutexattr_gettype,
		      int (*)(sctk_thread_mutexattr_t *, int *));
  sctk_add_func_type (sctk_thread_generic, mutexattr_init,
		  	  int (*)(sctk_thread_mutexattr_t *));
  sctk_add_func_type (sctk_thread_generic, mutex_lock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_thread_generic, mutex_spinlock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_thread_generic, mutex_trylock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_thread_generic, mutex_timedlock,
		  	  int (*)(sctk_thread_mutex_t *, const struct timespec*));
  sctk_add_func_type (sctk_thread_generic, mutex_unlock,
		      int (*)(sctk_thread_mutex_t *));
  sctk_add_func_type (sctk_thread_generic, mutex_destroy,
		      int (*)(sctk_thread_mutex_t *));
  __sctk_ptr_thread_mutex_init = sctk_thread_generic_mutex_init;

  /****** COND ******/
  sctk_thread_generic_conds_init();
  __sctk_ptr_thread_cond_init = sctk_thread_generic_cond_init;
   sctk_add_func_type (sctk_thread_generic, condattr_destroy,
			  int (*)(sctk_thread_condattr_t*));
   sctk_add_func_type (sctk_thread_generic, condattr_getpshared,
			  int (*)(sctk_thread_condattr_t*, int*));
   sctk_add_func_type (sctk_thread_generic, condattr_init,
			  int (*)(sctk_thread_condattr_t*));
   sctk_add_func_type (sctk_thread_generic, condattr_setpshared,
			  int (*)(sctk_thread_condattr_t*, int));
   sctk_add_func_type (sctk_thread_generic, condattr_setclock,
			  int (*)(sctk_thread_condattr_t*, int));
   sctk_add_func_type (sctk_thread_generic, condattr_getclock,
			  int (*)(sctk_thread_condattr_t*, int*));
   sctk_add_func_type (sctk_thread_generic, cond_destroy,
		      int (*)(sctk_thread_cond_t*));
   /*sctk_add_func_type (sctk_thread_generic, cond_init,
		      int (*)(sctk_thread_cond_t*, sctk_thread_mutex_t*));*/
   sctk_add_func_type (sctk_thread_generic, cond_wait,
		      int (*)(sctk_thread_cond_t*,sctk_thread_mutex_t*));
   sctk_add_func_type (sctk_thread_generic, cond_signal,
		      int (*)(sctk_thread_cond_t*));
   sctk_add_func_type (sctk_thread_generic, cond_timedwait,
		      int (*)(sctk_thread_cond_t*, sctk_thread_mutex_t*, const struct timespec*));
   sctk_add_func_type (sctk_thread_generic, cond_broadcast,
		      int (*)(sctk_thread_cond_t*));

  /****** RWLOCK ******/
  sctk_thread_generic_rwlocks_init();
  __sctk_ptr_thread_rwlock_init = sctk_thread_generic_rwlock_init;
  sctk_add_func_type (sctk_thread_generic, rwlockattr_destroy,
		  	  int (*)( sctk_thread_rwlockattr_t* attr ) );
  sctk_add_func_type (sctk_thread_generic, rwlockattr_getpshared,
		  	  int (*)( sctk_thread_rwlockattr_t* attr, int* val ) );
  sctk_add_func_type (sctk_thread_generic, rwlockattr_init,
		  	  int (*)( sctk_thread_rwlockattr_t* attr ) );
  sctk_add_func_type (sctk_thread_generic, rwlockattr_setpshared,
		  	  int (*)( sctk_thread_rwlockattr_t* attr, int val ) );
  sctk_add_func_type (sctk_thread_generic, rwlock_destroy,
		  	  int (*)( sctk_thread_rwlock_t* lock ) );
  sctk_add_func_type (sctk_thread_generic, rwlock_rdlock,
		  	  int (*)( sctk_thread_rwlock_t* lock ) );
  sctk_add_func_type (sctk_thread_generic, rwlock_wrlock,
		  	  int (*)( sctk_thread_rwlock_t* lock ) );
  sctk_add_func_type (sctk_thread_generic, rwlock_tryrdlock,
		  	  int (*)( sctk_thread_rwlock_t* lock ) );
  sctk_add_func_type (sctk_thread_generic, rwlock_trywrlock,
		  	  int (*)( sctk_thread_rwlock_t* lock ) );
  sctk_add_func_type (sctk_thread_generic, rwlock_unlock,
		  	  int (*)( sctk_thread_rwlock_t* lock ) );
  sctk_add_func_type (sctk_thread_generic, rwlock_timedrdlock,
		  	  int (*)( sctk_thread_rwlock_t* lock, const struct timespec* time ) );
  sctk_add_func_type (sctk_thread_generic, rwlock_timedwrlock,
		  	  int (*)( sctk_thread_rwlock_t* lock, const struct timespec* time ) );
  /*sctk_add_func_type (sctk_thread_generic, rwlockattr_setkind_np,
		  	  int (*)( sctk_thread_rwlockattr_t* attr, int ));
  sctk_add_func_type (sctk_thread_generic, rwlockattr_getkind_np,
		  	  int (*)( sctk_thread_rwlockattr_t* attr, int* ));*/

  /****** SEMAPHORE ******/
  sctk_thread_generic_sems_init();
  __sctk_ptr_thread_sem_init = sctk_thread_generic_sem_init;
  sctk_add_func_type (sctk_thread_generic, sem_wait,
		  	  int (*)( sctk_thread_sem_t* ));
  sctk_add_func_type (sctk_thread_generic, sem_trywait,
		  	  int (*)( sctk_thread_sem_t* ));
  sctk_add_func_type (sctk_thread_generic, sem_timedwait,
		  	  int (*)( sctk_thread_sem_t*, const struct timespec* ));
  sctk_add_func_type (sctk_thread_generic, sem_post,
		  	  int (*)( sctk_thread_sem_t* ));
  sctk_add_func_type (sctk_thread_generic, sem_getvalue,
		  	  int (*)( sctk_thread_sem_t*, int* ));
  sctk_add_func_type (sctk_thread_generic, sem_destroy,
		  	  int (*)( sctk_thread_sem_t* ));
  sctk_add_func_type (sctk_thread_generic, sem_open,
		  	  sctk_thread_sem_t* (*)( char*, int, ...));
  sctk_add_func_type (sctk_thread_generic, sem_close,
		  	  int (*)( sctk_thread_sem_t* ));
  sctk_add_func_type (sctk_thread_generic, sem_unlink,
		  	  int (*)( char* ));

  /****** THREAD BARRIER *******/
  sctk_thread_generic_barriers_init();
  __sctk_ptr_thread_barrier_init = sctk_thread_generic_barrier_init;
  sctk_add_func_type (sctk_thread_generic, barrierattr_destroy,
		  	  int (*)( sctk_thread_barrierattr_t* ));
  sctk_add_func_type (sctk_thread_generic, barrierattr_init,
		  	  int (*)( sctk_thread_barrierattr_t* ));
  sctk_add_func_type (sctk_thread_generic, barrierattr_getpshared,
		  	  int (*)( const sctk_thread_barrierattr_t* restrict, int* restrict ));
  sctk_add_func_type (sctk_thread_generic, barrierattr_setpshared,
		  	  int (*)( sctk_thread_barrierattr_t*, int ));
  sctk_add_func_type (sctk_thread_generic, barrier_destroy,
		  	  int (*)( sctk_thread_barrier_t* ));
  sctk_add_func_type (sctk_thread_generic, barrier_init,
		  	  int (*)( sctk_thread_barrier_t* restrict, 
				  const sctk_thread_barrierattr_t* restrict, unsigned ));
  sctk_add_func_type (sctk_thread_generic, barrier_wait,
		  	  int (*)( sctk_thread_barrier_t* ));

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

  sctk_add_func_type (sctk_thread_generic, attr_getbinding,
		      int (*)(const sctk_thread_attr_t *, int *));
  sctk_add_func_type (sctk_thread_generic, attr_setbinding,
		      int (*)(sctk_thread_attr_t *, int));

  sctk_add_func_type (sctk_thread_generic, attr_getstacksize,
		      int (*)(const sctk_thread_attr_t *,size_t  *));
  sctk_add_func_type (sctk_thread_generic, attr_setstacksize,
		      int (*)(sctk_thread_attr_t *, size_t));

  sctk_add_func_type (sctk_thread_generic, attr_getstackaddr,
		      int (*)(const sctk_thread_attr_t *,  void **));
  sctk_add_func_type (sctk_thread_generic, attr_setstackaddr,
		      int (*)(sctk_thread_attr_t *,void * ));

  sctk_add_func_type (sctk_thread_generic, attr_setstack,
		      int (*)(sctk_thread_attr_t *, void*, size_t ));
  sctk_add_func_type (sctk_thread_generic, attr_getstack,
		      int (*)(const sctk_thread_attr_t *, void**, size_t* ));

  sctk_add_func_type (sctk_thread_generic, attr_setdetachstate,
		  	  int (*)(sctk_thread_attr_t *, int));
  sctk_add_func_type (sctk_thread_generic, attr_getdetachstate,
		  	  int (*)(const sctk_thread_attr_t *, int *));

  sctk_add_func_type (sctk_thread_generic, user_create,
		      int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
			      void *(*)(void *), void *));
  sctk_add_func_type (sctk_thread_generic, create,
		      int (*)(sctk_thread_t *, const sctk_thread_attr_t *,
			      void *(*)(void *), void *));

  sctk_add_func_type (sctk_thread_generic, sched_get_priority_min,
		 	  int (*)( int ));
  sctk_add_func_type (sctk_thread_generic, sched_get_priority_max,
		  	  int (*)( int ));

  /****** THREAD SIGNALS ******/
  sctk_add_func_type (sctk_thread_generic, sigsuspend,
		  	  int (*)( const sigset_t* ));
  sctk_add_func_type (sctk_thread_generic, sigpending,
		  	  int (*)( sigset_t* ));
  sctk_add_func_type (sctk_thread_generic, sigmask,
		  	  int (*)( int, const sigset_t*, sigset_t*));
  /****** THREAD CANCEL ******/
  sctk_add_func_type (sctk_thread_generic, cancel,
			  int (*)( sctk_thread_generic_t ));
  sctk_add_func_type (sctk_thread_generic, setcancelstate,
			  int (*)( int, int* ));
  sctk_add_func_type (sctk_thread_generic, setcanceltype,
			  int (*)( int, int* ));
  sctk_add_func_type (sctk_thread_generic, testcancel,
		  	  void (*)());

  /****** THREAD KILL ******/
  sctk_add_func_type (sctk_thread_generic, kill,
		  	  int (*)( sctk_thread_generic_t, int ));
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

/*
static int
sctk_thread_generic_barrier_wait( sctk_thread_barrier_t* barrier ){
  return sctk_thread_generic_barriers_barrier_wait( (sctk_thread_generic_barrier_t*) barrier );
}*/
