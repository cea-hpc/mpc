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

#ifndef __SCTK_PORTALS_TOOLKIT_H_
#define __SCTK_PORTALS_TOOLKIT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef MPC_USE_PORTALS
#include <sctk_portals_helper.h>

typedef struct sctk_endpoint_s sctk_endpoint_t;
typedef struct sctk_rail_info_s sctk_rail_info_t;
typedef struct sctk_thread_ptp_message_s sctk_thread_ptp_message_t;

typedef enum
{
	SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_STATIC,
	SCTK_PORTALS_CONTROL_MESSAGE_ON_DEMAND_DYNAMIC
} sctk_portals_control_message_type_t;

typedef struct sctk_portals_connection_context_s
{
	sctk_portals_process_id_t from;
    ptl_pt_index_t entry;
	ptl_match_bits_t match;

} sctk_portals_connection_context_t;

void sctk_network_init_portals_all ( sctk_rail_info_t *rail );
void sctk_portals_on_demand_connection_handler( sctk_rail_info_t *rail, int dest_process );
void sctk_portals_send_put ( sctk_endpoint_t *endpoint, sctk_thread_ptp_message_t *msg);
int sctk_portals_polling_queue_for(sctk_rail_info_t*rail, size_t task_id);
#endif
#ifdef __cplusplus
}
#endif // MPC_USE_PORTALS
#endif // end .h file
