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

#ifndef __ARPC_H_
#define __ARPC_H_
#include <string.h>

struct sctk_arpc_context
{
	int dest;
	int rpcode;
	int srvcode;
	void* cxx_pool;
};
typedef struct sctk_arpc_context sctk_arpc_context_t;

int arpc_emit_call(sctk_arpc_context_t* ctx, const void* request, size_t req_size, void** response, size_t* resp_size);
int arpc_recv_call(sctk_arpc_context_t* ctx, const void* request, size_t req_size, void** response, size_t* resp_size);

int arpc_polling_request(sctk_arpc_context_t* ctx);

/* injected into the application code when building */
#pragma weak arpc_c_to_cxx_converter
int arpc_c_to_cxx_converter(sctk_arpc_context_t* ctx, const void * request, size_t req_size, void** response, size_t* resp_size);
#endif /* ifndef __ARPC_H_ */
