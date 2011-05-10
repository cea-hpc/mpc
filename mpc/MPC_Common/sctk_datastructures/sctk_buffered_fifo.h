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
/* #   - DIDELOT Sylvain didelot.sylvain@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sctk_config.h>

#ifndef __SCTK__BUFFERED_FIFO__
#define __SCTK__BUFFERED_FIFO__

/// Buffered fifo container
struct sctk_buffered_fifo_chunk {

    ///The payload array
    void *payload;
    struct sctk_buffered_fifo_chunk *p_prev;
    struct sctk_buffered_fifo_chunk *p_next;

    ///Number of elems in chunk
    uint32_t current_offset;

    ///Elems popped
    uint32_t current_begin;
};

/// Generic sctk_buffered_fifo
struct sctk_buffered_fifo {
    struct sctk_buffered_fifo_chunk *head;
    struct sctk_buffered_fifo_chunk *tail;

    ///Size of the payload
    size_t elem_size;

    ///Number of elem in a chunk
    uint32_t chunk_size;
    uint64_t elem_count;

    ////mode
    uint8_t is_collector;

    ///Is initialized
    uint8_t is_initialized;

    ///Lock
    pthread_mutex_t lock;

};

/**Ininitalizes a buffered fifo
* @param fifo Pointer to an allocated sctk_buffered_fifo
* @param elem_size Sizeof an element eg sizeof( TYPE ) ...
* @param chunk_size Number of elems per chunk (this is le unit of malloc )
* @param this defines if the chucks are to be freed when popped
* (it solves the ref problem)
**/
void sctk_buffered_fifo_new(struct sctk_buffered_fifo *fifo, size_t elem_size,
			uint32_t chunk_size, uint8_t is_collector);

/**Releases a buffered fifo
* @param fifo Pointer to an initialized sctk_buffered_fifo
* @param free_func function called on each chunks uppon release ( NULL means none )
**/
void sctk_buffered_fifo_free(struct sctk_buffered_fifo *fifo,
			   void (*free_func) (void *));

/**Push an elem in FIFO mode
* @param fifo Pointer to an initialized sctk_buffered_fifo
* @param elem pointer to an elem of the right type
* @return Pointer to the pushed value (useful for "collector" mode )
**/
void *sctk_buffered_fifo_push(struct sctk_buffered_fifo *fifo, void *elem);

/**Pop a pointer to the popped element
* @param fifo Pointer to an initialized sctk_buffered_fifo
* @return Pointer to the popped value
**/
void *sctk_buffered_fifo_pop_ref(struct sctk_buffered_fifo *fifo);

/**Pop a pointer to the popped element
* @param fifo Pointer to an initialized sctk_buffered_fifo
* @param dest where to coppy the popped element
* @return Pointer to the popped value
**/
void *sctk_buffered_fifo_pop_elem(struct sctk_buffered_fifo *fifo, void *dest);

//Internals

/**Pop a pointer to the popped elem (in a fifo chunk)
* @param fifo Pointer to an initialized sctk_buffered_fifo
* @param fifo_chunk from which to pop an elem
* @return Pointer to the popped value
**/
void *sctk_buffered_fifo_chunk_pop(struct sctk_buffered_fifo *fifo,
			      struct sctk_buffered_fifo_chunk *fifo_chunk);

/**Alocate a chunk
* @param fifo Pointer to an initialized sctk_buffered_fifo
* @param p_prev previous elem
* @param p_next next elem
* @return pointer to the fifo chunk
**/
struct sctk_buffered_fifo_chunk *
sctk_buffered_fifo_alloc_chunk(struct sctk_buffered_fifo
						      *fifo, struct
						      sctk_buffered_fifo_chunk
						      *p_prev, struct
						      sctk_buffered_fifo_chunk
						      *p_next);


/**Get ptr to head elem
* @param fifo Pointer to an initialized sctk_buffered_fifo
* @return pointer to the head
**/
//void *sctk_buffered_fifo_head(struct sctk_buffered_fifo *fifo);

  int
sctk_buffered_fifo_is_empty(struct sctk_buffered_fifo *fifo);

void *sctk_buffered_fifo_getnth(struct sctk_buffered_fifo *fifo, uint64_t n);

/**Get ptr to tail elem
* @param fifo Pointer to an initialized sctk_buffered_fifo
* @return pointer to the tail
**/
void*
sctk_buffered_fifo_get_elem_from_tail(struct sctk_buffered_fifo *fifo, uint64_t n);

#endif
