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
#include "sctk_atomics.h"

#include <pthread.h>
#include <stdio.h>
#include <sched.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define sctk_get_time_stamp               sctk_atomics_get_timestamp
/* #define sctk_get_time_stamp_gettimeofday  sctk_atomics_get_timestamp */

#define sctk_max(a, b)  ((a) > (b) ? (a) : (b))
#define sctk_min(a, b)  ((a) < (b) ? (a) : (b))

  double sctk_get_time_stamp (void);

  double sctk_get_time_stamp_gettimeofday();

  /*
   * CPU relax implemtation
   * */
  typedef volatile unsigned int sctk_atomic_test_t;

#if defined(SCTK_COMPILER_ACCEPT_ASM)
#define LOCK "lock ; "
#if defined(SCTK_i686_ARCH_SCTK) || defined(SCTK_x86_64_ARCH_SCTK)
  static __inline__ void __sctk_cpu_relax ()
  {
    __asm__ __volatile__ ("rep;nop":::"memory");
  }
#elif defined(SCTK_ia64_ARCH_SCTK)
  static __inline__ void __sctk_cpu_relax ()
  {
    __asm__ __volatile__ ("hint @pause":::"memory");
  }
#elif defined(SCTK_sparc_ARCH_SCTK)
#warning sctk_cpu_relax not available for the current architecture. Falling back to sched_yield()
  static __inline__ void __sctk_cpu_relax ()
  {
    sched_yield ();
  }

#else
#if defined(__GNU_COMPILER) || defined(__INTEL_COMPILER)
#warning "Unsupported architecture using default asm"
#endif

#define SCTK_USE_DEFAULT_ASM
  static __inline__ void __sctk_cpu_relax ()
  {
    sched_yield ();
  }
#endif

#ifndef __SCTK_ASM_C_
  static __inline__ void sctk_cpu_relax ()
  {
    __sctk_cpu_relax ();
  }
#endif

#else
  void sctk_cpu_relax (void);
#endif

#ifdef __cplusplus
}
#endif
#endif
