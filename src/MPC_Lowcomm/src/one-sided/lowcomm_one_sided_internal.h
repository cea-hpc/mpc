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
 * Lowcomm One sided submodule internal API.
 * This file contains shared interfaces and object between the different part of the lowcomm high level one sided API.
 */

#ifndef LOWCOMM_OSC_INTERNAL_H_
#define LOWCOMM_OSC_INTERNAL_H_

#include "mpc_lowcomm_datatypes.h"
#include "mpc_lowcomm_one_sided.h"
#include "lcp.h"
#include <list.h>

/** Debug name of the submodule */
#define MPC_MODULE "LowComm/OSC"

/**@defgroup LOWCOMM_ONE_SIDED_INTERNAL Internal LowComm one sided API
 * @{
 * @ingroup LOWCOMM_ONE_SIDED
 * Internals of the LowComm one sided communication API
 * @}
 */

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Translation table from MPC datatypes to LCP atomic datatypes
 */
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

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Translation table from MPC Atomic operations to LCP atomic operations
 */
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

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Maximum number of peers in an active target mode
 */
#define OSC_POST_PEER_MAX 32

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Special value for an unlocked window
 */
#define OSC_LOCK_UNLOCK 0x0000000000000000ull

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Special value for an exclusivity locked window
 */
#define OSC_LOCK_EXCLUSIVE 0x0000000100000000ull

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Maximum Size of a remote memory key
 */
#define OCS_LCP_RKEY_BUF_SIZE_MAX 1024

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Maximum number of memory attached to a dynamic window
 */
#define OSC_DYNAMIC_WIN_ATTACH_MAX 128

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Dynamic memory description
 */
typedef struct _lowcomm_osc_dynamic_win_s
{
	uint64_t base;                                /**< Address of the base of the buffer */
	size_t   size;                                /**< Size (in Bytes) of the memory region */
	size_t   rkey_len;                            /**< Size (in Bytes) of the remote memory key */
	char     rkey_buf[OCS_LCP_RKEY_BUF_SIZE_MAX]; /**< Remote memory key */
} _lowcomm_osc_dynamic_win_t;

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Synchronization states specific values
 */
typedef struct _lowcomm_osc_module_state_s
{
	volatile uint64_t                   completion_counter;                       /**< Number of complete received */
	volatile uint64_t                   post_counter;                             /**< Number of post received */
	volatile uint64_t                   post_state[OSC_POST_PEER_MAX];            /**< State of each post */
	volatile uint64_t                   acc_lock;                                 /**< State of the accumulate lock */
	volatile uint64_t                   global_lock;                              /**< State of the global lock */
	volatile uint64_t                   region_lock;                              /**< State of the regional lock */
	volatile uint64_t                   dynamic_win_count;                        /**< Number memory regions attached */
	volatile _lowcomm_osc_dynamic_win_t dynamic_wins[OSC_DYNAMIC_WIN_ATTACH_MAX]; /**< Memory regions */
} _lowcomm_osc_module_state_t;


/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Offset in the state part of the window of the completion counter
 */
#define OSC_STATE_COMPLETION_COUNTER_OFFSET 0

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Offset in the state part of the window of the post counter
 */
#define OSC_STATE_POST_COUNTER_OFFSET (OSC_STATE_COMPLETION_COUNTER_OFFSET + sizeof(uint64_t))

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Offset in the state part of the window of the posts
 */
#define OSC_STATE_POST_OFFSET (OSC_STATE_POST_COUNTER_OFFSET + sizeof(uint64_t))

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Offset in the state part of the window of the accumulate lock
 */
#define OSC_STATE_ACC_LOCK_OFFSET (OSC_STATE_POST_OFFSET + (OSC_POST_PEER_MAX * sizeof(uint64_t)))

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Offset in the state part of the window of the lock
 */
#define OSC_STATE_GLOBAL_LOCK_OFFSET (OSC_STATE_ACC_LOCK_OFFSET + sizeof(uint64_t))

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Offset in the state part of the window of the local lock
 */
#define OSC_STATE_REGION_LOCK_OFFSET (OSC_STATE_GLOBAL_LOCK_OFFSET + sizeof(uint64_t))

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Offset in the state part of the window of the dynamic attached memories count
 */
#define OSC_STATE_DYNAMIC_WIN_COUNT_OFFSET (OSC_STATE_REGION_LOCK_OFFSET + sizeof(uint64_t))

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Offset in the state part of the window of the dynamic attached memories array
 */
#define OSC_STATE_DYNAMIC_WIN_OFFSET (OSC_STATE_DYNAMIC_WIN_COUNT_OFFSET + sizeof(uint64_t))

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Description of a window
 */
typedef struct _lowcomm_osc_win_info_s
{
	uint64_t  addr; /**< Address of the described memory */
	lcp_mem_h rkey; /**< Remote key of the described memory */
} _lowcomm_osc_win_info_t;

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Description of a pending post operation
 */
typedef struct _lowcomm_osc_pending_post_s
{
	int             rank; /**< Rank of the source task*/
	mpc_list_elem_t elem; /**< Structure to make the pending post a queue */
} _lowcomm_osc_pending_post_t;

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Description of an attached memory region
 */
typedef struct _lowcomm_osc_local_dynamic_win_info_s
{
	lcp_mem_h    memh;   /**< Memory describer */
	volatile int refcnt; /**< Reference counting to avoid duplicates */
} _lowcomm_osc_local_dynamic_win_info_t;

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Description of a distant lock
 */
typedef struct _lowcomm_osc_lock_s
{
	bool                        is_no_check: 1;  /**< Does the lock should not really lock */
	mpc_lowcomm_osc_lock_type_t type       : 31; /**< Type of lock (see @ref mpc_lowcomm_osc_lock_type_t) */
	int                         target;          /**< Rank of the target task */
} _lowcomm_osc_lock_t;


/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * Internal structure of a LCP backed window
 */
typedef struct mpc_lowcomm_window_s
{
	// General information
	lcp_manager_h                          mngr;            /**< Communication manager */
	lcp_context_h                          ctx;             /**< Communication context */
	mpc_lowcomm_communicator_t             comm;            /**< Communicator */
	mpc_lowcomm_window_flag_t              win_flag;        /**< Window creation flag */

	// Memory related information
	void *                                 base_data;       /**< Address of the underlying user buffer */
	lcp_mem_h                              lkey_data;       /**< Local memory key of the user buffer memory */
	_lowcomm_osc_win_info_t *              rdata_win_info;  /**< Remote windows information about user buffer */

	_lowcomm_osc_module_state_t *          state;           /**< Address of the state values */
	lcp_mem_h                              lkey_state;      /**< Local memory key of the state values memory */
	_lowcomm_osc_win_info_t *              rstate_win_info; /**< Remote windows information about state */

	// Synchronization related  information
	// Passive target
	struct mpc_common_hashtable            outstanding_locks;    /**< Locks currently held */
	int                                    lock_all_is_no_check; /**< Does the lock all really lock */
	// PSCW
	int                                    post_count;           /**< Number of pending posts */
	mpc_list_elem_t                        pending_posts;        /**< Queue with the pending posts */
	mpc_lowcomm_group_t *                  start_group;          /**< Group with granted access through start */
	mpc_lowcomm_group_t *                  post_group;           /**< Group with granted exposure through post */
	int *                                  start_grp_ranks;      /**< Translation table between start group ranks */

	// Dynamic window specific
	_lowcomm_osc_local_dynamic_win_info_t *local_dynamic_win_infos;
} mpc_lowcomm_window_t;

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Perform a blocking atomic operation assessing the completion at return
 *
 * @param[in]  task            Current LCP task
 * @param[in]  ep              Endpoint to flush
 * @param[in]  value           Element to write in the distant memory
 * @param[out] result          Pointer to store the fetched value (should contain the compare element for cswap)
 * @param[in]  remote_addr     Address at the target where the operation should happen
 * @param[in]  rkey            Remote key of the target memory
 * @param[in]  atomic_datatype datatype of the element (see @ref lcp_atomic_dt_t)
 * @param[in]  op              Atomic operation to perform (see @ref lcp_atomic_op_t)
 * @param[in]  win             Window on which the communication happens
 *
 * @return                     MPC_LOWCOMM_SUCCESS upon success
 */
int _lowcomm_osc_perform_atomic(lcp_task_h task, lcp_ep_h ep, const uint64_t value, uint64_t *result,
                                const uint64_t remote_addr, const lcp_mem_h rkey,
                                const lcp_atomic_dt_t atomic_datatype, const lcp_atomic_op_t op,
                                mpc_lowcomm_window_t *win);

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Perform a blocking flush assessing all the communication are completed
 *
 * If the ep is NULL then, all endpoints will be flushed.
 * If the mem is NULL then, all memories will be flushed.
 * If both are NULL, the entire manager will be flushed (all endpoints and all memories).
 *
 * @param[in] task Current LCP task
 * @param[in] ep   Endpoint to flush
 * @param[in] mem  Memory to flush
 * @param[in] win  Window on which the flush happens
 *
 * @return         MPC_LOWCOMM_SUCCESS upon success
 */
int _lowcomm_osc_perform_flush(lcp_task_h task, lcp_ep_h ep, lcp_mem_h mem, mpc_lowcomm_window_t *win);


/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Retrieves the LCP endpoint to the target in the window
 *
 * @param[in]  target Rank of the target task
 * @param[in]  win    Window from which the endpoint should be retrieved
 * @param[out] ep_p   Pointer to store the retrieved endpoint
 */
void _lowcomm_osc_get_target_ep(const int target, mpc_lowcomm_window_t *win, lcp_ep_h *ep_p);

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Retrieves remote memory key of the target
 *
 * @param[in]  task        Current LCP task
 * @param[in]  target      Rank of the target task
 * @param[in]  remote_addr Address of the buffer in case of dynamic window
 * @param[in]  win         Window on which the communication happens
 * @param[out] rmem_p      Retrieved remote memory
 *
 * @return                 MPC_LOWCOMM_SUCCESS upon success
 */
int _lowcomm_osc_get_remote_memory(lcp_task_h task, const int target, const uint64_t remote_addr,
                                   mpc_lowcomm_window_t *const win, lcp_mem_h *rmem_p);

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Release a remote memory created with @ref _lowcomm_osc_get_remote_memory
 *
 * This function set the memory handle to NULL to avoid any misusage
 *
 * @param[in] rmem Reference on the remote memory to release
 * @param[in] win  Window associated with the retrieval
 *
 * @return         MPC_LOWCOMM_SUCCESS upon success
 */
int _lowcomm_osc_release_remote_memory(lcp_mem_h *rmem, mpc_lowcomm_window_t *const win);

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Finds the attached memory corresponding with base of size len
 *
 * This function is a dichotomic search implemented in a recursive manner
 *
 * @param[in]  regions    Array containing the dynamic memories of a window
 * @param[in]  min_idx    Start index for the search
 * @param[in]  max_idx    Stop index for the search
 * @param[in]  base       Address of the buffer associated with the memory to search
 * @param[in]  len        Size of the memory to find
 * @param[out] insert_idx Pointer to store the first free index in case the memory was not found
 *
 * @retval index of found memory
 * @retval -1 if not found
 */
int _lowcomm_osc_find_attached_region_position(const _lowcomm_osc_dynamic_win_t *regions,
                                               const int min_idx, const int max_idx,
                                               const uint64_t base, const size_t len, int *insert_idx);


/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Locks a window in exclusive mode
 *
 * @param[in] task        Current LCP Task
 * @param[in] target      Rank of the target task
 * @param[in] lock_offset Offset (in Bytes) of the lock state in the target window
 * @param[in] window      Window to lock
 *
 * @return                MPC_LOWCOMM_SUCCESS upon success
 */
int _lowcomm_osc_end_exclusive_lock(lcp_task_h task, const int target, const uint64_t lock_offset,
                                    mpc_lowcomm_window_t *window);

/**@ingroup LOWCOMM_ONE_SIDED_INTERNAL
 * @brief Unlocks a window in exclusive mode
 *
 * @param[in] task        Current LCP Task
 * @param[in] target      Rank of the target task
 * @param[in] lock_offset Offset (in Bytes) of the lock state in the target window
 * @param[in] window      Window to unlock
 *
 * @return                MPC_LOWCOMM_SUCCESS upon success
 */
int _lowcomm_osc_start_exclusive_lock(lcp_task_h task, const int target, const uint64_t lock_offset,
                                      mpc_lowcomm_window_t *window);

#endif // LOWCOMM_OSC_INTERNAL_H_
