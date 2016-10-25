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

#ifndef __mpcomp_internal__H
#define __mpcomp_internal__H

#include "mpcomp.h"
#include "sctk.h"
#include "sctk_atomics.h"
#include "sctk_asm.h"
#include "sctk_context.h"

#include "mpcomp_enum_types.h"
#include "mpcomp_stack.h"

#include "mpcomp_task.h"

#ifdef __cplusplus
extern "C"
{
#endif

/********************
 ****** Threadprivate 
 ********************/
typedef struct common_table 
{
	struct private_common *data[ KMP_HASH_TABLE_SIZE ];
} mpcomp_kmp_common_table_t;

     
/* Information to transfer to every thread
 * when entering a new parallel region
 *
 * WARNING: these variables should be written in sequential part
 * and read in parallel region.
 */
typedef struct mpcomp_new_parallel_region_info_s {
	/* MANDATORY INFO */
	void *(*func) (void *);	  /* Function to call by every thread */
	void *shared;		          /* Shared variables (for every thread) */
	long num_threads;		/* Current number of threads in the team */
	struct mpcomp_node_s * new_root ;
	int single_sections_current_save ;
	int for_dyn_current_save;
	long combined_pragma;
	mpcomp_local_icv_t icvs;	/* Set of ICVs for the child team */

	/* OPTIONAL INFO */
	long loop_lb;		        /* Lower bound */
	long loop_b;			/* Upper bound */
	long loop_incr;			/* Step */
	long loop_chunk_size;	        /* Size of each chunk */
	int nb_sections ;
} mpcomp_new_parallel_region_info_t ;

/* Team of OpenMP threads */
typedef struct mpcomp_team_s
{
	mpcomp_new_parallel_region_info_t info ; /* Info for new parallel region */
	int depth;  /* Depth of the current thread (0 = sequential region) */

	/* -- SINGLE/SECTIONS CONSTRUCT -- */
	sctk_atomics_int single_sections_last_current ;
	void *single_copyprivate_data;

	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	mpcomp_atomic_int_pad_t for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN + 1];

	/* ORDERED CONSTRUCT */
	volatile int next_ordered_offset; 

#if MPCOMP_TASK
	mpcomp_task_team_infos_t task_infos;	
#endif //MPCOMP_TASK

} mpcomp_team_t;


/* OpenMP thread */
typedef struct mpcomp_thread_s
{
	sctk_mctx_t uc;		/* Context (initializes registers, ...)
											 Initialized when another thread is 
											 blocked -- NOT USED -- */
	char *stack;                  /* The stack (initialized when another
																	 thread is blocked) -- NOT USED -- */
	mpcomp_new_parallel_region_info_t info ; /* Information needed when
																							entering a new parallel
																							region */
	long rank;			/* OpenMP rank (0 for master thread per team) */
	struct mpcomp_mvp_s *mvp;     /* Micro-vp of the thread*/
	volatile int done;            /* Done value for in-order scheduler */
	struct mpcomp_instance_s *children_instance; /* Instance for nested parallelism */

	struct mpcomp_instance_s *instance; /* Current instance (team + tree) */

	long push_num_threads ;	/** Number of threads for Intel parallel region */

	/* -- SINGLE/SECTIONS CONSTRUCT -- */
	int single_sections_current ;
	int single_sections_target_current ; /* When should we stop the current sections construct? */
	int single_sections_start_current ; /* When started the last sections construct? */

	struct mpcomp_thread_s * father ; /* Father thread  TODO: use it for ancestors */

	/* LOOP CONSTRUCT */
    int schedule_type;   /* Handle scheduling type */
	/* -- STATIC FOR LOOP CONSTRUCT -- */
	int static_nb_chunks;         /* Number of total static chunk for the thread */
	int static_current_chunk;     /* Current chunk index */
    int static_chunk_size_intel;  /* save nb_chunks from intel for kmp_dispatch_next */
    int first_iteration; /* is it the first iteration ? */

	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	int for_dyn_current;    /* Current position in 'for_dyn_chunk_info' array  */
	/* Chunks array for loop dynamic schedule constructs */
	mpcomp_atomic_int_pad_t for_dyn_remain[MPCOMP_MAX_ALIVE_FOR_DYN + 1]; 
	int for_dyn_total ;
	int * for_dyn_target ; /* Coordinates of target thread to steal */
	int * for_dyn_shift ; /* Shift of target thread to steal */
	int for_dyn_last_loop_iteration ; /* WORKAROUND (pr35196.c)
                                         Did we just execute the last 
                                         iteration of the original loop? */

	/* ORDERED CONSTRUCT */
	int current_ordered_iteration; 

#if MPCOMP_TASK 
	mpcomp_task_thread_infos_t	task_infos;
#endif //MPCOMP_TASK

    /* Threadprivate */
    struct common_table *th_pri_common;
    struct private_common *th_pri_head;

    /* reduction method */
    int reduction_method;
} mpcomp_thread_t;


     /* Instance of OpenMP runtime */
     typedef struct mpcomp_instance_s
     {
	  int nb_mvps;	        	  /* Number of microVPs for this instance */
	  struct mpcomp_mvp_s **mvps;	  /* All microVPs of this instance */
	  struct mpcomp_node_s *root;	  /* Root to the tree linking the microVPs */
	  struct mpcomp_team_s *team;   /* Information on the team */
	  int tree_depth ; /* Depth of the tree */
	  int *tree_base; /* Degree per level of the tree 
						 (array of size 'tree_depth' */
	  int *tree_cumulative; /* Initialized in __mpcomp_build_tree */
      int *tree_nb_nodes_per_depth; /* Number of nodes at each depth ([0] = root = 1) */
	  hwloc_topology_t topology;
	  int scatter_depth ; /* TODO check */
	  int core_depth; /* TODO check */
	  int nb_cores; /* TODO check */
	  int balanced_last_thread; /* TODO check */
	  int balanced_current_core; /* TODO check */
#if MPCOMP_TASK
	  mpcomp_task_instance_infos_t task_infos;
#endif /* MPCOMP_TASK */
     } mpcomp_instance_t;


     /* OpenMP Micro VP */
     typedef struct mpcomp_mvp_s
     {
	  sctk_mctx_t vp_context;   /* Context including registers, stack pointer, ... */
	  sctk_thread_t pid; 	    /* Thread ID */

#if MPCOMP_ALIGN
	  mpcomp_new_parallel_region_info_t info __attribute__ ((aligned (128))) ;
#else
	  mpcomp_new_parallel_region_info_t info ;
#endif

	  int nb_threads;           /* Total number of threads running on the Micro VP */
	  int next_nb_threads;
	  struct mpcomp_thread_s threads[MPCOMP_MAX_THREADS_PER_MICROVP];

	  int *tree_rank; 	          /* Array of rank in every node of the tree */
	  int rank;    	                  /* Rank within the microVPs */
	  int min_index[MPCOMP_AFFINITY_NB] ;
      int vp;                         /* VP on which microVP is executed */
	  char pad0[64];                  /* Padding */
	  volatile int enable;
	  char pad1[64];                  /* Padding */
	  struct mpcomp_node_s *root;     /* Root of the topology tree */
	  char pad1_1[64];                /* Padding */
	  struct mpcomp_node_s *father;   /* Father node in the topology tree */
	  char pad1_2[64];                /* Padding */
	  struct mpcomp_node_s *to_run;   /* Specify where the spin waiting for a new 
					     parallel region */
	  char pad2[64];                  /* Padding */
	  volatile int slave_running;
	  char pad3[64];                  /* Padding */

	  /* OMP 3.0 */
#if MPCOMP_TASK
	  mpcomp_task_mvp_infos_t task_infos;
#endif /* MPCOMP_TASK */

     } mpcomp_mvp_t;

 

     /* OpenMP Node */
     typedef struct mpcomp_node_s
     {
	  struct mpcomp_node_s *father;   /* Father in the topology tree */
	  long rank;                      /* Rank among children of my father */
	  int depth;                      /* Depth in the tree */
	  int nb_children;                /* Number of children */

#if MPCOMP_ALIGN
      mpcomp_new_parallel_region_info_t info __attribute__ ((aligned (128))) ;
#else
	  mpcomp_new_parallel_region_info_t info ;
#endif

	  /* The following indices correspond to the 'rank' value in microVPs */
	  int min_index[MPCOMP_AFFINITY_NB];   /* Flat min index of leaves in this subtree */

	  mpcomp_instance_t * instance ; /* Information on the whole team */

	  mpcomp_children_t child_type;       /* Kind of children (node or leaf) */
	  union node_or_leaf {
	       struct mpcomp_node_s **node;
	       struct mpcomp_mvp_s **leaf;
	  } children;                         /* Children list */

#if MPCOMP_ALIGN
	  volatile int slave_running __attribute__ ((aligned (128)));
#else
	  char pad0[64];                /* Padding */
      volatile int slave_running ;
	  char pad1[64];                /* Padding */
#endif

#if MPCOMP_ALIGN
	  sctk_atomics_int barrier __attribute__ ((aligned (128)));	                /* Barrier for the child team */
#else
	  sctk_atomics_int barrier ;
	  char pad2[64];                       /* Padding */
#endif

#if MPCOMP_ALIGN
	  volatile long barrier_done __attribute__ ((aligned (128)));          /* Is the barrier (for the child team) over? */
#else
	  volatile long barrier_done ;
	  char pad3[64];                       /* Padding */
#endif

#if MPCOMP_ALIGN
	  volatile long barrier_num_threads __attribute__ ((aligned (128)));   /* Number of threads involved in the barrier */
#else
	  volatile long barrier_num_threads ;   /* Number of threads involved in the barrier */
	  char pad4[64];                       /* Padding */
#endif

#if MPCOMP_TASK
	  mpcomp_task_node_infos_t task_infos;
#endif /* MPCOMP_TASK */
		
     int id_numa;  /* NUMA node on which this node is allocated */
        
     } mpcomp_node_t;

typedef struct mpcomp_node_s* mpcomp_node_ptr_t;

/*********
 * new meta elt for task initialisation
 */

typedef enum mpcomp_tree_meta_elt_type_e
{
	MPCOMP_TREE_META_ELT_MVP,
	MPCOMP_TREE_META_ELT_NODE,
	MPCOMP_TREE_META_ELT_COUNT
} mpcomp_tree_meta_elt_type_t;

typedef union mpcomp_tree_meta_elt_ptr_u
{
	struct mpcomp_node_s *node;
 	struct mpcomp_mvp_s *mvp;
} mpcomp_tree_meta_elt_ptr_t;

typedef struct mpcomp_tree_meta_elt_s
{
	mpcomp_tree_meta_elt_type_t type;
	mpcomp_tree_meta_elt_ptr_t  ptr;
}mpcomp_tree_meta_elt_t;


/*****************
 ****** VARIABLES 
 *****************/

     extern __thread void *sctk_openmp_thread_tls;
     extern mpcomp_global_icv_t mpcomp_global_icvs;


/*****************
 ****** FUNCTIONS  
 *****************/

#include "mpcomp_task_utils.h"
     
     void  __mpcomp_internal_begin_parallel_region(int arg_num_threads, 
        mpcomp_new_parallel_region_info_t info );

    void __mpcomp_internal_end_parallel_region( mpcomp_instance_t * instance );

     /* mpcomp.c */
     void __mpcomp_init(void);
     void __mpcomp_exit(void);
     void __mpcomp_instance_init(mpcomp_instance_t *instance, int nb_mvps, 
			 struct mpcomp_team_s * team);
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

     /* mpcomp_loop_dyn.c */
	 void __mpcomp_dynamic_loop_init_ull(mpcomp_thread_t *t, unsigned long long lb, 
            unsigned long long b, unsigned long long incr, unsigned long long chunk_size) ;
	 void __mpcomp_dynamic_loop_init(mpcomp_thread_t *t, long lb, long b, long incr, long chunk_size) ;

     /* mpcomp_sections.c */
	 void __mpcomp_sections_init( mpcomp_thread_t * t, int nb_sections ) ;

     /* mpcomp_single.c */
     void __mpcomp_single_coherency_entering_parallel_region();

     /* Stack primitives */
     mpcomp_stack_t * __mpcomp_create_stack(int max_elements);
     int __mpcomp_is_stack_empty(mpcomp_stack_t *s);
     void __mpcomp_push(mpcomp_stack_t *s, mpcomp_node_t *n);
     mpcomp_node_t * __mpcomp_pop(mpcomp_stack_t *s);
     void __mpcomp_free_stack(mpcomp_stack_t *s);

     mpcomp_stack_node_leaf_t * __mpcomp_create_stack_node_leaf(int max_elements);
     int __mpcomp_is_stack_node_leaf_empty(mpcomp_stack_node_leaf_t *s);
     void __mpcomp_push_node(mpcomp_stack_node_leaf_t *s, mpcomp_node_t *n);
     void __mpcomp_push_leaf(mpcomp_stack_node_leaf_t *s, mpcomp_mvp_t *n);
     mpcomp_elem_stack_t * __mpcomp_pop_elem_stack(mpcomp_stack_node_leaf_t *s);
     void __mpcomp_free_stack_node_leaf(mpcomp_stack_node_leaf_t *s);


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

     void __mpcomp_task_coherency_entering_parallel_region();
     void __mpcomp_task_coherency_ending_parallel_region();
     void __mpcomp_task_coherency_barrier();
     void mpcomp_warn_nested() ;

#ifdef __cplusplus
}
#endif
#endif
