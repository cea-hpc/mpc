#include "sctk_bool.h"
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_types.h"
#include "mpcomp_GOMP_common.h"
#include "mpcomp_GOMP_loop_internal.h"
#include "mpcomp_GOMP_parallel_internal.h" 

#include "mpcomp_GOMP_versionning.h"

/* Include de MPCOMP function protos */
#include "mpcomp_loop_static.h"
#include "mpcomp_loop_dyn.h"
#include "mpcomp_loop_guided.h"
#include "mpcomp_loop_runtime.h"

bool mpcomp_GOMP_loop_runtime_start (long istart, long iend, long incr, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   ret = mpcomp_internal_GOMP_loop_runtime_start(istart,iend,incr,start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ordered_runtime_start (long istart, long iend, long incr, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   ret = mpcomp_internal_GOMP_loop_ordered_runtime_start(istart,iend,incr,start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_runtime_next (long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   ret = mpcomp_runtime_loop_next(start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ordered_runtime_next (long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   ret = mpcomp_ordered_runtime_loop_next(start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

void mpcomp_GOMP_parallel_loop_static_start (void (*fn) (void *), void *data,
				 unsigned num_threads, long start, long end,
				 long incr, long chunk_size)
{
   bool ret;
   mpcomp_internal_GOMP_parallel_loop_generic_start(fn,data,num_threads,start,end,incr,chunk_size,(long)MPCOMP_COMBINED_STATIC_LOOP);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void mpcomp_GOMP_parallel_loop_dynamic_start(void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr, long chunk_size)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin\tSTART:%ld",__func__,start);
    mpcomp_internal_GOMP_parallel_loop_generic_start( fn, data, num_threads, start, end, incr, chunk_size, (long)MPCOMP_COMBINED_DYN_LOOP);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void mpcomp_GOMP_parallel_loop_guided_start (void (*fn) (void *), void *data,
				 unsigned num_threads, long start, long end,
				 long incr, long chunk_size)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   mpcomp_internal_GOMP_parallel_loop_guided_start( fn, data, num_threads, start, end, incr, chunk_size);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void mpcomp_GOMP_parallel_loop_runtime_start (void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   mpcomp_internal_GOMP_parallel_loop_runtime_start( fn, data, num_threads, start, end, incr);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void mpcomp_GOMP_parallel_loop_nonmonotonic_dynamic (void (*fn) (void *), void *data,
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

void mpcomp_GOMP_parallel_loop_nonmonotonic_guided (void (*fn) (void *), void *data,
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

void mpcomp_GOMP_loop_end (void)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__); 
   mpcomp_internal_GOMP_loop_end();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

bool mpcomp_GOMP_loop_end_cancel (void)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false;
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret; 
}

void mpcomp_GOMP_loop_end_nowait(void)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   mpcomp_internal_GOMP_loop_end_nowait();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

bool mpcomp_GOMP_loop_static_start (long istart, long iend, long incr, long chunk_size, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = mpcomp_internal_GOMP_loop_static_start(istart,iend,incr,chunk_size,start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_dynamic_start (long istart, long iend, long incr, long chunk_size,
			 long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = mpcomp_dynamic_loop_begin(istart,iend,incr,chunk_size,start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_guided_start (long istart, long iend, long incr, long chunk_size,
			long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = mpcomp_internal_GOMP_loop_guided_start(istart,iend,incr,chunk_size,start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_nonmonotonic_dynamic_start (long istart, long iend, long incr,
				      long chunk_size, long *start,
				      long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_nonmonotonic_guided_start(long istart, long iend, long incr,
				     long chunk_size, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(istart);
   MPCOMP_GOMP_UNUSED_VAR(iend);
   MPCOMP_GOMP_UNUSED_VAR(incr);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ordered_static_start(long istart, long iend, long incr,
				long chunk_size, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = mpcomp_internal_GOMP_loop_ordered_static_start(istart,iend,incr,chunk_size,start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ordered_dynamic_start(long istart, long iend, long incr,
				 long chunk_size, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = mpcomp_internal_GOMP_loop_ordered_dynamic_start(istart,iend,incr,chunk_size,start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ordered_guided_start(long istart, long iend, long incr,
				long chunk_size, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = mpcomp_internal_GOMP_loop_ordered_guided_start(istart,iend,incr,chunk_size,start,end);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_doacross_static_start(unsigned ncounts, long *counts,
				 long chunk_size, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(ncounts);
   MPCOMP_GOMP_UNUSED_VAR(counts);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_doacross_dynamic_start(unsigned ncounts, long *counts,
				  long chunk_size, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(ncounts);
   MPCOMP_GOMP_UNUSED_VAR(counts);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_doacross_guided_start(unsigned ncounts, long *counts,
				 long chunk_size, long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(ncounts);
   MPCOMP_GOMP_UNUSED_VAR(counts);
   MPCOMP_GOMP_UNUSED_VAR(chunk_size);
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_static_next(long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (mpcomp_static_loop_next(start,end)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_dynamic_next(long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (mpcomp_dynamic_loop_next(start,end)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_guided_next(long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (mpcomp_guided_loop_next(start,end)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_nonmonotonic_dynamic_next(long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_nonmonotonic_guided_next(long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false; 
   MPCOMP_GOMP_UNUSED_VAR(start);
   MPCOMP_GOMP_UNUSED_VAR(end);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ordered_static_next (long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (mpcomp_ordered_static_loop_next(start,end)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ordered_dynamic_next (long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (mpcomp_ordered_dynamic_loop_next(start,end)) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ordered_guided_next (long *start, long *end)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (mpcomp_ordered_guided_loop_next(start,end)) ? true : false;
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

/** OPENMP 4.0 **/

void
mpcomp_GOMP_parallel_loop_static (void (*fn) (void *), void *data,
               unsigned num_threads, long start, long end,
               long incr, long chunk_size, unsigned flags)
{
    mpcomp_start_parallel_static_loop(fn, data, num_threads, start, end, incr, chunk_size );
}

void
mpcomp_GOMP_parallel_loop_dynamic (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{
   mpcomp_start_parallel_dynamic_loop(fn, data, num_threads, start, end, incr, chunk_size );
}

void
mpcomp_GOMP_parallel_loop_guided (void (*fn) (void *), void *data, unsigned num_threads, long start, long end, long incr, long chunk_size, unsigned flags)
{
    mpcomp_start_parallel_guided_loop( fn, data, num_threads, start, end, incr, chunk_size);
}

void
mpcomp_GOMP_parallel_loop_runtime (void (*fn) (void *), void *data,
                unsigned num_threads, long start, long end,
                long incr, unsigned flags)
{
    mpcomp_start_parallel_runtime_loop(fn, data, num_threads, start, end, incr);
}

