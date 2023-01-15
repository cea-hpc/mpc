#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_header.h"
#include "lcp_prototypes.h"
#include "lcp_rndv.h"

#include "sctk_alloc.h"

int lcp_tag_recv_nb(lcp_context_h ctx, void *buffer, size_t count, 
                    mpc_lowcomm_request_t *request,
                    lcp_request_param_t *param)
{
	int rc;
	lcp_unexp_ctnr_t *match;
	sctk_rail_info_t *iface;
	lcp_request_t *req;

        //FIXME: could the lock be move and placed before lcp_match_umq ?
	LCP_CONTEXT_LOCK(ctx);

	rc = lcp_request_create(&req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not create request.");
		return MPC_LOWCOMM_ERROR;
	}
        req->flags |= LCP_REQUEST_MPI_COMPLETE;
        LCP_REQUEST_INIT_RECV(req, ctx, request, count, buffer);
	lcp_request_init_tag_recv(req, param->recv_info);

	iface = ctx->resources[ctx->priority_rail].iface;
	if (LCR_IFACE_IS_TM(iface) && (ctx->config.offload ||
            (param->flags & LCP_REQUEST_TRY_OFFLOAD))) {
                req->state.offloaded = 1;
		rc = lcp_recv_tag_zcopy(req, iface);

		LCP_CONTEXT_UNLOCK(ctx);

		return rc;
	}

        req->state.offloaded = 0;
	match = lcp_match_umq(ctx->umq_table,
			      (int16_t)req->recv.tag.comm_id,
			      (int32_t)req->recv.tag.tag,
			      (int32_t)req->recv.tag.src);
	if (match == NULL) {
		lcp_append_prq(ctx->prq_table, req,
			       (int16_t)req->recv.tag.comm_id,
			       (int32_t)req->recv.tag.tag,
			       (int32_t)req->recv.tag.src);

		LCP_CONTEXT_UNLOCK(ctx);
		return MPC_LOWCOMM_SUCCESS;
	}

	//NOTE: unlock context to enable endpoint creation in rndv_matched
	LCP_CONTEXT_UNLOCK(ctx);
	if (match->flags & LCP_RECV_CONTAINER_UNEXP_RPUT) {
		mpc_common_debug_info("LCP: matched rndv unexp req=%p, flags=%x", 
				      match, match->flags);
		rc = lcp_rndv_matched(ctx, req, (lcp_rndv_hdr_t *)(match + 1),
                                      LCP_RNDV_PUT);
        } else if (match->flags & LCP_RECV_CONTAINER_UNEXP_RGET) {
		mpc_common_debug_info("LCP: matched rndv unexp req=%p, flags=%x", 
				      match, match->flags);
		rc = lcp_rndv_matched(ctx, req, (lcp_rndv_hdr_t *)(match + 1),
                                      LCP_RNDV_GET);
	} else if (match->flags & LCP_RECV_CONTAINER_UNEXP_TAG) {
		mpc_common_debug("LCP: matched tag unexp req=%p, flags=%x", req, 
				 match->flags);
                lcp_tag_hdr_t *hdr = (lcp_tag_hdr_t *)(match + 1);

		/* copy data to receiver buffer and complete request */
                memcpy(req->recv.buffer, (void *)(hdr + 1), 
                       match->length - sizeof(lcp_tag_hdr_t));

                /* set recv info for caller */
                req->recv.recv_info->length = match->length - sizeof(lcp_tag_hdr_t);
                req->recv.recv_info->src    = hdr->src;
                req->recv.recv_info->tag    = hdr->tag;

		lcp_request_complete(req);
	} else {
		mpc_common_debug_error("LCP: unkown match flag=%x.", match->flags);
		rc = MPC_LOWCOMM_ERROR;
	}

	sctk_free(match);

	return rc;
}
