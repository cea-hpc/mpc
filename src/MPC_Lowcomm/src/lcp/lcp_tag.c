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

#include "mpc_common_debug.h"
#include "mpc_lowcomm_msg.h"
#include "mpc_lowcomm_types.h"
#include "sctk_alloc.h"


uint64_t lcp_msg_id(uint16_t rank, uint16_t sequence){
        uint64_t msg_id;

        msg_id = rank; // dest is int 16
        msg_id = msg_id << 32;
        msg_id |= sequence; // hdr->seqn is int 16;
        return msg_id;
}

/* ============================================== */
/* Packing                                        */
/* ============================================== */

/**
 * @brief Pack a message into a header
 * 
 * @param dest header
 * @param data data to pack
 * @return size_t size of packed data
 */
static size_t lcp_send_tag_eager_pack(void *dest, void *data)
{
	lcp_tag_hdr_t *hdr = dest;
	lcp_request_t *req = data;
        void *src = req->datatype == LCP_DATATYPE_CONTIGUOUS ?
               req->send.buffer : NULL; 
	
	hdr->comm     = req->send.tag.comm;
	hdr->tag      = req->send.tag.tag;
	hdr->src_tid  = req->send.tag.src_tid;
        hdr->dest_tid = req->send.tag.dest_tid;
	hdr->seqn     = req->seqn; 

        lcp_datatype_pack(req->ctx, req, req->datatype,
                          (void *)(hdr + 1), src, req->send.length);

	return sizeof(*hdr) + req->send.length;
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
	
	hdr->src  = req->send.tag.src_tid;
	hdr->msg_id = req->send.ack_msg_key;

	mpc_common_debug("LCP: packing ack seq=%ld", req->send.ack_msg_key);

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

        hdr->msg_id        = req->msg_id;
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

        req->info->length = req->send.length;
        req->info->src    = req->send.tag.src_tid;
        req->info->tag    = req->send.tag.tag;

	if(!req->request->synchronized)
		lcp_request_complete(req);
}

static void lcp_tag_recv_complete(lcr_completion_t *comp) {

	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      state.comp);

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
	int rc = MPC_LOWCOMM_SUCCESS;

	mpc_common_debug_info("LCP: send am eager tag bcopy comm=%d, src=%d, "
                              "dest=%d, length=%d, tag=%d, lcreq=%p.", req->send.tag.comm,
                              req->send.tag.src_tid, req->send.tag.dest_tid, 
                              req->send.length, req->send.tag.tag, req->request);

	if(!req->message_type){
		req->message_type = MPC_LOWCOMM_P2P_MESSAGE;
	}
	if(!req->send.pack_function)
		req->send.pack_function = lcp_send_tag_eager_pack;

	mpc_common_debug_info("LCP: send am eager tag bcopy comm=%d, src=%d, "
                              "dest=%d, length=%d, tag=%d, lcreq=%p msg_id=%lu seqn=%d.", req->send.tag.comm,
                              req->send.tag.src_tid, req->send.tag.dest_tid, 
                              req->send.length, req->send.tag.tag, req->request, req->msg_id, req->seqn);

        req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_send_complete,
        };

        rc = lcp_send_eager_bcopy(req, req->send.pack_function,
                                  req->message_type);

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
	int message_type;

	if(req->request->synchronized) message_type = MPC_LOWCOMM_P2P_SYNC_MESSAGE;
	else message_type = MPC_LOWCOMM_P2P_MESSAGE;

        rc = lcp_send_eager_zcopy(req, message_type, 
                                  hdr, sizeof(lcp_tag_hdr_t),
                                  iov, iovcnt, 
                                  &(req->state.comp));

err:
	return rc;
}

static int lcp_send_rndv_tag_rts_progress(lcp_request_t *req)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        ssize_t payload_size;
        lcp_ep_h ep = req->send.ep;
        lcp_chnl_idx_t cc = lcp_ep_get_next_cc(ep);

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
        size_t unpacked_len = 0;

        mpc_common_debug("LCP: matched tag unexp req=%p, src=%d, "
                         "tag=%d, comm=%d", req, hdr->src_tid, hdr->tag, 
                         hdr->comm);
        /* copy data to receiver buffer and complete request */
        unpacked_len = lcp_datatype_unpack(req->ctx, req, req->datatype, 
                                           req->recv.buffer, (void *)(hdr + 1),
                                           length);
        if (unpacked_len < 0) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        /* set recv info for caller */
        req->recv.send_length = length; 
        req->recv.tag.src_tid = hdr->src_tid;
        req->recv.tag.tag     = hdr->tag;

        lcp_tag_recv_complete(&(req->state.comp));
err:
        return rc;
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
static int _lcp_eager_tag_handler(lcp_request_t **request, void *arg, void *data,
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
	// try to match it with a posted message
	*request = (lcp_request_t *)lcp_match_prq(task->prq_table, 
					     hdr->comm, 
					     hdr->tag,
					     hdr->src_tid);
	// if request is not matched
	req = *request;
	if (req == NULL) {
                mpc_common_debug("LCP: recv unexp tag src=%d, length=%d, sequence=%d",
                                 hdr->src_tid, length, hdr->seqn);
                int ctnr_flags = is_sync ? LCP_RECV_CONTAINER_UNEXP_EAGER_TAG_SYNC | 
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

        if (is_sync) {
                rc = lcp_send_tag_ack(req);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }
		
        lcp_recv_eager_tag_data(req, hdr, length - sizeof(*hdr));
err:
	return rc;
}

int lcp_send_tag_ack(lcp_request_t *super)
{
        int rc;
	ssize_t payload;
        lcp_ep_h ep;
	lcp_request_t *ack;
	
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
	ack->msg_id           = super->msg_id;
        ack->send.tag.src_tid = super->recv.tag.dest_tid;

	mpc_common_debug("LCP: sending ack for request %lu, src %d", 
                         super->seqn, super->recv.tag.src);

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

static int lcp_am_tag_handler(void *arg, void *data,
				size_t length, 
				__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_request_t *req;
	if((_lcp_eager_tag_handler(&req, arg, data, length, 0, flags) 
            == MPC_LOWCOMM_SUCCESS) && req)
		lcp_request_complete(req);
	return rc;
}

static int lcp_am_tag_sync_handler(void *arg, void *data,
				size_t length, 
				__UNUSED__ unsigned flags){
	int rc;
	lcp_request_t *req = NULL;

	rc = _lcp_eager_tag_handler(&req, arg, data, length, 1, flags);

	return rc;
}

static int lcp_am_tag_ack_handler(void *arg, void *data,
				size_t length, 
				__UNUSED__ unsigned flags){
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

	req->flags |= LCP_REQUEST_DELETE_FROM_PENDING;
	lcp_tag_send_complete(req);
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

        req->recv.tag.src_tid  = hdr->base.src_tid;
        req->seqn              = hdr->base.seqn;
        req->recv.tag.tag      = hdr->base.tag;
        //TODO: on receiver msg_id might not be unique, it has to be completed
        //      with sender unique MPI identifier.
        req->msg_id            = hdr->msg_id;
        req->recv.send_length  = hdr->size; 
	req->state.comp = (lcr_completion_t) {
                .comp_cb = lcp_tag_recv_complete
        };
        assert(req->recv.tag.comm == hdr->base.comm);

        rc = lcp_rndv_process_rts(req, hdr + 1, length - sizeof(*hdr));
err:
        return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_P2P_MESSAGE, lcp_eager_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_RDV_MESSAGE, lcp_rndv_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_P2P_SYNC_MESSAGE, lcp_eager_tag_sync_handler, 0);
