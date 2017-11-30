#ifdef MPC_USE_PORTALS
#include "sctk_route.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_iface.h"
#include "sctk_net_tools.h"

static inline sctk_ptl_local_data_t* sctk_ptl_rdv_create_triggerGet(sctk_thread_ptp_message_t* msg, sctk_ptl_rail_info_t* srail, sctk_ptl_id_t remote, sctk_ptl_matchbits_t match, sctk_ptl_cnth_t trig )
{
	void* start = NULL;
	size_t size;
	int flags;
	sctk_ptl_pte_t *pte;
	sctk_ptl_local_data_t* request;
	

	/* is the message contiguous ? Maybe we could try to get an IOVEC instead of packing ? */
	if(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
	{
		start = msg->tail.message.contiguous.addr;
	}
	else
	{
		/* if not, we map a temporary buffer, ready for network serialized data */
		start = sctk_malloc(SCTK_MSG_SIZE(msg));
		/* and we flag the copy to 1, to remember some buffers will have to be recopied */
		msg->tail.ptl.copy = 1;
	}

	/* configure the MD */
	pte                    = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg));
	size                   = SCTK_MSG_SIZE(msg);
	flags                  = SCTK_PTL_MD_GET_FLAGS;
	request                = sctk_ptl_md_create(srail, start, size, flags);
	sctk_assert(pte);
	sctk_assert(request);
	request->msg           = msg;
	request->list          = SCTK_PTL_PRIORITY_LIST;
	msg->tail.ptl.user_ptr = request; /* exception: we store the GET-MD here (having the net buffer) */
	request->type = SCTK_PTL_TYPE_STD;
	request->prot = SCTK_PTL_PROT_RDV;
	request->match = match;
	request->cth   = trig;
	
	sctk_ptl_md_register(srail, request);

	sctk_ptl_emit_triggeredGet(
			request,
			size,
			remote,
			pte,
			match,
			0,
			0,
			request,
			trig,
			1
			);
	sctk_debug("TRIGGERED to %d/%d, match=%s, pte=%d, sz=%llu", remote.phys.nid, remote.phys.pid, __sctk_ptl_match_str(sctk_malloc(32), 32, match.raw), pte->idx, size);
	return request;
}

/**
 * Function to release the allocated message from the network.
 */
void sctk_ptl_rdv_free_memory(void* msg)
{
	sctk_free(msg);
	/*not_implemented();*/
}

/** What to do when a network message matched a locally posted request ?.
 *  
 *  \param[in,out] msg the send/recv bundle where both message headers are stored
 */
void sctk_ptl_rdv_message_copy(sctk_message_to_copy_t* msg)
{
	/* in this very-specific case (RDV), no contiguous data should be copied.
	 * By construction, data moves by zero-(re)copy.
	 * The function just has to complete_and_free messages & free tmp buffers
	 */

	/* if recv buffer has been replaced by a tmp buffer to get the data
	 * as CONTIGUOUS, we recopy the payload in user buffer through MPC unpack call
	 */
	if(msg->msg_recv->tail.ptl.copy)
	{
		sctk_ptl_local_data_t* send_data = msg->msg_send->tail.ptl.user_ptr;
		sctk_ptl_local_data_t* recv_data = msg->msg_recv->tail.ptl.user_ptr;

		sctk_net_message_copy_from_buffer(send_data->slot.me.start, msg, 0);
		sctk_ptl_me_free(recv_data, 1);
	}

	sctk_complete_and_free_message(msg->msg_send);
	sctk_complete_and_free_message(msg->msg_recv);
}

/**
 * What to do with a network incoming message ?.
 * 
 * Note that this function is called when a PUT (expected or not) is received.
 * This is called by the ME polling -> target side
 * For GET-REPLY, \see sctk_ptl_rdv_reply_message.
 *
 * \param[in] rail the Portails rail
 * \param[in] ev the generated PUT* event
 */
static inline void sctk_ptl_rdv_recv_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* ptr = (sctk_ptl_local_data_t*) ev.user_ptr;
	sctk_thread_ptp_message_t* msg = (sctk_thread_ptp_message_t*) ptr->msg;
	sctk_ptl_cnth_t cnth = ptr->slot.me.ct_handle;

	sctk_assert(msg);

	/* As usual, two cases can reach this function:
	 *   A. PUT in PRIORITY -> early-recv
	 *   B. PUT in OVERFLOW -> late-recv
	 *
	 * Independently from these cases, we have to consider two things about GET:
	 *   1. If the remote ID was known (not ANY_SOURCE), a TriggeredGet has been generated.
	 *   2. If the remote ID was not known (ANY_SOURCE), no GET op has been requested.
	 *
	 * Thus, This function should resolve these different scenarios:
	 *   (A-1) ''NOTHING TO DO'', the PUT reached the PRIORITY_LIST & GET ops should be triggered by itself
	 *   (B-1) We just have to call PtlCTInc() on the ME CT event, to trigger the GET manually. It is 
	 *         because the match in OVERFLOW_LIST did not reach the planned ME (to be sure, check the 
	 *         ct_handle.success field, to ensure the GET did not trigger).
	 *   (*-2) Whatever the list the request matches, this function will have to emit the GET op. Maybe
	 *         to avoid code duplication, we could publish a TriggeredGet on the previous ct_handle and
	 *         use the same 'if' than for (B-1) to trigger the operation.
	 */

	/* scenario (A-1) */
	if(ev.type == PTL_EVENT_PUT && SCTK_MSG_SRC_PROCESS(msg) != SCTK_ANY_SOURCE && 0)
	{
		return;
	}

	/* scenario (A/B-2) */
	if(SCTK_MSG_SRC_PROCESS(msg) == SCTK_ANY_SOURCE || 1)
	{
		sctk_ptl_local_data_t* get_request;
		sctk_ptl_pte_t* pte = NULL;
		sctk_ptl_rail_info_t* srail = &rail->network.ptl;
		
		/* create the GET op */
		pte = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg));
		get_request = sctk_ptl_rdv_create_triggerGet(
			msg,
			srail,
			ev.initiator,
			(sctk_ptl_matchbits_t)ev.match_bits,
			cnth
		);
		sctk_assert(get_request);
		sctk_assert(pte);
	}

	/* scenario (B-1) + (*-2), just inc the ct_handle (should be wrapped by the interface) */
	sctk_ptl_chk(PtlCTInc(
		cnth,
		(sctk_ptl_cnt_t) {.success=1, .failure=0}
	));

	sctk_debug("PORTALS: RECV-RDV to %d (idx=%d, match=%s, sz=%llu)", SCTK_MSG_DEST_PROCESS(msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, ev.match_bits), ev.mlength);
}

/**
 * What to do when a REPLY event has been generated by the local request ?.
 *
 * It means that the GET completed, we need to complete_and_free the msg.
 * This is called by the MD polling -> target-side
 *
 * \param[in] rail the Portails rail
 * \param[in] ev the generated PUT* event
 */
static inline void sctk_ptl_rdv_reply_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_thread_ptp_message_t* net_msg = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
	sctk_ptl_local_data_t* ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	sctk_thread_ptp_message_t* recv_msg = (sctk_thread_ptp_message_t*)ptr->msg;

	/* sanity checks */
	sctk_assert(rail);
	sctk_assert(ev.ni_fail_type == PTL_NI_OK);

	/* Free the counter used as a trigger */
	sctk_ptl_ct_free(ptr->cth);
	
	/* rebuild a complete MPC header msg (inter_thread_comm needs it) */
	sctk_init_header(net_msg, SCTK_MESSAGE_CONTIGUOUS , sctk_ptl_rdv_free_memory, sctk_ptl_rdv_message_copy);
	SCTK_MSG_SRC_PROCESS_SET     ( net_msg ,  ptr->match.data.rank);
	SCTK_MSG_SRC_TASK_SET        ( net_msg ,  ptr->match.data.rank);
	SCTK_MSG_DEST_PROCESS_SET    ( net_msg ,  sctk_get_process_rank());
	SCTK_MSG_DEST_TASK_SET       ( net_msg ,  sctk_get_process_rank());
	SCTK_MSG_COMMUNICATOR_SET    ( net_msg ,  SCTK_MSG_COMMUNICATOR(recv_msg));
	SCTK_MSG_TAG_SET             ( net_msg ,  ptr->match.data.tag);
	SCTK_MSG_NUMBER_SET          ( net_msg ,  ptr->match.data.uid);
	SCTK_MSG_MATCH_SET           ( net_msg ,  0);
	SCTK_MSG_SPECIFIC_CLASS_SET  ( net_msg ,  SCTK_MSG_SPECIFIC_CLASS(recv_msg));
	SCTK_MSG_SIZE_SET            ( net_msg ,  ev.mlength);
	SCTK_MSG_COMPLETION_FLAG_SET ( net_msg ,  NULL);

	/* save the Portals context in the tail
	 * Whatever the origin, the user_ptr here is the one attached to the PRIORITY one
	 */
	net_msg->tail.ptl.user_ptr = ptr;
	net_msg->tail.ptl.copy = 0;
	
	/* finish creating an MPC message header */
	sctk_rebuild_header(net_msg);

	sctk_debug("PORTALS: REPLY-RDV from %d (match=%s, rsize=%llu, size=%llu) -> %p", SCTK_MSG_SRC_TASK(net_msg), __sctk_ptl_match_str(malloc(32), 32, ptr->match.raw),ptr->slot.md.length , ev.mlength, ptr->slot.md.start);

	/* notify ther inter_thread_comm a new message has arrived */
	rail->send_message_from_network(net_msg);
}

/**
 * What to do when a remote process retrieved the data ?.
 *
 * We now have to complete_and_free the message and free the potential tmp buffer (CONTIGUOUS)
 * This is called by the ME polling -> initiator-side
 *
 * \param[in] rail the Portals rail
 * \param[in] ev the generated event
 */
static inline void sctk_ptl_rdv_complete_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* ptr = (sctk_ptl_local_data_t*) ev.user_ptr;
	sctk_thread_ptp_message_t *msg = (sctk_thread_ptp_message_t*)ptr->msg;

	sctk_debug("PORTALS: COMPLETE-RDV to %d (idx=%d, match=%s, rsize=%llu, size=%llu) -> %p", SCTK_MSG_SRC_PROCESS(msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, ev.match_bits),ev.mlength , ev.mlength, ev.start);
	if(msg->tail.ptl.copy)
		sctk_free(ptr->slot.me.start);

	sctk_complete_and_free_message(msg);
}


/**
 * Send a message through the RDV protocol.
 * \param[in] msg the msg to send
 * \param[in] endpoint the route to use
 */
void sctk_ptl_rdv_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	sctk_ptl_rail_info_t* srail    = &endpoint->rail->network.ptl;
	sctk_ptl_route_info_t* infos   = &endpoint->data.ptl;
	sctk_ptl_local_data_t *md_request , *me_request;
	int md_flags                      , me_flags;
	void* start;
	size_t size;
	sctk_ptl_id_t remote;
	sctk_ptl_pte_t* pte;
	sctk_ptl_matchbits_t match, ign;
	sctk_ptl_imm_data_t hdr;

	md_request      = me_request = NULL;
	md_flags        = me_flags   = 0;
	remote          = infos->dest;
	size            = 0;
	start           = NULL;
	match.data.tag  = SCTK_MSG_TAG(msg);
	match.data.rank = SCTK_MSG_SRC_PROCESS(msg);
	match.data.uid  = SCTK_MSG_NUMBER(msg);
	ign             = SCTK_PTL_MATCH_INIT;
	pte             = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg));

	/* if the message is non-contiguous, we need a copy to 'pack' it first */
	if(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
	{
		start = msg->tail.message.contiguous.addr;
	}
	else
	{
		/* in that case, the tail save the info, for later frees */
		start = sctk_malloc(SCTK_MSG_SIZE(msg));
		sctk_net_copy_in_buffer(msg, start);
		msg->tail.ptl.copy = 1;
	}
	size               = SCTK_MSG_SIZE(msg);
	
	/* Configure the Put() */
	md_flags           = SCTK_PTL_MD_PUT_FLAGS;
	md_request         = sctk_ptl_md_create(srail, NULL, 0, md_flags);
	
	/* prepare for the Get() */
	me_flags           = SCTK_PTL_ME_GET_FLAGS | SCTK_PTL_ONCE;
	me_request         = sctk_ptl_me_create(start, size, remote, match, ign, me_flags);

	/* double-linking */
	md_request->msg        = msg;
	me_request->msg        = msg;
	md_request->type       = SCTK_PTL_TYPE_STD;
	md_request->prot       = SCTK_PTL_PROT_RDV;
	me_request->type       = SCTK_PTL_TYPE_STD;
	me_request->prot       = SCTK_PTL_PROT_RDV;
	msg->tail.ptl.user_ptr = me_request;
	hdr.std.datatype       = SCTK_MSG_SPECIFIC_CLASS(msg);

	sctk_ptl_md_register(srail, md_request);
	sctk_ptl_me_register(srail, me_request, pte);
	sctk_ptl_emit_put(md_request, 0, remote, pte, match, 0, 0, hdr.raw, md_request); /* empty Put() */
	
	/* TODO: Need to handle the case where the data is larger than the max ME size */
	sctk_debug("PORTALS: SEND-RDV to %d (idx=%d, match=%s, sz=%llu)", SCTK_MSG_DEST_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), me_request->slot.me.length);
}

/**
 * Notification from upper layer that a local RECV has been posted.
 * \param[in] the generated msg
 * \param[in] srail the Portals-specific rail
 */
void sctk_ptl_rdv_notify_recv(sctk_thread_ptp_message_t* msg, sctk_ptl_rail_info_t* srail)
{
	sctk_ptl_matchbits_t match, ign;
	sctk_ptl_pte_t* pte;
	sctk_ptl_local_data_t *put_request, *get_request;
	int put_flags;
	
	/****** INIT COMMON ATTRIBUTES ******/
	pte             = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg));
	match.data.tag  = SCTK_MSG_TAG(msg);
	match.data.rank = SCTK_MSG_SRC_PROCESS(msg);
	match.data.uid  = SCTK_MSG_NUMBER(msg);
	ign.data.tag    = (SCTK_MSG_TAG(msg)         == SCTK_ANY_TAG)    ? SCTK_PTL_IGN_TAG  : SCTK_PTL_MATCH_TAG;
	ign.data.rank   = (SCTK_MSG_SRC_PROCESS(msg) == SCTK_ANY_SOURCE) ? SCTK_PTL_IGN_RANK : SCTK_PTL_MATCH_RANK;
	ign.data.uid    = SCTK_PTL_IGN_UID;

	/****** FIRST, take care of the PUT operation to receive *******/
	/***************************************************************/
	/* complete the ME data, this ME will be appended to the PRIORITY_LIST
	 * Here, we want a CT event attached to this ME (for triggered Op)
	 */
	put_flags = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ONCE;
	put_request  = sctk_ptl_me_create_with_cnt(srail, NULL, 0, SCTK_PTL_ANY_PROCESS, match, ign, put_flags);

	sctk_assert(put_request);
	sctk_assert(pte);

	put_request->msg       = msg;
	put_request->list      = SCTK_PTL_PRIORITY_LIST;
	msg->tail.ptl.user_ptr = NULL; /* set w/ the GET request */
	put_request->type = SCTK_PTL_TYPE_STD;
	put_request->prot = SCTK_PTL_PROT_RDV;
	
	/****** SECOND, attempt to use TriggeredOps if possible  *******/
	/***************************************************************/
#if 0
	if(SCTK_MSG_SRC_PROCESS(msg) != SCTK_ANY_SOURCE)
	{
		get_request = sctk_ptl_rdv_create_triggerGet(msg, srail, SCTK_PTL_ANY_PROCESS, match, put_request->slot.me.ct_handle);
		msg->tail.ptl.user_ptr = get_request; /* set w/ the GET request */
	}
	else
	{
		/* can't predict the remote ID.
		 * We will trigger the Get() when the Put() will match
		 */
		sctk_error("ANY_SOURCE");
		sctk_assert(ign.data.rank == SCTK_PTL_IGN_RANK);
	}
#endif
	/* this should be the last operation, to optimize the triggeredOps use */
	sctk_ptl_me_register(srail, put_request, pte);
	
	sctk_debug("PORTALS: NOTIFY-RECV-RDV from %d (idx=%llu, match=%s, ign=%llu)", SCTK_MSG_SRC_PROCESS(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), __sctk_ptl_match_str(malloc(32), 32, ign.raw));
}

void sctk_ptl_rdv_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	switch(ev.type)
	{
		case PTL_EVENT_PUT_OVERFLOW: /* a previous received PUT matched a just appended ME */
		case PTL_EVENT_PUT: /* a Put() reached the local process */
			/* we don't care about unexpected messaged reaching the OVERFLOW_LIST, we will just wait for their local counter-part */
			/* indexes from 0 to SCTK_PTL_PTE_HIDDEN-1 maps RECOVERY, CM & RDMA queues
			 * indexes from SCTK_PTL_PTE_HIDDEN to N maps communicators
			 */
			sctk_ptl_rdv_recv_message(rail, ev);
			break;

		case PTL_EVENT_GET: /* a remote process get the data back */
			sctk_ptl_rdv_complete_message(rail, ev);
			
			break;
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

void sctk_ptl_rdv_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* ptr = (sctk_ptl_local_data_t*) ev.user_ptr;
	switch(ev.type)
	{
		case PTL_EVENT_ACK: /* a PUT reached a remote process */
			/* just release the PUT MD, (no memory to free) */
			sctk_ptl_md_release(ptr);
			break;

		case PTL_EVENT_REPLY: /* a GET operation completed */
			sctk_ptl_rdv_reply_message(rail, ev);
			sctk_ptl_md_release(ptr);
			
			break;
		case PTL_EVENT_SEND:
			sctk_error("whut?");
			not_reachable();
		default:
			sctk_fatal("Unrecognized MD event: %d", ev.type);
			break;

	}
}

#endif
