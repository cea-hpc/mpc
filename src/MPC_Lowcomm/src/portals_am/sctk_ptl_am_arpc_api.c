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

#include "mpc_lowcomm.h"
#include "sctk_ptl_am_iface.h"
#include "mpc_config.h"
#include "mpc_common_rank.h"
#include "arpc_weak.h"

#include <mpc_common_rank.h>

static sctk_ptl_am_rail_info_t srail;
static void* cxx_pool;

static size_t nb_process;
static sctk_ptl_id_t* id_maps = NULL;

int mpc_arpc_init_ptl(int nb_srv)
{
	unsigned int i;
	srail = sctk_ptl_am_hardware_init();
	sctk_ptl_am_software_init(&srail, nb_srv);

	nb_process = mpc_common_get_process_count();
	id_maps = (sctk_ptl_id_t*)sctk_malloc(sizeof(sctk_ptl_id_t) * nb_process);

	for(i = 0; i < nb_process; i++)
	{
		id_maps[i] = SCTK_PTL_ANY_PROCESS;
	}

	id_maps[mpc_common_get_process_rank()] = sctk_ptl_am_self(&srail);

	sctk_ptl_am_register_process(&srail);
	return 0;
}


int mpc_arpc_emit_call_ptl(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size)
{
	sctk_ptl_am_msg_t msg, *msg_ret = NULL;
	msg.remote = id_maps[ctx->dest];

	if(__sctk_ptl_am_id_undefined(msg.remote))
	{
		msg.remote = sctk_ptl_am_map_id(&srail, ctx->dest);
		assert(!__sctk_ptl_am_id_undefined(msg.remote));
	}

	/* yes, it is dirty to provide a pointer to sctk_ptl_am_msg as a parameter and wait for
	 * an another sctk_ptl_am_msg pointer as return value. We chose this way to keep the same prototype
	 * for the send & the recv interface call.
	 * This could (should ?) be changed lated if necessary.
	 */
	msg_ret = sctk_ptl_am_send_request(&srail, ctx->srvcode, ctx->rpcode, input, req_size, response, resp_size, &msg);

	if(response != NULL)
	{
		assert(msg_ret);
		sctk_ptl_am_wait_response(&srail, msg_ret);
		*response = (void*)msg_ret->msg_type.rep.addr;
		*resp_size = msg_ret->size;
	}
	return 0;
}

int mpc_arpc_recv_call_ptl(sctk_arpc_context_t* ctx, const void* input, size_t req_size, void** response, size_t*resp_size, struct sctk_ptl_am_msg_s* msg)
{
	*response = NULL;
	*resp_size = 0;

	ctx->cxx_pool = cxx_pool;

	arpc_c_to_cxx_converter(ctx, input, req_size, response, resp_size);
	if(response != NULL) /* there is something to return */
	{
		sctk_ptl_am_send_response(&srail, ctx->srvcode, ctx->rpcode, *response, *resp_size, ctx->dest, msg);
	}
	return 0;
}

int mpc_arpc_polling_request_ptl(__UNUSED__ sctk_arpc_context_t* ctx)
{
	int ret = sctk_ptl_am_incoming_lookup(&srail);
	if(!ret)
		mpc_thread_yield();
	return ret;
}

int mpc_arpc_register_service_ptl(void* pool, int srvcode)
{
	if(!cxx_pool)
		cxx_pool = pool;

	sctk_ptl_am_pte_create(&srail, srvcode);
	return 0;
}

int mpc_arpc_free_response_ptl(void* resp_addr)
{
	sctk_ptl_am_free_response(resp_addr);
	return 0;
}

#endif /* MPC_USE_PORTALS */
