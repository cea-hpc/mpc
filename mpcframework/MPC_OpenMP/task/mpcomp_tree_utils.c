
#include "mpcomp_types_def.h"

#if( MPCOMP_TASK || defined( MPCOMP_OPENMP_3_0 ) )

#include "sctk_debug.h"
#include "sctk_atomics.h"
#include "mpcomp_types.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_macros.h"

int mpcomp_is_leaf(mpcomp_instance_t *instance, int globalRank) {
  sctk_assert(instance);
  sctk_assert(globalRank >= 0 &&
              MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(instance));
  return (globalRank >=
          (MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(instance) -
           instance->nb_mvps));
}

/* Return the ith neighbour of the element at rank 'globalRank' in the
 * tree_array */
int mpcomp_get_neighbour(int globalRank, int i) {
  /* Retrieve the current thread information */
  sctk_assert(sctk_openmp_thread_tls);
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(globalRank >= 0);
  sctk_assert(thread->instance);
  sctk_assert(globalRank <
              MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(thread->instance));

  int *treeBase = thread->instance->tree_base;
  sctk_assert(treeBase);

  int *path = NULL;
  int vectorSize;

  if (mpcomp_is_leaf(thread->instance, globalRank)) {
    mpcomp_mvp_t *mvp = (mpcomp_mvp_t *)(MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE(
        thread->mvp, globalRank));
    path = MPCOMP_TASK_MVP_GET_PATH(mvp);
    vectorSize = mvp->father->depth + 1;
  } else {
    mpcomp_node_t *node =
        (mpcomp_node_t *)(MPCOMP_TASK_NODE_GET_TREE_ARRAY_NODE(thread->mvp,
                                                               globalRank));
    ;
    path = MPCOMP_TASK_NODE_GET_PATH(node);
    vectorSize = node->depth;
  }

  int j;
  int r = i;
  int id = 0;
  int firstRank = 0;
  int nbSubleaves = 1;
  int v[vectorSize], res[vectorSize];

  for (j = 0; j < vectorSize; j++) {
    int base = treeBase[vectorSize - 1 - j];
    int level_size =
        MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE(thread->instance, j);

    sctk_assert(vectorSize - 1 - j >= 0 &&
                vectorSize - 1 - j < thread->mvp->father->depth + 1);
    sctk_assert(j >= 0 && j <= thread->mvp->father->depth + 1);

    /* Somme de I avec le codage de 0 dans le vecteur de l'arbre */
    v[j] = r % base;
    r /= base;

    /* Addition sans retenue avec le vecteur de n */
    res[j] = (path[vectorSize - 1 - j] + v[j]) % base;

    /* Calcul de l'identifiant du voisin dans l'arbre */
    id += res[j] * nbSubleaves;
    nbSubleaves *= base;
    firstRank += level_size;
  }

  return id + firstRank;
}

/* Return the ancestor of element at rank 'globalRank' at depth 'depth' */
int mpcomp_get_ancestor(int globalRank, int depth) {

  /* If it's the root, ancestor is itself */
  if (globalRank == 0) {
    return 0;
  }

  /* Retrieve the current thread information */
  sctk_assert(sctk_openmp_thread_tls);
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(globalRank >= 0);
  sctk_assert(thread->instance);
  sctk_assert(globalRank <
              MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_TOTAL_SIZE(thread->instance));

  int *path;
  int currentDepth;

  if (mpcomp_is_leaf(thread->instance, globalRank)) {
    mpcomp_mvp_t *mvp = (mpcomp_mvp_t *)(MPCOMP_TASK_MVP_GET_TREE_ARRAY_NODE(
        thread->mvp, globalRank));
    path = MPCOMP_TASK_MVP_GET_PATH(mvp);
    currentDepth = mvp->father->depth + 1;
  } else {
    mpcomp_node_t *node =
        (mpcomp_node_t *)(MPCOMP_TASK_NODE_GET_TREE_ARRAY_NODE(thread->mvp,
                                                               globalRank));
    path = MPCOMP_TASK_NODE_GET_PATH(node);
    currentDepth = node->depth;
  }

  if (currentDepth == depth) {
    return globalRank;
  }

  sctk_assert(depth >= 0 && depth < currentDepth);

  int i;
  int ancestor_id = 0;
  int firstRank = 0;
  int nbSubLeaves = 1;
  int *treeBase = thread->instance->tree_base;

  for (i = 0; i < depth; i++) {
    ancestor_id += path[depth - 1 - i] * nbSubLeaves;
    nbSubLeaves *= treeBase[depth - 1 - i];
    firstRank +=
        MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE(thread->instance, i);
  }

  return firstRank + ancestor_id;
}

/* Recursive call for checking neighbourhood from node n */
static void __mpcomp_task_check_neighbourhood_r(mpcomp_node_t *node) {
  int i, j;

  sctk_assert(node != NULL);

  /* Retrieve the current thread information */
  sctk_assert(sctk_openmp_thread_tls);
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  for (j = 1; j < MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE(
                      thread->instance, node->depth);
       j++) {
    const int tree_array_rank = MPCOMP_TASK_NODE_GET_TREE_ARRAY_RANK(node);
  }

  switch (node->child_type) {
  case MPCOMP_CHILDREN_NODE:
    for (i = 0; i < node->nb_children; i++) {
      mpcomp_node_t *child = node->children.node[i];
      /* Call recursively for all children nodes */
      __mpcomp_task_check_neighbourhood_r(child);
    }
    break;

  case MPCOMP_CHILDREN_LEAF:
    for (i = 0; i < node->nb_children; i++) {

      /* All the children are leafs */
      mpcomp_mvp_t *mvp = node->children.leaf[i];
      sctk_assert(mvp != NULL);

      for (j = 1; j < MPCOMP_TASK_INSTANCE_GET_ARRAY_TREE_LEVEL_SIZE(
                          thread->instance, node->depth + 1);
           j++) {
        const int tree_array_rank = MPCOMP_TASK_MVP_GET_TREE_ARRAY_RANK(mvp);
        fprintf(stderr, "neighbour n°%d of %d: %d\n", j, tree_array_rank,
                mpcomp_get_neighbour(tree_array_rank, j));
      }
    }
    break;

  default:
    sctk_nodebug("not reachable");
  }
}

/* Check all neighbour of all nodes */
void __mpcomp_task_check_neighbourhood(void) {
  mpcomp_thread_t *t;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  __mpcomp_task_check_neighbourhood_r(t->mvp->root);
}
#endif /* MPCOMP_TASK */
