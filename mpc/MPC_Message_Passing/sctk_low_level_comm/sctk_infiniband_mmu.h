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

#ifndef __SCTK__INFINIBAND_MMU_H_
#define __SCTK__INFINIBAND_MMU_H_

#include <sctk_spinlock.h>
#include <sctk_debug.h>

/* enumeration for entry state  */
typedef enum
{
  ibv_entry_used,
//  ibv_entry_perm,
  ibv_entry_free
} sctk_net_ibv_mmu_entry_status_t;

/* entry to the soft MMU */
typedef struct
{
  sctk_net_ibv_mmu_entry_status_t status;     /* status of the slot */

  void *ptr;                /* ptr to the MR */
  size_t size;              /* size of the MR */
  struct ibv_mr *mr;        /* MR */
//  uint32_t rkey;          /* REMOVED */
//  uint32_t lkey;          /* REMOVED */
} sctk_net_ibv_mmu_entry_t;


typedef struct sctk_net_ibv_mmu_s
{
  sctk_spinlock_t lock;     /* MMU lock */
  int entry_nb;             /* Number of entries */
  sctk_net_ibv_mmu_entry_t** entry;  /* entries */
} sctk_net_ibv_mmu_t;

#include "sctk_infiniband_qp.h"

/*-----------------------------------------------------------
 *  NEW / FREE
 *----------------------------------------------------------*/
sctk_net_ibv_mmu_t* sctk_net_ibv_mmu_new();

void sctk_net_ibv_mmu_free();

/*-----------------------------------------------------------
 *  Register / Unregister
 *----------------------------------------------------------*/

sctk_net_ibv_mmu_entry_t *
sctk_net_ibv_mmu_register(
    sctk_net_ibv_mmu_t* mmu,
    sctk_net_ibv_qp_local_t* local,
    void *ptr, size_t size);

int
sctk_net_ibv_mmu_unregister (
    sctk_net_ibv_mmu_t *mmu,
    sctk_net_ibv_mmu_entry_t *mmu_entry);

long unsigned
sctk_net_ibv_mmu_get_pagesize();
#endif
