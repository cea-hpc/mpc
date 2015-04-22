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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_topological_polling.h"

#include <stdlib.h>
#include "sctk_accessor.h"


/************************************************************************/
/* TOPOLOGICAL POLLING CELL SCTRUCTURE                                  */
/************************************************************************/

void sctk_topological_polling_cell_init( struct sctk_topological_polling_cell * cell )
{
	sctk_atomics_store_int( &cell->polling_counter, 0);
	cell->cell_id = SCTK_POLL_IGNORE;
}


/************************************************************************/
/* TOPOLOGICAL POLLING TREE SCTRUCTURE                                  */
/************************************************************************/

#if 0
struct sctk_topological_polling_tree
{
	struct sctk_polling_cell * cells; /**< Polling cells (one for each VP) */
	int cell_count; /**< Cell count, in fact the number of VPs */
	sctk_topological_polling_set_t polling_trigger; /**< Trigger is the value defining at which level the polling is done */
	sctk_topological_polling_set_t polling_range; /**< Range defines how far from the root PU we do POLL */
	int root_pu_id; /**< This is the ID of the core which is the closest from the polling target */
};
#endif



static inline int sctk_topological_polling_trigget_to_hwloc_type( sctk_topological_polling_set_t trigger )
{
	switch( trigger )
	{
		case SCTK_POLL_PU:
			return HWLOC_OBJ_PU;
		break;
		case SCTK_POLL_CORE:
			return HWLOC_OBJ_CORE;
		break;
		case SCTK_POLL_SOCKET:
			return HWLOC_OBJ_SOCKET;
		break;
		case SCTK_POLL_NUMA:
			return HWLOC_OBJ_NODE;
		break;
		case SCTK_POLL_MACHINE:
			return HWLOC_OBJ_MACHINE;
		break;
		
		default:
		case SCTK_POLL_NOT_SET:
			sctk_fatal("Bad polling trigger provided");
		break;
	}
}



void sctk_topological_polling_tree_init( struct sctk_topological_polling_tree * tree,  sctk_topological_polling_set_t trigger, sctk_topological_polling_set_t range, int root_pu )
{
	int number_of_vp = sctk_get_core_number();
	assume( number_of_vp != 0 );
	tree->cell_count = number_of_vp;
	
	/* Initialize cell array and set default values */
	
	tree->cells = sctk_malloc( sizeof( struct sctk_topological_polling_cell ) * tree->cell_count );
	assume( tree->cells != NULL );
	
	int i;
	
	for( i= 0 ; i < tree->cell_count ; i++ )
		sctk_topological_polling_cell_init( &tree->cells[i] );
	
	/* First extract the polling ranges */
	hwloc_cpuset_t range_cpuset;

	switch( range )
	{
		case SCTK_POLL_NONE:
			/* Nothing in range and there will be not trigger */
			return;
		break;
		case SCTK_POLL_PU:
			range_cpuset = sctk_topology_get_pu_cpuset( root_pu );
		break;
		case SCTK_POLL_CORE:
			range_cpuset = sctk_topology_get_core_cpuset( root_pu );
		break;
		case SCTK_POLL_SOCKET:
			range_cpuset = sctk_topology_get_socket_cpuset( root_pu );
		break;
		case SCTK_POLL_NUMA:
			range_cpuset = sctk_topology_get_numa_cpuset( root_pu );
		break;
		case SCTK_POLL_MACHINE:
			range_cpuset = sctk_topology_get_machine_cpuset( root_pu );
		break;
		
		default:
		case SCTK_POLL_NOT_SET:
			sctk_fatal("Bad polling range provided");
		break;
	}

	assume( range_cpuset );
	
	sctk_info("POLLING TREE ============");
	
	unsigned int trigger_vp_id = -1;
	
	/* Update the calling cells accordingly */
	hwloc_bitmap_foreach_begin( trigger_vp_id, range_cpuset )
	sctk_info("range_vp_id : %d (LOG : %d)",  trigger_vp_id,  sctk_topology_convert_os_pu_to_logical( trigger_vp_id ) );
	hwloc_bitmap_foreach_end(); 	
	
	/* Now do the same for the trigger */
		
	if( trigger == SCTK_POLL_NONE )
	{
		/* There is nothing to do here */
		return;
	}
	
	hwloc_cpuset_t roots_cpuset = sctk_topology_get_roots_for_level( sctk_topological_polling_trigget_to_hwloc_type(trigger) );
	
	assume( roots_cpuset );
	
	/* Do a "AND" with the range to extract the elligible triggers */
	hwloc_bitmap_and( roots_cpuset, roots_cpuset, range_cpuset );
	
	/* Now the root bitmap contains "1" only where it intersects with range */
	
	/* If bitmap is NULL, just return the range as trigger (only happens for CORE/CORE with core % root */
	if( hwloc_bitmap_iszero( roots_cpuset ) )
	{
		hwloc_bitmap_free( roots_cpuset );
		roots_cpuset = hwloc_bitmap_dup( range_cpuset );
	}
	
	/* Debug print */
	hwloc_bitmap_foreach_begin( trigger_vp_id, roots_cpuset )
	sctk_info("trigger_vp_id : %d (LOG %d)",  trigger_vp_id,  sctk_topology_convert_os_pu_to_logical( trigger_vp_id ) );
	hwloc_bitmap_foreach_end(); 		

	/* Now we can register trigger roots in the cell tree */
	hwloc_bitmap_foreach_begin( trigger_vp_id, roots_cpuset )
	
	/* Trigger on this VP */
	int trigger_logical_index = sctk_topology_convert_os_pu_to_logical( trigger_vp_id );
	
	if( ( 0 <= trigger_logical_index ) && ( trigger_logical_index < tree->cell_count ) )
	{
		tree->cells[trigger_logical_index].cell_id = SCTK_POLL_TRIGGER;
	}
	else
	{
		sctk_warning("Avoided a possible overflow when creating a topological polling tree");
	}
	hwloc_bitmap_foreach_end();
	
	
	/* Now we compute the ids starting from the triggers
	 * 
	 * The idea is to have:
	 * 
	 * T 1 2 3 4 .. T 1 2 3 4 .. T 1 2 3 4 
	 * 
	 * This will allow a direct tree based walk
	 * 
	 *  */
	int tree_id = 1;
	int in_trigger_range = 0;
	for( i= 0 ; i < tree->cell_count ; i++ )
	{
		if( tree->cells[i].cell_id == SCTK_POLL_TRIGGER )
		{
			in_trigger_range = 1;
			tree_id = 2;
			continue;
		}
		
		if( in_trigger_range )
		{
			tree->cells[i].cell_id = tree_id;
			tree_id++;
		}
		
	}
	
	/* After doing that make sure that values outside of range are 
	 * set to POLL_IGNORE */
	for( i= 0 ; i < tree->cell_count ; i++ )
	{
		int os_index = sctk_topology_convert_logical_pu_to_os( i );
		
		if( !hwloc_bitmap_isset(range_cpuset, os_index) )
		{
			sctk_info("Outside of range : %d",  i );
			tree->cells[i].cell_id = SCTK_POLL_IGNORE;
		}
	}
	
	for( i= 0 ; i < tree->cell_count ; i++ )
	{
		sctk_info("[%d]{%d}", i, tree->cells[i].cell_id );
	}
	
	
	sctk_info("POLLING TREE ======(END)==");
		
	
	hwloc_bitmap_free( roots_cpuset );
}


static inline void sctk_topological_polling_cell_poll( struct sctk_topological_polling_cell * cells, int offset,  void (*func)( void *), void * arg )
{
	struct sctk_topological_polling_cell * cell = &cells[offset];

	assert( 0 <= offset );
	
	int polling_counter = sctk_atomics_fetch_and_incr_int( &cell->polling_counter );
	
	/* Are we alone in the cell ? */
	if(  polling_counter == 0 )
	{
		/* Did we reach the calling point ? */
		if( cell->cell_id == SCTK_POLL_TRIGGER )
		{
			/* Yes */
			(func)( arg );
			/* We are done */
		}
		else
		{
			/* Compute the delta to parent cell
			 * note that this is not cell_id/2 due to rounding */
			int delta = cell->cell_id - (cell->cell_id / 2);
			int target_cell = offset - delta;

			/* Now Poll the parent cell */
			sctk_topological_polling_cell_poll( cells, target_cell, func, arg );
		}
		
	}
	
	sctk_atomics_decr_int( &cell->polling_counter );

}


void sctk_topological_polling_tree_poll( struct sctk_topological_polling_tree * tree,  void (*func)( void *), void * arg )
{
	int cpu_id = sctk_get_cpu();

	/* Nothing to do */
	if( !func )
	{
		return;
	}


	/* Check bounds */
	if( (cpu_id < 0) || ( tree->cell_count <= cpu_id ) )
	{
		/* Something went wrong the just poll */
		(func)( arg );
		return;
	} 
	
	/* Am I inside the range ? */
	if( tree->cells[cpu_id].cell_id == SCTK_POLL_IGNORE )
	{
		/* Nothing to do */
		return;
	}

	/* Now we enter the tree based polling
	 * here due to previous test we only have
	 * those which are part of the range */

	sctk_topological_polling_cell_poll( tree->cells, cpu_id,  func, arg );
	
}
