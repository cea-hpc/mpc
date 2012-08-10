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

#include <sctk_thread_spinlock.h>

void
sctk_thread_generic_spinlocks_init(){
  sctk_thread_generic_check_size( sctk_thread_spinlock_t, sctk_thread_generic_spinlock_t );

  //printf("TOTO %d - %d \n", sizeof(sctk_thread_spinlock_t), sizeof(sctk_thread_spinlock_t));
  /*static sctk_thread_spinlock_t loc = */
}

int
sctk_thread_generic_spinlocks_spin_destroy( sctk_thread_generic_spinlock_t* spinlock ){
  
  /*
	  ERRORS:
	  EBUSY  The implementation has detected an attempt to destroy a spin lock while 
	  		 it is in use
	  EINVAL The value of the spinlock argument is invalid
	*/

	printf("%d %d\n",sctk_spin_destroyed,spinlock->state);
  if( spinlock == NULL || spinlock->state != sctk_spin_initialized ) return SCTK_EINVAL;
  sctk_spinlock_t* p_lock = &(spinlock->lock);
  if( (*p_lock) != 0 ) return SCTK_EBUSY;

  spinlock->state = sctk_spin_destroyed;
  return 0;
}

int
sctk_thread_generic_spinlocks_spin_init( sctk_thread_generic_spinlock_t* spinlock,
					int pshared ){

  /*
	  ERRORS:
	  EBUSY  The implementation has detected an attempt to initialize a spin lock while 
	  		 it is in use
	  EINVAL The value of the spinlock argument is invalid
	  EAGAIN |>NOT IMPLEMENTED<| The system lacks the necessary resources to initialize 
	  		 another spin lock
	  ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the lock
	*/

  if( spinlock == NULL ) return SCTK_EINVAL;
  sctk_spinlock_t* p_lock = &(spinlock->lock);
  if( ( spinlock->state == sctk_spin_initialized || spinlock->state == sctk_spin_destroyed ) 
		  && ( (*p_lock) != 0 )) return SCTK_EBUSY;
  if( pshared == SCTK_THREAD_PROCESS_SHARED ){
	fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared spinlocks\n");
	return SCTK_ENOTSUP;
  }

  sctk_thread_generic_spinlock_t local = SCTK_THREAD_GENERIC_SPINLOCK_INIT;
  sctk_thread_generic_spinlock_t* p_local = &local;
  p_local->state = sctk_spin_initialized;
  (*spinlock) = local;
  printf("init:%d\n",spinlock->state);

  return 0;
}

int
sctk_thread_generic_spinlocks_spin_lock( sctk_thread_generic_spinlock_t* spinlock,
					sctk_thread_generic_scheduler_t* sched ){

  /*
	  ERRORS:
	  EINVAL  The value of the spinlock argument is invalid
	  EDEADLK The calling thread already holds the lock
	*/

  if( spinlock == NULL ) return SCTK_EINVAL;
  if( spinlock->owner == sched ) return SCTK_EDEADLK;

  long i = SCTK_SPIN_DELAY;
  sctk_spinlock_t* p_lock = &(spinlock->lock);
  while( sctk_spinlock_trylock( &(spinlock->lock) )){
	while( expect_true( (*p_lock) != 0 ) ){
	  i--;
	  if( expect_false( i <= 0 ) ){
		sctk_thread_generic_sched_yield( sched );
		i = SCTK_SPIN_DELAY;
	  }
	}
  }
  spinlock->owner = sched;
  printf("in lock:%d\n", spinlock->state);
  return 0;
}

int
sctk_thread_generic_spinlocks_spin_trylock( sctk_thread_generic_spinlock_t* spinlock,
					sctk_thread_generic_scheduler_t* sched ){

  /*
	  ERRORS:
	  EINVAL  The value of the spinlock argument is invalid
	  EDEADLK The calling thread already holds the lock
	  EBUSY   Another thread currently holds the lock
	*/

  if( spinlock == NULL ) return SCTK_EINVAL;
  if( spinlock->owner == sched ) return SCTK_EDEADLK;

  if( sctk_spinlock_trylock( &(spinlock->lock) ) != 0 )
	return SCTK_EBUSY;
  else{
	spinlock->owner = sched;
	return 0;
  }
}

int
sctk_thread_generic_spinlocks_spin_unlock( sctk_thread_generic_spinlock_t* spinlock,
					sctk_thread_generic_scheduler_t* sched ){
 
  /*
	  ERRORS:
	  EINVAL The value of the spinlock argument is invalid
	  EPERM  The calling thread does not hold the lock
	*/

  if( spinlock == NULL ) return SCTK_EINVAL;
  if( spinlock->owner != sched ) return SCTK_EPERM;

  spinlock->owner = NULL;
  sctk_spinlock_unlock( &(spinlock->lock) );
  printf("in unlock:%d\n", spinlock->state);
  return 0;
}

