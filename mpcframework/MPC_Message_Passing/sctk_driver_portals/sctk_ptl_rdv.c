#ifdef MPC_USE_PORTALS
#include "sctk_route.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_iface.h"

void sctk_ptl_rdv_recv_message(sctk_rail_info_t* srail, sctk_ptl_event_t ev)
{
	not_implemented();
}

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
	md_request         = sctk_ptl_md_create(srail, NULL, 0, md_flags);
	
	/* prepare for the Get() */
	me_flags           = SCTK_PTL_ME_GET_FLAGS | SCTK_PTL_ONCE;
	me_request         = sctk_ptl_me_create(start, size, remote, match, ign, me_flags);

	/* double-linking */
	md_request->msg        = msg;
	me_request->msg        = msg;
	msg->tail.ptl.user_ptr = md_request;
	hdr.rdv.datatype       = SCTK_MSG_SPECIFIC_CLASS(msg);

	sctk_ptl_md_register(srail, md_request);
	sctk_ptl_me_register(srail, me_request, pte);
	sctk_ptl_emit_put(md_request, 0, remote, pte, match, 0, 0, hdr.raw, md_request); /* empty Put() */
	
	/* TODO: Need to handle the case where the data is larger than the max ME size */
	sctk_warning("PORTALS: SEND-RDV to %d (idx=%d, match=%s, sz=%llu)", SCTK_MSG_DEST_TASK(msg), pte->idx, __sctk_ptl_match_str(malloc(32), 32, match.raw), size);
}

void sctk_ptl_rdv_notify_recv(sctk_thread_ptp_message_t* msg, sctk_ptl_rail_info_t* srail)
{
	void  *start = NULL;
	/* is the message contiguous ? Maybe we could try to get an IOVEC instead of packing ? */
	if(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)
	{
		start = msg->tail.message.contiguous.addr;
	}
	else
	{
		/* if not, we map a temporary buffer, ready for network serialized data */
		start = sctk_malloc(SCTK_MSG_SIZE(msg));
		sctk_net_copy_in_buffer(msg, start);
		/* and we flag the copy to 1, to remember some buffers will have to be recopied */
		msg->tail.ptl.copy = 1;
	}

}

#endif
