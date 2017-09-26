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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_LOOP_STATIC_ULL_H__
#define __MPCOMP_LOOP_STATIC_ULL_H__

#include "mpcomp_loop_static_ull_no_mpc_types.h"

void __mpcomp_static_loop_init_ull(mpcomp_thread_t *t, unsigned long long lb,
                                   unsigned long long b,
                                   unsigned long long incr,
                                   unsigned long long chunk_size);

void __mpcomp_static_schedule_get_specific_chunk_ull(
    unsigned long long rank, unsigned long long num_threads,
    mpcomp_loop_ull_iter_t *loop, unsigned long long chunk_num,
    unsigned long long *from, unsigned long long *to);

#endif /* __MPCOMP_LOOP_STATIC_ULL_H__ */
