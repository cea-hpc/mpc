#if (!defined( __SCTK_MPCOMP_TASK_UTILS_H__) && MPCOMP_TASK)
#define __SCTK_MPCOMP_TASK_UTILS_H__

#include "mpcomp_internal.h"

/* Return list of 'type' tasks contained in element of rank 'globalRank' */
static inline struct mpcomp_task_list_s *
mpcomp_task_get_list(int globalRank, mpcomp_tasklist_type_t type)
{
     mpcomp_thread_t *t;
     struct mpcomp_task_list_s *list;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

#if MPCOMP_OVERFLOW_CHECKING
     sctk_assert(globalRank >= 0 && globalRank < t->instance->tree_array_size);
     sctk_assert(type >= 0 && type < MPCOMP_TASK_TYPE_COUNT);
#endif /* MPCOMP_OVERFLOW_CHECKING */

    if (mpcomp_is_leaf(globalRank)) {
      mpcomp_mvp_t *mvp = (mpcomp_mvp_t *) (t->mvp->tree_array[globalRank]);
      list = mvp->tasklist[type];
     } else {
      mpcomp_node_t *n = t->mvp->tree_array[globalRank];
      list = n->tasklist[type];
     }

     return list;
}

#endif /* __SCTK_MPCOMP_TASK_UTILS_H__ */
