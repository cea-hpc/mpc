#include "lcp_ep.h"
#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_pending.h"
#include "lcp_header.h"
#include "lcp_proto.h"

#include "sctk_alloc.h"

static inline int lcp_send_start_frag(lcp_ep_h ep, lcp_request_t *req)
{
	int rc;

	mpc_common_debug_info("LCP: send rndv req=%p, comm_id=%lu, tag=%d, "
			      "src=%d, dest=%d, msg_id=%d.", req, req->send.tag.comm_id, 
			      req->send.tag.tag, req->send.tag.src_tsk, 
			      req->send.tag.dest_tsk, req->msg_id);

        req->state.status = MPC_LOWCOMM_LCP_RPUT_SYNC;
	req->flags        |= LCP_REQUEST_SEND_FRAG;

	/* register req request inside table */
	if (lcp_pending_create(ep->ctx->pend_send_req, req, 
				     req->msg_id) == NULL) {
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	rc = lcp_proto_send_do_rndv(req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not start rndv.");
		goto err;
	}

	rc = lcp_proto_recv_do_ack(req, ep->lct_eps[ep->priority_chnl]->rail);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not post ack.");
	}

err:
	return rc;
}

static inline int lcp_send_start_single(lcp_ep_h ep, lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	_mpc_lowcomm_endpoint_t *lcr_ep;

	lcr_ep  = ep->lct_eps[ep->priority_chnl];
	if (LCR_IFACE_IS_TM(lcr_ep->rail)) {
		rc = lcp_send_do_tag_offload_post(ep, req);
	} else {
		rc = lcp_send_do_tag(ep, 0, req);
	}

	if (rc == MPC_LOWCOMM_INPROGRESS) {
	 	req->state.status = MPC_LOWCOMM_LCP_PEND;
		req->flags |= LCP_REQUEST_SEND_CTRL; /* to delete ctrl msg upon completion */
		if (lcp_pending_create(ep->ctx->pend_send_req, req, 
					     req->msg_id) == NULL) {
			rc = MPC_LOWCOMM_ERROR;
		}
		mpc_common_debug("LCP: pending req dest=%d, msg_id=%llu", 
				 req->send.tag.dest, req->msg_id);
	}

	return rc;
}

int lcp_send_start(lcp_ep_h ep, lcp_request_t *req) {
        int rc;
	struct lcp_ep_config ep_config = ep->ep_config;

	if (req->send.length < ep_config.rndv_threshold) {
		rc = lcp_send_start_single(ep, req); 
        } else {
		rc = lcp_send_start_frag(ep, req);
        }

        return rc;
}

int lcp_send(lcp_ep_h ep, mpc_lowcomm_request_t *request, 
	     void *buffer, uint64_t seqn)
{
	int rc;
	lcp_request_t *req;

	uint64_t msg_id = lcp_rand_uint64();

	rc = lcp_request_create(&req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not create request.");
		return MPC_LOWCOMM_ERROR;
	}
	lcp_request_init_send(req, ep, request, buffer, seqn, msg_id);
        req->flags |= LCP_REQUEST_MPI_COMPLETE;

	if (ep->state == LCP_EP_FLAG_CONNECTING) {
	 	req->state.status = MPC_LOWCOMM_LCP_PEND;
		req->flags |= LCP_REQUEST_SEND_CTRL; /* to delete ctrl msg upon completion */
                if (lcp_pending_create(ep->ctx->pend_send_req, req, 
                                       req->msg_id) == NULL) {
			rc = MPC_LOWCOMM_ERROR;
		}
		mpc_common_debug("LCP: pending req dest=%d, msg_id=%llu", 
				 req->send.tag.dest, msg_id);
		return rc;
	}

        rc = lcp_send_start(ep, req);

	return rc;
}
