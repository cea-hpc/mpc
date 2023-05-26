#include "lcp.h"
#include "lcp_ep.h"
#include "lcp_context.h"
#include "lcp_request.h"
#include "lcp_pending.h"
//FIXME: remove header here since header should only be used in lcp_tag.c
#include "lcp_header.h"
#include "lcp_tag.h"
#include "lcp_tag_offload.h"

#include "mpc_common_debug.h"
#include "mpc_lowcomm_msg.h"
#include "mpc_lowcomm_types.h"
#include "opa_primitives.h"

//FIXME: static inline ?
/**
 * @brief Switch between protocols. Available protocols are : 
 * - buffered copy
 * - zero copy
 * - rendez-vous
 * 
 * @param ep endpoint to send the message
 * @param req request used to send the message
 * @param param request parameter used for offload flag
 * @return int MPI_SUCCESS in case of success
 */
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
                        req->send.func = lcp_send_tag_eager_tag_bcopy;
                } else if ((size <= ep->ep_config.tag.max_zcopy) &&
                           (param->datatype & LCP_DATATYPE_CONTIGUOUS)) {
                        req->send.func = lcp_send_tag_eager_tag_zcopy;
                } else {
                        req->request->synchronized = 0;
                        rc = lcp_send_rndv_offload_start(req);
                }
        } else {
                //NOTE: multiplexing might not always be efficient (IO NUMA
                //      effects). A specific scheduling policy should be 
                //      implemented to decide
                req->state.offloaded = 0;
                //FIXME: size set in the middle of nowhere...
                size = req->send.length + sizeof(lcp_tag_hdr_t);
                if (size <= ep->ep_config.am.max_bcopy || 
                    ((req->send.length <= ep->ep_config.tag.max_zcopy) &&
                     param->datatype & LCP_DATATYPE_DERIVED)) {
                        req->send.func = lcp_send_eager_tag_bcopy;
                } else if ((size <= ep->ep_config.am.max_zcopy) &&
                           (param->datatype & LCP_DATATYPE_CONTIGUOUS)) {
                        req->send.func = lcp_send_eager_tag_zcopy;
                } else {
                        req->request->synchronized = 0;
                        rc = lcp_send_rndv_tag_start(req);
                }
        }

        return rc;
}

//FIXME: It is not clear wether count is the number of element or the length in
//       bites. For now, the actual length in bites is given taking into account
//       the datatypes and stuff...
/**
 * @brief Send a message.
 * 
 * @param ep endpoint to send the message
 * @param task task
 * @param buffer message
 * @param count length of the message
 * @param request request to send the message
 * @param param request parameter used for offload flag
 * @return int MPI_SUCCESS in case of success
 */
int lcp_tag_send_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer, 
                    size_t count, mpc_lowcomm_request_t *request,
                    const lcp_request_param_t *param)
{
        int rc;
        lcp_request_t *req;

        // create the request to send
        rc = lcp_request_create(&req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not create request.");
                return MPC_LOWCOMM_ERROR;
        }
        req->flags |= LCP_REQUEST_MPI_COMPLETE;
        OPA_load_int(&ep->seqn);
        uint64_t msg_id = OPA_fetch_and_incr_int(&ep->seqn);

        // initialize request
        //FIXME: sequence number should be task specific. Following works in
        //       process-based but not in thread-based.
        //       Reorder is to be reimplemented.
        LCP_REQUEST_INIT_TAG_SEND(req, ep->ctx, task, request, param->recv_info, 
                                  count, ep, (void *)buffer, 
                                  OPA_fetch_and_incr_int(&ep->seqn), msg_id,
                                  param->datatype);

        // if the endpoint is not yet connected
        if (ep->state == LCP_EP_FLAG_CONNECTING) {
                // set the request to pending
                if (lcp_pending_create(ep->ctx->pend, req, 
                                       req->msg_id) == NULL) {
                        rc = MPC_LOWCOMM_ERROR;
                }
                mpc_common_debug("LCP: pending req dest=%d, msg_id=%llu", 
                                 req->send.tag.dest_tid, msg_id);
                return rc;
        }


        // prepare request depending on its type
        rc = lcp_send_start(ep, req, param);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not prepare send request.");
                return MPC_LOWCOMM_ERROR;
        }

        if(request->synchronized){
                req->message_type = MPC_LOWCOMM_P2P_SYNC_MESSAGE;
                lcp_pending_create(ep->ctx->pend, req, req->msg_id);
        }
        // send the request
        rc = lcp_request_send(req);
        return rc;
}
