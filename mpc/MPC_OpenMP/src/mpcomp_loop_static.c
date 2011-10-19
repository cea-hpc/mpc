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
#include "mpcomp_internal.h"
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
  mpcomp_thread_info_t *info;
  int rank;

  /* TODO use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

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
  mpcomp_thread_info_t *info;
  int rank;

  /* Original loop: lb -> b step incr */

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

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
  mpcomp_thread_info_t *info;
  int trip_count;
  int nb_threads;
  int rank;

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

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
  mpcomp_thread_info_t *info;

  __mpcomp_init ();

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

  /* Automatic chunk size -> at most one chunk */
  if (chunk_size == -1) {
      info->static_nb_chunks = 1 ;
      __mpcomp_static_schedule_get_single_chunk (lb, b, incr, from, to);
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
  mpcomp_thread_info_t *info;
  int nb_threads;
  int rank;

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

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

void
__mpcomp_start_parallel_static_loop (int arg_num_threads, void *(*func)
				     (void *), void *shared, int lb, int b,
				     int incr, int chunk_size)
{
  mpcomp_thread_info_t *current_info;
  int num_threads;
  int n;			/* Number of iterations */

  SCTK_PROFIL_START (__mpcomp_start_parallel_region);

  /* Compute the total number iterations */
  n = (b - lb) / incr;

  /* If this loop contains no iterations then exit */
  if ( n <= 0 ) {
    return ;
  }

  /* Initialize the OpenMP environment (call several times, but really executed
   * once) */
  __mpcomp_init ();

  /* Retrieve the information (microthread structure and current region) */
  /* TODO Use TLS if available */
  current_info = sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (current_info != NULL);

  sctk_nodebug
    ("__mpcomp_start_parallel_static_loop: "
     "Entering w/ loop %d -> %d step %d [cs=%d]",
     lb, b, incr, chunk_size);

  /* Grab the number of threads */
  num_threads = current_info->icvs.nthreads_var;
  if (arg_num_threads > 0 && arg_num_threads <= MPCOMP_MAX_THREADS)
    {
      num_threads = arg_num_threads;
    }

  /* Bypass if the parallel region contains only 1 thread */
  if (num_threads == 1)
    {
      sctk_nodebug
	("__mpcomp_start_parallel_static_loop: Only 1 thread -> call f");

      current_info->loop_lb = lb ;
      current_info->loop_b = b ;
      current_info->loop_incr = incr ;
      current_info->loop_chunk_size = chunk_size ;

      current_info->static_current_chunk = -1 ;
      current_info->static_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank( 0, num_threads, lb, b, incr, chunk_size ) ;

      func (shared);
      SCTK_PROFIL_END (__mpcomp_start_parallel_region);
      return;
    }

  sctk_nodebug
    ("__mpcomp_start_parallel_static_loop: -> Final num threads = %d",
     num_threads);

  /* Creation of a new microthread structure if the current region is
   * sequential or if nesting is allowed */
  if (current_info->depth == 0 || mpcomp_get_nested ())
    {
      sctk_microthread_t *new_task;
      sctk_microthread_t *current_task;
      int i;
      /*
      int n = num_threads / current_info->icvs.nmicrovps_var;
      int index = num_threads % current_info->icvs.nmicrovps_var;
      int vp;
      */
      SCTK_PROFIL_START (__mpcomp_start_parallel_region__creation);

      sctk_nodebug
	("__mpcomp_start_parallel_static_loop: starting new team at depth %d on %d microVP(s)",
	 current_info->depth, current_info->icvs.nmicrovps_var);

      current_task = current_info->task;

      /* Initialize a new microthread structure if needed */
      if (current_info->depth == 0)
	{
	  new_task = current_task;

	  sctk_nodebug
	    ("__mpcomp_start_parallel_static_loop: reusing the thread_info depth 0");
	}
      else
	{

	  /* If the first child has already been created, get the
	   * corresponding thread-info structure */
	  if (current_info->children[0] != NULL)
	    {
	      sctk_nodebug
		("__mpcomp_start_parallel_static_loop: reusing thread_info new depth");

	      new_task = current_info->children[0]->task;
	      sctk_assert (new_task != NULL);

	    }
	  else
	    {

	      sctk_nodebug
		("__mpcomp_start_parallel_static_loop: allocating thread_info new depth");

	      new_task = sctk_malloc (sizeof (sctk_microthread_t));
	      sctk_assert (new_task != NULL);
	      sctk_microthread_init (current_info->icvs.nmicrovps_var, new_task);
	    }
	}


      /* Fill the microthread structure with these new threads */
      for (i = num_threads - 1; i >= 0; i--)
	{
	  mpcomp_thread_info_t *new_info;
	  int microVP;
	  int val;
	  int res;

	  /* Compute the VP this thread will be scheduled on and the behavior of
	   * 'add_task' */
	  if (i < current_info->icvs.nmicrovps_var)
	    {
	      microVP = i;
	      val = MPC_MICROTHREAD_LAST_TASK;
	    }
	  else
	    {
	      microVP = i % (current_info->icvs.nmicrovps_var);
	      val = MPC_MICROTHREAD_NO_TASK_INFO;
	    }

	  /* Get the structure of the i-th children */
	  new_info = current_info->children[i];

	  /* If this is the first time that such a child exists
	     -> allocate memory once and initialize once */
	  if (new_info == NULL)
	    {

	      sctk_nodebug
		("__mpcomp_start_parallel_static_loop: "
		 "Child %d is NULL -> allocating thread_info",
		 i);

	      new_info =
		sctk_malloc_on_node (sizeof (mpcomp_thread_info_t), sctk_get_node_from_cpu(microVP));
	      sctk_assert (new_info != NULL);

	      current_info->children[i] = new_info;

	      __mpcomp_init_thread_info (new_info, func, shared, i,
					 num_threads, current_info->icvs,
					 current_info->depth + 1, microVP, 0,
					 current_info, 0, new_task);
	    }
	  else
	    {
	      __mpcomp_reset_thread_info (new_info, func, shared, num_threads,
					  current_info->icvs, 0, 0, microVP);
	    }


	  new_info->task = new_task;

	  /* Update private information about dynamic schedule */
	  new_info->loop_lb = lb ;
	  new_info->loop_b = b ;
	  new_info->loop_incr = incr ;
	  new_info->loop_chunk_size = chunk_size ;
	  new_info->static_current_chunk = -1 ;
	  new_info->static_nb_chunks = __mpcomp_get_static_nb_chunks_per_rank( i, num_threads, lb, b, incr, chunk_size ) ;

	  sctk_nodebug
	    ("__mpcomp_start_parallel_static_loop: "
	     "Adding op %d on microVP %d", i, microVP);

	  res = sctk_microthread_add_task (__mpcomp_wrapper_op, new_info, microVP,
					   &(new_info->step), new_task, val);
	  sctk_assert (res == 0);


	}

      SCTK_PROFIL_END (__mpcomp_start_parallel_region__creation);

      /* Launch the execution of this microthread structure */
      sctk_microthread_parallel_exec (new_task,
				      MPC_MICROTHREAD_DONT_WAKE_VPS);

      sctk_nodebug
	("__mpcomp_start_parallel_static_loop: Microthread execution done");

      /* Restore the key value (microthread structure & OpenMP info) */
      sctk_thread_setspecific (mpcomp_thread_info_key, current_info);
      sctk_thread_setspecific (sctk_microthread_key, current_task);

      /* Free the memory allocated for the new microthread structure */
      if (current_info->depth != 0)
	{
	  sctk_free (new_task);
	}

      /* TODO free the memory allocated for the OpenMP-thread info 
         maybe not because this is stored in current_info->children[] */


    }
  else
    {
      mpcomp_thread_info_t *new_info;

      sctk_nodebug
	("__mpcomp_start_parallel_static_loop: Serialize a new team at depth %d",
	 current_info->depth);

      /* TODO only flatterned nested supported for now */

      num_threads = 1;


      new_info = current_info->children[0];
      if (new_info == NULL)
	{

	  sctk_nodebug
	    ("__mpcomp_start_parallel_static_loop: Allocating new thread_info");

	  new_info = sctk_malloc (sizeof (mpcomp_thread_info_t));
	  sctk_assert (new_info != NULL);
	  current_info->children[0] = new_info;
	  __mpcomp_init_thread_info (new_info, func, shared,
				     0, 1,
				     current_info->icvs,
				     current_info->depth + 1,
				     current_info->vp, 0,
				     current_info, 0, current_info->task);
	}
      else
	{
	  sctk_nodebug
	    ("__mpcomp_start_parallel_static_loop: Reusing older thread_info");
	  __mpcomp_reset_thread_info (new_info, func, shared, 0,
				      current_info->icvs, 0, 0,
				      current_info->vp);
	}

      __mpcomp_wrapper_op (new_info);

      sctk_nodebug ("__mpcomp_start_parallel_dynamic_loop: after flat nested");

      sctk_thread_setspecific (mpcomp_thread_info_key, current_info);

      sctk_free (new_info);
    }

  /* No need to re-initialize team info for this thread */


  /* Restore the TLS for the main thread */
  sctk_hierarchical_tls = current_info->children[0]->hierarchical_tls;

  SCTK_PROFIL_END (__mpcomp_start_parallel_region);
}

int
__mpcomp_static_loop_next_ignore_nowait (int *from, int *to)
{
  not_implemented ();
  return 0;
}


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
  mpcomp_thread_info_t *info;
  int res ;

  res = __mpcomp_static_loop_begin(lb, b, incr, chunk_size, from, to ) ;

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

  info->current_ordered_iteration = *from ;

  return res ;
}

int
__mpcomp_ordered_static_loop_next(int *from, int *to)
{
  mpcomp_thread_info_t *info;
  int res ;

  res = __mpcomp_static_loop_next(from, to) ;

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

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
