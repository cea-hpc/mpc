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
#ifndef __MPC___DEBUG__
#define __MPC___DEBUG__

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdarg.h>
#include <mpc_config.h>

  typedef struct
  {
    char *ptr;			/*instruction pointer (needed) */
    char *name;			/*function name (out) */
    char *dir;			/*relative source file directory (out) */
    char *file;			/*source file (out) */
    char *absolute;		/*absolute source file directory (out) */
    unsigned long line;		/*line number (out) */
  } mpc_addr2line_t;

  extern void mpc_print_backtrace (const char *format, ...);
  extern void mpc_debug_resolve (mpc_addr2line_t * tab, int size);
#ifdef __cplusplus
}				/* end of extern "C" */
#endif
#endif
