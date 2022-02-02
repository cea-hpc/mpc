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
#include <tcp_toolkit.h>
#include <sys/socket.h> /* shutdown() */

#include <sctk_alloc.h>

#include "sctk_rail.h"

/********************************************************************/
/* Inter Thread Comm Hooks                                          */
/********************************************************************/

/**
 * Function called by each started polling thread, processing message on the given route.
 * \param[in] tmp the route to progress
 * \return NULL
 */
static void *__tcp_thread_loop(_mpc_lowcomm_endpoint_t *tmp)
{
	int fd = tmp->data.tcp.fd;

	mpc_common_debug("Rail %d from %d launched", tmp->rail->rail_number, tmp->dest);

	while(1)
	{
		mpc_lowcomm_ptp_message_t *msg;
		void *  body;
		size_t  size;
		ssize_t res;

		res = mpc_common_io_safe_read(fd, ( char * )&size, sizeof(size_t) );

		if( (res <= 0) )
		{
			/* EOF or ERROR */
			break;
		}

		if(res < (ssize_t)sizeof(size_t) )
		{
			break;
		}

		if(size < sizeof(mpc_lowcomm_ptp_message_body_t) )
		{
			break;
		}

		size = size - sizeof(mpc_lowcomm_ptp_message_body_t) + sizeof(mpc_lowcomm_ptp_message_t);

		msg = sctk_malloc(size);

		assume(msg != NULL);

		body = ( char * )msg + sizeof(mpc_lowcomm_ptp_message_t);


		/* Recv header*/
		res = mpc_common_io_safe_read(fd, ( char * )msg, sizeof(mpc_lowcomm_ptp_message_body_t) );

		if( (res <= 0) )
		{
			/* EOF or ERROR */
			break;
		}

		if(res != sizeof(mpc_lowcomm_ptp_message_body_t) )
		{
			break;
		}

		SCTK_MSG_COMPLETION_FLAG_SET(msg, NULL);
		msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;

		/* Recv body*/
		size = size - sizeof(mpc_lowcomm_ptp_message_t);

		res = mpc_common_io_safe_read(fd, ( char * )body, size);

		if( (res <= 0) )
		{
			/* EOF or ERROR */
			break;
		}

		_mpc_comm_ptp_message_clear_request(msg);
		_mpc_comm_ptp_message_set_copy_and_free(msg, sctk_free, sctk_net_message_copy);


		tmp->rail->send_message_from_network(msg);
	}

	shutdown(fd, SHUT_RDWR);
	close(fd);

	mpc_common_debug("TCP THREAD LEAVING");

	return NULL;
}

/**
 * Entry point for sending a message with the TCP network.
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
static void _mpc_lowcomm_tcp_send_message(mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint)
{
	size_t size;
	int    fd;

	mpc_common_spinlock_lock(&(endpoint->data.tcp.lock) );

	fd = endpoint->data.tcp.fd;

	size = SCTK_MSG_SIZE(msg) + sizeof(mpc_lowcomm_ptp_message_body_t);

	mpc_common_nodebug("SEND MSG of size %d ENDPOINT TCP to %d", size, endpoint->dest);

	mpc_common_io_safe_write(fd, ( char * )&size, sizeof(size_t) );

	mpc_common_io_safe_write(fd, ( char * )msg, sizeof(mpc_lowcomm_ptp_message_body_t) );

	sctk_net_write_in_fd(msg, fd);
	mpc_common_spinlock_unlock(&(endpoint->data.tcp.lock) );

	mpc_common_nodebug("SEND MSG ENDPOINT TCP to %d DONE", endpoint->dest);

	mpc_lowcomm_ptp_message_complete_and_free(msg);
}

/**
 * Handler triggering the send_message_from_network call, before reaching the inter_thread_comm matching process.
 * \param[in] msg the message received from the network, to be matched w/ a local RECV.
 */
static int _mpc_lowcomm_tcp_send_message_from_network(mpc_lowcomm_ptp_message_t *msg)
{
	if(_mpc_lowcomm_reorder_msg_check(msg) == _MPC_LOWCOMM_REORDER_NO_NUMBERING)
	{
		/* No reordering */
		_mpc_comm_ptp_message_send_check(msg, 1);
	}

	return 1;
}

/********************************************************************/
/* TCP Init                                                         */
/********************************************************************/

/**
 * Procedure to clear the TCP rail.
 * \param[in,out] rail the rail to close.
 */
void _mpc_lowcomm_tcp_finalize(sctk_rail_info_t *rail)
{
	_mpc_lowcomm_tcp_rail_info_t *rail_tcp = &rail->network.tcp;

	shutdown(rail_tcp->sockfd, SHUT_RDWR);
	close(rail_tcp->sockfd);
	rail_tcp->sockfd = -1;
	rail_tcp->portno = -1;
	rail_tcp->connection_infos[0]   = '\0';
	rail_tcp->connection_infos_size = 0;
}

/**
 * Procedure to initialize a new TCP rail.
 * \param[in] rail the TCP rail
 */
void sctk_network_init_tcp(sctk_rail_info_t *rail)
{
	/* Register Hooks in rail */
	rail->send_message_endpoint     = _mpc_lowcomm_tcp_send_message;
	rail->notify_recv_message       = NULL;
	rail->notify_matching_message   = NULL;
	rail->notify_perform_message    = NULL;
	rail->notify_idle_message       = NULL;
	rail->notify_any_source_message = NULL;
	rail->send_message_from_network = _mpc_lowcomm_tcp_send_message_from_network;
	rail->driver_finalize           = _mpc_lowcomm_tcp_finalize;

	sctk_rail_init_route(rail, rail->runtime_config_rail->topology, tcp_on_demand_connection_handler);

	char * interface = rail->runtime_config_rail->device;

	if(!interface)
	{
		interface = "default";
	}

	/* Handle the IPoIB case */
    char net_name[1024];
	snprintf(net_name, 1024, "TCP (%s)", interface);
    rail->network_name = strdup(net_name);

	/* Actually initialize the network (note TCP kind specific functions) */
	sctk_network_init_tcp_all(rail, interface, __tcp_thread_loop);
}
