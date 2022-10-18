#include "lcp_tag_offload.h"

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

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

ssize_t lcp_send_do_tag_offload_bcopy(lcp_request_t *req, uint64_t imm, 
				      lcr_pack_callback_t pack)
{
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->send.cc];

	lcr_tag_t tag = {.t_tag = {
		.tag = req->send.tag.tag,
		.src = req->send.tag.src,
		.seqn = req->msg_number }
	};

	req->send.t_ctx.arg = ep->ctx;
	req->send.t_ctx.req = req;
	req->send.t_ctx.comm_id = req->send.tag.comm_id;

	mpc_common_debug_info("LCP: post send tag bcopy req=%p, src=%d, dest=%d, size=%d, matching=[%d:%d:%d]", 
			      req, req->send.tag.src, req->send.tag.dest, req->send.length, tag.t_tag.tag, 
			      tag.t_tag.src, tag.t_tag.seqn);
	payload = lcr_ep->rail->send_tag_bcopy(lcr_ep, tag, imm, pack, 
					       req, 0, &(req->send.t_ctx));

	if (payload >= 0) {
		return MPC_LOWCOMM_SUCCESS;
	}

	return payload;
}

int lcp_send_do_ack_offload_post(lcp_request_t *req, 
				 lcr_pack_callback_t pack)
{
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->send.cc];

	lcr_tag_t tag = {.t_frag = {
		.f_id = 0,
		.m_id = req->msg_id }
	};

	lcp_tm_imm_t imm = { .hdr = {
		.prot = MPC_LOWCOMM_ACK_TM_MESSAGE,
		.seqn = req->msg_number }
	};

	req->send.t_ctx.arg = ep->ctx;
	req->send.t_ctx.req = req;
	req->send.t_ctx.comm_id = req->send.tag.comm_id;

	mpc_common_debug_info("LCP: post send ack bcopy req=%p, src=%d, dest=%d, size=%d, matching=[%d:%d]", 
			      req, req->send.tag.src, req->send.tag.dest, req->send.length, tag.t_frag.f_id, 
			      tag.t_frag.m_id);
	payload = lcr_ep->rail->send_tag_bcopy(lcr_ep, tag, imm.raw, pack,
					       req, 0, &(req->send.t_ctx));
	if (payload >= 0) {
		return MPC_LOWCOMM_SUCCESS;
	}

	return payload;
}

int lcp_send_do_frag_offload_post(lcp_request_t *req, size_t offset, 
				  unsigned frag_id, size_t length)
{
	int rc;
	lcp_ep_h ep = req->send.ep;
	lcp_chnl_idx_t cc = ep->current_chnl;
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

	req->send.t_ctx.arg = ep->ctx;
	req->send.t_ctx.req = req;
	req->send.t_ctx.comm_id = req->send.tag.comm_id;

	mpc_common_debug_info("LCP: post send frag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%llu, %d]", 
			      req, req->send.tag.src, req->send.tag.dest, length, tag.t_frag.f_id, 
			      tag.t_frag.m_id);
	rc = lcr_ep->rail->send_tag_zcopy(lcr_ep, tag, imm.raw, &iov, 1, 0, 
					  &(req->send.t_ctx));

	return rc;
}

int lcp_send_do_tag_offload_post(lcp_ep_h ep, lcp_request_t *req)
{
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->send.cc];
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
		.prot = MPC_LOWCOMM_P2P_MESSAGE,
		.seqn = req->msg_number }
	};

	req->send.t_ctx.arg = ep->ctx;
	req->send.t_ctx.req = req;
	req->send.t_ctx.comm_id = req->send.tag.comm_id;
	req->send.t_ctx.comp = cp;

	req->flags |= LCP_REQUEST_OFFLOADED;

	mpc_common_debug_info("LCP: post send tag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%d:%d:%d]", 
			      req, req->send.tag.src, req->send.tag.dest, req->send.length, tag.t_tag.tag, 
			      tag.t_tag.src, tag.t_tag.seqn);
	return lcr_ep->rail->send_tag_zcopy(lcr_ep, tag, imm.raw, &iov, 1, 
					  0, &(req->send.t_ctx));
}

int lcp_recv_frag_offload_post(lcp_request_t *req, sctk_rail_info_t *iface,
			       size_t offset, unsigned frag_id, size_t length)
{
	int rc;
	lcr_tag_t tag = {.t_frag = {
		.f_id = frag_id,
		.m_id = req->msg_id }
	};
	lcr_tag_t ign_tag = {.t = 0};

	struct iovec iov = {
		.iov_base = req->recv.buffer + offset,
		.iov_len  = length 
	};

	req->recv.t_ctx.arg = req->recv.ctx;
	req->recv.t_ctx.req = req;
	req->recv.t_ctx.comm_id = req->recv.tag.comm_id;
	req->recv.t_ctx.tag = tag;

	mpc_common_debug_info("LCP: post recv frag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%llu, %d]", 
			      req, req->recv.tag.src, req->recv.tag.dest, length, tag.t_frag.f_id, 
			      tag.t_frag.m_id);
	rc = iface->recv_tag_zcopy(iface, tag, ign_tag, &iov, 1, 
				   &(req->recv.t_ctx));

	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not send tag zcopy.");
	}

	return rc;
}

int lcp_recv_tag_offload_post(lcp_request_t *req, sctk_rail_info_t *iface)
{
	lcr_tag_t tag = { .t_tag = { 
		.tag = req->recv.tag.tag,
		.src = req->recv.tag.src,
		.seqn = req->msg_number }
	};
	lcr_tag_t ign_tag = {.t = LCP_TM_SEQN_MASK };
	ign_tag.t |= (int)req->recv.tag.src == MPC_ANY_SOURCE ? LCP_TM_SRC_MASK : 0;
	ign_tag.t |= req->recv.tag.tag == MPC_ANY_TAG ? LCP_TM_TAG_MASK : 0;

	struct iovec iov = {
		.iov_base = req->recv.buffer,
		.iov_len  = req->recv.length
	};

	req->recv.t_ctx.arg = req->recv.ctx;
	req->recv.t_ctx.req = req;
	req->recv.t_ctx.comm_id = req->recv.tag.comm_id;

	mpc_common_debug_info("LCP: post recv tag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%d:%d:%d], "
			      "ignore=[%d:%d:%d]", req, req->send.tag.src, req->send.tag.dest, req->send.length, 
			      tag.t_tag.tag, tag.t_tag.src, tag.t_tag.seqn, ign_tag.t_tag.tag, ign_tag.t_tag.src,
			      ign_tag.t_tag.seqn);
	return iface->recv_tag_zcopy(iface, tag, ign_tag, &iov, 1, 
				     &(req->recv.t_ctx));
}

int lcp_recv_ack_offload_post(lcp_request_t *req, sctk_rail_info_t *iface)
{
	lcr_tag_t tag = { .t_frag = {
		.f_id = 0,
		.m_id = req->msg_id }
	};
	lcr_tag_t ign_tag = { .t = 0 };

	struct iovec iov = { 
                .iov_base = NULL,
                .iov_len  = 0
	};

	req->tm.t_ctx.arg = req->recv.ctx;
	req->tm.t_ctx.req = req;
	req->tm.t_ctx.comm_id = req->tm.comm_id;

	mpc_common_debug_info("LCP: post recv ack zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%d:%d]", 
			      req, req->send.tag.src, req->send.tag.dest, req->send.length, tag.t_frag.f_id, 
			      tag.t_frag.m_id);
	return iface->recv_tag_zcopy(iface, tag, ign_tag, &iov, 1, 
				     &(req->tm.t_ctx));
}

static int lcp_tag_offload_handler(void *arg, void *data, size_t length,
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

LCP_DEFINE_AM(MPC_LOWCOMM_P2P_MESSAGE, lcp_tag_offload_handler, 0);
