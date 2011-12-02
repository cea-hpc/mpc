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
#ifdef MPC_Message_Passing
#include <sctk_communicator.h>
#endif


/*******************
       MACROS 
********************/
#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 0

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

/******** OPENMP THREAD *********/
struct mpcomp_thread_s {
  sctk_mctx_t uc;				/* Context: register initialization, etc. Initialized when another thread is blocked */
  char *stack;					/* Stack: initialized when another thread is blocked */
  int depth;					/* Depth of the current thread (0 =sequential region) */
  icv_t icvs;					/* Set of the ICVs for the child team */
  long rank;					/* OpenMP rank. 0 for master thread per team */
  long num_threads;				/* Current number of threads in the team */
  struct mpcomp_mvp_s *mvp;			/* Openmp microvp */
  int index_in_mvp;	
  volatile int done;
  struct mpcomp_instance_s *children_instance; 	/* Instance for nested parallelism */
  struct mpcomp_thread_s *father; 		/* TODO: check if this is useful */
  void *hierarchical_tls;			/* Local variables */
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
  long rank;					/* Rank among children of my father */
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
  volatile long barrier;			/* Barrier for the child team */
  char pad2[64];
  volatile long barrier_done;			/* Is the barrier for the child team is over? */
  char pad3[64];
  volatile long barrier_num_threads;		/* Number of threads involved in the barrier */
  char pad4[64];
  int num_threads;				/* Number of threads in the current team */ 
  void *(*func) (void*);
  void *shared;
};

typedef struct mpcomp_node_s mpcomp_node_t;


/*********************
       FUNCTIONS
**********************/
void __mpcomp_instance_init(mpcomp_instance_t *instance, int nb_mvps);
void __mpcomp_thread_init(mpcomp_thread_t *t);
void *mpcomp_slave_mvp_node(void *arg);
void *mpcomp_slave_mvp_leaf(void *arg);
void in_order_scheduler(mpcomp_mvp_t *mvp);

/* 
  Read the environment variables. Once per process 
*/
inline void __mpcomp_read_env_variables() 
	  OMP_VP_NUMBER = OMP_VP_NUMBER /  sctk_get_nb_task_local(SCTK_COMM_WORLD); /* DEFAULT */
	  if(OMP_VP_NUMBER < 1){
	    OMP_VP_NUMBER = 1;
	  }
#endif
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
	sctk_debug( "Remaining string for schedule: <%s>", &env[offset] ) ;
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

     sctk_nodebug("mpcomp_slave_mvp_node: Polling address %p ith ID %d",
		&(n->slave_running[0]),0); 

     /* Spinning on the designed node */
     sctk_thread_wait_for_value_and_poll((int *)&(n->slave_running[0]), 1 , NULL , NULL);
     n->slave_running[0] = 0;

     sctk_nodebug("mpcomp_slave_mvp_node: wake_up -> go to children using function %p", n->func);

     /* Wake up children node */
     while (n->child_type != CHILDREN_LEAF) {

        sctk_nodebug("mpcomp_slave_mvp_node: children node passing function %p", n->func);

        for (i=1 ; i<n->nb_children ; i++) {
	  n->children.node[i]->func = n->func;
	  n->children.node[i]->shared = n->shared;
	  n->children.node[i]->num_threads = n->num_threads;
	  n->children.node[i]->slave_running[0] = 1;
	}

#warning "TODO put the real barrier_num_threads"
        n->barrier_num_threads = n->nb_children;
        n = n->children.node[0];
     }

     /* Wake up children leaf */
     sctk_assert(n->child_type == CHILDREN_LEAF);
     sctk_nodebug("mpcomp_slave_mvp_node: children leaf will use %p",n->func);

     for (i=1 ; i<n->nb_children ; i++) {
       n->children.leaf[i]->slave_running[0] = 1;
     }

     n->barrier_num_threads = n->nb_children;
     
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

     /*Barrier to wait for the other microVPs*/
     volatile mpcomp_node_t *c;
     c = mvp->father;

     /* Step 1: Climb in the tree */
     volatile long b_done;
     b_done = c->barrier_done;

     volatile int b;

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

     /* Step 2: Wait for the barrier to be done */
     if (c->father != NULL) {
       /* Wait for c->barrier == c->barrier_num_threads */ 
       sctk_nodebug("mpcomp_slave_mvp_node: if thread %d",mvp->rank);
       while (b_done == c->barrier_done) {
          sctk_thread_yield();
       }
     }
     else {
       /* TODO: not sure that we need that. If we do need it, maybe we need to lock */
       sctk_nodebug("mpcomp_slave_mvp_node: else thread %d",mvp->rank);
       c->barrier_done++;
       c->barrier = 0;
     }

     /* Step 3: Go down in the tree to wake up the children */
     while (c->child_type != CHILDREN_LEAF) {
         sctk_nodebug("mpcomp_slave_mvp_node: step3 thread %d",mvp->rank);
         c = c->children.node[mvp->tree_rank[c->depth]];
         c->barrier_done++;
     }

#if 0
     mpcomp_node_t *c;
     int b;

     c = mvp->father;

     sctk_spinlock_lock(&(c->lock));
#if defined (SCTK_USE_OPTIMIZED_TLS)
	  sctk_tls_module = info->tls_module;
	  sctk_context_restore_tls_module_vp() ;
#endif

     b = c->barrier;
     b++;
     c->barrier = b;

     sctk_spinlock_unlock(&(c->lock));

     /* TODO: Didn't understand that barrier */
     while ((b == c->barrier_num_threads || b == c->nb_children) && c->father != NULL) {
        c->barrier = 0;
	c = c->father;

	sctk_spinlock_lock(&(c->lock));

        b = c->barrier;
        b++;
        c->barrier = b;

	sctk_spinlock_unlock(&(c->lock));
     }
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

     /*Barrier to wait for the other microVPs*/
     volatile mpcomp_node_t *c;
     c = mvp->father;

     /* Step 1: Climb in the tree */
     volatile long b_done;
     b_done = c->barrier_done;

     volatile int b;

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

     /* Step 2: Wait for the barrier to be done */
     if (c->father != NULL) {
       /* Wait for c->barrier == c->barrier_num_threads */ 
       sctk_nodebug("mpcomp_slave_mvp_leaf: if thread %d",mvp->rank);
       while (b_done == c->barrier_done) {
          sctk_thread_yield();
       }
     }
     else {
       /* TODO: not sure that we need that. If we do need it, maybe we need to lock */
       sctk_nodebug("mpcomp_slave_mvp_leaf: else thread %d",mvp->rank);
       c->barrier_done++;
       c->barrier = 0;
     }

     /* Step 3: Go down in the tree to wake up the children */
     while (c->child_type != CHILDREN_LEAF) {
         sctk_nodebug("mpcomp_slave_mvp_leaf: step3 thread %d",mvp->rank);
         c = c->children.node[mvp->tree_rank[c->depth]];
         c->barrier_done++;
     }

#if 0
     mpcomp_node_t *c;
     int b;

     c = mvp->father;

     sctk_spinlock_lock(&(c->lock));

     b = c->barrier;
     b++;
     c->barrier = b;

     sctk_spinlock_unlock(&(c->lock));

     /* TODO: Didn't understand that barrier */
     while (b == c->barrier_num_threads && c->father != NULL) {
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


   return NULL;
}

/*
   Initialize an OpenMP thread
*/
void __mpcomp_thread_init (mpcomp_thread_t *t)
{

   t->depth = 0;
   t->rank = 0;
   t->num_threads = 1;
   t->mvp = NULL;
   t->index_in_mvp = 0;
   t->done = 0;
   t->father = t;
   t->hierarchical_tls = NULL;
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
	case 32: /* NUMA tree */
#if 1
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
	  root->barrier = 0;
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
	    n->barrier = 0;
	    n->barrier_done = 0;
	    n->children.leaf = (mpcomp_mvp_t **)sctk_malloc_on_node(n->nb_children*sizeof(mpcomp_mvp_t *),i);

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
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d", current_mvp, target_vp, i, i);
	           instance->mvps[current_mvp]->to_run = root;

	           res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
					mpcomp_slave_mvp_node,
					instance->mvps[current_mvp]);

	           sctk_assert(res == 0);
	           break;

	         case 1: /* Numa */
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d", current_mvp, target_vp, i, i);
	           instance->mvps[current_mvp]->to_run = root->children.node[i];

	           res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
					mpcomp_slave_mvp_node,
					instance->mvps[current_mvp]);

	           sctk_assert(res == 0);
	           break;

	         case 2: /* MicroVP */
	           sctk_nodebug("__mpcomp_instance_init: Creation microVP %d. VP %d, NUMA %d", current_mvp, target_vp, i, i);

	           res = sctk_user_thread_create(&(instance->mvps[current_mvp]->pid), &__attr,
					mpcomp_slave_mvp_leaf,
					instance->mvps[current_mvp]);

	           sctk_assert(res == 0);
	           break;

	         default:
	           break;
	      }
	      
	      sctk_thread_attr_destroy(&__attr);

	      current_mvp++;
	      flag_level = -1;
	    }
	  }
#endif
	  break;
	default:
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

    num_threads_per_child = num_threads / instance->nb_mvps;
    max_index_with_more_threads = (num_threads % instance->nb_mvps)-1;

    sctk_nodebug("__mpcomp_start_parallel_region: #threads_per_mvp %d. %d mvps have an additional thread",num_threads_per_child,max_index_with_more_threads);

    n = root;

    while (n->child_type != CHILDREN_LEAF) {
       int nb_children_involved = 1;

       for (i=1 ; i<n->nb_children ; i++) {
         int temp_num_threads;

         sctk_nodebug("__mpcomp_start_parallel_region: Testing child %d with min_index = %d, #threads = %d",i,n->children.node[i]->min_index,num_threads);

         /* Compute the number of threads for the smallest child of the subtree 'n' */
         temp_num_threads = num_threads_per_child;
         if (n->children.node[i]->min_index < max_index_with_more_threads)
           temp_num_threads++;

         if (temp_num_threads > 0) {
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
      int num_threads_leaf;
      num_threads_leaf = num_threads / instance->nb_mvps;

      if (n->children.leaf[i]->rank < (num_threads%instance->nb_mvps)) num_threads_leaf++;
      if (num_threads_leaf > 0) {
        n->children.leaf[i]->slave_running[0] = 1;
        nb_leaves_involved++;
      }
    }

    sctk_nodebug("__mpcomp_start_parallel_region: nb_leaves_involved=%d",nb_leaves_involved);
    n->barrier_num_threads = nb_leaves_involved;

    instance->mvps[0]->func = func;
    instance->mvps[0]->shared = shared;
    instance->mvps[0]->nb_threads = 1;

    /* Compute the number of OpenMP threads and their rank running on microVP #0 */
    instance->mvps[0]->threads[0].num_threads = num_threads;
    instance->mvps[0]->threads[0].rank = 0;

    /* Start scheduling */
    in_order_scheduler(instance->mvps[0]);

    /*Barrier to wait for the other microVPs*/
    volatile mpcomp_node_t *c;
    c = instance->mvps[0]->father;

    /* Step 1: Climb in the tree */
    volatile long b_done;
    b_done = c->barrier_done;

    volatile int b;

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

    /* Step 2: Wait for the barrier to be done */
    if (c->father != NULL) {
      /* Wait for c->barrier == c->barrier_num_threads */ 
      sctk_nodebug("mpcomp_start_parallel: if thread %d",instance->mvps[0]->rank);
      while (b_done == c->barrier_done) {
         sctk_thread_yield();
      }
    }
    else {
      /* TODO: not sure that we need that. If we do need it, maybe we need to lock */
      sctk_nodebug("mpcomp_start_parallel: else thread %d",instance->mvps[0]->rank);
      c->barrier_done++;
      c->barrier = 0;
    }

    /* Step 3: Go down in the tree to wake up the children */
    while (c->child_type != CHILDREN_LEAF) {
        sctk_nodebug("mpcomp_start_parallel: step3 thread %d",instance->mvps[0]->rank);
        c = c->children.node[instance->mvps[0]->tree_rank[c->depth]];
        c->barrier_done++;
    }

#if 0
    mpcomp_node_t *c;
    c = instance->mvps[0]->father;

    int b;

    sctk_nodebug("__mpcomp_start_parallel_region: Entering the implicit final barrier");

    sctk_spinlock_lock(&(c->lock));

    b = c->barrier;
    b++;
    c->barrier = b;

    sctk_spinlock_unlock(&(c->lock));

    while(b == c->barrier_num_threads && c->father != NULL) {
       c->barrier = 0;
       c = c->father;

       sctk_spinlock_lock(&(c->lock));
  
       b = c->barrier;
       b++;
       c->barrier = b;
  
       sctk_spinlock_unlock(&(c->lock));
    }

    sctk_nodebug("mpcomp_start_parallel_region: thread %d before root barrier",instance->mvps[0]->rank);    

    b = root->barrier;

    sctk_nodebug("__mpcomp_start_parallel_region: Master waiting on the root node from %d to %d",b,root->barrier_num_threads);

    while(b != root->barrier_num_threads) {
       sctk_thread_yield();
       b = root->barrier;
    }
    root->barrier = 0;
#endif

    /* Restore the previous OpenMP info */
    sctk_openmp_thread_tls = t;
   
  }
  else {
    fprintf(stderr,"Nesting in not implemented.");
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
    sctk_nodebug("in_order_scheduler: func %p shared %p",mvp->func,mvp->shared);
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

