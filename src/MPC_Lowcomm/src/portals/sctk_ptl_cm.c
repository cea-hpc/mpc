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
#include "sctk_ptl_cm.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_types.h"
#include "msg_cpy.h"

#include <mpc_common_rank.h>

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
void sctk_ptl_cm_message_copy(mpc_lowcomm_ptp_message_content_to_copy_t* msg)
{
	/* dirty line: get back the ME address, where data are located
	 * + complete and free the message.
	 */
	_mpc_lowcomm_msg_cpy_from_buffer(msg->msg_send->tail.ptl.user_ptr->slot.me.start, msg, 1);
}

/**
 * Routine called when a new CM arrived from the network.
 * \param[in] rail the Portals rail
 * \param[in] ev the polled Portals event
 */
static inline void sctk_ptl_cm_recv_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	mpc_lowcomm_ptp_message_t* net_msg = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t));
	sctk_ptl_matchbits_t match         = (sctk_ptl_matchbits_t) ev.match_bits;
	sctk_ptl_imm_data_t hdr ;
	sctk_ptl_local_data_t* ptr         = (sctk_ptl_local_data_t*) ev.user_ptr;


	/* sanity checks */
	assert(rail);
	assert(ev.ni_fail_type == PTL_NI_OK);
	assert(ev.mlength <= rail->network.ptl.eager_limit);
	assert(ev.type == PTL_EVENT_PUT);

	/* rebuild a complete MPC header msg (inter_thread_comm needs it) */
	mpc_lowcomm_ptp_message_header_clear(net_msg, MPC_LOWCOMM_MESSAGE_CONTIGUOUS , sctk_ptl_cm_free_memory, sctk_ptl_cm_message_copy);
	SCTK_MSG_SRC_PROCESS_SET     ( net_msg ,  match.data.rank);
	SCTK_MSG_SRC_TASK_SET        ( net_msg ,  match.data.rank);
	SCTK_MSG_DEST_PROCESS_SET    ( net_msg ,  mpc_common_get_process_rank());
	SCTK_MSG_DEST_TASK_SET       ( net_msg ,  -1);
	SCTK_MSG_COMMUNICATOR_ID_SET ( net_msg ,  0);
	SCTK_MSG_TAG_SET             ( net_msg ,  match.data.tag);
	SCTK_MSG_NUMBER_SET          ( net_msg ,  ptr->msg_seq_nb);
	SCTK_MSG_SPECIFIC_CLASS_SET  ( net_msg ,  match.data.type);
	SCTK_MSG_MATCH_SET           ( net_msg ,  0);
	SCTK_MSG_SIZE_SET            ( net_msg ,  ev.mlength);
	SCTK_MSG_COMPLETION_FLAG_SET ( net_msg ,  NULL);

	/* de-serialise hdr_data */
	hdr                                       = (sctk_ptl_imm_data_t)ev.hdr_data;
	net_msg->body.header.message_type    = hdr.cm.type;

	/* save the Portals context in the tail */
	net_msg->tail.ptl.user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	net_msg->tail.ptl.user_ptr->slot.me.start = ev.start;

	/* finish creating an MPC message header */
	_mpc_comm_ptp_message_clear_request(net_msg);

	mpc_common_nodebug("PORTALS: RECV-CM from %d (idx=%d, match=%s, size=%lu) -> %p", SCTK_MSG_SRC_TASK(net_msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, match.raw), ev.mlength, ev.start);

	/* notify ther inter_thread_comm a new message has arrived */
	rail->send_message_from_network(net_msg);
}

/**
 * Send a CM.
 * \param[in] msg the message to send.
 * \param[in] endpoint the route to use.
 */
void sctk_ptl_cm_send_message(mpc_lowcomm_ptp_message_t* msg, _mpc_lowcomm_endpoint_t* endpoint)
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

	/* CM are always contiguous `*/
	assert(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS);

	/* configure the MD */
	start           = msg->tail.message.contiguous.addr;
	size            = SCTK_MSG_SIZE(msg);
	flags           = SCTK_PTL_MD_PUT_FLAGS;
	match.data.rank = SCTK_MSG_SRC_PROCESS(msg);
	match.data.tag  = SCTK_MSG_TAG(msg);
	match.data.uid  = SCTK_MSG_NUMBER(msg);
	match.data.type = SCTK_MSG_SPECIFIC_CLASS(msg);
	remote          = infos->dest;
	pte             = mpc_common_hashtable_get(&srail->pt_table, SCTK_PTL_PTE_CM);
	request         = sctk_ptl_md_create(srail, start, size, flags);

	assert(pte);
	assert(request);

	/* double-linking */
	request->msg           = msg;
	msg->tail.ptl.user_ptr = request;
	msg->tail.ptl.copy     = 0;
	request->type = SCTK_PTL_TYPE_CM;

	/* populate immediate data with CM-specific */
	hdr.cm.type            = msg->body.header.message_type;

	sctk_ptl_md_register(srail, request);
	sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, hdr.raw, request);

	mpc_common_nodebug("PORTALS: SEND-CM to %d (idx=%d, match=%s, addr=%p, sz=%llu)", SCTK_MSG_DEST_PROCESS(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), start, size);
}

/**
 * Notify the local process that a new CM reached the local process.
 * \param[in] rail the Portals rail
 * \param[in] ev the generated event
 */
void sctk_ptl_cm_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_pte_t fake;
	sctk_ptl_rail_info_t* srail = &rail->network.ptl;
	switch(ev.type)
	{
		case PTL_EVENT_PUT:                   /* a Put() reached the local process */
			fake = (sctk_ptl_pte_t){.idx = ev.pt_index};
			sctk_ptl_me_feed(srail,  &fake,  srail->eager_limit, 1, SCTK_PTL_PRIORITY_LIST, SCTK_PTL_TYPE_CM, SCTK_PTL_PROT_NONE);
			sctk_ptl_cm_recv_message(rail, ev);
			break;

		case PTL_EVENT_GET:                   /* a Get() reached the local process */
		case PTL_EVENT_PUT_OVERFLOW:          /* a previous received PUT matched a just appended ME */
		case PTL_EVENT_GET_OVERFLOW:          /* a previous received GET matched a just appended ME */
		case PTL_EVENT_ATOMIC:                /* an Atomic() reached the local process */
		case PTL_EVENT_ATOMIC_OVERFLOW:       /* a previously received ATOMIC matched a just appended one */
		case PTL_EVENT_FETCH_ATOMIC:          /* a FetchAtomic() reached the local process */
		case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: /* a previously received FETCH-ATOMIC matched a just appended one */
		case PTL_EVENT_PT_DISABLED:           /* ERROR: The local PTE is disabeld (FLOW_CTRL) */
		case PTL_EVENT_SEARCH:                /* a PtlMESearch completed */
			                              /* probably nothing to do here */
		case PTL_EVENT_LINK:                  /* MISC: A new ME has been linked, (maybe not useful) */
		case PTL_EVENT_AUTO_UNLINK:           /* an USE_ONCE ME has been automatically unlinked */
		case PTL_EVENT_AUTO_FREE:             /* an USE_ONCE ME can be now reused */
			not_reachable(); /* have been disabled */
			break;
		default:
			mpc_common_debug_fatal("Portals ME event not recognized: %d", ev.type);
			break;
	}
}

/**
 * Notify the local process that a new CM reached the remote process.
 * \param[in] rail the Portals rail
 * \param[in] ev the generated event
 */
void sctk_ptl_cm_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	mpc_lowcomm_ptp_message_t* msg = (mpc_lowcomm_ptp_message_t*)user_ptr->msg;

	UNUSED(rail);
	switch(ev.type)
	{
		case PTL_EVENT_ACK:   /* the request reached the process */
			mpc_lowcomm_ptp_message_complete_and_free(msg);
			sctk_ptl_md_release(user_ptr);
			break;

		case PTL_EVENT_REPLY: /* a Get() reached the local process */
		case PTL_EVENT_SEND:  /* a Put() just left the local process */
			not_reachable();
		default:
			mpc_common_debug_fatal("Unrecognized MD event: %d", ev.type);
			break;
	}
}
