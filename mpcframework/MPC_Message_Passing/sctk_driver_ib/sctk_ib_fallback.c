/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include <sctk_debug.h>
#include <sctk_ib_toolkit.h>
#include <sctk_route.h>
#include <sctk_net_tools.h>
#include <sctk_ib_prof.h>
#include <sctk_ib_fallback.h>
#include "sctk_ib.h"
#include <sctk_ibufs.h>
#include <sctk_ibufs_rdma.h>
#include <sctk_ib_mmu.h>
#include <sctk_ib_config.h>
#include "sctk_ib_qp.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_topology.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_async.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_buffered.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"

#define ASYNC_POLLING

#ifndef ASYNC_POLLING
#error "The fallback network use the async polling !!!"
#endif

static void sctk_network_send_message_ib ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );
	sctk_route_table_t *tmp;
	sctk_ib_route_info_t *route_data;
	sctk_ib_qp_t *remote;
	sctk_ibuf_t *ibuf;
	size_t size;
	char is_control_message = 0;
	specific_message_tag_t tag = SCTK_MSG_SPECIFIC_TAG ( msg );

	sctk_nodebug ( "send message through rail %d to %d", rail->rail_number, SCTK_MSG_DEST_PROCESS ( msg ) );
	sctk_nodebug ( "Send message with tag %d", SCTK_MSG_SPECIFIC_TAG ( msg ) );

	/* Determine the route to used */
	if ( IS_PROCESS_SPECIFIC_MESSAGE_TAG ( tag ) )
	{

		if ( IS_PROCESS_SPECIFIC_CONTROL_MESSAGE ( tag ) )
		{
			is_control_message = 1;
			/* send a message with no_ondemand connexion */
			tmp = sctk_get_route_to_process_static ( SCTK_MSG_DEST_PROCESS ( msg ), rail );
			sctk_nodebug ( "Send control message to process %d", SCTK_MSG_DEST_PROCESS ( msg ) );
		}
		else
		{
			/* send a message to a PROCESS with a possible ondemand connexion if the peer doest not
			* exist.  */
			tmp = sctk_get_route_to_process ( SCTK_MSG_DEST_PROCESS ( msg ), rail );
			sctk_nodebug ( "Send to process %d", SCTK_MSG_DEST_PROCESS ( msg ) );
		}

	}
	else
	{
		/* send a message to a TASK with a possible ondemand connexion if the peer doest not
		* exist.  */
		sctk_nodebug ( "Connexion to %d", SCTK_MSG_DEST_TASK ( msg ) );
		tmp = sctk_get_route ( SCTK_MSG_DEST_TASK ( msg ), rail );
	}

	route_data = &tmp->data.ib;
	remote = route_data->remote;

	size = SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_body_t );

	if ( is_control_message && ( ( size + IBUF_GET_EAGER_SIZE ) > config->eager_limit ) )
	{
		sctk_error ( "MPC tries to send a control message without using the Eager protocol."
		             "This is not supported and MPC is going to exit ..." );
		sctk_abort();
	}

	/* *
	*
	*  We switch between available protocols
	*
	* */

	if ( size + IBUF_GET_EAGER_SIZE <= config->eager_limit )
	{
		ibuf = sctk_ib_eager_prepare_msg ( rail_ib, remote, msg, size, -1, is_control_message );

		/* Actually, it is possible to get a NULL pointer for ibuf. We falback to buffered */
		if ( ibuf == NULL )
			goto buffered;

		/* Send message */
		sctk_ib_qp_send_ibuf ( rail_ib, remote, ibuf, is_control_message );
		sctk_complete_and_free_message ( msg );
		PROF_INC ( rail_ib->rail, ib_eager_nb );
		goto exit;
	}

buffered:

	/***** BUFFERED EAGER CHANNEL *****/
	if ( size <= config->buffered_limit )
	{
		/* Fallback to RDMA if buffered not available or low memory mode */
		if ( sctk_ib_buffered_prepare_msg ( rail, remote, msg, size ) == 1 )
			goto error;

		sctk_complete_and_free_message ( msg );
		PROF_INC ( rail_ib->rail, ib_buffered_nb );

		goto exit;
	}

error:
	sctk_error ( "MPC did not find any channel to use for sending the message."
	             "Your job is going to die ..." );
	sctk_abort();

exit:
	{}

}

static int sctk_network_poll_send_ibuf ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf, const char from_cp, struct sctk_ib_polling_s *poll )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	int release_ibuf = 1;

	/* Switch on the protocol of the received message */
	switch ( IBUF_GET_PROTOCOL ( ibuf->buffer ) )
	{
		case SCTK_IB_EAGER_PROTOCOL:
			release_ibuf = 1;
			break;

		case SCTK_IB_RDMA_PROTOCOL:
			release_ibuf = sctk_ib_rdma_poll_send ( rail, ibuf );
			sctk_nodebug ( "Received RMDA write send" );
			break;

		case SCTK_IB_BUFFERED_PROTOCOL:
			release_ibuf = 1;
			break;

		default:
			sctk_error ( "Got wrong protocol: %d %p", IBUF_GET_PROTOCOL ( ibuf->buffer ), &IBUF_GET_PROTOCOL ( ibuf->buffer ) );
			assume ( 0 );
			break;
	}

	assume ( IBUF_GET_CHANNEL ( ibuf ) & RC_SR_CHANNEL );

	if ( release_ibuf )
	{
		/* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
		sctk_ibuf_release ( rail_ib, ibuf, 1 );
	}
	else
	{
		sctk_ibuf_release_from_srq ( rail_ib, ibuf );
	}

	return 0;
}

static int sctk_network_poll_recv_ibuf ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf, const char from_cp, struct sctk_ib_polling_s *poll )
{
	const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL ( ibuf->buffer );
	int release_ibuf = 1;
	const struct ibv_wc wc = ibuf->wc;

	/* First we check if the message has an immediate data */
	if ( wc.wc_flags & IBV_WC_WITH_IMM )
	{
		assume ( 0 );
		sctk_abort();
	}
	else
	{
		/* Switch on the protocol of the received message */
		switch ( protocol )
		{
			case SCTK_IB_EAGER_PROTOCOL:
				sctk_ib_eager_poll_recv ( rail, ibuf );
				release_ibuf = 0;
				break;

			case SCTK_IB_RDMA_PROTOCOL:
				not_implemented();
				break;

			case SCTK_IB_BUFFERED_PROTOCOL:
				sctk_ib_buffered_poll_recv ( rail, ibuf );
				release_ibuf = 1;
				break;

			default:
				assume ( 0 );
		}
	}


	if ( release_ibuf )
	{
		/* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
		sctk_ibuf_release ( &rail->network.ib, ibuf, 1 );
	}
	else
	{
		sctk_ibuf_release_from_srq ( &rail->network.ib, ibuf );
	}

	return 0;
}

static int sctk_network_poll_recv ( sctk_rail_info_t *rail, struct ibv_wc *wc, struct sctk_ib_polling_s *poll )
{
	sctk_ibuf_t *ibuf = NULL;
	ibuf = ( sctk_ibuf_t * ) wc->wr_id;
	assume ( ibuf );

	ibuf->wc = *wc;
	return sctk_network_poll_recv_ibuf ( rail, ibuf, 0, poll );
}

static int sctk_network_poll_send ( sctk_rail_info_t *rail, struct ibv_wc *wc, struct sctk_ib_polling_s *poll )
{
	sctk_ibuf_t *ibuf = NULL;
	ibuf = ( sctk_ibuf_t * ) wc->wr_id;
	assume ( ibuf );

	sctk_nodebug ( "Send message released (rank: %d - pending_nb: %d)", ibuf->remote->rank, sctk_ib_qp_get_requests_nb ( ibuf->remote ) );

	ibuf->wc = *wc;
	return sctk_network_poll_send_ibuf ( rail, ibuf, 0, poll );
}


static int sctk_network_poll_all ( sctk_rail_info_t *rail )
{
	return 0;
}

static void sctk_network_notify_recv_message_ib ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{

}

static void sctk_network_notify_matching_message_ib ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{

}

/* WARNING: This function can be called with dest == sctk_process_rank */
static void sctk_network_notify_perform_message_ib ( int remote_process, int remote_task_id, int polling_task_id, int blocking, sctk_rail_info_t *rail )
{

}

static void sctk_network_notify_idle_message_ib ( sctk_rail_info_t *rail )
{

}

static void sctk_network_notify_any_source_message_ib ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{

}

static void sctk_network_connection_to_ib ( int from, int to, sctk_rail_info_t *rail )
{
	sctk_ib_cm_connect_to ( from, to, rail );
}

static void sctk_network_connection_from_ib ( int from, int to, sctk_rail_info_t *rail )
{
	sctk_ib_cm_connect_from ( from, to, rail );
}

/*-----------------------------------------------------------
 *  POLLING THREADS
 *----------------------------------------------------------*/
static void *sctk_ib_fallback_send_polling_thread ( void *arg )
{
	sctk_rail_info_t *rail = ( sctk_rail_info_t * ) arg;
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_DEVICE ( rail_ib );
	LOAD_CONFIG ( rail_ib );

	while ( 1 )
	{
		void *cq_context;
		struct ibv_cq *cq;

		if ( ibv_get_cq_event ( device->send_comp_channel, &cq, &cq_context ) )
		{
			sctk_error ( "Error on ibv_get_cq_event" );
			sctk_abort();
		}

		ibv_ack_cq_events ( cq, 1 );

		if ( ibv_req_notify_cq ( cq, 0 ) )
		{
			sctk_error ( "Error on ibv_req_notify_cq" );
			sctk_abort();
		}

		/* Poll sent messages */
		sctk_ib_polling_t poll;
		POLL_INIT ( &poll );
		sctk_ib_cq_poll ( rail, device->send_cq, config->wc_out_number, &poll, sctk_network_poll_send );
	}

	return NULL;
}

static void *sctk_ib_fallback_recv_polling_thread ( void *arg )
{
	sctk_rail_info_t *rail = ( sctk_rail_info_t * ) arg;
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_DEVICE ( rail_ib );
	LOAD_CONFIG ( rail_ib );

	while ( 1 )
	{
		void *cq_context;
		struct ibv_cq *cq;

		if ( ibv_get_cq_event ( device->recv_comp_channel, &cq, &cq_context ) )
		{
			sctk_error ( "Error on ibv_get_cq_event" );
			sctk_abort();
		}

		ibv_ack_cq_events ( cq, 1 );

		if ( ibv_req_notify_cq ( cq, 0 ) )
		{
			sctk_error ( "Error on ibv_req_notify_cq" );
			sctk_abort();
		}

		/* Poll received messages */
		sctk_ib_polling_t poll;
		POLL_INIT ( &poll );
		sctk_ib_cq_poll ( rail, device->recv_cq, config->wc_in_number, &poll, sctk_network_poll_recv );
	}

	return NULL;
}

void sctk_ib_fallback_init_polling_threads ( sctk_rail_info_t *rail )
{
	sctk_thread_t pidt;
	sctk_thread_attr_t attr;

	sctk_thread_attr_init ( &attr );
	sctk_thread_attr_setscope ( &attr, SCTK_THREAD_SCOPE_SYSTEM );

	sctk_user_thread_create ( &pidt, &attr, sctk_ib_fallback_send_polling_thread, ( void * ) rail );
	sctk_user_thread_create ( &pidt, &attr, sctk_ib_fallback_recv_polling_thread, ( void * ) rail );

	//#warning "Join threads"
}

void sctk_network_initialize_leader_task_fallback_ib ( sctk_rail_info_t *rail )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	sctk_ibuf_srq_check_and_post ( rail_ib );
	/* Initialize Async thread */
	sctk_ib_async_init ( rail_ib->rail );

	LOAD_CONFIG ( rail_ib );
	LOAD_DEVICE ( rail_ib );
	struct ibv_srq_attr mod_attr;
	int rc;
	mod_attr.srq_limit  = config->srq_credit_thread_limit;
	rc = ibv_modify_srq ( device->srq, &mod_attr, IBV_SRQ_LIMIT );
	assume ( rc == 0 );

	sctk_ib_fallback_init_polling_threads ( rail_ib->rail );
}


void sctk_network_initialize_task_fallback_ib ( sctk_rail_info_t *rail )
{

}

void sctk_network_finalize_task_fallback_ib ( sctk_rail_info_t *rail )
{

}

void sctk_network_init_fallback_ib ( sctk_rail_info_t *rail, int ib_rail_nb )
{
	/* XXX: memory never freed */
	char *network_name = sctk_malloc ( 256 );

	/* Infiniband Init */
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	memset ( rail_ib, 0, sizeof ( sctk_ib_rail_info_t ) );
	rail_ib->rail = rail;
	rail_ib->rail_nb = ib_rail_nb;

	/* Profiler init */
	sctk_ib_device_t *device;
	struct ibv_srq_init_attr srq_attr;

	/* Open config */
	sctk_ib_config_init ( rail_ib, "fallback" );

	/* Open device */
	device = sctk_ib_device_init ( rail_ib );
	sctk_ib_device_open ( rail_ib, -1 );

	/* Init Proctection Domain */
	sctk_ib_pd_init ( device );

	/* Init Completion Queues */
	device->send_comp_channel = sctk_ib_comp_channel_init ( device );
	device->recv_comp_channel = sctk_ib_comp_channel_init ( device );
	device->send_cq = sctk_ib_cq_init ( device, rail_ib->config, device->send_comp_channel );
	device->recv_cq = sctk_ib_cq_init ( device, rail_ib->config, device->recv_comp_channel );

	/* Init SRQ */
	srq_attr = sctk_ib_srq_init_attr ( rail_ib );
	sctk_ib_srq_init ( rail_ib, &srq_attr );

	/* Configure all channels */
	sctk_ib_eager_init ( rail_ib );

	/* Print config */
	if ( sctk_get_verbosity() >= 2 )
	{
		sctk_ib_config_print ( rail_ib );
	}

	sctk_ib_topology_init ( rail_ib );

	/* Initialize network */
	int init_cpu = sctk_get_cpu();
	sprintf ( network_name, "IB-MT (v2.0) Fallback %d/%d:%s - %dx %s (%d Gb/s) - %d]",
	          device->dev_index + 1, device->dev_nb, ibv_get_device_name ( device->dev ),
	          device->link_width, device->link_rate, device->data_rate, init_cpu );

	rail->initialize_task = sctk_network_initialize_task_fallback_ib;
	rail->initialize_leader_task = sctk_network_initialize_leader_task_fallback_ib;
	rail->finalize_task = sctk_network_finalize_task_fallback_ib;

	rail->connect_to = sctk_network_connection_to_ib;
	rail->connect_from = sctk_network_connection_from_ib;
	rail->send_message = sctk_network_send_message_ib;
	rail->notify_recv_message = sctk_network_notify_recv_message_ib;
	rail->notify_matching_message = sctk_network_notify_matching_message_ib;
	rail->notify_perform_message = sctk_network_notify_perform_message_ib;
	rail->notify_idle_message = sctk_network_notify_idle_message_ib;
	rail->notify_any_source_message = sctk_network_notify_any_source_message_ib;
	rail->network_name = network_name;

	sctk_ib_cm_connect_ring ( rail, rail->route, rail->route_init );
}
#endif
