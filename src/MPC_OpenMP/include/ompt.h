/* ############################# MPC License ############################## */
/* # Tue Oct 12 12:25:59 CEST 2021                                        # */
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
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Ricardo Bispo vieira <ricardo.bispo-vieira@exascale-computing.eu>  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef __MPC_OMP_OMPT_H__
#define __MPC_OMP_OMPT_H__

#include <stdint.h>
#include <stdlib.h>

/*****
 * Iterator lookup function 
 */

typedef void (*ompt_callback_t) ( void );
typedef void (*ompt_interface_fn_t) ( void );

typedef ompt_interface_fn_t (*ompt_function_lookup_t)
(
    const char *interface_function_name
);

#define FOREACH_OMPT_STATE( macro )                                           \
    /* WORK STATES */                                                         \
    macro( ompt_state_work_serial,                     0x000, "working outside parallel")\
    macro( ompt_state_work_parallel,                   0x001, "working within parallel")\
    macro( ompt_state_work_reduction,                  0x002, "perform a reduction")\
    /* BARRIER WAIT STATES */                                                 \
    macro( ompt_state_wait_barrier,                    0x010, "generic barrier")\
    macro( ompt_state_wait_barrier_implicit_parallel,  0x011, "todo")         \
    macro( ompt_state_wait_barrier_implicit_workshare, 0x012, "todo")         \
    macro( ompt_state_wait_barrier_implicit,           0x013, "implicite barrier")\
    macro( ompt_state_wait_barrier_explicit,           0x014, "explicite barrier")\
    /* TASK WAIT STATES */                                                    \
    macro( ompt_state_wait_taskwait,                   0x020, "waiting at a taskwait")\
    macro( ompt_state_wait_taskgroup,                  0x021, "waiting at a taskgroup")\
    /* MUTEX WAIT STATES */                                                   \
    macro( ompt_state_wait_mutex,                      0x040, "waiting for any mutex kind")\
    macro( ompt_state_wait_lock,                       0x041, "waiting for lock")\
    macro( ompt_state_wait_critical,                   0x042, "waiting for critical")\
    macro( ompt_state_wait_atomic,                     0x043, "waiting for atomic")\
    macro( ompt_state_wait_ordered,                    0x044, "waiting for ordered")\
    /* TARGET WAIT STATES */                                                  \
    macro( ompt_state_wait_target,                     0x080, "waiting for target")\
    macro( ompt_state_wait_target_map,                 0x081, "waiting for target data")\
    macro( ompt_state_wait_target_update,              0x082, "waiting for target update")\
    /* MISC STATES */                                                         \
    macro( ompt_state_idle,                            0x100, "waiting for work")\
    macro( ompt_state_overhead,                        0x101, "non-wait overhead")\
    macro( ompt_state_undefined,                       0x102, "undefined thread state")

typedef enum ompt_state_e {
#define ompt_state_macro(state, code, desc) state = code,
    FOREACH_OMPT_STATE( ompt_state_macro )
#undef ompt_state_macro
} ompt_state_t;

/**
 * OMPT_MUTEX_IMPL
 */

#define FOREACH_OMPT_MUTEX_IMPL( macro )                                      \
    macro( ompt_mutex_impl_none,                 0, "unknown impl")           \
    macro( ompt_mutex_impl_mutex,                1, "mpcomp mutex")           \
    macro( ompt_mutex_impl_spinlock,             2, "mpcomp spinlock")        \
    macro( ompt_mutex_impl_mcslock,              3, "mpcomp mcslock")

typedef enum ompt_mutex_impl_e {
#define ompt_mutex_impl_macro(mutex_impl, code, desc) mutex_impl = code,
    FOREACH_OMPT_MUTEX_IMPL( ompt_mutex_impl_macro )
#undef ompt_mutex_impl_macro
} ompt_mutex_impl_t;


/**
 * OMPT_SET_RESULT
 */
#define FOREACH_OMPT_SET_RESULT( macro )                                      \
    macro( ompt_set_error,                              0)                    \
    macro( ompt_set_never,                              1)                    \
    macro( ompt_set_impossible,                         2)                    \
    macro( ompt_set_sometimes,                          3)                    \
    macro( ompt_set_sometimes_paired,                   4)                    \
    macro( ompt_set_always,                             5)

typedef enum ompt_set_result_e {
#define ompt_set_result_macro(name, code) name = code,
    FOREACH_OMPT_SET_RESULT( ompt_set_result_macro )
#undef ompt_set_result_macro
} ompt_set_result_t;

/** 
 * OMPT_CALLBACK
 */

#define FOREACH_OMPT_CALLBACK( macro )                                        \
    /*--- Mandatory Events ---*/                                              \
    macro( ompt_callback_thread_begin,                 1, ompt_set_always )   \
    macro( ompt_callback_thread_end,                   2, ompt_set_always )   \
    macro( ompt_callback_parallel_begin,               3, ompt_set_always )   \
    macro( ompt_callback_parallel_end,                 4, ompt_set_always )   \
    macro( ompt_callback_task_create,                  5, ompt_set_always )   \
    macro( ompt_callback_task_schedule,                6, ompt_set_always )   \
    macro( ompt_callback_implicit_task,                7, ompt_set_always )   \
    macro( ompt_callback_target,                       8, ompt_set_never )    \
    macro( ompt_callback_target_data_op,               9, ompt_set_never )    \
    macro( ompt_callback_target_submit,               10, ompt_set_never )    \
    macro( ompt_callback_control_tool,                11, ompt_set_always )   \
    macro( ompt_callback_device_initialize,           12, ompt_set_never )    \
    macro( ompt_callback_device_finalize,             13, ompt_set_never )    \
    macro( ompt_callback_device_load,                 14, ompt_set_never )    \
    macro( ompt_callback_device_unload,               15, ompt_set_never )    \
    /*--- Optional Events ---*/                                               \
    macro( ompt_callback_sync_region_wait,            16, ompt_set_sometimes )\
    macro( ompt_callback_mutex_released,              17, ompt_set_sometimes )\
    macro( ompt_callback_dependences,                 18, ompt_set_always )   \
    macro( ompt_callback_task_dependence,             19, ompt_set_never )    \
    macro( ompt_callback_work,                        20, ompt_set_always )\
    macro( ompt_callback_master,                      21, ompt_set_sometimes )\
    macro( ompt_callback_target_map,                  22, ompt_set_never )    \
    macro( ompt_callback_sync_region,                 23, ompt_set_always )   \
    macro( ompt_callback_lock_init,                   24, ompt_set_sometimes )\
    macro( ompt_callback_lock_destroy,                25, ompt_set_sometimes )\
    macro( ompt_callback_mutex_acquire,               26, ompt_set_sometimes )\
    macro( ompt_callback_mutex_acquired,              27, ompt_set_sometimes )\
    macro( ompt_callback_nest_lock,                   28, ompt_set_sometimes )\
    macro( ompt_callback_flush,                       29, ompt_set_never )    \
    macro( ompt_callback_cancel,                      30, ompt_set_never )    \
    macro( ompt_callback_reduction,                   31, ompt_set_never )    \
    macro( ompt_callback_dispatch,                    32, ompt_set_sometimes )

typedef enum ompt_callbacks_e {
#define ompt_callback_macro(name, code, status) name = code,
	FOREACH_OMPT_CALLBACK( ompt_callback_macro )
#undef ompt_callback_macro
} ompt_callbacks_t;

/*
 * MUTEX KIND
 */

typedef enum ompt_mutex_e
{
    ompt_mutex_lock           = 1,
    ompt_mutex_test_lock      = 2,
    ompt_mutex_nest_lock      = 3,
    ompt_mutex_test_nest_lock = 4,
    ompt_mutex_critical       = 5,
    ompt_mutex_atomic         = 6,
    ompt_mutex_ordered        = 7
} ompt_mutex_t;

#define ompt_addr_none ~0
#define ompt_hwid_none 0
typedef uint64_t ompt_hwid_t;
typedef void ompt_device_t;
#define ompt_time_none 0
typedef uint64_t ompt_device_time_t;
typedef void ompt_buffer_t;
typedef uint64_t ompt_buffer_cursor_t;

typedef enum ompt_dispatch_e
{
    ompt_dispatch_iteration = 1,
    ompt_dispatch_section   = 2
} ompt_dispatch_t;

typedef enum ompt_native_mon_flag_e
{
    ompt_native_data_motion_explicit = 0x01,
    ompt_native_data_motion_implicit = 0x02,
    ompt_native_kernel_invocation    = 0x04,
    ompt_native_kernel_execution     = 0x08,
    ompt_native_driver               = 0x10,
    ompt_native_runtime              = 0x20,
    ompt_native_overhead             = 0x40,
    ompt_native_idleness             = 0x80
} ompt_native_mon_flag_t;

typedef enum ompt_target_map_flag_e
{
    ompt_target_map_flag_to       = 0x01,
    ompt_target_map_flag_from     = 0x02,
    ompt_target_map_flag_alloc    = 0x04,
    ompt_target_map_flag_release  = 0x08,
    ompt_target_map_flag_delete   = 0x10,
    ompt_target_map_flag_implicit = 0x20
} ompt_target_map_flag_t;

typedef enum ompt_target_data_op_e
{
    ompt_target_data_alloc                = 1,
    ompt_target_data_transfer_to_device   = 2,
    ompt_target_data_transfer_from_device = 3,
    ompt_target_data_delete               = 4,
    ompt_target_data_associate            = 5,
    ompt_target_data_disassociate         = 6
} ompt_target_data_op_t;

typedef enum ompt_target_e
{
    ompt_target            = 1,
    ompt_target_enter_data = 2,
    ompt_target_exit_data  = 3,
    ompt_target_update     = 4
} ompt_target_t;

typedef enum ompt_scope_endpoint_e
{
    ompt_scope_begin = 1,
    ompt_scope_end   = 2
} ompt_scope_endpoint_t;

typedef enum ompt_sync_region_e
{
    ompt_sync_region_barrier              		  = 1,
    ompt_sync_region_barrier_implicit       		= 2,
    ompt_sync_region_barrier_explicit       		= 3,
    ompt_sync_region_barrier_implementation 		= 4,
    ompt_sync_region_taskwait              		  = 5,
    ompt_sync_region_taskgroup              		= 6,
    ompt_sync_region_reduction             		  = 7,
		ompt_sync_region_barrier_implicit_workshare = 8,
		ompt_sync_region_barrier_implicit_parallel  = 9,
		ompt_sync_region_teams 											= 10
} ompt_sync_region_t;

typedef enum ompt_cancel_flag_e
{
    ompt_cancel_parallel       = 0x01,
    ompt_cancel_sections       = 0x02,
    ompt_cancel_loop           = 0x04,
    ompt_cancel_taskgroup      = 0x08,
    ompt_cancel_activated      = 0x10,
    ompt_cancel_detected       = 0x20,
    ompt_cancel_discarded_task = 0x40
} ompt_cancel_flag_t;

/*
 * identifiers
 */

#define ompt_id_none 0
typedef uint64_t ompt_id_t;
#define ompt_wait_id_none 0
typedef uint64_t ompt_wait_id_t;  /**< identifies what a thread is awaiting */

typedef union ompt_data_u{
    uint64_t value;               /**< integer ID under tool control */
    void *ptr;                    /**< pointer under tool control */
} ompt_data_t;

#define ompt_data_none {0}

#define FOREACH_OMPT_TASK_STATUS( macro )                                     \
    macro( ompt_task_complete,                         1 )                    \
    macro( ompt_task_yield,                            2 )                    \
    macro( ompt_task_cancel,                           3 )                    \
    macro( ompt_task_detach,                           4 )                    \
    macro( ompt_task_early_fulfill,                    5 )                    \
    macro( ompt_task_late_fulfill,                     6 )                    \
    macro( ompt_task_switch,                           7 )

typedef enum ompt_task_status_e
{
#define ompt_task_status_gen( name, id ) name = id,
    FOREACH_OMPT_TASK_STATUS( ompt_task_status_gen )
#undef ompt_task_status_gen
} ompt_task_status_t;

/*
 * ompt_frame_t
 */

typedef struct ompt_frame_s
{
    ompt_data_t exit_frame;  /**< runtime frame that reenters user code */
    ompt_data_t enter_frame; /**< user frame that reenters the runtime  */
    int exit_frame_flags;
    int enter_frame_flags;
} ompt_frame_t;

typedef enum ompt_frame_flag_e
{
    ompt_frame_runtime      = 0x00,
    ompt_frame_application  = 0x01,
    ompt_frame_cfa          = 0x10,
    ompt_frame_framepointer = 0x20,
    ompt_frame_stackaddress = 0x30 
} ompt_frame_flag_t;

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
} ompt_task_flag_t;

typedef enum ompt_thread_e
{
    ompt_thread_initial  = 1,
    ompt_thread_worker   = 2,
    ompt_thread_other    = 3,
    ompt_thread_unknown  = 4
} ompt_thread_t;

typedef enum ompt_dependence_type_e
{
    ompt_dependence_type_in            = 1,
    ompt_dependence_type_out           = 2,
    ompt_dependence_type_inout         = 3,
    ompt_dependence_type_mutexinoutset = 4,
    ompt_dependence_type_source        = 5,
    ompt_dependence_type_sink          = 6
} ompt_dependence_type_t;

typedef enum ompt_work_e
{
    ompt_work_loop            = 1,
    ompt_work_sections        = 2,
    ompt_work_single_executor = 3,
    ompt_work_single_other    = 4,
    ompt_work_workshare       = 5,
    ompt_work_distribute      = 6,
    ompt_work_taskloop        = 7
} ompt_work_t;

typedef struct ompt_dependence_s
{
    ompt_data_t variable;
    ompt_dependence_type_t dependence_type;
} ompt_dependence_t;

/*
 * Record types
 */

typedef enum ompt_record_e
{
    ompt_record_ompt    = 1,
    ompt_record_native  = 2,
    ompt_record_invalid = 3
} ompt_record_t;

typedef enum ompt_record_native_e
{
    ompt_record_native_info  = 1,
    ompt_record_native_event = 2
} ompt_record_native_t;

typedef struct ompt_record_abstract_s
{
    ompt_record_native_t rclass;
    const char *type;
    ompt_device_time_t start_time;
    ompt_device_time_t end_time;
    ompt_hwid_t hwid;
} ompt_record_abstract_t;

/**
 * INITIALIZATION FUNCTIONS
 *****/

typedef int (*ompt_initialize_t)( ompt_function_lookup_t lookup,
                                  int initial_device_num,
                                  ompt_data_t *tool_data );
typedef void (*ompt_finalize_t)( ompt_data_t *tool_data );

typedef struct ompt_start_tool_result_s {
    ompt_initialize_t initialize;
    ompt_finalize_t finalize;
    ompt_data_t tool_data;
} ompt_start_tool_result_t;

/** INQUIRY FUNCTION */

typedef int (*ompt_enumerate_states_t)
(
    int current_state,
    int *next_state,
    const char **next_state_name
);

typedef int (*ompt_enumerate_mutex_impls_t)
(
    int current_impl,
    int *next_impl,
    const char **next_impl_name
);

typedef ompt_set_result_t (*ompt_set_callback_t)
(
    ompt_callbacks_t event,
    ompt_callback_t callback
);

typedef int (*ompt_get_callback_t)
(
    ompt_callbacks_t event,
    ompt_callback_t* callback
);

typedef ompt_data_t *(*ompt_get_thread_data_t) ( void );
typedef int (*ompt_get_num_procs_t) ( void );
typedef int (*ompt_get_num_places_t) ( void );
typedef int (*ompt_get_place_num_t) ( void );
typedef int (*ompt_get_proc_id_t) ( void );
typedef int (*ompt_get_num_devices_t) ( void );
typedef uint64_t (*ompt_get_unique_id_t) ( void );
typedef void (*ompt_finalize_tool_t) ( void );

typedef int (*ompt_get_place_proc_ids_t)
(
    int place_num,
    int ids_size,
    int *ids
);

typedef int (*ompt_get_partition_place_nums_t)
(
    int place_nums_size,
    int *place_nums
);

typedef int (*ompt_get_state_t)
(
    ompt_wait_id_t *wait_id
);

typedef int (*ompt_get_parallel_info_t)
(
    int ancestor_level,
    ompt_data_t **parallel_data,
    int *team_size
);

typedef int (*ompt_get_task_info_t)
(
    int ancestor_level,
    int *flags,
    ompt_data_t **task_data,
    ompt_frame_t **task_frame,
    ompt_data_t **parallel_data,
    int *thread_num
);

typedef int (*ompt_get_task_memory_t)
(
    void **addr,
    size_t *size,
    int block
);

typedef int (*ompt_get_target_info_t)
(
    uint64_t *device_num,
    ompt_id_t *target_id,
    ompt_id_t *host_op_id
);

/** CALLBACK **/

typedef void (*ompt_callback_thread_begin_t)
(
    ompt_thread_t thread_type,
    ompt_data_t *thread_data
);

typedef void (*ompt_callback_thread_end_t)
(
    ompt_data_t *thread_data
);

typedef void (*ompt_callback_parallel_begin_t)
(
    ompt_data_t *encountering_task_data,
    const ompt_frame_t *encountering_task_frame,
    ompt_data_t *parallel_data,
    unsigned int requested_parallelism,
    int flags,
    const void *codeptr_ra
);

typedef void (*ompt_callback_parallel_end_t)
(
    ompt_data_t *parallel_data,
    ompt_data_t *encountering_task_data,
    int flags,
    const void *codeptr_ra
);

typedef void (*ompt_callback_work_t)
(
    ompt_work_t wstype,
    ompt_scope_endpoint_t endpoint,
    ompt_data_t* parallel_data,
    ompt_data_t* task_data,
    uint64_t count,
    const void *codeptr_ra
);

typedef void (*ompt_callback_dispatch_t)
(
    ompt_data_t *parallel_data,
    ompt_data_t* task_data,
    ompt_dispatch_t kind,
    ompt_data_t instance
);

typedef void (*ompt_callback_task_create_t)
(
    ompt_data_t *encountering_task_data,
    const ompt_frame_t *encountering_task_frame,
    ompt_data_t *new_task_data,
    int flags,
    int has_dependences,
    const void *codeptr_ra
);

typedef void (*ompt_callback_dependences_t)
(
    ompt_data_t *task_data,
    const ompt_dependence_t *deps,
    int ndeps
);

typedef void (*ompt_callback_task_dependence_t)
(
    ompt_data_t *src_task_data,
    ompt_data_t *sink_task_data
);

typedef void (*ompt_callback_task_schedule_t )
(
    ompt_data_t *prior_task_data,
    ompt_task_status_t prior_task_status,
    ompt_data_t *next_task_data
);

typedef void (*ompt_callback_implicit_task_t)
(
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    unsigned int actual_parallelism,
    unsigned int index,
    int flags
);

typedef void (*ompt_callback_master_t)
(
    ompt_scope_endpoint_t endpoint,
    ompt_data_t *parallel_data,
    ompt_data_t *task_data,
    const void *codeptr_ra
);

typedef void (*ompt_callback_sync_region_t)
(
    ompt_sync_region_t kind,
    ompt_scope_endpoint_t endpoint,
    ompt_data_t* parallel_data,
    ompt_data_t* task_data,
    const void *codeptr_ra
);

#define ompt_callback_sync_region_wait_t ompt_callback_sync_region_t
#define ompt_callback_reduction_t ompt_callback_sync_region_t

typedef void (*ompt_callback_mutex_acquire_t)
(
    ompt_mutex_t kind,
    unsigned int hint,
    unsigned int impl,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra
);

#define ompt_callback_lock_init_t ompt_callback_mutex_acquire_t
#define ompt_callback_lock_destroy_t ompt_callback_mutex_t

typedef void (*ompt_callback_mutex_t)
(
    ompt_mutex_t kind,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra
);

#define ompt_callback_mutex_acquired_t ompt_callback_mutex_t
#define ompt_callback_mutex_released_t ompt_callback_mutex_t

typedef void (*ompt_callback_nest_lock_t)
(
    ompt_scope_endpoint_t endpoint,
    ompt_wait_id_t wait_id,
    const void *codeptr_ra
);

typedef void (*ompt_callback_flush_t)
(
    ompt_data_t *thread_data,
    const void *codeptr_ra
);

typedef void (*ompt_callback_cancel_t)
(
    ompt_data_t *task_data,
    int flags,
    const void *codeptr_ra
);

typedef void (*ompt_callback_device_initialize_t)
(
    int device_num,
    const char *type,
    ompt_device_t *device,
    ompt_function_lookup_t lookup,
    const char *documentation
);

typedef void (*ompt_callback_device_finalize_t)
(
    int device_num
);

typedef void (*ompt_callback_device_load_t)
(
    int device_num,
    const char *filename,
    int64_t offset_in_file,
    void *vma_in_file,
    size_t bytes,
    void *host_addr,
    void *device_addr,
    uint64_t module_id
);

typedef void (*ompt_callback_device_unload_t)
(
    int device_num,
    uint64_t module_id
);

typedef void (*ompt_callback_buffer_request_t)
(
    int device_num,
    ompt_buffer_t **buffer,
    size_t *bytes
);

typedef void (*ompt_callback_buffer_complete_t)
(
    int device_num,
    ompt_buffer_t *buffer,
    size_t bytes,
    ompt_buffer_cursor_t begin,
    int buffer_owned
);

typedef void (*ompt_callback_target_data_op_t)
(
    ompt_id_t target_id,
    ompt_id_t host_op_id,
    ompt_target_data_op_t optype,
    void *src_addr,
    int src_device_num,
    void *dest_addr,
    int dest_device_num,
    size_t bytes,
    const void *codeptr_ra
);

typedef void (*ompt_callback_target_t)
(
    ompt_target_t kind,
    ompt_scope_endpoint_t endpoint,
    int device_num,
    ompt_data_t *task_data,
    ompt_id_t target_id,
    const void *codeptr_ra
);

typedef void (*ompt_callback_target_map_t)
(
    ompt_id_t target_id,
    unsigned int nitems,
    void **host_addr,
    void **device_addr,
    size_t *bytes,
    unsigned int *mapping_flags,
    const void *codeptr_ra
);

typedef void (*ompt_callback_target_submit_t)
(
    ompt_id_t target_id,
    ompt_id_t host_op_id,
    unsigned int requested_num_teams
);

typedef int (*ompt_callback_control_tool_t)
(
    uint64_t command,
    uint64_t modifier,
    void *arg,
    const void *codeptr_ra
);

/** TRACE RECORD **/

typedef struct ompt_record_thread_begin_s
{
    ompt_thread_t thread_type;
} ompt_record_thread_begin_t;

typedef struct ompt_record_parallel_begin_s
{
    ompt_id_t encountering_task_id;
    ompt_id_t parallel_id;
    unsigned int requested_parallelism;
    int flags;
    const void *codeptr_ra;
} ompt_record_parallel_begin_t;

typedef struct ompt_record_parallel_end_s
{
    ompt_id_t parallel_id;
    ompt_id_t encountering_task_id;
    int flags;
    const void *codeptr_ra;
} ompt_record_parallel_end_t;

typedef struct ompt_record_work_s
{
    ompt_work_t wstype;
    ompt_scope_endpoint_t endpoint;
    ompt_id_t parallel_id;
    ompt_id_t task_id;
    uint64_t count;
    const void *codeptr_ra;
} ompt_record_work_t;

typedef struct ompt_record_dispatch_s
{
    ompt_id_t parallel_id;
    ompt_id_t task_id;
    ompt_dispatch_t kind;
    ompt_data_t instance;
} ompt_record_dispatch_t;

typedef struct ompt_record_task_create_s
{
    ompt_id_t encountering_task_id;
    ompt_id_t new_task_id;
    int flags;
    int has_dependences;
    const void *codeptr_ra;
} ompt_record_task_create_t;

typedef struct ompt_record_dependences_s
{
    ompt_id_t task_id;
    ompt_dependence_t dep;
    int ndeps;
} ompt_record_dependences_t;

typedef struct ompt_record_task_dependence_s
{
    ompt_id_t src_task_id;
    ompt_id_t sink_task_id;
} ompt_record_task_dependence_t;

typedef struct ompt_record_task_schedule_s
{
    ompt_id_t prior_task_id;
    ompt_task_status_t prior_task_status;
    ompt_id_t next_task_id;
} ompt_record_task_schedule_t;

typedef struct ompt_record_implicit_task_s
{
    ompt_scope_endpoint_t endpoint;
    ompt_id_t parallel_id;
    ompt_id_t task_id;
    unsigned int actual_parallelism;
    unsigned int index;
    int flags;
} ompt_record_implicit_task_t;

typedef struct ompt_record_master_s
{
    ompt_scope_endpoint_t endpoint;
    ompt_id_t parallel_id;
    ompt_id_t task_id;
    const void *codeptr_ra;
} ompt_record_master_t;

typedef struct ompt_record_sync_region_s
{
    ompt_sync_region_t kind;
    ompt_scope_endpoint_t endpoint;
    ompt_id_t parallel_id;
    ompt_id_t task_id;
    const void *codeptr_ra;
} ompt_record_sync_region_t;

typedef struct ompt_record_mutex_acquire_s
{
    ompt_mutex_t kind;
    unsigned int hint;
    unsigned int impl;
    ompt_wait_id_t wait_id;
    const void *codeptr_ra;
} ompt_record_mutex_acquire_t;

typedef struct ompt_record_mutex_s
{
    ompt_mutex_t kind;
    ompt_wait_id_t wait_id;
    const void *codeptr_ra;
} ompt_record_mutex_t;

typedef struct ompt_record_nest_lock_s
{
    ompt_scope_endpoint_t endpoint;
    ompt_wait_id_t wait_id;
    const void *codeptr_ra;
} ompt_record_nest_lock_t;

typedef struct ompt_record_flush_s
{
    const void *codeptr_ra;
} ompt_record_flush_t;

typedef struct ompt_record_cancel_s
{
    ompt_id_t task_id;
    int flags;
    const void *codeptr_ra;
} ompt_record_cancel_t;

typedef struct ompt_record_target_data_op_s
{
    ompt_id_t host_op_id;
    ompt_target_data_op_t optype;
    void *src_addr;
    int src_device_num;
    void *dest_addr;
    int dest_device_num;
    size_t bytes;
    ompt_device_time_t end_time;
    const void *codeptr_ra;
} ompt_record_target_data_op_t;

typedef struct ompt_record_target_s
{
    ompt_target_t kind;
    ompt_scope_endpoint_t endpoint;
    int device_num;
    ompt_id_t task_id;
    ompt_id_t target_id;
    const void *codeptr_ra;
} ompt_record_target_t;

typedef struct ompt_record_target_map_s
{
    ompt_id_t target_id;
    unsigned int nitems;
    void **host_addr;
    void **device_addr;
    size_t *bytes;
    unsigned int *mapping_flags;
    const void *codeptr_ra;
} ompt_record_target_map_t;

typedef struct ompt_record_target_kernel_s
{
    ompt_id_t host_op_id;
    unsigned int requested_num_teams;
    unsigned int granted_num_teams;
    ompt_device_time_t end_time;
} ompt_record_target_kernel_t;

typedef struct ompt_record_control_tool_s
{
    uint64_t command;
    uint64_t modifier;
    const void *codeptr_ra;
} ompt_record_control_tool_t;

typedef struct ompt_record_ompt_s
{
    ompt_callbacks_t type;
    ompt_device_time_t time;
    ompt_id_t thread_id;
    ompt_id_t target_id;
    union {
        ompt_record_thread_begin_t thread_begin;
        ompt_record_parallel_begin_t parallel_begin;
        ompt_record_parallel_end_t parallel_end;
        ompt_record_work_t work;
        ompt_record_dispatch_t dispatch;
        ompt_record_task_create_t task_create;
        ompt_record_dependences_t dependences;
        ompt_record_task_dependence_t task_dependence;
        ompt_record_task_schedule_t task_schedule;
        ompt_record_implicit_task_t implicit_task;
        ompt_record_master_t master;
        ompt_record_sync_region_t sync_region;
        ompt_record_mutex_acquire_t mutex_acquire;
        ompt_record_mutex_t mutex;
        ompt_record_nest_lock_t nest_lock;
        ompt_record_flush_t flush;
        ompt_record_cancel_t cancel;
        ompt_record_target_t target;
        ompt_record_target_data_op_t target_data_op;
        ompt_record_target_map_t target_map;
        ompt_record_target_kernel_t target_kernel;
        ompt_record_control_tool_t control_tool;
    } record;
} ompt_record_ompt_t;

/** TRACE ENTRY POINTS **/

typedef int (*ompt_get_device_num_procs_t) (
    ompt_device_t * device
);

typedef ompt_device_time_t (*ompt_get_device_time_t) (
    ompt_device_t * device
);

typedef double (*ompt_translate_time_t) (
    ompt_device_t * device,
    ompt_device_time_t time
);

typedef ompt_set_result_t (*ompt_set_trace_ompt_t) (
    ompt_device_t * device,
    unsigned int enable,
    unsigned int etype
);

typedef ompt_set_result_t (*ompt_set_trace_native_t) (
    ompt_device_t * device,
    int enable,
    int flags
);

typedef int (*ompt_start_trace_t) (
    ompt_device_t * device,
    ompt_callback_buffer_request_t request,
    ompt_callback_buffer_complete_t complete
);

typedef int (*ompt_pause_trace_t) (
    ompt_device_t * device,
    int begin_pause
);

typedef int (*ompt_flush_trace_t) (
    ompt_device_t * device
);

typedef int (*ompt_stop_trace_t) (
    ompt_device_t * device
);

typedef int (*ompt_advance_buffer_cursor_t) (
    ompt_device_t * device,
    ompt_buffer_t * buffer,
    size_t size,
    ompt_buffer_cursor_t current,
    ompt_buffer_cursor_t * next
);

typedef ompt_record_t (*ompt_get_record_type_t) (
    ompt_buffer_t * buffer,
    ompt_buffer_cursor_t current
);

typedef ompt_record_ompt_t *(*ompt_get_record_ompt_t) (
    ompt_buffer_t * buffer,
    ompt_buffer_cursor_t current
);

typedef void *(*ompt_get_record_native_t) (
    ompt_buffer_t * buffer,
    ompt_buffer_cursor_t current,
    ompt_id_t * host_op_id
);

typedef ompt_record_abstract_t *(*ompt_get_record_abstract_t) (
    void * native_record
);

#endif /* __MPC_OMP_OMPT_H__ */
