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
  ****** STRUCTURES 
  *****************/

/******* ICV ********/
struct icv_s
{
  int nthreads_var;		/* Number of threads for the next team 
				   creation */
  int dyn_var;		/* Is dynamic thread adjustement on? */
  int nest_var;		/* Is nested OpenMP handled/allowed? */
  omp_sched_t run_sched_var;	/* Schedule to use when a 'schedule' clause is
				   set to 'runtime' */
  int modifier_sched_var;	/* Size of chunks for loop schedule */
  omp_sched_t def_sched_var;	/* Default schedule when no 'schedule' clause
			   is present */
  int nmicrovps_var;		/* Number of VPs */
};

typedef struct icv_s icv_t;

struct atomic_int_pad_s {
  sctk_atomics_int i ;
  char pad[8] ;
} ;
typedef struct atomic_int_pad_s atomic_int_pad ;


struct mpcomp_team_info_s {
  /* -- TEAM INFO -- */
  int depth;			/* Depth of the current thread (0 = sequential region) */

  /* -- SINGLE CONSTRUCT -- */
  int single_last_current ;
#if MPCOMP_USE_ATOMICS
  // sctk_atomics_int single_nb_threads_entered[ MPCOMP_MAX_ALIVE_SINGLE + 1 ] ;
  atomic_int_pad single_nb_threads_entered[ MPCOMP_MAX_ALIVE_SINGLE + 1 ] ;
#else
  sctk_spinlock_t single_lock_enter[MPCOMP_MAX_ALIVE_SINGLE + 1];
  volatile int single_nb_threads_entered[ MPCOMP_MAX_ALIVE_SINGLE + 1 ] ;
#endif
  volatile int single_first_copyprivate ;
  void * single_copyprivate_data ;
  volatile int single_nb_threads_stopped ;

  /* -- SECTION CONSTRUCT -- */

  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
  int for_dyn_last_current ;
  atomic_int_pad for_dyn_nb_threads_exited[ MPCOMP_MAX_ALIVE_FOR_DYN + 1 ] ;

  /* -- GUIDED FOR LOOP CONSTRUCT -- */

  /* Temp stats for chunk stealing */
  sctk_atomics_int stats_stolen_chunks; 
  sctk_atomics_int stats_last_mvp_chunk;
} ;

typedef struct mpcomp_team_info_s mpcomp_team_info ;

struct mpcomp_chunk_s {
  sctk_atomics_int total ;
  sctk_atomics_int remain ;
  // char pad[8] ;
} ;

typedef struct mpcomp_chunk_s mpcomp_chunk ;

/******* OPENMP THREAD ********/
struct mpcomp_thread_s {
  sctk_mctx_t uc;		/* Context (initializes registers, ...)
				   Initialized when another thread is blocked */
  icv_t icvs;			/* Set of ICVs for the child team */
  char *stack;		/* The stack (initialized when another thread is blocked) */
  long rank;			/* OpenMP rank (0 for master thread per team) */
  long num_threads;		/* Current number of threads in the team */
  struct mpcomp_mvp_s * mvp ;
  int index_in_mvp ;
  volatile int done ;
  struct mpcomp_instance_s * children_instance ; /* Instance for nested parallelism */

  struct mpcomp_team_info_s * team ; /* Information on the team */

  void *hierarchical_tls; /* Local variables */

  /* -- SINGLE CONSTRUCT -- */
  int single_current ;		/* Which 'single' construct did we already go through? */

  /* LOOP CONSTRUCT */
  int loop_lb;		/* Lower bound */
  int loop_b;			/* Upper bound */
  int loop_incr;		/* Step */
  int loop_chunk_size;	/* Size of each chunk */

  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
  int for_dyn_current ;	
  mpcomp_chunk for_dyn_chunk_info[ MPCOMP_MAX_ALIVE_FOR_DYN + 1 ] ;

  /* Infos for DFS */
  struct mpcomp_stack   *tree_stack;
  struct mpcomp_mvp_s   *stolen_mvp;
  int stolen_chunk_id;
  //struct mpcomp_node_s  *parent_node  
  int start_steal_chunk;
  
  //mpcomp_mvp_s **mvps; /* all mvps stores for chunk stealing (array of integers) */
#if 0
  mpcomp_stack_node_leaves for_dyn_stack ;
#endif

} ;

typedef struct mpcomp_thread_s mpcomp_thread ;

#if defined (SCTK_USE_OPTIMIZED_TLS)
	info->tls_module = sctk_tls_module;
#endif

#if defined (SCTK_USE_OPTIMIZED_TLS)
	sctk_tls_module_alloc_and_fill_in_specified_tls_module_with_specified_extls ( &info->tls_module, info->extls ) ;
#endif

/******* INSTANCE OF OPENMP ********/
struct mpcomp_instance_s {
  int nb_mvps;			/* Number of microVPs for this instance */
  struct mpcomp_mvp_s ** mvps ;	/* All microVPs of this instance */
  struct mpcomp_node_s * root ;	/* Root to the tree linking the microVPs */
} ;

typedef struct mpcomp_instance_s mpcomp_instance ;

/******* OPENMP MICRO VP ********/
struct mpcomp_mvp_s {
    /* Context including registers, stack pointer, ... */
    sctk_mctx_t vp_context;
    /* Thread ID */
    sctk_thread_t pid;

    int nb_threads ;
    int next_nb_threads ;
    struct mpcomp_thread_s threads[MPCOMP_MAX_THREADS_PER_MICROVP] ;
    /* array of rank in evey node of the tree */
    int * tree_rank ; 	
    /* Rank within the microVPs */
    int rank ;

    char pad0[64] ;

    volatile int enable ;

    char pad1[64] ;

    struct mpcomp_node_s * root ;

    char pad1_1[64] ;

    struct mpcomp_node_s * father ;

    char pad1_2[64] ;

    struct mpcomp_node_s * to_run ; /* Specify where the spin waiting for a new parallel region */
    char pad2[64] ;

    volatile int slave_running ;

    char pad3[64] ;

    void *(*func) (void *);	/* Function to call by every thread */
    void *shared;		/* Shared variables (for every thread) */

} ;

typedef struct mpcomp_mvp_s mpcomp_mvp ;

  typedef enum children_t {
    CHILDREN_NODE = 1,
    CHILDREN_LEAF = 2,
  } children_t ;

typedef enum context_t {
  MPCOMP_CONTEXT_IN_ORDER = 1,
  MPCOMP_CONTEXT_OUT_OF_ORDER_MAIN = 2,
  MPCOMP_CONTEXT_OUT_OF_ORDER_SUB = 3,
} context_t ;

/******* OPENMP INFO NODE ********/
  struct mpcomp_node_s {
    struct mpcomp_node_s * father ;
    long rank ; /* Rank among children of my father */
    int depth ; /* Depth in the tree */
    int nb_children ; /* Number of children */
    /* The following indices correspond to the 'rank' value in microVPs */
    int min_index ;  /* Flat min index of leaves in this subtree */
    int max_index ; /* Flat max index of leaves in this subtree */
    mpcomp_team_info * team_info ; /* Information on the whole team */
    children_t child_type ; /* Kind of children (node or leaf) */
    union node_or_leaf {
      struct mpcomp_node_s ** node ;
      struct mpcomp_mvp_s ** leaf ;
    } children ;
    sctk_spinlock_t lock;	/* Lock for structure updates */
    char pad0[64];
    volatile int slave_running ;
    char pad1[64];
#if MPCOMP_USE_ATOMICS
    sctk_atomics_int barrier;	/* Barrier for the child team */
    sctk_atomics_int chunks_avail; /* Flag for presence of chunks under current node */
    sctk_atomics_int nb_chunks_empty_children; /* Counter for presence of chunks */
#else
    volatile long barrier;	/* Barrier for the child team */
    volatile long chunks_avail; /* Flag for presence of chunks under current node */
    volatile long nb_chunks_empty_children; /* Counter for presence of chunks */
#endif
    char pad2[64];
    volatile long barrier_done;	/* Is the barrier (for the child team) over? */
    char pad3[64];
    volatile long barrier_num_threads;	/* Number of threads involved in the barrier */
    char pad4[64];

    int num_threads ; /* Number of threads in the current team */
    void *(*func) (void *);
    void *shared;

  } ;

  typedef struct mpcomp_node_s mpcomp_node ;

/************** BEGIN HEADER ****************/

struct mpcomp_stack {
  mpcomp_node ** elements ;
  int max_elements ;
  int n_elements ; /* corresponds to the head of the stack */
} ;

typedef struct mpcomp_stack mpcomp_stack ;



/************** END HEADER ****************/

/*****************
  ****** VARIABLES 
  *****************/


extern __thread void *sctk_openmp_thread_tls ;

/*****************
  ****** FUNCTIONS  
  *****************/

/* inline */

static inline void __mpcomp_team_info_init( mpcomp_team_info * team_info ) {
  int i ;

  /* -- TEAM INFO -- */
  team_info->depth = 0 ;

  /* -- SINGLE CONSTRUCT -- */
  team_info->single_last_current = MPCOMP_MAX_ALIVE_SINGLE ;
#if MPCOMP_USE_ATOMICS
  for ( i = 0 ; i < MPCOMP_MAX_ALIVE_SINGLE ; i++ ) {
    sctk_atomics_store_int( 
	&(team_info->single_nb_threads_entered[i].i), 0 ) ;
  }
  sctk_atomics_store_int( 
      &(team_info->single_nb_threads_entered[MPCOMP_MAX_ALIVE_SINGLE].i), MPCOMP_MAX_THREADS ) ;

  sctk_nodebug( "__mpcomp_team_info_init: Filling cell %d with %d", 
      MPCOMP_MAX_ALIVE_SINGLE, MPCOMP_MAX_THREADS ) ;
#else
  sctk_debug("__mpcomp_team_info_init: no atomics!");
  for ( i = 0 ; i < MPCOMP_MAX_ALIVE_SINGLE ; i++ ) {
    team_info->single_nb_threads_entered[i] = 0 ;
    team_info->single_lock_enter[ i ] = SCTK_SPINLOCK_INITIALIZER ;
  }
  team_info->single_nb_threads_entered[MPCOMP_MAX_ALIVE_SINGLE] = MPCOMP_NOWAIT_STOP_SYMBOL ;
  team_info->single_lock_enter[ MPCOMP_MAX_ALIVE_SINGLE ] = SCTK_SPINLOCK_INITIALIZER ;
#endif

  team_info->single_first_copyprivate = 0 ;
  team_info->single_copyprivate_data = NULL ;
  team_info->single_nb_threads_stopped = 0 ;

  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
  team_info->for_dyn_last_current = MPCOMP_MAX_ALIVE_FOR_DYN ;
  for ( i = 0 ; i < MPCOMP_MAX_ALIVE_FOR_DYN ; i++ ) {
    sctk_atomics_store_int( 
	&(team_info->for_dyn_nb_threads_exited[i].i), 0 ) ;
  }
  sctk_atomics_store_int( 
      &(team_info->for_dyn_nb_threads_exited[MPCOMP_MAX_ALIVE_FOR_DYN].i), 
      MPCOMP_NOWAIT_STOP_SYMBOL ) ;

 sctk_atomics_store_int( &(team_info->stats_stolen_chunks), 0);
 sctk_atomics_store_int( &(team_info->stats_last_mvp_chunk), 0);
}

static inline void __mpcomp_thread_init( mpcomp_thread * t, icv_t icvs, mpcomp_instance * instance,
    mpcomp_team_info * team_info ) {
  int i ;

  t->icvs = icvs ;
  t->rank = 0 ;
  t->num_threads = 1 ;
  t->mvp = NULL ;
  t->index_in_mvp = 0 ;
  t->done = 0 ;
  t->children_instance = instance ;
  t->team = team_info ;
  t->hierarchical_tls = NULL ;
  t->single_current = -1 ;

  /* -- DYNAMIC FOR LOOP CONSTRUCT -- */
  t->tree_stack = NULL;
  t->stolen_mvp = NULL;
  t->stolen_chunk_id = -1; //AMAHEO
  t->start_steal_chunk = -1; //AMAHEO
}

/* mpcomp.c */
void __mpcomp_init (void);
void __mpcomp_instance_init( mpcomp_instance * instance, int nb_mvps ) ;
void in_order_scheduler( mpcomp_mvp * mvp ) ;
void * mpcomp_slave_mvp_node( void * arg ) ;
void * mpcomp_slave_mvp_leaf( void * arg ) ;
void __mpcomp_internal_half_barrier() ;
void __mpcomp_internal_full_barrier() ;
void __mpcomp_barrier() ;

/* mpcomp_tree.c */
int __mpcomp_check_tree_parameters( int n_leaves, int depth, int * degree ) ;
int __mpcomp_build_default_tree( mpcomp_instance * instance ) ;
int __mpcomp_build_tree( mpcomp_instance * instance, int n_leaves, int depth, int * degree ) ;
int __mpcomp_check_tree_coherency( mpcomp_instance * instance ) ;
void __mpcomp_print_tree( mpcomp_instance * instance ) ;

/* mpcomp_loop_dyn.c */
int __mpcomp_dynamic_steal (int *from, int *to);
int __mpcomp_dynamic_loop_begin (int lb, int b, int incr,
			     int chunk_size, int *from, int *to);
int __mpcomp_dynamic_loop_next (int *from, int *to) ;
void __mpcomp_dynamic_loop_end () ;
void __mpcomp_dynamic_loop_end_nowait () ;

/* Stack primitives */
mpcomp_stack * __mpcomp_create_stack( int max_elements ) ;
int __mpcomp_is_stack_empty( mpcomp_stack * s ) ;
void __mpcomp_push( mpcomp_stack * s, mpcomp_node * n ) ;
mpcomp_node * __mpcomp_pop( mpcomp_stack * s ) ;
void __mpcomp_free_stack( mpcomp_stack * s ) ;

#ifdef __cplusplus
}
#endif
#endif
