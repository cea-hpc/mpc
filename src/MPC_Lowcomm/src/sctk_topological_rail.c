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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_topological_rail.h"


#include <mpc_common_rank.h>
#include <mpc_topology.h>
#include <multirail.h>
#include <mpc_lowcomm_monitor.h>

#include <sctk_alloc.h>

#include "sctk_rail.h"

_mpc_lowcomm_endpoint_t *sctk_topological_rail_ellect_endpoint(mpc_lowcomm_peer_uid_t remote, mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint)
{
	int vp_id = mpc_topology_get_pu();

	if(vp_id < 0)
	{
		/* If not on VP assume 0 */
		vp_id = 0;
	}

	assume(endpoint->parent_rail != NULL);

	sctk_topological_rail_info_t *topo_infos = &endpoint->parent_rail->network.topological;
	int target_subrail = topo_infos->vp_to_subrail[vp_id];

	/* If everything goes bad */
	if(target_subrail < 0)
	{
		target_subrail = 0;
	}


	sctk_rail_info_t *topological_rail = endpoint->parent_rail->subrails[target_subrail];
	assume(0 <= endpoint->subrail_id);
	sctk_rail_info_t *endpoint_rail = endpoint->rail;

	/* Do we match ? */
	if(topological_rail == endpoint_rail)
	{
		/* It means that the endpoint found by the multirail
		 * is already topological, just send through it */
		return endpoint;
	}
	else
	{
		/* Here the first endpoint is not the topological
		 * one, we have to look in the topological rail
		 * to see if this endpoint exists */
		_mpc_lowcomm_endpoint_t *topological_endpoint = sctk_rail_get_any_route_to_process(topological_rail, remote);

		/* Did we find one ? Is it connected yet ? */
		if(_mpc_lowcomm_endpoint_get_state(topological_endpoint) == _MPC_LOWCOMM_ENDPOINT_CONNECTED)
		{
			/* YES Send through it */
			return topological_endpoint;
		}
		else
		{
			if(!_mpc_comm_ptp_message_is_for_control(SCTK_MSG_SPECIFIC_CLASS(msg) ) )
			{
				/* Intiate an on-demand connection to the remote on the topological rail (to keep connection balanced) */
				if(topological_rail->on_demand)
				{
					/* Note that here we push the pending connection in
					 * a list as we cannot create connection while holding
					 * the route table in read (in the middle of a send )
					 * this will be done just before leaving the multirail-send function */
					sctk_pending_on_demand_push(topological_rail, remote);
				}
			}

			/* Fallback to the original endpoint (we pay the NUMA not to delay the message) */
			return endpoint;
		}
	}

	return NULL;
}

static void _mpc_lowcomm_topological_send_message(mpc_lowcomm_ptp_message_t *msg, _mpc_lowcomm_endpoint_t *endpoint)
{
	_mpc_lowcomm_endpoint_t *topo_endpoint = sctk_topological_rail_ellect_endpoint(SCTK_MSG_DEST_PROCESS_UID(msg), msg, endpoint);

	assume(topo_endpoint != NULL);

	sctk_rail_info_t *endpoint_rail = endpoint->rail;

	assume(endpoint_rail != NULL);

	endpoint_rail->send_message_endpoint(msg, topo_endpoint);
}

void topological_on_demand_connection_handler(sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest_process)
{
	int vp_id = mpc_topology_get_pu();

	if(vp_id < 0)
	{
		/* If not on VP assume 0 */
		vp_id = 0;
	}
	sctk_topological_rail_info_t *topo_infos = &rail->network.topological;
	int target_subrail = topo_infos->vp_to_subrail[vp_id];

	/* If everything goes bad */
	if(target_subrail < 0)
	{
		target_subrail = 0;
	}

	sctk_rail_info_t *topological_subrail = rail->subrails[target_subrail];

	if(topological_subrail->connect_on_demand)
	{
		topological_subrail->connect_on_demand(topological_subrail, dest_process);
	}
}


/********************************************************************/
/* TOPOLOGICAL Rail Init                                            */
/********************************************************************/

void sctk_network_init_topological_rail_info(sctk_rail_info_t *rail)
{
	sctk_topological_rail_info_t *infos = &rail->network.topological;

	/* Allocate an array of CPU size */
	infos->max_vp = mpc_topology_get_pu_count();

	infos->vp_to_subrail = sctk_malloc(infos->max_vp * sizeof(int) );
	assume(infos->vp_to_subrail != NULL);

	/* Now fill it according to different cases */
	int i;

	/* First put -1 everywhere */
	for(i = 0; i < infos->max_vp; i++)
	{
		infos->vp_to_subrail[i] = -1;
	}

	/* First case there is a single rail to choose */
	if(rail->subrail_count == 1)
	{
		for(i = 0; i < infos->max_vp; i++)
		{
			/* Always choose first rail */
			infos->vp_to_subrail[i] = 0;
		}

		/* Done */
		return;
	}

	char *device_regexp            = NULL;
	int   device_reg_exp_allocated = 0;

	/* Prepare to switch between the two last cases */
	if(sctk_rail_device_is_regexp(rail) )
	{
		/* Second case the topology is defined with a regexp */

		device_regexp = sctk_rail_get_device_name(rail);
		/* Increment by 1 to skip the '!' */
		device_regexp++;
	}
	else
	{
		/* Third case, the topology is defined as a list of subrails
		 * the tips here is to build a regexp from it this list
		 * to use the same code as if it was a regexp */

		/* Allocate the device name list */
		char **device_name_list = sctk_malloc(sizeof(char *) * rail->subrail_count);
		int    j;

		/* Retrieve names */
		for(i = 0; i < rail->subrail_count; i++)
		{
			if(sctk_rail_device_is_regexp(rail->subrails[i]) )
			{
				mpc_common_debug_fatal("Device names with regular expressions are not allowed in subrails");
			}

			device_name_list[i] = sctk_rail_get_device_name(rail->subrails[i]);
		}

		/* Bubble sort names (as a shorter regexp with the same name could match */

		int total_name_len = 0;

		for(i = 0; i < rail->subrail_count; i++)
		{
			char *name_i = device_name_list[i];

			int len_i = 0;
			if(name_i)
			{
				len_i = strlen(name_i);
			}

			/* In the mean time do total name len computation */
			total_name_len += len_i;

			for(j = i + 1; j < rail->subrail_count; j++)
			{
				char *name_j = device_name_list[j];

				int len_j = 0;
				if(name_j)
				{
					len_j = strlen(name_j);
				}

				if(len_i < len_j)
				{
					char *tmp = name_i;
					device_name_list[i] = name_j;
					device_name_list[j] = tmp;
				}
			}
		}

		/* Allocate space for the regexp (add 3*count for the \\|) */
		device_reg_exp_allocated = 1;
		int device_regexp_length = total_name_len + 3 * rail->subrail_count + 100 /* Some margin never killed */;
		device_regexp = sctk_malloc(device_regexp_length);
		assume(device_regexp != NULL);

		device_regexp[0] = '\0';

		/* Build the regexp */
		for(i = 0; i < rail->subrail_count; i++)
		{
			if(!device_name_list[i])
			{
				continue;
			}

			char local[500];
			snprintf(local, 500, "%s", device_name_list[i]);

			strncat(device_regexp, local, device_regexp_length);

			if(i != (rail->subrail_count - 1) )
			{
				strncat(device_regexp, "\\|", device_regexp_length);
			}
		}
	}

	/* Is the topology flat ? */
	if(mpc_topology_device_matrix_is_equidistant(device_regexp) )
	{
		/* In this case we split the devices among the PUs as the topology
		 * is flat therefore no need to think of distances just split
		 * devices envenly among the CPUS */

		for(i = 0; i < infos->max_vp; i++)
		{
			/* Comput ehte device bucket */
			infos->vp_to_subrail[i] = (i + mpc_common_get_local_process_rank())%rail->subrail_count;
			mpc_common_debug_info("[FLAT] TOPOLOGICAL Rail VP %d maps to device %d", i, infos->vp_to_subrail[i]);

			/* Should NEVER happen due to integer computation */
			if(rail->subrail_count <= infos->vp_to_subrail[i])
			{
				infos->vp_to_subrail[i] = rail->subrail_count - 1;
			}
		}

		/* Done */
	}
	else
	{
		/* Here we have to think of distance */
		for(i = 0; i < infos->max_vp; i++)
		{
			/* Always choose first rail */
			mpc_topology_device_t *dev = mpc_topology_device_get_closest_from_pu(i, device_regexp);
			/* Map device to subrail */
			infos->vp_to_subrail[i] = sctk_rail_get_subrail_id_with_device(rail, dev);
		}

		/* Done */
	}

	if(device_reg_exp_allocated)
	{
		sctk_free(device_regexp);
	}
}

void sctk_topological_rail_pin_region(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list, void *addr, size_t size)
{
	int i;

	/* Build a pinning list involving all subrails */
	for(i = 0; i < rail->subrail_count; i++)
	{
		struct sctk_rail_info_s *subrail = rail->subrails[i];

		if(!subrail->rail_pin_region || !subrail->rail_unpin_region)
		{
			mpc_common_debug_fatal("To be RDMA capable a topological rail must\n"
			                       "only contain RDMA capable networks (error %s)", subrail->network_name);
		}

		/* Here we fill the individual pin infos for subrails */
		subrail->rail_pin_region(subrail, list + i, addr, size);
	}
}

void sctk_topological_rail_unpin_region(struct sctk_rail_info_s *rail, struct sctk_rail_pin_ctx_list *list)
{
	int i;

	for(i = 0; i < rail->subrail_count; i++)
	{
		sctk_rail_info_t *subrail = rail->subrails[i];

		if(!subrail->rail_pin_region || !subrail->rail_unpin_region)
		{
			mpc_common_debug_fatal("To be RDMA capable a topological rail must\n"
			                       "only contain RDMA capable networks (error %s)", subrail->network_name);
		}

		subrail->rail_unpin_region(subrail, list + i);
	}
}

void sctk_topological_rail_rdma_write(
	sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg, void *src_addr,
	struct sctk_rail_pin_ctx_list *local_key, void *dest_addr,
	struct sctk_rail_pin_ctx_list *remote_key, size_t size)
{
	_mpc_lowcomm_endpoint_t *topo_endpoint =
		sctk_rail_get_any_route_to_process(rail, SCTK_MSG_DEST_PROCESS(msg) );

	if(!topo_endpoint)
	{
		topological_on_demand_connection_handler(rail, SCTK_MSG_DEST_PROCESS(msg) );
	}
	else
	{
		topo_endpoint = sctk_topological_rail_ellect_endpoint(
			SCTK_MSG_DEST_PROCESS(msg), msg, topo_endpoint);
	}

	/* By default use rail 0 */
	sctk_rail_info_t *endpoint_rail = rail->subrails[0];

	/* If we found a topo endpoint, override the 0 */
	if(topo_endpoint)
	{
		endpoint_rail = topo_endpoint->rail;
	}

	assume(endpoint_rail != NULL);
	assume(endpoint_rail->rdma_write != NULL);

	int subrail_id = endpoint_rail->subrail_id;
	assume(0 <= subrail_id);
	assume(subrail_id < rail->subrail_count);

	endpoint_rail->rdma_write(endpoint_rail, msg, src_addr,
	                          local_key + subrail_id, dest_addr,
	                          remote_key + subrail_id, size);
}

void sctk_topological_rail_rdma_read(sctk_rail_info_t *rail, mpc_lowcomm_ptp_message_t *msg,
                                     void *src_addr, struct  sctk_rail_pin_ctx_list *remote_key,
                                     void *dest_addr, struct  sctk_rail_pin_ctx_list *local_key,
                                     size_t size)
{
	_mpc_lowcomm_endpoint_t *topo_endpoint =
		sctk_rail_get_any_route_to_process(rail, SCTK_MSG_DEST_PROCESS(msg) );

	if(!topo_endpoint)
	{
		topological_on_demand_connection_handler(rail, SCTK_MSG_DEST_PROCESS(msg) );
	}
	else
	{
		topo_endpoint = sctk_topological_rail_ellect_endpoint(
			SCTK_MSG_DEST_PROCESS(msg), msg, topo_endpoint);
	}

	/* By default use rail 0 */
	sctk_rail_info_t *endpoint_rail = rail->subrails[0];

	/* If we found a topo endpoint, override the 0 */
	if(topo_endpoint)
	{
		endpoint_rail = topo_endpoint->rail;
	}

	assume(endpoint_rail != NULL);
	assume(endpoint_rail->rdma_read != NULL);

	int subrail_id = endpoint_rail->subrail_id;
	assume(0 <= subrail_id);
	assume(subrail_id < rail->subrail_count);

	endpoint_rail->rdma_read(endpoint_rail, msg, src_addr,
	                         remote_key + subrail_id, dest_addr,
	                         local_key + subrail_id, size);
}

void sctk_topological_rail_rdma_fetch_and_op(sctk_rail_info_t *rail,
                                             mpc_lowcomm_ptp_message_t *msg,
                                             void *fetch_addr,
                                             struct  sctk_rail_pin_ctx_list *local_key,
                                             void *remote_addr,
                                             struct  sctk_rail_pin_ctx_list *remote_key,
                                             void *add,
                                             RDMA_op op,
                                             RDMA_type type)
{
	_mpc_lowcomm_endpoint_t *topo_endpoint =
		sctk_rail_get_any_route_to_process(rail, SCTK_MSG_DEST_PROCESS(msg) );

	if(!topo_endpoint)
	{
		topological_on_demand_connection_handler(rail, SCTK_MSG_DEST_PROCESS(msg) );
	}
	else
	{
		topo_endpoint = sctk_topological_rail_ellect_endpoint(
			SCTK_MSG_DEST_PROCESS(msg), msg, topo_endpoint);
	}

	/* By default use rail 0 */
	sctk_rail_info_t *endpoint_rail = rail->subrails[0];

	/* If we found a topo endpoint, override the 0 */
	if(topo_endpoint)
	{
		endpoint_rail = topo_endpoint->rail;
	}

	assume(endpoint_rail != NULL);
	assume(endpoint_rail->rdma_read != NULL);

	int subrail_id = endpoint_rail->subrail_id;
	assume(0 <= subrail_id);
	assume(subrail_id < rail->subrail_count);

	endpoint_rail->rdma_fetch_and_op(endpoint_rail, msg, fetch_addr,
	                                 local_key + subrail_id, remote_addr,
	                                 remote_key + subrail_id, add, op, type);
}

void sctk_topological_rail_cas(sctk_rail_info_t *rail,
                               mpc_lowcomm_ptp_message_t *msg,
                               void *res_addr,
                               struct  sctk_rail_pin_ctx_list *local_key,
                               void *remote_addr,
                               struct  sctk_rail_pin_ctx_list *remote_key,
                               void *comp,
                               void *new,
                               RDMA_type type)
{
	_mpc_lowcomm_endpoint_t *topo_endpoint =
		sctk_rail_get_any_route_to_process(rail, SCTK_MSG_DEST_PROCESS(msg) );

	if(!topo_endpoint)
	{
		topological_on_demand_connection_handler(rail, SCTK_MSG_DEST_PROCESS(msg) );
	}
	else
	{
		topo_endpoint = sctk_topological_rail_ellect_endpoint(
			SCTK_MSG_DEST_PROCESS(msg), msg, topo_endpoint);
	}

	/* By default use rail 0 */
	sctk_rail_info_t *endpoint_rail = rail->subrails[0];

	/* If we found a topo endpoint, override the 0 */
	if(topo_endpoint)
	{
		endpoint_rail = topo_endpoint->rail;
	}

	assume(endpoint_rail != NULL);
	assume(endpoint_rail->rdma_read != NULL);

	int subrail_id = endpoint_rail->subrail_id;
	assume(0 <= subrail_id);
	assume(subrail_id < rail->subrail_count);

	endpoint_rail->rdma_cas(endpoint_rail, msg, res_addr, local_key + subrail_id,
	                        remote_addr, remote_key + subrail_id, comp, new,
	                        type);
}

int sctk_topological_rail_rdma_fetch_and_op_gate(sctk_rail_info_t *rail, size_t size, RDMA_op op, RDMA_type type)
{
	/* Check on all rails */
	int i;

	for(i = 0; i < rail->subrail_count; i++)
	{
		sctk_rail_info_t *subrail = rail->subrails[i];

		if(!subrail->rdma_fetch_and_op_gate)
		{
			return 0;
		}

		if( (subrail->rdma_fetch_and_op_gate)(subrail, size, op, type) == 0)
		{
			return 0;
		}
	}

	return 1;
}

int sctk_topological_rail_rdma_cas_gate(sctk_rail_info_t *rail, size_t size, RDMA_type type)
{
	/* Check on all rails */
	int i;

	for(i = 0; i < rail->subrail_count; i++)
	{
		sctk_rail_info_t *subrail = rail->subrails[i];

		if(!subrail->rdma_cas_gate)
		{
			return 0;
		}

		if( (subrail->rdma_cas_gate)(subrail, size, type) == 0)
		{
			return 0;
		}
	}

	return 1;
}

void sctk_network_init_topological(sctk_rail_info_t *rail)
{
	/* Register Hooks in rail */
	rail->send_message_endpoint     = _mpc_lowcomm_topological_send_message;
	rail->notify_recv_message       = NULL;
	rail->notify_matching_message   = NULL;
	rail->notify_perform_message    = NULL;
	rail->notify_idle_message       = NULL;
	rail->notify_probe_message      = NULL;
	rail->notify_any_source_message = NULL;
	rail->send_message_from_network = NULL;
	rail->control_message_handler   = NULL;
	rail->connect_to        = NULL;
	rail->connect_from      = NULL;
	rail->rail_pin_region   = sctk_topological_rail_pin_region;
	rail->rail_unpin_region = sctk_topological_rail_unpin_region;

	rail->rdma_write = sctk_topological_rail_rdma_write;
	rail->rdma_read  = sctk_topological_rail_rdma_read;

	rail->rdma_fetch_and_op_gate = sctk_topological_rail_rdma_fetch_and_op_gate;
	rail->rdma_fetch_and_op      = sctk_topological_rail_rdma_fetch_and_op;

	rail->rdma_cas_gate = sctk_topological_rail_rdma_cas_gate;
	rail->rdma_cas      = sctk_topological_rail_cas;

	rail->network_name = "Topological RAIL";

	sctk_network_init_topological_rail_info(rail);

	/* Init Routes (intialize on-demand context) */
	sctk_rail_init_route(rail, rail->runtime_config_rail->topology, topological_on_demand_connection_handler);
}
