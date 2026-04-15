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

#include "lowcomm_one_sided_internal.h"

#include <mpc_common_rank.h>
#include <mpc_common_progress.h>
#include <sctk_alloc.h>

/***********************************************/
/*************** Passive Target ****************/
/***********************************************/

static inline int need_atomic_lock(mpc_lowcomm_window_t *module, int target)
{
	mpc_osc_lock_t *lock = mpc_common_hashtable_get(&(module->outstanding_locks), target);

	return !(lock != NULL && (lock->type == MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE && !lock->is_no_check));
}

int start_exclusive_lock(mpc_lowcomm_window_t *win, lcp_task_h task, uint64_t lock_offset, int target)
{
	int      rc         = MPC_LOWCOMM_SUCCESS;
	uint64_t lock_state = OSC_LOCK_UNLOCK;
	uint64_t base_rstate;

	do
	{
		base_rstate = (uint64_t)win->rstate_win_info[target].addr;
		lcp_ep_h ep;
		_lowcomm_osc_get_target_ep(win, target, &ep);

		rc = _lowcomm_osc_perform_atomic(win, ep,
			task, OSC_LOCK_EXCLUSIVE,
			LCP_ATOMIC_DT_UINT64, &lock_state,
			base_rstate + lock_offset,
			win->rstate_win_info[target].rkey,
			LCP_ATOMIC_OP_CSWAP);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}

		if (lock_state != 0)
		{
			/* Reset lock value. */
			lock_state = OSC_LOCK_UNLOCK;
			rc         = mpc_common_progress();
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}
			// FIXME: enable thread progression sharing with mpc_lowcomm_perform_idle.
			mpc_thread_yield();
			continue;
		}

		mpc_common_nodebug("MPI OSC: start exclusive. to=%d", target);

		break;
	} while (1);

err:
	return rc;
}

int end_exclusive_lock(mpc_lowcomm_window_t *win, lcp_task_h task, uint64_t lock_offset, int target)
{
	int      rc          = MPC_LOWCOMM_SUCCESS;
	uint64_t value       = -OSC_LOCK_EXCLUSIVE;
	uint64_t base_rstate = (uint64_t)win->rstate_win_info[target].addr;

	lcp_ep_h ep;

	_lowcomm_osc_get_target_ep(win, target, &ep);
	mpc_common_nodebug("MPI OSC: end exclusive. to=%d", target);
	rc = _lowcomm_osc_perform_atomic(win, ep, task, value,
		LCP_ATOMIC_DT_UINT64, NULL, base_rstate
		+ lock_offset,
		win->rstate_win_info[target].rkey,
		LCP_ATOMIC_OP_ADD);
	return rc;
}

static int start_shared_lock(mpc_lowcomm_window_t *win, lcp_task_h task, int target)
{
	int      rc = MPC_LOWCOMM_SUCCESS;
	uint64_t lock_state;
	uint64_t base_rstate;

	lcp_ep_h ep;

	_lowcomm_osc_get_target_ep(win, target, &ep);

	do
	{
		base_rstate = (uint64_t)win->rstate_win_info[target].addr;
		rc          = _lowcomm_osc_perform_atomic(win, ep, task,
			1, LCP_ATOMIC_DT_UINT64,
			&lock_state, base_rstate
			+ OSC_STATE_GLOBAL_LOCK_OFFSET,
			win->rstate_win_info[target].rkey,
			LCP_ATOMIC_OP_ADD);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}

		if (lock_state >= OSC_LOCK_EXCLUSIVE)
		{
			/* back off. */
			rc = _lowcomm_osc_perform_atomic(win,
				ep,
				task, -1,
				LCP_ATOMIC_DT_UINT64, NULL,
				base_rstate
				+ OSC_STATE_GLOBAL_LOCK_OFFSET,
				win->rstate_win_info[target].rkey,
				LCP_ATOMIC_OP_ADD);
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}
		}

		break;
	} while (1);

err:
	return rc;
}

static int end_shared_lock(mpc_lowcomm_window_t *win, lcp_task_h task, int target)
{
	int      rc          = MPC_LOWCOMM_SUCCESS;
	uint64_t base_rstate = (uint64_t)win->rstate_win_info[target].addr;

	lcp_ep_h ep;

	_lowcomm_osc_get_target_ep(win, target, &ep);

	rc = _lowcomm_osc_perform_atomic(win, ep, task,
		-1, LCP_ATOMIC_DT_UINT64, NULL,
		base_rstate
		+ OSC_STATE_GLOBAL_LOCK_OFFSET,
		win->rstate_win_info[target].rkey,
		LCP_ATOMIC_OP_ADD);

	return rc;
}

static int start_atomic_lock(mpc_lowcomm_window_t *module, lcp_ep_h ep,
                             lcp_task_h task, int target)
{
	int      rc               = MPC_LOWCOMM_OSC_ALREADY_LOCKED;
	uint64_t lock_state       = 0;
	uint64_t value            = 1;
	size_t   remote_lock_addr = module->rstate_win_info[target].addr
	                            + OSC_STATE_ACC_LOCK_OFFSET;

	if (need_atomic_lock(module, target))
	{
		// FIXME: should this be added to a perform_idle type of function to
		//       avoid active waiting.
		do
		{
			rc = _lowcomm_osc_perform_atomic(module, ep, task, value,
				LCP_ATOMIC_DT_UINT64, &lock_state, remote_lock_addr,
				module->rstate_win_info[target].rkey, LCP_ATOMIC_OP_CSWAP);
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}

			if (lock_state == 0)
			{
				rc = MPC_LOWCOMM_SUCCESS;
				break;
			}

			/* Reset compare value. */
			lock_state = 0;

			rc = mpc_common_progress();
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}
			mpc_thread_yield();
		} while (1);
	}

	mpc_common_nodebug("MPI OSC: acc atomic lock. from=%d, to=%d, remote lock addr=%p",
		mpc_common_get_task_rank(), target, remote_lock_addr);

err:
	return rc;
}

static int end_atomic_lock(mpc_lowcomm_window_t *win, lcp_ep_h ep,
                           lcp_task_h task, int target)
{
	int      rc               = MPC_LOWCOMM_SUCCESS;
	uint64_t value            = 0;
	size_t   remote_lock_addr = win->rstate_win_info[target].addr
	                            + OSC_STATE_ACC_LOCK_OFFSET;
	uint64_t lock_state = 1;

	mpc_common_nodebug("MPI OSC:  unlock. remote lock addr=%p",
		remote_lock_addr);
	assert(ep);
	if (need_atomic_lock(win, target))
	{
		rc = _lowcomm_osc_perform_atomic(win, ep, task, value,
			LCP_ATOMIC_DT_UINT64, &lock_state, remote_lock_addr,
			win->rstate_win_info[target].rkey, LCP_ATOMIC_OP_CSWAP);
		if (lock_state != 1)
		{
			mpc_common_debug_fatal("failed with lock_state = %d", lock_state);
		}
		assert(lock_state == 1);
	}

	mpc_common_nodebug("MPI OSC: acc atomic unlock. from=%d, to=%d, remote lock addr=%p",
		mpc_common_get_task_rank(), target, remote_lock_addr);

	return rc;
}

int mpc_lowcomm_osc_lock(const mpc_lowcomm_osc_lock_type_t lock_type, mpc_lowcomm_window_t *win,
                         const int target_rank, const bool is_no_check)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	lcp_ep_h   ep;
	lcp_task_h task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());

	rc = _lowcomm_osc_get_target_ep(win, target_rank, &ep);


	mpc_osc_lock_t *lock = NULL;
	if (lock_type == MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE || lock_type == MPC_LOWCOMM_WINDOW_LOCK_SHARED)
	{
		// The target is already locked
		lock = mpc_common_hashtable_get(&win->outstanding_locks, target_rank);
		if (lock != NULL)
		{
			return MPC_LOWCOMM_OSC_ALREADY_LOCKED;
		}

		lock = sctk_malloc(sizeof(mpc_osc_lock_t));
		if (lock == NULL)
		{
			mpc_common_debug_fatal("MPC OSC: could not allocate new lock.");
		}
		lock->target = target_rank;
	}

	if (is_no_check == false)
	{
		switch (lock_type)
		{
		case MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE:
			lock->type = MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE;
			rc         = start_exclusive_lock(win, task, OSC_STATE_GLOBAL_LOCK_OFFSET, target_rank);
			break;

		case MPC_LOWCOMM_WINDOW_LOCK_SHARED:
			lock->type = MPC_LOWCOMM_WINDOW_LOCK_SHARED;
			rc         = start_shared_lock(win, task, target_rank);
			break;

		case MPC_LOWCOMM_WINDOW_LOCK_ATOMIC:
			rc = start_atomic_lock(win, ep, task, target_rank);
			goto end;
			break;

		default:
			mpc_common_debug_fatal("Unknown lock_type %d", lock_type);
			break;
		}
		lock->is_no_check = 0;
	}
	else
	{
		if (lock != NULL)
		{
			lock->is_no_check = 1;
		}
	}

	if (rc == MPC_LOWCOMM_SUCCESS)
	{
		mpc_common_nodebug("setting lock for target_rank %d, lock_type = %d", target_rank, lock->type);
		mpc_common_hashtable_set(&win->outstanding_locks, target_rank, lock);
	}
	else
	{
		sctk_free(lock);
	}

end:
	return rc;
}

int mpc_lowcomm_osc_unlock(mpc_lowcomm_window_t *win, const int target_rank, const bool is_atomic)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	lcp_ep_h   ep;
	lcp_task_h task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());

	rc = _lowcomm_osc_get_target_ep(win, target_rank, &ep);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto end;
	}

	rc = _lowcomm_osc_perform_flush(win, task, ep, NULL);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto end;
	}

	if ((is_atomic))
	{
		rc = end_atomic_lock(win, ep, task, target_rank);
		goto end;
	}

	mpc_osc_lock_t *lock;
	lock = mpc_common_hashtable_get(&win->outstanding_locks, target_rank);
	// Check the lock is still active
	if (lock == NULL)
	{
		return MPC_LOWCOMM_ERROR;
	}
	mpc_common_hashtable_delete(&win->outstanding_locks, target_rank);

	if (lock->is_no_check == 0)
	{
		switch (lock->type)
		{
		case MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE:
			rc = end_exclusive_lock(win, task, OSC_STATE_GLOBAL_LOCK_OFFSET, target_rank);
			break;

		case MPC_LOWCOMM_WINDOW_LOCK_SHARED:
			rc = end_shared_lock(win, task, target_rank);
			break;

		case MPC_LOWCOMM_WINDOW_LOCK_ATOMIC:
			rc = end_atomic_lock(win, ep, task, target_rank);
			break;

		default:
			mpc_common_debug_fatal("Unknown lock_type %d", lock->type);
			break;
		}
	}
	sctk_free(lock);

end:
	return rc;
}

int mpc_lowcomm_osc_lock_all(mpc_lowcomm_window_t *win, const bool is_no_check)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	int        i, j;
	lcp_task_h task;

	const int comm_size = mpc_lowcomm_communicator_size(win->comm);

	if (is_no_check == false)
	{
		task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());
		for (i = 0; i < comm_size; i++)
		{
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}
			rc = start_shared_lock(win, task, i);
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				for (j = 0; j < i; j++)
				{
					end_shared_lock(win, task, j);
				}
				return rc;
			}
		}
		win->lock_all_is_no_check = 0;
	}
	else
	{
		win->lock_all_is_no_check = 1;
	}

err:
	return rc;
}

int mpc_lowcomm_osc_unlock_all(mpc_lowcomm_window_t *win)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	int        i;
	lcp_task_h task = NULL;

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());
	const int comm_size = mpc_lowcomm_communicator_size(win->comm);

	rc = _lowcomm_osc_perform_flush(win, task, NULL, NULL);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	if (win->lock_all_is_no_check == 0)
	{
		for (i = 0; i < comm_size; i++)
		{
			rc |= end_shared_lock(win, task, i);
		}
		win->lock_all_is_no_check = -1;
	}
	else
	{
		win->lock_all_is_no_check = -1;
	}


err:
	return rc;
}

int mpc_lowcomm_osc_sync(mpc_lowcomm_window_t *win)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	lcp_task_h task;

	// NOTE: whenever an interface is used but does not support RMA and/or
	//       atomic operation. It is needed to progress the communication.
	//       The solution is to have a progress thread
	mpc_common_progress();

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());

	rc = _lowcomm_osc_perform_flush(win, task, NULL, win->lkey_data);

	return rc;
}

/***********************************************/
/**************** Active Target ****************/
/***********************************************/

static void mpc_osc_handle_incoming_post(mpc_lowcomm_window_t *module, volatile uint64_t *post_ptr,
                                         int ranks_in_grp_win[], int size)
{
	mpc_osc_pending_post_t *post;
	int i, post_rank = (int)*post_ptr - 1;

	/* Reset state post so it can be reused by another process. */
	*post_ptr = 0;

	for (i = 0; i < size; i++)
	{
		if (post_rank == ranks_in_grp_win[i])
		{
			module->post_count++;
			return;
		}
	}

	/* Push to pending list for later start. */
	post       = sctk_malloc(sizeof(mpc_osc_pending_post_t));
	post->rank = post_rank;
	mpc_list_push_head(&module->pending_posts, &post->elem);
}

int mpc_lowcomm_osc_start(mpc_lowcomm_group_t *group, mpc_lowcomm_window_t *win, const bool is_no_check)
{
	int i, rc = MPC_LOWCOMM_SUCCESS;
	int grp_size;
	mpc_lowcomm_group_t *win_grp;
	int *ranks_in_grp, *ranks_in_grp_win;

	win->start_group = mpc_lowcomm_group_dup(group);

	grp_size     = mpc_lowcomm_group_size(win->start_group);
	ranks_in_grp = sctk_malloc(grp_size * sizeof(int));
	if (ranks_in_grp == NULL)
	{
		mpc_common_debug_error("MPC OSC: could not allocate start rank in grp.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	for (i = 0; i < grp_size; i++)
	{
		ranks_in_grp[i] = i;
	}

	ranks_in_grp_win = sctk_malloc(grp_size * sizeof(int));
	if (ranks_in_grp_win == NULL)
	{
		mpc_common_debug_error("MPC OSC: could not allocate start rank in win grp.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	win_grp = mpc_lowcomm_communicator_group(win->comm);
	mpc_lowcomm_group_translate_ranks(group, grp_size, ranks_in_grp, win_grp, ranks_in_grp_win);

	if (is_no_check == false)
	{
		win->post_count = 0;

		/* First, handle pending post list. */
		mpc_osc_pending_post_t *post, *tmp;
		mpc_list_for_each_safe(post, tmp, &win->pending_posts, mpc_osc_pending_post_t, elem)
		{
			for (i = 0; i < grp_size; i++)
			{
				if (post->rank == ranks_in_grp_win[i])
				{
					win->post_count++;
					mpc_list_del(&post->elem);
					break;
				}
			}
		}

		while (win->post_count != grp_size)
		{
			for (i = 0; i < OSC_POST_PEER_MAX; i++)
			{
				if (win->state->post_state[i] == 0)
				{
					continue;
				}

				mpc_osc_handle_incoming_post(win, &win->state->post_state[i], ranks_in_grp_win, grp_size);
			}

			rc = mpc_common_progress();
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}

			mpc_thread_yield();
		}

		win->post_count = 0;
	}

	win->start_grp_ranks = ranks_in_grp_win;

	sctk_free(ranks_in_grp);

err:
	return rc;
}

int mpc_lowcomm_osc_post(mpc_lowcomm_group_t *group, mpc_lowcomm_window_t *win, __UNUSED__ const bool is_no_check)
{
	int                  i, j, rc = MPC_LOWCOMM_SUCCESS;
	uint64_t             my_rank, cur_idx, result;
	uint64_t             inc = 1;
	char *               base_rstate;
	int                  grp_size;
	mpc_lowcomm_group_t *win_grp;
	int *                ranks_in_grp, *ranks_in_grp_win;
	lcp_task_h           task;

	win->post_group = mpc_lowcomm_group_dup(group);

	grp_size     = mpc_lowcomm_group_size(win->post_group);
	ranks_in_grp = sctk_malloc(grp_size * sizeof(int));
	if (ranks_in_grp == NULL)
	{
		mpc_common_debug_error("MPC OSC: could not allocate start rank in grp.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}
	for (i = 0; i < grp_size; i++)
	{
		ranks_in_grp[i] = i;
	}

	ranks_in_grp_win = sctk_malloc(grp_size * sizeof(int));
	if (ranks_in_grp_win == NULL)
	{
		mpc_common_debug_error("MPC OSC: could not allocate start rank in win grp.");
		rc = MPC_LOWCOMM_ERROR;
		goto err;
	}

	win_grp = mpc_lowcomm_communicator_group(win->comm);
	mpc_lowcomm_group_translate_ranks(win->post_group, grp_size, ranks_in_grp, win_grp, ranks_in_grp_win);

	my_rank = mpc_lowcomm_communicator_rank(win->comm);

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());

	/* Acquire a free index. */
	for (i = 0; i < grp_size; i++)
	{
		lcp_ep_h ep;
		_lowcomm_osc_get_target_ep(win, ranks_in_grp_win[i], &ep);

		base_rstate = (char *)win->rstate_win_info[ranks_in_grp_win[i]].addr;

		rc = _lowcomm_osc_perform_atomic(win,
			ep,
			task, inc, LCP_ATOMIC_DT_UINT64,
			&result, (uint64_t)(base_rstate + OSC_STATE_POST_COUNTER_OFFSET),
			win->rstate_win_info[ranks_in_grp_win[i]].rkey,
			LCP_ATOMIC_OP_ADD);
		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}

		cur_idx = result & (OSC_POST_PEER_MAX - 1);

		result = 0;
		do
		{
			rc = _lowcomm_osc_perform_atomic(win,
				ep,
				task, my_rank + 1,
				LCP_ATOMIC_DT_UINT64,
				&result,
				(uint64_t)(base_rstate + OSC_STATE_POST_OFFSET + cur_idx * sizeof(uint64_t)),
				win->rstate_win_info[ranks_in_grp_win[i]].rkey,
				LCP_ATOMIC_OP_CSWAP);

			if (result == 0)
			{
				break;
			}

			for (j = 0; j < OSC_POST_PEER_MAX; j++)
			{
				if (win->state->post_state[j] == 0)
				{
					continue;
				}

				mpc_osc_handle_incoming_post(win, &win->state->post_state[j], ranks_in_grp_win, grp_size);

				mpc_common_progress();
				mpc_thread_yield();
			}
		} while (1);
	}
err:
	return rc;
}

int mpc_lowcomm_osc_complete(mpc_lowcomm_window_t *win)
{
	// Start already completed...
	if (win->start_group == NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	int        i, rc = MPC_LOWCOMM_SUCCESS;
	uint64_t   one = 1;
	uint64_t   base_rstate;
	int        target;
	int        grp_size = mpc_lowcomm_group_size(win->start_group);
	lcp_task_h task;

	task = lcp_context_task_get(win->ctx, mpc_common_get_task_rank());

	rc = _lowcomm_osc_perform_flush(win, task, NULL, NULL);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	for (i = 0; i < grp_size; i++)
	{
		target = win->start_grp_ranks[i];
		lcp_ep_h ep;
		_lowcomm_osc_get_target_ep(win, target, &ep);

		base_rstate = (uint64_t)win->rstate_win_info[win->start_grp_ranks[i]].addr;

		rc = _lowcomm_osc_perform_atomic(win, ep,
			task, one, LCP_ATOMIC_DT_UINT64,
			NULL, base_rstate
			+ OSC_STATE_COMPLETION_COUNTER_OFFSET,
			win->rstate_win_info[win->start_grp_ranks[i]].rkey,
			LCP_ATOMIC_OP_ADD);

		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}
	}

	sctk_free(win->start_grp_ranks);
	mpc_lowcomm_group_free(&win->start_group);
	win->start_group = NULL;
err:
	return rc;
}

int mpc_lowcomm_osc_wait(mpc_lowcomm_window_t *win)
{
	// Post already waited...
	if (win->post_group == NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	int grp_size = mpc_lowcomm_group_size(win->post_group);

	while (win->state->completion_counter != (uint64_t)grp_size)
	{
		mpc_common_progress();
		mpc_thread_yield();
	}

	win->state->completion_counter = 0;
	mpc_lowcomm_group_free(&win->post_group);
	win->post_group = NULL;


	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_osc_test(mpc_lowcomm_window_t *win, bool *is_completed)
{
	// Post already waited...
	if (win->post_group == NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}
	int grp_size = mpc_lowcomm_group_size(win->post_group);

	mpc_common_progress();

	if (win->state->completion_counter == (uint64_t)grp_size)
	{
		*is_completed = true;

		win->state->completion_counter = 0;
		mpc_lowcomm_group_free(&win->post_group);
		win->post_group = NULL;
	}
	else
	{
		*is_completed = false;
		// FIXME: handle active wait.
		mpc_thread_yield();
	}

	return MPC_LOWCOMM_SUCCESS;
}
