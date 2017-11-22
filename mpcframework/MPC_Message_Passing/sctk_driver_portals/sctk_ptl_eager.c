#ifdef MPC_USE_PORTALS
#include "sctk_route.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_eager.h"
#include "sctk_net_tools.h"

void sctk_ptl_eager_free_memory(void* msg)
{
	sctk_free(msg);
}

/** What to do when a network message matched a locally posted request ?.
 *  
 *  \param[in,out] msg the send/recv bundle where both message headers are stored
 */
void sctk_ptl_eager_message_copy(sctk_message_to_copy_t* msg)
{
	sctk_ptl_local_data_t* send_data = msg->msg_send->tail.ptl.user_ptr;
	
	/* We ensure data are already stored in the user buffer (zero-copy) only if the two condition are met:
	 *  1 - The message is a contiguous one (the locally-posted request is contiguous, the network request
	 *      is always contiguous).
	 *  2 - The recv has been posted before the network message arrived. This means that Portals directly
	 *      matched the request with a PRIORITY_LIST ME.
	 */
	if(msg->msg_send->tail.ptl.copy || msg->msg_recv->tail.ptl.copy)
	{
		/* here, we have to copy the message from the network buffer to the user buffer */
		sctk_net_message_copy_from_buffer(send_data->slot.me.start, msg, 0);
		/*TODO: free the memory */
	}
	else
	{
	}
	sctk_ptl_me_release(msg->msg_recv->tail.ptl.user_ptr);
	/* flag request as completed */
	sctk_complete_and_free_message(msg->msg_send);
	sctk_complete_and_free_message(msg->msg_recv);
}

/**
 * Recv an eager message from network.
 * \param[in] rail the Portals rail
 * \param[in] ev  the de-queued event, mapping a network message occurence.
 */
void sctk_ptl_eager_recv_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_thread_ptp_message_t* net_msg = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
	sctk_ptl_matchbits_t match = (sctk_ptl_matchbits_t) ev.match_bits;

	/* sanity checks */
	sctk_assert(rail);
	sctk_assert(ev.ni_fail_type == PTL_NI_OK);
	sctk_assert(ev.mlength <= rail->network.ptl.eager_limit);
	
	/* rebuild a complete MPC header msg (inter_thread_comm needs it) */
	sctk_init_header(net_msg, SCTK_MESSAGE_CONTIGUOUS , sctk_ptl_eager_free_memory, sctk_ptl_eager_message_copy);
	SCTK_MSG_SRC_PROCESS_SET     ( net_msg ,  match.data.rank);
	SCTK_MSG_SRC_TASK_SET        ( net_msg ,  match.data.rank);
	SCTK_MSG_DEST_PROCESS_SET    ( net_msg ,  sctk_get_process_rank());
	SCTK_MSG_DEST_TASK_SET       ( net_msg ,  sctk_get_process_rank());
	SCTK_MSG_COMMUNICATOR_SET    ( net_msg ,  ev.pt_index - SCTK_PTL_PTE_HIDDEN);
	SCTK_MSG_TAG_SET             ( net_msg ,  match.data.tag);
	SCTK_MSG_NUMBER_SET          ( net_msg ,  match.data.uid);
	SCTK_MSG_MATCH_SET           ( net_msg ,  0);
	SCTK_MSG_SPECIFIC_CLASS_SET  ( net_msg ,  ev.hdr_data);
	SCTK_MSG_SIZE_SET            ( net_msg ,  ev.mlength);
	SCTK_MSG_COMPLETION_FLAG_SET ( net_msg ,  NULL);

	/* save the Portals context in the tail */
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

	/* finish creating an MPC message heder */
	sctk_rebuild_header(net_msg);

	sctk_debug("PORTALS: RECV-EAGER from %d (idx=%d, match=%s, size=%lu) -> %p", SCTK_MSG_SRC_TASK(net_msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, match.raw), ev.mlength, ev.start);

	/* notify ther inter_thread_comm a new message has arrived */
	rail->send_message_from_network(net_msg);
}

/**
 * Send an eager message.
 *
 * \param[in] msg the message to send
 * \param[in] endpoint the route to use
 */
void sctk_ptl_eager_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
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

	/* prepare the Put() MD */
	size            = SCTK_MSG_SIZE(msg);
	flags           = SCTK_PTL_MD_PUT_FLAGS;
	match.data.tag  = SCTK_MSG_TAG(msg);
	match.data.rank = SCTK_MSG_SRC_PROCESS(msg);
	match.data.uid  = SCTK_MSG_NUMBER(msg);
	pte             = srail->pt_entries + SCTK_MSG_COMMUNICATOR(msg);
	remote          = infos->dest;
	request         = sctk_ptl_md_create(srail, start, size, flags);

	/* double-linking */
	request->msg           = msg;
	msg->tail.ptl.user_ptr = request;

	/* emit the request */
	sctk_ptl_md_register(srail, request);
	sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, SCTK_MSG_SPECIFIC_CLASS(msg));
	
	sctk_debug("PORTALS: SEND-EAGER to %d (idx=%d, match=%s, sz=%llu)", SCTK_MSG_DEST_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), size);
}
#endif
