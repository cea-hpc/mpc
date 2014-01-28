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

#if HWLOC_API_VERSION < 0x00010800
/* Duplicate the entire topology 'oldtopology' into 'newtopology' */
int hwloc_topology_dup (hwloc_topology_t * newtopology, hwloc_topology_t *oldtopology)
{
     char *xmlBuffer; 
     int bufLen;
     int err;

     /* Save 'oldtopology' in a xml buffer */
     err = hwloc_topology_export_xmlbuffer(*oldtopology, &xmlBuffer, &bufLen);
     if (err) {
	  perror("Export topology in xmlbuffer");
	  return -1;
     }

     /* Copy the topology saved in xml buffer inside 'newtopology' */
     err = hwloc_topology_set_xmlbuffer(*newtopology, xmlBuffer, bufLen);
     if (err) {
	  perror("Set topology with xmlbuffer");
	  return -1;
     }
     
     /* Free allocated xml buffer */
     hwloc_free_xmlbuffer(*oldtopology, xmlBuffer);

     return 0;
}
#endif

static int mpcomp_get_global_index_from_cpu (hwloc_topology_t topo, const int vp)
{
     hwloc_topology_t globalTopology = sctk_get_topology_object();
     const hwloc_obj_t pu = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, vp);
     hwloc_obj_t obj;

     assume(pu);    
     obj = hwloc_get_pu_obj_by_os_index(globalTopology, pu->os_index);
 
    return obj->logical_index;
}



/*
 * Walk down the topology object 'topology' from 'obj' node
 * and return the number of leaves.
 */
static int get_number_of_leaves(hwloc_obj_t obj, hwloc_topology_t topology, int *ignoredTypes)
{
     if (obj->type == HWLOC_OBJ_PU) {
	  /* If the object is a at lowest level */

	  return 1;
     } else {
	  unsigned nbChildren = obj->arity;
	  
	  if (ignoredTypes[obj->children[0]->type] == 0) {
	       /* Children level is not ignored, so no need to go deeper */

	       return nbChildren;
	  } else {
	       unsigned i, nbLeaves = 0;
	       
	       for (i=0; i<nbChildren; i++) {
		    /* Get the real arity of every children and sum up */
		    nbLeaves += get_number_of_leaves(obj->children[i], topology, ignoredTypes);
	       }

	       return nbLeaves;
	  }
     }
}

/*
 * Configure the topology detection for 'flatTopology' in order to get
 * a symmetric tree (For each level, all node must have the same arity).
 */
int __mpcomp_flatten_topology(hwloc_topology_t topology, hwloc_topology_t *flatTopology)
{
     unsigned depth, type, topoDepth;
     int err;
     int ignoredTypes[HWLOC_OBJ_TYPE_MAX];

     /* Initialize flat topology object */
     err = hwloc_topology_init(flatTopology);
     if (err) {
	  sctk_debug("Error on initializing topology");
	  return 1;
     }

     /* Duplicate 'topology' to flat topology */
     err = hwloc_topology_dup(flatTopology, &topology);
     if (err) {
	  sctk_debug("Error on duplicating topology");
	  return 1;
     }

     topoDepth = hwloc_topology_get_depth(topology);

     /* Initialize the array of ignored of hwloc objects types */
     for (type=0; type<HWLOC_OBJ_TYPE_MAX; type++) {
	  ignoredTypes[type] = 0;
     }

     /* Walk the topology with a tree style */
     for (depth=topoDepth-2; depth>0; depth--) {
	  int i, nbObjs, nbChildren, nbLeaves;
	  hwloc_obj_t obj = NULL;

	  obj = hwloc_get_next_obj_by_depth(topology, depth, obj);	  
	  nbObjs = hwloc_get_nbobjs_by_depth(topology, depth);
	  nbChildren = get_number_of_leaves(obj, topology, ignoredTypes);

	  /* Walk all the objects at current depth */
	  for (i=1; i<nbObjs; i++) {
	       obj = hwloc_get_next_obj_by_depth(topology, depth, obj);
	       nbLeaves = get_number_of_leaves(obj, topology, ignoredTypes);
	       
	       if (nbLeaves != nbChildren) {
		    /* If two object of the topology haven't the same number of leaves */

		    assert(obj->type != HWLOC_OBJ_MACHINE);
		    ignoredTypes[obj->type] = 1;
		    
		    /* Remove this level from topology for the detection */
		    hwloc_topology_ignore_type(*flatTopology, obj->type);

		    /* Go to next level */
		    break;
	       }
	  }
     }

     /* Duplicate 'topology' to flat topology */
     err = hwloc_topology_load(*flatTopology);
     if (err) {
	  sctk_debug("Error on loading topology");
	  return 1;
     }

     return 0;
}

/*
 *  Restrict the topology object of the current mpi task to 'nb_mvps' vps.
 */
int __mpcomp_restrict_topology(hwloc_topology_t *restrictedTopology, int nb_mvps)
{
     hwloc_topology_t topology;
     hwloc_cpuset_t cpuset;
     int taskRank, taskVp, taskNbVp, err;

     taskRank = sctk_get_task_rank();

     /* Get the cpuset of current task */
     taskVp = sctk_get_init_vp(taskRank);
     taskNbVp = nb_mvps;
     cpuset = hwloc_bitmap_alloc();
     hwloc_bitmap_set_range(cpuset, taskVp, taskVp + taskNbVp - 1);

     topology = sctk_get_topology_object();

     /* Allocate topology object */
     if ((err = hwloc_topology_init(restrictedTopology))) {
	  sctk_debug("restrict_topology(): init topology error");
	  return -1;
     }

     /* Duplicate current topology object */
     if ((err = hwloc_topology_dup(restrictedTopology, &topology))) {
	  sctk_debug("restrict_topology(): dup topology error");
	  return -1;
     }

     /* Load topology */
     if ((err = hwloc_topology_load(*restrictedTopology))) {
	  sctk_debug("restrict_topology(): load topology error");
	  return -1;
     }
     
     /* Restrict topology */
     if ((err = hwloc_topology_restrict(*restrictedTopology, cpuset, 0))) {
	  sctk_debug("restrict_topology(): restrict topology error");
	  return -1;
     }     
    
     return 0;
}

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

int mpcomp_ignore_all_keep_structure(hwloc_topology_t *topology)
{
     unsigned type;
     for(type = HWLOC_OBJ_SYSTEM; type < HWLOC_OBJ_TYPE_MAX; type++)
	  if (type != HWLOC_OBJ_MACHINE)
	       hwloc_topology_ignore_type_keep_structure(*topology, type);

     return 0;
}

/*
 * Compute and return a topological tree array where levels that
 * do not bring any hierarchical structure are ignored.
 * Also return the 'depth' of the array and return in the 'index' array, 
 * the three indexes of, respectively, physical threads level, cores level
 * and socket level. 'index' must be allocated (3 int sized).
 */
int *__mpcomp_compute_topo_tree_array(hwloc_topology_t topology, int *depth, int *index)
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
     
     /* Duplicate current topology object */
     hwloc_topology_dup(&simple_topology, &topology);

     /* Delete unessential levels */
     mpcomp_ignore_all_keep_structure(&simple_topology);

     hwloc_topology_load(simple_topology);

     /* Remove 1 because we would like the depth including only nodes (not leaves) */
     *depth = hwloc_topology_get_depth(simple_topology) - 1;

     /* Allocate and set the tree array */
     tree = malloc(*depth * sizeof(int));
     for (d = 0; d < *depth; d++) {
	  tree[d] =  hwloc_get_obj_by_depth(simple_topology, d, 0)->arity;
     }

     /* Set index of threads, cores and sockets levels */
     index[MPCOMP_TOPO_OBJ_THREAD] = __mpcomp_get_new_depth(HWLOC_OBJ_PU, 
							    topology, 
							    simple_topology);

     index[MPCOMP_TOPO_OBJ_CORE] = __mpcomp_get_new_depth(HWLOC_OBJ_CORE, 
							  topology, 
							  simple_topology);

     index[MPCOMP_TOPO_OBJ_SOCKET] = __mpcomp_get_new_depth(HWLOC_OBJ_SOCKET, 
							    topology, 
							    simple_topology);

     /* Release temporary topology structure */
     hwloc_topology_destroy(simple_topology);

     return tree;
}


/*
 * Build the default tree.
 */
int __mpcomp_build_default_tree(mpcomp_instance_t *instance)
{
	int depth ;
	int index[3] ;
	int * degree ;
	int n_leaves ;
	int i ;

	sctk_nodebug("__mpcomp_build_auto_tree begin"); 

	sctk_assert(instance->topology != NULL);

	/* Get the default topology shape */
	degree = __mpcomp_compute_topo_tree_array(instance->topology, &depth, index ) ;

	/* Compute the number of leaves */
	n_leaves = 1 ;
	for ( i = 0 ; i < depth ; i++ ) {
		n_leaves *= degree[i] ; 
	}

	sctk_debug( "__mpcomp_build_default_tree: Building tree depth:%d, n_leaves:%d",
			depth, n_leaves ) ;
	for ( i = 0 ; i < depth ; i++ ) {
		sctk_debug( "__mpcomp_build_default_tree:\tDegree[%d] = %d", i, degree[i] ) ;
	}

	/* Check if the tree shape is correct */
	if ( !__mpcomp_check_tree_parameters( n_leaves, depth, degree ) ) {
		/* Fall back to a flat tree */
		sctk_debug( "__mpcomp_build_default_tree: fall back to flat tree" ) ;
		depth = 1 ;
		n_leaves = sctk_get_cpu_number() ;
		degree[0] = n_leaves ;
	}

	TODO("Check the tree in hybrid mode (not w/ sctk_get_cpu_number)")



	/* Build the default tree */
	__mpcomp_build_tree( instance, n_leaves, depth, degree ) ;

	sctk_nodebug("__mpcomp_build_auto_tree done"); 

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
	  int i, j ;
	  int nb_cpus; 

	  /* Check input parameters */
	  sctk_assert( instance != NULL );
	  sctk_assert( instance->mvps != NULL );
	  sctk_assert( __mpcomp_check_tree_parameters( n_leaves, depth, degree ) );

	  /* Fill instance paratemers regarding tree shape */
	  instance->tree_depth = depth ;
	  instance->tree_base = degree ;
	  TODO( "check that we can copy degree pointer instead of deep copy")
	  instance->tree_cumulative = mpcomp_malloc(0,
			  sizeof(int) * depth, 0 ) ;
	  for ( i = 0 ; i < depth-1 ; i++ ) {
		  instance->tree_cumulative[i] = 1 ;
		  for ( j = i+1 ; j < depth ; j++ ) {
			  instance->tree_cumulative[i] *= instance->tree_base[j] ;
		  }
	  }
	  instance->tree_cumulative[depth-1] = 1 ; 
#if MPCOMP_TASK     
     instance->tree_level_size = mpcomp_malloc(0, 
			 sizeof(int) * (instance->tree_depth + 1), 0);
     instance->tree_array_first_rank = mpcomp_malloc(0, 
			 sizeof(int) * (instance->tree_depth + 1), 0);
     instance->tree_level_size[0] = 1;
     instance->tree_array_first_rank[0] = 0;
     instance->tree_array_size = 1;
     for (i=1; i<instance->tree_depth + 1; i++) {
	  instance->tree_level_size[i] = instance->tree_level_size[i-1] * 
		  instance->tree_base[i-1];
	  instance->tree_array_size += instance->tree_level_size[i]; 
	  instance->tree_array_first_rank[i] = 
		  instance->tree_array_first_rank[i-1] + 
		  instance->tree_level_size[i-1];

	  sctk_debug( "__mpcomp_build_tree: FirstRank[%d]=%d; "
			  "tree_level_size[%d]=%d; tree_array_size=%d", 
			  i, instance->tree_array_first_rank[i], 
			  i, instance->tree_level_size[i], 
			  instance->tree_array_size);
     }
#endif /* MPCOMP_TASK */


	  /* Compute the max number of elements on the stack */
	  max_stack_elements = 1;
	  for (i = 0; i < depth; i++)
	       max_stack_elements += degree[i];

	  sctk_nodebug( "__mpcomp_build_tree: max_stack_elements = %d", max_stack_elements );

	  /* Allocate memory for the node stack */
	  s = __mpcomp_create_stack( max_stack_elements );
	  sctk_assert( s != NULL );

	  /* Get the current VP number */
	  //current_mpc_vp = sctk_thread_get_vp ();
	  current_mpc_vp = 0;

	  /* Get the number of CPUs */
	  nb_cpus = sctk_get_cpu_number_topology(instance->topology) ;

	  sctk_debug( "__mpcomp_build_tree: number of cpus: %d", nb_cpus ) ;

	  /* Grab the right order to allocate microVPs (sctk_get_neighborhood) */
	  order = sctk_malloc( nb_cpus * sizeof( int ) );
	  sctk_assert( order != NULL );

	  sctk_get_neighborhood_topology(instance->topology, current_mpc_vp, nb_cpus, order );

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
	  root->instance = instance ;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running = 0;
	  sctk_atomics_store_int( &(root->barrier), 0 );

	  root->barrier_done = 0;

	  root->id_numa = sctk_get_node_from_cpu_topology(instance->topology, current_mpc_vp ) ;

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
	       sctk_assert( target_vp < nb_cpus && target_vp >= 0 ) ;

	       target_numa = sctk_get_node_from_cpu_topology(instance->topology, target_vp );

#if MPCOMP_TASK
	       for (i=0; i<MPCOMP_TASK_TYPE_COUNT; i++) {
		    n->tasklist[i] = NULL;
	       }
	       n->tasklist_randBuffer = NULL;
#endif /* MPCOMP_TASK */

	       if ( n->depth == depth - 1 ) { 
		    int i_thread;
		    int i_task ;
		    
		    /* Children are leaves */
		    n->child_type = MPCOMP_CHILDREN_LEAF;
		    n->children.leaf = (mpcomp_mvp_t **) mpcomp_malloc(1,
			 n->nb_children * sizeof(mpcomp_mvp_t *), target_numa);

		    for ( i = 0; i < n->nb_children; i++ ) {

			    sctk_assert( current_mvp >= 0 && current_mvp < nb_cpus ) ;
			    sctk_assert( target_vp < nb_cpus && target_vp >= 0 ) ;

			    /* Recompute the target_numa, in case of target_vp has been updated */
			    target_numa = sctk_get_node_from_cpu_topology(instance->topology, target_vp );


			    /* Allocate memory (on the right NUMA Node) */
			 instance->mvps[current_mvp] = (mpcomp_mvp_t *) mpcomp_malloc(1,
			      sizeof(mpcomp_mvp_t), target_numa);

			 /* Get the set of registers */
			 sctk_getcontext(&(instance->mvps[current_mvp]->vp_context));

			 /* Initialize the corresponding microVP (all but tree-related variables) */
			 instance->mvps[current_mvp]->nb_threads = 0;
			 instance->mvps[current_mvp]->next_nb_threads = 0;
			 instance->mvps[current_mvp]->rank = current_mvp;
			 instance->mvps[current_mvp]->vp = target_vp;
			 instance->mvps[current_mvp]->min_index = n->min_index+i;
			 instance->mvps[current_mvp]->enable = 1;
			 instance->mvps[current_mvp]->tree_rank = 
			      (int *)mpcomp_malloc(1, depth*sizeof(int), target_numa );
			 sctk_assert( instance->mvps[current_mvp]->tree_rank != NULL );

			 instance->mvps[current_mvp]->tree_rank[ depth - 1 ] = i;

			 for (i_thread = 0; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP; i_thread++) {
				 mpcomp_local_icv_t icvs ;
				 __mpcomp_thread_init( 
						 &(instance->mvps[current_mvp]->threads[i_thread]),
						 icvs,
						 instance ) ;
				 instance->mvps[current_mvp]->threads[i_thread].mvp = 
					 instance->mvps[current_mvp] ;
			 }

#if MPCOMP_TASK
			 for (i_task=0; i_task<MPCOMP_TASK_TYPE_COUNT; i_task++) {
			      instance->mvps[current_mvp]->tasklist[i_task] = NULL;
			 }
			 instance->mvps[current_mvp]->tasklist_randBuffer = NULL;
#endif /* MPCOMP_TASK */

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
			 sctk_thread_attr_setbinding (& __attr, mpcomp_get_global_index_from_cpu(instance->topology, target_vp) );
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

			 /* Recompute the target vp */
			 target_vp = order[ current_mvp ];


			 /* We reached the leaves */
			 previous_depth++;
		    }
	       } else {
		    /* Children are nodes */
		    n->child_type = MPCOMP_CHILDREN_NODE;
		    n->children.node= (mpcomp_node_t **) mpcomp_malloc(1, 
			 n->nb_children * sizeof( mpcomp_node_t * ), target_numa );

		    /* Traverse children in reverse order for correct ordering during the DFS */
		    for ( i = n->nb_children - 1; i >= 0; i-- ) {
			 mpcomp_node_t * n2;
			 int min_index ;
			 int child_target_numa ;

			 /* Compute the min rank of openmp thread in the 
			  * corresponding subtree */
			 min_index = n->min_index + 
				 i * (n->max_index - n->min_index) / 
				 degree[ n->depth ];

			 child_target_numa = sctk_get_node_from_cpu_topology( instance->topology,  order[ min_index ] ) ;

			 n2 = (mpcomp_node_t *)mpcomp_malloc(1, sizeof( mpcomp_node_t ), child_target_numa );

			 n->children.node[ i ] = n2;

			 n2->father = n;
			 n2->rank = i;
			 n2->depth = n->depth+1;
			 n2->nb_children = degree[ n2->depth ];
			 n2->min_index = n->min_index + i * (n->max_index - n->min_index) / degree[ n->depth ];
			 n2->max_index = n->min_index + (i+1) * (n->max_index - n->min_index) / degree[ n->depth ] ;
			 n2->instance = instance ;
			 n2->lock = SCTK_SPINLOCK_INITIALIZER;
			 n2->slave_running = 0;

			 sctk_atomics_store_int( &(n2->barrier), 0 );

			 n2->id_numa = child_target_numa ; 

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

	 /* Skip the debug messages if not enough verbosity */
	 if (sctk_get_verbosity()<3)
		 return ;

     sctk_assert( instance != NULL );

     TODO("compute the real number of max elements for this stack")
     s = __mpcomp_create_stack( 2048 );
     sctk_assert( s != NULL );

     __mpcomp_push( s, instance->root );

	 TODO("port this function w/ new debug function sctk_debug_no_newline") 

     fprintf( stderr, "==== Printing the current OpenMP tree ====\n" );

	 /* Print general information about the instance and the tree */
	 fprintf( stderr, "Depth = %d, Degree = [",
			 instance->tree_depth ) ;
	 for ( i = 0 ; i < instance->tree_depth ; i++ ) {
		 if ( i != 0 ) {
			 fprintf( stderr, ", " ) ;
		 }
		 fprintf( stderr, "%d", instance->tree_base[i] ) ;
	 }
	 fprintf( stderr, "]\n" ) ;

     while ( !__mpcomp_is_stack_empty( s ) ) {
	  mpcomp_node_t * n;

	  n = __mpcomp_pop( s );
	  sctk_assert( n != NULL );

	  /* Print this node */

	  /* Add tab according to depth */
	  for ( i = 0; i < n->depth; i++ ) {
	       fprintf( stderr, "\t" );
	  }

	  fprintf( stderr, "Node %ld (@ %p) -> NUMA %d, min/max %d / %d\n", 
			  n->rank, n, n->id_numa, n->min_index, n->max_index );

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

		    fprintf( stderr, " min:%d tree_rank @ %p", 
					mvp->min_index, mvp->tree_rank ) ;
		    for ( j = 0 ; j < n->depth + 1 ; j++ ) {
			 fprintf( stderr, " j=%d, %d", j, mvp->tree_rank[j] ) ;
		    }


		    fprintf( stderr, "\n" );
	       }
	       break;
	  default:
	       not_reachable();
	  }
     }

     __mpcomp_free_stack( s );
     free( s );

     return;
}
