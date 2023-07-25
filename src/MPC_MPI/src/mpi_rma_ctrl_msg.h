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

#ifndef MPI_RMA_CONTROL_MSG
#define MPI_RMA_CONTROL_MSG

#include "mpc_mpi.h"
#include "mpc_common_asm.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_types.h"
#include "sctk_window.h"

/************************************************************************/
/* INTERNAL NON-Contig data-type management                            */
/************************************************************************/

void mpc_MPI_Win_handle_non_contiguous_write(void *data, size_t size);
void mpc_MPI_Win_handle_non_contiguous_read(void *data, size_t size);

/************************************************************************/
/* Window Control Message Engine                                        */
/************************************************************************/

typedef enum {
  MPC_MPI_WIN_CTRL_MSG_LOCK,
  MPC_MPI_WIN_CTRL_MSG_UNLOCK,
  MPC_MPI_WIN_CTRL_MSG_LOCKALL,
  MPC_MPI_WIN_CTRL_MSG_UNLOCKALL,
  MPC_MPI_WIN_CTRL_NONCONTIG_SEND,
  MPC_MPI_WIN_CTRL_NONCONTIG_RECV,
  MPC_MPI_WIN_CTRL_ACCUMULATE_SEND,
  MPC_MPI_WIN_CTRL_FLUSH
} mpc_MPI_Win_ctrl_message_t;

struct mpc_MPI_Win_ctrl_message {
  int source_rank;
  int source_win;
  mpc_MPI_Win_ctrl_message_t type;
  MPI_Win target_win;
  MPI_Aint opt_arg1;
  size_t opt_arg2;
  int opt_arg3;
  MPI_Datatype opt_arg4;
  int opt_arg5;
};

void mpc_MPI_Win_control_message_handler(void *data, size_t size);

void mpc_MPI_Win_control_message_send(MPI_Win win, int rank,
                                      struct mpc_MPI_Win_ctrl_message *message);

void mpc_MPI_Win_control_message_send_piggybacked(
    MPI_Win win, int rank, struct mpc_MPI_Win_ctrl_message *message,
    size_t size);

int mpc_MPI_Win_init_lock_message(struct mpc_MPI_Win_ctrl_message *message,
                                  MPI_Win win, int rank, int lock_type);

int mpc_MPI_Win_init_unlock_message(struct mpc_MPI_Win_ctrl_message *message,
                                    MPI_Win win, int rank);

int mpc_MPI_Win_init_accumulate_send(struct mpc_MPI_Win_ctrl_message *message,
                                     MPI_Win win, int dest, MPI_Aint offset,
                                     size_t packed_size, int count,
                                     MPI_Datatype inner_type, MPI_Op op);

int mpc_MPI_Win_init_derived_send(struct mpc_MPI_Win_ctrl_message *message,
                                  MPI_Win win, int dest, MPI_Aint offset,
                                  size_t packed_size, int count);

int mpc_MPI_Win_init_derived_recv(struct mpc_MPI_Win_ctrl_message *message,
                                  MPI_Win win, int dest, MPI_Aint offset,
                                  size_t packed_size, int count);

struct mpc_MPI_Win_ctrl_message *
mpc_MPI_Win_init_flush_shadow(MPI_Win win, int remote, size_t shadow_size);

int mpc_MPI_Win_init_flush(struct mpc_MPI_Win_ctrl_message *message,
                           MPI_Win win, int dest);

void mpc_MPI_Win_notify_dest_ctx_counter(MPI_Win win);

void mpc_MPI_Win_notify_src_ctx_counter(MPI_Win win);

#endif /* MPI_RMA_CONTROL_MSG */
