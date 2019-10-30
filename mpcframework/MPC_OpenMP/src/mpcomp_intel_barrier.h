
#ifndef __MPCOMP_INTEL_BARRIER_H__
#define __MPCOMP_INTEL_BARRIER_H__

#include "mpcomp_intel_types.h"

void __kmpc_barrier(ident_t *, kmp_int32);
kmp_int32 __kmpc_barrier_master(ident_t *, kmp_int32);
void __kmpc_end_barrier_master(ident_t *, kmp_int32);
kmp_int32 __kmpc_barrier_master_nowait(ident_t *, kmp_int32);

#endif /* __MPCOMP_INTEL_BARRIER_H__ */
