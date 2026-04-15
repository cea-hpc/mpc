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

#ifndef LOWCOMM_OSC_INTERNAL_H_
#define LOWCOMM_OSC_INTERNAL_H_

#include "mpc_lowcomm_datatypes.h"
#include "mpc_lowcomm_one_sided.h"
#include "lcp.h"
#include <list.h>

/** Translation table from MPC to LCP atomic datatypes */
static const lcp_atomic_dt_t mpc2lcp_atomic_dt_table[] =
{
	[(uint64_t)MPC_LOWCOMM_FLOAT]            = LCP_ATOMIC_DT_FLOAT,
	[(uint64_t)MPC_LOWCOMM_REAL]             = LCP_ATOMIC_DT_FLOAT,
	[(uint64_t)MPC_LOWCOMM_REAL4]            = LCP_ATOMIC_DT_FLOAT,
	[(uint64_t)MPC_LOWCOMM_DOUBLE]           = LCP_ATOMIC_DT_DOUBLE,
	[(uint64_t)MPC_LOWCOMM_LONG_DOUBLE]      = LCP_ATOMIC_DT_DOUBLE,
	[(uint64_t)MPC_LOWCOMM_DOUBLE_PRECISION] = LCP_ATOMIC_DT_DOUBLE,
	[(uint64_t)MPC_LOWCOMM_REAL8]            = LCP_ATOMIC_DT_DOUBLE,
	[(uint64_t)MPC_LOWCOMM_CHAR]             = LCP_ATOMIC_DT_INT8,
	[(uint64_t)MPC_LOWCOMM_SIGNED_CHAR]      = LCP_ATOMIC_DT_INT8,
	[(uint64_t)MPC_LOWCOMM_INT8_T]           = LCP_ATOMIC_DT_INT8,
	[(uint64_t)MPC_LOWCOMM_CHARACTER]        = LCP_ATOMIC_DT_INT8,
	[(uint64_t)MPC_LOWCOMM_SHORT]            = LCP_ATOMIC_DT_INT16,
	[(uint64_t)MPC_LOWCOMM_INT16_T]          = LCP_ATOMIC_DT_INT16,
	[(uint64_t)MPC_LOWCOMM_INTEGER2]         = LCP_ATOMIC_DT_INT16,
	[(uint64_t)MPC_LOWCOMM_INT]                = LCP_ATOMIC_DT_INT32,
	[(uint64_t)MPC_LOWCOMM_INT32_T]            = LCP_ATOMIC_DT_INT32,
	[(uint64_t)MPC_LOWCOMM_INTEGER]            = LCP_ATOMIC_DT_INT32,
	[(uint64_t)MPC_LOWCOMM_INTEGER4]           = LCP_ATOMIC_DT_INT32,
	[(uint64_t)MPC_LOWCOMM_LONG]               = LCP_ATOMIC_DT_INT64,
	[(uint64_t)MPC_LOWCOMM_LONG_LONG]          = LCP_ATOMIC_DT_INT64,
	[(uint64_t)MPC_LOWCOMM_LONG_LONG_INT]      = LCP_ATOMIC_DT_INT64,
	[(uint64_t)MPC_LOWCOMM_INT64_T]            = LCP_ATOMIC_DT_INT64,
	[(uint64_t)MPC_LOWCOMM_INTEGER8]           = LCP_ATOMIC_DT_INT64,
	[(uint64_t)MPC_LOWCOMM_UNSIGNED_CHAR]      = LCP_ATOMIC_DT_UINT8,
	[(uint64_t)MPC_LOWCOMM_UINT8_T]            = LCP_ATOMIC_DT_UINT8,
	[(uint64_t)MPC_LOWCOMM_UNSIGNED_SHORT]     = LCP_ATOMIC_DT_UINT16,
	[(uint64_t)MPC_LOWCOMM_UINT16_T]           = LCP_ATOMIC_DT_UINT16,
	[(uint64_t)MPC_LOWCOMM_UNSIGNED]           = LCP_ATOMIC_DT_UINT32,
	[(uint64_t)MPC_LOWCOMM_UINT32_T]           = LCP_ATOMIC_DT_UINT32,
	[(uint64_t)MPC_LOWCOMM_UNSIGNED_LONG]      = LCP_ATOMIC_DT_UINT64,
	[(uint64_t)MPC_LOWCOMM_UNSIGNED_LONG_LONG] = LCP_ATOMIC_DT_UINT64,
	[(uint64_t)MPC_LOWCOMM_UINT64_T]           = LCP_ATOMIC_DT_UINT64,
	[(uint64_t)MPC_LOWCOMM_AINT]               = LCP_ATOMIC_DT_UINT64,
};

static const lcp_atomic_op_t mpc2lcp_atomic_op_table[] =
{
	[LOWCOMM_ATOMIC_OP_FETCH_ADD] = LCP_ATOMIC_OP_ADD,   /**< Atomic addition */
	[LOWCOMM_ATOMIC_OP_FETCH_SUB] = LCP_ATOMIC_OP_SUB,   /**< Atomic subtraction */
	[LOWCOMM_ATOMIC_OP_FETCH_AND] = LCP_ATOMIC_OP_AND,   /**< Atomic bitwise and */
	[LOWCOMM_ATOMIC_OP_FETCH_OR]  = LCP_ATOMIC_OP_OR,    /**< Atomic bitwise or */
	[LOWCOMM_ATOMIC_OP_FETCH_XOR] = LCP_ATOMIC_OP_XOR,   /**< Atomic bitwise xor */
	[LOWCOMM_ATOMIC_OP_SWAP]      = LCP_ATOMIC_OP_SWAP,  /**< Atomic swap */
	[LOWCOMM_ATOMIC_OP_CSWAP]     = LCP_ATOMIC_OP_CSWAP, /**< Atomic compare and swap */
};

#define OSC_POST_PEER_MAX 32

#define OSC_LOCK_UNLOCK    0x0000000000000000ull
#define OSC_LOCK_EXCLUSIVE 0x0000000100000000ull

#define OCS_LCP_RKEY_BUF_SIZE_MAX  1024
#define OSC_DYNAMIC_WIN_ATTACH_MAX 128

#define OSC_STATE_COMPLETION_COUNTER_OFFSET 0
#define OSC_STATE_POST_COUNTER_OFFSET       sizeof(uint64_t)
#define OSC_STATE_POST_OFFSET               OSC_STATE_POST_COUNTER_OFFSET + sizeof(uint64_t)
#define OSC_STATE_ACC_LOCK_OFFSET           OSC_STATE_POST_OFFSET + OSC_POST_PEER_MAX * sizeof(uint64_t)
#define OSC_STATE_GLOBAL_LOCK_OFFSET        OSC_STATE_ACC_LOCK_OFFSET + sizeof(uint64_t)
#define OSC_STATE_REGION_LOCK_OFFSET        OSC_STATE_GLOBAL_LOCK_OFFSET + sizeof(uint64_t)
#define OSC_STATE_DYNAMIC_WIN_COUNT_OFFSET  OSC_STATE_REGION_LOCK_OFFSET + sizeof(uint64_t)
#define OSC_STATE_DYNAMIC_WIN_OFFSET        OSC_STATE_DYNAMIC_WIN_COUNT_OFFSET + sizeof(uint64_t)

typedef struct mpc_osc_win_info
{
	uint64_t  addr;
	lcp_mem_h rkey;
	int       rkey_init;
} mpc_osc_win_info_t;

typedef struct mpc_osc_pending_post
{
	int             rank;
	mpc_list_elem_t elem;
} mpc_osc_pending_post_t;

typedef struct mpc_osc_local_dynamic_win_info
{
	lcp_mem_h memh;
	int       refcnt;
} mpc_osc_local_dynamic_win_info_t;

typedef struct mpc_osc_dynamic_win
{
	uint64_t base;
	size_t   size;
	size_t   rkey_len;
	char     rkey_buf[OCS_LCP_RKEY_BUF_SIZE_MAX];
} mpc_osc_dynamic_win_t;

typedef struct mpc_osc_lock
{
	mpc_lowcomm_osc_lock_type_t type;
	int                         target;
	int                         is_no_check;
} mpc_osc_lock_t;

typedef struct mpc_osc_module_state
{
	volatile uint64_t              completion_counter;
	volatile uint64_t              post_counter;
	volatile uint64_t              post_state[OSC_POST_PEER_MAX];     /* Depends on the group size. */
	volatile uint64_t              acc_lock;
	volatile uint64_t              global_lock;
	volatile uint64_t              region_lock;
	volatile uint64_t              dynamic_win_count;
	volatile mpc_osc_dynamic_win_t dynamic_wins[OSC_DYNAMIC_WIN_ATTACH_MAX];
} mpc_osc_module_state_t;

typedef struct mpc_lowcomm_window_s
{
	lcp_manager_h                    mngr;
	lcp_context_h                    ctx;
	mpc_lowcomm_communicator_t       comm;
	lcp_ep_h *                       eps;

	void *                           base_data;
	void *                           base_state;
	lcp_mem_h                        lkey_state;
	lcp_mem_h                        lkey_data;
	mpc_osc_win_info_t *             rdata_win_info;
	mpc_osc_win_info_t *             rstate_win_info;

	mpc_osc_module_state_t *         state;
	mpc_list_elem_t                  pending_posts;
	mpc_lowcomm_group_t *            start_group;
	mpc_lowcomm_group_t *            post_group;
	int *                            start_grp_ranks;
	int                              post_count;
	struct mpc_common_hashtable      outstanding_locks;
	mpc_osc_local_dynamic_win_info_t local_dynamic_win_infos[OSC_DYNAMIC_WIN_ATTACH_MAX];

	int                              lock_all_is_no_check;
	mpc_lowcomm_window_flag_t        win_flag;
} mpc_lowcomm_window_t;

int _lowcomm_osc_perform_atomic(mpc_lowcomm_window_t *win, lcp_ep_h ep,
                                lcp_task_h task, uint64_t value, lcp_atomic_dt_t atomic_datatype,
                                uint64_t *result, uint64_t remote_addr,
                                lcp_mem_h rkey, lcp_atomic_op_t op);
int _lowcomm_osc_perform_flush(mpc_lowcomm_window_t *win, lcp_task_h task, lcp_ep_h ep, lcp_mem_h mem);

int _lowcomm_osc_get_target_ep(mpc_lowcomm_window_t *const win, const int target, lcp_ep_h *ep_p);
int _lowcomm_osc_get_remote_memory(mpc_lowcomm_window_t *const win, const lcp_ep_h ep,
                                   lcp_task_h task, const mpc_lowcomm_window_flag_t win_flavor,
                                   const int target, const uint64_t remote_addr, lcp_mem_h *rmem_p);
int _lowcomm_osc_release_remote_memory(mpc_lowcomm_window_t *const win, lcp_mem_h rmem);
int _lowcomm_osc_get_dynamic_win_info(lcp_ep_h ep, lcp_task_h task, uint64_t remote_addr,
                                      mpc_lowcomm_window_t *win, int target, lcp_mem_h *rmem_p);
int _lowcomm_osc_find_attached_region_position(mpc_osc_dynamic_win_t *regions, int min_idx, int max_idx,
                                               uint64_t base, size_t len, int *insert_idx);

int end_exclusive_lock(mpc_lowcomm_window_t *win, lcp_task_h task, uint64_t lock_offset, int target);
int start_exclusive_lock(mpc_lowcomm_window_t *win, lcp_task_h task, uint64_t lock_offset, int target);

#endif // LOWCOMM_OSC_INTERNAL_H_
