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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_loop.h"
#include "mpcomp_loop_static_ull.h"

/* Compute the chunk for a static schedule (without specific chunk size)
 * Original loop -> lb -> b step incr
 * New loop -> *from -> *to step incr
 */
int mpcomp_static_schedule_get_single_chunk_ull( mpcomp_loop_ull_iter_t* loop, unsigned long long *from, unsigned long long *to)
{
    mpcomp_thread_t *t;

    /* Grab info on the current thread */
    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);      
    
    const unsigned long long rank = (unsigned long long) t->rank;
    const unsigned long long num_threads = (unsigned long long) t->info.num_threads;
    const unsigned long long trip_count = mpcomp_internal_loop_get_num_iters_ull( loop->lb, loop->b, loop->incr, loop->up );
 
	if ( rank >= trip_count ) 
    {
		return 0 ;
	}

    const unsigned long long extra_chunk = ( trip_count % num_threads && trip_count > num_threads ) + ( num_threads == 1 ) ? 1 : 0;
    const unsigned long long decal = (rank < extra_chunk) ? rank * loop->incr : extra_chunk * loop->incr;

    *from = loop->lb + decal + rank * loop->chunk_size * loop->incr;
    *to = *from + loop->chunk_size * loop->incr + ( ( rank < extra_chunk ) ? loop->chunk_size * loop->incr : 0 ) ;

    /* The final additionnal chunk is smaller, so its computation is a little bit different */
    *to = ( loop->up  && *to > loop->b ) ? loop->b : *to;
    *to = ( !loop->up && *to < loop->b ) ? loop->b : *to;
    
	return 1 ;
}

/* Return the chunk #'chunk_num' assuming a static schedule with 'chunk_size'
 * as a chunk size */
void __mpcomp_static_schedule_get_specific_chunk_ull(  unsigned long long rank, unsigned long long num_threads, mpcomp_loop_ull_iter_t* loop, unsigned long long chunk_num, unsigned long long *from, unsigned long long *to)
{
    const unsigned long long decal = chunk_num * num_threads * loop->chunk_size * loop->incr;
    *from = loop->lb + decal + loop->chunk_size * loop->incr * rank;
    *to = *from + loop->chunk_size * loop->incr; 

    *to = ( loop->up  && *to > loop->b ) ? loop->b : *to;
    *to = ( !loop->up && *to < loop->b ) ? loop->b : *to;
}

void mpcomp_static_loop_init_ull(mpcomp_thread_t *t, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size)
{
    return;
}

int mpcomp_loop_ull_static_begin (bool up, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size,
                unsigned long long *from, unsigned long long *to)
{
    mpcomp_thread_t *t;
    mpcomp_loop_ull_iter_t* loop;

    mpcomp_init();

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    //sctk_error( "[%d] %s: %d -> %d [%d] cs:%d", t->rank, __func__, lb, b, incr, chunk_size ) ;
    t->schedule_type = ( t->schedule_is_forced ) ? t->schedule_type : MPCOMP_COMBINED_STATIC_LOOP;  
    t->schedule_is_forced = 0;

    /* Fill private info about the loop */
    t->info.loop_infos.type = MPCOMP_LOOP_TYPE_ULL;
    loop = &( t->info.loop_infos.loop.mpcomp_ull );
    loop->up = up;
    loop->lb = lb; 
    loop->b = b;
    loop->incr = incr;
    loop->chunk_size = chunk_size;
    t->static_nb_chunks = __mpcomp_compute_static_nb_chunks_per_rank_ull(t->rank, t->info.num_threads, loop );

        /* As the loop_next function consider a chunk as already been realised
             we need to initialize to 0 minus 1 */
    t->static_current_chunk = -1 ;
    return mpcomp_loop_ull_static_next (from, to);
}

int mpcomp_loop_ull_static_next (unsigned long long *from, unsigned long long *to)
{
    mpcomp_thread_t *t;

    t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert(t != NULL);

    /* Retrieve the number of threads and the rank of this thread */
    const unsigned long long num_threads = t->info.num_threads;
    const unsigned long long rank = t->rank;

    /* Next chunk */
    t->static_current_chunk++;

    //sctk_error( "[%d] %s: checking if current_chunk %d >= nb_chunks %d?", t->rank, __func__, t->static_current_chunk, t->static_nb_chunks);

    /* Check if there is still a chunk to execute */
    if (t->static_current_chunk >= t->static_nb_chunks) return 0;

    __mpcomp_static_schedule_get_specific_chunk_ull( rank, num_threads, &( t->info.loop_infos.loop.mpcomp_ull ), t->static_current_chunk, from, to);
    //sctk_error( "[%d] %s: got a chunk %d -> %d", t->rank, __func__, *from, *to ) ;
    return 1;
}

/****
 *
 * ULL ORDERED VERSION
 *
 *
 *****/
int mpcomp_loop_ull_ordered_static_begin (bool up, unsigned long long lb, unsigned long long b, unsigned long long incr, unsigned long long chunk_size,
                    unsigned long long *from, unsigned long long *to)
{
     mpcomp_thread_t *t; 
     int res;


     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);  
         
    t->schedule_type = ( t->schedule_is_forced ) ? t->schedule_type : MPCOMP_COMBINED_STATIC_LOOP;  
    t->schedule_is_forced = 0;

     res = mpcomp_loop_ull_static_begin(up, lb, b, incr, chunk_size, from, to);

     t->current_ordered_iteration = *from;
         
     return res;
}

int mpcomp_loop_ull_ordered_static_next(unsigned long long *from, unsigned long long *to)
{
     mpcomp_thread_t *t;
     int res ;

     res = mpcomp_loop_ull_static_next(from, to);

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     t->current_ordered_iteration = *from;

     return res;
}

