
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

#include "mpcomp_task_stealing.h"

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

static inline int
__mpcomp_task_node_list_init( struct mpcomp_node_s* parent, struct mpcomp_node_s* child, const mpcomp_tasklist_type_t type )
{
    int tasklistNodeRank, allocation;
    mpcomp_task_list_t* list;

    const int task_vdepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH( parent->instance->team, type );
    const int child_vdepth = child->depth - parent->instance->root->depth;
 
    if( child_vdepth < task_vdepth + 1 )
        return 0;
    
    if( parent->depth >= task_vdepth )
    {
        allocation = 0;
        list = parent->task_infos.tasklist[type]; 
        sctk_assert( list );
        tasklistNodeRank = MPCOMP_TASK_NODE_GET_TASK_LIST_NODE_RANK(parent, type); 
    }
    else
    {
        sctk_assert( child->depth == task_vdepth );
        allocation = 1;
        list = mpcomp_alloc( sizeof(struct mpcomp_task_list_s) );
        sctk_assert(list);
        mpcomp_task_list_new(list);
        tasklistNodeRank = child->tree_array_rank; 
    }

    MPCOMP_TASK_NODE_SET_TASK_LIST_HEAD( child, type, list ); 
    MPCOMP_TASK_NODE_SET_LAST_STOLEN_TASK_LIST( child, type, NULL );
    MPCOMP_TASK_NODE_SET_TASK_LIST_NODE_RANK(child, type, tasklistNodeRank);     

    return allocation;
} 

static inline int 
__mpcomp_task_mvp_list_init( struct mpcomp_node_s* parent, struct mpcomp_mvp_s* child, const mpcomp_tasklist_type_t type  )
{
    int tasklistNodeRank, allocation;
    mpcomp_task_list_t* list = NULL;
    
    if( parent->task_infos.tasklist[type] )
    {
        allocation = 0;
        list = parent->task_infos.tasklist[type]; 
        tasklistNodeRank = MPCOMP_TASK_NODE_GET_TASK_LIST_NODE_RANK( parent, type ); 
    }
    else
    {
        allocation = 1;
        list = mpcomp_alloc( sizeof(struct mpcomp_task_list_s) );
        sctk_assert(list);
        mpcomp_task_list_new(list);
        sctk_assert( child->threads );
        tasklistNodeRank = child->threads->tree_array_rank; 
    }
    
    MPCOMP_TASK_MVP_SET_TASK_LIST_HEAD( child, type, list ); 
    MPCOMP_TASK_MVP_SET_LAST_STOLEN_TASK_LIST( child, type, NULL );
    MPCOMP_TASK_MVP_SET_TASK_LIST_NODE_RANK( child, type, tasklistNodeRank );     
    return allocation;
} 

static inline void 
__mpcomp_task_node_infos_init( struct mpcomp_node_s* parent, struct mpcomp_node_s* child )
{
    int ret, type;
    mpcomp_instance_t* instance;
    
    sctk_assert( child );
    sctk_assert( parent );
    sctk_assert( parent->instance );

    instance = parent->instance;
    sctk_assert( instance->team );

    const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( instance->team ); 

    for (type = 0, ret = 0; type < MPCOMP_TASK_TYPE_COUNT; type++) 
        ret += __mpcomp_task_node_list_init( parent, child, type );

    if (ret )
    {
        if( larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
            larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER)
        {
            const int global_rank = child->tree_array_rank;
            mpcomp_task_init_victim_random( &( instance->tree_array[global_rank] ) );   
        }
    }
}

static inline void __mpcomp_task_mvp_infos_init( struct mpcomp_node_s* parent, struct mpcomp_mvp_s* child )
{
    int ret, type;
    mpcomp_instance_t* instance;
    
    sctk_assert( child );
    sctk_assert( parent );
    sctk_assert( parent->instance );

    instance = parent->instance;
    sctk_assert( instance->team );

    const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE( instance->team ); 

    for (type = 0, ret = 0; type < MPCOMP_TASK_TYPE_COUNT; type++) 
        ret += __mpcomp_task_mvp_list_init( parent, child, type );

    if (ret )
    {
        if( larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
            larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER)
        {
            sctk_assert( child->threads );
            const int global_rank = child->threads->tree_array_rank;
            mpcomp_task_init_victim_random( &( instance->tree_array[global_rank] ) );   
        }
    }
}

static inline void mpcomp_task_team_infos_init(struct mpcomp_team_s *team,
                                               int depth) {

  memset(&(team->task_infos), 0, sizeof(mpcomp_task_team_infos_t));

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
mpcomp_task_get_list(int globalRank, mpcomp_tasklist_type_t type) 
{
    mpcomp_thread_t* thread;
    mpcomp_instance_t* instance;
    mpcomp_generic_node_t* gen_node;
    struct mpcomp_task_list_s* list;

    sctk_assert(type >= 0 && type < MPCOMP_TASK_TYPE_COUNT);
  
    /* Retrieve the current thread information */
    sctk_assert(sctk_openmp_thread_tls != NULL);
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert( thread->mvp );

    /* Retrieve the current thread instance */
    sctk_assert( thread->instance );
    instance = thread->instance;

    /* Retrieve target node or mvp generic node */
    sctk_assert( globalRank >= 0 && globalRank < instance->tree_array_size );
    gen_node = &(instance->tree_array[globalRank] );
    
    /* Retrieve target node or mvp tasklist */
    if( gen_node->type == MPCOMP_CHILDREN_LEAF )
        return  MPCOMP_TASK_MVP_GET_TASK_LIST_HEAD(gen_node->ptr.mvp, type);

    if( gen_node->type == MPCOMP_CHILDREN_NODE )
        return MPCOMP_TASK_NODE_GET_TASK_LIST_HEAD(gen_node->ptr.node, type);

    /* Extra node to preserve regular tree_shape */
    sctk_assert( gen_node->type == MPCOMP_CHILDREN_NULL );
    return NULL;
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
