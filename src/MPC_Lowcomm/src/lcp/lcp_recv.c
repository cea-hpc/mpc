#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_header.h"
#include "lcp_prototypes.h"
#include "lcp_rndv.h"
#include "lcp_datatype.h"
#include "lcp_task.h"

#include "sctk_alloc.h"

int lcp_tag_recv_nb(lcp_task_h task, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param)
{
	int rc;
	lcp_unexp_ctnr_t *match;
	sctk_rail_info_t *iface;
	lcp_request_t *req;
        lcp_context_h ctx = task->ctx;

	rc = lcp_request_create(&req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not create request.");
		return MPC_LOWCOMM_ERROR;
	}
        req->flags |= LCP_REQUEST_MPI_COMPLETE;
        LCP_REQUEST_INIT_RECV(req, ctx, task, request, param->recv_info,
                              count, buffer, param->datatype);
	lcp_request_init_tag_recv(req, param->recv_info);

	iface = ctx->resources[ctx->priority_rail].iface;
	if (LCR_IFACE_IS_TM(iface) && (ctx->config.offload ||
            (param->flags & LCP_REQUEST_TRY_OFFLOAD))) {

                req->state.offloaded = 1;
		rc = lcp_recv_tag_zcopy(req, iface);

		return rc;
	}

        mpc_common_debug_info("LCP: post recv am comm=%d, src=%d, tag=%d, length=%d, lcreq=%p",
                              req->recv.tag.comm, req->recv.tag.src_tid, 
                              req->recv.tag.tag, count, req->request);

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

	if (match->flags & LCP_RECV_CONTAINER_UNEXP_RPUT) {
		mpc_common_debug_info("LCP: matched rndv unexp req=%p, flags=%x", 
				      match, match->flags);
		rc = lcp_rndv_matched(req, (lcp_rndv_hdr_t *)(match + 1),
                                      match->length - sizeof(lcp_rndv_hdr_t), LCP_RNDV_PUT);
        } else if (match->flags & LCP_RECV_CONTAINER_UNEXP_RGET) {
		mpc_common_debug_info("LCP: matched rndv unexp req=%p, flags=%x", 
				      match, match->flags);
		rc = lcp_rndv_matched(req, (lcp_rndv_hdr_t *)(match + 1),
                                      match->length - sizeof(lcp_rndv_hdr_t),
                                      LCP_RNDV_GET);
	} else if (match->flags & LCP_RECV_CONTAINER_UNEXP_TAG) {
                lcp_tag_hdr_t *hdr = (lcp_tag_hdr_t *)(match + 1);

                mpc_common_debug("LCP: matched tag unexp req=%p, src=%d, "
                                 "tag=%d, comm=%d", req, hdr->src_tid, hdr->tag, 
                                 hdr->comm);
		/* copy data to receiver buffer and complete request */
                lcp_datatype_unpack(req->ctx, req, req->datatype, 
                                    req->recv.buffer, (void *)(hdr + 1),
                                    match->length - sizeof(lcp_tag_hdr_t));

                /* set recv info for caller */
                req->info->length = match->length - sizeof(lcp_tag_hdr_t);
                req->info->src    = hdr->src_tid;
                req->info->tag    = hdr->tag;

                //TODO: free match structure ??
		lcp_request_complete(req);
	} else {
		mpc_common_debug_error("LCP: unkown match flag=%x.", match->flags);
		rc = MPC_LOWCOMM_ERROR;
	}

	sctk_free(match);

	return rc;
}
