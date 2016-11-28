
#ifndef __MPCOMP_INTEL_PARALLEL_H__
#define __MPCOMP_INTEL_PARALLEL_H__

#include "mpcomp_intel_types.h"

kmp_int32 __kmpc_ok_to_fork(ident_t* );
void __kmpc_fork_call(ident_t*, kmp_int32, kmpc_micro, ...);
void __kmpc_serialized_parallel(ident_t*, kmp_int32); 
void __kmpc_end_serialized_parallel(ident_t *, kmp_int32); 
void __kmpc_push_num_threads( ident_t *, kmp_int32, kmp_int32);
void __kmpc_push_num_teams(ident_t *, kmp_int32, kmp_int32, kmp_int32 );
void __kmpc_fork_teams( ident_t *, kmp_int32, kmpc_micro, ... );
#endif /* __MPCOMP_INTEL_PARALLEL_H__ */
