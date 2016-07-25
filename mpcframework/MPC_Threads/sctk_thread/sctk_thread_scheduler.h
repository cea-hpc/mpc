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
#ifndef __SCTK_THREAD_SCHEDULER_H_
#define __SCTK_THREAD_SCHEDULER_H_

#include <stdlib.h>
#include <string.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_spinlock.h"
#include "sctk_internal_thread.h"
#include "sctk_context.h"
#include <utlist.h>
#include <semaphore.h>

#include "sctk_runtime_config.h"

/***************************************/
/* THREAD SCHEDULING                   */
/***************************************/

//Thread status
typedef enum{
  sctk_thread_generic_blocked,
  sctk_thread_generic_running,
  sctk_thread_generic_zombie,
  sctk_thread_generic_joined
}sctk_thread_generic_thread_status_t;

struct sctk_thread_generic_scheduler_s;
struct sctk_per_vp_data_s;

//structure which contain data for centralized scheduler
typedef struct{
  int dummy;
}sctk_centralized_scheduler_t;

//structure which contain data for multiple queues scheduler
typedef struct{
  int dummy;
}sctk_multiple_queues_scheduler_t;

//structure which contain data for both scheduler
typedef struct sctk_thread_generic_scheduler_generic_s{
  int vp_type;
  volatile int is_idle_mode;
  sctk_spinlock_t lock;
  struct sctk_thread_generic_scheduler_s* sched;
  struct sctk_thread_generic_scheduler_generic_s *prev, *next;
  sem_t sem;
  struct sctk_per_vp_data_s* vp;
  union{
    sctk_centralized_scheduler_t centralized;
    sctk_multiple_queues_scheduler_t multiple_queues;
  };
} sctk_thread_generic_scheduler_generic_t;

//scheduler
typedef struct sctk_thread_generic_scheduler_s{
  sctk_mctx_t ctx;
  sctk_mctx_t ctx_bootstrap;
  volatile sctk_thread_generic_thread_status_t status;
  sctk_spinlock_t debug_lock;
  struct sctk_thread_generic_p_s* th;
  union{
    sctk_thread_generic_scheduler_generic_t generic;
  };
} sctk_thread_generic_scheduler_t;


//NBC_hook functions
void (*sctk_multiple_queues_sched_NBC_Pthread_sched_increase_priority)();
void (*sctk_multiple_queues_sched_NBC_Pthread_sched_decrease_priority)();
void (*sctk_multiple_queues_sched_NBC_Pthread_sched_init)();

//polling mpc
void (*sctk_multiple_queues_task_polling_thread_sched_decrease_priority)(int core);
void (*sctk_multiple_queues_task_polling_thread_sched_increase_priority)(int bind_to);

extern void (*sctk_thread_generic_sched_yield)(sctk_thread_generic_scheduler_t*);
extern void (*sctk_thread_generic_thread_status)(sctk_thread_generic_scheduler_t*,
						sctk_thread_generic_thread_status_t);
extern void (*sctk_thread_generic_register_spinlock_unlock)(sctk_thread_generic_scheduler_t*,
							   sctk_spinlock_t*);
extern void (*sctk_thread_generic_wake)(sctk_thread_generic_scheduler_t*);

struct sctk_thread_generic_p_s;
extern void (*sctk_thread_generic_sched_create)(struct sctk_thread_generic_p_s*);

void sctk_thread_generic_scheduler_init(char* thread_type,char* scheduler_type, int vp_number); 
void sctk_thread_generic_polling_init(int vp_number);
void sctk_thread_generic_scheduler_init_thread(sctk_thread_generic_scheduler_t* sched,
					       struct sctk_thread_generic_p_s* th); 
char* sctk_thread_generic_scheduler_get_name();

extern inline void sctk_generic_swap_to_sched( sctk_thread_generic_scheduler_t* sched );
extern inline void sctk_thread_generic_wake_on_task_lock( sctk_thread_generic_scheduler_t* sched, int remove_from_task_list );
/***************************************/
/* TASK SCHEDULING                     */
/***************************************/
typedef struct sctk_thread_generic_task_s{
  volatile int *data;
  void (*func) (void *);
  void *arg;
  sctk_thread_generic_scheduler_t* sched;
  struct sctk_thread_generic_task_s *prev, *next;
  int value;
  int is_blocking;
  void (*free_func) (void*);
}sctk_thread_generic_task_t;

extern void (*sctk_thread_generic_add_task)(sctk_thread_generic_task_t*);
#endif
