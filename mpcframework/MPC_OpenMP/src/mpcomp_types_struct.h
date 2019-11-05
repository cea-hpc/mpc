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
/* #   - DIONISI Thomas thomas.dionisi@exascale-computing.eu              # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_TYPES_STRUCT_H__
#define __MPCOMP_TYPES_STRUCT_H__

#include "mpcomp.h"
#include "sctk_context.h"
#include "mpc_common_asm.h"
#include "mpc_topology.h"
#include "mpcomp_types_def.h"
#include "mpcomp_types_enum.h"
#include "mpcomp_types_icv.h"
#include "mpcomp_loop.h"
#include "mpcomp_task.h"
#include "mpcomp_tree.h"
#include "mpc_common_spinlock.h"

#ifdef MPCOMP_USE_INTEL_ABI
#include "omp_intel_types.h"
#endif /* MPCOMP_USE_INTEL_ABI */

#include "ompt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****  BREAK CIRCLE DEPS FOR TASK API *****/

#include "mpcomp_task.h"

/********************
 ****** Threadprivate
 ********************/

typedef struct mpcomp_atomic_int_pad_s {
  OPA_int_t i; /**< Value of the integer */
  char pad[8];        /* Padding */
} mpcomp_atomic_int_pad_t;

/* Information to transfer to every thread
 * when entering a new parallel region
 *
 * WARNING: these variables should be written in sequential part
 * and read in parallel region.
 */
typedef struct mpcomp_new_parallel_region_info_s {
  /* MANDATORY INFO */
  void *(*func)(void *); /* Function to call by every thread */
  void *shared;          /* Shared variables (for every thread) */
  long num_threads;      /* Current number of threads in the team */
  struct mpcomp_new_parallel_region_info_s* parent;
  struct mpcomp_node_s *new_root;
  int single_sections_current_save;
  mpc_common_spinlock_t update_lock;
  int for_dyn_current_save;
  long combined_pragma;
  mpcomp_local_icv_t icvs; /* Set of ICVs for the child team */

  /* META OPTIONAL INFO FOR ULL SUPPORT */
  mpcomp_loop_gen_info_t loop_infos;
  /* OPTIONAL INFO */
  long loop_lb;         /* Lower bound */
  long loop_b;          /* Upper bound */
  long loop_incr;       /* Step */
  long loop_chunk_size; /* Size of each chunk */
  int nb_sections;

#if 1 // OMPT_SUPPORT
	ompt_data_t ompt_region_data;
    int doing_single;
    int in_single;
#endif /* OMPT_SUPPORT */

} mpcomp_parallel_region_t;

static inline void
__mpcomp_parallel_region_infos_reset( mpcomp_parallel_region_t *info )
{
	sctk_assert( info );
	memset( info, 0, sizeof( mpcomp_parallel_region_t ) );
}

static inline void
__mpcomp_parallel_region_infos_init( mpcomp_parallel_region_t *info )
{
	sctk_assert( info );
	__mpcomp_parallel_region_infos_reset( info );
	info->combined_pragma = MPCOMP_COMBINED_NONE;
}

/* Team of OpenMP threads */
typedef struct mpcomp_team_s {
  mpcomp_parallel_region_t info; /* Info for new parallel region */
  int depth; /* Depth of the current thread (0 = sequential region) */
  int id; /* team unique id */

  /* -- SINGLE/SECTIONS CONSTRUCT -- */
  OPA_int_t single_sections_last_current;
  void *single_copyprivate_data;

  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
  mpcomp_atomic_int_pad_t
      for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN + 1];

  /* GUIDED LOOP CONSTRUCT */
  volatile int is_first[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
  OPA_ptr_t guided_from[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
  mpc_common_spinlock_t lock;


  /* ORDERED CONSTRUCT */
  int next_ordered_index;
  volatile long next_ordered_offset;
  volatile unsigned long long next_ordered_offset_ull;
  OPA_int_t next_ordered_offset_finalized;

#if MPCOMP_TASK
  struct mpcomp_task_team_infos_s task_infos;
#endif // MPCOMP_TASK

} mpcomp_team_t;

/** OpenMP thread struct 
 * An OpenMP thread is attached to a MVP, 
 * one thread per nested level */
typedef struct mpcomp_thread_s {
    
    /* -- Internal specific informations -- */
    unsigned int depth;
    /** OpenMP rank (0 for master thread per team) */
    long rank;
    long tree_array_rank;
    int* tree_array_ancestor_path;
    /* Micro-vp of the thread*/
    struct mpcomp_mvp_s *mvp; 
    /** Root node for nested parallel region */ 
    struct mpcomp_node_s* root;
    /* -- Parallel region information -- */
    /** Information needed when entering a new parallel region */
    mpcomp_parallel_region_t info;
    /** Current instance (team + tree) */
    struct mpcomp_instance_s *instance; 
    /** Instance for nested parallelism */
    struct mpcomp_instance_s* children_instance; 
    /* -- SINGLE/SECTIONS CONSTRUCT -- */
    /** Thread last single or sections generation number */ 
    int single_sections_current;
    /** Thread last sections generation number in sections construct */ 
    int single_sections_target_current; 
    /**  Thread first sections generation number in sections construct */
    int single_sections_start_current;  
    /** Thread sections number in sections construct */
    int nb_sections;
    /* -- LOOP CONSTRUCT -- */
    /** Handle scheduling type */
    int schedule_type;      
    /** Handle scheduling type from wrapper */
    int schedule_is_forced; 
    /* -- STATIC FOR LOOP CONSTRUCT -- */
    /* Thread number of total chunk for static loop */
    int static_nb_chunks;        
    /** Thread current index for static loop */
    int static_current_chunk;    
    /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
    //TODO MERGE THESE TWO STRUCT ...
    /* Current position in 'for_dyn_chunk_info' array  */ 
    int for_dyn_current;
    OPA_int_t for_dyn_ull_current;
    /* Chunks array for loop dynamic schedule constructs */
    mpcomp_atomic_int_pad_t for_dyn_remain[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
    /* Total number of chunks of the thread */
    unsigned long long for_dyn_total[MPCOMP_MAX_ALIVE_FOR_DYN +1];
    /** Coordinates of target thread to steal */
    int *for_dyn_target;
    /* last target */
    struct mpcomp_thread_s* dyn_last_target;
    /* Shift of target thread to steal */
    int *for_dyn_shift;  
    mpc_common_spinlock_t *for_dyn_lock;
    /* Did we just execute the last iteration of the original loop? ( pr35196.c )*/
    int for_dyn_last_loop_iteration;
    /* Thread next ordered index ( common for all active loop in parallel region )*/
    int next_ordered_index;
#if MPCOMP_TASK
  struct mpcomp_task_thread_infos_s task_infos;
#endif // MPCOMP_TASK
#ifdef MPCOMP_USE_INTEL_ABI
    /** Reduction methode index for Intel Runtim */
    int reduction_method;
    /** Thread private common pointer for Intel Runtime */
    struct common_table *th_pri_common;
    /** Thread private head pointer for Intel Runtime */
    struct private_common *th_pri_head;
    /** Number of threads for Intel Runtime */
    long push_num_threads;
    /* -- STATIC FOR LOOP CONSTRUCT -- */
    /** Static Loop is first iteration for Intel Runtime */
    int first_iteration;         
    /** Chunk number in static loop construct for Intel Runtime */
    int static_chunk_size_intel;
#endif /* MPCOMP_USE_INTEL_ABI */
#if 1 // OMPT_SUPPORT
    /** Thread state for OMPT */ 
	ompt_state_t state;
    /** Thread data for OMPT */ 
	ompt_data_t ompt_thread_data;
#endif /* OMPT_SUPPORT */

    /* MVP prev context */
    //int instance_ghost_rank;
    struct mpcomp_node_s* father_node;

    /** Nested thread chaining ( heap ) */
    struct mpcomp_thread_s *next;
    /** Father thread */
    struct mpcomp_thread_s *father;

    /* copy __kmpc_fork_call args */
    void** args_copy;
    /* Current size of args_copy */
    int temp_argc;

} mpcomp_thread_t;

/* Instance of OpenMP runtime */
typedef struct mpcomp_instance_s 
{
    /** Root to the tree linking the microVPs */
  	struct mpcomp_node_s *root;
    /** OpenMP information on the team */
  	struct mpcomp_team_s *team;   

    struct mpcomp_thread_s* master;

	/*-- Instance MVP --*/
    int buffered;

    /** Number of microVPs for this instance   */
    /** All microVPs of this instance  */
  	unsigned int nb_mvps;
    struct mpcomp_generic_node_s* mvps;
    int tree_array_size;
    struct mpcomp_generic_node_s* tree_array;
    /*-- Tree array informations --*/
    /** Depth of the tree */
  	int tree_depth;
    /** Degree per level of the tree */
  	int *tree_base;               
    /** Instance microVPs child number per depth */
  	int *tree_cumulative;         
    /** microVPs number at each depth */
  	int *tree_nb_nodes_per_depth;
    int *tree_first_node_per_depth;

    mpcomp_thread_t* thread_ancestor;

#if MPCOMP_TASK
    /** Task information of this instance */
    struct mpcomp_task_instance_infos_s task_infos;
#endif /* MPCOMP_TASK */

} mpcomp_instance_t;

typedef union mpcomp_node_or_leaf_u
{
    struct mpcomp_node_s **node;
    struct mpcomp_mvp_s **leaf;
} mpcomp_node_or_leaf_t;

/* OpenMP Micro VP */
typedef struct mpcomp_mvp_s 
{
    sctk_mctx_t vp_context;   /* Context including registers, stack pointer, ... */
    sctk_thread_t pid;
    /* -- MVP Thread specific informations --                   */
    /** VP on which microVP is executed                         */
    int thread_vp_idx;        
    /** MVP thread structure pointer                            */
    sctk_thread_t thread_self;
    /** MVP keep alive after fall asleep                        */
    volatile int enable;
    /** MVP spinning value in topology tree                     */
    volatile int spin_status;
    struct mpcomp_node_s* spin_node;
    /* -- MVP Tree related informations                         */
    unsigned int depth;
    /** Root of the topology tree                               */
    struct mpcomp_node_s *root;   
    /** Father node in the topology tree                        */
    struct mpcomp_node_s *father;
    /** Left sibbling mvp                                       */
    struct mpcomp_mvp_s *prev_brother;
    /* Right sibbling mvp                                       */
    struct mpcomp_mvp_s *next_brother;
    /** Which thread currently running over MVP                 */
    struct mpcomp_thread_s* threads;
    /* -- Parallel Region Information Transfert --              */
    /* Transfert OpenMP instance pointer to OpenMP thread       */
    struct mpcomp_instance_s* instance;
    /* Index in mvps instance array of last nested instance     */
    int instance_mvps_index;
#if MPCOMP_TRANSFER_INFO_ON_NODES
    /* Transfert OpenMP region information to OpenMP thread     */
    mpcomp_parallel_region_t info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
    mpcomp_thread_t* buffered_threads;
    /* -- Tree array MVP information --                         */
    /* Rank within the microVPs */
    //TODO Remove duplicate value    
    int rank;                               
    /** Rank within sibbling MVP        */ 
    int local_rank;
    int* tree_array_ancestor_path;  
    long tree_array_rank; 
    /** Rank within topology tree MVP depth level */
    int stage_rank;
    int global_rank;
    /** Size of topology tree MVP depth level    */
    int stage_size;
    int *tree_rank;                         /* Array of rank in every node of the tree */
    struct mpcomp_node_s* tree_array_father;
    int min_index[MPCOMP_AFFINITY_NB];
#if MPCOMP_TASK     /*      OMP 3.0     */
    struct mpcomp_task_mvp_infos_s task_infos;
#endif /* MPCOMP_TASK */
    struct mpcomp_mvp_rank_chain_elt_s* rank_list;
    struct mpcomp_mvp_saved_context_s*  prev_node_father;
} mpcomp_mvp_t;

struct mpcomp_meta_tree_node_s;

/* OpenMP Node */
typedef struct mpcomp_node_s 
{
    int already_init;
    /* -- MVP Thread specific informations --                   */
    struct mpcomp_meta_tree_node_s *tree_array;
    int* tree_array_ancestor_path;  
    long tree_array_rank; 
    /** MVP spinning as a node                                  */
    struct mpcomp_mvp_s *mvp;
    /** MVP spinning value in topology tree                     */
    volatile int spin_status;
    /* NUMA node on which this node is allocated                */
    int id_numa; 
    /* -- MVP Tree related information --                       */
    /** Depth in the tree                                       */
    int depth;
    /** Father node in the topology tree                        */
    int num_threads;
    int mvp_first_id;
    struct mpcomp_node_s *father;
    struct mpcomp_node_s *prev_father;
    /** Rigth brother node in the topology tree                 */
    struct mpcomp_node_s* prev_brother;
    /** Rigth brother node in the topology tree                 */
    struct mpcomp_node_s* next_brother;   
    /* Kind of children (node or leaf)                          */
    mpcomp_children_t child_type; 
    /* Number of children                                       */
    int nb_children;              
    /* Children list                                            */
    mpcomp_node_or_leaf_t children;
    /* -- Parallel Region Information Transfert --              */
    /* Transfert OpenMP instance pointer to OpenMP thread       */
    struct mpcomp_instance_s* instance;
    /* Index in mvps instance array of last nested instance     */
    int instance_mvps_index;
#if MPCOMP_TRANSFER_INFO_ON_NODES
    /* Transfert OpenMP region information to OpenMP thread     */
    mpcomp_parallel_region_t info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */

#if defined( MPCOMP_OPENMP_3_0 )
    int instance_stage_size;
    int instance_global_rank;
    int instance_stage_first_rank;
#endif /* MPCOMP_OPENMP_3_0 */
    /* -- Tree array MVP information --                         */ 
    /** Rank among children of my father -> local rank          */
    int tree_depth;
    int* tree_base;
    int* tree_cumulative;
    int* tree_nb_nodes_per_depth;
     
    long rank;                    
    int local_rank;
    int stage_rank;
    int stage_size;
    int global_rank;

    /* Flat min index of leaves in this subtree                 */
    int min_index[MPCOMP_AFFINITY_NB]; 
    /* -- Barrier specific information --                       */ 
    /** Threads number already wait on node                     */
    OPA_int_t barrier;
    /** Last generation barrier id perform on node              */
    volatile long barrier_done;
    /** Number of threads involved in the barrier               */
    volatile long barrier_num_threads;
#if MPCOMP_TASK
    struct mpcomp_task_node_infos_s task_infos;
#endif /* MPCOMP_TASK */
    struct mpcomp_node_s* mvp_prev_father;

    /* Tree reduction flag */
    volatile int* isArrived;
    /* Tree reduction data */
    void** reduce_data;

} mpcomp_node_t;

typedef struct mpcomp_node_s *mpcomp_node_ptr_t;

typedef struct mpcomp_mvp_rank_chain_elt_s
{
    struct mpcomp_mvp_rank_chain_elt_s* prev;
    unsigned int rank;
} mpcomp_mvp_rank_chain_elt_t;

typedef struct  mpcomp_mvp_saved_context_s
{
    struct mpcomp_node_s* father;
    struct mpcomp_node_s* node;
    unsigned int rank;
    struct mpcomp_mvp_saved_context_s* prev;
} mpcomp_mvp_saved_context_t;

typedef union mpcomp_node_gen_ptr_u
{
    struct mpcomp_mvp_s *mvp;
    struct mpcomp_node_s *node;
} mpcomp_node_gen_ptr_t;

typedef struct mpcomp_generic_node_s
{
    mpcomp_node_gen_ptr_t ptr;
    /* Kind of children (node or leaf) */
    mpcomp_children_t type;
} mpcomp_generic_node_t;

#ifdef __cplusplus
}
#endif

#endif /* __MPCOMP_TYPES_STRUCT_H__ */
