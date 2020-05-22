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

#ifndef SCTK_PROFILER
#define SCTK_PROFILER

#include <mpc_config.h>

#ifdef MPC_Profiler


#include <mpc_arch.h>
#include <mpc_common_types.h>
#include <mpc_common_spinlock.h>

/*****************************
 * PROFILER ARRAY DEFINITION *
 *****************************/


#undef SIZE_COUNTER
#undef COUNTER
#undef PROBE

#define SIZE_COUNTER PROBE
#define COUNTER PROBE

#define PROBE(key, parent, desc) \
	SCTK_PROFILE_##key,

typedef enum
{
	SCTK_PROFILE_NO_PARENT = 0,
#include "mpc_common_profiler_keys.h"
	SCTK_PROFILE_KEY_COUNT
} sctk_profile_key;

#undef SIZE_COUNTER
#undef COUNTER
#undef PROBE


typedef enum
{
	SCTK_PROFILE_TIME_PROBE,
	SCTK_PROFILE_COUNTER_SIZE_PROBE,
	SCTK_PROFILE_COUNTER_PROBE
} sctk_profile_key_type;

typedef enum
{
	SCTK_PROFILE_RENDER_WALK_DFS = 0,
	SCTK_PROFILE_RENDER_WALK_BFS = 1,
	SCTK_PROFILE_RENDER_WALK_BOTH = 2,
	SCTK_PROFILE_RENDER_WALK_COUNT
} sctk_profile_render_walk_mode;


struct sctk_profiler_array
{
	uint64_t sctk_profile_hits[ SCTK_PROFILE_KEY_COUNT ];
	uint64_t sctk_profile_value[ SCTK_PROFILE_KEY_COUNT ];
	uint64_t sctk_profile_max[ SCTK_PROFILE_KEY_COUNT ];
	uint64_t sctk_profile_min[ SCTK_PROFILE_KEY_COUNT ];

	mpc_common_spinlock_t lock;

	/* Timer for start (MPI_Init) and end (MPI_Finalize) */
	uint64_t initialize_time;
	uint64_t run_time;

	uint64_t been_unified;
};


extern sctk_profile_key_type sctk_profile_type[SCTK_PROFILE_KEY_COUNT];

static inline sctk_profile_key_type sctk_profiler_array_get_type( int id )
{
	return sctk_profile_type[id];
}


static inline void sctk_profiler_array_hit( struct sctk_profiler_array *array, int id, int64_t value )
{
	mpc_common_spinlock_lock( &array->lock );
	{
		array->sctk_profile_hits[ id ]++;

		if ( ( value + array->sctk_profile_value[ id ] ) < 0 )
		{
			array->sctk_profile_value[ id ] = 0;
		}
		else
		{
			array->sctk_profile_value[ id ] += value;
		}

		if ( ( array->sctk_profile_min[ id ] == 0 ) || ( value < array->sctk_profile_min[ id ] ) )
		{
			array->sctk_profile_min[ id ] = value;
		}

		if ( ( array->sctk_profile_max[ id ] == 0 ) || ( array->sctk_profile_max[ id ] < value ) )
		{
			array->sctk_profile_max[ id ] = value;
		}
	}
	mpc_common_spinlock_unlock( &array->lock );
}

/********************
 * INIT AND RELEASE *
 ********************/

/* Profiler internal interface */

void sctk_internal_profiler_init();
void sctk_internal_profiler_render();
void sctk_internal_profiler_release();

/*******************
 * PROFILER SWITCH *
 *******************/

extern int sctk_profiler_internal_switch;

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

/******************
 * PROFILER ARRAY *
 ******************/

extern __thread void *mpc_profiler;

static inline struct sctk_profiler_array *sctk_internal_profiler_get_tls_array()
{
	return ( struct sctk_profiler_array * )mpc_profiler;
}


struct sctk_profiler_array * sctk_profiler_get_reduce_array();


static inline void sctk_profiler_internal_hit( int key, uint64_t duration )
{
	struct sctk_profiler_array *array = sctk_internal_profiler_get_tls_array();

	if ( sctk_profiler_internal_enabled() && array )
	{
		sctk_profiler_array_hit( array, key, duration );
	}
}

/**********
 * MACROS *
 **********/

#define SCTK_PROFIL_START_DECLARE(key) uint64_t ___sctk_profile_begin_ ## key = 0;
#define SCTK_PROFIL_START_INIT(key) ___sctk_profile_begin_ ## key = mpc_arch_get_timestamp();


#define SCTK_PROFIL_START(key) uint64_t ___sctk_profile_begin_ ## key = mpc_arch_get_timestamp();
#define SCTK_PROFIL_END(key) sctk_profiler_internal_hit( SCTK_PROFILE_ ## key , mpc_arch_get_timestamp() - ___sctk_profile_begin_ ## key );


#define SCTK_PROFIL_END_WITH_VALUE(key, value) sctk_profiler_internal_hit( SCTK_PROFILE_ ## key , value );

#define SCTK_COUNTER_INC(key, value) if( sctk_profiler_array_get_type( SCTK_PROFILE_ ## key ) != SCTK_PROFILE_TIME_PROBE )\
	{\
		sctk_profiler_internal_hit( SCTK_PROFILE_ ## key , value );\
	}\
	else\
	{\
		mpc_common_debug_error("Cannot increment a time probe at %s:%d", __FILE__, __LINE__);\
		abort();\
	}

#define SCTK_COUNTER_DEC(key, value) if( sctk_profiler_array_get_type( SCTK_PROFILE_ ## key ) != SCTK_PROFILE_TIME_PROBE )\
	{\
		sctk_profiler_internal_hit( SCTK_PROFILE_ ## key , -value );\
	}\
	else\
	{\
		mpc_common_debug_error("Cannot decrement a time probe at %s:%d", __FILE__, __LINE__);\
		abort();\
	}

#else

#define SCTK_PROFIL_START(key)	(void)(0)
#define SCTK_PROFIL_END(key) (void)(0)
#define SCTK_COUNTER_INC(key,val) (void)(0);
#define SCTK_COUNTER_DEC(key,val) (void)(0);
#define SCTK_PROFIL_END_WITH_VALUE(key, value) (void)(0);

#if 0
#define sctk_internal_profiler_init() (void)(0)
#define sctk_internal_profiler_render() (void)(0)
#define sctk_internal_profiler_release() (void)(0)
#endif

#endif



#endif /* SCTK_PROFILER */
