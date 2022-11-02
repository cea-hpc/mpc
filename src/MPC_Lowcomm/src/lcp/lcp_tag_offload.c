#include "lcp_prototypes.h"

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

/* ============================================== */
/* Packing                                        */
/* ============================================== */

static size_t lcp_send_tag_tag_pack(void *dest, void *data)
{
	lcp_tag_hdr_t *hdr = dest;
	lcp_request_t *req = data;
	
	hdr->base.comm_id = req->send.tag.comm_id;
	hdr->dest         = req->send.tag.dest;
	hdr->dest_tsk     = req->send.tag.dest_tsk;
	hdr->src          = req->send.tag.src;
	hdr->src_tsk      = req->send.tag.src_tsk;
	hdr->tag          = req->send.tag.tag;
	hdr->seqn         = req->msg_number; 

	memcpy((void *)(hdr + 1), req->send.buffer, req->send.length);

	return sizeof(*hdr) + req->send.length;
}

/* ============================================== */
/* Completion                                     */
/* ============================================== */

void lcp_request_complete_tag_offload(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      send.t_ctx.comp);

        if (req->send.length > comp->sent) {
                //FIXME: set truncated request ?
                mpc_common_debug_error("LCP: send truncated for msg_id=%llu, "
                                      "expected length=%d, received=%d", req->msg_id,
                                     req->send.length, comp->sent);
        } 
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

/* ============================================== */
/* Send                                           */
/* ============================================== */

static inline int lcp_send_tag_eager_frag_zcopy(lcp_request_t *req, 
		lcp_chnl_idx_t cc,
		size_t offset, 
		unsigned frag_id, 
		size_t length)
{
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[cc];

	lcr_tag_t tag = {.t_frag = {
		.f_id = frag_id,
		.m_id = req->msg_id }
	};

	struct iovec iov = {
		.iov_base = req->send.buffer + offset,
		.iov_len  = length 
	};

	lcp_tm_imm_t imm = { .hdr = {
		.prot = MPC_LOWCOMM_FRAG_TM_MESSAGE,
		.seqn = req->msg_number }
	};

	lcr_completion_t cp = {
		.comp_cb = lcp_request_complete_frag,
	};

	req->send.t_ctx.comp = cp;
	req->send.t_ctx.arg = ep->ctx;
	req->send.t_ctx.req = req;
	req->send.t_ctx.comm_id = req->send.tag.comm_id;

	mpc_common_debug_info("LCP: post send frag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%llu, %d]", 
			      req, req->send.tag.src, req->send.tag.dest, length, tag.t_frag.f_id, 
			      tag.t_frag.m_id);
	return lcp_send_do_tag_zcopy(lcr_ep, tag, imm.raw, &iov, 1, &(req->send.t_ctx));
}

int lcp_send_tag_zcopy_multi(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	size_t frag_length, length;
	_mpc_lowcomm_endpoint_t *lcr_ep;
	lcp_ep_h ep = req->send.ep;
        lcr_rail_attr_t attr;

	/* get frag state */
	size_t offset    = req->state.offset;
	size_t remaining = req->send.length - offset;
	int f_id         = req->state.f_id;
	ep->current_chnl = req->state.cur;

	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_send_zcopy;

		length = remaining < frag_length ? remaining : frag_length;

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, offset=%d, "
				 "len=%d, remaining=%d", f_id, req->send.tag.src_tsk, 
				 req->send.tag.dest_tsk, req->msg_id, offset, length,
				 remaining);

		rc = lcp_send_tag_eager_frag_zcopy(req, ep->current_chnl, offset, f_id+1, length);
		if (rc == MPC_LOWCOMM_NO_RESOURCE) {
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

int lcp_send_tag_eager_tag_bcopy(lcp_request_t *req)
{
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	lcr_tag_t tag = {.t_tag = {
		.tag = req->send.tag.tag,
		.src = req->send.tag.src,
		.seqn = req->msg_number }
	};

	lcp_tm_imm_t imm = { .hdr = {
		.prot = MPC_LOWCOMM_P2P_TM_MESSAGE,
		.seqn = req->msg_number }
	};

	req->send.t_ctx.arg = ep->ctx;
	req->send.t_ctx.req = req;
	req->send.t_ctx.comm_id = req->send.tag.comm_id;

	payload = lcp_send_do_tag_bcopy(lcr_ep, 
			tag, 
			imm.raw, 
			lcp_send_tag_tag_pack, 
			req, 
			&(req->send.t_ctx));

	return payload;
}

int lcp_send_tag_eager_tag_zcopy(lcp_request_t *req)
{
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

	lcr_completion_t cp = {
		.comp_cb = lcp_request_complete_tag_offload,
	};

	lcr_tag_t tag = { .t_tag = {
		.src = req->send.tag.src,
		.tag = req->send.tag.tag,
		.seqn = req->msg_number }
	};

	struct iovec iov = {
		.iov_base = req->send.buffer,
		.iov_len  = req->send.length
	};

	lcp_tm_imm_t imm = { .hdr = {
		.prot = MPC_LOWCOMM_P2P_TM_MESSAGE,
		.seqn = req->msg_number }
	};

	req->send.t_ctx.arg = req->send.ep->ctx;
	req->send.t_ctx.req = req;
	req->send.t_ctx.comm_id = req->send.tag.comm_id;
	req->send.t_ctx.comp = cp;

	req->flags |= LCP_REQUEST_OFFLOADED;

	mpc_common_debug_info("LCP: post send tag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%d:%d:%d]", 
			      req, req->send.tag.src, req->send.tag.dest, req->send.length, tag.t_tag.tag, 
			      tag.t_tag.src, tag.t_tag.seqn);
	return lcp_send_do_tag_zcopy(lcr_ep, tag, imm.raw, &iov, 1, &(req->send.t_ctx));

}


/* ============================================== */
/* Recv                                           */
/* ============================================== */

int lcp_recv_tag_zcopy(lcp_request_t *rreq, sctk_rail_info_t *iface)
{
	lcr_tag_t tag = { .t_tag = { 
		.tag = rreq->recv.tag.tag,
		.src = rreq->recv.tag.src,
		.seqn = rreq->msg_number }
	};
	lcr_tag_t ign_tag = {.t = LCP_TM_SEQN_MASK };
	ign_tag.t |= (int)rreq->recv.tag.src == MPC_ANY_SOURCE ? LCP_TM_SRC_MASK : 0;
	ign_tag.t |= rreq->recv.tag.tag == MPC_ANY_TAG ? LCP_TM_TAG_MASK : 0;

	struct iovec iov = {
		.iov_base = rreq->recv.buffer,
		.iov_len  = rreq->recv.length
	};

	rreq->recv.t_ctx.arg = rreq->ctx; //FIXME: ctx also in rreq so not needed
	rreq->recv.t_ctx.req = rreq;
	rreq->recv.t_ctx.comm_id = rreq->recv.tag.comm_id;

	mpc_common_debug_info("LCP: post recv tag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%d:%d:%d], "
			      "ignore=[%d:%d:%d]", rreq, rreq->send.tag.src, rreq->send.tag.dest, rreq->send.length, 
			      tag.t_tag.tag, tag.t_tag.src, tag.t_tag.seqn, ign_tag.t_tag.tag, ign_tag.t_tag.src,
			      ign_tag.t_tag.seqn);
	return lcp_recv_do_tag_zcopy(iface,
			tag,
			ign_tag,
			&iov,
			1,
			&(rreq->recv.t_ctx));

}

static inline int lcp_recv_tag_frag_zcopy(lcp_request_t *rreq, 
		sctk_rail_info_t *iface,
		size_t offset,
		int frag_id,
		size_t length)
{
	lcr_tag_t tag = {.t_frag = {
		.f_id = frag_id,
		.m_id = rreq->msg_id }
	};
	lcr_tag_t ign_tag = {.t = 0};

	struct iovec iov = {
		.iov_base = rreq->recv.buffer + offset,
		.iov_len  = length 
	};

	rreq->recv.t_ctx.arg = rreq->ctx;
	rreq->recv.t_ctx.req = rreq;
	rreq->recv.t_ctx.comm_id = rreq->recv.tag.comm_id;
	rreq->recv.t_ctx.tag = tag;

	mpc_common_debug_info("LCP: post recv frag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%llu, %d]", 
			rreq, rreq->recv.tag.src, rreq->recv.tag.dest, length, tag.t_frag.f_id, 
			tag.t_frag.m_id);
	return lcp_recv_do_tag_zcopy(iface,
			tag,
			ign_tag,
			&iov,
			1,
			&(rreq->recv.t_ctx));
}

int lcp_recv_tag_zcopy_multi(lcp_ep_h ep, lcp_request_t *rreq)
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

                frag_length = attr.iface.cap.rndv.max_send_zcopy;

		length = remaining < frag_length ? remaining : frag_length;

		rc = lcp_recv_tag_frag_zcopy(rreq, lcr_ep->rail, 
				offset, ifrag+1, length);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			goto err;
		}

		mpc_common_debug_info("LCP: recv frag n=%d, src=%d, dest=%d, msg_id=%llu, offset=%d, "
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

/* ============================================== */
/* Handlers                                       */
/* ============================================== */

static int lcp_tag_handler(void *arg, void *data, size_t length,
				   unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcr_tag_context_t *t_ctx = arg;
	lcp_request_t *req  = t_ctx->req;

        if (req->recv.length > length)
                req->flags |= LCP_REQUEST_RECV_TRUNC;

	if (flags & LCR_IFACE_TM_OVERFLOW) {
		memcpy(req->recv.buffer, data, length);
	}
	lcp_request_complete(req);

	return rc;
}

//NOTE: data pointer unused since matching of fragments are always in priority.
static int lcp_tag_frag_handler(void *arg, 
		__UNUSED__ void *data, 
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

LCP_DEFINE_AM(MPC_LOWCOMM_P2P_TM_MESSAGE, lcp_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_FRAG_TM_MESSAGE, lcp_tag_frag_handler, 0);
