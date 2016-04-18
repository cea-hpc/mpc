#ifndef __MPCOMP_GOMP_PARALLEL_H__
#define __MPCOMP_GOMP_PARALLEL_H__

#include "mpcomp_internal.h"

void __mpcomp_internal_GOMP_start_parallel_region(void (*fn) (void *), void *data, unsigned num_threads);
void __mpcomp_internal_GOMP_end_parallel_region(void);
void __mpcomp_internal_GOMP_parallel_loop_generic_start (void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr, long chunk_size, long combined_pragma);
void __mpcomp_internal_GOMP_parallel_loop_guided_start (void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr, long chunk_size);
void __mpcomp_GOMP_parallel_loop_runtime_start (void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr);
void __mpcomp_internal_GOMP_parallel_sections_start(void (*fn) (void *), void *data, unsigned num_threads, unsigned count);

#endif /* __MPCOMP_GOMP_PARALLEL_H__ */
