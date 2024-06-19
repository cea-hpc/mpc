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

#include "lcp_context.h"
#include "lcp_prototypes.h"
#include "lcp_request.h"
#include "lcp_header.h"
#include "lcp_ep.h"
#include "lcp_datatype.h"
#include "lcp_eager.h"
#include "lcp_task.h"
#include "lcp_rndv.h"
#include "mpc_common_debug.h"

#define LCP_SEND_AM_IS_TASK(_req) \
        ((_req)->send.ep->uid == (_req)->mngr->ctx->process_uid) 

/* ============================================== */
/* Packing                                        */
/* ============================================== */

/* Memory layout for AM API after packing and just before sending message.
 * Unexpected messages are encapsulated with lcp_unexp_ctnr_t to shaddow buffers.
 *
 * Eager:
 *  ----------------------------------------------------------- -- - -
 *  | transport   |  lcp_am_hdr_t |             |
 *  |  hdr        |               |  user hdr   |  user data
 *  |             |               |             |
 *  |             |               |             |
 *  ----------------------------------------------------------- -- - -
 *                ^               <------------->
 *                |                user hdr size
 *               dest
 *
 * Rendez-vous:
 *  --------------------------------------------------------------------- -- - -
 *  | transport   |                  |                |            |
 *  |  hdr        |  lcp_rndv_hdr_t  |  remote key    | user hdr   | user data
 *  |             |                  |  (bmap + pin)  |            |
 *  |             |                  |                |            |
 *  --------------------------------------------------------------------- -- - -
 *                ^
 *                |
 *               dest
*/

static ssize_t lcp_send_am_eager_pack(void *dest, void *data)
{
        //TODO: add safeguard to check for max bcopy size
        ssize_t packed_length = 0;
        ssize_t packed_payload_length = 0;
	lcp_am_hdr_t *hdr = dest;
	lcp_request_t *req = data;
        void *src = req->datatype == LCP_DATATYPE_CONTIGUOUS ?
               req->send.buffer : NULL;
        void *ptr = (char *)(hdr + 1) + req->send.am.hdr_size;

        /* Pack AM data */
        hdr->am_id    = req->send.am.am_id;
        hdr->hdr_size = req->send.am.hdr_size;
        hdr->src_uid  = req->send.am.src_uid;
        hdr->dest_tid  = req->send.am.dest_tid;

        /* Pack user header */
        memcpy(hdr + 1, req->send.am.hdr, req->send.am.hdr_size);
        packed_length += sizeof(*hdr) + req->send.am.hdr_size;

        /* Pack actual data */
        packed_payload_length = lcp_datatype_pack(req->mngr->ctx, req, req->datatype,
                                                  ptr, src, req->send.length);

        if (packed_payload_length < 0) {
                packed_length = packed_payload_length;
        } else {
                packed_length += packed_payload_length;
        }

	return packed_length;
}

static ssize_t lcp_send_am_rndv_pack(void *dest, void *data)
{
        //TODO: add safeguard to check for max bcopy size
        ssize_t packed_length = 0;
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t *req = data;
        void *ptr;

        /* Pack AM data */
        hdr->am.am_id    = req->send.am.am_id;
        hdr->am.hdr_size = req->send.am.hdr_size;
        hdr->am.src_uid  = req->send.am.src_uid;
        hdr->am.dest_tid = req->send.am.dest_tid;
        hdr->size        = req->send.length;
        hdr->src_uid     = req->send.am.src_uid;
        hdr->msg_id      = req->msg_id;
        packed_length   += sizeof(*hdr);

        /* Pack rndv data */
        packed_length   += lcp_rndv_rts_pack(req, hdr);

        /* Pack user header */
        ptr = (char *)(hdr + 1) + packed_length;
        memcpy(ptr, req->send.am.hdr, req->send.am.hdr_size);
        packed_length += req->send.am.hdr_size;

	return packed_length;
}

/* ============================================== */
/* Completion                                     */
/* ============================================== */

static void lcp_am_send_complete(lcr_completion_t *comp) {

	lcp_request_t *req = mpc_container_of(comp, lcp_request_t,
					      state.comp);

        if ((req->flags & LCP_REQUEST_REMOTE_COMPLETED) &&
            (req->flags & LCP_REQUEST_LOCAL_COMPLETED)) {
                lcp_request_complete(req, send.send_cb, 
                                     MPC_LOWCOMM_SUCCESS, req->send.length);
        }
}

static void lcp_am_recv_complete(lcr_completion_t *comp)
{
	lcp_request_t *req = mpc_container_of(comp, lcp_request_t,
					      state.comp);
        lcp_unexp_ctnr_t *ctnr = req->recv.am.ctnr;

        lcp_request_complete(req, recv.am.cb, 
                             MPC_LOWCOMM_SUCCESS, 
                             req->recv.send_length);

        //FIXME: recheck why we need to test the flags before putting back the
        //       container.
        //FIXME: for now, these can be called only for RNDV but it should be
        //       generalized for EAGER messages also.
        if (req->flags & LCP_REQUEST_USER_CALLBACK) {
                /* Free container */
                lcp_container_put(ctnr);
        }
}

/* ============================================== */
/* Send                                           */
/* ============================================== */

static int lcp_send_rndv_am_rts_progress(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

        mpc_common_debug("LCP AM: start am rndv. src=%d, dest=%d, am_id=%d, "
                         "hdr_size=%d", req->send.am.src_uid, ep->uid,
                         req->send.am.am_id, req->send.am.hdr_size);

        payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc],
                                            LCP_AM_ID_RTS_AM,
                                            lcp_send_am_rndv_pack, req);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
        }

        return rc;
}

int lcp_send_rndv_am_start(lcp_request_t *req)
{
        int rc;

        /* Set both completion flags since completion will be called only once
         * upon reception of FIN message */
        req->flags |= LCP_REQUEST_LOCAL_COMPLETED |
                LCP_REQUEST_REMOTE_COMPLETED;
	req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_am_send_complete
        };

        rc = lcp_send_rndv_start(req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        req->send.func = lcp_send_rndv_am_rts_progress;

err:
        return rc;
}

int lcp_send_eager_am_bcopy(lcp_request_t *req)
{
        uint8_t am_id;
        lcr_pack_callback_t pack_cb;
        ssize_t payload;
	int rc = MPC_LOWCOMM_SUCCESS;

	mpc_common_debug_info("LCP: send eager am bcopy comm=%d, src=%d, "
                              "dest=%d, length=%d, tag=%d", req->send.tag.comm,
                              req->send.tag.src_tid, req->send.tag.dest_tid, 
                              req->send.length, req->send.tag.tag);

        pack_cb = lcp_send_am_eager_pack;
        if (req->is_sync) {
                am_id   = LCP_AM_ID_EAGER_AM_SYNC;
        } else {
		am_id       = LCP_AM_ID_EAGER_AM;
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
	}

        req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_am_send_complete,
        };

        payload = lcp_send_eager_bcopy(req, pack_cb, am_id);
        if (payload < 0) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;
        lcp_am_send_complete(&(req->state.comp));

err:
	return rc;
}

int lcp_send_eager_am_zcopy(lcp_request_t *req)
{

	int rc;
        uint8_t am_id;
        size_t iovcnt     = 0;
	lcp_ep_h ep       = req->send.ep;
        size_t max_iovec  = ep->config.am.max_iovecs;
	struct iovec *iov = alloca(max_iovec*sizeof(struct iovec));

        //FIXME: hdr never freed, how should it be initialized?
	lcp_am_hdr_t *hdr = sctk_malloc(sizeof(lcp_am_hdr_t));
        if (hdr == NULL) {
                mpc_common_debug_error("LCP: could not allocate tag header.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        hdr->am_id    = req->send.am.am_id;
        hdr->hdr_size = req->send.am.hdr_size;
        hdr->src_uid  = req->send.am.src_uid;

	req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_am_send_complete
        };

        /* First, set user header */
	iov[0].iov_base = req->send.am.hdr;
	iov[0].iov_len  = req->send.am.hdr_size;
        iovcnt++;

        /* Then, set the data */
	iov[1].iov_base = (void *)req->send.buffer;
	iov[1].iov_len  = req->send.length;
        iovcnt++;

	mpc_common_debug_info("LCP: send eager am zcopy am_id=%d, "
                              "length=%d", req->send.am.am_id,
                              req->send.length);

	if(req->is_sync) {
                am_id = LCP_AM_ID_EAGER_AM_SYNC;
        } else {
                am_id       = LCP_AM_ID_EAGER_AM;
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
        }

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;

        rc = lcp_send_eager_zcopy(req, am_id,
                                  hdr, sizeof(lcp_am_hdr_t),
                                  iov, iovcnt,
                                  &(req->state.comp));
err:
	return rc;
}

int lcp_am_send_start(lcp_ep_h ep, lcp_request_t *req,
                      const lcp_request_param_t *param)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        size_t size;

        if (param->field_mask & LCP_REQUEST_AM_SYNC) {
                req->is_sync = 1;
        }
        //NOTE: multiplexing might not always be efficient (IO NUMA
        //      effects). A specific scheduling policy should be
        //      implemented to decide
        req->state.offloaded = 0;
        //FIXME: remove usage of header structure
        size = req->send.length + req->send.am.hdr_size + sizeof(lcp_am_hdr_t);
        if (LCP_SEND_AM_IS_TASK(req)) {
                //FIXME: implement am send for task-based communication.
                mpc_common_debug_warning("LCP AM: task-based communication "
                                         "not implemented.");
                not_implemented();
        } else if (size <= ep->config.am.max_bcopy || 
            ((req->send.length <= ep->config.am.max_zcopy) &&
             (param->datatype & LCP_DATATYPE_DERIVED))) {
                req->send.func = lcp_send_eager_am_bcopy;
        } else if ((size <= ep->config.am.max_zcopy) &&
                   (param->datatype & LCP_DATATYPE_CONTIGUOUS)) {
                req->send.func = lcp_send_eager_am_zcopy;
        } else {
                rc = lcp_send_rndv_am_start(req);
        }

        return rc;
}

lcp_status_ptr_t lcp_am_send_nb(lcp_ep_h ep, lcp_task_h task, int32_t dest_tid,
                                uint8_t am_id, void *hdr, size_t hdr_size,
                                const void *buffer, size_t data_size,
                                const lcp_request_param_t *param)
{
        int rc;
        lcp_status_ptr_t ret;
        lcp_request_t *req;

        // create the request to send
        req = lcp_request_get_param(task, param);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not create request.");
                return LCP_STATUS_PTR(MPC_LOWCOMM_ERROR);
        }

        // initialize request
        //FIXME: sequence number should be task specific. Following works in
        //       process-based but not in thread-based.
        //       Reorder is to be reimplemented.
        //FIXME: what length should be set here, length of data or length of
        //       data + header size ?
        LCP_REQUEST_INIT_AM_SEND(req, ep->mngr, task, am_id, param->request,
                                 ep->mngr->ctx->process_uid, dest_tid, 
                                 data_size, ep, (void *)buffer, 0 /* no seqn for am */, 
                                 (uint64_t)req, param->datatype,
                                 param->field_mask & LCP_REQUEST_AM_SYNC ? 1 : 0);

        /* Copy user header send can be delayed and since it can be allocated on
         * the stack. */
        req->send.am.hdr_size = hdr_size;
        req->send.am.hdr = sctk_malloc(hdr_size);
        if (req->send.am.hdr == NULL) {
                mpc_common_debug_error("LCP AM: could not allocated user header "
                                       "for copy");
                return LCP_STATUS_PTR(MPC_LOWCOMM_ERROR);
        }
        memcpy(req->send.am.hdr, hdr, hdr_size);

        if (param->field_mask & LCP_REQUEST_SEND_CALLBACK) {
                req->flags          |= LCP_REQUEST_USER_CALLBACK;
                req->send.send_cb    = param->send_cb;
        }

        /* Prepare request depending on its type */
        rc = lcp_am_send_start(ep, req, param);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP: could not prepare send request.");
                return LCP_STATUS_PTR(MPC_LOWCOMM_ERROR);
        }

#ifdef MPC_ENABLE_TOPOLOGY_SIMULATION
        /* Simulate latency/bandwidth if needed */
        mpc_topology_simulate_distance(
                mpc_lowcomm_peer_get_rank(mpc_lowcomm_monitor_get_uid()),
                req->send.am.dest_tid,
                hdr_size + data_size);
#endif

        /* send the request */
        ret = lcp_request_send(req);

        return ret;
                
}

/* ============================================== */
/* Receive                                        */
/* ============================================== */

lcp_status_ptr_t lcp_am_recv_nb(lcp_manager_h mngr, lcp_task_h task, void *data_ctnr, void *buffer, 
                                size_t count, const lcp_request_param_t *param)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int packed_data_size;
        lcp_request_t *req;
        lcp_rndv_hdr_t *hdr;
        lcp_unexp_ctnr_t *ctnr = (lcp_unexp_ctnr_t *)data_ctnr - 1;

        /* Get back receive container */
        assert(ctnr->flags & LCP_RECV_CONTAINER_UNEXP_RNDV_AM);

        /* Create the request associated to the container */
        req = lcp_request_get_param(task, param);
        if (req == NULL) {
                mpc_common_debug_error("LCP: could not create request.");
                return LCP_STATUS_PTR(MPC_LOWCOMM_ERROR);
        }

        // initialize request
        LCP_REQUEST_INIT_AM_RECV(req, mngr, task, count, param->request, 
                                 buffer, param->datatype);

        if (param->field_mask & LCP_REQUEST_SEND_CALLBACK) {
                req->flags          |= LCP_REQUEST_USER_CALLBACK;
                req->request = param->request;
                req->recv.am.cb      = param->send_cb;
        }

        /* Set header */
        hdr = (lcp_rndv_hdr_t *)(ctnr + 1);

        req->recv.send_length = hdr->size;
        req->recv.am.am_id    = hdr->am.am_id;
        req->recv.am.ctnr     = ctnr;
        req->state.comp       = (lcr_completion_t) {
                .comp_cb = lcp_am_recv_complete,
        };

        packed_data_size = ctnr->length - sizeof(*hdr) - hdr->am.hdr_size;

        rc = lcp_rndv_process_rts(req, hdr, packed_data_size);
        if (rc == MPC_LOWCOMM_ERROR) {
                return LCP_STATUS_PTR(rc);
        }

        return req + 1;
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */
static int lcp_eager_am_handler(void *arg, void *data,
                                size_t length, unsigned flags)
{
        UNUSED(flags);
        lcp_ep_h ep;
        lcp_am_recv_param_t param = {};
        int rc             = MPC_LOWCOMM_SUCCESS;
        lcp_task_h task    = NULL;
        lcp_manager_h mngr  = arg;
        lcp_am_hdr_t *hdr  = data;
        lcp_am_user_handler_t handler;
        void *data_ptr     = (char *)(hdr + 1) + hdr->hdr_size;
        size_t data_size   = length - sizeof(*hdr) - hdr->hdr_size;

        task = lcp_context_task_get(mngr->ctx, hdr->dest_tid);  
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP: could not find task with tid=%d length=%ld", hdr->dest_tid, hdr->hdr_size);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        handler = task->tcct[mngr->id]->am.handlers[hdr->am_id];

        //FIXME: what happen when ep is in CONNECTING state ?
        rc = lcp_ep_get_or_create(mngr, hdr->src_uid, &ep, 0);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        //FIXME: no support for non contiguous data
        //FIXME: how to handle return code ?
        param.flags   |= LCP_AM_EAGER;
        param.reply_ep = ep;
        handler.cb(handler.user_arg, (void *)(hdr + 1), hdr->hdr_size,
                   data_ptr, data_size, &param);

err:
        return rc;
}

static inline void build_eager_am_handler_iov(void *data, size_t length, size_t hdr_size,
                                              unsigned flags, struct iovec *iov, size_t *iovcnt)
{
        assert(flags != 0);
        if (flags & LCR_IFACE_AM_LAYOUT_BUFFER) {
                iov[0].iov_base = data;
                iov[0].iov_len  = hdr_size;
                iov[1].iov_base = (char *)data + hdr_size;
                iov[1].iov_len  = length - hdr_size;
                *iovcnt         = 2;

        } else {
                memcpy(iov, data, length * sizeof(struct iovec));
                *iovcnt = length;
        }
}

static int lcp_eager_am_sync_handler(void *arg, void *data,
                                     size_t length, unsigned flags)
{
        UNUSED(flags);
        UNUSED(length);
        UNUSED(arg);
        UNUSED(data);

        not_implemented();

        return MPC_LOWCOMM_SUCCESS;
}

static int lcp_eager_am_sync_ack_handler(void *arg, void *data,
                                         size_t length, unsigned flags)
{
        UNUSED(flags);
        UNUSED(length);
        UNUSED(arg);
        UNUSED(data);

        not_implemented();

        return MPC_LOWCOMM_SUCCESS;
}

static int lcp_rndv_am_handler(void *arg, void *data,
                               size_t length, unsigned flags)
{
        UNUSED(flags);
        lcp_ep_h ep;
        void *user_hdr;
        size_t user_hdr_offset;
        size_t data_length;
        void *data_ptr;
	lcp_unexp_ctnr_t *ctnr;
        lcp_am_recv_param_t param;
        lcp_task_h task ;
        int rc              = MPC_LOWCOMM_SUCCESS;
        lcp_manager_h mngr  = arg;
        lcp_rndv_hdr_t *hdr = data;
        lcp_am_user_handler_t handler; 
        struct iovec *data_iov = alloca(2 * sizeof(struct iovec));
        size_t iovcnt;

        assert(flags & LCR_IFACE_AM_LAYOUT_BUFFER);
        build_eager_am_handler_iov(data, length, sizeof(lcp_rndv_hdr_t),
                                   flags, data_iov, &iovcnt);
        assert(iovcnt == 2);

        task = lcp_context_task_get(mngr->ctx, hdr->am.dest_tid);  
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP: could not find task with tid=%d length=%ld", hdr->am.dest_tid, hdr->am.hdr_size);

                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        handler = task->tcct[mngr->id]->am.handlers[hdr->am.am_id];

        //FIXME: what happen when ep is in CONNECTING state ?
        rc = lcp_ep_get_or_create(mngr, hdr->am.src_uid, &ep, 0);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = lcp_request_init_unexp_ctnr(task, &ctnr, data_iov, iovcnt,
                                         LCP_RECV_CONTAINER_UNEXP_RNDV_AM);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Reset address of hdr */
        user_hdr_offset = length - hdr->am.hdr_size;
        user_hdr = (char *)(ctnr + 1) + user_hdr_offset;

        //FIXME: no support for non contiguous data
        //FIXME: how to handle return code ?
        param.flags    = LCP_AM_RNDV;
        param.reply_ep = ep;
        data_ptr       = (char *)(ctnr + 1);
        data_length    = length - hdr->am.hdr_size - sizeof(*hdr);
        handler.cb(handler.user_arg, user_hdr, hdr->am.hdr_size,
                   data_ptr, data_length, &param);

        /* RTS is processed using the recv call */

err:
        return rc;
}

LCP_DEFINE_AM(LCP_AM_ID_EAGER_AM, lcp_eager_am_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_EAGER_AM_SYNC, lcp_eager_am_sync_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_ACK_SYNC, lcp_eager_am_sync_ack_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_RTS_AM, lcp_rndv_am_handler, 0);
