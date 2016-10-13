#include <stdbool.h>
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_internal.h"
#include "mpcomp_GOMP_common.h"

#if 0
void __mpcomp_GOMP_task(void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
	   long arg_size, long arg_align, bool if_clause, unsigned flags,
	   void **depend, int priority)
#else 
void __mpcomp_GOMP_task(void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
	   long arg_size, long arg_align, bool if_clause, unsigned flags, void **depend)
#endif
{
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tBegin",__func__);
   __mpcomp_task_with_deps( (void* (*)(void*) ) fn, data, cpyfn, arg_size, arg_align, if_clause, flags, depend);
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_taskwait(void)
{
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tBegin",__func__);
   __mpcomp_taskwait();
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_taskyield(void)
{
  /* Nothing at the moment.  */
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tBegin",__func__);
   __mpcomp_taskyield();
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_taskgroup_start(void)
{
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tBegin",__func__);
   not_implemented();
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_taskgroup_end(void)
{
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tBegin",__func__);
   not_implemented();
   sctk_nodebug("[Redirect __mpcomp_GOMP]%s:\tEnd",__func__);
}
