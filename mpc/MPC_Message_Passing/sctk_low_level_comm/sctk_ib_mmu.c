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
/* #   - DIDELOT Sylvain didelot.sylvain@gmail.com                        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include "sctk_ib_mmu.h"
#include <infiniband/verbs.h>
#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "MMU"
#include "sctk_ib_toolkit.h"
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "utlist.h"


/**
 *  Description:
 *
 *
 */

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

/*-----------------------------------------------------------
 *  CACHE
 *----------------------------------------------------------*/
void sctk_ib_mmu_cache_init(sctk_ib_rail_info_t *rail_ib){
  LOAD_MMU(rail_ib);

  mmu->cache.entries = NULL;
  mmu->cache.lock = SCTK_SPINLOCK_INITIALIZER;
  mmu->cache.entries_nb = 0;
}

/* Remove the Last Recently Used entry */
sctk_ib_mmu_entry_t*
 sctk_ib_mmu_cache_remove(sctk_ib_rail_info_t *rail_ib) {
   LOAD_MMU(rail_ib);
  sctk_ib_mmu_entry_t* mmu_entry;

  /* Lock already taken. */
#ifdef DEBUG_IB_MMU
  assume(mmu->cache.entries);
  assume(mmu->cache.entries->prev);
#endif
  /* select the LRU entry */
  mmu_entry = mmu->cache.entries->prev;
  /* If last entry still used, we cannot add another entry */
  sctk_nodebug(">>>>>> remove nb reg: %d (%p)",mmu_entry->registrations_nb, mmu_entry);
  if (mmu_entry->registrations_nb > 0) return NULL;
  sctk_nodebug("Entry %p removed from cache", mmu_entry);
  DL_DELETE(mmu->cache.entries, mmu_entry);
#ifdef DEBUG_IB_MMU
  assume(mmu_entry);
#endif
  mmu_entry->cache_status = not_cached;
  mmu->cache.entries_nb--;
  return mmu_entry;
}


int
 sctk_ib_mmu_cache_add(sctk_ib_rail_info_t *rail_ib,
    sctk_ib_mmu_entry_t* mmu_entry) {
  LOAD_MMU(rail_ib);
  LOAD_CONFIG(rail_ib);

  sctk_nodebug("Try to register (%p) %p %lu to cache %d", mmu_entry, mmu_entry->ptr, mmu_entry->size, mmu->cache.entries_nb );
  sctk_spinlock_lock(&mmu->cache.lock);
  /* Maximum number of cached entries reached */
  if (mmu->cache.entries_nb == config->ibv_mmu_cache_entries) {
    sctk_ib_mmu_entry_t* mmu_entry_to_remove;
    mmu_entry_to_remove = sctk_ib_mmu_cache_remove(rail_ib);
    if (!mmu_entry_to_remove) {
      sctk_spinlock_unlock(&mmu->cache.lock);
      return 0;
    }
    sctk_ib_mmu_unregister (rail_ib, mmu_entry_to_remove);
  }
#ifdef DEBUG_IB_MMU
  assume(mmu->cache.entries_nb < config->ibv_mmu_cache_entries);
#endif
  sctk_nodebug("Add entry %p to cache %p %lu", mmu_entry, mmu_entry->ptr, mmu_entry->size);
  mmu->cache.entries_nb++;
  mmu_entry->cache_status = cached;
  assume(mmu_entry->registrations_nb == 0);
  mmu_entry->registrations_nb++;
  sctk_nodebug("<<<<<<<< add nb reg: %d (%p)",mmu_entry->registrations_nb, mmu_entry);
  DL_PREPEND(mmu->cache.entries, mmu_entry);
  sctk_spinlock_unlock(&mmu->cache.lock);
  return 1;
}

sctk_ib_mmu_entry_t*
 sctk_ib_mmu_cache_search(sctk_ib_rail_info_t *rail_ib,
    void *ptr, size_t size) {
  LOAD_MMU(rail_ib);
  sctk_ib_mmu_entry_t *mmu_entry, *tmp;

  sctk_spinlock_lock(&mmu->cache.lock);
  DL_FOREACH_SAFE(mmu->cache.entries, mmu_entry, tmp) {
    if ( ptr >= mmu_entry->ptr ) {
      size_t offset;
      offset = ((unsigned long) ptr - (unsigned long) mmu_entry->ptr);
      /* entry found */
      if (offset + size <= mmu_entry->size) {
        sctk_nodebug("Entry %p reused (%p - %lu)", mmu_entry, ptr, size);
        /* Put the entry at the beginning of the array.
         * We check if the element is actually not the first element */
        if (mmu->cache.entries != mmu_entry) {
          DL_DELETE(mmu->cache.entries, mmu_entry);
          DL_PREPEND(mmu->cache.entries, mmu_entry);
        }
        mmu_entry->registrations_nb++;
        sctk_nodebug("<<<<<<<< search nb reg: %d (%p)",mmu_entry->registrations_nb, mmu_entry);
        sctk_spinlock_unlock(&mmu->cache.lock);
        return mmu_entry;
      }
    }
  }
  sctk_spinlock_unlock(&mmu->cache.lock);
  return NULL;
}


/*-----------------------------------------------------------
 *  MMU
 *----------------------------------------------------------*/
void
check_nb_entries(sctk_ib_rail_info_t *rail_ib, const unsigned int nb_entries)
{
  sctk_ib_mmu_t* mmu = rail_ib->mmu;
  sctk_ib_config_t *config = rail_ib->config;

  if (mmu->nb + nb_entries > config->device_attr->max_mr)
  {
    sctk_ib_debug("Cannot allocate more MMU entries: hardware limit %d reached",
          config->device_attr->max_mr);
    /* FIXME: behavior needed there */
    not_implemented();
  }
}

 void
sctk_ib_mmu_init(struct sctk_ib_rail_info_s *rail_ib)
{
  LOAD_CONFIG(rail_ib);
  sctk_ib_mmu_t* mmu;

  mmu = sctk_malloc (sizeof(sctk_ib_mmu_t));
  memset(mmu, 0, sizeof(sctk_ib_mmu_t));
  assume(mmu);
  rail_ib->mmu = mmu;

  if (config->ibv_mmu_cache_enabled) {
    sctk_ib_mmu_cache_init(rail_ib);
  }
  sctk_ib_mmu_alloc(rail_ib, rail_ib->config->ibv_init_mr);
}

 void
sctk_ib_mmu_alloc(struct sctk_ib_rail_info_s *rail_ib, const unsigned int nb_entries)
{
  int i;
  sctk_ib_mmu_t* mmu = rail_ib->mmu;
  sctk_ib_mmu_region_t* region = NULL;
  sctk_ib_mmu_entry_t* mmu_entry;
  sctk_ib_mmu_entry_t* mmu_entry_ptr;

  region = sctk_malloc (sizeof(sctk_ib_mmu_region_t));
  assume(region);
  memset(region, 0, sizeof(sctk_ib_mmu_region_t));

  mmu_entry = sctk_malloc(sizeof(sctk_ib_mmu_entry_t) * nb_entries);
  assume(mmu_entry);
  memset(mmu_entry, 0, sizeof(sctk_ib_mmu_entry_t) * nb_entries);

  /* Init region */
  region->mmu_entry = mmu_entry;
  DL_APPEND(mmu->regions, region);
  mmu->page_size = getpagesize();
  mmu->lock = SCTK_SPINLOCK_INITIALIZER;
  mmu->free_nb += nb_entries;

  for (i=0; i < nb_entries; ++i) {
    mmu_entry_ptr = mmu_entry + i;
    mmu_entry_ptr->region = region;
    DL_APPEND(mmu->free_entry,
        ((sctk_ib_mmu_entry_t*) mmu_entry + i));
    mmu_entry_ptr->status = ibv_entry_free;
    mmu_entry_ptr->cache_status = not_cached;
    mmu_entry_ptr->registrations_nb = 0;
  }

  mmu->nb += nb_entries;
  sctk_ib_debug("%d MMU entries allocated (total:%d, free:%d)", nb_entries, mmu->nb, mmu->free_nb);
}

static inline sctk_ib_mmu_entry_t *
__mmu_register ( sctk_ib_rail_info_t *rail_ib,
    void *ptr, size_t size, const char in_cache) {
  LOAD_MMU(rail_ib);
  LOAD_DEVICE(rail_ib);
  LOAD_CONFIG(rail_ib);
  sctk_ib_mmu_entry_t* mmu_entry;

#ifdef DEBUG_IB_MMU
  if ((uintptr_t) ptr % mmu->page_size) {
    sctk_error("MMU ptr is not aligned on page_size");
    sctk_abort();
  }
#endif
  sctk_nodebug("Try to pick entry %p %lu", ptr, size);

  if (in_cache && config->ibv_mmu_cache_enabled) {
    mmu_entry = sctk_ib_mmu_cache_search(rail_ib, ptr, size);
    if (mmu_entry) {
      return mmu_entry;
    }
  }

  sctk_spinlock_lock (&mmu->lock);
  /* No entry available */
  if (!mmu->free_entry) {
    /* Allocate more MMU entries */
    sctk_ib_mmu_alloc(rail_ib, config->ibv_size_mr_chunk);
  }
  mmu_entry = mmu->free_entry;
  /* pop the first element */
  DL_DELETE(mmu->free_entry, mmu->free_entry);
  mmu->free_nb--;
  sctk_ib_nodebug("Entry reserved (free_nb:%d size:%lu)", mmu->free_nb, size);
  sctk_spinlock_unlock (&mmu->lock);
  mmu_entry->status = ibv_entry_used;

  mmu_entry->mr = ibv_reg_mr (device->pd,
      ptr, size,
      IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
      IBV_ACCESS_REMOTE_READ);

#ifdef DEBUG_IB_MMU
  if (!mmu_entry->mr) {
    sctk_error ("Cannot register adm MR.");
    sctk_abort();
  }
#endif
  mmu_entry->ptr    = ptr;
  mmu_entry->size   = size;

  if (in_cache && config->ibv_mmu_cache_enabled)
    sctk_ib_mmu_cache_add(rail_ib, mmu_entry);

  sctk_nodebug("Entry %p registered", mmu_entry);
  return mmu_entry;
}


/* Register a new memory */
sctk_ib_mmu_entry_t *sctk_ib_mmu_register (
    sctk_ib_rail_info_t *rail_ib,
    void *ptr, size_t size) {
  return __mmu_register(rail_ib, ptr, size, 1);
}
sctk_ib_mmu_entry_t *sctk_ib_mmu_register_no_cache (
    sctk_ib_rail_info_t *rail_ib,
    void *ptr, size_t size) {
  return __mmu_register(rail_ib, ptr, size, 0);
}

void
sctk_ib_mmu_unregister (sctk_ib_rail_info_t *rail_ib,
    sctk_ib_mmu_entry_t *mmu_entry)
{
  LOAD_MMU(rail_ib);
  LOAD_CONFIG(rail_ib);

  if (config->ibv_mmu_cache_enabled) {
    if (mmu_entry->cache_status == cached) {
      sctk_spinlock_lock (&mmu->cache.lock);
      mmu_entry->registrations_nb--;
      sctk_nodebug("Release entry %p (nb:%d)", mmu_entry, mmu_entry->registrations_nb);
      sctk_spinlock_unlock (&mmu->cache.lock);
      return;
    }
  }
  assume (mmu_entry->registrations_nb == 0);
  sctk_nodebug("Entry %p unregistered", mmu_entry);

  mmu_entry->ptr = NULL;
  mmu_entry->size = 0;
  mmu_entry->status = ibv_entry_free;
  assume (ibv_dereg_mr (mmu_entry->mr) == 0);

  sctk_spinlock_lock (&mmu->lock);
  mmu->free_nb++;
  DL_PREPEND(mmu->free_entry,mmu_entry);
  sctk_spinlock_unlock (&mmu->lock);
}

#endif
