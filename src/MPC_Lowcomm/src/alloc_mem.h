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

#ifndef ALLOC_MEM_H
#define ALLOC_MEM_H

#include <mpc_lowcomm.h>

#include "mpc_common_asm.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_datastructure.h"
#include "mpc_common_spinlock.h"

#include <stdlib.h>

struct mpc_lowcomm_allocmem_pool {
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

extern struct mpc_lowcomm_allocmem_pool __mpc_lowcomm_memory_pool;

int mpc_lowcomm_allocmem_pool_init();
int mpc_lowcomm_allocmem_pool_release();

void mpc_lowcomm_accumulate_op_lock_init_shared();
void mpc_lowcomm_accumulate_op_lock_init();
void mpc_lowcomm_accumulate_op_lock();
void mpc_lowcomm_accumulate_op_unlock();

#endif /* ALLOC_MEM_H */
