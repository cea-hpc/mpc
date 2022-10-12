#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

void lcp_tag_complete(lcr_completion_t *comp) {

	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      state.comp);

	lcp_request_complete(req);
}

__UNUSED__ static size_t lcp_send_tag_pack(void *dest, void *data)
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

//FIXME: change return type to ssize_t. Should this be removed ?
int lcp_send_do_tag_bcopy(lcp_ep_h ep, uint8_t am_id, lcr_pack_callback_t pack,
			  lcp_request_t *req)
{
	ssize_t packed_size;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->send.cc];

	packed_size = lcr_ep->rail->send_am_bcopy(lcr_ep, am_id, pack, req, 0);

	if (packed_size >= 0) {
		return MPC_LOWCOMM_SUCCESS;
	}

	return packed_size;
}

int lcp_tag_matched(lcp_request_t *req, lcp_tag_hdr_t *hdr, size_t length)
{
	memcpy(req->recv.buffer, (void *)(hdr + 1), length - sizeof(*hdr));

	lcp_request_complete(req);
	return MPC_LOWCOMM_SUCCESS;
}

int lcp_recv_tag_handler(void *arg, void *data,
			  size_t length, 
			  __UNUSED__ unsigned flags)
{
	int rc;
	lcp_context_h ctx = arg;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;
	lcp_tag_hdr_t *hdr = data;

	LCP_CONTEXT_LOCK(ctx);
	mpc_common_debug_info("LCP: recv tag header src=%d, dest=%d",
			      hdr->src_tsk, hdr->dest_tsk);

	req = (lcp_request_t *)lcp_match_prq(ctx->prq_table, 
					     hdr->base.comm_id, 
					     hdr->tag,
					     hdr->src);
	if (req == NULL) {
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
						 LCP_RECV_CONTAINER_UNEXP_TAG);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			goto err;
		}
		lcp_append_umq(ctx->umq_table, (void *)ctnr, 
			       hdr->base.comm_id,
			       hdr->tag,
			       hdr->src);

		LCP_CONTEXT_UNLOCK(ctx);
		return MPC_LOWCOMM_SUCCESS;
	}
	LCP_CONTEXT_UNLOCK(ctx);
		
	rc = lcp_tag_matched(req, hdr, length);
err:
	return rc;
}


static int lcp_send_do_tag_zcopy(lcp_ep_h ep, uint8_t am_id, lcp_request_t *req)
{
	int rc;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->send.cc];
	struct iovec *iov = sctk_malloc(sizeof(struct iovec));
	int iovcnt = 1;

	lcp_tag_hdr_t hdr = {
		.base = {
			.comm_id = req->send.tag.comm_id, 
		},
		.dest = req->send.tag.dest,
		.dest_tsk = req->send.tag.dest_tsk,
		.src = req->send.tag.src,
		.src_tsk = req->send.tag.src_tsk,
		.tag = req->send.tag.tag,
		.seqn = req->msg_number
	};

	
	lcr_completion_t cp = {
		.comp_cb = lcp_tag_complete,
	};
	req->state.comp = cp;
	
	iov[0].iov_base = req->send.buffer;
	iov[0].iov_len  = req->send.length;

	rc = lcr_ep->rail->send_am_zcopy(lcr_ep, am_id, &hdr,
					 sizeof(hdr), iov,
					 iovcnt, 0, &req->state.comp);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not send tag zcopy.");
	}

	return rc;
}

int lcp_send_do_tag(lcp_ep_h ep, uint8_t am_id, lcp_request_t *req)
{
	int rc;
	//_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->send.cc];

	mpc_common_debug_info("LCP: send tag req=%p, comm_id=%lu, dest=%d.",
			      req, req->send.tag.comm_id, req->send.tag.dest);
	//size_t bcopy_thresh = lcr_ep->rail->config.bcopy_thresh;

        //FIXME: only do zcopy to avoid intermediary copy of bcopy.
	//if (req->send.length < bcopy_thresh) { 
	//	rc = lcp_send_do_tag_bcopy(ep, am_id, lcp_send_tag_pack, req);
	//} else {
        rc = lcp_send_do_tag_zcopy(ep, am_id, req);
	//}

	return rc;
}

LCP_DEFINE_AM(0, lcp_recv_tag_handler, 0);
