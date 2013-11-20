/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpcomp.h>
#include <mpcomp_abi.h>
#include "mpcomp_internal.h"
#include <sctk_debug.h>

/* From thread t, try to steal a chunk from thread target 
 * Returns 1 on success, 0 otherwise */
static int
__mpcomp_dynamic_loop_get_chunk_from_rank( mpcomp_thread_t * t, mpcomp_thread_t * target, 
		long * from, long * to ) {
	int r ;
	int index;
	int target_index ;

	/* Stop if target rank has already done this specific loop */
	if ( t->for_dyn_current < target->for_dyn_current ) {
		return 0 ;
	}

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	/* Get the current id of remaining chink for the target */
	r = sctk_atomics_load_int( &(target->for_dyn_remain[index].i) ) ;

	sctk_nodebug("[%d] __mpcomp_dynamic_loop_get_chunk_from_rank: target=%d, index=%d, r=%d", 
			t->rank, target->rank, index, target->for_dyn_chunk_info[index].remain);

	/* r==-1 => Initialize if the target did not reach the current loop already */
	if ( r == -1 &&  (t->for_dyn_current > target->for_dyn_current) ) {
		/* Compute the total number of chunks for this thread */
		t->for_dyn_total = __mpcomp_get_static_nb_chunks_per_rank(
				target->rank, 
				num_threads, 
				t->info.loop_lb, 
				t->info.loop_b, 
				t->info.loop_incr, 
				t->info.loop_chunk_size);

		/* Try to change the number of remaining chunks */
		r = sctk_atomics_cas_int(&(target->for_dyn_remain[index].i), -1, t->for_dyn_total);
	}

	/* r>0 => some chunks are remaining, we can try to steal */
	if ( r > 0 ) {
		int real_r ;

		real_r = sctk_atomics_cas_int( 
				&(target->for_dyn_remain[index].i), r, r-1 ) ;

		while ( real_r > 0 && real_r != r ) {
			sctk_nodebug("[%d] __mpcomp_dynamic_loop_get_chunk_from_rank: target=%d, r > 0, real_r=%d", 
					t->rank, target->rank, real_r);
			r = real_r ;
			real_r = sctk_atomics_cas_int( 
					&(target->for_dyn_remain[index].i), r, r-1 ) ;
		}

		if ( real_r > 0 ) {
			int chunk_id ;

			__mpcomp_get_specific_chunk_per_rank( target->rank, num_threads,
					t->info.loop_lb, t->info.loop_b, t->info.loop_incr, t->info.loop_chunk_size, 
					t->for_dyn_total - r, from, to ) ;

			sctk_nodebug( "[%d] __mpcomp_dynamic_loop_get_chunk_from_rank: Get a chunk %d -> %d", 
					t->rank, *from, *to ) ;

			/* SUCCESS */
			return 1 ;
		}


	}

	/* FAIL */
	return 0 ;
}

static int 
__mpcomp_dynamic_loop_steal( mpcomp_thread_t * t, long * from, long * to ) {
	int i ;
	int stop = 0 ; 
	int max_depth ; 

	/* Note: max_depth is a mix of new_root's depth and an artificial depth
	 * set to reduce the stealing frontiers */

	TODO( "__mpcomp_dynamic_loop_steal: Add artificial depth to limit stealing frontiers" )

	sctk_debug( "__mpcomp_dynamic_loop_steal[%d]: Enter", t->rank ) ;
	sctk_debug( "__mpcomp_dynamic_loop_steal[%d]: depth of new_root = %d",
			t->rank, t->info.new_root->depth ) ;

	/* Check that the target is allocated */
	if ( t->for_dyn_target == NULL ) {
		t->for_dyn_target = (int *)malloc( t->instance->tree_depth * sizeof( int ) ) ;
		for ( i = 0 ; i < t->instance->tree_depth ; i++ ) {
			t->for_dyn_target[i] = 0 ;
		}
	}
	sctk_assert( t->for_dyn_target != NULL ) ;

	/* Stop the stealing at the following depth.
	 * Nodes above this depth will not be traversed
	 */
	max_depth = t->info.new_root->depth ;

	while ( !stop ) {

	/* Increase target index */
	int carry_over = 1;
	for ( i = t->instance->tree_depth -1 ; i >= max_depth ; i-- ) {
		int value ;

		value = t->for_dyn_target[i] + carry_over ;

		carry_over = 0 ;
		t->for_dyn_target[i] = value ;
		if ( value >= t->instance->tree_base[i] ) {
			carry_over = value / t->instance->tree_base[i] ;
			t->for_dyn_target[i] = (value %  (t->instance->tree_base[i]) ) ;
		}
	}

#if 0
	fprintf( stderr, "__mpcomp_dynamic_loop_steal[%ld]: new victim = [",
			t->rank ) ;
	for ( i = 0 ; i < t->instance->tree_depth ; i++ ) {
		if ( i != 0 ) {
			fprintf( stderr, ", " ) ;
		}
		fprintf( stderr, "%d", t->for_dyn_target[i] ) ;
	}
	fprintf( stderr, "]\n" ) ;
#endif

	/* Add current microvp coordinates to select the target */
	for ( i = max_depth ; i < t->instance->tree_depth ; i++ ) {
		/* Add t->for_dyn_target[i] and t->mvp->tree_rank */

	}

	/* Compute the index of the target mvp according to its coordinates */

	/* Try to steal from this mvp */


	/* Check if target is 0 */
	stop = 1 ;
	for ( i = 0 ; i < t->instance->tree_depth ; i++ ) {
		if ( t->for_dyn_target[i] != 0 ) {
			stop = 0 ;
		}
	}
	}

	sctk_debug( "__mpcomp_dynamic_loop_steal[%d]: Exit", t->rank ) ;

	return 0 ;
}

static void 
__mpcomp_dynamic_loop_init(mpcomp_thread_t *t, 
		long lb, long b, long incr, long chunk_size)
{
	mpcomp_team_t *team_info;	/* Info on the team */
	int index;
	int r;
	int num_threads;

	/* Get the team info */
	sctk_assert(t->instance != NULL);
	team_info = t->instance->team;
	sctk_assert(team_info != NULL);

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	/* Stop if the maximum number of alive loops is reached */
	while (sctk_atomics_load_int( 
				&(team_info->for_dyn_nb_threads_exited[index].i)) == 
			MPCOMP_NOWAIT_STOP_SYMBOL) {
		sctk_thread_yield() ;
	}

	/* Fill private info about the loop */
	t->info.loop_lb = lb;
	t->info.loop_b = b;
	t->info.loop_incr = incr;
	t->info.loop_chunk_size = chunk_size;     

	/* Compute the total number of chunks for this thread */
	t->for_dyn_total = __mpcomp_get_static_nb_chunks_per_rank(t->rank, 
			num_threads, lb, b, incr, chunk_size);

	/* Try to change the number of remaining chunks */
	sctk_atomics_cas_int(&(t->for_dyn_remain[index].i), -1, t->for_dyn_total);

}


int __mpcomp_dynamic_loop_begin (long lb, long b, long incr,
				 long chunk_size, long *from, long *to)
{ 
	mpcomp_thread_t *t ;	/* Info on the current thread */

	/* Handle orphaned directive (initialize OpenMP environment) */
	__mpcomp_init();

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	/* Initialization of loop internals */
	__mpcomp_dynamic_loop_init(t, lb, b, incr, chunk_size);

	/* Return the next chunk to execute */
	return __mpcomp_dynamic_loop_next(from, to);
}

int
__mpcomp_dynamic_loop_next (long *from, long *to)
{
	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	sctk_nodebug("[%d] __mpcomp_dynamic_loop_next: start", t->rank);   

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	TODO( "__mpcomp_dynamic_loop_next: w/ 1 thread, put an if and remove CAS" )

	/* Check that the target is allocated */
	if ( t->for_dyn_target == NULL ) {
		t->for_dyn_target = (int *)malloc( t->instance->tree_depth * sizeof( int ) ) ;
		for ( i = 0 ; i < t->instance->tree_depth ; i++ ) {
			t->for_dyn_target[i] = 0 ;
		}
	}
	sctk_assert( t->for_dyn_target != NULL ) ;

	/* Stop the stealing at the following depth.
	 * Nodes above this depth will not be traversed
	 */
	max_depth = t->info.new_root->depth ;


	/**
	 * Main algo:
	 * if for_dyn_target is NULL, 
	 *    intialize for_dyn_shift to (0, ..., 0)
	 *    initialize for_dyn_target to current t->mvp->tree_rank
	 *
	 * while (
	 *   !__mpcomp_dynamic_loop_get_chunk_from_rank( t, t->for_dyn_target, from, to ) )
	 *     add 1 to for_dyn_shift (according to max_depth)
	 *     if for_dyn_shift == (0, ..., 0 )
	 *       break ;
	 *     add for_dyn_shift to for_dyn_target (according to max_depth)
	 *
	 *
	 */
}

#if 0
int
__mpcomp_dynamic_loop_next (long *from, long *to)
{
	mpcomp_thread_t *t ;	/* Info on the current thread */
	mpcomp_team_t *team_info ;	/* Info on the team */
	int index;
	int num_threads;
	int r ;

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	sctk_nodebug("[%d] __mpcomp_dynamic_loop_next: start", t->rank);   

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	TODO( "__mpcomp_dynamic_loop_next: w/ 1 thread, put an if and remove CAS" )

	/* Check what the status is to see if we have to steal */
	if ( t->for_dyn_status == 0 ) {

		r = sctk_atomics_load_int( &(t->for_dyn_remain[index].i) ) ;

		sctk_nodebug("[%d] __mpcomp_dynamic_loop_next: index=%d, r=%d", 
				t->rank, index, t->for_dyn_chunk_info[index].remain);

		if ( r > 0 ) {
			int real_r ;

			real_r = sctk_atomics_cas_int( 
					&(t->for_dyn_remain[index].i), r, r-1 ) ;

			while ( real_r > 0 && real_r != r ) {
				sctk_nodebug("[%d] __mpcomp_dynamic_loop_next: r > 0, real_r=%d", 
						t->rank, real_r);
				r = real_r ;
				real_r = sctk_atomics_cas_int( 
						&(t->for_dyn_remain[index].i), r, r-1 ) ;
			}

			if ( real_r > 0 ) {
				int chunk_id ;

				__mpcomp_get_specific_chunk_per_rank( t->rank, num_threads,
						t->info.loop_lb, t->info.loop_b, t->info.loop_incr, t->info.loop_chunk_size, 
						t->for_dyn_total - r, from, to ) ;

				sctk_nodebug( "__mpcomp_dynamic_loop_next: Get a chunk %d -> %d", *from, *to ) ;

				return 1 ;
			}
		}

		if ( num_threads > 1 ) {
			/* Entering STEAL state (update status and intialize) */
			t->for_dyn_status = 1 ;
		}

	}

	if ( num_threads > 1 ) {
		/* TODO stealing */
		__mpcomp_dynamic_loop_steal( t, from, to ) ;

	}

	/* Nothing to do and to steal */
	return 0 ;
}
#endif

void
__mpcomp_dynamic_loop_end ()
{
	mpcomp_thread_t *t ;	/* Info on the current thread */
	mpcomp_team_t *team_info ;	/* Info on the team */
	int index;
	int num_threads;
	int nb_threads_exited ; 

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	if (num_threads == 1) {
		return ;
	}

	/* Get the team info */
	sctk_assert (t->instance != NULL);
	team_info = t->instance->team ;
	sctk_assert (team_info != NULL);

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	sctk_nodebug("[%d] __mpcomp_dynamic_loop_end: begin", t->rank);

	/* WARNING: the following order is important */
	t->for_dyn_current++ ;
	sctk_atomics_store_int( &(t->for_dyn_remain[index].i), -1 ) ;

	/* Update the number of threads which ended this loop */
	nb_threads_exited = sctk_atomics_fetch_and_incr_int( 
			&(team_info->for_dyn_nb_threads_exited[index].i ) ) ;

	sctk_nodebug( 
			"[%d]__mpcomp_dynamic_loop_end: Exiting loop %d: %d -> %d",
			t->rank, index, nb_threads_exited, nb_threads_exited + 1 ) ;

	sctk_assert( nb_threads_exited >= 0 && nb_threads_exited < num_threads ) ;

	if ( nb_threads_exited == num_threads - 1 ) {
		int previous_index ;

		/* WARNING: the following order is important */
		sctk_atomics_store_int( 
				&(team_info->for_dyn_nb_threads_exited[index].i), 
				MPCOMP_NOWAIT_STOP_SYMBOL ) ;

		previous_index = (index - 1 + MPCOMP_MAX_ALIVE_FOR_DYN + 1 ) % 
			( MPCOMP_MAX_ALIVE_FOR_DYN + 1 ) ;

		sctk_nodebug( "__mpcomp_dynamic_loop_end: Move STOP symbol %d -> %d", previous_index, index ) ;

		sctk_atomics_store_int( 
				&(team_info->for_dyn_nb_threads_exited[previous_index].i), 
				0 ) ;
	}

}

void
__mpcomp_dynamic_loop_end_nowait ()
{
	__mpcomp_dynamic_loop_end() ;
}

int
__mpcomp_dynamic_loop_next_ignore_nowait (long *from, long *to)
{
     not_implemented() ;
     return 0;
}


/****
 *
 * ORDERED VERSION 
 *
 *
 *****/

int
__mpcomp_ordered_dynamic_loop_begin (long lb, long b, long incr, long chunk_size,
				     long *from, long *to)
{
     not_implemented() ;
     return -1;
}

int
__mpcomp_ordered_dynamic_loop_next(long *from, long *to)
{
     not_implemented() ;
     return -1;
}

void
__mpcomp_ordered_dynamic_loop_end()
{
     not_implemented() ;
}

void
__mpcomp_ordered_dynamic_loop_end_nowait()
{
     not_implemented() ;
}
