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

#if !defined(MPC_Common) || defined(MPC_PosixAllocator)

/************************** HEADERS ************************/
#include <stdlib.h>
#include "sctk_alloc_debug.h"
#include "sctk_alloc_posix.h"
#include "sctk_alloc_topology.h"
#include "sctk_alloc_chain.h"
#include "sctk_alloc_light_mm_source.h"

/************************* GLOBALS *************************/
#ifdef HAVE_HWLOC
/**
 * We create an allocation chain per node to use sctk_malloc_on_node. At init
 * we plug them on the related NUMA memory source.
 * Those chains are shared between all threads.
**/
static struct sctk_alloc_chain sctk_global_alloc_on_node_chain[SCTK_MAX_NUMA_NODE];
/**
 * By default we prefer to use separeted memory source which force the binding, but for future maybe
 * we can provide some options to use the common once from allocator, but they are less safe as
 * they didn't force the NUMA bindind.
**/
static struct sctk_alloc_mm_source_light sctk_global_alloc_on_node_mm_src[SCTK_MAX_NUMA_NODE];
/**
 * For debug, to check if it was initilized before usage.
**/
static bool sctk_global_alloc_on_node_initilized = false;
#endif //HAVE_HWLOC

/************************* FUNCTION ************************/
/**
 * Global initialisation of the array of allocation chain used for malloc on node.
 * Up to now it only reset the array to fill it with NULL pointers for lazy initialisations.
 * CAUTION this function must be called only once and never in parallel as it use global variables.
 * @param numa_nodes Define the number of numa nodes for which to init the array.
/*/
SCTK_INTERN void sctk_malloc_on_node_init(int numa_nodes)
{
	//if don't have hwloc, didn't need NUMA support so malloc_on_node fallback on standard malloc.
	#ifdef HAVE_HWLOC
	//vars
	int i;

	//errors
	assert(numa_nodes >= 0);
	assert(numa_nodes < SCTK_MAX_NUMA_NODE);
	assert(sctk_global_alloc_on_node_initilized == false);

	//init all to null to avoid stupid bug if bad access
	memset(sctk_global_alloc_on_node_chain,0,sizeof(sctk_global_alloc_on_node_chain));
	memset(sctk_global_alloc_on_node_mm_src,0,sizeof(sctk_global_alloc_on_node_mm_src));

	//do nothing if not numa node
	if ( sctk_is_numa_node() == false)
		return;

	//setup allocations chains for all nodes
	for( i = 0 ; i < numa_nodes ; ++i)
	{
		//init strict memory source whith no keed memory
		sctk_alloc_mm_source_light_init(sctk_global_alloc_on_node_mm_src + i,i,SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_STRICT);

		//init empty user chain
		sctk_alloc_chain_user_init(sctk_global_alloc_on_node_chain + i,NULL,0,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT|SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE);

		//link to the memory source
		assert(sctk_global_alloc_on_node_mm_src[i].source.request_memory != NULL);
		sctk_global_alloc_on_node_chain[i].source = &sctk_global_alloc_on_node_mm_src[i].source;
	}

	//mark as done
	sctk_global_alloc_on_node_initilized = true;
	#endif //HAVE_HWLOC
}

/************************* FUNCTION ************************/
/**
 * Reset the global variables, CAUTION, this is ONLY FOR UNIT TESTS.
**/
void sctk_malloc_on_node_reset(void) {
#ifdef HAVE_HWLOC
  sctk_global_alloc_on_node_initilized = false;
#endif // HAVE_HWLOC
}

/************************* FUNCTION ************************/
/**
 * Return the allocation chain to use for explicit NUMA allocation on the given node.
 * @param node Define the node on which to do malloc_on_node.
**/
#ifdef HAVE_HWLOC
struct sctk_alloc_chain *sctk_malloc_on_node_get_chain(int node) {
  // vars
  struct sctk_alloc_chain *chain;

  // errors
  assert(sctk_global_alloc_on_node_initilized);
  assume_m(node >= 0 && node < SCTK_MAX_NUMA_NODE,
           "Invalid NUMA node in sctk_malloc_on_node function.");

  // get the chain
  chain = sctk_global_alloc_on_node_chain + node;

  // check that it was init
  assume_m(chain->source != NULL,
           "Invalid NUMA node in sctk_malloc_on_node function.");

  // return
  return chain;
}
#endif //HAVE_HWLOC

/************************* FUNCTION ************************/
/**
 * In UMA mode, can directly use the current thread allocator.
**/
void *sctk_malloc_on_node_uma(size_t size, int node) {
  assume_m(node == 0, "You request mapping on NUMA node different from 0, but "
                      "their is no NUMA node or NUMA isn't supported.");
#if defined(MPC_Common) && !defined(MPC_PosixAllocator)
  return malloc(size);
#else  // defined(MPC_Common) && !defined(MPC_PosixAllocator)
  return sctk_malloc(size);
#endif // defined(MPC_Common) && !defined(MPC_PosixAllocator)
}

/************************* FUNCTION ************************/
/**
 * In NUMA mode, select the good NUMA chain and alloc on it.
**/
#ifdef HAVE_HWLOC
void *sctk_malloc_on_node_numa(size_t size, int node) {
  // vars
  void *ptr;
  struct sctk_alloc_chain *chain;

  // check if NUMA is enabled.
  if (sctk_is_numa_node()) {
    // get the good chain
    chain = sctk_malloc_on_node_get_chain(node);
    assert(chain != NULL);

    // allocate
    ptr = sctk_alloc_chain_alloc(chain, size);
  } else {
    ptr = sctk_malloc_on_node_uma(size, node);
  }

  // return
  return ptr;
}
#endif //HAVE_HWLOC

/************************* FUNCTION ************************/
/**
 * Request memory allocation on a specific NUMA node.
 * @param size Size of the memory bloc to allocate.
 * @param node Define the numa node on which to allocate.
 */
SCTK_PUBLIC void * sctk_malloc_on_node (size_t size, int node)
{
	#ifdef HAVE_HWLOC
		return sctk_malloc_on_node_numa(size,node);
	#else //HAVE_HWLOC
		return sctk_malloc_on_node_uma(size,node);
	#endif //HAVE_HWLOC
}

#endif //defined(MPC_Common) && defined(MPC_PosixAllocator)
