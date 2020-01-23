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
#include "sctk_ib_buffered.h"
#include "sctk_ib_polling.h"
#include "sctk_ib_topology.h"
#include "sctk_ibufs.h"
#include "sctk_route.h"
#include "sctk_ib_mmu.h"
#include "sctk_net_tools.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_cp.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_ib_prof.h"

#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
//#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "BUFF"
#include "sctk_ib_toolkit.h"
#include "math.h"


/* XXX: Modifications required:
 * - copy in user buffer if the message has already been posted - DONE
 * - Support of fragmented copy
 *
 * */

/* Handle non contiguous messages. Returns 1 if the message was handled, 0 if not */
static inline void *sctk_ib_buffered_send_non_contiguous_msg ( __UNUSED__ sctk_rail_info_t *rail,
                                                               __UNUSED__ sctk_ib_qp_t *remote,
                                                               mpc_lowcomm_ptp_message_t *msg,
                                                               size_t size )
{
	void *buffer = NULL;
	buffer = sctk_malloc ( size );
	sctk_net_copy_in_buffer ( msg, buffer );

	return buffer;
}


int sctk_ib_buffered_prepare_msg ( sctk_rail_info_t *rail,
                                   sctk_ib_qp_t *remote, mpc_lowcomm_ptp_message_t *msg, size_t size )
{
	sctk_ib_rail_info_t *rail_ib = &rail->network.ib;
	/* Maximum size for an eager buffer */
	size = size - sizeof ( mpc_lowcomm_ptp_message_body_t );
	int    buffer_index = 0;
	size_t msg_copied = 0;
	size_t payload_size;
	sctk_ibuf_t *ibuf;
	sctk_ib_buffered_t *buffered;
	void *payload;
	int number;

	if ( msg->tail.message_type != SCTK_MESSAGE_CONTIGUOUS )
	{
		payload = sctk_ib_buffered_send_non_contiguous_msg ( rail, remote, msg, size );
		assume ( payload );
	}
	else
	{
		payload = msg->tail.message.contiguous.addr;
	}

	number = OPA_fetch_and_incr_int ( &remote->ib_buffered.number );
	sctk_nodebug ( "Sending buffered message (size:%lu)", size );

	/* While it reamins slots to copy */
	do
	{
		size_t ibuf_size = ULONG_MAX;
		ibuf = sctk_ibuf_pick_send ( rail_ib, remote, &ibuf_size );
		ib_assume ( ibuf );

		size_t buffer_size = ibuf_size;
		/* We remove the buffered header's size from the size */
		sctk_nodebug ( "Payload with size %lu %lu", buffer_size, sizeof ( mpc_lowcomm_ptp_message_body_t ) );
		buffer_size -= IBUF_GET_BUFFERED_SIZE;
		sctk_nodebug ( "Sending a message with size %lu", buffer_size );

		buffered = IBUF_GET_BUFFERED_HEADER ( ibuf->buffer );
		buffered->number = number;

		ib_assume ( buffer_size >= sizeof ( mpc_lowcomm_ptp_message_body_t ) );
		memcpy ( &buffered->msg, msg, sizeof ( mpc_lowcomm_ptp_message_body_t ) );
		sctk_nodebug ( "Send number %d, src:%d, dest:%d", SCTK_MSG_NUMBER ( msg ), SCTK_MSG_SRC_TASK ( msg ), SCTK_MSG_DEST_TASK ( msg ) );

		if ( msg_copied + buffer_size > size )
		{
			payload_size = size - msg_copied;
		}
		else
		{
			payload_size = buffer_size;
		}

		PROF_TIME_START ( rail_ib->rail, ib_buffered_memcpy );
		memcpy ( IBUF_GET_BUFFERED_PAYLOAD ( ibuf->buffer ), ( char * ) payload + msg_copied,
		         payload_size );
		PROF_TIME_END ( rail_ib->rail, ib_buffered_memcpy );

		sctk_nodebug ( "Send message %d to %d (msg_copied:%lu pyload_size:%lu header:%lu, buffer_size:%lu number:%lu)",
		               buffer_index, remote->rank, msg_copied, payload_size, IBUF_GET_BUFFERED_SIZE, buffer_size, SCTK_MSG_NUMBER ( msg ) );
		/* Initialization of the buffer */
		buffered->index = buffer_index;
		buffered->payload_size = payload_size;
		buffered->copied = msg_copied;
		IBUF_SET_PROTOCOL ( ibuf->buffer, SCTK_IB_BUFFERED_PROTOCOL );
		msg_copied += payload_size;

		IBUF_SET_DEST_TASK ( ibuf->buffer, SCTK_MSG_DEST_TASK ( msg ) );
		IBUF_SET_SRC_TASK ( ibuf->buffer, SCTK_MSG_SRC_TASK ( msg ) );

		/* Recalculate size and send */
		sctk_ibuf_prepare ( remote, ibuf, payload_size + IBUF_GET_BUFFERED_SIZE );
		sctk_ib_qp_send_ibuf ( rail_ib, remote, ibuf );

		buffer_index++;
	}
	while ( msg_copied < size );

	/* We free the temp copy */
	if ( msg->tail.message_type != SCTK_MESSAGE_CONTIGUOUS )
	{
		assume ( payload );
		sctk_free ( payload );
	}

	sctk_nodebug ( "End of message sending" );
	return 0;
}

void sctk_ib_buffered_free_msg ( void *arg )
{
	mpc_lowcomm_ptp_message_t *msg = ( mpc_lowcomm_ptp_message_t * ) arg;
	sctk_ib_buffered_entry_t *entry = NULL;
	__UNUSED__ sctk_rail_info_t *rail;

	entry = msg->tail.ib.buffered.entry;
	rail = entry->msg.tail.ib.buffered.rail;
	ib_assume ( entry );

	switch ( entry->status & MASK_BASE )
	{
		case SCTK_IB_RDMA_RECOPY:
			sctk_nodebug ( "Free payload %p from entry %p", entry->payload, entry );
			sctk_free ( entry->payload );
			PROF_INC ( rail, ib_free_mem );
			break;

		case SCTK_IB_RDMA_ZEROCOPY:
			/* Nothing to do */
			break;

		default:
			not_reachable();
	}
}

void sctk_ib_buffered_copy ( mpc_lowcomm_ptp_message_content_to_copy_t *tmp )
{
	sctk_ib_buffered_entry_t *entry = NULL;
	__UNUSED__ sctk_rail_info_t *rail;
	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;

	recv = tmp->msg_recv;
	send = tmp->msg_send;
	rail = send->tail.ib.buffered.rail;
	entry = send->tail.ib.buffered.entry;
	ib_assume ( entry );
	//ib_assume(recv->tail.message_type == SCTK_MESSAGE_CONTIGUOUS);

	mpc_common_spinlock_lock ( &entry->lock );
	entry->copy_ptr = tmp;
	sctk_nodebug ( "Copy status (%p): %d", entry, entry->status );

	switch ( entry->status & MASK_BASE )
	{
		case SCTK_IB_RDMA_NOT_SET:
			sctk_nodebug ( "Message directly copied (entry:%p)", entry );

			if ( recv->tail.message_type == SCTK_MESSAGE_CONTIGUOUS )
			{
				entry->payload = recv->tail.message.contiguous.addr;
				/* Add matching OK */
				entry->status = SCTK_IB_RDMA_ZEROCOPY | SCTK_IB_RDMA_MATCH;
				mpc_common_spinlock_unlock ( &entry->lock );
				break;
			}

		case SCTK_IB_RDMA_RECOPY:
			sctk_nodebug ( "Message recopied" );

			/* transfer done */
			if ( ( entry->status & MASK_DONE ) == SCTK_IB_RDMA_DONE )
			{
				mpc_common_spinlock_unlock ( &entry->lock );
				/* The message is done. All buffers have been received */
				sctk_nodebug ( "Message recopied free from copy %d (%p)", entry->status, entry );
				sctk_net_message_copy_from_buffer ( entry->payload, tmp, 1 );
				sctk_free ( entry );
				PROF_INC ( rail, ib_free_mem );
			}
			else
			{
				sctk_nodebug ( "Matched" );
				/* Add matching OK */
				entry->status |= SCTK_IB_RDMA_MATCH;
				sctk_nodebug ( "1 Matched ? %p %d", entry, entry->status & MASK_MATCH );
				mpc_common_spinlock_unlock ( &entry->lock );
			}

			break;

		default:
			sctk_error ( "Got mask %d", entry->status );
			not_reachable();
	}
}

static inline sctk_ib_buffered_entry_t *sctk_ib_buffered_get_entry ( sctk_rail_info_t *rail,
                                                                     sctk_ib_qp_t *remote,
                                                                     sctk_ibuf_t *ibuf )
{
	sctk_ib_buffered_entry_t *entry = NULL;
	mpc_lowcomm_ptp_message_body_t *body;
	sctk_ib_buffered_t *buffered;
	int key;

	buffered = IBUF_GET_BUFFERED_HEADER ( ibuf->buffer );
	body = &buffered->msg;
	key = buffered->number;
	sctk_nodebug ( "Got message number %d", key );

	mpc_common_spinlock_lock ( &remote->ib_buffered.lock );
	HASH_FIND ( hh, remote->ib_buffered.entries, &key, sizeof ( int ), entry );

	if ( !entry )
	{
		/* Allocate memory for message header */
		entry = sctk_malloc ( sizeof ( sctk_ib_buffered_entry_t ) );
		ib_assume ( entry );
		PROF_INC ( rail, ib_alloc_mem );
		/* Copy message header */
		memcpy ( &entry->msg.body, body, sizeof ( mpc_lowcomm_ptp_message_body_t ) );
		entry->msg.tail.ib.protocol = SCTK_IB_BUFFERED_PROTOCOL;
		entry->msg.tail.ib.buffered.entry = entry;
		entry->msg.tail.ib.buffered.rail = rail;
		/* Prepare matching */
		entry->msg.tail.completion_flag = NULL;
		entry->msg.tail.message_type = SCTK_MESSAGE_NETWORK;
		_mpc_comm_ptp_message_clear_request ( &entry->msg );
		_mpc_comm_ptp_message_set_copy_and_free ( &entry->msg, sctk_ib_buffered_free_msg,
		                     sctk_ib_buffered_copy );
		/* Add msg to hashtable */
		entry->key = key;
		entry->total = buffered->nb;
		entry->status = SCTK_IB_RDMA_NOT_SET;
		sctk_nodebug ( "Not set: %d (%p)", entry->status, entry );
		entry->lock = SCTK_SPINLOCK_INITIALIZER;
		entry->current_copied_lock = SCTK_SPINLOCK_INITIALIZER;
		entry->current_copied = 0;
		entry->payload = NULL;
		entry->copy_ptr = NULL;
		HASH_ADD ( hh, remote->ib_buffered.entries, key, sizeof ( int ), entry );
		/* Send message to high level MPC */

		sctk_nodebug ( "Read msg with number %d", body->header.message_number );
		rail->send_message_from_network ( &entry->msg );

		mpc_common_spinlock_lock ( &entry->lock );

		/* Should be 'SCTK_IB_RDMA_NOT_SET' or 'SCTK_IB_RDMA_ZEROCOPY' */
		if ( ( entry->status & MASK_BASE ) == SCTK_IB_RDMA_NOT_SET )
		{
			sctk_nodebug ( "We recopy the message" );
			entry->payload = sctk_malloc ( body->header.msg_size );
			ib_assume ( entry->payload );
			PROF_INC ( rail, ib_alloc_mem );
			entry->status |= SCTK_IB_RDMA_RECOPY;
		}
		else
			if ( ( entry->status & MASK_BASE ) != SCTK_IB_RDMA_ZEROCOPY )
				not_reachable();

		mpc_common_spinlock_unlock ( &entry->lock );
	}

	mpc_common_spinlock_unlock ( &remote->ib_buffered.lock );

	return entry;
}

void sctk_ib_buffered_poll_recv ( sctk_rail_info_t *rail, sctk_ibuf_t *ibuf )
{
	mpc_lowcomm_ptp_message_body_t *body;
	sctk_ib_buffered_t *buffered;
	sctk_endpoint_t *route_table;
	sctk_ib_buffered_entry_t *entry = NULL;
	sctk_ib_qp_t *remote;
	size_t current_copied;
	int src_process;

	sctk_nodebug ( "Polled buffered message" );

	IBUF_CHECK_POISON ( ibuf->buffer );
	buffered = IBUF_GET_BUFFERED_HEADER ( ibuf->buffer );
	body = &buffered->msg;

	/* Determine the Source process */
	src_process = body->header.source;
	ib_assume ( src_process != -1 );
	/* Determine if the message is expected or not (good sequence number) */
	route_table = sctk_rail_get_any_route_to_process ( rail, src_process );
	ib_assume ( route_table );
	remote = route_table->data.ib.remote;

	/* Get the entry */
	entry = sctk_ib_buffered_get_entry ( rail, remote, ibuf );

	//fprintf(stderr, "Copy frag %d on %d (size:%lu copied:%lu)\n", buffered->index, buffered->nb, buffered->payload_size, buffered->copied);
	/* Copy the message payload */
	memcpy ( ( char * ) entry->payload + buffered->copied, IBUF_GET_BUFFERED_PAYLOAD ( ibuf->buffer ),
	         buffered->payload_size );

	/* We check if we have receive the whole message.
	* If yes, we send it to MPC */
	mpc_common_spinlock_lock ( &entry->current_copied_lock );
	current_copied = ( entry->current_copied += buffered->payload_size );
	mpc_common_spinlock_unlock ( &entry->current_copied_lock );
	sctk_nodebug ( "Received current copied : %lu on %lu number %d", current_copied, body->header.msg_size, body->header.message_number );

	/* XXX: horrible use of locks. but we do not have the choice */
	if ( current_copied >= body->header.msg_size )
	{
		ib_assume ( current_copied == body->header.msg_size );
		/* remove entry from HT.
		* XXX: We have to do this before marking message as done */
		mpc_common_spinlock_lock ( &remote->ib_buffered.lock );
		assume ( remote->ib_buffered.entries != NULL );
		HASH_DEL ( remote->ib_buffered.entries, entry );
		mpc_common_spinlock_unlock ( &remote->ib_buffered.lock );

		mpc_common_spinlock_lock ( &entry->lock );
		sctk_nodebug ( "2 - Matched ? %p %d", entry, entry->status & MASK_MATCH );

		switch ( entry->status & MASK_BASE )
		{
			case SCTK_IB_RDMA_RECOPY:

				/* Message matched */
				if ( ( entry->status & MASK_MATCH ) == SCTK_IB_RDMA_MATCH )
				{
					mpc_common_spinlock_unlock ( &entry->lock );
					ib_assume ( entry->copy_ptr );
					/* The message is done. All buffers have been received */
					sctk_net_message_copy_from_buffer ( entry->payload, entry->copy_ptr, 1 );
					sctk_nodebug ( "Message recopied free from done" );
					sctk_free ( entry );
					PROF_INC ( rail, ib_free_mem );
				}
				else
				{
					sctk_nodebug ( "Free done:%p", entry );
					entry->status |= SCTK_IB_RDMA_DONE;
					mpc_common_spinlock_unlock ( &entry->lock );
				}

				break;

			case SCTK_IB_RDMA_ZEROCOPY:

				/* Message matched */
				if ( ( entry->status & MASK_MATCH ) == SCTK_IB_RDMA_MATCH )
				{
					mpc_common_spinlock_unlock ( &entry->lock );
					ib_assume ( entry->copy_ptr );
					_mpc_comm_ptp_message_commit_request ( entry->copy_ptr->msg_send,
					                                   entry->copy_ptr->msg_recv );
					sctk_free ( entry );
					PROF_INC ( rail, ib_free_mem );
					sctk_nodebug ( "Message zerocpy free from done" );
				}
				else
				{
					mpc_common_spinlock_unlock ( &entry->lock );
					not_reachable();
				}

				break;

			default:
				not_reachable();
		}

		sctk_nodebug ( "Free done:%p", entry );
	}
}
#endif
