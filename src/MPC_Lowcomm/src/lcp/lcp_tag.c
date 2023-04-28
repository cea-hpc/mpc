#include "alloca.h"

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_prototypes.h"
#include "lcp_task.h"
#include "lcp_datatype.h"

#include "mpc_common_debug.h"
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
size_t lcp_send_tag_pack(void *dest, void *data)
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
	
	hdr->src  = req->send.tag.src;
	hdr->msg_id = req->msg_id;

	mpc_common_debug("LCP: packing ack seq=%ld", req->msg_id);

	return sizeof(*hdr);
}

/* ============================================== */
/* Completion                                     */
/* ============================================== */

/**
 * @brief Complete a request by its tag
 * 
 * @param comp completion
 */
void lcp_tag_complete(lcr_completion_t *comp) {

	lcp_request_t *req = mpc_container_of(comp, lcp_request_t, 
					      state.comp);
	if(!req->request->synchronized)
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
int lcp_send_am_eager_tag_bcopy(lcp_request_t *req) 
{
	
	int rc = MPC_LOWCOMM_SUCCESS;
	ssize_t payload;
	lcp_ep_h ep = req->send.ep;
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->state.cc];

	mpc_common_debug_info("LCP: send am eager tag bcopy comm=%d, src=%d, "
                              "dest=%d, length=%d, tag=%d, lcreq=%p.", req->send.tag.comm,
                              req->send.tag.src_tid, req->send.tag.dest_tid, 
                              req->send.length, req->send.tag.tag, req->request);
	int message_type;
	if(req->request->synchronized) message_type = MPC_LOWCOMM_P2P_SYNC_MESSAGE;
	else message_type = MPC_LOWCOMM_P2P_MESSAGE;

	mpc_common_debug_info("LCP: send am eager tag bcopy src=%d, dest=%d, length=%d msg_id=%d",
			      req->send.tag.src, req->send.tag.dest, req->send.length, req->msg_id);
        payload = lcp_send_do_am_bcopy(lcr_ep, 
                                       message_type, 
                                       lcp_send_tag_pack, 
                                       req);
	//FIXME: handle error
	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
	}
	if(!req->request->synchronized)
        lcp_request_complete(req);

	return rc;
}

/**
 * @brief Tag active message zcopy eager send function
 * 
 * @param req request
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_send_am_eager_tag_zcopy(lcp_request_t *req) 
{
	
	int rc;
        size_t iovcnt     = 0;
	lcp_ep_h ep       = req->send.ep;
        size_t max_iovec  = ep->ep_config.am.max_iovecs;
	struct iovec *iov = alloca(max_iovec*sizeof(struct iovec));
	_mpc_lowcomm_endpoint_t *lcr_ep = ep->lct_eps[req->state.cc];
	
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
                .comp_cb = lcp_tag_complete
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
	rc = lcp_send_do_am_zcopy(lcr_ep, 
								message_type, 
								hdr, 
								sizeof(lcp_tag_hdr_t),
								iov,
								iovcnt, 
								&(req->state.comp));

err:
	return rc;
}

/* ============================================== */
/* Recv                                           */
/* ============================================== */
//NOTE: no receive call for active message
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
static int _lcp_am_tag_handler(lcp_request_t **request, void *arg, void *data,
			  size_t length, int msg_flags,
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
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
						 msg_flags);
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
		
        mpc_common_debug("LCP: recv exp tag src=%d, length=%d, sequence=%d",
                         hdr->src_tid, length, hdr->seqn);
	/* copy data to receiver buffer and complete request */
        lcp_datatype_unpack(req->ctx, req, req->datatype, 
                            req->recv.buffer, (void *)(hdr + 1),
                            length - sizeof(*hdr));

        /* set recv info for caller */
        req->info->length = length - sizeof(*hdr);
        req->info->src    = hdr->src_tid;
        req->info->tag    = hdr->tag;
err:
	return rc;
}


int lcp_tag_send_ack(lcp_request_t *parent_request, lcp_tag_hdr_t *hdr){

	lcp_task_h task;

	lcp_request_param_t param = {
			.recv_info = &parent_request->request->recv_info,
			.datatype  = parent_request->request->dt_magic == (int)0xDDDDDDDD ? 
					LCP_DATATYPE_DERIVED : LCP_DATATYPE_CONTIGUOUS,
	};
	int tid = mpc_common_get_task_rank();
	lcp_context_task_get(parent_request->ctx, tid, &task);
	int payload;

	lcp_request_t *ack_request;
	lcp_ack_hdr_t ack_hdr;

	unsigned flags;

	lcp_ep_h ep;

	parent_request->msg_id = parent_request->seqn;


	lcp_ep_get_or_create(parent_request->ctx, 
		mpc_lowcomm_tag_get_endpoint_address(hdr), 
		&ep, 
		flags);

	assert(ep);

	mpc_common_debug("LCP: sending ack message for msg %ld", parent_request->seqn);
	lcp_request_create(&ack_request);
	if(ack_request == NULL){
		mpc_common_debug("LCP: error creating ack request");
	}

	uint64_t ack_key = lcp_msg_id(mpc_lowcomm_peer_get_rank(mpc_lowcomm_monitor_get_uid()), hdr->seqn);

	LCP_REQUEST_INIT_SEND(ack_request, ep->ctx, task, parent_request->request, 
							param.recv_info, sizeof(ack_hdr), 
							ep, (void *)&ack_hdr, 
							parent_request->seqn, parent_request->msg_id, param.datatype);
	mpc_common_debug("LCP: sending ack for request %lu, src %d", ack_hdr.msg_id, ack_hdr.src);
	if (ack_request == NULL) {
		return MPC_LOWCOMM_ERROR;
	}
	ack_request->msg_id = ack_key;

	// send an active message using buffer copy from the endpoint ep
	payload = lcp_send_do_am_bcopy(ep->lct_eps[ep->priority_chnl],
									MPC_LOWCOMM_P2P_ACK_MESSAGE,
									lcp_send_ack_pack,
									ack_request);
	if (payload < 0) {
			mpc_common_debug_error("LCP: error sending ack eager message");
			return MPC_LOWCOMM_ERROR;
	}
	mpc_common_debug("LCP: successfully sent ack message");

	lcp_request_complete(ack_request);

	return MPC_LOWCOMM_SUCCESS;
}

static int lcp_am_tag_handler(void *arg, void *data,
				size_t length, 
				__UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_request_t *req;
	if(_lcp_am_tag_handler(&req, arg, data, length, LCP_RECV_CONTAINER_UNEXP_TAG, flags) == MPC_LOWCOMM_SUCCESS && req)
		lcp_request_complete(req);
	return rc;
}

static int lcp_am_tag_sync_handler(void *arg, void *data,
				size_t length, 
				__UNUSED__ unsigned flags){
	int rc;
	lcp_request_t *req;
	rc = _lcp_am_tag_handler(&req, arg, data, length, LCP_RECV_CONTAINER_UNEXP_TAG | LCP_RECV_CONTAINER_UNEXP_TAG_SYNC, flags);
	
	// From here if req is null it means the received request 
	// was not matched so it is unexpected and added to the corresponding queue.
	// That means we have to send the ack when it is matched and continue from now on.

	if(req == NULL && rc == MPC_LOWCOMM_SUCCESS)
		return rc;

	lcp_tag_hdr_t *hdr = data;

	lcp_tag_send_ack(req, hdr);
	lcp_request_complete(req);

	return rc;
}

static int lcp_am_tag_ack_handler(void *arg, void *data,
				size_t length, 
				__UNUSED__ unsigned flags){
	int rc = MPC_LOWCOMM_SUCCESS;
	
	lcp_request_t *req;
	lcp_context_h ctx = arg;
	lcp_ack_hdr_t *hdr = (lcp_ack_hdr_t *)data;
	uint64_t ack_msg_id = hdr->msg_id;

	mpc_common_debug_info("LCP: recv ack header src=%d, msg_id=%lu",
							hdr->src, ack_msg_id);
	/* Retrieve request */
	req = lcp_pending_get_request(ctx->pend, ack_msg_id);
	if (req == NULL) {
			mpc_common_debug_error("LCP: could not find ack ctrl msg: "
									"msg id=%llu.", ack_msg_id);
			rc = MPC_LOWCOMM_ERROR;
			goto err;
	}
	req->flags |= LCP_REQUEST_DELETE_FROM_PENDING;
	lcp_request_complete(req);
err:
        return rc;
}
LCP_DEFINE_AM(MPC_LOWCOMM_P2P_MESSAGE, lcp_am_tag_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_P2P_SYNC_MESSAGE, lcp_am_tag_sync_handler, 0);
LCP_DEFINE_AM(MPC_LOWCOMM_P2P_ACK_MESSAGE, lcp_am_tag_ack_handler, 0);