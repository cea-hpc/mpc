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
#include <sctk.h>
#include <sctk_debug.h>

/*
   This file contains all functions related to SECTIONS constructs in OpenMP
 */

int
__mpcomp_sections_begin (int nb_sections)
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  int rank;
  int index;
  int num_threads;

  __mpcomp_init() ;

  sctk_nodebug( "Entering __mpcomp_sections_begin w/ %d section(s)", nb_sections ) ;

  /* Do not execute if there is no sections */
  if ( nb_sections == 0 ) {
    return 0 ;
  }

  /* Grab the thread info */
  /* TODO use TLS if available */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this thread is the only one of its team 
     -> it executes every section */
  if (num_threads == 1)
    {
      self->nb_sections[0] = nb_sections ;
      self->next_section[0] = 2 ;
      return 1;
    }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;
  index =
    (father->current_sections[rank] + 1) % (MPCOMP_MAX_ALIVE_SECTIONS + 1);

  sctk_nodebug
    ("__mpcomp_sections_begin[%d] Entering new construct index=%d with father %p and %d section(s)",
     rank, index, father, nb_sections);

  sctk_spinlock_lock (&(father->lock_enter_sections[index]));


  /* MPCOMP_NOWAIT_STOP_SYMBOL => there are too many alive sections construct
     Need to stop and wait for everyone */
  if (father->next_section[index] == MPCOMP_NOWAIT_STOP_SYMBOL)
    {
      father->next_section[index] = MPCOMP_NOWAIT_STOP_CONSUMED;
      sctk_spinlock_unlock (&(father->lock_enter_sections[index]));

      sctk_nodebug ("__mpcomp_sections_begin[%d] Barrier (first one)", rank);

      /* TODO need a dedicated barrier re-initializing only values related to
         in-flight sections */
      /* FIXME use good barrier instead of OLD! */
      __mpcomp_old_barrier ();

      sctk_nodebug ("__mpcomp_sections_begin[%d] After Barrier (first one)",
		    rank);

      return __mpcomp_sections_begin (nb_sections);
    }

  /* MPCOMP_NOWAIT_STOP_CONSUMED  => there are too many alive sections
     construct and someone else already entered the barrier */
  if (father->next_section[index] == MPCOMP_NOWAIT_STOP_CONSUMED)
    {
      sctk_spinlock_unlock (&(father->lock_enter_sections[index]));

      sctk_nodebug ("__mpcomp_sections_begin[%d] Barrier", rank);

      /* TODO need a dedicated barrier re-initializing only values related to
         in-flight sections */
      /* FIXME use good barrier instead of OLD! */
      __mpcomp_old_barrier ();

      sctk_nodebug ("__mpcomp_sections_begin[%d] After Barrier", rank);

      return __mpcomp_sections_begin (nb_sections);
    }

  /* Am I the first one to enter this sections construct */
  if (father->next_section[index] == 0)
    {
      /* Fill the number of sections to execute */
      father->nb_sections[index] = nb_sections;

      /* Update the next section to execute */
      father->next_section[index] = 2;

      /* Update the current section construct for the current thread */
      father->current_sections[rank]++;

      sctk_spinlock_unlock (&(father->lock_enter_sections[index]));

      sctk_nodebug
	("__mpcomp_sections_begin[%d] Returned section 1 (first one)", rank);

      /* TODO return 0 if there is no section to execute? */
      return 1;

    }
  else
    {
      int n;
      int nb ;

      n = father->next_section[index];

      father->next_section[index] = n + 1 ;

      /* Update the current section construct for the current thread */
      father->current_sections[rank]++;

      nb = father->nb_sections[index] ;


      sctk_spinlock_unlock (&(father->lock_enter_sections[index]));

      if (n > nb )
	{
	  n = 0;
	}

      sctk_nodebug ("__mpcomp_sections_begin[%d] Returned section %d", rank,
		    n);


      return n;
    }


}

int
__mpcomp_sections_next ()
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;
  int index;
  int num_threads;
  int return_section;
  int n;

  /* Grab the thread info */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  /* If this thread is the only one of its team 
     -> it executes every section */
  if ( num_threads == 1 ) {
    return_section = self->next_section[0] ;
    if ( return_section <= self->nb_sections[0] ) {
      self->next_section[0] = return_section + 1 ;
      return return_section ;
    }
    self->next_section[0] = 0 ; /* Reinitialize now to avoid overhead during the barrier */
    return 0 ;
  }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;
  index = (father->current_sections[rank]) % (MPCOMP_MAX_ALIVE_SECTIONS + 1);

  sctk_nodebug ("__mpcomp_sections_next[%d]: Entering -> current sections = %d", rank,
		father->current_sections[rank]);

  sctk_spinlock_lock (&(father->lock_enter_sections[index]));

  /* Get the next section to execute */
  n = father->next_section[index];

  if (n > father->nb_sections[index])
    {
      return_section = 0;
    }
  else
    {
      return_section = n;
      father->next_section[index]++;
    }

  sctk_nodebug
    ("__mpcomp_sections_next[%d] Construct %d: Next section %d vs. Nb sections %d => return %d",
     rank, index, n, father->nb_sections[index], return_section);

  sctk_spinlock_unlock (&(father->lock_enter_sections[index]));



  return return_section;
}

void
__mpcomp_sections_end ()
{
  /* TODO need a dedicated barrier re-initializing only values related to
     in-flight sections */
  /* FIXME use good barrier instead of OLD! */
  __mpcomp_old_barrier ();
}

void
__mpcomp_sections_end_nowait ()
{
  mpcomp_thread_info_t *self;	/* Info on the current thread */
  mpcomp_thread_info_t *father;	/* Info on the team */
  long rank;
  int index;
  int num_threads;

  /* Grab the thread info */
  self = (mpcomp_thread_info_t *) sctk_thread_getspecific
    (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  /* Number of threads in the current team */
  num_threads = self->num_threads;

  if ( num_threads == 1 ) {
    return ;
  }

  /* Get the father info (team info) */
  father = self->father;
  sctk_assert (father != NULL);

  /* Get the rank of the current thread */
  rank = self->rank;
  index = father->current_sections[rank] % (MPCOMP_MAX_ALIVE_SECTIONS + 1);

  sctk_spinlock_lock (&(father->lock_enter_sections[index]));

  /* I exited this sections construct */
  father->nb_threads_exited_sections[index]++;

  /* Move the barrier if I am the last one exiting the workshare construct */
  if (father->nb_threads_exited_sections[index] == num_threads)
    {
      int previous_index;
      father->next_section[index] = MPCOMP_NOWAIT_STOP_SYMBOL;	/* New barrier
								   here */
      sctk_spinlock_unlock (&(father->lock_enter_sections[index]));


      previous_index =
	((index + MPCOMP_MAX_ALIVE_SECTIONS) % (MPCOMP_MAX_ALIVE_SECTIONS +
						1));

      sctk_nodebug
	("__mpcomp_sections_end_nowait[%d] Last one of index %d (#threads %d) -> previous_index=%d with value %d",
	 rank, index, num_threads, previous_index,
	 father->next_section[previous_index]);

      sctk_spinlock_lock (&(father->lock_enter_sections[previous_index]));
      if (father->next_section[previous_index] == MPCOMP_NOWAIT_STOP_SYMBOL)
	{
	  father->next_section[previous_index] = 0;
	}
      else
	{
	  assert (father->next_section[previous_index] ==
		  MPCOMP_NOWAIT_STOP_CONSUMED);
	}
      sctk_spinlock_unlock (&(father->lock_enter_sections[previous_index]));



    }
  else
    {
      sctk_spinlock_unlock (&(father->lock_enter_sections[index]));
    }
}


void
__mpcomp_start_sections_parallel_region (int arg_num_threads, void *(*func) (void *),
				void *shared, int nb_sections )
{
  mpcomp_thread_info_t *current_info;
  int num_threads;

  SCTK_PROFIL_START (__mpcomp_start_parallel_region);

  /* Initialize the OpenMP environment (call several times, but really executed
   * once) */
  __mpcomp_init ();

  /* Do not execute if there is no sections */
  if ( nb_sections == 0 ) {
    return ;
  }

  /* Retrieve the information (microthread structure and current region) */
  /* TODO Use TLS if available */
  current_info = sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (current_info != NULL);

  sctk_nodebug
    ("__mpcomp_start_sections_parallel_region: "
     "Entering w/ %d section(s)",
     nb_sections );

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
	("__mpcomp_start_sections_parallel_region: Only 1 thread -> call f");

      current_info->nb_sections[0] = nb_sections ;
      current_info->next_section[0] = 1 ;

      func (shared);
      SCTK_PROFIL_END (__mpcomp_start_parallel_region);
      return;
    }

  sctk_nodebug
    ("__mpcomp_start_sections_parallel_region: -> Final num threads = %d",
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
	("__mpcomp_start_sections_parallel_region: starting new team at depth %d on %d microVP(s)",
	 current_info->depth, current_info->icvs.nmicrovps_var);

      current_task = current_info->task;

      /* Initialize a new microthread structure if needed */
      if (current_info->depth == 0)
	{
	  new_task = current_task;

	  sctk_nodebug
	    ("__mpcomp_start_sections_parallel_region: reusing the thread_info depth 0");
	}
      else
	{

	  /* If the first child has already been created, get the
	   * corresponding thread-info structure */
	  if (current_info->children[0] != NULL)
	    {
	      sctk_nodebug
		("__mpcomp_start_sections_parallel_region: reusing thread_info new depth");

	      new_task = current_info->children[0]->task;
	      sctk_assert (new_task != NULL);

	    }
	  else
	    {

	      sctk_nodebug
		("__mpcomp_start_sections_parallel_region: allocating thread_info new depth");

	      new_task = sctk_malloc (sizeof (sctk_microthread_t));
	      sctk_assert (new_task != NULL);
	      sctk_microthread_init (current_info->icvs.nmicrovps_var, new_task);
	    }
	}

      current_info->nb_sections[0] = nb_sections ;
      current_info->next_section[0] = 1 ;


#warning "TODO Only one way to distribute thread in case of parallel sections"
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
		("__mpcomp_start_sections_parallel_region: "
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

	  /* Update private information about sections */
	  current_info->current_sections[i] = 0 ;
	  /* NONE */

	  /* Update shared information about sections */
	  /* NONE */

	  sctk_nodebug
	    ("__mpcomp_start_sections_parallel_region: "
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
	("__mpcomp_start_sections_parallel_region: Microthread execution done");

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
	("__mpcomp_start_sections_parallel_region: Serialize a new team at depth %d",
	 current_info->depth);

      /* TODO only flatterned nested supported for now */

      num_threads = 1;


      new_info = current_info->children[0];
      if (new_info == NULL)
	{

	  sctk_nodebug
	    ("__mpcomp_start_sections_parallel_region: Allocating new thread_info");

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
	    ("__mpcomp_start_sections_parallel_region: Reusing older thread_info");
	  __mpcomp_reset_thread_info (new_info, func, shared, 0,
				      current_info->icvs, 0, 0,
				      current_info->vp);
	}

      __mpcomp_wrapper_op (new_info);

      sctk_nodebug ("__mpcomp_start_sections_parallel_region: after flat nested");

      sctk_thread_setspecific (mpcomp_thread_info_key, current_info);

      sctk_free (new_info);
    }

#warning "TODO can we only reset sections-team info?"
  /* Re-initializes team info for this thread */
  __mpcomp_reset_team_info (current_info, num_threads);


  /* Restore the TLS for the main thread */
  sctk_extls = current_info->children[0]->extls;
  sctk_tls_module = current_info->children[0]->tls_module;
  sctk_context_restore_tls_module_vp ();

  SCTK_PROFIL_END (__mpcomp_start_parallel_region);
}
