/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:07 CEST 2021                                        # */
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
/* # - Augustin Serraz <augustin.serraz@exascale-computing.eu>            # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Sylvain Didelot <sylvain.didelot@exascale-computing.eu>            # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef MPC_OMP_TREE_H_
#define MPC_OMP_TREE_H_

#include "mpcomp_types.h"
#include "omp_intel.h"

/***************
 * TREE STRUCT *
 ***************/

/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */
static inline void
_mpc_common_table_reset( struct common_table *th_pri_common )
{
	assert( th_pri_common );
	memset( th_pri_common, 0, sizeof( struct common_table ) );
}

static inline struct common_table *_mpc_common_table_allocate( void )
{
	struct common_table *th_pri_common;
	th_pri_common =
	    ( struct common_table * )sctk_malloc( sizeof( struct common_table ) );
	assert( th_pri_common );
	_mpc_common_table_reset( th_pri_common );
	return th_pri_common;
}

static inline void _mpc_omp_thread_infos_init( mpc_omp_thread_t *thread ) {
    assert( thread );

    int i;
	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	for ( i = 0; i < MPC_OMP_MAX_ALIVE_FOR_DYN + 1; i++ )
	{
		OPA_store_int( &( thread->for_dyn_remain[i].i ), -1 );
	}

#ifdef MPC_OMP_USE_INTEL_ABI
	thread->push_num_threads = -1;
	thread->th_pri_common = _mpc_common_table_allocate();
	thread->reduction_method = 0;
#endif /* MPC_OMP_USE_INTEL_ABI */
}
/* NOLINTEND(clang-diagnostic-unused-function) */

/**************
 * TASK UTILS *
 **************/

int mpc_omp_tree_array_get_neighbor( int globalRank, int i );
int mpc_omp_tree_array_ancestor_get( int globalRank, int depth );

void _mpc_omp_tree_task_check_neigborhood( void );

/**************
 * TREE ARRAY *
 **************/

void mpc_omp_init_seq_region();
void mpc_omp_init_initial_thread( const mpc_omp_local_icv_t icvs );
void _mpc_omp_tree_alloc( int *shape, int top_level, const int *cpus_order, const int place_depth, const int place_size );

int *mpc_omp_tree_array_compute_thread_min_rank( const int *shape, const int top_level, const int rank, const int core_depth );


#endif /* MPC_OMP_TREE_H_ */
