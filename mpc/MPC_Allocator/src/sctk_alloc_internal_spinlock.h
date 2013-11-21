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
/* #   - Valat Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ALLOC_INTERNAL_SPINLOCK_H_
#define SCTK_ALLOC_INTERNAL_SPINLOCK_H_

/********************************* INCLUDES *********************************/
#include "sctk_alloc_common.h"
#include <opa_primitives.h>

/********************************** TYPES ***********************************/
typedef OPA_int_t sctk_alloc_internal_spinlock_t;

/********************************* MACROS ***********************************/
#define SCTK_ALLOC_INTERNAL_LOCKED 1
#define SCTK_ALLOC_INTERNAL_UNLOCKED 0
#define SCTK_ALLOC_INTERNAL_SPINLOCK_INITIALIZER OPA_INT_T_INITIALIZER(0)

/********************************* FUNCTION *********************************/
static inline void sctk_alloc_cpu_relax(void)
{
	#ifdef __MIC__
	_mm_delay_32(20);
	#else
	__asm__ __volatile__("rep;nop":::"memory");
	#endif
}
/********************************* FUNCTION *********************************/
static inline void sctk_alloc_internal_spinlock_init(sctk_alloc_internal_spinlock_t * lock)
{
	OPA_store_int(lock,SCTK_ALLOC_INTERNAL_UNLOCKED);
}

/********************************* FUNCTION *********************************/
static inline void sctk_alloc_internal_spinlock_lock(sctk_alloc_internal_spinlock_t * lock)
{
	while (OPA_swap_int(lock,SCTK_ALLOC_INTERNAL_LOCKED)) {
		do {
			sctk_alloc_cpu_relax ();
		} while (OPA_load_int(lock));
	}
}

/********************************* FUNCTION *********************************/
static inline void sctk_alloc_internal_spinlock_unlock(sctk_alloc_internal_spinlock_t * lock)
{
	OPA_store_int(lock,SCTK_ALLOC_INTERNAL_UNLOCKED);
}

/********************************* FUNCTION *********************************/
static inline int sctk_alloc_internal_spinlock_trylock(sctk_alloc_internal_spinlock_t * lock)
{
	return OPA_swap_int(lock,SCTK_ALLOC_INTERNAL_LOCKED);
}

/********************************* FUNCTION *********************************/
static inline void sctk_alloc_internal_spinlock_destroy(sctk_alloc_internal_spinlock_t * lock)
{
	//do nothing
}

/********************************* FUNCTION *********************************/
static inline int sctk_alloc_internal_spinlock_is_lock(sctk_alloc_internal_spinlock_t * lock)
{
	return (OPA_load_int(lock) == SCTK_ALLOC_INTERNAL_LOCKED);
}

#endif /* SCTK_ALLOC_INTERNAL_SPINLOCK_H_ */
