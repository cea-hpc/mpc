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

#include <mpc_arch.h>

#if defined(MPC_I686_ARCH) || defined(MPC_X86_64_ARCH)
static inline void __sctk_cpu_relax ()
{
  #ifdef __MIC__
    _mm_delay_32(400);
  #else
    __asm__ __volatile__ ("rep;nop":::"memory");
  #endif
}
#elif defined(MPC_IA64_ARCH)
static inline void __sctk_cpu_relax ()
{
  __asm__ __volatile__ ("hint @pause":::"memory");
}
#else
static inline void __sctk_cpu_relax ()
{
  sched_yield ();
}
#endif

void sctk_cpu_relax ()
{
  __sctk_cpu_relax ();
}
