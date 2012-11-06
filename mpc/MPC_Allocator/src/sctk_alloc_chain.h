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

#ifndef SCTK_ALLOC_CHAIN_H
#define SCTK_ALLOC_CHAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES *********************************/
#include "sctk_alloc_common.h"
#include "sctk_alloc_lock.h"
#include "sctk_alloc_thread_pool.h"
#include "sctk_alloc_rfq.h"
//TODO find a way to remove this include from here
#include "sctk_alloc_spy.h"
#include "sctk_alloc_stats.h"

/********************************** ENUM ************************************/
/** List of flags to enable options of an allocation chain. **/
enum sctk_alloc_chain_flags
{
	/** Default options. By default it is thread safe. **/
	SCTK_ALLOC_CHAIN_FLAGS_DEFAULT = 0,
	/** Enable the locks in the chain to use it from multiple threads. **/
	SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE = 1,
	/** Disable chunk merge at free time. **/
	SCTK_ALLOC_CHAIN_DISABLE_MERGE = 2,
	/**
	 * Disable global region registry. Caution, in that case, you can't use free/delete on memory
	 * blocs allocated by this allocation chain. But you do not have the minimam size of 2M for
	 * macro blocs which is required by region representation.
	**/
	SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER = 4
};

/******************************** STRUCTURE *********************************/
struct sctk_alloc_numa_stat_s;

/******************************** STRUCTURE *********************************/
struct sctk_alloc_chain_stat
{
	sctk_size_t min_free_size;
	sctk_size_t max_free_size;
	sctk_size_t nb_free_chunks;
	sctk_size_t nb_macro_blocs;
	sctk_size_t cached_free_memory;
};

/******************************** STRUCTURE *********************************/
/**
 * The allocation permit to link a memory source to a thread pool. It can represent a user specific
 * allocation chain or a default thread allocation chain. This will be selected by a TLS variable
 * managed by allocation methods exposed to the user.
**/
struct sctk_alloc_chain
{
	struct sctk_thread_pool pool;
	struct sctk_alloc_mm_source * source;
	/** Can give a name to the memory source to help debugging. By default "Unknown". **/
	const char * name;
	void * base_addr;
	void * end_addr;
	/** Flags to enable some options of the allocation chain. **/
	enum sctk_alloc_chain_flags flags;
	/** Spinlock used to protect the thread pool if it was shared, ignored otherwire. **/
	sctk_alloc_spinlock_t lock;
	/** Remote free queue **/
	struct sctk_alloc_rfq rfq;
	/**
	 * If not NULL, function to call when empty for final destruction.
	 * This is more to be used internally for thread destruction.
	**/
	void (*destroy_handler)(struct sctk_alloc_chain * chain);
	/**
	 * Count the total number of macro bloc currently managed by the chain.
	 * It serve to known when to call the destroy_handler method.
	**/
	int cnt_macro_blocs;
	/** Stats of the given allocation chain. **/
	#ifndef SCTK_ALLOC_STATS
		SCTK_ALLOC_STATS_HOOK(struct sctk_alloc_stats_chain stats);
	#endif

	/** Struct specific for allocation chain spying. **/
	#ifdef SCTK_ALLOC_SPY
		SCTK_ALLOC_SPY_HOOK(struct sctk_alloc_spy_chain spy);
	#endif
};

/********************************* FUNCTION *********************************/
//allocation chain management
SCTK_STATIC void sctk_alloc_chain_base_init(struct sctk_alloc_chain * chain,enum sctk_alloc_chain_flags flags);
void sctk_alloc_chain_user_init(struct sctk_alloc_chain * chain,void * buffer,sctk_size_t size,enum sctk_alloc_chain_flags flags);
void sctk_alloc_chain_default_init(struct sctk_alloc_chain * chain,struct sctk_alloc_mm_source * source,enum sctk_alloc_chain_flags flags);
void * sctk_alloc_chain_alloc(struct sctk_alloc_chain * chain,sctk_size_t size);
void * sctk_alloc_chain_alloc_align(struct sctk_alloc_chain * chain,sctk_size_t boundary,sctk_size_t size);
void sctk_alloc_chain_free(struct sctk_alloc_chain * chain,void * ptr);
SCTK_STATIC bool sctk_alloc_chain_can_destroy(struct sctk_alloc_chain* chain);
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_chain_request_mem(struct sctk_alloc_chain* chain,sctk_size_t size);
SCTK_STATIC bool sctk_alloc_chain_refill_mem(struct sctk_alloc_chain* chain,sctk_size_t size);
void sctk_alloc_chain_destroy(struct sctk_alloc_chain * chain,bool force);
void sctk_alloc_chain_purge_rfq(struct sctk_alloc_chain * chain);
SCTK_STATIC void sctk_alloc_chain_free_macro_bloc(struct sctk_alloc_chain * chain,sctk_alloc_vchunk vchunk);
SCTK_STATIC bool sctk_alloc_chain_can_remap(struct sctk_alloc_chain * chain);
void * sctk_alloc_chain_realloc(struct sctk_alloc_chain * chain, void * ptr, sctk_size_t size);
void sctk_alloc_chain_numa_migrate(struct sctk_alloc_chain * chain, int target_numa_node,bool migrate_chain_struct,bool migrate_content,struct sctk_alloc_mm_source * new_mm_source);
bool sctk_alloc_chain_is_thread_safe(struct sctk_alloc_chain * chain);
void sctk_alloc_chain_make_thread_safe(struct sctk_alloc_chain * chain,bool value);
void sctk_alloc_chain_mark_for_destroy(struct sctk_alloc_chain * chain,void (*destroy_handler)(struct sctk_alloc_chain * chain));
SCTK_STATIC sctk_alloc_vchunk sctk_alloc_chain_prepare_and_reg_macro_bloc(struct sctk_alloc_chain * chaine,struct sctk_alloc_macro_bloc * macro_bloc);

/************************* FUNCTION ************************/
//some stat function for debug
void sctk_alloc_chain_get_numa_stat(struct sctk_alloc_numa_stat_s * numa_stat,struct sctk_alloc_chain * chain);
void sctk_alloc_chain_print_stat(struct sctk_alloc_chain * chain);
int sctk_alloc_chain_get_numa_node(struct sctk_alloc_chain * chain);

#ifdef __cplusplus
}
#endif

#endif /* SCTK_ALLOC_CHAIN_H_ */
