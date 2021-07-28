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

#ifndef __MPCOMPT_CALLBACK_H__
#define __MPCOMPT_CALLBACK_H__

#if OMPT_SUPPORT
#include "mpcompt_internal_structs.h"
#include "mpc_common_debug.h"

static mpc_omp_ompt_callback_infos_t mpcompt_callback_infos[] = {
#define ompt_callback_macro(callback, code, status) {#callback, code, status},
  FOREACH_OMPT_CALLBACK(ompt_callback_macro)
#undef ompt_callback_macro
};

static inline ompt_set_result_t
_mpc_omp_ompt_get_callback_status ( ompt_callbacks_t id ) {
    uint64_t i;
    ompt_set_result_t ret = ompt_set_error;

    const uint64_t status_array_len =
        sizeof( mpcompt_callback_infos ) / sizeof( mpc_omp_ompt_callback_infos_t );
    assert( status_array_len > 0 );

    for( i = 0; i < status_array_len; i++ ) {
        if( mpcompt_callback_infos[i].id != id )
            continue;

        ret = mpcompt_callback_infos[i].status;
        break;
    }

    return ret;
}

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_CALLBACK_H__ */
