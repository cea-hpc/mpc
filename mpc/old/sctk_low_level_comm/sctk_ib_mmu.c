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
#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_ib_mmu.h"
#include "sctk_ib_cm.h"
#include "sctk_ib_qp.h"
#include "sctk_ib_profiler.h"
#include "sctk_ib_const.h"
#include "sctk_ib_config.h"
#include <infiniband/verbs.h>

/*-----------------------------------------------------------
 *  NEW / FREE
 *----------------------------------------------------------*/

static unsigned long page_size;


  static inline void
sctk_net_ibv_mmu_init(sctk_net_ibv_qp_rail_t* rail, sctk_net_ibv_mmu_t* mmu, int nb_mmu_entries, uint8_t debug)
{
  int i;
  sctk_net_ibv_mmu_region_t* region = NULL;
  sctk_net_ibv_mmu_entry_t* mmu_entry;
  sctk_net_ibv_mmu_entry_t* mmu_entry_ptr;

  /* get the page-size of the system */
  page_size = getpagesize ();

  /* if we ask for more max_mr that the card supports */
  if (mmu->total_mmu_entry_nb + nb_mmu_entries >
      rail->max_mr)
  {
    PRINT_DEBUG(1, "[mmu] Cannot allocate more MMU entries: hardware limit %d reached",
          rail->max_mr);
    /* TODO: behavior needed there */
    not_implemented();
  }

  region = sctk_calloc (1, sizeof(sctk_net_ibv_mmu_region_t));
  assume(region);

  mmu_entry = sctk_malloc(
      sizeof(sctk_net_ibv_mmu_entry_t) * nb_mmu_entries);
  assume(mmu_entry);

  region->next_region = NULL;
  region->mmu_entry = mmu_entry;

  if (!mmu->begin_region)
    mmu->begin_region = region;

  if (!mmu->last_region) {
    mmu->last_region = region;
  } else {
    mmu->last_region = mmu->last_region->next_region;
  }

  mmu->lock = SCTK_SPINLOCK_INITIALIZER;
  mmu->free_mmu_entry_nb += nb_mmu_entries;

  for (i=0; i < nb_mmu_entries - 1; ++i)
  {
    mmu_entry_ptr = mmu_entry + i;
    mmu_entry_ptr->region = region;

    mmu_entry_ptr->desc.next = ((struct sctk_net_ibv_mmu_entry_s*) mmu_entry + i + 1);

    mmu_entry_ptr->status = ibv_entry_free;
  }

  mmu_entry_ptr = (sctk_net_ibv_mmu_entry_t*) mmu_entry + nb_mmu_entries - 1;
  mmu_entry_ptr->region = region;
  mmu_entry_ptr->desc.next = mmu->free_header;
  mmu_entry_ptr->status = ibv_entry_free;

  mmu->free_header = (sctk_net_ibv_mmu_entry_t*) mmu_entry;

  mmu->total_mmu_entry_nb += nb_mmu_entries;
  if (debug )
    PRINT_DEBUG(1, "[mmu] %d MMU entries allocated (total:%d, free:%d, got:%d)", nb_mmu_entries, mmu->total_mmu_entry_nb, mmu->free_mmu_entry_nb, mmu->got_mmu_entry_nb);
}



/**
 *  Create a new MMU object
 *  \param
 *  \return
 */
  sctk_net_ibv_mmu_t*
sctk_net_ibv_mmu_new(sctk_net_ibv_qp_rail_t* rail)
{
  sctk_net_ibv_mmu_t* mmu = NULL;
  sctk_net_ibv_mmu_region_t* region = NULL;

  /* get the page-size of the system */
  page_size = getpagesize ();

  /*allocate soft mmu entries */
  mmu = sctk_calloc (1, sizeof(sctk_net_ibv_mmu_t));
  assume(mmu);
  region = sctk_calloc (1, sizeof(sctk_net_ibv_mmu_region_t));
  assume(region);

  sctk_net_ibv_mmu_init(rail, mmu, ibv_max_mr, 0);

  rail->mmu = mmu;

  return mmu;
}


void
sctk_net_ibv_mmu_free(sctk_net_ibv_mmu_t* mmu) {
  sctk_free(mmu);
}

//TODO Handle multiple dclaration of the same pointer
sctk_net_ibv_mmu_entry_t *
sctk_net_ibv_mmu_register (
    sctk_net_ibv_qp_rail_t* rail,
    sctk_net_ibv_qp_local_t* local,
    void *ptr, size_t size)
{
  sctk_net_ibv_mmu_entry_t* mmu_entry;
  sctk_net_ibv_mmu_t* mmu;

  mmu = rail->mmu;

  if ((uintptr_t) ptr % page_size)
  {
    sctk_error("MMU ptr is not aligned on page_size");
    sctk_abort();
  }

  sctk_spinlock_lock (&mmu->lock);
  while(1)
  {
    if (mmu->free_header)
    {
      goto resume;
    } else {
      /* allocate more buffers */
      sctk_net_ibv_mmu_init(rail, mmu, ibv_size_mr_chunk, 1);
      goto resume;
    }
  }

resume:
  mmu_entry = mmu->free_header;
  mmu->free_header = mmu->free_header->desc.next;

  --mmu->free_mmu_entry_nb;
  ++mmu->got_mmu_entry_nb;
  sctk_nodebug("[mmu] entry reserved (%d %d)", mmu->free_mmu_entry_nb, mmu->got_mmu_entry_nb);
  sctk_spinlock_unlock (&mmu->lock);

  mmu_entry->status = ibv_entry_used;

  mmu_entry->mr = ibv_reg_mr (local->pd,
      ptr, size,
      IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
      IBV_ACCESS_REMOTE_READ);

  if (mmu_entry->mr == NULL)
  {
    sctk_error ("Cannot register adm MR.");
    sctk_abort();
  }
  mmu_entry->ptr    = ptr;
  mmu_entry->size   = size;

  return mmu_entry;
}

  int
sctk_net_ibv_mmu_unregister ( sctk_net_ibv_mmu_t *mmu,
    sctk_net_ibv_mmu_entry_t *mmu_entry)
{
  mmu_entry->ptr = NULL;
  mmu_entry->size = 0;
  mmu_entry->status = ibv_entry_free;
  ibv_dereg_mr (mmu_entry->mr);

  sctk_spinlock_lock (&mmu->lock);
  ++mmu->free_mmu_entry_nb;
  --mmu->got_mmu_entry_nb;
  mmu_entry->desc.next = mmu->free_header;
  mmu->free_header = mmu_entry;
  sctk_spinlock_unlock (&mmu->lock);
  return 0;
}


  long unsigned
sctk_net_ibv_mmu_get_pagesize()
{
  return page_size;
}
#endif
