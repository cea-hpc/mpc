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
#ifndef __SCTK_ETHREAD_H_
#define __SCTK_ETHREAD_H_

#include <signal.h>
#include <stdio.h>
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk_ethread.h"
#include "sctk_kernel_thread.h"
#include "sctk_internal_thread.h"
#include "sctk_context.h"
#include "sctk_spinlock.h"
#include "sctk_alloc.h"
#include "sctk_pthread_compatible_structures.h"

#ifdef MPC_Debugger
#include "sctk_thread_dbg.h"
#else
#define sctk_enable_lib_thread_db() (void)(0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  typedef void (*stck_ethread_key_destr_function_t) (void *);

  extern sctk_spinlock_t sctk_ethread_key_spinlock;
  extern int sctk_ethread_key_pos;
  extern stck_ethread_key_destr_function_t sctk_ethread_destr_func_tab[];

/*detachstate*/
#define SCTK_ETHREAD_CREATE_JOINABLE SCTK_THREAD_CREATE_JOINABLE
#define SCTK_ETHREAD_CREATE_DETACHED SCTK_THREAD_CREATE_DETACHED
/*schedpolicy*/
#define SCTK_ETHREAD_SCHED_OTHER 0
#define SCTK_ETHREAD_SCHED_RR 1
#define SCTK_ETHREAD_SCHED_FIFO 2
/*inheritsched*/
#define SCTK_ETHREAD_EXPLICIT_SCHED SCTK_THREAD_EXPLICIT_SCHED
#define SCTK_ETHREAD_INHERIT_SCHED SCTK_THREAD_INHERIT_SCHED
/*scope*/
#define SCTK_ETHREAD_SCOPE_SYSTEM SCTK_THREAD_SCOPE_SYSTEM
#define SCTK_ETHREAD_SCOPE_PROCESS SCTK_THREAD_SCOPE_PROCESS

  typedef struct sctk_ethread_attr_intern_s
  {
    int priority;
    int detached;
    int schedpolicy;
    int inheritsched;
    int scope;
    char *stack;
    int stack_size;
    unsigned int binding;
  } sctk_ethread_attr_intern_t;
#define SCTK_ETHREAD_ATTR_INIT {0,		\
      SCTK_ETHREAD_CREATE_JOINABLE,		\
      SCTK_ETHREAD_SCHED_OTHER,			\
      SCTK_ETHREAD_EXPLICIT_SCHED,		\
      SCTK_ETHREAD_SCOPE_PROCESS,               \
      NULL,SCTK_ETHREAD_THREAD_STACK_SIZE,-1}

  typedef enum
  {
    ethread_ready,
    ethread_blocked,
    ethread_zombie,
    ethread_joined,
    ethread_polling,
    ethread_inside_polling,
    ethread_block_on_vp,
    ethread_system,
    ethread_idle,
    ethread_migrate,
    ethread_dump
  } sctk_ethread_status_t;

  static inline char *sctk_ethread_debug_status (sctk_ethread_status_t status)
  {
    switch (status)
      {
      case ethread_ready:
	return "ethread_ready";
	case ethread_blocked:return "ethread_blocked";
	case ethread_zombie:return "ethread_zombie";
	case ethread_joined:return "ethread_joined";
	case ethread_polling:return "ethread_polling";
	case ethread_inside_polling:return "ethread_inside_polling";
	case ethread_block_on_vp:return "ethread_block_on_vp";
	case ethread_system:return "ethread_system";
	case ethread_idle:return "ethread_idle";
	case ethread_migrate:return "ethread_migrate";
	case ethread_dump:return "ethread_dump";
	default:not_reachable ();
      }
    return NULL;
  }

  struct sctk_ethread_virtual_processor_s;
  typedef struct sctk_ethread_per_thread_s
  {
    sctk_mctx_t ctx;
    void *tls[SCTK_THREAD_KEYS_MAX];

    volatile sctk_ethread_status_t status;
    int no_auto_enqueue;

    struct sctk_ethread_per_thread_s *next;
    void *ret_val;
    int nb_wait_for_join;

    sctk_ethread_attr_intern_t attr;

    void *(*start_routine) (void *);
    void *arg;
    char *stack;
    size_t stack_size;

    volatile struct sctk_ethread_virtual_processor_s *vp;

    sctk_spinlock_t spinlock;
    struct sctk_ethread_virtual_processor_s *migrate_to;
    void (*migration_func) (struct sctk_ethread_per_thread_s *);

    sctk_alloc_thread_data_t *tls_mem;
    int dump_for_migration;
    char *file_to_dump;
    volatile char cancel_state;
    volatile char cancel_type;
    volatile char cancel_status;

    volatile int nb_sig_pending;
    volatile char thread_sigpending[SCTK_NSIG];
    volatile sigset_t thread_sigset;

    volatile int nb_sig_threated;

    void* debug_p;
  } sctk_ethread_per_thread_t;


  extern sctk_ethread_per_thread_t sctk_ethread_main_thread;

  typedef struct sctk_ethread_polling_s
  {
    volatile int *volatile data;
    int value;
    void (*func) (void *);
    void *arg;

    struct sctk_ethread_polling_s *next;

    sctk_ethread_per_thread_t *my_self;
    volatile int forced;
  } sctk_ethread_polling_t;

  typedef struct sctk_ethread_virtual_processor_s
  {
    sctk_ethread_per_thread_t *ready_queue;
    sctk_ethread_per_thread_t *ready_queue_tail;

    sctk_ethread_per_thread_t *ready_queue_used;
    sctk_ethread_per_thread_t *ready_queue_tail_used;

    sctk_ethread_per_thread_t *zombie_queue;
    sctk_ethread_per_thread_t *zombie_queue_tail;

    volatile sctk_ethread_per_thread_t *incomming_queue;
    volatile sctk_ethread_per_thread_t *incomming_queue_tail;

    volatile sctk_ethread_per_thread_t *current;
    volatile sctk_ethread_per_thread_t *migration;
    volatile sctk_ethread_per_thread_t *dump;
    volatile sctk_ethread_per_thread_t *zombie;
    volatile int to_check;

    sctk_ethread_per_thread_t *idle;

    volatile sctk_ethread_polling_t *poll_list;

    sctk_spinlock_t spinlock;

    int rank;
    volatile sctk_long_long activity;
    volatile sctk_long_long idle_activity;
    volatile double usage;
    volatile int up;

    int bind_to;
  } sctk_ethread_virtual_processor_t;
#define SCTK_ETHREAD_VP_INIT {						\
    NULL,NULL,								\
      NULL,NULL,							\
      NULL,NULL,							\
      NULL,NULL,							\
      NULL,NULL,NULL,NULL,0,						\
      NULL,								\
      NULL,								\
      SCTK_SPINLOCK_INITIALIZER,					\
      0,0,0,0,1,-1}

  extern sctk_ethread_virtual_processor_t virtual_processor;


  sctk_ethread_t sctk_ethread_self (void);
  sctk_ethread_t sctk_ethread_mxn_self (void);
  void sctk_ethread_return_task (sctk_ethread_per_thread_t *);
  void sctk_ethread_mxn_return_task (sctk_ethread_per_thread_t *);

/*
  void sctk_ethread_mxn_place_task_on_vp (sctk_ethread_virtual_processor_t *,
					  sctk_ethread_per_thread_t *);
*/

  static inline
    void sctk_ethread_print_task (sctk_ethread_per_thread_t * task)
  {
    char *status = "not defined";
    switch (task->status)
      {
      case (ethread_ready):
	status = "ethread_ready";
	break;
      case (ethread_blocked):
	status = "ethread_blocked";
	break;
      case (ethread_zombie):
	status = "ethread_zombie";
	break;
      case (ethread_joined):
	status = "ethread_joined";
	break;
      case (ethread_polling):
	status = "ethread_polling";
	break;
      case (ethread_inside_polling):
	status = "ethread_inside_polling";
	break;
      case (ethread_block_on_vp):
	status = "ethread_block_on_vp";
	break;
      case (ethread_system):
	status = "ethread_system";
	break;
      case (ethread_idle):
	status = "ethread_idle";
	break;
      default:
	not_reachable ();
      }
    sctk_error ("task %p on %d\n\t\t\t- status %s\n\t\t\t- vp %p",
		task, task->vp->rank, status, task->vp);
  }

#ifndef NO_INTERNAL_ASSERT
  static inline void sctk_print_attr (sctk_ethread_attr_intern_t * attr)
  {
    sctk_nodebug ("Attr %p:", attr);
    if (attr != NULL)
      {
	sctk_nodebug ("\t- priority %d", attr->priority);
	sctk_nodebug ("\t- detached %d", attr->detached);
	sctk_nodebug ("\t- schedpolicy %d", attr->schedpolicy);
	sctk_nodebug ("\t- inheritsched %d", attr->inheritsched);
	sctk_nodebug ("\t- scope %d", attr->scope);
	sctk_nodebug ("\t- stack %p", attr->stack);
	sctk_nodebug ("\t- stack_size %d", attr->stack_size);
      }
  }
#else
#define sctk_print_attr(a) (void)(0)
#endif

  typedef struct sctk_ethread_freeze_on_vp_s
  {
    sctk_ethread_per_thread_t *queue;
    sctk_ethread_per_thread_t *queue_tail;
    sctk_ethread_virtual_processor_t *vp;
  } sctk_ethread_freeze_on_vp_t;

  static inline
    void __sctk_ethread_enqueue_task (sctk_ethread_per_thread_t * task,
				      sctk_ethread_per_thread_t **
				      queue,
				      sctk_ethread_per_thread_t ** queue_tail)
  {
    task->next = NULL;
    if (*queue != NULL)
      {
	(*queue_tail)->next = task;
      }
    else
      {
	*queue = task;
      }
    *queue_tail = task;
  }

  static inline
    void sctk_ethread_enqueue_task (sctk_ethread_virtual_processor_t *
				    vp, sctk_ethread_per_thread_t * task)
  {
    sctk_assert_func (if (task->status != ethread_ready)
		      {
		      char *th_status;
		      th_status =
		      sctk_ethread_debug_status (task->
						 status);
		      sctk_warning ("task %p task->status = %d (%s) ",
				    task, task->status, th_status);}
		      sctk_assert (task->status == ethread_ready););
    __sctk_ethread_enqueue_task (task,
				 &(vp->ready_queue), &(vp->ready_queue_tail));
  }

  static inline
    sctk_ethread_per_thread_t *
    __sctk_ethread_dequeue_task (sctk_ethread_per_thread_t ** queue,
				 sctk_ethread_per_thread_t ** queue_tail)
  {
    sctk_ethread_per_thread_t *task;
    if (*queue != NULL)
      {
	task = *queue;
	*queue = task->next;
	if (*queue == NULL)
	  {
	    *queue_tail = NULL;
	  }
      }
    else
      {
	return NULL;
      }
    task->next = NULL;
    return task;
  }

  static inline
    sctk_ethread_per_thread_t *
    sctk_ethread_dequeue_task (sctk_ethread_virtual_processor_t * vp)
  {
    sctk_ethread_per_thread_t *task;
    task = __sctk_ethread_dequeue_task (&(vp->ready_queue),
					&(vp->ready_queue_tail));
    return task;
  }

  static inline
    sctk_ethread_per_thread_t *
    sctk_ethread_dequeue_task_for_run (sctk_ethread_virtual_processor_t * vp)
  {
    sctk_ethread_per_thread_t *task;
    task = __sctk_ethread_dequeue_task (&(vp->ready_queue_used),
					&(vp->ready_queue_tail_used));
    return task;
  }

  static inline void
    sctk_ethread_get_incomming_tasks (sctk_ethread_virtual_processor_t * vp)
  {
    if (vp->incomming_queue != NULL)
      {
	sctk_ethread_per_thread_t *head;
	sctk_ethread_per_thread_t *tail;
	sctk_spinlock_lock (&vp->spinlock);
	if (vp->incomming_queue != NULL)
	  {
	    head = (sctk_ethread_per_thread_t *) vp->incomming_queue;
	    tail = (sctk_ethread_per_thread_t *) vp->incomming_queue_tail;
	    vp->incomming_queue = NULL;
	    vp->incomming_queue_tail = NULL;

	    if (vp->ready_queue_used != NULL)
	      {
		vp->ready_queue_tail_used->next = head;
	      }
	    else
	      {
		vp->ready_queue_used = head;
	      }
	    vp->ready_queue_tail_used = tail;

	  }
	sctk_spinlock_unlock (&vp->spinlock);
      }
  }

  static inline
    void sctk_ethread_get_old_tasks (sctk_ethread_virtual_processor_t * vp)
  {
    if (vp->ready_queue != NULL)
      {
	if (vp->ready_queue_used != NULL)
	  {
	    vp->ready_queue_tail_used->next =
	      (sctk_ethread_per_thread_t *) vp->ready_queue;
	  }
	else
	  {
	    vp->ready_queue_used =
	      (sctk_ethread_per_thread_t *) vp->ready_queue;
	  }
	vp->ready_queue_tail_used =
	  (sctk_ethread_per_thread_t *) vp->ready_queue_tail;
	vp->ready_queue = NULL;
	vp->ready_queue_tail = NULL;
      }
  }

  static sigset_t sctk_thread_default_set;
  static inline void sctk_init_default_sigset ()
  {
#ifndef WINDOWS_SYS
    sigset_t set;
    sigemptyset (&set);
    kthread_sigmask (SIG_SETMASK, &set, &sctk_thread_default_set);
    kthread_sigmask (SIG_SETMASK, &sctk_thread_default_set, NULL);
#endif
  }

  static inline void sctk_ethread_init_data (sctk_ethread_per_thread_t * data)
  {
    int i;
    sctk_ethread_attr_intern_t default_attr = SCTK_ETHREAD_ATTR_INIT;
    for (i = 0; i < SCTK_THREAD_KEYS_MAX; i++)
      {
	data->tls[i] = NULL;
      }
    for (i = 0; i < SCTK_NSIG; i++)
      {
	data->thread_sigpending[i] = 0;
#ifndef WINDOWS_SYS
	sctk_nodebug ("%d signal is %d", i + 1,
		      sigismember (&sctk_thread_default_set, i + 1));
#endif
      }
    data->nb_sig_pending = 0;
    data->next = NULL;
    data->status = ethread_ready;
    data->ret_val = NULL;
    data->nb_wait_for_join = 0;
    data->attr = default_attr;
    data->stack = NULL;
    data->cancel_state = SCTK_THREAD_CANCEL_ENABLE;
    data->cancel_type = SCTK_THREAD_CANCEL_DEFERRED;
    data->cancel_status = 0;
    data->thread_sigset = sctk_thread_default_set;
  }

#define sctk_ethread_check_size(a,b) sctk_size_checking(sizeof(a),sizeof(b),SCTK_STRING(a),SCTK_STRING(b),__FILE__,__LINE__)
#define sctk_ethread_check_size_eq(a,b) sctk_size_checking_eq(sizeof(a),sizeof(b),SCTK_STRING(a),SCTK_STRING(b),__FILE__,__LINE__)

  typedef struct sctk_ethread_mutex_cell_s
  {
    struct sctk_ethread_mutex_cell_s *next;
    sctk_ethread_per_thread_t *my_self;
    volatile int wake;
  } sctk_ethread_mutex_cell_t;


  int sctk_ethread_check_state (void);

#ifdef __cplusplus
}
#endif
#endif
