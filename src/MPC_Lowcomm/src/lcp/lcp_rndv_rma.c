#include "lcp_types.h"

#include "lcp_def.h"
#include "lcp_request.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"

static inline int lcp_send_get_zcopy(lcp_request_t *req,
                                     _mpc_lowcomm_endpoint_t *lcr_ep,
                                     lcp_chnl_idx_t cc,
                                     uint64_t local_addr,
                                     uint64_t remote_addr,
                                     size_t length)
{
        int rc;

        rc = lcp_get_zcopy(lcr_ep, 
                           local_addr,
                           remote_addr,
                           req->send.rndv.lmem->mems[cc],
                           req->send.rndv.rmem->mems[cc],
                           length,
                           &(req->state.comp));
        return rc;
}

static inline int lcp_send_put_zcopy(lcp_request_t *req,
                                     _mpc_lowcomm_endpoint_t *lcr_ep,
                                     lcp_chnl_idx_t cc,
                                     uint64_t local_addr,
                                     uint64_t remote_addr,
                                     size_t length)
{
        int rc;

        rc = lcp_put_zcopy(lcr_ep, 
                           local_addr,
                           remote_addr,
                           req->send.rndv.lmem->mems[cc],
                           req->send.rndv.rmem->mems[cc],
                           length,
                           &(req->state.comp));
        return rc;
}

int lcp_send_get_zcopy_multi(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	size_t frag_length, length;
	_mpc_lowcomm_endpoint_t *lcr_ep;
	lcp_ep_h ep = req->send.ep;
        lcr_rail_attr_t attr;
        uint64_t local_addr = req->send.rndv.lmem->base_addr;
        uint64_t remote_addr = req->send.rndv.rmem->base_addr;

	/* get frag state */
        size_t offset    = req->state.offset;
	size_t remaining = req->state.remaining;
	int f_id         = req->state.f_id;
	ep->current_chnl = req->state.cur;

	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_get_zcopy;
		length = remaining < frag_length ? remaining : frag_length;

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, remaining=%llu, "
				 "len=%d", f_id, req->send.tag.src_tsk, 
				 req->send.tag.dest_tsk, req->msg_id, remaining, length,
				 remaining);

                rc = lcp_send_get_zcopy(req, 
                                        lcr_ep, 
                                        ep->current_chnl, 
                                        remote_addr + offset, 
                                        local_addr + offset, 
                                        length);
		if (rc == MPC_LOWCOMM_NO_RESOURCE) {
			/* Save progress */
                        req->state.offset     = offset;
			req->state.remaining -= length;
			req->state.cur        = ep->current_chnl;
			req->state.f_id       = f_id;
			mpc_common_debug_info("LCP: fragmentation stalled, frag=%d, req=%p, msg_id=%llu, "
					"remaining=%d, ep=%d", f_id, req, req->msg_id, 
					remaining, ep->current_chnl);
			return rc;
		} else if (rc == MPC_LOWCOMM_ERROR) {
			mpc_common_debug_error("LCP: could not send fragment %d, req=%p, "
					"msg_id=%llu.", f_id, req, req->msg_id);
			break;
		}	       

                offset    += length;
		remaining -= length;

		ep->current_chnl = (ep->current_chnl + 1) % ep->num_chnls;
		f_id++;
	}

        //NOTE: request is completed only after ack is sent
        req->state.status = MPC_LOWCOMM_LCP_DONE;

	return rc;
}

int lcp_send_put_zcopy_multi(lcp_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	size_t frag_length, length;
	_mpc_lowcomm_endpoint_t *lcr_ep;
	lcp_ep_h ep = req->send.ep;
        lcr_rail_attr_t attr;
        uint64_t local_addr = req->send.rndv.lmem->base_addr;
        uint64_t remote_addr = req->send.rndv.rmem->base_addr;

	/* get frag state */
        size_t offset    = req->state.offset;
	size_t remaining = req->state.remaining;
	int f_id         = req->state.f_id;
	ep->current_chnl = req->state.cur;

	while (remaining > 0) {
		lcr_ep = ep->lct_eps[ep->current_chnl];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_put_zcopy;
		length = remaining < frag_length ? remaining : frag_length;

		mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, remaining=%llu, "
				 "len=%d, remote_addr=%p, local_addr=%p", f_id, req->send.tag.src_tsk, 
				 req->send.tag.dest_tsk, req->msg_id, remaining,
				 length, remote_addr, local_addr);

                rc = lcp_send_put_zcopy(req, 
                                        lcr_ep, 
                                        ep->current_chnl, 
                                        local_addr + offset, 
                                        remote_addr + offset, 
                                        length);
		if (rc == MPC_LOWCOMM_NO_RESOURCE) {
			/* Save progress */
                        req->state.offset     = offset;
			req->state.remaining -= length;
			req->state.cur        = ep->current_chnl;
			req->state.f_id       = f_id;
			mpc_common_debug_info("LCP: fragmentation stalled, frag=%d, req=%p, msg_id=%llu, "
					"remaining=%d, ep=%d", f_id, req, req->msg_id, 
					remaining, ep->current_chnl);
			return rc;
		} else if (rc == MPC_LOWCOMM_ERROR) {
			mpc_common_debug_error("LCP: could not send fragment %d, req=%p, "
					"msg_id=%llu.", f_id, req, req->msg_id);
			break;
		}	       

                offset    += length;
		remaining -= length;

		ep->current_chnl = (ep->current_chnl + 1) % ep->num_chnls;
		f_id++;
	}

        req->state.status = MPC_LOWCOMM_LCP_DONE;

	return rc;
}
