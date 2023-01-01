#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

/* ============================================== */
/* Utils                                          */
/* ============================================== */

static inline int build_memory_registration_bitmap(size_t length, 
                                                   size_t min_frag_size, 
                                                   int max_iface,
                                                   bmap_t *bmap_p)
{
        bmap_t bmap = MPC_BITMAP_INIT;
        int num_used_ifaces = 0;

        while ((length > num_used_ifaces * min_frag_size) &&
               (num_used_ifaces < max_iface)) {
                MPC_BITMAP_SET(bmap, num_used_ifaces);
                num_used_ifaces++;
        }

        *bmap_p = bmap;

        return num_used_ifaces;
}

/* ============================================== */
/* Packing                                        */
/* ============================================== */

static size_t lcp_rts_rget_pack(void *dest, void *data)
{
        lcp_rndv_hdr_t *hdr = dest;
        lcp_request_t *req = data;
        void *rkey_buf;
        int packed_size;

        hdr->base.comm = req->send.tag.comm_id;
        hdr->base.src  = req->send.rndv.src;
        hdr->base.tag  = req->send.rndv.tag;
        hdr->base.seqn = req->seqn;

        hdr->msg_id    = req->msg_id;
        hdr->size      = req->send.length;

        rkey_buf = hdr + 1;
        packed_size = lcp_mem_pack(req->ctx, rkey_buf, 
                                   req->state.lmem,
                                   req->state.memp_map);
        return sizeof(*hdr) + packed_size;
}

static size_t lcp_rts_rput_pack(void *dest, void *data)
{
        lcp_rndv_hdr_t *hdr = dest;
        lcp_request_t *req = data;

        hdr->base.comm = req->send.tag.comm_id;
        hdr->base.src  = req->send.rndv.src;
        hdr->base.tag  = req->send.rndv.tag;
        hdr->base.seqn = req->seqn;

        hdr->msg_id    = req->msg_id;
        hdr->size      = req->send.length;

        return sizeof(*hdr);
}

static size_t lcp_rndv_fin_pack(void *dest, void *data)
{
        lcp_rndv_ack_hdr_t *hdr = dest;
        lcp_request_t *req = data;

        hdr->msg_id = req->msg_id;
        hdr->src    = req->send.am.src;

        return sizeof(*hdr);
}

static size_t lcp_rtr_rput_pack(void *dest, void *data)
{
        lcp_rndv_ack_hdr_t *hdr = dest;
        lcp_request_t *req = data;
        lcp_request_t *super = req->super;
        int packed_size;
        void *rkey_ptr;

        hdr->msg_id = super->msg_id;
        hdr->src    = super->send.am.src;

        rkey_ptr = hdr + 1;
        packed_size = lcp_mem_pack(super->ctx, rkey_ptr, 
                                   super->state.lmem, 
                                   super->state.memp_map);

        return sizeof(*hdr) + packed_size;
}

/* ============================================== */
/* Rendez-vous send completion callback           */
/* ============================================== */

/* Rendez-vous send completion callback on sender side */
void lcp_request_complete_rsend(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_request_complete(req);
        } 
}

int lcp_send_rput_common(lcp_request_t *super, lcp_mem_h rmem)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i, num_used_ifaces;
        size_t remaining, offset;
        size_t frag_length, length;
        size_t *per_ep_length;
        lcp_ep_h ep;
        lcr_rail_attr_t attr;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        /* Set the length to be sent on each interface. The minimum size that
         * can be sent on a interface is defined by min_frag_size */
        num_used_ifaces = rmem->num_ifaces;
        per_ep_length   = sctk_malloc(num_used_ifaces * sizeof(size_t));
        for (i=0; i<num_used_ifaces; i++) {
                per_ep_length[i] = (size_t)super->send.length / num_used_ifaces;
        }
        per_ep_length[0] += super->send.length % num_used_ifaces;

        super->state.comp_stg = 1;
        remaining   = super->send.length;
        offset      = 0;
        ep          = super->send.ep;

        while (remaining > 0) {
                lcr_ep = ep->lct_eps[super->state.cc];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                /* length is min(max_frag_size, per_ep_length) */
                frag_length = attr.iface.cap.rndv.max_get_zcopy;
                length = per_ep_length[super->state.cc] < frag_length ? 
                        per_ep_length[super->state.cc] : frag_length;

                //FIXME: error managment => NO_RESOURCE not handled
                rc = lcp_send_do_put_zcopy(lcr_ep,
                                           (uint64_t)super->send.buffer + offset,
                                           offset,
                                           &(rmem->mems[super->state.cc]),
                                           length,
                                           &(super->send.t_ctx));

                offset    += length; remaining -= length;
                per_ep_length[super->state.cc] -= length;

                super->state.cc = (super->state.cc + 1) % num_used_ifaces;
        }

        return rc;
}

/* Recv ack callback from rendez-vous put protocol. Upon reception, extract
 * memory registration contexts sent by target and start running put on those
 * contexts */ 
void lcp_request_rput_ack_callback(lcr_completion_t *comp)
{
        int rc;
        lcp_mem_h rmem = NULL; 

        lcp_request_t *ack_req = mpc_container_of(comp, lcp_request_t, 
                                                  recv.t_ctx.comp);

        lcp_request_t *super = ack_req->super;

        /* first create and unpack remote key */
        rc = lcp_mem_create(super->ctx, &rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = lcp_mem_unpack(super->ctx, 
                            rmem, 
                            ack_req->recv.t_ctx.start,
                            super->state.memp_map);
        if (rc < 0) {
                goto err;
        }
        sctk_free(ack_req->recv.buffer);
        lcp_request_complete(ack_req);

        rc = lcp_send_rput_common(super, rmem);

        lcp_mem_delete(rmem);
err:
        return;

}

void lcp_request_complete_am_rput(lcr_completion_t *comp)
{
        int payload;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);

        lcp_ep_h ep = req->send.ep;

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {

                payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                               MPC_LOWCOMM_RFIN_MESSAGE,
                                               lcp_rndv_fin_pack,
                                               req);
                if (payload < 0) {
                        mpc_common_debug_error("LCP: error sending fin message");
                }

                lcp_request_complete(req);
        }
}

/* Rendez-vous put callback for completion */
void lcp_request_complete_rput(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);
        if (req->state.comp_stg == 0) {
                /* Stage 1: rendez-vous has been received by target, nothing 
                 * to do. */
        } else if (req->state.comp_stg == 1) {
                /* Stage 2: a put has completed on target, check if all data has
                 * been sent, if so send FIN message */
                req->state.remaining -= comp->sent;
                if (req->state.remaining == 0) {
                        lcp_ep_h ep;
                        struct iovec iov_fin[1];

                        iov_fin[0].iov_base = NULL;
                        iov_fin[0].iov_len = 0; 

                        ep = req->send.ep;
                        req->state.comp_stg = 2;

                        lcr_tag_t tag = { 0 };
                        LCP_TM_SET_MATCHBITS(tag.t, 
                                             req->send.rndv.src, 
                                             req->send.rndv.tag, 
                                             req->seqn);

                        lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                              tag,
                                              0,
                                              iov_fin,
                                              1,
                                              &(req->send.t_ctx));
                }
        } else {
                /* Stage 3: FIN message has been sent and received by target */
                lcp_request_complete(req);
        }
        return;
}

/* ============================================== */
/* Memory registration                            */
/* ============================================== */

int lcp_rndv_reg_send_buffer(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep = req->send.ep;
        sctk_rail_info_t *rail = ep->lct_eps[ep->priority_chnl]->rail;
        lcr_rail_attr_t attr;

        /* build registration bitmap */
        rail->iface_get_attr(rail, &attr);
        build_memory_registration_bitmap(req->send.length, 
                                         attr.iface.cap.rndv.min_frag_size,
                                         req->ctx->num_resources,
                                         &req->state.memp_map);

        /* Register and pack memory pin context that will be sent to remote */
        rc = lcp_mem_register(req->send.ep->ctx, 
                              &(req->state.lmem), 
                              req->send.buffer, 
                              req->send.length,
                              req->state.memp_map);

        return rc;
}

/* ============================================== */
/* Protocol starts                                */
/* ============================================== */

int lcp_send_rsend_start(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep = req->send.ep;

        uint64_t imm = 0;
        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_RSEND_TM_MESSAGE, req->send.length);

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->seqn);

        struct iovec iov[2];
        iov[0].iov_base = NULL;
        iov[0].iov_len  = 0;
        iov[1].iov_base = req->send.buffer;
        iov[1].iov_len  = req->send.length;

        req->send.t_ctx.req          = req;
        req->send.t_ctx.comm_id      = req->send.rndv.comm_id;
        req->send.t_ctx.comp.comp_cb = lcp_request_complete_rsend;

        /* Only priority iface should be registered */
        MPC_BITMAP_SET(req->state.memp_map, ep->priority_chnl);

        rc = lcp_send_do_tag_rndv_zcopy(ep->lct_eps[ep->priority_chnl],
                                        tag,
                                        imm,
                                        iov,
                                        2,
                                        &(req->send.t_ctx));

        return rc;
}

void lcp_request_complete_srget(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);

        if (req->state.comp_stg == 0) {
                req->state.comp_stg = 1;
        } else {
                lcp_mem_deregister(req->ctx, 
                                   req->state.lmem,
                                   req->state.memp_map);
                lcp_request_complete(req);
        }
}

//FIXME: bitmap construction suppose memory could be registered on both side and
//       identically. When packing, we should add the interface index to know to
//       which interface belong the matching tag. What happens when only
//       interface 1 and 3 were able to register ? How can the receiver find out
//       which interface to use ?
int lcp_send_rget_start(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep = req->send.ep;
        uint64_t imm = 0;
        struct iovec iov_send[1], iov_fin[1];

        /* Set immediate and tag for matching */
        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_RGET_TM_MESSAGE, 
                            req->send.length);

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->seqn);

        lcr_tag_t ign_tag = {
                .t = 0
        };

        iov_send[0].iov_base = sctk_malloc(req->state.lmem->num_ifaces*sizeof(lcr_memp_t));
        iov_send[0].iov_len  = lcp_mem_pack(req->send.ep->ctx, 
                                            iov_send[0].iov_base, 
                                            req->state.lmem,
                                            req->state.memp_map);

        /* Post FIN before sending RGET */
        iov_fin[0].iov_base = NULL;
        iov_fin[0].iov_len = 0; 

        req->send.t_ctx.req          = req;
        req->send.t_ctx.comm_id      = req->send.rndv.comm_id;
        req->send.t_ctx.comp.comp_cb = lcp_request_complete_srget;
        req->state.comp_stg          = 0;

        rc = lcp_recv_do_tag_zcopy(ep->lct_eps[ep->priority_chnl]->rail,
                                   tag,
                                   ign_tag,
                                   iov_fin,
                                   1,
                                   &(req->send.t_ctx));
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Finally, post send RGET */
        rc = lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                   tag,
                                   imm,
                                   iov_send,
                                   1,
                                   &(req->send.t_ctx));

err:
        return rc;
}

int lcp_send_rput_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep = req->send.ep;
        sctk_rail_info_t *rail = ep->lct_eps[ep->priority_chnl]->rail;
        lcr_rail_attr_t attr;
        uint64_t imm = 0;
        struct iovec iov_send[1];
        struct iovec iov_ack[1];
        int num_used_ifaces;
        lcp_request_t *ack;

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->seqn);

        /* Start with registration bitmap context build */
        rail->iface_get_attr(rail, &attr);
        num_used_ifaces = build_memory_registration_bitmap(req->send.length,
                                                           attr.iface.cap.rndv.min_frag_size,
                                                           req->ctx->num_resources,
                                                           &req->state.memp_map);

        /* Prepare reception of remote memory pin contexts */
        lcp_request_create(&ack);
        if (ack == NULL) {
                goto err;
        }
        ack->super = req;

        /* Size of the remote memory pin contexts will be no longer than
         * the number of interface used times the size of a context */ 
        ack->recv.length = num_used_ifaces * sizeof(lcr_memp_t); 
        ack->recv.buffer = sctk_malloc(ack->recv.length);

        lcr_tag_t ign_tag = {
                .t = 0
        };

        iov_ack[0].iov_base = ack->recv.buffer;
        iov_ack[0].iov_len  = ack->recv.length;

        ack->recv.t_ctx.comm_id      = req->send.rndv.comm_id;
        ack->recv.t_ctx.req          = ack;
        ack->recv.t_ctx.comp.comp_cb = lcp_request_rput_ack_callback;

        /* Post the receive to get remote memory contexts */
        rc = lcp_recv_do_tag_zcopy(ep->lct_eps[ep->priority_chnl]->rail,
                                   tag,
                                   ign_tag,
                                   iov_ack,
                                   1,
                                   &(ack->recv.t_ctx));
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Inside immediate data (header), set RPUT protocol and length of
         * request */
        LCP_TM_SET_HDR_DATA(imm, MPC_LOWCOMM_RPUT_TM_MESSAGE, 
                            req->send.length);

        /* No data to be sent for RPUT */
        iov_send[0].iov_base = NULL;
        iov_send[0].iov_len  = 0;

        req->send.t_ctx.req          = req;
        req->state.comp_stg          = 0;
        req->send.t_ctx.comp.comp_cb = lcp_request_complete_rput;
        req->send.t_ctx.comm_id      = req->send.rndv.comm_id;

        /* Finally, send message rendez-vous request */
        rc = lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                   tag,
                                   imm,
                                   iov_send,
                                   1,
                                   &(req->send.t_ctx));

        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
err:
        return rc;
}

int lcp_send_am_rts_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = ep->priority_chnl;

        if (lcp_pending_create(req->ctx, req, req->msg_id) == NULL) {
                mpc_common_debug_error("LCP: could not add pending message");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        switch (ep->ctx->config.rndv_mode) {
        case LCP_RNDV_GET:
                rc = lcp_rndv_reg_send_buffer(req);
                payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc], 
                                                    MPC_LOWCOMM_RGET_MESSAGE,
                                                    lcp_rts_rget_pack, req);
                if (payload_size < 0) {
                        rc = MPC_LOWCOMM_ERROR;
                }
                break;
        case LCP_RNDV_PUT:
                req->send.t_ctx.comp.comp_cb = lcp_request_complete_am_rput;
                payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc], 
                                                    MPC_LOWCOMM_RPUT_MESSAGE,
                                                    lcp_rts_rput_pack, req);
                if (payload_size < 0) {
                        rc = MPC_LOWCOMM_ERROR;
                }
                break;
        case LCP_RNDV_SEND:
                not_reachable();
                break;
        default:
                mpc_common_debug_error("LCP: unknown rndv type");
                rc = MPC_LOWCOMM_ERROR;
                break;
        }

err:
        return rc;
}

int lcp_send_rndv_start(lcp_request_t *req) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = ep->priority_chnl;

        mpc_common_debug_info("LCP: send rndv req=%p, comm_id=%lu, tag=%d, "
                              "src=%d, dest=%d, msg_id=%d.", req, req->send.rndv.comm_id, 
                              req->send.rndv.tag, req->send.rndv.src, 
                              req->send.rndv.dest, req->msg_id);

        if (LCR_IFACE_IS_TM(ep->lct_eps[cc]->rail)) {
                lcr_rail_attr_t attr; 
                ep->lct_eps[cc]->rail->iface_get_attr(ep->lct_eps[cc]->rail, &attr);

                switch(ep->ctx->config.rndv_mode) {
                case LCP_RNDV_SEND:
                        req->send.func = lcp_send_rsend_start;
                        break;
                case LCP_RNDV_GET:
                        rc = lcp_rndv_reg_send_buffer(req);
                        req->send.func = lcp_send_rget_start;
                        break;
                case LCP_RNDV_PUT:
                        rc = lcp_rndv_reg_send_buffer(req);
                        req->send.func = lcp_send_rput_start;
                        break;
                default:
                        mpc_common_debug_fatal("LCP: unknown protocol");
                } 	
        } else {
                req->send.func = lcp_send_am_rts_start;
        }

        return rc;
}

int lcp_recv_rget(lcp_request_t *req, void *hdr);
int lcp_recv_rput(lcp_request_t *req);

int lcp_rndv_matched(lcp_context_h ctx, 
                     lcp_request_t *rreq,
                     lcp_rndv_hdr_t *hdr,
                     lcp_rndv_mode_t rndv_mode) 
{
        int rc;
        lcp_ep_ctx_t *ctx_ep;
        lcp_ep_h ep;

        /* Init all protocol data */
        rreq->msg_id            = hdr->msg_id;
        rreq->recv.send_length  = hdr->size; 
        rreq->state.remaining   = hdr->size;
        rreq->state.offset      = 0;
        rreq->seqn              = hdr->base.seqn;

        /* Get LCP endpoint if exists */
        HASH_FIND(hh, ctx->ep_ht, &hdr->base.src, sizeof(uint64_t), ctx_ep);
        if (ctx_ep == NULL) {
                rc = lcp_ep_create(ctx, &ep, hdr->base.src, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep after match.");
                        goto err;
                }
        } else {
                ep = ctx_ep->ep;
        }

        switch (rndv_mode) {
        case LCP_RNDV_SEND:
                break;
        case LCP_RNDV_PUT:
                rc = lcp_recv_rput(rreq);
                break;
        case LCP_RNDV_GET:
                rc = lcp_recv_rget(rreq, hdr + 1);
                break;
        default:
                mpc_common_debug_error("LCP: unknown protocol in recv");
                rc = MPC_LOWCOMM_ERROR;
                break;
        }

err:
        return rc;
}

/* ============================================== */
/* Rendez-vous recv completion callback           */
/* ============================================== */

void lcp_request_complete_recv_rsend(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              recv.t_ctx.comp);

        lcp_request_complete(req);
}

void lcp_request_complete_recv_rget(lcr_completion_t *comp)
{
        int rc;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              recv.t_ctx.comp);

        if (req->state.comp_stg == 0) {
                req->state.remaining -= comp->sent;
                if (req->state.remaining == 0) {
                        lcp_ep_h ep;
                        lcp_ep_ctx_t *ctx_ep;
                        struct iovec iov_fin[1];

                        iov_fin[0].iov_base = NULL;
                        iov_fin[0].iov_len = 0; 
                        /* Get LCP endpoint if exists */
                        HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, sizeof(uint64_t), ctx_ep);
                        assert(ctx_ep);

                        ep = ctx_ep->ep;
                        req->state.comp_stg = 1;

                        if (LCR_IFACE_IS_TM(ep->lct_eps[ep->priority_chnl]->rail)) {
                                rc = lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                                           req->recv.t_ctx.tag,
                                                           0,
                                                           iov_fin,
                                                           1,
                                                           &(req->recv.t_ctx));
                        } else {
                                lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                                     MPC_LOWCOMM_RFIN_MESSAGE,
                                                     lcp_rndv_fin_pack,
                                                     req);
                        }

                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
        } else {
                lcp_request_complete(req);
        }
err:
        return;
}

void lcp_request_complete_recv_rput(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              recv.t_ctx.comp);

        lcp_mem_deregister(req->ctx, req->state.lmem, req->state.memp_map);

        lcp_request_complete(req);
}

void lcp_request_complete_srack(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);

        lcp_request_complete(req);
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */
int lcp_recv_rsend(lcp_request_t *req, void *hdr)
{
        int rc;
        lcp_mem_h rmem = NULL; 
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;

        /* Get LCP endpoint if exists */
        HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, sizeof(uint64_t), ctx_ep);
        if (ctx_ep == NULL) {
                rc = lcp_ep_create(req->ctx, &ep, req->recv.tag.src, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep after match.");
                        goto err;
                }
        } else {
                ep = ctx_ep->ep;
        }

        /* first create and unpack remote key */
        rc = lcp_mem_create(req->ctx, &rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
        MPC_BITMAP_SET(req->state.memp_map, ep->priority_chnl);
        rc = lcp_mem_unpack(req->ctx, rmem, hdr, req->state.memp_map);
        if (rc < 0) {
                goto err;
        }

        req->recv.t_ctx.comp.comp_cb = lcp_request_complete_recv_rsend;
        rc = lcp_send_do_get_zcopy(ep->lct_eps[req->state.cc],
                                   (uint64_t)req->recv.buffer,
                                   0,
                                   &rmem->mems[req->state.cc],
                                   req->recv.send_length,
                                   &(req->recv.t_ctx));

        lcp_mem_delete(rmem);
err:
        return rc;
}


int lcp_recv_rget(lcp_request_t *req, void *hdr)
{
        int rc, f_id, i, num_used_ifaces;
        size_t remaining, offset;
        size_t frag_length, length;
        size_t *per_ep_length;
        lcp_mem_h rmem = NULL; 
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;
        lcr_rail_attr_t attr;
        sctk_rail_info_t *rail;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        /* Get LCP endpoint if exists */
        HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, sizeof(uint64_t), ctx_ep);
        if (ctx_ep == NULL) {
                rc = lcp_ep_create(req->ctx, &ep, req->recv.tag.src, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep after match.");
                        goto err;
                }
        } else {
                ep = ctx_ep->ep;
        }

        /* Build registration bitmap */
        rail = ep->lct_eps[ep->priority_chnl]->rail;
        rail->iface_get_attr(rail, &attr);
        num_used_ifaces = build_memory_registration_bitmap(req->recv.send_length,
                                                           attr.iface.cap.rndv.min_frag_size,
                                                           req->ctx->num_resources,
                                                           &req->state.memp_map);

        /* Create and unpack remote key */
        rc = lcp_mem_create(req->ctx, &rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = lcp_mem_unpack(req->ctx, rmem, hdr, req->state.memp_map);
        if (rc < 0) {
                goto err;
        }

        /* Compute length per endpoints that will be get */
        per_ep_length        = sctk_malloc(num_used_ifaces * sizeof(size_t));
        for (i=0; i<num_used_ifaces; i++) {
                per_ep_length[i] = (size_t)req->recv.send_length / num_used_ifaces;
        }
        per_ep_length[0] += req->recv.send_length % num_used_ifaces;

        req->state.comp_stg = 0;
        req->recv.t_ctx.comp.comp_cb = lcp_request_complete_recv_rget;
        req->state.remaining = remaining = req->recv.send_length;
        f_id                 = 0;
        offset               = 0;

        while (remaining > 0) {
                lcr_ep = ep->lct_eps[req->state.cc];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_get_zcopy;
                length = per_ep_length[req->state.cc] < frag_length ? 
                        per_ep_length[req->state.cc] : frag_length;

                rc = lcp_send_do_get_zcopy(lcr_ep,
                                           (uint64_t)req->recv.buffer + offset, 
                                           offset, 
                                           &(rmem->mems[req->state.cc]),
                                           length,
                                           &(req->recv.t_ctx));

                per_ep_length[req->state.cc] -= length;
                offset += length; remaining  -= length;

                req->state.cc = (req->state.cc + 1) % num_used_ifaces;
                f_id++;
        }

        sctk_free(per_ep_length);
        lcp_mem_delete(rmem);
err:
        return rc;
}

int lcp_recv_rput(lcp_request_t *req)
{
        int rc, payload, num_used_ifaces;
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;
        sctk_rail_info_t *rail;
        lcr_rail_attr_t attr;
        lcp_request_t *ack;
        struct iovec iov_ack[1];
        struct iovec iov_fin[1];

        /* Get endpoint */
        //FIXME: redundant in am handler
        HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, sizeof(uint64_t), ctx_ep);
        if (ctx_ep == NULL) {
                rc = lcp_ep_create(req->ctx, &ep, req->recv.tag.src, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep after match.");
                        goto err;
                }
        } else {
                ep = ctx_ep->ep;
        }

        /* Build registration bitmap */
        rail = ep->lct_eps[ep->priority_chnl]->rail;
        rail->iface_get_attr(rail, &attr);
        num_used_ifaces = build_memory_registration_bitmap(req->recv.send_length,
                                                           attr.iface.cap.rndv.min_frag_size,
                                                           req->ctx->num_resources,
                                                           &req->state.memp_map);

        /* Register memory on which the put will be done */
        rc = lcp_mem_register(req->ctx, 
                              &(req->state.lmem), 
                              req->recv.buffer, 
                              req->recv.send_length,
                              req->state.memp_map);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Prepare ack request to send back memory registration contexts */
        lcp_request_create(&ack);
        if (ack == NULL) {
                goto err;
        }
        ack->super = req;

        if (LCR_IFACE_IS_TM(ep->lct_eps[ep->priority_chnl]->rail)) {
                /* Packed length is inferior or equal to the size of a memory context
                 * times the number of context to be sent */
                ack->send.buffer = sctk_malloc(num_used_ifaces * sizeof(lcr_memp_t));
                ack->send.length = lcp_mem_pack(ep->ctx, 
                                                ack->send.buffer, 
                                                req->state.lmem,
                                                req->state.memp_map);

                iov_ack[0].iov_base = ack->send.buffer;
                iov_ack[0].iov_len  = ack->send.length;
                ack->send.t_ctx.comm_id      = req->recv.tag.comm_id;
                ack->send.t_ctx.req          = ack;
                ack->send.t_ctx.comp.comp_cb = lcp_request_complete_srack; 
                rc = lcp_send_do_tag_zcopy(ep->lct_eps[ep->priority_chnl],
                                           req->recv.t_ctx.tag,
                                           0,
                                           iov_ack,
                                           1,
                                           &(ack->send.t_ctx));

                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                lcr_tag_t ign_tag = {
                        .t = 0
                };

                iov_fin[0].iov_base = NULL;
                iov_fin[0].iov_len = 0; 

                req->recv.t_ctx.comp.comp_cb = lcp_request_complete_recv_rput;

                rc = lcp_recv_do_tag_zcopy(ep->lct_eps[ep->priority_chnl]->rail,
                                           req->recv.t_ctx.tag,
                                           ign_tag,
                                           iov_fin,
                                           1,
                                           &(req->recv.t_ctx));
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        } else {
                if (lcp_pending_create(req->ctx, req, req->msg_id) == NULL) {
                        mpc_common_debug_error("LCP: could not add pending message");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                               MPC_LOWCOMM_ACK_RDV_MESSAGE,
                                               lcp_rtr_rput_pack,
                                               ack);
                if (payload < 0) {
                        mpc_common_debug_error("LCP: error sending ack rdv message");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                rc = MPC_LOWCOMM_SUCCESS;
        }

err:
        return rc;
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */

//NOTE: flags could be used to avoid duplicating functions for
//      AM and TAG
int lcp_rndv_am_rput_handler(void *arg, void *data,
                            size_t length, 
                            __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_rndv_hdr_t *hdr = data;
        lcp_request_t *req;
        lcp_unexp_ctnr_t *ctnr;

        /* No payload in rndv message for now */
        assert(length == sizeof(lcp_rndv_hdr_t));

        LCP_CONTEXT_LOCK(ctx);

        mpc_common_debug_info("LCP: recv rndv header src=%d, msg_id=%llu",
                              hdr->base.src, hdr->msg_id);

        req = lcp_match_prq(ctx->prq_table, hdr->base.comm, 
                            hdr->base.tag, hdr->base.src);
        if (req != NULL) {
                mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%lu, " 
                                      "tag=%d, src=%lu.", req, req->recv.tag.comm_id, 
                                      req->recv.tag.tag, req->recv.tag.src);
                LCP_CONTEXT_UNLOCK(ctx); //NOTE: unlock context to enable endpoint creation.
                rc = lcp_rndv_matched(ctx, req, hdr, LCP_RNDV_PUT);
        } else {
                lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
                                            LCP_RECV_CONTAINER_UNEXP_RPUT);
                lcp_append_umq(ctx->umq_table, (void *)ctnr, hdr->base.comm, 
                               hdr->base.tag, hdr->base.src);
                LCP_CONTEXT_UNLOCK(ctx);
                rc = MPC_LOWCOMM_SUCCESS;
        }

        return rc;
}

static int lcp_ack_am_tag_handler(void *arg, void *data,
                                  __UNUSED__ size_t size, 
                                  __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_rndv_ack_hdr_t *hdr = data;
        lcp_mem_h rmem = NULL;
        lcp_request_t *req;

        mpc_common_debug_info("LCP: recv tag ack header src=%d, "
                              "msg_id=%llu.", hdr->src, hdr->msg_id);

        /* Retrieve request */
        req = lcp_pending_get_request(ctx, hdr->msg_id);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not find ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        /* first create and unpack remote key */
        rc = lcp_mem_create(req->ctx, &rmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = lcp_mem_unpack(req->ctx, 
                            rmem, 
                            hdr + 1,
                            req->state.memp_map);
        if (rc < 0) {
                goto err;
        }

        rc = lcp_send_rput_common(req, rmem);

err:
        return rc;
}

static int lcp_rndv_am_rget_handler(void *arg, void *data,
                                    size_t length, 
                                  __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_rndv_hdr_t *hdr = data;
        lcp_request_t *req;
        lcp_unexp_ctnr_t *ctnr;

        LCP_CONTEXT_LOCK(ctx);

        mpc_common_debug_info("LCP: recv rndv header src=%d, msg_id=%llu",
                              hdr->base.src, hdr->msg_id);

        req = lcp_match_prq(ctx->prq_table, hdr->base.comm, 
                            hdr->base.tag, hdr->base.src);
        if (req != NULL) {
                mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%lu, " 
                                      "tag=%d, src=%lu.", req, req->recv.tag.comm_id, 
                                      req->recv.tag.tag, req->recv.tag.src);
                LCP_CONTEXT_UNLOCK(ctx); //NOTE: unlock context to enable endpoint creation.
                rc = lcp_rndv_matched(ctx, req, hdr, LCP_RNDV_GET);
        } else {
                lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
                                            LCP_RECV_CONTAINER_UNEXP_RGET);
                lcp_append_umq(ctx->umq_table, (void *)ctnr, hdr->base.comm, 
                               hdr->base.tag, hdr->base.src);
                LCP_CONTEXT_UNLOCK(ctx);
                rc = MPC_LOWCOMM_SUCCESS;
        }

        return rc;
}

static int lcp_rndv_am_fin_handler(void *arg, void *data,
                                   __UNUSED__ size_t length, 
                                   __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_rndv_ack_hdr_t *hdr = data;
        lcp_request_t *req;

        /* Retrieve request */
        req = lcp_pending_get_request(ctx, hdr->msg_id);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not find ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        lcp_mem_deregister(req->ctx, req->state.lmem, req->state.memp_map);
        lcp_request_complete(req);

err:
        return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_ACK_RDV_MESSAGE, lcp_ack_am_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RGET_MESSAGE, lcp_rndv_am_rget_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RPUT_MESSAGE, lcp_rndv_am_rput_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RFIN_MESSAGE, lcp_rndv_am_fin_handler, 0);
