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
#define SCTK_COMPILER_ACCEPT_ASM
#include "sctk_config.h"
#include "sctk_spinlock.h"
#include "sctk_asm.h"
#include <sched.h>
#include <stdlib.h>
#include <math.h>

int sctk_spinlock_lock_yield (sctk_spinlock_t * lock)
{
  unsigned int *p = (unsigned int *) lock;
  while (expect_true (sctk_test_and_set (p)))
	{
      do
	{
	int i;
#ifdef MPC_Threads
	sctk_thread_yield();
#else
	sched_yield();
#endif
	for(i = 0; (*p) && (i < 100); i++){
		 sctk_cpu_relax ();
	}
	}
      while (*p);
    }

  return 0 ;
}

int
sctk_spinlock_lock (sctk_spinlock_t * lock)
{
  unsigned int *p = (unsigned int *) lock;
  while (expect_true (sctk_test_and_set (p)))
    {
      do
	{
    	sctk_cpu_relax ();
	}
      while (*p);
    }

  return 0 ;
}

int
sctk_spinlock_trylock (sctk_spinlock_t * lock)
{
  return sctk_test_and_set (lock);
}

int
sctk_spinlock_unlock (sctk_spinlock_t * lock)
{
  *lock = 0;

  return 0;
}

int
sctk_spinlock_read_lock (sctk_spin_rwlock_t * lock)
{
#warning "To optimze with atomics"
  sctk_spinlock_lock(&(lock->writer_lock));
  sctk_spinlock_lock(&(lock->lock));
  lock->reader_number ++;
  sctk_spinlock_unlock(&(lock->lock));
  sctk_spinlock_unlock(&(lock->writer_lock));
  return 0;
}

int
sctk_spinlock_write_lock (sctk_spin_rwlock_t * lock)
{
#warning "To optimze with atomics"
  sctk_spinlock_lock(&(lock->writer_lock));
  while(expect_true (lock->reader_number != 0)) {
    sctk_cpu_relax ();
  }
  return 0;
}

int
sctk_spinlock_read_unlock (sctk_spin_rwlock_t * lock)
{
#warning "To optimze with atomics"
  sctk_spinlock_lock(&(lock->lock));
  lock->reader_number --;
  sctk_spinlock_unlock(&(lock->lock));
  return 0;
}

int
sctk_spinlock_write_unlock (sctk_spin_rwlock_t * lock)
{
#warning "To optimze with atomics"
  sctk_spinlock_unlock(&(lock->writer_lock));
  return 0;
}
