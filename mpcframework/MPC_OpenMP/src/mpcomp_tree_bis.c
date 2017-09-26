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

#include "sctk.h"
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

int __mpcomp_restrict_topology_for_mpcomp( hwloc_topology_t* restrictedTopology, const int omp_threads_expected, const int* cpulist )
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

    if( cpulist )
    {
        /* If smt is enable we use cpu_id as HT_id else core_id 
         * We can iterate on OBJ_PU directly for both configuration */   
         assume( omp_threads_expected == num_mvps );

         /* Restrict core_id to core in OMP_PLACES */
         for( i = 0; i < num_mvps; i++ )
         {
             const int cur_pu_id = cpulist[i];
             pu_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cur_pu_id);
             sctk_assert( pu_obj );
             hwloc_bitmap_or( final_cpuset, final_cpuset, pu_obj->cpuset );
         }
    } 
    else  /* No places */
    {
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
		     pu_obj = hwloc_get_obj_inside_cpuset_by_type(topology, core_obj->cpuset, HWLOC_OBJ_PU, i);
			 sctk_assert( pu_obj );
			 hwloc_bitmap_or( final_cpuset, final_cpuset, pu_obj->cpuset );
		  }
	    }
    }	

	sctk_assert( num_mvps == hwloc_bitmap_weight( final_cpuset ) );	

	if((err = hwloc_topology_init(restrictedTopology)))
		return -1;

	if((err = hwloc_topology_dup(restrictedTopology,topology)))
		return -1;

	if((err = hwloc_topology_restrict(*restrictedTopology,final_cpuset,HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES)))
		return -1;

    hwloc_obj_t prev_pu, next_pu;
    prev_pu = NULL; // Init hwloc iterator
    while( next_pu = hwloc_get_next_obj_by_type( *restrictedTopology, HWLOC_OBJ_PU, prev_pu ) )
    {
       prev_pu = next_pu;
    }
    hwloc_bitmap_free( final_cpuset );
	return 0;	
}

int mpcomp_build_tree(mpcomp_instance_t *instance, int n_leaves, int depth, int *degree) 
{
  int i;
  int *order; /* Topological sort of VPs where order[0] is the current VP */

  /***** PRINT ADDITIONAL SUMMARY (only first MPI rank) ******/
  if (getenv("MPC_DISABLE_BANNER") == NULL && sctk_get_task_rank() == 0) {
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

	return 0;
}

