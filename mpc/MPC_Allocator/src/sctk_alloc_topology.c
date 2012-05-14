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
/* #   - Valat Sébastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */


/************************** HEADERS ************************/
#include "sctk_alloc_debug.h"
#include "sctk_alloc_topology.h"

#ifdef HAVE_LIBNUMA
#include <hwloc.h>
#endif

#ifdef HAVE_LINUX_GETCPU
//it require linux 2.6.19
#include <linux/getcpu.h>
#define sctk_alloc_get_current_numa_node sctk_alloc_get_current_numa_node_getcpu
#else
//default policy if can't detect current cpu
#define sctk_alloc_get_current_numa_node sctk_alloc_get_current_numa_node_default
#endif

/************************* GLOBALS *************************/
#ifndef MPC_Threads
#ifdef HAVE_LIBNUMA
static hwloc_topology_t topology;
#endif
#endif

/************************* FUNCTION ************************/
void sctk_alloc_init_topology(void)
{
	#ifdef MPC_Threads
	sctk_topology_init();
	#elif defined(HAVE_LIBNUMA)
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
	#endif
}

/************************* FUNCTION ************************/
#ifndef MPC_Threads
bool sctk_is_numa_node (void)
{
	#ifdef HABE_LIBNUMA
	return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) != 0;
	#else
	return false;
	#endif
}
#endif

/************************* FUNCTION ************************/
#ifndef MPC_Threads
int sctk_get_numa_node_number ()
{
	#ifdef HABE_LIBNUMA
	return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) ;
	#else
	return 0;
	#endif
}
#endif

/************************* FUNCTION ************************/
#ifndef MPC_Threads
int sctk_get_node_from_cpu (const int vp)
{
	#ifdef HABE_LIBNUMA
	if(sctk_is_numa_node ()){
		const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
		const hwloc_obj_t node = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, pu);
		return node->logical_index;
	} else {
		return 0;
	}
	#else
	return 0;
	#endif
}
#endif

/************************* FUNCTION ************************/
#ifdef HAVE_LIBNUMA
int sctk_get_first_bit_in_bitmap(hwloc_bitmap_t bitmap)
{
	int last = hwloc_bitmap_last(bitmap);
	int current = hwloc_bitmap_first(bitmap);
	assert(current != -1);
	while (current != last)
	{
		if (hwloc_bitmap_isset(bitmap,current))
			break;
		current = hwloc_bitmap_next(bitmap,current);
	}
	return current;
}
#endif

/************************* FUNCTION ************************/
#ifndef MPC_Threads
#ifdef HAVE_LIBNUMA
int sctk_get_preferred_numa_node_no_mpc()
{
	hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
	hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
	hwloc_membind_policy_t policy;
	int res = -1;
	int weight;
	char buffer[4096];

	//nodes
	int status =  hwloc_get_membind_nodeset(topology,nodeset,&policy,HWLOC_MEMBIND_THREAD);
	assert(status == 0);

	status = hwloc_bitmap_list_snprintf(buffer,4096,nodeset);
	SCTK_PDEBUG("Current nodes : %s\n",buffer);

	//cores
	status =  hwloc_get_membind(topology,cpuset,&policy,HWLOC_MEMBIND_THREAD);
	assert(status == 0);

	status = hwloc_bitmap_list_snprintf(buffer,4096,cpuset);
	SCTK_PDEBUG("Current cores : %s\n",buffer);

	//nodes from cores
	hwloc_cpuset_to_nodeset(topology,cpuset,nodeset);

	status = hwloc_bitmap_list_snprintf(buffer,4096,nodeset);
	SCTK_PDEBUG("Current nodes from cores : %s\n",buffer);

	//calc res
	weight = hwloc_bitmap_weight(nodeset);
	assert(weight != 0);
	if (weight == 1)
		res = sctk_get_first_bit_in_bitmap(nodeset);

	hwloc_bitmap_free(cpuset);
	hwloc_bitmap_free(nodeset);

	return res;
}
#endif
#endif //MPC_Threads

/************************* FUNCTION ************************/
#ifdef HAVE_LIBNUMA
int sctk_get_preferred_numa_node()
{
	#ifdef MPC_Threads
	return sctk_get_node_from_cpu(sctk_get_cpu());
	#else
	return sctk_get_preferred_numa_node_no_mpc();
	#endif
}
#endif

/************************* FUNCTION ************************/
#ifndef MPC_Threads
#ifdef HAVE_GETCPU
#ifdef HAVE_LIBNUMA
int sctk_alloc_get_current_numa_node_getcpu(void)
{
	//get the current cpu ID
	unsigned int cpu_id;
	unsigned int node_id;
	unsigned int logical_node_id;
	int res = getcpu(&cpu_id,&node_id,NULL);

	//check that we get the same logical ID in hwloc
	logical_nod_id = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpu_id)->logical_index;
	assert(logical_node_id == node_id);

	return logical_node_id;
}
#endif //HABE_LIBNUMA
#endif //HAVE_GETCPU
#endif //MPC_Threads

/************************* FUNCTION ************************/
#ifdef HAVE_LIBNUMA
int sctk_alloc_get_current_numa_node_default(void)
{
	//if not, we can use the common memory source which is not specific to one numa node
	//the other way can be to select by random
	SCTK_PDEBUG("Random NUMA selection");
	return -1;
}
#endif

/************************* FUNCTION ************************/
#ifdef HAVE_LIBNUMA
int sctk_alloc_init_on_numa_node(void)
{
	int node = sctk_get_preferred_numa_node();
	if (node != -1)
		return node;
	else
		return sctk_alloc_get_current_numa_node();
}
#endif

/************************* FUNCTION ************************/
#ifdef HAVE_LIBNUMA
#ifndef MPC_Threads
hwloc_topology_t sctk_get_topology_object(void)
{
	return topology;
}
#endif
#endif
