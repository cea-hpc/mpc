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

#include "queue.h"
#include "mpc_mempool.h"

#include <stdatomic.h>

#define LCP_TASK_LOCK(_task) \
	mpc_common_spinlock_lock(&((_task)->task_lock))
#define LCP_TASK_UNLOCK(_task) \
	mpc_common_spinlock_unlock(&((_task)->task_lock))

#define LCP_TASK_MAX_COMM_CONTEXT 512

typedef struct lcp_am_user_handler {
        lcp_am_callback_t cb; /* User defined callback */
        void *user_arg; /* User data */
        uint64_t flags;
} lcp_am_user_handler_t;

//FIXME: study the possibility of using a union. A priori not possible since AM
//       and TAG are already being used together.
typedef struct lcp_task_comm_context {
        struct {
                int                    num_queues; /* Number of queues. */
                mpc_queue_head_t      *umqs; /* Unexpected Matching Queues. */
                mpc_queue_head_t      *prqs; /* Posted Receive Queues. */
        } tag;
        struct {
                lcp_am_user_handler_t *handlers; /* Table of user AM callbacks */
        } am;
        struct {
                uint32_t flush_counter;
                uint64_t outstandings;
        } rma;
} lcp_task_comm_context_t;

struct lcp_task {
        int tid; /* task identifier */
        uint64_t uid;

        lcp_context_h ctx; /* global context. */

        atomic_uint_fast32_t num_managers;
        lcp_task_comm_context_t **tcct; /* Task Communication Context table. */

        mpc_common_spinlock_t task_lock;
        //TODO: investigate the bug when memory pools are not allocated
        //      dynamically with Concurrency Kit
        mpc_mempool_t req_mp;   /* Request memory pool */
        mpc_mempool_t unexp_mp; /* Unexpected memory pool */

        //TODO: implement reordering.
};

/* Function for sending data between two tasks */
int lcp_task_fini(lcp_task_h task);

#endif
