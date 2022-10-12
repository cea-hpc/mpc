#include "lcp_proto.h"

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

static size_t lcp_send_am_pack(void *dest, void *data) {
	lcp_am_hdr_t *hdr  = dest;
	lcp_request_t *req = data;

	hdr->base.comm_id = req->send.am.comm_id;
	hdr->am_id        = req->send.am.am_id;

	memcpy((void *)(hdr + 1), req->send.buffer, req->send.length);

	return sizeof(*hdr) + req->send.length;
}

static size_t lcp_send_ack_pack(void *dest, void *data) {
	lcp_rndv_ack_hdr_t *hdr = dest;
	lcp_request_t *req      = data;

	hdr->base.base.comm_id = req->recv.tag.comm_id;

	hdr->msg_id       = req->msg_id;
	hdr->src          = req->recv.tag.src;
	hdr->dest         = req->recv.tag.dest;

	return sizeof(*hdr);
}

int lcp_send_do_am_ack(lcp_request_t *req)
{
	int rc;
	lcp_ep_h ep = req->send.ep;
	lcp_chnl_idx_t cc = ep->priority_chnl;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[cc];
	rc = lcr_ep->rail->send_am_bcopy(lcr_ep, MPC_LOWCOMM_ACK_MESSAGE,
					 lcp_send_ack_pack, 
					 req, 0);
	if (rc != sizeof(lcp_rndv_ack_hdr_t)) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
	}

	return rc;
}

int lcp_send_do_am_frag_zcopy(lcp_request_t *req, size_t offset, 
			      size_t length)
{
	int rc;
	struct iovec iov[1];
	int iovcnt = 1;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep;

	lcr_ep = ep->lct_eps[ep->current_chnl];
	lcp_frag_hdr_t hdr = {
		.base = {
			.base = {
				.comm_id = req->send.tag.comm_id,
			},
			.am_id = (uint8_t)MPC_LOWCOMM_FRAG_MESSAGE,
		},
		.dest = req->send.tag.dest,
		.msg_id = req->msg_id,
		.offset = offset 
	};

	iov[0].iov_base = req->send.buffer + offset;
	iov[0].iov_len  = length;

	rc = lcr_ep->rail->send_am_zcopy(lcr_ep, MPC_LOWCOMM_FRAG_MESSAGE, &hdr,
					 sizeof(hdr), iov, 
					 iovcnt, 0, &(req->send.t_ctx.comp));
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not am send frag.");
		rc = MPC_LOWCOMM_ERROR;
	}

	return rc;
}

int lcp_send_do_am_zcopy(lcp_ep_h ep, uint8_t am_id, lcp_request_t *req)
{
	int rc;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->send.cc];

	struct iovec iov[1];
	int iovcnt = 1;

	lcp_am_hdr_t hdr = {
		.base = {
			.comm_id = req->send.am.comm_id,
		},
		.am_id = req->send.am.am_id,
	};

	iov[0].iov_base = req->send.buffer;
	iov[0].iov_len  = req->send.length;

	rc = lcr_ep->rail->send_am_zcopy(lcr_ep, am_id, &hdr,
					 sizeof(hdr), iov, iovcnt, 
					 0, &req->state.comp);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not send tag rndv zcopy.");
	}

	return rc;
}

int lcp_send_do_am(lcp_ep_h ep, uint8_t am_id, lcp_request_t *req)
{
	int rc;
        lcr_rail_attr_t attr;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->send.cc];
        lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

	if (req->send.length < attr.cap.am.max_bcopy) { 
		rc = lcr_ep->rail->send_am_bcopy(lcr_ep, am_id,
					    lcp_send_am_pack, 
					    req, 0);
		if (rc != (int)req->send.length)
			mpc_common_debug_error("LCP: error packing bcopy.");
	} else {
		rc = lcp_send_do_am_zcopy(ep, am_id, req);
	}

	return rc;
}
