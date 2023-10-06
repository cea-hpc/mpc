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

#include "lcp_tag_matching.h"
#include "mpc_mempool.h"

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

        mpc_mpool_t *req_mp;   /* Request memory pool */
        mpc_mpool_t *unexp_mp; /* Unexpected memory pool */
};

int lcp_task_fini(lcp_task_h task);

#endif
