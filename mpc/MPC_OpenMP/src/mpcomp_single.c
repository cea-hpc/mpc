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



#define sctk_debug printf
int 
__mpcomp_do_single (void)
{
  mpcomp_thread_t *self;
  mpcomp_thread_team_t *team;

  long rank;
  int index;
  int num_threads;
  int nb_entered_threads;
  int previous_index;
  int i, j;

  __mpcomp_init();

  self = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert (self != NULL);

  team = self->team; 
  sctk_assert (team != NULL);

  num_threads = team->num_threads;

  if(num_threads == 1)
  {
   return 1;
  }

  rank = self->rank;

  index = (self->current_single + 1) % (MPCOMP_MAX_ALIVE_SINGLE+1);

  sctk_nodebug("[__mpcomp_do_single] beginning, rank=%d, index=%d, getting lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
  sctk_spinlock_lock (&(team->lock_enter_single[index]));
 sctk_nodebug("[__mpcomp_do_single] beginning, rank=%d, index=%d, lock taken, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
  
  
  //nb_entered_threads = team->nb_threads_entered_single[index];
  //sctk_spinlock_unlock (&(team->lock_enter_single[index]));
  
  sctk_nodebug("[mpcomp_do_single] t rank=%d begin.., index=%d, nb_entered_threads=%d, team->nb_threads_entered_single=%d, num_threads=%d\n", self->rank, index, nb_entered_threads, team->nb_threads_entered_single[index], num_threads); //AMAHEO - DEBUG

  sctk_nodebug("\n\n");
  sctk_nodebug("[mpcomp_do_single]rank=%d, index=%d, Display Single tab..\t\t", rank, index);
  for(i=0;i<MPCOMP_MAX_ALIVE_SINGLE+1;i++) {
   sctk_nodebug("%d\t", team->nb_threads_entered_single[i]);
  }
  sctk_nodebug("\n\n");

  /* Current thread encounters a STOP index */  
  if(team->nb_threads_entered_single[index] == MPCOMP_NOWAIT_STOP_SYMBOL)
  {

   //sctk_spinlock_lock (&(team->lock_enter_single[index]));
   //team->nb_threads_stop += 1;
   //self->stop_index = team->nb_threads_stop;
   sctk_nodebug("[__mpcomp_do_single] STOP_SYMBOL, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
   sctk_spinlock_unlock (&(team->lock_enter_single[index]));
   sctk_nodebug("[__mpcomp_do_single] STOP_SYMBOL, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

   //sctk_spinlock_unlock (&(team->lock_enter_single[index])); 
   sctk_nodebug("[__mpcomp_do_single] rank=%d, index=%d, nb entered threads=%d !!, nb_threads_stop=%d, stop_index=%d\n", rank, index, nb_entered_threads, team->nb_threads_stop, self->stop_index); //AMAHEO

   /* Spin on STOP index until it is released by next index */ 
   if(team->nb_threads_entered_single[index] == MPCOMP_NOWAIT_STOP_SYMBOL)
   {
    while(team->nb_threads_entered_single[index] == MPCOMP_NOWAIT_STOP_SYMBOL) 
    {
      sctk_nodebug("[__mpcomp_do_single] rank=%d, index=%d, spinning on STOP case..\n", rank, index); //AMAHEO
      sctk_thread_yield();
    }
   }

   sctk_nodebug("[__mpcomp_do_single] STOP_SYMBOL, rank=%d, index=%d, getting lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO      
   sctk_spinlock_lock (&(team->lock_enter_single[index]));  
   sctk_nodebug("[__mpcomp_do_single] STOP_SYMBOL, rank=%d, index=%d, lock taken, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO      
    
  }

  //sctk_spinlock_lock (&(team->lock_enter_single[index]));
  if(team->nb_threads_entered_single[index] == 0)
  {
sctk_nodebug("[__mpcomp_do_single] rank=%d, index=%d, team->nb_threads_entered_single=%d, first thread in Single construct, execute it 1..\n", rank, index, team->nb_threads_entered_single[index]);
    
    /* Increment number of threads on current index */
   //sctk_spinlock_lock (&(team->lock_enter_single[index]));
   team->nb_threads_entered_single[index] += 1;

   sctk_nodebug("[__mpcomp_do_single] nb_threads entered = 0, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO         
   sctk_spinlock_unlock (&(team->lock_enter_single[index])); 
   sctk_nodebug("[__mpcomp_do_single] nb threads entered = 0, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO      


   self->current_single = index; 
sctk_nodebug("[__mpcomp_do_single] rank=%d, index=%d, team->nb_threads_entered_single=%d, first thread in Single construct, execute it 2..\n", rank, index, team->nb_threads_entered_single[index]);
   return 1;
  }

  /* If I am the last thread to enter Single construct */
  if(team->nb_threads_entered_single[index] + 1 >= num_threads) 
  {
  
   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO          
   sctk_spinlock_unlock (&(team->lock_enter_single[index]));  
   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO      

   previous_index = (index-1+MPCOMP_MAX_ALIVE_SINGLE+1)%(MPCOMP_MAX_ALIVE_SINGLE+1);

   sctk_nodebug("***********[__mpcomp_do_single] rank=%d, index=%d, nb_threads_entered_single=%d, last thread in Single construct !, previous index=%d, previous value = %d\n", rank, index, nb_entered_threads, previous_index, team->nb_threads_entered_single[previous_index]); 

   //   sctk_thread_wait_for_value( (int*) &team->nb_threads_entered_single[previous_index], MPCOMP_NOWAIT_STOP_SYMBOL);
   if(team->nb_threads_entered_single[previous_index]  != MPCOMP_NOWAIT_STOP_SYMBOL) 
   { 
	   while(team->nb_threads_entered_single[previous_index] != MPCOMP_NOWAIT_STOP_SYMBOL)
	   {
		   sctk_nodebug("[__mpcomp_do_single] rank=%d, index=%d, spinning on STOP!\n", rank, index); //AMAHEO
		   sctk_thread_yield();
	   }
   }


  sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, getting lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO          
   sctk_spinlock_lock (&(team->lock_enter_single[index]));
   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, lock taken, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO      

   team->nb_threads_entered_single[index] = MPCOMP_NOWAIT_STOP_SYMBOL;

   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO          
   sctk_spinlock_unlock (&(team->lock_enter_single[index]));
   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO      


   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, getting lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO          
   sctk_spinlock_lock (&(team->lock_enter_single[previous_index]));    
   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, lock taken, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO      

   team->nb_threads_entered_single[previous_index] = 0;

   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[previous_index]); //AMAHEO          
   sctk_spinlock_unlock (&(team->lock_enter_single[previous_index]));   
   sctk_nodebug("[__mpcomp_do_single] last thread, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[previous_index]); //AMAHEO      

   self->current_single = index;   
   
    return 0; 
  }
   
  /* Increment number of threads on current index */
  sctk_nodebug("[__mpcomp_do_single] rank=%d, index=%d, nb_threads_entered_single=%d, increment nb of threads..\n", rank, index, nb_entered_threads);
  //sctk_spinlock_lock (&(team->lock_enter_single[index]));
  team->nb_threads_entered_single[index] += 1;
  sctk_spinlock_unlock (&(team->lock_enter_single[index]));   
  self->current_single = index;
 
  
 
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

  int i,j; 

  //single_t single_stop;
  //single_stop.nb_entered_threads = 0;
  //single_stop.flag = 0;

  __mpcomp_init();
 
  self = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  assert (self != NULL);

  team = self->team; 
  sctk_assert (team != NULL);

  num_threads = team->num_threads;

  if(num_threads == 1)
  {
    return 1;
  }

  sctk_assert (self != NULL);
  /* If this function is called from a sequential part (orphaned directive) or 
     this team has only 1 thread, the current thread will execute the single block
   */
  /* Get the rank of the current thread */
  rank = self->rank;
  index = (self->current_single + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);

  nb_entered_threads = team->nb_threads_entered_single[index];
     rank, index, self->vp, MPCOMP_MAX_ALIVE_SINGLE);


  printf("[mpcomp_do_single] t rank=%d begin.., index=%d, nb_entered_threads=%d, num_threads=%d\n", self->rank, index, nb_entered_threads, num_threads); //AMAHEO - DEBUG

  printf("\n\n");
  printf("[mpcomp_do_single]rank=%d, Display Single tab..\t\t", rank);
  for(i=0;i<MPCOMP_MAX_ALIVE_SINGLE+1;i++) {
   printf("%d\t", team->nb_threads_entered_single[i]);
  }
  printf("\n\n");

  //printf("[mpcomp_do_single] self rank=%d, index=%d, nb_entered_threads=%d\n", self->rank, index, nb_entered_threads); //AMAHEO  

  /* MPCOMP_NOWAIT_STOP_SYMBOL in the next workshare structure means that the
  * buffer is full (too many alive single constructs).
  * Therefore, the threads of this team have to wait. */
  if (nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL)
  {

    sctk_spinlock_lock (&(team->lock_enter_single[index]));
     
    team->nb_threads_stop += 1;
    self->stop_index = team->nb_threads_stop;

    //int next_index = (index + 1 + MPCOMP_MAX_ALIVE_SINGLE + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);

    //if(team->nb_threads_entered_single[next_index] + 1 == num_threads)
     //printf("[__mpcomp_do_single] rank=%d next index of STOP index full !! FAILURE !!\n", rank);

    //printf("[__mpcomp_do_single] MPCOMP_NOWAIT_STOP_SINGLE, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
    sctk_spinlock_unlock (&(team->lock_enter_single[index]));
    //printf("[__mpcomp_do_single] MPCOMP_NOWAIT_STOP_SINGLE, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
    
    //printf("[__mpcomp_do_single] rank=%d, index=%d, MPCOMP_NOWAIT_STOP_SYMBOL, nb_threads_entered_single=%d, nb_threads_stop=%d..\n", self->rank, index, team->nb_threads_entered_single[index], team->nb_threads_stop); //AMAHEO
       
    if(team->nb_threads_entered_single[index] == MPCOMP_NOWAIT_STOP_SYMBOL) {
     
     while((team->nb_threads_entered_single[index] == MPCOMP_NOWAIT_STOP_SYMBOL)) {
       //printf("[__mpcomp_do_single] rank=%d, index=%d, MPCOMP_NOWAIT_STOP_SYMBOL, nb_threads_entered_single=%d, thread spinning...\n", self->rank, index, team->nb_threads_entered_single[index]); //AMAHEO
       sctk_thread_yield();
     } 
    }
   
    /*
    sctk_spinlock_lock (&(team->lock_enter_single[index]));
    self->current_single = index;
    team->nb_threads_entered_single[index] += 1;
    sctk_spinlock_unlock (&(team->lock_enter_single[index]));
    */  

    //printf("[__mpcomp_do_single] rank=%d, index=%d, MPCOMP_NOWAIT_STOP_SYMBOL, nb_threads_entered_single=%d, thread unlocked...\n", self->rank, index, team->nb_threads_entered_single[index]); //AMAHEO

    
    //if(self->stop_index == 1)
    if(team->nb_threads_entered_single[index] == 1)
     return 1;
    else
     return 0;
    

    //return __mpcomp_do_single();
  }

/* Otherwise, just check if I'm not the last one and do not execute the
     single block */
  sctk_spinlock_lock (&(team->lock_enter_single[index]));
  team->nb_threads_entered_single[index] += 1;
  self->current_single = index;
  sctk_spinlock_unlock (&(team->lock_enter_single[index]));
 
     /* Am I the first one? */
  if (nb_entered_threads == 0)
    {
      sctk_spinlock_lock (&(team->lock_enter_single[index]));
      /* Yes => execute the single block */
      team->nb_threads_entered_single[index] = 1;

      //self->current_single = index;
      //printf("[__mpcomp_do_single] nb_entered_threads = 0, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
      sctk_spinlock_unlock (&(team->lock_enter_single[index]));
      //printf("[__mpcomp_do_single] nb_entered_threads = 0, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

      //printf("[__mpcomp_do_single] rank=%d, index=%d, nb_entered_threads=0, EXECUTE single construct..\n", self->rank, index); //AMAHEO 

      return 1;
    }


  /* If I'm the last thread to exit */
  //if (team->nb_threads_entered_single[index] + 1 == num_threads)
  if (nb_entered_threads + 1 >= num_threads)
    {
      int previous_index;
      int previous_nb_entered_threads;

      /* Update the info on the previous loop */
      previous_index = (index - 1 + MPCOMP_MAX_ALIVE_SINGLE + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);
      //previous_index = (self->current_single - 1 + MPCOMP_MAX_ALIVE_SINGLE + 1) % (MPCOMP_MAX_ALIVE_SINGLE + 1);

      printf("[__mpcomp_do_single] rank=%d, index=%d, previous_index=%d, previous index value=%d, nb_entered_threads=%d, num_threads=%d last thread to exit !!\n", self->rank, index, previous_index, team->nb_threads_entered_single[previous_index], nb_entered_threads, num_threads); //AMAHEO

      //printf("[__mpcomp_do_single] last thread, rank=%d, index=%d, getting lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
      sctk_spinlock_lock (&(team->lock_enter_single[index]));
      //printf("[__mpcomp_do_single] las thread, rank=%d, index=%d, lock acquired, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

      previous_nb_entered_threads =
	team->nb_threads_entered_single[previous_index];
   
      team->nb_threads_entered_single[index] = MPCOMP_NOWAIT_STOP_SYMBOL;

      //printf("[__mpcomp_do_single] end, rank=%d, index=%d, releasing lock, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO
      sctk_spinlock_unlock (&(team->lock_enter_single[index]));
      //printf("[__mpcomp_do_single] end, rank=%d, index=%d, lock released, lock_enter_single=%d..\n", self->rank, index, team->lock_enter_single[index]); //AMAHEO

      //assert(previous_nb_entered_threads == MPCOMP_NOWAIT_STOP_SYMBOL);     
 
      while (previous_nb_entered_threads != MPCOMP_NOWAIT_STOP_SYMBOL) 
      {
        sctk_thread_yield();
        //printf("[__mpcomp_do_single] rank=%d, index=%d, previous_index=%d previous_nb_entered_threads=%d, polling on STOP value ..\n", rank, index, previous_index, previous_nb_entered_threads); 
      }
     
      printf("[__mpcomp_do_single] rank=%d, index=%d, previous_nb_entered_threads = STOP \n", rank, index, previous_nb_entered_threads);

      sctk_spinlock_lock (&(team->lock_enter_single[previous_index]));
      team->nb_threads_entered_single[previous_index] = 0;
      //team->nb_threads_stop = 0;
      sctk_spinlock_unlock (&(team->lock_enter_single[previous_index]));

      //team->nb_threads_stop = 0;
 
      //team->nb_threads_entered_single[index] = nb_entered_threads + 1;
     // self->current_single = index;
    
     return 0;    
   }

 return 0;
}

