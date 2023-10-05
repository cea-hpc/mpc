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


#include "lcp_common.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_debug.h"
#include "mpc_lowcomm_types.h"

#include "lcp_def.h"
#include "lcp_context.h"


#include "sctk_alloc.h"

#include "lcp_request.h"


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
        lcp_task_entry_t *item = NULL;
        lcp_task_h task;

        assert(ctx); assert(tid >= 0);

        /* This is what we want to be initialized once per Process (in task context)*/
        if(mpc_common_get_local_task_rank() == 0)
        {
                lcp_request_storage_init();
        }

        /* Check if tid already in context */
        item = mpc_common_hashtable_get(&ctx->tasks->task_table, tid);
        if (item != NULL) {
                *task_p = item->task;
                mpc_common_spinlock_unlock(&(ctx->ctx_lock));
                mpc_common_debug_warning("LCP: task with tid=%d already "
                                         "created.", tid);
                goto err;
        }

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
        
        /* Insert task in context table */
        item = sctk_malloc(sizeof(lcp_task_entry_t));
        if (item == NULL) {
                mpc_common_debug_error("LCP: could not allocate "
                                       " task item");
                rc = LCP_ERROR;
                goto err;
        }
        memset(item, 0, sizeof(lcp_task_entry_t));

        /* Init table of user AM callbacks */
        task->am = sctk_malloc(LCP_AM_ID_USER_MAX * sizeof(lcp_am_user_handler_t));
        if (task->am == NULL) {
                mpc_common_debug_error("LCP CTX: Could not allocated user AM handlers");
                rc = LCP_ERROR;
                goto err;
        }
        memset(task->am, 0, LCP_AM_ID_USER_MAX * sizeof(lcp_am_user_handler_t));


        item->task_key = tid;
        item->task = *task_p = task;

        mpc_common_hashtable_set(&ctx->tasks->task_table, item->task_key, item);

        mpc_common_debug_info("LCP: created task tid=%d", tid);
err:
        return rc;
}

int lcp_task_fini(lcp_task_h task) {

        lcp_fini_matching_engine(task->umq_table, task->prq_table);

        if(task->tid == 0)
        {
                lcp_request_storage_release();
        }

        sctk_free(task->umq_table);
        sctk_free(task->prq_table);

        sctk_free(task);
        task = NULL;

        return LCP_SUCCESS;
}
