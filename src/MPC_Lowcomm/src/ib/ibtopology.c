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

#include "ibufs.h"

/* #define MPC_LOWCOMM_IB_MODULE_DEBUG */
#define MPC_LOWCOMM_IB_MODULE_NAME    "TOPO"
#include "ibtoolkit.h"
#include "ib.h"
#include "ibeager.h"
#include "ibconfig.h"
#include "ibufs_rdma.h"
#include "ibtopology.h"
#include "rail.h"

#include <mpc_thread.h>
#include <mpc_topology.h>
#include <sctk_alloc.h>


/* used to remember __thread var init for IB re-enabling */
extern volatile char *vps_reset;

/* For the moment, the following structures are supported by the TOPOLOGY interface :
 *  - Network buffers
 *  - Pinned cache memory
 *  - Polling lock
 *  - CP ? (to be moved !)
 *
 * */

/* TLS variable sized according the number of IB rails. Allows a more efficient access to the structure
 * of the closest node */
static __thread struct _mpc_lowcomm_ib_topology_numa_node_s **__ib_rail_numas = NULL;

static void __check_reset_tls(void)
{
	int nvp = mpc_topology_get_pu();

	/* if __thread vars need to be reset when re-enabling the rail */
	if(vps_reset[nvp] == 0)
	{
		if(__ib_rail_numas)
		{
			sctk_free(__ib_rail_numas);
			__ib_rail_numas = NULL;
		}
	}

	/* Create the TLS variable if not already created */
	if(__ib_rail_numas == NULL)
	{
		int rails_nb = sctk_rail_count();
		__ib_rail_numas = sctk_malloc(rails_nb * sizeof(struct _mpc_lowcomm_ib_topology_numa_node_s) );
		memset(__ib_rail_numas, 0, rails_nb * sizeof(struct _mpc_lowcomm_ib_topology_numa_node_s) );
	}
}

static void __init_tls(_mpc_lowcomm_ib_rail_info_t *rail_ib, _mpc_lowcomm_ib_topology_numa_node_t *node)
{
	assume(__ib_rail_numas);

	if(__ib_rail_numas[rail_ib->rail_nb] == NULL)
	{
		__ib_rail_numas[rail_ib->rail_nb] = node;
	}
}

void _mpc_lowcomm_ib_topology_init_task(struct sctk_rail_info_s *rail, int vp)
{
	_mpc_lowcomm_ib_rail_info_t *rail_ib            = &rail->network.ib;
	struct _mpc_lowcomm_ib_topology_s *topo = rail_ib->topology;
	struct _mpc_lowcomm_config_struct_net_driver_infiniband *config = rail_ib->config;
	int node_nb         = mpc_topology_get_numa_node_from_cpu(vp);
	int numa_node_count = topo->numa_node_count;

	_mpc_lowcomm_ib_topology_numa_node_init_t *init = &topo->init[node_nb];

	__check_reset_tls();

	mpc_common_nodebug("[%d] Initializing task on vp %d node %d", rail_ib->rail_nb, vp, node_nb);

	/* Only one task allocates the IB structures per NUMA node */
	mpc_common_spinlock_lock(&init->initialization_lock);

	/* First try to retrieve a previously allocated node */
	_mpc_lowcomm_ib_topology_numa_node_t *node = topo->nodes[node_nb];

	/* Is this is the first task to enter the init for the node, it is the leader for the node.
	 * The leader allocates and init IB structures for the NUMA node */
	if(init->is_leader == 1)
	{
		mpc_common_nodebug("[%d] Task leader on node init SR on node %d", rail_ib->rail_nb, node_nb);

		/* Allocate a new numa node */
		node = sctk_malloc(sizeof(_mpc_lowcomm_ib_topology_numa_node_t) );
		/* We touch the pages. The thread has already be pinned */
		memset(node, 0, sizeof(_mpc_lowcomm_ib_topology_numa_node_t) );

		/* If no previous default node set it as default */
		if(topo->default_node == NULL)
		{
			topo->default_node = node;
		}

		node->id = node_nb;
		mpc_common_spinlock_init(&node->polling_lock, 0);

		/* WARNING: we *MUST* initialize the TLS before initializing IB structure
		 * as they access to the TLS variable*/
		__init_tls(rail_ib, node);

		/* Register it */
		assume(topo->nodes[node_nb] == NULL);
		topo->nodes[node_nb] = node;

		node->ibufs.numa_node = node;
		_mpc_lowcomm_ib_ibuf_init_numa(rail_ib, &node->ibufs, config->init_ibufs, 1);
		init->is_leader = 0;
		mpc_common_nodebug("NUMA Node node %d on rail %d initialized", node_nb, rail_ib->rail_nb);
	}
	else
	{
		__init_tls(rail_ib, node);
	}

	mpc_common_spinlock_unlock(&init->initialization_lock);

	/* The last node is the SRQ. We init it ! */
	init = &topo->init[numa_node_count];
	mpc_common_spinlock_lock(&init->initialization_lock);

	if(init->is_leader == 1)
	{
		mpc_common_nodebug("Task leader on node init SRQ on node %d", numa_node_count);
		_mpc_lowcomm_ib_topology_numa_node_t *srq_node = sctk_malloc(sizeof(_mpc_lowcomm_ib_topology_numa_node_t) );
		memset(srq_node, 0, sizeof(_mpc_lowcomm_ib_topology_numa_node_t) );

		assume(topo->nodes[numa_node_count] == NULL);
		topo->nodes[numa_node_count] = srq_node;

		srq_node->id = numa_node_count;
		mpc_common_spinlock_init(&srq_node->polling_lock, 0); /* Not needed */

		srq_node->ibufs.numa_node = srq_node;

		_mpc_lowcomm_ib_ibuf_init_numa(rail_ib, &srq_node->ibufs, config->init_recv_ibufs, 1);
		_mpc_lowcomm_ib_ibuf_set_node_srq_buffers(rail_ib, &srq_node->ibufs);

		init->is_leader = 0;
		mpc_common_nodebug("SRQ node ready for rail %d", rail_ib->rail_nb);
	}

	mpc_common_spinlock_unlock(&init->initialization_lock);
}

void _mpc_lowcomm_ib_topology_free_task(void)
{
	if(__ib_rail_numas)
	{
		sctk_free(__ib_rail_numas);
		__ib_rail_numas = NULL;
	}
}

static int __use_default_node = 0;

void _mpc_lowcomm_ib_topology_init(_mpc_lowcomm_ib_topology_t *topology)
{
	/* Are we forced on a single node ? */
	char *env;

	if( (env = getenv("MPC_IBV_USE_DEFAULT_NODE") ) != NULL)
	{
		__use_default_node = atoi(env);
	}

	if(__use_default_node)
	{
		mpc_common_debug_error("Use default node !!");
	}

	/* Compute node and alloc count */

	int numa_node_count = mpc_topology_get_numa_node_count();
	/* FIXME: get_numa_node_number may return 1 when SMP */
	numa_node_count = (numa_node_count == 0) ? 1 : numa_node_count;
	/* We allocate a pool in addition in order to store the SRQ buffers */
	int alloc_nb = numa_node_count + 1;

	/* Fill the topology object */

	topology->numa_node_count = numa_node_count;
	topology->default_node    = NULL;
	/* Allocate node entries */
	topology->nodes = sctk_calloc(alloc_nb, sizeof(_mpc_lowcomm_ib_topology_numa_node_t *) );
	/* We allocate a structure which is used ONLY for initialization */
	topology->init = sctk_calloc(alloc_nb, sizeof(_mpc_lowcomm_ib_topology_numa_node_init_t) );

	int i;
	/* Init each node */
	for(i = 0; i < numa_node_count + 1; ++i)
	{
		_mpc_lowcomm_ib_topology_numa_node_init_t *init = &topology->init[i];
		mpc_common_spinlock_init(&init->initialization_lock, 0);
		init->is_leader = 1;
	}
}

void _mpc_lowcomm_ib_topology_init_rail(struct _mpc_lowcomm_ib_rail_info_s *rail_ib)
{
	/* Allocate the topology structure */
	struct _mpc_lowcomm_ib_topology_s *topology = sctk_malloc(sizeof(_mpc_lowcomm_ib_topology_t) );

	assume(topology);
	memset(topology, 0, sizeof(_mpc_lowcomm_ib_topology_t) );

	/* Initialize it */
	_mpc_lowcomm_ib_topology_init(topology);

	/* Set in the rail */
	rail_ib->topology = topology;

	/* Initialize the pool */
	_mpc_lowcomm_ib_ibuf_pool_init(rail_ib);
}

void _mpc_lowcomm_ib_topology_free(struct _mpc_lowcomm_ib_rail_info_s *rail_ib)
{
	_mpc_lowcomm_ib_ibuf_pool_free(rail_ib);

	int i = -1, numa_node_nb = rail_ib->topology->numa_node_count;
	for(i = 0; i < numa_node_nb; ++i)
	{
		if(rail_ib->topology->nodes[i])
		{
			_mpc_lowcomm_ib_ibuf_free_numa(&rail_ib->topology->nodes[i]->ibufs);
			sctk_free(rail_ib->topology->nodes[i]);
			rail_ib->topology->nodes[i] = NULL;
		}
	}

	if(rail_ib->topology->nodes[numa_node_nb])
	{
		_mpc_lowcomm_ib_ibuf_free_numa(&rail_ib->topology->nodes[numa_node_nb]->ibufs);
		sctk_free(rail_ib->topology->nodes[numa_node_nb]);
		rail_ib->topology->nodes[numa_node_nb] = NULL;
	}

	sctk_free(rail_ib->topology->nodes);  rail_ib->topology->nodes = NULL;
	sctk_free(rail_ib->topology->init);  rail_ib->topology->init   = NULL;
	sctk_free(rail_ib->topology);  rail_ib->topology = NULL;
}

_mpc_lowcomm_ib_topology_numa_node_t *
_mpc_lowcomm_ib_topology_get_default_numa_node(struct _mpc_lowcomm_ib_rail_info_s *rail_ib)
{
	__check_reset_tls();

	struct _mpc_lowcomm_ib_topology_s *topo = rail_ib->topology;
	return topo->default_node;
}

_mpc_lowcomm_ib_topology_numa_node_t *
_mpc_lowcomm_ib_topology_get_numa_node(struct _mpc_lowcomm_ib_rail_info_s *rail_ib)
{
	const int rail_nb = rail_ib->rail_nb;

	if(__use_default_node)
	{
		return _mpc_lowcomm_ib_topology_get_default_numa_node(rail_ib);
	}

	__check_reset_tls();

	if(__ib_rail_numas[rail_nb] == NULL)
	{
		struct _mpc_lowcomm_ib_topology_s *topo = rail_ib->topology;
		int vp = mpc_topology_get_pu();
		struct _mpc_lowcomm_ib_topology_numa_node_s *node;


		// TODO: we should initialize the VP number for *EACH* created thread, in *EVERY* thread modes
		if(vp < 0)
		{
			node = topo->default_node;
		}
		else
		{
			int node_num = mpc_topology_get_numa_node_from_cpu(vp);
			assume(topo);

			if(topo->numa_node_count == 0)
			{
				node = topo->default_node;
			}
			else if(topo->nodes[node_num] == NULL)
			{
				node = topo->default_node;
			}
			else
			{
				node = topo->nodes[node_num];
			}
		}

		__ib_rail_numas[rail_nb] = node;
		assume(__ib_rail_numas[rail_nb]);
	}


	return __ib_rail_numas[rail_nb];
}
