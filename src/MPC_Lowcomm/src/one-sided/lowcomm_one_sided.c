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

/**@file
 * One sided communication routines implementation
 */

#include "lowcomm_one_sided_internal.h"
#include "mpc_lowcomm_msg.h"     // For MPC_LOWCOMM_MESSAGE_DONE
#include "rma/lcp_rma.h"         // For atomic type and sizes

#include <mpc_common_rank.h>     // For mpc_common_get_task_rank
#include <mpc_common_progress.h> // For mpc_common_progress

/**@ingroup
 * @brief Complete an OSC request
 *
 * @param[in] status  Error status of the communication
 * @param[in] request Request to complete
 * @param[in] length  Size of the message sent/received
 *
 * @return            MPC_LOWCOMM_SUCCESS upon success
 */
static int _mpc_osc_request_send_complete(const int status, void *request, const size_t length)
{
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

	if (request == &MPC_REQUEST_NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	// The message may have been completed beforehand on certain circumstances (zero length RMA op...)
	if (req->completion_flag == MPC_LOWCOMM_MESSAGE_DONE)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	if (status != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_fatal("COMM: request failed.");
	}

	// FIXME: MPI standard does not require the size, tag and src fields to
	//       be filled but some MPC test in the CI requires send, so set
	//       them from header info.
	req->status_error     = status;
	req->recv_info.length = length;

	req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_osc_request_complete(const int status, mpc_lowcomm_request_t *request, const size_t length)
{
	return _mpc_osc_request_send_complete(status, request, length);
}

int mpc_lowcomm_iput(const void *const origin_buffer, const int size,
                     const int target, const intptr_t offset,
                     mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *const req)
{
	int            rc          = MPC_LOWCOMM_SUCCESS;
	const uint64_t remote_addr = win->rdata_win_info[target].addr + offset;
	lcp_task_h     task;
	lcp_mem_h      rmem;
	lcp_ep_h       ep;

	if (req != NULL && req != &MPC_REQUEST_NULL)
	{
		mpc_lowcomm_request_init(req, 1, NULL, NULL, 0);
		mpc_lowcomm_request_set_null(req, 0);
		req->request_type    = REQUEST_SEND;
		req->comm            = win->comm;
		req->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;
	}

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());
	_lowcomm_osc_get_target_ep(target, win, &ep);

	rc = _lowcomm_osc_get_remote_memory(task, target, remote_addr, win, &rmem);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}
	lcp_request_param_t req_param =
	{
		.ep         = ep,
		.datatype   = LCP_DATATYPE_CONTIGUOUS,
		.send_cb    = _mpc_osc_request_send_complete,
		.request    = req,
		.field_mask = req == &MPC_REQUEST_NULL ? 0 : (LCP_REQUEST_USER_REQUEST | LCP_REQUEST_SEND_CALLBACK),
	};

	lcp_status_ptr_t status;
	status = lcp_put_nb(ep, task, (void *)origin_buffer, size, remote_addr, rmem, &req_param);

	if (LCP_PTR_IS_ERR(status))
	{
		rc = MPC_LOWCOMM_ERROR;
	}

	_lowcomm_osc_release_remote_memory(&rmem, win);
err:
	return rc;
}

int mpc_lowcomm_iget(void *origin_buffer, const int size,
                     const int target, const intptr_t offset,
                     mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *const req)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	uint64_t remote_addr = win->rdata_win_info[target].addr + offset;


	lcp_task_h task;
	lcp_ep_h   ep;
	lcp_mem_h  rmem;

	if (req != NULL && req != &MPC_REQUEST_NULL)
	{
		mpc_lowcomm_request_init(req, 1, NULL, NULL, 0);
		mpc_lowcomm_request_set_null(req, 0);
		req->request_type    = REQUEST_RECV;
		req->comm            = win->comm;
		req->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;
	}

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());
	_lowcomm_osc_get_target_ep(target, win, &ep);

	rc = _lowcomm_osc_get_remote_memory(task, target, remote_addr, win, &rmem);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}
	lcp_request_param_t req_param =
	{
		.ep         = ep,
		.datatype   = LCP_DATATYPE_CONTIGUOUS,
		.send_cb    = _mpc_osc_request_send_complete,
		.request    = req,
		.field_mask = req == &MPC_REQUEST_NULL ? 0 : (LCP_REQUEST_USER_REQUEST | LCP_REQUEST_SEND_CALLBACK),
	};

	lcp_status_ptr_t status;
	status = lcp_get_nb(ep, task, origin_buffer, size, remote_addr, rmem, &req_param);

	if (LCP_PTR_IS_ERR(status))
	{
		rc = MPC_LOWCOMM_ERROR;
	}

	_lowcomm_osc_release_remote_memory(&rmem, win);
err:
	return rc;
}

int mpc_lowcomm_iflush(const int target, mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_status_ptr_t status;

	if (req == NULL || req == &MPC_REQUEST_NULL)
	{
		mpc_common_debug_error("Invalid request");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	lcp_task_h task;
	lcp_ep_h   ep;
	mpc_lowcomm_request_init(req, 0, NULL, NULL, 0);
	mpc_lowcomm_request_set_null(req, 0);
	req->request_type    = REQUEST_SEND;
	req->comm            = win->comm;
	req->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());
	// Flush all ?
	if (target == MPC_ANY_SOURCE)
	{
		ep = NULL;
	}
	else
	{
		_lowcomm_osc_get_target_ep(target, win, &ep);
	}

	lcp_request_param_t params =
	{
		.field_mask = ((ep != NULL ? LCP_REQUEST_USER_EPH : 0)) | LCP_REQUEST_USER_REQUEST | LCP_REQUEST_SEND_CALLBACK,
		.request    = req,
		.send_cb    = _mpc_osc_request_send_complete,
		.ep         = ep,
	};

	status = lcp_flush_nb(win->mngr, task, &params);
	if (LCP_PTR_IS_ERR(status))
	{
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

err:
	return rc;
}

bool mpc_lowcomm_atomic_dt_is_compatible(const mpc_lowcomm_datatype_t datatype)
{
	if (!mpc_lowcomm_datatype_is_common_predefined(datatype))
	{
		return false;
	}

	switch ((intptr_t)datatype)
	{
	case (intptr_t)MPC_LOWCOMM_FLOAT:
	case (intptr_t)MPC_LOWCOMM_REAL:
	case (intptr_t)MPC_LOWCOMM_REAL4:
	case (intptr_t)MPC_LOWCOMM_DOUBLE:
	case (intptr_t)MPC_LOWCOMM_LONG_DOUBLE:
	case (intptr_t)MPC_LOWCOMM_DOUBLE_PRECISION:
	case (intptr_t)MPC_LOWCOMM_REAL8:
	case (intptr_t)MPC_LOWCOMM_CHAR:
	case (intptr_t)MPC_LOWCOMM_SIGNED_CHAR:
	case (intptr_t)MPC_LOWCOMM_INT8_T:
	case (intptr_t)MPC_LOWCOMM_CHARACTER:
	case (intptr_t)MPC_LOWCOMM_SHORT:
	case (intptr_t)MPC_LOWCOMM_INT16_T:
	case (intptr_t)MPC_LOWCOMM_INTEGER2:
	case (intptr_t)MPC_LOWCOMM_INT:
	case (intptr_t)MPC_LOWCOMM_INT32_T:
	case (intptr_t)MPC_LOWCOMM_INTEGER:
	case (intptr_t)MPC_LOWCOMM_INTEGER4:
	case (intptr_t)MPC_LOWCOMM_LONG:
	case (intptr_t)MPC_LOWCOMM_LONG_LONG:
	case (intptr_t)MPC_LOWCOMM_LONG_LONG_INT:
	case (intptr_t)MPC_LOWCOMM_INT64_T:
	case (intptr_t)MPC_LOWCOMM_INTEGER8:
	case (intptr_t)MPC_LOWCOMM_UNSIGNED_CHAR:
	case (intptr_t)MPC_LOWCOMM_UINT8_T:
	case (intptr_t)MPC_LOWCOMM_UNSIGNED_SHORT:
	case (intptr_t)MPC_LOWCOMM_UINT16_T:
	case (intptr_t)MPC_LOWCOMM_UNSIGNED:
	case (intptr_t)MPC_LOWCOMM_UINT32_T:
	case (intptr_t)MPC_LOWCOMM_UNSIGNED_LONG:
	case (intptr_t)MPC_LOWCOMM_UNSIGNED_LONG_LONG:
	case (intptr_t)MPC_LOWCOMM_UINT64_T:
	case (intptr_t)MPC_LOWCOMM_AINT:
		return true;

	default:
		return false;
	}

	return false;
}

int mpc_lowcomm_iatomic(const void *const origin_buffer, void *fetch_buffer, const void *const compare_buffer,
                        const mpc_lowcomm_datatype_t datatype,
                        const int target, const intptr_t offset, const mpc_lowcomm_atomic_op_t op,
                        mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *req)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	lcp_status_ptr_t status;

	const uint64_t        remote_addr = win->rdata_win_info[target].addr + offset;
	lcp_task_h            task;
	lcp_mem_h             rmem;
	lcp_ep_h              ep;
	const lcp_atomic_dt_t lcp_dt = mpc2lcp_atomic_dt_table[(uint64_t)datatype];
	const lcp_atomic_op_t lcp_op = mpc2lcp_atomic_op_table[op];

	if (req != NULL && req != &MPC_REQUEST_NULL)
	{
		mpc_lowcomm_request_init(req, 1, datatype, NULL, 0);

		mpc_lowcomm_request_set_null(req, 0);
		req->request_type    = REQUEST_SEND;
		req->comm            = win->comm;
		req->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;
	}

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());
	_lowcomm_osc_get_target_ep(target, win, &ep);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	rc = _lowcomm_osc_get_remote_memory(task, target, remote_addr, win, &rmem);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	const uint32_t field_mask =
		(fetch_buffer != NULL ? LCP_REQUEST_REPLY_BUFFER : 0)                                    // Fetch ?
		| (req != &MPC_REQUEST_NULL ? LCP_REQUEST_USER_REQUEST | LCP_REQUEST_SEND_CALLBACK : 0); // Request to complete

	if (compare_buffer != NULL && fetch_buffer != NULL)
	{
		const int dt_size = lcp_atomic_dt_size[lcp_dt];
		memcpy(fetch_buffer, compare_buffer, dt_size);
	}

	lcp_request_param_t params =
	{
		.field_mask   = field_mask,
		.send_cb      = _mpc_osc_request_send_complete,
		.request      = req,
		.reply_buffer = fetch_buffer,
	};

	status = lcp_atomic_op_nb(ep, task, origin_buffer, lcp_dt, remote_addr, rmem, lcp_op, &params);
	if (LCP_PTR_IS_ERR(status))
	{
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}


err:
	return rc;
}
