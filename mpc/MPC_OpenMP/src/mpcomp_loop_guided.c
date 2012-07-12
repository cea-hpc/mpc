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

/* 
   This file includes the function related to the 'guided' schedule of a shared
   for loop.
 */


int
__mpcomp_guided_loop_begin (int lb, int b, int incr, int chunk_size,
			    int *from, int *to)
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;
  int index;
  int num_threads;
  int nb_entered_threads;
  int n;			/* Number of remaining iterations */
  int cs;			/* Current chunk size */


  /* Compute the total number iterations */
  n = (b - lb) / incr;

  /* If this loop contains no iterations then exit */
  if ( n <= 0 ) {
    return 0 ;
  }

  /* Grab the thread info */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or
     this team has only 1 thread, the current thread will execute the whole
     loop */
  if (num_threads == 1)
    {
      *from = lb;
      *to = b;
      return 1;
    }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;
  index =
    (father->current_for_guided[rank] + 1) % (MPCOMP_MAX_ALIVE_FOR_GUIDED +
					      1);

  sctk_nodebug
    ("__mpcomp_guided_loop_begin[%d]: Entering loop with index %d, vp=%d",
     rank, index, self->vp);

  sctk_spinlock_lock (&(father->lock_enter_for_guided[index]));

  nb_entered_threads = father->nb_threads_entered_for_guided[index];

  /* MPCOMP_NOWAIT_STOP_SYMBOL in the next workshare structure means that the
   * buffer is full (too many alive for loops with guided schedules).
   * Therefore, the threads of this team have to wait. */
  if (nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL)
    {

      sctk_nodebug
	("__mpcomp_guided_loop_begin[%d]: First barrier at %d (max %d)", rank,
	 index, MPCOMP_MAX_ALIVE_FOR_GUIDED);

      father->nb_threads_entered_for_guided[index] =
	MPCOMP_NOWAIT_STOP_CONSUMED;

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

      /* Generate a barrier (it should reinitialize all counters) */
      /* FIXME use good barrier instead of OLD! */
      __mpcomp_old_barrier ();

      return __mpcomp_guided_loop_begin (lb, b, incr, chunk_size, from, to);
    }

  /* MPCOMP_NOWAIT_STOP_CONSUMED => some else is already blocked here, so just
   * call the barrier */
  if (nb_entered_threads == MPCOMP_NOWAIT_STOP_CONSUMED)
    {
      sctk_nodebug ("__mpcomp_guided_loop_begin[%d]: Barrier at %d (max %d)",
		    rank, index, MPCOMP_MAX_ALIVE_FOR_GUIDED);

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

      /* Generate a barrier (it should reinitialize all counters) */
      /* FIXME use good barrier instead of OLD! */
      __mpcomp_old_barrier ();

      return __mpcomp_guided_loop_begin (lb, b, incr, chunk_size, from, to);
    }

  /* I can start this loop */

  /* Am I the first one? */
  if (nb_entered_threads == 0)
    {

      /* I am the first to enter this loop */
      sctk_nodebug
	("__mpcomp_guided_loop_begin[%d]: First one => index %d, #threads %d",
	 rank, index, num_threads);

      /* Update the current information */
      father->nb_threads_entered_for_guided[index] = 1;
      father->current_for_guided[rank] = index;

      /* Initialize some information for the rest of the team */
      father->nb_threads_exited_for_guided[index] = 0;


      /* Compute the current chunk size */
      cs = n / num_threads;
      if (cs < n && (n % num_threads) != 0)
	{
	  cs++;
	}

      sctk_nodebug
	("__mpcomp_guided_loop_begin[%d]: First one => n=%d->%d cs=%d", rank,
	 n, n - cs, cs);

      /* Update the number of remaining iterations */
      n = n - cs;


      father->nb_iterations_remaining[index] = n;
      father->current_from_for_guided[index] = lb + cs * incr;

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

      /* Fill private information about the loop */
      self->loop_lb = lb;
      self->loop_b = b;
      self->loop_incr = incr;
      self->loop_chunk_size = chunk_size;

      /* Return the corresponding chunk */
      *from = lb;
      *to = lb + cs * incr;

      return 1;

    }

  sctk_assert (nb_entered_threads < num_threads);

  /* I am not the first one to enter
     -> check if there is still a chunk to execute */
  sctk_nodebug
    ("__mpcomp_guided_loop_begin[%d]: Not first one -> index %d, #threads %d",
     rank, index, num_threads);

  /* Update the current information */
  father->nb_threads_entered_for_guided[index] = nb_entered_threads + 1;
  father->current_for_guided[rank] = index;

  n = father->nb_iterations_remaining[index];

  sctk_nodebug
    ("__mpcomp_guided_loop_begin[%d]: Not first one -> %d iteration(s) remaining",
     rank, n);

  /* If there is a chunk remaining */
  if (n > 0)
    {
      int next_from;

      /* Compute the next chunk size */
      cs = n / num_threads;

      /* Ceil value */
      if (cs < n && (n % num_threads) != 0)
	{
	  cs++;
	}

      /* This size should be at least equal to the 'chunk_size' value */
      /* Except for the last chunk */
      if (cs < self->loop_chunk_size)
	{
	  cs = self->loop_chunk_size;
	  if (n < self->loop_chunk_size)
	    {
	      cs = n;
	    }
	}

      n = n - cs;

      /* Grab the 'from' info */
      next_from = father->current_from_for_guided[index];

      /* Update info for the next chunk */
      father->nb_iterations_remaining[index] = n;
      father->current_from_for_guided[index] = next_from + cs * incr;

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

      /* Fill private information about the loop */
      self->loop_lb = lb;
      self->loop_b = b;
      self->loop_incr = incr;
      self->loop_chunk_size = chunk_size;

      /* Return the corresponding chunk */
      *from = next_from;
      *to = next_from + cs * incr;

      return 1;
    }

  sctk_nodebug
    ("__mpcomp_guided_loop_begin[%d]: Not first one -> no chunk left", rank);

  /* No chunk left... just unlock and return 0 */

  sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

  return 0;
}

int
__mpcomp_guided_loop_next (int *from, int *to)
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;
  int index;
  int num_threads;
  int n;


  /* Grab the thread info */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or
   * this team has only 1 thread, the current thread has already executed the
   * whole loop 
   */
  if (num_threads == 1)
    {
      return 0;
    }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;
  index = father->current_for_guided[rank];

  sctk_spinlock_lock (&(father->lock_enter_for_guided[index]));

  n = father->nb_iterations_remaining[index];

  sctk_nodebug
    ("__mpcomp_guided_loop_next[%d]: Next => %d iteration(s) remaining", rank,
     n);

  /* If there is a chunk remaining */
  if (n > 0)
    {
      int next_from;
      int cs;

      /* Compute the next chunk size */
      cs = n / num_threads;

      /* Ceil value */
      if (cs < n && (n % num_threads) != 0)
	{
	  cs++;
	}

      /* This size should be at least equal to the 'chunk_size' value */
      /* Except for the last chunk */
      if (cs < self->loop_chunk_size)
	{
	  cs = self->loop_chunk_size;
	  if (n < self->loop_chunk_size)
	    {
	      cs = n;
	    }
	}

      sctk_nodebug ("__mpcomp_guided_loop_next[%d]: Next => n=%d->%d cs=%d",
		    rank, n, n - cs, cs);

      n = n - cs;

      /* Grab the 'from' info */
      next_from = father->current_from_for_guided[index];

      /* Update info for the next chunk */
      father->nb_iterations_remaining[index] = n;
      father->current_from_for_guided[index] =
	next_from + cs * self->loop_incr;

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

      /* Return the corresponding chunk */
      *from = next_from;
      *to = next_from + cs * self->loop_incr;

      return 1;
    }

  /* No chunk left... just unlock and return 0 */

  sctk_nodebug ("__mpcomp_guided_loop_next[%d]: Next => no chunk left", rank);

  sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

  return 0;
}

void
__mpcomp_guided_loop_end ()
{
  sctk_nodebug ("__mpcomp_guided_loop_end[?]: End => barrier");
  /* FIXME use good barrier instead of OLD! */
  __mpcomp_old_barrier ();
}

void
__mpcomp_guided_loop_end_nowait ()
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;
  int index;
  int num_threads;
  int nb_exited_threads;

  /* Grab the thread info */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or
     this team has only 1 thread, no need to handle it */
  if (num_threads == 1)
    {
      return;
    }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;
  index = father->current_for_guided[rank];

  sctk_nodebug ("__mpcomp_guided_loop_end_nowait[%d]: End => index=%d",
		rank, index);



  sctk_spinlock_lock (&(father->lock_enter_for_guided[index]));

  nb_exited_threads = father->nb_threads_exited_for_guided[index] + 1;

  father->nb_threads_exited_for_guided[index] = nb_exited_threads;


  /* If I'm the last thread to exit */
  if (nb_exited_threads == num_threads)
    {
      int previous_index;
      int previous_nb_entered_threads;

      /* This index is now the nowait barrier */
      father->nb_threads_entered_for_guided[index] =
	MPCOMP_NOWAIT_STOP_SYMBOL;
      sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

      /* Update the info on the previous loop */
      previous_index = (index - 1 + MPCOMP_MAX_ALIVE_FOR_GUIDED + 1) %
	(MPCOMP_MAX_ALIVE_FOR_GUIDED + 1);

      sctk_spinlock_lock (&(father->lock_enter_for_guided[previous_index]));

      previous_nb_entered_threads =
	father->nb_threads_entered_for_guided[previous_index];

      if (previous_nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL)
	{

	  sctk_nodebug
	    ("__mpcomp_guided_loop_end_nowait[%d]: End => last one, moving barrier from %d",
	     rank, previous_index);

	  father->nb_threads_entered_for_guided[previous_index] = 0;

	}
      else
	{

	  sctk_nodebug
	    ("__mpcomp_guided_loop_end_nowait[%d]: End => last one, not moving",
	     rank);

	  sctk_assert (previous_nb_entered_threads ==
		       MPCOMP_NOWAIT_STOP_CONSUMED);
	}

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[previous_index]));


      return;
    }


  sctk_spinlock_unlock (&(father->lock_enter_for_guided[index]));

}


void
__mpcomp_start_parallel_guided_loop (int arg_num_threads, void *(*func)
				     (void *), void *shared, int lb, int b,
				     int incr, int chunk_size)
{
TODO("implement #pragma omp parallel for schedule(guided)")
  not_implemented ();
}


/* Start a loop shared by the team w/ a guided schedule.
   !WARNING! This function assumes that there is no loops w/ guided schedule
   and nowait clause previously executed in the same parallel region 
   */
int
__mpcomp_guided_loop_begin_ignore_nowait (int lb, int b, int incr, int
					  chunk_size, int *from, int *to)
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;
  int num_threads;
  int nb_entered_threads;
  int n;			/* Number of remaining iterations */
  int cs;			/* Current chunk size */

  /* Grab the thread info */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or
     this team has only 1 thread, the current thread will execute the whole
     loop */
  if (num_threads == 1)
    {
      *from = lb;
      *to = b;
      return 1;
    }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;

  sctk_nodebug
    ("__mpcomp_guided_loop_begin_ignore_nowait[%d]: Entering loop, " "vp=%d",
     rank, self->vp);

  sctk_spinlock_lock (&(father->lock_enter_for_guided[0]));

  nb_entered_threads = father->nb_threads_entered_for_guided[0];

  /* Am I the first one? */
  if (nb_entered_threads == 0)
    {

      /* I am the first to enter this loop */
      sctk_nodebug ("__mpcomp_guided_loop_begin_ignore_nowait[%d]: "
		    "First one => #threads %d", rank, num_threads);

      /* Update the current information */
      father->nb_threads_entered_for_guided[0] = 1;

      /* Compute the total number iterations */
      n = (b - lb) / incr;

      /* Compute the current chunk size */
      cs = n / num_threads;
      if (cs < n && (n % num_threads) != 0)
	{
	  cs++;
	}

      sctk_nodebug
	("__mpcomp_guided_loop_begin[%d]: First one => n=%d->%d cs=%d", rank,
	 n, n - cs, cs);

      /* Update the number of remaining iterations */
      n = n - cs;

      father->nb_iterations_remaining[0] = n;
      father->current_from_for_guided[0] = lb + cs * incr;

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[0]));

      /* Fill private information about the loop */
      self->loop_lb = lb;
      self->loop_b = b;
      self->loop_incr = incr;
      self->loop_chunk_size = chunk_size;

      /* Return the corresponding chunk */
      *from = lb;
      *to = lb + cs * incr;

      return 1;

    }

  sctk_assert (nb_entered_threads < num_threads);

  /* I am not the first one to enter
     -> check if there is still a chunk to execute */
  sctk_nodebug ("__mpcomp_guided_loop_begin[%d]: "
		"Not first one -> %d out of #threads %d",
		rank, nb_entered_threads, num_threads);

  /* Update the current information */
  father->nb_threads_entered_for_guided[0] = nb_entered_threads + 1;

  n = father->nb_iterations_remaining[0];

  sctk_nodebug
    ("__mpcomp_guided_loop_begin[%d]: Not first one -> %d iteration(s) remaining",
     rank, n);

  /* If there is a chunk remaining */
  if (n > 0)
    {
      int next_from;

      /* Compute the next chunk size */
      cs = n / num_threads;

      /* Ceil value */
      if (cs < n && (n % num_threads) != 0)
	{
	  cs++;
	}

      /* This size should be at least equal to the 'chunk_size' value */
      /* Except for the last chunk */
      if (cs < self->loop_chunk_size)
	{
	  cs = self->loop_chunk_size;
	  if (n < self->loop_chunk_size)
	    {
	      cs = n;
	    }
	}

      n = n - cs;

      /* Grab the 'from' info */
      next_from = father->current_from_for_guided[0];

      /* Update info for the next chunk */
      father->nb_iterations_remaining[0] = n;
      father->current_from_for_guided[0] = next_from + cs * incr;

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[0]));

      /* Fill private information about the loop */
      self->loop_lb = lb;
      self->loop_b = b;
      self->loop_incr = incr;
      self->loop_chunk_size = chunk_size;

      /* Return the corresponding chunk */
      *from = next_from;
      *to = next_from + cs * incr;

      return 1;
    }

  /* No chunk left... just unlock and return 0 */

  sctk_spinlock_unlock (&(father->lock_enter_for_guided[0]));

  sctk_nodebug ("__mpcomp_guided_loop_begin_ignore_nowait[%d]: "
		"Not first one -> no chunk left", rank);

  return 0;
}



int
__mpcomp_guided_loop_next_ignore_nowait (int *from, int *to)
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;
  int num_threads;
  int n;

  /* Grab the thread info */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this function is called from a sequential part (orphaned directive) or
   * this team has only 1 thread, the current thread has already executed the
   * whole loop 
   */
  if (num_threads == 1)
    {
      return 0;
    }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;

  sctk_spinlock_lock (&(father->lock_enter_for_guided[0]));

  n = father->nb_iterations_remaining[0];

  sctk_nodebug ("__mpcomp_guided_loop_next_ignore_nowait[%d]: "
		"Next => %d iteration(s) remaining", rank, n);

  /* If there is a chunk remaining */
  if (n > 0)
    {
      int next_from;
      int cs;

      /* Compute the next chunk size */
      cs = n / num_threads;

      /* Ceil value */
      if (cs < n && (n % num_threads) != 0)
	{
	  cs++;
	}

      /* This size should be at least equal to the 'chunk_size' value */
      /* Except for the last chunk */
      if (cs < self->loop_chunk_size)
	{
	  cs = self->loop_chunk_size;
	  if (n < self->loop_chunk_size)
	    {
	      cs = n;
	    }
	}

      sctk_nodebug ("__mpcomp_guided_loop_next_ignore_nowait[%d]: "
		    "Next => n=%d->%d cs=%d", rank, n, n - cs, cs);

      n = n - cs;

      /* Grab the 'from' info */
      next_from = father->current_from_for_guided[0];

      /* Update info for the next chunk */
      father->nb_iterations_remaining[0] = n;
      father->current_from_for_guided[0] = next_from + cs * self->loop_incr;

      sctk_spinlock_unlock (&(father->lock_enter_for_guided[0]));

      /* Return the corresponding chunk */
      *from = next_from;
      *to = next_from + cs * self->loop_incr;

      return 1;
    }

  /* No chunk left... just unlock and return 0 */
  sctk_spinlock_unlock (&(father->lock_enter_for_guided[0]));

  sctk_nodebug ("__mpcomp_guided_loop_next_ignore_nowait[%d]: "
		"Next => no chunk left", rank);


  return 0;
}

int
__mpcomp_ordered_guided_loop_begin (int lb, int b, int incr, int chunk_size,
			    int *from, int *to) {
  mpcomp_thread_info_t *info;
  int res ;

  res = __mpcomp_guided_loop_begin(lb, b, incr, chunk_size, from, to ) ;

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

  info->current_ordered_iteration = *from ;

  return res ;
}

int
__mpcomp_ordered_guided_loop_next (int *from, int *to) {
  mpcomp_thread_info_t *info;
  int res ;

  res = __mpcomp_guided_loop_next(from, to) ;

  /* TODO Use TLS if available */
  info =
    (mpcomp_thread_info_t *) mpc_thread_getspecific (mpcomp_thread_info_key);

  info->current_ordered_iteration = *from ;

  return res ;
}

void
__mpcomp_ordered_guided_loop_end ()
{
  __mpcomp_guided_loop_end() ;
}

void
__mpcomp_ordered_guided_loop_end_nowait ()
{
  __mpcomp_guided_loop_end() ;
}
