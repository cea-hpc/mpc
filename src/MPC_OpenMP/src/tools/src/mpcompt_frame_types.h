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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMPT_FRAME_TYPES_H__
#define __MPCOMPT_FRAME_TYPES_H__

#include "mpcompt_macros.h"

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT

/* Wrapper used for pragma directives.
 * UNDEF
 * GOMP  <=> GOMP (mpc_omp_GOMP_ and some mpcomp_ prefix)
 * INTEL <=> INTEL (kmpc_ prefix)
 */
typedef enum mpc_omp_ompt_wrapper_e {
    MPCOMP_UNDEF  = 0,
    MPCOMP_GOMP   = 1,
    MPCOMP_INTEL  = 2
} mpc_omp_ompt_wrapper_t;

/* Frame infos structure, one per thread.
 */
typedef struct mpc_omp_ompt_frame_info_s {
    mpc_omp_ompt_wrapper_t omp_wrapper;
    int outter_caller;
    ompt_frame_t ompt_frame_infos;
    void* ompt_return_addr;
} mpc_omp_ompt_frame_info_t;

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_FRAME_TYPES_H__ */
