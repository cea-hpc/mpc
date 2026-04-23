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

/**@file
 * One sided synchronization routines implementation
 */

/***********************************************/
/*************** Passive Target ****************/
/***********************************************/

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Tests if a window is already locked in exclusive mode
 *
 * This function is used to know if an atomic lock is necessary as those two type of locks are very similar.
 *
 * @param[in] target Rank of the target task
 * @param[in] window Window to check the if the lock is held
 *
 * @retval true if an atomic lock is needed (no exclusive lock found)
 * @retval false if no atomic lock is needed (exclusive lock found)
 */
static inline bool __sync_need_atomic_lock(const int target, mpc_lowcomm_window_t *window)
{
	_lowcomm_osc_lock_t *lock = mpc_common_hashtable_get(&(window->outstanding_locks), target);

	return !(lock != NULL && (lock->type == MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE && !lock->is_no_check));
}

int _lowcomm_osc_start_exclusive_lock(lcp_task_h task, const int target, const uint64_t lock_offset,
                                      mpc_lowcomm_window_t *window)
{
	int      rc         = MPC_LOWCOMM_SUCCESS;
	uint64_t lock_state = OSC_LOCK_UNLOCK;
	uint64_t base_rstate;

	do
	{
		base_rstate = (uint64_t)window->rstate_win_info[target].addr;
		lcp_ep_h ep;
		_lowcomm_osc_get_target_ep(target, window, &ep);

		rc = _lowcomm_osc_perform_atomic(task, ep, OSC_LOCK_EXCLUSIVE, &lock_state, base_rstate + lock_offset,
			window->rstate_win_info[target].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_CSWAP, window);

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

		mpc_common_nodebug("start exclusive. to=%d", target);

		break;
	} while (1);

err:
	return rc;
}

int _lowcomm_osc_end_exclusive_lock(lcp_task_h task, const int target, const uint64_t lock_offset,
                                    mpc_lowcomm_window_t *window)
{
	int      rc          = MPC_LOWCOMM_SUCCESS;
	uint64_t value       = -OSC_LOCK_EXCLUSIVE;
	uint64_t base_rstate = (uint64_t)window->rstate_win_info[target].addr;

	lcp_ep_h ep;

	_lowcomm_osc_get_target_ep(target, window, &ep);
	mpc_common_nodebug("end exclusive. to=%d", target);
	rc = _lowcomm_osc_perform_atomic(task, ep, value, NULL, base_rstate + lock_offset,
		window->rstate_win_info[target].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_ADD, window);
	return rc;
}

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Locks a window in shared mode
 *
 * @param[in] task   Current LCP Task
 * @param[in] target Rank of the target task
 * @param[in] window Window to lock
 *
 * @return           MPC_LOWCOMM_SUCCESS upon success
 */
static int __sync_start_shared_lock(lcp_task_h task, const int target, mpc_lowcomm_window_t *window)
{
	int      rc = MPC_LOWCOMM_SUCCESS;
	uint64_t lock_state;
	uint64_t base_rstate;

	lcp_ep_h ep;

	_lowcomm_osc_get_target_ep(target, window, &ep);

	do
	{
		base_rstate = (uint64_t)window->rstate_win_info[target].addr;
		rc          = _lowcomm_osc_perform_atomic(task, ep, 1, &lock_state, base_rstate + OSC_STATE_GLOBAL_LOCK_OFFSET,
			window->rstate_win_info[target].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_ADD, window);

		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}

		if (lock_state >= OSC_LOCK_EXCLUSIVE)
		{
			/* back off. */
			rc = _lowcomm_osc_perform_atomic(task, ep, -1, NULL, base_rstate + OSC_STATE_GLOBAL_LOCK_OFFSET,
				window->rstate_win_info[target].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_ADD, window);

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

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Unlocks a window in shared mode
 *
 * @param[in] task   Current LCP Task
 * @param[in] target Rank of the target task
 * @param[in] window Window to unlock
 *
 * @return           MPC_LOWCOMM_SUCCESS upon success
 */
static int __sync_end_shared_lock(lcp_task_h task, const int target, mpc_lowcomm_window_t *window)
{
	int      rc          = MPC_LOWCOMM_SUCCESS;
	uint64_t base_rstate = (uint64_t)window->rstate_win_info[target].addr;

	lcp_ep_h ep;

	_lowcomm_osc_get_target_ep(target, window, &ep);

	rc = _lowcomm_osc_perform_atomic(task, ep, -1, NULL, base_rstate + OSC_STATE_GLOBAL_LOCK_OFFSET,
		window->rstate_win_info[target].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_ADD, window);

	return rc;
}

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Locks a window in atomic mode
 *
 * @note If the window is already locked in exclusive mode, this function does nothing
 *
 * @param[in] task   Current LCP Task
 * @param[in] target Rank of the target task
 * @param[in] window Window to lock
 *
 * @retval           MPC_LOWCOMM_SUCCESS upon success
 * @retval           MPC_LOWCOMM_OSC_ALREADY_LOCKED if the lock is already taken (no need to unlock)
 * @retval           A LowComm errorcode otherwise
 */
static int __sync_start_atomic_lock(lcp_task_h task, const int target, mpc_lowcomm_window_t *window)
{
	int      rc               = MPC_LOWCOMM_OSC_ALREADY_LOCKED;
	uint64_t lock_state       = 0;
	uint64_t value            = 1;
	size_t   remote_lock_addr = window->rstate_win_info[target].addr + OSC_STATE_ACC_LOCK_OFFSET;
	lcp_ep_h ep;

	_lowcomm_osc_get_target_ep(target, window, &ep);

	if (__sync_need_atomic_lock(target, window))
	{
		// FIXME: should this be added to a perform_idle type of function to avoid active waiting.
		do
		{
			rc = _lowcomm_osc_perform_atomic(task, ep, value, &lock_state, remote_lock_addr,
				window->rstate_win_info[target].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_CSWAP, window);

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

	mpc_common_nodebug("acc atomic lock. from=%d, to=%d, remote lock addr=%p",
		mpc_common_get_task_rank(), target, remote_lock_addr);

err:
	return rc;
}

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Unlocks a window in atomic mode
 *
 * @note If the window is already locked in exclusive mode, this function does nothing
 *
 * @param[in] task   Current LCP Task
 * @param[in] target Rank of the target task
 * @param[in] window Window to unlock
 *
 * @return           MPC_LOWCOMM_SUCCESS upon success
 */
static int __sync_end_atomic_lock(lcp_task_h task, const int target, mpc_lowcomm_window_t *window)
{
	int      rc               = MPC_LOWCOMM_SUCCESS;
	uint64_t value            = 0;
	size_t   remote_lock_addr = window->rstate_win_info[target].addr + OSC_STATE_ACC_LOCK_OFFSET;
	uint64_t lock_state       = 1;
	lcp_ep_h ep;

	_lowcomm_osc_get_target_ep(target, window, &ep);
	assert(ep);

	mpc_common_nodebug("unlock. remote lock addr=%p", remote_lock_addr);
	if (__sync_need_atomic_lock(target, window))
	{
		rc = _lowcomm_osc_perform_atomic(task, ep, value, &lock_state, remote_lock_addr,
			window->rstate_win_info[target].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_CSWAP, window);

		if (lock_state != 1)
		{
			mpc_common_debug_fatal("failed with lock_state = %d", lock_state);
		}
	}

	mpc_common_nodebug("acc atomic unlock. from=%d, to=%d, remote lock addr=%p",
		mpc_common_get_task_rank(), target, remote_lock_addr);

	return rc;
}

int mpc_lowcomm_osc_lock(const mpc_lowcomm_osc_lock_type_t lock_type, const int target_rank,
                         mpc_lowcomm_window_t *window, const bool is_no_check)
{
	int        rc   = MPC_LOWCOMM_SUCCESS;
	lcp_task_h task = lcp_context_task_get(window->ctx, mpc_common_get_task_rank());

	_lowcomm_osc_lock_t *lock = NULL;

	if (lock_type == MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE || lock_type == MPC_LOWCOMM_WINDOW_LOCK_SHARED)
	{
		// The target is already locked
		lock = mpc_common_hashtable_get(&window->outstanding_locks, target_rank);
		if (lock != NULL)
		{
			return MPC_LOWCOMM_OSC_ALREADY_LOCKED;
		}

		lock = sctk_malloc(sizeof(_lowcomm_osc_lock_t));
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
			rc         = _lowcomm_osc_start_exclusive_lock(task, target_rank, OSC_STATE_GLOBAL_LOCK_OFFSET, window);
			break;

		case MPC_LOWCOMM_WINDOW_LOCK_SHARED:
			lock->type = MPC_LOWCOMM_WINDOW_LOCK_SHARED;
			rc         = __sync_start_shared_lock(task, target_rank, window);
			break;

		case MPC_LOWCOMM_WINDOW_LOCK_ATOMIC:
			rc = __sync_start_atomic_lock(task, target_rank, window);
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
		mpc_common_hashtable_set(&window->outstanding_locks, target_rank, lock);
	}
	else
	{
		sctk_free(lock);
	}

end:
	return rc;
}

int mpc_lowcomm_osc_unlock(const int target_rank, mpc_lowcomm_window_t *window, const bool is_atomic)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	lcp_ep_h   ep;
	lcp_task_h task = lcp_context_task_get(window->ctx, mpc_common_get_task_rank());

	_lowcomm_osc_get_target_ep(target_rank, window, &ep);

	rc = _lowcomm_osc_perform_flush(task, ep, NULL, window);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto end;
	}

	if ((is_atomic))
	{
		rc = __sync_end_atomic_lock(task, target_rank, window);
		goto end;
	}

	_lowcomm_osc_lock_t *lock;
	lock = mpc_common_hashtable_get(&window->outstanding_locks, target_rank);
	// Check the lock is still active
	if (lock == NULL)
	{
		return MPC_LOWCOMM_ERROR;
	}
	mpc_common_hashtable_delete(&window->outstanding_locks, target_rank);

	if (lock->is_no_check == 0)
	{
		switch (lock->type)
		{
		case MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE:
			rc = _lowcomm_osc_end_exclusive_lock(task, target_rank, OSC_STATE_GLOBAL_LOCK_OFFSET, window);
			break;

		case MPC_LOWCOMM_WINDOW_LOCK_SHARED:
			rc = __sync_end_shared_lock(task, target_rank, window);
			break;

		case MPC_LOWCOMM_WINDOW_LOCK_ATOMIC:
			rc = __sync_end_atomic_lock(task, target_rank, window);
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

int mpc_lowcomm_osc_lock_all(mpc_lowcomm_window_t *window, const bool is_no_check)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	int        i, j;
	lcp_task_h task;

	const int comm_size = mpc_lowcomm_communicator_size(window->comm);

	if (is_no_check == false)
	{
		task = lcp_context_task_get(window->ctx, mpc_common_get_task_rank());
		for (i = 0; i < comm_size; i++)
		{
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				goto err;
			}
			rc = __sync_start_shared_lock(task, i, window);
			if (rc != MPC_LOWCOMM_SUCCESS)
			{
				for (j = 0; j < i; j++)
				{
					__sync_end_shared_lock(task, j, window);
				}
				return rc;
			}
		}
		window->lock_all_is_no_check = 0;
	}
	else
	{
		window->lock_all_is_no_check = 1;
	}

err:
	return rc;
}

int mpc_lowcomm_osc_unlock_all(mpc_lowcomm_window_t *window)
{
	int        rc = MPC_LOWCOMM_SUCCESS;
	int        i;
	lcp_task_h task = NULL;

	task = lcp_context_task_get(window->ctx, mpc_common_get_task_rank());
	const int comm_size = mpc_lowcomm_communicator_size(window->comm);

	rc = _lowcomm_osc_perform_flush(task, NULL, NULL, window);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	if (window->lock_all_is_no_check == 0)
	{
		for (i = 0; i < comm_size; i++)
		{
			rc |= __sync_end_shared_lock(task, i, window);
		}
		window->lock_all_is_no_check = -1;
	}
	else
	{
		window->lock_all_is_no_check = -1;
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

	rc = _lowcomm_osc_perform_flush(task, NULL, win->lkey_data, win);

	return rc;
}

/***********************************************/
/**************** Active Target ****************/
/***********************************************/

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Updates the pending posts list on receiving a post from another task
 *
 * @param[in] post_ptr         Some internal state
 * @param[in] size             Number of tasks in the group
 * @param[in] ranks_in_grp_win Translation table between tasks in group and global task ranks
 * @param[in] window           Window for which the post are handled
 */
static void __sync_handle_incoming_post(volatile uint64_t *post_ptr, const int size,
                                        const int ranks_in_grp_win[static const size], mpc_lowcomm_window_t *window)
{
	_lowcomm_osc_pending_post_t *post;
	int i, post_rank = (int)*post_ptr - 1;

	/* Reset state post so it can be reused by another process. */
	*post_ptr = 0;

	for (i = 0; i < size; i++)
	{
		if (post_rank == ranks_in_grp_win[i])
		{
			window->post_count++;
			return;
		}
	}

	/* Push to pending list for later start. */
	post       = sctk_malloc(sizeof(_lowcomm_osc_pending_post_t));
	post->rank = post_rank;
	mpc_list_push_head(&window->pending_posts, &post->elem);
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
		_lowcomm_osc_pending_post_t *post, *tmp;
		mpc_list_for_each_safe(post, tmp, &win->pending_posts, _lowcomm_osc_pending_post_t, elem)
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

				__sync_handle_incoming_post(&win->state->post_state[i], grp_size, ranks_in_grp_win, win);
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

int mpc_lowcomm_osc_post(mpc_lowcomm_group_t *group, mpc_lowcomm_window_t *window, __UNUSED__ const bool is_no_check)
{
	int                  i, j, rc = MPC_LOWCOMM_SUCCESS;
	uint64_t             my_rank, cur_idx, result;
	uint64_t             inc = 1;
	char *               base_rstate;
	int                  grp_size;
	mpc_lowcomm_group_t *win_grp;
	int *                ranks_in_grp, *ranks_in_grp_win;
	lcp_task_h           task;

	window->post_group = mpc_lowcomm_group_dup(group);

	grp_size     = mpc_lowcomm_group_size(window->post_group);
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

	win_grp = mpc_lowcomm_communicator_group(window->comm);
	mpc_lowcomm_group_translate_ranks(window->post_group, grp_size, ranks_in_grp, win_grp, ranks_in_grp_win);

	my_rank = mpc_lowcomm_communicator_rank(window->comm);

	task = lcp_context_task_get(window->ctx, mpc_common_get_task_rank());

	/* Acquire a free index. */
	for (i = 0; i < grp_size; i++)
	{
		lcp_ep_h ep;
		_lowcomm_osc_get_target_ep(ranks_in_grp_win[i], window, &ep);

		base_rstate = (char *)window->rstate_win_info[ranks_in_grp_win[i]].addr;

		rc = _lowcomm_osc_perform_atomic(task, ep, inc, &result,
			(uint64_t)(base_rstate + OSC_STATE_POST_COUNTER_OFFSET), window->rstate_win_info[ranks_in_grp_win[i]].rkey,
			LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_ADD, window);

		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}

		cur_idx = result & (OSC_POST_PEER_MAX - 1);

		result = 0;
		do
		{
			rc = _lowcomm_osc_perform_atomic(task, ep, my_rank + 1, &result,
				(uint64_t)(base_rstate + OSC_STATE_POST_OFFSET + cur_idx * sizeof(uint64_t)),
				window->rstate_win_info[ranks_in_grp_win[i]].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_CSWAP, window);

			if (result == 0)
			{
				break;
			}

			for (j = 0; j < OSC_POST_PEER_MAX; j++)
			{
				if (window->state->post_state[j] == 0)
				{
					continue;
				}

				__sync_handle_incoming_post(&window->state->post_state[j], grp_size, ranks_in_grp_win, window);

				mpc_common_progress();
				mpc_thread_yield();
			}
		} while (1);
	}
err:
	return rc;
}

int mpc_lowcomm_osc_complete(mpc_lowcomm_window_t *window)
{
	// Start already completed...
	if (window->start_group == NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	int        i, rc = MPC_LOWCOMM_SUCCESS;
	uint64_t   one = 1;
	uint64_t   base_rstate;
	int        target;
	int        grp_size = mpc_lowcomm_group_size(window->start_group);
	lcp_task_h task;

	task = lcp_context_task_get(window->ctx, mpc_common_get_task_rank());

	rc = _lowcomm_osc_perform_flush(task, NULL, NULL, window);
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	for (i = 0; i < grp_size; i++)
	{
		target = window->start_grp_ranks[i];
		lcp_ep_h ep;
		_lowcomm_osc_get_target_ep(target, window, &ep);

		base_rstate = (uint64_t)window->rstate_win_info[window->start_grp_ranks[i]].addr;

		rc = _lowcomm_osc_perform_atomic(task, ep, one, NULL, base_rstate + OSC_STATE_COMPLETION_COUNTER_OFFSET,
			window->rstate_win_info[window->start_grp_ranks[i]].rkey, LCP_ATOMIC_DT_UINT64, LCP_ATOMIC_OP_ADD, window);

		if (rc != MPC_LOWCOMM_SUCCESS)
		{
			goto err;
		}
	}

	sctk_free(window->start_grp_ranks);
	mpc_lowcomm_group_free(&window->start_group);
	window->start_group = NULL;
err:
	return rc;
}

int mpc_lowcomm_osc_wait(mpc_lowcomm_window_t *window)
{
	// Post already waited...
	if (window->post_group == NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}

	int grp_size = mpc_lowcomm_group_size(window->post_group);

	while (window->state->completion_counter != (uint64_t)grp_size)
	{
		mpc_common_progress();
		mpc_thread_yield();
	}

	window->state->completion_counter = 0;
	mpc_lowcomm_group_free(&window->post_group);
	window->post_group = NULL;


	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_osc_test(mpc_lowcomm_window_t *window, bool *is_completed)
{
	// Post already waited...
	if (window->post_group == NULL)
	{
		return MPC_LOWCOMM_SUCCESS;
	}
	int grp_size = mpc_lowcomm_group_size(window->post_group);

	mpc_common_progress();

	if (window->state->completion_counter == (uint64_t)grp_size)
	{
		*is_completed = true;

		window->state->completion_counter = 0;
		mpc_lowcomm_group_free(&window->post_group);
		window->post_group = NULL;
	}
	else
	{
		*is_completed = false;
		// FIXME: handle active wait.
		mpc_thread_yield();
	}

	return MPC_LOWCOMM_SUCCESS;
}
