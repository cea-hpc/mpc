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

#ifndef __MPCOMPT_MUTEXIMPL_H__
#define __MPCOMPT_MUTEXIMPL_H__

#if OMPT_SUPPORT
#include "mpcompt_internal_structs.h"
#include "mpc_common_debug.h"

static mpc_omp_ompt_enumerate_infos_t mpcompt_mutexImpl_infos[] = {
#define ompt_mutex_impl_macro(mutex_impl, code, desc) {#mutex_impl, mutex_impl},
  FOREACH_OMPT_MUTEX_IMPL(ompt_mutex_impl_macro)
#undef ompt_mutex_impl_macro
};

static inline int
_mpc_omp_ompt_get_next_mutexImpl ( int current_impl,
                             int *next_impl,
                             const char **next_impl_name ) {
    uint64_t i;
    static const uint64_t mtx_impl_len =
        sizeof( mpcompt_mutexImpl_infos ) / sizeof( mpc_omp_ompt_enumerate_infos_t );
    assert( mtx_impl_len > 0 );

    /* Find current state in mpcompt_mutexImpl_infos tabular */
    for( i = 0; i < mtx_impl_len; i++ ) {
        if( mpcompt_mutexImpl_infos[i].id != (unsigned long) current_impl )
            continue;

        break;
    }

    /* get next value */
    i++;

    /* Found */
    if( i < mtx_impl_len ) {
        *next_impl = mpcompt_mutexImpl_infos[i].id;
        *next_impl_name = mpcompt_mutexImpl_infos[i].name;
    }

    return ( i < mtx_impl_len ) ? 1 : 0;
}

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_MUTEXIMPL_H__ */
