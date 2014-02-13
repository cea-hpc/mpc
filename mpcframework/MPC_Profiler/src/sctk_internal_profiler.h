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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef SCTK_INTERNAL_PROFILER
#define SCTK_INTERNAL_PROFILER

#include <stdint.h>
#include "sctk_debug.h"
#include "sctk_profiler_array.h"
#include "sctk_profile_meta.h"
#include "sctk_asm.h"
#include "sctk_atomics.h"
#include "sctk_tls.h"

/* Profiler internal interface */

void sctk_internal_profiler_init();
void sctk_internal_profiler_render();
void sctk_internal_profiler_release();

struct sctk_profile_meta *sctk_internal_profiler_get_meta();

/* ****************** */

/* Profiler switch */

extern int sctk_profiler_internal_switch;
extern __thread void* mpc_profiler;

static inline int sctk_profiler_internal_enabled()
{
	return sctk_profiler_internal_switch;
}

static inline void sctk_profiler_internal_enable()
{
	sctk_profiler_internal_switch = 1;
}

static inline void sctk_profiler_internal_disable()
{
	sctk_profiler_internal_switch = 0;
}

/* ****************** */

/* MPI Init / MPI Finalize */
static inline void sctk_profiler_set_initialize_time()
{
	struct sctk_profiler_array* array = (struct sctk_profiler_array *)mpc_profiler;
  array->initialize_time = sctk_atomics_get_timestamp();
  sctk_profiler_internal_enable();
}

static inline void sctk_profiler_set_finalize_time()
{
	struct sctk_profiler_array* array = (struct sctk_profiler_array *)mpc_profiler;
	sctk_profiler_internal_disable();
  array->run_time = ( (double)sctk_atomics_get_timestamp() - (double)array->initialize_time);
}



/* ****************** */


/* Profiler hit */

static inline struct sctk_profiler_array * sctk_internal_profiler_get_tls_array()
{
	return (struct sctk_profiler_array *)mpc_profiler;
}


static inline void sctk_profiler_internal_hit( int key, uint64_t duration )
{
	struct sctk_profiler_array *array = sctk_internal_profiler_get_tls_array();

	if( sctk_profiler_internal_enabled() && array )
	{
		sctk_profiler_array_hit( array, key, duration );
	}
}

/* ****************** */

/* Macros */
#define SCTK_PROFIL_START_DECLARE(key) uint64_t ___sctk_profile_begin_ ## key = 0;
#define SCTK_PROFIL_START_INIT(key) ___sctk_profile_begin_ ## key = sctk_atomics_get_timestamp();
#define SCTK_PROFIL_START(key) uint64_t ___sctk_profile_begin_ ## key = sctk_atomics_get_timestamp();
#define SCTK_PROFIL_END(key) sctk_profiler_internal_hit( SCTK_PROFILE_ ## key , sctk_atomics_get_timestamp() - ___sctk_profile_begin_ ## key );
#define SCTK_PROFIL_END_WITH_VALUE(key, value) sctk_profiler_internal_hit( SCTK_PROFILE_ ## key , value );

#define SCTK_COUNTER_INC(key, value) if( sctk_profiler_array_get_type( SCTK_PROFILE_ ## key ) != SCTK_PROFILE_TIME_PROBE )\
									{\
									   sctk_profiler_internal_hit( SCTK_PROFILE_ ## key , value );\
									}\
									else\
									{\
									   sctk_error("Cannot increment a time probe at %s:%d", __FILE__, __LINE__);\
									   abort();\
									}

#define SCTK_COUNTER_DEC(key, value) if( sctk_profiler_array_get_type( SCTK_PROFILE_ ## key ) != SCTK_PROFILE_TIME_PROBE )\
									{\
									   sctk_profiler_internal_hit( SCTK_PROFILE_ ## key , -value );\
									}\
									else\
									{\
									   sctk_error("Cannot decrement a time probe at %s:%d", __FILE__, __LINE__);\
									   abort();\
									}
/* ****** */

#endif /* SCTK_INTERNAL_PROFILER */
