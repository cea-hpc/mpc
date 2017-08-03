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
/* #   - GONCALVES Thomas thomas.goncalves@cea.fr                         # */
/* #   - ADAM Julien adamj@paratools.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_PORTALS_H_
#define __SCTK_PORTALS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <sctk_io_helper.h>
#ifdef MPC_USE_PORTALS
#include <sctk_portals_toolkit.h>

/**
 * Portals-specific information describing a route.
 * Is contained by route.h
 */
typedef struct sctk_portals_route_info_s
{
	sctk_portals_process_id_t dest; /**< the remote process the route is connected to */	
}
sctk_portals_route_info_t;

/**
 *
 */
typedef struct sctk_portals_msg_header_s
{
	sctk_portals_process_id_t remote;
	ptl_pt_index_t remote_index;
	ptl_match_bits_t tag;
	ptl_handle_ni_t* handler;
	void* payload;
	sctk_portals_pending_msg_list_t* list;

} sctk_portals_msg_header_t;

/**
 * Portals-specific information specializing a rail.
 * Is contained by rail.h
 */
typedef struct sctk_portals_rail_info_s
{
	sctk_portals_limits_t            max_limits;        /**< container for Portals thresholds */
	sctk_portals_interface_handler_t interface_handler; /**< Interface handler for the device */
	sctk_portals_process_id_t        current_id;        /**< Local id identifying this rail */
	sctk_portals_table_t             ptable;            /**< Portals table for this rail */

    char connection_infos[MAX_STRING_SIZE];                 /**< string identifying this rail over the PMI */
    size_t connection_infos_size;                           /**< Size of the above string */
} sctk_portals_rail_info_t;

void sctk_network_init_portals ( struct sctk_rail_info_s *rail);
void sctk_network_finalize_portals ( struct sctk_rail_info_s *rail);
#endif
#ifdef __cplusplus
}
#endif
#endif
