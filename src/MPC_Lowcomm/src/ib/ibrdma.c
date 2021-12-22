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

#include "ib.h"
#include "ibrdma.h"
#include "ibeager.h"
#include "ibtopology.h"
#include "ibpolling.h"
#include "ibufs.h"
#include "ibmmu.h"
#include "sctk_net_tools.h"
#include "cp.h"

#include <sctk_alloc.h>


/* IB debug macros */
#if defined MPC_LOWCOMM_IB_MODULE_NAME
#error "MPC_LOWCOMM_IB_MODULE already defined"
#endif
//#define MPC_LOWCOMM_IB_MODULE_DEBUG
#define MPC_LOWCOMM_IB_MODULE_NAME    "RDMA"
#include "ibtoolkit.h"
#include "sctk_checksum.h"

/*-----------------------------------------------------------
*  FUNCTIONS
*----------------------------------------------------------*/

static sctk_ib_header_rdma_t *recv_rdma_headers      = NULL;
static mpc_common_spinlock_t  recv_rdma_headers_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static OPA_int_t recv_rdma_headers_nb = OPA_INT_T_INITIALIZER(0);

static inline void _mpc_lowcomm_ib_rdma_align_msg(void *addr, size_t size,
                                                  void **aligned_addr, size_t *aligned_size)
{
	/* We do not need to align pointers by hand */

	*aligned_addr = addr;
	*aligned_size = size;
}

/*-----------------------------------------------------------
*  RDMA COMPLETION
*----------------------------------------------------------*/


mpc_lowcomm_ptp_message_t *_mpc_lowcomm_ib_rdma_recv_done_remote_imm(__UNUSED__ sctk_rail_info_t *rail, int imm_data)
{
	sctk_ib_header_rdma_t *    rdma = NULL;
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;

	/* Save the values of the ack because the buffer will be reused */
	mpc_common_spinlock_lock(&recv_rdma_headers_lock);
	HASH_FIND(hh, recv_rdma_headers, &imm_data, sizeof(int), rdma);
	mpc_common_spinlock_unlock(&recv_rdma_headers_lock);
	assume(rdma);



	mpc_lowcomm_perform_idle( ( int * )&rdma->local.ready, 1,
	                          (void (*)(void *) )_mpc_lowcomm_multirail_notify_idle, NULL);

	send = rdma->copy_ptr->msg_send;
	recv = rdma->copy_ptr->msg_recv;


	/* If MPC_LOWCOMM_IB_RDMA_RECOPY, we delete the temporary msg copy */
	mpc_common_nodebug("MSG DONE REMOTE");

	if(rdma->local.status == MPC_LOWCOMM_IB_RDMA_RECOPY)
	{
		sctk_net_message_copy_from_buffer(rdma->local.addr, rdma->copy_ptr,
		                                  0);

		mpc_common_nodebug("FREE: %p", rdma->local.addr);
		/* If we MPC_LOWCOMM_IB_RDMA_RECOPY, we can delete the temp buffer */
		sctk_free(rdma->local.addr);
	}

	_mpc_comm_ptp_message_commit_request(send, recv);

	return NULL;
}

/*
 * RECV DONE REMOTE
 */
static inline mpc_lowcomm_ptp_message_t *__rdma_recv_done_remote(__UNUSED__ sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	/* Save the values of the ack because the buffer will be reused */
	_mpc_lowcomm_ib_rdma_t *     rdma_msg  = (_mpc_lowcomm_ib_rdma_t *)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_done_t *rdma_done = &rdma_msg->done;

	mpc_lowcomm_ptp_message_t *dest_msg_header = rdma_done->dest_msg_header;
	sctk_ib_header_rdma_t *    rdma            = &dest_msg_header->tail.ib.rdma;


	mpc_lowcomm_perform_idle(
		(int *)&dest_msg_header->tail.ib.rdma.local.ready, 1,
		(void (*)(void *) )_mpc_lowcomm_multirail_notify_idle, NULL);

	mpc_lowcomm_ptp_message_t *send = rdma->copy_ptr->msg_send;
	mpc_lowcomm_ptp_message_t *recv = rdma->copy_ptr->msg_recv;

	mpc_common_nodebug("msg: %p - Rail: %p (%p-%p) copy_ptr:%p (send:%p recv:%p)",
	                   dest_msg_header, rail, ibuf, rdma_done, rdma->copy_ptr, send,
	                   recv);

	/* If MPC_LOWCOMM_IB_RDMA_RECOPY, we delete the temporary msg copy */
	mpc_common_nodebug("MSG DONE REMOTE %p ", dest_msg_header);

	if(dest_msg_header->tail.ib.rdma.local.status == MPC_LOWCOMM_IB_RDMA_RECOPY)
	{
		sctk_net_message_copy_from_buffer(dest_msg_header->tail.ib.rdma.local.addr,
		                                  dest_msg_header->tail.ib.rdma.copy_ptr,
		                                  0);

		mpc_common_nodebug("FREE: %p", dest_msg_header->tail.ib.rdma.local.addr);
		/* If we MPC_LOWCOMM_IB_RDMA_RECOPY, we can delete the temp buffer */
		sctk_free(dest_msg_header->tail.ib.rdma.local.addr);
	}

	_mpc_comm_ptp_message_commit_request(send, recv);

	return NULL;
}

/*
 * RECV DONE LOCAL
 */
static inline void __rdma_recv_done_local(mpc_lowcomm_ptp_message_t *msg)
{
	assume(msg->tail.ib.rdma.local.mmu_entry);
	_mpc_lowcomm_ib_mmu_relax(msg->tail.ib.rdma.local.mmu_entry);

	if(msg->tail.ib.rdma.local.status == MPC_LOWCOMM_IB_RDMA_RECOPY)
	{
		/* Unregister MMU and free message */
		mpc_common_nodebug("FREE PTR: %p", msg->tail.ib.rdma.local.addr);
		sctk_free(msg->tail.ib.rdma.local.addr);
	}

	mpc_common_nodebug("MSG LOCAL FREE %p", msg);
	mpc_lowcomm_ptp_message_complete_and_free(msg);
}

/*-----------------------------------------------------------
*  FREE RECV MESSAGE
*----------------------------------------------------------*/
void _mpc_lowcomm_ib_rdma_net_free_recv(void *arg)
{
	mpc_lowcomm_ptp_message_t *msg  = ( mpc_lowcomm_ptp_message_t * )arg;
	sctk_ib_header_rdma_t *    rdma = &msg->tail.ib.rdma;


	/* Unregister MMU and free message */
	mpc_common_nodebug("Free MMu for msg: %p", msg);
	assume(rdma->local.mmu_entry);
	_mpc_lowcomm_ib_mmu_relax(rdma->local.mmu_entry);
	mpc_common_nodebug("FREE: %p", msg);
	mpc_common_spinlock_lock(&recv_rdma_headers_lock);
	HASH_DELETE(hh, recv_rdma_headers, rdma);
	mpc_common_nodebug("REM msg %p with key %d", rdma, rdma->ht_key);
	mpc_common_spinlock_unlock(&recv_rdma_headers_lock);
	sctk_free(msg);
}

/*-----------------------------------------------------------
*  PREPARE RECV
*----------------------------------------------------------*/


static void _mpc_lowcomm_ib_rdma_prepare_recv_zerocopy(mpc_lowcomm_ptp_message_t *msg)
{
	sctk_ib_msg_header_t *send_header;

	send_header = &msg->tail.ib;
	_mpc_lowcomm_ib_rdma_align_msg(send_header->rdma.local.addr,
	                               send_header->rdma.local.size,
	                               &send_header->rdma.local.aligned_addr,
	                               &send_header->rdma.local.aligned_size);
}

static void _mpc_lowcomm_ib_rdma_prepare_recv_recopy(mpc_lowcomm_ptp_message_t *msg)
{
	sctk_ib_msg_header_t *send_header;
	size_t page_size;

	page_size   = getpagesize();
	send_header = &msg->tail.ib;

	/* Allocating memory according to the requested size */
	sctk_posix_memalign( ( void ** )&send_header->rdma.local.aligned_addr,
	                     page_size, send_header->rdma.requested_size);

	send_header->rdma.local.aligned_size = send_header->rdma.requested_size;
	send_header->rdma.local.size         = send_header->rdma.requested_size;
	send_header->rdma.local.addr         = send_header->rdma.local.aligned_addr;

	mpc_common_nodebug("Allocating memory (size:%lu,ptr:%p, msg:%p)",
	                   send_header->rdma.requested_size,
	                   send_header->rdma.local.aligned_addr, msg);
}

/*-----------------------------------------------------------
*  RENDEZ VOUS PROTOCOL
*----------------------------------------------------------*/

/*
 * Communication Overview:
 * =======================================================================================
 * P0                                                        P1
 *
 * _mpc_lowcomm_ib_rdma_rendezvous_prepare_req ---------------->
 * _mpc_lowcomm_ib_rdma_rendezvous_recv_req
 |                                                                        |
 | v                                                                        |
 | _mpc_lowcomm_ib_rdma_rendezvous_prepare_send_msg                    WAIT FOR MATCHING
 | RECV
 |
 |
 |                                                                       v
 | _mpc_lowcomm_ib_rdma_rendezvous_recv_ack <-------------------
 | _mpc_lowcomm_ib_rdma_rendezvous_net_copy
 |
 | v
 | _mpc_lowcomm_ib_rdma_rendezvous_prepare_data_write ***** RDMA >>>>>>>>>>>>>>>>>>>>
 |
 | WRITE POLLED
 |
 | v
 | _mpc_lowcomm_ib_rdma_rendezvous_prepare_done_write -------->
 | __rdma_recv_done_remote
 |                                                                  |
 | v                                                                  v
 | __rdma_recv_done_local                             Complete and Free
 |
 | v
 | complete and free
 | =======================================================================================
 */

/* ACKS */

static inline _mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_rdma_rendezvous_prepare_ack(__UNUSED__ sctk_rail_info_t *rail,
                                                                                  mpc_lowcomm_ptp_message_t *msg)
{
	sctk_ib_header_rdma_t * rdma = &msg->tail.ib.rdma;

	size_t ibuf_size = sizeof(_mpc_lowcomm_ib_rdma_t);

	_mpc_lowcomm_ib_ibuf_t *ibuf = _mpc_lowcomm_ib_ibuf_pick_send(&rdma->remote_rail->network.ib, rdma->remote_peer, &ibuf_size);
	assume(ibuf);

	IBUF_SET_DEST_TASK(ibuf->buffer, msg->tail.ib.rdma.source_task);
	IBUF_SET_SRC_TASK(ibuf->buffer, msg->tail.ib.rdma.destination_task);

	_mpc_lowcomm_ib_rdma_t * rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t * rdma_header = &rdma_msg->rdma_header;
	_mpc_lowcomm_ib_rdma_ack_t *rdma_ack = &rdma_msg->ack;

	rdma_ack->addr            = rdma->local.addr;
	rdma_ack->size            = rdma->local.size;
	rdma_ack->rkey            = rdma->local.mmu_entry->mr->rkey;
	rdma_ack->dest_msg_header = rdma->remote.msg_header;
	rdma_ack->src_msg_header  = msg;
	rdma_ack->ht_key          = rdma->ht_key;

	/* Initialization of the buffer */
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_RDMA_PROTOCOL);

	rdma_header->type = MPC_LOWCOMM_IB_RDMA_RDV_ACK_TYPE;

	mpc_common_nodebug("Send RDMA ACK message: %lu %u", rdma_ack->addr, rdma_ack->rkey);
	return ibuf;
}

static void _mpc_lowcomm_ib_rdma_rendezvous_send_ack(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg)
{
	_mpc_lowcomm_ib_ibuf_t *ibuf;

	sctk_ib_qp_t *        remote;
	sctk_ib_msg_header_t *send_header;

	send_header = &msg->tail.ib;
	sctk_ib_header_rdma_t *rdma = &send_header->rdma;
	/* Register MMU */
	send_header->rdma.local.mmu_entry = _mpc_lowcomm_ib_mmu_pin(
		&rdma->remote_rail->network.ib, send_header->rdma.local.aligned_addr,
		send_header->rdma.local.aligned_size);
	mpc_common_nodebug("MMU registered for msg %p", send_header);

	ibuf = _mpc_lowcomm_ib_rdma_rendezvous_prepare_ack(rail, msg);

	/* Send message */
	remote = send_header->rdma.remote_peer;
	sctk_ib_qp_send_ibuf(&rdma->remote_rail->network.ib, remote, ibuf);
	mpc_common_nodebug("Send ACK to rail %d for task %d", rdma->remote_rail->rail_number, SCTK_MSG_SRC_TASK(msg) );

	send_header->rdma.local.send_ack_timestamp = mpc_arch_get_timestamp();
}

_mpc_lowcomm_ib_ibuf_t *_mpc_lowcomm_ib_rdma_rendezvous_prepare_req(sctk_rail_info_t *rail,
                                                                    sctk_ib_qp_t *remote,
                                                                    mpc_lowcomm_ptp_message_t *msg,
                                                                    size_t size)
{
	LOAD_RAIL(rail);
	sctk_ib_header_rdma_t *     rdma = &msg->tail.ib.rdma;
	_mpc_lowcomm_ib_ibuf_t *    ibuf;

	size_t ibuf_size = sizeof(_mpc_lowcomm_ib_rdma_t);

	ibuf = _mpc_lowcomm_ib_ibuf_pick_send(rail_ib, remote, &ibuf_size);
	assume(ibuf);
	IBUF_SET_DEST_TASK(ibuf->buffer, SCTK_MSG_DEST_TASK(msg) );
	IBUF_SET_SRC_TASK(ibuf->buffer, SCTK_MSG_SRC_TASK(msg) );

	_mpc_lowcomm_ib_rdma_t *rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t * rdma_header = &rdma_msg->rdma_header;
	rdma_header->type = MPC_LOWCOMM_IB_RDMA_RDV_REQ_TYPE;

	/* Get the buffer */
	_mpc_lowcomm_ib_rdma_req_t *rdma_req = &rdma_msg->req;

	/* Initialization of the msg header */
	memcpy(&rdma_req->msg_header, msg, sizeof(mpc_lowcomm_ptp_message_body_t) );
	/* Message not ready: memory not pinned */
	rdma->local.ready         = 0;
	rdma->rail                = rail;
	rdma->remote_peer         = remote;
	rdma->local.req_timestamp = mpc_arch_get_timestamp();

	rdma->remote_rail     = rail;
	rdma_req->remote_rail = rdma->rail->rail_number;

	/* Initialization of the request */
	rdma_req->requested_size  = size - sizeof(mpc_lowcomm_ptp_message_body_t);
	rdma_req->dest_msg_header = msg;
	/* Register the type of the message */
	rdma_req->message_type = msg->tail.message_type;
	mpc_common_nodebug("Send message on rail %d (%d) and reply on rail %d (%d)",
	                   rail->rail_number, SCTK_MSG_DEST_TASK(msg),
	                   rdma_req->remote_rail, SCTK_MSG_SRC_TASK(msg) );

	/* Initialization of the buffer */
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_RDMA_PROTOCOL);

	mpc_common_nodebug("Request size: %d", sizeof(_mpc_lowcomm_ib_rdma_t));
	mpc_common_nodebug("Req sent (size:%lu, requested:%d, ibuf:%p)",
	                   sizeof(_mpc_lowcomm_ib_rdma_t), rdma_req->requested_size, ibuf);

	return ibuf;
}

void _mpc_lowcomm_ib_rdma_rendezvous_net_copy(mpc_lowcomm_ptp_message_content_to_copy_t *tmp)
{
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;
	sctk_ib_msg_header_t *     send_header;

	send        = tmp->msg_send;
	recv        = tmp->msg_recv;
	send_header = &send->tail.ib;

	/* The buffer has been posted and if the message is contiguous,
	 * we can initiate a RDMA data transfert */
	mpc_common_spinlock_lock(&send_header->rdma.lock);

	/* If the message has not yet been handled */
	if(send_header->rdma.local.status == MPC_LOWCOMM_IB_RDMA_NOT_SET)
	{
		if(recv->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
		{
			/* XXX: Check if the size requested is the size of the message posted */
//      assume(send_header->rdma.requested_size == recv->tail.message.contiguous.size);

			send_header->rdma.local.addr   = recv->tail.message.contiguous.addr;
			send_header->rdma.local.size   = recv->tail.message.contiguous.size;
			send_header->rdma.local.status = MPC_LOWCOMM_IB_RDMA_ZEROCOPY;
			mpc_common_nodebug("Zerocopy. (Send:%p Recv:%p)", send, recv);
			_mpc_lowcomm_ib_rdma_prepare_recv_zerocopy(send);
			_mpc_lowcomm_ib_rdma_rendezvous_send_ack(send_header->rdma.rail, send);
			send_header->rdma.copy_ptr    = tmp;
			send_header->rdma.local.ready = 1;
		}
		else
		/* not contiguous message */
		{
			send_header->rdma.local.status = MPC_LOWCOMM_IB_RDMA_RECOPY;
			_mpc_lowcomm_ib_rdma_prepare_recv_recopy(send);
			mpc_common_nodebug("Msg %p recopied from buffer %p", tmp->msg_send, send_header->rdma.local.addr);
			send_header->rdma.copy_ptr = tmp;
			_mpc_lowcomm_ib_rdma_rendezvous_send_ack(send_header->rdma.rail, send);
			send_header->rdma.local.ready = 1;
		}

		mpc_common_nodebug("Copy_ptr: %p (free:%p, ptr:%p)", tmp, send->tail.free_memory,
		                   send_header->rdma.local.addr);
	}
	else
	if(send_header->rdma.local.status == MPC_LOWCOMM_IB_RDMA_RECOPY)
	{
		send_header->rdma.copy_ptr    = tmp;
		send_header->rdma.local.ready = 1;
	}
	else
	{
		not_reachable();
	}

	mpc_common_spinlock_unlock(&send_header->rdma.lock);
}

static inline mpc_lowcomm_ptp_message_t *
_mpc_lowcomm_ib_rdma_rendezvous_recv_req(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	sctk_ib_header_rdma_t *     rdma;
	mpc_lowcomm_ptp_message_t * msg;

	_mpc_lowcomm_ib_rdma_t * rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_req_t *rdma_req = &rdma_msg->req;

	msg = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t) );
	memcpy(&msg->body, &rdma_req->msg_header,
	       sizeof(mpc_lowcomm_ptp_message_body_t) );

	/* We reinit the header before calculating the source */
	_mpc_comm_ptp_message_clear_request(msg);
	_mpc_comm_ptp_message_set_copy_and_free(msg, _mpc_lowcomm_ib_rdma_net_free_recv,
	                                        _mpc_lowcomm_ib_rdma_rendezvous_net_copy);
	msg->tail.ib.protocol = MPC_LOWCOMM_IB_RDMA_PROTOCOL;
	rdma = &msg->tail.ib.rdma;
	rdma->local.req_timestamp = mpc_arch_get_timestamp();

	SCTK_MSG_COMPLETION_FLAG_SET(msg, NULL);
	msg->tail.message_type = MPC_LOWCOMM_MESSAGE_NETWORK;
	/* Remote addr initially set to NULL */
	mpc_common_spinlock_init(&rdma->lock, 0);
	rdma->local.status   = MPC_LOWCOMM_IB_RDMA_NOT_SET;
	rdma->requested_size = rdma_req->requested_size;

	{
		/* Computing remote rail */
		int remote_rail_nb = rdma_req->remote_rail;

		assume(remote_rail_nb < sctk_rail_count() );

		rdma->remote_rail = sctk_rail_get_by_id(remote_rail_nb);

		/* Get the remote QP from the remote rail */
		
	    /* Determine the Source process */
	    int integer_src_task = msg->body.header.source_task;
	
        mpc_lowcomm_communicator_id_t comm_id = SCTK_MSG_COMMUNICATOR_ID(msg);
        mpc_lowcomm_communicator_t comm = mpc_lowcomm_get_communicator_from_id(comm_id);
        mpc_lowcomm_peer_uid_t src_process = mpc_lowcomm_communicator_uid(comm, integer_src_task);
        
        _mpc_lowcomm_endpoint_t *route;
		src_process = msg->body.header.source;
		assume(src_process != -1);
		route = sctk_rail_get_any_route_to_process_or_on_demand(rdma->remote_rail,
		                                                        src_process);
		assume(route);

		mpc_common_nodebug("Got message on rail %d and reply on rail %d",
		                   rail->rail_number, rdma->remote_rail->rail_number);
		/* Update the remote peer */
		rdma->remote_peer = route->data.ib.remote;
	}
	rdma->rail              = rail;
	rdma->local.addr        = NULL;
	rdma->remote.msg_header = rdma_req->dest_msg_header;
	rdma->local.msg_header  = msg;
	rdma->local.ready       = 0;
	rdma->ht_key            = OPA_fetch_and_incr_int(&recv_rdma_headers_nb);

	mpc_common_spinlock_lock(&recv_rdma_headers_lock);
	HASH_ADD(hh, recv_rdma_headers, ht_key, sizeof(int), rdma);
	mpc_common_nodebug("ADD msg %p with key %d", rdma, rdma->ht_key);
	mpc_common_spinlock_unlock(&recv_rdma_headers_lock);

	/* XXX: Only for collaborative polling */
	rdma->source_task      = IBUF_GET_SRC_TASK(ibuf->buffer);
	rdma->destination_task = IBUF_GET_DEST_TASK(ibuf->buffer);

	/* Send message to MPC. The message can be matched at the end
	 * of function. */
	rail->send_message_from_network(msg);

	mpc_common_nodebug("End send_message (matching:%p, stats:%d, "
	                   "requested_size:%lu, required_size:%lu)",
	                   rdma->local.addr, rdma->local.status, rdma_req->requested_size,
	                   rdma->local.size);

	return msg;
}

void _mpc_lowcomm_ib_rdma_rendezvous_prepare_send_msg(mpc_lowcomm_ptp_message_t *msg, size_t size)
{
	sctk_ib_header_rdma_t *rdma = &msg->tail.ib.rdma;
	void * aligned_addr         = NULL;
	size_t aligned_size         = 0;

	/* Do not allocate memory if contiguous message */
	if(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
	{
		_mpc_lowcomm_ib_rdma_align_msg(msg->tail.message.contiguous.addr,
		                               msg->tail.message.contiguous.size,
		                               &aligned_addr, &aligned_size);

		rdma->local.addr   = msg->tail.message.contiguous.addr;
		rdma->local.size   = msg->tail.message.contiguous.size;
		rdma->local.status = MPC_LOWCOMM_IB_RDMA_ZEROCOPY;
	}
	else
	{
		size_t page_size;
		aligned_size = size - sizeof(mpc_lowcomm_ptp_message_body_t);
		page_size    = getpagesize();

		sctk_posix_memalign( ( void ** )&aligned_addr, page_size, aligned_size);
		sctk_net_copy_in_buffer(msg, aligned_addr);

		rdma->local.addr   = aligned_addr;
		rdma->local.size   = aligned_size;
		rdma->local.status = MPC_LOWCOMM_IB_RDMA_RECOPY;
	}

	/* Register MMU */
	rdma->local.mmu_entry = _mpc_lowcomm_ib_mmu_pin(&rdma->remote_rail->network.ib, aligned_addr, aligned_size);

	/* Save addr and size */
	rdma->local.aligned_addr = aligned_addr;
	rdma->local.aligned_size = aligned_size;
	rdma->source_task        = SCTK_MSG_SRC_TASK(msg);
	rdma->destination_task   = SCTK_MSG_DEST_TASK(msg);
	rdma->local.msg_header   = msg;
	/* Message is now ready ready */
	rdma->local.ready = 1;
}

static inline mpc_lowcomm_ptp_message_t *
_mpc_lowcomm_ib_rdma_rendezvous_recv_ack(__UNUSED__ sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	mpc_lowcomm_ptp_message_t * dest_msg_header;
	mpc_lowcomm_ptp_message_t * src_msg_header;
	sctk_ib_header_rdma_t *     rdma;

	/* Save the values of the ack because the buffer will be reused */
	_mpc_lowcomm_ib_rdma_t * rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_ack_t *rdma_ack = &rdma_msg->ack;

	src_msg_header  = rdma_ack->dest_msg_header;
	rdma            = &src_msg_header->tail.ib.rdma;
	dest_msg_header = rdma_ack->src_msg_header;



	/* Wait while the message becomes ready */
	mpc_lowcomm_perform_idle(
		(int *)&rdma->local.ready, 1,
		(void (*)(void *) )_mpc_lowcomm_multirail_notify_idle, NULL);

	mpc_common_nodebug("Remote addr: %p", rdma_ack->addr);
	rdma->remote.addr       = rdma_ack->addr;
	rdma->remote.size       = rdma_ack->size;
	rdma->remote.rkey       = rdma_ack->rkey;
	rdma->remote.msg_header = dest_msg_header;
	rdma->ht_key            = rdma_ack->ht_key;

	return src_msg_header;
}

void _mpc_lowcomm_ib_rdma_rendezvous_prepare_data_write(
	__UNUSED__ sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *src_msg_header)
{
	sctk_ib_header_rdma_t *            rdma = &src_msg_header->tail.ib.rdma;
	_mpc_lowcomm_ib_ibuf_t *           ibuf;

	_mpc_lowcomm_endpoint_t *route;


    mpc_lowcomm_peer_uid_t remote_peer =  rdma->remote_peer->rank;


	route = sctk_rail_get_any_route_to_process_or_on_demand(
		rdma->remote_rail, remote_peer);
        

	ibuf = _mpc_lowcomm_ib_ibuf_pick_send_sr(&rdma->remote_rail->network.ib);
	assume(ibuf);


	_mpc_lowcomm_ib_rdma_t *rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t * rdma_header = &rdma_msg->rdma_header;
	_mpc_lowcomm_ib_rdma_data_write_t *rdma_data_write = &rdma_msg->write;
	rdma_data_write->src_msg_header = src_msg_header;

	IBUF_SET_DEST_TASK(ibuf->buffer, rdma->destination_task);
	IBUF_SET_SRC_TASK(ibuf->buffer, rdma->source_task);
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_RDMA_PROTOCOL);

	rdma_header->type = MPC_LOWCOMM_IB_RDMA_RDV_WRITE_TYPE;

	mpc_common_nodebug("Write from %p (%lu) to %p (%lu)", rdma->local.addr,
	                   rdma->local.size, rdma->remote.addr, rdma->remote.size);

	_mpc_lowcomm_ib_ibuf_write_init(ibuf, rdma->local.addr,
	                                rdma->local.mmu_entry->mr->lkey, rdma->remote.addr,
	                                rdma->remote.rkey, rdma->local.size,
	                                IBV_SEND_SIGNALED, IBUF_RELEASE);

	sctk_ib_qp_send_ibuf(&rdma->remote_rail->network.ib, route->data.ib.remote,
	                     ibuf);

	/*
	 #endif
	 */
	rdma->local.send_rdma_timestamp = mpc_arch_get_timestamp();
}

static inline mpc_lowcomm_ptp_message_t *
_mpc_lowcomm_ib_rdma_rendezvous_prepare_done_write(sctk_rail_info_t *rail,
                                                   _mpc_lowcomm_ib_ibuf_t *incoming_ibuf)
{
	LOAD_RAIL(rail);
	size_t ibuf_size = sizeof(_mpc_lowcomm_ib_rdma_t);

	_mpc_lowcomm_ib_rdma_t *incoming_rdma_msg = (_mpc_lowcomm_ib_rdma_t*)incoming_ibuf->buffer;

	_mpc_lowcomm_ib_rdma_data_write_t * rdma_data_write = &incoming_rdma_msg->write;

	mpc_lowcomm_ptp_message_t *src_msg_header  = rdma_data_write->src_msg_header;
	sctk_ib_header_rdma_t *rdma            = &src_msg_header->tail.ib.rdma;
	mpc_lowcomm_ptp_message_t *dest_msg_header = rdma->remote.msg_header;

#if 1
	/* Initialize & send the buffer */

	/* FIXME: we can remove it now as we send the done message using imm_data.
	 * At least we should provide an option for fallbacking to the previous mode */

	_mpc_lowcomm_ib_ibuf_t * ibuf        = _mpc_lowcomm_ib_ibuf_pick_send(rail_ib, rdma->remote_peer, &ibuf_size);
	assume(ibuf);

	_mpc_lowcomm_ib_rdma_t *rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t * rdma_header = &rdma_msg->rdma_header;
	_mpc_lowcomm_ib_rdma_done_t *rdma_done = &rdma_msg->done;

	rdma_done->dest_msg_header = dest_msg_header;
	rdma_header->type = MPC_LOWCOMM_IB_RDMA_RDV_DONE_TYPE;

	IBUF_SET_DEST_TASK(ibuf->buffer, rdma->destination_task);
	IBUF_SET_SRC_TASK(ibuf->buffer, rdma->source_task);
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_RDMA_PROTOCOL);

	sctk_ib_qp_send_ibuf(rail_ib, rdma->remote_peer, ibuf);
#endif

	return src_msg_header;
}

/*-----------------------------------------------------------
*  RDMA ATOMIC OPERATIONS GATES
*----------------------------------------------------------*/

int _mpc_lowcomm_ib_rdma_fetch_and_op_gate(__UNUSED__ sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type)
{
	/* IB only supports 64 bits operands */
	if(size != 8)
	{
		return 0;
	}

	/* IB only has fetch and ADD */
	if(op != RDMA_SUM)
	{
		return 0;
	}

	if(type == RDMA_TYPE_LONG || type == RDMA_TYPE_LONG_LONG ||
	   type == RDMA_TYPE_LONG_LONG_INT ||
	   type == RDMA_TYPE_UNSIGNED_LONG ||
	   type == RDMA_TYPE_UNSIGNED_LONG_LONG || type == RDMA_TYPE_WCHAR)
	{
		/* Addition is only valid on integers */
		return 1;
	}
	else
	{
		return 0;
	}
}

int _mpc_lowcomm_ib_rdma_cas_gate(__UNUSED__ sctk_rail_info_t *rail, size_t size, __UNUSED__ RDMA_type type)
{
	/* IB only supports 64 bits operands */
	if(size != 8)
	{
		return 0;
	}

	return 1;
}

/*-----------------------------------------------------------
*  RDMA READ & WRITE
*----------------------------------------------------------*/

mpc_lowcomm_ptp_message_t *_mpc_lowcomm_ib_rdma_retrieve_msg_ptr(_mpc_lowcomm_ib_ibuf_t *ibuf)
{
	_mpc_lowcomm_ib_rdma_t *rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t *rdma_header = &rdma_msg->rdma_header;

	return rdma_header->msg_pointer;
}

void _mpc_lowcomm_ib_rdma_write(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
                                void *src_addr, struct sctk_rail_pin_ctx_list *local_key,
                                void *dest_addr, struct sctk_rail_pin_ctx_list *remote_key,
                                size_t size)
{
	LOAD_RAIL(rail);


	_mpc_lowcomm_endpoint_t *route = sctk_rail_get_any_route_to_process_or_on_demand(rail, SCTK_MSG_DEST_PROCESS(msg) );

	_mpc_lowcomm_ib_ibuf_t *ibuf = _mpc_lowcomm_ib_ibuf_pick_send_sr(rail_ib);
	assume(ibuf);

	_mpc_lowcomm_ib_rdma_t *rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t *rdma_header = &rdma_msg->rdma_header;

	IBUF_SET_DEST_TASK(ibuf->buffer, SCTK_MSG_DEST_TASK(msg) );
	IBUF_SET_SRC_TASK(ibuf->buffer, SCTK_MSG_SRC_TASK(msg) );
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_RDMA_PROTOCOL);

	rdma_header->type = MPC_LOWCOMM_IB_RDMA_WRITE;
	rdma_header->payload_size = size;
	rdma_header->msg_pointer = msg;

	mpc_common_debug("RDMA Write from %p (%lu) to %p (%lu)", src_addr, size,
	                 dest_addr, size);

	_mpc_lowcomm_ib_ibuf_write_init(ibuf, src_addr, local_key->pin.ib.mr.lkey,
	                                dest_addr, remote_key->pin.ib.mr.rkey, size,
	                                IBV_SEND_SIGNALED, IBUF_RELEASE);

	sctk_ib_qp_send_ibuf(&rail->network.ib, route->data.ib.remote, ibuf);
}

void _mpc_lowcomm_ib_rdma_read(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
                               void *src_addr, struct  sctk_rail_pin_ctx_list *remote_key,
                               void *dest_addr, struct  sctk_rail_pin_ctx_list *local_key,
                               size_t size)
{
	LOAD_RAIL(rail);


	_mpc_lowcomm_endpoint_t *route =
		sctk_rail_get_any_route_to_process_or_on_demand(
			rail, SCTK_MSG_DEST_PROCESS(msg) );

	_mpc_lowcomm_ib_ibuf_t *ibuf = _mpc_lowcomm_ib_ibuf_pick_send_sr(rail_ib);
	assume(ibuf);

	_mpc_lowcomm_ib_rdma_t *rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t *rdma_header = &rdma_msg->rdma_header;

	IBUF_SET_DEST_TASK(ibuf->buffer, SCTK_MSG_DEST_TASK(msg) );
	IBUF_SET_SRC_TASK(ibuf->buffer, SCTK_MSG_SRC_TASK(msg) );
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_RDMA_PROTOCOL);

	rdma_header->type = MPC_LOWCOMM_IB_RDMA_READ;
	rdma_header->payload_size = size;
	rdma_header->msg_pointer = msg;

	mpc_common_debug("RDMA Read from %p (%lu) to %p (%lu)", src_addr, size,
	                 dest_addr, size);

	_mpc_lowcomm_ib_ibuf_read_init(ibuf, dest_addr, local_key->pin.ib.mr.rkey,
	                               src_addr, remote_key->pin.ib.mr.lkey, size);

	sctk_ib_qp_send_ibuf(&rail->network.ib, route->data.ib.remote, ibuf);
}

void _mpc_lowcomm_ib_rdma_fetch_and_op(sctk_rail_info_t *rail,
                                       mpc_lowcomm_ptp_message_t *msg, void *fetch_addr,
                                       struct sctk_rail_pin_ctx_list *local_key,
                                       void *remote_addr,
                                       struct sctk_rail_pin_ctx_list *remote_key,
                                       void *add, RDMA_op op, RDMA_type type)
{
	LOAD_RAIL(rail);

	if(op != RDMA_SUM)
	{
		mpc_common_debug_fatal("Infiniband only supports the SUM operand");
	}

	if(RDMA_type_size(type) != 8)
	{
		mpc_common_debug_fatal(
			"This implementation only supports 64bits operands for Fetch and Op");
	}


	_mpc_lowcomm_endpoint_t *route = sctk_rail_get_any_route_to_process_or_on_demand(
		rail, SCTK_MSG_DEST_PROCESS(msg) );

	_mpc_lowcomm_ib_ibuf_t *ibuf = _mpc_lowcomm_ib_ibuf_pick_send_sr(rail_ib);
	assume(ibuf);

	_mpc_lowcomm_ib_rdma_t *rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t *rdma_header = &rdma_msg->rdma_header;

	IBUF_SET_DEST_TASK(ibuf->buffer, SCTK_MSG_DEST_TASK(msg) );
	IBUF_SET_SRC_TASK(ibuf->buffer, SCTK_MSG_SRC_TASK(msg) );
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_RDMA_PROTOCOL);

	rdma_header->type = MPC_LOWCOMM_IB_RDMA_FETCH_AND_ADD;
	rdma_header->msg_pointer = msg;

	mpc_common_debug("RDMA Fetch and op from %p to %p add %lld", remote_addr, fetch_addr,
	                 add);

	uint64_t local_add = 0;
	memcpy(&local_add, add, sizeof(uint64_t) );

	_mpc_lowcomm_ib_ibuf_fetch_and_add_init(ibuf, fetch_addr, local_key->pin.ib.mr.lkey,
	                                        remote_addr, remote_key->pin.ib.mr.rkey,
	                                        local_add);

	sctk_ib_qp_send_ibuf(&rail->network.ib, route->data.ib.remote, ibuf);
}

void _mpc_lowcomm_ib_rdma_cas(sctk_rail_info_t *rail,
                              mpc_lowcomm_ptp_message_t *msg,
                              void *res_addr,
                              struct  sctk_rail_pin_ctx_list *local_key,
                              void *remote_addr,
                              struct  sctk_rail_pin_ctx_list *remote_key,
                              void *comp,
                              void *new,
                              RDMA_type type)
{
	LOAD_RAIL(rail);

	if(RDMA_type_size(type) != 8)
	{
		mpc_common_debug_fatal(
			"This implementation only supports 64bits operands for CAS");
	}


	_mpc_lowcomm_endpoint_t *route =
		sctk_rail_get_any_route_to_process_or_on_demand(
			rail, SCTK_MSG_DEST_PROCESS(msg) );

	_mpc_lowcomm_ib_ibuf_t *ibuf = _mpc_lowcomm_ib_ibuf_pick_send_sr(rail_ib);
	assume(ibuf);

	_mpc_lowcomm_ib_rdma_t *rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t *rdma_header = &rdma_msg->rdma_header;

	IBUF_SET_DEST_TASK(ibuf->buffer, SCTK_MSG_DEST_TASK(msg) );
	IBUF_SET_SRC_TASK(ibuf->buffer, SCTK_MSG_SRC_TASK(msg) );
	IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_RDMA_PROTOCOL);

	rdma_header->type = MPC_LOWCOMM_IB_RDMA_CAS;
	rdma_header->msg_pointer = msg;

	uint64_t local_comp = 0;
	memcpy(&local_comp, comp, sizeof(uint64_t) );

	uint64_t local_new = 0;
	memcpy(&local_new, new, sizeof(uint64_t) );

	mpc_common_debug("RDMA CASS to %p to comp %lld new %lld", remote_addr,
	                 local_comp, local_new);

	_mpc_lowcomm_ib_ibuf_compare_and_swap_init(ibuf, res_addr, local_key->pin.ib.mr.lkey,
	                                           remote_addr, remote_key->pin.ib.mr.rkey,
	                                           local_comp, local_new);

	sctk_ib_qp_send_ibuf(&rail->network.ib, route->data.ib.remote, ibuf);
}

/*-----------------------------------------------------------
*  POLLING SEND / RECV
*----------------------------------------------------------*/

int _mpc_lowcomm_ib_rdma_poll_recv(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{

	mpc_lowcomm_ptp_message_t *header;

	IBUF_CHECK_POISON(ibuf->buffer);


	_mpc_lowcomm_ib_rdma_t * rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t * rdma_header = &rdma_msg->rdma_header;

	/* Switch on the RDMA type of message */
	switch(rdma_header->type)
	{
		case MPC_LOWCOMM_IB_RDMA_RDV_REQ_TYPE:
		{
			mpc_common_nodebug("Poll recv: message RDMA req received");
			_mpc_lowcomm_ib_rdma_rendezvous_recv_req(rail, ibuf);
			return 1;
		}
		break;

		case MPC_LOWCOMM_IB_RDMA_RDV_ACK_TYPE:
		{
			mpc_common_nodebug("Poll recv: message RDMA ack received");
			/* Buffer reused: don't need to free it */
			header = _mpc_lowcomm_ib_rdma_rendezvous_recv_ack(rail, ibuf);
			_mpc_lowcomm_ib_rdma_rendezvous_prepare_data_write(rail, header);
			return 1;
		}
		break;

		case MPC_LOWCOMM_IB_RDMA_RDV_DONE_TYPE:
		{
			mpc_common_nodebug("Poll recv: message RDMA done received");
			__rdma_recv_done_remote(rail, ibuf);
			return 1;
		}
		break;

		default:
			mpc_common_debug_error("RDMA type %d not found!!!", rdma_header->type);
			not_reachable();
	}

	not_reachable();
	return 0;
}

int
_mpc_lowcomm_ib_rdma_poll_send(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	/* If the buffer *MUST* be released */
	int release_ibuf = 1;
	mpc_lowcomm_ptp_message_t *msg;

	IBUF_CHECK_POISON(ibuf->buffer);

	_mpc_lowcomm_ib_rdma_t * rdma_msg = (_mpc_lowcomm_ib_rdma_t*)ibuf->buffer;
	_mpc_lowcomm_ib_rdma_header_t *rdma_header = &rdma_msg->rdma_header;

	/* Switch on the RDMA type of message */
	switch(rdma_header->type )
	{
		case MPC_LOWCOMM_IB_RDMA_RDV_REQ_TYPE:
			mpc_common_nodebug("Poll send: message RDMA req received");
			break;

		case MPC_LOWCOMM_IB_RDMA_RDV_ACK_TYPE:
			mpc_common_nodebug("Poll send: message RDMA ack received");
			break;

		case MPC_LOWCOMM_IB_RDMA_RDV_WRITE_TYPE:
			mpc_common_nodebug("Poll send: message RDMA data write received");
			msg = _mpc_lowcomm_ib_rdma_rendezvous_prepare_done_write(rail, ibuf);
			__rdma_recv_done_local(msg);
			break;

		case MPC_LOWCOMM_IB_RDMA_RDV_DONE_TYPE:
			mpc_common_nodebug("Poll send: message RDMA done received");
			break;


		case MPC_LOWCOMM_IB_RDMA_WRITE:
			msg = _mpc_lowcomm_ib_rdma_retrieve_msg_ptr(ibuf);
			mpc_common_debug("RDMA Write DONE");
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			break;


		case MPC_LOWCOMM_IB_RDMA_READ:
			msg = _mpc_lowcomm_ib_rdma_retrieve_msg_ptr(ibuf);
			mpc_common_debug("RDMA Read DONE");
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			break;

		case MPC_LOWCOMM_IB_RDMA_FETCH_AND_ADD:
			msg = _mpc_lowcomm_ib_rdma_retrieve_msg_ptr(ibuf);
			mpc_common_debug("RDMA Fetch and op DONE");
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			break;

		case MPC_LOWCOMM_IB_RDMA_CAS:
			msg = _mpc_lowcomm_ib_rdma_retrieve_msg_ptr(ibuf);
			mpc_common_debug("RDMA CAS DONE");
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			break;

		default:
			mpc_common_debug_error("Got RDMA type:%d, payload_size;%lu",
			                       rdma_header->type,
			                       rdma_header->payload_size);
			break;
			char ibuf_desc[4096];
			mpc_common_debug_error("BEGIN ERROR");
			_mpc_lowcomm_ib_ibuf_print(ibuf, ibuf_desc);
			mpc_common_debug_error("\nIB - FATAL ERROR FROM PROCESS %d\n"
			                       "######### IBUF DESC ############\n"
			                       "%s\n"
			                       "################################",
			                       mpc_common_get_process_rank(), ibuf_desc);
			mpc_common_debug_error("END ERROR");
			//      not_reachable();
	}

	return release_ibuf;
}

/*-----------------------------------------------------------
*  PRINT
*----------------------------------------------------------*/
void _mpc_lowcomm_ib_rdma_print(mpc_lowcomm_ptp_message_t *msg)
{
	sctk_ib_msg_header_t *h = &msg->tail.ib;

	mpc_common_debug_error("MSG INFOS\n"
	                       "requested_size: %d\n"
	                       "rail: %p\n"
	                       "remote_peer: %p\n"
	                       "copy_ptr: %p\n"
	                       "--- LOCAL ---\n"
	                       "status: %d\n"
	                       "mmu_entry: %p\n"
	                       "addr: %p\n"
	                       "size: %lu\n"
	                       "aligned_addr: %p\n"
	                       "aligned_size: %lu\n"
	                       "ready: %d\n"
	                       "--- REMOTE ---\n"
	                       "msg_header: %p\n"
	                       "addr: %p\n"
	                       "size: %lu\n"
	                       "rkey: %u\n"
	                       "aligned_addr: %p\n"
	                       "aligned_size: %lu\n",
	                       h->rdma.requested_size,
	                       h->rdma.rail,
	                       h->rdma.remote_peer,
	                       h->rdma.copy_ptr,
	                       h->rdma.local.status,
	                       h->rdma.local.mmu_entry,
	                       h->rdma.local.addr,
	                       h->rdma.local.size,
	                       h->rdma.local.aligned_addr,
	                       h->rdma.local.aligned_size,
	                       h->rdma.local.ready,
	                       h->rdma.remote.msg_header,
	                       h->rdma.remote.addr,
	                       h->rdma.remote.size,
	                       h->rdma.remote.rkey,
	                       h->rdma.remote.aligned_addr,
	                       h->rdma.remote.aligned_size);
}
