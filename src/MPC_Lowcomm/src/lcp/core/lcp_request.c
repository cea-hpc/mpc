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

#include <core/lcp_request.h>
#include <core/lcp_context.h>

#include <mpc_mempool.h>
#include <mpc_common_debug.h>
#include <mpc_common_rank.h>
#include <sctk_alloc.h>
#include <string.h>

int lcp_request_check_status(void *request)
{
        lcp_request_t *req = (lcp_request_t *)(request) - 1;

        if (req->flags & LCP_REQUEST_REMOTE_COMPLETED &&
            req->flags & LCP_REQUEST_LOCAL_COMPLETED) {
                return req->status;
        }

        return MPC_LOWCOMM_IN_PROGRESS;
}

void *lcp_request_alloc(lcp_task_h task)
{
        lcp_request_t *req = NULL;

        assert(task->ctx->config.request.size > 0);

        req = lcp_request_get(task);
        if (req == NULL) {
                mpc_common_debug_error("LCP REQ: could not allocate "
                                       "request.");
                return NULL;
        }
        req->flags |= LCP_REQUEST_USER_ALLOCATED;
        //NOTE: with lcp_request_get, completion is set automatically.
        //      Unset it.
        req->flags &= ~LCP_REQUEST_RELEASE_ON_COMPLETION;

        return req + 1;
}

void lcp_request_free(void *request)
{
        lcp_request_t *req = (lcp_request_t *)(request) - 1;

        assert(req->flags & LCP_REQUEST_USER_ALLOCATED);

        lcp_request_put(req);
}

/**
 * @brief Store data from unexpected message.
 *
 * @param task the task
 * @param ctnr_p message data (out)
 * @param data message data (in)
 * @param length length of message
 * @param flags flag of the unexpected message
 * @return int MPC_LOWCOMM_SUCCESS in case of success
 */
int lcp_request_init_unexp_ctnr(lcp_task_h task, lcp_unexp_ctnr_t **ctnr_p,
                                struct iovec *iov, size_t iovcnt, unsigned flags)
{
	lcp_unexp_ctnr_t *ctnr;
        int i;

	ctnr = lcp_container_get(task);
	if (ctnr == NULL) {
		mpc_common_debug_error("LCP: could not allocate recv container.");
		return MPC_LOWCOMM_ERROR;
	}

        size_t elem_size = mpc_mpool_get_elem_size(&task->unexp_mp);
        ptrdiff_t offset = 0;

        for (i = 0; i < (int)iovcnt; i++) {
                assert(iov[i].iov_len + offset < elem_size);
                memcpy((char *)(ctnr + 1) + offset, iov[i].iov_base, iov[i].iov_len);
                offset += iov[i].iov_len;
        }

	ctnr->flags  = 0;
	ctnr->flags |= flags;
        ctnr->length = offset;

	*ctnr_p = ctnr;

	return MPC_LOWCOMM_SUCCESS;
}
