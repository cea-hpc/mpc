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

#include "mpc_lowcomm_types.h"
#include <list.h>
#include <mpc_lowcomm_communicator.h> //NOTE: for lowcomm_communicator_t
#include <mpc_common_datastructure.h>
#include <lcp.h>

#define OSC_POST_PEER_MAX 32

#define OSC_LOCK_EXCLUSIVE 0x0000000100000000ull

#define OSC_STATE_COMPLETION_COUNTER_OFFSET 0 
#define OSC_STATE_POST_COUNTER_OFFSET sizeof(uint64_t)
#define OSC_STATE_POST_OFFSET OSC_STATE_POST_COUNTER_OFFSET + sizeof(uint64_t)
#define OSC_STATE_LOCAL_LOCK_OFFSET OSC_STATE_POST_OFFSET + OSC_POST_PEER_MAX * sizeof(uint64_t)
#define OSC_STATE_GLOBAL_LOCK_OFFSET OSC_STATE_LOCAL_LOCK_OFFSET + sizeof(uint64_t)

typedef enum mpc_osc_epoch {
        NONE_EPOCH,
        FENCE_EPOCH,
        POST_WAIT_EPOCH,
        START_COMPLETE_EPOCH,
        PASSIVE_EPOCH,
        PASSIVE_ALL_EPOCH,
} mpc_osc_epoch_t;

typedef struct mpc_osc_epoch_type {
        mpc_osc_epoch_t access;
        mpc_osc_epoch_t exposure;
} mpc_osc_epoch_type_t;

typedef struct mpc_osc_pending_post {
        int rank;
        mpc_list_elem_t elem;
} mpc_osc_pending_post_t;

typedef struct mpc_osc_module_state {
        volatile uint64_t completion_counter;
        volatile uint64_t post_counter;
        volatile uint64_t post_state[OSC_POST_PEER_MAX]; /* Depends on the group size. */
        volatile uint64_t local_lock;
        volatile uint64_t global_lock;
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
        void                       **base_rdata;
        void                       **base_rstate;
        lcp_mem_h                   *rkeys_data;
        lcp_mem_h                   *rkeys_state;

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
        int                          leader;
} mpc_osc_module_t;

static inline int mpc_osc_perform_atomic_op(mpc_osc_module_t *mod, lcp_ep_h ep,
                                            lcp_task_h task, uint64_t value, size_t size,
                                            uint64_t *result, uint64_t remote_disp, 
                                            lcp_mem_h rkey, lcp_atomic_op_t op)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_status_ptr_t request;

        lcp_request_param_t params =  {
                .field_mask = (result != NULL ? LCP_REQUEST_REPLY_BUFFER : 0) |
                        LCP_REQUEST_NO_COMPLETE,
                .reply_buffer = result,
        };

        request = lcp_atomic_op_nb(ep, task, &value, size, 
                                   remote_disp, 
                                   rkey, 
                                   op, &params);
        if (LCP_PTR_IS_ERR(request)) {
                rc = MPC_LOWCOMM_ERROR;
                goto err;
        }

        while (lcp_request_check_status(request) != MPC_LOWCOMM_SUCCESS) {
                rc = lcp_progress(mod->mngr);
        }

        lcp_request_free(request);

err:
        return rc;
}

static inline int mpc_osc_perform_flush_op(mpc_osc_module_t *mod, lcp_task_h task, 
                                           lcp_ep_h ep, lcp_mem_h mem)
{
        int rc = MPC_LOWCOMM_SUCCESS;
        lcp_status_ptr_t request;
        lcp_request_param_t params = {
                .field_mask = ((ep != NULL ? LCP_REQUEST_USER_EPH : 0)    |
                               (mem != NULL ? LCP_REQUEST_USER_MEMH : 0)) |
                        LCP_REQUEST_NO_COMPLETE,
                .ep = ep,
                .mem = mem,
        };
        
        request = lcp_flush_nb(mod->mngr, task, &params);

        if ((rc = LCP_PTR_STATUS(request)) == MPC_LOWCOMM_IN_PROGRESS) {
                while (lcp_request_check_status(request) != MPC_LOWCOMM_SUCCESS) {
                        rc = lcp_progress(mod->mngr);
                        if (rc != MPC_LOWCOMM_SUCCESS) {
                                goto err;
                        }
                }
                rc = MPC_LOWCOMM_SUCCESS;
        } 

        lcp_request_free(request);

err:
        return rc;
}

#endif
