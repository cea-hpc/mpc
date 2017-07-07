
#if (!defined(__SCTK_MPCOMP_TASK_UTILS_H__) && ( MPCOMP_TASK || defined( MPCOMP_OPENMP_3_0 ) ) ) 
#define __SCTK_MPCOMP_TASK_UTILS_H__

#include "sctk_runtime_config.h"
#include "mpcomp_types.h"

#include "mpcomp_alloc.h"
#include "mpcomp_task.h"
#include "sctk_debug.h"
#include "mpcomp_task_dep.h"
#include "mpcomp_task_list.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_macros.h"

#include "ompt.h"
extern ompt_callback_t* OMPT_Callbacks;

// Round up a size to a power of two specified by val
// Used to insert padding between structures co-allocated using a single
// malloc() call
static size_t __kmp_round_up_to_val(size_t size, size_t val) {
  if (size & (val - 1)) {
    size &= ~(val - 1);
    if (size <= KMP_SIZE_T_MAX - val)
      size += val; // Round up if there is no overflow.
  }
  return size;
} // __kmp_round_up_to_va

/*** Task property primitives ***/

static inline void
mpcomp_task_reset_property(mpcomp_task_property_t *property) {
  *property = 0;
}

static inline void mpcomp_task_set_property(mpcomp_task_property_t *property,
                                            mpcomp_task_property_t mask) {
  *property |= mask;
}

static inline void mpcomp_task_unset_property(mpcomp_task_property_t *property,
                                              mpcomp_task_property_t mask) {
  *property &= ~(mask);
}

static inline int mpcomp_task_property_isset(mpcomp_task_property_t property,
                                             mpcomp_task_property_t mask) {
  return (property & mask);
}

/* Is Serialized Task Context if_clause not handle */
static inline int mpcomp_task_no_deps_is_serialized(mpcomp_thread_t *thread) {
  mpcomp_task_t *current_task = NULL;
  sctk_assert(thread);

  current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
  sctk_assert(current_task);

  if ((current_task &&
       mpcomp_task_property_isset(current_task->property, MPCOMP_TASK_FINAL)) ||
      (thread->info.num_threads == 1) ||
      (current_task &&
       current_task->depth > 8 /*t->instance->team->task_nesting_max*/)) {
    return 1;
  }

  return 0;
}

static inline void mpcomp_task_infos_reset(mpcomp_task_t *task) {
  sctk_assert(task);
  memset(task, 0, sizeof(mpcomp_task_t));
}

static inline int mpcomp_task_is_final(unsigned int flags,
                                       mpcomp_task_t *parent) {
  if (flags & 2) {
    return 1;
  }

  if (parent &&
      mpcomp_task_property_isset(parent->property, MPCOMP_TASK_FINAL)) {
    return 1;
  }

  return 0;
}

/* Initialization of a task structure */
static inline void __mpcomp_task_infos_init(mpcomp_task_t *task,
                                            void (*func)(void *), void *data,
                                            struct mpcomp_thread_s *thread) {
  sctk_assert(task != NULL);
  sctk_assert(thread != NULL);

  /* Reset all task infos to NULL */
  mpcomp_task_infos_reset(task);

  /* Set non null task infos field */
  task->func = func;
  task->data = data;
  task->icvs = thread->info.icvs;

  task->parent = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);
  task->depth = (task->parent) ? task->parent->depth + 1 : 0;
#if MPC_USE_SISTER_LIST
  task->children_lock = SCTK_SPINLOCK_INITIALIZER;
#endif /* MPC_USE_SISTER_LIST */
}

static inline void
mpcomp_task_node_task_infos_reset(struct mpcomp_node_s *node) {
  sctk_assert(node);
  memset(&(node->task_infos), 0, sizeof(mpcomp_task_node_infos_t));
}

static inline void mpcomp_task_mvp_task_infos_reset(struct mpcomp_mvp_s *mvp) {
  sctk_assert(mvp);
  memset(&(mvp->task_infos), 0, sizeof(mpcomp_task_mvp_infos_t));
}

static inline int
mpcomp_task_thread_get_numa_node_id(struct mpcomp_thread_s *thread) {
  sctk_assert(thread);

  if (thread->info.num_threads > 1) {
    sctk_assert(thread->mvp && thread->mvp->father);
    return thread->mvp->father->id_numa;
  }
  return sctk_get_node_from_cpu(sctk_get_init_vp(sctk_get_task_rank()));
}

static inline void
mpcomp_task_thread_task_infos_reset(struct mpcomp_thread_s *thread) {
  sctk_assert(thread);
  memset(&(thread->task_infos), 0, sizeof(mpcomp_task_thread_infos_t));
}

static inline void
mpcomp_tree_array_task_thread_init( struct mpcomp_thread_s* thread )
{
    mpcomp_task_t *implicit_task;
    mpcomp_task_list_t *tied_tasks_list;

    implicit_task = mpcomp_alloc( sizeof(mpcomp_task_t) );
    sctk_assert( implicit_task );
    memset( implicit_task, 0, sizeof(mpcomp_task_t) );

    thread->task_infos.current_task = implicit_task;
    __mpcomp_task_infos_init(implicit_task, NULL, NULL, thread); 
    sctk_atomics_store_int( &( implicit_task->refcount ), 1);  

#if MPCOMP_TASK_DEP_SUPPORT
    implicit_task->task_dep_infos = mpcomp_alloc(sizeof(mpcomp_task_dep_task_infos_t));
    sctk_assert(implicit_task->task_dep_infos); 
    memset(implicit_task->task_dep_infos, 0, sizeof(mpcomp_task_dep_task_infos_t ));
#endif /* MPCOMP_TASK_DEP_SUPPORT */
    
    tied_tasks_list = mpcomp_alloc( sizeof(mpcomp_task_list_t) );
    sctk_assert(tied_tasks_list);
    memset( tied_tasks_list, 0, sizeof(mpcomp_task_list_t) );
  
    thread->task_infos.tied_tasks = tied_tasks_list; 
}

static inline void mpcomp_task_thread_infos_init(struct mpcomp_thread_s *thread) {
  sctk_assert(thread);

  if (!MPCOMP_TASK_THREAD_IS_INITIALIZED(thread)) {
    mpcomp_task_t *implicit_task;
    mpcomp_task_list_t *tied_tasks_list;

    const int numa_node_id = mpcomp_task_thread_get_numa_node_id(thread);

    /* Allocate the default current task (no func, no data, no parent) */
    implicit_task = mpcomp_alloc( sizeof(mpcomp_task_t) );
    sctk_assert(implicit_task);
    MPCOMP_TASK_THREAD_SET_CURRENT_TASK(thread, NULL);

    __mpcomp_task_infos_init(implicit_task, NULL, NULL, thread);

#ifdef MPCOMP_USE_TASKDEP
    implicit_task->task_dep_infos =
        mpcomp_alloc(sizeof(mpcomp_task_dep_task_infos_t));
    sctk_assert(implicit_task->task_dep_infos);
    memset(implicit_task->task_dep_infos, 0,
           sizeof(mpcomp_task_dep_task_infos_t));
#endif /* MPCOMP_USE_TASKDEP */
	

#if 1 //OMPT_SUPPORT
	if( mpcomp_ompt_is_enabled() )
	{
		ompt_task_type_t task_type = ompt_task_implicit;

   	if( OMPT_Callbacks )
   	{
			ompt_callback_task_create_t callback; 
			callback = (ompt_callback_task_create_t) OMPT_Callbacks[ompt_callback_task_create];
			if( callback )
			{
				ompt_data_t* task_data = &( implicit_task->ompt_task_data);
				const void* code_ra = __builtin_return_address(0);	
				callback( NULL, NULL, task_data, task_type, ompt_task_initial, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */
	
    /* Allocate private task data structures */
    tied_tasks_list = mpcomp_alloc( sizeof(mpcomp_task_list_t) );
    sctk_assert(tied_tasks_list);

    MPCOMP_TASK_THREAD_SET_CURRENT_TASK(thread, implicit_task);
    MPCOMP_TASK_THREAD_SET_TIED_TASK_LIST_HEAD(thread, tied_tasks_list);
    sctk_atomics_store_int(&(implicit_task->refcount), 1);

    /* Change tasking_init_done to avoid multiple thread init */
    MPCOMP_TASK_THREAD_CMPL_INIT(thread);
  }
}

static inline void
mpcomp_task_instance_task_infos_reset(struct mpcomp_instance_s *instance) {
  sctk_assert(instance);
  memset(&(instance->task_infos), 0, sizeof(mpcomp_task_instance_infos_t));
}

static inline void
mpcomp_task_instance_task_infos_init(struct mpcomp_instance_s *instance) {
  int i, array_tree_total_size;
  int *tree_base, *array_tree_level_size, *array_tree_level_first;

  sctk_assert(instance && instance->tree_base);
  tree_base = instance->tree_base;

  const int depth = instance->tree_depth;
  array_tree_level_size = (int *)mpcomp_alloc( sizeof(int) * (depth + 1) );
  sctk_assert(array_tree_level_size);

  array_tree_level_first =
      (int *)mpcomp_alloc( sizeof(int) * (depth + 1) );
  sctk_assert(array_tree_level_first);

  array_tree_total_size = 1;
  array_tree_level_size[0] = 1;
  array_tree_level_first[0] = 0;

  for (i = 1; i < depth + 1; i++) {
    const int prev_tree_level_size = array_tree_level_size[i - 1];
    array_tree_level_size[i] = prev_tree_level_size * tree_base[i - 1];
    array_tree_level_first[i] =
        array_tree_level_first[i - 1] + prev_tree_level_size;
    array_tree_total_size += array_tree_level_size[i];
  }

  MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_TOTAL_SIZE(instance,
                                                 array_tree_total_size);
  MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_SIZE(instance,
                                                 array_tree_level_size);
  MPCOMP_TASK_INSTANCE_SET_ARRAY_TREE_LEVEL_FIRST(instance,
                                                  array_tree_level_first);
}

static inline void mpcomp_task_team_infos_reset(struct mpcomp_team_s *team) {
  sctk_assert(team);
  memset(&(team->task_infos), 0, sizeof(mpcomp_task_team_infos_t));
}

static inline void 
mpcomp_task_tree_array_node_init(struct mpcomp_node_s* parent, struct mpcomp_node_s* child, const int index)
{
    struct mpcomp_generic_node_s* meta_node;

    const int vdepth = parent->depth - parent->instance->root->depth;
    const int tree_base_val = parent->instance->tree_base[vdepth];
    const int next_tree_base_val = parent->instance->tree_base[vdepth+1];

    const int global_rank = parent->instance_global_rank + index; 
    child->instance_stage_size = parent->instance_stage_size * tree_base_val;
    child->instance_stage_first_rank = parent->instance_stage_first_rank + parent->instance_stage_size;
    const int local_rank = global_rank - parent->instance_stage_first_rank;
    child->instance_global_rank = child->instance_stage_first_rank + local_rank * next_tree_base_val;

    meta_node = &( parent->instance->tree_array[global_rank] );
    sctk_assert( meta_node );
    meta_node->ptr.node = child;    
    meta_node->type = MPCOMP_CHILDREN_NODE;

    /* Task list */
}

static inline void 
mpcomp_task_tree_array_mvp_init( struct mpcomp_node_s* parent, struct mpcomp_mvp_s* mvp, const int index )
{
    struct mpcomp_generic_node_s* meta_node;

    const int global_rank = parent->instance_global_rank + index;
    meta_node = &( parent->instance->tree_array[global_rank] );
    sctk_assert( meta_node );
    meta_node->ptr.node = mvp; 
    meta_node->type = MPCOMP_CHILDREN_LEAF;

    /* Task list */
}

static inline void mpcomp_task_team_infos_init(struct mpcomp_team_s *team,
                                               int depth) {
  mpcomp_task_team_infos_reset(team);

  const int new_tasks_depth_value =
      sctk_runtime_config_get()->modules.openmp.omp_new_task_depth;
  const int untied_tasks_depth_value =
      sctk_runtime_config_get()->modules.openmp.omp_untied_task_depth;
  const int omp_task_nesting_max =
      sctk_runtime_config_get()->modules.openmp.omp_task_nesting_max;
  const int omp_task_larceny_mode =
      sctk_runtime_config_get()->modules.openmp.omp_task_larceny_mode;

  /* Ensure tasks lists depths are correct */
  const int new_tasks_depth =
      (new_tasks_depth_value < depth) ? new_tasks_depth_value : depth;
  const int untied_tasks_depth =
      (untied_tasks_depth_value < depth) ? untied_tasks_depth_value : depth;

  /* Set variable in team task infos */
  MPCOMP_TASK_TEAM_SET_TASKLIST_DEPTH(team, MPCOMP_TASK_TYPE_NEW,
                                      new_tasks_depth);
  MPCOMP_TASK_TEAM_SET_TASKLIST_DEPTH(team, MPCOMP_TASK_TYPE_UNTIED,
                                      untied_tasks_depth);
  MPCOMP_TASK_TEAM_SET_TASK_NESTING_MAX(team, omp_task_nesting_max);
  MPCOMP_TASK_TEAM_SET_TASK_LARCENY_MODE(team, omp_task_larceny_mode);
}

/* mpcomp_task.c */
int mpcomp_task_all_task_executed(void);
void mpcomp_task_schedule(void);
void __mpcomp_task_exit(void);
void mpcomp_task_scheduling_infos_init(void);
int paranoiac_test_task_exit_check(void);
int mpcomp_task_get_task_left_in_team(struct mpcomp_team_s *);

/* Return list of 'type' tasks contained in element of rank 'globalRank' */
static inline struct mpcomp_task_list_s *
mpcomp_task_get_list(int globalRank, mpcomp_tasklist_type_t type) {
  mpcomp_thread_t *thread = NULL;
  struct mpcomp_task_list_s *list = NULL;

  sctk_assert(globalRank >= 0);
  sctk_assert(sctk_openmp_thread_tls != NULL);
  sctk_assert(type >= 0 && type < MPCOMP_TASK_TYPE_COUNT);

  /* Retrieve the current thread information */
  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(thread->mvp != NULL);
  sctk_assert(globalRank <
              MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(thread->instance));

  if (!MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODES(thread->mvp)) {
    return NULL;
  }

  if (mpcomp_is_leaf(thread->instance, globalRank)) {
    struct mpcomp_mvp_s *mvp =
        (struct mpcomp_mvp_s *)(MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE(
            thread->mvp, globalRank));
    list = MPCOMP_TASK_MVP_GET_TASK_LIST_HEAD(mvp, type);
  } else {
    struct mpcomp_node_s *node =
        (struct mpcomp_node_s *)(MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE(
            thread->mvp, globalRank));
    list = MPCOMP_TASK_NODE_GET_TASK_LIST_HEAD(node, type);
  }

  sctk_assert(list != NULL);
  return list;
}

struct mpcomp_task_s *__mpcomp_task_alloc(void (*fn)(void *), void *data,
                                          void (*cpyfn)(void *, void *),
                                          long arg_size, long arg_align,
                                          bool if_clause, unsigned flags,
                                          int deps_num);
void __mpcomp_task_process(mpcomp_task_t *new_task, bool if_clause);
void __mpcomp_task(void (*fn)(void *), void *data,
                   void (*cpyfn)(void *, void *), long arg_size, long arg_align,
                   bool if_clause, unsigned flags);

void __mpcomp_taskgroup_start(void);
void __mpcomp_taskgroup_end(void);
#endif /* __SCTK_MPCOMP_TASK_UTILS_H__ */
