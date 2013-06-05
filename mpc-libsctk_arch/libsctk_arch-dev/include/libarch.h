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
#ifndef __libarch__h__
#define __libarch__h__

#include <libarchconfig.h>


#if defined(__i686__) && !defined(__x86_64__)
#define SCTK_i686_ARCH_SCTK
#define SCTK_ARCH_SCTK  SCTK_i686_ARCH_SCTK
#elif defined(__x86_64__)
#define SCTK_x86_64_ARCH_SCTK
#define SCTK_ARCH_SCTK  SCTK_x86_64_ARCH_SCTK
#elif defined(__ia64__)
#define SCTK_ia64_ARCH_SCTK
#define SCTK_ARCH_SCTK  SCTK_ia64_ARCH_SCTK
#elif defined(__arm__)
#define SCTK_arm_ARCH_SCTK
#define SCTK_ARCH_SCTK  SCTK_arm_ARCH_SCTK
#else
  #error "Unknown architecture"
#endif
#endif
