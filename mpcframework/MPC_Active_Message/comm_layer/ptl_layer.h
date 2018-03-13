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

#ifndef __ARPC_PTL_LAYER_H_
#define __ARPC_PTL_LAYER_H_

#ifdef MPC_USE_PORTALS
#include <arpc.h>
#include <sctk_debug.h>

int arpc_init_ptl(int);
int arpc_register_service_ptl(int srvcode);

int arpc_emit_call_ptl(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size);
int arpc_recv_call_ptl(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size);

int arpc_polling_request_ptl(sctk_arpc_context_t* ctx);

#endif

#endif /* ifndef __ARPC_PTL_LAYER_H_ */
