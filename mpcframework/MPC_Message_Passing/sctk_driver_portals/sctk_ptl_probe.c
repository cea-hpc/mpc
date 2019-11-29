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
/* #   - ADAM Julien julien.adam@paratools.com                            # */
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@paratools.com        # */
/* #                                                                      # */
/* ######################################################################## */

#ifdef MPC_USE_PORTALS
#include <limits.h>
#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "sctk_atomics.h"
#include "sctk_ptl_iface.h"
#include "sctk_ptl_types.h"

static sctk_ptl_pending_entry_t* __sctk_ptl_pending_me_lock_cell(sctk_ptl_pte_t* pte, int rank, int tag, char look_for_free)
{
	int i;
	for(i = 0; i < SCTK_PTL_ME_OVERFLOW_NB; i++)
	{
		sctk_ptl_pending_entry_t* cell = &pte->pending.cells[i];
		sctk_spinlock_lock(&cell->lock);
		
		/** The cell need to match the type of cell (used or not) */
		if(cell->avail == look_for_free && /* if cell match what the caller asks for */
			(look_for_free || /* if the purpose is to find a free cell, return the first one */
			(
				(rank == SCTK_ANY_SOURCE || cell->rank == rank) && 
				(tag == SCTK_ANY_TAG || cell->tag == tag)
			))) /* otherwise match incoming message (taking care of ANY_* wildcards) */
		{
			sctk_nodebug("gotcha ! %p -- %d - %d - %d = %d", cell, cell->avail == look_for_free, look_for_free, (rank == SCTK_ANY_SOURCE || cell->rank == rank), (tag == SCTK_ANY_TAG || cell->tag == tag));
			return cell;
		}
		sctk_spinlock_unlock(&cell->lock);
	}
	return NULL;

}

static int sctk_ptl_pending_me_lookup(sctk_ptl_rail_info_t* prail, sctk_ptl_pte_t* pte, int rank, int tag, size_t* size, char dequeue)
{
	sctk_ptl_pending_entry_t* ret = __sctk_ptl_pending_me_lock_cell(pte, rank, tag, (char)0);
	if(ret)
	{
		*size = ret->size;
		if(dequeue)
		{
			ret->avail = 1;
		}
		sctk_spinlock_unlock(&ret->lock);
		
	}
	return (ret != NULL);
}

void sctk_ptl_pending_me_pop(sctk_ptl_rail_info_t* prail, sctk_ptl_pte_t* pte, int rank, int tag, size_t size, void* me_addr)
{
	sctk_assert( ! (pte->idx < SCTK_PTL_PTE_HIDDEN));
	if(me_addr)
	{
		sctk_ptl_pending_entry_t* entry;
		sctk_atomics_ptr* saved_slot = ((char*)me_addr) - sizeof(sctk_atomics_ptr);
	
		do
		{
			entry = (sctk_ptl_pending_entry_t**)sctk_atomics_swap_ptr(saved_slot, NULL);
		}
		while(entry == NULL);
		sctk_debug("POP: c%d r%d t%d s%llu (entry=%p)", pte->idx - SCTK_PTL_PTE_HIDDEN, rank, tag, size, entry);
	
		sctk_assert(entry != NULL);
		
		sctk_spinlock_lock(&entry->lock);
		entry->avail = 1;
		sctk_spinlock_unlock(&entry->lock);
	}
	else
	{
		sctk_ptl_pending_me_lookup(prail, pte, rank, tag, &size, (char)1);
	}
}

void sctk_ptl_pending_me_push(sctk_ptl_rail_info_t* prail, sctk_ptl_pte_t* pte, int rank, int tag, size_t size, void* me_addr)
{
	sctk_ptl_pending_entry_t* entry = __sctk_ptl_pending_me_lock_cell(pte, rank, tag, (char)1);
	sctk_atomics_ptr* saved_slot = ((char*)me_addr) - sizeof(sctk_atomics_ptr);

	if(entry)
	{
		entry->avail = 0;
		entry->rank = rank;
		entry->tag= tag;
		entry->size= size;
		sctk_debug("PUSH: c%d r%d t%d s%llu (entry=%p)", pte->idx - SCTK_PTL_PTE_HIDDEN, rank, tag, size, entry);
		if(me_addr)
		{
			sctk_assert(sctk_atomics_cas_ptr(saved_slot, NULL, NULL) == NULL);
			sctk_atomics_store_ptr(saved_slot, entry);
		}
		sctk_spinlock_unlock(&entry->lock);
	}
}

int sctk_ptl_pending_me_probe(sctk_ptl_rail_info_t* prail, sctk_communicator_t comm, int rank, int tag,  size_t* msg_size)
{
	sctk_debug("PROBE: c%d r%d t%d", comm, rank, tag);
	sctk_ptl_pte_t* pte = MPCHT_get(&prail->pt_table, (int)((comm + SCTK_PTL_PTE_HIDDEN_NB) % prail->nb_entries));
	/* 0 means 'Do not dequeue the element if a match exist' */
	return sctk_ptl_pending_me_lookup(prail, pte, rank, tag, msg_size, (char)0);
}

#endif