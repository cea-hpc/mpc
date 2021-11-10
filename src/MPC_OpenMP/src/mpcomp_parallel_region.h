/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:05 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Adrien Roussel <adrien.roussel@cea.fr>                             # */
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_OMP_PARALLEL_REGION_H__
#define __MPC_OMP_PARALLEL_REGION_H__

#include "mpcomp_types.h"

void _mpc_omp_start_parallel_region( void ( *func )( void * ), void *shared,
                                     unsigned arg_num_threads );

void _mpc_omp_start_sections_parallel_region( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads,
        unsigned nb_sections );

void _mpc_omp_start_parallel_dynamic_loop( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads, long lb,
        long b, long incr, long chunk_size );

void _mpc_omp_start_parallel_static_loop( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads, long lb,
        long b, long incr, long chunk_size );

void _mpc_omp_start_parallel_guided_loop( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads, long lb,
        long b, long incr, long chunk_size );

void _mpc_omp_start_parallel_runtime_loop( void ( *func )( void * ), void *shared,
        unsigned arg_num_threads, long lb,
        long b, long incr, long chunk_size );

void _mpc_omp_internal_begin_parallel_region( mpc_omp_parallel_region_t *info, const unsigned expected_num_threads );

void _mpc_omp_internal_end_parallel_region( mpc_omp_instance_t *instance );

static inline void _mpc_omp_parallel_set_specific_infos(
    mpc_omp_parallel_region_t *info, void *( *func )( void * ), void *data,
    mpc_omp_local_icv_t icvs, mpc_omp_combined_mode_t type )
{
	assert( info );
	info->func = ( void *( * ) ( void * ) ) func;
	info->shared = data;
	info->icvs = icvs;
	info->combined_pragma = type;
}

#endif /*  __MPC_OMP_PARALLEL_REGION_H__ */
