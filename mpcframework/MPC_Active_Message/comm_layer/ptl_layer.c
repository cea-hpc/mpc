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

#include "ptl_layer.h"
#include "sctk_ptl_am_iface.h"

static sctk_ptl_am_rail_info_t srail;

int arpc_init_ptl()
{
	srail = sctk_ptl_am_hardware_init();
	sctk_ptl_am_software_init(&srail, 1);
	return 0;
}


int arpc_emit_call_ptl(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	sctk_ptl_am_send_request(&srail, ctx->srvcode, ctx->rpcode, input, req_size, response, resp_size, sctk_ptl_am_self(&srail));
	return 0;
}

int arpc_recv_call_ptl(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	not_implemented();
}

int arpc_polling_request_ptl(sctk_arpc_context_t* ctx)
{
	not_implemented();
}

#endif
