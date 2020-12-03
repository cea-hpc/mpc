/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
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
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #                                                                      # */
/* ######################################################################## */

#include "ofi_toolkit.h"
#include "ofi_headers.h"
#include <sctk_rail.h>
#include <sctk_route.h>
#include <sctk_alloc.h>

#include <mpc_common_rank.h>

/**
 * @brief Find a provider fulfilling the requirements.
 * 
 * This function won't fail if no provider is compatible.
 * It is up to the parent function to deal with it. This gives more
 * flexibily to specific layer to query again with less constraints.
 * 
 * @param orail 
 * @param hint 
 */
void mpc_lowcomm_ofi_init_provider(mpc_lowcomm_ofi_rail_info_t* orail, struct fi_info* hint)
{
	assert(orail);
	struct fi_info* list = NULL;
	MPC_LOWCOMM_OFI_CHECK_RC(fi_getinfo(FI_VERSION(1,0), NULL, NULL, 0, hint, &list));
	orail->provider = list;
	
	if(list)
	{
		MPC_LOWCOMM_OFI_CHECK_RC(fi_fabric(list->fabric_attr, &orail->fabric, NULL));
		MPC_LOWCOMM_OFI_CHECK_RC(fi_domain(orail->fabric, list, &orail->domain, NULL));

	}

#ifndef NDEBUG
	if(!mpc_common_get_process_rank())
	{
		sctk_debug("found providers:");
		while(list)
		{
			sctk_debug(" - '%s' (d=%s)", list->fabric_attr->prov_name, list->domain_attr->name);
			list = list->next;
		}
	}
#endif
}

/**
 * @brief Set the provider hints from information read into user config.
 * 
 * @param hint the hint structure to set
 * @param config the OFI-specific configuration.
 */
void mpc_lowcomm_ofi_setup_hints_from_config(struct fi_info* hint, struct sctk_runtime_config_struct_net_driver_ofi config)
{

	switch(config.progress)
	{
		case MPC_LOWCOMM_OFI_PROGRESS_AUTO: 
			hint->domain_attr->data_progress = FI_PROGRESS_AUTO;
			hint->domain_attr->control_progress = FI_PROGRESS_AUTO;
			break;
		case MPC_LOWCOMM_OFI_PROGRESS_MANUAL: 
			hint->domain_attr->data_progress = FI_PROGRESS_MANUAL;
			hint->domain_attr->control_progress = FI_PROGRESS_MANUAL;
			break; 
		default:
			not_reachable();
	}

	switch(config.ep_type)
	{
		case MPC_LOWCOMM_OFI_EP_MSG: hint->ep_attr->type = FI_EP_MSG; break;
		case MPC_LOWCOMM_OFI_EP_RDM: hint->ep_attr->type = FI_EP_RDM; break;
		case MPC_LOWCOMM_OFI_EP_UNSPEC: break;
		default:
			not_reachable();
	}

	switch(config.av_type)
	{
		case MPC_LOWCOMM_OFI_AV_MAP: hint->domain_attr->av_type = FI_AV_MAP; break;
		case MPC_LOWCOMM_OFI_AV_TABLE: hint->domain_attr->av_type = FI_AV_TABLE; break;
		case MPC_LOWCOMM_OFI_AV_UNSPEC: break;
		default:
			not_reachable();
	}

	switch(config.rm_type)
	{
		case MPC_LOWCOMM_OFI_RM_ENABLED: hint->domain_attr->resource_mgmt = FI_RM_ENABLED; break;
		case MPC_LOWCOMM_OFI_RM_DISABLED: hint->domain_attr->resource_mgmt = FI_RM_DISABLED; break;
		case MPC_LOWCOMM_OFI_RM_UNSPEC: break;
		default:
			not_reachable();
	}

	if(config.provider)
	{
		hint->fabric_attr->prov_name = strdup(config.provider);
	}
}

/**
 * @brief Add a new libfabric route to the low_level_comm.
 * 
 * @param dest the process this route is directed to
 * @param ctx the opaque context to store into the endpoint
 * @param rail the current rail, owning the route
 * @param origin the route type (DYNAMIC / STATIC)
 * @param state the route state (CONNECTED / UNCONNECTED ...)
 * @return sctk_endpoint_t* the created MPC endpoint
 */
sctk_endpoint_t* mpc_lowcomm_ofi_add_route(int dest, void* ctx, sctk_rail_info_t* rail, sctk_route_origin_t origin, sctk_endpoint_state_t state)
{
	assert(rail);
	sctk_endpoint_t * route = sctk_malloc(sizeof(sctk_endpoint_t));
	assert(route);

	sctk_endpoint_init(route, dest, rail, origin);
	sctk_endpoint_set_state(route, state);

	route->data.ofi.ctx = ctx;

	if(origin == ROUTE_ORIGIN_STATIC)
	{
		sctk_rail_add_static_route (  rail, route );
	}
	else
	{
		sctk_rail_add_dynamic_route(  rail, route );
	}
	return route;
}
