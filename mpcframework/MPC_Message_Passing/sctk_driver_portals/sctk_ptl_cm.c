#ifdef MPC_USE_PORTALS

#include "sctk_route.h"
#include "sctk_ptl_cm.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_types.h"
#include "sctk_net_tools.h"

/**
 * Free the network-originated 'send' msg.
 */
void sctk_ptl_cm_free_memory(void* msg)
{
	sctk_free(msg);
}

/**
 * Complete the CM request.
 * By construction, a recv CM is generated when one reached the process, to stay
 * compliant with the inter_thread_comm matching process.
 * message_copy() just have to memcpy() the received data to the emulated recv.
 */
void sctk_ptl_cm_message_copy(sctk_message_to_copy_t* msg)
{
	/* dirty line: get back the ME address, where data are located 
	 * + complete and free the message.
	 */
	sctk_net_message_copy_from_buffer(msg->msg_send->tail.ptl.user_ptr->slot.me.start, msg, 1);
}

/**
 * Routine called when a new CM arrived from the network.
 * \param[in] rail the Portals rail
 * \param[in] ev the polled Portals event
 */
static inline void sctk_ptl_cm_recv_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_thread_ptp_message_t* net_msg = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
	sctk_ptl_matchbits_t match         = (sctk_ptl_matchbits_t) ev.match_bits;
	sctk_ptl_imm_data_t hdr ;

	/* sanity checks */
	sctk_assert(rail);
	sctk_assert(ev.ni_fail_type == PTL_NI_OK);
	sctk_assert(ev.mlength <= rail->network.ptl.eager_limit);
	sctk_assert(ev.type == PTL_EVENT_PUT);
	
	/* rebuild a complete MPC header msg (inter_thread_comm needs it) */
	sctk_init_header(net_msg, SCTK_MESSAGE_CONTIGUOUS , sctk_ptl_cm_free_memory, sctk_ptl_cm_message_copy);
	SCTK_MSG_SRC_PROCESS_SET     ( net_msg ,  match.data.rank);
	SCTK_MSG_SRC_TASK_SET        ( net_msg ,  match.data.rank);
	SCTK_MSG_DEST_PROCESS_SET    ( net_msg ,  sctk_get_process_rank());
	SCTK_MSG_DEST_TASK_SET       ( net_msg ,  -1);
	SCTK_MSG_COMMUNICATOR_SET    ( net_msg ,  0);
	SCTK_MSG_TAG_SET             ( net_msg ,  match.data.tag);
	SCTK_MSG_NUMBER_SET          ( net_msg ,  match.data.uid);
	SCTK_MSG_MATCH_SET           ( net_msg ,  0);
	SCTK_MSG_SIZE_SET            ( net_msg ,  ev.mlength);
	SCTK_MSG_COMPLETION_FLAG_SET ( net_msg ,  NULL);

	/* de-serialise hdr_data */
	hdr                                       = (sctk_ptl_imm_data_t)ev.hdr_data;
	net_msg->body.header.message_type.type    = hdr.cm.type;
	net_msg->body.header.message_type.subtype = hdr.cm.subtype;
	net_msg->body.header.message_type.param   = hdr.cm.param;
	net_msg->body.header.message_type.rail_id = hdr.cm.rail_id;

	/* save the Portals context in the tail */
	net_msg->tail.ptl.user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	net_msg->tail.ptl.user_ptr->slot.me.start = ev.start;

	/* finish creating an MPC message header */
	sctk_rebuild_header(net_msg);

	sctk_nodebug("PORTALS: RECV-CM from %d (idx=%d, match=%s, size=%lu) -> %p", SCTK_MSG_SRC_TASK(net_msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, match.raw), ev.mlength, ev.start);

	/* notify ther inter_thread_comm a new message has arrived */
	rail->send_message_from_network(net_msg);
}

/**
 * Send a CM.
 * \param[in] msg the message to send.
 * \param[in] endpoint the route to use.
 */
void sctk_ptl_cm_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	sctk_ptl_local_data_t* request = NULL;
	sctk_ptl_rail_info_t* srail    = &endpoint->rail->network.ptl;
	void* start                    = NULL;
	size_t size                    = 0;
	int flags                      = 0;
	sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
	sctk_ptl_pte_t* pte            = NULL;
	sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
	sctk_ptl_route_info_t* infos   = &endpoint->data.ptl;
	sctk_ptl_imm_data_t hdr;

	/* CM are always contiguous `*/
	sctk_assert(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS);

	/* configure the MD */
	start           = msg->tail.message.contiguous.addr;
	size            = SCTK_MSG_SIZE(msg);
	flags           = SCTK_PTL_MD_PUT_FLAGS;
	match.data.rank = SCTK_MSG_SRC_PROCESS(msg);
	match.data.tag  = SCTK_MSG_TAG(msg);
	match.data.uid  = SCTK_MSG_NUMBER(msg);
	remote          = infos->dest;
	pte             = MPCHT_get(&srail->pt_table, SCTK_PTL_PTE_CM);
	request         = sctk_ptl_md_create(srail, start, size, flags);
	
	sctk_assert(pte);
	sctk_assert(request);
	
	/* double-linking */
	request->msg           = msg;
	msg->tail.ptl.user_ptr = request;
	request->pt_idx        = pte->idx;

	/* populate immediate data with CM-specific */
	hdr.cm.type            = msg->body.header.message_type.type;
	hdr.cm.subtype         = msg->body.header.message_type.subtype;
	hdr.cm.param           = msg->body.header.message_type.param;
	hdr.cm.rail_id         = msg->body.header.message_type.rail_id;

	sctk_ptl_md_register(srail, request);
	sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, hdr.raw, request);
	
	sctk_nodebug("PORTALS: SEND-CM to %d (idx=%d, match=%s, addr=%p, sz=%llu)", SCTK_MSG_DEST_PROCESS(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), start, size);
}

void sctk_ptl_cm_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_pte_t fake;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	switch(ev.type)
	{
		case PTL_EVENT_PUT: /* a Put() reached the local process */
			fake = (sctk_ptl_pte_t){.idx = ev.pt_index};
			sctk_ptl_me_feed(srail,  &fake,  srail->eager_limit, 1, SCTK_PTL_PRIORITY_LIST);
			sctk_ptl_cm_recv_message(rail, ev);
			break;

		case PTL_EVENT_GET: /* a Get() reached the local process */
		case PTL_EVENT_PUT_OVERFLOW: /* a previous received PUT matched a just appended ME */
		case PTL_EVENT_GET_OVERFLOW: /* a previous received GET matched a just appended ME */
		case PTL_EVENT_ATOMIC: /* an Atomic() reached the local process */
		case PTL_EVENT_ATOMIC_OVERFLOW: /* a previously received ATOMIC matched a just appended one */
		case PTL_EVENT_FETCH_ATOMIC: /* a FetchAtomic() reached the local process */
		case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
		case PTL_EVENT_PT_DISABLED: /* ERROR: The local PTE is disabeld (FLOW_CTRL) */
		case PTL_EVENT_SEARCH: /* a PtlMESearch completed */
			/* probably nothing to do here */
		case PTL_EVENT_LINK: /* MISC: A new ME has been linked, (maybe not useful) */
		case PTL_EVENT_AUTO_UNLINK: /* an USE_ONCE ME has been automatically unlinked */
		case PTL_EVENT_AUTO_FREE: /* an USE_ONCE ME can be now reused */
			not_reachable(); /* have been disabled */
			break;
		default:
			sctk_fatal("Portals ME event not recognized: %d", ev.type);
			break;
	}

}

void sctk_ptl_cm_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	sctk_thread_ptp_message_t* msg = (sctk_thread_ptp_message_t*)user_ptr->msg;
	switch(ev.type)
	{
		case PTL_EVENT_ACK:
			sctk_complete_and_free_message(msg);
			sctk_ptl_md_release(user_ptr);
			break;

		case PTL_EVENT_REPLY:
		case PTL_EVENT_SEND:
			not_reachable();
		default:
			sctk_fatal("Unrecognized MD event: %d", ev.type);
			break;

	}

}
#endif
