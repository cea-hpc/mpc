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
#ifndef __SCTK_THREAD_GENERIC_H_
#define __SCTK_THREAD_GENERIC_H_

#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include <sctk_thread_keys.h>
#include <sctk_thread_mutex.h>
#include <sctk_thread_scheduler.h>
#include <sctk_thread_cond.h>
#include <sctk_thread_rwlock.h>
#include <sctk_thread_sem.h>
#include <sctk_thread_barrier.h>
#include <sctk_thread_spinlock.h>

typedef struct sctk_thread_generic_intern_attr_s{
  int scope;
  int detachstate; 
  int schedpolicy; 
  int inheritsched;
  int nb_wait_for_join;
  volatile int cancel_state;
  volatile int cancel_type;
  volatile int cancel_status;
  char* user_stack;
  char* stack;
  size_t stack_size;
  size_t stack_guardsize;
  void *(*start_routine) (void *);
  void *arg;
  void* return_value;
  int bind_to;
  int polling;                                             /* ----------------------------------------- */
  void* sctk_thread_generic_pthread_blocking_lock_table;   /* |BARRIER|COND|MUTEX|RWLOCK|SEM|TASK LOCK| */ 
  sctk_thread_rwlock_in_use_t* rwlocks_owned;              /* ----------------------------------------- */
  sctk_spinlock_t spinlock;
  volatile int nb_sig_pending;
  volatile int nb_sig_treated;
  volatile sigset_t old_thread_sigset;
  volatile sigset_t thread_sigset;
  volatile sigset_t sa_sigset_mask;
  volatile int thread_sigpending[SCTK_NSIG];
}sctk_thread_generic_intern_attr_t;
#define sctk_thread_generic_intern_attr_init {SCTK_THREAD_SCOPE_PROCESS,SCTK_THREAD_CREATE_JOINABLE,0,SCTK_THREAD_EXPLICIT_SCHED,0,PTHREAD_CANCEL_ENABLE,PTHREAD_CANCEL_DEFERRED,0,NULL,NULL,0,0,NULL,NULL,NULL,-1,0,NULL,NULL,SCTK_SPINLOCK_INITIALIZER,0,0}

typedef struct{
  sctk_thread_generic_intern_attr_t* ptr;
} sctk_thread_generic_attr_t;

typedef struct sctk_thread_generic_p_s{
  sctk_thread_generic_scheduler_t sched;
  sctk_thread_generic_keys_t keys;
  sctk_thread_generic_intern_attr_t attr;
} sctk_thread_generic_p_t;

typedef sctk_thread_generic_p_t* sctk_thread_generic_t;
void sctk_thread_generic_set_self(sctk_thread_generic_t th);
sctk_thread_generic_t sctk_thread_generic_self();
int
sctk_thread_generic_user_create (sctk_thread_generic_t * threadp,
				 sctk_thread_generic_attr_t * attr,
				 void *(*start_routine) (void *), void *arg);
extern int
sctk_thread_generic_attr_init (sctk_thread_generic_attr_t * attr);

extern void
sctk_thread_generic_check_signals( int select );
inline void
sctk_thread_generic_handle_zombies( sctk_thread_generic_scheduler_generic_t* th );
inline void
sctk_thread_generic_alloc_pthread_blocking_lock_table( const sctk_thread_generic_attr_t* attr);
int sctk_thread_generic_attr_destroy (sctk_thread_generic_attr_t * attr);

typedef enum{
  sctk_thread_generic_barrier,
  sctk_thread_generic_cond,
  sctk_thread_generic_mutex,
  sctk_thread_generic_rwlock,
  sctk_thread_generic_sem,
  sctk_thread_generic_task_lock
}sctk_thread_generic_lock_list_t;

#endif
