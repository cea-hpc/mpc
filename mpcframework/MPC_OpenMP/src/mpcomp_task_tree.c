#include "mpcomp_types_def.h"

#if MPCOMP_TASK

#include "sctk_debug.h"
#include "mpcomp_alloc.h"
#include "mpcomp_types.h"
#include "mpcomp_task_list.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_task_tree.h"
#include "mpcomp_task_stealing.h"

#if 0
static int mpcomp_task_tree_task_list_alloc(
    mpcomp_tree_meta_elt_t *meta_elt, int tasklist_depth, int *tasklistNodeRank,
    mpcomp_tasklist_type_t type, const int depth, const int id_numa) {
  mpcomp_task_list_t *list;

  sctk_assert(meta_elt);
  sctk_assert(depth >= 0);
  sctk_assert(tasklistNodeRank);

  /* If the node correspond to the new list depth, allocate the data structures
   */
  if (depth != tasklist_depth) {
    return 0;
  }


  switch (meta_elt->type) {
  case MPCOMP_TREE_META_ELT_NODE:
    sctk_assert(meta_elt->ptr.node);
    tasklistNodeRank[type] =
        MPCOMP_TASK_NODE_GET_TREE_ARRAY_RANK(meta_elt->ptr.node);
    MPCOMP_TASK_NODE_SET_TASK_LIST_HEAD(meta_elt->ptr.node, type, list);
    break;

  case MPCOMP_TREE_META_ELT_MVP:
    sctk_assert(meta_elt->ptr.mvp);
    tasklistNodeRank[type] =
        MPCOMP_TASK_MVP_GET_TREE_ARRAY_RANK(meta_elt->ptr.mvp);
    MPCOMP_TASK_MVP_SET_TASK_LIST_HEAD(meta_elt->ptr.mvp, type, list);
    break;

  default:
    sctk_fatal("unkown meta tree elt type");
  }

  return 1;
}

static inline void mpcomp_task_tree_allocate_all_task_list(
    mpcomp_tree_meta_elt_t *meta_elt, mpcomp_team_t *team,
    mpcomp_task_tree_infos_t *task_tree_infos, const int global_rank,
    const int depth, const int id_numa) {
  int type, ret;
  mpcomp_task_list_t *list;

  sctk_assert(meta_elt);
  sctk_assert(team);
  sctk_assert(task_tree_infos);

  /* Get stealing policy */
  const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE(team);

  for (type = 0, ret = 0; type < MPCOMP_TASK_TYPE_COUNT; type++) {
    const int tasklist_depth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(team, type);
    ret += mpcomp_task_tree_task_list_alloc(meta_elt, tasklist_depth,
                                            task_tree_infos->tasklistNodeRank,
                                            type, depth, id_numa);
    if (meta_elt->type == MPCOMP_TREE_META_ELT_MVP)
      MPCOMP_TASK_MVP_SET_TASK_LIST_NODE_RANK(
          meta_elt->ptr.mvp, type, task_tree_infos->tasklistNodeRank[type]);
  }

  if (ret && (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
              larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER)) {
    mpcomp_task_init_victim_random(meta_elt->ptr.node, meta_elt->type,
                                   global_rank);
  }
}

static void mpcomp_task_tree_init_task_tree_infos_leaf(
    mpcomp_mvp_t *mvp, mpcomp_task_tree_infos_t *task_tree_infos,
    const int rank, const int depth, const int id_numa) {
  mpcomp_thread_t *omp_thread_tls;
  mpcomp_tree_meta_elt_t meta_elt;

  sctk_assert(mvp);
  sctk_assert(task_tree_infos);

  /* Retrieve the current thread information */
  sctk_assert(sctk_openmp_thread_tls);
  omp_thread_tls = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  meta_elt.type = MPCOMP_TREE_META_ELT_MVP;
  meta_elt.ptr.mvp = mvp;

  sctk_assert(omp_thread_tls->instance);
  sctk_assert(rank < MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(
                         omp_thread_tls->instance));

  /* Set Leaf informations */
  MPCOMP_TASK_MVP_SET_TREE_ARRAY_RANK(mvp, rank);
  mpcomp_task_tree_register_node_in_all_numa_node_array(
      task_tree_infos->tree_array, (mpcomp_node_t *)mvp, rank);
  MPCOMP_TASK_MVP_SET_TREE_ARRAY_NODES(mvp,
                                       task_tree_infos->tree_array[id_numa]);

  /* Init untied and new list */
  sctk_assert(omp_thread_tls->instance->team);
  mpcomp_task_tree_allocate_all_task_list(
      &meta_elt, omp_thread_tls->instance->team, task_tree_infos, rank, depth,
      id_numa);
}

static mpcomp_task_tree_infos_t *mpcomp_task_tree_init_task_tree_infos_node(
    mpcomp_node_t *node, mpcomp_task_tree_infos_t *parent_task_tree_infos,
    int index) {
  int i;
  mpcomp_thread_t *thread;
  mpcomp_tree_meta_elt_t meta_elt;
  mpcomp_task_tree_infos_t *child_task_tree_infos;

  sctk_assert(index >= 0);
  sctk_assert(node != NULL);
  sctk_assert(parent_task_tree_infos != NULL);

  /* Retrieve the current thread information */
  sctk_assert(sctk_openmp_thread_tls);
  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  meta_elt.type = MPCOMP_TREE_META_ELT_NODE;
  meta_elt.ptr.node = node;

  const int cur_globalRank = parent_task_tree_infos->global_rank + index;
  sctk_assert(thread->instance);
  sctk_assert(cur_globalRank <
              MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(thread->instance));

  sctk_assert(node->depth >= 0);
  sctk_assert(thread->instance->tree_base);
  const int tree_base_value = thread->instance->tree_base[node->depth];

  /* Compute arguments for children */
  child_task_tree_infos =
      mpcomp_task_tree_infos_duplicate(parent_task_tree_infos, node->id_numa);
  child_task_tree_infos->stage_size =
      parent_task_tree_infos->stage_size * tree_base_value;
  child_task_tree_infos->first_rank =
      parent_task_tree_infos->first_rank + parent_task_tree_infos->stage_size;
  child_task_tree_infos->global_rank =
      child_task_tree_infos->first_rank +
      (cur_globalRank - parent_task_tree_infos->first_rank) * tree_base_value;

  const int numa_node_number = sctk_max(mpc_topology_get_numa_node_count(), 1);
  for (i = 0; i < numa_node_number; i++)
    parent_task_tree_infos->tree_array[i][cur_globalRank] = node;

  MPCOMP_TASK_NODE_SET_TREE_ARRAY_RANK(node, cur_globalRank);
  MPCOMP_TASK_NODE_SET_TREE_ARRAY_NODES(
      node, parent_task_tree_infos->tree_array[node->id_numa]);

  sctk_assert(thread->instance->team);
  mpcomp_task_tree_allocate_all_task_list(&meta_elt, thread->instance->team,
                                          child_task_tree_infos, cur_globalRank,
                                          node->depth, node->id_numa);

  return child_task_tree_infos;
}

static inline void mpcomp_task_tree_set_path(mpcomp_mvp_t *mvp, int rank) {
  mpcomp_node_t *node;

  mpcomp_thread_t *thread;
  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(mvp);

  MPCOMP_TASK_MVP_SET_PATH(mvp, mvp->tree_rank);

  node = mvp->father;
  while (node && !MPCOMP_TASK_NODE_GET_PATH(node)) {
    MPCOMP_TASK_NODE_SET_PATH(node, MPCOMP_TASK_MVP_GET_PATH(mvp));
    node = node->father;
  }
}

/* Recursive initialization of mpcomp tasks lists (new and untied) */
static void
mpcomp_task_tree_infos_init_r(mpcomp_node_t *node,
                              mpcomp_task_tree_infos_t *parent_task_tree_infos,
                              int index) {
  int i, next_child;
  mpcomp_task_tree_infos_t *child_task_tree_infos;

  sctk_assert(node);
  sctk_assert(index >= 0);
  sctk_assert(parent_task_tree_infos);

  const int cur_rank = parent_task_tree_infos->global_rank + index;
  child_task_tree_infos = mpcomp_task_tree_init_task_tree_infos_node(
      node, parent_task_tree_infos, index);

  const int node_child_type = node->child_type;
  const int branch_child_num =
      child_task_tree_infos->stage_size / parent_task_tree_infos->stage_size;
  /* Call recursively for all children nodes
   * or the children leafs */

  for (i = 0; i < node->nb_children; i++) {
    /* Look at deeper levels */
    switch (node_child_type) {
    case MPCOMP_CHILDREN_NODE:
      mpcomp_task_tree_infos_init_r(node->children.node[i],
                                    child_task_tree_infos, i);
      break;

    case MPCOMP_CHILDREN_LEAF:
      next_child = child_task_tree_infos->global_rank + i;
      mpcomp_task_tree_init_task_tree_infos_leaf(
          node->children.leaf[i], child_task_tree_infos, next_child,
          node->depth + 1, node->id_numa);
      mpcomp_task_tree_set_path(node->children.leaf[i], next_child);
      break;

    default:
      sctk_nodebug("not reachable");
    }
  }

  sctk_free(child_task_tree_infos);
}

void mpcomp_task_tree_infos_init(void) {
  mpcomp_team_t *team;
  mpcomp_thread_t *omp_thread_tls;

  /* Retrieve the current thread information */
  sctk_assert(sctk_openmp_thread_tls);
  omp_thread_tls = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(omp_thread_tls->instance);
  sctk_assert(omp_thread_tls->instance->team);
  team = omp_thread_tls->instance->team;

  /* Executed only when there are multiple threads */
  if (omp_thread_tls->info.num_threads > 1) {
    if (!MPCOMP_TASK_TEAM_IS_INITIALIZED(team)) {
      /* Executed only one time per process */
      if (MPCOMP_TASK_TEAM_TRY_INIT(team)) {
        mpcomp_task_tree_infos_t *task_tree_infos;
        const int tree_array_size =
            MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(
                omp_thread_tls->instance);

        sctk_assert(tree_array_size > 0);
        task_tree_infos = mpcomp_task_tree_infos_alloc(tree_array_size);

        sctk_assert(omp_thread_tls->mvp);
        mpcomp_task_tree_infos_init_r(omp_thread_tls->mvp->root,
                                      task_tree_infos, 0);

        mpcomp_task_tree_infos_free(task_tree_infos);
        MPCOMP_TASK_TEAM_CMPL_INIT(team);
      }
    }

    while (!MPCOMP_TASK_TEAM_IS_INITIALIZED(team)) {
      /* All threads wait until allocation is done */
      sctk_thread_yield();
    }
  }
}

/* Release of mpcomp tasks lists (new and untied) */
void __mpcomp_task_list_infos_exit(void) {
  mpcomp_thread_t *omp_thread_tls;

  /* Retrieve the current thread information */
  sctk_assert(sctk_openmp_thread_tls);
  omp_thread_tls = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  /* TODO: Add a recusrive search in children_instance of nb_mvps for nested
   * parallelism support */
  sctk_assert(omp_thread_tls->instance);
  if (omp_thread_tls->instance->nb_mvps > 1) {
    __mpcomp_task_list_infos_exit_r(omp_thread_tls->instance->root);
  } else {
    sctk_assert(omp_thread_tls->children_instance);
    if (omp_thread_tls->children_instance->nb_mvps > 1) {
      __mpcomp_task_list_infos_exit_r(omp_thread_tls->children_instance->root);
    }
  }
}
#endif 

#endif /* MPCOMP_TASK */
