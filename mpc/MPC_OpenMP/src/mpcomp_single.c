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
#include <mpcomp_internal.h>
#include <sctk_debug.h>

/* TODO maybe create a special version when the 'nowait' clause is not present?
 */

int 
__mpcomp_do_single (void)
{

  mpcomp_thread_t *self;
  mpcomp_thread_team_t *team;
  
  long rank;
  int index;
  int num_threads;
  int nb_entered_threads;

  __mpcomp_init();
 
  self = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert (self != NULL);


  num_threads = self->team->num_threads;

  if( num_threads == 1)
  {
    return 1;
  }

  team = self->team; 
  sctk_assert (team != NULL);
  sctk_assert (self != NULL);
  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */

  /* Get the rank of the current thread */
  rank = self->rank;
  index = (self->current_single + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);

  sctk_spinlock_lock (&(team->lock_enter_single[index]));
  nb_entered_threads = team->nb_threads_entered_single[index];
     rank, index, self->vp, MPCOMP_MAX_ALIVE_SINGLE);

   

  /* MPCOMP_NOWAIT_STOP_SYMBOL in the next workshare structure means that the
  * buffer is full (too many alive single constructs).
  * Therefore, the threads of this team have to wait. */
  if (nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL)
    {

      sctk_nodebug
	("__mpcomp_do_single[%d]: MPCOMP_NOWAIT_STOP_SYMBOL at %d (max %d)",
	 rank, index, MPCOMP_MAX_ALIVE_SINGLE);

      team->nb_threads_entered_single[index] = MPCOMP_NOWAIT_STOP_CONSUMED;
      
      sctk_spinlock_unlock (&(team->lock_enter_single[index]));

      /* Generate a barrier (it should reinitialize all counters, especially
       * 'single' ones) */
      /* TODO need a dedicated barrier re-initializing only values related to
         in-flight single */
      /* FIXME use good barrier instead of OLD! */
      __mpcomp_barrier ();

      return __mpcomp_do_single ();
    }

  /* MPCOMP_NOWAIT_STOP_CONSUMED => some else is already blocked here, so just
   * call the barrier */
  if (nb_entered_threads == MPCOMP_NOWAIT_STOP_CONSUMED)
    {
      sctk_nodebug
	("__mpcomp_do_single[%d]: MPCOMP_NOWAIT_STOP_CONSUMED at %d (max %d)",
	 rank, index, MPCOMP_MAX_ALIVE_SINGLE);

      sctk_spinlock_unlock (&(team->lock_enter_single[index]));

      /* Generate a barrier (it should reinitialize all counters) */
      /* TODO need a dedicated barrier re-initializing only values related to
         in-flight single */
      /* FIXME use good barrier instead of OLD! */
      __mpcomp_barrier ();

      return __mpcomp_do_single ();
    }

  /* Am I the first one? */
  if (nb_entered_threads == 0)
    {
      /* Yes => execute the single block */
      team->nb_threads_entered_single[index] = 1;

      sctk_spinlock_unlock (&(team->lock_enter_single[index]));

      self->current_single = index;

      return 1;
    }

  /* Otherwise, just check if I'm not the last one and do not execute the
     single block */

  team->nb_threads_entered_single[index] = nb_entered_threads + 1;
  
  sctk_spinlock_unlock (&(team->lock_enter_single[index]));

  self->current_single = index;

  /* If I'm the last thread to exit */
  if (nb_entered_threads + 1 == num_threads)
    {
      int previous_index;
      int previous_nb_entered_threads;

      /* Update the info on the previous loop */
      previous_index = (index - 1 + MPCOMP_MAX_ALIVE_SINGLE + 1) %
	(MPCOMP_MAX_ALIVE_SINGLE + 1);

      sctk_spinlock_lock (&(team->lock_enter_single[previous_index]));

      previous_nb_entered_threads =
	team->nb_threads_entered_single[previous_index];


      if (previous_nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL)
	{
	  sctk_nodebug
	    ("__mpcomp_do_single[%d]: End => last one, moving barrier from %d to %d",
	     rank, previous_index, index);

	  team->nb_threads_entered_single[previous_index] = 0;
	  /* FIXME I don't acquire a lock on 'index' because 'index--' is already
	   * locked and I'm the last thread to have entered this single w/ 'index'
	   * */
	  team->nb_threads_entered_single[index] =
	    MPCOMP_NOWAIT_STOP_SYMBOL;

	}
      else
	{

	  sctk_nodebug
	    ("__mpcomp_do_single[%d]: End => last one, not moving (#threads[%d]=%d)",
	     rank, previous_index, previous_nb_entered_threads);

	  sctk_assert (previous_nb_entered_threads ==
		       MPCOMP_NOWAIT_STOP_CONSUMED);
	}

      sctk_spinlock_unlock (&(team->lock_enter_single[previous_index]));
    }

  return 0;

}

