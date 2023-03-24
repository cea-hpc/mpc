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

#include "ofi_toolkit.h"
#include "ofi_rdma_types.h"
#include <rail.h>

#include <sctk_net_tools.h>
#include <mpc_common_spinlock.h>
#include <mpc_common_datastructure.h>
#include <sctk_alloc.h>
#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>

char * ofi_req_type_decoder[] = {
	"MPC_LOWCOMM_OFI_TYPE_EAGER",
	"MPC_LOWCOMM_OFI_TYPE_RDV_READY",
	"MPC_LOWCOMM_OFI_TYPE_RDV_READ",
	"MPC_LOWCOMM_OFI_TYPE_RDV_ACK",
	"MPC_LOWCOMM_OFI_TYPE_RDMA",
	"MPC_LOWCOMM_OFI_TYPE_NB"
};

struct mpc_common_hashtable __rank_av_map;

static inline fi_addr_t __mpc_lowcomm_ofi_rdma_get_peer(mpc_lowcomm_peer_uid_t rank)
{
	fi_addr_t* ret = (fi_addr_t *)mpc_common_hashtable_get(&__rank_av_map, rank);
	assume(ret != NULL);
	assert(*ret != FI_ADDR_UNSPEC);
	return *ret;
}

static inline void __mpc_lowcomm_ofi_rdma_set_peer(mpc_lowcomm_peer_uid_t rank, fi_addr_t addr)
{
	fi_addr_t * new = sctk_malloc(sizeof(fi_addr_t));
	assume(new != NULL);
	*new = addr;
	mpc_common_hashtable_set(&__rank_av_map, rank, new);
}

static inline void __mpc_lowcomm_ofi_rdv_message_copy(mpc_lowcomm_ptp_message_content_to_copy_t* copy)
{
	mpc_lowcomm_ofi_rail_info_t* orail = copy->msg_send->tail.ofi.ref_rail;
	struct fid_ep* ep = orail->spec.rdma.sep;
	size_t msg_size = SCTK_MSG_SIZE(copy->msg_send);
	fi_addr_t remote;
	void* addr = NULL;
	
	remote = __mpc_lowcomm_ofi_rdma_get_peer(SCTK_MSG_SRC_PROCESS(copy->msg_send));
	
	switch(copy->msg_recv->tail.message_type)
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		case MPC_LOWCOMM_MESSAGE_NETWORK:
			addr = copy->msg_recv->tail.message.contiguous.addr;
			break;
		default:
			addr = sctk_malloc(msg_size);
			break;
	}
	copy->msg_send->tail.ofi.rdv_complete = 0;

	struct fid_mr* mr;
	MPC_LOWCOMM_OFI_CHECK_RC(fi_mr_reg(orail->domain, addr, msg_size, FI_WRITE, 0, 0, 0, &mr, NULL));

	mpc_lowcomm_ofi_rdma_ctx_req_t ctx = (mpc_lowcomm_ofi_rdma_ctx_req_t)
	{
		.type = MPC_LOWCOMM_OFI_TYPE_RDV_READ,
		.data.custom.opaque = copy->msg_send
	};
	mpc_common_nodebug("fi_read %lu bytes from %d with addr = %p and key = %lu for msg = %p", msg_size, remote, copy->msg_send->tail.ofi.addr, copy->msg_send->tail.ofi.mr_key, copy->msg_send);

	MPC_LOWCOMM_OFI_CHECK_RC(fi_read(
		ep,
		addr,
		msg_size,
		fi_mr_desc(mr), /* DESC */
		remote,
		copy->msg_send->tail.ofi.addr,
		copy->msg_send->tail.ofi.mr_key,
		&ctx
	));

	int temp = 0;
	while(!copy->msg_send->tail.ofi.rdv_complete)   
	{
		mpc_thread_yield();
	}

	switch(copy->msg_recv->tail.message_type)
	{
		case MPC_LOWCOMM_MESSAGE_CONTIGUOUS:
		case MPC_LOWCOMM_MESSAGE_NETWORK:
			/* direct copy */
			break;
		default:
			/* recopy */
			not_implemented();
			break;
	}

	/* TODO: Send ACK to remote */
}


static inline mpc_lowcomm_ofi_ep_t* __mpc_lowcomm_ofi_rdma_register_endpoint(sctk_rail_info_t* rail, mpc_lowcomm_peer_uid_t remote, _mpc_lowcomm_endpoint_type_t origin, char* infos)
{
	mpc_lowcomm_ofi_ep_t* ctx = sctk_malloc(sizeof(mpc_lowcomm_ofi_ep_t));
	*ctx = (mpc_lowcomm_ofi_ep_t)
	{
		.ep = rail->network.ofi.spec.rdma.sep,
		.cq_r = rail->network.ofi.spec.rdma.cq_recv,
		.cq_s = rail->network.ofi.spec.rdma.cq_send,
		.lock = MPC_COMMON_SPINLOCK_INITIALIZER,
	};
	
	MPC_LOWCOMM_OFI_CHECK_RC(fi_av_insert(rail->network.ofi.spec.rdma.av, infos, 1, &ctx->spec.rdma.av_root, 0, NULL));
	__mpc_lowcomm_ofi_rdma_set_peer(remote, ctx->spec.rdma.av_root);

	ctx->sctk_ep = mpc_lowcomm_ofi_add_route(remote, ctx, rail, origin, _MPC_LOWCOMM_ENDPOINT_CONNECTED);
	return ctx;
}

static inline void __mpc_lowcomm_ofi_rdma_post_recv(mpc_lowcomm_ofi_rail_info_t* orail, mpc_lowcomm_ofi_rdma_ctx_req_t* input_req)
{
	struct fid_ep* ep = NULL;
	void* payload = NULL, *hdr = NULL;
	int nb_reqs = 1, i;
	if(!input_req)
	{
		input_req = sctk_malloc(sizeof(mpc_lowcomm_ofi_rdma_ctx_req_t) * MPC_LOWCOMM_OFI_REQ_NB);
		nb_reqs = MPC_LOWCOMM_OFI_REQ_NB;
	}
	
	ep = orail->spec.rdma.sep;

	for(i = 0;  i < nb_reqs; i++)
	{
		mpc_lowcomm_ofi_rdma_ctx_req_t* req = input_req + i; /* = input_req when provided */

		hdr = &req->data.eager.hdr[0];
		payload = &req->data.eager.buffer[0];
		req->type = -1;
		
		struct iovec iov[2];
		iov[0] = (struct iovec) {.iov_base = hdr, .iov_len = sizeof(mpc_lowcomm_ptp_message_body_t)};
		iov[1] = (struct iovec) {.iov_base = payload, .iov_len = MPC_LOWCOMM_OFI_EAGER_SIZE};
		struct fi_msg msg_v;
		
		msg_v = (struct fi_msg) {
			.msg_iov = iov,
			.desc = NULL,
			.iov_count = 2,
			.addr = FI_ADDR_UNSPEC,
			.context = req,
			.data = 0
		};
		
		MPC_LOWCOMM_OFI_CHECK_RC(fi_recvmsg(
			ep,
			&msg_v,
			FI_COMPLETION
		));
	}
}

static inline void __mpc_lowcomm_ofi_rdma_init(sctk_rail_info_t* rail)
{
	mpc_lowcomm_ofi_rail_info_t* orail = &rail->network.ofi;
	mpc_lowcomm_ofi_rail_spec_rdma_t* rdorail = &orail->spec.rdma;
	size_t i;

	size_t nb_processes = mpc_common_get_process_count();

	assume(sizeof(fi_addr_t) == sizeof(void *));
	mpc_common_hashtable_init(&__rank_av_map, nb_processes);

	for (i = 0; i < nb_processes; i++)
	{
		__mpc_lowcomm_ofi_rdma_set_peer(i, FI_ADDR_UNSPEC);
	}

	struct fi_av_attr av_attr = (struct fi_av_attr){
		.type = FI_AV_MAP,            /* type of AV */
		.rx_ctx_bits = 0,             /* address bits to identify rx ctx (only two per group) */
		.count = nb_processes,        /* # entries for AV */
		.ep_per_node = 0,             /* # endpoints per fabric address */
		.name = NULL,                 /* system name of AV */
		.map_addr = NULL,             /* base mmap address */
		.flags = 0                    /* operation flags */
	};

	struct fi_cq_attr cq_attr  =  (struct fi_cq_attr) {
		.size = MPC_LOWCOMM_OFI_CQ_SIZE,       /* EQ size */
		.flags = 0,                     /* spacial case */
		.format = FI_CQ_FORMAT_CONTEXT, /* format of the CQ event returned */
		.wait_obj = FI_WAIT_NONE,       /* Waiting procedure */
		.signaling_vector = 0,          /* range of cores to signal when incoming event */
		.wait_cond = FI_CQ_COND_NONE,   /* Condition on CQ */
		.wait_set = NULL                /* wait set to wait on */
	};

	MPC_LOWCOMM_OFI_CHECK_RC(fi_cq_open(orail->domain, &cq_attr, &rdorail->cq_recv, NULL));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_cq_open(orail->domain, &cq_attr, &rdorail->cq_send, NULL));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_av_open(orail->domain, &av_attr, &rdorail->av, NULL));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_endpoint(orail->domain, orail->provider, &rdorail->sep, NULL));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_ep_bind(rdorail->sep, &rdorail->av->fid, 0));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_ep_bind(rdorail->sep, &rdorail->cq_recv->fid, FI_RECV));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_ep_bind(rdorail->sep, &rdorail->cq_send->fid, FI_TRANSMIT));
	MPC_LOWCOMM_OFI_CHECK_RC(fi_enable(rdorail->sep));
	__mpc_lowcomm_ofi_rdma_post_recv(orail, NULL);
}

static inline void __sctk_network_ofi_rdma_bootstrap(sctk_rail_info_t* rail)
{
	mpc_lowcomm_ofi_rail_info_t* orail = &rail->network.ofi;
	orail->name_info_sz = FI_NAME_MAX;

	__mpc_lowcomm_ofi_rdma_init(rail);

	/* retrieve the ID for the current process */
	MPC_LOWCOMM_OFI_CHECK_RC(fi_getname(&orail->spec.rdma.sep->fid, orail->name_info, &orail->name_info_sz));


#if 0
	unsigned char raw_connection_infos[MPC_COMMON_MAX_STRING_SIZE], *connection_infos;
	int right_rank, left_rank, my_rank, nb_ranks;
	int need_right, need_left;
	
	my_rank = mpc_common_get_process_rank();
	nb_ranks = mpc_common_get_process_count();
	need_right = my_rank == 0 || nb_ranks >2;
	need_left = my_rank == 1 || nb_ranks > 2;
	right_rank = ( my_rank + 1 ) % nb_ranks;
	left_rank = ( my_rank - 1 + nb_ranks ) % nb_ranks;


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
	if(need_right)
	{
		assume ( mpc_launch_pmi_get_as_rank ((char *)raw_connection_infos, MPC_COMMON_MAX_STRING_SIZE, rail->rail_number, right_rank ) == 0 );
		connection_infos = mpc_common_datastructure_base64_decode((unsigned char*)raw_connection_infos, strlen((char*)raw_connection_infos), NULL);
		__mpc_lowcomm_ofi_rdma_register_endpoint(rail, right_rank, _MPC_LOWCOMM_ENDPOINT_STATIC, (char*)connection_infos);
		sctk_free(connection_infos);
	}
	if(need_left)
	{
		assume ( mpc_launch_pmi_get_as_rank ((char *)raw_connection_infos, MPC_COMMON_MAX_STRING_SIZE, rail->rail_number, left_rank ) == 0 );
		connection_infos = mpc_common_datastructure_base64_decode((unsigned char*)raw_connection_infos, strlen((char*)raw_connection_infos), NULL);
		__mpc_lowcomm_ofi_rdma_register_endpoint(rail, left_rank, _MPC_LOWCOMM_ENDPOINT_STATIC, (char*)connection_infos);
		sctk_free(connection_infos);
	}

	mpc_launch_pmi_barrier();
#endif
}

/**
 * Entry point for sending a message with the TCP network.
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
static void _mpc_lowcomm_ofirdma_send_message ( mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint )
{
	mpc_lowcomm_ofi_rail_info_t* orail = &endpoint->rail->network.ofi;
	fi_addr_t remote = FI_ADDR_UNSPEC;
	struct fid_ep* target_ep = NULL;
	size_t msg_size = SCTK_MSG_SIZE(msg);
	void* addr = NULL;
	struct fid_mr* mr = NULL;
	uint64_t content[2];
	mpc_lowcomm_ofi_rdma_ctx_req_t* req = sctk_malloc(sizeof(mpc_lowcomm_ofi_rdma_ctx_req_t));

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

	target_ep = orail->spec.rdma.sep;
	remote = __mpc_lowcomm_ofi_rdma_get_peer(endpoint->dest);
	req->data.custom.opaque = (void*)msg;
	

	if(MPC_LOWCOMM_OFI_EAGER_SIZE >= msg_size)
	{
		req->type = MPC_LOWCOMM_OFI_TYPE_EAGER;
	}
	else /* RDV */
	{
		req->type = MPC_LOWCOMM_OFI_TYPE_RDV_READY;
		fi_mr_reg(orail->domain, addr, msg_size, FI_READ, 0, MPC_LOWCOMM_OFI_RDV_KEY, 0, &mr, msg );
		content[0] = (uint64_t) fi_mr_key(mr);
		content[1] = (uint64_t) addr;
		addr = content;
		msg_size = sizeof(uint64_t) * 2;
	}
	
	struct iovec iov[2];
	iov[0] = (struct iovec) {.iov_base = &msg->body, .iov_len = sizeof(mpc_lowcomm_ptp_message_body_t)};
	iov[1] = (struct iovec) {.iov_base = addr, .iov_len = msg_size};
	struct fi_msg msg_v = (struct fi_msg) {
		.msg_iov = iov,
		.desc = NULL,
		.iov_count = 2,
		.addr = remote,
		.context = req,
		.data = 0
	};
	MPC_LOWCOMM_OFI_CHECK_RC(fi_sendmsg(target_ep, &msg_v, FI_COMPLETION ));
}

static inline void __mpc_lowcomm_ofi_progress_recv(sctk_rail_info_t* rail)
{
	mpc_lowcomm_ptp_message_t* msg = NULL;
	fi_addr_t src_addr = 0;
	struct fi_cq_entry entry;
	void(*copy_fn)(mpc_lowcomm_ptp_message_content_to_copy_t*) = NULL;
	int ret = 0;

	//MPC_LOWCOMM_OFI_CHECK_RC((ret = fi_cq_readfrom(rail->network.ofi.spec.rdma.cq_recv, &entry, 1, &src_addr)));
	MPC_LOWCOMM_OFI_CHECK_RC((ret = fi_cq_read(rail->network.ofi.spec.rdma.cq_recv, &entry, 1)));
	assert(ret > 0 || ret == -FI_EAGAIN);
	if(ret > 0)
	{
		mpc_lowcomm_ofi_rdma_ctx_req_t* req = (mpc_lowcomm_ofi_rdma_ctx_req_t*)entry.op_context;
		mpc_lowcomm_ptp_message_body_t* hdr_msg = NULL;
		hdr_msg = (mpc_lowcomm_ptp_message_body_t*)(&req->data.eager.hdr[0]);
		msg = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t) + hdr_msg->header.msg_size);
		memcpy(&msg->body, hdr_msg, sizeof(mpc_lowcomm_ptp_message_body_t));

		if(MPC_LOWCOMM_OFI_EAGER_SIZE >= hdr_msg->header.msg_size)
		{				
			assert(msg);
			memcpy((char*)msg + sizeof(mpc_lowcomm_ptp_message_t), &req->data.eager.buffer[0], hdr_msg->header.msg_size);
			copy_fn = sctk_net_message_copy;
			__mpc_lowcomm_ofi_rdma_post_recv(&rail->network.ofi, req);
		}
		else
		{
			assert(src_addr != FI_ADDR_NOTAVAIL);
			uint64_t key, addr;
			memcpy(&key, req->data.rdv.buffer,  sizeof(key));
			memcpy(&addr, req->data.rdv.buffer + sizeof(key), sizeof(addr));
			assert(msg);
			mpc_common_nodebug("Nothing to do... wait for copy from %p (addr = %p key = %lu)", src_addr, addr, key);
			copy_fn = __mpc_lowcomm_ofi_rdv_message_copy;
			msg->tail.ofi = (mpc_lowcomm_ofi_tail_t) {
				.ref_rail = &rail->network.ofi,
				.remote = src_addr,
				.addr = addr,
				.mr_key = key,
				.rdv_complete = 0
			};
			__mpc_lowcomm_ofi_rdma_post_recv(&rail->network.ofi, req);
		}

		/* complete the recved message and forward it to upper layers */
		SCTK_MSG_COMPLETION_FLAG_SET(msg, NULL);
		msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;
		_mpc_comm_ptp_message_clear_request(msg);
		_mpc_comm_ptp_message_set_copy_and_free(msg, sctk_free, copy_fn);
		rail->send_message_from_network(msg);
	}
}

static inline void __mpc_lowcomm_ofi_progress_send(sctk_rail_info_t* rail)
{
	mpc_lowcomm_ptp_message_t* msg = NULL;
	struct fi_cq_entry entry;
	int ret = 0;

	MPC_LOWCOMM_OFI_CHECK_RC((ret = fi_cq_read(rail->network.ofi.spec.rdma.cq_send, &entry, 1)));
	assert(ret > 0 || ret == -FI_EAGAIN);
	if(ret > 0)
	{
		mpc_lowcomm_ofi_rdma_ctx_req_t* req = (mpc_lowcomm_ofi_rdma_ctx_req_t*)entry.op_context;
		switch(req->type)
		{
		case MPC_LOWCOMM_OFI_TYPE_EAGER:
			msg = (mpc_lowcomm_ptp_message_t*)req->data.custom.opaque;
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			break;
		case MPC_LOWCOMM_OFI_TYPE_RDV_READY: /* nothing to do, wait for the ACK */
			
			break;
		case MPC_LOWCOMM_OFI_TYPE_RDV_READ:
			msg = (mpc_lowcomm_ptp_message_t*)req->data.custom.opaque;
			msg->tail.ofi.rdv_complete = 1;
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			break;
		case MPC_LOWCOMM_OFI_TYPE_RDV_ACK:
			break;
		default:
			not_reachable();
			break;
		}
	}
}

/**
 * Called by idle threads to progress messages.
 * This is not the purpose of this function for TCP, as a polling thread is created for each new route.
 * \param[in] msg
 * \param[in] rail
 */
static void _mpc_lowcomm_ofirdma_notify_idle(struct sctk_rail_info_s* rail)
{
	int val = 0;
	__mpc_lowcomm_ofi_progress_recv(rail);

	if((val++ % 1000) == 0)
		__mpc_lowcomm_ofi_progress_send(rail);
}

/**
 * Handler triggering the send_message_from_network call, before reaching the inter_thread_comm matching process.
 * \param[in] msg the message received from the network, to be matched w/ a local RECV.
 */
static inline int sctk_send_message_from_network_ofi_rdma ( mpc_lowcomm_ptp_message_t *msg )
{
	if ( _mpc_lowcomm_reorder_msg_check ( msg ) == _MPC_LOWCOMM_REORDER_NO_NUMBERING )
	{
		/* No reordering */
		_mpc_comm_ptp_message_send_check ( msg, 1 );
	}

	return 1;
}

void sctk_network_finalize_ofi_rdma(sctk_rail_info_t *rail)
{
	mpc_common_hashtable_release(&__rank_av_map);
	sctk_free(rail->network_name);
}


static char * __ofi_rdma_rail_name(sctk_rail_info_t *rail, char * buff, int bufflen)
{
	snprintf(buff, bufflen, "ofi_rdma_%d", rail->rail_number);
	return buff;
}


void mpc_lowcomm_ofi_rdma_on_demand_handler( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest_process )
{
	unsigned char *connection_infos;

	char rail_name[32];
	mpc_lowcomm_monitor_retcode_t ret = MPC_LOWCOMM_MONITOR_RET_SUCCESS;

	mpc_lowcomm_monitor_response_t resp = mpc_lowcomm_monitor_ondemand(dest_process,
																	   __ofi_rdma_rail_name(rail, rail_name, 32),
																	   NULL,
																	   0,
																	   &ret);

	if(ret != MPC_LOWCOMM_MONITOR_RET_SUCCESS)
	{
		mpc_common_debug_fatal("Could not connect to UID %lu", dest_process);
	}

	mpc_lowcomm_monitor_args_t *content = mpc_lowcomm_monitor_response_get_content(resp);

	connection_infos = mpc_common_datastructure_base64_decode(content->on_demand.data, strlen(content->on_demand.data), NULL);
	__mpc_lowcomm_ofi_rdma_register_endpoint(rail, dest_process, _MPC_LOWCOMM_ENDPOINT_DYNAMIC, (char*)connection_infos);

	mpc_lowcomm_monitor_response_free(resp);
}

static int __ofi_rdma_monitor_callback(mpc_lowcomm_peer_uid_t from,
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

static inline void __rdma_register_in_monitor(sctk_rail_info_t *rail)
{
	char rail_name[32];
	__ofi_rdma_rail_name(rail, rail_name, 32);

	mpc_lowcomm_monitor_register_on_demand_callback(rail_name, __ofi_rdma_monitor_callback, rail);
}

void sctk_network_init_ofi_rdma( sctk_rail_info_t *rail )
{
	mpc_lowcomm_ofi_rail_info_t* orail = &rail->network.ofi;
	/* Register Hooks in rail */
	rail->send_message_endpoint     = _mpc_lowcomm_ofirdma_send_message;
	rail->notify_recv_message       = NULL;
	rail->notify_matching_message   = NULL;
	rail->notify_perform_message    = NULL;
	rail->notify_idle_message       = _mpc_lowcomm_ofirdma_notify_idle;
	rail->notify_any_source_message = NULL;
	rail->send_message_from_network = sctk_send_message_from_network_ofi_rdma;
	rail->driver_finalize           = sctk_network_finalize_ofi_rdma;

	__rdma_register_in_monitor(rail);

	struct fi_info* hint = fi_allocinfo();

	mpc_lowcomm_ofi_setup_hints_from_config(hint, rail->runtime_config_driver_config->driver.value.ofi);

	/* This implementation requires two-sided, one sided (RMA) and target notification */
	hint->caps = FI_MSG | FI_READ | FI_WRITE | FI_SOURCE ;
	hint->mode = FI_CONTEXT;
	hint->domain_attr->av_type = FI_AV_MAP;
	//hint->domain_attr->mr_mode = FI_MR_SCALABLE;
	
	mpc_lowcomm_ofi_init_provider(orail, hint);

	/* for the implementation of this driver, we decided to rely on specific capabilities
	 * like scalable endpoint. Instead of crashing in case a compatible provider cannot
	 * be found, the code should ajust itself to become compatible. For instance,
	 * the support of a unique Tx/Rx context should be possible
	 */
	if(!orail->provider)
	{
		mpc_common_debug_fatal("Can't find a provider with given specs !");
	}

	/* just store a properly formatted string to describe this rail */
	rail->network_name = (char*)sctk_malloc(sizeof(char) * 64);
	snprintf(rail->network_name, 64, "Provider: %s (%s)", orail->provider->fabric_attr->prov_name, orail->provider->fabric_attr->name);

	__sctk_network_ofi_rdma_bootstrap(rail);
	fi_freeinfo(hint);
}
