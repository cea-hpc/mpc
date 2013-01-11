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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpcomp.h>
#include "mpcomp_internal.h"
#include <sctk_debug.h>

/*
   This file contains all synchronization functions related to OpenMP
 */

/* TODO move these locks (atomic and critical) in a per-task structure:
   - mpcomp_thread_info, (every instance sharing the same lock) 
   - Key
   - TLS if available
 */
TODO("OpenMP: Anonymous critical and global atomic are not per-task locks")
static sctk_spinlock_t global_atomic_lock = SCTK_SPINLOCK_INITIALIZER;
static sctk_thread_mutex_t global_critical_lock =
  SCTK_THREAD_MUTEX_INITIALIZER;

void
__mpcomp_atomic_begin ()
{
  sctk_spinlock_lock (&(global_atomic_lock));
}

void
__mpcomp_atomic_end ()
{
  sctk_spinlock_unlock (&(global_atomic_lock));
}


void
__mpcomp_anonymous_critical_begin ()
{
  sctk_nodebug ("__mpcomp_anonymous_critical_begin: Before lock");
  sctk_thread_mutex_lock (&global_critical_lock);
  sctk_nodebug ("__mpcomp_anonymous_critical_begin: After lock");
}

void
__mpcomp_anonymous_critical_end ()
{
  sctk_nodebug ("__mpcomp_anonymous_critical_end: Before unlock");
  sctk_thread_mutex_unlock (&global_critical_lock);
  sctk_nodebug ("__mpcomp_anonymous_critical_end: Before unlock");
}

void
__mpcomp_named_critical_begin (void **l)
{
  /* TODO check if l==NULL, then allocate memory and lock */
  TODO("OpenMP: Named critical acts as anonymous critical")
  sctk_thread_mutex_lock (&global_critical_lock);
}

void
__mpcomp_named_critical_end (void **l)
{
  /* TODO unlock *l */
  sctk_thread_mutex_unlock (&global_critical_lock);
}
