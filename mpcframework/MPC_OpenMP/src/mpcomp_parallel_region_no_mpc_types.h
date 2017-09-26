
#ifndef __MPCOMP_PARALLEL_REGION_NOMPC_H__
#define __MPCOMP_PARALLEL_REGION_NOMPC_H__

void __mpcomp_start_parallel_region(void (*func)(void *), void *shared,
                                    unsigned arg_num_threads);

void __mpcomp_start_sections_parallel_region(void (*func)(void *), void *shared,
                                             unsigned arg_num_threads,
                                             unsigned nb_sections);

void __mpcomp_start_parallel_dynamic_loop(void (*func)(void *), void *shared,
                                          unsigned arg_num_threads, long lb,
                                          long b, long incr, long chunk_size);

void __mpcomp_start_parallel_static_loop(void (*func)(void *), void *shared,
                                         unsigned arg_num_threads, long lb,
                                         long b, long incr, long chunk_size);

void __mpcomp_start_parallel_guided_loop(void (*func)(void *), void *shared,
                                         unsigned arg_num_threads, long lb,
                                         long b, long incr, long chunk_size);

void __mpcomp_start_parallel_runtime_loop(void (*func)(void *), void *shared,
                                          unsigned arg_num_threads, long lb,
                                          long b, long incr, long chunk_size);

#endif /* __MPCOMP_PARALLEL_REGION_NOMPC_H__ */

