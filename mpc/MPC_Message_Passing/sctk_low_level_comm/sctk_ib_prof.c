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
#include "utlist.h"
#include "sctk_route.h"
#include "sctk_ib_prof.h"

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

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void sctk_ib_prof_init(sctk_ib_rail_info_t *rail_ib) {
  rail_ib->profiler = sctk_malloc(sizeof(sctk_ib_prof_t));
  assume(rail_ib->profiler);
  memset(rail_ib->profiler, 0, sizeof(sctk_ib_prof_t));
}

void sctk_ib_prof_print(sctk_ib_rail_info_t *rail_ib) {
  fprintf(stderr, "[%d] %d %d %d %d %d %d\n", sctk_process_rank,
      PROF_LOAD(rail_ib, alloc_mem),
      PROF_LOAD(rail_ib, free_mem),
      PROF_LOAD(rail_ib, qp_created),
      PROF_LOAD(rail_ib, eager_nb),
      PROF_LOAD(rail_ib, buffered_nb),
      PROF_LOAD(rail_ib, rdma_nb));
}
#endif
#endif
