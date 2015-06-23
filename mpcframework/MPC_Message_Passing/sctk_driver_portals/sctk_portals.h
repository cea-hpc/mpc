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
/* #   - ADAM Julien julien.adam@cea.fr                                   # */
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
//#ifndef __SCTK_ROUTE_H_
//#error "sctk_route must be included before sctk_portals.h"
//#endif

struct sctk_portals_route_info_s
{
	ptl_process_t id;//to route
};
typedef struct sctk_portals_route_info_s sctk_portals_route_info_t;


struct sctk_portals_rail_info_s
{
	ptl_ni_limits_t   actual;
	ptl_handle_ni_t   ni_handle_phys;
	ptl_process_t     my_id;
	int               nb_tasks_per_process;
	ptl_pt_index_t    *pt_index; // one per thread
	sctk_portals_event_table_list_t     *event_list;	//event list, one by thread
	ptl_handle_eq_t   *event_handler;
	sctk_spinlock_t   *lock; //event list, one by thread
	ptl_ct_event_t    zeroCounter;

    char connection_infos[MAX_STRING_SIZE];
    size_t connection_infos_size;
};
typedef struct sctk_portals_rail_info_s sctk_portals_rail_info_t;

void sctk_network_init_portals ( sctk_rail_info_t *rail);
#endif
#ifdef __cplusplus
}
#endif
#endif
