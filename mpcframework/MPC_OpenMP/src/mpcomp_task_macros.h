#include "mpcomp_types.h"

#if (!defined(__MPCOMP_TASK_MACROS_H__) && MPCOMP_TASK)
#define __MPCOMP_TASK_MACROS_H__

#include "mpc_common_asm.h"




#define MPCOMP_TASK_DEP_UNUSED_VAR(var) (void)(sizeof(var))

#define MPCOMP_OVERFLOW_SANITY_CHECK(size, size_to_sum)                        \
  (size <= KMP_SIZE_T_MAX - size_to_sum)

#define MPCOMP_TASK_COMPUTE_TASK_DATA_PTR(ptr, size, arg_align)                \
  ptr + MPCOMP_TASK_ALIGN_SINGLE(size, arg_align)

/*** MPCOMP_TASK_INIT_STATUS ***/

typedef enum mpcomp_task_init_status_e {
  MPCOMP_TASK_INIT_STATUS_UNINITIALIZED,
  MPCOMP_TASK_INIT_STATUS_INIT_IN_PROCESS,
  MPCOMP_TASK_INIT_STATUS_INITIALIZED,
  MPCOMP_TASK_INIT_STATUS_COUNT
} mpcomp_task_init_status_t;

#define MPCOMP_TASK_STATUS_IS_INITIALIZED(status)                              \
  (OPA_load_int(&(status)) == MPCOMP_TASK_INIT_STATUS_INITIALIZED)

#define MPCOMP_TASK_STATUS_TRY_INIT(status)                                    \
  (OPA_cas_int(&(status), MPCOMP_TASK_INIT_STATUS_UNINITIALIZED,      \
                        MPCOMP_TASK_INIT_STATUS_INIT_IN_PROCESS) ==            \
   MPCOMP_TASK_INIT_STATUS_UNINITIALIZED)

#define MPCOMP_TASK_STATUS_CMPL_INIT(status)                                   \
  (OPA_store_int(&(status), MPCOMP_TASK_INIT_STATUS_INITIALIZED))


/*** THREAD ACCESSORS MACROS ***/

/** Check if thread is initialized */
#define MPCOMP_TASK_THREAD_IS_INITIALIZED(thread)                              \
  MPCOMP_TASK_STATUS_IS_INITIALIZED(thread->task_infos.status)

/** Avoid multiple team init (one thread per team ) */
#define MPCOMP_TASK_THREAD_TRY_INIT(thread)                                    \
  MPCOMP_TASK_STATUS_TRY_INIT(thread->task_infos.status)

/** Set thread status to INITIALIZED */
#define MPCOMP_TASK_THREAD_CMPL_INIT(thread)                                   \
  MPCOMP_TASK_STATUS_CMPL_INIT(thread->task_infos.status)

#define MPCOMP_TASK_THREAD_GET_LARCENY_ORDER(thread)                           \
  thread->task_infos.larceny_order

#define MPCOMP_TASK_THREAD_SET_LARCENY_ORDER(thread, ptr)                      \
  do {                                                                         \
    thread->task_infos.larceny_order = ptr;                                    \
  } while (0)

#define MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread)                            \
  thread->task_infos.current_task

#define MPCOMP_TASK_THREAD_SET_CURRENT_TASK(thread, ptr)                       \
  do {                                                                         \
    thread->task_infos.current_task = ptr;                                     \
  } while (0)

#define MPCOMP_TASK_THREAD_GET_TIED_TASK_LIST_HEAD(thread)                     \
  thread->task_infos.tied_tasks

#define MPCOMP_TASK_THREAD_SET_TIED_TASK_LIST_HEAD(thread, ptr)                \
  do {                                                                         \
    thread->task_infos.tied_tasks = ptr;                                       \
  } while (0)

/*** INSTANCE ACCESSORS MACROS ***/

#define MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_TOTAL_SIZE(instance, num)          \
  do {                                                                         \
    instance->task_infos.array_tree_total_size = num;                          \
  } while (0)

#define MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(instance)               \
  instance->task_infos.array_tree_total_size

#define MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_SIZE(instance, ptr)          \
  do {                                                                         \
    instance->task_infos.array_tree_level_size = ptr;                          \
  } while (0);

#define MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE(instance, depth)        \
  instance->task_infos.array_tree_level_size[depth]

#define MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_FIRST(instance, ptr)         \
  do {                                                                         \
    instance->task_infos.array_tree_level_first = ptr;                         \
  } while (0);

#define MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_FIRST(instance, depth)       \
  instance->task_infos.array_tree_level_first[depth]

/*** TEAM ACCESSORS MACROS ***/

/** Check if team is initialized */
#define MPCOMP_TASK_TEAM_IS_INITIALIZED(team)                                  \
  MPCOMP_TASK_STATUS_IS_INITIALIZED(team->task_infos.status)

/** Avoid multiple team init (one thread per team ) */
#define MPCOMP_TASK_TEAM_TRY_INIT(team)                                        \
  MPCOMP_TASK_STATUS_TRY_INIT(team->task_infos.status)

/** Set team status to INITIALIZED */
#define MPCOMP_TASK_TEAM_CMPL_INIT(team)                                       \
  MPCOMP_TASK_STATUS_CMPL_INIT(team->task_infos.status)

#define MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(team, type)                        \
  team->task_infos.tasklist_depth[type]

#define MPCOMP_TASK_TEAM_SET_TASKLIST_DEPTH(team, type, val)                   \
  do {                                                                         \
    team->task_infos.tasklist_depth[type] = val;                               \
  } while (0)

#define MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE(team)                           \
  team->task_infos.task_larceny_mode

#define MPCOMP_TASK_TEAM_SET_TASK_LARCENY_MODE(team, mode)                     \
  do {                                                                         \
    team->task_infos.task_larceny_mode = mode;                                 \
  } while (0)

#define MPCOMP_TASK_TEAM_GET_TASK_NESTING_MAX(team)                            \
  team->task_infos.task_nesting_max

#define MPCOMP_TASK_TEAM_SET_TASK_NESTING_MAX(team, val)                       \
  do {                                                                         \
    team->task_infos.task_nesting_max = val;                                   \
  } while (0)

#define MPCOMP_TASK_TEAM_SET_USE_TASK(team)                                    \
  do {                                                                         \
    OPA_cas_int(&(team->task_infos.use_task), 0, 1);                  \
  } while (0)

#define MPCOMP_TASK_TEAM_GET_USE_TASK(team)                                    \
  OPA_load_int(&(team->task_infos.use_task))

#define MPCOMP_TASK_TEAM_RESET_USE_TASK(team)                                  \
  do {                                                                         \
    OPA_store_int(&(team->task_infos.use_task), 0);                   \
  } while (0)

/*** NODE ACCESSORS MACROS ***/

#define MPCOMP_TASK_NODE_GET_TREE_ARRAY_RANK(node)                             \
  node->task_infos.tree_array_rank;
#define MPCOMP_TASK_NODE_SET_TREE_ARRAY_RANK(node, id)                         \
  do {                                                                         \
    node->task_infos.tree_array_rank = id;                                     \
  } while (0)

#define MPCOMP_TASK_NODE_GET_TREE_ARRAY_NODES(node)                            \
  node->task_infos.tree_array_node
#define MPCOMP_TASK_NODE_SET_TREE_ARRAY_NODES(node, ptr)                       \
  do {                                                                         \
    node->task_infos.tree_array_node = ptr;                                    \
  } while (0)

#define MPCOMP_TASK_NODE_GET_TREE_ARRAY_NODE(node, rank)                       \
  node->task_infos.tree_array_node[rank]

#define MPCOMP_TASK_NODE_GET_PATH(node) node->task_infos.path
#define MPCOMP_TASK_NODE_SET_PATH(node, ptr)                                   \
  do {                                                                         \
    node->task_infos.path = ptr;                                               \
  } while (0);

#define MPCOMP_TASK_NODE_GET_TASK_LIST_HEAD(node, type)                        \
  node->task_infos.tasklist[type]
#define MPCOMP_TASK_NODE_SET_TASK_LIST_HEAD(node, type, ptr)                   \
  do {                                                                         \
    node->task_infos.tasklist[type] = ptr;                                     \
  } while (0)

#define MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER(node)                        \
  node->task_infos.tasklist_randBuffer
#define MPCOMP_TASK_NODE_SET_TASK_LIST_RANDBUFFER(node, ptr)                   \
  do {                                                                         \
    node->task_infos.tasklist_randBuffer = ptr;                                \
  } while (0)

/*** MVP ACCESSORS MACROS ***/

#define MPCOMP_TASK_MVP_GET_TREE_ARRAY_RANK(mvp) mvp->task_infos.tree_array_rank
#define MPCOMP_TASK_MVP_SET_TREE_ARRAY_RANK(mvp, id)                           \
  do {                                                                         \
    mvp->task_infos.tree_array_rank = id;                                      \
  } while (0)

#define MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODES(mvp)                              \
  mvp->task_infos.tree_array_node
#define MPCOMP_TASK_MVP_SET_TREE_ARRAY_NODES(mvp, ptr)                         \
  do {                                                                         \
    mvp->task_infos.tree_array_node = ptr;                                     \
  } while (0)

#define MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE(mvp, rank)                         \
  mvp->task_infos.tree_array_node[rank]

#define MPCOMP_TASK_MVP_GET_PATH(mvp) mvp->path

#define MPCOMP_TASK_MVP_GET_TASK_LIST_HEAD(mvp, type)                          \
  mvp->task_infos.tasklist[type]
#define MPCOMP_TASK_MVP_SET_TASK_LIST_HEAD(mvp, type, ptr)                     \
  do {                                                                         \
    mvp->task_infos.tasklist[type] = ptr;                                      \
  } while (0)

#define MPCOMP_TASK_MVP_GET_LAST_STOLEN_TASK_LIST(mvp, type)                   \
  mvp->task_infos.lastStolen_tasklist[type]
#define MPCOMP_TASK_MVP_SET_LAST_STOLEN_TASK_LIST(mvp, type, ptr)              \
  do {                                                                         \
    mvp->task_infos.lastStolen_tasklist[type] = ptr;                           \
  } while (0)

#define MPCOMP_TASK_MVP_GET_TASK_LIST_RANK_BUFFER(mvp, rank)                   \
  MPCOMP_TASK_NODE_GET_TASK_LIST_RANDBUFFER(                                   \
      MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE(mvp, rank))

#define MPCOMP_TASK_MVP_GET_TASK_LIST_RANDBUFFER(mvp)                          \
  mvp->task_infos.tasklist_randBuffer
#define MPCOMP_TASK_MVP_SET_TASK_LIST_RANDBUFFER(mvp, ptr)                     \
  do {                                                                         \
    mvp->task_infos.tasklist_randBuffer = ptr;                                 \
  } while (0);

#define MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK(mvp, type)                     \
  mvp->task_infos.tasklistNodeRank[type]
#define MPCOMP_TASK_MVP_SET_TASK_LIST_NODE_RANK(mvp, type, id)                 \
  do {                                                                         \
    mvp->task_infos.tasklistNodeRank[type] = id;                               \
  } while (0)

#endif /* __MPCOMP_TASK_MACROS_H__ */
