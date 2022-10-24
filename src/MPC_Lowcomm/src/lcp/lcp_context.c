#include "lcp_context.h"

#include <string.h>
#include <stdio.h>

#include "sctk_alloc.h"
#include <lcr/lcr_component.h>
#include <uthash.h>

//TODO: memset to 0 all allocated structure (especially those containing
//      pointers). Creates non-null valid dummy pointers that can segfault 
//      later on.
//TODO: global variable to support notify new comm.
//	 try to enforce stateless architecture.
lcp_context_h context = NULL;

lcp_am_handler_t lcp_am_handlers[MPC_LOWCOMM_MSG_LAST] = {{NULL, 0}};

lcp_context_h lcp_context_get() {
	return context;
}

int lcp_context_has_comm(lcp_context_h ctx, uint64_t comm_key)
{
	lcp_comm_ctx_t *elem = NULL;

	HASH_FIND(hh, ctx->comm_ht, &comm_key, sizeof(mpc_lowcomm_communicator_id_t), elem);
	return elem ? 1 : 0;
}

//NOTE: hack to add comm to portals tables
int lcp_context_add_comm(lcp_context_h ctx, uint64_t comm_key)
{
	int rc;
	int i;

	lcp_comm_ctx_t *elem = sctk_malloc(sizeof(lcp_comm_ctx_t));
	if (elem == NULL) {
		mpc_common_debug_error("LCP: Could not allocate comm entry.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	HASH_ADD(hh, ctx->comm_ht, comm_key, sizeof(mpc_lowcomm_communicator_id_t), elem);

	mpc_lowcomm_communicator_t comm = 
		mpc_lowcomm_get_communicator_from_id(comm_key);
	size_t comm_size = 
		mpc_lowcomm_communicator_size(comm);

	for (i=0; i<ctx->num_resources; i++) {
		sctk_rail_info_t *rail = ctx->resources[i].iface;
		if (rail->notify_new_comm)
			rail->notify_new_comm(rail, comm_key, comm_size);
	}

	rc = MPC_LOWCOMM_SUCCESS;
err:
	return rc;
}

static inline int lcp_context_set_am_handler(lcp_context_h ctx, 
                                             sctk_rail_info_t *iface)
{
	int rc, am_id;

	for (am_id=0; am_id<MPC_LOWCOMM_MSG_LAST; am_id++) {
		
		rc = lcr_iface_set_am_handler(iface, am_id, 
					      lcp_am_handlers[am_id].cb, 
					      ctx, 
					      lcp_am_handlers[am_id].flags);

		if (rc != MPC_LOWCOMM_SUCCESS) {
			break;
		}
	}

	return rc;
}

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
		if (rc != MPC_LOWCOMM_SUCCESS) 
			goto err;
	}
	
	rc = MPC_LOWCOMM_SUCCESS;
err:
	return rc;
}


static int lcp_context_init_structures(lcp_context_h ctx)
{
        int rc;

	/* Init endpoint and fragmentation structures */
	mpc_common_spinlock_init(&(ctx->ctx_lock), 0);

	ctx->pend_send_req = sctk_malloc(sizeof(lcp_pending_table_t));
	ctx->pend_recv_req = sctk_malloc(sizeof(lcp_pending_table_t));
	if (ctx->pend_send_req == NULL || ctx->pend_recv_req == NULL) {
		mpc_common_debug_error("LCP: Could not allocate pending tables.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(ctx->pend_send_req, 0, sizeof(lcp_pending_table_t));
	memset(ctx->pend_recv_req, 0, sizeof(lcp_pending_table_t));
	mpc_common_spinlock_init(&ctx->pend_send_req->table_lock, 0);
	mpc_common_spinlock_init(&ctx->pend_recv_req->table_lock, 0);

	ctx->umq_table = sctk_malloc(sizeof(lcp_umq_match_table_t));
	ctx->prq_table = sctk_malloc(sizeof(lcp_prq_match_table_t));
	if (ctx->umq_table == NULL || ctx->prq_table == NULL) {
		mpc_common_debug_error("LCP: Could not allocate pending tables.");
		rc = MPC_LOWCOMM_ERROR;
		goto out_free_pending_tables;
	}
	memset(ctx->umq_table, 0, sizeof(lcp_umq_match_table_t));
	memset(ctx->prq_table, 0, sizeof(lcp_umq_match_table_t));
	mpc_common_spinlock_init(&ctx->umq_table->lock, 0);
	mpc_common_spinlock_init(&ctx->prq_table->lock, 0);

	rc = MPC_LOWCOMM_SUCCESS;

        return rc;
out_free_pending_tables:
        sctk_free(ctx->pend_recv_req); sctk_free(ctx->pend_send_req);
err:
	return rc;
}

int lcp_context_config_parse_list(const char *cfg_list,
                                                 char ***list_p,
                                                 int *length_p)
{
        int length = 0, i = 0;
        char *token;
        char **list;

        token = strtok((char *)cfg_list, ",");
        while (token != NULL) {
                length++;
                token = strtok(NULL, ",");
        }
        
        if (length == 0) {
                return MPC_LOWCOMM_SUCCESS;
        }

        list = sctk_malloc(length * sizeof(char *));
        if (list == NULL) {
                mpc_common_debug_error("LCP: could not allocate parse list");
                return MPC_LOWCOMM_ERROR;
        }
        token = strtok((char *)cfg_list, ",");
        while (token != NULL) {
                if((list[i] = strdup(token)) == NULL) {
                        mpc_common_debug_error("LCP: could not allocate list token");
                        sctk_free(list);
                        return MPC_LOWCOMM_ERROR;
                }
                token = strtok(NULL, ",");
                i++;
        }

        *list_p   = list;
        *length_p = length;

        return MPC_LOWCOMM_SUCCESS;
}

static int lcp_context_config_init(lcp_context_h ctx, 
                                   lcr_component_h *components,
                                   unsigned num_components,
                                   lcr_protocol_config_t *config)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i, j;
        /* Check for multirail */
        ctx->config.multirail_enabled = config->multirail_enabled;

        /* Get selected transports */
        rc = lcp_context_config_parse_list(config->transports,
                                           &ctx->config.selected_components,
                                           &ctx->config.num_selected_components);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        /* Does not support heterogeous multirail */
        if (ctx->config.num_selected_components > 1) {
                mpc_common_debug_error("LCP: heterogeous multirail not supported");
                rc = MPC_LOWCOMM_ERROR;
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
                        rc = MPC_LOWCOMM_ERROR;
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
                        rc = MPC_LOWCOMM_ERROR;
                        goto free_component_list;
                }
        }


        /* Does not support multirail with tcp */
        if (!strcmp(ctx->config.selected_components[0], "tcp") &&
            config->multirail_enabled) {
                mpc_common_debug_warning("LCP: multirail not supported with tcp."
                                         " Disabling it...");
                ctx->config.multirail_enabled = 0;
        }

        /* Get selected devices */
        rc = lcp_context_config_parse_list(config->devices,
                                           &ctx->config.selected_devices,
                                           &ctx->config.num_selected_devices);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto free_component_list;
        }

        if (!strcmp(ctx->config.selected_components[0], "tcp") &&
            ctx->config.num_selected_devices > 1               &&
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

static int lcp_context_query_component_devices(lcr_component_h component,
                                               lcr_device_t **devices_p,
                                               unsigned *num_devices_p)
{
        int rc;
        lcr_device_t *devices;
        unsigned num_devices;

        /* Get all available devices */
        rc = component->query_devices(component, &devices, &num_devices);
        if (rc != MPC_LOWCOMM_SUCCESS) {
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

        rc = MPC_LOWCOMM_SUCCESS;

err:
        return rc;
}

static inline void lcp_context_resource_init(lcp_rsc_desc_t *resource_p,
                                             lcr_component_h component,
                                             lcr_device_t *device)
{
        /* Fetch configuration */
        lcr_rail_config_t * iface_config = 
                _mpc_lowcomm_conf_rail_unfolded_get((char *)component->rail_name);
        lcr_driver_config_t *driver_config =
                _mpc_lowcomm_conf_driver_unfolded_get(iface_config->config);

        /* Init resource */
        lcp_rsc_desc_t resource = {
                .iface_config  = iface_config,
                .driver_config = driver_config,
                .priority = iface_config->priority,
                .iface = NULL,
                .component = component
        };
        strcpy(resource.name, device->name);

        *resource_p = resource;
}

void lcp_context_select_component(lcp_context_h ctx,
                                  lcr_component_h *components,
                                  int num_components,
                                  int *cmpt_index_p,
                                  int *max_ifaces_p)
{
        int i, j;
        int max_ifaces = -1;
        int max_prio;
        int cmpt_idx = -1;
        lcr_component_h cmpt;
        lcr_rail_config_t *config;

        /* Multi component is not supported */
        max_ifaces = 0; max_prio = 0;
        for (i=0; i<(int)num_components; i++) {
                cmpt = components[i];
                config = _mpc_lowcomm_conf_rail_unfolded_get(cmpt->rail_name);
                if (!ctx->config.user_defined) {
                        /* Get max ifaces from highest priority rail */
                        if (max_prio < config->priority && cmpt->num_devices > 0) {
                                max_ifaces = ctx->config.multirail_enabled ? 
                                        config->max_ifaces : 1;
                                max_prio   = config->priority;
                                cmpt_idx   = i;
                        }
                } else {
                        for (j=0; j<(int)ctx->config.num_selected_components; j++) {
                                if (!strcmp(cmpt->name, ctx->config.selected_components[j]) &&
                                    cmpt->num_devices > 0) {
                                        cmpt_idx   = i;
                                        max_ifaces = ctx->config.multirail_enabled ? 
                                                config->max_ifaces : 1;
                                }
                        }
                }
        }

        *max_ifaces_p = max_ifaces;
        *cmpt_index_p = cmpt_idx;

        if (cmpt_idx >= 0) {
                mpc_common_debug_info("LCP: component %s was selected with %d max iface", 
                                      components[cmpt_idx]->name, max_ifaces);
        }

}


static inline int lcp_context_resource_in_config(lcp_context_h ctx,
                                                 lcr_device_t *device)
{
        int i;

        for (i=0; i<(int)ctx->config.num_selected_devices; i++) {
                if (!strcmp(device->name, ctx->config.selected_devices[i]) ||
                    !strcmp(ctx->config.selected_devices[i], "any")) {
                        return 1;
                }
        }
        
        return 0;
}

static int lcp_context_add_resources(lcp_context_h ctx, 
                                     lcr_component_h *components,
                                     unsigned num_components)
{
	int rc = MPC_LOWCOMM_SUCCESS;
        int max_ifaces, cmpt_idx;
        int i, j, nrsc;
        lcr_component_h cmpt;
        lcr_device_t dev;

        /* First, query all available devices */
        for (i=0; i<(int)num_components; i++) {
                rc = lcp_context_query_component_devices(components[i],
                                                         &components[i]->devices,
                                                         &components[i]->num_devices);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto out_free_devices;
                }
        }

        lcp_context_select_component(ctx, components, num_components, 
                                     &cmpt_idx, &max_ifaces);
        if (max_ifaces == 0) {
                mpc_common_debug_error("LCP: could not find any interface");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        /* Allocate resources */
        ctx->resources = sctk_malloc(max_ifaces * sizeof(*ctx->resources));
        if (ctx->resources == NULL) {
                mpc_common_debug_error("LCP: could not allocate resources");
                rc = MPC_LOWCOMM_ERROR;
                goto out_free_devices;
        }
        ctx->num_resources = max_ifaces;

        cmpt = components[cmpt_idx]; nrsc = 0;
        for (j=0; j<(int)cmpt->num_devices; j++) {
                dev = cmpt->devices[j];
                if (!ctx->config.user_defined && nrsc < max_ifaces) {
                        lcp_context_resource_init(&ctx->resources[nrsc],
                                                  cmpt, &dev);
                        nrsc++;
                } else if (lcp_context_resource_in_config(ctx, &dev) && 
                           nrsc < max_ifaces) {
                        lcp_context_resource_init(&ctx->resources[nrsc],
                                                  cmpt, &dev);
                        nrsc++;
                }
        }

        if (nrsc == 0) {
                mpc_common_debug_error("LCP: no devices found in list:"); 
                for (j=0; j<(int)cmpt->num_devices; j++) {
                        mpc_common_debug_error("LCP:\t %d) %s", j, 
                                               cmpt->devices[j].name);
                }
                rc = MPC_LOWCOMM_ERROR;
        }

        return rc;

out_free_devices:
        for (j=0; j<i; i++) {
                sctk_free(components[j]->devices);
        }

err:
        return rc;
}

int lcp_context_create(lcp_context_h *ctx_p, __UNUSED__ unsigned flags)
{
	int rc, i;
	lcp_context_h ctx;	
        lcr_component_h *components;
        unsigned num_components;

	/* Allocate context */
	ctx = sctk_malloc(sizeof(struct lcp_context));
	if (ctx == NULL) {
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(ctx, 0, sizeof(struct lcp_context));

	/* init random seed to generate unique msg_id */
	rand_seed_init();

        /* Get all available components */
        rc = lcr_query_components(&components, &num_components);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto out_free_ctx;
        }
        ctx->num_cmpts = num_components;

        /* Init context config */
        rc = lcp_context_config_init(ctx, components, num_components,
                                     _mpc_lowcomm_config_proto_get());
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto out_free_components;
	}

	/* Init context config with resources */
	rc = lcp_context_add_resources(ctx, components, num_components);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto out_free_ctx_config;
	}

	/* Allocate rail interface */
	rc = lcp_context_open_interfaces(ctx);
	if (rc != MPC_LOWCOMM_SUCCESS) {
		goto out_free_resources;
	}

        rc = lcp_context_init_structures(ctx);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto out_free_ifaces;
        }

	*ctx_p = ctx;
	context = ctx; //TODO: global variable for accessibility in communicator.c
                       
        lcr_free_components(components, num_components, 1);

	return MPC_LOWCOMM_SUCCESS;

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

int lcp_context_fini(lcp_context_h ctx)
{
	int i;
        lcp_pending_fini(ctx->pend_send_req,
                         ctx->pend_recv_req);
	sctk_free(ctx->pend_send_req);
	sctk_free(ctx->pend_recv_req);
	lcp_fini_matching_engine(ctx->umq_table,
				 ctx->prq_table);
	sctk_free(ctx->umq_table);
	sctk_free(ctx->prq_table);

	/* Free allocated comm */
	lcp_comm_ctx_t *e_comm = NULL, *e_comm_tmp = NULL;
	HASH_ITER(hh, ctx->comm_ht, e_comm, e_comm_tmp) {
		HASH_DELETE(hh, ctx->comm_ht, e_comm);
		sctk_free(e_comm);
	}

	/* Free allocated endpoints */
	lcp_ep_ctx_t *e_ep = NULL, *e_ep_tmp = NULL; 
	HASH_ITER(hh, ctx->ep_ht, e_ep, e_ep_tmp) {
		for (i=0; i<e_ep->ep->num_chnls; i++) {
			sctk_free(e_ep->ep->lct_eps[i]);
		}
		sctk_free(e_ep->ep->lct_eps);
		sctk_free(e_ep->ep);
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
	sctk_free(ctx);

	return MPC_LOWCOMM_SUCCESS;
}

