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
#warning "MPC_OPENMP overload hwloc_topology_dup"
/* Duplicate the entire topology 'oldtopology' into 'newtopology' */
int hwloc_topology_dup(hwloc_topology_t * newtopology, 
        hwloc_topology_t * oldtopology)
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
#else
#warning "MPC_OPENMP use regular hwloc_topology_dup"
#endif


static int mpcomp_get_global_index_from_cpu (hwloc_topology_t topo, const int vp)
{
     hwloc_topology_t globalTopology = sctk_get_topology_object();
     // hwloc_topology_t globalTopology = sctk_get_topology_full_object() ;
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
     err = hwloc_topology_dup(flatTopology, topology);
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

#if 0
     /* Duplicate 'topology' to flat topology */
     err = hwloc_topology_load(*flatTopology);
     if (err) {
	  sctk_debug("Error on loading topology");
	  return 1;
     }
#endif

     if (sctk_get_verbosity()>=3) {
         fprintf( stderr, "FLATTEN TOPOLOGY\n" ) ;
         sctk_print_specific_topology( stderr, *flatTopology) ;
     }

     return 0;
}

/*
 * Restrict the topology object of the current mpi task to 'nb_mvps' vps.
 * This function handles multiple PUs per core.
 */
int 
__mpcomp_restrict_topology(hwloc_topology_t *restrictedTopology, int nb_mvps)
{
     hwloc_topology_t topology;
     hwloc_cpuset_t cpuset;
     hwloc_obj_t obj;
     int taskRank, taskVp, err, i, nbvps ;
     int nb_cores, nb_pus, nb_mvps_per_core ;
     int mvp, core ;

     /* Check input parameters */
     sctk_assert( nb_mvps > 0 ) ;
     sctk_assert ( restrictedTopology ) ;

     /* Get the current MPI task rank */
     taskRank = sctk_get_task_rank();

     /* Initialize the final cpuset */
     cpuset = hwloc_bitmap_alloc();

     /* Get the current global topology (already restricted by MPC) */
     topology = sctk_get_topology_object();

     /* Grab the current VP and the number of VPs for the current task */
     taskVp = sctk_get_init_vp_and_nbvp(taskRank, &nbvps);

     /* We do not support more mVPs than VPs */
     sctk_assert( nb_mvps <= nbvps ) ;

     /* BEGIN DEBUG */
     if (sctk_get_verbosity()>=3) 
     {
         fprintf( stderr, 
                 "[[%d]] __mpcomp_restrict_topology: GENERAL TOPOLOGY (rank= %d, vp=%d, logical range=[%d,%d]) \n", 
                 taskRank, taskRank, taskVp, taskVp, taskVp + nbvps - 1 ) ;
         sctk_print_specific_topology( stderr, topology ) ;
     }
     /* END DEBUG */

     /* Get information on the current topology */
     nb_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
     nb_pus = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
     nb_mvps_per_core = nb_mvps / nb_cores;

     mvp = 0;
     /* Iterate through all cores */
     for (core = 0; core < nb_cores && mvp < nb_mvps; core++) 
     {
         int local_slot_size ;
         hwloc_cpuset_t core_cpuset;
         
         /* Compute the number of PUs to handle for this core */
         local_slot_size = nb_mvps_per_core;
         if ((nb_mvps % nb_cores) > core)
         {
             local_slot_size++;
         }

         /* Get the core cpuset */
         core_cpuset = (hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, core))->cpuset;

         /* Add the PUs we want from the core cpuset to our target cpuset */
         for (i = 0; i < local_slot_size; i++, mvp++) 
         {
             hwloc_obj_t obj;
             obj = hwloc_get_obj_inside_cpuset_by_type(topology, core_cpuset, HWLOC_OBJ_PU, i);
             hwloc_bitmap_or(cpuset, cpuset, obj->cpuset);
         }
     }

     /* Allocate topology object */
     if ((err = hwloc_topology_init(restrictedTopology))) return -1;

     /* Duplicate current topology object */
     if ((err = hwloc_topology_dup(restrictedTopology, topology))) return -1;

     /* Restrict topology */
     if ((err = hwloc_topology_restrict(*restrictedTopology, 
                     cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES))) return -1;      

     /* BEGIN DEBUG */
     if (sctk_get_verbosity()>=3) 
     {
         fprintf( stderr, 
                 "[[%d]] __mpcomp_restrict_topology: RESTRICTED TOPOLOGY\n"
               , taskRank ) ;
         sctk_print_specific_topology( stderr, *restrictedTopology ) ;
     }
     /* END DEBUG */

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
 * Compute and return a topological tree array where levels that
 * do not bring any hierarchical structure are ignored.
 * Also return the 'depth' of the array and return in the 'index' array, 
 * the three indexes of, respectively, physical threads level, cores level
 * and socket level. 'index' must be allocated (3 int sized).
 */
int * 
__mpcomp_compute_topo_tree_array(hwloc_topology_t topology, int *depth, int *index)
{
    int * degree ; /* Returned array */
    hwloc_const_cpuset_t topo_cpuset ;
    int topo_depth ;
    int tree_depth = 0 ;
    int i ;
    int save_n ;

    /* Grab the cpuset of the topology */
    topo_cpuset  = hwloc_topology_get_complete_cpuset( topology ) ;

    /* Get the depth of the topology */
    topo_depth = hwloc_topology_get_depth( topology );

    /* First traversal: compute the tree depth */
    save_n = 1 ;
    for ( i = 0 ; i < topo_depth ; i++ ) {
        int n ;

        /* Get the number of elements at depth 'i' for the target cores */
        n = hwloc_get_nbobjs_inside_cpuset_by_depth( topology, topo_cpuset, i ) ;
        /* If this number is different from the previous level,
           it means that there is a change in the number of children
           Record this difference as a new level of the target tree
          */
        if ( n != save_n ) {
            sctk_nodebug( "__mpcomp_compute_topo_tree_array:\t"
                    "Degree of depth %d -> %d", tree_depth, n/save_n ) ;

            tree_depth++ ;
            save_n = n ;
        }
    }

    sctk_debug( "__mpcomp_compute_topo_tree_array: Depth of tree = %d",
            tree_depth ) ;

    /* Allocate array for tree shape */
    degree = (int *)malloc( tree_depth * sizeof( int ) ) ;
    sctk_assert( degree ) ;

    /* Second traversal: Fill tree */
    int current_depth = 0 ;
    save_n = 1 ;
    for ( i = 0 ; i < topo_depth ; i++ ) {
        int n ;
        hwloc_obj_type_t type ;

        type = hwloc_get_depth_type( topology, i ) ;
        if ( type == HWLOC_OBJ_PU ) { 
            sctk_debug("__mpcomp_compute_topo_tree_array:\t"
                    "PU topo depth %d, tree depth %d", i, current_depth-1 ) ;
            index[MPCOMP_TOPO_OBJ_THREAD] = current_depth-1 ;
        }
        if ( type == HWLOC_OBJ_CORE ) { 
            sctk_debug("__mpcomp_compute_topo_tree_array:\t"
                    "CORE topo depth %d, tree depth %d", i, current_depth-1 ) ;
            index[MPCOMP_TOPO_OBJ_CORE] = current_depth-1 ;
        }
        if ( type == HWLOC_OBJ_SOCKET ) { 
            sctk_debug("__mpcomp_compute_topo_tree_array:\t"
                    "SOCKET topo depth %d, tree depth %d", i, current_depth-1 ) ;
            index[MPCOMP_TOPO_OBJ_SOCKET] = current_depth-1 ;
        }

        n = hwloc_get_nbobjs_inside_cpuset_by_depth( topology, topo_cpuset, i ) ;
        if ( n != save_n ) {
            degree[current_depth] = n/save_n ;
            current_depth++ ;
            save_n = n ;
        }
    }


    /* Update the tree depth */
    *depth = tree_depth ;

    /* Return the tree */
    return degree ;
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

	sctk_assert(instance != NULL);
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

#if MPCOMP_MIC
	instance->scatter_depth = index[MPCOMP_TOPO_OBJ_CORE] ;
#else /* MPCOMP_MIC */
	instance->scatter_depth = index[MPCOMP_TOPO_OBJ_SOCKET] ;	
#endif /* MPCOMP_MIC */
	instance->core_depth = index[MPCOMP_TOPO_OBJ_CORE] ;

	/* Check if the tree shape is correct */
	if ( !__mpcomp_check_tree_parameters( n_leaves, depth, degree ) ) {
        /* TODO put warning or failed if needed */
		/* Fall back to a flat tree */
		sctk_debug( "__mpcomp_build_default_tree: fall back to flat tree" ) ;
		depth = 1 ;
		n_leaves = sctk_get_cpu_number() ;
		degree[0] = n_leaves ;
		instance->scatter_depth = 0;
	}

	TODO("Check the tree in hybrid mode (not w/ sctk_get_cpu_number)")

	/* Build the default tree */
	__mpcomp_build_tree( instance, n_leaves, depth, degree ) ;

#if MPCOMP_MIC
	instance->nb_cores = 1;
	for ( i = 0; i < instance->core_depth; i++ )
	     instance->nb_cores *= degree[i];
	__mpcomp_compute_min_index( instance, instance->root, n_leaves );
#endif /* MPCOMP_MIC */

	sctk_nodebug("__mpcomp_build_auto_tree done"); 

	return 1;
}

#if MPCOMP_MIC
int __mpcomp_compute_min_index( mpcomp_instance_t * instance, mpcomp_node_t * n, int nthreads)
{
     int i, nthreads_per_core;

     sctk_assert(instance != NULL);
     sctk_assert(n != NULL);

     if (n == instance->root) {
	  instance->balanced_last_thread = -1;
	  instance->balanced_current_core = -1;
     }

     nthreads_per_core = nthreads / instance->nb_cores ;

     if ( n->depth == instance->core_depth )
	  instance->balanced_current_core++;

     if ( n->child_type == MPCOMP_CHILDREN_LEAF ) { 
	  int k = 0;

	  if (instance->balanced_current_core < instance->nb_cores % nthreads)
	       nthreads_per_core++;
	  
	  for ( i = 0; i < n->nb_children; i++ ) {
	       mpcomp_mvp_t * mvp;
	       
	       mvp = n->children.leaf[i];
	       sctk_assert( mvp != NULL );
	      
	       if (i < nthreads_per_core && ++instance->balanced_last_thread < nthreads)
		    /* mvp->min_index[MPCOMP_AFFINITY_BALANCED] = n->min_index[MPCOMP_AFFINITY_BALANCED] + i; */
		    mvp->min_index[MPCOMP_AFFINITY_BALANCED] = instance->balanced_last_thread;
	       else
		    mvp->min_index[MPCOMP_AFFINITY_BALANCED] = instance->balanced_last_thread + nthreads + (k++);
	  }
     } else {
	  for ( i = 0 ; i < n->nb_children ; i++ ) {
	       mpcomp_node_t  *n2;
	       
	       n2 = n->children.node[i];
	       sctk_assert(n2 != NULL);

	       /* n2->min_index[MPCOMP_AFFINITY_BALANCED] = n->min_index[MPCOMP_AFFINITY_BALANCED] + i * instance->nb_cores * nb_threads_per_core;  */
	       n2->min_index[MPCOMP_AFFINITY_BALANCED] = instance->balanced_last_thread + 1; 
	       __mpcomp_compute_min_index( instance, n2, nthreads);
	       n2->max_index[MPCOMP_AFFINITY_BALANCED] = instance->balanced_last_thread; 
	  }	  
     }

     return 0;
}
#endif /* MPCOMP_MIC */

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
	  sctk_assert( n_leaves == instance->nb_mvps ) ;

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
	  for (i=0; i<MPCOMP_AFFINITY_NB; i++) {
	       root->min_index[i] = 0;
	       root->max_index[i] = n_leaves;
	  }
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

			 int j, nb_scatter_units;
			 nb_scatter_units = 1;
			 for ( j = 0; j < instance->scatter_depth ; j++ )
			      nb_scatter_units *= degree[j]; 

			 /* Initialize the corresponding microVP (all but tree-related variables) */
			 instance->mvps[current_mvp]->nb_threads = 0;
			 instance->mvps[current_mvp]->next_nb_threads = 0;
			 instance->mvps[current_mvp]->rank = current_mvp;
			 instance->mvps[current_mvp]->vp = target_vp;
			 instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_COMPACT] = n->min_index[MPCOMP_AFFINITY_COMPACT]+i;
			 instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_SCATTER] = n->min_index[MPCOMP_AFFINITY_SCATTER] + i * nb_scatter_units; /* TODO: compute the real value */
			 instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_BALANCED] = n->min_index[MPCOMP_AFFINITY_BALANCED]+i; /* TODO: compute the real value */
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
			 sctk_thread_attr_setbinding (& __attr, 
			     mpcomp_get_global_index_from_cpu(
			       instance->topology, target_vp) );
			 sctk_thread_attr_setstacksize (&__attr, 
			     mpcomp_global_icvs.stacksize_var);

			 /* User thread create... */
			 instance->mvps[current_mvp]->to_run = target_node;
			 instance->mvps[current_mvp]->slave_running = 0 ;

			 if ( target_node != root ) {
			   sctk_nodebug( "__mpcomp_build_tree: Creating mVPs #%d on VP #%d (index core %d)", 
			       current_mvp, target_vp, mpcomp_get_global_index_from_cpu(instance->topology, target_vp)  ) ;
			      if ( i == 0 ) {
				   sctk_assert( target_node != NULL );
				   /* The first child is spinning on a node */
				   instance->mvps[current_mvp]->to_run = target_node;
				   res = 
					sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), 
								 &__attr, mpcomp_slave_mvp_node, instance->mvps[current_mvp]);
				   sctk_assert (res == 0);
				   // sctk_thread_yield();
			      }
			      else {
				   /* The other children are regular leaves */
				   instance->mvps[current_mvp]->to_run = NULL;
				   res = 
					sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), 
								 &__attr, mpcomp_slave_mvp_leaf, instance->mvps[current_mvp]);
				   sctk_assert (res == 0);
				   // sctk_thread_yield();
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
			 int min_index[MPCOMP_AFFINITY_NB] ;
			 int child_target_numa, j, nb_underlying_scatter_units, nb_cores, nb_threads_per_core;

			 nb_underlying_scatter_units = 1;
			 for ( j = n->depth + 1; j < instance->scatter_depth; j++ )
			      nb_underlying_scatter_units *= degree[j]; 

			 /* Compute the min rank of openmp thread in the 
			  * corresponding subtree */
			 min_index[MPCOMP_AFFINITY_COMPACT] = n->min_index[MPCOMP_AFFINITY_COMPACT] + 
			      i * (n->max_index[MPCOMP_AFFINITY_COMPACT] - n->min_index[MPCOMP_AFFINITY_COMPACT]) / 
			      degree[ n->depth ];
			 min_index[MPCOMP_AFFINITY_SCATTER] = n->min_index[MPCOMP_AFFINITY_SCATTER] + 
			      i * nb_underlying_scatter_units;
			 min_index[MPCOMP_AFFINITY_BALANCED] = n->min_index[MPCOMP_AFFINITY_BALANCED] + 
			      i * (n->max_index[MPCOMP_AFFINITY_BALANCED] - n->min_index[MPCOMP_AFFINITY_BALANCED]) / 
			      degree[ n->depth ];; /* TODO: compute the real value */

			 /* TODO: The NUMA allocation may be a constraint if we want to change the affinity during the execution */
			 child_target_numa = sctk_get_node_from_cpu_topology( instance->topology,  order[ min_index[mpcomp_global_icvs.affinity] ] ) ;

			 n2 = (mpcomp_node_t *)mpcomp_malloc(1, sizeof( mpcomp_node_t ), child_target_numa );

			 n->children.node[ i ] = n2;

			 n2->father = n;
			 n2->rank = i;
			 n2->depth = n->depth+1;
			 n2->nb_children = degree[ n2->depth ];
			 for ( j = 0 ; j < MPCOMP_AFFINITY_NB ; j++)
			      n2->min_index[j] = min_index[j];

			 n2->max_index[MPCOMP_AFFINITY_COMPACT] = n->min_index[MPCOMP_AFFINITY_COMPACT] + (i+1) * (n->max_index[MPCOMP_AFFINITY_COMPACT] - n->min_index[MPCOMP_AFFINITY_COMPACT]) / degree[ n->depth ] ;
			 n2->max_index[MPCOMP_AFFINITY_SCATTER] = n->min_index[MPCOMP_AFFINITY_SCATTER] + (i+1) * nb_underlying_scatter_units;
			 n2->max_index[MPCOMP_AFFINITY_BALANCED] = n->min_index[MPCOMP_AFFINITY_BALANCED] + (i+1) * (n->max_index[MPCOMP_AFFINITY_BALANCED] - n->min_index[MPCOMP_AFFINITY_BALANCED]) / degree[ n->depth ] ;
			 n2->instance = instance ;
			 n2->lock = SCTK_SPINLOCK_INITIALIZER;
			 n2->slave_running = 0;

			 sctk_atomics_store_int( &(n2->barrier), 0 );
			 sctk_atomics_store_int( &(n2->chunks_avail), 0 );
			 sctk_atomics_store_int( &(n2->nb_chunks_empty_children), 0 );

			 n2->barrier_done = 0;
			 n2->barrier_num_threads = 0;

			 n2->id_numa = child_target_numa ; 


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

      /* Print main information about the node */
	  fprintf( stderr, 
              "Node %ld (@ %p) -> NUMA %d, min/max "
              , n->rank, n, n->id_numa ) ;

      /* Print all min_index/max_index for each affinity */
      for ( i = 0 ; i < MPCOMP_AFFINITY_NB ; i++ ) {
          fprintf( stderr, "[%d]->%.2d/%.2d ",
                  i, n->min_index[i],
                  n->max_index[i] ) ;
      }

      /* Print main information about the node (cont.) */
      fprintf( stderr,
              "(barrier_num_threads=%ld)\n", 
              n->barrier_num_threads ) ;


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

		    fprintf( stderr, 
                    "Instance @ %p Leaf %d rank %d @ %p vp %d "
                    "spinning on %p", 
                    instance, i, mvp->rank, &mvp, mvp->vp, 
                    mvp->to_run ) ;

		    fprintf( stderr, " min:%d tree_rank @ %p", 
					mvp->min_index[mpcomp_global_icvs.affinity], mvp->tree_rank ) ;
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
