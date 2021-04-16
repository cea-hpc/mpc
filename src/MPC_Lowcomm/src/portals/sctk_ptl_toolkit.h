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


#ifndef SCTK_PTL_TOOLKIT_H_
#define SCTK_PTL_TOOLKIT_H_

#include "endpoint.h"
#include <mpc_lowcomm_types.h>
#include <mpc_lowcomm_monitor.h>

sctk_ptl_id_t sctk_ptl_map_id(sctk_rail_info_t* rail, mpc_lowcomm_peer_uid_t dest);
void sctk_ptl_add_route(mpc_lowcomm_peer_uid_t dest, sctk_ptl_id_t id, sctk_rail_info_t* rail, _mpc_lowcomm_endpoint_type_t origin, _mpc_lowcomm_endpoint_state_t state);
void sctk_ptl_eqs_poll(sctk_rail_info_t* rail, size_t threshold);
void sctk_ptl_mds_poll(sctk_rail_info_t* rail, size_t threshold);
void sctk_ptl_free_memory(void* msg);
void sctk_ptl_message_copy(mpc_lowcomm_ptp_message_content_to_copy_t);
void sctk_ptl_comm_register(sctk_ptl_rail_info_t* srail, mpc_lowcomm_communicator_id_t comm_idx, size_t comm_size);
void sctk_ptl_init_interface(sctk_rail_info_t* rail);
void sctk_ptl_fini_interface(sctk_rail_info_t* rail);

void sctk_ptl_send_message(mpc_lowcomm_ptp_message_t* msg, _mpc_lowcomm_endpoint_t* endpoint);
void sctk_ptl_notify_recv(mpc_lowcomm_ptp_message_t* msg, sctk_rail_info_t* rail);
int sctk_ptl_pending_me_probe(sctk_rail_info_t* prail, mpc_lowcomm_ptp_message_header_t* hdr, int probe_level);

static inline sctk_rail_info_t* sctk_ptl_promote_to_rail(sctk_ptl_rail_info_t* srail)
{
        return (sctk_rail_info_t*)(container_of(srail, sctk_rail_info_t, network.ptl));
}

#endif
