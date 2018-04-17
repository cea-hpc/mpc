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

#include "mpcomp_types_def.h"

#include "sctk_bool.h"

#if MPCOMP_TASK

#include <sctk_int.h>
#include <sctk_asm.h>
#include "mpcomp_types.h"
#include "sctk_runtime_config_struct.h"
#include "sctk_debug.h"
#include "mpcomp_task.h"
#include "mpcomp_task_tree.h"
#include "mpcomp_task_macros.h"
#include "mpcomp_task_list.h"
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_stealing.h"
#include "sctk_atomics.h"
#include "mpcomp_task_dep.h"
#include "mpcomp_taskgroup.h"
#include "mpcomp_core.h" /* mpcomp_init */

#include "ompt.h"
#include "mpcomp_ompt_general.h"
#include<mpi.h>
extern ompt_callback_t* OMPT_Callbacks;

//#define MPC_OPENMP_PERF_TASK_COUNTERS
 
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
  static sctk_atomics_int __private_perf_call_steal = SCTK_ATOMICS_INT_T_INIT(0);
  static sctk_atomics_int __private_perf_success_steal = SCTK_ATOMICS_INT_T_INIT(0);
  static sctk_atomics_int __private_perf_create_task = SCTK_ATOMICS_INT_T_INIT(0);
  static sctk_atomics_int __private_perf_executed_task = SCTK_ATOMICS_INT_T_INIT(0);
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */

/**
 * Execute a specific task by the current thread.
 *
 * \param task Target task to execute by the current thread
 */
static void 
__mpcomp_task_execute(mpcomp_task_t *task) 
{
  mpcomp_thread_t *thread;
  mpcomp_local_icv_t saved_icvs;
  mpcomp_task_t *saved_current_task;

  sctk_assert(task);

  /* Retrieve the information (microthread structure and current region) */
  sctk_assert(sctk_openmp_thread_tls);
  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  /* A task can't be executed by multiple threads */
  sctk_assert(task->thread == NULL);

  /* Update task owner */
  task->thread = thread;

  /* Saved thread current task */
  saved_current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);

#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
   sctk_atomics_incr_int( &__private_perf_executed_task );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */

#if OMPT_SUPPORT
   if( mpcomp_ompt_is_enabled() )
   {
	   if( OMPT_Callbacks )
	   {
		   ompt_callback_task_schedule_t callback;
		   callback = (ompt_callback_task_schedule_t) OMPT_Callbacks[ompt_callback_task_schedule];
		   if( callback )
			   callback( &(saved_current_task->ompt_task_data), ompt_task_yield, &(task->ompt_task_data)); 
	   }
   }
#endif /* OMPT_SUPPORT */

  /* Update thread current task */
  MPCOMP_TASK_THREAD_SET_CURRENT_TASK(thread, task);

  /* Saved thread icv environnement */
  saved_icvs = thread->info.icvs;

  /* Get task icv environnement */
  thread->info.icvs = task->icvs;

  /* Execute task */
  task->func(task->data);

  /* Restore thread icv envionnement */
  thread->info.icvs = saved_icvs;

#if OMPT_SUPPORT
  if( mpcomp_ompt_is_enabled() )
  {
    if( OMPT_Callbacks )
	 {
		ompt_callback_task_schedule_t callback;
		callback = (ompt_callback_task_schedule_t) OMPT_Callbacks[ompt_callback_task_schedule];
		if( callback )
			callback( &(task->ompt_task_data), ompt_task_complete, &(saved_current_task->ompt_task_data) ); 
    }
  }
#endif /* OMPT_SUPPORT */

  /* Restore current task */
  MPCOMP_TASK_THREAD_SET_CURRENT_TASK(thread, saved_current_task);

  /* Reset task owner for implicite task */
  task->thread = NULL;
}

void 
mpcomp_task_ref_parent_task(mpcomp_task_t *task) 
{
  sctk_assert(task);

  if (!(task->parent))
    return;

  sctk_atomics_incr_int(&(task->refcount));
  sctk_atomics_incr_int(&(task->parent->refcount));
}

void 
mpcomp_task_unref_parent_task(mpcomp_task_t *task) 
{
  bool no_more_ref;
  mpcomp_task_t *mother, *swap_value;
  sctk_assert(task);
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  mother = task->parent;
  no_more_ref = (sctk_atomics_fetch_and_decr_int(&(task->refcount)) == 1);
  while (mother && no_more_ref) // FREE MY TASK AND CLIMB TREE
  {
    /* Try to store task to reuse it after and avoid alloc overhead */
    if(thread->task_infos.nb_reusable_tasks < MPCOMP_NB_REUSABLE_TASKS && !task->is_stealed && thread->task_infos.one_list_per_thread && task->task_size == thread->task_infos.max_task_tot_size ) 
    {
      thread->task_infos.reusable_tasks[thread->task_infos.nb_reusable_tasks] = task;
      thread->task_infos.nb_reusable_tasks ++;
    }

    else
    {
      sctk_free(task);
    }
    task = mother;
    mother = task->parent;
    no_more_ref = (sctk_atomics_fetch_and_decr_int(&(task->refcount)) == 1);
  }

  if (!mother && no_more_ref) // ROOT TASK
  {
    sctk_atomics_decr_int(&(task->refcount));
  } 
}

/**
 * Allocate a new task with provided information.
 *
 * The new task may be delayed, so copy arguments in a buffer 
 *
 * \param fn Function pointer containing the task body
 * \param data Argument to pass to the previous function pointer
 * \param cpyfn 
 * \param arg_size
 * \param arg_align
 * \param if_clause
 * \param flags 
 * \param deps_num 
 *
 * \return New allocated task (or NULL if an error occured)
 */
mpcomp_task_t *
__mpcomp_task_alloc(void (*fn)(void *), void *data,
			  void (*cpyfn)(void *, void *),
			  long arg_size, long arg_align,
			  bool if_clause, unsigned flags,
			  int deps_num) 
{
  sctk_assert(sctk_openmp_thread_tls);
  mpcomp_thread_t *t = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  /* default pading */
  const long align_size = (arg_align == 0) ? 8 : arg_align ;

  // mpcomp_task + arg_size
  const long mpcomp_task_info_size =
      mpcomp_task_align_single_malloc(sizeof(mpcomp_task_t),
		      align_size );
  const long mpcomp_task_data_size =
      mpcomp_task_align_single_malloc(arg_size,
		      align_size );

  /* Compute task total size */
  long mpcomp_task_tot_size = mpcomp_task_info_size;

  sctk_assert(MPCOMP_OVERFLOW_SANITY_CHECK(mpcomp_task_tot_size,
                                           mpcomp_task_data_size));
  mpcomp_task_tot_size += mpcomp_task_data_size;
  mpcomp_task_t *new_task;

  /* Reuse another task if we can to avoid alloc overhead */
	if(t->task_infos.one_list_per_thread)
	{
		if(mpcomp_task_tot_size > t->task_infos.max_task_tot_size)
    /* Tasks stored too small, free them */
		{
			t->task_infos.max_task_tot_size = mpcomp_task_tot_size;
			int i;
			for(i=0;i<t->task_infos.nb_reusable_tasks;i++) 
      {
				sctk_free(t->task_infos.reusable_tasks[i]);
				t->task_infos.reusable_tasks[i] = NULL;
      }
			t->task_infos.nb_reusable_tasks = 0;
		}

		if(t->task_infos.nb_reusable_tasks == 0) 
		{
			new_task = mpcomp_alloc( t->task_infos.max_task_tot_size);
		}
    /* Reuse last task stored */
		else
		{
	  	sctk_assert(t->task_infos.reusable_tasks[t->task_infos.nb_reusable_tasks-1]);
			new_task = (mpcomp_task_t*) t->task_infos.reusable_tasks[t->task_infos.nb_reusable_tasks -1];
			t->task_infos.nb_reusable_tasks --;
		}
	}    
	else
	{
		new_task = mpcomp_alloc( mpcomp_task_tot_size );
	}
  sctk_assert(new_task != NULL);

  void *task_data = (arg_size > 0)
                        ? (void *)((uintptr_t)new_task + mpcomp_task_info_size)
                        : NULL;

  /* Create new task */
  __mpcomp_task_infos_init(new_task, fn, task_data, t);

#ifdef MPCOMP_USE_TASKDEP
if (deps_num !=0) 
{
  new_task->task_dep_infos = sctk_malloc(sizeof(mpcomp_task_dep_task_infos_t));
  sctk_assert(new_task->task_dep_infos);
  memset(new_task->task_dep_infos, 0, sizeof(mpcomp_task_dep_task_infos_t));
}
#endif /* MPCOMP_USE_TASKDEP */
if(new_task->parent->func == NULL)
  mpcomp_task_set_property(&(new_task->property), MPCOMP_TASK_TIED);

  if (arg_size > 0) {
    if (cpyfn) {
      cpyfn(task_data, data);
    } else {
      memcpy(task_data, data, arg_size);
    }
  }

  /* If its parent task is final, the new task must be final too */
  if (mpcomp_task_is_final(flags, new_task->parent)) {
    mpcomp_task_set_property(&(new_task->property), MPCOMP_TASK_FINAL);
  }

  /* taskgroup */
  mpcomp_taskgroup_add_task(new_task);
  mpcomp_task_ref_parent_task(new_task);
  new_task->is_stealed = false;
  new_task->task_size = t->task_infos.max_task_tot_size;
  if(new_task->depth % MPCOMP_TASKS_DEPTH_JUMP == 2) new_task->far_ancestor = new_task->parent;
  else new_task->far_ancestor = new_task->parent->far_ancestor;

  #if OMPT_SUPPORT
  if( mpcomp_ompt_is_enabled() )
  {
    if( OMPT_Callbacks )
	 {
		ompt_callback_task_create_t callback;
		const void* code_ra = __builtin_return_address(0);
		callback = (ompt_callback_task_create_t) OMPT_Callbacks[ompt_callback_task_create];
		if( callback )
			callback( &(new_task->parent->ompt_task_data), 0, &( new_task->ompt_task_data ), ompt_task_explicit, deps_num > 0, code_ra ); 
    }
  }
  #endif /* OMPT_SUPPORT */

  return new_task;
}

/**
 * Try to find a list to put the task (if needed).
 *
 * \param thread OpenMP thread that will perform the actions
 * \param if_clause True if an if clause evaluated to 'true' 
 * has been provided by the user
 * \return The list where this task can be inserted or NULL ortherwise
 */
static mpcomp_task_list_t *
__mpcomp_task_try_delay(mpcomp_thread_t * thread, bool if_clause) 
{
  mpcomp_task_list_t *mvp_task_list = NULL;

  /* Is Serialized Task */
  if (!if_clause || mpcomp_task_no_deps_is_serialized(thread)) {
    return NULL;
  }
  /* Reserved place in queue */
  assert(thread->mvp != NULL);
  const int node_rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK(
      thread->mvp, MPCOMP_TASK_TYPE_NEW);
  assert(node_rank >= 0);

  // No list for a mvp should be an error ??
  if (!(mvp_task_list =
            mpcomp_task_get_list(node_rank, MPCOMP_TASK_TYPE_NEW))) {
    abort();
    return NULL;
  }

  const int __max_delayed = sctk_runtime_config_get()->modules.openmp.mpcomp_task_max_delayed;

   //Too much delayed tasks
  if (sctk_atomics_fetch_and_incr_int(&(mvp_task_list->nb_elements)) >=
      __max_delayed) {
    sctk_atomics_decr_int(&(mvp_task_list->nb_elements));
    return NULL;
  }

  return mvp_task_list;
}

/**
 * Process a newly allocated initialized task.
 *
 * \param new_task
 * \param if_clause
 */

void mpcomp_task_list_push (mpcomp_task_list_t *mvp_task_list,mpcomp_task_t *new_task)
{
#if MPCOMP_USE_LOCKFREE_QUEUE
    mpcomp_task_list_pushtotail(mvp_task_list, new_task);
#else
    mpcomp_task_list_pushtohead(mvp_task_list, new_task);
#endif
}

mpcomp_task_t * mpcomp_task_list_pop (mpcomp_task_list_t *mvp_task_list, bool is_stealing, bool one_list_per_thread)
{
#if MPCOMP_USE_LOCKFREE_QUEUE
  if (is_stealing)
  {
    return mpcomp_task_list_popfromhead(mvp_task_list);
  }
  else
  {
    /* For lockfree queue, pop from tail (same side that when pushing) when thread is not stealing and there is one list per thread to avoid big call stacks */
   
    if(one_list_per_thread)
    {
      return mpcomp_task_list_popfromtail(mvp_task_list);
    }
    else
    {
      return mpcomp_task_list_popfromhead(mvp_task_list);
    }
  }

#else
    return mpcomp_task_list_popfromhead(mvp_task_list);    
#endif
    
}

void 
__mpcomp_task_process(mpcomp_task_t *new_task, bool if_clause) 
{
  mpcomp_thread_t *thread;
  mpcomp_task_list_t *mvp_task_list;

  /* Retrieve the information (microthread structure and current region) */
  sctk_assert(sctk_openmp_thread_tls);
  thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
   sctk_atomics_incr_int( &__private_perf_create_task );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */

  mvp_task_list = __mpcomp_task_try_delay(thread, if_clause);

  /* If possible, push the task in the list of new tasks */
  if (mvp_task_list) {
    mpcomp_task_list_producer_lock(mvp_task_list, thread->task_infos.opaque);
    mpcomp_task_list_push(mvp_task_list, new_task);
    mpcomp_task_list_producer_unlock(mvp_task_list, thread->task_infos.opaque);
    return;
  }

  /* Otherwise, execute directly this task */
  mpcomp_task_set_property(&(new_task->property), MPCOMP_TASK_UNDEFERRED);

  __mpcomp_task_execute(new_task);

  mpcomp_taskgroup_del_task(new_task);

#ifdef MPCOMP_USE_TASKDEP
  __mpcomp_task_finalize_deps(new_task);
#endif /* MPCOMP_USE_TASKDEP */

  mpcomp_task_unref_parent_task(new_task);

}

/*
 * Creation of an OpenMP task.
 * 
 * This function can be called when encountering an 'omp task' construct
 *
 * \param fn
 * \param data
 * \param cpyfn
 * \param arg_size
 * \param arg_align
 * \param if_clause
 * \param flags
 */
void 
__mpcomp_task(void (*fn)(void *), void *data,
		void (*cpyfn)(void *, void *), long arg_size, long arg_align,
		bool if_clause, unsigned flags) 
{
  mpcomp_task_t *new_task;

  /* Intialize the OpenMP environnement (if needed) */
  __mpcomp_init();

  /* Allocate the new task w/ provided information */
  new_task = __mpcomp_task_alloc(fn, data, cpyfn, arg_size, arg_align,
                                 if_clause, flags, 0 /* no deps */);

  /* Process this new task (put inside a list or direct execution) */
  __mpcomp_task_process(new_task, if_clause);
}

/**
 * Try to steal a task inside a target list.
 *
 * \param list Target list where to look for a task
 * \return the stolen task or NULL if failed
 */
static mpcomp_task_t *
mpcomp_task_steal(mpcomp_task_list_t *list,int rank, int victim) 
{
    mpcomp_thread_t* thread ;
    mpcomp_task_t* current_task ;
    struct mpcomp_task_s *task = NULL;
    mpcomp_generic_node_t* gen_node;

    /* Check input argument */
    sctk_assert(list != NULL);

    /* Get current OpenMP thread */
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert( thread ) ;

    
    /* Get the current task executed by OpenMP thread */
    current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(thread);

#ifdef MPCOMP_USE_MCS_LOCK 
     mpcomp_task_list_consummer_lock(list, thread->task_infos.opaque);
#else /* MPCOMP_USE_MCS_LOCK */
    if( mpcomp_task_list_consummer_trylock(list, thread->task_infos.opaque))
        return NULL;
#endif /* MPCOMP_USE_MCS_LOCK */

    task = mpcomp_task_list_pop(list, true, thread->task_infos.one_list_per_thread);    
    if(task)
    {
      gen_node = &(thread->instance->tree_array[victim] );
      if( gen_node->type == MPCOMP_CHILDREN_LEAF )
      {
        MPCOMP_TASK_MVP_SET_LAST_STOLEN_TASK_LIST(thread->mvp, 0, list);
        thread->mvp->task_infos.lastStolen_tasklist_rank[0] = victim;
        gen_node->ptr.mvp->task_infos.last_thief = rank;
      }
    }
    mpcomp_task_list_consummer_unlock(list, thread->task_infos.opaque);
    return task;
}

/* Return the ith victim for task stealing initiated from element at
 * 'globalRank' */
int mpcomp_task_get_victim(int globalRank, int index,
                                         mpcomp_tasklist_type_t type) {
  int victim;

  /* Retrieve the information (microthread structure and current region) */
  sctk_assert(sctk_openmp_thread_tls);
  mpcomp_thread_t *thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;

  sctk_assert(thread->instance);
  sctk_assert(thread->instance->team);
  mpcomp_team_t *team = thread->instance->team;

  const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE(team);

  switch (larcenyMode) {

  case MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL:
    victim = mpcomp_task_get_victim_hierarchical(globalRank, index, type);
    break;

  case MPCOMP_TASK_LARCENY_MODE_RANDOM:
    victim = mpcomp_task_get_victim_random(globalRank, index, type);
    break;

  case MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER:
    victim = mpcomp_task_get_victim_random_order(globalRank, index, type);
    break;

  case MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN:
    victim = mpcomp_task_get_victim_roundrobin(globalRank, index, type);
    break;

  case MPCOMP_TASK_LARCENY_MODE_PRODUCER:
    victim = mpcomp_task_get_victim_producer(globalRank, index, type);
    break;

  case MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER:
    victim = mpcomp_task_get_victim_producer_order(globalRank, index, type);
    break;

  case MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL_RANDOM:
    victim = mpcomp_task_get_victim_hierarchical_random(globalRank, index, type);
    break;

  default:
    victim = mpcomp_task_get_victim_default(globalRank, index, type);
    break;
  }

  return victim;
}

/* Look in new and untied tasks lists of others */
static struct mpcomp_task_s *__mpcomp_task_larceny(void) {
  int i, type;
  mpcomp_thread_t *thread;
  struct mpcomp_task_s *task = NULL;
  struct mpcomp_mvp_s *mvp, *target;
  struct mpcomp_team_s *team;
  struct mpcomp_task_list_s *list;

#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
  sctk_atomics_incr_int( &__private_perf_call_steal );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */

  /* Retrieve the information (microthread structure and current region) */
  sctk_assert(sctk_openmp_thread_tls);
  thread = (mpcomp_thread_t *) sctk_openmp_thread_tls;

  mvp = thread->mvp;
  sctk_assert(mvp);

  sctk_assert(thread->instance);
  team = thread->instance->team;
  sctk_assert(team);
  mpcomp_generic_node_t* gen_node;

  const int larcenyMode = MPCOMP_TASK_TEAM_GET_TASK_LARCENY_MODE(team);
  const int isMonoVictim = (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM ||
      larcenyMode == MPCOMP_TASK_LARCENY_MODE_PRODUCER);

  /* Check first for NEW tasklists, then for UNTIED tasklists */
  for (type = 0; type <= MPCOMP_TASK_TYPE_NEW; type++) {

    /* Retrieve informations about task lists */
    int tasklistDepth = MPCOMP_TASK_TEAM_GET_TASKLIST_DEPTH(team, type);
    int nbTasklists = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];

    while(nbTasklists == 0)
    {
      tasklistDepth --;
      nbTasklists = (!tasklistDepth) ? 1 : thread->instance->tree_nb_nodes_per_depth[tasklistDepth];
    }

    /* If there are task lists in which we could steal tasks */
    int rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK(mvp, type);
    int victim;
    if ( nbTasklists > 1) {

      /* Try to steal inside the last stolen list*/
      if ((list = MPCOMP_TASK_MVP_GET_LAST_STOLEN_TASK_LIST(mvp, type))) {
        victim = mvp->task_infos.lastStolen_tasklist_rank[0];
        if (task = mpcomp_task_steal(list,rank,victim)) {
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
          sctk_atomics_incr_int( &__private_perf_success_steal );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
          return task;
        }
      }

      /* Try to steal to the last thread that steal a task to current thread */
      if (mvp->task_infos.last_thief != -1 && (list = mpcomp_task_get_list(mvp->task_infos.last_thief,type))) {
        victim = mvp->task_infos.last_thief;
        if (task = mpcomp_task_steal(list,rank,victim)) {
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
          sctk_atomics_incr_int( &__private_perf_success_steal );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
          return task;
        }
        mvp->task_infos.last_thief = -1;
      }


      /* Get the rank of the ancestor containing the task list */
      int nbVictims = (isMonoVictim) ? 1 : nbTasklists ;
      /* Look for a task in all victims lists */
      for (i = 1; i < nbVictims+1; i++) {
        victim = mpcomp_task_get_victim(rank, i, type);
        if(victim != rank) {
          list = mpcomp_task_get_list(victim, type);
          if(list) task = mpcomp_task_steal(list,rank, victim);
          if(task) {
#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
            sctk_atomics_incr_int( &__private_perf_success_steal );
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
            return task;
          } 
          }
        }

    }
  }

  return NULL;
}

/*
 * Schedule remaining tasks in the different task lists (tied, untied, new).
 *
 * Called at several schedule points :
 *     - in taskyield regions
 *     - in taskwait regions
 *     - in implicit and explicit barrier regions
 *
 * \param thread
 * \param mvp
 * \param team
 */

static void 
__internal_mpcomp_task_schedule( mpcomp_thread_t* thread, mpcomp_mvp_t* mvp, mpcomp_team_t* team ) 
{
    int type;
    mpcomp_task_t *task, *current_task;
    mpcomp_task_list_t* list;
    /* All argments must be non null */
    sctk_assert( thread && mvp && team);

    /*  If only one thread is running, tasks are not delayed. 
        No need to schedule                                    */
    if (thread->info.num_threads == 1) 
    {
        sctk_nodebug("sequential or no task");
        return;
    }

    /*    Executed once per thread    */
    current_task = thread->task_infos.current_task;
    
    for (type = 0, task = NULL; !task && type <= MPCOMP_TASK_TYPE_NEW; type++) 
    {
        const int node_rank = MPCOMP_TASK_MVP_GET_TASK_LIST_NODE_RANK(mvp, type);
        list = mpcomp_task_get_list(node_rank, type);
        sctk_assert(list);
#ifdef MPCOMP_USE_MCS_LOCK 
        mpcomp_task_list_consummer_lock(list, thread->task_infos.opaque);
#else /* MPCOMP_USE_MCS_LOCK */
        if( mpcomp_task_list_consummer_trylock(list, thread->task_infos.opaque))
           continue;
#endif /* MPCOMP_USE_MCS_LOCK */
        task = mpcomp_task_list_pop(list, false, thread->task_infos.one_list_per_thread);
        mpcomp_task_list_consummer_unlock(list,  thread->task_infos.opaque);
    }

    /* If no task found previously, try to thieve a task somewhere */
    if (task == NULL) {
        task = __mpcomp_task_larceny();
        if(task) task->is_stealed=true;
    }

    /* All tasks lists are empty, so exit task scheduling function */
    if (task == NULL) return;

    __mpcomp_task_execute(task);

    /* Clean function */
    mpcomp_taskgroup_del_task(task);

#ifdef MPCOMP_USE_TASKDEP
    __mpcomp_task_finalize_deps(task);
#endif /* MPCOMP_USE_TASKDEP */

    mpcomp_task_unref_parent_task(task);
}

/**
 * Perform a taskwait construct.
 *
 * Can be the entry point for a compiler.
 */
void 
mpcomp_taskwait(void) 
{
  mpcomp_task_t *current_task = NULL;     /* Current task execute */
  mpcomp_thread_t *omp_thread_tls = NULL; /* thread private data  */

  omp_thread_tls = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(omp_thread_tls);

  sctk_assert(omp_thread_tls->info.num_threads > 0);

  /* Perform this construct only with multiple threads
   * (with one threads, tasks are schedulded directly,
   * therefore taskwait has no effect)
   */
  if (omp_thread_tls->info.num_threads > 1) {
    current_task = MPCOMP_TASK_THREAD_GET_CURRENT_TASK(omp_thread_tls);
    sctk_assert(
        current_task); // Fail if tasks disable...(from full barrier call)

    /* Look for a children tasks list */
    while (sctk_atomics_load_int(&(current_task->refcount)) != 1) {

	    /* Schedule any other task 
	     * prevent recursive calls to mpcomp_taskwait with argument 0 */
      mpcomp_task_schedule(0);
    }
  }

#ifdef MPC_OPENMP_PERF_TASK_COUNTERS
    const int a = sctk_atomics_load_int( &__private_perf_call_steal );
    const int b = sctk_atomics_load_int( &__private_perf_success_steal );
    const int c = sctk_atomics_load_int( &__private_perf_create_task );
    const int d = sctk_atomics_load_int( &__private_perf_executed_task );
    if( 1 && !omp_thread_tls->rank )
       fprintf(stderr, "try steal : %d - success steal : %d -- total tasks : %d -- performed tasks : %d\n", a,b,c,d);
#endif /* MPC_OPENMP_PERF_TASK_COUNTERS */
}

/**
 * Task scheduling.
 *
 * Try to find a task to be scheduled and execute it.
 * This is the main function (it calls an internal function).
 *
 * \param[in] need_taskwait   True if it is necessary to perform a taskwait
 *                        after scheduling some tasks.
 */
void 
mpcomp_task_schedule( int need_taskwait )
{
    mpcomp_mvp_t *mvp;
    mpcomp_team_t *team;
    mpcomp_thread_t *thread;


    /* Get thread from current tls */
    thread = (mpcomp_thread_t *)sctk_openmp_thread_tls;
    sctk_assert( thread );

    /* Get mvp father from thread */
    mvp = thread->mvp;

    /* Sequential execution => no delayed task */
    if( !mvp ) return;

    /* Get team from thread */
    sctk_assert( thread->instance );
    team = thread->instance->team;
    sctk_assert( team );

    __internal_mpcomp_task_schedule( thread, mvp, team ); 

    /* schedule task can produce task ... */
    if ( need_taskwait )
	    mpcomp_taskwait();
    
    return;
}


/**
 * Taskyield construct.
 *
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void 
mpcomp_taskyield(void) 
{ 
	/* Actually, do nothing */
}

void 
__mpcomp_task_coherency_entering_parallel_region() 
{
#if 0
  struct mpcomp_team_s *team;
  struct mpcomp_mvp_s **mvp;
  mpcomp_thread_t *t;
  mpcomp_thread_t *lt;
  int i, nb_mvps;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->children_instance->nb_mvps > 1) {
    team = t->children_instance->team;
    sctk_assert(team != NULL);

    /* Check team tasking cohenrency */
    sctk_debug("__mpcomp_task_coherency_entering_parallel_region: "
               "Team init %d and tasklist init %d with %d nb_tasks\n",
               sctk_atomics_load_int(&team->tasking_init_done),
               sctk_atomics_load_int(&team->tasklist_init_done),
               sctk_atomics_load_int(&team->nb_tasks));

    /* Check per thread task system coherency */
    mvp = t->children_instance->mvps;
    sctk_assert(mvp != NULL);
    nb_mvps = t->children_instance->nb_mvps;

    for (i = 0; i < nb_mvps; i++) {
      lt = &mvp[i]->threads[0];
      sctk_assert(lt != NULL);

      sctk_debug("__mpcomp_task_coherency_entering_parallel_region: "
                 "Thread in mvp %d init %d in implicit task %p\n",
                 mvp[i]->rank, lt->tasking_init_done, lt->current_task);

      sctk_assert(mpcomp_task_property_isset(lt->current_task->property,
                                             MPCOMP_TASK_IMPLICIT) != 0);
    }
  }
#endif
}

void 
__mpcomp_task_coherency_ending_parallel_region() 
{
#if 0
  struct mpcomp_team_s *team;
  struct mpcomp_mvp_s **mvp;
  mpcomp_thread_t *t;
  mpcomp_thread_t *lt;
  int i_task, i, nb_mvps;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->children_instance->nb_mvps > 1) {
    team = t->children_instance->team;
    sctk_assert(team != NULL);

    /* Check team tasking cohenrency */
    sctk_debug("__mpcomp_task_coherency_ending_parallel_region: "
               "Team init %d and tasklist init %d with %d nb_tasks\n",
               sctk_atomics_load_int(&team->tasking_init_done),
               sctk_atomics_load_int(&team->tasklist_init_done),
               sctk_atomics_load_int(&team->nb_tasks));

    /* Check per thread and mvp task system coherency */
    mvp = t->children_instance->mvps;
    sctk_assert(mvp != NULL);
    nb_mvps = t->children_instance->nb_mvps;

    for (i = 0; i < nb_mvps; i++) {
      lt = &mvp[i]->threads[0];

      sctk_debug("__mpcomp_task_coherency_ending_parallel_region: "
                 "Thread %d init %d in implicit task %p\n",
                 lt->rank, lt->tasking_init_done, lt->current_task);

      sctk_assert(lt->current_task != NULL);
      sctk_assert(lt->current_task->children == NULL);
      sctk_assert(lt->current_task->list == NULL);
      sctk_assert(lt->current_task->depth == 0);

      if (lt->tasking_init_done) {
        if (mvp[i]->threads[0].tied_tasks)
          sctk_assert(mpcomp_task_list_isempty(mvp[i]->threads[0].tied_tasks) ==
                      1);

        for (i_task = 0; i_task < MPCOMP_TASK_TYPE_COUNT; i_task++) {
          if (mvp[i]->tasklist[i_task]) {
            sctk_assert(mpcomp_task_list_isempty(mvp[i]->tasklist[i_task]) ==
                        1);
          }
        }
      }
    }
  }
#endif
}

void 
__mpcomp_task_coherency_barrier() 
{
#if 0
  mpcomp_thread_t *t;
  struct mpcomp_task_list_s *list = NULL;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  sctk_debug("__mpcomp_task_coherency_barrier: "
             "Thread %d exiting barrier in implicit task %p\n",
             t->rank, t->current_task);

  sctk_assert(t->current_task != NULL);
  sctk_assert(t->current_task->children == NULL);
  sctk_assert(t->current_task->list == NULL);
  sctk_assert(t->current_task->depth == 0);

  if (t->tasking_init_done) {
    /* Check tied tasks list */
    sctk_assert(mpcomp_task_list_isempty(t->tied_tasks) == 1);

    /* Check untied tasks list */
    list =
        mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED],
                             MPCOMP_TASK_TYPE_UNTIED);
    sctk_assert(list != NULL);
    sctk_assert(mpcomp_task_list_isempty(list) == 1);

    /* Check New type tasks list */
    list = mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_NEW],
                                MPCOMP_TASK_TYPE_NEW);
    sctk_assert(list != NULL);
    sctk_assert(mpcomp_task_list_isempty(list) == 1);
  }
#endif
}

#else /* MPCOMP_TASK */

/*
 * Creation of an OpenMP task
 * Called when encountering an 'omp task' construct
 */
void __mpcomp_task(void (*fn)(void *), void *data,
                   void (*cpyfn)(void *, void *), long arg_size, long arg_align,
                   bool if_clause, unsigned flags) {
  if (cpyfn)
    sctk_abort();

  fn(data);
}

/*
 * Wait for the completion of the current task's children
 * Called when encountering a taskwait construct
 * Do nothing here as task are executed directly
 */
void mpcomp_taskwait() {}

/*
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void mpcomp_taskyield()
{ 
    /* Actually, do nothing */
}

#endif /* MPCOMP_TASK */
