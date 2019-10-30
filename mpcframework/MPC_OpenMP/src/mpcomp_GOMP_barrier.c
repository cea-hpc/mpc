#include "mpc_common_types.h"
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_barrier.h"
#include "mpcomp_GOMP_common.h"

void mpcomp_GOMP_barrier(void) {
  sctk_nodebug("[Redirect GOMP]%s:\tBegin", __func__);
  __mpcomp_barrier();
  sctk_nodebug("[Redirect GOMP]%s:\tEnd", __func__);
}

bool GOMP_barrier_cancel (void)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   not_implemented();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return false;
}
