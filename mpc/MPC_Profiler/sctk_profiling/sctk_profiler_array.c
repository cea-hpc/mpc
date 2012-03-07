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
#include "sctk_profiler_array.h"

#include <stdio.h>
#include <stdlib.h>

uint64_t sctk_profile_parent_key[ SCTK_PROFILE_KEY_COUNT ];


#undef PROBE
#define PROBE( key, parent, desc ) #desc,

static const char * const sctk_profile_key_desc[ SCTK_PROFILE_KEY_COUNT ] =
{
	"", /* SCTK_PROFILE_NO_PARENT empty desc */
	#include "sctk_profiler_keys.h"
};

#undef PROBE


const char * const sctk_profiler_array_get_desc( int id )
{
	return sctk_profile_key_desc[ id ];
}


#undef PROBE
#define PROBE( key, probe_parent, desc ) sctk_profile_parent_key[ SCTK_PROFILE_ ## key ] = SCTK_PROFILE_ ## probe_parent;

void sctk_profiler_array_init_parent_keys()
{
	/* Set parent probe
	 * sctk_profile_parent_key[ x ] = PARENT; */
	#include "sctk_profiler_keys.h"
}

#undef PROBE

void sctk_profiler_array_init(struct sctk_profiler_array *array)
{
	int i = 0;

	for( i = 0 ; i < SCTK_PROFILE_KEY_COUNT ; i++ )
	{
		array->sctk_profile_hits[i] = 0;
		array->sctk_profile_time[i] = 0;
		array->sctk_profile_min[i] = 0;
		array->sctk_profile_max[i] = 0;
	}
	
	array->lock = 0;
}

struct sctk_profiler_array * sctk_profiler_array_new()
{
	struct sctk_profiler_array *ret = (struct sctk_profiler_array *)malloc( sizeof( struct sctk_profiler_array ));
	
	if( !ret )
	{
		perror( "malloc" );
		abort();
	}
	
	sctk_profiler_array_init( ret );
	
	return ret;
}


void sctk_profiler_array_release(struct sctk_profiler_array *array)
{
	int i = 0;

	for( i = 0 ; i < SCTK_PROFILE_KEY_COUNT ; i++ )
	{
		array->sctk_profile_hits[i] = 0;
		array->sctk_profile_time[i] = 0;
		array->sctk_profile_min[i] = 0;
		array->sctk_profile_max[i] = 0;
	}

	array->lock = 0;
}


static void __sctk_profiler_array_walk( struct sctk_profiler_array *array, short *been_walked, int current_id, int depth, void (*handler)( struct sctk_profiler_array *array, int id, int parent_id, int depth, void *arg ), void *arg )
{
	(handler)( array, current_id, sctk_profile_parent_key[ current_id ], depth, arg );
	been_walked[current_id] = 1;
	
	int i = 0;
	
	for( i = current_id ; i < SCTK_PROFILE_KEY_COUNT ; i++ )
	{
		if( !been_walked[i] && (sctk_profile_parent_key[ i ] == current_id) )
		{
			__sctk_profiler_array_walk( array, been_walked, i, depth + 1,  handler, arg );
		}
	}
	
	
}

void sctk_profiler_array_walk( struct sctk_profiler_array *array, void (*handler)( struct sctk_profiler_array *array, int id, int parent_id, int depth, void *arg ), void *arg )
{
	short been_walked[ SCTK_PROFILE_KEY_COUNT ];
	
	int i = 0;
	for( i = 0 ; i < SCTK_PROFILE_KEY_COUNT ; i++ )
	{
		been_walked[i] = 0;
	}
	
	/* 0 is a dummy key used for no parent */
	been_walked[0] = 1;
	
	for( i = 0 ; i < SCTK_PROFILE_KEY_COUNT ; i++ )
	{
		if( !been_walked[i] && ( sctk_profile_parent_key[i] ==  SCTK_PROFILE_NO_PARENT ) )
		{
			__sctk_profiler_array_walk( array, been_walked, i, 0,  handler, arg );
		}
	}

	
}
