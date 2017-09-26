#include "sctk_debug.h"
#include "mpcomp_single.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_sync.h"

#include "mpcomp_sync.h"
#include "mpcomp_ordered.h"

kmp_int32 __kmpc_master(ident_t *loc, kmp_int32 global_tid) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  sctk_nodebug("[%d] %s: entering", t->rank, __func__);
  return (t->rank == 0) ? (kmp_int32)1 : (kmp_int32)0;
}

void __kmpc_end_master(ident_t *loc, kmp_int32 global_tid) {
  sctk_nodebug("[REDIRECT KMP]: %s -> None", __func__);
}

void __kmpc_ordered(ident_t *loc, kmp_int32 global_tid) {
  sctk_nodebug("[REDIRECT KMP]: %s -> __mpcomp_ordered_begin", __func__);
  __mpcomp_ordered_begin();
}

void __kmpc_end_ordered(ident_t *loc, kmp_int32 global_tid) {
  sctk_nodebug("[REDIRECT KMP]: %s -> __mpcomp_ordered_end", __func__);
  __mpcomp_ordered_end();
}

void __kmpc_critical(ident_t *loc, kmp_int32 global_tid,
                     kmp_critical_name *crit) {
  /* TODO Handle named critical */
  sctk_nodebug("[REDIRECT KMP]: %s -> __mpcomp_anonymous_critical_begin",
               __func__);
  __mpcomp_anonymous_critical_begin();
}

void __kmpc_end_critical(ident_t *loc, kmp_int32 global_tid,
                         kmp_critical_name *crit) {
  /* TODO Handle named critical */
  sctk_nodebug("[REDIRECT KMP]: %s -> __mpcomp_anonymous_critical_end",
               __func__);
  __mpcomp_anonymous_critical_end();
}

kmp_int32 __kmpc_single(ident_t *loc, kmp_int32 global_tid) {
  sctk_nodebug("[REDIRECT KMP]: %s -> __mpcomp_do_single", __func__);
  return (kmp_int32)__mpcomp_do_single();
}

void __kmpc_end_single(ident_t *loc, kmp_int32 global_tid) {
  sctk_nodebug("[REDIRECT KMP]: %s -> None", __func__);
}
