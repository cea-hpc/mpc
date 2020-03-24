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
#include "mpcthread_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_scheduler.h"
#include "sctk_thread_mutex.h"
#include "mpc_common_spinlock.h"
#include <utlist.h>

typedef struct sctk_thread_generic_cond_cell_s
{
	sctk_thread_generic_scheduler_t *       sched;
	sctk_thread_generic_mutex_t *           binded;
	struct sctk_thread_generic_cond_cell_s *prev, *next;
}sctk_thread_generic_cond_cell_t;

typedef struct
{
	mpc_common_spinlock_t            lock;
	sctk_thread_generic_cond_cell_t *blocked;
	clockid_t                        clock_id;
}sctk_thread_generic_cond_t;
#define SCTK_THREAD_GENERIC_COND_INIT    { SCTK_SPINLOCK_INITIALIZER, NULL, 0 }

typedef struct sctk_thread_generic_condattr_s
{
	int attrs;
}sctk_thread_generic_condattr_t;
#define SCTK_THREAd_GENERIC_CONDATTR_INIT    { 0 }

void sctk_thread_generic_conds_init();

int
sctk_thread_generic_conds_condattr_destroy(sctk_thread_generic_condattr_t *attr);

int
sctk_thread_generic_conds_condattr_getpshared(sctk_thread_generic_condattr_t *attr,
                                              int *pshared);

int
sctk_thread_generic_conds_condattr_init(sctk_thread_generic_condattr_t *attr);

int
sctk_thread_generic_conds_condattr_setpshared(sctk_thread_generic_condattr_t *attr,
                                              int pshared);

int
sctk_thread_generic_conds_condattr_setclock(sctk_thread_generic_condattr_t *attr,
                                            clockid_t clock_id);

int
sctk_thread_generic_conds_condattr_getclock(sctk_thread_generic_condattr_t *restrict attr,
                                            clockid_t *restrict clockid_t);

int
sctk_thread_generic_conds_cond_destroy(sctk_thread_generic_cond_t *lock);

int
sctk_thread_generic_conds_cond_init(sctk_thread_generic_cond_t *cond,
                                    const sctk_thread_generic_condattr_t *attr,
                                    sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_conds_cond_wait(sctk_thread_generic_cond_t *cond,
                                    sctk_thread_generic_mutex_t *mutex,
                                    sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_conds_cond_signal(sctk_thread_generic_cond_t *cond,
                                      sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_conds_cond_timedwait(sctk_thread_generic_cond_t *cond,
                                         sctk_thread_generic_mutex_t *mutex,
                                         const struct timespec *restrict time,
                                         sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_conds_cond_broadcast(sctk_thread_generic_cond_t *cond,
                                         sctk_thread_generic_scheduler_t *sched);

#endif
