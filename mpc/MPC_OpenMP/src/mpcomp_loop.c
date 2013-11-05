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
#include "mpcomp_internal.h"
#include <sctk_debug.h>

/* Return the number of chunks that a static schedule will create for the thread 'rank' */
int
__mpcomp_get_static_nb_chunks_per_rank (int rank, int nb_threads, long lb,
					long b, long incr, long chunk_size)
{
  long trip_count;
  long nb_chunks_per_thread;

  /* Original loop: lb -> b step incr */

  /* Compute the trip count (total number of iterations of the original loop) */
  trip_count = (b - lb) / incr;

  if ( (b - lb) % incr != 0 ) {
    trip_count++ ;
  }

  if ( trip_count <= 0 ) {
    return 0 ;
  }

  /* Compute the number of chunks per thread (floor value) */
  nb_chunks_per_thread = trip_count / (chunk_size * nb_threads);

  /* The first threads will have one more chunk (according to the previous
     approximation) */
  if (rank < (trip_count / chunk_size) % nb_threads)
    {
      nb_chunks_per_thread++;
    }

  /* One thread may have one more additionnal smaller chunk to finish the
     iteration domain */
  if (rank == (trip_count / chunk_size) % nb_threads
      && trip_count % chunk_size != 0)
    {
      nb_chunks_per_thread++;
    }

  sctk_nodebug
    ("__mpcomp_get_static_nb_chunks: final nb_chunks_per_thread = %ld",
     nb_chunks_per_thread);


  return nb_chunks_per_thread;
}

void
__mpcomp_get_specific_chunk_per_rank (int rank, int nb_threads,
				      long lb, long b, long incr,
				      long chunk_size, long chunk_num,
				      long *from, long *to)
{
  long trip_count;
  long local_from ;
  long local_to ;

  /* Compute the trip count (total number of iterations of the original loop) */
  trip_count = (b - lb) / incr;

  if ( (b - lb) % incr != 0 ) {
    trip_count++ ;
  }

  sctk_nodebug("__mpcomp_get_specific_chunk_per_rank: trip_count=%ld", 
			trip_count);

  /* The final additionnal chunk is smaller, so its computation is a little bit
     different */
  if (rank == (trip_count / chunk_size) % nb_threads
      && trip_count % chunk_size != 0
      && chunk_num == __mpcomp_get_static_nb_chunks_per_rank (rank,
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
      local_from =
	lb + chunk_num * nb_threads * chunk_size * incr +
	chunk_size * incr * rank;
      local_to =
	lb + chunk_num * nb_threads * chunk_size * incr +
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
