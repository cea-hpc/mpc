#include "lcp_prototypes.h"

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_rndv.h"
#include "lcp_mem.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

enum {
        LCP_PROTOCOL_EAGER = 0,
        LCP_PROTOCOL_RGET,
        LCP_PROTOCOL_RPUT
};

//FIXME: function also needed in lcp_mem.c
static inline void build_memory_bitmap(size_t length, 
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
}

/* ============================================== */
/* Packing                                        */
/* ============================================== */
static size_t lcp_rndv_fin_pack(void *dest, void *data)
{
        lcp_rndv_ack_hdr_t *hdr = dest;
        lcp_request_t *req = data;

        hdr->msg_id = req->msg_id;
        hdr->src    = req->send.am.src;

        return sizeof(*hdr);
}

//NOTE: for offload rndv algorithm, everything is sent through the matching bit
//      and the immediate data so there is nothing to pack.
//TODO: add send_tag_short() API call 
static size_t lcp_rts_offload_pack(void *dest, void *data) 
{
        UNUSED(dest);
        UNUSED(data);
        return 0;
}

static size_t lcp_rtr_rput_offload_pack(void *dest, void *data)
{
        lcp_rndv_ack_hdr_t *hdr = dest;
        lcp_request_t *req = data;
        lcp_request_t *super = req->super;
        int packed_size;
        void *rkey_ptr;

        hdr->msg_id = super->msg_id;
        hdr->src    = super->send.am.src;

        rkey_ptr = hdr + 1;
        packed_size = lcp_mem_rkey_pack(super->ctx, super->state.lmem, 
                                        rkey_ptr);

        return sizeof(*hdr) + packed_size;
}

static size_t lcp_send_tag_tag_pack(void *dest, void *data)
{
        lcp_request_t *req = data;

        memcpy(dest, req->send.buffer, req->send.length);

        return req->send.length;
}

/* ============================================== */
/* Completion                                     */
/* ============================================== */

void lcp_request_complete_tag_offload(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, state.comp);

        lcp_request_complete(req);
}

/* ============================================== */
/* Rendez-vous                                    */
/* ============================================== */
void lcp_request_complete_recv_rget_offload(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        req->state.remaining -= comp->sent;
        req->state.offset    += comp->sent;
        if (req->state.remaining == 0) {
                lcp_request_complete(req);
        }
}

void lcp_request_complete_send_rget_offload(lcr_completion_t *comp)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              send.t_ctx.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                rc = lcp_mem_unpost(req->ctx, req->state.lmem, 
                                    req->tm.imm);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP OFFLOAD: could "
                                               "not unpost");
                        return;
                }
                lcp_request_complete(req);
        }
}

int lcp_send_rget_offload_start(lcp_request_t *req)
{
        int rc;
        lcp_ep_h ep = req->send.ep;
        bmap_t bitmap = { 0 };
        struct iovec iov_send[1];
        lcr_rail_attr_t attr;
        //FIXME: change priority_chnl to tag_chnl
        sctk_rail_info_t *rail = ep->lct_eps[ep->priority_chnl]->rail;
        size_t iovcnt = 0;

        rail->iface_get_attr(rail, &attr);
        build_memory_bitmap(req->send.length, attr.iface.cap.rndv.min_frag_size,
                            req->ctx->num_resources, &bitmap);
        /* Set immediate and tag for matching */
        LCP_TM_SET_HDR_DATA(req->tm.imm.t, 0, LCP_PROTOCOL_RGET, bitmap.bits[0], //FIXME: change getter
                            req->send.length, req->msg_id);

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->send.rndv.comm_id);

        mpc_common_debug_info("LCP: send rndv get off req=%p, comm_id=%lu, "
                              "tag=%d, src=%d, dest=%d, muid=%d, buf=%p.", req, 
                              req->send.rndv.comm_id, 
                              req->send.rndv.tag, req->send.rndv.src, 
                              req->send.rndv.dest, req->msg_id, 
                              req->send.buffer);

        req->send.t_ctx.req          = req;
        req->send.t_ctx.comp.comp_cb = lcp_request_complete_send_rget_offload;
        req->state.comp_stg          = 0;

        unsigned int mem_flags = LCR_IFACE_TM_PERSISTANT_MEM;
        rc = lcp_mem_post(req->ctx, &req->state.lmem, req->send.buffer,
                          req->send.length, (lcr_tag_t)req->tm.imm, mem_flags, 
                          &(req->send.t_ctx));

        /* Post FIN before sending RGET */
        //TODO: add rts data in case message is unexpected
        iov_send[0].iov_base = NULL;
        iov_send[0].iov_len  = 0; 
        iovcnt++;

        /* Finally, post send RGET */
        rc = lcp_send_do_tag_bcopy(ep->lct_eps[ep->priority_chnl],
                                   tag, req->tm.imm.t,
                                   lcp_rts_offload_pack, req);

        return rc;
}

/* Rendez-vous put callback for completion */
void lcp_request_complete_rput_offload(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_ep_h ep;
                ssize_t payload;
                ep = req->send.ep;

                payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                               MPC_LOWCOMM_RFIN_TM_MESSAGE,
                                               lcp_rndv_fin_pack,
                                               req);
                if (payload < 0) {
                        mpc_common_debug_error("LCP OFFLOAD: could not "
                                               "send fin message");
                        return;
                }
                lcp_request_complete(req);
        }
        return;
}

int lcp_send_rput_offload_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        size_t iovcnt = 0;
        uint64_t imm  = 0;
        struct iovec iov_send[1];
        bmap_t bitmap;
        lcp_ep_h ep = req->send.ep;
        lcr_rail_attr_t attr;
        //FIXME: change priority_chnl to tag_chnl
        sctk_rail_info_t *rail = ep->lct_eps[ep->priority_chnl]->rail;

        mpc_common_debug_info("LCP: send rndv put off req=%p, comm_id=%lu, "
                              "tag=%d, src=%d, dest=%d, msg_id=%d, buf=%p.", 
                              req, req->send.rndv.comm_id, 
                              req->send.rndv.tag, req->send.rndv.src, 
                              req->send.rndv.dest, req->msg_id,
                              req->send.buffer);

        rail->iface_get_attr(rail, &attr);
        build_memory_bitmap(req->send.length, attr.iface.cap.rndv.min_frag_size,
                            req->ctx->num_resources, &bitmap);

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, 
                             req->send.rndv.src, 
                             req->send.rndv.tag, 
                             req->send.rndv.comm_id);

        /* Inside immediate data (header), set RPUT protocol and length of
         * request */
        LCP_TM_SET_HDR_DATA(imm, 0, LCP_PROTOCOL_RPUT, bitmap.bits[0], //FIXME: change getter
                            req->send.length, req->msg_id);

        /* Add request to match list for later rtr */
        if (lcp_pending_create(req->ctx->match_ht, req, req->msg_id) == NULL) {
                mpc_common_debug_error("LCP OFFLOAD: could not add pending "
                                       "request");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        req->flags |= LCP_REQUEST_OFFLOADED_RNDV;

        /* No data to be sent for RPUT */
        iov_send[0].iov_base = NULL;
        iov_send[0].iov_len  = 0;
        iovcnt++;

        req->state.comp.comp_cb = lcp_request_complete_rput_offload;

        /* Finally, send message rendez-vous request */
        rc = lcp_send_do_tag_bcopy(ep->lct_eps[ep->priority_chnl],
                                   tag, imm, lcp_rts_offload_pack, req);

        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
err:
        return rc;

}

int lcp_send_rndv_offload_start(lcp_request_t *req)
{
        int muid;
        lcp_ep_h ep = req->send.ep;

        req->state.offloaded = 1;
        
        /* Increment offload id and overwrite msg id */
        muid = OPA_fetch_and_incr_int(&req->ctx->muid);
        req->msg_id = muid;

        switch(ep->ctx->config.rndv_mode) {
        case LCP_RNDV_GET:
                req->send.func = lcp_send_rget_offload_start;
                break;
        case LCP_RNDV_PUT:
                req->send.func = lcp_send_rput_offload_start;
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown protocol");
                break;
        } 	

        return MPC_LOWCOMM_SUCCESS;
}

/* ============================================== */
/* Send                                           */
/* ============================================== */

int lcp_send_tag_eager_tag_bcopy(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t payload;
        lcp_ep_h ep = req->send.ep;
        _mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->state.cc];

        lcr_tag_t tag = {.t_tag = {
                .tag = req->send.tag.tag,
                .src = req->send.tag.src,
                .comm = req->send.tag.comm_id }
        };

        uint64_t imm = 0;
        LCP_TM_SET_HDR_DATA(imm, 0, LCP_PROTOCOL_EAGER, 
                            0, req->send.length, 0);

        payload = lcp_send_do_tag_bcopy(lcr_ep, tag, imm, 
                                        lcp_send_tag_tag_pack, 
                                        req);

	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
	}
        return rc;
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
        LCP_TM_SET_HDR_DATA(imm, 0, LCP_PROTOCOL_EAGER, 0,
                            req->send.length, 0);

        req->state.comp.comp_cb =  lcp_request_complete_tag_offload;

        mpc_common_debug_info("LCP: post send tag zcopy req=%p, src=%d, dest=%d, "
                              "size=%d, matching=[%d:%d:%d]", req, req->send.tag.src, 
                              req->send.tag.dest, req->send.length, tag.t_tag.tag, 
                              tag.t_tag.src, tag.t_tag.comm);
        return lcp_send_do_tag_zcopy(lcr_ep, tag, imm, &iov, 1, &(req->state.comp));
}


/* ============================================== */
/* Recv                                           */
/* ============================================== */

int lcp_recv_tag_rget(lcp_request_t *req);
int lcp_recv_tag_rput(lcp_request_t *req);

void lcp_recv_tag_callback(lcr_completion_t *comp) 
{
        uint8_t op;
        uint64_t comm_id, gid, imm;
        int32_t src;
        mpc_lowcomm_communicator_t comm;
        lcr_tag_t tag;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              recv.t_ctx.comp);

        //FIXME: to be refactor
        imm = req->recv.t_ctx.imm;
        tag = req->recv.t_ctx.tag;

        op                    = LCP_TM_GET_HDR_OP(imm);
        req->recv.send_length = LCP_TM_GET_HDR_LENGTH(imm); 
        req->msg_id           = LCP_TM_GET_HDR_UID(imm);

        src                   = LCP_TM_GET_SRC(tag.t);

        //FIXME: bad way to get back comm id
        gid = mpc_lowcomm_monitor_get_gid();
        comm_id = LCP_TM_GET_COMM(tag.t);
        comm_id |= gid << 32;

        mpc_common_debug_info("LCP: tag callback req=%p, src=%d, size=%d, "
                              "matching=[%d:%d:%d], comm id=%llu", req, src, 
                              req->recv.send_length, tag.t_tag.tag, 
                              tag.t_tag.src, tag.t_tag.comm, comm_id);

        comm = mpc_lowcomm_get_communicator_from_id(comm_id);
        req->recv.tag.src = mpc_lowcomm_communicator_uid(comm, src);
        req->recv.tag.tag = LCP_TM_GET_TAG(tag.t);

        switch(op) {
        case LCP_PROTOCOL_EAGER:
                //FIXME: add if send_length != comp->sent
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
        case LCP_PROTOCOL_RGET:
                //FIXME: change getter
                //FIXME: create new send request to be able to use 
                //       lcp_request_send
                req->state.comp.comp_cb = lcp_request_complete_recv_rget_offload;
                req->state.remaining = req->recv.send_length;
                req->state.offset = 0;
                req->state.mem_map.bits[0] = LCP_TM_GET_HDR_BITMAP(imm);
                lcp_recv_tag_rget(req);
                break;
        case LCP_PROTOCOL_RPUT:
                lcp_recv_tag_rput(req);
                break;
        default:
                mpc_common_debug_error("LCP: unknown recv protocol.");
                break;
        }

        return;
}

//FIXME: according to MPI Standard, tag must be positive. However, MPC uses
//       negative for some collectives. For such p2p, there is no need for the
//       receiver to get back the tag, so no problem.
//       Some safeguard should be used to forbid tag > 2^23-1.
int lcp_recv_tag_zcopy(lcp_request_t *rreq, sctk_rail_info_t *iface)
{
        size_t iovcnt = 0;
        unsigned int flags = 0;
        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, rreq->recv.tag.src, 
                             rreq->recv.tag.tag, 
                             rreq->recv.tag.comm_id);

        lcr_tag_t ign_tag = {.t = 0 };
        if ((int)rreq->recv.tag.src == MPC_ANY_SOURCE) {
                ign_tag.t |= LCP_TM_SRC_MASK;
        }
        if ((int)rreq->recv.tag.tag == MPC_ANY_TAG) {
                ign_tag.t |= LCP_TM_TAG_MASK;
        }

        struct iovec iov = {
                .iov_base = rreq->recv.buffer,
                .iov_len  = rreq->recv.length
        };
        iovcnt++;

        rreq->recv.t_ctx.req          = rreq;
        rreq->recv.t_ctx.comp.comp_cb = lcp_recv_tag_callback;

        mpc_common_debug_info("LCP: post recv tag zcopy req=%p, src=%d, dest=%d, "
                              "size=%d, matching=[%d:%d:%d], ignore=[%d:%d:%d]", 
                              rreq, rreq->recv.tag.src, rreq->recv.tag.dest, 
                              rreq->recv.length, tag.t_tag.tag, tag.t_tag.src, 
                              tag.t_tag.comm, ign_tag.t_tag.tag, ign_tag.t_tag.src,
                              ign_tag.t_tag.comm);
        return lcp_post_do_tag_zcopy(iface, tag, ign_tag,
                                     &iov, iovcnt,
                                     flags, &(rreq->recv.t_ctx));

}

int lcp_recv_tag_rget(lcp_request_t *req) 
{
        int rc, i, num_used_ifaces = 0;
        size_t remaining, offset;
        size_t frag_length, length;
        size_t *per_ep_length;
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;
        lcr_rail_attr_t attr;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        mpc_common_debug_info("LCP: recv offload rget src=%d, tag=%d",
                              req->recv.tag.src, req->recv.tag.tag);

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

        /* Retrieve request state */
        remaining = req->state.remaining;
        offset    = req->state.offset;

        //FIXME: wrong load balance in case of NO_RESOURCE during fragmentation
        /* Compute number of interfaces used */
        for (i=0; i<req->ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(req->state.mem_map, i)) {
                        num_used_ifaces++;
                }
        }

        /* Compute length per endpoints that will be get */
        per_ep_length = sctk_malloc(num_used_ifaces * sizeof(size_t));
        for (i=0; i<num_used_ifaces; i++) {
                per_ep_length[i] = (size_t)remaining / num_used_ifaces;
        }
        per_ep_length[0] += remaining % num_used_ifaces;

        while (remaining > 0) {
                lcr_ep = ep->lct_eps[req->state.cc];
                lcr_ep->rail->iface_get_attr(lcr_ep->rail, &attr);

                frag_length = attr.iface.cap.rndv.max_get_zcopy;
                length = per_ep_length[req->state.cc] < frag_length ? 
                        per_ep_length[req->state.cc] : frag_length;

                rc = lcp_send_do_get_tag_zcopy(lcr_ep, 
                                               (lcr_tag_t)req->recv.t_ctx.imm,
                                               (uint64_t)req->recv.buffer+offset, 
                                               offset, length,
                                               &(req->state.comp));

                offset    += length; 
                remaining -= length;
                per_ep_length[req->state.cc] -= length;

                req->state.cc = (req->state.cc + 1) % num_used_ifaces;
        }

        sctk_free(per_ep_length);
err:
        return rc;
}

int lcp_recv_tag_rput(lcp_request_t *req)
{
        int rc, payload;
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;
        lcp_request_t *ack;

        mpc_common_debug_info("LCP: recv offload rput src=%d, tag=%d",
                              req->recv.tag.src, req->recv.tag.tag);
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

        /* Add request to match list for later rtr */
        if (lcp_pending_create(req->ctx->match_ht, req, req->msg_id) == NULL) {
                mpc_common_debug_error("LCP OFFLOAD: could not add pending "
                                       "request");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        req->flags |= LCP_REQUEST_OFFLOADED_RNDV;

        /* Register memory on which the put will be done */
        //NOTE: rput tag protocol is similar with am protocol. The only
        //      difference is that first message is matched using hw matching
        rc = lcp_mem_register(req->ctx, 
                              &(req->state.lmem), 
                              req->recv.buffer, 
                              req->recv.send_length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Prepare ack request to send back memory registration contexts */
        lcp_request_create(&ack);
        if (ack == NULL) {
                goto err;
        }
        ack->super = req;

        payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                       MPC_LOWCOMM_RTR_TM_MESSAGE,
                                       lcp_rtr_rput_offload_pack,
                                       ack);
        if (payload < 0) {
                mpc_common_debug_error("LCP: error sending ack rdv message");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        lcp_request_complete(ack);

        rc = MPC_LOWCOMM_SUCCESS;
err:
        return rc;
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */
static int lcp_rndv_tag_rtr_handler(void *arg, void *data,
                                    size_t length, 
                                    __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_rndv_ack_hdr_t *hdr = data;
        lcp_mem_h rmem = NULL;
        int muid = (int)hdr->msg_id;
        lcp_request_t *req;

        mpc_common_debug_info("LCP: recv tag ack off header src=%d, "
                              "msg_id=%llu.", hdr->src, muid);

        /* Retrieve request */
        if ((req = lcp_pending_get_request(ctx->match_ht, muid)) == NULL) {
                mpc_common_debug_error("LCP OFFLOAD: could not find request");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        rc = lcp_mem_unpack(req->ctx, &rmem,  hdr + 1, 
                            length - sizeof(lcp_rndv_ack_hdr_t));
        if (rc < 0) {
                goto err;
        }

        rc = lcp_send_rput_common(req, rmem);

err:
        return rc;
}

static int lcp_rndv_tag_fin_handler(void *arg, void *data,
                                    __UNUSED__ size_t length, 
                                    __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_rndv_ack_hdr_t *hdr = data;
        int muid = (int)hdr->msg_id;
        lcp_request_t *req;

        mpc_common_debug_info("LCP: recv rfin header src=%d, msg_id=%llu",
                              hdr->src, hdr->msg_id);

        /* Retrieve request */
        if ((req = lcp_pending_get_request(ctx->match_ht, muid)) == NULL) {
                mpc_common_debug_error("LCP OFFLOAD: could not find request");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        
        lcp_request_complete(req);
err:
        return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_RTR_TM_MESSAGE, lcp_rndv_tag_rtr_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RFIN_TM_MESSAGE, lcp_rndv_tag_fin_handler, 0);
