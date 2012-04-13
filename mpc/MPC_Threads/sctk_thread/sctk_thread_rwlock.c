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

#include "sctk_thread_rwlock.h"

void sctk_thread_generic_rwlocks_init(){ 
  sctk_thread_generic_check_size (sctk_thread_generic_rwlock_t, sctk_thread_rwlock_t);
  sctk_thread_generic_check_size (sctk_thread_generic_rwlockattr_t, sctk_thread_rwlockattr_t);
  {
	static sctk_thread_generic_rwlock_t loc = SCTK_THREAD_GENERIC_RWLOCK_INIT;
	static sctk_thread_rwlock_t glob = SCTK_THREAD_RWLOCK_INITIALIZER;
	assume (memcmp (&loc, &glob, sizeof (sctk_thread_generic_rwlock_t)) == 0);
  }
}
