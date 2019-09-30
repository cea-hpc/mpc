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

#include <mpc_common_types.h>
#include "mpit_internal.h"

#undef SIZE_COUNTER
#undef COUNTER
#undef PROBE

#define SIZE_COUNTER PROBE
#define COUNTER PROBE

#define PROBE(key, parent, desc, mpit_hits, mpit_value, mpit_loww, mpit_highw) \
  SCTK_PROFILE_##key,

typedef enum
{
	SCTK_PROFILE_NO_PARENT = 0,
	#include "sctk_profiler_keys.h"
	SCTK_PROFILE_KEY_COUNT
}sctk_profile_key;

#undef SIZE_COUNTER
#undef COUNTER
#undef PROBE


typedef enum
{
	SCTK_PROFILE_TIME_PROBE,
	SCTK_PROFILE_COUNTER_SIZE_PROBE,
	SCTK_PROFILE_COUNTER_PROBE
}sctk_profile_key_type;

typedef enum
{
	SCTK_PROFILE_RENDER_WALK_DFS = 0,
	SCTK_PROFILE_RENDER_WALK_BFS = 1,
	SCTK_PROFILE_RENDER_WALK_BOTH = 2,
	SCTK_PROFILE_RENDER_WALK_COUNT
}sctk_profile_render_walk_mode;


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


extern uint64_t sctk_profile_has_child[ SCTK_PROFILE_KEY_COUNT ];
extern uint64_t sctk_profile_parent_key[ SCTK_PROFILE_KEY_COUNT ];
void sctk_profiler_array_init_parent_keys();

struct sctk_profiler_array * sctk_profiler_array_new();

void sctk_profiler_array_init(struct sctk_profiler_array *array);
void sctk_profiler_array_release(struct sctk_profiler_array *array);
void sctk_profiler_array_unify( struct sctk_profiler_array *array );
void sctk_profiler_array_walk( struct sctk_profiler_array *array, void (*handler)( struct sctk_profiler_array *array, int id, int parent_id, int depth, void *arg, int going_up ), void *arg , sctk_profile_render_walk_mode DFS );

extern sctk_profile_key_type sctk_profile_type[SCTK_PROFILE_KEY_COUNT];

static inline sctk_profile_key_type sctk_profiler_array_get_type(int id) {
  return sctk_profile_type[id];
}

MPC_T_pvar_t sctk_profiler_array_get_mpit_hits(int id);
MPC_T_pvar_t sctk_profiler_array_get_mpit_value(int id);
MPC_T_pvar_t sctk_profiler_array_get_mpit_size(int id);
MPC_T_pvar_t sctk_profiler_array_get_mpit_loww(int id);
MPC_T_pvar_t sctk_profiler_array_get_mpit_highw(int id);

static inline void sctk_profiler_array_hit( struct sctk_profiler_array *array, int id, int64_t value )
{
	mpc_common_spinlock_lock( &array->lock );
	{

		array->sctk_profile_hits[ id ]++;

		if( (value + array->sctk_profile_value[ id ]) < 0 )
		{
			array->sctk_profile_value[ id ] = 0;
		}
		else
		{
			array->sctk_profile_value[ id ] += value;
		}

		if( (array->sctk_profile_min[ id ] == 0) || (value < array->sctk_profile_min[ id ]) )
		{
			array->sctk_profile_min[ id ] = value;
		}

		if( (array->sctk_profile_max[ id ] == 0) || (array->sctk_profile_max[ id ] < value) )
		{
			array->sctk_profile_max[ id ] = value;
		}

                if (mpc_MPI_T_initialized()) {

                  MPC_T_pvar_t mpit_hits =
                      sctk_profiler_array_get_mpit_hits(id);

                  if (mpit_hits != MPI_T_PVAR_NULL) {
                    /* If we are here we have a PVAR */
                    MPC_T_session_set(-1, MPI_T_PVAR_ALL_HANDLES, mpit_hits,
                                      &array->sctk_profile_hits[id]);
                  }

                  MPC_T_pvar_t mpit_value =
                      sctk_profiler_array_get_mpit_value(id);

                  if (mpit_value != MPI_T_PVAR_NULL) {
                    /* If we are here we have a PVAR */
                    MPC_T_session_set(-1, MPI_T_PVAR_ALL_HANDLES, mpit_value,
                                      &array->sctk_profile_value[id]);
                  }

                  MPC_T_pvar_t mpit_loww =
                      sctk_profiler_array_get_mpit_loww(id);

                  if (mpit_loww != MPI_T_PVAR_NULL) {
                    /* If we are here we have a PVAR */
                    MPC_T_session_set(-1, MPI_T_PVAR_ALL_HANDLES, mpit_loww,
                                      &array->sctk_profile_min[id]);
                  }

                  MPC_T_pvar_t mpit_highw =
                      sctk_profiler_array_get_mpit_highw(id);

                  if (mpit_highw != MPI_T_PVAR_NULL) {
                    /* If we are here we have a PVAR */
                    MPC_T_session_set(-1, MPI_T_PVAR_ALL_HANDLES, mpit_highw,
                                      &array->sctk_profile_max[id]);
                  }
                }
        }
        mpc_common_spinlock_unlock(&array->lock);
}

/* GETTERS */

static inline uint64_t sctk_profiler_array_get_from_tab( struct sctk_profiler_array *array, uint64_t *tab , int id )
{
	uint64_t ret;

	mpc_common_spinlock_lock( &array->lock );
	{
		ret = tab[id];
	}
	mpc_common_spinlock_unlock( &array->lock );

	return ret;
}

static inline uint64_t sctk_profiler_array_get_hits( struct sctk_profiler_array *array, int id )
{
	return sctk_profiler_array_get_from_tab( array, array->sctk_profile_hits , id );
}

static inline uint64_t sctk_profiler_array_get_value( struct sctk_profiler_array *array, int id )
{
	return sctk_profiler_array_get_from_tab( array, array->sctk_profile_value , id );
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
