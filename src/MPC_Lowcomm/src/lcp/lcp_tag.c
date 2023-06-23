#include <alloca.h>

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_prototypes.h"
#include "lcp_task.h"
#include "lcp_datatype.h"
#include "lcp_eager.h"
#include "lcp_rndv.h"

#include "sctk_alloc.h"
#include "msg_cpy.h"

/* ============================================== */
/* Packing                                        */
/* ============================================== */

/**
 * @brief Pack a message into a header. Used also for sync since msg identifier
 *        is built on sequence number (seqn) and destination task identifier
 *        (dest_tid).
 * 
 * @param dest header
 * @param data data to pack
 * @return size_t size of packed data
 */
static size_t lcp_send_tag_eager_pack(void *dest, void *data)
{
        ssize_t packed_length;
	lcp_tag_hdr_t *hdr = dest;
	lcp_request_t *req = data;
        void *src = req->datatype == LCP_DATATYPE_CONTIGUOUS ?
               req->send.buffer : NULL; 
	
	hdr->comm     = req->send.tag.comm;
	hdr->tag      = req->send.tag.tag;
	hdr->src_tid  = req->send.tag.src_tid;
        hdr->dest_tid = req->send.tag.dest_tid;
	hdr->seqn     = req->seqn; 

        packed_length = lcp_datatype_pack(req->ctx, req, req->datatype,
                                          (void *)(hdr + 1), src, 
                                          req->send.length);

	return sizeof(*hdr) + packed_length;
}

static size_t lcp_send_tag_eager_sync_pack(void *dest, void *data)
{
        ssize_t packed_length;
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

        packed_length = lcp_datatype_pack(req->ctx, req, req->datatype,
                                          (void *)(hdr + 1), src, 
                                          req->send.length);

	return sizeof(*hdr) + packed_length;
}

/**
 * @brief Pack a message into an ack header
 * 
 * @param dest header
 * @param data data to pack
 * @return size_t size of packed data
 */
size_t lcp_send_ack_pack(void *dest, void *data)
{
	lcp_ack_hdr_t *hdr = dest;
	lcp_request_t *req = data;
	
	hdr->src    = req->send.ack.src_tid;
	hdr->msg_id = req->msg_id;

	return sizeof(*hdr);
}

static size_t lcp_send_tag_rndv_pack(void *dest, void *data) 
{
        size_t rkey_packed_size = 0;
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t  *req = data;

        hdr->tag.comm     = req->send.tag.comm;
        hdr->tag.tag      = req->send.tag.tag;
        hdr->tag.src_tid  = req->send.tag.src_tid;
        hdr->tag.dest_tid = req->send.tag.dest_tid;
        hdr->tag.seqn     = req->seqn;

        hdr->msg_id        = req->msg_id;
        hdr->size          = req->send.length;
        hdr->src_uid       = req->send.tag.src_uid;

        rkey_packed_size   = lcp_rndv_rts_pack(req, hdr + 1);

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

        //FIXME: should it be the state length actually received ?
        req->info->length = req->send.length;
        req->info->src    = req->send.tag.src_tid;
        req->info->tag    = req->send.tag.tag;

        if ((req->flags & LCP_REQUEST_REMOTE_COMPLETED) && 
            (req->flags & LCP_REQUEST_LOCAL_COMPLETED)) {
                lcp_request_complete(req);
        }
}

static void lcp_tag_recv_complete(lcr_completion_t *comp) {

	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      state.comp);

        //FIXME: should it be the state length actually received ?
        req->info->length = req->recv.send_length;
        req->info->src    = req->recv.tag.src_tid;
        req->info->tag    = req->recv.tag.tag;

	lcp_request_complete(req);
}

/* ============================================== */
/* Send                                           */
/* ============================================== */

/**
 * @brief Tag active message bcopy eager send function
 * 
 * @param req request
 * @return int LCP_SUCCESS in case of success
 */
int lcp_send_eager_tag_bcopy(lcp_request_t *req) 
{
        uint8_t am_id;
        lcr_pack_callback_t pack_cb;
        ssize_t payload;
	int rc = LCP_SUCCESS;

        mpc_common_debug_info("LCP: send am eager tag bcopy comm=%d, src=%d, "
                              "dest=%d, length=%d, tag=%d, seqn=%d, buf=%p.", 
                              req->send.tag.comm, req->send.tag.src_tid, 
                              req->send.tag.dest_tid, req->send.length, 
                              req->send.tag.tag, req->seqn, req->send.buffer);

        if (req->is_sync) {
                pack_cb = lcp_send_tag_eager_sync_pack;
                am_id   = LCP_AM_ID_EAGER_TAG_SYNC;

                req->msg_id = (uint64_t)req;
                if (lcp_pending_create(req->ctx->pend, 
                                       req, 
                                       req->msg_id) == NULL) {
                        rc = LCP_ERROR;
                }
        } else {
                pack_cb     = lcp_send_tag_eager_pack;
		am_id       = LCP_AM_ID_EAGER_TAG;
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
	}

        req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_send_complete,
        };

        payload = lcp_send_eager_bcopy(req, pack_cb, am_id);
        if (payload < 0) {
                rc = LCP_ERROR;
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
 * @return int LCP_SUCCESS in case of success
 */
int lcp_send_eager_tag_zcopy(lcp_request_t *req) 
{
	
	int rc;
        uint8_t am_id;
        void *hdr;
        size_t hdr_size;
        size_t iovcnt     = 0;
	lcp_ep_h ep       = req->send.ep;
        size_t max_iovec  = ep->ep_config.am.max_iovecs;
	struct iovec *iov = alloca(max_iovec*sizeof(struct iovec));
	
	req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_send_complete
        };

	iov[0].iov_base = req->send.buffer;
	iov[0].iov_len  = req->send.length;
        iovcnt++;

        mpc_common_debug_info("LCP: send am eager tag zcopy comm=%d, src=%d, "
                              "dest=%d, tag=%d, length=%d", req->send.tag.comm, 
                              req->send.tag.src_tid, req->send.tag.dest_tid, 
                              req->send.tag.tag, req->send.length);

	if(req->is_sync) {
                lcp_tag_sync_hdr_t *hdr_sync;
                hdr_size = sizeof(lcp_tag_sync_hdr_t);
                //FIXME: hdr never freed, how should it be initialized?
                hdr_sync = hdr = sctk_malloc(sizeof(lcp_tag_sync_hdr_t));
                if (hdr == NULL) {
                        mpc_common_debug_error("LCP: could not allocate tag header.");
                        rc = LCP_ERROR;
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
                if (lcp_pending_create(req->ctx->pend, 
                                       req, 
                                       req->msg_id) == NULL) {
                        rc = LCP_ERROR;
                }
        } else {
                lcp_tag_hdr_t *hdr_tag;
                hdr_size = sizeof(lcp_tag_hdr_t);
                //FIXME: hdr never freed, how should it be initialized?
                hdr_tag = hdr = sctk_malloc(sizeof(lcp_tag_hdr_t));
                if (hdr == NULL) {
                        mpc_common_debug_error("LCP: could not allocate tag header.");
                        rc = LCP_ERROR;
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

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;

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
	ssize_t payload;
        lcp_ep_h ep;
	lcp_request_t *ack;
        lcp_tag_sync_hdr_t *hdr = (lcp_tag_sync_hdr_t *)data;
	
        /* Get endpoint */
        rc = lcp_ep_get_or_create(super->ctx, hdr->src_uid, &ep, 0);
        if (rc != LCP_SUCCESS) {
                goto err;
        }

        rc = lcp_request_create(&ack);
        if (ack == NULL) {
                goto err;
        }
        ack->super            = super;
        ack->send.ack.src_tid = hdr->base.dest_tid;
        /* Set message identifier that will be sent back to sender so that he
         * can complete the request he stored in pending table */
        ack->msg_id = hdr->msg_id;

        mpc_common_debug("LCP TAG: send sync ack. dest_tid=%d", 
                         hdr->base.dest_tid);

        payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                       LCP_AM_ID_ACK_SYNC,
                                       lcp_send_ack_pack,
                                       ack);

        if (payload < 0) {
                return LCP_ERROR;
        }

        /* Complete ack request */
        lcp_request_complete(ack);

err:
	return rc;
}

static int lcp_send_rndv_tag_rts_progress(lcp_request_t *req)
{
        int rc = LCP_SUCCESS;
        ssize_t payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

        mpc_common_debug("LCP TAG: start rndv. src=%d, dest=%d, tag=%d",
                         req->send.tag.src_tid, req->send.tag.dest_tid,
                         req->send.tag.tag);

        payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc], 
                                            LCP_AM_ID_RTS_TAG,
                                            lcp_send_tag_rndv_pack, req);
        if (payload_size < 0) {
                rc = LCP_ERROR;
        }

        return rc;
}

int lcp_send_rndv_tag_start(lcp_request_t *req)
{
        int rc;

        /* Set both completion flags since completion will be called only once
         * upon reception of FIN message */
        req->flags |= LCP_REQUEST_LOCAL_COMPLETED | 
                LCP_REQUEST_REMOTE_COMPLETED;
	req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_send_complete
        };
        req->msg_id = (uint64_t)req;

        rc = lcp_send_rndv_start(req);
        if (rc != LCP_SUCCESS) {
                goto err;
        }

        req->send.func = lcp_send_rndv_tag_rts_progress;
        /* Register memory if GET protocol */
        rc = lcp_rndv_reg_send_buffer(req);

err:
        return rc;
}

/* ============================================== */
/* Receive                                        */
/* ============================================== */
int lcp_recv_eager_tag_data(lcp_request_t *req, void *hdr, 
                            void *data, size_t length)
{
        int rc = LCP_SUCCESS;
        lcp_tag_hdr_t *eager_hdr = (lcp_tag_hdr_t *)hdr;
        ssize_t unpacked_len = 0;

        mpc_common_debug_info("LCP: recv tag data req=%p, src=%d, dest=%d, "
                              "tag=%d, comm=%d, length=%d, seqn=%d", req, 
                              eager_hdr->src_tid, eager_hdr->dest_tid, eager_hdr->tag, 
                              eager_hdr->comm, length, eager_hdr->seqn);

        /* Set variables for MPI status */
        req->recv.tag.src_tid  = eager_hdr->src_tid;
        req->seqn              = eager_hdr->seqn;
        req->recv.tag.tag      = eager_hdr->tag;
        req->recv.send_length  = length;

        /* copy data to receiver buffer and complete request */
        unpacked_len = lcp_datatype_unpack(req->ctx, req, req->datatype, 
                                           req->recv.buffer, data,
                                           length);
        if (unpacked_len < 0) {
                rc = LCP_ERROR;
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
        req->state.comp        = (lcr_completion_t) {
                .comp_cb = lcp_tag_recv_complete,
        };
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
	int rc = LCP_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_unexp_ctnr_t *ctnr;
	lcp_request_t *req;
	lcp_tag_sync_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->base.dest_tid, &task);  
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP: could not find task with tid=%d", hdr->base.dest_tid);
                rc = LCP_ERROR;
                goto err;
        }


	LCP_TASK_LOCK(task);
	/* Try to match it with a posted message */
        req = (lcp_request_t *)lcp_match_prq(task->prq_table, 
                                             hdr->base.comm, 
                                             hdr->base.tag,
                                             hdr->base.src_tid);
	/* if request is not matched */
	if (req == NULL) {
                mpc_common_debug("LCP: recv unexp tag sync src=%d, length=%d, sequence=%d",
                                 hdr->base.src_tid, length, hdr->base.seqn);
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
                                                 LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC |
                                                 LCP_RECV_CONTAINER_UNEXP_EAGER_TAG);
		if (rc != LCP_SUCCESS) {
			goto err;
		}
		// add the request to the unexpected messages
		lcp_append_umq(task->umq_table, (void *)ctnr, 
			       hdr->base.comm,
			       hdr->base.tag,
			       hdr->base.src_tid);

		LCP_TASK_UNLOCK(task);
		return LCP_SUCCESS;
	}
	LCP_TASK_UNLOCK(task);

        rc = lcp_send_eager_sync_ack(req, hdr);
        if (rc != LCP_SUCCESS) {
                goto err;
        }

        /* Complete request */
        lcp_recv_eager_tag_data(req, hdr, hdr + 1, length - sizeof(*hdr));
err:
	return rc;
}


static int lcp_eager_tag_handler(void *arg, void *data,
                                 size_t length,
                                 __UNUSED__ unsigned flags)
{
        int rc = LCP_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_unexp_ctnr_t *ctnr;
        lcp_request_t *req;
        lcp_tag_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->dest_tid, &task);  
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP: could not find task with tid=%d", hdr->dest_tid);

                rc = LCP_ERROR;
                goto err;
        }


	LCP_TASK_LOCK(task);
	/* Try to match it with a posted message */
        req = (lcp_request_t *)lcp_match_prq(task->prq_table, 
                                             hdr->comm, 
                                             hdr->tag,
                                             hdr->src_tid);
	/* if request is not matched */
	if (req == NULL) {
                mpc_common_debug_info("LCP: recv unexp tag src=%d, tag=%d, dest=%d, "
                                      "length=%d, sequence=%d", hdr->src_tid, 
                                      hdr->tag, hdr->dest_tid, length - sizeof(lcp_tag_hdr_t), 
                                      hdr->seqn);
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
                                                 LCP_RECV_CONTAINER_UNEXP_EAGER_TAG);
		if (rc != LCP_SUCCESS) {
			goto err;
		}
		// add the request to the unexpected messages
		lcp_append_umq(task->umq_table, (void *)ctnr, 
			       hdr->comm,
			       hdr->tag,
			       hdr->src_tid);

		LCP_TASK_UNLOCK(task);
		return LCP_SUCCESS;
	}
	LCP_TASK_UNLOCK(task);
		
        /* Complete request */
        lcp_recv_eager_tag_data(req, hdr, hdr + 1, length - sizeof(*hdr));
err:
	return rc;
}

static int lcp_eager_tag_sync_ack_handler(void *arg, void *data,
                                          size_t length, 
                                          unsigned flags)
{
        UNUSED(length);
        UNUSED(flags);
	int rc = LCP_SUCCESS;
	
	lcp_request_t *req;
	lcp_context_h ctx = arg;
	lcp_ack_hdr_t *hdr = (lcp_ack_hdr_t *)data;

        mpc_common_debug_info("LCP: recv ack header src=%d, msg_id=%lu",
                              hdr->src, hdr->msg_id);
	/* Retrieve request */
	LCP_CONTEXT_LOCK(ctx);
	req = lcp_pending_get_request(ctx->pend, hdr->msg_id);
	if (req == NULL) {
                mpc_common_debug_error("LCP: could not find ack ctrl msg: "
                                       "msg id=%llu.", hdr->msg_id);
			rc = LCP_ERROR;
			goto err;
	}
	LCP_CONTEXT_UNLOCK(ctx);

        /* Set remote flag so that request can be actually completed */
	req->flags |= LCP_REQUEST_REMOTE_COMPLETED;

        lcp_pending_delete(req->ctx->pend, req->msg_id);
	lcp_tag_send_complete(&(req->state.comp));
err:
        return rc;
}

static int lcp_rndv_tag_handler(void *arg, void *data,
                                size_t length, unsigned flags)
{
        UNUSED(flags);
	int rc = LCP_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;
	lcp_rndv_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->tag.dest_tid, &task);  
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP: could not find task with tid=%d", hdr->tag.dest_tid);
                rc = LCP_ERROR;
                goto err;
        }

        LCP_TASK_LOCK(task);
        mpc_common_debug("LCP: task match. tid=%d", hdr->tag.dest_tid);
	req = (lcp_request_t *)lcp_match_prq(task->prq_table, 
					     hdr->tag.comm, 
					     hdr->tag.tag,
					     hdr->tag.src_tid);
	if (req == NULL) {
                mpc_common_debug("LCP: recv unexp tag src=%d, comm=%d, tag=%d, length=%d, seqn=%d",
                                 hdr->tag.src_tid, hdr->tag.comm, hdr->tag.tag, length, hdr->tag.seqn);
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
						 LCP_RECV_CONTAINER_UNEXP_RNDV_TAG);
		if (rc != LCP_SUCCESS) {
                        LCP_TASK_UNLOCK(task);
			goto err;
		}
		lcp_append_umq(task->umq_table, (void *)ctnr, 
			       hdr->tag.comm,
			       hdr->tag.tag,
			       hdr->tag.src_tid);

		LCP_TASK_UNLOCK(task);
		return rc;
	}
	LCP_TASK_UNLOCK(task);

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
