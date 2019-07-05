#ifndef __MPCOMP_OMPT_H__
#define __MPCOMP_OMPT_H__

#include <stdint.h>

void * __ompt_get_return_address(unsigned int level);

/*****
 * Iterateur lookup function 
 */

typedef void (*ompt_callback_t)(void);
typedef void (*ompt_interface_fn_t)(void);
typedef void ompt_device_t;

typedef ompt_interface_fn_t (*ompt_function_lookup_t)(
	const char *
);

#define FOREACH_OMPT_INQUIRY_FN(macro)		\
														\
	/* ENUMERATE */								\
	macro(ompt_enumerate_states)				\
	macro(ompt_enumerate_mutex_impls)		\
														\
	/* CALLBACK */									\
	macro(ompt_set_callback)					\
	macro(ompt_get_callback)					\
														\
	/* GETTER */									\
	macro(ompt_get_thread_data)				\
	macro(ompt_get_state)						\
	macro(ompt_get_parallel_info)				\
	macro(ompt_get_task_info)	

#define FOREACH_OMPT_STATE(macro)														\
																									\
	/* WORK STATES */																			\
	macro(omp_state_work_serial, 		0x000, "working outside parallel")		\
	macro(omp_state_work_parallel, 	0x001, "working within parallel")		\
	macro(omp_state_work_reduction, 	0x002, "perform a reduction")				\
																									\
	/* BARRIER WAIT STATES */																\
	macro(omp_state_wait_barrier, 0x010, "generic barrier")						\
	macro(omp_state_wait_barrier_implicit_parallel, 0x011, "todo")				\
	macro(omp_state_wait_barrier_implicit_workshare, 0x012,"todo")				\
	macro(omp_state_wait_barrier_implicit, 0x013,"implicite barrier")			\
	macro(omp_state_wait_barrier_explicit, 0x014,"explicite barrier")			\
																									\
	/* TASK WAIT STATES */																	\
	macro(omp_state_wait_taskwait, 	0x020, "waiting at a taskwait")			\
	macro(omp_state_wait_taskgroup, 0x021, "waiting at a taskgroup")			\
																									\
	/* MUTEX WAIT STATES */																	\
	macro(ompt_state_wait_mutex,0x040, "waiting for any mutex kind")			\
	macro(ompt_state_wait_lock,0x041, "waiting for lock")							\
	macro(ompt_state_wait_critical,0x042, "waiting for critical")				\
	macro(ompt_state_wait_atomic,0x043, "waiting for atomic")					\
	macro(ompt_state_wait_ordered,0x044, "waiting for ordered")					\
																									\
	/* TARGET WAIT STATES */																\
	macro(ompt_state_wait_target,0x080, "waiting for target")					\
	macro(ompt_state_wait_target_data,0x081, "waiting for target data")		\
	macro(ompt_state_wait_target_update,0x082, "waiting for target update")	\
																									\
	/* MISC STATES */																			\
	macro(omp_state_idle, 0x100, "waiting for work")								\
	macro(omp_state_overhead, 0x101, "non-wait overhead")							\
	macro(ompt_state_undefined, 0x102, "undefined thread state")		

typedef enum {
#define ompt_state_macro(state, code, desc) state = code,
	FOREACH_OMPT_STATE(ompt_state_macro)
#undef ompt_state_macro
} ompt_state_t;

/**
 * OMPT_MUTEX_IMPL
 */

#define FOREACH_OMPT_MUTEX_IMPL(macro)                            \
   macro(ompt_mutex_impl_unknown, 0x000, "mpcomp_mutex_unkown")   \
   macro(mpcomp_mutex,     0x001, "mpcomp_mutex")                 \
   macro(mpcomp_spinlock,  0x002, "mpcomp_spinlock")              \
   macro(mpcomp_mcslock,   0x003, "mpcomp_mcslock")

typedef enum {
#define ompt_mutex_impl_macro(mutex_impl, code, desc) mutex_impl = code,
	FOREACH_OMPT_MUTEX_IMPL(ompt_mutex_impl_macro)
#undef ompt_mutex_impl_macro
} ompt_mutex_impl_t;


/**
 * OMPT_SET_RESULT
 */
#define FOREACH_OMPT_SET_RESULT(macro)                                        \
   macro(ompt_set_error,                     0)                               \
   macro(ompt_set_none,                      1)                               \
   macro(ompt_set_sometimes,                 2)                               \
   macro(ompt_set_sometimes_pair,            3)                               \
   macro(ompt_set_always,                    4)                

typedef enum {
#define ompt_set_result_macro(name, code)	name = code,	
	FOREACH_OMPT_SET_RESULT(ompt_set_result_macro)
#undef ompt_set_result_macro
} ompt_set_result_t;

/** 
 * OMPT_CALLBACK
 */

#define FOREACH_OMPT_CALLBACK(macro)                                           \
                                                                               \
    /*--- Mandatory Events ---*/                                               \
macro( ompt_callback_thread_begin,                    1,   ompt_set_always )   \
macro( ompt_callback_thread_end,                      2,   ompt_set_always )   \
macro( ompt_callback_parallel_begin,                  3,   ompt_set_always )   \
macro( ompt_callback_parallel_end,                    4,   ompt_set_always )   \
macro( ompt_callback_task_create,                     5,   ompt_set_always )   \
macro( ompt_callback_task_schedule,                   6,   ompt_set_always )   \
macro( ompt_callback_implicit_task,                   7,   ompt_set_always )   \
macro( ompt_callback_target,                          8,   ompt_set_none )     \
macro( ompt_callback_target_data_op,                  9,   ompt_set_none )     \
macro( ompt_callback_target_submit,                   10,  ompt_set_none )     \
macro( ompt_callback_control_tool,                    11,  ompt_set_none )     \
macro( ompt_callback_device_initialize,               12,  ompt_set_none )     \
                                                                               \
/*--- Optional Events for Blame Shifting ---*/                                 \
macro( ompt_callback_idle,                            13,  ompt_set_sometimes )\
macro( ompt_callback_sync_region_wait,                14,  ompt_set_sometimes )\
macro( ompt_callback_mutex_released,                  15,  ompt_set_sometimes )\
                                                                               \
/*--- Optional Events for Instrumentation-based Tools ---*/                    \
macro( ompt_callback_task_dependences,                16,  ompt_set_always )   \
macro( ompt_callback_task_dependence,                 17,  ompt_set_none )     \
macro( ompt_callback_work,                            18,  ompt_set_sometimes )\
macro( ompt_callback_master,                          19,  ompt_set_none )     \
macro( ompt_callback_target_map,                      20,  ompt_set_none )     \
macro( ompt_callback_sync_region,                     21,  ompt_set_always )   \
macro( ompt_callback_lock_init,                       22,  ompt_set_sometimes )\
macro( ompt_callback_lock_destroy,                    23,  ompt_set_sometimes )\
macro( ompt_callback_mutex_acquire,                   24,  ompt_set_sometimes )\
macro( ompt_callback_mutex_acquired,                  25,  ompt_set_sometimes )\
macro( ompt_callback_nest_lock,                       26,  ompt_set_sometimes )\
macro( ompt_callback_flush,                           27,  ompt_set_none )     \
macro( ompt_callback_cancel,                          28,  ompt_set_none )

typedef enum {
#define ompt_callback_macro(name, code, status) name = code,
	FOREACH_OMPT_CALLBACK(ompt_callback_macro)
#undef ompt_callback_macro
} ompt_callbacks_t;

/*
 * MUTEX KIND
 */

typedef enum ompt_mutex_kind_e
{
	ompt_mutex				= 0x10,
	ompt_mutex_lock		= 0x11,
	ompt_mutex_nest_lock	= 0x12,
	ompt_mutex_critical	= 0x13,
	ompt_mutex_atomic		= 0x14,
	ompt_mutex_ordered	= 0x20
} ompt_mutex_kind_t;

typedef enum ompt_target_data_op_e
{
   ompt_target_data_alloc = 1,
   ompt_target_data_transfer_to_dev = 2,
   ompt_target_data_transfer_from_dev = 3,
   ompt_target_data_delete = 4
} ompt_target_data_op_t;

typedef enum ompt_target_type_e
{
   ompt_target = 1,
   ompt_target_enter_data = 2,
   ompt_target_exit_data = 3,
   ompt_target_update = 4
}ompt_target_type_t;

typedef enum ompt_scope_endpoint_e
{
   ompt_scope_begin = 1,
   ompt_scope_end = 2
} ompt_scope_endpoint_t;

typedef enum ompt_sync_region_e
{
	ompt_sync_region_barrier                = 1,
	ompt_sync_region_barrier_implicit       = 2,
	ompt_sync_region_barrier_explicit       = 3,
	ompt_sync_region_barrier_implementation = 4,
	ompt_sync_region_taskwait               = 5,
	ompt_sync_region_taskgroup              = 6,
	ompt_sync_region_reduction              = 7
}ompt_sync_region_t;

/*
 * identifiers
 */

typedef uint64_t ompt_id_t;
typedef uint64_t ompt_wait_id_t; /**< identifies what a thread is awaiting */

typedef union ompt_data_u{
   uint64_t value;               /**< integer ID under tool control */
   void* ptr;                    /**< pointer under tool control */
} ompt_data_t;

extern ompt_callback_t* OMPT_Callbacks; 
extern const ompt_data_t ompt_data_none;

#define FOREACH_OMPT_TASK_STATUS( macro )	\
	macro( ompt_task_complete, 1 )			\
	macro( ompt_task_yield, 	2 )			\
	macro( ompt_task_cancel, 	3 )			\
	macro( ompt_task_others, 	4 )			

typedef enum ompt_task_status_e
{
#define ompt_task_status_gen( name, id )	name = id,
	FOREACH_OMPT_TASK_STATUS( ompt_task_status_gen )
#undef ompt_task_status_gen
} ompt_task_status_t;
/*
 * ompt_frame_t
 */

typedef struct ompt_frame_s
{
	void *exit_frame; /**< runtime frame that reenters user code */
	void *enter_frame; /**< user frame that reenters the runtime  */
} ompt_frame_t;

/***
 *
 */
typedef enum ompt_parallel_flag_e
{
	ompt_parallel_invoker_program = 0x00000001,	 /**< program invokes master task */
	ompt_parallel_invoker_runtime = 0x00000002,  /**< runtime invokes master task */
    ompt_parallel_league          = 0x40000000,
    ompt_parallel_team            = 0x80000000,
} ompt_parallel_flag_t;

typedef enum ompt_task_flag_e
{
    ompt_task_undefined  = 0x00000000,
	ompt_task_initial    = 0x00000001,
	ompt_task_implicit   = 0x00000002,
	ompt_task_explicit   = 0x00000004,
	ompt_task_target     = 0x00000008,
	ompt_task_undeferred = 0x08000000,
    ompt_task_untied     = 0x10000000,
    ompt_task_final      = 0x20000000,
    ompt_task_mergeable  = 0x40000000,
    ompt_task_merged     = 0x80000000
}ompt_task_flag_t;

typedef enum ompt_thread_type_e
{
	ompt_thread_initial = 1,
	ompt_thread_worker = 2,
	ompt_thread_other = 3,
	ompt_thread_unknown = 4
}ompt_thread_type_t;

typedef enum ompt_task_dependence_flag_e
{
	ompt_task_dependence_type_out = 1,
	ompt_task_dependence_type_in = 2,
	ompt_task_dependence_type_inout = 3,
} ompt_task_dependence_flag_t;

typedef enum ompt_work_e
{
	ompt_work_loop            = 1,
	ompt_work_sections        = 2,
	ompt_work_single_executor = 3,
	ompt_work_single_other    = 4,
	ompt_work_workshare       = 5,
	ompt_work_distribute      = 6,
	ompt_work_taskloop        = 7
}ompt_work_t;

typedef struct ompt_task_dependence_s
{
	void* variable_addr;
	unsigned int dependence_flags;
} ompt_task_dependence_t;

/**
 * INITIALIZATION FUNCTIONS
 *****/

typedef struct ompt_fns_t ompt_fns_t;

typedef int (*ompt_initialize_t)( ompt_function_lookup_t ompt_fn_lookup, ompt_fns_t* fns );
typedef int (*ompt_finalize_t)( ompt_fns_t* fns );

typedef struct ompt_fns_t {
	ompt_initialize_t initialize;
	ompt_finalize_t finalize;	
} ompt_fns_t;

/** INQUIRY FUNCTION */

typedef int (*ompt_callback_set_t)
(
	ompt_callbacks_t event, 
	ompt_callback_t callback
);

typedef int (*ompt_callback_get_t)
(
	ompt_callbacks_t event, 
	ompt_callback_t* callback
);

typedef ompt_data_t* (*ompt_get_thread_data_t)
(
	void
);

typedef ompt_state_t (*ompt_get_state_t)
(
	ompt_wait_id_t *wait_id
);

typedef int (*ompt_get_parallel_info_t)
(
	int ancestor_level,
	ompt_data_t ** parallel_data,
	int *team_size
);

typedef int (*ompt_get_task_info_t)
(
	int ancestor_level,
	ompt_task_flag_t *type,
	ompt_data_t **task_data,
	ompt_frame_t **task_frame,
	ompt_data_t ** parallel_data,
	int *thread_num
);

/** CALLBACK **/
typedef void (*ompt_callback_thread_begin_t)
(
	ompt_thread_type_t,
	ompt_data_t *thread_data
);

typedef void (*ompt_callback_thread_end_t)
(
	ompt_data_t *thread_data
);

typedef void (*ompt_callback_work_t)
(
   ompt_work_t wstype,
   ompt_scope_endpoint_t endpoint,
   ompt_data_t* parallel_data,
   ompt_data_t* task_data,
   uint64_t count,
   const void * codeptr_ra
);

typedef void (*ompt_callback_parallel_begin_t)
(
   ompt_data_t* encountering_task_data,
   const ompt_frame_t* encountering_task_frame,
   ompt_data_t* parallel_data,
   unsigned int requested_parallelism,
   int flags,
   const void * codeptr_ra 
);

typedef void (*ompt_callback_sync_region_t)
(  
   ompt_sync_region_t kind,
   ompt_scope_endpoint_t endpoint,
   ompt_data_t* parallel_data,
   ompt_data_t* task_data,
   const void * codeptr_ra 
);

typedef void (*ompt_callback_parallel_end_t)
(
   ompt_data_t* parallel_data,
   ompt_data_t* encountering_task_data,
   int flags,
   const void * codeptr_ra	
);

typedef void (*ompt_callback_task_create_t)
(
	ompt_data_t* parent_task_data,
	const ompt_frame_t* parent_frame,
	ompt_data_t *new_task_data,
	ompt_task_flag_t task_type,
	int has_dependences,
	const void* codeptr_ra
);

typedef void (*ompt_callback_idle_t) 
(
	ompt_scope_endpoint_t endpoint
);

typedef void (*ompt_callback_implicit_task_t) 
( 
	ompt_scope_endpoint_t endpoint,
	ompt_data_t *parallel_data,
	ompt_data_t *task_data,
	unsigned int actial_parallelism,
	unsigned int index,
    int flags
);

typedef void (*ompt_callback_task_dependences_t) ( 
	ompt_data_t* task_data,
	const ompt_task_dependence_t *deps,
	int ndeps
);

typedef void (*ompt_callback_lock_init_t)
(
	ompt_mutex_kind_t kind,
	unsigned int hint,
	unsigned int impl,
	ompt_wait_id_t wait_id,
	const void* codeptr_ra
);

typedef void (*ompt_callback_lock_destroy_t)
(
	ompt_mutex_kind_t kind,
	ompt_wait_id_t wait_id,
   const void* codeptr_ra
);

typedef void (*ompt_callback_mutex_acquire_t)
(
	ompt_mutex_kind_t kind,
   unsigned int hint,
   unsigned int impl,
   ompt_wait_id_t wait_id,
   const void* codeptr_ra
);

typedef void (*ompt_callback_task_schedule_t )
(
	ompt_data_t *prior_task_data,
	ompt_task_status_t prior_task_status,
	ompt_data_t* next_task_data
);

typedef void (*ompt_callback_mutex_t)
(
	ompt_mutex_kind_t kind,
   ompt_wait_id_t wait_id,
   const void* codeptr_ra
);

typedef void (*ompt_callback_nest_lock_t)
(
	ompt_scope_endpoint_t endpoint,
   ompt_wait_id_t wait_id,
   const void* codeptr_ra
);

typedef void (*ompt_callback_flush_t)
(  
	ompt_data_t* task_data,
	const void* codeptr_ra
);

typedef void (*ompt_callback_cancel_t) 
(
	ompt_data_t* task_data,
	int flags,
   const void* codeptr_ra
);
#endif /* __MPCOMP_OMPT_H__ */
