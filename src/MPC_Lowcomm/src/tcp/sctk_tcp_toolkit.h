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

#ifndef __SCTK_TCP_TOOLKIT_H_
#define __SCTK_TCP_TOOLKIT_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "endpoint.h"
#include <mpc_lowcomm_monitor.h>

void sctk_network_init_tcp_all ( sctk_rail_info_t *rail, int sctk_use_tcp_o_ib,
                                 void * ( *tcp_thread ) ( _mpc_lowcomm_endpoint_t * ) );

void tcp_on_demand_connection_handler( sctk_rail_info_t *rail, mpc_lowcomm_peer_uid_t dest_process );

#ifdef __cplusplus
}
#endif
#endif

