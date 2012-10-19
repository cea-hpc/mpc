/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
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
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_INFINIBAND
#include <infiniband/verbs.h>
#include "sctk_ib_mmu.h"
#include "sctk_ib.h"
#include "sctk_ib_config.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_prof.h"
#include "utlist.h"
#include "uthash.h"

/* IB debug macros */
#if defined SCTK_IB_MODULE_NAME
#error "SCTK_IB_MODULE already defined"
#endif
//#define SCTK_IB_MODULE_DEBUG
#define SCTK_IB_MODULE_NAME "MMU"
#include "sctk_ib_toolkit.h"


/**
 *  Description:
 *  TODO:
 *  * use the clock algorithm to select the LRU entry
 *  * add hooks in the allocator to notice the MMU that
 *    a memory has been freed
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

  mmu->cache.ht_entries = NULL;
  mmu->cache.lru_entries = NULL;
  mmu->cache.lock = SCTK_SPINLOCK_INITIALIZER;
  mmu->cache.entries_nb = 0;
}

/* Remove the Last Recently Used entry from the
 * pinned cached entries.
 * When call this function, the lock has already been
 * taken */
static inline sctk_ib_mmu_entry_t*
 sctk_ib_mmu_cache_remove(sctk_ib_rail_info_t *rail_ib) {
   LOAD_MMU(rail_ib);
  sctk_ib_mmu_entry_t* mmu_entry;

#ifdef DEBUG_IB_MMU
  assume(mmu->cache.ht_entries);
  assume(mmu->cache.ht_entries->prev);
#endif

  /* select the LRU entry */
  mmu_entry = mmu->cache.lru_entries->prev;
  sctk_nodebug(">>>>>> remove nb reg: %d (%p)",mmu_entry->registrations_nb, mmu_entry);

  /* If last entry still used, we cannot add another entry */
  if (mmu_entry->registrations_nb > 0) return NULL;

  /* Remove the entry from HashTable and LRU */
  DL_DELETE(mmu->cache.lru_entries, mmu_entry);
  HASH_DEL(mmu->cache.ht_entries, mmu_entry);
  sctk_nodebug("Entry %p removed from cache", mmu_entry);
#ifdef DEBUG_IB_MMU
  assume(mmu_entry);
#endif
  mmu_entry->cache_status = not_cached;
  mmu->cache.entries_nb--;
  return mmu_entry;
}

/* Add a cache entry */
static inline int
 sctk_ib_mmu_cache_add(sctk_ib_rail_info_t *rail_ib,
    sctk_ib_mmu_entry_t* mmu_entry) {
  LOAD_MMU(rail_ib);
  LOAD_CONFIG(rail_ib);

  sctk_nodebug("Try to register (%p) %p %lu to cache %d", mmu_entry, mmu_entry->key.ptr, mmu_entry->key.size, mmu->cache.entries_nb );
  sctk_spinlock_lock(&mmu->cache.lock);
  /* Maximum number of cached entries reached */
  if (mmu->cache.entries_nb >= config->ibv_mmu_cache_entries) {
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
  assume(mmu_entry->registrations_nb == 0);
#endif
  sctk_nodebug("Add entry %p to cache %p %lu", mmu_entry, mmu_entry->key.ptr, mmu_entry->key.size);
  mmu->cache.entries_nb++;
  mmu_entry->cache_status = cached;

  /* Add the entry */
  HASH_ADD(hh, mmu->cache.ht_entries, key, sizeof(sctk_ib_mmu_ht_key_t), mmu_entry);
  DL_PREPEND(mmu->cache.lru_entries, mmu_entry);

  mmu_entry->registrations_nb++;
  sctk_spinlock_unlock(&mmu->cache.lock);
  return 1;
}

/* Search for a cached entry according to the ptr and
 * the size of the buffer */
sctk_ib_mmu_entry_t*
 sctk_ib_mmu_cache_search(sctk_ib_rail_info_t *rail_ib,
    void *ptr, size_t size) {
  LOAD_MMU(rail_ib);
  sctk_ib_mmu_entry_t *mmu_entry = NULL;
  sctk_ib_mmu_ht_key_t key;

  /* Construct the key */
  key.ptr = ptr;
  key.size = size;

  sctk_spinlock_lock(&mmu->cache.lock);
  HASH_FIND(hh,mmu->cache.ht_entries, &key,sizeof(sctk_ib_mmu_ht_key_t),mmu_entry);
  if (mmu_entry) {
#ifdef DEBUG_IB_MMU
    assume(mmu_entry->cache_status == cached);
    assume(mmu_entry->key.ptr == key.ptr);
    assume(mmu_entry->key.size == key.size);
#endif
    /* If the entry is not the head of the list */
    if (mmu->cache.lru_entries != mmu_entry) {
      DL_DELETE(mmu->cache.lru_entries, mmu_entry);
      DL_PREPEND(mmu->cache.lru_entries, mmu_entry);
    }
    mmu_entry->registrations_nb++;
  }
  sctk_spinlock_unlock(&mmu->cache.lock);
  return mmu_entry;
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

  sctk_nodebug("Try to pick entry %p %lu", ptr, size);

  if (in_cache && config->ibv_mmu_cache_enabled) {
    mmu_entry = sctk_ib_mmu_cache_search(rail_ib, ptr, size);
    if (mmu_entry) {
      PROF_INC(rail_ib->rail, mmu_cache_hit);
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
  sctk_spinlock_unlock (&mmu->lock);
  sctk_ib_nodebug("Entry reserved (free_nb:%d size:%lu)", mmu->free_nb, size);
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
  mmu_entry->key.ptr    = ptr;
  mmu_entry->key.size   = size;

  if (in_cache && config->ibv_mmu_cache_enabled)
    sctk_ib_mmu_cache_add(rail_ib, mmu_entry);

  sctk_nodebug("Entry %p registered", mmu_entry);
  PROF_INC(rail_ib->rail, mmu_cache_miss);
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
      sctk_spinlock_unlock (&mmu->cache.lock);
      return;
    }
  }
  assume (mmu_entry->registrations_nb == 0);
  sctk_nodebug("Entry %p unregistered", mmu_entry);

  mmu_entry->key.ptr = NULL;
  mmu_entry->key.size = 0;
  mmu_entry->status = ibv_entry_free;
  assume (ibv_dereg_mr (mmu_entry->mr) == 0);

  sctk_spinlock_lock (&mmu->lock);
  mmu->free_nb++;
  DL_PREPEND(mmu->free_entry,mmu_entry);
  sctk_spinlock_unlock (&mmu->lock);
}

#endif
