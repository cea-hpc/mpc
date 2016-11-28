#ifndef __MPCOMP_GOMP_SCHEDULER_INTERNAL_H__
#define __MPCOMP_GOMP_SCHEDULER_INTERNAL_H__

#include "mpcomp_abi.h"
#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_internal_extended.h"

/* Handle beginning of combined parallel region */
static inline void 
__in_order_scheduler_init(mpcomp_thread_t * t, mpcomp_new_parallel_region_info_t* info)
{
   switch(info->combined_pragma)
   {
      case MPCOMP_COMBINED_NONE:
         sctk_debug("%s:\tBEGIN - No combined parallel", __func__ );
         break;
      case MPCOMP_COMBINED_SECTION:
         sctk_debug("%s:\tBEGIN - Combined parallel/sections w/ %d section(s)", __func__, info->nb_sections);
         __mpcomp_sections_init(t, info->nb_sections);
         break;
      case MPCOMP_COMBINED_STATIC_LOOP:
         sctk_debug("%s:\tBEGIN - Combined parallel/loop", __func__);
         __mpcomp_static_loop_init(t, info->loop_lb, info->loop_b, info->loop_incr, info->loop_chunk_size);
         break;
      case MPCOMP_COMBINED_DYN_LOOP:
         sctk_debug("%s\tBEGIN - Combined parallel/loop", __func__);
         __mpcomp_dynamic_loop_init(t, info->loop_lb, info->loop_b, info->loop_incr, info->loop_chunk_size);
         break ;
      default:
         not_implemented();
   }
}

/* Handle ending of combined parallel region */
static inline void
__in_order_scheduler_fini(mpcomp_thread_t * t, mpcomp_new_parallel_region_info_t* info)
{
   switch( info->combined_pragma )
   {
      case MPCOMP_COMBINED_NONE:
         sctk_nodebug("%s:\tEND - No combined parallel");
         break;
      case MPCOMP_COMBINED_SECTION:
         sctk_nodebug("%s:\tEND - Combined parallel/sections w/ %d section(s)", info->nb_sections);
         break;
      case MPCOMP_COMBINED_STATIC_LOOP:
         sctk_nodebug("%s:\tEND - Combined parallel/loop");
         break;
      case MPCOMP_COMBINED_GUIDED_LOOP:
      case MPCOMP_COMBINED_DYN_LOOP:
         sctk_nodebug("%s:\tEND - Combined parallel/loop");
         __mpcomp_dynamic_loop_end_nowait(t);
         break;
      default:
         not_implemented();
         break;
   }
}

/* Handle beginning of combined parallel region */
static inline void 
__in_order_scheduler_init_ull(mpcomp_thread_t * t, mpcomp_new_parallel_region_info_t* info)
{
   switch(info->combined_pragma)
   {
      case MPCOMP_COMBINED_NONE:
         sctk_debug("%s:\tBEGIN - No combined parallel", __func__ );
         break;
      case MPCOMP_COMBINED_SECTION:
         sctk_debug("%s:\tBEGIN - Combined parallel/sections w/ %d section(s)", __func__, info->nb_sections);
         __mpcomp_sections_init(t, info->nb_sections);
         break;
      case MPCOMP_COMBINED_STATIC_LOOP:
         sctk_debug("%s:\tBEGIN - Combined parallel/loop", __func__);
         __mpcomp_static_loop_init_ull(t, info->loop_lb, info->loop_b, info->loop_incr, info->loop_chunk_size);
         break;
      case MPCOMP_COMBINED_DYN_LOOP:
         sctk_debug("%s\tBEGIN - Combined parallel/loop", __func__);
         __mpcomp_dynamic_loop_init_ull(t, info->loop_lb, info->loop_b, info->loop_incr, info->loop_chunk_size);
         break ;
      default:
         not_implemented();
   }
}

#endif /* __MPCOMP_GOMP_SCHEDULER_INTERNAL_H__ */
