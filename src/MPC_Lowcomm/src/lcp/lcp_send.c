#include "lcp_ep.h"
#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_pending.h"
#include "lcp_header.h"
#include "lcp_rndv.h"
#include "lcp_prototypes.h"

#include "opa_primitives.h"
#include "sctk_alloc.h"

//FIXME: static inline ?
int lcp_send_start(lcp_ep_h ep, lcp_request_t *req,
                   const lcp_request_param_t *param)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        size_t size;

        if (ep->ep_config.offload && (ep->ctx->config.offload ||
            (param->flags & LCP_REQUEST_TRY_OFFLOAD))) {
                req->state.offloaded = 1;
                if (req->send.length < ep->ep_config.tag.max_bcopy) {
                        lcp_request_init_tag_send(req);
                        req->send.func = lcp_send_tag_eager_tag_bcopy;
                } else if (req->send.length <= ep->ep_config.tag.max_zcopy) {
                        lcp_request_init_tag_send(req);
                        req->send.func = lcp_send_tag_eager_tag_zcopy;
                } else {
                        lcp_request_init_rndv_send(req);
                        rc = lcp_send_rndv_offload_start(req);
                }
        } else {
                req->state.cc = (ep->rr_cc = ep->rr_cc + 1) % ep->num_chnls;
                req->state.offloaded = 0;
                size = req->send.length + sizeof(lcp_tag_hdr_t);
                if (size <= ep->ep_config.am.max_bcopy) {
                        lcp_request_init_tag_send(req);
                        req->send.func = lcp_send_am_eager_tag_bcopy;
                } else if (size <= ep->ep_config.am.max_zcopy) {
                        lcp_request_init_tag_send(req);
                        req->send.func = lcp_send_am_eager_tag_zcopy;
                } else {
                        lcp_request_init_rndv_send(req);
                        rc = lcp_send_rndv_am_start(req);
                }
        }

        return rc;
}

int lcp_tag_send_nb(lcp_ep_h ep, const void *buffer, size_t count,
                    mpc_lowcomm_request_t *request,
                    const lcp_request_param_t *param)
{
        int rc;
        lcp_request_t *req;

        uint64_t msg_id = lcp_rand_uint64();

        rc = lcp_request_create(&req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not create request.");
                return MPC_LOWCOMM_ERROR;
        }
        req->flags |= LCP_REQUEST_MPI_COMPLETE;
        LCP_REQUEST_INIT_SEND(req, ep->ctx, request, count, 
                              ep, (void *)buffer, OPA_fetch_and_incr_int(&ep->seqn), 
                              msg_id);

        if (ep->state == LCP_EP_FLAG_CONNECTING) {
                if (lcp_pending_create(ep->ctx->pend, req, 
                                       req->msg_id) == NULL) {
                        rc = MPC_LOWCOMM_ERROR;
                }
                mpc_common_debug("LCP: pending req dest=%d, msg_id=%llu", 
                                 req->send.tag.dest, msg_id);
                return rc;
        }

        rc = lcp_send_start(ep, req, param);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not prepare send request.");
                return MPC_LOWCOMM_ERROR;
        }

        rc = lcp_request_send(req);

        return rc;
}
