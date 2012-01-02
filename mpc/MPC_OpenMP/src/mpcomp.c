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

/*******************
       MACROS 
********************/
#define ATOMICS 1

#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 0
#define OK 1
#define STOP 2
#define STOPPED 3

/* Maximum number of threads for each team of a parallel region */
#define MPCOMP_MAX_THREADS 64
/* Maximum number of shared for loops w/ dynamic schedule alive */
#define MPCOMP_MAX_ALIVE_FOR_DYN 7

__thread void *sctk_openmp_thread_tls = NULL;

/* Schedule type */
static omp_sched_t OMP_SCHEDULE = 1;
/* Schedule modifier */
static int OMP_MODIFIER_SCHEDULE = -1;
/* Defaults number of threads */
static int OMP_NUM_THREADS = 0;
/* Is dynamic adaptation on? */
static int OMP_DYNAMIC = 0;
/* Is nested parallelism handled or flaterned? */
static int OMP_NESTED = 0;
/* Number of VPs for each OpenMP team */
static int OMP_MICROVP_NUMBER = 0;


/*********************
       STRUCTURES
**********************/

/******** ICV ********/
struct icv_s {
  int nthreads_var;		/* Number of threads for the next team creation */
  int dyn_var;			/* Is dynamic thread adjustement on? */
  int nest_var;			/* Is nested OpenMP handled? */
  omp_sched_t run_sched_var;	/* Schedule to use when 'schedule' clause is set to 'runtime' */
  int modifier_sched_var;	/* Size of chunks for loop schedule */
  omp_sched_t def_sched_var;	/* Default schedule when no 'schedule' clause is present */
  int nmicrovps_var;		/* Number of VPs */
};
 
typedef struct icv_s icv_t;

/******* CHUNK ********/ 
struct mpcomp_chunk_s {
   int remain ;
   int total ;
   char pad[7] ;
};

typedef struct mpcomp_chunk_s mpcomp_chunk_t ;

/******** OPENMP THREAD *********/
struct mpcomp_thread_s {
  sctk_mctx_t uc;				/* Context: register initialization, etc. Initialized when another thread is blocked */
  char *stack;					/* Stack: initialized when another thread is blocked */
  int depth;					/* Depth of the current thread (0 =sequential region) */
  icv_t icvs;					/* Set of the ICVs for the child team */
  int rank;					/* OpenMP rank. 0 for master thread per team */
  int num_threads;				/* Current number of threads in the team */
  struct mpcomp_mvp_s *mvp;			/* Openmp microvp */
  int index_in_mvp;	
  volatile int done;
  struct mpcomp_instance_s *children_instance; 	/* Instance for nested parallelism */
  struct mpcomp_thread_s *father; 		/* TODO: check if this is useful */
  void *hierarchical_tls;			/* Local variables */
 
  /* DYNAMIC SCHEDULING */
  sctk_spinlock_t lock_for_dyn[MPCOMP_MAX_THREADS][MPCOMP_MAX_ALIVE_FOR_DYN];	/* Lock for dynamic scheduling. A lock for each loop */
  sctk_spinlock_t lock_stop_for_dyn;		
  sctk_spinlock_t lock_exited_for_dyn[MPCOMP_MAX_ALIVE_FOR_DYN];		/* Lock for dynamic scheduling. A lock for each loop */

  volatile int stop[MPCOMP_MAX_ALIVE_FOR_DYN];				
  volatile int nthread_exited_for_dyn[MPCOMP_MAX_ALIVE_FOR_DYN];
  volatile mpcomp_chunk_t chunk_info_for_dyn[MPCOMP_MAX_THREADS][MPCOMP_MAX_ALIVE_FOR_DYN];

  int private_current_for_dyn;
};

typedef struct mpcomp_thread_s mpcomp_thread_t;

/******* INSTANCE OF OPENMP ********/
struct mpcomp_instance_s {
  int nb_mvps;
  struct mpcomp_mvp_s **mvps;
  struct mpcomp_node_s *root;
};

typedef struct mpcomp_instance_s mpcomp_instance_t;

/********* OPENMP MICRO VP **********/
struct mpcomp_mvp_s {
  sctk_mctx_t vp_context;			/* Context including registers, stack pointer, etc. */
  sctk_thread_t pid;				/* Thread ID */
  int nb_threads;
  int next_nb_threads;				/* TODO: maybe we don't need it. Probably used for parallel nesting */
  struct mpcomp_thread_s threads[128];
  int *tree_rank;				/* Array of rank in every node of the tree */
  int rank;					/* Rank within the microVPs */
  char pad0[64];
  volatile int enable;
  char pad1[64];
  struct mpcomp_node_s *root;
  struct mpcomp_node_s *father;
  int region_id;				/* TODO: 0 or 1. To be checked */
  struct mpcomp_node_s *to_run;			/* Specify where is the spin waiting for a new parallel region */
  char pad2[64];
  volatile int slave_running[2];
  char pad3[64];
  void *(*func) (void*);			/* Function to call by every thread */
  void *shared;					/* Shared variables for every thread */
};

typedef struct mpcomp_mvp_s mpcomp_mvp_t;

typedef enum children_s {
  CHILDREN_NODE = 1,
  CHILDREN_LEAF = 2,
}children_t;

typedef enum context_t {
  MPCOMP_CONTEXT_IN_ORDER = 1,
  MPCOMP_CONTEXT_OUT_OF_ORDER_MAIN = 2,
  MPCOMP_CONTEXT_OUT_OF_ORDER_SUB = 3,
} context_t;


/******** OPENMP INFO NODE ********/
struct mpcomp_node_s {
  struct mpcomp_node_s *father;
  int rank;					/* Rank among children of my father */
  int depth;					/* Depth in the tree */
  int nb_children;				/* Number of children */
  int min_index;				/* Flat min index of leaves in this subtree */
  int max_index;				/* Flat max index of leaves in this subtree */
  children_t child_type;			/* Am I a node or a leaf? */ 
  union node_or_leaf {
    struct mpcomp_node_s **node;
    struct mpcomp_mvp_s **leaf;
  } children;
  sctk_spinlock_t lock;				/* Lock for structure update */
  char pad0[64];
  volatile int slave_running[2];
  char pad1[64];
#ifdef ATOMICS
  sctk_atomics_int barrier;				/* Barrier for the child team */
#else
  volatile int barrier;				/* Barrier for the child team */
#endif
  char pad2[64];
  volatile int barrier_done;			/* Is the barrier for the child team is over? */
  char pad3[64];
  /* TODO: check if it should be volatile or not */
  int barrier_num_threads;			/* Number of threads involved in the barrier */
  char pad4[64];
  int num_threads;				/* Number of threads in the current team */ 
  void *(*func) (void*);
  void *shared;
};

typedef struct mpcomp_node_s mpcomp_node_t;


/************************************
       OMP PARALLEL FUNCTIONS 
*************************************/
void __mpcomp_instance_init(mpcomp_instance_t *instance, int nb_mvps);
void __mpcomp_thread_init(mpcomp_thread_t *t);
void __mpcomp_internal_barrier(mpcomp_mvp_t *mvp);
void __mpcomp_internal_half_barrier(mpcomp_mvp_t *mvp);
void __mpcomp_barrier(void);
void *mpcomp_slave_mvp_node(void *arg);
void *mpcomp_slave_mvp_leaf(void *arg);
void in_order_scheduler(mpcomp_mvp_t *mvp);

/******************************************
       DYNAMIC SCHEDULING FUNCTIONS 
*******************************************/
void __mpcomp_barrier_for_dyn(void);
int __mpcomp_dynamic_loop_begin(int lb, int b, int incr, int chunk_size, int *from, int *to);
int __mpcomp_dynamic_loop_next(int *from, int *to);
void __mpcomp_dynamic_loop_end_nowait();


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

   sctk_nodebug("mpcomp_slave_mvp_node: Polling address %p -> father %p",
		&(mvp->slave_running[0]), mvp->father);

   while (mvp->enable) {
     int i;
     int num_threads_mvp;
     int num_threads_per_child;
     int max_index_with_more_threads;

     sctk_nodebug("mpcomp_slave_mvp_node: Polling address %p ith ID %d",
		&(n->slave_running[0]),0); 

     /* Spinning on the designed node */
     sctk_thread_wait_for_value_and_poll((int *)&(n->slave_running[0]), 1 , NULL , NULL);
     n->slave_running[0] = 0;

     sctk_nodebug("mpcomp_slave_mvp_node: wake_up -> go to children using function %p", n->func);

     /* Used in case of nb_threads < nb_mvps */
     max_index_with_more_threads = n->num_threads%OMP_MICROVP_NUMBER;

     /* Wake up children node */
     while (n->child_type != CHILDREN_LEAF) {
        int nb_children_involved = 1;

        sctk_nodebug("mpcomp_slave_mvp_node: children node passing function %p", n->func);

        n->children.node[0]->func = n->func;
        n->children.node[0]->shared = n->shared;
        n->children.node[0]->num_threads = n->num_threads;

        for (i=1 ; i<n->nb_children ; i++) {

          num_threads_per_child = n->num_threads / OMP_MICROVP_NUMBER; 

	  if (n->children.node[i]->min_index < max_index_with_more_threads) 
	    num_threads_per_child++;

          if (num_threads_per_child > 0) {
  	    n->children.node[i]->func = n->func;
	    n->children.node[i]->shared = n->shared;
	    n->children.node[i]->num_threads = n->num_threads;
	    n->children.node[i]->slave_running[0] = 1;
            nb_children_involved++;
          }
	}

        n->barrier_num_threads = nb_children_involved;
        n = n->children.node[0];
     }

     /* Wake up children leaf */
     sctk_assert(n->child_type == CHILDREN_LEAF);
     sctk_nodebug("mpcomp_slave_mvp_node: children leaf will use %p",n->func);

     int nb_leaves_involved = 1;

     for (i=1 ; i<n->nb_children ; i++) {

       num_threads_per_child = n->num_threads / OMP_MICROVP_NUMBER; 

       if (n->children.leaf[i]->rank < max_index_with_more_threads)
          num_threads_per_child++;

       if (num_threads_per_child > 0) {
         n->children.leaf[i]->slave_running[0] = 1;
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

       /* Copy information */
       mvp->threads[i].num_threads = n->num_threads;
       
       sctk_nodebug("mpcomp_slave_mvp: Got num threads %d",
		mvp->threads[i].num_threads);
     }
     /* TODO finish*/

     /* Update the total number of threads for this microVP */
     mvp->nb_threads = num_threads_mvp;

     /* Run */
     in_order_scheduler(mvp);

     /* Barrier to wait for the other microVPs */
     __mpcomp_internal_half_barrier(mvp);
#if defined (SCTK_USE_OPTIMIZED_TLS)
	  sctk_tls_module = info->tls_module;
	  sctk_context_restore_tls_module_vp() ;
#endif

     sctk_nodebug("node val %d out of %d", b, c->barrier_num_threads);

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

   sctk_nodebug("mpcomp_slave_mvp_leaf: Polling address %p -> father is %p",
		&(mvp->slave_running[0]),mvp->father);
   sctk_nodebug("mpcomp_slave_mvp_leaf: Will get value from father %p", mvp->father);
 
   /* Spin while this microVP is alive */  
   while (mvp->enable) {
     int i;
     int num_threads_mvp;

     sctk_nodebug("mpcomp_slave_mvp_leaf: Polling address %p with ID %d",
			&(mvp->slave_running[0]),0);

     /* Spin for new parallel region */
     sctk_thread_wait_for_value_and_poll((int*)&(mvp->slave_running[0]),1,NULL,NULL);
     mvp->slave_running[0] = 0;

     sctk_nodebug("mpcomp_slave_mvp_leaf: wake up");

     mvp->func = mvp->father->func;
     mvp->shared = mvp->father->shared;

     sctk_nodebug("mpcomp_slave_mvp_leaf: Function for this mvp %p, #threads %d (father %p)",
		mvp->func, mvp->father->num_threads, mvp->father);

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

       /* Copy information */
       mvp->threads[i].num_threads = mvp->father->num_threads;
       
       sctk_nodebug("mpcomp_slave_mvp: Got num threads %d",
		mvp->threads[i].num_threads);
     }
     /* TODO finish*/

     /* Update the total number of threads for this microVP */
     mvp->nb_threads = num_threads_mvp;

     /* Run */
     in_order_scheduler(mvp);

     /* Barrier to wait for the other microVPs */
     __mpcomp_internal_half_barrier(mvp);

   }

   return NULL;
}

/*
   Initialize an OpenMP thread
*/
void __mpcomp_thread_init (mpcomp_thread_t *t)
{
   int i,j;

   t->depth = 0;
   t->rank = 0;
   t->num_threads = 1;
   t->mvp = NULL;
   t->index_in_mvp = 0;
   t->done = 0;
   t->hierarchical_tls = NULL;

   for (i=0 ; i<MPCOMP_MAX_THREADS ; i++) {
     for (j=0 ; j<MPCOMP_MAX_ALIVE_FOR_DYN ; j++) {
       t->lock_for_dyn[i][j] = SCTK_SPINLOCK_INITIALIZER;
       t->chunk_info_for_dyn[i][j].remain = -1 ;
     }
   }

   t->lock_stop_for_dyn = SCTK_SPINLOCK_INITIALIZER;

   for (i=0 ; i<MPCOMP_MAX_ALIVE_FOR_DYN ; i++) {
      t->lock_exited_for_dyn[i] = SCTK_SPINLOCK_INITIALIZER;
      t->nthread_exited_for_dyn[i] = 0;
   }

   for (i=0 ; i<MPCOMP_MAX_ALIVE_FOR_DYN-1 ; i++) t->stop[i] = OK;				
   t->stop[MPCOMP_MAX_ALIVE_FOR_DYN-1] = STOP;

   t->private_current_for_dyn = 0;

   t->father = t;
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
	  root->slave_running[0] = 0;
	  root->slave_running[1] = 0;
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
	    n->slave_running[0] = 0;
	    n->slave_running[1] = 0;
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
	  root->slave_running[0] = 0;
	  root->slave_running[1] = 0;
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
            instance->mvps[current_mvp] = (mpcomp_mvp_t *)sctk_malloc_on_node(sizeof(mpcomp_mvp_t),target_vp);

            /* Get the set of registers */
            sctk_getcontext(&(instance->mvps[current_mvp]->vp_context));

            /* Initialize the corresponding microVP (all but the tree-related variables) */
            instance->mvps[current_mvp]->nb_threads = 0;
            instance->mvps[current_mvp]->next_nb_threads = 0;
            instance->mvps[current_mvp]->rank = current_mvp;
            instance->mvps[current_mvp]->enable = 1;
            instance->mvps[current_mvp]->region_id = 0;
            instance->mvps[current_mvp]->tree_rank = (int *)sctk_malloc_on_node(sizeof(int),target_vp);
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
#if 0 /* NUMA tree 32 cores */
#warning "OpenMp compiling w/ NUMA tree 32 cores"	    
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = 4;
	  root->min_index = 0;
	  root->max_index = 31;
	  root->child_type = CHILDREN_NODE;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running[0] = 0;
	  root->slave_running[1] = 0;
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
	    n->slave_running[0] = 0;
	    n->slave_running[1] = 0;
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

#if 0  /* NUMA tree degree 4 */
#warning "OpenMp compiling w/2-level NUMA tree 32 cores"	    
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = 4;
	  root->min_index = 0;
	  root->max_index = 31;
	  root->child_type = CHILDREN_NODE;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running[0] = 0;
	  root->slave_running[1] = 0;
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
	    n->slave_running[0] = 0;
	    n->slave_running[1] = 0;
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

               n2 = (mpcomp_node_t *)sctk_malloc_on_node(sizeof(mpcomp_mvp_t *),i);
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
               n2->slave_running[0] = 0;
               n2->slave_running[1] = 0;
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

#if 1  /* Flat tree */	      /*  */
#warning "OpenMp compiling w/flat tree 32 cores"	    
	  root->father = NULL;
	  root->rank = -1;
	  root->depth = 0;
	  root->nb_children = 32;
	  root->child_type = CHILDREN_LEAF;
	  root->lock = SCTK_SPINLOCK_INITIALIZER;
	  root->slave_running[0] = 0;
	  root->slave_running[1] = 0;
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
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;

  sctk_nodebug("__mpcomp_init: sctk_openmp_thread_tls");

  /* Need to initialize the current team */
  if (sctk_openmp_thread_tls == NULL) {
     mpcomp_instance_t *instance;
     mpcomp_thread_t *t;
     icv_t icvs;

     sctk_thread_mutex_lock(&lock);

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

     /* Current thread information is 't' */
     sctk_openmp_thread_tls = t;

     sctk_thread_mutex_unlock(&lock);

     sctk_nodebug("__mpcomp_init: Init done...");
  }
}


/*
   Parallel region creation
*/
void __mpcomp_start_parallel_region (int arg_num_threads, void *(*func) (void *), void *shared)
{
  mpcomp_thread_t *t;
  mpcomp_node_t *root;
  int num_threads;
  int i;

  __mpcomp_init();

  sctk_nodebug("__mpcomp_start_parallel_region: **** NEW PARALLE REGION (f %p) (arg %p) ****",func,shared);

  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Compute the number of threads for this parallel region */
  num_threads = t->icvs.nthreads_var;
  if (arg_num_threads > 0)
    num_threads = arg_num_threads;

  sctk_nodebug("__mpcomp_start_parallel_region: Final number of threads %d (default %d)",num_threads,t->icvs.nthreads_var);

  /* Bypass if the parallel region contains only 1 thread */
  if (num_threads == 1) {
    fprintf(stderr,"Parallel Region with 1 Thread. Not Implemented.");
    return;
  }

  /* First level of parallel region. No nesting */  
  if (t->depth == 0) {
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

    sctk_nodebug("__mpcomp_start_parallel_region: #threads_per_mvp %d. %d mvps have an additional thread",num_threads_per_child,max_index_with_more_threads);

    n = root;

    while (n->child_type != CHILDREN_LEAF) {
       int nb_children_involved = 1;

       for (i=1 ; i<n->nb_children ; i++) {

         sctk_nodebug("__mpcomp_start_parallel_region: Testing child %d with min_index = %d, #threads = %d",i,n->children.node[i]->min_index,num_threads);

    	 num_threads_per_child = num_threads/instance->nb_mvps;

         /* Compute the number of threads for the smallest child of the subtree 'n' */
         if (n->children.node[i]->min_index < max_index_with_more_threads)
           num_threads_per_child++;

         if (num_threads_per_child > 0) {
           n->children.node[i]->func = func;
           n->children.node[i]->shared = shared;
           n->children.node[i]->num_threads = num_threads;
           n->children.node[i]->slave_running[0] = 1;
           nb_children_involved++;
         }
       }

       sctk_nodebug("__mpcomp_start_parallel_region: nb_children_invloved = %d",nb_children_involved);
       n->barrier_num_threads = nb_children_involved;
       n = n->children.node[0];
    }

    n->func = func;
    n->shared = shared;
    n->num_threads = num_threads;

    /* Wake up children leaf */
    sctk_assert(n->child_type == CHILDREN_LEAF);
    int nb_leaves_involved = 1;


    for (i=1 ; i<n->nb_children ; i++) {
      num_threads_per_child = num_threads / instance->nb_mvps;

      if (n->children.leaf[i]->rank < max_index_with_more_threads) 
	num_threads_per_child++;

      if (num_threads_per_child > 0) {
        n->children.leaf[i]->slave_running[0] = 1;
        nb_leaves_involved++;
      }
    }

    n->barrier_num_threads = nb_leaves_involved;

    instance->mvps[0]->func = func;
    instance->mvps[0]->shared = shared;
    instance->mvps[0]->nb_threads = 1;

    /* Compute the number of OpenMP threads and their rank running on microVP #0 */
    instance->mvps[0]->threads[0].num_threads = num_threads;
    instance->mvps[0]->threads[0].rank = 0;
    instance->mvps[0]->threads[0].mvp = instance->mvps[0];

    /* Start scheduling */
    in_order_scheduler(instance->mvps[0]);

    /* Barrier to wait for the other microVPs */
    __mpcomp_internal_half_barrier(instance->mvps[0]);

    /* Finish the half barrier by spinning ton the root value */
#ifdef ATOMICS
    while (root->barrier_num_threads != sctk_atomics_load_int(&root->barrier)){
       sctk_thread_yield();
    }
    sctk_atomics_store_int (&root->barrier,0) ;
#else
    while (root->barrier_num_threads != root->barrier){
       sctk_thread_yield();
    }
    root->barrier = 0;
#endif

    /* Restore the previous OpenMP info */
    sctk_openmp_thread_tls = t;
   
  }
  else {
    not_implemented();
  }
}

/*
   Implicit barrier = half barrier = no step 3
*/
void __mpcomp_internal_half_barrier(mpcomp_mvp_t *mvp)
{

     sctk_nodebug("__mpcomp_internal_barrier: mvp rank %d",mvp->rank);

     /*Barrier to wait for the other microVPs*/
     mpcomp_node_t *c;
     c = mvp->father;

     /* Step 1: Climb in the tree */
     int b_done;
     b_done = c->barrier_done;

     int b;

#ifdef ATOMICS
     b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
#else
     sctk_spinlock_lock(&(c->lock));
     b = c->barrier;
     b++;
     c->barrier = b;
     sctk_spinlock_unlock(&(c->lock));
#endif

     while ((b+1 == c->barrier_num_threads) && (c->father != NULL)) {
#ifdef ATOMICS
        sctk_atomics_store_int (&c->barrier,0) ;
#else
        c->barrier = 0;
#endif
        c = c->father;

        b_done = c->barrier_done;      

#ifdef ATOMICS
        b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
#else
        sctk_spinlock_lock(&(c->lock));
        b = c->barrier;
        b++;
        c->barrier = b;
        sctk_spinlock_unlock(&(c->lock));
#endif

        sctk_nodebug("mpcomp_internal_barrier: else thread %d",mvp->rank);
     }
}
/*
   Full barrier
*/
void __mpcomp_internal_barrier(mpcomp_mvp_t *mvp)
{

     sctk_nodebug("__mpcomp_internal_barrier: mvp rank %d",mvp->rank);

     /*Barrier to wait for the other microVPs*/
     mpcomp_node_t *c;
     c = mvp->father;

     /* Step 1: Climb in the tree */
     int b_done;
     b_done = c->barrier_done;

     int b;

#ifdef ATOMICS
     b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
#else
     sctk_spinlock_lock(&(c->lock));
     b = c->barrier;
     b++;
     c->barrier = b;
     sctk_spinlock_unlock(&(c->lock));
#endif

     while ((b+1 == c->barrier_num_threads) && (c->father != NULL)) {
#ifdef ATOMICS
        sctk_atomics_store_int (&c->barrier,0) ;
#else
        c->barrier = 0;
#endif
        c = c->father;

        b_done = c->barrier_done;      

#ifdef ATOMICS
        b = sctk_atomics_fetch_and_incr_int (&c->barrier) ;
#else
        sctk_spinlock_lock(&(c->lock));
        b = c->barrier;
        b++;
        c->barrier = b;
        sctk_spinlock_unlock(&(c->lock));
#endif

        sctk_nodebug("mpcomp_internal_barrier: else thread %d",mvp->rank);
     }

     /* Step 2: Wait for the barrier to be done */
     if ((c->father != NULL) || (b+1 != c->barrier_num_threads)) {
       /* Wait for c->barrier == c->barrier_num_threads */ 
       sctk_nodebug("mpcomp_internal_barrier: if thread %d",mvp->rank);
       while (b_done == c->barrier_done) {
          sctk_thread_yield();
       }
     }
     else {
#ifdef ATOMICS
       sctk_atomics_store_int (&c->barrier,0);
#else
       c->barrier = 0;
#endif
       c->barrier_done++;
       /* TODO: not sure that we need that. If we do need it, maybe we need to lock */
       sctk_nodebug("mpcomp_internal_barrier: else thread %d",mvp->rank);
     }

     /* Step 3: Go down in the tree to wake up the children */
     while (c->child_type != CHILDREN_LEAF) {
         sctk_nodebug("mpcomp_internal_barrier: step3 thread %d",mvp->rank);
         c = c->children.node[mvp->tree_rank[c->depth]];
         c->barrier_done++;
     }
}

/*
   OpenMP barrier
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

  sctk_nodebug("thread rank %d",t->rank);

  /* Grab the corresponding microVP */
  mvp = t->mvp;
  sctk_assert(mvp != NULL);

  __mpcomp_internal_barrier(mvp);
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

    //sctk_spinlock_lock(&(mvp->father->lock));
    mvp->func(mvp->shared);
    //sctk_spinlock_unlock(&(mvp->father->lock));

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

  return t->num_threads;
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

void __mpcomp_barrier_for_dyn(void)
{
  mpcomp_thread_t *t;
  mpcomp_mvp_t *mvp;

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  /* Grab the corresponding microVP */
  mvp = t->mvp;
  sctk_assert(mvp != NULL);

  /* Block only if I am not the only thread in the team */
  if (t->num_threads > 1) {
    /* If there only 1 OpenMP threads on this microVP? */
    if (mvp->nb_threads == 1) {
      __mpcomp_internal_barrier(mvp);
    }    
  }

  
  
  sctk_nodebug("__mpcomp_barrier_for_dyn: Leaving");
}

int __mpcomp_dynamic_loop_begin(int lb, int b, int incr, int chunk_size, int *from, int *to)
{
  mpcomp_thread_t *t;
  mpcomp_thread_t *father;
  int rank;
  int index;

  /* Grab the info of the current thread */    
  t = (mpcomp_thread_t *)sctk_openmp_thread_tls;
  sctk_assert(t != NULL);

  sctk_nodebug("thread rank %d",t->rank);

  /* Get the father info */
  father = t->father;
  sctk_assert(father != NULL);

  /* Get the rank of the current thread */
  rank = t->rank;  

  /* Index of the next loop */
  index = (t->private_current_for_dyn + 1) % MPCOMP_MAX_ALIVE_FOR_DYN;

  /* If it is a barrier */
  if (father->stop[index] != OK) {

    sctk_spinlock_lock(&(father->lock_stop_for_dyn));

    /* Is it the last loop to perform before synchronization? */
    if (father->stop[index] == STOP) {
      father->stop[index] = STOPPED;
      sctk_spinlock_unlock(&(father->lock_stop_for_dyn));

      /* Call barrier */
      __mpcomp_barrier_for_dyn();

      return __mpcomp_dynamic_loop_begin(lb, b, incr, chunk_size, from, to);       
    }
   
    /* This loop has been the last loop before synchronization */ 
    if (father->stop[index] == STOPPED) {
      sctk_spinlock_unlock(&(father->lock_stop_for_dyn));

      /* Call barrier */
      __mpcomp_barrier_for_dyn();

      return __mpcomp_dynamic_loop_begin(lb, b, incr, chunk_size, from, to);       
    }

    sctk_spinlock_unlock(&(father->lock_stop_for_dyn));
  }
}

int __mpcomp_dynamic_loop_next(int *from, int *to)
{
}

void __mpcomp_dynamic_loop_end_nowait()
{
}



