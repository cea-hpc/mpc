#include <stdbool.h>
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"
#include "mpcomp_GOMP_parallel_internal.h"

void __mpcomp_GOMP_parallel(void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
    __mpcomp_internal_GOMP_parallel_start(fn, data, num_threads, flags);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_start (void (*fn) (void *), void *data, unsigned num_threads)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   __mpcomp_internal_GOMP_start_parallel_region(fn, data, num_threads);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_parallel_end (void)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   __mpcomp_internal_GOMP_end_parallel_region();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

bool __mpcomp_GOMP_cancellation_point(int which)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return false;
}

bool __mpcomp_GOMP_cancel(int which, bool do_cancel)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return false;
}
