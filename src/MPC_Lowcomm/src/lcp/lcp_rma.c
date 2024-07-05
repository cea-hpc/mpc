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

#include "lcp.h"

#include "lcp/lcp_ep.h"
#include "lcp_def.h"
#include "lcp_prototypes.h"
#include "lcp_request.h"
#include "lcp_mem.h"

#include <bitmap.h>
#include <queue.h>

#include "mpc_common_debug.h"

enum {
        LCP_FLUSH_SCOPE_MANAGER = 0,
        LCP_FLUSH_SCOPE_EP      = LCP_REQUEST_USER_PROVIDED_EPH,
        LCP_FLUSH_SCOPE_MEM     = LCP_REQUEST_USER_PROVIDED_MEMH,
        LCP_FLUSH_SCOPE_MEM_EP  = LCP_REQUEST_USER_PROVIDED_MEMH |
                LCP_REQUEST_USER_PROVIDED_EPH,
};

#define LCP_RMA_FLUSH_MASK ( LCP_REQUEST_USER_PROVIDED_EPH | \
                             LCP_REQUEST_USER_PROVIDED_MEMH )

/**
 * @brief Copy buffer from argument request to destination
 *
 * @param dest destination buffer
 * @param arg input request buffer
 * @return size_t size of copy
 */
static ssize_t lcp_rma_pack(void *dest, void *arg) {
        lcp_request_t *req = (lcp_request_t *)arg;

        memcpy(dest, req->send.buffer, req->send.length);

        return req->send.length;
}

static ssize_t lcp_rma_unpack(void* req, const void *dest, size_t arg) {
        UNUSED(req);
        UNUSED(dest);
        UNUSED(arg);
        not_implemented();
        return 0;
}

/**
 * @brief Complete a request.
 *
 * @param comp completion
 */
static void lcp_rma_request_complete(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_mem_deprovision(req->mngr, req->state.lmem);

                req->flags |= LCP_REQUEST_REMOTE_COMPLETED | 
                        LCP_REQUEST_LOCAL_COMPLETED;
                req->status = MPC_LOWCOMM_SUCCESS;

                lcp_request_complete(req, send.send_cb, req->status, 0);
        } 
}

static void lcp_flush_request_complete(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        if (--req->state.comp.count == 0) {
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED | 
                        LCP_REQUEST_LOCAL_COMPLETED;
                req->status = MPC_LOWCOMM_SUCCESS;
                lcp_request_complete(req, send.send_cb, req->status, 0);
        }
}

/**
 * @brief Put a buffer copy request.
 *
 * @param req request to put
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
__UNUSED__ static int lcp_rma_progress_bcopy(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = ep->rma_chnl; 

        not_implemented();
	mpc_common_debug_info("LCP: send put bcopy remote addr=%p, dest=%d",
			      req->send.rma.remote_addr, ep->uid);
        if (req->send.rma.is_get) {
                //FIXME: not implemented
                payload_size = lcp_send_do_get_bcopy(ep->lct_eps[cc], 
                                                     lcp_rma_unpack, req, 
                                                     req->send.rma.remote_addr,
                                                     &req->state.lmem->mems[cc],
                                                     &req->send.rma.rkey->mems[cc]);

        } else {
                payload_size = lcp_send_do_put_bcopy(ep->lct_eps[cc], 
                                                     lcp_rma_pack, req, 
                                                     req->send.rma.remote_addr,
                                                     &req->state.lmem->mems[cc],
                                                     &req->send.rma.rkey->mems[cc]);
        }

	if (payload_size < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
                goto err;
	}

        lcp_request_complete(req, send.send_cb, rc, req->send.length);

err:
        return rc;
}

//FIXME: code very similar with lcp_send_rput_common
/**
 * @brief Put a zero copy
 *
 * @param req request to put
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
static int lcp_rma_progress_zcopy(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        size_t remaining, offset;
        size_t frag_length, length;
        lcp_chnl_idx_t cc;
        lcp_ep_h ep;
        lcr_rail_attr_t attr;
        _mpc_lowcomm_endpoint_t *lcr_ep;

        ep        = req->send.ep;
        remaining = req->state.remaining;
        offset    = req->state.offset;
        cc        = ep->rma_chnl;

        /* Get the fragment length from rail attribute */
        //NOTE: max_{put,get}_zcopy might not be optimal.
        ep->lct_eps[cc]->rail->iface_get_attr(ep->lct_eps[cc]->rail, &attr);

        req->state.comp.comp_cb = lcp_rma_request_complete;

        while (remaining > 0) {
                //NOTE: must send on rail on which memory has been pinned.
                assert(MPC_BITMAP_GET(req->send.rma.rkey->bm, cc));
                lcr_ep = ep->lct_eps[cc];

                /* length is min(max_frag_size, per_ep_length) */
                frag_length = attr.iface.cap.rma.max_put_zcopy;
                length =  (remaining < frag_length) ? remaining : frag_length;

                if (req->send.rma.is_get) {
                        rc = lcp_send_do_get_zcopy(lcr_ep,
                                                   (uint64_t)req->send.buffer + offset,
                                                   req->send.rma.remote_addr  + offset,
                                                   &(req->state.lmem->mems[cc]),
                                                   &(req->send.rma.rkey->mems[cc]),
                                                   length,
                                                   &(req->state.comp));
                } else {
                        rc = lcp_send_do_put_zcopy(lcr_ep,
                                                   (uint64_t)req->send.buffer + offset,
                                                   req->send.rma.remote_addr  + offset,
                                                   &(req->state.lmem->mems[cc]),
                                                   &(req->send.rma.rkey->mems[cc]),
                                                   length,
                                                   &(req->state.comp));
                }

                if (rc == MPC_LOWCOMM_NO_RESOURCE) {
                        /* Save state */
                        req->state.remaining = remaining;
                        req->state.offset    = offset;
                        break;
                }

                offset  += length; remaining -= length;

                cc = lcp_ep_get_next_cc(ep, cc, ep->rma_bmap);
                assert(cc != LCP_NULL_CHANNEL);
        }

        return rc;
}

/**
 * @brief Register a send buffer and start a send
 *
 * @param ep endpoint
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
static inline int lcp_rma_start(lcp_ep_h ep, lcp_request_t *req) 
{
        int rc = MPC_LOWCOMM_SUCCESS;

        //FIXME: bcopy not yet supported by transports.
        //NOTE: Non contiguous datatype not supported for zcopy put
        assert(req->datatype == LCP_DATATYPE_CONTIGUOUS);
        req->send.func = lcp_rma_progress_zcopy;

        if (!(req->flags & LCP_REQUEST_USER_PROVIDED_MEMH)) {

                rc = lcp_mem_create(req->mngr, &req->state.lmem);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                rc = lcp_mem_reg_from_map(req->mngr, req->state.lmem, 
                                          ep->rma_bmap, req->send.buffer, 
                                          req->send.length, 
                                          LCR_IFACE_REGISTER_MEM_DYN,
                                          &req->state.lmem->bm);

                assert(mpc_bitmap_equal(ep->rma_bmap, req->state.lmem->bm));
        } else {
                //TODO: check memory and rma call validity.
        } 
err:
        return rc;
}

lcp_status_ptr_t lcp_put_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer, size_t length,
                            uint64_t remote_addr, lcp_mem_h rkey,
                            const lcp_request_param_t *param) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_status_ptr_t ret;
        lcp_request_t *req;

        req = lcp_request_get_param(task, param);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not allocate put request.");
                rc = MPC_LOWCOMM_ERROR;
                return LCP_STATUS_PTR(rc);
        }
        LCP_REQUEST_INIT_RMA_PUT(req, ep->mngr, task, length, param->request, ep, buffer, 
                                  0 /* no ordering for rma */, 0, param->datatype, 
                                  rkey, remote_addr);

        if (param->field_mask & LCP_REQUEST_SEND_CALLBACK) {
                req->flags       |= LCP_REQUEST_USER_CALLBACK;
                req->send.send_cb = param->send_cb;
        }

        if (param->field_mask & LCP_REQUEST_USER_MEMH) {
                req->flags |= LCP_REQUEST_USER_PROVIDED_MEMH;
                req->state.lmem = param->mem;
        }

        rc = lcp_rma_start(ep, req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                return LCP_STATUS_PTR(rc);
        }
        
        mpc_common_debug("LCP RMA: perform put. req=%p, task=%p, "
                         "buffer=%p, length=%d, remote addr=%p.", 
                         req, task, buffer, length, remote_addr);

        ret = lcp_request_send(req);

        return ret;
}

lcp_status_ptr_t lcp_get_nb(lcp_ep_h ep, lcp_task_h task, void *buffer, 
                            size_t length, uint64_t remote_addr, lcp_mem_h rkey,
                            const lcp_request_param_t *param) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_status_ptr_t ret;
        lcp_request_t *req;

        req = lcp_request_get_param(task, param);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not allocate get request.");
                return LCP_STATUS_PTR(MPC_LOWCOMM_ERROR);
        }
        LCP_REQUEST_INIT_RMA_GET(req, ep->mngr, task, length, param->request, 
                                 ep, (void *)buffer, 
                                 0 /* no ordering for rma */, 0, param->datatype, 
                                 rkey, remote_addr);

        if (param->field_mask & LCP_REQUEST_SEND_CALLBACK) {
                req->flags       |= LCP_REQUEST_USER_CALLBACK;
                req->send.send_cb = param->send_cb;
        }

        if (param->field_mask & LCP_REQUEST_USER_MEMH) {
                req->flags |= LCP_REQUEST_USER_PROVIDED_MEMH;
                req->state.lmem = param->mem;
        }

        rc = lcp_rma_start(ep, req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                return LCP_STATUS_PTR(rc);
        }

        mpc_common_debug("LCP RMA: perform get. req=%p, task=%p, "
                         "buffer=%p, length=%d, remote addr=%p.", 
                         req, task, buffer, length, remote_addr);

        ret = lcp_request_send(req);

        return ret;
}

static int lcp_flush_manager_nb(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        sctk_rail_info_t *iface;
        lcr_rail_attr_t attr;

        
        //FIXME: code a better way to get the number of interface that support
        //       RMA.
        for (i = 0; i < req->mngr->num_ifaces; i++) {
                iface = req->mngr->ifaces[i];

                iface->iface_get_attr(iface, &attr);
                if (attr.iface.cap.flags & LCR_IFACE_CAP_RMA) {
                        req->state.comp.count++;
                }
        }
        
        for (i = 0; i < req->mngr->num_ifaces; i++) {
                iface = req->mngr->ifaces[i];

                iface->iface_get_attr(iface, &attr);
                if (attr.iface.cap.flags & LCR_IFACE_CAP_RMA) {
                        rc = lcp_do_flush_iface(iface, &req->state.comp, 0);

                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
        }

err:
        return rc;
}

static int lcp_flush_ep_nb(lcp_request_t *req)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep = req->send.ep;
        sctk_rail_info_t *iface;

        //NOTE: at least on channel must support RMA flush.
        assert(req->send.ep->cap & LCR_IFACE_CAP_RMA);

        req->state.comp.count = mpc_bitmap_popcount(ep->rma_bmap);

        for (i = 0; i < ep->num_chnls; i++) {
                if (!MPC_BITMAP_GET(ep->rma_bmap, i)) {
                        continue;
                }

                iface = ep->lct_eps[i]->rail;
                rc = lcp_do_flush_ep(iface, ep->lct_eps[i], 
                                     &req->state.comp, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

err:
        return rc;
}

static int lcp_flush_mem_nb(lcp_request_t *req)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        lcp_mem_h mem = req->state.lmem;

        req->state.comp.count = mpc_bitmap_popcount(mem->bm);

        for (i = 0; i < req->mngr->num_ifaces; i++) {
                if (!MPC_BITMAP_GET(mem->bm, i)) {
                        continue;
                }

                rc = lcp_do_flush_mem(req->mngr->ifaces[i], &mem->mems[i], 
                                  &req->state.comp, 0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

err:
        return rc;
}

static int lcp_flush_mem_ep_nb(lcp_request_t *req)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep = req->send.ep;
        lcp_mem_h mem = req->state.lmem;
        sctk_rail_info_t *iface;
        bmap_t flush_map = mpc_bitmap_copy_and(ep->rma_bmap, mem->bm);

        //NOTE: at least one channel must support RMA flush.
        assert(req->send.ep->cap & LCR_IFACE_CAP_RMA);

        for (i = 0; i < req->mngr->num_ifaces; i++) {
                if (!MPC_BITMAP_GET(flush_map, i)) {
                        continue;
                }

                iface = ep->lct_eps[i]->rail;

                rc = lcp_do_flush_mem_ep(iface, ep->lct_eps[i], 
                                         &mem->mems[i], &req->state.comp, 
                                         0);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

err:
        return rc;
}

static lcp_status_ptr_t lcp_request_send_flush(lcp_request_t *req)
{
        int rc;
        lcp_status_ptr_t ret;

        switch((rc = req->send.func(req))) {
        case MPC_LOWCOMM_SUCCESS:
                ret = req + 1;
                break;
        case MPC_LOWCOMM_NO_RESOURCE:
                //TODO: implement thread-safe pending queue
                mpc_common_debug_error("LCP RMA: no resource flush "
                                       "operation not implemented.");
                not_implemented();
                ret = req + 1;
                break;
        case MPC_LOWCOMM_IN_PROGRESS:
                ret = req + 1;
                break;
        case MPC_LOWCOMM_ERROR:
                mpc_common_debug_error("LCP: could not send request.");
                ret = LCP_STATUS_PTR(rc);
                break;
        default:
                mpc_common_debug_fatal("LCP: unknown send error.");
                break;
        }
        return ret;

}

lcp_status_ptr_t lcp_flush_nb(lcp_manager_h mngr, lcp_task_h task,
                              const lcp_request_param_t *param)
{
        int rc;
        lcp_status_ptr_t ret;
        lcp_request_t *req;

        //TODO: add check request param macro.

        req = lcp_request_get_param(task, param);
        if (req == NULL) {
                mpc_common_debug_error("LCP RMA: could not allocate flush "
                                       "request.");
                return LCP_STATUS_PTR(MPC_LOWCOMM_ERROR);
        }

        LCP_REQUEST_INIT_RMA_FLUSH(req, mngr, task, 0, param->request, NULL,
                                   NULL, 0 , 0, param->datatype);

        req->state.comp.comp_cb = lcp_flush_request_complete;

        if (param->field_mask & LCP_REQUEST_USER_MEMH) {
                req->flags     |= LCP_REQUEST_USER_PROVIDED_MEMH;
                req->state.lmem = param->mem;
        }

        if (param->field_mask & LCP_REQUEST_USER_EPH) {
                req->flags  |= LCP_REQUEST_USER_PROVIDED_EPH;
                req->send.ep = param->ep;
        }

        if (param->field_mask & LCP_REQUEST_SEND_CALLBACK) {
                req->flags       |= LCP_REQUEST_USER_CALLBACK;
                req->send.send_cb = param->send_cb;
        }

        switch (req->flags & LCP_RMA_FLUSH_MASK) {
        case LCP_FLUSH_SCOPE_EP:
                req->send.func = lcp_flush_ep_nb;
                break;
        case LCP_FLUSH_SCOPE_MEM:
                req->send.func = lcp_flush_mem_nb;
                break;
        case LCP_FLUSH_SCOPE_MEM_EP:
                req->send.func = lcp_flush_mem_ep_nb;
                break;
        case LCP_FLUSH_SCOPE_MANAGER:
                req->send.func = lcp_flush_manager_nb;
                break;
        default:
                mpc_common_debug_error("LCP RMA: unknown flush operation.");
                rc = MPC_LOWCOMM_ERROR;
                return LCP_STATUS_PTR(rc);
                break;
        }

        ret = lcp_request_send_flush(req);

        return ret;
}
