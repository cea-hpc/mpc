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
/* #   - REYNIER Florian    florian.reynier@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_LCP_INIT_H
#define MPC_LCP_INIT_H

#include <mpc_common_debug.h>
#include <mpc_common_rank.h>
#include <mpc_common_helper.h>
#include <sctk_alloc.h>
#include <mpc_common_flags.h>
#include <lcp.h>
#include "lowcomm_request.h"
#include "lcp/lcp_init.h"
#include "lowcomm_protocol.h"
#include "mpc_lowcomm_ep.h"
#include "mpc_lowcomm_monitor.h"
#include "mpc_lowcomm_types.h"

int mpc_comm_progress();

extern lcp_context_h           lcp_ctx_loc;
extern lcp_manager_h           lcp_mngr_loc;
extern struct mpi_request_info mpi_req_info;

void mpc_lowcomm_protocol_init_driver()
{
	int rc;
	// FIXME: task count is number of MPI task while ctx->num_tasks should be
	//       the number of local tasks. At this point,
	//       mpc_common_get_local_task_count() is not setup yet.
	lcp_context_param_t ctx_params =
	{
		.field_mask   = LCP_CONTEXT_DATATYPE_OPS
		                | LCP_CONTEXT_NUM_TASKS
		                | LCP_CONTEXT_PROCESS_UID
		                | LCP_CONTEXT_REQUEST_SIZE
		                | LCP_CONTEXT_REQUEST_CB,
		.dt_ops       =
		{
			.pack   = mpc_lowcomm_request_pack,
			.unpack = mpc_lowcomm_request_unpack,
		},
		.num_tasks    = mpc_common_get_task_count(),
		.process_uid  = mpc_lowcomm_monitor_get_uid(),
		.request_init = mpc_lowcomm_request_initialize,
		.request_size = mpi_req_info.size + sizeof(mpc_lowcomm_request_t),
	};

	rc = lcp_context_create(&lcp_ctx_loc, &ctx_params);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("COMM: context creation failed.");
	}

	lcp_manager_param_t mngr_params =
	{
		.field_mask    = LCP_MANAGER_ESTIMATED_EPS | LCP_MANAGER_COMM_MODEL,
		.estimated_eps = mpc_common_get_process_count(),
		.flags         = LCP_MANAGER_TSC_MODEL,
	};
	rc = lcp_manager_create(lcp_ctx_loc, &lcp_mngr_loc, &mngr_params);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("COMM: manager creation failed.");
	}
}

int mpc_comm_progress()
{
	return lcp_progress(lcp_mngr_loc);
}

void mpc_lowcomm_init_protocol_per_task()
{
	int        task_rank = mpc_common_get_task_rank();
	lcp_task_h task;
	int        rc = lcp_task_create(lcp_ctx_loc, task_rank, &task);

	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("LCP: could not create task tid=%d", task_rank);
	}

	rc = lcp_task_associate(task, lcp_mngr_loc);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("LCP: could not associate task. tid=%d.", task_rank);
	}
}

void mpc_lowcomm_protocol_finalize_driver()
{
	lcp_manager_fini(lcp_mngr_loc);
	lcp_context_fini(lcp_ctx_loc);
}

void mpc_lowcomm_release_protocol_per_task()
{
	int        task_rank = mpc_common_get_task_rank();
	lcp_task_h task      = lcp_context_task_get(lcp_ctx_loc, task_rank);

	if (task == NULL)
	{
		mpc_common_debug_fatal("LCP: Could not find task with tid %d.", task_rank);
	}
	lcp_task_dissociate(task, lcp_mngr_loc);
}

void mpc_lowcomm_ep_free(mpc_lowcomm_ep_t *eps)
{
	free(eps);
}

#endif
