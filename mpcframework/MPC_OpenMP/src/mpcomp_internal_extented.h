#ifndef __MPCOMP_INTERNAL_EXTENTED_H__
#define __MPCOMP_INTERNAL_EXTENTED_H__

#include "mpcomp_abi.h"

void mpcomp_static_loop_init(mpcomp_thread_t *t, long lb, long b, long incr,
                             long chunk_size);
void mpcomp_static_loop_init_ull(mpcomp_thread_t *t, unsigned long long lb,
                                 unsigned long long b, unsigned long long incr,
                                 unsigned long long chunk_size);

#endif /* __MPCOMP_INTERNAL_EXTENTED_H__ */
