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


#include <sctk_debug.h>
#include <sctk_net_tools.h>
#include <sctk_tcp_toolkit.h>

extern volatile int sctk_online_program;

/********************************************************************/
/* Inter Thread Comm Hooks                                          */
/********************************************************************/

static void *sctk_tcp_thread ( sctk_endpoint_t *tmp )
{
	int fd = tmp->data.tcp.fd;

	sctk_debug ( "Rail %d from %d launched", tmp->rail->rail_number, tmp->dest );

	while ( 1 )
	{
		sctk_thread_ptp_message_t *msg;
		void *body;
		size_t size;
		ssize_t res;

		res = sctk_safe_read ( fd, ( char * ) &size, sizeof ( size_t ) );
		//sctk_debug ( "Got msg of size %d (online:%d)", res, sctk_online_program );

		if ( res < sizeof ( size_t ) )
		{
			return NULL;
		}

		if ( size < sizeof ( sctk_thread_ptp_message_body_t ) )
		{
			return NULL;
		}

		size = size - sizeof ( sctk_thread_ptp_message_body_t ) + sizeof ( sctk_thread_ptp_message_t );
	
		msg = sctk_malloc ( size );
		
		assume( msg != NULL );
		
		body = ( char * ) msg + sizeof ( sctk_thread_ptp_message_t );

		/* Recv header*/
		//sctk_debug ( "Read %d", sizeof ( sctk_thread_ptp_message_body_t ) );
		res = sctk_safe_read ( fd, ( char * ) msg, sizeof ( sctk_thread_ptp_message_body_t ) );

		if ( res != sizeof ( sctk_thread_ptp_message_body_t ) )
		{
			return NULL;
		}

		SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
		msg->tail.message_type = SCTK_MESSAGE_NETWORK;

		if ( SCTK_MSG_COMMUNICATOR ( msg ) < 0 )
		{
			return NULL;
		}

		/* Recv body*/
		size = size - sizeof ( sctk_thread_ptp_message_t );
		sctk_safe_read ( fd, ( char * ) body, size );

		sctk_rebuild_header ( msg );
		sctk_reinit_header ( msg, sctk_free, sctk_net_message_copy );


		tmp->rail->send_message_from_network ( msg );
	}
	
	//sctk_error("TCP THREAD LEAVING");

	return NULL;
}

static void sctk_network_send_message_endpoint_tcp ( sctk_thread_ptp_message_t *msg, sctk_endpoint_t *endpoint )
{
	sctk_error("SUBRAIL %d", endpoint->rail->subrail_id );
	
	size_t size;
	int fd;


	sctk_spinlock_lock ( & ( endpoint->data.tcp.lock ) );

	fd = endpoint->data.tcp.fd;

	size = SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_body_t );

	assume( fd != NULL );

	sctk_safe_write ( fd, ( char * ) &size, sizeof ( size_t ) );

	sctk_safe_write ( fd, ( char * ) msg, sizeof ( sctk_thread_ptp_message_body_t ) );

	sctk_net_write_in_fd ( msg, fd );
	sctk_spinlock_unlock ( & ( endpoint->data.tcp.lock ) );

	sctk_complete_and_free_message ( msg );
}

static void sctk_network_notify_recv_message_tcp ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{

}

static void sctk_network_notify_matching_message_tcp ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{

}

static void sctk_network_notify_perform_message_tcp ( int remote, int remote_task_id, int polling_task_id, int blocking, sctk_rail_info_t *rail )
{

}

static void sctk_network_notify_idle_message_tcp ()
{
	sched_yield();
}

static void sctk_network_notify_any_source_message_tcp ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{

}



static int sctk_send_message_from_network_tcp ( sctk_thread_ptp_message_t *msg )
{
	if ( sctk_send_message_from_network_reorder ( msg ) == REORDER_NO_NUMBERING )
	{
		/* No reordering */
		sctk_send_message_try_check ( msg, 1 );
	}

	return 1;
}


/********************************************************************/
/* TCP Init                                                         */
/********************************************************************/

void sctk_network_init_tcp ( sctk_rail_info_t *rail )
{
	/* Register Hooks in rail */
	rail->send_message_endpoint = sctk_network_send_message_endpoint_tcp;
	rail->notify_recv_message = sctk_network_notify_recv_message_tcp;
	rail->notify_matching_message = sctk_network_notify_matching_message_tcp;
	rail->notify_perform_message = sctk_network_notify_perform_message_tcp;
	rail->notify_idle_message = sctk_network_notify_idle_message_tcp;
	rail->notify_any_source_message = sctk_network_notify_any_source_message_tcp;
	rail->send_message_from_network = sctk_send_message_from_network_tcp;

	sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, tcp_on_demand_connection_handler );

	int sctk_use_tcp_o_ib = rail->runtime_config_driver_config->driver.value.tcp.tcpoib;

	/* Handle the IPoIB case */
	if ( sctk_use_tcp_o_ib == 0 )
	{
		rail->network_name = "TCP";
	}
	else
	{
		rail->network_name = "TCP_O_IB";
	}

	/* Actually initialize the network (note TCP kind specific functions) */
	sctk_network_init_tcp_all ( rail, sctk_use_tcp_o_ib, sctk_tcp_thread, rail->route_init );
}
