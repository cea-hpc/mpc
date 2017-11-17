#include "sctk_route.h"
#include "sctk_ptl_rdv.h"
#include "sctk_ptl_iface.h"

void sctk_ptl_rdv_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint)
{
	/* pack the data if necessary */
	/*void* user_data;*/
	/*if(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS)*/
	/*{*/
		/*user_data = msg->tail.message.contiguous.addr;*/
	/*}*/
	/*else*/
	/*{*/
		/*user_data = sctk_malloc(SCTK_MSG_SIZE(msg));*/
		/*sctk_net_copy_in_buffer(msg, user_data);*/
	/*}*/

	sctk_ptl_rail_info_t* srail    = &endpoint->rail->network.ptl;
	sctk_ptl_route_info_t* infos   = &endpoint->data.ptl;
	sctk_ptl_local_data_t *md_request , *me_request;
	void  *md_start                   , *me_start;
	size_t md_size                    , me_size;
	int md_flags                      , me_flags;
	sctk_ptl_matchbits_t md_match     , me_match, me_ign;
	sctk_ptl_pte_t *md_pte            , *me_pte;
	sctk_ptl_id_t *md_remote          , *me_remote;

	md_request = me_request = NULL;
	md_match   = me_match   = me_ign = SCTK_PTL_MATCH_INIT;
	md_pte     = me_pte     = NULL;
	md_remote  = me_remote  = SCTK_PTL_ANY_PROCESS;
	md_start   = me_start   = NULL;
	md_size    = me_size    = 0;
	md_flags   = me_flags   = 0;

	/* Configure the Put() */
	md_flags = SCTK_PTL_MD_PUT_FLAGS;
	md_match.data.tag = SCTK_MSG_TAG(msg);
	md_match.data.rank = SCTK_MSG_SRC_TASK(msg);
	md_pte = srail->pt_entries + SCTK_MSG_COMMUNICATOR(msg);
	md_remote = infos->dest;
	md_request = sctk_ptl_md_create(srail, md_start, md_size, md_flags);
	md_request->msg = msg;
	sctk_ptl_md_register(srail, md_request);

	/* prepare for the Get() */
	assert(msg->tail.message_type == SCTK_MESSAGE_CONTIGUOUS); /* temp */
	me_start = msg->tail.message.contiguous.addr;
	me_size = SCTK_MSG_SIZE(msg);
	me_flags = SCTK_PTL_ME_GET_FLAGS;
	me_match.data.tag = SCTK_MSG_TAG(msg);
	me_match.data.rank = SCTK_MSG_SRC_TASK(msg);
	me_pte = srail->pt_entries + SCTK_MSG_COMMUNICATOR(msg);
	me_remote = srail->id;
	me_request = sctk_ptl_me_create(me_start, me_size, me_remote, me_match.raw, me_ign.raw, me_flags);

	/* the MEappend and the Put() should be done atomically to preserve order */
	sctk_ptl_me_register(srail, me_user, me_pte);

	sctk_error("Posted a rdv send to %d (nid/pid=%llu/%llu, idx=%d, match=%llu)", SCTK_MSG_DEST_TASK(msg), remote.phys.nid, remote.phys.pid, pte->idx, match.raw);
	sctk_ptl_emit_put(request, size, remote, pte, match);
}
