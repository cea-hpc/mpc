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
#include "sctk_debug.h"
#include "mpcomp_types.h"
#include "mpcomp_loop.h"
#include "mpcomp_loop_dyn_utils.h"

/****
 *
 * CHUNK MANIPULATION
 *
 *
 *****/
/* Return the number of chunks that a static schedule will create for the thread 'rank' */
int __mpcomp_get_static_nb_chunks_per_rank(int rank, int num_threads,
                                           mpcomp_loop_long_iter_t *loop) {
  mpcomp_thread_t *t;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  long nb_chunks_per_thread;
  const long trip_count =
      __mpcomp_internal_loop_get_num_iters(loop->lb, loop->b, loop->incr);

  /* Compute the number of chunks per thread (floor value) */
  loop->chunk_size =
      (loop->chunk_size) ? loop->chunk_size : trip_count / num_threads;
  loop->chunk_size = (loop->chunk_size) ? loop->chunk_size : (long)1;

  nb_chunks_per_thread = trip_count / (loop->chunk_size * num_threads);

  /* Compute the number of extrat chunk nb_chunk_extra can't be greater than
   * nb_thread */
  const int nb_chunk_extra =
      (int)((trip_count / loop->chunk_size) % num_threads);
  const int are_not_multiple = (trip_count % loop->chunk_size) ? 1 : 0;

  /* The first threads will have one more chunk (according to the previous
   * approximation) */
  nb_chunks_per_thread += (rank < nb_chunk_extra) ? (long)1 : (long)0;

  /* One thread may have one more additionnal smaller chunk to finish the
   * iteration domain */
  nb_chunks_per_thread +=
      (rank == nb_chunk_extra && are_not_multiple) ? (long)1 : (long)0;

  return nb_chunks_per_thread;
}

void __mpcomp_get_specific_chunk_per_rank(int rank, int num_threads, long lb,
                                          long b, long incr, long chunk_size,
                                          long chunk_num, long *from,
                                          long *to) {
  not_implemented();
}

/****
 *
 * ULL CHUNK MANIPULATION
 *
 *
 *****/
/* Return the number of chunks that a static schedule will create for the thread 'rank' */
unsigned long long
__mpcomp_compute_static_nb_chunks_per_rank_ull(unsigned long long rank,
                                               unsigned long long num_threads,
                                               mpcomp_loop_ull_iter_t *loop) {
  unsigned long long nb_chunks_per_thread, chunk_size;
  const unsigned long long trip_count =
      __mpcomp_internal_loop_get_num_iters_ull(loop->lb, loop->b, loop->incr,
                                               loop->up);

  chunk_size = (loop->chunk_size) ? loop->chunk_size : trip_count / num_threads;
  chunk_size = (chunk_size) ? chunk_size : (unsigned long long)1;
  loop->chunk_size = chunk_size;

  /* Compute the number of chunks per thread (floor value) */
  nb_chunks_per_thread = trip_count / (chunk_size * num_threads);
  /* Compute the number of extrat chunk nb_chunk_extra can't be greater than
   * nb_thread */
  const int nb_chunk_extra = (int)((trip_count / chunk_size) % num_threads);
  const int are_not_multiple = (trip_count % chunk_size) ? 1 : 0;

  /* The first threads will have one more chunk (according to the previous
   * approximation) */
  nb_chunks_per_thread +=
      (rank < nb_chunk_extra) ? (unsigned long long)1 : (unsigned long long)0;

  /* One thread may have one more additionnal smaller chunk to finish the
   * iteration domain */
  nb_chunks_per_thread += (rank == nb_chunk_extra && are_not_multiple)
                              ? (unsigned long long)1
                              : (unsigned long long)0;

  return nb_chunks_per_thread;
}

unsigned long long __mpcomp_get_static_nb_chunks_per_rank_ull(
    int rank, int nb_threads, unsigned long long lb, unsigned long long b,
    unsigned long long incr, unsigned long long chunk_size) {
  return 0;
}

#if 0
void
__mpcomp_get_specific_chunk_per_rank_ull (unsigned long rank, unsigned long nb_threads,
                      unsigned long long lb, unsigned long long b, unsigned long long incr,
                      unsigned long long chunk_size, unsigned long long chunk_num,
                      unsigned long long *from, unsigned long long *to)
{
    unsigned long long local_from ;
    unsigned long long local_to ;
    int incr_sign = (lb + incr > lb) ? 1 : -1;
    /* Compute the trip count (total number of iterations of the original loop) */
    long long trip_count;
    long long incr_signed = (long long)((lb + incr) - lb);
    incr_signed = (incr_signed > 0) ? incr_signed : -incr_signed;
    if(incr_sign > 0)
    {
        trip_count = ((long long)(b - lb)) / incr_signed;
        if((long long)(b - lb) % incr_signed != 0)
            trip_count++;
    }
    else
    {
        trip_count = ((long long)(lb - b)) / incr_signed;
        if((long long)(lb - b) % incr_signed != 0)
            trip_count++;
    }

    trip_count = (trip_count >= 0) ? trip_count : -trip_count;
    
    /* The final additionnal chunk is smaller, so its computation is a little bit
    different */
    if (rank == (trip_count / chunk_size) % nb_threads
        && trip_count % chunk_size != 0
        && chunk_num == __mpcomp_get_static_nb_chunks_per_rank_ull (rank,
                                                            nb_threads, lb,
                                                            b, incr,
                                                            chunk_size) - 1)
    {
        local_from = lb + (trip_count / chunk_size) * chunk_size * incr;
        local_to = lb + trip_count * incr;

        sctk_nodebug ("__mpcomp_static_schedule_get_specific_chunk: "
            "Thread %d: %ld -> %ld (excl) step %ld => "
            "%ld -> %ld (excl) step %ld (chunk of %ld)\n",
            rank, lb, b, incr, local_from, local_to, incr,
			trip_count % chunk_size);

    }
    else
    {
        local_from = lb + chunk_num * nb_threads * chunk_size * incr +
                     chunk_size * incr * rank;

        local_to   = lb + chunk_num * nb_threads * chunk_size * incr +
                     chunk_size * incr * rank + chunk_size * incr;

        sctk_nodebug ("__mpcomp_static_schedule_get_specific_chunk: "
            "Thread %d / Chunk %ld: %ld -> %ld (excl) step %ld => "
            "%ld -> %ld (excl) step %ld (chunk of %ld)\n",
            rank, chunk_num, lb, b, incr, local_from, local_to, incr,
            chunk_size);
    }
    *from = local_from ;
    *to = local_to ;
}
#endif
