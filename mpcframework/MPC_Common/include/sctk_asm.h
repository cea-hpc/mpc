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
#ifndef __SCTK_ASM_H_
#define __SCTK_ASM_H_

#include "sctk_config.h"
#include "sctk_atomics.h"

#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#ifdef __MIC__
#include <immintrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define sctk_get_time_stamp sctk_atomics_get_timestamp
/* #define sctk_get_time_stamp_gettimeofday  sctk_atomics_get_timestamp */

#define sctk_max( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define sctk_min( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )

double sctk_get_time_stamp_gettimeofday();

#include <libpause.h>

/***********************************
 * MPC TEST AND SET IMPLEMENTATION *
 ***********************************/

typedef volatile int sctk_atomic_test_t;

#ifdef SCTK_OPENPA_AVAILABLE

static inline int sctk_test_and_set( sctk_atomic_test_t *atomic )
{
	return sctk_atomics_swap_int( (OPA_int_t *) atomic, 1 );
}

#else /* SCTK_OPENPA_AVAILABLE */

/* test_and_set implementation. Fallback to mutex if
 * openPA not available for the current architecture*/

static pthread_mutex_t sctk_atomics_default_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline int sctk_test_and_set( sctk_atomic_test_t *atomic )
{
	int res;
	pthread_mutex_lock( &sctk_atomics_default_mutex );
	res = *atomic;
	if ( *atomic == 0 )
	{
		*atomic = 1;
	}
	pthread_mutex_unlock( &sctk_atomics_default_mutex );
	return res;
}

#endif /* SCTK_OPENPA_AVAILABLE */


#ifdef __cplusplus
}
#endif
#endif
