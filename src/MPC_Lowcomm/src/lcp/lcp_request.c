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

#include "lcp_request.h"

#include <mpc_common_rank.h>

#include "lcp_def.h"
#include "lcp_pending.h"
#include "lcp_context.h"
#include "mpc_common_debug.h"
#include "mpc_mempool.h"

#include <sctk_alloc.h>
#include <string.h>

static mpc_mempool_t *__request_mempool = NULL;
static unsigned int __request_mempool_count = 0;

void lcp_request_storage_init()
{
	assume(__request_mempool_count == 0);

	__request_mempool_count = mpc_common_get_local_task_count();
	assume(__request_mempool_count >= 1);
	__request_mempool = sctk_malloc(sizeof(mpc_mempool_t) * __request_mempool_count);
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
	if (req == NULL) {
		mpc_common_debug_error("LCP: could not allocate recv request.");
		return LCP_ERROR;
	}

	memset(req, 0, sizeof(struct lcp_request));
	TODO("Find a way not to clear this request");

	*req_p = req;

	return LCP_SUCCESS;
}

/**
 * @brief Store data from unexpected message.
 *
 * @param task the task
 * @param ctnr_p message data (out)
 * @param data message data (in)
 * @param length length of message
 * @param flags flag of the message
 * @return int LCP_SUCCESS in case of success
 */
int lcp_request_init_unexp_ctnr(lcp_task_h task, lcp_unexp_ctnr_t **ctnr_p, void *data,
				size_t length, unsigned flags)
{
	lcp_unexp_ctnr_t *ctnr;

	ctnr = lcp_container_get(task);
	if (ctnr == NULL) {
		mpc_common_debug_error("LCP: could not allocate recv container.");
		return LCP_ERROR;
	}

        size_t elem_size = mpc_mpool_get_elem_size(task->unexp_mp);
        assert(sizeof(lcp_unexp_ctnr_t) + length < elem_size);

	ctnr->length = length;
	ctnr->flags = flags;

        mpc_common_debug_log("LCP: received unexpected message of length=%lu", length);
        //NOTE: check the standard but data could be NULL and length equal to 0.
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
	        req->request->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;
        }

        if (req->flags & LCP_REQUEST_RMA_COMPLETE) {
                req->send.rma.on_completion(req->request);
        }

        if (req->flags & LCP_REQUEST_OFFLOADED_RNDV) {
                lcp_pending_delete(req->ctx->match_ht, req->msg_id);
        }

        lcp_request_put(req);
        req = NULL;

	return LCP_SUCCESS;
}
