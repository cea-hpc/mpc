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

typedef struct{
  int scope;
  int detachstate; 
  int schedpolicy; 
  int inheritsched;
  char* stack;
  size_t stack_size;
  void *(*start_routine) (void *);
  void *arg;
  void* return_value;
  int bind_to;
  int polling;
}sctk_thread_generic_intern_attr_t;
#define sctk_thread_generic_intern_attr_init {SCTK_THREAD_SCOPE_PROCESS,SCTK_THREAD_CREATE_JOINABLE,0,SCTK_THREAD_EXPLICIT_SCHED,NULL,0,NULL,NULL,-1,0}

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
int
sctk_thread_generic_attr_init (sctk_thread_generic_attr_t * attr);
#endif
