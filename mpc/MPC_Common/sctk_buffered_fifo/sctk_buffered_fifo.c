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

//TODO : Implement remaining locks

#include "sctk_buffered_fifo.h"

    void
sctk_buffered_fifo_new(struct sctk_buffered_fifo *fifo, size_t elem_size,
        uint32_t chunk_size, uint8_t is_collector)
{
    fifo->head = NULL;
    fifo->tail = NULL;
    fifo->is_collector = is_collector;
    fifo->is_initialized = 1;
    fifo->elem_count = 0;
    fifo->chunk_size = chunk_size;
    fifo->elem_size = elem_size;
}

void
sctk_buffered_fifo_free(struct sctk_buffered_fifo *fifo,
        void (*free_func) (void *))
{
    struct sctk_buffered_fifo_chunk *tmp = fifo->head;
    struct sctk_buffered_fifo_chunk *to_free = NULL;
    uint32_t i = 0;
    while (tmp) {
        to_free = tmp;
        if (free_func) {
            for (i = 0; i < tmp->current_offset; i++) {
                free_func((char *) to_free->payload +
                        (i * fifo->elem_size));
            }}
        tmp = tmp->p_next;
        free(to_free->payload);
        free(to_free);
    }

}

struct sctk_buffered_fifo_chunk *
sctk_buffered_fifo_alloc_chunk(struct
        sctk_buffered_fifo
        *fifo, struct
        sctk_buffered_fifo_chunk
        *p_prev, struct
        sctk_buffered_fifo_chunk
        *p_next)
{
    struct sctk_buffered_fifo_chunk *tmp =
        malloc(sizeof(struct sctk_buffered_fifo_chunk));
    tmp->payload = malloc(fifo->elem_size * (fifo->chunk_size + 1));
    tmp->p_prev = p_prev;
    tmp->p_next = p_next;
    tmp->current_begin = 0;
    tmp->current_offset = 0;
    return tmp;
}


void *
sctk_buffered_fifo_push(struct sctk_buffered_fifo *fifo, void *elem)
{
  if (fifo->head == NULL) {
        fifo->head = sctk_buffered_fifo_alloc_chunk(fifo, NULL, NULL);
        fifo->tail = fifo->head;
    }

    else {
        if (fifo->head->current_offset == fifo->chunk_size) {
            fifo->head = sctk_buffered_fifo_alloc_chunk(fifo, NULL, fifo->head);
            fifo->head->p_next->p_prev = fifo->head;
        }
    }

    void *ret =
        (char *) fifo->head->payload +
        (fifo->head->current_offset * fifo->elem_size);
    fifo->head->current_offset++;
    fifo->elem_count++;

    memcpy(ret, elem, fifo->elem_size);
    return ret;
}

void *
sctk_buffered_fifo_chunk_pop(struct sctk_buffered_fifo *fifo,
        struct sctk_buffered_fifo_chunk *fifo_chunk)
{
    void *ret = NULL;
    if (fifo_chunk == NULL)
        return ret;

    if (fifo_chunk->current_offset <= fifo_chunk->current_begin ) {

        fifo->tail = fifo_chunk->p_prev;

        if( !fifo->is_collector )
        {
            if( fifo_chunk->p_prev )
                fifo_chunk->p_prev->p_next = NULL;
            free(fifo_chunk->payload);
            free(fifo_chunk);
        }

        if (fifo->tail == NULL) {
            fifo->head = NULL;
        }

        ret = sctk_buffered_fifo_chunk_pop(fifo, fifo->tail);

        return ret;
    }

    ret =
        (char *) fifo_chunk->payload +
        (fifo_chunk->current_begin * fifo->elem_size);

    fifo_chunk->current_begin++;

    return ret;
}


void *
sctk_buffered_fifo_pop_elem(struct sctk_buffered_fifo *fifo, void *dest)
{
    void* elem;

    elem = sctk_buffered_fifo_chunk_pop(fifo, fifo->tail);
    if (!elem)
    {
        return NULL;
    }
    fifo->elem_count--;

    if(dest)
      memcpy(dest, elem, fifo->elem_size);

    return elem;
}


void *
sctk_buffered_fifo_pop_ref(struct sctk_buffered_fifo *fifo)
{
    if( !fifo->is_collector )
    {
        printf("You can't pop refs if not in collector mode !\n");
        return NULL;
    }

    if( !fifo )
        return NULL;

    fifo->elem_count--;
    return sctk_buffered_fifo_chunk_pop(fifo, fifo->tail);
}


  int
sctk_buffered_fifo_is_empty(struct sctk_buffered_fifo *fifo)
{
    if (fifo->elem_count)
      return 0;
    return 1;
}


void*
sctk_buffered_fifo_get_elem_from_tail(struct sctk_buffered_fifo *fifo, uint64_t n)
{
    struct sctk_buffered_fifo_chunk *tmp = fifo->tail;
    uint64_t n_offset;
    char* ret;

    while (tmp) {

        n_offset = n + tmp->current_begin;

        if( n_offset < tmp->current_offset )
        {
            ret = (char *) tmp->payload + ( n_offset * (fifo->elem_size)) ;

            return ret;
        }
        else
        {
            n -= (tmp->current_offset - tmp->current_begin);
            tmp = tmp->p_prev;
        }
    }

    return NULL;
}


void *sctk_buffered_fifo_getnth(struct sctk_buffered_fifo *fifo, uint64_t n)
{
    struct sctk_buffered_fifo_chunk *tmp = fifo->head;
    char* ret;

    while (tmp) {

        if( n < tmp->current_offset )
        {
            ret = (char *) tmp->payload + (n * fifo->elem_size);
            return ret;
        }
        else
        {
            n -= tmp->current_offset;
            tmp = tmp->p_next;
        }
    }

    return NULL;
}
