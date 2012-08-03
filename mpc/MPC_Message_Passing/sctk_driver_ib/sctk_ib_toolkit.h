/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk.h"
#include "sctk_launch.h"
#include "string.h"

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__IB_TOOLKIT_H_
#define __SCTK__IB_TOOLKIT_H_

#include "sctk_shell_colors.h"

#define SMALL_BUFFER_SIZE (4*1024)
#define HAVE_SHELL_COLORS

#ifndef SCTK_IB_MODULE_NAME
#define SCTK_IB_MODULE_NAME "NONE"
#endif

void sctk_ib_toolkit_print_backtrace(void);

#ifdef SCTK_IB_MODULE_DEBUG
__UNUSED__ static void sctk_ib_debug(const char *fmt, ...)
{
  va_list ap;
  char buff[SMALL_BUFFER_SIZE];

  if (sctk_get_verbosity() >= 2)
  {
    va_start (ap, fmt);
#ifdef HAVE_SHELL_COLORS
    snprintf (buff, SMALL_BUFFER_SIZE,
        "%s "SCTK_COLOR_RED_BOLD([%5s])" %s\n", sctk_print_debug_infos(),
        SCTK_IB_MODULE_NAME, fmt);
#else
    snprintf (buff, SMALL_BUFFER_SIZE,
        "%s [%5s] %s\n", sctk_print_debug_infos(),
        SCTK_IB_MODULE_NAME_STR), fmt);
#endif

    sctk_noalloc_vfprintf (stderr, buff, ap);
    va_end (ap);
  }
}
#else
#if defined(__GNU_COMPILER) || defined(__INTEL_COMPILER)
#define sctk_ib_debug(fmt,...) (void)(0)
#else
  static inline void sctk_ib_debug (const char *fmt, ...)
  {
  }
#endif
#endif

#if defined(__GNU_COMPILER) || defined(__INTEL_COMPILER)
#define sctk_ib_nodebug(fmt,...) (void)(0)
#else
  static inline void sctk_ib_nodebug (const char *fmt, ...)
  {
  }
#endif
#define LOAD_RAIL(x) sctk_ib_rail_info_t *rail_ib = &(x)->network.ib;
#define LOAD_CONFIG(x) sctk_ib_config_t *config = (x)->config;
#define LOAD_MMU(x)    sctk_ib_mmu_t* mmu = (x)->mmu;
#define LOAD_DEVICE(x)    sctk_ib_device_t* device = (x)->device;
#define LOAD_POOL(x)    sctk_ibuf_pool_t* pool = (x)->pool_buffers;
#define LOAD_CP(x)    sctk_ib_cp_t* cp = (x)->cp;
#define LOAD_PROFILER(x)    sctk_ib_prof_t* profiler = (x)->profiler;


#endif
#endif
