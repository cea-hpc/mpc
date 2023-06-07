#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_header.h"
#include "lcp_tag.h"
#include "lcp_tag_offload.h"
#include "lcp_rndv.h"
#include "lcp_datatype.h"
#include "lcp_task.h"

#include "mpc_common_debug.h"
#include "sctk_alloc.h"
#include "lcp_tag.h"

int lcp_tag_recv_nb(lcp_task_h task, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param)
{
	int rc;
	lcp_unexp_ctnr_t *match;
	sctk_rail_info_t *iface;
	lcp_request_t *req;
        lcp_context_h ctx = task->ctx;

	// create a request to be matched with the received message
	rc = lcp_request_create(&req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not create request.");
		return  MPC_LOWCOMM_ERROR;
	}
        req->flags |= LCP_REQUEST_MPI_COMPLETE;
        LCP_REQUEST_INIT_TAG_RECV(req, ctx, task, request, param->recv_info,
                                  count, buffer, param->datatype);

	// get interface for the request to go through
	iface = ctx->resources[ctx->priority_rail].iface;

	// if we have to try offload
	if (LCR_IFACE_IS_TM(iface) && (ctx->config.offload ||
		(param->flags & LCP_REQUEST_TRY_OFFLOAD))) {

		req->state.offloaded = 1;
		// try to receive using zero copy
		rc = lcp_recv_tag_zcopy(req, iface);

		return rc;
	}

        mpc_common_debug_info("LCP: post recv am comm=%d, src=%d, tag=%d, length=%d, req=%p",
                              req->recv.tag.comm, req->recv.tag.src_tid, 
                              req->recv.tag.tag, count, req);

        req->state.offloaded = 0;

	LCP_TASK_LOCK(task);
	match = lcp_match_umq(task->umq_table,
			      req->recv.tag.comm,
			      req->recv.tag.tag,
			      req->recv.tag.src_tid);
	if (match == NULL) {
		lcp_append_prq(task->prq_table, req,
			       req->recv.tag.comm,
			       req->recv.tag.tag,
			       req->recv.tag.src_tid);

                LCP_TASK_UNLOCK(task);
		return MPC_LOWCOMM_SUCCESS;
	}

	LCP_TASK_UNLOCK(task);

	if (match->flags & LCP_RECV_CONTAINER_UNEXP_RNDV_TAG) {
		mpc_common_debug_info("LCP: matched rndv unexp req=%p, flags=%x", 
				      match, match->flags);

                lcp_recv_rndv_tag_data(req, match + 1);
                rc = lcp_rndv_process_rts(req, match + 1,
                                          match->length - sizeof(lcp_rndv_hdr_t));

	} else if (match->flags & LCP_RECV_CONTAINER_UNEXP_EAGER_TAG) {
                /* If synchronization was asked, send ack message */
		if(match->flags & LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC)
			lcp_send_eager_sync_ack(req, match + 1);

                rc = lcp_recv_eager_tag_data(req, match + 1, 
                                             match->length - sizeof(lcp_tag_hdr_t));

                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not unpack unexpected "
                                               "data.");
                }
	} else {
		mpc_common_debug_error("LCP: unkown match flag=%x.", match->flags);
		rc = MPC_LOWCOMM_ERROR;
	}
	sctk_free(match);

	return rc;
}
