#include "sctk_bool.h"
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_types.h"
#include "mpcomp_GOMP_common.h"

#if MPCOMP_USE_TASKDEP
void mpcomp_GOMP_task(void (*fn)(void *), void *data,
                      void (*cpyfn)(void *, void *), long arg_size,
                      long arg_align, bool if_clause, unsigned flags,
                      void **depend)
#else
void mpcomp_GOMP_task(void (*fn)(void *), void *data,
                      void (*cpyfn)(void *, void *), long arg_size,
                      long arg_align, bool if_clause, unsigned flags,
                      void **depend, int priority)
#endif
{
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tBegin", __func__);
#if MPCOMP_USE_TASKDEP
  mpcomp_task_with_deps((void *(*)(void *))fn, data, cpyfn, arg_size, arg_align,
                        if_clause, flags, depend);
#else
  __mpcomp_task((void *(*)(void *))fn, data, cpyfn, arg_size, arg_align,
                if_clause, flags);
#endif
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_taskwait(void) {
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tBegin", __func__);
  mpcomp_taskwait();
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_taskyield(void) {
  /* Nothing at the moment.  */
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tBegin", __func__);
  mpcomp_taskyield();
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tEnd", __func__);
}

#if MPCOMP_TASK
void mpcomp_GOMP_taskgroup_start(void) {
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tBegin", __func__);
  mpcomp_taskgroup_start();
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_taskgroup_end(void) {
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tBegin", __func__);
  mpcomp_taskgroup_end();
  sctk_nodebug("[Redirect mpcomp_GOMP]%s:\tEnd", __func__);
}
#endif /* MPCOMP_TASK */
