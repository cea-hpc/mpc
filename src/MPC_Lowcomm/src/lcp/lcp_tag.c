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
	
	hdr->src    = req->send.tag.src_tid;
	hdr->msg_id = req->msg_id;

	return sizeof(*hdr);
}

static size_t lcp_send_tag_rndv_pack(void *dest, void *data) 
{
        size_t rkey_packed_size = 0;
	lcp_rndv_hdr_t *hdr = dest;
	lcp_request_t  *req = data;

        hdr->base.comm     = req->send.tag.comm;
        hdr->base.tag      = req->send.tag.tag;
        hdr->base.src_tid  = req->send.tag.src_tid;
        hdr->base.dest_tid = req->send.tag.dest_tid;
        hdr->base.seqn     = req->seqn;

        hdr->size          = req->send.length;
        hdr->src_pid       = req->send.tag.src_pid;

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
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_eager_tag_bcopy(lcp_request_t *req) 
{
        uint8_t am_id;
        lcr_pack_callback_t pack_cb;
        ssize_t payload;
	int rc = MPC_LOWCOMM_SUCCESS;

	mpc_common_debug_info("LCP: send am eager tag bcopy comm=%d, src=%d, "
                              "dest=%d, length=%d, tag=%d, lcreq=%p.", req->send.tag.comm,
                              req->send.tag.src_tid, req->send.tag.dest_tid, 
                              req->send.length, req->send.tag.tag, req->request);

        pack_cb = lcp_send_tag_eager_pack;
        if (req->is_sync) {
                am_id   = MPC_LOWCOMM_P2P_SYNC_MESSAGE;
        } else {
		am_id       = MPC_LOWCOMM_P2P_MESSAGE;
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
	}

        req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_send_complete,
        };

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
        size_t iovcnt     = 0;
	lcp_ep_h ep       = req->send.ep;
        size_t max_iovec  = ep->ep_config.am.max_iovecs;
	struct iovec *iov = alloca(max_iovec*sizeof(struct iovec));
	
        //FIXME: hdr never freed, how should it be initialized?
	lcp_tag_hdr_t *hdr = sctk_malloc(sizeof(lcp_tag_hdr_t));
        if (hdr == NULL) {
                mpc_common_debug_error("LCP: could not allocate tag header.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        hdr->comm     = req->send.tag.comm;
        hdr->src_tid  = req->send.tag.src_tid;
        hdr->dest_tid = req->send.tag.dest_tid;
        hdr->tag      = req->send.tag.tag;
        hdr->seqn     = req->seqn;

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
                am_id = MPC_LOWCOMM_P2P_SYNC_MESSAGE;
                req->flags |= LCP_REQUEST_DELETE_FROM_PENDING;
        } else {
                am_id       = MPC_LOWCOMM_P2P_MESSAGE;
                req->flags |= LCP_REQUEST_REMOTE_COMPLETED;
        }

        rc = lcp_send_eager_zcopy(req, am_id, 
                                  hdr, sizeof(lcp_tag_hdr_t),
                                  iov, iovcnt, 
                                  &(req->state.comp));

        req->flags |= LCP_REQUEST_LOCAL_COMPLETED;
err:
	return rc;
}

int lcp_send_eager_sync_ack(lcp_request_t *super, void *data)
{
        int rc;
	ssize_t payload;
        lcp_ep_h ep;
	lcp_request_t *ack;
        lcp_tag_hdr_t *hdr = (lcp_tag_hdr_t *)data;
	
        /* Get endpoint */
        uint64_t puid = lcp_get_process_uid(super->ctx->process_uid,
                                            super->recv.tag.src_pid);
	
        rc = lcp_ep_get_or_create(super->ctx, puid, &ep, 0);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = lcp_request_create(&ack);
        if (ack == NULL) {
                goto err;
        }
        ack->super            = super;
        ack->send.tag.src_tid = hdr->dest_tid;
        /* Set message identifier that will be sent back to sender so that he
         * can complete the request he stored in pending table */
        LCP_REQUEST_SET_MSGID(ack->msg_id, hdr->dest_tid,
                              hdr->seqn);

        mpc_common_debug("LCP TAG: send sync ack. dest_tid=%d", hdr->dest_tid);

        payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
                                       MPC_LOWCOMM_P2P_ACK_MESSAGE,
                                       lcp_send_ack_pack,
                                       ack);

        if (payload < 0) {
                return MPC_LOWCOMM_ERROR;
        }

        /* Complete ack request */
        lcp_request_complete(ack);

err:
	return rc;
}

static int lcp_send_rndv_tag_rts_progress(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

        mpc_common_debug("LCP TAG: start rndv. src=%d, dest=%d, tag=%d",
                         req->send.tag.src_tid, req->send.tag.dest_tid,
                         req->send.tag.tag);

        payload_size = lcp_send_do_am_bcopy(ep->lct_eps[cc], 
                                            MPC_LOWCOMM_RDV_MESSAGE,
                                            lcp_send_tag_rndv_pack, req);
        if (payload_size < 0) {
                rc = MPC_LOWCOMM_ERROR;
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

        rc = lcp_send_rndv_start(req);
        if (rc != MPC_LOWCOMM_SUCCESS) {
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
int lcp_recv_eager_tag_data(lcp_request_t *req, void *data,
                            size_t length)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_tag_hdr_t *hdr = (lcp_tag_hdr_t *)data;
        ssize_t unpacked_len = 0;

        mpc_common_debug("LCP: recv tag data req=%p, src=%d, "
                         "tag=%d, comm=%d", req, hdr->src_tid, hdr->tag, 
                         hdr->comm);

        /* Set variables for MPI status */
        req->recv.tag.src_tid  = hdr->src_tid;
        req->seqn              = hdr->seqn;
        req->recv.tag.tag      = hdr->tag;

        /* copy data to receiver buffer and complete request */
        unpacked_len = lcp_datatype_unpack(req->ctx, req, req->datatype, 
                                           req->recv.buffer, (void *)(hdr + 1),
                                           length);
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

        req->recv.tag.tag = hdr->base.tag;
        req->recv.tag.src_tid = hdr->base.src_tid;
        req->seqn = hdr->base.seqn;
        req->recv.send_length = hdr->size;
        req->state.comp = (lcr_completion_t) {
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
static int _lcp_eager_tag_handler(void *arg, void *data,
                                  size_t length, int is_sync,
                                  __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_unexp_ctnr_t *ctnr;
	lcp_request_t *req;
	lcp_tag_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->dest_tid, &task);  
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not find task with tid=%d",
                                       hdr->dest_tid);
                rc = MPC_LOWCOMM_ERROR;
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
                mpc_common_debug("LCP: recv unexp tag src=%d, length=%d, sequence=%d",
                                 hdr->src_tid, length, hdr->seqn);
                unsigned ctnr_flags = is_sync ? LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC | 
                        LCP_RECV_CONTAINER_UNEXP_EAGER_TAG : 
                        LCP_RECV_CONTAINER_UNEXP_EAGER_TAG;
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
                                                 ctnr_flags);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			goto err;
		}
		// add the request to the unexpected messages
		lcp_append_umq(task->umq_table, (void *)ctnr, 
			       hdr->comm,
			       hdr->tag,
			       hdr->src_tid);

		LCP_TASK_UNLOCK(task);
		return MPC_LOWCOMM_SUCCESS;
	}
	LCP_TASK_UNLOCK(task);

        /* If sync requested, send ack */
        if (is_sync) {
                rc = lcp_send_eager_sync_ack(req, hdr);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }
		
        /* Complete request */
        lcp_recv_eager_tag_data(req, hdr, length - sizeof(*hdr));
err:
	return rc;
}


static int lcp_eager_tag_handler(void *arg, void *data,
				size_t length, 
				__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	rc = _lcp_eager_tag_handler(arg, data, length, 0, flags);

	return rc;
}

static int lcp_eager_tag_sync_handler(void *arg, void *data,
				size_t length, 
				__UNUSED__ unsigned flags)
{
	int rc;

	rc = _lcp_eager_tag_handler(arg, data, length, 1, flags);

	return rc;
}

static int lcp_eager_tag_sync_ack_handler(void *arg, void *data,
                                          size_t length, 
                                          unsigned flags)
{
        UNUSED(length);
        UNUSED(flags);
	int rc = MPC_LOWCOMM_SUCCESS;
	
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
			rc = MPC_LOWCOMM_ERROR;
			goto err;
	}
	LCP_CONTEXT_UNLOCK(ctx);

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
	lcp_context_h ctx = arg;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;
	lcp_rndv_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        lcp_context_task_get(ctx, hdr->base.dest_tid, &task);  
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not find task with tid=%d",
                                       hdr->base.dest_tid);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        LCP_TASK_LOCK(task);
	req = (lcp_request_t *)lcp_match_prq(task->prq_table, 
					     hdr->base.comm, 
					     hdr->base.tag,
					     hdr->base.src_tid);
	if (req == NULL) {
                mpc_common_debug("LCP: recv unexp tag src=%d, length=%d",
                                 hdr->base.src_tid, length);
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
						 LCP_RECV_CONTAINER_UNEXP_RNDV_TAG);
		if (rc != MPC_LOWCOMM_SUCCESS) {
                        LCP_TASK_UNLOCK(task);
			goto err;
		}
		lcp_append_umq(task->umq_table, (void *)ctnr, 
			       hdr->base.comm,
			       hdr->base.tag,
			       hdr->base.src_tid);

		LCP_TASK_UNLOCK(task);
		return rc;
	}
	LCP_TASK_UNLOCK(task);

        /* Fill up receive request */
        lcp_recv_rndv_tag_data(req, hdr);
        assert(req->recv.tag.comm == hdr->base.comm);

        rc = lcp_rndv_process_rts(req, hdr, length - sizeof(*hdr));
err:
        return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_P2P_MESSAGE, lcp_eager_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_P2P_SYNC_MESSAGE, lcp_eager_tag_sync_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_P2P_ACK_MESSAGE, lcp_eager_tag_sync_ack_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RDV_MESSAGE, lcp_rndv_tag_handler, 0);
