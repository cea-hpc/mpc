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
/* # - Patrick Carribault <patrick.carribault@cea.fr>                     # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Sylvain Didelot <sylvain.didelot@exascale-computing.eu>            # */
/* # - Thomas Dionisi <thomas.dionisi@exascale-computing.eu>              # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_OMP_TYPES_H__
#define __MPC_OMP_TYPES_H__

#define AD_USE_LOCK

#include <stdbool.h>
#include <mpc_common_types.h>
#include <stdbool.h>
#include "mpc_common_debug.h"
#include <mpc_common_asm.h>
#include "sctk_context.h"
#include "mpc_omp.h"
#include "mpc_omp_task_property.h"
#include "sctk_alloc.h"
#include "mpc_common_mcs.h"
#include "mpcompt_instance_types.h"
#include "mpcompt_frame_types.h"

#include "mpc_common_recycler.h"
#include "mpc_omp_task_trace.h"

#define MPC_OMP_USE_INTEL_ABI 1
# ifdef MPC_OMP_USE_INTEL_ABI
#  include "omp_intel_types.h"
# endif /* MPC_OMP_USE_INTEL_ABI */

#include "omp_gomp_constants.h"

/* uthash implementations */
#ifndef HASH_FUNCTION
#  define HASH_FUNCTION(keyptr, keylen, hashv)  do {                                                                            \
                                                    hashv = ((mpc_omp_thread_t *) mpc_omp_tls)->task_infos.hash_deps(*keyptr);  \
                                                } while(0)
#include "uthash.h"
#endif /* HASH_FUNCTION */

/*******************
 * OMP DEFINITIONS *
 *******************/

# define MPC_OMP_VERSION_MAJOR  3
# define MPC_OMP_VERSION_MINOR  1

/* FIBER */
#define MPC_OMP_TASK_COMPILE_FIBER 1

#if MPC_OMP_TASK_COMPILE_FIBER
# define MPC_OMP_TASK_FIBER_ENABLED     mpc_omp_conf_get()->task_use_fiber
# define MPC_OMP_TASK_FIBER_STACK_SIZE  mpc_omp_conf_get()->task_fiber_stack_size
# else
# define MPC_OMP_TASK_FIBER_ENABLED 0
#endif

/* RECYCLER */
# define MPC_OMP_TASK_USE_RECYCLERS     1
# if MPC_OMP_TASK_USE_RECYCLERS
#  define MPC_OMP_TASK_ALLOCATOR        mpc_omp_alloc
#  define MPC_OMP_TASK_DEALLOCATOR      mpc_omp_free
#  define MPC_OMP_TASK_DEFAULT_ALIGN    8
# endif

/* BARRIER ALGORITHM */
# define MPC_OMP_NAIVE_BARRIER              1
# define MPC_OMP_BARRIER_COMPILE_COND_WAIT  1

#if MPC_OMP_BARRIER_COMPILE_COND_WAIT
# if MPC_OMP_NAIVE_BARRIER != 1
#  error "You can only use MPC_OMP_TASK_BARRIER_COND_WAIT with MPC_OMP_NAIVE_BARRIER"
# endif /* MPC_OMP_NAIVE_BARRIER */
# define MPC_OMP_TASK_BARRIER_COND_WAIT_ENABLED        mpc_omp_conf_get()->task_cond_wait_enabled
# define MPC_OMP_TASK_BARRIER_COND_WAIT_NHYPERACTIVE   mpc_omp_conf_get()->task_cond_wait_nhyperactive
#else /* MPC_OMP_BARRIER_COMPILE_COND_WAIT */
# define MPC_OMP_TASK_BARRIER_COND_WAIT_ENABLED     0
#endif /* MPC_OMP_BARRIER_COMPILE_COND_WAIT */

/* Use MCS locks or not */
#define MPC_OMP_USE_MCS_LOCK 1

/* Maximum number of alive 'for dynamic' and 'for guided'  construct */
#define MPC_OMP_MAX_ALIVE_FOR_DYN 3

#define MPC_OMP_NOWAIT_STOP_SYMBOL ( -2 )

/* Uncomment to enable coherency checking */
#define MPC_OMP_COHERENCY_CHECKING 0
#define MPC_OMP_OVERFLOW_CHECKING 0

/* MACRO FOR PERFORMANCE */
#define MPC_OMP_MALLOC_ON_NODE 1
#define MPC_OMP_TRANSFER_INFO_ON_NODES 0
#define MPC_OMP_ALIGN 1

#define MPC_OMP_CHUNKS_NOT_AVAIL 1
#define MPC_OMP_CHUNKS_AVAIL 2

/* Define macro MPC_OMP_MIC in case of Xeon Phi compilation */
#if __MIC__ || __MIC2__
# define MPC_OMP_MIC 1
#endif /* __MIC__ || __MIC2__ */

#if KMP_ARCH_X86
# define KMP_SIZE_T_MAX ( 0xFFFFFFFF )
#else
# define KMP_SIZE_T_MAX ( 0xFFFFFFFFFFFFFFFF )
#endif

#define MPC_OMP_TASK_DEFAULT_ALIGN 8

/* OpenMP 5.0 Memory Management */
#define MPC_OMP_MAX_ALLOCATORS 256

/****************
 * OPENMP ENUMS *
 ****************/
typedef enum    mpc_omp_task_larceny_mode_e
{
    MPC_OMP_TASK_LARCENY_MODE_HIERARCHICAL,
    MPC_OMP_TASK_LARCENY_MODE_RANDOM,
    MPC_OMP_TASK_LARCENY_MODE_RANDOM_ORDER,
    MPC_OMP_TASK_LARCENY_MODE_ROUNDROBIN,
    MPC_OMP_TASK_LARCENY_MODE_PRODUCER,
    MPC_OMP_TASK_LARCENY_MODE_PRODUCER_ORDER,
    MPC_OMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM,
    MPC_OMP_TASK_LARCENY_MODE_COUNT
}               mpc_omp_task_larceny_mode_t;

typedef enum    mpc_omp_meta_tree_type_e
{
	MPC_OMP_META_TREE_UNDEF      = 0,
	MPC_OMP_META_TREE_NODE       = 1,
	MPC_OMP_META_TREE_LEAF       = 2,
	MPC_OMP_META_TREE_NONE       = 3
}               mpc_omp_meta_tree_type_t;


typedef enum    mpc_omp_combined_mode_e
{
	MPC_OMP_COMBINED_NONE            = 0,
	MPC_OMP_COMBINED_SECTION         = 1,
	MPC_OMP_COMBINED_STATIC_LOOP     = 2,
	MPC_OMP_COMBINED_DYN_LOOP        = 3,
	MPC_OMP_COMBINED_GUIDED_LOOP     = 4,
	MPC_OMP_COMBINED_RUNTIME_LOOP    = 5,
	MPC_OMP_COMBINED_COUNT           = 6
}               mpc_omp_combined_mode_t;

typedef enum    mpc_omp_task_yield_mode_e
{
    MPC_OMP_TASK_YIELD_MODE_NOOP,
    MPC_OMP_TASK_YIELD_MODE_STACK,
    MPC_OMP_TASK_YIELD_MODE_CIRCULAR,
    MPC_OMP_TASK_YIELD_MODE_COUNT
}               mpc_omp_task_yield_mode_t;

/* list policies */
typedef enum    mpc_omp_task_list_policy_e
{
    MPC_OMP_TASK_LIST_POLICY_LIFO,
    MPC_OMP_TASK_LIST_POLICY_FIFO
}               mpc_omp_task_list_policy_t;

/**
 *  TO IGNORE PRIORITIES, USE :
 *      - MPC_OMP_TASK_PRIORITY_POLICY_ZERO (== 0)
 *      - MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_NOOP (== 0)
 *      - MPC_OMP_TASK_PRIORITY_PROPAGATION_SYNCHRONOUS (== 0)
 *
 *  « SA1 » is (= SA IWOMP) :
 *      - MPC_OMP_TASK_PRIORITY_POLICY_CONVERT (== 1)
 *      - MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_DECR (== 1)
 *      - MPC_OMP_TASK_PRIORITY_PROPAGATION_SYNCHRONOUS (== 0)
 *
 *  « SA 2 » is :
 *      - MPC_OMP_TASK_PRIORITY_POLICY_COPY (== 2)
 *      - MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_EQUAL (== 2)
 *      - MPC_OMP_TASK_PRIORITY_PROPAGATION_SYNCHRONOUS (== 0)
 *
 *  « FA » is :
 *      - MPC_OMP_TASK_PRIORITY_POLICY_ZERO (== 0)
 *      - MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_DECR (== 1)
 *      - MPC_OMP_TASK_PRIORITY_PROPAGATION_ASYNCHRONOUS (== 1)
 */

/* priority policies */
typedef enum    mpc_omp_task_priority_policy_e
{
    /** 0) FIFO
     *      - priority hint is ignored
     *      - runtime set internal tasks priority to zero
     */
    MPC_OMP_TASK_PRIORITY_POLICY_ZERO,

    /** 1) CONVERT
     *      - user set a priorities on tasks
     *      - runtime convert non-null priorities to INT_MAX internally
     */
    MPC_OMP_TASK_PRIORITY_POLICY_CONVERT,

    /** 2) COPY
     *      - user set a priority hint on tasks
     *      - runtime copies the tasks hint as its internal priority
     */
    MPC_OMP_TASK_PRIORITY_POLICY_COPY

}               mpc_omp_task_priority_policy_t;

/* propagation policy */
typedef enum    mpc_omp_task_priority_propagation_policy_e
{
    /** 0) NOOP
     *      - priorities are not propagated
     */
    MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_NOOP,

    /** 1) DECR
     *      - runtime propagate priorities on predecessors
     *      - P(predecessors) = max(P(successors)) - 1
     */
    MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_DECR,

    /**
     *  2) EQUAL
     *      - runtime propagate priorities on predecessors
     *      - P(predecessors) = max(P(successors))
     */
    MPC_OMP_TASK_PRIORITY_PROPAGATION_POLICY_EQUAL

}               mpc_omp_task_priority_propagation_policy_t;

typedef enum    mpc_omp_task_priority_propagation_synchronousity_e
{
    /* if priorities should be propagated synchronously, on task creation */
    MPC_OMP_TASK_PRIORITY_PROPAGATION_SYNCHRONOUS,

    /* if priorities should be propagated asynchronously, during idle time */
    MPC_OMP_TASK_PRIORITY_PROPAGATION_ASYNCHRONOUS
}               mpc_omp_task_priority_propagation_synchronousity_t;

/* Type of children in the topology tree */
typedef enum    mpc_omp_children_e
{
	MPC_OMP_CHILDREN_NULL = 0,
	MPC_OMP_CHILDREN_NODE = 1,
    MPC_OMP_CHILDREN_LEAF = 2,
}               mpc_omp_children_t;

typedef enum    mpc_omp_topo_obj_type_e
{
	MPC_OMP_TOPO_OBJ_SOCKET,
	MPC_OMP_TOPO_OBJ_CORE,
	MPC_OMP_TOPO_OBJ_THREAD,
	MPC_OMP_TOPO_OBJ_COUNT
}               mpc_omp_topo_obj_type_t;

typedef enum    mpc_omp_mode_e
{
	MPC_OMP_MODE_SIMPLE_MIXED,
	MPC_OMP_MODE_OVERSUBSCRIBED_MIXED,
	MPC_OMP_MODE_ALTERNATING,
	MPC_OMP_MODE_FULLY_MIXED,
	MPC_OMP_MODE_COUNT
}               mpc_omp_mode_t;

typedef enum    mpc_omp_affinity_e
{
	MPC_OMP_AFFINITY_COMPACT,  /* Distribute over logical PUs */
	MPC_OMP_AFFINITY_SCATTER,  /* Distribute over memory controllers */
	MPC_OMP_AFFINITY_BALANCED, /* Distribute over physical PUs */
	MPC_OMP_AFFINITY_NB
}               mpc_omp_affinity_t;

/* new meta elt for task initialisation */
typedef enum    mpc_omp_tree_meta_elt_type_e
{
	MPC_OMP_TREE_META_ELT_MVP,
	MPC_OMP_TREE_META_ELT_NODE,
	MPC_OMP_TREE_META_ELT_COUNT
}               mpc_omp_tree_meta_elt_type_t;

typedef union   mpc_omp_tree_meta_elt_ptr_u
{
	struct mpc_omp_node_s *node;
	struct mpc_omp_mvp_s *mvp;
}               mpc_omp_tree_meta_elt_ptr_t;

typedef struct  mpc_omp_tree_meta_elt_s
{
	mpc_omp_tree_meta_elt_type_t type;
	mpc_omp_tree_meta_elt_ptr_t ptr;
}               mpc_omp_tree_meta_elt_t;

typedef enum    mpc_omp_mvp_state_e
{
	MPC_OMP_MVP_STATE_UNDEF = 0,
	MPC_OMP_MVP_STATE_SLEEP,
	MPC_OMP_MVP_STATE_AWAKE,
	MPC_OMP_MVP_STATE_READY
}               mpc_omp_mvp_state_t;


typedef enum    mpc_omp_loop_gen_type_e
{
	MPC_OMP_LOOP_TYPE_LONG,
	MPC_OMP_LOOP_TYPE_ULL,
}              mpc_omp_loop_gen_type_t;

/** Type of task pqueues */
typedef enum    mpc_omp_pqueue_type_e
{
    MPC_OMP_PQUEUE_TYPE_TIED        = 0,  /* ready-tied tasks (run once) */
    MPC_OMP_PQUEUE_TYPE_UNTIED      = 1,  /* ready-untied tasks (run once) */
    MPC_OMP_PQUEUE_TYPE_NEW         = 2,  /* ready tasks (never run) */
    MPC_OMP_PQUEUE_TYPE_SUCCESSOR   = 3,  /* ready tasks (never run, direct successors or a task that just finished, may be tied or untied) */
    MPC_OMP_PQUEUE_TYPE_COUNT       = 4
}               mpc_omp_task_pqueue_type_t;

/** Various lists that a task may be hold */
typedef enum    mpc_omp_task_list_type_e
{
    MPC_OMP_TASK_LIST_TYPE_SCHEDULER    = 0,
    MPC_OMP_TASK_LIST_TYPE_UP_DOWN      = 1,
    MPC_OMP_TASK_LIST_TYPE_COUNT        = 2
}               mpc_omp_task_list_type_t;

typedef enum    mpcomp_task_dep_task_status_e
{
    MPC_OMP_TASK_STATUS_NOT_READY       = 0,
    MPC_OMP_TASK_STATUS_READY           = 1,
    MPC_OMP_TASK_STATUS_FINALIZED       = 2,
    MPC_OMP_TASK_STATUS_COUNT           = 3
}               mpcomp_task_dep_task_status_t;

/**********************
 * OMPT STATUS        *
 **********************/

typedef enum    mpc_omp_tool_status_e {
    mpc_omp_tool_disabled = 0,
    mpc_omp_tool_enabled  = 1
}               mpc_omp_tool_status_t;

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
	int top_level;
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

/* Data structures */
typedef struct  mpc_omp_task_taskgroup_s
{
	struct mpc_omp_task_taskgroup_s * prev;
	OPA_int_t children_num;
    OPA_int_t cancelled;
    int last_task_uid;
}               mpc_omp_task_taskgroup_t;

#ifdef MPC_OMP_USE_MCS_LOCK
	typedef sctk_mcslock_t mpc_omp_task_lock_t;
#else  /* MPC_OMP_USE_MCS_LOCK */
	typedef mpc_common_spinlock_t mpc_omp_task_lock_t;
#endif /* MPC_OMP_USE_MCS_LOCK */

/* task dependencies structures */
struct mpc_omp_task_s;
struct mpc_omp_task_dep_node_s;

/**
 * \param[in]   addr    uintptr_t   depend addr ptr
 * \return      hash    uint32_t    hash computed value
 */
typedef uint32_t (*mpc_omp_task_dep_hash_func_t)(uintptr_t);

/* mpc_omp_task_t successors and predecessors lists */
typedef struct mpc_omp_task_list_elt_s
{
    struct mpc_omp_task_s           * task;
    struct mpc_omp_task_list_elt_s  * next;
}               mpc_omp_task_list_elt_t;

/* in, out, inoutset list element */
typedef struct mpc_omp_task_dep_list_elt_s
{
    /* the task for this dependency node */
    struct mpc_omp_task_s * task;

    /* the hmap entry in which this node is inserted */
    struct mpc_omp_task_dep_htable_entry_s * entry;

    /* next and previous elements in the list of the hmap entry */
    struct mpc_omp_task_dep_list_elt_s * next;
    struct mpc_omp_task_dep_list_elt_s * prev;

}               mpc_omp_task_dep_list_elt_t;

/** Task context structure */
#if MPC_OMP_TASK_COMPILE_FIBER

typedef struct  mpc_omp_task_fiber_s
{
    sctk_mctx_t initial;    /**< the initial context of this task (for recycling 'makecontext' calls) */
    sctk_mctx_t current;    /**< the current context of this task */
    sctk_mctx_t * exit;     /**< the context to return when this task is paused or finished */
}               mpc_omp_task_fiber_t;
#endif

struct mpc_omp_task_pqueue_s;

/* a task list */
typedef struct  mpc_omp_task_list_s
{
    /* number of tasks in the list */
    OPA_int_t nb_elements;

    /* head task in the list */
    struct mpc_omp_task_s * head;

    /* tail task inf the list */
    struct mpc_omp_task_s * tail;

    /* mutex */
    mpc_common_spinlock_t lock;

    /* list type */
    int type;
}               mpc_omp_task_list_t;

/* hash table entry,
 * note that collisions handle could be optimized
 * by using a tree structure, ordered by the 'addr' value */
typedef struct  mpc_omp_task_dep_htable_entry_s
{
    /* address for the dependency */
    void * addr;

    /* last 'out' or 'inout' task for this key */
    mpc_omp_task_dep_list_elt_t * out;

    /* list of last 'in' tasks for this key */
    mpc_omp_task_dep_list_elt_t * ins;

    /* list of last 'inoutset' tasks for this key */
    mpc_omp_task_dep_list_elt_t * inoutset;

    /* the lock to access lists */
    mpc_common_spinlock_t lock;

    /* the last task that had a dependency for this address
     * (used for redundancy check) */
    int task_uid;

    /* the hmap handle */
    UT_hash_handle hh;
}               mpc_omp_task_dep_htable_entry_t;

/* a task profile */
typedef struct  mpc_omp_task_profile_s
{
    /* next node */
    struct mpc_omp_task_profile_s * next;

    /* the task size */
    unsigned int size;

    /* the task properties */
    unsigned int property;

    /* number of successors (may be incomplete) */
    int nsuccessors;

    /* number of predecessors (is complete) */
    int npredecessors;

    /* priority associated to the profile */
    int priority;

    /* the parent task uid (control parent) */
    int parent_uid;
}               mpc_omp_task_profile_t;

/* critical tasks infos */
typedef struct  mpc_omp_task_profile_info_s
{
    /* head info node */
    OPA_ptr_t head;

    /* number of 'mpc_omp_task_profile_t' in list */
    OPA_int_t n;

    /* spinlock for concurrency issues */
    mpc_common_spinlock_t spinlock;
}               mpc_omp_task_profile_info_t;

/** Task priorities propagation context */
typedef struct  mpc_omp_task_priority_propagation_context_s
{
    /* lock so that only 1 thread my propagate priorities (for now) */
    mpc_common_spinlock_t lock;

    /* tasks to climb down to find leaves */
    mpc_omp_task_list_t down;

    /* tasks to climb up to match profile and propagate priorities */
    mpc_omp_task_list_t up;

    /* version to mark task and no visit them twice */
    OPA_int_t version;
}               mpc_omp_task_priority_propagation_context_t;

typedef struct  mpc_omp_task_dep_node_s
{
    /* hash table for child tasks dependencies */
    mpc_omp_task_dep_htable_entry_t * hmap;
    mpc_common_spinlock_t hmap_lock;

    /* lists */
    struct mpc_omp_task_list_elt_s * successors;
    OPA_int_t nsuccessors;

    struct mpc_omp_task_list_elt_s * predecessors;
    OPA_int_t npredecessors;

    /* depth in the data dependency graph */
    int depth;

    /* number of predecessors still existing */
    OPA_int_t ref_predecessors;

    /* maximum path length to reach a root, in the dependency tree */
    int top_level;

    /* status */
    OPA_int_t status;

    /* entries in the parent hash map for dependencies */
    mpc_omp_task_dep_list_elt_t * dep_list;
    unsigned int dep_list_size;

#if OMPT_SUPPORT
    /* Task dependences record */
    ompt_dependence_t * ompt_task_deps;
#endif /* OMPT_SUPPORT */

    /* profile version, to know if the profile matching should be performed */
    int profile_version;

}               mpc_omp_task_dep_node_t;

/** OpenMP task data structure */
typedef struct  mpc_omp_task_s
{
    /* nested task depth */
    int depth;

    /* function to execute */
    void (*func)(void *);

    /* argument of the function */
    void * data;

    /* total size of the task (sizeof(mpcomp_task_t) + data_size) */
    unsigned int size;

    /* task statuses */
    mpc_omp_task_statuses_t statuses;

    /* number of existing child for this task */
    OPA_int_t children_count;

#if MPC_OMP_TASK_COMPILE_FIBER
    mpc_omp_task_fiber_t * fiber;
#endif

    /* task properties */
    mpc_omp_task_property_t property;

    /* the thread on which the task last-run */
    struct mpc_omp_thread_s * thread;

    /* parent task (always non-null, except for the implicit task) */
    struct mpc_omp_task_s * parent;

    /* next/previous task pointers if the task is inside a list */
    struct mpc_omp_task_s * prev[MPC_OMP_TASK_LIST_TYPE_COUNT];
    struct mpc_omp_task_s * next[MPC_OMP_TASK_LIST_TYPE_COUNT];

    /* icv of the thread that created the task */
    mpc_omp_local_icv_t icvs;

    /* thread task/icvs/ompt_frame_infos before this task was bound to the thread */
    struct mpc_omp_task_s * prev_task;
    mpc_omp_local_icv_t prev_icvs;

    /* the task group */
    mpc_omp_task_taskgroup_t * taskgroup;

    /* task dependencies infos */
    mpc_omp_task_dep_node_t dep_node;

    /* task lock to update critical informations */
    mpc_common_spinlock_t lock;

    /* task reference counter to release memory appropriately */
    OPA_int_t ref_counter;

    /* last task allocated by the thread doing if0 pragma */
    struct mpc_omp_task_s * last_task_alloc;

    /* the omp task priority clause value */
    int omp_priority_hint;

    /* the task priority, used for scheduling. See mpcomp_task_priority_policy */
    int priority;

# if MPC_OMP_TASK_COMPILE_TRACE
    /* the task name (+1 for the null char.) */
    char label[MPC_OMP_TASK_LABEL_MAX_LENGTH + 1];

    /* task schedule id (= number of task previously scheduled) */
    int schedule_id;
# endif

    /* task uid (= number of task previously created) */
    int uid;

    /* the task list */
    struct mpc_omp_task_pqueue_s * pqueue;

#if OMPT_SUPPORT
    /* Task data and type */
    ompt_data_t ompt_task_data;
    ompt_task_flag_t ompt_task_type;

    /* Task frame */
    ompt_frame_t ompt_task_frame;
#endif /* OMPT_SUPPORT */

    /* task priorities propagation version */
    int propagation_version;

}               mpc_omp_task_t;

/** RB tree for task priorities */
typedef struct  mpc_omp_task_pqueue_node_s
{
    /* parent node */
    struct mpc_omp_task_pqueue_node_s * parent;

    /* left node (left->priority < this->priority) */
    struct mpc_omp_task_pqueue_node_s * left;

    /* right node (right->priority > this->priority) */
    struct mpc_omp_task_pqueue_node_s * right;

    /* priority for this node */
    int priority;

    /* tasks for this priority */
    mpc_omp_task_list_t tasks;

    /* 'R' or 'B' */
    char color;
}               mpc_omp_task_pqueue_node_t;

/**
 * Priority queue with :
 *  - Insertion : O(log2(n))
 *  - Deletion  : O(log2(n)) (not implemented yet)
 *  - Find top priority : O(1)
 *
 * Nodes are lists of tasks with the same priority.
 */
typedef struct  mpc_omp_task_pqueue_s
{
    /* the tree head */
    mpc_omp_task_pqueue_node_t * root;

    /* the tree lock */
    mpc_common_spinlock_t lock;

    /* number of element in the tree */
    OPA_int_t nb_elements;

    /* the queue type */
    mpc_omp_task_pqueue_type_t type;
}               mpc_omp_task_pqueue_t;

/**
 *  *  * Used by mpcomp_mvp_t and mpcomp_node_t
 *   *   */
typedef struct  mpc_omp_task_node_infos_s
{
    /** Lists of tasks */
    struct mpc_omp_task_pqueue_s * pqueue[MPC_OMP_PQUEUE_TYPE_COUNT];
    struct mpc_omp_task_pqueue_s * lastStolen_pqueue[MPC_OMP_PQUEUE_TYPE_COUNT];
    int taskPqueueNodeRank[MPC_OMP_PQUEUE_TYPE_COUNT];
    struct drand48_data * pqueue_randBuffer;
}               mpc_omp_task_node_infos_t;

/** mvp and node share same struct */
typedef struct  mpc_omp_task_mvp_infos_s
{
    /** Lists of tasks */
    struct mpc_omp_task_pqueue_s * pqueue[MPC_OMP_PQUEUE_TYPE_COUNT];
    struct mpc_omp_task_pqueue_s * lastStolen_pqueue[MPC_OMP_PQUEUE_TYPE_COUNT];
    int taskPqueueNodeRank[MPC_OMP_PQUEUE_TYPE_COUNT];
    struct drand48_data * pqueue_randBuffer;
    int last_thief;
}               mpc_omp_task_mvp_infos_t;

/** Extend mpc_omp_thread_t struct for openmp task support */
typedef struct  mpc_omp_task_thread_infos_s
{
    int * larceny_order;
    mpc_omp_task_t * current_task;      /* Currently running task */
    void* opaque;                       /* use mcslock buffered */

# if MPC_OMP_TASK_COMPILE_FIBER
    mpc_common_spinlock_t * spinlock_to_unlock; /* a spinlock to be unlocked, see mpc_omp_task_block() */
    mpc_omp_task_fiber_t * fiber;               /* the fiber to use when running a new task */
    sctk_mctx_t mctx;                           /* the thread context before the task started */
# endif /* MPC_OMP_TASK_COMPILE_FIBER */

# if MPC_OMP_TASK_USE_RECYCLERS
    mpc_common_nrecycler_t task_recycler;   /* Recycler for mpc_omp_task_t and its data */
#  if MPC_OMP_TASK_COMPILE_FIBER
    mpc_common_recycler_t  fiber_recycler;  /* Recycler for mpc_omp_task_fiber_t */
#  endif /* MPC_OMP_TASK_COMPILE_FIBER */
# endif /* MPC_OMP_TASK_USE_RECYCLERS */

    /* extra data for incoming task */
    struct
    {
        char * label;
        int extra_clauses;
        mpc_omp_task_dependency_t * dependencies;
        unsigned int ndependencies_type;
    } incoming;

# if MPC_OMP_TASK_COMPILE_TRACE
    mpc_omp_thread_task_trace_infos_t tracer;
# endif

    uintptr_t (*hash_deps)(void * addr);

# if WIP
    mpc_omp_task_list leaves;
# endif

    /* ? */
    size_t sizeof_kmp_task_t;

}               mpc_omp_task_thread_infos_t;

/**
 * Extend mpc_omp_instance_t struct for openmp task support
 */
typedef struct  mpc_omp_task_instance_infos_s
{
	int array_tree_total_size;
	int *array_tree_level_first;
	int *array_tree_level_size;
    bool is_initialized;

# if MPC_OMP_TASK_COMPILE_TRACE
    OPA_int_t next_schedule_id;
# endif /* MPCOMP_TASK_COMPILE_TRACE */

    OPA_int_t next_task_uid;

    /* number of existing tasks */
    OPA_int_t ntasks_allocated;

    /* number of ready tasks overall */
    OPA_int_t ntasks_ready;

    /* blocked tasks list */
    mpc_omp_task_list_t blocked_tasks;

# if MPC_OMP_BARRIER_COMPILE_COND_WAIT
    /* condition to make threads sleep when waiting for tasks */
    pthread_mutex_t work_cond_mutex;
    pthread_cond_t  work_cond;
    int             work_cond_nthreads;
# endif

    /* task profiles for asynchronous priority propagation */
    mpc_omp_task_profile_info_t profiles;

    /* priority propagation information */
    mpc_omp_task_priority_propagation_context_t propagation;
}               mpc_omp_task_instance_infos_t;

/**
 * Extend mpc_omp_team_t struct for openmp task support
 */
typedef struct  mpc_omp_task_team_infos_s
{
    /** Task max depth in task generation */
	int task_nesting_max;

    /* Task stealing policy */
	int task_larceny_mode;

    /* Thread team task's init status tag */
	OPA_int_t status;

    /* Thread team task create task */
    OPA_int_t use_task;

    /* Pqueue depth in the tree */
    int pqueue_depth[MPC_OMP_PQUEUE_TYPE_COUNT];
}           mpc_omp_task_team_infos_t;

typedef struct  mpc_omp_task_stealing_funcs_s {
  char *namePolicy;
  int allowMultipleVictims;
  int (*pre_init)(void);
  int (*get_victim)(int globalRank, int index, mpc_omp_task_pqueue_type_t type);
}               mpc_omp_task_stealing_funcs_t;

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
  int idx_list[MPC_OMP_MAX_ALLOCATORS];
  int nb_recycl_allocators;
} mpc_omp_recycl_alloc_info_t;

typedef struct mpc_omp_alloc_list_s
{
  mpc_omp_alloc_t list[MPC_OMP_MAX_ALLOCATORS];
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

    /* number of existing tasks in the region */
    OPA_int_t task_ref;
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
	info->combined_pragma = MPC_OMP_COMBINED_NONE;
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
	for_dyn_nb_threads_exited[MPC_OMP_MAX_ALIVE_FOR_DYN + 1];

	/* GUIDED LOOP CONSTRUCT */
	volatile int is_first[MPC_OMP_MAX_ALIVE_FOR_DYN + 1];
	OPA_ptr_t guided_from[MPC_OMP_MAX_ALIVE_FOR_DYN + 1];
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

#if MPC_OMP_NAIVE_BARRIER
    OPA_int_t threads_in_barrier;
    volatile int barrier_version;
#endif

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
	mpc_omp_atomic_int_pad_t for_dyn_remain[MPC_OMP_MAX_ALIVE_FOR_DYN + 1];
	/* Total number of chunks of the thread */
	unsigned long long for_dyn_total[MPC_OMP_MAX_ALIVE_FOR_DYN + 1];
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
#ifdef MPC_OMP_USE_INTEL_ABI
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
#endif /* MPC_OMP_USE_INTEL_ABI */

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
}       mpc_omp_thread_t;

typedef struct  mpc_omp_instance_callback_infos_s
{
    /* callbacks per events */
    mpc_omp_callback_t * callbacks[MPC_OMP_CALLBACK_MAX];

    /* callback lock per event */
    mpc_common_spinlock_t locks[MPC_OMP_CALLBACK_MAX];

    /* number of callbacks per event */
    int length[MPC_OMP_CALLBACK_MAX];
}               mpc_omp_instance_callback_infos_t;

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

    /* asynchronous callbacks */
    mpc_omp_instance_callback_infos_t callback_infos;

    /* mutex to dump thread debugging informations */
    mpc_common_spinlock_t debug_lock;

    /* DEBUGING */
    double t_hash;
    double t_deps;
    double t_total;
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
#if MPC_OMP_TRANSFER_INFO_ON_NODES
	/* Transfert OpenMP region information to OpenMP thread     */
	mpc_omp_parallel_region_t info;
#endif /* MPC_OMP_TRANSFER_INFO_ON_NODES */
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
	int min_index[MPC_OMP_AFFINITY_NB];
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
#if MPC_OMP_TRANSFER_INFO_ON_NODES
	/* Transfert OpenMP region information to OpenMP thread     */
	mpc_omp_parallel_region_t info;
#endif /* MPC_OMP_TRANSFER_INFO_ON_NODES */

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
	int min_index[MPC_OMP_AFFINITY_NB];
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

/**************
 * OPENMP TLS *
 **************/
extern __thread void *mpc_omp_tls;
extern mpc_omp_global_icv_t mpcomp_global_icvs;
// extern mpc_omp_alloc_list_t mpcomp_global_allocators;

#endif /* __MPC_OMP_TYPES_H__ */
