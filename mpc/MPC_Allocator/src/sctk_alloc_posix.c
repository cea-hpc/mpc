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
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include "sctk_alloc_lock.h"
#include "sctk_allocator.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_spy.h"
#include "sctk_alloc_inlined.h"
#include "sctk_alloc_light_mm_source.h"
//optional headers
#ifdef HAVE_LIBNUMA
#include "sctk_alloc_topology.h"
#endif

//if have NUMA support
#ifdef HAVE_LIBNUMA
#include <hwloc.h>
// #include "../../../install/include/mpcmp.h"
/** Select the NUMA memory source init function. **/
#define sctk_alloc_posix_mmsrc_init sctk_alloc_posix_mmsrc_numa_init
#else
/** Select the UMA memory source init function. **/
#define sctk_alloc_posix_mmsrc_init sctk_alloc_posix_mmsrc_uma_init
#endif

/*************************** ENUM **************************/
enum sctk_alloc_posix_init_state
{
	SCTK_ALLOC_POSIX_INIT_NONE,
	SCTK_ALLOC_POSIX_INIT_EGG,
	SCTK_ALLOC_POSIX_INIT_DEFAULT,
	SCTK_ALLOC_POSIX_INIT_NUMA,
};

/************************* GLOBALS *************************/
/** Permit to know if the base initialization was done or not. **/
static enum sctk_alloc_posix_init_state sctk_global_base_init = SCTK_ALLOC_POSIX_INIT_NONE;
/** Global memory source for the current process (shared between all threads). **/
static struct sctk_alloc_mm_source_light * sctk_global_memory_source[SCTK_MAX_NUMA_NODE + 1];
/**
 * Basic allocation for allocator internal management (allocation of chain structures....).
 * It's required to boostrap the allocator.
**/
static struct sctk_alloc_chain sctk_global_egg_chain;
/** Define the TLS pointer to the current allocation chain. **/
__thread struct sctk_alloc_chain * sctk_current_alloc_chain = NULL;

/************************* FUNCTION ************************/
#ifdef MPC_check_compatibility
/** Defined in MPC **/
int sctk_get_cpu();
/** Defined in MPC **/
int sctk_get_node_from_cpu (int cpu);
#endif

/************************* FUNCTION ************************/
/**
 * Update the current thread local allocation chain.
 * @param chain Define the allocation chain to setup.
**/
void sctk_alloc_posix_set_default_chain(struct sctk_alloc_chain * chain)
{
	//errors
	//assume(chain != NULL,"Can't set a default NULL allocation chain for local thread.");

	//setup allocation chain for current thread
	sctk_current_alloc_chain = chain;
}

/************************* FUNCTION ************************/
/** Init the global memory source for UMA architecture. **/
SCTK_STATIC void sctk_alloc_posix_mmsrc_uma_init(void)
{
	//error
	assume (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_EGG,"Invalid init state while calling allocator default init phase.");

	sctk_global_memory_source[0] = sctk_alloc_chain_alloc(&sctk_global_egg_chain,sizeof(struct sctk_alloc_mm_source_default));
	//sctk_alloc_mm_source_default_init(sctk_global_memory_source[0],SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE);
	sctk_alloc_mm_source_light_init(sctk_global_memory_source[0],0,SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT);
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_posix_mmsrc_numa_init_node(int id)
{
	//errors and debug
	assume(id <= SCTK_MAX_NUMA_NODE,"Caution, you get more node than supported by allocator. Limit is setup by SCTK_MAX_NUMA_NODE macro in sctk_alloc_posix.c.");
	SCTK_PDEBUG("Init memory source id = %d , MAX_NUMA_NODE = %d",id,SCTK_MAX_NUMA_NODE);

	SCTK_PDEBUG("Allocator init phase : Default");
	sctk_global_memory_source[id] = sctk_alloc_chain_alloc(&sctk_global_egg_chain,sizeof(struct sctk_alloc_mm_source_default));
	//sctk_alloc_mm_source_default_init(sctk_global_memory_source[id],SCTK_ALLOC_HEAP_BASE + SCTK_ALLOC_HEAP_SIZE * id,SCTK_ALLOC_HEAP_SIZE);
	if (id == SCTK_DEFAULT_NUMA_MM_SOURCE_ID)
		sctk_alloc_mm_source_light_init(sctk_global_memory_source[id],SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE,SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT);
	else
		sctk_alloc_mm_source_light_init(sctk_global_memory_source[id],id,SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT);
}

/************************* FUNCTION ************************/
SCTK_STATIC void sctk_alloc_posix_mmsrc_numa_init_phase_default(void)
{
	//error
	assume (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_EGG,"Invalid init state while calling allocator default init phase.");

	SCTK_PDEBUG("Allocator init phase : NUMA");
	//init the default source ID of thead which are on unknown NUMA node or for initial thread in MPC.
	sctk_alloc_posix_mmsrc_numa_init_node(SCTK_DEFAULT_NUMA_MM_SOURCE_ID);
}

/************************* FUNCTION ************************/
void sctk_alloc_posix_mmsrc_numa_init_phase_numa(void)
{
	#ifdef HAVE_LIBNUMA
	//vars
	int nodes;
	int i;

	//error
	#ifndef MPC_Common
	assume (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_EGG,"Invalid init state while calling allocator NUMA init phase.");
	#else
	assume (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_DEFAULT,"Invalid init state while calling allocator NUMA init phase.");
	#endif

	//get number of nodes
	SCTK_PDEBUG("Init numa nodes");
	nodes = sctk_get_numa_node_number();
	assume(nodes <= SCTK_MAX_NUMA_NODE,"Caution, you get more node than supported by allocator. Limit is setup by SCTK_MAX_NUMA_NODE macro in sctk_alloc_posix.c.");

	//debug
	SCTK_PDEBUG("Init with NUMA_NODES = %d , MAX_NUMA_NODE = %d",nodes,SCTK_MAX_NUMA_NODE);

	if (nodes == 0)
	{
		sctk_global_memory_source[0] = sctk_global_memory_source[SCTK_DEFAULT_NUMA_MM_SOURCE_ID];
	} else {
		//init heaps for each NUMA node
		SCTK_PDEBUG("Memory source size is %lu GB.",((SCTK_ALLOC_HEAP_SIZE)/1024ul/1024ul/1024ul));
		for ( i = 0 ; i < nodes ; ++i)
			sctk_alloc_posix_mmsrc_numa_init_node(i);
	}
	#endif

	//mark NUMA init phase as done.
	sctk_global_base_init = SCTK_ALLOC_POSIX_INIT_NUMA;
}

/************************* FUNCTION ************************/
void sctk_alloc_posix_plug_on_egg_allocator(void)
{
	sctk_alloc_posix_set_default_chain(&sctk_global_egg_chain);
}

/************************* FUNCTION ************************/
/**
 * Init the global memory sources for NUMA architecture.
 * It init one source for each available NUMA node.
**/
SCTK_STATIC void sctk_alloc_posix_mmsrc_numa_init(void)
{
	//setup default memory source
	sctk_alloc_posix_mmsrc_numa_init_phase_default();

	//in MPC NUMA is 
	#ifndef MPC_Common
	sctk_alloc_posix_mmsrc_numa_init_phase_numa();
	#endif
}

/************************* FUNCTION ************************/
/**
 * Return the local memory source depeding on the current NUMA node.
 * It will use numa_preferred() to get the current numa node.
**/
SCTK_STATIC struct sctk_alloc_mm_source* sctk_alloc_posix_get_local_mm_source(void)
{
	//vars
	struct sctk_alloc_mm_source * res;
	
	//get numa node
	#ifdef HAVE_LIBNUMA
	int node;
	//during init phase, allocator use the default memory source, so the default thread will not
	//be really numa aware.
	if (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_EGG || sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_DEFAULT)
		node = SCTK_DEFAULT_NUMA_MM_SOURCE_ID;
	else if (sctk_is_numa_node())
		node = sctk_alloc_init_on_numa_node();
	else
		node = SCTK_DEFAULT_NUMA_MM_SOURCE_ID;
	#else
	int node = 0;
	#endif

	//check and get
	SCTK_PDEBUG("Init allocation chain of thread on memory source %d (NUMA node %d)",node,node - 1);
	assert(sctk_global_base_init > SCTK_ALLOC_POSIX_INIT_NONE);
	assume(node <= SCTK_MAX_NUMA_NODE,"Node ID (%d) is larger than current SCTK_MAX_NUMA_NODE.");
	assert(node >= 0);
	res = (struct sctk_alloc_mm_source*)(sctk_global_memory_source[node]);
	assert(res != NULL);
	assume(res->request_memory != NULL,"The memory source to map to current new thread is no initialized.");

	//return
	return res;
}

/************************* FUNCTION ************************/
/**
 * Method used to setup the memory source (global for the process) at first allocation or at
 * initialisation step if available. This method is protected for exclusive access by an internal
 * mutex.
**/
void sctk_alloc_posix_base_init(void)
{
	/** @todo check for optimization **/
	static SCTK_ALLOC_INIT_LOCK_TYPE global_mm_mutex = SCTK_ALLOC_INIT_LOCK_INITIALIZER;
	#warning Maybe allocate this with mmap or sbrk
	static char buffer[SCTK_MACRO_BLOC_SIZE];

	//check to avoid taking the mutex if not necessary
	if (sctk_global_base_init >= SCTK_ALLOC_POSIX_INIT_DEFAULT)
		return;

	//critical initialization section
	SCTK_ALLOC_INIT_LOCK_LOCK(&global_mm_mutex);

	//check if not already init by previous thread
	if (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_NONE)
	{
		//debug
		SCTK_PDEBUG("Allocator init phase : Egg");

		//setup egg allocator to bootstrap thread allocation chains.
		/** @todo Maybe optimize by avoiding loading 2MB of virtual memory on egg allocator. **/
		sctk_alloc_chain_user_init(&sctk_global_egg_chain,buffer,SCTK_MACRO_BLOC_SIZE);

		#ifndef NDEBUG
		//init debug section
		sctk_alloc_debug_init();
		#endif

		//mark it as default chain to call for inti phase
		sctk_current_alloc_chain = &sctk_global_egg_chain;

		//marked egg as init
		sctk_global_base_init = SCTK_ALLOC_POSIX_INIT_EGG;

		//init topology system if have NUMA
		//on MPC it will be done latter after entering in MPC main()
		#ifndef MPC_Common
		#ifdef HAVE_LIBNUMA
		sctk_alloc_init_topology();
		#endif
		#endif

		//init global memory source
		sctk_alloc_posix_mmsrc_init();

		//can bind the memory source in egg allocator
		sctk_global_egg_chain.source = sctk_alloc_posix_get_local_mm_source();

		//unmark it
		sctk_current_alloc_chain = NULL;

		//marked as init
		sctk_global_base_init = SCTK_ALLOC_POSIX_INIT_DEFAULT;
	}

	//end of critical section
	SCTK_ALLOC_INIT_LOCK_UNLOCK(&global_mm_mutex);
}

/************************* FUNCTION ************************/
/**
 * Create a new thread local allocation chain.
**/
struct sctk_alloc_chain * sctk_alloc_posix_create_new_tls_chain(void)
{
	static int cnt = 0;
	cnt++;
	SCTK_PDEBUG("Create new alloc chain, total is %d",cnt);
	
	//start allocator base initialisation if not already done.
	sctk_alloc_posix_base_init();

	//allocate a new chain from egg allocator
	struct sctk_alloc_chain * chain = sctk_alloc_chain_alloc(&sctk_global_egg_chain,sizeof(struct sctk_alloc_chain));
	assert(chain != NULL);

	//init allocation chain.
	sctk_alloc_chain_user_init(chain,NULL,0);

	//bin to the adapted memory source depending on numa node availability
	chain->source = sctk_alloc_posix_get_local_mm_source();

	//mark as non shared
	chain->shared = false;

	//debug
	SCTK_PDEBUG("Init an allocation chain : %p",chain);

	/** @todo TODO register the allocation chain for debugging. **/
	//setup pointer for alocator memory dump in case of crash
	//#ifndef NDEBUG
	//TODO
	//sctk_alloc_chain_list[0] = &chain;
	//#endif
	return chain;
}

/************************* FUNCTION ************************/
/**
 * Setup the allocation chain for the current thread. It init an allocation chain and point it with
 * the TLS sctk_current_alloc_chain.
**/
struct sctk_alloc_chain * sctk_alloc_posix_setup_tls_chain(void)
{
	//check errors
	assert(sctk_current_alloc_chain == NULL);

	//create a new TLS chain
	struct sctk_alloc_chain * chain = sctk_alloc_posix_create_new_tls_chain();

	//make it default for current thread
	sctk_alloc_posix_set_default_chain(chain);

	//return it
	return chain;
}

/************************* FUNCTION ************************/
void * sctk_calloc (size_t nmemb, size_t size)
{
	void * ptr = malloc(nmemb * size);
	memset(ptr,0,nmemb * size);
	return ptr;
}

/************************* FUNCTION ************************/
void * sctk_malloc (size_t size)
{
	//to avoid many access to TLS variable
	struct sctk_alloc_chain * local_chain = sctk_current_alloc_chain;
	static int cnt =  0;
	void * res;

	//setup the local chain if not already done
	if (local_chain == NULL)
		local_chain = sctk_alloc_posix_setup_tls_chain();

	//purge the remote free queue
	sctk_alloc_chain_purge_rfq(local_chain);
	
	SCTK_PTRACE("//start alloc on chain %p -> %ld",local_chain,size);
	
	//to be compatible with glibc policy which didn't return NULL in this case.
	//otherwise we got crash in sed/grep/nano ...
	/** @todo Optimize by returning a specific fixed address instead of alloc size=1 **/
	if (size == 0)
		size = 1;

	//done allocation
	res = sctk_alloc_chain_alloc(local_chain,size);
	SCTK_PTRACE("void * ptr%p = malloc(%ld);//chain = %p",res,size,local_chain);
	return res;
}

/************************* FUNCTION ************************/
void * sctk_memalign(size_t boundary,size_t size)
{
	//to avoid many access to TLS variable
	struct sctk_alloc_chain * local_chain = sctk_current_alloc_chain;
	static int cnt =  0;
	void * res;

	//setup the local chain if not already done
	if (local_chain == NULL)
		local_chain = sctk_alloc_posix_setup_tls_chain();

	//purge the remote free queue
	sctk_alloc_chain_purge_rfq(local_chain);

	SCTK_PTRACE("//start memalign on chain %p -> %ld",local_chain,size);

	//to be compatible with glibc policy which didn't return NULL in this case.
	//otherwise we got crash in sed/grep/nano ...
	if (size == 0)
		size = 1;

	//done allocation
	res = sctk_alloc_chain_alloc_align(local_chain,boundary,size);
	SCTK_PTRACE("void * ptr%p = memalign(%ld,%ld);//chain = %p",res,boundary,size,local_chain);
	return res;
}

/************************* FUNCTION ************************/
void sctk_free (void * ptr)
{
	//to avoid many access to TLS variable
	struct sctk_alloc_chain * local_chain = sctk_current_alloc_chain;
	static int cnt = 0;

	//setup the local chain if not already done
	//we need a non null chain when spy is enabled to avoid crash on remote free before a first
	//call to malloc()
	#ifdef SCTK_ALLOC_SPY
	if (local_chain == NULL)
		local_chain = sctk_alloc_posix_setup_tls_chain();
	#endif

	//SCTK_PDEBUG("Free(%p);//%p",ptr,sctk_current_alloc_chain);

	//purge the remote free queue
	sctk_alloc_chain_purge_rfq(local_chain);
	
	//if NULL, nothing to do
	if (ptr == NULL)
		return;
	
	//Find the chain corresponding to the given memory bloc
	struct sctk_alloc_macro_bloc * macro_bloc = sctk_alloc_region_get_macro_bloc(ptr);
	if (macro_bloc == NULL)
	{
		#ifndef NDEBUG
		cnt++;
		sctk_alloc_pwarning("Don't free the block %p (cnt = %d).",ptr,cnt);
		abort();
		#endif
		return;
	}
	struct sctk_alloc_chain * chain = macro_bloc->chain;
	assume(chain != NULL,"Can't free a pointer not manage by an allocation chain from our allocator.");

	SCTK_PTRACE("free(ptr%p); //%p",ptr,chain);
	if (chain->shared || chain == local_chain)
	{
		//local free or protected free in shared allocation chain
		sctk_alloc_chain_free(chain,ptr);
	} else {
		//SCTK_PDEBUG("Register in RFQ of chain %p",chain);
		//remote free => simply register in free queue.
		SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_remote_free(chain,local_chain,ptr));
		sctk_alloc_rfq_register(&chain->rfq,ptr);
	}
}

/************************* FUNCTION ************************/
/**
 * Determine the old size of given bloc to be used in realloc.
 * @param ptr Define the pointer to test
 * @return Return the bloc size (body size, don't count the header), 0 if ptr is NULL of in case
 * of bad address.
**/
sctk_size_t sctk_alloc_posix_get_size(void *ptr)
{
	sctk_alloc_vchunk vchunk;
	if (ptr == NULL)
	{
		return 0;
	} else {
		vchunk = sctk_alloc_get_chunk((sctk_addr_t)ptr);
		return sctk_alloc_get_size(vchunk);
	}
}

/************************* FUNCTION ************************/
void * sctk_realloc (void * ptr, size_t size)
{
	sctk_size_t copy_size = size;
	void * res = NULL;

	#ifdef SCTK_ALLOC_SPY
	struct sctk_alloc_chain * local_chain = sctk_current_alloc_chain;
	if (local_chain == NULL)
		local_chain = sctk_alloc_posix_setup_tls_chain();
	#endif
	
	if (size != 0)
	{
		SCTK_ALLOC_SPY_HOOK(sctk_alloc_spy_emit_event_next_is_realloc(local_chain,ptr,size));
		res = malloc(size);
	}
	if (res != NULL && ptr != NULL)
	{
		copy_size = sctk_alloc_posix_get_size(ptr);
		if (size < copy_size)
			copy_size = size;
		memcpy(res,ptr,copy_size);
	}
	if (ptr != NULL)
		free(ptr);
	return res;
}

/************************* FUNCTION ************************/
struct sctk_alloc_chain * sctk_get_current_alloc_chain(void)
{
	return sctk_current_alloc_chain;
}
