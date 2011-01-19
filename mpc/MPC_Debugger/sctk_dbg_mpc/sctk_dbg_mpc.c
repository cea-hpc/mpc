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
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread_dbg.h"
#include "sctk_message_passing_dbg.h"
#include <pthread.h>
#include <stdarg.h>

static volatile int sctk_dbg_initialized = 0;
static pthread_mutex_t sctk_dbg_lock = PTHREAD_MUTEX_INITIALIZER;

void
sctk_debug_printf (const char *format, ...)
{
  if (sctk_dbg_initialized == 1)
    {
      va_list ap;
      char tmp[4096];
      va_start (ap, format);
      vsprintf (tmp, format, ap);
      va_end (ap);
      sctk_noalloc_fwrite (tmp, 1, strlen (tmp), stdout);
      fflush (stdout);
      fflush (stderr);
    }
}

void
sctk_dbg_init ()
{
  pthread_mutex_lock (&sctk_dbg_lock);
  if (sctk_dbg_initialized == 0)
    {
      sctk_thread_enable_debug ();
      sctk_message_passing_enable_debug ();
      sctk_dbg_initialized = 1;
    }
  pthread_mutex_unlock (&sctk_dbg_lock);

  sctk_thread_list ();
}

void
sctk_dbg_finalize ()
{
  not_implemented ();
}
