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
    mpcomp_parallel_region_t *info, void *(*func)(void *), void *data,
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


static inline void __mpcomp_sendtosleep_mvp(mpcomp_mvp_t *mvp) {
  /* Empty function */
}

#endif /* __MPCOMP_PARALLEL_REGION_H__ */
