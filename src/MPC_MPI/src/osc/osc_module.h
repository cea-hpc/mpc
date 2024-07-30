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
/* #                                                                      # */
/* ######################################################################## */

#ifndef OSC_COMMON_H 
#define OSC_COMMON_H 

#include <list.h>
#include "mpc_lowcomm_communicator.h"
#include "mpc_thread_accessor.h"
#include <mpc_common_datastructure.h>
#include <lcp.h>

#define OSC_POST_PEER_MAX 32

#define OSC_LOCK_UNLOCK    0x0000000000000000ull
#define OSC_LOCK_EXCLUSIVE 0x0000000100000000ull

#define OSC_GET_DISP(_module, _target) \
        ((_module)->disp_unit < 0 ? (_module)->disp_units[_target] : (_module)->disp_unit)

#define OCS_LCP_RKEY_BUF_SIZE_MAX 1024
#define OSC_DYNAMIC_WIN_ATTACH_MAX 512

#define OSC_STATE_COMPLETION_COUNTER_OFFSET 0 
#define OSC_STATE_POST_COUNTER_OFFSET sizeof(uint64_t)
#define OSC_STATE_POST_OFFSET OSC_STATE_POST_COUNTER_OFFSET + sizeof(uint64_t)
#define OSC_STATE_ACC_LOCK_OFFSET OSC_STATE_POST_OFFSET + OSC_POST_PEER_MAX * sizeof(uint64_t)
#define OSC_STATE_GLOBAL_LOCK_OFFSET OSC_STATE_ACC_LOCK_OFFSET + sizeof(uint64_t)
#define OSC_STATE_REGION_LOCK_OFFSET OSC_STATE_GLOBAL_LOCK_OFFSET + sizeof(uint64_t)
#define OSC_STATE_DYNAMIC_WIN_COUNT_OFFSET OSC_STATE_REGION_LOCK_OFFSET + sizeof(uint64_t)
#define OSC_STATE_DYNAMIC_WIN_OFFSET OSC_STATE_DYNAMIC_WIN_COUNT_OFFSET + sizeof(uint64_t)

typedef enum mpc_osc_epoch {
        NONE_EPOCH,
        FENCE_EPOCH,
        POST_WAIT_EPOCH,
        START_COMPLETE_EPOCH,
        PASSIVE_EPOCH,
        PASSIVE_ALL_EPOCH,
} mpc_osc_epoch_t;

typedef struct mpc_osc_win_info {
        uint64_t addr;
        lcp_mem_h rkey;
        int rkey_init;
} mpc_osc_win_info_t;

typedef struct mpc_osc_epoch_type {
        mpc_osc_epoch_t access;
        mpc_osc_epoch_t exposure;
} mpc_osc_epoch_type_t;

typedef struct mpc_osc_pending_post {
        int rank;
        mpc_list_elem_t elem;
} mpc_osc_pending_post_t;

typedef struct mpc_osc_local_dynamic_win_info {
        lcp_mem_h memh;
        int refcnt;
} mpc_osc_local_dynamic_win_info_t;

typedef struct mpc_osc_dynamic_win {
        uint64_t base;
        size_t size;
        size_t rkey_len;
        char rkey_buf[OCS_LCP_RKEY_BUF_SIZE_MAX];
} mpc_osc_dynamic_win_t;

typedef struct mpc_osc_module_state {
        volatile uint64_t completion_counter;
        volatile uint64_t post_counter;
        volatile uint64_t post_state[OSC_POST_PEER_MAX]; /* Depends on the group size. */
        volatile uint64_t acc_lock;
        volatile uint64_t global_lock;
        volatile uint64_t region_lock;
        volatile uint64_t dynamic_win_count;
        volatile mpc_osc_dynamic_win_t dynamic_wins[OSC_DYNAMIC_WIN_ATTACH_MAX];
} mpc_osc_module_state_t;

typedef enum {
        LOCK_EXCLUSIVE,
        LOCK_SHARED,
} lock_type_t;

typedef struct mpc_osc_lock {
        int target;
        lock_type_t type;
        int is_no_check;
} mpc_osc_lock_t;

typedef struct mpc_osc_module {
        lcp_manager_h                mngr;
        lcp_context_h                ctx;
        lcp_ep_h                    *eps;

        void                        *base_data;
        void                        *base_state;
        lcp_mem_h                    lkey_state;
        lcp_mem_h                    lkey_data;
        mpc_osc_win_info_t          *rdata_win_info;
        mpc_osc_win_info_t          *rstate_win_info;

        mpc_osc_module_state_t      *state;
        mpc_osc_epoch_type_t         epoch;
        int                          post_count;
        mpc_list_elem_t              pending_posts;
        mpc_lowcomm_group_t         *start_group;
        mpc_lowcomm_group_t         *post_group;
        int                         *start_grp_ranks;
        int                          lock_count;
        struct mpc_common_hashtable  outstanding_locks;
        int                          lock_all_is_no_check;
        mpc_osc_local_dynamic_win_info_t local_dynamic_win_infos[OSC_DYNAMIC_WIN_ATTACH_MAX];

        int disp_unit;
        int *disp_units;

} mpc_osc_module_t;

static inline int mpc_osc_get_comm_info(mpc_osc_module_t *mod, int target,
                                        mpc_lowcomm_communicator_t comm,
                                        lcp_ep_h *ep_p)
{
        int rc = 0;
        if (mod->eps[target] == NULL) {
                uint64_t target_uid = mpc_lowcomm_communicator_uid(comm, 
                                                                   target);

                rc = lcp_ep_create(mod->mngr, &mod->eps[target], 
                                   target_uid, 0);
                if (rc != 0) {
                        mpc_common_debug_fatal("Could not create endpoint.");
                        goto err;
                }
        }

        *ep_p = mod->eps[target];
err:
        return rc;
}

int mpc_osc_perform_atomic_op(mpc_osc_module_t *mod, lcp_ep_h ep,
                              lcp_task_h task, uint64_t value, size_t size,
                              uint64_t *result, uint64_t remote_addr, 
                              lcp_mem_h rkey, lcp_atomic_op_t op);

int mpc_osc_find_attached_region_position(mpc_osc_dynamic_win_t *regions,
                                          int min_idx, int max_idx, uint64_t base,
                                          size_t len, int *insert_idx);

int mpc_osc_perform_flush_op(mpc_osc_module_t *mod, lcp_task_h task, 
                             lcp_ep_h ep, lcp_mem_h mem);

void mpc_osc_schedule_progress(lcp_manager_h mngr, volatile int *data,
                              int value);

int mpc_osc_start_exclusive(mpc_osc_module_t *mod, lcp_task_h task, 
                            uint64_t lock_offset, int target);
int mpc_osc_end_exclusive(mpc_osc_module_t *mod, lcp_task_h task, 
                          uint64_t lock_offset, int target);

#endif
