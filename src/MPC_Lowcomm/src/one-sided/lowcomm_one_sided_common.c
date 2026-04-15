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
/* #   - MOREAU Gilles gilles.moreau@cea.fr                               # */
/* #   - BEAULIEU Corentin coretin.beaulieu@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_common_progress.h>
#include <sctk_alloc.h>
#include "mpc_lowcomm_one_sided.h"
#include "lowcomm_one_sided_internal.h"

int _lowcomm_osc_get_target_ep(mpc_lowcomm_window_t *const win, const int target, lcp_ep_h *ep_p)
{
	int rc = 0;

	if (win->eps[target] == NULL)
	{
		uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, target);

		rc = lcp_ep_create(win->mngr, &win->eps[target], target_uid, 0);
		if (rc != 0)
		{
			mpc_common_debug_error("Could not create endpoint.");
			goto err;
		}
	}

	*ep_p = win->eps[target];
err:
	return rc;
}

int _lowcomm_osc_perform_atomic(mpc_lowcomm_window_t *win, lcp_ep_h ep,
                                lcp_task_h task, uint64_t value, lcp_atomic_dt_t atomic_datatype,
                                uint64_t *result, uint64_t remote_addr,
                                lcp_mem_h rkey, lcp_atomic_op_t op)
{
	UNUSED(win);
	int              rc = MPC_LOWCOMM_SUCCESS;
	void *           req;
	lcp_status_ptr_t status;

	req = lcp_request_alloc(task);
	if (req == NULL)
	{
		mpc_common_debug_error("OSC COMMON: could not allocate request.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	lcp_request_param_t params =
	{
		.field_mask   = (result != NULL ? LCP_REQUEST_REPLY_BUFFER : 0) | LCP_REQUEST_USER_REQUEST,
		.request      = req,
		.reply_buffer = result,
	};

	status = lcp_atomic_op_nb(ep, task, &value, atomic_datatype, remote_addr, rkey, op, &params);
	if (LCP_PTR_IS_ERR(status))
	{
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	while (lcp_request_check_status(req) != MPC_LOWCOMM_SUCCESS)
	{
		rc = mpc_common_progress();
	}

	lcp_request_free(req);

err:
	return rc;
}

int _lowcomm_osc_perform_flush(mpc_lowcomm_window_t *win, lcp_task_h task, lcp_ep_h ep, lcp_mem_h mem)
{
	int              rc = MPC_LOWCOMM_SUCCESS;
	void *           req;
	lcp_status_ptr_t status;

	req = lcp_request_alloc(task);
	if (req == NULL)
	{
		mpc_common_debug_error("OSC COMMON: could not allocate request.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	lcp_request_param_t params =
	{
		.field_mask = (ep != NULL ? LCP_REQUEST_USER_EPH : 0)
		              | (mem != NULL ? LCP_REQUEST_USER_MEMH : 0)
		              | LCP_REQUEST_USER_REQUEST,
		.request = req,
		.ep      = ep,
		.mem     = mem,
	};

	status = lcp_flush_nb(win->mngr, task, &params);
	if (LCP_PTR_IS_ERR(status))
	{
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	while (lcp_request_check_status(req) != MPC_LOWCOMM_SUCCESS)
	{
		rc = mpc_common_progress();
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}
	}

	rc = MPC_LOWCOMM_SUCCESS;

	lcp_request_free(req);

err:
	return rc;
}

int _lowcomm_osc_find_attached_region_position(mpc_osc_dynamic_win_t *regions,
                                               int min_idx, int max_idx, uint64_t base,
                                               size_t len, int *insert_idx)
{
	int mid_idx = (min_idx + max_idx) >> 1;

	if (min_idx > max_idx)
	{
		*insert_idx = min_idx;
		return -1;
	}

	if (base < regions[mid_idx].base)
	{
		return _lowcomm_osc_find_attached_region_position(regions, min_idx, mid_idx - 1, base, len, insert_idx);
	}
	else if (base + len < regions[mid_idx].base + regions[mid_idx].size)
	{
		return mid_idx;
	}
	else
	{
		return _lowcomm_osc_find_attached_region_position(regions, mid_idx + 1, max_idx, base, len, insert_idx);
	}
}

int _lowcomm_osc_get_dynamic_win_info(lcp_ep_h ep, lcp_task_h task,
                                      uint64_t remote_addr,
                                      mpc_lowcomm_window_t *win,
                                      int target, lcp_mem_h *rmem_p)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_status_ptr_t       status;
	void *                 rdyn_win_info;
	lcp_request_param_t    params;
	uint64_t               dynamic_win_count;
	mpc_osc_dynamic_win_t *tmp_dyn_win_infos;
	size_t                 rdyn_win_info_len = sizeof(uint64_t) + OSC_DYNAMIC_WIN_ATTACH_MAX
	                                           * sizeof(mpc_osc_dynamic_win_t);
	lcp_mem_h rkey_state = win->rstate_win_info[target].rkey;
	lcp_mem_h rkey_data;
	uint64_t  remote_state_dyn_addr = win->rstate_win_info[target].addr
	                                  + OSC_STATE_DYNAMIC_WIN_COUNT_OFFSET;
	int found_idx, insert_idx;

	assert(ep != NULL && task != NULL);

	rdyn_win_info = sctk_malloc(rdyn_win_info_len);
	if (rdyn_win_info == NULL)
	{
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	/* Take the region lock since another region could be added concurrently at the target. */
	rc = start_exclusive_lock(win, task,
		OSC_STATE_REGION_LOCK_OFFSET,
		target);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	params = (lcp_request_param_t) {
		.field_mask = 0,
		.ep         = ep,
		.datatype   = LCP_DATATYPE_CONTIGUOUS,
	};

	status = lcp_get_nb(ep, task, rdyn_win_info, rdyn_win_info_len, remote_state_dyn_addr, rkey_state, &params);
	if (LCP_PTR_IS_ERR(status))
	{
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	rc = end_exclusive_lock(win, task,
		OSC_STATE_REGION_LOCK_OFFSET,
		target);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	rc = _lowcomm_osc_perform_flush(win, task, ep, NULL);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	memcpy(&dynamic_win_count, rdyn_win_info, sizeof(uint64_t));
	assert(dynamic_win_count < OSC_DYNAMIC_WIN_ATTACH_MAX);

	tmp_dyn_win_infos = (void *)((uintptr_t)rdyn_win_info + sizeof(uint64_t));
	found_idx         = _lowcomm_osc_find_attached_region_position(tmp_dyn_win_infos, 0,
		dynamic_win_count - 1, remote_addr, 1, &insert_idx);

	assert(found_idx >= 0 && (uint64_t)found_idx < dynamic_win_count);

	rc = lcp_mem_unpack(win->mngr,
		&rkey_data,
		tmp_dyn_win_infos[found_idx].rkey_buf,
		tmp_dyn_win_infos[found_idx].rkey_len);

	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	sctk_free(rdyn_win_info);

	*rmem_p = rkey_data;

err:
	return rc;
}

int _lowcomm_osc_get_remote_memory(mpc_lowcomm_window_t *const win, const lcp_ep_h ep,
                                   lcp_task_h task, const mpc_lowcomm_window_flag_t win_flavor,
                                   const int target, const uint64_t remote_addr,
                                   lcp_mem_h *rmem_p)
{
	int       rc = MPC_LOWCOMM_SUCCESS;
	lcp_mem_h rmem;

	if (win_flavor == MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC)
	{
		rc = _lowcomm_osc_get_dynamic_win_info(ep, task, remote_addr, win, target, &rmem);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}
	}
	else
	{
		rmem = win->rdata_win_info[target].rkey;
	}

	*rmem_p = rmem;
err:
	return rc;
}

int _lowcomm_osc_release_remote_memory(mpc_lowcomm_window_t *const win, lcp_mem_h rmem)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	if (win->win_flag == MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC)
	{
		rc = lcp_mem_deprovision(win->mngr, rmem);
	}
	return rc;
}
