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

#ifndef __SCTK__INFINIBAND_TOPOLOGY_H_
#define __SCTK__INFINIBAND_TOPOLOGY_H_


#include "ibufs.h"

typedef struct _mpc_lowcomm_ib_topology_numa_node_s
{
	int                         id;
	mpc_common_spinlock_t       polling_lock;
	_mpc_lowcomm_ib_ibuf_numa_t ibufs;
	char                        padd[4096];
} _mpc_lowcomm_ib_topology_numa_node_t;

/* Structure only used for the topology initialization */
typedef struct _mpc_lowcomm_ib_topology_numa_node_init_s
{
	int                   is_leader;
	mpc_common_spinlock_t initialization_lock;
} _mpc_lowcomm_ib_topology_numa_node_init_t;

typedef struct _mpc_lowcomm_ib_topology_s
{
	struct _mpc_lowcomm_ib_topology_numa_node_s **    nodes;
	struct _mpc_lowcomm_ib_topology_numa_node_init_s *init;

	/* Default node where the leader has allocated the IB structures.
	 * May be used if a task is trying to access NUMA structures and none are allocated */
	struct _mpc_lowcomm_ib_topology_numa_node_s *     default_node;

	int                                               numa_node_count;
} _mpc_lowcomm_ib_topology_t;

void _mpc_lowcomm_ib_topology_init(_mpc_lowcomm_ib_topology_t *topology);
void _mpc_lowcomm_ib_topology_free(struct sctk_ib_rail_info_s *rail);
void _mpc_lowcomm_ib_topology_init_rail(struct sctk_ib_rail_info_s *rail_ib);

void _mpc_lowcomm_ib_topology_init_task(struct sctk_rail_info_s *rail, int vp);
void _mpc_lowcomm_ib_topology_free_task();

/* Return the IB topology structure the closest from the current task
 *
 * / ! \ MAY return NULL */
_mpc_lowcomm_ib_topology_numa_node_t * _mpc_lowcomm_ib_topology_get_numa_node(struct sctk_ib_rail_info_s *rail_ib);

_mpc_lowcomm_ib_topology_numa_node_t * _mpc_lowcomm_ib_topology_get_default_numa_node(struct sctk_ib_rail_info_s *rail_ib);

#endif
