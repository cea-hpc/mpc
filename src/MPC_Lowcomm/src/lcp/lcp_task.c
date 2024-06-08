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
#include "mpc_common_debug.h"

#include "lcp_def.h"
#include "lcp_manager.h"
#include "lcp_manager.h"
#include "lcp_context.h"
#include "lcp_request.h"

#define NUM_QUEUES UINT16_MAX

int lcp_am_set_handler_callback(lcp_manager_h mngr, lcp_task_h task, uint8_t am_id,
                                void *user_arg, lcp_am_callback_t cb,
                                unsigned flags)
{
        lcp_am_user_handler_t *user_handler = &task->tcct[mngr->id]->am.handlers[am_id];

        user_handler->cb       = cb;
        user_handler->user_arg = user_arg;
        user_handler->flags    = flags;

        return MPC_LOWCOMM_SUCCESS;
}

void lcp_task_request_init(mpc_mempool_t *mp, void *request) {
        lcp_request_t *req = (lcp_request_t *)request;
        lcp_task_h task   = mpc_container_of(mp, struct lcp_task, req_mp);
        lcp_context_h ctx = task->ctx;

        if (ctx->config.request.init != NULL) {
                ctx->config.request.init(req + 1);
        }
}

int lcp_task_create(lcp_context_h ctx, int tid, lcp_task_h *task_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_task_h task;

        assert(ctx); assert(tid >= 0);

        //NOTE: In current MPC configuration, a task cannot be already
        //      created since done by init task func in comm.c

        task = sctk_malloc(sizeof(struct lcp_task));
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not allocate task");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(task, 0, sizeof(struct lcp_task));

        /* Set task unique identifier */
        task->tid = tid;
        task->uid = ctx->process_uid;
        task->ctx = ctx;

        /* Allocate table of communication context. */
        task->tcct = sctk_malloc(ctx->config.max_num_managers * 
                                 sizeof(lcp_task_comm_context_t *));
        if (task->tcct == NULL) {
                mpc_common_debug_error("LCP TASK: could not allocated task "
                                       "communication context table.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(task->tcct, 0, ctx->config.max_num_managers * 
               sizeof(lcp_task_comm_context_t *));

        /* Init task lock used for matching lists */
	mpc_common_spinlock_init(&task->task_lock, 0);

        /* Init memory pool of requests */
        mpc_mempool_param_t mp_req_params = {
                .field_mask = MPC_COMMON_MPOOL_INIT_CALLBACK,
                .alignment = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 512,
                .elem_size = sizeof(lcp_request_t) + ctx->config.request.size,
                .max_elems = UINT_MAX,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free,
                .obj_init_func = lcp_task_request_init,
        };
                
        rc = mpc_mpool_init(&task->req_mp, &mp_req_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Init memory pool of requests */
        //FIXME: define suitable size for unexpected buffers. Could be a set of
        //       memory pools, based on size to be copied the appropriate memory
        //       pool would be chosen.
        mpc_mempool_param_t mp_unexp_params = {
                .alignment = MPC_COMMON_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 128,
                .elem_size = 8192,
                .max_elems = 1024,
                .malloc_func = sctk_malloc,
                .free_func = sctk_free
        };
                
        rc = mpc_mpool_init(&task->unexp_mp, &mp_unexp_params);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Compute task index used in */
        ctx->tasks[tid] = task;

        *task_p = task;
        mpc_common_debug_info("LCP: created task tid=%d", tid);
err:
        return rc;
}

int lcp_task_associate(lcp_task_h task, lcp_manager_h mngr)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        lcp_task_comm_context_t *tcc;

        assert(mngr != NULL);

        if (task->tcct[mngr->id] != NULL) {
                return rc;
        }

        tcc = sctk_malloc(sizeof(lcp_task_comm_context_t));
        if (tcc == NULL) {
                mpc_common_debug_error("LCP TASK: could not allocate task communication "
                                       "context.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        if (mngr->flags & LCP_MANAGER_TSC_MODEL) {
                tcc->tag.num_queues = UINT16_MAX + 1;
                tcc->tag.umqs = sctk_malloc(tcc->tag.num_queues * sizeof(mpc_queue_head_t));
                if (tcc->tag.umqs == NULL) {
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }

                tcc->tag.prqs = sctk_malloc(tcc->tag.num_queues * sizeof(mpc_queue_head_t));
                if (tcc->tag.prqs == NULL) {
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }

                for (i = 0; i < tcc->tag.num_queues; i++) {
                        mpc_queue_init_head(&tcc->tag.umqs[i]);
                        mpc_queue_init_head(&tcc->tag.prqs[i]);
                }

                /* Init table of user AM callbacks */
                tcc->am.handlers = sctk_malloc(LCP_AM_ID_USER_MAX * 
                                      sizeof(lcp_am_user_handler_t));
                if (tcc->am.handlers == NULL) {
                        mpc_common_debug_error("LCP TASK: Could not allocate "
                                               "user AM handlers");
                        rc = MPC_LOWCOMM_ERROR;
                        goto err;
                }
                memset(tcc->am.handlers, 0, LCP_AM_ID_USER_MAX * 
                       sizeof(lcp_am_user_handler_t));

        }

        task->num_managers++;
        task->tcct[mngr->id] = tcc;

        mpc_common_debug_info("LCP TASK: associated task. tid=%d, mngr id=%d.",
                              task->tid, mngr->id);
err:
        return rc;
}

int lcp_task_dissociate(lcp_task_h task, lcp_manager_h mngr)
{
        sctk_free(task->tcct[mngr->id]->tag.prqs);
        sctk_free(task->tcct[mngr->id]->tag.umqs);
        sctk_free(task->tcct[mngr->id]->am.handlers);

        sctk_free(task->tcct[mngr->id]);

        task->tcct[mngr->id] = NULL;
        
        return MPC_LOWCOMM_SUCCESS;
}

int lcp_task_fini(lcp_task_h task) {

        mpc_mpool_fini(&task->req_mp);
        mpc_mpool_fini(&task->unexp_mp);

        //TODO: assert that all tack communication context are dissociated.

        sctk_free(task);
        task = NULL;

        return MPC_LOWCOMM_SUCCESS;
}
