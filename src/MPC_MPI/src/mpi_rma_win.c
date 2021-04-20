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

#include "mpi_rma_win.h"
#include "mpi_rma_ctrl_msg.h"
#include "sctk_control_messages.h"
#include <mpc_launch_shm.h>

#include "mpi_alloc_mem.h"
#include "mpi_rma_epoch.h"
#include "mpc_lowcomm.h"
#include "mpc_lowcomm_rdma.h"

#include <sctk_alloc.h>

/************************************************************************/
/* Window Progress List                                                 */
/************************************************************************/

#define TMP_SIZE 4194305

int mpc_MPI_Win_progress_probe(struct mpc_MPI_Win *desc, void *prebuff,
                               size_t buffsize) {
  int have_msg = 0;
  MPI_Status st;

  int dest = mpc_lowcomm_communicator_world_rank_of(desc->comm, desc->comm_rank);

  mpc_lowcomm_iprobe_src_dest(MPC_ANY_SOURCE, dest, 16008, desc->comm, &have_msg, &st);

  if (have_msg) {

    int msize = 0;
    MPI_Get_count(&st, MPI_CHAR, &msize);

    void *buff = prebuff;

    if (buffsize <= (size_t)msize) {
      buff = sctk_malloc(msize);
      assume(buff);
    }

    mpc_lowcomm_request_t req;
    memset(&req, 0, sizeof(mpc_lowcomm_request_t));
    mpc_lowcomm_irecv_class_dest(st.MPI_SOURCE, desc->comm_rank, buff, msize,
                                  16008, desc->comm, MPC_LOWCOMM_P2P_MESSAGE, &req);
    mpc_lowcomm_request_wait(&req);
    // PMPI_Recv(buff, msize, MPI_CHAR, st.MPI_SOURCE, 16008, desc->comm, &st);

    mpc_MPI_Win_control_message_handler(buff, msize);

    if (buffsize <= (size_t)msize) {
      sctk_free(buff);
    }

    mpc_MPI_Win_request_array_test(&desc->source.requests);
    mpc_MPI_Win_request_array_test(&desc->target.requests);

    return 1;
  }

  return 0;
}

#if 1

static mpc_common_spinlock_t __pool_submit_lock = SCTK_SPINLOCK_INITIALIZER;
static struct mpc_MPI_Win *submited_desc = NULL;
static volatile int progress_pool_count = 0;

struct mpc_MPI_Win *mpc_MPI_Win_progress_pool_wait() {
  struct mpc_MPI_Win *ret = NULL;

  int inc = 0;

  while (1) {
    if (mpc_common_spinlock_trylock(&__pool_submit_lock)) {
      mpc_thread_yield();
      continue;
    }

    if (!inc) {
      inc = 1;
      progress_pool_count++;
    }

    if (submited_desc) {
      ret = submited_desc;
      submited_desc = NULL;
      progress_pool_count--;
    }

    mpc_common_spinlock_unlock(&__pool_submit_lock);

    if (ret)
      break;

    mpc_thread_yield();
  }

  return ret;
}

void *mpc_MPI_Win_progress_thread(void *pdesc) {

  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)pdesc;

  int did_work = 0;

  char *_buff = sctk_malloc(TMP_SIZE);
  assume(_buff);

  while (1) {

    while (desc->progress_thread_running) {
      did_work = 0;

      int ret = 0, cnt = 0;
      while ((ret = mpc_MPI_Win_progress_probe(desc, _buff, TMP_SIZE)) &&
             (cnt++ < 32)) {
        did_work |= ret;
      }

      if (!did_work) {
        mpc_thread_yield();
      }
    }

    desc->progress_thread_running = 99;

    mpc_common_nodebug("Joining the POOL :=)");
    desc = mpc_MPI_Win_progress_pool_wait();
  }

  sctk_free(_buff);

  return NULL;
}
#endif

/************************************************************************/
/* MPI Window descriptor (to be stored as low-level win payload)        */
/************************************************************************/


struct mpc_MPI_Win *mpc_MPI_Win_init(int flavor, int model, MPI_Comm comm,
                                     int rank, size_t size, size_t disp,
                                     int is_over_network,
                                     mpc_MPI_win_storage storage) {
  mpc_lowcomm_rdma_MPI_windows_in_use();
  struct mpc_MPI_Win *ret = sctk_malloc(sizeof(struct mpc_MPI_Win));

  if (!ret) {
    perror("malloc");
    return NULL;
  }

  MPI_Comm win_com;
  PMPI_Comm_dup(comm, &win_com);

  ret->comm_rank = rank;
  ret->flavor = flavor;
  ret->storage = storage;

  ret->mode = SCTK_WIN_ACCESS_AUTO;

  if (flavor == MPI_WIN_FLAVOR_SHARED) {
    ret->mode = SCTK_WIN_ACCESS_DIRECT;
  } else if (flavor == MPI_WIN_FLAVOR_DYNAMIC) {
    ret->mode = SCTK_WIN_ACCESS_EMULATED;
  }
  ret->win_segment = NULL;
  ret->mmaped_size = 0;
  ret->model = model;
  ret->win_size = size;
  ret->win_disp = disp;

  ret->comm = win_com;
  ret->real_comm = comm;

  PMPI_Comm_size(win_com, &ret->comm_size);

  ret->remote_wins = sctk_malloc(sizeof(int) * ret->comm_size);

  if (!ret->remote_wins) {
    perror("malloc");
    return NULL;
  }

  ret->tainted_wins = sctk_calloc(ret->comm_size, sizeof(char));

  if (!ret->tainted_wins) {
    perror("malloc");
    return NULL;
  }

  ret->win_name = strdup("");

  mpc_Win_source_ctx_init(&ret->source, win_com);
  mpc_Win_target_ctx_init(&ret->target, win_com);

  ret->is_single_process =
      (mpc_common_get_task_count() == mpc_common_get_local_task_count());

  ret->is_over_network = is_over_network || !ret->is_single_process;

  mpc_MPI_Win_attr_ht_init(&ret->attrs);

  ret->progress_thread_running = 1;

  int used_pool_thread = 0;
  int pool_retries = 0;

  while (pool_retries++ < 32) {
    mpc_common_spinlock_lock_yield(&__pool_submit_lock);

    if (progress_pool_count) {
      if (!submited_desc) {
        used_pool_thread = 1;
        submited_desc = ret;
      }
    }

    mpc_common_spinlock_unlock(&__pool_submit_lock);

    if (used_pool_thread)
      break;
    else
      mpc_thread_yield();
  }

  mpc_thread_yield();

  if (!used_pool_thread) {
    mpc_thread_core_thread_create(&ret->progress_thread, NULL,
                            mpc_MPI_Win_progress_thread, ret);
  }

  return ret;
}

int mpc_MPI_Win_release(struct mpc_MPI_Win *win_desc) {
  win_desc->flavor = -1;
  win_desc->model = -1;

  PMPI_Comm_free(&win_desc->comm);
  win_desc->comm = MPI_COMM_NULL;

  win_desc->comm_size = -1;

  mpc_MPI_Win_attr_ht_release(&win_desc->attrs);

  sctk_free(win_desc->remote_wins);
  win_desc->remote_wins = NULL;

  sctk_free(win_desc->tainted_wins);
  win_desc->tainted_wins = NULL;

  free(win_desc->win_name);
  win_desc->win_name = NULL;

  sctk_free(win_desc);

  return 0;
}

/************************************************************************/
/* INTERNAL MPI Window Creation Implementation                          */
/************************************************************************/

static inline int mpc_MPI_Win_do_registration(mpc_lowcomm_rdma_window_t internal_win,
                                              int WIN_FLAVOR) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc =
      (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(internal_win);

  if (!desc) {
    mpc_common_debug_error("Could not retrieve internal payload");
    return MPI_ERR_INTERN;
  }

  /* Make sure that the call is collective */
  PMPI_Barrier(desc->comm);

  mpc_lowcomm_rdma_window_access_mode_t win_mode = SCTK_WIN_ACCESS_AUTO;

  if (WIN_FLAVOR == MPI_WIN_FLAVOR_DYNAMIC) {
    win_mode = SCTK_WIN_ACCESS_EMULATED;
  } else if (WIN_FLAVOR == MPI_WIN_FLAVOR_SHARED) {
    win_mode = SCTK_WIN_ACCESS_DIRECT;
  }

  int my_win_id = internal_win;
  struct mpc_lowcomm_rdma_window *win = sctk_win_translate(my_win_id);

  /* Override access mode from buffer knowledge */
  mpc_lowcomm_rdma_window_set_access_mode(win, win_mode);

  if ((win->is_emulated || mpc_lowcomm_is_remote_rank(win->owner)) &&
      (win_mode != SCTK_WIN_ACCESS_DIRECT)) {
    desc->source.requests.is_emulated = 1;
    desc->target.requests.is_emulated = 1;
  }

  /* Flag the empty windows */
  if ((win->size == 0) && (WIN_FLAVOR != MPI_WIN_FLAVOR_DYNAMIC)) {
    my_win_id = -1;
  }

  /* Now do exchange all the remote window ids
   * processes with no local data (size 0) have -1 as win id */
  int ret = PMPI_Allgather(&my_win_id, 1, MPI_INT, desc->remote_wins, 1,
                           MPI_INT, desc->comm);

  if (ret != MPI_SUCCESS) {
    return ret;
  }

  /* At this point, we have an array with the remote win ID
   * when the remote caries data -1 if not the array size
   * is the communiticator cardilality */

  struct mpc_lowcomm_rdma_window *remote_windows =
      sctk_calloc(desc->comm_size, sizeof(struct mpc_lowcomm_rdma_window));

  if (!remote_windows) {
    perror("calloc");
    mpc_common_debug_fatal("Failed to allocated window remote array");
  }

  /* We now exchange the remote windows all at once */
  ret =
      PMPI_Allgather(win, sizeof(struct mpc_lowcomm_rdma_window), MPI_CHAR, remote_windows,
                     sizeof(struct mpc_lowcomm_rdma_window), MPI_CHAR, desc->comm);

  if (ret != MPI_SUCCESS) {
    return ret;
  }

  /* We are about to remote MAP windows */

  /* We now proceeed to individual remote window resolution
   * by mapping ourselved to the remote windows */
  int i;

  int my_rank;
  PMPI_Comm_rank(desc->comm, &my_rank);

  for (i = 0; i < desc->comm_size; i++) {
    /* Skip zero-sized wins */
    if (desc->remote_wins[i] == -1)
      continue;

    desc->remote_wins[i] = mpc_lowcomm_rdma_window_build_from_remote(&remote_windows[i]);

    struct mpc_lowcomm_rdma_window *new_win = sctk_win_translate(desc->remote_wins[i]);

    /* Override access mode in function of window type */
    mpc_lowcomm_rdma_window_set_access_mode(new_win, win_mode);

    if ((new_win->is_emulated || mpc_lowcomm_is_remote_rank(new_win->owner)) &&
        (win_mode != SCTK_WIN_ACCESS_DIRECT)) {
      desc->source.requests.is_emulated |= 1;
      desc->target.requests.is_emulated |= 1;
    }

    new_win->payload = (void *)desc;
  }

  PMPI_Barrier(desc->comm);

  sctk_free(remote_windows);

  return MPI_SUCCESS;
}

int __mpc_MPI_Win_create(void *base, MPI_Aint size, int disp_unit,
                         __UNUSED__ MPI_Info info, MPI_Comm comm, MPI_Win *win,
                         int WIN_FLAVOR, int MEM_MODEL,
                         mpc_MPI_win_storage storage) {

  int my_rank;
  PMPI_Comm_rank(comm, &my_rank);

  int comm_size;
  PMPI_Comm_size(comm, &comm_size);

  int is_over_network = 1;

  /* A window is not emulated if all its members are local */
  if (WIN_FLAVOR == MPI_WIN_FLAVOR_SHARED)
    is_over_network = 0;

  struct mpc_MPI_Win *desc =
      mpc_MPI_Win_init(WIN_FLAVOR, MEM_MODEL, comm, my_rank, size, disp_unit,
                       is_over_network, storage);

  if (!desc) {
    return MPI_ERR_INTERN;
  }

  /* We now create the local window */

  mpc_lowcomm_rdma_window_t internal_win = mpc_lowcomm_rdma_window_init(base, size, disp_unit, comm);

  /* Attach the MPI descriptor */
  mpc_lowcomm_rdma_window_set_payload(internal_win, (void *)desc);

  /* Save win_id */
  desc->win_id = internal_win;

  int ret = mpc_MPI_Win_do_registration(internal_win, WIN_FLAVOR);

  if (ret != MPI_SUCCESS) {
    return ret;
  }

  /* Output the WIN id */
  *win = internal_win;

  PMPI_Barrier(comm);

  return MPI_SUCCESS;
}

int mpc_MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                       MPI_Comm comm, MPI_Win *win) {
  return __mpc_MPI_Win_create(base, size, disp_unit, info, comm, win,
                              MPI_WIN_FLAVOR_CREATE, MPI_WIN_SEPARATE,
                              MPC_MPI_WIN_STORAGE_NONE);
}

int mpc_MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win) {
  /* TODO ALL EMULATE FOR THIS CASE (OUTLINE EMULTATED RMA in sctk_rma.h) */
  return __mpc_MPI_Win_create(MPI_BOTTOM, 0, 1, info, comm, win,
                              MPI_WIN_FLAVOR_DYNAMIC, MPI_WIN_SEPARATE,
                              MPC_MPI_WIN_STORAGE_NONE);
}

int mpc_MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
                         MPI_Comm comm, void *base, MPI_Win *win) {

  /* Start by checking the shifting to a shared window */
  int youd_better_move_to_shared = 0;

  if (mpc_common_get_task_count() != mpc_common_get_local_task_count()) {
    /* If we are here we are not all in the same process
     * which is the initial requirement to use a shared win */
    MPI_Comm node_comm;

    int my_rank, source_size, node_size;
    PMPI_Comm_rank(comm, &my_rank);
    PMPI_Comm_size(comm, &source_size);

    PMPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, my_rank, MPI_INFO_NULL,
                         &node_comm);

    PMPI_Comm_size(node_comm, &node_size);

    PMPI_Comm_free(&node_comm);

    if (source_size == node_size) {
      youd_better_move_to_shared = 1;
    }
  }

  int ret = 0;

  if (youd_better_move_to_shared) {
    /* Opt for a shared win */
    ret = mpc_MPI_Win_allocate_shared(size, disp_unit, info, comm, base, win);

    /* Restore the correct flavor */
    struct mpc_MPI_Win *desc =
        (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(*win);
    assume(desc != NULL);
    desc->flavor = MPI_WIN_FLAVOR_ALLOCATE;

  } else {
    /* Here we opt for an allocated win
     * as all ranks were not on the same node
     * but still try to allocate shared*/
    void *new_win_segment = mpc_MPI_allocmem_pool_alloc(size);

    if (!new_win_segment) {
      perror("malloc");
      return MPI_ERR_INTERN;
    }

    ret = __mpc_MPI_Win_create(new_win_segment, size, disp_unit, info, comm,
                               win, MPI_WIN_FLAVOR_ALLOCATE, MPI_WIN_SEPARATE,
                               MPC_MPI_WIN_STORAGE_MEMALLOC);

    *((void **)base) = new_win_segment;
  }

  return ret;
}

static inline int __barrier(void *pcomm)
{
	MPI_Comm comm = (MPI_Comm)pcomm;
	PMPI_Barrier(comm);
	return 0;
}

static inline int __rank(void *pcomm)
{
	MPI_Comm comm = (MPI_Comm)pcomm;
	int ret;
	PMPI_Comm_rank(comm, &ret);
	return ret;
}

static inline int __bcast(void *buff, size_t len, void *pcomm)
{
	MPI_Comm comm = (MPI_Comm)pcomm;
	
	PMPI_Bcast(buff,len, MPI_BYTE, 0, comm);
	return 0;
}


int mpc_MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
                                MPI_Comm comm, void *base, MPI_Win *win) {
  int rank;
  PMPI_Comm_rank(comm, &rank);
  int comm_size;
  PMPI_Comm_size(comm, &comm_size);
  uint64_t total_size;
  uint64_t tmp_size = size;

  int ret = PMPI_Allreduce((void *)&tmp_size, (void *)&total_size, 1,
                           MPI_UINT64_T, MPI_SUM, comm);

  size_t to_map_size = ((total_size / SCTK_PAGE_SIZE) + 1) *
                       SCTK_PAGE_SIZE;
  void *win_segment = NULL;

  mpc_MPI_win_storage storage_type = MPC_MPI_WIN_STORAGE_NONE;

  /* Are all the tasks in the same process ? */
  if (mpc_common_get_task_count() == mpc_common_get_local_task_count()) {
    /* Easy case we are all in the same process */
    if (!rank) {
      /* Root does allocated */
      win_segment = sctk_malloc(total_size);
      assume(win_segment != NULL);
    }

    /* Broadcast the base address */
    ret = PMPI_Bcast(&win_segment, sizeof(void *), MPI_CHAR, 0, comm);

    storage_type = MPC_MPI_WIN_STORAGE_ROOT_ALLOC;
  } else {
    /* Now count number of processes by counting masters */
    int is_master = 0;

    if (mpc_common_get_local_task_rank() == 0) {
      is_master = 1;
    }

    MPI_Comm process_master_comm, node_comm;
    PMPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL,
                         &node_comm);

    PMPI_Comm_split(node_comm, is_master, rank, &process_master_comm);

    /* Only process masters do the mapping */
    if (mpc_common_get_local_task_rank() == 0) {

      mpc_launch_shm_exchange_params_t params;

			params.mpi.barrier = __barrier;
			params.mpi.bcast = __bcast;
			params.mpi.rank = __rank;
			params.mpi.pcomm = process_master_comm;

			win_segment = mpc_launch_shm_map(to_map_size, MPC_LAUNCH_SHM_USE_MPI, &params);
    }

    /* Make sure that non process masters have the address */
    ret = PMPI_Bcast(&win_segment, sizeof(void *), MPI_CHAR, 0, node_comm);

    PMPI_Comm_free(&process_master_comm);
    PMPI_Comm_free(&node_comm);

    storage_type = MPC_MPI_WIN_STORAGE_SHARED;
  }

  /* Prepare to compute offset for each rank */
  MPI_Aint *individual_sizes = sctk_calloc(comm_size, sizeof(MPI_Aint));
  assume(individual_sizes != NULL);

  ret = PMPI_Allgather((void *)&size, 1, MPI_AINT, (void *)individual_sizes, 1,
                       MPI_AINT, comm);

  size_t local_offset = 0;

  int i;

  for (i = 0; i < rank; i++) {
    local_offset += individual_sizes[i];
  }

  sctk_free(individual_sizes);

  void *local_base = win_segment + local_offset;
  mpc_common_nodebug("Base is %p for size %ld over WS %p", local_base, size,
               win_segment);

  ret = __mpc_MPI_Win_create(local_base, size, disp_unit, info, comm, win,
                             MPI_WIN_FLAVOR_SHARED, MPI_WIN_UNIFIED,
                             storage_type);

  if (ret != MPI_SUCCESS) {
    return ret;
  }

  /* Store the win segment to allow future releases */
  struct mpc_MPI_Win *desc =
      (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(*win);
  desc->win_segment = win_segment;
  desc->mmaped_size = to_map_size;

  *((void **)base) = local_base;

  return ret;
}

int mpc_MPI_Win_free(MPI_Win *win) {

  if (*win == MPI_WIN_NULL) {
    return MPI_ERR_ARG;
  }

  /* Retrieve the MPI Desc */
  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(*win);

  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)low_win->payload;

  /* Fence control messages */
  mpc_Win_contexes_fence_control(*win);

  /* Fence the requests */
  mpc_MPI_Win_request_array_fence(&desc->source.requests);
  mpc_MPI_Win_request_array_fence(&desc->target.requests);

  PMPI_Barrier(desc->comm);

#if 1
  desc->progress_thread_running = 0;

  while (desc->progress_thread_running != 99) {
    mpc_thread_yield();
  }
#else
  PMPI_Barrier(desc->comm);
  mpc_MPI_Win_progress_list_unregister(desc);
#endif

  PMPI_Barrier(desc->comm);

  /* Deregister wins */
  int i;

  for (i = 0; i < desc->comm_size; i++) {

    if (desc->remote_wins[i] < 0) {
      continue;
    }

    mpc_lowcomm_rdma_window_local_release(desc->remote_wins[i]);
  }

  int rank;
  PMPI_Comm_rank(desc->comm, &rank);

  /* Free win memory */
  switch (desc->storage) {
  case MPC_MPI_WIN_STORAGE_NONE:
    break;

  case MPC_MPI_WIN_STORAGE_ROOT_ALLOC:
    if (!rank) {
      sctk_free(low_win->start_addr);
    }
    break;

  case MPC_MPI_WIN_STORAGE_ALLOC:
    sctk_free(low_win->start_addr);
    break;

  case MPC_MPI_WIN_STORAGE_MEMALLOC:
    mpc_MPI_allocmem_pool_free(low_win->start_addr);
    break;

  case MPC_MPI_WIN_STORAGE_SHARED: {

    /* Now count number of processes by counting masters */
    int is_master = 0;

    if (mpc_common_get_local_task_rank() == 0) {
      is_master = 1;
    }


    MPI_Comm process_master_comm;
    PMPI_Comm_split(desc->comm, is_master, rank, &process_master_comm);

    if (mpc_common_get_local_task_rank() == 0) {
      mpc_launch_shm_unmap(desc->win_segment, desc->mmaped_size);
    }

    PMPI_Comm_free(&process_master_comm);
  } break;
  }

  PMPI_Barrier(desc->comm);

  mpc_lowcomm_rdma_window_set_payload(*win, NULL);

  mpc_MPI_Win_release(desc);

  mpc_common_debug("FREED win %d", *win);

  mpc_lowcomm_rdma_window_local_release(*win);

  *win = MPI_WIN_NULL;

  return MPI_SUCCESS;
}

/************************************************************************/
/* Window Group                                                         */
/************************************************************************/

int mpc_PMPI_Win_get_group(MPI_Win win, MPI_Group *group) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  if (!desc)
    return MPI_ERR_INTERN;

  return MPI_Comm_group(desc->comm, group);
}

/************************************************************************/
/* Window Shared                                                        */
/************************************************************************/

int mpc_MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size,
                             int *disp_unit, void *baseptr) {
  /* Retrieve the MPI Desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  if (!desc)
    return MPI_ERR_INTERN;

  if (desc->flavor != MPI_WIN_FLAVOR_SHARED) {
    return MPI_ERR_RMA_FLAVOR;
  }

  if (((rank < 0) || (desc->comm_size <= rank)) && (rank != MPI_PROC_NULL)) {
    return MPI_ERR_RANK;
  }

  if (!disp_unit || !baseptr) {
    return MPI_ERR_ARG;
  }

  if (rank == MPI_PROC_NULL) {
    /* Return the lowest rank with non-null size */
    int i;
    for (i = 0; i < desc->comm_size; i++) {
      if (0 <= mpc_MPI_win_get_remote_win(desc, i, 0)) {
        /* Found ! */
        rank = i;
        break;
      }
    }
  }

  int remote_win = mpc_MPI_win_get_remote_win(desc, rank, 0);

  struct mpc_lowcomm_rdma_window *low_win = NULL;

  if (remote_win < 0) {
    /* Empty window */
    *size = 0;
    *disp_unit = 0;
    *((void **)baseptr) = NULL;
    return MPI_SUCCESS;
  } else {
    low_win = sctk_win_translate(remote_win);
  }

  if (!low_win) {
    return MPI_ERR_WIN;
  }

  *size = low_win->size;
  *disp_unit = low_win->disp_unit;
  *((void **)baseptr) = low_win->start_addr;

  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI Window Attribute support                                */
/************************************************************************/

/* Keyval low-level storage */

static struct mpc_common_hashtable __win_keyval_ht;
mpc_common_spinlock_t __win_keyval_ht_lock = SCTK_SPINLOCK_INITIALIZER;
int __win_keyval_ht_init_done = 0;

static inline void win_keyval_ht_init_once() {
  mpc_common_spinlock_lock(&__win_keyval_ht_lock);

  if (!__win_keyval_ht_init_done) {
    mpc_common_hashtable_init(&__win_keyval_ht, 16);
    __win_keyval_ht_init_done = 1;
  }

  mpc_common_spinlock_unlock(&__win_keyval_ht_lock);
}

static inline uint64_t get_per_rank_keyval_key(int keyval) {
  uint64_t rank = mpc_common_get_task_rank();
  uint64_t ret = 0;

  ret = (rank << 32);
  ret |= keyval;

  return ret;
}
/* Attr definition */

static OPA_int_t __win_attr_counter;

struct mpc_MPI_Win_keyval *
mpc_MPI_Win_keyval_init(MPI_Win_copy_attr_function *copy_fn,
                        MPI_Win_delete_attr_function *delete_fn,
                        void *extra_state) {
  struct mpc_MPI_Win_keyval *ret =
      sctk_calloc(1, sizeof(struct mpc_MPI_Win_keyval));

  if (!ret) {
    perror("calloc");
    return NULL;
  }

  ret->copy_fn = copy_fn;
  ret->delete_fn = delete_fn;
  ret->extra_state = extra_state;

  int my_id = OPA_fetch_and_add_int(&__win_attr_counter, 1);

  /* Make sure to skip the compulsory attributes  (WIN_BASE, & co ) */
  my_id += 50;

  ret->keyval = my_id;

  return ret;
}

/* Attr storage */

struct mpc_MPI_Win_attr *
mpc_MPI_Win_attr_init(int keyval, void *value,
                      struct mpc_MPI_Win_keyval *keyval_pl, mpc_lowcomm_rdma_window_t win) {
  struct mpc_MPI_Win_attr *ret =
      sctk_calloc(1, sizeof(struct mpc_MPI_Win_attr));

  if (ret == NULL) {
    perror("Calloc");
    return ret;
  }

  ret->keyval = keyval;
  ret->value = value;
  memcpy(&ret->keyval_pl, keyval_pl, sizeof(struct mpc_MPI_Win_keyval));
  ret->win = win;

  return ret;
}

int mpc_MPI_Win_attr_ht_init(struct mpc_MPI_Win_attr_ht *atht) {
  mpc_common_hashtable_init(&atht->ht, 16);
  return 0;
}

int mpc_MPI_Win_attr_ht_release(struct mpc_MPI_Win_attr_ht *atht) {
  void *pattr = NULL;

  int found = -1;
  do {
    found = -1;

    struct mpc_MPI_Win_attr *attr = NULL;
    MPC_HT_ITER(&atht->ht, pattr)

    if (found < 0) {
      attr = (struct mpc_MPI_Win_attr *)pattr;
      if (attr) {
        found = attr->keyval;
      }
    }

    MPC_HT_ITER_END

    if (found != -1) {

      if (attr) {
        if (attr->keyval_pl.delete_fn != NULL) {
          (attr->keyval_pl.delete_fn)(attr->win, found, attr->value,
                                      attr->keyval_pl.extra_state);
        }
      }

      mpc_common_hashtable_delete(&atht->ht, found);
      sctk_free(attr);
    }

  } while (found != -1);

  mpc_common_hashtable_release(&atht->ht);

  return 0;
}

/* MPI Iface */

/* MPI Attr handling */

int mpc_MPI_Win_set_attr(MPI_Win win, int keyval, void *attr_val) {
  win_keyval_ht_init_once();

  if (!attr_val) {
    return MPI_ERR_ARG;
  }

  /* Retrieve win desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  if (!desc) {
    return MPI_ERR_WIN;
  }

  /* Check keyval */

  uint64_t per_rank_keyval = get_per_rank_keyval_key(keyval);

  struct mpc_MPI_Win_keyval *kv =
      (struct mpc_MPI_Win_keyval *)mpc_common_hashtable_get(&__win_keyval_ht, per_rank_keyval);

  if (!kv) {
    return MPI_ERR_KEYVAL;
  }

  /* If we are here ready to set */
  struct mpc_MPI_Win_attr *attr =
      (struct mpc_MPI_Win_attr *)mpc_common_hashtable_get(&desc->attrs.ht, keyval);

  if (!attr) {
    attr = mpc_MPI_Win_attr_init(keyval, attr_val, kv, win);
    mpc_common_hashtable_set(&desc->attrs.ht, keyval, attr);
  } else {
    attr->value = attr_val;
  }

  return MPI_SUCCESS;
}

int mpc_MPI_Win_get_attr(MPI_Win win, int keyval, void *attr_val, int *flag) {
  win_keyval_ht_init_once();

  *flag = 0;

  if (!attr_val || !flag) {
    return MPI_ERR_ARG;
  }

  /* Retrieve win desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  if (!desc) {
    return MPI_ERR_WIN;
  }

  struct mpc_lowcomm_rdma_window *low_win = sctk_win_translate(win);
  uintptr_t val;

  /* First handle special values */
  switch (keyval) {
  case MPI_WIN_BASE:
    *flag = 1;
    *((void **)attr_val) = low_win->start_addr;
    return MPI_SUCCESS;
  case MPI_WIN_SIZE:
    *flag = 1;
    *((void **)attr_val) = (void*)low_win->size;
    return MPI_SUCCESS;
  case MPI_WIN_DISP_UNIT:
    *flag = 1;
    *((void **)attr_val) = (void*)low_win->disp_unit;
    return MPI_SUCCESS;
  case MPI_WIN_CREATE_FLAVOR:
    *flag = 1;
    val = desc->flavor;
    *((void **)attr_val) = (void*)val;
    return MPI_SUCCESS;
  case MPI_WIN_MODEL:
    *flag = 1;
    val = desc->model;
    *((void **)attr_val) = (void*)val;
    return MPI_SUCCESS;
  }

  /* If we are here it is an user-defined ATTR */

  /* First check keyval */

  uint64_t per_rank_keyval = get_per_rank_keyval_key(keyval);

  void *pkv = mpc_common_hashtable_get(&__win_keyval_ht, per_rank_keyval);

  if (!pkv) {
    return MPI_ERR_KEYVAL;
  }

  /* Now try to retrieve attr */
  struct mpc_MPI_Win_attr *attr =
      (struct mpc_MPI_Win_attr *)mpc_common_hashtable_get(&desc->attrs.ht, keyval);

  if (!attr) {
    *flag = 0;
  } else {
    *flag = 1;
    *((void **)attr_val) = attr->value;
  }

  return MPI_SUCCESS;
}

int mpc_MPI_Win_delete_attr(MPI_Win win, int keyval) {
  win_keyval_ht_init_once();

  /* Retrieve win desc */
  struct mpc_MPI_Win *desc = (struct mpc_MPI_Win *)mpc_lowcomm_rdma_window_get_payload(win);

  if (!desc) {
    return MPI_ERR_WIN;
  }

  /* Check keyval */

  uint64_t per_rank_keyval = get_per_rank_keyval_key(keyval);

  struct mpc_MPI_Win_keyval *kv =
      (struct mpc_MPI_Win_keyval *)mpc_common_hashtable_get(&__win_keyval_ht, per_rank_keyval);

  if (!kv) {
    return MPI_ERR_KEYVAL;
  }

  /* Now try to retrieve attr */
  struct mpc_MPI_Win_attr *attr =
      (struct mpc_MPI_Win_attr *)mpc_common_hashtable_get(&desc->attrs.ht, keyval);

  if (attr) {
    if (kv->delete_fn != NULL) {
      (kv->delete_fn)(win, keyval, attr->value, kv->extra_state);
    }

    mpc_common_hashtable_delete(&desc->attrs.ht, keyval);
    sctk_free(attr);
  }

  return MPI_SUCCESS;
}

int mpc_MPI_Win_create_keyval(MPI_Win_copy_attr_function *copy_fn,
                              MPI_Win_delete_attr_function *delete_fn,
                              int *keyval, void *extra_state) {
  win_keyval_ht_init_once();

  if (!keyval) {
    return MPI_ERR_ARG;
  }

  struct mpc_MPI_Win_keyval *new =
      mpc_MPI_Win_keyval_init(copy_fn, delete_fn, extra_state);

  if (!new) {
    return MPI_ERR_INTERN;
  }

  /* Now add to the ATTR HT */
  uint64_t per_rank_keyval = get_per_rank_keyval_key(new->keyval);
  mpc_common_hashtable_set(&__win_keyval_ht, per_rank_keyval, new);

  /* SET the return keyval */
  *keyval = new->keyval;

  return MPI_SUCCESS;
}

int mpc_MPI_Win_free_keyval(int *keyval) {
  win_keyval_ht_init_once();

  if (!keyval) {
    return MPI_ERR_ARG;
  }

  uint64_t per_rank_keyval = get_per_rank_keyval_key(*keyval);

  void *pkv = mpc_common_hashtable_get(&__win_keyval_ht, per_rank_keyval);

  if (!pkv) {
    return MPI_ERR_KEYVAL;
  }

  mpc_common_hashtable_delete(&__win_keyval_ht, per_rank_keyval);
  sctk_free(pkv);

  return MPI_SUCCESS;
}
