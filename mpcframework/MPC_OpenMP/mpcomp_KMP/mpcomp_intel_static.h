
#ifndef __MPCOMP_INTEL_FOR_STATIC_H__
#define __MPCOMP_INTEL_FOR_STATIC_H__

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"

#if 0
void __kmpc_for_static_init_4( ident_t*,kmp_int32, kmp_int32, kmp_int32*, kmp_uint32*, kmp_uint32*, kmp_int32*, kmp_int32, kmp_int32); 
void __kmpc_for_static_init_4u( ident_t*,kmp_int32, kmp_int32, kmp_int32*, kmp_uint32*, kmp_uint32*, kmp_int32*, kmp_int32, kmp_int32); 
void __kmpc_for_static_init_8( ident_t*,kmp_int32, kmp_int32, kmp_int32*, kmp_uint32*, kmp_uint64*, kmp_int64*, kmp_int64, kmp_int64); 
void __kmpc_for_static_init_8u( ident_t*,kmp_int32, kmp_int32, kmp_int32*, kmp_uint32*, kmp_uint64*, kmp_int64*, kmp_int64, kmp_int64);
#endif

void __kmpc_for_static_fini(ident_t *, kmp_int32);

/* INLINE FUNCTIONS */
static inline kmp_int32
mpcomp_intel_compute_trip_count(kmp_int32 lb, kmp_int32 b, kmp_int32 incr) {
  kmp_int32 ret = 0;
  ret = (incr > 0) ? (b - lb) / incr + 1 : ret;
  ret = (incr < 0) ? (lb - b) / (-incr) + 1 : ret;
  return ret;
}

static inline long __kmpc_for_static_init_no_chunk_long(long lb, long b,
                                                        long incr, long *from,
                                                        long *to) {
  const kmp_int32 trip_count = mpcomp_intel_compute_trip_count(lb, b, incr);
  const long chunk_num =
      __mpcomp_static_schedule_get_single_chunk(lb, b, incr, from, to);

  if (chunk_num) /* CHUNK TO EXECUTE */
  {
    /*
 * - MPC chunk has not-inclusive upper bound while Intel runtime includes
       upper bound for calculation
 */
    *to = *to - incr;
  } else /* NO CHUNK TO EXECUTE */
  {
    *to = *to + incr;
    *from = *to;
  }
  return chunk_num;
}

static inline long __kmpc_for_static_islastiter(long rank, long thread_num,
                                                long chunk_num, long old_to,
                                                long new_from, long new_to,
                                                long incr) {
  long isLastIter = 0;

  if (chunk_num < thread_num) {
    isLastIter = (rank == chunk_num - 1);
  } else {
    isLastIter =
        (incr > 0) ? new_from <= old_to && new_to > old_to - incr : isLastIter;
    isLastIter =
        (incr < 0) ? new_from >= old_to && new_to < old_to - incr : isLastIter;
  }

  return isLastIter;
}

#endif /* __MPCOMP_INTEL_FOR_STATIC_H__ */
