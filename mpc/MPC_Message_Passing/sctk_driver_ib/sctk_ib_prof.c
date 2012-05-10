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
#include <infiniband/verbs.h>
#include "sctk_ib_mmu.h"
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "sctk_asm.h"
#include "utlist.h"
#include "sctk_route.h"
#include "sctk_ib_prof.h"
#include <sys/types.h>
#include <fcntl.h>

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "PROF"
#include "sctk_ib_toolkit.h"

#ifdef SCTK_IB_PROF
int sctk_ib_counters[128];

/**
 *  Description:
 *
 *
 */

/* Counters */
__thread double time_poll_cq = 0;
__thread double time_steals = 0;
__thread double time_own = 0;
__thread double time_ptp = 0;
__thread double time_coll = 0;
__thread long poll_steals = 0;
__thread long poll_steals_failed = 0;
__thread long poll_steals_success = 0;
__thread long poll_steal_same_node = 0;
__thread long poll_steal_other_node = 0;
__thread long poll_own = 0;
__thread long poll_own_failed = 0;
__thread long poll_own_success = 0;
__thread long call_to_polling = 0;
__thread long poll_cq = 0;

__thread double time_send = 0;
__thread double poll_send = 0;
__thread double poll_recv = 0;
__thread double tst = 0;


/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_ib_prof_init(sctk_ib_rail_info_t *rail_ib) {
  rail_ib->profiler = sctk_malloc(sizeof(sctk_ib_prof_t));
  assume(rail_ib->profiler);
  memset(rail_ib->profiler, 0, sizeof(sctk_ib_prof_t));

  sctk_ib_prof_qp_init();
}

void sctk_ib_prof_print(sctk_ib_rail_info_t *rail_ib) {
  sctk_ib_prof_qp_finalize();
  fprintf(stderr, "[%d] %d %d %d %d %d %d\n", sctk_process_rank,
      PROF_LOAD(rail_ib, alloc_mem),
      PROF_LOAD(rail_ib, free_mem),
      PROF_LOAD(rail_ib, qp_created),
      PROF_LOAD(rail_ib, eager_nb),
      PROF_LOAD(rail_ib, buffered_nb),
      PROF_LOAD(rail_ib, rdma_nb));
}


/*-----------------------------------------------------------
 *  QP profiling
 *----------------------------------------------------------*/

#define QP_PROF_BUFF_SIZE 400000
#define QP_PROF_OUTPUT_FILE "output/qp_prof_%d_%d"

void sctk_ib_prof_qp_init_task(int task_id) {}
void sctk_ib_prof_qp_finalize_task(int task_id) {}

  struct sctk_ib_prof_qp_buff_s {
  int proc;
  size_t size;
  double ts;
};

struct sctk_ib_prof_qp_s {
  struct sctk_ib_prof_qp_buff_s *buff;
  int head;
  int fd;
  int proc;
  sctk_spinlock_t lock;
};

struct sctk_ib_prof_qp_s * qp_prof;

void sctk_ib_prof_qp_init() {
  char filename[256];

  qp_prof = sctk_malloc ( sizeof(struct sctk_ib_prof_qp_s) );
  qp_prof->buff = sctk_malloc (QP_PROF_BUFF_SIZE * sizeof(struct sctk_ib_prof_qp_buff_s));

  sprintf(filename, QP_PROF_OUTPUT_FILE, sctk_get_node_rank(), sctk_get_process_rank());
  qp_prof->fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  assume (qp_prof->fd != -1);

  qp_prof->proc = sctk_get_process_rank();
  qp_prof->head = 0;
  qp_prof->lock = SCTK_SPINLOCK_INITIALIZER;

  sctk_ib_prof_qp_write(-1, 0, sctk_get_time_stamp());
}

void sctk_ib_prof_qp_flush() {
  sctk_debug("Dumping file with %lu elements", qp_prof->head);
  write(qp_prof->fd, qp_prof->buff,
      qp_prof->head * sizeof(struct sctk_ib_prof_qp_buff_s));
  sctk_debug("File dumped");
  qp_prof->head = 0;
}

extern volatile int sctk_online_program;
void sctk_ib_prof_qp_write(int proc, size_t size, double ts) {
  int head;

  assume(qp_prof);
  /* We flush */
  sctk_spinlock_lock(&qp_prof->lock);
  if (qp_prof->head > (QP_PROF_BUFF_SIZE - 1)) {
    sctk_ib_prof_qp_flush();
  }
  head = qp_prof->head;
  qp_prof->head++;
  sctk_spinlock_unlock(&qp_prof->lock);

  qp_prof->buff[head].proc = proc;
  qp_prof->buff[head].size = size;
  qp_prof->buff[head].ts = ts;
}

void sctk_ib_prof_qp_finalize() {
  /* End marker */
  sctk_ib_prof_qp_write(-1, 0, sctk_get_time_stamp());
  sctk_ib_prof_qp_flush();
  close(qp_prof->fd);
}









#if 0


#define QP_PROF_BUFF_SIZE 40000
#define QP_PROF_OUTPUT_FILE "output/qp_prof_%d_%d"

struct sctk_ib_prof_qp_buff_s {
  int proc;
  size_t size;
  double ts;
};

struct sctk_ib_prof_qp_s {
  struct sctk_ib_prof_qp_buff_s *buff;
  int head;
  int fd;
  int task_id;
};

__thread struct sctk_ib_prof_qp_s * qp_prof;

void sctk_ib_prof_qp_init_task(int task_id) {
  char filename[256];

  qp_prof = sctk_malloc ( sizeof(struct sctk_ib_prof_qp_s) );
  qp_prof->buff = sctk_malloc (QP_PROF_BUFF_SIZE * sizeof(struct sctk_ib_prof_qp_buff_s));

  sprintf(filename, QP_PROF_OUTPUT_FILE, sctk_get_node_rank(), task_id);
  qp_prof->fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  assume (qp_prof->fd != -1);

  qp_prof->task_id = task_id;
  qp_prof->head = 0;

  /* Tasks synchronization */
  sctk_terminaison_barrier (task_id);
  sctk_terminaison_barrier (task_id);
  sctk_terminaison_barrier (task_id);
  sctk_terminaison_barrier (task_id);
  sctk_ib_prof_qp_write(-1, 0, sctk_get_time_stamp());
}

void sctk_ib_prof_qp_flush() {
  sctk_debug("Dumping file with %lu elements", qp_prof->head);
  write(qp_prof->fd, qp_prof->buff,
      qp_prof->head * sizeof(struct sctk_ib_prof_qp_buff_s));
  sctk_debug("File dumped");
  qp_prof->head = 0;
}

void sctk_ib_prof_qp_write(int proc, size_t size, double ts) {

  /* We flush */
  if (qp_prof->head > (QP_PROF_BUFF_SIZE - 1)) {
    sctk_ib_prof_qp_flush();
  }

  qp_prof->buff[qp_prof->head].proc = proc;
  qp_prof->buff[qp_prof->head].size = size;
  qp_prof->buff[qp_prof->head].ts = ts;
  qp_prof->head ++;
}

void sctk_ib_prof_qp_finalize_task(int task_id) {
  sctk_debug("End of task %d", task_id);
  /* End marker */
  sctk_ib_prof_qp_write(-1, 0, sctk_get_time_stamp());
  sctk_ib_prof_qp_flush();
  close(qp_prof->fd);
}

#endif

#endif
#endif
