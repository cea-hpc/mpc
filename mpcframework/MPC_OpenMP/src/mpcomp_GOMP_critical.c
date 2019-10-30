#include "mpcomp_abi.h"
#include "sctk_debug.h"
#include "mpcomp_GOMP_common.h"

void mpcomp_GOMP_critical_start(void) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  __mpcomp_anonymous_critical_begin();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_critical_end(void) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  __mpcomp_anonymous_critical_end();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_critical_name_start(void **pptr) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  __mpcomp_named_critical_begin(pptr);
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_critical_name_end(void **pptr) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  __mpcomp_named_critical_end(pptr);
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_atomic_start(void) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  __mpcomp_atomic_begin();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

void mpcomp_GOMP_atomic_end(void) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  __mpcomp_atomic_end();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}
