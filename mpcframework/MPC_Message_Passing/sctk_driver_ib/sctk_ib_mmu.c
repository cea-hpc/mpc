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
#include "sctk_ib_mmu.h"
#include "sctk_ib_device.h"
#include "sctk_runtime_config.h"
#include "sctk_atomics.h"

#include <infiniband/verbs.h>
#include <stdlib.h>
#include <errno.h>

#include <stdlib.h>

sctk_ib_mmu_entry_t * sctk_ib_mmu_entry_new( sctk_ib_rail_info_t *rail_ib, void * addr, size_t size )
{
	sctk_ib_mmu_entry_t * new = sctk_malloc( sizeof( sctk_ib_mmu_entry_t ) );

	assume( new != NULL );
	
	/* Save address and size */
	new->addr = addr;
	new->size = size;
	
	/* Pin memory and save memory handle */
	if( rail_ib )
	{
		new->mr = ibv_reg_mr ( rail_ib->device->pd, addr, size, IBV_ACCESS_REMOTE_WRITE
				         | IBV_ACCESS_LOCAL_WRITE
				         | IBV_ACCESS_REMOTE_READ );
	}
	else
	{
		/* For tests */
		new->mr = NULL;
	}

	/* No one ows it yet */
	sctk_spin_rwlock_t lckinit = SCTK_SPIN_RWLOCK_INITIALIZER;
	new->entry_refcounter = lckinit;

	return new;
}

void sctk_ib_mmu_entry_release( sctk_ib_mmu_entry_t * release )
{
	sctk_spinlock_write_lock( &release->entry_refcounter );
	
	int ret = 0;
	
	/* Unregister memory */
	if( release->mr )
	{
		if ( ( ret = ibv_dereg_mr ( release->mr ) ) != 0 )
		{
			sctk_error ( "ibv_dereg_mr returned an error: %d %s (%p)", ret, strerror ( errno ), release->mr );
		}
	}
	
	/* Empty element */
	release->addr = NULL;
	release->size = 0;
	
	/* Free entry */
	sctk_free( release );
}

int sctk_ib_mmu_entry_contains( sctk_ib_mmu_entry_t * entry, void * addr, size_t size )
{
	sctk_nodebug("Test %p (%ld) == %p (%ld)\n",  addr, size,  entry->addr, entry->size );
	
	if( ( entry->addr <= addr )
	&&  ( (addr + size) < (entry->addr + entry->size) ) )
	{
		return 1;
	}
	
	return 0;
}

int sctk_ib_mmu_entry_intersects( sctk_ib_mmu_entry_t * entry, void * addr, size_t size )
{
	/* Equality */
	if( sctk_ib_mmu_entry_contains( entry, addr, size ) )
		return 1;
	
	void * A1 = addr;
	void * A2 = addr + size;

	void * B1 = entry->addr;
	void * B2 = entry->addr + entry->size;
	
	/* A1       A2
	 *      B1         B2 */
	if( ( B1 < A2 )
	&&  ( A1 <= B1 ) )
	{
		return 1;
	}
	
	
	/*            A1           A2
	 * B1             B2 */
	if( (B2 < A2  )
	&&  (A1 <= B2 ) )
	{
		return 1;
	}
	
	return 0;
}

void sctk_ib_mmu_entry_acquire( sctk_ib_mmu_entry_t * entry )
{
	if( !entry )
		return;
	
	sctk_spinlock_read_lock( &entry->entry_refcounter );
}

void sctk_ib_mmu_entry_relax( sctk_ib_mmu_entry_t * entry )
{
	if( !entry )
		return;
	
	sctk_spinlock_read_unlock( &entry->entry_refcounter );
}



void _sctk_ib_mmu_init( struct sctk_ib_mmu * mmu )
{
	struct sctk_runtime_config * config = sctk_runtime_config_get();
	struct sctk_runtime_config_struct_ib_global * ib_global_config = &config->modules.low_level_comm.ib_global;
	
	/* Clear the MMU (particularly the fast cache) */
	memset( mmu, 0, sizeof( struct sctk_ib_mmu ) );
	
	mmu->cache_lock = 0;
	
	mmu->cache_enabled = ib_global_config->mmu_cache_enabled;
	
	if( mmu->cache_enabled )
	{
		mmu->cache_max_size = ib_global_config->mmu_cache_size_global;
		
		sctk_nodebug("CACHE IS %d", mmu->cache_max_size );
		
		assume( mmu->cache_max_size != 0 );
		mmu->cache = sctk_calloc( mmu->cache_max_size , sizeof( sctk_ib_mmu_entry_t * ));
	}
	else
	{
		mmu->cache_max_size = 0;
		mmu->cache = NULL;
	}
	
}

void _sctk_ib_mmu_release( struct sctk_ib_mmu * mmu )
{
	int i;

	
	memset( mmu, 0, sizeof( struct sctk_ib_mmu ) );
	
}


static inline int __sctk_ib_mmu_get_entry_containing( sctk_ib_mmu_entry_t * entry, void * addr, size_t size )
{
	/* If cell Present */
	if( entry )
	{
	
		/* Is the request within the target ? */
		if( sctk_ib_mmu_entry_contains( entry, addr, size ) )
		{
			/* Yes, then return */
			return 1;
		}
	
	}
	
	return 0;
}


sctk_ib_mmu_entry_t * sctk_ib_mmu_get_entry_containing_no_lock( struct sctk_ib_mmu * mmu, void * addr, size_t size )
{
	int i;

	for( i = 0 ; i < mmu->cache_max_size ; i++ )
	{
		sctk_ib_mmu_entry_acquire( mmu->cache[i] );

		if( __sctk_ib_mmu_get_entry_containing( mmu->cache[i], addr, size ) )
		{
			/* note that here the cell is held in read and not relaxed */
			return mmu->cache[i];
		}

		sctk_ib_mmu_entry_relax( mmu->cache[i] );
	}
	
	return NULL;
}


sctk_ib_mmu_entry_t * _sctk_ib_mmu_get_entry_containing( struct sctk_ib_mmu * mmu, void * addr, size_t size )
{
	/* No cache means no HIT */
	if( !mmu->cache_enabled )
		return NULL;
	
	sctk_ib_mmu_entry_t * ret = NULL;
		/* Lock the MMU */
	sctk_spinlock_lock( &mmu->cache_lock );
	
	ret = sctk_ib_mmu_get_entry_containing_no_lock( mmu, addr, size );
	
	sctk_spinlock_unlock( &mmu->cache_lock );
	
	return ret;
}



void _sctk_ib_mmu_push_entry( struct sctk_ib_mmu * mmu , sctk_ib_mmu_entry_t * entry )
{
	/* No cache means no storage */
	if( !mmu->cache_enabled )
		return;
	
	/* Warning YOU must enter here MMU LOCKED ! */
	
	int i;
	
	/* First try to register in fast cache */
	for( i = 0 ; i < mmu->cache_max_size ; i++ )
	{
		/* Cell is free */
		if( !mmu->cache[i] )
		{
			mmu->cache[i] = entry;
			/* We are done */
			return;
		}
	}
	
	/* If we are here all cells were in use
	 * try to free a random one and return */

	int start_point = rand() % mmu->cache_max_size;

	for( i = start_point ; i <  ( start_point + mmu->cache_max_size) ; i++ )
	{
		int index = i % mmu->cache_max_size;
		
		/* Cell is free */
		if( !mmu->cache[index] )
		{
			/* Try to find entries with no current reader */
			if( sctk_atomics_load_int( &mmu->cache[index]->entry_refcounter.reader_number) == 0 )
			{
				sctk_ib_mmu_entry_release( mmu->cache[index] );
				mmu->cache[i] = entry;
				return;
			}
		}
	}
	
	
	/* If we get here we put nothing in the cache as no entries were free */
	
}


sctk_ib_mmu_entry_t * _sctk_ib_mmu_pin(  struct sctk_ib_mmu * mmu,  sctk_ib_rail_info_t *rail_ib, void * addr, size_t size)
{
	sctk_ib_mmu_entry_t * entry = NULL;
	
	/* Look first in local then go up the cache */

	entry = _sctk_ib_mmu_get_entry_containing( mmu, addr, size );
	
	if( entry )
	{
			/* Value is already from local, just return */
			return entry;
	}

	/* If we are here we did not find an entry */
	/* Lock root */
	sctk_spinlock_lock( &mmu->cache_lock );

	/* Create a new entry pinning this area */
	entry = sctk_ib_mmu_entry_new( rail_ib, addr, size );

	/* Push in the root */
	_sctk_ib_mmu_push_entry( mmu, entry );
	
	/* Acquire it in read */
	sctk_ib_mmu_entry_acquire( entry );


	/* Unlock root MMU */
	sctk_spinlock_unlock( &mmu->cache_lock );

	return entry;
}

/* We need this variable as some free inside this function possbily
 * trigger larger frees on pinned segments also triggering the hook
 * therefore, the solution is just to ignore the recursive segments */
__thread int __already_inside_unpin = 0;

int _sctk_ib_mmu_unpin(  struct sctk_ib_mmu * mmu, void * addr, size_t size)
{
	int ret = 1;

	if( __already_inside_unpin )
		return 0;

	/* Lock root */
	sctk_spinlock_lock( &mmu->cache_lock );
	
	__already_inside_unpin = 1;
	
	/* Now release intersecting cells */
	int i;

	for( i = 0 ; i < mmu->cache_max_size ; i++ )
	{
		/* Cell is in use */
		if( mmu->cache[i] )
		{
			/* If cell points to a valid entry (is normaly guaranteed by locking) */
	
			/* If entry intersects with the freed segment */
			if( sctk_ib_mmu_entry_intersects( mmu->cache[i],  addr, size ) )
			{
				/* Free content */
				sctk_ib_mmu_entry_release( mmu->cache[i] );
				mmu->cache[i] = NULL;
				ret = 0;
			}
		
		}
	}
	
	/* Unlock root MMU */
	sctk_spinlock_unlock( &mmu->cache_lock );
	
	__already_inside_unpin = 0;
	
	return ret;
}



/* MMU Topological Interface */
static struct sctk_ib_mmu __main_ib_mmu;
volatile int __main_ib_mmu_init_done = 0;

__thread struct sctk_ib_mmu * __per_kernel_thread_ib_mmu = NULL;


void sctk_ib_mmu_init()
{
	_sctk_ib_mmu_init( &__main_ib_mmu );
	__main_ib_mmu_init_done = 1;
}

void sctk_ib_mmu_release()
{
	_sctk_ib_mmu_release( &__main_ib_mmu );	
}


sctk_ib_mmu_entry_t * sctk_ib_mmu_pin( sctk_ib_rail_info_t *rail_ib, void * addr, size_t size)
{
	return _sctk_ib_mmu_pin( &__main_ib_mmu,  rail_ib, addr, size);
}

void sctk_ib_mmu_relax( sctk_ib_mmu_entry_t * handler )
{
	if( !handler )
		return;

	sctk_ib_mmu_entry_relax( handler );
}

int sctk_ib_mmu_unpin(  void * addr, size_t size )
{
	if( ! __main_ib_mmu_init_done )
		return 0;
	
	return _sctk_ib_mmu_unpin(  &__main_ib_mmu, addr, size);
}






/** TESTS **/


static int init( struct sctk_ib_mmu * root )
{
	_sctk_ib_mmu_init( root );
	
	return 0;
}

static int release( struct sctk_ib_mmu * root )
{
	_sctk_ib_mmu_release( root );
	
	return 0;
}

static int init_and_release( struct sctk_ib_mmu * root )
{
	init( root );
	release( root);
	
	return 0;
}


#define ENTRY_COUNT 500

static int store_onethousand_then_retrieve_centers_on_root( struct sctk_ib_mmu * root )
{
	init( root );
	
	void * pointers[ENTRY_COUNT];
	
	int i;
	
	for( i = 0 ; i < ENTRY_COUNT ; i++ )
	{
		pointers[i] = sctk_malloc( 64 );

		//sctk_error("Push %p", pointers[i]);
		sctk_ib_mmu_entry_t * ret = _sctk_ib_mmu_pin( root, NULL, pointers[i], 64 );
		
		if( ret == NULL )
		{
			return 1;
		}
		
		sctk_ib_mmu_entry_relax( ret );
		
	}
	
	for( i = 0 ; i < ENTRY_COUNT ; i++ )
	{
		sctk_ib_mmu_entry_t * ent = _sctk_ib_mmu_get_entry_containing( root, pointers[i] , 8 );
		
		//sctk_error("Get %p  -- %p", pointers[i], ent);
		
		if( ent == NULL )
			return 1;
		
		sctk_ib_mmu_entry_relax( ent );
	}
	
	
	release( root );
	
	return 0;
}



static int pin_unpin_on_root( struct sctk_ib_mmu * root)
{
	init( root );
	
	void * pointer = malloc( 128 );
	
	
	sctk_ib_mmu_entry_t * ret = _sctk_ib_mmu_pin( root, NULL, pointer, 128 );
	
	if( ret == NULL )
	{
		return 1;
	}
	
	sctk_ib_mmu_entry_relax( ret );

	
	_sctk_ib_mmu_unpin( root, pointer + 8, 8 );


	sctk_ib_mmu_entry_t * ent = _sctk_ib_mmu_get_entry_containing( root, pointer , 8 );

	if( ent != NULL )
		return 1;

	release( root );
	
	return 0;
}



void test_topological_mmu()
{
	
	struct sctk_ib_mmu root;
	
	if( init_and_release( &root ) )
	{
		sctk_fatal("init_and_release");
	}
	
	if( store_onethousand_then_retrieve_centers_on_root( &root ) )
	{
		sctk_fatal("store_onethousand_then_retrieve_centers_on_root");
	}
	
	if( pin_unpin_on_root( &root ) )
	{
		sctk_fatal("pin_unpin_on_root");
	}

}


