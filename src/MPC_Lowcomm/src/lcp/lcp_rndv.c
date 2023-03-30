#include "lcp_rndv.h"

#include "lcp_ep.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"
#include "lcp_context.h"
#include "lcp_task.h"

#include "sctk_alloc.h"
#include "sctk_net_tools.h"

//FIXME: function also needed in lcp_mem.c
static inline void build_memory_bitmap(size_t length, 
                                       size_t min_frag_size, 
                                       int max_iface,
                                       int start_idx,
                                       bmap_t *bmap_p)
{
        bmap_t bmap = MPC_BITMAP_INIT;
        int num_used_ifaces = 0;

        while ((length > num_used_ifaces * min_frag_size) &&
               (num_used_ifaces < max_iface)) {
                MPC_BITMAP_SET(bmap, start_idx);
                start_idx = (start_idx + 1) % max_iface;
                num_used_ifaces++;
        }

        *bmap_p = bmap;
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
        packed_size = lcp_mem_rkey_pack(req->ctx, req->state.lmem,
                                        rkey_buf);
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

        memcpy(hdr + 1, &req->state.lmem->bm, sizeof(bmap_t));

        return sizeof(*hdr) + sizeof(bmap_t);
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
        packed_size = lcp_mem_rkey_pack(super->ctx, super->state.lmem, 
                                        rkey_ptr);

        return sizeof(*hdr) + packed_size;
}

/* ============================================== */
/* Rendez-vous send completion callback           */
/* ============================================== */

int lcp_send_rput_common(lcp_request_t *super, lcp_mem_h rmem)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i, last_iface = 0, num_used_ifaces = 0;
        size_t remaining, offset;
        size_t frag_length, length;
        size_t *per_ep_length;
        lcp_ep_h ep;
        lcr_rail_attr_t attr;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        /* Count number of iface to use */
        for (i=0; i<super->ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(rmem->bm, i)) {
                        num_used_ifaces++;
                }
        }
        /* Set the length to be sent on each interface. The minimum size that
         * can be sent on a interface is defined by min_frag_size */
        per_ep_length = sctk_malloc(super->ctx->num_resources * sizeof(size_t));
        for (i=0; i<super->ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(rmem->bm, i)) {
                        per_ep_length[i] = (size_t)super->send.length / 
                                num_used_ifaces;
                        last_iface = i;
                }
        }
        per_ep_length[last_iface] += super->send.length % num_used_ifaces;

        super->state.comp_stg = 1;
        remaining   = super->send.length;
        offset      = 0;
        ep          = super->send.ep;

        while (remaining > 0) {
                if (!MPC_BITMAP_GET(rmem->bm, super->state.cc)) {
                        super->state.cc = (super->state.cc + 1) % 
                                super->ctx->num_resources;
                        continue;
                }
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
                                           &(super->state.comp));

                offset    += length; remaining -= length;
                per_ep_length[super->state.cc] -= length;

                super->state.cc = (super->state.cc + 1) % 
                        super->ctx->num_resources;
        }

        return rc;
}

void lcp_request_complete_am_rput(lcr_completion_t *comp)
{
        int payload;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_ep_h ep = req->send.ep;

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


/* ============================================== */
/* Protocol starts                                */
/* ============================================== */
int lcp_send_am_rget_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int payload_size;
        lcr_rail_attr_t attr;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = req->state.cc;
        sctk_rail_info_t *iface = ep->lct_eps[cc]->rail;

        mpc_common_debug_info("LCP: send rndv get am req=%p, comm_id=%lu, tag=%d, "
                              "src=%d, dest=%d, msg_id=%d, buf=%p.", req, 
                              req->send.rndv.comm_id, 
                              req->send.rndv.tag, req->send.rndv.src, 
                              req->send.rndv.dest, req->msg_id,
                              req->send.buffer);

        /* memory registration */
        lcp_mem_create(req->ctx, &(req->state.lmem));

        iface->iface_get_attr(iface, &attr);
        build_memory_bitmap(req->send.length, attr.iface.cap.rndv.min_frag_size,
                            req->ctx->num_resources, cc, &(req->state.lmem->bm));

        /* Register and pack memory pin context that will be sent to remote */
        //FIXME: having the bitmap in the request state seems not useful. Could
        //       be only in the lcp_mem_h
        rc = lcp_mem_reg_from_map(req->send.ep->ctx, 
                                  req->state.lmem,
                                  req->state.lmem->bm,
                                  req->send.buffer, 
                                  req->send.length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc], 
                                            MPC_LOWCOMM_RGET_MESSAGE,
                                            lcp_rts_rget_pack, req);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
        }

        /* Do not complete request right now */
        //NOTE: no completion function for am rget since request is completed
        //      with fin handler.
err:
        return rc;
}

int lcp_send_am_rput_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int payload_size;
        lcr_rail_attr_t attr;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = req->state.cc;
        sctk_rail_info_t *iface = ep->lct_eps[cc]->rail;

        mpc_common_debug_info("LCP: send rndv put am req=%p, comm_id=%lu, tag=%d, "
                              "src=%d, dest=%d, msg_id=%d, buf=%p.", req, 
                              req->send.rndv.comm_id, 
                              req->send.rndv.tag, req->send.rndv.src, 
                              req->send.rndv.dest, req->msg_id,
                              req->send.buffer);

        /* Register and pack memory pin context that will be sent to remote */
        //NOTE: memory registration for put not necessary for portals.
        //      however, needed probably for other rma transports.
        //      Sender is responsible to choose which communication channel will
        //      be used. So the memory bitmap is packed and sent.
        /* memory registration */
        lcp_mem_create(req->ctx, &(req->state.lmem));

        iface->iface_get_attr(iface, &attr);
        build_memory_bitmap(req->send.length, attr.iface.cap.rndv.min_frag_size,
                            req->ctx->num_resources, cc, &(req->state.lmem->bm));

        /* Register and pack memory pin context that will be sent to remote */
        //FIXME: having the bitmap in the request state seems not useful. Could
        //       be only in the lcp_mem_h
        rc = lcp_mem_reg_from_map(req->send.ep->ctx, 
                                  req->state.lmem,
                                  req->state.lmem->bm,
                                  req->send.buffer, 
                                  req->send.length);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_request_complete_am_rput
        };

        payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc], 
                                            MPC_LOWCOMM_RPUT_MESSAGE,
                                            lcp_rts_rput_pack, req);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
        }

        /* Do not complete request right now */
err:
        return rc;
}

int lcp_send_rndv_am_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep = req->send.ep;

        if (lcp_pending_create(req->ctx->pend, req, req->msg_id) == NULL) {
                mpc_common_debug_error("LCP: could not add pending message");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        /* delete from pending list on completion */
        req->flags |= LCP_REQUEST_DELETE_FROM_PENDING; 

        switch (ep->ctx->config.rndv_mode) {
        case LCP_RNDV_GET:
                req->send.func = lcp_send_am_rget_start;
                break;
        case LCP_RNDV_PUT:
                req->send.func = lcp_send_am_rput_start;
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown protocol");
                break;
        }

err:
        return rc;
}

void lcp_request_complete_am_rget(lcr_completion_t *comp);
int lcp_recv_am_rput(lcp_request_t *req);
int lcp_recv_am_rget(lcp_request_t *req);

int lcp_rndv_matched(lcp_request_t *rreq,
                     lcp_rndv_hdr_t *hdr,
                     size_t length,
                     lcp_rndv_mode_t rndv_mode) 
{
        int rc; 
        uint64_t gid, comm_id, src;
        mpc_lowcomm_communicator_t comm;

        //FIXME: ugly...
        gid = mpc_lowcomm_monitor_get_gid();
        comm_id = hdr->base.comm;
        comm_id |= gid << 32;

        /* Init all protocol data */
        comm = mpc_lowcomm_get_communicator_from_id(comm_id);
        src  = mpc_lowcomm_communicator_uid(comm, hdr->base.src);
        rreq->recv.tag.src      = src;
        rreq->recv.tag.src_task = hdr->base.src;
        rreq->recv.tag.tag      = hdr->base.tag;
        rreq->msg_id            = hdr->msg_id;
        rreq->recv.send_length  = hdr->size; 
        rreq->state.remaining   = hdr->size;
        rreq->state.offset      = 0;
        rreq->seqn              = hdr->base.seqn;

        switch (rndv_mode) {
        case LCP_RNDV_PUT:
                rreq->state.mem_map = *(bmap_t *)(hdr + 1);
                rc = lcp_recv_am_rput(rreq);
                break;
        case LCP_RNDV_GET:
                rreq->state.comp = (lcr_completion_t) {
                        .comp_cb = lcp_request_complete_am_rget
                };
                rc = lcp_mem_unpack(rreq->ctx, &rreq->state.rmem, 
                                    hdr + 1, length);
                if (rc < 0) {
                        goto err;
                }
                rc = lcp_recv_am_rget(rreq);
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown protocol in recv");
                rc = MPC_LOWCOMM_ERROR;
                break;
        }

err:
        return rc;
}

/* ============================================== */
/* Rendez-vous recv completion callback           */
/* ============================================== */
void lcp_request_complete_am_rget(lcr_completion_t *comp)
{
        int payload_size;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_ep_h ep;
                lcp_ep_ctx_t *ctx_ep;
                /* Get LCP endpoint */
                HASH_FIND(hh, req->ctx->ep_ht, &req->recv.tag.src, 
                          sizeof(uint64_t), ctx_ep);
                assert(ctx_ep);

                ep = ctx_ep->ep;

                payload_size = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                                    MPC_LOWCOMM_RFIN_MESSAGE,
                                                    lcp_rndv_fin_pack,
                                                    req);
                if (payload_size < 0) {
                        goto err;
                }

                lcp_request_complete(req);
        }
err:
        return;
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */
int lcp_recv_am_rget(lcp_request_t *req)
{
        int rc, i, last_iface = 0, num_used_ifaces = 0;
        size_t remaining, offset;
        size_t frag_length, length;
        size_t *per_ep_length;
        lcp_mem_h rmem = req->state.rmem; 
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;
        lcr_rail_attr_t attr;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        mpc_common_debug_info("LCP: recv offload rget src=%d, tag=%d",
                              req->recv.tag.src_task, req->recv.tag.tag);

        /* Get LCP endpoint if exists */
        //FIXME: do this twice, one here and one in rndv_matched
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

        /* Count number of iface to use */
        for (i=0; i<req->ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(rmem->bm, i)) {
                        num_used_ifaces++;
                }
        }
        /* Set the length to be sent on each interface. The minimum size that
         * can be sent on a interface is defined by min_frag_size */
        per_ep_length = sctk_malloc(req->ctx->num_resources * sizeof(size_t));
        for (i=0; i<req->ctx->num_resources; i++) {
                if (MPC_BITMAP_GET(rmem->bm, i)) {
                        per_ep_length[i] = (size_t)req->recv.send_length / 
                                num_used_ifaces;
                        last_iface = i;
                }
        }
        per_ep_length[last_iface] += req->send.length % num_used_ifaces;

        req->state.comp_stg = 0;
        req->state.remaining = remaining = req->recv.send_length;
        offset               = 0;

        while (remaining > 0) {
                if (!MPC_BITMAP_GET(rmem->bm, req->state.cc)) {
                        req->state.cc = (req->state.cc + 1) % 
                                req->ctx->num_resources;
                        continue;
                }
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
                                           &(req->state.comp));

                per_ep_length[req->state.cc] -= length;
                offset += length; remaining  -= length;

                req->state.cc = (req->state.cc + 1) % req->ctx->num_resources;
        }

        sctk_free(per_ep_length);
        lcp_mem_delete(rmem);
err:
        return rc;
}

int lcp_recv_am_rput(lcp_request_t *req)
{
        int rc, payload;
        lcp_ep_h ep;
        lcp_ep_ctx_t *ctx_ep;
        lcp_request_t *ack;

        mpc_common_debug_info("LCP: recv offload rput src=%d, tag=%d",
                              req->recv.tag.src_task, req->recv.tag.tag);
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

        /* Register memory on which the put will be done */
        lcp_mem_create(req->ctx, &(req->state.lmem));
        //FIXME: remote bitmap is sent as payload in rndv put message. It is
        //       unpacked and saved by caller.
        req->state.lmem->bm  = req->state.mem_map;

        /* Register and pack memory pin context that will be sent to remote */
        //FIXME: revise how the memory is registered by sender and receiver...
        rc = lcp_mem_reg_from_map(req->ctx, 
                                  req->state.lmem,
                                  req->state.lmem->bm,
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

        if (lcp_pending_create(req->ctx->pend, req, req->msg_id) == NULL) {
                mpc_common_debug_error("LCP: could not add pending message");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        /* delete from pending list on completion */
        req->flags |= LCP_REQUEST_DELETE_FROM_PENDING; 

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

        lcp_request_complete(ack);
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
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->base.dest, &task);  
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not find task with tid=%d",
                                       hdr->base.dest);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        LCP_TASK_LOCK(task);
        mpc_common_debug_info("LCP: recv rput header src=%d, msg_id=%llu",
                              hdr->base.src, hdr->msg_id);

        req = lcp_match_prq(task->prq_table, hdr->base.comm, 
                            hdr->base.tag, hdr->base.src);
        if (req != NULL) {
                mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%lu, " 
                                      "tag=%d, src=%lu.", req, req->recv.tag.comm_id, 
                                      req->recv.tag.tag, req->recv.tag.src_task);
                LCP_TASK_UNLOCK(task); //NOTE: unlock context to enable endpoint creation.
                rc = lcp_rndv_matched(req, hdr, length - sizeof(*hdr), LCP_RNDV_PUT);
        } else {
                lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
                                            LCP_RECV_CONTAINER_UNEXP_RPUT);
                lcp_append_umq(task->umq_table, (void *)ctnr, hdr->base.comm, 
                               hdr->base.tag, hdr->base.src);
                LCP_TASK_UNLOCK(task);
                rc = MPC_LOWCOMM_SUCCESS;
        }

err:
        return rc;
}

static int lcp_ack_am_tag_handler(void *arg, void *data,
                                  size_t size, 
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
        req = lcp_pending_get_request(ctx->pend, hdr->msg_id);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not find ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        rc = lcp_mem_unpack(req->ctx, &rmem,  hdr + 1, 
                            size - sizeof(lcp_rndv_ack_hdr_t));
        if (rc < 0) {
                goto err;
        }

        rc = lcp_send_rput_common(req, rmem);

        //FIXME: remote key should be stored in case of NO_RESOURCE
        lcp_mem_delete(rmem);
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
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->base.dest, &task);  
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not find task with tid=%d",
                                       hdr->base.dest);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        LCP_TASK_LOCK(task);
        mpc_common_debug_info("LCP: recv rget header src=%d, msg_id=%llu",
                              hdr->base.src, hdr->msg_id);

        req = lcp_match_prq(task->prq_table, hdr->base.comm, 
                            hdr->base.tag, hdr->base.src);
        if (req != NULL) {
                mpc_common_debug_info("LCP: matched rndv exp handler req=%p, comm_id=%llu, " 
                                      "tag=%d, src=%llu.", req, req->recv.tag.comm_id, 
                                      req->recv.tag.tag, req->recv.tag.src_task);
                LCP_TASK_UNLOCK(task); //NOTE: unlock context to enable endpoint creation.
                rc = lcp_rndv_matched(req, hdr, length - sizeof(lcp_rndv_hdr_t), 
                                      LCP_RNDV_GET);
        } else {
                lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
                                            LCP_RECV_CONTAINER_UNEXP_RGET);
                lcp_append_umq(task->umq_table, (void *)ctnr, hdr->base.comm, 
                               hdr->base.tag, hdr->base.src);
                LCP_TASK_UNLOCK(task);
                rc = MPC_LOWCOMM_SUCCESS;
        }

err:
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


        mpc_common_debug_info("LCP: recv rfin header src=%d, msg_id=%llu",
                              hdr->src, hdr->msg_id);
        /* Retrieve request */
        req = lcp_pending_get_request(ctx->pend, hdr->msg_id);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not find ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        lcp_mem_deregister(req->ctx, req->state.lmem);
        lcp_request_complete(req);

err:
        return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_ACK_RDV_MESSAGE, lcp_ack_am_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RGET_MESSAGE, lcp_rndv_am_rget_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RPUT_MESSAGE, lcp_rndv_am_rput_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RFIN_MESSAGE, lcp_rndv_am_fin_handler, 0);
