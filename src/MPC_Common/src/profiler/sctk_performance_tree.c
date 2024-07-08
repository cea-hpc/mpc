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

#ifdef MPC_Profiler
#include "sctk_performance_tree.h"

#include "mpc_common_debug.h"

#include <stdlib.h>
#include <string.h>

void sctk_performance_tree_relative_percentage( struct sctk_profiler_array *array, int id, int parent_id, __UNUSED__ int depth, void *arg , __UNUSED__ int going_up )
{
	struct sctk_performance_tree *tr = (struct sctk_performance_tree *)arg;

	if(  ( sctk_profiler_array_get_type( id ) == SCTK_PROFILE_COUNTER_PROBE )
	||   (sctk_profiler_array_get_type( id ) != sctk_profiler_array_get_type( parent_id )) )
	{
			tr->entry_relative_percentage_time[ id ] =  0;
			tr->entry_relative_percentage_hits[ id ] =  0;
			return;
	}
	else if( sctk_profiler_array_get_parent( id ) != SCTK_PROFILE_NO_PARENT ) 	/* If entry has parent of the same type and is a time or size probe*/
	{
		/* Compute relative percentage */
		if( sctk_profiler_array_get_value( array, parent_id ) )
		{
			tr->entry_relative_percentage_time[ id ] = (double)sctk_profiler_array_get_value( array, id ) / (double)sctk_profiler_array_get_value( array, parent_id );
		}
		else
		{
			tr->entry_relative_percentage_time[ id ] =  0;
		}

		if( sctk_profiler_array_get_hits( array, parent_id ) )
		{
			tr->entry_relative_percentage_hits[ id ] = (double)sctk_profiler_array_get_hits( array, id ) / (double)sctk_profiler_array_get_hits( array, parent_id );
		}
		else
		{
			tr->entry_relative_percentage_hits[ id ] =  0;
		}

		if( 1 < tr->entry_relative_percentage_time[ id ] )
		{
			mpc_common_debug_warning( "sctk_performance_tree detected an overflow in performance measurements");
			tr->entry_relative_percentage_time[ id ] = 1;
		}

		if( 1 < tr->entry_relative_percentage_hits[ id ] )
		{
			mpc_common_debug_warning( "sctk_performance_tree detected an overflow in performance measurements");
			tr->entry_relative_percentage_hits[ id ] = 1;
		}

	}
	else
	{
		uint64_t roots_hits = 0;
		uint64_t roots_time = 0;

		int i = 0;

		for( i = 1 ; i < SCTK_PROFILE_KEY_COUNT ; i++ )
		{
			if( sctk_profiler_array_get_parent( i ) == SCTK_PROFILE_NO_PARENT )
			{
				roots_hits += sctk_profiler_array_get_hits( array, i );
				roots_time += sctk_profiler_array_get_value( array, i );
			}
		}

		tr->entry_relative_percentage_time[ id ] = (double)sctk_profiler_array_get_value( array, id ) / roots_time;
		tr->entry_relative_percentage_hits[ id ] = (double)sctk_profiler_array_get_hits( array, id ) / roots_hits;
	}
}


void sctk_performance_tree_total_time_and_hits( struct sctk_profiler_array *array, int id, int parent_id, int depth, void *arg, int going_up  )
{
	struct sctk_performance_tree *tr = (struct sctk_performance_tree *)arg;

	if( !sctk_profiler_array_has_child( id ) &&  (sctk_profiler_array_get_type(id) == SCTK_PROFILE_TIME_PROBE) )
	{
		/* Only process leaf nodes */

    		tr->total_time += sctk_profiler_array_get_value( array, id );
		tr->total_hits += sctk_profiler_array_get_hits( array, id );

	}


}

void sctk_performance_tree_absolute_percentage( struct sctk_profiler_array *array, int id, int parent_id, int depth, void *arg, int going_up  )
{
	struct sctk_performance_tree *tr = (struct sctk_performance_tree *)arg;

	if( sctk_profiler_array_get_type(id) == SCTK_PROFILE_TIME_PROBE )
	{
		/* Compute relative percentage */
		if( tr->total_time )
		{
			tr->entry_total_percentage_time[ id ] = (double)sctk_profiler_array_get_value( array, id ) / (double)tr->total_time;
		}
		else
		{
			tr->entry_total_percentage_time[ id ] =  0;
		}

		if( tr->total_hits )
		{
			tr->entry_total_percentage_hits[ id ] = (double)sctk_profiler_array_get_hits( array, id ) / (double)tr->total_hits;
		}
		else
		{
			tr->entry_total_percentage_hits[ id ] =  0;
		}
	}
	else
	{
		tr->entry_total_percentage_time[ id ] =  0;
		tr->entry_total_percentage_hits[ id ] =  0;
	}
}




void  sctk_performance_tree_init( struct sctk_performance_tree *tr, struct sctk_profiler_array *array)
{
	if( !array->been_unified )
	{
		mpc_common_debug_error( "Performance tree can be built only over an unified tree !");
		abort();
	}

	memset( (void *)tr, 0, sizeof(  struct sctk_performance_tree ) );

	/* Fill relative percentage */
	sctk_profiler_array_walk( array, sctk_performance_tree_relative_percentage , (void *)tr , 1);
	/* Fill Total time and hits */
	sctk_profiler_array_walk( array, sctk_performance_tree_total_time_and_hits , (void *)tr , 1);
  	/* Fill the total time */
	//tr->total_time = array->run_time;
  	/* Compute absolute percentages */
	sctk_profiler_array_walk( array, sctk_performance_tree_absolute_percentage , (void *)tr , 1);

	int i = 0;

	/* Fill average time */
	for( i = 0 ; i  < SCTK_PROFILE_KEY_COUNT ; i++ )
	{
		if( sctk_profiler_array_get_hits( array, i ) )
		{
			tr->entry_average_time[ i ] = sctk_profiler_array_get_value( array, i ) / sctk_profiler_array_get_hits( array, i );
		}
		else
		{
			tr->entry_average_time[ i ] =  0;
		}
	}

}


void  sctk_performance_tree_release( struct sctk_performance_tree *tr )
{
	memset( (void *)tr, 0, sizeof(  struct sctk_performance_tree ) );
}

#endif /* MPC_Profiler */
