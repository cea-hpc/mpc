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

#include "sctk_debug.h"
#include "sctk_topology.h"
#include "mpcomp.h"
#include "mpcomp_internal.h"


/**
 * Check if the following parameters are correct to build a coherent tree
 */
int __mpcomp_check_tree_parameters(int n_leaves, int depth, int *degree)
{
     int i;
     int computed_n_leaves;

     /* Check if the number of leaves is strictly positive 
	and is lower than the total number of CPUs */
     if ((n_leaves <= 0) || (n_leaves > sctk_get_cpu_number()))
	  return 0;

     /* Check if the depth of the tree is strictly positive */
     if (depth <= 0)
	  return 0;

     /* Check if the number of children per node is strictly positive */
     for (i=0; i<depth; i++)
	  if (degree[i] <= 0)
	       return 0;

     /* Compute the total number of leaves according to the tree structure */
     computed_n_leaves = degree[0];
     for (i=1; i<depth; i++)
	  computed_n_leaves *= degree[i];

     /* Check if this number of coherent with the input argument */
     if (n_leaves != computed_n_leaves)
	  return 0;

     sctk_nodebug("__mpcomp_check_tree_parameters: Check OK");

     return 1;
}

/* Count the number of core below (so on his sub-tree) an hwloc object */
unsigned int __mpcomp_get_cpu_number_below(hwloc_obj_t obj)
{
     return hwloc_bitmap_weight(obj->cpuset);
}


/* 
 * Return the depth of 'type' object kind from topology 'topology' 
 * in 'new_topology'.  
 */
static unsigned __mpcomp_get_new_depth(hwloc_obj_type_t type, hwloc_topology_t topology, 
			      hwloc_topology_t new_topology)
{
     int nbobjs = hwloc_get_nbobjs_by_type(topology, type);
     int above_depth = hwloc_get_type_or_above_depth(new_topology, type);
     int below_depth = hwloc_get_type_or_below_depth(new_topology, type);
     
     /* Find the above or below level corresponding to the same amount of objects */
     if (nbobjs == hwloc_get_nbobjs_by_depth(new_topology, above_depth))
	  return above_depth;
     else if (nbobjs == hwloc_get_nbobjs_by_depth(new_topology, below_depth))
	  return below_depth;

     return 0;
}

/*
 * Compute and return a topological tree array where levels that
 * do not bring any hierarchical structure are ignored.
 * Also return the 'depth' of the array and return in the 'index' array, 
 * the three indexes of, respectively, physical threads level, cores level
 * and socket level. 'index' must be allocated (3 int sized).
 */
int *__mpcomp_compute_topo_tree_array(int *depth, int *index)
{
     hwloc_topology_t simple_topology;
     int d;
     int *tree;

     if (!depth || !index) {
	  sctk_error("__mpcomp_compute_topo_tree_array: Unable to compute tree (depth or index unallocated)");
	  return NULL;
     }

     /* Create a temporary topology */
     hwloc_topology_init(&simple_topology);
     hwloc_topology_set_custom(simple_topology);
     
     /* Duplicate current topology object */
     hwloc_custom_insert_topology(simple_topology, 
				  hwloc_get_obj_by_depth(simple_topology, 0, 0), 
				  sctk_get_topology_object(), NULL);

     /* Delete unessential levels */
     hwloc_topology_ignore_all_keep_structure(simple_topology);
 
     hwloc_topology_load(simple_topology);

     *depth = hwloc_topology_get_depth(simple_topology);

     /* Allocate and set the tree array */
     tree = malloc(*depth * sizeof(int));
     for (d = 0; d < *depth; d++)
	  tree[d] =  hwloc_get_nbobjs_by_depth(simple_topology, d);

     /* Set index of threads, cores and sockets levels */
     index[MPCOMP_TOPO_OBJ_THREAD] = __mpcomp_get_new_depth(HWLOC_OBJ_PU, 
							    sctk_get_topology_object(), 
							    simple_topology);
     index[MPCOMP_TOPO_OBJ_CORE] = __mpcomp_get_new_depth(HWLOC_OBJ_CORE, 
							  sctk_get_topology_object(), 
							  simple_topology);
     index[MPCOMP_TOPO_OBJ_SOCKET] = __mpcomp_get_new_depth(HWLOC_OBJ_SOCKET, 
							    sctk_get_topology_object(), 
							    simple_topology);

     /* Release temporary topology structure */
     hwloc_topology_destroy(simple_topology);

     return tree;
}


/* recursive function constructing automatically the open-mp tree corresponding to the topology */
void __mpcomp_build_auto_tree_recursive_bloc(mpcomp_instance_t *instance, int *order, 
					     hwloc_obj_t obj, mpcomp_node_t *father, 
					     int current_mvp, int id_loc)
{
     int i;	
     mpcomp_node_t *node = NULL;
     mpcomp_mvp_t *leaf = NULL;
	
     /* pass over 1-1 links of the hwloc tree */
     while ((obj->arity == 1) && (obj->type != HWLOC_OBJ_PU))
	  obj = obj->children[0];
		
     if (father == NULL) {   /* case root */
	  node = (mpcomp_node_t *) sctk_malloc(sizeof(mpcomp_node_t));
	  sctk_assert(node != NULL);
     
	  instance->root = node;

          sctk_debug("__mpcomp_build_auto_tree_recursive_bloc: root @ %p", instance->root);
		
	  node->father = NULL;
	  node->rank = -1;
	  node->depth = 0;
	  node->nb_children = obj->arity;
	  node->min_index = 0;
	  node->max_index = __mpcomp_get_cpu_number_below(obj) - 1;
	  node->lock = SCTK_SPINLOCK_INITIALIZER;
	  node->slave_running = 0;
	  
	  sctk_atomics_store_int(&(node->barrier), 0);

	  /* Chunks infos */
	  sctk_atomics_store_int(&(node->chunks_avail), MPCOMP_CHUNKS_AVAIL);
	  sctk_atomics_store_int(&(node->nb_chunks_empty_children), 0);
     
	  node->barrier_done = 0;
		
	  /* will be allocated by the first child */
	  node->children.node = NULL;
	  node->children.leaf = NULL;		
	  /* node->child_type is modified by children */
	  
	  node->type = MPCOMP_MYSELF_ROOT;
	  node->current_mvp = 0;
	  node->id_numa = 0;
	  
#if MPCOMP_TASK
	  node->untied_tasks = NULL;
	  node->new_tasks = NULL;
#if MPCOMP_TASK_LARCENY_MODE == 1
	  node->untied_rand_buffer = NULL;
	  node->new_rand_buffer = NULL;
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MPCOMP_TASK

     } else {   /* case leaf or node */
	  int target_vp = order[current_mvp];

   	  if (__mpcomp_get_cpu_number_below(obj) == 1) {   /* case leaf (mvp) */		
	       int i_thread;
	       int depth;
	       mpcomp_node_t *node_tmp;
	       int res;
		    
	       if ((father->children.node == NULL) && (father->children.leaf == NULL)) { /* Only the first child */
		    father->child_type = MPCOMP_CHILDREN_LEAF;
#if MPCOMP_MALLOC_ON_NODE
		    father->children.leaf = (mpcomp_mvp_t **) sctk_malloc_on_node(sizeof(mpcomp_mvp_t *) * father->nb_children, father->id_numa);
#else
		    father->children.leaf = (mpcomp_mvp_t **) sctk_malloc(sizeof(mpcomp_mvp_t *) * father->nb_children);
#endif //MPCOMP_MALLOC_ON_NODE
		    sctk_assert(father->children.leaf != NULL);
	       }

	       /* Allocate memory (on the right NUMA Node) */
#if MPCOMP_MALLOC_ON_NODE
	       instance->mvps[current_mvp] = (mpcomp_mvp_t *) sctk_malloc_on_node(sizeof(mpcomp_mvp_t), father->id_numa);
#else
	       instance->mvps[current_mvp] = (mpcomp_mvp_t *) sctk_malloc(sizeof(mpcomp_mvp_t));
#endif //MPCOMP_MALLOC_ON_NODE
	       sctk_assert(instance->mvps[current_mvp] != NULL);
	       
	       leaf = instance->mvps[current_mvp];

	       /* Get the set of registers */
	       sctk_getcontext (&(leaf->vp_context));
	       
	       /* Initialize the corresponding microVP (all but tree-related variables) */
	       /* TODO put the microVP initialization in a function somewhere. 
		* If possible where we could call a function to initialize each thread 
		*/
	       leaf->nb_threads = 0;
	       leaf->next_nb_threads = 0;
	       leaf->children_instance = instance;		  
	       leaf->rank = current_mvp;
               leaf->vp = target_vp;             
	       leaf->enable = 1;
				       
	       for (i_thread = 0; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP; i_thread++) {
		    int i_fordyn;
		    for (i_fordyn = 0; i_fordyn < MPCOMP_MAX_ALIVE_FOR_DYN+1; i_fordyn++) {
			 sctk_atomics_store_int(&(leaf->threads[i_thread].for_dyn_chunk_info[i_fordyn].remain), -1);
		    }
	       }
	       
	       depth = father->depth + 1;
#if MPCOMP_MALLOC_ON_NODE
	       leaf->tree_rank = (int *) sctk_malloc_on_node(depth * sizeof(int), father->id_numa);
#else
	       leaf->tree_rank = (int *) sctk_malloc(depth * sizeof(int));
#endif //MPCOMP_MALLOC_ON_NODE
	       sctk_assert(leaf->tree_rank != NULL);
	       
	       /* Set the tree_rank values (starting from leaves level) */
	       node_tmp = father;	       
	       leaf->tree_rank[depth-1] = id_loc;
	       for(i=depth-2; i>=0; i--) {
		    leaf->tree_rank[i] = node_tmp->rank;
		    node_tmp = node_tmp->father;
	       }
		   
	       leaf->root = instance->root;
	       leaf->father = father;
	       
	       father->children.leaf[id_loc] = leaf;
	       
	       if (current_mvp == 0) {   /*  case mvp : root */
		    leaf->type = MPCOMP_MYSELF_ROOT;
		    leaf->to_run = instance->root;

	       } else { 
		    sctk_thread_attr_t __attr;

		    /* Initialize the mvp thread parameters */
		    sctk_thread_attr_init(&__attr);	       
		    sctk_thread_attr_setbinding(& __attr, target_vp);	       
		    sctk_thread_attr_setstacksize(&__attr, mpcomp_global_icvs.stacksize_var);

		    if (id_loc == 0) {   /*  case mvp : node */		    
			 mpcomp_node_t *to_run = leaf->father;
			 
			 leaf->type = MPCOMP_MYSELF_NODE;
			 while (to_run->rank == 0)
			      to_run = to_run->father;
			 leaf->to_run = to_run;
							
			 res = sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), 
							&__attr, mpcomp_slave_mvp_node, 
							instance->mvps[current_mvp]);		
		    } else {   /*  case mvp : leaf */
			 leaf->type = MPCOMP_MYSELF_LEAF;
			 res = sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), 
							&__attr, mpcomp_slave_mvp_leaf, 
							instance->mvps[current_mvp]);
			 sctk_assert (res == 0);
			 leaf->to_run = NULL;
		    }
		    
		    sctk_thread_attr_destroy(&__attr);
	       }
			
#if MPCOMP_TASK
	       leaf->untied_tasks = NULL;
	       leaf->new_tasks = NULL;
#if MPCOMP_TASK_LARCENY_MODE == 1
	       leaf->untied_rand_buffer = NULL;
	       leaf->new_rand_buffer = NULL;
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MPCOMP_TASK

	  } else {   /* case node */
	    int id_numa = 0;
#if MPCOMP_MALLOC_ON_NODE
               id_numa = sctk_get_node_from_cpu(target_vp);
	       node = (mpcomp_node_t *) sctk_malloc_on_node(sizeof(mpcomp_node_t), id_numa);
#else
	       node = (mpcomp_node_t *) sctk_malloc(sizeof(mpcomp_node_t));
#endif // MPCOMP_MALLOC_ON_NODE
	       node->id_numa = id_numa;
	       sctk_assert( node != NULL );
	       
	       if ((father->children.node == NULL) && (father->children.leaf == NULL)) {   /* Only the first child */
		    father->child_type = MPCOMP_CHILDREN_NODE;
#if MPCOMP_MALLOC_ON_NODE
		    father->children.node = (mpcomp_node_t **) sctk_malloc_on_node(sizeof(mpcomp_node_t *) * father->nb_children, father->id_numa);
#else
		    father->children.node = (mpcomp_node_t **) sctk_malloc(sizeof(mpcomp_node_t *) * father->nb_children);
#endif //MPCOMP_MALLOC_ON_NODE
		    sctk_assert( father->children.node != NULL );
	       }
	       
	       father->children.node[id_loc] = node;
			
	       node->type = MPCOMP_MYSELF_NODE;
	       node->father = father;
	       node->rank = id_loc;
	       node->depth = father->depth + 1;
	       node->nb_children = obj->arity;

	       node->current_mvp = current_mvp;
			
	       node->min_index = node->current_mvp;
	       node->max_index = node->current_mvp + __mpcomp_get_cpu_number_below(obj) - 1;
						
	       node->lock = SCTK_SPINLOCK_INITIALIZER;
	       node->slave_running = 0;
	       sctk_atomics_store_int(&(node->barrier), 0);
	       /* Chunks infos*/
	       sctk_atomics_store_int(&(node->chunks_avail), MPCOMP_CHUNKS_AVAIL);
	       sctk_atomics_store_int(&(node->nb_chunks_empty_children), 0); 

	       node->barrier_done = 0;
	       
	       node->children.node = NULL;
	       node->children.leaf = NULL;
			
#if MPCOMP_TASK
	       node->untied_tasks = NULL;
	       node->new_tasks = NULL;
#if MPCOMP_TASK_LARCENY_MODE == 1
	       node->untied_rand_buffer = NULL;
	       node->new_rand_buffer = NULL;
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MPCOMP_TASK			
	  }
     }
     
     /* recursive call */
     if (node != NULL) { 
	  int cpu_nb_child, id_child, cpu_nb_total = 0;
	  
	  for (i=0; i<node->nb_children; i++) {
	       cpu_nb_child = __mpcomp_get_cpu_number_below(obj->children[i]);
	       id_child = cpu_nb_total + node->min_index;
	       cpu_nb_total += cpu_nb_child;
	       __mpcomp_build_auto_tree_recursive_bloc(instance, order, obj->children[i],
						       node, id_child, i);
	  }
     }
}


/*
 * Build the default tree.
 */
int __mpcomp_build_default_tree(mpcomp_instance_t *instance)
{
     /* Assuming that hwloc use a flat representation of nested NUMA nodes (no hierarchy) like in the example below :
	http://trac.mcs.anl.gov/projects/mpich2/browser/mpich2/trunk/src/pm/hydra/tools/bind/hwloc/hwloc/tests/linux/256ia64-64n2s2c.output?rev=7614 */
     int nb_cpus;
     int *order;
     int current_mpc_vp;

     sctk_nodebug("__mpcomp_build_auto_tree begin"); 
     
     /* Retrieve the number of cores of the machine */
     nb_cpus = sctk_get_cpu_number();
     
     order = sctk_malloc(nb_cpus * sizeof(int));
     sctk_assert(order != NULL);
     
     /* Get the current VP number */
     current_mpc_vp = sctk_thread_get_vp();

     sctk_nodebug("__mpcomp_build_default_tree: current_mpc_vp=%d", current_mpc_vp);
     
     /* TODO So far, we do not fully support when the OpenMP instance is created from any VP */
     //sctk_assert(current_mpc_vp == 0);
     sctk_get_neighborhood(current_mpc_vp, nb_cpus, order);
     
     __mpcomp_build_auto_tree_recursive_bloc(instance, order, 
					     hwloc_get_root_obj(sctk_get_topology_object()),
					     NULL, 0, 0);
     
     sctk_free(order);
     
     sctk_nodebug("__mpcomp_build_auto_tree done"); 
	
     __mpcomp_print_tree(instance);
     
     return 1;
}


/*
 * Build a tree according to three parameters.
 */
int __mpcomp_build_tree( mpcomp_instance_t * instance, int n_leaves, int depth, int * degree ) {
	  mpcomp_node_t * root; /* Root of the tree */
	  int current_mpc_vp; /* Current VP where the master is */
	  int current_mvp; /* Current microVP we are dealing with */
	  int * order; /* Topological sort of VPs where order[0] is the current VP */
	  mpcomp_stack_t * s; /* Stack used to create the tree with a DFS */
	  int max_stack_elements; /* Max elements of the stack computed thanks to depth and degrees */
	  int previous_depth; /* What was the previously stacked depth (to check if depth is increasing or decreasing on the stack) */
	  mpcomp_node_t * target_node; /* Target node to spin when creating the next mVP */
	  int i;

	  /* Check input parameters */
	  sctk_assert( instance != NULL );
	  sctk_assert( instance->mvps != NULL );
	  sctk_assert( __mpcomp_check_tree_parameters( n_leaves, depth, degree ) );

	  /* Compute the max number of elements on the stack */
	  max_stack_elements = 1;
	  for (i = 0; i < depth; i++)
	       max_stack_elements += degree[i];

	  sctk_nodebug( "__mpcomp_build_tree: max_stack_elements = %d", max_stack_elements );

	  /* Allocate memory for the node stack */
	  s = __mpcomp_create_stack( max_stack_elements );
	  sctk_assert( s != NULL );

	  /* Get the current VP number */
	  current_mpc_vp = sctk_thread_get_vp ();

	  /* Grab the right order to allocate microVPs (sctk_get_neighborhood) */
	  order = sctk_malloc( sctk_get_cpu_number () * sizeof( int ) );
	  sctk_assert( order != NULL );

	  sctk_get_neighborhood( current_mpc_vp, sctk_get_cpu_number (), order );

	  /* Build the tree of this OpenMP instance */

	  root = (mpcomp_node_t *)sctk_malloc( sizeof( mpcomp_node_t ) );
	  sctk_assert( root != NULL );

	  instance->root = root;

	  current_mvp = 0;
	  previous_depth = -1;
	  target_node = NULL;

	  /* Fill root information */
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = degree[0];
	  root->min_index = 0;
	  root->max_index = n_leaves;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running = 0;
	  sctk_atomics_store_int( &(root->barrier), 0 );

	  root->barrier_done = 0;

	  __mpcomp_push( s, root );

	  while ( !__mpcomp_is_stack_empty( s ) ) {
	       mpcomp_node_t * n;
	       int target_vp;
	       int target_numa;

	       n = __mpcomp_pop( s );
	       sctk_assert( n != NULL );

	       if (previous_depth == -1 ) {
		    target_node = root;
	       } else {
		    if ( n->depth < previous_depth ) {
			 target_node = n;
		    }
	       }
	       previous_depth = n->depth;

	       target_vp = order[ current_mvp ];
#if MPCOMP_MALLOC_ON_NODE
	       target_numa = sctk_get_node_from_cpu( target_vp );
#else
	       target_numa = 0;
#endif //MPCOMP_MALLOC_ON_NODE

#if MPCOMP_TASK
	       n->untied_tasks = NULL;
	       n->new_tasks = NULL;
#if MPCOMP_TASK_LARCENY_MODE == 1
	       n->untied_rand_buffer = NULL;
	       n->new_rand_buffer = NULL;
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MPCOMP_TASK

	       if ( n->depth == depth - 1 ) { 
		    int i_thread;
		    
		    /* Children are leaves */
		    n->child_type = MPCOMP_CHILDREN_LEAF;
		    n->children.leaf = (mpcomp_mvp_t **) sctk_malloc_on_node(
			 n->nb_children * sizeof(mpcomp_mvp_t *), target_numa);

		    for ( i = 0; i < n->nb_children; i++ ) {
			 /* Allocate memory (on the right NUMA Node) */
			 instance->mvps[current_mvp] = (mpcomp_mvp_t *) sctk_malloc_on_node(
			      sizeof(mpcomp_mvp_t), target_numa);

			 /* Get the set of registers */
			 sctk_getcontext(&(instance->mvps[current_mvp]->vp_context));

			 /* Initialize the corresponding microVP (all but tree-related variables) */
			 instance->mvps[current_mvp]->children_instance = instance;
			 instance->mvps[current_mvp]->nb_threads = 0;
			 instance->mvps[current_mvp]->next_nb_threads = 0;
			 instance->mvps[current_mvp]->rank = current_mvp;
			 instance->mvps[current_mvp]->vp = target_vp;
			 instance->mvps[current_mvp]->enable = 1;
			 instance->mvps[current_mvp]->tree_rank = 
			      (int *)sctk_malloc_on_node( depth*sizeof(int), target_numa );
			 sctk_assert( instance->mvps[current_mvp]->tree_rank != NULL );

			 instance->mvps[current_mvp]->tree_rank[ depth - 1 ] = i;

			 for (i_thread = 0; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP; i_thread++) {
			      int i_fordyn;
			      for (i_fordyn = 0; i_fordyn < MPCOMP_MAX_ALIVE_FOR_DYN+1; i_fordyn++) {
				   sctk_atomics_store_int(&(instance->mvps[current_mvp]->threads[i_thread].for_dyn_chunk_info[i_fordyn].remain), -1);
			      }
			 }

#if MPCOMP_TASK
			 instance->mvps[current_mvp]->untied_tasks = NULL;
			 instance->mvps[current_mvp]->new_tasks = NULL;
#if MPCOMP_TASK_LARCENY_MODE == 1
			 instance->mvps[current_mvp]->untied_rand_buffer = NULL;
			 instance->mvps[current_mvp]->new_rand_buffer = NULL;
#endif //MPCOMP_TASK_LARCENY_MODE == 1
#endif //MPCOMP_TASK

			 mpcomp_node_t * current_node = n;
			 while ( current_node->father != NULL ) {
			      instance->mvps[current_mvp]->
				   tree_rank[ current_node->depth - 1 ] = current_node->rank;

			      current_node = current_node->father;
			 }

			 instance->mvps[current_mvp]->root = root;
			 instance->mvps[current_mvp]->father = n;

			 n->children.leaf[i] = instance->mvps[current_mvp];

			 sctk_thread_attr_t __attr;
			 int res;

			 sctk_thread_attr_init(&__attr);
			 sctk_thread_attr_setbinding (& __attr, target_vp );
			 sctk_thread_attr_setstacksize (&__attr, mpcomp_global_icvs.stacksize_var);

			 /* User thread create... */
			 instance->mvps[current_mvp]->to_run = target_node;

			 if ( target_node != root ) {
			      if ( i == 0 ) {
				   sctk_assert( target_node != NULL );
				   /* The first child is spinning on a node */
				   instance->mvps[current_mvp]->to_run = target_node;
				   res = 
					sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), 
								 &__attr, mpcomp_slave_mvp_node, instance->mvps[current_mvp]);
				   sctk_assert (res == 0);
			      }
			      else {
				   /* The other children are regular leaves */
				   instance->mvps[current_mvp]->to_run = NULL;
				   res = 
					sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), 
								 &__attr, mpcomp_slave_mvp_leaf, instance->mvps[current_mvp]);
				   sctk_assert (res == 0);
			      }
			 } else {
			      /* Special case when depth==1, the current node is root */
			      if ( n == root ) {
				   target_node = NULL;
				   sctk_assert( i == 0 );
			      } else {
				   target_node = n;
			      }
			 }

			 sctk_thread_attr_destroy(&__attr);

			 current_mvp++;

			 /* Recompute the target vp/numa */
			 target_vp = order[ current_mvp ];
#if MPCOMP_MALLOC_ON_NODE
			 target_numa = sctk_get_node_from_cpu( target_vp );
#else
			 target_numa = 0;
#endif //MPCOMP_MALLOC_ON_NODE

			 /* We reached the leaves */
			 previous_depth++;
		    }
	       } else {
		    /* Children are nodes */
		    n->child_type = MPCOMP_CHILDREN_NODE;
		    n->children.node= (mpcomp_node_t **) sctk_malloc_on_node( 
			 n->nb_children * sizeof( mpcomp_node_t * ), target_numa );

		    /* Traverse children in reverse order for correct ordering during the DFS */
		    for ( i = n->nb_children - 1; i >= 0; i-- ) {
			 mpcomp_node_t * n2;

			 n2 = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), target_numa );

			 n->children.node[ i ] = n2;

			 n2->father = n;
			 n2->rank = i;
			 n2->depth = n->depth+1;
			 n2->nb_children = degree[ n2->depth ];
			 n2->min_index = n->min_index + i * (n->max_index - n->min_index) / degree[ n->depth ];
			 n2->max_index = n->min_index + (i+1) * (n->max_index - n->min_index) / degree[ n->depth ] - 1;
			 n2->lock = SCTK_SPINLOCK_INITIALIZER;
			 n2->slave_running = 0;

			 sctk_atomics_store_int( &(n2->barrier), 0 );

			 n2->barrier_done = 0;

			 __mpcomp_push( s, n2 );

		    }
	       }
	  }


	  /* Free memory */
	  __mpcomp_free_stack( s );
	  free( s );
	  free( order );

	  __mpcomp_print_tree( instance );

	  return 0;
}

void __mpcomp_print_tree( mpcomp_instance_t * instance ) {

     mpcomp_stack_t * s;
     int i, j;


     sctk_assert( instance != NULL );

     /* TODO compute the real number of max elements for this stack */
     s = __mpcomp_create_stack( 2048 );
     sctk_assert( s != NULL );

     __mpcomp_push( s, instance->root );

     fprintf( stderr, "==== Printing the current OpenMP tree ====\n" );

     while ( !__mpcomp_is_stack_empty( s ) ) {
	  mpcomp_node_t * n;

	  n = __mpcomp_pop( s );
	  sctk_assert( n != NULL );

	  /* Print this node */

	  /* Add tab according to depth */
	  for ( i = 0; i < n->depth; i++ ) {
	       fprintf( stderr, "\t" );
	  }

	  fprintf( stderr, "Node %ld (@ %p)\n", n->rank, n );

	  switch( n->child_type ) {
	  case MPCOMP_CHILDREN_NODE:
	       for ( i = n->nb_children - 1; i >= 0; i-- ) {
		    __mpcomp_push( s, n->children.node[i] );
	       }
	       break;
	  case MPCOMP_CHILDREN_LEAF:
	       for ( i = 0; i < n->nb_children; i++ ) {
		    mpcomp_mvp_t * mvp;

		    mvp = n->children.leaf[i];
		    sctk_assert( mvp != NULL );

		    /* Add tab according to depth */
		    for ( j = 0; j < n->depth + 1; j++ ) {
			 fprintf( stderr, "\t" );
		    }

		    fprintf( stderr, "Instance @ %p Leaf %d rank %d @ %p vp %d spinning on %p", instance, i, mvp->rank, &mvp, mvp->vp, mvp->to_run ) ;

		    fprintf( stderr, " tree_rank @ %p", mvp->tree_rank ) ;
		    for ( j = 0 ; j < n->depth + 1 ; j++ ) {
			 fprintf( stderr, " j=%d, %d", j, mvp->tree_rank[j] ) ;
		    }


		    fprintf( stderr, "\n" );
	       }
	       break;
	  default:
	       //not_reachable();
	       sctk_nodebug("not reachable"); 
	  }
     }

     __mpcomp_free_stack( s );
     free( s );

     return;
}
