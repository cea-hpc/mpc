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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include "hashtable.h"

#include <mpc_common_spinlock.h>

#include <mpc_common_debug.h>
#include <string.h>

#include <sctk_alloc.h>

void _mpc_ht_cell_init( struct _mpc_ht_cell *cell, uint64_t key, void *data, struct _mpc_ht_cell *next )
{
	cell->use_flag = 1;
	cell->key = key;
	cell->data = data;
	cell->next = next;
}

struct _mpc_ht_cell *_mpc_ht_cell_new( uint64_t key, void *data, struct _mpc_ht_cell *next )
{
	struct _mpc_ht_cell *ret = sctk_malloc( sizeof( struct _mpc_ht_cell ) );

	if ( !ret )
	{
		abort();
	}

	_mpc_ht_cell_init( ret, key, data, next );
	return ret;
}

void _mpc_ht_cell_release( struct _mpc_ht_cell *cell )
{
	while ( cell )
	{
		struct _mpc_ht_cell *to_sctk_free = cell;
		cell = cell->next;
		memset( to_sctk_free, 0, sizeof( struct _mpc_ht_cell ) );
		sctk_free( to_sctk_free );
	}
}

struct _mpc_ht_cell *_mpc_ht_cell_get( struct _mpc_ht_cell *cell, uint64_t key )
{
	while ( cell )
	{
		if ( cell->key == key )
		{
			return cell;
		}

		cell = cell->next;
	}

	return NULL;
}

struct _mpc_ht_cell *_mpc_ht_cell_pop( struct _mpc_ht_cell *head, uint64_t key )
{
	struct _mpc_ht_cell *target = _mpc_ht_cell_get( head, key );

	if ( !target )
	{
		/* Leave the list inchanged */
		return head;
	}

	if ( head == target )
	{
		struct _mpc_ht_cell *next = head->next;
		sctk_free( head );
		return next;
	}

	struct _mpc_ht_cell *tmp = head;

	while ( tmp )
	{
		if ( tmp->next == target )
		{
			tmp->next = target->next;
			sctk_free( target );
			return head;
		}

		tmp = tmp->next;
	}

	/* Should never happen */
	return head;
}

static inline uint64_t murmur_hash( uint64_t val )
{
	/* This is MURMUR Hash under MIT
		 * https://code.google.com/p/smhasher/ */
	uint64_t h = val;
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdllu;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53llu;
	h ^= h >> 33;
	return h;
}

/* THESE Functions are locking buckets (as offset) */

void mpc_common_hashtable_lock_read( struct mpc_common_hashtable *ht, uint64_t bucket )
{
	mpc_common_rwlock_t *lock = &ht->rwlocks[bucket];
	mpc_common_spinlock_read_lock_yield( lock );
}

void mpc_common_hashtable_unlock_read( struct mpc_common_hashtable *ht, uint64_t bucket )
{
	mpc_common_rwlock_t *lock = &ht->rwlocks[bucket];
	mpc_common_spinlock_read_unlock( lock );
}

static inline void mpc_common_hashtable_lock_write( struct mpc_common_hashtable *ht, uint64_t bucket )
{
	mpc_common_nodebug( "LOCKING cell %d", bucket );
	mpc_common_rwlock_t *lock = &ht->rwlocks[bucket];
	mpc_common_spinlock_write_lock_yield( lock );
}

static inline void mpc_common_hashtable_unlock_write( struct mpc_common_hashtable *ht, uint64_t bucket )
{
	mpc_common_nodebug( "UN-LOCKING cell %d", bucket );
	mpc_common_rwlock_t *lock = &ht->rwlocks[bucket];
	mpc_common_spinlock_write_unlock( lock );
}

/* THESE Functions are locking buckets according to KEYS */

void mpc_common_hashtable_lock_cell_read( struct mpc_common_hashtable *ht, uint64_t key )
{
	mpc_common_hashtable_lock_read( ht, murmur_hash( key ) % ht->table_size );
}

void mpc_common_hashtable_unlock_cell_read( struct mpc_common_hashtable *ht, uint64_t key )
{
	mpc_common_hashtable_unlock_read( ht, murmur_hash( key ) % ht->table_size );
}

void mpc_common_hashtable_lock_cell_write( struct mpc_common_hashtable *ht, uint64_t key )
{
	mpc_common_hashtable_lock_write( ht, murmur_hash( key ) % ht->table_size );
}

void mpc_common_hashtable_unlock_cell_write( struct mpc_common_hashtable *ht, uint64_t key )
{
	mpc_common_hashtable_unlock_write( ht, murmur_hash( key ) % ht->table_size );
}

void mpc_common_hashtable_init( struct mpc_common_hashtable *ht, uint64_t size )
{
	if ( size == 0 )
	{
		abort();
	}

	ht->table_size = size;
	ht->cells = sctk_calloc( size, sizeof( struct _mpc_ht_cell ) );

	if ( ht->cells == NULL )
	{
		perror( "sctk_malloc" );
		mpc_common_debug_fatal( "Could not create HT array" );
	}

	ht->rwlocks = sctk_malloc( size * sizeof( mpc_common_rwlock_t ) );

	if ( ht->rwlocks == NULL )
	{
		perror( "sctk_malloc" );
		mpc_common_debug_fatal( "Could not create HT RW lock array" );
	}

	unsigned int i;

	for ( i = 0; i < size; i++ )
	{
		sctk_spin_rwlock_init( &( ht->rwlocks[i] ) );
	}
}
void mpc_common_hashtable_release( struct mpc_common_hashtable *ht )
{
	unsigned int i;

	for ( i = 0; i < ht->table_size; i++ )
	{
		mpc_common_hashtable_lock_write( ht, i );

		if ( ht->cells[i].next )
		{
			_mpc_ht_cell_release( ht->cells[i].next );
		}

		mpc_common_hashtable_unlock_write( ht, i );
	}

	sctk_free( ht->cells );
	ht->cells = NULL;
	sctk_free( ht->rwlocks );
	ht->rwlocks = NULL;
	ht->table_size = 0;
}

void *mpc_common_hashtable_get( struct mpc_common_hashtable *ht, uint64_t key )
{
	uint64_t bucket = murmur_hash( key ) % ht->table_size;
	struct _mpc_ht_cell *head = &ht->cells[bucket];
	mpc_common_hashtable_lock_read( ht, bucket );

	if ( !head->use_flag )
	{
		/* The static header cell is empty */
		mpc_common_hashtable_unlock_read( ht, bucket );
		return NULL;
	}

	if ( head->key == key )
	{
		/* Direct match */
		void *ret = head->data;
		mpc_common_hashtable_unlock_read( ht, bucket );
		return ret;
	}

	/* Now walk sibblings */
	struct _mpc_ht_cell *cell = _mpc_ht_cell_get( head->next, key );
	void *ret = NULL;

	if ( cell )
	{
		ret = cell->data;
	}

	mpc_common_hashtable_unlock_read( ht, bucket );
	return ret;
}

void *mpc_common_hashtable_get_or_create( struct mpc_common_hashtable *ht, uint64_t key, void *( create_entry )( uint64_t key ), int *did_create )
{
	uint64_t bucket = murmur_hash( key ) % ht->table_size;
	struct _mpc_ht_cell *head = &ht->cells[bucket];

	if ( did_create )
	{
		*did_create = 1;
	}

	mpc_common_hashtable_lock_write( ht, bucket );
	void *data = NULL;

	if ( head->use_flag == 0 )
	{
		/* Static cell is free */
		if ( create_entry )
		{
			data = ( create_entry )( key );
		}

		_mpc_ht_cell_init( head, key, data, NULL );
		mpc_common_hashtable_unlock_write( ht, bucket );
		return head->data;
	}

	/* Otherwise make sure that such cell is not present yet */
	struct _mpc_ht_cell *candidate = _mpc_ht_cell_get( head, key );

	if ( candidate )
	{
		mpc_common_hashtable_unlock_write( ht, bucket );

		if ( did_create )
		{
			*did_create = 0;
		}

		return candidate->data;
	}

	/* If not we have to push it */

	if ( create_entry )
	{
		data = ( create_entry )( key );
	}

	struct _mpc_ht_cell *new_cell = _mpc_ht_cell_new( key, data, head->next );

	head->next = new_cell;

	mpc_common_hashtable_unlock_write( ht, bucket );

	return new_cell->data;
}

void mpc_common_hashtable_set( struct mpc_common_hashtable *ht, uint64_t key, void *data )
{
	uint64_t bucket = murmur_hash( key ) % ht->table_size;
	struct _mpc_ht_cell *head = &ht->cells[bucket];
	mpc_common_hashtable_lock_write( ht, bucket );

	if ( head->use_flag == 0 )
	{
		/* Static cell is free */
		_mpc_ht_cell_init( head, key, data, NULL );
		mpc_common_hashtable_unlock_write( ht, bucket );
		return;
	}

	/* Otherwise make sure that such cell is not present yet */
	struct _mpc_ht_cell *candidate = _mpc_ht_cell_get( head, key );

	if ( candidate )
	{
		candidate->data = data;
		mpc_common_hashtable_unlock_write( ht, bucket );
		return;
	}

	/* If not we have to push it */
	struct _mpc_ht_cell *new_cell = _mpc_ht_cell_new( key, data, head->next );
	head->next = new_cell;
	mpc_common_hashtable_unlock_write( ht, bucket );
}

void mpc_common_hashtable_delete( struct mpc_common_hashtable *ht, uint64_t key )
{
	uint64_t bucket = murmur_hash( key ) % ht->table_size;
	struct _mpc_ht_cell *head = &ht->cells[bucket];
	mpc_common_hashtable_lock_write( ht, bucket );

	if ( head->key == key )
	{
		if ( head->next )
		{
			struct _mpc_ht_cell *tofree = head->next;
			memcpy( head, tofree, sizeof( struct _mpc_ht_cell ) );
			sctk_free( tofree );
		}
		else
		{
			/* Just flag head as unused */
			head->use_flag = 0;
		}

		mpc_common_hashtable_unlock_write( ht, bucket );
		return;
	}

	/* Handle the tail */
	head->next = _mpc_ht_cell_pop( head->next, key );
	mpc_common_hashtable_unlock_write( ht, bucket );
}
int mpc_common_hashtable_empty( struct mpc_common_hashtable *ht )
{
	int i, sz = ht->table_size;

	for ( i = 0; i < sz; ++i )
	{
		if ( ht->cells[i].use_flag != 0 )
		{
			return 0;
		}
	}

	return 1;
}
