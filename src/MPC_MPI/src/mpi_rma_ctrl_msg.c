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

#include "mpi_rma_ctrl_msg.h"

#include "datatype.h"
#include "mpc_mpi_internal.h"
#include "comm_lib.h"
#include <string.h>

#include "mpi_alloc_mem.h"
#include "mpi_rma_win.h"

/************************************************************************/
/* INTERNAL Flush Shadow win                                            */
/************************************************************************/

void mpc_MPI_Win_handle_shadow_win_flush(void *data, size_t size) {
  struct mpc_MPI_Win_ctrl_message *message =
      (struct mpc_MPI_Win_ctrl_message *)data;
  void *shadow_region = data + sizeof(struct mpc_MPI_Win_ctrl_message);
  size_t shadow_size = size - sizeof(struct mpc_MPI_Win_ctrl_message);

  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(message->target_win);
  assume(low_win);


  mpc_Win_exposure_start();

  assume(shadow_size == (2 * low_win->size));

  size_t i;

  for (i = 0; i < low_win->size; i++) {
    char *source = (char *)(shadow_region + i);
    char *dest = (char *)(low_win->start_addr + i);
    char *mask = (char *)(shadow_region + low_win->size + i);

    if (*mask) {
      *dest = *source;
    }
  }

  mpc_Win_exposure_end();
}

void mpc_MPI_Win_handle_win_flush(void *data ) {
  struct mpc_MPI_Win_ctrl_message *message =
      (struct mpc_MPI_Win_ctrl_message *)data;

  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(message->target_win);
  assume(low_win);
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)low_win->payload;

  mpc_Win_exposure_start();

  int outgoing_rma = message->opt_arg1;

  int incoming_rma = 0;


  int source_cw_rank =
      mpc_lowcomm_communicator_world_rank_of(desc->comm, message->source_rank);

  incoming_rma =
      OPA_load_int(&low_win->incoming_emulated_rma[source_cw_rank]);

  int cnt = 0;
  while (incoming_rma != outgoing_rma) {

    // mpc_common_debug_error("%d == %d", outgoing_rma , incoming_rma );
    mpc_thread_yield();

    if (10 < cnt) {
      mpc_MPI_Win_request_array_test(&desc->source.requests);
      mpc_MPI_Win_request_array_test(&desc->target.requests);

    }
    incoming_rma =
        OPA_load_int(&low_win->incoming_emulated_rma[source_cw_rank]);
    cnt++;
  }
  static int dummy;

  mpc_lowcomm_request_t *request =
      mpc_MPI_Win_request_array_pick(&desc->target.requests);
  mpc_lowcomm_isend_class_src(desc->comm_rank, message->source_rank, &dummy,
                               sizeof(int), TAG_RDMA_FENCE, desc->comm,
                               MPC_LOWCOMM_RDMA_MESSAGE, request);

  mpc_MPI_Win_request_array_fence(&desc->target.requests);
  mpc_Win_exposure_end();
}

/************************************************************************/
/* INTERNAL NON-Contig data-type management                             */
/************************************************************************/

int __dummy_non_contig_val;

void mpc_MPI_Win_handle_non_contiguous_write(void *data, size_t size) {
  struct mpc_MPI_Win_ctrl_message *message =
      (struct mpc_MPI_Win_ctrl_message *)data;

  MPI_Aint target_disp = message->opt_arg1;
  size_t pack_size = message->opt_arg2;
  int target_count = message->opt_arg3;

  mpc_lowcomm_datatype_t target_type = _mpc_cl_derived_type_deserialize(
      data, size, sizeof(struct mpc_MPI_Win_ctrl_message));

  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(message->target_win);
  assume(low_win);

  /* Retrieve the MPI Desc */

  struct mpc_MPI_Win *desc =
      (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(message->target_win);
  assume(desc);

  void *pack_data = sctk_malloc(pack_size);

  if (!pack_data) {
    perror("malloc");
    abort();
  }

  void *start_addr =
      (char *)low_win->start_addr + low_win->disp_unit * target_disp;

  mpc_lowcomm_request_t req;
  mpc_lowcomm_irecv_class_dest(message->source_rank, desc->comm_rank,
                                pack_data, pack_size, TAG_RDMA_WRITE,
                                desc->comm, MPC_LOWCOMM_RDMA_MESSAGE, &req);
  mpc_lowcomm_request_wait(&req);

  size_t target_t_ext;
  _mpc_cl_type_size(target_type, &target_t_ext);

  mpc_common_nodebug(
      "UNPACK to %p in a win of size %ld starting at %p (disp here %d, remote "
      "off %d) an incoming size of %ld to a type of size %ld count %d",
      start_addr, low_win->size, low_win->start_addr, low_win->disp_unit,
      target_disp, pack_size, target_t_ext, target_count);

  int pos = 0;
  PMPI_Unpack(pack_data, pack_size, &pos, start_addr, target_count,
                          target_type, desc->comm);

  /* Now ACK */
  // mpc_lowcomm_request_t * request = mpc_MPI_Win_request_array_pick(
  // &desc->target.requests );
  // mpc_lowcomm_isend_class_src(desc->comm_rank, message->source_rank,
  // &__dummy_non_contig_val, sizeof(int), TAG_RDMA_WRITE_ACK, desc->comm ,
  // MPC_LOWCOMM_RDMA_MESSAGE,  request);

  sctk_free(pack_data);

  _mpc_cl_type_free(&target_type);

  mpc_lowcomm_rdma_window_inc_incoming(low_win, message->source_rank);
  OPA_incr_int(&desc->target.ctrl_message_counter);
}

void mpc_MPI_Win_handle_non_contiguous_read(void *data, size_t size) {
  struct mpc_MPI_Win_ctrl_message *message =
      (struct mpc_MPI_Win_ctrl_message *)data;

  MPI_Aint target_disp = message->opt_arg1;
  size_t pack_size = message->opt_arg2;
  int target_count = message->opt_arg3;

  mpc_lowcomm_datatype_t target_type = _mpc_cl_derived_type_deserialize(
      data, size, sizeof(struct mpc_MPI_Win_ctrl_message));

  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(message->target_win);
  assume(low_win);

  /* Retrieve the MPI Desc */

  struct mpc_MPI_Win *desc =
      (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(message->target_win);
  assume(desc);

  void *pack_data = mpc_MPI_Win_tmp_alloc(&desc->target.tmp_buffs, pack_size);

  if (!pack_data) {
    perror("malloc");
    abort();
  }

  void *start_addr =
      (char *)low_win->start_addr + low_win->disp_unit * target_disp;

  mpc_common_nodebug("ADDR %p LOWIN %p disp %lld target %lld", start_addr,
               low_win->start_addr, low_win->disp_unit, target_disp);

  size_t target_t_ext;
  _mpc_cl_type_size(target_type, &target_t_ext);

  mpc_common_nodebug(
      "PACK to %p in a win of size %ld starting at %p (disp here %d, remote "
      "off %d) an incoming size of %ld to a type of size %ld count %d",
      start_addr, low_win->size, low_win->start_addr, low_win->disp_unit,
      target_disp, pack_size, target_t_ext, target_count);

  int pos = 0;

  PMPI_Pack(start_addr, target_count, target_type, pack_data,
                        pack_size, &pos, desc->comm);

  mpc_lowcomm_request_t *request =
      mpc_MPI_Win_request_array_pick(&desc->target.requests);
  mpc_lowcomm_isend_class_src(desc->comm_rank, message->source_rank, pack_data,
                               pack_size, TAG_RDMA_READ, desc->comm,
                               MPC_LOWCOMM_RDMA_MESSAGE, request);

  /* Now ACK */
  //
  // mpc_lowcomm_isend_class_src(desc->comm_rank, message->source_rank,
  // &__dummy_non_contig_val, sizeof(int), TAG_RDMA_WRITE_ACK, desc->comm ,
  // MPC_LOWCOMM_RDMA_MESSAGE,  request);

  _mpc_cl_type_free(&target_type);
}

void mpc_MPI_Win_handle_non_contiguous_accumulate_send(void *data,
                                                       size_t size) {
  struct mpc_MPI_Win_ctrl_message *message =
      (struct mpc_MPI_Win_ctrl_message *)data;

  MPI_Aint target_disp = message->opt_arg1;
  size_t pack_size = message->opt_arg2;
  int target_count = message->opt_arg3;
  MPI_Datatype inner_type = message->opt_arg4;
  MPI_Op op = (MPI_Op)message->opt_arg5;

  mpc_lowcomm_datatype_t target_type = _mpc_cl_derived_type_deserialize(
      data, size, sizeof(struct mpc_MPI_Win_ctrl_message));

  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(message->target_win);
  assume(low_win);

  /* Retrieve the MPI Desc */

  struct mpc_MPI_Win *desc =
      (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(message->target_win);
  assume(desc);

  /* Allocate pack_storage (from source) */

  void *pack_data = sctk_malloc(pack_size);

  if (!pack_data) {
    perror("malloc");
    abort();
  }

  /* Receive packed data from origin */
  mpc_lowcomm_request_t req;
  mpc_lowcomm_irecv_class_dest(message->source_rank, desc->comm_rank,
                                pack_data, pack_size, TAG_RDMA_ACCUMULATE,
                                desc->comm, MPC_LOWCOMM_RDMA_MESSAGE, &req);
  mpc_lowcomm_request_wait(&req);

  /* Now pack local data using the remote data-type */

  int local_pack_size;

  if (PMPI_Pack_size(target_count, target_type, desc->comm, &local_pack_size) !=
      MPI_SUCCESS) {
    return;
  }

  void *local_pack_data = sctk_malloc(local_pack_size);

  if (!local_pack_data) {
    perror("malloc");
    abort();
  }

  void *start_addr =
      (char *)low_win->start_addr + low_win->disp_unit * target_disp;
  int pos = 0;

  mpc_MPI_accumulate_op_lock();

  PMPI_Pack(start_addr, target_count, target_type, local_pack_data,
                        local_pack_size, &pos, desc->comm);

  /* Now do the sum in the local pack
   * pack_data : contains data from source
   * local_pack_data : contains data from target */

  sctk_op_t *mpi_op = sctk_convert_to_mpc_op(op);
  sctk_Op mpc_op = mpi_op->op;
  sctk_Op_f func = sctk_get_common_function(inner_type, mpc_op);

  MPI_Count per_elem_size = 0;
  PMPI_Type_size_x(inner_type, &per_elem_size);

  size_t max_size = ((unsigned int)local_pack_size < pack_size) ? (size_t)local_pack_size : pack_size;

  func(pack_data, local_pack_data, max_size / per_elem_size, inner_type);

  /* Now that we are done put the computed data back in place */
  pos = 0;
  PMPI_Unpack(local_pack_data, local_pack_size, &pos, start_addr,
                          target_count, target_type, desc->comm);

  mpc_MPI_accumulate_op_unlock();

  sctk_free(pack_data);
  sctk_free(local_pack_data);

  _mpc_cl_type_free(&target_type);

  mpc_lowcomm_rdma_window_inc_incoming(low_win, message->source_rank);
  OPA_incr_int(&desc->target.ctrl_message_counter);
}

/************************************************************************/
/* Window Control Message Engine                                        */
/************************************************************************/

int lock_type_to_exposure_mode(int lock_type) {
  if (lock_type == MPI_LOCK_EXCLUSIVE) {
    return MPC_WIN_TARGET_PASSIVE_EXCL;
  } else if (lock_type == MPI_LOCK_SHARED) {
    return MPC_WIN_TARGET_PASSIVE_SHARED;
  } else {
    mpc_common_debug_fatal("No such lock type");
    return MPC_WIN_TARGET_NONE;
  }
}

void mpc_MPI_Win_control_message_handler(void *data, size_t size) {
  struct mpc_MPI_Win_ctrl_message *message =
      (struct mpc_MPI_Win_ctrl_message *)data;

  /* Get target win desc */
  struct mpc_MPI_Win *desc =
      (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(message->target_win);
  /* Get target context */
  struct mpc_Win_target_ctx *ctx = &desc->target;

  switch (message->type) {
  case MPC_MPI_WIN_CTRL_MSG_LOCK:
    if (mpc_common_spinlock_trylock(&ctx->lock)) {
      mpc_common_debug("Delayed lock from %d", message->source_rank);
      /* Already locked */
      mpc_MPI_win_locks_push_delayed(&ctx->locks, message->source_rank,
                                     message->opt_arg1);
      return;
    }

    mpc_Win_target_ctx_check_for_pending_locks(message->target_win);

    mpc_common_debug("Got lock from %d on %d", message->source_rank, desc->comm_rank);
    mpc_Win_target_ctx_start_exposure_no_lock(
        message->target_win, MPC_WIN_SINGLE_REMOTE, &message->source_rank, 1,
        lock_type_to_exposure_mode(message->opt_arg1));

    mpc_common_spinlock_unlock(&ctx->lock);
    break;
  case MPC_MPI_WIN_CTRL_MSG_UNLOCK:
    if (mpc_common_spinlock_trylock(&ctx->lock)) {
      mpc_common_debug("Delayed unlock from %d", message->source_rank);
      /* Already locked */
      mpc_MPI_win_locks_push_delayed(&ctx->locks, message->source_rank,
                                     WIN_LOCK_UNLOCK);
      return;
    }

    mpc_common_debug("Got UNlock from %d on %d", message->source_rank,
              desc->comm_rank);
    mpc_Win_target_ctx_end_exposure_no_lock(message->target_win,
                                            MPC_WIN_TARGET_PASSIVE_SHARED,
                                            message->source_rank);

    mpc_Win_target_ctx_check_for_pending_locks(message->target_win);

    mpc_common_spinlock_unlock(&ctx->lock);
    break;
  case MPC_MPI_WIN_CTRL_MSG_LOCKALL:
    not_implemented();
    break;
  case MPC_MPI_WIN_CTRL_MSG_UNLOCKALL:
    not_implemented();
    break;
    break;
  case MPC_MPI_WIN_CTRL_NONCONTIG_SEND:
    mpc_MPI_Win_handle_non_contiguous_write(data, size);
    break;
  case MPC_MPI_WIN_CTRL_NONCONTIG_RECV:
    mpc_MPI_Win_handle_non_contiguous_read(data, size);
    break;
  case MPC_MPI_WIN_CTRL_ACCUMULATE_SEND:
    mpc_MPI_Win_handle_non_contiguous_accumulate_send(data, size);
    break;
  case MPC_MPI_WIN_CTRL_FLUSH:
    mpc_MPI_Win_handle_win_flush(data);
    break;
  }
}

void mpc_MPI_Win_control_message_send(MPI_Win win, int rank,
                                      struct mpc_MPI_Win_ctrl_message *message) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);


  if (desc->is_single_process) {
    mpc_MPI_Win_control_message_handler(
        (void *)message, sizeof(struct mpc_MPI_Win_ctrl_message));
  } else {
    mpc_lowcomm_request_t req;
    memset(&req, 0, sizeof(mpc_lowcomm_request_t));
    mpc_lowcomm_isend_class_src(desc->comm_rank, rank, message,
                                 sizeof(struct mpc_MPI_Win_ctrl_message), 16008,
                                 desc->comm, MPC_LOWCOMM_P2P_MESSAGE, &req);
    mpc_lowcomm_request_wait(&req);
    // PMPI_Send( message, sizeof(struct mpc_MPI_Win_ctrl_message), MPI_CHAR,
    // rank, 16008, desc->comm);
    // sctk_control_messages_send_to_task ( rank, desc->comm,
    // SCTK_PROCESS_RDMA_CONTROL_MESSAGE,  0, message,  sizeof(struct
    // mpc_MPI_Win_ctrl_message));
  }

  mpc_thread_yield();
}

void mpc_MPI_Win_control_message_send_piggybacked(
    MPI_Win win, int rank, struct mpc_MPI_Win_ctrl_message *message,
    size_t size) {
  assume(sizeof(struct mpc_MPI_Win_ctrl_message) <= size);

  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  mpc_lowcomm_request_t req;
  mpc_lowcomm_isend_class_src(desc->comm_rank, rank, message, size, 16008,
                               desc->comm, MPC_LOWCOMM_P2P_MESSAGE, &req);
  mpc_lowcomm_request_wait(&req);

  mpc_thread_yield();
}

int __mpc_MPI_Win_init_message(MPI_Win win,
                               struct mpc_MPI_Win_ctrl_message *message,
                               mpc_MPI_Win_ctrl_message_t type, int remote) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  memset(message, 0, sizeof(struct mpc_MPI_Win_ctrl_message));

  int myrank;
  MPI_Comm_rank(desc->comm, &myrank);

  message->source_rank = myrank;
  message->source_win = win;
  message->type = type;

  if (0 <= remote) {
    if (desc->comm_size < remote)
      return 1;

    int local_win = mpc_MPI_win_get_remote_win(desc, remote, 0);

    if (local_win < 0)
      return 1;

    struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(local_win);

    if (!low_win) {
      return 1;
    }

    /* Make sure it is a remote one */
    assume(0 <= low_win->remote_id);
    /* It is a remote one */
    message->target_win = low_win->remote_id;
  }

  return 0;
}

int mpc_MPI_Win_init_lock_message(struct mpc_MPI_Win_ctrl_message *message,
                                  MPI_Win win, int rank, int lock_type) {
  int ret =
      __mpc_MPI_Win_init_message(win, message, MPC_MPI_WIN_CTRL_MSG_LOCK, rank);
  message->opt_arg1 = lock_type;
  return ret;
}

int mpc_MPI_Win_init_unlock_message(struct mpc_MPI_Win_ctrl_message *message,
                                    MPI_Win win, int rank) {
  int ret = __mpc_MPI_Win_init_message(win, message,
                                       MPC_MPI_WIN_CTRL_MSG_UNLOCK, rank);
  return ret;
}

int mpc_MPI_Win_init_accumulate_send(struct mpc_MPI_Win_ctrl_message *message,
                                     MPI_Win win, int dest, MPI_Aint offset,
                                     size_t packed_size, int count,
                                     MPI_Datatype inner_type, MPI_Op op) {
  int ret = __mpc_MPI_Win_init_message(win, message,
                                       MPC_MPI_WIN_CTRL_ACCUMULATE_SEND, dest);
  message->opt_arg1 = offset;
  message->opt_arg2 = packed_size;
  message->opt_arg3 = count;
  message->opt_arg4 = inner_type;
  message->opt_arg5 = (int)op;
  return ret;
}

int mpc_MPI_Win_init_derived_send(struct mpc_MPI_Win_ctrl_message *message,
                                  MPI_Win win, int dest, MPI_Aint offset,
                                  size_t packed_size, int count) {
  int ret = __mpc_MPI_Win_init_message(win, message,
                                       MPC_MPI_WIN_CTRL_NONCONTIG_SEND, dest);
  message->opt_arg1 = offset;
  message->opt_arg2 = packed_size;
  message->opt_arg3 = count;
  return ret;
}

int mpc_MPI_Win_init_derived_recv(struct mpc_MPI_Win_ctrl_message *message,
                                  MPI_Win win, int dest, MPI_Aint offset,
                                  size_t packed_size, int count) {
  int ret = __mpc_MPI_Win_init_message(win, message,
                                       MPC_MPI_WIN_CTRL_NONCONTIG_RECV, dest);
  message->opt_arg1 = offset;
  message->opt_arg2 = packed_size;
  message->opt_arg3 = count;
  return ret;
}

int mpc_MPI_Win_init_flush(struct mpc_MPI_Win_ctrl_message *message,
                           MPI_Win win, int dest) {
  int ret =
      __mpc_MPI_Win_init_message(win, message, MPC_MPI_WIN_CTRL_FLUSH, dest);

  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);
  int target_win = mpc_MPI_win_get_remote_win(desc, dest, 0);

  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(target_win);

  assume(low_win);

  message->opt_arg1 = OPA_load_int(&low_win->outgoing_emulated_rma);

  return ret;
}

void mpc_MPI_Win_notify_dest_ctx_counter(MPI_Win win) {

  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  if (!desc)
    return;

  OPA_incr_int(&desc->target.ctrl_message_counter);
  mpc_common_nodebug("NOTIFY TAR on %d(%p) now %d", win, desc,
               OPA_load_int(&desc->target.ctrl_message_counter));
}

void mpc_MPI_Win_notify_src_ctx_counter(MPI_Win win) {

  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  if (!desc)
    return;

  OPA_incr_int(&desc->source.ctrl_message_counter);
  mpc_common_nodebug("NOTIFY SRC on %d(%p) now %d", win, desc,
               OPA_load_int(&desc->source.ctrl_message_counter));
}
