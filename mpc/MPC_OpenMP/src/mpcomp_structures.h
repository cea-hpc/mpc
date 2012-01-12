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

#ifdef __cplusplus
extern "C"
{
#endif

#include <sctk_asm.h>
#include <sctk_context.h>
#include <sctk_posix_thread.h>
#include <sctk_thread_api.h>
#include <sctk_spinlock.h>
#include <sctk_tls.h>
#include <sctk_debug.h>
#include <mpcmicrothread.h>
#include <mpcomp_abi.h>

/*************************************************
*************************************************
	BEGIN: NEW MPCOMP STRUCTURES
*************************************************
**************************************************/

/*******************
       MACROS 
********************/

#define SCTK_OMP_VERSION_MAJOR 3
#define SCTK_OMP_VERSION_MINOR 0
#define OK 1
#define STOP 2
#define STOPPED 3

/* Maximum number of threads for each team of a parallel region */
#define MPCOMP_MAX_THREADS 		64
/* Maximum number of shared for loops w/ dynamic schedule alive */
#define MPCOMP_MAX_ALIVE_FOR_DYN 	7
/* Maximum number of alive 'single' construct */
#define MPCOMP_MAX_ALIVE_SINGLE 	3

#define MPCOMP_NOWAIT_STOP_SYMBOL    	-1
#define MPCOMP_NOWAIT_STOP_CONSUMED    	-2
#define MPCOMP_NOWAIT_OK_SYMBOL        	-3	

#define MPCOMP_LOCK_INIT {0,0,NULL,NULL,NULL,SCTK_SPINLOCK_INITIALIZER}

extern __thread void *sctk_openmp_thread_tls;

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

/****** Linked list of locks (per lock structure, who is blocked with this lock) ******/

struct mpcomp_slot_s {
   struct mpcomp_thread_s *t;
   struct mpcomp_slot_s *next;
};
typedef struct mpcomp_slot_s mpcomp_slot_t;

/******** OPENMP THREAD *********/
struct mpcomp_thread_s {
  sctk_mctx_t uc;				/* Context: register initialization, etc. Initialized when another thread is blocked */
  char *stack;					/* Stack: initialized when another thread is blocked */
  int depth;					/* Depth of the current thread (0 =sequential region) */
  icv_t icvs;					/* Set of the ICVs for the child team */
  int rank;					/* OpenMP rank. 0 for master thread per team */
  int num_threads;				/* Current number of threads in the team */
  struct mpcomp_mvp_s *mvp;			/* Openmp microvp */
  int is_running;				
  int index_in_mvp;	
  volatile int done;
  struct mpcomp_instance_s *children_instance; 	/* Instance for nested parallelism */
  struct mpcomp_thread_father_s *father; 	/* TODO: check if this is useful */
  void *hierarchical_tls;			/* Local variables */

  /* Private info on the current loop (whatever its schedule is)  */
  int loop_lb;					/* Lower bound */
  int loop_b;					/* Upper bound */
  int loop_incr;				/* Step */
  int loop_chunk_size;				/* Size of each chunk */

  /* FOR LOOPS - STATIC SCHEDULE */
  int static_nb_chunks;

  /* What is the currently scheduled chunk for static loops and/or ordered loops */
  int static_current_chunk ;

  /* SINGLE CONSTRUCT */
  int current_single;				/* Which 'single' construct did we already go through? */

  /* ORDERED CONSTRUCT */
  int current_ordered_iteration ; 

};

typedef struct mpcomp_thread_s mpcomp_thread_t;

/******** OPENMP THREAD FATHER *********/
struct mpcomp_thread_father_s {
  int depth;                                    /* Depth of the current thread (0 =sequential region) */
  icv_t icvs;                                   /* Set of the ICVs for the child team */
  int rank;                                     /* OpenMP rank. 0 for master thread per team */
  int num_threads;                              /* Current number of threads in the team */
  struct mpcomp_instance_s *children_instance;  /* Instance for nested parallelism */
  void *hierarchical_tls;                       /* Local variables */

  /* DYNAMIC SCHEDULING */
  sctk_spinlock_t lock_for_dyn[MPCOMP_MAX_THREADS][MPCOMP_MAX_ALIVE_FOR_DYN];   /* Lock for dynamic scheduling. A lock for each loop */
  sctk_spinlock_t lock_stop_for_dyn;
  sctk_spinlock_t lock_exited_for_dyn[MPCOMP_MAX_ALIVE_FOR_DYN];                /* Lock for dynamic scheduling. A lock for each loop */

  volatile int stop[MPCOMP_MAX_ALIVE_FOR_DYN];
  volatile int nthread_exited_for_dyn[MPCOMP_MAX_ALIVE_FOR_DYN];
  volatile mpcomp_chunk_t chunk_info_for_dyn[MPCOMP_MAX_THREADS][MPCOMP_MAX_ALIVE_FOR_DYN];

  int private_current_for_dyn;

  /* SINGLE CONSTRUCT */
  sctk_spinlock_t lock_enter_single[MPCOMP_MAX_ALIVE_SINGLE];
  volatile int nb_threads_entered_single[MPCOMP_MAX_ALIVE_SINGLE];

};

typedef struct mpcomp_thread_father_s mpcomp_thread_father_t;


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
#warning "OpenMp compiling w/atomics"	    
  sctk_atomics_int barrier;				/* Barrier for the child team */
#else
#warning "OpenMp compiling without atomics"	    
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
void __mpcomp_thread_father_init(mpcomp_thread_father_t *father);
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

/*************************************************
*************************************************
	END: NEW MPCOMP STRUCTURES
*************************************************
**************************************************/

#ifdef __cplusplus
}
#endif
#endif
