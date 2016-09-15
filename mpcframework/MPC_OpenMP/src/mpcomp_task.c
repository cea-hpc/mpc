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
#include "sctk_runtime_config_struct.h"

#if MPCOMP_TASK
#include "mpcomp_task_utils.h"
#include "mpcomp_tree_utils.h"
#include "mpcomp_task_stealing.h"

static inline int
fn_lolz(mpcomp_node_t *n, int *tasklist_depth, int* tasklistNodeRank, mpcomp_tasklist_type_t type)
{
    /* If the node correspond to the new list depth, allocate the data structures */
    if (n->depth != tasklist_depth[type])
        return 0;

    tasklistNodeRank[type] = n->tree_array_rank;
    n->tasklist[type] = mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), n->id_numa);
    mpcomp_task_list_new(n->tasklist[type]);
    return 1;
}

 
/* Recursive initialization of mpcomp tasks lists (new and untied) */
void 
__mpcomp_task_list_infos_init_r(mpcomp_node_t *n, struct mpcomp_node_s ***treeArray,
				     unsigned nb_numa_nodes, int current_globalRank, 
				     int current_stageSize, int current_firstRank, 
				     int *tasklistNodeRank)
{
     mpcomp_thread_t *t;
     int i, j;
     int child_stageSize, child_firstRank, first_child_globalRank, larcenyMode;

     sctk_assert(n != NULL);

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);
     
     larcenyMode = t->instance->team->task_larceny_mode;

     n->tree_array_rank = current_globalRank;
#if MPCOMP_OVERFLOW_CHECKING
     assert(current_globalRank < t->instance->tree_array_size);
#endif /* MPCOMP_OVERFLOW_CHECKING */

     for (j=0; j<nb_numa_nodes; j++)
	  treeArray[j][current_globalRank] = n;

     if(fn_lolz(n, t->instance->team->tasklist_depth, tasklistNodeRank, MPCOMP_TASK_TYPE_NEW))
     {
	    if ((larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM
		        || larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER)) {
                    mpcomp_task_init_victim_random(n, current_globalRank);
     } 
	  
#if MPCOMP_OVERFLOW_CHECKING
	  assert(n->id_numa < nb_numa_nodes);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	  n->tree_array = treeArray[n->id_numa];
     }
	  
     /* If the node correspond to the untied list depth, allocate the data structures */
     if (n->depth == t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]) {
	  tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED] = n->tree_array_rank;
	  n->tasklist[MPCOMP_TASK_TYPE_UNTIED] = mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), n->id_numa);
	  mpcomp_task_list_new(n->tasklist[MPCOMP_TASK_TYPE_UNTIED]);

	  if ((larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM
		  || larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER)) {
            mpcomp_task_init_victim_random(n, current_globalRank);
	  }
	       
#if MPCOMP_OVERFLOW_CHECKING
	  assert(n->id_numa < nb_numa_nodes);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	  n->tree_array = treeArray[n->id_numa];
     }

     /* Compute arguments for children */
     child_stageSize = current_stageSize * t->instance->tree_base[n->depth];
     child_firstRank = current_firstRank + current_stageSize;
     first_child_globalRank = child_firstRank + (current_globalRank - current_firstRank) * t->instance->tree_base[n->depth];

     /* Look at deeper levels */
	  switch (n->child_type) {
	  case MPCOMP_CHILDREN_NODE:
	       for (i=0; i<n->nb_children; i++) {
		    /* Call recursively for all children nodes */
		    __mpcomp_task_list_infos_init_r(n->children.node[i], treeArray, nb_numa_nodes, 
						    first_child_globalRank + i, child_stageSize, 
						    child_firstRank, tasklistNodeRank);
	       }
	       break;
		    
	  case MPCOMP_CHILDREN_LEAF:
	       for (i=0; i<n->nb_children; i++) {
		    /* All the children are leafs */
		    
		    mpcomp_mvp_t *mvp;
		    
		    mvp = n->children.leaf[i];
		    sctk_assert(mvp != NULL);

		    mvp->tree_array_rank = first_child_globalRank + i;
#if MPCOMP_OVERFLOW_CHECKING
		    assert(mvp->tree_array_rank < t->instance->tree_array_size);
#endif /* MPCOMP_OVERFLOW_CHECKING */
		    for (j=0; j<nb_numa_nodes; j++)
			 treeArray[j][mvp->tree_array_rank] = (mpcomp_node_t *) mvp;
		    
#if MPCOMP_OVERFLOW_CHECKING
		    assert(n->id_numa < nb_numa_nodes);
#endif /* MPCOMP_OVERFLOW_CHECKING */
		    mvp->tree_array = treeArray[n->id_numa];

		    /* If the mvp correspond to the new list depth, allocate the data structure */
		    if (n->depth + 1 == t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {
			 tasklistNodeRank[MPCOMP_TASK_TYPE_NEW] = mvp->tree_array_rank;
			 mvp->tasklist[MPCOMP_TASK_TYPE_NEW] = mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), n->id_numa);
			 mpcomp_task_list_new(mvp->tasklist[MPCOMP_TASK_TYPE_NEW]);

			 if (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM
			     || larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER) {
                    mpcomp_task_init_victim_random(n, current_globalRank);
			 }
		    }
			 
		    /* If the mvp correspond to the untied list depth, allocate the data structure */
		    if (n->depth + 1 == t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]) {
			 tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED] = mvp->tree_array_rank;
			 mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED] = mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), n->id_numa);
			 mpcomp_task_list_new(mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED]);

			 if ( (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM
				 || larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER)) {
                    mpcomp_task_init_victim_random(n, current_globalRank);
			 }
		    }

		    for (j=0; j<MPCOMP_TASK_TYPE_COUNT; j++)
			 mvp->tasklistNodeRank[j] = tasklistNodeRank[j];

	       }
	       break;
		    
	  default:
	       sctk_nodebug("not reachable"); 
	  }
	  
     return;
}

/* Initialization of mpcomp tasks lists (new and untied) */
void __mpcomp_task_list_infos_init()
{
     mpcomp_thread_t *t;
     int i, nbNumaNodes;
     mpcomp_instance_t *instance;
     struct mpcomp_node_s ***treeArray;
     int tasklistNodeRank[MPCOMP_TASK_TYPE_COUNT];

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     instance = t->instance;
     nbNumaNodes = sctk_max(sctk_get_numa_node_number(), 1);	  
     treeArray = mpcomp_malloc(0, sizeof(struct mpcomp_node_s **) * nbNumaNodes, 0);

     for (i=0; i<nbNumaNodes; i++)
	  treeArray[i] = mpcomp_malloc(1, sizeof(struct mpcomp_node_s *) * instance->tree_array_size, i);

     __mpcomp_task_list_infos_init_r(t->mvp->root, treeArray, nbNumaNodes, 0, 1, 0, 
				     tasklistNodeRank);

     mpcomp_free(0, treeArray, sizeof(struct mpcomp_node_s **) * nbNumaNodes);
}


void __mpcomp_task_list_infos_exit_r(mpcomp_node_t *n)
{
     mpcomp_thread_t *t;
     int i, j;
     
     sctk_assert(n != NULL);

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     /* If the node correspond to the new list depth, release the data structure */
     if (n->depth == t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {
	  //fprintf(stdout, "[node:%d-%d] new_tasks->total:%d new_tasks->nb_larcenies:%d\n", n->min_index, n->max_index, n->tasklist[MPCOMP_TASK_TYPE_NEW]->total, sctk_atomics_load_int(&n->tasklist[MPCOMP_TASK_TYPE_NEW]->nb_larcenies));
	  mpcomp_task_list_free(n->tasklist[MPCOMP_TASK_TYPE_NEW]);
	  n->tasklist[MPCOMP_TASK_TYPE_NEW] = NULL;
     }

     /* If the node correspond to the untied list depth, release the data structure */
     if (n->depth == t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]) {
	  //fprintf(stderr, "[node:%d-%d] untied_tasks->total:%d\n",  n->min_index, n->max_index, n->tasklist[MPCOMP_TASK_TYPE_UNTIED]->total);	       
	  mpcomp_task_list_free(n->tasklist[MPCOMP_TASK_TYPE_UNTIED]);
	  n->tasklist[MPCOMP_TASK_TYPE_UNTIED] = NULL;
     }
     
     /* If the untied or the new lists are at a lower level, look deeper */
     if (n->depth <= t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED] || n->depth <= t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {	       
	  switch (n->child_type) {
	  case MPCOMP_CHILDREN_NODE:
	       for (i=0; i<n->nb_children; i++) {
		    /* Call recursively for all children nodes */
		    __mpcomp_task_list_infos_exit_r(n->children.node[i]);
	       }
	       break;
	       
	  case MPCOMP_CHILDREN_LEAF:
	       for (i=0; i<n->nb_children; i++) {
		    /* All the children are leafs */
		    
		    mpcomp_mvp_t *mvp;
		    
		    mvp = n->children.leaf[i];
		    sctk_assert(mvp != NULL);
		    
		    /* If the node correspond to the new list depth, release the data structure */
		    if (n->depth + 1 == t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {
			 fprintf(stdout, "[mvp:%d] new_tasks->total:%d new_tasks->nb_larcenies:%d\n", mvp->rank, mvp->tasklist[MPCOMP_TASK_TYPE_NEW]->total, sctk_atomics_load_int(&mvp->tasklist[MPCOMP_TASK_TYPE_NEW]->nb_larcenies));
			 mpcomp_task_list_free(mvp->tasklist[MPCOMP_TASK_TYPE_NEW]);
			 mvp->tasklist[MPCOMP_TASK_TYPE_NEW] = NULL;
		    }
		    
		    /* If the node correspond to the untied list depth, release the data structure */
		    if (n->depth + 1 == t->instance->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]) {
			 fprintf(stderr, "[mvp:%d] untied_tasks->total:%d\n", mvp->rank, mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED]->total);
			 mpcomp_task_list_free(mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED]);
			 mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED] = NULL;
		    }
	       }
	       break;
		    
	  default:
	       sctk_nodebug("not reachable"); 
	  }
     }
	  
     return;
}

/* Release of mpcomp tasks lists (new and untied) */
void 
__mpcomp_task_list_infos_exit()
{
     mpcomp_thread_t *t;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     /* TODO: Add a recusrive search in children_instance of nb_mvps for nested parallelism support */
     if (t->instance->nb_mvps > 1)
	  __mpcomp_task_list_infos_exit_r(t->instance->root);
     else if (t->children_instance->nb_mvps > 1)
	  __mpcomp_task_list_infos_exit_r(t->children_instance->root);
}


void 
__mpcomp_task_exit()
{
#if 0
     mpcomp_thread_t *t;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     if ((t->info.num_threads > 1 && t->tasking_init_done) || t->children_instance->mvps[0]->threads[0].tasking_init_done) {
	  /*fprintf(stderr, "nb_tasks %d + %d\n", 
		  sctk_atomics_load_int(&(t->instance->team->nb_tasks)),
		  sctk_atomics_load_int(&(t->children_instance->team->nb_tasks)));*/

	  __mpcomp_task_list_infos_exit();
     } else {
	  //fprintf(stderr, "[t:%lu %p] __mpcomp_task_exit TASK INIT NOT DONE\n", t->rank, t);
     }
#endif
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

	  int id_numa;

	  if (t->info.num_threads > 1)
	       id_numa = t->mvp->father->id_numa;
	  else
	        id_numa = sctk_get_node_from_cpu(sctk_get_init_vp(sctk_get_task_rank()));

#if 0
      /* Allocate the default current task (no func, no data, no parent) */
	  t->current_task = mpcomp_malloc(1, sizeof(struct mpcomp_task_s), id_numa);
	  __mpcomp_task_init(t->current_task, NULL, NULL, t);
	  t->current_task->parent = NULL;
#endif

          /* Allocate private task data structures */
          t->tied_tasks =
              mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), id_numa);
          mpcomp_task_list_new(t->tied_tasks);

          t->tasking_init_done = 1;
     }

	 /* Executed only when there are multiple threads */
     if (t->info.num_threads > 1 && sctk_atomics_load_int(&(t->instance->team->tasklist_init_done)) == 0) 
     {
	    /* Executed only one time per process */
	    if (sctk_atomics_cas_int(&(t->instance->team->tasking_init_done), 0, 1) == 0) 
        { 
	        __mpcomp_task_list_infos_init();
	        //__mpcomp_task_check_neighbourhood();
	        sctk_atomics_store_int(&(t->instance->team->nb_tasks), 0);
	        sctk_atomics_store_int(&(t->instance->team->tasklist_init_done), 1);
	    }
	  
	  /* All threads wait until allocation is done */
	  while (sctk_atomics_load_int(&(t->instance->team->tasklist_init_done)) == 0)
	       ;
     }
}

/*
 *  Delete the parent link between the task 'parent' and all its children.
 *  These tasks became orphaned.
 */
void mpcomp_task_clear_parent(struct mpcomp_task_s *parent)
{
     struct mpcomp_task_s *child;
     //TODO: parent may be equal to NULL in between
     
     sctk_spinlock_lock(&(parent->children_lock));
     child = parent->children;
     while(child)
     {
	  child->parent = NULL;
	  child = child->next_child;
	  if (child) {
	       child->prev_child->next_child = NULL;
	       child->prev_child = NULL;
	  }
     }
     parent->children = NULL;
     sctk_spinlock_unlock(&(parent->children_lock));	       
}

/* 
 * Creation of an OpenMP task 
 * Called when encountering an 'omp task' construct 
 */
void 
__mpcomp_task(void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
		   long arg_size, long arg_align, bool if_clause, unsigned flags)
{
     mpcomp_thread_t *t;
     struct mpcomp_task_list_s *new_list;
     volatile int nb_delayed;
     
     /* Initialize the OpenMP environment (1 per task) */
     __mpcomp_init();

     /* Initialize the OpenMP task environment (1 per worker) */
     __mpcomp_task_infos_init();

     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);	 
     
     assume(t->instance != NULL);
     assume(t->instance->team != NULL);
     sctk_atomics_incr_int(&(t->instance->team->nb_tasks));
    
    /* squencial execution */
    if (t->info.num_threads == 1)
    {
        nb_delayed = 0;
        new_list = NULL;
    }
    else 
    {
      assume(t->mvp != NULL);
      assume(t->mvp->tasklistNodeRank != NULL);

	  new_list = mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_NEW], 
				     MPCOMP_TASK_TYPE_NEW);
          assume(new_list != NULL); 
     	  //nb_delayed = sctk_atomics_load_int(&(new_list->nb_elements));
     	  //sctk_atomics_incr_int(&(new_list->nb_elements));
          nb_delayed = sctk_atomics_load_int(&(new_list->nb_elements));
     	 // nb_delayed = sctk_atomics_fetch_and_add_int(&(new_list->nb_elements), 1);
     }

		 sctk_debug( "[%d] __mpcomp_task: new task (n_threads=%ld, current_task=%s, current_task_depth=%d, "
				 "nb_delayed=%d, current_task=%p)",
				 t->rank, t->info.num_threads, (t->current_task)?"yes":"no",
				 (t->current_task)?t->current_task->depth:0 ,
				 (t->info.num_threads == 1)?0:sctk_atomics_load_int(&(new_list->nb_elements)),
                 t->current_task
				 ) ;

     if (!if_clause
	 || (t->current_task && mpcomp_task_property_isset (t->current_task->property, MPCOMP_TASK_FINAL))
	 || (t->info.num_threads == 1)
	 || (nb_delayed > MPCOMP_TASK_MAX_DELAYED)
	 || (t->current_task && t->current_task->depth > 8/*t->instance->team->task_nesting_max*/)) {


	  /* Execute directly */
//	  fn(data);
	  struct mpcomp_task_s task;
	  struct mpcomp_task_s *prev_task;
	  mpcomp_local_icv_t icvs;

          __mpcomp_task_init(&task, NULL, NULL, t);
          mpcomp_task_set_property(&(task.property), MPCOMP_TASK_UNDEFERRED);
          prev_task = t->current_task;

          if ((prev_task && mpcomp_task_property_isset(prev_task->property,
                                                       MPCOMP_TASK_FINAL)) ||
              (flags & 2)) {
            /* If its parent task is final, the new task must be final too */
            mpcomp_task_set_property(&(task.property), MPCOMP_TASK_FINAL);
          }

          /* Current task is the new task which will be executed immediately */
          t->current_task = &task;
          sctk_assert(task.thread == NULL);
          icvs = t->info.icvs;
          task.icvs = t->info.icvs;
          task.thread = t;

          sctk_debug("[%d] __mpcomp_task: sequential task w/ args size %d",
                     t->rank, arg_size + arg_align - 1);

// fn (data);
#if 1
	  if (cpyfn != NULL) {
	       char tmp[arg_size + arg_align - 1];
	       char *data_cpy = (char *) (((uintptr_t) tmp + arg_align - 1)
	  				  & ~(uintptr_t) (arg_align - 1));

	       /* If cpyfn is given, use it to copy args */
	       cpyfn (data_cpy, data);
	       fn (data_cpy);
	  } else {
	       fn (data);
	  }
#endif

		sctk_debug( "[%d] __mpcomp_task: end of sequential task, current=%p",
				t->rank, prev_task ) ;
	  
	  /* Replace current task */
	  t->current_task = prev_task;
	  mpcomp_task_clear_parent(&task);
	  t->info.icvs = icvs;
	  TODO("Add cleaning for the task data structures")
     } else {
	  struct mpcomp_task_s *task;
	  struct mpcomp_task_s *parent;
	  char *data_cpy;
     

	  /* The new task may be delayed, so copy arguments in a buffer */
	  task = mpcomp_malloc(1, sizeof (struct mpcomp_task_s) + arg_size + arg_align - 1, t->mvp->father->id_numa);
	  sctk_assert (task != NULL);

	  data_cpy = (char*) (((uintptr_t) (task + 1) + arg_align - 1)
	  		      & ~(uintptr_t) (arg_align - 1));

#if MPCOMP_OVERFLOW_CHECKING
	  sctk_assert(data_cpy + arg_size < (char*)(task) + (sizeof (struct mpcomp_task_s) + arg_size + arg_align - 1));
#endif /* MPCOMP_OVERFLOW_CHECKING */

	  if (cpyfn) {
	       cpyfn (data_cpy, data);
	  } else {
	       memcpy(data_cpy, data, arg_size);
	  }

	  /* Create new task */
          __mpcomp_task_init(task, fn, data_cpy, t);
          task->icvs = t->info.icvs;
          mpcomp_task_set_property(&(task->property), MPCOMP_TASK_TIED);

          parent = task->parent;

          if ((parent && mpcomp_task_property_isset(parent->property,
                                                    MPCOMP_TASK_FINAL)) ||
              (flags & 2)) {
            /* If its parent task is final, the new task must be final too */
            mpcomp_task_set_property(&(task->property), MPCOMP_TASK_FINAL);
          }

          sctk_nodebug("task_depth = %d", task->depth);

          sctk_assert(task->depth <= 8 + 1);

          // sctk_assert(__mpcomp_task_check_circular_link(task) == 0);

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
          mpcomp_task_list_lock(new_list);
          mpcomp_task_list_pushtohead(new_list, task);
          mpcomp_task_list_unlock(new_list);
     }

   //  if(new_list != NULL)
     //   sctk_atomics_decr_int(&(new_list->nb_elements));
}


/* Steal a task in the 'type' task list of node identified by 'globalRank' */
static inline struct mpcomp_task_s *
mpcomp_task_steal(struct mpcomp_task_list_s *list)
{
     struct mpcomp_task_s *task = NULL;

     sctk_assert(list != NULL);
     mpcomp_task_list_lock(list);
     task = mpcomp_task_list_popfromtail(list);
     if (task)
	  sctk_atomics_incr_int(&list->nb_larcenies);
     mpcomp_task_list_unlock(list);
	 
     return task;
}


/* Return the ith victim for task stealing initiated from element at 'globalRank' */
static inline int 
mpcomp_task_get_victim(int globalRank, int index, mpcomp_tasklist_type_t type)
{
     mpcomp_thread_t *highwayman;
     int larcenyMode;
     int victim;

     /* Retrieve the information (microthread structure and current region) */
     highwayman = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(highwayman != NULL);

     larcenyMode = highwayman->instance->team->task_larceny_mode;

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

        case MPCOMP_TASK_LARCENY_MODE_PRODUCER:
            victim = mpcomp_task_get_victim_producer(globalRank, index, type);
	        break;

        case MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER:
	        victim = mpcomp_task_get_victim_producer_order(globalRank, index, type);
	        break;

        default:
	        victim = mpcomp_task_get_victim_default(globalRank, index, type);
	        break;	  
     }
     
     return victim;
}

/* Look in new and untied tasks lists of others */
static struct mpcomp_task_s * 
__mpcomp_task_larceny()
{
     mpcomp_thread_t *highwayman;
     struct mpcomp_task_s *task = NULL;
     struct mpcomp_mvp_s *mvp;
     struct mpcomp_task_list_s *list;
     int i, t, checked, rank, tasklistDepth, nbTasklists, larcenyMode, nbVictims;

     /* Retrieve the information (microthread structure and current region) */
     highwayman = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(highwayman != NULL);

     mvp = highwayman->mvp;
     larcenyMode = highwayman->instance->team->task_larceny_mode;

     /* Check first for NEW tasklists, then for UNTIED tasklists */
     for (t=0; t<MPCOMP_TASK_TYPE_COUNT; t++) {
	  
	  /* Retrieve informations about task lists */
	  tasklistDepth = highwayman->instance->team->tasklist_depth[t];
	  nbTasklists = highwayman->instance->tree_level_size[tasklistDepth];

	  if (nbTasklists > 1) {
	  /* If there are task lists in which we could steal tasks */

	       /* Try to steal inside the last stolen list*/
	       list = mvp->lastStolen_tasklist[t];
	       if (list) {
		    task = mpcomp_task_steal(list);
		    if (task) {
			 return task;	       
		    }
	       }

	       /* Get the rank of the ancestor containing the task list */
	       rank = mvp->tasklistNodeRank[t];
	       
	       if (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM 
		   || larcenyMode == MPCOMP_TASK_LARCENY_MODE_PRODUCER)
		    nbVictims = 1;
	       else
		    nbVictims = nbTasklists-1;
	       
	       /* Look for a task in all victims lists */
	       for (i=1; i<nbVictims+1; i++) {
		    int victim = mpcomp_task_get_victim(rank, i, t);
		    sctk_assert(victim != rank);
		    list = mpcomp_task_get_list(victim, t);
		    task = mpcomp_task_steal(list);
		    if (task) {
			 mvp->lastStolen_tasklist[t] = list;
			 return task;
		    }
	       }
	  }
     }
     
     return NULL;
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
     mpcomp_local_icv_t icvs;
     int i;
     
     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);	 

     /* If only one thread is running, tasks are not delayed. No need to schedule */
     if (t->info.num_threads == 1)
	  return;

     if (sctk_atomics_load_int(&(t->instance->team->nb_tasks)) == 0) {
       return ;
     }

     /* Initialize the OpenMP task environment (call several times, but really executed
      * once per worker thread) */
     __mpcomp_task_infos_init();
     

     prev_task = t->current_task;
	  
     while (1) {
	  /* Find a remaining tied task */
	  list = t->tied_tasks;
	  task = mpcomp_task_list_popfromtail(list);

	  if (task == NULL) {
	       /* If no task found previously, find a remaining untied task */
	       list = mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED], 
					   MPCOMP_TASK_TYPE_UNTIED);
	       mpcomp_task_list_lock(list);
	       task = mpcomp_task_list_popfromhead(list);
	       mpcomp_task_list_unlock(list);
	  }
      
	  if (task == NULL) {
	       /* If no task found previously, find a remaining new task */
	       list = mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_NEW], 
					   MPCOMP_TASK_TYPE_NEW);
	       mpcomp_task_list_lock(list);
	       task = mpcomp_task_list_popfromhead(list);
	       mpcomp_task_list_unlock(list);
	  }
	  
	  if (task == NULL) {
	       /* If no task found previously, try to thieve a task somewhere */
	       task = __mpcomp_task_larceny();
	  }
	  
	  if (task == NULL) { 
	       /* All tasks lists are empty, so exit task scheduling function */
	       return;
	  }

	  /* Current task is the new task which will be executed */
	  t->current_task = task;
	  sctk_assert(task->thread == NULL);
	  icvs = t->info.icvs;
	  t->info.icvs = task->icvs;
	  task->thread = t;
	  task->func(task->data);
	  
	  t->info.icvs = icvs;

	  mpcomp_task_clear_parent(task);
	  
	  parent_task = task->parent;
	  //parent_task maybe equal to NULL in between
	  if (parent_task) {
	       /* Remove the task from his parent children list */
	       sctk_spinlock_lock(&(parent_task->children_lock));
	       if (parent_task->children == task)
		    parent_task->children = task->next_child;
	       if (task->next_child != NULL)
		    task->next_child->prev_child = task->prev_child;
	       if (task->prev_child != NULL)
		    task->prev_child->next_child = task->next_child;
	       sctk_spinlock_unlock(&(parent_task->children_lock));
	  }
	  
	  /* Replace current task */
	  t->current_task = prev_task;
	  
	  mpcomp_free(1, task, sizeof(struct mpcomp_task_s));
	  task = NULL;
     }
}


#define MY_TASKWAIT 0
/* 
 * Wait for the completion of the current task's children
 * Called when encountering a taskwait construct
 */
#if MY_TASKWAIT
void 
__mpcomp_taskwait()
{
     mpcomp_thread_t *t;
     struct mpcomp_task_s *task = NULL;
     struct mpcomp_task_s *prev_task = NULL;
     struct mpcomp_task_s *next_child = NULL;
     struct mpcomp_task_list_s *list;
     mpcomp_local_icv_t icvs;
     int ret;

     /* Initialize the OpenMP task environment (call several times, but really executed
      * once per worker thread) */
     __mpcomp_task_infos_init();
     
     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     /* If only one thread is running, tasks are not delayed. No need to wait */
     if (t->info.num_threads == 1)
       return;

     /* Save current task pointer */
     prev_task = t->current_task;
     sctk_assert(prev_task != NULL);

     sctk_spinlock_lock(&(prev_task->children_lock));

     /* Look for a children tasks list */
     while (prev_task->children != NULL) {
	  task = prev_task->children;

	  /* Search for every child task */
	  while (task != NULL) {
	       next_child = task->next_child;
	       sctk_spinlock_unlock(&(prev_task->children_lock));

	       list = task->list;
	  
	       /* If the task has not been remove by another thread */
	       if (list != NULL) {
		    mpcomp_task_list_lock(list);
		    ret = mpcomp_task_list_remove(list, task);
		    mpcomp_task_list_unlock(list);
		    
		    /* If the task has not been remove by another thread */
		    if (ret == 1) {
			 /* Current task is the new task which will be executed */
			 t->current_task = task;
			 sctk_assert(task->thread == NULL);		      
			 //sctk_assert(__mpcomp_task_check_circular_link(task) == 0);
			 icvs = t->info.icvs;
			 t->info.icvs = task->icvs;
			 task->thread = t;
			 task->func(task->data);

			 t->info.icvs = icvs;

			 mpcomp_task_clear_parent(task);
			 
			 /* Remove the task from his parent children list */
			 sctk_spinlock_lock(&(prev_task->children_lock));
			 if (prev_task->children == task)
			      prev_task->children = task->next_child;
			 if (task->next_child != NULL)
			      task->next_child->prev_child = task->prev_child;
			 if (task->prev_child != NULL)
			      task->prev_child->next_child = task->next_child;
			 sctk_spinlock_unlock(&(prev_task->children_lock));
		    
			 /* Replace current task */
			 t->current_task = prev_task;
			 
			 next_child = task->next_child;
			 mpcomp_free(1, task, sizeof(struct mpcomp_task_s));
			 task = NULL;
		    }
	       }
	       task = next_child;
	       sctk_spinlock_lock(&(prev_task->children_lock));
	  }
	  
	  sctk_spinlock_unlock(&(prev_task->children_lock));
     	  sctk_thread_yield();	  
	  sctk_spinlock_lock(&(prev_task->children_lock)); 
     }

     sctk_spinlock_unlock(&(prev_task->children_lock));
}
#else /* MY_TASKWAIT */
void __mpcomp_taskwait()
{
     mpcomp_thread_t *t;
     struct mpcomp_task_s *prev_task = NULL;

     /* Initialize the OpenMP task environment (call several times, but really executed
      * once per worker thread) */
     __mpcomp_task_infos_init();
     
     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     /* If only one thread is running, tasks are not delayed. No need to wait */
     if (t->info.num_threads == 1)
	  return;

     /* Save current task pointer */
     prev_task = t->current_task;
     sctk_assert(prev_task != NULL);

     /* Look for a children tasks list */
     while (prev_task->children != NULL) {
	  __mpcomp_task_schedule();
     }
}
#endif /* MY_TASKWAIT */

/*
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void 
__mpcomp_taskyield()
{
     /* Actually, do nothing */
}

void __mpcomp_task_coherency_entering_parallel_region() {
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

    sctk_assert(sctk_atomics_load_int(&team->nb_tasks) == 0);

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

      sctk_assert(lt->current_task != NULL);
      sctk_assert(lt->current_task->children == NULL);
      sctk_assert(lt->current_task->list == NULL);
      sctk_assert(lt->current_task->depth == 0);
      sctk_assert(mpcomp_task_property_isset(lt->current_task->property,
                                             MPCOMP_TASK_IMPLICIT) != 0);
    }
  }
}

void __mpcomp_task_coherency_ending_parallel_region() {
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
}

void __mpcomp_task_coherency_barrier() {
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
}

#else /* MPCOMP_TASK */

/* 
 * Creation of an OpenMP task 
 * Called when encountering an 'omp task' construct 
 */
void 
__mpcomp_task(void *(*fn) (void *), void *data, void (*cpyfn) (void *, void *),
		   long arg_size, long arg_align, bool if_clause, unsigned flags)
{
     fn(data);
}

/* 
 * Wait for the completion of the current task's children
 * Called when encountering a taskwait construct
 * Do nothing here as task are executed directly
 */
void 
__mpcomp_taskwait()
{
}

/*
 * The current may be suspended in favor of execution of
 * a different task
 * Called when encountering a taskyield construct
 */
void 
__mpcomp_taskyield()
{
     /* Actually, do nothing */
}

#endif /* MPCOMP_TASK */
