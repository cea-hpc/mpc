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
#ifndef __mpcomp_internal__H
#define __mpcomp_internal__H

#include "mpcomp.h"
#include "sctk.h"
#include "sctk_atomics.h"
#include "sctk_context.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*****************
 ****** MACROS 
 *****************/

/* Maximum number of threads for each team of a parallel region */
#define MPCOMP_MAX_THREADS		256
/* Number of threads per microVP */
#define MPCOMP_MAX_THREADS_PER_MICROVP	8


/* Maximum number of alive 'single' construct */
#define MPCOMP_MAX_ALIVE_SINGLE		1023	
/* Maximum number of alive 'for dynamic' construct */
#define MPCOMP_MAX_ALIVE_FOR_DYN 	1023

#define MPCOMP_NOWAIT_STOP_SYMBOL	(-2)
#define MPCOMP_NOWAIT_STOP_CONSUMED	(-3)


     /* MACRO FOR PERFORMANCE */
#define MPCOMP_USE_ATOMICS	1
#define MPCOMP_MALLOC_ON_NODE	1

#define MPCOMP_CHUNKS_NOT_AVAIL 1
#define MPCOMP_CHUNKS_AVAIL     2



/*****************
 ************ ENUM 
 *****************/

     /* Type of element in the stack for dynamic work stealing */
     typedef enum mpcomp_elem_stack_type_t {
	  MPCOMP_ELEM_STACK_NODE = 1,
	  MPCOMP_ELEM_STACK_LEAF = 2,
     } mpcomp_elem_stack_type_t;

     /* Type of children in the topology tree */
     typedef enum mpcomp_children_t {
	  MPCOMP_CHILDREN_NODE = 1,
	  MPCOMP_CHILDREN_LEAF = 2,
     } mpcomp_children_t;


     typedef enum mpcomp_context_t {
	  MPCOMP_CONTEXT_IN_ORDER = 1,
	  MPCOMP_CONTEXT_OUT_OF_ORDER_MAIN = 2,
	  MPCOMP_CONTEXT_OUT_OF_ORDER_SUB = 3,
     } mpcomp_context_t;

     typedef enum mpcomp_myself_t {
	  MPCOMP_MYSELF_ROOT = 0,
	  MPCOMP_MYSELF_NODE = 1,
	  MPCOMP_MYSELF_LEAF = 2,
     } mpcomp_myself_t ;


/*****************
 ****** STRUCTURES 
 *****************/

     /* Internal Control Variables */
     /* One structure per OpenMP thread */
     typedef struct mpcomp_icv_s 
     {
	  int nthreads_var;		/* Number of threads for the next team 
					   creation */
	  int dyn_var;		        /* Is dynamic thread adjustement on? */
	  int nest_var;		        /* Is nested OpenMP handled/allowed? */
	  omp_sched_t run_sched_var;	/* Schedule to use when a 'schedule' clause is
					   set to 'runtime' */
	  int modifier_sched_var;	/* Size of chunks for loop schedule */
	  omp_sched_t def_sched_var;	/* Default schedule when no 'schedule' clause
					   is present */
	  int nmicrovps_var;		/* Number of VPs */
     } mpcomp_icv_t;

     
    /* Integer atomic with padding to avoid false sharing */
     typedef struct mpcomp_atomic_int_pad_s
     {
	  sctk_atomics_int i;   /* Value of the integer */
	  char pad[8];          /* Padding */
     } mpcomp_atomic_int_pad_t;


     /* Element of the stack for dynamic work stealing */
     typedef struct mpcomp_elem_stack_s
     {
	  union node_leaf {
	       struct mpcomp_node_s *node;
	       struct mpcomp_mvp_s *leaf;
	  } elem;                               /* Stack element */
	  enum mpcomp_elem_stack_type_t type;   /* Type of the 'elem' field */
     } mpcomp_elem_stack_t;

     
     /* Stack structure for dynamic work stealing */
     typedef struct mpcomp_stack_node_leaf_s
     {
	  struct mpcomp_elem_stack_s **elements;   /* List of elements */
	  enum mpcomp_children_t *child_type;      /* Type of childs : nodes or leafs */
	  int max_elements;                        /* Number of max elements */
	  int n_elements;                          /* Corresponds to the head of the stack */
     } mpcomp_stack_node_leaf_t;


     /* Team of OpenMP threads */
     typedef struct mpcomp_team_s
     {
	  int depth;			/* Depth of the current thread (0 = sequential region) */

	  /* -- SINGLE CONSTRUCT -- */
	  int single_last_current;
#if MPCOMP_USE_ATOMICS
	  mpcomp_atomic_int_pad_t single_nb_threads_entered[MPCOMP_MAX_ALIVE_SINGLE + 1];
#else
	  sctk_spinlock_t single_lock_enter[MPCOMP_MAX_ALIVE_SINGLE + 1];
	  volatile int single_nb_threads_entered[MPCOMP_MAX_ALIVE_SINGLE + 1];
#endif
	  volatile int single_first_copyprivate;
	  void *single_copyprivate_data;
	  volatile int single_nb_threads_stopped;

	  /* -- SECTION CONSTRUCT -- */

	  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	  int for_dyn_last_current;
	  mpcomp_atomic_int_pad_t for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN + 1];

	  /* -- GUIDED FOR LOOP CONSTRUCT -- */

	  /* ORDERED CONSTRUCT */
	  volatile int next_ordered_offset; 
     } mpcomp_team_t;


     /* Chunk of iteration loops */
     typedef struct mpcomp_chunk_s
     {
	  sctk_atomics_int total;    /* Number of total chunks */
	  sctk_atomics_int remain;   /* Number of chunks remaining (to be executed) */
     } mpcomp_chunk_t;


     /* OpenMP thread */
     typedef struct mpcomp_thread_s
     {
	  sctk_mctx_t uc;		/* Context (initializes registers, ...)
					   Initialized when another thread is blocked */
	  mpcomp_icv_t icvs;		/* Set of ICVs for the child team */
	  char *stack;                  /* The stack (initialized when another thread is 
					   blocked) */
	  long rank;			/* OpenMP rank (0 for master thread per team) */
	  long num_threads;		/* Current number of threads in the team */
	  struct mpcomp_mvp_s *mvp;     /* Micro-vp of the thread*/
	  volatile int done;            /* Done value for in-order scheduler */
	  struct mpcomp_instance_s *children_instance; /* Instance for nested parallelism */

	  struct mpcomp_team_s *team;   /* Information on the team */

	  /* -- SINGLE CONSTRUCT -- */
	  int single_current;		/* Which 'single' construct did we already go through? */

	  /* LOOP CONSTRUCT */
	  int loop_lb;		        /* Lower bound */
	  int loop_b;			/* Upper bound */
	  int loop_incr;		/* Step */
	  int loop_chunk_size;	        /* Size of each chunk */

	  /* -- STATIC FOR LOOP CONSTRUCT -- */
	  int static_nb_chunks;         /* Number of total static chunk for the thread */
	  int static_current_chunk;     /* Current chunk index */

	  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	  int for_dyn_current;    /* Current position in 'for_dyn_chunk_info' array  */
	  mpcomp_chunk_t for_dyn_chunk_info[MPCOMP_MAX_ALIVE_FOR_DYN + 1]; /* Chunks array 
									      for loop dynamic 
									      schedule constructs */

	  /* ORDERED CONSTRUCT */
	  int current_ordered_iteration; 

	  /* Informations for DFS */
	  struct mpcomp_stack_node_leaf_s *tree_stack; /* Stack containing the OpenMP thread tree */
	  struct mpcomp_mvp_s *stolen_mvp;             /* Last microvp where chunks were stolen */
	  int stolen_chunk_id;                         /* Chunk id in the mvp current thread's */
	  int start_mvp_index;                         /* Mark to know on what mvp rank we start
							  for chunk stealing
							  - avoid useless search */
     } mpcomp_thread_t;


     /* Instance of OpenMP runtime */
     typedef struct mpcomp_instance_s
     {
	  int nb_mvps;	        	  /* Number of microVPs for this instance */
	  struct mpcomp_mvp_s **mvps;	  /* All microVPs of this instance */
	  struct mpcomp_node_s *root;	  /* Root to the tree linking the microVPs */
     } mpcomp_instance_t;


     /* OpenMP Micro VP */
     typedef struct mpcomp_mvp_s
     {
	  sctk_mctx_t vp_context;   /* Context including registers, stack pointer, ... */
	  sctk_thread_t pid; 	    /* Thread ID */
	  int nb_threads;           /* Total number of threads running on the Micro VP */
	  int next_nb_threads;
	  struct mpcomp_thread_s threads[MPCOMP_MAX_THREADS_PER_MICROVP];

	  int *tree_rank; 	          /* Array of rank in every node of the tree */
	  int rank;    	                  /* Rank within the microVPs */
	  char pad0[64];                  /* Padding */
	  volatile int enable;
	  char pad1[64];                  /* Padding */
	  struct mpcomp_node_s *root;     /* Root of the topology tree */
	  char pad1_1[64];                /* Padding */
	  struct mpcomp_node_s *father;   /* Father node in the topology tree */
	  char pad1_2[64];                /* Padding */
	  struct mpcomp_node_s *to_run;   /* Specify where the spin waiting for a new 
					     parallel region */
	  char pad2[64];                  /* Padding */
	  volatile int slave_running;
	  char pad3[64];                  /* Padding */
	  enum mpcomp_myself_t type;      /**/
	  void *(*func) (void *);	  /* Function to call by every thread */
	  void *shared;		          /* Shared variables (for every thread) */
     } mpcomp_mvp_t;

 

     /* OpenMP Node */
     typedef struct mpcomp_node_s
     {
	  struct mpcomp_node_s *father;   /* Father in the topology tree */
	  long rank;                      /* Rank among children of my father */
	  int depth;                      /* Depth in the tree */
	  int nb_children;                /* Number of children */

	  /* The following indices correspond to the 'rank' value in microVPs */
	  int min_index;   /* Flat min index of leaves in this subtree */
	  int max_index;   /* Flat max index of leaves in this subtree */

	  mpcomp_team_t *team_info;   /* Information on the whole team */

	  mpcomp_children_t child_type;       /* Kind of children (node or leaf) */
	  union node_or_leaf {
	       struct mpcomp_node_s **node;
	       struct mpcomp_mvp_s **leaf;
	  } children;                         /* Children list */

	  sctk_spinlock_t lock;	        /* Lock for structure updates */
	  char pad0[64];                /* Padding */
	  volatile int slave_running;
	  char pad1[64];                /* Padding */

#if MPCOMP_USE_ATOMICS
	  sctk_atomics_int barrier;	                /* Barrier for the child team */
	  sctk_atomics_int chunks_avail;                /* Flag for presence of chunks 
							   under current node */
	  sctk_atomics_int nb_chunks_empty_children;    /* Counter for presence of chunks */
#else
	  volatile long barrier;	                /* Barrier for the child team */
	  volatile long chunks_avail;                   /* Flag for presence of chunks 
							   under current node */
	  volatile long nb_chunks_empty_children;       /* Counter for presence of chunks */
#endif

	  char pad2[64];                       /* Padding */
	  volatile long barrier_done;          /* Is the barrier (for the child team) over? */
	  char pad3[64];                       /* Padding */
	  volatile long barrier_num_threads;   /* Number of threads involved in the barrier */
	  char pad4[64];                       /* Padding */

	  enum mpcomp_myself_t type;
	  int current_mvp;
	  int id_numa;
        
	  int num_threads;          /* Number of threads in the current team */
	  void *(*func) (void *);
	  void *shared;
     } mpcomp_node_t;


     /* Stack (maybe the same that mpcomp_stack_node_leaf_s structure) */
     typedef struct mpcomp_stack {
	  mpcomp_node_t **elements;
	  int max_elements;
	  int n_elements;             /* Corresponds to the head of the stack */
     } mpcomp_stack_t;


/*****************
 ****** VARIABLES 
 *****************/

     extern __thread void *sctk_openmp_thread_tls;   /* Current thread pointer */



/*****************
 ****** FUNCTIONS  
 *****************/
     
     static inline void __mpcomp_team_init(mpcomp_team_t *team_info)
     {
	  int i;

	  /* -- TEAM INFO -- */
	  team_info->depth = 0;

	  /* -- SINGLE CONSTRUCT -- */
	  team_info->single_last_current = MPCOMP_MAX_ALIVE_SINGLE;
#if MPCOMP_USE_ATOMICS
	  for (i=0; i<MPCOMP_MAX_ALIVE_SINGLE; i++)
	       sctk_atomics_store_int(&(team_info->single_nb_threads_entered[i].i), 0);
	  sctk_atomics_store_int(&(team_info->single_nb_threads_entered[MPCOMP_MAX_ALIVE_SINGLE].i), 
				 MPCOMP_MAX_THREADS);
	  sctk_nodebug("__mpcomp_team_init: Filling cell %d with %d", 
		       MPCOMP_MAX_ALIVE_SINGLE, MPCOMP_MAX_THREADS);
#else
	  sctk_debug("__mpcomp_team_init: no atomics!");
	  for (i=0 ; i<MPCOMP_MAX_ALIVE_SINGLE; i++) {
	       team_info->single_nb_threads_entered[i] = 0;
	       team_info->single_lock_enter[i] = SCTK_SPINLOCK_INITIALIZER;
	  }
	  team_info->single_nb_threads_entered[MPCOMP_MAX_ALIVE_SINGLE] = MPCOMP_NOWAIT_STOP_SYMBOL;
	  team_info->single_lock_enter[MPCOMP_MAX_ALIVE_SINGLE] = SCTK_SPINLOCK_INITIALIZER;
#endif
	  team_info->single_first_copyprivate = 0;
	  team_info->single_copyprivate_data = NULL;
	  team_info->single_nb_threads_stopped = 0;

	  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	  team_info->for_dyn_last_current = MPCOMP_MAX_ALIVE_FOR_DYN;
	  for (i=0; i<MPCOMP_MAX_ALIVE_FOR_DYN; i++)
	       sctk_atomics_store_int(&(team_info->for_dyn_nb_threads_exited[i].i), 0);
	  sctk_atomics_store_int(&(team_info->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i),
				 MPCOMP_NOWAIT_STOP_SYMBOL);
     }

     static inline void __mpcomp_thread_init(mpcomp_thread_t *t, mpcomp_icv_t icvs,
					     mpcomp_instance_t *instance,
					     mpcomp_team_t *team_info)
     {
	  int i;

	  t->icvs = icvs;
	  t->stack = NULL;
	  t->rank = 0;
	  t->num_threads = 1;
	  t->mvp = NULL;
	  t->done = 0;
	  t->children_instance = instance;
	  t->team = team_info;
	  t->single_current = -1;

	  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
	  t->tree_stack = NULL;
	  t->stolen_mvp = NULL;
	  t->start_mvp_index = -1; //AMAHEO
     }

     /* mpcomp.c */
     void __mpcomp_init(void);
     void __mpcomp_instance_init(mpcomp_instance_t *instance, int nb_mvps);
     void in_order_scheduler(mpcomp_mvp_t * mvp);
     void * mpcomp_slave_mvp_node(void *arg);
     void * mpcomp_slave_mvp_leaf(void *arg);
     void __mpcomp_internal_half_barrier();
     void __mpcomp_internal_full_barrier();
     void __mpcomp_barrier();

     /* mpcomp_tree.c */
     int __mpcomp_check_tree_parameters(int n_leaves, int depth, int *degree);
     int __mpcomp_build_default_tree(mpcomp_instance_t *instance);
     int __mpcomp_build_tree(mpcomp_instance_t *instance, int n_leaves, int depth, int *degree);
     int __mpcomp_check_tree_coherency(mpcomp_instance_t *instance);
     void __mpcomp_print_tree(mpcomp_instance_t *instance);

     /* mpcomp_loop_dyn.c */
     int __mpcomp_dynamic_steal(int *from, int *to);
     int __mpcomp_dynamic_loop_begin(int lb, int b, int incr,
				      int chunk_size, int *from, int *to);
     int __mpcomp_dynamic_loop_next(int *from, int *to);
     void __mpcomp_dynamic_loop_end();
     void __mpcomp_dynamic_loop_end_nowait();

     /* Stack primitives */
     mpcomp_stack_t * __mpcomp_create_stack(int max_elements);
     int __mpcomp_is_stack_empty(mpcomp_stack_t *s);
     void __mpcomp_push(mpcomp_stack_t *s, mpcomp_node_t *n);
     mpcomp_node_t * __mpcomp_pop(mpcomp_stack_t *s);
     void __mpcomp_free_stack(mpcomp_stack_t *s);

     mpcomp_stack_node_leaf_t * __mpcomp_create_stack_node_leaf(int max_elements);
     int __mpcomp_is_stack_node_leaf_empty(mpcomp_stack_node_leaf_t *s);
     void __mpcomp_push_node(mpcomp_stack_node_leaf_t *s, mpcomp_node_t *n);
     void __mpcomp_push_leaf(mpcomp_stack_node_leaf_t *s, mpcomp_mvp_t *n);
     mpcomp_elem_stack_t * __mpcomp_pop_elem_stack(mpcomp_stack_node_leaf_t *s);
     void __mpcomp_free_stack_node_leaf(mpcomp_stack_node_leaf_t *s);


     void __mpcomp_get_specific_chunk_per_rank (int rank, int nb_threads,
						int lb, int b, int incr,
						int chunk_size, int chunk_num,
						int *from, int *to);

     int __mpcomp_get_static_nb_chunks_per_rank (int rank, int nb_threads, int lb,
						 int b, int incr, int chunk_size);

#ifdef __cplusplus
}
#endif
#endif
