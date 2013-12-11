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
#include "sctk_alloc_config.h"

#ifdef HAVE_HWLOC
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
#ifdef HAVE_HWLOC
static hwloc_topology_t topology;
#endif
#endif

/************************* FUNCTION ************************/
SCTK_INTERN void sctk_alloc_init_topology(void)
{
	#ifdef MPC_Threads
	sctk_topology_init();
	#elif defined(HAVE_HWLOC)
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);
	#endif
}

/************************* FUNCTION ************************/
#ifndef MPC_Threads
SCTK_INTERN int sctk_is_numa_node (void)
{
	#ifdef HAVE_HWLOC
	//avoid to request multiple times as it will not change
	static int res = -1;
	if (res == -1)
		res = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) != 0;
	return res;
	#else
	return 0;
	#endif
}
#endif

/************************* FUNCTION ************************/
#ifndef MPC_Threads
SCTK_INTERN int sctk_get_numa_node_number ()
{
	#ifdef HAVE_HWLOC
	//avoid to request multiple times as it will not change
	static int res = -1;
	if (res == -1)
		res = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) ;
	return res;
	#else
	return sctk_alloc_config()->mm_sources;
	#endif
}
#endif

/************************* FUNCTION ************************/
#ifndef MPC_Threads
SCTK_INTERN int sctk_get_node_from_cpu (const int vp)
{
	#ifdef HAVE_HWLOC
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
#ifdef HAVE_HWLOC
SCTK_INTERN int sctk_get_first_bit_in_bitmap(hwloc_bitmap_t bitmap)
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
#ifdef HAVE_HWLOC
SCTK_INTERN int sctk_get_preferred_numa_node_no_mpc_numa_binding()
{
	hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
	hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
	hwloc_membind_policy_t policy;
	int res = -1;
	int weight;
	int status;
	#if defined(SCTK_ALLOC_DEBUG) && defined(hwloc_bitmap_list_snprintf)
	char buffer[4096];
	#endif

	//if no numa node, return immediately
	if (sctk_is_numa_node() == false)
		return -1;

	//nodes
	// flags = 0 fallback on PROCESS if THREAD is not supported (as for windows).
	status =  hwloc_get_membind_nodeset(topology,nodeset,&policy,0);
	assert(status == 0);
	if (status == 0)
		return -1;

	#if defined(SCTK_ALLOC_DEBUG) && defined(hwloc_bitmap_list_snprintf)
	status = hwloc_bitmap_list_snprintf(buffer,4096,nodeset);
	SCTK_PDEBUG("Current nodes : %s\n",buffer);
	#endif

	//cores
	// flags = 0 fallback on PROCESS if THREAD is not supported (as for windows).
	status =  hwloc_get_membind(topology,cpuset,&policy,0);
	assert(status == 0);
	if (status == 0)
		return -1;

	#if defined(SCTK_ALLOC_DEBUG) && defined(hwloc_bitmap_list_snprintf)
	status = hwloc_bitmap_list_snprintf(buffer,4096,cpuset);
	SCTK_PDEBUG("Current cores : %s\n",buffer);
	#endif

	//nodes from cores
	hwloc_cpuset_to_nodeset(topology,cpuset,nodeset);

	#if defined(SCTK_ALLOC_DEBUG) && defined(hwloc_bitmap_list_snprintf)
	status = hwloc_bitmap_list_snprintf(buffer,4096,nodeset);
	SCTK_PDEBUG("Current nodes from cores : %s\n",buffer);
	#endif

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
#ifndef MPC_Threads
#ifdef HAVE_HWLOC
SCTK_INTERN int sctk_get_preferred_numa_node_no_mpc_thread_binding()
{
	hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
	hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
	int res = -1;
	int weight;
	#if defined(SCTK_ALLOC_DEBUG) && defined(hwloc_bitmap_list_snprintf)
	char buffer[4096];
	#endif
	
	//get current core binding
	//for windows use 0 instead of HWLOC_CPUBIND_THREAD
	int status = hwloc_get_cpubind (topology, cpuset, 0);
	assert(status == 0);
	if (status == 0)
		return -1;

	#if defined(SCTK_ALLOC_DEBUG) && defined(hwloc_bitmap_list_snprintf)
	status = hwloc_bitmap_list_snprintf(buffer,4096,cpuset);
	SCTK_PDEBUG("Current cores : %s\n",buffer);
	#endif

	//nodes from cores
	hwloc_cpuset_to_nodeset(topology,cpuset,nodeset);

	#if defined(SCTK_ALLOC_DEBUG) && defined(hwloc_bitmap_list_snprintf)
	status = hwloc_bitmap_list_snprintf(buffer,4096,nodeset);
	SCTK_PDEBUG("Current nodes from cores : %s\n",buffer);
	#endif

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
#ifndef MPC_Threads
#ifdef HAVE_HWLOC
SCTK_INTERN int sctk_get_preferred_numa_node_no_mpc()
{
	//vars
	int res = -1;

	//try to find by using NUMA bindings
	res = sctk_get_preferred_numa_node_no_mpc_numa_binding();

	//if not found try to find with thread binding on cores
	res = sctk_get_preferred_numa_node_no_mpc_thread_binding();

	return res;
}
#endif
#endif //MPC_Threads

/************************* FUNCTION ************************/
#ifdef HAVE_HWLOC
SCTK_INTERN int sctk_get_preferred_numa_node()
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
#ifdef HAVE_HWLOC
SCTK_INTERN int sctk_alloc_get_current_numa_node_getcpu(void)
{
	//get the current cpu ID
	unsigned int cpu_id;
	unsigned int node_id;
	unsigned int logical_node_id;
	int res = getcpu(&cpu_id,&node_id,NULL);

	//check that we get the same logical ID in hwloc
	logical_node_id = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpu_id)->logical_index;
	assert(logical_node_id == node_id);

	return logical_node_id;
}
#endif //HAVE_HWLOC
#endif //HAVE_GETCPU
#endif //MPC_Threads

/************************* FUNCTION ************************/
#ifdef HAVE_HWLOC
SCTK_INTERN int sctk_alloc_get_current_numa_node_default(void)
{
	//if not, we can use the common memory source which is not specific to one numa node
	//the other way can be to select by random
	SCTK_PDEBUG("Random NUMA selection");
	return -1;
}
#endif

/************************* FUNCTION ************************/
#ifdef HAVE_HWLOC
SCTK_INTERN int sctk_alloc_init_on_numa_node(void)
{
	int node = sctk_get_preferred_numa_node();
	if (node != -1)
		return node;
	else
		return sctk_alloc_get_current_numa_node();
}
#endif

/************************* FUNCTION ************************/
#ifdef HAVE_HWLOC
#ifndef MPC_Threads
SCTK_INTERN hwloc_topology_t sctk_get_topology_object(void)
{
	return topology;
}
#endif
#endif

/************************* FUNCTION ************************/
#ifdef HAVE_HWLOC
SCTK_INTERN void sctk_alloc_migrate_numa_mem(void * addr,sctk_size_t size,int target_numa_node)
{
	//vars
	hwloc_obj_t obj;
	hwloc_topology_t topo;
	int res;

	//errors
	assert(addr != NULL);
	assert(size > 0);
	assert(target_numa_node >= -1);
	assert(target_numa_node <= SCTK_MAX_NUMA_NODE);

	//trivial case
	if (addr == NULL || size == 0 || ! sctk_alloc_config()->numa_migration )
		return;

	//round the size and addr
	/** @todo replace by a cleaner round function. **/
	size += (sctk_addr_t)addr % SCTK_ALLOC_PAGE_SIZE;
	addr = (void*)((sctk_addr_t)addr - (sctk_addr_t)addr % SCTK_ALLOC_PAGE_SIZE);
	if (size % SCTK_ALLOC_PAGE_SIZE != 0)
		size += SCTK_ALLOC_PAGE_SIZE - (size % SCTK_ALLOC_PAGE_SIZE);

	//debug
	SCTK_PDEBUG("Request change of memory binding on area %p [%llu] to node %d.",addr,size,target_numa_node);

	//get topo
	topo = sctk_get_topology_object();
	//get hwloc object for binding
	obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_NODE, target_numa_node);

	//if -1, then reset to default
	if (target_numa_node != -1)
	{
		res = hwloc_set_area_membind_nodeset(topo, addr, size, obj->nodeset, HWLOC_MEMBIND_BIND, HWLOC_MEMBIND_THREAD | HWLOC_MEMBIND_MIGRATE);
	} else {
		res = hwloc_set_area_membind_nodeset(topo, addr, size, obj->nodeset, HWLOC_MEMBIND_DEFAULT, HWLOC_MEMBIND_THREAD);
	}

	//check errors
	if (res != 0)
	{
		SCTK_PDEBUG("Tried to move pages to node %d",target_numa_node);
		warning("Failed to move pages on NUMA node.");
		sctk_alloc_pwarning("numa node id : %d, size : %lu, ptr=%p",target_numa_node,size,addr);
		warning(strerror(res));
	}
}
#endif //HAVE_HWLOC

/************************* FUNCTION ************************/
#ifdef HAVE_HWLOC
/**
 * Bind the current thread on requested core by using hwloc. It's more a debug and test feature than
 * fore real usage, please avoid to use it in release mode.
 * 
 * Source : the global layout of this code is taken from hwloc documentation and examples, thanks to them.
**/
SCTK_INTERN void sctk_alloc_topology_bind_thread_on_core(int id)
{
	//vars
	int depth;
	hwloc_cpuset_t cpuset;
	hwloc_obj_t obj;
	hwloc_topology_t topology;

	//errors
	assert(id >= 0);

	//get topology
	topology = sctk_get_topology_object();

	//debug
	SCTK_PDEBUG("Bind thread with hwloc on core %d\n",id);
	depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_CORE);
	assert(depth != HWLOC_TYPE_DEPTH_UNKNOWN);
	/* Get first. */
	obj = hwloc_get_obj_by_depth(topology, depth,id);
	if (obj) {
			/* Get a copy of its cpuset that we may modify. */
			cpuset = hwloc_bitmap_dup(obj->cpuset);

			/* Get only one logical processor (in case the core is
					SMT/hyperthreaded). */
			hwloc_bitmap_singlify(cpuset);

			/* And try to bind ourself there. */
			if (hwloc_set_cpubind(topology, cpuset, 0)) {
					char *str;
					int error = errno;
					hwloc_bitmap_asprintf(&str, obj->cpuset);
					fprintf(stderr,"Couldn't bind to cpuset %s: %s\n", str, strerror(error));
					free(str);
			}

			/* Free our cpuset copy */
			hwloc_bitmap_free(cpuset);
	}
}
#else
SCTK_INTERN void sctk_alloc_topology_bind_thread_on_core(int id)
{
	warning("Thread binding is not supported, please enable support of hwloc at compile time.");
}
#endif

/********************************* FUNCTION *********************************/
/**
 * Permit to know if the memory allocator must use NUMA mode of not. It depend
 * on presence of NUMA on current node and on field "numa" of MPC configuration.
 * @return Return true if NUMA support must be used.
**/
SCTK_INTERN bool sctk_alloc_is_numa(void)
{
	return sctk_is_numa_node() && sctk_alloc_config()->numa;
}
