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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __SCTK_SPINLOCK_H__
#define __SCTK_SPINLOCK_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include "sctk_config.h"
#if defined (MPC_Threads)
#include "sctk_pthread_compatible_structures.h"
#else
  typedef volatile unsigned int sctk_spinlock_t;
#define SCTK_SPINLOCK_INITIALIZER 0
#endif
#define sctk_spinlock_init(a,b) do{*((sctk_spinlock_t*)(a))=b;}while(0)
  int sctk_spinlock_lock (sctk_spinlock_t * lock);
  int sctk_spinlock_unlock (sctk_spinlock_t * lock);
  int sctk_spinlock_trylock (sctk_spinlock_t * lock);

  typedef struct {
    sctk_spinlock_t lock;
    sctk_spinlock_t writer_lock;
    volatile int reader_number;
  }sctk_spin_rwlock_t;
#define SCTK_SPIN_RWLOCK_INITIALIZER {SCTK_SPINLOCK_INITIALIZER,SCTK_SPINLOCK_INITIALIZER,0}

  int sctk_spinlock_read_lock (sctk_spin_rwlock_t * lock);
  int sctk_spinlock_write_lock (sctk_spin_rwlock_t * lock);
  int sctk_spinlock_read_unlock (sctk_spin_rwlock_t * lock);
  int sctk_spinlock_write_unlock (sctk_spin_rwlock_t * lock);

#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
