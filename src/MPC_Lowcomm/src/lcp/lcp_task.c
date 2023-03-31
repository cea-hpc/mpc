#include "lcp_task.h"

#include "lcp_common.h"
#include "mpc_lowcomm_types.h"

#include "lcp_def.h"
#include "lcp_context.h"
#include "sctk_alloc.h"

#include "uthash.h"

int lcp_task_create(lcp_context_h ctx, int tid, lcp_task_h *task_p)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_task_entry_t *item = NULL;
        lcp_task_h task;

        assert(ctx); assert(tid >= 0);

        /* Check if tid already in context */
	HASH_FIND(hh, ctx->tasks->table, &tid, sizeof(int), item);
        if (item != NULL) {
                mpc_common_debug_warning("LCP: task with tid=%d already "
                                         "created.", tid);
                goto err;
        }

        task = sctk_malloc(sizeof(struct lcp_task));
        if (task == NULL) {
                mpc_common_debug_error("LCP: could not allocate task");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        /* Set task unique identifier */
        task->tid = tid;
        task->ctx = ctx;

        /* Allocate tag matching lists */
        task->prq_table = sctk_malloc(sizeof(lcp_prq_match_table_t));
        task->umq_table = sctk_malloc(sizeof(lcp_umq_match_table_t));
        if (task->prq_table == NULL || task->umq_table == NULL) {
                mpc_common_debug_error("LCP: could not allocate prq "
                                       "or umq table for tid=%d", tid);
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(task->prq_table, 0, sizeof(lcp_prq_match_table_t));
        memset(task->umq_table, 0, sizeof(lcp_umq_match_table_t));
	mpc_common_spinlock_init(&task->prq_table->lock, 0);
	mpc_common_spinlock_init(&task->umq_table->lock, 0);

        /* Init task lock used for matching lists */
	mpc_common_spinlock_init(&task->task_lock, 0);
        
        /* Insert task in context table */
        item = sctk_malloc(sizeof(lcp_task_entry_t));
        if (item == NULL) {
                mpc_common_debug_error("LCP: could not allocate "
                                       " task item");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(item, 0, sizeof(lcp_task_entry_t));

        item->task_key = tid;
        item->task = *task_p = task;

        //FIXME: which lock take ? Context lock or task table lock ?
        //       one of them is redundant.
        mpc_common_spinlock_lock(&(ctx->tasks->lock));
        HASH_ADD(hh, ctx->tasks->table, task_key, sizeof(int), item);
        mpc_common_spinlock_unlock(&(ctx->tasks->lock));

        mpc_common_debug_info("LCP: created task tid=%d", tid);
err:
        return rc;
}

int lcp_task_fini(lcp_task_h task) {

        lcp_fini_matching_engine(task->umq_table, task->prq_table);
        sctk_free(task->umq_table);
        sctk_free(task->prq_table);

        sctk_free(task);
        task = NULL;

        return MPC_LOWCOMM_SUCCESS;
}
