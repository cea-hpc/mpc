#include "lcp_proto.h"

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_tag_offload.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

//NOTE: completion callback are used:
//      1) for send zcopy (tag-matching and no tag-matching)
//      2) for send bcopy (tag-matching)
//      But not used for recv since handler is called and will
//      handle request completion. 
void lcp_request_complete_rndv(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        req->state.status = MPC_LOWCOMM_LCP_RPUT_SYNC;
}

void lcp_request_complete_ack(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        req->state.status = MPC_LOWCOMM_LCP_DONE;
	lcp_request_complete(req);
}

void lcp_request_complete_frag(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);
	//FIXME: is lock needed here?
	req->state.remaining -= comp->sent;
	mpc_common_debug("LCP: req=%p, remaining=%d, frag len=%lu.", 
			 req, req->state.remaining, comp->sent);
	if (req->state.remaining == 0) {
                req->state.status = MPC_LOWCOMM_LCP_DONE;
		lcp_request_complete(req);
	}
}

static size_t lcp_proto_rndv_pack(void *dest, void *data)
{
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->base.base.comm_id = req->send.tag.comm_id;
	hdr->base.dest         = req->send.tag.dest;
	hdr->base.dest_tsk     = req->send.tag.dest_tsk;
	hdr->base.src          = req->send.tag.src;
	hdr->base.src_tsk      = req->send.tag.src_tsk;
	hdr->base.tag          = req->send.tag.tag;
	hdr->base.seqn         = req->msg_number;
	
	hdr->dest       = req->send.tag.dest;
	hdr->msg_id     = req->msg_id;
	hdr->total_size = req->send.length;

	return sizeof(*hdr);
}

static size_t lcp_proto_ack_pack(void *dest, void *data)
{
	lcp_rndv_ack_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->base.base.comm_id = req->send.am.comm_id;
	hdr->base.am_id = req->send.am.am_id;
	
	hdr->dest       = req->send.am.dest;
	hdr->msg_id     = req->msg_id;

	return sizeof(*hdr);
}

__UNUSED__ static size_t lcp_proto_frag_pack(void *dest, void *data)
{
	lcp_frag_hdr_t *hdr = dest;
	lcp_context_multi_t *m_ctx  = data;
	lcp_request_t *req = m_ctx->req;

	hdr->base.base.comm_id = req->send.tag.comm_id;
	hdr->base.am_id = MPC_LOWCOMM_FRAG_MESSAGE;
	
	hdr->dest       = req->send.tag.dest;
	hdr->msg_id     = req->msg_id;
	hdr->offset     = m_ctx->offset;

	memcpy(hdr + 1, m_ctx->req->send.buffer + m_ctx->offset, m_ctx->length);

	return sizeof(*hdr);
}

int lcp_proto_send_do_rndv(lcp_request_t *req)
{
	int rc;
	lcp_chnl_idx_t cc = req->send.cc;
	lcp_ep_h ep = req->send.ep;

	if (LCR_IFACE_IS_TM(ep->lct_eps[cc]->rail)) {
		lcp_tm_imm_t imm = { .hdr = {
			.prot = MPC_LOWCOMM_RDV_TM_MESSAGE,
			.seqn = req->msg_number }
		};
		lcr_completion_t cp = {
			.comp_cb = lcp_request_complete_rndv 
		};
		req->send.t_ctx.comp = cp;
		req->flags |= LCP_REQUEST_OFFLOADED;
                rc = lcp_send_do_tag_offload_bcopy(req, imm.raw, 
                                                   lcp_proto_rndv_pack);
	} else {
		lcr_completion_t cp = { 
			.comp_cb = lcp_request_complete_rndv 
		};
		req->send.t_ctx.comp = cp;
		rc = lcp_send_do_tag_bcopy(ep, MPC_LOWCOMM_RDV_MESSAGE,
					       lcp_proto_rndv_pack, req);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not send rndv req=%p, "
                                               "msg_id=%llu.", req, req->msg_id);
                }

	}

	return rc;
}

int lcp_proto_send_do_ack(lcp_ep_h ep, lcp_request_t *req)
{
	int rc;
	lcp_chnl_idx_t cc = req->send.cc;

	if (LCR_IFACE_IS_TM(ep->lct_eps[cc]->rail)) {
		lcr_completion_t cp = {
			.comp_cb = lcp_request_complete_ack
		};
		req->send.t_ctx.comp = cp;
		req->flags |= LCP_REQUEST_OFFLOADED;
		rc = lcp_send_do_ack_offload_post(req, lcp_proto_ack_pack);
	} else {
		rc = lcp_send_do_tag_bcopy(ep, MPC_LOWCOMM_ACK_MESSAGE, 
					       lcp_proto_ack_pack, req);	
	}

	return rc;
}

int lcp_proto_recv_do_ack(lcp_request_t *req, sctk_rail_info_t *iface)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	if (LCR_IFACE_IS_TM(iface)) {
		req->flags |= LCP_REQUEST_OFFLOADED;
		rc = lcp_recv_ack_offload_post(req, iface);
	}

	return rc;
}

int lcp_recv_do_zcopy_multi(lcp_ep_h ep, lcp_request_t *rreq)
{
	int rc;
	size_t remaining, offset, length, frag_length;
	int ifrag;
        _mpc_lowcomm_endpoint_t *lcr_ep;
        lcr_rail_attr_t attr;

	/* size from rndv has been set */
	assert(rreq->recv.send_length > 0);

	/* Fragment message */
	remaining = rreq->recv.send_length;
	offset = 0;
	ep->current_chnl = ep->priority_chnl;
	ifrag = 0;
	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.cap.rndv.max_put_zcopy;

		length = remaining < frag_length ? remaining : frag_length;

		if (LCR_IFACE_IS_TM(lcr_ep->rail)) {
			rc = lcp_recv_frag_offload_post(rreq, lcr_ep->rail, 
							offset, ifrag+1, length);
			if (rc != MPC_LOWCOMM_SUCCESS) {
				goto err;
			}
		}
		mpc_common_debug("LCP: recv frag n=%d, src=%d, dest=%d, msg_id=%llu, offset=%d, "
				 "len=%d", ifrag, rreq->recv.tag.src_tsk, rreq->recv.tag.dest_tsk,
				 rreq->msg_id, offset, length);

		offset += length;
		ep->current_chnl = (ep->current_chnl + 1) % ep->num_chnls;
		remaining = rreq->recv.send_length - offset;
		ifrag++;
	}

err:
	return rc;
}

int lcp_send_do_zcopy_multi(lcp_ep_h ep, lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	size_t frag_length, length;
	_mpc_lowcomm_endpoint_t *lcr_ep;
        lcr_rail_attr_t attr;

	/* get frag state */
	size_t offset    = req->state.offset;
	size_t remaining = req->send.length - offset;
	int f_id         = req->state.f_id;
	ep->current_chnl = req->state.cur;

	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.cap.rndv.max_put_zcopy;

		length = remaining < frag_length ? remaining : frag_length;

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, offset=%d, "
				 "len=%d, remaining=%d", f_id, req->send.tag.src_tsk, 
				 req->send.tag.dest_tsk, req->msg_id, offset, length,
				 remaining);
		if (LCR_IFACE_IS_TM(lcr_ep->rail)) {
			lcr_completion_t cp = {
				.comp_cb = lcp_request_complete_frag,
			};
			req->send.t_ctx.comp = cp;
			rc = lcp_send_do_frag_offload_post(req, offset, f_id+1, length);
		} else {
			lcr_completion_t cp = {
				.comp_cb = lcp_request_complete_frag,
			};
			req->send.t_ctx.comp = cp;
			rc = lcp_send_do_am_frag_zcopy(req, offset, length);
		}

		if (rc == MPC_LOWCOMM_INPROGRESS) {
			/* Save progress */
			req->state.offset    = offset;
			req->state.cur       = ep->current_chnl;
			req->state.f_id      = f_id;
			mpc_common_debug_info("LCP: fragmentation stalled, frag=%d, req=%p, msg_id=%llu, "
					"remaining=%d, offset=%d, ep=%d", f_id, req, req->msg_id, 
					remaining, offset, ep->current_chnl);
			return rc;
		} else if (rc == MPC_LOWCOMM_ERROR) {
			mpc_common_debug_error("LCP: could not send fragment %d, req=%p, "
					"msg_id=%llu.", f_id, req, req->msg_id);
			break;
		}	       

		offset += length;
		remaining -= length;

		ep->current_chnl = (ep->current_chnl + 1) % ep->num_chnls;
		f_id++;
	}

        req->state.status = MPC_LOWCOMM_LCP_DONE;

	return rc;
}

int lcp_rndv_matched(lcp_context_h ctx, lcp_request_t *rreq,
		     lcp_rndv_hdr_t *hdr) 
{
	int rc;
	lcp_ep_ctx_t *ctx_ep;
	lcp_ep_h ep;
	lcp_request_t *ack_req;

	/* Init all protocol data */
	rreq->msg_id            = hdr->msg_id;
	rreq->recv.send_length  = hdr->total_size; 
        rreq->state.remaining   = hdr->total_size;
	rreq->state.offset      = 0;
	rreq->msg_number        = hdr->base.seqn;
	rreq->state.status      = MPC_LOWCOMM_LCP_RPUT_FRAG;
	rreq->flags             |= LCP_REQUEST_RECV_FRAG;

	/* Get LCP endpoint if exists */
	HASH_FIND(hh, ctx->ep_ht, &hdr->base.src, sizeof(uint64_t), ctx_ep);
	if (ctx_ep == NULL) {
		rc = lcp_ep_create(ctx, &ep, hdr->base.src, 0);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			mpc_common_debug_error("LCP: could not create ep after match.");
			goto err;
		}
	} else {
		ep = ctx_ep->ep;
	}

	/* register request inside table */
        if (lcp_pending_create(ctx->pend_recv_req, rreq, 
                               rreq->msg_id) == NULL) {
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	/* Post receive if tag offload interface exists */
	rc = lcp_recv_do_zcopy_multi(ep, rreq);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto err;
	}

	/* Send ack */
	rc = lcp_request_create(&ack_req);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto err;
	}
	lcp_request_init_ack(ack_req, ep, 
                             hdr->base.base.comm_id, 
                             hdr->base.dest,
                             hdr->base.seqn,
                             hdr->msg_id);

	mpc_common_debug("LCP: send ack comm_id=%llu, dest=%d, msg_id=%llu",
			 ack_req->send.am.comm_id, ack_req->send.am.dest, 
			 ack_req->msg_id);
	rc = lcp_proto_send_do_ack(ep, ack_req);

err:
	return rc;
}

int lcp_rndv_tag_handler(void *arg, void *data,
			 size_t length, 
			 __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_rndv_hdr_t *hdr = data;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;

	/* No payload in rndv message for now */
	assert(length - sizeof(lcp_rndv_hdr_t) == 0);

	LCP_CONTEXT_LOCK(ctx);

	mpc_common_debug_info("LCP: recv rndv header src=%d, dest=%d, msg_id=%llu",
			      hdr->base.src_tsk, hdr->base.dest_tsk, hdr->msg_id);

	req = lcp_match_prq(ctx->prq_table, 
			    hdr->base.base.comm_id, 
			    hdr->base.tag, hdr->base.src);
	if (req != NULL) {
		mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%lu, " 
				 "tag=%d, src=%lu.", req, req->recv.tag.comm_id, 
				 req->recv.tag.tag, req->recv.tag.src);
		LCP_CONTEXT_UNLOCK(ctx); //NOTE: unlock context to enable endpoint creation.
		rc = lcp_rndv_matched(ctx, req, hdr);
	} else {
		lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
					    LCP_RECV_CONTAINER_UNEXP_RNDV);
		lcp_append_umq(ctx->umq_table, (void *)ctnr, hdr->base.base.comm_id, 
			       hdr->base.tag, hdr->base.src);
		LCP_CONTEXT_UNLOCK(ctx);
		rc = MPC_LOWCOMM_SUCCESS;
	}


	return rc;
}

static int lcp_rndv_tag_offload_handler(void *arg, void *data, size_t length,
					__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcr_tag_context_t *t_ctx = arg;
	lcp_context_h ctx = t_ctx->arg;
	lcp_rndv_hdr_t *hdr = data;
	lcp_request_t *req  = t_ctx->req;

	/* No payload in rndv message for now */
	assert(length == sizeof(lcp_rndv_hdr_t));

	mpc_common_debug_info("LCP: recv rndv offload header src=%d, dest=%d, msg_id=%llu",
			      hdr->base.src_tsk, hdr->base.dest_tsk, hdr->msg_id);


	rc = lcp_rndv_matched(ctx, req, hdr);

	return rc;
}

static int lcp_rndv_ack_tag_offload_handler(void *arg, void *data,
					    size_t size, 
					    __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcr_tag_context_t *t_ctx = arg;
	lcp_request_t *req = t_ctx->req;

        assert(size == 0 && data == NULL);

	mpc_common_debug_info("LCP: recv tag offload ack header msg_id=%llu",
                              req->msg_id);

	/* Update request state */
	req->state.status = MPC_LOWCOMM_LCP_RPUT_FRAG;

	return rc;
}

static int lcp_rndv_ack_tag_handler(void *arg, void *data,
				__UNUSED__ size_t size, 
				__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_rndv_ack_hdr_t *hdr = data;
	lcp_request_t *req;

	mpc_common_debug_info("LCP: recv tag ack header comm_id=%llu, src=%d, dest=%d, "
			      "msg_id=%llu.", hdr->base.base.comm_id, hdr->src, hdr->dest, 
			      hdr->msg_id);

	/* Retrieve request */
        req = lcp_pending_get_request(ctx->pend_send_req, hdr->msg_id);
	if (req == NULL) {
		mpc_common_debug_error("LCP: could not find ctrl msg: comm id=%llu, "
				       "msg id=%llu, src=%d, dest=%d.", 
				       hdr->base.base.comm_id, hdr->msg_id,
				       hdr->src, hdr->dest);
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	/* Update request state */
	req->state.status = MPC_LOWCOMM_LCP_RPUT_FRAG;
err:
	return rc;
}

int lcp_rndv_frag_handler(void *arg, void *data, 
			  size_t length,
			  __UNUSED__ unsigned flags)
{
	int rc;
	lcp_context_h ctx = arg;
	lcp_frag_hdr_t *hdr = data;
	int hdr_len = sizeof(lcp_frag_hdr_t);
	lcp_request_t *req;

        req = lcp_pending_get_request(ctx->pend_recv_req,
                                      hdr->msg_id);
	if (req == NULL) {
		mpc_common_debug_error("LCP: could not find lcp request. "
				       "uuid=%lu, seqn:%lu", 
				       hdr->base.base.comm_id,
				       hdr->msg_id);
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	mpc_common_debug("LCP: frag req=%p, remaining=%d, msg_id=%llu, len=%lu, n=%d.", 
			 req, req->state.remaining, length, hdr->msg_id,
			 hdr->offset);
	LCP_CONTEXT_LOCK(ctx);
	//FIXME: do we actually need this lock:
	//       - probably yes but only to update request state (btw, could not 
	//       find any locks in handlers in UCX codebase)
	lcp_request_update_state(req, length - hdr_len);
	LCP_CONTEXT_UNLOCK(ctx);

	memcpy(req->recv.buffer + hdr->offset, hdr + 1, length - sizeof(*hdr));

	if (req->state.status == MPC_LOWCOMM_LCP_DONE) {
		lcp_request_complete(req);
	}

	rc = MPC_LOWCOMM_SUCCESS;
err:
	return rc;
}

//NOTE: data pointer unused since matching of fragments are always in priority.
int lcp_rndv_tag_offload_frag_handler(void *arg, __UNUSED__ void *data, 
				      size_t length,
				      __UNUSED__ unsigned flags)
{
	int rc;
	lcr_tag_context_t *t_ctx = arg;
	lcp_context_h ctx = t_ctx->arg;
	lcp_request_t *req = t_ctx->req;

	LCP_CONTEXT_LOCK(ctx);
	lcp_request_update_state(req, length); 
	mpc_common_debug("LCP: req=%p, remaining=%d, len=%lu, n=%d.", 
			 req, req->state.remaining, length, 
			 t_ctx->tag.t_frag.f_id);
	LCP_CONTEXT_UNLOCK(ctx);

	if (req->state.status == MPC_LOWCOMM_LCP_DONE) {
		lcp_request_complete(req);
	}

	rc = MPC_LOWCOMM_SUCCESS;

	return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_ACK_MESSAGE, lcp_rndv_ack_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_ACK_TM_MESSAGE, lcp_rndv_ack_tag_offload_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RDV_MESSAGE, lcp_rndv_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RDV_TM_MESSAGE, lcp_rndv_tag_offload_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_FRAG_MESSAGE, lcp_rndv_frag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_FRAG_TM_MESSAGE, lcp_rndv_tag_offload_frag_handler, 0);
