#ifndef __MPCOMP_OMPT_INTERNAL_H__
#define __MPCOMP_OMPT_INTERNAL_H__

#include <string.h>

#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "mpcomp_ompt_types_misc.h"

#define _OMP_EXTERN extern "C" 

typedef uint64_t ompt_task_id_t;
typedef uint64_t ompt_wait_id_t;
typedef uint64_t ompt_thread_id_t;
typedef uint64_t ompt_parallel_id_t;

typedef struct mpcomp_ompt_task_info_s
{
	ompt_frame_t frame;
	ompt_data_t* task_data;
	//ompt_task_id_t taskid;
#if MPCOMP_USE_TASKDEP
	unsigned int ndeps;
	ompt_dependence_t *deps;
#endif /* MPCOMP_USE_TASKDEP */
} mpcomp_ompt_task_info_t;

typedef struct mpcomp_ompt_team_info_s
{
	/* ... */
	int tmp;
} mpcomp_ompt_team_info_t;

typedef struct mpcomp_ompt_parallel_info_s{
	ompt_data_t* parent_task_data; /**< parent task data (tool controlled) */ 
	ompt_data_t* parallel_data; /**< parallel data (tool controlled) */ 
} mpcomp_ompt_parallel_info_t;

typedef struct mpcomp_ompt_thread_info_s
{
	void* idle_frame;
	//ompt_state_t state;
	ompt_wait_id_t wait_id;
	ompt_data_t* thread_data;	
} mpcomp_ompt_thread_info_t;

static inline void mpcomp_ompt_task_info_reset( mpcomp_ompt_task_info_t* info )
{
	sctk_assert( info );
	memset( info, 0, sizeof( mpcomp_ompt_task_info_t ));
}

static inline mpcomp_ompt_task_info_t* mpcomp_ompt_task_info_alloc( void )
{
	mpcomp_ompt_task_info_t* info = NULL;
	info = ( mpcomp_ompt_task_info_t*) sctk_malloc( sizeof( mpcomp_ompt_task_info_t ));
	sctk_assert( info );
	mpcomp_ompt_task_info_reset( info );
	return info;
}

static inline void mpcomp_ompt_parallel_info_reset( mpcomp_ompt_parallel_info_t* info )
{
	sctk_assert( info );
	memset( info, 0, sizeof( mpcomp_ompt_parallel_info_t ));
}

static inline mpcomp_ompt_parallel_info_t* mpcomp_ompt_parallel_info_alloc( void )
{
	mpcomp_ompt_parallel_info_t* info = NULL;
	info = ( mpcomp_ompt_parallel_info_t*) sctk_malloc( sizeof( mpcomp_ompt_parallel_info_t ));
	sctk_assert( info );
	mpcomp_ompt_parallel_info_reset( info );
	return info;
}

static inline void 
mpcomp_ompt_thread_info_reset( mpcomp_ompt_thread_info_t* info )
{
	sctk_assert( info );
	memset( info, 0, sizeof( mpcomp_ompt_thread_info_t ));
}

static inline mpcomp_ompt_thread_info_t* mpcomp_ompt_thread_info_alloc( void )
{
	mpcomp_ompt_thread_info_t* info = NULL;
	info = ( mpcomp_ompt_thread_info_t*) sctk_malloc( sizeof( mpcomp_ompt_thread_info_t ));
	sctk_assert( info );
	mpcomp_ompt_thread_info_reset( info );
	return info;
}

#endif /* __MPCOMP_OMPT_INTERNAL_H__ */
