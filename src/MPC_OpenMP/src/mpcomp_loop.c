/* ############################# MPC License ############################## */
/* # Tue Oct 12 10:33:56 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - Adrien Roussel <adrien.roussel@cea.fr>                             # */
/* # - Antoine Capra <capra@paratools.com>                                # */
/* # - Augustin Serraz <augustin.serraz@exascale-computing.eu>            # */
/* # - Aurele Maheo <aurele.maheo@exascale-computing.eu>                  # */
/* # - Jean-Baptiste Besnard <jbbesnard@paratools.com>                    # */
/* # - Julien Adam <adamj@paratools.com>                                  # */
/* # - Patrick Carribault <patrick.carribault@cea.fr>                     # */
/* # - Romain Pereira <romain.pereira@cea.fr>                             # */
/* # - Souad Koliai <skoliai@exascale-computing.eu>                       # */
/* # - Sylvain Didelot <sylvain.didelot@exascale-computing.eu>            # */
/* # - Thomas Dionisi <thomas.dionisi@exascale-computing.eu>              # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_omp.h>
#include "mpc_common_debug.h"
#include "mpcomp_core.h"
#include "mpcomp_types.h"
#include "mpcomp_loop.h"
#include "mpcomp_openmp_tls.h"
#include "mpcomp_sync.h"
#include "mpcomp_task.h"
#include "mpcompt_workShare.h"
#include "mpcompt_dispatch.h"
#include "mpcompt_frame.h"

static inline int __loop_dyn_get_for_dyn_current( struct mpc_omp_thread_s *thread )
{
	return OPA_load_int( &( thread->for_dyn_ull_current ) );
}

static inline int __loop_dyn_get_for_dyn_prev_index( struct mpc_omp_thread_s *thread )
{
	const int for_dyn_current = __loop_dyn_get_for_dyn_current( thread );
	return ( for_dyn_current + MPC_OMP_MAX_ALIVE_FOR_DYN - 1 ) %
	       ( MPC_OMP_MAX_ALIVE_FOR_DYN + 1 );
}

static inline int __loop_dyn_get_for_dyn_index( struct mpc_omp_thread_s *thread )
{
	const int for_dyn_current = __loop_dyn_get_for_dyn_current( thread );
	return ( for_dyn_current % ( MPC_OMP_MAX_ALIVE_FOR_DYN + 1 ) );
}

/*******************
 * LOOP CORE TYPES *
 *******************/

/* Chunk Manipulation */

static inline unsigned long long __loop_get_num_iters_ull( unsigned long long start,
        unsigned long long end,
        unsigned long long step, bool up )
{
	unsigned long long ret = ( unsigned long long ) 0;
	ret = ( up && start < end )
	      ? ( end - start + step - ( unsigned long long ) 1 ) / step
	      : ret;
	ret = ( !up && start > end )
	      ? ( start - end - step - ( unsigned long long ) 1 ) / -step
	      : ret;
	return ret;
}

static inline long __loop_get_num_iters( long start, long end,
        long step )
{
	long ret = 0;
	const bool up = ( step > 0 );
	ret = ( up && start < end ) ? ( end - start + step - ( long ) 1 ) / step : ret;
	ret = ( !up && start > end ) ? ( start - end - step - ( long ) 1 ) / -step : ret;
	return ( ret >= 0 ) ? ret : -ret;
}

static inline uint64_t __loop_get_num_iters_gen( mpc_omp_loop_gen_info_t *loop_infos )
{
	uint64_t count = 0;

	if ( loop_infos->type == MPC_OMP_LOOP_TYPE_LONG )
	{
		mpc_omp_loop_long_iter_t *long_loop = &( loop_infos->loop.mpcomp_long );
		count = __loop_get_num_iters( long_loop->lb, long_loop->b, long_loop->incr );
	}
	else
	{
		mpc_omp_loop_ull_iter_t *ull_loop = &( loop_infos->loop.mpcomp_ull );
		count = __loop_get_num_iters_ull( ull_loop->lb, ull_loop->b, ull_loop->incr, ull_loop->up );
	}

	return count;
}


/* Return the number of chunks that a static schedule will create for the thread 'rank' */
static inline int __loop_get_static_nb_chunks_per_rank( int rank, int num_threads,
        mpc_omp_loop_long_iter_t *loop )
{
	long nb_chunks_per_thread;
	const long trip_count =
	    __loop_get_num_iters( loop->lb, loop->b, loop->incr );
	/* Compute the number of chunks per thread (floor value) */
	loop->chunk_size =
	    ( loop->chunk_size ) ? loop->chunk_size : trip_count / num_threads;
	loop->chunk_size = ( loop->chunk_size ) ? loop->chunk_size : ( long ) 1;
	nb_chunks_per_thread = trip_count / ( loop->chunk_size * num_threads );
	/* Compute the number of extrat chunk nb_chunk_extra can't be greater than
	* nb_thread */
	const int nb_chunk_extra =
	    ( int ) ( ( trip_count / loop->chunk_size ) % num_threads );
	const int are_not_multiple = ( trip_count % loop->chunk_size ) ? 1 : 0;
	/* The first threads will have one more chunk (according to the previous
	* approximation) */
	nb_chunks_per_thread += ( rank < nb_chunk_extra ) ? ( long ) 1 : ( long ) 0;
	/* One thread may have one more additionnal smaller chunk to finish the
	* iteration domain */
	nb_chunks_per_thread +=
	    ( rank == nb_chunk_extra && are_not_multiple ) ? ( long ) 1 : ( long ) 0;
	return nb_chunks_per_thread;
}

/* ULL Chunk Manipulation */

/* Return the number of chunks that a static schedule will create for the thread 'rank' */
static inline unsigned long long __loop_get_static_nb_chunks_per_rank_ull( unsigned long long rank,
        unsigned long long num_threads,
        mpc_omp_loop_ull_iter_t *loop )
{
	unsigned long long nb_chunks_per_thread, chunk_size;
	const unsigned long long trip_count =
	    __loop_get_num_iters_ull( loop->lb, loop->b, loop->incr,
	            loop->up );
	chunk_size = ( loop->chunk_size ) ? loop->chunk_size : trip_count / num_threads;
	chunk_size = ( chunk_size ) ? chunk_size : ( unsigned long long ) 1;
	loop->chunk_size = chunk_size;
	/* Compute the number of chunks per thread (floor value) */
	nb_chunks_per_thread = trip_count / ( chunk_size * num_threads );
	/* Compute the number of extrat chunk nb_chunk_extra can't be greater than
	* nb_thread */
	const unsigned long long nb_chunk_extra = ( int ) ( ( trip_count / chunk_size ) % num_threads );
	const int are_not_multiple = ( trip_count % chunk_size ) ? 1 : 0;
	/* The first threads will have one more chunk (according to the previous
	* approximation) */
	nb_chunks_per_thread +=
	    ( rank < nb_chunk_extra ) ? ( unsigned long long ) 1 : ( unsigned long long ) 0;
	/* One thread may have one more additionnal smaller chunk to finish the
	* iteration domain */
	nb_chunks_per_thread += ( rank == nb_chunk_extra && are_not_multiple )
	                        ? ( unsigned long long ) 1
	                        : ( unsigned long long ) 0;
	return nb_chunks_per_thread;
}

/***************
 * LOOP STATIC *
 ***************/

/****
 *
 * CHUNK MANIPULATION
 *
 *
 *****/
/* Compute the chunk for a static schedule (without specific chunk size) */
static inline int __loop_static_schedule_get_single_chunk( long lb, long b, long incr,
        long *from, long *to )
{
	/*
	Original loop -> lb -> b step incr
	New loop -> *from -> *to step incr
	*/
	int trip_count, chunk_size;
	/* Grab info on the current thread */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	const int num_threads = t->info.num_threads;
	const int rank = t->rank;
	trip_count = ( b - lb ) / incr;

	if ( ( b - lb ) % incr != 0 )
	{
		trip_count++;
	}

	if ( trip_count <= t->rank )
	{
		mpc_common_nodebug( "____loop_static_schedule_get_single_chunk: "
		              "#%d (%d -> %d step %d) -> NO CHUNK",
		              rank, lb, b, incr );
		return 0;
	}

	chunk_size = trip_count / num_threads;

	if ( rank < ( trip_count % num_threads ) )
	{
		/* The first part is homogeneous with a chunk size a little bit larger */
		*from = lb + rank * ( chunk_size + 1 ) * incr;
		*to = lb + ( rank + 1 ) * ( chunk_size + 1 ) * incr;
	}
	else
	{
		/* The final part has a smaller chunk_size, therefore the computation is
		splitted in 2 parts */
		*from = lb + ( trip_count % num_threads ) * ( chunk_size + 1 ) * incr +
		        ( rank - ( trip_count % num_threads ) ) * chunk_size * incr;
		*to = lb + ( trip_count % num_threads ) * ( chunk_size + 1 ) * incr +
		      ( rank + 1 - ( trip_count % num_threads ) ) * chunk_size * incr;
	}

	assert( ( incr > 0 ) ? ( *to - incr <= b ) : ( *to - incr >= b ) );
	mpc_common_nodebug( "____loop_static_schedule_get_single_chunk: "
	              "#%d (%d -> %d step %d) -> (%d -> %d step %d)",
	              rank, lb, b, incr, *from, *to, incr );
	return 1;
}

void mpc_omp_static_schedule_get_single_chunk( long lb, long b, long incr,
        long *from, long *to )
{
  __loop_static_schedule_get_single_chunk(lb, b, incr, from, to);
}

/* Return the number of chunks that a static schedule would create */
static inline int __loop_static_schedule_get_nb_chunks( long lb, long b, long incr,
        long chunk_size )
{
	/* Original loop: lb -> b step incr */
	long trip_count;
	long nb_chunks_per_thread;
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	/* Retrieve the number of threads and the rank of the current thread */
	const int nb_threads = t->info.num_threads;
	const int rank = t->rank;
	/* Compute the trip count (total number of iterations of the original loop) */
	trip_count = ( b - lb ) / incr;

	if ( ( b - lb ) % incr != 0 )
	{
		trip_count++;
	}

	/* Compute the number of chunks per thread (floor value) */
	nb_chunks_per_thread = trip_count / ( chunk_size * nb_threads );

	/* The first threads will have one more chunk (according to the previous
	approximation) */
	if ( rank < ( trip_count / chunk_size ) % nb_threads )
	{
		nb_chunks_per_thread++;
	}

	/* One thread may have one more additionnal smaller chunk to finish the
	iteration domain */
	if ( rank == ( trip_count / chunk_size ) % nb_threads && trip_count % chunk_size != 0 )
	{
		nb_chunks_per_thread++;
	}

	mpc_common_nodebug( "____loop_static_schedule_get_nb_chunks[%d]: %ld -> [%ld] "
	              "(cs=%ld) final nb_chunks = %ld",
	              rank, lb, b, incr, chunk_size, nb_chunks_per_thread );
	return nb_chunks_per_thread;
}

/* Return the chunk #'chunk_num' assuming a static schedule with 'chunk_size'
 * as a chunk size */
void __loop_static_schedule_get_specific_chunk( long rank, long num_threads,
        mpc_omp_loop_long_iter_t *loop,
        long chunk_num, long *from,
        long *to )
{
	const long add = loop->chunk_size * loop->incr;
	const long decal = chunk_num * num_threads * add;
	*from = loop->lb + decal + add * rank;
	*to = *from + add;

	/* Do not compare if *to > b because *to can overflow if b close to LONG_MAX, we
	instead compare b - *from - *add to 0 */

	if ( ( loop->b - *from - add > 0 && loop->incr < 0 ) || ( loop->b - *from - add < 0 && loop->incr > 0 ) )
	{
		*to = loop->b;
	}

	mpc_common_nodebug( " %d %s: from: %ld  %ld - to: %ld %ld  - %ld - %ld",
	              rank, __func__, *from, loop->lb, loop->b, *to,
	              loop->incr, chunk_num );
}

/****
 *
 * LONG VERSION
 *
 *
 *****/

void _mpc_omp_static_loop_init( struct mpc_omp_thread_s *t, long lb, long b, long incr,
                                long chunk_size )
{
	mpc_omp_loop_gen_info_t *loop_infos;
	assert( t->instance != NULL );

	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_STATIC_LOOP;
	t->schedule_is_forced = 0;

	loop_infos = &( t->info.loop_infos );
	_mpc_omp_loop_gen_infos_init( loop_infos, lb, b, incr, chunk_size );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop,
                             ompt_scope_begin,
                             __loop_get_num_iters_gen( &( t->info.loop_infos )));
#endif /* OMPT_SUPPORT */

	/*  As the loop_next function consider a chunk as already been realised
	  we need to initialize to 0 minus 1 */
	t->static_current_chunk = -1;

	if ( !( t->info.loop_infos.ischunked ) )
	{
		t->static_nb_chunks = 1;
		return;
	}

	/* Compute the number of chunk for this thread */
	t->static_nb_chunks = __loop_get_static_nb_chunks_per_rank(
	                          t->rank, t->info.num_threads, &( loop_infos->loop.mpcomp_long ) );
	mpc_common_nodebug( "[%d] %s: Got %d chunk(s)", t->rank, __func__,
	              t->static_nb_chunks );
}

int mpc_omp_static_loop_begin( long lb, long b, long incr, long chunk_size,
                                long *from, long *to )
{
	mpc_omp_init();

	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();

	/* Initialization of loop internals */
	_mpc_omp_static_loop_init( t, lb, b, incr, chunk_size );

	return ( !from && !to ) ? -1 : mpc_omp_static_loop_next( from, to );
}

int mpc_omp_static_loop_next( long *from, long *to )
{
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	mpc_omp_loop_gen_info_t *loop_infos = &( t->info.loop_infos );
	const long rank = ( long ) t->rank;
	const long nb_threads = ( long ) t->info.num_threads;
	/* Next chunk */
	t->static_current_chunk++;
	mpc_common_nodebug( "[%d] %s: checking if current_chunk %d >= nb_chunks %d?",
	              t->rank, t->static_current_chunk, t->static_nb_chunks );

	/* Check if there is still a chunk to execute */
	if ( t->static_current_chunk >= t->static_nb_chunks )
	{
		return 0;
	}

	if ( !( loop_infos->ischunked ) )
	{
		mpc_omp_loop_long_iter_t *loop = &( loop_infos->loop.mpcomp_long );
		int ret = __loop_static_schedule_get_single_chunk( loop->lb, loop->b,
		                                                   loop->incr, from, to );

#if OMPT_SUPPORT
        if( ret ) {
            ompt_data_t ompt_from = { (uint64_t) *from };
            _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                         ompt_from );
        }
#endif /* OMPT_SUPPORT */

        return ret;
	}

	__loop_static_schedule_get_specific_chunk(
	    rank, nb_threads, &( loop_infos->loop.mpcomp_long ),
	    t->static_current_chunk, from, to );

	mpc_common_nodebug( "[%d] ___mpcomp_static_loop_next: got a chunk %d -> %d",
	              t->rank, *from, *to );
 
#if OMPT_SUPPORT
    ompt_data_t ompt_from = { (uint64_t) *from };
    _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                 ompt_from );
#endif /* OMPT_SUPPORT */

	return 1;
}

void mpc_omp_static_loop_end_nowait()
{
#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop, ompt_scope_end, 0 );
#endif /* OMPT_SUPPORT */
}

void mpc_omp_static_loop_end()
{
	mpc_omp_static_loop_end_nowait();
	mpc_omp_barrier(ompt_sync_region_barrier_implicit_workshare);
}

/****
 *
 * LONG ORDERED VERSION
 *
 *
 *****/
int mpc_omp_ordered_static_loop_begin( long lb, long b, long incr,
                                        long chunk_size, long *from, long *to )
{
	mpc_omp_init();
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	const int ret = mpc_omp_static_loop_begin( lb, b, incr, chunk_size, from, to );
	mpc_common_nodebug( "%d %s: lb : %ld b: %ld", t->rank, __func__, lb, b );

	if ( !from )
	{
		t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = lb;
		return -1;
	}

	t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
	return ret;
}

int mpc_omp_ordered_static_loop_next( long *from, long *to )
{
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	const int ret = mpc_omp_static_loop_next( from, to );
	t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
	return ret;
}

void mpc_omp_ordered_static_loop_end()
{
	mpc_omp_static_loop_end();
}

void mpc_omp_ordered_static_loop_end_nowait()
{
	mpc_omp_static_loop_end();
}

/* ULL version */

/* Return the chunk #'chunk_num' assuming a static schedule with 'chunk_size'
 * as a chunk size */

static inline void __loop_static_schedule_get_specific_chunk_ull(
    unsigned long long rank, unsigned long long num_threads,
    mpc_omp_loop_ull_iter_t *loop, unsigned long long chunk_num,
    unsigned long long *from, unsigned long long *to )
{
	const unsigned long long decal =
	    chunk_num * num_threads * loop->chunk_size * loop->incr;
	*from = loop->lb + decal + loop->chunk_size * loop->incr * rank;
	*to = *from + loop->chunk_size * loop->incr;
	*to = ( loop->up && *to > loop->b ) ? loop->b : *to;
	*to = ( !loop->up && *to < loop->b ) ? loop->b : *to;
}

int mpc_omp_static_loop_begin_ull( bool up, unsigned long long lb,
                                    unsigned long long b,
                                    unsigned long long incr,
                                    unsigned long long chunk_size,
                                    unsigned long long *from,
                                    unsigned long long *to )
{
	mpc_omp_loop_ull_iter_t *loop;

	mpc_omp_init();

	/* Grab info on the current thread */
	mpc_omp_thread_t *t = mpc_omp_get_thread_tls();

	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_STATIC_LOOP;
	t->schedule_is_forced = 0;

	/* Fill private info about the loop */
	t->info.loop_infos.type = MPC_OMP_LOOP_TYPE_ULL;
	loop = &( t->info.loop_infos.loop.mpcomp_ull );
	loop->up = up;
	loop->lb = lb;
	loop->b = b;
	loop->incr = incr;
	loop->chunk_size = chunk_size;
	t->static_nb_chunks = __loop_get_static_nb_chunks_per_rank_ull(
	                          t->rank, t->info.num_threads, loop );

	/* As the loop_next function consider a chunk as already been realised
	   we need to initialize to 0 minus 1 */
	t->static_current_chunk = -1;

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop,
                             ompt_scope_begin,
                             __loop_get_num_iters_gen( &( t->info.loop_infos )));
#endif /* OMPT_SUPPORT */

	if ( !from && !to )
	{
		return -1;
	}

	return mpc_omp_static_loop_next_ull( from, to );
}

int mpc_omp_static_loop_next_ull( unsigned long long *from,
                                   unsigned long long *to )
{
	/* Grab info on the current thread */
	mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
	/* Retrieve the number of threads and the rank of this thread */
	const unsigned long long num_threads = t->info.num_threads;
	const unsigned long long rank = t->rank;
	/* Next chunk */
	t->static_current_chunk++;

	/* Check if there is still a chunk to execute */
	if ( t->static_current_chunk >= t->static_nb_chunks )
	{
		return 0;
	}

	__loop_static_schedule_get_specific_chunk_ull(
	    rank, num_threads, &( t->info.loop_infos.loop.mpcomp_ull ),
	    t->static_current_chunk, from, to );

#if OMPT_SUPPORT
    ompt_data_t ompt_from = { (uint64_t) *from };
    _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                 ompt_from );
#endif /* OMPT_SUPPORT */

	return 1;
}

/****
 *
 * ULL ORDERED VERSION
 *
 *
 *****/
int mpc_omp_ordered_static_loop_begin_ull( bool up, unsigned long long lb,
        unsigned long long b,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *from,
        unsigned long long *to )
{
	/* Grab info on the current thread */
	mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
	const int ret =
	    mpc_omp_static_loop_begin_ull( up, lb, b, incr, chunk_size, from, to );

	if ( !from )
	{
		return -1;
	}

	t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
	return ret;
}

int mpc_omp_ordered_static_loop_next_ull( unsigned long long *from,
        unsigned long long *to )
{
	/* Grab info on the current thread */
	mpc_omp_thread_t *t = mpc_omp_get_thread_tls();
	const int ret = mpc_omp_static_loop_next_ull( from, to );
	t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
	return ret;
}

/***************
 * LOOP GUIDED *
 ***************/

/****
 *
 * LONG VERSION
 *
 *
 *****/

int mpc_omp_guided_loop_begin( long lb, long b, long incr, long chunk_size,
                                long *from, long *to )
{
    mpc_omp_init();

	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t );

	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_GUIDED_LOOP;
	t->schedule_is_forced = 1;

	const int index = __loop_dyn_get_for_dyn_index( t );

	/* if current thread is too much ahead */
	while ( OPA_load_int( &( t->instance->team->for_dyn_nb_threads_exited[index].i ) ) ==
	        MPC_OMP_NOWAIT_STOP_SYMBOL )
	{
		mpc_thread_yield();
	}

	_mpc_omp_loop_gen_infos_init( &( t->info.loop_infos ), lb, b, incr, chunk_size );
	mpc_common_spinlock_lock( &( t->instance->team->lock ) );

	/* First thread store shared first iteration value */
	if ( t->instance->team->is_first[index] == 1 )
	{
		t->instance->team->is_first[index] = 0;
		OPA_store_ptr( &( t->instance->team->guided_from[index] ), ( void * ) lb );
	}

	mpc_common_spinlock_unlock( &( t->instance->team->lock ) );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop,
                             ompt_scope_begin,
                             __loop_get_num_iters_gen( &( t->info.loop_infos )));
#endif /* OMPT_SUPPORT */

	return ( !from && !to ) ? -1 : mpc_omp_guided_loop_next( from, to );
}

int mpc_omp_guided_loop_next( long *from, long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t );
	mpc_omp_loop_long_iter_t *loop = &( t->info.loop_infos.loop.mpcomp_long );
	const long num_threads = ( long ) t->info.num_threads;
	long long int ret = 0, anc_from, chunk_size, new_from;
	const int index = __loop_dyn_get_for_dyn_index( t );

	while ( ret == 0 ) /* will loop again if another thread changed from value at the same time */
	{
		anc_from = ( long ) OPA_load_ptr( &( t->instance->team->guided_from[index] ) );
		chunk_size = ( loop->b - anc_from ) / ( 2 * loop->incr * num_threads );

		if ( chunk_size < 0 )
		{
			chunk_size = -chunk_size;
		}

		if ( loop->chunk_size > chunk_size )
		{
			chunk_size = loop->chunk_size;
		}

		if ( ( loop->b - anc_from - chunk_size * loop->incr < 0 && loop->incr > 0 ) || ( loop->b - anc_from - chunk_size * loop->incr > 0 && loop->incr < 0 ) ) // Last iteration
		{
			new_from = loop->b;
		}
		else
		{
			new_from = anc_from + chunk_size * loop->incr;
		}

		ret = ( long ) OPA_cas_ptr( &( t->instance->team->guided_from[index] ), ( void * ) anc_from, ( void * ) new_from );
		ret = ( ret == anc_from ) ? 1 : 0;
	}

	*from = anc_from;
	*to = new_from;

	if (( *from < *to && loop->incr > 0 ) || ( *from > *to && loop->incr < 0 ) )
	{
#if OMPT_SUPPORT
        ompt_data_t ompt_from = { (uint64_t) *from };
        _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                     ompt_from );
#endif /* OMPT_SUPPORT */

		return 1;
	}
	else
	{
		return 0;
	}
}

void mpc_omp_guided_loop_end_nowait()
{
	int nb_threads_exited;
	struct mpc_omp_thread_s *t; /* Info on the current thread */
	mpc_omp_team_t *team_info;  /* Info on the team */
	/* Grab the thread info */
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	/* Number of threads in the current team */
	const int num_threads = t->info.num_threads;
	/* Compute the index of the dynamic for construct */
	const int index = __loop_dyn_get_for_dyn_index( t );
	assert( index >= 0 && index < MPC_OMP_MAX_ALIVE_FOR_DYN + 1 );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop, ompt_scope_end, 0 );
#endif /* OMPT_SUPPORT */

	/* Get the team info */
	assert( t->instance != NULL );
	team_info = t->instance->team;
	assert( team_info != NULL );
	/* WARNING: the following order is important */
	mpc_common_spinlock_lock( &( t->info.update_lock ) );
	OPA_incr_int( &( t->for_dyn_ull_current ) );
	mpc_common_spinlock_unlock( &( t->info.update_lock ) );
	/* Update the number of threads which ended this loop */
	nb_threads_exited = OPA_fetch_and_incr_int(
	                        &( team_info->for_dyn_nb_threads_exited[index].i ) ) +
	                    1;
	assert( nb_threads_exited > 0 && nb_threads_exited <= num_threads );

	if ( nb_threads_exited == num_threads )
	{
		t->instance->team->is_first[index] = 1;
		const int previous_index = __loop_dyn_get_for_dyn_prev_index( t );
		assert( previous_index >= 0 &&
		             previous_index < MPC_OMP_MAX_ALIVE_FOR_DYN + 1 );
		OPA_store_int( &( team_info->for_dyn_nb_threads_exited[index].i ),
		               MPC_OMP_NOWAIT_STOP_SYMBOL );
		OPA_swap_int(
		    &( team_info->for_dyn_nb_threads_exited[previous_index].i ), 0 );
	}
}

void mpc_omp_guided_loop_end()
{
	mpc_omp_guided_loop_end_nowait();
	mpc_omp_barrier(ompt_sync_region_barrier_implicit_workshare);
}



/****
 *
 * ORDERED LONG VERSION
 *
 *
 *****/

int mpc_omp_ordered_guided_loop_begin( long lb, long b, long incr,
                                        long chunk_size, long *from, long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	const int ret = mpc_omp_guided_loop_begin( lb, b, incr, chunk_size, from, to );

	if ( from )
	{
		t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
	}

	return ret;
}

int mpc_omp_ordered_guided_loop_next( long *from, long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	const int ret = mpc_omp_guided_loop_next( from, to );
	t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
	return ret;
}

void mpc_omp_ordered_guided_loop_end()
{
	mpc_omp_guided_loop_end();
}

void mpc_omp_ordered_guided_loop_end_nowait()
{
	mpc_omp_guided_loop_end();
}

/****
 *
 * ULL VERSION
 *
 *
 *****/
int mpc_omp_loop_ull_guided_begin( bool up, unsigned long long lb,
                                    unsigned long long b,
                                    unsigned long long incr,
                                    unsigned long long chunk_size,
                                    unsigned long long *from,
                                    unsigned long long *to )
{
    mpc_omp_init();

	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t );

	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_GUIDED_LOOP;
	t->schedule_is_forced = 1;

	const int index = __loop_dyn_get_for_dyn_index( t );

	while ( OPA_load_int( &( t->instance->team->for_dyn_nb_threads_exited[index].i ) ) ==
	        MPC_OMP_NOWAIT_STOP_SYMBOL )
	{
		mpc_thread_yield();
	}

	_mpc_omp_loop_gen_infos_init_ull( &( t->info.loop_infos ), lb, b, incr, chunk_size );
	mpc_omp_loop_long_iter_t *loop = ( mpc_omp_loop_long_iter_t * ) & ( t->info.loop_infos.loop.mpcomp_ull );
	loop->up = up;

	mpc_common_spinlock_lock( &( t->instance->team->lock ) );

	if ( t->instance->team->is_first[index] == 1 )
	{
		t->instance->team->is_first[index] = 0;
		OPA_store_ptr( &( t->instance->team->guided_from[index] ), ( void * ) lb );
	}

	mpc_common_spinlock_unlock( &( t->instance->team->lock ) );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop,
                             ompt_scope_begin,
                             __loop_get_num_iters_gen( &( t->info.loop_infos )));
#endif /* OMPT_SUPPORT */

	return ( !from && !to ) ? -1 : mpc_omp_loop_ull_guided_next( from, to );
}

int mpc_omp_loop_ull_guided_next( unsigned long long *from,
                                   unsigned long long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t );
	mpc_omp_loop_ull_iter_t *loop = ( mpc_omp_loop_ull_iter_t * ) & ( t->info.loop_infos.loop.mpcomp_ull );
	const unsigned long long num_threads = ( unsigned long long ) t->info.num_threads;
	unsigned long long ret = 0, anc_from, chunk_size, new_from;
	const int index = __loop_dyn_get_for_dyn_index( t );

	while ( ret == 0 ) /* will loop again if another thread changed from value at the same time */
	{
		anc_from = ( unsigned long long ) OPA_load_ptr( &( t->instance->team->guided_from[index] ) );

		if ( loop->up )
		{
			chunk_size = ( ( loop->b - anc_from ) / ( loop->incr ) ) / ( 2 * num_threads );
		}
		else
		{
			chunk_size = ( ( anc_from - loop->b ) / ( -loop->incr ) ) / ( 2 * num_threads );
		}

		if ( loop->chunk_size > chunk_size )
		{
			chunk_size = loop->chunk_size;
		}

		if ( ( ( loop->b - anc_from ) / chunk_size < loop->incr && loop->up ) || ( ( anc_from - loop->b ) / chunk_size < ( -loop->incr ) && !loop->up ) ) // Last iteration
		{
			new_from = loop->b;
		}
		else
		{
			new_from = anc_from + chunk_size * loop->incr;
		}

		ret = ( unsigned long long ) OPA_cas_ptr( &( t->instance->team->guided_from[index] ), ( void * ) anc_from, ( void * ) new_from );
		ret = ( ret == anc_from ) ? 1 : 0;
	}

	*from = anc_from;
	*to = new_from;

	if ( *from != *to )
	{
#if OMPT_SUPPORT
        ompt_data_t ompt_from = { (uint64_t) *from };
        _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                     ompt_from );
#endif /* OMPT_SUPPORT */

		return 1;
	}
	else
	{
		return 0;
	}
}

void _mpc_omp_guided_loop_ull_end()
{
	mpc_omp_guided_loop_end();
}

void _mpc_omp_guided_loop_ull_end_nowait()
{
	mpc_omp_guided_loop_end_nowait();
}

/****
 *
 * ULL ORDERED VERSION
 *
 *
 *****/
int mpc_omp_loop_ull_ordered_guided_begin( bool up, unsigned long long lb,
        unsigned long long b,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *from,
        unsigned long long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	const int ret =
	    mpc_omp_loop_ull_guided_begin( up, lb, b, incr, chunk_size, from, to );

	if ( from )
	{
		t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
	}

	return ret;
}

int mpc_omp_loop_ull_ordered_guided_next( unsigned long long *from,
        unsigned long long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	const int ret = mpc_omp_loop_ull_guided_next( from, to );
	t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
	return ret;
}

/****************
 * LOOP DYNAMIC *
 ****************/

/* Utility Functions */

/* dest = src1+src2 in base 'base' of size 'depth' up to dimension 'top_level'
 */
static inline int __loop_dyn_add( int *dest, int *src1, int *src2,
        int *base, int depth,
        int top_level,
        int include_carry_over )
{
	int i, ret, carry_over;
	ret = 1;

	/* Step to the next target */
	for ( i = depth - 1; i >= top_level; i-- )
	{
		const int value = src1[i] + src2[i];
		dest[i] = value;
		carry_over = 0;

		if ( value >= base[i] )
		{
			ret = ( i == top_level ) ? 0 : ret;
			carry_over = ( include_carry_over ) ? value / base[i] : carry_over;
			dest[i] = ( value % base[i] );
		}
	}

	return ret;
}

/* Return 1 if overflow, otherwise 0 */
static inline int __loop_dyn_increase( int *target, int *base,
        int depth, int top_level,
        int include_carry_over )
{
	int i, carry_over, ret;
	ret = 0;
	carry_over = 1;

	/* Step to the next target */
	for ( i = depth - 1; i >= top_level; i-- )
	{
		const int value = target[i] + carry_over;
		carry_over = 0;
		target[i] = value;

		if ( value >= base[i] )
		{
			ret = ( i == top_level ) ? 1 : ret;
			carry_over = ( include_carry_over ) ? value / base[i] : carry_over;
			target[i] = value % base[i];
		}
	}

	return ret;
}

static inline void __loop_dyn_init_target_chunk_ull( struct mpc_omp_thread_s *thread,
        struct mpc_omp_thread_s *target,
        unsigned int num_threads )
{
	/* Compute the index of the dynamic for construct */
	const int index = __loop_dyn_get_for_dyn_index( thread );
	int cur = OPA_load_int( &( target->for_dyn_remain[index].i ) );

	if ( cur < 0 )
	{
		if ( !mpc_common_spinlock_trylock( &( target->info.update_lock ) ) )
		{
			/* Get the current id of remaining chunk for the target */
			cur = OPA_load_int( &( target->for_dyn_remain[index].i ) );

			if ( cur < 0 && ( __loop_dyn_get_for_dyn_current( thread ) >
			                  __loop_dyn_get_for_dyn_current( target ) ) )
			{
				target->for_dyn_total[index] = __loop_get_static_nb_chunks_per_rank_ull(
				                                   ( unsigned long long ) target->rank, ( unsigned long long ) num_threads, &( thread->info.loop_infos.loop.mpcomp_ull ) );
				OPA_cas_int( &( target->for_dyn_remain[index].i ), -1,
				             target->for_dyn_total[index] );
			}

			mpc_common_spinlock_unlock( &( target->info.update_lock ) );
		}
	}
}

static inline void __loop_dyn_init_target_chunk( struct mpc_omp_thread_s *thread,
                                     struct mpc_omp_thread_s *target,
                                     unsigned int num_threads )
{
	/* Compute the index of the dynamic for construct */
	const int index = __loop_dyn_get_for_dyn_index( thread );
	int cur = OPA_load_int( &( target->for_dyn_remain[index].i ) );

	if ( cur < 0 )
	{
		if ( !mpc_common_spinlock_trylock( &( target->info.update_lock ) ) )
		{
			/* Get the current id of remaining chunk for the target */
			cur = OPA_load_int( &( target->for_dyn_remain[index].i ) );

			if ( cur < 0 && ( __loop_dyn_get_for_dyn_current( thread ) >
			                  __loop_dyn_get_for_dyn_current( target ) ) )
			{
				target->for_dyn_total[index] = __loop_get_static_nb_chunks_per_rank(
				                                   target->rank, num_threads, &( thread->info.loop_infos.loop.mpcomp_long ) );
				OPA_cas_int( &( target->for_dyn_remain[index].i ), -1,
				             target->for_dyn_total[index] );
			}

			mpc_common_spinlock_unlock( &( target->info.update_lock ) );
		}
	}
}

static inline int __loop_dyn_get_chunk_from_target( struct mpc_omp_thread_s *thread,
        struct mpc_omp_thread_s *target )
{
	int prev, cur;
	/* Compute the index of the dynamic for construct */
	const int index = __loop_dyn_get_for_dyn_index( thread );
	cur = OPA_load_int( &( target->for_dyn_remain[index].i ) );

	/* Init target chunk if it is not */
	if ( cur < 0 )
	{
		if ( thread->info.loop_infos.type == MPC_OMP_LOOP_TYPE_LONG )
		{
			__loop_dyn_init_target_chunk( thread, target, thread->info.num_threads );
		}
		else
		{
			__loop_dyn_init_target_chunk_ull( thread, target, thread->info.num_threads );
		}

		cur = OPA_load_int( &( target->for_dyn_remain[index].i ) );
	}

	if ( cur <= 0 )
	{
		return -1;
	}

	int success = 0;
	unsigned long long for_dyn_total;

	do
	{
		if ( thread == target )
		{
			prev = cur;
			cur = OPA_cas_int( &( target->for_dyn_remain[index].i ), prev,
			                   prev - 1 );
			for_dyn_total = target->for_dyn_total[index];
			success = ( cur == prev );
		}
		else
		{
			if ( __loop_dyn_get_for_dyn_current( thread ) >=
			     __loop_dyn_get_for_dyn_current( target ) )
			{
				if ( !mpc_common_spinlock_trylock( &( target->info.update_lock ) ) )
				{
					cur = OPA_load_int( &( target->for_dyn_remain[index].i ) );

					if ( ( __loop_dyn_get_for_dyn_current( thread ) >=
					       __loop_dyn_get_for_dyn_current( target ) ) &&
					     cur > 0 )
					{
						prev = cur;
						cur = OPA_cas_int( &( target->for_dyn_remain[index].i ), prev,
						                   prev - 1 );
						success = ( cur == prev );
					}
					else
					{
						/* NO MORE STEAL */
						cur = 0;
					}

					for_dyn_total = target->for_dyn_total[index];
					mpc_common_spinlock_unlock( &( target->info.update_lock ) );
				}
				else
				{
					success = 0;
				}
			}
			else
			{
				/* NO MORE STEAL */
				cur = 0;
			}
		}
	}
	while ( cur > 0 && !success );

	return ( cur > 0 && success ) ? ( int ) ( for_dyn_total - cur ) : -1;
}

static inline int __loop_dyn_get_victim_rank( struct mpc_omp_thread_s *thread )
{
	int i;
	unsigned int target;
	int *tree_cumulative;
	assert( thread );
	assert( thread->instance );
	tree_cumulative = thread->instance->tree_cumulative + 1;
	const int tree_depth = thread->instance->tree_depth - 1;
	assert( thread->for_dyn_target );
	assert( thread->instance->tree_cumulative );

	for ( i = 0, target = 0; i < tree_depth; i++ )
	{
		target += thread->for_dyn_target[i] * tree_cumulative[i];
	}

	target = ( thread->rank + target ) % thread->instance->nb_mvps;
	assert( target < thread->instance->nb_mvps );
	//fprintf(stderr, "[%d] ::: %s ::: Get Victim  %d\n", thread->rank, __func__, target );
	return target;
}

static inline void __loop_dyn_target_reset( struct mpc_omp_thread_s *thread )
{
	int i;
	assert( thread );
	assert( thread->instance );
	const int tree_depth = thread->instance->tree_depth - 1;
	assert( thread->mvp );
	assert( thread->mvp->tree_rank );
	assert( thread->for_dyn_target );
	assert( thread->for_dyn_shift );

	/* Re-initialize the target to steal */
	for ( i = 0; i < tree_depth; i++ )
	{
		thread->for_dyn_shift[i] = 0;
		thread->for_dyn_target[i] = thread->mvp->tree_rank[i];
	}
}

static inline void __loop_dyn_target_init( struct mpc_omp_thread_s *thread )
{
	assert( thread );

	if ( thread->for_dyn_target )
	{
		return;
	}

	assert( thread->instance );
	const int tree_depth = thread->instance->tree_depth - 1;
	thread->for_dyn_target = ( int * ) malloc( tree_depth * sizeof( int ) );
	assert( thread->for_dyn_target );
	thread->for_dyn_shift = ( int * ) malloc( tree_depth * sizeof( int ) );
	assert( thread->for_dyn_shift );
	assert( thread->mvp );
	assert( thread->mvp->tree_rank );
	__loop_dyn_target_reset( thread );
	mpc_common_nodebug( "[%d] %s: initialization of target and shift", thread->rank,
	              __func__ );
}

static int __loop_dyn_get_chunk_from_rank( struct mpc_omp_thread_s *t,
        struct mpc_omp_thread_s *target,
        long *from, long *to )
{
	int cur;

	if ( !target )
	{
		return 0;
	}

	const long rank = ( long ) target->rank;
	const long num_threads = ( long ) t->info.num_threads;
	assert( t->info.loop_infos.type == MPC_OMP_LOOP_TYPE_LONG );
	mpc_omp_loop_long_iter_t *loop = &( t->info.loop_infos.loop.mpcomp_long );
	cur = __loop_dyn_get_chunk_from_target( t, target );

	if ( cur < 0 )
	{
		return 0;
	}

	__loop_static_schedule_get_specific_chunk( rank, num_threads, loop,
	        cur, from, to );
	return 1;
}

void _mpc_omp_dynamic_loop_init( struct mpc_omp_thread_s *t, long lb, long b, long incr,
                                 long chunk_size )
{
	/* Get the team info */
	mpc_omp_team_t *team_info;
	assert( t->instance != NULL );
	team_info = t->instance->team;
	assert( team_info != NULL );

	/* WORKAROUND (pr35196.c)
	* Reinitialize the flag for the last iteration of the loop */
	t->for_dyn_last_loop_iteration = 0;

	const long num_threads = ( long ) t->info.num_threads;
	const int index = __loop_dyn_get_for_dyn_index( t );

	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_DYN_LOOP;
	t->schedule_is_forced = 0;

	/* Stop if the maximum number of alive loops is reached */
	while (
	    OPA_load_int( &( team_info->for_dyn_nb_threads_exited[index].i ) ) ==
	    MPC_OMP_NOWAIT_STOP_SYMBOL )
	{
		mpc_thread_yield();
	}

	/* Fill private info about the loop */
	_mpc_omp_loop_gen_infos_init( &( t->info.loop_infos ), lb, b, incr, chunk_size );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop,
                             ompt_scope_begin,
                             __loop_get_num_iters_gen( &( t->info.loop_infos )));
#endif /* OMPT_SUPPORT */

	/* Try to change the number of remaining chunks */
	t->dyn_last_target = t;
	t->for_dyn_total[index] = __loop_get_static_nb_chunks_per_rank(
	                              t->rank, num_threads, &( t->info.loop_infos.loop.mpcomp_long ) );
	( void ) OPA_cas_int( &( t->for_dyn_remain[index].i ), -1, t->for_dyn_total[index] );
}

int mpc_omp_dynamic_loop_begin( long lb, long b, long incr, long chunk_size,
                                 long *from, long *to )
{
    /* Info on the current thread */
	struct mpc_omp_thread_s *t;

	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();

	/* Grab the thread info */
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );

	/* Initialization of loop internals */
	t->for_dyn_last_loop_iteration = 0;
	_mpc_omp_dynamic_loop_init( t, lb, b, incr, chunk_size );

	if ( !( t->instance->root ) )
	{
		if( from ) *from = lb;
		if( to ) *to = b;
		t->schedule_type =
		    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_DYN_LOOP;
		t->schedule_is_forced = 0;
		return 1;
	}

	return ( !from && !to ) ? -1 : mpc_omp_dynamic_loop_next( from, to );
}

int mpc_omp_dynamic_loop_next( long *from, long *to )
{
	mpc_omp_mvp_t *target_mvp;
	struct mpc_omp_thread_s *t, *target;
	int found, target_idx, barrier_nthreads, ret;
	/* Grab the thread info */
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	/* Stop the stealing at the following depth.
	 * Nodes above this depth will not be traversed */
	const int num_threads = t->info.num_threads;

	/* 1 thread => no stealing, directly get a chunk */
	if ( num_threads == 1 )
	{
        int ret = __loop_dyn_get_chunk_from_rank( t, t, from, to );

#if OMPT_SUPPORT
        if( ret ) {
            ompt_data_t ompt_from = { (uint64_t) *from };
            _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                         ompt_from );
        }
#endif /* OMPT_SUPPORT */

		return ret;
	}

	/* Check that the target is allocated */
	__loop_dyn_target_init( t );

	/*
	 * WORKAROUND (pr35196.c):
	 * Stop if the current thread already executed
	 * the last iteration of the current loop
	 */
	if ( t->for_dyn_last_loop_iteration == 1 )
	{
		__loop_dyn_target_reset( t );
		return 0;
	}

	int *tree_base = t->instance->tree_base + 1;
	const int tree_depth = t->instance->tree_depth - 1;
	const int top_level = t->instance->root->depth;
	/* Compute the index of the target */
	target = t->dyn_last_target;
	found = 1;
	ret = 0;

	/* While it is not possible to get a chunk */
	while ( !__loop_dyn_get_chunk_from_rank( t, target, from, to ) )
	{
do_increase:

		if ( ret )
		{
			found = 0;
			break;
		}

		ret = __loop_dyn_increase( t->for_dyn_shift, tree_base, tree_depth, top_level, 1 );
		/* Add t->for_dyn_shift and t->mvp->tree_rank[i] w/out carry over and store it into t->for_dyn_target */
		__loop_dyn_add( t->for_dyn_target, t->for_dyn_shift, t->mvp->tree_rank, tree_base, tree_depth, top_level, 0 );
		/* Compute the index of the target */
		target_idx = __loop_dyn_get_victim_rank( t );
		target_mvp = t->instance->mvps[target_idx].ptr.mvp;
		target = ( target_mvp ) ? target_mvp->threads : NULL;

		if ( !( target ) )
		{
			goto do_increase;
		}

		barrier_nthreads = target_mvp->father->barrier_num_threads;

		/* Stop if the target is not a launch thread */
		if ( t->for_dyn_target[tree_depth - 1] >= barrier_nthreads )
		{
			goto do_increase;
		}
	}

	/* Did we exit w/ or w/out a chunk? */
	if ( found )
	{
		/* WORKAROUND (pr35196.c):
		* to avoid issue w/ lastprivate and GCC code generation,
		* check if this is the last chunk to avoid further execution
		* for this thread.
		*/
		if ( *to == t->info.loop_b )
		{
			t->for_dyn_last_loop_iteration = 1;
		}

#if OMPT_SUPPORT
        ompt_data_t ompt_from = { (uint64_t) *from };
        _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                     ompt_from );
#endif /* OMPT_SUPPORT */

		t->dyn_last_target = target;
		return 1;
	}

	return 0;
}

void mpc_omp_dynamic_loop_end_nowait( void )
{
	int nb_threads_exited;
	struct mpc_omp_thread_s *t; /* Info on the current thread */
	mpc_omp_team_t *team_info;  /* Info on the team */
	/* Grab the thread info */
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	/* Number of threads in the current team */
	const int num_threads = t->info.num_threads;
	/* Compute the index of the dynamic for construct */
	const int index = __loop_dyn_get_for_dyn_index( t );
	assert( index >= 0 && index < MPC_OMP_MAX_ALIVE_FOR_DYN + 1 );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop, ompt_scope_end, 0 );
#endif /* OMPT_SUPPORT */

	/* In case of 1 thread, re-initialize the number of remaining chunks
	   * but do not increase the current index */
	if ( num_threads == 1 )
	{
		OPA_store_int( &( t->for_dyn_remain[index].i ), -1 );
		return;
	}

	/* Get the team info */
	assert( t->instance != NULL );
	team_info = t->instance->team;
	assert( team_info != NULL );
	/* WARNING: the following order is important */
	mpc_common_spinlock_lock( &( t->info.update_lock ) );
	OPA_incr_int( &( t->for_dyn_ull_current ) );
	OPA_store_int( &( t->for_dyn_remain[index].i ), -1 );
	mpc_common_spinlock_unlock( &( t->info.update_lock ) );
	/* Update the number of threads which ended this loop */
	nb_threads_exited = OPA_fetch_and_incr_int(
	                        &( team_info->for_dyn_nb_threads_exited[index].i ) ) +
	                    1;
	assert( nb_threads_exited > 0 && nb_threads_exited <= num_threads );

	if ( nb_threads_exited == num_threads )
	{
		const int previous_index = __loop_dyn_get_for_dyn_prev_index( t );
		assert( previous_index >= 0 &&
		             previous_index < MPC_OMP_MAX_ALIVE_FOR_DYN + 1 );
		OPA_store_int( &( team_info->for_dyn_nb_threads_exited[index].i ),
		               MPC_OMP_NOWAIT_STOP_SYMBOL );
		OPA_swap_int(
		    &( team_info->for_dyn_nb_threads_exited[previous_index].i ), 0 );
	}
}

void mpc_omp_dynamic_loop_end( void )
{
	mpc_omp_dynamic_loop_end_nowait();
	mpc_omp_barrier(ompt_sync_region_barrier_implicit_workshare);
}

int mpc_omp_dynamic_loop_next_ignore_nowait( __UNUSED__ long *from, __UNUSED__ long *to )
{
	not_implemented();
	return 0;
}

/****
 *
 * ORDERED LONG VERSION
 *
 *
 *****/

int mpc_omp_ordered_dynamic_loop_begin( long lb, long b, long incr,
        long chunk_size, long *from, long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	_mpc_omp_loop_gen_infos_init( &( t->info.loop_infos ), lb, b, incr, chunk_size );
	const int ret =
	    mpc_omp_dynamic_loop_begin( lb, b, incr, chunk_size, from, to );

	if ( from )
	{
		t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
	}

	return ret;
}

int mpc_omp_ordered_dynamic_loop_next( long *from, long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	const int ret = mpc_omp_dynamic_loop_next( from, to );
	t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
	return ret;
}

void mpc_omp_ordered_dynamic_loop_end()
{
	mpc_omp_dynamic_loop_end();
}

void mpc_omp_ordered_dynamic_loop_end_nowait()
{
	mpc_omp_dynamic_loop_end();
}

void _mpc_omp_for_dyn_coherency_end_parallel_region(
    mpc_omp_instance_t *instance )
{
	struct mpc_omp_thread_s *t_first;
	mpc_omp_team_t *team;
	int i;
	int n;
	team = instance->team;
	assert( team != NULL );
	const int tree_depth = instance->tree_depth - 1;
	mpc_common_nodebug( "[X] _mpcomp_for_dyn_coherency_checking_end_barrier: "
	              "Checking for %d thread(s)...",
	              team->info.num_threads );
	/* Check team coherency */
	n = 0;

	for ( i = 0; i < MPC_OMP_MAX_ALIVE_FOR_DYN + 1; i++ )
	{
		switch ( OPA_load_int( &team->for_dyn_nb_threads_exited[i].i ) )
		{
			case 0:
				break;

			case MPC_OMP_NOWAIT_STOP_SYMBOL:
				n++;
				break;

			default:
				/* This array should contain only '0' and
				* one MPC_OMP_MAX_ALIVE_FOR_DYN */
				not_reachable();
				break;
		}

		mpc_common_nodebug( "[X] _mpcomp_for_dyn_coherency_checking_end_barrier: "
		              "TEAM - FOR_DYN - nb_threads_exited[%d] = %d",
		              i, team->for_dyn_nb_threads_exited[i].i );
	}

	if ( n != 1 )
	{
		/* This array should contain exactly 1 MPC_OMP_MAX_ALIVE_FOR_DYN */
		not_reachable();
	}

	/* Check tree coherency */
	/* Check thread coherency */
	t_first = &( instance->mvps[0].ptr.mvp->threads[0] );

	for ( i = 0; i < team->info.num_threads; i++ )
	{
		struct mpc_omp_thread_s *target_t;
		int j;
		target_t = &( instance->mvps[i].ptr.mvp->threads[0] );
		mpc_common_nodebug( "[X] _mpcomp_for_dyn_coherency_checking_end_barrier: "
		              "Checking thread %d...",
		              target_t->rank );

		/* Checking for_dyn_current */
		if ( t_first->for_dyn_current != target_t->for_dyn_current )
		{
			not_reachable();
		}

		/* Checking for_dyn_remain */
		for ( j = 0; j < MPC_OMP_MAX_ALIVE_FOR_DYN + 1; j++ )
		{
			if ( OPA_load_int( &( target_t->for_dyn_remain[j].i ) ) != -1 )
			{
				not_reachable();
			}
		}

		/* Checking for_dyn_target */
		/* for_dyn_tarfer can be NULL if there were no dynamic loop
		   * since the beginning of the program
		   */
		if ( target_t->for_dyn_target != NULL )
		{
			for ( j = 0; j < tree_depth; j++ )
			{
				if ( target_t->for_dyn_target[j] != target_t->mvp->tree_rank[j] )
				{
					mpc_common_nodebug( "[X] _mpcomp_for_dyn_coherency_checking_end_barrier: "
					              "error rank %d has target[%d] = %d (depth: %d, "
					              "top_level: %d, for_dyn_current=%d)",
					              target_t->rank, j, target_t->for_dyn_target[j],
					              tree_depth, team->info.new_root->depth,
					              target_t->for_dyn_current );
					not_reachable();
				}
			}
		}

		/* Checking for_dyn_shift */
		/* for_dyn_shift can be NULL if there were no dynamic loop
		   * since the beginning of the program
		   */
		if ( target_t->for_dyn_shift != NULL )
		{
			// for ( j = t->info.new_root->depth ; j < instance->tree_depth ; j++ )
			for ( j = 0; j < tree_depth; j++ )
			{
				if ( target_t->for_dyn_shift[j] != 0 )
				{
					mpc_common_nodebug( "[X] _mpcomp_for_dyn_coherency_checking_end_barrier: "
					              "error rank %d has shift[%d] = %d (depth: %d, "
					              "top_level: %d, for_dyn_current=%d)",
					              target_t->rank, j, target_t->for_dyn_shift[j],
					              tree_depth, team->info.new_root->depth,
					              target_t->for_dyn_current );
					not_reachable();
				}
			}
		}
	}
}

/****
 *
 * ULL CHUNK MANIPULATION
 *
 *
 *****/

/* From thread t, try to steal a chunk from thread target
 * Returns 1 on success, 0 otherwise */
static int __loop_dyn_get_chunk_from_rank_ull(
    struct mpc_omp_thread_s *t, struct mpc_omp_thread_s *target, unsigned long long *from,
    unsigned long long *to )
{
	int cur;

	if ( !target )
	{
		return 0;
	}

	/* Number of threads in the current team */
	const unsigned long long rank = ( unsigned long long ) target->rank;
	const unsigned long long num_threads =
	    ( unsigned long long ) t->info.num_threads;
	mpc_omp_loop_ull_iter_t *loop = &( t->info.loop_infos.loop.mpcomp_ull );
	cur = __loop_dyn_get_chunk_from_target( t, target );

	if ( cur < 0 )
	{
		return 0;
	}

	__loop_static_schedule_get_specific_chunk_ull( rank, num_threads, loop,
	        cur, from, to );

#if OMPT_SUPPORT
    ompt_data_t ompt_from = { (uint64_t) *from };
    _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                 ompt_from );
#endif /* OMPT_SUPPORT */

	return 1;
}

/****
 *
 * ULL VERSION
 *
 *
 *****/
static inline void __loop_dyn_init_ull( struct mpc_omp_thread_s *t, bool up,
                                     unsigned long long lb, unsigned long long b,
                                     unsigned long long incr,
                                     unsigned long long chunk_size )
{
	mpc_omp_loop_ull_iter_t *loop;

	/* Get the team info */
	assert( t->instance != NULL );
	assert( t->instance->team != NULL );

	/* WORKAROUND (pr35196.c)
	* Reinitialize the flag for the last iteration of the loop */
	t->for_dyn_last_loop_iteration = 0;

	const int num_threads = t->info.num_threads;
	const int index = __loop_dyn_get_for_dyn_index( t );

	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_DYN_LOOP;
	t->schedule_is_forced = 0;

	/* Stop if the maximum number of alive loops is reached */
	while ( OPA_load_int(
	            &( t->instance->team->for_dyn_nb_threads_exited[index].i ) ) ==
	        MPC_OMP_NOWAIT_STOP_SYMBOL )
	{
		mpc_thread_yield();
	}

	/* Fill private info about the loop */
	t->info.loop_infos.type = MPC_OMP_LOOP_TYPE_ULL;
	loop = &( t->info.loop_infos.loop.mpcomp_ull );
	loop->up = up;
	loop->lb = lb;
	loop->b = b;
	loop->incr = incr;
	loop->chunk_size = chunk_size;

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_work( ompt_work_loop,
                             ompt_scope_begin,
                             __loop_get_num_iters_gen( &( t->info.loop_infos )));
#endif /* OMPT_SUPPORT */

	/* Try to change the number of remaining chunks */
	t->for_dyn_total[index] = __loop_get_static_nb_chunks_per_rank_ull(
	                              t->rank, num_threads, &( t->info.loop_infos.loop.mpcomp_ull ) );
	( void ) OPA_cas_int( &( t->for_dyn_remain[index].i ), -1, t->for_dyn_total[index] );
}

int mpc_omp_loop_ull_dynamic_begin( bool up, unsigned long long lb,
                                     unsigned long long b,
                                     unsigned long long incr,
                                     unsigned long long chunk_size,
                                     unsigned long long *from,
                                     unsigned long long *to )
{
	struct mpc_omp_thread_s *t;

	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();

	/* Grab the thread info */
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );

	t->dyn_last_target = t;

	/* Initialization of loop internals */
	t->for_dyn_last_loop_iteration = 0;
	__loop_dyn_init_ull( t, up, lb, b, incr, chunk_size );

	if ( !( t->instance->root ) )
	{
		if( from ) *from = lb;
		if( to ) *to = b;
		t->schedule_type =
		    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_DYN_LOOP;
		t->schedule_is_forced = 0;
		return 1;
	}

	return ( !from && !to ) ? -1 : mpc_omp_loop_ull_dynamic_next( from, to );
}

/* DEBUG */
void mpc_omp_loop_ull_dynamic_next_debug( __UNUSED__ char *funcname )
{
	int i;
	struct mpc_omp_thread_s *t; /* Info on the current thread */
	/* Grab the thread info */
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	assert( t->instance );
	const int tree_depth = t->instance->tree_depth;

	for ( i = 0; i < tree_depth; i++ )
	{
		mpc_common_nodebug( "[%d] %s:\ttarget[%d] = %d \t shift[%d] = %d", t->rank,
		              funcname, i, t->for_dyn_target[i], i, t->for_dyn_shift[i] );
	}
}

int mpc_omp_loop_ull_dynamic_next( unsigned long long *from,
                                    unsigned long long *to )
{
	mpc_omp_mvp_t *target_mvp;
	struct mpc_omp_thread_s *t, *target;
	int found, target_idx, barrier_nthreads, ret;
	/* Grab the thread info */
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	/* Stop the stealing at the following depth.
	 * Nodes above this depth will not be traversed */
	const int num_threads = t->info.num_threads;

	/* 1 thread => no stealing, directly get a chunk */
	if ( num_threads == 1 )
	{
		return __loop_dyn_get_chunk_from_rank_ull( t, t, from, to );
	}

	/* Check that the target is allocated */
	__loop_dyn_target_init( t );

	/*
	 * WORKAROUND (pr35196.c):
	 * Stop if the current thread already executed
	 * the last iteration of the current loop
	 */
	if ( t->for_dyn_last_loop_iteration == 1 )
	{
		__loop_dyn_target_reset( t );
		return 0;
	}

	int *tree_base = t->instance->tree_base + 1;
	const int tree_depth = t->instance->tree_depth - 1;
	const int top_level = t->instance->root->depth;
	/* Compute the index of the target */
	target = t->dyn_last_target;
	found = 1;
	ret = 0;

	/* While it is not possible to get a chunk */
	while ( !__loop_dyn_get_chunk_from_rank_ull( t, target, from, to ) )
	{
do_increase:

		if ( ret )
		{
			found = 0;
			break;
		}

		ret = __loop_dyn_increase( t->for_dyn_shift, tree_base, tree_depth, top_level, 1 );
		/* Add t->for_dyn_shift and t->mvp->tree_rank[i] w/out carry over and store it into t->for_dyn_target */
		__loop_dyn_add( t->for_dyn_target, t->for_dyn_shift, t->mvp->tree_rank, tree_base, tree_depth, top_level, 0 );
		/* Compute the index of the target */
		target_idx = __loop_dyn_get_victim_rank( t );
		target_mvp = t->instance->mvps[target_idx].ptr.mvp;
		target = ( target_mvp ) ? target_mvp->threads : NULL;

		if ( !( target ) )
		{
			goto do_increase;
		}

		barrier_nthreads = target_mvp->father->barrier_num_threads;

		/* Stop if the target is not a launch thread */
		if ( t->for_dyn_target[tree_depth - 1] >= barrier_nthreads )
		{
			goto do_increase;
		}
	}

	/* Did we exit w/ or w/out a chunk? */
	if ( found )
	{
		/* WORKAROUND (pr35196.c):
		* to avoid issue w/ lastprivate and GCC code generation,
		* check if this is the last chunk to avoid further execution
		* for this thread.
		*/
		if ( *to == t->info.loop_infos.loop.mpcomp_ull.b )
		{
			t->for_dyn_last_loop_iteration = 1;
		}

#if OMPT_SUPPORT
        ompt_data_t ompt_from = { (uint64_t) *from };
        _mpc_omp_ompt_callback_dispatch( ompt_dispatch_iteration,
                                     ompt_from );
#endif /* OMPT_SUPPORT */

		t->dyn_last_target = target;
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
int mpc_omp_loop_ull_ordered_dynamic_begin( bool up, unsigned long long lb,
        unsigned long long b,
        unsigned long long incr,
        unsigned long long chunk_size,
        unsigned long long *from,
        unsigned long long *to )
{
	struct mpc_omp_thread_s *t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_DYN_LOOP;
	t->schedule_is_forced = 1;
	// TODO EXTEND MODIF WITH UP VALUE
	mpc_common_nodebug( "[%d] %s: %d -> %d [%d] cs:%d", t->rank, __func__, lb, b, incr,
	              chunk_size );
	const int res =
	    mpc_omp_loop_ull_dynamic_begin( up, lb, b, incr, chunk_size, from, to );

	if ( from )
	{
		t->info.loop_infos.loop.mpcomp_ull.cur_ordered_iter = *from;
	}

	mpc_common_nodebug( "[%d] %s: exit w/ res=%d", t->rank, __func__, res );
	return res;
}

int mpc_omp_loop_ull_ordered_dynamic_next( unsigned long long *from,
        unsigned long long *to )
{
	struct mpc_omp_thread_s *t;
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	const int ret = mpc_omp_loop_ull_dynamic_next( from, to );
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
mpc_omp_for_dyn_coherency_end_barrier()
{
	struct mpc_omp_thread_s *t ;
	mpc_omp_instance_t *instance ;
	mpc_omp_team_t *team ;
	int i ;
	int n ;
	/* Grab info on the current thread */
	t = ( struct mpc_omp_thread_s * ) mpc_omp_tls;
	assert( t != NULL );
	mpc_common_nodebug( "[%d] _mpcomp_for_dyn_coherency_checking_end_barrier: "
	              "Checking for %d thread(s)...",
	              t->rank, t->info.num_threads ) ;
	instance = t->instance ;
	assert( instance != NULL ) ;
	team = instance->team ;
	assert( team != NULL ) ;
	/* Check team coherency */
	n = 0 ;

	for ( i = 0 ; i < MPC_OMP_MAX_ALIVE_FOR_DYN + 1 ; i++ )
	{
		switch ( OPA_load_int( &team->for_dyn_nb_threads_exited[i].i ) )
		{
			case 0:
				break ;

			case MPC_OMP_NOWAIT_STOP_SYMBOL:
				n++ ;
				break ;

			default:
				/* This array should contain only '0' and
				* one MPC_OMP_MAX_ALIVE_FOR_DYN */
				not_reachable() ;
				break ;
		}

		mpc_common_nodebug( "[%d] _mpcomp_for_dyn_coherency_checking_end_barrier: "
		              "TEAM - FOR_DYN - nb_threads_exited[%d] = %d"
		              ,
		              t->rank, i, team->for_dyn_nb_threads_exited[i].i ) ;
	}

	if ( n != 1 )
	{
		/* This array should contain exactly 1 MPC_OMP_MAX_ALIVE_FOR_DYN */
		not_reachable() ;
	}

	/* Check tree coherency */

	/* Check thread coherency */
	for ( i = 0 ; i < t->info.num_threads ; i++ )
	{
		struct mpc_omp_thread_s *target_t ;
		int j ;
		target_t = &( instance->mvps[i].ptr.mvp->threads[0] ) ;
		mpc_common_nodebug( "[%d] _mpcomp_for_dyn_coherency_checking_end_barrier: "
		              "Checking thread %d...",
		              t->rank, target_t->rank ) ;

		/* Checking for_dyn_current */
		if ( t->for_dyn_current != target_t->for_dyn_current )
		{
			not_reachable() ;
		}

		/* Checking for_dyn_remain */
		for ( j = 0 ; j < MPC_OMP_MAX_ALIVE_FOR_DYN + 1 ; j++ )
		{
			if ( OPA_load_int( &( target_t->for_dyn_remain[j].i ) ) != -1 )
			{
				not_reachable() ;
			}
		}

		/* Checking for_dyn_target */
		/* for_dyn_tarfer can be NULL if there were no dynamic loop
		* since the beginning of the program
		*/
		if ( target_t->for_dyn_target != NULL )
		{
			for ( j = 0 ; j < instance->tree_depth ; j++ )
			{
				if ( target_t->for_dyn_target[j] != target_t->mvp->tree_rank[j] )
				{
					mpc_common_nodebug( "[%d] _mpcomp_for_dyn_coherency_checking_end_barrier: "
					              "error rank %d has target[%d] = %d (depth: %d, top_level: %d, for_dyn_current=%d)",
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
		if ( target_t->for_dyn_shift != NULL )
		{
			// for ( j = t->info.new_root->depth ; j < instance->tree_depth ; j++ )
			for ( j = 0 ; j < instance->tree_depth ; j++ )
			{
				if ( target_t->for_dyn_shift[j] != 0 )
				{
					mpc_common_nodebug( "[%d] _mpcomp_for_dyn_coherency_checking_end_barrier: "
					              "error rank %d has shift[%d] = %d (dept: %d, top_level: %d, for_dyn_current=%d)",
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

/****************
 * LOOP RUNTIME *
 ****************/

TODO( runtime schedule
      : ICVs are not well transfered ! )

/****
 *
 * LONG VERSION
 *
 *
 *****/
int mpc_omp_runtime_loop_begin( long lb, long b, long incr, long *from,
                                 long *to )
{
	int ret;
	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_RUNTIME_LOOP;
	t->schedule_is_forced = 1;
	const int run_sched_var = t->info.icvs.run_sched_var;
	const long chunk_size = t->info.icvs.modifier_sched_var;

	switch ( run_sched_var )
	{
		case omp_sched_static:
			ret = mpc_omp_static_loop_begin( lb, b, incr, chunk_size, from, to );
			break;

		case omp_sched_dynamic:
			ret = mpc_omp_dynamic_loop_begin( lb, b, incr, chunk_size, from, to );
			break;

		case omp_sched_guided:
			ret = mpc_omp_guided_loop_begin( lb, b, incr, chunk_size, from, to );
			break;

		default:
			not_reachable();
	}

	return ret;
}

int mpc_omp_runtime_loop_next( long *from, long *to )
{
	int ret;
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	const int run_sched_var = t->info.icvs.run_sched_var;

	switch ( run_sched_var )
	{
		case omp_sched_static:
			ret = mpc_omp_static_loop_next( from, to );
			break;

		case omp_sched_dynamic:
			ret = mpc_omp_dynamic_loop_next( from, to );
			break;

		case omp_sched_guided:
			ret = mpc_omp_guided_loop_next( from, to );
			break;

		default:
			not_reachable();
	}

	return ret;
}

void mpc_omp_runtime_loop_end( void )
{
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	const int run_sched_var = t->info.icvs.run_sched_var;

	switch ( run_sched_var )
	{
		case omp_sched_static:
			mpc_omp_static_loop_end();
			break;

		case omp_sched_dynamic:
			mpc_omp_dynamic_loop_end();
			break;

		case omp_sched_guided:
			mpc_omp_guided_loop_end();
			break;

		default:
			not_reachable();
	}
}

void mpc_omp_runtime_loop_end_nowait()
{
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	const int run_sched_var = t->info.icvs.run_sched_var;

	switch ( run_sched_var )
	{
		case omp_sched_static:
			mpc_omp_static_loop_end_nowait();
			break;

		case omp_sched_dynamic:
			mpc_omp_dynamic_loop_end_nowait();
			break;

		case omp_sched_guided:
			mpc_omp_guided_loop_end_nowait();
			break;

		default:
			not_reachable();
	}
}

/****
 *
 * ORDERED LONG VERSION
 *
 *
 *****/

int mpc_omp_ordered_runtime_loop_begin( long lb, long b, long incr, long *from,
        long *to )
{
	int ret;
	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_RUNTIME_LOOP;
	t->schedule_is_forced = 1;
	const int run_sched_var = t->info.icvs.run_sched_var;
	const long chunk_size = t->info.icvs.modifier_sched_var;

	switch ( run_sched_var )
	{
		case omp_sched_static:
			ret = mpc_omp_ordered_static_loop_begin( lb, b, incr, chunk_size, from, to );
			break;

		case omp_sched_dynamic:
			ret =
			    mpc_omp_ordered_dynamic_loop_begin( lb, b, incr, chunk_size, from, to );
			break;

		case omp_sched_guided:
			ret = mpc_omp_ordered_guided_loop_begin( lb, b, incr, chunk_size, from, to );
			break;

		default:
			not_reachable();
	}

	return ret;
}

int mpc_omp_ordered_runtime_loop_next( long *from, long *to )
{
	int ret;
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	const int run_sched_var = t->info.icvs.run_sched_var;

	switch ( run_sched_var )
	{
		case omp_sched_static:
			ret = mpc_omp_ordered_static_loop_next( from, to );
			break;

		case omp_sched_dynamic:
			ret = mpc_omp_ordered_dynamic_loop_next( from, to );
			break;

		case omp_sched_guided:
			ret = mpc_omp_ordered_guided_loop_next( from, to );
			break;

		default:
			not_reachable();
	}

	return ret;
}

void mpc_omp_ordered_runtime_loop_end( void )
{
	mpc_omp_runtime_loop_end();
}

void mpc_omp_ordered_runtime_loop_end_nowait( void )
{
	mpc_omp_runtime_loop_end();
}

/****
 *
 * ULL VERSION
 *
 *
 *****/
int mpc_omp_loop_ull_runtime_begin( bool up, unsigned long long lb,
                                     unsigned long long b,
                                     unsigned long long incr,
                                     unsigned long long *from,
                                     unsigned long long *to )
{
	int ret;
	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_RUNTIME_LOOP;
	t->schedule_is_forced = 1;
	const unsigned long long chunk_size =
	    ( unsigned long long ) t->info.icvs.modifier_sched_var;

	switch ( t->info.icvs.run_sched_var )
	{
		case omp_sched_static:
			ret = mpc_omp_static_loop_begin_ull( up, lb, b, incr, chunk_size, from, to );
			break;

		case omp_sched_dynamic:
			ret = mpc_omp_loop_ull_dynamic_begin( up, lb, b, incr, chunk_size, from, to );
			break;

		case omp_sched_guided:
			ret = mpc_omp_loop_ull_guided_begin( up, lb, b, incr, chunk_size, from, to );
			break;

		default:
			not_reachable();
	}

	return ret;
}

int mpc_omp_loop_ull_runtime_next( unsigned long long *from,
                                    unsigned long long *to )
{
	int ret;
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();

	switch ( t->info.icvs.run_sched_var )
	{
		case omp_sched_static:
			ret = mpc_omp_static_loop_next_ull( from, to );
			break;

		case omp_sched_dynamic:
			ret = mpc_omp_loop_ull_dynamic_next( from, to );
			break;

		case omp_sched_guided:
			ret = mpc_omp_loop_ull_guided_next( from, to );
			break;

		default:
			not_reachable();
	}

	return ret;
}

/****
 *
 * ORDERED ULL VERSION
 *
 *
 *****/
int mpc_omp_loop_ull_ordered_runtime_begin( bool up, unsigned long long lb,
        unsigned long long b,
        unsigned long long incr,
        unsigned long long *from,
        unsigned long long *to )
{
	int ret;
	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	t->schedule_type =
	    ( t->schedule_is_forced ) ? t->schedule_type : MPC_OMP_COMBINED_RUNTIME_LOOP;
	t->schedule_is_forced = 1;
	const int run_sched_var = t->info.icvs.run_sched_var;
	const unsigned long long chunk_size =
	    ( unsigned long long ) t->info.icvs.modifier_sched_var;
	mpc_common_nodebug( "%s: value of schedule %d", __func__, run_sched_var );

	switch ( run_sched_var )
	{
		case omp_sched_static:
			ret = mpc_omp_ordered_static_loop_begin_ull( up, lb, b, incr, chunk_size,
			        from, to );
			break;

		case omp_sched_dynamic:
			ret = mpc_omp_loop_ull_ordered_dynamic_begin( up, lb, b, incr, chunk_size,
			        from, to );
			break;

		case omp_sched_guided:
			ret = mpc_omp_loop_ull_ordered_guided_begin( up, lb, b, incr, chunk_size,
			        from, to );
			break;

		default:
			not_reachable();
	}

	return ret;
}

int mpc_omp_loop_ull_ordered_runtime_next( unsigned long long *from,
        unsigned long long *to )
{
	int ret;
	/* Handle orphaned directive (initialize OpenMP environment) */
	mpc_omp_init();
	/* Grab the thread info */
	struct mpc_omp_thread_s *t = mpc_omp_get_thread_tls();
	const int run_sched_var = t->info.icvs.run_sched_var;

	switch ( run_sched_var )
	{
		case omp_sched_static:
			ret = mpc_omp_ordered_static_loop_next_ull( from, to );
			break;

		case omp_sched_dynamic:
			ret = mpc_omp_loop_ull_ordered_dynamic_next( from, to );
			break;

		case omp_sched_guided:
			ret = mpc_omp_loop_ull_ordered_guided_next( from, to );
			break;

		default:
			not_reachable();
	}

	return ret;
}

/***********
 * ORDERED *
 ***********/


static inline void __loop_internal_ordered_begin( mpc_omp_thread_t *t, mpc_omp_loop_gen_info_t* loop_infos )
{
    assert( loop_infos && loop_infos->type == MPC_OMP_LOOP_TYPE_LONG );
    mpc_omp_loop_long_iter_t* loop = &( loop_infos->loop.mpcomp_long );
    const long cur_ordered_iter = loop->cur_ordered_iter;

	/* First iteration of the loop -> initialize 'next_ordered_offset' */
	if( cur_ordered_iter == loop->lb ) 
    {
        while( OPA_cas_int( &( t->instance->team->next_ordered_offset_finalized ), 0, 1 ) )
            mpc_thread_yield();

        return;
    }

    /* Do we have to wait for the right iteration? */
    while( cur_ordered_iter != ( loop->lb + loop->incr * t->instance->team->next_ordered_offset ) )
	    mpc_thread_yield();
} 

static inline void __loop_internal_ordered_begin_ull( mpc_omp_thread_t *t, mpc_omp_loop_gen_info_t* loop_infos )
{
    assert( loop_infos && loop_infos->type == MPC_OMP_LOOP_TYPE_ULL );
    mpc_omp_loop_ull_iter_t* loop = &( loop_infos->loop.mpcomp_ull );
    const unsigned long long cur_ordered_iter = loop->cur_ordered_iter;

    /* First iteration of the loop -> initialize 'next_ordered_offset' */
    if( cur_ordered_iter == loop->lb )                   
    {
        while( OPA_cas_int( &( t->instance->team->next_ordered_offset_finalized), 0, 1 ) )
            mpc_thread_yield();

        return;
    }

    /* Do we have to wait for the right iteration? */
    while( cur_ordered_iter != ( loop->lb + loop->incr * t->instance->team->next_ordered_offset_ull ) )
        mpc_thread_yield();
}

void mpc_omp_ordered_begin( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t;
    mpc_omp_loop_gen_info_t* loop_infos; 

	mpc_omp_init();

	t = (mpc_omp_thread_t *) mpc_omp_tls;
	assert(t != NULL); 

    /* No need to check something in case of 1 thread */
    if ( t->info.num_threads == 1 )
         return ;

    loop_infos = &( t->info.loop_infos );

    if( loop_infos->type == MPC_OMP_LOOP_TYPE_LONG )
    { 
//        fprintf(stderr, ":: %s :: Start ordered  %d\n", __func__, t->rank);
        __loop_internal_ordered_begin( t, loop_infos );
    }
    else
    {
        __loop_internal_ordered_begin_ull( t, loop_infos );
    }
}

static inline void __internal_ordered_end( mpc_omp_thread_t* t, mpc_omp_loop_gen_info_t* loop_infos  )
{
    int isLastIteration;

    assert( loop_infos && loop_infos->type == MPC_OMP_LOOP_TYPE_LONG );
    mpc_omp_loop_long_iter_t* loop = &( loop_infos->loop.mpcomp_long );

    isLastIteration = 0;
    isLastIteration += (loop->up  && loop->cur_ordered_iter >= loop->b) ? ( long) 1 : (long) 0;
    isLastIteration += (!loop->up && loop->cur_ordered_iter <= loop->b) ? ( long) 1 : (long) 0;

	loop->cur_ordered_iter += loop->incr;

    isLastIteration += ((loop->incr > 0)  && loop->cur_ordered_iter >= loop->b) ? ( long) 1 : (long) 0;
    isLastIteration += ((loop->incr < 0)  && loop->cur_ordered_iter <= loop->b) ? ( long) 1 : (long) 0;

    if( isLastIteration )
    {
	    t->instance->team->next_ordered_offset = (long) 0;
        OPA_cas_int(&(t->instance->team->next_ordered_offset_finalized), 1, 0 );
    }
    else
    {
        t->instance->team->next_ordered_offset += (long) 1;
    }
}

static inline void __internal_ordered_end_ull( mpc_omp_thread_t* t, mpc_omp_loop_gen_info_t* loop_infos  )
{
    int isLastIteration;
    assert( loop_infos && loop_infos->type == MPC_OMP_LOOP_TYPE_ULL );
    mpc_omp_loop_ull_iter_t* loop = &( loop_infos->loop.mpcomp_ull );

    unsigned long long cast_cur_ordered_iter = (unsigned long long) loop->cur_ordered_iter;

    isLastIteration = 0;
    isLastIteration += (loop->up && cast_cur_ordered_iter >= loop->b) ?  (unsigned long long) 1 : (unsigned long long) 0;
    isLastIteration += (!loop->up && cast_cur_ordered_iter <= loop->b) ? (unsigned long long) 1 : (unsigned long long) 0;

    if( loop->up )
    {
	    cast_cur_ordered_iter += loop->incr;
    }
    else
    {
	    cast_cur_ordered_iter -=  loop->incr;
    }

    loop->cur_ordered_iter = ( long ) cast_cur_ordered_iter; 

    isLastIteration += (loop->up && cast_cur_ordered_iter >= loop->b) ?  (unsigned long long) 1 : (unsigned long long) 0;
    isLastIteration += (!loop->up && cast_cur_ordered_iter <= loop->b) ? (unsigned long long) 1 : (unsigned long long) 0;

    if( isLastIteration )
    {
	    t->instance->team->next_ordered_offset_ull = (long) 0;
        OPA_cas_int(&(t->instance->team->next_ordered_offset_finalized), 1, 0 );
    }
    else
    {
        t->instance->team->next_ordered_offset_ull += (long) 1;
    }
}

void mpc_omp_ordered_end( void )
{
#if OMPT_SUPPORT && MPCOMPT_HAS_FRAME_SUPPORT
    _mpc_omp_ompt_frame_get_wrapper_infos( MPC_OMP_GOMP );
#endif /* OMPT_SUPPORT */

	mpc_omp_thread_t *t;
    mpc_omp_loop_gen_info_t* loop_infos; 

	t = (mpc_omp_thread_t *) mpc_omp_tls;
	assert(t != NULL); 

    /* No need to check something in case of 1 thread */
    if ( t->info.num_threads == 1 ) return ;

    loop_infos = &( t->info.loop_infos );

    if( loop_infos->type == MPC_OMP_LOOP_TYPE_LONG )
    { 
        __internal_ordered_end( t, loop_infos );
    }
    else
    {
        __internal_ordered_end_ull( t, loop_infos );
    }
}
