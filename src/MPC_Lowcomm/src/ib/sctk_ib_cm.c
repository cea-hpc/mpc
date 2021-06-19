/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
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

#include "sctk_ib.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_eager.h"

#include "sctk_ibufs_rdma.h"
#include "mpc_common_asm.h"

#include <mpc_launch_pmi.h>

/* IB debug macros */
#if defined MPC_LOWCOMM_IB_MODULE_NAME
#error "MPC_LOWCOMM_IB_MODULE already defined"
#endif
#define MPC_LOWCOMM_IB_MODULE_DEBUG
#define MPC_LOWCOMM_IB_MODULE_NAME    "CM"
#include "sctk_ib_toolkit.h"

#include "mpc_lowcomm_msg.h"
#include "sctk_control_messages.h"
#include "sctk_rail.h"

/*-----------------------------------------------------------
*  Interface for Infiniband connexions.
*
*  - RACE CONDITITONS: for avoid race conditions between peers, a
*  process responds to a request with a positive ack only when
*  it has not initiated a request or its rank is higher than the
*  source rank
*----------------------------------------------------------*/

struct sctk_ib_qp_s;

/*-----------------------------------------------------------
*  FUNCTIONS
*----------------------------------------------------------*/

/* Change the state of a remote process */
static void __update_endpoint_state(sctk_rail_info_t *rail, _mpc_lowcomm_endpoint_t *endpoint)
{
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;

	_mpc_lowcomm_endpoint_state_t state_before_connexion;

	state_before_connexion = _mpc_lowcomm_endpoint_get_state(endpoint);

	switch(_mpc_lowcomm_endpoint_get_type(endpoint) )
	{
		case _MPC_LOWCOMM_ENDPOINT_STATIC: /* Static route */
			_MPC_LOWCOMM_ENDPOINT_LOCK(endpoint);
			_mpc_lowcomm_endpoint_set_state(endpoint, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
			_MPC_LOWCOMM_ENDPOINT_UNLOCK(endpoint);

			sctk_ib_debug("[%d] Static QP connected to process %d", rail->rail_number, remote->rank);
			break;

		case _MPC_LOWCOMM_ENDPOINT_DYNAMIC: /* Dynamic route */
			_MPC_LOWCOMM_ENDPOINT_LOCK(endpoint);
			_mpc_lowcomm_endpoint_set_state(endpoint, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
			_MPC_LOWCOMM_ENDPOINT_UNLOCK(endpoint);

			if(state_before_connexion == _MPC_LOWCOMM_ENDPOINT_RECONNECTING)
			{
				sctk_ib_debug("[%d] OD QP reconnected to process %d", rail->rail_number, remote->rank);
			}
			else
			{
				sctk_ib_debug("[%d] OD QP connected to process %d", rail->rail_number, remote->rank);
			}



			break;

		default:
			not_reachable();
			mpc_common_debug_abort(); /* Not reachable */
	}
}

/* Change the state of a remote process */
static void __set_endpoint_ready_to_receive(sctk_rail_info_t *rail,
                                            _mpc_lowcomm_endpoint_t *endpoint,
                                            enum sctk_ib_cm_change_state_type_e type)
{
	struct sctk_ib_qp_s *         remote = endpoint->data.ib.remote;
	_mpc_lowcomm_endpoint_state_t state;
	char txt[256];
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	LOAD_DEVICE(rail_ib);

	if(type == CONNECTION)
	{
		state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTING, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
		sprintf(txt, MPC_COLOR_GREEN(RTR CONNECTED) );
		assume(state == _MPC_LOWCOMM_ENDPOINT_CONNECTING);
	}
	else
	if(type == RESIZING)
	{
		if(remote->rdma.pool->resizing_request.recv_keys.nb == 0)
		{
			/* We deconnect */
			/* Change the state of the route */
			state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_FLUSHED, _MPC_LOWCOMM_ENDPOINT_DECONNECTED);
			sprintf(txt, MPC_COLOR_RED(RTR DECONNECTED) );
		}
		else
		{
			/* We connect */
			state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_FLUSHED, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
			sprintf(txt, MPC_COLOR_BLUE(RTR RESIZED) );
		}

		assume(state == _MPC_LOWCOMM_ENDPOINT_FLUSHED);
	}
	else
	{
		not_reachable();
	}

	sctk_ib_debug("%s with process %d (nb:%d->%d, size:%d->%d rdma_connections:%d allocated:%.0fko)", txt,
	              remote->rank,
	              remote->rdma.pool->region[REGION_RECV].nb_previous,
	              remote->rdma.pool->region[REGION_RECV].nb,
	              remote->rdma.pool->region[REGION_RECV].size_ibufs_previous,
	              remote->rdma.pool->region[REGION_RECV].size_ibufs,
	              device->eager_rdma_connections,
	              (double)_mpc_lowcomm_ib_ibuf_rdma_region_size_get(remote) / 1024.0);
}

/* Change the state of a remote process */
static void __set_endpoint_ready_to_send(sctk_rail_info_t *rail,
                                         _mpc_lowcomm_endpoint_t *endpoint,
                                         enum sctk_ib_cm_change_state_type_e type)
{
	struct sctk_ib_qp_s *remote  = endpoint->data.ib.remote;
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	LOAD_DEVICE(rail_ib);
	_mpc_lowcomm_endpoint_state_t state;
	char txt[256];

	if(type == CONNECTION)
	{
		state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTING, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
		sprintf(txt, MPC_COLOR_GREEN(RTS CONNECTED) );
		assume(state == _MPC_LOWCOMM_ENDPOINT_CONNECTING);
	}
	else
	if(type == RESIZING)
	{
		if(remote->rdma.pool->resizing_request.send_keys.nb == 0)
		{
			/* We deconnect */
			state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_FLUSHED, _MPC_LOWCOMM_ENDPOINT_DECONNECTED);
			sprintf(txt, MPC_COLOR_RED(RTS DISCONNECTED) );
		}
		else
		{
			/* We connect */
			state = _mpc_lowcomm_ib_ibuf_rdma_cas_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_FLUSHED, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
			sprintf(txt, MPC_COLOR_BLUE(RTS RESIZED) );
		}

		assume(state == _MPC_LOWCOMM_ENDPOINT_FLUSHED);
	}
	else
	{
		not_reachable();
	}

	/* Change the state of the route */
	sctk_ib_debug("%s with process %d (nb:%d->%d, size:%d->%d, rdma_connections:%d  allocated:%0.fko ts:%f)", txt,
	              remote->rank,
	              remote->rdma.pool->region[REGION_SEND].nb_previous,
	              remote->rdma.pool->region[REGION_SEND].nb,
	              remote->rdma.pool->region[REGION_SEND].size_ibufs_previous,
	              remote->rdma.pool->region[REGION_SEND].size_ibufs,
	              device->eager_rdma_connections,
	              (double)_mpc_lowcomm_ib_ibuf_rdma_region_size_get(remote) / 1024.0,
	              mpc_arch_get_timestamp_gettimeofday() - remote->rdma.creation_timestamp);
}

/*-----------------------------------------------------------
*  STATIC CONNEXIONS : intialization to a ring topology
*  Messages are exchange in a string format. This is because
*  we are using the PMI library.
*----------------------------------------------------------*/
void sctk_ib_cm_connect_ring(sctk_rail_info_t *rail)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	int dest_rank;
	int src_rank;
	_mpc_lowcomm_endpoint_t *        endpoint_src, *endpoint_dest;
	_mpc_lowcomm_endpoint_info_ib_t *route_dest, *route_src;
	sctk_ib_cm_qp_connection_t       keys;

	ib_assume(rail->send_message_from_network != NULL);

	dest_rank = (mpc_common_get_process_rank() + 1) % mpc_common_get_process_count();
	src_rank  = (mpc_common_get_process_rank() + mpc_common_get_process_count() - 1) % mpc_common_get_process_count();

	if(mpc_common_get_process_count() > 2)
	{
		/* XXX: Set QP in a Ready-To-Send mode. Ideally, we should check that
		 * the remote QP has sent an ack */

		/* create remote for dest */
		endpoint_dest = sctk_ib_create_remote();
		sctk_ib_init_remote(dest_rank, rail, endpoint_dest, 0);
		route_dest = &endpoint_dest->data.ib;

		/* create remote for src */
		endpoint_src = sctk_ib_create_remote();
		sctk_ib_init_remote(src_rank, rail, endpoint_src, 0);
		route_src = &endpoint_src->data.ib;

		sctk_ib_qp_keys_send(rail_ib, route_dest->remote);

		mpc_launch_pmi_barrier();

		/* change state to RTR */

		keys = sctk_ib_qp_keys_recv(rail_ib, src_rank);
		sctk_ib_qp_allocate_rtr(rail_ib, route_src->remote, &keys);
		sctk_ib_qp_allocate_rts(rail_ib, route_src->remote);
		sctk_ib_qp_keys_send(rail_ib, route_src->remote);
		mpc_launch_pmi_barrier();

		keys = sctk_ib_qp_keys_recv(rail_ib, dest_rank);
		sctk_ib_qp_allocate_rtr(rail_ib, route_dest->remote, &keys);
		sctk_ib_qp_allocate_rts(rail_ib, route_dest->remote);
		mpc_launch_pmi_barrier();

		sctk_rail_add_static_route(rail, endpoint_dest);
		sctk_rail_add_static_route(rail, endpoint_src);

		/* Change to connected */
		__update_endpoint_state(rail, endpoint_dest);
		__update_endpoint_state(rail, endpoint_src);
	}
	else
	{
		mpc_common_nodebug("Send msg to rail %d", rail->rail_number);
		/* create remote for dest */
		endpoint_dest = sctk_ib_create_remote();
		sctk_ib_init_remote(dest_rank, rail, endpoint_dest, 0);
		route_dest = &endpoint_dest->data.ib;

		sctk_ib_qp_keys_send(rail_ib, route_dest->remote);
		mpc_launch_pmi_barrier();


		/* change state to RTR */
		keys = sctk_ib_qp_keys_recv(rail_ib, src_rank);
		sctk_ib_qp_allocate_rtr(rail_ib, route_dest->remote, &keys);
		sctk_ib_qp_allocate_rts(rail_ib, route_dest->remote);
		mpc_launch_pmi_barrier();

		sctk_rail_add_static_route(rail, endpoint_dest);

		/* Change to connected */
		__update_endpoint_state(rail, endpoint_dest);
	}

	mpc_common_nodebug("Recv from %d, send to %d", src_rank, dest_rank);
}

/*-----------------------------------------------------------
*  STATIC CONNEXIONS : process to process connexions during intialization
*  time
*  Messages are sent using raw data (not like ring where messages are converted into
*  string).
*----------------------------------------------------------*/
void sctk_ib_cm_connect_to(int from, int to, sctk_rail_info_t *rail)
{
	/* We assume that the node we want to connect to is not the current  */
	assume(from != to);
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_DEVICE(rail_ib);
	_mpc_lowcomm_endpoint_t *  endpoint;
	struct sctk_ib_qp_s *      remote;
	sctk_ib_cm_qp_connection_t send_keys;
	sctk_ib_cm_qp_connection_t recv_keys;
	sctk_ib_cm_done_t          done;
	mpc_common_nodebug("Connection TO from %d to %d", from, to);

	/* create remote for dest */
	endpoint = sctk_ib_create_remote();
	sctk_ib_init_remote(from, rail, endpoint, 0);
	remote = endpoint->data.ib.remote;
	sctk_ib_debug("[%d] QP connection request to process %d", rail->rail_number, remote->rank);

	mpc_lowcomm_request_t req;
	mpc_lowcomm_irecv_class_dest(from,
	                             to,
	                             &recv_keys,
	                             sizeof(sctk_ib_cm_qp_connection_t),
	                             CM_OD_STATIC_TAG,
	                             MPC_COMM_WORLD,
	                             MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL,
	                             &req);
	mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);

	sctk_ib_qp_allocate_rtr(rail_ib, remote, &recv_keys);

	sctk_ib_qp_key_fill(&send_keys, device->port_attr.lid,
	                    remote->qp->qp_num, remote->psn);
	send_keys.rail_id = rail->rail_number;

	mpc_lowcomm_isend_class_src(to,
	                            from,
	                            &send_keys,
	                            sizeof(sctk_ib_cm_qp_connection_t),
	                            CM_OD_STATIC_TAG,
	                            MPC_COMM_WORLD,
	                            MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL,
	                            &req);
	mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);

	mpc_lowcomm_irecv_class_dest(from,
	                             to,
	                             &done,
	                             sizeof(sctk_ib_cm_done_t),
	                             CM_OD_STATIC_TAG,
	                             MPC_COMM_WORLD,
	                             MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL,
	                             &req);
	mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);

	sctk_ib_qp_allocate_rts(rail_ib, remote);
	/* Add route */
	sctk_rail_add_static_route(rail, endpoint);
	/* Change to connected */
	__update_endpoint_state(rail, endpoint);
}

void sctk_ib_cm_connect_from(int from, int to, sctk_rail_info_t *rail)
{
	/* We assume that the node we want to connect to is not the current  */
	assume(from != to);
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	LOAD_DEVICE(rail_ib);
	_mpc_lowcomm_endpoint_t *  endpoint;
	struct sctk_ib_qp_s *      remote;
	sctk_ib_cm_qp_connection_t send_keys;
	sctk_ib_cm_qp_connection_t recv_keys;

	/* Message to exchange to the peer */
	sctk_ib_cm_done_t done =
	{
		.rail_id = rail->rail_number,
		.done    = 1,
	};
	mpc_common_nodebug("Connection FROM from %d to %d", from, to);

	/* create remote for dest */
	endpoint = sctk_ib_create_remote();
	sctk_ib_init_remote(to, rail, endpoint, 0);
	remote = endpoint->data.ib.remote;
	sctk_ib_debug("[%d] QP connection  request to process %d", rail->rail_number, remote->rank);

	assume(remote->qp);
	sctk_ib_qp_key_fill(&send_keys, device->port_attr.lid,
	                    remote->qp->qp_num, remote->psn);
	send_keys.rail_id = rail->rail_number;

	mpc_lowcomm_request_t req;
	mpc_lowcomm_isend_class_src(from,
	                            to,
	                            &send_keys,
	                            sizeof(sctk_ib_cm_qp_connection_t),
	                            CM_OD_STATIC_TAG,
	                            MPC_COMM_WORLD,
	                            MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL,
	                            &req);
	mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);

	mpc_lowcomm_irecv_class_dest(to,
	                             from,
	                             &recv_keys,
	                             sizeof(sctk_ib_cm_qp_connection_t),
	                             CM_OD_STATIC_TAG,
	                             MPC_COMM_WORLD,
	                             MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL,
	                             &req);
	mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);

	sctk_ib_qp_allocate_rtr(rail_ib, remote, &recv_keys);
	sctk_ib_qp_allocate_rts(rail_ib, remote);
	mpc_common_nodebug("FROM: Ready to send to %d", to);

	mpc_lowcomm_isend_class_src(from,
	                            to,
	                            &done,
	                            sizeof(sctk_ib_cm_done_t),
	                            CM_OD_STATIC_TAG,
	                            MPC_COMM_WORLD,
	                            MPC_LOWCOMM_CONTROL_MESSAGE_INTERNAL,
	                            &req);
	mpc_lowcomm_wait(&req, MPC_LOWCOMM_STATUS_NULL);

	/* Add route */
	sctk_rail_add_static_route(rail, endpoint);
	/* Change to connected */
	__update_endpoint_state(rail, endpoint);
}

/*-----------------------------------------------------------
*  ON DEMAND CONNEXIONS: process to process connexions during run time.
*  Messages are sent using raw data (not like ring where messages are converted into
*  string).
*----------------------------------------------------------*/

#if 0

static inline void sctk_ib_cm_on_demand_recv_done(sctk_rail_info_t *rail, int src)
{
	sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;

	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_dynamic_route_to_process(rail, src);

	ib_assume(endpoint);
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(remote);

	ib_assume(sctk_ib_qp_allocate_get_rts(remote) == 0);
	sctk_ib_qp_allocate_rts(rail_ib_targ, remote);

	/* Change to connected */
	__update_endpoint_state(rail, endpoint);
}

static inline void sctk_ib_cm_on_demand_recv_ack(sctk_rail_info_t *rail, void *ack, int src)
{
	sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;

	sctk_ib_cm_qp_connection_t recv_keys;
	sctk_ib_cm_done_t          done =
	{
		.rail_id = *( ( int * )ack),
		.done    = 1,
	};

	sctk_ib_nodebug("OD QP connexion ACK received from process %d %s", src, ack);
	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_dynamic_route_to_process(rail, src);
	ib_assume(endpoint);
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(remote);
	memcpy(&recv_keys, ack, sizeof(sctk_ib_cm_qp_connection_t) );

	ib_assume(sctk_ib_qp_allocate_get_rtr(remote) == 0);
	sctk_ib_qp_allocate_rtr(rail_ib_targ, remote, &recv_keys);

	ib_assume(sctk_ib_qp_allocate_get_rts(remote) == 0);
	sctk_ib_qp_allocate_rts(rail_ib_targ, remote);

	/* SEND MESSAGE */
	sctk_control_messages_send_rail(src, CM_OD_DONE_TAG, 0, &done, sizeof(sctk_ib_cm_done_t), rail->rail_number);

	/* Change to connected */
	__update_endpoint_state(rail, endpoint);
}

int sctk_ib_cm_on_demand_recv_request(sctk_rail_info_t *rail, void *request, int src)
{
	sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;

	LOAD_DEVICE(rail_ib_targ);
	sctk_ib_cm_qp_connection_t send_keys;
	sctk_ib_cm_qp_connection_t recv_keys;
	int added;

	/* create remote for source */
	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_add_or_reuse_route_dynamic(rail, src, sctk_ib_create_remote, sctk_ib_init_remote, &added, 0);
	ib_assume(endpoint);

	_MPC_LOWCOMM_ENDPOINT_LOCK(endpoint);

	if(_mpc_lowcomm_endpoint_get_state(endpoint) == _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
	{
		_mpc_lowcomm_endpoint_set_state(endpoint, _MPC_LOWCOMM_ENDPOINT_CONNECTING);
	}

	_MPC_LOWCOMM_ENDPOINT_UNLOCK(endpoint);

	sctk_ib_debug("[%d] OD QP connexion request from %d (initiator:%d) START",
	              rail->rail_number, src, _mpc_lowcomm_endpoint_is_initiator(endpoint) );

	/* RACE CONDITION AVOIDING -> positive ACK */
	if(_mpc_lowcomm_endpoint_is_initiator(endpoint) == 0 || mpc_common_get_process_rank() > src)
	{
		struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
		sctk_ib_debug("[%d] OD QP connexion request to process %d (initiator:%d)",
		              rail->rail_number, remote->rank, _mpc_lowcomm_endpoint_is_initiator(endpoint) );
		memcpy(&recv_keys, request, sizeof(sctk_ib_cm_qp_connection_t) );

		ib_assume(sctk_ib_qp_allocate_get_rtr(remote) == 0);
		sctk_ib_qp_allocate_rtr(rail_ib_targ, remote, &recv_keys);

		sctk_ib_qp_key_fill(&send_keys, device->port_attr.lid,
		                    remote->qp->qp_num, remote->psn);
		send_keys.rail_id = rail->rail_number;

		/* Send ACK */
		sctk_ib_debug("OD QP ack to process %d: (tag:%d)", src, CM_OD_ACK_TAG);

		sctk_control_messages_send_rail(src, CM_OD_ACK_TAG, 0, &send_keys, sizeof(sctk_ib_cm_qp_connection_t), rail->rail_number);
	}

	return 0;
}

/*
 * Send a connexion request to the process 'dest'
 */
_mpc_lowcomm_endpoint_t *sctk_ib_cm_on_demand_request(int dest, sctk_rail_info_t *rail)
{
	sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;

	LOAD_DEVICE(rail_ib_targ);
	_mpc_lowcomm_endpoint_t *  endpoint;
	sctk_ib_cm_qp_connection_t send_keys;
	/* Message to exchange to the peer */
	int added;
	/* If we need to send the request */
	int send_request = 0;

	/* create remote for dest if it does not exist */
	endpoint = sctk_rail_add_or_reuse_route_dynamic(rail, dest, sctk_ib_create_remote, sctk_ib_init_remote, &added, 1);
	ib_assume(endpoint);

	_MPC_LOWCOMM_ENDPOINT_LOCK(endpoint);

	if(_mpc_lowcomm_endpoint_get_state(endpoint) == _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
	{
		_mpc_lowcomm_endpoint_set_state(endpoint, _MPC_LOWCOMM_ENDPOINT_CONNECTING);
		send_request = 1;
	}

	_MPC_LOWCOMM_ENDPOINT_UNLOCK(endpoint);

	/* If we are the first to access the route and if the state
	 * is deconnected, so we can proceed to a connection*/
	if(send_request)
	{
		struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;

		sctk_ib_qp_key_fill(&send_keys, device->port_attr.lid, remote->qp->qp_num, remote->psn);
		send_keys.rail_id = rail->rail_number;

		sctk_ib_debug("[%d] OD QP connexion requested to %d", rail->rail_number, remote->rank);
		sctk_control_messages_send_rail(dest, CM_OD_REQ_TAG, 0, &send_keys, sizeof(sctk_ib_cm_qp_connection_t), rail->rail_number);
	}

	return endpoint;
}

#endif

/******************************
 * ON DEMAND WITH THE MONITOR *
 ******************************/

typedef enum
{
	_IB_MONITOR_OD,
	_IB_MONITOR_RTS
}__ib_monitor_action_t;


char * __monitor_get_rail_name(char * data, int len, sctk_rail_info_t *rail, __ib_monitor_action_t action)
{
	switch(action)
	{
		case _IB_MONITOR_OD:
    		snprintf(data, len, "IB-OD-%d", rail->rail_number);
		break;
		case _IB_MONITOR_RTS:
    		snprintf(data, len, "IB-RTS-%d", rail->rail_number);
		break;
	}
    return data;
}

#define _ALREADY_CONNECTED_MSG "ALREADY HERE BUDDY"

static inline int __ib_monitor_on_demand(mpc_lowcomm_peer_uid_t from,
										 char *data,
										 char * return_data,
										 int return_data_len,
										 void *ctx)
{
    mpc_common_debug("IB MONITOR OD from %lu", from);
    sctk_rail_info_t *rail = (sctk_rail_info_t *)ctx;
	sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
	LOAD_DEVICE(rail_ib_targ);

    /* Convert source keys from string */
    sctk_ib_cm_qp_connection_t recv_keys = sctk_ib_qp_keys_convert ( data );


	/* create remote for source */
	int added;

	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_add_or_reuse_route_dynamic(rail, from, sctk_ib_create_remote, sctk_ib_init_remote, &added, 0);
	ib_assume(endpoint);

    if(!added)
    {
        /* This means we are racing in cross connect
         * we then know that both processes 
         * may encounter this error the collision
         * avoidance rule of the largest rank
         * is then to be used */
        mpc_lowcomm_peer_uid_t local = mpc_lowcomm_monitor_get_uid();

        if(from < local)
        {
            snprintf(return_data, return_data_len, _ALREADY_CONNECTED_MSG);
            return 1;
        }
    
    }

	_MPC_LOWCOMM_ENDPOINT_LOCK(endpoint);

	if(_mpc_lowcomm_endpoint_get_state(endpoint) == _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
	{
		_mpc_lowcomm_endpoint_set_state(endpoint, _MPC_LOWCOMM_ENDPOINT_CONNECTING);
	}

	_MPC_LOWCOMM_ENDPOINT_UNLOCK(endpoint);

    /* Set QP in RTR */
    struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(sctk_ib_qp_allocate_get_rtr(remote) == 0);
    sctk_ib_qp_allocate_rtr(rail_ib_targ, remote, &recv_keys);

    /* Prepare response with own info */
	sctk_ib_cm_qp_connection_t send_keys;
	sctk_ib_qp_key_fill(&send_keys, device->port_attr.lid,
		                remote->qp->qp_num, remote->psn);
	send_keys.rail_id = rail->rail_number;

    /* Serialize */
    sctk_ib_qp_key_create_value (return_data, return_data_len, &send_keys);

    return 0;
}

static inline int __ib_monitor_rts(mpc_lowcomm_peer_uid_t from,
								   char *data,
								   char * return_data,
								   int return_data_len,
								   void *ctx)
{
    mpc_common_nodebug("IB MONITOR RTS from %lu", from);
    sctk_rail_info_t *rail = (sctk_rail_info_t *)ctx;
	sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;

	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_dynamic_route_to_process(rail, from);

	ib_assume(endpoint);
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(remote);

	ib_assume(sctk_ib_qp_allocate_get_rts(remote) == 0);
	sctk_ib_qp_allocate_rts(rail_ib_targ, remote);

	/* Change to connected */
	__update_endpoint_state(rail, endpoint);

    return 0;
}


_mpc_lowcomm_endpoint_t * sctk_ib_cm_on_demand_request_monitor(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest)
{
    int added = 0;

    _mpc_lowcomm_endpoint_t *  endpoint = sctk_rail_add_or_reuse_route_dynamic(rail, dest, sctk_ib_create_remote, sctk_ib_init_remote, &added, 1);
	ib_assume(endpoint);

	int send_request = 0;

	_MPC_LOWCOMM_ENDPOINT_LOCK(endpoint);

	if(_mpc_lowcomm_endpoint_get_state(endpoint) == _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
	{
		_mpc_lowcomm_endpoint_set_state(endpoint, _MPC_LOWCOMM_ENDPOINT_CONNECTING);
		send_request = 1;
	}

	_MPC_LOWCOMM_ENDPOINT_UNLOCK(endpoint);

    mpc_common_nodebug("IB MONITOR OD to %s send %d", mpc_lowcomm_peer_format(dest), send_request);

	/* If we are the first to access the route and if the state
	 * is deconnected, so we can proceed to a connection*/
	if(send_request)
	{
	    sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;
	    LOAD_DEVICE(rail_ib_targ);
		struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;

	    sctk_ib_cm_qp_connection_t send_keys;
		sctk_ib_qp_key_fill(&send_keys, device->port_attr.lid, remote->qp->qp_num, remote->psn);
		send_keys.rail_id = rail->rail_number;

        /* Convert to char to send with Monitor */
        char local_qp_infos[128];
        sctk_ib_qp_key_create_value (local_qp_infos, 128, &send_keys);
        /* Proceed with the MONITOR command */
		char monitor_code[128];

		mpc_lowcomm_monitor_retcode_t mon_retcode = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

		mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_ondemand(dest,
																		   __monitor_get_rail_name(monitor_code, 128, rail, _IB_MONITOR_OD),
																		   local_qp_infos,
																		   &mon_retcode);
	/* Decode incoming addr */
		mpc_lowcomm_monitor_args_t *resp_content = mpc_lowcomm_monitor_response_get_content(resp);

		if( resp_content->on_demand.retcode!= MPC_LOWCOMM_MONITOR_RET_SUCCESS)
		{
            /* HANDLE CROSS CONNECT */
            if(!strcmp(resp_content->on_demand.data, _ALREADY_CONNECTED_MSG))
            {
                while(_mpc_lowcomm_endpoint_get_state(endpoint) != _MPC_LOWCOMM_ENDPOINT_CONNECTED)
                {
                    mpc_thread_yield();
                }
           
		        mpc_lowcomm_monitor_response_free(resp);
                return endpoint;
            }
            else
            {
                mpc_common_debug_fatal("On DEMAND callback failed");
            }
		}

	
		sctk_ib_cm_qp_connection_t recv_keys = sctk_ib_qp_keys_convert ( resp_content->on_demand.data );

		/* Set ourselves in RTR */
		ib_assume(sctk_ib_qp_allocate_get_rtr(remote) == 0);
    	sctk_ib_qp_allocate_rtr(rail_ib_targ, remote, &recv_keys);

		/* Set QP in RTS */
		ib_assume(sctk_ib_qp_allocate_get_rts(remote) == 0);
		sctk_ib_qp_allocate_rts(rail_ib_targ, remote);


        /* Done with the monitor command */
		mpc_lowcomm_monitor_response_free(resp);

		mpc_lowcomm_monitor_response_free(
			mpc_lowcomm_monitor_ondemand(dest,
										__monitor_get_rail_name(monitor_code, 128, rail, _IB_MONITOR_RTS),
										"I'm Ready Set Yourself RTS",
										&mon_retcode)
		);

		if(mon_retcode != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
		{
			mpc_lowcomm_monitor_retcode_print(mon_retcode, "IB RTS");
			mpc_common_debug_fatal("Failed to set remote infiniband peer in RTS");
		}

		__update_endpoint_state(rail, endpoint);
	}

    return endpoint;
}

void sctk_ib_cm_monitor_register_callbacks(sctk_rail_info_t * rail)
{
  char cb_name[128];
  mpc_lowcomm_monitor_register_on_demand_callback(__monitor_get_rail_name(cb_name, 128, rail, _IB_MONITOR_OD),
                                                  __ib_monitor_on_demand,
                                                  (void*)rail);

  mpc_lowcomm_monitor_register_on_demand_callback(__monitor_get_rail_name(cb_name, 128, rail, _IB_MONITOR_RTS),
                                                  __ib_monitor_rts,
                                                  (void*)rail);
}

/*-----------------------------------------------------------
*  ON DEMAND RDMA CONNEXIONS:
*  Messages are sent using raw data (not like ring where messages are converted into
*  string).
*----------------------------------------------------------*/

/* Function which returns if a remote can be connected using RDMA */
int sctk_ib_cm_on_demand_rdma_check_request(__UNUSED__ sctk_rail_info_t *rail, struct sctk_ib_qp_s *remote)
{
	int send_request = 0;

	ib_assume(_mpc_lowcomm_endpoint_get_state(remote->endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED);

	/* If several threads call this function, only 1 will send a request to
	 * the remote process */
	_MPC_LOWCOMM_ENDPOINT_LOCK(remote->endpoint);

	if(_mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote) == _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
	{
		_mpc_lowcomm_ib_ibuf_rdma_set_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTING);
		send_request = 1;
	}

	_MPC_LOWCOMM_ENDPOINT_UNLOCK(remote->endpoint);

	return send_request;
}

/*
 * Send a connexion request to the process 'dest'.
 * Data exchanged:
 * - size of slots
 * - number of slots
 */
int sctk_ib_cm_on_demand_rdma_request(sctk_rail_info_t *rail, struct sctk_ib_qp_s *remote, int entry_size, int entry_nb)
{
    mpc_common_debug_error("HERE SENDING RDMA");
	sctk_ib_rail_info_t *rail_ib_targ = &rail->network.ib;

	LOAD_DEVICE(rail_ib_targ);
	/* If we need to send the request */
	ib_assume(_mpc_lowcomm_endpoint_get_state(remote->endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED);


	/* If we are the first to access the route and if the state
	 * is deconnected, so we can proceed to a connection*/

	if(_mpc_lowcomm_ib_ibuf_rdma_is_connectable(rail_ib_targ) )
	{
		/* Can connect to RDMA */

		sctk_ib_cm_rdma_connection_t send_keys;
		mpc_common_nodebug("Can connect to remote %d", remote->rank);

		send_keys.connected = 1;

		/* We fill the request and we save how many slots are requested as well
		 * as the size of each slot */
		remote->od_request.nb         = send_keys.nb = entry_nb;
		remote->od_request.size_ibufs = send_keys.size = entry_size;

		sctk_ib_debug("[%d] OD QP RDMA connexion requested to %d (size:%d nb:%d rdma_connections:%d rdma_cancel:%d)",
		              rail->rail_number, remote->rank, send_keys.size, send_keys.nb, device->eager_rdma_connections, remote->rdma.cancel_nb);

		send_keys.rail_id = rail->rail_number;
		sctk_control_messages_send_rail(remote->rank, CM_OD_RDMA_REQ_TAG, 0, &send_keys, sizeof(sctk_ib_cm_rdma_connection_t), rail->rail_number);
	}
	else
	{
		/* Cannot connect to RDMA */
		mpc_common_debug("[%d] Cannot connect to remote %d", rail->rail_number, remote->rank);
		/* We reset the state to deconnected */
		/* FIXME: state to reset */
		_mpc_lowcomm_ib_ibuf_rdma_set_remote_state_rts(remote, _MPC_LOWCOMM_ENDPOINT_DECONNECTED);
	}

	return 1;
}

static inline void sctk_ib_cm_on_demand_rdma_done_recv(sctk_rail_info_t *rail, void *done, int src)
{
	sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * )done;

	/* get the route to process */
	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_any_route_to_process(rail, src);

	ib_assume(endpoint);
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(remote);

	sctk_ib_debug("[%d] OD QP connexion DONE REQ received from process %d (%p:%u)", rail->rail_number, src,
	              recv_keys->addr, recv_keys->rkey);

	ib_assume(recv_keys->connected == 1);
	/* Update the RDMA regions */
	_mpc_lowcomm_ib_ibuf_rdma_update_remote_addr(remote, recv_keys, REGION_SEND);
	__set_endpoint_ready_to_receive(rail, endpoint, CONNECTION);
}

static inline void sctk_ib_cm_on_demand_rdma_recv_ack(sctk_rail_info_t *rail, void *ack, int src)
{
	sctk_ib_rail_info_t *         rail_ib_targ = &rail->network.ib;
	sctk_ib_cm_rdma_connection_t  send_keys;
	sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * )ack;

	/* get the route to process */
	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_any_route_to_process(rail, src);

	ib_assume(endpoint);
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(remote);

	sctk_ib_debug("[%d] OD QP connexion ACK received from process %d (%p:%u)", rail->rail_number, src,
	              recv_keys->addr, recv_keys->rkey);

	/* If the remote peer is connectable */
	if(recv_keys->connected == 1)
	{
		/* Allocate the buffer */
		_mpc_lowcomm_ib_ibuf_rdma_pool_init(remote);
		/* We create the SEND region */
		_mpc_lowcomm_ib_ibuf_rdma_region_init(rail_ib_targ, remote,
		                                      &remote->rdma.pool->region[REGION_SEND],
		                                      MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL, remote->od_request.nb, remote->od_request.size_ibufs);

		/* Update the RDMA regions */
		_mpc_lowcomm_ib_ibuf_rdma_update_remote_addr(remote, recv_keys, REGION_RECV);
		/* Fill the keys */
		_mpc_lowcomm_ib_ibuf_rdma_fill_remote_addr(remote, &send_keys, REGION_SEND);
		send_keys.rail_id = *( ( int * )ack);

		/* Send the message */
		send_keys.connected = 1;
		sctk_control_messages_send_rail(src, CM_OD_RDMA_DONE_TAG, 0, &send_keys, sizeof(sctk_ib_cm_rdma_connection_t), rail->rail_number);

		__set_endpoint_ready_to_send(rail, endpoint, CONNECTION);
	}
	else
	{
		/* cannot connect */
		/* We cancel the RDMA connection */
		_mpc_lowcomm_ib_ibuf_rdma_connexion_cancel(rail_ib_targ, remote);
	}
}

/*
 * Receive a request for a source process
 * Data exchanged:
 * - size of slots
 * - number of slots
 * - Address of the send region
 * - Address of the recv region
 */
static inline void sctk_ib_cm_on_demand_rdma_recv_request(sctk_rail_info_t *rail, void *request, int src)
{
	sctk_ib_rail_info_t *        rail_ib_targ = &rail->network.ib;
	sctk_ib_cm_rdma_connection_t send_keys;

	memset(&send_keys, 0, sizeof(sctk_ib_cm_rdma_connection_t) );

	/* get the route to process */
	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_any_route_to_process(rail, src);
	ib_assume(endpoint);
	/* We assume the route is connected */
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(_mpc_lowcomm_endpoint_get_state(endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED);


	_MPC_LOWCOMM_ENDPOINT_LOCK(endpoint);

	if(_mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(remote) == _MPC_LOWCOMM_ENDPOINT_DECONNECTED)
	{
		_mpc_lowcomm_ib_ibuf_rdma_set_remote_state_rtr(remote, _MPC_LOWCOMM_ENDPOINT_CONNECTING);
	}

	_MPC_LOWCOMM_ENDPOINT_UNLOCK(endpoint);

	sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * )request;
	sctk_ib_debug("[%d] OD RDMA connexion REQUEST to process %d (connected:%d size:%d nb:%d )",
	              rail->rail_number, remote->rank, recv_keys->connected, recv_keys->size, recv_keys->nb);

	/* We do not send a request if we do not want to be connected
	 * using RDMA. This is stupid :-) */
	ib_assume(recv_keys->connected == 1);

	/* We check if we can also be connected using RDMA */
	if(_mpc_lowcomm_ib_ibuf_rdma_is_connectable(rail_ib_targ) )
	{
		/* Can connect to RDMA */

		/* We can change to RTR because we are RDMA connectable */
		send_keys.connected = 1;

		/* We firstly allocate the main structure. 'ibuf_rdma_pool_init'
		 * implicitely does not allocate memory if already created */
		_mpc_lowcomm_ib_ibuf_rdma_pool_init(remote);
		/* We create the RECV region */
		_mpc_lowcomm_ib_ibuf_rdma_region_init(rail_ib_targ, remote,
		                                      &remote->rdma.pool->region[REGION_RECV],
		                                      MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_RECV_CHANNEL, recv_keys->nb, recv_keys->size);

		/* Fill the keys */
		_mpc_lowcomm_ib_ibuf_rdma_fill_remote_addr(remote, &send_keys, REGION_RECV);
	}
	else
	{
		/* Cannot connect to RDMA */
		mpc_common_nodebug("Cannot connect to remote %d", remote->rank);

		send_keys.connected = 0;
		send_keys.size      = 0;
		send_keys.nb        = 0;
		send_keys.rail_id   = *( ( int * )request);
	}

	/* Send ACK */
	sctk_ib_debug("[%d] OD QP ack to process %d (%p:%u)", rail->rail_number, src,
	              send_keys.addr, send_keys.rkey);

	sctk_control_messages_send_rail(src, CM_OD_RDMA_ACK_TAG, 0, &send_keys, sizeof(sctk_ib_cm_rdma_connection_t), rail->rail_number);
}

/*-----------------------------------------------------------
*  ON DEMAND RDMA RESIZING:
*  Messages are sent using raw data (not like ring where messages are converted into
*  string).
*----------------------------------------------------------*/

/*
 * Send a connexion request to the process 'dest'.
 * Data exchanged:
 * - size of slots
 * - number of slots. If 0, the remote needs to be deconnected
 *
 */
int sctk_ib_cm_resizing_rdma_request(sctk_rail_info_t *rail, struct sctk_ib_qp_s *remote, int entry_size, int entry_nb)
{
	/* Assume that the route is in a flushed state */
	ib_assume(_mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote) == _MPC_LOWCOMM_ENDPOINT_FLUSHED);
	/* Assume there is no more pending messages */
	ib_assume(OPA_load_int(&remote->rdma.pool->busy_nb[REGION_SEND]) == 0);

	sctk_ib_cm_rdma_connection_t send_keys;

	send_keys.connected = 1;
	send_keys.nb        = entry_nb;
	send_keys.size      = entry_size;
	send_keys.rail_id   = rail->rail_number;

	sctk_ib_nodebug("[%d] Sending RDMA RESIZING request to %d (size:%d nb:%d resizing_nb:%d)",
	                rail->rail_number, remote->rank, send_keys.size, send_keys.nb,
	                remote->rdma.resizing_nb);

	sctk_control_messages_send_rail(remote->rank, CM_RESIZING_RDMA_REQ_TAG, 0, &send_keys, sizeof(sctk_ib_cm_rdma_connection_t), rail->rail_number);

	return 1;
}

void sctk_ib_cm_resizing_rdma_ack(sctk_rail_info_t *rail, struct sctk_ib_qp_s *remote, sctk_ib_cm_rdma_connection_t *send_keys)
{
	ib_assume(_mpc_lowcomm_endpoint_get_state(remote->endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED);
	/* Assume that the route is in a flushed state */
	ib_assume(_mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rtr(remote) == _MPC_LOWCOMM_ENDPOINT_FLUSHED);
	/* Assume there is no more pending messages */
	ib_assume(OPA_load_int(&remote->rdma.pool->busy_nb[REGION_RECV]) == 0);

	sctk_ib_nodebug("[%d] Sending RDMA RESIZING ACK to %d (addr:%p - rkey:%u)",
	                rail->rail_number, remote->rank, send_keys->addr, send_keys->rkey);

	send_keys->rail_id = rail->rail_number;
	sctk_control_messages_send_rail(remote->rank, CM_RESIZING_RDMA_ACK_TAG, 0, send_keys, sizeof(sctk_ib_cm_rdma_connection_t), rail->rail_number);
}

static inline void sctk_ib_cm_resizing_rdma_done_recv(sctk_rail_info_t *rail, void *done, int src)
{
	sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * )done;

	/* get the route to process */
	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_any_route_to_process(rail, src);

	ib_assume(endpoint);
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(remote);

	sctk_ib_nodebug("[%d] RDMA RESIZING done req received from process %d (%p:%u)", rail->rail_number, src,
	                recv_keys->addr, recv_keys->rkey);

	ib_assume(recv_keys->connected == 1);

	/* Update the RDMA regions */
	_mpc_lowcomm_ib_ibuf_rdma_update_remote_addr(remote, recv_keys, REGION_SEND);

	__set_endpoint_ready_to_receive(rail, endpoint, RESIZING);
}

static inline void sctk_ib_cm_resizing_rdma_ack_recv(sctk_rail_info_t *rail, void *ack, int src)
{
	sctk_ib_rail_info_t *         rail_ib_targ = &rail->network.ib;
	sctk_ib_cm_rdma_connection_t *recv_keys    = ( sctk_ib_cm_rdma_connection_t * )ack;

	/* get the route to process */
	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_any_route_to_process_or_forward(rail, src);

	ib_assume(endpoint);
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(remote);

	sctk_ib_nodebug("[%d] RDMA RESIZING ACK received from process %d (addr:%p rkey:%u)", rail->rail_number, src, recv_keys->addr, recv_keys->rkey);

	/* Update the RDMA regions */
	/* FIXME: the rail number should be determinated */
	_mpc_lowcomm_ib_ibuf_rdma_update_remote_addr(remote, recv_keys, REGION_RECV);

	/* If the remote peer is connectable */
	sctk_ib_cm_rdma_connection_t *send_keys =
		&remote->rdma.pool->resizing_request.send_keys;
	/* FIXME: do some cool stuff here */
	/* Resizing the RDMA buffer */
	_mpc_lowcomm_ib_ibuf_rdma_region_reinit(rail_ib_targ, remote,
	                                        &remote->rdma.pool->region[REGION_SEND],
	                                        MPC_LOWCOMM_IB_RDMA_CHANNEL | MPC_LOWCOMM_IB_SEND_CHANNEL,
	                                        send_keys->nb, send_keys->size);

	OPA_incr_int(&remote->rdma.resizing_nb);
	send_keys->connected = 1;
	send_keys->rail_id   = rail_ib_targ->rail->rail_number;
	_mpc_lowcomm_ib_ibuf_rdma_fill_remote_addr(remote, send_keys, REGION_SEND);

	sctk_control_messages_send_rail(src, CM_RESIZING_RDMA_DONE_TAG, 0, send_keys, sizeof(sctk_ib_cm_rdma_connection_t), rail->rail_number);

	__set_endpoint_ready_to_send(rail, endpoint, RESIZING);
}

/*
 * Receive a request for a source process
 * Data exchanged:
 * - size of slots
 * - number of slots
 * - Address of the send region
 * - Address of the recv region
 */
static inline int sctk_ib_cm_resizing_rdma_recv_request(sctk_rail_info_t *rail, void *request, int src)
{
	sctk_ib_rail_info_t *        rail_ib_targ = &rail->network.ib;
	sctk_ib_cm_rdma_connection_t send_keys;

	memset(&send_keys, 0, sizeof(sctk_ib_cm_rdma_connection_t) );

	/* get the route to process */
	_mpc_lowcomm_endpoint_t *endpoint = sctk_rail_get_any_route_to_process_or_forward(rail, src);
	ib_assume(endpoint);
	/* We assume the route is connected */
	ib_assume(_mpc_lowcomm_endpoint_get_state(endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED);
	struct sctk_ib_qp_s *remote = endpoint->data.ib.remote;
	ib_assume(remote);

	sctk_ib_cm_rdma_connection_t *recv_keys = ( sctk_ib_cm_rdma_connection_t * )request;
	ib_assume(recv_keys->connected == 1);
	remote->rdma.pool->resizing_request.recv_keys.nb   = recv_keys->nb;
	remote->rdma.pool->resizing_request.recv_keys.size = recv_keys->size;

	sctk_ib_nodebug("[%d] Receiving RDMA RESIZING request to process %d (connected:%d size:%d nb:%d)",
	                rail->rail_number, remote->rank, recv_keys->connected, recv_keys->size, recv_keys->nb);

	/* Start flushing */
	_mpc_lowcomm_ib_ibuf_rdma_flush_recv(rail_ib_targ, remote);

	return 0;
}

/*-----------------------------------------------------------
*  Handler of OD connexions
*----------------------------------------------------------*/

void sctk_ib_cm_control_message_handler(struct sctk_rail_info_s *rail, int process_src, __UNUSED__ int source_rank, char subtype, __UNUSED__ char param, void *payload, __UNUSED__ size_t size)
{
	switch(subtype)
	{
		/* QP connection */
		case CM_OD_REQ_TAG:
			not_implemented();
			//sctk_ib_cm_on_demand_recv_request(rail, payload, process_src);
			break;

		case CM_OD_ACK_TAG:
			not_implemented();
			//sctk_ib_cm_on_demand_recv_ack(rail, payload, process_src);
			break;

		case CM_OD_DONE_TAG:
			not_implemented();
			//sctk_ib_cm_on_demand_recv_done(rail, process_src);
			break;

		/* RDMA connection */
		case CM_OD_RDMA_REQ_TAG:
			sctk_ib_cm_on_demand_rdma_recv_request(rail, payload, process_src);
			break;

		case CM_OD_RDMA_ACK_TAG:
			sctk_ib_cm_on_demand_rdma_recv_ack(rail, payload, process_src);
			break;

		case CM_OD_RDMA_DONE_TAG:
			sctk_ib_cm_on_demand_rdma_done_recv(rail, payload, process_src);
			break;

		/* RDMA resizing */
		case CM_RESIZING_RDMA_REQ_TAG:
			sctk_ib_cm_resizing_rdma_recv_request(rail, payload, process_src);
			break;

		case CM_RESIZING_RDMA_ACK_TAG:
			sctk_ib_cm_resizing_rdma_ack_recv(rail, payload, process_src);
			break;

		case CM_RESIZING_RDMA_DONE_TAG:
			sctk_ib_cm_resizing_rdma_done_recv(rail, payload, process_src);
			break;

		default:
			mpc_common_debug_error("Message Type Not Handled");
			not_reachable();
			break;
	}
}
