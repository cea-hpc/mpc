#ifndef __MPCOMP_PARALLEL_REGION_H__
#define __MPCOMP_PARALLEL_REGION_H__

#include "sctk_debug.h"
#include "mpcomp_types.h"

void __mpcomp_internal_begin_parallel_region(
    struct mpcomp_new_parallel_region_info_s *, unsigned arg_num_threads);
void __mpcomp_internal_end_parallel_region(struct mpcomp_instance_s *instance);

void *mpcomp_slave_mvp_node(void *);
void *mpcomp_slave_mvp_leaf(void *);

void __mpcomp_start_parallel_region(void (*)(void *), void *, unsigned);
void __mpcomp_start_sections_parallel_region(void (*)(void *), void *, unsigned,
                                             unsigned);

void __mpcomp_start_parallel_dynamic_loop(void (*)(void *), void *, unsigned,
                                          long, long, long, long);
void __mpcomp_start_parallel_static_loop(void (*)(void *), void *, unsigned,
                                         long, long, long, long);
void __mpcomp_start_parallel_guided_loop(void (*)(void *), void *, unsigned,
                                         long, long, long, long);
void __mpcomp_start_parallel_runtime_loop(void (*)(void *), void *, unsigned,
                                          long, long, long, long);

/* INLINED FUNCTION */
static inline void __mpcomp_parallel_set_specific_infos(
    mpcomp_new_parallel_region_info_t *info, void *(*func)(void *), void *data,
    mpcomp_local_icv_t icvs, mpcomp_combined_mode_t type) {

  sctk_assert(info);

  info->func = (void *(*)(void *))func;
  info->shared = data;
  info->icvs = icvs;
  info->combined_pragma = type;
}

static inline void __mpcomp_save_team_info(mpcomp_team_t *team,
                                           mpcomp_thread_t *t) {
  sctk_nodebug("%s:  saving single/section %d and for dyn %d", __func__,
               t->single_sections_current, t->for_dyn_current);
  team->info.single_sections_current_save = t->single_sections_current;
  team->info.for_dyn_current_save = t->for_dyn_current;
}

/* TODO check that this is correct (only 1 thread) */
static inline void __mpcomp_wakeup_mvp(mpcomp_mvp_t *mvp, mpcomp_node_t *n) {

  if (n) {
    sctk_assert(n == mvp->father);
#if MPCOMP_TRANSFER_INFO_ON_NODES
    mvp->info = n->info;
#else
    mvp->info = n->instance->team->info;
#endif
  }

  sctk_assert(mvp);

  /* Update the total number of threads for this microVP */
  mvp->nb_threads = 1;
  mvp->threads[0].info = mvp->info;
  mvp->threads[0].rank = mvp->min_index[mpcomp_global_icvs.affinity];
  mvp->threads[0].single_sections_current =
      mvp->info.single_sections_current_save;
  mvp->threads[0].for_dyn_current = mvp->info.for_dyn_current_save;

  sctk_nodebug("%s: value of single_sections_current %d and for_dyn_current %d",
               __func__, mvp->info.single_sections_current_save,
               mvp->info.for_dyn_current_save);
}

static inline void __mpcomp_sendtosleep_mvp(mpcomp_mvp_t *mvp) {
  /* Empty function */
}

static inline mpcomp_node_t *__mpcomp_wakeup_node(int master,
                                                  mpcomp_node_t *start_node,
                                                  unsigned num_threads,
                                                  mpcomp_instance_t *instance) {
  int i;
  mpcomp_node_t *n;

  sctk_assert(start_node);
  sctk_assert(instance);

  n = start_node;

  while (n->child_type != MPCOMP_CHILDREN_LEAF) {
    int nb_children_involved = 1;

    /* This part should keep two different loops because we first need to
     * update barrier_num_threads before waking up the children to avoid
     * one child to complete a barrier before waking up all children and
     * getting a wrong barrier_num_threads value */

    /* Compute the number of children that will be involved for the barrier
     */
    for (i = 1; i < n->nb_children; i++) {
      if (n->children.node[i]->min_index[mpcomp_global_icvs.affinity] <
          num_threads) {
        nb_children_involved++;
      }
    }

    sctk_nodebug("%s: nb_children_involved=%d", __func__, nb_children_involved);

    /* Update the number of threads for the barrier */
    n->barrier_num_threads = nb_children_involved;

    if (nb_children_involved == 1) {
      /* Bypass algorithm are ok with threads only with compact affinity
         Otherwise, with more than 1 thread, multiple subtree of the root
         will be used
       */
      if (master && mpcomp_global_icvs.affinity == MPCOMP_AFFINITY_COMPACT) {
        /* Only one subtree => bypass */
        instance->team->info.new_root = n->children.node[0];
        sctk_nodebug("__mpcomp_wakeup_node: Bypassing "
                     "root from depth %d to %d",
                     n->depth, n->depth + 1);
      }
    } else {
      /* Wake up children and transfer information */
      for (i = 1; i < n->nb_children; i++) {
        if (n->children.node[i]->min_index[mpcomp_global_icvs.affinity] <
            num_threads) {
#if MPCOMP_TRANSFER_INFO_ON_NODES
          n->children.node[i]->info = n->info;
#endif
          n->children.node[i]->slave_running = 1;
        }
      }
    }

/* Transfer information for the first child */
#if MPCOMP_TRANSFER_INFO_ON_NODES
    n->children.node[0]->info = n->info;
#endif

    /* Go down towards the leaves */
    n = n->children.node[0];
  }

  return n;
}

static inline mpcomp_node_t *__mpcomp_wakeup_leaf(mpcomp_node_t *start_node,
                                                  int num_threads,
                                                  mpcomp_instance_t *instance) {

  int i;
  mpcomp_node_t *n;

  n = start_node;

  /* Wake up children leaf */
  sctk_assert(n->child_type == MPCOMP_CHILDREN_LEAF);
  int nb_children_involved = 1;
  for (i = 1; i < n->nb_children; i++) {
    if (n->children.leaf[i]->min_index[mpcomp_global_icvs.affinity] <
        num_threads) {
      nb_children_involved++;
    }
  }
  sctk_nodebug("__mpcomp_wakeup_leaf: nb_leaves_involved=%d",
               nb_children_involved);
  n->barrier_num_threads = nb_children_involved;
  for (i = 1; i < n->nb_children; i++) {
    if (n->children.leaf[i]->min_index[mpcomp_global_icvs.affinity] <
        num_threads) {
#if MPCOMP_TRANSFER_INFO_ON_NODES
      n->children.leaf[i]->info = n->info;
#else
      n->children.leaf[i]->info = instance->team->info;
#endif

      sctk_nodebug("__mpcomp_wakeup_leaf: waking up leaf %d", i);

      n->children.leaf[i]->slave_running = 1;
    }
  }

  return n;
}

#endif /* __MPCOMP_PARALLEL_REGION_H__ */
