#ifndef __MPCOMP_OPENMP_THREAD_TLS_H__
#define __MPCOMP_OPENMP_THREAD_TLS_H__

#include "mpc_common_debug.h"
#include "mpcomp_types.h"

static inline mpc_omp_thread_t *mpc_omp_get_thread_tls(void) {
  mpc_omp_thread_t *thread = (mpc_omp_thread_t *)mpc_omp_tls;
  assert(thread);
  return thread;
}

static inline mpc_omp_thread_t *mpc_omp_tree_array_ancestor_get_thread_tls(int level) {
  int i;
  mpc_omp_thread_t *thread = mpc_omp_get_thread_tls();
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

static inline mpc_omp_thread_t *
mpc_omp_thread_tls_store(mpc_omp_thread_t *new_value) {
  mpc_omp_tls = (void *)new_value;
  return new_value;
}

static inline mpc_omp_thread_t *
mpc_omp_thread_tls_swap(mpc_omp_thread_t *new_value) {
  mpc_omp_thread_t *old = (mpc_omp_thread_t *)mpc_omp_tls;
  mpc_omp_tls = (void *)new_value;
  return old;
}

static inline mpc_omp_thread_t *mpc_omp_thread_tls_store_first_mvp(void) {
  mpc_omp_thread_t *father = NULL;
  mpc_omp_thread_t *thread = (mpc_omp_thread_t *)mpc_omp_tls;
  assert(thread);
  father = thread->father;
  mpc_omp_tls = (void *)father;
  return father;
}

static inline mpc_omp_thread_t *mpc_omp_thread_tls_store_father(void) {
  mpc_omp_thread_t *father = NULL;
  mpc_omp_thread_t *thread = (mpc_omp_thread_t *)mpc_omp_tls;
  assert(thread);
  father = thread->father;
  mpc_omp_tls = (void *)father;
  return father;
}

static inline mpc_omp_thread_t *mpc_omp_thread_tls_swap_father(void) {
  mpc_omp_thread_t *father = NULL;
  mpc_omp_thread_t *thread = (mpc_omp_thread_t *)mpc_omp_tls;
  assert(thread);
  father = thread->father;
  mpc_omp_tls = (void *)father;
  return thread;
}

#endif /* __MPCOMP_OPENMP_THREAD_TLS_H__ */
