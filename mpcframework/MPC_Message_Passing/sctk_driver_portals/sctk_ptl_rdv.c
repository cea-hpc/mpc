#ifdef MPC_USE_PORTALS
#include "sctk_route.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_iface.h"
#include "sctk_net_tools.h"

/**
 * Function to release the allocated message from the network.
 */
void sctk_ptl_rdv_free_memory(void* msg)
{
	/*sctk_free(msg);*/
	not_implemented();
}

/** What to do when a network message matched a locally posted request ?.
 *  
 *  \param[in,out] msg the send/recv bundle where both message headers are stored
 */
void sctk_ptl_rdv_message_copy(sctk_message_to_copy_t* msg)
{
	not_implemented();
}

static inline void sctk_ptl_rdv_recv_message(sctk_rail_info_t* srail, sctk_ptl_event_t ev)
{
	not_implemented();
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

	md_request = me_request = NULL;
	md_flags   = me_flags   = 0;
	remote  = SCTK_PTL_ANY_PROCESS;
	size    = 0;
	start   = NULL;
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
	md_request         = sctk_ptl_md_create(srail, &srail->id, sizeof(sctk_ptl_id_t), md_flags);
	
	/* prepare for the Get() */
	me_flags           = SCTK_PTL_ME_GET_FLAGS | SCTK_PTL_ONCE;
	me_request         = sctk_ptl_me_create(start, size, remote, match, ign, me_flags);

	/* double-linking */
	md_request->msg        = msg;
	me_request->msg        = msg;
	md_request->pt_idx     = pte->idx;
	me_request->pt_idx     = pte->idx;
	msg->tail.ptl.user_ptr = md_request;
	hdr.rdv.datatype       = SCTK_MSG_SPECIFIC_CLASS(msg);

	sctk_ptl_md_register(srail, md_request);
	sctk_ptl_me_register(srail, me_request, pte);
	sctk_ptl_emit_put(md_request, sizeof(sctk_ptl_id_t), remote, pte, match, 0, 0, hdr.raw, md_request); /* empty Put() */
	
	/* TODO: Need to handle the case where the data is larger than the max ME size */
	sctk_warning("PORTALS: SEND-RDV to %d (idx=%d, match=%s, sz=%llu)", SCTK_MSG_DEST_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), size);
}

/**
 * Notification from upper layer that a local RECV has been posted.
 * \param[in] the generated msg
 * \param[in] srail the Portals-specific rail
 */
void sctk_ptl_rdv_notify_recv(sctk_thread_ptp_message_t* msg, sctk_ptl_rail_info_t* srail)
{
	void  *get_start = NULL, *put_start = NULL;
	size_t put_size, get_size;
	sctk_ptl_matchbits_t match, ign;
	sctk_ptl_pte_t* pte;
	sctk_ptl_id_t remote;
	sctk_ptl_local_data_t *put_request, *get_request;
	int put_flags, get_flags;
	
	/****** INIT COMMON ATTRIBUTES ******/
	pte             = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR(msg));
	match.data.tag  = SCTK_MSG_TAG(msg);
	match.data.rank = SCTK_MSG_SRC_PROCESS(msg);
	match.data.uid  = SCTK_MSG_NUMBER(msg);
	ign.data.tag    = (match.data.tag  == SCTK_ANY_TAG)    ? SCTK_PTL_IGN_TAG  : SCTK_PTL_MATCH_TAG;
	ign.data.rank   = (match.data.rank == SCTK_ANY_SOURCE) ? SCTK_PTL_IGN_RANK : SCTK_PTL_MATCH_RANK;
	ign.data.uid    = SCTK_PTL_IGN_UID;

	/****** FIRST, take care of the PUT operation to receive *******/
	/***************************************************************/
	/* complete the ME data, this ME will be appended to the PRIORITY_LIST
	 * Here, we want a CT event attached to this ME (for triggered Op)
	 */
	put_flags = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ONCE;
	remote    = SCTK_PTL_ANY_PROCESS;
	put_request  = sctk_ptl_me_create_with_cnt(srail, NULL, 0, remote, match, ign, put_flags);

	sctk_assert(put_request);
	sctk_assert(pte);

	put_request->msg       = msg;
	put_request->list      = SCTK_PTL_PRIORITY_LIST;
	msg->tail.ptl.user_ptr = put_request;
	
	/****** SECOND, attempt to use TriggeredOps if possible  *******/
	/***************************************************************/
	if(SCTK_MSG_SRC_PROCESS(msg) != SCTK_ANY_SOURCE)
	{
		/* is the message contiguous ? Maybe we could try to get an IOVEC instead of packing ? */
		if(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
		{
			get_start = msg->tail.message.contiguous.addr;
		}
		else
		{
			/* if not, we map a temporary buffer, ready for network serialized data */
			get_start = sctk_malloc(SCTK_MSG_SIZE(msg));
			/* and we flag the copy to 1, to remember some buffers will have to be recopied */
			msg->tail.ptl.copy = 1;
		}

		/* configure the MD */
		get_size = SCTK_MSG_SIZE(msg);
		get_flags = SCTK_PTL_ME_GET_FLAGS | SCTK_PTL_ONCE;
		/*remote = ;*/

		get_request = sctk_ptl_md_create(srail, get_start, get_size, get_flags);
		sctk_ptl_md_register(srail, get_request);
		get_request->msg = msg;

		sctk_ptl_emit_triggeredGet(
				get_request,
				get_size,
				remote,
				pte,
				match,
				0,
				0,
				get_request,
				put_request->slot.me.ct_handle,
				1
		);
	}
	else
	{
		/* can't predict the remote ID.
		 * We will trigget the Get() when the Put() will match
		 */
	}

	/* this should be the last operation, to optimize the triggeredOps use */
	sctk_ptl_me_register(srail, put_request, pte);
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

		case PTL_EVENT_GET: /* a Get() reached the local process */
		case PTL_EVENT_GET_OVERFLOW: /* a previous received GET matched a just appended ME */
			break;
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
	switch(ev.type)
	{
		case PTL_EVENT_ACK:
		case PTL_EVENT_REPLY:
			
			break;
		case PTL_EVENT_SEND:
			not_reachable();
		default:
			sctk_fatal("Unrecognized MD event: %d", ev.type);
			break;

	}
}

#endif
