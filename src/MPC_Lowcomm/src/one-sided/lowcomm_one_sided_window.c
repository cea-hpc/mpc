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

#include "mpc_lowcomm.h"
#include "mpc_lowcomm_one_sided.h"
#include "lowcomm_one_sided_internal.h"

#include <mpc_common_rank.h>
#include <mpc_common_progress.h>
#include <mpc_common_spinlock.h>

#include <sctk_alloc.h>

extern lcp_context_h         lcp_ctx_loc;
static int                   osc_is_initialized = 0;
static lcp_manager_h         osc_mngr           = NULL;
static mpc_common_spinlock_t osc_win_lock       = MPC_COMMON_SPINLOCK_INITIALIZER;

int mpc_osc_progress()
{
	return lcp_progress(osc_mngr);
}

int lowcomm_osc_init(void)
{
	lcp_task_h task;
	int        tid;
	int        rc = MPC_LOWCOMM_SUCCESS;

	osc_is_initialized = 1;
	tid  = mpc_common_get_task_rank();
	task = lcp_context_task_get(lcp_ctx_loc, tid);
	if (task == NULL)
	{
		mpc_common_debug_fatal("Unable to find the task");
	}

	mpc_common_spinlock_lock(&osc_win_lock);
	if (osc_mngr == NULL)
	{
		lcp_manager_param_t mngr_param =
		{
			.estimated_eps = mpc_common_get_task_count(),
			.flags         = LCP_MANAGER_OSC_MODEL,
			.field_mask    = LCP_MANAGER_ESTIMATED_EPS | LCP_MANAGER_COMM_MODEL,
		};
		rc = lcp_manager_create(lcp_ctx_loc, &osc_mngr, &mngr_param);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_spinlock_unlock(&osc_win_lock);
			goto err;
		}

		mpc_common_progress_register(mpc_osc_progress);
	}
	mpc_common_spinlock_unlock(&osc_win_lock);

	rc = lcp_task_associate(task, osc_mngr);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_error("MPI OSC: could not associate task on OSC manager. tid=%s, mngr_h=%p",
			tid, osc_mngr);
		goto err;
	}
err:
	return rc;
}

int lowcomm_osc_deinit(void)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	int        tid;
	lcp_task_h task;

	if (!osc_is_initialized)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	tid  = mpc_common_get_task_rank();
	task = lcp_context_task_get(lcp_ctx_loc, tid);

	rc = lcp_task_dissociate(task, osc_mngr);

	// NOTE: make sure all task have been dissociated before finishing the manager.
	mpc_lowcomm_termination_barrier();

	mpc_common_spinlock_lock(&osc_win_lock);
	if (osc_mngr != NULL)
	{
		mpc_common_progress_unregister(mpc_osc_progress);

		rc       = lcp_manager_fini(osc_mngr);
		osc_mngr = NULL;
	}
	mpc_common_spinlock_unlock(&osc_win_lock);
	return rc;
}

static int _win_setup(mpc_lowcomm_window_t *module, void **base, size_t size,
                      mpc_lowcomm_communicator_t comm, mpc_lowcomm_window_flag_t flavor)
{
	int            rc = MPC_LOWCOMM_SUCCESS;
	lcp_mem_h      w_mem_data = NULL, w_mem_state = NULL;
	lcp_mem_attr_t attr;
	void *         win_info = NULL;
	size_t         win_info_len;
	int            win_info_offset = 0;
	void *         key_data_buf = NULL, *key_state_buf = NULL;
	int            key_data_len, key_state_len;
	int            rkey_data_len, rkey_state_len;
	int *          win_info_lengths = NULL;
	int *          win_info_disps   = NULL;
	int            total_win_info_lengths;
	char *         win_info_buf = NULL;
	int            comm_size    = mpc_lowcomm_communicator_size(comm);
	const int      comm_rank    = mpc_lowcomm_communicator_rank_of(comm, mpc_common_get_task_rank());

	if (flavor == MPC_LOWCOMM_WINDOW_FLAG_SHARED)
	{
		rc = -1;
		goto err;
	}

	/* Register memory according to flavor. */
	if (flavor == MPC_LOWCOMM_WINDOW_FLAG_CREATE)
	{
		lcp_mem_param_t param =
		{
			.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD,
			.flags      = LCP_MEM_REGISTER_CREATE | LCP_MEM_REGISTER_STATIC,
			.address    = *base,
			.size       = size
		};

		rc = lcp_mem_provision(module->mngr, &w_mem_data, &param);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_error("MPI OSC: could not provision created data memory.");
			goto err;
		}
		module->lkey_data = w_mem_data;

		attr.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD;
		rc = lcp_mem_query(w_mem_data, &attr);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_error("MPI OSC: could not query memory attributes.");
			goto err;
		}

		assert(attr.size >= size && attr.address == *base);
		module->base_data = *base;
	}
	else if (flavor == MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE)
	{
		/* Memory is already allocated, just register it. */
		lcp_mem_param_t param =
		{
			.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD,
			.flags      = LCP_MEM_REGISTER_ALLOCATE | LCP_MEM_REGISTER_STATIC,
			.address    = NULL,
			.size       = size
		};

		rc = lcp_mem_provision(module->mngr, &w_mem_data, &param);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_error("MPI OSC: could not provision allocated data memory.");
			goto err;
		}
		module->lkey_data = w_mem_data;

		attr.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD;
		rc = lcp_mem_query(w_mem_data, &attr);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_error("MPI OSC: could not query memory attributes.");
			goto err;
		}

		assert(attr.size >= size);
		module->base_data = *base = attr.address;
	}
	else if (flavor == MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC)
	{
		// nothing to do.
	}
	else
	{
		mpc_common_debug_error("MPI OSC: wrong flavor.");
		goto err;
	}

	/* Allocate state for remote synchronization. */
	module->state = sctk_malloc(sizeof(mpc_osc_module_state_t));
	if (module->state == NULL)
	{
		mpc_common_debug_error("MPI OSC: could not allocate module state.");
		rc = MPC_LOWCOMM_ERROR;
		goto unmap_mem_data;
	}
	memset(module->state, 0, sizeof(mpc_osc_module_state_t));

	lcp_mem_param_t mem_state_param =
	{
		.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD,
		.flags      = LCP_MEM_REGISTER_CREATE | LCP_MEM_REGISTER_STATIC,
		.address    = module->state,
		.size       = sizeof(mpc_osc_module_state_t),
	};

	rc = lcp_mem_provision(module->mngr, &w_mem_state, &mem_state_param);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_error("MPI OSC: could not provision state memory.");
		goto unmap_mem_data;
	}
	attr.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD;
	rc = lcp_mem_query(w_mem_state, &attr);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_error("MPI OSC: could not query memory attributes.");
		goto err;
	}

	assert(attr.size >= sizeof(mpc_osc_module_state_t) && attr.address != NULL);
	module->base_state = attr.address;
	module->lkey_state = w_mem_state;

	/* Exchange the memory key with every other process.
	 * First, pack state memory key. */
	if (flavor == MPC_LOWCOMM_WINDOW_FLAG_CREATE || flavor == MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE)
	{
		rc = lcp_mem_pack(module->mngr, module->lkey_data, &key_data_buf, &key_data_len);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto unmap_mem_state;
		}
	}
	else
	{
		key_data_len = 0;
	}

	/* Second, pack state memory key. */
	rc = lcp_mem_pack(module->mngr, module->lkey_state, &key_state_buf, &key_state_len);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto release_key;
	}

	win_info_len  = 0;
	win_info_len += 2 * sizeof(void *);
	win_info_len += 2 * sizeof(int);
	win_info_len += key_data_len + key_state_len;
	win_info      = sctk_malloc(win_info_len);
	if (win_info == NULL)
	{
		mpc_common_debug_error("MPC OSC: could not allocate buffer for both memory keys.");
		rc = MPC_LOWCOMM_ERROR;
		goto release_key;
	}
	// FIXME: simplify with macro
	memcpy((char *)win_info,                   &module->base_data,  sizeof(void *));
	win_info_offset += sizeof(void *);
	memcpy((char *)win_info + win_info_offset, &module->base_state, sizeof(void *));
	win_info_offset += sizeof(void *);
	memcpy((char *)win_info + win_info_offset, &key_data_len,       sizeof(int));
	win_info_offset += sizeof(int);
	memcpy((char *)win_info + win_info_offset, &key_state_len,      sizeof(int));
	win_info_offset += sizeof(int);
	if (key_data_len > 0)
	{
		memcpy((char *)win_info + win_info_offset, key_data_buf, key_data_len);
		win_info_offset += key_data_len;
	}
	if (key_state_len > 0)
	{
		memcpy((char *)win_info + win_info_offset, key_state_buf, key_state_len);
	}

	/* First send size of local key to all others. */
	win_info_lengths = sctk_malloc(comm_size * sizeof(int));
	win_info_disps   = sctk_malloc(comm_size * sizeof(int));
	if (win_info_lengths == NULL || win_info_disps == NULL)
	{
		mpc_common_debug_error("WIN: could not allocate remote keys size table");
		goto release_key;
	}
	memset(win_info_lengths, 0, comm_size * sizeof(int));
	memset(win_info_disps,   0, comm_size * sizeof(int));

	mpc_lowcomm_allgather(&win_info_len, win_info_lengths, sizeof(int), comm);

	total_win_info_lengths = 0;
	for (int i = 0; i < comm_size; i++)
	{
		win_info_disps[i]       = total_win_info_lengths;
		total_win_info_lengths += win_info_lengths[i];
	}

	win_info_buf = sctk_malloc(total_win_info_lengths);
	if (win_info_buf == NULL)
	{
		mpc_common_debug_error("WIN: could not allocate memory for rkeys buffer.");
		goto release_key;
	}

	// Here we emulate an allgather_v
	mpc_lowcomm_request_t *recv_req = mpc_lowcomm_request_alloc();
	mpc_lowcomm_request_t *send_req = mpc_lowcomm_request_alloc();
	for (int i = 0; i < comm_size; i++)
	{
		if (comm_rank == i)
		{
			mpc_lowcomm_isend(0, win_info, win_info_len, -22, comm, send_req, MPC_LOWCOMM_SEND_STANDARD);
		}
		if (comm_rank == 0)
		{
			mpc_lowcomm_irecv(i, win_info_buf + win_info_disps[i], win_info_lengths[i], -22, comm, recv_req);
		}
		mpc_lowcomm_request_wait(send_req);
		mpc_lowcomm_request_wait(recv_req);
	}
	mpc_lowcomm_bcast(win_info_buf, total_win_info_lengths, 0, comm);

	mpc_lowcomm_request_free(send_req - 1);
	mpc_lowcomm_request_free(recv_req - 1);

	module->rdata_win_info  = sctk_malloc(comm_size * sizeof(mpc_osc_win_info_t));
	module->rstate_win_info = sctk_malloc(comm_size * sizeof(mpc_osc_win_info_t));
	if (module->rdata_win_info == NULL || module->rstate_win_info == NULL)
	{
		mpc_common_debug_error("OSC WIN: could not allocate memory for remote memory keys.");
		goto release_key;
	}

	win_info_offset = 0;
	for (int i = 0; i < comm_size; i++)
	{
		// FIXME: simplify with macro
		memcpy(&module->rdata_win_info[i].addr,  (char *)win_info_buf + win_info_disps[i],
			sizeof(void *));
		win_info_offset += sizeof(void *);
		memcpy(&module->rstate_win_info[i].addr, (char *)win_info_buf + win_info_disps[i] + win_info_offset,
			sizeof(void *));
		win_info_offset += sizeof(void *);
		memcpy(&rkey_data_len,                   (char *)win_info_buf + win_info_disps[i] + win_info_offset,
			sizeof(int));
		win_info_offset += sizeof(int);
		memcpy(&rkey_state_len,                  (char *)win_info_buf + win_info_disps[i] + win_info_offset,
			sizeof(int));
		win_info_offset += sizeof(int);

		module->rdata_win_info[i].rkey_init = 0;
		if (rkey_data_len > 0
		    && (flavor == MPC_LOWCOMM_WINDOW_FLAG_CREATE || flavor == MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE))
		{
			rc = lcp_mem_unpack(module->mngr, &module->rdata_win_info[i].rkey,
				(void *)((char *)win_info_buf + win_info_disps[i] + win_info_offset),
				rkey_data_len);
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				mpc_common_debug_error("OSC WIN: could not unpack memory for remote memory keys.");
				goto release_key;
			}
			win_info_offset += rkey_data_len;
			module->rdata_win_info[i].rkey_init = 1;
		}

		rc = lcp_mem_unpack(module->mngr, &module->rstate_win_info[i].rkey,
			(void *)((char *)win_info_buf + win_info_disps[i] + win_info_offset),
			rkey_state_len);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto release_key;
		}
		module->rstate_win_info[i].rkey_init = 1;
		win_info_offset = 0;
	}

	/* Finally, synchronize with everyone. */
	mpc_lowcomm_barrier(comm);

	if (flavor == MPC_LOWCOMM_WINDOW_FLAG_CREATE || flavor == MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE)
	{
		lcp_mem_release_rkey_buf(key_data_buf);
	}
	lcp_mem_release_rkey_buf(key_state_buf);

	mpc_common_debug("MPI OSC: win setup. state=%p", module->base_state);

	sctk_free(win_info_lengths);
	sctk_free(win_info_buf);
	sctk_free(win_info_disps);

	return rc;

release_key:
	if (win_info_buf)
	{
		sctk_free(win_info_buf);
	}
	if (win_info_disps)
	{
		sctk_free(win_info_disps);
	}
	if (win_info_lengths)
	{
		sctk_free(win_info_lengths);
	}
	if (key_data_buf)
	{
		lcp_mem_release_rkey_buf(key_data_buf);
	}
	if (key_state_buf)
	{
		lcp_mem_release_rkey_buf(key_state_buf);
	}
unmap_mem_state:
	lcp_mem_deprovision(module->mngr, w_mem_state);
unmap_mem_data:
	if (module->state)
	{
		sctk_free(module->state);
	}
	lcp_mem_deprovision(module->mngr, w_mem_data);
err:
	return rc;
}

static int _win_init(mpc_lowcomm_window_t **win_p, const mpc_lowcomm_window_flag_t flavor,
                     mpc_lowcomm_communicator_t comm, void **base, size_t size)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	mpc_lowcomm_window_t *win;

	/* Allocate the window and initialize fields. */
	win = sctk_malloc(sizeof(mpc_lowcomm_window_t));
	if (win == NULL)
	{
		rc = -1;
		goto err;
	}
	memset(win, 0, sizeof(mpc_lowcomm_window_t));


	int comm_size = mpc_lowcomm_communicator_size(comm);
	win->ctx  = lcp_ctx_loc;
	win->mngr = osc_mngr;

	// win->flavor = flavor;
	// win->model  = MPI_WIN_SEPARATE;
	// win->size   = size;
	// win->info   = info;
	// win->group = mpc_lowcomm_communicator_group(comm);
	// strcpy(win->win_name, "MPI_WIN_LCP");
	// mpc_common_hashtable_init(&win->attrs, 16);
	win->win_flag             = flavor;
	win->lock_all_is_no_check = -1;

	win->comm = mpc_lowcomm_communicator_dup(comm);
	win->eps  = sctk_malloc(comm_size * sizeof(lcp_ep_h));
	if (win->eps == NULL)
	{
		mpc_common_debug_error("WIN: could not allocate window endpoint table.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	memset(win->eps, 0, comm_size * sizeof(lcp_ep_h));

	mpc_common_hashtable_init(&win->outstanding_locks, 1024);
	mpc_list_init_head(&win->pending_posts);

	/* Exchange windows information with other processes. */
	rc = _win_setup(win, base, size, comm, flavor);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	*win_p = win;

	return rc;

err:
	mpc_common_debug_error("MPI OSC: could not initialize window.");
	return rc;
}

int mpc_lowcomm_window_create(void **buffer, const size_t size,
                              mpc_lowcomm_communicator_t comm, mpc_lowcomm_window_flag_t flag,
                              mpc_lowcomm_window_t **window_p)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	mpc_lowcomm_window_t *win;

	switch (flag)
	{
	case MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC:
		rc = _win_init(&win, flag, comm, NULL, 0);
		break;

	case MPC_LOWCOMM_WINDOW_FLAG_CREATE:
		rc = _win_init(&win, flag, comm, buffer, size);
		break;

	case MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE:
		rc = _win_init(&win, flag, comm, buffer, size);
		break;

	case MPC_LOWCOMM_WINDOW_FLAG_SHARED:
		rc = MPC_LOWCOMM_ERROR;
		break;
	}
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		return rc;
	}

	*window_p = win;

	return rc;
}

int mpc_lowcomm_window_free(mpc_lowcomm_window_t **win_p)
{
	int rc = MPC_LOWCOMM_SUCCESS, i;
	mpc_lowcomm_window_t *win  = *win_p;
	lcp_task_h            task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());
	int comm_size = mpc_lowcomm_communicator_size(win->comm);


	rc = _lowcomm_osc_perform_flush(win, task, NULL, NULL);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	mpc_lowcomm_barrier(win->comm);


	/* Deregister local key */
	if (win->win_flag == MPC_LOWCOMM_WINDOW_FLAG_CREATE || win->win_flag == MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE)
	{
		rc             = lcp_mem_deprovision(win->mngr, win->lkey_data);
		win->lkey_data = NULL;
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			mpc_common_debug_error("WIN: could not deregister local data memory.");
			goto err;
		}

		for (i = 0; i < comm_size; i++)
		{
			rc = lcp_mem_deprovision(win->mngr, win->rdata_win_info[i].rkey);
			win->rdata_win_info[i].rkey = NULL;
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}
		}
	}

	/* Detach all attached memory. */
	for (i = 0; i < (int)win->state->dynamic_win_count; i++)
	{
		rc = lcp_mem_deprovision(win->mngr, win->local_dynamic_win_infos[i].memh);
		win->local_dynamic_win_infos[i].memh = NULL;
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}
	}
	win->state->dynamic_win_count = 0;

	rc = lcp_mem_deprovision(win->mngr, win->lkey_state);
	win->lkey_state = NULL;
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_debug_error("WIN: could not deregister local state memory.");
		goto err;
	}

	mpc_common_hashtable_release(&win->outstanding_locks);

	sctk_free(win->rdata_win_info);
	sctk_free(win->rstate_win_info);

	*win_p = NULL;

err:
	return rc;
}

int mpc_lowcomm_window_attach(mpc_lowcomm_window_t *win, void *base, const size_t len)
{
	assert(win->win_flag == MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC);
	int            rc         = MPC_LOWCOMM_SUCCESS;
	int            my_rank    = mpc_lowcomm_communicator_rank_fast(win->comm);
	int            insert_idx = 0;
	int            found_idx;
	int            memkey_length;
	void *         memkey_buf;
	lcp_task_h     task;
	lcp_ep_h       ep;
	lcp_mem_attr_t attr;

	if (win->state->dynamic_win_count >= OSC_DYNAMIC_WIN_ATTACH_MAX)
	{
		return MPC_LOWCOMM_ERROR;
	}

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());
	rc   = _lowcomm_osc_get_target_ep(win, my_rank, &ep);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	rc = start_exclusive_lock(win, task, OSC_STATE_REGION_LOCK_OFFSET, my_rank);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}
	if (win->state->dynamic_win_count > 0)
	{
		found_idx = _lowcomm_osc_find_attached_region_position((mpc_osc_dynamic_win_t *)win->state->dynamic_wins, 0,
			win->state->dynamic_win_count - 1,
			(uint64_t)base, len, &insert_idx);

		if (found_idx != -1)
		{
			win->local_dynamic_win_infos[found_idx].refcnt++;
			rc = end_exclusive_lock(win, task, OSC_STATE_REGION_LOCK_OFFSET, my_rank);
			return rc;
		}

		assert(insert_idx >= 0 && insert_idx <= (int)win->state->dynamic_win_count);

		memmove((void *)&win->local_dynamic_win_infos[insert_idx + 1],
			(void *)&win->local_dynamic_win_infos[insert_idx],
			(OSC_DYNAMIC_WIN_ATTACH_MAX - (insert_idx + 1)) * sizeof(mpc_osc_local_dynamic_win_info_t));
		memmove((void *)&win->state->dynamic_wins[insert_idx + 1],
			(void *)&win->state->dynamic_wins[insert_idx],
			(OSC_DYNAMIC_WIN_ATTACH_MAX - (insert_idx + 1)) * sizeof(mpc_osc_dynamic_win_t));
	}
	else
	{
		insert_idx = 0;
	}

	lcp_mem_param_t param =
	{
		.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD,
		.flags      = LCP_MEM_REGISTER_CREATE | LCP_MEM_REGISTER_STATIC,
		.address    = base,
		.size       = len
	};

	rc = lcp_mem_provision(win->mngr,
		&(win->local_dynamic_win_infos[insert_idx].memh),
		&param);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock_dyn;
	}

	attr.field_mask = LCP_MEM_ADDR_FIELD | LCP_MEM_SIZE_FIELD;
	rc = lcp_mem_query(win->local_dynamic_win_infos[insert_idx].memh,
		&attr);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock_dyn;
	}

	assert(attr.size >= len && attr.address == base);

	win->state->dynamic_wins[insert_idx].base = (uint64_t)base;
	win->state->dynamic_wins[insert_idx].size = len;

	rc = lcp_mem_pack(win->mngr,
		win->local_dynamic_win_infos[insert_idx].memh,
		&memkey_buf,
		&memkey_length);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto unlock_dyn;
	}

	assert(memkey_length <= OCS_LCP_RKEY_BUF_SIZE_MAX);

	win->state->dynamic_wins[insert_idx].rkey_len = memkey_length;
	memcpy((void *)win->state->dynamic_wins[insert_idx].rkey_buf,
		memkey_buf, memkey_length);

	win->local_dynamic_win_infos[insert_idx].refcnt++;
	win->state->dynamic_win_count++;

	lcp_mem_release_rkey_buf(memkey_buf);

	mpc_common_debug("MPI OSC: attached memory. base=%p, dyn win count=%d",
		base, win->state->dynamic_win_count);

unlock_dyn:
	rc = end_exclusive_lock(win, task, OSC_STATE_REGION_LOCK_OFFSET, my_rank);
err:
	return rc;
}

int mpc_lowcomm_window_detach(mpc_lowcomm_window_t *win, const void *base)
{
	assert(win->win_flag == MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC);
	int        rc      = MPC_LOWCOMM_SUCCESS;
	int        my_rank = mpc_lowcomm_communicator_rank_fast(win->comm);
	int        region_idx, insert_idx;
	lcp_task_h task;

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());

	rc = start_exclusive_lock(win, task, OSC_STATE_REGION_LOCK_OFFSET, my_rank);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}
	region_idx =
		_lowcomm_osc_find_attached_region_position((mpc_osc_dynamic_win_t *)win->state->dynamic_wins,
			0, win->state->dynamic_win_count - 1, (uint64_t)base, 1, &insert_idx);

	if (region_idx == -1)
	{
		rc = end_exclusive_lock(win, task, OSC_STATE_REGION_LOCK_OFFSET, my_rank);
		return rc;
	}

	win->local_dynamic_win_infos[region_idx].refcnt--;
	if (win->local_dynamic_win_infos[region_idx].refcnt == 0)
	{
		lcp_mem_deprovision(win->mngr, win->local_dynamic_win_infos[region_idx].memh);

		memmove((void *)&win->local_dynamic_win_infos[region_idx],
			(void *)&win->local_dynamic_win_infos[region_idx + 1],
			(OSC_DYNAMIC_WIN_ATTACH_MAX - (region_idx + 1)) * sizeof(mpc_osc_local_dynamic_win_info_t));
		memmove((void *)&win->state->dynamic_wins[region_idx],
			(void *)&win->state->dynamic_wins[region_idx + 1],
			(OSC_DYNAMIC_WIN_ATTACH_MAX - (region_idx + 1)) * sizeof(mpc_osc_dynamic_win_t));

		win->state->dynamic_win_count--;
		mpc_common_debug("MPI OSC: dyn win count. count=%d",
			win->state->dynamic_win_count);
	}
	rc = end_exclusive_lock(win, task, OSC_STATE_REGION_LOCK_OFFSET, my_rank);

err:
	return rc;
}
