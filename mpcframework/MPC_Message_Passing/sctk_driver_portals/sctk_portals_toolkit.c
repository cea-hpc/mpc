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

/**
 * @brief Portals handler for getting a message
 *
 * Rendezvous protocol w/ Portals implementation allows to make data transfer only when user buffer
 * is ready, avoiding useless recopies. sctk_portals_message_copy get back the data from the sender and copies them into
 * user buffer directly
 *
 * @param tmp sender/recever ptp messages
 */
void sctk_portals_message_copy ( sctk_message_to_copy_t *tmp ){
	
	sctk_thread_ptp_message_t* sender;
	sctk_thread_ptp_message_t* recver;
	sctk_portals_msg_header_t* remote_info;

	sender = tmp->msg_send;
	recver = tmp->msg_recv;
	remote_info = &sender->tail.portals;

	sctk_portals_helper_get_request(recver->tail.message.contiguous.addr, SCTK_MSG_SIZE(recver), remote_info->handler, remote_info->remote, remote_info->remote_index, remote_info->tag, NULL);

	//notify upper layer that data are now available
	sctk_complete_and_free_message(recver);
}

/**
 * @brief freeing portals-specific data from ptp_message
 *
 * @param msg
 */
void sctk_portals_free ( void *msg ) //free isn't atomatic because we reuse memory allocated
{
	sctk_free(msg);
}

/**
 * @brief add a route into the rail with Portals specifications
 *
 * @param dest
 * @param id
 * @param rail
 * @param route_type
 */
void sctk_portals_add_route(int dest, ptl_process_t id, sctk_rail_info_t *rail, sctk_route_origin_t route_type ){
	sctk_endpoint_t *new_route;

	new_route = sctk_malloc ( sizeof ( sctk_endpoint_t ) );
	assume(new_route != NULL);
	sctk_endpoint_init(new_route, dest, rail, route_type);
	sctk_endpoint_set_state(new_route, STATE_CONNECTED);

	new_route->data.portals.dest = id;
	if(route_type == ROUTE_ORIGIN_STATIC){
		sctk_rail_add_static_route(rail, dest, new_route);
	} else {
		sctk_rail_add_dynamic_route(rail, dest, new_route);
	}
}


/**
 * @brief Send a given ptp_message to a given route
 *
 * @param endpoint the route
 * @param msg the message
 */
void sctk_portals_send_put ( sctk_endpoint_t *endpoint, sctk_thread_ptp_message_t *msg)
{
	sctk_portals_rail_info_t * prail = &endpoint->rail->network.portals;
	sctk_portals_route_info_t *proute = &endpoint->data.portals;
	void* payload = NULL;
	size_t payload_size = 0;

	ptl_pt_index_t local_entry;
	ptl_pt_index_t remote_entry;
	ptl_match_bits_t match;
	ptl_me_t slot;

	//compute remote Portals entry
	remote_entry = SCTK_MSG_DEST_TASK(msg) % prail->ptable.nb_entries;
	local_entry = SCTK_MSG_SRC_TASK(msg) % prail->ptable.nb_entries;
	
	//if routing : apply Get() before sending header
	if(SCTK_MSG_SRC_PROCESS(msg) != sctk_get_process_rank()){
		ptl_me_t me;
		payload = sctk_malloc(sizeof(SCTK_MSG_SIZE(msg)));
		payload_size = SCTK_MSG_SIZE(msg);
		sctk_portals_msg_header_t *remote_info = &msg->tail.portals;
		sctk_debug("PORTALS: ROUTING - GET FROM %lu, %d, %d", remote_info->remote.phys.pid, remote_info->remote_index, remote_info->tag);
		sctk_portals_helper_get_request(payload, SCTK_MSG_SIZE(msg), remote_info->handler, remote_info->remote, remote_info->remote_index, remote_info->tag, NULL);
	}
	else {
		payload = msg->tail.message.contiguous.addr;
		payload_size = msg->tail.message.contiguous.size;
	}

	sctk_portals_helper_set_bits_from_msg(&match, &prail->ptable.head[local_entry]->entry_cpt);
	
	//if control message, set the good entry to match
	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(msg))){
		local_entry = prail->ptable.nb_entries - 1;
		sctk_debug("PORTALS: SEND CONTROL_MESSAGE TO   (%d -> %d) [%lu -> %lu] - %lu - %lu", SCTK_MSG_SRC_PROCESS(msg),  SCTK_MSG_DEST_PROCESS(msg), prail->current_id.phys.pid,proute->dest.phys.pid, local_entry, match);
	}
	else {
		sctk_debug("PORTALS: SEND TO   (%d -> %d) [%lu -> %lu] - %lu - %lu", SCTK_MSG_SRC_PROCESS(msg),  SCTK_MSG_DEST_PROCESS(msg), prail->current_id.phys.pid,proute->dest.phys.pid, local_entry, match);
	}

	sctk_portals_helper_init_new_entry(&slot, &prail->interface_handler, payload, payload_size, match, SCTK_PORTALS_ME_GET_OPTIONS);
	sctk_portals_helper_register_new_entry(&prail->interface_handler, local_entry, &slot, msg);
	sctk_portals_helper_put_request(msg, sizeof(sctk_thread_ptp_message_body_t), &prail->interface_handler, proute->dest, remote_entry, SCTK_PORTALS_BITS_HEADER, NULL, match);	
}
/**
 * @brief When a message have been sent, the source have to notify and unlock tasks waiting for completion
 *
 * @param rail
 * @param event event triggered by PtlGet from target
 */
void sctk_portals_ack_get (sctk_rail_info_t* rail, ptl_event_t* event){
	ptl_me_t new_me;
	ptl_handle_me_t new_me_handle;

	//retrieve msg address for completion
	sctk_thread_ptp_message_t* content = (sctk_thread_ptp_message_t*)event->user_ptr;
	sctk_complete_and_free_message(content);

	//adding a new ME to replace the consumed one
	sctk_portals_helper_init_new_entry(&new_me, &rail->network.portals.interface_handler, (sctk_thread_ptp_message_t*)sctk_malloc(sizeof(sctk_thread_ptp_message_t)), sizeof(sctk_thread_ptp_message_t), SCTK_PORTALS_BITS_HEADER, SCTK_PORTALS_ME_PUT_OPTIONS);
	sctk_portals_helper_register_new_entry(&rail->network.portals.interface_handler, event->pt_index, &new_me, NULL);
}

/**
 * @brief RECV handling routing, stores incoming header for accessing it later
 *
 * @param rail
 * @param event
 */
void sctk_portals_recv_put (sctk_rail_info_t* rail, ptl_event_t* event){
	sctk_thread_ptp_message_t* content = event->start;

	//store needed data in message tail (forwarded to sctk_message_copy())
	content->tail.portals.remote = event->initiator;
	content->tail.portals.remote_index = SCTK_MSG_SRC_TASK(content) % rail->network.portals.ptable.nb_entries;
	//this field contains ME header from source where data have been tagged
	content->tail.portals.tag = (ptl_match_bits_t)event->hdr_data;
	content->tail.portals.handler = &rail->network.portals.interface_handler;

	if(sctk_message_class_is_control_message(SCTK_MSG_SPECIFIC_CLASS(content))){
		content->tail.portals.remote_index = rail->network.portals.ptable.nb_entries - 1;
		sctk_debug("PORTALS: RECV CONTROL_MESSAGE FROM (%d -> %d) [%lu -> %lu] %lu - %lu", SCTK_MSG_SRC_PROCESS(content), SCTK_MSG_DEST_PROCESS(content),  event->initiator.phys.pid,rail->network.portals.current_id.phys.pid, content->tail.portals.remote_index, content->tail.portals.tag);
	}
	else {
		sctk_debug("PORTALS: RECV FROM (%d -> %d) [%lu -> %lu] %lu - %lu", SCTK_MSG_SRC_PROCESS(content), SCTK_MSG_DEST_PROCESS(content), rail->network.portals.current_id.phys.pid, event->initiator.phys.pid, content->tail.portals.remote_index, content->tail.portals.tag);
	}

	//notify upper layer that a message is arrived
	sctk_rebuild_header(content);
	sctk_reinit_header(content, sctk_portals_free, sctk_portals_message_copy);
	SCTK_MSG_COMPLETION_FLAG_SET(content, NULL);
	rail->send_message_from_network(content);
}

/**
 * @brief Poll incoming events in a queue for the given id
 *
 * @param rail
 * @param id
 *
 * @return 1 if polling have been useful, 0 otherwise
 */
static int sctk_portals_poll_one_queue(sctk_rail_info_t *rail, size_t id)
{
	int ret = 1;
	ptl_event_t event;
	sctk_portals_table_t* ptable = &rail->network.portals.ptable;
	ptl_handle_eq_t* queue = ptable->head[id]->event_list;
	
	//if no other tasks are polling this list
	if(sctk_spinlock_trylock(&ptable->head[id]->lock) == 0){
		//while there are event to poll, continue
		while(PtlEQGet(*queue, &event) == PTL_OK){
			//if it is for data transfer => release resources and complete and free message
			switch(event.type){
				case PTL_EVENT_GET:
					sctk_portals_ack_get(rail, &event);
					break;
				case PTL_EVENT_PUT: //handle recv event only for incoming header
					switch(event.match_bits){
						case SCTK_PORTALS_BITS_EAGER_SLOT:
						case SCTK_PORTALS_BITS_OPEN_CONNECTION:
							break;
						default:
							sctk_portals_recv_put(rail, &event);
							break;
					}

					break;
				default:
					break;
			}

		}
		sctk_spinlock_unlock(&ptable->head[id]->lock);
	}
	
	return ret;
}

/**
 * @brief try to poll incoming message for current task and neighbors 
 *
 * @param rail
 * @param task_id
 *
 * @return 
 */
int sctk_portals_polling_queue_for(sctk_rail_info_t*rail, size_t task_id){
	int ret = 0, ret_bef = 0, ret_aft = 0;
	size_t task_bef = 0, task_aft = 0, mytask = sctk_get_task_rank();
	size_t nb_entries = rail->network.portals.ptable.nb_entries;

	//if the request is to poll every lists
	if(task_id == SCTK_PORTALS_POLL_ALL){
		int i;
		if(sctk_spinlock_trylock(&rail->network.portals.ptable.table_lock) == 0){
			for (i = 0; i < nb_entries; ++i) {
				sctk_portals_poll_one_queue(rail, (mytask+i)%nb_entries);
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
	}
	if(ret_bef || ret_aft)
		ret = 1; //no polled queue
	else ret = 0;

	return ret;
}


static void sctk_portals_network_connection_to(int from, int to, sctk_rail_info_t* rail, sctk_route_origin_t route_type)
{

}

static void sctk_portals_network_connection_to_ctx(int src, sctk_rail_info_t* rail,sctk_portals_connection_context_t * ctx, sctk_route_origin_t route_type)
{
	sctk_portals_rail_info_t* prail = &rail->network.portals;
	sctk_debug("PORTALS: ON DEMAND TO   %lu - %lu", ctx->from.phys.pid, ctx->entry);
	sctk_portals_helper_put_request(&prail->current_id, sizeof(sctk_portals_process_id_t), &prail->interface_handler, ctx->from, ctx->entry, SCTK_PORTALS_BITS_OPEN_CONNECTION, NULL, 0);

	//adding route to remote process
	sctk_portals_add_route(src, ctx->from, rail, route_type);
}

static void sctk_portals_network_connection_from(int from, int to, sctk_rail_info_t* rail, sctk_route_origin_t route_type) {
    sctk_portals_connection_context_t ctx;
    sctk_portals_process_id_t slot;
    ptl_ct_event_t ctc;
    ptl_me_t me;
    ptl_handle_me_t me_handler;

	//prepare network context to send to remote process
    ctx.from = rail->network.portals.current_id;
    ctx.entry = rail->network.portals.ptable.nb_entries - 1;
	sctk_debug("PORTALS: ON DEMAND FROM %lu - %lu", ctx.from.phys.pid, ctx.entry);
	
	// init ME w/ default
	sctk_portals_helper_init_new_entry(&me, &rail->network.portals.interface_handler, (void*)&slot, sizeof(sctk_portals_process_id_t), SCTK_PORTALS_BITS_OPEN_CONNECTION, SCTK_PORTALS_ME_PUT_OPTIONS);
	sctk_portals_helper_register_new_entry(&rail->network.portals.interface_handler, ctx.entry, &me, NULL);

    
	//depending on creation type, route can be created as a dynamic or static one
	if(route_type == ROUTE_ORIGIN_STATIC){
        sctk_control_messages_send_rail(to,SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC,0, &ctx,sizeof(sctk_portals_connection_context_t), rail->rail_number );
    } else {
        sctk_control_messages_send_rail(to,SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC, 0, &ctx,sizeof(sctk_portals_connection_context_t), rail->rail_number ); 
    }
    //wait remote to publish
	//wait for a message to arrive. When occurs ME counter is incremented.
    sctk_portals_assume(PtlCTWait(me.ct_handle, 1, &ctc));
	assume(ctc.failure == 0);
    //Finally, add a route to this process
   sctk_portals_add_route(to, slot, rail, route_type);
}

static void sctk_portals_connection_from(int from, int to , sctk_rail_info_t *rail){
    sctk_portals_network_connection_from(from, to, rail, ROUTE_ORIGIN_STATIC);
}
static void sctk_portals_connection_to(int from, int to , sctk_rail_info_t *rail){
    sctk_portals_network_connection_to(from, to, rail, ROUTE_ORIGIN_STATIC);

}
void sctk_portals_on_demand_connection_handler( sctk_rail_info_t *rail, int dest_process )
{
    sctk_portals_network_connection_from(sctk_process_rank, dest_process, rail, ROUTE_ORIGIN_DYNAMIC);
}

void sctk_portals_control_message_handler( sctk_rail_info_t *rail, int src_process, int source_rank, char subtype, char param, void * data, size_t size) {
    sctk_portals_control_message_t action = subtype;
	assume(size == sizeof(sctk_portals_connection_context_t));
    switch(action){
        case SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC:
            sctk_portals_network_connection_to_ctx(source_rank, rail, (sctk_portals_connection_context_t *)data, ROUTE_ORIGIN_STATIC);
            break;
        case SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC:
            sctk_portals_network_connection_to_ctx(source_rank, rail, (sctk_portals_connection_context_t *)data, ROUTE_ORIGIN_DYNAMIC);
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

   	sctk_debug("PORTALS: Bind %d = %lu", sctk_get_process_rank(), rail->network.portals.current_id.phys.pid); 
	//compute left and right neighbour rank
    right_rank = ( sctk_process_rank + 1 ) % sctk_process_number;
    left_rank = ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;

	//SYNCING : Every process should upload its connection string before binding
    sctk_pmi_barrier();

    sctk_pmi_get_connection_info ( right_rank_connection_infos, MAX_STRING_SIZE, rail->rail_number, right_rank );
    sctk_portals_helper_from_str(right_rank_connection_infos, &right_id, sizeof ( right_id ) );
	/*sctk_debug ( "Got id %lu\t%lu", right_id.phys.nid, right_id.phys.pid );	*/

    sctk_pmi_barrier();
	//Register the right neighbour as a connection and let Portals make the binding
	/*sctk_portals_helper_bind_to(&rail->network.portals.interface_handler, right_id);*/
    sctk_debug ( "PORTALS: Bind %d(%lu) -> %d(%lu)", sctk_process_rank, rail->network.portals.current_id.phys.pid,  right_rank, right_id.phys.pid);

	//if we need an initialization to the left process (bidirectional ring)
    if(sctk_process_number > 2){
        sctk_pmi_get_connection_info ( left_rank_connection_infos, MAX_STRING_SIZE, rail->rail_number, left_rank );
        /*sctk_debug ( "Got KEY %s from %d", right_rank_connection_infos, right_rank);*/

        /*decode portals identification string into nid and pid*/
        sctk_portals_helper_from_str( left_rank_connection_infos, &left_id, sizeof (sctk_portals_process_id_t) );
        /*sctk_debug ( "Got id %lu\t%lu", left_id.phys.nid, left_id.phys.pid );	*/
		// register the left neighbour as a connection and let Portals make the binding
        /*sctk_portals_helper_bind_to(&rail->network.portals.interface_handler, left_id);*/
    	sctk_debug ( "PORTALS: Bind %d(%lu) -> %d(%lu)", sctk_process_rank, rail->network.portals.current_id.phys.pid,  left_rank, left_id.phys.pid);
    }
   
	//Syncing before adding the route
    sctk_pmi_barrier();

	//add routes
    sctk_portals_add_route (right_rank, right_id, rail, ROUTE_ORIGIN_STATIC);
    if ( sctk_process_number > 2 )
    {
        sctk_portals_add_route (left_rank, left_id, rail, ROUTE_ORIGIN_STATIC);
    }
   
	//syncing with other processes
    sctk_pmi_barrier();

}
#endif
