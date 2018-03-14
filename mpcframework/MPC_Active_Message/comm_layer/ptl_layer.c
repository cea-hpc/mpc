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
#include "sctk_config.h"

static sctk_ptl_am_rail_info_t srail;

int arpc_init_ptl(int nb_srv)
{
	srail = sctk_ptl_am_hardware_init();
	sctk_ptl_am_software_init(&srail, nb_srv);
	sctk_ptl_am_create_ring(&srail);
	return 0;
}


int arpc_emit_call_ptl(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	sctk_ptl_am_send_request(&srail, ctx->srvcode, ctx->rpcode, input, req_size, response, resp_size, ctx->dest);
	return 0;
}

int arpc_recv_call_ptl(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	*response = NULL;
	*resp_size = 0;
	arpc_c_to_cxx_converter(ctx, input, req_size, response, resp_size);	
	
	if(*response != NULL) /* there is something to return */
	{
		sctk_ptl_am_send_response(&srail, ctx->srvcode, ctx->rpcode, *response, *resp_size, ctx->dest);
	}
	return 0;
}

int arpc_polling_request_ptl(sctk_arpc_context_t* ctx)
{
	sctk_ptl_am_incoming_lookup(&srail);
	return 0;
}

int arpc_register_service_ptl(int srvcode)
{
	sctk_ptl_am_pte_create(&srail, srvcode);
	return 0;
}

#endif
