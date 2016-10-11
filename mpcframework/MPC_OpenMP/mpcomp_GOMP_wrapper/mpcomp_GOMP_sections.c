#include <stdbool.h>
#include <sctk_debug.h>
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"
#include "mpcomp_GOMP_parallel_internal.h"


unsigned __mpcomp_GOMP_sections_start (unsigned count)
{
   unsigned ret; 
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_sections_begin(count);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

unsigned __mpcomp_GOMP_sections_next (void)
{
   unsigned ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_sections_next();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

void __mpcomp_GOMP_parallel_sections_start (void (*fn) (void *), void *data,
			      unsigned num_threads, unsigned count)
{
  sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   __mpcomp_internal_GOMP_parallel_sections_start(fn,data,num_threads,count);
  sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_sections (void (*fn) (void *), void *data,
			unsigned num_threads, unsigned count, unsigned flags)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   num_threads = (num_threads == 0) ? -1 : num_threads; 
    __mpcomp_start_sections_parallel_region( num_threads, (void* (*)(void*)) fn , data, count);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_sections_end (void)
{
  sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   /* Nothing to do */
  sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

bool __mpcomp_GOMP_sections_end_cancel (void)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = false;
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret; 
}

void __mpcomp_GOMP_sections_end_nowait (void)
{
  sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   /* Nothing to do */
  sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}
