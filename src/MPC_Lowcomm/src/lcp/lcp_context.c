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
#include <sctk_alloc.h>
#include <mpc_common_flags.h>

#include "lowcomm_config.h"
#include "mpc_common_datastructure.h"
#include "rail.h"

#include "lcp.h"
#include "lcp_mem.h"
#include "lcp_common.h"
#include "lcp_ep.h"
#include "lcp_task.h"
#include "lcr/lcr_component.h"
#include "lcr/lcr_def.h"

lcp_am_handler_t lcp_am_handlers[LCP_AM_ID_LAST] = {{NULL, 0}};
//FIXME: add context param to decide whether to allocate context statically or
//       dynamically.
static int lcp_context_is_initialized = 0;
static lcp_context_h static_ctx = NULL;


static inline int lcp_context_set_am_handler(lcp_context_h ctx, 
                                             sctk_rail_info_t *iface)
{
	int rc = 0;
        int am_id = 0;

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
	int rc = 0;
        int i = 0;
        lcp_rsc_desc_t *rsc = NULL;

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
        int rc = 0;

	/* Init endpoint and fragmentation structures */
	mpc_common_spinlock_init(&(ctx->ctx_lock), 0);

        /* Init pending matching message request table */
	ctx->match_ht = lcp_pending_init();
	if (ctx->match_ht == NULL) {
		mpc_common_debug_error("LCP: Could not allocate matching pending tables.");
		rc = LCP_ERROR;
		goto err;
	}

        /* Init hash table of tasks */
	ctx->tasks = sctk_malloc(ctx->num_tasks * sizeof(lcp_task_h));
	if (ctx->tasks == NULL) {
		mpc_common_debug_error("LCP: Could not allocate tasks table.");
		rc = LCP_ERROR;
		goto out_free_pending_tables;
	}

        mpc_common_hashtable_init(&ctx->ep_htable, 1024);

        lcp_pinning_mmu_init();

	rc = LCP_SUCCESS;

        return rc;

out_free_pending_tables:
        sctk_free(ctx->match_ht); 
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
        int rc = 0;
        lcr_device_t *devices = NULL;
        unsigned num_devices = 0;

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
        assume(iface_config != NULL);
        lcr_driver_config_t *driver_config =
                _mpc_lowcomm_conf_driver_unfolded_get(iface_config->config);
        assume(driver_config != NULL);


        /* Init resource */
        resource_p->iface_config  = iface_config;
        resource_p->driver_config = driver_config;
        resource_p->iface         = NULL;
        resource_p->priority      = iface_config->priority;
        resource_p->component     = component;
        strcpy(resource_p->name, device->name);
        resource_p->used = 0;
}

lcp_task_h lcp_context_task_get(lcp_context_h ctx, int tid)
{
        return ctx->tasks[tid];
}

/**
 * @brief This takes a rail configuration and resolves the corresponding LCP driver name to return its internal configurations
 *
 * @param driver_config a driver config to resolve
 * @return lcr_component_t* The component template matching the driver
 */
static inline lcr_component_t * __resolve_config_to_driver(struct _mpc_lowcomm_config_struct_net_driver_config * driver_config)
{
        assume((0 <= driver_config->driver.type) && (driver_config->driver.type < MPC_LOWCOMM_CONFIG_DRIVER_COUNT));

        const char * const driver_name = _mpc_lowcomm_config_struct_net_driver_type_name[driver_config->driver.type];
        lcr_component_t * reference_component = lcr_query_component_by_name(driver_name);

        if(!reference_component)
        {
                bad_parameter("Cannot locate a driver named %s", driver_name);
        }

        return reference_component;
}

/**
 * @brief This initializes a LCR component
 *
 * @param rail rail matching the component
 * @param component target component to be initialized (in the components array)
 * @return int 0 on success
 */
static inline int __init_lcr_component(struct _mpc_lowcomm_config_struct_net_rail *rail, struct lcr_component *component)
{
        /* Get rail and config */
        assume(rail);
        struct _mpc_lowcomm_config_struct_net_driver_config *driver_config = _mpc_lowcomm_conf_driver_unfolded_get(rail->config);
        assume(driver_config);

        if(!driver_config)
        {
		bad_parameter("Rail %s references an unknown configuration %s\n", rail->name, rail->config);
        }

        /* Retrieve reference configuration from driver */
        memcpy(component, __resolve_config_to_driver(driver_config), sizeof(struct lcr_component));

        component->driver_config = driver_config;

        /* Make sure to set the corresponding rail name in the component */
        (void)snprintf(component->rail_name, LCR_COMPONENT_NAME_MAX, "%s", rail->name);

        /* Load devices from component */
        if( lcp_context_query_component_devices(component,
                                                &component->devices,
                                                &component->num_devices))
        {
                mpc_common_debug_error("Failed to query devices for %s", rail->name);
                return -1;
        }

        /* Filter only selected devices (as per rail configuration)
           here we do it only if it is not "any" */
        if(strcmp(rail->device, "any") != 0)
        {
                int filtered_count = 0;
                lcr_device_t * filtered_devices = malloc(sizeof(lcr_device_t) * component->num_devices);
                assume(filtered_devices != NULL);

                unsigned int i = 0;
                for(i = 0; i < component->num_devices; i++)
                {
                        TODO(Device detection logic is too simple);
                        if(strstr(rail->device, component->devices[i].name))
                        {
                                memcpy(&filtered_devices[filtered_count], &component->devices[i], sizeof(lcr_device_t));
                                filtered_count++;
                        }
                }

                if(filtered_count == 0)
                {
                        bad_parameter("No device was selected for rail %s with filter %s", rail->name, rail->device);
                }

                free(component->devices);
                component->devices = filtered_devices;
                component->num_devices = filtered_count;
        }

        return 0;
}


#define RAIL_BUFFER_SIZE 32

/**
 * @brief This function is a logic used to filter rails in function of the state of MPC **before** starting them
 * 
 * @param rails array of rail pointers to be checked
 * @param rail_count a pointer to the number of rails (to be changed if needed)
 * @return int 0 on success
 */
static inline int __detect_most_optimal_network_config(struct _mpc_lowcomm_config_struct_net_rail * rails[RAIL_BUFFER_SIZE],  unsigned int * rail_count)
{
TODO(understand why it crashes running without networking);
#if 0
        /* The case of a single rank with a single process  == no networking */
        if( (mpc_common_get_process_count() == 1) && (mpc_common_get_task_count() == 1) )
        {
                /* No need for networking */
                *rail_count = 0;
                return 0;
        }
#endif

        unsigned int i = 0;

        /* The case of a single process and multiple tasks == TBSM only */
        if( (mpc_common_get_process_count() == 1) && (mpc_common_get_task_count() > 1) )
        {
                for(i=0; i < *rail_count; i++)
                {
                        struct _mpc_lowcomm_config_struct_net_driver_config *driver_config = _mpc_lowcomm_conf_driver_unfolded_get(rails[i]->config);
                        lcr_component_t * cmpt = __resolve_config_to_driver(driver_config);

                        if( strcmp(cmpt->name, "tbsm") != 0)
                        {
                                rails[i] = NULL;
                        }
                }
        }


        /* Common END, we only keep the non-removed rails */

        /* Now count non-null and reshape */
        int final_count = 0;
        struct _mpc_lowcomm_config_struct_net_rail *returned_rails[RAIL_BUFFER_SIZE] = {0};

        for(i=0; i < *rail_count; i++)
        {
                if(rails[i])
                {
                        returned_rails[i] = rails[i];
                        final_count++;
                }
        }

        memcpy(rails, returned_rails, RAIL_BUFFER_SIZE * sizeof(struct _mpc_lowcomm_config_struct_net_rail*));
        *rail_count = final_count;

        return 0;
}


/**
 * @brief This function is used to sort the rail array by priority before unfolding resources
 * 
 * @param pa pointer to pointer of rail (qsort)
 * @param pb pointer to pointer of rail (qsort)
 * @return int order function result
 */
int __sort_rails_by_priority(const void * pa, const void *pb)
{
        struct _mpc_lowcomm_config_struct_net_rail **a = (struct _mpc_lowcomm_config_struct_net_rail**)pa;
        struct _mpc_lowcomm_config_struct_net_rail **b = (struct _mpc_lowcomm_config_struct_net_rail**)pb;

        return (*a)->priority < (*b)->priority;
}

/**
 * @brief This is the main function for initializing LCP & Rails
 *
 * @param ctx context to intialize
 * @return int 0 on success
 */
static inline int __init_rails(lcp_context_h ctx)
{
	char *option_name = _mpc_lowcomm_config_net_get()->cli_default_network;

	/* If we have a network name from the command line (through sctk_launch.c)  */
	if(mpc_common_get_flags()->network_driver_name != NULL)
	{
		option_name = mpc_common_get_flags()->network_driver_name;
	}
	/* Here we retrieve the network configuration from the network list
	 * according to its name */

	mpc_common_nodebug("Run with driver %s", option_name);

	mpc_conf_config_type_t *cli_option = _mpc_lowcomm_conf_cli_get(option_name);


	if(cli_option == NULL)
	{
		/* We did not find this name in the network configurations */
		bad_parameter("No configuration found for the network '%s'. Please check you '-net=' argument"
		              " and your configuration file", option_name);
	}

        /* Check that all selected rails do exist */
	unsigned int k = 0;
        unsigned int num_rails = mpc_conf_config_type_count(cli_option);

        struct _mpc_lowcomm_config_struct_net_rail * sorted_rails[RAIL_BUFFER_SIZE] = {0};
        assume(num_rails < RAIL_BUFFER_SIZE);


	for(k = 0; k < num_rails; ++k)
	{
		mpc_conf_config_type_elem_t *erail = mpc_conf_config_type_nth(cli_option, k);

		/* Get the rail */
		struct _mpc_lowcomm_config_struct_net_rail *rail = _mpc_lowcomm_conf_rail_unfolded_get(mpc_conf_type_elem_get_as_string(erail) );

		if(!rail)
		{
			bad_parameter("Could not find a rail config named %s", rail);
		}
                /* Save pointer to rail for sorting by priority afterwards */
                sorted_rails[k] = rail;
	}

        /* Now filter reasonable configurations as per current process layout */
        __detect_most_optimal_network_config(sorted_rails, &num_rails);


        /* Now we sort the rails by prioriy to allow their resource unfolding in order */
        qsort(sorted_rails, num_rails, sizeof(struct _mpc_lowcomm_config_struct_net_rail *), __sort_rails_by_priority);


        /* Now generate the component list extracting driver configurations */
        ctx->cmpts = malloc(num_rails * sizeof(struct lcr_component));
        assume(ctx->cmpts);
        ctx->num_cmpts = num_rails;

        for(k = 0; k < num_rails; ++k)
	{
                if( __init_lcr_component( sorted_rails[k], &ctx->cmpts[k]) )
                {
                        mpc_common_debug_fatal("Failed to start LCR component for rail %s", sorted_rails[k]->name);
                }
        }

        /* We now proceed to initialize all resources
           to do so we first need to count all devices
           then we allocate and initialize each device */
        int resource_count = 0;
        unsigned int l = 0;
        for(k = 0; k < ctx->num_cmpts; ++k)
	{
                struct lcr_component * tmp = &ctx->cmpts[k];
                for(l=0; l < tmp->num_devices; l++)
                {
                        resource_count++;
                }
        }

        ctx->num_resources = resource_count;
        ctx->resources = malloc(sizeof(lcp_rsc_desc_t) * resource_count);
        assume(ctx->resources);
        ctx->progress_counter = malloc(sizeof(int) * resource_count);
        assume(ctx->progress_counter);

        /* Walk again to initialize each ressource */
        resource_count = 0;

        for(k = 0; k < ctx->num_cmpts; ++k)
	{
                struct lcr_component * tmp = &ctx->cmpts[k];
                for(l=0; l < tmp->num_devices; l++)
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

        for(k = 0; k < ctx->num_resources; ++k)
	{
                total_priorities += ctx->progress_counter[k]; 
        }

        unsigned int max_prio = 0;

        /* Normalize priorities */
        for(k = 0; k < ctx->num_resources; ++k)
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


        return 0;
}

/**
 * @brief Check if the component is being used in the given context
 *
 * @param ctx the context to check
 * @param name the name of the component
 * @return int 1 yes 0 no
 */
static inline int __one_component_is(lcp_context_h ctx, const char* name)
{
        unsigned int i = 0;
        for(i = 0 ; i < ctx->num_cmpts; i++)
        {
                if(!strcmp(ctx->cmpts[i].name, name))
                {
                        return -1;
                }
        }

        return 0;
}

/**
 * @brief Helper function to count total numbers of devices on a given context
 *
 * @param ctx the context to count from
 * @return number of devices
 */
__UNUSED__ static inline unsigned int __get_component_device_count(lcp_context_h ctx)
{
        unsigned int ret = 0;
        unsigned int i = 0;
        for(i = 0 ; i < ctx->num_cmpts; i++)
        {
                ret += ctx->cmpts[i].num_devices;
        }

        return ret;
}

/**
 * @brief This function generates the listing of the network configuration when passing -v to mpcrun
 * 
 * @param ctx the context to generate the description of
 * @return int 0 on success
 */
#define NETWORK_DESC_BUFFER_SIZE (4*1024llu)
static inline int __generate_configuration_summary(lcp_context_h ctx)
{
        if(mpc_common_get_flags()->sctk_network_description_string)
        {
                free( mpc_common_get_flags()->sctk_network_description_string);
        }
        mpc_common_get_flags()->sctk_network_description_string = malloc(NETWORK_DESC_BUFFER_SIZE);


        unsigned int i = 0;
        unsigned int j = 0;

        char * name = mpc_common_get_flags()->sctk_network_description_string;
        *name = '\0';

        if(!ctx->num_cmpts)
        {
                snprintf(name, NETWORK_DESC_BUFFER_SIZE, "No networking");
                return 0;
        }

        char tmp[512];

        for(i= 0 ; i < ctx->num_cmpts; i++)
        {
                lcr_component_t * cmt = &ctx->cmpts[i];

                (void)snprintf(tmp, 512, "\n - %s\n", cmt->name);
                strncat(name, tmp, NETWORK_DESC_BUFFER_SIZE - 1);

                for(j = 0 ; j < cmt->num_devices; j++)
                {
                        (void)snprintf(tmp, 512, " \t* %s\n", cmt->devices[j].name);
                        strncat(name, tmp, NETWORK_DESC_BUFFER_SIZE - 1);
                }
        }

        strncat(name, "\nInitialized ressources:\n", NETWORK_DESC_BUFFER_SIZE - 1);

        int k = 0;

        for(k=0; k < ctx->num_resources; k++)
        {
                lcp_rsc_desc_t *res = &ctx->resources[k];
                (void)snprintf(tmp, 512, " - [%d] %s %s (%s, %s)\n", res->priority, res->name, res->component->name, res->iface_config->name, res->driver_config->name);
                strncat(name, tmp, NETWORK_DESC_BUFFER_SIZE- 1);
        }

        return 0;
}


/**
 * @brief Count non composable rails (TBSM and SHM only are composable)
 * 
 * @param ctx context to count from
 * @return unsigned number of rails not counting TBSM and SHM
 */
unsigned int __count_non_composable_rails(lcp_context_h ctx)
{
        unsigned int num_composable = 0;

        int i;

        for(i =  0; i < (int)ctx->num_cmpts; i++)
        {
                struct lcr_component * comp = &ctx->cmpts[i];

                if(!strcmp(comp->name, "shm") || !strcmp(comp->name, "tbsm"))
                        continue;
        
                num_composable++;
        }

        return num_composable;
}




/**
 * @brief This is called after fully initializing drivers to validate final configuration
 * 
 * @param ctx the context to validate
 * @return int 1 on error.
 */
static inline int __check_configuration(lcp_context_h ctx)
{
        /* Does not support heterogeous multirail (tsbm and shm not counted) */
        if(__count_non_composable_rails(ctx) > 1) {
                mpc_common_debug_error("LCP: heterogeous multirail not supported");
                return -1;
        }

        /* Does not support multirail with tcp */
        if( __one_component_is(ctx, "tcp") && ctx->config.multirail_enabled)
        {
                mpc_common_debug_warning("LCP: multirail not supported with tcp."
                                         " Disabling it...");
                ctx->config.multirail_enabled = 0;
        }

        /* No more than 1 device in TCP */
        if( __one_component_is(ctx, "tcp") && !ctx->config.multirail_enabled)
        {
                lcr_component_t *tcp_cmpt = lcr_query_component_by_name("tcp");
                if(tcp_cmpt->num_devices > 1)
                {
                        mpc_common_debug_warning("LCP: cannot use multiple device with tcp.");
                        return -1;
                }
        }

        return __generate_configuration_summary(ctx);
}

static int lcp_context_check_offload(lcp_context_h ctx)
{
        int rc = LCP_SUCCESS, i;
        int has_offload = 0;
        lcp_rsc_desc_t rsc;
        lcr_rail_config_t *iface_config;

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

/**
 * @brief This is the main entry point to configure LCP
 * 
 * @param ctx_p pointer to the context
 * @param param configuration params for pack, unpack and getuid
 * @return int LCP_SUCCESS on success
 */
int lcp_context_create(lcp_context_h *ctx_p, lcp_context_param_t *param)
{
	int rc = LCP_SUCCESS;
        int i = 0;
	lcp_context_h ctx = {0};

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

        ctx->config.max_num_tasks        = 512;
        if (param->num_tasks <= 0) {
                mpc_common_debug_error("LCP: wrong number of tasks");
                rc = LCP_ERROR;
                goto out_free_ctx;
        }

        /* Get global configuration */
        struct _mpc_lowcomm_config_struct_protocol * config = _mpc_lowcomm_config_proto_get();

        ctx->config.multirail_enabled    = config->multirail_enabled;
	ctx->config.rndv_mode            = (lcp_rndv_mode_t)config->rndv_mode;
        ctx->config.offload              = config->offload;
        ctx->process_uid                 = param->process_uid;
        //FIXME: number of tasks is currently number of MPI ranks. This is
        //       because the number of local tasks with
        //       mpc_common_get_local_task_count() is not set yet. There might
        //       be another way.
        ctx->num_tasks                   = param->num_tasks;

	/* init random seed to generate unique msg_id */
	rand_seed_init();
        OPA_store_int(&ctx->msg_id, 0);

        /* init match uid when using match request */
        OPA_store_int(&ctx->muid, 0);

        /* Generate rail list and unfold components and then resources from components */
        if( __init_rails(ctx) )
        {
                rc = LCP_ERROR;
                mpc_common_debug_error("Failed to initialize rails");
                goto out_free_components;
        }

        /* Check that the resulting configuration is possible*/
        if( __check_configuration(ctx) )
        {
                rc = LCP_ERROR;
                mpc_common_debug_error("Invalid configuration");
                goto out_free_components;
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


	return LCP_SUCCESS;

TODO("Check how things are released it seems weak");
out_free_ifaces:
        for (i=0; i<ctx->num_resources; i++) {
                sctk_rail_info_t *iface = ctx->resources[i].iface;
                iface->driver_finalize(iface);
                sctk_free(ctx->resources[i].iface);
        }
out_free_resources:
        for (i=0; i<(int)ctx->num_cmpts; i++) {
                sctk_free(ctx->cmpts[i].devices);
        }
        sctk_free(ctx->resources);
out_free_components:
        lcr_free_components(&ctx->cmpts, ctx->num_cmpts, 0);
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


        lcp_pinning_mmu_release();

        /* Free task table */
        for (i = 0; i < ctx->num_tasks; i++) {
                if (ctx->tasks[i] != NULL) {
                        lcp_task_fini(ctx->tasks[i]);
                }
        }
        sctk_free(ctx->tasks);

	/* Free allocated endpoints */
	lcp_ep_ctx_t *e_ep = NULL;

	MPC_HT_ITER(&ctx->ep_htable, e_ep)
        {
                lcp_ep_delete(e_ep->ep);
		sctk_free(e_ep);
	}
        MPC_HT_ITER_END(&ctx->ep_htable);

	sctk_rail_info_t *iface = NULL;
	for (i=0; i<ctx->num_resources; i++) {
		iface = ctx->resources[i].iface; 
		if (iface->driver_finalize)
			iface->driver_finalize(iface);
		sctk_free(iface);
	}
	sctk_free(ctx->resources);

	sctk_free(ctx);

	return LCP_SUCCESS;
}
