/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:33:57 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Patrick Carribault <patrick.carribault@cea.fr>                     # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Stephane Bouhrour <stephane.bouhrour@exascale-computing.eu>        # */
/* #                                                                      # */
/* ######################################################################## */
#include "hwloc.h"
#include "utlist.h"
#include "mpc_common_spinlock.h"
#include "mpc_topology.h"
#include "mpcomp_places_env.h"
#include "mpcomp_core.h"

static inline void
__places_restrict_bitmap( hwloc_bitmap_t res, const int nb_mvps )
{
	hwloc_bitmap_t default_bitmap;
	default_bitmap = mpc_omp_places_get_default_include_bitmap( nb_mvps );
	hwloc_bitmap_and( res, res, default_bitmap );
	hwloc_bitmap_free( default_bitmap );
}

static inline hwloc_bitmap_t
__places_dup_with_stride( const hwloc_bitmap_t origin, int stride, int idx, const int nb_mvps )
{
	int index;
	hwloc_bitmap_t new_bitmap;
	assert( origin );
	assert( stride != 0 );
	new_bitmap = hwloc_bitmap_alloc();
	assert( new_bitmap );
	hwloc_bitmap_zero( new_bitmap );
	hwloc_bitmap_foreach_begin( index, origin )
	hwloc_bitmap_set( new_bitmap, index + idx * stride );
	hwloc_bitmap_foreach_end();
	__places_restrict_bitmap( new_bitmap, nb_mvps );
	return new_bitmap;
}

static inline mpc_omp_places_info_t *
__places_infos_init( int allocate_bitmap )
{
	mpc_omp_places_info_t *new_place = NULL;
	new_place = ( mpc_omp_places_info_t * ) sctk_malloc( sizeof( mpc_omp_places_info_t ) );
	assert( new_place );
	memset( new_place, 0, sizeof( mpc_omp_places_info_t ) );

	if ( allocate_bitmap )
	{
		new_place->interval = hwloc_bitmap_alloc();
		assert( new_place->interval );
		hwloc_bitmap_zero( new_place->interval );
	}

	return new_place;
}

static inline int mpc_omp_places_expand_place_bitmap( mpc_omp_places_info_t *list, mpc_omp_places_info_t *place, int len, int stride, const int nb_mvps )
{
	int i;
	mpc_omp_places_info_t *new_place;
	assert( stride != 0 && len > 0 );
	const unsigned int place_id = place->id;

	/* i = 0 is place */
	for ( i = 1; i < len; i++ )
	{
		new_place = __places_infos_init( 0 );
		new_place->interval = __places_dup_with_stride( place->interval, i, stride, nb_mvps );

		if ( !( new_place->interval ) )
		{
			return 1;
		}

		DL_APPEND( list, new_place );
		new_place->id = place_id + i;
	}

	return 0;
}

static inline int ___safe_atoi( char *string, char **next )
{
	long retval = strtol( string, next, 10 );
	assert( retval > INT_MIN && retval < INT_MAX );

	if ( string == *next )
	{
		mpc_common_debug_error("missing numeric value\n" );
	}

	return ( int ) retval;
}

static inline int mpc_omp_places_detect_collision( mpc_omp_places_info_t *list )
{
	int colision_count;
	hwloc_bitmap_t bitmap_val = hwloc_bitmap_alloc();
	char string_place[256], string_place2[256], string_place3[256];
	mpc_omp_places_info_t *place, *place2, *saveptr, *saveptr2;
	colision_count = 0;
	DL_FOREACH_SAFE( list, place, saveptr )
	{
		DL_FOREACH_SAFE( saveptr, place2, saveptr2 )
		{
			if ( place == place2 )
			{
				continue;
			}

			hwloc_bitmap_and( bitmap_val, place->interval, place2->interval );

			if ( hwloc_bitmap_intersects( place->interval, place2->interval ) )
			{
				colision_count++;
				hwloc_bitmap_list_snprintf( string_place, 255, place->interval );
				hwloc_bitmap_list_snprintf( string_place2, 255, place2->interval );
				hwloc_bitmap_list_snprintf( string_place3, 255, bitmap_val );
				string_place[255] = '\0';
				string_place2[255] = '\0';
				mpc_common_debug_warning("colision between place #%d (%s) and place #%d (%s) |%s|\n", place->id, string_place, place2->id, string_place2, string_place3 );
			}
		}
	}

	if ( colision_count )
	{
		mpc_common_debug_warning("found %d colision(s)\n", colision_count );
	}

	return colision_count;
}


static inline void
__places_merge_interval( hwloc_bitmap_t res,
                              const hwloc_bitmap_t include,
                              const hwloc_bitmap_t exclude,
                              const int nb_mvps )
{
	assert( res );
	assert( include );
	assert( exclude );
	/* merge include et exclude interval */
	const int include_is_empty = !hwloc_bitmap_weight( include );
	const int exclude_is_empty = !hwloc_bitmap_weight( exclude );

	if ( exclude_is_empty )
	{
		hwloc_bitmap_copy( res, include );
	}
	else
	{
		if ( include_is_empty )
		{
			hwloc_bitmap_not( res, exclude );
		}
		else
		{
			hwloc_bitmap_andnot( res, include, exclude );
		}
	}

	__places_restrict_bitmap( res, nb_mvps );
}

static inline hwloc_bitmap_t
__places_build_interval_bitmap( int res, int num_places, int stride )
{
	int i;
	hwloc_bitmap_t interval;
	interval = hwloc_bitmap_alloc();
	assert( interval );
	hwloc_bitmap_zero( interval );

	for ( i = 0; i < num_places; i++ )
	{
		hwloc_bitmap_set( interval, res + i * stride );
	}

	return interval;
}

mpc_omp_places_info_t *
mpc_omp_places_build_interval_infos( char *string, char **end, const int nb_mvps )
{
	mpc_omp_places_info_t *place;
	int res, num_places, stride, exclude, error;
	hwloc_bitmap_t bitmap_tmp, bitmap_to_update;
	hwloc_bitmap_t include_interval, exclude_interval;

	if ( *string == '\0' )
	{
		return NULL;
	}

	place = __places_infos_init( 1 );
	include_interval = hwloc_bitmap_alloc();
	assert( include_interval );
	hwloc_bitmap_zero( include_interval );
	exclude_interval = hwloc_bitmap_alloc();
	assert( exclude_interval );
	hwloc_bitmap_zero( exclude_interval );
	error = 0;

	while ( 1 )
	{
		stride = 1;
		exclude = 0;
		num_places = 1;
		bitmap_to_update = include_interval;

		if ( *string == '!' )
		{
			string++;
			exclude = 1;
			bitmap_to_update = exclude_interval;
		}

		res = ___safe_atoi( string, end );
		string = *end;

		if ( !exclude )
		{
			if ( *string == ':' )
			{
				string++;
				num_places = ___safe_atoi( string, end );

				if ( string == *end )
				{
					error = 1; // missing value
					break;
				}

				if ( num_places <= 0 )
				{
					error = 1;
					mpc_common_debug_error("num_places in subplace is not positive integer ( %d )\n", num_places );
					break;
				}

				string = *end;
			}

			if ( *string == ':' )
			{
				string++;
				stride = ___safe_atoi( string, end );

				if ( string == *end )
				{
					error = 1; // missing value
					break;
				}

				string = *end;
			}
		}

		bitmap_tmp = __places_build_interval_bitmap( res, num_places, stride );
		hwloc_bitmap_or( bitmap_to_update, bitmap_to_update, bitmap_tmp );
		hwloc_bitmap_free( bitmap_tmp );

		if ( *string != ',' )
		{
			break;
		}

		string++; // skip ','
	}

	__places_merge_interval( place->interval, include_interval, exclude_interval, nb_mvps );
	return ( error ) ? NULL : place;
}

static char *
__places_build_numas_places( const int places_number, int *error )
{
	int numa_found;
	size_t cnwrites, tnwrites;
	char *places_string;
	hwloc_topology_t topology;
	hwloc_obj_t prev_numa, next_numa;
	hwloc_obj_t prev_pu, next_pu;

   topology = mpc_topology_get();
	prev_numa = NULL;
	places_string = ( char * ) malloc( 4096 * sizeof( char ) );
	assert( places_string );
	numa_found = 0;
	tnwrites = 0;
	const int number_numas = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_NUMANODE );
	const int __places_number = ( places_number == -1 ) ? number_numas : places_number;

	while ( ( next_numa = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_NUMANODE, prev_numa ) ) )
	{
		/* Level with no PU ... skip */
		if ( hwloc_get_nbobjs_inside_cpuset_by_type( topology, next_numa->cpuset, HWLOC_OBJ_PU ) <= 0 )
		{
			continue;
		}

		size_t current_index = 0;
		int pu_found = 0;
		char currend_bitmap_list[255];
		prev_pu = NULL;

		while ( ( next_pu = hwloc_get_next_obj_inside_cpuset_by_type( topology, next_numa->cpuset, HWLOC_OBJ_PU, prev_pu ) ) )
		{
			int tmp_write;
			assert( current_index < 254 );

			if ( pu_found )
			{
				tmp_write = snprintf( currend_bitmap_list + current_index, 255 - current_index, ",%d", next_pu->logical_index );
			}
			else
			{
				tmp_write = snprintf( currend_bitmap_list + current_index, 255 - current_index, "%d", next_pu->logical_index );
			}

			current_index += tmp_write;
			pu_found++;
			prev_pu = next_pu;
		}

		assert( current_index < 254 );
		currend_bitmap_list[current_index] = '\0';
		/* {"pulist"} */
		const size_t size2copy = strlen( currend_bitmap_list ) + 2 + ( ( numa_found ) ? 1 : 0 );
		assume( tnwrites + size2copy < 4096 );

		if ( !numa_found )
		{
			cnwrites = snprintf( places_string + tnwrites, 4096 - tnwrites, "{%s}", currend_bitmap_list );
		}
		else
		{
			cnwrites = snprintf( places_string + tnwrites, 4096 - tnwrites, ",{%s}", currend_bitmap_list );
		}

		tnwrites += cnwrites;
		prev_numa = next_numa;
		numa_found++;

		/* Enough PLACE for the user */
		if ( numa_found == __places_number )
		{
			break;
		}
	}

	if ( numa_found < __places_number )
	{
		mpc_common_debug_warning("Resize number places from %d to %d\n", __places_number, numa_found );
	}

	if ( !numa_found )
	{
		mpc_common_debug_error("No numa nodes found on node\n" );
		free( places_string );
		places_string = NULL;
		assert( error );
		*error = 1;
	}
	else
	{
		assume( tnwrites + 1 < 4096 );
		places_string[tnwrites] = '\0';
		assert( error );
		*error = 0;
	}

	return places_string;
}

static char *
mpc_omp_places_build_sockets_places( const int places_number, int *error )
{
	int socket_found;
	size_t cnwrites, tnwrites;
	char *places_string;
	hwloc_topology_t topology;
	hwloc_obj_t prev_socket, next_socket;
	hwloc_obj_t prev_pu, next_pu;

   topology = mpc_topology_get();
	prev_socket = NULL;
	places_string = ( char * ) malloc( 4096 * sizeof( char ) );
	assert( places_string );
	socket_found = 0;
	tnwrites = 0;
	const int number_sockets = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_PACKAGE );
	const int __places_number = ( places_number == -1 ) ? number_sockets : places_number;

	while ( ( next_socket = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_PACKAGE, prev_socket ) ) )
	{
		/* Level with no PU ... skip */
		if ( hwloc_get_nbobjs_inside_cpuset_by_type( topology, next_socket->cpuset, HWLOC_OBJ_PU ) <= 0 )
		{
			continue;
		}

		size_t current_index = 0;
		int pu_found = 0;
		char currend_bitmap_list[255];
		prev_pu = NULL;

		while ( ( next_pu = hwloc_get_next_obj_inside_cpuset_by_type( topology, next_socket->cpuset, HWLOC_OBJ_PU, prev_pu ) ) )
		{
			int tmp_write;
			assert( current_index < 254 );

			if ( pu_found )
			{
				tmp_write = snprintf( currend_bitmap_list + current_index, 255 - current_index, ",%d", next_pu->logical_index );
			}
			else
			{
				tmp_write = snprintf( currend_bitmap_list + current_index, 255 - current_index, "%d", next_pu->logical_index );
			}

			current_index += tmp_write;
			pu_found++;
			prev_pu = next_pu;
		}

		/* {"pulist"} */
		const size_t size2copy = strlen( currend_bitmap_list ) + 2 + ( ( socket_found ) ? 1 : 0 );
		assume( tnwrites + size2copy < 4096 );

		if ( !socket_found )
		{
			cnwrites = snprintf( places_string + tnwrites, 4096 - tnwrites, "{%s}", currend_bitmap_list );
		}
		else
		{
			cnwrites = snprintf( places_string + tnwrites, 4096 - tnwrites, ",{%s}", currend_bitmap_list );
		}

		tnwrites += cnwrites;
		prev_socket = next_socket;
		socket_found++;

		/* Enough PLACE for the user */
		if ( socket_found == __places_number )
		{
			break;
		}
	}

	if ( socket_found < __places_number )
	{
		mpc_common_debug_warning("Resize number places from %d to %d\n", __places_number, socket_found );
	}

	if ( !socket_found )
	{
		mpc_common_debug_error("No socket found on node\n" );
		free( places_string );
		places_string = NULL;
		assert( error );
		*error = 1;
	}
	else
	{
		assume( tnwrites + 1 < 4096 );
		places_string[tnwrites] = '\0';
		assert( error );
		*error = 0;
	}

	return places_string;
}

static char *
mpc_omp_places_build_threads_places( const int places_number, int *error )
{
	int pu_found;
	size_t cnwrites, tnwrites;
	char *places_string;
	hwloc_topology_t topology;
	hwloc_obj_t prev_pu, next_pu;

   topology = mpc_topology_get();
	prev_pu = NULL;
	places_string = ( char * ) malloc( 4096 * sizeof( char ) );
	assert( places_string );
	pu_found = 0;
	tnwrites = 0;
	const int threads_number = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_PU );
	const int __places_number = ( places_number == -1 ) ? threads_number : places_number;

	while ( ( next_pu = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_PU, prev_pu ) ) )
	{
		char currend_bitmap_list[255];
		hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, next_pu->cpuset );
		/* {"pulist"} */
		const size_t size2copy = strlen( currend_bitmap_list ) + 2 + ( ( pu_found ) ? 1 : 0 );
		assume( tnwrites + size2copy < 4096 );

		if ( !pu_found )
		{
			cnwrites = snprintf( places_string + tnwrites, 4096 - tnwrites, "{%s}", currend_bitmap_list );
		}
		else
		{
			cnwrites = snprintf( places_string + tnwrites, 4096 - tnwrites, ",{%s}", currend_bitmap_list );
		}

		tnwrites += cnwrites;
		prev_pu = next_pu;
		pu_found++;

		/* Enough PLACE for the user */
		if ( pu_found == __places_number )
		{
			break;
		}
	}

	if ( pu_found < __places_number )
	{
		mpc_common_debug_warning("Resize number places from %d to %d\n", __places_number, pu_found );
	}

	assume( tnwrites + 1 < 4096 );
	places_string[tnwrites] = '\0';
	assert( error );
	*error = 0;
	return places_string;
}

static char *
mpc_omp_places_build_cores_places( const int places_number, int *error )
{
	int core_found;
	size_t cnwrites, tnwrites;
	char *places_string;
	hwloc_topology_t topology;
	hwloc_obj_t prev_core, next_core;

   topology = mpc_topology_get();
	prev_core = NULL;
	places_string = ( char * ) malloc( 4096 * sizeof( char ) );
	assert( places_string );
	core_found = 0;
	tnwrites = 0;
	const int core_number = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_CORE );
	const int __places_number = ( places_number == -1 ) ? core_number : places_number;

	while ( ( next_core = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_CORE, prev_core ) ) )
	{
		char currend_bitmap_list[255];
		hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, next_core->cpuset );
		/* {"pulist"} */
		const size_t size2copy = strlen( currend_bitmap_list ) + 2 + ( ( core_found ) ? 1 : 0 );
		assume( tnwrites + size2copy < 4096 );

		if ( !core_found )
		{
			cnwrites = snprintf( places_string + tnwrites, 4096 - tnwrites, "{%s}", currend_bitmap_list );
		}
		else
		{
			cnwrites = snprintf( places_string + tnwrites, 4096 - tnwrites, ",{%s}", currend_bitmap_list );
		}

		tnwrites += cnwrites;
		prev_core = next_core;
		core_found++;

		/* Enough PLACE for the user */
		if ( core_found == __places_number )
		{
			break;
		}
	}

	if ( core_found < __places_number )
	{
		mpc_common_debug_warning("Resize number places from %d to %d\n", __places_number, core_found );
	}

	assume( tnwrites + 1 < 4096 );
	places_string[tnwrites] = '\0';
	assert( error );
	*error = 0;
	return places_string;
}

/**
 * Parse an optional number of places with named places.
 *
 * Format: (unsigned non-null integer)
 *
 * \return The number of places (0 if there was an issue
 * or -1 if there was no such number of places)
 */
static int mpc_omp_places_named_extract_num( const char *env, char *string )
{
	char *next_char;

	if ( *string == '\0' )
	{
		return -1;    /* Dynamic size */
	}

	if ( *string != '(' )
	{
		return 0;    /* Error */
	}

	string++; // skip '('
	const int num_places = ___safe_atoi( string, &next_char );

	if ( num_places <= 0 || string == next_char )
	{
		if ( num_places <= 0 )
		{
			mpc_common_debug_error("len in place is not positive integer ( %d )\n", num_places );
		}

		return 0;
	}

	string = next_char;

	if ( *string != ')' )
	{
		return 0;
	}

	string++; // skip ')'

	if ( 0 && *string != '\0' )
	{
		mpc_common_debug_error("offset %ld with \'%c\' end: %s\n", string - env - 1, *string, string );
		return 0;
	}

	return num_places;
}

/**
 * Check if a string corresponds to a named places.
 *
 * This function checks if the string argument corresponds
 * to a known place name with an additional integer for the
 * number of places).
 *
 * \param[in] env String to parse corresponding to places
 * \param[in] string ??
 * \param[out] error Output error (if different from 0)
 * \return The places with string format or NULL if this
 * was not a named places
 */
static char *
__places_is_named_places( const char *env, char *string, int *error )
{
	if ( !strncmp( string, "threads", 7 ) )
	{
		const int num_places = mpc_omp_places_named_extract_num( env, string + 7 );

		if ( !num_places )
		{
			return NULL;
		}

		return mpc_omp_places_build_threads_places( num_places, error );
	}

	if ( !strncmp( string, "cores", 5 ) )
	{
		const int num_places = mpc_omp_places_named_extract_num( env, string + 5 );

		if ( !num_places )
		{
			return NULL;
		}

		return mpc_omp_places_build_cores_places( num_places, error );
	}

	if ( !strncmp( string, "sockets", 7 ) )
	{
		const int num_places = mpc_omp_places_named_extract_num( env, string + 7 );

		if ( !num_places )
		{
			return NULL;
		}

		return mpc_omp_places_build_sockets_places( num_places, error );
	}

	if ( !strncmp( string, "numas", 5 ) )
	{
		const int num_places = mpc_omp_places_named_extract_num( env, string + 5 );

		if ( !num_places )
		{
			return NULL;
		}

		return __places_build_numas_places( num_places, error );
	}

	*error = 0;
	return NULL;
}

static inline mpc_omp_places_info_t *
__places_build_place_infos( char *string, char **end, const int nb_mvps )
{
	int exclude, stride, len;
	mpc_omp_places_info_t *new_place, *list;
	static int _mpcomp_places_generate_count = 0;
	list = NULL; // init utlist.h

	if ( *string == '\0' )
	{
		return NULL;
	}

	while ( 1 )
	{
		len = 1;
		stride = 1;
		exclude = 0;

		if ( *string == '!' )
		{
			exclude = 1;
			string++; // skip '!'
		}

		if ( *string != '{' )
		{
			break;
		}

		string++; // skip '{'
		new_place = mpc_omp_places_build_interval_infos( string, end, nb_mvps );

		if ( !new_place )
		{
			mpc_common_debug_error("can't parse subplace\n" );
			break;
		}

		DL_APPEND( list, new_place );
		new_place->id = _mpcomp_places_generate_count++;
		string = *end;

		if ( *string != '}' )
		{
			break;
		}

		string++; // skip '}'

		if ( !exclude )
		{
			if ( *string == ':' )
			{
				string++;
				len = ___safe_atoi( string, end );

				if ( string == *end )
				{
					break;
				}

				if ( len <= 0 )
				{
					mpc_common_debug_error("len in place is not positive integer ( %d )\n", len );
					break;
				}

				string = *end;
			}

			if ( *string == ':' )
			{
				string++;
				stride = ___safe_atoi( string, end );

				if ( string == *end )
				{
					break;
				}

				string = *end;
			}

			_mpcomp_places_generate_count += ( len - 1 );
			mpc_omp_places_expand_place_bitmap( list, new_place, len, stride, nb_mvps );
		}
		else
		{
			hwloc_bitmap_not( new_place->interval, new_place->interval );
		}

		if ( *string != ',' )
		{
			break;
		}

		string++; // skip ','
	}

	*end = string;
	return list;
}

hwloc_bitmap_t
mpc_omp_places_get_default_include_bitmap( const int nb_mvps )
{
	int i;
	hwloc_obj_t pu;
	static volatile hwloc_bitmap_t __default_num_threads_bitmap = NULL;
	static mpc_common_spinlock_t multiple_tasks_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

	if ( !__default_num_threads_bitmap )
	{
		mpc_common_spinlock_lock( &multiple_tasks_lock );

		if ( !__default_num_threads_bitmap )
		{
			hwloc_bitmap_t __tmp_default_bitmap_places = hwloc_bitmap_alloc();
			hwloc_topology_t topology = mpc_topology_get();
			hwloc_bitmap_zero( __tmp_default_bitmap_places );

			for ( i = 0; i < nb_mvps; i++ )
			{
				pu = hwloc_get_obj_by_type( topology, HWLOC_OBJ_PU, i );

				if ( pu )
				{
					hwloc_bitmap_or( __tmp_default_bitmap_places, __tmp_default_bitmap_places, pu->cpuset );
				}
			}

			__default_num_threads_bitmap = __tmp_default_bitmap_places;
		}

		mpc_common_spinlock_unlock( &multiple_tasks_lock );
	}

#if 0  /* DEBUG */
	char currend_bitmap_list[256];
	hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, __default_num_threads_bitmap );
	fprintf( stderr, "Default cpuset : %s w/ %d threads\n", currend_bitmap_list, hwloc_bitmap_weight( __default_num_threads_bitmap ) );
#endif /* DEBUG */
	return hwloc_bitmap_dup( __default_num_threads_bitmap );
}

void mpc_omp_display_places( mpc_omp_places_info_t *list )
{
	int count = 0;
	char currend_bitmap_list[256];
	mpc_omp_places_info_t *place, *saveptr;
	DL_FOREACH_SAFE( list, place, saveptr )
	{
		hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, place->interval );
		currend_bitmap_list[255] = '\0';
		fprintf( stdout, "place #%d: %s\n", count++, currend_bitmap_list );
	}
}

int mpc_omp_places_get_real_nb_mvps( mpc_omp_places_info_t *list )
{
	int real_nb_mvps = 0;
	mpc_omp_places_info_t *place, *saveptr;
	DL_FOREACH_SAFE( list, place, saveptr )
	{
		real_nb_mvps += hwloc_bitmap_weight( place->interval );
	}
	return real_nb_mvps;
}

static inline void __places_translate_cpu_list( int **cpus_order, const int cpu_number )
{
	int *__cpus_order;
	int i, next, prev, pos_in_bitmap;
	hwloc_bitmap_t __cpus_order_set = hwloc_bitmap_alloc();
	hwloc_bitmap_zero( __cpus_order_set );
	__cpus_order = *cpus_order;

	/* Build a set */
	for ( i = 0; i < cpu_number; i++ )
	{
		hwloc_bitmap_set( __cpus_order_set, __cpus_order[i] );
	}

#if 0  /* DEBUG */
	char currend_bitmap_list[256];
	hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, __cpus_order_set );
	fprintf( stderr, "%s:%d : %s w/ %d threads\n", __func__, __LINE__, currend_bitmap_list, hwloc_bitmap_weight( __cpus_order_set ) );
#endif /* DEBUG */
	prev = -1;
	pos_in_bitmap = 0;

	while ( ( next = hwloc_bitmap_next( __cpus_order_set, prev ) ) != -1 )
	{
		for ( i = 0; i < cpu_number; i++ )
		{
			if ( __cpus_order[i] == next )
			{
				__cpus_order[i] = pos_in_bitmap++;
				break;
			}
		}

		prev = next;
	}
}

int _mpc_omp_places_get_topo_info( mpc_omp_places_info_t *list, int **shape, int **cpus_order )
{
	int __cur_cpu_id, __place_cur_id, __place_old_id;
	mpc_omp_places_info_t *place, *saveptr;
	int *__place_shape, *__cpus_order;
	__place_shape = ( int * ) malloc( sizeof( int ) * 2 );
	assert( __place_shape );
	memset( __place_shape, 0, sizeof( int ) * 2 );
	DL_COUNT( list, place, __place_shape[0] );
	/* No DL_FIRST ... bypass utlist API */
	__place_shape[1] = hwloc_bitmap_weight( list->interval );
	//fprintf(stderr, "Tree Shape : %d %d\n", __place_shape[0], __place_shape[1] );
	const int __places_nb_mvps = mpc_omp_places_get_real_nb_mvps( list );
	assert( __place_shape[0] * __place_shape[1] == __places_nb_mvps );
	/* Build cpu order */
	__cpus_order = ( int * ) malloc( sizeof( int ) * __places_nb_mvps );
	assert( __cpus_order );
	memset( __cpus_order, 0, sizeof( int ) * __places_nb_mvps );
	__cur_cpu_id = 0;
	DL_FOREACH_SAFE( list, place, saveptr )
	{
		__place_old_id = -1;

		/* Get first elt */
		while ( ( __place_cur_id = hwloc_bitmap_next( place->interval, __place_old_id ) ) >= 0 )
		{
			__cpus_order[__cur_cpu_id++] = __place_cur_id;
			__place_old_id = __place_cur_id;
		}
	}
	/* Rename CPU PROC in places */
	__places_translate_cpu_list( &__cpus_order, __cur_cpu_id );
	/* Build logical mpc task cpu in places */
	int __rename_cur_cpu_id = 0;
	DL_FOREACH_SAFE( list, place, saveptr )
	{
		int i;
		const int place_num_elts = hwloc_bitmap_weight( place->interval );
		place->logical_interval = hwloc_bitmap_alloc();
		hwloc_bitmap_zero( place->logical_interval );

		for ( i = 0; i < place_num_elts; i++ )
		{
			hwloc_bitmap_set( place->logical_interval, __cpus_order[__rename_cur_cpu_id++] );
		}
	}
#if 0  /* Begin debug infos */
	fprintf( stderr, "CPU order: " );
	int i;

	for ( i = 0; i < __places_nb_mvps; i++ )
	{
		fprintf( stderr, "%d ", __cpus_order[i] );
	}

	fprintf( stderr, "\n" );
#endif /* End debug infos */
	/* Return PLACES INFOS */
	*shape = __place_shape;
	*cpus_order = __cpus_order;
	return __places_nb_mvps;
}

static int
__places_detect_heretogeneous_places( mpc_omp_places_info_t *list )
{
	int invalid_size_count;
	const int first_size = hwloc_bitmap_weight( list->interval );
	mpc_omp_places_info_t *place, *saveptr;
	invalid_size_count = 0;
	DL_FOREACH_SAFE( list, place, saveptr )
	{
		if ( first_size != hwloc_bitmap_weight( place->interval ) )
		{
			invalid_size_count++;
		}
	}

	if ( invalid_size_count )
	{
		//mpc_common_debug_warning("Can't build regular tree\n" );
	}

	return invalid_size_count;
}

/**
 * Entry point to parse OMP_PLACES environment variable.
 *
 * \param[in] nb_mvps Target number of microVPs (i.e., OpenMP threads)
 * \return List of places (or NULL if an error occured)
 */
mpc_omp_places_info_t *
_mpc_omp_places_env_variable_parsing( const int nb_mvps )
{
	int error = 0;
	char *tmp, *string, *end;
	mpc_omp_places_info_t *list = NULL;
	/* Get the value of OMP_PLACES (as a string) */
	tmp = mpc_omp_conf_get()->places;

	char * env_override = getenv("OMP_PLACES");

	if(env_override)
	{
		tmp = env_override;
	}

	mpc_common_debug_log("OMP_PLACES = <%s>\n", tmp );
	const char *prev_env = strdup( tmp );
	string = ( char * ) prev_env;
	const char *named_str = __places_is_named_places( prev_env, string, &error );

	if ( error )
	{
		mpc_common_debug_error("OMP_PLACES ignored\n" );
		return NULL;
	}

	const char *env = ( named_str ) ? strdup( named_str ) : strdup( tmp );
	string = ( char * ) env;

	if ( !env )
	{
		mpc_common_debug_error("Can't parse named places\n" );
		return NULL;
	}

	list = __places_build_place_infos( string, &end, nb_mvps );

	if ( *end != '\0' )
	{
		mpc_common_debug_error("offset %ld with \'%c\' end: %s\n", end - env - 1, *end, end );
		return NULL;
	}

	if ( mpc_omp_places_detect_collision( list ) )
	{
		mpc_common_debug_warning("MPC doesn't support collision between places\n" );
		return NULL;
	}

	if ( __places_detect_heretogeneous_places( list ) )
	{
		//mpc_common_debug_warning("Every place must have the same number of threads\n" );
		return NULL;
	}

	return list;
}
