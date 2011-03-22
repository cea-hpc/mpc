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

#ifndef __SCTK__INFINIBAND_PROFILER_H_
#define __SCTK__INFINIBAND_PROFILER_H_
#include "sctk.h"
#include "stdint.h"

#define XSTR(X)  STR(X)
#define STR(X)  #X

typedef enum
{

  /* EAGER MESSAGES */
  IBV_EAGER_NB = 0,
  IBV_EAGER_REQUEST_NB = 1,
  IBV_EAGER_ACK_NB = 2,
  IBV_EAGER_DONE_NB = 3,

  IBV_EAGER_BCAST_NB = 4,
  IBV_EAGER_REDUCE_NB = 5,
  IBV_EAGER_BCAST_INIT_BARRIER_NB = 6,

  IBV_EAGER_SIZE = 7,
  IBV_EAGER_REQUEST_SIZE = 8,
  IBV_EAGER_ACK_SIZE = 9,
  IBV_EAGER_DONE_SIZE = 10,

  IBV_EAGER_BCAST_SIZE = 11,
  IBV_EAGER_REDUCE_SIZE = 12,
  IBV_EAGER_BCAST_INIT_BARRIER_SIZE = 13,

  /* RDVZ MESSAGES */
  IBV_RDVZ_READ_NB = 14,
  IBV_RDVZ_READ_SIZE = 15,

  IBV_NEIGHBOUR_PTP = 16,
  IBV_NEIGHBOUR_COLL = 17,
  IBV_NEIGHBOUR_ALL = 18,

  /* IBUFS */
  IBV_IBUF_CHUNKS = 19,
  IBV_IBUF_TOT_SIZE = 20,

  IBV_QP_CONNECTED = 21,

  IBV_MEM_TRACE = 22,

  /* FRAG BUFFERS */
  IBV_FRAG_EAGER_NB = 23,
  IBV_FRAG_EAGER_SIZE = 24,

} ibv_profiler_id;
#define NB_PROFILE_ID 25

struct sctk_ibv_profiler_entry_s
{
  char name[255];
  uint64_t value;
};

#define ENTRY(X) {#X, 0}

static sctk_spinlock_t locks[NB_PROFILE_ID];

static struct sctk_ibv_profiler_entry_s counters[NB_PROFILE_ID] =
{
  ENTRY(IBV_EAGER_NB),
  ENTRY(IBV_EAGER_REQUEST_NB),
  ENTRY(IBV_EAGER_ACK_NB),
  ENTRY(IBV_EAGER_DONE_NB),

  ENTRY(IBV_EAGER_BCAST_NB),
  ENTRY(IBV_EAGER_REDUCE_NB),
  ENTRY(IBV_EAGER_BCAST_INIT_BARRIER_NB),

  ENTRY(IBV_EAGER_SIZE),
  ENTRY(IBV_EAGER_REQUEST_SIZE),
  ENTRY(IBV_EAGER_ACK_SIZE),
  ENTRY(IBV_EAGER_DONE_SIZE),

  ENTRY(IBV_EAGER_BCAST_SIZE),
  ENTRY(IBV_EAGER_REDUCE_SIZE),
  ENTRY(IBV_EAGER_BCAST_INIT_BARRIER_SIZE),

  /* RDVZ MESSAGES */
  ENTRY(IBV_RDVZ_READ_NB),
  ENTRY(IBV_RDVZ_READ_SIZE),

  ENTRY(IBV_NEIGHBOUR_PTP),
  ENTRY(IBV_NEIGHBOUR_COLL),
  ENTRY(IBV_NEIGHBOUR_ALL),

  /* IBUFS */
  ENTRY(IBV_IBUF_CHUNKS),
  ENTRY(IBV_IBUF_TOT_SIZE),

  ENTRY(IBV_QP_CONNECTED),

  ENTRY(IBV_MEM_TRACE),

  ENTRY(IBV_FRAG_EAGER_NB),
  ENTRY(IBV_FRAG_EAGER_SIZE),
};

void sctk_ibv_profiler_init();

#endif
