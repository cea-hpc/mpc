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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sctk_spinlock.h"

#ifndef __SCTK__IB_LIST_H_
#define __SCTK__IB_LIST_H_

#define SCTK_LIST_HEAD_INIT(n) {   \
  (n)->p_prev = n;             \
  (n)->p_next = n;             \
  (n)->lock = SCTK_SPINLOCK_INITIALIZER; \
}


struct sctk_list_header {
  struct sctk_list_header* p_prev;
  struct sctk_list_header* p_next;
  sctk_spinlock_t lock;
};

#define sctk_ib_list_is_empty(n) \
  (!(n)->p_prev ? 1 : 0)

static inline void
__add (struct sctk_list_header *new,
  struct sctk_list_header *p,
  struct sctk_list_header *n)
{
  n->p_prev = new;
  new->p_next = n;
  new->p_prev = p;
  p->p_next = new;
}

static inline void
sctk_ib_list_push_head(struct sctk_list_header* list,
    struct sctk_list_header *elem)
{
  __add(elem, list, (list)->p_next);
}

  static inline void
sctk_ib_list_push_tail(struct sctk_list_header* list,
    struct sctk_list_header *elem)
{
  __add(elem, (list)->p_prev, list);
}

  static inline void
__remove (struct sctk_list_header* prev,
    struct sctk_list_header* next)
{
  (next)->p_prev = prev;
  (prev)->p_next = next;
}

  static inline void
sctk_ib_list_remove(struct sctk_list_header *elem)
{
  __remove((elem)->p_prev, (elem)->p_next);
}

#define sctk_ib_list_get_entry(ptr, type, header) ({\
  type * __ptr  = NULL; \
  (type *) ( (void*) ptr - (unsigned long) (&__ptr->header));})


#define sctk_ib_list_foreach(ptr, list) \
  for (ptr=(list)->p_next; ptr!=list; ptr=ptr->p_next)

#define sctk_ib_list_lock(list) \
  sctk_spinlock_lock(&(list)->lock)

#define sctk_ib_list_unlock(list) \
  sctk_spinlock_unlock(&(list)->lock)

#define sctk_ib_list_trylock(list) \
  sctk_spinlock_trylock(&(list)->lock)

#endif
