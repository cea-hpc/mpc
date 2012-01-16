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
void
__mpcomp_static_schedule_get_single_chunk (int lb, int b, int incr, int *from,
					   int *to)
{
  /*
     Original loop -> lb -> b step incr
     New loop -> *from -> *to step incr
   */

  int trip_count;
  int chunk_size;
  int num_threads;
  mpcomp_thread_t *info;
  int rank;

  info = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(info != NULL);      


  num_threads = info->num_threads;
  rank = info->rank;

  trip_count = (b - lb) / incr;

  if ( (b - lb) % incr != 0 ) {
    trip_count++ ;
  }

  chunk_size = trip_count / num_threads;

  if (rank < (trip_count % num_threads))
    {
      /* The first part is homogeneous with a chunk size a little bit larger */
      *from = lb + rank * (chunk_size + 1) * incr;
      *to = lb + (rank + 1) * (chunk_size + 1) * incr;
    }
  else
    {
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
int
__mpcomp_static_schedule_get_nb_chunks (int lb, int b, int incr,
					int chunk_size)
{
  int trip_count;
  int nb_chunks_per_thread;
  int nb_threads;
  mpcomp_thread_t *info;
  int rank;

  /* Original loop: lb -> b step incr */

  info = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(info != NULL);   

  /* Retrieve the number of threads and the rank of the current thread */
  nb_threads = info->num_threads;
  rank = info->rank;

  /* Compute the trip count (total number of iterations of the original loop) */
  trip_count = (b - lb) / incr;

  if ( (b - lb) % incr != 0 ) {
    trip_count++ ;
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
    ("__mpcomp_static_schedule_get_nb_chunks[%d]: %d -> [%d] (cs=%d) final nb_chunks = %d",
     rank, lb, b, incr, chunk_size, nb_chunks_per_thread);


  return nb_chunks_per_thread;
}

/* Return the chunk #'chunk_num' assuming a static schedule with 'chunk_size'
 * as a chunk size */
void
__mpcomp_static_schedule_get_specific_chunk (int lb, int b, int incr,
					     int chunk_size, int chunk_num,
					     int *from, int *to)
{
  mpcomp_thread_t *info;
  int trip_count;
  int nb_threads;
  int rank;


  info = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(info != NULL);  

  /* Retrieve the number of threads and the rank of this thread */
  nb_threads = info->num_threads;
  rank = info->rank;

  /* Compute the trip count (total number of iterations of the original loop) */
  trip_count = (b - lb) / incr;

  if ( (b - lb) % incr != 0 ) {
    trip_count++ ;
  }


  /* The final additionnal chunk is smaller, so its computation is a little bit
     different */
  if (rank == (trip_count / chunk_size) % nb_threads
      && trip_count % chunk_size != 0
      && chunk_num == __mpcomp_static_schedule_get_nb_chunks (lb, b, incr,
							      chunk_size) - 1)
    {

      //int last_chunk_size = trip_count % chunk_size;

      *from = lb + (trip_count / chunk_size) * chunk_size * incr;
      *to = lb + trip_count * incr;

      sctk_nodebug ("__mpcomp_static_schedule_get_specific_chunk: "
		    "Thread %d / Chunk %d: %d -> %d (excl) step %d => "
		    "%d -> %d (excl) step %d (chunk of %d)\n",
		    rank, nb_chunks_per_thread, lb, b, incr, from, to, incr,
		    last_chunk_size);

    }
  else
    {
      *from =
	lb + chunk_num * nb_threads * chunk_size * incr +
	chunk_size * incr * rank;
      *to =
	lb + chunk_num * nb_threads * chunk_size * incr +
	chunk_size * incr * rank + chunk_size * incr;

      sctk_nodebug ("__mpcomp_static_schedule_get_specific_chunk: "
		    "Thread %d / Chunk %d: %d -> %d (excl) step %d => "
		    "%d -> %d (excl) step %d (chunk of %d)\n",
		    rank, chunk_num, lb, b, incr, *from, *to, incr,
		    chunk_size);
    }
}

int
__mpcomp_static_loop_begin (int lb, int b, int incr, int chunk_size,
			    int *from, int *to)
{
  mpcomp_thread_t *info;

  __mpcomp_init ();

  info = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(info != NULL);  



  /* Automatic chunk size -> at most one chunk */
  if (chunk_size == 0) {
      info->static_nb_chunks = 1 ;
      __mpcomp_static_schedule_get_single_chunk (lb, b, incr, from, to);

      info->loop_lb = lb ;
      info->loop_b = b ;
      info->loop_incr = incr ;
      info->loop_chunk_size = 1 ;
    }
  else {
    int nb_threads;
    int rank;


    /* Retrieve the number of threads and the rank of this thread */
    nb_threads = info->num_threads;
    rank = info->rank;

    info->static_nb_chunks = 
      __mpcomp_get_static_nb_chunks_per_rank( rank, nb_threads, lb, b, incr, chunk_size ) ;

    /* No chunk -> exit the 'for' loop */
    if ( info->static_nb_chunks <= 0 ) {
      return 0 ;
    }

    info->loop_lb = lb ;
    info->loop_b = b ;
    info->loop_incr = incr ;
    info->loop_chunk_size = chunk_size ;

    info->static_current_chunk = 0 ;
    __mpcomp_get_specific_chunk_per_rank( rank, nb_threads, lb, b, incr, chunk_size, 0, from, to ) ;

  }
  return 1;
}

int
__mpcomp_static_loop_next (int *from, int *to)
{
  mpcomp_thread_t *info;
  int nb_threads;
  int rank;

  info = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(info != NULL);  


  /* Retrieve the number of threads and the rank of this thread */
  nb_threads = info->num_threads;
  rank = info->rank;

  /* Next chunk */
  info->static_current_chunk++ ;

  /* Check if there is still a chunk to execute */
  if ( info->static_current_chunk >= info->static_nb_chunks ) {
    return 0 ;
  }

  __mpcomp_get_specific_chunk_per_rank( rank, nb_threads, info->loop_lb,
      info->loop_b, info->loop_incr, info->loop_chunk_size,
      info->static_current_chunk, from, to ) ;

  return 1;
}

void
__mpcomp_static_loop_end ()
{
  __mpcomp_barrier ();
}

void
__mpcomp_static_loop_end_nowait ()
{
  /* Nothing to do */
}

#if defined (SCTK_USE_OPTIMIZED_TLS)
  sctk_tls_module = current_info->children[0]->tls_module;
  sctk_context_restore_tls_module_vp ();
#endif

/****
  *
  * ORDERED VERSION 
  *
  *
  *****/

int
__mpcomp_ordered_static_loop_begin (int lb, int b, int incr, int chunk_size,
			    int *from, int *to)
{
  mpcomp_thread_t *info;
  int res ;

  res = __mpcomp_static_loop_begin(lb, b, incr, chunk_size, from, to ) ;

  info = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(info != NULL);  

  info->current_ordered_iteration = *from ;

  return res ;
}

int
__mpcomp_ordered_static_loop_next(int *from, int *to)
{
  mpcomp_thread_t *info;
  int res ;

  res = __mpcomp_static_loop_next(from, to) ;

  info = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(info != NULL);  

  info->current_ordered_iteration = *from ;

  return res ;
}

void
__mpcomp_ordered_static_loop_end()
{
  __mpcomp_static_loop_end() ;
}

void
__mpcomp_ordered_static_loop_end_nowait()
{
  __mpcomp_static_loop_end() ;
}

