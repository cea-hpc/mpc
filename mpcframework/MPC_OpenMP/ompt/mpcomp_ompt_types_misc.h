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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPCOMP_OMPT_TYPES_MISCELLANEOUS_H__
#define __MPCOMP_OMPT_TYPES_MISCELLANEOUS_H__

#include <stdint.h>

/** used to mark OMPT functions obtained from look ip function passed to ompt_initialize_fn_t */
#define OMPT_API

/** used to mark OMPT functions obtained from look ip function passed to ompt_target_get_device_info */
#define OMPT_TARG_API

typedef void ompt_target_device_t;	/**< opaque object representing a target device */
typedef uint64_t ompt_target_time_t;	/**< raw time value on device */
typedef void ompt_target_buffer_t;	/**< opaque handle for a target buffer */
typedef uint64_t ompt_target_buffer_cursors_t; /**< opaque handle for position in target buffer */

typedef enum ompt_thread_type_e
{
	ompt_thread_initial = 1,
	ompt_thread_worker = 2,
	ompt_thread_other = 3,
	ompt_thread_unknown = 4
}ompt_thread_type_t;

typedef enum ompt_sync_region_kind_e
{
	ompt_sync_region_barrier = 1,
	ompt_sync_region_taskwait = 2,
	ompt_sync_region_taskgroup = 3
}ompt_sync_region_kind_t;

typedef enum ompt_target_data_op_e
{
	ompt_target_data_alloc = 1,
	ompt_target_data_transfer_to_dev = 2,
	ompt_target_data_transfer_from_dev = 3,
	ompt_target_data_delete = 4
} ompt_target_data_op_t;

typedef enum ompt_worksharing_type_e
{
	ompt_worksharing_loop = 1,	
	ompt_worksharing_sections = 2,	
	ompt_worksharing_single_executor = 3,	
	ompt_worksharing_single_other = 4,	
	ompt_worksharing_workshare = 5,	
	ompt_worksharing_distribute = 6	
}ompt_worksharing_type_t;

typedef enum ompt_native_mon_flag_e
{
ompt_native_data_motion_explicite = 1,
ompt_native_data_motion_implicite = 2,
ompt_native_kernel_invocation = 4,
ompt_native_kernel_execution = 8,
ompt_native_driver = 16,
ompt_native_runtime = 32,
ompt_native_overhead = 64,
ompt_native_idleness = 128
}ompt_native_mon_flag_t;

typedef enum ompt_task_type_e
{
	ompt_task_initial = 1,
	ompt_task_implicite = 2,
	ompt_task_explicite = 3,
	ompt_task_target = 4,
	ompt_task_degenerate = 5
}ompt_task_type_t;

typedef enum ompt_target_type_e
{
	ompt_target = 1,	
	ompt_target_enter_data = 2,	
	ompt_target_exit_data = 3,	
	ompt_target_update = 4
}ompt_target_type_t;

typedef enum ompt_target_map_flag_e
{
	ompt_target_map_flag_to = 1,
	ompt_target_map_flag_from = 2,
	ompt_target_map_flag_alloc = 4,
	ompt_target_map_flag_release = 8,
	ompt_target_map_flag_delete = 16
} ompt_target_map_flag_t;

typedef enum ompt_task_dependence_flag_e
{
	// a two bit field for the dependence type
	ompt_task_dependence_type_out = 1,
	ompt_task_dependence_type_in = 2,
	ompt_task_dependence_type_inout = 3,
}ompt_task_dependence_flag_t;

typedef struct ompt_task_dependence_s
{
	void* variable_addr;
	uint32_t dependence_flags;
} ompt_task_dependence_t;

#define ompt_hwid_none	(-1)
#define ompt_dev_task_none	(~0ULL)
#define ompt_time_none	(~0ULL)

#define ompt_id_none 0

#define ompt_mutex_impl_unknown	0

#endif /* __MPCOMP_OMPT_TYPES_MISCELLANEOUS_H__ */
