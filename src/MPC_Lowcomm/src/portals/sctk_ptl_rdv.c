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

#ifdef MPC_USE_PORTALS

#include "endpoint.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_iface.h"
#include "msg_cpy.h"

#include <mpc_common_rank.h>
#include <mpc_lowcomm_datatypes.h>
/** see sctk_portals.c */
extern sctk_ptl_id_t* ranks_ids_map;

/**
 * Function to release the allocated message from the network.
 * \param[in] msg the net incoming msg.
 */
void sctk_ptl_rdv_free_memory(void* msg)
{
	sctk_ptl_me_free(((mpc_lowcomm_ptp_message_t*)msg)->tail.ptl.user_ptr, 1);
	sctk_free(msg);
	/*not_implemented();*/
}

/** What to do when a network message matched a locally posted request ?.
 *
 *  \param[in,out] msg the send/recv bundle where both message headers are stored
 */
void sctk_ptl_rdv_message_copy(mpc_lowcomm_ptp_message_content_to_copy_t* msg)
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
		_mpc_lowcomm_msg_cpy_from_buffer(send_data->slot.me.start, msg, 0);
	}

	_mpc_comm_ptp_message_commit_request(msg->msg_send, msg->msg_recv);
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
	sctk_ptl_local_data_t* get_request = NULL;
	sctk_ptl_pte_t* pte                = NULL;
	sctk_ptl_rail_info_t* srail        = &rail->network.ptl;
	sctk_ptl_local_data_t* ptr         = (sctk_ptl_local_data_t*) ev.user_ptr;
	mpc_lowcomm_ptp_message_t* msg     = (mpc_lowcomm_ptp_message_t*) ptr->msg;
	void* start;
	size_t cur_off, chunk_sz, chunk_nb, chunk, chunk_rest, sz_sent;
	int flags;

	assert(msg);

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

	/* is the message contiguous ? Maybe we could try to get an IOVEC instead of packing ? */
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

	/* configure the MD */
	pte       = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR_ID(msg));
	flags     = SCTK_PTL_MD_GET_FLAGS;
	sz_sent  = *(size_t*)ev.start;
	assert(pte);

	/* this function will compute byte equal distribution between multiple GET, if needed */
	assert(((sctk_ptl_imm_data_t)ev.hdr_data).std.putsz);
	sctk_ptl_compute_chunks(srail, sz_sent, &chunk_sz, &chunk_nb, &chunk_rest);
	assert(chunk_nb > 0);
   	assert(chunk_rest < chunk_nb);
	/* create a new MD and configure it */
	get_request = sctk_ptl_md_create(
		srail,
		start,
		sz_sent,
		flags
	);
	assert(get_request);

	get_request->msg                    = msg;
	get_request->list                  = SCTK_PTL_PRIORITY_LIST;
	get_request->type                  = SCTK_PTL_TYPE_STD;
	get_request->prot                  = SCTK_PTL_PROT_RDV;

	/* retreive and store the match_bits, it contains the actual SRC_RANK (in case of ANY_SOURCE,
	 * we don't have such information) */
	get_request->match      = (sctk_ptl_matchbits_t)ev.match_bits;
	get_request->msg_seq_nb = ((sctk_ptl_imm_data_t)ev.hdr_data).std.msg_seq_nb;
	OPA_store_int(&get_request->cnt_frag, chunk_nb);

	/* The GET request only target slots with the bit to 1 */
	sctk_ptl_md_register(srail, get_request);

	cur_off = 0;
	sctk_ptl_matchbits_t md_match = get_request->match;
	SCTK_PTL_TYPE_RDV_SET(md_match.data.type);

	md_match.data.rank = mpc_common_get_process_rank();

	for (chunk = 0; chunk < chunk_nb; ++chunk)
	{
		/* if the should take '+1' to compensate non-distributed bytes */
		size_t cur_sz = (chunk < chunk_rest) ? chunk_sz + 1 : chunk_sz;

		mpc_common_debug("GET IS to @ %s", __sctk_ptl_match_str(malloc(32), 32, md_match.raw));

		mpc_common_debug("EMIT GET %d - %d (sz=%d for %d chunks)", cur_off, cur_off + cur_sz, cur_sz, chunk_nb);
		sctk_ptl_emit_get(
			get_request,        /* the current GET request */
			cur_sz,             /* the size for this chunk */
			ev.initiator,       /* the ID contained in the PUT request */
			pte,                /* the PT entry */
			md_match,           /* the match_bits used by the remote to expose the memory */
			cur_off,            /* local_offset */
			cur_off,            /* remote_offset */
			get_request         /* user_ptr */
		);
		cur_off += cur_sz;
	}

	mpc_common_debug("PORTALS: RECV-RDV to %llu (idx=%d, match=%s, ch_nb=%llu, ch_sz=%llu)", SCTK_MSG_SRC_PROCESS_UID(msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, ev.match_bits), chunk_nb, chunk_sz);
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
	mpc_lowcomm_ptp_message_t* net_msg = sctk_malloc(sizeof(mpc_lowcomm_ptp_message_t));
	sctk_ptl_local_data_t* ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	mpc_lowcomm_ptp_message_t* recv_msg = (mpc_lowcomm_ptp_message_t*)ptr->msg;

	/* sanity checks */
	assert(rail);
	assert(ev.ni_fail_type == PTL_NI_OK);

	/* As the UID cannot be transported in the portals matchbit
	   we need to rebuild it from the (comm + task_id) infos */
	mpc_lowcomm_communicator_id_t comm_id = SCTK_MSG_COMMUNICATOR_ID(((mpc_lowcomm_ptp_message_t*)ptr->msg));
	mpc_lowcomm_communicator_t comm = mpc_lowcomm_get_communicator_from_id(comm_id);
	mpc_lowcomm_peer_uid_t process_uid = mpc_lowcomm_communicator_uid(comm, ptr->match.data.rank);

	/* rebuild a complete MPC header msg (inter_thread_comm needs it) */
	mpc_lowcomm_ptp_message_header_clear(net_msg, MPC_LOWCOMM_MESSAGE_CONTIGUOUS , sctk_ptl_rdv_free_memory, sctk_ptl_rdv_message_copy);
	SCTK_MSG_SRC_PROCESS_UID_SET ( net_msg ,  process_uid);
	SCTK_MSG_SRC_TASK_SET        ( net_msg ,  ptr->match.data.rank);
	SCTK_MSG_DEST_PROCESS_UID_SET    ( net_msg ,  mpc_lowcomm_monitor_get_uid());
	SCTK_MSG_DEST_TASK_SET       ( net_msg ,  mpc_common_get_process_rank());
	SCTK_MSG_COMMUNICATOR_ID_SET ( net_msg ,  SCTK_MSG_COMMUNICATOR_ID(recv_msg));
	SCTK_MSG_TAG_SET             ( net_msg ,  ptr->match.data.tag);
	SCTK_MSG_NUMBER_SET          ( net_msg ,  ptr->msg_seq_nb);
	SCTK_PTL_TYPE_RDV_UNSET(ptr->match.data.type); /* this infomation should be removed before rebuilding the message */
	SCTK_MSG_SPECIFIC_CLASS_SET  ( net_msg ,  ptr->match.data.type);
	SCTK_MSG_MATCH_SET           ( net_msg ,  0);
	SCTK_MSG_SIZE_SET            ( net_msg ,  ev.mlength);
	SCTK_MSG_COMPLETION_FLAG_SET ( net_msg ,  NULL);
	SCTK_MSG_USE_MESSAGE_NUMBERING_SET(net_msg, 1);
	SCTK_MSG_DATATYPE_SET(net_msg, MPC_LOWCOMM_BYTE);

	/* save the Portals context in the tail
	 * Whatever the origin, the user_ptr here is the one attached to the PRIORITY one
	 */
	net_msg->tail.ptl.user_ptr = ptr;
	net_msg->tail.ptl.copy = 0;

	/* finish creating an MPC message header */
	_mpc_comm_ptp_message_clear_request(net_msg);

	mpc_common_debug("PORTALS: REPLY-RDV !! from %llu (match=%s, rsize=%llu, size=%llu) -> %p", SCTK_MSG_SRC_PROCESS_UID(net_msg), __sctk_ptl_match_str(malloc(32), 32, ptr->match.raw),ptr->slot.md.length , ev.mlength, ptr->slot.md.start);

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
	mpc_lowcomm_ptp_message_t *msg = (mpc_lowcomm_ptp_message_t*)ptr->msg;

	UNUSED(rail);

	mpc_common_debug("PORTALS: COMPLETE-RDV to %llu (idx=%d, match=%s, rsize=%llu, size=%llu) -> %p", SCTK_MSG_SRC_PROCESS_UID(msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, ev.match_bits),ev.mlength , ev.mlength, ev.start);
	if(msg->tail.ptl.copy)
		sctk_free(ptr->slot.me.start);

	mpc_lowcomm_ptp_message_complete_and_free(msg);
}


/**
 * Send a message through the RDV protocol.
 * \param[in] msg the msg to send
 * \param[in] endpoint the route to use
 */
void sctk_ptl_rdv_send_message(mpc_lowcomm_ptp_message_t* msg, _mpc_lowcomm_endpoint_t* endpoint)
{
	sctk_ptl_rail_info_t* srail    = &endpoint->rail->network.ptl;
	_mpc_lowcomm_endpoint_info_portals_t* infos   = &endpoint->data.ptl;
	sctk_ptl_local_data_t *md_request , *me_request;
	int md_flags                      , me_flags;
	void* start;
	size_t chunk_nb, chunk_sz, chunk_rest;
	sctk_ptl_id_t remote;
	sctk_ptl_pte_t* pte;
	sctk_ptl_matchbits_t match, ign;
	sctk_ptl_imm_data_t hdr;

	memset(&msg->tail.ptl, 0, sizeof(sctk_ptl_tail_t));

	md_request      = me_request = NULL;
	md_flags        = me_flags   = 0;
	remote          = infos->dest;
	start           = NULL;
	match.data.tag  = SCTK_MSG_TAG(msg)            % SCTK_PTL_MAX_TAGS;
	match.data.rank = SCTK_MSG_SRC_TASK(msg)    % SCTK_PTL_MAX_RANKS;
	match.data.uid  = SCTK_MSG_NUMBER(msg)         % SCTK_PTL_MAX_UIDS;
	match.data.type = SCTK_MSG_SPECIFIC_CLASS(msg) % SCTK_PTL_MAX_TYPES;
	SCTK_PTL_TYPE_RDV_UNSET(match.data.type); /* the PUT does not carry that information */
	ign             = SCTK_PTL_MATCH_INIT;
	pte             = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR_ID(msg));

	/************************/
	/* 1. Configure the PUT */
	/************************/
	md_flags           = SCTK_PTL_MD_PUT_FLAGS;
	md_request         = sctk_ptl_md_create(srail, &(SCTK_MSG_SIZE(msg)), sizeof(size_t), md_flags);
	md_request->msg    = msg;
	md_request->type   = SCTK_PTL_TYPE_STD;
	md_request->prot   = SCTK_PTL_PROT_RDV;
	md_request->match  = match;
	hdr.std.msg_seq_nb = SCTK_MSG_NUMBER(msg);
	hdr.std.putsz      = 1; /**< this means to the receiver : "The payload of this PUT is the message size" */
	sctk_ptl_md_register(srail, md_request);

	/***************************/
	/* 2. Configure the GET(s) */
	/***************************/
	/* compute how many chunks we'll need to expose the memory  */
	sctk_ptl_compute_chunks(srail, SCTK_MSG_SIZE(msg), &chunk_sz, &chunk_nb, &chunk_rest);
	assert(chunk_nb > 0ull);
	assert(chunk_rest < chunk_nb);
	me_flags         = SCTK_PTL_ME_GET_FLAGS | ((chunk_nb == 1) ? SCTK_PTL_ONCE : 0);
	/* if the message is non-contiguous, we need a copy to 'pack' it first */
	if(msg->tail.message_type == MPC_LOWCOMM_MESSAGE_CONTIGUOUS)
	{
		start = msg->tail.message.contiguous.addr;
	}
	else
	{
		/* in that case, the tail save the info, for later frees */
		start = sctk_malloc(SCTK_MSG_SIZE(msg));
		_mpc_lowcomm_msg_cpy_in_buffer(msg, start);
		msg->tail.ptl.copy = 1;
	}

	sctk_ptl_matchbits_t me_match = match;
	/* Set the bit to 1 to differenciate incoming PUT and waiting-to-complete GETs
	 * Set the rank to DEST process to identify it among others
	 * It is this one which is stored into rdv_extras struct
	 */
	me_match.data.rank = SCTK_MSG_DEST_TASK(msg);
	SCTK_PTL_TYPE_RDV_SET(me_match.data.type);
	/* create the MD request and configure it */
	me_request = sctk_ptl_me_create(
		start,
		SCTK_MSG_SIZE(msg),
		remote,
		me_match,
		ign,
		me_flags
	);

	mpc_common_debug("GET IS @ %s", __sctk_ptl_match_str(malloc(32), 32, me_match.raw));

	me_request->msg        = msg;
	me_request->type       = SCTK_PTL_TYPE_STD;
	me_request->prot       = SCTK_PTL_PROT_RDV;
	/* store infos that should be expanded from forthcomin REPLY op */
	me_request->match = me_match;
	me_request->msg_seq_nb = SCTK_MSG_NUMBER(msg);
	OPA_store_int(&me_request->cnt_frag, ((chunk_nb == 1) ? 0 : chunk_nb)); /* special case - see event_me() */

	msg->tail.ptl.user_ptr = me_request;
	sctk_ptl_me_register(srail, me_request, pte);

	sctk_ptl_emit_put(md_request, sizeof(size_t), remote, pte, match, 0, 0, hdr.raw, md_request); /* empty Put() */

	/* TODO: Need to handle the case where the data is larger than the max ME size */
	mpc_common_debug("PORTALS: SEND-RDV to %d (idx=%d, match=%s, chunk_nb=%llu, ch_sz=%llu)", SCTK_MSG_DEST_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), chunk_nb , chunk_sz);
}

/**
 * Notification from upper layer that a local RECV has been posted.
 * \param[in] the generated msg
 * \param[in] srail the Portals-specific rail
 */
void sctk_ptl_rdv_notify_recv(mpc_lowcomm_ptp_message_t* msg, sctk_ptl_rail_info_t* srail)
{
	sctk_ptl_matchbits_t match, ign;
	sctk_ptl_pte_t* pte;
	sctk_ptl_local_data_t *put_request;
	int put_flags;

	/****** INIT COMMON ATTRIBUTES ******/
	pte             = SCTK_PTL_PTE_ENTRY(srail->pt_table, SCTK_MSG_COMMUNICATOR_ID(msg));
	match.data.tag  = SCTK_MSG_TAG(msg)            % SCTK_PTL_MAX_TAGS;
	match.data.rank = SCTK_MSG_SRC_TASK(msg)    % SCTK_PTL_MAX_RANKS;
	match.data.uid  = SCTK_MSG_NUMBER(msg)         % SCTK_PTL_MAX_UIDS;
	match.data.type = SCTK_MSG_SPECIFIC_CLASS(msg) % SCTK_PTL_MAX_TYPES;
	SCTK_PTL_TYPE_RDV_UNSET(match.data.type); /* the emitted PUT does not carry the information */
	ign.data.tag    = (SCTK_MSG_TAG(msg)         == MPC_ANY_TAG)    ? SCTK_PTL_IGN_TAG  : SCTK_PTL_MATCH_TAG;
	ign.data.rank   = (SCTK_MSG_SRC_TASK(msg) == MPC_ANY_SOURCE) ? SCTK_PTL_IGN_RANK : SCTK_PTL_MATCH_RANK;
	ign.data.uid    = SCTK_PTL_IGN_UID;
	ign.data.type   = SCTK_PTL_MATCH_TYPE;

	/* complete the ME data, this ME will be appended to the PRIORITY_LIST
	 * Here, we want a CT event attached to this ME (for triggered Op)
	 */
	put_flags = SCTK_PTL_ME_PUT_FLAGS | SCTK_PTL_ONCE;
	put_request  = sctk_ptl_me_create(NULL, sizeof(size_t), SCTK_PTL_ANY_PROCESS, match, ign, put_flags);
	/* when dealing with RDV, the first PUT will transfer the total size to send
	 * As we prepare a buffer in the local_data to receive it (but me_create()
	 * allocates it, we need to set up AFTER calling the IFACE function
	 */
	 put_request->slot.me.start = &put_request->req_sz;

	assert(put_request);
	assert(pte);

	put_request->msg       = msg;
	put_request->list      = SCTK_PTL_PRIORITY_LIST;
	put_request->type      = SCTK_PTL_TYPE_STD;
	put_request->prot      = SCTK_PTL_PROT_RDV;
	msg->tail.ptl.user_ptr = NULL; /* set w/ the GET request when the PUT will be received */

	/* this should be the last operation, to optimize the triggeredOps use */
	sctk_ptl_me_register(srail, put_request, pte);

	mpc_common_debug("PORTALS: NOTIFY-RECV-RDV from %llu (idx=%llu, match=%s, ign=%s, msg=%p)", SCTK_MSG_SRC_PROCESS_UID(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), __sctk_ptl_match_str(malloc(32), 32, ign.raw), msg);
}

/**
 * Function called by sctk_ptl_toolkit.c when an incoming ME event is issued to the RDV protocol.
 *
 * \param[in] rail the Portals rail
 * \param[in] ev the generated event
 */
void sctk_ptl_rdv_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* ptr = (sctk_ptl_local_data_t*) ev.user_ptr;
	int cur = 0;

	switch(ev.type)
	{
		case PTL_EVENT_PUT_OVERFLOW:         /* a previous received PUT matched a just appended ME */
		case PTL_EVENT_PUT:                  /* a Put() reached the local process */
			/* we don't care about unexpected messaged reaching the OVERFLOW_LIST, we will just wait for their local counter-part */
			/* indexes from 0 to SCTK_PTL_PTE_HIDDEN-1 maps RECOVERY, CM & RDMA queues
			 * indexes from SCTK_PTL_PTE_HIDDEN to N maps communicators
			 */
			sctk_ptl_rdv_recv_message(rail, ev);
			break;

		case PTL_EVENT_GET:                  /* a remote process get the data back */
			cur = OPA_fetch_and_decr_int(&ptr->cnt_frag) - 1;
			/* fun fact here...
			 * We set the cnt_frag differently, depending on the number of chunkds :
			 *  - if only one fragment, an ME w/ PTL_USE_ONCE has been set and there is nothing to do anymore
			 *  - Otherwise (multiple requests), the allocated ME needs to be manually freed to avoid leaks.
			 *
			 *  For the first case, cnt_frag is set to a negative value, otherwis, it contains the number of
			 *  chunks to process before releasing the message
			 */
			if(cur <= 0) /* if lower or equal to zero, all chunks have be consumed -> release the message */
			{
				sctk_ptl_rdv_complete_message(rail, ev);
				if(cur == 0) /* special case : when only one chunk is allocated, cnt_frag is negative to avoid entering this condition */
				{
					sctk_ptl_me_release(ptr);
					sctk_ptl_me_free(ptr, 0);
				}
			}

			break;
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
 * Function called by sctk_ptl_toolkit.c when an incoming MD event is for a RDV msg.
 * \param[in] rail the Portals rail
 * \param[in] ev the generated event
 */
void sctk_ptl_rdv_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_ptl_local_data_t* ptr = (sctk_ptl_local_data_t*) ev.user_ptr;



	int cur = 0;
	switch(ev.type)
	{
		case PTL_EVENT_ACK:   /* a PUT reached a remote process */
			/* just release the PUT MD, (no memory to free) */
			sctk_ptl_md_release(ptr);
			break;

		case PTL_EVENT_REPLY: /* a GET operation completed */
			cur = OPA_fetch_and_decr_int(&ptr->cnt_frag) - 1;
			if(cur <= 0)
			{
				sctk_ptl_rdv_reply_message(rail, ev);
				sctk_ptl_md_release(ptr);
			}
			break;
		case PTL_EVENT_SEND: /* a Put() left the local process */
			not_reachable();
		default:
			mpc_common_debug_fatal("Unrecognized MD event: %d", ev.type);
			break;
	}
}

#endif /* MPC_USE_PORTALS */
