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
#ifndef SCTK_PTL_PROBE_H_
#define SCTK_PTL_PROBE_H_

#include <portals4.h>
#include "sctk_types.h"
#include "sctk_ptl_types.h"
#include "sctk_debug.h"

/* unmatched ME management */
void sctk_ptl_pending_me_pop(sctk_ptl_rail_info_t* prail, sctk_ptl_pte_t* pte, int rank, int tag, size_t size, void* addr);
void sctk_ptl_pending_me_push(sctk_ptl_rail_info_t* prail, sctk_ptl_pte_t* pte, int rank, int tag, size_t size, void* addr);
int sctk_ptl_pending_me_probe(sctk_ptl_rail_info_t* prail, sctk_communicator_t comm, int rank, int tag,  size_t* msg_size);

#endif
#endif