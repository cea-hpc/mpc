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

#ifndef SCTK_ALLOC_THREAD_POOL_H
#define SCTK_ALLOC_THREAD_POOL_H

#ifdef __cplusplus
extern "C"
{
#endif

/********************************* INCLUDES *********************************/
#include "sctk_alloc_common.h"
#include "sctk_alloc_chunk.h"

/********************************** GLOBALS *********************************/
/** Size class for the free lists. **/
extern const sctk_ssize_t SCTK_ALLOC_FREE_SIZES[SCTK_ALLOC_NB_FREE_LIST];

/********************************** ENUM ************************************/
/** Define how to insert a given bloc in free list, at the end or begening. **/
enum sctk_alloc_insert_mode
{
	SCTK_ALLOC_INSERT_AT_END,
	SCTK_ALLOC_INSERT_AT_START
};

/******************************** STRUCTURE *********************************/
/**
 * A thread pool is created for each active threads and will be pluged on top of a common memory
 * source. It manage the free small blocs ans exchange with the memory source by "pages". Each
 * "page" is splited to get the meomry chunk allocated by the user.
 *
 * A thread pool can also be used to generated a user managed allocator chain by providing a static
 * memory source based on a specific segement allocated by the user. We only provide the chunk
 * management to avoid the user to handle bloc lists.... which is the work of an allocator.
**/
struct sctk_thread_pool
{
	/**
	 * Lists of free element managed by the thread pool. All elements are place in specific lists
	 * depending on their size to be quicly find (best fit method).
	**/
	struct sctk_alloc_free_chunk free_lists[SCTK_ALLOC_NB_FREE_LIST];
	/**
	 * Store free list empty status as a bitmap, it permit a quicker search for allocation when
	 * we got only a large bloc in the last one which is the default at start time of just after
	 * memory refilling.
	**/
	bool free_list_status[SCTK_ALLOC_NB_FREE_LIST];
	/** If you free list size follow an analytic low, you can provide a method to reverse it is more optimized way. **/
	int (*reverse_analytic_free_size)(sctk_size_t size,const sctk_ssize_t * size_list);
	/**
	 * Define the size of each class of free blocs, must be terminated by -1.
	 * and must must contain at least SCTK_ALLOC_NB_FREE_LIST elements.
	**/
	const sctk_ssize_t * alloc_free_sizes;
	/** Define the number of entries in free lists (must be lower than SCTK_ALLOC_NB_FREE_LIST). **/
	short int nb_free_lists;
};

/********************************* FUNCTION *********************************/
//thread pool management
void sctk_alloc_thread_pool_init(
    struct sctk_thread_pool *pool,
    const sctk_ssize_t alloc_free_sizes[SCTK_ALLOC_NB_FREE_LIST]);
sctk_alloc_free_list_t *sctk_alloc_get_free_list(struct sctk_thread_pool *pool,
                                                 sctk_size_t size);
sctk_size_t sctk_alloc_get_list_class(struct sctk_thread_pool *pool,
                                      sctk_alloc_free_list_t *list);
void sctk_alloc_free_list_insert_raw(struct sctk_thread_pool *pool, void *ptr,
                                     sctk_size_t size, void *prev);
void sctk_alloc_free_list_insert(
    struct sctk_thread_pool *pool,
    struct sctk_alloc_chunk_header_large *chunk_large,
    enum sctk_alloc_insert_mode insert_mode);
void sctk_alloc_free_list_remove(struct sctk_thread_pool *pool,
                                 struct sctk_alloc_free_chunk *fchunk);
struct sctk_alloc_free_chunk *
sctk_alloc_find_adapted_free_chunk(sctk_alloc_free_list_t *list,
                                   sctk_ssize_t size);
struct sctk_alloc_free_chunk *
sctk_alloc_find_free_chunk(struct sctk_thread_pool *pool, sctk_size_t size);
sctk_alloc_free_list_t *
sctk_alloc_get_next_list(const struct sctk_thread_pool *pool,
                         sctk_alloc_free_list_t *list);
bool sctk_alloc_free_list_empty(const sctk_alloc_free_list_t *list);
sctk_alloc_vchunk sctk_alloc_merge_chunk(struct sctk_thread_pool *pool,
                                         sctk_alloc_vchunk chunk,
                                         sctk_alloc_vchunk first_page_chunk,
                                         sctk_addr_t max_address);
void sctk_alloc_free_chunk_range(struct sctk_thread_pool *pool,
                                 sctk_alloc_vchunk first,
                                 sctk_alloc_vchunk last);
sctk_alloc_vchunk sctk_alloc_split_free_bloc(sctk_alloc_vchunk *chunk,
                                             sctk_size_t size);
sctk_alloc_vchunk
sctk_alloc_free_chunk_to_vchunk(struct sctk_alloc_free_chunk *chunk);
sctk_alloc_free_list_t *
sctk_alloc_find_first_free_non_empty_list(struct sctk_thread_pool *pool,
                                          sctk_alloc_free_list_t *list);
void sctk_alloc_free_list_mark_empty(struct sctk_thread_pool *pool,
                                     sctk_alloc_free_list_t *list);
void sctk_alloc_free_list_mark_non_empty(struct sctk_thread_pool *pool,
                                         sctk_alloc_free_list_t *list);
bool sctk_alloc_free_list_is_not_empty_quick(struct sctk_thread_pool *pool,
                                             sctk_alloc_free_list_t *list);
extern int sctk_alloc_reverse_analytic_free_size(sctk_size_t size,
                                                 const sctk_ssize_t *size_list);

#ifdef __cplusplus
}
#endif

#endif /* SCTK_ALLOC_THREAD_POOL_H */
