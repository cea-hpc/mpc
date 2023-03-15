#include "alloca.h"

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
	
	hdr->comm = req->send.tag.comm_id;
	hdr->src  = req->send.tag.src;
	hdr->tag  = req->send.tag.tag;
	hdr->seqn = req->seqn; 

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

        lcp_request_complete(req);

	return rc;
}

int lcp_send_am_eager_tag_zcopy(lcp_request_t *req) 
{
	
	int rc;
        size_t iovcnt     = 0;
	lcp_ep_h ep       = req->send.ep;
        size_t max_iovec  = ep->ep_config.am.max_iovecs;
	struct iovec *iov = alloca(max_iovec*sizeof(struct iovec));
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->state.cc];
	
        //FIXME: hdr never freed, how should it be initialized?
	lcp_tag_hdr_t *hdr = sctk_malloc(sizeof(lcp_tag_hdr_t));
        hdr->comm = req->send.tag.comm_id;
        hdr->src  = req->send.tag.src;
        hdr->tag  = req->send.tag.tag;
        hdr->seqn = req->seqn;

	req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_complete
        };

	iov[0].iov_base = req->send.buffer;
	iov[0].iov_len  = req->send.length;
        iovcnt++;

	mpc_common_debug_info("LCP: send am eager tag zcopy comm=%d, src=%d, "
                              "dest=%d, tag=%d, length=%d", req->send.tag.comm_id, 
                              req->send.tag.src, req->send.tag.dest, req->send.tag.tag, 
                              req->send.length);
        rc = lcp_send_do_am_zcopy(lcr_ep, 
                                  MPC_LOWCOMM_P2P_MESSAGE, 
                                  hdr, 
                                  sizeof(lcp_tag_hdr_t),
                                  iov,
                                  iovcnt, 
                                  &(req->state.comp));

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

	req = (lcp_request_t *)lcp_match_prq(ctx->prq_table, 
					     hdr->comm, 
					     hdr->tag,
					     hdr->src);
	if (req == NULL) {
                mpc_common_debug("LCP: recv unexp tag src=%d, length=%d",
                                 hdr->src, length);
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
						 LCP_RECV_CONTAINER_UNEXP_TAG);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			goto err;
		}
		lcp_append_umq(ctx->umq_table, (void *)ctnr, 
			       hdr->comm,
			       hdr->tag,
			       hdr->src);

		LCP_CONTEXT_UNLOCK(ctx);
		return MPC_LOWCOMM_SUCCESS;
	}
	LCP_CONTEXT_UNLOCK(ctx);
		
        mpc_common_debug("LCP: recv exp tag src=%d, length=%d",
                         hdr->src, length);
	/* copy data to receiver buffer and complete request */
	memcpy(req->recv.buffer, (void *)(hdr + 1), length - sizeof(*hdr));

        /* set recv info for caller */
        req->info->length = length - sizeof(*hdr);
        req->info->src    = hdr->src;
        req->info->tag    = hdr->tag;

	lcp_request_complete(req);
err:
	return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_P2P_MESSAGE, lcp_am_tag_handler, 0);
