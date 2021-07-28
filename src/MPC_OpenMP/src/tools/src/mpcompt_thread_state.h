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

#ifndef __MPCOMPT_THREAD_STATE_H__
#define __MPCOMPT_THREAD_STATE_H__

#include "mpcompt_macros.h"

#if OMPT_SUPPORT
#include "mpcomp_types.h"
#include "mpc_common_debug.h"

static inline ompt_state_t
_mpc_omp_ompt_thread_set_state ( ompt_state_t state,
                             ompt_wait_id_t wait_id ) {
    mpc_omp_thread_t* thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    /* TODO */

    return ompt_state_undefined;
}

static inline int
_mpc_omp_ompt_thread_get_state ( ompt_wait_id_t* wait_id ) {
    mpc_omp_thread_t* thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    ompt_state_t ret = ompt_state_overhead;

    /* TODO */

    return ret;
}

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_THREAD_STATE_H__ */
