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

#ifndef __SCTK_THREAD_BARRIER_H_
#define __SCTK_THREAD_BARRIER_H_

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_scheduler.h"
#include "sctk_spinlock.h"
#include <utlist.h>

typedef struct sctk_thread_generic_barrier_cell_s{ 
  sctk_thread_generic_scheduler_t* sched;
  struct sctk_thread_generic_barrier_cell_s *prev, *next;
}sctk_thread_generic_barrier_cell_t;

typedef struct sctk_thread_generic_barrier_s{
  sctk_spinlock_t lock;
  volatile int nb_max;
  volatile int nb_current;
  sctk_thread_generic_barrier_cell_t* blocked;
}sctk_thread_generic_barrier_t;
#define SCTK_THREAD_GENERIC_BARRIER_INIT {SCTK_SPINLOCK_INITIALIZER,0,0,NULL}

typedef struct sctk_thread_generic_barrierattr_s{
  volatile int pshared;
}sctk_thread_generic_barrierattr_t;
#define SCTK_THREAD_GENERIC_BARRIERATTR_INIT {SCTK_THREAD_PROCESS_PRIVATE}

int
sctk_thread_generic_barriers_barrierattr_destroy( sctk_thread_generic_barrierattr_t* attr );

int
sctk_thread_generic_barriers_barrierattr_init( sctk_thread_generic_barrierattr_t* attr );

int
sctk_thread_generic_barriers_barrierattr_getpshared( const sctk_thread_generic_barrierattr_t* 
			restrict attr, int* restrict pshared );

int
sctk_thread_generic_barriers_barrierattr_setpshared( sctk_thread_generic_barrierattr_t* attr,
			int pshared );

int
sctk_thread_generic_barriers_barrier_destroy( sctk_thread_generic_barrier_t* barrier );

int
sctk_thread_generic_barriers_barrier_init( sctk_thread_generic_barrier_t* restrict barrier,
			const sctk_thread_generic_barrierattr_t* restrict attr, unsigned count );

int
sctk_thread_generic_barriers_barrier_wait( sctk_thread_generic_barrier_t* barrier,
			sctk_thread_generic_scheduler_t* sched );

void
sctk_thread_generic_barriers_init();

#endif

