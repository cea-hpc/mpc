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

/** array of VPS, for remembering __thread vars to reset when rail is re-enabled */
volatile char *vps_reset = NULL;


static void sctk_network_send_message_ib_endpoint ( sctk_thread_ptp_message_t *msg , sctk_endpoint_t *endpoint )
{
	sctk_rail_info_t * rail = endpoint->rail;
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG ( rail_ib );
	sctk_ib_route_info_t *route_data;
	sctk_ib_qp_t *remote;
	sctk_ibuf_t *ibuf;
	size_t size;


	assume( sctk_endpoint_get_state ( endpoint ) == STATE_CONNECTED );

	route_data = &endpoint->data.ib;
	remote = route_data->remote;

	/* Check if the remote task is in low mem mode */
	size = SCTK_MSG_SIZE ( msg ) + sizeof ( sctk_thread_ptp_message_body_t );

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
		sctk_debug ( "Eager" );
		ibuf = sctk_ib_eager_prepare_msg ( rail_ib, remote, msg, size, -1, sctk_message_class_is_process_specific ( SCTK_MSG_SPECIFIC_CLASS( msg ) ) );

		/* Actually, it is possible to get a NULL pointer for ibuf. We falback to buffered */
		if ( ibuf == NULL )
			goto buffered;

		/* Send message */
		sctk_ib_qp_send_ibuf ( rail_ib, remote, ibuf );
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
		sctk_debug ( "Buffered" );
		sctk_ib_buffered_prepare_msg ( rail, remote, msg, size );
		sctk_complete_and_free_message ( msg );
		PROF_INC ( rail_ib->rail, ib_buffered_nb );

		/* Remote profiling */
		sctk_ibuf_rdma_update_remote ( rail_ib, remote, size );

		goto exit;
	}

	/***** RDMA RENDEZVOUS CHANNEL *****/
	sctk_debug ( "Size of message: %lu", size );
	ibuf = sctk_ib_rdma_rendezvous_prepare_req ( rail, remote, msg, size, -1 );

	/* Send message */
	sctk_ib_qp_send_ibuf ( rail_ib, remote, ibuf );
	sctk_ib_rdma_rendezvous_prepare_send_msg ( rail_ib, msg, size );
	PROF_INC ( rail_ib->rail, ib_rdma_nb );
exit:
	{}

  // PROF_TIME_END ( rail, ib_send_message );
}


sctk_endpoint_t * sctk_on_demand_connection_ib( struct sctk_rail_info_s * rail , int dest )
{
	sctk_endpoint_t * tmp = NULL;

	/* Wait until we reach the 'deconnected' state */
	tmp = sctk_rail_get_dynamic_route_to_process ( rail, dest );

	if ( tmp )
	{
		sctk_endpoint_state_t state;
		state = sctk_endpoint_get_state ( tmp );

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

	/* We send the request using the signalization rail */
	tmp = sctk_ib_cm_on_demand_request ( dest, rail );
	assume ( tmp );

	/* If route not connected, so we wait for until it is connected */
	while ( sctk_endpoint_get_state ( tmp ) != STATE_CONNECTED )
	{
	//	sctk_warning("YA WAIT");
		
		
		sctk_network_notify_idle_message();

		if ( sctk_endpoint_get_state ( tmp ) != STATE_CONNECTED )
		{
			sctk_thread_yield();
		}
	}

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
			CRASH();
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
		{
			if ( wc.imm_data & IMM_DATA_RDMA_MSG )
			{
				sctk_ib_rdma_recv_done_remote_imm ( rail,
				                                    wc.imm_data - IMM_DATA_RDMA_MSG );
			}
			else
			{
				not_reachable();
			}
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

	if ( (IBUF_GET_CHANNEL ( ibuf ) & RC_SR_CHANNEL))
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


static pthread_mutex_t poll_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  poll_cond = PTHREAD_COND_INITIALIZER;
static int retry;


static void __release_cq_broadcast ()
{
	pthread_mutex_lock ( &poll_mutex );
	pthread_cond_broadcast ( &poll_cond );
	pthread_mutex_unlock ( &poll_mutex );
}

static void __release_cq_signal ()
{
	pthread_mutex_lock ( &poll_mutex );
	pthread_cond_signal ( &poll_cond );
	pthread_mutex_unlock ( &poll_mutex );
}

//#define SCTK_IB_CQ_MUTEX

void sctk_network_poll_all_cq ( sctk_rail_info_t *rail, sctk_ib_polling_t *poll, int polling_task_id, int blocking )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_DEVICE ( rail_ib );
	LOAD_CONFIG ( rail_ib );


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

	sctk_ib_cp_poll_global_list(rail, poll);
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

	int ret;
	sctk_ib_rdma_eager_walk_remotes ( rail_ib, sctk_ib_rdma_eager_poll_remote, &ret );

	if ( ret == REORDER_FOUND_EXPECTED )
		return;

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

__thread int idle_poll_all = 0;
__thread int idle_poll_freq = 100;
static void sctk_network_notify_idle_message_ib ( sctk_rail_info_t *rail )
{
    sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
    LOAD_CONFIG ( rail_ib );
    struct sctk_ib_polling_s poll;

    if(rail->state != SCTK_RAIL_ST_ENABLED)
	    return;

	idle_poll_all++;

    if( idle_poll_all != idle_poll_freq )
        return;
    else
        idle_poll_all = 0;

    if( idle_poll_freq < idle_poll_all )
        idle_poll_all = 0;

    int ret;
    sctk_ib_rdma_eager_walk_remotes ( rail_ib, sctk_ib_rdma_eager_poll_remote, &ret );

    if ( ret == REORDER_FOUND_EXPECTED )
    {
        idle_poll_freq = 100;
        return;
    }
    else
    {
        idle_poll_freq *= 4;
    }

    int polling_task_id = sctk_get_task_rank();
    POLL_INIT ( &poll );
    sctk_network_poll_all_cq ( rail, &poll, polling_task_id, 0 );

    /* If the polling returned something or someone already inside the function,
    * we retry to poll the pending lists */
    if ( POLL_GET_RECV_CQ ( &poll ) != 0 )
    {
        idle_poll_freq = 100;
        if ( polling_task_id >= 0 )
        {
            POLL_INIT ( &poll );
            sctk_ib_cp_poll ( rail, &poll, polling_task_id );
        }
    }
    else
    {
        idle_poll_freq *= 4;
    }

    if( 150000 < idle_poll_freq )
            idle_poll_freq = 150000;
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



int sctk_send_message_from_network_mpi_ib ( sctk_thread_ptp_message_t *msg )
{
	int ret = sctk_send_message_from_network_reorder ( msg );

	if ( ret == REORDER_NO_NUMBERING )
	{
		/*
		  No reordering
		*/
		sctk_send_message_try_check ( msg, 1 );
	}

	return ret;
}


void sctk_connect_on_demand_mpi_ib( struct sctk_rail_info_s * rail , int dest )
{
	sctk_endpoint_t * new_endpoint = sctk_on_demand_connection_ib ( rail, dest );
        /* add_dynamic_route() is not necessary here, as its is done
         * by the IB handshake protocol deeply in the function above */
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

void sctk_network_initialize_task_mpi_ib ( sctk_rail_info_t *rail, int taskid, int vp )
{
	sctk_network_initialize_task_collaborative_ib(rail, taskid, vp);
}

void sctk_network_finalize_task_mpi_ib ( sctk_rail_info_t *rail, int taskid, int vp )
{
	sctk_ib_prof_finalize ( &rail->network.ib );
	sctk_network_finalize_task_collaborative_ib(rail, taskid, vp);
}


/* This hook is called by the allocator when freing a segment */
void sctk_network_memory_free_hook_ib ( void * ptr, size_t size )
{
	sctk_ib_mmu_unpin(  ptr, size);
}


/* Pinning */

void sctk_ib_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size )
{
	/* Fill entry */
	list->rail_id = rail->rail_number;
	/* Save the pointer (to free the block returned by sctk_ib_mmu_entry_new) */
	list->pin.ib.p_entry = sctk_ib_mmu_pin( &rail->network.ib, addr, size);
	
	/* Save it inside the struct to allow serialization */
	memcpy( &list->pin.ib.mr , list->pin.ib.p_entry->mr, sizeof( struct ibv_mr ) );
}

void sctk_ib_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list )
{
	sctk_nodebug("Unpin %p at %p size %ld releaseon %d", list->pin.ib.p_entry, list->pin.ib.p_entry->addr, list->pin.ib.p_entry->size, list->pin.ib.p_entry->free_on_relax );
	
	sctk_ib_mmu_relax( list->pin.ib.p_entry );
	list->pin.ib.p_entry = NULL;
	memset( &list->pin.ib.mr, 0, sizeof( struct ibv_mr ) );
	list->rail_id = -1;
}

void sctk_network_finalize_mpi_ib( sctk_rail_info_t *rail)
{
	sctk_network_set_ib_unused();
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_CONFIG (rail_ib);
	LOAD_DEVICE(rail_ib);

	/* Clear the QPs                                              */
	sctk_ib_qp_free_all(rail_ib);

	/* Stop the async event polling thread                        */
	sctk_ib_async_finalize(rail);

	/* collaborative polling shutdown (from leader_initialize)    */
	sctk_ib_cp_finalize(rail_ib);

	/* - Free IB topology                                         */
	sctk_ib_topology_free(rail_ib);

	/* - Flush the eager entries (sctk_ib_eager_init)             */
	sctk_ib_eager_finalize(rail_ib);

	/* - Destroy the Shared Receive Queue (sctk_ib_srq_init)      */
	sctk_ib_srq_free(rail_ib);

	/* - Free completion queues (send & recv) sctk_ib_cq_init)    */
	sctk_ib_cq_free(device->send_cq); device->send_cq = NULL;
	sctk_ib_cq_free(device->recv_cq); device->recv_cq = NULL;

	/* - Free the protected domain (sctk_ib_pd_init)              */
	sctk_ib_pd_free(device);

	/* - Free MMU */
	sctk_ib_mmu_release();

	/* - unload IB device (sctk_ib_device_open)                   */
	/* - close IB device struct (sctk_ib_device_init)             */
	sctk_ib_device_close(&rail->network.ib);

	/* - Reset ib config struct (sctk_ib_config_init)             */
	config = NULL;

	memset(vps_reset, 0, sizeof(char) * sctk_get_cpu_number());

}

void sctk_network_init_mpi_ib ( sctk_rail_info_t *rail )
{
	sctk_network_set_ib_used();
	
	/* XXX: memory never freed */
	char *network_name = sctk_malloc ( 256 );

	/* Retrieve config pointers */
	struct sctk_runtime_config_struct_net_rail *rail_config = rail->runtime_config_rail;
	struct sctk_runtime_config_struct_net_driver_config *driver_config = rail->runtime_config_driver_config;

	/* init the first time the rail is enabled */
	if(!vps_reset)
	{
		int nbvps = sctk_get_cpu_number();
		vps_reset = sctk_malloc(nbvps * sizeof(char));
		assert(vps_reset);
		memset(vps_reset, 0, sizeof(char) * nbvps);
	}


	/* Register topology */
	sctk_rail_init_route ( rail, rail_config->topology, NULL );

	/* Infiniband Init */
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	memset ( rail_ib, 0, sizeof ( sctk_ib_rail_info_t ) );
	rail_ib->rail = rail;
	rail_ib->rail_nb = rail->rail_number;

	/* Profiler init */
	sctk_ib_prof_mem_init ( rail_ib );
	sctk_ib_device_t *device;
	struct ibv_srq_init_attr srq_attr;

	/* Open config */
	sctk_ib_config_init ( rail_ib, "MPI" );

	/* Open device */
	device = sctk_ib_device_init ( rail_ib );

	/* Automatically open the closest IB HCA */
	sctk_ib_device_open ( rail_ib, rail_config->device );

	/* Init the MMU (done once) */
	sctk_ib_mmu_init();

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

	sctk_ib_topology_init_rail ( rail_ib );
	sctk_ib_topology_init_task ( rail, sctk_thread_get_vp() );
	
	/* Initialize network */
	int init_cpu = sctk_get_cpu();
        sprintf(network_name, "IB-MT - %dx %s (%d Gb/s)", device->link_width,device->link_rate, device->data_rate);

#ifdef IB_DEBUG

	if ( sctk_process_rank == 0 )
	{
		fprintf ( stderr, SCTK_COLOR_RED_BOLD ( WARNING: MPC debug mode activated. Your job * MAY * be * VERY * slow! ) "\n" );
	}

#endif

	rail->initialize_task = sctk_network_initialize_task_mpi_ib;
	rail->finalize_task = sctk_network_finalize_task_mpi_ib;

	sctk_network_initialize_leader_task_mpi_ib ( rail );

	
	rail->control_message_handler = sctk_ib_cm_control_message_handler;

	rail->connect_to = sctk_network_connection_to_ib;
	rail->connect_from = sctk_network_connection_from_ib;
	rail->connect_on_demand = sctk_connect_on_demand_mpi_ib;
	rail->driver_finalize = sctk_network_finalize_mpi_ib;
	
	rail->send_message_endpoint = sctk_network_send_message_ib_endpoint;

	rail->notify_recv_message = sctk_network_notify_recv_message_ib;
	rail->notify_matching_message = sctk_network_notify_matching_message_ib;
	rail->notify_perform_message = sctk_network_notify_perform_message_ib;
	rail->notify_idle_message = sctk_network_notify_idle_message_ib;
	rail->notify_any_source_message = sctk_network_notify_any_source_message_ib;
	
	/* PIN */
	rail->rail_pin_region = sctk_ib_pin_region;
	rail->rail_unpin_region = sctk_ib_unpin_region;
	
	/* RDMA */
	rail->rdma_write = sctk_ib_rdma_write;
	rail->rdma_read = sctk_ib_rdma_read;
	
	rail->rdma_fetch_and_op_gate = sctk_ib_rdma_fetch_and_op_gate;
	rail->rdma_fetch_and_op = sctk_ib_rdma_fetch_and_op;
	
	rail->rdma_cas_gate = sctk_ib_rdma_cas_gate;
	rail->rdma_cas = sctk_ib_rdma_cas;
	
	rail->network_name = network_name;

	rail->send_message_from_network = sctk_send_message_from_network_mpi_ib;

	/* Boostrap the ring only if required */
	if( rail->requires_bootstrap_ring )
	{
		/* Bootstrap a ring on this network */
		sctk_ib_cm_connect_ring ( rail );
	}

}
#endif
