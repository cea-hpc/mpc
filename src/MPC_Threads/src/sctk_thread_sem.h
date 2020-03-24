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

#ifndef __SCTK_THREAD_SEM_H_
#define __SCTK_THREAD_SEM_H_

#include <stdlib.h>
#include <string.h>
#include "mpcthread_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_scheduler.h"
#include "mpc_common_spinlock.h"
#include "sctk_thread_mutex.h"
#include <utlist.h>
#include <time.h>

typedef struct sctk_thread_generic_sem_s
{
	volatile unsigned int             lock;
	mpc_common_spinlock_t             spinlock;
	sctk_thread_generic_mutex_cell_t *list;
}sctk_thread_generic_sem_t;

#define SCTK_THREAD_GENERIC_SEM_INIT    { 0, SCTK_SPINLOCK_INITIALIZER, NULL }

typedef struct sctk_thread_generic_sem_named_list_s
{
	char *                                       name;
	volatile int                                 nb;
	volatile int                                 unlink;
	volatile mode_t                              mode;
	sctk_thread_generic_sem_t *                  sem;
	struct sctk_thread_generic_sem_named_list_s *prev, *next;
}sctk_thread_generic_sem_named_list_t;

void sctk_thread_generic_sems_init();

int
sctk_thread_generic_sems_sem_init(sctk_thread_generic_sem_t *sem,
                                  int pshared, unsigned int value);

int
sctk_thread_generic_sems_sem_wait(sctk_thread_generic_sem_t *sem,
                                  sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_sems_sem_trywait(sctk_thread_generic_sem_t *sem,
                                     sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_sems_sem_timedwait(sctk_thread_generic_sem_t *sem,
                                       const struct timespec *time,
                                       sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_sems_sem_post(sctk_thread_generic_sem_t *sem,
                                  sctk_thread_generic_scheduler_t *sched);

int
sctk_thread_generic_sems_sem_getvalue(sctk_thread_generic_sem_t *sem,
                                      int *sval);

int
sctk_thread_generic_sems_sem_destroy(sctk_thread_generic_sem_t *sem);

sctk_thread_generic_sem_t *
sctk_thread_generic_sems_sem_open(const char *name, int oflag, ...);

int
sctk_thread_generic_sems_sem_close(sctk_thread_generic_sem_t *sem);

int
sctk_thread_generic_sems_sem_unlink(const char *name);

#endif
