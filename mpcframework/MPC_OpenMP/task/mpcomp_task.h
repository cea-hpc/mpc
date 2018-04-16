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

#include "mpcomp_types_def.h"

#if (!defined(__SCTK_MPCOMP_TASK_H__) && MPCOMP_TASK)
#define __SCTK_MPCOMP_TASK_H__

#include "stdbool.h"
#include "sctk_spinlock.h"
#include "mpcomp_types_icv.h"
#include "mpcomp_task_macros.h"

#include "ompt.h"

#define MPCOMP_USE_TASKDEP 1
#define MPCOMP_NB_REUSABLE_TASKS 100
#define MPCOMP_TASKS_DEPTH_JUMP 10
/** Declaration  mpcomp_types.h
 *  Break loop of include
 */

struct mpcomp_node_s;
struct mpcomp_thread_s;

typedef enum mpcomp_tasklist_type_e {
  MPCOMP_TASK_TYPE_NEW = 0,
  MPCOMP_TASK_TYPE_UNTIED = 1,
  MPCOMP_TASK_TYPE_COUNT = 2,
} mpcomp_tasklist_type_t;

/** Property of an OpenMP task */
typedef unsigned int mpcomp_task_property_t;

/* Data structures */
typedef struct mpcomp_task_taskgroup_s {
  struct mpcomp_task_s *children;
  struct mpcomp_task_taskgroup_s *prev;
  sctk_atomics_int children_num;
} mpcomp_task_taskgroup_t;

/** OpenMP task data structure */
typedef struct mpcomp_task_s {
  int depth;                 /**< nested task depth */
  void (*func)(void *);      /**< Function to execute */
  void *data;                /**< Arguments of the function 				    */
  sctk_atomics_int refcount; /**< refcount delay task free                   */
  mpcomp_task_property_t property; /**< Task property
                                      */
  struct mpcomp_task_s *parent;    /**< Mother task
                                      */
  struct mpcomp_thread_s
      *thread; /**< The thread owning the task 				*/
  struct mpcomp_task_list_s
      *list;                  /**< The current list of the task 				*/
#if MPCOMP_USE_LOCKFREE_QUEUE
  sctk_atomics_ptr next;
  sctk_atomics_ptr prev;
#else /* MPCOMP_USE_LOCKFREE_QUEUE */
  struct mpcomp_task_s *prev; /**< Prev task in the thread's task list */
  struct mpcomp_task_s *next; /**< Next task in the thread's task list */
#endif /* MPCOMP_USE_LOCKFREE_QUEUE */
  mpcomp_local_icv_t icvs;    /**< ICVs of the thread that create the task    */
  mpcomp_local_icv_t prev_icvs; /**< ICVs of the thread that create the task */
  struct mpcomp_task_taskgroup_s *taskgroup;
#ifdef MPCOMP_OMP_4_0 /* deps infos */
  struct mpcomp_task_dep_task_infos_s *task_dep_infos;
#endif /* MPCOMP_GOMP_4_0 */

#if 1 //OMPT_SUPPORT
	ompt_data_t 	ompt_task_data;
	ompt_frame_t 	ompt_task_frame;
	ompt_task_type_t ompt_task_type;
#endif /* OMPT_SUPPORT */
/* TODO if INTEL */
struct mpcomp_task_s *last_task_alloc; /**< last task allocated by the thread doing if0 pragma */

  bool is_stealed;
  int task_size;
  struct mpcomp_task_s *far_ancestor; 
  
} mpcomp_task_t;

/**
 * Used by mpcomp_mvp_t and mpcomp_node_t
 */
typedef struct mpcomp_task_node_infos_s {
  /** Lists of  tasks */
  struct mpcomp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT];
  struct mpcomp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
  int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
  struct drand48_data *tasklist_randBuffer;
} mpcomp_task_node_infos_t;

/** mvp and node share same struct */
typedef struct mpcomp_task_mvp_infos_s {
  /** Lists of  tasks */
  struct mpcomp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT];
  struct mpcomp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
  int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
  struct drand48_data *tasklist_randBuffer;
  int last_thief;
  int lastStolen_tasklist_rank[MPCOMP_TASK_TYPE_COUNT];
} mpcomp_task_mvp_infos_t;

/**
 * Extend mpcomp_thread_t struct for openmp task support
 */
typedef struct mpcomp_task_thread_infos_s {
  int *larceny_order;
  sctk_atomics_int status;               /**< Thread task's init tag */
  struct mpcomp_task_s *current_task;    /**< Currently running task */
  struct mpcomp_task_list_s *tied_tasks; /**< List of suspended tied tasks */
  void* opaque; /**< use mcslock buffered */
  int nb_reusable_tasks; /**< Number of current tasks reusable */
  struct mpcomp_task_s **reusable_tasks; /**< Reusable tasks buffer */
  int max_task_tot_size; /**< max task size */
  bool one_list_per_thread; /** True if there is one list for each thread */
} mpcomp_task_thread_infos_t;

/**
 * Extend mpcomp_instance_t struct for openmp task support
 */
typedef struct mpcomp_task_instance_infos_s {
  int array_tree_total_size;
  int *array_tree_level_first;
  int *array_tree_level_size;
} mpcomp_task_instance_infos_t;

/**
 * Extend mpcomp_team_t struct for openmp task support
 */
typedef struct mpcomp_task_team_infos_s {
  int task_nesting_max;      /**< Task max depth in task generation */
  int task_larceny_mode;     /**< Task stealing policy
                                */
  sctk_atomics_int status;   /**< Thread team task's init status tag */
  sctk_atomics_int use_task; /**< Thread team task create task */
  int tasklist_depth[MPCOMP_TASK_TYPE_COUNT]; /**< Depth in the tree of task
                                                 lists 			*/
} mpcomp_task_team_infos_t;

#endif /* __SCTK_MPCOMP_TASK_H__ */
