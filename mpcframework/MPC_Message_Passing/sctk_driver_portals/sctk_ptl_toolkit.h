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
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS
#ifndef SCTK_PTL_TOOLKIT_H_
#define SCTK_PTL_TOOLKIT_H_

#include "sctk_route.h"
void sctk_ptl_create_ring(sctk_rail_info_t *rail);
sctk_ptl_id_t sctk_ptl_map_id(sctk_rail_info_t* rail, int dest);
void sctk_ptl_add_route(int dest, sctk_ptl_id_t id, sctk_rail_info_t* rail, sctk_route_origin_t origin, sctk_endpoint_state_t state);
void sctk_ptl_eqs_poll(sctk_rail_info_t* rail, size_t threshold);
void sctk_ptl_mds_poll(sctk_rail_info_t* rail, size_t threshold);
void sctk_ptl_free_memory(void* msg);
void sctk_ptl_message_copy(sctk_message_to_copy_t);
void sctk_ptl_comm_register(sctk_ptl_rail_info_t* srail, int comm_idx, size_t comm_size);
void sctk_ptl_init_interface(sctk_rail_info_t* rail);
void sctk_ptl_fini_interface(sctk_rail_info_t* rail);

void sctk_ptl_send_message(sctk_thread_ptp_message_t* msg, sctk_endpoint_t* endpoint);
void sctk_ptl_notify_recv(sctk_thread_ptp_message_t* msg, sctk_rail_info_t* rail);

static inline sctk_rail_info_t* sctk_ptl_promote_to_rail(sctk_ptl_rail_info_t* srail)
{
        return (sctk_rail_info_t*)(container_of(srail, sctk_rail_info_t, network.ptl));
}

#endif
#endif
