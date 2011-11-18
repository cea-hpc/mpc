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
#define SCTK_IB_MODULE_NAME "MMU"
#include "sctk_ib_toolkit.h"
#include "sctk_ib_config.h"
#include "utlist.h"


/**
 *  Description:
 *
 *
 */

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/

void
check_nb_entries(sctk_rail_info_ib_t *rail_ib, const unsigned int nb_entries)
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
sctk_ib_mmu_init(struct sctk_rail_info_ib_s *rail_ib)
{
  sctk_ib_mmu_t* mmu;

  mmu = sctk_malloc (sizeof(sctk_ib_mmu_t));
  assume(mmu);
  rail_ib->mmu = mmu;

  sctk_ib_mmu_alloc(rail_ib, rail_ib->config->ibv_init_mr);
}

 void
sctk_ib_mmu_alloc(struct sctk_rail_info_ib_s *rail_ib, const unsigned int nb_entries)
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
    DL_APPEND(mmu->free_header,
        ((sctk_ib_mmu_entry_t*) mmu_entry + i));
    mmu_entry_ptr->status = ibv_entry_free;
  }

  mmu->nb += nb_entries;
  sctk_ib_debug("%d MMU entries allocated (total:%d, free:%d)", nb_entries, mmu->nb, mmu->free_nb);
}


/* Register a new memory */
sctk_ib_mmu_entry_t *sctk_ib_mmu_register (
    sctk_rail_info_ib_t *rail_ib,
    void *ptr, size_t size)
{
  sctk_ib_mmu_t* mmu = rail_ib->mmu;
  sctk_ib_config_t *config = rail_ib->config;
  sctk_ib_mmu_entry_t* mmu_entry;

#ifdef DEBUG_IB_MMU
  if ((uintptr_t) ptr % mmu->page_size) {
    sctk_error("MMU ptr is not aligned on page_size");
    sctk_abort();
  }
#endif

  sctk_spinlock_lock (&mmu->lock);
  /* No entry available */
  if (!mmu->free_header) {
    /* Allocate more MMU entries */
    sctk_ib_mmu_alloc(rail_ib, config->ibv_size_mr_chunk);
  }
  mmu_entry = mmu->free_header;
  /* pop the first element */
  DL_DELETE(mmu->free_header, mmu->free_header);
  mmu->free_nb--;
  sctk_ib_debug("entry reserved (free:%d)", mmu->free_nb);
  sctk_spinlock_unlock (&mmu->lock);
  mmu_entry->status = ibv_entry_used;

  /* FIXME: allocate PD */
#if 0
  mmu_entry->mr = ibv_reg_mr (local->pd,
      ptr, size,
      IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
      IBV_ACCESS_REMOTE_READ);
#endif

#ifdef DEBUG_IB_MMU
  if (!mmu_entry->mr) {
    sctk_error ("Cannot register adm MR.");
    sctk_abort();
  }
#endif
  mmu_entry->ptr    = ptr;
  mmu_entry->size   = size;

  return mmu_entry;
}

void
sctk_ib_mmu_unregister (sctk_rail_info_ib_t *rail_ib,
    sctk_ib_mmu_entry_t *mmu_entry)
{
  sctk_ib_mmu_t* mmu = rail_ib->mmu;

  mmu_entry->ptr = NULL;
  mmu_entry->size = 0;
  mmu_entry->status = ibv_entry_free;
  assume (ibv_dereg_mr (mmu_entry->mr) == 0);

  sctk_spinlock_lock (&mmu->lock);
  mmu->free_nb++;
  DL_PREPEND(mmu->free_header,mmu_entry);
  sctk_spinlock_unlock (&mmu->lock);
}

#endif
