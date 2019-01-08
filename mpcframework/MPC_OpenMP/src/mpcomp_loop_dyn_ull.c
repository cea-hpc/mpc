
#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "mpcomp_types.h"
#include "mpcomp_loop.h"
#include "mpcomp_loop_dyn.h"
#include "mpcomp_loop_dyn_utils.h"

#include "mpcomp_loop_static_ull.h"

#include "mpcomp_core.h"
#include "ompt.h"

/****
 *
 * ULL CHUNK MANIPULATION
 *
 *
 *****/

/* From thread t, try to steal a chunk from thread target
 * Returns 1 on success, 0 otherwise */
static int __mpcomp_dynamic_loop_get_chunk_from_rank_ull(
    mpcomp_thread_t *t, mpcomp_thread_t *target, unsigned long long *from,
    unsigned long long *to) {

  int cur;

  if ( !target ) return 0;

  /* Number of threads in the current team */
  const unsigned long long rank = (unsigned long long)target->rank;
  const unsigned long long num_threads =
      (unsigned long long)t->info.num_threads;

  sctk_assert(t->info.loop_infos.type == MPCOMP_LOOP_TYPE_ULL);
  mpcomp_loop_ull_iter_t *loop = &(t->info.loop_infos.loop.mpcomp_ull);

  cur = __mpcomp_loop_dyn_get_chunk_from_target(t, target);

  if (cur < 0) {
    return 0;
  }

  __mpcomp_static_schedule_get_specific_chunk_ull(rank, num_threads, loop,
                                              cur, from, to);
  return 1;
}

/****
 *
 * ULL VERSION
 *
 *
 *****/
void __mpcomp_dynamic_loop_init_ull(mpcomp_thread_t *t, bool up,
                                    unsigned long long lb, unsigned long long b,
                                    unsigned long long incr,
                                    unsigned long long chunk_size) {
  mpcomp_loop_ull_iter_t *loop;
  /* Get the team info */
  sctk_assert(t->instance != NULL);
  sctk_assert(t->instance->team != NULL);

  /* WORKAROUND (pr35196.c)
   * Reinitialize the flag for the last iteration of the loop */
  t->for_dyn_last_loop_iteration = 0;

  const int num_threads = t->info.num_threads;
  const int index = __mpcomp_loop_dyn_get_for_dyn_index(t);

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_DYN_LOOP;
  t->schedule_is_forced = 0;

  /* Stop if the maximum number of alive loops is reached */
  while (sctk_atomics_load_int(
             &(t->instance->team->for_dyn_nb_threads_exited[index].i)) ==
         MPCOMP_NOWAIT_STOP_SYMBOL) {
    sctk_thread_yield();
  }

  /* Fill private info about the loop */

  t->info.loop_infos.type = MPCOMP_LOOP_TYPE_ULL;
  loop = &(t->info.loop_infos.loop.mpcomp_ull);
  loop->up = up;
  loop->lb = lb;
  loop->b = b;
  loop->incr = incr;
  loop->chunk_size = chunk_size;
  /* Try to change the number of remaining chunks */
  t->for_dyn_total[index] = __mpcomp_get_static_nb_chunks_per_rank_ull(
      t->rank, num_threads, &(t->info.loop_infos.loop.mpcomp_ull));
  (void) sctk_atomics_cas_int(&(t->for_dyn_remain[index].i), -1, t->for_dyn_total[index]);

}

int __mpcomp_loop_ull_dynamic_begin(bool up, unsigned long long lb,
                                    unsigned long long b,
                                    unsigned long long incr,
                                    unsigned long long chunk_size,
                                    unsigned long long *from,
                                    unsigned long long *to) {
  	mpcomp_thread_t *t; /* Info on the current thread */

  	/* Handle orphaned directive (initialize OpenMP environment) */
  	__mpcomp_init();

  	/* Grab the thread info */
  	t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  	sctk_assert(t != NULL);
    
    t->dyn_last_target = t ;

        if( !( t->instance->root ) )
    {
        *from = lb;
        *to = b;
  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_DYN_LOOP;
  t->schedule_is_forced = 0;
        return 1;
    }

  	/* Initialization of loop internals */
  	t->for_dyn_last_loop_iteration = 0;
  	__mpcomp_dynamic_loop_init_ull(t, up, lb, b, incr, chunk_size);

#if OMPT_SUPPORT
	/* Avoid double call during runtime schedule policy */
	if( ( t->schedule_type == MPCOMP_COMBINED_DYN_LOOP ) && mpcomp_ompt_is_enabled() )
	{
   	if( OMPT_Callbacks )
   	{
			ompt_callback_work_t callback; 
			callback = (ompt_callback_work_t) OMPT_Callbacks[ompt_callback_work];
			if( callback )
			{
				uint64_t ompt_iter_count = 0;
				ompt_iter_count = __mpcomp_internal_loop_get_num_iters_gen(&(t->info.loop_infos));
				ompt_data_t* parallel_data = &( t->instance->team->info.ompt_region_data );
				const void* code_ra = __ompt_get_return_address(3);
				callback( ompt_worksharing_loop, ompt_scope_begin, parallel_data, NULL, ompt_iter_count, code_ra);
			}
		}
	}
#endif /* OMPT_SUPPORT */

  return (!from && !to) ? -1 : __mpcomp_loop_ull_dynamic_next(from, to);
}

/* DEBUG */
void __mpcomp_loop_ull_dynamic_next_debug(__UNUSED__ char *funcname) {
  int i;
  mpcomp_thread_t *t; /* Info on the current thread */

  /* Grab the thread info */
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);
  sctk_assert(t->instance);

  const int tree_depth = t->instance->tree_depth;

  for (i = 0; i < tree_depth; i++) {
    sctk_nodebug("[%d] %s:\ttarget[%d] = %d \t shift[%d] = %d", t->rank,
                 funcname, i, t->for_dyn_target[i], i, t->for_dyn_shift[i]);
  }
}

int __mpcomp_loop_ull_dynamic_next(unsigned long long *from,
                                   unsigned long long *to) {
    
    mpcomp_mvp_t* target_mvp; 
    mpcomp_thread_t *t, *target;
    int i, found, target_idx, barrier_nthreads, ret;

    /* Grab the thread info */
    t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
    sctk_assert( t != NULL );

    /* Stop the stealing at the following depth.
     * Nodes above this depth will not be traversed */
    const int num_threads = t->info.num_threads;

    /* 1 thread => no stealing, directly get a chunk */
    if (num_threads == 1)
        return __mpcomp_dynamic_loop_get_chunk_from_rank_ull(t, t, from, to);

    /* Check that the target is allocated */
    __mpcomp_loop_dyn_target_init(t);

    /*
     * WORKAROUND (pr35196.c):
     * Stop if the current thread already executed
     * the last iteration of the current loop
     */
    if( t->for_dyn_last_loop_iteration == 1 ) 
    {
        __mpcomp_loop_dyn_target_reset(t);
        return 0;
    }

    int *tree_base = t->instance->tree_base + 1;
    const int tree_depth = t->instance->tree_depth -1;
    const int max_depth = t->instance->root->depth;

    /* Compute the index of the target */
    target = t->dyn_last_target;
    found = 1;
    ret = 0;

    /* While it is not possible to get a chunk */
    while(!__mpcomp_dynamic_loop_get_chunk_from_rank_ull( t, target, from, to)) 
    {

      do_increase:

      if( ret ) 
      {
          found = 0;
          break;
      }

      ret = __mpcomp_loop_dyn_dynamic_increase(t->for_dyn_shift, tree_base, tree_depth, max_depth, 1);
      /* Add t->for_dyn_shift and t->mvp->tree_rank[i] w/out carry over and store it into t->for_dyn_target */
      __mpcomp_loop_dyn_dynamic_add(t->for_dyn_target, t->for_dyn_shift, t->mvp->tree_rank, tree_base, tree_depth, max_depth, 0);

      /* Compute the index of the target */
      target_idx = __mpcomp_loop_dyn_get_victim_rank(t);
      target_mvp = t->instance->mvps[target_idx].ptr.mvp;
      target = ( target_mvp ) ? target_mvp->threads : NULL;

      if( !( target ) ) goto do_increase;
      barrier_nthreads = target_mvp->father->barrier_num_threads;

      /* Stop if the target is not a launch thread */
      if (t->for_dyn_target[tree_depth - 1] >= barrier_nthreads)
        goto do_increase;
    }


  /* Did we exit w/ or w/out a chunk? */
  if (found) {

    /* WORKAROUND (pr35196.c):
     * to avoid issue w/ lastprivate and GCC code generation,
     * check if this is the last chunk to avoid further execution
     * for this thread.
     */
    if (*to == t->info.loop_infos.loop.mpcomp_ull.b) {
      t->for_dyn_last_loop_iteration = 1;
    }
    t->dyn_last_target = target ;
    return 1;
  }

  return 0;
}

/****
 *
 * ULL ORDERED VERSION
 *
 *
 *****/
int __mpcomp_loop_ull_ordered_dynamic_begin(bool up, unsigned long long lb,
                                            unsigned long long b,
                                            unsigned long long incr,
                                            unsigned long long chunk_size,
                                            unsigned long long *from,
                                            unsigned long long *to) {
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_DYN_LOOP;
  t->schedule_is_forced = 1;

  // TODO EXTEND MODIF WITH UP VALUE
  sctk_nodebug("[%d] %s: %d -> %d [%d] cs:%d", t->rank, __func__, lb, b, incr,
               chunk_size);
  const int res =
      __mpcomp_loop_ull_dynamic_begin(up, lb, b, incr, chunk_size, from, to);
  if (from) {
    t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
  }
  sctk_nodebug("[%d] %s: exit w/ res=%d", t->rank, __func__, res);
  return res;
}

int __mpcomp_loop_ull_ordered_dynamic_next(unsigned long long *from,
                                           unsigned long long *to) {
  mpcomp_thread_t *t;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  const int ret = __mpcomp_loop_ull_dynamic_next(from, to);
  t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
  return ret;
}

/****
 *
 * COHERENCY CHECKING
 *
 *
 *****/
#if 0
void
mpcomp_for_dyn_coherency_end_barrier() 
{
     mpcomp_thread_t *t ;
		 mpcomp_instance_t * instance ;
		 mpcomp_team_t * team ;
		 int i ;
		 int n ;

     /* Grab info on the current thread */
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

		 sctk_nodebug( "[%d] __mpcomp_for_dyn_coherency_checking_end_barrier: "
				 "Checking for %d thread(s)...",
				 t->rank, t->info.num_threads ) ;

		 instance = t->instance ;
		 sctk_assert( instance != NULL ) ;

		 team = instance->team ;
		 sctk_assert( team != NULL ) ;

		 /* Check team coherency */
		 n = 0 ;
		 for ( i = 0 ; i < MPCOMP_MAX_ALIVE_FOR_DYN + 1 ; i++ ) {
			 switch( sctk_atomics_load_int( &team->for_dyn_nb_threads_exited[i].i) ) {
				 case 0:
					 break ;
				 case MPCOMP_NOWAIT_STOP_SYMBOL:
					 n++ ;
					 break ;
				 default:
					 /* This array should contain only '0' and 
						* one MPCOMP_MAX_ALIVE_FOR_DYN */
					 not_reachable() ;
					 break ;
			 }

			 sctk_nodebug( "[%d] __mpcomp_for_dyn_coherency_checking_end_barrier: "
					 "TEAM - FOR_DYN - nb_threads_exited[%d] = %d"
					 ,
					 t->rank, i, team->for_dyn_nb_threads_exited[i].i ) ;
					
		 }
		 
		 if ( n != 1 ) {
			 /* This array should contain exactly 1 MPCOMP_MAX_ALIVE_FOR_DYN */
			 not_reachable() ;
		 }

		 /* Check tree coherency */

		 /* Check thread coherency */
		 for ( i = 0 ; i < t->info.num_threads ; i++ )
		 {
			 mpcomp_thread_t * target_t ;
			 int j ;

			 target_t = &(instance->mvps[i].ptr.mvp->threads[0]) ;

			 sctk_nodebug( "[%d] __mpcomp_for_dyn_coherency_checking_end_barrier: "
					 "Checking thread %d...",
					 t->rank, target_t->rank ) ;

			 /* Checking for_dyn_current */
			 if ( t->for_dyn_current != target_t->for_dyn_current ) {
				 not_reachable() ;
			 }

			 /* Checking for_dyn_remain */
			 for ( j = 0 ; j < MPCOMP_MAX_ALIVE_FOR_DYN+1 ; j++ ) {
				 if ( sctk_atomics_load_int( &(target_t->for_dyn_remain[j].i) ) != -1 ) {
					 not_reachable() ;
				 }
			 }

			 /* Checking for_dyn_target */
			 /* for_dyn_tarfer can be NULL if there were no dynamic loop
				* since the beginning of the program
				*/
			 if ( target_t->for_dyn_target!= NULL ) {
				 for ( j = 0 ; j < instance->tree_depth ; j++ ) 
				 {
					 if ( target_t->for_dyn_target[j] != target_t->mvp->tree_rank[j] ) {
						 sctk_nodebug( "[%d] __mpcomp_for_dyn_coherency_checking_end_barrier: "
								 "error rank %d has target[%d] = %d (depth: %d, max_depth: %d, for_dyn_current=%d)",
								 t->rank,
								 target_t->rank, j, 
								 target_t->for_dyn_target[j],
								instance->tree_depth, 
								team->info.new_root->depth,
							 target_t->for_dyn_current	) ;

						 not_reachable() ;
					 }


				 } 
			 }

			 /* Checking for_dyn_shift */
			 /* for_dyn_shift can be NULL if there were no dynamic loop
				* since the beginning of the program
				*/
			 if ( target_t->for_dyn_shift != NULL ) {
				 // for ( j = t->info.new_root->depth ; j < instance->tree_depth ; j++ ) 
				 for ( j = 0 ; j < instance->tree_depth ; j++ ) 
				 {
					 if ( target_t->for_dyn_shift[j] != 0 ) {
						 sctk_nodebug( "[%d] __mpcomp_for_dyn_coherency_checking_end_barrier: "
								 "error rank %d has shift[%d] = %d (dept: %d, max_depth: %d, for_dyn_current=%d)",
								 t->rank,
								 target_t->rank, j, 
								 target_t->for_dyn_shift[j],
								instance->tree_depth, 
								t->info.new_root->depth,
							 target_t->for_dyn_current	) ;

						 not_reachable() ;
					 }
				 } 
			 }

		 }
}
#endif
