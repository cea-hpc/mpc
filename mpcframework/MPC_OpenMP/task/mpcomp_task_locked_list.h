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
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include "mpcomp_types_def.h"

#if (!defined(__MPCOMP_TASK_LOCKED_LIST_H__) && defined(MPCOMP_TASK))
#define __MPCOMP_TASK_LOCKED_LIST_H__

#include "mpcomp.h"

#include "mpc_common_asm.h"
#include "mpc_common_asm.h"
#include "sctk_context.h"
#include "sctk_tls.h"

#include "mpcomp_task.h"
#include "mpcomp_alloc.h"
#include "sctk_debug.h"

#ifdef MPCOMP_USE_MCS_LOCK
#include "mpc_common_spinlock.h"
#else /* MPCOMP_USE_MCS_LOCK */
#include "mpc_common_spinlock.h"
#endif /* MPCOMP_USE_MCS_LOCK */

#define MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING 128

#ifdef MPCOMP_USE_MCS_LOCK
typedef sctk_mcslock_t sctk_mpcomp_task_lock_t;
#else /* MPCOMP_USE_MCS_LOCK */
typedef mpc_common_spinlock_t sctk_mpcomp_task_lock_t; 
#endif /* MPCOMP_USE_MCS_LOCK */

/** OpenMP task list data structure */
typedef struct mpcomp_task_list_s {
  OPA_ptr_t lockfree_head; 
  OPA_ptr_t lockfree_tail; 
  char pad1[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING-2*sizeof(OPA_ptr_t)];	
  OPA_ptr_t lockfree_shadow_head;
  char pad2[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING-1*sizeof(OPA_ptr_t)];
  OPA_int_t nb_elements; /**< Number of tasks in the list */
  sctk_mpcomp_task_lock_t mpcomp_task_lock; /**< Lock of the list                                 */
  int total;
  struct mpcomp_task_s *head; /**< First task of the list */
  struct mpcomp_task_s *tail; /**< Last task of the list */
  OPA_int_t nb_larcenies; /**< Number of tasks in the list */
} mpcomp_task_list_t;

static inline void mpcomp_task_list_reset(mpcomp_task_list_t *list) {
	memset( list, 0, sizeof( mpcomp_task_list_t ));
}


static inline int mpcomp_task_is_child(mpcomp_task_t *task, mpcomp_task_t *current_task)
{
  mpcomp_task_t *parent_task = task->parent;

  while(parent_task->depth > current_task->depth + MPCOMP_TASKS_DEPTH_JUMP)
  {
    parent_task = parent_task->far_ancestor;
  }
  while(parent_task->depth > current_task->depth)
  {
    parent_task = parent_task->parent;
  }
  if(parent_task != current_task) 
    return 0;
  else 
    return 1;
  
}

static inline int mpcomp_task_locked_list_isempty(mpcomp_task_list_t *list) {
  return (list->head == NULL);
}

static inline void mpcomp_task_locked_list_pushtohead(mpcomp_task_list_t *list,
                                               mpcomp_task_t *task) {
  if (mpcomp_task_locked_list_isempty(list)) {
    task->prev = NULL;
    task->next = NULL;
    list->head = task;
    list->tail = task;
  } else {
    task->prev = NULL;
    task->next = list->head;
    list->head->prev = task;
    list->head = task;
  }

  list->total += 1;
  task->list = list;
}

static inline void mpcomp_task_locked_list_pushtotail(mpcomp_task_list_t *list,
                                               mpcomp_task_t *task) {
  if (mpcomp_task_locked_list_isempty(list)) {
    task->prev = NULL;
    task->next = NULL;
    list->head = task;
    list->tail = task;
  } else {
    task->prev = list->tail;
    task->next = NULL;
    list->tail->next = task;
    list->tail = task;
  }

  list->total += 1;
  task->list = list;
}

static inline mpcomp_task_t *
mpcomp_task_locked_list_popfromhead(mpcomp_task_list_t *list) {
  mpcomp_task_t *task = NULL;

  /* No task in list */
  if (mpcomp_task_locked_list_isempty(list))
    return NULL;

  task = list->head;
  mpcomp_thread_t *thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
  mpcomp_task_t *current_task = thread->task_infos.current_task;
  /* Only child tasks can be scheduled to respect OpenMP norm */
  if ( current_task->func && !mpcomp_task_is_child(task, current_task) && thread->task_infos.one_list_per_thread) /* If not implicit task and one list per thread*/ 
    return NULL;

  /* Get First task in list */
  list->head = task->next;
  if(list->head) list->head->prev = NULL;
  if (!(task->next))
    list->tail = NULL;
  OPA_decr_int(&list->nb_elements);

  task->list=NULL;
  return task;
}

static inline mpcomp_task_t *
mpcomp_task_locked_list_popfromtail(mpcomp_task_list_t *list) {
  if (!mpcomp_task_locked_list_isempty(list)) {
    mpcomp_task_t *task = list->tail;

    mpcomp_thread_t *thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
    mpcomp_task_t *current_task = thread->task_infos.current_task;
    /* Only child tasks can be scheduled to respect OpenMP norm */
    if ( current_task->func && !mpcomp_task_is_child(task, current_task) && thread->task_infos.one_list_per_thread) /* If not implicit task and one list per thread*/
      return NULL;

    list->tail = task->prev;
    if(list->tail) list->tail->next = NULL;
    if (!(task->prev)) {
      list->head = NULL;
    }

    OPA_decr_int(&(list->nb_elements));
    return task;
  }

  return NULL;
}

static inline int mpcomp_task_locked_list_remove(mpcomp_task_list_t *list,
                                          mpcomp_task_t *task) {

  if (task->list == NULL || mpcomp_task_locked_list_isempty(list))
    return -1;

  if (task == list->head)
    list->head = task->next;

  if (task == list->tail)
    list->tail = task->prev;

  if (task->next)
    task->next->prev = task->prev;

  if (task->prev)
    task->prev->next = task->next;

  OPA_decr_int(&(list->nb_elements));
  task->list = NULL;

  return 1;
}

static inline mpcomp_task_t *
mpcomp_task_locked_list_search_task_in_list(mpcomp_task_list_t *list, int depth) {
  mpcomp_task_t *cur_task = list->head;

  while (cur_task) {
    if (cur_task->depth > depth)
      break;
    cur_task = cur_task->next;
  }

  if (cur_task != NULL) {
    mpcomp_task_locked_list_remove(list, cur_task);
  }

  return cur_task;
}

static inline void mpcomp_task_locked_list_consummer_lock( mpcomp_task_list_t *list, void* opaque ) 
{
#ifdef MPCOMP_USE_MCS_LOCK
    sctk_mcslock_push_ticket(&( list->mpcomp_task_lock ), (sctk_mcslock_ticket_t *) opaque, SCTK_MCSLOCK_WAIT );
#else /* MPCOMP_USE_MCS_LOCK */
  	mpc_common_spinlock_lock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_locked_list_consummer_unlock( mpcomp_task_list_t *list, void* opaque ) {
#ifdef MPCOMP_USE_MCS_LOCK
    sctk_mcslock_trash_ticket( &(list->mpcomp_task_lock), (sctk_mcslock_ticket_t *) opaque );
#else /* MPCOMP_USE_MCS_LOCK */
  	mpc_common_spinlock_unlock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int mpcomp_task_locked_list_consummer_trylock(__UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void* opaque ) {
#ifdef MPCOMP_USE_MCS_LOCK
    not_implemented();
    return 0;
#else /* MPCOMP_USE_MCS_LOCK */
  return mpc_common_spinlock_trylock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_locked_list_producer_lock(mpcomp_task_list_t *list, void* opaque) {
#ifdef MPCOMP_USE_MCS_LOCK
    sctk_mcslock_push_ticket(&( list->mpcomp_task_lock ), (sctk_mcslock_ticket_t *) opaque, SCTK_MCSLOCK_WAIT );
#else /* MPCOMP_USE_MCS_LOCK */
  	mpc_common_spinlock_lock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_locked_list_producer_unlock(mpcomp_task_list_t *list, void* opaque) {
#ifdef MPCOMP_USE_MCS_LOCK
    sctk_mcslock_trash_ticket( &(list->mpcomp_task_lock), (sctk_mcslock_ticket_t *) opaque );
#else /* MPCOMP_USE_MCS_LOCK */
  	mpc_common_spinlock_unlock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int mpcomp_task_locked_list_producer_trylock(__UNUSED__ mpcomp_task_list_t *list, __UNUSED__ void* opaque) {
#ifdef MPCOMP_USE_MCS_LOCK
    not_implemented();
    return 0;
#else /* MPCOMP_USE_MCS_LOCK */
  return mpc_common_spinlock_trylock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

#endif /* !__MPCOMP_TASK_LOCKED_LIST_H__ && MPCOMP_TASK */
