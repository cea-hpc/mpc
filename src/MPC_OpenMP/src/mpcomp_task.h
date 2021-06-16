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
#include "mpcomp_types.h"

#if ( !defined( __SCTK_MPCOMP_TASK_H__ ) && MPCOMP_TASK )
#define __SCTK_MPCOMP_TASK_H__

#include "mpc_common_asm.h"

/**********
 * MACROS *
 **********/

#define MPCOMP_TASK_DEP_UNUSED_VAR( var ) (void) ( sizeof( var ) )

#define MPCOMP_OVERFLOW_SANITY_CHECK( size, size_to_sum ) \
	( size <= KMP_SIZE_T_MAX - size_to_sum )

#define MPCOMP_TASK_COMPUTE_TASK_DATA_PTR( ptr, size, arg_align ) \
	ptr + MPCOMP_TASK_ALIGN_SINGLE( size, arg_align )

#define MPCOMP_TASK_STATUS_IS_INITIALIZED( status ) \
	( OPA_load_int( &( status ) ) == MPCOMP_TASK_INIT_STATUS_INITIALIZED )

#define MPCOMP_TASK_STATUS_TRY_INIT( status )                          \
	( OPA_cas_int( &( status ), MPCOMP_TASK_INIT_STATUS_UNINITIALIZED, \
	               MPCOMP_TASK_INIT_STATUS_INIT_IN_PROCESS ) ==        \
	  MPCOMP_TASK_INIT_STATUS_UNINITIALIZED )

#define MPCOMP_TASK_STATUS_CMPL_INIT( status ) \
	( OPA_store_int( &( status ), MPCOMP_TASK_INIT_STATUS_INITIALIZED ) )

/*** THREAD ACCESSORS MACROS ***/

/** Check if thread is initialized */
#define MPCOMP_TASK_THREAD_IS_INITIALIZED( thread ) \
	MPCOMP_TASK_STATUS_IS_INITIALIZED( thread->task_infos.status )

/** Avoid multiple team init (one thread per team ) */
#define MPCOMP_TASK_THREAD_TRY_INIT( thread ) \
	MPCOMP_TASK_STATUS_TRY_INIT( thread->task_infos.status )

/** Set thread status to INITIALIZED */
#define MPCOMP_TASK_THREAD_CMPL_INIT( thread ) \
	MPCOMP_TASK_STATUS_CMPL_INIT( thread->task_infos.status )

#define MPCOMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) \
	thread->task_infos.larceny_order

#define MPCOMP_TASK_THREAD_SET_LARCENY_ORDER( thread, ptr ) \
	do                                                      \
	{                                                       \
		thread->task_infos.larceny_order = ptr;             \
	} while ( 0 )

#define MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread ) \
	thread->task_infos.current_task

#define MPCOMP_TASK_THREAD_SET_CURRENT_TASK( thread, ptr ) \
	do                                                     \
	{                                                      \
		thread->task_infos.current_task = ptr;             \
	} while ( 0 )

#define MPCOMP_TASK_THREAD_GET_TIED_TASK_LIST_HEAD( thread ) \
	thread->task_infos.tied_tasks

#define MPCOMP_TASK_THREAD_SET_TIED_TASK_LIST_HEAD( thread, ptr ) \
	do                                                            \
	{                                                             \
		thread->task_infos.tied_tasks = ptr;                      \
	} while ( 0 )

/*** INSTANCE ACCESSORS MACROS ***/

#define MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_TOTAL_SIZE( instance, num ) \
	do                                                                  \
	{                                                                   \
		instance->task_infos.array_tree_total_size = num;               \
	} while ( 0 )

#define MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE( instance ) \
	instance->task_infos.array_tree_total_size

#define MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_SIZE( instance, ptr ) \
	do                                                                  \
	{                                                                   \
		instance->task_infos.array_tree_level_size = ptr;               \
	} while ( 0 );

#define MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( instance, depth ) \
	instance->task_infos.array_tree_level_size[depth]

#define MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_FIRST( instance, ptr ) \
	do                                                                   \
	{                                                                    \
		instance->task_infos.array_tree_level_first = ptr;               \
	} while ( 0 );

#define MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_FIRST( instance, depth ) \
	instance->task_infos.array_tree_level_first[depth]

/*** TEAM ACCESSORS MACROS ***/

/** Check if team is initialized */
#define MPCOMP_TASK_TEAM_IS_INITIALIZED( team ) \
	MPCOMP_TASK_STATUS_IS_INITIALIZED( team->task_infos.status )

/** Avoid multiple team init (one thread per team ) */
#define MPCOMP_TASK_TEAM_TRY_INIT( team ) \
	MPCOMP_TASK_STATUS_TRY_INIT( team->task_infos.status )

/** Set team status to INITIALIZED */
#define MPCOMP_TASK_TEAM_CMPL_INIT( team ) \
	MPCOMP_TASK_STATUS_CMPL_INIT( team->task_infos.status )

#define MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( team, type ) \
	team->task_infos.tasklist_depth[type]

#define MPCOMP_TASK_TEAM_SET_TASKLIST_DEPTH( team, type, val ) \
	do                                                         \
	{                                                          \
		team->task_infos.tasklist_depth[type] = val;           \
	} while ( 0 )

#define MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( team ) \
	team->task_infos.task_larceny_mode

#define MPCOMP_TASK_TEAM_SET_TASK_LARCENY_MODE( team, mode ) \
	do                                                       \
	{                                                        \
		team->task_infos.task_larceny_mode = mode;           \
	} while ( 0 )

#define MPCOMP_TASK_TEAM_GET_TASK_NESTING_MAX( team ) \
	team->task_infos.task_nesting_max

#define MPCOMP_TASK_TEAM_SET_TASK_NESTING_MAX( team, val ) \
	do                                                     \
	{                                                      \
		team->task_infos.task_nesting_max = val;           \
	} while ( 0 )

#define MPCOMP_TASK_TEAM_SET_USE_TASK( team )                \
	do                                                       \
	{                                                        \
		OPA_cas_int( &( team->task_infos.use_task ), 0, 1 ); \
	} while ( 0 )

#define MPCOMP_TASK_TEAM_GET_USE_TASK( team ) \
	OPA_load_int( &( team->task_infos.use_task ) )

#define MPCOMP_TASK_TEAM_RESET_USE_TASK( team )             \
	do                                                      \
	{                                                       \
		OPA_store_int( &( team->task_infos.use_task ), 0 ); \
	} while ( 0 )

/*** NODE ACCESSORS MACROS ***/

#define MPCOMP_TASK_NODE_GET_TREE_ARRAY_RANK( node ) \
	node->task_infos.tree_array_rank;
#define MPCOMP_TASK_NODE_SET_TREE_ARRAY_RANK( node, id ) \
	do                                                   \
	{                                                    \
		node->task_infos.tree_array_rank = id;           \
	} while ( 0 )

#define MPCOMP_TASK_NODE_GET_TREE_ARRAY_NODES( node ) \
	node->task_infos.tree_array_node
#define MPCOMP_TASK_NODE_SET_TREE_ARRAY_NODES( node, ptr ) \
	do                                                     \
	{                                                      \
		node->task_infos.tree_array_node = ptr;            \
	} while ( 0 )

#define MPCOMP_TASK_NODE_GET_TREE_ARRAY_NODE( node, rank ) \
	node->task_infos.tree_array_node[rank]

#define MPCOMP_TASK_NODE_GET_PATH( node ) node->task_infos.path
#define MPCOMP_TASK_NODE_SET_PATH( node, ptr ) \
	do                                         \
	{                                          \
		node->task_infos.path = ptr;           \
	} while ( 0 );

#define MPCOMP_TASK_NODE_GET_TASK_LIST_HEAD( node, type ) \
	node->task_infos.tasklist[type]
#define MPCOMP_TASK_NODE_SET_TASK_LIST_HEAD( node, type, ptr ) \
	do                                                         \
	{                                                          \
		node->task_infos.tasklist[type] = ptr;                 \
	} while ( 0 )

#define MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER( node ) \
	node->task_infos.tasklist_randBuffer
#define MPCOMP_TASK_NODE_SET_TASK_LIST_RANDBUFFER( node, ptr ) \
	do                                                         \
	{                                                          \
		node->task_infos.tasklist_randBuffer = ptr;            \
	} while ( 0 )

/*** MVP ACCESSORS MACROS ***/

#define MPCOMP_TASK_MVP_GET_TREE_ARRAY_RANK( mvp ) mvp->task_infos.tree_array_rank
#define MPCOMP_TASK_MVP_SET_TREE_ARRAY_RANK( mvp, id ) \
	do                                                 \
	{                                                  \
		mvp->task_infos.tree_array_rank = id;          \
	} while ( 0 )

#define MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODES( mvp ) \
	mvp->task_infos.tree_array_node
#define MPCOMP_TASK_MVP_SET_TREE_ARRAY_NODES( mvp, ptr ) \
	do                                                   \
	{                                                    \
		mvp->task_infos.tree_array_node = ptr;           \
	} while ( 0 )

#define MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE( mvp, rank ) \
	mvp->task_infos.tree_array_node[rank]

#define MPCOMP_TASK_MVP_GET_PATH( mvp ) mvp->path

#define MPCOMP_TASK_MVP_GET_TASK_LIST_HEAD( mvp, type ) \
	mvp->task_infos.tasklist[type]
#define MPCOMP_TASK_MVP_SET_TASK_LIST_HEAD( mvp, type, ptr ) \
	do                                                       \
	{                                                        \
		mvp->task_infos.tasklist[type] = ptr;                \
	} while ( 0 )

#define MPCOMP_TASK_MVP_GET_LAST_STOLEN_TASK_LIST( mvp, type ) \
	mvp->task_infos.lastStolen_tasklist[type]
#define MPCOMP_TASK_MVP_SET_LAST_STOLEN_TASK_LIST( mvp, type, ptr ) \
	do                                                              \
	{                                                               \
		mvp->task_infos.lastStolen_tasklist[type] = ptr;            \
	} while ( 0 )

#define MPCOMP_TASK_MVP_GET_TASK_LIST_RANK_BUFFER( mvp, rank ) \
	MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER(                 \
	        MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE( mvp, rank ) )

#define MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER( mvp ) \
	mvp->task_infos.tasklist_randBuffer
#define MPCOMP_TASK_MVP_SET_TASK_LIST_RANDBUFFER( mvp, ptr ) \
	do                                                       \
	{                                                        \
		mvp->task_infos.tasklist_randBuffer = ptr;           \
	} while ( 0 );

#define MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK( mvp, type ) \
	mvp->task_infos.tasklistNodeRank[type]
#define MPCOMP_TASK_MVP_SET_TASK_LIST_NODE_RANK( mvp, type, id ) \
	do                                                           \
	{                                                            \
		mvp->task_infos.tasklistNodeRank[type] = id;             \
	} while ( 0 )

/******************
 * GOMP CONSTANTS *
 ******************/

#define GOMP_TASK_FLAG_FINAL ( 1 << 1 )
#define GOMP_TASK_FLAG_MERGEABLE ( 1 << 2 )
#define GOMP_TASK_FLAG_DEPEND ( 1 << 3 )
#define GOMP_TASK_FLAG_PRIORITY ( 1 << 4 )
#define GOMP_TASK_FLAG_UP ( 1 << 8 )
#define GOMP_TASK_FLAG_GRAINSIZE ( 1 << 9 )
#define GOMP_TASK_FLAG_IF ( 1 << 10 )
#define GOMP_TASK_FLAG_NOGROUP ( 1 << 11 )

#define MPCOMP_TASK_FLAG_FINAL GOMP_TASK_FLAG_FINAL
#define MPCOMP_TASK_FLAG_MERGEABLE GOMP_TASK_FLAG_MERGEABLE
#define MPCOMP_TASK_FLAG_DEPEND GOMP_TASK_FLAG_DEPEND
#define MPCOMP_TASK_FLAG_PRIORITY GOMP_TASK_FLAG_PRIORITY
#define MPCOMP_TASK_FLAG_UP GOMP_TASK_FLAG_UP
#define MPCOMP_TASK_FLAG_GRAINSIZE GOMP_TASK_FLAG_GRAINSIZE
#define MPCOMP_TASK_FLAG_IF GOMP_TASK_FLAG_IF
#define MPCOMP_TASK_FLAG_NOGROUP GOMP_TASK_FLAG_NOGROUP

/*********
 * UTILS *
 *********/

/*** Task property primitives ***/

int mpcomp_task_parse_larceny_mode(char * mode);

static inline void
mpcomp_task_reset_property( mpcomp_task_property_t *property )
{
	*property = 0;
}

static inline void mpcomp_task_set_property( mpcomp_task_property_t *property,
        mpcomp_task_property_t mask )
{
	*property |= mask;
}

static inline void mpcomp_task_unset_property( mpcomp_task_property_t *property,
        mpcomp_task_property_t mask )
{
	*property &= ~( mask );
}

static inline int mpcomp_task_property_isset( mpcomp_task_property_t property,
        mpcomp_task_property_t mask )
{
	return ( property & mask );
}

static inline int _mpc_task_is_final( unsigned int flags,
                                        mpcomp_task_t *parent )
{
	if ( flags & 2 )
	{
		return 1;
	}

	if ( parent &&
	     mpcomp_task_property_isset( parent->property, MPCOMP_TASK_FINAL ) )
	{
		return 1;
	}

	return 0;
}

/* Initialization of a task structure */
static inline void _mpc_task_info_init( mpcomp_task_t *task,
        void ( *func )( void * ), void *func_data,
        void* data, size_t data_size,
        struct mpcomp_thread_s *thread )
{
	assert( task != NULL );
	assert( thread != NULL );
	/* Reset all task infos to NULL */
	memset( task, 0, sizeof( mpcomp_task_t ) );
	/* Set non null task infos field */
	task->func = func;
	task->func_data = func_data;
	task->data = data;
	task->data_size = data_size;
	task->icvs = thread->info.icvs;
	task->parent = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( thread );
	task->depth = ( task->parent ) ? task->parent->depth + 1 : 0;
#if MPC_USE_SISTER_LIST
	task->children_lock = SCTK_SPINLOCK_INITIALIZER;
#endif /* MPC_USE_SISTER_LIST */
}

/******************
 * TASK INTERFACE *
 ******************/

static inline long _mpc_task_align_single_malloc( long size, long arg_align )
{
	if ( size & ( arg_align - 1 ) )
	{
		const size_t tmp_size = size & ( ~( arg_align - 1 ) );

		if ( tmp_size <= KMP_SIZE_T_MAX - arg_align )
		{
			return tmp_size + arg_align;
		}
	}

	return size;
}

void _mpc_task_ref_parent_task( mpcomp_task_t *task );

void _mpc_task_unref_parent_task( mpcomp_task_t *task );

void _mpc_task_free( mpcomp_thread_t *thread );

/**
 * Allocate a new task with provided information.
 *
 * The new task may be delayed, so copy arguments in a buffer
 *
 * \param fn Function pointer containing the task body
 * \param data Argument to pass to the previous function pointer
 * \param cpyfn
 * \param arg_size
 * \param arg_align
 * \param if_clause
 * \param flags
 * \param deps_num
 *
 * \return New allocated task (or NULL if an error occured)
 */
mpcomp_task_t *_mpc_task_alloc( void ( *fn )( void * ), void *data,
                                    void ( *cpyfn )( void *, void * ),
                                    long arg_size, long arg_align,
                                    bool if_clause, unsigned flags,
                                    int has_deps );

void _mpc_task_process( mpcomp_task_t *new_task, bool if_clause );

/*
 * Creation of an OpenMP task.
 *
 * This function can be called when encountering an 'omp task' construct
 *
 * \param fn
 * \param data
 * \param cpyfn
 * \param arg_size
 * \param arg_align
 * \param if_clause
 * \param flags
 */
void _mpc_task_new( void ( *fn )( void * ), void *data,
                    void ( *cpyfn )( void *, void * ), long arg_size, long arg_align,
                    bool if_clause, unsigned flags );

void _mpc_task_newgroup_start( void );
void _mpc_task_newgroup_end( void );

void _mpc_taskyield( void );

void _mpc_task_list_interface_init();

void _mpc_task_mvp_info_init( struct mpcomp_node_s *parent, struct mpcomp_mvp_s *child );

void _mpc_task_node_info_init( struct mpcomp_node_s *parent, struct mpcomp_node_s *child );

void _mpc_task_team_info_init( struct mpcomp_team_s *team, int depth );

void _mpc_task_root_info_init( struct mpcomp_node_s *root );

void _mpc_task_schedule( int need_taskwait );

void _mpc_task_wait( void );

/*******************
 * TREE ARRAY TASK *
 *******************/

void _mpc_task_tree_array_thread_init( struct mpcomp_thread_s *thread );

/*********************
 * TASK DEPENDENCIES *
 *********************/

void _mpc_task_new_with_deps( void ( *fn )( void * ), void *data,
                            void ( *cpyfn )( void *, void * ), long arg_size,
                            long arg_align, bool if_clause, unsigned flags,
                            void **depend, bool intel_alloc, mpcomp_task_t *intel_task );

void _mpc_task_dep_new_finalize( mpcomp_task_t *task );

/*************
 * TASKGROUP *
 *************/

/* Functions prototypes */
void _mpc_task_taskgroup_start( void );
void _mpc_task_taskgroup_end( void );

#ifdef MPCOMP_TASKGROUP
/* Inline functions */
static inline void mpcomp_taskgroup_add_task( mpcomp_task_t *new_task )
{
	mpcomp_task_t *current_task;
	mpcomp_thread_t *omp_thread_tls;
	mpcomp_task_taskgroup_t *taskgroup;
	assert( sctk_openmp_thread_tls );
	omp_thread_tls = ( mpcomp_thread_t * ) sctk_openmp_thread_tls;
	current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK( omp_thread_tls );
	taskgroup = current_task->taskgroup;

	if ( taskgroup )
	{
		new_task->taskgroup = taskgroup;
		OPA_incr_int( &( taskgroup->children_num ) );
	}
}

static inline void mpcomp_taskgroup_del_task( mpcomp_task_t *task )
{
	if ( task->taskgroup )
	{
		OPA_decr_int( &( task->taskgroup->children_num ) );
	}
}

#else /* MPCOMP_TASKGROUP */

static inline void mpcomp_taskgroup_add_task( mpcomp_task_t *new_task )
{
}

static inline void mpcomp_taskgroup_del_task( mpcomp_task_t *task ) {}

#endif /* MPCOMP_TASKGROUP */

#endif /* __SCTK_MPCOMP_TASK_H__ */
