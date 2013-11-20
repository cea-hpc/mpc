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
#ifndef __SCTK__INFINIBAND_TOPOLOGY_H_
#define __SCTK__INFINIBAND_TOPOLOGY_H_

#include "sctk.h"
#include "sctk_ibufs.h"
#include "sctk_ib_mmu.h"

#define LOAD_TOPO(rail_ib) \
  struct sctk_ib_topology_s * topo = rail_ib->topology;

extern __thread struct sctk_ib_topology_numa_node_s * * numa_node_task;

typedef struct sctk_ib_topology_numa_node_s {
  int id;
  sctk_spinlock_t polling_lock;
  sctk_ibuf_numa_t ibufs;
  sctk_ib_mmu_t mmu;
  char padd[4096];
} sctk_ib_topology_numa_node_t;

/* Structure only used for the topology initialization */
typedef struct sctk_ib_topology_numa_node_init_s {
  int is_leader;
  sctk_spinlock_t initialization_lock;
} sctk_ib_topology_numa_node_init_t;

typedef struct sctk_ib_topology_s {
  struct sctk_ib_topology_numa_node_s ** nodes;
  struct sctk_ib_topology_numa_node_init_s * init;

/* Default node where the leader has allocated the IB structures.
 * May be used if a task is trying to access NUMA structures and none are allocated */
  struct sctk_ib_topology_numa_node_s * default_node;

  int nodes_nb;
} sctk_ib_topology_t;

void sctk_ib_topology_init(struct sctk_ib_rail_info_s * rail_ib);
void sctk_ib_topology_init_task(struct sctk_rail_info_s * rail, int vp);

TODO("The maximum number of NUMA nodes should be dynamically determined!")
#define MAX_NUMA_NODE_NB 32
__UNUSED__ static void sctk_ib_topology_check_and_allocate_tls (sctk_ib_rail_info_t * rail_ib) {
  /* Create the TLS variable if not already created */
  if (numa_node_task == NULL) {
//    int rails_nb = sctk_network_ib_get_rails_nb() ;
    numa_node_task = sctk_malloc(MAX_NUMA_NODE_NB * sizeof(struct sctk_ib_topology_numa_node_s));
    memset(numa_node_task, 0, MAX_NUMA_NODE_NB * sizeof(struct sctk_ib_topology_numa_node_s));
  }
}

/* Return the IB topology structure the closest from the current task
 *
 * / ! \ MAY return NULL */
 sctk_ib_topology_numa_node_t *
sctk_ib_topology_get_numa_node(struct sctk_ib_rail_info_s * rail_ib);

  sctk_ib_topology_numa_node_t *
sctk_ib_topology_get_default_numa_node(struct sctk_ib_rail_info_s * rail_ib);

#endif
#endif
