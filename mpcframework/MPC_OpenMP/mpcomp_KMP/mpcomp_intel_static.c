#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_global.h"
#include "mpcomp_intel_static.h"

#include "mpcomp_loop.h"

void __kmpc_for_static_init_4(ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
                              kmp_int32 *plastiter, kmp_int32 *plower,
                              kmp_int32 *pupper, kmp_int32 *pstride,
                              kmp_int32 incr, kmp_int32 chunk) {
  long from, to, chunk_num;
  mpcomp_thread_t *t;
  int res;
  // save old pupper
  kmp_int32 pupper_old = *pupper;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->info.num_threads == 1) {
    if (plastiter != NULL)
      *plastiter = TRUE;
    *pstride =
        (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));
    return;
  }

  // trip_count computation
  sctk_nodebug("[%d] %s: <%s> "
               "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
               "*plastiter=%d *pstride=%d",
               t->rank, __func__, loc->psource, schedtype, *plastiter, *plower,
               *pupper, *pstride, incr, chunk, *plastiter, *pstride);

  switch (schedtype) {
  case kmp_sch_static: {
    chunk_num = __kmpc_for_static_init_no_chunk_long(*plower, *pupper + incr,
                                                     incr, &from, &to);
    if (plastiter != NULL) {
      *plastiter =
          __kmpc_for_static_islastiter(t->rank, t->info.num_threads, chunk_num,
                                       (long)*pupper, from, to, incr);
    }
    *pupper = (kmp_int32)to;
    *plower = (kmp_int32)from;
    break;
  }
  case kmp_sch_static_chunked: {
    chunk = (chunk < 1) ? 1 : chunk;
    chunk_num = mpcomp_intel_compute_trip_count(*plower, *pupper + incr, incr);
    *pstride = (chunk * incr) * t->info.num_threads;
    *plower = *plower + ((chunk * incr) * t->rank);
    *pupper = *plower + (chunk * incr) - incr;
    // plastiter computation
    if (plastiter != NULL)
      *plastiter = (t->rank == ((chunk_num - 1) / chunk) % t->info.num_threads);
    break;
  }
  default:
    not_implemented();
    break;
  }
}

void __kmpc_for_static_init_4u(ident_t *loc, kmp_int32 gtid,
                               kmp_int32 schedtype, kmp_int32 *plastiter,
                               kmp_uint32 *plower, kmp_uint32 *pupper,
                               kmp_int32 *pstride, kmp_int32 incr,
                               kmp_int32 chunk) {
  unsigned long long from, to;
  mpcomp_thread_t *t;
  int res;
  kmp_uint32 trip_count;
  mpcomp_loop_gen_info_t *loop_infos;
  // save old pupper
  kmp_uint32 pupper_old = *pupper;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->info.num_threads == 1) {
    if (plastiter != NULL)
      *plastiter = TRUE;
    *pstride =
        (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));

    return;
  }

  loop_infos = &(t->info.loop_infos);
  __mpcomp_loop_gen_infos_init_ull(
      loop_infos, (unsigned long long)*plower,
      (unsigned long long)*pupper + (unsigned long long)incr,
      (unsigned long long)incr, (unsigned long long)chunk, (incr > 0));

  // trip_count computation
  if (incr == 1) {
    trip_count = *pupper - *plower + 1;
  } else if (incr == -1) {
    trip_count = *plower - *pupper + 1;
  } else {
    if (incr > 1) {
      trip_count = (*pupper - *plower) / incr + 1;
    } else {
      trip_count = (*plower - *pupper) / (-incr) + 1;
    }
  }

  sctk_nodebug("[%d] __kmpc_for_static_init_4u: <%s> "
               "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
               "*plastiter=%d *pstride=%d",
               ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, loc->psource,
               schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
               *plastiter, *pstride);

  switch (schedtype) {
  case kmp_sch_static:
    /* Get the single chunk for the current thread */
    res = __mpcomp_static_schedule_get_single_chunk_ull(
        &(loop_infos->loop.mpcomp_ull), incr, &from, &to);

    /* Chunk to execute? */
    if (res) {
      sctk_nodebug(
          "[%d] Results for __kmpc_for_static_init_4u (kmp_sch_static): "
          "%ld -> %ld excl %ld incl [%d]",
          ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, from, to,
          to - incr, incr);

      /* Remarks:
          - MPC chunk has not-inclusive upper bound while Intel runtime includes
          upper bound for calculation
          - Need to cast from long to int because MPC handles everything has a
         long
          (like GCC) while Intel creates different functions
          */
      *plower = (kmp_uint32)from;
      *pupper = (kmp_uint32)to - incr;

    } else {
      /* No chunk */
      *pupper = *pupper + incr;
      *plower = *pupper;
    }

    // plastiter computation
    if (trip_count < t->info.num_threads) {
      if (plastiter != NULL)
        *plastiter = (t->rank == trip_count - 1);
    } else {
      if (incr > 0) {
        if (plastiter != NULL)
          *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
      } else {
        if (plastiter != NULL)
          *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
      }
    }

    break;
  case kmp_sch_static_chunked:
    if (chunk < 1)
      chunk = 1;

    *pstride = (chunk * incr) * t->info.num_threads;
    *plower = *plower + ((chunk * incr) * t->rank);
    *pupper = *plower + (chunk * incr) - incr;

    // plastiter computation
    if (plastiter != NULL)
      *plastiter =
          (t->rank == ((trip_count - 1) / chunk) % t->info.num_threads);

    /* Remarks:
        - MPC chunk has not-inclusive upper bound while Intel runtime includes
        upper bound for calculation
        - Need to cast from long to int because MPC handles everything has a
       long
        (like GCC) while Intel creates different functions
        */

    sctk_nodebug("[%d] Results: "
                 "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
                 t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
                 *plastiter);

    break;
  default:
    not_implemented();
    break;
  }
}

void __kmpc_for_static_init_8(ident_t *loc, kmp_int32 gtid, kmp_int32 schedtype,
                              kmp_int32 *plastiter, kmp_int64 *plower,
                              kmp_int64 *pupper, kmp_int64 *pstride,
                              kmp_int64 incr, kmp_int64 chunk) {
  long from, to;
  mpcomp_thread_t *t;
  int res;
  kmp_int64 trip_count;
  // save old pupper
  kmp_int64 pupper_old = *pupper;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->info.num_threads == 1) {
    if (plastiter != NULL)
      *plastiter = TRUE;
    *pstride =
        (incr > 0) ? (*pupper - *plower + 1) : (-(*plower - *pupper + 1));

    return;
  }

  // trip_count computation
  if (incr == 1) {
    trip_count = *pupper - *plower + 1;
  } else if (incr == -1) {
    trip_count = *plower - *pupper + 1;
  } else {
    if (incr > 1) {
      trip_count = (*pupper - *plower) / incr + 1;
    } else {
      trip_count = (*plower - *pupper) / (-incr) + 1;
    }
  }

  sctk_nodebug("[%d] __kmpc_for_static_init_8: <%s> "
               "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
               "*plastiter=%d *pstride=%d",
               ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, loc->psource,
               schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
               *plastiter, *pstride);

  switch (schedtype) {
  case kmp_sch_static:
    /* Get the single chunk for the current thread */
    res = __mpcomp_static_schedule_get_single_chunk(*plower, *pupper + incr,
                                                    incr, &from, &to);

    /* Chunk to execute? */
    if (res) {
      sctk_nodebug(
          "[%d] Results for __kmpc_for_static_init_8 (kmp_sch_static): "
          "%ld -> %ld excl %ld incl [%d]",
          ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, from, to,
          to - incr, incr);

      /* Remarks:
          - MPC chunk has not-inclusive upper bound while Intel runtime includes
          upper bound for calculation
          - Need to cast from long to int because MPC handles everything has a
         long
          (like GCC) while Intel creates different functions
          */
      *plower = (kmp_int64)from;
      *pupper = (kmp_int64)to - incr;

    } else {
      /* No chunk */
      *pupper = *pupper + incr;
      *plower = *pupper;
    }

    // plastiter computation
    if (trip_count < t->info.num_threads) {
      if (plastiter != NULL)
        *plastiter = (t->rank == trip_count - 1);
    } else {
      if (incr > 0) {
        if (plastiter != NULL)
          *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
      } else {
        if (plastiter != NULL)
          *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
      }
    }

    break;
  case kmp_sch_static_chunked:
    if (chunk < 1)
      chunk = 1;

    *pstride = (chunk * incr) * t->info.num_threads;
    *plower = *plower + ((chunk * incr) * t->rank);
    *pupper = *plower + (chunk * incr) - incr;

    // plastiter computation
    if (plastiter != NULL)
      *plastiter =
          (t->rank == ((trip_count - 1) / chunk) % t->info.num_threads);

    /* Remarks:
        - MPC chunk has not-inclusive upper bound while Intel runtime includes
        upper bound for calculation
        - Need to cast from long to int because MPC handles everything has a
       long
        (like GCC) while Intel creates different functions
        */

    sctk_nodebug("[%d] Results: "
                 "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
                 t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
                 *plastiter);

    break;
  default:
    not_implemented();
    break;
  }
}

void __kmpc_for_static_init_8u(ident_t *loc, kmp_int32 gtid,
                               kmp_int32 schedtype, kmp_int32 *plastiter,
                               kmp_uint64 *plower, kmp_uint64 *pupper,
                               kmp_int64 *pstride, kmp_int64 incr,
                               kmp_int64 chunk) {
  /* TODO: the same as unsigned long long in GCC... */
  sctk_nodebug(
      "__kmpc_for_static_init_8u: siweof long = %d, sizeof long long %d",
      sizeof(long), sizeof(long long));
  unsigned long long from, to;
  mpcomp_thread_t *t;
  int res;
  kmp_uint64 trip_count;
  mpcomp_loop_gen_info_t *loop_infos;
  // save old pupper
  kmp_uint64 pupper_old = *pupper;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->info.num_threads == 1) {
    if (plastiter != NULL)
      *plastiter = TRUE;

    if (incr > 0) {
      *pstride = (*pupper - *plower + 1);
      if (*pstride < incr)
        *pstride = incr;
    } else {
      *pstride = ((-1) * (*plower - *pupper + 1));
      if (*pstride > incr)
        *pstride = incr;
    }
    return;
  }

  loop_infos = &(t->info.loop_infos);
  __mpcomp_loop_gen_infos_init_ull(
      loop_infos, (unsigned long long)*plower,
      (unsigned long long)*pupper + (unsigned long long)incr,
      (unsigned long long)incr, (unsigned long long)chunk, (incr > 0));

  // trip_count computation
  if (incr == 1) {
    trip_count = *pupper - *plower + 1;
  } else if (incr == -1) {
    trip_count = *plower - *pupper + 1;
  } else {
    if (incr > 1) {
      trip_count = (*pupper - *plower) / incr + 1;
    } else {
      trip_count = (*plower - *pupper) / (-incr) + 1;
    }
  }

  sctk_nodebug("[%d] __kmpc_for_static_init_8u: <%s> "
               "schedtype=%d, %d? %d -> %d incl. [%d], incr=%d chunk=%d "
               "*plastiter=%d *pstride=%d",
               ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, loc->psource,
               schedtype, *plastiter, *plower, *pupper, *pstride, incr, chunk,
               *plastiter, *pstride);

  switch (schedtype) {
  case kmp_sch_static:
    /* Get the single chunk for the current thread */
    res = __mpcomp_static_schedule_get_single_chunk_ull(
        &(loop_infos->loop.mpcomp_ull), &from, &to);

    /* Chunk to execute? */
    if (res) {
      sctk_nodebug(
          "[%d] Results for __kmpc_for_static_init_8u (kmp_sch_static): "
          "%ld -> %ld excl %ld incl [%d]",
          ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, from, to,
          to - incr, incr);

      /* Remarks:
          - MPC chunk has not-inclusive upper bound while Intel runtime includes
          upper bound for calculation
          - Need to cast from long to int because MPC handles everything has a
         long
          (like GCC) while Intel creates different functions
          */
      *plower = from;
      *pupper = to - incr;

    } else {
      /* No chunk */
      *pupper = *pupper + incr;
      *plower = *pupper;
    }

    // plastiter computation
    if (trip_count < t->info.num_threads) {
      if (plastiter != NULL)
        *plastiter = (t->rank == trip_count - 1);
    } else {
      if (incr > 0) {
        if (plastiter != NULL)
          *plastiter = *plower <= pupper_old && *pupper > pupper_old - incr;
      } else {
        if (plastiter != NULL)
          *plastiter = *plower >= pupper_old && *pupper < pupper_old - incr;
      }
    }

    break;
  case kmp_sch_static_chunked:
    if (chunk < 1)
      chunk = 1;

    *pstride = (chunk * incr) * t->info.num_threads;
    *plower = *plower + ((chunk * incr) * t->rank);
    *pupper = *plower + (chunk * incr) - incr;

    // plastiter computation
    if (plastiter != NULL)
      *plastiter =
          (t->rank == ((trip_count - 1) / chunk) % t->info.num_threads);

    /* Remarks:
        - MPC chunk has not-inclusive upper bound while Intel runtime includes
        upper bound for calculation
        - Need to cast from long to int because MPC handles everything has a
       long
        (like GCC) while Intel creates different functions
        */

    sctk_nodebug("[%d] Results: "
                 "%ld -> %ld excl %ld incl [%d], trip=%d, plastiter = %d",
                 t->rank, *plower, *pupper, *pupper - incr, incr, trip_count,
                 *plastiter);

    break;
  default:
    not_implemented();
    break;
  }
}

void __kmpc_for_static_fini(ident_t *loc, kmp_int32 global_tid) {
  sctk_nodebug("[REDIRECT KMP]: %s -> None", __func__);
}
