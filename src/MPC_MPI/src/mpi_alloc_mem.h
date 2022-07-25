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

#ifndef MPI_ALLOC_MEM_H
#define MPI_ALLOC_MEM_H

#include "mpc_common_asm.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"
#include <stdlib.h>

struct mpc_MPI_allocmem_pool {
  void *_pool;
  void *pool;
  size_t size;
  size_t size_left;
  size_t mapped_size;
  size_t space_per_bit;
  OPA_int_t *lock;
  struct mpc_common_bit_array mask;
  struct mpc_common_hashtable size_ht;
};

extern struct mpc_MPI_allocmem_pool _mpc_MPI_mem_pool;

int mpc_MPI_allocmem_pool_init();
int mpc_MPI_allocmem_pool_release();

void *mpc_MPI_allocmem_pool_alloc(size_t size);
void *mpc_MPI_allocmem_pool_alloc_check(size_t size, int * is_shared);
int mpc_MPI_allocmem_pool_free(void *ptr);
int mpc_MPI_allocmem_pool_free_size(void *ptr, ssize_t known_size);

int mpc_MPI_allocmem_is_in_pool(void *ptr);

static inline int _mpc_MPI_allocmem_is_in_pool(void *ptr) {
  struct mpc_MPI_allocmem_pool *p = &_mpc_MPI_mem_pool;
  if ((p->pool <= ptr) && (ptr < (p->pool + p->size))) {
    return 1;
  }

  return 0;
}

static inline size_t _mpc_MPI_allocmem_relative_addr(void *in_pool_addr)
{
  return in_pool_addr - _mpc_MPI_mem_pool.pool;
}

static inline void * _mpc_MPI_allocmem_abs_addr(size_t relative_addr)
{
  return (char *)_mpc_MPI_mem_pool.pool + relative_addr;
}

void mpc_MPI_accumulate_op_lock_init_shared();
void mpc_MPI_accumulate_op_lock_init();
void mpc_MPI_accumulate_op_lock();
void mpc_MPI_accumulate_op_unlock();

#endif /* MPI_ALLOC_MEM_H */
