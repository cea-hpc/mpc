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
/* #   - TABOADA Hugo hugo.taboada.ocre@cea.fr                            # */
/* #                                                                      # */
/* ######################################################################## */

#include "sctk_debug.h"
#include "sctk_topology.h"
#include "mpcomp.h"
#include "sctk_alloc.h"

#include "mpcomp_types.h"
#include "mpcomp_stack.h"
#include "mpcomp_alloc.h"
#include "mpcomp_parallel_region.h"

#include "mpcomp_tree_structs.h"

/**
 * Check if the following parameters are correct to build a coherent tree
 */
static int __mpcomp_check_tree_parameters(int n_leaves, int depth,
                                          int *degree) {
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
  for (i = 0; i < depth; i++)
    if (degree[i] <= 0)
      return 0;

  /* Compute the total number of leaves according to the tree structure */
  computed_n_leaves = degree[0];
  for (i = 1; i < depth; i++)
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
 * Also return the 'depth' of the array.
 */
static int *__mpcomp_compute_topo_tree_array(hwloc_topology_t topology,
                                             int *depth) {
  int *degree; /* Returned array */
  hwloc_const_cpuset_t topo_cpuset;
  int topo_depth;
  int tree_depth;
  int i;
  int save_n;
  int current_depth;

  /* Grab the cpuset of the topology */
  topo_cpuset = hwloc_topology_get_complete_cpuset(topology);

  /* Get the depth of the topology */
  topo_depth = hwloc_topology_get_depth(topology);
  sctk_assert(topo_depth > 0);

  /* First traversal: compute the tree depth */
  tree_depth = 0;
  save_n = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, topo_cpuset,
                                                   topo_depth - 1);
  for (i = topo_depth - 1; i >= 0; i--) {
    int n;

    /* Get the number of elements at depth 'i' for the target cores */
    n = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, topo_cpuset, i);

    sctk_debug("__mpcomp_compute_topo_tree_array:\tComparing level %d: n=%d, "
               "save_n=%d",
               i, n, save_n);

    if ((n != save_n) && (save_n % n == 0)) {
      sctk_debug("__mpcomp_compute_topo_tree_array:\t"
                 "Degree of depth %d -> %d",
                 tree_depth, save_n / n);
      tree_depth++;
      save_n = n;
    }
  }

  sctk_debug("__mpcomp_compute_topo_tree_array: Depth of tree = %d",
             tree_depth);

  sctk_assert(tree_depth > 0);

  /* Allocate array for tree shape */
  degree = (int *)malloc(tree_depth * sizeof(int));
  sctk_assert(degree);

  /* Second traversal: Fill tree */
  current_depth = tree_depth - 1;
  save_n = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, topo_cpuset,
                                                   topo_depth - 1);
  for (i = topo_depth - 1; i >= 0; i--) {
    int n;

    n = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, topo_cpuset, i);
    if ((n != save_n) && (save_n % n == 0)) {
      sctk_assert(current_depth >= 0);
      degree[current_depth] = save_n / n;
      current_depth--;
      save_n = n;
    }
  }

  /* Update the tree depth */
  *depth = tree_depth;

  /* Return the tree */
  return degree;
}

#if 1
static int __mpcomp_get_available_vp_by_mpi_task( int* first_cpu )
{
	int num_mvps;
	const int mpi_task_rank = sctk_get_task_rank(); 
	*first_cpu = sctk_get_init_vp_and_nbvp( mpi_task_rank, &num_mvps );	
	return &num_mvps;
}
#else
static int __mpcomp_get_available_vp_by_mpi_task( int* first_cpu )
{	
	// Call hwloc and get n_cores;
}
#endif 

void __mpcomp_restrict_topology_for_mpcomp( hwloc_topology_t* restrictedTopology, const int omp_threads_expected )
{
	int i, err, num_mvps, core;	
	hwloc_topology_t topology;
	hwloc_bitmap_t final_cpuset;
	hwloc_obj_t core_obj, pu_obj;
	
	final_cpuset = hwloc_bitmap_alloc();
	topology = sctk_get_topology_object();

	const int taskRank = sctk_get_task_rank();
	const int taskVp = sctk_get_init_vp_and_nbvp( taskRank, &num_mvps );
	const int npus = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU); 
	const int ncores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE); 

	// Every core must have the same number of PU
	sctk_assert( npus % ncores  == 0 );

	if( omp_threads_expected > 0 && num_mvps > omp_threads_expected )
		num_mvps = omp_threads_expected;

	const int quot = num_mvps / ncores; 	
	const int rest = num_mvps % ncores;
	
	// Get min between ncores and num_mvps
	const int ncores_select = ( num_mvps > ncores ) ? ncores : num_mvps;

	for( core = 0; core < ncores_select; core++ )
	{
		const int num_pus_select = ( rest > core ) ? quot+1 : quot; 
		core_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, taskVp+core );	
		sctk_assert( core_obj );
	
		for( i = 0; i < num_pus_select; i ++ )
		{
			fprintf(stderr, "[%d] ::: %s ::: %d - > %d\n", taskVp, __func__, core, i );
			pu_obj = hwloc_get_obj_inside_cpuset_by_type(topology, core_obj->cpuset, HWLOC_OBJ_PU, i);
			sctk_assert( pu_obj );
			hwloc_bitmap_or( final_cpuset, final_cpuset, pu_obj->cpuset );
		}
	}	

	sctk_assert( num_mvps == hwloc_bitmap_weight( final_cpuset ) );	

	if((err = hwloc_topology_init(restrictedTopology)))
		return -1;

	if((err = hwloc_topology_dup(restrictedTopology,topology)))
		return -1;

	if((err = hwloc_topology_restrict(*restrictedTopology,final_cpuset,HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES)))
		return -1;
	
	sctk_print_specific_topology(stderr, *restrictedTopology);

	return 0;	
}

void __mpcomp_prepare_mpcomp_tree_build( const int omp_threads_expected, const int mpi_num_tasks )
{

	
	/* Compute the number of cores for this task */
//	const max_num_mvps = __mpcomp_get_available_vp_by_mpi_task();

}

/*
 * Build the default tree.
 */
int mpcomp_build_default_tree(mpcomp_instance_t *instance) {
  int depth;
  int *degree;
  int n_leaves;
  int i;

  sctk_nodebug("__mpcomp_build_auto_tree begin");

  sctk_assert(instance != NULL);
  sctk_assert(instance->topology != NULL);

  /* Get the default topology shape */
  degree = __mpcomp_compute_topo_tree_array(instance->topology, &depth);

  /* Compute the number of leaves */
  n_leaves = 1;
  for (i = 0; i < depth; i++) {
    n_leaves *= degree[i];
  }

  sctk_debug("__mpcomp_build_default_tree: Building tree depth:%d, n_leaves:%d",
             depth, n_leaves);
  for (i = 0; i < depth; i++) {
    sctk_debug("__mpcomp_build_default_tree:\tDegree[%d] = %d", i, degree[i]);
  }

  /* Check if the tree shape is correct */
  if (!__mpcomp_check_tree_parameters(n_leaves, depth, degree)) {
    /* TODO put warning or failed if needed */
    /* Fall back to a flat tree */
    sctk_debug("__mpcomp_build_default_tree: fall back to flat tree");
    depth = 1;
    n_leaves = sctk_get_cpu_number();
    degree[0] = n_leaves;
    instance->scatter_depth = 0;
  }

  TODO("Check the tree in hybrid mode (not w/ sctk_get_cpu_number)")

  /* Build the default tree */
  mpcomp_build_tree(instance, n_leaves, depth, degree);

  sctk_nodebug("mpcomp_build_auto_tree done");

  return 1;
}

#if 0
static int
__mpcomp_tree_find_depth( int* nnodes_per_depth, const int tree_depth, const int n_elts )
{
   int cur_depth;

   sctk_assert( n_elts > 0 );
   sctk_assert( tree_depth > 0 );
   sctk_assert( nnodes_per_depth );

   for (cur_depth = tree_depth; cur_depth >= 0; cur_depth--)
   {
      if( nnodes_per_depth[cur_depth] <= n_elts )
         break;
   }
   return cur_depth;
}

static void
mpcomp_build_tree_set_node_numa_depth( mpcomp_node_t* node, const int n_numa )
{
	int depth;
	int* nnodes_per_depth;

	sctk_assert( node );
	sctk_assert( n_cores > 0 );

	const int tree_depth = node->tree_depth;
	nnodes_per_depth = node->tree_nb_nodes_per_depth;

	depth = __mpcomp_tree_find_depth( nnodes_per_depth, tree_depth, n_numa );
	node->numa_depth = depth;
}

static void 
mpcomp_build_tree_set_node_core_depth( mpcomp_node_t* node, const int n_cores )
{
	int depth;
	int* nnodes_per_depth;
	
	sctk_assert( node );
	sctk_assert( n_cores > 0 );

	const int tree_depth = node->tree_depth;
	nnodes_per_depth = node->tree_nb_nodes_per_depth;
	
	depth = __mpcomp_tree_find_depth( nnodes_per_depth, tree_depth, n_cores );
	node->core_depth = depth;
}
#endif 

/*
 * Build a tree according to three parameters.
 */
int mpcomp_build_tree(mpcomp_instance_t *instance, int n_leaves, int depth, int *degree) 
{
  int i;
  int *order; /* Topological sort of VPs where order[0] is the current VP */

  /***** PRINT ADDITIONAL SUMMARY (only first MPI rank) ******/
  if (getenv("MPC_DISABLE_BANNER") == NULL && sctk_process_rank == 0) {
    fprintf(stderr, "\tOMP_TREE depth %d [", depth);
    for (i = 0; i < depth; i++) {
      fprintf(stderr, "%d", degree[i]);
      if (i != depth - 1) {
        fprintf(stderr, ", ");
      }
    }
    fprintf(stderr, "]\n");
  }

  if (__mpcomp_check_tree_parameters(n_leaves, depth, degree) == 0) {
    sctk_error("Wrong OMP_TREE!");
    sctk_abort();
  }

  if (n_leaves != instance->nb_mvps) {
    sctk_error("Wrong OMP_TREE! (test 2): nb leaves %d, nb mvps %d", n_leaves,
               instance->nb_mvps);
    sctk_abort();
  }

#if 0
 	/* Compute the depth for BALANCED and SCATTER */
  	const int n_cores = hwloc_get_nbobjs_by_type(instance->topology, HWLOC_OBJ_CORE);
  	const int n_numa = hwloc_get_nbobjs_by_type(instance->topology, HWLOC_OBJ_NUMANODE);

  	instance->core_depth = -1;
 	instance->scatter_depth = -1;
   
	/* Stop when encountering a level with the right number of elements
  	 * or a lower number (it may happen when the user specified the tree shape
  	 */
     for (i = instance->tree_depth; i >= 0; i--) {
       if (instance->core_depth == -1 &&
           instance->tree_nb_nodes_per_depth[i] <= n_cores) {
         instance->core_depth = i;
       }
       if (instance->scatter_depth == -1 &&
           instance->tree_nb_nodes_per_depth[i] <= n_numa) {
         instance->scatter_depth = i;
       }
     }
#endif 
     /* Get the number of CPUs */
     const int nb_cpus = sctk_get_cpu_number_topology(instance->topology);

     /* Grab the right order to allocate microVPs (sctk_get_neighborhood) */
     order = (int *)sctk_malloc((nb_cpus + 1) * sizeof(int));
     sctk_assert(order != NULL);

 //	sctk_get_neighborhood_topology(instance->topology, current_mpc_vp, nb_cpus, order);
 	//__mpcomp_alloc_openmp_tree_struct( degree, depth, NULL, instance ); 
 	free(order);

	return 0;
}

