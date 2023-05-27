/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:34:06 CEST 2021                                        # */
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
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Patrick Carribault <patrick.carribault@cea.fr>                     # */
/* # - Ricardo Bispo vieira <ricardo.bispo-vieira@exascale-computing.eu>  # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Stephane Bouhrour <stephane.bouhrour@exascale-computing.eu>        # */
/* # - Thomas Dionisi <thomas.dionisi@exascale-computing.eu>              # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef __MPC_OMP_TASK_H__
# define __MPC_OMP_TASK_H__

# include "mpcomp_types.h"
# include "mpc_common_asm.h"
# include "mpc_omp_task_property.h"
# include "omp_gomp_constants.h"

/**********
 * MACROS *
 **********/

#define MPC_OMP_OVERFLOW_SANITY_CHECK( size, size_to_sum ) \
    ( size <= KMP_SIZE_T_MAX - size_to_sum )

#define MPC_OMP_TASK_COMPUTE_TASK_DATA_PTR( ptr, size, arg_align ) \
    ptr + MPC_OMP_TASK_ALIGN_SINGLE( size, arg_align )

/*** THREAD ACCESSORS MACROS ***/
#define MPC_OMP_TASK_THREAD_GET_LARCENY_ORDER( thread ) \
    thread->task_infos.larceny_order

#define MPC_OMP_TASK_THREAD_SET_LARCENY_ORDER( thread, ptr ) \
    do                                                      \
    {                                                       \
        thread->task_infos.larceny_order = ptr;             \
    } while ( 0 )

#define MPC_OMP_TASK_THREAD_GET_CURRENT_TASK( thread ) \
    thread->task_infos.current_task

#define MPC_OMP_TASK_THREAD_SET_CURRENT_TASK( thread, ptr ) \
    do                                                     \
    {                                                      \
        thread->task_infos.current_task = ptr;             \
    } while ( 0 )

/*** INSTANCE ACCESSORS MACROS ***/

#define MPC_OMP_TASK_INSTANCE_SET_ARRAY_TREE_TOTAL_SIZE( instance, num ) \
    do                                                                  \
    {                                                                   \
        instance->task_infos.array_tree_total_size = num;               \
    } while ( 0 )

#define MPC_OMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE( instance ) \
    instance->task_infos.array_tree_total_size

#define MPC_OMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_SIZE( instance, ptr ) \
    do                                                                  \
    {                                                                   \
        instance->task_infos.array_tree_level_size = ptr;               \
    } while ( 0 );

#define MPC_OMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE( instance, depth ) \
    instance->task_infos.array_tree_level_size[depth]

#define MPC_OMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_FIRST( instance, ptr ) \
    do                                                                   \
    {                                                                    \
        instance->task_infos.array_tree_level_first = ptr;               \
    } while ( 0 );

#define MPC_OMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_FIRST( instance, depth ) \
    instance->task_infos.array_tree_level_first[depth]

/*** TEAM ACCESSORS MACROS ***/

/** Check if team is initialized */
#define MPC_OMP_TASK_TEAM_IS_INITIALIZED( team ) \
    MPC_OMP_TASK_STATUS_IS_INITIALIZED( team->task_infos.status )

/** Avoid multiple team init (one thread per team ) */
#define MPC_OMP_TASK_TEAM_TRY_INIT( team ) \
    MPC_OMP_TASK_STATUS_TRY_INIT( team->task_infos.status )

/** Set team status to INITIALIZED */
#define MPC_OMP_TASK_TEAM_CMPL_INIT( team ) \
    MPC_OMP_TASK_STATUS_CMPL_INIT( team->task_infos.status )

#define MPC_OMP_TASK_TEAM_GET_PQUEUE_DEPTH( team, type ) \
    team->task_infos.pqueue_depth[type]

#define MPC_OMP_TASK_TEAM_SET_PQUEUE_DEPTH( team, type, val ) \
    do                                                         \
    {                                                          \
        team->task_infos.pqueue_depth[type] = val;           \
    } while ( 0 )

#define MPC_OMP_TASK_TEAM_GET_TASK_LARCENY_MODE( team ) \
    team->task_infos.task_larceny_mode

#define MPC_OMP_TASK_TEAM_SET_TASK_LARCENY_MODE( team, mode ) \
    do                                                       \
    {                                                        \
        team->task_infos.task_larceny_mode = mode;           \
    } while ( 0 )

#define MPC_OMP_TASK_TEAM_GET_TASK_NESTING_MAX( team ) \
    team->task_infos.task_nesting_max

#define MPC_OMP_TASK_TEAM_SET_TASK_NESTING_MAX( team, val ) \
    do                                                     \
    {                                                      \
        team->task_infos.task_nesting_max = val;           \
    } while ( 0 )

#define MPC_OMP_TASK_TEAM_SET_USE_TASK( team )                \
    do                                                       \
    {                                                        \
        OPA_cas_int( &( team->task_infos.use_task ), 0, 1 ); \
    } while ( 0 )

#define MPC_OMP_TASK_TEAM_GET_USE_TASK( team ) \
    OPA_load_int( &( team->task_infos.use_task ) )

#define MPC_OMP_TASK_TEAM_RESET_USE_TASK( team )             \
    do                                                      \
    {                                                       \
        OPA_store_int( &( team->task_infos.use_task ), 0 ); \
    } while ( 0 )

/*** NODE ACCESSORS MACROS ***/

#define MPC_OMP_TASK_NODE_GET_TREE_ARRAY_RANK( node ) \
    node->task_infos.tree_array_rank;
#define MPC_OMP_TASK_NODE_SET_TREE_ARRAY_RANK( node, id ) \
    do                                                   \
    {                                                    \
        node->task_infos.tree_array_rank = id;           \
    } while ( 0 )

#define MPC_OMP_TASK_NODE_GET_TREE_ARRAY_NODES( node ) \
    node->task_infos.tree_array_node
#define MPC_OMP_TASK_NODE_SET_TREE_ARRAY_NODES( node, ptr ) \
    do                                                     \
    {                                                      \
        node->task_infos.tree_array_node = ptr;            \
    } while ( 0 )

#define MPC_OMP_TASK_NODE_GET_TREE_ARRAY_NODE( node, rank ) \
    node->task_infos.tree_array_node[rank]

#define MPC_OMP_TASK_NODE_GET_PATH( node ) node->task_infos.path
#define MPC_OMP_TASK_NODE_SET_PATH( node, ptr ) \
    do                                         \
    {                                          \
        node->task_infos.path = ptr;           \
    } while ( 0 );

#define MPC_OMP_TASK_NODE_GET_TASK_PQUEUE_HEAD( node, type ) \
    node->task_infos.pqueue[type]
#define MPC_OMP_TASK_NODE_SET_TASK_PQUEUE_HEAD( node, type, ptr ) \
    do                                                         \
    {                                                          \
        node->task_infos.pqueue[type] = ptr;                 \
    } while ( 0 )

#define MPC_OMP_TASK_NODE_GET_TASK_PQUEUE_RANDBUFFER( node ) \
    node->task_infos.pqueue_randBuffer
#define MPC_OMP_TASK_NODE_SET_TASK_PQUEUE_RANDBUFFER( node, ptr ) \
    do                                                         \
    {                                                          \
        node->task_infos.pqueue_randBuffer = ptr;            \
    } while ( 0 )

/*** MVP ACCESSORS MACROS ***/

#define MPC_OMP_TASK_MVP_GET_TREE_ARRAY_RANK( mvp ) mvp->task_infos.tree_array_rank
#define MPC_OMP_TASK_MVP_SET_TREE_ARRAY_RANK( mvp, id ) \
    do                                                 \
    {                                                  \
        mvp->task_infos.tree_array_rank = id;          \
    } while ( 0 )

#define MPC_OMP_TASK_MVP_GET_TREE_ARRAY_NODES( mvp ) \
    mvp->task_infos.tree_array_node
#define MPC_OMP_TASK_MVP_SET_TREE_ARRAY_NODES( mvp, ptr ) \
    do                                                   \
    {                                                    \
        mvp->task_infos.tree_array_node = ptr;           \
    } while ( 0 )

#define MPC_OMP_TASK_MVP_GET_TREE_ARRAY_NODE( mvp, rank ) \
    mvp->task_infos.tree_array_node[rank]

#define MPC_OMP_TASK_MVP_GET_PATH( mvp ) mvp->path

#define MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_HEAD( mvp, type ) \
    mvp->task_infos.pqueue[type]
#define MPC_OMP_TASK_MVP_SET_TASK_PQUEUE_HEAD( mvp, type, ptr ) \
    do                                                       \
    {                                                        \
        mvp->task_infos.pqueue[type] = ptr;                \
    } while ( 0 )

#define MPC_OMP_TASK_MVP_GET_LAST_STOLEN_TASK_PQUEUE( mvp, type ) \
    mvp->task_infos.lastStolen_pqueue[type]
#define MPC_OMP_TASK_MVP_SET_LAST_STOLEN_TASK_PQUEUE( mvp, type, ptr ) \
    do                                                              \
    {                                                               \
        mvp->task_infos.lastStolen_pqueue[type] = ptr;            \
    } while ( 0 )

#define MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_RANK_BUFFER( mvp, rank ) \
    MPC_OMP_TASK_NODE_GET_TASK_PQUEUE_RANDBUFFER(                 \
            MPC_OMP_TASK_MVP_GET_TREE_ARRAY_NODE( mvp, rank ) )

#define MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_RANDBUFFER( mvp ) \
    mvp->task_infos.pqueue_randBuffer
#define MPC_OMP_TASK_MVP_SET_TASK_PQUEUE_RANDBUFFER( mvp, ptr ) \
    do                                                       \
    {                                                        \
        mvp->task_infos.pqueue_randBuffer = ptr;           \
    } while ( 0 );

#define MPC_OMP_TASK_MVP_GET_TASK_PQUEUE_NODE_RANK(mvp, type ) \
    mvp->task_infos.taskPqueueNodeRank[type]
#define MPC_OMP_TASK_MVP_SET_TASK_PQUEUE_NODE_RANK(mvp, type, id ) \
    do                                                           \
    {                                                            \
        mvp->task_infos.taskPqueueNodeRank[type] = id;             \
    } while ( 0 )

/*********
 * UTILS *
 *********/

/*** Task property primitives ***/

int mpc_omp_task_parse_larceny_mode(char * mode);

/******************
 * TASK INTERFACE *
 ******************/

static inline long
_mpc_omp_task_align_single_malloc( long size, long arg_align )
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

# define TASK_STATE_TRANSITION(TASK, TO) OPA_store_int(&(TASK->state), TO)
# define TASK_STATE_TRANSITION_ATOMIC(TASK, FROM, TO) (OPA_cas_int(&(TASK->state), FROM, TO) == FROM)
# define TASK_STATE(task) OPA_load_int(&(task->state))

int _mpc_omp_task_process(mpc_omp_task_t * task);

void _mpc_omp_task_initgroup_start( void );
void _mpc_omp_task_initgroup_end( void );

void _mpc_omp_task_yield( void );

void _mpc_omp_task_mvp_info_init( struct mpc_omp_node_s *parent, struct mpc_omp_mvp_s *child );

void _mpc_omp_task_node_info_init( struct mpc_omp_node_s *parent, struct mpc_omp_node_s *child );

void _mpc_omp_task_team_info_init( struct mpc_omp_team_s *team, int depth );

void _mpc_omp_task_root_info_init( struct mpc_omp_node_s *root );

int _mpc_omp_task_schedule(void);

void _mpc_omp_task_wait(void ** depend, int nowait);

/*******************
 * TREE ARRAY TASK *
 *******************/

void _mpc_omp_task_tree_init(struct mpc_omp_thread_s * thread);
void _mpc_omp_task_tree_deinit(struct mpc_omp_thread_s * thread);


/**
 * Allocate a task
 */
mpc_omp_task_t * _mpc_omp_task_allocate(size_t size);

/*
 * Create a new openmp task
 *
 * This function can be called when encountering an 'omp task' construct
 *
 * \param task the task buffer to use (may be NULL, and so, a new task is allocated)
 * \param fn the task entry point
 * \param data the task data (fn parameters)
 * \param size the total size of the task in bytes
 * \param properties
 * \param depend
 * \param priority_hint
 */
mpc_omp_task_t * _mpc_omp_task_init(
    mpc_omp_task_t * task,
    void (*fn)(void *), void *data,
    size_t size,
    mpc_omp_task_property_t properties);

/**
 * Between `_mpc_omp_task_init` and `_mpc_omp_task_deinit` - the task can be accessed safely
 * Calling `_mpc_omp_task_deinit` ensure to the runtime
 * that the task will no longer be accessed, and thatit can release it anytime
 */
void _mpc_omp_task_deinit(mpc_omp_task_t * task);

/**
 * set task dependencies
 *  - task is the task
 *  - depend is the dependency array (GOMP format)
 *  - priority_hint is the user task priority hint
 */
void _mpc_omp_task_deps(mpc_omp_task_t * task, void ** depend, int priority_hint);

/*************
 * TASKGROUP *
 *************/

/* Functions prototypes */
void _mpc_omp_task_taskgroup_start(void);
void _mpc_omp_task_taskgroup_end(void);
void _mpc_omp_task_taskgroup_cancel(void);

void _mpc_omp_taskgroup_add_task(mpc_omp_task_t * new_task);
void mpc_omp_taskgroup_del_task(mpc_omp_task_t * task);

/* tasks */
void mpc_omp_task_process(mpc_omp_task_t * task);
void _mpc_omp_task_finalize(mpc_omp_task_t * task);

mpc_omp_task_t * __mpc_omp_task_init(
    mpc_omp_task_t * task,
    void (*fn)(void *),
    void *data,
    void (*cpyfn)(void *, void *),
    long arg_size,
    long arg_align,
    mpc_omp_task_property_t properties,
    void ** depend,
    int priority_hint);


/************
 * TASKLOOP *
 ************/

unsigned long
_mpc_task_loop_compute_loop_value_grainsize(
        long iteration_num, unsigned long num_tasks,
        long step, long *taskstep,
        unsigned long *extra_chunk);

unsigned long
_mpc_task_loop_compute_loop_value(
        long iteration_num, unsigned long num_tasks,
        long step, long *taskstep,
        unsigned long *extra_chunk);


unsigned long
_mpc_omp_task_loop_compute_num_iters(long start, long end, long step);

/* TODO : refractor this prototype, move them to another more appropriate file */
# define MPC_OMP_CLAUSE_USE_FIBER   (1 << 0)
# define MPC_OMP_CLAUSE_UNTIED      (1 << 1)

void _mpc_omp_callback_run(mpc_omp_callback_scope_t scope, mpc_omp_callback_when_t when);
void _mpc_omp_task_unblock(mpc_omp_event_handle_block_t * handle);
void _mpc_omp_event_handle_ref(mpc_omp_event_handle_t * handle);
void _mpc_omp_event_handle_unref(mpc_omp_event_handle_t * handle);
void _mpc_omp_task_profile_register_current(int priority);

/** Persistent tasks functions
 *
 * Multiple points to modify :
 *  - on task creation
 *      - only recopy private variables and reset task flags
 *      - keep 1 more reference
 *  - on task completion
 *      - don
 *  - on persistent region exit
 *      - taskwait and delete every tasks
 */
mpc_omp_persistent_region_t * mpc_omp_get_persistent_region(void);
mpc_omp_task_t * mpc_omp_get_persistent_task(void);
void _mpc_omp_task_reinit_persistent(mpc_omp_task_t * task);
void mpc_omp_persistent_region_push(mpc_omp_task_t * task);

#endif /* __MPC_OMP_TASK_H__ */
