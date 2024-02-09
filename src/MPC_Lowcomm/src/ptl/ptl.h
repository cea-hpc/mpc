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

#ifndef LCR_PTL_H
#define LCR_PTL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "ptl_types.h"

//FIXME: hack for previous public ptl function.
static inline int ptl_offcoll_barrier(int comm_idx, int rank, int size) {
        UNUSED(comm_idx);
        UNUSED(rank);
        UNUSED(size);
        not_implemented();
        return 0;
}

static inline int ptl_offcoll_bcast(int comm_idx, int rank, int size, 
                                    void* buf, size_t bytes, int root) {
        UNUSED(comm_idx);
        UNUSED(rank);
        UNUSED(size);
        UNUSED(buf);
        UNUSED(bytes);
        UNUSED(root);
        not_implemented();
        return 0;
}

static inline int ptl_offcoll_enabled() {
        return 0;
}

#ifdef __cplusplus
}
#endif
#endif
