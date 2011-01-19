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
#ifndef __SCTK_ASM_H_
#define __SCTK_ASM_H_

#include "sctk_config.h"
#include <stdio.h>
#include <sched.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef volatile unsigned int sctk_atomic_test_t;
/*   typedef struct */
/*   { */
/*     volatile int counter; */
/*   } sctk_atomic_t; */

#if defined(SCTK_COMPILER_ACCEPT_ASM)
#define LOCK "lock ; "
#if defined(SCTK_i686_ARCH_SCTK) || defined(SCTK_x86_64_ARCH_SCTK)
  static __inline__ int __sctk_test_and_set (sctk_atomic_test_t * atomic)
  {
    int ret;

    __asm__ __volatile__ ("lock; xchgl %0, %1":"=r" (ret),
			  "=m" (*atomic):"0" (1), "m" (*atomic):"memory");
      return ret;
  }
  static __inline__ void __sctk_cpu_relax ()
  {
    __asm__ __volatile__ ("rep;nop":::"memory");
  }
#elif defined(SCTK_ia64_ARCH_SCTK)
  static __inline__ int __sctk_test_and_set (sctk_atomic_test_t * atomic)
  {
    int ret;

    __asm__ __volatile__ ("xchg4 %0=%1, %2":"=r" (ret),
			  "=m" (*atomic):"0" (1), "m" (*atomic):"memory");
      return ret;
  }

  static __inline__ void __sctk_cpu_relax ()
  {
    __asm__ __volatile__ ("hint @pause":::"memory");
  }
#elif defined(SCTK_sparc_ARCH_SCTK)
  static __inline__ int __sctk_test_and_set (sctk_atomic_test_t * spinlock)
  {
    char ret = 0;

    __asm__ __volatile__ ("ldstub [%0], %1":"=r" (spinlock),
			  "=r" (ret):"0" (spinlock), "1" (ret):"memory");

      return (unsigned) ret;
  }
#warning "to optimize"
  static __inline__ void __sctk_cpu_relax ()
  {
    sched_yield ();
  }
#else
#if defined(__GNU_COMPILER) || defined(__INTEL_COMPILER)
#warning "Unsupported architecture using default asm"
#endif

#define SCTK_USE_DEFAULT_ASM
  static __inline__ int __sctk_test_and_set (sctk_atomic_test_t * atomic)
  {
    return __asm_default_sctk_test_and_set (atomic);
  }
  static __inline__ void __sctk_cpu_relax ()
  {
    sched_yield ();
  }
#endif


#ifndef __SCTK_ASM_C_
  static __inline__ int sctk_test_and_set (sctk_atomic_test_t * atomic)
  {
    return __sctk_test_and_set (atomic);
  }

  static __inline__ void sctk_cpu_relax ()
  {
    __sctk_cpu_relax ();
  }
#endif


#else
  int sctk_test_and_set (sctk_atomic_test_t * atomic);
  void sctk_cpu_relax (void);
#endif

/*   static inline void sctk_atomic_set (sctk_atomic_t * atomic, int val) */
/*   { */
/*     atomic->counter = val; */
/*   } static inline int sctk_atomic_read (sctk_atomic_t * atomic) */
/*   { */
/*     return atomic->counter; */
/*   } */

#define sctk_max(a, b)  ((a) > (b) ? (a) : (b))
#define sctk_min(a, b)  ((a) < (b) ? (a) : (b))
  int __asm_default_sctk_test_and_set (sctk_atomic_test_t * atomic);

  double sctk_get_time_stamp (void);






#ifdef __cplusplus
}
#endif
#endif
