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
/* #   - Adam Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_LOCK_H
#define SCTK_ALLOC_LOCK_H

/************************** HEADERS ************************/
#ifndef _WIN32
	#include <pthread.h>
#else
	#include <Windows.h>
#endif

/************************** MACROS *************************/
#ifdef MPC_Threads
	#include "sctk_spinlock.h"
#endif

#ifdef __MIC__
	#define SCTK_ALLOC_USE_INTERNAL_LOCKS
#endif

/************************** MACROS *************************/
//Can't use pthread mutex into MPC as we are called before thread init, so use spinlocks.
//But in POSIX standard, spinlock didn't have static INITIALIZER, so prefer to use mutex in
//this case
//pthread_once wan't usable in MPC as it's based on mutex too.
//on windows, seams to get a bug on our internal spinlock implementation with optim flags
#ifdef _WIN32
	//@TODO find a way to use static mutexes, here it didn't failed as long as we use the DLL init step
	//which is called no multithread, but it may fail one day
	#define SCTK_ALLOC_INIT_LOCK_TYPE int
	#define SCTK_ALLOC_INIT_LOCK_INITIALIZER 0
	#define SCTK_ALLOC_INIT_LOCK_LOCK(x) do {} while(0)
	#define SCTK_ALLOC_INIT_LOCK_UNLOCK(x) do {} while(0)
	#define sctk_alloc_spinlock_t CRITICAL_SECTION
	#define sctk_alloc_spinlock_init(x,y) InitializeCriticalSection((x))
	#define sctk_alloc_spinlock_lock(x) EnterCriticalSection(x)
	#define sctk_alloc_spinlock_unlock(x) LeaveCriticalSection(x)
	#define sctk_alloc_spinlock_trylock(x) (!TryEnterCriticalSection(x))
	#define sctk_alloc_spinlock_destroy(x) DeleteCriticalSection(x)
#elif defined(MPC_Threads)
	#define SCTK_ALLOC_INIT_LOCK_TYPE sctk_spinlock_t
	#define SCTK_ALLOC_INIT_LOCK_INITIALIZER SCTK_SPINLOCK_INITIALIZER
	#define SCTK_ALLOC_INIT_LOCK_LOCK(x) sctk_spinlock_lock(x)
	#define SCTK_ALLOC_INIT_LOCK_UNLOCK(x) sctk_spinlock_unlock(x)
	#define sctk_alloc_spinlock_t sctk_spinlock_t
	#define sctk_alloc_spinlock_init(x,y) sctk_spinlock_init(x,y)
	#define sctk_alloc_spinlock_lock(x) sctk_spinlock_lock(x)
	#define sctk_alloc_spinlock_unlock(x) sctk_spinlock_unlock(x)
	#define sctk_alloc_spinlock_trylock(x) sctk_spinlock_trylock(x)
	#define sctk_alloc_spinlock_destroy(x) do{}while(0)
#elif defined(SCTK_USE_PTHREAD_MUTEX)
	#define SCTK_ALLOC_INIT_LOCK_TYPE pthread_mutex_t
	#define SCTK_ALLOC_INIT_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
	#define SCTK_ALLOC_INIT_LOCK_LOCK(x) pthread_mutex_lock(x)
	#define SCTK_ALLOC_INIT_LOCK_UNLOCK(x) pthread_mutex_unlock(x)
	#define sctk_alloc_spinlock_t pthread_mutex_t
	#define sctk_alloc_spinlock_init(x,y) pthread_mutex_init(x,NULL)
	#define sctk_alloc_spinlock_lock(x) pthread_mutex_lock(x)
	#define sctk_alloc_spinlock_unlock(x) pthread_mutex_unlock(x)
	#define sctk_alloc_spinlock_trylock(x) pthread_mutex_trylock(x)
	#define sctk_alloc_spinlock_destroy(x) pthread_mutex_destroy(x)
#elif defined(SCTK_ALLOC_USE_INTERNAL_LOCKS)
	#include "sctk_alloc_internal_spinlock.h"
	#define SCTK_ALLOC_INIT_LOCK_TYPE sctk_alloc_internal_spinlock_t
	#define SCTK_ALLOC_INIT_LOCK_INITIALIZER SCTK_ALLOC_INTERNAL_SPINLOCK_INITIALIZER
	#define SCTK_ALLOC_INIT_LOCK_LOCK(x) sctk_alloc_internal_spinlock_lock(x)
	#define SCTK_ALLOC_INIT_LOCK_UNLOCK(x) sctk_alloc_internal_spinlock_unlock(x)
	#define sctk_alloc_spinlock_t sctk_alloc_internal_spinlock_t
	#define sctk_alloc_spinlock_init(x,y) sctk_alloc_internal_spinlock_init(x)
	#define sctk_alloc_spinlock_lock(x) sctk_alloc_internal_spinlock_lock(x)
	#define sctk_alloc_spinlock_unlock(x) sctk_alloc_internal_spinlock_unlock(x)
	#define sctk_alloc_spinlock_trylock(x) sctk_alloc_internal_spinlock_trylock(x)
	#define sctk_alloc_spinlock_destroy(x) sctk_alloc_internal_spinlock_destroy(x)
#else //MPC_Threads
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
#endif //MPC_Threads

#endif //SCTK_ALLOC_LOCK_H
