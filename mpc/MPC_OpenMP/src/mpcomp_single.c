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

  int i,j; 

  //single_t single_stop;
  //single_stop.nb_entered_threads = 0;
  //single_stop.flag = 0;

  __mpcomp_init();
 
  self = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert (self != NULL);


  //num_threads = self->num_threads;
 
  /*
  if( num_threads == 1)
  {
    return 1;
  }
 */

  team = self->team; 
  sctk_assert (team != NULL);

  num_threads = team->num_threads;

  if(num_threads == 1)
  {
    return 1;
  }

  rank = self->rank;
  index = (self->current_single + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);

  //printf("[__mpcomp_do_single] begin, self rank=%d, index=%d, waiting for getting lock, lock_enter_single=%d\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
  sctk_spinlock_lock (&(team->lock_enter_single[index]));
  //printf("[__mpcomp_do_single] begin, rank=%d, index=%d lock acquired, lock_enter_single=%d.\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
  
  nb_entered_threads = team->nb_threads_entered_single[index];

  //printf("[mpcomp_do_single] t rank=%d begin..\n", self->rank); //AMAHEO - DEBUG

  //printf("[mpcomp_do_single] Display Single tab..\t\t");
  for(i=0;i<MPCOMP_MAX_ALIVE_SINGLE+1;i++) {
   //printf("%d\t", team->nb_threads_entered_single[i]);
  }

  //printf("\n\n");

  //printf("[mpcomp_do_single] self rank=%d, index=%d, nb_entered_threads=%d\n", self->rank, index, nb_entered_threads); //AMAHEO  
  //single_stop.nb_entered_threads = nb_entered_threads;
  //single_stop.flag = 0;

  /* MPCOMP_NOWAIT_STOP_SYMBOL in the next workshare structure means that the
  * buffer is full (too many alive single constructs).
  * Therefore, the threads of this team have to wait. */
  if (nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL)
  { 
    team->nb_threads_stop += 1;
    self->stop_index = team->nb_threads_stop;

    //printf("[__mpcomp_do_single] MPCOMP_NOWAIT_STOP_SINGLE, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
    sctk_spinlock_unlock (&(team->lock_enter_single[index]));
    //printf("[__mpcomp_do_single] MPCOMP_NOWAIT_STOP_SINGLE, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
    
    //printf("[__mpcomp_do_single] rank=%d, index=%d, MPCOMP_NOWAIT_STOP_SYMBOL, nb_threads_entered_single=%d, nb_threads_stop=%d..\n", self->rank, index, team->nb_threads_entered_single[index], team->nb_threads_stop); //AMAHEO

    /* Spin until last thread unlocks STOP state */
    //sctk_thread_wait_for_value_and_poll(&(single_stop.flag), 1, check_single_flag, &(single_stop));

         
    if(team->nb_threads_entered_single[index] == MPCOMP_NOWAIT_STOP_SYMBOL) {
     
     while((team->nb_threads_entered_single[index] == MPCOMP_NOWAIT_STOP_SYMBOL)) {
       //printf("[__mpcomp_do_single] rank=%d, index=%d, MPCOMP_NOWAIT_STOP_SYMBOL, nb_threads_entered_single=%d, thread spinning...\n", self->rank, index, team->nb_threads_entered_single[index]); //AMAHEO
       sctk_thread_yield();
     } 
    }
    
    self->current_single = index;
  
    //printf("[__mpcomp_do_single] rank=%d, index=%d, MPCOMP_NOWAIT_STOP_SYMBOL, nb_threads_entered_single=%d, thread unlocked...\n", self->rank, index, team->nb_threads_entered_single[index]); //AMAHEO

    if(self->stop_index == 1)
     return 1;
    else
     return 0;
    
    //return __mpcomp_do_single();
  }
 
     /* Am I the first one? */
  if (nb_entered_threads == 0)
    { 
      /* Yes => execute the single block */
      team->nb_threads_entered_single[index] = 1;

     self->current_single = index;
      //printf("[__mpcomp_do_single] nb_entered_threads = 0, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
      sctk_spinlock_unlock (&(team->lock_enter_single[index]));
      //printf("[__mpcomp_do_single] nb_entered_threads = 0, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

      //printf("[__mpcomp_do_single] rank=%d, index=%d, nb_entered_threads=0, EXECUTE single construct..\n", self->rank, index); //AMAHEO 

      return 1;
    }

  /* Otherwise, just check if I'm not the last one and do not execute the
     single block */

  team->nb_threads_entered_single[index] = nb_entered_threads + 1;

  //printf("[__mpcomp_do_single] otherwise, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
  //sctk_spinlock_unlock (&(team->lock_enter_single[index]));
  //printf("[__mpcomp_do_single] otherwise, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

  self->current_single = index;

  /* If I'm the last thread to exit */
  if (nb_entered_threads + 1 == num_threads)
    {
      int previous_index;
      int previous_nb_entered_threads;

      /* Update the info on the previous loop */
      previous_index = (index - 1 + MPCOMP_MAX_ALIVE_SINGLE + 1) %
	(MPCOMP_MAX_ALIVE_SINGLE + 1);

      //printf("[__mpcomp_do_single] rank=%d, index=%d, previous_index=%d, last thread to exit !!\n", self->rank, index, previous_index); //AMAHEO

      //printf("[__mpcomp_do_single] last thread, rank=%d, index=%d, getting lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
      //sctk_spinlock_lock (&(team->lock_enter_single[index]));
      //printf("[__mpcomp_do_single] las thread, rank=%d, index=%d, lock acquired, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

      previous_nb_entered_threads =
	team->nb_threads_entered_single[previous_index];
   
      team->nb_threads_entered_single[index] =
	    MPCOMP_NOWAIT_STOP_SYMBOL;

      //printf("[__mpcomp_do_single] end, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
      sctk_spinlock_unlock (&(team->lock_enter_single[index]));
      //printf("[__mpcomp_do_single] end, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

      if (previous_nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL)
	{
	  sctk_nodebug
	    ("__mpcomp_do_single[%d]: End => last one, moving barrier from %d to %d",
	     rank, previous_index, index);

          //printf("[__mpcomp_do_single] last thread, rank=%d, index=%d, getting lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[previous_index]); //AMAHEO
          sctk_spinlock_lock (&(team->lock_enter_single[previous_index]));
          //printf("[__mpcomp_do_single] las thread, rank=%d, index=%d, lock acquired, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[previous_index]); //AMAHEO

   
          /* DEBUG */
          if(team->nb_threads_stop == MPCOMP_NOWAIT_STOP_SYMBOL) {
            printf("[__mpcomp_do_single[%d] team->nb_threads_stop equals MPCOMP_NOWAIT_STOP_SYMBOL !\n", rank);
          }
          printf("[__mpcomp_do_single] rank=%d, nb_threads_stop=%d at index %d..\n", rank, team->nb_threads_stop, index);
  
          //printf("[mpcomp_do_single] rank=%d index=%d, previous_index=%d, nb_threads_entered_single=%d, STOP_SYMBOL, nb_threads_stop=%d..\n", self->rank, index, previous_index, team->nb_threads_entered_single[previous_index], team->nb_threads_stop); //AMAHEO 
	  team->nb_threads_entered_single[previous_index] = team->nb_threads_stop;
          //team->nb_threads_entered_single[previous_index] = 0;
          team->nb_threads_stop = 0;
	  /* FIXME I don't acquire a lock on 'index' because 'index--' is already
	   * locked and I'm the last thread to have entered this single w/ 'index'
	   * */

          //printf("[__mpcomp_do_single] end, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[previous_index]); //AMAHEO
          sctk_spinlock_unlock (&(team->lock_enter_single[previous_index]));
          //printf("[__mpcomp_do_single] end, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[previous_index]); //AMAHEO
	 
	}
    
     return 0;    
   }

  //printf("[__mpcomp_do_single] end, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
  sctk_spinlock_unlock (&(team->lock_enter_single[index]));
  //printf("[__mpcomp_do_single] end, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

  //printf("[__mpcomp_do_single] rank=%d, index=%d, end..\n", self->rank, index); 

 return 0;
}

int 
__old_mpcomp_do_single (void)
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

