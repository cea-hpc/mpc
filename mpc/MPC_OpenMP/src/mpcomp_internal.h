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

/* Max size of each stack */
/* TODO Make it dynamic to be compilant w/ OpenMP 3.0? */
#define STACK_SIZE 16*1024

/* Maximum number of threads for each team of a parallel region */
#define MPCOMP_MAX_THREADS		64
/* Maximum number of shared for loops w/ dynamic schedule alive */
#define MPCOMP_MAX_ALIVE_FOR_DYN	7
/* Maximum number of shared for loops w/ guided schedule alive */
#define MPCOMP_MAX_ALIVE_FOR_GUIDED	3
/* Maximum number of alive 'sections' construct */
#define MPCOMP_MAX_ALIVE_SECTIONS	3
/* Maximum number of alive 'single' construct */
#define MPCOMP_MAX_ALIVE_SINGLE		3

#define MPCOMP_NOWAIT_STOP_SYMBOL	(-1)
#define MPCOMP_NOWAIT_STOP_CONSUMED	(-2)
#define MPCOMP_NOWAIT_OK_SYMBOL		(-3)

/*
#define MPCOMP_LOCK_INIT {0,0,NULL,NULL,NULL,SCTK_THREAD_MUTEX_INITIALIZER}
*/
#define MPCOMP_LOCK_INIT {0,0,NULL,NULL,NULL,SCTK_SPINLOCK_INITIALIZER}


  struct mpcomp_chunk_s 
  {
    int remain ;
    int total ;
    char pad[7] ;
  } ;
  typedef struct mpcomp_chunk_s mpcomp_chunk_t ;

  /* Internal Control Variables
   * - See OpenMP API 2.5, Section 2.3 
   */
  struct icv_s
  {
    int nthreads_var;		/* Number of threads for the next team 
				   creation */
    int dyn_var;		/* Is dynamic thread adjustement on? */
    int nest_var;		/* Is nested OpenMP handled/allowed? */
    /* TODO change run_sched_var and def_sched_var to integers 
       -> See OpenMP 3.0 for the corresponding numbers */
    omp_sched_t run_sched_var;	/* Schedule to use when a 'schedule' clause is
				   set to 'runtime' */
    int modifier_sched_var;	/* Size of chunks for loop schedule */
    char *def_sched_var;	/* Default schedule when no 'schedule' clause
				   is present */
    int nmicrovps_var;		/* Number of VPs */
    /* TODO handle chunk size for runtime schedule? */
  };

  typedef struct icv_s icv_t;

  struct mpcomp_thread_info_s
  {
    /************** PRIVATE INFO FOR THE CURRENT THREAD ****************/

    sctk_mctx_t uc;		/* Context (initializes registers, ...)
				   Initialized when another thread is blocked */
    void *(*func) (void *);	/* Function to call */
    void *shared;		/* Shared variables */
    long rank;			/* OpenMP rank (0 for master thread per team) */
    long num_threads;		/* Current number of threads in the team */
    icv_t icvs;			/* Set of ICVs for the child team */
    int depth;			/* Depth of the current thread (0 = sequential region) */
    long vp;			/* Index of the corresponding VP */
    volatile long barrier;	/* Barrier for the child team */
    volatile long barrier_done;	/* Is the barrier (for the child team) over? */
    struct mpcomp_thread_info_s *father;	/* Father region or NULL for
						   sequential (depth=0) */
    long step;			/* Index of the current execution in the VP */
    char *stack;		/* The stack (initialized when another thread is blocked) */
    volatile long done;		/* Is the work over for this micro thread */
    volatile long context;	/* Is the context allocated (stack, ...)
				   This is a long probably for alignment reasons */
    // sctk_thread_mutex_t lock;	/* Mutex for structure updates */
    /* TODO TEMP lock2 -> better perf than mutex for barrier */
    sctk_spinlock_t lock2;	/* Mutex for structure updates */
    char pad_l2_in[ 8 ] ;
    volatile long is_running;	/* Is this thread currently scheduled (i.e., non
				   locked)? */

    char pad_l2_out[ 8 ] ;
    /* TODO add macros for 'running'? */
    struct sctk_microthread_s *task;	/* Corresponding microthread "task" */

    void *extls;
#if defined (SCTK_USE_OPTIMIZED_TLS)
	void *tls_module;
#endif

    /* Private info on the current loop (whatever its schedule is)  */
    int loop_lb;		/* Lower bound */
    int loop_b;			/* Upper bound */
    int loop_incr;		/* Step */
    int loop_chunk_size;	/* Size of each chunk */

    /* -- FOR LOOPS - STATIC SCHEDULE -- */
    int static_nb_chunks;

    /* What is the currently scheduled chunk for static loops and/or ordered loops */
    int static_current_chunk ;

    /* -- SINGLE CONSTRUCT -- */
    int current_single;		/* Which 'single' construct did we already go through? */

    /* -- ORDERED CONSTRUCT -- */
    int current_ordered_iteration ; 

    /************** SHARED INFO FOR THE CHILD TEAM ****************/


    struct mpcomp_thread_info_s *children[MPCOMP_MAX_THREADS];	/*
								   Information
								   on
								   children
								 */

    /*
       TODO Single locks for every workshare?

       // These locks can be dimensioned with the number of microVPs
       sctk_spinlock_t lock_workshare[MPCOMP_MAX_THREADS ][MPCOMP_MAX_ALIVE_WORKSHARE+1] ;

       sctk_spinlock_t lock_stop ;

       // Need padding
       sctk_spinlock_t lock_exited[ MPCOMP_MAX_ALIVE_WORKSHARE+1 ] ;

       // Need padding
       volatile int stop[ MPCOMP_MAX_ALIVE_WORKSHARE+1 ] ;

       // Need padding
       volatile mpcomp_chunk_t chunk_info[ MPCOMP_MAX_THREADS ][ MPCOMP_MAX_ALIVE_WORKSHARE+1 ] ;

       // Need padding
       volatile int nb_threads_exited[ MPCOMP_MAX_ALIVE_WORKSHARE+1 ] ;


     */




    /* -- SINGLE CONSTRUCT -- */
    sctk_spinlock_t lock_enter_single[MPCOMP_MAX_ALIVE_SINGLE + 1];
    volatile int nb_threads_entered_single[MPCOMP_MAX_ALIVE_SINGLE + 1];
    void *single_copyprivate_data;
    volatile int single_first_copyprivate;
    sctk_spinlock_t lock_single_copyprivate ;

    /* -- ORDERED CONSTRUCT -- */
    volatile int next_ordered_offset ; 

    /* -- FOR LOOPS - DYNAMIC SCHEDULE -- */
    /* PUBLIC */
    /* TODO check the alignment of every value to avoid false sharing */
    sctk_spinlock_t lock_for_dyn[MPCOMP_MAX_THREADS][MPCOMP_MAX_ALIVE_FOR_DYN+1
      + 64 ] ;
    char foo_dyn_3[ 64 ] ;
    sctk_spinlock_t lock_stop_for_dyn ;
    char foo_dyn_4[ 64 ] ;
    sctk_spinlock_t lock_exited_for_dyn[MPCOMP_MAX_ALIVE_FOR_DYN+1] ;
    char foo_dyn_5[ 64 ] ;
    volatile int stop_index_for_dyn ;
    volatile int stop_value_for_dyn ;
    char foo_dyn_6[ 64 ] ;
    /* -> Already exist */
    volatile int nb_threads_exited_for_dyn[MPCOMP_MAX_ALIVE_FOR_DYN + 1]; 
    volatile mpcomp_chunk_t chunk_info_for_dyn[ MPCOMP_MAX_THREADS ][ MPCOMP_MAX_ALIVE_FOR_DYN+1
      + 64 ] ;

    /* PRIVATE */
    int private_current_for_dyn ;
      char foo_dyn2[ 64 ] ;


    /* -- FOR LOOPS - GUIDED SCHEDULE -- */
    sctk_spinlock_t lock_enter_for_guided[MPCOMP_MAX_ALIVE_FOR_GUIDED + 1];
    volatile int current_for_guided[MPCOMP_MAX_THREADS];	/* W:per thread R:per team */
    volatile int nb_threads_entered_for_guided[MPCOMP_MAX_ALIVE_FOR_GUIDED +
					       1];
    /* W:per team R:per team */
    volatile int nb_threads_exited_for_guided[MPCOMP_MAX_ALIVE_FOR_GUIDED +
					      1];
    /* W:per team R:per team */
    volatile int nb_iterations_remaining[MPCOMP_MAX_ALIVE_FOR_GUIDED + 1];
    volatile int current_from_for_guided[MPCOMP_MAX_ALIVE_FOR_GUIDED + 1];

    /* -- SECTIONS/SECTION CONSTRUCT -- */
    sctk_spinlock_t lock_enter_sections[MPCOMP_MAX_ALIVE_SECTIONS + 1];
    /* TODO put this variable in children? */
    volatile int current_sections[MPCOMP_MAX_THREADS];
    volatile int next_section[MPCOMP_MAX_ALIVE_SECTIONS + 1];
    volatile int nb_sections[MPCOMP_MAX_ALIVE_SECTIONS + 1];
    volatile int nb_threads_exited_sections[MPCOMP_MAX_ALIVE_SECTIONS + 1];


    /* To force a right aligment on ia64 architecture */
    /* + padding */
    /* TODO check the alignment on every architecture */
    char foo2[8];
  };

/* Main structure to retrieve information on the current OpenMP thread */
  typedef struct mpcomp_thread_info_s mpcomp_thread_info_t;

  /* Initialization of a thread-info structure */
  /* TODO update according to new structures (for dyn, guided, ...) */
  static inline void __mpcomp_init_thread_info
    (struct mpcomp_thread_info_s *info,
     void *(*func) (void *),
     void *shared,
     long rank,
     long num_threads,
     icv_t icvs,
     int depth,
     long vp, long barrier, struct mpcomp_thread_info_s *father, long step,
     sctk_microthread_t * task)
  {
    int i, j;
    int keep[sctk_extls_max_scope];

      info->func = func;
      info->shared = shared;
      info->rank = rank;
      info->num_threads = num_threads;
      info->icvs = icvs;
      info->depth = depth;
      info->vp = vp;
      info->barrier = barrier;
      info->barrier_done = 0;
      info->father = father;
      info->step = step;
      info->done = 0;
      info->context = 0;
      // info->lock = init;
    /* TODO TEMP LOCK2 */
      info->lock2 = SCTK_SPINLOCK_INITIALIZER;
      info->is_running = 1;
      info->stack = NULL;
      info->task = task;

    /* TLS */
    if (rank == 0)
      {
	sctk_nodebug ("__mpcomp_init_thread_info: keep everything \n");
	info->extls = sctk_extls;

#if defined (SCTK_USE_OPTIMIZED_TLS)
	info->tls_module = sctk_tls_module;
#endif
      }
    else
      {
	sctk_extls_duplicate (&info->extls);
	/* memset (keep, 1, sctk_tls_max_scope * sizeof (int)); */
	for (i = 0; i < sctk_extls_max_scope; i++)
	  {
	    keep[i] = 1;
	  }
	sctk_nodebug ("__mpcomp_init_thread_info: keep[1] = %d\n", keep[1]);
	keep[sctk_extls_openmp_scope] = 0;
	sctk_extls_keep_with_specified_extls (info->extls, keep);

#if defined (SCTK_USE_OPTIMIZED_TLS)
	sctk_tls_module_alloc_and_fill_in_specified_tls_module_with_specified_extls ( &info->tls_module, info->extls ) ;
#endif
      }

#if 0
    sctk_debug ("__mpcomp_init_thread_info: first omp tls = %p\n",
		__sctk__tls_get_addr__openmp_scope (0, 0));
#endif


    /* SINGLE */
    info->current_single = -1;
    info->single_first_copyprivate = 0;
    info->lock_single_copyprivate = SCTK_SPINLOCK_INITIALIZER ;

    /* ORDERED */
    info->next_ordered_offset = -1 ;

    /* TODO BEGIN NEW */
    info->private_current_for_dyn = 0 ;
    info->lock_stop_for_dyn = SCTK_SPINLOCK_INITIALIZER ;
    for (i = 0; i < MPCOMP_MAX_THREADS; i++) {
      for ( j = 0 ; j <= MPCOMP_MAX_ALIVE_FOR_DYN ; j++ ) {
	info->chunk_info_for_dyn[i][j].remain = -1 ;
	info->lock_for_dyn[i][j] = SCTK_SPINLOCK_INITIALIZER ;
      }
    }

    for (i = 0; i <= MPCOMP_MAX_ALIVE_FOR_DYN; i++)
    {
      info->nb_threads_exited_for_dyn[i] = 0 ;
      info->lock_exited_for_dyn[i] = SCTK_SPINLOCK_INITIALIZER ;
    }
    info->stop_index_for_dyn = MPCOMP_MAX_ALIVE_FOR_DYN ;
    info->stop_value_for_dyn = MPCOMP_NOWAIT_STOP_SYMBOL ;
    /* TODO END NEW */

    for (i = 0; i < MPCOMP_MAX_THREADS; i++)
      {
	info->children[i] = NULL;
	info->current_for_guided[i] = -1;
	info->current_sections[i] = -1;
      }

    for (i = 0; i <= MPCOMP_MAX_ALIVE_FOR_GUIDED; i++)
      {
	info->lock_enter_for_guided[i] = SCTK_SPINLOCK_INITIALIZER;
	info->nb_threads_entered_for_guided[i] = 0;
      }
    info->nb_threads_entered_for_guided[MPCOMP_MAX_ALIVE_FOR_GUIDED] = -1;

    for (i = 0; i <= MPCOMP_MAX_ALIVE_SECTIONS; i++)
      {
	info->next_section[i] = 0;
	info->nb_sections[i] = 0;
	info->nb_threads_exited_sections[i] = 0;
	info->lock_enter_sections[i] = SCTK_SPINLOCK_INITIALIZER;
      }
    info->next_section[MPCOMP_MAX_ALIVE_SECTIONS] = -1;

    for (i = 0; i <= MPCOMP_MAX_ALIVE_SINGLE; i++)
      {
	info->lock_enter_single[i] = SCTK_SPINLOCK_INITIALIZER;
	info->nb_threads_entered_single[i] = 0;
      }
    info->nb_threads_entered_single[MPCOMP_MAX_ALIVE_SINGLE] = -1;

  }

  /* RESET INFO FOR DYNAMIC SCHEDULE */

  static inline void __mpcomp_reset_dynamic_thread_info
    (struct mpcomp_thread_info_s *info)
  {
    // info->current_for_dyn = 0 ;
  }

  static inline void __mpcomp_reset_dynamic_team_info
    (struct mpcomp_thread_info_s *info, int num_threads)
  {
    int i;
    int next_index = (info->children[0]->private_current_for_dyn)%(MPCOMP_MAX_ALIVE_FOR_DYN+1) ;

    if ( next_index != -1 ) {

      for ( i = 0 ; i < MPCOMP_MAX_THREADS ; i++ ) {
	info->chunk_info_for_dyn[i][next_index].remain = -1 ;
      }
    }

#if 0
    /* TODO update '-1' to the corresponding macro */

    for (i = 0; i < num_threads; i++)
      {
	info->current_for_dyn[i] = -1;
      }

    /* Reset only the number of entered threads */
    for (i = 0; i <= MPCOMP_MAX_ALIVE_FOR_DYN; i++)
      {
	info->nb_threads_entered_for_dyn[i] = 0;
      }
    info->nb_threads_entered_for_dyn[MPCOMP_MAX_ALIVE_FOR_DYN] = -1;
#endif
  }

  /* RESET INFO FOR GUIDED SCHEDULE */
  /* TODO */
  /* RESET INFO FOR SECTIONS CONSTRUCT */
  /* TODO */
  /* RESET INFO FOR ORDERED CONSTRUCT */
  /* TODO */
  /* RESET INFO FOR DYNAMIC SCHEDULE */
  /* TODO */
  /* RESET INFO FOR DYNAMIC SCHEDULE */
  /* TODO */

  /* Reinitialization of shared parameters after executing a workshare region
   */
  static inline void __mpcomp_reset_team_info
    (struct mpcomp_thread_info_s *info, int num_threads)
  {
    int i;

    info->single_first_copyprivate = 0;

    __mpcomp_reset_dynamic_team_info( info, num_threads ) ;

    /* TODO update '-1' to the corresponding macro */

    for (i = 0; i < num_threads; i++)
      {
	info->current_sections[i] = -1;
	info->current_for_guided[i] = -1;
	info->children[i]->current_single = -1;
      }

    /* TODO check what really needs to be updated w/ sections */
    for (i = 0; i <= MPCOMP_MAX_ALIVE_SECTIONS; i++)
      {
	info->next_section[i] = 0;
	info->nb_sections[i] = 0;
	info->nb_threads_exited_sections[i] = 0;
      }
    info->next_section[MPCOMP_MAX_ALIVE_SECTIONS] = -1;

    /* Reset only the number of entered threads */
    for (i = 0; i <= MPCOMP_MAX_ALIVE_FOR_GUIDED; i++)
      {
	info->nb_threads_entered_for_guided[i] = 0;
      }
    info->nb_threads_entered_for_guided[MPCOMP_MAX_ALIVE_FOR_GUIDED] = -1;

    /* Reset only the number of entered threads */
    for (i = 0; i <= MPCOMP_MAX_ALIVE_SINGLE; i++)
      {
	info->nb_threads_entered_single[i] = 0;
      }
    info->nb_threads_entered_single[MPCOMP_MAX_ALIVE_SINGLE] = -1;
  }


  static inline void __mpcomp_reset_guided_team_info
    (struct mpcomp_thread_info_s *info, int num_threads)
  {
    int i;

    /* TODO update '-1' to the corresponding macro */

    for (i = 0; i < num_threads; i++)
      {
	info->current_for_guided[i] = -1;
      }

    /* Reset only the number of entered threads */
    for (i = 0; i <= MPCOMP_MAX_ALIVE_FOR_GUIDED; i++)
      {
	info->nb_threads_entered_for_guided[i] = 0;
      }
    info->nb_threads_entered_for_guided[MPCOMP_MAX_ALIVE_FOR_GUIDED] = -1;
  }

  static inline void __mpcomp_reset_sections_team_info
    (struct mpcomp_thread_info_s *info, int num_threads)
  {
    int i;

    /* TODO update '-1' to the corresponding macro */

    for (i = 0; i < num_threads; i++)
      {
	info->current_sections[i] = -1;
      }

    /* TODO check what really needs to be updated w/ sections */
    for (i = 0; i <= MPCOMP_MAX_ALIVE_SECTIONS; i++)
      {
	info->next_section[i] = 0;
	info->nb_sections[i] = 0;
	info->nb_threads_exited_sections[i] = 0;
      }
    info->next_section[MPCOMP_MAX_ALIVE_SECTIONS] = -1;
  }

  static inline void __mpcomp_reset_single_team_info
    (struct mpcomp_thread_info_s *info, int num_threads)
  {
    int i;

    info->single_first_copyprivate = 0;

    /* TODO update '-1' to the corresponding macro */

    for (i = 0; i < num_threads; i++)
      {
	info->children[i]->current_single = -1;
      }

    /* Reset only the number of entered threads */
    for (i = 0; i <= MPCOMP_MAX_ALIVE_SINGLE; i++)
      {
	info->nb_threads_entered_single[i] = 0;
      }
    info->nb_threads_entered_single[MPCOMP_MAX_ALIVE_SINGLE] = -1;
  }

  static inline void __mpcomp_reset_thread_info
    (struct mpcomp_thread_info_s *info,
     void *(*func) (void *),
     void *shared, long num_threads, icv_t icvs, long barrier, long step,
     long vp)
  {
    info->func = func;
    info->shared = shared;

    info->num_threads = num_threads;
    info->icvs = icvs;
    info->vp = vp;
    info->barrier = barrier;
    info->barrier_done = 0;
    info->step = step;
    // info->stack = NULL;
    info->done = 0;
    info->context = 0;
    info->is_running = 1;

    info->current_single = -1;

    /* TODO BEGIN NEW */
    info->private_current_for_dyn = 0 ; 
    /* TODO END NEW */

  }

  /* Linked list of locks 
     (per lock structure, who is blocked with this lock)
   */
  typedef struct mpcomp_slot_s
  {
    struct mpcomp_thread_info_s *thread;
    struct mpcomp_slot_s *next;
  } mpcomp_slot_t;

/* OpenMP information of the current thread */
  extern sctk_thread_key_t mpcomp_thread_info_key;


/* mpcomp.c */
  void __mpcomp_init (void);
  void mpcomp_fork_when_blocked (sctk_microthread_vp_t * self, long step);
  void mpcomp_macro_scheduler (sctk_microthread_vp_t * self, long step);
  void *__mpcomp_wrapper_op (void *arg);
  void __mpcomp_old_barrier (void);

/* mpcomp_checkpoint.c */

/* mpcomp_fortran.c */

/* mpcomp_lock.c */

/* mpcomp_loop.c */
  int __mpcomp_get_static_nb_chunks_per_rank (int rank, int nb_threads,
					      int lb, int b, int incr,
					      int chunk_size);
  void __mpcomp_get_specific_chunk_per_rank (int rank, int nb_threads, int lb,
					     int b, int incr, int chunk_size,
					     int chunk_num, int *from,
					     int *to);

/* mpcomp_loop_dyn.c */

/* mpcomp_loop_guided.c */
int __mpcomp_guided_loop_begin (int lb, int b, int incr, int chunk_size, int *from, int *to) ;
int __mpcomp_guided_loop_next (int *from, int *to) ;
void __mpcomp_guided_loop_end () ;
void __mpcomp_guided_loop_end_nowait () ;
void __mpcomp_start_parallel_guided_loop (int arg_num_threads, void *(*func)
				     (void *), void *shared, int lb, int b,
				     int incr, int chunk_size) ;
int __mpcomp_guided_loop_begin_ignore_nowait (int lb, int b, int incr, int
					  chunk_size, int *from, int *to) ;
int __mpcomp_guided_loop_next_ignore_nowait (int *from, int *to) ;

/* mpcomp_loop_runtime.c */

/* mpcomp_loop_static.c */

/* mpcomp_sections.c */
int __mpcomp_sections_begin (int nb_sections) ;
int __mpcomp_sections_next () ;
void __mpcomp_sections_end () ;
void __mpcomp_sections_end_nowait () ;

/* mpcomp_single.c */

/* mpcomp_sync.c */


#ifdef __cplusplus
}
#endif
#endif
