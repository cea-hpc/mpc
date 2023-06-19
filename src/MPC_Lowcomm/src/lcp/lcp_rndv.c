#include "lcp_rndv.h"

#include "lcp_ep.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"
#include "lcp_context.h"
#include "lcp_task.h"
#include "lcp_datatype.h"
#include "lcp_header.h"
#include "lcp_pending.h"
#include "lcp_request.h"

#include "sctk_alloc.h"
#include "msg_cpy.h"

/* ============================================== */
/* Packing                                        */
/* ============================================== */

/**
 * @brief Pack data for rget.
 * 
 * @param dest destination header
 * @param data request to pack
 * @return size_t size of packed data
 */
//FIXME: change return type to ssize_t
static size_t lcp_rndv_rtr_pack(void *dest, void *data)
{
        size_t packed_size = 0;
        lcp_ack_hdr_t *hdr = dest;
        lcp_request_t *rndv_req = data;
        lcp_request_t *super    = rndv_req->super;

        hdr->msg_id = rndv_req->msg_id;
        packed_size = lcp_mem_rkey_pack(super->ctx, 
                                        super->state.lmem,
                                        hdr + 1);

        return sizeof(*hdr) + packed_size;
}

/**
 * @brief Pack data for rput
 * 
 * @param dest destination header
 * @param data request to pack
 * @return size_t size of packed data
 */
//FIXME: change return type to ssize_t
size_t lcp_rndv_rts_pack(lcp_request_t *req, void *dest)
{
        size_t packed_size = 0; 

        if (req->ctx->config.rndv_mode == LCP_RNDV_GET) {
                packed_size = lcp_mem_rkey_pack(req->ctx, 
                                                req->state.lmem,
                                                dest);
        }

        return packed_size;
}

/**
 * @brief Pack data for rendez-vous finalization message
 * 
 * @param dest destination header
 * @param data request to pack
 * @return size_t size of packed data
 */
//FIXME: change return type to ssize_t
static size_t lcp_rndv_fin_pack(void *dest, void *data)
{
        lcp_ack_hdr_t *hdr = dest;
        lcp_request_t *rndv_req = data;

        hdr->msg_id = rndv_req->send.rndv.msg_id;

        return sizeof(*hdr);
}

/* ============================================== */
/* Rendez-vous RMA                                */
/* ============================================== */

/**
 * @brief Send a rma request.
 * 
 * @param super request to send
 * @param rmem // TODO ?
 * @return int LCP_SUCCESS in case of success
 */
int lcp_rndv_rma_progress(lcp_request_t *rndv_req)
{
        int rc = LCP_SUCCESS;
        lcp_chnl_idx_t cc;
        size_t remaining, offset;
        size_t frag_length, length;
        lcp_mem_h rkey;
        lcp_ep_h ep;
        lcr_rail_attr_t attr;
        uint64_t start;

        rkey      = rndv_req->send.rndv.rkey; 
        ep        = rndv_req->send.ep;
        remaining = rndv_req->state.remaining;
        offset    = rndv_req->state.offset;
        start     = rndv_req->datatype & LCP_DATATYPE_DERIVED ?
                (uint64_t)rndv_req->state.pack_buf : 
                (uint64_t)rndv_req->send.buffer;

        cc = lcp_ep_get_next_cc(ep);

        ep->lct_eps[cc]->rail->iface_get_attr(ep->lct_eps[cc]->rail, &attr);
        frag_length = rndv_req->ctx->config.rndv_mode == LCP_RNDV_GET ?
                attr.iface.cap.rndv.max_get_zcopy :
                attr.iface.cap.rndv.max_put_zcopy;
        
        while (remaining > 0) {
                if (!MPC_BITMAP_GET(rkey->bm, cc)) {
                        continue;
                }

                length = remaining < frag_length ? remaining : frag_length;

                //FIXME: error managment => NO_RESOURCE not handled
                if (rndv_req->ctx->config.rndv_mode == LCP_RNDV_GET) {
                        /* Get source address */
                        rc = lcp_send_do_get_zcopy(ep->lct_eps[cc],
                                                   start + offset, 
                                                   offset, 
                                                   &(rkey->mems[cc]),
                                                   length,
                                                   &(rndv_req->state.comp));
                } else {
                        rc = lcp_send_do_put_zcopy(ep->lct_eps[cc],
                                                   start + offset,
                                                   offset,
                                                   &(rkey->mems[cc]),
                                                   length,
                                                   &(rndv_req->state.comp));
                }
                
                if (rc == MPC_LOWCOMM_NO_RESOURCE) {
                        /* Save state */
                        rndv_req->state.remaining = remaining;
                        rndv_req->state.offset    = offset;
                        break;
                }

                offset += length; remaining -= length;
                cc = lcp_ep_get_next_cc(ep);
        }

        return rc;
}

/* ============================================== */
/* Memory registration                            */
/* ============================================== */
int lcp_rndv_reg_send_buffer(lcp_request_t *req)
{
        int rc = LCP_SUCCESS;

        if (req->ctx->config.rndv_mode == LCP_RNDV_GET) {
                rc = lcp_mem_create(req->ctx, &(req->state.lmem));
                if (rc != LCP_SUCCESS) {
                        goto err;
                }

                /* Get source address */
                void *start = req->datatype & LCP_DATATYPE_CONTIGUOUS ?
                        req->send.buffer : req->state.pack_buf;

                /* Register and pack memory pin context that will be sent to remote */
                req->state.lmem->bm = req->send.ep->conn_map;
                rc = lcp_mem_reg_from_map(req->ctx, 
                                          req->state.lmem,
                                          req->state.lmem->bm,
                                          start, 
                                          req->send.length);
        }
err:
        return rc;
}

int lcp_rndv_reg_recv_buffer(lcp_request_t *rndv_req)
{
        int rc;
        lcp_request_t *req = rndv_req->super;

        rc = lcp_mem_create(req->ctx, &(req->state.lmem));
        if (rc != LCP_SUCCESS) {
                goto err;
        }

        /* Get source address */
        void *start = req->datatype & LCP_DATATYPE_CONTIGUOUS ?
                req->recv.buffer : req->state.pack_buf;

        /* Register and pack memory pin context that will be sent to remote */
        req->state.lmem->bm = rndv_req->send.ep->conn_map;
        rc = lcp_mem_reg_from_map(req->ctx, 
                                  req->state.lmem,
                                  req->state.lmem->bm,
                                  start, 
                                  req->recv.send_length);
err:
        return rc;
}

/* ============================================== */
/* Rendez-vous completion callback                */
/* ============================================== */

void lcp_rndv_complete(lcr_completion_t *comp)
{
        int payload;
        lcp_request_t *rndv_req = mpc_container_of(comp, lcp_request_t, 
                                                   state.comp);

        rndv_req->state.remaining -= comp->sent;
        if (rndv_req->state.remaining == 0) {
                lcp_ep_h ep = rndv_req->send.ep;
                lcp_request_t *super = rndv_req->super;

                payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                               LCP_AM_ID_RFIN,
                                               lcp_rndv_fin_pack,
                                               rndv_req);
                if (payload < 0) {
                        mpc_common_debug_error("LCP: error sending fin message");
                        return;
                }

                /* Complete super request with specified completion function */
                super->state.comp.comp_cb(&(super->state.comp));

                /* Complete rndv request */
                lcp_request_complete(rndv_req);
        }
}

/* ============================================== */
/* Protocol starts                                */
/* ============================================== */

/**
 * @brief Start an rndv protocol with active message
 * 
 * @param req request
 * @return int LCP_SUCCESS In case of success
 */
int lcp_send_rndv_start(lcp_request_t *req)
{
        int rc = LCP_SUCCESS;
        lcp_request_t *rndv_req;

        /* If data is derived, buffer must be allocated and data packed to make
         * it contiguous and use zcopy and memory registration. */
        //FIXME: could be factorized. Code similar for send/recv and
        //       onload/offload datapaths.
        if (req->datatype & LCP_DATATYPE_DERIVED) {
                req->state.pack_buf = sctk_malloc(req->send.length);
                if (req->state.pack_buf == NULL) {
                        mpc_common_debug_error("LCP: could not allocate pack "
                                               "buffer");
                        rc = LCP_ERROR;
                        goto err;
                }

                lcp_datatype_pack(req->ctx, req, req->datatype,
                                  req->state.pack_buf, NULL, 
                                  req->send.length);
        }

        /* Create rendez-vous request */
        rc = lcp_request_create(&rndv_req);
        if (rc != LCP_SUCCESS) {
                goto err;
        }
        rndv_req->super = req;

        rndv_req->ctx             = req->ctx;
        rndv_req->datatype        = req->datatype;
        rndv_req->send.length     = req->send.length;
        rndv_req->send.buffer     = req->send.buffer;
        rndv_req->send.ep         = req->send.ep;
        rndv_req->state.remaining = req->send.length;
        rndv_req->state.offset    = 0;
        rndv_req->state.comp  = (lcr_completion_t) {
                .comp_cb = lcp_rndv_complete,
        };

        /* Set message identifiers */
        rndv_req->send.rndv.msg_id = req->msg_id; 

        if (lcp_pending_create(req->ctx->pend, rndv_req, req->msg_id) == NULL) {
                mpc_common_debug_error("LCP: could not add pending message");
                rc = LCP_ERROR;
                goto err;
        }

err:
        return rc;
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */
static int lcp_rndv_progress_rtr(lcp_request_t *rndv_req)
{
        int rc, payload;
        lcp_ep_h ep        = rndv_req->send.ep;
        lcp_chnl_idx_t cc  = lcp_ep_get_next_cc(ep);

        payload = lcp_send_do_am_bcopy(ep->lct_eps[cc],
                                       LCP_AM_ID_RTR,
                                       lcp_rndv_rtr_pack,
                                       rndv_req);
        if (payload < 0) {
                mpc_common_debug_error("LCP: error sending ack rdv message");
                rc = LCP_ERROR;
                goto err;
        }
        rc = LCP_SUCCESS;
err:
        return rc;
}

int lcp_rndv_process_rts(lcp_request_t *rreq,
                         void *data, size_t length) 
{
        int rc; 
        lcp_request_t *rndv_req;
        lcp_rndv_hdr_t *hdr = data;
        lcp_ep_ctx_t *ctx_ep;

        /* Rendez-vous request used for RTR (PUT) or RMA (GET) */
        rc = lcp_request_create(&rndv_req);
        if (rc != LCP_SUCCESS) {
                goto err;
        }
        rndv_req->super = rreq;

        rndv_req->ctx             = rreq->ctx;
        rndv_req->datatype        = rreq->datatype;
        rndv_req->send.buffer     = rreq->recv.buffer;
        rndv_req->state.remaining = rreq->recv.send_length;
        rndv_req->state.offset    = 0;

        /* Set message identifiers from incomming message */
        rndv_req->send.rndv.msg_id = hdr->msg_id;

        /* Buffer must be allocated and data packed to make it contiguous
         * and use zcopy and memory registration. */
        if (rreq->datatype & LCP_DATATYPE_DERIVED) {
                rreq->state.pack_buf = sctk_malloc(rreq->recv.send_length);
                if (rreq->state.pack_buf == NULL) {
                        mpc_common_debug_error("LCP: could not allocate pack "
                                               "buffer");
                        return LCP_ERROR;
                }
                rndv_req->state.pack_buf = rreq->state.pack_buf;
        }

        /* Get endpoint */
        HASH_FIND(hh, rndv_req->ctx->ep_ht, &hdr->src_uid, 
                  sizeof(uint64_t), ctx_ep);
        if (ctx_ep == NULL) {
                rc = lcp_ep_create(rndv_req->ctx, &(rndv_req->send.ep), 
                                   hdr->src_uid, 0);
                if (rc != LCP_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep "
                                               "after match.");
                        goto err;
                }
        } else {
                rndv_req->send.ep = ctx_ep->ep;
        }

        switch (rreq->ctx->config.rndv_mode) {
        case LCP_RNDV_PUT:
                not_implemented();
                /* Append request to request hash table */
                if (lcp_pending_create(rreq->ctx->pend, rndv_req, 
                                       rreq->msg_id) == NULL) {
                        mpc_common_debug_error("LCP: could not add pending "
                                               "message");
                        rc = LCP_ERROR;
                        goto err;
                }

                /* Register memory through rndv request since we need the
                 * endpoint connection map stored in the endpoint */ 
                rc = lcp_rndv_reg_recv_buffer(rndv_req);
                if (rc != LCP_SUCCESS) {
                        goto err;
                }

                /* Set RTR function that will send packed rkey */
                rndv_req->send.func = lcp_rndv_progress_rtr;
                break;
        case LCP_RNDV_GET:
                /* For RGET, data is unpacked upon completion */
                rndv_req->state.comp = (lcr_completion_t) {
                        .comp_cb = lcp_rndv_complete
                };

                /* Unpack remote key */
                rc = lcp_mem_unpack(rreq->ctx, &(rndv_req->send.rndv.rkey), 
                                    hdr + 1, length);
                if (rc < 0) {
                        goto err;
                }

                rndv_req->send.func = lcp_rndv_rma_progress;
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown protocol in recv");
                rc = LCP_ERROR;
                break;
        }

        lcp_request_send(rndv_req);
err:
        return rc;
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */

/* RTR handler will be run for PUT protocol only */
static int lcp_rndv_rtr_handler(void *arg, void *data,
                                size_t size, 
                                unsigned flags)
{
        UNUSED(flags);
        int rc = LCP_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_ack_hdr_t *hdr = data;
        lcp_request_t *rndv_req;

        /* Retrieve request */
        rndv_req = lcp_pending_get_request(ctx->pend, hdr->msg_id);
        if (rndv_req == NULL) {
                mpc_common_debug_error("LCP: could not find ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
                rc = LCP_ERROR;
                goto err;
        }

        rc = lcp_mem_unpack(rndv_req->ctx, &(rndv_req->send.rndv.rkey), hdr + 1, 
                            size - sizeof(lcp_ack_hdr_t));
        if (rc < 0) {
                goto err;
        }

        rndv_req->send.func = lcp_rndv_rma_progress;

        rc = lcp_request_send(rndv_req);

err:
        return rc;
}

/**
 * @brief Callback for rendez-vous finalization.
 * 
 * @param arg lcp context
 * @param data rendez-vous header
 * @param length header length
 * @param flags message flags
 * @return int LCP_SUCCESS in case of success
 */
static int lcp_rndv_fin_handler(void *arg, void *data,
                                size_t length, 
                                unsigned flags)
{
        UNUSED(length);
        UNUSED(flags);
        int rc = LCP_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_ack_hdr_t *hdr = data;
        lcp_request_t *rndv_req, *req;

        /* Retrieve request */
        rndv_req = lcp_pending_get_request(ctx->pend, hdr->msg_id);
        if (rndv_req == NULL) {
                mpc_common_debug_error("LCP: could not find ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
                rc = LCP_ERROR;
                goto err;
        }
        req = rndv_req->super;

        mpc_common_debug_info("LCP: recv rfin header msg_id=%llu, req=%p",
                              hdr->msg_id, rndv_req);

        if (req->datatype & LCP_DATATYPE_DERIVED) {
                lcp_datatype_unpack(req->ctx, req, req->datatype,
                                    NULL, req->state.pack_buf, 
                                    req->state.lmem->length);
                /* Free buffer allocated by rndv */
                sctk_free(req->state.pack_buf);
        }

        /* For both PUT and GET, when receiving a FIN message, we have to
         * unregister the memory */
        lcp_mem_deregister(req->ctx, req->state.lmem);

        //NOTE: rndv request must be completed before the super request so that
        //      it is removed from pending table before another task could
        //      allocate a lcp_request_t at that address.
        lcp_pending_delete(req->ctx->pend, req->msg_id);
        lcp_request_complete(rndv_req);

        /* Call completion of super request */
        req->state.comp.comp_cb(&(req->state.comp));

	LCP_CONTEXT_UNLOCK(ctx);
err:
        return rc;
}

LCP_DEFINE_AM(LCP_AM_ID_RTR, lcp_rndv_rtr_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_RFIN, lcp_rndv_fin_handler, 0);
