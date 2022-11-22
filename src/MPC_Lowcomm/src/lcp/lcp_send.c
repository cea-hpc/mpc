#include "lcp_ep.h"
#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_pending.h"
#include "lcp_header.h"
#include "lcp_rndv.h"
#include "lcp_prototypes.h"

#include "opa_primitives.h"
#include "sctk_alloc.h"

int lcp_send_start(lcp_ep_h ep, lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->priority_chnl];

        if (req->send.length < ep->ep_config.max_bcopy) {
                lcp_request_init_tag_send(req);
                if (LCR_IFACE_IS_TM(lcr_ep->rail)) {
                        req->send.func = lcp_send_tag_eager_tag_bcopy;
                } else {
                        req->send.func = lcp_send_am_eager_tag_bcopy;
                }
        } else if (req->send.length <= ep->ep_config.max_zcopy) {
                lcp_request_init_tag_send(req);
                if (LCR_IFACE_IS_TM(lcr_ep->rail)) {
                        req->send.func = lcp_send_tag_eager_tag_zcopy;
                } else {
                        req->send.func = lcp_send_am_eager_tag_zcopy;
                }
        } else {
                lcp_request_init_rndv_send(req);
                rc = lcp_send_rndv_start(req);
        }

        return rc;
}

int lcp_send(lcp_ep_h ep, mpc_lowcomm_request_t *request, 
             const void *buffer)
{
        int rc;
        lcp_request_t *req;

        uint64_t msg_id = lcp_rand_uint64();

        rc = lcp_request_create(&req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not create request.");
                return MPC_LOWCOMM_ERROR;
        }
        LCP_REQUEST_INIT_SEND(req, ep->ctx, request, request->header.msg_size, 
                              ep, (void *)buffer, OPA_fetch_and_incr_int(&ep->seqn), 
                              msg_id);

        if (ep->state == LCP_EP_FLAG_CONNECTING) {
                if (lcp_pending_create(ep->ctx, req, 
                                       req->msg_id) == NULL) {
                        rc = MPC_LOWCOMM_ERROR;
                }
                mpc_common_debug("LCP: pending req dest=%d, msg_id=%llu", 
                                 req->send.tag.dest, msg_id);
                return rc;
        }

        rc = lcp_send_start(ep, req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not prepare send request.");
                return MPC_LOWCOMM_ERROR;
        }

        rc = lcp_request_send(req);

        return rc;
}
