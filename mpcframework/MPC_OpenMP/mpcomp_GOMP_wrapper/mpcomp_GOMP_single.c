#include <stdbool.h>
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"

bool mpcomp_GOMP_single_start (void)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = (mpcomp_do_single()) ? true : false; 
   sctk_nodebug("[Redirect GOMP]%s:\tEnd -- ret: %s",__func__, (ret == true) ? "true" : "false");
   return ret;
}

void* mpcomp_GOMP_single_copy_start (void)
{
   void* ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = mpcomp_do_single_copyprivate_begin();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

void mpcomp_GOMP_single_copy_end (void *data)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   mpcomp_do_single_copyprivate_end(data);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return;
}
