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


void
mpcomp_macro_scheduler (sctk_microthread_vp_t * self, long step)
{
  not_implemented();
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
  not_implemented();
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
      sctk_extls = info->extls;
    }

  sctk_nodebug
    ("__mpcomp_wrapper_op: Op #%d started on VP %d -  %p->%p (depth:%d)",
     info->step, info->vp, __mpcomp_wrapper_op, info->func, info->depth);

  sctk_nodebug("I am on cpu %d",sctk_get_cpu());
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
 * Update the number of threads that will be used for the next
 * parallel region.
 * If an incorrect number is passed as argument (<=0 or > than the max), the
 * number of threads remain unchanged.
 * (See OpenMP API 2.5 Section 3.2.1)
 */
void
mpcomp_set_num_threads (int num_threads)
{
  not_implemented();
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

#if 0
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
#endif

/*
 * Return the maximum number of threads that can be used inside a parallel region.
 * This function may be called either from serial or parallel parts of the program.
 * See OpenMP API 2.5 Section 3.2.3
 */
#if 0
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
#endif 

#if 0
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
#endif 

/*
 * Return the maximum number of processors.
 * See OpenMP API 1.0 Section 3.2.5
 */
int
mpcomp_get_num_procs (void)
{
  mpcomp_thread_info_t *self;

  not_implemented();
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
  not_implemented();
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
  not_implemented();
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
  not_implemented();
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
  not_implemented();
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
  not_implemented();
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

  not_implemented();
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

  not_implemented();
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
  not_implemented();
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

  not_implemented();
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
	  res = sctk_makecontext_extls (&(info->uc),
				        (void *) self->op_list[i].
				        arg,
				        (void (*)(void *)) self->
				        op_list[i].func,
				        info->stack, STACK_SIZE,
				        info->extls);
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
__rename_mpcomp_barrier (void)
{

  not_implemented();
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

  not_implemented();
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

  return ;
}


#if 0
void __mpcomp_ordered_begin() {
  mpcomp_thread_info_t *info;
  mpcomp_thread_info_t *team;

  not_implemented();
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

  not_implemented();
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
#endif
