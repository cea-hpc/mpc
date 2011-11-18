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
#ifndef __SCTK__IB_MMU_H_
#define __SCTK__IB_MMU_H_

#include <sctk_spinlock.h>
#include <sctk_debug.h>
#include <sctk_config.h>
#include <stdint.h>

struct sctk_rail_info_ib_s;

/* Enumeration for entry status  */
typedef enum sctk_ib_mmu_entry_status_e
{
  ibv_entry_free = 111,
  ibv_entry_used = 222,
} sctk_ib_mmu_entry_status_t;

/* Entry to the soft MMU */
typedef struct sctk_ib_mmu_entry_s
{
  /* For DL list */
  struct sctk_ib_mmu_entry_s* prev;
  struct sctk_ib_mmu_entry_s* next;
  /* status of the slot */
  sctk_ib_mmu_entry_status_t status;     /* status of entry */
  struct sctk_ib_mmu_region_s* region;  /* first region */
  void *ptr;                /* ptr to the MR */
  size_t size;              /* size of the MR */
  struct ibv_mr *mr;        /* MR */
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

typedef struct sctk_ib_mmu_s
{
  struct sctk_ib_mmu_region_s *regions;
  sctk_spinlock_t lock;     /* MMU lock */
  unsigned int nb;     /* Total Number of  mmu entries */
  int free_nb;    /* Number of free mmu entries */
  size_t page_size; /* size of a system page */
  sctk_ib_mmu_entry_t*  free_header;
} sctk_ib_mmu_t;

/*-----------------------------------------------------------
 *  FUNCTIONS
 *----------------------------------------------------------*/
void sctk_ib_mmu_init(struct sctk_rail_info_ib_s *rail_ib);

 void sctk_ib_mmu_alloc(struct sctk_rail_info_ib_s *rail_ib,
     const unsigned int nb_entries);

sctk_ib_mmu_entry_t *sctk_ib_mmu_register (
  struct sctk_rail_info_ib_s *rail_ib, void *ptr, size_t size);

void ctk_ib_mmu_unregister (struct sctk_rail_info_ib_s *rail_ib,
    sctk_ib_mmu_entry_t *mmu_entry);

#endif
#endif
