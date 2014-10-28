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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpcomp.h>
#include "mpcomp_internal.h"
#include <sctk_debug.h>

/*
   This file contains all synchronization functions related to OpenMP
 */

/* TODO move these locks (atomic and critical) in a per-task structure:
   - mpcomp_thread_info, (every instance sharing the same lock) 
   - Key
   - TLS if available
 */
TODO("OpenMP: Anonymous critical and global atomic are not per-task locks")
static sctk_spinlock_t global_atomic_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_thread_mutex_t global_critical_lock =
  SCTK_THREAD_MUTEX_INITIALIZER;

static sctk_spinlock_t global_allocate_named_lock = SCTK_SPINLOCK_INITIALIZER;

void
__mpcomp_atomic_begin ()
{
	sctk_nodebug( "__mpcomp_atomic_begin: entering..." ) ;
	
  sctk_spinlock_lock (&(global_atomic_lock));

	sctk_nodebug( "__mpcomp_atomic_begin: exiting..." ) ;
}

void
__mpcomp_atomic_end ()
{
	sctk_nodebug( "__mpcomp_atomic_end: entering..." ) ;

  sctk_spinlock_unlock (&(global_atomic_lock));

	sctk_nodebug( "__mpcomp_atomic_end: exiting..." ) ;
}


INFO("Wrong atomic/critical behavior in case of OpenMP oversubscribing")

void
__mpcomp_anonymous_critical_begin ()
{
  sctk_nodebug ("[%d] __mpcomp_anonymous_critical_begin: Before lock",
			((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );

  sctk_thread_mutex_lock (&global_critical_lock);

  sctk_nodebug ("[%d] __mpcomp_anonymous_critical_begin: After lock",
			((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );
}

void
__mpcomp_anonymous_critical_end ()
{
  sctk_nodebug ("[%d] __mpcomp_anonymous_critical_end: Before unlock",
			((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );

  sctk_thread_mutex_unlock (&global_critical_lock);

  sctk_nodebug ("[%d] __mpcomp_anonymous_critical_end: Before unlock",
			((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );
}

void
__mpcomp_named_critical_begin (void **l)
{
	sctk_assert( l ) ;

	if ( *l == NULL ) {
	  sctk_spinlock_lock( &(global_allocate_named_lock) ) ;
	  if ( *l == NULL ) {
	    sctk_thread_mutex_t * temp_l ;
		sctk_nodebug( "[%d] __mpcomp_named_critical_begin: should allocated lock",
				((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );


		temp_l = malloc( sizeof( sctk_thread_mutex_t ) ) ;
		sctk_assert( temp_l ) ;

		sctk_thread_mutex_init( temp_l, NULL ) ;

		*l = temp_l ;
		sctk_assert( *l ) ;
	  }
	  sctk_spinlock_unlock( &(global_allocate_named_lock) ) ;
	}

	sctk_nodebug ("[%d] __mpcomp_named_critical_begin: Before lock",
			((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );

	sctk_thread_mutex_lock ( (sctk_thread_mutex_t *)*l );

	sctk_nodebug ("[%d] __mpcomp_named_critical_begin: After lock",
			((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );
}

void
__mpcomp_named_critical_end (void **l)
{
	sctk_assert( l ) ;
	sctk_assert( *l ) ;

	sctk_nodebug ("[%d] __mpcomp_named_critical_end: Before unlock",
			((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );
	sctk_thread_mutex_unlock ((sctk_thread_mutex_t *)*l );

	sctk_nodebug ("[%d] __mpcomp_named_critical_end: After unlock",
			((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank );
}
