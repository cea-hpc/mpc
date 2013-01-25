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
     int i ;
     int computed_n_leaves ;

     /* Check if the number of leaves is strictly positive 
	and is lower than the total number of CPUs */
     if ((n_leaves <= 0) || (n_leaves > sctk_get_cpu_number()))
	  return 0 ;

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

/* recursive function counting the number of core below (so on his sub-tree) an hwloc object */
unsigned int __mpcomp_get_cpu_number_below(hwloc_obj_t obj)
{
     unsigned int i, count = 0;

     if (hwloc_compare_types(obj->type, HWLOC_OBJ_PU) == 0)
	  return 1;
     
     for (i=0; i<obj->arity; i++)
	  count += __mpcomp_get_cpu_number_below(obj->children[i]);

     return count;
}

//#if 0
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
		
     if (father == NULL) {   
          /* case root */
	  node = (mpcomp_node_t *) sctk_malloc(sizeof(mpcomp_node_t));
	  sctk_assert(node != NULL);
     
	  instance->root = node;
		
	  node->father = NULL;
	  node->rank = -1;
	  node->depth = 0;
	  node->nb_children = obj->arity;
	  node->min_index = 0;
	  node->max_index = __mpcomp_get_cpu_number_below(obj) - 1;
	  /* node->child_type is modified by children */
	  node->lock = SCTK_SPINLOCK_INITIALIZER;
	  node->slave_running = 0;
	  
#if MPCOMP_USE_ATOMICS
	  sctk_atomics_store_int(&(node->barrier), 0);
	  /* Chunks infos */
	  sctk_atomics_store_int(&(node->chunks_avail), MPCOMP_CHUNKS_AVAIL);
	  sctk_atomics_store_int(&(node->nb_chunks_empty_children), 0);
#else
	  node->barrier = 0;
	  /* Chunks infos */
	  node->chunks_avail = MPCOMP_CHUNKS_AVAIL;
	  node->nb_chunks_empty_children = 0;
#endif	
     
	  node->barrier_done = 0;
		
	  /* will be allocated by the first child */
	  node->children.node = NULL;
	  node->children.leaf = NULL;		
	  
	  node->type = MPCOMP_MYSELF_ROOT;
	  node->current_mvp = 0;
	  node->id_numa = 0;
	  
	  /* tree_rank avec tableaux intermediaires */
          /* node->temporary_tree_rank = sctk_malloc_on_node(sizeof(int)*(1), node->id_numa);*/
          /* sctk_assert( node->temporary_tree_rank != NULL );*/
          /* node->temporary_tree_rank[0] = 0;*/
	
#ifdef MPCOMP_TASK
	  node->untied_tasks = NULL;
	  node->new_tasks = NULL;
#endif //MPCOMP_TASK

     } else {   /* case leaf or node */		
   	  if (__mpcomp_get_cpu_number_below(obj) == 1) {   /* case leaf (mvp) */		
	       int target_vp = order[current_mvp];
	       int i_thread;
	       int depth;
	       mpcomp_node_t *node_tmp;
	       sctk_thread_attr_t __attr ;
	       int res ;
		    
	       father->child_type = MPCOMP_CHILDREN_LEAF;
	       if ((father->children.node == NULL) && (father->children.leaf == NULL)) {
		    father->children.leaf = (mpcomp_mvp_t **) sctk_malloc_on_node(sizeof(mpcomp_mvp_t*), father->id_numa);
		    sctk_assert(father->children.leaf != NULL);
	       }			
	       
	       /* Allocate memory (on the right NUMA Node) */
#if MPCOMP_MALLOC_ON_NODE
	       instance->mvps[current_mvp] = (mpcomp_mvp_t *) sctk_malloc_on_node(sizeof(mpcomp_mvp_t), father->id_numa);
#else
	       instance->mvps[current_mvp] = (mpcomp_mvp_t *) sctk_malloc_on_node(sizeof(mpcomp_mvp_t), 0);
#endif
	       sctk_assert(instance->mvps[current_mvp] != NULL);
	       
	       /* Get the set of registers */
	       sctk_getcontext (&(instance->mvps[current_mvp]->vp_context));
	       leaf = instance->mvps[current_mvp];
	       
	       /* Initialize the corresponding microVP (all but tree-related variables) */
	       /* TODO put the microVP initialization in a function somewhere. 
		* If possible where ze could call a function to initialize each thread 
		*/
	       leaf->nb_threads = 0;
	       leaf->next_nb_threads = 0;
	       leaf->children_instance = instance;
		  
	       
	       sctk_nodebug( "////////// MVP INITIALIZATION //////////" );
	       for (i_thread = 0; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP; i_thread++) {
		    int i_fordyn;
		    for (i_fordyn = 0; i_fordyn < MPCOMP_MAX_ALIVE_FOR_DYN+1; i_fordyn++) {
			 sctk_nodebug("[__mpcomp_build_auto_tree] set remain to -1");
			 sctk_atomics_store_int(&(instance->mvps[current_mvp]->threads[i_thread].for_dyn_chunk_info[i_fordyn].remain), -1);
		    }
	       }
	       
	       leaf->rank = current_mvp;
               //leaf->vp = 0;
               leaf->vp = sctk_thread_get_vp(); //AMAHEO
              
	       leaf->enable = 1;
	       depth = father->depth + 1;
			
#if MPCOMP_MALLOC_ON_NODE
	       leaf->tree_rank = (int *) sctk_malloc_on_node(depth * sizeof(int), father->id_numa);
#else
	       leaf->tree_rank = (int *) sctk_malloc_on_node(depth * sizeof(int), 0);
#endif
	       
	       sctk_assert(leaf->tree_rank != NULL);
			
	       /* tree_rank */
			
	       node_tmp = father;

               //AMAHEO
               /* 
               for(i=0;i<depth;i++) {
                 leaf->tree_rank[i] = 0;
               }
               */

               sctk_nodebug("__mpcomp_build_auto_tree_recursive: id_loc=%d", id_loc); //AMAHEO 
               sctk_assert(id_loc < 40); //AMAHEO
//#if MPCOMP_USE_ATOMICS
               //sctk_atomics_int atomics_id_loc;
               //atomics_id_loc = id_loc;
 	       //sctk_atomics_store_int(&(leaf->tree_rank[depth-1]), &atomics_id_loc);
  //sctk_spinlock_lock( &(node->lock) ) ;
//#else
	       leaf->tree_rank[depth-1] = id_loc;
  //sctk_spinlock_unlock( &(node->lock) ) ;

//#endif
	       for(i=depth-2; i>=0; i--) {
//#if MPCOMP_USE_ATOMICS
 //   	            sctk_atomics_store_int(&(leaf->tree_rank[i]), node_tmp->rank);		
//#else
  //sctk_spinlock_lock( &(node->lock) ) ;
                    sctk_nodebug("__mpcomp_build_auto_tree_recursive: node_tmp rank=%d", node_tmp->rank); //AMAHEO
                    sctk_assert(node_tmp->rank < 40); //AMAHEO

		    leaf->tree_rank[i] = node_tmp->rank;
  //sctk_spinlock_unlock( &(node->lock) ) ;
//#endif
		    node_tmp = node_tmp->father;
	       }

               /* Check tree_rank */
               for(i=0;i<depth;i++)
		sctk_debug("__mpcomp_build_auto_tree_recursive: current_mvp=%d, tree_rank[%d]: %d", current_mvp, i, leaf->tree_rank[i]); //AMAHEO
			
	       /* tree_rank avec tableaux intermediaires */
               /* for(i=1; i<depth; i++){*/
               /*      leaf->tree_rank[i-1] = father->temporary_tree_rank[i];*/
	       /* } */
               /* leaf->tree_rank[depth-1] = id_loc;*/
			
	       leaf->root = instance->root ;
	       leaf->father = father;
	       
	       father->children.leaf[id_loc] = leaf;
	       
	       sctk_thread_attr_init(&__attr);
	       
	       sctk_thread_attr_setbinding(& __attr, target_vp);
	       
	       sctk_thread_attr_setstacksize(&__attr, mpcomp_global_icvs.stacksize_var);
									
	       if (current_mvp == 0) {   /*  case mvp : root */
		    leaf->type = MPCOMP_MYSELF_ROOT;
		    leaf->to_run = instance->root;
	       } else if (id_loc == 0) {   /*  case mvp : node */		    
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

#ifdef MPCOMP_TASK
	       leaf->untied_tasks = NULL;
	       leaf->new_tasks = NULL;
#endif //MPCOMP_TASK
	       
	  } else {   /* case node */
	       int id_numa;

#if MPCOMP_MALLOC_ON_NODE
	       id_numa = sctk_get_node_from_cpu(order[current_mvp]);
#else
	       id_numa = 0;
#endif
	       
	       node = (mpcomp_node_t *) sctk_malloc_on_node(sizeof(mpcomp_node_t), id_numa);
	       sctk_assert( node != NULL );
			
	       node->id_numa = id_numa;
	       
	       if ((father->children.node == NULL) && (father->children.leaf == NULL)) {
		    /* only the first child do this */
		    father->child_type = MPCOMP_CHILDREN_NODE;
		    /* alloc for the father */
		    father->children.node = (mpcomp_node_t **) sctk_malloc_on_node(sizeof(mpcomp_node_t *) * father->nb_children, father->id_numa);
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
	       node->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	       sctk_atomics_store_int(&(node->barrier), 0);
	       /* Chunks infos*/
	       sctk_atomics_store_int(&(node->chunks_avail), MPCOMP_CHUNKS_AVAIL);
	       sctk_atomics_store_int(&(node->nb_chunks_empty_children), 0); 
#else
	       node->barrier = 0 ;
	       /* Chunks infos */
	       node->chunks_avail = MPCOMP_CHUNKS_AVAIL;
	       node->nb_chunks_empty_children = 0;
#endif
	       node->barrier_done = 0;
	       
	       node->children.node = NULL;
	       node->children.leaf = NULL;
			
#ifdef MPCOMP_TASK
	       node->untied_tasks = NULL;
	       node->new_tasks = NULL;
#endif //MPCOMP_TASK
			
	       /* tree_rank avec tableaux intermediaires */
               /* #if MPCOMP_MALLOC_ON_NODE */
	       /*     node->temporary_tree_rank = (int *)sctk_malloc_on_node((node->depth+1)*sizeof(int), node->id_numa ) ; */
	       /* #else */
	       /*     node->temporary_tree_rank = (int *)sctk_malloc_on_node((node->depth+1)*sizeof(int), 0 ) ; */
	       /* #endif */
	       /* */
	       /*    sctk_assert( node->temporary_tree_rank != NULL ); */
	       /* */
	       /*    for(i=0; i<node->depth; i++){ */
	       /* 	 node->temporary_tree_rank[i] = father->temporary_tree_rank[i]; */
	       /*    } */
	       /*    node->temporary_tree_rank[node->depth] = node->rank; */
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
	  /* temporary_tree_rank can be safely realeased here -because it's after the recursive call- */
          /* sctk_free(node->temporary_tree_rank); */
          /* node->temporary_tree_rank = NULL; */
     }
}


//#if 0
/*
 * Build the default tree.
 * TODO this function should create a tree according to architecture tolopogy
 */
int __mpcomp_build_default_tree(mpcomp_instance_t *instance)
{
     /* en supposant que hwloc mette les noeuds NUMA à plat sur une imbrication de plusieurs numa (sans hierarchie) comme dans l'exemple trouvé ici :
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

     sctk_debug("__mpcomp_build_default_tree: current_mpc_vp=%d", current_mpc_vp);
     
     /* TODO So far, we do not fully support when the OpenMP instance is created from any VP */
     //sctk_assert(current_mpc_vp == 0);
     sctk_get_neighborhood(current_mpc_vp, nb_cpus, order);
     
     __mpcomp_build_auto_tree_recursive_bloc(instance, order, 
					     hwloc_get_root_obj(sctk_get_topology_object()),
					     NULL, 0, 0);
     
     sctk_free(order);
     
     sctk_nodebug("__mpcomp_build_auto_tree done"); 
	
     //__mpcomp_print_tree(instance);
     
     return 1;
}
//#endif

#if 0
/*
   Build the default tree.
   TODO this function should create a tree according to architecture tolopogy
 */
int __mpcomp_build_default_tree( mpcomp_instance_t * instance ) {
  int * order ;
  int i ;
  int current_mvp ;
  int flag_level ;
  int current_mpc_vp;
  mpcomp_node_t * root ;

  sctk_nodebug("__mpcomp_build_default_tree begin"); 

  /* Get the current VP number */
  current_mpc_vp = sctk_thread_get_vp ();

  /* TODO So far, we do not fully support when the OpenMP instance is created from any VP */
  //sctk_assert( current_mpc_vp == 0 ) ;

  /* Grab the right order to allocate microVPs (sctk_get_neighborhood) */
  order = sctk_malloc( sctk_get_cpu_number () * sizeof( int ) ) ;
  sctk_assert( order != NULL ) ;

  sctk_get_neighborhood( current_mpc_vp, sctk_get_cpu_number (), order ) ;

  /* Build the tree of this OpenMP instance */

  root = (mpcomp_node_t *)sctk_malloc( sizeof( mpcomp_node_t ) ) ;
  sctk_assert( root != NULL ) ;

  instance->root = root ;

  current_mvp = 0 ;
  flag_level = 0 ;

  printf("__mpcomp_build_default_tree: nb cores=%d\n", sctk_get_cpu_number());

  switch( sctk_get_cpu_number() ) {
    case 8:
#if 1 /* NUMA Tree */
//#warning "OpenMP Compiling w/ NUMA tree (8 cores)"
      sctk_nodebug("__mpcomp_build_default_tree: case 8"); 

      root->father = NULL ;
      root->rank = -1 ;
      root->depth = 0 ;
      root->nb_children = 2 ;
      root->child_type = MPCOMP_CHILDREN_NODE ;
      root->lock = SCTK_SPINLOCK_INITIALIZER ;
      root->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
      sctk_atomics_store_int( &(root->barrier), 0 ) ;
      /* Chunks infos */
      sctk_atomics_store_int( &(root->chunks_avail), MPCOMP_CHUNKS_AVAIL);
      sctk_atomics_store_int( &(root->nb_chunks_empty_children), 0);
#else
      root->barrier = 0 ;
      /* Chunks infos */
      root->chunks_avail = MPCOMP_CHUNKS_AVAIL;
      root->nb_chunks_empty_children = 0;
#endif
      root->barrier_done = 0 ;
      root->children.node = (mpcomp_node_t **)sctk_malloc( root->nb_children * sizeof( mpcomp_node_t *) ) ;
      sctk_assert( root->children.node != NULL ) ;


      for ( i = 0 ; i < root->nb_children ; i++ ) {
	mpcomp_node_t * n ;
	int j ;

	if ( flag_level == -1 ) { flag_level = 1 ; }

	n = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), i  ) ;
	sctk_assert( n != NULL ) ;

	root->children.node[i] = n ;

	n->father = root ;
	n->rank = i ;
	n->depth = 1 ;
	n->nb_children = 4 ;
	n->child_type = MPCOMP_CHILDREN_LEAF ;
	n->lock = SCTK_SPINLOCK_INITIALIZER ;
	n->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	sctk_atomics_store_int( &(n->barrier), 0 ) ;
        /* Chunks infos */
        sctk_atomics_store_int( &(n->chunks_avail), MPCOMP_CHUNKS_AVAIL);
        sctk_atomics_store_int( &(n->nb_chunks_empty_children), 0);
#else
	n->barrier = 0 ;
        /* Chunks infos */
        n->chunks_avail = MPCOMP_CHUNKS_AVAIL;
        n->nb_chunks_empty_children = 0;
#endif
	n->barrier_done = 0 ;
	n->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node( n->nb_children* sizeof(mpcomp_mvp_t *), i ) ;


	for ( j = 0 ; j < n->nb_children ; j++ ) {
	  /* This are leaves -> create the microVP */
	  int target_vp ;

	  target_vp = order[4*i+j] ;

	  if ( flag_level == -1 ) { flag_level = 2 ; }

	  /* Allocate memory (on the right NUMA Node) */
	  instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(
	      sizeof(mpcomp_mvp_t), i ) ;

	  /* Get the set of registers */
	  sctk_getcontext (&(instance->mvps[current_mvp]->vp_context));

	  /* Initialize the corresponding microVP (all but tree-related variables) */
	  instance->mvps[current_mvp]->nb_threads = 0 ;
	  instance->mvps[current_mvp]->next_nb_threads = 0 ;

           {
	      sctk_nodebug( "////////// MVP INITIALIZATION //////////" ) ;
	      int i_thread ;
	      for ( i_thread = 0 ; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP ; i_thread++ ) {
		int i_fordyn ;
		for ( i_fordyn = 0 ; i_fordyn < MPCOMP_MAX_ALIVE_FOR_DYN + 1 ; i_fordyn++ ) {
                  //sctk_nodebug("Initializing chunks info (remain=-1)");
		  sctk_atomics_store_int( 
		      &(instance->mvps[current_mvp]->threads[i_thread].for_dyn_chunk_info[i_fordyn].remain), 
		      -1 ) ;
		}
	      }
           }   

	  instance->mvps[current_mvp]->rank = current_mvp ;
	  instance->mvps[current_mvp]->enable = 1 ;
	  instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node( 2*sizeof(int), i ) ;
	  sctk_assert( instance->mvps[current_mvp]->tree_rank != NULL ) ;
	  instance->mvps[current_mvp]->tree_rank[0] = n->rank ;
	  instance->mvps[current_mvp]->tree_rank[1] = j ;
	  instance->mvps[current_mvp]->root = root ;
	  instance->mvps[current_mvp]->father = n ;

	  n->children.leaf[j] = instance->mvps[current_mvp] ;

	  sctk_thread_attr_t __attr ;
	  size_t stack_size;
	  int res ;

	  sctk_thread_attr_init(&__attr);

	  sctk_thread_attr_setbinding (& __attr, target_vp ) ;

	  if (sctk_is_in_fortran == 1)
	    stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	  else
	    stack_size = SCTK_ETHREAD_STACK_SIZE;

	  sctk_thread_attr_setstacksize (&__attr, stack_size);

	  switch ( flag_level ) {
	    case 0: /* Root */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on root", current_mvp, target_vp, i ) ;
	      instance->mvps[current_mvp]->to_run = root ;
	      break ;
	    case 1: /* Numa */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on NUMA node %d", current_mvp, target_vp, i, i ) ;
	      instance->mvps[current_mvp]->to_run = root->children.node[i] ;
	      res =
		sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		    mpcomp_slave_mvp_node,
		    instance->mvps[current_mvp]);
	      sctk_assert (res == 0);
	      break ;
	    case 2: /* MicroVP */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) w/ internal spinning", current_mvp, target_vp, i ) ;
	      res =
		sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		    mpcomp_slave_mvp_leaf,
		    instance->mvps[current_mvp]);
	      sctk_assert (res == 0);
	      break ;
	    default:
	      not_reachable() ;
	      break ;
	  }


	  sctk_thread_attr_destroy(&__attr);

	  current_mvp++ ;
	  flag_level = -1 ;
	}
      }
#endif

    case 12:
#if 1 /* NUMA Tree */
//#warning "OpenMP Compiling w/ NUMA tree (8 cores)"
      sctk_nodebug("__mpcomp_build_default_tree: case 12"); 
      printf("__mpcomp_build_default_tree: case 12\n");

      root->father = NULL ;
      root->rank = -1 ;
      root->depth = 0 ;
      root->nb_children = 2 ;
      root->child_type = MPCOMP_CHILDREN_NODE ;
      root->lock = SCTK_SPINLOCK_INITIALIZER ;
      root->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
      sctk_atomics_store_int( &(root->barrier), 0 ) ;
      /* Chunks infos */
      sctk_atomics_store_int( &(root->chunks_avail), MPCOMP_CHUNKS_AVAIL);
      sctk_atomics_store_int( &(root->nb_chunks_empty_children), 0);
#else
      root->barrier = 0 ;
      /* Chunks infos */
      root->chunks_avail = MPCOMP_CHUNKS_AVAIL;
      root->nb_chunks_empty_children = 0;
#endif
      root->barrier_done = 0 ;
      root->children.node = (mpcomp_node_t **)sctk_malloc( root->nb_children * sizeof( mpcomp_node_t *) ) ;
      sctk_assert( root->children.node != NULL ) ;


      for ( i = 0 ; i < root->nb_children ; i++ ) {
	mpcomp_node_t * n ;
	int j ;

	if ( flag_level == -1 ) { flag_level = 1 ; }

	n = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), i  ) ;
	sctk_assert( n != NULL ) ;

	root->children.node[i] = n ;

	n->father = root ;
	n->rank = i ;
	n->depth = 1 ;
	n->nb_children = 6 ;
	n->child_type = MPCOMP_CHILDREN_LEAF ;
	n->lock = SCTK_SPINLOCK_INITIALIZER ;
	n->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	sctk_atomics_store_int( &(n->barrier), 0 ) ;
        /* Chunks infos */
        sctk_atomics_store_int( &(n->chunks_avail), MPCOMP_CHUNKS_AVAIL);
        sctk_atomics_store_int( &(n->nb_chunks_empty_children), 0);
#else
	n->barrier = 0 ;
        /* Chunks infos */
        n->chunks_avail = MPCOMP_CHUNKS_AVAIL;
        n->nb_chunks_empty_children = 0;
#endif
	n->barrier_done = 0 ;
	n->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node( n->nb_children* sizeof(mpcomp_mvp_t *), i ) ;


	for ( j = 0 ; j < n->nb_children ; j++ ) {
	  /* This are leaves -> create the microVP */
	  int target_vp ;

	  target_vp = order[6*i+j] ;

	  if ( flag_level == -1 ) { flag_level = 2 ; }

	  /* Allocate memory (on the right NUMA Node) */
	  instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(
	      sizeof(mpcomp_mvp_t), i ) ;

	  /* Get the set of registers */
	  sctk_getcontext (&(instance->mvps[current_mvp]->vp_context));

	  /* Initialize the corresponding microVP (all but tree-related variables) */
	  instance->mvps[current_mvp]->nb_threads = 0 ;
	  instance->mvps[current_mvp]->next_nb_threads = 0 ;

           {
	      sctk_nodebug( "////////// MVP INITIALIZATION //////////" ) ;
	      int i_thread ;
	      for ( i_thread = 0 ; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP ; i_thread++ ) {
		int i_fordyn ;
		for ( i_fordyn = 0 ; i_fordyn < MPCOMP_MAX_ALIVE_FOR_DYN + 1 ; i_fordyn++ ) {
                  //sctk_nodebug("Initializing chunks info (remain=-1)");
		  sctk_atomics_store_int( 
		      &(instance->mvps[current_mvp]->threads[i_thread].for_dyn_chunk_info[i_fordyn].remain), 
		      -1 ) ;
		}
	      }
           }   

	  instance->mvps[current_mvp]->rank = current_mvp ;
	  instance->mvps[current_mvp]->enable = 1 ;
	  instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node( 2*sizeof(int), i ) ;
	  sctk_assert( instance->mvps[current_mvp]->tree_rank != NULL ) ;
	  instance->mvps[current_mvp]->tree_rank[0] = n->rank ;
	  instance->mvps[current_mvp]->tree_rank[1] = j ;
	  instance->mvps[current_mvp]->root = root ;
	  instance->mvps[current_mvp]->father = n ;

	  n->children.leaf[j] = instance->mvps[current_mvp] ;

	  sctk_thread_attr_t __attr ;
	  size_t stack_size;
	  int res ;

	  sctk_thread_attr_init(&__attr);

	  sctk_thread_attr_setbinding (& __attr, target_vp ) ;

	  if (sctk_is_in_fortran == 1)
	    stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	  else
	    stack_size = SCTK_ETHREAD_STACK_SIZE;

	  sctk_thread_attr_setstacksize (&__attr, stack_size);

	  switch ( flag_level ) {
	    case 0: /* Root */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on root", current_mvp, target_vp, i ) ;
	      instance->mvps[current_mvp]->to_run = root ;
	      break ;
	    case 1: /* Numa */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on NUMA node %d", current_mvp, target_vp, i, i ) ;
	      instance->mvps[current_mvp]->to_run = root->children.node[i] ;
	      res =
		sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		    mpcomp_slave_mvp_node,
		    instance->mvps[current_mvp]);
	      sctk_assert (res == 0);
	      break ;
	    case 2: /* MicroVP */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) w/ internal spinning", current_mvp, target_vp, i ) ;
	      res =
		sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		    mpcomp_slave_mvp_leaf,
		    instance->mvps[current_mvp]);
	      sctk_assert (res == 0);
	      break ;
	    default:
	      not_reachable() ;
	      break ;
	  }


	  sctk_thread_attr_destroy(&__attr);

	  current_mvp++ ;
	  flag_level = -1 ;
	}
      }
#endif

      break ;
    case 32:
      // not_implemented() ;
#if 0 /* NUMA Tree 4 Degree */
#warning "OpenMP Compiling w/ 2-level NUMA tree (32 cores)"
      sctk_nodebug( "Building OpenMP 4-2-4 tree for 32 cores" ) ;
      root->father = NULL ;
      root->rank = -1 ;
      root->depth = 0 ;
      root->nb_children = 4 ;
      root->min_index = 0 ;
      root->max_index = 31 ;
      root->child_type = CHILDREN_NODE ;
      root->lock = SCTK_SPINLOCK_INITIALIZER ;
      root->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
      sctk_atomics_store_int( &(root->barrier), 0 ) ;
#else
      root->barrier = 0 ;
#endif
      root->barrier_done = 0 ;
      root->children.node = (mpcomp_node **)sctk_malloc( root->nb_children * sizeof( mpcomp_node *) ) ;
      sctk_assert( root->children.node != NULL ) ;

      /* Depth 1 -> output degree 2*/
      for ( i = 0 ; i < root->nb_children ; i++ ) {

	mpcomp_node * n ;
	int j ;

	if ( flag_level == -1 ) { flag_level = 1 ; }

	n = (mpcomp_node *)sctk_malloc_on_node( sizeof( mpcomp_node ), i ) ;
	sctk_assert( n != NULL ) ;

	root->children.node[i] = n ;

	n->father = root ;
	n->rank = i ;
	n->depth = 1 ;
	n->nb_children = 2 ;
	n->min_index = i*8 ;
	n->max_index = (i+1)*8-1 ;
	n->child_type = CHILDREN_NODE ;
	n->lock = SCTK_SPINLOCK_INITIALIZER ;
	n->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	sctk_atomics_store_int( &(n->barrier), 0 ) ;
#else
	n->barrier = 0 ;
#endif
	n->barrier_done = 0 ;
	n->children.leaf = (mpcomp_mvp **)sctk_malloc_on_node( n->nb_children* sizeof(mpcomp_mvp *), i ) ;

	sctk_nodebug( "__mpcomp_instance_init: Tree node (%d) rang [ %d, %d ]", i, n->min_index, n->max_index ) ;

	/* Depth 2 -> output degree 4*/
	for ( j = 0 ; j < n->nb_children ; j++ ) {
	  mpcomp_node * n2 ;
	  int k ;

	  if ( flag_level == -1 ) { flag_level = 2 ; }

	  n2 = (mpcomp_node *)sctk_malloc_on_node( sizeof( mpcomp_node ), i ) ;
	  sctk_assert( n2 != NULL ) ;

	  n->children.node[j] = n2 ;

	  n2->father = n ;
	  n2->rank = j ;
	  n2->depth = 2 ;
	  n2->nb_children = 4 ;
	  n2->min_index = i*8 + j*4;
	  n2->max_index = i*8 + (j+1)*4 - 1 ;
	  n2->child_type = CHILDREN_LEAF ;
	  n2->lock = SCTK_SPINLOCK_INITIALIZER ;
	  n2->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	  sctk_atomics_store_int( &(n2->barrier), 0 ) ;
#else
	  n2->barrier = 0 ;
#endif
	  n2->barrier_done = 0 ;
	  n2->children.leaf = (mpcomp_mvp **)sctk_malloc_on_node( n2->nb_children* sizeof(mpcomp_mvp *), i ) ;

	  sctk_nodebug( "__mpcomp_instance_init: Tree node (%d,%d) rang [ %d, %d ]", i, j, n2->min_index, n2->max_index ) ;

	  /* Depth 3 -> leaves */
	  for ( k = 0 ; k < n2->nb_children ; k++ ) {
	    /* This are leaves -> create the microVP */
	    int target_vp ;

	    target_vp = order[8*i+4*j+k] ;

	    if ( flag_level == -1 ) { flag_level = 3 ; }

	    /* Allocate memory (on the right NUMA Node) */
	    instance->mvps[current_mvp] = (mpcomp_mvp *)sctk_malloc_on_node(
		sizeof(mpcomp_mvp), i ) ;


	    /* Get the set of registers */
	    sctk_getcontext (&(instance->mvps[current_mvp]->vp_context));

	    /* Initialize the corresponding microVP (all but tree-related variables) */
	    instance->mvps[current_mvp]->nb_threads = 0 ;
	    instance->mvps[current_mvp]->next_nb_threads = 0 ;
	    instance->mvps[current_mvp]->rank = current_mvp ;
	    instance->mvps[current_mvp]->enable = 1 ;
	    instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node( 3*sizeof(int), i ) ;
	    sctk_assert( instance->mvps[current_mvp]->tree_rank != NULL ) ;
	    instance->mvps[current_mvp]->tree_rank[0] = i ;
	    instance->mvps[current_mvp]->tree_rank[1] = j ;
	    instance->mvps[current_mvp]->tree_rank[2] = k ;
	    instance->mvps[current_mvp]->root = root ;
	    instance->mvps[current_mvp]->father = n2 ;

	    n2->children.leaf[k] = instance->mvps[current_mvp] ;

	    sctk_thread_attr_t __attr ;
	    size_t stack_size;
	    int res ;

	    sctk_thread_attr_init(&__attr);

	    sctk_thread_attr_setbinding (& __attr, target_vp ) ;

#warning "BUG car target_vp vaut m'importe quoi"

	    if (sctk_is_in_fortran == 1)
	      stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	    else
	      stack_size = SCTK_ETHREAD_STACK_SIZE;

	    sctk_thread_attr_setstacksize (&__attr, stack_size);

	    switch ( flag_level ) {
	      case 0: /* Root */
		sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on root (%d,%d,%d)", 
		    current_mvp, target_vp, i, i, j, k ) ;
		instance->mvps[current_mvp]->to_run = root ;
		/*
		   res =
		   sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		   mpcomp_slave_mvp_node,
		   instance->mvps[current_mvp]);
		   sctk_assert (res == 0);
		 */
		break ;
	      case 1: /* Numa */
		sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on NUMA node %d (%d,%d,%d)", 
		    current_mvp, target_vp, i, i, i, j, k ) ;
		instance->mvps[current_mvp]->to_run = root->children.node[i] ;
		res =
		  sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		      mpcomp_slave_mvp_node,
		      instance->mvps[current_mvp]);
		sctk_assert (res == 0);
		break ;
	      case 2: /* Numa */
		sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on NUMA node %d (level 2) (%d,%d,%d)", 
		    current_mvp, target_vp, i, i, i, j, k ) ;
		instance->mvps[current_mvp]->to_run = root->children.node[i]->children.node[j] ;
		res =
		  sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		      mpcomp_slave_mvp_node,
		      instance->mvps[current_mvp]);
		sctk_assert (res == 0);
		break ;
	      case 3: /* MicroVP */
		sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) w/ internal spinning (%d,%d,%d)", 
		    current_mvp, target_vp, i, i, j, k ) ;
		res =
		  sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		      mpcomp_slave_mvp_leaf,
		      instance->mvps[current_mvp]);
		sctk_assert (res == 0);
		break ;
	      default:
		not_reachable() ;
		break ;
	    }



	    sctk_thread_attr_destroy(&__attr);

	    current_mvp++ ;
	    flag_level = -1 ;
	  }
	}
      }
#endif

#if 1 /* NUMA Tree */
//#warning "OpenMP Compiling w/ NUMA tree (32 cores)"
//#warning "NO USE ATOMICS ET MALLOC ON NODE POUR LES OPTIONS"
      sctk_nodebug( "Building OpenMP 4-8 tree for 32 cores" ) ;
      root->father = NULL ;
      root->rank = -1 ;
      root->depth = 0 ;
      root->nb_children = 4 ;
      root->min_index = 0 ;
      root->max_index = 31 ;
      root->child_type = MPCOMP_CHILDREN_NODE ;
      root->lock = SCTK_SPINLOCK_INITIALIZER ;
      root->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
      sctk_atomics_store_int( &(root->barrier), 0 ) ;
      /* Chunks infos */
      sctk_atomics_store_int( &(root->chunks_avail), MPCOMP_CHUNKS_AVAIL);
      sctk_atomics_store_int( &(root->nb_chunks_empty_children), 0);
#else
      root->barrier = 0 ;
      /* Chunks infos */
      root->chunks_avail = MPCOMP_CHUNKS_AVAIL;
      root->nb_chunks_empty_children = 0;
#endif
      root->barrier_done = 0 ;
      root->children.node = (mpcomp_node_t **)sctk_malloc( root->nb_children * sizeof( mpcomp_node_t *) ) ;
      sctk_assert( root->children.node != NULL );

      sctk_nodebug("Building tree for 32 cores: root has %d children", root->nb_children); 

      for ( i = 0 ; i < root->nb_children ; i++ ) {
	mpcomp_node_t * n ;
	int j ;

	if ( flag_level == -1 ) { flag_level = 1 ; }

	n = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), i ) ;
	sctk_assert( n != NULL ) ;

	root->children.node[i] = n ;

	n->father = root ;
	n->rank = i ;
	n->depth = 1 ;
	n->nb_children = 8 ;
	n->min_index = i*8 ;
	n->max_index = (i+1)*8 ;
	n->child_type = MPCOMP_CHILDREN_LEAF ;
	n->lock = SCTK_SPINLOCK_INITIALIZER ;
	n->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	sctk_atomics_store_int( &(n->barrier), 0 ) ;
        /* Chunks infos */
        sctk_atomics_store_int( &(n->chunks_avail), MPCOMP_CHUNKS_AVAIL);
        sctk_atomics_store_int( &(n->nb_chunks_empty_children), 0);
#else
	n->barrier = 0 ;
        /* Chunks infos*/
        n->chunks_avail = MPCOMP_CHUNKS_AVAIL;
        n->nb_chunks_empty_children = 0;
#endif
	n->barrier_done = 0 ;
	n->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node( n->nb_children* sizeof(mpcomp_mvp_t *), i ) ;

        sctk_nodebug("node %d has %d children", i, n->nb_children); 

	for ( j = 0 ; j < n->nb_children ; j++ ) {
	  /* This are leaves -> create the microVP */
	  int target_vp ;

	  target_vp = order[8*i+j] ;

	  if ( flag_level == -1 ) { flag_level = 2 ; }

	  /* Allocate memory (on the right NUMA Node) */
	  instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(
	      sizeof(mpcomp_mvp_t), i ) ;


	  /* Get the set of registers */
	  sctk_getcontext (&(instance->mvps[current_mvp]->vp_context));

	  /* Initialize the corresponding microVP (all but tree-related variables) */
	  instance->mvps[current_mvp]->nb_threads = 0 ;
	  instance->mvps[current_mvp]->next_nb_threads = 0 ;

            {
	      sctk_nodebug( "////////// MVP INITIALIZATION //////////" ) ;
	      int i_thread ;
	      for ( i_thread = 0 ; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP ; i_thread++ ) {
		int i_fordyn ;
		for ( i_fordyn = 0 ; i_fordyn < MPCOMP_MAX_ALIVE_FOR_DYN + 1 ; i_fordyn++ ) {
                  //sctk_nodebug("Initializing chunks info (remain=-1)");
		  sctk_atomics_store_int( 
		      &(instance->mvps[current_mvp]->threads[i_thread].for_dyn_chunk_info[i_fordyn].remain), 
		      -1 ) ;
		}
	      }
	    }   

	  instance->mvps[current_mvp]->rank = current_mvp ;
	  instance->mvps[current_mvp]->enable = 1 ;
	  instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node( 2*sizeof(int), i ) ;
	  sctk_assert( instance->mvps[current_mvp]->tree_rank != NULL ) ;
	  instance->mvps[current_mvp]->tree_rank[0] = n->rank ;
	  instance->mvps[current_mvp]->tree_rank[1] = j ;
	  instance->mvps[current_mvp]->root = root ;
	  instance->mvps[current_mvp]->father = n ;

	  n->children.leaf[j] = instance->mvps[current_mvp] ;

	  sctk_thread_attr_t __attr ;
	  size_t stack_size;
	  int res ;

	  sctk_thread_attr_init(&__attr);

	  sctk_thread_attr_setbinding (& __attr, target_vp ) ;

	  if (sctk_is_in_fortran == 1)
	    stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	  else
	    stack_size = SCTK_ETHREAD_STACK_SIZE;

	  sctk_thread_attr_setstacksize (&__attr, stack_size);

	  switch ( flag_level ) {
	    case 0: /* Root */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on root", current_mvp, target_vp, i ) ;
	      instance->mvps[current_mvp]->to_run = root ;
/*
	      res =
		sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		    mpcomp_slave_mvp_node,
		    instance->mvps[current_mvp]);
	      sctk_assert (res == 0);
*/
	      break ;
	    case 1: /* Numa */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on NUMA node %d", current_mvp, target_vp, i, i ) ;
	      instance->mvps[current_mvp]->to_run = root->children.node[i] ;
	      res =
		sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		    mpcomp_slave_mvp_node,
		    instance->mvps[current_mvp]);
	      sctk_assert (res == 0);
	      break ;
	    case 2: /* MicroVP */
	      sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) w/ internal spinning", current_mvp, target_vp, i ) ;
	      res =
		sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		    mpcomp_slave_mvp_leaf,
		    instance->mvps[current_mvp]);
	      sctk_assert (res == 0);
	      break ;
	    default:
	      not_reachable() ;
	      break ;
	  }



	  sctk_thread_attr_destroy(&__attr);

	  current_mvp++ ;
	  flag_level = -1 ;
	}
      }
#endif

      break ;
    case 128:

#if 1 /* NUMA Tree */
//#warning "OpenMP Compiling w/ NUMA tree (128 cores)"
      root->father = NULL ;
      root->rank = -1 ;
      root->depth = 0 ;
      root->nb_children = 4 ;
      root->min_index = 0 ;
      root->max_index = 127 ;
      root->child_type = MPCOMP_CHILDREN_NODE ;
      root->lock = SCTK_SPINLOCK_INITIALIZER ;
      root->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
      sctk_atomics_store_int( &(root->barrier), 0 ) ;
      /* Chunks infos */
      sctk_atomics_store_int( &(root->chunks_avail), MPCOMP_CHUNKS_AVAIL);
      sctk_atomics_store_int( &(root->nb_chunks_empty_children), 0);
#else
      root->barrier = 0 ;
      /* Chunks infos */
      root->chunks_avail = MPCOMP_CHUNKS_AVAIL;
      root->nb_chunks_empty_children = 0;
#endif
      root->barrier_done = 0 ;
      root->children.node = (mpcomp_node_t **)sctk_malloc_on_node( root->nb_children * sizeof( mpcomp_node_t *), 0 ) ;
      sctk_assert( root->children.node != NULL ) ;

      /* Depth 1 -> output degree 4*/
      for ( i = 0 ; i < root->nb_children ; i++ ) {

	mpcomp_node_t * n ;
	int j ;

	if ( flag_level == -1 ) { flag_level = 1 ; }

#if MPCOMP_MALLOC_ON_NODE
	n = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), 4*i ) ;
#else
	n = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), 0 ) ;
#endif
	sctk_assert( n != NULL ) ;

	root->children.node[i] = n ;

	n->father = root ;
	n->rank = i ;
	n->depth = 1 ;
	n->nb_children = 4 ;
	n->min_index = i*32 ;
	n->max_index = (i+1)*32-1 ;
	n->child_type = MPCOMP_CHILDREN_NODE ;
	n->lock = SCTK_SPINLOCK_INITIALIZER ;
	n->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	sctk_atomics_store_int( &(n->barrier), 0 ) ;
        /* Chunks infos*/
        sctk_atomics_store_int( &(n->chunks_avail), MPCOMP_CHUNKS_AVAIL);
        sctk_atomics_store_int( &(n->nb_chunks_empty_children), 0); 
#else
	n->barrier = 0 ;
        /* Chunks infos */
        n->chunks_avail = MPCOMP_CHUNKS_AVAIL;
        n->nb_chunks_empty_children = 0;
#endif
	n->barrier_done = 0 ;
#if MPCOMP_MALLOC_ON_NODE
	n->children.node = (mpcomp_node_t **)sctk_malloc_on_node( n->nb_children* sizeof(mpcomp_node_t *), 4*i ) ;
#else
	n->children.node = (mpcomp_node_t **)sctk_malloc_on_node( n->nb_children* sizeof(mpcomp_node_t *), 0 ) ;
#endif

	sctk_nodebug( "__mpcomp_instance_init: Tree node (%d) rang [ %d, %d ]", i, n->min_index, n->max_index ) ;

	/* Depth 2 -> output degree 8*/
	for ( j = 0 ; j < n->nb_children ; j++ ) {
	  mpcomp_node_t * n2 ;
	  int k ;

	  if ( flag_level == -1 ) { flag_level = 2 ; }

#if MPCOMP_MALLOC_ON_NODE
	  n2 = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), 4*i+j ) ;
#else
	  n2 = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), 0 ) ;
#endif
	  sctk_assert( n2 != NULL ) ;

	  n->children.node[j] = n2 ;

	  n2->father = n ;
	  n2->rank = j ;
	  n2->depth = 2 ;
	  n2->nb_children = 8 ;
	  n2->min_index = i*32 + j*8;
	  n2->max_index = i*32 + (j+1)*8 - 1 ;
	  n2->child_type = MPCOMP_CHILDREN_LEAF ;
	  n2->lock = SCTK_SPINLOCK_INITIALIZER ;
	  n2->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	  sctk_atomics_store_int( &(n2->barrier), 0 ) ;
          /* Chunks infos */
          sctk_atomics_store_int( &(n2->chunks_avail), MPCOMP_CHUNKS_AVAIL);
          sctk_atomics_store_int( &(n2->nb_chunks_empty_children), 0);
#else
	  n2->barrier = 0 ;
          /* Chunks infos */
          n2->chunks_avail = MPCOMP_CHUNKS_AVAIL;
          n2->nb_chunks_empty_children = 0;
#endif
	  n2->barrier_done = 0 ;
#if MPCOMP_MALLOC_ON_NODE
	  n2->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node( n2->nb_children* sizeof(mpcomp_mvp_t *), 4*i+j ) ;
#else
	  n2->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node( n2->nb_children* sizeof(mpcomp_mvp_t *), 0 ) ;
#endif

	  sctk_nodebug( "__mpcomp_instance_init: Tree node (%d,%d) rang [ %d, %d ]", i, j, n2->min_index, n2->max_index ) ;

	  /* Depth 3 -> leaves */
	  for ( k = 0 ; k < n2->nb_children ; k++ ) {
	    /* This are leaves -> create the microVP */
	    int target_vp ;

	    target_vp = order[32*i+8*j+k] ;

	    if ( flag_level == -1 ) { flag_level = 3 ; }

	    /* Allocate memory (on the right NUMA Node) */
#if MPCOMP_MALLOC_ON_NODE
	    instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(
		sizeof(mpcomp_mvp_t), 4*i+j ) ;
#else
	    instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(
		sizeof(mpcomp_mvp_t), 0 ) ;
#endif


	    /* Get the set of registers */
	    sctk_getcontext (&(instance->mvps[current_mvp]->vp_context));

	    /* Initialize the corresponding microVP (all but tree-related variables) */
	    /* TODO put the microVP initialization in a function somewhere. If possible where ze could call a function to initialize each thread */
	    instance->mvps[current_mvp]->nb_threads = 0 ;
	    instance->mvps[current_mvp]->next_nb_threads = 0 ;
	    {
	      sctk_nodebug( "////////// MVP INITIALIZATION //////////" ) ;
	      int i_thread ;
	      for ( i_thread = 0 ; i_thread < MPCOMP_MAX_THREADS_PER_MICROVP ; i_thread++ ) {
		int i_fordyn ;
		for ( i_fordyn = 0 ; i_fordyn < MPCOMP_MAX_ALIVE_FOR_DYN + 1 ; i_fordyn++ ) {
                 sctk_nodebug("[__mpcomp_build_default_tree] set remain to -1");
		  sctk_atomics_store_int( 
		      &(instance->mvps[current_mvp]->threads[i_thread].for_dyn_chunk_info[i_fordyn].remain), 
		      -1 ) ;
		}
	      }
	    }
	    instance->mvps[current_mvp]->rank = current_mvp ;
	    instance->mvps[current_mvp]->enable = 1 ;
#if MPCOMP_MALLOC_ON_NODE
	    instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node( 3*sizeof(int), 4*i+j ) ;
#else
	    instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node( 3*sizeof(int), 0 ) ;
#endif
	    sctk_assert( instance->mvps[current_mvp]->tree_rank != NULL ) ;
	    instance->mvps[current_mvp]->tree_rank[0] = i ;
	    instance->mvps[current_mvp]->tree_rank[1] = j ;
	    instance->mvps[current_mvp]->tree_rank[2] = k ;
	    instance->mvps[current_mvp]->root = root ;
	    instance->mvps[current_mvp]->father = n2 ;

	    n2->children.leaf[k] = instance->mvps[current_mvp] ;

	    sctk_thread_attr_t __attr ;
	    size_t stack_size;
	    int res ;

	    sctk_thread_attr_init(&__attr);

	    sctk_thread_attr_setbinding (& __attr, target_vp ) ;

	    if (sctk_is_in_fortran == 1)
	      stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	    else
	      stack_size = SCTK_ETHREAD_STACK_SIZE;

	    sctk_thread_attr_setstacksize (&__attr, stack_size);

	    switch ( flag_level ) {
	      case 0: /* Root */
		sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on root (%d,%d,%d)", 
		    current_mvp, target_vp, 4*i+j, i, j, k ) ;
		instance->mvps[current_mvp]->to_run = root ;
		/*
		   res =
		   sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		   mpcomp_slave_mvp_node,
		   instance->mvps[current_mvp]);
		   sctk_assert (res == 0);
		 */
		break ;
	      case 1: /* Numa */
		sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on NUMA node %d (%d,%d,%d)", 
		    current_mvp, target_vp, 4*i+j, i, i, j, k ) ;
		instance->mvps[current_mvp]->to_run = root->children.node[i] ;
		res =
		  sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		      mpcomp_slave_mvp_node,
		      instance->mvps[current_mvp]);
		sctk_assert (res == 0);
		break ;
	      case 2: /* Numa */
		sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) spinning on NUMA node %d (level 2) (%d,%d,%d)", 
		    current_mvp, target_vp, 4*i+j, i, i, j, k ) ;
		instance->mvps[current_mvp]->to_run = root->children.node[i]->children.node[j] ;
		res =
		  sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		      mpcomp_slave_mvp_node,
		      instance->mvps[current_mvp]);
		sctk_assert (res == 0);
		break ;
	      case 3: /* MicroVP */
		sctk_nodebug( "__mpcomp_instance_init: Creating microVP %d (VP %d, NUMA %d) w/ internal spinning (%d,%d,%d)", 
		    current_mvp, target_vp, 4*i+j, i, j, k ) ;
		res =
		  sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), &__attr,
		      mpcomp_slave_mvp_leaf,
		      instance->mvps[current_mvp]);
		sctk_assert (res == 0);
		break ;
	      default:
		not_reachable() ;
		break ;
	    }



	    sctk_thread_attr_destroy(&__attr);

	    current_mvp++ ;
	    flag_level = -1 ;
	  }
	}
      }
#endif

      break;
    default:
      //not_implemented() ;
      sctk_nodebug("not implemented");
      break ;
  }

  sctk_free( order ) ;

#if 0
     hwloc_topology_t h ;
     h = sctk_get_topology_object() ;

     int d = hwloc_topology_get_depth( h ) ;
     // int d = hwloc_get_type_depth( h, HWLOC_OBJ_PU ) ;

     sctk_nodebug( "String for PU: %s", hwloc_obj_type_string( HWLOC_OBJ_PU ) ) ;

     int * deg ;

     deg = (int *)malloc( d * sizeof( int ) ) ;
     sctk_assert( deg != NULL ) ;

     for ( i = 0 ; i < d ; i++ ) {
     deg[i] = hwloc_get_nbobjs_by_depth( h, i ) ;
     }

     int n = 1 ;

     for ( i = 0 ; i < d ; i++ ) {
     n *= deg[i] ;
     }

     sctk_nodebug( "Computed default tree with depth %d for %d leaves", d, n ) ;
     for ( i = 0 ; i < d ; i++ ) {
       sctk_nodebug( "\tdepth[%d] = %d", i, deg[i] ) ;
     }

     /*
     return __mpcomp_build_tree( instance, n, d, deg ) ;
     */
#endif

     __mpcomp_print_tree( instance ) ;
  return 1 ;
}
#endif


int __mpcomp_build_tree( mpcomp_instance_t * instance, int n_leaves, int depth, int * degree ) {
	  mpcomp_node_t * root ; /* Root of the tree */
	  int current_mpc_vp; /* Current VP where the master is */
	  int current_mvp ; /* Current microVP we are dealing with */
	  int * order ; /* Topological sort of VPs where order[0] is the current VP */
	  mpcomp_stack_t * s ; /* Stack used to create the tree with a DFS */
	  int max_stack_elements ; /* Max elements of the stack computed thanks to depth and degrees */
	  int previous_depth ; /* What was the previously stacked depth (to check if depth is increasing or decreasing on the stack) */
	  mpcomp_node_t * target_node ; /* Target node to spin when creating the next mVP */
	  int i ;


	  /* Check input parameters */
	  sctk_assert( instance != NULL );
	  sctk_assert( instance->mvps != NULL ) ;
	  sctk_assert( __mpcomp_check_tree_parameters( n_leaves, depth, degree ) ) ;

	  /* Compute the max number of elements on the stack */
	  max_stack_elements = 1 ;
	  for ( i = 0 ; i < depth ; i++ ) {
	  max_stack_elements += degree[i] ;
     }

	  sctk_nodebug( "__mpcomp_build_tree: max_stack_elements = %d", max_stack_elements ) ;

	  /* Allocate memory for the node stack */
	  s = __mpcomp_create_stack( max_stack_elements ) ;
	  sctk_assert( s != NULL ) ;

	  /* Get the current VP number */
	  current_mpc_vp = sctk_thread_get_vp ();

	  /* TODO So far, we do not fully support when the OpenMP instance is created from any VP */
	  sctk_assert( current_mpc_vp == 0 ) ;

	  /* Grab the right order to allocate microVPs (sctk_get_neighborhood) */
	  order = sctk_malloc( sctk_get_cpu_number () * sizeof( int ) ) ;
	  sctk_assert( order != NULL ) ;

	  sctk_get_neighborhood( current_mpc_vp, sctk_get_cpu_number (), order ) ;

	  /* Build the tree of this OpenMP instance */

	  root = (mpcomp_node_t *)sctk_malloc( sizeof( mpcomp_node_t ) ) ;
	  sctk_assert( root != NULL ) ;

	  instance->root = root ;

	  current_mvp = 0 ;
	  previous_depth = -1 ;
	  target_node = NULL ;

	  /* Fill root information */
	  root->father = NULL ;
	  root->rank = -1 ;
	  root->depth = 0 ;
	  root->nb_children = degree[0] ;
	  root->min_index = 0 ;
	  root->max_index = n_leaves ;
	  root->lock = SCTK_SPINLOCK_INITIALIZER ;
	  root->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
	  sctk_atomics_store_int( &(root->barrier), 0 ) ;
#else
	  root->barrier = 0 ;
#endif
	  root->barrier_done = 0 ;

	  __mpcomp_push( s, root ) ;

	  while ( !__mpcomp_is_stack_empty( s ) ) {
	       mpcomp_node_t * n ;
	       int target_vp ;
	       int target_numa ;

	       n = __mpcomp_pop( s ) ;
	       sctk_assert( n != NULL ) ;

	       if (previous_depth == -1 ) {
		    target_node = root ;
	       } else {
		    if ( n->depth < previous_depth ) {
			 target_node = n ;
		    }
	       }
	       previous_depth = n->depth ;

	       target_vp = order[ current_mvp ] ;
#if MPCOMP_MALLOC_ON_NODE
	       target_numa = sctk_get_node_from_cpu( target_vp ) ;
#else
	       target_numa = 0 ;
#endif

#ifdef MPCOMP_TASK
	       n->untied_tasks = NULL;
	       n->new_tasks = NULL;
#endif //MPCOMP_TASK

	       if ( n->depth == depth - 1 ) { 

		    /* Children are leaves */
		    n->child_type = MPCOMP_CHILDREN_LEAF ;
		    n->children.leaf = (mpcomp_mvp_t **) sctk_malloc_on_node( 
			 n->nb_children * sizeof( mpcomp_mvp_t * ), target_numa ) ;

		    for ( i = 0 ; i < n->nb_children ; i++ ) {
			 /* Allocate memory (on the right NUMA Node) */
			 instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(
			      sizeof(mpcomp_mvp_t), target_numa ) ;

			 /* Get the set of registers */
			 sctk_getcontext (&(instance->mvps[current_mvp]->vp_context));

			 /* Initialize the corresponding microVP (all but tree-related variables) */
			 instance->mvps[current_mvp]->children_instance = instance ;
			 instance->mvps[current_mvp]->nb_threads = 0 ;
			 instance->mvps[current_mvp]->next_nb_threads = 0 ;
			 instance->mvps[current_mvp]->rank = current_mvp ;
			 instance->mvps[current_mvp]->vp = target_vp ;
			 instance->mvps[current_mvp]->enable = 1 ;
			 instance->mvps[current_mvp]->tree_rank = 
			      (int *)sctk_malloc_on_node( depth*sizeof(int), target_numa ) ;
			 sctk_assert( instance->mvps[current_mvp]->tree_rank != NULL ) ;

			 instance->mvps[current_mvp]->tree_rank[ depth - 1 ] = i ;

#ifdef MPCOMP_TASK
			 instance->mvps[current_mvp]->untied_tasks = NULL ;
			 instance->mvps[current_mvp]->new_tasks = NULL ;
#endif //MPCOMP_TASK

			 mpcomp_node_t * current_node = n ;
			 while ( current_node->father != NULL ) {

			      instance->mvps[current_mvp]->
				   tree_rank[ current_node->depth - 1 ] = current_node->rank ;

			      current_node = current_node->father ;
			 }

			 instance->mvps[current_mvp]->root = root ;
			 instance->mvps[current_mvp]->father = n ;

			 n->children.leaf[i] = instance->mvps[current_mvp] ;

			 sctk_thread_attr_t __attr ;
			 int res ;

			 sctk_thread_attr_init(&__attr);

			 sctk_thread_attr_setbinding (& __attr, target_vp ) ;

			 sctk_thread_attr_setstacksize (&__attr, mpcomp_global_icvs.stacksize_var);

			 /* User thread create... */
			 instance->mvps[current_mvp]->to_run = target_node ;

			 if ( target_node != root ) {
			      if ( i == 0 ) {
				   sctk_assert( target_node != NULL ) ;
				   /* The first child is spinning on a node */
				   instance->mvps[current_mvp]->to_run = target_node ;
				   res = 
					sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), 
								 &__attr, mpcomp_slave_mvp_node, instance->mvps[current_mvp]);
				   sctk_assert (res == 0);
			      }
			      else {
				   /* The other children are regular leaves */
				   instance->mvps[current_mvp]->to_run = NULL ;
				   res = 
					sctk_user_thread_create (&(instance->mvps[current_mvp]->pid), 
								 &__attr, mpcomp_slave_mvp_leaf, instance->mvps[current_mvp]);
				   sctk_assert (res == 0);
			      }
			 } else {
			      /* Special case when depth==1, the current node is root */
			      if ( n == root ) {
				   target_node = NULL ;
				   sctk_assert( i == 0 ) ;
			      } else {
				   target_node = n ;
			      }
			 }

			 sctk_thread_attr_destroy(&__attr);

			 current_mvp++ ;

			 /* Recompute the target vp/numa */
			 target_vp = order[ current_mvp ] ;
#if MPCOMP_MALLOC_ON_NODE
			 target_numa = sctk_get_node_from_cpu( target_vp ) ;
#else
			 target_numa = 0 ;
#endif

			 /* We reached the leaves */
			 previous_depth++ ;
		    }
	       } else {
		    /* Children are nodes */
		    n->child_type = MPCOMP_CHILDREN_NODE ;
		    n->children.node= (mpcomp_node_t **) sctk_malloc_on_node( 
			 n->nb_children * sizeof( mpcomp_node_t * ), target_numa ) ;

		    /* Traverse children in reverse order for correct ordering during the DFS */
		    for ( i = n->nb_children - 1 ; i >= 0 ; i-- ) {
			 mpcomp_node_t * n2 ;

			 n2 = (mpcomp_node_t *)sctk_malloc_on_node( sizeof( mpcomp_node_t ), target_numa ) ;

			 n->children.node[ i ] = n2 ;

			 n2->father = n ;
			 n2->rank = i ;
			 n2->depth = n->depth+1 ;
			 n2->nb_children = degree[ n2->depth ] ;
			 n2->min_index = n->min_index + i * (n->max_index - n->min_index) / degree[ n->depth ] ;
			 n2->max_index = n->min_index + (i+1) * (n->max_index - n->min_index) / degree[ n->depth ] - 1 ;
			 n2->lock = SCTK_SPINLOCK_INITIALIZER ;
			 n2->slave_running = 0 ;
#if MPCOMP_USE_ATOMICS
			 sctk_atomics_store_int( &(n2->barrier), 0 ) ;
#else
			 n2->barrier = 0 ;
#endif
			 n2->barrier_done = 0 ;

			 __mpcomp_push( s, n2 ) ;

		    }
	       }
	  }


	  /* Free memory */
	  __mpcomp_free_stack( s ) ;
	  free( s ) ;
	  free( order );

	  __mpcomp_print_tree( instance ) ;

	  return 0 ;

	  }

void __mpcomp_print_tree( mpcomp_instance_t * instance ) {

     mpcomp_stack_t * s ;
     int i, j ;


     sctk_assert( instance != NULL ) ;

     /* TODO compute the real number of max elements for this stack */
     s = __mpcomp_create_stack( 2048 ) ;
     sctk_assert( s != NULL ) ;

     __mpcomp_push( s, instance->root ) ;

     fprintf( stderr, "==== Printing the current OpenMP tree ====\n" ) ;

     while ( !__mpcomp_is_stack_empty( s ) ) {
	  mpcomp_node_t * n ;

	  n = __mpcomp_pop( s ) ;
	  sctk_assert( n != NULL ) ;

	  /* Print this node */

	  /* Add tab according to depth */
	  for ( i = 0 ; i < n->depth ; i++ ) {
	       fprintf( stderr, "\t" ) ;
	  }

	  fprintf( stderr, "Node %ld (@ %p)\n", n->rank, n ) ;

	  switch( n->child_type ) {
	  case MPCOMP_CHILDREN_NODE:
	       for ( i = n->nb_children - 1 ; i >= 0 ; i-- ) {
		    __mpcomp_push( s, n->children.node[i] ) ;
	       }
	       break ;
	  case MPCOMP_CHILDREN_LEAF:
	       for ( i = 0 ; i < n->nb_children ; i++ ) {
		    mpcomp_mvp_t * mvp ;

		    mvp = n->children.leaf[i] ;
		    sctk_assert( mvp != NULL ) ;

		    /* Add tab according to depth */
		    for ( j = 0 ; j < n->depth + 1 ; j++ ) {
			 fprintf( stderr, "\t" ) ;
		    }

		    fprintf( stderr, "Leaf %d rank %d vp %d spinning on %p", i, mvp->rank, mvp->vp, mvp->to_run ) ;

		    fprintf( stderr, " tree_rank" ) ;
		    for ( j = 0 ; j < n->depth + 1 ; j++ ) {
			 fprintf( stderr, " %d", mvp->tree_rank[j] ) ;
		    }


		    fprintf( stderr, "\n" ) ;
	       }
	       break ;
	  default:
	       //not_reachable() ;
	       sctk_nodebug("not reachable"); 
	  }
     }

     __mpcomp_free_stack( s ) ;
     free( s ) ;

     return ;
}
