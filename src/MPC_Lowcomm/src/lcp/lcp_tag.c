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

#include <alloca.h>
#include <stdint.h>

#include "lcp_tag.h"

#include "lcp.h"
#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_task.h"
#include "lcp_datatype.h"
#include "lcp_eager.h"
#include "lcp_rndv.h"
#include "lcp_tag_match.h"

#include "mpc_common_debug.h"

/* ============================================== */
/* Packing                                        */
/* ============================================== */

/* Memory layout for TAG API after packing and just before sending message.
 * Unexpected messages are encapsulated with lcp_unexp_ctnr_t to shaddow buffers.
 *
 * Eager:
 *  ----------------------------------------------------------- -- - -
 *  | transport   |  lcp_tag_hdr |
 *  |  hdr        |  (tag, rndv, |  user data
 *  |             |   sync)      |
 *  |             |              |
 *  ----------------------------------------------------------- -- - -
 *                ^
 *                |
 *               dest
 *
 * Rendez-vous:
 *  ----------------------------------------------------------- -- - -
 *  | transport   |                  |                |
 *  |  hdr        |  lcp_rndv_hdr_t  |  remote key    | user data
 *  |             |                  |  (bmap + pin)  |
 *  |             |                  |                |
 *  ----------------------------------------------------------- -- - -
 *                ^
 *                |
 *               dest
*/

/**
 * @brief Pack a message into a header. Used also for sync since msg identifier
 *        is built on sequence number (seqn) and destination task identifier
 *        (dest_tid).
 *
 * @param dest header
 * @param data data to pack
 * @return size_t size of packed data
 */
static ssize_t lcp_send_tag_eager_pack(void *dest, void *data)
{
        ssize_t packed_length = 0;
        ssize_t packed_payload_length = 0;
	lcp_tag_hdr_t *hdr = dest;
	lcp_request_t *req = data;
        void *src = req->datatype == LCP_DATATYPE_CONTIGUOUS ?
               req->send.buffer : NULL; 
	
	hdr->comm      = req->send.tag.comm;
	hdr->tag       = req->send.tag.tag;
	hdr->src_tid   = req->send.tag.src_tid;
        hdr->dest_tid  = req->send.tag.dest_tid;
	hdr->seqn      = req->seqn; 

        packed_length += sizeof(*hdr);

        packed_payload_length = lcp_datatype_pack(req->mngr->ctx, req, req->datatype,
                                                   (void *)(hdr + 1), src, 
                                                   req->send.length);
        if (packed_payload_length < 0) {
                packed_length = packed_payload_length;
        } else {
                packed_length += packed_payload_length;
        }

	return packed_length;
}

static ssize_t lcp_send_tag_eager_sync_pack(void *dest, void *data)
{
        ssize_t packed_length = 0;
        ssize_t packed_payload_length = 0;
	lcp_tag_sync_hdr_t *hdr = dest;
	lcp_request_t *req = data;
        void *src = req->datatype == LCP_DATATYPE_CONTIGUOUS ?
               req->send.buffer : NULL;

	hdr->base.comm     = req->send.tag.comm;
	hdr->base.tag      = req->send.tag.tag;
	hdr->base.src_tid  = req->send.tag.src_tid;
        hdr->base.dest_tid = req->send.tag.dest_tid;
	hdr->base.seqn     = req->seqn;
        hdr->msg_id        = req->msg_id;
        hdr->src_uid       = req->send.tag.src_uid;
        
        packed_length += sizeof(*hdr);

        packed_payload_length = lcp_datatype_pack(req->mngr->ctx, req, req->datatype,
                                          (void *)(hdr + 1), src, 
                                          req->send.length);
        if (packed_payload_length < 0) {
                packed_length = packed_payload_length;
        } else {
                packed_length += packed_payload_length;
        }

	return packed_length;
}

/**
 * @brief Pack a message into an ack header
 *
 * @param dest header
 * @param data data to pack
 * @return size_t size of packed data
 */
ssize_t lcp_send_ack_pack(void *dest, void *data)
{
	lcp_ack_hdr_t *hdr = dest;
	lcp_request_t *req = data;

	hdr->src    = req->send.ack.src_tid;
	hdr->msg_id = req->msg_id;

	return sizeof(*hdr);
}

static ssize_t lcp_send_tag_rndv_pack(void *dest, void *data) 
{
        size_t rkey_packed_size = 0;
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t  *req = data;

        hdr->tag.comm     = req->send.tag.comm;
        hdr->tag.tag      = req->send.tag.tag;
        hdr->tag.src_tid  = req->send.tag.src_tid;
        hdr->tag.dest_tid = req->send.tag.dest_tid;
        hdr->tag.seqn     = req->seqn;

        hdr->src_uid      = req->send.tag.src_uid;

        rkey_packed_size  = lcp_rndv_rts_pack(req, hdr);

        return sizeof(*hdr) + rkey_packed_size;
}

/* ============================================== */
/* Completion                                     */
/* ============================================== */

/**
 * @brief Complete a request by its tag
 *
 * @param comp completion
 */
static void lcp_tag_send_complete(lcr_completion_t *comp) {

	lcp_request_t *req = mpc_container_of(comp, lcp_request_t,
					      state.comp);

        if ((req->flags & LCP_REQUEST_REMOTE_COMPLETED) && 
            (req->flags & LCP_REQUEST_LOCAL_COMPLETED)) {
                req->status = MPC_LOWCOMM_SUCCESS;
                lcp_request_complete(req, send.send_cb, req->status, req->send.length);
        }
}

static void lcp_tag_recv_complete(lcr_completion_t *comp) {

	lcp_request_t *req = mpc_container_of(comp, lcp_request_t,
					      state.comp);

        req->recv.tag.info.length = req->recv.send_length;
        req->recv.tag.info.src    = req->recv.tag.src_tid;
        req->recv.tag.info.tag    = req->recv.tag.tag;

	lcp_request_complete(req, recv.tag.recv_cb, req->status, 
                             &req->recv.tag.info);
}

static void lcp_tag_task_bcopy_complete(lcp_task_completion_t *self) {
        lcp_request_t *sreq = self->sreq;
        lcp_request_t *mreq = self->mreq;

        intptr_t hdr_offset = sreq->is_sync ? sizeof(lcp_tag_sync_hdr_t) :
                sizeof(lcp_tag_hdr_t);

        //NOTE: this completion is called only when requests were matched.

        /* Set variables for MPI status */
        mreq->recv.tag.src_tid  = sreq->send.tag.src_tid;
        mreq->seqn              = sreq->seqn;
        mreq->recv.tag.tag      = sreq->send.tag.tag;
        mreq->recv.send_length  = sreq->send.length;

        lcp_recv_eager_tag_data(mreq, self->buffer + hdr_offset);

        //NOTE: complete send request only if sync request. For simple bcopy,
        //      send request has already been completed.
        if (!(sreq->flags & LCP_REQUEST_REMOTE_COMPLETED)) {
                sreq->flags |= LCP_REQUEST_REMOTE_COMPLETED;
                lcp_tag_send_complete(&sreq->state.comp);
        }
}

static void lcp_tag_task_zcopy_complete(lcp_task_completion_t *self) {
        lcp_request_t *sreq = self->sreq; /* send request    */
        lcp_request_t *mreq = self->mreq; /* matched request */

        //NOTE: this completion is called only when requests were matched.

        /* Set variables for MPI status */
        mreq->recv.tag.src_tid  = sreq->send.tag.src_tid;
        mreq->seqn              = sreq->seqn;
        mreq->recv.tag.tag      = sreq->send.tag.tag;
        mreq->recv.send_length  = sreq->send.length;

        lcp_recv_eager_tag_data(mreq, (void *)sreq->send.buffer);

        /* Since recv request complete, set send remote complete */
        sreq->flags |= LCP_REQUEST_REMOTE_COMPLETED;
        //FIXME: should we avoid this function call?
        lcp_tag_send_complete(&sreq->state.comp);
}

/* ============================================== */
/* Send                                           */
/* ============================================== */

//NOTE: For zcopy protocol, because there could be races and hazardous multiple
//      completion on the same request, thread who matches takes ownership of both
//      receive AND send requests. As a consequence, he is responsible to complete
//      them both. In addition, zcopy protocol enforce a synchronization since
//      the send request can only be completed if matched has happened.
int lcp_send_task_tag_zcopy(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;

        mpc_common_debug_info("LCP TAG: send am task tag zcopy comm=%d, src=%d, "
                              "dest=%d, tag=%d, length=%d, seqn=%d, req=%p", 
                              req->send.tag.comm, req->send.tag.src_tid, 
                              req->send.tag.dest_tid, req->send.tag.tag, 
                              req->send.length, req->seqn, req);

        //NOTE: setting a header structure within task_completion_t is necessary
        //      for the matching mechanism is case of eager send (message
        //      inserted in unexpected message queue.
        req->state.tcomp = (lcp_task_completion_t) {
                .hdr     = {
                        .src_tid  = req->send.tag.src_tid,
                        .tag      = req->send.tag.tag,
                },
                .sreq    = req,
                .mreq    = NULL,
                .comp_cb = lcp_tag_task_zcopy_complete,
        };

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;

        /* For zcopy, in case of unexpected send, pointer to send request is
         * needed to get back the send buffer */
        req->msg_id = (uint64_t)req;

        rc = lcp_send_task_zcopy(req, &req->state.tcomp);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP TAG: could not send task zcopy request");
                goto err;
        }

err:
        return rc;
}

int lcp_send_task_tag_bcopy(lcp_request_t *req)
{
        mpc_common_debug_info("LCP TAG: send am task tag bcopy comm=%d, src=%d, "
                              "dest=%d, tag=%d, length=%d, seqn=%d, req=%p", 
                              req->send.tag.comm, req->send.tag.src_tid, 
                              req->send.tag.dest_tid, req->send.tag.tag, 
                              req->send.length, req->seqn, req);

        int rc          = MPC_LOWCOMM_SUCCESS;
        unsigned flags  = 0;
        lcr_pack_callback_t pack_cb;

        req->state.tcomp = (lcp_task_completion_t) {
                .sreq    = req,
                .mreq    = NULL,
                .comp_cb = lcp_tag_task_bcopy_complete,
        };

        if (req->is_sync) {
                req->msg_id = (uint64_t)req;
                pack_cb     = lcp_send_tag_eager_sync_pack;
                flags      |= LCP_RECV_CONTAINER_UNEXP_TASK_TAG_ZCOPY;
                //NOTE: REMOTE_COMPLETED flag set whenever ack has been received
                //      or task send matched, see below.
        } else {
                pack_cb     = lcp_send_tag_eager_pack;
                flags      |= LCP_RECV_CONTAINER_UNEXP_TASK_TAG_BCOPY;
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
        }

        rc = lcp_send_task_bcopy(req, pack_cb, flags, &req->state.tcomp);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug_error("LCP TAG: could not send task bcopy request");
                goto err;
        }

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;
        lcp_tag_send_complete(&(req->state.comp));

err:
        return rc;
}

/**
 * @brief Tag active message bcopy eager send function
 *
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_eager_tag_bcopy(lcp_request_t *req)
{
        uint8_t am_id;
        lcr_pack_callback_t pack_cb;
        ssize_t payload;
	int rc = MPC_LOWCOMM_SUCCESS;

        mpc_common_debug_info("LCP TAG: send eager tag bcopy comm=%d, src=%d, "
                              "dest=%d, length=%d, tag=%d, seqn=%d, buf=%p, req=%p.", 
                              req->send.tag.comm, req->send.tag.src_tid, 
                              req->send.tag.dest_tid, req->send.length, 
                              req->send.tag.tag, req->seqn, req->send.buffer, req);

        req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_send_complete,
        };

        if (req->is_sync) {
                pack_cb = lcp_send_tag_eager_sync_pack;
                am_id   = LCP_AM_ID_EAGER_TAG_SYNC;

                req->msg_id = (uint64_t)req;
                //NOTE: REMOTE_COMPLETED flag set whenever ack has been
                //received.
        } else {
                pack_cb     = lcp_send_tag_eager_pack;
                am_id       = LCP_AM_ID_EAGER_TAG;
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
        }

        payload = lcp_send_eager_bcopy(req, pack_cb, am_id);
        if (payload < 0) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;
        lcp_tag_send_complete(&(req->state.comp));

err:
	return rc;
}

/**
 * @brief Tag active message zcopy eager send function
 *
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_eager_tag_zcopy(lcp_request_t *req)
{

	int rc;
        uint8_t am_id;
        void *hdr;
        size_t hdr_size;
        size_t iovcnt     = 0;
	lcp_ep_h ep       = req->send.ep;
        size_t max_iovec  = ep->config.am.max_iovecs;
	struct iovec *iov = alloca(max_iovec*sizeof(struct iovec));

	req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_send_complete
        };

        mpc_common_debug_info("LCP TAG: send am eager tag zcopy comm=%d, src=%d, "
                              "dest=%d, tag=%d, length=%d", req->send.tag.comm, 
                              req->send.tag.src_tid, req->send.tag.dest_tid, 
                              req->send.tag.tag, req->send.length);

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;

        iov[0].iov_base = (void *)req->send.buffer;
        iov[0].iov_len  = req->send.length;
        iovcnt++;

        if(req->is_sync) {
                lcp_tag_sync_hdr_t *hdr_sync;
                hdr_size = sizeof(lcp_tag_sync_hdr_t);
                //FIXME: hdr never freed, how should it be initialized?
                hdr_sync = hdr = sctk_malloc(sizeof(lcp_tag_sync_hdr_t));
                if (hdr == NULL) {
                        mpc_common_debug_error("LCP TAG: could not allocate tag header.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }

                req->msg_id = (uint64_t)req;

                hdr_sync->base.comm     = req->send.tag.comm;
                hdr_sync->base.src_tid  = req->send.tag.src_tid;
                hdr_sync->base.dest_tid = req->send.tag.dest_tid;
                hdr_sync->base.tag      = req->send.tag.tag;
                hdr_sync->base.seqn     = req->seqn;
                hdr_sync->msg_id        = req->msg_id;
                hdr_sync->src_uid       = req->send.tag.src_uid;

                am_id = LCP_AM_ID_EAGER_TAG_SYNC;
        } else {
                lcp_tag_hdr_t *hdr_tag;
                hdr_size = sizeof(lcp_tag_hdr_t);
                //FIXME: hdr never freed, how should it be initialized?
                hdr_tag = hdr = sctk_malloc(sizeof(lcp_tag_hdr_t));
                if (hdr == NULL) {
                        mpc_common_debug_error("LCP TAG: could not allocate tag header.");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }

                hdr_tag->comm     = req->send.tag.comm;
                hdr_tag->src_tid  = req->send.tag.src_tid;
                hdr_tag->dest_tid = req->send.tag.dest_tid;
                hdr_tag->tag      = req->send.tag.tag;
                hdr_tag->seqn     = req->seqn;

                am_id       = LCP_AM_ID_EAGER_TAG;
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
        }

        rc = lcp_send_eager_zcopy(req, am_id,
                                  hdr, hdr_size,
                                  iov, iovcnt,
                                  &(req->state.comp));
err:
	return rc;
}

int lcp_send_eager_sync_ack(lcp_request_t *super, void *data)
{
        int rc;
	ssize_t payload_size;
        lcp_ep_h ep;
	lcp_request_t *ack;
        lcp_tag_sync_hdr_t *hdr = (lcp_tag_sync_hdr_t *)data;

        /* Get endpoint */
        rc = lcp_ep_get_or_create(super->mngr, hdr->src_uid, &ep, 0);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        ack = lcp_request_get(super->task);
        if (ack == NULL) {
                goto err;
        }
        ack->super            = super;
        ack->send.ack.src_tid = hdr->base.dest_tid;
        ack->send.ep          = ep;
        /* Set message identifier that will be sent back to sender so that he
         * can complete the request he stored in pending table */
        ack->msg_id = hdr->msg_id;

        mpc_common_debug("LCP TAG: send sync ack. dest_tid=%d",
                         hdr->base.dest_tid);

        payload_size = lcp_send_eager_bcopy(ack, lcp_send_ack_pack, 
                                            LCP_AM_ID_ACK_SYNC);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        /* Complete ack request */
        lcp_request_complete(ack, send.send_cb, MPC_LOWCOMM_SUCCESS, 
                             ack->send.length);

err:
	return rc;
}

static int lcp_send_rndv_tag_rts_progress(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t payload_size;

        mpc_common_debug("LCP TAG: send rndv ready-to-send bcopy (uses am). comm=%d, src=%d, "
                         "dest=%d, tag=%d, length=%d, seqn=%d, buf=%p, req=%p",
                         req->send.tag.comm, req->send.tag.src_tid,
                         req->send.tag.dest_tid, req->send.tag.tag,
                         req->send.length, req->seqn, req->send.buffer, req);

        payload_size = lcp_send_eager_bcopy(req, lcp_send_tag_rndv_pack,
                                            LCP_AM_ID_RTS_TAG);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
        }

        return rc;
}

int lcp_send_rndv_tag_start(lcp_request_t *req)
{
        int rc;

        //FIXME: effectively set REMOTE_COMPLETED upon reception of the FIN
        //       message.
        req->flags |= LCP_REQUEST_LOCAL_COMPLETED |
                LCP_REQUEST_REMOTE_COMPLETED;
	req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_send_complete
        };

        rc = lcp_send_rndv_start(req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        req->send.func = lcp_send_rndv_tag_rts_progress;
err:
        return rc;
}

/* ============================================== */
/* Receive                                        */
/* ============================================== */

//FIXME: set truncate setting the status of the request is not adequate.
static inline size_t lcp_recv_set_truncate(lcp_request_t *req) {
        size_t length = req->recv.send_length;
        /* Check for truncation. */
        if (req->recv.send_length > req->recv.length) {
                length = req->recv.length;
                req->status = MPC_LOWCOMM_ERR_TRUNCATE;
                mpc_common_debug_warning("LCP TAG: request truncated.");
        } else {
                req->status = MPC_LOWCOMM_SUCCESS;
        }

        return length;
}

//FIXME: tag data reception should be factorized between task, rndv, and eager.
int lcp_recv_eager_tag_data(lcp_request_t *req, void *data)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t unpacked_len = 0;

        mpc_common_debug_info("LCP TAG: recv tag data req=%p, src=%d, dest=%d, "
                              "tag=%d, comm=%d, length=%d, seqn=%d", req, 
                              req->recv.tag.src_tid, req->recv.tag.dest_tid, 
                              req->recv.tag.tag, req->recv.tag.comm, 
                              req->recv.send_length, req->seqn);

        /* Overwrite send length in case truncation is needed. */
        req->recv.send_length = lcp_recv_set_truncate(req);

        /* copy data to receiver buffer and complete request */
        unpacked_len = lcp_datatype_unpack(req->mngr->ctx, req, req->datatype, 
                                           req->recv.buffer, data,
                                           req->recv.send_length);
        if (unpacked_len < 0) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        lcp_tag_recv_complete(&(req->state.comp));

err:
        return rc;
}

void lcp_recv_rndv_tag_data(lcp_request_t *req, void *data)
{
        lcp_rndv_hdr_t *hdr = (lcp_rndv_hdr_t *)data;

        req->recv.tag.tag      = hdr->tag.tag;
        req->recv.tag.src_tid  = hdr->tag.src_tid;
        req->recv.tag.src_uid  = hdr->src_uid;
        req->recv.tag.dest_tid = hdr->tag.dest_tid;
        req->seqn              = hdr->tag.seqn;
        req->recv.send_length  = hdr->size;
        //FIXME: msg_id for rndv is set to address of the rendez-vous request
        //       through the hdr member msg_id. However, this is protocol logic
        //       and should not appear here.
        req->msg_id            = hdr->msg_id;
        req->state.comp        = (lcr_completion_t) {
                .comp_cb = lcp_tag_recv_complete,
        };

        /* Overwrite send length in case truncation is needed. */
        req->recv.send_length = lcp_recv_set_truncate(req);
}

/* ============================================== */
/* Handlers                                       */
/* ============================================== */
/**
 * @brief Handler for receiving an active tag message
 *
 * @param arg context
 * @param data header
 * @param length length of the data
 * @param flags flag of the message
 * @return int
 */
static int lcp_eager_tag_sync_handler(void *arg, void *data,
                                      size_t length,
                                      __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_manager_h mngr = arg;
	lcp_unexp_ctnr_t *ctnr;
	lcp_request_t *req;
	lcp_tag_sync_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        task = lcp_context_task_get(mngr->ctx, hdr->base.dest_tid);  
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP TAG: could not find task with tid=%d", hdr->base.dest_tid);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }


	LCP_TASK_LOCK(task);
	/* Try to match it with a posted message */
        req = (lcp_request_t *)lcp_match_prqueue(task->tcct[mngr->id]->tag.prqs,
                                                 hdr->base.comm, 
                                                 hdr->base.tag,
                                                 hdr->base.src_tid);
	/* if request is not matched */
	if (req == NULL) {
                mpc_common_debug("LCP TAG: recv unexp tag sync src=%d, length=%d, sequence=%d",
                                 hdr->base.src_tid, length, hdr->base.seqn);
		rc = lcp_request_init_unexp_ctnr(task, &ctnr, hdr, length,
                                                 LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			goto err;
		}
		// add the request to the unexpected messages
                lcp_append_umqueue(task->tcct[mngr->id]->tag.umqs, &ctnr->elem, 
                                   hdr->base.comm);

		LCP_TASK_UNLOCK(task);
		return MPC_LOWCOMM_SUCCESS;
	}
	LCP_TASK_UNLOCK(task);

        rc = lcp_send_eager_sync_ack(req, hdr);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Set variables for MPI status */
        req->recv.tag.src_tid  = hdr->base.src_tid;
        req->seqn              = hdr->base.seqn;
        req->recv.tag.tag      = hdr->base.tag;
        req->recv.send_length  = length - sizeof(lcp_tag_sync_hdr_t);

        /* Complete request */
        lcp_recv_eager_tag_data(req, hdr + 1);
err:
	return rc;
}

static int lcp_eager_tag_handler(void *arg, void *data,
                                 size_t length,
                                 __UNUSED__ unsigned flags)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_manager_h mngr = arg;
        lcp_unexp_ctnr_t *ctnr;
        lcp_request_t *req;
        lcp_tag_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        task = lcp_context_task_get(mngr->ctx, hdr->dest_tid);  
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP TAG: could not find task with tid=%d", hdr->dest_tid);

                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }


	LCP_TASK_LOCK(task);
	/* Try to match it with a posted message */
        req = lcp_match_prqueue(task->tcct[mngr->id]->tag.prqs, 
                                hdr->comm, 
                                hdr->tag,
                                hdr->src_tid);
	/* if request is not matched */
	if (req == NULL) {
                mpc_common_debug_info("LCP TAG: recv unexp tag src=%d, tag=%d, dest=%d, "
                                      "length=%d, sequence=%d", hdr->src_tid, 
                                      hdr->tag, hdr->dest_tid, length - sizeof(lcp_tag_hdr_t), 
                                      hdr->seqn);
		rc = lcp_request_init_unexp_ctnr(task, &ctnr, hdr, length,
                                                 LCP_RECV_CONTAINER_UNEXP_EAGER_TAG);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			goto err;
		}
		// add the request to the unexpected messages
		lcp_append_umqueue(task->tcct[mngr->id]->tag.umqs, &ctnr->elem, 
			       hdr->comm);

		LCP_TASK_UNLOCK(task);
		return MPC_LOWCOMM_SUCCESS;
	}
	LCP_TASK_UNLOCK(task);

        /* Set variables for MPI status */
        req->recv.tag.src_tid  = hdr->src_tid;
        req->seqn              = hdr->seqn;
        req->recv.tag.tag      = hdr->tag;
        req->recv.send_length  = length - sizeof(lcp_tag_hdr_t);

        /* Complete request */
        lcp_recv_eager_tag_data(req, hdr + 1);
err:
	return rc;
}

static int lcp_eager_tag_sync_ack_handler(void *arg, void *data,
                                          size_t length,
                                          unsigned flags)
{
        UNUSED(arg);
        UNUSED(length);
        UNUSED(flags);
	int rc = MPC_LOWCOMM_SUCCESS;
	
	lcp_request_t *req;
	lcp_ack_hdr_t *hdr = (lcp_ack_hdr_t *)data;

        mpc_common_debug_info("LCP TAG: recv ack header src=%d, msg_id=%lu",
                              hdr->src, hdr->msg_id);

	/* Retrieve request */
	req = (lcp_request_t *)(hdr->msg_id);
	if (req == NULL) {
                mpc_common_debug_error("LCP TAG: could not find ack ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
			rc = MPC_LOWCOMM_ERROR;
			goto err;
	}

        /* Set remote flag so that request can be actually completed */
	req->flags |= LCP_REQUEST_REMOTE_COMPLETED;

	lcp_tag_send_complete(&(req->state.comp));
err:
        return rc;
}

static int lcp_rndv_tag_handler(void *arg, void *data,
                                size_t length, unsigned flags)
{
        UNUSED(flags);
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_manager_h mngr = arg;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;
	lcp_rndv_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        task = lcp_context_task_get(mngr->ctx, hdr->tag.dest_tid);
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP TAG: could not find task with tid=%d", hdr->tag.dest_tid);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        LCP_TASK_LOCK(task);
        req = lcp_match_prqueue(task->tcct[mngr->id]->tag.prqs, 
                                hdr->tag.comm, 
                                hdr->tag.tag,
                                hdr->tag.src_tid);
	if (req == NULL) {
                mpc_common_debug("LCP TAG: recv unexp rndv tag src=%d, comm=%d, tag=%d, "
                                 "length=%d, seqn=%d", hdr->tag.src_tid, hdr->tag.comm, 
                                 hdr->tag.tag, hdr->size, hdr->tag.seqn);
		rc = lcp_request_init_unexp_ctnr(task, &ctnr, hdr, length,
						 LCP_RECV_CONTAINER_UNEXP_RNDV_TAG);
		if (rc != MPC_LOWCOMM_SUCCESS) {
                        LCP_TASK_UNLOCK(task);
			goto err;
		}
                lcp_append_umqueue(task->tcct[mngr->id]->tag.umqs, &ctnr->elem, 
                                   hdr->tag.comm);

		LCP_TASK_UNLOCK(task);
		return rc;
	}
	LCP_TASK_UNLOCK(task);

        mpc_common_debug("LCP TAG: recv exp rndv tag src=%d, comm=%d, tag=%d, "
                         "length=%d, seqn=%d", hdr->tag.src_tid, hdr->tag.comm, 
                         hdr->tag.tag, hdr->size, hdr->tag.seqn);

        /* Fill up receive request */
        lcp_recv_rndv_tag_data(req, hdr);
        assert(req->recv.tag.comm == hdr->tag.comm);

        rc = lcp_rndv_process_rts(req, hdr, length - sizeof(*hdr));
err:
        return rc;
}

LCP_DEFINE_AM(LCP_AM_ID_EAGER_TAG, lcp_eager_tag_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_EAGER_TAG_SYNC, lcp_eager_tag_sync_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_ACK_SYNC, lcp_eager_tag_sync_ack_handler, 0);
LCP_DEFINE_AM(LCP_AM_ID_RTS_TAG, lcp_rndv_tag_handler, 0);
