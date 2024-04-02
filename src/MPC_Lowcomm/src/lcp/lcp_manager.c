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

#include "lcp_manager.h"

#include "lcp_context.h"
#include "lcr_component.h"
#include "lcp_pending.h"
#include "rail.h"
#include "lcp_task.h"
#include "lcp_mem.h"
#include "lcp_ep.h" //FIXME: for lcp_manager_fini but maybe add a lcp_ep_close
                    //       to the API. Let the user store the endpoints handles.

#include "sctk_alloc.h"

lcp_am_handler_t lcp_am_handlers[LCP_AM_ID_LAST] = {{NULL, 0}};

static inline int _lcp_manager_set_am_handler(lcp_manager_h mngr, 
                                             sctk_rail_info_t *iface)
{
	int rc = 0;
        int am_id = 0;

	for (am_id=0; am_id<LCP_AM_ID_LAST; am_id++) {
		
		rc = lcr_iface_set_am_handler(iface, am_id, 
					      lcp_am_handlers[am_id].cb, 
					      mngr, 
					      lcp_am_handlers[am_id].flags);

		if (rc != MPC_LOWCOMM_SUCCESS) {
                        mpc_common_debug_error("LCP MNGR: could not register am "
                                               "handler.");
			break;
		}
	}

	return rc;
}

static int _lcp_manager_register_iface_attrs(lcp_manager_h mngr)
{
        int i;
        int rc = MPC_LOWCOMM_SUCCESS;
        sctk_rail_info_t *iface = NULL;

        mpc_list_init_head(&mngr->progress_head);

        for (i = 0; i < mngr->num_ifaces; i++) {
                iface = mngr->ifaces[i];

                /* Register interfaces that need progression and am handlers. */
                if (iface->iface_progress != NULL) {
                        mpc_list_push_head(&mngr->progress_head, &iface->progress);
                }

		rc = _lcp_manager_set_am_handler(mngr, iface);
		if (rc != MPC_LOWCOMM_SUCCESS) 
			break;
        }

        return rc;
}

static int _lcp_manager_init_structures(lcp_manager_h mngr)
{
        int rc = MPC_LOWCOMM_SUCCESS;

        mpc_common_hashtable_init(&mngr->eps, mngr->num_eps);

        mpc_common_spinlock_init(&mngr->mngr_lock, 0);

        /* Init hash table of tasks */
	mngr->tasks = sctk_malloc(mngr->num_tasks * sizeof(lcp_task_h));
	if (mngr->tasks == NULL) {
		mpc_common_debug_error("LCP MNGR: Could not allocate tasks table.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
        //NOTE: to release tasks, lcp_context_fini relies on the fact that all entries 
        //      are memset to NULL. 
        memset(mngr->tasks, 0, mngr->num_tasks * sizeof(lcp_task_h));

        /* Init pending matching message request table */
	mngr->match_ht = lcp_pending_init();
	if (mngr->match_ht == NULL) {
		mpc_common_debug_error("LCP MNGR: Could not allocate matching pending "
                                       "tables.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

        /* init match uid when using match request */
        OPA_store_int(&mngr->muid, 0);

        /* Initialize memory management unit. */
        rc = lcp_pinning_mmu_init(&mngr->mmu, 0 /* unused */);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        mpc_list_init_head(&mngr->memory_list);

        /* Initialize interfaces. */
        mngr->ifaces = sctk_malloc(mngr->ctx->num_resources * sizeof(sctk_rail_info_t *));
        if (mngr->ifaces == NULL) {
                mpc_common_debug_error("LCP MNGR: could not allocate interfaces.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        mngr->num_ifaces = mngr->ctx->num_resources;

err:
        return rc;
}

static int _lcp_manager_open_interfaces(lcp_manager_h mngr)
{
	int rc = 0;
        int i = 0;
        lcp_rsc_desc_t *rsc = NULL;
        lcp_context_h ctx   = mngr->ctx;

	for (i=0; i<ctx->num_resources; i++) {
                /* Build required transport capabilities. */
                rsc = &ctx->resources[i];
                rc = rsc->component->iface_open(rsc->name, i,
				                rsc->iface_config,
                                                rsc->driver_config,
                                                &mngr->ifaces[i],
                                                mngr->flags);
		if (mngr->ifaces[i] == NULL) {
			goto err;
		}

                //FIXME: in case of offload with multirail, the priority rail
                //       chosen will be the first interface. Maybe we could find
                //       a more formal way of setting it.
                if (ctx->config.offload && i == 0) {
                        mngr->priority_iface = i;
                }
	}
	
	rc = MPC_LOWCOMM_SUCCESS;
err:
	return rc;
}

int lcp_manager_create(lcp_context_h ctx, 
                       lcp_manager_h *mngr_p, 
                       lcp_manager_param_t *params)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_manager_h mngr = NULL;

        //FIXME: maybe use context init check function.
        if (ctx == NULL) {
                mpc_common_debug_error("LCP MNGR: context not initialized.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        if (!(params->field_mask & LCP_MANAGER_ESTIMATED_EPS) && 
            params->estimated_eps < 0) {
                mpc_common_debug_error("LCP MNGR: must specify an correct "
                                       "estimated number of endpoints.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        //FIXME: number of tasks is currently number of MPI ranks. This is
        //       because the number of local tasks with
        //       mpc_common_get_local_task_count() is not set yet. There might
        //       be another way.
        if ( (params->field_mask & LCP_MANAGER_NUM_TASKS) && 
            (params->num_tasks <= 0) ) {
                mpc_common_debug_error("LCP MNGR: wrong number of tasks");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        if ( !(params->field_mask & LCP_MANAGER_COMM_MODEL) ) {
                mpc_common_debug_error("LCP MNGR: must specify at least one "
                                       "communication model.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        mngr = sctk_malloc(sizeof(struct lcp_manager));
        if (mngr == NULL) {
                mpc_common_debug_error("LCP MNGR: could not allocate "
                                       "manager.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(mngr, 0, sizeof(struct lcp_manager));

        /* Initialize parameter fields. */
        mngr->ctx       = ctx;
        mngr->num_tasks = params->num_tasks;
        mngr->num_eps   = params->estimated_eps;
        mngr->flags     = params->flags;

        rc = _lcp_manager_init_structures(mngr);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = _lcp_manager_open_interfaces(mngr);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        rc = _lcp_manager_register_iface_attrs(mngr);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }
        
        *mngr_p = mngr;

err:
        //TODO: resource release in case of error.
        return rc;
}

int lcp_manager_fini(lcp_manager_h mngr)
{
        int i;
        int rc = MPC_LOWCOMM_SUCCESS;

        lcp_pinning_mmu_release(mngr->mmu);

	/* Free allocated endpoints */
	lcp_ep_ctx_t *e_ep = NULL;

	MPC_HT_ITER(&mngr->eps, e_ep)
        {
                lcp_ep_delete(e_ep->ep);
		sctk_free(e_ep);
	}
        MPC_HT_ITER_END(&mngr->eps);

        mpc_common_hashtable_release(&mngr->eps);

        /* Free task table */
        for (i = 0; i < mngr->num_tasks; i++) {
                if (mngr->tasks[i] != NULL) {
                        lcp_task_fini(mngr->tasks[i]);
                }
        }
        sctk_free(mngr->tasks);

        for (i = 0; i < mngr->num_ifaces; i++) {
                if (mngr->ifaces[i]->driver_finalize != NULL) {
                        mngr->ifaces[i]->driver_finalize(mngr->ifaces[i]);
                }
        }
        sctk_free(mngr->ifaces);

        sctk_free(mngr);

        return rc;
}

lcp_task_h lcp_manager_task_get(lcp_manager_h mngr, int tid)
{
        return mngr->tasks[tid];
}

