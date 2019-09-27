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
  extern "C"
  {
#endif



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mpc_common_spinlock.h"

/**
 * @brief Structure defining a chunk of FIFO. 
 * The Buffered_FIFO is made of several Buffered_FIFO_chunk containing the data. 
 * 
 * This is useful to avoid allocating/deallocating at each push/pop. 
 */
struct Buffered_FIFO_chunk {
	mpc_common_spinlock_t lock;                 /**< @brief a lock for concurrent internal manipulations */
	char *payload;                      /**< @brief the actual data */
	uint64_t chunk_size;                /**< @brief the size of the current chunk */
	size_t elem_size;                   /**< @brief the size of stored elements */
	uint64_t start_offset;              /**< @brief the start offset (position of oldest element in payload */
	uint64_t end_offset;                /**< @brief the end offset (position of the end of newest element in payload */
	
	struct Buffered_FIFO_chunk *prev;   /**< @brief previous chunk of the FIFO (newer elements) */
};

/**
 * @brief Initializes a Buffered_FIFO_chunk
 * @param ch the chunk to initialize
 * @param chunk_size the number of elements the chunk can contain
 * @param elem_size the size of the elements to store
 */
void Buffered_FIFO_chunk_init( struct Buffered_FIFO_chunk *ch, 
								uint64_t chunk_size, uint64_t elem_size );

/**
 * @brief Allocates a new Buffered_FIFO_chunk
 * @param chunk_size the number of elements the chunk can contain
 * @param elem_size the size of the elements to store
 * @return the newly created chunk
 */
struct Buffered_FIFO_chunk * Buffered_FIFO_chunk_new( uint64_t chunk_size, uint64_t elem_size );

/**
 * @brief Deallocates a new Buffered_FIFO_chunk
 * @param ch the chunk to deallocate
 */
void Buffered_FIFO_chunk_release( struct Buffered_FIFO_chunk *ch );

/**
 * @brief Pushes an element into a chunk
 * @param ch the chunk where to push the element
 * @param elem the element to push
 * @return a pointer to the element stored in ch->payload (NULL if there is not enough room)
 */
void * Buffered_FIFO_chunk_push( struct Buffered_FIFO_chunk *ch, void *elem );

/**
 * @brief pops en element from a chunk
 * @param ch the chunk where to push the element
 * @param dest the element where to copy data
 * @return a pointer to the element stored in ch->payload (NULL the chunk is empty)
 */
void * Buffered_FIFO_chunk_pop( struct Buffered_FIFO_chunk *ch, void *dest );

/**
 * @brief Thread-safely sets the previous chunk (ch->prev)
 * @param ch the chunk where to set the previous chunk
 * @param prev the new previous chunk
 */
static inline void Buffered_FIFO_chunk_ctx_prev( struct Buffered_FIFO_chunk *ch, struct Buffered_FIFO_chunk *prev)
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
static inline struct Buffered_FIFO_chunk * Buffered_FIFO_chunk_prev( struct Buffered_FIFO_chunk *ch )
{
	struct Buffered_FIFO_chunk *ret;
	mpc_common_spinlock_lock( &ch->lock );
	ret =ch->prev;
	mpc_common_spinlock_unlock( &ch->lock );

	return ret;
}

/**
 * @}
 */

/**
 * @addtogroup Buffered_FIFO_
 * @{
 */

/**
 * @brief This is a struct defining a FIFO
 * It is composed of several Buffered_FIFO_chunk
 */
struct Buffered_FIFO {
	mpc_common_spinlock_t head_lock;            /**< @brief a lock for concurent access to the head */
	struct Buffered_FIFO_chunk *head;   /**< @brief the head chunk (chere elements are pushed */
	mpc_common_spinlock_t tail_lock;            /**< @brief a lock for concurent access to the tail */
	struct Buffered_FIFO_chunk *tail;   /**< @brief the tail chunk (from where elements are popped */

	uint64_t chunk_size;                /**< @brief the size of the composing chunks */
	size_t elem_size;                   /**< @brief the size of each stored element */

	mpc_common_spinlock_t lock;                 /**< @brief a lock for concurrent access to element_count */
	uint64_t elem_count;                /**< @brief the number of elements currently stored in the FIFO */
};

/**
 * @brief Initializes a Buffered_FIFO
 * @param fifo the FIFO to initialize
 * @param chunk_size the wanted size for the chunks
 * @param elem_size the size of stored elements
 */
void Buffered_FIFO_init(struct Buffered_FIFO *fifo, uint64_t chunk_size, size_t elem_size);

/**
 * @brief releases a FIFO
 * @param fifo the FIFO to release
 * @param free_func 
 * @warning free_func parameter is unused. 
 * Release of FIFO consists of setting everything to 0 in it. 
 */
void Buffered_FIFO_release(struct Buffered_FIFO *fifo);

/**
 * @brief Pushes an element into a FIFO
 * @param fifo the FIFO where to push the element
 * @param elem the element to push
 * @return the internally copied element
 */
void *Buffered_FIFO_push(struct Buffered_FIFO *fifo, void *elem);

/**
 * @brief Pops an element from a FIFO
 * @param fifo the FIFO from where to pop the element
 * @param dest where to copy the popped element
 * @return the popped element, NULL if the FIFO is empty
 */
void *Buffered_FIFO_pop(struct Buffered_FIFO *fifo, void *dest);

/**
 * @brief Thread-safely gets the number of elements stored in the FIFO
 * @param fifo the FIFO where to count elements
 * @return the number of elements stored in the FIFO
 */
uint64_t Buffered_FIFO_count(struct Buffered_FIFO *fifo);

/**
 * @brief Apply a function to each element of the FIFO
 * @param fifo the FIFO where to process elements
 * @param func the function to apply to all elements
 * @param arg arbitrary pointer passed to function calls
 * @return number of elements processed
 */
int Buffered_FIFO_process(struct Buffered_FIFO *fifo, int (*func)( void * elem, void * arg), void * arg );

#ifdef __cplusplus
  }
#endif


#endif /* BUFFERED_FIFO_H  */
  
/**
 * @}
 */
