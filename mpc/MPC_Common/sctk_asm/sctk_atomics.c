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
/* #   - PULVERAIL Sebastien sebastien.pulverail@sogeti.com               # */
/* #   - COTTRET Maxime maxime.cottret@sogeti.com                         # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_atomics.h"

#include <sys/time.h>

#if defined(SCTK_ia64_ARCH_SCTK) && defined(SCTK_COMPILER_ACCEPT_ASM)
double
sctk_atomics_get_timestamp ()
{
  unsigned long t;
  __asm__ volatile ("mov %0=ar%1":"=r" (t):"i" (44));
  return (double) t;
}
#elif defined(SCTK_i686_ARCH_SCTK) && defined(SCTK_COMPILER_ACCEPT_ASM)
double
sctk_atomics_get_timestamp ()
{
  unsigned long long t;
  __asm__ volatile ("rdtsc":"=A" (t));
  return (double) t;
}
#elif defined(SCTK_x86_64_ARCH_SCTK) && defined(SCTK_COMPILER_ACCEPT_ASM)
double
sctk_atomics_get_timestamp ()
{
  unsigned int a;
  unsigned int d;
  unsigned long t;
  __asm__ volatile ("rdtsc":"=a" (a), "=d" (d));
  t = ((unsigned long) a) | (((unsigned long) d) << 32);
  return (double) t;
}
#else
#warning "Use get time of day for profiling"
double
sctk_atomics_get_timestamp ()
{
  struct timeval tp;
  gettimeofday (&tp, NULL);
  return tp.tv_usec + tp.tv_sec * 1000000;
}
#endif
