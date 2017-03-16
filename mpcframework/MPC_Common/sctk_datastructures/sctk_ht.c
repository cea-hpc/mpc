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
#include "sctk_ht.h"
#include "sctk_thread.h"

#include <string.h>

void MPCHT_Cell_init( struct MPCHT_Cell * cell, sctk_uint64_t key, void * data, struct MPCHT_Cell * next )
{
	cell->use_flag = 1;
	cell->key = key;
	cell->data = data;
	cell->next = next;
}

struct MPCHT_Cell * MPCHT_Cell_new( sctk_uint64_t key, void * data, struct MPCHT_Cell * next )
{
	struct MPCHT_Cell * ret = sctk_malloc( sizeof( struct MPCHT_Cell ) );
	
	if( !ret )
	{
		abort();
	}
	
	MPCHT_Cell_init( ret, key, data, next );
	
	return ret;
}


void MPCHT_Cell_release( struct MPCHT_Cell * cell )
{
	while( cell )
	{
		struct MPCHT_Cell * to_sctk_free = cell;
		cell = cell->next;
		memset( to_sctk_free, 0, sizeof( struct MPCHT_Cell ) );
		sctk_free( to_sctk_free );
	}
}

struct MPCHT_Cell * MPCHT_Cell_get( struct MPCHT_Cell * cell, sctk_uint64_t key )
{
		while( cell )
		{
			if( cell->key == key )
				return cell;
			
			cell = cell->next;
		}
		
		return NULL;
}


struct MPCHT_Cell * MPCHT_Cell_pop( struct MPCHT_Cell * head, sctk_uint64_t key )
{
	struct MPCHT_Cell * target = MPCHT_Cell_get( head, key );
	
	if( !target )
	{
		/* Leave the list inchanged */
		return head;
	}
	
	if( head == target )
	{
		struct MPCHT_Cell * next = head->next;
		sctk_free( head );
		return next;
	}
	
	struct MPCHT_Cell * tmp = head;
	
	while( tmp )
	{
		if( tmp->next == target )
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

static inline sctk_uint64_t murmur_hash( sctk_uint64_t val )
{
		/* This is MURMUR Hash under MIT 
		 * https://code.google.com/p/smhasher/ */
        sctk_uint64_t h = val;

        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdllu;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53llu;
        h ^= h >> 33;

        return h;
}

/* THESE Functions are locking buckets (as offset) */

static inline void MPCHT_lock_read( struct MPCHT * ht , sctk_uint64_t bucket)
{
	sctk_spin_rwlock_t * lock = &ht->rwlocks[bucket];
	sctk_spinlock_read_lock( lock );
}

static inline void MPCHT_unlock_read( struct MPCHT * ht , sctk_uint64_t bucket)
{
	sctk_spin_rwlock_t * lock = &ht->rwlocks[bucket];
	sctk_spinlock_read_unlock( lock );
}

static inline void MPCHT_lock_write( struct MPCHT * ht , sctk_uint64_t bucket)
{
  sctk_nodebug("LOCKING cell %d", bucket);
  sctk_spin_rwlock_t *lock = &ht->rwlocks[bucket];
  sctk_spinlock_write_lock_yield(lock);
}

static inline void MPCHT_unlock_write( struct MPCHT * ht , sctk_uint64_t bucket)
{
  sctk_nodebug("UN-LOCKING cell %d", bucket);
  sctk_spin_rwlock_t *lock = &ht->rwlocks[bucket];
  sctk_spinlock_write_unlock(lock);
}


/* THESE Functions are locking buckets according to KEYS */

void MPCHT_lock_cell_read( struct MPCHT * ht , sctk_uint64_t key)
{
	MPCHT_lock_read( ht, murmur_hash( key ) % ht->table_size);
}

void MPCHT_unlock_cell_read( struct MPCHT * ht , sctk_uint64_t key)
{
	MPCHT_unlock_read( ht, murmur_hash( key ) % ht->table_size);
}

void MPCHT_lock_cell_write( struct MPCHT * ht , sctk_uint64_t key)
{
	MPCHT_lock_write( ht, murmur_hash( key ) % ht->table_size);
}

void MPCHT_unlock_cell_write( struct MPCHT * ht , sctk_uint64_t key)
{
	MPCHT_unlock_write( ht, murmur_hash( key ) % ht->table_size);
}


void MPCHT_init( struct MPCHT * ht, sctk_uint64_t size )
{
	if( size == 0 )
	{
		abort();
	}
	
	ht->table_size = size;
	
	ht->cells = sctk_calloc( size, sizeof( struct MPCHT_Cell ) );
	
	if( ht->cells == NULL )
	{
		perror("sctk_malloc");
		sctk_fatal("Could not create HT array");
	}
	
	ht->rwlocks = sctk_malloc( size * sizeof( sctk_spin_rwlock_t ) );
	
	if( ht->rwlocks == NULL )
	{
		perror("sctk_malloc");
		sctk_fatal("Could not create HT RW lock array");
	}
	
	int i;
	
	for( i = 0 ; i < size ; i++ )
		sctk_spin_rwlock_init(&(ht->rwlocks[i]));
}
void MPCHT_release( struct MPCHT * ht )
{
	int i;
	

	for( i = 0 ; i < ht->table_size ; i++ )
	{
		MPCHT_lock_write( ht , i);
	
		if( ht->cells[i].next )
		{
			MPCHT_Cell_release( ht->cells[i].next );
		}
	}
	
	sctk_free( ht->cells );
	
	ht->table_size = 0;
}

void * MPCHT_get(  struct MPCHT * ht, sctk_uint64_t key )
{
	sctk_uint64_t bucket = murmur_hash( key ) % ht->table_size;
	
	struct MPCHT_Cell * head = &ht->cells[bucket];
	
	MPCHT_lock_read( ht, bucket );
	
	if( !head->use_flag )
	{
		/* The static header cell is empty */
		MPCHT_unlock_read( ht, bucket );
		return NULL;
	}
	
	if( head->key == key )
	{
		/* Direct match */
                void *ret = head->data;
                MPCHT_unlock_read(ht, bucket);
                return ret;
        }

        /* Now walk sibblings */
        struct MPCHT_Cell *cell = MPCHT_Cell_get(head->next, key);
        void *ret = NULL;

        if (cell)
          ret = cell->data;

        MPCHT_unlock_read(ht, bucket);
        return ret;
}

void * MPCHT_get_or_create(  struct MPCHT * ht, sctk_uint64_t key , void * (create_entry)( sctk_uint64_t key ), int * did_create )
{
	sctk_uint64_t bucket = murmur_hash( key ) % ht->table_size;
	
	struct MPCHT_Cell * head = &ht->cells[bucket];

	if( did_create )
		*did_create = 1;	
	
	MPCHT_lock_write( ht, bucket );
	
	void * data = NULL;
	
	if( head->use_flag == 0 )
	{
		/* Static cell is free */
		
		if( create_entry )
		{
			data = (create_entry)( key );
		}
		
		MPCHT_Cell_init( head, key, data, NULL );
		MPCHT_unlock_write( ht, bucket );
		return head->data;
	}

	/* Otherwise make sure that such cell is not present yet */
	struct MPCHT_Cell * candidate = MPCHT_Cell_get( head, key );
	
	if( candidate )
	{
		MPCHT_unlock_write( ht, bucket );
		if( did_create )
			*did_create = 0;	
		return candidate->data;
	}
	
	/* If not we have to push it */
	
	if( create_entry )
	{
		data = (create_entry)( key );
	}
	
	struct MPCHT_Cell * new_cell = MPCHT_Cell_new( key, data, head->next );
	head->next = new_cell;
	
	MPCHT_unlock_write( ht, bucket );
	
	return new_cell->data;
}

void MPCHT_set(  struct MPCHT * ht, sctk_uint64_t key, void * data )
{
	sctk_uint64_t bucket = murmur_hash( key ) % ht->table_size;
	
	struct MPCHT_Cell * head = &ht->cells[bucket];
	
	MPCHT_lock_write( ht, bucket );

        if (head->use_flag == 0) {
          /* Static cell is free */
          MPCHT_Cell_init(head, key, data, NULL);
          MPCHT_unlock_write(ht, bucket);
          return;
        }

        /* Otherwise make sure that such cell is not present yet */
        struct MPCHT_Cell *candidate = MPCHT_Cell_get(head, key);

        if (candidate) {
          candidate->data = data;
          MPCHT_unlock_write(ht, bucket);
          return;
        }

        /* If not we have to push it */
        struct MPCHT_Cell *new_cell = MPCHT_Cell_new(key, data, head->next);
        head->next = new_cell;

        MPCHT_unlock_write(ht, bucket);
}

void MPCHT_delete(  struct MPCHT * ht, sctk_uint64_t key )
{
	sctk_uint64_t bucket = murmur_hash( key ) % ht->table_size;
	
	struct MPCHT_Cell * head = &ht->cells[bucket];
	
	MPCHT_lock_write( ht, bucket );
	
	if( head->key == key )
	{
		if( head->next )
		{
			struct MPCHT_Cell * tofree = head->next;
			memcpy( head, tofree, sizeof( struct MPCHT_Cell ) );
			sctk_free( tofree );
		}
		else
		{
			/* Just flag head as unused */
			head->use_flag = 0;
		}
		
		MPCHT_unlock_write( ht, bucket );
		return;
	}
	
	/* Handle the tail */
	head->next = MPCHT_Cell_pop( head->next, key );

	MPCHT_unlock_write( ht, bucket );
}
