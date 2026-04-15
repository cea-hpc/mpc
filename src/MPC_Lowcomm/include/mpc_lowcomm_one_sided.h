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

#ifndef MPC_LOWCOMM_ONE_SIDED_H_
#define MPC_LOWCOMM_ONE_SIDED_H_

#include <stddef.h>

#include <mpc_common_datastructure.h>
#include <list.h>

#include "mpc_lowcomm_types.h"

/**
 * @defgroup lowcomm_one_sided MPC LowComm one sided communications API
 */


typedef struct mpc_lowcomm_window_s mpc_lowcomm_window_t;

typedef enum mpc_lowcomm_window_flag_e
{
	MPC_LOWCOMM_WINDOW_FLAG_CREATE,
	MPC_LOWCOMM_WINDOW_FLAG_ALLOCATE,
	MPC_LOWCOMM_WINDOW_FLAG_DYNAMIC,
	MPC_LOWCOMM_WINDOW_FLAG_SHARED,
} mpc_lowcomm_window_flag_t;

typedef enum
{
	MPC_LOWCOMM_WINDOW_LOCK_EXCLUSIVE = 301,
	MPC_LOWCOMM_WINDOW_LOCK_SHARED    = 302,
	MPC_LOWCOMM_WINDOW_LOCK_ATOMIC    = 666,     /**< For internal use*/
} mpc_lowcomm_osc_lock_type_t;

static const int MPC_LOWCOMM_OSC_ALREADY_LOCKED = 15990;

typedef struct mpc_lowcomm_osc_win_attr_s
{
	void *base_data;
} mpc_lowcomm_osc_win_attr_t;

/*****************************
 * WINDOW management
 *****************************/

int mpc_lowcomm_window_create(void **buffer, const size_t size,
                              mpc_lowcomm_communicator_t comm, mpc_lowcomm_window_flag_t flag,
                              mpc_lowcomm_window_t **window_p);
int mpc_lowcomm_window_free(mpc_lowcomm_window_t **window_p);
int mpc_lowcomm_window_attach(mpc_lowcomm_window_t *window, void *base, const size_t len);
int mpc_lowcomm_window_detach(mpc_lowcomm_window_t *window, const void *base);

int mpc_lowcomm_iput(const void *const origin_buffer, const int size,
                     const int target, const intptr_t offset,
                     mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *const req);

int mpc_lowcomm_iget(void *origin_buffer, const int size,
                     const int target, const intptr_t offset,
                     mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *const req);

/** LCP atomics operation identifiers */
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

bool mpc_lowcomm_atomic_dt_is_compatible(const mpc_lowcomm_datatype_t datatype);

int mpc_lowcomm_iatomic(const void *const origin_buffer, void *fetch_buffer, const void *const compare_buffer,
                        const mpc_lowcomm_datatype_t datatype,
                        const int target, const intptr_t offset, const mpc_lowcomm_atomic_op_t op,
                        mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *req);

/* @ingroup lowcomm_one_sided
 * @brief Enqueue a flush operation on the specified window for the specified target
 *
 * The request provided should have been allocated using @ref mpc_lowcomm_request_alloc().
 *
 * @warning It is the user responsibility to wait for the completion of the flush operation
 *
 * @param[in] target Rank of the target with whom to flush the communications
 *                   (MPC_ANY_RANK to flush all the communications of the window)
 * @param[in] win    Window on which the flush will be performed
 * @param[inout] req Request to wait for the flush to complete.
 * @return           error code
 */
int mpc_lowcomm_iflush(const int target, mpc_lowcomm_window_t *win, mpc_lowcomm_request_t *const req);

int mpc_lowcomm_osc_lock(const mpc_lowcomm_osc_lock_type_t lock, mpc_lowcomm_window_t *win,
                         const int target_rank, const bool is_no_check);
int mpc_lowcomm_osc_unlock(mpc_lowcomm_window_t *win, const int target_rank, const bool is_atomic);
int mpc_lowcomm_osc_lock_all(mpc_lowcomm_window_t *win, const bool is_no_check);
int mpc_lowcomm_osc_unlock_all(mpc_lowcomm_window_t *win);
int mpc_lowcomm_osc_sync(mpc_lowcomm_window_t *win);

int mpc_lowcomm_osc_start(mpc_lowcomm_group_t *group, mpc_lowcomm_window_t *win, const bool is_no_check);
int mpc_lowcomm_osc_post(mpc_lowcomm_group_t *group, mpc_lowcomm_window_t *win, __UNUSED__ const bool is_no_check);
int mpc_lowcomm_osc_complete(mpc_lowcomm_window_t *win);
int mpc_lowcomm_osc_wait(mpc_lowcomm_window_t *win);
int mpc_lowcomm_osc_test(mpc_lowcomm_window_t *win, bool *is_completed);

int mpc_lowcomm_osc_request_complete(const int status, mpc_lowcomm_request_t *request, const size_t length);


// Helper functions for internal use
mpc_lowcomm_communicator_t mpc_lowcomm_osc_win_get_comm(const mpc_lowcomm_window_t *const win);

mpc_lowcomm_group_t *mpc_lowcomm_osc_win_get_group(const mpc_lowcomm_window_t *const win);

mpc_lowcomm_osc_win_attr_t mpc_lowcomm_osc_win_get_attr(const mpc_lowcomm_window_t *const win);

#endif // MPC_LOWCOMM_ONE_SIDED_H_
