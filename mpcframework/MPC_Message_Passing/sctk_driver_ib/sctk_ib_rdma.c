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

#ifdef MPC_USE_INFINIBAND
#include "sctk_ib.h"
#include "sctk_ib_rdma.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_topology.h"
#include "sctk_ib_polling.h"
#include "sctk_ibufs.h"
#include "sctk_ib_mmu.h"
#include "sctk_net_tools.h"
#include "sctk_ib_cp.h"
#include "sctk_ib_prof.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
//#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "RDMA"
#include "sctk_ib_toolkit.h"
#include "sctk_checksum.h"

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
#define IBUF_GET_RDMA_TYPE(x) ((x)->type)
#define IBUF_SET_RDMA_TYPE(x, __type) ((x)->type = __type)
#define IBUF_GET_RDMA_PLSIZE(x) ((x)->payload_size)
#define IBUF_SET_RDMA_PLSIZE(x, __size) ((x)->payload_size = __size)
#define IBUF_GET_RDMA_POINTER(x) ((x)->msg_pointer)
#define IBUF_SET_RDMA_POINTER(x, __msg_pointer) ((x)->msg_pointer = __msg_pointer)

sctk_ib_header_rdma_t *recv_rdma_headers = NULL;
sctk_spinlock_t recv_rdma_headers_lock = SCTK_SPINLOCK_INITIALIZER;
OPA_int_t recv_rdma_headers_nb = OPA_INT_T_INITIALIZER ( 0 );

static inline void sctk_ib_rdma_align_msg ( void *addr, size_t  size,
                                            void **aligned_addr, size_t *aligned_size )
{
	/* We do not need to align pointers by hand */

	*aligned_addr = addr;
	*aligned_size = size;
}

 /*-----------------------------------------------------------
 *  RDMA COMPLETION
 *----------------------------------------------------------*/


sctk_thread_ptp_message_t * sctk_ib_rdma_recv_done_remote_imm ( sctk_rail_info_t *rail, int imm_data )
{
	sctk_ib_header_rdma_t *rdma = NULL;
	sctk_thread_ptp_message_t *send;
	sctk_thread_ptp_message_t *recv;

	/* Save the values of the ack because the buffer will be reused */
	sctk_spinlock_lock ( &recv_rdma_headers_lock );
	HASH_FIND ( hh, recv_rdma_headers, &imm_data, sizeof ( int ), rdma );
	sctk_spinlock_unlock ( &recv_rdma_headers_lock );
	assume ( rdma );

	struct sctk_ib_polling_s poll;
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	SCTK_PROFIL_START ( ib_rdma_idle );
	sctk_inter_thread_perform_idle ( ( int * ) &rdma->local.ready, 1,
	                                 ( void ( * ) ( void * ) ) sctk_network_notify_idle_message, NULL );
	SCTK_PROFIL_END ( ib_rdma_idle );

	send = rdma->copy_ptr->msg_send;
	recv = rdma->copy_ptr->msg_recv;

	sctk_nodebug ( "msg: %p - Rail: %p (%p) copy_ptr:%p (send:%p recv:%p)", dest_msg_header, 
																			rail, ibuf, 
																			rdma->copy_ptr, 
																			send, recv );

	/* If SCTK_IB_RDMA_RECOPY, we delete the temporary msg copy */
	sctk_nodebug ( "MSG DONE REMOTE" );

	if ( rdma->local.status == SCTK_IB_RDMA_RECOPY )
	{
		sctk_nodebug ( "MSG with addr %p completed, SCTK_IB_RDMA_RECOPY to %p (checksum:%lu)", 
					   send->tail.ib.rdma.local.addr,
		               recv->tail.message.contiguous.addr,
		               sctk_checksum_buffer ( rdma->local.addr, rdma->copy_ptr->msg_recv ) );
		            
		sctk_net_message_copy_from_buffer ( rdma->local.addr,
		                                    rdma->copy_ptr , 0 );
		                                    
		sctk_nodebug ( "FREE: %p", rdma->local.addr );
		/* If we SCTK_IB_RDMA_RECOPY, we can delete the temp buffer */
		sctk_free ( rdma->local.addr );
		PROF_INC ( rail, ib_free_mem );
	}

	sctk_message_completion_and_free ( send, recv );

	return NULL;
}



/*
 * RECV DONE REMOTE
 */
static inline sctk_thread_ptp_message_t * sctk_ib_rdma_recv_done_remote ( sctk_rail_info_t *rail, 
																		   sctk_ibuf_t *ibuf )
{
	sctk_ib_rdma_done_t *rdma_done;
	sctk_ib_header_rdma_t *rdma;
	sctk_thread_ptp_message_t *send;
	sctk_thread_ptp_message_t *recv;
	sctk_thread_ptp_message_t *dest_msg_header;

	/* Save the values of the ack because the buffer will be reused */
	rdma_done = IBUF_GET_RDMA_DONE ( ibuf->buffer );
	dest_msg_header = rdma_done->dest_msg_header;
	rdma = &dest_msg_header->tail.ib.rdma;

	struct sctk_ib_polling_s poll;
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	SCTK_PROFIL_START ( ib_rdma_idle );
	sctk_inter_thread_perform_idle ( ( int * ) &dest_msg_header->tail.ib.rdma.local.ready, 1,
	                                 ( void ( * ) ( void * ) ) sctk_network_notify_idle_message, NULL );
	SCTK_PROFIL_END ( ib_rdma_idle );

	send = rdma->copy_ptr->msg_send;
	recv = rdma->copy_ptr->msg_recv;

	sctk_nodebug ( "msg: %p - Rail: %p (%p-%p) copy_ptr:%p (send:%p recv:%p)", dest_msg_header, rail, 
																			   ibuf, rdma_done, rdma->copy_ptr, 
																			   send, recv );

	/* If SCTK_IB_RDMA_RECOPY, we delete the temporary msg copy */
	sctk_nodebug ( "MSG DONE REMOTE %p ", dest_msg_header );

	if ( dest_msg_header->tail.ib.rdma.local.status == SCTK_IB_RDMA_RECOPY )
	{
		sctk_nodebug ( "MSG with addr %p completed, SCTK_IB_RDMA_RECOPY to %p (checksum:%lu)", 
					   send->tail.ib.rdma.local.addr,
		               recv->tail.message.contiguous.addr,
					   sctk_checksum_buffer ( dest_msg_header->tail.ib.rdma.local.addr, dest_msg_header->tail.ib.rdma.copy_ptr->msg_recv ) );
		
		sctk_net_message_copy_from_buffer ( dest_msg_header->tail.ib.rdma.local.addr,
		                                    dest_msg_header->tail.ib.rdma.copy_ptr , 0 );
		                          
		sctk_nodebug ( "FREE: %p", dest_msg_header->tail.ib.rdma.local.addr );
		/* If we SCTK_IB_RDMA_RECOPY, we can delete the temp buffer */
		sctk_free ( dest_msg_header->tail.ib.rdma.local.addr );
		PROF_INC ( rail, ib_free_mem );
	}

	sctk_message_completion_and_free ( send, recv );

	return NULL;
}

/*
 * RECV DONE LOCAL
 */
static inline void
sctk_ib_rdma_recv_done_local ( sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg )
{
	assume ( msg->tail.ib.rdma.local.mmu_entry );
	sctk_ib_mmu_relax ( msg->tail.ib.rdma.local.mmu_entry );

	if ( msg->tail.ib.rdma.local.status == SCTK_IB_RDMA_RECOPY )
	{
		/* Unregister MMU and free message */
		sctk_nodebug ( "FREE PTR: %p", msg->tail.ib.rdma.local.addr );
		sctk_free ( msg->tail.ib.rdma.local.addr );
		PROF_INC ( rail, ib_free_mem );
	}

	sctk_nodebug ( "MSG LOCAL FREE %p", msg );
	sctk_complete_and_free_message ( msg );

}

/*-----------------------------------------------------------
 *  FREE RECV MESSAGE
 *----------------------------------------------------------*/
void sctk_ib_rdma_net_free_recv ( void *arg )
{
	sctk_thread_ptp_message_t *msg = ( sctk_thread_ptp_message_t * ) arg;
	sctk_ib_header_rdma_t *rdma = &msg->tail.ib.rdma;
	sctk_rail_info_t *rail = rdma->rail;

	/* Unregister MMU and free message */
	sctk_nodebug ( "Free MMu for msg: %p", msg );
	assume ( rdma->local.mmu_entry );
	sctk_ib_mmu_relax ( rdma->local.mmu_entry );
	sctk_nodebug ( "FREE: %p", msg );
	sctk_spinlock_lock ( &recv_rdma_headers_lock );
	HASH_DELETE ( hh, recv_rdma_headers, rdma );
	sctk_nodebug ( "REM msg %p with key %d", rdma, rdma->ht_key );
	sctk_spinlock_unlock ( &recv_rdma_headers_lock );
	sctk_free ( msg );
	PROF_INC ( rail, ib_free_mem );
}


/*-----------------------------------------------------------
 *  PREPARE RECV
 *----------------------------------------------------------*/


static void sctk_ib_rdma_prepare_recv_zerocopy ( sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg )
{
	sctk_ib_msg_header_t *send_header;

	send_header = &msg->tail.ib;
	sctk_ib_rdma_align_msg ( send_header->rdma.local.addr,
	                         send_header->rdma.local.size,
	                         &send_header->rdma.local.aligned_addr,
	                         &send_header->rdma.local.aligned_size );

}

static void sctk_ib_rdma_prepare_recv_recopy ( sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg )
{
	sctk_ib_msg_header_t *send_header;
	size_t page_size;

	page_size = getpagesize();
	send_header = &msg->tail.ib;

	/* Allocating memory according to the requested size */
	posix_memalign ( ( void ** ) &send_header->rdma.local.aligned_addr,
	                 page_size, send_header->rdma.requested_size );
	PROF_INC ( rail, ib_alloc_mem );

	send_header->rdma.local.aligned_size  = send_header->rdma.requested_size;
	send_header->rdma.local.size          = send_header->rdma.requested_size;
	send_header->rdma.local.addr          = send_header->rdma.local.aligned_addr;

	sctk_nodebug ( "Allocating memory (size:%lu,ptr:%p, msg:%p)", send_header->rdma.requested_size, 
																  send_header->rdma.local.aligned_addr, msg );
}

/*-----------------------------------------------------------
 *  RENDEZ VOUS PROTOCOL
 *----------------------------------------------------------*/
 
 /*
 Communication Overview:
 =======================================================================================
 P0                                                        P1
 
 sctk_ib_rdma_rendezvous_prepare_req ----------------> sctk_ib_rdma_rendezvous_recv_req
 |                                                                        |                    
 v                                                                        |
 sctk_ib_rdma_rendezvous_prepare_send_msg                    WAIT FOR MATCHING RECV
                                                                          |
                                                                          |
                                                                          v
 sctk_ib_rdma_rendezvous_recv_ack <------------------- sctk_ib_rdma_rendezvous_net_copy
 |
 v
 sctk_ib_rdma_rendezvous_prepare_data_write ***** RDMA >>>>>>>>>>>>>>>>>>>>
 |
 WRITE POLLED
 |
 v
 sctk_ib_rdma_rendezvous_prepare_done_write --------> sctk_ib_rdma_recv_done_remote
 |                                                                  |
 v                                                                  v
 sctk_ib_rdma_recv_done_local                             Complete and Free
 |
 v
 complete and free
  =======================================================================================
 */
 
 /* ACKS */
 
static inline sctk_ibuf_t *sctk_ib_rdma_rendezvous_prepare_ack ( sctk_rail_info_t *rail,
                                                      sctk_thread_ptp_message_t *msg )
{
	sctk_ib_header_rdma_t *rdma = &msg->tail.ib.rdma;
	sctk_ibuf_t *ibuf;
	LOAD_RAIL ( rail );
	sctk_ib_rdma_t *rdma_header;
	sctk_ib_rdma_ack_t *rdma_ack;
	size_t ibuf_size = IBUF_GET_RDMA_ACK_SIZE;

	ibuf = sctk_ibuf_pick_send ( &rdma->remote_rail->network.ib, rdma->remote_peer, &ibuf_size );
	assume ( ibuf );

	IBUF_SET_DEST_TASK ( ibuf->buffer, msg->tail.ib.rdma.source_task );
	IBUF_SET_SRC_TASK ( ibuf->buffer, msg->tail.ib.rdma.destination_task );
	rdma_header = IBUF_GET_RDMA_HEADER ( ibuf->buffer );

	rdma_ack    = IBUF_GET_RDMA_ACK ( ibuf->buffer );

	rdma_ack->addr = rdma->local.addr;
	rdma_ack->size = rdma->local.size;
	rdma_ack->rkey = rdma->local.mmu_entry->mr->rkey;
	rdma_ack->dest_msg_header = rdma->remote.msg_header;
	rdma_ack->src_msg_header = msg;
	rdma_ack->ht_key = rdma->ht_key;

	/* Initialization of the buffer */
	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_RDMA_PROTOCOL );
	IBUF_SET_RDMA_TYPE ( rdma_header, SCTK_IB_RDMA_RDV_ACK_TYPE );

	sctk_nodebug ( "Send RDMA ACK message: %lu %u", rdma_ack->addr, rdma_ack->rkey );
	return ibuf;
}

static void sctk_ib_rdma_rendezvous_send_ack ( sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg )
{
	sctk_ibuf_t *ibuf;
	LOAD_RAIL ( rail );
	sctk_ib_qp_t *remote;
	sctk_ib_msg_header_t *send_header;

	send_header = &msg->tail.ib;
	sctk_ib_header_rdma_t *rdma = &send_header->rdma;
	/* Register MMU */
	PROF_TIME_START ( rail_ib->rail, recv_mmu_register );
	send_header->rdma.local.mmu_entry =  sctk_ib_mmu_pin (
	                                         &rdma->remote_rail->network.ib, send_header->rdma.local.aligned_addr,
	                                         send_header->rdma.local.aligned_size );
	PROF_TIME_END ( rail_ib->rail, recv_mmu_register );
	sctk_nodebug ( "MMU registered for msg %p", send_header );

	ibuf = sctk_ib_rdma_rendezvous_prepare_ack ( rail, msg );

	/* Send message */
	remote = send_header->rdma.remote_peer;
	sctk_ib_qp_send_ibuf ( &rdma->remote_rail->network.ib, remote, ibuf);
	sctk_nodebug ( "Send ACK to rail %d for task %d", rdma->remote_rail->rail_number, SCTK_MSG_SRC_TASK ( msg ) );

	send_header->rdma.local.send_ack_timestamp = sctk_ib_prof_get_time_stamp();
}

 
 
 sctk_ibuf_t *sctk_ib_rdma_rendezvous_prepare_req ( sctk_rail_info_t *rail,
													sctk_ib_qp_t *remote, 
													sctk_thread_ptp_message_t *msg, 
													size_t size, 
													int low_memory_mode )
{
	LOAD_RAIL ( rail );
	sctk_ib_header_rdma_t *rdma = &msg->tail.ib.rdma;
	sctk_ibuf_t *ibuf;
	sctk_ib_rdma_t *rdma_header;
	sctk_ib_rdma_req_t *rdma_req;
	size_t ibuf_size = IBUF_GET_RDMA_REQ_SIZE;

	ibuf = sctk_ibuf_pick_send ( rail_ib, remote, &ibuf_size );
	assume ( ibuf );
	IBUF_SET_DEST_TASK ( ibuf->buffer, SCTK_MSG_DEST_TASK ( msg ) );
	IBUF_SET_SRC_TASK ( ibuf->buffer, SCTK_MSG_SRC_TASK ( msg ) );

	rdma_header = IBUF_GET_RDMA_HEADER ( ibuf->buffer );
	IBUF_SET_RDMA_TYPE ( rdma_header, SCTK_IB_RDMA_RDV_REQ_TYPE );

	/* Get the buffer */
	rdma_req    = IBUF_GET_RDMA_REQ ( ibuf->buffer );

	/* Initialization of the msg header */
	memcpy ( &rdma_req->msg_header, msg, sizeof ( sctk_thread_ptp_message_body_t ) );
	/* Message not ready: memory not pinned */
	rdma->local.ready = 0;
	rdma->rail = rail;
	rdma->remote_peer = remote;
	rdma->local.req_timestamp = sctk_ib_prof_get_time_stamp();


	rdma->remote_rail     = rail;
	rdma_req->remote_rail = rdma->rail->rail_number;

	/* Initialization of the request */
	rdma_req->requested_size = size - sizeof ( sctk_thread_ptp_message_body_t );
	rdma_req->dest_msg_header = msg;
	/* Register the type of the message */
	rdma_req->message_type = msg->tail.message_type;
	sctk_nodebug ( "Send message on rail %d (%d) and reply on rail %d (%d)", rail->rail_number,
	               SCTK_MSG_DEST_TASK ( msg ), rdma_req->remote_rail, SCTK_MSG_SRC_TASK ( msg ) );

	/* Initialization of the buffer */
	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_RDMA_PROTOCOL );

	sctk_nodebug ( "Request size: %d", IBUF_GET_RDMA_REQ_SIZE );
	sctk_nodebug ( "Req sent (size:%lu, requested:%d, ibuf:%p)", IBUF_GET_RDMA_REQ_SIZE,
	               rdma_req->requested_size, ibuf );


	return ibuf;
}



void sctk_ib_rdma_rendezvous_net_copy ( sctk_message_to_copy_t *tmp )
{
	sctk_thread_ptp_message_t *send;
	sctk_thread_ptp_message_t *recv;
	sctk_ib_msg_header_t *send_header;

	send = tmp->msg_send;
	recv = tmp->msg_recv;
	send_header = &send->tail.ib;

	/* The buffer has been posted and if the message is contiguous,
	 * we can initiate a RDMA data transfert */
	sctk_spinlock_lock ( &send_header->rdma.lock );

	/* If the message has not yet been handled */
	if ( send_header->rdma.local.status == SCTK_IB_RDMA_NOT_SET )
	{

		if ( recv->tail.message_type == SCTK_MESSAGE_CONTIGUOUS )
		{

			/* XXX: Check if the size requested is the size of the message posted */
//      assume(send_header->rdma.requested_size == recv->tail.message.contiguous.size);

			send_header->rdma.local.addr  = recv->tail.message.contiguous.addr;
			send_header->rdma.local.size  = recv->tail.message.contiguous.size;
			send_header->rdma.local.status       = SCTK_IB_RDMA_ZEROCOPY;
			sctk_nodebug ( "Zerocopy. (Send:%p Recv:%p)", send, recv );
			sctk_ib_rdma_prepare_recv_zerocopy ( send_header->rdma.rail, send );
			sctk_ib_rdma_rendezvous_send_ack ( send_header->rdma.rail, send );
			send_header->rdma.copy_ptr = tmp;
			send_header->rdma.local.ready = 1;
		}
		else
			/* not contiguous message */
		{
			send_header->rdma.local.status       = SCTK_IB_RDMA_RECOPY;
			sctk_ib_rdma_prepare_recv_recopy ( send_header->rdma.rail, send );
			sctk_nodebug ( "Msg %p recopied from buffer %p", tmp->msg_send, send_header->rdma.local.addr );
			send_header->rdma.copy_ptr = tmp;
			sctk_ib_rdma_rendezvous_send_ack ( send_header->rdma.rail, send );
			send_header->rdma.local.ready = 1;
		}

		sctk_nodebug ( "Copy_ptr: %p (free:%p, ptr:%p)", tmp, send->tail.free_memory,
		               send_header->rdma.local.addr );

	}
	else
		if ( send_header->rdma.local.status == SCTK_IB_RDMA_RECOPY )
		{
			send_header->rdma.copy_ptr = tmp;
			send_header->rdma.local.ready = 1;
		}
		else
			not_reachable();

	sctk_spinlock_unlock ( &send_header->rdma.lock );
}

 
static inline sctk_thread_ptp_message_t * sctk_ib_rdma_rendezvous_recv_req ( sctk_rail_info_t *rail, 
																			  sctk_ibuf_t *ibuf )
{
	sctk_ib_header_rdma_t *rdma;
	sctk_thread_ptp_message_t *msg;
	sctk_ib_rdma_req_t *rdma_req = IBUF_GET_RDMA_REQ ( ibuf->buffer );

	msg = sctk_malloc ( sizeof ( sctk_thread_ptp_message_t ) );
	PROF_INC ( rail, ib_alloc_mem );
	memcpy ( &msg->body, &rdma_req->msg_header, sizeof ( sctk_thread_ptp_message_body_t ) );

	/* We reinit the header before calculating the source */
	sctk_rebuild_header ( msg );
	sctk_reinit_header ( msg, sctk_ib_rdma_net_free_recv, sctk_ib_rdma_rendezvous_net_copy );
	msg->tail.ib.protocol = SCTK_IB_RDMA_PROTOCOL;
	rdma = &msg->tail.ib.rdma;
	rdma->local.req_timestamp = sctk_ib_prof_get_time_stamp();

	SCTK_MSG_COMPLETION_FLAG_SET ( msg , NULL );
	msg->tail.message_type = SCTK_MESSAGE_NETWORK;
	/* Remote addr initially set to NULL */
	rdma->lock        = SCTK_SPINLOCK_INITIALIZER;
	rdma->local.status      = SCTK_IB_RDMA_NOT_SET;
	rdma->requested_size  = rdma_req->requested_size;

	{
		/* Computing remote rail */
		int remote_rail_nb = rdma_req->remote_rail;
		
		assume ( remote_rail_nb < sctk_rail_count() );
		
		rdma->remote_rail     = sctk_rail_get_by_id( remote_rail_nb );

		/* Get the remote QP from the remote rail */
		int src_process;
		sctk_endpoint_t *route;
		src_process = sctk_determine_src_process_from_header ( &msg->body );
		assume ( src_process != -1 );
		route = sctk_rail_get_any_route_to_process_or_on_demand ( rdma->remote_rail, src_process );
		assume ( route );

		sctk_nodebug ( "Got message on rail %d and reply on rail %d", rail->rail_number, rdma->remote_rail->rail_number );
		/* Update the remote peer */
		rdma->remote_peer = route->data.ib.remote;

	}
	rdma->rail        = rail;
	rdma->local.addr  = NULL;
	rdma->remote.msg_header = rdma_req->dest_msg_header;
	rdma->local.msg_header = msg;
	rdma->local.ready = 0;
	rdma->ht_key = OPA_fetch_and_incr_int ( &recv_rdma_headers_nb );
	
	sctk_spinlock_lock ( &recv_rdma_headers_lock );
	HASH_ADD ( hh, recv_rdma_headers, ht_key, sizeof ( int ), rdma );
	sctk_nodebug ( "ADD msg %p with key %d", rdma, rdma->ht_key );
	sctk_spinlock_unlock ( &recv_rdma_headers_lock );
	
	/* XXX: Only for collaborative polling */
	rdma->source_task = IBUF_GET_SRC_TASK ( ibuf->buffer );
	rdma->destination_task = IBUF_GET_DEST_TASK ( ibuf->buffer );

	/* Send message to MPC. The message can be matched at the end
	 * of function. */
	rail->send_message_from_network ( msg );

	sctk_nodebug ( "End send_message (matching:%p, stats:%d, "
	               "requested_size:%lu, required_size:%lu)",
	               rdma->local.addr, rdma->local.status,
	               rdma_req->requested_size, rdma->local.size );

	return msg;
}

void sctk_ib_rdma_rendezvous_prepare_send_msg ( sctk_ib_rail_info_t *rail_ib,
												sctk_thread_ptp_message_t *msg, size_t size )
{
	sctk_ib_header_rdma_t *rdma = &msg->tail.ib.rdma;
	void *aligned_addr = NULL;
	size_t aligned_size = 0;

	/* Do not allocate memory if contiguous message */
	if ( msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS )
	{

		sctk_ib_rdma_align_msg ( msg->tail.message.contiguous.addr,
		                         msg->tail.message.contiguous.size,
		                         &aligned_addr, &aligned_size );

		rdma->local.addr = msg->tail.message.contiguous.addr;
		rdma->local.size = msg->tail.message.contiguous.size;
		rdma->local.status = SCTK_IB_RDMA_ZEROCOPY;
	}
	else
	{
		size_t page_size;
		aligned_size = size - sizeof ( sctk_thread_ptp_message_body_t );
		page_size = getpagesize();

		posix_memalign ( ( void ** ) &aligned_addr, page_size, aligned_size );
		PROF_INC ( rail_ib->rail, ib_alloc_mem );
		sctk_net_copy_in_buffer ( msg, aligned_addr );

		sctk_nodebug ( "Sending NOT contiguous message %p of size: %lu, add:%p, type:%d (src cs:%lu, dest cs:%lu)", msg, aligned_size, aligned_addr, msg->tail.message_type, msg->body.checksum, sctk_checksum_buffer ( aligned_addr, msg ) );

		rdma->local.addr = aligned_addr;
		rdma->local.size = aligned_size;
		rdma->local.status = SCTK_IB_RDMA_RECOPY;
	}

	/* Register MMU */
	PROF_TIME_START ( rail_ib->rail, send_mmu_register );
	sctk_nodebug ( "[%d] register mmu", rail_ib->rail_nb );
	rdma->local.mmu_entry =  sctk_ib_mmu_pin ( &rdma->remote_rail->network.ib, aligned_addr, aligned_size );
	PROF_TIME_END ( rail_ib->rail, send_mmu_register );

	/* Save addr and size */
	rdma->local.aligned_addr = aligned_addr;
	rdma->local.aligned_size = aligned_size;
	rdma->source_task = SCTK_MSG_SRC_TASK ( msg );
	rdma->destination_task = SCTK_MSG_DEST_TASK ( msg );
	rdma->local.msg_header = msg;
	/* Message is now ready ready */
	rdma->local.ready = 1;
}


static inline sctk_thread_ptp_message_t * sctk_ib_rdma_rendezvous_recv_ack ( sctk_rail_info_t *rail, 
																			  sctk_ibuf_t *ibuf )
{
	sctk_ib_rdma_ack_t *rdma_ack;
	sctk_thread_ptp_message_t *dest_msg_header;
	sctk_thread_ptp_message_t *src_msg_header;
	sctk_ib_header_rdma_t *rdma;

	/* Save the values of the ack because the buffer will be reused */
	rdma_ack = IBUF_GET_RDMA_ACK ( ibuf->buffer );
	src_msg_header  = rdma_ack->dest_msg_header;
	rdma = &src_msg_header->tail.ib.rdma;
	dest_msg_header = rdma_ack->src_msg_header;

	struct sctk_ib_polling_s poll;
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;

	/* Wait while the message becomes ready */
	SCTK_PROFIL_START ( ib_rdma_idle );
	sctk_inter_thread_perform_idle ( ( int * ) &rdma->local.ready, 1,
	                                 ( void ( * ) ( void * ) ) sctk_network_notify_idle_message, NULL );
	SCTK_PROFIL_END ( ib_rdma_idle );

	sctk_nodebug ( "Remote addr: %p", rdma_ack->addr );
	rdma->remote.addr = rdma_ack->addr;
	rdma->remote.size = rdma_ack->size;
	rdma->remote.rkey = rdma_ack->rkey;
	rdma->remote.msg_header = dest_msg_header;
	rdma->ht_key = rdma_ack->ht_key;

	return src_msg_header;
}

inline void sctk_ib_rdma_rendezvous_prepare_data_write ( sctk_rail_info_t *rail,
														 sctk_thread_ptp_message_t *src_msg_header )
{
	LOAD_RAIL ( rail );
	sctk_ib_header_rdma_t *rdma = &src_msg_header->tail.ib.rdma;
	sctk_ib_rdma_data_write_t *rdma_data_write;
	sctk_ib_rdma_t *rdma_header;
	sctk_ibuf_t *ibuf;

	int src_process;
	sctk_endpoint_t *route;
	route = sctk_rail_get_any_route_to_process_or_on_demand ( rdma->remote_rail, rdma->remote_peer->rank );

	ibuf = sctk_ibuf_pick_send_sr ( &rdma->remote_rail->network.ib );
	assume ( ibuf );

	rdma_header = IBUF_GET_RDMA_HEADER ( ibuf->buffer );
	rdma_data_write = IBUF_GET_RDMA_DATA_WRITE ( ibuf->buffer );
	rdma_data_write->src_msg_header = src_msg_header;

	IBUF_SET_DEST_TASK ( ibuf->buffer, rdma->destination_task );
	IBUF_SET_SRC_TASK ( ibuf->buffer, rdma->source_task );
	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_RDMA_PROTOCOL );
	IBUF_SET_RDMA_TYPE ( rdma_header, SCTK_IB_RDMA_RDV_WRITE_TYPE );

	sctk_nodebug ( "Write from %p (%lu) to %p (%lu)",
	               rdma->local.addr,
	               rdma->local.size,
	               rdma->remote.addr,
	               rdma->remote.size );

	sctk_ibuf_rdma_write_init ( ibuf,
	                            rdma->local.addr,
	                            rdma->local.mmu_entry->mr->lkey,
	                            rdma->remote.addr,
	                            rdma->remote.rkey,
	                            rdma->local.size,
	                            IBV_SEND_SIGNALED, IBUF_RELEASE );

	sctk_ib_qp_send_ibuf ( &rdma->remote_rail->network.ib,
	                       route->data.ib.remote, ibuf);

/*
#endif
*/
	rdma->local.send_rdma_timestamp = sctk_ib_prof_get_time_stamp();
}

static inline sctk_thread_ptp_message_t * sctk_ib_rdma_rendezvous_prepare_done_write ( sctk_rail_info_t *rail, 
																						sctk_ibuf_t *incoming_ibuf )
{
	LOAD_RAIL ( rail );
	sctk_ib_header_rdma_t *rdma;
	sctk_thread_ptp_message_t *src_msg_header;
	sctk_thread_ptp_message_t *dest_msg_header;
	sctk_ib_rdma_data_write_t *rdma_data_write;
	sctk_ibuf_t *ibuf;
	size_t ibuf_size = IBUF_GET_RDMA_DONE_SIZE;


	rdma_data_write = IBUF_GET_RDMA_DATA_WRITE ( incoming_ibuf->buffer );
	src_msg_header = rdma_data_write->src_msg_header;
	rdma = &src_msg_header->tail.ib.rdma;
	dest_msg_header = rdma->remote.msg_header;

#if 1
	/* Initialize & send the buffer */
	/* FIXME: we can remove it now as we send the done message using imm_data.
	 * At least we should provide an option for fallbacking to the previous mode */
	sctk_ib_rdma_t *rdma_header;
	rdma_header = IBUF_GET_RDMA_HEADER ( incoming_ibuf->buffer );
	ibuf = sctk_ibuf_pick_send ( rail_ib, rdma->remote_peer, &ibuf_size );
	assume ( ibuf );
	sctk_ib_rdma_done_t *rdma_done;
	rdma_done = IBUF_GET_RDMA_DONE ( ibuf->buffer );
	rdma_done->dest_msg_header = dest_msg_header;
	rdma_header = IBUF_GET_RDMA_HEADER ( ibuf->buffer );
	IBUF_SET_DEST_TASK ( ibuf->buffer, rdma->destination_task );
	IBUF_SET_SRC_TASK ( ibuf->buffer, rdma->source_task );

	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_RDMA_PROTOCOL );
	IBUF_SET_RDMA_TYPE ( rdma_header, SCTK_IB_RDMA_RDV_DONE_TYPE );

	sctk_ib_qp_send_ibuf ( rail_ib, rdma->remote_peer, ibuf);
#endif

	return src_msg_header;
}


/*-----------------------------------------------------------
 *  RDMA ATOMIC OPERATIONS GATES
 *----------------------------------------------------------*/

int sctk_ib_rdma_fetch_and_op_gate( size_t size, RDMA_op op, RDMA_type type )
{
	/* IB only supports 64 bits operands */
	if( size != 8 )
	{
		return 0;
	}
	
	/* IB only has fetch and ADD */
	if( op != RDMA_SUM )
	{
		return 0;
	}
	
	return 1;
}

int sctk_ib_rdma_swap_gate( size_t size, RDMA_op op, RDMA_type type )
{
	/* IB only supports 64 bits operands */
	if( size != 8 )
	{
		return 0;
	}
	
	return 1;
}

/*-----------------------------------------------------------
 *  RDMA READ & WRITE
 *----------------------------------------------------------*/

sctk_thread_ptp_message_t * sctk_ib_rdma_retrieve_msg_ptr(  sctk_ibuf_t *ibuf )
{
	sctk_ib_rdma_t *rdma_header  = IBUF_GET_RDMA_HEADER ( ibuf->buffer );
	
	sctk_thread_ptp_message_t * ret = IBUF_GET_RDMA_POINTER( rdma_header  );
	
	return ret;
}


void sctk_ib_rdma_write(  sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
                         void * src_addr, struct sctk_rail_pin_ctx_list * local_key,
                         void * dest_addr, struct sctk_rail_pin_ctx_list * remote_key,
                         size_t size )
{
	LOAD_RAIL ( rail );

	sctk_error("RDMA WRITE NET INSIDE");

	int src_process;
	sctk_endpoint_t  * route = sctk_rail_get_any_route_to_process_or_on_demand ( rail, SCTK_MSG_DEST_PROCESS ( msg ) );

	sctk_ibuf_t * ibuf = sctk_ibuf_pick_send_sr ( rail_ib );
	assume ( ibuf );

	sctk_ib_rdma_t *rdma_header  = IBUF_GET_RDMA_HEADER ( ibuf->buffer );
	

	IBUF_SET_DEST_TASK ( ibuf->buffer, SCTK_MSG_DEST_TASK ( msg ) );
	IBUF_SET_SRC_TASK ( ibuf->buffer, SCTK_MSG_SRC_TASK ( msg ) );
	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_RDMA_PROTOCOL );
	
	IBUF_SET_RDMA_TYPE ( rdma_header, SCTK_IB_RDMA_WRITE );
	IBUF_SET_RDMA_PLSIZE( rdma_header, size);
	IBUF_SET_RDMA_POINTER( rdma_header, msg );

	sctk_error ( "RDMA Write from %p (%lu) to %p (%lu)",
	               src_addr,
	               size,
	               dest_addr,
	               size );

	sctk_ibuf_rdma_write_init ( ibuf,
	                            src_addr,
	                            local_key->pin.ib.mr.lkey,
	                            dest_addr,
	                            remote_key->pin.ib.mr.rkey,
	                            size,
	                            IBV_SEND_SIGNALED, IBUF_RELEASE );

	sctk_ib_qp_send_ibuf ( &rail->network.ib,
	                       route->data.ib.remote, ibuf);

}

 
void sctk_ib_rdma_read(   sctk_rail_info_t *rail, sctk_thread_ptp_message_t *msg,
                         void * src_addr, struct  sctk_rail_pin_ctx_list * remote_key,
                         void * dest_addr, struct  sctk_rail_pin_ctx_list * local_key,
                         size_t size )
{
	LOAD_RAIL ( rail );

	int src_process;
	sctk_endpoint_t  * route = sctk_rail_get_any_route_to_process_or_on_demand ( rail, SCTK_MSG_SRC_PROCESS ( msg ) );

	sctk_ibuf_t * ibuf = sctk_ibuf_pick_send_sr ( rail_ib );
	assume ( ibuf );

	sctk_ib_rdma_t *rdma_header  = IBUF_GET_RDMA_HEADER ( ibuf->buffer );

	IBUF_SET_DEST_TASK ( ibuf->buffer, SCTK_MSG_DEST_TASK ( msg ) );
	IBUF_SET_SRC_TASK ( ibuf->buffer, SCTK_MSG_SRC_TASK ( msg ) );
	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_RDMA_PROTOCOL );
	
	IBUF_SET_RDMA_TYPE ( rdma_header, SCTK_IB_RDMA_READ );
	IBUF_SET_RDMA_PLSIZE( rdma_header, size);
	IBUF_SET_RDMA_POINTER( rdma_header, msg );


	sctk_error ( "RDMA Read from %p (%lu) to %p (%lu)",
	               src_addr,
	               size,
	               dest_addr,
	               size );
	               
	             
	sctk_ibuf_rdma_read_init ( ibuf, dest_addr, local_key->pin.ib.mr.rkey, src_addr, remote_key->pin.ib.mr.lkey, size );

	sctk_ib_qp_send_ibuf ( &rail->network.ib,
	                       route->data.ib.remote, ibuf);

}


void sctk_ib_rdma_fetch_and_op(   sctk_rail_info_t *rail,
								  sctk_thread_ptp_message_t *msg,
								  void * fetch_addr,
								  struct  sctk_rail_pin_ctx_list * local_key,
								  void * remote_addr,
								  struct  sctk_rail_pin_ctx_list * remote_key,
								  sctk_uint64_t add,
								  RDMA_op op,
							      RDMA_type type )
{
	LOAD_RAIL ( rail );
	
	if( op != RDMA_SUM )
	{
		sctk_fatal("Infiniband only supports the SUM operand");
	}
	
	if( RDMA_type_size( type ) != 8 )
	{
		sctk_fatal("Infiniband only supports 64bits operands");	
	}

	int src_process;
	sctk_endpoint_t  * route = sctk_rail_get_any_route_to_process_or_on_demand ( rail, SCTK_MSG_SRC_PROCESS ( msg ) );

	sctk_ibuf_t * ibuf = sctk_ibuf_pick_send_sr ( rail_ib );
	assume ( ibuf );

	sctk_ib_rdma_t *rdma_header  = IBUF_GET_RDMA_HEADER ( ibuf->buffer );

	IBUF_SET_DEST_TASK ( ibuf->buffer, SCTK_MSG_DEST_TASK ( msg ) );
	IBUF_SET_SRC_TASK ( ibuf->buffer, SCTK_MSG_SRC_TASK ( msg ) );
	IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_RDMA_PROTOCOL );
	
	IBUF_SET_RDMA_TYPE ( rdma_header, SCTK_IB_RDMA_FETCH_AND_ADD );
	IBUF_SET_RDMA_POINTER( rdma_header, msg );


	sctk_error ( "RDMA Fetch and add from %p to %p add %lld",
	               remote_addr,
	               fetch_addr,
	               add );
	               
	sctk_ibuf_rdma_fetch_and_add_init(  ibuf,
									    fetch_addr,
										local_key->pin.ib.mr.rkey,
										remote_addr,
										remote_key->pin.ib.mr.lkey,
										add );

	sctk_ib_qp_send_ibuf ( &rail->network.ib,
	                       route->data.ib.remote, ibuf);

}


/*-----------------------------------------------------------
 *  POLLING SEND / RECV
 *----------------------------------------------------------*/
 
int sctk_ib_rdma_poll_recv ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf )
{
	sctk_ib_rdma_t *rdma_header;
	sctk_thread_ptp_message_t *header;

	IBUF_CHECK_POISON ( ibuf->buffer );
	rdma_header = IBUF_GET_RDMA_HEADER ( ibuf->buffer );

	/* Switch on the RDMA type of message */
	switch ( IBUF_GET_RDMA_TYPE ( rdma_header ) )
	{
		case SCTK_IB_RDMA_RDV_REQ_TYPE:
		{
			sctk_nodebug ( "Poll recv: message RDMA req received" );
			sctk_ib_rdma_rendezvous_recv_req ( rail, ibuf );
			return 1;
		}
		break;

		case SCTK_IB_RDMA_RDV_ACK_TYPE:
		{
			sctk_nodebug ( "Poll recv: message RDMA ack received" );
			/* Buffer reused: don't need to free it */
			header = sctk_ib_rdma_rendezvous_recv_ack ( rail, ibuf );
			sctk_ib_rdma_rendezvous_prepare_data_write ( rail, header );
			return 1;
		}
		break;

		case SCTK_IB_RDMA_RDV_DONE_TYPE:
		{
			sctk_nodebug ( "Poll recv: message RDMA done received" );
			sctk_ib_rdma_recv_done_remote ( rail, ibuf );
			return 1;
		}
		break;

		default:
			sctk_error ( "RDMA type %d not found!!!", IBUF_GET_RDMA_TYPE ( rdma_header ) );
			not_reachable();
	}

	not_reachable();
	return 0;
}

int
sctk_ib_rdma_poll_send ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf )
{
	sctk_ib_rdma_t *rdma_header;
	/* If the buffer *MUST* be released */
	int release_ibuf = 1;
	sctk_thread_ptp_message_t *msg;

	IBUF_CHECK_POISON ( ibuf->buffer );
	rdma_header = IBUF_GET_RDMA_HEADER ( ibuf->buffer );

	/* Switch on the RDMA type of message */
	switch ( IBUF_GET_RDMA_TYPE ( rdma_header ) )
	{
		case SCTK_IB_RDMA_RDV_REQ_TYPE:
			sctk_nodebug ( "Poll send: message RDMA req received" );
			break;

		case SCTK_IB_RDMA_RDV_ACK_TYPE:
			sctk_nodebug ( "Poll send: message RDMA ack received" );
			break;

		case SCTK_IB_RDMA_RDV_WRITE_TYPE:
			sctk_nodebug ( "Poll send: message RDMA data write received" );
			msg = sctk_ib_rdma_rendezvous_prepare_done_write ( rail, ibuf );
			sctk_ib_rdma_recv_done_local ( rail, msg );
			break;

		case SCTK_IB_RDMA_RDV_DONE_TYPE:
			sctk_nodebug ( "Poll send: message RDMA done received" );
			break;


		case SCTK_IB_RDMA_WRITE:
			msg = sctk_ib_rdma_retrieve_msg_ptr( ibuf );
			sctk_error ( "RDMA Write DONE" );
			sctk_complete_and_free_message ( msg );
			break;


		case SCTK_IB_RDMA_READ:
			msg = sctk_ib_rdma_retrieve_msg_ptr( ibuf );
			sctk_error ( "RDMA Read DONE" );
			sctk_complete_and_free_message ( msg );
			break;

		case SCTK_IB_RDMA_FETCH_AND_ADD:
			msg = sctk_ib_rdma_retrieve_msg_ptr( ibuf );
			sctk_error ( "RDMA Fetch and add DONE" );
			sctk_complete_and_free_message ( msg );
			break;
			
		default:
			sctk_error ( "Got RDMA type:%d, payload_size;%lu", IBUF_GET_RDMA_TYPE ( rdma_header ), 
															   rdma_header->payload_size );
			break;
			char ibuf_desc[4096];
			sctk_error ( "BEGIN ERROR" );
			sctk_ibuf_print ( ibuf, ibuf_desc );
			sctk_error ( "\nIB - FATAL ERROR FROM PROCESS %d\n"
			             "######### IBUF DESC ############\n"
			             "%s\n"
			             "################################",
			             sctk_process_rank, ibuf_desc );
			sctk_error ( "END ERROR" );
//      not_reachable();
	}

	return release_ibuf;
}

/*-----------------------------------------------------------
 *  PRINT
 *----------------------------------------------------------*/
void sctk_ib_rdma_print ( sctk_thread_ptp_message_t *msg )
{
	sctk_ib_msg_header_t *h = &msg->tail.ib;

	sctk_error ( "MSG INFOS\n"
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
	             h->rdma.remote.aligned_size );
}

#endif
