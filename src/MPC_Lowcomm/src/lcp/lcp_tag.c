#include "alloca.h"

#include "lcp_ep.h"
#include "lcp_header.h"
#include "lcp_request.h"
#include "lcp_context.h"
#include "lcp_prototypes.h"
#include "lcp_task.h"
#include "lcp_datatype.h"

#include "sctk_alloc.h"

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
static size_t lcp_send_tag_pack(void *dest, void *data)
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

	lcp_request_complete(req);
}

/* ============================================== */
/* Send                                           */
/* ============================================== */

/**
 * @brief Callback for buffered copy in tagged active message eager
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
        payload = lcp_send_do_am_bcopy(lcr_ep, 
                                       MPC_LOWCOMM_P2P_MESSAGE, 
                                       lcp_send_tag_pack, 
                                       req);
	//FIXME: handle error
	if (payload < 0) {
		mpc_common_debug_error("LCP: error packing bcopy.");
		rc = MPC_LOWCOMM_ERROR;
	}

        lcp_request_complete(req);

	return rc;
}

/**
 * @brief Callback for zero copy in tagged active message eager
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
        rc = lcp_send_do_am_zcopy(lcr_ep, 
                                  MPC_LOWCOMM_P2P_MESSAGE, 
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
static int lcp_am_tag_handler(void *arg, void *data,
                              size_t length, 
                              __UNUSED__ unsigned flags)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = arg;
	lcp_request_t *req;
	lcp_unexp_ctnr_t *ctnr;
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
	req = (lcp_request_t *)lcp_match_prq(task->prq_table, 
					     hdr->comm, 
					     hdr->tag,
					     hdr->src_tid);
	if (req == NULL) {
                mpc_common_debug("LCP: recv unexp tag src=%d, length=%d",
                                 hdr->src_tid, length);
		rc = lcp_request_init_unexp_ctnr(&ctnr, hdr, length, 
						 LCP_RECV_CONTAINER_UNEXP_TAG);
		if (rc != MPC_LOWCOMM_SUCCESS) {
			goto err;
		}
		lcp_append_umq(task->umq_table, (void *)ctnr, 
			       hdr->comm,
			       hdr->tag,
			       hdr->src_tid);

		LCP_TASK_UNLOCK(task);
		return MPC_LOWCOMM_SUCCESS;
	}
	LCP_TASK_UNLOCK(task);
		
        mpc_common_debug("LCP: recv exp tag src=%d, length=%d",
                         hdr->src_tid, length);
	/* copy data to receiver buffer and complete request */
        lcp_datatype_unpack(req->ctx, req, req->datatype, 
                            req->recv.buffer, (void *)(hdr + 1),
                            length - sizeof(*hdr));

        /* set recv info for caller */
        req->info->length = length - sizeof(*hdr);
        req->info->src    = hdr->src_tid;
        req->info->tag    = hdr->tag;

	lcp_request_complete(req);
err:
	return rc;
}

LCP_DEFINE_AM(MPC_LOWCOMM_P2P_MESSAGE, lcp_am_tag_handler, 0);
