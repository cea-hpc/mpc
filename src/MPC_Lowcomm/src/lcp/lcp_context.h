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

#ifndef LCP_CONTEXT_H
#define LCP_CONTEXT_H

#include "lcp.h"
#include "lcp_def.h"
#include "lcr/lcr_def.h"
#include "lcp_types.h"
#include "lcp_pending.h"
#include "lcp_types.h"
#include "uthash.h"
#include "opa_primitives.h"
#include <mpc_common_datastructure.h>

#define LCP_CONTEXT_LOCK(_ctx) \
	mpc_common_spinlock_lock_yield(&( (_ctx)->ctx_lock) )
#define LCP_CONTEXT_UNLOCK(_ctx) \
	mpc_common_spinlock_unlock(&( (_ctx)->ctx_lock) )

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
	UT_hash_handle hh;

	uint64_t       ep_key;
	lcp_ep_h       ep;
} lcp_ep_ctx_t;

typedef struct lcp_match_ctx
{
	UT_hash_handle hh;

	int            muid_key;
	lcp_request_t *req;
} lcp_match_ctx_t;

typedef struct lcp_rsc_desc
{
	int                  priority;
	char                 name[LCP_CONF_STRING_SIZE];
	sctk_rail_info_t *   iface; /* Rail interface */
	lcr_rail_config_t *  iface_config;
	lcr_driver_config_t *driver_config;
	lcr_component_h      component;
	short       used;          /* Is the resource used in any endpoint (used to determine if progress needed)*/
} lcp_rsc_desc_t;

/**
 * @brief context configuration
 *
 */
typedef struct lcp_context_config
{
	int             multirail_enabled;
	int             multirail_heterogeneous_enabled;
	lcp_rndv_mode_t rndv_mode;
	int             offload;
        int             max_num_tasks;
} lcp_context_config_t;

/**
 * @brief context (configuration, devices, resources, queues)
 *
 */
struct lcp_context
{
	int                   priority_rail;

	lcp_context_config_t  config;

	unsigned              flags;

	OPA_int_t             msg_id;        /* unique message identifier */

	OPA_int_t             muid;          /* matching unique identifier */
	lcp_pending_table_t * match_ht;      /* ht of matching request */

	struct lcr_component *cmpts;         /* available component handles */
	unsigned              num_cmpts;     /* number of components */

	lcr_device_t *        devices;       /* available device descriptors */
	unsigned              num_devices;   /* number of devices */

	int                   num_resources; /* number of resources (iface) */
	lcp_rsc_desc_t *      resources;     /* opened resources (iface) */
	unsigned int *        progress_counter; /* Add a counter for priority-based polling */
	unsigned char         current_progress_value; /* This is the counter for progress calls */


	mpc_common_spinlock_t ctx_lock;      /* Context lock */

	int                   num_eps;       /* number of endpoints created */
	struct mpc_common_hashtable  ep_htable;         /* Hash table of created endpoint */

	uint64_t              process_uid;   /* process uid used for endpoint creation */

	mpc_queue_head_t      pending_queue; /* Queue of pending requests to be sent */

	lcp_task_h *          tasks;         /* LCP tasks (per thread data) */
        int                   num_tasks;     /* Number of tasks */

	lcp_dt_ops_t          dt_ops;        /* pack/unpack functions */
};

#endif
