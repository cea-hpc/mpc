/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include "lcp_rndv.h"

#include "lcp_def.h"
#include "lcp_ep.h"
#include "lcp_prototypes.h"
#include "lcp_mem.h"
#include "lcp_context.h"
#include "lcp_datatype.h"
#include "lcp_header.h"
#include "lcp_request.h"

#include "mpc_common_debug.h"
#include "mpc_lowcomm_types.h"

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
size_t lcp_rndv_rts_pack(lcp_request_t *super, void *dest)
{
        lcp_request_t *rndv_req = super->rndv_req;
        lcp_rndv_hdr_t *hdr = (lcp_rndv_hdr_t *)dest;
        size_t packed_size = 0; 

        hdr->size   = rndv_req->send.length; 
        hdr->msg_id = rndv_req->msg_id; 

        if (super->ctx->config.rndv_mode == LCP_RNDV_GET) {
                /* Pack remote key from super request */
                packed_size = lcp_mem_rkey_pack(super->ctx, 
                                                super->state.lmem,
                                                hdr + 1);
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

        hdr->msg_id = rndv_req->msg_id;

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

        assert(attr.iface.cap.rndv.max_get_zcopy);
        assert(attr.iface.cap.rndv.max_put_zcopy);

        while (remaining > 0) {

                if (!MPC_BITMAP_GET(rkey->bm, cc)) {
                        cc = lcp_ep_get_next_cc(ep);
                        continue;
                }

                length = (remaining < frag_length) ? remaining : frag_length;

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


                mpc_common_debug("LCP: register send buffer. Conn map=%x", 
                                 req->send.ep->conn_map);
                /* Get source address */
                void *start = req->datatype & LCP_DATATYPE_CONTIGUOUS ?
                        req->send.buffer : req->state.pack_buf;

                req->state.lmem = lcp_pinning_mmu_pin(req->ctx, start, req->send.length,req->send.ep->conn_map);
                req->state.offset = (size_t)(start - req->state.lmem->base_addr);
                assume(req->state.offset == 0);

                return MPC_LOWCOMM_SUCCESS;

        }

        return rc;
}

int lcp_rndv_reg_recv_buffer(lcp_request_t *rndv_req)
{
        lcp_request_t *req = rndv_req->super;

        /* Get source address */
        void *start = req->datatype & LCP_DATATYPE_CONTIGUOUS ?
                req->recv.buffer : req->state.pack_buf;

        mpc_common_debug("LCP: register recv buffer. Conn map=%x", 
                         rndv_req->send.ep->conn_map);
        /* Register and pack memory pin context that will be sent to remote */
        req->state.lmem = lcp_pinning_mmu_pin(req->ctx, start, req->recv.send_length,
                                              rndv_req->send.ep->conn_map);
        req->state.offset = (size_t)(start - req->state.lmem->base_addr);
        assume(req->state.offset == 0);

        return MPC_LOWCOMM_SUCCESS;
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
                lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

                payload = lcp_send_do_am_bcopy(ep->lct_eps[cc],
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
        rndv_req = lcp_request_get(req->task);
        if (rndv_req == NULL) {
                mpc_common_debug_error("LCP: could not create request.");
                return LCP_ERROR;
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

        /* Set pointer to rndv request to be used when packing */
        //NOTE: on sender side, msg_id corresponds to local rndv_req and will be
        //      sent to receiver through hdr. This will be retransmitted by
        //      receiver in FIN message so that rndv_req can be found and
        //      completed.
        req->rndv_req    = rndv_req;
        rndv_req->msg_id = (uint64_t)rndv_req; 

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

        mpc_common_debug("LCP RNDV: progress rtr request=%p",
                         rndv_req);
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

//FIXME: should data be the actual rndv data which is the memory key or the rndv
//       header ? Some necessary info are in the tag/am part of the request.
//       Acceeding them here would break rndv modularity
int lcp_rndv_process_rts(lcp_request_t *rreq,
                         void *data, size_t length) 
{
        int rc; 
        lcp_request_t *rndv_req;
        lcp_rndv_hdr_t *hdr = data;

        /* Rendez-vous request used for RTR (PUT) or RMA (GET) */
        rndv_req = lcp_request_get(rreq->task);
        if (rndv_req == NULL) {
                mpc_common_debug_error("LCP: could not create request.");
                return LCP_ERROR;
        }
        rndv_req->super = rreq;

        rndv_req->ctx             = rreq->ctx;
        rndv_req->datatype        = rreq->datatype;
        rndv_req->send.buffer     = rreq->recv.buffer;
        rndv_req->state.remaining = rreq->recv.send_length;
        rndv_req->state.offset    = 0;

        /* Set message identifiers from incoming message */
        //NOTE: on receive side, msg_id is set to hdr->msg_id which correspond
        //      to the sender's rndv_req address.
        rndv_req->msg_id = hdr->msg_id;

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
        if (!(rndv_req->send.ep = lcp_ep_get(rndv_req->ctx, hdr->src_uid))) {
                rc = lcp_ep_create(rndv_req->ctx, &(rndv_req->send.ep), 
                                   hdr->src_uid, 0);
                if (rc != LCP_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep "
                                               "after match.");
                        goto err;
                }
        } 

        switch (rreq->ctx->config.rndv_mode) {
        case LCP_RNDV_PUT:
                /* Register memory through rndv request since we need the
                 * endpoint connection map stored in the endpoint */ 
                mpc_common_debug("LCP RNDV: process rts put request=%p",
                                 rndv_req);
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

                mpc_common_debug("LCP RNDV: process rts get request=%p", rndv_req);
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
        rndv_req = (lcp_request_t *)hdr->msg_id;
        //FIXME: add a flags to specify when a request is valid since the not
        //       null test may not be enough
        assume(rndv_req != NULL);

        rc = lcp_mem_unpack(ctx, &(rndv_req->send.rndv.rkey), hdr + 1, 
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
        rndv_req = (lcp_request_t *)hdr->msg_id;
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
        lcp_pinning_mmu_unpin(req->ctx, req->state.lmem);

        /* Call completion of super request */
        req->state.comp.comp_cb(&(req->state.comp));

        lcp_request_complete(rndv_req);

        return rc;
}

LCP_DEFINE_AM(LCP_AM_ID_RTR, lcp_rndv_rtr_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_RFIN, lcp_rndv_fin_handler, 0);
