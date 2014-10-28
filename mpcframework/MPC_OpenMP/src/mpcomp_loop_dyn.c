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

/* dest = src1+src2 in base 'base' of size 'depth' up to dimension 'max_depth'
 */
static int
__mpcomp_dynamic_add( int * dest, int * src1, int *src2, 
		int * base, int depth, int max_depth, 
		int include_carry_over ) {
		int i ;
		int ret = 1 ;
		int carry_over = 1 ; 

		/* Step to the next target */
		for ( i = depth -1 ; i >= max_depth ; i-- ) {
			int value ;

			value = src1[i] + src2[i];

			carry_over = 0 ;
			dest[i] = value ;
			if ( value >= base[i] ) {
				if ( i == max_depth ) {
					ret = 0 ;
				}
				if ( include_carry_over ) {
					carry_over = value / base[i] ;
				}
				dest[i] = (value %  base[i] ) ;
			}
		}

		return ret ;
}

/* Return 1 if overflow, otherwise 0 */
static int
__mpcomp_dynamic_increase( int * target, int * base,
		int depth, int max_depth, int include_carry_over ) {
		int i ;
		int carry_over = 1;
		int ret = 0 ;

#if 0
		mpcomp_thread_t *t ;	/* Info on the current thread */
		t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
		if ( t->rank == 1 ) {

		sctk_debug( "__mpcomp_dynamic_increase[%d]: "
				"depth=%d, mac_depth=%d, include_carry_over=%d"
				,
				t->rank,
				depth, max_depth, include_carry_over ) ;
		for ( i = 0 ; i < depth ; i++ ) {
			sctk_debug( "__mpcomp_dynamic_increase[%d]: "
					"\ttarget[%d] = %d"
					,
					t->rank,
					i, target[i] ) ;
		}
		for ( i = 0 ; i < depth ; i++ ) {
			sctk_debug( "__mpcomp_dynamic_increase[%d]: "
					"\tbase[%d] = %d"
					,
					t->rank,
					i, base[i] ) ;
		}
		}
#endif

#if 0
		{
			mpcomp_thread_t *t ;	/* Info on the current thread */
			t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

			if ( t->rank == 0 ) {
				sctk_debug( "[%d] __mpcomp_dynamic_increase: "
						"shift[1] before %d",
						t->rank, target[1] ) ;
			}
		}
#endif


		/* Step to the next target */
		for ( i = depth -1 ; i >= max_depth ; i-- ) {
			int value ;

			value = target[i] + carry_over ;

			carry_over = 0 ;
			target[i] = value ;
			if ( value >= base[i] ) {
				if ( i == max_depth ) {
					ret = 1 ;
				}
				if ( include_carry_over ) {
					carry_over = value / base[i] ;
				}
				target[i] = (value %  base[i] ) ;
			}
		}

#if 0
		{
		mpcomp_thread_t *t ;	/* Info on the current thread */
		t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
		if ( t->rank == 1 ) {

		for ( i = 0 ; i < depth ; i++ ) {
			sctk_debug( "__mpcomp_dynamic_increase[%d]: "
					"\tFinal target[%d] = %d    (ret=%d)"
					,
					t->rank,
					i, target[i], ret ) ;
		}
		}
		}
#endif

#if 0
		{
			mpcomp_thread_t *t ;	/* Info on the current thread */
			t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;

			if ( t->rank == 0 ) {
				sctk_debug( "[%d] __mpcomp_dynamic_increase: "
						"shift[1] after %d",
						t->rank, target[1] ) ;
			}
		}
#endif

		return ret ;
}

/* From thread t, try to steal a chunk from thread target 
 * Returns 1 on success, 0 otherwise */
static int
__mpcomp_dynamic_loop_get_chunk_from_rank( mpcomp_thread_t * t, 
		mpcomp_thread_t * target, long * from, long * to ) {
	int r ;
	int index;
	int target_index ;
	int num_threads;


	sctk_nodebug(
			"[%d] __mpcomp_dynamic_loop_get_chunk_from_rank: "
			"Get chunk from thread %d (current %d) "
			"to thread %d (current %d)", 
			t->rank, t->rank, t->for_dyn_current,
			target->rank, target->for_dyn_current ) ;

#if 0
	TODO("DEBUGGING PURPOSE ONLY")
	/* TODO for debugging purpose only */
	if ( t != target && t->rank != 0 ) {
		return 0 ;
	}
#endif

#if 0
	/* Disable work-stealing */
	if ( t != target ) {
		return 0 ;
	}
#endif

	/* Stop if target rank has already done this specific loop */
	if ( (t != target) && (t->for_dyn_current < target->for_dyn_current) ) {
		return 0 ;
	}

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	sctk_assert( index >= 0 && index < (MPCOMP_MAX_ALIVE_FOR_DYN + 1) ) ;

	/* Get the current id of remaining chink for the target */
	r = sctk_atomics_load_int( &(target->for_dyn_remain[index].i) ) ;

	sctk_nodebug("[%d] __mpcomp_dynamic_loop_get_chunk_from_rank: target=%d, index=%d, r=%d", 
			t->rank, t->rank, target->rank, index, r);

	/* r==-1 => Initialize if the target did not reach the current loop already */
	if ( r == -1 &&  ( (t == target) || (t->for_dyn_current > target->for_dyn_current)) ) {
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

		sctk_assert( index >= 0 && index < (MPCOMP_MAX_ALIVE_FOR_DYN + 1) ) ;
	}

	/* r>0 => some chunks are remaining, we can try to steal */
	if ( r > 0 ) {
		int real_r ;

#if 1
		/* TEMP 
		 * TODO need to update this value only when we change the target */
		t->for_dyn_total = __mpcomp_get_static_nb_chunks_per_rank(
				target->rank, 
				num_threads, 
				t->info.loop_lb, 
				t->info.loop_b, 
				t->info.loop_incr, 
				t->info.loop_chunk_size);
#endif

		real_r = sctk_atomics_cas_int( 
				&(target->for_dyn_remain[index].i), r, r-1 ) ;

		while ( real_r > 0 && real_r != r ) {
			sctk_nodebug("[%d] __mpcomp_dynamic_loop_get_chunk_from_rank: target=%d, r > 0, real_r=%d", 
					t->rank, target->rank, real_r);
			r = real_r ;
			real_r = sctk_atomics_cas_int( 
					&(target->for_dyn_remain[index].i), r, r-1 ) ;

			sctk_assert( index >= 0 && index < (MPCOMP_MAX_ALIVE_FOR_DYN + 1) ) ;
		}

		if ( real_r > 0 ) {
			int chunk_id ;

			__mpcomp_get_specific_chunk_per_rank( target->rank, num_threads,
					t->info.loop_lb, t->info.loop_b, t->info.loop_incr, t->info.loop_chunk_size, 
					t->for_dyn_total - r, from, to ) ;

			sctk_nodebug( "[%d] __mpcomp_dynamic_loop_get_chunk_from_rank: Get a chunk #%d out of %d from rank %d %d -> %d", 
					t->rank, r, t->for_dyn_total, target->rank, *from, *to ) ;

			/* SUCCESS */
			return 1 ;
		}


	}

	/* FAIL */
	return 0 ;
}


void 
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

	/* WORKAROUND (pr35196.c)
	 * Reinitialize the flag for the last iteration of the loop */
	t->for_dyn_last_loop_iteration = 0 ;

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	sctk_nodebug( "[%d] __mpcomp_dynamic_loop_init:"
			"Entering for loop %ld -> %ld [%ld] (CS:%ld) with for_dyn_current %d",
			t->rank, lb, b, incr, chunk_size, t->for_dyn_current ) ;

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

#if 0
	/* TODO FOR DEBUGGIN PURPOSE */
	if ( t->for_dyn_shift != NULL ) {
		if ( t->for_dyn_shift[1] != 0 ) {
			fprintf( stderr, "[%d] __mpcomp_dynamic_loop_begin: "
					"error rank %d has shift[1] = %d\n",
					t->rank, t->rank, t->for_dyn_shift[1] ) ;
		}
	}
#endif

	/* Initialization of loop internals */
	__mpcomp_dynamic_loop_init(t, lb, b, incr, chunk_size);

	/* Return the next chunk to execute */
	return __mpcomp_dynamic_loop_next(from, to);
}

int
__mpcomp_dynamic_loop_next (long *from, long *to)
{
	mpcomp_thread_t *t ;	/* Info on the current thread */
	int num_threads;
	int index;
	int max_depth ;
	int i ;

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

	sctk_nodebug("[%d] __mpcomp_dynamic_loop_next: start", t->rank);   

	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	sctk_assert( index >= 0 && index < (MPCOMP_MAX_ALIVE_FOR_DYN+1) ) ;

	/* 1 thread => no stealing, directly get a chunk */
	if ( num_threads == 1 ) {
		return __mpcomp_dynamic_loop_get_chunk_from_rank(t,t,from,to) ;
	}

	/* Check that the target is allocated */
	if ( t->for_dyn_target == NULL ) {
		int i ;
		t->for_dyn_target = (int *)malloc( t->instance->tree_depth * sizeof( int ) ) ;

		for ( i = 0 ; i < t->instance->tree_depth ; i++ ) {
			t->for_dyn_target[i] = t->mvp->tree_rank[i] ;
		}

		t->for_dyn_shift = (int *)malloc( t->instance->tree_depth * sizeof( int ) ) ;

		for ( i = 0 ; i < t->instance->tree_depth ; i++ ) {
			t->for_dyn_shift[i] = 0 ;
		}

		sctk_nodebug( 
				"[%d] __mpcomp_dynamic_loop_next: "
				"initialization of target and shift", 
				t->rank ) ;

	}
	sctk_assert( t->for_dyn_target != NULL ) ;
	sctk_assert( t->for_dyn_shift != NULL ) ;

	/* DEBUG */
	int i_debug ;
	for ( i_debug = 0 ; i_debug < t->instance->tree_depth ; i_debug++ ) {
		sctk_nodebug(
				"[%d] __mpcomp_dynamic_loop_next: "
				"\ttarget[%d] = %d \t shift[%d] = %d", 
				t->rank, 
				i_debug, t->for_dyn_target[i_debug],
				i_debug, t->for_dyn_shift[i_debug] 
				) ;

	}


	/* Stop the stealing at the following depth.
	 * Nodes above this depth will not be traversed
	 */
	max_depth = t->info.new_root->depth ;

	sctk_nodebug( 
				"[%d] __mpcomp_dynamic_loop_next[%d]: "
				"max_depth = %d",
				t->rank, max_depth 
			) ;

	int found = 1 ;
	int index_target = 0 ;

	sctk_assert( t->instance->tree_cumulative != NULL ) ;

	/* Compute the index of the target */
	for ( i = 0 ; i < t->instance->tree_depth ; i++ ) {
		index_target += t->for_dyn_target[i] * t->instance->tree_cumulative[i] ;
	}

	sctk_nodebug( 
				"[%d] __mpcomp_dynamic_loop_next: "
				"targetting mvp #%d",
				t->rank, index_target
			) ;


	sctk_assert( index_target >= 0 ) ;
	sctk_assert( index_target < t->instance->nb_mvps ) ;

	/* 
	 * WORKAROUND (pr35196.c):
	 * Stop if the current thread already executed 
	 * the last iteration of the current loop */
	if ( t->for_dyn_last_loop_iteration == 1 ) {
		return 0 ;
	}

	/* While it is not possible to get a chunk */
	while ( !__mpcomp_dynamic_loop_get_chunk_from_rank( 
				t, 
				&(t->instance->mvps[index_target]->threads[0]), 
				from, 
				to ) ) {

		sctk_nodebug( 
				"[%d] __mpcomp_dynamic_loop_next: "
				"Get chunk failed from %d -> Inside main while loop", 
				t->rank, index_target ) ;


do_increase:
		if (__mpcomp_dynamic_increase( t->for_dyn_shift, t->instance->tree_base,
					t->instance->tree_depth, max_depth, 1 ) ) {

			sctk_nodebug( 
					"[%d] __mpcomp_dynamic_loop_next: "
					"Target tour done", t->rank ) ;

#if 0
			/* TODO DEBUGGING PURPOSE */
			if ( t->for_dyn_shift[1] != 0 ) {
				// fprintf( stderr, "EEEERRRRRR\n" ) ;
				sctk_debug( "EEEERRRRRR\n" ) ;
			}
#endif

			/* Initialized target to the curent thread */
			/* TODO maybe find another solution because it can be time consuming */
			__mpcomp_dynamic_add(  t->for_dyn_target, t->for_dyn_shift, t->mvp->tree_rank,
					t->instance->tree_base, t->instance->tree_depth, max_depth, 0 ) ;

#if 0
			/* TODO DEBUGGING PURPOSE */
			if ( t->for_dyn_shift[1] != 0 ) {
				// fprintf( stderr, "EEEERRRRRR 2\n" ) ;
				sctk_debug( "EEEERRRRRR 2\n" ) ;
			}
#endif

			found = 0 ;
			break ;
		}

		/* Add t->for_dyn_shift and t->mvp->tree_rank[i] w/out
		 * carry over and store it into t->for_dyn_target */
		__mpcomp_dynamic_add(  t->for_dyn_target, t->for_dyn_shift, t->mvp->tree_rank,
				t->instance->tree_base, t->instance->tree_depth, max_depth, 0 ) ;

		index_target = 0 ;
		/* Compute the index of the target */
		for ( i = 0 ; i < t->instance->tree_depth ; i++ ) {
			index_target += t->for_dyn_target[i] * t->instance->tree_cumulative[i] ;
		}

		sctk_assert( index_target >= 0 ) ;
		sctk_assert( index_target < t->instance->nb_mvps ) ;

		sctk_nodebug( "__mpcomp_dynamic_loop_next[%d]: "
			"tree_depth=%d, target[%d]=%d, index_target=%d, b_n_threads=%d"
			,
			t->rank,
			t->instance->tree_depth, t->instance->tree_depth-1, 
			t->for_dyn_target[t->instance->tree_depth-1],
			index_target,
			t->instance->mvps[index_target]->father->barrier_num_threads ) ;

		/* Stop if the target is not a launch thread */
		if ( t->for_dyn_target[t->instance->tree_depth-1] >= 
				t->instance->mvps[index_target]->father->barrier_num_threads ) {

			sctk_nodebug( 
					"__mpcomp_dynamic_loop_next[%d]: "
					"target not in scope, skipping..."
					, t->rank
					) ;

			/* TODO promote this goto to a loop */
			goto do_increase ;
		}


		sctk_nodebug( 
				"__mpcomp_dynamic_loop_next[%d]: "
				"targetting now mvp #%d",
				t->rank, index_target
				) ;
	}


	/* Did we exit w/ or w/out a chunk? */
	if ( found ) {
		sctk_nodebug( 
				"[%d] __mpcomp_dynamic_loop_next: "
				"Exiting main while loop with chunk [%d -> %d] from rank %d", t->rank, 
				*from, *to, index_target ) ;

		/* WORKAROUND (pr35196.c):
		 * to avoid issue w/ lastprivate and GCC code generation,
		 * check if this is the last chunk to avoid further execution 
		 * for this thread.
		 */
		if ( *to == t->info.loop_b ) { 
			t->for_dyn_last_loop_iteration = 1 ;
		}

		return 1 ;
	}

	sctk_nodebug( 
			"[%d] __mpcomp_dynamic_loop_next: "
			"Exiting with no chunks", t->rank ) ;

#if 0
	/*
	if ( t->rank == 2 ) {
		sleep(3) ;
	}
	*/
	/* TODO DEBUGGING PURPOSE */
	if ( t->for_dyn_shift[1] != 0 ) {
		fprintf( stderr, "[%d] __mpcomp_dynamic_loop_next: exit - shift[1] = %d\n",
				t->rank, t->for_dyn_shift[1]	) ;
		// not_reachable() ;
	} 
#endif

	return 0 ;
}

#if 0
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
#endif

void
__mpcomp_dynamic_loop_end_nowait ()
{
	mpcomp_thread_t *t ;	/* Info on the current thread */
	mpcomp_team_t *team_info ;	/* Info on the team */
	int index;
	int num_threads;
	int nb_threads_exited ; 

	/* Grab the thread info */
	t = (mpcomp_thread_t *) sctk_openmp_thread_tls ;
	sctk_assert( t != NULL ) ;

#if 0
			/* TODO DEBUGGING PURPOSE */
			if ( t->for_dyn_shift[1] != 0 ) {
				fprintf( stderr, "[%d] __mpcomp_dynamic_loop_end_nowait: begin - shift[1] = %d\n",
					 t->rank, t->for_dyn_shift[1]	) ;
				// not_reachable() ;
			} else {
				if ( t->rank != 0 ) {
					// sleep(1); 
				}
			}
#endif


	/* Number of threads in the current team */
	num_threads = t->info.num_threads;

	if (num_threads == 1) {
		/* In case of 1 thread, re-initialize the number of remaining chunks
		 * but do not increase the current index */
		index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);
		sctk_atomics_store_int( &(t->for_dyn_remain[index].i), -1 ) ;
		return ;
	}

	/* Get the team info */
	sctk_assert (t->instance != NULL);
	team_info = t->instance->team ;
	sctk_assert (team_info != NULL);

	/* Compute the index of the dynamic for construct */
	index = (t->for_dyn_current) % (MPCOMP_MAX_ALIVE_FOR_DYN + 1);

	sctk_assert( index >= 0 && index < MPCOMP_MAX_ALIVE_FOR_DYN + 1 ) ;

	sctk_nodebug("[%d] __mpcomp_dynamic_loop_end_nowait: begin", t->rank);

	/* WARNING: the following order is important */
	t->for_dyn_current++ ;
	sctk_atomics_store_int( &(t->for_dyn_remain[index].i), -1 ) ;

	/* Update the number of threads which ended this loop */
	nb_threads_exited = sctk_atomics_fetch_and_incr_int( 
			&(team_info->for_dyn_nb_threads_exited[index].i ) ) ;

	sctk_nodebug( 
			"[%d]__mpcomp_dynamic_loop_end_nowait: Exiting loop %d: %d -> %d",
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

		sctk_assert( previous_index >= 0 && previous_index < MPCOMP_MAX_ALIVE_FOR_DYN + 1 ) ;

		sctk_nodebug( "__mpcomp_dynamic_loop_end_nowait: Move STOP symbol %d -> %d", previous_index, index ) ;

		sctk_atomics_store_int( 
				&(team_info->for_dyn_nb_threads_exited[previous_index].i), 
				0 ) ;
	}

}

void
__mpcomp_dynamic_loop_end ()
{
	sctk_nodebug( "__mpcomp_dynamic_loop_end:" ) ;
	__mpcomp_dynamic_loop_end_nowait() ;
	__mpcomp_barrier ();

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
     mpcomp_thread_t *t;
     int res;

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);  
     

	 sctk_nodebug( "[%d] __mpcomp_dynamic_loop_begin: %d -> %d [%d] cs:%d",
			 t->rank, lb, b, incr, chunk_size ) ;

     res = __mpcomp_dynamic_loop_begin(lb, b, incr, chunk_size, from, to);

     t->current_ordered_iteration = *from;

	 sctk_nodebug( "[%d] __mpcomp_dynamic_loop_begin: exit w/ res=%d",
			 t->rank, res ) ;
     
     return res;
}

int
__mpcomp_ordered_dynamic_loop_next(long *from, long *to)
{
     mpcomp_thread_t *t;
     int res ;
     
     res = __mpcomp_dynamic_loop_next(from, to);
     
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);
     
     t->current_ordered_iteration = *from;
     
     return res;
}

void
__mpcomp_ordered_dynamic_loop_end()
{
     __mpcomp_dynamic_loop_end();
}

void
__mpcomp_ordered_dynamic_loop_end_nowait()
{
     __mpcomp_dynamic_loop_end();
}

void
__mpcomp_for_dyn_coherency_end_barrier() 
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
			 /* This arrau should contain exactly 1 MPCOMP_MAX_ALIVE_FOR_DYN */
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

			 /* Checking for_dyn_shift */
			 /* for_dyn_shift can be NULL if there were no dynamic loop
				* since the beginning of the program
				*/
			 if ( target_t->for_dyn_shift != NULL ) {
				 // for ( j = t->info.new_root->depth ; j < instance->tree_depth ; j++ ) 
				 for ( j = 0 ; j < instance->tree_depth ; j++ ) 
				 {
					 if ( target_t->for_dyn_shift[j] != 0 ) {
						 sctk_debug( "[%d] __mpcomp_for_dyn_coherency_checking_end_barrier: "
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
