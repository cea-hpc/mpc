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

#include "sctk_ptl_offcoll.h"
#include "sctk_alloc.h"
#include "sctk_ptl_iface.h"

static inline int __sctk_ptl_offcoll_enabled(sctk_ptl_rail_info_t* srail)
{
	static int cache = -1;
	if(cache == -1)
		cache = SCTK_PTL_IS_OFFLOAD_COLL(srail->offload_support); 

	sctk_error("cache = %d", cache);
	return cache;
}

int sctk_ptl_offcoll_enabled(sctk_ptl_rail_info_t* srail)
{
	return __sctk_ptl_offcoll_enabled(srail);
}
 
/** TODO: REMOVE all these global variables (should be per rail) */
sctk_ptl_local_data_t* md_up;
sctk_ptl_local_data_t* md_down;

sctk_ptl_cnth_t* me_cnt_up, *me_cnt_down, *md_cnt_up, *md_cnt_down;

void sctk_ptl_offcoll_init(sctk_ptl_rail_info_t* srail)
{
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
	md_up = sctk_ptl_md_create_with_cnt(srail, NULL, 0, SCTK_PTL_MD_PUT_NOEV_FLAGS);
	md_down = sctk_ptl_md_create_with_cnt(srail, NULL, 0, SCTK_PTL_MD_PUT_NOEV_FLAGS);
	sctk_ptl_md_register(srail, md_up);
	sctk_ptl_md_register(srail, md_down);

	md_cnt_up = &md_up->slot.md.ct_handle;
	md_cnt_down = &md_down->slot.md.ct_handle;
}

void sctk_ptl_offcoll_fini(sctk_ptl_rail_info_t* srail)
{
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
	sctk_ptl_md_release(md_up);   md_up = NULL;
	sctk_ptl_md_release(md_down); md_down = NULL;
}

void sctk_ptl_offcoll_pte_init(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte)
{
	sctk_ptl_local_data_t* me_up = NULL, *me_down = NULL;
	sctk_assert(srail);
	sctk_assert(pte);
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
	
	me_up = sctk_ptl_me_create_with_cnt(srail, NULL, 0, SCTK_PTL_ANY_PROCESS, SCTK_PTL_MATCH_OFFCOLL_BARRIER_UP, SCTK_PTL_MATCH_INIT, SCTK_PTL_ME_PUT_NOEV_FLAGS);
	me_up->msg = NULL;
	me_up->list = SCTK_PTL_PRIORITY_LIST;
	/// TODO: Wrong setting to check (but not event generated...)
	me_up->type = 0;
	me_up->prot = 0;
	me_down = sctk_ptl_me_create_with_cnt(srail, NULL, 0, SCTK_PTL_ANY_PROCESS, SCTK_PTL_MATCH_OFFCOLL_BARRIER_DOWN, SCTK_PTL_MATCH_INIT, SCTK_PTL_ME_PUT_NOEV_FLAGS);
	me_down->msg = NULL;
	me_down->list = SCTK_PTL_PRIORITY_LIST;
	/// TODO: Wrong setting to check (but not event generated...)
	me_down->type = 0;
	me_down->prot = 0;
	sctk_assert(me_up);
	sctk_assert(me_down);
	sctk_ptl_me_register(srail, me_up, pte);
	sctk_ptl_me_register(srail, me_down, pte);
	me_cnt_up = &me_up->slot.me.ct_handle;
	me_cnt_down = &me_down->slot.me.ct_handle;
}

void sctk_ptl_offcoll_pte_fini(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte)
{
	sctk_assert(__sctk_ptl_offcoll_enabled(srail));
}

void sctk_ptl_offcoll_halfbarrier_up(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte)
{
}

void sctk_ptl_offcoll_halfbarrier_down(sctk_ptl_rail_info_t* srail, sctk_ptl_pte_t* pte)
{
}

#endif
