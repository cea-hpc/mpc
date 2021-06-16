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

#ifndef __MPCOMPT_INSTANCE_TYPES_H__
#define __MPCOMPT_INSTANCE_TYPES_H__

#include "mpcompt_macros.h"

#if OMPT_SUPPORT
#include "opa_primitives.h"
#include "mpc_common_spinlock.h"

typedef enum {
    uninitialized         = 0,
    initialized           = 1,
    inactive              = 2,
    active                = 3,
    pending               = 4,
    e_tool_status_count   = 5
} mpcompt_tool_status_t;

typedef struct mpcompt_tool_instance_s {
    char *path;
    ompt_start_tool_result_t *start_result;
    ompt_callback_t *callbacks;
    mpc_common_rwlock_t lock;
    ompt_wait_id_t wait_id;
    mpc_common_spinlock_t wait_id_lock;
    OPA_int_t nb_native_threads_exited;
} mpcompt_tool_instance_t;

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_INSTANCE_H__ */
