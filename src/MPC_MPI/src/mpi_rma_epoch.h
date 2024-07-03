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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef MPI_RMA_EPOCH_H
#define MPI_RMA_EPOCH_H

#include "mpc_mpi.h"
#include "mpc_common_asm.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_types.h"
#include "sctk_window.h"

/************************************************************************/
/* MPI Window free List                                                 */
/************************************************************************/

struct mpc_MPI_Win_tmp_buff {
  void *buff;
  size_t size;
  struct mpc_MPI_Win_tmp_buff *next;
};

struct mpc_MPI_Win_tmp {
  struct mpc_MPI_Win_tmp_buff *head;
  mpc_common_spinlock_t lock;
};

void mpc_MPI_Win_tmp_init(struct mpc_MPI_Win_tmp *tmp);
void mpc_MPI_Win_tmp_release(struct mpc_MPI_Win_tmp *tmp);

int mpc_MPI_Win_tmp_register(struct mpc_MPI_Win_tmp *tmp, void *ptr,
                             size_t size);
void *mpc_MPI_Win_tmp_alloc(struct mpc_MPI_Win_tmp *tmp, size_t size);
void mpc_MPI_Win_tmp_purge(struct mpc_MPI_Win_tmp *tmp);

/************************************************************************/
/* MPI Request Counter Pool                                             */
/************************************************************************/

/** This registers a request in a pool counter
 *	@arg request The request to be registered
 *	@arg pool_cnt A pointer incremented when the request is processes
 *	@return non-zero on error
 */
int mpc_MPI_register_request_counter(mpc_lowcomm_request_t *request,
                                     OPA_int_t *pool_cnt);

/** This remove a request from the counter pool
 * @arg request Request to be removed
 * @return non-zero on error
 */
int mpc_MPI_unregister_request_counter(mpc_lowcomm_request_t *request);

/** This notifies the counter (if any) that the request is completed
 * @arg Address of the request to notify
 * @return non null if notified
 */
int mpc_MPI_notify_request_counter(mpc_lowcomm_request_t *request);

/************************************************************************/
/* MPI Window request handling                                          */
/************************************************************************/

/** This macro defines the number of requests
 * to be stored in the \ref mpc_MPI_Win_request_array
 * larger values lead to more expensive polling */
#define MAX_PENDING_RMA 8

/** This is the RMA request pool implementation
 * it allows request to be tested in O(1) while
 * providing the required polling for emulated RMA */
struct mpc_MPI_Win_request_array {
  volatile int pending_rma;       /*<< The number of pending RMA operations */
  OPA_int_t available_req; /*<< The number of available requests */
  mpc_lowcomm_request_t requests[MAX_PENDING_RMA]; /*<< The MPC request array */
  MPI_Comm comm;        /*<< The communicator associated with the array */
  mpc_common_spinlock_t lock; /*<< A lock to protect the RMA array */
  int is_emulated;      /*<< Wether the window is emulated or not */
  int test_start;       /*<< Point where to start request testing (arity
                                                 see \ref
                           mpc_MPI_Win_request_array_test */
};

/*
 * NOLINTBEGIN(clang-diagnostic-unused-function):
 * False positive, static functions used elsewhere.
 */

/** Add a pending RMA operation to the request array
 * \warning This is needed for non-atomic RMA operation such as get_accumulate
 * @arg ra A pointer to an initialized request array
 */
static inline void
mpc_MPI_Win_request_array_add_pending(struct mpc_MPI_Win_request_array *ra) {
  mpc_common_spinlock_lock_yield(&ra->lock);
  ra->pending_rma++;
  mpc_common_spinlock_unlock(&ra->lock);
}

/** Remove a pending RMA operation to the request array
 * \warning This is needed for non-atomic RMA operation such as get_accumulate
 * @arg ra A pointer to an initialized request array
 */
static inline void
mpc_MPI_Win_request_array_add_done(struct mpc_MPI_Win_request_array *ra) {
  mpc_common_spinlock_lock_yield(&ra->lock);
  ra->pending_rma--;
  mpc_common_spinlock_unlock(&ra->lock);
}

/* NOLINTEND(clang-diagnostic-unused-function) */

/** Initialize an empty request array
 * @arg ra A pointer to an unitialized request-array
 * @arg comm The communicator associated with the RA
 * @return 0 on success
 */
int mpc_MPI_Win_request_array_init(struct mpc_MPI_Win_request_array *ra,
                                   MPI_Comm comm);

/** Release a request array
 * \warning this call does fence the array
 * @arg ra Pointer to an initialized request array
 */
int mpc_MPI_Win_request_array_release(struct mpc_MPI_Win_request_array *ra);

/** Pick a ready to use request from the array
 * @arg ra Pointer to an initialized request array
 * @return pointer to a ready to use request
 */
mpc_lowcomm_request_t *
mpc_MPI_Win_request_array_pick(struct mpc_MPI_Win_request_array *ra);

/** Poll the request array trying to progress requests
 * @arg ra Pointer to an initialized request array
 * @return 0 on success
 */
int mpc_MPI_Win_request_array_test(struct mpc_MPI_Win_request_array *ra);

/** Fence a request array
 *	Wait for the completion of all request do poll if needed
 *	by calling \ref mpc_MPI_Win_request_array_test
 *
 *	@arg ra Pointer to an initialized request array
 *	@return 0 on success
 */
int mpc_MPI_Win_request_array_fence(struct mpc_MPI_Win_request_array *ra);
int mpc_MPI_Win_request_array_fence_no_ops(
    struct mpc_MPI_Win_request_array *ra);

/** Fence two request array
 *	Wait for the completion of all request do poll if needed
 *	by calling \ref mpc_MPI_Win_request_array_test on two arrays
 *
 *	@arg ra1 Pointer to an initialized request array
 *	@arg ra2 Pointer to an initialized request array
 *	@return 0 on success
 */
int mpc_MPI_Win_request_array_fence_dual(struct mpc_MPI_Win_request_array *ra1,
                                         struct mpc_MPI_Win_request_array *ra2);

/************************************************************************/
/* Window Locking Engine                                                */
/************************************************************************/

#define WIN_LOCK_UNLOCK 1638
#define WIN_LOCK_ALL 1639

struct mpc_MPI_win_lock_request {
  int lock_type;
  int source_rank;
  struct mpc_MPI_win_lock_request *next;
};

struct mpc_MPI_win_lock_request *mpc_MPI_win_lock_request_new(int rank,
                                                              int lock_type);
void mpc_MPI_win_lock_request_free(struct mpc_MPI_win_lock_request *req);

struct mpc_MPI_win_locks {
  struct mpc_MPI_win_lock_request *head;
  mpc_common_spinlock_t lock;

  mpc_common_rwlock_t win_lock;
};

int mpc_MPI_win_locks_init(struct mpc_MPI_win_locks *locks);
int mpc_MPI_win_locks_release(struct mpc_MPI_win_locks *locks);

int mpc_MPI_win_locks_push_delayed(struct mpc_MPI_win_locks *locks, int rank,
                                   int lock_type);
int *mpc_MPI_win_locks_pop(struct mpc_MPI_win_locks *locks, int *count_popped,
                           int *mode);

int mpc_Win_target_ctx_check_for_pending_locks(MPI_Win win);

/************************************************************************/
/* MPI Window States                                                    */
/************************************************************************/

typedef enum {
  MPC_WIN_NO_REMOTE,
  MPC_WIN_SINGLE_REMOTE,
  MPC_WIN_MULTIPLE_REMOTE,
  MPC_WIN_FENCE_REMOTE,
  MPC_WIN_ALL_REMOTE
} mpc_Win_arity;

int *mpc_Win_ctx_add_remote(int *current, int *current_count, int *new,
                            int new_count);

int mpc_Win_ctx_ack_remote(struct mpc_MPI_Win_request_array *ra,
                           int source_rank, int remote_rank);

int mpc_Win_ctx_get_ack_remote(struct mpc_MPI_Win_request_array *ra,
                               int source_rank, int remote_rank);

/************************************************************************/
/* MPI Window Target Context                                            */
/************************************************************************/

typedef enum {
  MPC_WIN_TARGET_NONE,
  MPC_WIN_TARGET_FENCE,
  MPC_WIN_TARGET_ACTIVE,
  MPC_WIN_TARGET_PASSIVE_EXCL,
  MPC_WIN_TARGET_PASSIVE_SHARED
} mpc_Win_target_state;

struct mpc_Win_target_ctx {
  /* Remote handling */
  mpc_Win_arity arity;
  int *remote_ranks;
  int remote_count;

  int passive_exposure_count;
  /* State */
  mpc_Win_target_state state;

  /* Pending locks */
  struct mpc_MPI_win_locks locks;

  /* Requests */
  struct mpc_MPI_Win_request_array requests;

  /* TMP Buffs */
  struct mpc_MPI_Win_tmp tmp_buffs;

  /* Lock */
  mpc_common_spinlock_t lock;

  /* Counter */
  OPA_int_t ctrl_message_counter;

  /* Active epoch */
  mpc_common_rwlock_t active_epoch_lock;
};

int mpc_Win_target_ctx_init(struct mpc_Win_target_ctx *ctx, MPI_Comm comm);
int mpc_Win_target_ctx_release(struct mpc_Win_target_ctx *ctx);

int mpc_Win_target_ctx_start_exposure(MPI_Win win, mpc_Win_arity arity,
                                      int *remotes, int remote_count,
                                      mpc_Win_target_state state);
int mpc_Win_target_ctx_start_exposure_no_lock(MPI_Win win, mpc_Win_arity arity,
                                              int *remotes, int remote_count,
                                              mpc_Win_target_state state);
int mpc_Win_target_ctx_end_exposure(MPI_Win win, mpc_Win_target_state state,
                                    int source_rank);
int mpc_Win_target_ctx_end_exposure_no_lock(MPI_Win win,
                                            mpc_Win_target_state state,
                                            int source_rank);

int mpc_Win_exposure_wait();
int mpc_Win_exposure_start();
int mpc_Win_exposure_end();

/************************************************************************/
/* MPI Window Source Context                                            */
/************************************************************************/

typedef enum {
  MPC_WIN_SOURCE_NONE,
  MPC_WIN_SOURCE_FENCE,
  MPC_WIN_SOURCE_ACTIVE,
  MPC_WIN_SOURCE_PASIVE
} mpc_Win_source_state;

struct mpc_Win_source_ctx {
  /* Remote handling */
  mpc_Win_arity arity;
  int *remote_ranks;
  int remote_count;
  OPA_int_t stacked_acess;
  /* State */
  mpc_Win_source_state state;

  /* Requests */
  struct mpc_MPI_Win_request_array requests;

  /* TMP Buffs */
  struct mpc_MPI_Win_tmp tmp_buffs;

  /* Lock */
  mpc_common_spinlock_t lock;

  /* Counter */
  OPA_int_t ctrl_message_counter;

  /* Current lock type */
  int lock_type;
};

int mpc_Win_source_ctx_init(struct mpc_Win_source_ctx *ctx, MPI_Comm comm);
int mpc_Win_source_ctx_release(struct mpc_Win_source_ctx *ctx);

int mpc_Win_source_ctx_start_access(MPI_Win win, mpc_Win_arity arity,
                                    int *remotes, int remote_count,
                                    mpc_Win_source_state state);
int mpc_Win_source_ctx_start_access_no_lock(MPI_Win win, mpc_Win_arity arity,
                                            int *remotes, int remote_count,
                                            mpc_Win_source_state state);
int mpc_Win_source_ctx_end_access(MPI_Win win, mpc_Win_source_state state,
                                  int remote_rank);
int mpc_Win_source_ctx_end_access_no_lock(MPI_Win win,
                                          mpc_Win_source_state state,
                                          int remote_rank);

/************************************************************************/
/* INTERNAL MPI Win Flush Calls                                         */
/************************************************************************/

int mpc_MPI_Win_sync(MPI_Win win);

int mpc_MPI_Win_flush(int rank, MPI_Win win);

int mpc_MPI_Win_flush_local(int rank, MPI_Win win);

int mpc_MPI_Win_flush_all(MPI_Win win);

int mpc_MPI_Win_flush_local_all(MPI_Win win);

/************************************************************************/
/* INTERNAL MPI Window Synchronizations                                 */
/************************************************************************/

int mpc_MPI_Win_fence(int assert, MPI_Win win);

int mpc_Win_contexes_fence_control(MPI_Win win);

/************************************************************************/
/* Window Locking                                                       */
/************************************************************************/

int mpc_MPI_Win_lock(int, int, int, MPI_Win);

int mpc_MPI_Win_lock_all(int, MPI_Win);

int mpc_MPI_Win_unlock_all(MPI_Win win);

int mpc_MPI_Win_unlock(int, MPI_Win);

/************************************************************************/
/* Active targe Sync                                                    */
/************************************************************************/

int mpc_MPI_Win_post(MPI_Group group, int assert, MPI_Win win);

int mpc_MPI_Win_wait(MPI_Win win);

int mpc_MPI_Win_test(MPI_Win win, int *flag);

int mpc_MPI_Win_start(MPI_Group group, int assert, MPI_Win win);

int mpc_MPI_Win_complete(MPI_Win win);

#endif
