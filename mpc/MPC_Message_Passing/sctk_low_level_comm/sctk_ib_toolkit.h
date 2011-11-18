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

struct sctk_ibuf_pool_s;
struct sctk_ib_mmu_s;
struct sctk_ib_config_s;

typedef struct sctk_rail_info_ib_s {
  struct sctk_ibuf_pool_s *pool_buffers;
  struct sctk_ib_mmu_s    *mmu;
  struct sctk_ib_config_s *config;
} sctk_rail_info_ib_t;

#define LOAD_CONFIG(x) sctk_ib_config_t *config = (x)->config;
#define LOAD_MMU(x)    sctk_ib_mmu_t* mmu = (x)->mmu;

/* const for debugging IB */
#define DEBUG_IB_MMU
