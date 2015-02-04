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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include <sctk.h>
#include <sctk_debug.h>
#include <sctk_route.h>
#include <sctk_reorder.h>
#include <sctk_communicator.h>
#include <sctk_spinlock.h>
#include <sctk_ib_cm.h>
#include <sctk_low_level_comm.h>
#include "sctk_ib_qp.h"
#include <utarray.h>


/************************************************************************/
/* Routes Storage                                                       */
/************************************************************************/

/** Here are stored the dynamic routes (hash table) */
static sctk_endpoint_t *sctk_dynamic_route_table = NULL;
/** This is the dynamic route lock */
static sctk_spin_rwlock_t sctk_route_table_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

/** Here are stored the dynamic routes (hash table) -- no lock as they are read only */
static sctk_endpoint_t *sctk_static_route_table = NULL;

/** This conditionnal lock is used during route initialization */
static sctk_spin_rwlock_t sctk_route_table_init_lock = SCTK_SPIN_RWLOCK_INITIALIZER;


typedef struct sctk_route_table_s
{
	/* Dynamic Routes */
	sctk_endpoint_t * dynamic_route_table; /** Here are stored the dynamic routes (hash table) */
	sctk_spin_rwlock_t dynamic_route_table_lock; /** This is the dynamic route lock */
	/* Static Routes */
	sctk_endpoint_t *static_route_table; /** Here are stored static routes (hash table) -- no lock as they are read only */
}sctk_route_table_t;


void sctk_route_table_init( sctk_route_table_t * table )
{
	table->dynamic_route_table = NULL;
	sctk_spin_rwlock_t lck = SCTK_SPIN_RWLOCK_INITIALIZER;

	table->dynamic_route_table_lock = lck;
	table->static_route_table = NULL;	
}


/************************************************************************/
/* Routes Memory Status                                                 */
/************************************************************************/

int sctk_route_cas_low_memory_mode_local ( sctk_endpoint_t *tmp, int oldv, int newv )
{
	return ( int ) OPA_cas_int ( &tmp->low_memory_mode_local, oldv, newv );
}

int sctk_route_is_low_memory_mode_local ( sctk_endpoint_t *tmp )
{
	return ( int ) OPA_load_int ( &tmp->low_memory_mode_local );
}
void sctk_route_set_low_memory_mode_local ( sctk_endpoint_t *tmp, int low )
{
	if ( low )
		sctk_warning ( "Local process %d set to low level memory", tmp->key.destination );

	OPA_store_int ( &tmp->low_memory_mode_local, low );
}
int sctk_route_is_low_memory_mode_remote ( sctk_endpoint_t *tmp )
{
	return ( int ) OPA_load_int ( &tmp->low_memory_mode_remote );
}

void sctk_route_set_low_memory_mode_remote ( sctk_endpoint_t *tmp, int low )
{
	if ( low )
		sctk_warning ( "Remote process %d set to low level memory", tmp->key.destination );

	OPA_store_int ( &tmp->low_memory_mode_remote, low );
}

/************************************************************************/
/* Route Getters                                                        */
/************************************************************************/

/*
 * Return the route entry of the process 'dest'.
 * If the entry is not found, it is created using the 'create_func' function and
 * initialized using the 'init_funct' function.
 *
 * Return:
 *  - added: if the entry has been created
 *  - is_initiator: if the current process is the initiator of the entry creation.
 */
sctk_endpoint_t *sctk_route_dynamic_safe_add ( int dest, sctk_rail_info_t *rail, sctk_endpoint_t * ( *create_func ) (), void ( *init_func ) ( int dest, sctk_rail_info_t *rail, sctk_endpoint_t *route_table, int ondemand ), int *added, char is_initiator )
{
	sctk_route_key_t key;
	sctk_endpoint_t *tmp;
	*added = 0;

	key.destination = dest;
	key.rail = rail->rail_number;

	sctk_spinlock_write_lock ( &sctk_route_table_lock );
	HASH_FIND ( hh, sctk_dynamic_route_table, &key, sizeof ( sctk_route_key_t ), tmp );

	/* Entry not found, we create it */
	if ( tmp == NULL )
	{
		/* QP added on demand */
		tmp = create_func();
		init_func ( dest, rail, tmp, 1 );
		/* We init the entry and add it to the table */
		sctk_init_dynamic_route ( dest, tmp, rail );
		tmp->is_initiator = is_initiator;
		HASH_ADD ( hh, sctk_dynamic_route_table, key, sizeof ( sctk_route_key_t ), tmp );
		*added = 1;
	}
	else
	{
		if ( sctk_route_get_state ( tmp ) == STATE_RECONNECTING )
		{
			ROUTE_LOCK ( tmp );
			sctk_route_set_state ( tmp, STATE_DECONNECTED );
			init_func ( dest, rail, tmp, 1 );
			sctk_route_set_low_memory_mode_local ( tmp, 0 );
			sctk_route_set_low_memory_mode_remote ( tmp, 0 );
			tmp->is_initiator = is_initiator;
			ROUTE_UNLOCK ( tmp );
			*added = 1;
		}
	}

	sctk_spinlock_write_unlock ( &sctk_route_table_lock );
	return tmp;
}

/* Return if the process is the initiator of the connexion or not */
char sctk_route_get_is_initiator ( sctk_endpoint_t *route_table )
{
	assume ( route_table );
	int is_initiator;

	ROUTE_LOCK ( route_table );
	is_initiator = route_table->is_initiator;
	ROUTE_UNLOCK ( route_table );

	return is_initiator;
}

sctk_endpoint_t *sctk_route_dynamic_search ( int dest, sctk_rail_info_t *rail )
{
	sctk_route_key_t key;
	sctk_endpoint_t *tmp;

	key.destination = dest;
	key.rail = rail->rail_number;
	sctk_spinlock_read_lock ( &sctk_route_table_lock );
	HASH_FIND ( hh, sctk_dynamic_route_table, &key, sizeof ( sctk_route_key_t ), tmp );
	sctk_spinlock_read_unlock ( &sctk_route_table_lock );
	return tmp;
}

/************************************************************************/
/* Route Initialization and Management                                  */
/************************************************************************/
void sctk_init_dynamic_route ( int dest, sctk_endpoint_t *tmp, sctk_rail_info_t *rail )
{
	tmp->key.destination = dest;
	tmp->key.rail = rail->rail_number;
	tmp->rail = rail;

	/* sctk_assert (sctk_route_dynamic_search(dest, rail) == NULL); */
	sctk_route_set_low_memory_mode_local ( tmp, 0 );
	sctk_route_set_low_memory_mode_remote ( tmp, 0 );
	sctk_route_set_state ( tmp, STATE_DECONNECTED );

	tmp->is_initiator = CHAR_MAX;
	tmp->lock = SCTK_SPINLOCK_INITIALIZER;

	tmp->origin = ROUTE_ORIGIN_DYNAMIC;
}

void sctk_add_dynamic_route ( int dest, sctk_endpoint_t *tmp, sctk_rail_info_t *rail )
{
	sctk_spinlock_write_lock ( &sctk_route_table_lock );
	HASH_ADD ( hh, sctk_dynamic_route_table, key, sizeof ( sctk_route_key_t ), tmp );
	sctk_spinlock_write_unlock ( &sctk_route_table_lock );
}

void sctk_init_static_route ( int dest, sctk_endpoint_t *tmp, sctk_rail_info_t *rail )
{
	tmp->key.destination = dest;
	tmp->key.rail = rail->rail_number;
	tmp->rail = rail;
	/* FIXME: the following commented line may potentially break other modules (like TCP). */
	sctk_route_set_low_memory_mode_local ( tmp, 0 );
	sctk_route_set_low_memory_mode_remote ( tmp, 0 );
	sctk_route_set_state ( tmp, STATE_DECONNECTED );

	tmp->is_initiator = CHAR_MAX;
	tmp->lock = SCTK_SPINLOCK_INITIALIZER;

	tmp->origin = ROUTE_ORIGIN_STATIC;
}

void sctk_add_static_route ( int dest, sctk_endpoint_t *tmp, sctk_rail_info_t *rail )
{
	sctk_nodebug ( "Add process %d to static routes", dest );
	/* FIXME: Ideally the initialization should not be in the 'add' function */
	sctk_init_static_route ( dest, tmp, rail );
	
	sctk_spinlock_write_lock(&sctk_route_table_init_lock);
	HASH_ADD ( hh, sctk_static_route_table, key, sizeof ( sctk_route_key_t ), tmp );
	sctk_spinlock_write_unlock(&sctk_route_table_init_lock);
}


/************************************************************************/
/* Route Walking                                                        */
/************************************************************************/

UT_icd ptr_icd = {sizeof ( sctk_endpoint_t ** ), NULL, NULL, NULL};

/*
 * Walk through all registered routes and call the function 'func'.
 * Static and dynamic routes are involved
 */
void sctk_walk_all_routes ( const sctk_rail_info_t *rail, void ( *func ) ( const sctk_rail_info_t *rail, sctk_endpoint_t *table ) )
{
	sctk_endpoint_t *current_route, *tmp, **tmp2;
	UT_array *routes;
	utarray_new ( routes, &ptr_icd );

	/* We do not need to take a lock */
	HASH_ITER ( hh, sctk_static_route_table, current_route, tmp )
	{
		if ( sctk_route_get_state ( current_route ) == STATE_CONNECTED )
		{
			if ( sctk_route_cas_low_memory_mode_local ( current_route, 0, 1 ) == 0 )
			{
				utarray_push_back ( routes, &current_route );
			}
		}
	}

	sctk_spinlock_read_lock ( &sctk_route_table_lock );
	HASH_ITER ( hh, sctk_dynamic_route_table, current_route, tmp )
	{
		if ( sctk_route_get_state ( current_route ) == STATE_CONNECTED )
		{
			if ( sctk_route_cas_low_memory_mode_local ( current_route, 0, 1 ) == 0 )
			{
				utarray_push_back ( routes, &current_route );
			}
		}
	}
	sctk_spinlock_read_unlock ( &sctk_route_table_lock );

	tmp2 = NULL;

	while ( ( tmp2 = ( sctk_endpoint_t ** ) utarray_next ( routes, tmp2 ) ) )
	{
		func ( rail, *tmp2 );
	}
}

/* Walk through all registered routes and call the function 'func'.
 * Only dynamic routes are involved */
void sctk_walk_all_dynamic_routes ( const sctk_rail_info_t *rail, void ( *func ) ( const sctk_rail_info_t *rail, sctk_endpoint_t *table ) )
{
	sctk_endpoint_t *current_route, *tmp, **tmp2;
	UT_array *routes;
	utarray_new ( routes, &ptr_icd );

	/* Do not walk on static routes */

	sctk_spinlock_read_lock ( &sctk_route_table_lock );
	HASH_ITER ( hh, sctk_dynamic_route_table, current_route, tmp )
	{
		if ( sctk_route_get_state ( current_route ) == STATE_CONNECTED )
		{
			if ( sctk_route_cas_low_memory_mode_local ( current_route, 0, 1 ) == 0 )
			{
				utarray_push_back ( routes, &current_route );
			}
		}
	}
	sctk_spinlock_read_unlock ( &sctk_route_table_lock );

	tmp2 = NULL;

	while ( ( tmp2 = ( sctk_endpoint_t ** ) utarray_next ( routes, tmp2 ) ) )
	{
		func ( rail, *tmp2 );
	}
}


/************************************************************************/
/* Route Calculation                                                    */
/************************************************************************/

/* Get a static route with no routing (can return NULL ) */
static inline sctk_endpoint_t *sctk_get_static_route_to_process_no_routing ( int dest, sctk_rail_info_t *rail )
{
	sctk_route_key_t key;
	sctk_endpoint_t *tmp;

	key.destination = dest;
	key.rail = rail->rail_number;

	/* We do not need to take a lock for the static table. No route can be created
	* or destructed during execution time */

	HASH_FIND ( hh, sctk_static_route_table, &key, sizeof ( sctk_route_key_t ), tmp );
	sctk_nodebug ( "Get static route for %d -> %p", dest, tmp );

	return tmp;
}

/* Get a route (static / dynamic) with no routing (can return NULL) */
sctk_endpoint_t *sctk_get_route_to_process_no_routing ( int dest, sctk_rail_info_t *rail )
{
	sctk_route_key_t key;
	sctk_endpoint_t *tmp;

	key.destination = dest;
	key.rail = rail->rail_number;

	/* First try static routes */
	tmp = sctk_get_static_route_to_process_no_routing ( dest, rail );

	/* It it fails look for dynamic routes on current rail */
	if ( tmp == NULL )
	{
		sctk_spinlock_read_lock ( &sctk_route_table_lock );
		HASH_FIND ( hh, sctk_dynamic_route_table, &key, sizeof ( sctk_route_key_t ), tmp );
		sctk_spinlock_read_unlock ( &sctk_route_table_lock );

		/* If the route is deconnected, we do not use it*/
		if ( tmp && sctk_route_get_state ( tmp ) != STATE_CONNECTED )
		{
			tmp = NULL;
		}
	}

	sctk_nodebug ( "Get static route for %d -> %p", dest, tmp );
	return tmp;
}

struct wait_connexion_args_s
{
	sctk_route_state_t state;
	sctk_endpoint_t *route_table;
	sctk_rail_info_t *rail;
	int done;
};

void *__wait_connexion ( void *a )
{
	struct wait_connexion_args_s *args = ( struct wait_connexion_args_s * ) a;

	if ( sctk_route_get_state ( args->route_table ) == args->state )
	{
		args->done = 1;
	}
	else
	{
		/* The notify idle message *MUST* be filled for supporting on-demand
		 * connexion */
		sctk_network_notify_idle_message();
	}

	return NULL;
}

static inline void __wait_state ( sctk_rail_info_t *rail, sctk_endpoint_t *route_table, sctk_route_state_t state )
{
	struct wait_connexion_args_s args;
	args.route_table = route_table;
	args.done = 0;
	args.rail = rail;
	args.state = state;
	sctk_thread_wait_for_value_and_poll ( ( int * ) &args.done, 1, ( void ( * ) ( void * ) ) __wait_connexion, &args );
	assume ( sctk_route_get_state ( route_table ) == state );
}

/* Get a route to a process only on static routes (does not create new routes => Relies on routing) */
inline sctk_endpoint_t *sctk_get_route_to_process_static ( int dest, sctk_rail_info_t *rail )
{
	sctk_endpoint_t *tmp;
	tmp = sctk_get_static_route_to_process_no_routing ( dest, rail );

	if ( tmp == NULL )
	{
		dest = rail->route ( dest, rail );
		return sctk_get_route_to_process_static ( dest, rail );
	}

	return tmp;
}

/* Get a route to a process with no ondemand connexions (relies on both static and dynamic routes without creating
 * routes => Relies on routing ) */
inline sctk_endpoint_t *sctk_get_route_to_process_no_ondemand ( int dest, sctk_rail_info_t *rail )
{
	sctk_endpoint_t *tmp;
	/* Try to get a dynamic / static route until this process */
	tmp = sctk_get_route_to_process_no_routing ( dest, rail );

	if ( tmp == NULL )
	{
		/* Otherwise route until target process */
		dest = rail->route ( dest, rail );
		/* Use the same function which does not create new routes */
		return sctk_get_route_to_process_no_ondemand ( dest, rail );
	}

	return tmp;
}

/* Get a route to process, creating the route if not present */
sctk_endpoint_t *sctk_get_route_to_process ( int dest, sctk_rail_info_t *rail )
{
	sctk_endpoint_t *tmp;

	/* Try to find a direct route with no routing */
	tmp = sctk_get_route_to_process_no_routing ( dest, rail );

	if ( tmp )
	{
		sctk_nodebug ( "Directly connected to %d", dest );
	}
	else
	{
		sctk_nodebug ( "NOT Directly connected to %d", dest );
	}

	if ( tmp == NULL )
	{
		/* Here we did not found the route therefore we instantiate it */
#if MPC_USE_INFINIBAND
		if ( rail->on_demand )
		{
			sctk_nodebug ( "%d Trying to connect to process %d (remote:%p)", sctk_process_rank, dest, tmp );

			/* Wait until we reach the 'deconnected' state */
			tmp = sctk_route_dynamic_search ( dest, rail );

			if ( tmp )
			{
				sctk_route_state_t state;
				state = sctk_route_get_state ( tmp );
				sctk_nodebug ( "Got state %d", state );

				do
				{
					state = sctk_route_get_state ( tmp );

					if ( state != STATE_DECONNECTED && state != STATE_CONNECTED &&
					        state != STATE_RECONNECTING )
					{
						sctk_network_notify_idle_message();
						sctk_thread_yield();
					}
				}
				while ( state != STATE_DECONNECTED && state != STATE_CONNECTED && state != STATE_RECONNECTING );
			}

			sctk_nodebug ( "QP in a KNOWN STATE" );

			/* We send the request using the signalization rail */
			tmp = sctk_ib_cm_on_demand_request ( dest, rail );
			assume ( tmp );

			/* If route not connected, so we wait for until it is connected */
			while ( sctk_route_get_state ( tmp ) != STATE_CONNECTED )
			{
				sctk_network_notify_idle_message();

				if ( sctk_route_get_state ( tmp ) != STATE_CONNECTED )
				{
					sctk_thread_yield();
				}

				//        __wait_state(rail, tmp, STATE_CONNECTED);
			}

			sctk_nodebug ( "Connected to process %d", dest );
			return tmp;
		}
		else
		{
#endif
			dest = rail->route ( dest, rail );
			return sctk_get_route_to_process_no_ondemand ( dest, rail );
#if MPC_USE_INFINIBAND
		}

#endif
	}

	return tmp;
}

/* Get the route to a given task (on demand mode) */
sctk_endpoint_t *sctk_get_route ( int dest, sctk_rail_info_t *rail )
{
	sctk_endpoint_t *tmp;
	int process;

	process = sctk_get_process_rank_from_task_rank ( dest );
	tmp = sctk_get_route_to_process ( process, rail );
	return tmp;
}

/************************************************************************/
/* Routes                                                               */
/************************************************************************/

static void sctk_free_route_messages ( void *ptr )
{

}

typedef struct
{
	sctk_request_t request;
	sctk_thread_ptp_message_t msg;
} sctk_route_messages_t;

void sctk_route_messages_send ( int myself, int dest, specific_message_tag_t specific_message_tag, int tag, void *buffer, size_t size )
{
	sctk_communicator_t communicator = SCTK_COMM_WORLD;
	sctk_route_messages_t msg;
	sctk_route_messages_t *msg_req;

	msg_req = &msg;
	sctk_init_header ( & ( msg_req->msg ), myself, SCTK_MESSAGE_CONTIGUOUS, sctk_free_route_messages, sctk_message_copy );
	sctk_add_adress_in_message ( & ( msg_req->msg ), buffer, size );
	sctk_set_header_in_message ( & ( msg_req->msg ), tag, communicator, myself, dest,  & ( msg_req->request ), size, specific_message_tag, MPC_DATATYPE_IGNORE );
	sctk_send_message ( & ( msg_req->msg ) );
	sctk_wait_message ( & ( msg_req->request ) );
}

void sctk_route_messages_recv ( int src, int myself, specific_message_tag_t specific_message_tag, int tag, void *buffer, size_t size )
{
	sctk_communicator_t communicator = SCTK_COMM_WORLD;
	sctk_route_messages_t msg;
	sctk_route_messages_t *msg_req;

	msg_req = &msg;

	sctk_init_header ( & ( msg_req->msg ), myself, SCTK_MESSAGE_CONTIGUOUS, sctk_free_route_messages, sctk_message_copy );
	sctk_add_adress_in_message ( & ( msg_req->msg ), buffer, size );
	sctk_set_header_in_message ( & ( msg_req->msg ), tag, communicator,  src, myself,  & ( msg_req->request ), size, specific_message_tag, MPC_DATATYPE_IGNORE );
	sctk_recv_message ( & ( msg_req->msg ), NULL, 1 );
	sctk_wait_message ( & ( msg_req->request ) );
}


/************************************************************************/
/* Signalization rails: getters and setters                             */
/************************************************************************/
static sctk_rail_info_t *rail_signalization = NULL;

void sctk_route_set_signalization_rail ( sctk_rail_info_t *rail )
{
	rail_signalization = rail;
}
sctk_rail_info_t *sctk_route_get_signalization_rail()
{
	assume ( rail_signalization );
	return rail_signalization;
}

