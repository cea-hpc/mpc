/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include "osc_mpi.h"

#include "osc_module.h"

#include "mpc_thread_accessor.h"
#include "sctk_alloc.h"

int mpc_osc_fence(int mpi_assert, mpc_win_t *win)
{
        UNUSED(mpi_assert);
        mpc_osc_module_t *module = &win->win_module;
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_task_h task;
        int tid = mpc_common_get_task_rank();

        if (module->epoch.access != NONE_EPOCH && 
            module->epoch.access != FENCE_EPOCH) {
                return MPC_LOWCOMM_ERROR;
        }

        if (mpi_assert & MPI_MODE_NOSUCCEED) {
                module->epoch.access = NONE_EPOCH;
        } else {
                module->epoch.access = FENCE_EPOCH;
        }

        /* Get task. */
        task = lcp_context_task_get(module->ctx, tid);

        if (!(mpi_assert & MPI_MODE_NOSUCCEED)) {
                rc = mpc_osc_perform_flush_op(module, task, NULL, NULL);
                if (rc != MPC_LOWCOMM_SUCCESS)
                        goto err;
        }

        PMPI_Barrier(win->comm);
err:
        return rc;
}

void mpc_osc_handle_incoming_post(mpc_osc_module_t *module, volatile uint64_t *post_ptr, 
                                 int ranks_in_grp_win[], int size) 
{
        mpc_osc_pending_post_t *post;
        int i, post_rank = (int)*post_ptr - 1;

        /* Reset state post so it can be reused by another process. */
        *post_ptr = 0;
        
        for (i = 0; i < size; i++) {
                if (post_rank == ranks_in_grp_win[i]) {
                        module->post_count++;
                        return;
                }
        }

        /* Push to pending list for later start. */
        post = sctk_malloc(sizeof(mpc_osc_pending_post_t));
        post->rank = post_rank;
        mpc_list_push_head(&module->pending_posts, &post->elem);
}

int mpc_osc_start(mpc_lowcomm_group_t *group, 
                  int mpi_assert, 
                  mpc_win_t *win)
{
        UNUSED(mpi_assert);
        int i, rc = MPC_LOWCOMM_SUCCESS;
        int grp_size, grp_win_size;
        mpc_lowcomm_group_t *win_grp;
        int *ranks_in_grp, *ranks_in_grp_win;

        if (win->win_module.epoch.access != NONE_EPOCH && 
            win->win_module.epoch.access != FENCE_EPOCH) {
                mpc_common_debug_error("MPC OSC: access epoch already active.");
                return MPC_LOWCOMM_ERROR;
        }

        win->win_module.epoch.access = START_COMPLETE_EPOCH;
        win->win_module.start_group  = mpc_lowcomm_group_dup(group);
        
        grp_size = mpc_lowcomm_group_size(win->win_module.start_group);
        ranks_in_grp = sctk_malloc(grp_size * sizeof(int));
        if (ranks_in_grp == NULL) {
                mpc_common_debug_error("MPC OSC: could not allocate start "
                                       "rank in grp.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        for (i = 0; i < grp_size; i++) {
                ranks_in_grp[i] = i;
        }

        ranks_in_grp_win = sctk_malloc(grp_size * sizeof(int));
        if (ranks_in_grp_win == NULL) {
                mpc_common_debug_error("MPC OSC: could not allocate start "
                                       "rank in win grp.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        win_grp = mpc_lowcomm_communicator_group(win->comm);
        grp_win_size = mpc_lowcomm_group_size(win_grp);
        mpc_lowcomm_group_translate_ranks(group, grp_size, ranks_in_grp, 
                                          win_grp, ranks_in_grp_win);

        if ((mpi_assert & MPI_MODE_NOCHECK) == 0) {
                win->win_module.post_count = 0;

                /* First, handle pending post list. */
                mpc_osc_pending_post_t *post, *tmp;
                mpc_list_for_each_safe(post, tmp, &win->win_module.pending_posts, 
                                       mpc_osc_pending_post_t, elem) {
                        for (i = 0; i < grp_size; i++) {
                                if (post->rank == ranks_in_grp_win[i]) {
                                        win->win_module.post_count++;
                                        mpc_list_del(&post->elem);
                                        break;
                                }
                        }
                }

                while (win->win_module.post_count != grp_size) {

                        for (i = 0; i < OSC_POST_PEER_MAX; i++) {
                                if (win->win_module.state->post_state[i] == 0) {
                                        continue;
                                }

                                mpc_osc_handle_incoming_post(&win->win_module, 
                                                             &win->win_module.state->post_state[i], 
                                                             ranks_in_grp_win, grp_size);
                        }

                        lcp_progress(win->win_module.mngr);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }

                        mpc_thread_yield();

                        //mpc_osc_schedule_progress(win->win_module.mngr,
                        //                          &win->win_module.post_count,
                        //                          grp_size);
                }

                win->win_module.post_count = 0;

        }

        win->win_module.start_grp_ranks = ranks_in_grp_win;

        sctk_free(ranks_in_grp);

err:
        return rc;
}

int mpc_osc_post(mpc_lowcomm_group_t *group, int mpi_assert, mpc_win_t *win) 
{
        UNUSED(mpi_assert);
        mpc_osc_module_t *module = &win->win_module;
        int i, j, rc = MPC_LOWCOMM_SUCCESS;
        uint64_t my_rank, cur_idx, result;
        uint64_t inc = 1;
        char *base_rstate;
        int grp_size;
        mpc_lowcomm_group_t *win_grp;
        int *ranks_in_grp, *ranks_in_grp_win;
        lcp_task_h task;

        if (module->epoch.exposure != NONE_EPOCH) {
                mpc_common_debug_error("MPC OSC: access epoch already active.");
                return MPC_LOWCOMM_ERROR;
        }

        module->post_group = mpc_lowcomm_group_dup(group);
        
        grp_size = mpc_lowcomm_group_size(module->post_group);
        ranks_in_grp = sctk_malloc(grp_size * sizeof(int));
        if (ranks_in_grp == NULL) {
                mpc_common_debug_error("MPC OSC: could not allocate start "
                                       "rank in grp.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        for (i = 0; i < grp_size; i++) {
                ranks_in_grp[i] = i;
        }

        ranks_in_grp_win = sctk_malloc(grp_size * sizeof(int));
        if (ranks_in_grp_win == NULL) {
                mpc_common_debug_error("MPC OSC: could not allocate start "
                                       "rank in win grp.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        win_grp = mpc_lowcomm_communicator_group(win->comm);
        mpc_lowcomm_group_translate_ranks(group, grp_size, ranks_in_grp, 
                                          win_grp, ranks_in_grp_win);

        my_rank = mpc_lowcomm_communicator_rank(win->comm);

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        /* Acquire a free index. */
        for (i = 0; i < grp_size; i++) {
               if (module->eps[ranks_in_grp_win[i]] == NULL) {
                       uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                          ranks_in_grp_win[i]);

                       rc = lcp_ep_create(module->mngr, &module->eps[ranks_in_grp_win[i]], 
                                          target_uid, 0);
                       if (rc != 0) {
                               mpc_common_debug_fatal("Could not create endpoint.");
                       }
                }

               base_rstate = (char *)module->rstate_win_info[ranks_in_grp_win[i]].addr;

                rc = mpc_osc_perform_atomic_op(module,
                                               module->eps[ranks_in_grp_win[i]],
                                               task, inc, sizeof(uint64_t),
                                               &result, (uint64_t)(base_rstate
                                                                   + OSC_STATE_POST_COUNTER_OFFSET),
                                               module->rstate_win_info[ranks_in_grp_win[i]].rkey,
                                               LCP_ATOMIC_OP_ADD);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                cur_idx = result & (OSC_POST_PEER_MAX - 1);

                result = 0;
                do {
                        rc = mpc_osc_perform_atomic_op(module,
                                                       module->eps[ranks_in_grp_win[i]],
                                                       task, my_rank + 1,
                                                       sizeof(uint64_t),
                                                       &result,
                                                       (uint64_t)(base_rstate
                                                                  + OSC_STATE_POST_OFFSET
                                                                  + cur_idx*sizeof(uint64_t)),
                                                       module->rstate_win_info[ranks_in_grp_win[i]].rkey,
                                                       LCP_ATOMIC_OP_CSWAP);

                        if (result == 0) {
                                break;
                        }

                        for (j = 0; j < OSC_POST_PEER_MAX; j++) {
                                if (win->win_module.state->post_state[j] == 0) {
                                        continue;
                                }

                                mpc_osc_handle_incoming_post(module, 
                                                             &module->state->post_state[j], 
                                                             ranks_in_grp_win, grp_size);

                                lcp_progress(module->mngr);
                                usleep(100);
                        }
                } while (1);

        }

        module->epoch.exposure = POST_WAIT_EPOCH;
err:
        return rc;
}

int mpc_osc_complete(mpc_win_t *win)
{
        mpc_osc_module_t *module = &win->win_module;
        int i, rc = MPC_LOWCOMM_SUCCESS;
        uint64_t one = 1;
        uint64_t base_rstate;
        int target;
        int grp_size = mpc_lowcomm_group_size(win->win_module.start_group);
        lcp_task_h task;

        if (module->epoch.access != START_COMPLETE_EPOCH) {
                return MPC_LOWCOMM_ERROR;
        }

        module->epoch.access = NONE_EPOCH;

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        rc = mpc_osc_perform_flush_op(module, task, NULL, NULL);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        for (i = 0; i < grp_size; i++) {
                target = module->start_grp_ranks[i];
               if (module->eps[target] == NULL) {
                       uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                          target);

                       rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                          target_uid, 0);
                       if (rc != 0) {
                               mpc_common_debug_fatal("Could not create endpoint.");
                       }
                }

               base_rstate = (uint64_t)module->rstate_win_info[module->start_grp_ranks[i]].addr;
                rc = mpc_osc_perform_atomic_op(module, module->eps[target],
                                               task, one, sizeof(uint64_t),
                                               NULL, base_rstate
                                               + OSC_STATE_COMPLETION_COUNTER_OFFSET,
                                               module->rstate_win_info[module->start_grp_ranks[i]].rkey,
                                               LCP_ATOMIC_OP_ADD);

                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }
        }


        sctk_free(win->win_module.start_grp_ranks);
        mpc_lowcomm_group_free(&win->win_module.start_group);
        win->win_module.start_group = NULL;
err:
        return rc;
}

int mpc_osc_wait(mpc_win_t *win)
{
        mpc_osc_module_t *module = &win->win_module;
        int grp_size = mpc_lowcomm_group_size(win->win_module.post_group);

        if (win->win_module.epoch.exposure != POST_WAIT_EPOCH) {
                return MPC_LOWCOMM_ERROR;
        }
        while(win->win_module.state->completion_counter != (uint64_t) grp_size) {
                lcp_progress(module->mngr);
                mpc_thread_yield();
        }

        //mpc_osc_schedule_progress(module->mngr,
        //                          (int *)&module->state->completion_counter,
        //                          grp_size);

        win->win_module.state->completion_counter = 0;
        mpc_lowcomm_group_free(&win->win_module.post_group);
        win->win_module.post_group = NULL;

        win->win_module.epoch.exposure = NONE_EPOCH;

        return MPC_LOWCOMM_SUCCESS;
}

int mpc_osc_test(mpc_win_t *win, int *flag) 
{
        mpc_osc_module_t *module = &win->win_module;
        int grp_size = mpc_lowcomm_group_size(win->win_module.post_group);

        if (win->win_module.epoch.exposure != POST_WAIT_EPOCH) {
                return MPC_LOWCOMM_ERROR;
        }

        lcp_progress(module->mngr);

        if (win->win_module.state->completion_counter == (uint64_t) grp_size) {
                *flag = 1;

                win->win_module.state->completion_counter = 0;
                win->win_module.post_group = NULL;

                win->win_module.epoch.exposure = NONE_EPOCH;
        } else {
                *flag = 0;
                //FIXME: handle active wait.
                mpc_thread_yield();
        }


        return MPC_LOWCOMM_SUCCESS;
}
