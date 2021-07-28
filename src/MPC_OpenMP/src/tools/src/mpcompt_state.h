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

#ifndef __MPCOMPT_STATE_H__
#define __MPCOMPT_STATE_H__

#if OMPT_SUPPORT
#include "mpcompt_internal_structs.h"
#include "mpcompt_thread_state.h"
#include "mpc_common_debug.h"

static mpc_omp_ompt_enumerate_infos_t mpcompt_state_infos[] = {
#define ompt_state_macro(state, code, desc) {#state, code},
  FOREACH_OMPT_STATE(ompt_state_macro)
#undef ompt_state_macro
};

static inline int
_mpc_omp_ompt_get_next_state ( int current_state,
                         int *next_state,
                         const char **next_state_name ) {
    int i;
    static const int state_array_len =
        sizeof( mpcompt_state_infos ) / sizeof( mpc_omp_ompt_enumerate_infos_t );
    assert( state_array_len > 0 );

    if( current_state == ompt_state_undefined ) {
        /* next state -> i = 0 */
        i = -1;
    }
    else {
        /* Find current state in mpcompt_state_infos tabular */
        for( i = 0; i < state_array_len; i++ ) {
            if( mpcompt_state_infos[i].id != (unsigned long) current_state )
                continue;

            break;
        }
    }

    /* get next value */
    i++;

    /* Found */
    if( i < state_array_len ) {
        *next_state = mpcompt_state_infos[i].id;
        *next_state_name = mpcompt_state_infos[i].name;
    }

    return ( i < state_array_len ) ? 1 : 0;
}

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_STATE_H__ */
