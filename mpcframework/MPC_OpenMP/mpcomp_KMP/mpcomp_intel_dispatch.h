
#ifndef __MPCOMP_INTEL_DISPATCH_H__
#define __MPCOMP_INTEL_DISPATCH_H__

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_intel_types.h"

static inline  void __kmpc_dispatch_init_mpcomp_long(mpcomp_thread_t* t, long lb, long b, long incr, long chunk )
{
  sctk_assert( t );

  switch( t->schedule_type ) 
  {
    /* NORMAL AUTO OR STATIC */
    case kmp_sch_auto:
    case kmp_sch_static:
        chunk = 0;
    case kmp_sch_static_chunked:
        __mpcomp_static_loop_begin( lb, b, incr, chunk, NULL, NULL );
        break ;

    /* ORDERED AUTO OR STATIC */
    case kmp_ord_auto:
    case kmp_ord_static:
        chunk = 0;
    case kmp_ord_static_chunked:
        __mpcomp_ordered_static_loop_begin( lb, b, incr, chunk, NULL, NULL );
        break;

    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        __mpcomp_dynamic_loop_begin( lb, b, incr, chunk, NULL, NULL );
        break ;
    case kmp_ord_dynamic_chunked:
        __mpcomp_ordered_dynamic_loop_begin( lb, b, incr, chunk, NULL, NULL );
        break ;

    case kmp_sch_guided_chunked:
        __mpcomp_guided_loop_begin( lb, b, incr, chunk, NULL, NULL );
        break;
    case kmp_ord_guided_chunked:
        __mpcomp_guided_loop_begin( lb, b, incr, chunk, NULL, NULL );
        break ;

    /* runtime */
    case kmp_sch_runtime:
        __mpcomp_runtime_loop_begin( lb, b, incr, chunk, NULL, NULL );
        break;
    case kmp_ord_runtime:
        __mpcomp_ordered_runtime_loop_begin( lb, b, incr, chunk, NULL, NULL );
        break ; 

    default:
      sctk_error("Schedule not handled: %d\n", t->schedule_type);
      not_implemented() ;
  }
}

static inline  void __kmpc_dispatch_init_mpcomp_ull(mpcomp_thread_t* t, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk )
{
  sctk_assert( t );
  switch( t->schedule_type ) 
  {
    /* NORMAL AUTO OR STATIC */
    case kmp_sch_auto:
    case kmp_sch_static:
        chunk = 0;
    case kmp_sch_static_chunked:
        __mpcomp_static_loop_begin_ull( lb, b, incr, chunk, NULL, NULL );
        break ;

    /* ORDERED AUTO OR STATIC */
    case kmp_ord_auto:
    case kmp_ord_static:
        chunk = 0;
    case kmp_ord_static_chunked:
        __mpcomp_ordered_static_loop_begin_ull( lb, b, incr, chunk, NULL, NULL );
        break;

    /* regular scheduling */
    case kmp_sch_dynamic_chunked:
        __mpcomp_loop_ull_dynamic_begin( lb, b, incr, chunk, NULL, NULL );
        break ;
    case kmp_ord_dynamic_chunked:
        __mpcomp_loop_ull_ordered_dynamic_begin( lb, b, incr, chunk, NULL, NULL );
        break ;

    case kmp_sch_guided_chunked:
        __mpcomp_loop_ull_guided_begin( lb, b, incr, chunk, NULL, NULL );
        break;
    case kmp_ord_guided_chunked:
        __mpcomp_loop_ull_guided_begin( lb, b, incr, chunk, NULL, NULL );
        break ;

    /* runtime */
    case kmp_sch_runtime:
        __mpcomp_loop_ull_runtime_begin( lb, b, incr, chunk, NULL, NULL );
        break;
    case kmp_ord_runtime:
        __mpcomp_loop_ull_ordered_runtime_begin( lb, b, incr, chunk, NULL, NULL );
        break ; 

    default:
      sctk_error("Schedule not handled: %d\n", t->schedule_type);
      not_implemented() ;
      break ;
  }
}

static inline int __kmp_dispatch_next_mpcomp_long( mpcomp_thread_t* t, long* from, long *to )
{
  int ret;


  switch( t->schedule_type ) 
  {
    /* regular scheduling */
    case kmp_sch_auto:
    case kmp_sch_static:
    case kmp_sch_static_chunked:
        ret =  __mpcomp_static_loop_next( from, to ) ;
        sctk_nodebug(" from: %ld -- to: %ld", *from, *to );
        break;

    case kmp_ord_auto:
    case kmp_ord_static:
    case kmp_ord_static_chunked:
        ret =  __mpcomp_ordered_static_loop_next( from, to ) ;
        sctk_nodebug(" from: %ld -- to: %ld", *from, *to );
        break;

    /* dynamic */
    case kmp_sch_dynamic_chunked:
        ret =  __mpcomp_dynamic_loop_next( from, to ) ;
        break;
    case kmp_ord_dynamic_chunked:
        ret =  __mpcomp_ordered_dynamic_loop_next( from, to ) ;
        break;

    /* guided */
    case kmp_sch_guided_chunked:
        ret =  __mpcomp_guided_loop_next( from, to ) ;
        break ;
    case kmp_ord_guided_chunked:
        ret =  __mpcomp_ordered_guided_loop_next( from, to ) ;
        break ;

    /* runtime */
    case kmp_sch_runtime:
        ret =  __mpcomp_runtime_loop_next( from, to ) ;
        break;
    case kmp_ord_runtime:
        ret =  __mpcomp_ordered_runtime_loop_next( from, to ) ;
        break;
    default:
        not_implemented();
    break;
  }
    return ret;
}
static inline int __kmp_dispatch_next_mpcomp_ull( mpcomp_thread_t* t, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long* from, unsigned long long *to )
{
  int ret;
  switch( t->schedule_type ) 
  {
    /* regular scheduling */
    case kmp_sch_auto:
    case kmp_sch_static:
    case kmp_sch_static_chunked:
        ret =  __mpcomp_static_loop_next_ull( from, to ) ;
        break;

    case kmp_ord_auto:
    case kmp_ord_static:
    case kmp_ord_static_chunked:
        ret =  __mpcomp_ordered_static_loop_next_ull( from, to ) ;
        break;

    /* dynamic */
    case kmp_sch_dynamic_chunked:
        ret =  __mpcomp_loop_ull_dynamic_next( from, to ) ;
        break;
    case kmp_ord_dynamic_chunked:
        ret =  __mpcomp_loop_ull_ordered_dynamic_next( from, to ) ;
        break;

    /* guided */
    case kmp_sch_guided_chunked:
        ret =  __mpcomp_loop_ull_guided_next( from, to ) ;
        break ;
    case kmp_ord_guided_chunked:
        ret =  __mpcomp_loop_ull_ordered_guided_next( from, to ) ;
        break ;

    /* runtime */
    case kmp_sch_runtime:
        ret =  __mpcomp_loop_ull_runtime_next( from, to ) ;
        break;
    case kmp_ord_runtime:
        ret =  __mpcomp_loop_ull_ordered_runtime_next( from, to ) ;
        break;
    default:
        not_implemented();
    break;
  }
  return ret;
}

static inline void __kmpc_dispatch_fini_mpcomp_gen( ident_t *loc, kmp_int32 gtid )
{
    mpcomp_thread_t* t;
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
    sctk_assert( t != NULL );
    
    switch( t->schedule_type )   
    {
        case kmp_sch_static:
        case kmp_sch_auto:
        case kmp_sch_static_chunked:
             __mpcomp_static_loop_end_nowait() ;
            break;

        case kmp_ord_static:
        case kmp_ord_auto:
        case kmp_ord_static_chunked:
             __mpcomp_ordered_static_loop_end_nowait() ;
            break;

        case kmp_sch_dynamic_chunked:
             __mpcomp_dynamic_loop_end_nowait() ;
            break;
        case kmp_ord_dynamic_chunked:
             __mpcomp_ordered_dynamic_loop_end_nowait() ;
            break;

        case kmp_sch_guided_chunked:
             __mpcomp_guided_loop_end_nowait() ;
            break ;
        case kmp_ord_guided_chunked:
            __mpcomp_ordered_guided_loop_end_nowait() ;
            break ;

        case kmp_sch_runtime:
            __mpcomp_runtime_loop_end_nowait() ;
            break;
        case kmp_ord_runtime:
            __mpcomp_ordered_runtime_loop_end_nowait() ;
            break;
        default:
            not_implemented();
        break;
    }
}
#endif /*  __MPCOMP_INTEL_DISPATCH_H__ */
