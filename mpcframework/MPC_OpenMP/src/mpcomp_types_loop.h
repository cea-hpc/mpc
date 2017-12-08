#ifndef __MPCOMP_TYPES_LOOP_H__
#define __MPCOMP_TYPES_LOOP_H__

#include "sctk_debug.h"

typedef enum mpcomp_loop_gen_type_e {
  MPCOMP_LOOP_TYPE_LONG,
  MPCOMP_LOOP_TYPE_ULL,
} mpcomp_loop_gen_type_t;

typedef struct mpcomp_loop_long_iter_s {
  bool up;
  long lb;         /* Lower bound          */
  long b;          /* Upper bound          */
  long incr;       /* Step                 */
  long chunk_size; /* Size of each chunk   */
  long cur_ordered_iter;
} mpcomp_loop_long_iter_t;

typedef struct mpcomp_loop_ull_iter_s {
  bool up;
  unsigned long long lb;         /* Lower bound              */
  unsigned long long b;          /* Upper bound              */
  unsigned long long incr;       /* Step                     */
  unsigned long long chunk_size; /* Size of each chunk       */
  unsigned long long cur_ordered_iter;
} mpcomp_loop_ull_iter_t;

typedef union mpcomp_loop_gen_iter_u {
  mpcomp_loop_ull_iter_t mpcomp_ull;
  mpcomp_loop_long_iter_t mpcomp_long;
} mpcomp_loop_gen_iter_t;

typedef struct mpcomp_loop_gen_info_s {
  int fresh;
  int ischunked;
  mpcomp_loop_gen_type_t type;
  mpcomp_loop_gen_iter_t loop;
} mpcomp_loop_gen_info_t;

static inline void
__mpcomp_loop_gen_infos_init(mpcomp_loop_gen_info_t *loop_infos, long lb,
                             long b, long incr, long chunk_size) {
  sctk_assert(loop_infos);

  loop_infos->fresh = true;
  loop_infos->ischunked = (chunk_size) ? 1 : 0;
  loop_infos->type = MPCOMP_LOOP_TYPE_LONG;
  loop_infos->loop.mpcomp_long.up = (incr > 0);
  loop_infos->loop.mpcomp_long.b = b;
  loop_infos->loop.mpcomp_long.lb = lb;
  loop_infos->loop.mpcomp_long.incr = incr;
  /* Automatic chunk size -> at most one chunk */
  loop_infos->loop.mpcomp_long.chunk_size = (chunk_size) ? chunk_size : 1;
}

static inline void
__mpcomp_loop_gen_infos_init_ull(mpcomp_loop_gen_info_t *loop_infos,
                                 unsigned long long lb, unsigned long long b,
                                 unsigned long long incr,
                                 unsigned long long chunk_size) {
  sctk_assert(loop_infos);

  loop_infos->fresh = true;
  loop_infos->ischunked = (chunk_size) ? 1 : 0;
  loop_infos->type = MPCOMP_LOOP_TYPE_ULL;
  loop_infos->loop.mpcomp_ull.up = (incr > 0);
  loop_infos->loop.mpcomp_ull.b = b;
  loop_infos->loop.mpcomp_ull.lb = lb;
  loop_infos->loop.mpcomp_ull.incr = incr;
  /* Automatic chunk size -> at most one chunk */
  loop_infos->loop.mpcomp_ull.chunk_size = (chunk_size) ? chunk_size : 1;
}

static inline void
__mpcomp_loop_gen_loop_infos_cpy(mpcomp_loop_gen_info_t *in,
                                 mpcomp_loop_gen_info_t *out) {
  memcpy(out, in, sizeof(mpcomp_loop_gen_info_t));
}

static inline void
__mpcomp_loop_gen_loop_infos_reset(mpcomp_loop_gen_info_t *loop) {
  memset(loop, 0, sizeof(mpcomp_loop_gen_info_t));
}

#endif /* __MPCOMP_TYPES_LOOP_H__ */
