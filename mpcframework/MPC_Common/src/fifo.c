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
#include "fifo.h"

#include <stdlib.h>
#include <string.h>

uint64_t mpc_common_fifo_count( struct mpc_common_fifo *fifo )
{
	uint64_t ret = 0;

	mpc_common_spinlock_lock( &fifo->lock );
	ret = fifo->elem_count;
	mpc_common_spinlock_unlock( &fifo->lock );

	return ret;
}

void mpc_common_fifo_chunk_init( struct mpc_common_fifo_chunk *ch,
							   uint64_t chunk_size, uint64_t elem_size )
{
	ch->lock = 0;
	ch->chunk_size = chunk_size;
	ch->elem_size = elem_size;
	ch->start_offset = 0;
	ch->end_offset = 0;

	ch->payload = malloc( chunk_size * elem_size );

	if ( !ch->payload )
	{
		perror( "malloc" );
		exit( 1 );
	}

	ch->prev = NULL;
}

struct mpc_common_fifo_chunk *mpc_common_fifo_chunk_new( uint64_t chunk_size, uint64_t elem_size )
{
	struct mpc_common_fifo_chunk *ret = malloc( sizeof( struct mpc_common_fifo_chunk ) );

	if ( !ret )
	{
		perror( "malloc" );
		exit( 1 );
	}

	mpc_common_fifo_chunk_init( ret, chunk_size, elem_size );

	return ret;
}

void mpc_common_fifo_chunk_release( struct mpc_common_fifo_chunk *ch )
{
	free( ch->payload );
	memset( ch, 0, sizeof( struct mpc_common_fifo_chunk ) );
}

void *mpc_common_fifo_chunk_push( struct mpc_common_fifo_chunk *ch, void *elem )
{
	void *ret = NULL;

	mpc_common_spinlock_lock( &ch->lock );

	/* Do we have free room ? */

	if ( ch->end_offset < ch->chunk_size )
	{
		char *dest = ch->payload + ( ch->end_offset * ch->elem_size );
		ret = dest;
		memcpy( dest, elem, ch->elem_size );
		ch->end_offset++;
	}

	mpc_common_spinlock_unlock( &ch->lock );

	return ret;
}

void *mpc_common_fifo_chunk_pop( struct mpc_common_fifo_chunk *ch, void *dest )
{
	void *ret = NULL;

	mpc_common_spinlock_lock( &ch->lock );

	/* Is there any elem left ? */
	if ( ch->start_offset < ch->end_offset )
	{
		char *src = ch->payload + ( ch->start_offset * ch->elem_size );
		ret = src;
		memcpy( dest, src, ch->elem_size );
		ch->start_offset++;
	}

	mpc_common_spinlock_unlock( &ch->lock );

	return ret;
}

void mpc_common_fifo_init( struct mpc_common_fifo *fifo, uint64_t chunk_size, size_t elem_size )
{
	fifo->head_lock = 0;
	fifo->head = NULL;
	fifo->tail_lock = 0;
	fifo->tail = NULL;

	fifo->chunk_size = chunk_size;
	fifo->elem_size = elem_size;

	fifo->lock = 0;
	fifo->elem_count = 0;
}

void mpc_common_fifo_release( struct mpc_common_fifo *fifo )
{
	struct mpc_common_fifo_chunk *tmp = fifo->head;
	struct mpc_common_fifo_chunk *to_free = NULL;

	mpc_common_spinlock_lock( &fifo->head_lock );
	mpc_common_spinlock_lock( &fifo->tail_lock );

	while ( tmp )
	{
		to_free = tmp;
		tmp = tmp->prev;

		mpc_common_fifo_chunk_release( to_free );
	}

	memset( fifo, 0, sizeof( struct mpc_common_fifo ) );
}

void *mpc_common_fifo_push( struct mpc_common_fifo *fifo, void *elem )
{
	void *ret = NULL;

	mpc_common_spinlock_lock( &fifo->head_lock );
	/* Case of empty FIFO */
	if ( fifo->head == NULL )
	{
		mpc_common_spinlock_lock( &fifo->tail_lock );
		fifo->head = mpc_common_fifo_chunk_new( fifo->chunk_size, fifo->elem_size );
		fifo->tail = fifo->head;
		mpc_common_spinlock_unlock( &fifo->tail_lock );
	}

	while ( !ret )
	{

		ret = mpc_common_fifo_chunk_push( fifo->head, elem );

		/* Chunk is full  create a new one*/
		if ( !ret )
		{
			struct mpc_common_fifo_chunk *new = mpc_common_fifo_chunk_new( fifo->chunk_size, fifo->elem_size );
			mpc_common_fifo_chunk_ctx_prev( fifo->head, new );
			fifo->head = new;
		}
	}

	mpc_common_spinlock_lock( &fifo->lock );
	fifo->elem_count++;
	mpc_common_spinlock_unlock( &fifo->lock );

	mpc_common_spinlock_unlock( &fifo->head_lock );

	return ret;
}

void *mpc_common_fifo_pop( struct mpc_common_fifo *fifo, void *dest )
{
	void *ret = NULL;
	mpc_common_spinlock_lock( &fifo->tail_lock );

	if ( fifo->tail != NULL )
	{
		struct mpc_common_fifo_chunk *target = fifo->tail;

		while ( !ret )
		{
			ret = mpc_common_fifo_chunk_pop( target, dest );

			if ( ret )
			{
				mpc_common_spinlock_lock( &fifo->lock );
				fifo->elem_count--;
				mpc_common_spinlock_unlock( &fifo->lock );
			}
			else
			{
				/* Go to previous chunk */
				struct mpc_common_fifo_chunk *to_free = target;

				target = mpc_common_fifo_chunk_prev( target );

				/* Found previous */
				if ( target )
				{

					/* Free old block */
					mpc_common_fifo_chunk_release( to_free );
					free( to_free );
					/* Move tail */
					fifo->tail = target;
				}
				else
				{
					/* Head is empty */

					break;
				}
			}
		}
	}

	mpc_common_spinlock_unlock( &fifo->tail_lock );

	return ret;
}

int mpc_common_fifo_process( struct mpc_common_fifo *fifo, int ( *func )( void *elem, void *arg ), void *arg )
{
	if ( !fifo || !func )
		return 0;

	int did_process = 0;

	/* Block the FIFO */
	mpc_common_spinlock_lock( &fifo->head_lock );
	mpc_common_spinlock_lock( &fifo->tail_lock );

	struct mpc_common_fifo_chunk *chunk = fifo->head;

	while ( chunk )
	{
		unsigned int i;

		for ( i = chunk->start_offset; i < chunk->end_offset; i++ )
		{
			char *src = chunk->payload + ( i * chunk->elem_size );
			did_process += ( func )( src, arg );
		}

		chunk = chunk->prev;
	}

	mpc_common_spinlock_unlock( &fifo->tail_lock );
	mpc_common_spinlock_unlock( &fifo->head_lock );

	return did_process;
}
