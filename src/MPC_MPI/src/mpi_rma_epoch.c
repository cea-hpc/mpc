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

#include "mpi_rma_epoch.h"

#include "datatype.h"
#include "mpc_mpi_internal.h"
#include "comm_lib.h"

#include "sctk_low_level_comm.h"
#include <string.h>

#include "mpi_rma_ctrl_msg.h"
#include "mpi_rma_win.h"

/************************************************************************/
/* MPI Window free List                                                 */
/************************************************************************/

void mpc_MPI_Win_tmp_init(struct mpc_MPI_Win_tmp *tmp) {
  tmp->head = NULL;
  mpc_common_spinlock_init(&tmp->lock, 0);
}

void mpc_MPI_Win_tmp_purge(struct mpc_MPI_Win_tmp *tmp) {
  mpc_common_spinlock_lock_yield(&tmp->lock);

  struct mpc_MPI_Win_tmp_buff *curr = tmp->head;
  struct mpc_MPI_Win_tmp_buff *to_free = NULL;

  while (curr) {
    to_free = curr;
    curr = curr->next;
    mpc_common_nodebug("Freeing %p size %ld", to_free->buff, to_free->size);
    sctk_free(to_free->buff);
    to_free->buff = NULL;
    sctk_free(to_free);
  }

  tmp->head = NULL;

  mpc_common_spinlock_unlock(&tmp->lock);
}

void mpc_MPI_Win_tmp_release(struct mpc_MPI_Win_tmp *tmp) {
  mpc_MPI_Win_tmp_purge(tmp);
}

int mpc_MPI_Win_tmp_register(struct mpc_MPI_Win_tmp *tmp, void *ptr,
                             size_t size) {
  mpc_common_spinlock_lock_yield(&tmp->lock);

  struct mpc_MPI_Win_tmp_buff *pbuff =
      sctk_malloc(sizeof(struct mpc_MPI_Win_tmp_buff));

  if (!pbuff) {
    perror("malloc");
    mpc_common_spinlock_unlock(&tmp->lock);
    return -1;
  }

  pbuff->buff = ptr;

  if (!pbuff->buff) {
    mpc_common_spinlock_unlock(&tmp->lock);
    return -1;
  }

  pbuff->size = size;

  mpc_common_nodebug("Registering %p size %ld", pbuff->buff, pbuff->size);
  pbuff->next = tmp->head;
  tmp->head = pbuff;

  mpc_common_spinlock_unlock(&tmp->lock);

  return 0;
}

void *mpc_MPI_Win_tmp_alloc(struct mpc_MPI_Win_tmp *tmp, size_t size) {
  void *ret = NULL;

  ret = sctk_malloc(size);

  if (!ret) {
    perror("malloc");
    return NULL;
  }

  mpc_MPI_Win_tmp_register(tmp, ret, size);

  return ret;
}

/************************************************************************/
/* MPI Request Counter Pool                                             */
/************************************************************************/

static struct mpc_common_hashtable __request_pool_ht;
static mpc_common_spinlock_t __request_pool_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static volatile int __request_pool_init_done = 0;

static inline void request_poll_ht_check_init() {
  if (__request_pool_init_done)
    return;

  mpc_common_spinlock_lock(&__request_pool_lock);
  if (__request_pool_init_done == 0) {
    mpc_common_hashtable_init(&__request_pool_ht, 64);
    __request_pool_init_done = 1;
  }

  mpc_common_spinlock_unlock(&__request_pool_lock);
}

int mpc_MPI_register_request_counter(mpc_lowcomm_request_t *request,
                                     OPA_int_t *pool_cnt) {
  request_poll_ht_check_init();
  mpc_common_hashtable_set(&__request_pool_ht, (uint64_t)request, (void *)pool_cnt);
  return 0;
}

int mpc_MPI_unregister_request_counter(mpc_lowcomm_request_t *request) {
  request_poll_ht_check_init();
  mpc_common_hashtable_delete(&__request_pool_ht, (uint64_t)request);
  return 0;
}

int mpc_MPI_notify_request_counter(mpc_lowcomm_request_t *request) {
  request_poll_ht_check_init();

  OPA_int_t *pool_cnt =
      (OPA_int_t *)mpc_common_hashtable_get(&__request_pool_ht, (uint64_t)request);

  if (pool_cnt) {
    OPA_incr_int(pool_cnt);
    return 1;
  }

  return 0;
}

/************************************************************************/
/* MPI Window request handling                                          */
/************************************************************************/

/* Request Array */

int mpc_MPI_Win_request_array_init(struct mpc_MPI_Win_request_array *ra,
                                   MPI_Comm comm) {
  mpc_common_spinlock_init(&ra->lock, 0);
  ra->comm = comm;
  ra->is_emulated = 0;
  ra->pending_rma = 0;

  int comm_size;
  PMPI_Comm_size(ra->comm, &comm_size);

  ra->test_start = 0;

  memset(ra->requests, 0, sizeof(mpc_lowcomm_request_t) * MAX_PENDING_RMA);

  int i;

  for (i = 0; i < MAX_PENDING_RMA; i++) {
    memset(&ra->requests[i], 0, sizeof(mpc_lowcomm_request_t));
    mpc_lowcomm_request_init(&ra->requests[i], comm, REQUEST_PICKED);
    /* Register the request counter */
    mpc_MPI_register_request_counter(&ra->requests[i], &ra->available_req);
  }

  OPA_store_int(&ra->available_req, MAX_PENDING_RMA);

  return 0;
}

int mpc_MPI_Win_request_array_release(struct mpc_MPI_Win_request_array *ra) {
  mpc_MPI_Win_request_array_fence(ra);

  int comm_size, i;
  PMPI_Comm_size(ra->comm, &comm_size);

  /* Unregister the request counter */
  for (i = 0; i < MAX_PENDING_RMA; i++) {
    mpc_MPI_unregister_request_counter(&ra->requests[i]);
  }

  return 0;
}

int mpc_MPI_Win_request_array_fence_no_ops(
    struct mpc_MPI_Win_request_array *ra) {

  if (OPA_load_int(&ra->available_req) == MAX_PENDING_RMA)
    return 0;

  while (OPA_load_int(&ra->available_req) != MAX_PENDING_RMA) {
    mpc_MPI_Win_request_array_test(ra);

    _mpc_lowcomm_multirail_notify_idle();
    mpc_thread_yield();
  }

  return 0;
}

int mpc_MPI_Win_request_array_fence(struct mpc_MPI_Win_request_array *ra) {

  mpc_MPI_Win_request_array_fence_no_ops(ra);
  mpc_thread_wait_for_value_and_poll(&ra->pending_rma, 0, NULL, NULL);

  return 0;
}

int mpc_MPI_Win_request_array_fence_no_ops_dual(
    struct mpc_MPI_Win_request_array *ra1,
    struct mpc_MPI_Win_request_array *ra2) {
  int cnt = 0;

  int req1 = OPA_load_int(&ra1->available_req);
  int req2 = OPA_load_int(&ra2->available_req);

  while ((req1 != MAX_PENDING_RMA) || (req2 != MAX_PENDING_RMA)) {

    mpc_MPI_Win_request_array_test(ra1);
    mpc_MPI_Win_request_array_test(ra2);
    cnt++;

    req1 = OPA_load_int(&ra1->available_req);
    req2 = OPA_load_int(&ra2->available_req);

    mpc_thread_yield();
  }

  return 0;
}

int mpc_MPI_Win_request_array_fence_dual(
    struct mpc_MPI_Win_request_array *ra1,
    struct mpc_MPI_Win_request_array *ra2) {

  int wait_for_ops1 = 0;
  int wait_for_ops2 = 0;

  do {
    mpc_MPI_Win_request_array_fence_no_ops_dual(ra1, ra2);

    mpc_common_spinlock_lock(&ra1->lock);
    mpc_common_spinlock_lock(&ra2->lock);
    wait_for_ops1 = ra1->pending_rma;
    wait_for_ops2 = ra2->pending_rma;
    mpc_common_spinlock_unlock(&ra2->lock);
    mpc_common_spinlock_unlock(&ra1->lock);

  } while (wait_for_ops1 || wait_for_ops2);

  return 0;
}

int mpc_MPI_Win_request_array_test(struct mpc_MPI_Win_request_array *pra) {

  struct mpc_MPI_Win_request_array *ra = pra;

  if (mpc_common_spinlock_trylock(&ra->lock) == 1) {
    return 0;
  }

  int i;
  for (i = 0; i < MAX_PENDING_RMA; i++) {

    if (ra->requests[i].request_type != REQUEST_PICKED) {
      struct mpc_lowcomm_ptp_msg_progress_s _wait;
      mpc_lowcomm_ptp_msg_wait_init(&_wait, &ra->requests[i], 0);
      mpc_lowcomm_ptp_msg_progress(&_wait);
    } else {
      if (!ra->is_emulated)
        break;
    }
  }

  mpc_common_spinlock_unlock(&ra->lock);

  return 1;
}

mpc_lowcomm_request_t *
mpc_MPI_Win_request_array_pick(struct mpc_MPI_Win_request_array *ra) {

  while (1) {

    mpc_common_spinlock_lock_yield(&ra->lock);

    if (OPA_load_int(&ra->available_req) != 0) {
      int i;
      for (i = 0; i < MAX_PENDING_RMA; i++) {
        if (ra->requests[i].completion_flag == 1) {
          OPA_decr_int(&ra->available_req);

          memset(&ra->requests[i], 0, sizeof(mpc_lowcomm_request_t));

          mpc_lowcomm_request_init(&ra->requests[i], ra->comm, REQUEST_PICKED);

          mpc_common_spinlock_unlock(&ra->lock);

          return &ra->requests[i];
        }
      }
    }

    mpc_common_spinlock_unlock(&ra->lock);

    mpc_thread_yield();
  }
}

/************************************************************************/
/* MPI Window States                                                    */
/************************************************************************/

int *mpc_Win_ctx_add_remote(int *current, int *current_count, int *new,
                            int new_count) {
  int *ret = NULL;

  assume(current_count != NULL);
  assume(new != NULL);
  assume(new_count != 0);

  int k;
  int minus_1 = 0;

  for (k = 0; k < new_count; ++k) {
    if (new[k] < 0) {
      minus_1++;
    }
  }

  if (minus_1) {
    if (minus_1 == new_count)
      return current;

    new_count -= minus_1;
  }

  ret = sctk_realloc(current, sizeof(int) * (*current_count + new_count));

  if (!ret) {
    perror("realloc");
    abort();
    return NULL;
  } else {
    int i;
    int cnt = 0;
    for (i = 0; i < new_count + minus_1; i++) {
      if (0 <= new[i]) {
        ret[*current_count + cnt] = new[i];
        cnt++;
      }
    }

    *current_count = *current_count + new_count;
  }

  return ret;
}

int mpc_Win_ctx_ack_remote(struct mpc_MPI_Win_request_array *ra,
                           int source_rank, int remote_rank) {
  assume(ra != NULL);

  char data = 0;

  mpc_lowcomm_request_t *request = mpc_MPI_Win_request_array_pick(ra);

  mpc_common_debug("ACK FROM %d to %d", source_rank, remote_rank);

  assume(0 <= remote_rank);

  mpc_lowcomm_isend_class_src(source_rank, remote_rank, &data, sizeof(char),
                               TAG_RDMA_ACK + source_rank, ra->comm,
                               MPC_LOWCOMM_RDMA_MESSAGE, request);

  return 0;
}

int mpc_Win_ctx_get_ack_remote(struct mpc_MPI_Win_request_array *ra,
                               int source_rank, int remote_rank) {
  assume(ra != NULL);
  mpc_lowcomm_request_t *request = mpc_MPI_Win_request_array_pick(ra);

  char data = 0;

  mpc_common_debug("GETACK FROM %d to %d", remote_rank, source_rank);

  assume(0 <= remote_rank);

  mpc_lowcomm_irecv_class_dest(remote_rank, source_rank, &data, sizeof(char),
                                TAG_RDMA_ACK + remote_rank, ra->comm,
                                MPC_LOWCOMM_RDMA_MESSAGE, request);

  return 0;
}

/************************************************************************/
/* Window Locking Engine                                                */
/************************************************************************/

struct mpc_MPI_win_lock_request *mpc_MPI_win_lock_request_new(int rank,
                                                              int lock_type) {
  struct mpc_MPI_win_lock_request *ret =
      sctk_malloc(sizeof(struct mpc_MPI_win_lock_request));

  if (!ret) {
    perror("malloc");
    return NULL;
  }

  ret->lock_type = lock_type;
  ret->source_rank = rank;

  ret->next = NULL;

  return ret;
}

int mpc_MPI_win_locks_push_delayed(struct mpc_MPI_win_locks *locks, int rank,
                                   int lock_type) {

  struct mpc_MPI_win_lock_request *new =
      mpc_MPI_win_lock_request_new(rank, lock_type);

  mpc_common_spinlock_lock_yield(&locks->lock);

  if (!locks->head) {
    locks->head = new;
  } else {
    struct mpc_MPI_win_lock_request *cur = locks->head;

    while (cur) {
      if (!cur->next) {
        assume(new->next == NULL);
        cur->next = new;
        break;
      }

      cur = cur->next;
    }
  }

  mpc_common_spinlock_unlock(&locks->lock);

  return 0;
}

void mpc_MPI_win_lock_request_free(struct mpc_MPI_win_lock_request *req) {
  memset(req, 0, sizeof(struct mpc_MPI_win_lock_request));
  sctk_free(req);
}

int mpc_MPI_win_locks_init(struct mpc_MPI_win_locks *locks) {
  /* To avoid refactoring errors
   * event if probability is low */
  assert(MPI_LOCK_SHARED != WIN_LOCK_UNLOCK);
  assert(MPI_LOCK_EXCLUSIVE != WIN_LOCK_UNLOCK);
  assert(MPI_LOCK_SHARED != WIN_LOCK_ALL);
  assert(MPI_LOCK_EXCLUSIVE != WIN_LOCK_ALL);

  locks->head = NULL;
  mpc_common_spinlock_init(&locks->lock, 0);

  sctk_spin_rwlock_init(&locks->win_lock);

  return 0;
}

int mpc_MPI_win_locks_release(struct mpc_MPI_win_locks *locks) {

  mpc_common_spinlock_lock_yield(&locks->lock);

  struct mpc_MPI_win_lock_request *cur = locks->head;
  struct mpc_MPI_win_lock_request *to_free = NULL;

  while (cur) {
    mpc_common_debug_warning("MPI Win : A lock from %d was not unlocked", cur->source_rank);
    to_free = cur;
    cur = cur->next;
    mpc_MPI_win_lock_request_free(to_free);
  }

  mpc_common_spinlock_unlock(&locks->lock);

  return 0;
}

int *mpc_MPI_win_locks_pop(struct mpc_MPI_win_locks *locks, int *count_popped,
                           int *mode) {
  assume(locks != NULL);
  assume(mode != NULL);
  assume(count_popped != NULL);

  int *ret = NULL;

  mpc_common_spinlock_lock_yield(&locks->lock);

  struct mpc_MPI_win_lock_request *to_free = NULL;
  struct mpc_MPI_win_lock_request *cur = locks->head;

  cur = locks->head;

  while (cur) {
    assert((cur->lock_type == MPI_LOCK_SHARED) ||
           (cur->lock_type == MPI_LOCK_EXCLUSIVE) ||
           (cur->lock_type == WIN_LOCK_UNLOCK));

    if (cur->lock_type == MPI_LOCK_SHARED) {
      /* We now stack shared locks to enter them all at once */
      ret = mpc_Win_ctx_add_remote(ret, count_popped, &cur->source_rank, 1);

      *mode = MPI_LOCK_SHARED;

      /* Pop the lock */
      to_free = cur;
      locks->head = cur->next;
      mpc_MPI_win_lock_request_free(to_free);
    }

    if (cur->lock_type == MPI_LOCK_EXCLUSIVE) {
      if (ret == NULL) {
        /* First encountered */
        ret = mpc_Win_ctx_add_remote(ret, count_popped, &cur->source_rank, 1);

        /* Pop the lock */
        to_free = cur;
        locks->head = cur->next;
        mpc_MPI_win_lock_request_free(to_free);

        *mode = MPI_LOCK_EXCLUSIVE;
      }

      /* We can only take a single exclusive lock */
      mpc_common_spinlock_unlock(&locks->lock);
      return ret;
    }

    if (cur->lock_type == WIN_LOCK_UNLOCK) {
      if (ret == NULL) {
        *count_popped = cur->source_rank;

        /* Pop the lock */
        to_free = cur;
        locks->head = cur->next;
        mpc_MPI_win_lock_request_free(to_free);

        *mode = WIN_LOCK_UNLOCK;
      }

      /* We can only take a single exclusive lock */
      mpc_common_spinlock_unlock(&locks->lock);
      return ret;
    }

    cur = locks->head;
  }

  mpc_common_spinlock_unlock(&locks->lock);

  return ret;
}

/************************************************************************/
/* MPI Window Target Context                                            */
/************************************************************************/

int mpc_Win_target_ctx_init(struct mpc_Win_target_ctx *ctx, MPI_Comm comm) {
  ctx->arity = MPC_WIN_NO_REMOTE;
  ctx->remote_ranks = NULL;
  ctx->remote_count = 0;
  ctx->state = MPC_WIN_TARGET_NONE;
  mpc_common_spinlock_init(&ctx->lock, 0);
  OPA_store_int(&ctx->ctrl_message_counter, 0);
  ctx->passive_exposure_count = 0;
  mpc_MPI_win_locks_init(&ctx->locks);
  mpc_MPI_Win_request_array_init(&ctx->requests, comm);
  mpc_MPI_Win_tmp_init(&ctx->tmp_buffs);

  sctk_spin_rwlock_init(&ctx->active_epoch_lock);

  return 0;
}

int mpc_Win_target_ctx_release(struct mpc_Win_target_ctx *ctx) {
  ctx->arity = MPC_WIN_NO_REMOTE;
  ctx->remote_ranks = NULL;
  ctx->state = MPC_WIN_TARGET_NONE;

  mpc_MPI_win_locks_release(&ctx->locks);
  mpc_MPI_Win_request_array_release(&ctx->requests);

  mpc_MPI_Win_tmp_release(&ctx->tmp_buffs);

  mpc_common_spinlock_init(&ctx->lock, 0);
  OPA_store_int(&ctx->ctrl_message_counter, 0);

  return 0;
}

int mpc_Win_target_ctx_check_for_pending_locks(MPI_Win win) {
  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(win);
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)low_win->payload;
  struct mpc_Win_target_ctx *ctx = &desc->target;
  /* Are there pending locks on the target
   * this test may directly start another exposure epoch */

  int *pending_locks = NULL;
  int pending_locks_count = 0;
  int lock_mode = -1;

  /* Note that we may pop multiple shared locks */
  pending_locks =
      mpc_MPI_win_locks_pop(&ctx->locks, &pending_locks_count, &lock_mode);

  if (pending_locks_count || (lock_mode == WIN_LOCK_UNLOCK)) {
    int target_state = MPC_WIN_TARGET_NONE;

    if (lock_mode == MPI_LOCK_EXCLUSIVE) {
      assume(pending_locks != NULL);
      assume(pending_locks_count == 1);
      target_state = MPC_WIN_TARGET_PASSIVE_EXCL;
    } else if (lock_mode == MPI_LOCK_SHARED) {

      assume(pending_locks != NULL);
      target_state = MPC_WIN_TARGET_PASSIVE_SHARED;
    } else if (lock_mode == WIN_LOCK_UNLOCK) {
      /* source_rank_is_stored_as_pending_count_for_unlock */
      int unlock_source = pending_locks_count;
      mpc_Win_target_ctx_end_exposure_no_lock(
          win, MPC_WIN_TARGET_PASSIVE_SHARED, unlock_source);
      return 1;
    } else {
      mpc_common_debug_fatal("MPI Win : unhandled lock mode");
    }

    /* In this case directly start another exposure epoch
     * it will also notify the remote waiter */
    mpc_Win_target_ctx_start_exposure_no_lock(
        win, MPC_WIN_MULTIPLE_REMOTE, pending_locks, pending_locks_count,
        target_state);

    return 1;
  }

  sctk_free(pending_locks);

  return 0;
}

int mpc_Win_target_ctx_start_exposure_no_lock(MPI_Win win, mpc_Win_arity arity,
                                              int *remotes, int remote_count,
                                              mpc_Win_target_state state) {
  /* Retrieve the MPI Desc */

  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(win);
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)low_win->payload;
  struct mpc_Win_target_ctx *ctx = &desc->target;

  /* Check state transitionning */

  /* These values are used to enter if
   * protected processings after this switch */
  int had_error = 0;

  /* Locking */
  int taking_a_lock = 0;
  int delayed_locking = 0;

  /* Active */
  int active_exposure_start = 0;

  /* Fences */
  int active_exposure_fence_start = 0;
  int active_exposure_fence_end = 0;

  /* End subprocess values */

  switch (state) {
  case MPC_WIN_TARGET_NONE:
    mpc_common_debug_error("MPI Win : you are starting a NONE exposure");
    had_error = 1;
    break;

  case MPC_WIN_TARGET_FENCE:

    assume(arity == MPC_WIN_FENCE_REMOTE);

    if (ctx->state != MPC_WIN_TARGET_NONE) {
      if (ctx->state == MPC_WIN_TARGET_FENCE) {
        /* We are in fact ending previous fence before starting current  */
        active_exposure_fence_end = 1;
        active_exposure_fence_start = 1;
      } else {
        mpc_common_debug_error("MPI Win : you cannot enter a fence when already being "
                   "exposed through another mechanism");
        had_error = 1;
      }
    } else {
      active_exposure_fence_start = 1;
    }
    break;

  case MPC_WIN_TARGET_ACTIVE:
    if (ctx->state != MPC_WIN_TARGET_NONE) {
      mpc_common_debug_error("MPI Win : you cannot enter an active target epoch when "
                 "already being exposed through another mechanism");
      had_error = 1;
    } else {
      active_exposure_start = 1;
    }
    break;

  case MPC_WIN_TARGET_PASSIVE_EXCL:
    /* Here if the window is already taken the only allowed operation
     * is another locking primitive */
    if ((ctx->state != MPC_WIN_TARGET_NONE) &&
        (ctx->state != MPC_WIN_TARGET_FENCE)) {
      if ((ctx->state == MPC_WIN_TARGET_PASSIVE_EXCL) ||
          (ctx->state == MPC_WIN_TARGET_PASSIVE_SHARED)) {
        /* Lock is already taken we cannot enter exclusive */
        delayed_locking = 1;
      } else {
        mpc_common_debug_fatal("MPI Win : you cannot lock exclusive a window which is "
                   "being manipulated through another mechanism");
        had_error = 1;
      }
    } else {
      /* First to lock take the lock */
      taking_a_lock = 1;
    }
    break;

  case MPC_WIN_TARGET_PASSIVE_SHARED: {

    int i, j;

    for (i = 0; i < ctx->remote_count; i++) {
      for (j = 0; j < remote_count; j++) {
        if (ctx->remote_ranks[i] == remotes[j]) {
          mpc_MPI_win_locks_push_delayed(&ctx->locks, remotes[j],
                                         MPI_LOCK_SHARED);
          remotes[j] = -1;
        }
      }
    }

    /* Here the window is already taken shared the only allowed operation
     * is another locking primitive */
    if ((ctx->state != MPC_WIN_TARGET_NONE) &&
        (ctx->state != MPC_WIN_TARGET_FENCE)) {
      if (ctx->state == MPC_WIN_TARGET_PASSIVE_EXCL) {
        /* Lock is already taken exclusive we cannot enter shared */
        delayed_locking = 1;
      } else if (ctx->state == MPC_WIN_TARGET_PASSIVE_SHARED) {
        /* Lock is already taken shared we can enter shared */
        taking_a_lock = 1;
      } else {
        mpc_common_debug_error("MPI Win : you cannot lock shared a window which is being "
                   "manipulated through another mechanism");
        had_error = 1;
      }
    } else {
      /* First to lock take the lock */
      taking_a_lock = 1;
    }
  } break;
  }

  assume(!((taking_a_lock == 1) && (delayed_locking == 1)));

  /* LOCKING
   * This code handles the locking operation
   * it can be either locking, delayed or not invoked
   */
  if (taking_a_lock) {
    assume((state == MPC_WIN_TARGET_PASSIVE_EXCL) ||
           (state == MPC_WIN_TARGET_PASSIVE_SHARED));
    /* Set locking state */
    ctx->state = state;

    /* Append remote note that we can have multiple remote
     * in the case of a shared lock */
    int rcount_beef = ctx->remote_count;
    ctx->remote_ranks = mpc_Win_ctx_add_remote(
        ctx->remote_ranks, &ctx->remote_count, remotes, remote_count);

    /* Increment passive epoch counter only for the new locks
     * also ingnoring -1 ones see add_remote code */
    ctx->passive_exposure_count += (ctx->remote_count - rcount_beef);

    int p;
    for (p = 0; p < remote_count; p++) {
      mpc_common_debug("Adding remote remote rank %d now has %d remotes PCOUNT %d ",
                remotes[p], ctx->remote_count, ctx->passive_exposure_count);
    }

    if (!ctx->remote_ranks) {
      had_error = 1;
    }

    if (state == MPC_WIN_TARGET_PASSIVE_SHARED) {

      int i;

      for (i = 0; i < remote_count; i++) {
        if (remotes[i] < 0)
          continue;

        mpc_common_spinlock_read_lock(&ctx->locks.win_lock);
      }
    } else if (state == MPC_WIN_TARGET_PASSIVE_EXCL) {
      mpc_common_spinlock_write_lock(&ctx->locks.win_lock);
    } else {
      not_reachable();
    }

  } else {
    if (delayed_locking) {
      assume(remote_count);
      assume(remotes != NULL);
      assume((state == MPC_WIN_TARGET_PASSIVE_EXCL) ||
             (state == MPC_WIN_TARGET_PASSIVE_SHARED));

      int lock_type;

      if (state == MPC_WIN_TARGET_PASSIVE_EXCL) {
        lock_type = MPI_LOCK_EXCLUSIVE;
      } else {
        lock_type = MPI_LOCK_SHARED;
      }

      int i;

      for (i = 0; i < remote_count; i++) {
        if (remotes[i] < 0)
          continue;

        mpc_MPI_win_locks_push_delayed(&ctx->locks, remotes[i], lock_type);
      }

      /* Now we are done the exposure was delayed */
      return 0;
    }
  }
  /* LOCKING end ==== */

  /* FENCES
   * This code may be invoked when fencing the window */

  /* First do we end a previous fence ? */
  if (active_exposure_fence_end) {
    mpc_common_debug("\t END");
    if (mpc_Win_target_ctx_end_exposure_no_lock(win, MPC_WIN_TARGET_FENCE,
                                                -1)) {
      had_error = 1;
    }
  }

  /* Do we start a fence ?
   * note that we may end AND start */
  if (active_exposure_fence_start && !had_error) {
    mpc_common_debug("\t START");
    /* Set fence state */
    ctx->state = MPC_WIN_TARGET_FENCE;
  }
  /* Fences end ==== */

  /* ACTIVE exposure
   * This code may be invoked when posting the window */
  if (active_exposure_start) {
    /* Set active exposure state */
    ctx->state = MPC_WIN_TARGET_ACTIVE;

    /* Make sure we have no previous context */
    assume(ctx->remote_ranks == NULL);
    assume(ctx->remote_count == 0);

    /* Append remote members */
    ctx->remote_ranks = mpc_Win_ctx_add_remote(
        ctx->remote_ranks, &ctx->remote_count, remotes, remote_count);
  }

  if (had_error) {
    return 1;
  }

  /* Eventually Do the exposure synchronization
   * (only on remotes involved in this call) */

  ctx->arity = arity;
  int i;
  switch (ctx->arity) {
  case MPC_WIN_NO_REMOTE:
    break;

  case MPC_WIN_SINGLE_REMOTE:
    assume(remotes != NULL);
    assume(remote_count == 1);

    if (remotes[0] == desc->comm_rank)
      break;

    if (remotes[0] < 0)
      break;

    mpc_common_debug("     >>> SEND %d to %d", desc->comm_rank, remotes[0]);

    /* Ack the single remote process */
    mpc_Win_ctx_ack_remote(&ctx->requests, desc->comm_rank, remotes[0]);
    break;

  case MPC_WIN_MULTIPLE_REMOTE:
    assume(remotes != NULL);

    /* Ack all remote processes */

    for (i = 0; i < remote_count; i++) {
      if (remotes[i] == desc->comm_rank)
        continue;

      if (remotes[i] < 0)
        continue;

      mpc_Win_ctx_ack_remote(&ctx->requests, desc->comm_rank, remotes[i]);
    }
    break;

  case MPC_WIN_FENCE_REMOTE:
    break;

  case MPC_WIN_ALL_REMOTE:
    not_implemented();
    break;
  }

  if (state != MPC_WIN_TARGET_ACTIVE) {
    mpc_MPI_Win_request_array_fence(&ctx->requests);
  }

  return 0;
}

int mpc_Win_target_ctx_start_exposure(MPI_Win win, mpc_Win_arity arity,
                                      int *remotes, int remote_count,
                                      mpc_Win_target_state state) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  struct mpc_Win_target_ctx *ctx = &desc->target;

  int ret = 0;

  mpc_Win_exposure_start();

  mpc_common_spinlock_lock_yield(&ctx->lock);

  mpc_Win_target_ctx_check_for_pending_locks(win);

  ret = mpc_Win_target_ctx_start_exposure_no_lock(win, arity, remotes,
                                                  remote_count, state);

  mpc_common_spinlock_unlock(&ctx->lock);

  return ret;
}

int mpc_Win_target_ctx_end_exposure_no_lock(MPI_Win win,
                                            mpc_Win_target_state state,
                                            int source_rank) {
  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(win);
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)low_win->payload;
  struct mpc_Win_target_ctx *ctx = &desc->target;

  /* We start by matching the closing state to the current lock type if
   * needed. Indeed the Unlock control message is MPC_WIN_TARGET_PASSIVE_EXCL
   * but we may be in shared state. As the remote unlock call does not
   * provide the lock type, we have to retrieve this from the window state
   * here by correcting to shared if it is the case */
  if (state == MPC_WIN_TARGET_PASSIVE_EXCL) {
    if (ctx->state == MPC_WIN_TARGET_PASSIVE_SHARED) {
      state = MPC_WIN_TARGET_PASSIVE_SHARED;
    }
  }

  if ((state == MPC_WIN_TARGET_PASSIVE_EXCL) ||
      (state == MPC_WIN_TARGET_PASSIVE_SHARED)) {
    ctx->arity = MPC_WIN_NO_REMOTE;
  }

  if (state == MPC_WIN_TARGET_PASSIVE_EXCL) {
    /* Make sure that this unlock is for us */
    if (source_rank != ctx->remote_ranks[0]) {
      mpc_common_debug("Delay exl unlock from %d", source_rank);
      /* Delay the unlock */
      mpc_MPI_win_locks_push_delayed(&ctx->locks, source_rank, WIN_LOCK_UNLOCK);
      return 0;
    }
  } else if (state == MPC_WIN_TARGET_PASSIVE_SHARED) {
    int found_in_current = 0;

    int i;

    for (i = 0; i < ctx->remote_count; i++) {
      mpc_common_debug("[%d == %d]", ctx->remote_ranks[i], source_rank);
      if (ctx->remote_ranks[i] == source_rank) {
        found_in_current = 1;
        break;
      }
    }

    if (!found_in_current) {
      mpc_common_debug("Delay Shared unlock from %d", source_rank);
      /* Delay the unlock */
      mpc_MPI_win_locks_push_delayed(&ctx->locks, source_rank, WIN_LOCK_UNLOCK);
      return 0;
    }
  }

  /* Now handle state transition */

  int had_error = 0;
  int last_rlock = 0;
  int clear_remote_ranks = 0;
  int check_for_pending_locks = 0;

  switch (ctx->state) {
  case MPC_WIN_TARGET_NONE:
    mpc_common_debug_fatal("MPI Win : Cannot end an exposure epoch which did not start");
    had_error = 1;
    break;

  case MPC_WIN_TARGET_ACTIVE:
    /* End an active exposure */
    ctx->state = MPC_WIN_TARGET_NONE;
    clear_remote_ranks = 1;
    break;

  case MPC_WIN_TARGET_PASSIVE_EXCL:
    assert(ctx->remote_count == 1);
    ctx->passive_exposure_count--;
    mpc_common_debug("UNLOCK %d pending", ctx->passive_exposure_count);
    if (ctx->passive_exposure_count == 0) {
      mpc_common_spinlock_write_unlock(&ctx->locks.win_lock);
      check_for_pending_locks = 1;
      clear_remote_ranks = 1;
      ctx->state = MPC_WIN_TARGET_NONE;
    }

    break;
  case MPC_WIN_TARGET_PASSIVE_SHARED:
    check_for_pending_locks = 1;
    ctx->passive_exposure_count--;
    last_rlock = mpc_common_spinlock_read_unlock(&ctx->locks.win_lock);
    mpc_common_debug("UNLOCK %d pending (unlock from %d)", ctx->passive_exposure_count,
              source_rank);
    if ((ctx->passive_exposure_count == 0) && (last_rlock == 0)) {
      clear_remote_ranks = 1;
      ctx->state = MPC_WIN_TARGET_NONE;
    }
    break;

  case MPC_WIN_TARGET_FENCE:
    ctx->state = MPC_WIN_TARGET_NONE;

    /* Purge Local buffers */
    mpc_MPI_Win_tmp_purge(&ctx->tmp_buffs);

    break;
  }

  /* Now we wait for all the pending requests of the window */
  mpc_MPI_Win_request_array_fence(&ctx->requests);

  /* First Do the exposure synchronization
   * (only on remotes involved in this call) */

  int i;

  switch (ctx->arity) {
  case MPC_WIN_NO_REMOTE:
    break;

  case MPC_WIN_SINGLE_REMOTE:
    /* Ack the single remote process */
    if (source_rank == desc->comm_rank)
      break;

    // mpc_common_debug_error("END exposure RECV from %d to %d", desc->comm_rank,
    // source_rank );

    mpc_Win_ctx_get_ack_remote(&ctx->requests, desc->comm_rank, source_rank);
    break;

  case MPC_WIN_MULTIPLE_REMOTE:
    assume(ctx->remote_ranks != NULL);

    /* Wait for an ACK from all remote processes */

    for (i = 0; i < ctx->remote_count; i++) {
      if (ctx->remote_ranks[i] == desc->comm_rank)
        continue;

      mpc_Win_ctx_get_ack_remote(&ctx->requests, desc->comm_rank,
                                 ctx->remote_ranks[i]);
    }

    break;

  case MPC_WIN_FENCE_REMOTE:
    break;

  case MPC_WIN_ALL_REMOTE:
    not_implemented();
    break;
  }

  /* Now we wait for all the pending requests of the window */
  mpc_MPI_Win_request_array_fence(&ctx->requests);

  if (clear_remote_ranks) {

    mpc_common_debug("CLEAR ranks");
    /* Remote remote */
    sctk_free(ctx->remote_ranks);
    ctx->remote_ranks = NULL;
    ctx->remote_count = 0;
  }

  if (check_for_pending_locks) {
    mpc_Win_target_ctx_check_for_pending_locks(win);
  }

  if (had_error) {
    return 1;
  }

  return 0;
}

int mpc_Win_target_ctx_end_exposure(MPI_Win win, mpc_Win_target_state state,
                                    int source_rank) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  struct mpc_Win_target_ctx *ctx = &desc->target;

  int ret = 0;

  mpc_common_spinlock_lock_yield(&ctx->lock);

  ret = mpc_Win_target_ctx_end_exposure_no_lock(win, state, source_rank);

  mpc_common_spinlock_unlock(&ctx->lock);

  mpc_Win_exposure_end();

  return ret;
}

static OPA_int_t exposure_count;

int mpc_Win_exposure_wait() {
  while (OPA_load_int(&exposure_count)) {
    mpc_thread_yield();
  }

  return 0;
}

int mpc_Win_exposure_start() {
  OPA_fetch_and_add_int(&exposure_count, 1);
  return 0;
}

int mpc_Win_exposure_end() {
  OPA_fetch_and_add_int(&exposure_count, -1);
  return 0;
}

/************************************************************************/
/* MPI Window Source Context                                            */
/************************************************************************/

int mpc_Win_source_ctx_init(struct mpc_Win_source_ctx *ctx, MPI_Comm comm) {
  ctx->arity = MPC_WIN_NO_REMOTE;
  ctx->remote_ranks = NULL;
  ctx->state = MPC_WIN_SOURCE_NONE;
  ctx->remote_count = 0;
  OPA_store_int(&ctx->stacked_acess, 0);
  mpc_common_spinlock_init(&ctx->lock, 0);
  OPA_store_int(&ctx->ctrl_message_counter, 0);

  mpc_MPI_Win_request_array_init(&ctx->requests, comm);
  mpc_MPI_Win_tmp_init(&ctx->tmp_buffs);

  return 0;
}

int mpc_Win_source_ctx_release(struct mpc_Win_source_ctx *ctx) {
  ctx->arity = MPC_WIN_NO_REMOTE;
  ctx->remote_ranks = NULL;
  ctx->state = MPC_WIN_SOURCE_NONE;
  ctx->remote_count = 0;
  mpc_common_spinlock_init(&ctx->lock, 0);
  mpc_MPI_Win_request_array_release(&ctx->requests);
  mpc_MPI_Win_tmp_release(&ctx->tmp_buffs);
  OPA_store_int(&ctx->ctrl_message_counter, 0);

  return 0;
}

int mpc_Win_source_ctx_start_access_no_lock(MPI_Win win, mpc_Win_arity arity,
                                            int *remotes, int remote_count,
                                            mpc_Win_source_state state) {
  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(win);
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)low_win->payload;
  struct mpc_Win_source_ctx *ctx = &desc->source;

  /* First check transitions */

  int had_error = 0;

  int enter_active = 0;
  int fence_end = 0;
  int fence_start = 0;
  int pasive_start = 0;

  ctx->arity = arity;

  switch (state) {
  case MPC_WIN_SOURCE_NONE:
    mpc_common_debug_error("MPI Win : Error cannot enter a none access epoch");
    had_error = 1;
    break;

  case MPC_WIN_SOURCE_ACTIVE:
    enter_active = 1;
    break;

  case MPC_WIN_SOURCE_FENCE:
    if (ctx->state != MPC_WIN_SOURCE_NONE) {
      if (ctx->state == MPC_WIN_SOURCE_FENCE) {
        fence_end = 1;
        fence_start = 1;
      } else {
        fence_start = 1;
      }
    } else {
      /* We enter a fence */
      fence_start = 1;
    }
    break;

  case MPC_WIN_SOURCE_PASIVE:

    pasive_start = 1;
    break;
  }

  if ((arity != MPC_WIN_NO_REMOTE) && !(ctx->remote_count)) {
    mpc_MPI_Win_request_array_test(&ctx->requests);
  }

  assume(!(enter_active && pasive_start));

  /* Active access epoch handling */
  if (enter_active) {
    /* Set active exposure state */
    ctx->state = MPC_WIN_SOURCE_ACTIVE;
  }

  /* Passive access epoch handling */
  if (pasive_start) {
    /* Set active exposure state */
    ctx->state = MPC_WIN_SOURCE_PASIVE;
  }

  /* Code common to active and passive */
  if (enter_active || pasive_start) {
    if ((arity != MPC_WIN_ALL_REMOTE) && (arity != MPC_WIN_NO_REMOTE)) {
      ctx->remote_ranks = mpc_Win_ctx_add_remote(
          ctx->remote_ranks, &ctx->remote_count, remotes, remote_count);

      if (ctx->remote_count != 1) {
        ctx->arity = MPC_WIN_MULTIPLE_REMOTE;
      }
    }

    OPA_incr_int(&ctx->stacked_acess);
  }

  /* Fencing handling */
  if (fence_end) {
    /* First notify the ending of the previous fence */
    if (mpc_Win_source_ctx_end_access_no_lock(win, MPC_WIN_SOURCE_FENCE, -1)) {
      had_error = 1;
    }
  }

  if (fence_start) {
    /* Now enter the fence */
    ctx->state = MPC_WIN_SOURCE_FENCE;
  }

  if (had_error) {
    return 1;
  }

  /* Now proceed to access epoch sync */

  /* Note that in passive entry we
   * always have a single remote
   * even if we move to multiple
   * remote in the unlock */
  int to_do_arity = ctx->arity;

  if (state == MPC_WIN_SOURCE_PASIVE) {
    to_do_arity = arity;
  }

  int i;

  switch (to_do_arity) {
  case MPC_WIN_NO_REMOTE:
    return 0;
    break;

  case MPC_WIN_SINGLE_REMOTE:
    /* If single remote we wait for the remote post */
    assume(remotes != NULL);
    assume(remote_count == 1);
    assume(ctx->remote_ranks != NULL);
    assume(ctx->remote_count != 0);

    if (desc->comm_rank != remotes[0]) {
      mpc_common_debug("     <<< RECV from %d to %d", remotes[0], desc->comm_rank);

      mpc_Win_ctx_get_ack_remote(&ctx->requests, desc->comm_rank, remotes[0]);
    }
    break;

  case MPC_WIN_MULTIPLE_REMOTE:
    assume(ctx->remote_ranks != NULL);

    /* Wait for an ACK from all remote processes */

    for (i = 0; i < ctx->remote_count; i++) {
      if (ctx->remote_ranks[i] == desc->comm_rank)
        continue;

      mpc_Win_ctx_get_ack_remote(&ctx->requests, desc->comm_rank,
                                 ctx->remote_ranks[i]);
    }

    break;

  case MPC_WIN_FENCE_REMOTE:

    /* Purge Local buffers */
    mpc_MPI_Win_tmp_purge(&ctx->tmp_buffs);

    break;

  case MPC_WIN_ALL_REMOTE:

    for (i = 0; i < desc->comm_size; i++) {
      if ((i == desc->comm_rank) || (desc->remote_wins[i] < 0))
        continue;

      // mpc_common_debug_error("START ALL RECV from  %d", i);

      mpc_Win_ctx_get_ack_remote(&ctx->requests, desc->comm_rank, i);
    }
    break;
  }

  mpc_MPI_Win_request_array_fence(&ctx->requests);

  return 0;
}

int mpc_Win_source_ctx_start_access(MPI_Win win, mpc_Win_arity arity,
                                    int *remotes, int remote_count,
                                    mpc_Win_source_state state) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  struct mpc_Win_source_ctx *ctx = &desc->source;

  int ret = 0;

  mpc_common_spinlock_lock_yield(&ctx->lock);

  ret = mpc_Win_source_ctx_start_access_no_lock(win, arity, remotes,
                                                remote_count, state);

  mpc_common_spinlock_unlock(&ctx->lock);

  return ret;
}

int mpc_Win_source_ctx_end_access_no_lock(MPI_Win win,
                                          mpc_Win_source_state state,
                                          int remote_rank) {
  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(win);
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)low_win->payload;
  struct mpc_Win_source_ctx *ctx = &desc->source;

  int i;

  int to_do_arity = ctx->arity;
  int single_remote_rank = -1;

  if (ctx->remote_ranks)
    single_remote_rank = ctx->remote_ranks[0];

  /* Are we processing an UNLOCK ?
   * in this case we have a single rank to notify */
  if (0 <= remote_rank) {
    assert(ctx->state == MPC_WIN_SOURCE_PASIVE);

    to_do_arity = MPC_WIN_SINGLE_REMOTE;
    single_remote_rank = remote_rank;
  }

  if ((ctx->arity != MPC_WIN_NO_REMOTE)) {
    mpc_MPI_Win_request_array_fence(&ctx->requests);
  }

  int is_passive = (state == MPC_WIN_SOURCE_PASIVE);

  if (is_passive)
    to_do_arity = MPC_WIN_NO_REMOTE;

  switch (to_do_arity) {
  case MPC_WIN_NO_REMOTE:
    break;

  case MPC_WIN_SINGLE_REMOTE:
    if (desc->comm_rank != single_remote_rank) {
      mpc_common_debug("END access send from %d to %d", desc->comm_rank,
                single_remote_rank);

      mpc_Win_ctx_ack_remote(&ctx->requests, desc->comm_rank,
                             single_remote_rank);
    }
    break;

  case MPC_WIN_MULTIPLE_REMOTE:
    assume(ctx->remote_ranks != NULL);

    /* Wait for an ACK from all remote processes */

    for (i = 0; i < ctx->remote_count; i++) {
      if (ctx->remote_ranks[i] == desc->comm_rank)
        continue;

      mpc_common_debug("Imul END access send from %d to %d", desc->comm_rank,
                ctx->remote_ranks[i]);

      mpc_Win_ctx_ack_remote(&ctx->requests, desc->comm_rank,
                             ctx->remote_ranks[i]);
    }
    break;

  case MPC_WIN_FENCE_REMOTE:
    break;

  case MPC_WIN_ALL_REMOTE:

    for (i = 0; i < desc->comm_size; i++) {
      if ((i == desc->comm_rank) || (desc->remote_wins[i] < 0))
        continue;

      // mpc_common_debug_error("END ALL SEND to  %d", i);

      mpc_Win_ctx_ack_remote(&ctx->requests, desc->comm_rank, i);
    }
    break;
  }

  /* Its now time to handle state transition */

  int had_error = 0;

  int clear_remote_ranks = 0;

  switch (ctx->state) {
  case MPC_WIN_SOURCE_NONE:
    mpc_common_debug_error("MPI Win : Cannot end an access epoch which did not start");
    had_error = 1;
    break;

  case MPC_WIN_SOURCE_ACTIVE:
    /* End an active exposure */
    ctx->state = MPC_WIN_SOURCE_NONE;
    clear_remote_ranks = 1;
    break;

  case MPC_WIN_SOURCE_PASIVE: {
    int current_access =
        OPA_fetch_and_add_int(&ctx->stacked_acess, -1) - 1;

    if (current_access == 0) {

      if ((ctx->arity != MPC_WIN_ALL_REMOTE) &&
          (ctx->arity != MPC_WIN_NO_REMOTE)) {
        ctx->remote_count--;

        if (ctx->remote_count == 0) {
          clear_remote_ranks = 1;
          ctx->state = MPC_WIN_SOURCE_NONE;
        }
      } else {
        ctx->state = MPC_WIN_SOURCE_NONE;
      }
    }
  } break;

  case MPC_WIN_SOURCE_FENCE:
    ctx->state = MPC_WIN_SOURCE_NONE;
    break;
  }

  if (clear_remote_ranks) {
    /* Remote remote */
    sctk_free(ctx->remote_ranks);
    ctx->remote_ranks = NULL;
    ctx->remote_count = 0;
  }

  if (had_error) {
    return 1;
  }

  return 0;
}

int mpc_Win_source_ctx_end_access(MPI_Win win, mpc_Win_source_state state,
                                  int remote_rank) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  struct mpc_Win_source_ctx *ctx = &desc->source;

  int ret = 0;

  mpc_common_spinlock_lock_yield(&ctx->lock);

  ret = mpc_Win_source_ctx_end_access_no_lock(win, state, remote_rank);

  mpc_common_spinlock_unlock(&ctx->lock);

  return ret;
}

/************************************************************************/
/* INTERNAL MPI Win Flush Calls                                         */
/************************************************************************/

int mpc_MPI_Win_sync(MPI_Win win) {
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  if (desc->source.requests.is_emulated == 0) {
    return MPI_SUCCESS;
  }

  /* Wait for local pending */
  mpc_MPI_Win_request_array_fence(&desc->source.requests);
  mpc_MPI_Win_request_array_fence(&desc->target.requests);

  return MPI_SUCCESS;
}

static inline int __mpc_MPI_Win_flush(int rank, MPI_Win win, int remote,
                                      int do_wait) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  /* Did we communicate with this window ? */
  if ((desc->source.requests.is_emulated == 0) || (desc->is_single_process)) {
    desc->tainted_wins[rank] = 0;
    return MPI_SUCCESS;
  }

  /* Wait for local pending */
  mpc_MPI_Win_request_array_fence(&desc->source.requests);
  mpc_MPI_Win_request_array_fence(&desc->target.requests);

  if (!mpc_lowcomm_is_remote_rank(mpc_lowcomm_communicator_world_rank_of(desc->comm, rank))) {
    desc->tainted_wins[rank] = 0;
    return MPI_SUCCESS;
  }

  if (!(desc->tainted_wins[rank] & 4)) {
    desc->tainted_wins[rank] = 0;
    return MPI_SUCCESS;
  }

  if ((rank < 0) || (desc->comm_size < rank)) {
    return MPI_ERR_ARG;
  }

  if (remote) {
    if (0 <= desc->remote_wins[rank]) {
      if (!(desc->tainted_wins[rank] & 4)) {
        desc->tainted_wins[rank] = 0;
      } else {

        struct mpc_MPI_Win_ctrl_message fm;
        mpc_MPI_Win_init_flush(&fm, win, rank);

        mpc_MPI_Win_control_message_send(win, rank, &fm);

        mpc_lowcomm_request_t _req;
        mpc_lowcomm_request_t *req = &_req;

        if (!do_wait) {
          req = mpc_MPI_Win_request_array_pick(&desc->source.requests);
        }

        static int dummy;
        mpc_lowcomm_irecv_class_dest(rank, desc->comm_rank, &dummy,
                                      sizeof(int), TAG_RDMA_FENCE, desc->comm,
                                      MPC_LOWCOMM_RDMA_MESSAGE, req);

        if (do_wait) {
          mpc_lowcomm_request_wait(req);
        }
      }
    }
  }

  if (do_wait) {
    /* Wait for local pending */
    mpc_MPI_Win_request_array_fence(&desc->source.requests);
    mpc_MPI_Win_request_array_fence(&desc->target.requests);
  }

  return MPI_SUCCESS;
}

int mpc_MPI_Win_flush(int rank, MPI_Win win) {
  return __mpc_MPI_Win_flush(rank, win, 1, 1);
}

int mpc_MPI_Win_flush_local(int rank, MPI_Win win) {
  return __mpc_MPI_Win_flush(rank, win, 0, 0);
}

int __mpc_MPI_Win_flush_all(MPI_Win win, int remote) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  mpc_MPI_Win_sync(win);

  if (remote) {

    int i;

    for (i = 0; i < desc->comm_size; i++) {
      __mpc_MPI_Win_flush(i, win, 1, 0);
    }
  }

  mpc_MPI_Win_request_array_fence(&desc->source.requests);

  return MPI_SUCCESS;
}

int mpc_MPI_Win_flush_all(MPI_Win win) {
  return __mpc_MPI_Win_flush_all(win, 1);
}

int mpc_MPI_Win_flush_local_all(MPI_Win win) {
  return __mpc_MPI_Win_flush_all(win, 0);
}

/************************************************************************/
/* INTERNAL MPI Window Synchronizations                                 */
/************************************************************************/

int mpc_Win_contexes_fence_control(MPI_Win win) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  struct mpc_Win_source_ctx *sctx = &desc->source;
  struct mpc_Win_target_ctx *tctx = &desc->target;

  int source = OPA_load_int(&sctx->ctrl_message_counter);

  long long int reduce_source = 0;

  /* Is there any control message to check ? */
  PMPI_Allreduce(&source, &reduce_source, 1, MPI_INT, MPI_SUM, desc->comm);

  if (reduce_source == 0) {
    /* There were no RMA relying on control messages
     * and incuring write dependency we are all set ! */
    return 0;
  }

  /* If we are here there were RMA we have to check the
   * target windows */

  long long int reduce_target = 0;
  int retry = 0;

  while (retry < 65536) {

    {
      _mpc_lowcomm_multirail_notify_idle();
      mpc_thread_yield();
    }

    int target = OPA_load_int(&tctx->ctrl_message_counter);

    PMPI_Allreduce(&target, &reduce_target, 1, MPI_INT, MPI_SUM, desc->comm);
    mpc_common_nodebug("REDUCE TARG %ld/%ld", reduce_target, reduce_source);

    /* All messages were matched, all OK */
    if (reduce_target == reduce_source) {
      return 0;
    }

    retry++;
  }

  mpc_common_debug_warning(
      "Someting went wrong when probing MPI Window control message counters");

  return 0;
}

int mpc_MPI_Win_fence(__UNUSED__ int assert, MPI_Win win) {

  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  /* Wait for local pending */
  mpc_MPI_Win_request_array_fence(&desc->source.requests);
  mpc_MPI_Win_request_array_fence(&desc->target.requests);

  mpc_Win_contexes_fence_control(win);

  PMPI_Barrier(desc->comm);
  //~ if( !(assert & MPI_MODE_NOPRECEDE) )
  {
    if (mpc_Win_target_ctx_start_exposure(win, MPC_WIN_FENCE_REMOTE, NULL, 0,
                                          MPC_WIN_TARGET_FENCE)) {
      return MPI_ERR_INTERN;
    }
  }

  //~ if( !(assert & MPI_MODE_NOSUCCEED) )
  {
    if (mpc_Win_source_ctx_start_access(win, MPC_WIN_FENCE_REMOTE, NULL, 0,
                                        MPC_WIN_SOURCE_FENCE)) {
      return MPI_ERR_INTERN;
    }
  }

  mpc_common_nodebug("FENCE ============================");

  return MPI_SUCCESS;
}

/************************************************************************/
/* Window Locking                                                       */
/************************************************************************/

int mpc_MPI_Win_lock(int lock_type, int rank, __UNUSED__ int assert, MPI_Win win) {
  mpc_common_nodebug("LLLLLLLLLLLLLLLLL to %d", rank);
  if (rank == MPC_PROC_NULL) {
    return MPI_SUCCESS;
  }

  int do_lock_all = 0;

  if (lock_type == WIN_LOCK_ALL) {
    do_lock_all = 1;
    lock_type = MPI_LOCK_SHARED;
  }

  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  /* Are we in the single process case
   * is so directly use read-write locks */
  if (desc->is_single_process) {
    int remote_win = mpc_MPI_win_get_remote_win(desc, rank, 0);

    if (remote_win < 0)
      return MPI_SUCCESS;

    struct mpc_MPI_Win *rdesc =
        (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(remote_win);

    if (lock_type == MPI_LOCK_EXCLUSIVE) {
      while (mpc_common_spinlock_write_trylock(&rdesc->target.locks.win_lock) != 0) {
        mpc_MPI_Win_request_array_fence(&desc->source.requests);
        mpc_MPI_Win_request_array_test(&desc->target.requests);
      }

      mpc_common_spinlock_lock_yield(&rdesc->target.lock);
      rdesc->target.state = MPC_WIN_TARGET_PASSIVE_EXCL;
      mpc_common_spinlock_unlock(&rdesc->target.lock);

      if (!do_lock_all) {
        if (mpc_Win_source_ctx_start_access(win, MPC_WIN_NO_REMOTE, NULL, 0,
                                            MPC_WIN_SOURCE_PASIVE)) {
          return MPI_ERR_INTERN;
        }
      }

      return MPI_SUCCESS;
    } else if (lock_type == MPI_LOCK_SHARED) {
      mpc_common_debug("LOCKSH %d", rank);
      while (mpc_common_spinlock_read_trylock(&rdesc->target.locks.win_lock) != 0) {
        mpc_MPI_Win_request_array_fence(&desc->source.requests);
        mpc_MPI_Win_request_array_test(&desc->target.requests);
      }

      mpc_common_spinlock_lock_yield(&rdesc->target.lock);
      rdesc->target.state = MPC_WIN_TARGET_PASSIVE_SHARED;
      mpc_common_spinlock_unlock(&rdesc->target.lock);

      if (!do_lock_all) {
        if (mpc_Win_source_ctx_start_access(win, MPC_WIN_NO_REMOTE, NULL, 0,
                                            MPC_WIN_SOURCE_PASIVE)) {
          return MPI_ERR_INTERN;
        }
      }

      return MPI_SUCCESS;
    } else {
      not_reachable();
    }
  }

  /* If we are here we now rely on the "message-based"
   * locking scheme */

  /* Notify our intention to lock */
  struct mpc_MPI_Win_ctrl_message message;

  if (mpc_MPI_Win_init_lock_message(&message, win, rank, lock_type)) {
    /* Win has no data */
    return MPI_SUCCESS;
  }

  /* Now enter the access epoch */

  mpc_MPI_Win_control_message_send(win, rank, &message);

  if (!do_lock_all) {
    if (mpc_Win_source_ctx_start_access(win, MPC_WIN_SINGLE_REMOTE, &rank, 1,
                                        MPC_WIN_SOURCE_PASIVE)) {
      return MPI_ERR_INTERN;
    }
  }

  mpc_common_nodebug("DONE LLLLLLLLLLLLLLLLL to %d", rank);

  return MPI_SUCCESS;
}

static inline int __mpc_MPI_Win_unlock(int rank, MPI_Win win,
                                       int do_unlock_all) {
  mpc_common_debug("UUUUUUUUUUUUUU to %d", rank);
  if (rank == MPC_PROC_NULL)
    return MPI_SUCCESS;

  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);
  int local_win = -1;

  /* Are we in shared ? */
  if (desc->is_single_process) {
    int remote_win = mpc_MPI_win_get_remote_win(desc, rank, 0);

    if (remote_win < 0)
      return MPI_SUCCESS;

    struct mpc_MPI_Win *rdesc =
        (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(remote_win);

    if (rdesc->target.state == MPC_WIN_TARGET_PASSIVE_EXCL) {

      mpc_common_spinlock_lock_yield(&rdesc->target.lock);

      rdesc->target.state = MPC_WIN_TARGET_NONE;

      /* Clear remote ranks */
      rdesc->target.remote_count = 0;
      sctk_free(rdesc->target.remote_ranks);
      rdesc->target.remote_ranks = NULL;

      mpc_common_spinlock_unlock(&rdesc->target.lock);
      mpc_common_spinlock_write_unlock(&rdesc->target.locks.win_lock);

      if (!do_unlock_all) {
        if (mpc_Win_source_ctx_end_access(win, MPC_WIN_SOURCE_PASIVE, -1)) {
          mpc_common_debug_error("Could not end access epoch");
          return MPI_ERR_INTERN;
        }
      }

      goto UNPD;
    } else if (rdesc->target.state == MPC_WIN_TARGET_PASSIVE_SHARED) {
      int rlv = mpc_common_spinlock_read_unlock(&rdesc->target.locks.win_lock);
      mpc_common_debug("UNLOCK SH %d @ %d", rank, rlv);

      if (rlv == 0) {
        mpc_common_spinlock_lock_yield(&rdesc->target.lock);
        if (rdesc->target.passive_exposure_count == 0) {
          rdesc->target.state = MPC_WIN_TARGET_NONE;

          /* Clear remote ranks */
          rdesc->target.remote_count = 0;
          sctk_free(rdesc->target.remote_ranks);
          rdesc->target.remote_ranks = 0;
        }
        mpc_common_spinlock_unlock(&rdesc->target.lock);
      }

      if (!do_unlock_all) {
        if (mpc_Win_source_ctx_end_access(win, MPC_WIN_SOURCE_PASIVE, -1)) {
          mpc_common_debug_error("Could not end access epoch");
          return MPI_ERR_INTERN;
        }
      }

      goto UNPD;
    } else {
      mpc_common_debug_error("CTX is %d", rdesc->target.state);
      not_reachable();
    }
  }

  local_win = mpc_MPI_win_get_remote_win(desc, rank, 0);

  if (local_win < 0) {
    /* Remote has no data */
    return MPI_SUCCESS;
  }

  if (!do_unlock_all) {
    /* Now end the access epoch */
    if (mpc_Win_source_ctx_end_access(win, MPC_WIN_SOURCE_PASIVE, rank)) {
      mpc_common_debug_error("Could not end access epoch");
      return MPI_ERR_INTERN;
    }
  }

  /* Notify our intention to unlock */
  struct mpc_MPI_Win_ctrl_message message;
  if (mpc_MPI_Win_init_unlock_message(&message, win, rank)) {
    /* If we are here the remote has no data */
    return MPI_SUCCESS;
  }

  mpc_MPI_Win_control_message_send(win, rank, &message);

  if (!do_unlock_all) {
    mpc_MPI_Win_request_array_fence(&desc->source.requests);
    mpc_MPI_Win_request_array_test(&desc->target.requests);
  }

UNPD:
  if (mpc_common_spinlock_trylock(&desc->target.lock) == 0) {
    mpc_Win_target_ctx_check_for_pending_locks(win);
    mpc_common_spinlock_unlock(&desc->target.lock);
  }

  mpc_common_debug("DONE UUUUUUUUUUUUUU to %d", rank);
  return MPI_SUCCESS;
}

int mpc_MPI_Win_unlock(int rank, MPI_Win win) {
  return __mpc_MPI_Win_unlock(rank, win, 0);
}

int mpc_MPI_Win_lock_all(__UNUSED__ int assert, MPI_Win win) {

  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  int i;

  for (i = desc->comm_rank; i < (desc->comm_size + desc->comm_rank); i++) {
    int targ = i % desc->comm_size;
    mpc_MPI_Win_lock(WIN_LOCK_ALL, targ, 0, win);
  }

  int my_arity = MPC_WIN_ALL_REMOTE;

  if (desc->is_single_process) {
    my_arity = MPC_WIN_NO_REMOTE;
  }

  mpc_Win_source_ctx_start_access(win, my_arity, NULL, 0,
                                  MPC_WIN_SOURCE_PASIVE);

  return MPI_SUCCESS;
}

int mpc_MPI_Win_unlock_all(MPI_Win win) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  mpc_Win_source_ctx_end_access(win, MPC_WIN_SOURCE_PASIVE, -1);

  int i;
  for (i = desc->comm_rank; i < (desc->comm_size + desc->comm_rank); i++) {
    int targ = i % desc->comm_size;
    __mpc_MPI_Win_unlock(targ, win, 1);
  }

  // mpc_MPI_Win_flush_all( win );

  return MPI_SUCCESS;
}

/************************************************************************/
/* Active Target Sync                                                   */
/************************************************************************/

int mpc_MPI_Win_post(MPI_Group group, __UNUSED__ int assert, MPI_Win win) {
  int group_size;
  int *ranks = NULL;

  PMPI_Group_size(group, &group_size);

  if (!group_size)
    return MPI_SUCCESS;

  ranks = mpc_lowcomm_group_world_ranks(group);

  if (mpc_Win_target_ctx_start_exposure(win, MPC_WIN_MULTIPLE_REMOTE, ranks,
                                        group_size, MPC_WIN_TARGET_ACTIVE)) {
    sctk_free(ranks);
    return MPI_ERR_INTERN;
  }

  sctk_free(ranks);
  return MPI_SUCCESS;
}

int mpc_MPI_Win_wait(MPI_Win win) {

  if (mpc_Win_target_ctx_end_exposure(win, MPC_WIN_TARGET_ACTIVE, -1)) {
    return MPI_ERR_INTERN;
  }

  return MPI_SUCCESS;
}

int mpc_MPI_Win_test(MPI_Win win, int *flag) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  int a = mpc_MPI_Win_request_array_test(&desc->source.requests);
  int b = mpc_MPI_Win_request_array_test(&desc->target.requests);

  *flag = a | b;

  return MPI_SUCCESS;
}

int mpc_MPI_Win_start(MPI_Group group, __UNUSED__ int assert, MPI_Win win) {
  int group_size;
  int *ranks = NULL;

  PMPI_Group_size(group, &group_size);

  if (!group_size)
    return MPI_SUCCESS;

  ranks = mpc_lowcomm_group_world_ranks(group);

  if (mpc_Win_source_ctx_start_access(win, MPC_WIN_MULTIPLE_REMOTE, ranks,
                                      group_size, MPC_WIN_SOURCE_ACTIVE)) {
    sctk_free(ranks);
    return MPI_ERR_INTERN;
  }

  sctk_free(ranks);
  return MPI_SUCCESS;
}

int mpc_MPI_Win_complete(MPI_Win win) {

  if (mpc_Win_source_ctx_end_access(win, MPC_WIN_SOURCE_ACTIVE, -1)) {
    return MPI_ERR_INTERN;
  }

  return MPI_SUCCESS;
}
