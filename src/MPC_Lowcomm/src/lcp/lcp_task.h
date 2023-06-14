#ifndef LCP_TASK_H
#define LCP_TASK_H

#include "lcp_def.h"

#include "lcp_tag_matching.h"

#define LCP_TASK_LOCK(_task) \
	mpc_common_spinlock_lock(&((_task)->task_lock))
#define LCP_TASK_UNLOCK(_task) \
	mpc_common_spinlock_unlock(&((_task)->task_lock))

typedef struct lcp_am_user_handler {
        lcp_am_callback_t cb;
        void *user_arg;
        uint64_t flags;
} lcp_am_user_handler_t;

struct lcp_task {
        int tid; /* task identifier */

        lcp_context_h ctx;

        mpc_common_spinlock_t task_lock;

        //FIXME: pointer not needed
        //FIXME: table lock not needed
	lcp_prq_match_table_t *prq_table; /* Posted Receive Queue */
	lcp_umq_match_table_t *umq_table; /* Unexpected Message Queue */

        lcp_am_user_handler_t *am; /* Table of user AM callbacks */

};

int lcp_task_fini(lcp_task_h task);

#endif
