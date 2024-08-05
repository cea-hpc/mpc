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

#include <lcp.h>
#include <core/lcp_prototypes.h>

#include <core/lcp_ep.h>
#include <core/lcp_header.h>
#include <core/lcp_request.h>
#include <core/lcp_context.h>
#include <core/lcp_manager.h>
#include <core/lcp_mem.h>
#include <core/lcp_datatype.h>
#include <tag/lcp_pending.h>

#include "mpc_common_debug.h"
#include "mpc_common_compiler_def.h"

/* ============================================== */
/* Offload utils                                  */
/* ============================================== */
/* lcp_tag_t: for matching
 *
 * 64                                              0
 * <-----------------><-----------------><--------->
 *       src (24)             tag (24)     comm (16)
 */

#define LCP_TM_COMM_MASK 0x000000000000ffffull
#define LCP_TM_TAG_MASK  0x000000ffffff0000ull
#define LCP_TM_SRC_MASK  0xffffff0000000000ull

#define LCP_TM_SET_MATCHBITS(_matchbits, _src, _tag, _comm) \
        _matchbits |= (_src & 0xffffffull); \
        _matchbits  = (_matchbits << 24); \
        _matchbits |= (_tag & 0xffffffull); \
        _matchbits  = (_matchbits << 16); \
        _matchbits |= (_comm & 0xffffull);

#define LCP_TM_GET_SRC(_matchbits) \
        ((int)((_matchbits & LCP_TM_SRC_MASK) >> 40))
#define LCP_TM_GET_TAG(_matchbits) \
        ((int)((_matchbits & LCP_TM_TAG_MASK) >> 16))
#define LCP_TM_GET_COMM(_matchbits) \
        ((uint16_t)(_matchbits & LCP_TM_COMM_MASK))

/* immediate data

 * 64 63   60   56                          16        0
 * <--><---><---><--------------------------><-------->
 * sync  op  bitmap        size                 uid
 */
#define LCP_TM_HDR_SYNC_MASK   0x8000000000000000ull
#define LCP_TM_HDR_OP_MASK     0x7000000000000000ull
#define LCP_TM_HDR_LENGTH_MASK 0x0ffffffffff00000ull
#define LCP_TM_HDR_UID_MASK    0x00000000000fffffull

#define LCP_TM_SET_HDR_DATA(_hdr, _sync, _op, _length, _uid) \
        _hdr |= (_sync ? 0 : 1); \
        _hdr  = (_hdr << 3); \
        _hdr |= (_op & 0x7ull); \
        _hdr  = (_hdr << 40); \
        _hdr |= (_length & 0x000000ffffffffffull); \
        _hdr  = (_hdr << 20); \
        _hdr |= (_uid & 0x00000000000fffffull);

#define LCP_TM_GET_HDR_SYNC(_hdr) \
        ((int)((_hdr & LCP_TM_HDR_SYNC_MASK) >> 63))
#define LCP_TM_GET_HDR_OP(_hdr) \
        ((uint8_t)((_hdr & LCP_TM_HDR_OP_MASK) >> 60))
#define LCP_TM_GET_HDR_LENGTH(_hdr) \
        ((size_t)((_hdr & LCP_TM_HDR_LENGTH_MASK) >> 20))
#define LCP_TM_GET_HDR_UID(_hdr) \
        ((size_t)(_hdr & LCP_TM_HDR_UID_MASK))

#define LCP_OFFLOAD_MUID_SRC_MASK 0xffffffff00000000ull
#define LCP_OFFLOAD_MUID_MID_MASK 0x00000000ffffffffull

#define LCP_OFFLOAD_SET_MUID(_msg_id, _src, _mid) \
        _msg_id |= (_src & 0xffffffffull); \
        _msg_id  = (_msg_id << 32);        \
        _msg_id |= (_mid & 0xffffffffull);

#define LCP_OFFLOAD_GET_MUID_SRC(_muid) \
        ((int)((_hdr & LCP_OFFLOAD_MUID_SRC_MASK) >> 32))
#define LCP_OFFLOAD_GET_MUID_MID(_muid) \
        ((int)((_hdr & LCP_OFFLOAD_GET_MUID_MID)))

enum {
        LCP_PROTOCOL_EAGER = 0,
        LCP_PROTOCOL_RNDV,
};

static inline uint64_t lcp_get_process_uid(uint64_t pid, int32_t tid) {
        pid &= 0xFFFFFFFF00000000ull; /* first reset least significant bits to 0 */
        return pid | tid;
}

/* ============================================== */
/* Packing                                        */
/* ============================================== */

//NOTE: for offload rndv algorithm, everything is sent through the matching bit
//      and the immediate data so there is nothing to pack.
//TODO: add send_tag_short() API call
static ssize_t lcp_rts_offload_dummy_pack(void *dest, void *data)
{
        UNUSED(dest);
        UNUSED(data);
        return 0;
}

/**
 * @brief Pack data for rput (same as lcp_rtr_rput_pack)
 *
 * @param dest destination header
 * @param data source request
 * @return size_t size of packed data
 */
static ssize_t lcp_send_tag_offload_pack(void *dest, void *data)
{
        ssize_t packed_length;
        lcp_request_t *req = data;
        void *src = req->datatype == LCP_DATATYPE_CONTIGUOUS ?
               req->send.buffer : NULL;

        packed_length = lcp_datatype_pack(req->mngr->ctx, req, req->datatype,
                                          dest, src, req->send.length);

        return packed_length;
}

/* ============================================== */
/* Rendez-vous RMA                                */
/* ============================================== */

/**
 * @brief Complete a tag request.
 *
 * @param comp completion
 */
int lcp_rndv_offload_rma_progress(lcp_request_t *rndv_req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_chnl_idx_t cc;
        size_t remaining, offset;
        size_t frag_length, length;
        lcp_ep_h ep;
        lcr_rail_attr_t attr;
        uint64_t start, muid = 0;

        /* Set up state variables */
        ep        = rndv_req->send.ep;
        remaining = rndv_req->state.remaining;
        offset    = rndv_req->state.offset;
        start     = (uint64_t)rndv_req->send.buffer;
        LCP_OFFLOAD_SET_MUID(muid, rndv_req->send.tag.src_tid, rndv_req->tm.mid);

        /* Get next communication channel on which to communicate */
        //FIXME: because ep->next_cc is set to ep->tag_chnl the following works.
        //       It also acts as a hack to increment ep->next_cc
        cc = ep->tag_chnl;

        /* Get maximum frag size attribute */
        //NOTE: this suppose that all rails involved in fragmentation are
        //      homogeneous.
        ep->lct_eps[cc]->rail->iface_get_attr(ep->lct_eps[cc]->rail, &attr);
        frag_length = rndv_req->mngr->ctx->config.rndv_mode == LCP_RNDV_GET ?
                attr.iface.cap.rndv.max_get_zcopy :
                attr.iface.cap.rndv.max_put_zcopy;

        while (remaining > 0) {
                //FIXME: what bitmap to choose
                if (!MPC_BITMAP_GET(ep->tag_bmap, cc)) {
                        continue;
                }

                length = remaining < frag_length ? remaining : frag_length;

                if (rndv_req->mngr->ctx->config.rndv_mode == LCP_RNDV_GET) {
                        /* Get source address */
                        rc = lcp_send_do_get_tag_zcopy(ep->lct_eps[cc],
                                                       (lcr_tag_t)muid,
                                                       start + offset,
                                                       offset, length,
                                                       &(rndv_req->state.comp));
                } else {
                        //NOTE: put tag is actually just a send tag
                        not_implemented();
                }

                if (rc == MPC_LOWCOMM_NO_RESOURCE) {
                        /* Save state */
                        rndv_req->state.remaining = remaining;
                        rndv_req->state.offset    = offset;
                        break;
                }

                offset += length; remaining -= length;
                cc = lcp_ep_get_next_cc(ep, cc, ep->tag_bmap);
        }

        return rc;
}

/* ============================================== */
/* Memory Registration                            */
/* ============================================== */

int lcp_rndv_offload_reg_send_buffer(lcp_request_t *rndv_req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t muid = 0;
        lcp_request_t *req = rndv_req->super;

        if (req->mngr->ctx->config.rndv_mode == LCP_RNDV_GET) {
                rc = lcp_mem_create(req->mngr, &(req->state.lmem));
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                //FIXME: we use the tag_context of the rndv request but the local
                //       memory handle of the super request...
                LCP_OFFLOAD_SET_MUID(muid, req->send.tag.src_tid, req->tm.mid);
                //FIXME: modify lcp_mem_post_from_map signature to use const
                //       start
                rc = lcp_mem_post_from_map(req->mngr, req->state.lmem, rndv_req->send.ep->tag_bmap,
                                           (void*)req->send.buffer, req->send.length, (lcr_tag_t)muid,
                                           0, &(rndv_req->send.t_ctx));
        }
err:
        return rc;
}

/* ============================================== */
/* Completion callbacks eager tag                 */
/* ============================================== */

void lcp_tag_offload_send_complete(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, state.comp);

        lcp_request_complete(req, send.send_cb, req->status, req->send.length);
}

void lcp_tag_offload_recv_complete(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, state.comp);

        //FIXME: should it be the state length actually received ?
        req->recv.tag.info.length = req->recv.send_length;
        req->recv.tag.info.src    = req->recv.tag.src_tid;
        req->recv.tag.info.tag    = req->recv.tag.tag;

        lcp_request_complete(req, recv.tag.recv_cb, req->status,
                             &req->recv.tag.info);
}

/* ============================================== */
/* Completion callbacks rndv                      */
/* ============================================== */
void lcp_rndv_offload_send_complete(lcr_completion_t *comp)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *rndv_req = mpc_container_of(comp, lcp_request_t,
                                              send.t_ctx.comp);

        rndv_req->state.remaining -= comp->sent;
        if (rndv_req->state.remaining == 0) {
                lcp_request_t *super = rndv_req->super;

                if (super->mngr->ctx->config.rndv_mode == LCP_RNDV_GET) {
                        rc = lcp_mem_unpost(super->mngr, super->state.lmem,
                                            super->tm.imm);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                mpc_common_debug_error("LCP OFFLOAD: could "
                                                       "not unpost");
                                return;
                        }
                }

                /* Call super request completion callback */
                super->state.comp.comp_cb(&(super->state.comp));

                lcp_request_complete(rndv_req, send.send_cb, MPC_LOWCOMM_SUCCESS,
                                     rndv_req->send.length);
        }
}

void lcp_rndv_offload_recv_complete(lcr_completion_t *comp)
{
        lcp_request_t *rndv_req = mpc_container_of(comp, lcp_request_t,
                                                   state.comp);

        rndv_req->state.remaining -= comp->sent;
        if (rndv_req->state.remaining == 0) {
                lcp_request_t *super = rndv_req->super;

                /* Call super request completion callback */
                super->state.comp.comp_cb(&(super->state.comp));

                lcp_request_complete(rndv_req, send.send_cb, MPC_LOWCOMM_SUCCESS,
                                     rndv_req->send.length);
        }
}

/* ============================================== */
/* Protocol starts                                */
/* ============================================== */
static int lcp_send_rndv_tag_rts_progress(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = ep->tag_chnl;

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t,
                             req->send.tag.src_tid,
                             req->send.tag.tag,
                             req->send.tag.comm);

        /* Immediate data set beforehand since needed to post memory */

        mpc_common_debug_info("LCP: send rndv get offload req=%p, comm_id=%lu, "
                              "tag=%d, src_tid=%d, src_uid=%lu, dest_tid=%d, "
                              "mid=%d, buf=%p.", req, req->send.tag.comm,
                              req->send.tag.tag, req->send.tag.src_tid,
                              req->send.tag.src_uid, req->send.tag.dest_tid,
                              req->tm.mid, req->send.buffer);


        payload_size = lcp_send_do_tag_bcopy(ep->lct_eps[cc],
                                             tag, req->tm.imm.t,
                                             lcp_rts_offload_dummy_pack, req);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
        }

        return rc;
}

/**
 * @brief Start a rendez-vous send.
 *
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_rndv_offload_start(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *rndv_req;

        req->state.offloaded = 1;

        /* Increment offload id */
        req->tm.mid = OPA_fetch_and_incr_int(&req->mngr->muid);
        req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_offload_send_complete,
        };

        /* Create rendez-vous request */
        rndv_req = lcp_request_get(req->task);
        if (rndv_req == NULL) {
                rc = MPC_LOWCOMM_ERROR;
                mpc_common_debug_error("LCP OFFLOAD: could not allocate "
                                       "rndv request.");
                goto err;
        }
        rndv_req->super = req;

        rndv_req->mngr            = req->mngr;
        rndv_req->msg_id          = req->msg_id;
        rndv_req->datatype        = req->datatype;
        rndv_req->send.length     = req->send.length;
        rndv_req->send.buffer     = req->send.buffer;
        rndv_req->send.ep         = req->send.ep;
        rndv_req->state.remaining = req->send.length;
        rndv_req->state.offset    = 0;

        /* Set immediate and tag for matching */
        LCP_TM_SET_HDR_DATA(req->tm.imm.t, 0, LCP_PROTOCOL_RNDV,
                            req->send.length, req->tm.mid);

        /* Set up tag context */
        rndv_req->send.t_ctx.req          = rndv_req;
        rndv_req->send.t_ctx.comp.comp_cb = lcp_rndv_offload_send_complete;

        /* Register memory (for RGET) */
        rc = lcp_rndv_offload_reg_send_buffer(rndv_req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Finally, set RTS send function */
        req->send.func = lcp_send_rndv_tag_rts_progress;

err:
        return rc;
}

/* ============================================== */
/* Send eager                                     */
/* ============================================== */

/**
 * @brief Send a tag eager message using buffer copy
 *
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_tag_offload_eager_bcopy(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t payload;
        lcp_ep_h ep = req->send.ep;
        _mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->tag_chnl];

        lcr_tag_t tag = {.t_tag = {
                .tag = req->send.tag.tag,
                .src = req->send.tag.src_tid,
                .comm = req->send.tag.comm }
        };

        uint64_t imm = 0;
        LCP_TM_SET_HDR_DATA(imm, 0, LCP_PROTOCOL_EAGER,
                            req->send.length, 0);

        payload = lcp_send_do_tag_bcopy(lcr_ep, tag, imm,
                                        lcp_send_tag_offload_pack,
                                        req);

	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
                goto err;
	}

        lcp_request_complete(req, send.send_cb, rc, req->send.length);
err:
        return rc;
}

/**
 * @brief Send a tag eager message using zero copy
 *
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_tag_offload_eager_zcopy(lcp_request_t *req)
{
        lcp_ep_h ep = req->send.ep;
        _mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[ep->tag_chnl];

        lcr_tag_t tag = { 0 };
        LCP_TM_SET_MATCHBITS(tag.t, req->send.tag.src_tid,
                             req->send.tag.tag,
                             req->send.tag.comm);

        struct iovec iov = {
                .iov_base = (void *)req->send.buffer,
                .iov_len  = req->send.length
        };

        uint64_t imm = 0;
        LCP_TM_SET_HDR_DATA(imm, 0, LCP_PROTOCOL_EAGER,
                            req->send.length, 0);

        req->state.comp.comp_cb =  lcp_tag_offload_send_complete;

        mpc_common_debug_info("LCP: post send tag zcopy req=%p, src=%d, dest=%d, "
                              "size=%d, matching[src:tag:comm]=[%d:%d:%d]", req,
                              req->send.tag.src_tid, req->send.tag.dest_tid,
                              req->send.length, tag.t_tag.src, tag.t_tag.tag,
                              tag.t_tag.comm);
        return lcp_send_do_tag_zcopy(lcr_ep, tag, imm, &iov, 1, &(req->state.comp));
}


/* ============================================== */
/* Recv                                           */
/* ============================================== */

int lcp_rndv_offload_process_rts(lcp_request_t *rreq)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *rndv_req;

        /* Rendez-vous request used for RTR (PUT) or RMA (GET) */
        rndv_req = lcp_request_get(rreq->task);
        if (rndv_req == NULL) {
                rc = MPC_LOWCOMM_ERROR;
                mpc_common_debug_error("LCP OFFLOAD: could not allocate "
                                       "rndv request.");
                goto err;
        }
        rndv_req->super = rreq;

        rndv_req->mngr             = rreq->mngr;
        rndv_req->send.tag.src_tid = rreq->recv.tag.src_tid;
        rndv_req->msg_id           = rreq->msg_id;
        rndv_req->datatype         = rreq->datatype;
        rndv_req->send.buffer      = rreq->recv.buffer;
        rndv_req->state.remaining  = rreq->recv.send_length;
        rndv_req->tm               = rreq->tm;
        rndv_req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_rndv_offload_recv_complete
        };
        rndv_req->state.offset    = 0;

        /* Get endpoint */
        uint64_t puid = lcp_get_process_uid(rreq->mngr->ctx->process_uid,
                                            rreq->recv.tag.src_uid);

        if (!(rndv_req->send.ep = lcp_ep_get(rndv_req->mngr, puid))) {
                rc = lcp_ep_create(rndv_req->mngr, &(rndv_req->send.ep), puid, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP: could not create ep after match.");
                        goto err;
                }
        }

        switch (rreq->mngr->ctx->config.rndv_mode) {
        case LCP_RNDV_PUT:
                not_implemented();
                break;
        case LCP_RNDV_GET:
                /* For RGET, data is unpacked upon completion */
                rndv_req->send.func = lcp_rndv_offload_rma_progress;
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown protocol in recv");
                rc = MPC_LOWCOMM_ERROR;
                break;
        }

        lcp_request_send(rndv_req);
err:
        return rc;
}

/* ============================================== */
/* Receive                                        */
/* ============================================== */

int lcp_recv_eager_tag_offload_data(lcp_request_t *req, void *data,
                                    size_t length)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t unpacked_len = 0;

        mpc_common_debug("LCP: recv tag offload data req=%p, src=%d, "
                         "tag=%d, comm=%d", req, req->recv.tag.src_tid,
                         req->recv.tag.tag, req->recv.tag.comm);

        /* Unpack necessary if datatype is derived or if
         * data was in overflow list from underlying transport */
        if (req->recv.t_ctx.flags & LCR_IFACE_TM_OVERFLOW) {
                unpacked_len = lcp_datatype_unpack(req->mngr->ctx, req, req->datatype,
                                                   req->recv.buffer, data, length);
                if (unpacked_len < 0) {
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
        }

        lcp_tag_offload_recv_complete(&(req->state.comp));
err:
        return rc;
}

/**
 * @brief Callback for receiving and completing a tag message
 *
 * @param comp completion
 */
void lcp_recv_tag_callback(lcr_completion_t *comp)
{
        uint8_t op;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t,
                                              recv.t_ctx.comp);
        lcr_tag_t tag = req->recv.t_ctx.tag;
        uint64_t imm = req->recv.t_ctx.imm;

        /* Retrieve protocol data */
        op                    = LCP_TM_GET_HDR_OP(imm);
        req->recv.send_length = LCP_TM_GET_HDR_LENGTH(imm);
        req->tm.mid           = LCP_TM_GET_HDR_UID(imm);
        req->tm.imm           = (lcr_tag_t)imm;

        /* Retrieve tag data */
        //NOTE: process based => tid == pid
        //FIXME: what about gateway of tag offloading ?
        req->recv.tag.src_tid = req->recv.tag.src_uid = LCP_TM_GET_SRC(tag.t);
        req->recv.tag.tag = LCP_TM_GET_TAG(tag.t);
        assert(req->recv.tag.comm == LCP_TM_GET_COMM(tag.t));

        mpc_common_debug_info("LCP: tag callback req=%p, size=%d, "
                              "matching[src:tag:comm]=[%d:%d:%d]", req,
                              req->recv.send_length,
                              tag.t_tag.src, tag.t_tag.tag, tag.t_tag.comm);

        switch(op) {
        case LCP_PROTOCOL_EAGER:
                lcp_recv_eager_tag_offload_data(req, req->recv.t_ctx.start,
                                                req->recv.t_ctx.comp.sent);
                break;
        case LCP_PROTOCOL_RNDV:
                lcp_rndv_offload_process_rts(req);
                break;
        default:
                mpc_common_debug_error("LCP: unknown recv protocol.");
                break;
        }

        return;
}

void lcp_recv_tag_probe_callback(lcr_completion_t *comp)
{
        uint64_t imm;
        lcr_tag_t tag;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t,
                                              recv.t_ctx.comp);

        imm = req->recv.t_ctx.imm;
        tag = req->recv.t_ctx.tag;

        /* Fill out recv info given by user */
        req->recv.tag.info.tag    = LCP_TM_GET_TAG(tag.t);
        req->recv.tag.info.src    = LCP_TM_GET_SRC(tag.t);
        req->recv.tag.info.length = LCP_TM_GET_HDR_LENGTH(imm);
        req->recv.tag.info.found  = 1;

        mpc_common_debug_info("LCP: probe tag callback req=%p, src=%d, size=%d, "
                              "matching=[%d:%d:%d]", req, req->recv.tag.info.src,
                              req->recv.tag.info.length, tag.t_tag.tag,
                              tag.t_tag.src, tag.t_tag.comm);

        lcp_request_complete(req, recv.tag.recv_cb, MPC_LOWCOMM_SUCCESS,
                             &req->recv.tag.info);

        return;
}

//FIXME: according to MPI Standard, tag must be positive. However, MPC uses
//       negative for some collectives (so is OMPI). For such p2p, there is no need for the
//       receiver to get back the tag, so no problem.
//       Some safeguard should be used to forbid tag > 2^23-1.
/**
 * @brief Receive a tag message using zero copy.
 *
 * @param rreq received request
 * @param iface interface to do the copy through
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_recv_tag_zcopy(lcp_request_t *rreq, sctk_rail_info_t *iface)
{
        int rc;
        size_t iovcnt = 0;
        unsigned int flags = 0;
        lcr_tag_t tag = { 0 };

        LCP_TM_SET_MATCHBITS(tag.t, rreq->recv.tag.src_tid,
                             rreq->recv.tag.tag,
                             rreq->recv.tag.comm);

        lcr_tag_t ign_tag = {.t = 0 };
        if (rreq->recv.tag.src_tid == MPC_ANY_SOURCE) {
                ign_tag.t |= LCP_TM_SRC_MASK;
        }
        if (rreq->recv.tag.tag == MPC_ANY_TAG) {
                ign_tag.t |= LCP_TM_TAG_MASK;
        }

        struct iovec iov = {
                .iov_base = rreq->recv.buffer,
                .iov_len  = rreq->recv.length
        };
        iovcnt++;

        rreq->recv.t_ctx.req          = rreq;
        rreq->recv.t_ctx.comp.comp_cb = lcp_recv_tag_callback;
        rreq->state.comp.comp_cb      = lcp_tag_offload_recv_complete;

        mpc_common_debug_info("LCP: post recv tag zcopy req=%p, src=%d, dest=%d, "
                              "size=%d, matching[src:tag:comm]=[%d:%d:%d], ignore=[%d:%d:%d]",
                              rreq, rreq->recv.tag.src_tid, rreq->recv.tag.dest_tid,
                              rreq->recv.length, tag.t_tag.src, tag.t_tag.tag,
                              tag.t_tag.comm, ign_tag.t_tag.tag, ign_tag.t_tag.src,
                              ign_tag.t_tag.comm);
        rc = lcp_post_do_tag_zcopy(iface, tag, ign_tag,
                                   &iov, iovcnt,
                                   flags, &(rreq->recv.t_ctx));

        return rc;
}

int lcp_recv_tag_probe(lcp_task_h task, sctk_rail_info_t *rail, const int src, const int tag,
                       const uint64_t comm, lcp_tag_recv_info_t *recv_info)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        unsigned flags = 0;
        lcp_request_t *probe_req;
        struct iovec iov[1]; int iovcnt = 1;
        lcr_tag_t utag = { 0 };
        LCP_TM_SET_MATCHBITS(utag.t, src, tag, comm);

        lcr_tag_t ign_tag = {.t = 0 };
        if (src == MPC_ANY_SOURCE) {
                ign_tag.t |= LCP_TM_SRC_MASK;
        }
        if (tag == MPC_ANY_TAG) {
                ign_tag.t |= LCP_TM_TAG_MASK;
        }

        iov[0].iov_base = NULL;
        iov[0].iov_base = 0;

        probe_req = lcp_request_get(task);
        if (probe_req == NULL) {
                rc = MPC_LOWCOMM_ERROR;
                mpc_common_debug_error("LCP OFFLOAD: could not allocate "
                                       "rndv request.");
                goto err;
        }

        probe_req->recv.tag.info           = *recv_info;
        probe_req->recv.t_ctx.req          = probe_req;
        probe_req->recv.t_ctx.comp.comp_cb = lcp_recv_tag_probe_callback;

        flags |= LCR_IFACE_TM_SEARCH;

        /* Post search type ME. Do not make communication progress. */
        rc = lcp_post_do_tag_zcopy(rail, utag, ign_tag, iov,
                                   iovcnt, flags,
                                   &(probe_req->recv.t_ctx));
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Event will be raised with next communication progress. */

err:
        return rc;
}
