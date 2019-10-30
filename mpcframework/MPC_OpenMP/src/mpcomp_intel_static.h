
#ifndef __MPCOMP_INTEL_FOR_STATIC_H__
#define __MPCOMP_INTEL_FOR_STATIC_H__

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_loop_static.h"

#if 0
void __kmpc_for_static_init_4( ident_t*,kmp_int32, kmp_int32, kmp_int32*, kmp_uint32*, kmp_uint32*, kmp_int32*, kmp_int32, kmp_int32); 
void __kmpc_for_static_init_4u( ident_t*,kmp_int32, kmp_int32, kmp_int32*, kmp_uint32*, kmp_uint32*, kmp_int32*, kmp_int32, kmp_int32); 
void __kmpc_for_static_init_8( ident_t*,kmp_int32, kmp_int32, kmp_int32*, kmp_uint32*, kmp_uint64*, kmp_int64*, kmp_int64, kmp_int64); 
void __kmpc_for_static_init_8u( ident_t*,kmp_int32, kmp_int32, kmp_int32*, kmp_uint32*, kmp_uint64*, kmp_int64*, kmp_int64, kmp_int64);
#endif

void __kmpc_for_static_fini(ident_t *, kmp_int32);


#endif /* __MPCOMP_INTEL_FOR_STATIC_H__ */
