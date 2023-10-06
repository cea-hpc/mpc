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

#include <mpc_common_rank.h>


#include "lcp.h"
#include "lcp_common.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_debug.h"
#include "mpc_lowcomm_types.h"

#include "lcp_def.h"
#include "lcp_context.h"


#include "sctk_alloc.h"

#include "lcp_request.h"


#include "sctk_alloc_posix.h"
#include "uthash.h"

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
        task->prq_table = lcp_prq_match_table_init();
        task->umq_table = lcp_umq_match_table_init();
        if (task->prq_table == NULL || task->umq_table == NULL) {
                mpc_common_debug_error("LCP: could not allocate prq "
                                       "or umq table for tid=%d", tid);
                rc = LCP_ERROR;
                goto err;
        }

        /* Init task lock used for matching lists */
	mpc_common_spinlock_init(&task->task_lock, 0);

        /* Init memory pool of requests */
        task->req_mp = sctk_malloc(sizeof(ck_stack_t));
        mpc_mpool_param_t mp_req_params = {
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
        task->unexp_mp = sctk_malloc(sizeof(ck_stack_t));
        mpc_mpool_param_t mp_unexp_params = {
                .alignment = MPC_SYS_CACHE_LINE_SIZE,
                .elem_per_chunk = 64,
                .elem_size = 8192,
                .max_elems = 512,
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

        /* Compute task index used in */
        task_idx = tid % ctx->num_tasks;
        ctx->tasks[task_idx] = task;

        *task_p = task;
        mpc_common_debug_info("LCP: created task tid=%d with idx=%d", tid, task_idx);
err:
        return rc;
}

int lcp_task_fini(lcp_task_h task) {

        lcp_fini_matching_engine(task->umq_table, task->prq_table);

        sctk_free(task->umq_table);
        sctk_free(task->prq_table);

        mpc_mpool_fini(task->req_mp);

        sctk_free(task);
        task = NULL;

        return LCP_SUCCESS;
}
