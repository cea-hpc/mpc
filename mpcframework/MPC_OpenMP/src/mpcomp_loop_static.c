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
#include "sctk.h"
#include <mpcomp.h>
#include <mpcomp_abi.h>
#include "mpcmicrothread_internal.h"
#include <sctk_debug.h>

#include "mpcomp_core.h"
#include "mpcomp_loop.h"
#include "mpcomp_openmp_tls.h"

/****
 *
 * CHUNK MANIPULATION
 *
 *
 *****/
/* Compute the chunk for a static schedule (without specific chunk size) */
int __mpcomp_static_schedule_get_single_chunk(long lb, long b, long incr,
                                              long *from, long *to) {
  /*
    Original loop -> lb -> b step incr
    New loop -> *from -> *to step incr
  */

  int trip_count, chunk_size;

  /* Grab info on the current thread */
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int num_threads = t->info.num_threads;
  const int rank = t->rank;

  trip_count = (b - lb) / incr;
  if ((b - lb) % incr != 0)
    trip_count++;

  if (trip_count <= t->rank) {
    sctk_nodebug("____mpcomp_static_schedule_get_single_chunk: "
                 "#%d (%d -> %d step %d) -> NO CHUNK",
                 rank, lb, b, incr);
    return 0;
  }

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

  sctk_assert((incr > 0) ? (*to - incr <= b) : (*to - incr >= b));
  sctk_nodebug("____mpcomp_static_schedule_get_single_chunk: "
               "#%d (%d -> %d step %d) -> (%d -> %d step %d)",
               rank, lb, b, incr, *from, *to, incr);

  return 1;
}

/* Return the number of chunks that a static schedule would create */
int __mpcomp_static_schedule_get_nb_chunks (long lb, long b, long incr,
					    long chunk_size)
{
     /* Original loop: lb -> b step incr */
     
     long trip_count;
     long nb_chunks_per_thread;

     mpcomp_thread_t *t = mpcomp_get_thread_tls();

     /* Retrieve the number of threads and the rank of the current thread */
     const int nb_threads = t->info.num_threads;
     const int rank = t->rank;

     /* Compute the trip count (total number of iterations of the original loop) */
     trip_count = (b - lb) / incr;
     if((b - lb) % incr != 0)
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

     sctk_nodebug("____mpcomp_static_schedule_get_nb_chunks[%d]: %ld -> [%ld] "
                  "(cs=%ld) final nb_chunks = %ld",
                  rank, lb, b, incr, chunk_size, nb_chunks_per_thread);

     return nb_chunks_per_thread;
}

/* Return the chunk #'chunk_num' assuming a static schedule with 'chunk_size'
 * as a chunk size */
void __mpcomp_static_schedule_get_specific_chunk(long rank, long num_threads,
                                                 mpcomp_loop_long_iter_t *loop,
                                                 long chunk_num, long *from,
                                                 long *to) {
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  /* Compute the number of chunks per thread (floor value) */
  const long thread_chunk_num =
      __mpcomp_get_static_nb_chunks_per_rank(rank, num_threads, loop);
  const long trip_count =
      __mpcomp_internal_loop_get_num_iters(loop->lb, loop->b, loop->incr);
  const long decal = chunk_num * num_threads * loop->chunk_size * loop->incr;

  *from = loop->lb + decal + loop->chunk_size * loop->incr * rank;

  if (rank == (trip_count / loop->chunk_size) % num_threads &&
      trip_count % loop->chunk_size != 0 && chunk_num == thread_chunk_num - 1) {
    *to = loop->b;
  } else {
    *to = *from + loop->chunk_size * loop->incr;
  }
  sctk_nodebug("[%d - %d] %s: from: %ld  %ld - to: %ld %ld  - %ld - %ld",
               t->rank, rank, __func__, *from, loop->lb, loop->b, *to,
               loop->incr, chunk_num);
}

/****
 *
 * LONG VERSION
 *
 *
 *****/

void __mpcomp_static_loop_init(mpcomp_thread_t *t, long lb, long b, long incr,
                               long chunk_size) {
  mpcomp_loop_gen_info_t *loop_infos;

  /* Get the team info */
  sctk_assert(t->instance != NULL);

  t->schedule_type =
      (t->schedule_is_forced) ? t->schedule_type : MPCOMP_COMBINED_STATIC_LOOP;
  t->schedule_is_forced = 0;

  loop_infos = &(t->info.loop_infos);
  __mpcomp_loop_gen_infos_init(loop_infos, lb, b, incr, chunk_size);

  /*  As the loop_next function consider a chunk as already been realised
      we need to initialize to 0 minus 1 */
  t->static_current_chunk = -1;

  if (!(t->info.loop_infos.ischunked)) {
    t->static_nb_chunks = 1;
    return;
  }

  /* Compute the number of chunk for this thread */
  t->static_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank(
      t->rank, t->info.num_threads, &(loop_infos->loop.mpcomp_long));
  sctk_nodebug("[%d] %s: Got %d chunk(s)", t->rank, __func__,
               t->static_nb_chunks);
}

int __mpcomp_static_loop_begin (long lb, long b, long incr, long chunk_size,
				long *from, long *to)
{
	__mpcomp_init ();

        mpcomp_thread_t *t = mpcomp_get_thread_tls();

        sctk_nodebug("[%d] %s: %d -> %d [%d] cs:%d", t->rank, __func__, lb, b,
                     incr, chunk_size);

        /* Initialization of loop internals */
        __mpcomp_static_loop_init(t, lb, b, incr, chunk_size);

        if (!from && !to) /* Disable next in begin */
        {
          sctk_nodebug("No next call in begin ...");
          return -1;
        }

        return __mpcomp_static_loop_next(from, to);
}

int __mpcomp_static_loop_next (long *from, long *to)
{
  mpcomp_thread_t *t = mpcomp_get_thread_tls();
  mpcomp_loop_gen_info_t *loop_infos = &(t->info.loop_infos);

  const long rank = (long)t->rank;
  const long nb_threads = (long)t->info.num_threads;

  /* Next chunk */
  t->static_current_chunk++;

  sctk_nodebug("[%d] %s: checking if current_chunk %d >= nb_chunks %d?",
               t->rank, t->static_current_chunk, t->static_nb_chunks);

  /* Check if there is still a chunk to execute */
  if (t->static_current_chunk >= t->static_nb_chunks) {
    return 0;
  }

  if (!(loop_infos->ischunked)) {
    mpcomp_loop_long_iter_t *loop = &(loop_infos->loop.mpcomp_long);
    return __mpcomp_static_schedule_get_single_chunk(loop->lb, loop->b,
                                                     loop->incr, from, to);
  }

  __mpcomp_static_schedule_get_specific_chunk(
      rank, nb_threads, &(loop_infos->loop.mpcomp_long),
      t->static_current_chunk, from, to);
  sctk_nodebug("[%d] ____mpcomp_static_loop_next: got a chunk %d -> %d",
               t->rank, *from, *to);
  return 1;
}

void __mpcomp_static_loop_end ()
{
     __mpcomp_barrier();
     // sctk_nodebug( "end barrier" );
}

void __mpcomp_static_loop_end_nowait ()
{
     /* Nothing to do */
}


/****
 *
 * LONG ORDERED VERSION 
 *
 *
 *****/
int __mpcomp_ordered_static_loop_begin(long lb, long b, long incr,
                                       long chunk_size, long *from, long *to) {
  __mpcomp_init();

  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int ret = __mpcomp_static_loop_begin(lb, b, incr, chunk_size, from, to);
  sctk_nodebug("%d %s: lb : %ld b: %ld", t->rank, __func__, lb, b);
  if (!from) {
    t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = lb;
    return -1;
  }
  t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
  return ret;
}

int __mpcomp_ordered_static_loop_next(long *from, long *to)
{
  mpcomp_thread_t *t = mpcomp_get_thread_tls();

  const int ret = __mpcomp_static_loop_next(from, to);
  t->info.loop_infos.loop.mpcomp_long.cur_ordered_iter = *from;
  return ret;
}

void __mpcomp_ordered_static_loop_end()
{
     __mpcomp_static_loop_end();
}

void __mpcomp_ordered_static_loop_end_nowait()
{
     __mpcomp_static_loop_end();
}

