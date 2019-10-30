
#ifndef __MPCOMP_INTEL_SYNC_H__
#define __MPCOMP_INTEL_SYNC_H__

#include "mpcomp_intel_types.h"

kmp_int32 __kmpc_master(ident_t *, kmp_int32);
void __kmpc_end_master(ident_t *, kmp_int32);
void __kmpc_ordered(ident_t *, kmp_int32);
void __kmpc_end_ordered(ident_t *, kmp_int32);
void __kmpc_critical(ident_t *, kmp_int32, kmp_critical_name *);
void __kmpc_end_critical(ident_t *, kmp_int32, kmp_critical_name *);
kmp_int32 __kmpc_single(ident_t *, kmp_int32);
void __kmpc_end_single(ident_t *, kmp_int32);

#endif /* __MPCOMP_INTEL_SYNC_H__ */
