#include <stdbool.h>
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"

bool __mpcomp_GOMP_single_start (void)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (__mpcomp_do_single()) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

void* __mpcomp_GOMP_single_copy_start (void)
{
   void* ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = __mpcomp_do_single_copyprivate_begin();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

void __mpcomp_GOMP_single_copy_end (void *data)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   __mpcomp_do_single_copyprivate_end(data);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return;
}
