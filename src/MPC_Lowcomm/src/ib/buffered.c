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
#include "buffered.h"
#include "endpoint.h"
#include "qp.h"

#include <msg_cpy.h>
#include <sctk_alloc.h>

#include "rail.h"
/* XXX: Modifications required:
 * - copy in user buffer if the message has already been posted - DONE
 * - Support of fragmented copy
 *
 * */

/* Handle non contiguous messages. Returns 1 if the message was handled, 0 if not */
static inline void *__pack_non_contig_msg(mpc_lowcomm_ptp_message_t *msg,
                                          size_t size)
{
	void *buffer = NULL;

	buffer = sctk_malloc(size);
	_mpc_lowcomm_msg_cpy_in_buffer(msg, buffer);

	return buffer;
}

int _mpc_lowcomm_ib_buffered_prepare_msg(sctk_rail_info_t *rail,
                                         _mpc_lowcomm_ib_qp_t *remote,
                                         mpc_lowcomm_ptp_message_t *msg,
                                         size_t size)
{
	_mpc_lowcomm_ib_rail_info_t *rail_ib = &rail->network.ib;

	/* Maximum size for an eager buffer */
	size = size - sizeof(mpc_lowcomm_ptp_message_body_t);

	int    buffer_index = 0;
	size_t already_sent = 0;

	void *payload = NULL;

	if(msg->tail.message_type != MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
	{
		payload = __pack_non_contig_msg(msg, size);
		assume(payload);
	}
	else
	{
		payload = msg->tail.message.contiguous.addr;
	}

	int number = OPA_fetch_and_incr_int(&remote->ib_buffered.number);
	mpc_common_nodebug("Sending buffered message (size:%lu)", size);

	/* While it reamins slots to copy */
	do
	{
		size_t buffer_size           = ULONG_MAX;
		_mpc_lowcomm_ib_ibuf_t *ibuf = _mpc_lowcomm_ib_ibuf_pick_send(rail_ib, remote, &buffer_size);
		ib_assume(ibuf);

		/* At this point buffer_size is now max eager size */

		/* We remove the buffered header's size from the size
		 * at it cannot be accounted for actual payload */
		assume(buffer_size > sizeof(_mpc_lowcomm_ib_buffered_t) );
		buffer_size -= sizeof(_mpc_lowcomm_ib_buffered_t);

		_mpc_lowcomm_ib_buffered_t *buffered_msg = (_mpc_lowcomm_ib_buffered_t *)(ibuf->buffer);

		/* Copy message header */
		memcpy(&buffered_msg->msg, msg, sizeof(mpc_lowcomm_ptp_message_body_t) );

		size_t payload_size = 0;

		if(size < already_sent + buffer_size)
		{
			/* Can fit in last buffer send the remainder */
			payload_size = size - already_sent;
		}
		else
		{
			/* Still too large send full buffer */
			payload_size = buffer_size;
		}

		/* Copy message payload */
		memcpy(buffered_msg->payload,
		       ( char * )payload + already_sent,
		       payload_size);

	    mpc_common_nodebug("Send message %d to %d (already_sent:%lu pyload_size:%lu header:%lu, buffer_size:%lu number:%lu)",
		                   buffer_index, remote->rank, already_sent, payload_size, sizeof(_mpc_lowcomm_ib_buffered_t), buffer_size, SCTK_MSG_NUMBER(msg) );

		/* Initialization of the buffer */

		buffered_msg->buffered_header.number       = number;
		buffered_msg->buffered_header.index        = buffer_index;
		buffered_msg->buffered_header.payload_size = payload_size;
		buffered_msg->buffered_header.copied       = already_sent;

		IBUF_SET_PROTOCOL(ibuf->buffer, MPC_LOWCOMM_IB_BUFFERED_PROTOCOL);

		IBUF_SET_DEST_TASK(ibuf->buffer, SCTK_MSG_DEST_TASK(msg) );
		IBUF_SET_SRC_TASK(ibuf->buffer, SCTK_MSG_SRC_TASK(msg) );

		/* Recalculate size and send */
		_mpc_lowcomm_ib_ibuf_prepare(remote, ibuf, payload_size + sizeof(_mpc_lowcomm_ib_buffered_t) );
		_mpc_lowcomm_ib_qp_send_ibuf(rail_ib, remote, ibuf);

		buffer_index++;
		already_sent += payload_size;
	} while(already_sent < size);

	/* We free the temp copy */
	if(msg->tail.message_type != MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
	{
		assume(payload);
		sctk_free(payload);
	}

	mpc_common_nodebug("End of message sending");
	return 0;
}

static void __buffered_free_msg(void *arg)
{
	mpc_lowcomm_ptp_message_t *       msg   = ( mpc_lowcomm_ptp_message_t * )arg;
	_mpc_lowcomm_ib_buffered_entry_t *entry = NULL;

	entry = msg->tail.ib.buffered.entry;
	ib_assume(entry);

	switch(entry->status & MASK_BASE)
	{
		case MPC_LOWCOMM_IB_RDMA_RECOPY:
			mpc_common_nodebug("Free payload %p from entry %p", entry->payload, entry);
			sctk_free(entry->payload);
			break;

		case MPC_LOWCOMM_IB_RDMA_ZEROCOPY:
			/* Nothing to do */
			break;

		default:
			not_reachable();
	}
}

static void __buffered_copy_msg(mpc_lowcomm_ptp_message_content_to_copy_t *tmp)
{
	_mpc_lowcomm_ib_buffered_entry_t *entry = NULL;

	mpc_lowcomm_ptp_message_t *send;
	mpc_lowcomm_ptp_message_t *recv;

	recv = tmp->msg_recv;
	send = tmp->msg_send;

	entry = send->tail.ib.buffered.entry;
	ib_assume(entry);
	//ib_assume(recv->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS);

	mpc_common_spinlock_lock(&entry->lock);
	entry->copy_ptr = tmp;
	mpc_common_nodebug("Copy status (%p): %d", entry, entry->status);

	switch(entry->status & MASK_BASE)
	{
		case MPC_LOWCOMM_IB_RDMA_NOT_SET:
			mpc_common_nodebug("Message directly copied (entry:%p)", entry);

			if(recv->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
			{
				entry->payload = recv->tail.message.contiguous.addr;
				/* Add matching OK */
				entry->status = MPC_LOWCOMM_IB_RDMA_ZEROCOPY | MPC_LOWCOMM_IB_RDMA_MATCH;
				mpc_common_spinlock_unlock(&entry->lock);
				break;
			}

		case MPC_LOWCOMM_IB_RDMA_RECOPY:
			mpc_common_nodebug("Message recopied");

			/* transfer done */
			if( (entry->status & MASK_DONE) == MPC_LOWCOMM_IB_RDMA_DONE)
			{
				mpc_common_spinlock_unlock(&entry->lock);
				/* The message is done. All buffers have been received */
				mpc_common_nodebug("Message recopied free from copy %d (%p)", entry->status, entry);
				_mpc_lowcomm_msg_cpy_from_buffer(entry->payload, tmp, 1);
				sctk_free(entry);
			}
			else
			{
				mpc_common_nodebug("Matched");
				/* Add matching OK */
				entry->status |= MPC_LOWCOMM_IB_RDMA_MATCH;
				mpc_common_nodebug("1 Matched ? %p %d", entry, entry->status & MASK_MATCH);
				mpc_common_spinlock_unlock(&entry->lock);
			}

			break;

		default:
			mpc_common_debug_error("Got mask %d", entry->status);
			not_reachable();
	}
}

static inline _mpc_lowcomm_ib_buffered_entry_t *__buffered_get_entry(sctk_rail_info_t *rail,
                                                                     _mpc_lowcomm_ib_qp_t *remote,
                                                                     _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	_mpc_lowcomm_ib_buffered_entry_t *entry        = NULL;
	_mpc_lowcomm_ib_buffered_t *      buffered_msg = (_mpc_lowcomm_ib_buffered_t *)ibuf->buffer;
	mpc_lowcomm_ptp_message_body_t *  body         = &buffered_msg->msg;
	int key = buffered_msg->buffered_header.number;

	mpc_common_nodebug("Got message number %d", key);

	mpc_common_spinlock_lock(&remote->ib_buffered.lock);

	HASH_FIND(hh, remote->ib_buffered.entries, &key, sizeof(int), entry);

	if(!entry)
	{
		/* Allocate memory for message header */
		entry = sctk_malloc(sizeof(_mpc_lowcomm_ib_buffered_entry_t) );
		ib_assume(entry);
		/* Copy message header */
		memcpy(&entry->msg.body, body, sizeof(mpc_lowcomm_ptp_message_body_t) );
		entry->msg.tail.ib.protocol       = MPC_LOWCOMM_IB_BUFFERED_PROTOCOL;
		entry->msg.tail.ib.buffered.entry = entry;
		entry->msg.tail.ib.buffered.rail  = rail;

		/* Prepare matching */
		entry->msg.tail.completion_flag = NULL;
		entry->msg.tail.message_type    = MPC_LOWCOMM_MESSAGE_NETWORK;
		_mpc_comm_ptp_message_clear_request(&entry->msg);
		_mpc_comm_ptp_message_set_copy_and_free(&entry->msg, __buffered_free_msg, __buffered_copy_msg);

		/* Add msg to hashtable */
		entry->key    = key;
		entry->total  = buffered_msg->buffered_header.nb;
		entry->status = MPC_LOWCOMM_IB_RDMA_NOT_SET;
		mpc_common_nodebug("Not set: %d (%p)", entry->status, entry);
		mpc_common_spinlock_init(&entry->lock, 0);
		mpc_common_spinlock_init(&entry->current_copied_lock, 0);
		entry->current_copied = 0;
		entry->payload        = NULL;
		entry->copy_ptr       = NULL;
		HASH_ADD(hh, remote->ib_buffered.entries, key, sizeof(int), entry);
		/* Send message to high level MPC */

		mpc_common_nodebug("Read msg with number %d", body->header.message_number);
		rail->send_message_from_network(&entry->msg);

		mpc_common_spinlock_lock(&entry->lock);

		/* Should be 'MPC_LOWCOMM_IB_RDMA_NOT_SET' or 'MPC_LOWCOMM_IB_RDMA_ZEROCOPY' */
		if( (entry->status & MASK_BASE) == MPC_LOWCOMM_IB_RDMA_NOT_SET)
		{
			mpc_common_nodebug("We recopy the message");
			entry->payload = sctk_malloc(body->header.msg_size);
			ib_assume(entry->payload);
			entry->status |= MPC_LOWCOMM_IB_RDMA_RECOPY;
		}
		else if( (entry->status & MASK_BASE) != MPC_LOWCOMM_IB_RDMA_ZEROCOPY)
		{
			not_reachable();
		}

		mpc_common_spinlock_unlock(&entry->lock);
	}

	mpc_common_spinlock_unlock(&remote->ib_buffered.lock);

	return entry;
}

void _mpc_lowcomm_ib_buffered_poll_recv(sctk_rail_info_t *rail, _mpc_lowcomm_ib_ibuf_t *ibuf)
{
	mpc_common_nodebug("Polled buffered message");

	IBUF_CHECK_POISON(ibuf->buffer);
	_mpc_lowcomm_ib_buffered_t *    buffered_msg = (_mpc_lowcomm_ib_buffered_t *)ibuf->buffer;
	mpc_lowcomm_ptp_message_body_t *body         = &buffered_msg->msg;

	/* Determine the Source process */
	int integer_src_task = body->header.source_task;


    mpc_lowcomm_communicator_id_t comm_id = body->header.communicator_id;
    
    
    mpc_lowcomm_communicator_t comm = mpc_lowcomm_get_communicator_from_id(comm_id);
    mpc_lowcomm_peer_uid_t src_process = -1;
   

    if(!mpc_lowcomm_communicator_is_intercomm(comm))
    {
        src_process = mpc_lowcomm_communicator_uid(comm, integer_src_task);
    }
    else
    {
        mpc_lowcomm_communicator_t rcomm = mpc_lowcomm_communicator_get_remote(comm);
        integer_src_task = mpc_lowcomm_group_rank_for(mpc_lowcomm_communicator_group(rcomm), integer_src_task, mpc_lowcomm_monitor_uid_of(mpc_lowcomm_peer_get_set(comm_id), integer_src_task));
        
        src_process = mpc_lowcomm_communicator_remote_uid(comm, integer_src_task);
    }

    //mpc_common_debug_error("MSG from %d to %d on comm %d src %s", body->header.source, body->header.destination, comm_id, mpc_lowcomm_peer_format(src_process));
    
    ib_assume(src_process != -1);
	/* Determine if the message is expected or not (good sequence number) */
	_mpc_lowcomm_endpoint_t *route_table = sctk_rail_get_any_route_to_process(rail, src_process);
	

    ib_assume(route_table);
    _mpc_lowcomm_ib_qp_t * remote = route_table->data.ib.remote;

	/* Get the entry */
	_mpc_lowcomm_ib_buffered_entry_t *entry = __buffered_get_entry(rail, remote, ibuf);

	//fprintf(stderr, "Copy frag %d on %d (size:%lu copied:%lu)\n", buffered_msg->buffered_header.index, buffered_msg->buffered_header.nb, buffered_msg->buffered_header.payload_size, buffered_msg->buffered_header.copied);
	/* Copy the message payload */
	memcpy( ( char * )entry->payload + buffered_msg->buffered_header.copied,
	        buffered_msg->payload,
	        buffered_msg->buffered_header.payload_size);

	size_t current_copied = 0;

	/* We check if we have receive the whole message.
	 * If yes, we send it to MPC */
	mpc_common_spinlock_lock(&entry->current_copied_lock);
    entry->current_copied += buffered_msg->buffered_header.payload_size;
	current_copied = entry->current_copied;
    mpc_common_spinlock_unlock(&entry->current_copied_lock);

	//mpc_common_debug_error("Received current copied : %lu on %lu number %d", current_copied, body->header.msg_size, body->header.message_number);


	/* XXX: horrible use of locks. but we do not have the choice */
	if(current_copied == body->header.msg_size)
	{
		mpc_common_spinlock_lock(&remote->ib_buffered.lock);
		assume(remote->ib_buffered.entries != NULL);
		HASH_DEL(remote->ib_buffered.entries, entry);
		mpc_common_spinlock_unlock(&remote->ib_buffered.lock);

		mpc_common_spinlock_lock(&entry->lock);
		mpc_common_nodebug("2 - Matched ? %p %d", entry, entry->status & MASK_MATCH);

		switch(entry->status & MASK_BASE)
		{
			case MPC_LOWCOMM_IB_RDMA_RECOPY:

				/* Message matched */
				if( (entry->status & MASK_MATCH) == MPC_LOWCOMM_IB_RDMA_MATCH)
				{
					mpc_common_spinlock_unlock(&entry->lock);
					ib_assume(entry->copy_ptr);
					/* The message is done. All buffers have been received */
					_mpc_lowcomm_msg_cpy_from_buffer(entry->payload, entry->copy_ptr, 1);
					mpc_common_nodebug("Message recopied free from done");
					sctk_free(entry);
				}
				else
				{
					mpc_common_nodebug("Free done:%p", entry);
					entry->status |= MPC_LOWCOMM_IB_RDMA_DONE;
					mpc_common_spinlock_unlock(&entry->lock);
				}

				break;

			case MPC_LOWCOMM_IB_RDMA_ZEROCOPY:
				/* Message matched */
				if( (entry->status & MASK_MATCH) == MPC_LOWCOMM_IB_RDMA_MATCH)
				{
					mpc_common_spinlock_unlock(&entry->lock);
					ib_assume(entry->copy_ptr);
					_mpc_comm_ptp_message_commit_request(entry->copy_ptr->msg_send,
					                                     entry->copy_ptr->msg_recv);
					sctk_free(entry);
					mpc_common_nodebug("Message zerocpy free from done");
				}
				else
				{
					mpc_common_spinlock_unlock(&entry->lock);
					not_reachable();
				}

				break;

			default:
				not_reachable();
		}

		mpc_common_nodebug("Free done:%p", entry);
	}
}
