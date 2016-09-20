#ifndef __MPCOMP_ENUM_TYPES_H__
#define __MPCOMP_ENUM_TYPES_H__

typedef enum mpcomp_combined_mode_e
{
	MPCOMP_COMBINED_NONE 			= 0,
	MPCOMP_COMBINED_SECTION 		= 1,
	MPCOMP_COMBINED_STATIC_LOOP 	= 2,
	MPCOMP_COMBINED_DYN_LOOP 		= 3,	
	MPCOMP_COMBINED_GUIDED_LOOP	= 4,
	MPCOMP_COMBINED_RUNTIME_LOOP 	= 5,
	MPCOMP_COMBINED_COUNT 			= 6
} mpcomp_combined_mode_t;

/* Type of element in the stack for dynamic work stealing */
typedef enum mpcomp_elem_stack_type_e 
{
	MPCOMP_ELEM_STACK_NODE 		= 1,
	MPCOMP_ELEM_STACK_LEAF 		= 2,
	MPCOMP_ELEM_STACK_COUNT 	= 3,
} mpcomp_elem_stack_type_t;

/* Type of children in the topology tree */
typedef enum mpcomp_children_e 
{
	MPCOMP_CHILDREN_NODE = 1,
	MPCOMP_CHILDREN_LEAF = 2,
} mpcomp_children_t;

typedef enum mpcomp_context_e 
{
	MPCOMP_CONTEXT_IN_ORDER = 1,
	MPCOMP_CONTEXT_OUT_OF_ORDER_MAIN = 2,
	MPCOMP_CONTEXT_OUT_OF_ORDER_SUB = 3,
} mpcomp_context_t;

typedef enum mpcomp_topo_obj_type_e
{
	MPCOMP_TOPO_OBJ_SOCKET, 
	MPCOMP_TOPO_OBJ_CORE, 
	MPCOMP_TOPO_OBJ_THREAD, 
	MPCOMP_TOPO_OBJ_COUNT
} mpcomp_topo_obj_type_t;

typedef enum mpcomp_mode_e 
{
	MPCOMP_MODE_SIMPLE_MIXED,
	MPCOMP_MODE_OVERSUBSCRIBED_MIXED,
	MPCOMP_MODE_ALTERNATING,
	MPCOMP_MODE_FULLY_MIXED,
	MPCOMP_MODE_COUNT
} mpcomp_mode_t;

typedef enum mpcomp_affinity_e
{
	MPCOMP_AFFINITY_COMPACT,   /* Distribute over logical PUs */
	MPCOMP_AFFINITY_SCATTER,   /* Distribute over memory controllers */
	MPCOMP_AFFINITY_BALANCED,   /* Distribute over physical PUs */
	MPCOMP_AFFINITY_NB
} mpcomp_affinity_t;

/* Global Internal Control Variables
 * One structure per OpenMP instance */
typedef struct mpcomp_global_icv_s 
{
	omp_sched_t def_sched_var;    /**< Default schedule when no 'schedule' clause is present 			*/
	int bind_var;                 /**< Is the OpenMP threads bound to cpu cores 							*/
	int stacksize_var;            /**< Size of the stack per thread (in Bytes) 							*/
	int active_wait_policy_var;   /**< Is the threads wait policy active or passive 						*/
	int thread_limit_var;         /**< Number of Open threads to use for the whole program 			*/
	int max_active_levels_var;    /**< Maximum number of nested active parallel regions 				*/
	int nmicrovps_var;            /**< Number of VPs 																*/
	int warn_nested ;             /**< Emit warning for nested parallelism? 								*/
	int affinity;             		/**< OpenMP threads affinity  												*/
} mpcomp_global_icv_t;


/** Local Internal Control Variables
 * One structure per OpenMP thread 				*/
typedef struct mpcomp_local_icv_s 
{
	int nthreads_var; 				/**< Number of threads for the next team creation 						*/
	int dyn_var;		  				/**< Is dynamic thread adjustement on? 									*/
	int nest_var;		        		/**< Is nested OpenMP handled/allowed? 									*/
	omp_sched_t run_sched_var;		/**< Schedule to use when a 'schedule' clause is set to 'runtime' */
	int modifier_sched_var;			/**< Size of chunks for loop schedule 										*/
  	int active_levels_var; 			/**< Number of nested, active enclosing parallel regions 			*/
  	int levels_var ; 					/**< Number of nested enclosing parallel regions 						*/
} mpcomp_local_icv_t;

/* Integer atomic with padding to avoid false sharing */
typedef struct mpcomp_atomic_int_pad_s
{
	sctk_atomics_int i;   /**< Value of the integer */
	char pad[8];          /* Padding */
} mpcomp_atomic_int_pad_t;

#endif /* __MPCOMP_ENUM_TYPES_H__ */
