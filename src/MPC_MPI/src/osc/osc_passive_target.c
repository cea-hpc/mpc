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

#include "mpc_thread_accessor.h"
#include "osc_mpi.h"
#include "osc_module.h"

#include <sctk_alloc.h>

static uint64_t one       = 1;
static uint64_t minusone  = -1;
static uint64_t zero      = 0;

static int start_exclusive(mpc_win_t *win, lcp_task_h task,
                                  int target)
{
        mpc_osc_module_t *module = &win->win_module;
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t lock_state = 0;
        uint64_t base_rstate;

        do {
               if (module->eps[target] == NULL) {
                       uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                          target);

                       rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                          target_uid, 0);
                       if (rc != 0) {
                               mpc_common_debug_fatal("Could not create endpoint.");
                       }
                }

               base_rstate = (uint64_t)module->rstate_win_info[target].addr;
                
                rc = mpc_osc_perform_atomic_op(module, module->eps[target],
                                               task, zero, sizeof(uint64_t),
                                               &lock_state, base_rstate
                                               + OSC_STATE_GLOBAL_LOCK_OFFSET,
                                               module->rstate_win_info[target].rkey,
                                               LCP_ATOMIC_OP_CSWAP);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                if (lock_state != 0) {
                        rc = lcp_progress(module->mngr);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                        continue;
                }

                break;
        } while (1);

err:
        return rc;
}

static int end_exclusive(mpc_win_t *win, lcp_task_h task,
                                int target)
{
        mpc_osc_module_t *module = &win->win_module;
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t value = -OSC_LOCK_EXCLUSIVE;
        uint64_t base_rstate = (uint64_t)module->rstate_win_info[target].addr;

        rc = mpc_osc_perform_atomic_op(module, module->eps[target], task, value,
                                       sizeof(uint64_t), NULL, base_rstate
                                       + OSC_STATE_GLOBAL_LOCK_OFFSET,
                                       module->rstate_win_info[target].rkey,
                                       LCP_ATOMIC_OP_ADD);
        return rc;
}

static int start_shared(mpc_win_t *win, lcp_task_h task,
                               int target)
{
        mpc_osc_module_t *module = &win->win_module;
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t lock_state = 0;
        uint64_t base_rstate;

        do {
               if (module->eps[target] == NULL) {
                       uint64_t target_uid = mpc_lowcomm_communicator_uid(win->comm, 
                                                                          target);

                       rc = lcp_ep_create(module->mngr, &module->eps[target], 
                                          target_uid, 0);
                       if (rc != 0) {
                               mpc_common_debug_fatal("Could not create endpoint.");
                       }
                }
                
               base_rstate = (uint64_t)module->rstate_win_info[target].addr; 
               rc = mpc_osc_perform_atomic_op(module, module->eps[target],
                                                   task, one, sizeof(uint64_t),
                                                   &lock_state, base_rstate
                                                   + OSC_STATE_GLOBAL_LOCK_OFFSET,
                                                   module->rstate_win_info[target].rkey,
                                                   LCP_ATOMIC_OP_ADD);
                if (rc != MPC_LOWCOMM_SUCCESS) {
                        goto err;
                }

                if (lock_state >= OSC_LOCK_EXCLUSIVE) {
                        /* back off. */
                        rc = mpc_osc_perform_atomic_op(module,
                                                       module->eps[target],
                                                       task, minusone,
                                                       sizeof(uint64_t), NULL,
                                                       base_rstate
                                                       + OSC_STATE_GLOBAL_LOCK_OFFSET,
                                                       module->rstate_win_info[target].rkey,
                                                       LCP_ATOMIC_OP_ADD);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }

                break;
        } while (1);
                        
err:
        return rc;
}

static int end_shared(mpc_win_t *win, lcp_task_h task,
                             int target)
{
        mpc_osc_module_t *module = &win->win_module;
        int rc = MPC_LOWCOMM_SUCCESS;
        uint64_t base_rstate = (uint64_t)module->rstate_win_info[target].addr;

        rc = mpc_osc_perform_atomic_op(module, module->eps[target], task,
                                       minusone, sizeof(uint64_t), NULL,
                                       base_rstate
                                       + OSC_STATE_GLOBAL_LOCK_OFFSET,
                                       module->rstate_win_info[target].rkey,
                                       LCP_ATOMIC_OP_ADD);
        return rc;

}

int mpc_osc_lock(int lock_type, int target, int mpi_assert, mpc_win_t *win)
{
        int rc = MPC_LOWCOMM_ERROR;
        mpc_osc_module_t *module = &win->win_module;
        mpc_osc_epoch_t original_epoch = module->epoch.access;
        lcp_task_h task;

        if (module->lock_count == 0) {
                if (module->epoch.access != NONE_EPOCH &&
                    module->epoch.access != FENCE_EPOCH) {
                        return MPC_LOWCOMM_ERROR;
                }
        } else {
                mpc_osc_lock_t *lock;
                lock = mpc_common_hashtable_get(&module->outstanding_locks, target);
                if (lock != NULL) {
                        return MPC_LOWCOMM_ERROR;
                }
        }

        module->epoch.access = PASSIVE_EPOCH;
        module->lock_count++;

        mpc_osc_lock_t *lock = sctk_malloc(sizeof(mpc_osc_lock_t));
        if (lock == NULL) {
                mpc_common_debug_error("MPC OSC: could not allocate new lock.");
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }
        lock->target = target;

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        if ((mpi_assert & MPI_MODE_NOCHECK) == 0) {
                if (lock_type == MPI_LOCK_EXCLUSIVE) {
                        rc = start_exclusive(win, task, target);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                } else {
                        rc = start_shared(win, task, target);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
                lock->is_no_check = 0;
        } else {
                lock->is_no_check = 1;
        }

        if (rc == MPC_LOWCOMM_SUCCESS) {
                mpc_common_hashtable_set(&module->outstanding_locks, target, lock);
        } else {
                sctk_free(lock);
                module->epoch.access = original_epoch;
        }
err:
        return rc;
}

int mpc_osc_unlock(int target, mpc_win_t *win) 
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        lcp_ep_h ep;
        lcp_task_h task;
        mpc_osc_lock_t *lock;

        if (module->epoch.access != PASSIVE_EPOCH) {
                return MPC_LOWCOMM_ERROR;
        }

        lock = mpc_common_hashtable_get(&module->outstanding_locks, target);
        if (lock == NULL) {
                return MPC_LOWCOMM_ERROR;
        }

        mpc_common_hashtable_delete(&module->outstanding_locks, target);

        ep = module->eps[target]; 

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        rc = mpc_osc_perform_flush_op(module, task, ep, NULL);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        if (lock->is_no_check == 0) {
                if (lock->type == LOCK_EXCLUSIVE) {
                        rc = end_exclusive(win, task, target);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                } else {
                        rc = end_shared(win, task, target);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
        }

        sctk_free(lock);

        module->lock_count--;
        if (module->lock_count == 0) {
                module->epoch.access = NONE_EPOCH;
        }

err:
        return rc;
}

int mpc_osc_lock_all(int mpi_assert, mpc_win_t *win)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i, j;
        mpc_osc_module_t *module = &win->win_module;
        lcp_task_h task;

        if (module->lock_count == 0) {
                if (module->epoch.access != NONE_EPOCH &&
                    module->epoch.access != FENCE_EPOCH) {
                        return MPC_LOWCOMM_ERROR;
                }
        } 

        module->epoch.access = PASSIVE_ALL_EPOCH;

        if ((mpi_assert & MPI_MODE_NOCHECK) == 0) {
                task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());
                for (i = 0; i < win->comm_size; i++) {
                        rc = start_shared(win, task, i);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                for (j = 0; j < i; j++) {
                                        end_shared(win, task, j);
                                }
                                return rc;
                        }
                }
                module->lock_all_is_no_check = 0;
        } else {
                module->lock_all_is_no_check = 1;
        }

        return rc;
}

int mpc_osc_unlock_all(mpc_win_t *win)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        int i;
        mpc_osc_module_t *module = &win->win_module;
        lcp_task_h task = NULL;

        if (module->epoch.access != PASSIVE_ALL_EPOCH) {
                return MPC_LOWCOMM_ERROR;
        } 

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        rc = mpc_osc_perform_flush_op(module, task, NULL, NULL);
        if (rc != MPC_LOWCOMM_SUCCESS) {
                goto err;
        }

        if (module->lock_all_is_no_check == 0) {
                for (i = 0; i < win->comm_size; i++) {
                        rc |= end_shared(win, task, i);
                }
                module->lock_all_is_no_check = 0;
        } else {
                module->lock_all_is_no_check = 1;
        }

        module->epoch.access = NONE_EPOCH;

err:
        return rc;
}

int mpc_osc_flush(int target, mpc_win_t *win)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        lcp_ep_h ep;
        lcp_task_h task = NULL;
        
        if (module->epoch.access != PASSIVE_EPOCH &&
            module->epoch.access != PASSIVE_ALL_EPOCH) {
                return MPC_LOWCOMM_ERROR;
        }

        ep = module->eps[target];

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        rc = mpc_osc_perform_flush_op(module, task, ep, NULL);

        return rc;
}

int mpc_osc_flush_all(mpc_win_t *win)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        mpc_osc_module_t *module = &win->win_module;
        lcp_task_h task = NULL;
        
        if (module->epoch.access != PASSIVE_EPOCH &&
            module->epoch.access != PASSIVE_ALL_EPOCH) {
                return MPC_LOWCOMM_ERROR;
        }

        task = lcp_context_task_get(module->ctx, mpc_common_get_task_rank());

        rc = mpc_osc_perform_flush_op(module, task, NULL, NULL);

        return rc;
}

int mpc_osc_flush_local(int target, mpc_win_t *win)
{
        return mpc_osc_flush(target, win);
}

int mpc_osc_flush_local_all(mpc_win_t *win)
{
        return mpc_osc_flush_all(win);
}