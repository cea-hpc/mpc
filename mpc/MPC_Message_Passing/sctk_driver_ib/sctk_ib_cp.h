/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__IB_CP_H_
#define __SCTK__IB_CP_H_

#include <sctk_spinlock.h>
#include <uthash.h>
#include <sctk_inter_thread_comm.h>
#include <sctk_ibufs.h>
#include "sctk_ib_polling.h"

enum cp_counters_e{
  matched = 0,
  not_matched,
};

extern __thread int task_node_number;

typedef struct sctk_ib_cp_task_s{
  UT_hash_handle hh_vp;
  UT_hash_handle hh_all;
  int ready;
  int vp;
  /* numa node */
  int node;
  /* rank is the key of HT */
  int rank;
  /* local pending ibufs for the current task  */
  sctk_ibuf_t *local_ibufs_list;
  sctk_spinlock_t local_ibufs_list_lock;
  char dummy[64];
  /* global pending ibufs for the current task  */
  sctk_ibuf_t * volatile * global_ibufs_list;
  sctk_spinlock_t *global_ibufs_list_lock;

  /* Counters */
  OPA_int_t c[64];
  /* Timers */
  sctk_spinlock_t lock_timers;
  double time_stolen;
  double time_steals;
  double time_own;
  /* Tasks linked together on NUMA */
  struct sctk_ib_cp_task_s* prev;
  struct sctk_ib_cp_task_s* next;
  char pad[128];
} sctk_ib_cp_task_t;
#define CP_PROF_INC(t,x) do {   \
  OPA_incr_int(&t->c[x]);        \
} while(0)

#define CP_PROF_ADD(t,x,y) do {   \
  OPA_add_int(&t->c[x],y);        \
} while(0)

#define CP_PROF_PRINT(t,x) ((int) OPA_load_int(&t->c[x]))
/* XXX:should be determined dynamically */
#define CYCLES_PER_SEC (2270.000*1e6)

/*-----------------------------------------------------------
 *  Structures
 *----------------------------------------------------------*/
struct sctk_rail_info_s;

void sctk_ib_cp_init(struct sctk_ib_rail_info_s* rail_ib);

void sctk_ib_cp_init_task(int rank, int vp);

void sctk_ib_cp_finalize_task(int rank);

int sctk_ib_cp_handle_message(struct sctk_rail_info_s *rail, sctk_ibuf_t *ibuf, int dest_task, int target_task);

int sctk_ib_cp_poll(const struct sctk_rail_info_s const* rail, struct sctk_ib_polling_s *poll,
    int task_id);

void sctk_ib_cp_poll_all(const struct sctk_rail_info_s const* rail, struct sctk_ib_polling_s *poll);

int sctk_ib_cp_steal(struct sctk_rail_info_s* rail, struct sctk_ib_polling_s *poll, char other_numa);

sctk_ib_cp_task_t *sctk_ib_cp_get_task(int rank);

sctk_ib_cp_task_t *sctk_ib_cp_get_polling_task();

void sctk_ib_cp_poll_global_list(const struct sctk_rail_info_s const * rail, struct sctk_ib_polling_s *poll);
/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

#endif
#endif
