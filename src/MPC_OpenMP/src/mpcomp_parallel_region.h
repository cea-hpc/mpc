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
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_PARALLEL_REGION_H__
#define __MPCOMP_PARALLEL_REGION_H__

#include "mpcomp_types.h"

void __mpcomp_start_parallel_region( void ( *func )( void * ), void *shared,
                                     unsigned arg_num_threads );

void __mpcomp_start_sections_parallel_region( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads,
        unsigned nb_sections );

void __mpcomp_start_parallel_dynamic_loop( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads, long lb,
        long b, long incr, long chunk_size );

void __mpcomp_start_parallel_static_loop( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads, long lb,
        long b, long incr, long chunk_size );

void __mpcomp_start_parallel_guided_loop( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads, long lb,
        long b, long incr, long chunk_size );

void __mpcomp_start_parallel_runtime_loop( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads, long lb,
        long b, long incr, long chunk_size );

void __mpcomp_internal_begin_parallel_region( mpcomp_parallel_region_t *info, const unsigned expected_num_threads );

void __mpcomp_internal_end_parallel_region( mpcomp_instance_t *instance );

static inline void __mpcomp_parallel_set_specific_infos(
    mpcomp_parallel_region_t *info, void *( *func )( void * ), void *data,
    mpcomp_local_icv_t icvs, mpcomp_combined_mode_t type )
{
	assert( info );
	info->func = ( void *( * ) ( void * ) ) func;
	info->shared = data;
	info->icvs = icvs;
	info->combined_pragma = type;
}
#endif /*  __MPCOMP_PARALLEL_REGION_H__ */
