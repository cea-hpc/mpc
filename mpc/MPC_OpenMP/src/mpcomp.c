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

#include "sctk.h"
#include "sctk_debug.h"
#include "sctk_topology.h"
#include "sctk_runtime_config.h"
#include "sctk_atomics.h"
#include <mpcomp.h>
#include <mpcomp_internal.h>

__thread void *sctk_openmp_thread_tls = NULL;

/* 
  Read the environment variables. Once per process 
*/
inline void __mpcomp_read_env_variables() 
{
  char *env;
  int nb_threads;

  sctk_nodebug("__mpcomp_init: Read environment variables / MPC tank %d",sctk_get_task_rank());  

  /******* OMP_MICROVP_NUMBER *********/
  env = getenv ("OMP_MICROVP_NUMBER");
  /*DEFAULT */
  OMP_MICROVP_NUMBER = sctk_get_processor_number ();
  if (env != NULL) {
      int arg = atoi( env ) ;
      if ( arg <= 0 ) {
	fprintf( stderr, "Warning: Incorrect number of microVPs (OMP_VP_NUMBER=<%s>) -> "
		         "Switching to default value %d\n", env, OMP_MICROVP_NUMBER ) ;
      } 
      else {
	OMP_MICROVP_NUMBER = arg ;
      }
  }

  /******* OMP_SCHEDULE *********/
  env = getenv ("OMP_SCHEDULE");
  /* DEFAULT */
  OMP_SCHEDULE = 1 ;					
  if (env != NULL) {
      int ok = 0 ;
      int offset = 0 ;
      if ( strncmp (env, "static", 6) == 0 ) {
	OMP_SCHEDULE = 1 ;
	offset = 6 ;
	ok = 1 ;
      } 
      if ( strncmp (env, "dynamic", 7) == 0 ) {
	OMP_SCHEDULE = 2 ;
	offset = 7 ;
	ok = 1 ;
      } 
      if ( strncmp (env, "guided", 6) == 0 ) {
	OMP_SCHEDULE = 3 ;
	offset = 6 ;
	ok = 1 ;
      } 
      if ( strncmp (env, "auto", 4) == 0 ) {
	OMP_SCHEDULE = 4 ;
	offset = 4 ;
	ok = 1 ;
      } 
      if ( ok ) {
	int chunk_size = 0 ;
	/* Check for chunk size, if present */
	sctk_nodebug( "Remaining string for schedule: <%s>", &env[offset] ) ;
	switch( env[offset] ) {
	  case ',':
	    sctk_nodebug( "There is a chunk size -> <%s>", &env[offset+1] ) ;
	    chunk_size = atoi( &env[offset+1] ) ;
	    if ( chunk_size <= 0 ) {
	      fprintf (stderr, "Warning: Incorrect chunk size within OMP_SCHEDULE variable: <%s>\n", env ) ;
	      chunk_size = 0 ;
	    } 
	    else {
	      OMP_MODIFIER_SCHEDULE = chunk_size ;
	    }
	    break ;
	  case '\0':
	    sctk_nodebug( "No chunk size\n" ) ;
	    break ;
	  default:
	  fprintf (stderr, "Syntax error with envorinment variable OMP_SCHEDULE <%s>,"
		           " should be \"static|dynamic|guided|auto[,chunk_size]\"\n", env ) ;
	  exit( 1 ) ;
	}
      }  
      else {
	  fprintf (stderr, "Warning: Unknown schedule <%s> (must be static, guided or dynamic),"
		           " fallback to default schedule <%d>\n", env,
		           OMP_SCHEDULE);
      }
    }

  /******* OMP_NUM_THREADS *********/
  env = getenv ("OMP_NUM_THREADS");
  /* DEFAULT */
  OMP_NUM_THREADS = OMP_MICROVP_NUMBER;
  if (env != NULL) {
      nb_threads = atoi (env);
      if (nb_threads > 0) {
	  OMP_NUM_THREADS = nb_threads;
      }
  }

  /******* OMP_DYNAMIC *********/
  env = getenv ("OMP_DYNAMIC");
  /* DEFAULT */
  OMP_DYNAMIC = 0;
  if (env != NULL) {
      if (strcmp (env, "true") == 0) {
	  OMP_DYNAMIC = 1;
      }
  }

  /******* OMP_NESTED *********/
  env = getenv ("OMP_NESTED");
  /* DEFAULT */
  OMP_NESTED = 0;	
  if (env != NULL) {
      if (strcmp (env, "1") == 0 || strcmp (env, "TRUE") == 0 || strcmp (env, "true") == 0 ) {
	  OMP_NESTED = 1;
      }
  }

  /***** PRINT SUMMARY ******/
	  if ( (sctk_runtime_config_get()->modules.launcher.banner) && (sctk_get_process_rank() == 0) ) {
    fprintf (stderr, "MPC OpenMP version %d.%d (DEV)\n",
		     SCTK_OMP_VERSION_MAJOR, SCTK_OMP_VERSION_MINOR);
    fprintf (stderr, "\tOMP_SCHEDULE %d\n", OMP_SCHEDULE);
    fprintf (stderr, "\tOMP_NUM_THREADS %d\n", OMP_NUM_THREADS);
    fprintf (stderr, "\tOMP_DYNAMIC %d\n", OMP_DYNAMIC);
    fprintf (stderr, "\tOMP_NESTED %d\n", OMP_NESTED);
    fprintf (stderr, "\t%d microVPs (OMP_MICROVP_NUMBER)\n", OMP_MICROVP_NUMBER);
  }
 
}

/*
   Entry point for microVP in charge of passing information to other microVPs.
   Spinning on a specific node to wake up.
*/
void *mpcomp_slave_mvp_node (void *arg)
{
   int index;
   mpcomp_mvp_t *mvp;	/* Current microVP */
   mpcomp_node_t *n;	/* Spinning node + Traversing node */
   mpcomp_node_t *root;	/* Tree root */ 
   mpcomp_node_t *numa_node;

   mvp = (mpcomp_mvp_t *)arg;

   n = mvp->to_run;

   root = mvp->root;

   numa_node = root->children.node[mvp->tree_rank[0]];

   while (mvp->enable) {
     int i,j,k;
     int num_threads_mvp;
     int num_threads_per_child;
     int max_index_with_more_threads;

     /* Spinning on the designed node */
     sctk_thread_wait_for_value_and_poll((int *)&(n->slave_running), 1 , NULL , NULL);
     n->slave_running = 0;

     /* Used in case of nb_threads < nb_mvps */
     max_index_with_more_threads = n->num_threads%OMP_MICROVP_NUMBER;

     /* Wake up children node */
     while (n->child_type != CHILDREN_LEAF) {
        int nb_children_involved = 1;

        n->children.node[0]->func = n->func;
        n->children.node[0]->shared = n->shared;
        n->children.node[0]->team = n->team;
        n->children.node[0]->num_threads = n->num_threads;

        for (i=1 ; i<n->nb_children ; i++) {

          num_threads_per_child = n->num_threads / OMP_MICROVP_NUMBER; 

	  if (n->children.node[i]->min_index < max_index_with_more_threads) 
	    num_threads_per_child++;

          if (num_threads_per_child > 0) {
  	    n->children.node[i]->func = n->func;
	    n->children.node[i]->shared = n->shared;
	    n->children.node[i]->team = n->team;
	    n->children.node[i]->num_threads = n->num_threads;
	    n->children.node[i]->slave_running = 1;
            nb_children_involved++;
          }
	}

        n->barrier_num_threads = nb_children_involved;
        n = n->children.node[0];
     }

     /* Wake up children leaf */
     sctk_assert(n->child_type == CHILDREN_LEAF);

     int nb_leaves_involved = 1;

     for (i=1 ; i<n->nb_children ; i++) {

       num_threads_per_child = n->num_threads / OMP_MICROVP_NUMBER; 

       if (n->children.leaf[i]->rank < max_index_with_more_threads)
          num_threads_per_child++;

       if (num_threads_per_child > 0) {
         n->children.leaf[i]->slave_running = 1;
         nb_leaves_involved++;
       }
     }

     n->barrier_num_threads = nb_leaves_involved;

     n = mvp->to_run;
     mvp->func = n->func;
     mvp->shared = n->shared;

     /* TODO: Didn't understand this part */
     /* Compute the number of threads for this microVP */
     int fetched_num_threads = n->num_threads;
     num_threads_mvp = 1;
     if (fetched_num_threads <= mvp->rank) 
       num_threads_mvp = 0;

     for (i=0 ; i<num_threads_mvp ; i++) {
       int rank;
       /* Compute the rank of this thread */
       rank = mvp->rank;
       mvp->threads[i].rank = rank;
       mvp->threads[i].mvp = mvp;
       mvp->threads[i].index_in_mvp = i;
       mvp->threads[i].done = 0;
       mvp->threads[i].team = mvp->father->team;
       mvp->threads[i].hierarchical_tls = NULL;
       mvp->threads[i].current_single = -1;
       mvp->threads[i].private_current_for_dyn = 0;

       mvp->threads[i].is_running = 1;
     }
     /* TODO finish*/

     /* Update the total number of threads for this microVP */
     mvp->nb_threads = num_threads_mvp;

     /* Run */
     in_order_scheduler(mvp);

     /* Barrier to wait for the other microVPs */
     __mpcomp_half_barrier();
#if defined (SCTK_USE_OPTIMIZED_TLS)
	  sctk_tls_module = info->tls_module;
	  sctk_context_restore_tls_module_vp() ;
#endif

TODO("to translate")
   }


   return NULL;
}

/*
   Entry point for microVP working on their own
   Spinning on a variable inside the microVP
*/
void *mpcomp_slave_mvp_leaf (void *arg)
{
   mpcomp_mvp_t *mvp;

   /* Grab the current microVP */
   mvp = (mpcomp_mvp_t *)arg;

   /* Spin while this microVP is alive */  
   while (mvp->enable) {
     int i,j,k;
     int num_threads_mvp;

     /* Spin for new parallel region */
     sctk_thread_wait_for_value_and_poll((int*)&(mvp->slave_running),1,NULL,NULL);
     mvp->slave_running = 0;

     mvp->func = mvp->father->func;
     mvp->shared = mvp->father->shared;

     /* Compute the number of threads for this microVP */
     int fetched_num_threads = mvp->father->num_threads;
     num_threads_mvp = 1;
     if (fetched_num_threads <= mvp->rank) 
       num_threads_mvp = 0;

     for (i=0 ; i<num_threads_mvp ; i++) {
       int rank;
       /* Compute the rank of this thread */
       rank = mvp->rank;
       mvp->threads[i].rank = rank;
       mvp->threads[i].mvp = mvp;
       mvp->threads[i].index_in_mvp = i;
       mvp->threads[i].done = 0;
       mvp->threads[i].team = mvp->father->team;
       mvp->threads[i].hierarchical_tls = NULL;
       mvp->threads[i].current_single = -1;
       mvp->threads[i].private_current_for_dyn = 0;

       mvp->threads[i].is_running = 1;
     }
     /* TODO finish*/

     /* Update the total number of threads for this microVP */
     mvp->nb_threads = num_threads_mvp;

     /* Run */
     in_order_scheduler(mvp);

     /* Barrier to wait for the other microVPs */
     __mpcomp_half_barrier();

   }

   return NULL;
}

/*
   Initialize an OpenMP thread
*/
void __mpcomp_thread_init (mpcomp_thread_t *t)
{
   int i,j;

   t->rank = 0;
   t->mvp = NULL;
   t->index_in_mvp = 0;
   t->done = 0;
   t->hierarchical_tls = NULL;
   t->current_single = -1;
   t->private_current_for_dyn = 0;

}

/*
   Initialize an OpenMP thread team
*/
void __mpcomp_thread_team_init (mpcomp_thread_team_t *team)
{
   int i,j;

   team->depth = 0;
   team->rank = 0;
   team->num_threads = OMP_NUM_THREADS;
   team->hierarchical_tls = NULL;

   /* Init for dynamic scheduling construct */
   team->lock_stop_for_dyn = SCTK_SPINLOCK_INITIALIZER;

   for (i=0 ; i<MPCOMP_MAX_ALIVE_FOR_DYN ; i++) {
      team->lock_exited_for_dyn[i] = SCTK_SPINLOCK_INITIALIZER;
      team->nthread_exited_for_dyn[i] = 0;
   }

   for (i=0 ; i<MPCOMP_MAX_ALIVE_FOR_DYN-1 ; i++) 
     team->stop[i] = MPCOMP_OK;				

   team->stop[MPCOMP_MAX_ALIVE_FOR_DYN-1] = MPCOMP_STOP;

   /* Init for single construct */
   for (i = 0; i <= MPCOMP_MAX_ALIVE_SINGLE; i++) {
	team->lock_enter_single[i] = SCTK_SPINLOCK_INITIALIZER;
	team->nb_threads_entered_single[i] = 0;
   }

   team->nb_threads_entered_single[MPCOMP_MAX_ALIVE_SINGLE] = MPCOMP_NOWAIT_STOP_SYMBOL;
   
   team->nb_threads_stop = 0;

   /* Init for dynamic scheduling construct */
   for (i=0 ; i<MPCOMP_MAX_THREADS ; i++) {
     for (j=0 ; j<MPCOMP_MAX_ALIVE_FOR_DYN ; j++) {
       team->lock_for_dyn[i][j] = SCTK_SPINLOCK_INITIALIZER;
       team->chunk_info_for_dyn[i][j].remain = -1 ;
     }
   }

}

/*
   Initialize the OpenMP instance
*/
void __mpcomp_instance_init (mpcomp_instance_t *instance, int nb_mvps)
{
  int *order;
  int i;
  int current_mvp;
  int flag_level;
  mpcomp_node_t *root;
  int current_mpc_vp;

  /* Alloc memory for 'nb_mvps' microVPs */
  instance->mvps = (mpcomp_mvp_t **)sctk_malloc(nb_mvps*sizeof(mpcomp_mvp_t *));
  sctk_assert(instance->mvps != NULL);

  instance->nb_mvps = nb_mvps;

  /* Get the current VP number */
  current_mpc_vp = sctk_thread_get_vp();

  /* TODO: so far, we don't fully support when OpenMP instance is created from any VP */
  sctk_assert(current_mpc_vp == 0);

  /* Grab the right order to allocate microVPs */
  order = sctk_malloc(sctk_get_cpu_number()*sizeof(int));
  sctk_assert(order != NULL);
  
  sctk_get_neighborhood(current_mpc_vp,sctk_get_cpu_number(),order);

  /* Build the tree of the OpenMP instance */
  root = (mpcomp_node_t *)sctk_malloc(sizeof(mpcomp_node_t));
  sctk_assert(root != NULL);

  instance->root = root;

  current_mvp = 0;
  flag_level = 0;

  switch (sctk_get_cpu_number()) {
	case 8: 
#if 0  /* NUMA tree 8 cores */
#warning "OpenMp compiling w/ NUMA tree 8 cores"	    
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = 2;
	  root->child_type = CHILDREN_NODE;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running = 0;
#ifdef ATOMICS
	  sctk_atomics_store_int (&root->barrier,0) ;
#else
	  root->barrier = 0;
#endif
	  root->barrier_done = 0;
	  root->children.node = (mpcomp_node_t **)sctk_malloc(root->nb_children*sizeof(mpcomp_node_t *));
	  sctk_assert(root->children.node != NULL);

	  for (i=0 ; i<root->nb_children ; i++) {
	    mpcomp_node_t *n;
	    int j;

	    if (flag_level == -1) flag_level = 1;

	    n = (mpcomp_node_t *)sctk_malloc_on_node(sizeof(mpcomp_node_t),i);
	    sctk_assert(n != NULL);

	    root->children.node[i] = n;

	    n->father = root;
	    n->rank = i;
	    n->depth = 1;
	    n->nb_children = 4;
	    n->child_type = CHILDREN_LEAF;
	    n->lock = SCTK_SPINLOCK_INITIALIZER;
	    n->slave_running = 0;
#ifdef ATOMICS
	    sctk_atomics_store_int (&n->barrier,0) ;
#else
	    n->barrier = 0;
#endif
	    n->barrier_done = 0;
	    n->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node(n->nb_children*sizeof(mpcomp_mvp_t *),i);

	    for (j=0 ; j<n->nb_children ; j++) {
	      /* These are leaves -> create the microVP */
	      int target_vp;

	      target_vp = order[4*i+j];

	      if (flag_level == -1) flag_level = 2;

	      /* Allocate memory on the right NUMA node */
	      instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(sizeof(mpcomp_mvp_t),i);

	      /* Get the set of registers */
	      sctk_getcontext(&(instance->mvps[current_mvp]->vp_context));

	      /* Initialize the corresponding microVP. all but tree-related variables */
	      instance->mvps[current_mvp]->nb_threads = 0;
	      instance->mvps[current_mvp]->next_nb_threads = 0;
	      instance->mvps[current_mvp]->rank = current_mvp;
	      instance->mvps[current_mvp]->enable = 1;
	      instance->mvps[current_mvp]->region_id = 0;
	      instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node(2*sizeof(int),i);
	      sctk_assert(instance->mvps[current_mvp]->tree_rank != NULL);
	      instance->mvps[current_mvp]->tree_rank[0] = n->rank;
	      instance->mvps[current_mvp]->tree_rank[1] = j;
	      instance->mvps[current_mvp]->root = root;
	      instance->mvps[current_mvp]->father = n;

	      n->children.leaf[j] = instance->mvps[current_mvp];

	      sctk_thread_attr_t __attr;
	      size_t stack_size;
	      int res;

	      sctk_thread_attr_init(&__attr);

	      sctk_thread_attr_setbinding(&__attr,target_vp);

	      if (sctk_is_in_fortran == 1)
	         stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	      else
	         stack_size = SCTK_ETHREAD_STACK_SIZE;

	      sctk_thread_attr_setstacksize(&__attr,stack_size);

	      switch (flag_level) {
	         case 0: /* Root */
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. Spinning on root", current_mvp, target_vp, i);
	           instance->mvps[current_mvp]->to_run = root;
	           break;

	         case 1: /* Numa */
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. Spinning on NUMA node %d", current_mvp, target_vp, i, i);
	           instance->mvps[current_mvp]->to_run = root->children.node[i];

	           res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
					mpcomp_slave_mvp_node,
					instance->mvps[current_mvp]);

	           sctk_assert(res == 0);
	           break;

	         case 2: /* MicroVP */
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. w/ internal spinning", current_mvp, target_vp, i);

	           res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
					mpcomp_slave_mvp_leaf,
					instance->mvps[current_mvp]);

	           sctk_assert(res == 0);
	           break;

	         default:
		   not_reachable();
	           break;
	      }
	      
	      sctk_thread_attr_destroy(&__attr);

	      current_mvp++;
	      flag_level = -1;
	    }
	  }
#endif

#if 1  /* Flat tree */
#warning "OpenMp compiling w/flat tree 8 cores"	    
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = 8;
	  root->child_type = CHILDREN_LEAF;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running = 0;
#ifdef ATOMICS
	  sctk_atomics_store_int (&root->barrier,0) ;
#else
	  root->barrier = 0;
#endif

	  root->barrier_done = 0;
	  root->children.node = (mpcomp_node_t **)sctk_malloc(root->nb_children*sizeof(mpcomp_node_t *));
	  sctk_assert(root->children.node != NULL);

	  for (i=0 ; i<root->nb_children ; i++) {
	    mpcomp_node_t *n;
	    int j;

            n = root;

            if (flag_level == -1) flag_level = 2;

            /* These are leaves ->create the microVP */
            int target_vp;

            target_vp = order[i];

            /* Allocate memory on the right NUMA node */
            instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(sizeof(mpcomp_mvp_t),target_vp/4);

            /* Get the set of registers */
            sctk_getcontext(&(instance->mvps[current_mvp]->vp_context));

            /* Initialize the corresponding microVP (all but the tree-related variables) */
            instance->mvps[current_mvp]->nb_threads = 0;
            instance->mvps[current_mvp]->next_nb_threads = 0;
            instance->mvps[current_mvp]->rank = current_mvp;
            instance->mvps[current_mvp]->enable = 1;
            instance->mvps[current_mvp]->region_id = 0;
            instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node(sizeof(int),target_vp/4);
            sctk_assert(instance->mvps[current_mvp]->tree_rank != NULL);
            instance->mvps[current_mvp]->tree_rank[0] = i;
            instance->mvps[current_mvp]->root = root;
            instance->mvps[current_mvp]->father = n;

            n->children.leaf[i] = instance->mvps[current_mvp];

            sctk_thread_attr_t __attr;
            size_t stack_size;
            int res;

            sctk_thread_attr_init(&__attr);

            sctk_thread_attr_setbinding(& __attr, target_vp);

            if (sctk_is_in_fortran == 1)
               stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
            else
               stack_size = SCTK_ETHREAD_STACK_SIZE;

            sctk_thread_attr_setstacksize(&__attr, stack_size);

	    switch (flag_level) {
	       case 0: /* Root */
	         sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. Spinning on root", current_mvp);
	         instance->mvps[current_mvp]->to_run = root;
	         break;

	       case 1: /* Numa */
	         sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. Spinning on NUMA node %d", current_mvp, target_vp, i);
	         instance->mvps[current_mvp]->to_run = root->children.node[i];

	         res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
	    			    mpcomp_slave_mvp_node,
	          		    instance->mvps[current_mvp]);

	         sctk_assert(res == 0);
	         break;

	       case 2: /* MicroVP */
	         sctk_nodebug("__mpcomp_instance_init: Creation microVP %d w/ internal spinning", current_mvp);

	         res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
	          		    mpcomp_slave_mvp_leaf,
	          		    instance->mvps[current_mvp]);

	         sctk_assert(res == 0);
	         break;

	       default:
		 not_reachable();
	         break;
	    }

  	    sctk_thread_attr_destroy(&__attr);

	    current_mvp++;
	    flag_level = -1;
          }	
#endif

	  break;

	case 32: 
#if 0  /* NUMA tree 32 cores */
#warning "OpenMp compiling w/ NUMA tree 32 cores"	    
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = 4;
	  root->min_index = 0;
	  root->max_index = 31;
	  root->child_type = CHILDREN_NODE;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running = 0;
#ifdef ATOMICS
	  sctk_atomics_store_int (&root->barrier,0) ;
#else
	  root->barrier = 0;
#endif
	  root->barrier_done = 0;
	  root->children.node = (mpcomp_node_t **)sctk_malloc(root->nb_children*sizeof(mpcomp_node_t *));
	  sctk_assert(root->children.node != NULL);

	  for (i=0 ; i<root->nb_children ; i++) {
	    mpcomp_node_t *n;
	    int j;

	    if (flag_level == -1) flag_level = 1;

	    n = (mpcomp_node_t *)sctk_malloc_on_node(sizeof(mpcomp_node_t),i);
	    sctk_assert(n != NULL);

#if defined (SCTK_USE_OPTIMIZED_TLS)
  sctk_tls_module = current_info->children[0]->tls_module;
  sctk_context_restore_tls_module_vp() ;
#endif
	    n->father = root;
	    n->rank = i;
	    n->depth = 1;
	    n->nb_children = 8;
	    n->min_index = i*8;
	    n->max_index = (i+1)*8;
	    n->child_type = CHILDREN_LEAF;
	    n->lock = SCTK_SPINLOCK_INITIALIZER;
	    n->slave_running = 0;
#ifdef ATOMICS
	    sctk_atomics_store_int (&n->barrier,0) ;
#else
	    n->barrier = 0;
#endif
	    n->barrier_done = 0;
	    n->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node(n->nb_children*sizeof(mpcomp_mvp_t *),i);

	    root->children.node[i] = n;

	    for (j=0 ; j<n->nb_children ; j++) {
	      /* These are leaves -> create the microVP */
	      int target_vp;

	      target_vp = order[8*i+j];

	      if (flag_level == -1) flag_level = 2;

	      /* Allocate memory on the right NUMA node */
	      instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(sizeof(mpcomp_mvp_t),i);

	      /* Get the set of registers */
	      sctk_getcontext(&(instance->mvps[current_mvp]->vp_context));

	      /* Initialize the corresponding microVP. all but tree-related variables */
	      instance->mvps[current_mvp]->nb_threads = 0;
	      instance->mvps[current_mvp]->next_nb_threads = 0;
	      instance->mvps[current_mvp]->rank = current_mvp;
	      instance->mvps[current_mvp]->enable = 1;
	      instance->mvps[current_mvp]->region_id = 0;
	      instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node(2*sizeof(int),i);
	      sctk_assert(instance->mvps[current_mvp]->tree_rank != NULL);
	      instance->mvps[current_mvp]->tree_rank[0] = n->rank;
	      instance->mvps[current_mvp]->tree_rank[1] = j;
	      instance->mvps[current_mvp]->root = root;
	      instance->mvps[current_mvp]->father = n;

	      n->children.leaf[j] = instance->mvps[current_mvp];

	      sctk_thread_attr_t __attr;
	      size_t stack_size;
	      int res;

	      sctk_thread_attr_init(&__attr);

	      sctk_thread_attr_setbinding(&__attr,target_vp);

	      if (sctk_is_in_fortran == 1)
	         stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	      else
	         stack_size = SCTK_ETHREAD_STACK_SIZE;

	      sctk_thread_attr_setstacksize(&__attr,stack_size);

	      switch (flag_level) {
	         case 0: /* Root */
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. Spinning on root", current_mvp, target_vp, i);
	           instance->mvps[current_mvp]->to_run = root;
	           break;

	         case 1: /* Numa */
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. Spinning on NUMA node %d", current_mvp, target_vp, i, i);
	           instance->mvps[current_mvp]->to_run = root->children.node[i];

	           res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
					mpcomp_slave_mvp_node,
					instance->mvps[current_mvp]);

	           sctk_assert(res == 0);
	           break;

	         case 2: /* MicroVP */
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. w/ internal spinning", current_mvp, target_vp, i);

	           res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
					mpcomp_slave_mvp_leaf,
					instance->mvps[current_mvp]);

	           sctk_assert(res == 0);
	           break;

	         default:
		   not_reachable();
	           break;
	      }

	      sctk_thread_attr_destroy(&__attr);

	      current_mvp++;
	      flag_level = -1;
	    }
	  }
#endif

#if 1   /* NUMA tree degree 4 */
#warning "OpenMp compiling w/2-level NUMA tree 32 cores"	    
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = 4;
	  root->min_index = 0;
	  root->max_index = 31;
	  root->child_type = CHILDREN_NODE;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running = 0;
#ifdef ATOMICS
	  sctk_atomics_store_int (&root->barrier,0) ;
#else
	  root->barrier = 0;
#endif
	  root->barrier_done = 0;
	  root->children.node = (mpcomp_node_t **)sctk_malloc(root->nb_children*sizeof(mpcomp_node_t *));
	  sctk_assert(root->children.node != NULL);

          /* Depth 1 -> output degree 2 */
	  for (i=0 ; i<root->nb_children ; i++) {
	    mpcomp_node_t *n;
	    int j;

	    if (flag_level == -1) flag_level = 1;

	    n = (mpcomp_node_t *)sctk_malloc_on_node(sizeof(mpcomp_node_t),i);
	    sctk_assert(n != NULL);

	    root->children.node[i] = n;

	    n->father = root;
	    n->rank = i;
	    n->depth = 1;
	    n->nb_children = 2;
	    n->min_index = i*8;
	    n->max_index = (i+1)*8-1;
	    n->child_type = CHILDREN_NODE;
	    n->lock = SCTK_SPINLOCK_INITIALIZER;
	    n->slave_running = 0;
#ifdef ATOMICS
	    sctk_atomics_store_int (&n->barrier,0) ;
#else
	    n->barrier = 0;
#endif
	    n->barrier_done = 0;
	    n->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node(n->nb_children*sizeof(mpcomp_mvp_t *),i);

            /* Depth 2 -> output degree 4 */
	    for (j=0 ; j<n->nb_children ; j++) {
               mpcomp_node_t *n2;
	       int k;

               if (flag_level == -1) flag_level = 2;

               n2 = (mpcomp_node_t *)sctk_malloc_on_node(sizeof(mpcomp_mvp_t),i);
               sctk_assert(n2 != NULL);

               n->children.node[j] = n2;
               
               n2->father = n;
               n2->rank = j;
               n2->depth = 2;
               n2->nb_children = 4;
               n2->min_index = i*8 + j*4;
               n2->max_index = i*8 + (j+1)*4 - 1;
               n2->child_type = CHILDREN_LEAF;
               n2->lock = SCTK_SPINLOCK_INITIALIZER;
               n2->slave_running = 0;
#ifdef ATOMICS
	       sctk_atomics_store_int (&n2->barrier,0) ;
#else
               n2->barrier = 0;
#endif
               n2->barrier_done = 0;
               n2->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node(n2->nb_children*sizeof(mpcomp_mvp_t *),i);

               /* Depth 3 -> leaves*/
               for (k=0 ; k<n2->nb_children ; k++) {
                  /* These are leaves -> create the microVP */
                  int target_vp;

                  target_vp = order[8*i+4*j+k];

                  if (flag_level == -1) flag_level = 3;

                  /* Allocate memory on the right NUMA node */
                  instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(sizeof(mpcomp_mvp_t),i);

                  /* Get the set of registers */
                  sctk_getcontext(&(instance->mvps[current_mvp]->vp_context));

                  /* Initialize the corresponding microVP (all but the tree-related variables) */
                  instance->mvps[current_mvp]->nb_threads = 0;
                  instance->mvps[current_mvp]->next_nb_threads = 0;
                  instance->mvps[current_mvp]->rank = current_mvp;
                  instance->mvps[current_mvp]->enable = 1;
                  instance->mvps[current_mvp]->region_id = 0;
                  instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node(3*sizeof(int),i);
                  sctk_assert(instance->mvps[current_mvp]->tree_rank != NULL);
                  instance->mvps[current_mvp]->tree_rank[0] = i;
                  instance->mvps[current_mvp]->tree_rank[1] = j;
                  instance->mvps[current_mvp]->tree_rank[2] = k;
                  instance->mvps[current_mvp]->root = root;
                  instance->mvps[current_mvp]->father = n2;

                  n2->children.leaf[k] = instance->mvps[current_mvp];

                  sctk_thread_attr_t __attr;
                  size_t stack_size;
                  int res;

                  sctk_thread_attr_init(&__attr);

                  sctk_thread_attr_setbinding(& __attr, target_vp);

                  if (sctk_is_in_fortran == 1)
                     stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
                  else
                     stack_size = SCTK_ETHREAD_STACK_SIZE;

                  sctk_thread_attr_setstacksize(&__attr, stack_size);

	          switch (flag_level) {
	             case 0: /* Root */
	               sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. Spinning on root (%d, %d, %d)", current_mvp, target_vp, i, i, j, k);
	               instance->mvps[current_mvp]->to_run = root;
	               break;

	             case 1: /* Numa */
	               sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. Spinning on NUMA node %d (%d, %d, %d)", current_mvp, target_vp, i, i, i, j, k);
	               instance->mvps[current_mvp]->to_run = root->children.node[i];

	               res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
		  			    mpcomp_slave_mvp_node,
					    instance->mvps[current_mvp]);

	               sctk_assert(res == 0);
	               break;

	             case 2: /* Numa */
	               sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. Spinning on NUMA node %d (level 2) (%d, %d, %d)", current_mvp, target_vp, i, i, i, j, k);
	               instance->mvps[current_mvp]->to_run = root->children.node[i]->children.node[j];

	               res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
		  			    mpcomp_slave_mvp_node,
					    instance->mvps[current_mvp]);

	               sctk_assert(res == 0);
	               break;

	             case 3: /* MicroVP */
	               sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d. w/ internal spinning (%d,%d,%d)", current_mvp, target_vp, i, i, j, k);

	               res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
					    mpcomp_slave_mvp_leaf,
					    instance->mvps[current_mvp]);

	               sctk_assert(res == 0);
	               break;

	             default:
		       not_reachable();
	               break;
	          }

  	          sctk_thread_attr_destroy(&__attr);

	          current_mvp++;
	          flag_level = -1;
               }
            }
           }
          
#endif

#if 0  /* Flat tree */	      /*  */
#warning "OpenMp compiling w/flat tree 32 cores"	    
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = 32;
	  root->child_type = CHILDREN_LEAF;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running = 0;
#ifdef ATOMICS
	    sctk_atomics_store_int (&root->barrier,0) ;
#else
	    root->barrier = 0;
#endif
	  root->barrier_done = 0;
	  root->children.node = (mpcomp_node_t **)sctk_malloc(root->nb_children*sizeof(mpcomp_node_t *));
	  sctk_assert(root->children.node != NULL);

	  for (i=0 ; i<root->nb_children ; i++) {
	    mpcomp_node_t *n;
	    int j;

            n = root;

            if (flag_level == -1) flag_level = 2;

            /* These are leaves ->create the microVP */
            int target_vp;

            target_vp = order[i];

            /* Allocate memory on the right NUMA node */
            instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(sizeof(mpcomp_mvp_t),target_vp/8);

            /* Get the set of registers */
            sctk_getcontext(&(instance->mvps[current_mvp]->vp_context));

            /* Initialize the corresponding microVP (all but the tree-related variables) */
            instance->mvps[current_mvp]->nb_threads = 0;
            instance->mvps[current_mvp]->next_nb_threads = 0;
            instance->mvps[current_mvp]->rank = current_mvp;
            instance->mvps[current_mvp]->enable = 1;
            instance->mvps[current_mvp]->region_id = 0;
            instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node(sizeof(int),target_vp/8);
            sctk_assert(instance->mvps[current_mvp]->tree_rank != NULL);
            instance->mvps[current_mvp]->tree_rank[0] = i;
            instance->mvps[current_mvp]->root = root;
            instance->mvps[current_mvp]->father = n;

            n->children.leaf[i] = instance->mvps[current_mvp];

            sctk_thread_attr_t __attr;
            size_t stack_size;
            int res;

            sctk_thread_attr_init(&__attr);

            sctk_thread_attr_setbinding(& __attr, target_vp);

            if (sctk_is_in_fortran == 1)
               stack_size = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
            else
               stack_size = SCTK_ETHREAD_STACK_SIZE;

            sctk_thread_attr_setstacksize(&__attr, stack_size);

	    switch (flag_level) {
	       case 0: /* Root */
	         sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. Spinning on root", current_mvp);
	         instance->mvps[current_mvp]->to_run = root;
	         break;

	       case 1: /* Numa */
	         sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. Spinning on NUMA node %d", current_mvp, target_vp, i);
	         instance->mvps[current_mvp]->to_run = root->children.node[i];

	         res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
	    			    mpcomp_slave_mvp_node,
	          		    instance->mvps[current_mvp]);

	         sctk_assert(res == 0);
	         break;

	       case 2: /* MicroVP */
	         sctk_nodebug("__mpcomp_instance_init: Creation microVP %d w/ internal spinning", current_mvp);

	         res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
	          		    mpcomp_slave_mvp_leaf,
	          		    instance->mvps[current_mvp]);

	         sctk_assert(res == 0);
	         break;

	       default:
		 not_reachable();
	         break;
	    }

  	    sctk_thread_attr_destroy(&__attr);

	    current_mvp++;
	    flag_level = -1;
          }	
#endif
	  break;

	default:
            //not_implemented();
      	    break;
  }
  
  sctk_free(order);
}

/* 
   Runtime OpenMP Initialization. 
   Can be called several times without any side effects.
   Called during the initialization of MPC 
*/
void __mpcomp_init (void)
{
  static volatile int done = 0;
  static sctk_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
  mpcomp_instance_t *instance;
  mpcomp_thread_t *t;
  mpcomp_thread_team_t *team;
  icv_t icvs;


  /* Need to initialize the current team */
  if (sctk_openmp_thread_tls == NULL) {

     sctk_spinlock_lock(&lock);

     /* Initialize the whole runtime (ie. environement variables) */
     if (done == 0) {
        __mpcomp_read_env_variables();
        done = 1;
     }

     /* Allocate an instance of OpenMP */
     instance = (mpcomp_instance_t *)sctk_malloc(sizeof(mpcomp_instance_t));
     sctk_assert(instance != NULL);

     /* Initialization of the OpenMP instance */
     __mpcomp_instance_init(instance,OMP_MICROVP_NUMBER);
     sctk_assert(instance != NULL);

     /* Allocate informations for the sequential region */
     t = (mpcomp_thread_t *)sctk_malloc(sizeof(mpcomp_thread_t));
     sctk_assert(t != NULL);

     /* Initialize default ICVs */
     icvs.nthreads_var = OMP_NUM_THREADS;
     icvs.dyn_var = OMP_DYNAMIC;
     icvs.nest_var = OMP_NESTED;
     icvs.run_sched_var = OMP_SCHEDULE;
     icvs.modifier_sched_var = OMP_MODIFIER_SCHEDULE;
     icvs.def_sched_var = omp_sched_static;
     icvs.nmicrovps_var = OMP_MICROVP_NUMBER;
				        op_list[i].func,
				        info->stack, STACK_SIZE,
				        info->extls, info->tls_module);
#else
#endif

     /* Initialize the OpenMP thread */
     __mpcomp_thread_init(t);
     t->icvs = icvs;
     t->children_instance = instance;

     /* Allocate informations for the thread team */
     team = (mpcomp_thread_team_t *)sctk_malloc(sizeof(mpcomp_thread_team_t));
     sctk_assert(team != NULL);
 
     __mpcomp_thread_team_init(team);
     team->icvs = icvs;
     team->children_instance = instance;

     t->team = team;

     /* Current thread information is 't' */
     sctk_openmp_thread_tls = t;

     //sctk_thread_mutex_unlock(&lock);
     sctk_spinlock_unlock(&lock);
  }

}


/*
   Parallel region creation
*/
void __mpcomp_start_parallel_region (int arg_num_threads, void *(*func) (void *), void *shared)
{
  mpcomp_thread_t *t;
  mpcomp_node_t *root;
  mpcomp_node_t *new_root;
  int num_threads;
  int num_threads_mvp;
  int i,j,k;

  __mpcomp_init();

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);      
  sctk_assert(t->team != NULL);

  /* Compute the number of threads for this parallel region */
  num_threads = t->icvs.nthreads_var;
  if (arg_num_threads > 0)
    num_threads = arg_num_threads;

  /* Bypass if the parallel region contains only 1 thread */
  if (num_threads == 1) {
    /* Simulate a parallel region by incrementing the depth */
    t->team->depth++ ;

    func(shared);

    t->team->depth-- ;

    return;
  }

  sctk_nodebug("__mpcomp_start_parallel_region: depth %d",t->team->depth);
 
  /* First level of parallel region. No nesting */  
  if (t->team->depth == 0) {

    mpcomp_instance_t *instance;

    /* Grab the OpenMP instance already allocated during the initialization */
    instance = t->children_instance;
    sctk_assert(instance != NULL);

    /* Get the root node of the main tree */
    root = instance->root;
    sctk_assert(root != NULL);

    /* Root is awake -> propagate the values to children */
    mpcomp_node_t *n;
    int num_threads_per_child;
    int max_index_with_more_threads;

    max_index_with_more_threads = num_threads % instance->nb_mvps;

    new_root = root;
    n = root;

    /* TODO should be done differently to manage the fact the number 
       of threads can change during the execution of the program */
#if 0
    if (num_threads <= 8) {
      n = n->children.node[0];
      n->father = NULL;
      new_root = n;
    }
#endif

    while (n->child_type != CHILDREN_LEAF) {
       int nb_children_involved = 1;

       for (i=1 ; i<n->nb_children ; i++) {

    	 num_threads_per_child = num_threads/instance->nb_mvps;

         /* Compute the number of threads for the smallest child of the subtree 'n' */
         if (n->children.node[i]->min_index < max_index_with_more_threads)
           num_threads_per_child++;

         if (num_threads_per_child > 0) {
           n->children.node[i]->func = func;
           n->children.node[i]->shared = shared;
           n->children.node[i]->team = t->team;
           n->children.node[i]->num_threads = num_threads;
           n->children.node[i]->slave_running = 1;
           nb_children_involved++;
         }
       }

       n->barrier_num_threads = nb_children_involved;
       n = n->children.node[0];
    }

    n->func = func;
    n->shared = shared;
    n->num_threads = num_threads;
    n->team = t->team;

    /* Wake up children leaf */
    sctk_assert(n->child_type == CHILDREN_LEAF);
    int nb_leaves_involved = 1;

    for (i=1 ; i<n->nb_children ; i++) {
      num_threads_per_child = num_threads / instance->nb_mvps;

      if (n->children.leaf[i]->rank < max_index_with_more_threads) 
	num_threads_per_child++;

      if (num_threads_per_child > 0) {
        n->children.leaf[i]->slave_running = 1;
        nb_leaves_involved++;
      }
    }

    n->barrier_num_threads = nb_leaves_involved;

    num_threads_mvp = 1;

    instance->mvps[0]->func = func;
    instance->mvps[0]->shared = shared;
    instance->mvps[0]->nb_threads = 1;

    for (i=0 ; i<num_threads_mvp ; i++) {
      instance->mvps[0]->threads[i].rank = 0;
      instance->mvps[0]->threads[i].mvp = instance->mvps[0];
      instance->mvps[0]->threads[i].index_in_mvp = i;
      instance->mvps[0]->threads[i].done = 0;
      instance->mvps[0]->threads[i].team = t->team;
      instance->mvps[0]->threads[i].hierarchical_tls = NULL;
      instance->mvps[0]->threads[i].current_single = -1;
      instance->mvps[0]->threads[i].private_current_for_dyn = 0;

      instance->mvps[0]->threads[i].is_running = 1;
    }

    /* Start scheduling */
    in_order_scheduler(instance->mvps[0]);

    /* Barrier to wait for the other microVPs */
    __mpcomp_half_barrier();

    /* Finish the half barrier by spinning ton the root value */
#ifdef ATOMICS
    while (new_root->barrier_num_threads != sctk_atomics_load_int(&new_root->barrier)){
       sctk_thread_yield();
    }
    sctk_atomics_store_int (&new_root->barrier,0) ;
#else
    while (new_root->barrier_num_threads != new_root->barrier){
       sctk_thread_yield();
    }
    new_root->barrier = 0;
#endif

    /* Restore the previous OpenMP info */
    sctk_openmp_thread_tls = t;

#if 0
    /* Check if the barrier info is correct at the end of the parallel region */
    check_parallel_region_correctness();
#endif
   
  }
  else {
    not_implemented();
  }
}

/*
   Half barrier
*/
void __mpcomp_internal_half_barrier(mpcomp_mvp_t *mvp)
{
   mpcomp_node_t *c;
   int b_done;
   int b;

   /*Barrier to wait for the other microVPs*/
   c = mvp->father;

#ifdef ATOMICS
   b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
   //sctk_atomics_write_barrier(); 

   while ((b+1 == c->barrier_num_threads) && (c->father != NULL)) {
      sctk_atomics_store_int (&c->barrier,0) ;
      //sctk_atomics_write_barrier(); 

      c = c->father;

      b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
      sctk_atomics_write_barrier(); 
   }
#else

   sctk_spinlock_lock(&(c->lock));
   b = c->barrier;
   b++;
   c->barrier = b;
   sctk_spinlock_unlock(&(c->lock));

   while ((b == c->barrier_num_threads) && (c->father != NULL)) {
      c->barrier = 0;
      c = c->father;

      sctk_spinlock_lock(&(c->lock));
      b = c->barrier;
      b++;
      c->barrier = b;
      sctk_spinlock_unlock(&(c->lock));
   }
#endif
}

/*
   OpenMP implicit barrier - half barrier)
*/
void __mpcomp_half_barrier()
{
  mpcomp_thread_t *t;
  mpcomp_mvp_t *mvp;

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  mvp = t->mvp;
  sctk_assert(mvp != NULL);

  sctk_nodebug("__mpcomp_half_barrier: Entering mvp %d",mvp->rank);

  __mpcomp_internal_half_barrier(mvp);

  t = sctk_openmp_thread_tls;

  sctk_nodebug("__mpcomp_half_barrier: Leaving mvp %d",mvp->rank);

}


/*
   Full barrier
*/
void __mpcomp_internal_barrier(mpcomp_mvp_t *mvp)
{
   mpcomp_node_t *c;
   int b_done;
   int b;

   /*Barrier to wait for the other microVPs*/
   c = mvp->father;

   /* Step 1: Climb in the tree */
   b_done = c->barrier_done;

#ifdef ATOMICS
   b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
   //sctk_atomics_write_barrier(); 

   while ((b+1 == c->barrier_num_threads) && (c->father != NULL)) {
      sctk_atomics_store_int (&c->barrier,0) ;

      c = c->father;

      b_done = c->barrier_done;      

      b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
      //sctk_atomics_write_barrier(); 
   }

   /* Step 2: Wait for the barrier to be done */
   if ((c->father != NULL) || (c->father == NULL && b+1 != c->barrier_num_threads)) {
     /* Wait for c->barrier == c->barrier_num_threads */ 
     while (b_done == c->barrier_done) {
        sctk_thread_yield();
     }

   }
   else {
     sctk_atomics_store_int (&c->barrier,0);
     c->barrier_done++;
     /* TODO: not sure that we need that. If we do need it, maybe we need to lock */
   }

#else
   sctk_spinlock_lock(&(c->lock));
   b = c->barrier;
   b++;
   c->barrier = b;
   sctk_spinlock_unlock(&(c->lock));

   while ((b == c->barrier_num_threads) && (c->father != NULL)) {
      c->barrier = 0;

      c = c->father;

      b_done = c->barrier_done;      

      sctk_spinlock_lock(&(c->lock));
      b = c->barrier;
      b++;
      c->barrier = b;
      sctk_spinlock_unlock(&(c->lock));
   }

   /* Step 2: Wait for the barrier to be done */
   if ((c->father != NULL) || (c->father == NULL && b != c->barrier_num_threads)) {

     /* Wait for c->barrier == c->barrier_num_threads */ 
     while (b_done == c->barrier_done) {
        sctk_thread_yield();
     }
   }
   else {
     c->barrier = 0;
     c->barrier_done++;
     /* TODO: not sure that we need that. If we do need it, maybe we need to lock */
   }
#endif


   /* Step 3: Go down in the tree to wake up the children */
   while (c->child_type != CHILDREN_LEAF) {
       c = c->children.node[mvp->tree_rank[c->depth]];
       c->barrier_done++;
   }
}

/*
   OpenMP explicit barrier
*/
void __mpcomp_barrier(void)
{
  mpcomp_thread_t *t;
  mpcomp_mvp_t *mvp;
  int num_threads;

  sctk_nodebug("__mpcomp_barrier: starting ...");

  __mpcomp_init();

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  if (t->team->num_threads > 1) {
    /* Grab the corresponding microVP */
    mvp = t->mvp;
    sctk_assert(mvp != NULL);

    __mpcomp_internal_barrier(mvp);

  }
}

/*
   Run the parallel code function
*/
void in_order_scheduler (mpcomp_mvp_t *mvp)
{
  int i;

  sctk_nodebug("in_order_scheduler: Starting to schedule %d thread(s) with rank %d", mvp->nb_threads, mvp->rank);

  /* TODO: handle out of order */
  for (i=0 ; i<mvp->nb_threads ; i++) {
    sctk_openmp_thread_tls = &mvp->threads[i];

    mvp->func(mvp->shared);

    mvp->threads[i].done = 1;
  }
  
INFO("__mpcomp_flush: need to call mpcomp_macro_scheduler")
}

/*
   Grab the total number of threads
*/
int mpcomp_get_num_threads ()
{
  mpcomp_thread_t *t;

  sctk_nodebug("mpcomp_get_num_threads: entering...");

  t = sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  return t->team->num_threads;
}

/*
   Grab the rank of the current thread
*/
int mpcomp_get_thread_num ()
{
  mpcomp_thread_t *t;

  sctk_nodebug("mpcomp_get_thread_num: entering...");

  t = sctk_openmp_thread_tls;
  sctk_assert(t != NULL );

  return t->rank;
}



/*
 * Return the maximum number of threads that can be used inside a parallel region.
 * This function may be called either from serial or parallel parts of the program.
 * See OpenMP API 2.5 Section 3.2.3
 */
int
mpcomp_get_max_threads (void)
{
  mpcomp_thread_t *t;

  __mpcomp_init ();

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  return t->team->num_threads;
}


/* timing routines */
double
mpcomp_get_wtime (void)
{
  double res;
  struct timeval tp;

  gettimeofday (&tp, NULL);
  res = tp.tv_sec + tp.tv_usec * 0.000001;

  sctk_nodebug ("Wtime = %f", res);
  return res;
}

double
mpcomp_get_wtick (void)
{
  return 10e-6;
}

/* 
  Check if the barrier info is correct at the end of the parallel region 
*/
void check_parallel_region_correctness(void)
{

  mpcomp_thread_t *t;
  mpcomp_node_t *root;  
  mpcomp_node_t *n;
  mpcomp_node_t *n1;
  int i;

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  mpcomp_instance_t *instance;

  /* Grab the OpenMP instance already allocated during the initialization */
  instance = t->children_instance;
  sctk_assert(instance != NULL);

  /* Get the root node of the main tree */
  root = instance->root;
  sctk_assert(root != NULL);

  n = root;

  sctk_nodebug("root barrier %d",sctk_atomics_load_int(&n->barrier));
  sctk_nodebug("root barrier done %d",n->barrier_done);

  sctk_assert(sctk_atomics_load_int(&n->barrier) == 0);

  for (i=0 ; i<n->nb_children ; i++) {
    n1 = n->children.node[i];
    sctk_nodebug("numa node %d",i);
    sctk_nodebug("numa barrier %d",sctk_atomics_load_int(&n1->barrier));
    sctk_nodebug("numa barrier done %d",n1->barrier_done);

    sctk_assert(sctk_atomics_load_int(&n1->barrier) == 0);
  }

}

/*
  Push tree node in stack
*/
void push(mpcomp_node_t *node, mpcomp_stack_t *head)
{
  
  mpcomp_stack_t *s = malloc(sizeof(mpcomp_stack_t));

  if(s == NULL) {
    printf("Error: no space available for stack!\n");
    return;
  } else {
    s->node = node;
    s->next = head;
    head = s;
    head->size++;
  } 
}


/*
 Pop last element of stack
*/
mpcomp_node_t *pop(mpcomp_stack_t *head)
{
 
 if(head == NULL)
 {
   printf("Error: stack underflow!!\n");
   return NULL;
 }
 else {
   mpcomp_stack_t *top = head;
   mpcomp_node_t *node;
   node = top->node;
   head = top->next;
   head->size--; 
   free(top);
   return node;
 }

}

/*
 Initialize stack
*/
void mk_empty_stack(mpcomp_stack_t *head)
{
 head->size = 0; 
}

/*
  Check if stack is empty
*/
int is_empty_stack(mpcomp_stack_t *head)
{
  if(head->size == 0)
    return 1;
  
  return 0;
}


/*
 Prefix in depth tree search
*/
void in_depth_tree_search(mpcomp_node_t *node, int *index)
{

  int i;
  mpcomp_stack_t *stack;

  mk_empty_stack(stack);
  push(node, stack);

  while(!is_empty_stack(stack)) 
  {
    node = pop(stack);
    
    if(node->nb_children != 0)
    {
       for(i=0;i<node->nb_children;i++) {
         if(node->child_type == CHILDREN_NODE)
          push(node->children.node[i], stack); 
       }     
    }  
  }
}


