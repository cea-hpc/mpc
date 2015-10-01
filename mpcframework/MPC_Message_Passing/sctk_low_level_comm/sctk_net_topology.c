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
#include "sctk_net_topology.h"
#include "sctk_route.h"

#include <sctk_pmi.h>



static sctk_spinlock_t sctk_topology_init_lock = SCTK_SPINLOCK_INITIALIZER;

/************************************************************************/
/* None                                                                 */
/************************************************************************/

void sctk_route_none_init ( sctk_rail_info_t *rail )
{
	/* Nothing to do here by definition */
}

/************************************************************************/
/* Random                                                               */
/************************************************************************/

void sctk_route_random_init ( sctk_rail_info_t *rail )
{
	/* This is possible only on rails where
	 * one sided connections were fully validated
	 * it means TPC for the moment */

	struct sctk_runtime_config_struct_net_driver_config * driver_config = rail->runtime_config_driver_config;
	enum sctk_runtime_config_struct_net_driver_type net_type = driver_config->driver.type;

	/* For the Moment only TCP is supported */
	if( net_type != SCTK_RTCFG_net_driver_tcp )
	{
		sctk_fatal("The random topology is only supported by TCP for the moment");
	}



}


/************************************************************************/
/* Ring                                                                 */
/************************************************************************/

void sctk_route_ring_init ( sctk_rail_info_t *rail )
{
	/* Nothing to do as the driver is supposed to
	 * provide a ring at startup */
}

int sctk_route_ring ( int dest, sctk_rail_info_t *rail )
{
	int old_dest;
	int delta_1;

	old_dest = dest;

	if ( sctk_process_rank > dest )
	{
		delta_1 = sctk_process_rank - dest;

		dest = ( delta_1 > sctk_process_number - delta_1 ) ?
		       ( sctk_process_rank + sctk_process_number + 1 ) % sctk_process_number :
		       ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number;
	}
	else
	{
		delta_1 = dest - sctk_process_rank;

		dest = ( delta_1 > sctk_process_number - delta_1 ) ?
		       ( sctk_process_rank + sctk_process_number - 1 ) % sctk_process_number :
		       ( sctk_process_rank + sctk_process_number + 1 ) % sctk_process_number;
	}

	sctk_nodebug ( "Route via dest - %d to %d (delta1:%d - process_rank:%d - process_number:%d)", dest, old_dest, delta_1, sctk_process_rank, sctk_process_number );

	return dest;
}


/************************************************************************/
/* Fully Connected                                                      */
/************************************************************************/
int sctk_route_fully ( int dest, sctk_rail_info_t *rail )
{
	not_reachable();
	return -1;
}

void sctk_route_fully_init ( sctk_rail_info_t *rail )
{
	int ( *sav_sctk_route ) ( int , sctk_rail_info_t * );

	sctk_pmi_barrier();

	if ( 3 < sctk_process_number  )
	{
		int from;
		int to;

		for ( from = 0; from < sctk_process_number; from++ )
		{
			for ( to = 0; to < sctk_process_number; to ++ )
			{
				if ( to > from )
				{
					sctk_endpoint_t *tmp;

					if ( from == sctk_process_rank )
					{

						tmp = sctk_rail_get_any_route_to_process ( rail, to );

						if ( tmp == NULL )
						{
							sctk_nodebug("ON-DEMAND : REQUEST %d <--> %d", from, to);
							rail->connect_from ( from, to, rail );
							SCTK_COUNTER_INC ( signalization_endpoints, 1 );
						}


					}

					if ( to == sctk_process_rank )
					{

						tmp = sctk_rail_get_any_route_to_process ( rail, from );

						if ( tmp == NULL )
						{
							rail->connect_to ( from, to, rail );
							SCTK_COUNTER_INC ( signalization_endpoints, 1 );
						}


					}
				}
			}
		}
		sctk_nodebug("%d finished its requests !!!!!!!", sctk_process_rank);

		sctk_pmi_barrier();
	}

	sctk_pmi_barrier();
	sctk_pmi_barrier();
}


/************************************************************************/
/* sctk_Torus_t                                                         */
/************************************************************************/

void sctk_route_torus_init(sctk_rail_info_t* rail)
{
	size_t nbprocs = sctk_get_process_number();
	size_t dim_x, dim_y;
	size_t i, offset, right, top, left, bot;
	
	dim_x = (int)sqrt(nbprocs);
	dim_y = dim_x;
	i = sctk_get_process_rank();

	/*if(nbprocs % dim_x > 0)*/
	/*{*/
		/*sctk_fatal("Torus with a number of process which is not a square, are not supported for now !");*/
	/*}*/

	offset = (int)(i/dim_x) * dim_x;

	right = ( ( (i + 1) % dim_y ) + offset) % nbprocs;
	left  = ( ( (i - 1) % dim_y ) + offset) % nbprocs;
	top   = ( i - dim_x )                   % nbprocs;
	bot   = ( i + dim_x)                    % nbprocs;

	if(sctk_rail_get_any_route_to_process ( rail, right ) == NULL)
	{
		rail->connect_from ( i, right, rail );
		SCTK_COUNTER_INC ( signalization_endpoints, 1 );
	}

    if(sctk_rail_get_any_route_to_process ( rail, left ) == NULL)
	{
		rail->connect_to( i, left, rail );
		SCTK_COUNTER_INC ( signalization_endpoints, 1 );
	}

	if(sctk_rail_get_any_route_to_process ( rail, top ) == NULL)
	{
		rail->connect_from ( i, top, rail );
		SCTK_COUNTER_INC ( signalization_endpoints, 1 );
	}

	if(sctk_rail_get_any_route_to_process ( rail, bot) == NULL)
	{
		rail->connect_to ( i, bot, rail );
		SCTK_COUNTER_INC ( signalization_endpoints, 1 );
	}
	

	sctk_pmi_barrier();
}
