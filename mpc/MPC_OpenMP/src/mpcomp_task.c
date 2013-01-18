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


#include <sctk_bool.h>
#include <sctk_int.h>
#include <sctk_asm.h>
#include "mpcomp_internal.h"


/* Initialization of mpcomp tasks lists (new and untied) */
void __mpcomp_task_list_infos_init()
{
     mpcomp_thread_t *t;
     mpcomp_stack_t *s;
     int i, j;
     
     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     /* TODO compute the real number of max elements for this stack */
     s = __mpcomp_create_stack(2048);
     sctk_assert(s != NULL);

     /* Push the root in the stack */
     __mpcomp_push(s, t->mvp->root);

     /* DFS starting from the root */
     while (!__mpcomp_is_stack_empty(s)) {
	  mpcomp_node_t *n;

	  /* Get the last node inserted in the stack */
	  n = __mpcomp_pop(s);
	  sctk_assert(n != NULL);

	  /* If the node correspond to the new list depth, allocate the data structure */
	  if (n->depth == n->team_info->new_depth) {
	       n->new_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
	       mpcomp_task_list_new(n->new_tasks);
	  }
	  
	  /* If the node correspond to the untied list depth, allocate the data structure */
	  if (n->depth == n->team_info->untied_depth) {
	       n->untied_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
	       mpcomp_task_list_new(n->untied_tasks);
	  }
	  
	  switch (n->child_type) {
	  case MPCOMP_CHILDREN_NODE:
	       for (i=n->nb_children-1; i>=0; i--) {
		    /* Insert all the node's children in the stack */
		    __mpcomp_push(s, n->children.node[i]);
	       }
	       break;
	       
	  case MPCOMP_CHILDREN_LEAF:
	       for (i=0; i<n->nb_children; i++) {
		    /* All the children are leafs */

		    mpcomp_mvp_t *mvp;
		    
		    mvp = n->children.leaf[i];
		    sctk_assert(mvp != NULL);

		    /* If the node correspond to the new list depth, allocate the data structure */
		    if (n->depth + 1 == n->team_info->new_depth) {
			 mvp->new_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
			 mpcomp_task_list_new(n->new_tasks);
		    }
		    
		    /* If the node correspond to the untied list depth, allocate the data structure */
		    if (n->depth + 1 == n->team_info->untied_depth) {
			 mvp->untied_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
			 mpcomp_task_list_new(n->untied_tasks);
		    }
	       }
	       break ;
	
	  default:
	       sctk_nodebug("not reachable"); 
	  }
     }     
     __mpcomp_free_stack(s);
     free(s);
     
     return;
}


/* 
 * Initialization of OpenMP task environment 
 */
void __mpcomp_task_infos_init()
{
     static struct mpcomp_task_list_s **new_tasks;
     mpcomp_thread_t *t;
     
     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);	 
     
     if (t->tasking_init_done == 0) {
	  /* Executed only one time per thread */

	  volatile int *done = (volatile int *) &(t->team->tasking_init_done);
	  static volatile int barrier = 0;

	  /* Allocate the default current task (no func, no data, no parent) */
	  t->current_task = sctk_malloc(sizeof(struct mpcomp_task_s));
	  __mpcomp_task_init (t->current_task, NULL, NULL, t);
	  t->current_task->parent = NULL;

	  /* Allocate private task data structures */
	  t->tied_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
	  mpcomp_task_list_new (t->tied_tasks);
      
	  if (sctk_test_and_set(done) == 0) { 
	       /* Executed only one time per process */

	       __mpcomp_task_list_infos_init();
	       barrier = 1;
	  }
	  
	  /* All threads wait until allocation is done */
	  while (barrier == 0)
	       ;
	  t->tasking_init_done = 1;
     }
}



/* 
 * Creation of an OpenMP task 
 * Called when encountering an 'omp task' construct 
 */
void __mpcomp_task(void *(*fn) (void *), void *data, void (*cpyfn) (void *, void *),
		   long arg_size, long arg_align, bool if_clause, unsigned flags)
{
     mpcomp_thread_t *t;

     /* Initialize the OpenMP environment (called several times, but really executed
      * once) */
     __mpcomp_init ();

     /* Initialize the OpenMP task environment (called several times, but really 
      * executed once per worker thread) */
     __mpcomp_task_infos_init();

     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);	 

     TODO("Maybe add test in case only one thread is running in the team or if max number of tasks is reached")
     if (!if_clause
	 || (t->current_task && mpcomp_task_type_isset (t->current_task->type, MPCOMP_TASK_FINAL))) {
	  /* Execute directly */

	  struct mpcomp_task_s task;

	  __mpcomp_task_init (&task, NULL, NULL, t);
	  mpcomp_task_set_type (&(task.type), MPCOMP_TASK_UNDEFERRED);
      
	  if ((t->current_task
	       && mpcomp_task_type_isset (t->current_task->type, MPCOMP_TASK_FINAL))
	      || (flags & 2)) {
	       /* If its parent task is final, the new task must be final too */
	       mpcomp_task_set_type (&(task.type), MPCOMP_TASK_FINAL);
	  }
	       
	  /* Current task is the new task which will be executed immediately */
	  t->current_task = &task;

	  if (cpyfn != NULL) {
	       /* If cpyfn is given, usr it to copy args */
	       char tmp[arg_size + arg_align - 1];
	       char *data_cpy = (char *) (((uintptr_t) tmp + arg_align - 1) 
					  & ~(uintptr_t) (arg_align - 1));
	       cpyfn (data_cpy, data);

	       fn (data_cpy);
	  } else {
	       fn (data);
	  }
	       
	  /* Replace current task */
	  t->current_task = task.parent;
	  TODO("Add cleaning for the task data structures")
     } else {
	  struct mpcomp_task_s *task;
	  struct mpcomp_task_s *parent;
	  struct mpcomp_task_list_s *new_list;
	  char *data_cpy;

	  /* The new task may be delayed, so copy arguments in a buffer */
	  task = sctk_malloc (sizeof (struct mpcomp_task_s) + arg_size + arg_align - 1);
	  sctk_assert (task != NULL);
	  data_cpy = (char*) (((uintptr_t) (task + 1) + arg_align -1)
			      & ~(uintptr_t) (arg_align - 1));
	  if (cpyfn) {
	       cpyfn (data_cpy, data);
	  } else {
	       memcpy(data_cpy, data, arg_size);
	  }

	  /* Create new task */
	  __mpcomp_task_init (task, fn, data_cpy, t);
	  mpcomp_task_set_type (&(task->type), MPCOMP_TASK_TIED);
	  if (flags & 2) {
	       mpcomp_task_set_type (&(task->type), MPCOMP_TASK_FINAL);
	  }

	  parent = task->parent;

	  if (parent) {
	       /* If task has a parent, add it to the children list */
	       sctk_spinlock_lock(&(parent->children_lock));

	       if (parent->children) {
		    /* Parent task already had children */

		    task->next_child = parent->children;
		    task->prev_child = NULL;
		    task->next_child->prev_child = task;
	       } else {
		    /* Children list is empty */

		    task->next_child = NULL;
		    task->prev_child = NULL;
	       }
	       /* The task became the first task */
	       parent->children = task;

	       sctk_spinlock_unlock(&(parent->children_lock));
	  }
	  
	  /* Push the task in the list of new tasks */
	  new_list = mpcomp_task_list_get_newlist(t);
	  mpcomp_task_list_lock (new_list);
	  mpcomp_task_list_pushtotail (new_list, task);
	  mpcomp_task_list_unlock (new_list);
     }
}


/* 
 * Schedule remaining tasks in the different task lists (tied, untied, new) 
 * Called at several schedule points : 
 *     - in taskyield regions
 *     - in taskwait regions
 *     - in implicit and explicit barrier regions
 */
void __mpcomp_task_schedule()
{
     mpcomp_thread_t *t;
     struct mpcomp_task_s *task = NULL;
     struct mpcomp_task_s *parent_task = NULL;
     struct mpcomp_task_s *prev_task = NULL;
     struct mpcomp_task_list_s *list;
     int i;
     
     /* Initialize the OpenMP task environment (call several times, but really executed
      * once per worker thread) */
     __mpcomp_task_infos_init();

     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);	 
     
     prev_task = t->current_task;
     TODO("Maybe add test in case only one thread is running in the team")
	  
     while (1) {
	  /* Find a remaining tied task */
	  list = t->tied_tasks;
	  task = mpcomp_task_list_popfromtail (list);
	       
	  if (task == NULL) {
	       /* If no task found previously, find a remaining untied task */
	       list = mpcomp_task_list_get_untiedlist(t);
	       mpcomp_task_list_lock (list);
	       task = mpcomp_task_list_popfromtail (list);
	       mpcomp_task_list_unlock (list);
	  }
      
	  if (task == NULL) {
	       /* If no task found previously, find a remaining task */
	       list = mpcomp_task_list_get_newlist(t);		    		    
	       mpcomp_task_list_lock (list);
	       task = mpcomp_task_list_popfromhead (list);
	       mpcomp_task_list_unlock (list);
	  }
	  
	  if (task == NULL) {
	       /* All tasks lists are empty, so exit task scheduling function */
	       return;
	  }

	  parent_task = task->parent;

	  /* Remove the task from his parent children list */
	  sctk_spinlock_lock(&(parent_task->children_lock));
	  if (parent_task->children == task)
	       parent_task->children = task->next_child;
	  if (task->next_child != NULL)
	       task->next_child->prev_child = task->prev_child;
	  if (task->prev_child != NULL)
	       task->prev_child->next_child = task->next_child;
	  sctk_spinlock_unlock(&(parent_task->children_lock));
	  

	  /* Current task is the new task which will be executed */
	  t->current_task = task;
	  task->func (task->data);

	  /* Replace current task */
	  t->current_task = prev_task;
	 
	  free(task);
	  task = NULL;
     }
}


/* 
 * Wait for the completion of the current task's children
 * Called when encountering a taskwait construct
 */
void __mpcomp_taskwait()
{
     mpcomp_thread_t *t;
     struct mpcomp_task_s *task = NULL;
     struct mpcomp_task_s *prev_task = NULL;
     struct mpcomp_task_list_s *list;
     int ret;

     /* Initialize the OpenMP task environment (call several times, but really executed
      * once per worker thread) */
     __mpcomp_task_infos_init();
     
     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     /* Save current task pointer */
     prev_task = t->current_task;
     sctk_assert(prev_task != NULL);

     sctk_spinlock_lock(&(prev_task->children_lock));

     /* Look for a children tasks list */
     while (prev_task->children != NULL) {
	  task = prev_task->children;

	  /* Search for every child task */
	  while (task != NULL) {
	       sctk_spinlock_unlock(&(prev_task->children_lock));
	  
	       list = task->list;
	  
	       /* If the task has not been remove by another thread */
	       if (list != NULL) {
		    mpcomp_task_list_lock(list);
		    ret = mpcomp_task_list_remove(list, task);
		    mpcomp_task_list_unlock(list);
		    
		    /* If the task has not been remove by another thread */
		    if (ret == 1) {

			 /* Remove the task from his parent children list */
			 sctk_spinlock_lock(&(prev_task->children_lock));
			 if (prev_task->children == task)
			      prev_task->children = task->next_child;
			 if (task->next_child != NULL)
			      task->next_child->prev_child = task->prev_child;
			 if (task->prev_child != NULL)
			      task->prev_child->next_child = task->next_child;
			 sctk_spinlock_unlock(&(prev_task->children_lock));
		    
			 /* Current task is the new task which will be executed */
			 t->current_task = task;
			 task->func(task->data);

			 /* Replace current task */
			 t->current_task = prev_task;
			 
			 free(task);
		    }
	       }
	       
	       sctk_spinlock_lock(&(prev_task->children_lock));
	       task = task->next_child;
	  }
	  
	  sctk_spinlock_unlock(&(prev_task->children_lock));
     	  sctk_thread_yield();	  
	  sctk_spinlock_lock(&(prev_task->children_lock)); 
     }
     
     sctk_spinlock_unlock(&(prev_task->children_lock));
}


/*
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void __mpcomp_taskyield()
{
     /* Actually, do nothing */
}
