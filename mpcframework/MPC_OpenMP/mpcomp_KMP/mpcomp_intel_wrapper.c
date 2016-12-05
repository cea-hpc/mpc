#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_wrapper.h"

void mpcomp_intel_wrapper_func(void *arg) {
  mpcomp_intel_wrapper_t *w = (mpcomp_intel_wrapper_t *)arg;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(t);
  sctk_assert(w);

  sctk_nodebug("%s: f: %p, argc: %d, args: %p", __func__, w->f, w->argc,
               w->args);
  __kmp_invoke_microtask(w->f, t->rank, 0, w->argc, w->args);
}
