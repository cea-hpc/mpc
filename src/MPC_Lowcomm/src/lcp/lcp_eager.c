#include "lcp_request.h" 
#include "lcp_prototypes.h"

ssize_t lcp_send_eager_bcopy(lcp_request_t *req, 
                         lcr_pack_callback_t pack,
                         uint8_t am_id) 
{
        ssize_t payload;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

        payload = lcp_send_do_am_bcopy(ep->lct_eps[cc],
                                       am_id, pack, req);

	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
                return MPC_LOWCOMM_ERROR;
	}

        return payload;
}

int lcp_send_eager_zcopy(lcp_request_t *req, uint8_t am_id,
                         void *hdr, size_t hdr_size,
                         struct iovec *iov, size_t iovcnt,
                         lcr_completion_t *comp) 
{
	lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

        return lcp_send_do_am_zcopy(ep->lct_eps[cc], 
                                    am_id, hdr, hdr_size,
                                    iov, iovcnt, comp);
}
