#ifdef MPC_USE_PORTALS
#include "sctk_route.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_eager.h"

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
	match.data.rank = SCTK_MSG_SRC_TASK(msg);
	pte = srail->pt_entries + SCTK_MSG_COMMUNICATOR(msg);
	remote = infos->dest;
	request = sctk_ptl_md_create(srail, start, size, flags);
	request->msg = msg;

	sctk_ptl_md_register(srail, request);
	sctk_error("Posted a eager send to %d (nid/pid=%llu/%llu, idx=%d, match=%llu)", SCTK_MSG_DEST_TASK(msg), remote.phys.nid, remote.phys.pid, pte->idx, match.raw);
	sctk_ptl_emit_put(request, size, remote, pte, match, 0, 0);
}
#endif
