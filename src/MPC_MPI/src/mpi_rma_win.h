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

#ifndef MPI_RMA_WINDOW_H
#define MPI_RMA_WINDOW_H

#include "mpc_mpi.h"
#include "mpi_rma_epoch.h"
#include "mpc_common_asm.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include "mpc_common_types.h"
#include "mpc_common_types.h"
#include "mpc_thread.h"
#include "sctk_window.h"

/************************************************************************/
/* INTERNAL MPI Window Attribute support                                */
/************************************************************************/

/* Keyval definition */

struct mpc_MPI_Win_keyval {
  uint64_t keyval;
  MPI_Win_copy_attr_function *copy_fn;
  MPI_Win_delete_attr_function *delete_fn;
  void *extra_state;
};

struct mpc_MPI_Win_keyval *
mpc_MPI_Win_keyval_init(MPI_Win_copy_attr_function *copy_fn,
                        MPI_Win_delete_attr_function *delete_fn,
                        void *extra_state);

/* Attr storage */

struct mpc_MPI_Win_attr {
  int keyval;
  void *value;
  struct mpc_MPI_Win_keyval keyval_pl;
  mpc_lowcomm_rdma_window_t win;
};

struct mpc_MPI_Win_attr *
mpc_MPI_Win_attr_init(int keyval, void *value,
                      struct mpc_MPI_Win_keyval *keyval_pl, mpc_lowcomm_rdma_window_t win);

struct mpc_MPI_Win_attr_ht {
  struct mpc_common_hashtable ht;
};

int mpc_MPI_Win_attr_ht_init(struct mpc_MPI_Win_attr_ht *ht);
int mpc_MPI_Win_attr_ht_release(struct mpc_MPI_Win_attr_ht *ht);

/* MPI Iface */

int mpc_MPI_Win_create_keyval(MPI_Win_copy_attr_function *copy_fn,
                              MPI_Win_delete_attr_function *delete_fn,
                              int *keyval, void *extra_state);
int mpc_MPI_Win_free_keyval(int *keyval);

int mpc_MPI_Win_set_attr(MPI_Win win, int keyval, void *attr_val);
int mpc_MPI_Win_get_attr(MPI_Win win, int keyval, void *attr_val, int *flag);
int mpc_MPI_Win_delete_attr(MPI_Win win, int keyval);

/************************************************************************/
/* MPI Window descriptor (to be stored as low-level win payload         */
/************************************************************************/

typedef enum {
  MPC_MPI_WIN_STORAGE_ALLOC,
  MPC_MPI_WIN_STORAGE_MEMALLOC,
  MPC_MPI_WIN_STORAGE_NONE,
  MPC_MPI_WIN_STORAGE_SHARED,
  MPC_MPI_WIN_STORAGE_ROOT_ALLOC
} mpc_MPI_win_storage;

struct mpc_MPI_Win {
  int win_id; /*<< Identifier of the corresponding window */

  int comm_rank; /*<< Comm rank (in win comm) of the task holding the win */
  int comm_size; /*<< Comm size of win_comm */

  MPI_Comm comm;      /*<< Win comm which is a dup of the win-provided comm */
  MPI_Comm real_comm; /*<< The actual comm passed as argument at creation */

  int is_over_network;   /*<< Does the win involves muliple processes */
  int is_single_process; /*<< Does the win only span on a single process */

  int flavor; /*<< MPI Win flavor (as per the standard) */
  int model;  /*<< MPI Win model (as per the standard) */
  mpc_lowcomm_rdma_window_access_mode_t
      mode; /*<< MPI Win access_mode (as per the standard) */

  void
      *win_segment; /*<< Pointer used to store the pointer to the mapped segment
                                                when using a shared window */
  size_t mmaped_size; /*<< Size of the mmaped segment (page alligned) */

  mpc_MPI_win_storage storage; /*<< How the memory of this win was allocated */

  MPI_Info info;                    /*<< MPI Win infos */
  char *win_name;                   /*<< MPI Win name */
  struct mpc_MPI_Win_attr_ht attrs; /*<< MPI Win attributes */

  size_t win_size; /*<< The size of this win */
  size_t win_disp; /*<< The Displacement unit of this win */

  mpc_lowcomm_rdma_window_t *remote_wins; /*<< List of low windows (comm_size) */
  char
      *tainted_wins; /*<< The data state of the remote wins (comm_size)
                                                  1 == used, 4 == used for write
                                                      0 == not used */

  struct mpc_Win_target_ctx target; /*<< Source context for this set of wins */
  struct mpc_Win_source_ctx source; /*<< Target context for this set of wins */

  mpc_thread_t
      progress_thread; /*<< The progress thread can come from a pool */
  volatile int
      progress_thread_running; /*<< Whereas this thread is held by the struct
                                  (see pool code) */
};

static inline int mpc_MPI_win_can_write_directly(struct mpc_MPI_Win *desc,
                                                 void *ptr, int target_rank) {
  /* Is a shared win */
  if (desc->mode == SCTK_WIN_ACCESS_DIRECT) {
    return 1;
  }

  /* In in the shared pool */
  if (mpc_lowcomm_allocmem_is_in_pool(ptr)) {
    return 1;
  }

  /* Is in the same process */
  if (!mpc_lowcomm_is_remote_rank(mpc_lowcomm_communicator_world_rank_of(desc->comm, target_rank))) {
    return 1;
  }

  return 0;
}

static inline int mpc_MPI_win_get_remote_win(struct mpc_MPI_Win *desc, int rank,
                                             int tainted_write) {
  if (rank == MPI_PROC_NULL)
    return -1;

  assert(rank < desc->comm_size);
  desc->tainted_wins[rank] |= 1;
  if (tainted_write)
    desc->tainted_wins[rank] |= 4;
  return desc->remote_wins[rank];
}

struct mpc_MPI_Win *mpc_MPI_Win_init(int flavor, int model, MPI_Comm comm,
                                     int rank, size_t size, size_t disp,
                                     int is_over_network,
                                     mpc_MPI_win_storage storage);

int mpc_MPI_Win_release(struct mpc_MPI_Win *win_desc);

/************************************************************************/
/* INTERNAL MPI Window Creation Implementation                          */
/************************************************************************/

int mpc_MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                       MPI_Comm comm, MPI_Win *win);

int mpc_MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win);

int mpc_MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
                         MPI_Comm comm, void *base, MPI_Win *win);

int mpc_MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
                                MPI_Comm comm, void *base, MPI_Win *win);

int mpc_MPI_Win_free(MPI_Win *win);

/************************************************************************/
/* INTERNAL Window Group                                                */
/************************************************************************/

int mpc_PMPI_Win_get_group(MPI_Win win, MPI_Group *group);

/************************************************************************/
/* Window Shared                                                        */
/************************************************************************/

int mpc_MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size,
                             int *disp_unit, void *baseptr);

/************************************************************************/
/* Window Progress List                                                 */
/************************************************************************/

int mpc_MPI_Win_progress_probe(struct mpc_MPI_Win *desc, void *prebuff,
                               size_t buffsize);

#endif /* MPI_RMA_WINDOW_H */
