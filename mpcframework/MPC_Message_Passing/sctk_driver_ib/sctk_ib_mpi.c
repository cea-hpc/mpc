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
#include <sctk_multirail_ib.h>

#include <sctk_ib_fallback.h>
#include "sctk_ib.h"
#include <sctk_ibufs.h>
#include <sctk_ibufs_rdma.h>
#include <sctk_ib_mmu.h>
#include <sctk_ib_config.h>
#include "sctk_ib_qp.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_async.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_buffered.h"
#include "sctk_ib_topology.h"
#include "sctk_ib_prof.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"

static void
sctk_network_send_message_ib ( sctk_thread_ptp_message_t *msg, sctk_rail_info_t *rail )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );
	sctk_endpoint_t *tmp;
	sctk_ib_route_info_t *route_data;
	sctk_ib_qp_t *remote;
	sctk_ibuf_t *ibuf;
	size_t size;
	char is_control_message = 0;
	specific_message_tag_t tag = SCTK_MSG_SPECIFIC_TAG ( msg );

	PROF_TIME_START ( rail, ib_send_message );

	sctk_nodebug ( "send message through rail %d from %d to %d (%d to %d) size:%lu tag:%d", rail->rail_number,
	               SCTK_MSG_SRC_PROCESS ( msg ),
	               SCTK_MSG_DEST_PROCESS ( msg ),
	               SCTK_MSG_SRC_TASK ( msg ),
	               SCTK_MSG_DEST_TASK ( msg ),
	               SCTK_MSG_SIZE ( msg ),
	               SCTK_MSG_TAG ( msg ) );

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

	/* Check if the remote task is in low mem mode */
	size = SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_body_t );

	if ( is_control_message && size + IBUF_GET_EAGER_SIZE > config->eager_limit )
	{
		sctk_error ( "MPC tries to send a control message without using the Eager protocol."
		             "This is not supported and MPC is going to exit ..." );
		sctk_abort();
	}

	sctk_ib_prof_qp_write ( remote->rank, size, sctk_get_time_stamp(), PROF_QP_SEND );

	/* *
	*
	*  We switch between available protocols
	*
	* */
	if ( ( ( sctk_ibuf_rdma_get_remote_state_rts ( remote ) == STATE_CONNECTED )
	        && ( size + IBUF_GET_EAGER_SIZE + IBUF_RDMA_GET_SIZE <= sctk_ibuf_rdma_get_eager_limit ( remote ) ) )
	        || ( size + IBUF_GET_EAGER_SIZE <= config->eager_limit ) )
	{
		sctk_nodebug ( "Eager" );
		ibuf = sctk_ib_eager_prepare_msg ( rail_ib, remote, msg, size, -1, is_control_message );

		/* Actually, it is possible to get a NULL pointer for ibuf. We falback to buffered */
		if ( ibuf == NULL )
			goto buffered;

		/* Send message */
		sctk_ib_qp_send_ibuf ( rail_ib, remote, ibuf, is_control_message );
		sctk_complete_and_free_message ( msg );
		PROF_INC ( rail_ib->rail, ib_eager_nb );

		/* Remote profiling */
		sctk_ibuf_rdma_update_remote ( rail_ib, remote, size );
		goto exit;
	}

buffered:

	/***** BUFFERED EAGER CHANNEL *****/
	if ( size <= config->buffered_limit )
	{
		sctk_nodebug ( "Buffered" );
		sctk_ib_buffered_prepare_msg ( rail, remote, msg, size );
		sctk_complete_and_free_message ( msg );
		PROF_INC ( rail_ib->rail, ib_buffered_nb );

		/* Remote profiling */
		sctk_ibuf_rdma_update_remote ( rail_ib, remote, size );

		goto exit;
	}

	/***** RDMA RENDEZVOUS CHANNEL *****/
rdma:
	{}
	sctk_nodebug ( "Size of message: %lu", size );
	ibuf = sctk_ib_rdma_prepare_req ( rail, remote, msg, size, -1 );

	/* Send message */
	sctk_ib_qp_send_ibuf ( rail_ib, remote, ibuf, 0 );
	sctk_ib_rdma_prepare_send_msg ( rail_ib, msg, size );
	PROF_INC ( rail_ib->rail, ib_rdma_nb );
exit:
	{}

	PROF_TIME_END ( rail, ib_send_message );
}


sctk_endpoint_t * sctk_on_demand_connection_ib( struct sctk_rail_info_s * rail , int dest )
{
	sctk_endpoint_t * tmp = NULL;

	sctk_nodebug ( "%d Trying to connect to process %d (remote:%p)", sctk_process_rank, dest, tmp );

	/* Wait until we reach the 'deconnected' state */
	tmp = sctk_route_dynamic_search ( dest, rail );

	if ( tmp )
	{
		sctk_endpoint_state_t state;
		state = sctk_endpoint_get_state ( tmp );
		sctk_nodebug ( "Got state %d", state );

		do
		{
			state = sctk_endpoint_get_state ( tmp );

			if ( state != STATE_DECONNECTED && state != STATE_CONNECTED 
			&& state != STATE_RECONNECTING )
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
	while ( sctk_endpoint_get_state ( tmp ) != STATE_CONNECTED )
	{
		sctk_network_notify_idle_message();

		if ( sctk_endpoint_get_state ( tmp ) != STATE_CONNECTED )
		{
			sctk_thread_yield();
		}
	}

	sctk_nodebug ( "Connected to process %d", dest );
	return tmp;
}



int sctk_network_poll_send_ibuf ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf,  const char from_cp, struct sctk_ib_polling_s *poll )
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
			not_reachable();
			break;
	}

	/* We do not need to release the buffer with the RDMA channel */
	if ( IBUF_GET_CHANNEL ( ibuf ) & RDMA_CHANNEL )
	{
		sctk_ibuf_release ( rail_ib, ibuf, 1 );
		return 0;
	}

	ib_assume ( IBUF_GET_CHANNEL ( ibuf ) & RC_SR_CHANNEL );

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

int sctk_network_poll_recv_ibuf ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf, const char from_cp, struct sctk_ib_polling_s *poll )
{
	PROF_TIME_START ( rail, ib_poll_recv_ibuf );
	const sctk_ib_protocol_t protocol = IBUF_GET_PROTOCOL ( ibuf->buffer );
	int release_ibuf = 1;
	const struct ibv_wc wc = ibuf->wc;

	sctk_nodebug ( "[%d] Recv ibuf:%p", rail->rail_number, ibuf );
	sctk_nodebug ( "Protocol received: %s", sctk_ib_protocol_print ( protocol ) );

	/* First we check if the message has an immediate data */
	if ( wc.wc_flags == IBV_WC_WITH_IMM )
	{
		sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
		sctk_ib_qp_t *remote = sctk_ib_qp_ht_find ( rail_ib, wc.qp_num );

		if ( wc.imm_data & IMM_DATA_RDMA_PIGGYBACK )
		{
			int piggyback = wc.imm_data - IMM_DATA_RDMA_PIGGYBACK;

			OPA_add_int ( &remote->rdma.pool->busy_nb[REGION_SEND], -piggyback );

		}
		else
			if ( wc.imm_data & IMM_DATA_RDMA_MSG )
			{
				sctk_ib_rdma_recv_done_remote_imm ( rail,
				                                    wc.imm_data - IMM_DATA_RDMA_MSG );
			}
			else
			{
				not_reachable();
			}

		/* Check if we are in a flush state */
		sctk_ibuf_rdma_check_flush_send ( rail_ib, remote );
		/* We release the buffer */
		release_ibuf = 1;

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
				release_ibuf = sctk_ib_rdma_poll_recv ( rail, ibuf );
				break;

			case SCTK_IB_BUFFERED_PROTOCOL:
				sctk_ib_buffered_poll_recv ( rail, ibuf );
				release_ibuf = 1;
				break;

			default:
				not_reachable();
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

	PROF_TIME_END ( rail, ib_poll_recv_ibuf );

	return 0;
}

static int sctk_network_poll_recv ( sctk_rail_info_t *rail, struct ibv_wc *wc,  struct sctk_ib_polling_s *poll )
{
	sctk_ibuf_t *ibuf = NULL;
	ibuf = ( sctk_ibuf_t * ) wc->wr_id;
	ib_assume ( ibuf );
	int dest_task = -1;

#if 0
	/* Get the remote associated to the ibuf */
	const sctk_ib_qp_t const *remote = sctk_ib_qp_ht_find ( rail_ib, wc->qp_num );
	ib_assume ( remote );
	sctk_ib_prof_qp_write ( remote->rank, wc->byte_len, sctk_get_time_stamp(), PROF_QP_RECV );
#endif

	/* Put a timestamp in the buffer. We *MUST* do it
	* before pushing the message to the list */
	ibuf->polled_timestamp = sctk_ib_prof_get_time_stamp();
	/* We *MUST* recopy some informations to the ibuf */
	ibuf->wc = *wc;
	ibuf->cq = SCTK_IB_RECV_CQ;
	ibuf->rail = rail;

	/* If this is an immediate message, we process it */
	if ( wc->wc_flags == IBV_WC_WITH_IMM )
	{
		return sctk_network_poll_recv_ibuf ( rail, ibuf, 0, poll );
	}

	if ( IBUF_GET_CHANNEL ( ibuf ) & RC_SR_CHANNEL )
	{
		dest_task = IBUF_GET_DEST_TASK ( ibuf->buffer );
		ibuf->cq = SCTK_IB_RECV_CQ;

		if ( sctk_ib_cp_handle_message ( rail, ibuf, dest_task, dest_task ) == 0 )
		{
			return sctk_network_poll_recv_ibuf ( rail, ibuf, 0, poll );
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return sctk_network_poll_recv_ibuf ( rail, ibuf, 0, poll );
	}
}

static int sctk_network_poll_send ( sctk_rail_info_t *rail, struct ibv_wc *wc, struct sctk_ib_polling_s *poll )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	sctk_ibuf_t *ibuf = NULL;
	ibuf = ( sctk_ibuf_t * )
	       wc->wr_id;

	ib_assume ( ibuf );
	int src_task = -1;
	int dest_task = -1;

	/* First we check if the message has an immediate data.
	* If it is, we decrease the number of pending requests and we check
	* if the remote is in a 'flushing' state*/
	/* CAREFUL: we cannot access the imm_data from the wc here
	* as it is only set for the receiver, not the sender */
	if ( wc->wc_flags & IBV_WC_WITH_IMM )
	{
		if ( ibuf->send_imm_data & IMM_DATA_RDMA_PIGGYBACK )
		{
			int current_pending;
			current_pending = sctk_ib_qp_fetch_and_sub_pending_data ( ibuf->remote, ibuf );
			sctk_ibuf_rdma_update_max_pending_data ( rail_ib, ibuf->remote,
			                                         current_pending );

			/* Check if we are in a flush state */
			OPA_decr_int ( &ibuf->remote->rdma.pool->busy_nb[REGION_RECV] );
			sctk_ibuf_rdma_check_flush_recv ( rail_ib, ibuf->remote );

			return 0;
		}
		else
			if ( ibuf->send_imm_data & IMM_DATA_RDMA_MSG )
			{
				/* Nothing to do here */
			}
			else
			{
				not_reachable();
			}
	}

	/* Put a timestamp in the buffer. We *MUST* do it
	* before pushing the message to the list */
	ibuf->polled_timestamp = sctk_ib_prof_get_time_stamp();

	/* We *MUST* recopy some informations to the ibuf */
	ibuf->wc = *wc;
	ibuf->cq = SCTK_IB_SEND_CQ;
	ibuf->rail = rail;

	/* Decrease the number of pending requests */
	int current_pending;
	current_pending = sctk_ib_qp_fetch_and_sub_pending_data ( ibuf->remote, ibuf );
	sctk_ibuf_rdma_update_max_pending_data ( rail_ib, ibuf->remote,
	                                         current_pending );

	if ( IBUF_GET_CHANNEL ( ibuf ) & RC_SR_CHANNEL )
	{
		src_task = IBUF_GET_SRC_TASK ( ibuf->buffer );
		dest_task = IBUF_GET_DEST_TASK ( ibuf->buffer );

		/* We still check the dest_task. If it is -1, this is a process_specific
		* message, so we need to handle the message asap */
		if ( sctk_ib_cp_handle_message ( rail, ibuf, dest_task, src_task ) == 0 )
		{
			return sctk_network_poll_send_ibuf ( rail, ibuf, 0, poll );
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return sctk_network_poll_send_ibuf ( rail, ibuf, 0, poll );
	}
}

/* Count how many times the vp is entered to the polling function. We
 * allow recursive calls to the polling function */


pthread_mutex_t poll_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  poll_cond = PTHREAD_COND_INITIALIZER;
int ib_thread_first_to_poll = 1;
__thread int ib_thread_has_cq_lock  = 0;
int retry;


static void __release_cq_broadcast ()
{
	pthread_mutex_lock ( &poll_mutex );
	ib_thread_first_to_poll = 1;
	pthread_cond_broadcast ( &poll_cond );
	pthread_mutex_unlock ( &poll_mutex );
}

static void __release_cq_signal ()
{
	pthread_mutex_lock ( &poll_mutex );
	ib_thread_first_to_poll = 1;
	pthread_cond_signal ( &poll_cond );
	pthread_mutex_unlock ( &poll_mutex );
}

//#define SCTK_IB_CQ_MUTEX

void sctk_network_poll_all_cq ( sctk_rail_info_t *rail, sctk_ib_polling_t *poll, int polling_task_id, int blocking )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_DEVICE ( rail_ib );
	LOAD_CONFIG ( rail_ib );

#ifdef SCTK_IB_CQ_MUTEX
	pthread_mutex_lock ( &poll_mutex );

recheck:

	if ( ib_thread_first_to_poll == 1 || ib_thread_has_cq_lock )
	{
		retry = 0;
		ib_thread_first_to_poll = 0;
		pthread_mutex_unlock ( &poll_mutex );
		ib_thread_has_cq_lock ++;

		do
		{
#endif
			SCTK_PROFIL_START ( ib_poll_cq );
			/* Poll received messages */
			sctk_ib_cq_poll ( rail, device->recv_cq, config->wc_in_number, poll, sctk_network_poll_recv );
			/* Poll sent messages */
			sctk_ib_cq_poll ( rail, device->send_cq, config->wc_out_number, poll, sctk_network_poll_send );
			SCTK_PROFIL_END ( ib_poll_cq );

#ifdef SCTK_IB_CQ_MUTEX
#if 1

			if ( sctk_net_is_mode_hybrid() )
			{
				/* The condition must change according if we are in MT or not */
				if ( POLL_GET_RECV ( poll ) > 0 || polling_task_id < 0 )
				{
					retry = 0;
					__release_cq_broadcast();
				}
				else
					if ( sctk_ib_cp_get_nb_pending_msg() != 0 )
					{
						/* If nothing to poll from the pending list, we retry */
						retry = 1;
						__release_cq_broadcast();
					}
					else
					{
						if ( blocking )
						{
							retry = 1;
						}
						else
						{
							retry = 2;
							__release_cq_signal();
						}
					}
			}
			else
			{
				retry = 0;
				__release_cq_broadcast();
			}
		}
		while ( retry == 1 );

		ib_thread_has_cq_lock --;
	}
	else
		if ( blocking == 0 )
		{
			/* We leave the polling function as soon as possible */
			pthread_mutex_unlock ( &poll_mutex );
		}
		else
		{
			/*  We are in blocked mode, so we wait ... */
			pthread_cond_wait ( &poll_cond, &poll_mutex );

			/* If a recheck is asked, we return to the beginning of the function without
			* releasing the lock */
			if ( retry == 2 )
				goto recheck;

			pthread_mutex_unlock ( &poll_mutex );
		}

#else
		}

		while ( 0 )
			;

		ib_thread_has_cq_lock --;
		__release_cq_broadcast();
	}
	else
	{
		/*  We are in blocked mode, so we wait ... */
		pthread_cond_wait ( &poll_cond, &poll_mutex );
		/* If a recheck is asked, we return to the beginning of the function without
		* releasing the lock */
		pthread_mutex_unlock ( &poll_mutex );
	}

#endif
#endif


	TODO ( "Should be reactivated for Hetero collective" )
	//  sctk_ib_cp_poll_global_list(rail, poll);
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
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );
	struct sctk_ib_polling_s poll;

#if 0
	{
		sctk_ib_qp_t *remote;
		sctk_ib_route_info_t *route_data;
		sctk_endpoint_t *route =  sctk_get_route_to_process_no_ondemand ( remote_process, rail );
		ib_assume ( route );
		int ret;

		route_data = &route->data.ib;
		remote = route_data->remote;

		/*  Poll messages fistly on RDMA. If no message has been found,
		*       * we continue to poll SR channel */
		ret = sctk_ib_rdma_eager_poll_remote ( rail_ib, remote );

		if ( ret == REORDER_FOUND_EXPECTED )
			return;
	}
#else

	int ret;
	sctk_ib_rdma_eager_walk_remotes ( rail_ib, sctk_ib_rdma_eager_poll_remote, &ret );

	if ( ret == REORDER_FOUND_EXPECTED )
	{
		return;
	}

#endif

	POLL_INIT ( &poll );
	SCTK_PROFIL_START ( ib_perform_all );
	sctk_ib_cp_poll ( rail, &poll, polling_task_id );

	/* If nothing to poll, we poll the CQ */
	if ( POLL_GET_RECV ( &poll ) == 0 )
	{
		POLL_INIT ( &poll );
		sctk_network_poll_all_cq ( rail, &poll, polling_task_id, blocking );

		/* If the polling returned something or someone already inside the function,
		* we retry to poll the pending lists */
		if ( POLL_GET_RECV_CQ ( &poll ) != 0 )
		{
			POLL_INIT ( &poll );
			sctk_ib_cp_poll ( rail, &poll, polling_task_id );
		}
	}

	SCTK_PROFIL_END ( ib_perform_all );
}

static void sctk_network_notify_idle_message_ib ( sctk_rail_info_t *rail )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );
	struct sctk_ib_polling_s poll;
	int polling_task_id = sctk_get_task_rank();

	int ret;
	sctk_ib_rdma_eager_walk_remotes ( rail_ib, sctk_ib_rdma_eager_poll_remote, &ret );

	if ( ret == REORDER_FOUND_EXPECTED )
		return;

	if ( polling_task_id >= 0 )
	{
		POLL_INIT ( &poll );
		sctk_ib_cp_poll ( rail, &poll, polling_task_id );
	}

	/* If nothing to poll, we poll the CQ */
	if ( POLL_GET_RECV ( &poll ) == 0 )
	{
		POLL_INIT ( &poll );
		sctk_network_poll_all_cq ( rail, &poll, polling_task_id, 0 );

		/* If the polling returned something or someone already inside the function,
		* we retry to poll the pending lists */
		if ( POLL_GET_RECV_CQ ( &poll ) != 0 )
		{
			if ( polling_task_id >= 0 )
			{
				POLL_INIT ( &poll );
				sctk_ib_cp_poll ( rail, &poll, polling_task_id );
			}
		}
	}

	if ( config->steal > 0 )
	{
		sctk_ib_cp_steal ( rail, &poll, 0 );
	}
}

static void sctk_network_notify_any_source_message_ib ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );
	struct sctk_ib_polling_s poll;

	{
		int ret;
		sctk_ib_rdma_eager_walk_remotes ( rail_ib, sctk_ib_rdma_eager_poll_remote, &ret );

		if ( ret == REORDER_FOUND_EXPECTED )
			return;
	}


	POLL_INIT ( &poll );
	sctk_ib_cp_poll ( rail, &poll, polling_task_id );

	/* If nothing to poll, we poll the CQ */
	if ( POLL_GET_RECV ( &poll ) == 0 )
	{
		POLL_INIT ( &poll );
		sctk_network_poll_all_cq ( rail, &poll, polling_task_id, blocking );

		/* If the polling returned something or someone already inside the function,
		* we retry to poll the pending lists */
		if ( POLL_GET_RECV_CQ ( &poll ) != 0
		        || POLL_GET_RECV_CQ ( &poll ) == POLL_CQ_BUSY )
		{
			POLL_INIT ( &poll );
			sctk_ib_cp_poll ( rail, &poll, polling_task_id );
		}
	}
}

static void sctk_network_connection_to_ib ( int from, int to, sctk_rail_info_t *rail )
{
	sctk_ib_cm_connect_to ( from, to, rail );
}

static void
sctk_network_connection_from_ib ( int from, int to, sctk_rail_info_t *rail )
{
	sctk_ib_cm_connect_from ( from, to, rail );
}

void sctk_network_initialize_leader_task_mpi_ib ( sctk_rail_info_t *rail )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	/* Fill SRQ with buffers */
	sctk_ibuf_srq_check_and_post ( rail_ib );

	/* Initialize Async thread */
	sctk_ib_async_init ( rail_ib->rail );

	/* Initialize collaborative polling */
	sctk_ib_cp_init ( rail_ib );

	LOAD_CONFIG ( rail_ib );
	LOAD_DEVICE ( rail_ib );
	struct ibv_srq_attr mod_attr;
	int rc;
	mod_attr.srq_limit  = config->srq_credit_thread_limit;
	rc = ibv_modify_srq ( device->srq, &mod_attr, IBV_SRQ_LIMIT );
	ib_assume ( rc == 0 );
}

void sctk_network_initialize_task_mpi_ib ( sctk_rail_info_t *rail )
{

}

void sctk_network_finalize_task_mpi_ib ( sctk_rail_info_t *rail )
{
	sctk_ib_prof_finalize ( &rail->network.ib );
}

void sctk_network_init_mpi_ib ( sctk_rail_info_t *rail, int ib_rail_nb )
{
	/* XXX: memory never freed */
	char *network_name = sctk_malloc ( 256 );

	/* Infiniband Init */
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	memset ( rail_ib, 0, sizeof ( sctk_ib_rail_info_t ) );
	rail_ib->rail = rail;
	rail_ib->rail_nb = ib_rail_nb;

	/* Profiler init */
	sctk_ib_prof_mem_init ( rail_ib );
	sctk_ib_device_t *device;
	struct ibv_srq_init_attr srq_attr;

	/* Open config */
	sctk_ib_config_init ( rail_ib, "MPI" );

	/* Open device */
	device = sctk_ib_device_init ( rail_ib );

	/* Automatically open the closest IB HCA */
	//  sctk_ib_device_open(rail_ib, (3 - ib_rail_nb) % 4);
	sctk_ib_device_open ( rail_ib, -1 );

	/* Init Proctection Domain */
	sctk_ib_pd_init ( device );

	/* Init Completion Queues */
	device->send_cq = sctk_ib_cq_init ( device, rail_ib->config, NULL );
	device->recv_cq = sctk_ib_cq_init ( device, rail_ib->config, NULL );

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
	sprintf ( network_name, "IB-MT (v2.0) MPI      %d/%d:%s - %dx %s (%d Gb/s) - %d]",
	          device->dev_index + 1, device->dev_nb, ibv_get_device_name ( device->dev ),
	          device->link_width, device->link_rate, device->data_rate, init_cpu );

#ifdef IB_DEBUG

	if ( sctk_process_rank == 0 )
	{
	fprintf ( stderr, SCTK_COLOR_RED_BOLD ( WARNING: MPC debug mode activated. Your job * MAY * be * VERY * slow! ) "\n" );
	}

#endif

	rail->initialize_task = sctk_network_initialize_task_mpi_ib;
	rail->initialize_leader_task = sctk_network_initialize_leader_task_mpi_ib;
	rail->finalize_task = sctk_network_finalize_task_mpi_ib;

	rail->connect_to = sctk_network_connection_to_ib;
	rail->connect_from = sctk_network_connection_from_ib;
	rail->send_message = sctk_network_send_message_ib;
	rail->notify_recv_message = sctk_network_notify_recv_message_ib;
	rail->notify_matching_message = sctk_network_notify_matching_message_ib;
	rail->notify_perform_message = sctk_network_notify_perform_message_ib;
	rail->notify_idle_message = sctk_network_notify_idle_message_ib;
	rail->notify_any_source_message = sctk_network_notify_any_source_message_ib;
	rail->network_name = network_name;
	rail->on_demand_connection = sctk_on_demand_connection_ib;

	sctk_ib_cm_connect_ring ( rail, rail->route, rail->route_init );
}
#endif
