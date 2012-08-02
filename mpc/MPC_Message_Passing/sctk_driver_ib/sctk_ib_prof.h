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

#ifndef __SCTK__IB_PROF_H_
#define __SCTK__IB_PROF_H_

#ifdef MPC_USE_INFINIBAND

#include <sctk_spinlock.h>
#include <sctk_debug.h>
#include <sctk_config.h>
#include "opa_primitives.h"
#include "sctk_stdint.h"

/* Uncomment to enable Counters */
 #define SCTK_IB_PROF
/* Uncomment to enable QP profiling */
//#define SCTK_IB_QP_PROF
/* Uncomment to enable MEM profiling */
//#define SCTK_IB_MEM_PROF

#ifdef SCTK_IB_PROF
#define PROF_DECL(type, name) type name
typedef struct sctk_ib_prof_s {
  /* Type double */
  PROF_DECL(double, time_steals);
  PROF_DECL(double, time_own);
  PROF_DECL(double, time_poll_cq);
  PROF_DECL(double, time_ptp);
  PROF_DECL(double, time_coll);
  PROF_DECL(double, time_send);
  PROF_DECL(double, poll_send);
  PROF_DECL(double, poll_recv);
  PROF_DECL(double, post_send);
  PROF_DECL(double, resize_rdma);
  PROF_DECL(double, ibuf_release);
  PROF_DECL(double, tst);

  /* Type long */
  PROF_DECL(long, poll_steals);
  PROF_DECL(long, poll_steals_failed);
  PROF_DECL(long, poll_steals_success);
  PROF_DECL(long, poll_steal_same_node);
  PROF_DECL(long, poll_steal_other_node);
  PROF_DECL(long, poll_own);
  PROF_DECL(long, poll_own_failed);
  PROF_DECL(long, poll_own_success);
  PROF_DECL(long, call_to_polling);
  PROF_DECL(long, poll_cq);

  PROF_DECL(long, cp_matched);
  PROF_DECL(long, cp_not_matched);
  PROF_DECL(long, poll_found);
  PROF_DECL(long, poll_not_found);
  PROF_DECL(long, alloc_mem);
  PROF_DECL(long, free_mem);
  PROF_DECL(long, qp_created);
  PROF_DECL(long, eager_nb);
  PROF_DECL(long, buffered_nb);
  PROF_DECL(long, rdma_nb);
  PROF_DECL(long, ibuf_sr_nb);
  PROF_DECL(long, ibuf_rdma_nb);
  PROF_DECL(long, ibuf_rdma_hits_nb);
  PROF_DECL(long, ibuf_rdma_miss_nb);
  PROF_DECL(long, rdma_connection);
  PROF_DECL(long, rdma_resizing);
  PROF_DECL(long, rdma_deconnection);
} sctk_ib_prof_t;

extern __thread struct sctk_ib_prof_s * sctk_ib_profiler;
extern __thread struct sctk_ib_prof_s * sctk_ib_profiler_start;

static void sctk_ib_prof_check_init_task() {
  if (sctk_ib_profiler == NULL) {
    sctk_ib_profiler = sctk_malloc(sizeof(sctk_ib_prof_t) * sctk_route_get_rail_nb());
    assume(sctk_ib_profiler);
    memset(sctk_ib_profiler, 0, sizeof(sctk_ib_prof_t) * sctk_route_get_rail_nb());

    sctk_ib_profiler_start = sctk_malloc(sizeof(sctk_ib_prof_t) * sctk_route_get_rail_nb());
    assume(sctk_ib_profiler_start);
    memset(sctk_ib_profiler_start, 0, sizeof(sctk_ib_prof_t) * sctk_route_get_rail_nb());
  }
}

#define PROF_TIME_START(r, x) do {                                  \
  sctk_ib_prof_check_init_task();                                   \
  sctk_ib_profiler_start[r->rail_number].x = sctk_get_time_stamp(); \
} while (0)

#define PROF_TIME_END(r, x) do {                                    \
  const double ___end = sctk_get_time_stamp();                      \
  const double ___start = sctk_ib_profiler_start[r->rail_number].x; \
  sctk_ib_prof_check_init_task();                                   \
  sctk_ib_profiler[r->rail_number].x += (___end - ___start);         \
} while(0)

#define PROF_INC(r,x) do {                              \
  sctk_ib_prof_check_init_task();                       \
  sctk_ib_profiler[r->rail_number].x ++;                \
} while(0)

#define PROF_ADD(r,x,y) do {                            \
  sctk_ib_prof_check_init_task();                       \
  sctk_ib_profiler[r->rail_number].x += y;              \
} while(0)


#define PROF_LOAD(r,x) sctk_ib_profiler[r->rail_number].x

void sctk_ib_prof_init(int nb_rails);
void sctk_ib_prof_init_task(int rank, int vp);
void sctk_ib_prof_print(sctk_ib_rail_info_t *rail_ib);
void sctk_ib_prof_finalize(sctk_ib_rail_info_t *rail_ib);
double sctk_ib_prof_get_mem_used();

#else

#define PROF_TIME_START(x,y) (void)(0)
#define PROF_TIME_END(x,y) (void)(0)
#define PROF_INC_RAIL_IB(x,y) (void)(0)
#define PROF_INC(x,y) (void)(0)
#define PROF_INC_RAIL_IB(x,y) (void)(0)
#define PROF_LOAD(x,y) 0
#define sctk_ib_prof_init(x) (void)(0)
#define sctk_ib_prof_init_task(x, y) (void)(0)
#define sctk_ib_prof_print(x) (void)(0)
#define sctk_ib_prof_finalize(x) (void)(0)
static double sctk_ib_prof_get_mem_used() {
  return 0;
}
#endif

/*
 * QP profiling
 */
#define PROF_QP_SEND 0
#define PROF_QP_RECV 1
#define PROF_QP_SYNC 2
#define PROF_QP_CREAT 3
#define PROF_QP_RDMA_CONNECTED 4
#define PROF_QP_RDMA_RESIZING 5
#define PROF_QP_RDMA_DISCONNECTED 6

#ifdef SCTK_IB_QP_PROF
/* QP profiling */
void sctk_ib_prof_qp_init();
void sctk_ib_prof_qp_init_task(int task_id, int vp);
void sctk_ib_prof_qp_flush();
void sctk_ib_prof_qp_finalize_task(int task_id);
void sctk_ib_prof_qp_write(int proc, size_t size, double ts, char from);
#else
/* QP profiling */
#define sctk_ib_prof_qp_init(x) (void)(0)
#define sctk_ib_prof_qp_init_task(x,y) (void)(0)
#define sctk_ib_prof_qp_flush() (void)(0)
#define sctk_ib_prof_qp_finalize_task(x) (void)(0)
#define sctk_ib_prof_qp_write(a,b,c,d) (void)(0)
#endif


/*
 * Memory profiling
 */
/* TODO: no more working. Should be fixed */
#ifdef SCTK_IB_MEM_PROF
void sctk_ib_prof_mem_init(sctk_ib_rail_info_t *rail_ib);
void sctk_ib_prof_mem_flush();
void sctk_ib_prof_mem_write(double ts, double mem);
void sctk_ib_prof_mem_finalize(sctk_ib_rail_info_t *rail_ib);
#else
#define sctk_ib_prof_mem_init(x) (void)(0)
#define sctk_ib_prof_mem_flush(x) (void)(0)
#define sctk_ib_prof_mem_write(x,y) (void)(0)
#define sctk_ib_prof_mem_finalize(x) (void)(0)

#endif  /* SCTK_IB_PROF */

#endif  /* MPC_USE_INFINIBAND */
#endif  /* __SCTK__IB_PROF_H_ */

/* In Bytes */
#define IBV_MEM_USED_LIMIT (33.0 * 1000.0 * 1000.0 * 1024.0)
