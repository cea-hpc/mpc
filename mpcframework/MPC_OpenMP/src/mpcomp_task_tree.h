#if (!defined(__MPCOMP_TASK_TREE_H__) && MPCOMP_TASK)
#define __MPCOMP_TASK_TREE_H__

#include "mpc_common_helper.h"
#include <mpc_topology.h>
#include "mpcomp_types.h"
#include "mpcomp_core.h"

#include "sctk_debug.h"

typedef struct mpcomp_task_tree_infos_s {
  int global_rank;                /** */
  int stage_size;                 /** */
  int first_rank;                 /** */
  mpcomp_node_ptr_t **tree_array; /** */
  int *tasklistNodeRank;          /** */
} mpcomp_task_tree_infos_t;

static inline mpcomp_task_tree_infos_t *
mpcomp_task_tree_infos_alloc(const int tree_array_size) {
  int i;
  mpcomp_task_tree_infos_t *task_tree_infos;

  sctk_assert(tree_array_size >= 0);

  const int numa_node_number = mpc_common_max(mpc_topology_get_numa_node_count(), 1);
  sctk_assert(numa_node_number > 0);

  task_tree_infos = mpcomp_alloc( sizeof(mpcomp_task_tree_infos_t) );
  sctk_assert(task_tree_infos);

  task_tree_infos->tree_array =
      mpcomp_alloc( sizeof(mpcomp_node_ptr_t *) * numa_node_number );
  sctk_assert(task_tree_infos->tree_array);

  sctk_assert(tree_array_size > 0);
  for (i = 0; i < numa_node_number; i++) {
    task_tree_infos->tree_array[i] =
        mpcomp_alloc( sizeof(mpcomp_node_ptr_t) * tree_array_size );
    sctk_assert(task_tree_infos->tree_array[i]);
    memset(task_tree_infos->tree_array[i], 0,
           sizeof(mpcomp_node_ptr_t) * tree_array_size);
  }

  task_tree_infos->tasklistNodeRank =
      mpcomp_alloc( sizeof(int) * MPCOMP_TASK_TYPE_COUNT );
  sctk_assert(task_tree_infos->tasklistNodeRank);
  memset(task_tree_infos->tasklistNodeRank, 0,
         MPCOMP_TASK_TYPE_COUNT * sizeof(int));

  task_tree_infos->global_rank = 0;
  task_tree_infos->stage_size = 1;
  task_tree_infos->first_rank = 0;

  return task_tree_infos;
}

static inline mpcomp_task_tree_infos_t *mpcomp_task_tree_infos_duplicate(
    mpcomp_task_tree_infos_t *parent_task_tree_infos, int id_numa) {
  mpcomp_task_tree_infos_t *new_task_tree_infos;

  sctk_assert(id_numa >= 0);
  sctk_assert(parent_task_tree_infos);

  new_task_tree_infos =
      mpcomp_alloc( sizeof(mpcomp_task_tree_infos_t) );
  sctk_assert(new_task_tree_infos);

  memcpy(new_task_tree_infos, parent_task_tree_infos,
         sizeof(mpcomp_task_tree_infos_t));

  return new_task_tree_infos;
}

static inline void
mpcomp_task_tree_infos_free(mpcomp_task_tree_infos_t *task_tree_infos) {
  sctk_assert(task_tree_infos);
  sctk_assert(task_tree_infos->tree_array);
  mpcomp_free(task_tree_infos->tree_array);
  mpcomp_free(task_tree_infos);
}

static inline void mpcomp_task_tree_register_node_in_all_numa_node_array(
    mpcomp_node_ptr_t **tree_array, mpcomp_node_t *node,
    const int global_rank) {
  int i;

  sctk_assert(node);
  sctk_assert(tree_array);
  sctk_assert(global_rank >= 0);

  const int numa_node_number = mpc_common_max(mpc_topology_get_numa_node_count(), 1);

  for (i = 0; i < numa_node_number; i++) {
    sctk_assert(tree_array[i]);
    tree_array[i][global_rank] = node;
  }
}

void mpcomp_task_tree_infos_init(void);

#endif /*  ( !defined( __MPCOMP_TASK_TREE_H__ ) && MPCOMP_TASK ) */
