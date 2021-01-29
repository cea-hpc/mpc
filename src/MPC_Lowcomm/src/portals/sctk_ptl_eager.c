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
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#include "endpoint.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_eager.h"
#include "sctk_net_tools.h"

#include <mpc_common_rank.h>


/**
 * Function to release the allocated message from the network.
 */
void sctk_ptl_eager_free_memory(void* msg)
{
	sctk_free(msg);
}

/** What to do when a network message matched a locally posted request ?.
 *  
 *  \param[in,out] msg the send/recv bundle where both message headers are stored
 */
void sctk_ptl_eager_message_copy(mpc_lowcomm_ptp_message_content_to_copy_t* msg)
{
	sctk_ptl_local_data_t* recv_data = msg->msg_recv->tail.ptl.user_ptr;
	
	/* We ensure data are already stored in the user buffer (zero-copy) only if the two condition are met:
	 *  1 - The message is a contiguous one (the locally-posted request is contiguous, the network request
	 *      is always contiguous). This is notified by the 'copy' field true in the recv msdg.
	 *  2 - The recv has been posted before the network message arrived. This means that Portals directly
	 *      matched the request with a PRIORITY_LIST ME. This is notified by the 'copy' field true 
	 *      in the 'send' msg.
	 */
	if(msg->msg_send->tail.ptl.copy || msg->msg_recv->tail.ptl.copy)
	{
		sctk_ptl_local_data_t* send_data = msg->msg_send->tail.ptl.user_ptr;
		/* here, we have to copy the message from the network buffer to the user buffer */
		sctk_net_message_copy_from_buffer(send_data->slot.me.start, msg, 0);
		/*
		 * If msg reached the OVERFLOW_LIST --> free the request & the temp buffer
		 * otherwise, the request is the same than the one free'd above
		 */
		sctk_ptl_me_free(send_data, msg->msg_send->tail.ptl.copy);
	}

	/* First, free local resources (PRIORITY ME)
	 * Free the temp buffer (for contiguous) if necessary (copy)
	 */
	sctk_ptl_me_free(recv_data, msg->msg_recv->tail.ptl.copy);

	/* flag request as completed */
	_mpc_comm_ptp_message_commit_request(msg->msg_send, msg->msg_recv);
}

/**
 * Recv an eager message from network.
 * \param[in] rail the Portals rail
 * \param[in] ev  the de-queued event, mapping a network message occurence.
 */
static inline void sctk_ptl_eager_recv_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	mpc_lowcomm_ptp_message_t* net_msg = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t));
	sctk_ptl_matchbits_t match = (sctk_ptl_matchbits_t) ev.match_bits;

	/* sanity checks */
	assert(rail);
	assert(ev.ni_fail_type == PTL_NI_OK);
	assert(ev.mlength <= rail->network.ptl.eager_limit);
	
	/* rebuild a complete MPC header msg (inter_thread_comm needs it) */
	mpc_lowcomm_ptp_message_header_clear(net_msg, MPC_LOWCOMM_MESSAGE_CONTIGUOUS , sctk_ptl_eager_free_memory, sctk_ptl_eager_message_copy);
	SCTK_MSG_SRC_PROCESS_SET     ( net_msg ,  match.data.rank);
	SCTK_MSG_SRC_TASK_SET        ( net_msg ,  match.data.rank);
	SCTK_MSG_DEST_PROCESS_SET    ( net_msg ,  mpc_common_get_process_rank());
	SCTK_MSG_DEST_TASK_SET       ( net_msg ,  mpc_common_get_process_rank());
	SCTK_MSG_COMMUNICATOR_ID_SET ( net_msg ,  ev.pt_index - SCTK_PTL_PTE_HIDDEN);
	SCTK_MSG_TAG_SET             ( net_msg ,  match.data.tag);
	SCTK_MSG_NUMBER_SET          ( net_msg ,  match.data.uid);
	SCTK_MSG_MATCH_SET           ( net_msg ,  0);
	SCTK_MSG_SPECIFIC_CLASS_SET  ( net_msg ,  match.data.type);
	SCTK_MSG_SIZE_SET            ( net_msg ,  ev.mlength);
	SCTK_MSG_COMPLETION_FLAG_SET ( net_msg ,  NULL);

	/* save the Portals context in the tail
	 * Whatever the origin, the user_ptr here is the one attached to the PRIORITY one
	 */
	net_msg->tail.ptl.user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;

	/* this value let us know if the message reached the OVERFLOW_LIST before being matched to the PRIORITY_LIST
	 * In that case, Portals ensure that 'start' field contains the overflow slot address
	 * Thus, we can know if some data need to be freed when the msg will complete
	 */
	net_msg->tail.ptl.copy = (net_msg->tail.ptl.user_ptr->slot.me.start != ev.start);

	/* We don't need to save the old ME start value, as it points to the user buffer and this info
	 * can be retrieved through the msg itself. We have to store where Portals effectively put the data.
	 * By convenience, it uses the same attribute.
	 */
	net_msg->tail.ptl.user_ptr->slot.me.start = ev.start;
	net_msg->tail.ptl.user_ptr->slot.me.length = ev.mlength;

	/* finish creating an MPC message heder */
	_mpc_comm_ptp_message_clear_request(net_msg);

	mpc_common_nodebug("PORTALS: RECV-EAGER from %d (idx=%d, match=%s, size=%lu) -> %p", SCTK_MSG_SRC_TASK(net_msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, match.raw), ev.mlength, ev.start);

	/* notify ther inter_thread_comm a new message has arrived */
	rail->send_message_from_network(net_msg);
}

/**
 * Send an eager message.
 *
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
void sctk_ptl_eager_send_message(mpc_lowcomm_ptp_message_t* msg, _mpc_lowcomm_endpoint_t* endpoint)
{
	sctk_ptl_local_data_t* request = NULL;
	sctk_ptl_rail_info_t* srail    = &endpoint->rail->network.ptl;
	void* start                    = NULL;
	size_t size                    = 0;
	int flags                      = 0;
	sctk_ptl_matchbits_t match     = SCTK_PTL_MATCH_INIT;
	sctk_ptl_pte_t* pte            = NULL;
	sctk_ptl_id_t remote           = SCTK_PTL_ANY_PROCESS;
	_mpc_lowcomm_endpoint_info_portals_t* infos   = &endpoint->data.ptl;
	sctk_ptl_imm_data_t hdr;
	
	/* clear the PTL msg tail struct */
	memset(&msg->tail.ptl, 0, sizeof(sctk_ptl_tail_t));

	/* if the message is non-contiguous, we need a copy to 'pack' it first */
	if(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
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

	/* prepare the Put() MD */
	size                   = SCTK_MSG_SIZE(msg);
	flags                  = SCTK_PTL_MD_PUT_FLAGS;
	match.data.tag         = SCTK_MSG_TAG(msg)            % SCTK_PTL_MAX_TAGS;
	match.data.rank        = SCTK_MSG_SRC_PROCESS(msg)    % SCTK_PTL_MAX_RANKS;
	match.data.uid         = SCTK_MSG_NUMBER(msg)         % SCTK_PTL_MAX_UIDS;
	match.data.type        = SCTK_MSG_SPECIFIC_CLASS(msg) % SCTK_PTL_MAX_TYPES;
	pte                    = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR_ID(msg));
	remote                 = infos->dest;
	request                = sctk_ptl_md_create(srail, start, size, flags);

	assert(request);
	assert(pte);

	/* double-linking */
	request->msg           = msg;
	request->type          = SCTK_PTL_TYPE_STD;
	request->prot          = SCTK_PTL_PROT_EAGER;
	request->msg_seq_nb    = SCTK_MSG_NUMBER(msg);
	hdr.std.msg_seq_nb     = SCTK_MSG_NUMBER(msg);
	hdr.std.putsz          = 0; /* this set the protocol in imm_data for receiver optimizations */
	msg->tail.ptl.user_ptr = request;

	/* emit the request */
	sctk_ptl_md_register(srail, request);
	sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, hdr.raw, request);
	
	mpc_common_nodebug("PORTALS: SEND-EAGER to %d (idx=%d, match=%s, sz=%llu)", SCTK_MSG_DEST_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), size);
}

/**
 * Notificatio that a local recv has been posted.
 *
 * This function will pin memory region to be ready for incoming network data.
 * \param[in] msg the generated RECV
 * \param[in] srail the Portals rail
 */
void sctk_ptl_eager_notify_recv(mpc_lowcomm_ptp_message_t* msg, sctk_ptl_rail_info_t* srail)
{
	void* start                     = NULL;
	size_t size                     = 0;
	sctk_ptl_matchbits_t match      = SCTK_PTL_MATCH_INIT;
	sctk_ptl_matchbits_t ign        = SCTK_PTL_MATCH_INIT;
	int flags                       = 0;
	sctk_ptl_pte_t* pte             = NULL;
	sctk_ptl_local_data_t* user_ptr = NULL;
	sctk_ptl_id_t remote            = SCTK_PTL_ANY_PROCESS;

	/* clear the PTL msg tail struct */
	memset(&msg->tail.ptl, 0, sizeof(sctk_ptl_tail_t));
	
	/* is the message contiguous ? */
	if(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
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

	/* Build the match_bits */
	match.data.tag  = SCTK_MSG_TAG(msg)            % SCTK_PTL_MAX_TAGS;
	match.data.rank = SCTK_MSG_SRC_PROCESS(msg)    % SCTK_PTL_MAX_RANKS;
	match.data.uid  = SCTK_MSG_NUMBER(msg)         % SCTK_PTL_MAX_UIDS;
	match.data.type = SCTK_MSG_SPECIFIC_CLASS(msg) % SCTK_PTL_MAX_TYPES;

	/* apply the mask, depending on request infos
	 * The UID is always ignored, it we only be used to make RDV requests consistent
	 */
	ign.data.tag  = (SCTK_MSG_TAG(msg)         == MPC_ANY_TAG)    ? SCTK_PTL_IGN_TAG  : SCTK_PTL_MATCH_TAG;
	ign.data.rank = (SCTK_MSG_SRC_PROCESS(msg) == MPC_ANY_SOURCE) ? SCTK_PTL_IGN_RANK : SCTK_PTL_MATCH_RANK;
	ign.data.uid  = SCTK_PTL_IGN_UID;
	ign.data.type = SCTK_PTL_MATCH_TYPE;

	/* complete the ME data, this ME will be appended to the PRIORITY_LIST */
	size     = SCTK_MSG_SIZE(msg);
	pte      = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR_ID(msg));
	flags    = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ONCE;
	user_ptr = sctk_ptl_me_create(start, size, remote, match, ign, flags); assert(user_ptr);

	assert(user_ptr);
	assert(pte);

	user_ptr->msg          = msg;
	user_ptr->list         = SCTK_PTL_PRIORITY_LIST;
	user_ptr->type         = SCTK_PTL_TYPE_STD;
	user_ptr->prot         = SCTK_PTL_PROT_EAGER;
	msg->tail.ptl.user_ptr = user_ptr;
	sctk_ptl_me_register(srail, user_ptr, pte);
	
	mpc_common_nodebug("PORTALS: NOTIFY-RECV-EAGER from %d (idx=%llu, match=%s, ign=%llu start=%p, sz=%llu)", SCTK_MSG_SRC_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), __sctk_ptl_match_str(malloc(32), 32, ign.raw), start, size);
}

/**
 * Notification that an incoming eager msg reached the local process.
 * \param[in] rail the Portals rail
 * \param[in] ev the generated event
 */
void sctk_ptl_eager_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	switch(ev.type)
	{
		case PTL_EVENT_PUT_OVERFLOW:          /* a previous received PUT matched a just appended ME */
		case PTL_EVENT_PUT:                   /* a Put() reached the local process */
			/* we don't care about unexpected messaged reaching the OVERFLOW_LIST, we will just wait for their local counter-part */
			/* indexes from 0 to SCTK_PTL_PTE_HIDDEN-1 maps RECOVERY, CM & RDMA queues
			 * indexes from SCTK_PTL_PTE_HIDDEN to N maps communicators
			 */
			sctk_ptl_eager_recv_message(rail, ev);
			break;

		case PTL_EVENT_GET:                   /* a Get() reached the local process */
		case PTL_EVENT_GET_OVERFLOW:          /* a previous received GET matched a just appended ME */
		case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
		case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
		case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
		case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
		case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabeld (FLOW_CTRL) */
		case PTL_EVENT_SEARCH:                /* a PtlMESearch completed */
		case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
		case PTL_EVENT_AUTO_UNLINK:           /* an USE_ONCE ME has been automatically unlinked */
		case PTL_EVENT_AUTO_FREE:             /* an USE_ONCE ME can be now reused */
			not_reachable();              /* have been disabled */
			break;
		default:
			mpc_common_debug_fatal("Portals ME event not recognized: %d", ev.type);
			break;
	}
}

/**
 * Notification that a locally-emitted request has been asynchronously progressed.
 * \param[in] rail the portals rail
 * \param[in] ev the generated event
 */
void sctk_ptl_eager_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	mpc_lowcomm_ptp_message_t* msg = (mpc_lowcomm_ptp_message_t*)user_ptr->msg;

	UNUSED(rail); 

	switch(ev.type)
	{
		case PTL_EVENT_ACK:  /* the Put() reached the remote process */
			if(msg->tail.ptl.copy)
			{
				sctk_free(user_ptr->slot.md.start);
			}
			/* tag the message as completed */
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			sctk_ptl_md_release(user_ptr);
			break;

		case PTL_EVENT_REPLY: /* the Get() downloaded the data from the remote process */
		case PTL_EVENT_SEND:  /* a local request left the local process */
			not_reachable();
			break;
		default:
			mpc_common_debug_fatal("Unrecognized MD event: %d", ev.type);
			break;
	}
}
