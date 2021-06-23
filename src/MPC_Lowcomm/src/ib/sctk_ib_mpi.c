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

#include <mpc_common_debug.h>
#include <sctk_ib_toolkit.h>
#include "endpoint.h"
#include <sctk_net_tools.h>
#include "sctk_ib.h"
#include <sctk_ibufs.h>
#include <sctk_ibufs_rdma.h>
#include <sctk_ib_mmu.h>
#include <sctk_ib_config.h>
#include <mpc_topology.h>
#include "sctk_ib_qp.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_async.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_buffered.h"
#include "sctk_ib_topology.h"
#include "sctk_ib_cp.h"
#include "mpc_common_asm.h"
#include "sctk_rail.h"

#include <sctk_alloc.h>


/** array of VPS, for remembering __thread vars to reset when rail is re-enabled */
volatile char *vps_reset = NULL;

#ifdef MPC_ENABLE_DEBUG_MESSAGES
static char *sctk_ib_protocol_print(_mpc_lowcomm_ib_protocol_t prot)
{
	switch(prot)
	{
		case MPC_LOWCOMM_IB_EAGER_PROTOCOL:
			return "MPC_LOWCOMM_IB_EAGER_PROTOCOL";

			break;

		case MPC_LOWCOMM_IB_BUFFERED_PROTOCOL:
			return "MPC_LOWCOMM_IB_BUFFERED_PROTOCOL";

			break;

		case MPC_LOWCOMM_IB_RDMA_PROTOCOL:
			return "MPC_LOWCOMM_IB_RDMA_PROTOCOL";

			break;

		case MPC_LOWCOMM_IB_NULL_PROTOCOL:
			return "MPC_LOWCOMM_IB_NULL_PROTOCOL";

			break;

		default:
			return "null";

			break;
	}
}

#endif



static void _mpc_lowcomm_ib_send_message(mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint)
{
	sctk_rail_info_t *   rail    = endpoint->rail;
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	LOAD_CONFIG(rail_ib);
	_mpc_lowcomm_endpoint_info_ib_t *  route_data;
	sctk_ib_qp_t *          remote;
	_mpc_lowcomm_ib_ibuf_t *ibuf;
	size_t size;


	assume(_mpc_lowcomm_endpoint_get_state(endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED);

	route_data = &endpoint->data.ib;
	remote     = route_data->remote;

	/* Check if the remote task is in low mem mode */
	size = SCTK_MSG_SIZE(msg) + sizeof(mpc_lowcomm_ptp_message_body_t);


	/* *
	 *
	 *  We switch between available protocols
	 *
	 * */
	if( ( (_mpc_lowcomm_ib_ibuf_rdma_get_remote_state_rts(remote) == _MPC_LOWCOMM_ENDPOINT_CONNECTED) &&
	      (size + IBUF_GET_EAGER_SIZE + IBUF_RDMA_GET_SIZE <= _mpc_lowcomm_ib_ibuf_rdma_eager_limit_get(remote) ) ) ||
	    (size + IBUF_GET_EAGER_SIZE <= config->eager_limit) )
	{
		mpc_common_debug("Eager");
		ibuf = _mpc_lowcomm_ib_eager_prepare_msg(rail_ib, remote, msg, size, _mpc_comm_ptp_message_is_for_process(SCTK_MSG_SPECIFIC_CLASS(msg) ) );

		/* Actually, it is possible to get a NULL pointer for ibuf. We falback to buffered */
		if(ibuf == NULL)
		{
			goto buffered;
		}

		/* Send message */
		sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf);
		mpc_lowcomm_ptp_message_complete_and_free(msg);

		/* Remote profiling */
		_mpc_lowcomm_ib_ibuf_rdma_remote_update(remote, size);
		goto exit;
	}

buffered:

	/***** BUFFERED EAGER CHANNEL *****/
	if(size <= config->buffered_limit)
	{
		mpc_common_debug("Buffered");
		_mpc_lowcomm_ib_buffered_prepare_msg(rail, remote, msg, size);
		mpc_lowcomm_ptp_message_complete_and_free(msg);

		/* Remote profiling */
		_mpc_lowcomm_ib_ibuf_rdma_remote_update(remote, size);

		goto exit;
	}

	/***** RDMA RENDEZVOUS CHANNEL *****/
	mpc_common_debug("Size of message: %lu", size);
	ibuf = _mpc_lowcomm_ib_rdma_rendezvous_prepare_req(rail, remote, msg, size);

	/* Send message */
	sctk_ib_qp_send_ibuf(rail_ib, remote, ibuf);
	_mpc_lowcomm_ib_rdma_rendezvous_prepare_send_msg(msg, size);
exit:
	{}

	// PROF_TIME_END ( rail, ib_send_message );
}

_mpc_lowcomm_endpoint_t *sctk_on_demand_connection_ib(struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest)
{
	_mpc_lowcomm_endpoint_t *tmp = NULL;

	/* Wait until we reach the 'deconnected' state */
	tmp = sctk_rail_get_dynamic_route_to_process(rail, dest);

	if(tmp)
	{
		_mpc_lowcomm_endpoint_state_t state;

		do
		{
			state = _mpc_lowcomm_endpoint_get_state(tmp);

			if(state != _MPC_LOWCOMM_ENDPOINT_DECONNECTED && state != _MPC_LOWCOMM_ENDPOINT_CONNECTED &&
			   state != _MPC_LOWCOMM_ENDPOINT_RECONNECTING)
			{
				_mpc_lowcomm_multirail_notify_idle();
				mpc_thread_yield();
			}
		} while(state != _MPC_LOWCOMM_ENDPOINT_DECONNECTED && state != _MPC_LOWCOMM_ENDPOINT_CONNECTED && state != _MPC_LOWCOMM_ENDPOINT_RECONNECTING);
	}
	/* We send the request using the signalization rail */
	tmp = sctk_ib_cm_on_demand_request_monitor(rail, dest);
	assume(tmp);

	/* If route not connected, so we wait for until it is connected */
	while(_mpc_lowcomm_endpoint_get_state(tmp) != _MPC_LOWCOMM_ENDPOINT_CONNECTED)
	{
		_mpc_lowcomm_multirail_notify_idle();

		if(_mpc_lowcomm_endpoint_get_state(tmp) != _MPC_LOWCOMM_ENDPOINT_CONNECTED)
		{
			mpc_thread_yield();
		}
	}

	return tmp;
}

int sctk_network_poll_send_ibuf(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	int release_ibuf             = 1;

	/* Switch on the protocol of the received message */
	switch(IBUF_GET_PROTOCOL(ibuf->buffer) )
	{
		case MPC_LOWCOMM_IB_EAGER_PROTOCOL:
			release_ibuf = 1;
			break;

		case MPC_LOWCOMM_IB_RDMA_PROTOCOL:
			release_ibuf = _mpc_lowcomm_ib_rdma_poll_send(rail, ibuf);
			mpc_common_nodebug("Received RMDA write send");
			break;

		case MPC_LOWCOMM_IB_BUFFERED_PROTOCOL:
			release_ibuf = 1;
			break;

		default:
			mpc_common_debug_error("Got wrong protocol: %d %p", IBUF_GET_PROTOCOL(ibuf->buffer), &IBUF_GET_PROTOCOL(ibuf->buffer) );
			MPC_CRASH();
			break;
	}

	/* We do not need to release the buffer with the RDMA channel */
	if(IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_RDMA_CHANNEL)
	{
		_mpc_lowcomm_ib_ibuf_release(rail_ib, ibuf, 1);
		return 0;
	}

	ib_assume(IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_RC_SR_CHANNEL);

	if(release_ibuf)
	{
		/* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
		_mpc_lowcomm_ib_ibuf_release(rail_ib, ibuf, 1);
	}
	else
	{
		_mpc_lowcomm_ib_ibuf_srq_release(rail_ib, ibuf);
	}

	return 0;
}

int sctk_network_poll_recv_ibuf(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	const _mpc_lowcomm_ib_protocol_t protocol = IBUF_GET_PROTOCOL(ibuf->buffer);
	int release_ibuf       = 1;
	const struct ibv_wc wc = ibuf->wc;

	mpc_common_nodebug("[%d] Recv ibuf:%p", rail->rail_number, ibuf);
	mpc_common_nodebug("Protocol received: %s", sctk_ib_protocol_print(protocol) );

	/* First we check if the message has an immediate data */
	if(wc.wc_flags == IBV_WC_WITH_IMM)
	{
		sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
		sctk_ib_qp_t *       remote  = sctk_ib_qp_ht_find(rail_ib, wc.qp_num);

		if(wc.imm_data & IMM_DATA_RDMA_PIGGYBACK)
		{
			int piggyback = wc.imm_data - IMM_DATA_RDMA_PIGGYBACK;

			OPA_add_int(&remote->rdma.pool->busy_nb[REGION_SEND], -piggyback);
		}
		else
		{
			if(wc.imm_data & IMM_DATA_RDMA_MSG)
			{
				_mpc_lowcomm_ib_rdma_recv_done_remote_imm(rail,
				                                          wc.imm_data - IMM_DATA_RDMA_MSG);
			}
			else
			{
				not_reachable();
			}
		}

		/* Check if we are in a flush state */
		_mpc_lowcomm_ib_ibuf_rdma_flush_send(rail_ib, remote);
		/* We release the buffer */
		release_ibuf = 1;
	}
	else
	{
		/* Switch on the protocol of the received message */
		switch(protocol)
		{
			case MPC_LOWCOMM_IB_EAGER_PROTOCOL:
				_mpc_lowcomm_ib_eager_poll_recv(rail, ibuf);
				release_ibuf = 0;
				break;

			case MPC_LOWCOMM_IB_RDMA_PROTOCOL:
				release_ibuf = _mpc_lowcomm_ib_rdma_poll_recv(rail, ibuf);
				break;

			case MPC_LOWCOMM_IB_BUFFERED_PROTOCOL:
				_mpc_lowcomm_ib_buffered_poll_recv(rail, ibuf);
				release_ibuf = 1;
				break;

			default:
				not_reachable();
		}
	}

	if(release_ibuf)
	{
		/* sctk_ib_qp_release_entry(&rail->network.ib, ibuf->remote); */
		_mpc_lowcomm_ib_ibuf_release(&rail->network.ib, ibuf, 1);
	}
	else
	{
		_mpc_lowcomm_ib_ibuf_srq_release(&rail->network.ib, ibuf);
	}

	return 0;
}

static int sctk_network_poll_recv(sctk_rail_info_t *rail, struct ibv_wc *wc)
{
	_mpc_lowcomm_ib_ibuf_t *ibuf = NULL;

	ibuf = ( _mpc_lowcomm_ib_ibuf_t * )wc->wr_id;
	ib_assume(ibuf);
	int dest_task = -1;


	/* Put a timestamp in the buffer. We *MUST* do it
	 * before pushing the message to the list */
	ibuf->polled_timestamp = mpc_arch_get_timestamp();
	/* We *MUST* recopy some informations to the ibuf */
	ibuf->wc   = *wc;
	ibuf->cq   = MPC_LOWCOMM_IB_RECV_CQ;
	ibuf->rail = rail;

	/* If this is an immediate message, we process it */
	if(wc->wc_flags == IBV_WC_WITH_IMM)
	{
		return sctk_network_poll_recv_ibuf(rail, ibuf);
	}

	if(IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_RC_SR_CHANNEL)
	{
		dest_task = IBUF_GET_DEST_TASK(ibuf->buffer);
		ibuf->cq  = MPC_LOWCOMM_IB_RECV_CQ;

		if(_mpc_lowcomm_ib_cp_ctx_handle_message(ibuf, dest_task, dest_task) == 0)
		{
			return sctk_network_poll_recv_ibuf(rail, ibuf);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return sctk_network_poll_recv_ibuf(rail, ibuf);
	}
}

static int sctk_network_poll_send(sctk_rail_info_t *rail, struct ibv_wc *wc)
{
	sctk_ib_rail_info_t *   rail_ib = &rail->network.ib;
	_mpc_lowcomm_ib_ibuf_t *ibuf    = NULL;

	ibuf = ( _mpc_lowcomm_ib_ibuf_t * )
	       wc->wr_id;

	ib_assume(ibuf);

	int src_task  = -1;
	int dest_task = -1;

	/* First we check if the message has an immediate data.
	 * If it is, we decrease the number of pending requests and we check
	 * if the remote is in a 'flushing' state*/

	/* CAREFUL: we cannot access the imm_data from the wc here
	 * as it is only set for the receiver, not the sender */
	if(wc->wc_flags & IBV_WC_WITH_IMM)
	{
		if(ibuf->send_imm_data & IMM_DATA_RDMA_PIGGYBACK)
		{
			int current_pending;
			current_pending = sctk_ib_qp_fetch_and_sub_pending_data(ibuf->remote, ibuf);
			_mpc_lowcomm_ib_ibuf_rdma_max_pending_data_update(ibuf->remote,
			                                                  current_pending);

			/* Check if we are in a flush state */
			OPA_decr_int(&ibuf->remote->rdma.pool->busy_nb[REGION_RECV]);
			_mpc_lowcomm_ib_ibuf_rdma_check_flush_recv(rail_ib, ibuf->remote);

			return 0;
		}
		else
		if(ibuf->send_imm_data & IMM_DATA_RDMA_MSG)
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
	ibuf->polled_timestamp = mpc_arch_get_timestamp();

	/* We *MUST* recopy some informations to the ibuf */
	ibuf->wc   = *wc;
	ibuf->cq   = MPC_LOWCOMM_IB_SEND_CQ;
	ibuf->rail = rail;

	/* Decrease the number of pending requests */
	int current_pending;
	current_pending = sctk_ib_qp_fetch_and_sub_pending_data(ibuf->remote, ibuf);
	_mpc_lowcomm_ib_ibuf_rdma_max_pending_data_update(ibuf->remote,
	                                                  current_pending);

	if( (IBUF_GET_CHANNEL(ibuf) & MPC_LOWCOMM_IB_RC_SR_CHANNEL) )
	{
		src_task  = IBUF_GET_SRC_TASK(ibuf->buffer);
		dest_task = IBUF_GET_DEST_TASK(ibuf->buffer);

		/* We still check the dest_task. If it is -1, this is a process_specific
		 * message, so we need to handle the message asap */
		if(_mpc_lowcomm_ib_cp_ctx_handle_message(ibuf, dest_task, src_task) == 0)
		{
			return sctk_network_poll_send_ibuf(rail, ibuf);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return sctk_network_poll_send_ibuf(rail, ibuf);
	}
}

static inline void __check_wc(struct sctk_ib_rail_info_s *rail_ib, struct ibv_wc wc)
{
	struct _mpc_lowcomm_config_struct_net_driver_infiniband *config = (rail_ib)->config;
	struct _mpc_lowcomm_ib_ibuf_s *ibuf;
	char host[HOSTNAME];
	char ibuf_desc[4096];

	if(wc.status != IBV_WC_SUCCESS)
	{
		ibuf = ( struct _mpc_lowcomm_ib_ibuf_s * )wc.wr_id;
		assume(ibuf);
		gethostname(host, HOSTNAME);

		if(config->quiet_crash)
		{
			mpc_common_debug_error("\nIB - PROCESS %d CRASHED (%s): %s",
			                       mpc_common_get_process_rank(), host, ibv_wc_status_str(wc.status) );
		}
		else
		{
			_mpc_lowcomm_ib_ibuf_print(ibuf, ibuf_desc);
			mpc_common_debug_error("\nIB - FATAL ERROR FROM PROCESS %d (%s)\n"
			                       "################################\n"
			                       "Work ID is   : %d\n"
			                       "Status       : %s\n"
			                       "ERROR Vendor : %d\n"
			                       "Byte_len     : %d\n"
			                       "Dest process : %d\n"
			                       "######### IBUF DESC ############\n"
			                       "%s\n"
			                       "################################", mpc_common_get_process_rank(),
			                       host,
			                       wc.wr_id,
			                       ibv_wc_status_str(wc.status),
			                       wc.vendor_err,
			                       wc.byte_len,
			                       /* Remote can be NULL if the buffer comes from SRQ */
			                       (ibuf->remote) ? ibuf->remote->rank : 0,
			                       ibuf_desc);
		}

		mpc_common_debug_abort();
	}
}

#define WC_COUNT    32
static inline void __poll_cq(sctk_rail_info_t *rail,
                             struct ibv_cq *cq,
                             struct sctk_ib_polling_s *poll,
                             int (*ptr_func)(sctk_rail_info_t *rail, struct ibv_wc *) )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	struct ibv_wc        wc[WC_COUNT];
	int res = 0;
	int i;

	do
	{
		res = ibv_poll_cq(cq, WC_COUNT, wc);

		if(res)
		{
			mpc_common_nodebug("Polled %d msgs from cq", res);
		}

		for(i = 0; i < res; ++i)
		{
			__check_wc(rail_ib, wc[i]);
			ptr_func(rail, &wc[i]);
			POLL_RECV_CQ(poll);
		}
	} while(res == WC_COUNT);
}

/* Count how many times the vp is entered to the polling function. We
 * allow recursive calls to the polling function */

mpc_common_spinlock_t __cq_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

#define POLL_CQ_CONCURENCY 4

static inline void __poll_send(sctk_rail_info_t *rail, sctk_ib_polling_t *poll)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	LOAD_DEVICE(rail_ib);
	LOAD_CONFIG(rail_ib);

    OPA_int_t max_cq_poll = { 0 };

    int polling = OPA_fetch_and_incr_int(&max_cq_poll);

    if(polling < POLL_CQ_CONCURENCY)
    {
	    /* Poll sent messages */
        __poll_cq(rail, device->send_cq, poll, sctk_network_poll_send);
    }

    OPA_decr_int(&max_cq_poll);
}

static inline void __poll_recv(sctk_rail_info_t *rail, sctk_ib_polling_t *poll)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	LOAD_DEVICE(rail_ib);
	LOAD_CONFIG(rail_ib);


    OPA_int_t max_cq_poll = { 0 };

    int polling = OPA_fetch_and_incr_int(&max_cq_poll);

    if(polling < POLL_CQ_CONCURENCY)
    {
	    /* Poll received messages */
	    __poll_cq(rail, device->recv_cq, poll, sctk_network_poll_recv);
    }

    OPA_decr_int(&max_cq_poll);



}

static inline void __poll_all_cq(sctk_rail_info_t *rail, sctk_ib_polling_t *poll)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	__poll_send(rail, poll);
	__poll_recv(rail, poll);
    _mpc_lowcomm_ib_cp_ctx_poll_global_list(poll);
 }

static void _mpc_lowcomm_ib_receive_message(__UNUSED__ mpc_lowcomm_ptp_message_t *msg, __UNUSED__ sctk_rail_info_t *rail)
{
    struct sctk_ib_polling_s poll;

	POLL_INIT(&poll);

	__poll_recv(rail, &poll);

	if(POLL_GET_RECV_CQ(&poll) != 0)
	{
		POLL_INIT(&poll);
		_mpc_lowcomm_ib_cp_ctx_poll(&poll, SCTK_MSG_DEST_TASK(msg) );
	}
}

/* WARNING: This function can be called with dest == mpc_common_get_process_rank() */
static void _mpc_lowcomm_ib_notify_perform(__UNUSED__ mpc_lowcomm_peer_uid_t remote_process, __UNUSED__ int remote_task_id, __UNUSED__ int polling_task_id, __UNUSED__ int blocking, sctk_rail_info_t *rail)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	struct sctk_ib_polling_s poll;

	int ret;

	_mpc_lowcomm_ib_ibuf_rdma_eager_walk_remote(rail_ib, _mpc_lowcomm_ib_ibuf_rdma_eager_poll_remote, &ret);

	if(ret == REORDER_FOUND_EXPECTED)
	{
		return;
	}

	POLL_INIT(&poll);

	_mpc_lowcomm_ib_cp_ctx_poll(&poll, polling_task_id);


	/* If nothing to poll, we poll the CQ */
	if(POLL_GET_RECV(&poll) == 0)
	{
		POLL_INIT(&poll);
		__poll_all_cq(rail, &poll);

		/* If the polling returned something or someone already inside the function,
		 * we retry to poll the pending lists */
		if(POLL_GET_RECV_CQ(&poll) != 0)
		{
			POLL_INIT(&poll);
			_mpc_lowcomm_ib_cp_ctx_poll(&poll, polling_task_id);
		}
	}



}

__thread int idle_poll_all  = 0;
__thread int idle_poll_freq = 100;
static void _mpc_lowcomm_ib_notify_idle(sctk_rail_info_t *rail)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;


	struct sctk_ib_polling_s poll;

	if(rail->state != SCTK_RAIL_ST_ENABLED)
	{
		return;
	}


	idle_poll_all++;

	if(idle_poll_all != idle_poll_freq)
	{
		return;
	}
	else
	{
		idle_poll_all = 0;
	}

	if(idle_poll_freq < idle_poll_all)
	{
		idle_poll_all = 0;
	}


	int ret;
	_mpc_lowcomm_ib_ibuf_rdma_eager_walk_remote(rail_ib, _mpc_lowcomm_ib_ibuf_rdma_eager_poll_remote, &ret);

	if(ret == REORDER_FOUND_EXPECTED)
	{
		idle_poll_freq = 100;
		return;
	}
	else
	{
		idle_poll_freq *= 4;
	}

	POLL_INIT(&poll);
	__poll_all_cq(rail, &poll);

	/* If the polling returned something or someone already inside the function,
	 * we retry to poll the pending lists */
	if(POLL_GET_RECV_CQ(&poll) != 0)
	{
		idle_poll_freq = 100;
		int polling_task_id = mpc_common_get_task_rank();
		if(polling_task_id >= 0)
		{
			POLL_INIT(&poll);
			_mpc_lowcomm_ib_cp_ctx_poll(&poll, polling_task_id);
		}
	}
	else
	{
		idle_poll_freq *= 4;
	}

	if(150000 < idle_poll_freq)
	{
		idle_poll_freq = 150000;
	}
}

static void _mpc_lowcomm_ib_notify_anysource(int polling_task_id, __UNUSED__ int blocking, sctk_rail_info_t *rail)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	struct sctk_ib_polling_s poll;

	{
		int ret;
		_mpc_lowcomm_ib_ibuf_rdma_eager_walk_remote(rail_ib, _mpc_lowcomm_ib_ibuf_rdma_eager_poll_remote, &ret);

		if(ret == REORDER_FOUND_EXPECTED)
		{
			return;
		}
	}

	POLL_INIT(&poll);
	_mpc_lowcomm_ib_cp_ctx_poll(&poll, polling_task_id);

	/* If nothing to poll, we poll the CQ */
	if(POLL_GET_RECV(&poll) == 0)
	{
		POLL_INIT(&poll);
		__poll_all_cq(rail, &poll);

		/* If the polling returned something or someone already inside the function,
		 * we retry to poll the pending lists */
		if(POLL_GET_RECV_CQ(&poll) != 0 ||
		   POLL_GET_RECV_CQ(&poll) == POLL_CQ_BUSY)
		{
			POLL_INIT(&poll);
			_mpc_lowcomm_ib_cp_ctx_poll(&poll, polling_task_id);
		}
	}
}

static void sctk_network_connection_to_ib(int from, int to, sctk_rail_info_t *rail)
{
	sctk_ib_cm_connect_to(from, to, rail);
}

static void
sctk_network_connection_from_ib(int from, int to, sctk_rail_info_t *rail)
{
	sctk_ib_cm_connect_from(from, to, rail);
}

int sctk_send_message_from_network_mpi_ib(mpc_lowcomm_ptp_message_t *msg)
{
	int ret = sctk_send_message_from_network_reorder(msg);

	if(ret == REORDER_NO_NUMBERING)
	{
		/*
		 * No reordering
		 */
		_mpc_comm_ptp_message_send_check(msg, 1);
	}

	return ret;
}

void sctk_connect_on_demand_mpi_ib(struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest)
{
	//sctk_on_demand_connection_ib(rail, dest);
    sctk_ib_cm_on_demand_request_monitor(rail, dest);

	/* add_dynamic_route() is not necessary here, as its is done
	 * by the IB handshake protocol deeply in the function above */
}

void sctk_network_initialize_leader_task_mpi_ib(sctk_rail_info_t *rail)
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	/* Fill SRQ with buffers */
	_mpc_lowcomm_ib_ibuf_srq_post(rail_ib);

	/* Initialize Async thread */
	sctk_ib_async_init(rail_ib->rail);

	/* Initialize collaborative polling */
	_mpc_lowcomm_ib_cp_ctx_init(rail_ib);

	LOAD_CONFIG(rail_ib);
	LOAD_DEVICE(rail_ib);
	struct ibv_srq_attr mod_attr;

	mod_attr.srq_limit = config->srq_credit_thread_limit;
	ibv_modify_srq(device->srq, &mod_attr, IBV_SRQ_LIMIT);
}

void sctk_network_initialize_task_mpi_ib(sctk_rail_info_t *rail, int taskid, int vp)
{
	sctk_network_initialize_task_collaborative_ib(rail, taskid, vp);
}

void sctk_network_finalize_task_mpi_ib(sctk_rail_info_t *rail, int taskid, int vp)
{
	sctk_network_finalize_task_collaborative_ib(rail, taskid, vp);
}

/* This hook is called by the allocator when freing a segment */
void sctk_network_memory_free_hook_ib(void *ptr, size_t size)
{
	_mpc_lowcomm_ib_mmu_unpin(ptr, size);
}

/* Pinning */

void sctk_ib_pin_region(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list, void *addr, size_t size)
{
	/* Fill entry */
	list->rail_id = rail->rail_number;
	/* Save the pointer (to free the block returned by _mpc_lowcomm_ib_mmu_entry_new) */
	list->pin.ib.p_entry = _mpc_lowcomm_ib_mmu_pin(&rail->network.ib, addr, size);

	/* Save it inside the struct to allow serialization */
	memcpy(&list->pin.ib.mr, list->pin.ib.p_entry->mr, sizeof(struct ibv_mr) );
}

void sctk_ib_unpin_region(__UNUSED__ struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list)
{
	mpc_common_nodebug("Unpin %p at %p size %ld releaseon %d", list->pin.ib.p_entry, list->pin.ib.p_entry->addr, list->pin.ib.p_entry->size, list->pin.ib.p_entry->free_on_relax);

	_mpc_lowcomm_ib_mmu_relax(list->pin.ib.p_entry);
	list->pin.ib.p_entry = NULL;
	memset(&list->pin.ib.mr, 0, sizeof(struct ibv_mr) );
	list->rail_id = -1;
}

void sctk_network_finalize_mpi_ib(sctk_rail_info_t *rail)
{
	sctk_network_set_ib_unused();
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	LOAD_DEVICE(rail_ib);

	/* Clear the QPs                                              */
	sctk_ib_qp_free_all(rail_ib);

	/* Stop the async event polling thread                        */
	sctk_ib_async_finalize(rail);

	/* collaborative polling shutdown (from leader_initialize)    */
	_mpc_lowcomm_ib_cp_ctx_finalize(rail_ib);

	/* - Free IB topology                                         */
	_mpc_lowcomm_ib_topology_free(rail_ib);

	/* - Flush the eager entries (_mpc_lowcomm_ib_eager_init)             */
	_mpc_lowcomm_ib_eager_finalize(rail_ib);

	/* - Destroy the Shared Receive Queue (sctk_ib_srq_init)      */
	sctk_ib_srq_free(rail_ib);

	/* - Free completion queues (send & recv) sctk_ib_cq_init)    */
	sctk_ib_cq_free(device->send_cq); device->send_cq = NULL;
	sctk_ib_cq_free(device->recv_cq); device->recv_cq = NULL;

	/* - Free the protected domain (sctk_ib_pd_init)              */
	sctk_ib_pd_free(device);

	/* - Free MMU */
	_mpc_lowcomm_ib_mmu_release();

	/* - unload IB device (sctk_ib_device_open)                   */
	/* - close IB device struct (sctk_ib_device_init)             */
	sctk_ib_device_close(&rail->network.ib);


	memset( (void *)vps_reset, 0, sizeof(char) * mpc_topology_get_pu_count() );
}

void sctk_network_init_mpi_ib(sctk_rail_info_t *rail)
{
	sctk_network_set_ib_used();

	/* XXX: memory never freed */
	char *network_name = sctk_malloc(256);

	/* Retrieve config pointers */
	struct _mpc_lowcomm_config_struct_net_rail *rail_config = rail->runtime_config_rail;

	/* init the first time the rail is enabled */
	if(!vps_reset)
	{
		int nbvps = mpc_topology_get_pu_count();
		vps_reset = sctk_malloc(nbvps * sizeof(char) );
		assert(vps_reset);
		memset( (void *)vps_reset, 0, sizeof(char) * nbvps);
	}


	/* Register topology */
	sctk_rail_init_route(rail, rail_config->topology, NULL);

	/* Infiniband Init */
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	memset(rail_ib, 0, sizeof(sctk_ib_rail_info_t) );
	rail_ib->rail    = rail;
	rail_ib->rail_nb = rail->rail_number;

	/* Profiler init */
	sctk_ib_device_t *       device;
	struct ibv_srq_init_attr srq_attr;

	/* Open config */
	sctk_ib_config_init(rail_ib, "MPI");

	/* Open device */
	device = sctk_ib_device_init(rail_ib);

	/* Automatically open the closest IB HCA */
	sctk_ib_device_open(rail_ib, rail_config->device);

	/* Init the MMU (done once) */
	_mpc_lowcomm_ib_mmu_init();

	/* Init Proctection Domain */
	sctk_ib_pd_init(device);

	/* Init Completion Queues */
	device->send_cq = sctk_ib_cq_init(device, rail_ib->config, NULL);
	device->recv_cq = sctk_ib_cq_init(device, rail_ib->config, NULL);

	/* Init SRQ */
	srq_attr = sctk_ib_srq_init_attr(rail_ib);
	sctk_ib_srq_init(rail_ib, &srq_attr);

	/* Configure all channels */
	_mpc_lowcomm_ib_eager_init(rail_ib);

	/* Print config */
	if(mpc_common_get_flags()->verbosity >= 2)
	{
		sctk_ib_config_print(rail_ib);
	}

	_mpc_lowcomm_ib_topology_init_rail(rail_ib);
	_mpc_lowcomm_ib_topology_init_task(rail, mpc_topology_get_pu() );

	/* Initialize network */
	sprintf(network_name, "IB-MT - %dx %s (%d Gb/s)", device->link_width, device->link_rate, device->data_rate);

#ifdef IB_DEBUG
	if(mpc_common_get_process_rank() == 0)
	{
		fprintf(stderr, MPC_COLOR_RED_BOLD(WARNING: MPC debug mode activated.Your job *MAY *be *VERY *slow !) "\n");
	}
#endif

	rail->initialize_task = sctk_network_initialize_task_mpi_ib;
	rail->finalize_task   = sctk_network_finalize_task_mpi_ib;

	sctk_network_initialize_leader_task_mpi_ib(rail);


	rail->control_message_handler = sctk_ib_cm_control_message_handler;

	rail->connect_to        = sctk_network_connection_to_ib;
	rail->connect_from      = sctk_network_connection_from_ib;
	rail->connect_on_demand = sctk_connect_on_demand_mpi_ib;
	rail->driver_finalize   = sctk_network_finalize_mpi_ib;

	rail->send_message_endpoint = _mpc_lowcomm_ib_send_message;

	rail->notify_recv_message       = _mpc_lowcomm_ib_receive_message;
	rail->notify_matching_message   = NULL;
	rail->notify_perform_message    = _mpc_lowcomm_ib_notify_perform;
	rail->notify_idle_message       = _mpc_lowcomm_ib_notify_idle;
	rail->notify_any_source_message = _mpc_lowcomm_ib_notify_anysource;

	/* PIN */
	rail->rail_pin_region   = sctk_ib_pin_region;
	rail->rail_unpin_region = sctk_ib_unpin_region;

	/* RDMA */
	rail->rdma_write = _mpc_lowcomm_ib_rdma_write;
	rail->rdma_read  = _mpc_lowcomm_ib_rdma_read;

	rail->rdma_fetch_and_op_gate = _mpc_lowcomm_ib_rdma_fetch_and_op_gate;
	rail->rdma_fetch_and_op      = _mpc_lowcomm_ib_rdma_fetch_and_op;

	rail->rdma_cas_gate = _mpc_lowcomm_ib_rdma_cas_gate;
	rail->rdma_cas      = _mpc_lowcomm_ib_rdma_cas;

	rail->network_name = network_name;

	rail->send_message_from_network = sctk_send_message_from_network_mpi_ib;

	/* Boostrap the ring only if required */
	if(rail->requires_bootstrap_ring)
	{
		/* Bootstrap a ring on this network */
		//sctk_ib_cm_connect_ring(rail);
	}


    sctk_ib_cm_monitor_register_callbacks(rail);

}
