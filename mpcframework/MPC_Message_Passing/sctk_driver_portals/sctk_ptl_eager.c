#ifdef MPC_USE_PORTALS
#include "sctk_route.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_eager.h"
#include "sctk_net_tools.h"

void sctk_ptl_eager_free_memory(void* msg)
{
}

void sctk_ptl_eager_message_copy(sctk_message_to_copy_t* msg)
{
	sctk_ptl_local_data_t* send_data = msg->msg_send->tail.ptl.user_ptr;
	
	if(!msg->msg_send->tail.ptl.copy && msg->msg_recv.tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
	{
		sctk_message_completion_and_free(msg->msg_send, msg->msg_recv);
	}
	else
	{
		/* complete_and_free() called by this function */
		sctk_net_message_copy_from_buffer(send_data->slot.me.start, msg, 1);
	}
}

void sctk_ptl_eager_recv_message(sctk_rail_info_t* rail, sctk_ptl_event_t ev)
{
	sctk_thread_ptp_message_t* net_msg = sctk_malloc(sizeof(sctk_thread_ptp_message_t));
	sctk_ptl_matchbits_t match = (sctk_ptl_matchbits_t) ev.match_bits;

	sctk_assert(ev.ni_fail_type == PTL_NI_OK);
	/* we have to rebuild a msg for the inter_thread_comm */
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

	net_msg->tail.ptl.user_ptr = (sctk_ptl_local_data_t*)ev.user_ptr;
	net_msg->tail.ptl.copy = (net_msg->tail.ptl.user_ptr->slot.me.start != ev.start); /* Check if this recv is a late one */
	net_msg->tail.ptl.user_ptr->slot.me.start = ev.start; /* in any case, save where data really resides */

	sctk_rebuild_header(net_msg);

	sctk_debug("PORTALS: RECV-EAGER from %d (idx=%d, match=%s, size=%lu) -> %p", SCTK_MSG_SRC_TASK(net_msg), ev.pt_index, __sctk_ptl_match_str(malloc(32), 32, match.raw), ev.mlength, ev.start);
	rail->send_message_from_network(net_msg);
}

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


	assert(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS); /* temp */
	start = msg->tail.message.contiguous.addr;

	size = SCTK_MSG_SIZE(msg);
	flags = SCTK_PTL_MD_PUT_FLAGS;
	match.data.tag = SCTK_MSG_TAG(msg);
	match.data.rank = SCTK_MSG_SRC_PROCESS(msg);
	match.data.uid = SCTK_MSG_NUMBER(msg);

	pte = srail->pt_entries + SCTK_MSG_COMMUNICATOR(msg);
	remote = infos->dest;
	request = sctk_ptl_md_create(srail, start, size, flags);
	request->msg = msg;

	sctk_ptl_md_register(srail, request);
	sctk_debug("PORTALS: SEND-EAGER to %d (idx=%d, match=%s, sz=%llu)", SCTK_MSG_DEST_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), size);
	sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0, SCTK_MSG_SPECIFIC_CLASS(msg));
}
#endif
