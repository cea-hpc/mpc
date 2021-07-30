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

#ifndef __MPCOMP_TYPES_H__
#define __MPCOMP_TYPES_H__

#define AD_USE_LOCK

#include <stdbool.h>
#include "mpc_common_debug.h"
#include <mpc_common_asm.h>
#include "sctk_context.h"
#include "mpc_common_types.h"
#include "mpc_omp.h"
#include "mpc_omp_task_property.h"
#include "sctk_alloc.h"
#include "mpc_common_mcs.h"
#include "mpcompt_instance_types.h"
#include "mpcompt_frame_types.h"

#include <stdbool.h>

// #define TLS_ALLOCATORS

#ifdef MPCOMP_USE_INTEL_ABI
	#include "omp_intel_types.h"
#endif /* MPCOMP_USE_INTEL_ABI */

/*******************
 * OMP DEFINITIONS *
 *******************/

#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 1

#define MPCOMP_USE_INTEL_ABI 1

/* Maximum number of alive 'for dynamic' and 'for guided'  construct */
#define MPCOMP_MAX_ALIVE_FOR_DYN 3

#define MPCOMP_NOWAIT_STOP_SYMBOL ( -2 )

/* Uncomment to enable coherency checking */
#define MPCOMP_COHERENCY_CHECKING 0
#define MPCOMP_OVERFLOW_CHECKING 0

/* MACRO FOR PERFORMANCE */
#define MPCOMP_MALLOC_ON_NODE 1
#define MPCOMP_TRANSFER_INFO_ON_NODES 0
#define MPCOMP_ALIGN 1

#define MPCOMP_CHUNKS_NOT_AVAIL 1
#define MPCOMP_CHUNKS_AVAIL 2

/* Define macro MPCOMP_MIC in case of Xeon Phi compilation */
#if __MIC__ || __MIC2__
	#define MPCOMP_MIC 1
#endif /* __MIC__ || __MIC2__ */

#define MPCOMP_NB_REUSABLE_TASKS 100
#define MPCOMP_TASKS_DEPTH_JUMP 10

#if KMP_ARCH_X86
#define KMP_SIZE_T_MAX ( 0xFFFFFFFF )
#else
#define KMP_SIZE_T_MAX ( 0xFFFFFFFFFFFFFFFF )
#endif

#define MPCOMP_OMP_4_0

#define MPCOMP_TASK_DEFAULT_ALIGN 8

#define MPCOMP_TASK_MAX_DELAYED 600

/*** Tasks property bitmasks ***/

/* Task list type and lock type */

#define MPCOMP_USE_MCS_LOCK 1

#define MPCOMP_TASK_DEP_MPC_HTABLE_SIZE 1001 /* 7 * 11 * 13 */
#define MPCOMP_TASK_DEP_MPC_HTABLE_SEED 2

#define MPCOMP_TASK_DEP_LOCK_NODE(node) mpc_common_spinlock_lock(&(node->lock))
#define MPCOMP_TASK_DEP_UNLOCK_NODE(node) mpc_common_spinlock_unlock(&(node->lock))

enum mpc_omp_task_larceny_mode_t
{
    MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL,
    MPCOMP_TASK_LARCENY_MODE_RANDOM,
    MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER,
    MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN,
    MPCOMP_TASK_LARCENY_MODE_PRODUCER,
    MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER,
    MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM,
    MPCOMP_TASK_LARCENY_MODE_COUNT
};

/* OpenMP 5.0 Memory Management */
#define MPCOMP_MAX_ALLOCATORS 256

/****************
 * OPENMP ENUMS *
 ****************/

typedef enum
{
	MPCOMP_META_TREE_UNDEF      = 0,
	MPCOMP_META_TREE_NODE       = 1,
	MPCOMP_META_TREE_LEAF       = 2,
	MPCOMP_META_TREE_NONE       = 3
} mpc_omp_meta_tree_type_t;


typedef enum mpc_omp_combined_mode_e
{
	MPCOMP_COMBINED_NONE = 0,
	MPCOMP_COMBINED_SECTION = 1,
	MPCOMP_COMBINED_STATIC_LOOP = 2,
	MPCOMP_COMBINED_DYN_LOOP = 3,
	MPCOMP_COMBINED_GUIDED_LOOP = 4,
	MPCOMP_COMBINED_RUNTIME_LOOP = 5,
	MPCOMP_COMBINED_COUNT = 6
} mpc_omp_combined_mode_t;

/* Type of element in the stack for dynamic work stealing */
typedef enum mpc_omp_elem_stack_type_e
{
	MPCOMP_ELEM_STACK_NODE = 1,
	MPCOMP_ELEM_STACK_LEAF = 2,
	MPCOMP_ELEM_STACK_COUNT = 3,
} mpc_omp_elem_stack_type_t;

/* Type of children in the topology tree */
typedef enum mpc_omp_children_e
{
	MPCOMP_CHILDREN_NULL = 0,
	MPCOMP_CHILDREN_NODE = 1,
	MPCOMP_CHILDREN_LEAF = 2,
} mpc_omp_children_t;

typedef enum mpc_omp_context_e
{
	MPCOMP_CONTEXT_IN_ORDER = 1,
	MPCOMP_CONTEXT_OUT_OF_ORDER_MAIN = 2,
	MPCOMP_CONTEXT_OUT_OF_ORDER_SUB = 3,
} mpc_omp_context_t;

typedef enum mpc_omp_topo_obj_type_e
{
	MPCOMP_TOPO_OBJ_SOCKET,
	MPCOMP_TOPO_OBJ_CORE,
	MPCOMP_TOPO_OBJ_THREAD,
	MPCOMP_TOPO_OBJ_COUNT
} mpc_omp_topo_obj_type_t;

typedef enum mpc_omp_mode_e
{
	MPCOMP_MODE_SIMPLE_MIXED,
	MPCOMP_MODE_OVERSUBSCRIBED_MIXED,
	MPCOMP_MODE_ALTERNATING,
	MPCOMP_MODE_FULLY_MIXED,
	MPCOMP_MODE_COUNT
} mpc_omp_mode_t;

typedef enum mpc_omp_affinity_e
{
	MPCOMP_AFFINITY_COMPACT,  /* Distribute over logical PUs */
	MPCOMP_AFFINITY_SCATTER,  /* Distribute over memory controllers */
	MPCOMP_AFFINITY_BALANCED, /* Distribute over physical PUs */
	MPCOMP_AFFINITY_NB
} mpc_omp_affinity_t;

/*********
 * new meta elt for task initialisation
 */

typedef enum mpc_omp_tree_meta_elt_type_e
{
	MPCOMP_TREE_META_ELT_MVP,
	MPCOMP_TREE_META_ELT_NODE,
	MPCOMP_TREE_META_ELT_COUNT
} mpc_omp_tree_meta_elt_type_t;

typedef union mpc_omp_tree_meta_elt_ptr_u
{
	struct mpc_omp_node_s *node;
	struct mpc_omp_mvp_s *mvp;
} mpc_omp_tree_meta_elt_ptr_t;

typedef struct mpc_omp_tree_meta_elt_s
{
	mpc_omp_tree_meta_elt_type_t type;
	mpc_omp_tree_meta_elt_ptr_t ptr;
} mpc_omp_tree_meta_elt_t;

typedef enum
{
	MPCOMP_MVP_STATE_UNDEF = 0,
	MPCOMP_MVP_STATE_SLEEP,
	MPCOMP_MVP_STATE_AWAKE,
	MPCOMP_MVP_STATE_READY
} mpc_omp_mvp_state_t;


typedef enum mpc_omp_loop_gen_type_e
{
	MPCOMP_LOOP_TYPE_LONG,
	MPCOMP_LOOP_TYPE_ULL,
} mpc_omp_loop_gen_type_t;

/*** MPCOMP_TASK_INIT_STATUS ***/

typedef enum mpc_omp_task_init_status_e {
  MPCOMP_TASK_INIT_STATUS_UNINITIALIZED,
  MPCOMP_TASK_INIT_STATUS_INIT_IN_PROCESS,
  MPCOMP_TASK_INIT_STATUS_INITIALIZED,
  MPCOMP_TASK_INIT_STATUS_COUNT
} mpc_omp_task_init_status_t;

typedef enum mpc_omp_tasklist_type_e
{
	MPCOMP_TASK_TYPE_NEW = 0,
	MPCOMP_TASK_TYPE_UNTIED = 1,
	MPCOMP_TASK_TYPE_COUNT = 2,
} mpc_omp_tasklist_type_t;

/**
 * Don't modify order NONE < IN < OUT */
typedef enum mpc_omp_task_dep_type_e {
  MPCOMP_TASK_DEP_NONE = 0,
  MPCOMP_TASK_DEP_IN = 1,
  MPCOMP_TASK_DEP_OUT = 2,
  MPCOMP_TASK_DEP_COUNT = 3,
} mpc_omp_task_dep_type_t;

typedef enum mpc_omp_task_dep_task_status_e {
  MPCOMP_TASK_DEP_TASK_PROCESS_DEP = 0,
  MPCOMP_TASK_DEP_TASK_NOT_EXECUTE = 1,
  MPCOMP_TASK_DEP_TASK_RELEASED = 2,
  MPCOMP_TASK_DEP_TASK_FINALIZED = 3,
  MPCOMP_TASK_DEP_TASK_COUNT = 4
} mpc_omp_task_dep_task_status_t;

/**********************
 * OMPT STATUS        *
 **********************/

typedef enum mpc_omp_tool_status_e {
    mpc_omp_tool_disabled = 0,
    mpc_omp_tool_enabled  = 1
} mpc_omp_tool_status_t;

/**********************
 * STRUCT DEFINITIONS *
 **********************/

/* Global Internal Control Variables
 * One structure per OpenMP instance */
typedef struct mpc_omp_global_icv_s
{
	omp_sched_t def_sched_var;  /**< Default schedule when no 'schedule' clause is
                                 present 			*/
	int bind_var;				/**< Is the OpenMP threads bound to cpu cores
                                 */
	int stacksize_var;			/**< Size of the stack per thread (in Bytes)
                                 */
	int active_wait_policy_var; /**< Is the threads wait policy active or passive
                                 */
	int thread_limit_var;		/**< Number of Open threads to use for the whole program
                           */
	int max_active_levels_var;  /**< Maximum number of nested active parallel
                                regions 				*/
	int nmicrovps_var;			/**< Number of VPs
                                */
	int warn_nested;			/**< Emit warning for nested parallelism?
                                */
	int affinity;				/**< OpenMP threads affinity
                                */
	mpc_omp_tool_status_t tool;  /**< OpenMP Tool interface (enabled/disabled)
                                */
	char* tool_libraries;	   	/**< OpenMP Tool paths (absolutes paths)
                                */
} mpc_omp_global_icv_t;

/** Local Internal Control Variables
 * One structure per OpenMP thread 				*/
typedef struct mpc_omp_local_icv_s
{
	unsigned int nthreads_var; /**< Number of threads for the next team creation
                                */
	int dyn_var;			   /**< Is dynamic thread adjustement on?
                                */
	int nest_var;			   /**< Is nested OpenMP handled/allowed?
                                */
	omp_sched_t run_sched_var; /**< Schedule to use when a 'schedule' clause is
                                set to 'runtime' */
	int modifier_sched_var;	/**< Size of chunks for loop schedule
                                */
	int active_levels_var;	 /**< Number of nested, active enclosing parallel
                                regions 			*/
	int levels_var;			   /**< Number of nested enclosing parallel regions
                                */
  omp_allocator_handle_t def_allocator_var ; /**< Memory allocator to be used by
											   memory allocation routines
											   */
} mpc_omp_local_icv_t;

typedef struct mpc_omp_tree_array_global_info_s
{
	int *tree_shape;
	int max_depth;
} mpc_omp_tree_array_global_info_t;

typedef struct mpc_omp_meta_tree_node_s
{
	/* Generic information */
	mpc_omp_meta_tree_type_t type;
	/* Father information */
	int *fathers_array;
	unsigned int fathers_array_size;
	/* Children information */
	int *first_child_array;
	unsigned int *children_num_array;
	unsigned int children_array_size;

	/* Places */

	/* Min index */
	void *user_pointer;
	OPA_int_t children_ready;
} mpc_omp_meta_tree_node_t;

typedef struct mpc_omp_mvp_thread_args_s
{
	unsigned int rank;
	unsigned int pu_id;
	mpc_omp_meta_tree_node_t *array;
	unsigned int place_id;
	unsigned int target_vp;
	unsigned int place_depth;
  unsigned int mpi_rank;
#if OMPT_SUPPORT
    /* Transfer common tool instance infos */
    mpc_omp_ompt_tool_status_t tool_status;
    mpc_omp_ompt_tool_instance_t* tool_instance;
#endif /* OMPT_SUPPORT */
} mpc_omp_mvp_thread_args_t;

/** Property of an OpenMP task */
typedef unsigned int mpc_omp_task_property_t;

/* Data structures */
typedef struct mpc_omp_task_taskgroup_s
{
	struct mpc_omp_task_s *children;
	struct mpc_omp_task_taskgroup_s *prev;
	OPA_int_t children_num;
} mpc_omp_task_taskgroup_t;

#define MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING 128

#ifdef MPCOMP_USE_MCS_LOCK
	typedef sctk_mcslock_t mpc_omp_task_lock_t;
#else  /* MPCOMP_USE_MCS_LOCK */
	typedef mpc_common_spinlock_t mpc_omp_task_lock_t;
#endif /* MPCOMP_USE_MCS_LOCK */

/** OpenMP task list data structure */
typedef struct mpc_omp_task_list_s
{
	OPA_ptr_t lockfree_head;
	OPA_ptr_t lockfree_tail;
	char pad1[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING - 2 * sizeof( OPA_ptr_t )];
	OPA_ptr_t lockfree_shadow_head;
	char pad2[MPCOMP_TASK_LOCKFREE_CACHELINE_PADDING - 1 * sizeof( OPA_ptr_t )];
	OPA_int_t nb_elements;					  /**< Number of tasks in the list */
	mpc_omp_task_lock_t mpcomp_task_lock; /**< Lock of the list                                 */
	int total;
	struct mpc_omp_task_s *head; /**< First task of the list */
	struct mpc_omp_task_s *tail; /**< Last task of the list */
	OPA_int_t nb_larcenies;		/**< Number of tasks in the list */
} mpc_omp_task_list_t;

static inline void __task_list_reset( mpc_omp_task_list_t *list )
{
	memset( list, 0, sizeof( mpc_omp_task_list_t ) );
}


/** OpenMP task data structure */
typedef struct mpc_omp_task_s
{
	int depth;						 /**< nested task depth */
	void ( *func )( void * );		 /**< Function to execute */
	void *func_data;				 /**< Arguments of the function 				    */
	void *data;
	size_t data_size;
	OPA_int_t refcount;				 /**< refcount delay task free                   */
	mpc_omp_task_property_t property; /**< Task property
                                      */
	struct mpc_omp_task_s *parent;	/**< Mother task
                                      */
	struct mpc_omp_thread_s
		*thread; /**< The thread owning the task 				*/
	struct mpc_omp_task_list_s
		*list; /**< The current list of the task 				*/

	OPA_ptr_t lockfree_next;	/**< Prev task in the thread's task list if lists are lockfree */
	OPA_ptr_t lockfree_prev;	/**< Next task in the thread's task list if lists are lockfree */
	struct mpc_omp_task_s *prev; /**< Prev task in the thread's task list if lists are locked */
	struct mpc_omp_task_s *next; /**< Next task in the thread's task list if lists are locked */

	mpc_omp_local_icv_t icvs;	  /**< ICVs of the thread that create the task    */
	mpc_omp_local_icv_t prev_icvs; /**< ICVs of the thread that create the task */
	struct mpc_omp_task_taskgroup_s *taskgroup;
#ifdef MPCOMP_OMP_4_0 /* deps infos */
	struct mpc_omp_task_dep_task_infos_s *task_dep_infos;
#endif /* MPCOMP_GOMP_4_0 */
	/* TODO if INTEL */
	struct mpc_omp_task_s *last_task_alloc; /**< last task allocated by the thread doing if0 pragma */

	bool is_stealed;					/**< is task have been stealed or not */
	int task_size;						/**< Size of allocation task */
	struct mpc_omp_task_s *far_ancestor; /**< far parent (MPCOMP_TASKS_DEPTH_JUMP) of the task */
#if OMPT_SUPPORT
    /* Task data and type */
    ompt_data_t ompt_task_data;
    ompt_task_flag_t ompt_task_type;
    /* Task frame */
    ompt_frame_t ompt_task_frame;
#endif /* OMPT_SUPPORT */
} mpc_omp_task_t;

/**
 * Used by mpc_omp_mvp_t and mpc_omp_node_t
 */
typedef struct mpc_omp_task_node_infos_s
{
	/** Lists of  tasks */
	struct mpc_omp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT];
	struct mpc_omp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
	int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
	struct drand48_data *tasklist_randBuffer;
} mpc_omp_task_node_infos_t;

/** mvp and node share same struct */
typedef struct mpc_omp_task_mvp_infos_s
{
	/** Lists of  tasks */
	struct mpc_omp_task_list_s *tasklist[MPCOMP_TASK_TYPE_COUNT];
	struct mpc_omp_task_list_s *lastStolen_tasklist[MPCOMP_TASK_TYPE_COUNT];
	int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];
	struct drand48_data *tasklist_randBuffer;
	int last_thief;
	int lastStolen_tasklist_rank[MPCOMP_TASK_TYPE_COUNT];
} mpc_omp_task_mvp_infos_t;

/**
 * Extend mpc_omp_thread_t struct for openmp task support
 */
typedef struct mpc_omp_task_thread_infos_s
{
	int *larceny_order;
	OPA_int_t status;					   /**< Thread task's init tag */
	struct mpc_omp_task_s *current_task;	/**< Currently running task */
	struct mpc_omp_task_list_s *tied_tasks; /**< List of suspended tied tasks */
	void *opaque;						   /**< use mcslock buffered */
	int nb_reusable_tasks;				   /**< Number of current tasks reusable */
	struct mpc_omp_task_s **reusable_tasks; /**< Reusable tasks buffer */
	int max_task_tot_size;				   /**< max task size */
	bool one_list_per_thread;			   /** True if there is one list for each thread */
	size_t sizeof_kmp_task_t;
} mpc_omp_task_thread_infos_t;

/**
 * Extend mpc_omp_instance_t struct for openmp task support
 */
typedef struct mpc_omp_task_instance_infos_s
{
	int array_tree_total_size;
	int *array_tree_level_first;
	int *array_tree_level_size;
	bool is_initialized;
} mpc_omp_task_instance_infos_t;

/**
 * Extend mpc_omp_team_t struct for openmp task support
 */
typedef struct mpc_omp_task_team_infos_s
{
	int task_nesting_max;						/**< Task max depth in task generation */
	int task_larceny_mode;						/**< Task stealing policy
                                */
	OPA_int_t status;							/**< Thread team task's init status tag */
	OPA_int_t use_task;							/**< Thread team task create task */
	int tasklist_depth[MPCOMP_TASK_TYPE_COUNT]; /**< Depth in the tree of task
                                                 lists 			*/
} mpc_omp_task_team_infos_t;


typedef struct mpc_omp_task_dep_node_s {
  mpc_common_spinlock_t lock;
  OPA_int_t ref_counter;
  OPA_int_t predecessors;
  struct mpc_omp_task_s *task;
  OPA_int_t status;
  struct mpc_omp_task_dep_node_list_s *successors;
  bool if_clause;
#if OMPT_SUPPORT
  /* Task dependences record */
  ompt_dependence_t* ompt_task_deps;
#endif /* OMPT_SUPPORT */
} mpc_omp_task_dep_node_t;

typedef struct mpc_omp_task_dep_node_list_s {
  mpc_omp_task_dep_node_t *node;
  struct mpc_omp_task_dep_node_list_s *next;
} mpc_omp_task_dep_node_list_t;

typedef struct mpc_omp_task_dep_ht_entry_s {
  uintptr_t base_addr;
  mpc_omp_task_dep_node_t *last_out;
  mpc_omp_task_dep_node_list_t *last_in;
  struct mpc_omp_task_dep_ht_entry_s *next;
} mpc_omp_task_dep_ht_entry_t;

typedef struct mpc_omp_task_dep_ht_bucket_s {
  int num_entries;
  uint64_t base_addr;
  bool redundant_out;
  uint64_t *base_addr_list;
  mpc_omp_task_dep_type_t type;
  mpc_omp_task_dep_ht_entry_t *entry;
} mpc_omp_task_dep_ht_bucket_t;

/**
 * \param[in] 	addr	uintptr_t	depend addr ptr
 * \param[in] 	size	uint32_t 	max hash size value
 * \param[in] 	seed	uint32_t 	seed value (can be ignored)
 * \return 		hash 	uint32_t 	hash computed value
 */
typedef uint32_t (*mpc_omp_task_dep_hash_func_t)(uintptr_t, uint32_t, uint32_t);

typedef struct mpc_omp_task_dep_ht_table_s {
  uint32_t hsize;
  uint32_t hseed;
  mpc_common_spinlock_t hlock;
  mpc_omp_task_dep_hash_func_t hfunc;
  struct mpc_omp_task_dep_ht_bucket_s *buckets;
} mpc_omp_task_dep_ht_table_t;

typedef struct mpc_omp_task_dep_task_infos_s {
  mpc_omp_task_dep_node_t *node;
  struct mpc_omp_task_dep_ht_table_s *htable;
} mpc_omp_task_dep_task_infos_t;

typedef struct mpc_omp_task_stealing_funcs_s {
  char *namePolicy;
  int allowMultipleVictims;
  int (*pre_init)(void);
  int (*get_victim)(int globalRank, int index, mpc_omp_tasklist_type_t type);
}mpc_omp_task_stealing_funcs_t;

/* Loops */


typedef struct mpc_omp_loop_long_iter_s
{
	bool up;
	long lb;		 /* Lower bound          */
	long b;			 /* Upper bound          */
	long incr;		 /* Step                 */
	long chunk_size; /* Size of each chunk   */
	long cur_ordered_iter;
} mpc_omp_loop_long_iter_t;

typedef struct mpc_omp_loop_ull_iter_s
{
	bool up;
	unsigned long long lb;		   /* Lower bound              */
	unsigned long long b;		   /* Upper bound              */
	unsigned long long incr;	   /* Step                     */
	unsigned long long chunk_size; /* Size of each chunk       */
	unsigned long long cur_ordered_iter;
} mpc_omp_loop_ull_iter_t;

typedef union mpc_omp_loop_gen_iter_u
{
	mpc_omp_loop_ull_iter_t mpcomp_ull;
	mpc_omp_loop_long_iter_t mpcomp_long;
} mpc_omp_loop_gen_iter_t;

typedef struct mpc_omp_loop_gen_info_s
{
	int fresh;
	int ischunked;
	mpc_omp_loop_gen_type_t type;
	mpc_omp_loop_gen_iter_t loop;
} mpc_omp_loop_gen_info_t;

/********************
 ****** OpenMP 5.0
 ****** Memory Management
 ********************/

/* Combine a memspace to a full set of traits */
typedef struct mpc_omp_alloc_s
{
  omp_memspace_handle_t memspace;
  omp_alloctrait_t traits[omp_atk_partition]; // Assume here that omp_atk_partition is the last argument of the omp_alloctrait_t enum
} mpc_omp_alloc_t;

typedef struct mpc_omp_recycl_alloc_info_s
{
  int idx_list[MPCOMP_MAX_ALLOCATORS];
  int nb_recycl_allocators;
} mpc_omp_recycl_alloc_info_t;

typedef struct mpc_omp_alloc_list_s
{
  mpc_omp_alloc_t list[MPCOMP_MAX_ALLOCATORS];
  int nb_init_allocators;
  int last_index;
  mpc_omp_recycl_alloc_info_t recycling_info;
	mpc_common_spinlock_t lock;
} mpc_omp_alloc_list_t;

/********************
 ****** Threadprivate
 ********************/

typedef struct mpc_omp_atomic_int_pad_s
{
	OPA_int_t i; /**< Value of the integer */
	char pad[8]; /* Padding */
} mpc_omp_atomic_int_pad_t;

/* Information to transfer to every thread
 * when entering a new parallel region
 *
 * WARNING: these variables should be written in sequential part
 * and read in parallel region.
 */
typedef struct mpc_omp_new_parallel_region_info_s
{
	/* MANDATORY INFO */
	void *( *func )( void * ); /* Function to call by every thread */
	void *shared;			   /* Shared variables (for every thread) */
	long num_threads;		   /* Current number of threads in the team */
	struct mpc_omp_new_parallel_region_info_s *parent;
	struct mpc_omp_node_s *new_root;
	int single_sections_current_save;
	mpc_common_spinlock_t update_lock;
	int for_dyn_current_save;
	long combined_pragma;
	mpc_omp_local_icv_t icvs; /* Set of ICVs for the child team */

	/* META OPTIONAL INFO FOR ULL SUPPORT */
	mpc_omp_loop_gen_info_t loop_infos;
	/* OPTIONAL INFO */
	long loop_lb;		  /* Lower bound */
	long loop_b;		  /* Upper bound */
	long loop_incr;		  /* Step */
	long loop_chunk_size; /* Size of each chunk */
	int nb_sections;
#if OMPT_SUPPORT
    /* Parallel region data */
    ompt_data_t ompt_parallel_data;
    /* Single contruct fields specific to OMPT
     * Used to trigger work end event in barrier */
    int doing_single;
    int in_single;
#endif /* OMPT_SUPPORT */
} mpc_omp_parallel_region_t;

static inline void
_mpc_omp_parallel_region_infos_reset( mpc_omp_parallel_region_t *info )
{
	assert( info );
	memset( info, 0, sizeof( mpc_omp_parallel_region_t ) );
}

static inline void
_mpc_omp_parallel_region_infos_init( mpc_omp_parallel_region_t *info )
{
	assert( info );
	_mpc_omp_parallel_region_infos_reset( info );
	info->combined_pragma = MPCOMP_COMBINED_NONE;
}

/* Team of OpenMP threads */
typedef struct mpc_omp_team_s
{
	mpc_omp_parallel_region_t info; /* Info for new parallel region */
	int depth;					   /* Depth of the current thread (0 = sequential region) */
	int id;						   /* team unique id */

	/* -- SINGLE/SECTIONS CONSTRUCT -- */
	OPA_int_t single_sections_last_current;
	void *single_copyprivate_data;

	/* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	mpc_omp_atomic_int_pad_t
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
  
	/* ATOMIC/CRITICAL CONSTRUCT */
  mpc_common_spinlock_t atomic_lock;
  omp_lock_t* critical_lock;
  
	struct mpc_omp_task_team_infos_s task_infos;

#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    /* Frame struct to transfert infos at parallel region */
    mpc_omp_ompt_frame_info_t frame_infos;
#endif /* OMPT_SUPPORT */
} mpc_omp_team_t;

/** OpenMP thread struct
 * An OpenMP thread is attached to a MVP,
 * one thread per nested level */
typedef struct mpc_omp_thread_s
{

	/* -- Internal specific informations -- */
	unsigned int depth;
	/** OpenMP rank (0 for master thread per team) */
	long rank;
	long tree_array_rank;
	int *tree_array_ancestor_path;
	/* Micro-vp of the thread*/
	struct mpc_omp_mvp_s *mvp;
	/** Root node for nested parallel region */
	struct mpc_omp_node_s *root;
	/* -- Parallel region information -- */
	/** Information needed when entering a new parallel region */
	mpc_omp_parallel_region_t info;
	/** Current instance (team + tree) */
	struct mpc_omp_instance_s *instance;
	/** Instance for nested parallelism */
	struct mpc_omp_instance_s *children_instance;
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
	mpc_omp_atomic_int_pad_t for_dyn_remain[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
	/* Total number of chunks of the thread */
	unsigned long long for_dyn_total[MPCOMP_MAX_ALIVE_FOR_DYN + 1];
	/** Coordinates of target thread to steal */
	int *for_dyn_target;
	/* last target */
	struct mpc_omp_thread_s *dyn_last_target;
	/* Shift of target thread to steal */
	int *for_dyn_shift;
	mpc_common_spinlock_t *for_dyn_lock;
	/* Did we just execute the last iteration of the original loop? ( pr35196.c )*/
	int for_dyn_last_loop_iteration;
	/* Thread next ordered index ( common for all active loop in parallel region )*/
	int next_ordered_index;
	struct mpc_omp_task_thread_infos_s task_infos;
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

  /* OpenMP 5.0 -- Memory Management */

  omp_allocator_handle_t default_allocator;
  // Allocators set (including default memory allocator)
  //  - 1 x (Memspace + Traits) / allocator_handle
  // struct mpc_omp_alloc_s *alloc_set;
#ifdef TLS_ALLOCATORS
  struct mpc_omp_alloc_list_s allocators;
#endif

	/* MVP prev context */
	//int instance_ghost_rank;
	struct mpc_omp_node_s *father_node;

	/** Nested thread chaining ( heap ) */
	struct mpc_omp_thread_s *next;
	/** Father thread */
	struct mpc_omp_thread_s *father;

	/* copy __kmpc_fork_call args */
	void **args_copy;
	/* Current size of args_copy */
	int temp_argc;
#if OMPT_SUPPORT
    /* Thread state */
    ompt_state_t state;
    /* Wait id */
    ompt_wait_id_t wait_id;
    /* Thread data */
    ompt_data_t ompt_thread_data;
#if MPCOMPT_HAS_FRAME_SUPPORT
    /* Frame addr and frame return addr infos */
    mpc_omp_ompt_frame_info_t frame_infos;
#endif
    /* Common tool instance status */
    mpc_omp_ompt_tool_status_t tool_status;
    /* Common tool instance infos */
    mpc_omp_ompt_tool_instance_t* tool_instance;
#endif /* OMPT_SUPPORT */
} mpc_omp_thread_t;

/* Instance of OpenMP runtime */
typedef struct mpc_omp_instance_s
{
	/** Root to the tree linking the microVPs */
	struct mpc_omp_node_s *root;
	/** OpenMP information on the team */
	struct mpc_omp_team_s *team;

	struct mpc_omp_thread_s *master;

	/*-- Instance MVP --*/
	int buffered;

	/** Number of microVPs for this instance   */
	/** All microVPs of this instance  */
	unsigned int nb_mvps;
	struct mpc_omp_generic_node_s *mvps;
	int tree_array_size;
	struct mpc_omp_generic_node_s *tree_array;
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

	mpc_omp_thread_t *thread_ancestor;

	/** Task information of this instance */
	struct mpc_omp_task_instance_infos_s task_infos;

} mpc_omp_instance_t;

typedef union mpc_omp_node_or_leaf_u
{
	struct mpc_omp_node_s **node;
	struct mpc_omp_mvp_s **leaf;
} mpc_omp_node_or_leaf_t;

typedef struct worker_workshare_s
{
  void* workshare;
  int mpi_rank;
} worker_workshare_t;


/* OpenMP Micro VP */
typedef struct mpc_omp_mvp_s
{
	sctk_mctx_t vp_context; /* Context including registers, stack pointer, ... */
	mpc_thread_t pid;
	/* -- MVP Thread specific informations --                   */
	/** VP on which microVP is executed                         */
	int thread_vp_idx;
	/** PU on which microVP is executed                         */
	int pu_id;
	/** MVP thread structure pointer                            */
	mpc_thread_t thread_self;
	/** MVP keep alive after fall asleep                        */
	volatile int enable;
	/** MVP spinning value in topology tree                     */
	volatile int spin_status;
	struct mpc_omp_node_s *spin_node;
	/* -- MVP Tree related informations                         */
	unsigned int depth;
	/** Root of the topology tree                               */
	struct mpc_omp_node_s *root;
	/** Father node in the topology tree                        */
	struct mpc_omp_node_s *father;
	/** Left sibbling mvp                                       */
	struct mpc_omp_mvp_s *prev_brother;
	/* Right sibbling mvp                                       */
	struct mpc_omp_mvp_s *next_brother;
	/** Which thread currently running over MVP                 */
	struct mpc_omp_thread_s *threads;
	/* -- Parallel Region Information Transfert --              */
	/* Transfert OpenMP instance pointer to OpenMP thread       */
	struct mpc_omp_instance_s *instance;
	/* Index in mvps instance array of last nested instance     */
	int instance_mvps_index;
#if MPCOMP_TRANSFER_INFO_ON_NODES
	/* Transfert OpenMP region information to OpenMP thread     */
	mpc_omp_parallel_region_t info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */
	mpc_omp_thread_t *buffered_threads;
	/* -- Tree array MVP information --                         */
	/* Rank within the microVPs */
	//TODO Remove duplicate value
	int rank;

	/** Rank within sibbling MVP        */
	int local_rank;
	int *tree_array_ancestor_path;
	long tree_array_rank;
	/** Rank within topology tree MVP depth level */
	int stage_rank;
	int global_rank;
	/** Size of topology tree MVP depth level    */
	int stage_size;
	int *tree_rank; /* Array of rank in every node of the tree */
	struct mpc_omp_node_s *tree_array_father;
	int min_index[MPCOMP_AFFINITY_NB];
	struct mpc_omp_task_mvp_infos_s task_infos;
	struct mpc_omp_mvp_rank_chain_elt_s *rank_list;
	struct mpc_omp_mvp_saved_context_s *prev_node_father;
#if OMPT_SUPPORT
    /* MVP thread data */
    ompt_data_t ompt_mvp_data;
#endif /* OMPT_SUPPORT */
} mpc_omp_mvp_t;

/* OpenMP Node */

typedef struct mpc_omp_node_s
{
	int already_init;
	/* -- MVP Thread specific informations --                   */
	struct mpc_omp_meta_tree_node_s *tree_array;
	int *tree_array_ancestor_path;
	long tree_array_rank;
	/** MVP spinning as a node                                  */
	struct mpc_omp_mvp_s *mvp;
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
	struct mpc_omp_node_s *father;
	struct mpc_omp_node_s *prev_father;
	/** Rigth brother node in the topology tree                 */
	struct mpc_omp_node_s *prev_brother;
	/** Rigth brother node in the topology tree                 */
	struct mpc_omp_node_s *next_brother;
	/* Kind of children (node or leaf)                          */
	mpc_omp_children_t child_type;
	/* Number of children                                       */
	int nb_children;
	/* Children list                                            */
	mpc_omp_node_or_leaf_t children;
	/* -- Parallel Region Information Transfert --              */
	/* Transfert OpenMP instance pointer to OpenMP thread       */
	struct mpc_omp_instance_s *instance;
	/* Index in mvps instance array of last nested instance     */
	int instance_mvps_index;
#if MPCOMP_TRANSFER_INFO_ON_NODES
	/* Transfert OpenMP region information to OpenMP thread     */
	mpc_omp_parallel_region_t info;
#endif /* MPCOMP_TRANSFER_INFO_ON_NODES */

	int instance_stage_size;
	int instance_global_rank;
	int instance_stage_first_rank;

	/* -- Tree array MVP information --                         */
	/** Rank among children of my father -> local rank          */
	int tree_depth;
	int *tree_base;
	int *tree_cumulative;
	int *tree_nb_nodes_per_depth;

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

	struct mpc_omp_task_node_infos_s task_infos;
	struct mpc_omp_node_s *mvp_prev_father;

	/* Tree reduction flag */
	volatile int *isArrived;
	/* Tree reduction data */
	void **reduce_data;
  worker_workshare_t* worker_ws;

} mpc_omp_node_t;


typedef struct mpc_omp_node_s *mpcomp_node_ptr_t;

typedef struct mpc_omp_mvp_rank_chain_elt_s
{
	struct mpc_omp_mvp_rank_chain_elt_s *prev;
	unsigned int rank;
} mpc_omp_mvp_rank_chain_elt_t;

typedef struct mpc_omp_mvp_saved_context_s
{
	struct mpc_omp_node_s *father;
	struct mpc_omp_node_s *node;
	unsigned int rank;
	struct mpc_omp_mvp_saved_context_s *prev;
} mpc_omp_mvp_saved_context_t;

typedef union mpc_omp_node_gen_ptr_u
{
	struct mpc_omp_mvp_s *mvp;
	struct mpc_omp_node_s *node;
} mpc_omp_node_gen_ptr_t;

typedef struct mpc_omp_generic_node_s
{
	mpc_omp_node_gen_ptr_t ptr;
	/* Kind of children (node or leaf) */
	mpc_omp_children_t type;
} mpc_omp_generic_node_t;

/* Element of the stack for dynamic work stealing */
typedef struct mpc_omp_elem_stack_s
{
	union node_leaf
	{
		struct mpc_omp_node_s *node;
		struct mpc_omp_mvp_s *leaf;
	} elem; /**< Stack element
             */
	mpc_omp_elem_stack_type_t
	type; /**< Type of the 'elem' field 				*/
} mpc_omp_elem_stack_t;

/* Stack structure for dynamic work stealing */
typedef struct mpc_omp_stack_node_leaf_s
{
	mpc_omp_elem_stack_t **elements; /**< List of elements
                                     */
	int max_elements;				/**< Number of max elements
                                     */
	int n_elements;					/**< Corresponds to the head of the stack */
} mpc_omp_stack_node_leaf_t;

/* Stack (maybe the same that mpc_omp_stack_node_leaf_s structure) */
typedef struct mpc_omp_stack
{
	struct mpc_omp_node_s **elements;
	int max_elements;
	int n_elements; /* Corresponds to the head of the stack */
} mpc_omp_stack_t;


/**************
 * OPENMP TLS *
 **************/

extern __thread void *mpc_omp_tls;
extern mpc_omp_global_icv_t mpcomp_global_icvs;
// extern mpc_omp_alloc_list_t mpcomp_global_allocators;

#endif /* __MPCOMP_TYPES_H__ */
