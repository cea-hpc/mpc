#include <stdbool.h>
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_internal.h"
#include "mpcomp_GOMP_common.h"
#include "mpcomp_GOMP_loop_internal.h"
#include "mpcomp_GOMP_parallel_internal.h" 

bool __mpcomp_GOMP_loop_runtime_start (long start, long end, long incr, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   ret = __mpcomp_internal_GOMP_loop_runtime_start(start,end,incr,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_ordered_runtime_start (long start, long end, long incr, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   ret = __mpcomp_internal_GOMP_loop_ordered_runtime_start(start,end,incr,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_runtime_next (long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   ret = __mpcomp_runtime_loop_next(istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_ordered_runtime_next (long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   ret = __mpcomp_ordered_runtime_loop_next(istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

void __mpcomp_GOMP_parallel_loop_static_start (void (*fn) (void *), void *data,
				 unsigned num_threads, long start, long end,
				 long incr, long chunk_size)
{
   bool ret;
   __mpcomp_internal_GOMP_parallel_loop_generic_start(fn,data,num_threads,start,end,incr,chunk_size,(long)MPCOMP_COMBINED_STATIC_LOOP);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_dynamic_start(void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr, long chunk_size)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin\tSTART:%ld",__func__,start);
    __mpcomp_internal_GOMP_parallel_loop_generic_start(fn,data,num_threads,start,end,incr,chunk_size,(long)MPCOMP_COMBINED_DYN_LOOP);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_guided_start (void (*fn) (void *), void *data,
				 unsigned num_threads, long start, long end,
				 long incr, long chunk_size)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   __mpcomp_internal_GOMP_parallel_loop_guided_start(fn,data,num_threads,start,end,incr,chunk_size);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_runtime_start (void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   __mpcomp_internal_GOMP_parallel_loop_runtime_start(fn,data,num_threads,start,end,incr);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_static (void (*fn) (void *), void *data,
			   unsigned num_threads, long start, long end,
			   long incr, long chunk_size, unsigned flags)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   MPCOMP_GOMP_UNUSED_VAR(fn);
   MPCOMP_GOMP_UNUSED_VAR(data); 
   MPCOMP_GOMP_UNUSED_VAR(num_threads);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(flags);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_dynamic (void (*fn) (void *), void *data,
			    unsigned num_threads, long start, long end,
			    long incr, long chunk_size, unsigned flags)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   MPCOMP_GOMP_UNUSED_VAR(fn);
   MPCOMP_GOMP_UNUSED_VAR(data); 
   MPCOMP_GOMP_UNUSED_VAR(num_threads);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(flags);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_guided (void (*fn) (void *), void *data,
			  unsigned num_threads, long start, long end,
			  long incr, long chunk_size, unsigned flags)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   MPCOMP_GOMP_UNUSED_VAR(fn);
   MPCOMP_GOMP_UNUSED_VAR(data); 
   MPCOMP_GOMP_UNUSED_VAR(num_threads);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(flags);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_nonmonotonic_dynamic (void (*fn) (void *), void *data,
					 unsigned num_threads, long start,
					 long end, long incr, long chunk_size,
					 unsigned flags)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   MPCOMP_GOMP_UNUSED_VAR(fn);
   MPCOMP_GOMP_UNUSED_VAR(data); 
   MPCOMP_GOMP_UNUSED_VAR(num_threads);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(flags);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_nonmonotonic_guided (void (*fn) (void *), void *data,
					unsigned num_threads, long start,
					long end, long incr, long chunk_size,
					unsigned flags)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   MPCOMP_GOMP_UNUSED_VAR(fn);
   MPCOMP_GOMP_UNUSED_VAR(data); 
   MPCOMP_GOMP_UNUSED_VAR(num_threads);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(flags);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_loop_runtime (void (*fn) (void *), void *data,
			    unsigned num_threads, long start, long end,
			    long incr, unsigned flags)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   MPCOMP_GOMP_UNUSED_VAR(fn);
   MPCOMP_GOMP_UNUSED_VAR(data); 
   MPCOMP_GOMP_UNUSED_VAR(num_threads);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(flags);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}


void __mpcomp_GOMP_loop_end (void)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   __mpcomp_internal_GOMP_loop_end();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

bool __mpcomp_GOMP_loop_end_cancel (void)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false;
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret; 
}

void __mpcomp_GOMP_loop_end_nowait(void)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   __mpcomp_internal_GOMP_loop_end_nowait();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

bool __mpcomp_GOMP_loop_static_start (long start, long end, long incr, long chunk_size, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_internal_GOMP_loop_static_start(start,end,incr,chunk_size,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_dynamic_start (long start, long end, long incr, long chunk_size,
			 long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_internal_GOMP_loop_dynamic_start(start,end,incr,chunk_size,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_guided_start (long start, long end, long incr, long chunk_size,
			long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_internal_GOMP_loop_guided_start(start,end,incr,chunk_size,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_nonmonotonic_dynamic_start (long start, long end, long incr,
				      long chunk_size, long *istart,
				      long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_nonmonotonic_guided_start(long start, long end, long incr,
				     long chunk_size, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_ordered_static_start(long start, long end, long incr,
				long chunk_size, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_internal_GOMP_loop_ordered_static_start(start,end,incr,chunk_size,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_ordered_dynamic_start(long start, long end, long incr,
				 long chunk_size, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_internal_GOMP_loop_ordered_dynamic_start(start,end,incr,chunk_size,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_ordered_guided_start(long start, long end, long incr,
				long chunk_size, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_internal_GOMP_loop_ordered_guided_start(start,end,incr,chunk_size,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_doacross_static_start(unsigned ncounts, long *counts,
				 long chunk_size, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(ncounts);
   MPCOMP_GOMP_UNUSED_VAR(counts);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_doacross_dynamic_start(unsigned ncounts, long *counts,
				  long chunk_size, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(ncounts);
   MPCOMP_GOMP_UNUSED_VAR(counts);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_doacross_guided_start(unsigned ncounts, long *counts,
				 long chunk_size, long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(ncounts);
   MPCOMP_GOMP_UNUSED_VAR(counts);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_static_next(long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (__mpcomp_static_loop_next(istart,iend)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_dynamic_next(long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (__mpcomp_dynamic_loop_next(istart,iend)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_guided_next(long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (__mpcomp_guided_loop_next(istart,iend)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_nonmonotonic_dynamic_next(long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_nonmonotonic_guided_next(long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_ordered_static_next (long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (__mpcomp_ordered_static_loop_next(istart,iend)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_ordered_dynamic_next (long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (__mpcomp_ordered_dynamic_loop_next(istart,iend)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool __mpcomp_GOMP_loop_ordered_guided_next (long *istart, long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (__mpcomp_ordered_guided_loop_next(istart,iend)) ? true : false;
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}
