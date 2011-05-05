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
#include "sctk.h"
#include "sctk_alloc.h"
#include "sctk_asm.h"
#include "sctk_debug.h"
#include "sctk_topology.h"
#include "mpcmicrothread_internal.h"
#include "mpcomp_internal.h"
#include <sys/time.h>

#define SCTK_OMP_VERSION_MAJOR 2
#define SCTK_OMP_VERSION_MINOR 5


/*
 * Environment variable used to fill the ICVs (OpenMP 2.5) 
 */

/* Schedule type */
static omp_sched_t OMP_SCHEDULE = 1;
/* Schedule modifier */
static int OMP_MODIFIER_SCHEDULE = -1;
/* Defaults number of threads */
static int OMP_NUM_THREADS = 0;
/* Is dynamic adaptation on? */
static int OMP_DYNAMIC = 0;
/* Is nested parallelism handled or flaterned? */
static int OMP_NESTED = 0;
/* Number of VPs for each OpenMP team */
static int OMP_VP_NUMBER = 0;

/* OpenMP information of the current thread */
sctk_thread_key_t mpcomp_thread_info_key;

static mpc_thread_once_t mpcomp_thread_info_key_is_initialized =
  SCTK_THREAD_ONCE_INIT;

/* Initialization of the key named 'mpcomp_thread_info_key' */
void
mpcomp_thread_info_key_init ()
{
  sctk_thread_key_create (&mpcomp_thread_info_key, NULL);
}

/* Runtime OpenMP Initialization 
 Can be called several times without any side effects */
void
__mpcomp_init (void)
{
  static volatile int done = 0;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  mpcomp_thread_info_t *init_info;
  char *env;
  int nb_threads;
  sctk_microthread_t *self;
  int res;

  /* Initialization of the sctk_microthread key (only once) */
  res = mpc_thread_once (&mpcomp_thread_info_key_is_initialized,
			 mpcomp_thread_info_key_init);
  sctk_assert (res == 0);

  sctk_nodebug ("__mpcomp_init: Enter init");


  init_info = sctk_thread_getspecific (mpcomp_thread_info_key);

  if (init_info == NULL)
    {
      icv_t icvs;
      sctk_thread_mutex_lock (&lock);

      sctk_nodebug ("__mpcomp_init: Enter init w/ lock");

      /* Read the environment variables only once per process */
      if (done == 0)
	{

	  sctk_nodebug ("__mpcomp_init: Read env vars (MPC rank: %d)",
			sctk_get_task_rank ());

	  /******* OMP_VP_NUMBER *********/
	  env = getenv ("OMP_VP_NUMBER");
	  OMP_VP_NUMBER = sctk_get_processor_number (); /* DEFAULT */
	  if (env != NULL)
	    {
	      int arg = atoi( env ) ;
	      if ( arg <= 0 ) {
		fprintf( stderr, 
			  "Warning: Incorrect number of microVPs (OMP_VP_NUMBER=<%s>) -> "
			  "Switching to default value %d\n", env, OMP_VP_NUMBER ) ;
	      } else {
		OMP_VP_NUMBER = arg ;
	      }
	    }

	  /******* OMP_SCHEDULE *********/
	  env = getenv ("OMP_SCHEDULE");
	  OMP_SCHEDULE = 1 ;	/* DEFAULT */
	  if (env != NULL)
	    {
	      int ok = 0 ;
	      int offset = 0 ;
	      if ( strncmp (env, "static", 6) == 0 ) {
		OMP_SCHEDULE = 1 ;
		offset = 6 ;
		ok = 1 ;
	      } 
	      if ( strncmp (env, "dynamic", 7) == 0 ) {
		OMP_SCHEDULE = 2 ;
		offset = 7 ;
		ok = 1 ;
	      } 
	      if ( strncmp (env, "guided", 6) == 0 ) {
		OMP_SCHEDULE = 3 ;
		offset = 6 ;
		ok = 1 ;
	      } 
	      if ( strncmp (env, "auto", 4) == 0 ) {
		OMP_SCHEDULE = 4 ;
		offset = 4 ;
		ok = 1 ;
	      } 
	      if ( ok ) {
		int chunk_size = 0 ;
		/* Check for chunk size, if present */
		sctk_debug( "Remaining string for schedule: <%s>", &env[offset] ) ;
		switch( env[offset] ) {
		  case ',':
		    sctk_nodebug( "There is a chunk size -> <%s>", &env[offset+1] ) ;
		    chunk_size = atoi( &env[offset+1] ) ;
		    if ( chunk_size <= 0 ) {
		      fprintf (stderr,
			  "Warning: Incorrect chunk size within OMP_SCHEDULE variable: <%s>\n", env ) ;
		      chunk_size = 0 ;
		    } else {
		      OMP_MODIFIER_SCHEDULE = chunk_size ;
		    }
		    break ;
		  case '\0':
		    sctk_nodebug( "No chunk size\n" ) ;
		    break ;
		  default:
		  fprintf (stderr,
			   "Syntax error with envorinment variable OMP_SCHEDULE <%s>,"
			   " should be \"static|dynamic|guided|auto[,chunk_size]\"\n", env ) ;
		  exit( 1 ) ;
		}
	      }  else {
		  fprintf (stderr,
			   "Warning: Unknown schedule <%s> (must be static, guided or dynamic),"
			   " fallback to default schedule <%d>\n", env,
			   OMP_SCHEDULE);
	      }
	    }


	  /******* OMP_NUM_THREADS *********/
	  env = getenv ("OMP_NUM_THREADS");
	  OMP_NUM_THREADS = OMP_VP_NUMBER;	/* DEFAULT */
	  if (env != NULL)
	    {
	      nb_threads = atoi (env);
	      if (nb_threads > 0)
		{
		  OMP_NUM_THREADS = nb_threads;
		}
	    }


	  /******* OMP_DYNAMIC *********/
	  env = getenv ("OMP_DYNAMIC");
	  OMP_DYNAMIC = 0;	/* DEFAULT */
	  if (env != NULL)
	    {
	      if (strcmp (env, "true") == 0)
		{
		  OMP_DYNAMIC = 1;
		}
	    }



	  /******* OMP_NESTED *********/
	  env = getenv ("OMP_NESTED");
	  OMP_NESTED = 0;	/* DEFAULT */
	  if (env != NULL)
	    {
	      if (strcmp (env, "1") == 0 || strcmp (env, "TRUE") == 0 || strcmp (env, "true") == 0 )
		{
		  OMP_NESTED = 1;
		}
	    }



	  /******* ADDITIONAL VARIABLES *******/
	  /* NONE */




	  /***** PRINT SUMMARY ******/
	  if ( (getenv ("MPC_DISABLE_BANNER") == NULL) && (sctk_get_process_rank() == 0) ) {
	    fprintf (stderr,
		"MPC OpenMP version %d.%d (DEV)\n",
		SCTK_OMP_VERSION_MAJOR, SCTK_OMP_VERSION_MINOR);
	    fprintf (stderr, "\tOMP_SCHEDULE %d\n", OMP_SCHEDULE);
	    fprintf (stderr, "\tOMP_NUM_THREADS %d\n", OMP_NUM_THREADS);
	    fprintf (stderr, "\tOMP_DYNAMIC %d\n", OMP_DYNAMIC);
	    fprintf (stderr, "\tOMP_NESTED %d\n", OMP_NESTED);
	    fprintf (stderr, "\t%d VPs (OMP_VP_NUMBER)\n", OMP_VP_NUMBER);
	  }

	  done = 1;


	}

      /* Initialize the microthread structure that will be used for the first
       * level of OpenMP multithreading */
      self = sctk_malloc (sizeof (sctk_microthread_t));
      sctk_assert (self != NULL);

      sctk_microthread_init (OMP_VP_NUMBER, self);
      res = sctk_thread_setspecific (sctk_microthread_key, self);
      sctk_assert (res == 0);

      sctk_nodebug( "mpcomp_init: allocation of thread_info size %d and microVP %d"
	  , sizeof( mpcomp_thread_info_t ), sizeof( sctk_microthread_vp_t ) ) ;
     	

      /* Thread-info structure allocation and assignment */
      init_info = sctk_malloc (sizeof (mpcomp_thread_info_t));
      sctk_assert (init_info != NULL);

      icvs.nthreads_var = OMP_NUM_THREADS;
      icvs.dyn_var = OMP_DYNAMIC;
      icvs.nest_var = OMP_NESTED;
      icvs.run_sched_var = OMP_SCHEDULE;
      icvs.modifier_sched_var = OMP_MODIFIER_SCHEDULE ;
      icvs.def_sched_var = "static";
      icvs.nmicrovps_var = OMP_VP_NUMBER;

      __mpcomp_init_thread_info (init_info, NULL, NULL, 0, 1, icvs,
				 0, 0, 0, NULL, -1, self);

      res = sctk_thread_setspecific (mpcomp_thread_info_key, init_info);
      sctk_assert (res == 0);

      sctk_nodebug ("__mpcomp_init: Exit init w/ Lock");

      sctk_thread_mutex_unlock (&lock);

    }
}

void
mpcomp_macro_scheduler (sctk_microthread_vp_t * self, long step)
{
  long i;
  long to_do;
  mpcomp_thread_info_t *info;
  mpcomp_thread_info_t *info_init;

  info_init = (mpcomp_thread_info_t *) self->op_list[step].arg;

  sctk_nodebug( "mpcomp_macro_scheduler[%d]: Enter with context %d", info_init->rank, info_init->context ) ;

  to_do = self->to_do_list;
  i = (step + 1) % to_do;
  info = (mpcomp_thread_info_t *) self->op_list[i].arg;
  /*
     context -> 0 when there is no context related to this microthread (no stack has been created)
     is_running -> 0 when the current thread is blocked due to a lock
     done -> 1 when this microthread has successfully executed
   */
  /* TODO update numbers to macros */
  while ((info->context == 0) || (info->is_running == 0) || (info->done == 1))
    {
      i = (i + 1) % to_do;
      info = (mpcomp_thread_info_t *) self->op_list[i].arg;
      if (step == i)
	{
	  sctk_thread_yield ();
	}
    }
  if (i != step)
    {
      sctk_assert (info->done == 0);
      sctk_assert (info->is_running != 0);
      sctk_assert (info->context != 0);

      sctk_nodebug ("mpcomp_macro_scheduler: Swap to %d", i);

      sctk_swapcontext (&(info_init->uc), &(info->uc));

      sctk_nodebug ("mpcomp_macro_scheduler: Restore Swap to %d", i);

      sctk_thread_setspecific (mpcomp_thread_info_key, info_init);
    }
}

void *
__mpcomp_wrapper_op (void *arg)
{
  mpcomp_thread_info_t *info;
  void *res;

  /* Retrieve and store the OpenMP thread-specific information */
  info = (mpcomp_thread_info_t *) arg;
  sctk_thread_setspecific (mpcomp_thread_info_key, info);

  sctk_nodebug( "__mpcomp_wrapper_op[%d]: Enter with context %d", info->rank, info->context ) ;

  sctk_nodebug ("__mpcomp_wrapper_op: Key for microthread = %d",
		sctk_microthread_key);
  res = sctk_thread_getspecific (sctk_microthread_key);
  sctk_assert (res != NULL);

  if (info->context == 0)
    {
      sctk_hierarchical_tls = info->hierarchical_tls;
    }

  sctk_nodebug
    ("__mpcomp_wrapper_op: Op #%d started on VP %d -  %p->%p (depth:%d)",
     info->step, info->vp, __mpcomp_wrapper_op, info->func, info->depth);


  /* Call the function extracted by OpenMP */
  info->func (info->shared);

  sctk_nodebug ("__mpcomp_wrapper_op: Op #%d done on VP %d", info->step,
		info->vp);

  /* This function is done */
  info->done = 1;

  /* 'context' is updated to 1 when a context is created with a fork from
   * another thread in the same VP (see mpcomp_fork_when_blocked) */
  /* while (info->context == 1) -> previous code */
  if (info->context == 1)
    {
      sctk_microthread_vp_t *my_vp;
      /* Continue to schedule other microthreads on behalf of the main thread */
      my_vp = &(info->task->__list[info->vp]);
      mpcomp_macro_scheduler (my_vp, info->step);
    }

  /* If we've been through dummy func ('context' is updated to 3 inside this
   * function) */
  if (info->context == 3)
    {
      sctk_microthread_vp_t *my_vp;
#warning "TODO to translate"
      /* Pour repasser la main à la pile principale de dummy_func */
      my_vp = &(info->task->__list[info->vp]);
      sctk_nodebug ("__mpcomp_wrapper_op: Restore main (context=3)");
      sctk_setcontext (&(my_vp->vp_context));
    }

  return NULL;
}


/* 
   Start a new parallel region (Compiler ABI).

   The new parallel region will be executed by 'arg_num_threads' thread(s) or
 the default number if 'arg_num_threads' == -1. Each thread will call the
 function 'func' with the argument 'shared'.
 */
void
__mpcomp_start_parallel_region (int arg_num_threads, void *(*func) (void *),
				void *shared)
{
  mpcomp_thread_info_t *current_info;
  int num_threads;

  SCTK_PROFIL_START (__mpcomp_start_parallel_region);

  /* Initialize the OpenMP environment (call several times, but really executed
   * once) */
  __mpcomp_init ();

  /* Retrieve the information (microthread structure and current region) */
  /* TODO Use TLS if available */
  current_info = sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (current_info != NULL);

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
	("__mpcomp_start_parallel_region: Only 1 thread -> call f");
      
      /* Simulate a parallel region by incrementing the depth */
      current_info->depth++ ;

      func (shared);

      current_info->depth-- ;

      SCTK_PROFIL_END (__mpcomp_start_parallel_region);
      return;
    }

  sctk_nodebug
    ("__mpcomp_start_parallel_region: -> Final num threads = %d",
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
	("__mpcomp_start_parallel_region: starting new team at depth %d on %d VP(s)",
	 current_info->depth, current_info->icvs.nmicrovps_var);

      current_task = current_info->task;

      /* Initialize a new microthread structure if needed */
      if (current_info->depth == 0)
	{
	  new_task = current_task;

	  sctk_nodebug
	    ("__mpcomp_start_parallel_region: reusing the thread_info depth 0");
	}
      else
	{

	  sctk_nodebug
	    ("__mpcomp_start_parallel_region: Nesting");

	  /* If the first child has already been created, get the
	   * corresponding thread-info structure */
	  if (current_info->children[0] != NULL)
	    {
	      sctk_nodebug
		("__mpcomp_start_parallel_region: reusing thread_info new depth");

	      new_task = current_info->children[0]->task;
	      sctk_assert (new_task != NULL);

	    }
	  else
	    {

	      sctk_nodebug
		("__mpcomp_start_parallel_region: allocating thread_info new depth");

	      new_task = sctk_malloc (sizeof (sctk_microthread_t));
	      sctk_assert (new_task != NULL);
	      sctk_microthread_init (OMP_VP_NUMBER, new_task);
	    }
	}


      /* Fill the microthread structure with these new threads */
      for (i = num_threads - 1; i >= 0; i--)
	{
	  mpcomp_thread_info_t *new_info;
	  int vp;
	  int val;
	  int res;

	  /* Compute the VP this thread will be scheduled on and the behavior of
	   * 'add_task' */
	  if (i < current_info->icvs.nmicrovps_var)
	    {
	      vp = i;
	      val = MPC_MICROTHREAD_LAST_TASK;
	    }
	  else
	    {
	      vp = i % (current_info->icvs.nmicrovps_var);
	      val = MPC_MICROTHREAD_NO_TASK_INFO;
	    }

	  /* Get the structure of the i-th children */
	  new_info = current_info->children[i];

	  /* If this is the first time that such a child exists
	     -> allocate memory once and initialize once */
	  if (new_info == NULL)
	    {

	      sctk_nodebug
		("__mpcomp_start_parallel_region: Child %d is NULL -> allocating thread_info",
		 i);

	      /* Allocate on the correct NUMA node if the MPC allocator is
	         included */
	      new_info =
		sctk_malloc_on_node (sizeof (mpcomp_thread_info_t), vp);
	      sctk_assert (new_info != NULL);

	      current_info->children[i] = new_info;

	      __mpcomp_init_thread_info (new_info, func, shared, i,
					 num_threads, current_info->icvs,
					 current_info->depth + 1, vp, 0,
					 current_info, 0, new_task);
	    }
	  else
	    {
	      __mpcomp_reset_thread_info (new_info, func, shared, num_threads,
					  current_info->icvs, 0, 0, vp);
	    }


	  new_info->task = new_task;

	  sctk_nodebug
	    ("__mpcomp_start_parallel_region: Adding op %d on VP %d", i, vp);

	  res = sctk_microthread_add_task (__mpcomp_wrapper_op, new_info, vp,
					   &(new_info->step), new_task, val);
	  sctk_assert (res == 0);


	}

      SCTK_PROFIL_END (__mpcomp_start_parallel_region__creation);

      /* Launch the execution of this microthread structure */
      sctk_microthread_parallel_exec (new_task,
				      MPC_MICROTHREAD_DONT_WAKE_VPS);

      sctk_nodebug
	("__mpcomp_start_parallel_region: Microthread execution done");

      /* Restore the key value (microthread structure & OpenMP info) */
      sctk_thread_setspecific (mpcomp_thread_info_key, current_info);
      sctk_thread_setspecific (sctk_microthread_key, current_task);

      /* Free the memory allocated for the new microthread structure */
      /*
      if (current_info->depth != 0)
	{
	  sctk_free (new_task);
	}
	*/

      /* TODO free the memory allocated for the OpenMP-thread info 
         maybe not because this is stored in current_info->children[] */


    }
  else
    {
      mpcomp_thread_info_t *new_info;

      sctk_nodebug
	("__mpcomp_start_parallel_region: Serialize a new team at depth %d",
	 current_info->depth);

      /* TODO only flatterned nested supported for now */

      num_threads = 1;


      new_info = current_info->children[0];
      if (new_info == NULL)
	{

	  sctk_nodebug
	    ("__mpcomp_start_parallel_region: Allocating new thread_info");

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
	    ("__mpcomp_start_parallel_region: Reusing older thread_info");
	  __mpcomp_reset_thread_info (new_info, func, shared, 0,
				      current_info->icvs, 0, 0,
				      current_info->vp);
	}

      __mpcomp_wrapper_op (new_info);

      sctk_nodebug ("__mpcomp_start_parallel_region: after flat nested");

      sctk_thread_setspecific (mpcomp_thread_info_key, current_info);

      /* sctk_free (new_info); */
    }

  /* Re-initializes team info for this thread */
  __mpcomp_reset_team_info (current_info, num_threads);


  /* Restore the TLS for the main thread */
  sctk_hierarchical_tls = current_info->children[0]->hierarchical_tls;

  SCTK_PROFIL_END (__mpcomp_start_parallel_region);
}


/*
 * Update the number of threads that will be used for the next
 * parallel region.
 * If an incorrect number is passed as argument (<=0 or > than the max), the
 * number of threads remain unchanged.
 * (See OpenMP API 2.5 Section 3.2.1)
 */
void
mpcomp_set_num_threads (int num_threads)
{
  mpcomp_thread_info_t *self;

  __mpcomp_init ();

  if (num_threads > 0 && num_threads <= MPCOMP_MAX_THREADS)
    {
      /* TODO use TLS */
      self = (mpcomp_thread_info_t *)
	sctk_thread_getspecific (mpcomp_thread_info_key);
      sctk_assert (self != NULL);
      self->icvs.nthreads_var = num_threads;
    }
}


/*
 * Return the number of threads used in the current team (direct enclosing
 * parallel region).
 * If called from a sequential part of the program, this function returns 1.
 * (See OpenMP API 2.5 Section 3.2.2)
 */
int
mpcomp_get_num_threads (void)
{
  mpcomp_thread_info_t *self;

  __mpcomp_init ();

  /* TODO use TLS */
  self = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  return self->num_threads;
}

/*
 * Return the maximum number of threads that can be used inside a parallel region.
 * This function may be called either from serial or parallel parts of the program.
 * See OpenMP API 2.5 Section 3.2.3
 */
int
mpcomp_get_max_threads (void)
{
  mpcomp_thread_info_t *self;

  __mpcomp_init ();
  /* TODO use TLS */
  self = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (self != NULL);
  return self->icvs.nthreads_var;
}

/*
 * Return the thread number (ID) of the current thread whitin its team.
 * The master thread of each team has a thread ID equals to 0.
 * This function returns 0 if called from a serial part of the code (or from a
 * flatterned nested parallel region).
 * See OpenMP API 2.5 Section 3.2.4
 */
int
mpcomp_get_thread_num (void)
{
  mpcomp_thread_info_t *self;

  __mpcomp_init ();

  /* TODO use TLS */
  self = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  sctk_nodebug ("Rank %d on cpu %d", self->rank, sctk_get_cpu ());

  return self->rank;
}

/*
 * Return the maximum number of processors.
 * See OpenMP API 1.0 Section 3.2.5
 */
int
mpcomp_get_num_procs (void)
{
  mpcomp_thread_info_t *self;

  __mpcomp_init ();

  /* TODO use TLS */
  self = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (self != NULL);

  return self->icvs.nmicrovps_var;
}

/*
 * Check whether the current flow is located inside a parallel region or not.
 * See OpenMP API 2.5 Section 3.2.6
 */
int
mpcomp_in_parallel (void)
{
  mpcomp_thread_info_t *info;
  __mpcomp_init ();
  /* TODO use TLS */
  info = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);
  return (info->depth != 0);
}

/**
  * Set or unset the dynamic adaptation of the thread number.
  * See OpenMP API 2.5 Section 3.1.7
  */
void
mpcomp_set_dynamic (int dynamic_threads)
{
  mpcomp_thread_info_t *info;
  __mpcomp_init ();
  /* TODO use TLS */
  info = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);
  info->icvs.dyn_var = dynamic_threads;
}

/**
  * Retrieve the current dynamic adaptation of the program.
  * See OpenMP API 2.5 Section 3.2.8
  */
int
mpcomp_get_dynamic (void)
{
  mpcomp_thread_info_t *info;
  __mpcomp_init ();
  /* TODO use TLS */
  info = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);
  return info->icvs.dyn_var;
}

/**
  *
  * See OpenMP API 2.5 Section 3.2.9
  */
void
mpcomp_set_nested (int nested)
{
  mpcomp_thread_info_t *info;
  __mpcomp_init ();
  /* TODO use TLS */
  info = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);
  info->icvs.nest_var = nested;
}

/**
  *
  * See OpenMP API 2.5 Section 3.2.10
  */
int
mpcomp_get_nested (void)
{
  mpcomp_thread_info_t *info;
  __mpcomp_init ();
  /* TODO use TLS */
  info = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);
  return info->icvs.nest_var;
}


/**
  *
  * See OpenMP API 3.0 Section 3.2.11
  */
void omp_set_schedule( omp_sched_t kind, int modifier ) {
  mpcomp_thread_info_t *info;

  __mpcomp_init ();

  /* TODO use TLS */
  info = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  info->icvs.run_sched_var = kind ;
  info->icvs.modifier_sched_var = modifier ;
}

/**
  *
  * See OpenMP API 3.0 Section 3.2.12
  */
void omp_get_schedule( omp_sched_t * kind, int * modifier ) {
  mpcomp_thread_info_t *info;

  __mpcomp_init ();

  /* TODO use TLS */
  info = (mpcomp_thread_info_t *)
    sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  *kind = info->icvs.run_sched_var ;
  *modifier = info->icvs.modifier_sched_var ;
}

/* Function called the first time a stack has been created for a specific micro
 * thread */
static void *
mpcomp_macro_wrapper (void *arg)
{
  mpcomp_thread_info_t *info;

  info = (mpcomp_thread_info_t *) arg;

  sctk_nodebug ("mpcomp_macro_wrapper: Entering");

  if (info->done == 0)
    {
      sctk_microthread_vp_t *my_vp;

      my_vp = &(info->task->__list[info->vp]);
      sctk_thread_setspecific (mpcomp_thread_info_key, info);
      sctk_assert (info->context == 1);	/* TODO macro for context==1 */
      info->context = 3;	/* TODO macro for context==3 */

      sctk_nodebug ("mpcomp_macro_wrapper: Will save %d %p", info->vp, my_vp);

      sctk_swapcontext (&(my_vp->vp_context), &(info->uc));
    }

  sctk_nodebug ("mpcomp_macro_wrapper: Leaving");

  return arg;
}

/* Create stacks for other micro thread when the current micro thread is
 * blocked (due to a barrier, mutex, etc) */
void
mpcomp_fork_when_blocked (sctk_microthread_vp_t * self, long step)
{
  long i;
  long to_do;
  mpcomp_thread_info_t *info;

  /* Get the information about the currently-scheduled microthread */
  info = (mpcomp_thread_info_t *) self->op_list[step].arg;

  sctk_nodebug( "mpcomp_fork_when_blocked[%d]: Enter", info->rank ) ;

  /* If the current micro thread has no context (e.g., no stack) */
  /* TODO replace the 'context' w/ macros */
  if (info->context == 0)
    {
      to_do = self->to_do_list;
      for (i = step + 1; i < to_do; i++)
	{
	  int res;

	  info = (mpcomp_thread_info_t *) self->op_list[i].arg;

	  sctk_nodebug
	    ("mpcomp_fork_when_blocked: Fork thread (op:%d) %p->%p", i,
	     self->op_list[i].func, info->func);
	  if (info->stack == NULL)
	    {
	      /* TODO free this stack when the thread_info is re-used? */
	      /* TODO Use stack size specified by the user (OMP 3.0)? */
	      info->stack = sctk_malloc (STACK_SIZE);

	      sctk_nodebug( "mpcomp_fork_when_blocked[%d]: Null stack -> mallocing"
		  , info->rank ) ;
	    }
	  res = sctk_makecontext_hierarchical_tls (&(info->uc),
						   (void *) self->op_list[i].
						   arg,
						   (void (*)(void *)) self->
						   op_list[i].func,
						   info->stack, STACK_SIZE,
						   info->hierarchical_tls);
	  sctk_assert (res == TRUE);

	  /* Context created */
	  info->context = 1;

	  /* Bypass the first call to mpcomp_macro_wrapper
	     The 'real' function will be called by swapping the context to the
	     previously-created one
	   */
	  self->op_list[i].func = MPC_MICROTHREAD_FUNC_T mpcomp_macro_wrapper;
	}

      i = step;
      info = (mpcomp_thread_info_t *) self->op_list[i].arg;
      info->context = 2;	/* TODO update number to macros */

      sctk_getcontext (&(info->uc));
    }
}


/*
   OpenMP barrier.
   All threads of the same team must meet.
   This barrier uses some optimizations for threads inside the same microVP.
 */
void
__mpcomp_barrier (void)
{

  mpcomp_thread_info_t *info;

  __mpcomp_init ();

  /* TODO Use TLS if available */
  info = sctk_thread_getspecific (mpcomp_thread_info_key);

  sctk_nodebug( "__mpcomp_barrier[%d]: Enter", info->rank ) ;

  /* Block only if I'm not the only thread in the team */
  if (info->num_threads > 1)
    {

      sctk_microthread_vp_t *my_vp;
      long micro_vp_barrier;
      long micro_vp_barrier_done;
      int num_omp_threads_micro_vp;

      /* Grab the microVP */
      my_vp = &(info->task->__list[info->vp]);

      /* Get the total number of OpenMP threads on this microVP */
      num_omp_threads_micro_vp = my_vp->to_do_list;

      /* Is there only 1 OpenMP thread on this microVP? */
      if (num_omp_threads_micro_vp == 1)
	{
	  mpcomp_thread_info_t *father;
	  long barrier_done_init;
	  long barrier;

	  /* Grab the father region */
	  father = info->father;

	  /* Update info on the barrier (father region) */
	  sctk_spinlock_lock (&(father->lock2));
	  barrier_done_init = father->barrier_done;
	  barrier = father->barrier + 1;
	  father->barrier = barrier;

	  /* If I am the last thread of the team to enter this barrier */
	  if (barrier == info->num_threads)
	    {
	      father->barrier = 0;
	      father->barrier_done++;
	      sctk_spinlock_unlock (&(father->lock2));
	    }
	  else
	    {
	      sctk_spinlock_unlock (&(father->lock2));

	      /* Wait for the barrier to be done */
	      while (father->barrier_done == barrier_done_init)
		{
		  sctk_thread_yield ();
		}
	    }
	}
      else
	{

	  sctk_nodebug
	    ("__mpcomp_barrier[mVP=%d]: number of threads on this vp: %d",
	     info->vp, num_omp_threads_micro_vp);

	  /* Get the barrier number for this microVP */
	  micro_vp_barrier = my_vp->barrier;
	  micro_vp_barrier_done = my_vp->barrier_done;

	  sctk_nodebug
	    ("__mpcomp_barrier[mVP=%d]: number of threads blocked on this vp: %ld",
	     info->vp, micro_vp_barrier);

	  /* Create a context for the next microthreads on the same VP (only the
	   * first time) */
	  if (micro_vp_barrier == 0)
	    {
	      mpcomp_fork_when_blocked (my_vp, info->step);
	    }

	  /* Increment the barrier counter for this microVP */
	  my_vp->barrier = micro_vp_barrier + 1;

	  if (my_vp->barrier == num_omp_threads_micro_vp)
	    {
	      /* The barrier on this microVP is done, only the barrier between
	         microVPs has to be done */
	      mpcomp_thread_info_t *father;
	      long barrier_done_init;
	      long barrier;

	      sctk_nodebug
		("__mpcomp_barrier[mVP=%d]: barrier done for this microVP",
		 info->vp);

	      /* Grab the father region */
	      father = info->father;

	      sctk_spinlock_lock (&(father->lock2));

	      barrier_done_init = father->barrier_done;
	      barrier = father->barrier + num_omp_threads_micro_vp;
	      father->barrier = barrier;

	      sctk_nodebug
		("__mpcomp_barrier[mVP=%d]: %d threads have entered (out of %d)",
		 info->vp, barrier, info->num_threads);

	      /* If I am the last microVP of the current team to finish this barrier */
	      if (barrier == info->num_threads)
		{
		  father->barrier = 0;
		  father->barrier_done++;
		  sctk_spinlock_unlock (&(father->lock2));
		}
	      else
		{
		  sctk_spinlock_unlock (&(father->lock2));

		  while (father->barrier_done == barrier_done_init)
		    {
		      sctk_thread_yield ();
		    }
		}

	      /* Release each microVP internally */
	      my_vp->barrier = 0;
	      my_vp->barrier_done++;

	    }
	  else
	    {

	      sctk_nodebug
		("__mpcomp_barrier[mVP=%d]: waiting on this microVP",
		 info->vp);

	      /* Spin while the barrier is not done */
	      mpcomp_macro_scheduler (my_vp, info->step);
	      while (my_vp->barrier_done == micro_vp_barrier_done)
		{
		  mpcomp_macro_scheduler (my_vp, info->step);
		  if (my_vp->barrier_done == micro_vp_barrier_done)
		    {
		      sctk_thread_yield ();
		    }
		}
	    }

	}

    }

  sctk_nodebug ("__mpcomp_barrier: Leaving");
}

/*
   OpenMP barrier.
   All threads of the same team must meet
 */
void
__mpcomp_old_barrier (void)
{
  mpcomp_thread_info_t *info;

  __mpcomp_init ();

  /* TODO Use TLS if available */
  info = sctk_thread_getspecific (mpcomp_thread_info_key);
  sctk_assert (info != NULL);

  sctk_nodebug ("__mpcomp_old_barrier[%d]: Entering (on microVP %d)",
		info->rank, info->vp);

  /* Block only if I'm not the only thread in the team */
  if (info->num_threads > 1)
    {
      mpcomp_thread_info_t *father;
      sctk_microthread_vp_t *my_vp;
      long barrier_done_init;
      long barrier;

      /* Grab the father region */
      father = info->father;
      // sctk_assert (father != NULL);

      /* Catch the VP */
      my_vp = &(info->task->__list[info->vp]);

      /* Create a context for the next microthread on the same VP */
      mpcomp_fork_when_blocked (my_vp, info->step);

      // sctk_nodebug ("__mpcomp_barrier: Using %p %d", my_vp, info->vp);

      /* Update info on the barrier (father region) */
      // sctk_thread_mutex_lock (&(father->lock));
      sctk_spinlock_lock (&(father->lock2));
      barrier_done_init = father->barrier_done;
      barrier = father->barrier + 1;
      father->barrier = barrier;

      /* TODO temp to re-initialize internal information about current for dyn */
      info->private_current_for_dyn = -1 ;

      /* father->chunk_info_for_dyn[info->rank][0].remain = -1 ; */

      /* If I am the last thread of the team to enter this barrier */
      if (barrier == info->num_threads)
	{

	  /**********************
	    Release counters related to special parallel constructs 
	   ***********************/


	  /* TODO remove some updates and put them inside the corresponding part
	     e.g., reinitialization of dyn schedule in specialized barrier (?) */
	  __mpcomp_reset_team_info (father, info->num_threads);


	  /**********************
	    Release other threads of the team 
	   ***********************/

	  father->barrier = 0;
	  father->barrier_done++;

	}
      // sctk_thread_mutex_unlock (&(father->lock));
      sctk_spinlock_unlock (&(father->lock2));

      /* Run the macro scheduler overloading the micro one */
      mpcomp_macro_scheduler (my_vp, info->step);
      while (father->barrier_done == barrier_done_init)
	{
	  mpcomp_macro_scheduler (my_vp, info->step);
	  if (father->barrier_done == barrier_done_init)
	    {
	      sctk_thread_yield ();
	    }
	}
    }

  sctk_nodebug ("__mpcomp_barrier: Leaving");
}




/* timing routines */
double
mpcomp_get_wtime (void)
{
  double res;
  struct timeval tp;

  gettimeofday (&tp, NULL);
  res = tp.tv_sec + tp.tv_usec * 0.000001;

  sctk_nodebug ("Wtime = %f", res);
  return res;
}

double
mpcomp_get_wtick (void)
{
  return 10e-6;
}



/* TODO mode this function to an appropriate file */

void __mpcomp_flush() {
  mpcomp_thread_info_t *info;
  sctk_microthread_vp_t *my_vp;

  __mpcomp_init ();

#warning "__mpcomp_flush: need to call mpcomp_macro_scheduler"

  sctk_nodebug( "__mpcomp_flush: entering..." ) ;

  /* TODO Use TLS if available */
  info = sctk_thread_getspecific (mpcomp_thread_info_key);

  /* Grab the microVP */
  my_vp = &(info->task->__list[info->vp]);

  mpcomp_fork_when_blocked (my_vp, info->step);

  sctk_nodebug( "__mpcomp_flush: towards thread_yield" ) ;

  sctk_thread_yield ();
}


void __mpcomp_ordered_begin() {
  mpcomp_thread_info_t *info;
  mpcomp_thread_info_t *team;

  __mpcomp_init ();

  /* TODO Use TLS if available */
  info = sctk_thread_getspecific (mpcomp_thread_info_key);

  team = info->father ;

  /* First iteration of the loop -> initialize 'next_ordered_offset' */
  if ( info->current_ordered_iteration == info->loop_lb ) {
    team->next_ordered_offset = 0 ;
  } else {
    /* Do we have to wait for the right iteration? */
    if ( info->current_ordered_iteration != 
	(info->loop_lb + info->loop_incr * team->next_ordered_offset) ) {
      sctk_microthread_vp_t *my_vp;

      sctk_nodebug( "__mpcomp_ordered_begin[%d]: Waiting to schedule iteration %d",
	  info->rank, info->current_ordered_iteration ) ;

      /* Grab the microVP */
      my_vp = &(info->task->__list[info->vp]);

      mpcomp_fork_when_blocked (my_vp, info->step);

      /* Spin while the condition is not satisfied */
      mpcomp_macro_scheduler (my_vp, info->step);
      while ( info->current_ordered_iteration != 
	  (info->loop_lb + info->loop_incr * team->next_ordered_offset) ) {
	mpcomp_macro_scheduler (my_vp, info->step);
	if ( info->current_ordered_iteration != 
	    (info->loop_lb + info->loop_incr * team->next_ordered_offset) ) {
	  sctk_thread_yield ();
	}
      }
    }
  }

  sctk_nodebug( "__mpcomp_ordered_begin[%d]: Allowed to schedule iteration %d",
      info->rank, info->current_ordered_iteration ) ;
}

void __mpcomp_ordered_end() {
  mpcomp_thread_info_t *info;
  mpcomp_thread_info_t *team;

  /* TODO Use TLS if available */
  info = sctk_thread_getspecific (mpcomp_thread_info_key);

  team = info->father ;

  info->current_ordered_iteration += info->loop_incr ;
  if ( (info->loop_incr > 0 && info->current_ordered_iteration >= info->loop_b) ||
      (info->loop_incr < 0 && info->current_ordered_iteration <= info->loop_b) ) {
    team->next_ordered_offset = -1 ;
  } else {
    team->next_ordered_offset++ ;
  }

}
