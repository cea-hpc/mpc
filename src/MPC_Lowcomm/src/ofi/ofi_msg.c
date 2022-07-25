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
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */
#include "ofi_msg.h"
#include "ofi_msg_types.h"
#include "ofi_toolkit.h"

#include <sctk_alloc.h>
#include <sctk_rail.h>
#include "endpoint.h"
#include <sctk_net_tools.h>
#include <mpc_common_spinlock.h>

#include <mpc_common_datastructure.h>
#include <mpc_common_rank.h>
#include <mpc_launch_pmi.h>
#include <mpc_lowcomm_monitor.h>

/** Lookup table to cache contexts associated to each endpoint */
mpc_lowcomm_ofi_ep_t* *ctx_ep_table = NULL;

/**
 * @brief ctx_ep_table accessor, will ease to move this functionality to 
 * something else than a global variable.
 * 
 * @param rank the rank to get the context from
 * @return mpc_lowcomm_ofi_ep_t* a pointer to the context
 */
static inline mpc_lowcomm_ofi_ep_t* __mpc_lowcomm_ofi_get_ctx_from_process_rank(int rank)
{
	assert(rank >= 0);
	assert(rank < mpc_common_get_process_count());
	return ctx_ep_table[rank];
}


/**
 * @brief ctx_ep_table accessor, will ease moving the cache to something better than a global variable.
 * 
 * @param rank the rank this context belongs
 * @param ctx the actual context to set
 */
static inline void __mpc_lowcomm_ofi_set_ctx_from_process_rank(int rank, mpc_lowcomm_ofi_ep_t* ctx)
{
	assert(ctx);
	assert(rank >= 0);
	assert(rank < mpc_common_get_process_count());
	assert(!ctx_ep_table[rank]);
	ctx_ep_table[rank] = ctx;
}

/**
 * @brief Set the 'server' side for the current process.
 * 
 * This is set for any process, to handle new incoming connections.
 * 
 * @param orail the driver-specific rail owning the current configuraiton
 */
static inline void __sctk_network_ofi_run_passive(mpc_lowcomm_ofi_rail_info_t* orail)
{
	assert(orail);
	mpc_lowcomm_ofi_rail_spec_msg_t* msg_ofi = &orail->spec.msg;
	struct fi_eq_attr eq_attr;
	
	/* in case latter versions of this struct add new fields, this struct should remain memset'd */
	memset(&eq_attr, 0, sizeof(struct fi_eq_attr));
	eq_attr = (struct fi_eq_attr) {
		.size             = MPC_LOWCOMM_OFI_EQ_SIZE, /* EQ size */
		.flags            = 0,                /* special case */
		.wait_obj         = FI_WAIT_UNSPEC,   /* Waiting procedure */
		.signaling_vector = 0,                /* range of cores to signal when incoming event */
		.wait_set         = NULL              /* wait set to wait on */
	};
	struct fi_poll_attr poll_attr = (struct fi_poll_attr) {
		.flags            = 0                 /* reserved, should not be used for now */
	};

	/* create new fabric objects */
	MPC_LOWCOMM_OFI_CHECK_RC(fi_passive_ep(orail->fabric, orail->provider, &msg_ofi->pep, NULL));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_eq_open(orail->fabric, &eq_attr, &msg_ofi->cm_eq, NULL));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_poll_open(orail->domain, &poll_attr, &msg_ofi->cq_send_pollset));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_poll_open(orail->domain, &poll_attr, &msg_ofi->cq_recv_pollset));
	sctk_spin_rwlock_init(&msg_ofi->pollset_lock);

	/* bind the event queue to this passive endpoint to receive new incoming requests */
	MPC_LOWCOMM_OFI_CHECK_RC(fi_ep_bind((struct fid_ep*)msg_ofi->pep, &msg_ofi->cm_eq->fid, 0));
 	MPC_LOWCOMM_OFI_CHECK_RC(fi_listen(msg_ofi->pep));
}

/**
 * @brief post a new recv() on the provided endpoint
 * 
 * @param ep_ctx the context owned by the endpoint a recv() wants to be posted into
 */
static inline void __mpc_lowcomm_ofi_msg_post_recv(mpc_lowcomm_ofi_ep_t* ep_ctx)
{
	assert(ep_ctx);
	MPC_LOWCOMM_OFI_CHECK_RC(fi_recv(ep_ctx->ep, &ep_ctx->spec.msg.sz_buf, sizeof(ep_ctx->spec.msg.sz_buf), NULL, FI_ADDR_UNSPEC, ep_ctx));
}

/**
 * @brief register a new fabric endpoint by creating a new route withing low_level_comm.
 * 
 * @param rail the driver-agnostic rail owning the future route
 * @param remote the remote rank the route will be directed to
 * @param side in which 'state' the process calling this function is ? server-side or client-side ?
 * @param origin route type (DYNAMIC/STATIC)
 * @param infos actual infos used to initialize a fabric endpoint (content depends on the 'side' parameter)
 * @return mpc_lowcomm_ofi_ep_t* the newly allocated context for the created endpoint/route
 */
static inline mpc_lowcomm_ofi_ep_t* __mpc_lowcomm_ofi_register_endpoint(sctk_rail_info_t* rail, mpc_lowcomm_peer_uid_t remote, int side, _mpc_lowcomm_endpoint_type_t origin, char* infos)
{
	assert(rail);
	assert(remote !=  mpc_lowcomm_monitor_get_uid());
	assert(side == MPC_LOWCOMM_OFI_PASSIVE_SIDE || side == MPC_LOWCOMM_OFI_ACTIVE_SIDE);
	mpc_lowcomm_ofi_ep_t* ep_ctx = NULL;
	mpc_lowcomm_ofi_rail_info_t* orail = NULL;
	struct fid_cq* cq_recv = NULL, *cq_send = NULL;
	struct fid_ep* ep = NULL;
	struct fi_cq_attr cq_attr;
	
	orail = &rail->network.ofi;
	
	/* ensure latter versions adding new fields won't be an issue */
	memset(&cq_attr, 0, sizeof(struct fi_cq_attr));
	cq_attr =  (struct fi_cq_attr) {
		.size             = MPC_LOWCOMM_OFI_CQ_SIZE, /* EQ size */
		.flags            = 0,                /* spacial case */
		.format           = FI_CQ_FORMAT_MSG, /* format of the CQ event returned */
		.wait_obj         = FI_WAIT_UNSPEC,   /* Waiting procedure */
		.signaling_vector = 0,                /* range of cores to signal when incoming event */
		.wait_cond        = FI_CQ_COND_NONE,  /* no condition on waiting for a event */
		.wait_set         = NULL              /* wait set to wait on */
	};

	ep_ctx = sctk_malloc(sizeof(mpc_lowcomm_ofi_ep_t));
	
	/* 
	   - if active side, 'infos' contain the remote server string, used by fi_connect(),
	   - if passive side, 'infos' contain entry.info data, from remote client, used when creating the endpoint
	*/	
	if(side == MPC_LOWCOMM_OFI_ACTIVE_SIDE)
	{
		MPC_LOWCOMM_OFI_CHECK_RC(fi_endpoint(orail->domain, orail->provider, &ep, ep_ctx));
	}
	else
	{
		MPC_LOWCOMM_OFI_CHECK_RC(fi_endpoint(orail->domain, (struct fi_info*)infos, &ep, ep_ctx));
	}

	/* create new fabric objects for the new endpoint */
	MPC_LOWCOMM_OFI_CHECK_RC(fi_cq_open(orail->domain, &cq_attr, &cq_recv, ep_ctx));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_cq_open(orail->domain, &cq_attr, &cq_send, ep_ctx));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_ep_bind(ep, &orail->spec.msg.cm_eq->fid, 0));
	/* Note:  be aware that FI_SELECTIVE_COMPLETION implies the 'sender' to explicitly ask for a local completion */
	MPC_LOWCOMM_OFI_CHECK_RC(fi_ep_bind(ep, &cq_send->fid, FI_TRANSMIT | FI_SELECTIVE_COMPLETION));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_ep_bind(ep, &cq_recv->fid, FI_RECV));
	*ep_ctx = (mpc_lowcomm_ofi_ep_t){
		.ep              = ep,
		.cq_r            = cq_recv,
		.cq_s            = cq_send,
		.lock            = MPC_COMMON_SPINLOCK_INITIALIZER,
		.spec.msg.sz_buf = 0
	};

	/* both these calls are enabling the endpoint */
	if(side == MPC_LOWCOMM_OFI_ACTIVE_SIDE)
	{
		int process_rank = mpc_common_get_process_rank();
		/* the immediate data will be stored in the event polled from the other side */
		MPC_LOWCOMM_OFI_CHECK_RC(fi_connect(ep, infos, &process_rank, sizeof(int)));
	}
	else
	{
		/* here, we don't care of exchanged data, as this accept() is motivated by an already-received connect() */
		MPC_LOWCOMM_OFI_CHECK_RC(fi_accept(ep, NULL, 0));
	}

	/* update the pollsets for being notified of incoming event on this endpoint
	* Is the lock really necessary if the domain is set w/ THREAD_SAFE ?
	*/
	mpc_common_spinlock_write_lock(&orail->spec.msg.pollset_lock);
	fi_poll_add(orail->spec.msg.cq_send_pollset, &cq_send->fid, 0);
	fi_poll_add(orail->spec.msg.cq_recv_pollset, &cq_recv->fid, 0);
	mpc_common_spinlock_write_unlock(&orail->spec.msg.pollset_lock);

	ep_ctx->sctk_ep = mpc_lowcomm_ofi_add_route(remote, ep_ctx, rail, origin, _MPC_LOWCOMM_ENDPOINT_CONNECTING);
	return ep_ctx;
}

/**
 * @brief Poll for a new EVENT (connection management oriented).
 * 
 * @param rail the rail owning the event-queue to poll
 * @return mpc_lowcomm_ofi_ep_t* the context associated with the received endpoint, if any (can return NULL if no event have been received)
 */
static inline mpc_lowcomm_ofi_ep_t* __mpc_lowcomm_ofi_poll_eq(sctk_rail_info_t* rail)
{
	assert(rail);

	mpc_lowcomm_ofi_rail_info_t* orail = &rail->network.ofi;
	mpc_lowcomm_ofi_ep_t* ep_new = NULL;
	int ret = -1;
	uint32_t type = -1;
	struct fi_eq_cm_entry * entry = NULL;

	entry = alloca(sizeof(struct fi_eq_cm_entry) + sizeof(int));
	MPC_LOWCOMM_OFI_CHECK_RC((ret = fi_eq_read(orail->spec.msg.cm_eq, &type, entry, sizeof(struct fi_eq_cm_entry)+sizeof(int), 0)));
	if(ret > 0)
	{
		switch(type)
		{
			case FI_CONNREQ: /* an incoming connection request, connection info are stored in the entry */
			{
				/* retrieve the remote rank ID for this connection request (written in immediata data) */
				int remote_rank = *((int*)entry->data);
				ep_new = __mpc_lowcomm_ofi_register_endpoint(rail, remote_rank, MPC_LOWCOMM_OFI_PASSIVE_SIDE, _MPC_LOWCOMM_ENDPOINT_DYNAMIC, (char*)entry->info);
				break;
			}
			case FI_CONNECTED: /* the endpoint can be now enabled */
			{
				/* both the passive and active side receive such event. As the behavior for handling immediate data
				is different, we are not relying on it. A connection should already be pending (_MPC_LOWCOMM_ENDPOINT_CONNECTING).
				By retrieving the endpoint, we just set the route as usable now
				*/
				mpc_lowcomm_ofi_ep_t* ep_ctx = (mpc_lowcomm_ofi_ep_t*)entry->fid->context;
				_mpc_lowcomm_endpoint_t* ep = (_mpc_lowcomm_endpoint_t*)ep_ctx->sctk_ep;
				/* post a first recv() to be able to poll new event for incoming msgs */
				__mpc_lowcomm_ofi_msg_post_recv(ep_ctx);
				_mpc_lowcomm_endpoint_set_state(ep, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
				__mpc_lowcomm_ofi_set_ctx_from_process_rank(ep->dest, ep_ctx);
				break;
			}
			case FI_SHUTDOWN: /* be careful, the EQ has to be detached from any endpoint before calling fi_close() */
				break;
			default:
				not_reachable();
		}
	}
	return ep_new;
}

/**
 * @brief A new recv is available on a CQ and will be processed by this function.
 * 
 * @param rail the current OFI rail
 * @param ep_ctx the context attached to the endpoint with a new incoming recv()
 */
static inline void __mpc_lowcomm_ofi_msg_read_recv_data(sctk_rail_info_t* rail, mpc_lowcomm_ofi_ep_t* ep_ctx)
{
	assert(rail);
	assert(ep_ctx);
	mpc_lowcomm_ptp_message_t* msg = NULL;
	int ret;
	void * addr = NULL;
	struct fi_cq_msg_entry entry;

	/* OFI guaranteess message ordering between two processes but not completion ordering.
	 * Extra events can be in the CQ between the two recv() (size & payload).
	 * For this reason, this 'interleave' buffer is used to register potential interleaved recv(). 
	*/
	mpc_lowcomm_ofi_ep_t* interleave[MPC_LOWCOMM_OFI_MAX_INTERLEAVE];
	memset(interleave, 0, sizeof(mpc_lowcomm_ofi_ep_t*) * MPC_LOWCOMM_OFI_MAX_INTERLEAVE);
	size_t nb_inter = 0;

	/* lock to ensure the second rec() won't be stealed from somewhere else */
	mpc_common_spinlock_lock(&ep_ctx->lock);
	MPC_LOWCOMM_OFI_CHECK_RC((ret = fi_cq_read(ep_ctx->cq_r, &entry, 1)));
	if(ret > 0)
	{
		if(entry.flags & (FI_RECV | FI_MSG)) /* if two-sided message */
		{
			/* Prepare to recv the message from the endpoint */
			msg = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t) + ep_ctx->spec.msg.sz_buf);
			addr = (char*) msg + sizeof(mpc_lowcomm_ptp_message_t);
			struct iovec iov[2];
			iov[0] = (struct iovec) {.iov_base = &msg->body, .iov_len = sizeof(mpc_lowcomm_ptp_message_body_t)};
			iov[1] = (struct iovec) {.iov_base = addr, .iov_len = ep_ctx->spec.msg.sz_buf};
				struct fi_msg msg_v = (struct fi_msg) {
				.msg_iov = iov,
				.desc = NULL,
				.iov_count = 2,
				.addr = FI_ADDR_UNSPEC,
				.context = msg,
				.data = 0
			};
			/* emit the local recv */
			MPC_LOWCOMM_OFI_CHECK_RC(fi_recvmsg(ep_ctx->ep, &msg_v, FI_COMPLETION));
			do
			{
				/* wait for its completion. 
				/!\ While data ordering is preserved, completion ordering is not !
				We have to ensure such reliability and interleaved events are stored 
				to be processed later on.
				*/
				MPC_LOWCOMM_OFI_CHECK_RC((ret = fi_cq_sread(ep_ctx->cq_r, &entry, 1, NULL, -1)));
				if(entry.op_context != msg)
				{
					assert(nb_inter < MPC_LOWCOMM_OFI_MAX_INTERLEAVE);
					interleave[nb_inter++] = entry.op_context;
				}
			} while(entry.op_context != msg);
			/* post a new size-specific recv() */
			__mpc_lowcomm_ofi_msg_post_recv(ep_ctx);
			SCTK_MSG_COMPLETION_FLAG_SET(msg, NULL);
			msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;
			_mpc_comm_ptp_message_clear_request(msg);
			_mpc_comm_ptp_message_set_copy_and_free(msg, sctk_free, sctk_net_message_copy);
			/* By releasing the lock, we will avoid unecessary yield() by {inter,low}_level_comm */
			mpc_common_spinlock_unlock(&ep_ctx->lock);
			rail->send_message_from_network(msg);
		}
		else if(entry.flags & (FI_RMA | FI_REMOTE_READ | FI_REMOTE_WRITE)) /* if one-sided operation */
		{
			mpc_common_spinlock_unlock(&ep_ctx->lock);
		}
	}
	else
	{
		mpc_common_spinlock_unlock(&ep_ctx->lock);
	}
	
	/* post process interleaved event. It should only be size-specific events */
	for(;nb_inter > 0;nb_inter--)
	{
		__mpc_lowcomm_ofi_msg_read_recv_data(rail, interleave[nb_inter-1]);
	}
}

/**
 * @brief a completion is processed here for a sending completion.
 * 
 * @param sctk_rail_info_t* the current rail
 * @param ep_ctx the context associated to endpoint emitting the completion
 */
static inline void __mpc_lowcomm_ofi_msg_read_send_data(__UNUSED__ sctk_rail_info_t* rail, mpc_lowcomm_ofi_ep_t* ep_ctx)
{
	assert(rail);
	assert(ep_ctx);
	struct fi_cq_msg_entry entry;
	int ret;
	mpc_lowcomm_ptp_message_t* msg = NULL;

	/* read the completion & free the message */
	MPC_LOWCOMM_OFI_CHECK_RC((ret = fi_cq_read(ep_ctx->cq_s, &entry, 1)));
	if(ret > 0)
	{
		/* if any case, if a initiator side completion is received, the operation completed and the MPC msg can be freed */
		if(entry.flags & (FI_SEND | FI_MSG) || entry.flags & (FI_RMA | FI_READ | FI_WRITE))
		{
			msg = (mpc_lowcomm_ptp_message_t*)entry.op_context;
			mpc_lowcomm_ptp_message_complete_and_free(msg);
		}
	}
}

/**
 * @brief Look for a new completion for the given queue.
 * 
 * @param rail the current rail
 * @param pollset the set attached to one or multiple completion queues
 * @param complete the completion function to call (send or recv completion ?)
 */
static inline void __mpc_lowcomm_ofi_poll_cq(sctk_rail_info_t* rail, struct fid_poll* pollset, void(*complete)(sctk_rail_info_t* rail, mpc_lowcomm_ofi_ep_t*))
{
	assert(rail);
	assert(pollset);
	assert(complete);
	void* context[MPC_LOWCOMM_OFI_CQ_POLL_MAX];
	int nb, i;

	/* lock used to dyanmically add new CQ to pollset when endpoints are created dynamically */
	mpc_common_spinlock_read_lock(&rail->network.ofi.spec.msg.pollset_lock);
	MPC_LOWCOMM_OFI_CHECK_RC((nb = fi_poll(pollset, (void**)&context, MPC_LOWCOMM_OFI_CQ_POLL_MAX)));
	mpc_common_spinlock_read_unlock(&rail->network.ofi.spec.msg.pollset_lock);

	if(nb > 0)
	{
		for(i = 0; i < nb; i++)
		{
			mpc_lowcomm_ofi_ep_t* ep_ctx = context[i];
			assert(ep_ctx);
			assert(complete);
			complete(rail, ep_ctx);
		}
	}
}

/**
 * @brief Initialize the driver in a connected-mode.
 * 
 * @param rail the rail to initialize
 */
static inline void __sctk_network_ofi_bootstrap(sctk_rail_info_t* rail)
{

	assert(rail);
	mpc_lowcomm_ofi_rail_info_t* orail = &rail->network.ofi;
	orail->name_info_sz = FI_NAME_MAX;

	/* initialize a passive ep, always ready to listen for new connections */
	__sctk_network_ofi_run_passive(orail);

	/* retrieve the passive endpoint ID for the current process */
	MPC_LOWCOMM_OFI_CHECK_RC(fi_getname((fid_t)orail->spec.msg.pep, orail->name_info, &orail->name_info_sz));

	/* This is the old ring code left here for reference of how to connect */
#if 0
	mpc_lowcomm_ofi_ep_t* ctx_connect = NULL, *ctx_accept = NULL;
	unsigned char raw_connection_infos[MPC_COMMON_MAX_STRING_SIZE], *connection_infos;
	int right_rank, my_rank, nb_ranks;
	int will_connect, will_accept;
	
	/* bunch of setups */
	my_rank = mpc_common_get_process_rank();
	nb_ranks = mpc_common_get_process_count();
	will_connect = my_rank == 0 || nb_ranks >2;
	will_accept = my_rank == 1 || nb_ranks > 2;
	right_rank = ( my_rank + 1 ) % nb_ranks;

	/* it has to be encoded because what fi_getname() returns is not always PMI-format compatible (strings) */
	connection_infos = mpc_common_datastructure_base64_encode((unsigned char*)orail->name_info, orail->name_info_sz, NULL);
	assume ( mpc_launch_pmi_put_as_rank((char *)connection_infos, rail->rail_number, 0 ) == 0 );
	mpc_launch_pmi_barrier();
	sctk_free(connection_infos);

	/* here is the trick :
		1. A connect() request is sent to the 'passive' process to establish a connection.
		2. Asynchronously, the pasive side will process the event and accept the new connection.
		3. The active side then just has to wait to receive an FI_CONNECT event, meaning that both sides are
			all set up.
		4. The only pitfall here would be to use the endpoint before the 'passive' side get the notification.
			But as we are at initialisation step, the PMI guarantees us no message will be on the wire before
			we leave this function
	*/
	if(will_connect)
	{
		assume ( mpc_launch_pmi_get_as_rank ( (char *)raw_connection_infos, MPC_COMMON_MAX_STRING_SIZE, rail->rail_number, right_rank ) == 0 );
		connection_infos = mpc_common_datastructure_base64_decode((unsigned char*)raw_connection_infos, strlen((char*)raw_connection_infos), NULL);
		ctx_connect = __mpc_lowcomm_ofi_register_endpoint(rail, right_rank, MPC_LOWCOMM_OFI_ACTIVE_SIDE, _MPC_LOWCOMM_ENDPOINT_STATIC, (char*)connection_infos);
		sctk_free(connection_infos);
	}

	if(will_accept)
	{
		/* the __mpc_lowcomm_ofi_poll_eq looks for the next event. it is returning the attached context if 
		a new endpoint is created during the process. In that case, we should receive an incoming request
		to create a new connection. By doing so, we are waiting to retrieve the context handler
		*/
		while(!ctx_accept)
			ctx_accept =  __mpc_lowcomm_ofi_poll_eq(rail);
	}

	/* while rings is not fully set up, poll the EQ (at least one is required to progress the connect()) */
	do
	{
		(void) __mpc_lowcomm_ofi_poll_eq(rail);
	}
	while((	ctx_connect && _mpc_lowcomm_endpoint_get_state((_mpc_lowcomm_endpoint_t*)ctx_connect->sctk_ep) != _MPC_LOWCOMM_ENDPOINT_CONNECTED) ||
		(	ctx_accept  && _mpc_lowcomm_endpoint_get_state((_mpc_lowcomm_endpoint_t*)ctx_accept->sctk_ep) != _MPC_LOWCOMM_ENDPOINT_CONNECTED));
	
	/* the ring need to be fully initialized before continuing...
	 * Could this be replaced with a OFI-based barrier ? 
	 * But this is "only" the second barrier of the driver initialisation 
	*/
	mpc_launch_pmi_barrier();
#endif
}

/**
 * @brief Send a message on the given elected endpoint.
 * 
 * @param msg the MPC message to send.
 * @param endpoint the endpoint elected by the low_level_comm to be used for the transfer
 */
static void _mpc_lowcomm_ofi_send_message ( mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint )
{
	assert(endpoint);
	assert(msg);

	void* addr = NULL;
	size_t msg_size;
	mpc_lowcomm_ofi_ep_t* ep_ctx = NULL;
	
	ep_ctx   = (mpc_lowcomm_ofi_ep_t*)endpoint->data.ofi.ctx;
	msg_size = SCTK_MSG_SIZE(msg);

	/* if not contiguous, the buffer needs to be copied as going to be sent as a contiguous one.
	 * Is this still valid considering that libfabric can deal with iovecs ? Maybe, because one can be
	 * sure that the rec() side will comply with the datatype from the sender side
	 */
	switch(msg->tail.message_type)
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		case MPC_LOWCOMM_MESSAGE_NETWORK:
			addr = msg->tail.message.contiguous.addr;
			break;
		default:
			addr = sctk_malloc(msg_size);
			sctk_net_copy_in_buffer(msg, addr);
			break;
	}
	
	/* a lock is required to ensure the two sends (size & payload) are not interleaved with another send()
	 * Note that this send() WILL NOT generate an event !!
	 */
	mpc_common_spinlock_lock(&ep_ctx->lock);
	MPC_LOWCOMM_OFI_CHECK_RC(fi_send(ep_ctx->ep, &msg_size, sizeof(size_t), NULL, FI_ADDR_UNSPEC, msg)); /* size */

	/* build the iovec sending the message_body & the payload (sending a completion event) */
	struct iovec iov[2];
	iov[0] = (struct iovec) {.iov_base = msg, .iov_len = sizeof(mpc_lowcomm_ptp_message_body_t)};
	iov[1] = (struct iovec) {.iov_base = addr, .iov_len = msg_size};
	struct fi_msg msg_v = (struct fi_msg) {
		.msg_iov   = iov,
		.desc      = NULL,
		.iov_count = 2,
		.addr      = FI_ADDR_UNSPEC,
		.context   = msg,
		.data      = 0
	};
	MPC_LOWCOMM_OFI_CHECK_RC(fi_sendmsg(ep_ctx->ep, &msg_v, FI_COMPLETION));
	mpc_common_spinlock_unlock(&ep_ctx->lock);
}

/**
 * Called by idle threads to progress messages.
 * \param[in] rail the rail where communications should be progressed on
 */
static void _mpc_lowcomm_ofi_notify_idle(struct sctk_rail_info_s* rail)
{
	assert(rail);
	static volatile int val = 0;

	/* poll both recv() and send() at least once */
	__mpc_lowcomm_ofi_poll_cq(rail, rail->network.ofi.spec.msg.cq_recv_pollset, __mpc_lowcomm_ofi_msg_read_recv_data);
	__mpc_lowcomm_ofi_poll_cq(rail, rail->network.ofi.spec.msg.cq_send_pollset, __mpc_lowcomm_ofi_msg_read_send_data);

	/* occasionnally, check for new incoming events */
	if((val++ % 1000) == 0)
		(void) __mpc_lowcomm_ofi_poll_eq(rail);
}

/**
 * Handler triggering the send_message_from_network call, before reaching the inter_thread_comm matching process.
 * \param[in] msg the message received from the network, to be matched w/ a local RECV.
 */
static inline int sctk_send_message_from_network_ofi_msg ( mpc_lowcomm_ptp_message_t *msg )
{
	if ( _mpc_lowcomm_reorder_msg_check ( msg ) == _MPC_LOWCOMM_REORDER_NO_NUMBERING )
	{
		/* No reordering */
		_mpc_comm_ptp_message_send_check ( msg, 1 );
	}
	return 1;
}

/**
 * @brief Properly lose the rail.
 * 
 * Currently not completely implemented
 * 
 * @param rail the rail to close
 */
void sctk_network_finalize_ofi_msg(sctk_rail_info_t *rail)
{
    sctk_free(rail->network_name);
}

static char * __ofi_rail_name(sctk_rail_info_t *rail, char * buff, int bufflen)
{
	snprintf(buff, bufflen, "ofi_%d", rail->rail_number);
	return buff;
}

/**
 * @brief Create on-demand route when asked by low_level_comm.
 * 
 * Here, the on-demand connections still not relies on already-established routes and use the registered PMI keys
 * to retrieve remote information to open a new connection.
 * 
 * @param rail the rail owning the future route
 * @param dest_process the process to connect to
 */
void mpc_lowcomm_ofi_msg_on_demand_handler( struct sctk_rail_info_s *rail, mpc_lowcomm_peer_uid_t dest_process )
{
	assert(rail);
	assert(dest_process >= 0 && dest_process !=  mpc_common_get_process_rank());

	mpc_lowcomm_ofi_ep_t* ep_ctx = NULL;
	unsigned char *connection_infos = NULL;

	char rail_name[32];
	mpc_lowcomm_monitor_retcode_t ret = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_ondemand(dest_process,
																	   __ofi_rail_name(rail, rail_name, 32),
																	   NULL,
																	   0,
																	   &ret);

	if(ret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_fatal("Could not connect to UID %lu", dest_process);
	}

	mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

	connection_infos = mpc_common_datastructure_base64_decode(content->on_demand.data, strlen(content->on_demand.data), NULL);

	ep_ctx = __mpc_lowcomm_ofi_register_endpoint(rail, dest_process, MPC_LOWCOMM_OFI_ACTIVE_SIDE, _MPC_LOWCOMM_ENDPOINT_DYNAMIC, (char*)connection_infos);

	/* Wait for route completion, this function cannot be exited without this guarantee... */
	while(_mpc_lowcomm_endpoint_get_state((_mpc_lowcomm_endpoint_t*)ep_ctx->sctk_ep) != _MPC_LOWCOMM_ENDPOINT_CONNECTED)
	{
		(void) __mpc_lowcomm_ofi_poll_eq(rail);
	}

	mpc_lowcomm_monitor_response_free(resp);
}


static int __ofi_monitor_callback(mpc_lowcomm_peer_uid_t from,
								  char *data,
								  char * return_data,
								  int return_data_len,
								  void *ctx)
{
	sctk_rail_info_t *rail = (sctk_rail_info_t *)ctx;
	mpc_lowcomm_ofi_rail_info_t* orail = &rail->network.ofi;

	/* Here we return the encoded OFI connection infos */
	char * connection_infos = mpc_common_datastructure_base64_encode((unsigned char*)orail->name_info, orail->name_info_sz, NULL);

	assume(connection_infos != NULL);
	assume(strlen(connection_infos) < return_data_len);

	snprintf(return_data, return_data_len, "%s", connection_infos);

	sctk_free(connection_infos);

	return 0;
}

static inline void __register_in_monitor(sctk_rail_info_t *rail)
{
	char rail_name[32];
	__ofi_rail_name(rail, rail_name, 32);

	mpc_lowcomm_monitor_register_on_demand_callback(rail_name, __ofi_monitor_callback, rail);
}


/**
 * @brief test to assess if current rail supports fetch_and_op operations.
 * No for most of connected-endpoint providers we used to work on (FI_ATOMIC)
 * 
 * @param rail the current rail
 * @param size Operation size
 * @param op the actual operation
 * @param type datatype
 * @return int a boolean, currently 0 as not supported
 */
int mpc_lowcomm_ofi_msg_fetch_and_op_gate( sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type )
{
	UNUSED(rail); 
	UNUSED(size);
	UNUSED(type);
	UNUSED(op);
	return 0;
}

/**
 * @brief test to assess if current rail support compare_and_swap operations.
 * Not supported for most of connected-endpoint providers we used to work on (FI_ATOMIC)
 * 
 * @param rail the current rail
 * @param size the operation size
 * @param type the datatype
 * @return int a boolean, currently 0 as not supported
 */
int mpc_lowcomm_ofi_msg_cas_gate( sctk_rail_info_t *rail, size_t size, RDMA_type type )
{
	UNUSED(rail);
	UNUSED(size);
	UNUSED(type);
	return 0;
}

/**
 * Implement the one-sided Write() method.
 *
 *
 * \param[in] rail the current rail
 * \param[in] msg the built msg
 * \param[in] src_addr for WRITE, the local addr where data will be picked up
 * \param[in] local_key the local RDMA struct associated to src_addr
 * \param[in] dest_addr for WRITE, the remote addr where data will be written
 * \param[in] remote_key the remote RDMA struct associated to dest_addr
 * \param[in] size the amount of bytes to send
 */
void mpc_lowcomm_ofi_msg_write(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
		void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * remote_key,
		size_t size)
{
	assert(rail);
	assert(msg);
	assert(src_addr);
	assert(dest_addr);
	assert(local_key);
	assert(remote_key);
	UNUSED(local_key);

	struct fid_mr *mr = NULL;
	mpc_lowcomm_ofi_ep_t* ctx = __mpc_lowcomm_ofi_get_ctx_from_process_rank(SCTK_MSG_DEST_PROCESS(msg));
	void* desc = NULL;
	struct iovec iov = (struct iovec) { .iov_base = src_addr, .iov_len = size };
	struct fi_rma_iov iov_rma = (struct fi_rma_iov) {.addr = (uint64_t)dest_addr, .len = size, .key = remote_key->pin.ofi.key};

	/* register the local buffer in a memory region as providers may require the memory to be pinned for the operation to success */
	MPC_LOWCOMM_OFI_CHECK_RC(fi_mr_reg(rail->network.ofi.domain, src_addr, size, FI_WRITE, 0, MPC_LOWCOMM_OFI_RDMA_KEY, 0, &mr, NULL));
	desc = fi_mr_desc(mr);

	struct fi_msg_rma rma_msg = (struct fi_msg_rma)
	{
		.msg_iov       = &iov,
		.desc          = &desc,
		.iov_count     = 1,
		.addr          = FI_ADDR_UNSPEC,
		.rma_iov       = &iov_rma,
		.rma_iov_count = 1,
		.context       = msg,
		.data          = 0
	};

	MPC_LOWCOMM_OFI_CHECK_RC(fi_writemsg(
		ctx->ep,
		&rma_msg,
		FI_COMPLETION
	));
}

/**
 * Implement the one-sided Read() method.
 *
 * \param[in] rail the curent rail
 * \param[in] msg the built msg
 * \param[in] src_addr for READ, the remote addr where data will be read
 * \param[in] remote_key the remote RDMA struct associated to src_addr
 * \param[in] dest_addr for READ, the local addr where data will be written
 * \param[in] remote_key the local RDMA struct associated to dest_addr
 * \param[in] size the amount of bytes to send
 */
void mpc_lowcomm_ofi_msg_read(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
		void * src_addr,  struct  sctk_rail_pin_ctx_list * remote_key,
		void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
		size_t size)
{
	assert(rail);
	assert(msg);
	assert(src_addr);
	assert(dest_addr);
	assert(local_key);
	assert(remote_key);
	UNUSED(local_key);

	struct fid_mr *mr = NULL;
	mpc_lowcomm_ofi_ep_t* ctx = __mpc_lowcomm_ofi_get_ctx_from_process_rank(SCTK_MSG_DEST_PROCESS(msg));
	void* desc = NULL;
	struct iovec iov = (struct iovec) { .iov_base = dest_addr, .iov_len = size };
	struct fi_rma_iov iov_rma = (struct fi_rma_iov) {.addr = (uint64_t)src_addr, .len = size, .key = remote_key->pin.ofi.key};

	/* register the local buffer in a memory region as providers may require the memory to be pinned for the operation to success */
	MPC_LOWCOMM_OFI_CHECK_RC(fi_mr_reg(rail->network.ofi.domain, src_addr, size, FI_WRITE, 0, MPC_LOWCOMM_OFI_RDMA_KEY, 0, &mr, NULL));
	desc = fi_mr_desc(mr);
	struct fi_msg_rma rma_msg = (struct fi_msg_rma)
	{
		.msg_iov       = &iov,
		.desc          = &desc,
		.iov_count     = 1,
		.addr          = FI_ADDR_UNSPEC,
		.rma_iov       = &iov_rma,
		.rma_iov_count = 1,
		.context       = msg,
		.data          = 0
	};

	MPC_LOWCOMM_OFI_CHECK_RC(fi_readmsg(
		ctx->ep,
		&rma_msg,
		FI_COMPLETION
	));
}

/**
 * Register a new segment, reachable for RDMA operations.
 *
 * \param[in] rail the current rail
 * \param[out] list the driver-specific ctx to fill
 * \param[in] addr the start address
 * \param[in] size the region length
 */
void mpc_lowcomm_ofi_msg_pin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list, void * addr, size_t size )
{
	assert(rail);
	assert(list);

	MPC_LOWCOMM_OFI_CHECK_RC(fi_mr_reg(rail->network.ofi.domain, addr, size, FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE, 0, MPC_LOWCOMM_OFI_RDMA_KEY, 0, &list->pin.ofi.mr, NULL));
	list->pin.ofi.start = addr;
	list->pin.ofi.size = size;
	list->pin.ofi.key = fi_mr_key(list->pin.ofi.mr);
}

/**
 * Destroy a previously-registered segment.
 *
 * Note that all requests must have been completed before calling this function.
 * If not, some hardware implementation can hang.
 *
 * \param[in] rail the current rail
 * \param[in] list the ctx filled when segment was pinned
 */
void mpc_lowcomm_ofi_msg_unpin_region( struct sctk_rail_info_s * rail, struct sctk_rail_pin_ctx_list * list )
{
	UNUSED(rail);
	assert(rail);
	assert(list);
	MPC_LOWCOMM_OFI_CHECK_RC(fi_close(&list->pin.ofi.mr->fid));
}

/**
 * @brief Dealing with control-messages for the current rail.
 * 
 * Not useful for now, as on-demand is handled through PMI.
 */
void sctk_network_ofi_control_message_handler( __UNUSED__ struct sctk_rail_info_s * rail, __UNUSED__ int source_process, __UNUSED__ int source_rank, __UNUSED__ char subtype, __UNUSED__ char param, __UNUSED__ void * data, __UNUSED__ size_t size )
{
	/* nothing to do for now... */
}

static void _mpc_lowcomm_ofi_msg_notify_anysource ( int polling_task_id, int blocking, sctk_rail_info_t *rail )
{
	_mpc_lowcomm_ofi_notify_idle(rail);
}

/**
 * @brief Initialize a libfabric rail, in connected mode.
 * 
 * @param rail the rail to initialize
 */
void sctk_network_init_ofi_msg( sctk_rail_info_t *rail )
{
	assert(rail);
	mpc_lowcomm_ofi_rail_info_t* orail = &rail->network.ofi;

	/* Register Hooks in rail */
	rail->send_message_endpoint     = _mpc_lowcomm_ofi_send_message;
	rail->notify_recv_message       = NULL;
	rail->notify_matching_message   = NULL;
	rail->notify_perform_message    = NULL;
	rail->notify_idle_message       = _mpc_lowcomm_ofi_notify_idle;
	rail->notify_any_source_message = _mpc_lowcomm_ofi_msg_notify_anysource;
	rail->send_message_from_network = sctk_send_message_from_network_ofi_msg;
	rail->driver_finalize           = sctk_network_finalize_ofi_msg;
	rail->control_message_handler   = sctk_network_ofi_control_message_handler;

	/* RDMA */
	rail->rail_pin_region        = mpc_lowcomm_ofi_msg_pin_region;
	rail->rail_unpin_region      = mpc_lowcomm_ofi_msg_unpin_region;
	rail->rdma_write             = mpc_lowcomm_ofi_msg_write;
	rail->rdma_read              = mpc_lowcomm_ofi_msg_read;
	rail->rdma_fetch_and_op_gate = mpc_lowcomm_ofi_msg_fetch_and_op_gate;
	rail->rdma_fetch_and_op      = NULL;
	rail->rdma_cas_gate          = mpc_lowcomm_ofi_msg_cas_gate;
	rail->rdma_cas               = NULL;

	__register_in_monitor(rail);

	/* generic initialization */
	sctk_rail_init_route ( rail, rail->runtime_config_rail->topology, mpc_lowcomm_ofi_msg_on_demand_handler );

	/* prepare the lookup table */
	ctx_ep_table = sctk_calloc(mpc_common_get_process_count(), sizeof(mpc_lowcomm_ofi_ep_t));

	/* find compatible providers */
	struct fi_info* hint = fi_allocinfo();
	mpc_lowcomm_ofi_setup_hints_from_config(hint, rail->runtime_config_driver_config->driver.value.ofi);
	
	/* some parameters forced by the current rail */
	hint->caps = FI_MSG | FI_RMA;
	hint->ep_attr->type = FI_EP_MSG;
	hint->mode = FI_CONTEXT;
	mpc_lowcomm_ofi_init_provider(orail, hint);

	/* if no provider abort().
	 * The search should be inverted, where the driver would be adapted to existing provider contraints
	 */
	if(!orail->provider)
	{
		mpc_common_debug_fatal("Can't find a provider with given specs !");
	}
	fi_freeinfo(hint);

	__sctk_network_ofi_bootstrap(rail);
	rail->network_name = (char*)sctk_malloc(sizeof(char) * 64);
	snprintf(rail->network_name, 64, "Provider: %s (%s)", orail->provider->fabric_attr->prov_name, orail->provider->fabric_attr->name);
}
