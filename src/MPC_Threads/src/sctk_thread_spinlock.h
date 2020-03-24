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

#ifndef __SCTK_THREAD_SPINLOCK_H_
#define __SCTK_THREAD_SPINLOCK_H_

#include <stdlib.h>
#include "mpcthread_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_scheduler.h"
#include "mpc_common_spinlock.h"

typedef enum
{
	sctk_spin_unitialized,
	sctk_spin_initialized,
	sctk_spin_destroyed
} sctk_spin_state;

typedef struct sctk_thread_generic_spinlock_s
{
	mpc_common_spinlock_t            lock;
	sctk_spin_state                  state;
	sctk_thread_generic_scheduler_t *owner;
}sctk_thread_generic_spinlock_t;

#define SCTK_THREAD_GENERIC_SPINLOCK_INIT    { SCTK_SPINLOCK_INITIALIZER, sctk_spin_unitialized, NULL }

#define SCTK_SPIN_DELAY                      10

int
sctk_thread_generic_spinlocks_spin_destroy(sctk_thread_generic_spinlock_t *spinlock);

int
sctk_thread_generic_spinlocks_spin_init(sctk_thread_generic_spinlock_t *spinlock,
                                        int pshared);

int
sctk_thread_generic_spinlocks_spin_lock(sctk_thread_generic_spinlock_t *spinlock,
                                        sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_spinlocks_spin_trylock(sctk_thread_generic_spinlock_t *spinlock,
                                           sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_spinlocks_spin_unlock(sctk_thread_generic_spinlock_t *spinlock,
                                          sctk_thread_generic_scheduler_t *sched);

void
sctk_thread_generic_spinlocks_init();

#endif
