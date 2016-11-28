#include "mpcomp_abi.h"
#include "mpcomp_types.h"
#include "mpcomp_GOMP_wrapper.h"
#include "sctk_debug.h"
#include "mpcomp_tree_structs.h"

//#include "mpcomp_GOMP_parallel_internal.h"

/* Declaration from MPC
TODO: put these declarations in header file
    ================================================================================   */
//void mpcomp_internal_begin_parallel_region(int arg_num_threads, mpcomp_new_parallel_region_info_t info);
//void mpcomp_internal_end_parallel_region(mpcomp_instance_t* instance);
/*   ================================================================================   */

static void mpcomp_internal_GOMP_in_order_scheduler_master_begin(mpcomp_mvp_t * mvp)
{
   mpcomp_thread_t *t;
   
   /* Switch OpenMP TLS */
   sctk_openmp_thread_tls = &(mvp->threads[0]);
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 

   sctk_assert(t->instance != NULL) ;
   sctk_assert(t->instance->team != NULL);
   sctk_assert(t->info.func != NULL);

   t->schedule_type = (long)t->info.combined_pragma;

   /* Handle beginning of combined parallel region */
   switch(t->info.combined_pragma)
   {
      case MPCOMP_COMBINED_NONE:
         sctk_nodebug("%s:\nBEGIN - No combined parallel",__func__);
         break;
      case MPCOMP_COMBINED_SECTION:
         sctk_nodebug("%s:\n BEGIN - Combined parallel/sections w/ %d section(s)",__func__,t->info.nb_sections);
         __mpcomp_sections_init(t,t->info.nb_sections);
         break;
      case MPCOMP_COMBINED_STATIC_LOOP:
         sctk_nodebug("%s:\tBEGIN - Combined parallel/loop",__func__);
         __mpcomp_static_loop_init(t,t->info.loop_lb,t->info.loop_b,t->info.loop_incr,t->info.loop_chunk_size);
         break;
      case MPCOMP_COMBINED_DYN_LOOP:
         sctk_nodebug("%s:\tBEGIN - Combined parallel/loop",__func__);
         __mpcomp_dynamic_loop_init(t,t->info.loop_lb,t->info.loop_b,t->info.loop_incr,t->info.loop_chunk_size);
         break;
      default:
         not_implemented();
	 break;
    }
}

static void mpcomp_internal_GOMP_in_order_scheduler_master_end(void)
{
   mpcomp_thread_t *t = (mpcomp_thread_t*) sctk_openmp_thread_tls;
   sctk_assert(t != NULL);
 
   /* Handle ending of combined parallel region */
   switch(t->info.combined_pragma) 
   {
       case MPCOMP_COMBINED_NONE:
          sctk_nodebug("%s:\tEND - No combined parallel",__func__);
          break;
       case MPCOMP_COMBINED_SECTION:
          sctk_nodebug("%s:\tEND - Combined parallel/sections w/ %d section(s)",__func__,t->info.nb_sections);
          break;
       case MPCOMP_COMBINED_STATIC_LOOP:
          sctk_nodebug("%s:\tEND - Combined parallel/loop",__func__);
          break;
       case MPCOMP_COMBINED_DYN_LOOP:
          sctk_nodebug("%s:\tEND - Combined parallel/loop",__func__);
          __mpcomp_dynamic_loop_end_nowait(t);
          break;
       default:
          not_implemented();
          break; 
   }

   t->done = 1; 

    //mpcomp_taskwait();

   /* Restore previous TLS */
   sctk_openmp_thread_tls = t->father;
}

void mpcomp_internal_GOMP_parallel_start(void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags)
{
    num_threads = (num_threads == 0) ? -1 : num_threads;
    __mpcomp_start_parallel_region( num_threads, fn, data );
} 

void mpcomp_internal_GOMP_start_parallel_region(void (*fn) (void *), void *data, unsigned num_threads)
{
   mpcomp_thread_t* t;
   mpcomp_new_parallel_region_info_t* info;
   mpcomp_GOMP_wrapper_t *wrapper_gomp;

   /* Initialize OpenMP environment */
  __mpcomp_init();

   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);

   /* Prepare MPC OpenMP parallel region infos */
   info = sctk_malloc(sizeof(mpcomp_new_parallel_region_info_t));
   sctk_assert(info);
   __mpcomp_parallel_region_infos_init(info);

   info->func = fn; 
   info->shared = data;
   info->combined_pragma = MPCOMP_COMBINED_NONE;
   
   /* Begin scheduling */
   __mpcomp_internal_begin_parallel_region(info, num_threads);
   
   /* Start scheduling */
   mpcomp_mvp_t * mvp = t->children_instance->mvps[0];
   mpcomp_internal_GOMP_in_order_scheduler_master_begin( mvp );

   return;
}

void mpcomp_internal_GOMP_end_parallel_region(void)
{
   mpcomp_internal_GOMP_in_order_scheduler_master_end();

   mpcomp_thread_t *t = sctk_openmp_thread_tls;   
   sctk_assert(t != NULL);

   mpcomp_instance_t *instance = t->children_instance;
   sctk_assert(instance != NULL);

   __mpcomp_internal_end_parallel_region( instance );
}

void mpcomp_internal_GOMP_parallel_loop_generic_start (void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr, long chunk_size, long combined_pragma)
{
   mpcomp_thread_t *t;
   mpcomp_new_parallel_region_info_t* info;
   mpcomp_GOMP_wrapper_t *wrapper_gomp;

   /* Initialize OpenMP environment */
  __mpcomp_init() ;

   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);

   info = sctk_malloc(sizeof(mpcomp_new_parallel_region_info_t));
   sctk_assert(info);
   __mpcomp_parallel_region_infos_init(info);

   wrapper_gomp = sctk_malloc(sizeof(mpcomp_GOMP_wrapper_t));
   sctk_assert(wrapper_gomp);
   wrapper_gomp->fn = fn;
   wrapper_gomp->data = data;

   info->func = mpcomp_GOMP_wrapper_fn;
   info->shared = wrapper_gomp;
   info->combined_pragma = combined_pragma;

   info->loop_lb = start;
   info->loop_b = end;
   info->loop_incr = incr;
   info->loop_chunk_size = chunk_size;

    /* Begin scheduling */
   num_threads = (num_threads == 0) ? -1 : num_threads; 
   __mpcomp_internal_begin_parallel_region(info, num_threads);

   /* Start scheduling */
   mpcomp_mvp_t * mvp = t->children_instance->mvps[0];
   mpcomp_internal_GOMP_in_order_scheduler_master_begin(mvp);

   return;
}

void mpcomp_internal_GOMP_parallel_loop_guided_start (void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr, long chunk_size)
{
   //TODO Implemented guided algorithm, Guided fallback to dynamic loop in mpcomp
    mpcomp_internal_GOMP_parallel_loop_generic_start(fn,data,num_threads,start,end,incr,chunk_size,(long)MPCOMP_COMBINED_GUIDED_LOOP);
}

void mpcomp_internal_GOMP_parallel_loop_runtime_start (void (*fn) (void *), void *data,
				  unsigned num_threads, long start, long end,
				  long incr)
{
   long combined_pragma, chunk_size;
   mpcomp_thread_t *t ;    /* Info on the current thread */

   /* Handle orphaned directive (initialize OpenMP environment) */
  __mpcomp_init();

   /* Grab the thread info */
   t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
   sctk_assert( t != NULL ) ;
  
   switch(t->info.icvs.run_sched_var) 
   {
      case omp_sched_static:
         combined_pragma = (long) MPCOMP_COMBINED_STATIC_LOOP; 
         break;
      case omp_sched_dynamic:
      case omp_sched_guided:
         combined_pragma = (long) MPCOMP_COMBINED_DYN_LOOP; 
         break;
      default:
         not_reachable();
         break;
   }
   chunk_size = t->info.icvs.modifier_sched_var;
   mpcomp_internal_GOMP_parallel_loop_generic_start(fn,data,num_threads,start,end,incr,chunk_size,combined_pragma);

}

void mpcomp_internal_GOMP_parallel_sections_start(void (*fn) (void *), void *data, unsigned num_threads, unsigned count)
{
   mpcomp_thread_t *t;
   mpcomp_new_parallel_region_info_t* info;
   mpcomp_GOMP_wrapper_t *wrapper_gomp;

   /* Initialize OpenMP environment */
  __mpcomp_init() ;

   t = (mpcomp_thread_t *) sctk_openmp_thread_tls; 
   sctk_assert(t != NULL);

   info = sctk_malloc(sizeof(mpcomp_new_parallel_region_info_t));
   sctk_assert(info);
   __mpcomp_parallel_region_infos_init(info);

   wrapper_gomp = sctk_malloc(sizeof(mpcomp_GOMP_wrapper_t));
   sctk_assert(wrapper_gomp);
   wrapper_gomp->fn = fn;
   wrapper_gomp->data = data;

   info->func = mpcomp_GOMP_wrapper_fn;
   info->shared = wrapper_gomp;
   info->combined_pragma = MPCOMP_COMBINED_SECTION;

   info->nb_sections = count;

    /* Begin scheduling */
   num_threads = (num_threads == 0) ? -1 : num_threads; 
   __mpcomp_internal_begin_parallel_region(info, num_threads);
 
   /* Start scheduling */
   mpcomp_mvp_t * mvp = t->children_instance->mvps[0];
   mpcomp_internal_GOMP_in_order_scheduler_master_begin(mvp);

   return;
}
