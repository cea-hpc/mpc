#include "lcp_request.h"
#include "lcp_def.h"
#include "lcp_ep.h"
#include "lcp_pending.h"
#include "lcp_context.h"
#include "mpc_common_debug.h"
#include "mpc_common_spinlock.h"
#include "mpc_mempool.h"
#include "mpc_thread_accessor.h"

#include <mpc_common_rank.h>

#include <sctk_alloc.h>

static mpc_mempool *__request_mempool = NULL;
static unsigned int __request_mempool_count = 0;

void lcp_request_storage_init()
{
	assume(__request_mempool_count == 0);

	__request_mempool_count = mpc_common_get_local_task_count();
	assume(__request_mempool_count >= 1);
	__request_mempool = sctk_malloc(sizeof(mpc_mempool) * __request_mempool_count);
	assume(__request_mempool != NULL);

	unsigned int i = 0;

	for(i = 0 ; i < __request_mempool_count; i++)
	{
		/* We allocate 0 by default to avoid bad numa */
		mpc_mempool_init(&__request_mempool[i], 0, 100, sizeof(lcp_request_t), sctk_malloc, sctk_free);
	}
}

void lcp_request_storage_release()
{
	unsigned int i;

	for(i = 0 ; i < __request_mempool_count; i++)
	{
		mpc_mempool_empty(&__request_mempool[i]);
	}

	sctk_free(__request_mempool);
	__request_mempool = NULL;
	__request_mempool_count = 0;
}

/**
 * @brief Create a request.
 *
 * @param req_p pointer to the request to be created
 * @return int LCP_SUCCESS in case of success
 */
int lcp_request_create(lcp_request_t **req_p)
{
	lcp_request_t *req = NULL;
	int local_rank = mpc_common_get_local_task_rank();

	if(local_rank < 0)
	{
		/* If the request is allocated outside of a task context default to 0*/
		local_rank = 0;
	}

	req = mpc_mempool_alloc(&__request_mempool[local_rank]);
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

	mpc_mempool_free(NULL, req);

	return LCP_SUCCESS;
}
