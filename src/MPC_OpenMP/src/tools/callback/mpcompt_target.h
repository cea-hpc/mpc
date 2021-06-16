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

#ifndef __MPCOMPT_TARGET_H__
#define __MPCOMPT_TARGET_H__

#include "mpcompt_macros.h"

#if OMPT_SUPPORT

void
__mpcompt_callback_target_data_op( ompt_id_t target_id,
                                   ompt_id_t host_op_id,
                                   ompt_target_data_op_t optype,
                                   void *src_addr,
                                   int src_device_num,
                                   void *dest_addr,
                                   int dest_device_num,
                                   size_t bytes );

void
__mpcompt_callback_target( ompt_target_t kind,
                           ompt_scope_endpoint_t endpoint,
                           int device_num,
                           ompt_data_t *task_data,
                           ompt_id_t target_id );

void
__mpcompt_callback_target_map( ompt_id_t target_id,
                               unsigned int nitems,
                               void **host_addr,
                               void **device_addr,
                               size_t *bytes,
                               unsigned int *mapping_flags );

void
__mpcompt_callback_target_submit( ompt_id_t target_id,
                                  ompt_id_t host_op_id,
                                  unsigned int requested_num_teams );

#endif /* OMPT_SUPPORT */
#endif /* __MPCOMPT_TARGET_H__ */
