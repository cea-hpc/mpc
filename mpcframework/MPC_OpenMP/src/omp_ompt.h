#ifndef MPC_OMP_OMP_OMPT_H_
#define MPC_OMP_OMP_OMPT_H_

#include <string.h>

/* For definition of OMPT_SUPPORT macro */
#include "mpcomp_types.h"
#include "sctk_alloc.h"

#include "ompt.h"

void _mpc_ompt_pre_init( void );
void _mpc_ompt_post_init( void );

/** used to mark OMPT functions obtained from look ip function passed to ompt_initialize_fn_t */
#define OMPT_API

/** used to mark OMPT functions obtained from look ip function passed to ompt_target_get_device_info */
#define OMPT_TARG_API

typedef void ompt_target_device_t;			   /**< opaque object representing a target device */
typedef uint64_t ompt_target_time_t;		   /**< raw time value on device */
typedef void ompt_target_buffer_t;			   /**< opaque handle for a target buffer */
typedef uint64_t ompt_target_buffer_cursors_t; /**< opaque handle for position in target buffer */

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
} ompt_native_mon_flag_t;

typedef enum ompt_target_map_flag_e
{
	ompt_target_map_flag_to = 1,
	ompt_target_map_flag_from = 2,
	ompt_target_map_flag_alloc = 4,
	ompt_target_map_flag_release = 8,
	ompt_target_map_flag_delete = 16
} ompt_target_map_flag_t;

#define ompt_hwid_none ( -1 )
#define ompt_dev_task_none ( ~0ULL )
#define ompt_time_none ( ~0ULL )

#define ompt_id_none 0

#define ompt_mutex_impl_unknown 0

#define _OMP_EXTERN extern "C"

typedef uint64_t ompt_task_id_t;
typedef uint64_t ompt_wait_id_t;
typedef uint64_t ompt_thread_id_t;
typedef uint64_t ompt_parallel_id_t;

typedef struct mpcomp_ompt_task_info_s
{
	ompt_frame_t frame;
	ompt_data_t *task_data;
	//ompt_task_id_t taskid;
#if MPCOMP_USE_TASKDEP
	unsigned int ndeps;
	//ompt_task_dependence_t *deps;
#endif /* MPCOMP_USE_TASKDEP */
} mpcomp_ompt_task_info_t;

typedef struct mpcomp_ompt_team_info_s
{
	/* ... */
	int tmp;
} mpcomp_ompt_team_info_t;

typedef struct mpcomp_ompt_parallel_info_s
{
	ompt_data_t *parent_task_data; /**< parent task data (tool controlled) */
	ompt_data_t *parallel_data;	/**< parallel data (tool controlled) */
} mpcomp_ompt_parallel_info_t;

typedef struct mpcomp_ompt_thread_info_s
{
	void *idle_frame;
	//ompt_state_t state;
	ompt_wait_id_t wait_id;
	ompt_data_t *thread_data;
} mpcomp_ompt_thread_info_t;

static inline void mpcomp_ompt_task_info_reset( mpcomp_ompt_task_info_t *info )
{
	sctk_assert( info );
	memset( info, 0, sizeof( mpcomp_ompt_task_info_t ) );
}

static inline mpcomp_ompt_task_info_t *mpcomp_ompt_task_info_alloc( void )
{
	mpcomp_ompt_task_info_t *info = NULL;
	info = ( mpcomp_ompt_task_info_t * ) sctk_malloc( sizeof( mpcomp_ompt_task_info_t ) );
	sctk_assert( info );
	mpcomp_ompt_task_info_reset( info );
	return info;
}

static inline void mpcomp_ompt_parallel_info_reset( mpcomp_ompt_parallel_info_t *info )
{
	sctk_assert( info );
	memset( info, 0, sizeof( mpcomp_ompt_parallel_info_t ) );
}

static inline mpcomp_ompt_parallel_info_t *mpcomp_ompt_parallel_info_alloc( void )
{
	mpcomp_ompt_parallel_info_t *info = NULL;
	info = ( mpcomp_ompt_parallel_info_t * ) sctk_malloc( sizeof( mpcomp_ompt_parallel_info_t ) );
	sctk_assert( info );
	mpcomp_ompt_parallel_info_reset( info );
	return info;
}

static inline void
mpcomp_ompt_thread_info_reset( mpcomp_ompt_thread_info_t *info )
{
	sctk_assert( info );
	memset( info, 0, sizeof( mpcomp_ompt_thread_info_t ) );
}

static inline mpcomp_ompt_thread_info_t *mpcomp_ompt_thread_info_alloc( void )
{
	mpcomp_ompt_thread_info_t *info = NULL;
	info = ( mpcomp_ompt_thread_info_t * ) sctk_malloc( sizeof( mpcomp_ompt_thread_info_t ) );
	sctk_assert( info );
	mpcomp_ompt_thread_info_reset( info );
	return info;
}

extern int _omp_ompt_enabled_flag;

static inline int _mpc_omp_ompt_is_enabled( void )
{
	return _omp_ompt_enabled_flag;
}

static inline ompt_callback_t mpcomp_ompt_get_callback( int callback_type )
{
	return OMPT_Callbacks[callback_type];
}

#endif /* MPC_OMP_OMP_OMPT_H_ */