#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_prototypes.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

/* ============================================== */
/* Packing                                        */
/* ============================================== */

static size_t lcp_send_tag_pack(void *dest, void *data)
{
	lcp_tag_hdr_t *hdr = dest;
	lcp_request_t *req = data;
	
	hdr->base.comm_id = req->send.tag.comm_id;
	hdr->dest         = req->send.tag.dest;
	hdr->src          = req->send.tag.src;
	hdr->tag          = req->send.tag.tag;
	hdr->seqn         = req->seqn; 

	memcpy((void *)(hdr + 1), req->send.buffer, req->send.length);

	return sizeof(*hdr) + req->send.length;
}

/* ============================================== */
/* Completion                                     */
/* ============================================== */

void lcp_tag_complete(lcr_completion_t *comp) {

	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      state.comp);

	lcp_request_complete(req);
}

void lcp_frag_complete(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
			send.t_ctx.comp);
	//FIXME: is lock needed here?
	req->state.remaining -= comp->sent;
	mpc_common_debug("LCP: req=%p, remaining=%d, frag len=%lu.", 
			req, req->state.remaining, comp->sent);
	if (req->state.remaining == 0) {
		lcp_request_complete(req);
	}
}
	

/* ============================================== */
/* Send                                           */
/* ============================================== */

int lcp_send_am_eager_tag_bcopy(lcp_request_t *req) 
{
	
	int rc = MPC_LOWCOMM_SUCCESS;
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->state.cc];

	mpc_common_debug_info("LCP: send am eager tag bcopy src=%d, dest=%d, length=%d",
			      req->send.tag.src, req->send.tag.dest, req->send.length);
	payload = lcp_send_do_am_bcopy(lcr_ep, 
			MPC_LOWCOMM_P2P_MESSAGE, 
			lcp_send_tag_pack, 
			req);
	//FIXME: handle error
	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
	}

	return rc;
}

int lcp_send_am_eager_tag_zcopy(lcp_request_t *req) 
{
	
	int rc;
	struct iovec iov[1];
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->state.cc];
	
	lcp_tag_hdr_t hdr = {
		.base = {
			.comm_id = req->send.tag.comm_id, 
		},
		.dest = req->send.tag.dest,
		.src = req->send.tag.src,
		.tag = req->send.tag.tag,
		.seqn = req->seqn
	};

	lcr_completion_t cp = {
		.comp_cb = lcp_tag_complete,
	};
	req->state.comp = cp;

	iov[0].iov_base = req->send.buffer;
	iov[0].iov_len  = req->send.length;

	mpc_common_debug_info("LCP: send am eager tag zcopy src=%d, dest=%d, length=%d",
			      req->send.tag.src, req->send.tag.dest, req->send.length);
	rc = lcp_send_do_am_zcopy(lcr_ep, 
			MPC_LOWCOMM_P2P_MESSAGE, 
			&hdr, 
			sizeof(lcp_tag_hdr_t),
			iov,
			1, 
			&(req->state.comp));

	return rc;
}

static inline int lcp_send_am_frag_zcopy(lcp_request_t *req, 
		lcp_chnl_idx_t cc, 
		size_t offset, 
		size_t length)
{
	int rc;
	struct iovec iov[1];
	int iovcnt = 1;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[cc];

	lcr_ep = ep->lct_eps[req->state.cc];
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
	lcr_completion_t cp = {
		.comp_cb = lcp_frag_complete,
	};
	req->send.t_ctx.comp = cp;

	iov[0].iov_base = req->send.buffer + offset;
	iov[0].iov_len  = length;

	rc = lcp_send_do_am_zcopy(lcr_ep, MPC_LOWCOMM_FRAG_MESSAGE, &hdr,
			sizeof(hdr), iov, iovcnt, &(req->state.comp));
	if (rc != MPC_LOWCOMM_SUCCESS) {
		mpc_common_debug_error("LCP: could not am send frag.");
		rc = MPC_LOWCOMM_ERROR;
	}

	return rc;
}

int lcp_send_am_zcopy_multi(lcp_request_t *req)
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

	while (remaining > 0) {
		lcr_ep = ep->lct_eps[req->state.cc];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_put_zcopy;

		length = remaining < frag_length ? remaining : frag_length;

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, offset=%d, "
				 "len=%d, remaining=%d", f_id, req->send.tag.src, 
				 req->send.tag.dest, req->msg_id, offset, length,
				 remaining);

		rc = lcp_send_am_frag_zcopy(req, req->state.cc, offset, length);
		if (rc == MPC_LOWCOMM_NO_RESOURCE) {
			/* Save progress */
			req->state.offset    = offset;
			req->state.f_id      = f_id;
			mpc_common_debug_info("LCP: fragmentation stalled, frag=%d, req=%p, msg_id=%llu, "
					"remaining=%d, offset=%d, ep=%d", f_id, req, req->msg_id, 
					remaining, offset, req->state.cc);
			return rc;
		} else if (rc == MPC_LOWCOMM_ERROR) {
			mpc_common_debug_error("LCP: could not send fragment %d, req=%p, "
					"msg_id=%llu.", f_id, req, req->msg_id);
			break;
		}	       

		offset += length;
		remaining -= length;

                //FIXME: replace by num used interface.
		req->state.cc = (req->state.cc + 1) % ep->num_chnls;
		f_id++;
	}

	return rc;
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */
//NOTE: no receive call for active message
/* ============================================== */
/* Handlers                                       */
/* ============================================== */

static int lcp_am_tag_handler(void *arg, void *data,
			  size_t length, 
			  __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;
	lcp_tag_hdr_t *hdr = data;

	LCP_CONTEXT_LOCK(ctx);
	mpc_common_debug_info("LCP: recv tag header src=%d, dest=%d",
			      hdr->src, hdr->dest);

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
		
	/* copy data to receiver buffer and complete request */
	memcpy(req->recv.buffer, (void *)(hdr + 1), length - sizeof(*hdr));
	lcp_request_complete(req);
err:
	return rc;
}

static int lcp_am_frag_handler(void *arg, void *data, 
			  size_t length,
			  __UNUSED__ unsigned flags)
{
	int rc;
	lcp_context_h ctx = arg;
	lcp_frag_hdr_t *hdr = data;
	lcp_request_t *req;

        req = lcp_pending_get_request(ctx, hdr->msg_id);
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
	req->state.remaining -= length - sizeof(*hdr);
	req->state.offset    += length - sizeof(*hdr);
	LCP_CONTEXT_UNLOCK(ctx);

	memcpy(req->recv.buffer + hdr->offset, hdr + 1, length - sizeof(*hdr));

	if (req->state.remaining == 0) {
		lcp_request_complete(req);
	}

	rc = MPC_LOWCOMM_SUCCESS;
err:
	return rc;
}


LCP_DEFINE_AM(MPC_LOWCOMM_P2P_MESSAGE, lcp_am_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_FRAG_MESSAGE, lcp_am_frag_handler, 0);
