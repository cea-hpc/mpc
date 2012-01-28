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

typedef struct sctk_ib_prof_s {
  OPA_int_t counters[128];
} sctk_ib_prof_t;

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
};

#define PROF_INC(r,x) do {                              \
  sctk_ib_rail_info_t *rail_ib = &(r)->network.ib;      \
  OPA_incr_int(&rail_ib->profiler->counters[x]);         \
} while(0)

#define PROF_INC_RAIL_IB(r,x) do {                      \
  OPA_incr_int(&r->profiler->counters[x]);        \
} while(0)


#define PROF_LOAD(r,x) OPA_load_int(&r->profiler->counters[x])

void sctk_ib_prof_init(sctk_ib_rail_info_t *rail_ib);

void sctk_ib_prof_print(sctk_ib_rail_info_t *rail_ib);
#endif
#endif
