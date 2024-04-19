/* ############################# MPC License ############################## */
/* # Fri Oct  6 12:44:31 CEST 2023                                        # */
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
/* # - Gilles Moreau <gilles.moreau@cea.fr>                               # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Paul Canat <pcanat@paratools.com>                                  # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef LCP_MANAGER_H
#define LCP_MANAGER_H

#include "lcp.h"
#include "lcr_def.h"

#include <mpc_common_datastructure.h>

#include <stdatomic.h>
#include <opa_primitives.h>
#include <list.h>
#include <queue.h>

#define LCP_MANAGER_LOCK(_mngr) \
	mpc_common_spinlock_lock_yield(&( (_mngr)->mngr_lock) )
#define LCP_MANAGER_UNLOCK(_mngr) \
	mpc_common_spinlock_unlock(&( (_mngr)->mngr_lock) )

/**
 * @brief handler holding a callback and a flag
 *
 */
typedef struct lcp_am_handler
{
	lcr_am_callback_t cb;
	uint64_t          flags;
} lcp_am_handler_t;

#define LCP_DEFINE_AM(_id, _cb, _flags)              \
	MPC_STATIC_INIT {                            \
		lcp_am_handlers[_id].cb    = _cb;    \
		lcp_am_handlers[_id].flags = _flags; \
	}

extern lcp_am_handler_t lcp_am_handlers[];

/**
 * @brief endpoint
 *
 */
typedef struct lcp_ep_ctx
{
	uint64_t       ep_key;
	lcp_ep_h       ep;
} lcp_ep_ctx_t;

struct lcp_manager {
        lcp_context_h         ctx;
        unsigned              flags;

        int                   id;  /* Manager identifier. */
        
	int                   num_eps;       /* number of endpoints created */
	struct mpc_common_hashtable eps; /* Hash table of endpoints for other sets */

	OPA_int_t             muid;          /* matching unique identifier */
	lcp_pending_table_t  *match_ht;      /* ht of matching request */

	mpc_common_spinlock_t mngr_lock;      /* Manager lock */

	lcp_task_h           *tasks;         /* LCP tasks (per thread data) */
        int                   num_tasks;     /* Number of tasks */

	mpc_queue_head_t      pending_queue; /* Queue of pending requests to be sent */

        mpc_list_elem_t       progress_head; /* List of interface registered for progress */

        struct lcp_pinning_mmu *mmu;

        int                   num_ifaces;
        sctk_rail_info_t    **ifaces;  /* Table of manager interfaces. */
        int                   priority_iface;
};

#endif 
