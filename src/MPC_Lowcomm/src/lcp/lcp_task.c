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
#include "lcp_task.h"

#include "lcp.h"
#include "lcp_common.h"
#include "mpc_common_debug.h"

#include "lcp_def.h"
#include "lcp_context.h"
#include "lcp_header.h"
#include "lcp_request.h"

static size_t lcp_send_task_tag_zcopy_pack(void *dest, void *data)
{
	lcp_tag_sync_hdr_t *hdr = dest;
	lcp_request_t  *req = data;

        hdr->base.comm     = req->send.tag.comm;
        hdr->base.tag      = req->send.tag.tag;
        hdr->base.src_tid  = req->send.tag.src_tid;
        hdr->base.dest_tid = req->send.tag.dest_tid;
        hdr->base.seqn     = req->seqn;

        hdr->src_uid      = req->send.tag.src_uid;
        hdr->msg_id       = (uint64_t)req;

	return sizeof(*hdr);
}

int lcp_send_task_bcopy(lcp_request_t *sreq, lcp_request_t **matched_req, 
                        lcr_pack_callback_t pack_cb, unsigned flags, 
                        lcp_unexp_ctnr_t **ctnr_p) 
{
        int rc = LCP_SUCCESS;
        lcp_context_h ctx = sreq->ctx;
        lcp_unexp_ctnr_t *ctnr;
        lcp_request_t *rreq;
        lcp_task_h recv_task = NULL;

        /* Ensure provided matching is null */
        assume(*matched_req == NULL);

        recv_task = lcp_context_task_get(ctx, sreq->send.tag.dest_tid);  
        if (recv_task == NULL) {
                mpc_common_errorpoint_fmt("LCP: could not find task with tid=%d", 
                                          sreq->send.tag.dest_tid);

                rc = LCP_ERROR;
                goto err;
        }

        if (recv_task->expected_seqn[sreq->send.tag.src_tid] != sreq->seqn) {
                mpc_common_debug_warning("LCP SELF: wrong sequence number. Expected %d, "
                                         "got %d", recv_task->expected_seqn[sreq->send.tag.src_tid],
                                         sreq->seqn);
        }

        /* For buffered copy, data is systematically copied to a shaddow buffer
         * before being sent. */
        rc = lcp_request_init_unexp_ctnr(recv_task, &ctnr, sreq->send.buffer, 
                                         0, flags);
        if (rc != LCP_SUCCESS) {
                goto err;
        }
        ctnr->length = pack_cb(ctnr + 1, sreq);

        if (ctnr->length < 0) {
                mpc_common_debug_error("LCP SELF: could not pack data.");
                goto err;
        }

        /* If data successfully copied, then set container pointer so it can be
         * retrieved by caller. */
        *ctnr_p = ctnr;

	LCP_TASK_LOCK(recv_task);

        recv_task->expected_seqn[sreq->send.tag.src_tid]++;
	/* Try to match it with a posted message */
        rreq = (lcp_request_t *)lcp_match_prq(&recv_task->prq_table, 
                                             sreq->send.tag.comm, 
                                             sreq->send.tag.tag,
                                             sreq->send.tag.src_tid);

	/* If request is not matched */
	if (rreq == NULL) {
                mpc_common_debug_info("LCP: recv unexp tag src=%d, tag=%d, dest=%d, "
                                      "length=%d, sequence=%d, req=%p", sreq->send.tag.src_tid, 
                                      sreq->send.tag.tag, sreq->send.tag.dest_tid, 
                                      sreq->send.length, sreq->seqn, sreq);


		// add the request to the unexpected messages
		lcp_append_umq(&recv_task->umq_table, (void *)ctnr, 
                               sreq->send.tag.comm, 
                               sreq->send.tag.tag,
                               sreq->send.tag.src_tid);

		LCP_TASK_UNLOCK(recv_task);
		return rc;
	}
	LCP_TASK_UNLOCK(recv_task);

        /* Request has been matched so set corresponding flag so it can be
         * completed by caller */
        *matched_req = rreq;

err:
	return rc;
}

int lcp_send_task_zcopy(lcp_request_t *sreq, lcp_request_t **matched_req) 
{
        int rc = LCP_SUCCESS;
        lcp_context_h ctx = sreq->ctx;
        lcp_unexp_ctnr_t *ctnr;
        lcp_request_t *rreq;
        lcp_task_h recv_task = NULL;

        /* Ensure provided matching is null */
        assume(*matched_req == NULL);

        recv_task = lcp_context_task_get(ctx, sreq->send.tag.dest_tid);
        if (recv_task == NULL) {
                mpc_common_errorpoint_fmt("LCP: could not find task with tid=%d", 
                                          sreq->send.tag.dest_tid);

                rc = LCP_ERROR;
                goto err;
        }

        if (recv_task->expected_seqn[sreq->send.tag.src_tid] != sreq->seqn) {
                mpc_common_debug_warning("LCP SELF: wrong sequence number. Expected %d, "
                                         "got %d", recv_task->expected_seqn[sreq->send.tag.src_tid],
                                         sreq->seqn);
        }

	LCP_TASK_LOCK(recv_task);

        recv_task->expected_seqn[sreq->send.tag.src_tid]++;
	/* Try to match it with a posted message */
        rreq = (lcp_request_t *)lcp_match_prq(&recv_task->prq_table, 
                                             sreq->send.tag.comm, 
                                             sreq->send.tag.tag,
                                             sreq->send.tag.src_tid);

	/* If request is not matched */
	if (rreq == NULL) {
                mpc_common_debug_info("LCP: recv unexp tag src=%d, tag=%d, dest=%d, "
                                      "length=%d, sequence=%d, req=%p", sreq->send.tag.src_tid, 
                                      sreq->send.tag.tag, sreq->send.tag.dest_tid, 
                                      sreq->send.length, sreq->seqn, sreq);

                rc = lcp_request_init_unexp_ctnr(recv_task, &ctnr, sreq->send.buffer, 
                                                 0, LCP_RECV_CONTAINER_UNEXP_TASK_TAG_ZCOPY);
                if (rc != LCP_SUCCESS) {
                        goto err;
                }
                ctnr->length = lcp_send_task_tag_zcopy_pack(ctnr + 1, sreq);

                if (ctnr->length < 0) {
                        mpc_common_debug_error("LCP SELF: could not pack data.");
                        goto err;
                }

		// add the request to the unexpected messages
		lcp_append_umq(&recv_task->umq_table, (void *)ctnr, 
                               sreq->send.tag.comm, 
                               sreq->send.tag.tag,
                               sreq->send.tag.src_tid);

		LCP_TASK_UNLOCK(recv_task);
		return rc;
	}
	LCP_TASK_UNLOCK(recv_task);

        /* Request has been matched so set corresponding flag so it can be
         * completed by caller */
        *matched_req = rreq;

err:
	return rc;
}

int lcp_am_set_handler_callback(lcp_task_h task, uint8_t am_id,
                                void *user_arg, lcp_am_callback_t cb,
                                unsigned flags)
{
        lcp_am_user_handler_t *user_handler = &task->am[am_id];

        user_handler->cb       = cb;
        user_handler->user_arg = user_arg;
        user_handler->flags    = flags;

        return LCP_SUCCESS;
}



int lcp_task_create(lcp_context_h ctx, int tid, lcp_task_h *task_p)
{
        int rc = LCP_SUCCESS;
        int task_idx;
        lcp_task_h task;

        assert(ctx); assert(tid >= 0);

        //NOTE: In current MPC configuration, a task cannot be already 
        //      created since done by init task func in comm.c 

        task = sctk_malloc(sizeof(struct lcp_task));
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not allocate task");
                rc = LCP_ERROR;
                goto err;
        }

        /* Set task unique identifier */
        task->tid = tid;
        task->ctx = ctx;

        /* Allocate tag matching lists */
        rc = lcp_prq_match_table_init(&task->prq_table);
        if (rc != LCP_SUCCESS) {
                goto err;
        }
        rc = lcp_umq_match_table_init(&task->umq_table);
        if (rc != LCP_SUCCESS) {
                goto err;
        }

        /* Init task lock used for matching lists */
	mpc_common_spinlock_init(&task->task_lock, 0);

        /* Init memory pool of requests */
        task->req_mp = sctk_malloc(sizeof(mpc_mempool_t));
        mpc_mempool_param_t mp_req_params = {
                .alignment = MPC_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 512,
                .elem_size = sizeof(lcp_request_t),
                .max_elems = 2048,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };
                
        rc = mpc_mpool_init(task->req_mp, &mp_req_params);
        if (rc != LCP_SUCCESS) {
                goto err;
        }

        /* Init memory pool of requests */
        task->unexp_mp = sctk_malloc(sizeof(mpc_mempool_t));
        //FIXME: define suitable size for unexpected buffers. Could be a set of
        //       memory pools, based on size to be copied the appropriate memory
        //       pool would be chosen.
        mpc_mempool_param_t mp_unexp_params = {
                .alignment = MPC_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 128,
                .elem_size = 8192,
                .max_elems = 1024,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };
                
        rc = mpc_mpool_init(task->unexp_mp, &mp_unexp_params);
        if (rc != LCP_SUCCESS) {
                goto err;
        }

        /* Init table of user AM callbacks */
        task->am = sctk_malloc(LCP_AM_ID_USER_MAX * sizeof(lcp_am_user_handler_t));
        if (task->am == NULL) {
                mpc_common_debug_error("LCP CTX: Could not allocated user AM handlers");
                rc = LCP_ERROR;
                goto err;
        }
        memset(task->am, 0, LCP_AM_ID_USER_MAX * sizeof(lcp_am_user_handler_t));

        task->seqn          = sctk_malloc(ctx->num_tasks * sizeof(uint64_t));
        task->expected_seqn = sctk_malloc(ctx->num_tasks * sizeof(uint64_t));
        for (int i = 0; i < ctx->num_tasks; i++) {
                task->seqn[i] = 0;
                task->expected_seqn[i] = 0;
        }

        /* Compute task index used in */
        task_idx = tid % ctx->num_tasks;
        ctx->tasks[task_idx] = task;

        *task_p = task;
        mpc_common_debug_info("LCP: created task tid=%d with idx=%d", tid, task_idx);
err:
        return rc;
}

int lcp_task_fini(lcp_task_h task) {

        lcp_fini_matching_engine(&task->umq_table, &task->prq_table);

        mpc_mpool_fini(task->req_mp);

        sctk_free(task);
        task = NULL;

        return LCP_SUCCESS;
}
