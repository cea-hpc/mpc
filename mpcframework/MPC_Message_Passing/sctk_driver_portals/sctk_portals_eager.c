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
/* # had knowledge of the CeCILL-C license and tt you accept its          # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - ADAM Julien adamj@paratools.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#include <sctk_route.h>
#include <sctk_portals_toolkit.h>
#include <sctk_portals_eager.h>
#include <sctk_pmi.h>
#include <sctk_control_messages.h>
#include <sctk_net_tools.h>
#include <sctk_low_level_comm.h>

#define sctk_min(a, b)  ((a) < (b) ? (a) : (b))

void sctk_portals_eager_message_copy ( sctk_message_to_copy_t *tmp )
{
sctk_thread_ptp_message_t* sender;
	sctk_thread_ptp_message_t* recver;
	sctk_portals_msg_header_t* remote_info;
	sender = tmp->msg_send;
	recver = tmp->msg_recv;
	remote_info = &sender->tail.portals;
	size_t size;

	size = sctk_min(SCTK_MSG_SIZE(sender), SCTK_MSG_SIZE(recver));

	if(recver->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
	{
		memcpy(remote_info->payload, recver->tail.message.contiguous.addr, size);
	}
	else
	{
		sctk_net_message_copy_from_buffer(remote_info->payload, tmp, 0);
	}

	sctk_free(remote_info->payload);

	sctk_message_completion_and_free(sender, recver);
}

void sctk_portals_eager_free (void* msg)
{
	sctk_free(msg);
}

void sctk_portals_eager_send_put ( sctk_endpoint_t *endpoint, sctk_thread_ptp_message_t *msg)
{
	sctk_portals_rail_info_t * prail = &endpoint->rail->network.portals;
	sctk_portals_route_info_t *proute = &endpoint->data.portals;
	void* payload = NULL;
	sctk_portals_list_entry_extra_t * stuff = NULL;
	size_t payload_size = 0, header_size = 0, eager_size = 0;

	ptl_pt_index_t local_entry;
	ptl_pt_index_t remote_entry;

	payload = msg->tail.portals.payload;

	//create associated data
	stuff = sctk_malloc(sizeof(sctk_portals_list_entry_extra_t));
	stuff->cat_msg = SCTK_PORTALS_CAT_EAGER;
	stuff->extra_data = msg;

	//compute Portals entries
	remote_entry = SCTK_MSG_DEST_TASK(msg) % prail->ptable.nb_entries;
	local_entry = SCTK_MSG_SRC_TASK(msg) % prail->ptable.nb_entries;

	payload_size = SCTK_MSG_SIZE(msg);
	header_size = sizeof(sctk_thread_ptp_message_body_t);
	eager_size = header_size + payload_size;

	payload = sctk_malloc(eager_size);

	memcpy(payload, msg, header_size);

	if(SCTK_MSG_SRC_PROCESS(msg) != sctk_get_process_rank())
	{
		ptl_me_t me;
		sctk_portals_msg_header_t *remote_info = &msg->tail.portals;

		memcpy(payload + header_size, msg->tail.portals.payload, payload_size);

		stuff->cat_msg = SCTK_PORTALS_CAT_ROUTING_MSG;
		stuff->extra_data = payload;
	}
	else
	{ // if regular message (need to make specification depending on message type)
		if(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
		{
			memcpy(payload + header_size, msg->tail.message.contiguous.addr, payload_size);
		}
		else
		{
			sctk_net_copy_in_buffer(msg, payload + header_size);
		}
	}

	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg))){
		sctk_debug("PORTALS: EAGER SEND CONTROL_MESSAGE - (%d -> %d) [%lu -> %lu] at (%lu)", SCTK_MSG_SRC_PROCESS(msg),  SCTK_MSG_DEST_PROCESS(msg), prail->current_id.phys.pid,proute->dest.phys.pid, local_entry);
	}
	else {
		sctk_debug("PORTALS: EAGER SEND - (%d -> %d) [%lu -> %lu] at (%lu)", SCTK_MSG_SRC_PROCESS(msg),  SCTK_MSG_DEST_PROCESS(msg), prail->current_id.phys.pid,proute->dest.phys.pid, local_entry);
	}

	//send the header request
	sctk_portals_helper_put_request(
		&prail->ptable.pending_list,
		payload , eager_size, 0,
		&prail->interface_handler,
		proute->dest,
		remote_entry,
		SCTK_PORTALS_BITS_EAGER_SLOT, // match_bits for header put...
		stuff,
		0,
		SCTK_PORTALS_NO_BLOCKING_REQUEST,
		SCTK_PORTALS_ACK_MSG);
}

void sctk_portals_eager_recv_put (sctk_rail_info_t* rail, ptl_event_t* event)
{
	sctk_thread_ptp_message_t* msg = NULL;
	void* payload = NULL;
	ptl_me_t new_me;

	msg = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
	memcpy(msg, event->start, sizeof(sctk_thread_ptp_message_body_t));

	payload = event->start + sizeof(sctk_thread_ptp_message_body_t);

	//store needed data in message tail (forwarded to sctk_message_copy())
	msg->tail.portals.remote = event->initiator;
	msg->tail.portals.remote_index = SCTK_MSG_SRC_TASK(msg) % rail->network.portals.ptable.nb_entries;

	//this field contains ME header from source where data have been tagged
	msg->tail.portals.tag = (ptl_match_bits_t)event->hdr_data;
	msg->tail.portals.handler = &rail->network.portals.interface_handler;
	msg->tail.portals.list = &rail->network.portals.ptable.pending_list;

	msg->tail.portals.payload = sctk_malloc(SCTK_MSG_SIZE(msg));
	memcpy(msg->tail.portals.payload, payload, SCTK_MSG_SIZE(msg));

	//if control message, ajust local entry point for message
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg)))
	{
		sctk_debug("PORTALS: EAGER RECV CONTROL_MESSAGE - (%d -> %d) [%lu -> %lu] at (%lu/%lu)", SCTK_MSG_SRC_PROCESS(msg), SCTK_MSG_DEST_PROCESS(msg),  event->initiator.phys.pid,rail->network.portals.current_id.phys.pid, msg->tail.portals.remote_index, msg->tail.portals.tag);
	}
	else
	{
		sctk_debug("PORTALS: EAGER RECV - (%d -> %d) [%lu -> %lu] at (%lu/%lu)", SCTK_MSG_SRC_PROCESS(msg), SCTK_MSG_DEST_PROCESS(msg),  event->initiator.phys.pid,rail->network.portals.current_id.phys.pid, msg->tail.portals.remote_index, msg->tail.portals.tag);
	}
	//notify upper layer that a message is arrived
	SCTK_MSG_COMPLETION_FLAG_SET(msg, NULL);
	sctk_rebuild_header(msg);
	sctk_reinit_header(msg, sctk_portals_eager_free, sctk_portals_eager_message_copy);

	//adding a new ME to replace the consumed one
	sctk_portals_helper_init_new_entry(
		&new_me, &rail->network.portals.interface_handler,
		(void*)event->start,
		rail->network.portals.ptable.eager_limit,
		SCTK_PORTALS_BITS_EAGER_SLOT,
		SCTK_PORTALS_ME_PUT_OPTIONS);

	sctk_portals_helper_register_new_entry(
		&rail->network.portals.interface_handler,
		event->pt_index,
		&new_me,
		NULL);

    rail->send_message_from_network(msg);
}
