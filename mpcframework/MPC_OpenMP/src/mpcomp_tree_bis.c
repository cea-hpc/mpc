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

void* __mpcomp_alloc_openmp_tree_struct( const int* shape, const int max_depth, int *tree_size, mpcomp_instance_t* instance );

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

static void __mpcomp_print_tree(mpcomp_instance_t *instance) {

  mpcomp_stack_t *s;
  int i, j;

  /* Skip the debug messages if not enough verbosity */
  if (sctk_get_verbosity() < 3)
    return;

  sctk_assert(instance != NULL);

  TODO("compute the real number of max elements for this stack")
  s = __mpcomp_create_stack(2048);
  sctk_assert(s != NULL);

  __mpcomp_push(s, instance->root);

  TODO("Port this function w/ sctk_debug function by allocating strings")

  fprintf(stderr, "==== Printing the current OpenMP tree ====\n");

  /* Print general information about the instance and the tree */
  fprintf(stderr, "Depth = %d, Degree = [", instance->tree_depth);
  for (i = 0; i < instance->tree_depth; i++) {
    if (i != 0) {
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "%d", instance->tree_base[i]);
  }
  fprintf(stderr, "]");

  /* Print the cumulative array */
  fprintf(stderr, " Cumulative = [");
  for (i = 0; i < instance->tree_depth; i++) {
    if (i != 0) {
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "%d", instance->tree_cumulative[i]);
  }
  fprintf(stderr, "]");

  /* Print the number of nodes per level */
  fprintf(stderr, " #Nodes per depth = [");
  for (i = 0; i <= instance->tree_depth; i++) {
    if (i != 0) {
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "%d", instance->tree_nb_nodes_per_depth[i]);
  }
  fprintf(stderr, "]");

  /* Print the level of physical cores and scatter */
  fprintf(stderr, " physical_core level=%d, scatter level=%d",
          instance->core_depth, instance->scatter_depth);

  fprintf(stderr, "\n");

  while (!__mpcomp_is_stack_empty(s)) {
    mpcomp_node_t *n;

    n = __mpcomp_pop(s);
    sctk_assert(n != NULL);

    /* Print this node */

    /* Add tab according to depth */
    for (i = 0; i < n->depth; i++) {
      fprintf(stderr, "\t");
    }

    /* Print main information about the node */
    fprintf(stderr, "Node %ld (@ %p) -> NUMA %d, min/max ", n->rank, n,
            n->id_numa);

    /* Print all min_index for each affinity */
    for (i = 0; i < MPCOMP_AFFINITY_NB; i++) {
      fprintf(stderr, "[%d]->%.2d ", i, n->min_index[i]);
    }

    /* Print main information about the node (cont.) */
    fprintf(stderr, "(barrier_num_threads=%ld)\n", n->barrier_num_threads);

    switch (n->child_type) {
    case MPCOMP_CHILDREN_NODE:
      for (i = n->nb_children - 1; i >= 0; i--) {
        __mpcomp_push(s, n->children.node[i]);
      }
      break;
    case MPCOMP_CHILDREN_LEAF:
      for (i = 0; i < n->nb_children; i++) {
        mpcomp_mvp_t *mvp;

        mvp = n->children.leaf[i];
        sctk_assert(mvp != NULL);

        /* Add tab according to depth */
        for (j = 0; j < n->depth + 1; j++) {
          fprintf(stderr, "\t");
        }

        fprintf(stderr, "Instance @ %p Leaf %d rank %d @ %p vp %d "
                        "spinning on %p",
                instance, i, mvp->rank, mvp, mvp->vp, mvp->to_run);

        /* Print min_index for each affinity */
        for (j = 0; j < MPCOMP_AFFINITY_NB; j++) {
          fprintf(stderr, " [%d]->%.2d", j, mvp->min_index[j]);
        }

        fprintf(stderr, " tree_rank @ %p", mvp->tree_rank);
        for (j = 0; j < n->depth + 1; j++) {
          fprintf(stderr, " j=%d, %d", j, mvp->tree_rank[j]);
        }

        fprintf(stderr, "\n");
      }
      break;
    default:
      not_reachable();
    }
  }

  __mpcomp_free_stack(s);
  free(s);

  return;
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

static int __mpcomp_compute_scatter_min_index(mpcomp_node_t *father,
                                              mpcomp_instance_t *instance,
                                              int rank_in_children) {
  sctk_assert(father != NULL);
  sctk_assert(instance != NULL);
  sctk_assert(rank_in_children >= 0);
  sctk_assert(rank_in_children < father->nb_children);

  return father->min_index[MPCOMP_AFFINITY_SCATTER] +
         rank_in_children * instance->tree_nb_nodes_per_depth[father->depth];
}

/*
 * Build a tree according to three parameters.
 */
int mpcomp_build_tree(mpcomp_instance_t *instance, int n_leaves, int depth,
                      int *degree) {
  mpcomp_node_t *root; /* Root of the tree */
  int current_mpc_vp;  /* Current VP where the master is */
  int current_mvp;     /* Current microVP we are dealing with */
  int *order; /* Topological sort of VPs where order[0] is the current VP */
  mpcomp_stack_t *s;      /* Stack used to create the tree with a DFS */
  int max_stack_elements; /* Max elements of the stack computed thanks to depth
                             and degrees */
  int previous_depth;     /* What was the previously stacked depth (to check if
                             depth is increasing or decreasing on the stack) */
  mpcomp_node_t
      *target_node; /* Target node to spin when creating the next mVP */
  int i, j;
  int nb_cpus;

  /* Check input parameters */
  sctk_assert(instance != NULL);
  sctk_assert(instance->mvps != NULL);

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

  /* Fill instance paratemers regarding tree shape */
  instance->tree_depth = depth;
  instance->tree_base = degree;
  /* tree_cumulative */
  TODO("Perform a deep copy of degree for tree_base")
  instance->tree_cumulative = mpcomp_malloc(0, sizeof(int) * depth, 0);
  for (i = 0; i < depth - 1; i++) {
    instance->tree_cumulative[i] = 1;
    for (j = i + 1; j < depth; j++) {
      instance->tree_cumulative[i] *= instance->tree_base[j];
    }
  }
  instance->tree_cumulative[depth - 1] = 1;
  /* tree_nb_nodes_per_depth */
  instance->tree_nb_nodes_per_depth =
      mpcomp_malloc(0, sizeof(int) * (depth + 1), 0);
  instance->tree_nb_nodes_per_depth[0] = 1;
  for (i = 1; i <= depth; i++) {
    instance->tree_nb_nodes_per_depth[i] =
        instance->tree_nb_nodes_per_depth[i - 1] * instance->tree_base[i - 1];
  }

#if MPCOMP_TASK
  mpcomp_task_instance_task_infos_init(instance);
#endif /* MPCOMP_TASK */

     /* Compute the depth for BALANCED and SCATTER */
     int n_cores, n_numa;

     n_cores = hwloc_get_nbobjs_by_type(instance->topology, HWLOC_OBJ_CORE);
     n_numa = hwloc_get_nbobjs_by_type(instance->topology, HWLOC_OBJ_NUMANODE);

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

     sctk_nodebug("__mpcomp_build_tree: n_cores=%d, n_numa=%d => core_depth=%d, "
                "scatter_depth=%d",
                n_cores, n_numa, instance->core_depth, instance->scatter_depth);

     /* Compute the max number of elements on the stack */
     max_stack_elements = 1;
     for (i = 0; i < depth; i++)
       max_stack_elements += degree[i];

     sctk_nodebug("__mpcomp_build_tree: max_stack_elements = %d",
                  max_stack_elements);

     /* Allocate memory for the node stack */
     s = __mpcomp_create_stack(max_stack_elements);
     sctk_assert(s != NULL);

     /* Get the current VP number */
     /* current_mpc_vp = sctk_thread_get_vp (); */
     current_mpc_vp = 0;

     /* Get the number of CPUs */
     nb_cpus = sctk_get_cpu_number_topology(instance->topology);

     sctk_nodebug("__mpcomp_build_tree: number of cpus: %d", nb_cpus);

     /* Grab the right order to allocate microVPs (sctk_get_neighborhood) */
     order = (int *)sctk_malloc((nb_cpus + 1) * sizeof(int));
     sctk_assert(order != NULL);

     sctk_get_neighborhood_topology(instance->topology, current_mpc_vp, nb_cpus,
                                    order);

#if 0
     /* Build the tree of this OpenMP instance */
     root = (mpcomp_node_t *)sctk_malloc(sizeof(mpcomp_node_t));
     sctk_assert(root != NULL);

     instance->root = root;

     current_mvp = 0;
     previous_depth = -1;
     target_node = NULL;

     /* Fill root information */
     root->father = NULL;
     root->rank = -1;
     root->depth = 0;
     root->nb_children = degree[0];
     for (i = 0; i < MPCOMP_AFFINITY_NB; i++) {
       root->min_index[i] = 0;
     }
     root->instance = instance;
     root->slave_running = 0;
     sctk_atomics_store_int(&(root->barrier), 0);

     root->barrier_done = 0;
     root->barrier_num_threads = 0;

     root->id_numa =
         sctk_get_node_from_cpu_topology(instance->topology, current_mpc_vp);

     __mpcomp_push(s, root);
#endif
//    struct timeval tv_start, tv_end;
//    (void) gettimeofday( &tv_start, NULL ); 
    instance->root = __mpcomp_alloc_openmp_tree_struct( degree, depth, NULL, instance ); 
#if 0
    (void) gettimeofday( &tv_end, NULL );
   
    long secs_used = tv_end.tv_sec - tv_start.tv_sec; //avoid overflow by subtracting first
    long micros_used = tv_end.tv_usec - tv_start.tv_usec;
    if( micros_used < 0 )
    {
        secs_used++;    //retenue
        micros_used += 1000000; 
    }
    fprintf(stderr, "Building tree in %d sec %d msec\n", secs_used, micros_used ); 
#endif 
#if 0
     while (!__mpcomp_is_stack_empty(s)) {
       mpcomp_node_t *n;
       int target_vp;
       int target_numa;

       n = __mpcomp_pop(s);
       sctk_assert(n != NULL);

       if (previous_depth == -1) {
         target_node = root;
       } else {
         if (n->depth < previous_depth) {
           target_node = n;
         }
       }
       previous_depth = n->depth;

       target_vp = order[current_mvp];
       sctk_assert(target_vp < nb_cpus && target_vp >= 0);

       target_numa =
           sctk_get_node_from_cpu_topology(instance->topology, target_vp);

#if MPCOMP_TASK
       mpcomp_task_node_task_infos_reset(n);
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
			 instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_COMPACT] = n->min_index[MPCOMP_AFFINITY_COMPACT]+i;
             instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_SCATTER] = 
                 __mpcomp_compute_scatter_min_index( n, instance, i ) ;
             // instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_BALANCED]
             // = n->min_index[MPCOMP_AFFINITY_COMPACT]+i; /* TODO: compute the
             // real value */
             if (n->depth < instance->core_depth) {
               instance->mvps[current_mvp]
                   ->min_index[MPCOMP_AFFINITY_BALANCED] =
                   n->min_index[MPCOMP_AFFINITY_BALANCED] +
                   // i * instance->tree_cumulative[n->depth] /
                   // instance->tree_nb_nodes_per_depth[instance->core_depth] ;
                   i * instance->tree_cumulative[n->depth] /
                       instance->tree_cumulative[instance->core_depth - 1];
             } else {
               instance->mvps[current_mvp]
                   ->min_index[MPCOMP_AFFINITY_BALANCED] =
                   n->min_index[MPCOMP_AFFINITY_BALANCED] +
                   i * instance->tree_nb_nodes_per_depth[n->depth];
             }
                fprintf(stderr, "rank %d -- %d %d %d\n", target_vp, instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_COMPACT], instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_SCATTER], instance->mvps[current_mvp]->min_index[MPCOMP_AFFINITY_BALANCED] ); 
             instance->mvps[current_mvp]->enable = 1;
             instance->mvps[current_mvp]->tree_rank =
                 (int *)mpcomp_malloc(1, depth * sizeof(int), target_numa);
             sctk_assert(instance->mvps[current_mvp]->tree_rank != NULL);

             instance->mvps[current_mvp]->tree_rank[depth - 1] = i;

             instance->mvps[current_mvp]->threads = (mpcomp_thread_t*) 
                    malloc( MPCOMP_MAX_THREADS_PER_MICROVP * sizeof( mpcomp_thread_t ) );
             assert( instance->mvps[current_mvp]->threads );
             

             for (i_thread = 0; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP;
                  i_thread++) {
               mpcomp_local_icv_t icvs;
               __mpcomp_thread_infos_init(
                   &(instance->mvps[current_mvp]->threads[i_thread]), icvs,
                   instance, sctk_openmp_thread_tls);
               instance->mvps[current_mvp]->threads[i_thread].mvp =
                   instance->mvps[current_mvp];
             }
             instance->mvps[current_mvp]->threads[0].rank = current_mvp;

#if MPCOMP_TASK
             mpcomp_task_mvp_task_infos_reset(instance->mvps[current_mvp]);
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
                         sctk_thread_attr_setbinding(
                             &__attr, (sctk_get_cpu() + target_vp) %
                                          sctk_get_cpu_number());
                         res = sctk_thread_attr_setstacksize(
                             &__attr, mpcomp_global_icvs.stacksize_var);
                         //   mpcomp_global_icvs.stacksize_var);

                         /* User thread create... */
                         instance->mvps[current_mvp]->to_run = target_node;
                         instance->mvps[current_mvp]->slave_running = 0;

                         if (target_node != root) {
                           sctk_nodebug("__mpcomp_build_tree: Creating mVPs "
                                        "#%d on VP #%d (index core %d)",
                                        current_mvp, target_vp,
                                        sctk_get_global_index_from_cpu(
                                            instance->topology, target_vp));
                           if (i == 0) {
                             sctk_assert(target_node != NULL);
                             /* The first child is spinning on a node */
                             instance->mvps[current_mvp]->to_run = target_node;
                             res = sctk_user_thread_create(
                                 &(instance->mvps[current_mvp]->pid), &__attr,
                                 mpcomp_slave_mvp_node,
                                 instance->mvps[current_mvp]);
                             sctk_assert(res == 0);
                             // sctk_thread_yield();
                           } else {
                             /* The other children are regular leaves */
                             instance->mvps[current_mvp]->to_run = NULL;
                             res = sctk_user_thread_create(
                                 &(instance->mvps[current_mvp]->pid), &__attr,
                                 mpcomp_slave_mvp_leaf,
                                 instance->mvps[current_mvp]);
                             sctk_assert(res == 0);
                             // sctk_thread_yield();
                           }
                         } else {
                           /* Special case when depth==1, the current node is
                            * root */
                           if (n == root) {
                             target_node = NULL;
                             sctk_assert(i == 0);
                           } else {
                             target_node = n;
                           }
                         }

                         sctk_thread_attr_destroy(&__attr);

                         current_mvp++;

                         /* Recompute the target vp */
                         target_vp = order[current_mvp];

                         /* We reached the leaves */
                         previous_depth++;
                    }
               } else {


                 /* Children are nodes */
                 n->child_type = MPCOMP_CHILDREN_NODE;
                 n->children.node = (mpcomp_node_t **)mpcomp_malloc(
                     1, n->nb_children * sizeof(mpcomp_node_t *), target_numa);

                 /* Traverse children in reverse order for correct ordering
                  * during the DFS */
                 for (i = n->nb_children - 1; i >= 0; i--) {
                   mpcomp_node_t *n2;
                   int min_index[MPCOMP_AFFINITY_NB];
                   int child_target_numa, nb_cores, nb_threads_per_core;

                   /* Compute the min rank of openmp thread in the
                    * corresponding subtree */
                   min_index[MPCOMP_AFFINITY_COMPACT] =
                       n->min_index[MPCOMP_AFFINITY_COMPACT] +
                       i * instance->tree_cumulative[n->depth];
                   min_index[MPCOMP_AFFINITY_SCATTER] =
                       __mpcomp_compute_scatter_min_index(n, instance, i);
                   // min_index[MPCOMP_AFFINITY_BALANCED] =
                   // min_index[MPCOMP_AFFINITY_COMPACT] ;
                   if (n->depth < instance->core_depth) {
                     min_index[MPCOMP_AFFINITY_BALANCED] =
                         n->min_index[MPCOMP_AFFINITY_BALANCED] +
                         // i * instance->tree_cumulative[n->depth] /
                         // instance->tree_nb_nodes_per_depth[instance->core_depth]
                         // ;
                         i * instance->tree_cumulative[n->depth] /
                             instance
                                 ->tree_cumulative[instance->core_depth - 1];
                   } else {
                     min_index[MPCOMP_AFFINITY_BALANCED] =
                         n->min_index[MPCOMP_AFFINITY_BALANCED] +
                         i * instance->tree_nb_nodes_per_depth[n->depth];
                   }
                   sctk_debug("__mpcomp_build_tree: i=%d, n->depth=%d, "
                              "instance->core_depth=%d, "
                              "min_index[MPCOMP_AFFINITY_BALANCED]=%d "
                              "n->min_index[MPCOMP_AFFINITY_BALANCED]=%d",
                              i, n->depth, instance->core_depth,
                              min_index[MPCOMP_AFFINITY_BALANCED],
                              n->min_index[MPCOMP_AFFINITY_BALANCED]);

                   child_target_numa = sctk_get_node_from_cpu_topology(
                       instance->topology,
                       order[min_index[MPCOMP_AFFINITY_COMPACT]]);

                   n2 = (mpcomp_node_t *)mpcomp_malloc(1, sizeof(mpcomp_node_t),
                                                       child_target_numa);

                   n->children.node[i] = n2;

                   n2->father = n;
                   n2->rank = i;
                   n2->depth = n->depth + 1;
                   n2->nb_children = degree[n2->depth];
                   for (j = 0; j < MPCOMP_AFFINITY_NB; j++)
                     n2->min_index[j] = min_index[j];

                   n2->instance = instance;
                   n2->slave_running = 0;

                   sctk_atomics_store_int(&(n2->barrier), 0);

                   n2->barrier_done = 0;
                   n2->barrier_num_threads = 0;

                   n2->id_numa = child_target_numa;

                   __mpcomp_push(s, n2);
                 }
               }
     }

     /* Free memory */
     __mpcomp_free_stack(s);
     free(s);
     free(order);
#endif
     /* Print the final tree */
     __mpcomp_print_tree(instance);

     return 0;
}

