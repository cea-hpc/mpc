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

#ifndef __SCTK_PTL_CM_H_
#define __SCTK_PTL_CM_H_

#ifdef MPC_USE_PORTALS
#include "sctk_ptl_am_types.h"
#include <mpc_lowcomm_msg.h>

void sctk_ptl_cm_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev);
void sctk_ptl_cm_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev);
void sctk_ptl_cm_send_message(mpc_lowcomm_ptp_message_t* msg, _mpc_lowcomm_endpoint_t* endpoint);

#endif /* MPC_USE_PORTALS */
#endif /* ifndef __SCTK_PTL_CM_H_ */
