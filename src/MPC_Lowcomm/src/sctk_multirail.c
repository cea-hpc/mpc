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

#include <stdlib.h>

#include <sctk_alloc.h>


/************************************************************************/
/* Rail Gates                                                           */
/************************************************************************/

/* HERE ARE DEFAULT GATES */

int sctk_rail_gate_boolean( __UNUSED__ sctk_rail_info_t * rail, __UNUSED__ mpc_lowcomm_ptp_message_t * message , void * gate_config )
{
	struct _mpc_lowcomm_config_struct_gate_boolean * conf = (struct _mpc_lowcomm_config_struct_gate_boolean *)gate_config;
	return conf->value;
}

int sctk_rail_gate_probabilistic(  __UNUSED__ sctk_rail_info_t * rail, __UNUSED__ mpc_lowcomm_ptp_message_t * message , void * gate_config )
{
	struct _mpc_lowcomm_config_struct_gate_probabilistic * conf = (struct _mpc_lowcomm_config_struct_gate_probabilistic *)gate_config;
	
	int num = ( rand() % 100 );

	return ( num < conf->probability );
}

int sctk_rail_gate_minsize( __UNUSED__ sctk_rail_info_t * rail, mpc_lowcomm_ptp_message_t * message , void * gate_config )
{
	struct _mpc_lowcomm_config_struct_gate_min_size * conf = (struct _mpc_lowcomm_config_struct_gate_min_size *)gate_config;
	
	size_t message_size = SCTK_MSG_SIZE( message );
	
	return ( conf->value < message_size );
}

int sctk_rail_gate_maxsize( __UNUSED__ sctk_rail_info_t * rail, mpc_lowcomm_ptp_message_t * message , void * gate_config )
{
	struct _mpc_lowcomm_config_struct_gate_max_size * conf = (struct _mpc_lowcomm_config_struct_gate_max_size *)gate_config;
	
	size_t message_size = SCTK_MSG_SIZE( message );
	
	return ( message_size < conf->value );
}


int sctk_rail_gate_msgtype( __UNUSED__ sctk_rail_info_t * rail, mpc_lowcomm_ptp_message_t * message , void * gate_config )
{
	struct _mpc_lowcomm_config_struct_gate_message_type * conf = (struct _mpc_lowcomm_config_struct_gate_message_type *)gate_config;
	
	int is_process_specific = sctk_is_process_specific_message ( SCTK_MSG_HEADER ( message ) );
        int tag = SCTK_MSG_TAG(message);

        mpc_lowcomm_ptp_message_class_t class =
            (SCTK_MSG_HEADER(message))->message_type.type;

        /* It is a emulated RMA and it is not allowed */
        if ((class == MPC_LOWCOMM_RDMA_MESSAGE) && !conf->emulated_rma) {
          return 0;
        }

        /* It is a task level control  message and it is not allowed */
        if (((class == MPC_LOWCOMM_CONTROL_MESSAGE_TASK) || (tag == 16008)) &&
            !conf->task) {
          return 0;
        }
        /* It is a process specific message
         * and it is allowed */
        if (is_process_specific && conf->process)
          return 1;

        /* It is a common message and it is allowed */
        if (!is_process_specific && conf->common)
          return 1;

        return 0;
}

struct sctk_gate_context
{
	int (*func)( sctk_rail_info_t * rail, mpc_lowcomm_ptp_message_t * message , void * gate_config );
	void * params;
};


static inline void sctk_gate_get_context( struct _mpc_lowcomm_config_struct_net_gate * gate , struct sctk_gate_context * ctx )
{
	ctx->func = NULL;
	ctx->params = NULL;

	if( ! gate )
		return;

	switch( gate->type )
	{
		case MPC_CONF_RAIL_GATE_BOOLEAN:
			ctx->func = sctk_rail_gate_boolean;
			ctx->params = (void *)&gate->value.boolean;
		break;
		case MPC_CONF_RAIL_GATE_PROBABILISTIC:
			ctx->func = sctk_rail_gate_probabilistic;
			ctx->params = (void *)&gate->value.probabilistic;
		break;
		case MPC_CONF_RAIL_GATE_MINSIZE:
			ctx->func = sctk_rail_gate_minsize;
			ctx->params = (void *)&gate->value.minsize;		
		break;
		case MPC_CONF_RAIL_GATE_MAXSIZE:
			ctx->func = sctk_rail_gate_maxsize;
			ctx->params = (void *)&gate->value.maxsize;			
		break;
		case MPC_CONF_RAIL_GATE_USER:
			ctx->func = (int (*)( sctk_rail_info_t *, mpc_lowcomm_ptp_message_t *, void *))gate->value.user.gatefunc.value;
			if(!ctx->func)
			{
				bad_parameter("Could not resolve user-defined rail gate function %s == %p", gate->value.user.gatefunc.name, gate->value.user.gatefunc.value);
			}
			ctx->params = (void *)&gate->value.user;		
		break;
		
		case MPC_CONF_RAIL_GATE_MSGTYPE:
			ctx->func = sctk_rail_gate_msgtype;
			ctx->params = (void *)&gate->value.msgtype;		
		break;
		
		case MPC_CONF_RAIL_GATE_NONE:
		default:
			mpc_common_debug_fatal("No such gate type");
	}
}

/************************************************************************/
/* _mpc_lowcomm_endpoint_list                                                   */
/************************************************************************/

int _mpc_lowcomm_endpoint_list_init_entry( _mpc_lowcomm_endpoint_list_t * entry, _mpc_lowcomm_endpoint_t * endpoint )
{
	if( !endpoint )
	{
		mpc_common_debug_error("No endpoint provided.. ignoring");
		return 1;
	}
	
	/* TO CHANGE */
	entry->priority = endpoint->rail->runtime_config_rail->priority;
	
	if( !endpoint->rail )
	{
		mpc_common_debug_error("No rail found in endpoint.. ignoring");
		return 1;	
	}
	
	entry->gates = endpoint->rail->runtime_config_rail->gates;
	entry->gate_count = endpoint->rail->runtime_config_rail->gates_size;
	
	entry->endpoint = endpoint;

	entry->next = NULL;
	entry->prev = NULL;
	
	return 0;
}


_mpc_lowcomm_endpoint_list_t * _mpc_lowcomm_endpoint_list_new_entry( _mpc_lowcomm_endpoint_t * endpoint )
{
	_mpc_lowcomm_endpoint_list_t *  ret = sctk_malloc( sizeof( _mpc_lowcomm_endpoint_list_t ) );
	
	assume( ret != NULL );
	
	if( _mpc_lowcomm_endpoint_list_init_entry( ret, endpoint ) )
	{
		sctk_free( ret );
		return NULL;
	}	
	
	return ret;
}


_mpc_lowcomm_endpoint_list_t * _mpc_lowcomm_endpoint_list_push( _mpc_lowcomm_endpoint_list_t * list, _mpc_lowcomm_endpoint_t * endpoint )
{
	_mpc_lowcomm_endpoint_list_t * table_start = list;
	_mpc_lowcomm_endpoint_list_t * new = _mpc_lowcomm_endpoint_list_new_entry( endpoint );

	if( !new )
	{
		mpc_common_debug_error("Failed to insert a route.. ignoring");
		return list;
	}
	
	_mpc_lowcomm_endpoint_list_t * insertion_point = table_start;
	
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
				new->prev->next = new;
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

_mpc_lowcomm_endpoint_list_t * _mpc_lowcomm_endpoint_list_pop( _mpc_lowcomm_endpoint_list_t * list, _mpc_lowcomm_endpoint_list_t * topop )
{
	_mpc_lowcomm_endpoint_list_t * table_start = list;
	
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
	
	_mpc_lowcomm_endpoint_list_free_entry( topop );
	
	return table_start;
}

_mpc_lowcomm_endpoint_list_t * _mpc_lowcomm_endpoint_list_pop_endpoint( _mpc_lowcomm_endpoint_list_t * list, _mpc_lowcomm_endpoint_t * topop )
{
	_mpc_lowcomm_endpoint_list_t * topop_list_entry = NULL;
	
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
		mpc_common_debug_warning("Could not find this entry in table");
		return list;
	}
	
	return _mpc_lowcomm_endpoint_list_pop( list, topop_list_entry );
}

void _mpc_lowcomm_endpoint_list_free_entry( _mpc_lowcomm_endpoint_list_t * entry )
{
	if( ! entry )
		return;
	
	memset( entry, 0, sizeof( _mpc_lowcomm_endpoint_list_t) );
	/* Release the entry */
	sctk_free( entry );
}

void _mpc_lowcomm_endpoint_list_release( _mpc_lowcomm_endpoint_list_t ** list )
{
	_mpc_lowcomm_endpoint_list_t * tofree = NULL;
	_mpc_lowcomm_endpoint_list_t * head = *list;

	while( head )
	{
		tofree = head;
		head = head->next;
		_mpc_lowcomm_endpoint_list_free_entry( tofree );
	};

	*list = NULL;
}

void _mpc_lowcomm_endpoint_list_prune( _mpc_lowcomm_endpoint_list_t ** list)
{
	_mpc_lowcomm_endpoint_list_t *entry = *list;
	_mpc_lowcomm_endpoint_list_t *tofree = NULL;
	_mpc_lowcomm_endpoint_t *route;

	while( entry )
	{
		route = entry->endpoint;

		tofree = entry;
		entry = entry->next;
		
		if(route->rail->state == SCTK_RAIL_ST_DISABLED)
		{
			*list = _mpc_lowcomm_endpoint_list_pop(*list, tofree);
		}
	}
}

/************************************************************************/
/* sctk_multirail_destination_table_entry                               */
/************************************************************************/

void sctk_multirail_destination_table_entry_init( sctk_multirail_destination_table_entry_t * entry, int destination )
{
	entry->endpoints = NULL;
	mpc_common_rwlock_t lckinit = SCTK_SPIN_RWLOCK_INITIALIZER;
	entry->endpoints_lock = lckinit;
	entry->destination = destination;
}

void sctk_multirail_destination_table_entry_release( sctk_multirail_destination_table_entry_t * entry )
{
	if( ! entry )
		return;
	
	/* Make sure that we are not acquired */
	mpc_common_spinlock_write_lock( &entry->endpoints_lock );
	
	_mpc_lowcomm_endpoint_list_release( &entry->endpoints );
	entry->destination = -1;
}

void sctk_multirail_destination_table_entry_free( sctk_multirail_destination_table_entry_t * entry )
{
	sctk_multirail_destination_table_entry_release( entry );
	sctk_free( entry );
}

void sctk_multirail_destination_table_entry_prune(sctk_multirail_destination_table_entry_t * entry)
{
	mpc_common_spinlock_write_lock(&entry->endpoints_lock);
	_mpc_lowcomm_endpoint_list_prune(&entry->endpoints);
	mpc_common_spinlock_write_unlock(&entry->endpoints_lock);
}

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_entry_new(int destination)
{
	sctk_multirail_destination_table_entry_t * ret = sctk_malloc( sizeof( sctk_multirail_destination_table_entry_t) );
	
	assume( ret != NULL );
	
	sctk_multirail_destination_table_entry_init( ret, destination );
	
	return ret;
}

void sctk_multirail_destination_table_entry_push_endpoint( sctk_multirail_destination_table_entry_t * entry, _mpc_lowcomm_endpoint_t * endpoint )
{
	mpc_common_spinlock_write_lock( &entry->endpoints_lock );
	entry->endpoints =  _mpc_lowcomm_endpoint_list_push( entry->endpoints, endpoint );
	mpc_common_spinlock_write_unlock( &entry->endpoints_lock );
}

void sctk_multirail_destination_table_entry_pop_endpoint( sctk_multirail_destination_table_entry_t * entry, _mpc_lowcomm_endpoint_t * endpoint )
{
	mpc_common_spinlock_write_lock( &entry->endpoints_lock );
	entry->endpoints =  _mpc_lowcomm_endpoint_list_pop_endpoint( entry->endpoints, endpoint );
	mpc_common_spinlock_write_unlock( &entry->endpoints_lock );
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
/* Endpoint Election Logic                                              */
/************************************************************************/
/**
 * Choose the endpoint to use for sending the message.
 * This function will try to find a route to send the message. It it does not find
 * a direct link, NULL is returned.
 * \param[in] msg the message to send
 * \param[in] destination_process the remote process
 * \param[in] is_process_specific is the message targeting the process (not an MPI message) ?
 * \param[in] is_for_on_demand is a on-demand request ?
 * \param[in] ext_routes route array to iterate with (will be locked).
 * \return a pointer to the matching route, NULL otherwise (the route lock will NOT be hold in case of match)
 */
_mpc_lowcomm_endpoint_t * sctk_multirail_ellect_endpoint( mpc_lowcomm_ptp_message_t *msg, int destination_process, int is_process_specific, int is_for_on_demand, sctk_multirail_destination_table_entry_t ** ext_routes )
{
	
	sctk_multirail_destination_table_entry_t * routes = sctk_multirail_destination_table_acquire_routes( destination_process );
	*ext_routes = routes;

	_mpc_lowcomm_endpoint_list_t * cur = NULL;
	
	if( routes )
	{
		cur = routes->endpoints;
	}
	
	/* Note that in the case of process specific messages
	 * we return the first matching route which is the
	 * one of Highest priority */
	while( cur && !is_process_specific )
	{
		int this_rail_has_been_elected = 1;
		
		int cur_gate;
		
		
		/* First we want only to check on-demand rails */
		if( !cur->endpoint->rail->connect_on_demand && is_for_on_demand )
		{
			/* No on-demand in this rail */
			this_rail_has_been_elected = 0;
		}
		else
		{
			/* If it is connecting just wait for
			* the connection completion */
			int wait_connecting;
			 
			do
			{
				if(  _mpc_lowcomm_endpoint_get_state ( cur->endpoint ) == _MPC_LOWCOMM_ENDPOINT_CONNECTING )
				{
					wait_connecting = 1;
					sctk_network_notify_idle_message ();
					sched_yield();
				}
				else
				{
					wait_connecting = 0;
				}

			}while(wait_connecting);
			
			
			/* Now that we wait for the connection only try an endpoint if it is connected */
			if( _mpc_lowcomm_endpoint_get_state ( cur->endpoint ) == _MPC_LOWCOMM_ENDPOINT_CONNECTED )
			{
				/* Note that no gate is equivalent to being elected */
				for( cur_gate = 0 ; cur_gate < cur->gate_count ; cur_gate++ )
				{
					struct sctk_gate_context gate_ctx;
					sctk_gate_get_context( &cur->gates[ cur_gate ] , &gate_ctx );
					
					if( ( gate_ctx.func )( cur->endpoint->rail, msg, gate_ctx.params ) == 0 )
					{
						this_rail_has_been_elected = 0;
						break;
					}
					
				}
			}
			else
			{	
				/* If the rail is not connected it
				 * cannot be elected yet */
				this_rail_has_been_elected = 0;
			}
		}
		
		if( this_rail_has_been_elected )
		{
			break;
		}
		
		cur = cur->next;
	}
	
	if( !cur )
		return NULL;
	
	
	return cur->endpoint;
}



mpc_common_spinlock_t on_demand_connection_lock = SCTK_SPINLOCK_INITIALIZER;

/* Per VP pending on-demand connection list */
static __thread sctk_pending_on_demand_t * __pending_on_demand = NULL;

void sctk_pending_on_demand_push( sctk_rail_info_t * rail, int dest )
{
	/* Is the rail on demand ? */
	if( !rail->connect_on_demand )
	{
		/* No then nothing to do */
		return;
	}
	
	sctk_pending_on_demand_t * new = sctk_malloc( sizeof(  struct sctk_pending_on_demand_s ) );
	assume( new != NULL );
	
	new->next = (struct sctk_pending_on_demand_s *)__pending_on_demand;

	new->dest = dest;
	new->rail = rail;
	
	__pending_on_demand = new;
}


void sctk_pending_on_demand_release( sctk_pending_on_demand_t * pod )
{
	sctk_free( pod );
}

sctk_pending_on_demand_t * sctk_pending_on_demand_get()
{
	sctk_pending_on_demand_t * ret = __pending_on_demand;
	
	if( ret )
	{
		__pending_on_demand = ret->next;
	}
	
	return ret;
}


void sctk_pending_on_demand_process()
{
	sctk_pending_on_demand_t * pod = sctk_pending_on_demand_get();
	
	while( pod )
	{
		/* Check if the endpoint exist in the rail */
		_mpc_lowcomm_endpoint_t * previous_endpoint = sctk_rail_get_any_route_to_process (  pod->rail, pod->dest );
		
		/* No endpoint ? then its worth locking */
		if( !previous_endpoint )
		{
			mpc_common_spinlock_lock( & on_demand_connection_lock );
			
			/* Check again to avoid RC */
			previous_endpoint = sctk_rail_get_any_route_to_process (  pod->rail, pod->dest );
			
			/* We can create it */
			if( !previous_endpoint )
			{
				(pod->rail->connect_on_demand)( pod->rail,  pod->dest );
			}
			
			mpc_common_spinlock_unlock( & on_demand_connection_lock );
		}
		
		sctk_pending_on_demand_t * to_free = pod;
		/* Get next */
		pod = sctk_pending_on_demand_get();
		/* Free previous */
		sctk_pending_on_demand_release( to_free );
	}
}

/** 
 * Called to create a new route for the sending the given message.
 * \param[in] msg the message to send.
 */
void sctk_multirail_on_demand_connection( mpc_lowcomm_ptp_message_t *msg )
{
	
	int count = sctk_rail_count();
	int i;
	
	int tab[64];
	assume( count < 64 );
	
	memset( tab, 0, 64 * sizeof( int ) );
	
	/* Lets now test each rail to find the one accepting this 
	 * message while being able to setup on demand-connections */
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		
		/* First we want only to check on-demand rails */
		if( !rail->connect_on_demand )
		{
			/* No on-demand in this rail */
			continue;
		}
		
		/* ###############################################################################
		 * Now we test gate functions to detect if this message is elligible on this rail
		 * ############################################################################### */
		
		/* Retrieve data from CTX */
		struct _mpc_lowcomm_config_struct_net_gate * gates = rail->runtime_config_rail->gates;
		int gate_count = rail->runtime_config_rail->gates_size;
		
		int priority = rail->runtime_config_rail->priority;
		
		int this_rail_has_been_elected = 1;
		
		/* Test all gates on this rail */
		int j;
		for( j = 0 ; j < gate_count; j++ )
		{
			struct sctk_gate_context gate_ctx;
			sctk_gate_get_context( &gates[ j ], &gate_ctx );

			if( ( gate_ctx.func )( rail, msg, gate_ctx.params ) == 0 )
			{
				/* This gate does not pass */
				this_rail_has_been_elected = 0;
				break;
			}
		
		}
		
		/* ############################################################################### */
		
		/* If we are here the rail is ellected */
		
		if( this_rail_has_been_elected )
		{
			/* This rail passed the test save its priority 
			 * to prepare the next phase of prority selection */
			tab[i] = priority;
		}
	}
	
	/* Lets now connect to the rails passing the test with the highest priority */
	int current_max = 0;
	int max_offset = -1;

	for( i = 0 ; i < count ; i++ )
	{
		if( ! tab[i] )
			continue;
		
		if( current_max <= tab[i] )
		{
			/* This is the new BEST */
			current_max = tab[i]; 
			max_offset = i;
		}
		
	}


	if( max_offset < 0 )
	{
		/* No rail found */
		mpc_common_debug_fatal("No route to host == Make sure you have at least one on-demand rail able to satify any type of message");
	}

	/* Enter the critical section to guanrantee the uniqueness of the 
	 * newly created rail by first checking if it is not already present */
	mpc_common_spinlock_lock( & on_demand_connection_lock );

	int dest_process = SCTK_MSG_DEST_PROCESS ( msg );

	/* First retry to acquire a route for on-demand
	 * in order to avoid double connections */
	sctk_multirail_destination_table_entry_t * routes = NULL;
	_mpc_lowcomm_endpoint_t * previous_endpoint = sctk_multirail_ellect_endpoint( msg, dest_process, 0 /* Not Process Specific */, 1 /* For On-Demand */ , &routes );

	/* We need to relax the routes before pushing the endpoint as the new entry
	 * will end in the same endpoint list */ 	
	sctk_multirail_destination_table_relax_routes( routes );
	

	/* Check if no previous */
	if( ! previous_endpoint )
	{
		/* If we are here we have elected the on-demand rail with the highest priority and matching this message
		 * initiate the on-demand connection */
		sctk_rail_info_t * elected_rail = sctk_rail_get_by_id ( max_offset );
		(elected_rail->connect_on_demand)( elected_rail,  dest_process );
	}

	mpc_common_spinlock_unlock( & on_demand_connection_lock );
}


/************************************************************************/
/* sctk_multirail_hooks                                                 */
/************************************************************************/
/**
 * Main entry point in low_level_comm for sending a message.
 * \param[in] msg the message to send.
 */
void sctk_multirail_send_message( mpc_lowcomm_ptp_message_t *msg )
{
	int retry;
	int destination_process;
	
	int is_process_specific = sctk_is_process_specific_message ( SCTK_MSG_HEADER ( msg ) );
	
	/* If the message is based on signalization we directly rely on routing */
	if( is_process_specific )
	{
		/* Find the process to which to route to */
		sctk_multirail_destination_table_route_to_process( SCTK_MSG_DEST_PROCESS ( msg ), &destination_process );
	}
	else
	{
		/* We want to reach the desitination of the Message */
		destination_process = SCTK_MSG_DEST_PROCESS ( msg );
	}

	int no_existing_route_matched = 0;

	do
	{
		retry = 0;
		
		if( no_existing_route_matched )
		{
			/* We have to do an on-demand connection */
			sctk_multirail_on_demand_connection( msg );
		}
		
		/* We need to retrieve the route entry in order to be
		 * able to relax it after use */
		sctk_multirail_destination_table_entry_t * routes = NULL;
		
		_mpc_lowcomm_endpoint_t * endpoint = sctk_multirail_ellect_endpoint( msg, destination_process, is_process_specific, 0 /* Not for On-Demand */ , &routes );
		
		if( endpoint )
		{
			sctk_rail_info_t * target_rail = endpoint->rail;
			
			/* Override by parent if present (topological rail) */
			if( endpoint->parent_rail != NULL )
				target_rail = endpoint->parent_rail;
			
			mpc_common_nodebug("RAIL %d", target_rail->rail_number );
			
			/* Prepare reordering */
			sctk_prepare_send_message_to_network_reorder ( msg );

			/* Send the message */
			(target_rail->send_message_endpoint)( msg, endpoint );
		}
		else
		{
			/* Here we found no route to the process or
			 * none mathed this message, we have to retry
			 * while creating a new connection */
			no_existing_route_matched = 1;
			retry = 1;
		}

		/* Relax Routes */
		sctk_multirail_destination_table_relax_routes( routes );

	}while( retry );
	
	/* Before Leaving Make sure that there are no pending on-demand
	 * connections (addeb by the topological rail while holding routes) */
	sctk_pending_on_demand_process();
}

/**
 * Main entry-point for notifying rails a new RECV has been posted.
 * ALL RAILS WILL BE NOTIFIED (not only the one that may be the real recever).
 * \param[in] msg the locally-posted RECV.
 */
void sctk_multirail_notify_receive( mpc_lowcomm_ptp_message_t * msg )
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

void sctk_multirail_notify_matching( mpc_lowcomm_ptp_message_t * msg )
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

static void sctk_multirail_notify_probe  (mpc_lowcomm_ptp_message_header_t* hdr, int *status)
{
	int count = sctk_rail_count();
	int i;
	int ret = 0, tmp_ret = -1;
	static int no_rail_support = 0;

	if(no_rail_support) /* shortcut when no rail support probing at all */
        {
                *status = -1;
                return;
        }

	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		tmp_ret = -1; /* re-init for next rail */
		
		/* if current driver can handle a probing procedure... */
		if( rail->notify_probe_message )
		{
			/* possible values of tmp_ret:
			 * -1 -> driver-specific probing not handled (either because no function pointer set or the function returns -1)
			 * 0 -> No matching message found
			 * 1 -> At least one message found based on requirements
			 */
			(rail->notify_probe_message)( rail, hdr, &tmp_ret);
		}
		
		assert(tmp_ret == -1 || tmp_ret == 1 || tmp_ret == 0);
		
		/* three scenarios based on that :
		 * - If a rail found a matching message -> returns 1
		 * - If all rails support probing, and returns 0 -> returns 0
		 * - If at least one rail returns -1 AND probing failed -> returns -1, meaning that the previous behavior (logical header lookup through perform()) should be run.
		 */
		if(tmp_ret == 1) /* found */
		{
			*status = 1;
			return;
			
		}
		ret += tmp_ret;
	}
	*status = ret;
	if(ret == -count) /* not a single rail support probing: stop running this function */
        {
                no_rail_support = 1;
        }
}

void sctk_multirail_notify_idle()
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->state == SCTK_RAIL_ST_ENABLED && rail->notify_idle_message )
		{
		    (rail->notify_idle_message)(rail);
        }
	}
}


struct sctk_anysource_polling_ctx
{
	int polling_task_id;
	int blocking;
	sctk_rail_info_t * rail;
};


void notify_anysource_trampoline( void * pctx )
{
	struct sctk_anysource_polling_ctx * ctx = (struct sctk_anysource_polling_ctx *) pctx;
	(ctx->rail->notify_any_source_message)( ctx->polling_task_id, ctx->blocking, ctx->rail );
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
			struct sctk_anysource_polling_ctx ctx;
			
			ctx.polling_task_id = polling_task_id;
			ctx.blocking = blocking;
			ctx.rail = rail;
			
			sctk_topological_polling_tree_poll( &rail->any_source_polling_tree , notify_anysource_trampoline , (void *)&ctx );
		}
	}
}

void sctk_multirail_notify_new_comm(int comm_idx, size_t size)
{
	int count = sctk_rail_count();
	int i;
	
	for( i = 0 ; i < count ; i++ )
	{
		sctk_rail_info_t * rail = sctk_rail_get_by_id ( i );
		
		if( rail->notify_new_comm )
		{
			rail->notify_new_comm(rail, comm_idx, size);
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
	mpc_common_rwlock_t lckinit = SCTK_SPIN_RWLOCK_INITIALIZER;
	table->table_lock = lckinit;
	
	/* Register Probing Calls */
	sctk_network_send_message_set ( sctk_multirail_send_message );
	
	sctk_network_notify_recv_message_set ( sctk_multirail_notify_receive );
	sctk_network_notify_matching_message_set ( sctk_multirail_notify_matching );
	sctk_network_notify_perform_message_set ( sctk_multirail_notify_perform );
	sctk_network_notify_idle_message_set ( sctk_multirail_notify_idle );
	sctk_network_notify_any_source_message_set ( sctk_multirail_notify_anysource );
	sctk_network_notify_new_communicator_set( sctk_multirail_notify_new_comm );
	sctk_network_notify_probe_message_set( sctk_multirail_notify_probe );
	
	
}

void sctk_multirail_destination_table_release()
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * entry;
	sctk_multirail_destination_table_entry_t * to_free;

	mpc_common_spinlock_write_lock( &table->table_lock );


	HASH_ITER(hh, table->destinations, to_free, entry)
	{
		HASH_DEL( table->destinations, to_free);
		sctk_multirail_destination_table_entry_free( to_free );
	}
	
}

void sctk_multirail_destination_table_prune(void)
{
	struct sctk_multirail_destination_table* table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t *entry = NULL, *tmp = NULL;

	mpc_common_spinlock_write_lock( &table->table_lock);
        
	HASH_ITER(hh, table->destinations, entry, tmp)
	{
		sctk_multirail_destination_table_entry_prune(entry);
	}

	mpc_common_spinlock_write_unlock(&table->table_lock);
}

sctk_multirail_destination_table_entry_t * sctk_multirail_destination_table_acquire_routes(int destination )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;
	
	mpc_common_spinlock_read_lock( &table->table_lock );
	
	//mpc_common_debug_warning("GET endpoint to %d", destination );
	
	HASH_FIND_INT(table->destinations, &destination, dest_entry);
	
	if( dest_entry )
	{
		/* Acquire entries in read */
		mpc_common_spinlock_read_lock( &dest_entry->endpoints_lock );
	}
	
	mpc_common_spinlock_read_unlock( &table->table_lock );
	
	return dest_entry;
}

void sctk_multirail_destination_table_relax_routes( sctk_multirail_destination_table_entry_t * entry )
{
	if(!entry)
		return;
	
	/* Just relax the read lock inside a previously acquired entry */
	mpc_common_spinlock_read_unlock( &entry->endpoints_lock );
}

void sctk_multirail_destination_table_push_endpoint(_mpc_lowcomm_endpoint_t * endpoint )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;

	mpc_common_spinlock_write_lock( &table->table_lock );
	
	HASH_FIND_INT(table->destinations, &endpoint->dest, dest_entry);
	
	if( !dest_entry )
	{
		/* There is no previous destination_table_entry */
		dest_entry = sctk_multirail_destination_table_entry_new(endpoint->dest);
		HASH_ADD_INT( table->destinations, destination, dest_entry );
	}
	
        sctk_multirail_destination_table_entry_push_endpoint( dest_entry, endpoint );
	
	mpc_common_spinlock_write_unlock( &table->table_lock );
	
}

void sctk_multirail_destination_table_pop_endpoint( _mpc_lowcomm_endpoint_t * topop )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * dest_entry = NULL;
	
	mpc_common_spinlock_write_lock( &table->table_lock );
	
	HASH_FIND_INT(table->destinations, &topop->dest, dest_entry);
	
	if( dest_entry )
	{
		sctk_multirail_destination_table_entry_pop_endpoint( dest_entry, topop );
	}
	
	mpc_common_spinlock_write_unlock( &table->table_lock );	
}

/**
 * Compute the closest neighbor to the final destination.
 * This function is called when an on-demand route cannot be crated or a CM message
 * is sent but no direct routes exist. It assumes that at least one rail has a connected (no singleton)
 * and that the topology is at least based on a ring (be aware of potential deadlocks if not)
 *
 * The distance between the destination and the new destination is computed by substracting
 * the final destination rank by the best intermediate one. The destination with the lowest result will
 * be elected.
 *
 * \param[in] the final destination process to reach
 * \param[out] new_destination the next process to target (can be the final destination)
 */
void sctk_multirail_destination_table_route_to_process( int destination, int * new_destination )
{
	struct sctk_multirail_destination_table * table = sctk_multirail_destination_table_get();
	sctk_multirail_destination_table_entry_t * tmp;
	sctk_multirail_destination_table_entry_t * entry;

	int distance = -1;

	mpc_common_spinlock_read_lock( &table->table_lock );


	HASH_ITER(hh, table->destinations, entry, tmp)
	{
		/* Only test connected endpoint as some networks
		 * might create the endpoint before sending
		 * the control message to the target process.
		 * In such case it would be selected (dist == 0)
		 * but would not be connected ! */
	
		
		if( entry->endpoints )
		{
			
			mpc_common_nodebug("STATE %d == %d", entry->destination, _mpc_lowcomm_endpoint_get_state ( entry->endpoints->endpoint ) );
	
			if( _mpc_lowcomm_endpoint_get_state ( entry->endpoints->endpoint ) == _MPC_LOWCOMM_ENDPOINT_CONNECTED )
			{
			
				mpc_common_nodebug( " %d --- %d" , destination, entry->destination  );
				if( destination == entry->destination )
				{
					distance = 0;
					*new_destination = destination;
					break;
				}
				
				int cdistance = entry->destination - destination;
				
				if( cdistance < 0 )
					cdistance = -cdistance;
				
				if( ( distance < 0 ) || (cdistance < distance ) )
				{
					distance = cdistance;
					*new_destination = entry->destination;
				}
			
			}
		
		}
	}
	
	mpc_common_spinlock_read_unlock( &table->table_lock );
	
	if( distance == -1 )
	{
		mpc_common_debug_warning("No route to host %d ", destination);
	}
	
}
