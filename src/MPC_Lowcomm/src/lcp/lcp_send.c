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
                size = req->send.length;
                req->state.offloaded = 1;
                if (size <= ep->ep_config.tag.max_bcopy) {
                        lcp_request_init_tag_send(req);
                        req->send.func = lcp_send_tag_eager_tag_bcopy;
                } else if ((size <= ep->ep_config.tag.max_zcopy) &&
                           (param->datatype & LCP_DATATYPE_CONTIGUOUS)) {
                        lcp_request_init_tag_send(req);
                        req->send.func = lcp_send_tag_eager_tag_zcopy;
                } else {
                        lcp_request_init_rndv_send(req);
                        rc = lcp_send_rndv_offload_start(req);
                }
        } else {
                //NOTE: multiplexing might now always be efficient (IO NUMA
                //      effects). A specific scheduling policy should be 
                //      implemented to decide
                req->state.cc = (ep->rr_cc = ep->rr_cc + 1) % ep->num_chnls;
                req->state.offloaded = 0;
                //FIXME: size set in the middle of nowhere...
                size = req->send.length + sizeof(lcp_tag_hdr_t);
                if (size <= ep->ep_config.am.max_bcopy || 
                    ((req->send.length <= ep->ep_config.tag.max_zcopy) &&
                     param->datatype & LCP_DATATYPE_DERIVED)) {
                        lcp_request_init_tag_send(req);
                        req->send.func = lcp_send_am_eager_tag_bcopy;
                } else if ((size <= ep->ep_config.am.max_zcopy) &&
                           (param->datatype & LCP_DATATYPE_CONTIGUOUS)) {
                        lcp_request_init_tag_send(req);
                        req->send.func = lcp_send_am_eager_tag_zcopy;
                } else {
                        lcp_request_init_rndv_send(req);
                        rc = lcp_send_rndv_am_start(req);
                }
        }

        return rc;
}

//FIXME: It is not clear wether count is the number of element or the length in
//       bites. For now, the actual length in bites is given taking into account
//       the datatypes and stuff...
int lcp_tag_send_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer, 
                    size_t count, mpc_lowcomm_request_t *request,
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
        //FIXME: sequence number should be task specific. Following works in
        //       process-based but not in thread-based.
        //       Reorder is to be reimplemented.
        LCP_REQUEST_INIT_SEND(req, ep->ctx, task, request, param->recv_info, 
                              count, ep, (void *)buffer, 
                              OPA_fetch_and_incr_int(&ep->seqn), msg_id,
                              param->datatype);

        rc = lcp_send_start(ep, req, param);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not prepare send request.");
                return MPC_LOWCOMM_ERROR;
        }

        rc = lcp_request_send(req);

        return rc;
}
