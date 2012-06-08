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

#ifdef MPC_USE_INFINIBAND
#ifndef __SCTK__IB_PROF_H_
#define __SCTK__IB_PROF_H_

#include <sctk_spinlock.h>
#include <sctk_debug.h>
#include <sctk_config.h>
#include <stdint.h>
#include "opa_primitives.h"

#define SCTK_IB_PROF

enum sctk_ib_prof_counters_e {
  cp_matched = 0,
  cp_not_matched = 1,
  poll_found = 2,
  poll_not_found = 3,
  alloc_mem = 4,
  free_mem = 5,
  qp_created = 6,
  eager_nb = 7,
  buffered_nb = 8,
  rdma_nb = 9,
  ibuf_sr_nb = 10,
  ibuf_rdma_nb = 11,
  ibuf_rdma_hits_nb = 12,
  ibuf_rdma_miss_nb = 13,
};

extern __thread double time_steals;
extern __thread double time_own;
extern __thread double time_poll_cq;
extern __thread double time_ptp;
extern __thread double time_coll;
extern __thread long poll_steals;
extern __thread long poll_steals_failed;
extern __thread long poll_steals_success;
extern __thread long poll_steal_same_node;
extern __thread long poll_steal_other_node;
extern __thread long poll_own;
extern __thread long poll_own_failed;
extern __thread long poll_own_success;
extern __thread long call_to_polling;
extern __thread long poll_cq;

extern __thread double time_send;
extern __thread double poll_send;
extern __thread double poll_recv;
extern __thread double tst;

#ifdef SCTK_IB_PROF
typedef struct sctk_ib_prof_s {
  OPA_int_t counters[128];
} sctk_ib_prof_t;

#define PROF_INC(r,x) do {                              \
  sctk_ib_rail_info_t *rail_ib = &(r)->network.ib;      \
  OPA_incr_int(&rail_ib->profiler->counters[x]);         \
} while(0)

#define PROF_INC_RAIL_IB(r,x) do {                      \
  OPA_incr_int(&r->profiler->counters[x]);        \
} while(0)


#define PROF_LOAD(r,x) OPA_load_int(&r->profiler->counters[x])

#else

#define PROF_INC(x,y) (void)(0)
#define PROF_INC_RAIL_IB(x,y) (void)(0)
#define PROF_LOAD(x,y) 0
#define sctk_ib_prof_init(x) (void)(0)
#define sctk_ib_prof_print(x) (void)(0)
#define sctk_ib_prof_finalize(x) (void)(0)

#endif

#if 0
void sctk_ib_prof_init(sctk_ib_rail_info_t *rail_ib);
void sctk_ib_prof_print(sctk_ib_rail_info_t *rail_ib);
void sctk_ib_prof_finalize(sctk_ib_rail_info_t *rail_ib);
void sctk_ib_prof_qp_init();

/* QP profiling */
void sctk_ib_prof_qp_init_task(int task_id, int vp);
void sctk_ib_prof_qp_flush();
void sctk_ib_prof_qp_finalize_task(int task_id);
void sctk_ib_prof_qp_write(int proc, size_t size, double ts, char from);

/* MEM profiling */
void sctk_ib_prof_mem_flush();
void sctk_ib_prof_mem_write(double ts, double mem);
void sctk_ib_prof_mem_init(sctk_ib_rail_info_t *rail_ib);
void sctk_ib_prof_mem_finalize(sctk_ib_rail_info_t *rail_ib);


#define PROF_QP_SEND 0
#define PROF_QP_RECV 1
#define PROF_QP_SYNC 2
#define PROF_QP_CREAT 3

#else
/* QP profiling */
#define sctk_ib_prof_qp_init(x) (void)(0)
#define sctk_ib_prof_qp_init_task(x,y) (void)(0)
#define sctk_ib_prof_qp_flush() (void)(0)
#define sctk_ib_prof_qp_finalize_task(x) (void)(0)
#define sctk_ib_prof_qp_write(a,b,c,d) (void)(0)

/* MEM profiling */
#define sctk_ib_prof_mem_flush(x) (void)(0)
#define sctk_ib_prof_mem_write(x,y) (void)(0)
#define sctk_ib_prof_mem_init(x) (void)(0)
#define sctk_ib_prof_mem_finalize(x) (void)(0)


#endif

#endif
#endif
