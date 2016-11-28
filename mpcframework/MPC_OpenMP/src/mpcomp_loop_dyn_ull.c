
#include "sctk_debug.h"
#include "sctk_alloc.h"
#include "mpcomp_types.h"

/****
 *
 * ULL CHUNK MANIPULATION
 *
 *
 *****/

/* From thread t, try to steal a chunk from thread target 
 * Returns 1 on success, 0 otherwise */
static int __mpcomp_dynamic_loop_get_chunk_from_rank_ull( mpcomp_thread_t * t, mpcomp_thread_t * target, unsigned long long * from, unsigned long long * to ) 
{
    mpcomp_loop_ull_iter_t* loop; 
	int r, index, target_index, num_threads;

	//sctk_error( "%s: Get chunk from thread %d (current %d) to thread %d (current %d)",  __func__, t->rank, t->for_dyn_current, target->rank, target->for_dyn_current ) ;

	/* Stop if target rank has already done this specific loop */
	if ( (t != target) && (t->for_dyn_current < target->for_dyn_current) ) 
    {
		return 0 ;
	}

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	sctk_assert( index >= 0 && index < (MPCOMP_MAX_ALIVE_FOR_DYN + 1) ) ;

	/* Get the current id of remaining chunk for the target */
	r = sctk_atomics_load_int( &(target->for_dyn_remain[index].i) ) ;

    loop = &(t->info.loop_infos.loop.mpcomp_ull);

	/* r==-1 => Initialize if the target did not reach the current loop already */
	if ( r == -1 &&  ( (t == target) || (t->for_dyn_current > target->for_dyn_current)) ) 
    {
		/* Compute the total number of chunks for this thread */
        t->for_dyn_total = __mpcomp_compute_static_nb_chunks_per_rank_ull( target->rank, num_threads, loop);
            
		/* Try to change the number of remaining chunks */
		r = sctk_atomics_cas_int(&(target->for_dyn_remain[index].i), -1, t->for_dyn_total);

		sctk_assert( index >= 0 && index < (MPCOMP_MAX_ALIVE_FOR_DYN + 1) ) ;
	}

	/* r>0 => some chunks are remaining, we can try to steal */
	if ( r > 0 ) 
    {
		int real_r ;

		/* TEMP 
		 * TODO need to update this value only when we change the target */
        t->for_dyn_total = __mpcomp_compute_static_nb_chunks_per_rank_ull( target->rank, num_threads, loop);

		real_r = sctk_atomics_cas_int( &(target->for_dyn_remain[index].i), r, r-1 ) ;

		while ( real_r > 0 && real_r != r ) 
        {
			sctk_nodebug("[%d]%s: target=%d, r > 0, real_r=%d", t->rank, __func__, target->rank, real_r);
			r = real_r ;
			real_r = sctk_atomics_cas_int( &(target->for_dyn_remain[index].i), r, r-1 ) ;
			sctk_assert( index >= 0 && index < (MPCOMP_MAX_ALIVE_FOR_DYN + 1) ) ;
		}

		if ( real_r > 0 ) 
        {
            __mpcomp_static_schedule_get_specific_chunk_ull( target->rank, num_threads, loop, t->for_dyn_total - r, from, to) ;
			
            //sctk_error( "[%d] %s: Get a chunk out of %d from rank %llu %llu -> %llu", t->rank, __func__, t->for_dyn_total, target->rank, *from, *to ) ;

			/* SUCCESS */
			return 1 ;
		}
	}

	/* FAIL */
	return 0 ;
}

/****
 *
 * ULL VERSION
 *
 *
 *****/
void 
mpcomp_dynamic_loop_init_ull(mpcomp_thread_t *t, bool up, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size)
{
	mpcomp_team_t *team_info;	/* Info on the team */
	int index, r, num_threads;
    mpcomp_loop_ull_iter_t* loop; 
	
    /* Get the team info */
	sctk_assert(t->instance != NULL);
	team_info = t->instance->team;
	sctk_assert(team_info != NULL);

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	/* WORKAROUND (pr35196.c)
	 * Reinitialize the flag for the last iteration of the loop */
	t->for_dyn_last_loop_iteration = 0 ;

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	sctk_nodebug( "[%d] %s: loop %llu -> %llu [%llu] (CS:%llu) with for_dyn_current %d", t->rank, __func__, lb, b, incr, chunk_size, t->for_dyn_current ) ;

	/* Stop if the maximum number of alive loops is reached */
	while (sctk_atomics_load_int( &(team_info->for_dyn_nb_threads_exited[index].i)) == MPCOMP_NOWAIT_STOP_SYMBOL) 
    {
		sctk_thread_yield() ;
	}

	/* Fill private info about the loop */
    t->info.loop_infos.type = MPCOMP_LOOP_TYPE_ULL;
    loop = &( t->info.loop_infos.loop.mpcomp_ull );
    loop->up = up;
    loop->lb = lb; 
    loop->b = b;
    loop->incr = incr;
    loop->chunk_size = chunk_size;

    /* Compute the total number of chunks for this thread */
	t->for_dyn_total = __mpcomp_compute_static_nb_chunks_per_rank_ull(t->rank, num_threads, loop );

	/* Try to change the number of remaining chunks */
	sctk_atomics_cas_int(&(t->for_dyn_remain[index].i), -1, t->for_dyn_total);
}

int mpcomp_loop_ull_dynamic_begin (bool up, unsigned long long lb, unsigned long long b, unsigned long long incr,
				 unsigned long long chunk_size, unsigned long long *from, unsigned long long *to)
{ 
	mpcomp_thread_t *t ;	/* Info on the current thread */

	/* Handle orphaned directive (initialize OpenMP environment) */
	mpcomp_init();

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

    t->schedule_type = ( t->schedule_is_forced ) ? t->schedule_type : MPCOMP_COMBINED_DYN_LOOP;  
    t->schedule_is_forced = 0;

	/* Initialization of loop internals */
	mpcomp_dynamic_loop_init_ull(t, up, lb, b, incr, chunk_size);

	/* Return the next chunk to execute */
	return mpcomp_loop_ull_dynamic_next(from, to);
}

/* DEBUG */
void mpcomp_loop_ull_dynamic_next_debug( char* funcname ) 
{
	int i;
	mpcomp_thread_t *t ;	/* Info on the current thread */

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;
    sctk_assert( t->instance );

    const int tree_depth = t->instance->tree_depth;
    
	for ( i = 0 ; i < tree_depth ; i++ ) 
    { 
        sctk_nodebug("[%d] %s:\ttarget[%d] = %d \t shift[%d] = %d", t->rank, funcname, i,  t->for_dyn_target[i], i,  t->for_dyn_shift[i]); 
	}

}

void mpcomp_loop_dynamic_target_init( mpcomp_thread_t* thread ) 
{
    int i ;

    sctk_assert( thread );

    if( thread->for_dyn_target )
        return;

    sctk_assert( thread->instance );
    const int tree_depth = thread->instance->tree_depth;

    thread->for_dyn_target = (int *) malloc( tree_depth * sizeof( int ) ) ;
    sctk_assert( thread->for_dyn_target );

	thread->for_dyn_shift = (int *) malloc( tree_depth * sizeof( int ) ) ;
    sctk_assert( thread->for_dyn_shift );
    
    sctk_assert( thread->mvp );
    sctk_assert( thread->mvp->tree_rank );

    for ( i = 0 ; i < tree_depth ; i++ ) 
    {
		thread->for_dyn_shift[i] = 0 ;
	    thread->for_dyn_target[i] = thread->mvp->tree_rank[i];
	}

	sctk_nodebug( "[%d] %s: initialization of target and shift", thread->rank, __func__ );
}

int mpcomp_loop_dynamic_get_victim_rank(  mpcomp_thread_t* thread )
{
    int i, target;

    sctk_assert( thread );
    sctk_assert( thread->instance );

    const int tree_depth = thread->instance->tree_depth;

    sctk_assert( thread->for_dyn_target );
    sctk_assert( thread->instance->tree_cumulative );

	for ( i = 0, target = 0 ; i < tree_depth ; i++ ) 
    {
		target += thread->for_dyn_target[i] * thread->instance->tree_cumulative[i] ;
	}

	sctk_assert( target >= 0 ) ;
	sctk_assert( target < thread->instance->nb_mvps ) ;
	//sctk_error( "[%d] %s: targetting mvp #%d", thread->rank, __func__, target );
	
    return target;
}

void mpcomp_loop_dynamic_target_reset( mpcomp_thread_t* thread ) 
{
    int i;
    sctk_assert( thread );

    sctk_assert( thread->instance );
    const int tree_depth = thread->instance->tree_depth;

    sctk_assert( thread->mvp );
    sctk_assert( thread->mvp->tree_rank );
    
    sctk_assert( thread->for_dyn_target );
    sctk_assert( thread->for_dyn_shift );

    /* Re-initialize the target to steal */
	for( i = 0 ; i < tree_depth ; i++ ) 
    {
	    thread->for_dyn_shift[i] = 0 ;
	    thread->for_dyn_target[i] = thread->mvp->tree_rank[i] ;
    }
}

int mpcomp_loop_ull_dynamic_next (unsigned long long *from, unsigned long long *to)
{
	mpcomp_thread_t *t, *target_thread;
	int num_threads, index, index_target, barrier_num_threads;

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	//sctk_error("[%d] %s: start", t->rank, __func__ );   

	/* Stop the stealing at the following depth.
	 * Nodes above this depth will not be traversed */
	const int max_depth = t->info.new_root->depth;
	sctk_nodebug( "[%d] %s: max_depth = %d", t->rank, __func__, max_depth );

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	/* 1 thread => no stealing, directly get a chunk */
	if ( num_threads == 1 ) 
    {
		return __mpcomp_dynamic_loop_get_chunk_from_rank_ull( t, t, from, to);
	}

	/* Check that the target is allocated */
    mpcomp_loop_dynamic_target_init( t );
    mpcomp_loop_ull_dynamic_next_debug( __func__ );

	/* Compute the index of the target */
    index_target = mpcomp_loop_dynamic_get_victim_rank( t );
    barrier_num_threads = t->instance->mvps[index_target]->father->barrier_num_threads;

    /* 
     * WORKAROUND (pr35196.c):
     * Stop if the current thread already executed 
     * the last iteration of the current loop 
     */
	if ( t->for_dyn_last_loop_iteration == 1 ) 
    {
        mpcomp_loop_dynamic_target_reset( t ); 
		return 0 ;
	}

	int found = 1 ;

    const int* tree_base = t->instance->tree_base;
    const int tree_depth = t->instance->tree_depth;

	/* While it is not possible to get a chunk */
	while ( !__mpcomp_dynamic_loop_get_chunk_from_rank_ull( t, &(t->instance->mvps[index_target]->threads[0]), from, to ) ) 
    {

		sctk_nodebug("[%d] %s: Get chunk failed from %d -> Inside main while loop",	t->rank, __func__, index_target);

do_increase:
		if (__mpcomp_dynamic_increase( t->for_dyn_shift, tree_base, tree_depth, max_depth, 1)) 
        {
			sctk_nodebug( "[%d] %s: Target tour done", t->rank, __func__ );
			/* Initialized target to the curent thread */
			/* TODO maybe find another solution because it can be time consuming */
			__mpcomp_dynamic_add( t->for_dyn_target, t->for_dyn_shift, t->mvp->tree_rank, tree_base, tree_depth, max_depth, 0 );
			found = 0 ;
			break ;
		}

		/* Add t->for_dyn_shift and t->mvp->tree_rank[i] w/out
		 * carry over and store it into t->for_dyn_target */
		__mpcomp_dynamic_add( t->for_dyn_target, t->for_dyn_shift, t->mvp->tree_rank, tree_base, tree_depth, max_depth, 0);

		/* Compute the index of the target */
        index_target = mpcomp_loop_dynamic_get_victim_rank( t );
        barrier_num_threads = t->instance->mvps[index_target]->father->barrier_num_threads;

		sctk_nodebug( "[%d] %s : tree_depth=%d, target[%d]=%d, index_target=%d, b_n_threads=%d", t->rank, __func__, tree_depth, tree_depth-1, t->for_dyn_target[tree_depth-1], index_target, barrier_num_threads) ;

		/* Stop if the target is not a launch thread */
		if ( t->for_dyn_target[tree_depth-1] >= barrier_num_threads ) 
        {
			sctk_nodebug( "[%d] %s: target not in scope, skipping...", t->rank, __func__ );
			/* TODO promote this goto to a loop */
			goto do_increase ;
		}

		sctk_nodebug( "[%d] %s: targetting now mvp #%d", t->rank, __func__, index_target);
	}


	/* Did we exit w/ or w/out a chunk? */
	if ( found ) 
    {
	    sctk_nodebug( "[%d] %s: Exiting main while loop with chunk [%llu -> %llu] from rank %d", t->rank, __func__, *from, *to, index_target);

		/* WORKAROUND (pr35196.c):
		 * to avoid issue w/ lastprivate and GCC code generation,
		 * check if this is the last chunk to avoid further execution 
		 * for this thread.
		 */
		if ( *to == t->info.loop_infos.loop.mpcomp_ull.b ) 
        { 
			t->for_dyn_last_loop_iteration = 1 ;
		}

		return 1 ;
	}

	//sctk_error( "[%d] %s: Exiting with no chunks", t->rank, __func__ );
	sctk_nodebug( "[%d] %s: exit - shift[%d] = %d", t->rank, __func__, 1, t->for_dyn_shift[1] ); 
	return 0 ;
}

/****
 *
 * ULL ORDERED VERSION
 *
 *
 *****/
int mpcomp_loop_ull_ordered_dynamic_begin(unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size, unsigned long long *from, unsigned long long *to) 
{
    mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);  
     
    t->schedule_type = ( t->schedule_is_forced ) ? t->schedule_type : MPCOMP_COMBINED_DYN_LOOP;  
    t->schedule_is_forced = 0;

    //TODO EXTEND MODIF WITH UP VALUE
    sctk_nodebug( "[%d] %s: %d -> %d [%d] cs:%d", t->rank, __func__, lb, b, incr, chunk_size ) ;
    const int res = mpcomp_loop_ull_dynamic_begin( 0, lb, b, incr, chunk_size, from, to); 
    t->current_ordered_iteration = *from;
    sctk_nodebug( "[%d] %s: exit w/ res=%d", t->rank, __func__, res ) ;
     
    return res; 
}

int
mpcomp_loop_ull_ordered_dynamic_next(unsigned long long *from, unsigned long long *to)
{
     mpcomp_thread_t *t;
     int res ;

     res = mpcomp_loop_ull_dynamic_next(from, to);

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     t->current_ordered_iteration = *from;

     return res;
}

/****
 *
 * COHERENCY CHECKING
 *
 *
 *****/

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

			 target_t = &(instance->mvps[i]->threads[0]) ;

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

