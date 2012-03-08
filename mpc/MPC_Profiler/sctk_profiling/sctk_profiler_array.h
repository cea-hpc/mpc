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
#ifndef SCTK_PROFILER_ARRAY
#define SCTK_PROFILER_ARRAY

#include "stdint.h"

#include "sctk_spinlock.h"

#undef PROBE
#define PROBE( key, parent, desc ) SCTK_PROFILE_ ## key,


typedef enum
{
	SCTK_PROFILE_NO_PARENT = 0,
	#include "sctk_profiler_keys.h"
	SCTK_PROFILE_KEY_COUNT
}sctk_profile_key;




struct sctk_profiler_array
{
	uint64_t sctk_profile_hits[ SCTK_PROFILE_KEY_COUNT ];
	uint64_t sctk_profile_time[ SCTK_PROFILE_KEY_COUNT ];
	uint64_t sctk_profile_max[ SCTK_PROFILE_KEY_COUNT ];
	uint64_t sctk_profile_min[ SCTK_PROFILE_KEY_COUNT ];
	sctk_spinlock_t lock;
	
	uint64_t been_unified;
};


extern uint64_t sctk_profile_has_child[ SCTK_PROFILE_KEY_COUNT ];
extern uint64_t sctk_profile_parent_key[ SCTK_PROFILE_KEY_COUNT ];
void sctk_profiler_array_init_parent_keys();

struct sctk_profiler_array * sctk_profiler_array_new();

void sctk_profiler_array_init(struct sctk_profiler_array *array);
void sctk_profiler_array_release(struct sctk_profiler_array *array);
void sctk_profiler_array_unify( struct sctk_profiler_array *array );
void sctk_profiler_array_walk( struct sctk_profiler_array *array, void (*handler)( struct sctk_profiler_array *array, int id, int parent_id, int depth, void *arg ), void *arg , int DFS );

static inline void sctk_profiler_array_hit( struct sctk_profiler_array *array, int id, uint64_t duration )
{
	sctk_spinlock_lock( &array->lock );
	{

		array->sctk_profile_hits[ id ]++;
		array->sctk_profile_time[ id ] += duration;
		
		if( (array->sctk_profile_min[ id ] == 0) || (duration < array->sctk_profile_min[ id ]) )
		{
			array->sctk_profile_min[ id ] = duration;
		}
		
		if( (array->sctk_profile_max[ id ] == 0) || (array->sctk_profile_max[ id ] < duration) )
		{
			array->sctk_profile_max[ id ] = duration;
		}

	}
	sctk_spinlock_unlock( &array->lock );
}


/* GETTERS */

static inline uint64_t sctk_profiler_array_get_from_tab( struct sctk_profiler_array *array, uint64_t *tab , int id )
{
	uint64_t ret;

	sctk_spinlock_lock( &array->lock );
	{
		ret = tab[id];
	}
	sctk_spinlock_unlock( &array->lock );

	return ret;
}

static inline uint64_t sctk_profiler_array_get_hits( struct sctk_profiler_array *array, int id )
{
	return sctk_profiler_array_get_from_tab( array, array->sctk_profile_hits , id );
}

static inline uint64_t sctk_profiler_array_get_time( struct sctk_profiler_array *array, int id )
{
	return sctk_profiler_array_get_from_tab( array, array->sctk_profile_time , id );
}

static inline uint64_t sctk_profiler_array_get_max( struct sctk_profiler_array *array, int id )
{
	return sctk_profiler_array_get_from_tab( array, array->sctk_profile_max , id );
}

static inline uint64_t sctk_profiler_array_get_min( struct sctk_profiler_array *array, int id )
{
	return sctk_profiler_array_get_from_tab( array, array->sctk_profile_min , id );
}

static inline uint64_t sctk_profiler_array_get_parent( int id )
{
	return sctk_profile_parent_key[ id ];
}

static inline uint64_t sctk_profiler_array_has_child( int id )
{
	return sctk_profile_has_child[ id ];
}

char * sctk_profiler_array_get_desc( int id );

char * sctk_profiler_array_get_name( int id );

/* ******** */


#endif
