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

#ifndef SCTK_ALLOC_SPINLOCK_H
#define SCTK_ALLOC_SPINLOCK_H

/************************** HEADERS ************************/
#include "sctk_alloc_spinlock_asm.h"

/************************** TYPES **************************/
typedef volatile unsigned int sctk_alloc_spinlock_t;

/************************** MACROS *************************/
#define SCTK_ALLOC_SPINLOCK_INITIALIZER 0
// #define sctk_alloc_spinlock_init(a,b) do{*((sctk_alloc_spinlock_t*)(a))=b;}while(0)
#define expect_true(expr) __builtin_expect(!!(expr),1)
#define sctk_alloc_spinlock_destroy(x) //spinlock_destroy(x)

/************************* FUNCTION ************************/
static inline int sctk_alloc_spinlock_init(sctk_alloc_spinlock_t * lock,int flag)
{
	*lock = 0;
}

/************************* FUNCTION ************************/
static inline int sctk_alloc_spinlock_lock (sctk_alloc_spinlock_t * lock){
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

/************************* FUNCTION ************************/
static inline int sctk_alloc_spinlock_unlock (sctk_alloc_spinlock_t * lock)
{
	*lock = 0;
	return 0;
}

/************************* FUNCTION ************************/
static inline int sctk_alloc_spinlock_trylock (sctk_alloc_spinlock_t * lock)
{
	return sctk_test_and_set (lock);
}

#endif //SCTK_ALLOC_SPINLOCK_H
