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
#ifndef __SCTK__IB_MMU_H_
#define __SCTK__IB_MMU_H_

#include <uthash.h>
#include <sctk_spinlock.h>
#include <sctk_debug.h>
#include <sctk_config.h>
#include <stdint.h>
#include "hb_tree.h"

#define DEBUG_IB_MMU

#define ALIGN_ON(x, align) ( (x + (align-1)) & (~(align-1)) )

struct sctk_ib_rail_info_s;
struct sctk_ib_rail_info_s;

/* Enumeration for entry status  */
typedef enum sctk_ib_mmu_entry_status_e
{
  ibv_entry_free = 111,
  ibv_entry_used = 222,
} sctk_ib_mmu_entry_status_t;

typedef enum sctk_ib_mmu_cached_status_e
{
  not_cached = 111,
  cached = 222,
} sctk_ib_mmu_cached_status_t;

typedef struct sctk_ib_mmu_ht_key_s {
  void *ptr;
  size_t size;
} sctk_ib_mmu_ht_key_t;

/* Entry to the soft MMU */
typedef struct sctk_ib_mmu_entry_s
{
  /* For DL list */
  struct sctk_ib_mmu_entry_s* prev;
  struct sctk_ib_mmu_entry_s* next;
  /* status of the slot */
  sctk_ib_mmu_entry_status_t status;     /* status of entry */
  struct sctk_ib_mmu_region_s* region;  /* first region */
//  void *ptr;                /* ptr to the MR */
//  size_t size;              /* size of the MR */
  struct ibv_mr *mr;        /* MR */
  /* Pointer to the MMU the entry owns */
  struct sctk_ib_mmu_s *mmu;

  /* Status of the entry in cache */
  sctk_ib_mmu_cached_status_t cache_status;
  /* Number of registration. If 0, the entry can be erased from cache  */
  unsigned int registrations_nb;
  /* UTHash key */
  sctk_ib_mmu_ht_key_t key;
  /* UTHash header */
  UT_hash_handle hh;
  struct sctk_ib_rail_info_s *rail_ib;
} sctk_ib_mmu_entry_t;


/* Region of mmu entries */
typedef struct sctk_ib_mmu_region_s
{
  /* For DL list */
  struct sctk_ib_mmu_region_s* next;
  struct sctk_ib_mmu_region_s* prev;
  /* Number of MMU entries in region */
  unsigned int nb;
  sctk_ib_mmu_entry_t* mmu_entry;
} sctk_ib_mmu_region_t;

typedef struct sctk_ib_cache_s
{
  /* Libdict structure to store the MMU cache tree */
  hb_tree* hb_entries;
  /* LRU list of entries in cache */
  sctk_ib_mmu_entry_t* lru_entries;

  sctk_spin_rwlock_t lock;
  unsigned int entries_nb;
} sctk_ib_cache_t;

typedef struct sctk_ib_mmu_s
{
  struct sctk_ib_mmu_region_s *regions;
  sctk_spinlock_t lock;     /* MMU lock */
  unsigned int nb;     /* Total Number of  mmu entries */
  int free_nb;    /* Number of free mmu entries */
  size_t page_size; /* size of a system page */
  sctk_ib_mmu_entry_t*  free_entry;
  sctk_ib_cache_t cache;
  struct sctk_ib_topology_numa_node_s * node;
} sctk_ib_mmu_t;

struct sctk_ib_topology_numa_node_s;
/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void sctk_ib_mmu_init(struct sctk_ib_rail_info_s *rail_ib, struct sctk_ib_mmu_s * mmu, struct sctk_ib_topology_numa_node_s * node);

 void sctk_ib_mmu_alloc(struct sctk_ib_rail_info_s *rail_ib,
     struct sctk_ib_mmu_s * mmu,
     const unsigned int nb_entries);

sctk_ib_mmu_entry_t *sctk_ib_mmu_register (
  struct sctk_ib_rail_info_s *rail_ib, void *ptr, size_t size);
sctk_ib_mmu_entry_t *sctk_ib_mmu_register_no_cache(
  struct sctk_ib_rail_info_s *rail_ib, void *ptr, size_t size);

void ctk_ib_mmu_unregister (struct sctk_ib_rail_info_s *rail_ib,
    sctk_ib_mmu_entry_t *mmu_entry);

void
sctk_ib_mmu_unregister (struct sctk_ib_rail_info_s *rail_ib,
    sctk_ib_mmu_entry_t *mmu_entry);

sctk_ib_mmu_t * sctk_ib_mmu_get_mmu_from_vp( struct sctk_ib_rail_info_s *rail_ib );
#endif
#endif
