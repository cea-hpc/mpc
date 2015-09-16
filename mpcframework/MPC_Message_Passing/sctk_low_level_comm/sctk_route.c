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
#include <sctk_multirail.h>


/************************************************************************/
/* Route Table                                                          */
/************************************************************************/

sctk_route_table_t * sctk_route_table_new()
{
	sctk_route_table_t * ret = sctk_malloc( sizeof( sctk_route_table_t ) );
	
	assume( ret != NULL );

	sctk_spin_rwlock_t lck = SCTK_SPIN_RWLOCK_INITIALIZER;
	ret->dynamic_route_table_lock = lck;
	
	MPCHT_init( &ret->dynamic_route_table, 128 );
	MPCHT_init( &ret->static_route_table, 128 );


	return ret;
}


/************************************************************************/
/* Routes Memory Status                                                 */
/************************************************************************/

int sctk_endpoint_cas_low_memory_mode_local ( sctk_endpoint_t *tmp, int oldv, int newv )
{
	return ( int ) OPA_cas_int ( &tmp->low_memory_mode_local, oldv, newv );
}

int sctk_endpoint_is_low_memory_mode_local ( sctk_endpoint_t *tmp )
{
	return ( int ) OPA_load_int ( &tmp->low_memory_mode_local );
}
void sctk_endpoint_set_low_memory_mode_local ( sctk_endpoint_t *tmp, int low )
{
	if ( low )
		sctk_warning ( "Local process %d set to low level memory", tmp->dest );

	OPA_store_int ( &tmp->low_memory_mode_local, low );
}
int sctk_endpoint_is_low_memory_mode_remote ( sctk_endpoint_t *tmp )
{
	return ( int ) OPA_load_int ( &tmp->low_memory_mode_remote );
}

void sctk_endpoint_set_low_memory_mode_remote ( sctk_endpoint_t *tmp, int low )
{
	if ( low )
		sctk_warning ( "Remote process %d set to low level memory", tmp->dest );

	OPA_store_int ( &tmp->low_memory_mode_remote, low );
}

/************************************************************************/
/* Route Getters                                                        */
/************************************************************************/


/* Return if the process is the initiator of the connexion or not */
char sctk_endpoint_get_is_initiator ( sctk_endpoint_t *route_table )
{
	assume ( route_table );
	int is_initiator;

	ROUTE_LOCK ( route_table );
	is_initiator = route_table->is_initiator;
	ROUTE_UNLOCK ( route_table );

	return is_initiator;
}


/************************************************************************/
/* ENDPOINTS                                                            */
/************************************************************************/

void sctk_endpoint_init ( sctk_endpoint_t *tmp,  int dest, sctk_rail_info_t *rail, sctk_route_origin_t origin )
{
	memset( tmp, 0, sizeof( sctk_endpoint_t ) );
	tmp->dest = dest;
	tmp->rail = rail;
	tmp->parent_rail = NULL;
	tmp->subrail_id = -1;

	sctk_endpoint_set_low_memory_mode_local ( tmp, 0 );
	sctk_endpoint_set_low_memory_mode_remote ( tmp, 0 );
	sctk_endpoint_set_state ( tmp, STATE_DECONNECTED );

	tmp->is_initiator = CHAR_MAX;
	tmp->lock = SCTK_SPINLOCK_INITIALIZER;

	tmp->origin = origin;
}

void sctk_endpoint_init_static ( sctk_endpoint_t *tmp, int dest, sctk_rail_info_t *rail )
{
	sctk_endpoint_init ( tmp, dest, rail, ROUTE_ORIGIN_STATIC );
}

void sctk_endpoint_init_dynamic ( sctk_endpoint_t *tmp, int dest, sctk_rail_info_t *rail )
{
	sctk_endpoint_init ( tmp, dest, rail, ROUTE_ORIGIN_DYNAMIC );
}



/************************************************************************/
/* Route Table setters                                                  */
/************************************************************************/


void sctk_route_table_add_dynamic_route_no_lock (  sctk_route_table_t * table, sctk_endpoint_t *tmp, int push_in_multirail )
{
	assume( tmp->origin == ROUTE_ORIGIN_DYNAMIC );
	
	MPCHT_set(  &table->dynamic_route_table, tmp->dest, tmp );

	if( push_in_multirail )
	{
		/* Register route in multi-rail */
		sctk_multirail_destination_table_push_endpoint( tmp );
	}
}

void sctk_route_table_add_dynamic_route (  sctk_route_table_t * table, sctk_endpoint_t *tmp, int push_in_multirail )
{
	assume( tmp->origin == ROUTE_ORIGIN_DYNAMIC );
	
	sctk_spinlock_write_lock ( &table->dynamic_route_table_lock );
	sctk_route_table_add_dynamic_route_no_lock ( table,tmp, push_in_multirail );
	sctk_spinlock_write_unlock ( &table->dynamic_route_table_lock  );
}


void sctk_route_table_add_static_route (  sctk_route_table_t * table, sctk_endpoint_t *tmp , int push_in_multirail)
{
	sctk_nodebug("New static route to %d", tmp->dest );
	
	assume( tmp->origin == ROUTE_ORIGIN_STATIC );
		
	MPCHT_set( &table->static_route_table, tmp->dest, tmp );
	
	if( push_in_multirail )
	{
		/* Register route in multi-rail */
		sctk_multirail_destination_table_push_endpoint( tmp );
	}
}


/************************************************************************/
/* Route Table Getters                                                  */
/************************************************************************/

sctk_endpoint_t * sctk_route_table_get_static_route(   sctk_route_table_t * table , int dest )
{
	sctk_endpoint_t *tmp = NULL;

	tmp = MPCHT_get(&table->static_route_table, dest);

	sctk_nodebug ( "Get static route for %d -> %p", dest, tmp );

	return tmp;
}

sctk_endpoint_t * sctk_route_table_get_dynamic_route_no_lock( sctk_route_table_t * table , int dest )
{
	sctk_endpoint_t *tmp = NULL;

	tmp = MPCHT_get( &table->dynamic_route_table, dest );

	return tmp;
}

sctk_endpoint_t * sctk_route_table_get_dynamic_route( sctk_route_table_t * table , int dest )
{
	sctk_endpoint_t *tmp;

	sctk_spinlock_read_lock ( &table->dynamic_route_table_lock );
	tmp = sctk_route_table_get_dynamic_route_no_lock( table ,  dest );
	sctk_spinlock_read_unlock ( &table->dynamic_route_table_lock );
	
	return tmp;
}




/************************************************************************/
/* Route Walking                                                        */
/************************************************************************/

UT_icd ptr_icd = {sizeof ( sctk_endpoint_t ** ), NULL, NULL, NULL};

/* Walk through all registered routes and call the function 'func'.
 * Only dynamic routes are involved */
void sctk_walk_all_dynamic_routes ( sctk_route_table_t * table, void ( *func ) (  sctk_endpoint_t *endpoint, void * arg  ), void * arg )
{
	sctk_endpoint_t *current_route, *tmp, **tmp2;
	UT_array *routes;
	utarray_new ( routes, &ptr_icd );

	/* Do not walk on static routes */

	sctk_spinlock_read_lock ( &table->dynamic_route_table_lock );
	
	MPC_HT_ITER ( &table->dynamic_route_table, current_route )
	{
		if ( sctk_endpoint_get_state ( current_route ) == STATE_CONNECTED )
		{
			if ( sctk_endpoint_cas_low_memory_mode_local ( current_route, 0, 1 ) == 0 )
			{
				utarray_push_back ( routes, &current_route );
			}
		}
	}
	MPC_HT_ITER_END
	
	sctk_spinlock_read_unlock ( &table->dynamic_route_table_lock );

	tmp2 = NULL;

	while ( ( tmp2 = ( sctk_endpoint_t ** ) utarray_next ( routes, tmp2 ) ) )
	{
		func ( *tmp2, arg );
	}
}


/* Walk through all registered routes and call the function 'func'.
 * Only dynamic routes are involved */
void sctk_walk_all_static_routes( sctk_route_table_t * table, void ( *func ) (  sctk_endpoint_t *endpoint, void * arg  ), void * arg )
{
	sctk_endpoint_t *current_route, *tmp, **tmp2;
	UT_array *routes;
	utarray_new ( routes, &ptr_icd );

	/* Do not walk on static routes */

	MPC_HT_ITER ( &table->static_route_table, current_route )
	{
		if ( sctk_endpoint_get_state ( current_route ) == STATE_CONNECTED )
		{
			if ( sctk_endpoint_cas_low_memory_mode_local ( current_route, 0, 1 ) == 0 )
			{
				utarray_push_back ( routes, &current_route );
			}
		}
	}
	MPC_HT_ITER_END

	tmp2 = NULL;

	while ( ( tmp2 = ( sctk_endpoint_t ** ) utarray_next ( routes, tmp2 ) ) )
	{
		func ( *tmp2, arg );
	}
}


/************************************************************************/
/* Route Calculation                                                    */
/************************************************************************/


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


void sctk_route_messages_send ( int myself, int dest, sctk_message_class_t message_class, int tag, void *buffer, size_t size )
{
	sctk_info("ROUTE SEND [ %d -> %d ] ( size %d)", myself, dest, size );
	
	sctk_communicator_t communicator = SCTK_COMM_WORLD;
	sctk_route_messages_t msg;
	sctk_route_messages_t *msg_req;
	
	msg_req = &msg;
	
	sctk_init_header ( & ( msg_req->msg ), SCTK_MESSAGE_CONTIGUOUS, sctk_free_route_messages, sctk_message_copy );
	sctk_add_adress_in_message ( & ( msg_req->msg ), buffer, size );
	sctk_set_header_in_message ( & ( msg_req->msg ), tag, communicator, myself, dest,  & ( msg_req->request ), size, message_class, SCTK_DATATYPE_IGNORE );
	sctk_send_message ( & ( msg_req->msg ) );
	sctk_wait_message ( & ( msg_req->request ) );
}

void sctk_route_messages_recv ( int src, int myself, sctk_message_class_t message_class, int tag, void *buffer, size_t size )
{
	sctk_info("ROUTE RECV [ %d -> %d ] ( size %d)", src, myself, size );
	
	sctk_communicator_t communicator = SCTK_COMM_WORLD;
	sctk_route_messages_t msg;
	sctk_route_messages_t *msg_req;

	msg_req = &msg;

	sctk_init_header ( & ( msg_req->msg ), SCTK_MESSAGE_CONTIGUOUS, sctk_free_route_messages, sctk_message_copy );
	sctk_add_adress_in_message ( & ( msg_req->msg ), buffer, size );
	sctk_set_header_in_message ( & ( msg_req->msg ), tag, communicator,  src, myself,  & ( msg_req->request ), size, message_class, SCTK_DATATYPE_IGNORE );
	sctk_recv_message ( & ( msg_req->msg ), NULL, 1 );
	sctk_wait_message ( & ( msg_req->request ) );
}


