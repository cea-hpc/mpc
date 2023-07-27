#include "lcp_request.h"
#include "lcp_def.h"
#include "lcp_ep.h"
#include "lcp_pending.h"
#include "lcp_context.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include "mpc_mempool.h"

#include <sctk_alloc.h>

mpc_mempool *request_mempool;

__attribute__((constructor)) static void init_mempool(){
	request_mempool = (void *)sctk_malloc(sizeof(mpc_mempool));
	mpc_mempool_init(request_mempool, 10, 100, sizeof(lcp_request_t), sctk_malloc, sctk_free);
}

__attribute__((constructor)) static void destroy_mempool(){
	mpc_mempool_empty(request_mempool);
}

/**
 * @brief Create a request.
 * 
 * @param req_p pointer to the request to be created
 * @return int LCP_SUCCESS in case of success
 */
int lcp_request_create(lcp_request_t **req_p)
{
	lcp_request_t *req;
	req = mpc_mempool_alloc(request_mempool);
	// req = (lcp_request_t *)sctk_malloc(sizeof(lcp_request_t));
	if (req == NULL) {
		mpc_common_debug_error("LCP: could not allocate recv request.");
		return LCP_ERROR;
	}
	memset(req, 0, sizeof(lcp_request_t));

	*req_p = req;

	return LCP_SUCCESS;
}

/**
 * @brief Store data from unexpected message.
 * 
 * @param ctnr_p message data (out)
 * @param data message data (in)
 * @param length length of message
 * @param flags flag of the message
 * @return int LCP_SUCCESS in case of success
 */
int lcp_request_init_unexp_ctnr(lcp_unexp_ctnr_t **ctnr_p, void *data, 
				size_t length, unsigned flags)
{
	lcp_unexp_ctnr_t *ctnr;

	ctnr = sctk_malloc(sizeof(lcp_unexp_ctnr_t) + length);
	if (ctnr == NULL) {
		mpc_common_debug_error("LCP: could not allocate recv "
				       "container.");
		return LCP_ERROR;
	}

	ctnr->length = length;
	ctnr->flags  = 0;
	ctnr->flags |= flags;

	memcpy(ctnr + 1, data, length);

	*ctnr_p = ctnr;
	return LCP_SUCCESS;
}

/**
 * @brief Set a request as completed
 * 
 * @param req request to be marked as completed
 * @return int LCP_SUCCESS in case of success
 */
int lcp_request_complete(lcp_request_t *req)
{
	mpc_common_debug("LCP: complete req=%p, comm_id=%llu, msg_id=%llu, "
			 "seqn=%d, lcreq=%p", req, req->send.tag.comm, req->msg_id, 
			 req->seqn, req->request);

        //FIXME: modifying mpc request here breaks modularity
        if (req->flags & LCP_REQUEST_RECV_TRUNC)
                req->request->truncated = 1;

        if (req->flags & LCP_REQUEST_MPI_COMPLETE) {
                assert(req->request);
                //FIXME: calling request completion here breaks modularity
                req->request->request_completion_fn(req->request);
        }

        if (req->flags & LCP_REQUEST_RMA_COMPLETE) {
                req->send.cb(req->request);
        }

        if (req->flags & LCP_REQUEST_OFFLOADED_RNDV) {
                lcp_pending_delete(req->ctx->match_ht, req->msg_id);
        }

	mpc_mempool_free(request_mempool, req);
	// sctk_free(req);

	return LCP_SUCCESS;
}
