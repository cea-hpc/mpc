#include "mpcomp_abi.h"
#include "mpcomp_internal.h"

void __mpcomp_internal_GOMP_loop_end(void)
{
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);

   switch(t->schedule_type)
   {
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
         __mpcomp_runtime_loop_end_nowait();
         break;
      }
      default:
      {
         __mpcomp_static_loop_end();
      }
   }
}

void __mpcomp_internal_GOMP_loop_end_nowait(void)
{
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);

   switch(t->schedule_type)
   {
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
         __mpcomp_static_loop_end_nowait();
      }
   }
}

bool __mpcomp_internal_GOMP_loop_runtime_start (long start, long end, long incr, long *istart, long *iend)
{
   bool ret;
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_RUNTIME_LOOP; 
   ret = (__mpcomp_runtime_loop_begin(start,end,incr,istart,iend)) ? true : false;
   return ret;
}

bool __mpcomp_internal_GOMP_loop_ordered_runtime_start (long start, long end, long incr, long *istart, long *iend)
{
   bool ret;
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_RUNTIME_LOOP; 
   ret = (__mpcomp_ordered_runtime_loop_begin(start,end,incr,istart,iend)) ? true : false;
   return ret;
}

bool __mpcomp_internal_GOMP_loop_static_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   bool ret;
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_STATIC_LOOP; 
   ret = (__mpcomp_static_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
   return ret;
}

bool __mpcomp_internal_GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   bool ret;
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_DYN_LOOP; 
   ret = (__mpcomp_dynamic_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
   return ret;
}

bool __mpcomp_internal_GOMP_loop_guided_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   bool ret;
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_DYN_LOOP; 
   ret = (__mpcomp_dynamic_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
   return ret;
}

bool __mpcomp_internal_GOMP_loop_ordered_static_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   bool ret;
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_STATIC_LOOP; 
   ret = (__mpcomp_ordered_static_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
   return ret;
}

bool __mpcomp_internal_GOMP_loop_ordered_dynamic_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   bool ret;
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_DYN_LOOP; 
   ret = (__mpcomp_ordered_dynamic_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
   return ret;
}

bool __mpcomp_internal_GOMP_loop_ordered_guided_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   bool ret;
   mpcomp_thread_t *t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_DYN_LOOP; 
   ret = (__mpcomp_ordered_dynamic_loop_begin(start,end,incr,chunk_size,istart,iend)) ? true : false;
   return ret;
}


