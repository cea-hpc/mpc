
#include "sctk_debug.h"
#include "mpcomp_api.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"

kmp_int32 __kmpc_global_thread_num(ident_t *loc) {
  sctk_nodebug("[REDIRECT KMP]: %s -> omp_get_max_threads", __func__);
  return (kmp_int32)omp_get_thread_num();
}

kmp_int32 __kmpc_global_num_threads(ident_t *loc) {
  sctk_nodebug("[REDIRECT KMP]: %s -> omp_get_num_threads", __func__);
  return (kmp_int32)omp_get_num_threads();
}

kmp_int32 __kmpc_bound_thread_num(ident_t *loc) {
  sctk_nodebug("[REDIRECT KMP]: %s -> omp_get_thread_num", __func__);
  return (kmp_int32)omp_get_thread_num();
}

kmp_int32 __kmpc_bound_num_threads(ident_t *loc) {
  sctk_nodebug("[REDIRECT KMP]: %s -> omp_get_num_threads", __func__);
  return (kmp_int32)omp_get_num_threads();
}

kmp_int32 __kmpc_in_parallel(ident_t *loc) {
  sctk_nodebug("[REDIRECT KMP]: %s -> omp_in_parallel", __func__);
  return (kmp_int32)omp_in_parallel();
}

void __kmpc_flush(ident_t *loc, ...) {
  /* TODO depending on the compiler, call the right function
   * Maybe need to do the same for mpcomp_flush...
   */
  __sync_synchronize();
}
