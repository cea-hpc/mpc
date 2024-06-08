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

#include "lcp_ep.h"
#include "lcp_def.h"
#include "lcp_prototypes.h"
#include "lcp_request.h"
#include "lcp_mem.h"
#include "lcp_rma.h"

#include <bitmap.h>

#include "mpc_common_debug.h"

static lcr_atomic_op_t lcp_atomic_op_table[] = {
        [LCP_ATOMIC_OP_ADD]   = LCR_ATOMIC_OP_ADD,
        [LCP_ATOMIC_OP_AND]   = LCR_ATOMIC_OP_AND,
        [LCP_ATOMIC_OP_OR]    = LCR_ATOMIC_OP_OR,
        [LCP_ATOMIC_OP_XOR]   = LCR_ATOMIC_OP_XOR,
        [LCP_ATOMIC_OP_SWAP]  = LCR_ATOMIC_OP_SWAP,
        [LCP_ATOMIC_OP_CSWAP] = LCR_ATOMIC_OP_CSWAP,
};

void lcp_atomic_complete(lcr_completion_t *comp)
{
        lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
                                              state.comp);

        req->flags |= LCP_REQUEST_REMOTE_COMPLETED | 
                LCP_REQUEST_LOCAL_COMPLETED;
        req->status = MPC_LOWCOMM_SUCCESS;

        lcp_request_complete(req, send.send_cb, req->status, 0);
}

static int lcp_atomic_post(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_endpoint_t *ep = req->send.ep->lct_eps[req->send.ep->ato_chnl];

        assert(req->send.ep->cap & LCR_IFACE_CAP_ATOMICS);

        rc = lcp_atomic_do_post(ep, req->send.ato.value,
                                req->send.ato.remote_addr, 
                                lcp_atomic_op_table[req->send.ato.op], 
                                &req->send.ato.rkey->mems[req->send.ep->ato_chnl],
                                req->send.length,
                                &req->state.comp);

        return rc;
}

static int lcp_atomic_fetch(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_endpoint_t *ep = req->send.ep->lct_eps[req->send.ep->ato_chnl];

        assert(req->send.ep->cap & LCR_IFACE_CAP_ATOMICS);

        rc = lcp_atomic_do_fetch(ep, req->send.ato.reply_buffer, 
                                 req->send.ato.value,
                                 req->send.ato.remote_addr, 
                                 lcp_atomic_op_table[req->send.ato.op], 
                                 &req->send.ato.rkey->mems[req->send.ep->ato_chnl],
                                 req->send.length,
                                 &req->state.comp);

        return rc;
}

static int lcp_atomic_cswap(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        _mpc_lowcomm_endpoint_t *ep = req->send.ep->lct_eps[req->send.ep->ato_chnl];

        assert(req->send.ep->cap & LCR_IFACE_CAP_ATOMICS);

        rc = lcp_atomic_do_cswap(ep, req->send.ato.reply_buffer, 
                                 req->send.ato.value,
                                 *(uint64_t *)req->send.ato.reply_buffer,
                                 req->send.ato.remote_addr, 
                                 lcp_atomic_op_table[req->send.ato.op], 
                                 &req->send.ato.rkey->mems[req->send.ep->ato_chnl],
                                 req->send.length,
                                 &req->state.comp);

        return rc;
}

lcp_atomic_proto_t ato_rma_proto = {
        .send_cswap = lcp_atomic_cswap,
        .send_fetch = lcp_atomic_fetch,
        .send_post  = lcp_atomic_post,
};

static inline void lcp_atomic_select_proto(lcp_ep_h ep, lcp_atomic_proto_t **ato_proto_p) 
{
        if (ep->cap & LCR_IFACE_CAP_ATOMICS) {
                *ato_proto_p = &ato_rma_proto;
        } else {
                *ato_proto_p = &ato_sw_proto;
        }
}


lcp_status_ptr_t lcp_atomic_op_nb(lcp_ep_h ep, lcp_task_h task, const void *buffer,
                                  size_t length, uint64_t remote_addr, lcp_mem_h rkey,
                                  lcp_atomic_op_t op_type, const lcp_request_param_t *param)
{
        lcp_status_ptr_t status = MPC_LOWCOMM_SUCCESS;
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_request_t *req;
        lcp_atomic_proto_t *atomic_proto;

        if (length > sizeof(uint64_t)) {
                mpc_common_debug_error("LCP ATO: atomic operation of length "
                                       "superior to sizeof(uint64_t)=%d not "
                                       "supported.", sizeof(uint64_t));
                rc = MPC_LOWCOMM_NOT_SUPPORTED;
                goto err;
        }

        req = lcp_request_get_param(task, param);
        if (req == NULL) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        LCP_REQUEST_INIT_ATO_SEND(req, ep->mngr, task, length, param->request, ep, 
                                  (void *)buffer, 0, 0, param->datatype, 
                                  rkey, remote_addr, op_type);

        if (param->field_mask & LCP_REQUEST_NO_COMPLETE) {
                req->flags |= LCP_REQUEST_USER_ALLOCATED;
        }

        if (param->field_mask & LCP_REQUEST_SEND_CALLBACK) {
                assert(param->field_mask & LCP_REQUEST_USER_REQUEST);
                req->flags       |= LCP_REQUEST_USER_CALLBACK;
                req->send.send_cb = param->send_cb;
        }
        
        if (param->field_mask & LCP_REQUEST_REPLY_BUFFER) {
                req->send.ato.reply_buffer = param->reply_buffer;
        }

        if (param->field_mask & LCP_REQUEST_USER_MEMH) {
                req->flags |= LCP_REQUEST_USER_PROVIDED_MEMH;
                req->state.lmem = param->mem;
        }

        req->state.comp.comp_cb = lcp_atomic_complete;
        
        lcp_atomic_select_proto(ep, &atomic_proto);
        
        if (param->field_mask & LCP_REQUEST_REPLY_BUFFER) {
                req->flags |= LCP_REQUEST_USER_PROVIDED_REPLY_BUF;
                if (op_type == LCP_ATOMIC_OP_CSWAP) {
                        req->send.func = atomic_proto->send_cswap;
                } else {
                        req->send.func = atomic_proto->send_fetch;
                }
        } else {
                req->send.func = atomic_proto->send_post;
        }

        rc = lcp_request_send(req);

        if (rc != MPC_LOWCOMM_SUCCESS) {
                status = (lcp_status_ptr_t)(uint64_t)rc;
        } else {
                status = (lcp_status_ptr_t)(req + 1);
        }
err:
        return status;
}

