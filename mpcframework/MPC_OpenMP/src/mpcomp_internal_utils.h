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
#include "mpcomp_macros.h"

#ifndef __MPCOMP_INTERNAL_UTILS_H__
#define __MPCOMP_INTERNAL_UTILS_H__

#include "mpcomp_internal.h"
#include "mpcomp_enum_types.h"
#include "mpcomp_task_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

static inline void 
__mpcomp_new_parallel_region_info_init( mpcomp_new_parallel_region_info_t * info ) 
{
	sctk_assert( MPCOMP_COMBINED_NONE == 0 );
	memset( info,  0, sizeof(mpcomp_new_parallel_region_info_t ));
}

static inline void 
__mpcomp_team_init( mpcomp_team_t *team_info )
{
	memset( team_info,  0, sizeof( mpcomp_team_t ) );
	sctk_atomics_store_int( &( team_info->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i ), MPCOMP_NOWAIT_STOP_SYMBOL );
	__mpcomp_task_team_init( team_info );
}

static inline void
mpcomp_thread_htable_kmp( mpcomp_thread_t *thread )
{
	sctk_assert( thread );
	thread->th_pri_common = ( mpcomp_kmp_common_table_t * ) sctk_malloc( sizeof( mpcomp_kmp_common_table_t ) );
	sctk_assert( thread->th_pri_common );
	memset( thread->th_pri_common, 0, sizeof( mpcomp_kmp_common_table_t ));
   thread->th_pri_head = NULL;
}

static inline void 
__mpcomp_thread_init( mpcomp_thread_t *thread, mpcomp_local_icv_t icvs, mpcomp_instance_t *instance, mpcomp_thread_t *father )
{
	int i;
	sctk_assert( thread );
    sctk_error( "memset ..");
	memset( thread, 0,  sizeof( mpcomp_thread_t ) );

    sctk_error( "memset ok");
	thread->info.num_threads = 1;
	thread->info.icvs = icvs;
	thread->instance = instance;
	thread->push_num_threads = -1;
	thread->father = father;

	for( i = 0; i < MPCOMP_MAX_ALIVE_FOR_DYN + 1; i++)
	{
		sctk_atomics_store_int( &( thread->for_dyn_remain[i].i ), -1 );
	}

	mpcomp_thread_task_init( thread );
	mpcomp_thread_htable_kmp( thread );
}

void  __mpcomp_internal_begin_parallel_region(int, mpcomp_new_parallel_region_info_t);
void __mpcomp_internal_end_parallel_region( mpcomp_instance_t *);


/* mpcomp.c */
void __mpcomp_init(void);
void __mpcomp_exit(void);
void __mpcomp_instance_init(mpcomp_instance_t *instance, int nb_mvps, struct mpcomp_team_s * team);
void in_order_scheduler(mpcomp_mvp_t * mvp);
void * mpcomp_slave_mvp_node(void *arg);
void * mpcomp_slave_mvp_leaf(void *arg);
void __mpcomp_internal_half_barrier();
void __mpcomp_internal_full_barrier();
void __mpcomp_barrier();

     /* mpcomp_tree.c */
int __mpcomp_flatten_topology(hwloc_topology_t topology, hwloc_topology_t *flatTopology);
int __mpcomp_restrict_topology(hwloc_topology_t *restrictedTopology, int nb_mvps);
int __mpcomp_check_tree_parameters(int n_leaves, int depth, int *degree);
int __mpcomp_build_default_tree(mpcomp_instance_t *instance);
int __mpcomp_build_tree(mpcomp_instance_t *instance, int n_leaves, int depth, int *degree);
int __mpcomp_check_tree_coherency(mpcomp_instance_t *instance);
void __mpcomp_print_tree(mpcomp_instance_t *instance);
int *__mpcomp_compute_topo_tree_array(hwloc_topology_t topology, int *depth, int *index) ;

/* mpcomp_loop_dyn.c */
void __mpcomp_dynamic_loop_init_ull(mpcomp_thread_t *t, unsigned long long lb, 
            unsigned long long b, unsigned long long incr, unsigned long long chunk_size) ;
	 void __mpcomp_dynamic_loop_init(mpcomp_thread_t *t, long lb, long b, long incr, long chunk_size) ;

     /* mpcomp_sections.c */
	 void __mpcomp_sections_init( mpcomp_thread_t * t, int nb_sections ) ;

     /* mpcomp_single.c */
void __mpcomp_single_coherency_entering_parallel_region();

	void
__mpcomp_get_specific_chunk_per_rank (int rank, int nb_threads,
		long lb, long b, long incr,
		long chunk_size, long chunk_num,
		long *from, long *to) ;

	void
__mpcomp_get_specific_chunk_per_rank_ull (int rank, int nb_threads,
		unsigned long long lb, unsigned long long b, unsigned long long incr,
		unsigned long long chunk_size, unsigned long long chunk_num,
		unsigned long long *from, unsigned long long *to) ;

long
__mpcomp_get_static_nb_chunks_per_rank (int rank, int nb_threads, long lb,
					long b, long incr, long chunk_size) ;


long
__mpcomp_get_static_nb_chunks_per_rank_ull (int rank, int nb_threads, unsigned long long lb,
					unsigned long long b, unsigned long long incr, unsigned long long chunk_size) ;

void mpcomp_warn_nested() ;

#ifdef __cplusplus
}
#endif
#endif /* __MPCOMP_INTERNAL_UTILS_H__ */
