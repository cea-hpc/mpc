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
#include "sctk_thread_scheduler.h"
#include "sctk_spinlock.h"
#include "sctk_thread_generic.h"

void (*sctk_thread_generic_sched_yield)(sctk_thread_generic_scheduler_t*) = NULL;
void (*sctk_thread_generic_thread_status)(sctk_thread_generic_scheduler_t*,
					  sctk_thread_generic_thread_status_t) = NULL;
void (*sctk_thread_generic_register_spinlock_unlock)(sctk_thread_generic_scheduler_t*,
						     sctk_spinlock_t*) = NULL;
void (*sctk_thread_generic_wake)(sctk_thread_generic_scheduler_t*) = NULL;

void (*sctk_thread_generic_scheduler_init_thread_p)(sctk_thread_generic_scheduler_t* ) = NULL;

void (*sctk_thread_generic_create)(sctk_thread_generic_p_t*) = NULL;;
/***************************************/
/* CENTRALIZED SCHEDULER               */
/***************************************/

static void sctk_centralized_sched_yield(sctk_thread_generic_scheduler_t*sched){
  not_implemented();
}

static 
void sctk_centralized_thread_status(sctk_thread_generic_scheduler_t* sched,
				    sctk_thread_generic_thread_status_t status){
  not_implemented();
}

static void sctk_centralized_register_spinlock_unlock(sctk_thread_generic_scheduler_t* sched,
						      sctk_spinlock_t* lock){
  not_implemented();
}

static void sctk_centralized_wake(sctk_thread_generic_scheduler_t* sched){
  not_implemented();
}

static sctk_centralized_create(sctk_thread_generic_p_t*thread){
  not_implemented();
}

static void sctk_centralized_scheduler_init_thread(sctk_thread_generic_scheduler_t* sched){
  sched->centralized.next = NULL;
  sched->centralized.prev = NULL;
}


/***************************************/
/* INIT                                */
/***************************************/
static char sched_type[4096];
void sctk_thread_generic_scheduler_init(char* scheduler_type){ 
  sprintf(sched_type,"%s",scheduler_type);
  
  if(strcmp("centralized",scheduler_type) ==0){
    sctk_thread_generic_sched_yield = sctk_centralized_sched_yield;
    sctk_thread_generic_thread_status = sctk_centralized_thread_status;
    sctk_thread_generic_register_spinlock_unlock = sctk_centralized_register_spinlock_unlock;
    sctk_thread_generic_wake = sctk_centralized_wake;
    sctk_thread_generic_scheduler_init_thread_p = sctk_centralized_scheduler_init_thread;
    sctk_thread_generic_create = sctk_centralized_create;
  } else {
    not_reachable();
  }
}

void sctk_thread_generic_scheduler_init_thread(sctk_thread_generic_scheduler_t* sched){
  sched->status = sctk_thread_generic_running;
  sctk_centralized_scheduler_init_thread(sched);
}

char* sctk_thread_generic_scheduler_get_name(){
  return sched_type;
}
