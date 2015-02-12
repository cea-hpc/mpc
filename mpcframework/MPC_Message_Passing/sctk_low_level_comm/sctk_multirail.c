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
#include "sctk_multirail.h"
#include "sctk_low_level_comm.h"

/************************************************************************/
/* sctk_endpoint_list                                                   */
/************************************************************************/

int sctk_endpoint_list_init_entry( sctk_endpoint_list_t * entry, sctk_endpoint_t * endpoint )
{
	if( !endpoint )
	{
		sctk_error("No endpoint provided.. ignoring");
		return 1;
	}
	
	/* TO CHANGE */
	entry->priority = 1;
	
	if( !endpoint->rail )
	{
		sctk_error("No rail found in endpoint.. ignoring");
		return 1;	
	}
	
	entry->gate = endpoint->rail->gate;
	
	entry->endpoint = endpoint;

	entry->next = NULL;
	entry->prev = NULL;
	
	return 0;
}


sctk_endpoint_list_t * sctk_endpoint_list_new_entry( sctk_endpoint_t * endpoint )
{
	sctk_endpoint_list_t *  ret = sctk_malloc( sizeof( sctk_endpoint_list_t ) );
	
	assume( ret != NULL );
	
	if( sctk_endpoint_list_init_entry( ret, endpoint ) )
	{
		sctk_free( ret );
		return NULL;
	}	
	
	return ret;
}


sctk_endpoint_list_t * sctk_endpoint_list_push( sctk_endpoint_list_t * list, sctk_endpoint_t * endpoint )
{
	sctk_endpoint_list_t * table_start = list;
	sctk_endpoint_list_t * new = sctk_endpoint_list_new_entry( endpoint );

	if( !new )
	{
		sctk_error("Failed to insert a route.. ignoring");
		return list;
	}
	
	sctk_endpoint_list_t * insertion_point = table_start;
	
	/* If list already contains elements */
	if( insertion_point )
	{
	
		/* Go down the list according to priorities */
		while( 1 )
		{
			if( !insertion_point->next )
				break;
			
			if( insertion_point->next->priority < new->priority )
				break;
			
			insertion_point = insertion_point->next;
		}
		
		/* Here we know where we want to be inserted (after insertion_point)  */

		if( new->priority < insertion_point->priority )
		{
			/* Insert after */
			new->next = insertion_point->next;
			
			if( new->next )
			{
				new->next->prev = new;
			}
			
			new->prev = insertion_point;
			insertion_point->next = new;
		}
		else
		{
			/* Insert before (when there is a single entry) */
			new->prev = insertion_point->prev;
			new->next = insertion_point;
			
			if( new->prev )
			{
				new->prev->next = insertion_point;
			}
			else
			{
				/* We are list head, update accordingly */
				table_start = new;
			}
			
			insertion_point->prev = new;
			
		}
	}
	else
	{
		/* List is empty, become table start */
		table_start = new;
	}
	
	return table_start;
}

sctk_endpoint_list_t * sctk_endpoint_list_pop( sctk_endpoint_list_t * list, sctk_endpoint_list_t * topop )
{
	sctk_endpoint_list_t * table_start = list;
	
	if( topop->prev )
	{
		topop->prev->next = topop->next;
	}
	else
	{
		/* We were the first element (update table start) */
		table_start = topop->next;
	}
	
	if( topop->next )
	{
		topop->next->prev = topop->prev;
	}
	
	sctk_endpoint_list_free_entry( topop );
	
	return table_start;
}

sctk_endpoint_list_t * sctk_endpoint_list_pop_endpoint( sctk_endpoint_list_t * list, sctk_endpoint_t * topop )
{
	sctk_endpoint_list_t * topop_list_entry = NULL;
	
	while( list )
	{
		if( list->endpoint == topop )
		{
			topop_list_entry = list;
			break;
		}
	}
	
	if( !topop_list_entry )
	{
		sctk_warning("Could not find this entry in table");
		return list;
	}
	
	return sctk_endpoint_list_pop( list, topop_list_entry );
}

void sctk_endpoint_list_free_entry( sctk_endpoint_list_t * entry )
{
	if( ! entry )
		return;
	
	memset( entry, 0, sizeof( sctk_endpoint_list_t) );
	/* Release the entry */
	sctk_free( entry );
}

void sctk_endpoint_list_release( sctk_endpoint_list_t ** list )
{
	sctk_endpoint_list_t * tofree = NULL;
	sctk_endpoint_list_t * head = *list;

	while( head )
	{
		tofree = head;
		head = head->next;
		sctk_endpoint_list_free_entry( tofree );
	};

	*list = NULL;
}


/************************************************************************/
/* sctk_multirail_destination_table_entry                               */
/************************************************************************/

void sctk_multirail_destination_table_entry_init( sctk_multirail_destination_table_entry_t * entry, int destination )
{
	entry->endpoints = NULL;
	sctk_spin_rwlock_t lckinit = SCTK_SPIN_RWLOCK_INITIALIZER;
	entry->endpoints_lock = lckinit;
	entry->destination = destination;
}

void sctk_multirail_destination_table_entry_release( sctk_multirail_destination_table_entry_t * entry )
{
	if( ! entry )
		return;
	
	/* Make sure that we are not acquired */
	sctk_spinlock_write_lock( &entry->endpoints_lock );
	
	sctk_endpoint_list_release( &entry->endpoints );
	entry->destination = -1;
}

void sctk_multirail_destination_table_entry_free( sctk_multirail_destination_table_entry_t * entry )
{
	sctk_multirail_destination_table_entry_release( entry );
	sctk_free( entry );
}

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_entry_new(int destination)
{
	sctk_multirail_destination_table_entry_t * ret = sctk_malloc( sizeof( sctk_multirail_destination_table_entry_t) );
	
	assume( ret != NULL );
	
	sctk_multirail_destination_table_entry_init( ret, destination );
	
	return ret;
}

void sctk_multirail_destination_table_entry_push_endpoint( sctk_multirail_destination_table_entry_t * entry, sctk_endpoint_t * endpoint )
{
	sctk_spinlock_write_lock( &entry->endpoints_lock );
	entry->endpoints =  sctk_endpoint_list_push( entry->endpoints, endpoint );
	sctk_spinlock_write_unlock( &entry->endpoints_lock );
}

void sctk_multirail_destination_table_entry_pop_endpoint( sctk_multirail_destination_table_entry_t * entry, sctk_endpoint_t * endpoint )
{
	sctk_spinlock_write_lock( &entry->endpoints_lock );
	entry->endpoints =  sctk_endpoint_list_pop_endpoint( entry->endpoints, endpoint );
	sctk_spinlock_write_unlock( &entry->endpoints_lock );
}



/************************************************************************/
/* sctk_multirail_destination_table                                     */
/************************************************************************/


static struct sctk_multirail_destination_table ___sctk_multirail_table;

static inline struct sctk_multirail_destination_table * sctk_multirail_destination_table_get()
{
	return &___sctk_multirail_table;
}


/************************************************************************/
/* sctk_multirail_hooks                                                 */
/************************************************************************/

void sctk_multirail_send_message( sctk_thread_ptp_message_t *msg )
{
	sctk_multirail_destination_table_entry_t * routes = sctk_multirail_destination_table_acquire_routes( SCTK_MSG_DEST_PROCESS ( msg ) );
	
	if( ! routes )
	{
		sctk_error("NO ROUTE");
		CRASH();
		return;
	}
	
	
	sctk_endpoint_list_t *endpoints = routes->endpoints;
	
	sctk_endpoint_list_t *cur = endpoints;

	
	while( cur )
	{
		//sctk_warning("EDP : %d", cur->endpoint->dest);
		cur = cur->next;
	}
	
	(endpoints[0].endpoint->rail->send_message_endpoint)( msg, endpoints[0].endpoint );
	
	
	sctk_multirail_destination_table_relax_routes( routes );
}


void sctk_multirail_notify_receive( sctk_thread_ptp_message_t * msg )
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_recv_message )
		{
			(rail->notify_recv_message)( msg, rail );
		}
	}
}

void sctk_multirail_notify_matching( sctk_thread_ptp_message_t * msg )
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_matching_message )
		{
			(rail->notify_matching_message)( msg, rail );
		}
	}
}

void sctk_multirail_notify_perform( int remote, int remote_task_id, int polling_task_id, int blocking  )
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_perform_message )
		{
			(rail->notify_perform_message)( remote, remote_task_id, polling_task_id, blocking, rail );
		}
	}
}

void sctk_multirail_notify_idle()
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_idle_message )
		{
			(rail->notify_idle_message)( rail );
		}
	}
}

void sctk_multirail_notify_anysource( int polling_task_id, int blocking )
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_any_source_message )
		{
			(rail->notify_any_source_message)(polling_task_id, blocking, rail );
		}
	}
}




/************************************************************************/
/* sctk_multirail_destination_table                                     */
/************************************************************************/


void sctk_multirail_destination_table_init()
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	table->destinations = NULL;
	sctk_spin_rwlock_t lckinit = SCTK_SPIN_RWLOCK_INITIALIZER;
	table->table_lock = lckinit;
	
	/* Register Probing Calls */
	sctk_network_send_message_set ( sctk_multirail_send_message );
	
	sctk_network_notify_recv_message_set ( sctk_multirail_notify_receive );
	sctk_network_notify_matching_message_set ( sctk_multirail_notify_matching );
	sctk_network_notify_perform_message_set ( sctk_multirail_notify_perform );
	sctk_network_notify_idle_message_set ( sctk_multirail_notify_idle );
	sctk_network_notify_any_source_message_set ( sctk_multirail_notify_anysource );
	
	
}

void sctk_multirail_destination_table_release()
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * entry;
	sctk_multirail_destination_table_entry_t * to_free;

	sctk_spinlock_write_lock( &table->table_lock );


	HASH_ITER(hh, table->destinations, to_free, entry)
	{
		HASH_DEL( table->destinations, to_free);
		sctk_multirail_destination_table_entry_free( to_free );
	}
	
}

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_acquire_routes(int destination )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;
	
	sctk_spinlock_read_lock( &table->table_lock );
	
	//sctk_warning("GET endpoint to %d", destination );
	
	HASH_FIND_INT(table->destinations, &destination, dest_entry);
	
	if( dest_entry )
	{
		/* Acquire entries in read */
		sctk_spinlock_read_lock( &dest_entry->endpoints_lock );
	}
	
	
	sctk_spinlock_read_unlock( &table->table_lock );
	
	return dest_entry;
}

void sctk_multirail_destination_table_relax_routes( sctk_multirail_destination_table_entry_t * entry )
{
	if(!entry)
		return;
	
	/* Just relax the read lock inside a previously acquired entry */
	sctk_spinlock_read_unlock( &entry->endpoints_lock );
}

void sctk_multirail_destination_table_push_endpoint(sctk_endpoint_t * endpoint )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;
	
	sctk_warning("Push endpoint to %d", endpoint->dest );
	
	sctk_spinlock_write_lock( &table->table_lock );
	
	HASH_FIND_INT(table->destinations, &endpoint->dest, dest_entry);
	
	if( !dest_entry )
	{
		sctk_warning("New endpoint");
		/* There is no previous destination_table_entry */
		dest_entry = sctk_multirail_destination_table_entry_new(endpoint->dest);
		HASH_ADD_INT( table->destinations, destination, dest_entry );
	}
	
	sctk_multirail_destination_table_entry_push_endpoint( dest_entry, endpoint );
	
	sctk_spinlock_write_unlock( &table->table_lock );
	
}

void sctk_multirail_destination_table_pop_endpoint( sctk_endpoint_t * topop )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;
	
	sctk_spinlock_write_lock( &table->table_lock );
	
	HASH_FIND_INT(table->destinations, &topop->dest, dest_entry);
	
	if( dest_entry )
	{
		sctk_multirail_destination_table_entry_pop_endpoint( dest_entry, topop );
	}
	
	sctk_spinlock_write_unlock( &table->table_lock );	
}


