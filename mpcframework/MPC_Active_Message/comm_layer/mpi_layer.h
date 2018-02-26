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

#ifndef __ARPC_MPI_LAYER_H
#define __ARPC_MPI_LAYER_H

#include <arpc.h>
#include <mpi.h>

#define MPI_ARPC_TAG  1000
#define __arpc_print_ctx(ctx, format,...) do { \
	sctk_warning("[%d,S:%d,C:%d] "format, ctx->dest,ctx->srvcode, ctx->rpcode, ##__VA_ARGS__);} while(0)
#define MAX_STATIC_ARPC_SIZE 4096
#define SCTK_SIZEOF_INTERNAL_CTX (4 * sizeof(int))
typedef struct sctk_arpc_mpi_ctx_s
{
	int rpcode;
	int srvcode;
	int next_tag;
	int msize;
	char raw[MAX_STATIC_ARPC_SIZE];
} sctk_arpc_mpi_ctx_t;

int arpc_emit_call_mpi(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size);
int arpc_recv_call_mpi(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_sizem );

int arpc_polling_request_mpi(sctk_arpc_context_t* ctx);


#endif /* ifndef __ARPC_MPI_LAYER_H */
