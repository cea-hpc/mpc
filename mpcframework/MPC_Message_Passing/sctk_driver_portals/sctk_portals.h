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
/* #                                                                      # */
/* ######################################################################## */

#ifndef __SCTK_PORTALS_H_
#define __SCTK_PORTALS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <sctk_portals_toolkit.h>
#ifdef MPC_USE_PORTALS

typedef struct sctk_portals_route_info_s
{
	ptl_process_t id;//to route
} sctk_portals_route_info_t;


typedef struct sctk_portals_rail_info_s
{
	ptl_ni_limits_t   actual;
	ptl_handle_ni_t   ni_handle_phys;
	ptl_process_t     my_id;
	int               ntasks;
	ptl_pt_index_t    *pt_index;
	sctk_EventQ_t     *EvQ;	//event list, one by thread
	ptl_handle_eq_t   *eq_h;
	sctk_spinlock_t   *lock; //event list, one by thread
	ptl_ct_event_t    zeroCounter;

} sctk_portals_rail_info_t;

void sctk_network_init_portals ( sctk_rail_info_t *rail);
#ifdef __cplusplus
}
#endif
#endif
#endif
