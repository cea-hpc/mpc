#ifndef MPC_LOWCOMM_LCP_H
#define MPC_LOWCOMM_LCP_H
#include <lcp.h>
#include <comm.h>

#include <mpc_arch.h>
#include <mpc_common_helper.h>
#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>
#include <uthash.h>
#include <utlist.h>
#include <string.h>
#include <mpc_common_asm.h>
#include <checksum.h>
#include <reorder.h>
#include <mpc_launch.h>
#include <mpc_common_profiler.h>
#include <sctk_alloc.h>
#include <coll.h>
#include <mpc_common_flags.h>
#include <mpc_topology.h>



#include "mpc_lowcomm_types.h"
#include <mpc_lowcomm_datatypes.h>
#include <mpc_common_progress.h>


#include "lowcomm_request.h"
#include <mpc_common_rank.h>

lcp_context_h lcp_ctx_loc  = NULL;
lcp_manager_h lcp_mngr_loc = NULL;

/************
 * MESSAGES *
 ************/
void *mpc_lowcomm_request_alloc()
{
	mpc_lowcomm_request_t *request = NULL;
	int        tid = mpc_common_get_task_rank();
	lcp_task_h task;

	task = lcp_context_task_get(lcp_ctx_loc, tid);
	if (task == NULL)
	{
		mpc_common_debug_fatal("COMM: Could not find task with tid %d.", tid);
	}

	request = (mpc_lowcomm_request_t *)lcp_request_alloc(task);
	if (request == NULL)
	{
		return request;
	}
	request->flags        = 0;
	request->is_allocated = MPC_LOWCOMM_REQUEST_ALLOC;

	return request + 1;
}

void mpc_lowcomm_set_packed_buf(mpc_lowcomm_request_t *request, void *buf,
                                size_t packed_size)
{
	request->flags      |= MPC_LOWCOMM_REQUEST_PACKED;
	request->packed_buf  = buf;
	request->packed_size = packed_size;
}

// FIXME: this function needs to be revised. Is she really useful?
int mpc_lowcomm_request_complete(mpc_lowcomm_request_t *request)
{
	mpc_lowcomm_request_set_null(request, 1);
	return MPC_LOWCOMM_SUCCESS;
}

void mpc_lowcomm_request_free(mpc_lowcomm_request_t *req)
{
	if (req->flags & MPC_LOWCOMM_REQUEST_PACKED)
	{
		sctk_free(req->packed_buf);
	}
	lcp_request_free(req);
}

static inline void _mpc_lowcomm_build_send_tag_info(lcp_tag_info_t *tag_info,
                                                    mpc_lowcomm_communicator_t comm,
                                                    int dest, int tag,
                                                    mpc_lowcomm_request_t *req)
{
	tag_info->tag     = tag;
	tag_info->src_uid = mpc_lowcomm_monitor_get_uid();
	tag_info->comm_id = (uint16_t)mpc_lowcomm_communicator_id(comm);

	if (mpc_lowcomm_communicator_is_intercomm(comm))
	{
		tag_info->src      = mpc_lowcomm_communicator_rank(comm);
		tag_info->dest_uid = mpc_lowcomm_communicator_remote_uid(comm, dest);
		tag_info->dest     = mpc_lowcomm_communicator_remote_world_rank(comm, dest);
	}
	else
	{
		tag_info->src      = mpc_lowcomm_communicator_rank_fast(comm);
		tag_info->dest_uid = mpc_lowcomm_communicator_uid(comm, dest);
		tag_info->dest     = mpc_lowcomm_communicator_world_rank_of(comm, dest);
	}

	// FIXME: see fixme below.
	req->recv_info.src  = tag_info->src;
	req->recv_info.dest = tag_info->dest;
	req->recv_info.tag  = tag;
}

int mpc_lowcomm_isend(int dest, const void *data, size_t size, int tag,
                      mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req,
                      mpc_lowcomm_sendmode_t mode)
{
	lcp_tag_info_t   tag_info;
	lcp_ep_h         ep;
	lcp_task_h       task;
	lcp_status_ptr_t request;

	/* Initialize needed lowcomm request fields. */
	mpc_lowcomm_request_set_null(req, 0);
	req->request_type    = REQUEST_SEND;
	req->comm            = comm;
	req->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;

	/* Initialize tag information based on comm: src, src_uid, tag,... */
	_mpc_lowcomm_build_send_tag_info(&tag_info, comm, dest, tag, req);

	/* Try to get cached endpoint. */
	ep = mpc_lowcomm_communicator_lookup(comm, dest);
	if (ep == NULL)
	{
		int rc = lcp_ep_get_or_create(lcp_mngr_loc, tag_info.dest_uid,
			&ep, 0);
		if (rc != 0)
		{
			mpc_common_debug_fatal("Could not create endpoint.");
		}
		/* Add endpoint to communicator cache. */
		mpc_lowcomm_communicator_add_ep(comm, dest, ep);
	}

	/* Get MPI rank data store. */
	task = lcp_context_task_get(lcp_ctx_loc, mpc_common_get_task_rank());
	if (task == NULL)
	{
		mpc_common_debug_fatal("LCP: Could not find task with tid %d.", tag_info.src);
	}

	/* Fill up request parameters. */
	lcp_request_param_t param =
	{
		.field_mask = (mode == MPC_LOWCOMM_SEND_SYNCHRONIZED ? LCP_REQUEST_TAG_SYNC : 0)
		              | LCP_REQUEST_SEND_CALLBACK,
		// NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
		.datatype = req->dt_magic == (int)0xDDDDDDDD ? LCP_DATATYPE_DERIVED : LCP_DATATYPE_CONTIGUOUS,
		.request  = req,
		.send_cb  = _mpc_lowcomm_request_send_complete,
	};

	/* Request has been allocated through mpc_lowcomm_request_alloc. */
	if (req->is_allocated == MPC_LOWCOMM_REQUEST_ALLOC)
	{
		param.field_mask |= LCP_REQUEST_USER_REQUEST;
	}
	else
	{
		req->flags = 0;
	}

	request = lcp_tag_send_nb(ep, task, data, size, &tag_info, &param);

	if (LCP_PTR_IS_ERR(request))
	{
		return MPC_LOWCOMM_ERROR;
	}
	else
	{
		return MPC_LOWCOMM_SUCCESS;
	}
}

static int _mpc_lowcomm_request_recv_complete(int status, void *request,
                                              lcp_tag_recv_info_t *tag_info)
{
	mpc_lowcomm_request_t *req = (mpc_lowcomm_request_t *)request;

	assert(status == MPC_LOWCOMM_SUCCESS);

	req->recv_info.length = tag_info->length;
	req->recv_info.src    = tag_info->src;
	req->recv_info.dest   = tag_info->dest;
	req->recv_info.tag    = tag_info->tag;
	req->status_error     = status;

	req->header.destination_task = tag_info->dest;

	if (req->flags & MPC_LOWCOMM_REQUEST_CALLBACK)
	{
		req->request_complete(req);
	}

	req->completion_flag = MPC_LOWCOMM_MESSAGE_DONE;

	return MPC_LOWCOMM_SUCCESS;
}

static inline void _mpc_lowcomm_build_recv_tag_info(lcp_tag_info_t *tag_info,
                                                    mpc_lowcomm_communicator_t comm,
                                                    int src, int tag)
{
	tag_info->tag     = tag;
	tag_info->src     = src;
	tag_info->dest    = mpc_common_get_task_rank();
	tag_info->comm_id = (uint16_t)mpc_lowcomm_communicator_id(comm);
}

int mpc_lowcomm_irecv(int src, void *data, size_t size, int tag,
                      mpc_lowcomm_communicator_t comm, mpc_lowcomm_request_t *req)
{
	lcp_status_ptr_t request;
	lcp_tag_info_t   tag_info;
	lcp_task_h       task;

	/* Initialize needed lowcomm request fields. */
	mpc_lowcomm_request_set_null(req, 0);
	req->request_type    = REQUEST_RECV;
	req->comm            = comm;
	req->completion_flag = MPC_LOWCOMM_MESSAGE_PENDING;

	/* Initialize tag information based on comm: src, src_uid, tag,... */
	_mpc_lowcomm_build_recv_tag_info(&tag_info, comm, src, tag);

	task = lcp_context_task_get(lcp_ctx_loc, tag_info.dest);
	if (task == NULL)
	{
		mpc_common_debug_fatal("LCP: Could not find task with tid %d.", tag_info.src);
	}

	/* Fill up request parameters. */
	lcp_request_param_t param =
	{
		.field_mask = LCP_REQUEST_RECV_CALLBACK,
		// NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
		.datatype = req->dt_magic == (int)0xDDDDDDDD ? LCP_DATATYPE_DERIVED : LCP_DATATYPE_CONTIGUOUS,
		.request  = req,
		.recv_cb  = _mpc_lowcomm_request_recv_complete,
	};
	if (req->is_allocated == MPC_LOWCOMM_REQUEST_ALLOC)
	{
		param.field_mask |= LCP_REQUEST_USER_REQUEST;
	}
	else
	{
		req->flags = 0;
	}

	request = lcp_tag_recv_nb(lcp_mngr_loc, task, data, size, &tag_info,
		src == MPC_ANY_SOURCE ? 0 : ~0,
		tag == MPC_ANY_TAG ? 0 : ~0,
		&param);

	if (LCP_PTR_IS_ERR(request))
	{
		return MPC_LOWCOMM_ERROR;
	}
	else
	{
		return MPC_LOWCOMM_SUCCESS;
	}
}

int mpc_lowcomm_iprobe_src_dest(const int world_source, const int world_destination, const int tag,
                                const mpc_lowcomm_communicator_t comm, int *flag, mpc_lowcomm_status_t *status)
{
	// FIXME: move code to mpc_lowcomm_iprobe
	int                           rc          = MPC_LOWCOMM_SUCCESS;
	lcp_task_h                    task        = NULL;
	lcp_tag_recv_info_t           recv_info   = { 0 };
	mpc_lowcomm_status_t          status_init = MPC_LOWCOMM_STATUS_INIT;
	int                           has_status  = 1;
	mpc_lowcomm_communicator_id_t comm_id     = mpc_lowcomm_communicator_id(comm);

	if (status == MPC_LOWCOMM_STATUS_NULL)
	{
		has_status = 0;
	}
	else
	{
		*status = status_init;
	}

	task = lcp_context_task_get(lcp_ctx_loc, world_destination);
	if (task == NULL)
	{
		mpc_common_debug_fatal("LCP: Could not find task with tid %d.", world_destination);
	}

	rc = lcp_tag_probe_nb(lcp_mngr_loc, task, world_source, tag,
		comm_id, &recv_info);

	// FIXME: should it be before probe?
	/* Progress communications to check if event has been raised. */
	mpc_common_progress();

	if ((*flag = recv_info.found))
	{
		if (has_status)
		{
			status->MPC_SOURCE = recv_info.src;
			status->MPC_TAG    = recv_info.tag;
			status->size       = recv_info.length;
			status->MPC_ERROR  = MPC_LOWCOMM_SUCCESS;
		}

		return rc;
	}

	return rc;
}

int mpc_lowcomm_iprobe(int source, int tag, mpc_lowcomm_communicator_t comm, int *flag, mpc_lowcomm_status_t *status)
{
	/* Translate ranks */

	if (source != MPC_ANY_SOURCE)
	{
		if (mpc_lowcomm_communicator_is_intercomm(comm))
		{
			source = mpc_lowcomm_communicator_remote_world_rank(comm, source);
		}
		else
		{
			source = mpc_lowcomm_communicator_world_rank_of(comm, source);
		}
	}
	else
	{
		source = MPC_ANY_SOURCE;
	}

	return mpc_lowcomm_iprobe_src_dest(source, mpc_common_get_task_rank(), tag, comm, flag, status);
}

#endif
