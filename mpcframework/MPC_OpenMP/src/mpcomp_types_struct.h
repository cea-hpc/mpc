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

/* OpenMP thread */
typedef struct mpcomp_thread_s {
#if 0 //  NOT USED
	sctk_mctx_t uc;		/* Context (initializes registers, ...)
											 Initialized when another thread is 
											 blocked -- NOT USED -- */
	char *stack;                  /* The stack (initialized when another
																	 thread is blocked) -- NOT USED -- */
#endif
  mpcomp_new_parallel_region_info_t
      info;                 /* Information needed when
                                                                                                                                                                   entering a new parallel
                                                                                                                                                                   region */
  long rank;                /* OpenMP rank (0 for master thread per team) */
  struct mpcomp_mvp_s *mvp; /* Micro-vp of the thread*/
  volatile int done;        /* Done value for in-order scheduler */
  struct mpcomp_instance_s
      *children_instance; /* Instance for nested parallelism */

  struct mpcomp_instance_s *instance; /* Current instance (team + tree) */

  long push_num_threads; /** Number of threads for Intel parallel region */

  /* -- SINGLE/SECTIONS CONSTRUCT -- */
  int single_sections_current;
  int single_sections_target_current; /* When should we stop the current
                                         sections construct? */
  int single_sections_start_current;  /* When started the last sections
                                         construct? */
  int nb_sections;

  struct mpcomp_thread_s
      *father; /* Father thread  TODO: use it for ancestors */

  /* LOOP CONSTRUCT */
  int schedule_type;      /* Handle scheduling type */
  int schedule_is_forced; /* Handle scheduling type from wrapper */
  /* -- STATIC FOR LOOP CONSTRUCT -- */
  int static_nb_chunks;        /* Number of total static chunk for the thread */
  int static_current_chunk;    /* Current chunk index */
  int static_chunk_size_intel; /* save nb_chunks from intel for
                                  kmp_dispatch_next */
  int first_iteration;         /* is it the first iteration ? */

  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
  int for_dyn_current; /* Current position in 'for_dyn_chunk_info' array  */
  sctk_atomics_int
      for_dyn_ull_current; /* Current position in 'for_dyn_chunk_info' array  */
  /* Chunks array for loop dynamic schedule constructs */
  mpcomp_atomic_int_pad_t for_dyn_remain[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
  int for_dyn_total;
  int *for_dyn_target; /* Coordinates of target thread to steal */
  int *for_dyn_shift;  /* Shift of target thread to steal */
  sctk_spinlock_t *for_dyn_lock;
  int for_dyn_last_loop_iteration; /* WORKAROUND (pr35196.c)
                                  Did we just execute the last
                                  iteration of the original loop? */

  /* ORDERED CONSTRUCT */
  int next_ordered_index;

#if MPCOMP_TASK
  struct mpcomp_task_thread_infos_s task_infos;
#endif // MPCOMP_TASK

/* Threadprivate */

#ifdef MPCOMP_USE_INTEL_ABI
  struct common_table *th_pri_common;
  struct private_common *th_pri_head;
#endif /* MPCOMP_USE_INTEL_ABI */

#if 1 // OMPT_SUPPORT
	ompt_state_t state;
	ompt_data_t ompt_thread_data;
#endif /* OMPT_SUPPORT */
  /* reduction method */
  int reduction_method;
} mpcomp_thread_t;

/* Instance of OpenMP runtime */
typedef struct mpcomp_instance_s {
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
  hwloc_topology_t topology;
  int scatter_depth;         /* TODO check */
  int core_depth;            /* TODO check */
  int nb_cores;              /* TODO check */
  int balanced_last_thread;  /* TODO check */
  int balanced_current_core; /* TODO check */
#if MPCOMP_TASK
  struct mpcomp_task_instance_infos_s task_infos;
#endif /* MPCOMP_TASK */
} mpcomp_instance_t;

/* OpenMP Micro VP */
typedef struct mpcomp_mvp_s {
  sctk_mctx_t vp_context; /* Context including registers, stack pointer, ... */
  sctk_thread_t pid;      /* Thread ID */
  void* tree_array_node;

#if MPCOMP_ALIGN
  mpcomp_new_parallel_region_info_t info __attribute__((aligned(128)));
#else
  mpcomp_new_parallel_region_info_t info;
#endif

  int nb_threads; /* Total number of threads running on the Micro VP */
  int next_nb_threads;
  struct mpcomp_thread_s* threads;

  int *tree_rank; /* Array of rank in every node of the tree */
  int rank;       /* Rank within the microVPs */
  int min_index[MPCOMP_AFFINITY_NB];
  int vp;        /* VP on which microVP is executed */
  char pad0[64]; /* Padding */
  volatile int enable;
  char pad1[64];                /* Padding */
  struct mpcomp_node_s *root;   /* Root of the topology tree */
  char pad1_1[64];              /* Padding */
  struct mpcomp_node_s *father; /* Father node in the topology tree */
  char pad1_2[64];              /* Padding */
  struct mpcomp_node_s *to_run; /* Specify where the spin waiting for a new
                                   parallel region */
  char pad2[64];                /* Padding */
  volatile int slave_running;
  char pad3[64]; /* Padding */

/* OMP 3.0 */
#if MPCOMP_TASK
  struct mpcomp_task_mvp_infos_s task_infos;
#endif /* MPCOMP_TASK */

} mpcomp_mvp_t;

/* OpenMP Node */
typedef struct mpcomp_node_s {
  struct mpcomp_node_s *father; /* Father in the topology tree */
  long rank;                    /* Rank among children of my father */
  int depth;                    /* Depth in the tree */
  int nb_children;              /* Number of children */
  void* tree_array_node;

#if MPCOMP_ALIGN
  mpcomp_new_parallel_region_info_t info __attribute__((aligned(128)));
#else
  mpcomp_new_parallel_region_info_t info;
#endif

  /* The following indices correspond to the 'rank' value in microVPs */
  int min_index
      [MPCOMP_AFFINITY_NB]; /* Flat min index of leaves in this subtree */

  mpcomp_instance_t *instance; /* Information on the whole team */

  mpcomp_children_t child_type; /* Kind of children (node or leaf) */
  union node_or_leaf {
    struct mpcomp_node_s **node;
    struct mpcomp_mvp_s **leaf;
  } children; /* Children list */

#if MPCOMP_ALIGN
  volatile int slave_running __attribute__((aligned(128)));
#else
  char pad0[64]; /* Padding */
  volatile int slave_running;
  char pad1[64]; /* Padding */
#endif

#if MPCOMP_ALIGN
  sctk_atomics_int barrier
      __attribute__((aligned(128))); /* Barrier for the child team */
#else
  sctk_atomics_int barrier;
  char pad2[64]; /* Padding */
#endif

#if MPCOMP_ALIGN
  volatile long barrier_done __attribute__((
      aligned(128))); /* Is the barrier (for the child team) over? */
#else
  volatile long barrier_done;
  char pad3[64]; /* Padding */
#endif

#if MPCOMP_ALIGN
  volatile long barrier_num_threads __attribute__((
      aligned(128))); /* Number of threads involved in the barrier */
#else
  volatile long
      barrier_num_threads; /* Number of threads involved in the barrier */
  char pad4[64];           /* Padding */
#endif

#if MPCOMP_TASK
  struct mpcomp_task_node_infos_s task_infos;
#endif /* MPCOMP_TASK */

  int id_numa; /* NUMA node on which this node is allocated */

} mpcomp_node_t;

typedef struct mpcomp_node_s *mpcomp_node_ptr_t;

#ifdef __cplusplus
}
#endif

#endif /* __MPCOMP_TYPES_STRUCT_H__ */
