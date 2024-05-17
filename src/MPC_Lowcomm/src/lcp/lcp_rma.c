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

#include "mpc_common_debug.h"

/**
 * @brief Copy buffer from argument request to destination
 *
 * @param dest destination buffer
 * @param arg input request buffer
 * @return size_t size of copy
 */
size_t lcp_rma_put_pack(void *dest, void *arg) {
        lcp_request_t *req = (lcp_request_t *)arg;

        memcpy(dest, req->send.buffer, req->send.length);

        return req->send.length;
}

/**
 * @brief Complete a request.
 *
 * @param comp completion
 */
void lcp_rma_request_complete_put(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t,
                                              send.t_ctx.comp);

        req->state.remaining -= comp->sent;
        if (req->state.remaining == 0) {
                lcp_request_complete(req);
        }
}

/**
 * @brief Put a buffer copy request.
 *
 * @param req request to put
 * @return int LCP_SUCCESS in case of success
 */
int lcp_rma_put_bcopy(lcp_request_t *req)
{
        int rc = LCP_SUCCESS;
        int payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep); //FIXME: not sure this works
                                                    //       in multirail

	mpc_common_debug_info("LCP: send put bcopy remote addr=%p, dest=%d",
			      req->send.rma.remote_addr, ep->uid);
        payload_size = lcp_send_do_put_bcopy(ep->lct_eps[cc],
                                             lcp_rma_put_pack, req,
                                             req->send.rma.remote_addr,
                                             &req->send.rma.rkey->mems[cc]);

	if (payload_size < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = LCP_ERROR;
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
 * @return int LCP_SUCCESS in case of succes
 */
int lcp_rma_put_zcopy(lcp_request_t *req)
{
        int rc = LCP_SUCCESS;
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
        frag_length = attr.iface.cap.rndv.max_put_zcopy;

        req->state.comp.comp_cb = lcp_rma_request_complete_put;

        while (remaining > 0) {
                //NOTE: must send on rail on which memory has been pinned.
                assert(MPC_BITMAP_GET(req->send.rma.rkey->bm, cc));
                lcr_ep = ep->lct_eps[cc];

                /* length is min(max_frag_size, per_ep_length) */
                frag_length = attr.iface.cap.rma.max_put_zcopy;
                length =  (remaining < frag_length) ? remaining : frag_length;

                rc = lcp_send_do_put_zcopy(lcr_ep,
                                           (uint64_t)req->send.buffer + offset,
                                           offset,
                                           &(req->send.rma.rkey->mems[cc]),
                                           length,
                                           &(req->state.comp));

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
 * @return int LCP_SUCCESS in case of success
 */
static inline int lcp_rma_put_start(lcp_ep_h ep, lcp_request_t *req)
{
        if (req->send.length <= ep->ep_config.rma.max_put_bcopy) {
                req->send.func = lcp_rma_put_bcopy;
        } else if (req->send.length <= ep->ep_config.rma.max_put_zcopy) {
                //NOTE: Non contiguous datatype not supported for zcopy put
                assert(req->datatype == LCP_DATATYPE_CONTIGUOUS);
                req->send.func = lcp_rma_put_zcopy;
        }

        //FIXME: no need for return code anymore since no registration on sender
        //       side.
        return LCP_SUCCESS;
}

int lcp_put_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer, size_t length,
               uint64_t remote_addr, lcp_mem_h rkey,
               const lcp_request_param_t *param)
{
        int rc = LCP_SUCCESS;
        lcp_request_t *req;

        assert(param->flags & LCP_REQUEST_USER_REQUEST);

        req = lcp_request_get(task);
        if (req == NULL) {
                rc = LCP_ERROR;
                goto err;
        }
        req->flags |= LCP_REQUEST_RMA_COMPLETE;
        LCP_REQUEST_INIT_RMA_SEND(req, ep->ctx, task, NULL, NULL, length, ep, (void *)buffer,
                                  0 /* no ordering for rma */, 0, param->datatype);
        lcp_request_init_rma_put(req, remote_addr, rkey, param);

        rc = lcp_rma_put_start(ep, req);
        if (rc != LCP_SUCCESS) {
                goto err;
        }

        rc = lcp_request_send(req);
err:
        return rc;
}
