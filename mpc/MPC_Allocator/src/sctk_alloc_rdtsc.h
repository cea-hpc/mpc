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
#ifndef SCTK_ALLOC_RDTSC_H_
#define SCTK_ALLOC_RDTSC_H_

#ifdef _MSC_VER
	#define __inline__ __inline
	#define __sctk_rdtscll(val) 0
#else
	#define __inline__ inline
#endif

/************************** MACROS *************************/
// Define rdtscll for x86_64 arch
#ifdef __x86_64__
	#define __sctk_rdtscll(val) do { \
			unsigned int __a,__d; \
			asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
			(val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
	} while(0)
#endif

/************************** MACROS *************************/
// Define rdtscll for i386 arch
#ifdef __i386__
	#define __sctk_rdtscll(val) { \
		asm volatile ("rdtsc" : "=A"(val)); \
		}
#endif

/************************* FUNCTION ************************/
static __inline__ unsigned long long sctk_alloc_rdtsc() {
    unsigned long long t;
    __sctk_rdtscll(t);
    return t;
}

/************************* FUNCTION ************************/
static __inline__ void sctk_alloc_fill_rdtsc(unsigned long * value)
{
	__sctk_rdtscll(*value);
}

#endif //SCTK_ALLOC_RDTSC_H_
