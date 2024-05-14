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

#include "lcp_context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpc_common_debug.h>
#include <mpc_common_rank.h>
#include <mpc_common_helper.h>
#include <sctk_alloc.h>
#include <mpc_common_flags.h>
#include <bitmap.h>

#include "lowcomm_config.h"

#include "lcp.h"
#include "lcp_task.h"

#include "lcr_component.h"
#include "lcr_def.h"

//FIXME: add context param to decide whether to allocate context statically or
//       dynamically.
static lcp_context_h static_ctx = NULL;

/**
 * Get the resources of a component.
 * 
 * component component to get the resources from
 * devices_p resources (out)
 * num_devices_p number of resources
 * int MPI_SUCCESS in case of success
 */
static int _lcp_context_query_component_devices(lcr_component_h component,
                                                lcr_device_t **devices_p,
                                                unsigned *num_devices_p)
{
        int rc = 0;
        lcr_device_t *devices = NULL;
        unsigned num_devices = 0;

        /* Get all available devices */
        rc = component->query_devices(component, &devices, &num_devices);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                mpc_common_debug("LCP: error querying component: %s",
                                 component->name);
                goto err;
        }

        if (num_devices == 0) {
                mpc_common_debug_warning("LCP: no device found for component: %s",
                                 component->name);
        }

        *num_devices_p = num_devices;
        *devices_p     = devices;

        rc = MPC_LOWCOMM_SUCCESS;

err:
        return rc;
}



/**
 * Initialize resources with components and device name
 *
 * resource_p resource to initialize
 * component components used to fill the resources
 * device used for resource name
 */
static inline void _lcp_context_resource_init(lcp_rsc_desc_t *resource_p,
                                              lcr_component_h component,
                                              lcr_device_t *device)
{
        size_t device_name_length = strlen(device->name) + 1;

        /* Init resource */
        resource_p->iface_config  = component->rail_config;
        resource_p->driver_config = component->driver_config;
        resource_p->priority      = component->rail_config->priority;
        resource_p->component     = component;
        resource_p->name          = sctk_malloc(sizeof(char*) * device_name_length);
        strncpy(resource_p->name, device->name, device_name_length);
        resource_p->used          = 0;
}

/**
 * This function is used to sort the rail array by priority before unfolding resources
 * 
 * pa pointer to pointer of rail (qsort)
 * pb pointer to pointer of rail (qsort)
 * int order function result
 */
int __sort_rails_by_priority(const void * pa, const void *pb)
{
        struct _mpc_lowcomm_config_struct_net_rail **a = (struct _mpc_lowcomm_config_struct_net_rail**)pa;
        struct _mpc_lowcomm_config_struct_net_rail **b = (struct _mpc_lowcomm_config_struct_net_rail**)pb;

        if ( *a == NULL && *b != NULL) {
                return 1;
        } else if ( *a != NULL && *b == NULL) {
                return -1;
        } else if ( *a == NULL && *b == NULL) {
                return 0;
        } else {
                return (*a)->priority < (*b)->priority;
        }
}

#if 0
static inline int __init_rails(lcp_context_h ctx)
{
        ctx->num_resources = resource_count;
        ctx->resources = sctk_malloc(sizeof(lcp_rsc_desc_t) * resource_count);
        assume(ctx->resources);
        ctx->progress_counter = sctk_malloc(sizeof(int) * resource_count);
        assume(ctx->progress_counter);

        /* Walk again to initialize each resource */
        resource_count = 0;

        //FIXME: To be moved to manager.
        unsigned int l = 0;
        for(k = 0; k < ctx->num_cmpts; ++k)
	{
                struct lcr_component * tmp = &ctx->cmpts[k];
                unsigned int num_devices = mpc_common_min((int)ctx->cmpts[k].num_devices,
                                                   sorted_rails[k]->max_ifaces);
                for(l=0; l < num_devices; l++)
                {
                        lcp_context_resource_init(&ctx->resources[resource_count],
                                                  tmp,
                                                  &tmp->devices[l]);
                        ctx->progress_counter[resource_count] = ctx->resources[resource_count].iface_config->priority;
                        resource_count++;
                }
        }


        /* Compute total priorities */

        unsigned int total_priorities = 0;

        for(k = 0; k < (unsigned int)ctx->num_resources; ++k)
	{
                total_priorities += ctx->progress_counter[k];
        }

        unsigned int max_prio = 0;

        /* Normalize priorities */
        for(k = 0; k < (unsigned int)ctx->num_resources; ++k)
	{
                ctx->progress_counter[k] = ctx->progress_counter[k] * 100 / total_priorities;
                /* And minimum 1 */
                if(ctx->progress_counter[k] == 0)
                {
                        ctx->progress_counter[k] = 1;
                }

                if(max_prio < ctx->progress_counter[k])
                {
                        max_prio = ctx->progress_counter[k];
                }

        }

        assume(max_prio > 0);

        /* Make Max Prio to be 256 (max of unsigned char) and scale others */
        for(k = 0; k < (unsigned int)ctx->num_resources; ++k)
	{
                ctx->progress_counter[k] = ctx->progress_counter[k] * 256 / max_prio;
                mpc_common_debug("%s has polling frequency %d", ctx->resources[k].name, ctx->progress_counter[k]);
        }

        /* It is this value which will wrap around to elect progressed rails */
        ctx->current_progress_value = 0;


        return MPC_LOWCOMM_SUCCESS;
}
#endif

/**
 * This function generates the listing of the network configuration when passing -v to mpcrun
 * 
 * ctx the context to generate the description of
 * int 0 on success
 */
#define NETWORK_DESC_BUFFER_SIZE (8*1024llu)
static inline void _lcp_context_log_resource_config_summary(lcp_context_h ctx)
{
        unsigned int i = 0;
        unsigned int j = 0;

        if(mpc_common_get_flags()->sctk_network_description_string)
        {
                sctk_free(mpc_common_get_flags()->sctk_network_description_string);
        }
        mpc_common_get_flags()->sctk_network_description_string = 
                sctk_malloc(NETWORK_DESC_BUFFER_SIZE);

        /* Initialize network description string. */
        char * name = mpc_common_get_flags()->sctk_network_description_string;
        *name = '\0';

        char tmp[512];

        for(i = 0 ; i < ctx->num_cmpts; i++)
        {
                lcr_component_h cmt = ctx->components[i];

                (void)snprintf(tmp, 512, "\n - %s\n", cmt->name);
                strncat(name, tmp, NETWORK_DESC_BUFFER_SIZE - 1);

                for(j = 0 ; j < cmt->num_devices; j++)
                {
                        (void)snprintf(tmp, 512, " \t* %s\n", cmt->devices[j].name);
                        strncat(name, tmp, NETWORK_DESC_BUFFER_SIZE - 1);
                }
        }

        strncat(name, "\nInitialized resources:\n", NETWORK_DESC_BUFFER_SIZE - 1);

        int k = 0;
        for(k = 0; k < ctx->num_resources; k++)
        {
                lcp_rsc_desc_t *res = &ctx->resources[k];
                (void)snprintf(tmp, 512, " - [%d] %s %s (%s, %s)\n", res->priority, 
                               res->name, res->component->name, res->iface_config->name, 
                               res->driver_config->name);
                strncat(name, tmp, NETWORK_DESC_BUFFER_SIZE- 1);
        }
}

static int _lcp_context_load_ctx_config(lcp_context_h ctx, lcp_context_param_t *param)
{
        int i, j, net_found;
        int rc = MPC_LOWCOMM_SUCCESS; 
        lcr_rail_config_t **rail_configs = NULL;
        int num_configs, tmp_num_configs; 
        lcr_component_h *components = NULL;
        int num_components;

        /* First, load context configuration. */
        struct _mpc_lowcomm_config_struct_protocol * config = 
                _mpc_lowcomm_config_proto_get();

        ctx->config.multirail_enabled    = config->multirail_enabled;
	ctx->config.rndv_mode            = (lcp_rndv_mode_t)config->rndv_mode;
        ctx->config.max_num_managers     = 128; //FIXME: should be added to
                                                //       lowcomm_config.
        ctx->process_uid                 = param->process_uid;

        /* Load the rail configurations from CLI. */
        rc = _mpc_lowcomm_conf_load_rail_from_cli(&rail_configs, &num_configs);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err_free;
        }

        /* Load and init available components. */
        rc = lcr_query_components(&components, (unsigned int *)&num_components);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err_free;
        }

        /* Check that rails asked by user through CLI can be instanciated. */
        for (i = 0; i < num_configs; i++) {
                net_found = 0;
                for (j = 0; j < num_components; j++) {
                        if (strcmp(rail_configs[i]->name, 
                                    components[j]->name) == 0) {
                                net_found = 1;
                        }
                }
                if (!net_found) {
                        mpc_common_debug_error("LCP CTX: requested %s but no rail "
                                               "available with such name.", 
                                               rail_configs[i]->name);
                        rc = MPC_LOWCOMM_ERROR;
                        goto err_free;
                }
        }

        /* Filter CLI rails configs based on process/task layout. */
        tmp_num_configs = num_configs;
        if( (mpc_common_get_process_count() == 1) && 
            (mpc_common_get_task_count() > 1) ) {
                for(i=0; i < num_configs; i++) {
                        if( strcmp(rail_configs[i]->name, "tbsm") != 0) {
                                rail_configs[i]   = NULL;
                                tmp_num_configs--;
                        }
                }
        }

        assert(tmp_num_configs >= 0);
        if (tmp_num_configs == 0) {
                mpc_common_debug_error("LCP CTX: no rail asked from the CLI "
                                       "satisfies the requirements.");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free;
        }

        /* Check offload capabilities. */
        int has_offload = 0;
        ctx->config.offload = 0;
        for (i = 0; i < num_configs; i++) {
                if(rail_configs[i]->offload) {
                        has_offload = 1;
                        ctx->config.offload = 1;
                }
        }

        /* Count the number of non-composable rail for multirail checks. */
        int non_composable_count = 0;
        for (i = 0; i < num_configs; i++) {
                if (rail_configs[i] == NULL) break;
                if (!rail_configs[i]->composable) {
                        non_composable_count++;
                } else { /* There can be only one interface for composable rail. */
                        assert(rail_configs[i]->max_ifaces == 1);
                }
        }

        /* In case multirail is enable and multiple configs are available, make
         * sure that there is at most one non-composable rail. */
        if (non_composable_count > 1 && ctx->config.multirail_enabled) {
                mpc_common_debug_error("LCP CTX: heterogeous non-composable"
                                       " multirail not supported.");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free;
        }

        /* Similarly, if multirail not enabled but CLI specified multiple non
         * composable rail, then error out. */
        if (!ctx->config.multirail_enabled && non_composable_count > 1) {
                mpc_common_debug_error("LCP CTX: multirail disabled but cli "
                                       "specified multiple non-composable rail.");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free;
        }

        /* Throw warning for multirail and max_iface incoherences. */
        for (i = 0; i < num_configs; i++) {
                if (rail_configs[i] == NULL) break;
                if (!ctx->config.multirail_enabled && 
                    rail_configs[i]->max_ifaces > 1) {
                        mpc_common_debug_warning("LCP CTX: multirail not enabled "
                                                 "but max_ifaces > 1 for rail %s",
                                                 rail_configs[i]->name);
                        rail_configs[i]->max_ifaces = 1;
                }
        }

        /* Check offload configuration. */
        if (ctx->config.offload && 
            mpc_common_get_process_count() != (int)mpc_common_get_task_count()) {
                mpc_common_debug_error("LCP CTX: offload must be run in "
                                       "process mode.");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free;
        }

        if (ctx->config.offload && !has_offload) {
                mpc_common_debug_error("LCP CTX: offload requested but no available "
                                       "interface to support it");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free;
        }

        /* Sort config by priority and copy filtered list to context. */
        qsort(rail_configs, num_configs, sizeof(lcr_rail_config_t *), 
              __sort_rails_by_priority);

        /* Retain only components requested by user. */
        ctx->num_cmpts  = num_configs;
        ctx->components = sctk_malloc(num_configs * sizeof(lcr_component_h));
        if (ctx->components == NULL) {
                mpc_common_debug_error("LCP CTX: could not allocate context "
                                       "components.");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free;
        }
        for (i = 0; (unsigned int)i < ctx->num_cmpts; i++) {
                for (j = 0; j < num_components; j++) {
                        if (!strcmp(rail_configs[i]->name, components[j]->name)) {
                                ctx->components[i] = components[j];
                                /* Also set their config. */
                                ctx->components[i]->rail_config   = rail_configs[i];
                                ctx->components[i]->driver_config = 
                                        _mpc_lowcomm_conf_driver_unfolded_get(rail_configs[i]->config);
                        }
                }
        }

        /* At that point, context is configured with the priority ordered list
         * of rail configs and components that will be used during run. */
err_free:
        if (rail_configs) sctk_free(rail_configs);
        if (components)   sctk_free(components);

        return rc;
}

static int _lcp_context_devices_load_and_filter(lcp_context_h ctx)
{
        int rc, i, j;
        int total_num_devices = 0, num_offload_devices = 0;
        bmap_t dev_map = MPC_BITMAP_INIT; // device map.

        /* Load all available devices. */
        for (i = 0; (unsigned int)i < ctx->num_cmpts; i++) {
                rc = _lcp_context_query_component_devices(ctx->components[i],
                                                          &ctx->components[i]->devices,
                                                          &ctx->components[i]->num_devices);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err_free;
                }
                if (ctx->components[i]->rail_config->offload && ctx->config.offload) {
                        num_offload_devices += ctx->components[i]->num_devices;
                }
                total_num_devices += ctx->components[i]->num_devices;
        }
        if (ctx->config.offload && num_offload_devices == 0) {
                mpc_common_debug_error("LCP CTX: offloading capabilities were "
                                       "requested but no device that supports it found.");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free;
        }

        if (total_num_devices == 0) {
                mpc_common_debug_error("LCP CTX: could not find any devices "
                                       "for the provided list of rail.");
                rc = MPC_LOWCOMM_ERROR;
                goto err_free;
        }

        /* Device filtering depends on device's name provided by the
         * configuration and if multirail is enabled:
         * - if rail is composable (shm or tbsm), then pick it.
         * - if device name is "any":
         *   - if multirail is enabled, then pick the first max_ifaces specified
         *   by the configuration.
         *   - else pick the first device from the list //FIXME: no topology.
         * - else (a list of device name is provided):
         *   max_ifaces config is overwritten, pick all devices found. */ 
        int num_dev = 0;
        int dev_index = 0;
        for (i = 0; (unsigned)i < ctx->num_cmpts; i++) {
                if (ctx->components[i]->num_devices == 0) {
                        dev_index++;
                        break;
                }
                if (ctx->components[i]->rail_config->composable) {
                        /* There can be only one iface for composable rail such
                         * as tbsm or shm. */
                        MPC_BITMAP_SET(dev_map, dev_index);
                        dev_index++; num_dev++;
                } else {
                        if (strcmp(ctx->components[i]->rail_config->device, "any") == 0) {
                                for (j = 0; j < ctx->components[i]->rail_config->max_ifaces; j++) {
                                        MPC_BITMAP_SET(dev_map, dev_index);
                                        dev_index++; num_dev++;
                                }
                        } else {
                                /* Here, we override max_ifaces config. */
                                for (j = 0; (unsigned int)j < ctx->components[j]->num_devices; j++) {
                                        if (strstr(ctx->components[i]->rail_config->device, 
                                                   ctx->components[i]->devices[j].name)) {
                                                MPC_BITMAP_SET(dev_map, dev_index);
                                                num_dev++;
                                        }
                                        dev_index++;
                                }
                        }
                }
        }

        /* Once all transport devices have been set on the device map, allocate
         * and fill up the resource table. */
        int rsc_count = 0;
        dev_index = 0;
        ctx->num_resources = num_dev;
        ctx->resources     = sctk_malloc(num_dev * sizeof(lcp_rsc_desc_t));
        for (i = 0; (unsigned int)i < ctx->num_cmpts; i++) {
                for (j = 0 ; (unsigned int)j < ctx->components[i]->num_devices; j++) {
                        if (MPC_BITMAP_GET(dev_map, dev_index)) {
                                _lcp_context_resource_init(&ctx->resources[rsc_count],
                                                           ctx->components[i],
                                                           &ctx->components[i]->devices[j]);
                                rsc_count++;
                        }
                        dev_index++;
                }
        }

        //TODO: add progress counter.

err_free:
        for (i = 0; (unsigned int)i < ctx->num_cmpts; i++) {
                if (ctx->components[i]->num_devices > 0) {
                        sctk_free(ctx->components[i]->devices);
                }
        }

        return rc;
}

int _lcp_context_structures_init(lcp_context_h ctx)
{
        int rc = MPC_LOWCOMM_SUCCESS;

        ctx->num_managers = 0;

        /* Preallocate a table of manager handles. */
        //NOTE: Upon manager creation, see lcp_manager_create, a handle will be
        //      atomically retrieved by incrementing the num_managers.
        ctx->mngrt = sctk_malloc(ctx->config.max_num_managers * 
                                 sizeof(lcp_manager_h));
        if (ctx-> mngrt == NULL) {
                mpc_common_debug_error("LCP CTX: could not allocated "
                                       "manager table.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        memset(ctx->mngrt, 0, ctx->config.max_num_managers * sizeof(lcp_manager_h));

        /* Init hash table of tasks */
	ctx->tasks = sctk_malloc(ctx->num_tasks * sizeof(lcp_task_h));
	if (ctx->tasks == NULL) {
		mpc_common_debug_error("LCP CTX: Could not allocate tasks table.");
		rc = MPC_LOWCOMM_ERROR;
                goto err;
	}
        //NOTE: to release tasks, lcp_context_fini relies on the fact that all entries 
        //      are memset to NULL. 
        memset(ctx->tasks, 0, ctx->num_tasks * sizeof(lcp_task_h));

        return rc;
err:
        if (ctx->mngrt) sctk_free(ctx->mngrt);

        return rc;
}

int lcp_context_create(lcp_context_h *ctx_p, lcp_context_param_t *param)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = {0};

        //FIXME: should context creation be thread-safe ?
        if (static_ctx != NULL) {
                *ctx_p = static_ctx;
                return MPC_LOWCOMM_SUCCESS;
        }

        /* Parameter check. */
        //FIXME: number of tasks is currently number of MPI ranks. This is
        //       because the number of local tasks with
        //       mpc_common_get_local_task_count() is not set yet. There might
        //       be another way.
        if ( (param->field_mask & LCP_MANAGER_NUM_TASKS) && 
             (param->num_tasks <= 0) ) {
                mpc_common_debug_error("LCP CTX: wrong number of tasks.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        if (!(param->field_mask & LCP_CONTEXT_PROCESS_UID)) {
                mpc_common_debug_error("LCP CTX: process uid not set.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        //FIXME: test if param is valid
        if (!(param->field_mask & LCP_CONTEXT_DATATYPE_OPS)) {
                mpc_common_debug_error("LCP CTX: datatype operations not set.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

	/* Allocate context */
	ctx = sctk_malloc(sizeof(struct lcp_context));
	if (ctx == NULL) {
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(ctx, 0, sizeof(struct lcp_context));

        ctx->dt_ops      = param->dt_ops;
        ctx->process_uid = param->process_uid;
        ctx->num_tasks   = param->num_tasks;

        if (param->field_mask & LCP_CONTEXT_REQUEST_SIZE) {
                ctx->config.request.size = param->request_size;
        }
        if (param->field_mask & LCP_CONTEXT_REQUEST_SIZE) {
                ctx->config.request.init = param->request_init;
        }

        rc = _lcp_context_load_ctx_config(ctx, param);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto out_free_ctx;
        }

        rc = _lcp_context_devices_load_and_filter(ctx);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto out_free_ctx;
        }

        rc = _lcp_context_structures_init(ctx);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto out_free_ctx;
        }

        _lcp_context_log_resource_config_summary(ctx);

	*ctx_p = static_ctx = ctx;

	return MPC_LOWCOMM_SUCCESS;

out_free_ctx:
        sctk_free(ctx);
err:
	return rc;
}

int lcp_context_fini(lcp_context_h ctx)
{
	int i;

        //FIXME: free devices
	for (i=0; i<ctx->num_resources; i++) {
                sctk_free(ctx->devices);
                free(ctx->resources[i].name);
	}
	sctk_free(ctx->resources);
        sctk_free(ctx->progress_counter);

        /* Free task table */
        for (i = 0; i < ctx->num_tasks; i++) {
                if (ctx->tasks[i] != NULL) {
                        lcp_task_fini(ctx->tasks[i]);
                }
        }
        sctk_free(ctx->tasks);

	sctk_free(ctx);

	return MPC_LOWCOMM_SUCCESS;
}

/* Returns the manager identifier. */
int lcp_context_register(lcp_context_h ctx, lcp_manager_h mngr) 
{
        int i = 1;
        LCP_CONTEXT_LOCK(ctx);

        /* Get first available slot in manager table. */
        while (ctx->mngrt[i] != NULL) i++;

        ctx->num_managers++;
        ctx->mngrt[i] = mngr;

        LCP_CONTEXT_UNLOCK(ctx);

        return i;
}

void lcp_context_unregister(lcp_context_h ctx, int manager_id) 
{
        LCP_CONTEXT_LOCK(ctx);
        ctx->mngrt[manager_id] = NULL;
        LCP_CONTEXT_UNLOCK(ctx);
}

lcp_task_h lcp_context_task_get(lcp_context_h ctx, int tid)
{
        return ctx->tasks[tid];
}

