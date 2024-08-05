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

#include <rndv/lcp_rndv.h>
#include <lcp_def.h>
#include <core/lcp_ep.h>
#include <core/lcp_prototypes.h>
#include <core/lcp_mem.h>
#include <core/lcp_context.h>
#include <core/lcp_manager.h>
#include <core/lcp_header.h>
#include <core/lcp_request.h>

#include "mpc_common_compiler_def.h"
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
static ssize_t lcp_rndv_rtr_pack(void *dest, void *data)
{
        size_t packed_size = 0;
        lcp_ack_hdr_t *hdr = dest;
        lcp_request_t *rndv_req = data;

        hdr->msg_id = rndv_req->msg_id;
        hdr->remote_addr = (uint64_t)rndv_req->send.buffer;

        packed_size = lcp_mem_rkey_pack(rndv_req->mngr,
                                        rndv_req->state.lmem,
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
ssize_t lcp_rndv_rts_pack(lcp_request_t *super, void *dest)
{
        //NOTE: use msg_id to recover rndv_req
        lcp_request_t *rndv_req = (lcp_request_t *)super->msg_id;
        lcp_rndv_hdr_t *hdr = (lcp_rndv_hdr_t *)dest;
        size_t packed_size = 0;

        hdr->size        = rndv_req->send.length;
        hdr->msg_id      = rndv_req->msg_id;
        hdr->remote_addr = (uint64_t)rndv_req->send.buffer;

        //FIXME: store rndv_mode in endpoint.
        if (rndv_req->mngr->ctx->config.rndv_mode == LCP_RNDV_GET) {
                /* Pack remote key from super request */
                packed_size = lcp_mem_rkey_pack(rndv_req->mngr,
                                                rndv_req->state.lmem,
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
static ssize_t lcp_rndv_fin_pack(void *dest, void *data)
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
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_rndv_rma_progress(lcp_request_t *rndv_req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_chnl_idx_t cc;
        size_t remaining, offset;
        size_t frag_length, length;
        lcp_mem_h rkey;
        lcp_ep_h ep;
        lcr_rail_attr_t attr;

        rkey      = rndv_req->send.rndv.rkey;
        ep        = rndv_req->send.ep;
        remaining = rndv_req->state.remaining;
        offset    = rndv_req->state.offset;

        cc = ep->rma_chnl;

        ep->lct_eps[cc]->rail->iface_get_attr(ep->lct_eps[cc]->rail, &attr);
        //FIXME: to much indirection to get rndv config.
        frag_length = rndv_req->mngr->ctx->config.rndv_mode == LCP_RNDV_GET ?
                attr.iface.cap.rndv.max_get_zcopy :
                attr.iface.cap.rndv.max_put_zcopy;

        assert(attr.iface.cap.rndv.max_get_zcopy);
        assert(attr.iface.cap.rndv.max_put_zcopy);

        //NOTE: using a do while handles the 0 length case.
        do {
                if (!MPC_BITMAP_GET(rkey->bm, cc)) {
                        cc = lcp_ep_get_next_cc(ep, cc, ep->rma_bmap);
                        assert(cc != LCP_NULL_CHANNEL);
                        continue;
                }

                length = (remaining < frag_length) ? remaining : frag_length;

                if (rndv_req->mngr->ctx->config.rndv_mode == LCP_RNDV_GET) {
                        /* Get source address */
                        rc = lcp_send_do_get_zcopy(ep->lct_eps[cc],
                                                   mpc_buffer_offset(rndv_req->send.buffer, offset),
                                                   mpc_buffer_offset(rndv_req->send.rndv.addr, offset),
                                                   &(rndv_req->state.lmem->mems[cc]),
                                                   &(rkey->mems[cc]),
                                                   length,
                                                   &(rndv_req->state.comp));
                } else {
                        rc = lcp_send_do_put_zcopy(ep->lct_eps[cc],
                                                   mpc_buffer_offset(rndv_req->send.buffer, offset),
                                                   mpc_buffer_offset(rndv_req->send.rndv.addr, offset),
                                                   &(rndv_req->state.lmem->mems[cc]),
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
                cc = lcp_ep_get_next_cc(ep, cc, ep->rma_bmap);
        } while (remaining > 0);

        return rc;
}

/* ============================================== */
/* Memory registration                            */
/* ============================================== */
int lcp_rndv_register_buffer(lcp_request_t *rndv_req)
{
        int rc;

        rc = lcp_mem_create(rndv_req->mngr, &rndv_req->state.lmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mpc_common_debug("LCP RNDV: register buffer. rndv_req=%p",
                         rndv_req);
        /* Register memory. */
        rc = lcp_mem_reg_from_map(rndv_req->mngr, rndv_req->state.lmem,
                                  rndv_req->send.ep->rma_bmap,
                                  rndv_req->send.buffer,
                                  rndv_req->send.length,
                                  LCR_IFACE_REGISTER_MEM_DYN,
                                  &rndv_req->state.lmem->bm);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
        assert(mpc_bitmap_equal(rndv_req->send.ep->rma_bmap,
                                rndv_req->state.lmem->bm));

        assume(rndv_req->state.offset == 0);

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
                lcp_chnl_idx_t cc = ep->am_chnl;

                mpc_common_debug("LCP RNDV: send fin. uid=%llu, rndv req=%p",
                                 ep->uid, rndv_req);
                payload = lcp_send_do_am_bcopy(ep->lct_eps[cc],
                                               LCP_AM_ID_RFIN,
                                               lcp_rndv_fin_pack,
                                               rndv_req);
                if (payload < 0) {
                        mpc_common_debug_error("LCP RNDV: error sending fin message.");
                        return;
                }

                /* For both PUT and GET, when receiving a FIN message, we have to
                 * unregister the memory */
                payload = lcp_mem_deprovision(rndv_req->mngr, rndv_req->state.lmem);
                if (payload != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP RNDV: count not unpin memory.");
                        return;
                }

                if (rndv_req->flags & LCP_REQUEST_RELEASE_REMOTE_KEY) {
                        lcp_mem_delete(rndv_req->send.rndv.rkey);
                }

                /* Complete super request with specified completion function */
                super->state.comp.comp_cb(&(super->state.comp));

                /* Complete rndv request */
                lcp_request_complete(rndv_req, send.send_cb, MPC_LOWCOMM_SUCCESS,
                                     rndv_req->send.length);
        }
}

/* ============================================== */
/* Protocol starts                                */
/* ============================================== */

/**
 * @brief Start an rndv protocol with active message
 *
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS In case of success
 */
int lcp_send_rndv_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *rndv_req;

        /* Create rendez-vous request */
        rndv_req = lcp_request_get(req->task);
        if (rndv_req == NULL) {
                mpc_common_debug_error("LCP RNDV: could not create request.");
                return MPC_LOWCOMM_ERROR;
        }
        rndv_req->super = req;

        rndv_req->mngr            = req->mngr;
        rndv_req->datatype        = req->datatype;
        rndv_req->send.length     = req->send.length;
        rndv_req->send.buffer     = req->recv.buffer;
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
        //NOTE: set both super and rndv requests because it is needed during
        //      packing, see lcp_rndv_rts_pack.
        rndv_req->msg_id = req->msg_id = (uint64_t)rndv_req;

        /* Register memory if GET protocol */
        rc = lcp_rndv_register_buffer(rndv_req);

        return rc;
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */
static int lcp_rndv_progress_rtr(lcp_request_t *rndv_req)
{
        int rc = MPC_LOWCOMM_SUCCESS, payload;
        lcp_ep_h ep        = rndv_req->send.ep;
        lcp_chnl_idx_t cc  = ep->am_chnl;

        mpc_common_debug("LCP RNDV: progress rtr request=%p",
                         rndv_req);
        payload = lcp_send_do_am_bcopy(ep->lct_eps[cc],
                                       LCP_AM_ID_RTR,
                                       lcp_rndv_rtr_pack,
                                       rndv_req);
        if (payload < 0) {
                mpc_common_debug_error("LCP RNDV: error sending ack rdv message");
                rc = MPC_LOWCOMM_ERROR;
        }

        return rc;
}

//FIXME: should data be the actual rndv data which is the memory key or the rndv
//       header ? Some necessary info are in the tag/am part of the request.
//       Acceding them here would break rndv modularity
int lcp_rndv_process_rts(lcp_request_t *rreq,
                         void *data, size_t length)
{
        int rc;
        lcp_request_t *rndv_req;
        lcp_rndv_hdr_t *hdr = data;

        /* Rendez-vous request used for RTR (PUT) or RMA (GET) */
        rndv_req = lcp_request_get(rreq->task);
        if (rndv_req == NULL) {
                mpc_common_debug_error("LCP RNDV: could not create request.");
                return MPC_LOWCOMM_ERROR;
        }
        rndv_req->super  = rreq;

        rndv_req->mngr            = rreq->mngr;
        rndv_req->datatype        = rreq->datatype;
        rndv_req->send.length     = rreq->recv.send_length;

        assert(rreq->datatype & LCP_DATATYPE_CONTIGUOUS);
        rndv_req->send.buffer     = rreq->recv.buffer;
        rndv_req->state.remaining = rreq->recv.send_length;
        rndv_req->state.offset    = 0;

        /* Set message identifiers from incoming message */
        //NOTE: on receive side, msg_id is set to hdr->msg_id which corresponds
        //      to the sender's rndv_req address.
        rndv_req->msg_id         = hdr->msg_id;
        rndv_req->send.rndv.addr = hdr->remote_addr;

        /* Get endpoint */
        if (!(rndv_req->send.ep = lcp_ep_get(rndv_req->mngr, hdr->src_uid))) {
                rc = lcp_ep_create(rndv_req->mngr, &(rndv_req->send.ep),
                                   hdr->src_uid, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP RNDV: could not create ep "
                                               "after match.");
                        goto err;
                }
        }

        /* Register memory through rndv request. */
        rc = lcp_rndv_register_buffer(rndv_req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        switch (rreq->mngr->ctx->config.rndv_mode) {
        case LCP_RNDV_PUT:
                /* Set RTR function that will send packed rkey */
                rndv_req->send.func = lcp_rndv_progress_rtr;
                break;
        case LCP_RNDV_GET:
                /* For RGET, data is unpacked upon completion */
                rndv_req->state.comp = (lcr_completion_t) {
                        .comp_cb = lcp_rndv_complete
                };

                /* Unpack remote key */
                rc = lcp_mem_unpack(rreq->mngr, &(rndv_req->send.rndv.rkey),
                                    hdr + 1, length);
                if (rc < 0) {
                        goto err;
                }
                rndv_req->flags |= LCP_REQUEST_RELEASE_REMOTE_KEY;

                rndv_req->send.func = lcp_rndv_rma_progress;
                break;
        default:
                mpc_common_debug_fatal("LCP RNDV: unknown protocol in recv.");
                rc = MPC_LOWCOMM_ERROR;
                break;
        }

        //FIXME: add error check.
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
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_manager_h mngr = arg;
        lcp_ack_hdr_t *hdr = data;
        lcp_request_t *rndv_req;

        /* Retrieve request */
        rndv_req = (lcp_request_t *)hdr->msg_id;
        //FIXME: add a flags to specify when a request is valid since the not
        //       null test may not be enough
        assume(rndv_req != NULL);

        rc = lcp_mem_unpack(mngr, &(rndv_req->send.rndv.rkey), hdr + 1,
                            size - sizeof(lcp_ack_hdr_t));
        if (rc < 0) {
                goto err;
        }
        rndv_req->flags |= LCP_REQUEST_RELEASE_REMOTE_KEY;

        rndv_req->send.func = lcp_rndv_rma_progress;

        //FIXME: add error check.
        lcp_request_send(rndv_req);

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
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
static int lcp_rndv_fin_handler(void *arg, void *data,
                                size_t length,
                                unsigned flags)
{
        UNUSED(length);
        UNUSED(flags);
        UNUSED(arg);
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_ack_hdr_t *hdr = data;
        lcp_request_t *rndv_req, *req;

        /* Retrieve request */
        rndv_req = (lcp_request_t *)hdr->msg_id;
        req = rndv_req->super;

        mpc_common_debug_info("LCP RNDV: recv rfin header msg_id=%llu, req=%p",
                              hdr->msg_id, rndv_req);

        /* For both PUT and GET, when receiving a FIN message, we have to
         * unregister the memory */
        rc = lcp_mem_deprovision(rndv_req->mngr, rndv_req->state.lmem);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Call completion of super request */
        req->state.comp.comp_cb(&(req->state.comp));

        lcp_request_complete(rndv_req, send.send_cb, MPC_LOWCOMM_SUCCESS,
                             rndv_req->send.length);

err:
        return rc;
}

LCP_DEFINE_AM(LCP_AM_ID_RTR, lcp_rndv_rtr_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_RFIN, lcp_rndv_fin_handler, 0);
