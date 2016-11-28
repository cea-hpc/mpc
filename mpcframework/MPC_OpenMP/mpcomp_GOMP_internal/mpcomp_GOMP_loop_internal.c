#include "sctk_bool.h"
#include "mpcomp_abi.h"
#include "sctk_debug.h"
#include "mpcomp_types.h"

void mpcomp_internal_GOMP_loop_end(void)
{
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);

   switch(t->schedule_type)
   {
      case MPCOMP_COMBINED_STATIC_LOOP:
      {
         __mpcomp_static_loop_end();
         break;
      }
      case MPCOMP_COMBINED_DYN_LOOP:
      {
         __mpcomp_dynamic_loop_end();
         break;
      }
      case MPCOMP_COMBINED_GUIDED_LOOP:
      {
         __mpcomp_guided_loop_end();
         break;
      }
      case MPCOMP_COMBINED_RUNTIME_LOOP:
      {
         __mpcomp_runtime_loop_end();
         break;
       }
      default:
      {
         not_implemented();
         //__mpcomp_static_loop_end();
         //__mpcomp_dynamic_loop_end();
      }
   }
}

void mpcomp_internal_GOMP_loop_end_nowait(void)
{
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);

   switch(t->schedule_type)
   {
      case MPCOMP_COMBINED_STATIC_LOOP:
      {
         __mpcomp_static_loop_end_nowait();
         break;
      }
      case MPCOMP_COMBINED_DYN_LOOP:
      {
         __mpcomp_dynamic_loop_end_nowait();
         break;
      }
      case MPCOMP_COMBINED_GUIDED_LOOP:
      {
         __mpcomp_guided_loop_end_nowait();
         break;
      }
      case MPCOMP_COMBINED_RUNTIME_LOOP:
      {
         __mpcomp_runtime_loop_end_nowait();
         break;
      }
      default:
      {
         not_implemented();
         //__mpcomp_static_loop_end_nowait();
      }
   }
}

bool mpcomp_internal_GOMP_loop_runtime_start (long start, long end, long incr, long *istart, long *iend)
{
   return (__mpcomp_runtime_loop_begin(start,end,incr,istart,iend)) ? true : false;
}

bool mpcomp_internal_GOMP_loop_ordered_runtime_start (long start, long end, long incr, long *istart, long *iend)
{
   return (__mpcomp_ordered_runtime_loop_begin(start,end,incr,istart,iend)) ? true : false;
}

bool mpcomp_internal_GOMP_loop_static_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   return (__mpcomp_static_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
}

bool mpcomp_internal_GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   return (__mpcomp_dynamic_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
}

bool mpcomp_internal_GOMP_loop_guided_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   return ( __mpcomp_dynamic_loop_begin(start,end,incr,chunk_size,istart,iend) ) ? true : false;
}

bool mpcomp_internal_GOMP_loop_ordered_static_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   return (__mpcomp_ordered_static_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
}

bool mpcomp_internal_GOMP_loop_ordered_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   return (__mpcomp_ordered_dynamic_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
}

bool mpcomp_internal_GOMP_loop_ordered_guided_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   return (__mpcomp_ordered_dynamic_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
}
