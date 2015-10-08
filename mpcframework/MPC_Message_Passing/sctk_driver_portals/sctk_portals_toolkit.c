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
/* #   - GONCALVES Thomas thomas.goncalves@cea.fr                         # */
/* #   - ADAM Julien adamj@paratools.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS

#include <sctk_route.h>
#include <sctk_portals_toolkit.h>
#include <sctk_pmi.h>
#include <sctk_control_messages.h>

#define sctk_min(a, b)  ((a) < (b) ? (a) : (b))

/**
 * @brief Portals handler for getting a message
 *
 * Rendezvous protocol w/ Portals implementation allows to make data transfer only when user buffer
 * is ready, avoiding useless recopies. sctk_portals_message_copy get back the data from the sender and copies them into
 * user buffer directly
 *
 * @param tmp sender/recever ptp messages
 */
void sctk_portals_message_copy ( sctk_message_to_copy_t *tmp )
{
	sctk_thread_ptp_message_t* sender;
	sctk_thread_ptp_message_t* recver;
	sctk_portals_msg_header_t* remote_info;
	sctk_portals_request_type_t type = SCTK_PORTALS_NO_BLOCKING_REQUEST;
	sender = tmp->msg_send;
	recver = tmp->msg_recv;
	remote_info = &sender->tail.portals;
	sctk_portals_list_entry_extra_t stuff;
	size_t size;
	stuff.cat_msg = SCTK_PORTALS_CAT_REGULAR;
	stuff.extra_data = recver;

	size = sctk_min(SCTK_MSG_SIZE(recver), recver->tail.message.contiguous.size );

	//if msg is control_message, wait after Get (implementation dependent)
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(sender)))
	{
		type = SCTK_PORTALS_BLOCKING_REQUEST;
		stuff.cat_msg = SCTK_PORTALS_CAT_CTLMESSAGE;
	}

	// send the get request (need to be specific depending on message type)
	sctk_portals_helper_get_request(
		remote_info->list,
		recver->tail.message.contiguous.addr,
		size,
		0,
		remote_info->handler,
		remote_info->remote,
		remote_info->remote_index,
		remote_info->tag,
		&stuff, // contains message type and pointer to msg (for completion)
		type);
}

void sctk_portals_free ( void *msg )
{
	sctk_free(msg); // free the preallocated thread_ptp_message_body_t
}

/**
 * @brief add a route into the rail with Portals specifications
 *
 * @param dest remote process rank
 * @param id associated ptl_process
 * @param rail current rail
 * @param route_type type of route (STATIC/DYNAMIC)
 * @param state route state (if needed : CONNECTING, CONNECTED...)
 *
 * @return a sctk_endpoint_t pointer on created route
 */
sctk_endpoint_t*  sctk_portals_add_route(int dest, ptl_process_t id, sctk_rail_info_t *rail,
								sctk_route_origin_t route_type, sctk_endpoint_state_t state)
{

	sctk_endpoint_t *new_route;

	new_route = sctk_malloc ( sizeof ( sctk_endpoint_t ) );
	assume(new_route != NULL);

	sctk_endpoint_init(new_route, dest, rail, route_type);
	new_route->data.portals.dest = id;

	if(route_type == ROUTE_ORIGIN_STATIC)
	{
		sctk_rail_add_static_route(rail, dest, new_route);
	}
	else
	{
		sctk_rail_add_dynamic_route(rail, dest, new_route);
	}

	sctk_endpoint_set_state(new_route, state);
	sctk_debug("PORTALS: REGISTER ROUTE - (me -> %d) [%lu -> %lu]", dest, rail->network.portals.current_id.phys.pid, id.phys.pid);

	return new_route;
}

void sctk_portals_send_put ( sctk_endpoint_t *endpoint, sctk_thread_ptp_message_t *msg)
{
	sctk_portals_rail_info_t * prail = &endpoint->rail->network.portals;
	sctk_portals_route_info_t *proute = &endpoint->data.portals;
	void* payload = NULL;
	sctk_portals_list_entry_extra_t * stuff = NULL;
	size_t payload_size = 0;

	ptl_pt_index_t local_entry;
	ptl_pt_index_t remote_entry;
	ptl_match_bits_t match;
	ptl_me_t slot;

	//create associated data
	stuff = sctk_malloc(sizeof(sctk_portals_list_entry_extra_t));
	stuff->cat_msg = SCTK_PORTALS_CAT_REGULAR;
	stuff->extra_data = msg;

	//compute Portals entries
	remote_entry = SCTK_MSG_DEST_TASK(msg) % prail->ptable.nb_entries;
	local_entry = SCTK_MSG_SRC_TASK(msg) % prail->ptable.nb_entries;

	//if control message, tag it as is + shift message queue where ME will be registered
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg)))
	{
		local_entry = prail->ptable.nb_entries + 1;
		stuff->cat_msg = SCTK_PORTALS_CAT_CTLMESSAGE;
	}

	//if routing : apply Get() before sending header
	if(SCTK_MSG_SRC_PROCESS(msg) != sctk_get_process_rank())
	{
		ptl_me_t me;
		sctk_portals_msg_header_t *remote_info = &msg->tail.portals;

		payload = sctk_malloc(SCTK_MSG_SIZE(msg));
		payload_size = SCTK_MSG_SIZE(msg);
		stuff->cat_msg = SCTK_PORTALS_CAT_ROUTING_MSG;
		stuff->extra_data = payload;

		sctk_debug("PORTALS: ROUTING MSG - %lu at (%lu,%lu)", remote_info->remote.phys.pid, remote_info->remote_index, remote_info->tag);
		sctk_portals_helper_get_request(
			&prail->ptable.pending_list,
			payload, payload_size,
			0,
			remote_info->handler,
			remote_info->remote,
			remote_info->remote_index,
			remote_info->tag,
			stuff,
			SCTK_PORTALS_BLOCKING_REQUEST); //BLOCKING to avoid reentrance in this function
	}
	else
	{ // if regular message (need to make specification depending on message type)
		payload = msg->tail.message.contiguous.addr;
		payload_size = msg->tail.message.contiguous.size;
	}

	// setting unique match bits for given ME
	sctk_portals_helper_set_bits_from_msg(&match, &prail->ptable.head[local_entry]->entry_cpt);

	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg))){
		sctk_debug("PORTALS: SEND CONTROL_MESSAGE - (%d -> %d) [%lu -> %lu] at (%lu/%lu)", SCTK_MSG_SRC_PROCESS(msg),  SCTK_MSG_DEST_PROCESS(msg), prail->current_id.phys.pid,proute->dest.phys.pid, local_entry, match);
	}
	else {
		sctk_debug("PORTALS: SEND - (%d -> %d) [%lu -> %lu] at (%lu/%lu)", SCTK_MSG_SRC_PROCESS(msg),  SCTK_MSG_DEST_PROCESS(msg), prail->current_id.phys.pid,proute->dest.phys.pid, local_entry, match);
	}

	//create and register new ME
	sctk_portals_helper_init_new_entry(
		&slot   , &prail->interface_handler,
		payload , payload_size,
		match   , SCTK_PORTALS_ME_GET_OPTIONS);

	sctk_portals_helper_register_new_entry(&prail->interface_handler, local_entry, &slot, stuff);

	//send the header request
	sctk_portals_helper_put_request(
		&prail->ptable.pending_list,
		msg , sizeof(sctk_thread_ptp_message_body_t), 0,
		&prail->interface_handler,
		proute->dest,
		remote_entry,
		SCTK_PORTALS_BITS_HEADER, // match_bits for header put...
		stuff,
		match,
		SCTK_PORTALS_NO_BLOCKING_REQUEST,
		SCTK_PORTALS_NO_ACK_MSG);
}
/**
 * @brief When a message have been sent, the source have to notify and unlock tasks waiting for completion
 *
 * @param rail
 * @param event event triggered by PtlGet from target
 */
void sctk_portals_ack_get (sctk_rail_info_t* rail, ptl_event_t* event)
{
	ptl_me_t new_me;
	ptl_handle_me_t new_me_handle;
	sctk_portals_list_entry_extra_t* stuff = NULL;
	sctk_thread_ptp_message_t* content = NULL;

	//retrieve msg address for completion
	stuff = event->user_ptr;
	content = (sctk_thread_ptp_message_t*)stuff->extra_data;

	sctk_debug("PORTALS: ACK GET MSG - %lu at (%lu,%lu)", event->initiator.phys.pid, event->pt_index, event->match_bits);
	//free message
	sctk_complete_and_free_message(content);

	//adding a new ME to replace the consumed one
	sctk_portals_helper_init_new_entry(
		&new_me, &rail->network.portals.interface_handler,
		(sctk_thread_ptp_message_t*)sctk_malloc(sizeof(sctk_thread_ptp_message_t)),
		sizeof(sctk_thread_ptp_message_t),
		SCTK_PORTALS_BITS_HEADER,
		SCTK_PORTALS_ME_PUT_OPTIONS);

	sctk_portals_helper_register_new_entry(
		&rail->network.portals.interface_handler,
		event->pt_index,
		&new_me,
		NULL);
}

/**
 * @brief RECV handling routing, stores incoming header for accessing it later
 *
 * @param rail
 * @param event
 */
void sctk_portals_recv_put (sctk_rail_info_t* rail, ptl_event_t* event)
{
	sctk_thread_ptp_message_t* content = NULL;

	content = event->start;

	//store needed data in message tail (forwarded to sctk_message_copy())
	content->tail.portals.remote = event->initiator;
	content->tail.portals.remote_index = SCTK_MSG_SRC_TASK(content) % rail->network.portals.ptable.nb_entries;

	//this field contains ME header from source where data have been tagged
	content->tail.portals.tag = (ptl_match_bits_t)event->hdr_data;
	content->tail.portals.handler = &rail->network.portals.interface_handler;
	content->tail.portals.list = &rail->network.portals.ptable.pending_list;

	//if control message, ajust local entry point for message
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(content)))
	{
		content->tail.portals.remote_index = rail->network.portals.ptable.nb_entries + 1;
		sctk_debug("PORTALS: RECV CONTROL_MESSAGE - (%d -> %d) [%lu -> %lu] at (%lu/%lu)", SCTK_MSG_SRC_PROCESS(content), SCTK_MSG_DEST_PROCESS(content),  event->initiator.phys.pid,rail->network.portals.current_id.phys.pid, content->tail.portals.remote_index, content->tail.portals.tag);
	}
	else
	{
		sctk_debug("PORTALS: RECV - (%d -> %d) [%lu -> %lu] at (%lu/%lu)", SCTK_MSG_SRC_PROCESS(content), SCTK_MSG_DEST_PROCESS(content),  event->initiator.phys.pid,rail->network.portals.current_id.phys.pid, content->tail.portals.remote_index, content->tail.portals.tag);
	}
	//notify upper layer that a message is arrived
	SCTK_MSG_COMPLETION_FLAG_SET(content, NULL);
	sctk_rebuild_header(content);
	sctk_reinit_header(content, sctk_portals_free, sctk_portals_message_copy);

	rail->send_message_from_network(content);
}

void sctk_portals_poll_pending_msg_list(sctk_rail_info_t *rail)
{
	sctk_portals_pending_msg_list_t* list = NULL;
	sctk_portals_pending_msg_t* elt = NULL, *tmp = NULL;

	sctk_portals_list_entry_extra_t pending;
	sctk_thread_ptp_message_t* msg = NULL;
	int to_free, ret;
	list = &rail->network.portals.ptable.pending_list;

	//for each registered message :
	LL_FOREACH_SAFE(list->head, elt, tmp)
	{
		ptl_event_t event;
		ret = PtlEQGet(elt->md.eq_handle, &event);
		to_free = 0;
		pending = elt->data;
		msg = NULL;

		//while an avent is contained in current event_queue
		while(ret == PTL_OK)
		{
			assume(event.ni_fail_type == PTL_NI_OK);

			//depending on event type
			switch(event.type)
			{
				case PTL_EVENT_SEND: // data from MD have been sent
					sctk_debug("PORTALS: POLL SEND - %lu (%lu)", rail->network.portals.current_id.phys.pid, pending.cat_msg);
					to_free = (elt->ack_type == SCTK_PORTALS_NO_ACK_MSG);
					break;

				case PTL_EVENT_REPLY: // data have been copied into MD
					sctk_debug("PORTALS: POLL REPLY - %lu (%lu)", rail->network.portals.current_id.phys.pid, pending.cat_msg);
					//depending on message type
					switch(pending.cat_msg)
					{
						case SCTK_PORTALS_CAT_RDMA: // special msg : RDMA
						case SCTK_PORTALS_CAT_REGULAR: // when standard message is copied in userspace
							msg = (sctk_thread_ptp_message_t*) pending.extra_data;
							sctk_complete_and_free_message(msg);
							break;


						case SCTK_PORTALS_CAT_CTLMESSAGE: // when control message is received
						case SCTK_PORTALS_CAT_RESERVED:  // when connection is initialized
						case SCTK_PORTALS_CAT_ROUTING_MSG: //when MD is a routing slot (no action)
							break;
						default:
							not_reachable();
					}
					to_free = 1;
					break;
				case PTL_EVENT_ACK:	//just notify that a Put() have been completed to the target
				sctk_debug("PORTALS: POLL ACK - %lu (%lu)", rail->network.portals.current_id.phys.pid, pending.cat_msg);

					switch(pending.cat_msg)
					{
						case SCTK_PORTALS_CAT_RDMA: // special msg : RDMA
							msg = (sctk_thread_ptp_message_t*) pending.extra_data;
							sctk_complete_and_free_message(msg);
							break;
						case SCTK_PORTALS_CAT_CTLMESSAGE: // when control message is received
						case SCTK_PORTALS_CAT_RESERVED:  // when connection is initialized
						case SCTK_PORTALS_CAT_ROUTING_MSG: //when MD is a routing slot (no action)
							break;
						default:
							not_reachable();
					}
					to_free = 1;
					break;
				default:
					not_reachable();
			}
			ret = PtlEQGet(elt->md.eq_handle, &event);
		}
		if(to_free){
			PtlMDRelease(elt->md_handler);
			LL_DELETE(list->head, elt);
			sctk_free(elt);
		}
	}
}

int sctk_portals_poll_one_queue(sctk_rail_info_t *rail, size_t id)
{
	int ret = 1, to_free = 1;
	ptl_event_t event;
	sctk_portals_table_t* ptable = &rail->network.portals.ptable;
	ptl_handle_eq_t* queue = ptable->head[id]->event_list;
	sctk_portals_list_entry_extra_t* stuff = NULL;
	sctk_portals_slot_category_t cat;

	assume(id < ptable->nb_entries+2);

	//if no other tasks are polling this list
	if(sctk_spinlock_trylock(&ptable->head[id]->lock) == 0)
	{
		//while there are event to poll, continue
		while(PtlEQGet(*queue, &event) == PTL_OK)
		{
			assume(event.ni_fail_type == PTL_NI_OK);
			stuff = event.user_ptr;
			cat = (stuff == NULL) ? SCTK_PORTALS_CAT_REGULAR : stuff->cat_msg;

			//depending on event type
			switch(event.type){
				case PTL_EVENT_GET: // Data in this ME have been accessed
				case PTL_EVENT_GET_OVERFLOW:

					sctk_debug("PORTALS: (REMOTE) POLL GET - %lu (%lu)", rail->network.portals.current_id.phys.pid, cat);

					//depending on messate type
					switch(cat)
					{
						case SCTK_PORTALS_CAT_REGULAR:
						case SCTK_PORTALS_CAT_CTLMESSAGE:
							sctk_portals_ack_get(rail, &event);
							break;

						case SCTK_PORTALS_CAT_RDMA:
							to_free = 0;
							break;
						case SCTK_PORTALS_CAT_ROUTING_MSG:
							sctk_free(stuff->extra_data); // free temporary payload
							break;
						case SCTK_PORTALS_CAT_RESERVED:
							break;
						default:
							CRASH();
							not_reachable();
					}
					break;

				case PTL_EVENT_PUT: // data have been published in this ME
				case PTL_EVENT_PUT_OVERFLOW:
					sctk_debug("PORTALS: (REMOTE) POLL PUT - %lu (%lu)", rail->network.portals.current_id.phys.pid, cat);

					//depending on message type
					switch(cat)
					{
						case SCTK_PORTALS_CAT_REGULAR:
						case SCTK_PORTALS_CAT_CTLMESSAGE:
							sctk_portals_recv_put(rail, &event); // handle the message
							break;
						case SCTK_PORTALS_CAT_RDMA:
							to_free = 0;
							sctk_nodebug("Put for RDMA REQUEST ");
							break;
						case SCTK_PORTALS_CAT_RESERVED:
						case SCTK_PORTALS_CAT_ROUTING_MSG:
							break;
						default:
							not_reachable();
					}
					break;
				case PTL_EVENT_FETCH_ATOMIC:
				case PTL_EVENT_FETCH_ATOMIC_OVERFLOW:
					sctk_debug("PORTALS: (REMOTE) POLL FETCH-ATOMIC - %lu (%lu)", rail->network.portals.current_id.phys.pid, cat);

					break;

				//not used events for now
				case PTL_EVENT_ATOMIC:
				case PTL_EVENT_ATOMIC_OVERFLOW:
				case PTL_EVENT_PT_DISABLED:
				case PTL_EVENT_SEARCH:
					sctk_debug("PORTALS: POLL NOT HANDLED EVENT - %lu (%lu)", rail->network.portals.current_id.phys.pid, cat);

					break;
				default:
					not_reachable();
			}
			// if extra data have been allocated:
			if(stuff && to_free) sctk_free(stuff);
			ret = 0;
		}
		sctk_spinlock_unlock(&ptable->head[id]->lock);
	}

	return ret;
}

int sctk_portals_polling_queue_for(sctk_rail_info_t*rail, size_t task_id)
{
	int ret = 1, ret_bef = 1, ret_aft = 1;
	size_t task_bef = 0, task_aft = 0, mytask = sctk_get_task_rank();
	size_t nb_entries = rail->network.portals.ptable.nb_entries;

	//if the request is to poll every lists
	if(task_id == SCTK_PORTALS_POLL_ALL){
		int i;
		if(sctk_spinlock_trylock(&rail->network.portals.ptable.table_lock) == 0){
			for (i = 0; i < nb_entries; ++i) {
				sctk_portals_poll_one_queue(rail, (mytask+i)%(nb_entries));
			}
			sctk_spinlock_unlock(&rail->network.portals.ptable.table_lock);
		}
		return 0;
	}

	//else, poll my queue
	ret = sctk_portals_poll_one_queue(rail, task_id);

	//if not, poll neighbors
	if(ret){
		task_bef = (task_id - 1) % nb_entries;
		task_aft = (task_id + 1) % nb_entries;

		ret_bef = sctk_portals_poll_one_queue(rail, task_bef);
		ret_aft = sctk_portals_poll_one_queue(rail, task_aft);

		if(ret_bef && ret_aft)
		ret = 1; //no polled queue
		else ret = 0;
	}

	return ret;
}

/**
 * Declared for compatibility
 * Not used by Portals as route creation is bidirectionnal
 */
void sctk_portals_network_connection_to(int from, int to, sctk_rail_info_t* rail, sctk_route_origin_t route_type)
{

}


/**
 * @brief function called when a control message for route initialisation is triggered
 *
 * the received context contains information from initiator process for route initialization
 *
 * @param src initiator process
 * @param rail current rail
 * @param ctx context with data
 * @param route_type which kind of route to init (STATIC/DYNAMIC?)
 */
void sctk_portals_network_connection_to_ctx(int src, sctk_rail_info_t* rail,
					sctk_portals_connection_context_t * ctx, sctk_route_origin_t route_type)
{
	sctk_portals_rail_info_t* prail = &rail->network.portals;
	sctk_portals_list_entry_extra_t stuff;

	sctk_debug("PORTALS: RECV ON-DEMAND - %lu at (%lu,%lu)", ctx->from, ctx->entry, ctx->match);

	//directly add the route
	sctk_portals_add_route(src, ctx->from, rail, route_type, STATE_CONNECTED);

	stuff.cat_msg = SCTK_PORTALS_CAT_RESERVED;
	stuff.extra_data = NULL;
	sctk_portals_helper_put_request(
		&rail->network.portals.ptable.pending_list,
		&prail->current_id,
		sizeof(sctk_portals_process_id_t),
		0,
		&prail->interface_handler,
		ctx->from,
		ctx->entry,
		ctx->match,
		&stuff,
		0,
		SCTK_PORTALS_NO_BLOCKING_REQUEST,
		SCTK_PORTALS_NO_ACK_MSG);

}

/**
 * @brief route initialization request from local process through control messages
 * @param from local process
 * @param to remote process
 * @param rail current Portals rail
 * @param route_type which kind of route to init (STATIC/DYNAMIC)
 */
void sctk_portals_network_connection_from(int from, int to, sctk_rail_info_t* rail,
													sctk_route_origin_t route_type) {
    sctk_portals_connection_context_t ctx;
    sctk_portals_process_id_t slot;
    ptl_ct_event_t ctc;
    ptl_me_t me;
    ptl_handle_me_t me_handler;
	sctk_endpoint_t* route;
	sctk_portals_list_entry_extra_t* stuff = NULL;

	stuff = sctk_malloc(sizeof(sctk_portals_list_entry_extra_t));
	stuff->cat_msg = SCTK_PORTALS_CAT_RESERVED;
	stuff->extra_data = NULL;

	//prepare network context to send to remote process
	ctx.from = rail->network.portals.current_id;
	ctx.entry = rail->network.portals.ptable.nb_entries + 1;

	sctk_portals_helper_set_bits_from_msg(&ctx.match, &rail->network.portals.ptable.head[ctx.entry]->entry_cpt);

	sctk_debug("PORTALS: SEND ON-DEMAND - %lu at (%lu,%lu)", ctx.from, ctx.entry, ctx.match);

	// init ME w/ default
	sctk_portals_helper_init_new_entry(&me, &rail->network.portals.interface_handler, (void*)&slot, sizeof(sctk_portals_process_id_t), ctx.match, SCTK_PORTALS_ME_PUT_OPTIONS);
	sctk_portals_helper_register_new_entry(&rail->network.portals.interface_handler, ctx.entry, &me, stuff);

	//add the route
	route = sctk_portals_add_route(to, slot, rail, route_type, STATE_CONNECTING);

	//depending on creation type, route can be created as a dynamic or static one
	if(route_type == ROUTE_ORIGIN_STATIC)
	{
		sctk_control_messages_send_rail(to,SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC,0, &ctx,sizeof(sctk_portals_connection_context_t), rail->rail_number );
	}
	else
	{
		sctk_control_messages_send_rail(to,SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC, 0, &ctx,sizeof(sctk_portals_connection_context_t), rail->rail_number );
	}

	//waiting for remote data to process
	while(PtlCTGet(me.ct_handle, &ctc) == PTL_OK)
	{
		sctk_debug("PORTALS: SEND ON-DEMAND - %lu at (%lu,%lu) WAITING", ctx.from, ctx.entry, ctx.match);
		assume(ctc.failure == 0);
		if(ctc.success > 0)
			break;
		rail->notify_idle_message(rail);
	}
	route->data.portals.dest = slot;
	sctk_endpoint_set_state(route, STATE_CONNECTED);
	sctk_debug("PORTALS: SEND ON-DEMAND - %lu at (%lu,%lu) COMPLETED", ctx.from, ctx.entry, ctx.match);
}

void sctk_portals_connection_from(int from, int to , sctk_rail_info_t *rail)
{
    sctk_portals_network_connection_from(from, to, rail, ROUTE_ORIGIN_STATIC);
}

void sctk_portals_connection_to(int from, int to , sctk_rail_info_t *rail)
{
    sctk_portals_network_connection_to(from, to, rail, ROUTE_ORIGIN_STATIC);

}

void sctk_portals_on_demand_connection_handler( sctk_rail_info_t *rail, int dest_process )
{
	sctk_portals_network_connection_from(sctk_process_rank, dest_process, rail, ROUTE_ORIGIN_DYNAMIC);
}

void sctk_portals_control_message_handler( sctk_rail_info_t *rail, int src_process, int source_rank, char subtype, char param, void * data, size_t size)
{
    sctk_portals_control_message_type_t action = subtype;
	assume(size == sizeof(sctk_portals_connection_context_t));
    switch(action)
    {
        case SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC:
            sctk_portals_network_connection_to_ctx(src_process, rail, (sctk_portals_connection_context_t *)data, ROUTE_ORIGIN_STATIC);
            break;
        case SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC:
            sctk_portals_network_connection_to_ctx(src_process, rail, (sctk_portals_connection_context_t *)data, ROUTE_ORIGIN_DYNAMIC);
            break;
    }
}

void sctk_network_init_portals_all ( sctk_rail_info_t *rail )
{
    int right_rank;
    int left_rank;
    sctk_portals_process_id_t right_id, left_id;
    char right_rank_connection_infos[MAX_STRING_SIZE];
    char left_rank_connection_infos[MAX_STRING_SIZE];

    rail->connect_from = sctk_portals_connection_from;
    rail->connect_to = sctk_portals_connection_to;
    rail->control_message_handler = sctk_portals_control_message_handler;

	sctk_debug("PORTALS: LIB INITIALIZATION");

	sctk_portals_helper_lib_init(&rail->network.portals.interface_handler, &rail->network.portals.current_id, &rail->network.portals.ptable);

	//if process binding w/ ring is not required, stop here
    if( rail->requires_bootstrap_ring == 0 )
    {
        return;
    }

	/** Portals initialization : Ring topology */
	/**
	 * 1. bind to right process through PMI
	 * 2. if nbProcs > 2 create a binding to the process to the left
	 * => Bidirectional ring
	 */
	// Process registration through PMI
    sctk_portals_helper_to_str( &rail->network.portals.current_id, sizeof (sctk_portals_process_id_t), rail->network.portals.connection_infos,MAX_STRING_SIZE);
	rail->network.portals.connection_infos_size = strlen(rail->network.portals.connection_infos);
    assume(rail->network.portals.connection_infos_size  < MAX_STRING_SIZE);
	assume(sctk_pmi_put_connection_info ( rail->network.portals.connection_infos, MAX_STRING_SIZE, rail->rail_number ) == 0);

	//compute left and right neighbour rank
    right_rank = ( sctk_process_rank + 1 ) % sctk_process_number;
    left_rank = ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;

	//SYNCING : Every process should upload its connection string before binding
    sctk_pmi_barrier();

    sctk_pmi_get_connection_info ( right_rank_connection_infos, MAX_STRING_SIZE, rail->rail_number, right_rank );
    sctk_portals_helper_from_str(right_rank_connection_infos, &right_id, sizeof ( right_id ) );

    sctk_debug("PORTALS: PMI MAPPING - %d(%lu) -> %d(%lu)", sctk_process_rank, rail->network.portals.current_id.phys.pid,  right_rank, right_id.phys.pid);

    sctk_pmi_barrier();
	//Register the right neighbour as a connection and let Portals make the binding

	//if we need an initialization to the left process (bidirectional ring)
    if(sctk_process_number > 2){
        sctk_pmi_get_connection_info ( left_rank_connection_infos, MAX_STRING_SIZE, rail->rail_number, left_rank );

        /*decode portals identification string into nid and pid*/
        sctk_portals_helper_from_str( left_rank_connection_infos, &left_id, sizeof (sctk_portals_process_id_t) );
        // register the left neighbour as a connection and let Portals make the binding
		sctk_debug("PORTALS: PMI MAPPING - %d(%lu) -> %d(%lu)", sctk_process_rank, rail->network.portals.current_id.phys.pid,  right_rank, right_id.phys.pid);
	}

	//Syncing before adding the route
    sctk_pmi_barrier();

	//add routes
    sctk_portals_add_route (right_rank, right_id, rail, ROUTE_ORIGIN_STATIC, STATE_CONNECTED);
    if ( sctk_process_number > 2 )
    {
        sctk_portals_add_route (left_rank, left_id, rail, ROUTE_ORIGIN_STATIC, STATE_CONNECTED);
    }

	//syncing with other processes
    sctk_pmi_barrier();
}
#endif
