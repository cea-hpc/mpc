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
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_rail.h"
#include "sctk_net_topology.h"

#include <sctk_pmi.h>

/************************************************************************/
/* Rails Storage                                                        */
/************************************************************************/

/** Dynamic array storing rails */
static sctk_rail_info_t *rails = NULL;
/** Number of rails */
static int rail_number = 0;
static int rail_current_id = 0;
/** Set to 1 when routes have been committed by a call to \ref sctk_rail_commit */
static int is_route_finalized = 0;

/************************************************************************/
/* Rail Init and Getters                                                */
/************************************************************************/

void sctk_rail_allocate ( int count )
{
	rails = sctk_calloc ( count , sizeof ( sctk_rail_info_t ) );
	assume ( rails != NULL );
	rail_number = count;
}

sctk_rail_info_t * sctk_rail_new ( struct sctk_runtime_config_struct_net_rail *runtime_config_rail,
                                         struct sctk_runtime_config_struct_net_driver_config *runtime_config_driver_config )
{
	if ( rail_current_id == rail_number )
	{
		sctk_fatal ( "Error : Rail overflow detected\n" );
	}

	sctk_rail_info_t *new_rail = &rails[rail_current_id];

	new_rail->runtime_config_rail = runtime_config_rail;
	new_rail->runtime_config_driver_config = runtime_config_driver_config;
	new_rail->rail_number = rail_current_id;
	
	

	rail_current_id++;

	return new_rail;
}

int sctk_rail_count()
{
	return rail_number;
}


sctk_rail_info_t * sctk_rail_get_by_id ( int i )
{
	return & ( rails[i] );
}



/* Finalize Rails (call the rail route init func ) */
void sctk_rail_commit()
{
	char *net_name;
	int i;
	char *name_ptr;

	net_name = sctk_malloc ( rail_number * 4096 );
	name_ptr = net_name;

	for ( i = 0; i < rail_number; i++ )
	{
		rails[i].route_init ( & ( rails[i] ) );
		sprintf ( name_ptr, "\nRail %d/%d [%s (%s)]", i + 1, rail_number, rails[i].network_name, rails[i].topology_name );
		name_ptr = net_name + strlen ( net_name );
		sctk_pmi_barrier();
	}

	sctk_network_mode = net_name;
	is_route_finalized = 1;
}



/* Returs wether the route has been finalized */
int sctk_rail_committed()
{
	return is_route_finalized;
}



/************************************************************************/
/* route_init                                                           */
/************************************************************************/

void sctk_rail_init_route ( sctk_rail_info_t *rail, char *topology )
{
	rail->on_demand = 0;

	if ( strcmp ( "ring", topology ) == 0 )
	{
		rail->route = sctk_route_ring;
		rail->route_init = sctk_route_ring_init;
		rail->topology_name = "ring";
	}
	else
		if ( strcmp ( "fully", topology ) == 0 )
		{
			rail->route = sctk_route_fully;
			rail->route_init = sctk_route_fully_init;
			rail->topology_name = "fully connected";
		}
		else
			if ( strcmp ( "ondemand", topology ) == 0 )
			{
				rail->route = sctk_route_ondemand;
				rail->route_init = sctk_route_ondemand_init;
				rail->topology_name = "On-Demand connections";
			}
			else
				if ( strcmp ( "torus", topology ) == 0 )
				{
					rail->route = sctk_route_torus;
					rail->route_init = sctk_route_torus_init;
					rail->topology_name = "torus";
				}
				else
				{
					not_reachable();
				}
}


