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
#ifndef __SCTK_THREAD_RWLOCK_H_
#define __SCTK_THREAD_RWLOCK_H_

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_thread_scheduler.h"
#include "sctk_spinlock.h"
#include <utlist.h>
#include <time.h>
#include <uthash.h>

#define SCTK_RWLOCK_READ 1
#define SCTK_RWLOCK_WRITE 2
#define SCTK_RWLOCK_TRYREAD 3
#define SCTK_RWLOCK_TRYWRITE 4
#define SCTK_RWLOCK_ALONE 0
#define SCTK_RWLOCK_NO_WR_WAITING 0
#define SCTK_RWLOCK_WR_WAITING 1

typedef enum{
  SCTK_UNINITIALIZED, SCTK_INITIALIZED, SCTK_DESTROYED
}sctk_rwlock_status_t;

typedef struct sctk_thread_generic_rwlock_cell_s{
  sctk_thread_generic_scheduler_t* sched;
  volatile unsigned int type;
  struct sctk_thread_generic_rwlock_cell_s *prev, *next;
}sctk_thread_generic_rwlock_cell_t;

typedef struct sctk_thread_generic_rwlockattr_s{
  volatile int pshared;
}sctk_thread_generic_rwlockattr_t;

#define SCTK_THREAD_GENERIC_RWLOCKATTR_INIT {SCTK_THREAD_PROCESS_PRIVATE}

typedef struct sctk_thread_generic_rwlock_s{
  sctk_spinlock_t lock;
  volatile sctk_rwlock_status_t status;
  volatile unsigned int count;
  volatile unsigned int reads_count;
  volatile unsigned int current;
  volatile unsigned int wait;
  volatile sctk_thread_generic_scheduler_t* writer;
  volatile sctk_thread_generic_rwlock_cell_t* readers;
  volatile sctk_thread_generic_rwlock_cell_t* waiting;
}sctk_thread_generic_rwlock_t;

#define SCTK_THREAD_GENERIC_RWLOCK_INIT {SCTK_SPINLOCK_INITIALIZER,SCTK_UNINITIALIZED,0,0,SCTK_RWLOCK_ALONE,SCTK_RWLOCK_NO_WR_WAITING,NULL,NULL,NULL}

typedef struct{
  sctk_thread_generic_rwlock_t* rwlock;
  UT_hash_handle hh; 
}sctk_thread_rwlock_in_use_t;

int
sctk_thread_generic_rwlocks_rwlockattr_destroy( sctk_thread_generic_rwlockattr_t* attr );

int
sctk_thread_generic_rwlocks_rwlockattr_getpshared( const sctk_thread_generic_rwlockattr_t* attr,
					int* val );

int
sctk_thread_generic_rwlocks_rwlockattr_init( sctk_thread_generic_rwlockattr_t* attr );

int
sctk_thread_generic_rwlocks_rwlockattr_setpshared( sctk_thread_generic_rwlockattr_t* attr,
					int val );

int
sctk_thread_generic_rwlocks_rwlock_destroy( sctk_thread_generic_rwlock_t* lock );

int
sctk_thread_generic_rwlocks_rwlock_init( sctk_thread_generic_rwlock_t* lock,
					const sctk_thread_generic_rwlockattr_t* attr,
					sctk_thread_generic_scheduler_t* sched );

int
sctk_thread_generic_rwlocks_rwlock_rdlock( sctk_thread_generic_rwlock_t* lock,
					sctk_thread_generic_scheduler_t* sched );

int
sctk_thread_generic_rwlocks_rwlock_wrlock( sctk_thread_generic_rwlock_t* lock,
					sctk_thread_generic_scheduler_t* sched );

int
sctk_thread_generic_rwlocks_rwlock_timedrdlock( sctk_thread_generic_rwlock_t* lock,
					const struct timespec* restrict time,
					sctk_thread_generic_scheduler_t* sched );

int
sctk_thread_generic_rwlocks_rwlock_timedwrlock( sctk_thread_generic_rwlock_t* lock,
					const struct timespec* restrict time,
					sctk_thread_generic_scheduler_t* sched );

int
sctk_thread_generic_rwlocks_rwlock_tryrdlock( sctk_thread_generic_rwlock_t* lock,
					sctk_thread_generic_scheduler_t* sched );

int
sctk_thread_generic_rwlocks_rwlock_trywrlock( sctk_thread_generic_rwlock_t* lock,
					sctk_thread_generic_scheduler_t* sched );

int
sctk_thread_generic_rwlocks_rwlock_unlock( sctk_thread_generic_rwlock_t* lock,
					sctk_thread_generic_scheduler_t* sched );

int
sctk_thread_generic_rwlocks_rwlockattr_getkind_np( sctk_thread_generic_rwlockattr_t * attr,
					int* pref);

int
sctk_thread_generic_rwlocks_rwlockattr_setkind_np( sctk_thread_generic_rwlockattr_t * attr,
					int pref);

void
sctk_thread_generic_rwlocks_init();

#endif
