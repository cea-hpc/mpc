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

#ifndef __MPCOMP_TYPES_STRUCT_H__
#define __MPCOMP_TYPES_STRUCT_H__

#include "mpcomp.h"
#include "sctk_context.h"
#include "sctk_atomics.h"
#include "sctk_topology.h"
#include "mpcomp_types_def.h"
#include "mpcomp_types_icv.h"
#include "mpcomp_types_loop.h"

#ifdef MPCOMP_USE_INTEL_ABI
#include "mpcomp_intel_types.h"
#endif /* MPCOMP_USE_INTEL_ABI */

#include "ompt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****  BREAK CIRCLE DEPS FOR TASK API *****/
struct mpcomp_task_mvp_infos_s;
struct mpcomp_task_node_infos_s;
struct mpcomp_task_team_infos_s;
struct mpcomp_task_thread_infos_s;
struct mpcomp_task_instance_infos_s;


/********************
 ****** Threadprivate
 ********************/

typedef struct mpcomp_atomic_int_pad_s {
  sctk_atomics_int i; /**< Value of the integer */
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
  sctk_spinlock_t update_lock;
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
#endif /* OMPT_SUPPORT */

} mpcomp_new_parallel_region_info_t;

/* Team of OpenMP threads */
typedef struct mpcomp_team_s {
  mpcomp_new_parallel_region_info_t info; /* Info for new parallel region */
  int depth; /* Depth of the current thread (0 = sequential region) */

  /* -- SINGLE/SECTIONS CONSTRUCT -- */
  sctk_atomics_int single_sections_last_current;
  void *single_copyprivate_data;

  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
  mpcomp_atomic_int_pad_t
      for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN + 1];

  /* ORDERED CONSTRUCT */
  int next_ordered_index;
  volatile long next_ordered_offset[5];
  volatile unsigned long long next_ordered_offset_ull[5];
  sctk_atomics_int next_ordered_offset_finalized[5];

#if MPCOMP_TASK
  struct mpcomp_task_team_infos_s task_infos;
#endif // MPCOMP_TASK

} mpcomp_team_t;

/** OpenMP thread struct 
 * An OpenMP thread is attached to a MVP, 
 * one thread per nested level */
typedef struct mpcomp_thread_s {
    /* -- Internal specific informations -- */
    /** OpenMP rank (0 for master thread per team) */
    long rank;      
    /* Micro-vp of the thread*/
    struct mpcomp_mvp_s *mvp; 
    /** Root node for nested parallel region */ 
    struct mpcomp_node_s* root;
    /** Father thread */
    struct mpcomp_thread_s* father; 
    /* -- Parallel region information -- */
    /** Information needed when entering a new parallel region */
    mpcomp_new_parallel_region_info_t info;
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
    sctk_atomics_int for_dyn_ull_current;
    /* Chunks array for loop dynamic schedule constructs */
    mpcomp_atomic_int_pad_t for_dyn_remain[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
    int for_dyn_total;
    /** Coordinates of target thread to steal */
    int *for_dyn_target;
    /* Shift of target thread to steal */
    int *for_dyn_shift;  
    sctk_spinlock_t *for_dyn_lock;
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
    /** Nested thread chaining ( heap ) */
    struct mpcomp_thread_s *next;
} mpcomp_thread_t;

/* Instance of OpenMP runtime */
typedef struct mpcomp_instance_s {
  hwloc_topology_t topology;

  int is_buffered;              /*!< Keep instance allocation               */
  int nb_mvps;                  /*!< Number of microVPs for this instance   */
  struct mpcomp_mvp_s **mvps;   /*!< All microVPs of this instance          */
  struct mpcomp_node_s *root;   /*!< Root to the tree linking the microVPs  */
  struct mpcomp_team_s *team;   /* Information on the team */
  int tree_depth;               /* Depth of the tree */
  int *tree_base;               /* Degree per level of the tree
                                                       (array of size 'tree_depth' */
  int *tree_cumulative;         /* Initialized in __mpcomp_build_tree */
  int *tree_nb_nodes_per_depth; /* Number of nodes at each depth ([0] = root =
                                   1) */
  int scatter_depth;         /* TODO check */
  int core_depth;            /* TODO check */
  int nb_cores;              /* TODO check */
  int balanced_last_thread;  /* TODO check */
  int balanced_current_core; /* TODO check */

#if MPCOMP_TASK
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
    /* -- MVP Thread specific informations --                   */
    /** VP on which microVP is executed                         */
    int thread_vp_idx;        
    /** MVP thread structure pointer                            */
    sctk_thread_t* thread_self;
    /** MVP keep alive after fall asleep                        */
    volatile int enable;
    /** MVP spinning value in topology tree                     */
    volatile int slave_running;
    /* -- MVP Tree related informations                         */
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
    mpcomp_new_parallel_region_info_t info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
    /* -- Tree array MVP information --                         */
    /* Rank within the microVPs */
    //TODO Remove duplicate value    
    int rank;                               
    /** Rank within sibbling MVP        */ 
    int local_rank;
    /** Rank within topology tree MVP depth level */
    int stage_rank;
    /** Size of topology tree MVP depth level    */
    int stage_size;
    int *tree_rank;                         /* Array of rank in every node of the tree */
    int min_index[MPCOMP_AFFINITY_NB];
/* OMP 3.0 */
#if MPCOMP_TASK
  struct mpcomp_task_mvp_infos_s task_infos;
#endif /* MPCOMP_TASK */
} mpcomp_mvp_t;

/* OpenMP Node */
typedef struct mpcomp_node_s 
{
    /* -- MVP Thread specific informations --                   */
    /** MVP spinning as a node                                  */
    struct mpcomp_mvp_s *mvp;
    /** MVP spinning value in topology tree                     */
    volatile int slave_running;
    /* NUMA node on which this node is allocated                */
    int id_numa; 
    /* -- MVP Tree related information --                       */
    /** Depth in the tree                                       */
    int depth;
    /** Father node in the topology tree                        */
    struct mpcomp_node_s *father;
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
    mpcomp_new_parallel_region_info_t info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
    /* -- Tree array MVP information --                         */ 
    /** Rank among children of my father -> local rank          */
    long rank;                    
    int local_rank;
    int stage_rank;
    int stage_size;
    /* Flat min index of leaves in this subtree                 */
    int min_index[MPCOMP_AFFINITY_NB]; 
    /* -- Barrier specific information --                       */ 
    /** Threads number already wait on node                     */
    sctk_atomics_int barrier;
    /** Last generation barrier id perform on node              */
    volatile long barrier_done;
    /** Number of threads involved in the barrier               */
    volatile long barrier_num_threads;
#if MPCOMP_TASK
    struct mpcomp_task_node_infos_s task_infos;
#endif /* MPCOMP_TASK */
} mpcomp_node_t;

typedef struct mpcomp_node_s *mpcomp_node_ptr_t;

#ifdef __cplusplus
}
#endif

#endif /* __MPCOMP_TYPES_STRUCT_H__ */
