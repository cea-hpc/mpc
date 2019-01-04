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
/* #   - Adam Julien julien.adam@cea.fr                                   # */
/* #                                                                      # */
/* ######################################################################## */

#if defined(MPC_PosixAllocator) || !defined(MPC_Common)

/************************** HEADERS ************************/
#if defined(_WIN32)
	#include <windows.h>
#else
	#include <sys/mman.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include "sctk_alloc_lock.h"
#include "sctk_alloc_debug.h"
#include "sctk_alloc_config.h"
#include "sctk_alloc_inlined.h"
#include "sctk_alloc_light_mm_source.h"
#include "sctk_alloc_posix.h"
#include "sctk_alloc_on_node.h"
#include "sctk_alloc_chain.h"
#include "sctk_alloc_region.h"
#include "sctk_alloc_hooks.h"

//optional headers
#ifdef HAVE_HWLOC
	#include "sctk_alloc_topology.h"
#endif

//optional header
#ifdef MPC_Common
#include "sctk.h"
#endif

//if have NUMA support
#ifdef HAVE_HWLOC
	#include <hwloc.h>
	// #include "../../../install/include/mpcmp.h"
	/** Select the NUMA memory source init function. **/
	#define sctk_alloc_posix_mmsrc_init sctk_alloc_posix_mmsrc_numa_init
#else //HAVE_HWLOC
	/** Select the UMA memory source init function. **/
	#define sctk_alloc_posix_mmsrc_init sctk_alloc_posix_mmsrc_uma_init
#endif //HAVE_HWLOC

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
#ifdef _WIN32
	static int sctk_current_alloc_chain = -1;
#else
	__thread struct sctk_alloc_chain * sctk_current_alloc_chain = NULL;
#endif

/************************* FUNCTION ************************/
#ifdef MPC_check_compatibility
/** Defined in MPC **/
int sctk_get_cpu();
/** Defined in MPC **/
int sctk_get_node_from_cpu (int cpu);
#endif

/*************************** FUNCTION **********************/
SCTK_PUBLIC void * sctk_malloc_hook(size_t size,__AL_UNUSED__ const void * caller)
{
	return sctk_malloc(size);
}

/*************************** FUNCTION **********************/
SCTK_PUBLIC void sctk_free_hook(void * ptr, __AL_UNUSED__ const void * caller)
{
	sctk_free(ptr);
}

/*************************** FUNCTION **********************/
SCTK_PUBLIC void * sctk_realloc_hook(void * ptr,size_t size,__AL_UNUSED__ const void * caller)
{
	return sctk_realloc(ptr,size);
}

/*************************** FUNCTION **********************/
SCTK_PUBLIC void * sctk_memalign_hook(size_t align,size_t size,__AL_UNUSED__ const void * caller)
{
	return sctk_memalign(align,size);
}

/*************************** FUNCTION **********************/
/**
 * Tls chain setter
 * @param chain Define the allocation chain to setup.
**/
void sctk_set_tls_chain(struct sctk_alloc_chain *chain) {
#ifdef _WIN32
  assume_m(sctk_current_alloc_chain != -1, "The TLS wasn't initialized.");
  TlsSetValue(sctk_current_alloc_chain, (LPVOID)chain);
#else
  sctk_current_alloc_chain = chain;
#endif
}

/*************************** FUNCTION **********************/
/**
 * Tls chain getter
**/
struct sctk_alloc_chain *sctk_get_tls_chain() {
#ifdef _WIN32
  assume_m(sctk_current_alloc_chain != -1, "The TLS wasn't initialized.");
  return (struct sctk_alloc_chain *)(TlsGetValue(sctk_current_alloc_chain));
#else
  return sctk_current_alloc_chain;
#endif
}

/*************************** FUNCTION **********************/
/**
 * Windows specifics. Tls variables need to be initialised before using.
**/
void sctk_alloc_tls_chain() {
#ifdef _WIN32
  assume_m(sctk_current_alloc_chain == -1,
           "Try to double alloc the allocator global TLS");
  sctk_current_alloc_chain = TlsAlloc();
  assume_m(sctk_current_alloc_chain != TLS_OUT_OF_INDEXES,
           "Not enougth indexes for TLS");
  sctk_set_tls_chain(NULL);
#endif
}

/*************************** FUNCTION **********************/
/**
 * Windows specifics. Tls variables need to be initialised before using.
**/
SCTK_INTERN void sctk_alloc_tls_chain_local_reset()
{
	#ifdef _WIN32
		sctk_set_tls_chain(NULL);
	#endif
}

/************************* FUNCTION ************************/
/**
 * Update the current thread local allocation chain.
 * @param chain Define the allocation chain to setup.
 * @return Return the old chain, if the user want to reset it after capturing some elements.
**/
SCTK_PUBLIC struct sctk_alloc_chain * sctk_alloc_posix_set_default_chain(struct sctk_alloc_chain * chain)
{
	//get old one
	struct sctk_alloc_chain * old_chain = sctk_get_tls_chain();
	//errors
	//assume_m(chain != NULL,"Can't set a default NULL allocation chain for local thread.");

	//setup allocation chain for current thread
	sctk_set_tls_chain(chain);

	//return the old one
	return old_chain;
}

/************************* FUNCTION ************************/
/** Init the global memory source for UMA architecture. **/
void sctk_alloc_posix_mmsrc_uma_init(void) {
  // error
  assume_m(sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_EGG,
           "Invalid init state while calling allocator default init phase.");

  sctk_global_memory_source[0] = sctk_alloc_chain_alloc(
      &sctk_global_egg_chain, sizeof(struct sctk_alloc_mm_source_light));
  // sctk_alloc_mm_source_default_init(sctk_global_memory_source[0],SCTK_ALLOC_HEAP_BASE,SCTK_ALLOC_HEAP_SIZE);
  sctk_alloc_mm_source_light_init(sctk_global_memory_source[0], 0,
                                  SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT);
}

/************************* FUNCTION ************************/
void sctk_alloc_posix_mmsrc_numa_init_node(int id) {
  // errors and debug
  assume_m(id <= SCTK_MAX_NUMA_NODE,
           "Caution, you get more node than supported by allocator. Limit is "
           "setup by SCTK_MAX_NUMA_NODE macro in sctk_alloc_common.h.");
  SCTK_NO_PDEBUG("Init memory source id = %d , MAX_NUMA_NODE = %d", id,
                 SCTK_MAX_NUMA_NODE);

  SCTK_NO_PDEBUG("Allocator init phase : Default");
  sctk_global_memory_source[id] = sctk_alloc_chain_alloc(
      &sctk_global_egg_chain, sizeof(struct sctk_alloc_mm_source_light));
  // sctk_alloc_mm_source_default_init(sctk_global_memory_source[id],SCTK_ALLOC_HEAP_BASE
  // + SCTK_ALLOC_HEAP_SIZE * id,SCTK_ALLOC_HEAP_SIZE);
  if (id == SCTK_DEFAULT_NUMA_MM_SOURCE_ID)
    sctk_alloc_mm_source_light_init(sctk_global_memory_source[id],
                                    SCTK_ALLOC_MM_SOURCE_LIGHT_NUMA_NODE_IGNORE,
                                    SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT);
  else
    sctk_alloc_mm_source_light_init(sctk_global_memory_source[id], id,
                                    SCTK_ALLOC_MM_SOURCE_LIGHT_DEFAULT);
}

/************************* FUNCTION ************************/
void sctk_alloc_posix_mmsrc_numa_init_phase_default(void) {
  // error
  assume_m(sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_EGG,
           "Invalid init state while calling allocator default init phase.");

  SCTK_NO_PDEBUG("Allocator init phase : NUMA");
  // init the default source ID of thead which are on unknown NUMA node or for
  // initial thread in MPC.
  sctk_alloc_posix_mmsrc_numa_init_node(SCTK_DEFAULT_NUMA_MM_SOURCE_ID);
}

/************************* FUNCTION ************************/
SCTK_INTERN void sctk_alloc_posix_mmsrc_numa_init_phase_numa(void)
{
	#ifdef HAVE_HWLOC
	//vars
	int nodes;
	int i;

	//error
	assume_m (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_DEFAULT,"Invalid init state while calling allocator NUMA init phase.");

	//get number of nodes
	SCTK_NO_PDEBUG("Init numa nodes");
	nodes = sctk_get_numa_node_number();
	assume_m(nodes <= SCTK_MAX_NUMA_NODE,"Caution, you get more node than supported by allocator. Limit is setup by SCTK_MAX_NUMA_NODE macro in sctk_alloc_posix.c.");

	//debug
	SCTK_NO_PDEBUG("Init with NUMA_NODES = %d , MAX_NUMA_NODE = %d",nodes,SCTK_MAX_NUMA_NODE);

	if (nodes <= 1)
	{
		sctk_global_memory_source[0] = sctk_global_memory_source[SCTK_DEFAULT_NUMA_MM_SOURCE_ID];
	} else {
		//init heaps for each NUMA node
		SCTK_NO_PDEBUG("Memory source size is %lu GB.",((SCTK_ALLOC_HEAP_SIZE)/1024ul/1024ul/1024ul));
		for ( i = 0 ; i < nodes ; ++i)
			sctk_alloc_posix_mmsrc_numa_init_node(i);
	}

	//setup malloc on node
	sctk_malloc_on_node_init(sctk_get_numa_node_number());
	#endif

	//mark NUMA init phase as done.
	sctk_global_base_init = SCTK_ALLOC_POSIX_INIT_NUMA;
}

/************************* FUNCTION ************************/
SCTK_INTERN void sctk_alloc_posix_plug_on_egg_allocator(void)
{
	sctk_alloc_posix_set_default_chain(&sctk_global_egg_chain);
}

/************************* FUNCTION ************************/
/**
 * Init the global memory sources for NUMA architecture.
 * It init one source for each available NUMA node.
**/
void sctk_alloc_posix_mmsrc_numa_init(void) {
  // setup default memory source
  sctk_alloc_posix_mmsrc_numa_init_phase_default();
}

/************************* FUNCTION ************************/
int sctk_alloc_posix_source_round_robin(void) {
  static sctk_alloc_spinlock_t lock;
  static int cnt = -1;
  int res;
  if (cnt == -1) {
    sctk_alloc_spinlock_init(&lock, PTHREAD_PROCESS_PRIVATE);
    cnt = 0;
  }

  sctk_alloc_spinlock_lock(&lock);
  res = cnt;
  cnt = (cnt + 1) % sctk_get_numa_node_number();
  sctk_alloc_spinlock_unlock(&lock);
  return res;
}

/************************* FUNCTION ************************/
/**
 * Return the local memory source depeding on the current NUMA node.
 * It will use numa_preferred() to get the current numa node.
**/
struct sctk_alloc_mm_source *
sctk_alloc_posix_get_local_mm_source(int force_default_numa_mm_source) {
  // vars
  struct sctk_alloc_mm_source *res;

// get numa node
#ifdef HAVE_HWLOC
  int node;
  // during init phase, allocator use the default memory source, so the default
  // thread will not
  // be really numa aware.
  if (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_EGG ||
      sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_DEFAULT)
    node = SCTK_DEFAULT_NUMA_MM_SOURCE_ID;
  else if (sctk_alloc_is_numa() && !force_default_numa_mm_source)
    node = sctk_alloc_init_on_numa_node();
  else
    node = SCTK_DEFAULT_NUMA_MM_SOURCE_ID;
#else
  int node = 0;
#endif

#if !defined(MPC_Common) && defined(HAVE_HWLOC)
  // use round robin on NUMA source if required, only out of MPC
  if ((node == -1 || SCTK_DEFAULT_NUMA_MM_SOURCE_ID) &&
      sctk_alloc_config()->numa_round_robin)
    node = sctk_alloc_posix_source_round_robin();
#endif // !defined(MPC_Common) && defined HAVE_HWLOC

  // check res
  if (node == -1)
    node = SCTK_DEFAULT_NUMA_MM_SOURCE_ID;

  // check and get
  SCTK_NO_PDEBUG(
      "Init allocation chain of thread on memory source %d (NUMA node %d)",
      node, node - 1);
  assert(sctk_global_base_init > SCTK_ALLOC_POSIX_INIT_NONE);
  assume_m(node <= SCTK_MAX_NUMA_NODE,
           "Node ID (%d) is larger than current SCTK_MAX_NUMA_NODE.");
  assert(node >= 0);
  res = (struct sctk_alloc_mm_source *)(sctk_global_memory_source[node]);
  assert(res != NULL);
  assume_m(res->request_memory != NULL,
           "The memory source to map to current new thread is no initialized.");

  // return
  return res;
}

/************************* FUNCTION ************************/
#ifdef ENABLE_GLIBC_ALLOC_HOOKS
SCTK_INTERN void sctk_alloc_posix_init_glibc_hooks(void)
{
	//setup hooks
	__malloc_hook = sctk_malloc_hook;
	__free_hook = sctk_free_hook;
	__realloc_hook = sctk_realloc_hook;
	__memalign_hook = sctk_memalign_hook;

	//call the libc handler
	if (__malloc_initialize_hook != NULL)
		__malloc_initialize_hook();
}
#endif //ENABLE_GLIBC_ALLOC_HOOKS

/************************* FUNCTION ************************/
/**
 * Method used to setup the memory source (global for the process) at first allocation or at
 * initialisation step if available. This method is protected for exclusive access by an internal
 * mutex.
**/
SCTK_INTERN void sctk_alloc_posix_base_init(void)
{
	static SCTK_ALLOC_INIT_LOCK_TYPE global_mm_mutex = SCTK_ALLOC_INIT_LOCK_INITIALIZER;
	static char buffer[SCTK_ALLOC_EGG_INIT_MEM];

	//check to avoid taking the mutex if not necessary
	if (sctk_global_base_init >= SCTK_ALLOC_POSIX_INIT_DEFAULT)
		return;

	//critical initialization section
	SCTK_ALLOC_INIT_LOCK_LOCK(&global_mm_mutex);

	//check if not already init by previous thread
	if (sctk_global_base_init == SCTK_ALLOC_POSIX_INIT_NONE)
	{
		//debug
		SCTK_NO_PDEBUG("Allocator init phase : Egg");

		//init glibc hooks
		#ifdef ENABLE_GLIBC_ALLOC_HOOKS
		sctk_alloc_posix_init_glibc_hooks();
		#endif //ENABLE_GLIBC_ALLOC_HOOKS

		//setup hooks if required
		#ifdef ENABLE_ALLOC_HOOKS
		sctk_alloc_hooks_init(&sctk_alloc_gbl_hooks);
		#endif //ENABLE_ALLOC_HOOKS

		//setup default values of config
		sctk_alloc_config_egg_init();

		//tls chain initialization
		sctk_alloc_tls_chain();
		//setup egg allocator to bootstrap thread allocation chains.
		sctk_alloc_chain_user_init(&sctk_global_egg_chain,buffer,SCTK_ALLOC_EGG_INIT_MEM,SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE);

		//set name
		sctk_global_egg_chain.name = "mpc_egg_allocator";

		#ifdef SCTK_ALLOC_DEBUG
		//init debug section
		sctk_alloc_debug_init();
		#endif

		//mark it as default chain to call for inti phase
		sctk_set_tls_chain(&sctk_global_egg_chain);

		//marked egg as init
		sctk_global_base_init = SCTK_ALLOC_POSIX_INIT_EGG;

		//init topology system if have NUMA
		//on MPC it will be done latter after entering in MPC main()
		#ifndef MPC_Common
		#ifdef HAVE_HWLOC
		sctk_alloc_init_topology();
		#endif
		#endif

		//init global memory source
		sctk_alloc_posix_mmsrc_init();

		//Now can load the user config values as we get egg_allocator to parse the file
		//like for phase_nunma we delay it in MPC to be done in main function, not before.
		#ifndef MPC_Common
		sctk_alloc_config_init();
		#endif //MPC_Common

		//can bind the memory source in egg allocator
		sctk_global_egg_chain.source = &sctk_global_memory_source[SCTK_DEFAULT_NUMA_MM_SOURCE_ID]->source;

		//marked as init
		sctk_global_base_init = SCTK_ALLOC_POSIX_INIT_DEFAULT;

		//if standelone, init NUMA now, in MPC it will be called later after init of hwloc and
		//restriction of nodeset.
		#ifndef MPC_Common
		sctk_alloc_posix_mmsrc_numa_init_phase_numa();
		#endif //MPC_Common

		//unmark egg_allocator from currant allocator
		sctk_set_tls_chain(NULL);
	}

	//end of critical section
	SCTK_ALLOC_INIT_LOCK_UNLOCK(&global_mm_mutex);
}

/************************* FUNCTION ************************/
/**
 * Create a new thread local allocation chain.
**/
SCTK_INTERN struct sctk_alloc_chain * sctk_alloc_posix_create_new_tls_chain(void)
{
	//vars
	struct sctk_alloc_chain * chain;
	static int cnt = 0;

	//disable valgrind here as we never free most the the blocs from here
	SCTK_ALLOC_MMCHECK_DISABLE_REPORT();

	cnt++;
	SCTK_NO_PDEBUG("Create new alloc chain, total is %d",cnt);

	//start allocator base initialisation if not already done.
	sctk_alloc_posix_base_init();

	//allocate a new chain from egg allocator
	chain = sctk_alloc_chain_alloc(&sctk_global_egg_chain,sizeof(struct sctk_alloc_chain));
	assert(chain != NULL);

	//init allocation chain, use default to mark as non shared
	sctk_alloc_chain_user_init(chain,NULL,0,SCTK_ALLOC_CHAIN_FLAGS_DEFAULT);
	chain->name = "mpc_posix_thread_allocator";

	//bin to the adapted memory source depending on numa node availability
	chain->source = sctk_alloc_posix_get_local_mm_source(true);

	//debug
	SCTK_NO_PDEBUG("Init an allocation chain : %p with mm_source = %p (node = %d)",chain,chain->source,sctk_alloc_chain_get_numa_node(chain));

	//reenable valgrind
	SCTK_ALLOC_MMCHECK_ENABLE_REPORT();
	return chain;
}

/************************* FUNCTION ************************/
/**
 * Setup the allocation chain for the current thread. It init an allocation chain and point it with
 * the TLS sctk_current_alloc_chain.
**/
SCTK_INTERN struct sctk_alloc_chain * sctk_alloc_posix_setup_tls_chain(void)
{
	//vars
	struct sctk_alloc_chain * chain;

	//debug
	SCTK_NO_PDEBUG("sctk_alloc_posix_setup_tls_chain()");

	//profiling
	SCTK_PROFIL_START(sctk_alloc_posix_setup_tls_chain);

	//check errors
	assert(sctk_get_tls_chain() == NULL);

	//temporaty use egg_allocator as default chain
	sctk_alloc_posix_set_default_chain(&sctk_global_egg_chain);

	//create a new TLS chain
	chain = sctk_alloc_posix_create_new_tls_chain();

	//make it default for current thread
	sctk_alloc_posix_set_default_chain(chain);

	SCTK_PROFIL_END(sctk_alloc_posix_setup_tls_chain);

	//return it
	return chain;
}

/************************* FUNCTION ************************/
SCTK_PUBLIC void * sctk_calloc (size_t nmemb, size_t size)
{
	void * ptr;
	SCTK_PROFIL_START(sctk_calloc);
	ptr = sctk_malloc(nmemb * size);
	memset(ptr,0,nmemb * size);
	SCTK_PROFIL_END(sctk_calloc);
	return ptr;
}

/************************* FUNCTION ************************/
SCTK_PUBLIC void * sctk_malloc (size_t size)
{

	//vars
	struct sctk_alloc_chain * local_chain;
	void * res;

	//profile
	SCTK_PROFIL_START(sctk_malloc);

	//get TLS
	local_chain = sctk_get_tls_chain();

	//setup the local chain if not already done
	if (local_chain == NULL)
		local_chain = sctk_alloc_posix_setup_tls_chain();

	//call hook if required
	#ifdef ENABLE_GLIBC_ALLOC_HOOKS
	if (__malloc_hook != sctk_malloc_hook)
		return __malloc_hook(size,sctk_malloc);
	#endif //ENABLE_GLIBC_ALLOC_HOOKS

	//purge the remote free queue
	sctk_alloc_chain_purge_rfq(local_chain);

	SCTK_PTRACE("//start alloc on chain %p -> %ld",local_chain,size);

	//to be compatible with glibc policy which didn't return NULL in this case.
	//otherwise we got crash in sed/grep/nano ...
	/**
	 * @todo Optimize by returning a specific fixed address instead of alloc size=1 *
	 * but need to check in spec if it was correct.
	**/
	if (size == 0)
		size = 1;

	//done allocation
	res = sctk_alloc_chain_alloc(local_chain,size);
	SCTK_PTRACE("void * ptr%p = malloc(%ld);//chain = %p",res,size,local_chain);
	SCTK_PROFIL_END(sctk_malloc);
	return res;
}

/************************* FUNCTION ************************/
SCTK_PUBLIC void * sctk_memalign(size_t boundary,size_t size)
{
	//vars
	struct sctk_alloc_chain * local_chain;
	void * res;

	//profile
	SCTK_PROFIL_START(sctk_memalign);

	//get TLS
	local_chain = sctk_get_tls_chain();

	//setup the local chain if not already done
	if (local_chain == NULL)
		local_chain = sctk_alloc_posix_setup_tls_chain();

	//call hook if required
	#ifdef ENABLE_GLIBC_ALLOC_HOOKS
	if (__memalign_hook != sctk_memalign_hook)
		return __memalign_hook(boundary,size,sctk_memalign);
	#endif //ENABLE_GLIBC_ALLOC_HOOKS

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
	SCTK_PROFIL_END(sctk_memalign);
	return res;
}

/************************* FUNCTION ************************/
SCTK_PUBLIC int sctk_posix_memalign(void **memptr, size_t boundary, size_t size)
{
	SCTK_PROFIL_START(sctk_posix_memalign);
	assert(memptr != NULL);
	*memptr = sctk_memalign(boundary,size);
	SCTK_PROFIL_END(sctk_posix_memalign);
	if (memptr == NULL)
		return ENOMEM;
	else
		return 0;
}

/************************* FUNCTION ************************/
SCTK_PUBLIC void sctk_free (void * ptr)
{
	//vars
	struct sctk_alloc_chain * local_chain = NULL;
	struct sctk_alloc_macro_bloc * macro_bloc = NULL;
	struct sctk_alloc_chain * chain = NULL;

	#ifdef SCTK_ALLOC_DEBUG
	static int cnt = 0;
	#endif

	SCTK_PROFIL_START(sctk_free);
	local_chain = sctk_get_tls_chain();

	//setup the local chain if not already done
	//we need a non null chain when spy is enabled to avoid crash on remote free before a first
	//call to malloc()
	if (local_chain == NULL)
		local_chain = sctk_alloc_posix_setup_tls_chain();

	//call hook if required
	#ifdef ENABLE_GLIBC_ALLOC_HOOKS
	if (__free_hook != sctk_free_hook)
		return __free_hook(ptr,sctk_free);
	#endif //ENABLE_GLIBC_ALLOC_HOOKS

	//purge the remote free queue
	sctk_alloc_chain_purge_rfq(local_chain);

	//if NULL, nothing to do
	if (ptr == NULL)
		return;

	//Find the chain corresponding to the given memory bloc
	macro_bloc = sctk_alloc_region_get_macro_bloc(ptr);
	if (macro_bloc == NULL)
	{
		if ( sctk_alloc_config()->strict )
		{
			sctk_fatal("Allocator error on free(%p), the address you provide is not managed by this memory allocator.",ptr);
		} else {
			#ifdef _WIN32
				//used to fallback on default HeapAlloc/HeapFree/HeapRealloc/HeapSize when getting some legacy
				//segments not managed by our allocator It append typically with atexit method which init is segment
				//before allocator overriding.
				OutputDebugStringA("Try to free segment managed by default MSVCR, transmit to it\n");
				HeapFree((HANDLE)_get_heap_handle(),0,ptr);
			#else
				#ifdef SCTK_ALLOC_DEBUG
					cnt++;
					sctk_alloc_pwarning("Don't free the block %p (cnt = %d).",ptr,cnt);
				#endif//SCTK_ALLOC_DEBUG
			#endif//_WIN32
			return;
		}
	} else {
		assert(ptr > (void*)macro_bloc && (sctk_addr_t)ptr < (sctk_addr_t)macro_bloc + sctk_alloc_get_chunk_header_large_size(&macro_bloc->header));
	}

	chain = macro_bloc->chain;
	assume_m(chain != NULL,"Can't free a pointer not manage by an allocation chain from our allocator.");

	SCTK_PTRACE("free(ptr%p); //%p",ptr,chain);
	if ((chain->flags & SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE) || chain == local_chain)
	{
		//local free or protected free in shared allocation chain
		sctk_alloc_chain_free(chain,ptr);
	} else {
		SCTK_NO_PDEBUG("Register in RFQ of chain %p",chain);
		//remote free => simply register in free queue.
		SCTK_ALLOC_HOOK(chain_remote_free,chain,local_chain,ptr);
		sctk_alloc_rfq_register(&chain->rfq,ptr);
	}
	SCTK_PROFIL_END(sctk_free);
}

/************************* FUNCTION ************************/
/**
 * Determine the old size of given bloc to be used in realloc.
 * @param ptr Define the pointer to test
 * @return Return the bloc size (body size, don't count the header), 0 if ptr is NULL of in case
 * of bad address.
**/
SCTK_INTERN sctk_size_t sctk_alloc_posix_get_size(void *ptr)
{
	sctk_alloc_vchunk vchunk;
	if (ptr == NULL)
	{
		return 0;
	} else {
		vchunk = sctk_alloc_get_chunk_unpadded((sctk_addr_t)ptr);
		//ptr - vchunk + 1 to take in accound padding if present.
		return sctk_alloc_get_usable_size(vchunk) - ((sctk_addr_t)ptr - (sctk_addr_t)vchunk) + 1;
	}
}

/************************* FUNCTION ************************/
#ifdef _WIN32
SCTK_PUBLIC size_t sctk_alloc_posix_get_size_win(void *ptr)
{
	//vars
	size_t retval;
	struct sctk_alloc_macro_bloc * macro_bloc;
	HANDLE handle;

	//trivial case
	if (ptr == NULL)
		return EINVAL;

	//try to find the region to known if it's our or a legacy from standard HeapAlloc
	macro_bloc = sctk_alloc_region_get_macro_bloc(ptr);
	if (macro_bloc != NULL)
	{
		retval = sctk_alloc_posix_get_size(ptr);
	} else {
		//used to fallback on default HeapAlloc/HeapFree/HeapRealloc/HeapSize when getting some legacy
		//segments not managed by our allocator It append typically with atexit method which init is segment
		//before allocator overriding.
		handle = (HANDLE)_get_heap_handle();
		retval = (size_t)HeapSize(handle, 0, ptr);
	}

	return retval;
}
#endif //_WIN32

/************************* FUNCTION ************************/
SCTK_PUBLIC void * sctk_realloc (void * ptr, size_t size)
{
	struct sctk_alloc_chain * local_chain = NULL;
	struct sctk_alloc_chain * chain = NULL;
	struct sctk_alloc_macro_bloc * macro_bloc = NULL;
	void * res = NULL;

	SCTK_PROFIL_START(sctk_realloc);

	//trivial cases
	if (ptr == NULL)
		return sctk_malloc(size);
	if (size == 0)
	{
		sctk_free(ptr);
		return NULL;
	}

	//get the current chain
	local_chain = sctk_get_tls_chain();
	if (local_chain == NULL)
		local_chain = sctk_alloc_posix_setup_tls_chain();

	#ifdef ENABLE_GLIBC_ALLOC_HOOKS
	//call hook if required
	if (__realloc_hook != sctk_realloc_hook)
		return __realloc_hook(ptr,size,sctk_realloc);
	#endif //ENABLE_GLIBC_ALLOC_HOOKS

	//get the chain of the chunk
	macro_bloc = sctk_alloc_region_get_macro_bloc(ptr);
	if (macro_bloc != NULL)
		chain = macro_bloc->chain;

	//on windows we may got some segments allocated by HeapAlloc, detect them and fallback to HeapRealloc for them
	#ifdef _WIN32
	if (chain == NULL)
	{
		//used to fallback on default HeapAlloc/HeapFree/HeapRealloc/HeapSize when getting some legacy
		//segments not managed by our allocator It append typically with atexit method which init is segment
		//before allocator overriding.
		OutputDebugStringA("Try to realloc segments from MSVCRT, transmit to it\n");
		return HeapReAlloc((HANDLE)_get_heap_handle(),0,ptr,size);
	}
	#endif //_WIN32

	//error handling
	assume_m (chain != NULL,"Allocator error on realloc(%p,%llu), the address you provide is not managed by this memory allocator.",ptr,size);

	if (chain == local_chain || sctk_alloc_chain_is_thread_safe(chain))
	{
		//if distant chain is thread safe, we can try to use chain_realloc which
		//use more optimized memory reuse if possible. If local its also good.
		res = sctk_alloc_chain_realloc(chain,ptr,size);
	#ifdef HAVE_MREMAP
	} else if (SCTK_ALLOC_HUGE_CHUNK_SEGREGATION && size > SCTK_HUGE_BLOC_LIMIT && sctk_alloc_chain_can_remap(chain) && sctk_alloc_posix_get_size(ptr) > SCTK_HUGE_BLOC_LIMIT) {
		//for huge segment, we can try to remap and update register field in region, it may not break
		//thead-safety as in this case we didn't need to take locks on the chain
		//remap will also not break current NUMA mapping (hope).
		res = sctk_alloc_chain_realloc(chain,ptr,size);
	#endif //HAVE_MREMAP
	} else {
		//for other cases we cannot do anything due to thread-safety, so need to alloc/copy/free
		res = sctk_realloc_inter_chain(ptr,size);
	}

	SCTK_PTRACE("//ptr%p = realloc(ptr%p,%llu); //%p",res,ptr,size);
	SCTK_PROFIL_END(sctk_realloc);
	return res;
}

/************************* FUNCTION ************************/
void *sctk_realloc_inter_chain(void *ptr, size_t size) {
  sctk_size_t copy_size = size;
  void *res = NULL;

  SCTK_PROFIL_START(sctk_realloc_inter_chain);

  // when using hooking, we need to know the chain
  if (SCTK_ALLOC_HAS_HOOK(chain_next_is_realloc)) {
    struct sctk_alloc_chain *local_chain = sctk_get_tls_chain();
    if (local_chain == NULL)
      local_chain = sctk_alloc_posix_setup_tls_chain();
    SCTK_ALLOC_HOOK(chain_next_is_realloc, local_chain, ptr, size);
  }

  if (size != 0)
    res = sctk_malloc(size);

  if (res != NULL && ptr != NULL) {
    copy_size = sctk_alloc_posix_get_size(ptr);
    if (size < copy_size)
      copy_size = size;
    memcpy(res, ptr, copy_size);
  }

  if (ptr != NULL)
    sctk_free(ptr);

  SCTK_PROFIL_END(sctk_realloc_inter_chain);

  return res;
}

/************************* FUNCTION ************************/
SCTK_INTERN struct sctk_alloc_chain * sctk_get_current_alloc_chain(void)
{
	return sctk_get_tls_chain();
}

/************************* FUNCTION ************************/
/**
 * Reconfigure the allocator binding on memory source, mainly to adapt after a NUMA migration.
**/
SCTK_PUBLIC void sctk_alloc_posix_numa_migrate(void)
{
	//vars
	struct sctk_alloc_chain * local_chain;

	//get the current allocation chain
	local_chain = sctk_get_tls_chain();

	//if we didn't have an allocation, we can skip this, it will be
	//create at first use on the current numa node, so automatically OK
	if (local_chain == NULL)
	{
		SCTK_NO_PDEBUG("Not allocation to migrate.");
		return;
	}

	//move the chain
	sctk_alloc_posix_numa_migrate_chain(local_chain);
}

/************************* FUNCTION ************************/
/**
 * Migrate the memory of a given allocation chain to current NUMA node.
 * CAUTION, it suppose that you provide a posix allocation chain, this function is not build
 * to support generic once.
 * @param chain Define the memory chain to migrate.
**/
SCTK_INTERN void sctk_alloc_posix_numa_migrate_chain(struct sctk_alloc_chain * chain)
{
	//vars
	struct sctk_alloc_mm_source * old_source;
	struct sctk_alloc_mm_source * new_source;
	struct sctk_alloc_mm_source_light * light_source;
	int old_numa_node = -1;
	int new_numa_node = -1;

	SCTK_PROFIL_START(sctk_alloc_posix_numa_migrate);

	#ifdef MPC_Theads
	SCTK_NO_PDEBUG("Migration on %d",sctk_get_cpu());
	#endif

	//if NULL nothing to do otherwise remind the current mm source
	if (chain == NULL)
		return;
	else
		old_source = chain->source;

	//re-setup the memory source.
	new_source = sctk_alloc_posix_get_local_mm_source(false);

	//get numa node of sources
	light_source = sctk_alloc_get_mm_source_light(old_source);
	if (light_source != NULL)
		old_numa_node = light_source->numa_node;
	light_source = sctk_alloc_get_mm_source_light(new_source);
	if (light_source != NULL)
		new_numa_node = light_source->numa_node;

	SCTK_NO_PDEBUG("Request NUMA migration to thread allocator (%p) %d -> %d",chain,old_numa_node,new_numa_node);

	//check if need to migrate NUMA explicitely
	if (old_numa_node != new_numa_node && new_numa_node != SCTK_DEFAULT_NUMA_MM_SOURCE_ID)
		sctk_alloc_chain_numa_migrate(chain,new_numa_node,true,true,new_source);

	SCTK_PROFIL_END(sctk_alloc_posix_numa_migrate);
}

/************************* FUNCTION ************************/
SCTK_PUBLIC void sctk_alloc_posix_chain_print_stat(void)
{
	struct sctk_alloc_chain * local_chain = sctk_get_tls_chain();
	if (local_chain == NULL)
		warning("The current thread hasn't setup his allocation up to now.");
	else
		sctk_alloc_chain_print_stat(local_chain);
}

/************************* FUNCTION ************************/
SCTK_INTERN void sctk_alloc_posix_destroy_handler(struct sctk_alloc_chain * chain)
{
	//errors
	assert(chain != &sctk_global_egg_chain);

	//free the memory used by the chain struct.
	sctk_alloc_chain_free(&sctk_global_egg_chain,chain);
}

/************************* FUNCTION ************************/
SCTK_INTERN void sctk_alloc_posix_mark_current_for_destroy(void)
{
	//vars
	struct sctk_alloc_chain * local_chain;

	//get current allocation chain
	local_chain = sctk_get_tls_chain();

	//nothing to do
	if (local_chain == NULL)
		return;

	//replace default by egg_allocator to avoid recreating another one if some steps follow in thread destroy
	sctk_alloc_posix_set_default_chain(&sctk_global_egg_chain);

	//mark the current chain for destroy
	sctk_alloc_chain_mark_for_destroy(local_chain,sctk_alloc_posix_destroy_handler);
}

#endif //define(MPC_PosixAllocator) || !defined(MPC_Common)
