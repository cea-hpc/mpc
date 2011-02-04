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
/* #   - DIDELOT Sylvain didelot.sylvain@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdio.h>
#include <sctk.h>
#include "sctk_list.h"

/* TODO: Collector mode not implemented */
void
sctk_list_new(struct sctk_list* list, uint8_t is_collector)
{
  list->elem_count = 0;
  list->lock = SCTK_SPINLOCK_INITIALIZER;
  list->is_collector = is_collector;
  list->head = NULL;
  list->tail = NULL;
}


struct sctk_list_elem*
sctk_list_alloc_elem(void* elem)
{
  struct sctk_list_elem *tmp = malloc(sizeof(struct sctk_list_elem));
  tmp->elem = elem;
  return tmp;
}

struct sctk_list_elem*
sctk_list_get_from_head(struct sctk_list* list, uint32_t n)
{
  struct sctk_list_elem *tmp = list->head;

  while (tmp && n--) {
    tmp = tmp->p_next;
  }

  if (tmp != NULL)
    return tmp->elem;
  else return NULL;
}

  void*
sctk_list_remove(struct sctk_list* list, struct sctk_list_elem* elem)
{
  void* payload;

  if (!list || !elem)
    return NULL;
  /* 1st elem */
  if (list->head == list->tail)
  {
    list->head = list->tail = NULL;
  }
  /* first elem */
  else if (!elem->p_prev)
  {
    elem->p_next->p_prev = NULL;
    list->head = elem->p_next;
  /* last elem */
  } else if(!elem->p_next) {
    elem->p_prev->p_next = NULL;
    list->tail = elem->p_prev;
  /* somewhere else */
  } else {
    elem->p_prev->p_next = elem->p_next;
    elem->p_next->p_prev = elem->p_prev;
  }

  payload = elem->elem;
  sctk_free(elem);
  return payload;
}

  void*
sctk_list_pop(struct sctk_list* list)
{
  return sctk_list_remove(list, list->head);
}

  struct sctk_list_elem*
sctk_list_push(struct sctk_list* list, void *elem)
{
  struct sctk_list_elem *new_elem = NULL;

  if (list->tail == NULL)
  {
    new_elem = sctk_list_alloc_elem(elem);
    new_elem->p_prev = NULL;
    new_elem->p_next = NULL;
    list->head = new_elem;
    list->tail = new_elem;
  } else {
    new_elem = sctk_list_alloc_elem(elem);
    new_elem->p_prev = list->tail;
    new_elem->p_next = NULL;
    list->tail->p_next = new_elem;
    list->tail = new_elem;
  }

  list->elem_count++;
  return new_elem;
}

void* sctk_list_walk(struct sctk_list* list,
    void* (*funct) (void* elem), int remove)
{
  struct sctk_list_elem *tmp = list->head;
  assume(funct);
  void* ret = NULL;

  while (tmp) {

    ret = funct(tmp->elem);
    if (ret)
    {
      if (remove)
        sctk_list_remove(list, tmp);
      return ret;
    }

    tmp = tmp->p_next;
  }
  return ret;
}


void* sctk_list_walk_on_cond(struct sctk_list* list, int cond,
    void* (*funct) (void* elem, int cond), int remove)
{
  struct sctk_list_elem *tmp = list->head;
  assume(funct);
  void* ret = NULL;

  while (tmp) {

    ret = funct(tmp->elem, cond);
    if (ret)
    {
      if (remove)
        sctk_list_remove(list, tmp);
      return ret;
    }

    tmp = tmp->p_next;
  }
  return ret;
}


int sctk_list_is_empty(struct sctk_list* list)
{
  if(list->head) return 0;
  else return 1;
}

void sctk_list_lock(struct sctk_list* list)
{
  sctk_spinlock_lock(&list->lock);
}

void sctk_list_unlock(struct sctk_list* list)
{
  sctk_spinlock_unlock(&list->lock);
}
