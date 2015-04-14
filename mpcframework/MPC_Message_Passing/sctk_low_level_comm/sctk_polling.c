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
#include "sctk_polling.h"
#include <stdlib.h>
#include "sctk_accessor.h"

void sctk_polling_cell_zero( struct sctk_polling_cell * cell )
{
	sctk_atomics_store_int( &cell->polling_counter, 0);	
	cell->is_free = 1;	
}

void sctk_polling_tree_init_empty( struct sctk_polling_tree * tree )
{
	tree->push_lock = SCTK_SPINLOCK_INITIALIZER;
	tree->cell_count = 0;
	tree->cells = NULL;
	tree->local_task_id_to_cell_lut = NULL;

	
	/* We assume that only the number of thread can
	 * enter the polling function */
	tree->max_load = sctk_get_core_number();
	/* Note that we set max load here
	 * as the rail might want to override it */
}

void sctk_polling_tree_init_once( struct sctk_polling_tree * tree )
{
	sctk_spinlock_lock( &tree->push_lock );
	
	if( tree->cell_count )
	{
		sctk_spinlock_unlock( &tree->push_lock );
		return;
	}
	
	int local_task_count = sctk_get_local_task_number();
	assume( local_task_count != 0 );
	tree->cell_count = local_task_count;
	
	tree->cells = sctk_malloc( sizeof( struct sctk_polling_cell ) * tree->cell_count );
	assume( tree->cells );
	
	tree->local_task_id_to_cell_lut = sctk_malloc( sizeof( int ) * tree->cell_count );
	assume( tree->local_task_id_to_cell_lut );
	
	int i;
	
	for( i = 0 ; i < tree->cell_count ; i++ )
	{
		sctk_polling_cell_zero( &tree->cells[i] );
		tree->local_task_id_to_cell_lut[i] = -1;
	}
	
	sctk_spinlock_unlock( &tree->push_lock );
}

void sctk_polling_tree_release( struct sctk_polling_tree * tree )
{
	sctk_spinlock_lock( &tree->push_lock );
	
	tree->cell_count = 0;
	
	sctk_free( tree->cells );
	tree->cells = NULL;
	
	sctk_free( tree->local_task_id_to_cell_lut );
	tree->local_task_id_to_cell_lut = NULL;
}

void sctk_polling_tree_join( struct sctk_polling_tree * tree )
{
	int my_local_rank = sctk_get_local_task_rank();
	
	assume( 0 <= my_local_rank );
	
	sctk_spinlock_lock( &tree->push_lock );
	
	int i;

	for( i = 0 ; i < tree->cell_count ; i++ )
	{
		/* Cell is free ? */
		if( tree->cells[i].is_free )
		{
			/* Acquire it */
			tree->cells[i].is_free = 0;
			/* A cell cannot be acquire twice */
			//sctk_warning("TASK %d got cell %d", my_local_rank, i );
		
			//assume( tree->local_task_id_to_cell_lut[ my_local_rank ] == -1 );
			/* Register in LUT */
			tree->local_task_id_to_cell_lut[ my_local_rank ] = i;
			/* Done */
			
			
			
			break;
		}
	}
	
	
	sctk_spinlock_unlock( &tree->push_lock );
}


void __sctk_polling_tree_poll( struct sctk_polling_tree * tree , void (*func)(void *) , int cell_id , int call_id, void * arg )
{
	assume( 0 <= cell_id );
	assume( cell_id < tree->cell_count );
	
	struct sctk_polling_cell * cell = &tree->cells[ cell_id ];
	
	int current_load = sctk_atomics_fetch_and_incr_int( &cell->polling_counter );
	
	/* We consider that only core_count threads should go through */
	if( tree->max_load  < current_load )
	{
		sctk_atomics_decr_int( &cell->polling_counter );
		return;
	}

	/* Root cell ? */
	if( cell_id == 0 )
	{
		/* Call polling */
		(func)( arg );
	}
	else
	{
		/* Go up the tree */
		int parent_cell_id =  ( ( cell_id + 1 )/2 ) - 1;
		__sctk_polling_tree_poll( tree, func, parent_cell_id, call_id, arg );
	}
	
	sctk_atomics_decr_int( &cell->polling_counter );
}

__thread volatile int __last_local_id = -1; 

void sctk_polling_tree_poll( struct sctk_polling_tree * tree , void (*func)(void *), void * arg)
{
	/* Nothing to do */
	if( !func )
		return;

	if( tree->cell_count == 0 )
	{
		/* We are before thread launch the polling
		 * tree has not been initialized yet then 
		 * just call the polling function directly */
		(func)( arg );
		return;
	}

	int my_local_rank = sctk_get_local_task_rank();
	
	if( my_local_rank < 0 )
	{
		if( __last_local_id < 0 )
		{
			return;
		}
		else
		{
			my_local_rank = __last_local_id;
		}
	}
	else
	{
		/* We do this as some polling function
		 * do not have a context this way we preserve
		 * the topology anyway */
		__last_local_id = my_local_rank;
	}
	
	
	/* Translate the rank to a cell id */
	int cell_id = tree->local_task_id_to_cell_lut[ my_local_rank ];
	
	if( cell_id < 0 )
		return;
	
	__sctk_polling_tree_poll( tree, func, cell_id, my_local_rank, arg );
}


