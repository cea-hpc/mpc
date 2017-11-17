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

#ifdef MPC_USE_PORTALS
#ifndef __SCTK_PORTALS_H_
#define __SCTK_PORTALS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include <sctk_io_helper.h>
#include "sctk_ptl_types.h"


/**
 *
 */
typedef struct sctk_portals_msg_header_s
{
	//sctk_portals_process_id_t remote;
	//ptl_pt_index_t remote_index;
	//ptl_match_bits_t tag;
	//ptl_handle_ni_t* handler;
	//void* payload;
	//sctk_portals_pending_msg_list_t* list;

} sctk_portals_msg_header_t;


void sctk_network_init_ptl ( struct sctk_rail_info_s *rail);
void sctk_network_finalize_ptl ( struct sctk_rail_info_s *rail);

#ifdef __cplusplus
}
#endif
#endif
#endif
