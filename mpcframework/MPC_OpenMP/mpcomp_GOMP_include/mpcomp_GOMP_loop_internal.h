#ifndef __MPCOMP_GOMP_LOOP_INTERNAL_H__
#define __MPCOMP_GOMP_LOOP_INTERNAL_H__

#include <stdbool.h>

void mpcomp_internal_GOMP_loop_end(void);
void mpcomp_internal_GOMP_loop_end_nowait(void);

/* LOOP STANDARD */
bool mpcomp_internal_GOMP_loop_static_start(long start, long end, long incr,
                                            long chunk_size, long *istart,
                                            long *iend);

bool mpcomp_internal_GOMP_loop_dynamic_start(long start, long end, long incr,
                                             long chunk_size, long *istart,
                                             long *iend);

bool mpcomp_internal_GOMP_loop_guided_start(long start, long end, long incr,
                                            long chunk_size, long *istart,
                                            long *iend);

bool mpcomp_internal_GOMP_loop_runtime_start(long start, long end, long incr,
                                             long *istart, long *iend);

/* LOOP ORDERED */
bool mpcomp_internal_GOMP_loop_ordered_static_start(long start, long end,
                                                    long incr, long chunk_size,
                                                    long *istart, long *iend);

bool mpcomp_internal_GOMP_loop_ordered_dynamic_start(long start, long end,
                                                     long incr, long chunk_size,
                                                     long *istart, long *iend);

bool mpcomp_internal_GOMP_loop_ordered_guided_start(long start, long end,
                                                    long incr, long chunk_size,
                                                    long *istart, long *iend);

bool mpcomp_internal_GOMP_loop_ordered_runtime_start(long start, long end,
                                                     long incr, long *istart,
                                                     long *iend);

#endif /* __MPCOMP_GOMP_LOOP_INTERNAL_H__ */
