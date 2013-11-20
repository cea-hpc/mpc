/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Universit�� de Versailles         # */
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
#include "sctk_ib_topology.h"
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
 *  ACCESSOR
 *----------------------------------------------------------*/
sctk_ib_mmu_t * sctk_ib_mmu_get_mmu_from_vp( sctk_ib_rail_info_t *rail_ib ) {
  struct sctk_ib_topology_numa_node_s * node;

  if(sctk_net_is_mode_hybrid()) {
    /* In FullMPI, we MUST have NUMA aware structures, in hybrid we *MUST* use the default structure.
     * If not, we may register several times the same pinned memory.
     * We should use a hierarchical NUMA aware cache */
    node = sctk_ib_topology_get_default_numa_node(rail_ib);
  } else {
    node = sctk_ib_topology_get_numa_node(rail_ib);
  }
  assume(node);

  return &node->mmu;
}

/*-----------------------------------------------------------
 *  CACHE
 *----------------------------------------------------------*/

/* Entry 'b' is the entry passed to the search function. It our
 * case, it is the memory that we want to register */
static inline int sctk_ib_mmu_compare_entries(sctk_ib_mmu_entry_t *a,
    sctk_ib_mmu_entry_t *b){

  /* Check if the both regions overlap well */
  if ( (a->key.ptr >= b->key.ptr) && (
      ( (uintptr_t) a->key.ptr + a->key.size ) <= ( (uintptr_t) b->key.ptr + b->key.size) ) ) {
    return 0;
  }

  if (a->key.ptr < b->key.ptr) {
    return -1;
  }
  return +1;
}

/* We do not free the entries in these functions */
static inline void sctk_ib_mmu_delete_key(sctk_ib_mmu_entry_t *entry) {}
static inline void sctk_ib_mmu_memlist_delete(sctk_ib_mmu_entry_t *entry) {}

void sctk_ib_mmu_cache_init(struct sctk_ib_mmu_s * mmu){
  mmu->cache.hb_entries = hb_tree_new(
      (dict_cmp_func) sctk_ib_mmu_compare_entries,
      (dict_del_func) sctk_ib_mmu_delete_key,
      (dict_del_func) sctk_ib_mmu_memlist_delete);
  assume (mmu->cache.hb_entries);

  mmu->cache.lru_entries = NULL;
  sctk_spin_rwlock_init(&mmu->cache.lock);
  mmu->cache.entries_nb = 0;
}

/* Remove the Last Recently Used entry from the
 * pinned cached entries.
 * When call this function, the lock has already been
 * taken */
static inline sctk_ib_mmu_entry_t*
 sctk_ib_mmu_cache_remove(sctk_ib_mmu_t * mmu) {
  sctk_ib_mmu_entry_t* mmu_entry;

#ifdef DEBUG_IB_MMU
  assume(mmu->cache.hb_entries);
#endif

  /* select the LRU entry */
  mmu_entry = mmu->cache.lru_entries->prev;

  /* If last entry still used, we cannot add another entry */
  if (mmu_entry->registrations_nb > 0) return NULL;

  /* Remove the entry from HashTable and LRU */
  DL_DELETE(mmu->cache.lru_entries, mmu_entry);
  hb_tree_remove(mmu->cache.hb_entries, mmu_entry, 0);
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
 sctk_ib_mmu_cache_add(sctk_ib_rail_info_t *rail_ib, sctk_ib_mmu_t * mmu,
    sctk_ib_mmu_entry_t* mmu_entry) {
  LOAD_CONFIG(rail_ib);

  sctk_nodebug("Try to register (%p) %p %lu to cache %d", mmu_entry, mmu_entry->key.ptr, mmu_entry->key.size, mmu->cache.entries_nb );
  sctk_spinlock_write_lock(&mmu->cache.lock);
  /* Maximum number of cached entries reached */
  if (mmu->cache.entries_nb >= config->mmu_cache_entries) {
    sctk_ib_mmu_entry_t* mmu_entry_to_remove;
    mmu_entry_to_remove = sctk_ib_mmu_cache_remove(mmu);
    if (!mmu_entry_to_remove) {
      sctk_spinlock_write_unlock(&mmu->cache.lock);
      return 0;
    }
    sctk_ib_mmu_unregister (rail_ib, mmu_entry_to_remove);
  }
#ifdef DEBUG_IB_MMU
  assume(mmu->cache.entries_nb < config->mmu_cache_entries);
  assume(mmu_entry->registrations_nb == 0);
#endif
  sctk_nodebug("Add entry %p to cache %p %lu %d on node %d", mmu_entry, mmu_entry->key.ptr, mmu_entry->key.size, mmu->cache.entries_nb,
      mmu->node->id);
  mmu->cache.entries_nb++;
  mmu_entry->cache_status = cached;

  /* Add the entry */
  hb_tree_insert(mmu->cache.hb_entries, mmu_entry, mmu_entry, 0);
  sctk_nodebug("Add %p <-> %lu",  mmu_entry->key.ptr, mmu_entry->key.size);
  DL_PREPEND(mmu_entry->mmu->cache.lru_entries, mmu_entry);

  mmu_entry->registrations_nb++;
  sctk_spinlock_write_unlock(&mmu->cache.lock);
  return 1;
}

/* Search for a cached entry according to the ptr and
 * the size of the buffer */
sctk_ib_mmu_entry_t*
 sctk_ib_mmu_cache_search(sctk_ib_mmu_t * mmu,
    void *ptr, size_t size) {
  sctk_ib_mmu_entry_t *mmu_entry = NULL, to_find;

  /* Construct the key */
  to_find.key.ptr = ptr;
  to_find.key.size = size;

  sctk_spinlock_read_lock(&mmu->cache.lock);
  mmu_entry = (sctk_ib_mmu_entry_t*) hb_tree_search(mmu->cache.hb_entries, &to_find);
  sctk_spinlock_read_unlock(&mmu->cache.lock);
  if (mmu_entry) {
#ifdef DEBUG_IB_MMU
    assume(mmu_entry->cache_status == cached);
#endif
    sctk_spinlock_write_lock(&mmu->cache.lock);
    /* If the entry is not the head of the list */
    if (mmu->cache.lru_entries != mmu_entry) {
      DL_DELETE(mmu->cache.lru_entries, mmu_entry);
      DL_PREPEND(mmu->cache.lru_entries, mmu_entry);
    }
    mmu_entry->registrations_nb++;
    sctk_spinlock_write_unlock(&mmu->cache.lock);
  }
  return mmu_entry;
}


/*-----------------------------------------------------------
 *  MMU
 *----------------------------------------------------------*/

 void
sctk_ib_mmu_init(struct sctk_ib_rail_info_s *rail_ib, sctk_ib_mmu_t * mmu, sctk_ib_topology_numa_node_t * node)
{
  LOAD_CONFIG(rail_ib);

  memset(mmu, 0, sizeof(sctk_ib_mmu_t));

  if (config->mmu_cache_enabled) {
    sctk_ib_mmu_cache_init(mmu);
    mmu->node = node;
  }
  sctk_ib_mmu_alloc(rail_ib, mmu, config->init_mr);
}

 void
sctk_ib_mmu_alloc(struct sctk_ib_rail_info_s *rail_ib,
    struct sctk_ib_mmu_s * mmu,
    const unsigned int nb_entries)
{
  int i;
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
    mmu_entry_ptr->rail_ib = rail_ib;
    mmu_entry_ptr->mmu = mmu;
  }

  mmu->nb += nb_entries;
  sctk_ib_debug("%d MMU entries allocated (total:%d, free:%d)", nb_entries, mmu->nb, mmu->free_nb);
}

static inline sctk_ib_mmu_entry_t *
__mmu_register ( sctk_ib_rail_info_t *rail_ib,
    void *ptr, size_t size, const char in_cache) {
  sctk_ib_mmu_t * mmu = sctk_ib_mmu_get_mmu_from_vp(rail_ib);
  LOAD_DEVICE(rail_ib);
  LOAD_CONFIG(rail_ib);
  sctk_ib_mmu_entry_t* mmu_entry;

  if (in_cache && config->mmu_cache_enabled) {
    mmu_entry = sctk_ib_mmu_cache_search(mmu, ptr, size);
    if (mmu_entry) {
      PROF_INC(rail_ib->rail, mmu_cache_hit);
      return mmu_entry;
    }
  }

  sctk_spinlock_lock (&mmu->lock);
  /* No entry available */
  if (!mmu->free_entry) {
    /* Allocate more MMU entries */
    sctk_ib_mmu_alloc(rail_ib, mmu, config->size_mr_chunk);
  }
  mmu_entry = mmu->free_entry;
  /* pop the first element */
  DL_DELETE(mmu->free_entry, mmu->free_entry);
  mmu->free_nb--;
  sctk_spinlock_unlock (&mmu->lock);
  sctk_nodebug("Entry reserved (free_nb:%d size:%lu)", mmu->free_nb, size);
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

  if (in_cache && config->mmu_cache_enabled)
    sctk_ib_mmu_cache_add(rail_ib, mmu, mmu_entry);

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
  sctk_ib_mmu_t * mmu = mmu_entry->mmu;
  LOAD_CONFIG(rail_ib);
  int ret;

  if (config->mmu_cache_enabled) {
    if (mmu_entry->cache_status == cached) {
      sctk_spinlock_write_lock (&mmu->cache.lock);
      mmu_entry->registrations_nb--;
      sctk_spinlock_write_unlock (&mmu->cache.lock);
      return;
    }
  }
  //assume (mmu_entry->registrations_nb == 0);
  sctk_nodebug("Entry %p unregistered", mmu_entry);

  mmu_entry->key.ptr = NULL;
  mmu_entry->key.size = 0;
  mmu_entry->status = ibv_entry_free;
  if ((ret = ibv_dereg_mr (mmu_entry->mr)) != 0) {
    sctk_error("ibv_dereg_mr returned an error: %d %s (%p)", ret, strerror(errno), mmu_entry->mr);
  }

  sctk_spinlock_lock (&mmu->lock);
  mmu->free_nb++;
  DL_PREPEND(mmu_entry->mmu->free_entry,mmu_entry);
  sctk_spinlock_unlock (&mmu->lock);
}

#endif
