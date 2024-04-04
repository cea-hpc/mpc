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
                             LCP_REQUEST_USER_PROVIDED_EPH )

/**
 * @brief Copy buffer from argument request to destination
 *
 * @param dest destination buffer
 * @param arg input request buffer
 * @return size_t size of copy
 */
size_t lcp_rma_pack(void *dest, void *arg) {
        lcp_request_t *req = (lcp_request_t *)arg;

        memcpy(dest, req->send.buffer, req->send.length);

        return req->send.length;
}

size_t lcp_rma_unpack(void* req, const void *dest, size_t arg) {
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
void lcp_rma_request_complete(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                req->task->tcct[req->mngr->id]->rma.outstandings--;
                lcp_request_complete(req);
        }
}

void lcp_flush_request_complete(lcr_completion_t *comp)
{
        lcp_request_t *flush_req;
        mpc_queue_iter_t iter;
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        if (--req->state.comp.count == 0) {
                LCP_MANAGER_LOCK(req->mngr);
                mpc_queue_for_each_safe(flush_req, iter, lcp_request_t, 
                                        &req->mngr->flush_queue, queue) {
                        if (flush_req->send.flush.flush == req->send.flush.flush) {
                                lcp_request_complete(flush_req);
                                mpc_queue_del_iter(&req->mngr->flush_queue, iter);
                        }
                }

                req->mngr->flush_completed++;
                LCP_MANAGER_UNLOCK(req->mngr);

                lcp_request_complete(req);
        }
}

/**
 * @brief Put a buffer copy request.
 *
 * @param req request to put
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_rma_progress_bcopy(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep); //FIXME: not sure this works
                                                    //       in multirail

        not_implemented();
	mpc_common_debug_info("LCP: send put bcopy remote addr=%p, dest=%d",
			      req->send.rma.remote_disp, ep->uid);
        if (req->send.rma.is_get) {
                //FIXME: not implemented
                payload_size = lcp_send_do_get_bcopy(ep->lct_eps[cc], 
                                                     lcp_rma_unpack, req, 
                                                     req->send.rma.remote_disp,
                                                     &req->state.lmem->mems[cc],
                                                     &req->send.rma.rkey->mems[cc]);

        } else {
                payload_size = lcp_send_do_put_bcopy(ep->lct_eps[cc], 
                                                     lcp_rma_pack, req, 
                                                     req->send.rma.remote_disp,
                                                     &req->state.lmem->mems[cc],
                                                     &req->send.rma.rkey->mems[cc]);
        }

	if (payload_size < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
                goto err;
	}

        lcp_request_complete(req);

err:
        return rc;
}

//FIXME: code very similar with lcp_send_rput_common
/**
 * @brief Put a zero copy
 *
 * @param req request to put
 * @return int MPC_LOWCOMM_SUCCESS in case of succes
 */
int lcp_rma_progress_zcopy(lcp_request_t *req)
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

        cc = lcp_ep_get_next_cc(ep);

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
                                                   offset,
                                                   offset,
                                                   &(req->state.lmem->mems[cc]),
                                                   &(req->send.rma.rkey->mems[cc]),
                                                   length,
                                                   &(req->state.comp));
                } else {
                        rc = lcp_send_do_put_zcopy(lcr_ep,
                                                   offset,
                                                   offset,
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

                cc = lcp_ep_get_next_cc(ep);
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

        size_t max_bcopy = req->send.rma.is_get ? ep->config.rma.max_get_bcopy :
                ep->config.rma.max_put_bcopy;
        size_t max_zcopy = req->send.rma.is_get ? ep->config.rma.max_get_zcopy :
                ep->config.rma.max_put_zcopy;

        if (req->send.length <= max_bcopy) {
                req->send.func = lcp_rma_progress_bcopy;
        } else if (req->send.length <= max_zcopy) {
                //NOTE: Non contiguous datatype not supported for zcopy put
                assert(req->datatype == LCP_DATATYPE_CONTIGUOUS);
                req->send.func = lcp_rma_progress_zcopy;
        }

        if (!(req->flags & LCP_REQUEST_USER_PROVIDED_MEMH)) {
                req->state.lmem = lcp_pinning_mmu_pin(req->mngr, req->send.buffer, 
                                                      req->send.length, ep->conn_map, 
                                                      0 /* static memory. */);
                if (req->state.lmem == NULL) {
                        rc = MPC_LOWCOMM_ERROR;
                }
        }

        return rc;
}

int lcp_put_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer, size_t length,
               uint64_t remote_disp, lcp_mem_h rkey,
               const lcp_request_param_t *param) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *req;

        req = lcp_request_get(task);
        if (req == NULL) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        LCP_REQUEST_INIT_RMA_PUT(req, ep->mngr, task, length, ep, buffer, 
                                  0 /* no ordering for rma */, 0, param->datatype, 
                                  rkey, remote_disp);

        task->tcct[ep->mngr->id]->rma.outstandings++;

        if (param->flags & LCP_REQUEST_RMA_CALLBACK) {
                assert(param->flags & LCP_REQUEST_USER_REQUEST);
                req->send.send_cb = param->send_cb;
                req->user_data    = param->user_request;
        }

        if (param->flags & LCP_REQUEST_USER_MEMH) {
                req->flags |= LCP_REQUEST_USER_PROVIDED_MEMH;
                req->state.lmem = param->mem;
        }

        rc = lcp_rma_start(ep, req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = lcp_request_send(req);
err:
        return rc;
}

int lcp_get_nb(lcp_ep_h ep, lcp_task_h task, void *buffer, 
               size_t length, uint64_t remote_addr, lcp_mem_h rkey,
               const lcp_request_param_t *param) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *req;

        if (!(param->flags & LCP_REQUEST_USER_MEMH)) {
                mpc_common_debug_error("LCP RMA: rma get without "
                                       "user-provided memory handle not "
                                       "supported yet.");
                rc = MPC_LOWCOMM_NOT_SUPPORTED;
                goto err;
        }

        req = lcp_request_get(task);
        if (req == NULL) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        LCP_REQUEST_INIT_RMA_GET(req, ep->mngr, task, length, ep, (void *)buffer, 
                                  0 /* no ordering for rma */, 0, param->datatype, 
                                  rkey, remote_addr);

        if (param->flags & LCP_REQUEST_RMA_CALLBACK) {
                assert(param->flags & LCP_REQUEST_USER_REQUEST);
                req->send.send_cb = param->send_cb;
                req->user_data    = param->user_request;
        }

        if (param->flags & LCP_REQUEST_USER_MEMH) {
                req->flags |= LCP_REQUEST_USER_PROVIDED_MEMH;
                req->state.lmem = param->mem;
        }

        rc = lcp_rma_start(ep, req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;  
        }

        rc = lcp_request_send(req);
err:
        return rc;
}

int lcp_flush_manager_nb(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        sctk_rail_info_t *iface;
        lcr_rail_attr_t attr;
        
        for (i = 0; i < req->mngr->num_ifaces; i++) {
                iface = req->mngr->ifaces[i];

                iface->iface_get_attr(iface, &attr);
                if (attr.iface.cap.flags & LCR_IFACE_CAP_RMA) {
                        req->state.comp.count++;
                        rc = lcp_do_flush(iface, NULL, NULL, &req->state.comp, 
                                          LCR_IFACE_FLUSH_IFACE);

                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
        }

err:
        return rc;
}

int lcp_flush_ep_nb(lcp_request_t *req)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep = req->send.ep;
        sctk_rail_info_t *iface;
        lcr_rail_attr_t attr;

        //NOTE: at least on channel must support RMA flush.
        assert(req->send.ep->cap & LCR_IFACE_CAP_RMA);

        for (i = 0; i < ep->num_chnls; i++) {
                if (!MPC_BITMAP_GET(ep->conn_map, i)) {
                        continue;
                }

                iface = ep->lct_eps[i]->rail;

                iface->iface_get_attr(iface, &attr);
                if (attr.iface.cap.flags & LCR_IFACE_CAP_RMA) {
                        req->state.comp.count++;
                        rc = lcp_do_flush(iface, ep->lct_eps[i], 
                                          NULL, &req->state.comp, 
                                          LCR_IFACE_FLUSH_EP);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
        }

err:
        return rc;
}

int lcp_flush_mem_nb(lcp_request_t *req)
{
        int i, rc = MPC_LOWCOMM_SUCCESS;
        lcp_mem_h mem = req->state.lmem;

        for (i = 0; req->mngr->num_ifaces; i++) {
                if (!MPC_BITMAP_GET(mem->bm, i)) {
                        continue;
                }

                req->state.comp.count++;
                rc = lcp_do_flush(req->mngr->ifaces[i], NULL, &mem->mems[i], 
                                  &req->state.comp, 
                                  LCR_IFACE_FLUSH_MEM);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

err:
        return rc;
}

int lcp_flush_mem_ep_nb(lcp_request_t *req)
{

        int i, rc = MPC_LOWCOMM_SUCCESS;
        lcp_ep_h ep = req->send.ep;
        lcp_mem_h mem = req->state.lmem;
        sctk_rail_info_t *iface;
        lcr_rail_attr_t attr;

        //NOTE: at least on channel must support RMA flush.
        assert(req->send.ep->cap & LCR_IFACE_CAP_RMA);

        for (i = 0; i < req->mngr->num_ifaces; i++) {
                if (!MPC_BITMAP_GET(ep->conn_map, i) ||
                    !MPC_BITMAP_GET(mem->bm, i) ) {
                        continue;
                }

                iface = ep->lct_eps[i]->rail;

                iface->iface_get_attr(iface, &attr);
                if (attr.iface.cap.flags & LCR_IFACE_CAP_RMA) {
                        req->state.comp.count++;
                        rc = lcp_do_flush(iface, ep->lct_eps[i], 
                                          &mem->mems[i], &req->state.comp, 
                                          LCR_IFACE_FLUSH_EP);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
        }

err:
        return rc;
}

int lcp_flush_nb(lcp_manager_h mngr, lcp_task_h task,
                 const lcp_request_param_t *param)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *req;

        req = lcp_request_get(task);
        if (req == NULL) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        LCP_REQUEST_INIT_RMA_FLUSH(req, mngr, task, 0, NULL, NULL, 
                                  0 /* no ordering for rma */, 0, param->datatype);

        req->state.comp.comp_cb = lcp_flush_request_complete;
        req->send.flush.flush   = task->tcct[mngr->id]->rma.flush_counter++;

        LCP_MANAGER_LOCK(mngr);
        /* Check if someone has already started flushing. */
        if (req->send.flush.flush < mngr->flush_count) {
                /* Check if flushing is already completed. */
                if (req->send.flush.flush < mngr->flush_completed) {
                        rc = MPC_LOWCOMM_SUCCESS;
                } else {
                        mpc_queue_push(&mngr->flush_queue, &req->queue);
                        rc = MPC_LOWCOMM_IN_PROGRESS;
                }
                goto unlock_manager;
        } else {
                mngr->flush_count++;
        }
        LCP_MANAGER_UNLOCK(mngr);

        if (param->flags & LCP_REQUEST_USER_MEMH) {
                req->flags |= LCP_REQUEST_USER_PROVIDED_MEMH;
                req->state.lmem = param->mem;
        }

        if (param->flags & LCP_REQUEST_USER_EPH) {
                req->flags |= LCP_REQUEST_USER_PROVIDED_EPH;
                req->send.ep = param->ep;
        }

        if (param->flags & LCP_REQUEST_RMA_CALLBACK) {
                req->flags |= LCP_REQUEST_RMA_COMPLETE;
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
                break;
        }

        return lcp_request_send(req);
unlock_manager:
        LCP_MANAGER_UNLOCK(mngr);
err:
        return rc;
}
