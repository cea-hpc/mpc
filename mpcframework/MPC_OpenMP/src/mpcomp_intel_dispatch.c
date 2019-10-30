#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_dispatch.h"

void __kmpc_dispatch_init_4(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid,
                            enum sched_type schedule, kmp_int32 lb,
                            kmp_int32 ub, kmp_int32 st, kmp_int32 chunk) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);

  sctk_nodebug("[%d] %s: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d",
               t->rank, __func__, lb, ub, ub + st, st, chunk, schedule);

  /* add to sync with MPC runtime bounds */
  const long add = ((ub - lb) % st == 0) ? st : st - ((ub - lb) % st);
  const long b = (long)ub + add;

  t->schedule_type = schedule;
  t->schedule_is_forced = 1;
  __kmpc_dispatch_init_mpcomp_long(t, lb, b, (long)st, (long)chunk);
}

void __kmpc_dispatch_init_4u(__UNUSED__ ident_t *loc,__UNUSED__  kmp_int32 gtid,
                             enum sched_type schedule, kmp_uint32 lb,
                             kmp_uint32 ub, kmp_int32 st, kmp_int32 chunk) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);

  sctk_nodebug(
      "[%d] %s: enter %llu -> %llud incl, %llu excl [%llu] ck:%llud sch:%d",
      t->rank, __func__, lb, ub, ub + st, st, chunk, schedule);

  /* add to sync with MPC runtime bounds */
  const long long add = ((ub - lb) % st == 0) ? st : st - (kmp_int32)((ub - lb) % st);
  const unsigned long long b = (unsigned long long)ub + add;

  const bool up = (st > 0);
  const unsigned long long st_ull = (up) ? st : -st;

  t->schedule_type = schedule;
  t->schedule_is_forced = 1;
  __kmpc_dispatch_init_mpcomp_ull(t, up, lb, b, (unsigned long long)st_ull,
                                  (unsigned long long)chunk);
}

void __kmpc_dispatch_init_8(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid,
                            enum sched_type schedule, kmp_int64 lb,
                            kmp_int64 ub, kmp_int64 st, kmp_int64 chunk) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);
  sctk_nodebug("[%d] %s: enter %d -> %d incl, %d excl [%d] ck:%d sch:%d",
               t->rank, __func__, lb, ub, ub + st, st, chunk, schedule);

  /* add to sync with MPC runtime bounds */
  const long add = ((ub - lb) % st == 0) ? st : st - ((ub - lb) % st);
  const long b = (long)ub + add;

  t->schedule_type = schedule;
  t->schedule_is_forced = 1;
  __kmpc_dispatch_init_mpcomp_long(t, lb, b, (long)st, (long)chunk);
}

void __kmpc_dispatch_init_8u(__UNUSED__ ident_t *loc,__UNUSED__  kmp_int32 gtid,
                             enum sched_type schedule, kmp_uint64 lb,
                             kmp_uint64 ub, kmp_int64 st, kmp_int64 chunk) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t);

  sctk_nodebug(
      "[%d] %s: enter %llu -> %llud incl, %llu excl [%llu] ck:%llud sch:%d",
      t->rank, __func__, lb, ub, ub + st, st, chunk, schedule);

  /* add to sync with MPC runtime bounds */
  const bool up = (st > 0);
  const unsigned long long st_ull = (up) ? st : -st;

  const long long add = ((ub - lb) % st == 0) ? st : st - (kmp_int32)((ub - lb) % st);
  const unsigned long long b = (unsigned long long)ub + add;
  t->schedule_type = schedule;
  t->schedule_is_forced = 1;
  __kmpc_dispatch_init_mpcomp_ull(t, up, lb, b, (unsigned long long)st_ull,
                                  (unsigned long long)chunk);
}

int __kmpc_dispatch_next_4(ident_t *loc, kmp_int32 gtid, __UNUSED__ kmp_int32 *p_last,
                           kmp_int32 *p_lb, kmp_int32 *p_ub, __UNUSED__  kmp_int32 *p_st) {
  long from, to;
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  sctk_assert(t->info.loop_infos.type == MPCOMP_LOOP_TYPE_LONG);
  const long incr = t->info.loop_infos.loop.mpcomp_long.incr;

  sctk_nodebug("%s: p_lb %ld, p_ub %ld, p_st %ld", __func__, *p_lb, *p_ub,
               *p_st);
  const int ret = __kmp_dispatch_next_mpcomp_long(t, &from, &to);

  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel)
   */
  *p_lb = (kmp_int32)from;
  *p_ub = (kmp_int32)to - (kmp_int32)incr;

  if (!ret) {
    __kmpc_dispatch_fini_mpcomp_gen(loc, gtid);
  }

  return ret;
}

int __kmpc_dispatch_next_4u(ident_t *loc, kmp_int32 gtid, __UNUSED__ kmp_int32 *p_last,
                            kmp_uint32 *p_lb, kmp_uint32 *p_ub,
                            __UNUSED__ kmp_int32 *p_st) {
  mpcomp_thread_t *t;
  unsigned long long from, to;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  sctk_assert(t->info.loop_infos.type == MPCOMP_LOOP_TYPE_ULL);
  const unsigned long long incr = t->info.loop_infos.loop.mpcomp_ull.incr;

  sctk_nodebug("%s: p_lb %llu, p_ub %llu, p_st %llu", __func__, *p_lb, *p_ub,
               *p_st);

  const int ret = __kmp_dispatch_next_mpcomp_ull(t, &from, &to);

  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel)
   */
  *p_lb = (kmp_uint32)from;
  *p_ub = (kmp_uint32)to - (kmp_uint32)incr;

  if (!ret) {
    __kmpc_dispatch_fini_mpcomp_gen(loc, gtid);
  }
  return ret;
}

int __kmpc_dispatch_next_8(ident_t *loc, kmp_int32 gtid, __UNUSED__ kmp_int32 *p_last,
                           kmp_int64 *p_lb, kmp_int64 *p_ub, __UNUSED__ kmp_int64 *p_st) {
  long from, to;
  mpcomp_thread_t *t;
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  sctk_assert(t->info.loop_infos.type == MPCOMP_LOOP_TYPE_LONG);
  const long incr = t->info.loop_infos.loop.mpcomp_long.incr;

  sctk_nodebug("%s: p_lb %ld, p_ub %ld, p_st %ld", __func__, *p_lb, *p_ub,
               *p_st);

  const int ret = __kmp_dispatch_next_mpcomp_long(t, &from, &to);

  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel)
   */
  *p_lb = (kmp_int64)from;
  *p_ub = (kmp_int64)to - (kmp_int64)incr;

  if (!ret) {
    __kmpc_dispatch_fini_mpcomp_gen(loc, gtid);
  }
  return ret;
}

int __kmpc_dispatch_next_8u(ident_t *loc, kmp_int32 gtid, __UNUSED__ kmp_int32 *p_last,
                            kmp_uint64 *p_lb, kmp_uint64 *p_ub,
                            __UNUSED__ kmp_int64 *p_st) {
  mpcomp_thread_t *t;
  unsigned long long from, to;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  sctk_assert(t->info.loop_infos.type == MPCOMP_LOOP_TYPE_ULL);
  const unsigned long long incr = t->info.loop_infos.loop.mpcomp_ull.incr;

  sctk_nodebug("%s: p_lb %llu, p_ub %llu, p_st %llu", __func__, *p_lb, *p_ub,
               *p_st);
  const int ret = __kmp_dispatch_next_mpcomp_ull(t, &from, &to);

  /* sync with intel runtime (A->B excluded with gcc / A->B included with intel)
   */
  *p_lb = from;
  *p_ub = to - (kmp_uint64)incr;

  if (!ret) {
    __kmpc_dispatch_fini_mpcomp_gen(loc, gtid);
  }
  return ret;
}

void __kmpc_dispatch_fini_4(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid) {}

void __kmpc_dispatch_fini_8(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid) {}

void __kmpc_dispatch_fini_4u(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid) {}

void __kmpc_dispatch_fini_8u(__UNUSED__ ident_t *loc, __UNUSED__ kmp_int32 gtid) {}
