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
#include <math.h>
#include <mpc_launch_pmi.h>
#include <mpc_common_rank.h>
#include <mpc_common_profiler.h>

/************************************************************************/
/* None                                                                 */
/************************************************************************/

void sctk_route_none_init (  __UNUSED__ sctk_rail_info_t *rail )
{
	/* Nothing to do here by definition */
}

/************************************************************************/
/* Ring                                                                 */
/************************************************************************/
/**
 * Fake function because MPC assume that any rail will create a ring by default.
 * This function does nothing because of this assumption.
 * \param[in] rail no used.
 */
void sctk_route_ring_init (  __UNUSED__ sctk_rail_info_t *rail )
{
	/* Nothing to do as the driver is supposed to
	 * provide a ring at startup */
}

int sctk_route_ring ( int dest,  __UNUSED__ sctk_rail_info_t *rail )
{
	int delta_1;


	if ( mpc_common_get_process_rank() > dest )
	{
		delta_1 = mpc_common_get_process_rank() - dest;

		dest = ( delta_1 > mpc_common_get_process_count() - delta_1 ) ?
		       ( mpc_common_get_process_rank() + mpc_common_get_process_count() + 1 ) % mpc_common_get_process_count() :
		       ( mpc_common_get_process_rank() + mpc_common_get_process_count() - 1 ) % mpc_common_get_process_count();
	}
	else
	{
		delta_1 = dest - mpc_common_get_process_rank();

		dest = ( delta_1 > mpc_common_get_process_count() - delta_1 ) ?
		       ( mpc_common_get_process_rank() + mpc_common_get_process_count() - 1 ) % mpc_common_get_process_count() :
		       ( mpc_common_get_process_rank() + mpc_common_get_process_count() + 1 ) % mpc_common_get_process_count();
	}


	return dest;
}


/************************************************************************/
/* Fully Connected                                                      */
/************************************************************************/
int sctk_route_fully (  __UNUSED__ int dest,  __UNUSED__ sctk_rail_info_t *rail )
{
	not_reachable();
	return -1;
}

/**
 * Create a mesh topology for the given rail.
 * \param[in] rail the rail owning the future topology.
 */
void sctk_route_fully_init ( sctk_rail_info_t *rail )
{

 mpc_launch_pmi_barrier();

	if ( 3 < mpc_common_get_process_count()  )
	{
		int from;
		int to;

		for ( from = 0; from < mpc_common_get_process_count(); from++ )
		{
			for ( to = 0; to < mpc_common_get_process_count(); to ++ )
			{
				if ( to > from )
				{
					_mpc_lowcomm_endpoint_t *tmp;

					if ( from == mpc_common_get_process_rank() )
					{

						tmp = sctk_rail_get_any_route_to_process ( rail, to );

						if ( tmp == NULL )
						{
							mpc_common_nodebug("ON-DEMAND : REQUEST %d <--> %d", from, to);
							rail->connect_from ( from, to, rail );
							SCTK_COUNTER_INC ( signalization_endpoints, 1 );
						}


					}

					if ( to == mpc_common_get_process_rank() )
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
		mpc_common_nodebug("%d finished its requests !!!!!!!", mpc_common_get_process_rank());

	 mpc_launch_pmi_barrier();
	}

 mpc_launch_pmi_barrier();
 mpc_launch_pmi_barrier();
}


/************************************************************************/
/* sctk_Torus_t                                                         */
/************************************************************************/
/**
 * Will create a 3D torus topology.
 * \param[in] railt the rail to use for creating the topology
 */
void sctk_route_torus_init(sctk_rail_info_t* rail)
{
	size_t nbprocs = mpc_common_get_process_count();
	size_t dim_x, dim_y;
	size_t i, offset, right, top, left, bot;
	
	dim_x = (int)sqrt(nbprocs);
	dim_y = dim_x;
	i = mpc_common_get_process_rank();

	/*if(nbprocs % dim_x > 0)*/
	/*{*/
		/*mpc_common_debug_fatal("Torus with a number of process which is not a square, are not supported for now !");*/
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
	

 mpc_launch_pmi_barrier();
}
