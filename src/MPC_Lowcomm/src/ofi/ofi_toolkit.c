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
		mpc_common_debug("found providers:");
		while(list)
		{
			mpc_common_debug(" - '%s' (d=%s)", list->fabric_attr->prov_name, list->domain_attr->name);
			list = list->next;
		}
	}
#endif
}

enum _mpc_lowcomm_ofi_mode mpc_lowcomm_ofi_decode_mode(char *value)
{
	if(!strcasecmp(value, "connected"))
	{
		return MPC_LOWCOMM_OFI_CONNECTED;
	}
	else if(!strcasecmp(value, "connectionless"))
	{
		return MPC_LOWCOMM_OFI_CONNECTIONLESS;
	}
	else
	{
		bad_parameter("OFI mode value '%s' is not supported. Correct values are:\n\t- 'connected': use connected protocols (default)\n\t- 'connectionless': use connectionless protocols", value);
	}
}

enum _mpc_lowcomm_ofi_progress mpc_lowcomm_ofi_decode_progress(char *value)
{
	if(!strcasecmp(value, "auto"))
	{
		return MPC_LOWCOMM_OFI_PROGRESS_AUTO;
	}
	else if(!strcasecmp(value, "inline"))
	{
		return MPC_LOWCOMM_OFI_PROGRESS_MANUAL;
	}
	else
	{
		bad_parameter("OFI progress value '%s' is not supported. Correct values are:\n\t- 'auto': use a dedicated progress thread (default)\n\t- 'inline': proceed to progress inside OFI calls", value);
	}
}

enum _mpc_lowcomm_ofi_ep_type mpc_lowcomm_ofi_decode_eptype(char *value)
{
	if(!strcasecmp(value, "datagram"))
	{
		return MPC_LOWCOMM_OFI_EP_MSG;
	}
	else if(!strcasecmp(value, "connected"))
	{
		return MPC_LOWCOMM_OFI_EP_RDM;
	}
	else if(!strcasecmp(value, "unspecified"))
	{
		return MPC_LOWCOMM_OFI_EP_UNSPEC;
	}
	else
	{
		bad_parameter("OFI endpoint type value '%s' is not supported. Correct values are:\n\t- 'connected': reliable & connected mode (default)\n\t- 'datagram': reliable & datagram mode\n\t- 'unspecified': let OFI choose the best model", value);
	}
}

enum _mpc_lowcomm_ofi_av_type mpc_lowcomm_ofi_decode_avtype(char *value)
{
	if(!strcasecmp(value, "map"))
	{
		return MPC_LOWCOMM_OFI_AV_MAP;
	}
	else if(!strcasecmp(value, "table"))
	{
		return MPC_LOWCOMM_OFI_AV_TABLE;
	}
	else if(!strcasecmp(value, "unspecified"))
	{
		return MPC_LOWCOMM_OFI_AV_UNSPEC;
	}
	else
	{
		bad_parameter("OFI address vector type value '%s' is not supported. Correct values are:\n\t- 'map': use a hash-table\n\t- 'table': use a table\n\t- 'unspecified': let OFI choose the best model (default)", value);
	}
}

enum _mpc_lowcomm_ofi_rm_type mpc_lowcomm_ofi_decode_rmtype(char *value)
{
	if(!strcasecmp(value, "enabled"))
	{
		return MPC_LOWCOMM_OFI_RM_ENABLED;
	}
	else if(!strcasecmp(value, "disabled"))
	{
		return MPC_LOWCOMM_OFI_RM_DISABLED;
	}
	else if(!strcasecmp(value, "unspecified"))
	{
		return MPC_LOWCOMM_OFI_RM_UNSPEC;
	}
	else
	{
		bad_parameter("OFI ressource management type value '%s' is not supported. Correct values are:\n\t- 'enabled': request ressource management in the OFI driver\n\t- 'disabled': disable ressource management in the driver\n\t- 'unspecified': let OFI choose the best model (default)", value);
	}
}


/**
 * @brief Set the provider hints from information read into user config.
 * 
 * @param hint the hint structure to set
 * @param config the OFI-specific configuration.
 */
void mpc_lowcomm_ofi_setup_hints_from_config(struct fi_info* hint, struct _mpc_lowcomm_config_struct_net_driver_ofi config)
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
 * @return _mpc_lowcomm_endpoint_t* the created MPC endpoint
 */
_mpc_lowcomm_endpoint_t* mpc_lowcomm_ofi_add_route(mpc_lowcomm_peer_uid_t dest, void* ctx, sctk_rail_info_t* rail, _mpc_lowcomm_endpoint_type_t origin, _mpc_lowcomm_endpoint_state_t state)
{
	assert(rail);
	_mpc_lowcomm_endpoint_t * route = sctk_malloc(sizeof(_mpc_lowcomm_endpoint_t));
	assert(route);

	_mpc_lowcomm_endpoint_init(route, dest, rail, origin);
	_mpc_lowcomm_endpoint_set_state(route, state);

	route->data.ofi.ctx = ctx;

	if(origin == _MPC_LOWCOMM_ENDPOINT_STATIC)
	{
		sctk_rail_add_static_route (  rail, route );
	}
	else
	{
		sctk_rail_add_dynamic_route(  rail, route );
	}
	return route;
}
