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

#if MPCOMP_TASK

static int mpcomp_task_nb_delayed = 0;

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
	  if (n->depth == t->team->new_depth) {
	       n->new_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
	       mpcomp_task_list_new(n->new_tasks);
	       t->team->nb_newlists++;
	  }
	  
	  /* If the node correspond to the untied list depth, allocate the data structure */
	  if (n->depth == t->team->untied_depth) {
	       n->untied_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
	       mpcomp_task_list_new(n->untied_tasks);
	       t->team->nb_untiedlists++;
	  }
	  
	  if (n->depth <= t->team->untied_depth || n->depth <= t->team->new_depth) {	       
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
			 if (n->depth + 1 == t->team->new_depth) {
			      mvp->new_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
			      mpcomp_task_list_new(mvp->new_tasks);
			      t->team->nb_newlists++;
			 }
			 
			 /* If the node correspond to the untied list depth, allocate the data structure */
			 if (n->depth + 1 == t->team->untied_depth) {
			      mvp->untied_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
			      mpcomp_task_list_new(mvp->untied_tasks);
			      t->team->nb_untiedlists++;
			 }
		    }
		    break ;
		    
	       default:
		    sctk_nodebug("not reachable"); 
	       }
	  }
     }     
     __mpcomp_free_stack(s);
     free(s);
	  
     return;
}



void __mpcomp_task_list_larceny_init()
{
     mpcomp_thread_t *t;
     mpcomp_stack_t *s;
     struct mpcomp_task_list_s **newl;
     struct mpcomp_task_list_s ***dist_newl;
     struct mpcomp_task_list_s **untiedl;
     struct mpcomp_task_list_s ***dist_untiedl;
     int i, ni, ui;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     /* TODO compute the real number of max elements for this stack */
     s = __mpcomp_create_stack(2048);
     sctk_assert(s != NULL);


     ni = ui = 0;
     newl = sctk_malloc(sizeof(struct mpcomp_task_list_s *) * t->team->nb_newlists); 
     dist_newl = sctk_malloc(sizeof(struct mpcomp_task_list_s **) * t->team->nb_newlists); 
     untiedl = sctk_malloc(sizeof(struct mpcomp_task_list_s *) * t->team->nb_newlists); 
     dist_untiedl = sctk_malloc(sizeof(struct mpcomp_task_list_s **) * t->team->nb_newlists); 

     /* Push the root in the stack */
     __mpcomp_push(s, t->mvp->root);

     /* DFS starting from the root */
     while (!__mpcomp_is_stack_empty(s)) {
	  mpcomp_node_t *n;

	  /* Get the last node inserted in the stack */
	  n = __mpcomp_pop(s);
	  sctk_assert(n != NULL);
	  
	  /* If the node correspond to the new list depth, allocate the data structure */
	  if (n->depth == t->team->new_depth) {
#if MPCOMP_MALLOC_ON_NODE
	       n->dist_new_tasks = sctk_malloc_on_node(sizeof(struct mpcomp_task_list_s *) * t->team->nb_newlists - 1, n->id_numa);
#if MPCOMP_TASK_LARCENY_MODE == 1
	       n->new_rand_buffer = sctk_malloc_on_node(sizeof(struct drand48_data), n->id_numa);
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#else
	       n->dist_new_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s *) * t->team->nb_newlists - 1);
#if MPCOMP_TASK_LARCENY_MODE == 1
	       n->new_rand_buffer = sctk_malloc(sizeof(struct drand48_data));
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MALLOC_ON_NODE
	       for (i=0; i<ni; i++) {
		    n->dist_new_tasks[i] = newl[i];
		    dist_newl[i][ni - 1] = n->new_tasks;
	       }
	       newl[ni] = n->new_tasks;
	       dist_newl[ni] = n->dist_new_tasks;

#if MPCOMP_TASK_LARCENY_MODE == 1
	       srand48_r(time(NULL), n->new_rand_buffer);
#elif MPCOMP_TASK_LARCENY_MODE == 2
	       n->new_rank = ni;
#endif //MPCOMP_TASK_LARCENY_MODE == 1,2

		    ni++;
	  }
	  
	  /* If the node correspond to the untied list depth, allocate the data structure */
	  if (n->depth == t->team->untied_depth) {
#if MPCOMP_MALLOC_ON_NODE
	       n->dist_untied_tasks = sctk_malloc_on_node(sizeof(struct mpcomp_task_list_s *) * t->team->nb_untiedlists - 1, n->id_numa);
#if MPCOMP_TASK_LARCENY_MODE == 1
	       n->untied_rand_buffer = sctk_malloc_on_node(sizeof(struct drand48_data), n->id_numa);
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#else
	       n->dist_untied_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s *) * t->team->nb_untiedlists - 1);
#if MPCOMP_TASK_LARCENY_MODE == 1
	       n->untied_rand_buffer = sctk_malloc(sizeof(struct drand48_data));
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MALLOC_ON_NODE
	       for (i=0; i<ui; i++) {
		    n->dist_untied_tasks[i] = untiedl[i];
		    dist_untiedl[i][ui - 1] = n->untied_tasks;
	       }
	       untiedl[ui] = n->untied_tasks;
	       dist_untiedl[ui] = n->dist_untied_tasks;

#if MPCOMP_TASK_LARCENY_MODE == 1
	       srand48_r(time(NULL), n->untied_rand_buffer);
#elif MPCOMP_TASK_LARCENY_MODE == 2
	       n->untied_rank = ui;
#endif //MPCOMP_TASK_LARCENY_MODE == 1,2
	       
	       ui++;
	  }
	  
	  if (n->depth < t->team->untied_depth || n->depth < t->team->new_depth) {	       
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
			 if (n->depth + 1 == t->team->new_depth) {
#if MPCOMP_MALLOC_ON_NODE
			      mvp->dist_new_tasks = sctk_malloc_on_node(sizeof(struct mpcomp_task_list_s *) * t->team->nb_newlists - 1, mvp->father->id_numa);
#if MPCOMP_TASK_LARCENY_MODE == 1
			      mvp->new_rand_buffer = sctk_malloc_on_node(sizeof(struct drand48_data), mvp->father->id_numa);
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#else
			      mvp->dist_new_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s *) * t->team->nb_newlists - 1);
#if MPCOMP_TASK_LARCENY_MODE == 1
			      mvp->new_rand_buffer = sctk_malloc(sizeof(struct drand48_data));
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MALLOC_ON_NODE
			      for (i=0; i<ni; i++) {
				   mvp->dist_new_tasks[i] = newl[i];
				   dist_newl[i][ni - 1] = mvp->new_tasks;
			      }
			      newl[ni] = mvp->new_tasks;
			      dist_newl[ni] = mvp->dist_new_tasks;

#if MPCOMP_TASK_LARCENY_MODE == 1
			      srand48_r(time(NULL), mvp->new_rand_buffer);
#elif MPCOMP_TASK_LARCENY_MODE == 2
			      mvp->new_rank = ni;
#endif //MPCOMP_TASK_LARCENY_MODE == 1,2

			      ni++;
			 }
			 
			 /* If the node correspond to the untied list depth, allocate the data structure */
			 if (n->depth + 1 == t->team->untied_depth) {
#if MPCOMP_MALLOC_ON_NODE
			      mvp->dist_untied_tasks = sctk_malloc_on_node(sizeof(struct mpcomp_task_list_s *) * t->team->nb_untiedlists - 1, mvp->father->id_numa);
#if MPCOMP_TASK_LARCENY_MODE == 1
			      mvp->untied_rand_buffer = sctk_malloc_on_node(sizeof(struct drand48_data), mvp->father->id_numa);
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#else
			      mvp->dist_untied_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s *) * t->team->nb_untiedlists - 1);
#if MPCOMP_TASK_LARCENY_MODE == 1
			      mvp->untied_rand_buffer = sctk_malloc(sizeof(struct drand48_data));
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MALLOC_ON_NODE
			      for (i=0; i<ui; i++) {
				   mvp->dist_untied_tasks[i] = untiedl[i];
				   dist_untiedl[i][ui - 1] = mvp->untied_tasks;
			      }
			      untiedl[ui] = mvp->untied_tasks;
			      dist_untiedl[ui] = mvp->dist_untied_tasks;
			      ui++;

#if MPCOMP_TASK_LARCENY_MODE == 1
			      srand48_r(time(NULL), mvp->untied_rand_buffer);
#elif MPCOMP_TASK_LARCENY_MODE == 2
			      mvp->untied_rank = ni;
#endif //MPCOMP_TASK_LARCENY_MODE == 1,2
			 }
		    }
		    break ;
		    
	       default:
		    sctk_nodebug("not reachable"); 
	       }
	  }
     }     
     __mpcomp_free_stack(s);
     free(s);
     sctk_free(dist_untiedl);
     sctk_free(untiedl);
     sctk_free(dist_newl);
     sctk_free(newl);
     
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
	       __mpcomp_task_list_larceny_init();
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
	 || (t->current_task && mpcomp_task_type_isset (t->current_task->type, MPCOMP_TASK_FINAL))
	 || mpcomp_task_nb_delayed > MPCOMP_TASK_MAX_DELAYED) {
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
	  mpcomp_task_nb_delayed++;
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
	  new_list = mpcomp_task_list_get_newlist(t->mvp);
	  mpcomp_task_list_lock (new_list);
	  mpcomp_task_list_pushtotail (new_list, task);
	  mpcomp_task_list_unlock (new_list);
     }
}

#if MPCOMP_TASK_LARCENY_MODE == 0
static struct mpcomp_task_s *__mpcomp_get_new_task(struct mpcomp_node_s *local_root, int new_depth)
{
     struct mpcomp_task_list_s *list;
     struct mpcomp_task_s *task = NULL;
     struct mpcomp_node_s *n;
     mpcomp_stack_t *s;
     int i;

      /* TODO compute the real number of max elements for this stack */
     s = __mpcomp_create_stack(2048);
     sctk_assert(s != NULL);
     
     if (sctk_atomics_load_int(&(local_root->barrier)) + 1 < local_root->barrier_num_threads) {
	  /* Some threads are still working under this node*/
	  
	  /* Push the node containing new_list in the stack */
	  __mpcomp_push(s, local_root);

	  while (!__mpcomp_is_stack_empty(s)) {
	       
	       /* Get the last node inserted in the stack */
	       n = __mpcomp_pop(s);
	       sctk_assert(n != NULL);

	       if (sctk_atomics_load_int(&(n->barrier)) + 1 < n->barrier_num_threads) {
		    /* Some threads are still working under this node */

		    if (n->depth == new_depth) {
			 /* If current node own a new list */

			 list = n->new_tasks;
			 sctk_assert(list != NULL);
			 mpcomp_task_list_lock(list);
			 task = mpcomp_task_list_popfromtail(list);
			 mpcomp_task_list_unlock(list);
			 if (task != NULL)
			      return task;
		    } else {
			 /* Need to go deeper to find new tasks lists */

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
				   
				   list = mvp->new_tasks;
				   sctk_assert(list != NULL);
				   mpcomp_task_list_lock(list);
				   task = mpcomp_task_list_popfromtail(list);
				   mpcomp_task_list_unlock(list);
				   if (task != NULL)
					return task;
			      }
			      break ;
			      
			 default:
			      sctk_nodebug("not reachable"); 
			 }
		    }
	       }
	  }
     }

     __mpcomp_free_stack(s);
     free(s);
     return NULL;
}

static struct mpcomp_task_s *__mpcomp_get_untied_task(struct mpcomp_node_s *local_root, int untied_depth)
{
     struct mpcomp_task_list_s *list;
     struct mpcomp_task_s *task = NULL;
     struct mpcomp_node_s *n;
     mpcomp_stack_t *s;
     int i;

      /* TODO compute the real number of max elements for this stack */
     s = __mpcomp_create_stack(2048);
     sctk_assert(s != NULL);
     
     if (sctk_atomics_load_int(&(local_root->barrier)) + 1 < local_root->barrier_num_threads) {
	  /* Some threads are still working under this node*/
	  
	  /* Push the node containing untied_list in the stack */
	  __mpcomp_push(s, local_root);

	  while (!__mpcomp_is_stack_empty(s)) {
	       
	       /* Get the last node inserted in the stack */
	       n = __mpcomp_pop(s);
	       sctk_assert(n != NULL);

	       if (sctk_atomics_load_int(&(n->barrier)) + 1 < n->barrier_num_threads) {
		    /* Some threads are still working under this node */

		    if (n->depth == untied_depth) {
			 /* If current node own a untied list */

			 list = n->untied_tasks;
			 sctk_assert(list != NULL);
			 mpcomp_task_list_lock(list);
			 task = mpcomp_task_list_popfromtail(list);
			 mpcomp_task_list_unlock(list);
			 if (task != NULL)
			      return task;
		    } else {
			 /* Need to go deeper to find untied tasks lists */

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
				   
				   list = mvp->untied_tasks;
				   sctk_assert(list != NULL);
				   mpcomp_task_list_lock(list);
				   task = mpcomp_task_list_popfromtail(list);
				   mpcomp_task_list_unlock(list);
				   if (task != NULL)
					return task;
			      }
			      break ;
			      
			 default:
			      sctk_nodebug("not reachable"); 
			 }
		    }
	       }
	  }
     }

     __mpcomp_free_stack(s);
     free(s);
     return NULL;
}
#endif //MPCOMP_TASK_LARCENY_MODE == 0

#if MPCOMP_TASK_LARCENY_MODE == 0
static struct mpcomp_task_s * __mpcomp_task_larceny()
{
     mpcomp_thread_t *highwayman;
     struct mpcomp_task_list_s *list;
     struct mpcomp_task_s *task = NULL;
     struct mpcomp_node_s *ancestor, *n;
     int i, checked;

     /* Retrieve the information (microthread structure and current region) */
     highwayman = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(highwayman != NULL);


     /* Retrieve the node containing the new list */
     ancestor = highwayman->mvp->father;
     if (highwayman->team->new_depth < ancestor->depth + 1) {
	  while(ancestor->depth >= highwayman->team->new_depth && ancestor->father != NULL)
	       ancestor = ancestor->father;
          
	  /* Look inside each untied list of nearest threads (sharing the same new list) */
	  task = __mpcomp_get_untied_task(ancestor,  highwayman->team->untied_depth);
	  if (task)
	       return task;
     }

     /* Look inside new lists of others */
     n = ancestor;
     while (n->father) {
	  checked = n->rank;
	  n = n->father;
	  
	  for (i=0; i<n->nb_children; i++)
	       if (i != checked) {
		    task = __mpcomp_get_new_task(n->children.node[i], highwayman->team->new_depth);
		    if (task)
			 return task;
	       }
     }

     /* Look inside untied lists of others */
     n = ancestor;
     while (n->father) {
	  checked = n->rank;
	  n = n->father;
	  
	  for (i=0; i<n->nb_children; i++)
	       if (i != checked) {
		    task = __mpcomp_get_untied_task(n->children.node[i], highwayman->team->untied_depth);
		    if (task)
			 return task;
	       }
     }

     return NULL;
}
#endif // MPCOMP_TASK_LARCENY_MODE == 0


#if MPCOMP_TASK_LARCENY_MODE == 1 || MPCOMP_TASK_LARCENY_MODE == 2 || MPCOMP_TASK_LARCENY_MODE == 3

#if MPCOMP_TASK_LARCENY_MODE == 1
static void draw_lot(int res[], int n, struct drand48_data *rand_buffer)
{
     int i, k, no, tmp;
     long int v;

     for (i=0; i<n; i++)
	  res[i]= i;

     for (i=0, no=n; i<n; i++, no--) {
          lrand48_r(rand_buffer, &v);
	  k = v%no;
	  tmp = res[i];
	  res[i] = res[i+k];
	  res[i+k] = tmp;
     }
}
#endif //MPCOMP_TASK_LARCENY_MODE == 1

static int set_order(int res[], int n, mpcomp_node_t *node, mpcomp_mvp_t *mvp, int new)
{
#if MPCOMP_TASK_LARCENY_MODE == 1
     struct drand48_data *rand_buffer;

     if (new == 1)
	  rand_buffer = node ? node->new_rand_buffer : mvp->new_rand_buffer;
     else
	  rand_buffer = node ? node->untied_rand_buffer : mvp->untied_rand_buffer;
     
     draw_lot(res, n, rand_buffer);
     
     return n;
#elif MPCOMP_TASK_LARCENY_MODE == 2
     int i;
     unsigned long rank;

     if (new == 1)
	  rank = node ? node->new_rank : mvp->new_rank;
     else
	  rank = node ? node->untied_rank : mvp->untied_rank;
     
     for (i=0; i<n; i++)
	  res[i] = (rank + i + 1) % n;

     return n;
#elif MPCOMP_TASK_LARCENY_MODE == 3
     int i, j, k, no, nbe;
     struct mpcomp_task_list_s **tab_list;
     struct mpcomp_task_list_s *list;
     int *nb_elements = sctk_malloc(sizeof(int) * n);

     if (new == 1)
	  tab_list = node ? node->dist_new_tasks : mvp->dist_new_tasks;
     else
	  tab_list = node ? node->dist_untied_tasks : mvp->dist_untied_tasks;
     
     no = 0;
     for (i=0; i<n; i++) {
	  list = tab_list[i];
	  sctk_assert(list != NULL);
	  mpcomp_task_list_lock(list);
	  nbe = list->nb_elements;
	  mpcomp_task_list_unlock(list);
	  if (nbe > 0) {
	       j=0;
	       while (j<no && nbe <= nb_elements[res[j]])
		    j++;
	       
	       for (k=no; k>j; k++)
		    res[k] = res[k-1];
	       
	       res[j] = i;
	       nb_elements[i] = nbe;
	       no++;
	  }
     }

     sctk_free(nb_elements);
     
     return no;

#endif //MPCOMP_TASK_LARCENY_MODE == 1,2,3
}

static struct mpcomp_task_s * __mpcomp_task_larceny()
{
     mpcomp_thread_t *highwayman;
     struct mpcomp_task_list_s *list;
     struct mpcomp_task_s *task = NULL;
     int i, checked;
     int nb_newlists, nb_untiedlists, nbl;
     int *order;

     /* Retrieve the information (microthread structure and current region) */
     highwayman = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(highwayman != NULL);

     nb_newlists = highwayman->team->nb_newlists;
     nb_untiedlists = highwayman->team->nb_untiedlists;
     
     if (!highwayman->larceny_order)
	  highwayman->larceny_order = sctk_malloc(nb_newlists - 1);
     order = highwayman->larceny_order;

     if (highwayman->team->new_depth < highwayman->mvp->father->depth + 1) {
	  /* New lists are at some node level */

	  struct mpcomp_node_s *n;
	  
	  /* Retrieve the node containing the new list */
	  n = highwayman->mvp->father;
	  while(n->depth > highwayman->team->new_depth && n->father != NULL)
	       n = n->father;

	  nbl = set_order(order, nb_newlists - 1, n, NULL, 1);

	  for (i=0; i<nbl; i++) {
	       list = n->dist_new_tasks[order[i]];
	       sctk_assert(list != NULL);
	       mpcomp_task_list_lock(list);
	       task = mpcomp_task_list_popfromtail(list);
	       mpcomp_task_list_unlock(list);
	       if (task != NULL)
		    return task;
	  }
     } else {
	  /* New lists are at mvp level */

	  struct mpcomp_mvp_s *mvp;

	  mvp = highwayman->mvp;

	  nbl = set_order(order, nb_newlists - 1, NULL, mvp, 1);

	  for (i=0; i<nbl; i++) {
	       list = mvp->dist_new_tasks[order[i]];
	       sctk_assert(list != NULL);
	       mpcomp_task_list_lock(list);
	       task = mpcomp_task_list_popfromtail(list);
	       mpcomp_task_list_unlock(list);
	       if (task != NULL)
		    return task;
	  }
     }

     if (highwayman->team->untied_depth < highwayman->mvp->father->depth + 1) {
	  /* Untied lists are at some node level */
	  
	  struct mpcomp_node_s *n;
	  
	  /* Retrieve the node containing the untied list */
	  n = highwayman->mvp->father;
	  while(n->depth > highwayman->team->new_depth && n->father != NULL)
	       n = n->father;
	  
	  nbl = set_order(order, nb_untiedlists - 1, n, NULL, 0);

	  for (i=0; i<nbl; i++) {
	       list = n->dist_untied_tasks[order[i]];
	       sctk_assert(list != NULL);
	       mpcomp_task_list_lock(list);
	       task = mpcomp_task_list_popfromtail(list);
	       mpcomp_task_list_unlock(list);
	       if (task != NULL)
		    return task;
	  }
     } else {
	  /* Untied lists are at mvp level */
	  
	  struct mpcomp_mvp_s *mvp;

	  mvp = highwayman->mvp;

	  nbl = set_order(order, nb_untiedlists - 1, NULL, mvp, 0);

	  for (i=0; i<nbl; i++) {
	       list = mvp->dist_untied_tasks[order[i]];
	       sctk_assert(list != NULL);
	       mpcomp_task_list_lock(list);
	       task = mpcomp_task_list_popfromtail(list);
	       mpcomp_task_list_unlock(list);
	       if (task != NULL)
		    return task;
	  }
     }
     
     return NULL;
}
#endif //MPCOMP_TASK_LARCENY_MODE == 1 || MPCOMP_TASK_LARCENY_MODE == 2 || MPCOMP_TASK_LARCENY_MODE == 3

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
	       list = mpcomp_task_list_get_untiedlist(t->mvp);
	       mpcomp_task_list_lock (list);
	       task = mpcomp_task_list_popfromtail (list);
	       mpcomp_task_list_unlock (list);
	  }
      
	  if (task == NULL) {
	       /* If no task found previously, find a remaining task */
	       list = mpcomp_task_list_get_newlist(t->mvp);		    		    
	       mpcomp_task_list_lock (list);
	       task = mpcomp_task_list_popfromhead (list);
	       mpcomp_task_list_unlock (list);
	  }
	  
	  if (task == NULL) {
	       /* If no task found previously, try to thieve a task somewhere */
	       task = __mpcomp_task_larceny();
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

	  mpcomp_task_nb_delayed--;
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
			 mpcomp_task_nb_delayed--;
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

#endif //MPCOMP_TASK
