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

#include <mpc_common_debug.h>
#include <sctk_net_tools.h>
#include <sctk_tcp_toolkit.h>

#include <sctk_alloc.h>

#include "sctk_rail.h"

/**
 * Define TCP RDMA values
 */
typedef enum
{
    MPC_LOWCOMM_RDMA_MESSAGE_HEADER, /**< It is an RDMA header */
    SCTK_RDMA_READ,           /**< RDMA read */
    SCTK_RDMA_WRITE           /**< RDMA write */
} sctk_tcp_rdma_type_t;

/**
 * Called when a net message matched with a local RECV.
 * \param[in] tmp the request object (local-recv,remote-send)
 */
static void sctk_tcp_rdma_message_copy ( mpc_lowcomm_ptp_message_content_to_copy_t *tmp )
{
	_mpc_lowcomm_endpoint_t *route;
	int fd;

	route = tmp->msg_send->tail.route_table;

	fd = route->data.tcp.fd;
	mpc_common_spinlock_lock ( & ( route->data.tcp.lock ) );
	{
		sctk_tcp_rdma_type_t op_type;
		op_type = SCTK_RDMA_READ;
		mpc_common_io_safe_write ( fd, &op_type, sizeof ( sctk_tcp_rdma_type_t ) );
		mpc_common_io_safe_write ( fd, & ( tmp->msg_send->tail.rdma_src ), sizeof ( void * ) );
		mpc_common_io_safe_write ( fd, & ( tmp ), sizeof ( void * ) );
	}
	mpc_common_spinlock_unlock ( & ( route->data.tcp.lock ) );
}

/************************************************************************/
/* Network Hooks                                                        */
/************************************************************************/

/**
 * Polling function created for each route.
 * \param[in] tmp the endpoint to progress.
 */
static void *sctk_tcp_rdma_thread ( _mpc_lowcomm_endpoint_t *tmp )
{
	int fd;
	fd = tmp->data.tcp.fd;

	while ( 1 )
	{
		mpc_lowcomm_ptp_message_t *msg;
		size_t size;
		sctk_tcp_rdma_type_t op_type;
		ssize_t res;

		res = mpc_common_io_safe_read ( fd, ( char * ) &op_type, sizeof ( sctk_tcp_rdma_type_t ) );

		if ( res < (ssize_t)sizeof ( sctk_tcp_rdma_type_t ) )
		{
			return NULL;
		}

		switch ( op_type )
		{
			case MPC_LOWCOMM_RDMA_MESSAGE_HEADER:
			{
				size = sizeof ( mpc_lowcomm_ptp_message_t );
				msg = sctk_malloc ( size );

				/* Recv header*/
				mpc_common_nodebug ( "Read %d", sizeof ( mpc_lowcomm_ptp_message_body_t ) );
				mpc_common_io_safe_read ( fd, ( char * ) msg, sizeof ( mpc_lowcomm_ptp_message_body_t ) );
				mpc_common_io_safe_read ( fd, & ( msg->tail.rdma_src ), sizeof ( void * ) );
				msg->tail.route_table = tmp;

				SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
				msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;

				_mpc_comm_ptp_message_clear_request ( msg );
				_mpc_comm_ptp_message_set_copy_and_free ( msg, sctk_free, sctk_tcp_rdma_message_copy );

				mpc_common_nodebug ( "Msg recved" );
				tmp->rail->send_message_from_network ( msg );
				break;
			}

			case SCTK_RDMA_READ :
			{
				mpc_lowcomm_ptp_message_content_to_copy_t *copy_ptr;
				mpc_common_io_safe_read ( fd, ( char * ) &msg, sizeof ( void * ) );
				mpc_common_io_safe_read ( fd, ( char * ) &copy_ptr, sizeof ( void * ) );

				mpc_common_spinlock_lock ( & ( tmp->data.tcp.lock ) );
				op_type = SCTK_RDMA_WRITE;
				mpc_common_io_safe_write ( fd, &op_type, sizeof ( sctk_tcp_rdma_type_t ) );
				mpc_common_io_safe_write ( fd, & ( copy_ptr ), sizeof ( void * ) );
				sctk_net_write_in_fd ( msg, fd );
				mpc_common_spinlock_unlock ( & ( tmp->data.tcp.lock ) );

				mpc_lowcomm_ptp_message_complete_and_free ( msg );
				break;
			}

			case SCTK_RDMA_WRITE :
			{
				mpc_lowcomm_ptp_message_content_to_copy_t *copy_ptr;
				mpc_lowcomm_ptp_message_t *send = NULL;
				mpc_lowcomm_ptp_message_t *recv = NULL;

				mpc_common_io_safe_read ( fd, ( char * ) &copy_ptr, sizeof ( void * ) );
				sctk_net_read_in_fd ( copy_ptr->msg_recv, fd );

				send = copy_ptr->msg_send;
				recv = copy_ptr->msg_recv;
				_mpc_comm_ptp_message_commit_request ( send, recv );
				break;
			}

			default:
				not_reachable();
		}

	}

	return NULL;
}

/**
 * Send an TCP RDMA message.
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
static void _mpc_lowcomm_tcprdma_send_message ( mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint )
{
	int fd;

	mpc_common_spinlock_lock ( & ( endpoint->data.tcp.lock ) );

	fd = endpoint->data.tcp.fd;

	sctk_tcp_rdma_type_t op_type = MPC_LOWCOMM_RDMA_MESSAGE_HEADER;

	mpc_common_io_safe_write ( fd, &op_type, sizeof ( sctk_tcp_rdma_type_t ) );
	mpc_common_io_safe_write ( fd, ( char * ) msg, sizeof ( mpc_lowcomm_ptp_message_body_t ) );
	mpc_common_io_safe_write ( fd, &msg, sizeof ( void * ) );

	mpc_common_spinlock_unlock ( & ( endpoint->data.tcp.lock ) );

}


/************************************************************************/
/* TCP RDMA Init                                                        */
/************************************************************************/
/**
 * not used for this network.
 * \param[in] rail not used
 */
void sctk_network_finalize_tcp_rdma(__UNUSED__ sctk_rail_info_t* rail)
{
}

/**
 * Procedure to init an TCP RDMA rail.
 * \param[in,out] rail the rail to init.
 */
void sctk_network_init_tcp_rdma ( sctk_rail_info_t *rail )
{
	/* Init RDMA specific infos */
	rail->send_message_endpoint     = _mpc_lowcomm_tcprdma_send_message;
	rail->notify_recv_message       = NULL;
	rail->notify_matching_message   = NULL;
	rail->notify_perform_message    = NULL;
	rail->notify_idle_message       = NULL;
	rail->notify_any_source_message = NULL;
	rail->driver_finalize           = sctk_network_finalize_tcp_rdma;

	char * interface = rail->runtime_config_rail->device;

	if(!interface)
	{
		interface = "default";
	}

    char net_name[128];
    snprintf(net_name, 128, "TCP RDMA (%s)", interface);
    rail->network_name = strdup(net_name);


	/* Actually Init the TCP layer */
	sctk_network_init_tcp_all ( rail, interface, sctk_tcp_rdma_thread );
}
