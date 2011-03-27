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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sctk_config.h>
#include <sctk_spinlock.h>

#ifndef __SCTK__LIST__
#define __SCTK__LIST__

struct sctk_list_elem {
  void* elem;
  struct sctk_list_elem *p_prev;
  struct sctk_list_elem *p_next;
};

struct sctk_list {
  uint64_t elem_count;
  uint8_t is_collector;
  size_t size_payload;
  sctk_spinlock_t   lock;
  struct sctk_list_elem *head;
  struct sctk_list_elem *tail;
};

void
sctk_list_new(struct sctk_list* list, uint8_t is_collector, size_t size_payload);

struct sctk_list_elem*
sctk_list_push(struct sctk_list* list, void *elem);

struct sctk_list_elem*
sctk_list_get_from_head(struct sctk_list* list, uint32_t n);

  void*
sctk_list_remove(struct sctk_list* list, struct sctk_list_elem* elem);

int sctk_list_is_empty(struct sctk_list* list);

void* sctk_list_walk_on_cond(struct sctk_list* list, int cond,
    void* (*funct) (void* elem, int cond), int remove);

void* sctk_list_walk(struct sctk_list* list,
    void* (*funct) (void* elem), int remove);

void sctk_list_lock(struct sctk_list* list);

void sctk_list_unlock(struct sctk_list* list);

  void*
sctk_list_pop(struct sctk_list* list);

void* sctk_list_search_and_free(struct sctk_list* list,
    void* elem);
#endif				/*  */
