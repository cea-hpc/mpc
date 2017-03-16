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

#include "mpi_alloc_mem.h"

#include "mpc_mpi.h"
#include "mpc_thread.h"
#include "sctk_accessor.h"
#include "sctk_handler_mpi.h"
#include "sctk_runtime_config.h"
#include "sctk_shm_mapper.h"
#include "sctk_thread.h"
#include "string.h"

struct mpc_MPI_allocmem_pool ____mpc_sctk_mpi_alloc_mem_pool;

static int _pool_init_done = 0;
static int _pool_only_local = 0;
static sctk_spinlock_t _pool_init_lock = 0;

static size_t _forced_pool_size = 0;

void mpc_MPI_allocmem_adapt(char *exename) {
  if (!exename)
    return;

  if (_forced_pool_size != 0)
    return;

  int is_linear = sctk_runtime_config_get()
                      ->modules.rma.alloc_mem_pool_force_process_linear;

  if (sctk_runtime_config_get()->modules.rma.alloc_mem_pool_autodetect) {
    if (!strstr(exename, "IMB")) {
      /* Force linear */
      is_linear = 1;
    }
  }

  if (is_linear) {
    _forced_pool_size =
        sctk_runtime_config_get()->modules.rma.alloc_mem_pool_per_process_size *
        sctk_get_local_process_number();
    sctk_info("Info : setting MPI_Alloc_mem pool size to %d MB",
              _forced_pool_size / (1024 * 1024));
  }
}

size_t mpc_MPI_allocmem_get_pool_size() {

  if (_forced_pool_size != 0) {
    return _forced_pool_size;
  }

  return sctk_runtime_config_get()->modules.rma.alloc_mem_pool_size;
}

int mpc_MPI_allocmem_pool_init() {
  size_t pool_size = mpc_MPI_allocmem_get_pool_size();

  int do_init = 0;

  sctk_spinlock_lock(&_pool_init_lock);

  if (_pool_init_done == 0) {
    _pool_init_done = 1;
    do_init = 1;
  }

  sctk_spinlock_unlock(&_pool_init_lock);

  int cw_rank;
  PMPI_Comm_rank(MPI_COMM_WORLD, &cw_rank);

  /* This code is one task per MPC process */
  size_t inner_size = pool_size;

  /* Book space for the bit array at buffer start */
  inner_size += SCTK_SHM_MAPPER_PAGE_SIZE;

  /* Book space for the spinlock */
  inner_size += sizeof(sctk_atomics_int);

  /* Are all the tasks in the same process ? */
  if (sctk_get_task_number() == sctk_get_local_task_number()) {
    mpc_MPI_accumulate_op_lock_init_shared();
    return 0;
  } else {
    /* First build a comm for each node */
    MPI_Comm node_comm;

    PMPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, cw_rank,
                         MPI_INFO_NULL, &node_comm);

    int my_rank, comm_size, node_size;
    PMPI_Comm_rank(node_comm, &my_rank);
    PMPI_Comm_size(node_comm, &comm_size);

    int alloc_mem_enabled =
        sctk_runtime_config_get()->modules.rma.alloc_mem_pool_enable;

    if ((comm_size == 1) || (alloc_mem_enabled == 0)) {
      mpc_MPI_accumulate_op_lock_init_shared();
      _pool_only_local = 1;
      PMPI_Comm_free(&node_comm);
      return 0;
    }

    sctk_shm_mapper_role_t role = SCTK_SHM_MAPPER_ROLE_SLAVE;

    if (!my_rank) {
      /* Root rank becomes the master */
      role = SCTK_SHM_MAPPER_ROLE_MASTER;
    }

    /* Now count number of processes by counting masters */
    int is_master = 0;

    if (sctk_get_local_task_rank() == 0) {
      is_master = 1;
    }

    int process_on_node_count = 0;

    MPI_Comm process_master_comm;
    PMPI_Comm_split(node_comm, is_master, cw_rank, &process_master_comm);

    PMPI_Comm_size(process_master_comm, &process_on_node_count);

    if (is_master) {

      size_t to_map_size = ((inner_size / SCTK_SHM_MAPPER_PAGE_SIZE) + 1) *
                           SCTK_SHM_MAPPER_PAGE_SIZE;

      ____mpc_sctk_mpi_alloc_mem_pool.mapped_size = to_map_size;

      /* Prepare to use the MPI handlers */
      sctk_alloc_mapper_handler_t *handler =
          sctk_shm_mpi_handler_init(process_master_comm);

      /* Here we need to map a shared segment */
      ____mpc_sctk_mpi_alloc_mem_pool._pool = sctk_shm_mapper_create(
          to_map_size, role, process_on_node_count, handler);

      sctk_shm_mpi_handler_free(handler);
    }

    PMPI_Comm_free(&node_comm);
    PMPI_Comm_free(&process_master_comm);
  }

  PMPI_Barrier(MPI_COMM_WORLD);

  assume(____mpc_sctk_mpi_alloc_mem_pool._pool != NULL);

  if (do_init) {

    /* Keep pointer to the allocated pool  */
    void *bit_array_page = ____mpc_sctk_mpi_alloc_mem_pool._pool;

    /* After we have a lock */
    ____mpc_sctk_mpi_alloc_mem_pool.lock =
        bit_array_page + SCTK_SHM_MAPPER_PAGE_SIZE;

    /* And to finish the actual memory pool */
    ____mpc_sctk_mpi_alloc_mem_pool.pool =
        ____mpc_sctk_mpi_alloc_mem_pool.lock + 1;

    sctk_nodebug("BASE %p LOCK %p POOL %p", bit_array_page,
                 ____mpc_sctk_mpi_alloc_mem_pool.lock,
                 ____mpc_sctk_mpi_alloc_mem_pool.pool);

    ____mpc_sctk_mpi_alloc_mem_pool.size = pool_size;
    ____mpc_sctk_mpi_alloc_mem_pool.size_left = pool_size;
    sctk_atomics_store_int(____mpc_sctk_mpi_alloc_mem_pool.lock, 0);

    sctk_bit_array_init_buff(&____mpc_sctk_mpi_alloc_mem_pool.mask,
                             SCTK_SHM_MAPPER_PAGE_SIZE * 8, bit_array_page,
                             SCTK_SHM_MAPPER_PAGE_SIZE);

    ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit =
        (pool_size / (SCTK_SHM_MAPPER_PAGE_SIZE * 8));

    MPCHT_init(&____mpc_sctk_mpi_alloc_mem_pool.size_ht, 32);
  }

  mpc_MPI_accumulate_op_lock_init();

  PMPI_Barrier(MPI_COMM_WORLD);

  return 0;
}

int mpc_MPI_allocmem_pool_release() {
  PMPI_Barrier(MPI_COMM_WORLD);

  /* Are all the tasks in the same process ? */
  if (_pool_only_local ||
      (sctk_get_task_number() == sctk_get_local_task_number())) {
    return 0;
  }

  int is_master = 0;

  sctk_spinlock_lock(&_pool_init_lock);

  if (_pool_init_done) {
    is_master = 1;
    _pool_init_done = 0;
  }

  sctk_spinlock_unlock(&_pool_init_lock);

  if (_pool_only_local ||
      (sctk_get_task_number() == sctk_get_local_task_number())) {
    if (is_master) {
      sctk_free(____mpc_sctk_mpi_alloc_mem_pool._pool);
    }
  } else {
    sctk_alloc_shm_remove(____mpc_sctk_mpi_alloc_mem_pool._pool,
                          ____mpc_sctk_mpi_alloc_mem_pool.mapped_size);
  }

  PMPI_Barrier(MPI_COMM_WORLD);

  if (is_master) {
    MPCHT_release(&____mpc_sctk_mpi_alloc_mem_pool.size_ht);
    /* All the rest is static */
    memset(&____mpc_sctk_mpi_alloc_mem_pool, 0,
           sizeof(struct mpc_MPI_allocmem_pool));
  }

  return 0;
}

static inline void mpc_MPI_allocmem_pool_lock() {
  assert(____mpc_sctk_mpi_alloc_mem_pool.lock);
  while (sctk_atomics_cas_int(____mpc_sctk_mpi_alloc_mem_pool.lock, 0, 1)) {
    mpc_thread_yield();
  }
}

static inline void mpc_MPI_allocmem_pool_unlock() {
  assert(____mpc_sctk_mpi_alloc_mem_pool.lock);
  sctk_atomics_store_int(____mpc_sctk_mpi_alloc_mem_pool.lock, 0);
}

void *mpc_MPI_allocmem_pool_alloc(size_t size) {
  /* Are all the tasks in the same process ? */
  if (_pool_only_local ||
      (sctk_get_task_number() == sctk_get_local_task_number())) {
    return sctk_malloc(size);
  }

  /* We are sure that it does not fit */
  if ((____mpc_sctk_mpi_alloc_mem_pool.size < size)) {
    return sctk_malloc(size);
  }

  mpc_MPI_allocmem_pool_lock();

  size_t number_of_bits =
      1 +
      (size + (size % ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit)) /
          ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit;

  /* Now try to find this number of contiguous free bits */
  int i, j;

  struct sctk_bit_array *ba = &____mpc_sctk_mpi_alloc_mem_pool.mask;

  for (i = 0; i < ba->real_size; i++) {
    if (sctk_bit_array_get(ba, i)) {
      /* This bit is taken */
      continue;
    }

    int end_of_seg_off = i + number_of_bits;

    if (ba->real_size <= end_of_seg_off) {
      /* We are too close from the end */
      break;
    }

    if (sctk_bit_array_get(ba, end_of_seg_off) == 0) {
      int found_taken_bit = 0;
      /* Last bit is free it is a good candidate */

      /* Now scan the whole segment */
      for (j = i; (j < (i + number_of_bits)); j++) {
        if (sctk_bit_array_get(ba, j)) {
          found_taken_bit = 1;
          break;
        }
      }

      if (found_taken_bit == 0) {
        /* We found enough space */

        /* Book the bits */
        int k;

        for (k = i; k < (i + number_of_bits); k++) {
          sctk_bit_array_set(ba, k, 1);
        }

        /* Compute address */
        void *addr = ____mpc_sctk_mpi_alloc_mem_pool.pool +
                     (i * ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit);

        /* Store bit size for free for address */
        MPCHT_set(&____mpc_sctk_mpi_alloc_mem_pool.size_ht, (sctk_uint64_t)addr,
                  (void *)number_of_bits);

        sctk_nodebug("ALLOC %ld", number_of_bits);
        mpc_MPI_allocmem_pool_unlock();

        /* Proudly return the address */
        return addr;
      }
    }
    // else
    //{
    //	/* Last bit is taken skip to next */
    //	i = end_of_seg_off + 1;
    //}
  }

  mpc_MPI_allocmem_pool_unlock();

  /* We failed */
  return sctk_malloc(size);
}

int mpc_MPI_allocmem_pool_free(void *ptr) {
  /* Are all the tasks in the same process ? */
  if (_pool_only_local ||
      (sctk_get_task_number() == sctk_get_local_task_number())) {
    sctk_free(ptr);
    return 0;
  }

  mpc_MPI_allocmem_pool_lock();

  void *size_in_ptr =
      MPCHT_get(&____mpc_sctk_mpi_alloc_mem_pool.size_ht, (sctk_uint64_t)ptr);

  if (size_in_ptr) {

    size_t size = (size_t)size_in_ptr;

    sctk_nodebug("FREE %ld", size);

    /* Compute bit_offset */
    size_t off = (ptr - ____mpc_sctk_mpi_alloc_mem_pool.pool) /
                 ____mpc_sctk_mpi_alloc_mem_pool.space_per_bit;
    size_t number_of_bits = size;

    size_t i;

    for (i = off; i < (off + number_of_bits); i++) {
      sctk_bit_array_set(&____mpc_sctk_mpi_alloc_mem_pool.mask, i, 0);
    }

    MPCHT_delete(&____mpc_sctk_mpi_alloc_mem_pool.size_ht, (sctk_uint64_t)ptr);

    mpc_MPI_allocmem_pool_unlock();
  } else {
    mpc_MPI_allocmem_pool_unlock();
    /* If we are here the pointer was allocated */
    sctk_free(ptr);
  }

  return 0;
}

int mpc_MPI_allocmem_is_in_pool(void *ptr) {
  return _mpc_MPI_allocmem_is_in_pool(ptr);
}

/* This is the accumulate pool protection */

static sctk_spinlock_t *__accululate_master_lock = NULL;
static sctk_spinlock_t __static_lock_for_acc = 0;

void mpc_MPI_accumulate_op_lock_init_shared() {
  __accululate_master_lock = &__static_lock_for_acc;
}

void mpc_MPI_accumulate_op_lock_init() {

  /* First build a comm for each node */
  MPI_Comm node_comm;

  int cw_rank;
  PMPI_Comm_rank(MPI_COMM_WORLD, &cw_rank);

  PMPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, cw_rank,
                       MPI_INFO_NULL, &node_comm);

  int my_rank, comm_size, node_size;
  PMPI_Comm_rank(node_comm, &my_rank);
  PMPI_Comm_size(node_comm, &comm_size);

  void *p = NULL;

  if (!my_rank) {
    p = mpc_MPI_allocmem_pool_alloc(sizeof(sctk_spinlock_t));
    *((sctk_spinlock_t *)p) = 0;
  }

  PMPI_Bcast(&p, 1, MPI_AINT, 0, node_comm);

  __accululate_master_lock = (sctk_spinlock_t *)p;

  PMPI_Comm_free(&node_comm);
}

void mpc_MPI_accumulate_op_lock() {
  assert(__accululate_master_lock != NULL);
  sctk_spinlock_lock_yield(__accululate_master_lock);
}

void mpc_MPI_accumulate_op_unlock() {
  assert(__accululate_master_lock != NULL);
  sctk_spinlock_unlock(__accululate_master_lock);
}
