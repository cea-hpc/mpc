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

#ifndef __MPCOMPT_MUTEX_H__
#define __MPCOMPT_MUTEX_H__

#include "mpcompt_macros.h"

#if OMPT_SUPPORT
#include "mpcomp_types.h"

void
_mpc_omp_ompt_callback_mutex_acquire( ompt_mutex_t kind,
                                  unsigned int hint,
                                  unsigned int impl,
                                  ompt_wait_id_t wait_id );

void
_mpc_omp_ompt_callback_lock_init( ompt_mutex_t kind,
                              unsigned int hint,
                              unsigned int impl,
                              ompt_wait_id_t wait_id );

void
_mpc_omp_ompt_callback_lock_destroy( ompt_mutex_t kind,
                                 ompt_wait_id_t wait_id );

void
_mpc_omp_ompt_callback_mutex_acquired( ompt_mutex_t kind,
                                   ompt_wait_id_t wait_id );

void
_mpc_omp_ompt_callback_mutex_released( ompt_mutex_t kind,
                                   ompt_wait_id_t wait_id );

void
_mpc_omp_ompt_callback_nest_lock( ompt_scope_endpoint_t endpoint,
                              ompt_wait_id_t wait_id );

/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */

static inline ompt_wait_id_t
_mpc_omp_ompt_mutex_gen_wait_id() {
    ompt_wait_id_t wait_id = 0;
    mpc_omp_thread_t* thread;
    mpc_omp_ompt_tool_instance_t* tool_instance;

    /* Get current thread infos */
    thread = (mpc_omp_thread_t*) mpc_omp_tls;
    assert( thread );

    if( thread->tool_status == active ) {
        assert( thread->tool_instance );
        tool_instance = thread->tool_instance;

        mpc_common_spinlock_lock( &tool_instance->wait_id_lock );
        tool_instance->wait_id += 1;
        wait_id = tool_instance->wait_id;
        mpc_common_spinlock_unlock( &tool_instance->wait_id_lock );
    }

    return wait_id;
}

/* NOLINTEND(clang-diagnostic-unused-function) */

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_MUTEX_H__ */
