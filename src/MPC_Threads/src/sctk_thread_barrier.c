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

#include "sctk_thread_barrier.h"
#include <sctk_thread_generic.h>

#include <errno.h>

int
sctk_thread_generic_barriers_barrierattr_destroy( sctk_thread_generic_barrierattr_t* attr ){

  /*  
	ERRORS:
	EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL ) return EINVAL;

  return 0;
}

int
sctk_thread_generic_barriers_barrierattr_init( sctk_thread_generic_barrierattr_t* attr ){

  /*  
	ERRORS:
	EINVAL The value specified for the argument is not correct
	ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the barrier 
		   attributes object
	*/

  if( attr == NULL ) return EINVAL;

  attr->pshared = SCTK_THREAD_PROCESS_PRIVATE;

  return 0;
}

int
sctk_thread_generic_barriers_barrierattr_getpshared( const sctk_thread_generic_barrierattr_t* 
		            restrict attr, int* restrict pshared ){

  /*  
	ERRORS:
	EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL || pshared == NULL ) return EINVAL;

  (*pshared) = attr->pshared;

  return 0;
}

int
sctk_thread_generic_barriers_barrierattr_setpshared( sctk_thread_generic_barrierattr_t* attr,
		            int pshared ){

  /*  
	ERRORS:
	EINVAL The value specified for the argument is not correct
	*/

  if( attr == NULL ) return EINVAL;
  if( pshared != SCTK_THREAD_PROCESS_PRIVATE
		  && pshared != SCTK_THREAD_PROCESS_SHARED ) return EINVAL;

  int ret = 0;
  if( pshared == SCTK_THREAD_PROCESS_SHARED ){
	fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared barriers\n");
	ret = ENOTSUP;
  }
  attr->pshared = pshared;

  return ret;
}

int
sctk_thread_generic_barriers_barrier_destroy( sctk_thread_generic_barrier_t* barrier ){

  /*  
	ERRORS:
	EINVAL The value specified for the argument is not correct
	EBUSY  The implementation has detected an attempt to destroy a barrier while it is in use
	*/

  if( barrier == NULL ) return EINVAL;
  if( barrier->blocked != NULL ) return EBUSY;

  return 0;
}

int
sctk_thread_generic_barriers_barrier_init( sctk_thread_generic_barrier_t* restrict barrier,
		            const sctk_thread_generic_barrierattr_t* restrict attr, unsigned count ){

  /*  
	ERRORS:
	EINVAL The value specified for the argument is not correct or count is equal to zero
	EAGAIN |>NOT IMPLEMENTED<| The system lacks the necessary resources to initialize 
		   another barrier
	ENOMEM |>NOT IMPLEMENTED<| Insufficient memory exists to initialize the barrier
	EBUSY  |>NOT IMPLEMENTED<| The implementation has detected an attempt to destroy a 
		   barrier while it is in use
	*/

  if( barrier == NULL || count == 0 ) return EINVAL;

  sctk_thread_generic_barrier_t loc = SCTK_THREAD_GENERIC_BARRIER_INIT;
  sctk_thread_generic_barrier_t* ptr_loc = &loc;

  int ret = 0;
  if( attr != NULL ){
	if( attr->pshared == SCTK_THREAD_PROCESS_SHARED ){
	  fprintf (stderr, "Invalid pshared value in attr, MPC doesn't handle process shared barriers\n");
	  ret = ENOTSUP;
	}
  }
  ptr_loc->nb_current = count;
  ptr_loc->nb_max = count;
  (*barrier) = loc;

  return ret;
}

int
sctk_thread_generic_barriers_barrier_wait( sctk_thread_generic_barrier_t* barrier,
				sctk_thread_generic_scheduler_t* sched ){

  /*  
	ERRORS:
	EINVAL The value specified for the argument is not correct or count is equal to zero
	*/

  if( barrier == NULL ) return EINVAL;

  int ret = 0;
  sctk_thread_generic_barrier_cell_t cell;
  sctk_thread_generic_barrier_cell_t* list;
  void** tmp = sched->th->attr.sctk_thread_generic_pthread_blocking_lock_table;

  mpc_common_spinlock_lock ( &(barrier->lock) );
  barrier->nb_current--;
  if( barrier->nb_current > 0 ){
	cell.sched = sched;
	DL_APPEND( barrier->blocked, &cell );
	sctk_thread_generic_thread_status( sched, sctk_thread_generic_blocked );
	tmp[sctk_thread_generic_barrier] = (void*) barrier;
	sctk_nodebug ("blocked on %p", barrier);
	sctk_thread_generic_register_spinlock_unlock( sched, &(barrier->lock) );
	sctk_thread_generic_sched_yield( sched );
	tmp[sctk_thread_generic_barrier] = NULL;
  }
  else {
	list = barrier->blocked;
	while( list != NULL ){
	  DL_DELETE( barrier->blocked, list );
	  if( list->sched->status != sctk_thread_generic_running){
		sctk_nodebug ("Sched %p pass barrier %p", list->sched, barrier );
		sctk_thread_generic_wake( list->sched );
	  }
	  list = barrier->blocked;
	}
	barrier->nb_current = barrier->nb_max;
	ret = SCTK_THREAD_BARRIER_SERIAL_THREAD;
	mpc_common_spinlock_unlock( &(barrier->lock) );
  }

  return ret;
}

void
sctk_thread_generic_barriers_init(){
  sctk_thread_generic_check_size (sctk_thread_generic_barrier_t, sctk_thread_barrier_t);
  sctk_thread_generic_check_size (sctk_thread_generic_barrierattr_t, sctk_thread_barrierattr_t);

  {
	static sctk_thread_generic_barrier_t loc = SCTK_THREAD_GENERIC_BARRIER_INIT;
	static sctk_thread_barrier_t glob;
	assume (memcmp (&loc, &glob, sizeof (sctk_thread_generic_barrier_t)) == 0); 
  }
}

