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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk.h"
#include "string.h"

#include "sctk_shell_colors.h"

#define SMALL_BUFFER_SIZE (4*1024)
#define HAVE_SHELL_COLORS

#ifndef SCTK_IB_MODULE_NAME
#define SCTK_IB_MODULE_NAME "NONE"
#endif

void sctk_ib_debug(const char *fmt, ...);

#if defined(__GNU_COMPILER) || defined(__INTEL_COMPILER)
#define sctk_ib_nodebug(fmt,...) (void)(0)
#else
  static inline void sctk_ib_nodebug (const char *fmt, ...)
  {
  }
#endif

#define LOAD_CONFIG(x) sctk_ib_config_t *config = (x)->config;
#define LOAD_MMU(x)    sctk_ib_mmu_t* mmu = (x)->mmu;
#define LOAD_DEVICE(x)    sctk_ib_device_t* device = (x)->device;
#define LOAD_POOL(x)    sctk_ibuf_pool_t* pool = (x)->pool_buffers;

/* const for debugging IB */
#define DEBUG_IB_MMU
#define DEBUG_IB_BUFS
