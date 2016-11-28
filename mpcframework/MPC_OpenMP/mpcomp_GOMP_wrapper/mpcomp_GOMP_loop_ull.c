#include <stdbool.h>
#include "sctk_debug.h"
#include "mpcomp_abi.h"
#include "mpcomp_GOMP_common.h"
#include "mpcomp_GOMP_loop_ull_internal.h"

#define MPC_SUPPORT_ULL_LOOP

bool mpcomp_GOMP_loop_ull_runtime_start(bool up, unsigned long long start, unsigned long long end, 
				          unsigned long long incr, 
                                          unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = __mpcomp_loop_ull_runtime_begin(up,start,end,incr,istart,iend);
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_runtime_start (bool up, unsigned long long start, unsigned long long end,
				     unsigned long long incr, unsigned long long *istart,
				     unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = mpcomp_internal_GOMP_loop_ull_ordered_runtime_start(up,start,end,incr,istart,iend);
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_doacross_runtime_start(unsigned ncounts, unsigned long long *counts,
				      unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = false;
   not_implemented();
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_runtime_next(unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = __mpcomp_loop_ull_runtime_next(istart,iend);  
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_runtime_next(unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = __mpcomp_loop_ull_ordered_runtime_next(istart,iend);  
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}


bool mpcomp_GOMP_loop_ull_static_start(bool up, unsigned long long start, unsigned long long end,
			    unsigned long long incr, unsigned long long chunk_size,
			    unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = mpcomp_internal_GOMP_loop_ull_static_start(up,start,end,incr,chunk_size,istart,iend);
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_dynamic_start(bool up, unsigned long long start, unsigned long long end,
			     unsigned long long incr, unsigned long long chunk_size,
			     unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
#ifdef MPC_SUPPORT_ULL_LOOP
   sctk_nodebug("[Redirect GOMP]%s:\tBegin - start: %llu - end: %llu - step: %llu - up: %d",__func__, start, end, incr, up);
   ret = __mpcomp_loop_ull_dynamic_begin( up, start, end, incr, chunk_size, istart, iend);
   //ret = mpcomp_internal_GOMP_loop_ull_dynamic_start(up,start,end,incr,chunk_size,istart,iend);
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   return ret;
}

bool mpcomp_GOMP_loop_ull_guided_start (bool up, unsigned long long start, unsigned long long end,
			    unsigned long long incr, unsigned long long chunk_size,
			    unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = __mpcomp_loop_ull_guided_begin( up, start, end, incr, chunk_size, istart, iend);
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_start(bool up, unsigned long long start,
					  unsigned long long end, unsigned long long incr,
					  unsigned long long chunk_size,
					  unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#if (defined(MPC_SUPPORT_ULL_LOOP) && defined(MPC_SUPPORT_NONMONOTONIC))
   ret = false;
   not_implemented();
#else /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_nonmonotonic_guided_start (bool up, unsigned long long start, unsigned long long end,
					 unsigned long long incr, unsigned long long chunk_size,
					 unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#if (defined(MPC_SUPPORT_ULL_LOOP) && defined(MPC_SUPPORT_NONMONOTONIC))
   ret = false;
   not_implemented();
#else /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_static_start (bool up, unsigned long long start, unsigned long long end,
				    unsigned long long incr, unsigned long long chunk_size,
				    unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (mpcomp_internal_GOMP_loop_ull_ordered_static_start(up, start, end, incr, chunk_size, istart, iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_dynamic_start (bool up, unsigned long long start, unsigned long long end,
				     unsigned long long incr, unsigned long long chunk_size,
				     unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (mpcomp_internal_GOMP_loop_ull_ordered_dynamic_start(up, start, end, incr, chunk_size, istart, iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_guided_start (bool up, unsigned long long start, unsigned long long end,
				    unsigned long long incr, unsigned long long chunk_size,
				    unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (mpcomp_internal_GOMP_loop_ull_ordered_dynamic_start(up, start, end, incr, chunk_size, istart, iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_doacross_static_start (unsigned ncounts, unsigned long long *counts,
				     unsigned long long chunk_size, unsigned long long *istart,
				     unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = false;
   not_implemented();
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_doacross_dynamic_start (unsigned ncounts, unsigned long long *counts,
				      unsigned long long chunk_size, unsigned long long *istart,
				      unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#if (defined(MPC_SUPPORT_ULL_LOOP) && defined(MPC_SUPPORT_DOACCROSS))
   ret = false;
   not_implemented();
#else /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_DOACCROSS */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_DOACCROSS */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_doacross_guided_start(unsigned ncounts, unsigned long long *counts,
				     unsigned long long chunk_size, unsigned long long *istart,
				     unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#if (defined(MPC_SUPPORT_ULL_LOOP) && defined(MPC_SUPPORT_DOACCROSS))
   ret = false;
   not_implemented();
#else /* MPC_SUPPORT_ULL_LOOP  && MPC_SUPPORT_DOACCROSS */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_DOACCROSS */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_static_next(unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (__mpcomp_static_loop_next_ull(istart,iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_dynamic_next(unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (__mpcomp_loop_ull_dynamic_next(istart,iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_guided_next(unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (__mpcomp_loop_ull_guided_next(istart,iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}


bool mpcomp_GOMP_loop_ull_nonmonotonic_dynamic_next(unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#if (defined(MPC_SUPPORT_ULL_LOOP) && defined(MPC_SUPPORT_NONMONOTONIC))
   not_implemented();
#else /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_nonmonotonic_guided_next(unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#if (defined(MPC_SUPPORT_ULL_LOOP) && defined(MPC_SUPPORT_NONMONOTONIC))
   not_implemented();
#else /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP && MPC_SUPPORT_NONMONOTONIC */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_static_next (unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (__mpcomp_ordered_static_loop_next_ull(istart,iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_dynamic_next (unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (__mpcomp_loop_ull_ordered_dynamic_next(istart,iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}

bool mpcomp_GOMP_loop_ull_ordered_guided_next (unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   sctk_nodebug("[Redirect GOMP]%s:\tBegin",__func__);
#ifdef MPC_SUPPORT_ULL_LOOP
   ret = (__mpcomp_loop_ull_ordered_guided_next(istart,iend)) ? true : false;
#else /* MPC_SUPPORT_ULL_LOOP */
   ret = false;
   not_implemented();
#endif /* MPC_SUPPORT_ULL_LOOP */
   sctk_nodebug("[Redirect GOMP]%s:\tEnd",__func__);
   return ret;
}
