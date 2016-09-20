#include "mpcomp_macros.h"

#if ( !defined( __SCTK_MPCOMP_TASK_H__ ) && MPCOMP_TASK )
#define __SCTK_MPCOMP_TASK_H__

#include "mpcomp_enum_types.h"
#include "mpcomp_task_macros.h"

/** Declaration in mpcomp_internal.h 
 *  Break loop of include 
 */
struct mpcomp_node_s; 
struct mpcomp_thread_s; 

typedef enum mpcomp_tasklist_type_t 
{
	MPCOMP_TASK_TYPE_NEW 	= 0,
	MPCOMP_TASK_TYPE_UNTIED = 1,
	MPCOMP_TASK_TYPE_COUNT 	= 2,
} mpcomp_tasklist_type_t;

/** Property of an OpenMP task */
typedef unsigned int mpcomp_task_property_t;

/** OpenMP task data structure */
typedef struct mpcomp_task_s
{
	void (*func) (void *);             	/**< Function to execute 										*/
   void *data;                         /**< Arguments of the function 								*/
   mpcomp_task_property_t property;    /**< Task property 												*/
   struct mpcomp_task_s *parent;       /**< Mother task 													*/
   struct mpcomp_task_s *children;     /**< Children list 												*/
   struct mpcomp_task_s *prev_child;   /**< Prev sister task in mother task's children list 	*/
   struct mpcomp_task_s *next_child;   /**< Next sister task in mother task's children list 	*/
   sctk_spinlock_t children_lock;      /**< Lock for the task's children list 					*/
   struct mpcomp_thread_s *thread;     /**< The thread owning the task 								*/
   struct mpcomp_task_list_s *list;    /**< The current list of the task 							*/
   struct mpcomp_task_s *prev;         /**< Prev task in the thread's task list 					*/
   struct mpcomp_task_s *next;         /**< Next task in the thread's task list 					*/
   int depth;									/**< nested task depth 											*/
	mpcomp_local_icv_t icvs;            /**< ICVs of the thread that create the task 			*/

	/*TODO remove after rebase */
   int isGhostTask;							
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
typedef struct mpcomp_task_node_infos_s mpcomp_task_mvp_infos_t;

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
