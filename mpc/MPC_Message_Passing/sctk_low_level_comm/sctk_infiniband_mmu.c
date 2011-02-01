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

#include "sctk_low_level_comm.h"
#include "sctk_debug.h"
#include "sctk_infiniband_mmu.h"
#include "sctk_infiniband_qp.h"
#include <infiniband/verbs.h>

#define SCTK_MAX_MR_ALLOWED 2000

/*-----------------------------------------------------------
 *  NEW / FREE
 *----------------------------------------------------------*/

static unsigned long page_size;

/**
 *  \fn ibv_init_soft_mmu
 *  \brief Inialize the soft MMU list
 */
sctk_net_ibv_mmu_t*
sctk_net_ibv_mmu_new(sctk_net_ibv_qp_rail_t* rail)
{
  int i;
  int maxval = 0;
  sctk_net_ibv_mmu_t* mmu = NULL;

  /* get the page-size of the system */
  page_size = getpagesize ();

  /* if we ask for more max_mr that the card supports */
  if (SCTK_MAX_MR_ALLOWED > rail->max_mr)
  {
    sctk_error ("The value of SCTK_MAX_MR_ALLOWED is greater that the"
        "number of mr supported by the card (which is %d)",
        rail->max_mr);
    sctk_abort();
  }

  /*allocate soft mmu entries */
  mmu = sctk_malloc (sizeof(sctk_net_ibv_mmu_t));
  assume(mmu);
  mmu->entry = sctk_malloc (SCTK_MAX_MR_ALLOWED * sizeof (sctk_net_ibv_mmu_entry_t*));
  assume(mmu->entry);
  for (i = 0; i < SCTK_MAX_MR_ALLOWED; ++i)
  {
    mmu->entry[i] = sctk_malloc(sizeof(sctk_net_ibv_mmu_entry_t));
    assume(mmu->entry[i]);
  }

  mmu->lock = 0;
  mmu->entry_nb = 0;

  for (i = 0; i < SCTK_MAX_MR_ALLOWED; i++)
  {
    mmu->entry[i]->status = ibv_entry_free;
  }

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
    sctk_net_ibv_mmu_t* mmu, sctk_net_ibv_qp_local_t* local,
    void *ptr, size_t size)
{
  int i;

  if ((uintptr_t) ptr % page_size)
  {
    sctk_error("MMU ptr is not aligned on page_size");
    sctk_abort();
  }

  sctk_spinlock_lock (&mmu->lock);
  for (i = 0; i < SCTK_MAX_MR_ALLOWED; i++)
  {
    if (mmu->entry[i]->status == ibv_entry_free)
    {
      sctk_nodebug("MMU entry %d is free", i);

      sctk_nodebug("\t\t\t\tUse PD %p", local->pd);
      mmu->entry[i]->mr = ibv_reg_mr (local->pd,
          ptr, size,
            IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
            IBV_ACCESS_REMOTE_READ);

      if (mmu->entry[i]->mr == NULL)
      {
        sctk_error ("Cannot register adm MR.");
        sctk_abort();
      }
      mmu->entry[i]->ptr    = ptr;
      mmu->entry[i]->size   = size;
      mmu->entry[i]->status = ibv_entry_used;
      mmu->entry_nb++;
      sctk_spinlock_unlock (&mmu->lock);

      return mmu->entry[i];
    }
  }
  sctk_spinlock_unlock (&mmu->lock);
  sctk_error("No more MMU entries are available");
  assume (0);
  return NULL;
}

int
sctk_net_ibv_mmu_unregister ( sctk_net_ibv_mmu_t *mmu,
    sctk_net_ibv_mmu_entry_t *mmu_entry)
{
  int i;
  sctk_spinlock_lock (&mmu->lock);

  ibv_dereg_mr (mmu_entry->mr);

  sctk_nodebug("MMU ENTRY FREE");
  mmu->entry_nb--;
  mmu_entry->status = ibv_entry_free;
  mmu_entry->ptr = NULL;
  mmu_entry->size = 0;
  sctk_spinlock_unlock (&mmu->lock);
  return 0;
}


long unsigned
sctk_net_ibv_mmu_get_pagesize()
{
  return page_size;
}
