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
#include "mpcthread_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "mpc_common_spinlock.h"
#include "sctk_internal_thread.h"
#include "sctk_context.h"
#include <utlist.h>
#include <semaphore.h>

#include "mpc_runtime_config.h"

/***************************************/
/* THREAD SCHEDULING                   */
/***************************************/

// Thread status
typedef enum
{
	_mpc_threads_ng_blocked,
	_mpc_threads_ng_running,
	_mpc_threads_ng_zombie,
	_mpc_threads_ng_joined
}_mpc_threads_ng_thread_status_t;

struct _mpc_threads_ng_scheduler_s;
struct sctk_per_vp_data_s;

// structure which contain data for centralized scheduler
typedef struct
{
	int dummy;
}sctk_centralized_scheduler_t;

// structure which contain data for multiple queues scheduler
typedef struct
{
	int dummy;
}sctk_multiple_queues_scheduler_t;

// structure which contain data for both scheduler
typedef struct _mpc_threads_ng_scheduler_generic_s
{
	int                                             vp_type;
	volatile int                                    is_idle_mode;
	mpc_common_spinlock_t                           lock;
	struct _mpc_threads_ng_scheduler_s *        sched;
	struct _mpc_threads_ng_scheduler_generic_s *prev, *next;
	sem_t                                           sem;
	struct sctk_per_vp_data_s *                     vp;
	union
	{
		sctk_centralized_scheduler_t     centralized;
		sctk_multiple_queues_scheduler_t multiple_queues;
	};
} _mpc_threads_ng_scheduler_generic_t;

// scheduler
typedef struct _mpc_threads_ng_scheduler_s
{
	sctk_mctx_t                                  ctx;
	sctk_mctx_t                                  ctx_bootstrap;
	volatile _mpc_threads_ng_thread_status_t status;
	mpc_common_spinlock_t                        debug_lock;
	struct _mpc_threads_ng_p_s *             th;
	union
	{
		_mpc_threads_ng_scheduler_generic_t generic;
	};
} _mpc_threads_ng_scheduler_t;

// NBC_hook functions
void (*sctk_multiple_queues_sched_NBC_Pthread_sched_increase_priority)();
void (*sctk_multiple_queues_sched_NBC_Pthread_sched_decrease_priority)();
void (*sctk_multiple_queues_sched_NBC_Pthread_sched_init)();

// polling mpc
void (*sctk_multiple_queues_task_polling_thread_sched_decrease_priority)(
        int core);
void (*sctk_multiple_queues_task_polling_thread_sched_increase_priority)(
        int bind_to);

extern void (*_mpc_threads_ng_sched_yield)(_mpc_threads_ng_scheduler_t *);
extern void (*_mpc_threads_ng_thread_status)(_mpc_threads_ng_scheduler_t *,
                                                 _mpc_threads_ng_thread_status_t);
extern void (*_mpc_threads_ng_register_spinlock_unlock)(_mpc_threads_ng_scheduler_t *,
                                                            mpc_common_spinlock_t *);
extern void (*_mpc_threads_ng_wake)(_mpc_threads_ng_scheduler_t *);

struct _mpc_threads_ng_p_s;
extern void (*_mpc_threads_ng_sched_create)(struct _mpc_threads_ng_p_s *);

void _mpc_threads_ng_scheduler_init(char *thread_type, char *scheduler_type, int vp_number);
void _mpc_threads_ng_polling_init(int vp_number);
void _mpc_threads_ng_scheduler_init_thread(_mpc_threads_ng_scheduler_t *sched,
                                               struct _mpc_threads_ng_p_s *th);
char *_mpc_threads_ng_scheduler_get_name();

extern void sctk_generic_swap_to_sched(_mpc_threads_ng_scheduler_t *sched);
extern void
_mpc_threads_ng_wake_on_task_lock(_mpc_threads_ng_scheduler_t *sched,
                                      int remove_from_task_list);

/***************************************/
/* TASK SCHEDULING                     */
/***************************************/
typedef struct _mpc_threads_ng_task_s
{
	volatile int *                     data;
	void (*func)(void *);
	void *                             arg;
	_mpc_threads_ng_scheduler_t *  sched;
	struct _mpc_threads_ng_task_s *prev, *next;
	int                                value;
	int                                is_blocking;
	void (*free_func)(void *);
}_mpc_threads_ng_task_t;

extern void (*_mpc_threads_ng_add_task)(_mpc_threads_ng_task_t *);
#endif
