#ifndef __MPCOMP_GOMP_LOOP_ULL_INTERNAL_H__
#define __MPCOMP_GOMP_LOOP_ULL_INTERNAL_H__

#include <stdbool.h>

bool mpcomp_internal_GOMP_loop_ull_static_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend);

bool mpcomp_internal_GOMP_loop_ull_guided_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend);


bool mpcomp_internal_GOMP_loop_ull_dynamic_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend);


bool mpcomp_internal_GOMP_loop_ull_guided_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend);

bool mpcomp_internal_GOMP_loop_ull_static_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend);

bool mpcomp_internal_GOMP_loop_ull_ordered_static_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend);


bool mpcomp_internal_GOMP_loop_ull_ordered_dynamic_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend);


bool mpcomp_internal_GOMP_loop_ull_ordered_guided_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend);

#endif /* __MPCOMP_GOMP_LOOP_ULL_INTERNAL_H__ */
