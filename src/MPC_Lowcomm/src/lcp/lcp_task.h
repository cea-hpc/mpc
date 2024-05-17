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

#ifndef LCP_TASK_H
#define LCP_TASK_H

#include "lcp_def.h"
#include "lcr/lcr_def.h"
#include "queue.h"

#include "lcp_tag_matching.h"
#include "mpc_mempool.h"

#define LCP_TASK_LOCK(_task) \
	mpc_common_spinlock_lock(&((_task)->task_lock))
#define LCP_TASK_UNLOCK(_task) \
	mpc_common_spinlock_unlock(&((_task)->task_lock))

typedef struct lcp_am_user_handler {
        lcp_am_user_func_t cb; /* User defined callback */
        void *user_arg; /* User data */
        uint64_t flags;
} lcp_am_user_handler_t;

struct lcp_task {
        int tid; /* task identifier */

        lcp_context_h ctx;

        mpc_common_spinlock_t task_lock;

        int num_queues;
        mpc_queue_head_t *umqs;
        mpc_queue_head_t *prqs;

        lcp_am_user_handler_t *am; /* Table of user AM callbacks */

        //TODO: investigate the bug when memory pools are not allocated
        //      dynamically with Concurrency Kit
        mpc_mempool_t *req_mp;   /* Request memory pool */
        mpc_mempool_t *unexp_mp; /* Unexpected memory pool */

        //TODO: implement reordering.
};

/* Function for sending data between two tasks */
int lcp_send_task_bcopy(lcp_request_t *req, lcr_pack_callback_t pack_cb,
                        unsigned flags, lcp_task_completion_t *comp);
int lcp_send_task_zcopy(lcp_request_t *req, lcp_task_completion_t *comp);
int lcp_task_fini(lcp_task_h task);

#endif
