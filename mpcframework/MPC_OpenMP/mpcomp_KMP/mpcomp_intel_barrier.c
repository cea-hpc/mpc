
#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_barrier.h"
#include "mpcomp_intel_types.h"
#include "mpcomp_intel_barrier.h"

void __kmpc_barrier(ident_t *loc, kmp_int32 global_tid) 
{
    mpcomp_thread_t* t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert( t );
    sctk_nodebug( "[%d] %s: entering...", t->rank, __func__ );
    __mpcomp_barrier() ;
}

kmp_int32 __kmpc_barrier_master(ident_t *loc, kmp_int32 global_tid) 
{
  not_implemented() ;
  return (kmp_int32) 0;
}

void __kmpc_end_barrier_master(ident_t *loc, kmp_int32 global_tid) 
{
  not_implemented() ;
}

kmp_int32 __kmpc_barrier_master_nowait(ident_t *loc, kmp_int32 global_tid)
{
  not_implemented() ;
  return (kmp_int32) 0;
}

