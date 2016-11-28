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

#if ( !defined( __SCTK_MPCOMP_TASK_H__ ) && MPCOMP_TASK )
#define __SCTK_MPCOMP_TASK_H__

#include "sctk_bool.h"
#include "sctk_spinlock.h"
#include "mpcomp_types_icv.h"
#include "mpcomp_task_macros.h"

/** Declaration  mpcomp_types.h 
 *  Break loop of include 
 */
struct mpcomp_node_s; 
struct mpcomp_thread_s; 

typedef enum mpcomp_tasklist_type_e
{
	MPCOMP_TASK_TYPE_NEW 	= 0,
	MPCOMP_TASK_TYPE_UNTIED = 1,
	MPCOMP_TASK_TYPE_COUNT 	= 2,
} mpcomp_tasklist_type_t;

/** Property of an OpenMP task */
typedef unsigned int mpcomp_task_property_t;

/* Data structures */
typedef struct mpcomp_task_taskgroup_s
{
	struct mpcomp_task_s* children;
	struct mpcomp_task_taskgroup_s* prev;
	sctk_atomics_int children_num;
} mpcomp_task_taskgroup_t;

/** OpenMP task data structure */
typedef struct mpcomp_task_s
{
    void (*func) (void *);             			/**< Function to execute 										*/
    void *data;                         		/**< Arguments of the function 								*/
	bool if_clause;
    sctk_atomics_int refcount;
    mpcomp_task_property_t property;    		/**< Task property 												*/
	struct mpcomp_task_taskgroup_s* taskgroup;
    struct mpcomp_task_s *parent;       		/**< Mother task 													*/
#if MPC_USE_SISTER_LIST
    struct mpcomp_task_s *children;     		/**< Children list 												*/
    struct mpcomp_task_s *prev_child;   		/**< Prev sister task in mother task's children list 	*/
    struct mpcomp_task_s *next_child;   		/**< Next sister task in mother task's children list 	*/
   sctk_spinlock_t children_lock;      		/**< Lock for the task's children list 					*/
#endif /* MPC_USE_SISTER_LIST */
   sctk_spinlock_t task_lock;
   struct mpcomp_thread_s *thread;     		/**< The thread owning the task 								*/
   struct mpcomp_task_list_s *list;    		/**< The current list of the task 							*/
   struct mpcomp_task_s *prev;         		/**< Prev task in the thread's task list 					*/
   struct mpcomp_task_s *next;         		/**< Next task in the thread's task list 					*/
   int depth;								/**< nested task depth 											*/
   mpcomp_local_icv_t icvs;            	/**< ICVs of the thread that create the task 			*/

#ifdef MPCOMP_OMP_4_0	/* deps infos */
	struct mpcomp_task_dep_task_infos_s* task_dep_infos;
#endif /* MPCOMP_GOMP_4_0 */

} mpcomp_task_t;

/**
 * Used by mpcomp_mvp_t and mpcomp_node_t 
 */
typedef struct mpcomp_task_node_infos_s
{
	struct mpcomp_node_s **tree_array_node;  												/**< Array representation of the tree 	*/
	unsigned tree_array_rank;           													/**< Rank in tree_array 					*/
	int *path;                          													/**< Path in the tree 						*/
    struct mpcomp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT]; 					/**< Lists of tasks 							*/
    struct mpcomp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
    int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
    struct drand48_data *tasklist_randBuffer;
} mpcomp_task_node_infos_t;

/** mvp and node share same struct */
typedef struct mpcomp_task_mvp_infos_s
{
	struct mpcomp_node_s **tree_array_node;  												/**< Array representation of the tree 	*/
	unsigned tree_array_rank;           													/**< Rank in tree_array 					*/
	int *path;                          													/**< Path in the tree 						*/
   struct mpcomp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT]; 					/**< Lists of tasks 							*/
   struct mpcomp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
   int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
   struct drand48_data *tasklist_randBuffer;
} mpcomp_task_mvp_infos_t;

/**
 * Extend mpcomp_thread_t struct for openmp task support
 */
typedef struct mpcomp_task_thread_infos_s
{
	sctk_atomics_int status;                  /**< Thread task's init tag */
	struct mpcomp_task_s *current_task;	   	/**< Currently running task */
	struct mpcomp_task_list_s *tied_tasks;   	/**< List of suspended tied tasks */
	int *larceny_order;
} mpcomp_task_thread_infos_t;

/**
 * Extend mpcomp_instance_t struct for openmp task support
 */
typedef struct mpcomp_task_instance_infos_s
{
	int array_tree_total_size;
	int *array_tree_level_first;
	int *array_tree_level_size;
} mpcomp_task_instance_infos_t;

/**
 * Extend mpcomp_team_t struct for openmp task support
 */
typedef struct mpcomp_task_team_infos_s
{
   
   int task_nesting_max;							/**< Task max depth in task generation 				*/
   int task_larceny_mode;						    /**< Task stealing policy 									*/ 
   sctk_atomics_int status;             		    /**< Thread team task's init status tag 				*/
   sctk_atomics_int use_task;             		    /**< Thread team task create task 				*/
   int tasklist_depth[MPCOMP_TASK_TYPE_COUNT];      /**< Depth in the tree of task lists 					*/
} mpcomp_task_team_infos_t;


#endif /* __SCTK_MPCOMP_TASK_H__ */
