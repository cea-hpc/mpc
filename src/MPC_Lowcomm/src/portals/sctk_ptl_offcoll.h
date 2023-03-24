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

#ifndef SCTK_PTL_OFFCOLL_H_
#define SCTK_PTL_OFFCOLL_H_

#include "sctk_ptl_types.h"
#include "rail.h"

#define SCTK_PTL_ACTIVE_UNLOCK_THRS ((size_t)42)
#define SCTK_PTL_OFFCOLL_UP (0)
#define SCTK_PTL_OFFCOLL_DOWN (1)
#define SCTK_PTL_ME_PUT_NOEV_FLAGS (SCTK_PTL_ME_PUT_FLAGS  | PTL_ME_EVENT_COMM_DISABLE )
#define SCTK_PTL_ME_GET_NOEV_FLAGS (SCTK_PTL_ME_GET_FLAGS  | PTL_ME_EVENT_COMM_DISABLE )
#define SCTK_PTL_MD_PUT_NOEV_FLAGS (SCTK_PTL_MD_PUT_FLAGS | PTL_MD_EVENT_SUCCESS_DISABLE )
#define SCTK_PTL_MD_GET_NOEV_FLAGS (SCTK_PTL_MD_GET_FLAGS | PTL_MD_EVENT_SUCCESS_DISABLE )
#define SCTK_PTL_MATCH_OFFCOLL_BARRIER_UP (sctk_ptl_matchbits_t) {.offload.type = MPC_LOWCOMM_BARRIER_OFFLOAD_MESSAGE, .offload.iter = 0, .offload.pad2 = 0, .offload.dir = SCTK_PTL_OFFCOLL_UP}
#define SCTK_PTL_MATCH_OFFCOLL_BARRIER_DOWN (sctk_ptl_matchbits_t) {.offload.type = MPC_LOWCOMM_BARRIER_OFFLOAD_MESSAGE, .offload.iter = 0, .offload.pad2 = 0, .offload.dir = SCTK_PTL_OFFCOLL_DOWN}
#define SCTK_PTL_MATCH_OFFCOLL_BCAST (sctk_ptl_matchbits_t) {.offload.type = MPC_LOWCOMM_BROADCAST_OFFLOAD_MESSAGE, .offload.iter = 0, .offload.pad2 = 0, .offload.dir = 0}
#define SCTK_PTL_MATCH_OFFCOLL_BCAST_LARGE (sctk_ptl_matchbits_t) {.offload.type = MPC_LOWCOMM_BROADCAST_OFFLOAD_MESSAGE, .offload.iter = 0, .offload.pad2 = 0, .offload.dir = 1}
#define SCTK_PTL_MATCH_OFFCOLL_BCAST_LARGET(x) (sctk_ptl_matchbits_t) {.offload.type = MPC_LOWCOMM_BROADCAST_OFFLOAD_MESSAGE, .offload.iter = x, .offload.pad2 = 0, .offload.dir = 2}

int sctk_ptl_offcoll_enabled(sctk_ptl_rail_info_t* rail);
void sctk_ptl_offcoll_init(sctk_ptl_rail_info_t* rail);
void sctk_ptl_offcoll_fini(sctk_ptl_rail_info_t* rail);
void sctk_ptl_offcoll_pte_init(sctk_ptl_rail_info_t* rail, sctk_ptl_pte_t* pte);
void sctk_ptl_offcoll_pte_fini(sctk_ptl_rail_info_t* rail, sctk_ptl_pte_t* pte);
void sctk_ptl_offcoll_event_me(sctk_rail_info_t* rail, sctk_ptl_event_t ev);
void sctk_ptl_offcoll_event_md(sctk_rail_info_t* rail, sctk_ptl_event_t ev);

/***** INTERFACE *******/
int ptl_offcoll_barrier(int, int, int);
int ptl_offcoll_bcast(int comm_idx, int rank, int size, void* buf, size_t bytes, int root);
int ptl_offcoll_enabled();

#endif /* SCTK_PTL_OFFCOLL_H_ */
