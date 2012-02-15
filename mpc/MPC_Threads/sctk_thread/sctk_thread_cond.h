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
#ifndef __SCTK_THREAD_COND_H_
#define __SCTK_THREAD_COND_H_

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_scheduler.h"
#include "sctk_thread_mutex.h"
#include "sctk_spinlock.h"
#include <utlist.h>

typedef int sctk_thread_generic_condattr_t;

typedef struct sctk_thread_generic_cond_cell_s{
sctk_thread_generic_scheduler_t* sched;
struct sctk_thread_generic_cond_cell_s *prev, *next;
} sctk_thread_generic_cond_cell_t;

typedef struct {
  sctk_spinlock_t lock;
  sctk_thread_generic_cond_cell_t *blocked;
}sctk_thread_generic_cond_t;
#define SCTK_THREAD_GENERIC_COND_INIT {SCTK_SPINLOCK_INITIALIZER,NULL}

void sctk_thread_generic_conds_init();
int sctk_thread_generic_conds_cond_init (sctk_thread_generic_cond_t * cond,
					 const  sctk_thread_generic_condattr_t* attr,
					 sctk_thread_generic_scheduler_t* sched);

int sctk_thread_generic_conds_cond_wait (sctk_thread_generic_cond_t * cond,
					 sctk_thread_generic_mutex_t* mutex,
					 sctk_thread_generic_scheduler_t* sched);

int sctk_thread_generic_conds_cond_signal (sctk_thread_generic_cond_t * cond,
					 sctk_thread_generic_scheduler_t* sched);

int sctk_thread_generic_conds_cond_broadcast (sctk_thread_generic_cond_t * cond,
					 sctk_thread_generic_scheduler_t* sched);


#endif
