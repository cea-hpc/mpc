#include <stdbool.h>
#include <sctk_debug.h>
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"

void mpcomp_GOMP_ordered_start(void)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
    mpcomp_ordered_begin();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void mpcomp_GOMP_ordered_end(void)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   mpcomp_ordered_end();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}
