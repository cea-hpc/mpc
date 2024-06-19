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

#include "lcp_def.h"
#include "lcp_request.h"
#include "lcp_eager.h"
#include "lcp_context.h"

#include "lcp_rma.h"

#include <bitmap.h>

#include "mpc_common_debug.h"

typedef uint64_t (*lcp_atomic_func_t)(uint64_t *orig_val, uint64_t *remote_val);

typedef struct lcp_atomic_reply {
        uint64_t dest_uid;
        uint64_t msg_id;
        uint64_t result;
} lcp_atomic_reply_t;

static uint32_t lcp_ato_sw_add32(uint32_t *orig_val, uint32_t *remote_val) {
        uint32_t tmp = *orig_val;
        *orig_val += *remote_val;
        return tmp;
}

static uint64_t lcp_ato_sw_add64(uint64_t *orig_val, uint64_t *remote_val) {
        uint64_t tmp = *orig_val;
        *orig_val += *remote_val;
        return tmp;
}

static uint32_t lcp_ato_sw_or32(uint32_t *orig_val, uint32_t *remote_val) {
        uint32_t tmp = *orig_val;
        *orig_val |= *remote_val;
        return tmp;
}

static uint64_t lcp_ato_sw_or64(uint64_t *orig_val, uint64_t *remote_val) {
        uint64_t tmp = *orig_val;
        *orig_val |= *remote_val;
        return tmp;
}

static uint32_t lcp_ato_sw_xor32(uint32_t *orig_val, uint32_t *remote_val) {
        uint32_t tmp = *orig_val;
        *orig_val ^= *remote_val;
        return tmp;
}

static uint64_t lcp_ato_sw_xor64(uint64_t *orig_val, uint64_t *remote_val) {
        uint64_t tmp = *orig_val;
        *orig_val ^= *remote_val;
        return tmp;
}

static uint32_t lcp_ato_sw_and32(uint32_t *orig_val, uint32_t *remote_val) {
        uint32_t tmp = *orig_val;
        *orig_val &= *remote_val;
        return tmp;
}

static uint64_t lcp_ato_sw_and64(uint64_t *orig_val, uint64_t *remote_val) {
        uint64_t tmp = *orig_val;
        *orig_val &= *remote_val;
        return tmp;
}

static uint32_t lcp_ato_sw_swap32(uint32_t *orig_val, uint32_t *remote_val) {
        uint32_t tmp = *orig_val;
        *orig_val = *remote_val;
        return tmp;
}

static uint64_t lcp_ato_sw_swap64(uint64_t *orig_val, uint64_t *remote_val) {
        uint64_t tmp = *orig_val;
        *orig_val = *remote_val;
        return tmp;
}

static uint32_t lcp_ato_sw_cswap32(uint32_t *orig_val, uint32_t compare, 
                                   uint32_t *remote_val) {
        uint32_t tmp = *orig_val;
        if (*orig_val == compare) {
                *orig_val = *remote_val;
        }
        return tmp;
}

static uint64_t lcp_ato_sw_cswap64(uint64_t *orig_val, uint64_t compare, 
                                 uint64_t *remote_val) {
        uint64_t tmp = *orig_val;
        if (*orig_val == compare) {
                *orig_val = *remote_val;
        }
        return tmp;
}

ssize_t lcp_atomic_pack(void *dest, void *data) {
        lcp_request_t *req = (lcp_request_t *)data;
        lcp_ato_hdr_t *hdr = (lcp_ato_hdr_t *)dest;

        //FIXME: maybe a difference should be made between LCP protocol data and
        //       actual atomic payload.
        hdr->op          = req->send.ato.op;
        hdr->value       = req->send.ato.value;
        hdr->dest_tid    = req->task->tid;
        hdr->src_uid     = req->task->uid;
        hdr->remote_addr = req->send.ato.remote_addr;
        hdr->length      = req->send.length;

        if (req->flags & LCP_REQUEST_USER_PROVIDED_REPLY_BUF) {
            hdr->msg_id = (uint64_t)req;
            if (req->send.ato.op == LCP_ATOMIC_OP_CSWAP) { 
                hdr->compare = req->send.ato.compare;
            }
        } else {
                hdr->msg_id = 0;
        }


        return sizeof(*hdr);
}

ssize_t lcp_atomic_reply_pack(void *dest, void *data) {
        lcp_request_t *req = (lcp_request_t *)data;
        lcp_ato_reply_hdr_t *hdr = (lcp_ato_reply_hdr_t *)dest;

        hdr->msg_id      = req->send.reply_ato.msg_id;
        hdr->result      = req->send.reply_ato.result;

        return sizeof(*hdr);
}

static void lcp_atomic_complete(lcr_completion_t *comp) 
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      state.comp);

        if ( (req->flags & LCP_REQUEST_REMOTE_COMPLETED) && 
             (req->flags & LCP_REQUEST_LOCAL_COMPLETED) ) {
                req->status = MPC_LOWCOMM_SUCCESS;
                lcp_request_complete(req, send.send_cb, req->status, req->send.length); 
        }
}

int lcp_atomic_sw(lcp_request_t *req) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t packed_size;

        mpc_common_debug("LCP ATO SW: perform op. req=%p, task=%p, tid=%d, "
                         "remote addr=%p, op=%s, compare=%llu, value=%llu", req,
                         req->task, req->task->tid, req->send.ato.remote_addr,
                         lcp_ato_sw_decode_op(req->send.ato.op), 
                         req->send.ato.compare,
                         req->send.ato.value);

        packed_size = lcp_send_eager_bcopy(req, lcp_atomic_pack, LCP_AM_ID_ATOMIC);

        if (packed_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
        }

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;
        if (!(req->flags & LCP_REQUEST_USER_PROVIDED_REPLY_BUF)) {
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
        } 

        lcp_atomic_complete(&req->state.comp);

        return rc;
}

int lcp_atomic_reply(lcp_manager_h mngr, lcp_task_h task, 
                     lcp_atomic_reply_t *ato_reply)
{
        int rc;
	ssize_t payload_size;
        lcp_ep_h ep;
	lcp_request_t *reply_req;
	
        /* Get endpoint */
        rc = lcp_ep_get_or_create(mngr, ato_reply->dest_uid, &ep, 0);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        reply_req = lcp_request_get(task);
        if (reply_req == NULL) {
                mpc_common_debug_error("LCP ATO SW: could not allocate "
                                       "reply request.");
                goto err;
        }
        reply_req->send.reply_ato.result = ato_reply->result;
        reply_req->send.reply_ato.msg_id = ato_reply->msg_id;
        reply_req->send.ep = ep;

        mpc_common_debug("LCP ATO: send atomic reply. dest_tid=%d", 
                         ato_reply->dest_uid);

        payload_size = lcp_send_eager_bcopy(reply_req, lcp_atomic_reply_pack, 
                                            LCP_AM_ID_ATOMIC_REPLY);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        reply_req->flags |= LCP_REQUEST_LOCAL_COMPLETED | 
                LCP_REQUEST_REMOTE_COMPLETED;

        /* Complete ack request */
        lcp_request_complete(reply_req, send.send_cb, reply_req->status, 
                             reply_req->send.length);

err:
	return rc;

}

lcp_atomic_proto_t ato_sw_proto = {
        .send_fetch = lcp_atomic_sw,
        .send_cswap = lcp_atomic_sw,
        .send_post  = lcp_atomic_sw,
};

static int lcp_atomic_handler(void *arg, void *data, 
                              size_t length,
                              unsigned flags)
{
        UNUSED(length);
        UNUSED(flags);
        int i = 0, rc = MPC_LOWCOMM_SUCCESS;
        lcp_manager_h mngr = (lcp_manager_h)arg;
        lcp_context_h ctx  = mngr->ctx;
        lcp_ato_hdr_t *hdr = (lcp_ato_hdr_t *)data;
        lcp_task_h task;
        lcp_atomic_reply_t reply;

        reply.dest_uid = hdr->src_uid;
        reply.msg_id   = hdr->msg_id;

        mpc_common_spinlock_lock(&mngr->atomic_lock);
        switch (hdr->op) {
        case LCP_ATOMIC_OP_ADD:
                switch(hdr->length) {
                case sizeof(uint32_t):
                        reply.result = lcp_ato_sw_add32((uint32_t *)hdr->remote_addr, 
                                                        (uint32_t *)&hdr->value);
                        break;
                case sizeof(uint64_t):
                        reply.result = lcp_ato_sw_add64((uint64_t *)hdr->remote_addr, 
                                                        &hdr->value);
                        break;
                default:
                        mpc_common_debug_fatal("LCP ATO SW: invalid atomic length: %d", hdr->length);
                        break;
                }
                break;
        case LCP_ATOMIC_OP_AND:
                switch(hdr->length) {
                case sizeof(uint32_t):
                        reply.result = lcp_ato_sw_and32((uint32_t *)hdr->remote_addr, 
                                                        (uint32_t *)&hdr->value);
                        break;
                case sizeof(uint64_t):
                        reply.result = lcp_ato_sw_and64((uint64_t *)hdr->remote_addr, 
                                                        &hdr->value);
                        break;
                default:
                        mpc_common_debug_fatal("LCP ATO SW: invalid atomic length: %d", hdr->length);
                        break;
                }
                break;
                break;
        case LCP_ATOMIC_OP_OR:
                switch(hdr->length) {
                case sizeof(uint32_t):
                        reply.result = lcp_ato_sw_or32((uint32_t *)hdr->remote_addr, 
                                                       (uint32_t *)&hdr->value);
                        break;
                case sizeof(uint64_t):
                        reply.result = lcp_ato_sw_or64((uint64_t *)hdr->remote_addr, 
                                                       &hdr->value);
                        break;
                default:
                        mpc_common_debug_fatal("LCP ATO SW: invalid atomic length: %d", hdr->length);
                        break;
                }
                break;
        case LCP_ATOMIC_OP_XOR:
                switch(hdr->length) {
                case sizeof(uint32_t):
                        reply.result = lcp_ato_sw_xor32((uint32_t *)hdr->remote_addr, 
                                                        (uint32_t *)&hdr->value);
                        break;
                case sizeof(uint64_t):
                        reply.result = lcp_ato_sw_xor64((uint64_t *)hdr->remote_addr, 
                                                        &hdr->value);
                        break;
                default:
                        mpc_common_debug_fatal("LCP ATO SW: invalid atomic length: %d", hdr->length);
                        break;
                }
                break;
        case LCP_ATOMIC_OP_SWAP:
                switch(hdr->length) {
                case sizeof(uint32_t):
                        reply.result = lcp_ato_sw_swap32((uint32_t *)hdr->remote_addr, 
                                                         (uint32_t *)&hdr->value);
                        break;
                case sizeof(uint64_t):
                        reply.result = lcp_ato_sw_swap64((uint64_t *)hdr->remote_addr, 
                                                         &hdr->value);
                        break;
                default:
                        mpc_common_debug_fatal("LCP ATO SW: invalid atomic length: %d", hdr->length);
                        break;
                }
                break;
        case LCP_ATOMIC_OP_CSWAP:
                switch(hdr->length) {
                case sizeof(uint32_t):
                        reply.result = lcp_ato_sw_cswap32((uint32_t *)hdr->remote_addr, 
                                                          (uint32_t)hdr->compare, 
                                                          (uint32_t *)&hdr->value);
                        break;
                case sizeof(uint64_t):
                        reply.result = lcp_ato_sw_cswap64((uint64_t *)hdr->remote_addr, 
                                                          (uint64_t)hdr->compare, 
                                                          (uint64_t *)&hdr->value);
                        break;
                default:
                        mpc_common_debug_fatal("LCP ATO SW: invalid atomic length: %d", hdr->length);
                        break;
                }
                break;
        }
        mpc_common_spinlock_unlock(&mngr->atomic_lock);

        if (hdr->msg_id != 0) {
                //FIXME: find another way to get to the task. Our only need is to get to
                //       pool of request that are only associated to the tasks for now.
                while (ctx->tasks[i] == NULL) i++;

                task = lcp_context_task_get(mngr->ctx, i);
                if (task == NULL) {
                        mpc_common_debug_error("LCP ATO: could not find any task");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }

                rc = lcp_atomic_reply(mngr, task, &reply);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }

err:
        return rc;
}

static int lcp_atomic_reply_handler(void *arg, void *data, 
                                    size_t length,
                                    unsigned flags)
{
        UNUSED(length);
        UNUSED(flags);
        UNUSED(arg);
        lcp_ato_reply_hdr_t *hdr = (lcp_ato_reply_hdr_t *)data;

        lcp_request_t *req = (lcp_request_t *)hdr->msg_id;

        memcpy(req->send.ato.reply_buffer, &hdr->result, req->send.length);

        req->flags |= LCP_REQUEST_REMOTE_COMPLETED;

        lcp_atomic_complete(&req->state.comp);

        return MPC_LOWCOMM_SUCCESS;
}

LCP_DEFINE_AM(LCP_AM_ID_ATOMIC, lcp_atomic_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_ATOMIC_REPLY, lcp_atomic_reply_handler, 0);
