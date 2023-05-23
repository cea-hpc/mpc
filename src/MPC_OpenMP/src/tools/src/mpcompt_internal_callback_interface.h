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

#ifndef __MPCOMPT_INTERNAL_CALLBACK_INTERFACE_H__
#define __MPCOMPT_INTERNAL_CALLBACK_INTERFACE_H__

#if OMPT_SUPPORT
#include "mpcomp_types.h"
#include "mpc_common_debug.h"

static inline int
__mpc_omp_ompt_isActive( mpc_omp_thread_t* thread ) {
    return thread->tool_status == active;
}

static inline ompt_callback_t
__mpc_omp_ompt_get_callback( mpc_omp_thread_t* thread,
                         ompt_callbacks_t event ) {
    ompt_callback_t* callbacks;
    ompt_callback_t callback = NULL;

    assert( thread );
    assert( thread->tool_instance );

    /* Get tool instance callback array */
    callbacks = thread->tool_instance->callbacks;

    /* Get event callback */
    if( callbacks ) {
        callback = callbacks[event];
    }

    return callback;
}

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_INTERNAL_CALLBACK_INTERFACE_H__ */
