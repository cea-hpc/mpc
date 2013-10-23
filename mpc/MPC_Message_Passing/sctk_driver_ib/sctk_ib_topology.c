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

#include "sctk_ibufs.h"

/* #define SCTK_IB_MODULE_DEBUG */
#define SCTK_IB_MODULE_NAME "TOPO"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_eager.h"
#include "sctk_ib_config.h"
#include "sctk_ib_prof.h"
#include "sctk_ibufs_rdma.h"
#include "sctk_ib_topology.h"

/* For the moment, the following structures are supported by the TOPOLOGY interface :
 *  - Network buffers
 *  - Pinned cache memory
 *  - Polling lock
 *  - CP ? (to be moved !)
 *
 * */

/* TLS variable sized according the number of IB rails. Allows a more efficient access to the structure
 * of the closest node */
__thread struct sctk_ib_topology_numa_node_s ** numa_node_task = NULL;

static void sctk_ib_topology_init_tls (sctk_ib_rail_info_t * rail_ib, sctk_ib_topology_numa_node_t * node) {
  assume(numa_node_task);
  if (numa_node_task[rail_ib->rail_nb] == NULL) {
    numa_node_task[rail_ib->rail_nb] = node;
  }
}

void sctk_ib_topology_init_task(struct sctk_rail_info_s * rail, int vp) {
  sctk_ib_rail_info_t* rail_ib = &rail->network.ib;
  LOAD_TOPO(rail_ib);
  LOAD_CONFIG(rail_ib);
  int node_nb =  sctk_get_node_from_cpu(vp);
  int nodes_nb = topo->nodes_nb;
  sctk_ib_topology_numa_node_init_t * init = &topo->init[node_nb];

  sctk_ib_topology_check_and_allocate_tls(rail_ib);

  sctk_nodebug("[%d] Initializing task on vp %d node %d", rail_ib->rail_nb, vp, node_nb);
  /* Only one task allocates the IB structures per NUMA node */
  sctk_spinlock_lock(&init->initialization_lock);
  sctk_ib_topology_numa_node_t * node = topo->nodes[node_nb];
  /* Is this is the first task to enter the init for the node, it is the leader for the node.
   * The leader allocates and init IB structures for the NUMA node */
  if (init->is_leader == 1) {
    sctk_nodebug("[%d] Task leader on node init SR on node %d", rail_ib->rail_nb, node_nb);
    assume(node == NULL);
    node = sctk_malloc(sizeof(sctk_ib_topology_numa_node_t));
    /* We touch the pages. The thread has already be pinned */
    memset(node, 0, sizeof(sctk_ib_topology_numa_node_t));
    topo->nodes[node_nb] = node;
    if (topo->default_node == NULL) topo->default_node = node;

    node->id = node_nb;
    node->polling_lock = SCTK_SPINLOCK_INITIALIZER;

    /* WARNING: we *MUST* initialize the TLS before initializing IB structure
     * as they access to the TLS variable*/
    sctk_ib_topology_init_tls(rail_ib, node);

    sctk_ib_mmu_init(rail_ib, &node->mmu, node);
    sctk_ibuf_node_set_mmu(&node->ibufs, &node->mmu);
    node->ibufs.numa_node = node;
    sctk_ibuf_init_numa_node(rail_ib, &node->ibufs, config->init_ibufs, 1);
    init->is_leader = 0;
    sctk_nodebug("NUMA Node node %d on rail %d initialized", node_nb, rail_ib->rail_nb);
  } else {
    sctk_ib_topology_init_tls(rail_ib, node);
  }
  sctk_spinlock_unlock(&init->initialization_lock);

  /* The last node is the SRQ. We init it ! */
  init = &topo->init[nodes_nb];
  sctk_spinlock_lock(&init->initialization_lock);
  if (init->is_leader == 1) {
    sctk_nodebug("Task leader on node init SRQ on node %d", nodes_nb);
    sctk_ib_topology_numa_node_t * srq_node = sctk_malloc(sizeof(sctk_ib_topology_numa_node_t));
    memset(srq_node, 0, sizeof(sctk_ib_topology_numa_node_t));

    assume(topo->nodes[nodes_nb] == NULL);
    topo->nodes[nodes_nb] = srq_node;

    srq_node->id = nodes_nb;
    srq_node->polling_lock = SCTK_SPINLOCK_INITIALIZER; /* Not needed */

    sctk_ib_mmu_init(rail_ib, &srq_node->mmu, node);
    sctk_ibuf_node_set_mmu(&srq_node->ibufs, &srq_node->mmu);
    srq_node->ibufs.numa_node = srq_node;
    sctk_ibuf_init_numa_node(rail_ib, &srq_node->ibufs, config->init_recv_ibufs, 1);
    sctk_ibuf_pool_set_node_srq_buffers(rail_ib, &srq_node->ibufs);
    init->is_leader = 0;
    sctk_nodebug("SRQ node ready for rail %d", rail_ib->rail_nb);
  }
  sctk_spinlock_unlock(&init->initialization_lock);

}

static int sctk_ib_topology_use_default_node = 0;

void
sctk_ib_topology_init(struct sctk_ib_rail_info_s * rail_ib)
{
  struct sctk_ib_topology_s * topology;
  int alloc_nb;
  unsigned int nodes_nb;
  int i;

  rail_ib->topology = sctk_malloc(sizeof(sctk_ib_topology_t));
  topology = rail_ib->topology;
  assume(topology);
  memset(topology, 0, sizeof(sctk_ib_topology_t));

  char * env;
  if ( (env = getenv("MPC_IBV_USE_DEFAULT_NODE")) != NULL) {
    sctk_ib_topology_use_default_node = atoi(env);
  }
  if (sctk_ib_topology_use_default_node) {
    sctk_error("Use default node !!");
  }

  nodes_nb = sctk_get_numa_node_number();
 /* FIXME: get_numa_node_number may return 1 when SMP */
  nodes_nb = (nodes_nb == 0) ? 1 : nodes_nb;
  /* We allocate a pool in addition in order to store the SRQ buffers */
  alloc_nb = nodes_nb + 1;
  topology->nodes_nb = nodes_nb;
  topology->default_node = NULL;

  /* Allocate node entries */
  topology->nodes = sctk_malloc(alloc_nb * sizeof(sctk_ib_topology_numa_node_t*));
  memset(topology->nodes, 0, alloc_nb * sizeof(sctk_ib_topology_numa_node_t*));

  /* We allocate a structure which is used ONLY for initialization */
  topology->init = sctk_malloc(alloc_nb * sizeof(sctk_ib_topology_numa_node_init_t));
  memset(topology->init, 0, alloc_nb * sizeof(sctk_ib_topology_numa_node_init_t));

  /* Init each node */
  for (i=0; i < nodes_nb+1; ++i) {
    sctk_ib_topology_numa_node_init_t * init = &topology->init[i];
    init->initialization_lock = SCTK_SPINLOCK_INITIALIZER;
    init->is_leader = 1;
  }

  sctk_ibuf_pool_init(rail_ib);
}

  sctk_ib_topology_numa_node_t *
sctk_ib_topology_get_default_numa_node(struct sctk_ib_rail_info_s * rail_ib) {
  sctk_ib_topology_check_and_allocate_tls(rail_ib);

  LOAD_TOPO(rail_ib);
  return topo->default_node;
}

  sctk_ib_topology_numa_node_t *
sctk_ib_topology_get_numa_node(struct sctk_ib_rail_info_s * rail_ib)
{
  const int rail_nb = rail_ib->rail_nb;
  PROF_TIME_START(rail_ib->rail, ib_get_numa);

  if (sctk_ib_topology_use_default_node) {
    PROF_TIME_END(rail_ib->rail, ib_get_numa);
    return sctk_ib_topology_get_default_numa_node(rail_ib);
  }

  sctk_ib_topology_check_and_allocate_tls(rail_ib);

  if (numa_node_task[rail_nb] == NULL) {
    LOAD_TOPO(rail_ib);
    int vp = sctk_thread_get_vp();
    struct sctk_ib_topology_numa_node_s * node;
    // TODO: we should initialize the VP number for *EACH* created thread, in *EVERY* thread modes
    if (vp < 0) {
      node = topo->default_node;
    } else {
      int node_num =  sctk_get_node_from_cpu(vp);
      assume(topo);
      assume(node_num < topo->nodes_nb);
      if (topo->nodes[node_num] == NULL) {
        node = topo->default_node;
      } else {
        node = topo->nodes[node_num];
      }
    }

    numa_node_task[rail_nb] = node;
    assume(numa_node_task[rail_nb]);
  }

  PROF_TIME_END(rail_ib->rail, ib_get_numa);
  return numa_node_task[rail_nb];
}

#endif
