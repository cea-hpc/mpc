#include "lcp_context.h"
#include "lcp_common.h"
#include "lcp_ep.h"

#include <string.h>
#include <stdio.h>
#include <alloca.h>

#include <sctk_alloc.h>
#include <lowcomm_config.h>
#include <rail.h>

#include <lcr/lcr_component.h>
#include "lcp_task.h"
#include <uthash.h>

//TODO: memset to 0 all allocated structure (especially those containing
//      pointers). Creates non-null valid dummy pointers that can segfault 
//      later on.

lcp_am_handler_t lcp_am_handlers[LCP_AM_ID_LAST] = {{NULL, 0}};
//FIXME: add context param to decide whether to allocate context statically or
//       dynamically.
static int lcp_context_is_initialized = 0;
static lcp_context_h static_ctx = NULL;

static int lcp_context_check_if_valid(lcp_context_h ctx)
{
        int rc = LCP_SUCCESS, i;
        int has_offload = 0;
        lcp_rsc_desc_t rsc;
        lcr_rail_config_t *iface_config;
        lcr_driver_config_t *driver_config;

        for (i = 0; i<ctx->num_resources; i++) {
                rsc = ctx->resources[i];
		iface_config = _mpc_lowcomm_conf_rail_unfolded_get(rsc.component->rail_name);

                if (iface_config->offload) {
                        has_offload = 1;
                        break;
                }
        }

        if (ctx->config.offload && ctx->num_cmpts > 1 && has_offload) {
                mpc_common_debug_error("LCP CONTEXT: offload interface not "
                                       "supported with heterogenous multirail");
                rc = LCP_ERROR;
                goto err;
        } 
err:
        return rc;
}

static inline int lcp_context_set_am_handler(lcp_context_h ctx, 
                                             sctk_rail_info_t *iface)
{
	int rc, am_id;

	for (am_id=0; am_id<LCP_AM_ID_LAST; am_id++) {
		
		rc = lcr_iface_set_am_handler(iface, am_id, 
					      lcp_am_handlers[am_id].cb, 
					      ctx, 
					      lcp_am_handlers[am_id].flags);

		if (rc != LCP_SUCCESS) {
			break;
		}
	}

	return rc;
}

/**
 * @brief Open all resources from a context.
 * 
 * @param ctx context to open
 * @return int MPI_SUCCESS in case of success
 */
static int lcp_context_open_interfaces(lcp_context_h ctx)
{
	int rc, i;
        lcp_rsc_desc_t *rsc;

	for (i=0; i<ctx->num_resources; i++) {
                rsc = &ctx->resources[i];
                rc = rsc->component->iface_open(rsc->name, i,
				                rsc->iface_config,
                                                rsc->driver_config,
                                                &rsc->iface);
		if (rsc->iface == NULL) {
			goto err;
		}

		rc = lcp_context_set_am_handler(ctx, rsc->iface);
		if (rc != LCP_SUCCESS) 
			goto err;
	}
	
	rc = LCP_SUCCESS;
err:
	return rc;
}

/**
 * @brief Alloc and initialize structures in context
 * 
 * @param ctx context to initialize
 * @return int MPI_SUCCESS in case of success
 */
static int lcp_context_init_structures(lcp_context_h ctx)
{
        int rc;

	/* Init endpoint and fragmentation structures */
	mpc_common_spinlock_init(&(ctx->ctx_lock), 0);

        /* Init pending matching message request table */
	ctx->match_ht = sctk_malloc(sizeof(lcp_pending_table_t));
	if (ctx->match_ht == NULL) {
		mpc_common_debug_error("LCP: Could not allocate matching pending tables.");
		rc = LCP_ERROR;
		goto err;
	}
	memset(ctx->match_ht, 0, sizeof(lcp_pending_table_t));
	mpc_common_spinlock_init(&ctx->match_ht->table_lock, 0);

        /* Init pending active message request table */
	ctx->pend = sctk_malloc(sizeof(lcp_pending_table_t));
	if (ctx->pend == NULL) {
		mpc_common_debug_error("LCP: Could not allocate am pending tables.");
		rc = LCP_ERROR;
		goto err;
	}
	memset(ctx->pend, 0, sizeof(lcp_pending_table_t));
	mpc_common_spinlock_init(&ctx->pend->table_lock, 0);

        /* Init hash table of tasks */
	ctx->tasks = sctk_malloc(sizeof(lcp_task_table_t));
	if (ctx->tasks == NULL) {
		mpc_common_debug_error("LCP: Could not allocate tasks table.");
		rc = LCP_ERROR;
		goto out_free_pending_tables;
	}
	memset(ctx->tasks, 0, sizeof(lcp_task_table_t));
	mpc_common_spinlock_init(&ctx->tasks->lock, 0);

	rc = LCP_SUCCESS;

        return rc;

out_free_pending_tables:
        sctk_free(ctx->pend); 
err:
	return rc;
}

static int str_in_list(char **list,
                       int length,
                       char *str)
{
        int i;

        for (i = 0; i < length; i++) {
                if (!strcmp(list[i], str)) {
                        return 1;
                }
        }
        return 0;
}

/**
 * @brief allocates and fill a (context) list by parsing cfg_list string using comma-separated lists
 * 
 * @param cfg_list config to be parsed
 * @param list_p out config list
 * @param length_p length of the config list
 * @return int 
 */
static int lcp_context_config_parse_list(const char *cfg_list,
                                         char ***list_p,
                                         int *length_p)
{
        int length = 0, i = 0;
        char *token;
        char *cfg_list_dup;
        char **list;

        /* Make a copy of config list */
        cfg_list_dup = sctk_malloc((strlen(cfg_list)+1) * sizeof(char));
        strcpy(cfg_list_dup, cfg_list);

        token = strtok(cfg_list_dup, ",");
        while (token != NULL) {
                length++;
                token = strtok(NULL, ",");
        }
        
        if (length == 0) {
                return LCP_SUCCESS;
        }

        list = sctk_malloc(length * sizeof(char *));
        if (list == NULL) {
                mpc_common_debug_error("LCP: could not allocate parse list");
                return LCP_ERROR;
        }

        /* Reset duplicate that has been changed with strtok */
        strcpy(cfg_list_dup, cfg_list);
        token = strtok(cfg_list_dup, ",");
        while (token != NULL) {
                //FIXME: memory leak with strdup
                if((list[i] = strdup(token)) == NULL) {
                        mpc_common_debug_error("LCP: could not allocate list token");
                        sctk_free(list);
                        return LCP_ERROR;
                }
                token = strtok(NULL, ",");
                i++;
        }

        *list_p   = list;
        *length_p = length;

        sctk_free(cfg_list_dup);

        return LCP_SUCCESS;
}

/**
 * @brief Initialize lcp context using mpc configuration.
 * 
 * @param ctx context to initialize
 * @param components components to initialize in the configuration
 * @param num_components number of components to initialize
 * @param config configuration to initialize in the context
 * @return int MPI_SUCCESS In case of success
 */
static int lcp_context_config_init(lcp_context_h ctx, 
                                   lcr_component_h *components,
                                   unsigned num_components,
                                   lcr_protocol_config_t *config)
{
        int rc = LCP_SUCCESS;
        int i, j;

        /* Check for multirail */
        ctx->config.multirail_enabled    = config->multirail_enabled;
	ctx->config.rndv_mode            = (lcp_rndv_mode_t)config->rndv_mode;
        ctx->config.offload              = config->offload;

        /* Get selected transports */
        rc = lcp_context_config_parse_list(config->transports,
                                           &ctx->config.selected_components,
                                           &ctx->config.num_selected_components);

        if (rc != LCP_SUCCESS) {
                goto err;
        }

        /* Does not support heterogeous multirail (tsbm always counted) */
        if (ctx->config.num_selected_components > 2) {
                mpc_common_debug_error("LCP: heterogeous multirail not supported");
                rc = LCP_ERROR;
                goto free_component_list;
        }

        /* If component is any, one with highest priority will be selected */
        /* No need to read devices names */
        if (!strcmp(ctx->config.selected_components[0], "any")) { 
                ctx->config.user_defined = 0;

                /* Set any device, in this case, the first discovered device
                 * will be chosen. */
                ctx->config.selected_devices     = sctk_malloc(sizeof(char *));
                if (ctx->config.selected_devices == NULL) {
                        mpc_common_debug_error("LCP: could not allocate device list");
                        rc = LCP_ERROR;
                        goto free_component_list;
                }

                ctx->config.selected_devices[0]  = strdup("any");
                ctx->config.num_selected_devices = 1;
                return rc;
        }

        ctx->config.user_defined = 1;
        int ok;
        for (i=0; i<ctx->config.num_selected_components; i++) {
                ok = 0;
                for (j=0; j<(int)num_components; j++) {
                        if (!strcmp(ctx->config.selected_components[i], 
                                    components[j]->name)) {
                                ok = 1;
                        }
                }
                if (!ok) {
                        mpc_common_debug_error("LCP: component %s is not supported.",
                                               ctx->config.selected_components[i]);
                        rc = LCP_ERROR;
                        goto free_component_list;
                }
        }

        /* Does not support multirail with tcp */
        if (str_in_list(ctx->config.selected_components,  
                        ctx->config.num_selected_components, "tcp") &&
            config->multirail_enabled) {
                mpc_common_debug_warning("LCP: multirail not supported with tcp."
                                         " Disabling it...");
                ctx->config.multirail_enabled = 0;
        }

        /* Get selected devices */
        rc = lcp_context_config_parse_list(config->devices,
                                           &ctx->config.selected_devices,
                                           &ctx->config.num_selected_devices);
        if (rc != LCP_SUCCESS) {
                goto free_component_list;
        }

        if (str_in_list(ctx->config.selected_components,  
                        ctx->config.num_selected_components, "tcp") &&
            ctx->config.num_selected_devices > 1                    &&
            !ctx->config.multirail_enabled) {
                mpc_common_debug_warning("LCP: cannot use multiple device with tcp."
                                         " Using only iface: %s.", 
                                         ctx->config.selected_devices[0]);
                ctx->config.num_selected_devices = 1;
        }

        return rc;

free_component_list:
        sctk_free(ctx->config.selected_components);

err:
        return rc;
}

/**
 * @brief Get the resources of a component.
 * 
 * @param component component to get the resources from
 * @param devices_p resources (out)
 * @param num_devices_p number of resources
 * @return int MPI_SUCCESS in case of success
 */
static int lcp_context_query_component_devices(lcr_component_h component,
                                               lcr_device_t **devices_p,
                                               unsigned *num_devices_p)
{
        int rc;
        lcr_device_t *devices;
        unsigned num_devices;

        /* Get all available devices */
        rc = component->query_devices(component, &devices, &num_devices);
        if (rc != LCP_SUCCESS) {
                mpc_common_debug("LCP: error querying component: %s",
                                 component->name);
                goto err;
        }

        if (num_devices == 0) {
                mpc_common_debug("LCP: no resources found for component: %s",
                                 component->name);
        }

        *num_devices_p = num_devices;
        *devices_p     = devices;

        rc = LCP_SUCCESS;

err:
        return rc;
}

/**
 * @brief Initialize resources with components and device name
 * 
 * @param resource_p resource to initialize
 * @param component components used to fill the resources
 * @param device used for resource name
 */
static inline void lcp_context_resource_init(lcp_rsc_desc_t *resource_p,
                                             lcr_component_h component,
                                             lcr_device_t *device)
{
        /* Fetch configuration */
        lcr_rail_config_t *iface_config = 
                _mpc_lowcomm_conf_rail_unfolded_get((char *)component->rail_name);
        lcr_driver_config_t *driver_config =
                _mpc_lowcomm_conf_driver_unfolded_get(iface_config->config);

        /* Init resource */
        resource_p->iface_config  = iface_config;
        resource_p->driver_config = driver_config;
        resource_p->iface         = NULL;
        resource_p->priority      = iface_config->priority;
        resource_p->component     = component;
        strcpy(resource_p->name, device->name);
}

//FIXME: refacto. code too difficult to understand...
static void lcp_context_select_components(lcp_context_h ctx,
                                          lcr_component_h *components,
                                          int num_components,
                                          int **cmpt_index_p,
                                          int *max_ifaces_p)
{
        int i, j;
        int max_prio, cmpt_cnt;
        int self_added = 0;
        int *cmpt_idx;
        int max_ifaces = 0;
        lcr_component_h cmpt;
        lcr_rail_config_t *config;

        /* Multi component is not supported (except with SHM) */
        cmpt_cnt = 0; max_prio = 0;
        cmpt_idx = alloca(num_components * sizeof(int));
        memset(cmpt_idx, -1, num_components * sizeof(int));
	for (i=0; i<(int)num_components; i++) {
		cmpt = components[i];
		config = _mpc_lowcomm_conf_rail_unfolded_get(cmpt->rail_name);
		if (!ctx->config.user_defined) {
			/* Get max ifaces from highest priority rail */
			if ((max_prio < config->priority && cmpt->num_devices > 0) ||
                            (config->self && !self_added)) {
                                if (config->self) 
                                        self_added = 1;
                                max_ifaces += ctx->config.multirail_enabled ? 
                                        config->max_ifaces : 1;
				max_prio           = config->priority;
				cmpt_idx[cmpt_cnt] = i;
                                cmpt_cnt++;
			}
		} else {
			for (j=0; j<(int)ctx->config.num_selected_components; j++) {
				if ((!strcmp(cmpt->name, ctx->config.selected_components[j]) &&
						cmpt->num_devices > 0)) {
					cmpt_idx[cmpt_cnt] = i;
                                        max_ifaces += ctx->config.multirail_enabled ?
                                                LCP_MIN(config->max_ifaces, (int)cmpt->num_devices) : 1;
                                        cmpt_cnt++;
				}
			}
		}
	}

        *max_ifaces_p = max_ifaces;
        memcpy(*cmpt_index_p, cmpt_idx, num_components * sizeof(int));

        for (i = 0; i < cmpt_cnt; i++) {
                mpc_common_debug_info("LCP: component %s was selected.", 
                                      components[cmpt_idx[i]]->name);
        }
        mpc_common_debug_info("LCP: total number of interface is %d.", 
                              max_ifaces);
}

/**
 * @brief Return true if device is in config.
 * 
 * @param ctx context containing selected devices
 * @param device device to check
 * @return int MPI_SUCCESS in case of success
 */
static inline int lcp_context_resource_in_config(lcp_context_h ctx,
                                                 lcr_device_t *device)
{
        int i;

        for (i=0; i<(int)ctx->config.num_selected_devices; i++) {
                if (!strcmp(device->name, ctx->config.selected_devices[i]) ||
                    !strcmp(ctx->config.selected_devices[i], "any") || 
                    !strcmp(ctx->config.selected_devices[i], "tbsm")) {
                        return 1;
                }
        }
        
        return 0;
}

/**
 * @brief Add resources / devices to a context.
 * 
 * @param ctx context to add the resources to
 * @param components resources to add to the context
 * @param num_components number of resources to add
 * @return int MPI_SUCCESS in case of success
 */
static int lcp_context_add_resources(lcp_context_h ctx, 
                                     lcr_component_h *components,
                                     unsigned num_components)
{
	int rc = LCP_SUCCESS;
        int max_ifaces, idx; 
        int *cmpt_idx;
        int i, j, nrsc = 0, nrsc_per_cmpt;
        lcr_component_h cmpt;
        lcr_rail_config_t *config;
        lcr_device_t dev;

        /* First, query all available devices */
        for (i=0; i<(int)num_components; i++) {
                rc = lcp_context_query_component_devices(components[i],
                                                         &components[i]->devices,
                                                         &components[i]->num_devices);
                if (rc != LCP_SUCCESS) {
                        goto out_free_devices;
                }
        }

        cmpt_idx = alloca(num_components * sizeof(int));
        memset(cmpt_idx, -1, num_components * sizeof(int));
        lcp_context_select_components(ctx, components, num_components, 
                                      &cmpt_idx, &max_ifaces);
	//FIXME: mpc_print_config cannot abort otherwise mpcrun also abort and
	//       nothing is launched
        //if (max_ifaces == 0) {
        //        mpc_common_debug_error("LCP: could not find any interface");
        //        rc = LCP_ERROR;
        //        goto err;
        //}

        /* Allocate resources */
        ctx->resources = sctk_malloc(max_ifaces * sizeof(*ctx->resources));
        if (ctx->resources == NULL) {
                mpc_common_debug_error("LCP: could not allocate resources");
                rc = LCP_ERROR;
                goto out_free_devices;
        }
        ctx->num_resources = max_ifaces;

        nrsc = 0;
        for (i = 0; i < (int)num_components; i++) {
                if ((idx = cmpt_idx[i]) < 0) 
                        continue;
                cmpt = components[idx];
		config = _mpc_lowcomm_conf_rail_unfolded_get(cmpt->rail_name);
                nrsc_per_cmpt = 0;
                for (j=0; j<(int)cmpt->num_devices; j++) {
                        dev = cmpt->devices[j];
                        if (!ctx->config.user_defined && nrsc < max_ifaces) {
                                lcp_context_resource_init(&ctx->resources[nrsc],
                                                          cmpt, &dev);
                                nrsc++; nrsc_per_cmpt++;
                        } else if (lcp_context_resource_in_config(ctx, &dev) && 
                                   nrsc < max_ifaces) {
                                lcp_context_resource_init(&ctx->resources[nrsc],
                                                          cmpt, &dev);
                                nrsc++; nrsc_per_cmpt++;
                        }

                        if (nrsc_per_cmpt == config->max_ifaces) 
                                break;
                }
        }

        if (nrsc == 0) {
                mpc_common_debug_error("LCP: no devices found in list:"); 
                for (i = 0; i < (int)num_components; i++) {
                        if ((idx = cmpt_idx[i]) < 0) 
                                continue;
                        cmpt = components[idx];
                        for (j=0; j<(int)cmpt->num_devices; j++) {
                                mpc_common_debug_error("LCP:\t %d) %s", j, 
                                                       cmpt->devices[j].name);
                        }
                }
                rc = LCP_ERROR;
        }

        return rc;

out_free_devices:
        for (j=0; j<i; i++) {
                sctk_free(components[j]->devices);
        }

        return rc;
}

void lcp_context_task_get(lcp_context_h ctx, int tid, lcp_task_h *task_p)
{
        lcp_task_entry_t *item;

	HASH_FIND(hh, ctx->tasks->table, &tid, sizeof(int), item);
	if (item == NULL) {
                *task_p = NULL;
        } else {
                *task_p = item->task;
        }
}

int lcp_context_create(lcp_context_h *ctx_p, lcp_context_param_t *param)
{
	int rc, i;
	lcp_context_h ctx;	
        lcr_component_h *components;
        unsigned num_components;

        //FIXME: should context creation be thread-safe ?
        if (lcp_context_is_initialized) {
                *ctx_p = static_ctx;
                return LCP_SUCCESS;
        }

	/* Allocate context */
	ctx = sctk_malloc(sizeof(struct lcp_context));
	if (ctx == NULL) {
		rc = LCP_ERROR;
		goto err;
	}
	memset(ctx, 0, sizeof(struct lcp_context));

        //FIXME: test if param is valid
        if (param->flags & LCP_CONTEXT_DATATYPE_OPS) {
                ctx->dt_ops = param->dt_ops;
        }

        if (!(param->flags & LCP_CONTEXT_PROCESS_UID)) {
                mpc_common_debug_error("LCP: process uid not set");
                rc = LCP_ERROR;
                goto out_free_ctx;
        }
        ctx->process_uid = param->process_uid;

	/* init random seed to generate unique msg_id */
	rand_seed_init();
        OPA_store_int(&ctx->msg_id, 0);

        /* init match uid when using match request */
        OPA_store_int(&ctx->muid, 0);

        /* Get all available components */
        rc = lcr_query_components(&components, &num_components);
        if (rc != LCP_SUCCESS) {
                goto out_free_ctx;
        }
        ctx->num_cmpts = num_components;

        /* Init context config */
        rc = lcp_context_config_init(ctx, components, num_components,
                                     _mpc_lowcomm_config_proto_get());
	if (rc != LCP_SUCCESS) {
		goto out_free_components;
	}

	/* Init context config with resources */
	rc = lcp_context_add_resources(ctx, components, num_components);
	if (rc != LCP_SUCCESS) {
		goto out_free_ctx_config;
	}

        rc = lcp_context_check_if_valid(ctx);
        if (rc != LCP_SUCCESS) {
                goto out_free_resources;
        }

	/* Allocate rail interface */
	rc = lcp_context_open_interfaces(ctx);
	if (rc != LCP_SUCCESS) {
		goto out_free_resources;
	}

        rc = lcp_context_init_structures(ctx);
        if (rc != LCP_SUCCESS) {
                goto out_free_ifaces;
        }

	*ctx_p = static_ctx = ctx;
        lcp_context_is_initialized = 1;
                       
        lcr_free_components(components, num_components, 1);

	return LCP_SUCCESS;

out_free_ifaces:
        for (i=0; i<ctx->num_resources; i++) {
                sctk_rail_info_t *iface = ctx->resources[i].iface;
                iface->driver_finalize(iface);
                sctk_free(ctx->resources[i].iface);
        }
out_free_resources:
        for (i=0; i<(int)num_components; i++) {
                sctk_free(components[i]->devices);
        }
        sctk_free(ctx->resources);
out_free_ctx_config:
        sctk_free(ctx->config.selected_components);
        sctk_free(ctx->config.selected_devices);
out_free_components:
        lcr_free_components(components, num_components, 0);
out_free_ctx:
        sctk_free(ctx);
err:
	return rc;
}

/**
 * @brief Release a context.
 * 
 * @param ctx context to free
 * @return int MPI_SUCCESS in case of success
 */
int lcp_context_fini(lcp_context_h ctx)
{
	int i;
        lcp_pending_fini(ctx->pend);
	sctk_free(ctx->pend);

	lcp_task_entry_t *e_task = NULL, *e_task_tmp = NULL;
	HASH_ITER(hh, ctx->tasks->table, e_task, e_task_tmp) {
                lcp_task_fini(e_task->task);
		HASH_DELETE(hh, ctx->tasks->table, e_task);
                sctk_free(e_task);
	}
        sctk_free(ctx->tasks);

	/* Free allocated endpoints */
	lcp_ep_ctx_t *e_ep = NULL, *e_ep_tmp = NULL; 
	HASH_ITER(hh, ctx->ep_ht, e_ep, e_ep_tmp) {
                lcp_ep_delete(e_ep->ep);
		HASH_DELETE(hh, ctx->ep_ht, e_ep);
		sctk_free(e_ep);
	}

	sctk_rail_info_t *iface;
	for (i=0; i<ctx->num_resources; i++) {
		iface = ctx->resources[i].iface; 
		if (iface->driver_finalize)
			iface->driver_finalize(iface);
		sctk_free(iface);
	}
	sctk_free(ctx->resources);

        /* config release */
        sctk_free(ctx->config.selected_components);

	sctk_free(ctx);

	return LCP_SUCCESS;
}
