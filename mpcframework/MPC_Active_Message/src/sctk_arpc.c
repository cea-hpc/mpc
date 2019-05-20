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

#include <arpc.h>
#include <mpi_layer.h>
#include <ptl_layer.h>
#include <stdio.h>

#ifdef MPC_Active_Message
#include <sctk_runtime_config.h>
#else
#include <assert.h>
#include <stdlib.h>
#define sctk_assert assert
#define sctk_fatal(...) do{fprintf(stderr, __VA_ARGS__); exit(42); } while(0)
#define not_reachable() do { fprintf(stderr, "line %s:%d should not be reached !!\n", __FUNCTION__, __LINE__); } while(0)
#endif

static int (*reg_fn)(void*,int);
static int (*free_fn)(void*);
static int (*emit_fn)(sctk_arpc_context_t*, const void*, size_t, void**, size_t*) = NULL;
static int (*recv_fn)(sctk_arpc_context_t*, const void*, size_t, void**, size_t*) = NULL;
static int (*poll_fn)(sctk_arpc_context_t*) = NULL;
static int (*init_fn)(int) = NULL;

static struct sctk_runtime_config_struct_arpc_type arpc_config;

static void __arpc_init_callbacks()
{
	switch(arpc_config.net_layer)
	{
		case ARPC_MPI:
			reg_fn  = arpc_register_service_mpi;
			emit_fn = arpc_emit_call_mpi;
			recv_fn = arpc_recv_call_mpi;
			poll_fn = arpc_polling_request_mpi;
			init_fn = arpc_init_mpi;
			free_fn = arpc_free_response_mpi;
			break;
		case ARPC_PTL:
#ifdef MPC_USE_PORTALS
			reg_fn  = arpc_register_service_ptl;
			emit_fn = arpc_emit_call_ptl;
			recv_fn = NULL;
			poll_fn = arpc_polling_request_ptl;
			init_fn = arpc_init_ptl;
			free_fn = arpc_free_response_ptl;
#else
			sctk_fatal("Cannot use Portals4 implementation without compiling MPC w/ Portals support");
#endif
			break;
		default:
			not_reachable();
	}
}

void arpc_init()
{
	arpc_config = sctk_runtime_config_get_checked()->modules.arpc;
	__arpc_init_callbacks();
	sctk_assert(init_fn);
	init_fn(arpc_config.nb_srv);
}

int arpc_register_service(void* pool, int code)
{
	return reg_fn(pool, code);
}

int arpc_emit_call(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	sctk_assert(emit_fn);
	sctk_assert(ctx);
	return emit_fn(ctx, input, req_size, response, resp_size);
}

int arpc_recv_call(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	sctk_assert(recv_fn);
	sctk_assert(ctx);
	return recv_fn(ctx, input, req_size, response, resp_size);
}

int arpc_polling_request(sctk_arpc_context_t* ctx)
{
	sctk_assert(poll_fn);
	sctk_assert(ctx);
	return poll_fn(ctx);
}

int arpc_free_response(void* addr)
{
	sctk_assert(free_fn);
	return free_fn(addr);
}
