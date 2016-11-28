#include "sctk_bool.h"
#include "mpcomp_abi.h"
#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_GOMP_loop_ull_internal.h"

bool mpcomp_internal_GOMP_loop_ull_static_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   mpcomp_thread_t* t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_STATIC_LOOP;
   ret = (mpcomp_loop_ull_static_begin(up, start,end,incr,chunk_size, istart, iend)) ? true : false;
   return ret;
}

bool mpcomp_internal_GOMP_loop_ull_dynamic_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   mpcomp_thread_t* t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_DYN_LOOP;
   ret = (mpcomp_loop_ull_dynamic_begin(up, start,end,incr,chunk_size, istart, iend)) ? true : false;
   return ret;
}

bool mpcomp_internal_GOMP_loop_ull_guided_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   mpcomp_thread_t* t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_GUIDED_LOOP;
   ret = (mpcomp_loop_ull_guided_begin(up, start,end,incr,chunk_size, istart, iend)) ? true : false;
   return ret;
}

bool mpcomp_internal_GOMP_loop_ull_runtime_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr,
                            unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   mpcomp_thread_t* t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_RUNTIME_LOOP;
   ret = (mpcomp_loop_ull_runtime_begin(up, start,end,incr, istart, iend)) ? true : false;
   return ret;
}

bool mpcomp_internal_GOMP_loop_ull_ordered_static_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   mpcomp_thread_t* t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_STATIC_LOOP;
   ret = (mpcomp_loop_ull_ordered_static_begin(start,end,incr,chunk_size, istart, iend)) ? true : false;
   return ret;
}

bool mpcomp_internal_GOMP_loop_ull_ordered_dynamic_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   mpcomp_thread_t* t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_DYN_LOOP;
   ret = (mpcomp_loop_ull_ordered_dynamic_begin(start,end,incr,chunk_size, istart, iend)) ? true : false;
   return ret;
}

bool mpcomp_internal_GOMP_loop_ull_ordered_guided_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr, unsigned long long chunk_size,
                            unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   mpcomp_thread_t* t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_GUIDED_LOOP;
   ret = (mpcomp_loop_ull_ordered_guided_begin(start,end,incr,chunk_size, istart, iend)) ? true : false;
   return ret;
}

bool mpcomp_internal_GOMP_loop_ull_ordered_runtime_start(bool up, unsigned long long start, unsigned long long end,
                            unsigned long long incr,
                            unsigned long long *istart, unsigned long long *iend)
{
   bool ret;
   mpcomp_thread_t* t;
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
   t->schedule_type = MPCOMP_COMBINED_RUNTIME_LOOP;
   ret = (mpcomp_loop_ull_ordered_runtime_begin(start,end,incr, istart, iend)) ? true : false;
   return ret;
}
