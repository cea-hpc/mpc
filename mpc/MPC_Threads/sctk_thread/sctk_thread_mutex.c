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

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_mutex.h"
#include "sctk_thread_scheduler.h"
#include "sctk_spinlock.h"

void sctk_thread_generic_mutexes_init(){ 
  sctk_thread_generic_check_size (sctk_thread_generic_mutex_t, sctk_thread_mutex_t);
  sctk_thread_generic_check_size (sctk_thread_generic_mutexattr_t, sctk_thread_mutexattr_t);

  {
    static sctk_thread_generic_mutex_t loc = SCTK_THREAD_GENERIC_MUTEX_INIT;
    static sctk_thread_mutex_t glob = SCTK_THREAD_MUTEX_INITIALIZER;
    assume (memcmp (&loc, &glob, sizeof (sctk_thread_generic_mutex_t)) == 0);
  }
 
}
