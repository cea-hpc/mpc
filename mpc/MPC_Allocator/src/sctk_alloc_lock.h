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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_LOCK_H
#define SCTK_ALLOC_LOCK_H

/************************** HEADERS ************************/
#include <pthread.h>

/************************** MACROS *************************/
#ifdef MPC_check_compatibility
#define SCTK_ALLOC_ENABLE_INTERNAL_SPINLOCK 1
#endif

/************************** HEADERS ************************/
#ifdef SCTK_ALLOC_ENABLE_INTERNAL_SPINLOCK
#include "sctk_alloc_spinlock.h"
#endif //SCTK_ALLOC_ENABLE_INTERNAL_SPINLOCK

/************************** MACROS *************************/
//Can't use pthread mutex into MPC as we are called before thread init, so use spinlocks.
//But in POSIX standard, spinlock didn't have static INITIALIZER, so prefer to use mutex in
//this case
//pthread_once wan't usable in MPC as it's based on mutex too.
#ifdef SCTK_ALLOC_ENABLE_INTERNAL_SPINLOCK
#define SCTK_ALLOC_INIT_LOCK_TYPE sctk_alloc_spinlock_t
#define SCTK_ALLOC_INIT_LOCK_INITIALIZER SCTK_ALLOC_SPINLOCK_INITIALIZER
#define SCTK_ALLOC_INIT_LOCK_LOCK(x) sctk_alloc_spinlock_lock(x)
#define SCTK_ALLOC_INIT_LOCK_UNLOCK(x) sctk_alloc_spinlock_unlock(x)
#else //SCTK_ALLOC_ENABLE_INTERNAL_SPINLOCK
#define SCTK_ALLOC_INIT_LOCK_TYPE pthread_mutex_t
#define SCTK_ALLOC_INIT_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define SCTK_ALLOC_INIT_LOCK_LOCK(x) pthread_mutex_lock(x)
#define SCTK_ALLOC_INIT_LOCK_UNLOCK(x) pthread_mutex_unlock(x)
#define sctk_alloc_spinlock_t pthread_spinlock_t
#define sctk_alloc_spinlock_init(x,y) pthread_spin_init(x,y)
#define sctk_alloc_spinlock_lock(x) pthread_spin_lock(x)
#define sctk_alloc_spinlock_unlock(x) pthread_spin_unlock(x)
#define sctk_alloc_spinlock_trylock(x) pthread_spin_trylock(x)
#define sctk_alloc_spinlock_destroy(x) pthread_spin_destroy(x)
#endif //SCTK_ALLOC_ENABLE_INTERNAL_SPINLOCK

#endif //SCTK_ALLOC_LOCK_H
