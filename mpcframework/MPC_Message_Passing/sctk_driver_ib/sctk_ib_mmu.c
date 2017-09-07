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
#ifdef MPC_USE_INFINIBAND
#include "sctk_ib_mmu.h"
#include "sctk_ib_device.h"
#include "sctk_runtime_config.h"
#include "sctk_atomics.h"

#include <infiniband/verbs.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>

#include <sctk_alloc.h>

sctk_ib_mmu_entry_t * sctk_ib_mmu_entry_new( sctk_ib_rail_info_t *rail_ib, void * addr, size_t size )
{
	sctk_ib_mmu_entry_t * new = sctk_malloc( sizeof( sctk_ib_mmu_entry_t ) );

	assume( new != NULL );
	
	/* Save address size and rail */
	new->addr = addr;
	new->size = size;
	new->rail = rail_ib;

	const struct sctk_runtime_config * config = sctk_runtime_config_get();
	const struct sctk_runtime_config_struct_ib_global * ib_global_config = &config->modules.low_level_comm.ib_global;

	
	if( (ib_global_config->mmu_cache_maximum_pin_size) < size )
	{
		sctk_fatal("You have reached the maximum size of a pinned memory region(%g GB),"
				   "consider splitting this large segment is several segments or \n"
				   "increase the size limitation in the config depending on your IB card specs",
		           ib_global_config->mmu_cache_maximum_pin_size / (1024.0 * 1024.0 * 1024.0));
	}
	
	sctk_nodebug("NEW MMU ENTRY at %p size %ld", new->addr, new->size );
	
	/* Pin memory and save memory handle */
	if( rail_ib )
	{
		new->mr = ibv_reg_mr ( rail_ib->device->pd, addr, size, IBV_ACCESS_REMOTE_WRITE
																 | IBV_ACCESS_LOCAL_WRITE
																 | IBV_ACCESS_REMOTE_READ
																 | IBV_ACCESS_REMOTE_ATOMIC );
        assume(new->mr != NULL);
	}
	else
	{
		/* For tests */
		new->mr = NULL;
	}

	/* No one ows it yet */
	sctk_spin_rwlock_t lckinit = SCTK_SPIN_RWLOCK_INITIALIZER;
	new->entry_refcounter = lckinit;

	new->free_on_relax = 0;

	return new;
}

void sctk_ib_mmu_entry_release( sctk_ib_mmu_entry_t * release )
{
	int ret = 0;
	
	sctk_nodebug("MMU UNPIN at %p size %ld", release->addr, release->size );
	
	/* Unregister memory */
	if( release->mr )
	{
		ret = ibv_dereg_mr ( release->mr );
		if(ret)
		{
			sctk_fatal ( "Failure to de-register the MR: %s (%p)", strerror ( ret ) );
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
	&&  ( (addr + size) <= (entry->addr + entry->size) ) )
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
	
	sctk_nodebug("ACQUIRING(%p) %p s %ld", entry, entry->addr, entry->size );
	
	sctk_spinlock_read_lock( &entry->entry_refcounter );
}

void sctk_ib_mmu_entry_relax( sctk_ib_mmu_entry_t * entry )
{

	if( !entry )
		return;

	sctk_nodebug("Entry RELAX %p", entry );
	
	
	sctk_spinlock_read_unlock( &entry->entry_refcounter );
	
	/* This is called when the entry was not pushed 
	 * in the cache (case where all entries were in use
	 * this is really an edge case */
	if( entry->free_on_relax )
	{
		sctk_debug("Forced free on relax %p s %ld", entry->addr, entry->size );
		sctk_ib_mmu_entry_release( entry );
	}
}



void _sctk_ib_mmu_init( struct sctk_ib_mmu * mmu )
{
	const struct sctk_runtime_config * config = sctk_runtime_config_get();
	const struct sctk_runtime_config_struct_ib_global * ib_global_config = &config->modules.low_level_comm.ib_global;
	
	/* Clear the MMU (particularly the fast cache) */
	memset( mmu, 0, sizeof( struct sctk_ib_mmu ) );
	
	sctk_spin_rwlock_init( &mmu->cache_lock );
	
	mmu->cache_enabled = ib_global_config->mmu_cache_enabled;

#ifndef MPC_Allocator
    mmu->cache_enabled = 0;
#endif
	
	if( mmu->cache_enabled )
	{
		mmu->cache_max_entry_count = ib_global_config->mmu_cache_entry_count;
		
		sctk_nodebug("CACHE IS %d", mmu->cache_max_entry_count );
		
		assume( mmu->cache_max_entry_count != 0 );
		mmu->cache = sctk_calloc( mmu->cache_max_entry_count , sizeof( sctk_ib_mmu_entry_t * ));
	}
	else
	{
		mmu->cache_max_entry_count = 0;
		mmu->cache = NULL;
	}
	

	mmu->cache_maximum_size = ib_global_config->mmu_cache_maximum_size;
	//17179869184llu;
	mmu->current_size = 0;
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


sctk_ib_mmu_entry_t * sctk_ib_mmu_get_entry_containing_no_lock( struct sctk_ib_mmu * mmu, void * addr, size_t size, sctk_ib_rail_info_t *rail_ib )
{
	int i;

	for( i = 0 ; i < mmu->cache_max_entry_count ; i++ )
	{
		sctk_ib_mmu_entry_acquire( mmu->cache[i] );

		if( __sctk_ib_mmu_get_entry_containing( mmu->cache[i], addr, size ) )
		{
			/* Do we have a rail constraint ?
			 * useful when pinning in multirail */
			if( rail_ib )
			{
				/* In this case we also check rail equality
				 * to make sure that we are pinned on the right
				 * device, requiring otherwise the addition */
				if( mmu->cache[i]->rail == rail_ib )
				{
					/* note that here the cell is held in read and not relaxed */
					return mmu->cache[i];
				}
			}
			else
			{
				/* No rail then just return the entry */
				
				/* note that here the cell is held in read and not relaxed */
				return mmu->cache[i];
			}
		}

		sctk_ib_mmu_entry_relax( mmu->cache[i] );
	}
	
	return NULL;
}


sctk_ib_mmu_entry_t * _sctk_ib_mmu_get_entry_containing( struct sctk_ib_mmu * mmu, void * addr, size_t size, sctk_ib_rail_info_t * rail_ib )
{
	/* No cache means no HIT */
	if( !mmu->cache_enabled )
		return NULL;
	
	sctk_ib_mmu_entry_t * ret = NULL;
		/* Lock the MMU */
	sctk_spinlock_read_lock( &mmu->cache_lock );
	
	ret = sctk_ib_mmu_get_entry_containing_no_lock( mmu, addr, size, rail_ib );
	
	sctk_spinlock_read_unlock( &mmu->cache_lock );
	
	return ret;
}

static inline int _sctk_ib_mmu_try_to_release_and_replace_entry( struct sctk_ib_mmu * mmu, sctk_ib_mmu_entry_t * entry )
{
	int start_point = rand() % mmu->cache_max_entry_count;

	int i;

	for( i = start_point ; i <  ( start_point + mmu->cache_max_entry_count) ; i++ )
	{
		int index = i % mmu->cache_max_entry_count;
		
		/* Cell is free */
		if( mmu->cache[index] )
		{
			/* Try to find entries with no current reader */
			if( sctk_atomics_load_int( &mmu->cache[index]->entry_refcounter.reader_number) == 0 )
			{
				/* Remove */
				mmu->current_size -= mmu->cache[index]->size;
				sctk_ib_mmu_entry_release( mmu->cache[index] );
				
				/* Add */
				if( entry )
				{
					mmu->current_size += entry->size;
				}
		
				mmu->cache[index] = entry;
				
				return 1;
			}
		}
	}
	
	return 0;
}

#define MMU_PUSH_MAX_TRIAL 50

void _sctk_ib_mmu_push_entry( struct sctk_ib_mmu * mmu , sctk_ib_mmu_entry_t * entry )
{
	/* No cache means no storage */
	if( !mmu->cache_enabled ){
        entry->free_on_relax = 1;
		return;
    }

	/* Check if we are not over the memory limit (note that current entry is already
	 * pinned at this moment this is why it also enters in the accounting  */
	while( mmu->cache_maximum_size <= ( mmu->current_size + entry->size ) )
	{
		/* Here we need to free some room (replacing by NULL) */
		 _sctk_ib_mmu_try_to_release_and_replace_entry( mmu, NULL );
	}

	sctk_nodebug("Current MMU size %ld", mmu->current_size );

	/* Warning YOU must enter here MMU LOCKED ! */
	int trials = 0;
	
	while( trials < MMU_PUSH_MAX_TRIAL )
	{
	
		int i;
		
		/* First try to register in cache */
		for( i = 0 ; i < mmu->cache_max_entry_count ; i++ )
		{
			/* Cell is free */
			if( !mmu->cache[i] )
			{
				/* Add */
				mmu->cache[i] = entry;
				mmu->current_size += entry->size;

				/* We are done */
				return;
			}
		}
		
		/* If we are here all cells were in use
		 * try to free a random one and return */
		 
		if( _sctk_ib_mmu_try_to_release_and_replace_entry( mmu, entry ) )
		{
			/* We are done */
			return;
		}
		
		trials++;
	}
	
	/* If we get here we put nothing in the cache as no entries were freed (all in use) */
	
	/* We store the fact that this entry will be freed on relax (this is a clear edge case
	 * which can happen on caches with a very small count in case of slot scarcity) */
	entry->free_on_relax = 1;
}


sctk_ib_mmu_entry_t * _sctk_ib_mmu_pin(  struct sctk_ib_mmu * mmu,  sctk_ib_rail_info_t *rail_ib, void * addr, size_t size)
{
	sctk_ib_mmu_entry_t * entry = NULL;
	
	/* Look first in local then go up the cache */

	entry = _sctk_ib_mmu_get_entry_containing( mmu, addr, size, rail_ib );
	
	if( entry )
	{
		/* Value is already from local, just return */
		return entry;
	}


	/* Create a new entry pinning this area */
	entry = sctk_ib_mmu_entry_new( rail_ib, addr, size );


	/* If we are here we did not find an entry */
	/* Lock root */
	sctk_spinlock_write_lock( &mmu->cache_lock );

	/* Note that we do not check if the entry
	 * has been pushed in the meantime as
	 * we consider it improbable, moreover,
	 * it is legal to pin the same area twice */
	
	/* Push in the root */
	_sctk_ib_mmu_push_entry( mmu, entry );
	
	/* Acquire it in read */
	sctk_ib_mmu_entry_acquire( entry );

	/* If we are here we did not find an entry */
	/* Lock root */
	sctk_spinlock_write_unlock( &mmu->cache_lock );
	
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
	sctk_spinlock_write_lock( &mmu->cache_lock );
	
	__already_inside_unpin = 1;
	
	/* Now release intersecting cells */
	int i;

	for( i = 0 ; i < mmu->cache_max_entry_count ; i++ )
	{
		/* Cell is in use */
		if( mmu->cache[i] )
		{
			/* If cell points to a valid entry (is normaly guaranteed by locking) */
	
			/* If entry intersects with the freed segment */
			if( sctk_ib_mmu_entry_intersects( mmu->cache[i],  addr, size ) )
			{
				/* Free content */
				mmu->current_size -= mmu->cache[i]->size;
				sctk_ib_mmu_entry_release( mmu->cache[i] );
				mmu->cache[i] = NULL;
				ret = 0;
			}
		
		}
	}
	
	/* Unlock root MMU */
	sctk_spinlock_write_unlock( &mmu->cache_lock );
	
	__already_inside_unpin = 0;
	
	return ret;
}



/* MMU Topological Interface */
static struct sctk_ib_mmu __main_ib_mmu;
volatile int __main_ib_mmu_init_done = 0;

void sctk_ib_mmu_init()
{
	/* Only init once */
	if( __main_ib_mmu_init_done )
		return;
	
	_sctk_ib_mmu_init( &__main_ib_mmu );
	__main_ib_mmu_init_done = 1;
}

void sctk_ib_mmu_release()
{
	_sctk_ib_mmu_release( &__main_ib_mmu );
	__main_ib_mmu_init_done = 0;
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
		sctk_ib_mmu_entry_t * ent = _sctk_ib_mmu_get_entry_containing( root, pointers[i] , 8 , NULL);
		
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


	sctk_ib_mmu_entry_t * ent = _sctk_ib_mmu_get_entry_containing( root, pointer , 8 , NULL );

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


#endif
