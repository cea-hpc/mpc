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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

//need some fixes
#if 0
#ifndef MPC_ALLOC_PUBLIC_API_H
#define MPC_ALLOC_PUBLIC_API_H

/********************************* INCLUDES *********************************/
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

/********************************** TYPES ***********************************/
struct sctk_alloc_chain;
typedef struct sctk_alloc_chain sctk_alloc_chain_t;

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
	 * blocs allocated by this allocation chain. But you do not have the minimum size of 2M for
	 * macro blocs which is required by region representation.
	**/
	SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER = 4,
	/** Flag to create a simple standalone allocation chain. **/
	SCTK_ALLOC_CHAIN_STANDALONE = SCTK_ALLOC_CHAIN_DISABLE_REGION_REGISTER | SCTK_ALLOC_CHAIN_FLAGS_THREAD_SAFE
};

/********************************* FUNCTION *********************************/
//chain life cycle
void sctk_alloc_chain_user_init(struct sctk_alloc_chain * chain,void * buffer,sctk_size_t size,enum sctk_alloc_chain_flags flags);
void sctk_alloc_chain_standalone_init(struct sctk_alloc_chain * chain,void * buffer,sctk_size_t size);
struct sctk_alloc_chain * sctk_alloc_chain_shared_init(void * buffer, sctk_size_t size);
void sctk_alloc_chain_destroy(struct sctk_alloc_chain * chain,bool force);
void sctk_alloc_chain_mark_for_destroy(struct sctk_alloc_chain * chain,void (*destroy_handler)(struct sctk_alloc_chain * chain));
//chain options
bool sctk_alloc_chain_is_thread_safe(struct sctk_alloc_chain * chain);
void sctk_alloc_chain_make_thread_safe(struct sctk_alloc_chain * chain,bool value);
int sctk_alloc_chain_get_numa_node(struct sctk_alloc_chain * chain);
size_t sctk_alloc_chain_struct_size(void);
//chain usage
void * sctk_alloc_chain_alloc(struct sctk_alloc_chain * chain,sctk_size_t size);
void * sctk_alloc_chain_alloc_align(struct sctk_alloc_chain * chain,sctk_size_t boundary,sctk_size_t size);
void sctk_alloc_chain_free(struct sctk_alloc_chain * chain,void * ptr);
void * sctk_alloc_chain_realloc(struct sctk_alloc_chain * chain, void * ptr, sctk_size_t size);
void sctk_alloc_chain_remote_free(struct sctk_alloc_chain * chain,void * ptr);
void sctk_alloc_chain_purge_rfq(struct sctk_alloc_chain * chain);
void sctk_alloc_chain_numa_migrate(struct sctk_alloc_chain * chain, int target_numa_node,bool migrate_chain_struct,bool migrate_content,struct sctk_alloc_mm_source * new_mm_source);
void sctk_alloc_chain_get_numa_stat(struct sctk_alloc_numa_stat_s * numa_stat,struct sctk_alloc_chain * chain);
void sctk_alloc_chain_print_stat(struct sctk_alloc_chain * chain);
void sctk_alloc_chain_user_refill(struct sctk_alloc_chain * chain, void * buffer, sctk_size_t size);

/********************************* FUNCTION *********************************/
//posix allocator functions
void sctk_alloc_posix_numa_migrate(void);
void sctk_alloc_posix_chain_print_stat(void);
struct sctk_alloc_chain * sctk_alloc_posix_set_default_chain(struct sctk_alloc_chain * chain);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //MPC_ALLOC_PUBLIC_API_H
#endif
