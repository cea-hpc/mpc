#ifndef __MPCOMP_OPENMP_THREAD_TLS_H__
#define __MPCOMP_OPENMP_THREAD_TLS_H__

#include "mpc_common_debug.h"
#include "mpcomp_types.h"

static inline mpcomp_thread_t *mpcomp_get_thread_tls(void) {
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert(thread);
  return thread;
}

static inline mpcomp_thread_t *mpc_omp_tree_array_ancestor_get_thread_tls(int level) {
  int i;
  mpcomp_thread_t *thread = mpcomp_get_thread_tls();
  const int max_level_var = thread->info.icvs.levels_var;

  /* If level is outside of bounds, return -1 */
  if (level < 0 || level >= max_level_var) {
    return NULL;
  }

  /* Go up to the right level and catch the rank */
  for (i = max_level_var; i > level; i--) {
    thread = thread->father;
    assert(thread != NULL);
  }

  return thread;
}

static inline mpcomp_thread_t *
mpcomp_thread_tls_store(mpcomp_thread_t *new_value) {
  sctk_openmp_thread_tls = (void *)new_value;
  return new_value;
}

static inline mpcomp_thread_t *
mpcomp_thread_tls_swap(mpcomp_thread_t *new_value) {
  mpcomp_thread_t *old = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_openmp_thread_tls = (void *)new_value;
  return old;
}

static inline mpcomp_thread_t *mpcomp_thread_tls_store_first_mvp(void) {
  mpcomp_thread_t *father = NULL;
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert(thread);
  father = thread->father;
  sctk_openmp_thread_tls = (void *)father;
  return father;
}

static inline mpcomp_thread_t *mpcomp_thread_tls_store_father(void) {
  mpcomp_thread_t *father = NULL;
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert(thread);
  father = thread->father;
  sctk_openmp_thread_tls = (void *)father;
  return father;
}

static inline mpcomp_thread_t *mpcomp_thread_tls_swap_father(void) {
  mpcomp_thread_t *father = NULL;
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert(thread);
  father = thread->father;
  sctk_openmp_thread_tls = (void *)father;
  return thread;
}

#endif /* __MPCOMP_OPENMP_THREAD_TLS_H__ */
