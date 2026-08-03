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
/* #   - BEAULIEU Corentin coretin.beaulieu@cea.fr                        # */
/* #                                                                      # */
/* ######################################################################## */

/**@file
 * Lowcomm top level One Sided API
 */

#ifndef MPC_LOWCOMM_ONE_SIDED_H_
#define MPC_LOWCOMM_ONE_SIDED_H_

#include <stddef.h>
#include <stdatomic.h>

#include <mpc_common_datastructure.h>
#include <list.h>

#include "mpc_lowcomm_types.h"

/**
 * @defgroup LOWCOMM_ONE_SIDED MPC LowComm one sided communications API
 */

/**@ingroup LOWCOMM_ONE_SIDED
 * Opaque object representing a window
 */
typedef struct mpc_lowcomm_window_s mpc_lowcomm_window_t;

/*********************
 * Window management *
 *********************/

/**@ingroup LOWCOMM_ONE_SIDED
 * Window creation flags
 */
typedef enum mpc_lowcomm_window_flag_e
{
	MPC_LOWCOMM_WINDOW_FLAG_CREATE,   /**< Create with a pre existing buffer */
	MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE, /**< Create by allocating a buffer */
	MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC,  /**< Create a dynamic window */
	MPC_LOWCOMM_WINDOW_FLAG_SHARED,   /**< Create a window shared in memory (implemented) */
} mpc_lowcomm_window_flag_t;

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Creates a window for one sided communications use
 *
 * @param[in]  buffer   Underlying buffer of the window
 *                      It will be set when allocating and unused for dynamic windows
 * @param[in]  size     Size (in Bytes) of the requested window (unused for dynamic windows)
 * @param[in]  comm     Comunicator on which the one sided operations will happen
 * @param[in]  flag     Creation flag to modify the behaviour
 * @param[out] window_p Pointer to store an handle on the created window
 *
 * @retval MPC_LOWCOMM_SUCCESS on success
 * @retval an errorcode otherwise
 */
int mpc_lowcomm_window_create(void **buffer, const size_t size, mpc_lowcomm_communicator_t comm,
                              mpc_lowcomm_window_flag_t flag, mpc_lowcomm_window_t **window_p);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Frees a window
 *
 * The handle is set to NULL upon success to avoid any misusage of a released window
 *
 * @param[in] window_p Reference on the window handle
 *
 * @return             MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_window_free(mpc_lowcomm_window_t **window_p);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Attaches a buffer to a dynamic window
 *
 * @warning Attaching a buffer to any other type of window is erroneous
 *
 * @param[in] window Handle on the window on which the memory should be attached
 * @param[in] base   Address of the buffer to attach
 * @param[in] len    Size (in Bytes) of the buffer to attach
 *
 * @return           MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_window_attach(mpc_lowcomm_window_t *window, void *base, const size_t len);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Detaches a buffer from a dynamic window
 *
 * @warning Detaching a buffer to any other type of window is erroneous
 *
 * @param[in] window Handle on the window from which the memory should be detached
 * @param[in] base   Address of the buffer to detach
 *
 * @return           MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_window_detach(mpc_lowcomm_window_t *window, const void *base);


/************************
 * Basic communications *
 ************************/

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Register an non blocking put communication
 *
 * If the request is set to @ref MPC_REQUEST_NULL, the completion can be checked with synchronisations only.
 * Otherwise, the req should have been allocated used @ref mpc_lowcomm_request_alloc.
 *
 * @note Completion is assessed either by a synchronization routine or by waiting the request
 *
 * @param[in]  origin_buffer Address of the buffer to read from
 * @param[in]  size          Size (in Bytes) of the data to read
 * @param[in]  target        Rank of the target task
 * @param[in]  offset        Offset (in Bytes) in the target window to write to
 * @param[in]  window        Handle on the window to use
 * @param[out] req           Request tracking the completion of the operation
 *
 * @return                   MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_iput(const void *const origin_buffer, const int size,
                     const int target, const intptr_t offset,
                     mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *const req);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Register an non blocking get communication
 *
 * If the request is set to @ref MPC_REQUEST_NULL, the completion can be checked with synchronisations only.
 * Otherwise, the req should have been allocated used @ref mpc_lowcomm_request_alloc.
 *
 * @note Completion is assessed either by a synchronization routine or by waiting the request
 *
 * @param[in]  origin_buffer Address of the buffer to write to
 * @param[in]  size          Size (in Bytes) of the data to write
 * @param[in]  target        Rank of the target task
 * @param[in]  offset        Offset (in Bytes) in the target window to read from
 * @param[in]  window        Handle on the window to use
 * @param[out] req           Request tracking the completion of the operation
 *
 * @return                   MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_iget(void *origin_buffer, const int size,
                     const int target, const intptr_t offset,
                     mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *const req);


/*************************
 * Atomic communications *
 *************************/

/**@ingroup LOWCOMM_ONE_SIDED
 * LOWCOMM atomics operation identifiers
 */
typedef enum mpc_lowcomm_atomic_op_e
{
	LOWCOMM_ATOMIC_OP_FETCH_ADD,     /**< Atomic addition */
	LOWCOMM_ATOMIC_OP_FETCH_SUB,     /**< Atomic subtraction */
	LOWCOMM_ATOMIC_OP_FETCH_AND,     /**< Atomic bitwise and */
	LOWCOMM_ATOMIC_OP_FETCH_OR,      /**< Atomic bitwise or */
	LOWCOMM_ATOMIC_OP_FETCH_XOR,     /**< Atomic bitwise xor */
	LOWCOMM_ATOMIC_OP_SWAP,          /**< Atomic swap */
	LOWCOMM_ATOMIC_OP_CSWAP,         /**< Atomic compare and swap */
} mpc_lowcomm_atomic_op_t;

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Test if a MPC datatype is compatible with atomic operations
 *
 * @param[in] datatype Datatype to test
 *
 * @retval true if the datatype is compatible
 * @retval false if the datatype is not compatible
 */
bool mpc_lowcomm_atomic_dt_is_compatible(const mpc_lowcomm_datatype_t datatype);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Performs an distant atomic operation
 *
 * The type of operation and the argument usage is decided by the op argument.
 * For more information about the available operations see @ref mpc_lowcomm_atomic_op_t.
 *
 * @note Completion is assessed either by a synchronization routine or by waiting the request
 *
 * @param[in]  origin_buffer  Address of the buffer to read from
 * @param[in]  fetch_buffer   Address of the buffer to write to
 * @param[in]  compare_buffer Address of the buffer containing the operand for comparison (used for CSWAP)
 * @param[in]  datatype       Datatype of the element communicated
 * @param[in]  target         Rank of the target task
 * @param[in]  offset         Offset (in Bytes) in the target window to read from
 * @param[in]  op             Operation identifier (see @ref mpc_lowcomm_atomic_op_t)
 * @param[in]  window         Handle on the window to use
 * @param[out] req            Request tracking the completion of the operation
 *
 * @return                    Lowcomm errorcode
 */
int mpc_lowcomm_iatomic(const void *const origin_buffer, void *fetch_buffer, const void *const compare_buffer,
                        const mpc_lowcomm_datatype_t datatype,
                        const int target, const intptr_t offset, const mpc_lowcomm_atomic_op_t op,
                        mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *req);

/**@ingroup LOWCOMM_ONE_SIDED
 * LOWCOMM accumulate operation identifiers
 */
typedef enum mpc_lowcomm_accumulate_op_e
{
	LOWCOMM_ACCUMULATE_REPLACE,     /**< Replace the target buffer with the origin buffer */
	LOWCOMM_ACCUMULATE_MAX,         /**< Element wise max operation */
	LOWCOMM_ACCUMULATE_MIN,         /**< Element wise min operation */
	LOWCOMM_ACCUMULATE_SUM,         /**< Element wise addition operation */
	LOWCOMM_ACCUMULATE_PROD,        /**< Element wise product operation */
	LOWCOMM_ACCUMULATE_LAND,        /**< Element wise logical AND operation */
	LOWCOMM_ACCUMULATE_BAND,        /**< Element wise bitwise AND operation */
	LOWCOMM_ACCUMULATE_LOR,         /**< Element wise logical OR operation */
	LOWCOMM_ACCUMULATE_BOR,         /**< Element wise bitwise OR operation */
	LOWCOMM_ACCUMULATE_LXOR,        /**< Element wise logical XOR operation */
	LOWCOMM_ACCUMULATE_BXOR,        /**< Element wise bitwise XOR operation */
	LOWCOMM_ACCUMULATE_UNSUPPORTED, /**< Unsupported operation (used for error handling) */
} mpc_lowcomm_accumulate_op_t;

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Performs an distant accumulate operation
 *
 * The type of operation and the argument usage is decided by the op argument.
 * For more information about the available operations see @ref mpc_lowcomm_accumulate_op_t.
 *
 * @note Completion is assessed either by a synchronization routine or by waiting the request
 * @warning Depending on the backend this routine may be unsupported. In such cases, it will return
 * MPC_LOWCOMM_NOT_SUPPORTED.
 *
 * @param[in]  origin_buffer Address of the buffer to read from
 * @param[in]  fetch_buffer  Address of the buffer to write to (NULL for accumulate without get)
 * @param[in]  count         Number of elements of type datatype
 * @param[in]  datatype      Datatype of the element communicated
 * @param[in]  target        Rank of the target task
 * @param[in]  offset        Offset (in Bytes) in the target window to read from
 * @param[in]  op            Operation identifier (see @ref mpc_lowcomm_accumulate_op_t)
 * @param[in]  window        Handle on the window to use
 * @param[out] req           Request tracking the completion of the operation
 *
 * @return                   Lowcomm errorcode
 */
int mpc_lowcomm_iaccumulate(const void *const origin_buffer, void *fetch_buffer, const size_t count,
                            const mpc_lowcomm_datatype_t datatype, const int target, const intptr_t offset,
                            const mpc_lowcomm_accumulate_op_t op, mpc_lowcomm_window_t *win,
                            mpc_lowcomm_request_t *req);

/********************
 * Synchronizations *
 ********************/

/**@ingroup LOWCOMM_ONE_SIDED
 * LOWCOMM Lock types
 */
typedef enum
{
	MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE = 301, /**< Exclusive access to distant memory */
	MPC_LOWCOMM_WINDOW_LOCK_SHARED    = 302, /**< Shared access to distant memory */
	MPC_LOWCOMM_WINDOW_LOCK_ATOMIC    = 666, /**< For internal use*/
} mpc_lowcomm_osc_lock_type_t;

/**@ingroup LOWCOMM_ONE_SIDED
 * Special value to significate that a window is already locked
 * @note It do not have to be an error
 */
static const int MPC_LOWCOMM_OSC_ALREADY_LOCKED = 15990;


/* @ingroup LOWCOMM_ONE_SIDED
 * @brief Enqueues a flush operation on the specified window for the specified target
 *
 * The request provided should have been allocated using @ref mpc_lowcomm_request_alloc().
 *
 * @warning It is the user responsibility to wait for the completion of the flush operation
 *
 * @param[in]  target Rank of the target with whom to flush the communications
 *                   (MPC_ANY_SOURCE to flush all the communications of the window)
 * @param[in]  win    Window on which the flush will be performed
 * @param[out] req    Request to wait for the flush to complete.
 * @return            MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_iflush(const int target, mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *const req);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Locks a target window
 *
 * @param[in] lock        Type of lock to perform (see @mpc_lowcomm_osc_lock_type_t)
 * @param[in] target      Rank of the target task
 * @param[in] win         Handle on the window to lock
 * @param[in] is_no_check Whether to perform an actual lock or not
 *
 * @retval                MPC_LOWCOMM_SUCCESS upon success
 * @retval                MPC_LOWCOMM_OSC_ALREADY_LOCKED if the lock is already taken
 * @retval                An error code otherwise
 */
int mpc_lowcomm_osc_lock(const mpc_lowcomm_osc_lock_type_t lock, const int target,
                         mpc_lowcomm_window_t *win, const bool is_no_check);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Unlocks a target window
 *
 * @param[in] target    Rank of the target task
 * @param[in] win       Handle on the window to unlock
 * @param[in] is_atomic Whether the lock is of atomic type (special case)
 *
 * @retval                MPC_LOWCOMM_SUCCESS upon success
 * @retval                An error code otherwise
 */
int mpc_lowcomm_osc_unlock(const int target_rank, mpc_lowcomm_window_t *win, const bool is_atomic);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Locks all windows
 *
 * The definition of "all" is based on the communicator embedded in the window.
 *
 * @param[in] win         Handle on the window
 * @param[in] is_no_check Whether to perform an actual lock or not
 *
 * @return                MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_lock_all(mpc_lowcomm_window_t *win, const bool is_no_check);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Unlocks all windows
 *
 * The definition of "all" is based on the communicator embedded in the window.
 *
 * @param[in] win Handle on the window
 *
 * @return        MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_unlock_all(mpc_lowcomm_window_t *win);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Synchronises the outstanding communications with the network
 *
 * @param[in] win Handle on the window to sync
 *
 * @return        MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_sync(mpc_lowcomm_window_t *win);


/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Opens the window access of the specified group
 *
 * @param[in] group       Group to get access to
 * @param[in] win         Handle on the window
 * @param[in] is_no_check Whether to perform an actual lock or not
 *
 * @return                MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_start(mpc_lowcomm_group_t *group, mpc_lowcomm_window_t *win, const bool is_no_check);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Opens the window exposure to the specified group
 *
 * @param[in] group       Group to provide access to
 * @param[in] win         Handle on the window
 * @param[in] is_no_check Whether to perform an actual lock or not (UNUSED)
 *
 * @return                MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_post(mpc_lowcomm_group_t *group, mpc_lowcomm_window_t *win, __UNUSED__ const bool is_no_check);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Closes the access to the windows of the specified group
 *
 * @param[in] win Handle on the window
 *
 * @return        MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_complete(mpc_lowcomm_window_t *win);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Waits for outstanding communication and Closes the exposure
 *
 * @param[in] win Handle on the window
 *
 * @return        MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_wait(mpc_lowcomm_window_t *win);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Looks for outstanding communication and Closes the exposure if none are left
 *
 * @param[in]  win          Handle on the window
 * @param[out] is_completed Whether all the communications are completed and the exposure closed
 *
 * @return                  MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_test(mpc_lowcomm_window_t *win, bool *is_completed);


/********************
 * Helper Functions *
 ********************/

/**@ingroup LOWCOMM_ONE_SIDED
 * Attributes of a window that can be retrieved
 */
typedef struct mpc_lowcomm_osc_win_attr_s
{
	void *base_data; /**< Address of the underlying buffer */
} mpc_lowcomm_osc_win_attr_t;

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Completes a request given by any of the communication or synchronization routine
 *
 * This is useful to mimic a meta communication and still use the same type of request
 *
 * @param[in] status  Status on which the request is completed
 * @param[in] request Request to complete
 * @param[in] length  Length of the message mimicked, written in request for future retrieving
 *
 * @return            MPC_LOWCOMM_SUCCESS upon success
 */
int mpc_lowcomm_osc_request_complete(const int status, mpc_lowcomm_request_t *request, const size_t length);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Retrieves the communicator embedded in the window
 *
 * @param[in] win Window from which the communicator should be retrieved
 *
 * @return        A reference on the communicator
 */
mpc_lowcomm_communicator_t mpc_lowcomm_osc_win_get_comm(const mpc_lowcomm_window_t *const win);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Retrieves the group embedded in the window
 *
 * @param[in] win Window from which the group should be retrieved
 *
 * @return        A copy of the group
 */
mpc_lowcomm_group_t *mpc_lowcomm_osc_win_get_group(const mpc_lowcomm_window_t *const win);

/**@ingroup LOWCOMM_ONE_SIDED
 * @brief Retrieves the attributes of a window
 *
 * @param[in] win Window from which the attributes should be retrieved
 *
 * @return        A copy of the attributes
 */
mpc_lowcomm_osc_win_attr_t mpc_lowcomm_osc_win_get_attr(const mpc_lowcomm_window_t *const win);

#endif // MPC_LOWCOMM_ONE_SIDED_H_
