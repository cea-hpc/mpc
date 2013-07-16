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
#include "sctk_alloc_numa_stat.h"

#if MPCOMP_TASK

int mpcomp_is_leaf(int globalRank)
{
     mpcomp_thread_t *t;
     mpcomp_instance_t *instance;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     instance = t->team->instance;

     return (globalRank >= (instance->tree_array_size - instance->nb_mvps));
}

/* Return the ith neighbour of the element at rank 'globalRank' in the tree_array */
int mpcomp_get_neighbour(int globalRank, int i)
{
     mpcomp_thread_t *t;
     mpcomp_instance_t *instance;
     int j, r, firstRank, id, nbSubleaves, vectorSize;
     int *treeBase, *treeLevelSize, *path;
     
     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     instance = t->team->instance;
     treeLevelSize = instance->tree_level_size;
     treeBase = instance->tree_base;

     if(mpcomp_is_leaf(globalRank)) {
	  mpcomp_mvp_t *mvp = (mpcomp_mvp_t *) (t->mvp->tree_array[globalRank]);
	  path = mvp->path;
	  vectorSize = mvp->father->depth + 1;
     } else {
	  mpcomp_node_t *n = t->mvp->tree_array[globalRank];
	  path = n->path;
	  vectorSize = n->depth;
     }

     int v[vectorSize], res[vectorSize];

     r = i;
     id = 0;
     firstRank = 0;
     nbSubleaves = 1;
     for (j=0; j<vectorSize; j++) {
	  int base = treeBase[vectorSize-1-j]; 
	  int level_size = treeLevelSize[j];

	  /* Somme de i avec le codage de 0 dans le vecteur de l'arbre */
	  v[j] = r % base;
	  r /= base;

	  /* Addition sans retenue avec le vecteur de n */
	  res[j] = (path[vectorSize-1-j] + v[j]) % base;	 
#if MPCOMP_OVERFLOW_CHECKING
	  res[j] < base;
#endif /* MPCOMP_OVERFLOW_CHECKING */

	  /* Calcul de l'identifiant du voisin dans l'arbre */
	  id += res[j] * nbSubleaves;
	  nbSubleaves *= base;
	  firstRank += level_size;
     }

     return id + firstRank;
}

/* Return the ancestor of element at rank 'globalRank' at depth 'depth' */
int mpcomp_get_ancestor(int globalRank, int depth)
{
     mpcomp_thread_t *t;
     int i, ancestor_id, firstRank, nbSubLeaves, currentDepth;
     int *levelSize, *treeBase, *path;

     /* If it's the root, ancestor is itself */
     if (globalRank == 0)
	  return 0;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     if(mpcomp_is_leaf(globalRank)) {
	  mpcomp_mvp_t *mvp = (mpcomp_mvp_t *) (t->mvp->tree_array[globalRank]);
	  path = mvp->path;
	  currentDepth = mvp->father->depth + 1;
     } else {
	  mpcomp_node_t *n = t->mvp->tree_array[globalRank];
	  path = n->path;
	  currentDepth = n->depth;
     }

     if (currentDepth == depth)
	  return globalRank;

     levelSize = t->team->instance->tree_level_size;
     treeBase = t->team->instance->tree_base;
     ancestor_id =0;
     firstRank = 0;
     nbSubLeaves = 1;
     for (i=0; i<depth; i++) {
	  ancestor_id += path[depth-1-i] * nbSubLeaves;
	  nbSubLeaves *= treeBase[depth-1-i]; 
	  firstRank += levelSize[i];
     }

     return firstRank + ancestor_id;
}

/* Recursive initialization of mpcomp tasks lists (new and untied) */
void __mpcomp_task_list_infos_init_r(mpcomp_node_t *n, struct mpcomp_node_s ***treeArray,
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
     
     larcenyMode = t->team->task_larceny_mode;

     n->tree_array_rank = current_globalRank;
#if MPCOMP_OVERFLOW_CHECKING
     assert(current_globalRank < t->team->instance->tree_array_size);
#endif /* MPCOMP_OVERFLOW_CHECKING */
     for (j=0; j<nb_numa_nodes; j++)
	  treeArray[j][current_globalRank] = n;

     /* If the node correspond to the new list depth, allocate the data structures */
     if (n->depth == t->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {
	  tasklistNodeRank[MPCOMP_TASK_TYPE_NEW] = n->tree_array_rank;
	  n->tasklist[MPCOMP_TASK_TYPE_NEW] = mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), n->id_numa);
	  mpcomp_task_list_new(n->tasklist[MPCOMP_TASK_TYPE_NEW]);

	  if (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM
	      || larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER) {
	       n->tasklist_randBuffer = mpcomp_malloc(1, sizeof(struct drand48_data), n->id_numa);
	       srand48_r(time(NULL), n->tasklist_randBuffer);
	  }
	  
#if MPCOMP_OVERFLOW_CHECKING
	  assert(n->id_numa < nb_numa_nodes);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	  n->tree_array = treeArray[n->id_numa];
     }
	  
     /* If the node correspond to the untied list depth, allocate the data structures */
     if (n->depth == t->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]) {
	  tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED] = n->tree_array_rank;
	  n->tasklist[MPCOMP_TASK_TYPE_UNTIED] = mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), n->id_numa);
	  mpcomp_task_list_new(n->tasklist[MPCOMP_TASK_TYPE_UNTIED]);

	  if (!n->tasklist_randBuffer 
	      && (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM
		  || larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER)) {
	       n->tasklist_randBuffer = mpcomp_malloc(1, sizeof(struct drand48_data), n->id_numa);
	       srand48_r(time(NULL), n->tasklist_randBuffer);
	  }
	       
#if MPCOMP_OVERFLOW_CHECKING
	  assert(n->id_numa < nb_numa_nodes);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	  n->tree_array = treeArray[n->id_numa];
     }

     /* Compute arguments for children */
     child_stageSize = current_stageSize * t->team->instance->tree_base[n->depth];
     child_firstRank = current_firstRank + current_stageSize;
     first_child_globalRank = child_firstRank + (current_globalRank - current_firstRank) * t->team->instance->tree_base[n->depth];

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
		    assert(mvp->tree_array_rank < t->team->instance->tree_array_size);
#endif /* MPCOMP_OVERFLOW_CHECKING */
		    for (j=0; j<nb_numa_nodes; j++)
			 treeArray[j][mvp->tree_array_rank] = (mpcomp_node_t *) mvp;
		    
		    /* If the mvp correspond to the new list depth, allocate the data structure */
		    if (n->depth + 1 == t->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {
			 tasklistNodeRank[MPCOMP_TASK_TYPE_NEW] = mvp->tree_array_rank;
			 mvp->tasklist[MPCOMP_TASK_TYPE_NEW] = mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), n->id_numa);
			 mpcomp_task_list_new(mvp->tasklist[MPCOMP_TASK_TYPE_NEW]);

			 if (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM
			     || larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER) {
			      mvp->tasklist_randBuffer = mpcomp_malloc(1, sizeof(struct drand48_data), mvp->father->id_numa);
			      srand48_r(time(NULL), mvp->tasklist_randBuffer);
			 }
			 
#if MPCOMP_OVERFLOW_CHECKING
			 assert(n->id_numa < nb_numa_nodes);
#endif /* MPCOMP_OVERFLOW_CHECKING */
			 mvp->tree_array = treeArray[n->id_numa];
		    }
			 
		    /* If the mvp correspond to the untied list depth, allocate the data structure */
		    if (n->depth + 1 == t->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]) {
			 tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED] = mvp->tree_array_rank;
			 mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED] = mpcomp_malloc(1, sizeof(struct mpcomp_task_list_s), n->id_numa);
			 mpcomp_task_list_new(mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED]);

			 if (!n->tasklist_randBuffer 
			     && (larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM
				 || larcenyMode == MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER)) {
			      mvp->tasklist_randBuffer = mpcomp_malloc(1, sizeof(struct drand48_data), mvp->father->id_numa);
			      srand48_r(time(NULL), mvp->tasklist_randBuffer);
			 }
			 
#if MPCOMP_OVERFLOW_CHECKING		    
			 assert(n->id_numa < nb_numa_nodes);
#endif /* MPCOMP_OVERFLOW_CHECKING */
			 mvp->tree_array = treeArray[n->id_numa];
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

     instance = t->team->instance;
     nbNumaNodes = sctk_get_numa_node_number();	  
     treeArray = sctk_malloc(sizeof(struct mpcomp_node_s **) * nbNumaNodes);
     for (i=0; i<nbNumaNodes; i++)
	  treeArray[i] = mpcomp_malloc(1, sizeof(struct mpcomp_node_s *) * instance->tree_array_size, i);

     __mpcomp_task_list_infos_init_r(t->mvp->root, treeArray, nbNumaNodes, 0, 1, 0, 
				     tasklistNodeRank);

     sctk_free(treeArray);
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
     if (n->depth == t->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {
	  //fprintf(stderr, "[node:%d-%d] new_tasks->total:%d\n", n->min_index, n->max_index, n->tasklist[MPCOMP_TASK_TYPE_NEW]->total);
	  mpcomp_task_list_free(n->tasklist[MPCOMP_TASK_TYPE_NEW]);
     }

     /* If the node correspond to the untied list depth, release the data structure */
     if (n->depth == t->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]) {
	  //fprintf(stderr, "[node:%d-%d] untied_tasks->total:%d\n",  n->min_index, n->max_index, n->tasklist[MPCOMP_TASK_TYPE_UNTIED]->total);	       
	  mpcomp_task_list_free(n->tasklist[MPCOMP_TASK_TYPE_UNTIED]);
     }
     
     /* If the untied or the new lists are at a lower level, look deeper */
     if (n->depth <= t->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED] || n->depth <= t->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {	       
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
		    if (n->depth + 1 == t->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]) {
			 //fprintf(stderr, "[mvp:%d] new_tasks->total:%d\n", mvp->rank, mvp->tasklist[MPCOMP_TASK_TYPE_NEW]->total);
			 mpcomp_task_list_free(mvp->tasklist[MPCOMP_TASK_TYPE_NEW]);
		    }
		    
		    /* If the node correspond to the untied list depth, release the data structure */
		    if (n->depth + 1 == t->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]) {
			 //fprintf(stderr, "[mvp:%d] untied_tasks->total:%d\n", mvp->rank, mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED]->total);
			 mpcomp_task_list_free(mvp->tasklist[MPCOMP_TASK_TYPE_UNTIED]);
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
void __mpcomp_task_list_infos_exit()
{
     mpcomp_thread_t *t;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     __mpcomp_task_list_infos_exit_r(t->mvp->root);
}


void __mpcomp_task_exit()
{
     mpcomp_thread_t *t;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     if ((t->num_threads > 1 && t->tasking_init_done) || t->children_instance->mvps[0]->threads[0].tasking_init_done) {
	  //fprintf(stderr, "[t:%lu %p] __mpcomp_task_exit TASK INIT DONE\n", t->rank, t);
	  __mpcomp_task_list_infos_exit();
     } else {
	  //fprintf(stderr, "[t:%lu %p] __mpcomp_task_exit TASK INIT NOT DONE\n", t->rank, t);
     }
}

/* Recursive call for checking neighbourhood from node n */
void __mpcomp_task_check_neighbourhood_r(mpcomp_node_t *n)
{
     mpcomp_thread_t *t;
     int i, j;

     sctk_assert(n != NULL);

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     for (j=1; j<t->team->instance->tree_level_size[n->depth]; j++) {
	  fprintf(stderr, "neighbour n°%d of %d: %d\n", j, n->tree_array_rank, 
		  mpcomp_get_neighbour(n->tree_array_rank, j));
     }

     switch (n->child_type) {
     case MPCOMP_CHILDREN_NODE:
	  for (i=0; i<n->nb_children; i++) {
	       mpcomp_node_t *child = n->children.node[i];

	       /* Call recursively for all children nodes */
	       __mpcomp_task_check_neighbourhood_r(child);
	  }
	  break;
	  
     case MPCOMP_CHILDREN_LEAF:
	  for (i=0; i<n->nb_children; i++) {
	       /* All the children are leafs */
	       
	       mpcomp_mvp_t *mvp;
	       
	       mvp = n->children.leaf[i];
	       sctk_assert(mvp != NULL);

	       for (j=1; j<t->team->instance->tree_level_size[n->depth+1]; j++) {
		    fprintf(stderr, "neighbour n°%d of %d: %d\n", j, mvp->tree_array_rank, 
			    mpcomp_get_neighbour(mvp->tree_array_rank, j));
	       }
	  }
	  break ;
	  
     default:
	  sctk_nodebug("not reachable");
     }
     
     return;
}

/* Check all neighbour of all nodes */
void __mpcomp_task_check_neighbourhood()
{
     mpcomp_thread_t *t;

     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     __mpcomp_task_check_neighbourhood_r(t->mvp->root);
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

	  /* Allocate the default current task (no func, no data, no parent) */
#if MPCOMP_MALLOC_ON_NODE
	  t->current_task = sctk_malloc_on_node(sizeof(struct mpcomp_task_s), t->mvp->father->id_numa);
#else
	  t->current_task = sctk_malloc(sizeof(struct mpcomp_task_s));
#endif /* MPCOMP_MALLOC_ON_NODE */

	  __mpcomp_task_init(t->current_task, NULL, NULL, t);
	  t->current_task->parent = NULL;

	  /* Allocate private task data structures */
#if MPCOMP_MALLOC_ON_NODE
	  t->tied_tasks = sctk_malloc_on_node(sizeof(struct mpcomp_task_list_s), t->mvp->father->id_numa);
#else
	  t->tied_tasks = sctk_malloc(sizeof(struct mpcomp_task_list_s));
#endif /* MPCOMP_MALLOC_ON_NODE */
	  mpcomp_task_list_new (t->tied_tasks);
	  
	  t->tasking_init_done = 1;
     }

     if (t->num_threads > 1 && sctk_atomics_load_int(&(t->team->tasklist_init_done)) == 0) {
	  /* Executed only when there are multiple threads */

	  volatile int *done = (volatile int *) &(t->team->tasking_init_done);	  

	  if (sctk_test_and_set(done) == 0) { 
	       /* Executed only one time per process */
	       
	       __mpcomp_task_list_infos_init();
	       //__mpcomp_task_check_neighbourhood();
	       sctk_atomics_store_int(&(t->team->tasklist_init_done), 1);
	  }
	  
	  /* All threads wait until allocation is done */
	  while (sctk_atomics_load_int(&(t->team->tasklist_init_done)) == 0)
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
     sctk_spinlock_unlock(&(parent->children_lock));	       
}

static int
__mpcomp_task_check_circular_link(struct mpcomp_task_s *task)
{
     struct mpcomp_task_s *parent = task->parent;

     while (parent) {
	  parent = parent->parent;
	  if (parent == task)
	       return 1;
     }

     return 0;          
}

/* Return list of 'type' tasks contained in element of rank 'globalRank' */
static inline struct mpcomp_task_list_s *mpcomp_task_get_list(int globalRank, mpcomp_tasklist_type_t type)
{
     mpcomp_thread_t *t;
     struct mpcomp_task_list_s *list;

     /* Retrieve the current thread information */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     if (mpcomp_is_leaf(globalRank)) {
	  mpcomp_mvp_t *mvp = (mpcomp_mvp_t *) (t->mvp->tree_array[globalRank]);
	  list = mvp->tasklist[type];
     } else {
	  mpcomp_node_t *n = t->mvp->tree_array[globalRank];
	  list = n->tasklist[type];
     }
	 
     return list;
}


/* 
 * Creation of an OpenMP task 
 * Called when encountering an 'omp task' construct 
 */
void __mpcomp_task(void *(*fn) (void *), void *data, void (*cpyfn) (void *, void *),
		   long arg_size, long arg_align, bool if_clause, unsigned flags)
{
     mpcomp_thread_t *t;
     struct mpcomp_task_list_s *new_list;
     int nb_delayed;

     /* Initialize the OpenMP environment (called several times, but really executed
      * once) */
     __mpcomp_init ();

     /* Initialize the OpenMP task environment (called several times, but really 
      * executed once per worker thread) */
     __mpcomp_task_infos_init();
     
     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);	 

     new_list = mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_NEW], 
				     MPCOMP_TASK_TYPE_NEW);

     if (t->num_threads == 1)
     	  nb_delayed = 0;
     else
     	  nb_delayed = sctk_atomics_load_int(&(new_list->nb_elements));

     if (!if_clause
	 || (t->current_task && mpcomp_task_property_isset (t->current_task->property, MPCOMP_TASK_FINAL))
	 || t->num_threads == 1
	 || nb_delayed > MPCOMP_TASK_MAX_DELAYED
	 /*|| (t->current_task && t->current_task->depth > MPCOMP_TASK_NESTING_MAX)*/) {
	  /* Execute directly */

	  struct mpcomp_task_s task;

	  __mpcomp_task_init (&task, NULL, NULL, t);
	  mpcomp_task_set_property (&(task.property), MPCOMP_TASK_UNDEFERRED);
      
	  if ((t->current_task
	       && mpcomp_task_property_isset (t->current_task->property, MPCOMP_TASK_FINAL))
	      || (flags & 2)) {
	       /* If its parent task is final, the new task must be final too */
	       mpcomp_task_set_property (&(task.property), MPCOMP_TASK_FINAL);
	  }
	       
	  /* Current task is the new task which will be executed immediately */
	  t->current_task = &task;
	  sctk_assert(task.thread == NULL);
	  task.thread = t;

	  if (cpyfn != NULL) {
	       /* If cpyfn is given, use it to copy args */
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
	  mpcomp_task_clear_parent(&task);
	  TODO("Add cleaning for the task data structures")
     } else {
	  struct mpcomp_task_s *task;
	  struct mpcomp_task_s *parent;
	  char *data_cpy;

	  /* The new task may be delayed, so copy arguments in a buffer */
#if MPCOMP_MALLOC_ON_NODE
	  task = sctk_malloc_on_node(sizeof (struct mpcomp_task_s) + arg_size + arg_align - 1, t->mvp->father->id_numa);
#else
	  task = sctk_malloc(sizeof (struct mpcomp_task_s) + arg_size + arg_align - 1);
#endif /* MPCOMP_MALLOC_ON_NODE */
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
	  __mpcomp_task_init (task, fn, data_cpy, t);
	  mpcomp_task_set_property (&(task->property), MPCOMP_TASK_TIED);
	  if (flags & 2) {
	       mpcomp_task_set_property (&(task->property), MPCOMP_TASK_FINAL);
	  }

	  parent = task->parent;

	  assert(__mpcomp_task_check_circular_link(task) == 0);

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
	  mpcomp_task_list_pushtotail(new_list, task);
	  mpcomp_task_list_unlock(new_list);
     }
}

#if MPCOMP_TASK_LARCENY_MODE == 0

/* Steal a task in the 'type' task list of node identified by 'globalRank' */
static inline struct mpcomp_task_s *mpcomp_task_steal(struct mpcomp_task_list_s *list)
{
     struct mpcomp_task_s *task = NULL;

     sctk_assert(list != NULL);
     mpcomp_task_list_lock(list);
     task = mpcomp_task_list_popfromtail(list);
     mpcomp_task_list_unlock(list);
	 
     return task;
}

static inline void  mpcomp_task_allocate_larceny_order(mpcomp_thread_t *t)
{
     int nbMaxLists = t->team->instance->tree_level_size[t->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]];
     
     t->larceny_order = mpcomp_malloc(1, (nbMaxLists-1) * sizeof(int), t->mvp->father->id_numa);
}

/* Return the ith victim for task stealing initiated from element at 'globalRank' */
static inline int mpcomp_task_get_victim(int globalRank, int i)
{
     mpcomp_thread_t *highwayman;
     int larcenyMode;
     int victim;
     int *firstRank;
     long int value;
     int depth;
     int j;
     struct drand48_data *randBuffer;
     int nbVictims;
     int *larcenyOrder;

     /* Retrieve the information (microthread structure and current region) */
     highwayman = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(highwayman != NULL);

     larcenyMode = highwayman->team->task_larceny_mode;

     switch (larcenyMode) {

     case MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL:

	  victim = mpcomp_get_neighbour(globalRank, i);
	  break;

     case MPCOMP_TASK_LARCENY_MODE_RANDOM:

	  if (mpcomp_is_leaf(globalRank)) {
	       mpcomp_mvp_t *mvp =(mpcomp_mvp_t *) highwayman->mvp->tree_array[globalRank]; 
	       lrand48_r(mvp->tasklist_randBuffer, &value);
	       depth = mvp->father->depth + 1;
	  } else {
	       mpcomp_node_t *n = highwayman->mvp->tree_array[globalRank]; 
	       lrand48_r(n->tasklist_randBuffer, &value);
	       depth = n->depth;
	  }
	  victim = (value % (highwayman->team->instance->tree_level_size[depth] - 1)) 
	       + highwayman->team->instance->tree_array_first_rank[depth];
	  break;
 
     case MPCOMP_TASK_LARCENY_MODE_RANDOM_ORDER:

	  if (mpcomp_is_leaf(globalRank)) {
	       mpcomp_mvp_t *mvp =(mpcomp_mvp_t *) highwayman->mvp->tree_array[globalRank]; 
	       randBuffer = mvp->tasklist_randBuffer;
	       depth = mvp->father->depth + 1;
	  } else {
	       mpcomp_node_t *n = highwayman->mvp->tree_array[globalRank]; 
	       randBuffer = n->tasklist_randBuffer;
	       depth = n->depth;
	  }

	  nbVictims = highwayman->team->instance->tree_level_size[depth] - 1;
	  
	  if (!highwayman->larceny_order) {
	       mpcomp_task_allocate_larceny_order(highwayman);

	       larcenyOrder = highwayman->larceny_order;

	       for (j=0; j<nbVictims; j++)
		    larcenyOrder[j] = j + highwayman->team->instance->tree_array_first_rank[depth];
	  } else {	  
	       larcenyOrder = highwayman->larceny_order;
	  }

	  if (i == 1) {
	       /* Initialization */
	       int n;
	       for (j=0, n=nbVictims; j<nbVictims; j++, n--) {
		    int k, tmp;

		    lrand48_r(randBuffer, &value);
		    k = value % n;
		    tmp = larcenyOrder[j];
		    larcenyOrder[j] = larcenyOrder[j+k];
		    larcenyOrder[j+k] = tmp;
	       }
	       //fprintf(stderr, "[t %lu]: larcenyOrder = [ ", highwayman->rank);
	       for (j=0; j<nbVictims; j++) {
		    fprintf(stderr, "[t %lu]: larcenyOrder[%d] = %d\n", highwayman->rank,
			    j, larcenyOrder[j]);
		    //fprintf(stderr, "%d ", larcenyOrder[j]);
	       }
	       //fprintf(stderr, "]\n");
	       
	  }
	  victim = highwayman->larceny_order[i];
	  break;

     case MPCOMP_TASK_LARCENY_MODE_ROUNDROBIN:
	  //highwayman->team->instance->tree_array_first_rank[]
	  break;

     case MPCOMP_TASK_LARCENY_MODE_PRODUCER:
	  break;

     case MPCOMP_TASK_LARCENY_MODE_PRODUCER_ORDER:
	  if (!highwayman->larceny_order)
	       mpcomp_task_allocate_larceny_order(highwayman);
	  break;

     default:
	  victim = globalRank;
	  break;	  
     }
     
     return victim;
}

/* Look in new and untied tasks lists of others */
static struct mpcomp_task_s * __mpcomp_task_larceny()
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
     larcenyMode = highwayman->team->task_larceny_mode;

     /* Check first for NEW tasklists, then for UNTIED tasklists */
     for (t=0; t<MPCOMP_TASK_TYPE_COUNT; t++) {
	  
	  /* Retrieve informations about task lists */
	  tasklistDepth = highwayman->team->tasklist_depth[t];
	  nbTasklists = highwayman->team->instance->tree_level_size[tasklistDepth];

	  //fprintf(stderr, "[t %lu] MPCOMP_LARCENY tour %d (nbTasklists = %d; tasklistDepth)\n", 
	  //highwayman->rank, t, nbTasklists, tasklistDepth);

	  if (nbTasklists > 1) {
	  /* If there are task lists in which we could steal tasks */

	       /* Try to steal inside the last stolen list*/
	       list = mvp->lastStolen_tasklist[t];
	       if (list) {
		    task = mpcomp_task_steal(list);
		    if (task){
			 //fprintf(stderr, "[t %lu] MPCOMP_LARCENY réussi\n", highwayman->rank);
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
	       
	       //fprintf(stderr, "[t %lu] MPCOMP_LARCENY tenté(nbVictims:%d)\n", 
	       //highwayman->rank, nbVictims);

	       /* Look for a task in all victims lists */
	       for (i=1; i<nbVictims+1; i++) {
		    int victim = mpcomp_task_get_victim(rank, i);
		    list = mpcomp_task_get_list(victim, t);
		    fprintf(stderr, "[mvp%d] t=%d list=%p, victim:%d \n", mvp->rank, t, list, victim);
		    task = mpcomp_task_steal(list);
		    if (task) {
			 mvp->lastStolen_tasklist[t] = list;
			 //fprintf(stderr, "[t %lu] MPCOMP_LARCENY réussi\n", highwayman->rank);
			 return task;
		    }
	       }
	  }
     }
     
     return NULL;
}
#endif /* MPCOMP_TASK_LARCENY_MODE == 0 */


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
#if MPCOMP_OVERFLOW_CHECKING
	  sctk_assert(i+k < n);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	  res[i] = res[i+k];
	  res[i+k] = tmp;
     }
}
#endif //MPCOMP_TASK_LARCENY_MODE == 1

static int set_order(int res[], int n, mpcomp_node_t *node, mpcomp_mvp_t *mvp, int new)
{
     //fprintf(stderr, "[t:%lu] set_order(%p, %d, %p, %p, %d)\n", ((mpcomp_thread_t *)sctk_openmp_thread_tls)->rank, res, n, node, mvp, new);

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
     
     /* Order the list according to their size */
     no = 0;
     for (i=0; i<n; i++) {
	  /* Search for every task list */
	  list = tab_list[i];
	  sctk_assert(list != NULL);

	  /* Get the number of tasks in the list */
	  mpcomp_task_list_lock(list);
	  nbe = sctk_atomics_load_int(&list->nb_elements);
	  mpcomp_task_list_unlock(list);

	  if (nbe > 0) {
	       /* If the list is not empty */

	       /* Find the position of the list in the current ordered array */
	       j=0;
#if MPCOMP_OVERFLOW_CHECKING
		    if (j<no)
			 sctk_assert(res[j] < n);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	       while (j<no && nbe <= nb_elements[res[j]]) {
		    j++;
#if MPCOMP_OVERFLOW_CHECKING
		    if (j<no)
			 sctk_assert(res[j] < n);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	       }

#if MPCOMP_OVERFLOW_CHECKING
//	       fprintf(stderr, "res[j=%d]=%d < n=%d\n",j, res[j], n);
	       sctk_assert(no < n)
;
	       sctk_assert(j < n);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	       /* Store the list   */
	       for (k=no; k>j; k--)
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
     int max_nb_lists;

     /* Retrieve the information (microthread structure and current region) */
     highwayman = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(highwayman != NULL);

     nb_newlists = highwayman->team->instance->tree_level_size[highwayman->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW]];
     nb_untiedlists = highwayman->team->instance->tree_level_size[highwayman->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED]];
     max_nb_lists = sctk_max(nb_newlists, nb_untiedlists);
     
     if (nb_newlists == 1 && nb_untiedlists == 1)
	  return NULL;

     if (!highwayman->larceny_order) {
#if MPCOMP_MALLOC_ON_NODE
	  highwayman->larceny_order = sctk_malloc_on_node(sizeof(int) * (max_nb_lists - 1), highwayman->mvp->father->id_numa);     
#else
	  highwayman->larceny_order = sctk_malloc(sizeof(int) * (max_nb_lists - 1));     
#endif /* MPCOMP_MALLOC_ON_NODE */
     }

     order = highwayman->larceny_order;

     if (nb_newlists > 1) {
	  if (highwayman->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW] < highwayman->mvp->father->depth + 1) {
	       /* New lists are at some node level */
	       
	       struct mpcomp_node_s *n;
	       
	       /* Retrieve the node containing the new list */
	       n = highwayman->mvp->father;
	       while (n->depth > highwayman->team->tasklist_depth[MPCOMP_TASK_TYPE_NEW] && n->father != NULL)
		    n = n->father;
	       
#if MPCOMP_OVERFLOW_CHECKING
	       sctk_assert(nb_newlists - 1 < max_nb_lists);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	       nbl = set_order(order, nb_newlists - 1, n, NULL, 1);

	       for (i=0; i<nbl; i++) {
		    list = n->dist_new_tasks[order[i]];
		    sctk_assert(list != NULL);
		    mpcomp_task_list_lock(list);
		    task = mpcomp_task_list_popfromhead(list);
		    mpcomp_task_list_unlock(list);
		    if (task != NULL)
			 return task;
	       }
	  } else {
	       /* New lists are at mvp level */
	       
	       struct mpcomp_mvp_s *mvp;
	       
	       mvp = highwayman->mvp;

#if MPCOMP_OVERFLOW_CHECKING
	       sctk_assert(nb_newlists - 1 < max_nb_lists);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	       nbl = set_order(order, nb_newlists - 1, NULL, mvp, 1);
	       
	       for (i=0; i<nbl; i++) {
		    list = mvp->dist_new_tasks[order[i]];
		    sctk_assert(list != NULL);
		    mpcomp_task_list_lock(list);
		    task = mpcomp_task_list_popfromhead(list);
		    mpcomp_task_list_unlock(list);
		    if (task != NULL)
			 return task;	       
	       }
	  }
     }

     if (nb_untiedlists > 1) {
	  if (highwayman->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED] < highwayman->mvp->father->depth + 1) {
	       /* Untied lists are at some node level */
	       
	       struct mpcomp_node_s *n;
	       
	       /* Retrieve the node containing the untied list */
	       n = highwayman->mvp->father;
	       while (n->depth > highwayman->team->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED] && n->father != NULL)
		    n = n->father;
	       
#if MPCOMP_OVERFLOW_CHECKING
	       sctk_assert(nb_untiedlists - 1 < max_nb_lists);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	       nbl = set_order(order, nb_untiedlists - 1, n, NULL, 0);
	       
	       for (i=0; i<nbl; i++) {
		    list = n->dist_untied_tasks[order[i]];
		    sctk_assert(list != NULL);
		    mpcomp_task_list_lock(list);
		    task = mpcomp_task_list_popfromhead(list);
		    mpcomp_task_list_unlock(list);
		    if (task != NULL)
			 return task;
	       }
	  } else {
	       /* Untied lists are at mvp level */
	       
	       struct mpcomp_mvp_s *mvp;
	       
	       mvp = highwayman->mvp;
	       
#if MPCOMP_OVERFLOW_CHECKING
	       sctk_assert(nb_untiedlists - 1 < max_nb_lists);
#endif /* MPCOMP_OVERFLOW_CHECKING */
	       nbl = set_order(order, nb_untiedlists - 1, NULL, mvp, 0);
	       
	       for (i=0; i<nbl; i++) {
		    list = mvp->dist_untied_tasks[order[i]];
		    sctk_assert(list != NULL);
		    mpcomp_task_list_lock(list);
		    task = mpcomp_task_list_popfromhead(list);
		    mpcomp_task_list_unlock(list);
		    if (task != NULL)
			 return task;
	       }
	  }
     }
     
     return NULL;
}
#endif /* MPCOMP_TASK_LARCENY_MODE == 1 || MPCOMP_TASK_LARCENY_MODE == 2 || MPCOMP_TASK_LARCENY_MODE == 3 */

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
     int i, wait;
     
     /* Initialize the OpenMP task environment (call several times, but really executed
      * once per worker thread) */
     __mpcomp_task_infos_init();
     
     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);	 

     /* If only one thread is running, tasks are not delayed. No need to schedule */
     if (t->num_threads == 1)
	  return;

     prev_task = t->current_task;
	  
     while (1) {
	  /* Find a remaining tied task */
	  list = t->tied_tasks;
	  task = mpcomp_task_list_popfromtail(list);
	  wait = 0;

	  if (task == NULL) {
	       /* If no task found previously, find a remaining untied task */
	       list = mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_UNTIED], 
					   MPCOMP_TASK_TYPE_UNTIED);
	       mpcomp_task_list_lock(list);
	       task = mpcomp_task_list_popfromtail(list);
	       mpcomp_task_list_unlock(list);
	  }
      
	  if (task == NULL) {
	       /* If no task found previously, find a remaining new task */
	       list = mpcomp_task_get_list(t->mvp->tasklistNodeRank[MPCOMP_TASK_TYPE_NEW], 
					   MPCOMP_TASK_TYPE_NEW);
	       mpcomp_task_list_lock(list);
	       task = mpcomp_task_list_popfromtail(list);
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
	  assert(task->thread == NULL);
	  task->thread = t;
	  task->func (task->data);
	  
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
	  
	  sctk_free(task);
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
     struct mpcomp_task_s *next_child = NULL;
     struct mpcomp_task_list_s *list;
     int ret;

     /* Initialize the OpenMP task environment (call several times, but really executed
      * once per worker thread) */
     __mpcomp_task_infos_init();
     
     /* Retrieve the information (microthread structure and current region) */
     t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
     sctk_assert(t != NULL);

     /* If only one thread is running, tasks are not delayed. No need to wait */
     if (t->num_threads == 1)
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
			 assert(task->thread == NULL);			      
			 assert(__mpcomp_task_check_circular_link(task) == 0);
			 task->thread = t;
			 task->func(task->data);

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
			 sctk_free(task);
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
