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
#include "mpc_conf.h"
#include "mpc_common_asm.h"

#include <infiniband/verbs.h>
#include <stdlib.h>
#include <errno.h>
#include <mpc_common_debug.h>
#include <sctk_alloc.h>
#include <sctk_alloc.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>

#include "lowcomm_config.h"

#include <sctk_alloc.h>

_mpc_lowcomm_ib_mmu_entry_t * _mpc_lowcomm_ib_mmu_entry_new(sctk_ib_rail_info_t *rail_ib, void *addr, size_t size)
{
	_mpc_lowcomm_ib_mmu_entry_t *new = sctk_malloc(sizeof(_mpc_lowcomm_ib_mmu_entry_t) );

	assume(new != NULL);

	/* Save address size and rail */
	new->addr = addr;
	new->size = size;
	new->rail = rail_ib;

	struct _mpc_lowcomm_config *lowcomm_conf = _mpc_lowcomm_conf_get();
	const struct _mpc_lowcomm_config_struct_ib_global *ib_global_config = &lowcomm_conf->infiniband;


	if( (ib_global_config->mmu_cache_maximum_pin_size) < size)
	{
		mpc_common_debug_fatal("You have reached the maximum size of a pinned memory region(%g GB),"
		                       "consider splitting this large segment is several segments or \n"
		                       "increase the size limitation in the config depending on your IB card specs",
		                       ib_global_config->mmu_cache_maximum_pin_size / (1024.0 * 1024.0 * 1024.0) );
	}

	mpc_common_nodebug("NEW MMU ENTRY at %p size %ld", new->addr, new->size);

	/* Pin memory and save memory handle */
	if(rail_ib)
	{
		new->mr = ibv_reg_mr(rail_ib->device->pd, addr, size, IBV_ACCESS_REMOTE_WRITE
		                     | IBV_ACCESS_LOCAL_WRITE
		                     | IBV_ACCESS_REMOTE_READ
		                     | IBV_ACCESS_REMOTE_ATOMIC);
		assume(new->mr != NULL);
	}
	else
	{
		/* For tests */
		new->mr = NULL;
	}

	/* No one ows it yet */
	mpc_common_rwlock_t lckinit = MPC_COMMON_SPIN_RWLOCK_INITIALIZER;
	new->entry_refcounter = lckinit;

	new->free_on_relax = 0;

	return new;
}

void _mpc_lowcomm_ib_mmu_entry_release(_mpc_lowcomm_ib_mmu_entry_t *release)
{
	int ret = 0;

	mpc_common_nodebug("MMU UNPIN at %p size %ld", release->addr, release->size);

	/* Unregister memory */
	if(release->mr)
	{
		ret = ibv_dereg_mr(release->mr);
		if(ret)
		{
			mpc_common_debug_fatal("Failure to de-register the MR: %s (%p)", strerror(ret) );
		}
	}

	/* Empty element */
	release->addr = NULL;
	release->size = 0;

	/* Free entry */
	sctk_free(release);
}

int _mpc_lowcomm_ib_mmu_entry_contains(_mpc_lowcomm_ib_mmu_entry_t *entry, void *addr, size_t size)
{
	mpc_common_nodebug("Test %p (%ld) == %p (%ld)\n", addr, size, entry->addr, entry->size);

	if( (entry->addr <= addr) &&
	    ( (addr + size) <= (entry->addr + entry->size) ) )
	{
		return 1;
	}

	return 0;
}

int _mpc_lowcomm_ib_mmu_entry_intersects(_mpc_lowcomm_ib_mmu_entry_t *entry, void *addr, size_t size)
{
	/* Equality */
	if(_mpc_lowcomm_ib_mmu_entry_contains(entry, addr, size) )
	{
		return 1;
	}

	void *A1 = addr;
	void *A2 = addr + size;

	void *B1 = entry->addr;
	void *B2 = entry->addr + entry->size;

	/* A1       A2
	 *      B1         B2 */
	if( (B1 < A2) &&
	    (A1 <= B1) )
	{
		return 1;
	}


	/*            A1           A2
	 * B1             B2 */
	if( (B2 < A2) &&
	    (A1 <= B2) )
	{
		return 1;
	}

	return 0;
}

void _mpc_lowcomm_ib_mmu_entry_acquire(_mpc_lowcomm_ib_mmu_entry_t *entry)
{
	if(!entry)
	{
		return;
	}

	mpc_common_nodebug("ACQUIRING(%p) %p s %ld", entry, entry->addr, entry->size);

	mpc_common_spinlock_read_lock(&entry->entry_refcounter);
}

void _mpc_lowcomm_ib_mmu_entry_relax(_mpc_lowcomm_ib_mmu_entry_t *entry)
{
	if(!entry)
	{
		return;
	}

	mpc_common_nodebug("Entry RELAX %p", entry);


	mpc_common_spinlock_read_unlock(&entry->entry_refcounter);

	/* This is called when the entry was not pushed
	 * in the cache (case where all entries were in use
	 * this is really an edge case */
	if(entry->free_on_relax)
	{
		mpc_common_debug("Forced free on relax %p s %ld", entry->addr, entry->size);
		_mpc_lowcomm_ib_mmu_entry_release(entry);
	}
}

void __mpc_lowcomm_ib_mmu_init(struct _mpc_lowcomm_ib_mmu *mmu)
{
	struct _mpc_lowcomm_config *lowcomm_conf = _mpc_lowcomm_conf_get();
	const struct _mpc_lowcomm_config_struct_ib_global *ib_global_config = &lowcomm_conf->infiniband;

	/* Clear the MMU (particularly the fast cache) */
	memset(mmu, 0, sizeof(struct _mpc_lowcomm_ib_mmu) );

	sctk_spin_rwlock_init(&mmu->cache_lock);

	mmu->cache_enabled = ib_global_config->mmu_cache_enabled;

#ifndef MPC_Allocator
	mmu->cache_enabled = 0;
#endif

	if(mmu->cache_enabled)
	{
		mmu->cache_max_entry_count = ib_global_config->mmu_cache_entry_count;

		mpc_common_nodebug("CACHE IS %d", mmu->cache_max_entry_count);

		assume(mmu->cache_max_entry_count != 0);
		mmu->cache = sctk_calloc(mmu->cache_max_entry_count, sizeof(_mpc_lowcomm_ib_mmu_entry_t *) );
	}
	else
	{
		mmu->cache_max_entry_count = 0;
		mmu->cache = NULL;
	}


	mmu->cache_maximum_size = ib_global_config->mmu_cache_maximum_size;
	mmu->current_size = 0;
}

void __mpc_lowcomm_ib_mmu_release(struct _mpc_lowcomm_ib_mmu *mmu)
{
	memset(mmu, 0, sizeof(struct _mpc_lowcomm_ib_mmu) );
}

static inline int ___mpc_lowcomm_ib_mmu_get_entry_containing(_mpc_lowcomm_ib_mmu_entry_t *entry, void *addr, size_t size)
{
	/* If cell Present */
	if(entry)
	{
		/* Is the request within the target ? */
		if(_mpc_lowcomm_ib_mmu_entry_contains(entry, addr, size) )
		{
			/* Yes, then return */
			return 1;
		}
	}

	return 0;
}

_mpc_lowcomm_ib_mmu_entry_t *_mpc_lowcomm_ib_mmu_get_entry_containing_no_lock(struct _mpc_lowcomm_ib_mmu *mmu, void *addr, size_t size, sctk_ib_rail_info_t *rail_ib)
{
	unsigned int i;

	for(i = 0; i < mmu->cache_max_entry_count; i++)
	{
		_mpc_lowcomm_ib_mmu_entry_acquire(mmu->cache[i]);

		if(___mpc_lowcomm_ib_mmu_get_entry_containing(mmu->cache[i], addr, size) )
		{
			/* Do we have a rail constraint ?
			 * useful when pinning in multirail */
			if(rail_ib)
			{
				/* In this case we also check rail equality
				 * to make sure that we are pinned on the right
				 * device, requiring otherwise the addition */
				if(mmu->cache[i]->rail == rail_ib)
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

		_mpc_lowcomm_ib_mmu_entry_relax(mmu->cache[i]);
	}

	return NULL;
}

_mpc_lowcomm_ib_mmu_entry_t *__mpc_lowcomm_ib_mmu_get_entry_containing(struct _mpc_lowcomm_ib_mmu *mmu, void *addr, size_t size, sctk_ib_rail_info_t *rail_ib)
{
	/* No cache means no HIT */
	if(!mmu->cache_enabled)
	{
		return NULL;
	}

	_mpc_lowcomm_ib_mmu_entry_t *ret = NULL;
	/* Lock the MMU */
	mpc_common_spinlock_read_lock(&mmu->cache_lock);

	ret = _mpc_lowcomm_ib_mmu_get_entry_containing_no_lock(mmu, addr, size, rail_ib);

	mpc_common_spinlock_read_unlock(&mmu->cache_lock);

	return ret;
}

static inline int __mpc_lowcomm_ib_mmu_try_to_release_and_replace_entry(struct _mpc_lowcomm_ib_mmu *mmu, _mpc_lowcomm_ib_mmu_entry_t *entry)
{
	int start_point = rand() % mmu->cache_max_entry_count;

	unsigned int i;

	for(i = start_point; i < (start_point + mmu->cache_max_entry_count); i++)
	{
		int index = i % mmu->cache_max_entry_count;
		/* Cell is free */
		if(mmu->cache[index])
		{
			/* Try to find entries with no current reader */
			if(OPA_load_int(&mmu->cache[index]->entry_refcounter.reader_number) == 0)
			{
				/* Remove */
				mmu->current_size -= mmu->cache[index]->size;
				_mpc_lowcomm_ib_mmu_entry_release(mmu->cache[index]);
				/* Add */
				if(entry)
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

#define MMU_PUSH_MAX_TRIAL    50

void __mpc_lowcomm_ib_mmu_push_entry(struct _mpc_lowcomm_ib_mmu *mmu, _mpc_lowcomm_ib_mmu_entry_t *entry)
{
	/* No cache means no storage */
	if(!mmu->cache_enabled)
	{
		entry->free_on_relax = 1;
		return;
	}

	/* Check if we are not over the memory limit (note that current entry is already
	 * pinned at this moment this is why it also enters in the accounting  */
	while(mmu->cache_maximum_size <= (mmu->current_size + entry->size) )
	{
		/* Here we need to free some room (replacing by NULL) */
		__mpc_lowcomm_ib_mmu_try_to_release_and_replace_entry(mmu, NULL);
	}

	mpc_common_nodebug("Current MMU size %ld", mmu->current_size);

	/* Warning YOU must enter here MMU LOCKED ! */
	int trials = 0;

	while(trials < MMU_PUSH_MAX_TRIAL)
	{
		unsigned int i;

		/* First try to register in cache */
		for(i = 0; i < mmu->cache_max_entry_count; i++)
		{
			/* Cell is free */
			if(!mmu->cache[i])
			{
				/* Add */
				mmu->cache[i]      = entry;
				mmu->current_size += entry->size;

				/* We are done */
				return;
			}
		}

		/* If we are here all cells were in use
		 * try to free a random one and return */

		if(__mpc_lowcomm_ib_mmu_try_to_release_and_replace_entry(mmu, entry) )
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

_mpc_lowcomm_ib_mmu_entry_t *__mpc_lowcomm_ib_mmu_pin(struct _mpc_lowcomm_ib_mmu *mmu, sctk_ib_rail_info_t *rail_ib, void *addr, size_t size)
{
	_mpc_lowcomm_ib_mmu_entry_t *entry = NULL;

	/* Look first in local then go up the cache */

	entry = __mpc_lowcomm_ib_mmu_get_entry_containing(mmu, addr, size, rail_ib);

	if(entry)
	{
		/* Value is already from local, just return */
		return entry;
	}


	/* Create a new entry pinning this area */
	entry = _mpc_lowcomm_ib_mmu_entry_new(rail_ib, addr, size);


	/* If we are here we did not find an entry */
	/* Lock root */
	mpc_common_spinlock_write_lock(&mmu->cache_lock);

	/* Note that we do not check if the entry
	 * has been pushed in the meantime as
	 * we consider it improbable, moreover,
	 * it is legal to pin the same area twice */

	/* Push in the root */
	__mpc_lowcomm_ib_mmu_push_entry(mmu, entry);

	/* Acquire it in read */
	_mpc_lowcomm_ib_mmu_entry_acquire(entry);

	/* If we are here we did not find an entry */
	/* Lock root */
	mpc_common_spinlock_write_unlock(&mmu->cache_lock);

	return entry;
}

/* We need this variable as some free inside this function possbily
 * trigger larger frees on pinned segments also triggering the hook
 * therefore, the solution is just to ignore the recursive segments */
__thread int __already_inside_unpin = 0;

int __mpc_lowcomm_ib_mmu_unpin(struct _mpc_lowcomm_ib_mmu *mmu, void *addr, size_t size)
{
	int ret = 1;

	if(__already_inside_unpin)
	{
		return 0;
	}

	/* Lock root */
	mpc_common_spinlock_write_lock(&mmu->cache_lock);

	__already_inside_unpin = 1;

	/* Now release intersecting cells */
	unsigned int i;

	for(i = 0; i < mmu->cache_max_entry_count; i++)
	{
		/* Cell is in use */
		if(mmu->cache[i])
		{
			/* If cell points to a valid entry (is normaly guaranteed by locking) */

			/* If entry intersects with the freed segment */
			if(_mpc_lowcomm_ib_mmu_entry_intersects(mmu->cache[i], addr, size) )
			{
				/* Free content */
				mmu->current_size -= mmu->cache[i]->size;
				_mpc_lowcomm_ib_mmu_entry_release(mmu->cache[i]);
				mmu->cache[i] = NULL;
				ret           = 0;
			}
		}
	}

	/* Unlock root MMU */
	mpc_common_spinlock_write_unlock(&mmu->cache_lock);

	__already_inside_unpin = 0;

	return ret;
}

/* MMU Topological Interface */
static struct _mpc_lowcomm_ib_mmu __main_ib_mmu;
volatile int __main_ib_mmu_init_done = 0;

void _mpc_lowcomm_ib_mmu_init()
{
	/* Only init once */
	if(__main_ib_mmu_init_done)
	{
		return;
	}

	__mpc_lowcomm_ib_mmu_init(&__main_ib_mmu);
	__main_ib_mmu_init_done = 1;
}

void _mpc_lowcomm_ib_mmu_release()
{
	__mpc_lowcomm_ib_mmu_release(&__main_ib_mmu);
	__main_ib_mmu_init_done = 0;
}

_mpc_lowcomm_ib_mmu_entry_t *_mpc_lowcomm_ib_mmu_pin(sctk_ib_rail_info_t *rail_ib, void *addr, size_t size)
{
	mpc_common_nodebug("Pin %p side %ld", addr, size);
	return __mpc_lowcomm_ib_mmu_pin(&__main_ib_mmu, rail_ib, addr, size);
}

void _mpc_lowcomm_ib_mmu_relax(_mpc_lowcomm_ib_mmu_entry_t *handler)
{
	if(!handler)
	{
		return;
	}

	_mpc_lowcomm_ib_mmu_entry_relax(handler);
}

int _mpc_lowcomm_ib_mmu_unpin(void *addr, size_t size)
{
	if(!__main_ib_mmu_init_done)
	{
		return 0;
	}

	mpc_common_nodebug("UNPIN %p side %ld", addr, size);

	return __mpc_lowcomm_ib_mmu_unpin(&__main_ib_mmu, addr, size);
}
