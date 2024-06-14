/* ############################ MALP License ############################## */
/* # Fri Jan 18 14:00:00 CET 2013                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */

/**
 * @addtogroup internal_FIFO_
 * @{
 */

#ifndef BUFFERED_FIFO_H
#define BUFFERED_FIFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mpc_common_datastructure.h>

/**
 * @brief Structure defining a chunk of FIFO.
 * The mpc_common_fifo is made of several mpc_common_fifo_chunk containing the data.
 *
 * This is useful to avoid allocating/deallocating at each push/pop.
 */
struct mpc_common_fifo_chunk
{
	mpc_common_spinlock_t lock;		  /**< @brief a lock for concurrent internal manipulations */
	char *payload;					  /**< @brief the actual data */
	uint64_t chunk_size;			  /**< @brief the size of the current chunk */
	size_t elem_size;				  /**< @brief the size of stored elements */
	uint64_t start_offset;			  /**< @brief the start offset (position of oldest element in payload */
	uint64_t end_offset;			  /**< @brief the end offset (position of the end of newest element in payload */
	struct mpc_common_fifo_chunk *prev; /**< @brief previous chunk of the FIFO (newer elements) */
};

/**
 * @brief Initializes a mpc_common_fifo_chunk
 * @param ch the chunk to initialize
 * @param chunk_size the number of elements the chunk can contain
 * @param elem_size the size of the elements to store
 */
void mpc_common_fifo_chunk_init( struct mpc_common_fifo_chunk *ch,
			       uint64_t chunk_size, uint64_t elem_size );

/**
 * @brief Allocates a new mpc_common_fifo_chunk
 * @param chunk_size the number of elements the chunk can contain
 * @param elem_size the size of the elements to store
 * @return the newly created chunk
 */
struct mpc_common_fifo_chunk *mpc_common_fifo_chunk_new( uint64_t chunk_size, uint64_t elem_size );

/**
 * @brief Deallocates a new mpc_common_fifo_chunk
 * @param ch the chunk to deallocate
 */
void mpc_common_fifo_chunk_release( struct mpc_common_fifo_chunk *ch );

/**
 * @brief Pushes an element into a chunk
 * @param ch the chunk where to push the element
 * @param elem the element to push
 * @return a pointer to the element stored in ch->payload (NULL if there is not enough room)
 */
void *mpc_common_fifo_chunk_push( struct mpc_common_fifo_chunk *ch, void *elem );

/**
 * @brief pops en element from a chunk
 * @param ch the chunk where to push the element
 * @param dest the element where to copy data
 * @return a pointer to the element stored in ch->payload (NULL the chunk is empty)
 */
void *mpc_common_fifo_chunk_pop( struct mpc_common_fifo_chunk *ch, void *dest );

// NOLINTBEGIN(clang-diagnostic-unused-function)

/**
 * @brief Thread-safely sets the previous chunk (ch->prev)
 * @param ch the chunk where to set the previous chunk
 * @param prev the new previous chunk
 */
static inline void mpc_common_fifo_chunk_ctx_prev( struct mpc_common_fifo_chunk *ch, struct mpc_common_fifo_chunk *prev )
{
	mpc_common_spinlock_lock( &ch->lock );
	ch->prev = prev;
	mpc_common_spinlock_unlock( &ch->lock );
}

/**
 * @brief Thread-safely gets the previous chunk (ch->prev)
 * @param ch the chunk from where to get the previous chunk
 * @return the previous chunk
 */
static inline struct mpc_common_fifo_chunk *mpc_common_fifo_chunk_prev( struct mpc_common_fifo_chunk *ch )
{
	struct mpc_common_fifo_chunk *ret;
	mpc_common_spinlock_lock( &ch->lock );
	ret = ch->prev;
	mpc_common_spinlock_unlock( &ch->lock );

	return ret;
}

// NOLINTEND(clang-diagnostic-unused-function)

/**
 * @}
 */

/**
 * @addtogroup mpc_common_fifo_
 * @{
 */

#ifdef __cplusplus
}
#endif

#endif /* BUFFERED_FIFO_H  */

/**
 * @}
 */
