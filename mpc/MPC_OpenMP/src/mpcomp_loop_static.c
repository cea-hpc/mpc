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
#include "mpcmicrothread_internal.h"
#include <mpcomp_internal.h>
#include "sctk.h"
#include <sctk_debug.h>



/* Compute the chunk for a static schedule (without specific chunk size) */
void __mpcomp_static_schedule_get_single_chunk (long lb, long b, long incr, long *from,
						long *to)
{
     /*
       Original loop -> lb -> b step incr
       New loop -> *from -> *to step incr
     */

     int trip_count;
     int chunk_size;
     int num_threads;
     mpcomp_thread_t *t;
     int rank;

     /* Grab info on the current thread */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);      

     num_threads = t->info.num_threads;
     rank = t->rank;
     
     trip_count = (b - lb) / incr;
     
     if ( (b - lb) % incr != 0 )
	  trip_count++;
     
     chunk_size = trip_count / num_threads;
     
     if (rank < (trip_count % num_threads)) {
	  /* The first part is homogeneous with a chunk size a little bit larger */
	  *from = lb + rank * (chunk_size + 1) * incr;
	  *to = lb + (rank + 1) * (chunk_size + 1) * incr;
     } else {
	  /* The final part has a smaller chunk_size, therefore the computation is
	     splitted in 2 parts */
	  *from = lb + (trip_count % num_threads) * (chunk_size + 1) * incr +
	       (rank - (trip_count % num_threads)) * chunk_size * incr;
	  *to = lb + (trip_count % num_threads) * (chunk_size + 1) * incr +
	       (rank + 1 - (trip_count % num_threads)) * chunk_size * incr;
     }

     sctk_nodebug ("__mpcomp_static_schedule_get_single_chunk: "
		   "#%d (%d -> %d step %d) -> (%d -> %d step %d)", rank, lb, b,
		   incr, *from, *to, incr);
}

/* Return the number of chunks that a static schedule would create */
int __mpcomp_static_schedule_get_nb_chunks (long lb, long b, long incr,
					    long chunk_size)
{
     /* Original loop: lb -> b step incr */
     
     long trip_count;
     long nb_chunks_per_thread;
     int nb_threads;
     mpcomp_thread_t *t;
     int rank;

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);   

     /* Retrieve the number of threads and the rank of the current thread */
     nb_threads = t->info.num_threads;
     rank = t->rank;

     /* Compute the trip count (total number of iterations of the original loop) */
     trip_count = (b - lb) / incr;

     if ( (b - lb) % incr != 0 )
	  trip_count++;

     /* Compute the number of chunks per thread (floor value) */
     nb_chunks_per_thread = trip_count / (chunk_size * nb_threads);

     /* The first threads will have one more chunk (according to the previous
	approximation) */
     if (rank < (trip_count / chunk_size) % nb_threads)
	  nb_chunks_per_thread++;

     /* One thread may have one more additionnal smaller chunk to finish the
	iteration domain */
     if (rank == (trip_count / chunk_size) % nb_threads
	 && trip_count % chunk_size != 0)
	  nb_chunks_per_thread++;

     sctk_nodebug
	  ("__mpcomp_static_schedule_get_nb_chunks[%d]: %ld -> [%ld] (cs=%ld) final nb_chunks = %ld",
	   rank, lb, b, incr, chunk_size, nb_chunks_per_thread);


     return nb_chunks_per_thread;
}

/* Return the chunk #'chunk_num' assuming a static schedule with 'chunk_size'
 * as a chunk size */
void __mpcomp_static_schedule_get_specific_chunk (long lb, long b, long incr,
						  long chunk_size, long chunk_num,
						  long *from, long *to)
{
     mpcomp_thread_t *t;
     long trip_count;
     long nb_threads;
     int rank;

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);  

     /* Retrieve the number of threads and the rank of this thread */
     nb_threads = t->info.num_threads;
     rank = t->rank;

     /* Compute the trip count (total number of iterations of the original loop) */
     trip_count = (b - lb) / incr;

     if ( (b - lb) % incr != 0 )
	  trip_count++;

     /* The final additionnal chunk is smaller, so its computation is a little bit
	different */
     if (rank == (trip_count / chunk_size) % nb_threads
	 && trip_count % chunk_size != 0
	 && chunk_num == __mpcomp_static_schedule_get_nb_chunks (lb, b, incr,
								 chunk_size) - 1) {
	  //int last_chunk_size = trip_count % chunk_size;

	  *from = lb + (trip_count / chunk_size) * chunk_size * incr;
	  *to = lb + trip_count * incr;

	  sctk_nodebug ("__mpcomp_static_schedule_get_specific_chunk: "
			"Thread %d / Chunk %ld: %ld -> %ld (excl) step %ld => "
			"%ld -> %ld (excl) step %ld (chunk of %ld)\n",
			rank, nb_chunks_per_thread, lb, b, incr, from, to, incr,
			last_chunk_size);	  
     } else {
	  *from = lb + chunk_num * nb_threads * chunk_size * incr
	       + chunk_size * incr * rank;
	  *to = lb + chunk_num * nb_threads * chunk_size * incr +
	       chunk_size * incr * rank + chunk_size * incr;

	  sctk_nodebug ("__mpcomp_static_schedule_get_specific_chunk: "
			"Thread %d / Chunk %ld: %ld -> %ld (excl) step %ld => "
			"%ld -> %ld (excl) step %ld (chunk of %ld)\n",
			rank, chunk_num, lb, b, incr, *from, *to, incr,
			chunk_size);
     }
}

int __mpcomp_static_loop_begin (long lb, long b, long incr, long chunk_size,
				long *from, long *to)
{
     mpcomp_thread_t *t;
     
     __mpcomp_init ();

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);  

     /* Automatic chunk size -> at most one chunk */
     if (chunk_size == 0) {
	  t->static_nb_chunks = 1;
	  __mpcomp_static_schedule_get_single_chunk (lb, b, incr, from, to);

	  t->info.loop_lb = lb;
	  t->info.loop_b = b;
	  t->info.loop_incr = incr;
	  t->info.loop_chunk_size = 1;
     } else {
	  int nb_threads;
	  int rank;
	  
	  /* Retrieve the number of threads and the rank of this thread */
	  nb_threads = t->info.num_threads;

	  rank = t->rank;
	  
	  t->static_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank(rank, nb_threads, 
								       lb, b, incr, 
								       chunk_size );

	  /* No chunk -> exit the 'for' loop */
	  if ( t->static_nb_chunks <= 0 )
	       return 0;

	  t->info.loop_lb = lb;
	  t->info.loop_b = b;
	  t->info.loop_incr = incr;
	  t->info.loop_chunk_size = chunk_size;
	  
	  t->static_current_chunk = 0 ;
	  __mpcomp_get_specific_chunk_per_rank(rank, nb_threads, lb, b, incr, chunk_size, 
					       0, from, to );
     }
     return 1;
}

int __mpcomp_static_loop_next (long *from, long *to)
{
     mpcomp_thread_t *t;
     int nb_threads;
     int rank;
     
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);  

     /* Retrieve the number of threads and the rank of this thread */
     nb_threads = t->info.num_threads;
     rank = t->rank;

     /* Next chunk */
     t->static_current_chunk++;

     /* Check if there is still a chunk to execute */
     if (t->static_current_chunk >= t->static_nb_chunks)
	  return 0;

     __mpcomp_get_specific_chunk_per_rank(rank, nb_threads, t->info.loop_lb,
					  t->info.loop_b, t->info.loop_incr, t->info.loop_chunk_size,
					  t->static_current_chunk, from, to);
     
     return 1;
}

void __mpcomp_static_loop_end ()
{
     __mpcomp_barrier();
}

void __mpcomp_static_loop_end_nowait ()
{
     /* Nothing to do */
}


/****
 *
 * ORDERED VERSION 
 *
 *
 *****/

int __mpcomp_ordered_static_loop_begin (long lb, long b, long incr, long chunk_size,
					long *from, long *to)
{
     mpcomp_thread_t *t;
     int res;

     res = __mpcomp_static_loop_begin(lb, b, incr, chunk_size, from, to);

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);  
     
     t->current_ordered_iteration = *from;
     
     return res;
}

int __mpcomp_ordered_static_loop_next(long *from, long *to)
{
     mpcomp_thread_t *t;
     int res ;
     
     res = __mpcomp_static_loop_next(from, to);
     
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);
     
     t->current_ordered_iteration = *from;
     
     return res;
}

void __mpcomp_ordered_static_loop_end()
{
     __mpcomp_static_loop_end();
}

void __mpcomp_ordered_static_loop_end_nowait()
{
     __mpcomp_static_loop_end();
}

