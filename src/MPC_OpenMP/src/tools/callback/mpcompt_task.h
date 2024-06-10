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

#ifndef __MPCOMPT_TASK_H__
#define __MPCOMPT_TASK_H__

#include "mpcompt_macros.h"

#if OMPT_SUPPORT
#include "mpcomp_core.h"
#include "mpcomp_task.h"

void
_mpc_omp_ompt_callback_task_create( mpc_omp_task_t *task,
                                int flags,
                                int has_dependences );

void
_mpc_omp_ompt_callback_dependences( mpc_omp_task_t *task,
                                const ompt_dependence_t *deps,
                                int ndeps );

void
_mpc_omp_ompt_callback_task_dependence( ompt_data_t *src_task_data,
                                    ompt_data_t *sink_task_data );

void
_mpc_omp_ompt_callback_task_schedule( ompt_data_t *prior_task_data,
                                  ompt_task_status_t prior_task_status,
                                  ompt_data_t *next_task_data );

void
_mpc_omp_ompt_callback_implicit_task( ompt_scope_endpoint_t endpoint,
                                  unsigned int actual_parallelism,
                                  unsigned int index,
                                  int flags );


static inline ompt_task_flag_t
__mpc_omp_ompt_get_task_flags( mpc_omp_thread_t * thread, mpc_omp_task_t *new_task ) {
    ompt_task_flag_t flags = ompt_task_explicit;
    int task_nesting_max = mpc_omp_conf_get()->task_depth_threshold;

    if( mpc_omp_task_property_isset( new_task->property, MPC_OMP_TASK_PROP_UNDEFERRED ))
        flags |= ompt_task_undeferred;

    if( mpc_omp_task_property_isset( new_task->property, MPC_OMP_TASK_PROP_FINAL ))
        flags |= ompt_task_final;

    if( !( flags & ompt_task_undeferred )
        && ( thread->info.num_threads == 1
             || MPC_OMP_TASK_THREAD_GET_CURRENT_TASK(thread)->depth > task_nesting_max ))
        flags |= ompt_task_undeferred;

    /* NOTE: One case not handled: if task list has reached the maximum capacity
     *       of delayed tasks the new task will be undeferred in method task process
     *       and we cannot currently manage it.
     */

    return flags;
}

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_TASK_H__ */
