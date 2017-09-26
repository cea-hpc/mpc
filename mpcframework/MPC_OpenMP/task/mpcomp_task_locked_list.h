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
#include "sctk.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"
#include "sctk_context.h"
#include "sctk_tls.h"

#include "mpcomp_task.h"
#include "mpcomp_alloc.h"
#include "sctk_debug.h"

#ifdef MPCOMP_USE_MCS_LOCK
typedef sctk_mcslock_t sctk_mpcomp_task_lock_t;
#else /* MPCOMP_USE_MCS_LOCK */
typedef sctk_spinlock_t sctk_mpcomp_task_lock_t; 
#endif /* MPCOMP_USE_MCS_LOCK */

/** OpenMP task list data structure */
typedef struct mpcomp_task_list_s {
  sctk_atomics_int nb_elements; /**< Number of tasks in the list */
  sctk_mpcomp_task_lock_t mpcomp_task_lock; /**< Lock of the list                                 */
  struct mpcomp_task_s *head; /**< First task of the list */
  struct mpcomp_task_s *tail; /**< Last task of the list */
  int total; /**< Total number of tasks pushed in the list         */
  sctk_atomics_int nb_larcenies; /**< Number of tasks in the list */
} mpcomp_task_list_t;

static inline void mpcomp_task_list_reset(mpcomp_task_list_t *list) {
	memset( list, 0, sizeof( mpcomp_task_list_t ));
}

static inline void mpcomp_task_list_free(mpcomp_task_list_t *list) {
  mpcomp_free(list);
}

static inline int mpcomp_task_list_isempty(mpcomp_task_list_t *list) {
  // return ( sctk_atomics_load_int( &( list->nb_elements ) ) == 0 );
  return (list->head == NULL);
}

static inline void mpcomp_task_list_pushtohead(mpcomp_task_list_t *list,
                                               mpcomp_task_t *task) {
  if (mpcomp_task_list_isempty(list)) {
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

static inline void mpcomp_task_list_pushtotail(mpcomp_task_list_t *list,
                                               mpcomp_task_t *task) {
  if (mpcomp_task_list_isempty(list)) {
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
mpcomp_task_list_popfromhead(mpcomp_task_list_t *list, int depth) {
  mpcomp_task_t *task = NULL;

  /* No task in list */
  if (mpcomp_task_list_isempty(list))
    return NULL;

  task = list->head;
  sctk_assert(task);

  /* No task with depth greater than mine in list */
  if (task->depth <= depth && depth)
    return NULL;

  /* Get First task in list */
  list->head = task->next;
  if (!(task->next))
    list->tail = NULL;
  sctk_atomics_decr_int(&list->nb_elements);

  return task;
}

static inline mpcomp_task_t *
mpcomp_task_list_popfromtail(mpcomp_task_list_t *list, int depth) {
  if (!mpcomp_task_list_isempty(list)) {
    mpcomp_task_t *task = list->tail;

    list->tail = task->prev;
    if (!(task->prev)) {
      list->head = NULL;
    }

    sctk_atomics_decr_int(&(list->nb_elements));
    return task;
  }

  return NULL;
}

static inline int mpcomp_task_list_remove(mpcomp_task_list_t *list,
                                          mpcomp_task_t *task) {

  if (task->list == NULL || mpcomp_task_list_isempty(list))
    return -1;

  if (task == list->head)
    list->head = task->next;

  if (task == list->tail)
    list->tail = task->prev;

  if (task->next)
    task->next->prev = task->prev;

  if (task->prev)
    task->prev->next = task->next;

  sctk_atomics_decr_int(&(list->nb_elements));
  task->list = NULL;

  return 1;
}

static inline mpcomp_task_t *
mpcomp_task_list_search_task_in_list(mpcomp_task_list_t *list, int depth) {
  mpcomp_task_t *cur_task = list->head;

  while (cur_task) {
    if (cur_task->depth > depth)
      break;
    cur_task = cur_task->next;
  }

  if (cur_task != NULL) {
    mpcomp_task_list_remove(list, cur_task);
  }

  return cur_task;
}

static inline void mpcomp_task_list_consummer_lock( mpcomp_task_list_t *list, void* opaque ) 
{
#ifdef MPCOMP_USE_MCS_LOCK
    sctk_mcslock_push_ticket(&( list->mpcomp_task_lock ), (sctk_mcslock_ticket_t *) opaque, SCTK_MCSLOCK_WAIT );
#else /* MPCOMP_USE_MCS_LOCK */
  	sctk_spinlock_lock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_list_consummer_unlock( mpcomp_task_list_t *list, void* opaque ) {
#ifdef MPCOMP_USE_MCS_LOCK
    sctk_mcslock_trash_ticket( &(list->mpcomp_task_lock), (sctk_mcslock_ticket_t *) opaque );
#else /* MPCOMP_USE_MCS_LOCK */
  	sctk_spinlock_unlock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int mpcomp_task_list_consummer_trylock(mpcomp_task_list_t *list, void* opaque ) {
#ifdef MPCOMP_USE_MCS_LOCK
    not_implemented();
#else /* MPCOMP_USE_MCS_LOCK */
  return sctk_spinlock_trylock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_list_producer_lock(mpcomp_task_list_t *list, void* opaque) {
#ifdef MPCOMP_USE_MCS_LOCK
    sctk_mcslock_push_ticket(&( list->mpcomp_task_lock ), (sctk_mcslock_ticket_t *) opaque, SCTK_MCSLOCK_WAIT );
#else /* MPCOMP_USE_MCS_LOCK */
  	sctk_spinlock_lock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline void mpcomp_task_list_producer_unlock(mpcomp_task_list_t *list, void* opaque) {
#ifdef MPCOMP_USE_MCS_LOCK
    sctk_mcslock_trash_ticket( &(list->mpcomp_task_lock), (sctk_mcslock_ticket_t *) opaque );
#else /* MPCOMP_USE_MCS_LOCK */
  	sctk_spinlock_unlock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

static inline int mpcomp_task_list_producer_trylock(mpcomp_task_list_t *list, void* opaque) {
#ifdef MPCOMP_USE_MCS_LOCK
    not_implemented();
#else /* MPCOMP_USE_MCS_LOCK */
  return sctk_spinlock_trylock(&(list->mpcomp_task_lock));
#endif /* MPCOMP_USE_MCS_LOCK */
}

#endif /* !__MPCOMP_TASK_LOCKED_LIST_H__ && MPCOMP_TASK */
