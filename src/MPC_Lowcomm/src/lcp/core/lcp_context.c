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

#define MPC_MODULE "Lowcomm/LCP/Context"

// FIXME: add context param to decide whether to allocate context statically or
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
	int           rc          = 0;
	lcr_device_t *devices     = NULL;
	unsigned      num_devices = 0;

	/* Get all available devices */
	rc = component->query_devices(component, &devices, &num_devices);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug("Error querying component: %s",
			component->name);
		goto err;
	}

	if (num_devices == 0)
	{
		mpc_common_debug("No device found for component: %s",
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
	resource_p->name          = sctk_malloc(sizeof(char *) * device_name_length);
	strncpy(resource_p->name, device->name, device_name_length);
	resource_p->used = 0;
}

/**
 * This function is used to sort the rail array by priority before unfolding resources
 *
 * pa pointer to pointer of rail (qsort)
 * pb pointer to pointer of rail (qsort)
 * int order function result
 */
int __sort_rails_by_priority(const void *pa, const void *pb)
{
	struct _mpc_lowcomm_config_struct_net_rail **a = (struct _mpc_lowcomm_config_struct_net_rail **)pa;
	struct _mpc_lowcomm_config_struct_net_rail **b = (struct _mpc_lowcomm_config_struct_net_rail **)pb;

	if (*a == NULL && *b != NULL)
	{
		return 1;
	}
	else if (*a != NULL && *b == NULL)
	{
		return -1;
	}
	else if (*a == NULL && *b == NULL)
	{
		return 0;
	}
	else
	{
		return (*a)->priority < (*b)->priority;
	}
}

/**
 * @brief Generates the listing of the network configuration when passing -v to mpcrun
 *
 * @param[in] ctx Context to generate the description of
 */
#define NETWORK_DESC_BUFFER_SIZE (8 * 1024llu)
static inline void _lcp_context_log_resource_config_summary(lcp_context_h ctx)
{
	unsigned int i = 0;
	unsigned int j = 0;

	if (mpc_common_get_flags()->sctk_network_description_string)
	{
		sctk_free(mpc_common_get_flags()->sctk_network_description_string);
	}
	char *name =
		mpc_common_get_flags()->sctk_network_description_string = sctk_calloc(NETWORK_DESC_BUFFER_SIZE, sizeof(char));
	assert(name != NULL);

	// Updated available space to write in name string
	size_t available_space_name = NETWORK_DESC_BUFFER_SIZE - 1;
	// Maximum number of char to concatenate (min of tmp size and available space left in name)
	size_t max_char_to_cat;

	char tmp[512] = {};

	// List available network components
	for (i = 0 ; i < ctx->num_cmpts; i++)
	{
		lcr_component_h cmt = ctx->components[i];

		available_space_name -= snprintf(tmp, 512, "\n - %s\n", cmt->name);
		max_char_to_cat       = (available_space_name > 512 ? 512 : available_space_name);
		strncat(name, tmp, max_char_to_cat);

		// List available devices for each components
		for (j = 0 ; j < cmt->num_devices && available_space_name > 0; j++)
		{
			available_space_name -= snprintf(tmp, 512, " \t* %s\n", cmt->devices[j].name);
			max_char_to_cat       = (available_space_name > 512 ? 512 : available_space_name);
			strncat(name, tmp, max_char_to_cat);
		}
	}

	static const size_t constant_string_size = strlen("\nInitialized resources:\n");
	assert(available_space_name > constant_string_size);
	strcat(name, "\nInitialized resources:\n");

	int k = 0;
	// List initialized devices (and associated interfaces)
	for (k = 0; k < ctx->num_resources && available_space_name > 0; k++)
	{
		lcp_rsc_desc_t *res = &ctx->resources[k];

		available_space_name -= snprintf(tmp, 512, " - [%d] %s %s (%s, %s)\n",
			res->priority, res->name, res->component->name,
			res->iface_config->name, res->driver_config->name);

		max_char_to_cat = (available_space_name > 512 ? 512 : available_space_name);
		strncat(name, tmp, max_char_to_cat);
	}

	// Should be triggered only on truncation -> increase NETWORK_DESC_BUFFER_SIZE
	assert(available_space_name > 0 && "Please increase string size to print context");
}

static int _lcp_context_load_ctx_config(lcp_context_h ctx, lcp_context_param_t *param)
{
	int i, j, net_found;
	int rc = MPC_LOWCOMM_SUCCESS;
	lcr_rail_config_t **rail_configs = NULL;
	int num_configs;
	lcr_component_h *components = NULL;
	int num_components;

	/* First, load context configuration. */
	struct _mpc_lowcomm_config_struct_protocol *config =
		_mpc_lowcomm_config_proto_get();

	ctx->config.multirail_enabled = config->multirail_enabled;
	ctx->config.rndv_mode         = (lcp_rndv_mode_t)config->rndv_mode;
	ctx->config.max_num_managers  = 128;        // FIXME: should be added to lowcomm_config.
	ctx->process_uid = param->process_uid;


	/* Load the rail configurations from CLI. */
	rc = _mpc_lowcomm_conf_load_rail_from_cli(&rail_configs, &num_configs);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err_free;
	}
	mpc_common_debug("Loaded User Configurations:\n");
	for (int i = 0; i < num_configs; i++)
	{
		mpc_common_debug("\tCONF: %s\n", rail_configs[i]->name);
	}

	/* Load and init available components. */
	rc = lcr_query_components(&components, (unsigned int *)&num_components);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err_free;
	}
	mpc_common_debug("Loaded Available Components:\n");
	for (int i = 0; i < num_components; i++)
	{
		mpc_common_debug("\tCONF: %s\n", components[i]->name);
	}


	/* Check that rails asked by user through CLI can be instantiated. */
	for (i = 0; i < num_configs; i++)
	{
		net_found = 0;
		for (j = 0; j < num_components; j++)
		{
			if (strcmp(rail_configs[i]->name,
				components[j]->name) == 0)
			{
				net_found = 1;
			}
		}
		if (!net_found)
		{
			mpc_common_debug_error("LCP CTX: requested %s but no rail "
				                   "available with such name.",
				rail_configs[i]->name);
			rc = MPC_LOWCOMM_ERROR;
			goto err_free;
		}
	}

	/* Check offload capabilities. */
	ctx->config.offload = 0;
	for (i = 0; i < num_configs; i++)
	{
		if (rail_configs[i]->offload)
		{
			ctx->config.offload = 1;
		}
	}

	/* Throw warning for multirail and max_iface incoherences. */
	for (i = 0; i < num_configs; i++)
	{
		if (!ctx->config.multirail_enabled
		    && rail_configs[i]->max_ifaces > 1)
		{
			mpc_common_debug_warning("LCP CTX: multirail not enabled but max_ifaces > 1 for rail %s",
				rail_configs[i]->name);
			rail_configs[i]->max_ifaces = 1;
		}
	}

	/* Check offload configuration. */
	if (ctx->config.offload
	    && mpc_common_get_process_count() != (int)mpc_common_get_task_count())
	{
		// NOTE: this could be supported by implementing post
		//      cancellation on the offload interface in case a message
		//      has been sent eagerly.
		mpc_common_debug_error("LCP CTX: offload must be run in process mode.");
		rc = MPC_LOWCOMM_ERROR;
		goto err_free;
	}

	/* Sort config by priority and copy filtered list to context. */
	qsort(rail_configs, num_configs, sizeof(lcr_rail_config_t *),
		__sort_rails_by_priority);

	/* Retain only components requested by user. */
	ctx->num_cmpts  = num_configs;
	ctx->components = sctk_malloc(num_configs * sizeof(lcr_component_h));
	if (ctx->components == NULL)
	{
		mpc_common_debug_error("LCP CTX: could not allocate context "
			                   "components.");
		rc = MPC_LOWCOMM_ERROR;
		goto err_free;
	}
	for (i = 0; (unsigned int)i < ctx->num_cmpts; i++)
	{
		for (j = 0; j < num_components; j++)
		{
			if (!strcmp(rail_configs[i]->name, components[j]->name))
			{
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
	if (rail_configs)
	{
		sctk_free(rail_configs);
	}
	if (components)
	{
		sctk_free(components);
	}

	return rc;
}

static int _lcp_context_devices_load_and_filter(lcp_context_h ctx)
{
	int                rc;
	int                total_num_devices = 0, num_offload_devices = 0;
	bmap_t             dev_map = MPC_BITMAP_INIT; // device map.
	unsigned int       non_composable_actif     = 0;
	unsigned int       non_composable_total     = 0;
	lcr_component_h    non_composable_component = NULL;
	const unsigned int node_number    = mpc_common_get_node_count();
	const unsigned int process_number = mpc_common_get_process_count();

	/* Load all available devices. */
	for (unsigned int i = 0; i < ctx->num_cmpts; i++)
	{
		rc = _lcp_context_query_component_devices(ctx->components[i],
			&ctx->components[i]->devices,
			&ctx->components[i]->num_devices);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err_free;
		}

		// check non-composable active devices
		if (!ctx->components[i]->rail_config->composable)
		{
			non_composable_total++;
			if (ctx->components[i]->num_devices > 0)
			{
				non_composable_actif++;
				if (non_composable_actif > 1)
				{
					mpc_common_debug_fatal(
						"Heterogeous non-composable multirail not supported,"
						" 2 non composables components provided: %s & %s",
						non_composable_component == NULL ? non_composable_component->name : "",
						ctx->components[i]->name);
				}
				else
				{
					non_composable_component = ctx->components[i];
				}
			}
			else
			{
				mpc_common_debug_info("Found non-composable rail %s with 0 devices,"
					                  " rail will no be configured",
					ctx->components[i]->name);
			}
		}

		if (ctx->components[i]->rail_config->offload && ctx->config.offload)
		{
			num_offload_devices += ctx->components[i]->num_devices;
		}
		total_num_devices += ctx->components[i]->num_devices;
	}

	if (non_composable_total > 0 && non_composable_actif == 0 && node_number > 1)
	{
		mpc_common_debug_warning("No devices found for inter nodes communications but %d nodes requested:",
			node_number);
		for (unsigned int i = 0; i < ctx->num_cmpts; i++)
		{
			if (!ctx->components[i]->rail_config->composable)
			{
				mpc_common_debug_warning("\tNo device found for %s", ctx->components[i]->name);
			}
		}
	}

	if (ctx->config.offload && num_offload_devices == 0)
	{
		mpc_common_debug_error("Offloading capabilities were "
			                   "requested but no device that supports it found");
		rc = MPC_LOWCOMM_ERROR;
		goto err_free;
	}

	if (total_num_devices == 0)
	{
		mpc_common_debug_error("Could not find any devices "
			                   "for the provided list of rail");
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
	int nb_dev = 0;
	for (unsigned int i = 0; i < ctx->num_cmpts; i++)
	{
		if (ctx->components[i]->num_devices == 0)
		{
			mpc_common_debug("Skipping registration for component %s as no devices were founds",
				ctx->components[i]->name);
			continue;
		}

		if (ctx->components[i]->rail_config->composable)
		{
			// Skip shm if no need for it
			if ((strncmp("shm", ctx->components[i]->name, 3) == 0) && (node_number == process_number))
			{
				mpc_common_debug("Skipping registration for %s as only one process per node was requested",
					ctx->components[i]->name);
				continue;
			}
			// // May have multiple devices select only one
			// if (ctx->components[i]->num_devices > 1)
			// {
			// 	ctx->components[i]->query_device_nearest(&ctx->components[i]->devices,
			// 		&ctx->components[i]->num_devices);
			// }
			mpc_common_debug("Registering %s as composable component", ctx->components[i]->name);
			/* There can be only one iface for composable rail such as tbsm or shm. */
			MPC_BITMAP_SET(dev_map, i);
			nb_dev++;
			continue;
		}

		if (strcmp(ctx->components[i]->rail_config->device, "any") == 0)
		{
			if (ctx->config.multirail_enabled)
			{
				for (unsigned int j = 0; j < ctx->components[i]->num_devices; j++)
				{
					mpc_common_debug("Registering device %s for component %s, 'any' devices requested",
						ctx->components[i]->devices[j].name,
						ctx->components[i]->name);
					MPC_BITMAP_SET(dev_map, i);
					nb_dev++;
				}
			}
			else
			{
				const int ierr = ctx->components[i]->query_device_nearest(
					&ctx->components[i]->devices, &ctx->components[i]->num_devices);
				if (ierr != MPC_LOWCOMM_SUCCESS)
				{
					mpc_common_debug(
						"Failed to elect nearest device for component %s, 'any' devices requested and no multirail"
						": taking first in list %s",
						ctx->components[i]->name,
						ctx->components[i]->devices[0].name);
					ctx->components[i]->num_devices = 1;
				}
				else
				{
					mpc_common_debug("Registering device %s for component %s, 'any' devices requested and no multirail",
						ctx->components[i]->devices[0].name,
						ctx->components[i]->name);
				}
				MPC_BITMAP_SET(dev_map, i);
				nb_dev++;
			}
			continue;
		}

		/* Here, we override max_ifaces config. */
		for (unsigned int j = 0; j < ctx->components[i]->num_devices; j++)
		{
			if (strstr(ctx->components[i]->rail_config->device,
				ctx->components[i]->devices[j].name))
			{
				mpc_common_debug("Registering device %s for component %s, matching config request (%s)",
					ctx->components[i]->devices[j].name,
					ctx->components[i]->name,
					ctx->components[i]->rail_config->device);
				MPC_BITMAP_SET(dev_map, i);
				nb_dev++;
			}
			else
			{
				mpc_common_debug(
					"Not registering device %s for component %s, as device name does not match config (%s)",
					ctx->components[i]->devices[j].name,
					ctx->components[i]->name,
					ctx->components[i]->rail_config->device);
			}
		}
	}

	/* Once all transport devices have been set on the device map, allocate
	 * and fill up the resource table. */
	int rsc_count = 0;
	ctx->num_resources = nb_dev;
	ctx->resources     = sctk_malloc(nb_dev * sizeof(lcp_rsc_desc_t));
	for (unsigned int i = 0; i < ctx->num_cmpts; i++)
	{
		mpc_common_debug("Initializing resources for %s", ctx->components[i]->name);
		for (unsigned int j = 0 ; j < ctx->components[i]->num_devices; j++)
		{
			if (MPC_BITMAP_GET(dev_map, i))
			{
				mpc_common_debug("Initializing resource %s for %s",
					ctx->components[i]->devices[j].name, ctx->components[i]->name);

				_lcp_context_resource_init(&ctx->resources[rsc_count], ctx->components[i],
					&ctx->components[i]->devices[j]);

				rsc_count++;
			}
		}
	}
	assert(rsc_count == nb_dev);

	return rc;

	// TODO: add progress counter.

err_free:
	for (unsigned int i = 0; i < ctx->num_cmpts; i++)
	{
		if (ctx->components[i]->num_devices > 0)
		{
			sctk_free(ctx->components[i]->devices);
		}
		ctx->components[i]->num_devices = 0;
	}

	return rc;
}

int _lcp_context_structures_init(lcp_context_h ctx)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	ctx->num_managers = 0;

	/* Preallocate a table of manager handles. */
	// NOTE: Upon manager creation, see lcp_manager_create, a handle will be
	//      atomically retrieved by incrementing the num_managers.
	ctx->mngrt = sctk_malloc(ctx->config.max_num_managers
		* sizeof(lcp_manager_h));
	if (ctx->mngrt == NULL)
	{
		mpc_common_debug_error("LCP CTX: could not allocated "
			                   "manager table.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(ctx->mngrt, 0, ctx->config.max_num_managers * sizeof(lcp_manager_h));

	/* Init hash table of tasks */
	ctx->tasks = sctk_malloc(ctx->num_tasks * sizeof(lcp_task_h));
	if (ctx->tasks == NULL)
	{
		mpc_common_debug_error("LCP CTX: Could not allocate tasks table.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	// NOTE: to release tasks, lcp_context_fini relies on the fact that all entries
	//      are memset to NULL.
	memset(ctx->tasks, 0, ctx->num_tasks * sizeof(lcp_task_h));

	return rc;

err:
	if (ctx->mngrt)
	{
		sctk_free(ctx->mngrt);
	}

	return rc;
}

int lcp_context_create(lcp_context_h *ctx_p, lcp_context_param_t *param)
{
	int           rc  = MPC_LOWCOMM_SUCCESS;
	lcp_context_h ctx = { 0 };

	// FIXME: should context creation be thread-safe ?
	if (static_ctx != NULL)
	{
		*ctx_p = static_ctx;
		return MPC_LOWCOMM_SUCCESS;
	}

	/* Parameter check. */
	// FIXME: number of tasks is currently number of MPI ranks. This is
	//       because the number of local tasks with
	//       mpc_common_get_local_task_count() is not set yet. There might
	//       be another way.
	if ((param->field_mask & LCP_CONTEXT_NUM_TASKS)
	    && (param->num_tasks <= 0))
	{
		mpc_common_debug_error("LCP CTX: wrong number of tasks.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	if (!(param->field_mask & LCP_CONTEXT_PROCESS_UID))
	{
		mpc_common_debug_error("LCP CTX: process uid not set.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	// FIXME: test if param is valid
	if (!(param->field_mask & LCP_CONTEXT_DATATYPE_OPS))
	{
		mpc_common_debug_error("LCP CTX: datatype operations not set.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	/* Allocate context */
	ctx = sctk_malloc(sizeof(struct lcp_context));
	if (ctx == NULL)
	{
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(ctx, 0, sizeof(struct lcp_context));

	ctx->dt_ops      = param->dt_ops;
	ctx->process_uid = param->process_uid;
	ctx->num_tasks   = param->num_tasks;

	if (param->field_mask & LCP_CONTEXT_REQUEST_SIZE)
	{
		ctx->config.request.size = param->request_size;
	}
	if (param->field_mask & LCP_CONTEXT_REQUEST_SIZE)
	{
		ctx->config.request.init = param->request_init;
	}

	mpc_common_debug("Loading context config");
	rc = _lcp_context_load_ctx_config(ctx, param);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto out_free_ctx;
	}

	mpc_common_debug("Loading & filtering devices");
	rc = _lcp_context_devices_load_and_filter(ctx);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto out_free_ctx;
	}

	rc = _lcp_context_structures_init(ctx);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto out_free_ctx;
	}

	_lcp_context_log_resource_config_summary(ctx);

	*ctx_p = static_ctx = ctx;

	mpc_common_debug("Context creation success");
	return MPC_LOWCOMM_SUCCESS;

out_free_ctx:
	sctk_free(ctx);
err:
	return rc;
}

int lcp_context_fini(lcp_context_h ctx)
{
	int i;

	// Free the resources
	for (i = 0; i < ctx->num_resources; i++)
	{
		sctk_free(ctx->resources[i].name);
	}
	sctk_free(ctx->resources);

	// Free the components
	lcr_free_components(ctx->components, ctx->num_cmpts);
	sctk_free(ctx->progress_counter);

	/* Free task table */
	for (i = 0; i < ctx->num_tasks; i++)
	{
		if (ctx->tasks[i] != NULL)
		{
			lcp_task_fini(ctx->tasks[i]);
		}
	}
	sctk_free(ctx->tasks);

	// Don't try to deallocate the static context
	if (ctx != static_ctx)
	{
		sctk_free(ctx);
	}

	return MPC_LOWCOMM_SUCCESS;
}

/* Returns the manager identifier. */
int lcp_context_register(lcp_context_h ctx, lcp_manager_h mngr)
{
	int i = 1;

	LCP_CONTEXT_LOCK(ctx);

	/* Get first available slot in manager table. */
	while (ctx->mngrt[i] != NULL)
	{
		i++;
	}

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
