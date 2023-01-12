#include "lcp_prototypes.h"

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_rndv.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

/* ============================================== */
/* Packing                                        */
/* ============================================== */

static size_t lcp_send_tag_tag_pack(void *dest, void *data)
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

void lcp_request_complete_tag_offload(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);

        lcp_request_complete(req);
}

void lcp_request_complete_frag(lcr_completion_t *comp)
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

//FIXME: dead code ?
static inline int lcp_send_tag_eager_frag_zcopy(lcp_request_t *req, 
                                                lcp_chnl_idx_t cc,
                                                size_t offset, 
                                                unsigned frag_id, 
                                                size_t length)
{
        lcp_ep_h ep = req->send.ep;
        _mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[cc];

        lcr_tag_t tag = {.t_frag = {
                .f_id = frag_id,
                .m_id = req->msg_id }
        };

        uint64_t imm = 0;
        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_P2P_TM_MESSAGE, 
                            req->send.length, req->seqn);

        struct iovec iov = {
                .iov_base = req->send.buffer + offset,
                .iov_len  = length 
        };

        lcr_completion_t cp = {
                .comp_cb = lcp_request_complete_frag,
        };

        req->send.t_ctx.comp = cp;
        req->send.t_ctx.req = req;

        mpc_common_debug_info("LCP: post send frag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%llu, %d]", 
                              req, req->send.tag.src, req->send.tag.dest, length, tag.t_frag.f_id, 
                              tag.t_frag.m_id);
        return lcp_send_do_tag_zcopy(lcr_ep, tag, imm, &iov, 1, &(req->send.t_ctx));
}

//FIXME: dead code ?
int lcp_send_tag_zcopy_multi(lcp_request_t *req)
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

                frag_length = attr.iface.cap.rndv.max_send_zcopy;

                length = remaining < frag_length ? remaining : frag_length;

                mpc_common_debug("LCP: send frag n=%d, src=%d, dest=%d, msg_id=%llu, offset=%d, "
                                 "len=%d, remaining=%d", f_id, req->send.tag.src, 
                                 req->send.tag.dest, req->msg_id, offset, length,
                                 remaining);

                rc = lcp_send_tag_eager_frag_zcopy(req, req->state.cc, offset, f_id+1, length);
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

                //FIXME: should be the computed number of used interface 
                req->state.cc = (req->state.cc + 1) % ep->num_chnls;
                f_id++;
        }

        return rc;
}

int lcp_send_tag_eager_tag_bcopy(lcp_request_t *req)
{
        ssize_t payload;
        lcp_ep_h ep = req->send.ep;
        _mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->state.cc];

        lcr_tag_t tag = {.t_tag = {
                .tag = req->send.tag.tag,
                .src = req->send.tag.src,
                .comm = req->send.tag.comm_id }
        };

        uint64_t imm = 0;
        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_P2P_TM_MESSAGE, 
                            req->send.length, req->seqn);

        req->send.t_ctx.req = req;

        payload = lcp_send_do_tag_bcopy(lcr_ep, 
                                        tag, 
                                        imm, 
                                        lcp_send_tag_tag_pack, 
                                        req, 
                                        &(req->send.t_ctx));

        return payload;
}

int lcp_send_tag_eager_tag_zcopy(lcp_request_t *req)
{
        lcp_ep_h ep = req->send.ep;
        _mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->state.cc];

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, req->send.tag.src, 
                             req->send.tag.tag, 
                             req->send.tag.comm_id);

        struct iovec iov = {
                .iov_base = req->send.buffer,
                .iov_len  = req->send.length
        };

        uint64_t imm = 0;
        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_P2P_TM_MESSAGE, 
                            req->send.length, req->seqn);

        req->send.t_ctx.req = req;
        req->send.t_ctx.comp = (lcr_completion_t) {
                .comp_cb = lcp_request_complete_tag_offload
        };

        mpc_common_debug_info("LCP: post send tag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%d:%d:%d]", 
                              req, req->send.tag.src, req->send.tag.dest, req->send.length, tag.t_tag.tag, 
                              tag.t_tag.src, tag.t_tag.comm);
        return lcp_send_do_tag_zcopy(lcr_ep, tag, imm, &iov, 1, &(req->send.t_ctx));

}


/* ============================================== */
/* Recv                                           */
/* ============================================== */

void lcp_recv_tag_callback(lcr_completion_t *comp) 
{
        uint8_t op;
        uint64_t comm_id;
        uint64_t gid;
        int32_t src;
        mpc_lowcomm_communicator_t comm;
        lcr_tag_t tag;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              recv.t_ctx.comp);

        op                    = LCP_TM_GET_HDR_OP(req->recv.t_ctx.imm);
        req->recv.send_length = LCP_TM_GET_HDR_LENGTH(req->recv.t_ctx.imm); 
        req->seqn             = LCP_TM_GET_HDR_SEQN(req->recv.t_ctx.imm);
        tag                   = req->recv.t_ctx.tag;
        src                   = LCP_TM_GET_SRC(tag.t);

        gid = mpc_lowcomm_monitor_get_gid();
        comm_id = LCP_TM_GET_COMM(tag.t);
        comm_id |= gid << 32;

        mpc_common_debug_info("LCP: tag callback req=%p, src=%d, size=%d, matching=[%d:%d:%d], "
                              "comm id=%llu", req, src, req->recv.send_length, tag.t_tag.tag, 
                              tag.t_tag.src, tag.t_tag.comm, comm_id);

        comm = mpc_lowcomm_get_communicator_from_id(comm_id);
        req->recv.tag.src = mpc_lowcomm_communicator_uid(comm, src);
        req->recv.tag.tag = LCP_TM_GET_TAG(tag.t);

        switch(op) {
        case MPC_LOWCOMM_P2P_TM_MESSAGE:
                if (req->recv.length < comp->sent) {
                        req->flags |= LCP_REQUEST_RECV_TRUNC;
                } else {
                        if (req->recv.t_ctx.flags & LCR_IFACE_TM_OVERFLOW) {
                                memcpy(req->recv.buffer, req->recv.t_ctx.start, 
                                       req->recv.send_length);
                        }

                        /* set recv info for caller */
                        req->recv.recv_info->length = comp->sent;
                        req->recv.recv_info->src    = req->recv.tag.src;
                        req->recv.recv_info->tag    = req->recv.tag.tag;
                }
                lcp_request_complete(req);
                break;
        case MPC_LOWCOMM_RSEND_TM_MESSAGE:
                lcp_recv_rsend(req, (void *)&req->recv.t_ctx.tag.t);
                break;
        case MPC_LOWCOMM_RGET_TM_MESSAGE:
                lcp_recv_rget(req, req->recv.t_ctx.start);
                break;
        case MPC_LOWCOMM_RPUT_TM_MESSAGE:
                lcp_recv_rput(req, NULL);
                break;
        default:
                mpc_common_debug_error("LCP: unknown recv protocol.");
                break;
        }

        return;
}

int lcp_recv_tag_zcopy(lcp_request_t *rreq, sctk_rail_info_t *iface)
{
        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, rreq->recv.tag.src, 
                             rreq->recv.tag.tag, 
                             rreq->recv.tag.comm_id);

        lcr_tag_t ign_tag = {.t = 0 };
        if ((int)rreq->recv.tag.src == MPC_ANY_SOURCE) {
                ign_tag.t |= LCP_TM_SRC_MASK;
        }
        if ((int)rreq->recv.tag.src == MPC_ANY_TAG) {
                ign_tag.t |= LCP_TM_TAG_MASK;
        }

        struct iovec iov = {
                .iov_base = rreq->recv.buffer,
                .iov_len  = rreq->recv.length
        };

        rreq->recv.t_ctx.req          = rreq;
        rreq->recv.t_ctx.comp.comp_cb = lcp_recv_tag_callback;

        mpc_common_debug_info("LCP: post recv tag zcopy req=%p, src=%d, dest=%d, "
                              "size=%d, matching=[%d:%d:%d], ignore=[%d:%d:%d]", 
                              rreq, rreq->recv.tag.src, rreq->recv.tag.dest, 
                              rreq->recv.length, tag.t_tag.tag, tag.t_tag.src, 
                              tag.t_tag.comm, ign_tag.t_tag.tag, ign_tag.t_tag.src,
                              ign_tag.t_tag.comm);
        return lcp_recv_do_tag_zcopy(iface,
                                     tag,
                                     ign_tag,
                                     &iov,
                                     1,
                                     &(rreq->recv.t_ctx));

}

//FIXME: dead code ?
static inline int lcp_recv_tag_frag_zcopy(lcp_request_t *rreq, 
                                          sctk_rail_info_t *iface,
                                          size_t offset,
                                          int frag_id,
                                          size_t length)
{
        lcr_tag_t tag = {.t_frag = {
                .f_id = frag_id,
                .m_id = rreq->msg_id }
        };
        lcr_tag_t ign_tag = {.t = 0};

        struct iovec iov = {
                .iov_base = rreq->recv.buffer + offset,
                .iov_len  = length 
        };

        rreq->recv.t_ctx.req = rreq;
        rreq->recv.t_ctx.tag = tag;

        mpc_common_debug_info("LCP: post recv frag zcopy req=%p, src=%d, dest=%d, size=%d, matching=[%llu, %d]", 
                              rreq, rreq->recv.tag.src, rreq->recv.tag.dest, length, tag.t_frag.f_id, 
                              tag.t_frag.m_id);
        return lcp_recv_do_tag_zcopy(iface,
                                     tag,
                                     ign_tag,
                                     &iov,
                                     1,
                                     &(rreq->recv.t_ctx));
}

//FIXME: dead code ?
int lcp_recv_tag_zcopy_multi(lcp_ep_h ep, lcp_request_t *rreq)
{
        int rc;
        size_t remaining, offset, length, frag_length;
        int ifrag;
        _mpc_lowcomm_endpoint_t *lcr_ep;
        lcr_rail_attr_t attr;

        /* size from rndv has been set */
        assert(rreq->recv.send_length > 0);

        /* Fragment message */
        remaining = rreq->recv.send_length;
        offset = 0;
        ifrag = 0;
        while (remaining > 0) {
                lcr_ep = ep->lct_eps[rreq->state.cc];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_send_zcopy;

                length = remaining < frag_length ? remaining : frag_length;

                rc = lcp_recv_tag_frag_zcopy(rreq, lcr_ep->rail, 
                                             offset, ifrag+1, length);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                mpc_common_debug_info("LCP: recv frag n=%d, src=%d, dest=%d, msg_id=%llu, offset=%d, "
                                      "len=%d", ifrag, rreq->recv.tag.src, rreq->recv.tag.dest,
                                      rreq->msg_id, offset, length);

                offset += length;
                //FIXME: should number of computed used interface.
                rreq->state.cc = (rreq->state.cc + 1) % ep->num_chnls;
                remaining = rreq->recv.send_length - offset;
                ifrag++;
        }

err:
        return rc;
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */

//FIXME: dead code ?
static int lcp_tag_handler(void *arg, 
                           void *data, 
                           size_t length,
                           unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcr_tag_context_t *t_ctx = arg;
        lcp_request_t *req  = t_ctx->req;

        if (req->recv.length > length) {
                req->flags |= LCP_REQUEST_RECV_TRUNC;
        }

        if (flags & LCR_IFACE_TM_OVERFLOW) {
                memcpy(req->recv.buffer, data, length);
        }
        lcp_request_complete(req);

        return rc;
}

//NOTE: data pointer unused since matching of fragments are always in priority.
//FIXME: dead code ?
static int lcp_tag_frag_handler(void *arg, 
                                __UNUSED__ void *data, 
                                size_t length,
                                __UNUSED__ unsigned flags)
{
        int rc;
        lcr_tag_context_t *t_ctx = arg;
        lcp_request_t *req = t_ctx->req;
        lcp_context_h ctx = req->ctx;

        LCP_CONTEXT_LOCK(ctx);
        req->state.remaining -= length;
        req->state.offset    += length;
        mpc_common_debug("LCP: req=%p, remaining=%d, len=%lu, n=%d.", 
                         req, req->state.remaining, length, 
                         t_ctx->tag.t_frag.f_id);
        LCP_CONTEXT_UNLOCK(ctx);

        if (req->state.remaining == 0) {
                lcp_request_complete(req);
        }

        rc = MPC_LOWCOMM_SUCCESS;

        return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_FRAG_TM_MESSAGE, lcp_tag_frag_handler, 0);
