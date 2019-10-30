#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"

/* The public OpenMP API routines that access these variables.  */

#if 0
void __mpcomp_GOMP_omp_set_num_threads(int n)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   mpcomp_set_num_threads(n);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

void __mpcomp_GOMP_omp_set_dynamic(int val)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   mpcomp_set_dynamic(val);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

int __mpcomp_GOMP_omp_get_dynamic(void)
{
   int ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = omp_get_dynamic();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

void __mpcomp_GOMP_omp_set_nested(int val)
{
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   omp_set_nested(val);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
}

int __mpcomp_GOMP_omp_get_nested(void)
{
   int ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = omp_get_nested();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

void __mpcomp_GOMP_omp_set_schedule (omp_sched_t kind, int chunk_size)
{
}

void __mpcomp_GOMP_omp_get_schedule(omp_sched_t *kind, int *chunk_size)
{
}

int __mpcomp_GOMP_omp_get_max_threads (void)
{
   int ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
   ret = mpcomp_get_max_threads();
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

int
__mpcomp_GOMP_omp_get_thread_limit (void)
{
}

void
__mpcomp_GOMP_omp_set_max_active_levels (int max_levels)
{
}

int
__mpcomp_GOMP_omp_get_max_active_levels (void)
{
}

int
__mpcomp_GOMP_omp_get_cancellation (void)
{
}

int
__mpcomp_GOMP_omp_get_max_task_priority (void)
{
}

__mpcomp_GOMP_omp_proc_bind_t __mpcomp_GOMP_omp_get_proc_bind (void)
{
}

void
__mpcomp_GOMP_omp_set_default_device (int device_num)
{
}

int __mpcomp_GOMP_omp_get_default_device (void)
{
}

int
__mpcomp_GOMP_omp_get_num_devices (void)
{
}

int
__mpcomp_GOMP_omp_get_num_teams (void)
{
  /* Hardcoded to 1 on host, MIC, HSAIL?  Maybe variable on PTX.  */
  return 1;
}

int __mpcomp_GOMP_omp_get_team_num (void)
{
  /* Hardcoded to 0 on host, MIC, HSAIL?  Maybe variable on PTX.  */
  return 0;
}

int
__mpcomp_GOMP_omp_is_initial_device (void)
{
  /* Hardcoded to 1 on host, should be 0 on MIC, HSAIL, PTX.  */
  return 1;
}

int
__mpcomp_GOMP_omp_get_initial_device (void)
{
}

int
__mpcomp_GOMP_omp_get_num_places (void)
{
}

int
__mpcomp_GOMP_omp_get_place_num (void)
{
}

int
__mpcomp_GOMP_omp_get_partition_num_places (void)
{
}

void
__mpcomp_GOMP_omp_get_partition_place_nums (int *place_nums)
{
}
#endif
