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

#ifndef MPC_LOWCOMM_IB_MMU_H
#define MPC_LOWCOMM_IB_MMU_H

#include <stdlib.h>
#include <mpc_common_spinlock.h>

/** This is the actual payload */
struct ibv_mr;
struct sctk_ib_rail_info_s;

typedef struct sctk_ib_mmu_entry_s
{
	/* Content */
	void * addr; /** Adress of the pinned block */
	size_t size; /** Size of the pinned block */
	struct sctk_ib_rail_info_s *rail; /** Rail for which the block is pinned */
	struct ibv_mr *mr; /** Pointer to the IBV memory region */

	mpc_common_rwlock_t entry_refcounter; /** Refcounter */
	
	int free_on_relax;
}sctk_ib_mmu_entry_t;

sctk_ib_mmu_entry_t * sctk_ib_mmu_entry_new( struct sctk_ib_rail_info_s *rail_ib, void * addr, size_t size );
void sctk_ib_mmu_entry_release( sctk_ib_mmu_entry_t * release );

int sctk_ib_mmu_entry_contains( sctk_ib_mmu_entry_t * entry, void * addr, size_t size );
int sctk_ib_mmu_entry_intersects( sctk_ib_mmu_entry_t * entry, void * addr, size_t size );

void sctk_ib_mmu_entry_acquire( sctk_ib_mmu_entry_t * entry );
void sctk_ib_mmu_entry_relax( sctk_ib_mmu_entry_t * entry );



/** This define the structure of an MMU (internal interface with _PREFIX) */
struct sctk_ib_mmu
{
	int cache_enabled;
	sctk_ib_mmu_entry_t ** cache; /** Static fast cache */
	mpc_common_rwlock_t cache_lock; /** Lock for the MMU */
	unsigned int cache_max_entry_count; /** Maximum size of the slow cache from the config */
	
	size_t cache_maximum_size;
	size_t current_size;
};

void _sctk_ib_mmu_init( struct sctk_ib_mmu * mmu );
void _sctk_ib_mmu_release( struct sctk_ib_mmu * mmu );

sctk_ib_mmu_entry_t * _sctk_ib_mmu_pin(  struct sctk_ib_mmu * mmu,  struct sctk_ib_rail_info_s *rail_ib, void * addr, size_t size);
int _sctk_ib_mmu_unpin(  struct sctk_ib_mmu * mmu, void * addr, size_t size);

/** This is the main interface (abstracting storage) */

void sctk_ib_mmu_init();
void sctk_ib_mmu_release();

sctk_ib_mmu_entry_t * sctk_ib_mmu_pin( struct sctk_ib_rail_info_s *rail_ib, void * addr, size_t size);
void sctk_ib_mmu_relax( sctk_ib_mmu_entry_t * handler );
int sctk_ib_mmu_unpin(  void * addr, size_t size);


#endif /* MPC_LOWCOMM_IB_MMU_H */
