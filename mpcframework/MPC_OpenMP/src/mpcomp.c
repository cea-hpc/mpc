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
#include "sctk_runtime_config.h"
#include "mpcomp_internal.h"
#include <sys/time.h>
#include <ctype.h>

/*****************
  ****** GLOBAL VARIABLES 
  *****************/

/*
  Avoid mix of MPC with Intel and GCC OpenMP runtimes
 */

#define ABORT_FUNC_OMP(a,b)			\
  void a(){					\
    sctk_error(b);				\
    abort();					\
  }

ABORT_FUNC_OMP(__kmpc_for_static_init_4,"Mix Intel OpenMP runtime with MPC")
ABORT_FUNC_OMP(GOMP_parallel_start,"Mix GCC OpenMP runtime with MPC")

/* Schedule type */
static omp_sched_t OMP_SCHEDULE = 1;
/* Schedule modifier */
static int OMP_MODIFIER_SCHEDULE = 0;
/* Defaults number of threads */
static int OMP_NUM_THREADS = 0;
/* Is dynamic adaptation on? */
static int OMP_DYNAMIC = 0;
/* Is nested parallelism handled or flaterned? */
static int OMP_NESTED = 0;
/* Number of VPs for each OpenMP team */
static int OMP_MICROVP_NUMBER = 0;
/* Default tree for OpenMP instances? (number of nodes per level) */
static int * OMP_TREE = NULL ;
/* Number of level for the default tree (size of OMP_TREE) */
static int OMP_TREE_DEPTH = 0 ;
/* Total number of leaves for the tree (product of OMP_TREE elements) */
static int OMP_TREE_NB_LEAVES = 0 ;
/* Is thread binding enabled or not */
static int OMP_PROC_BIND = 0;
/* Size of the thread stack size */
static int OMP_STACKSIZE = 0;
/* Behavior of waiting threads */
static int OMP_WAIT_POLICY = 0;
/* Maximum number of threads participing in the whole program */
static int OMP_THREAD_LIMIT = 0;
/* Maximum number of nested active parallel regions */
static int OMP_MAX_ACTIVE_LEVELS = 0;
#if MPCOMP_TASK
/* Depth of the new tasks lists in the tree */
static int OMP_NEW_TASKS_DEPTH = 0;
/* Depth of the untied tasks lists in the tree */
static int OMP_UNTIED_TASKS_DEPTH = 0;
/* Task stealing policy */
static int OMP_TASK_LARCENY_MODE = MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL;
/* Task max depth in task generation */
static int OMP_TASK_NESTING_MAX = 8;
#endif /* MPCOMP_TASK */
/* Should we emit a warning when nesting is used? */
static int OMP_WARN_NESTED = 0 ;
/* Hybrid MPI/OpenMP mode */
static mpcomp_mode_t OMP_MODE = MPCOMP_MODE_SIMPLE_MIXED ;

mpcomp_global_icv_t mpcomp_global_icvs;



/*****************
  ****** FUNCTIONS  
  *****************/

TODO(" function __mpcomp_tokenizer is inspired from sctk_launch.c. Need to merge")
static char ** __mpcomp_tokenizer( char * string_to_tokenize, int * nb_tokens ) {
    /*    size_t len;*/
    char *cursor;
    int i;
    char **new_argv;
    int new_argc = 0;

    /* TODO check sizes of every malloc... */

    new_argv = (char **) sctk_malloc ( 32 * sizeof (char *));
    sctk_assert( new_argv != NULL ) ;

    cursor = string_to_tokenize ;

    while (*cursor == ' ')
      cursor++;
    while (*cursor != '\0')
    {
      int word_len = 0;
      new_argv[new_argc] = (char *) sctk_malloc (1024 * sizeof (char));
      while ((word_len < 1024) && (*cursor != '\0') && (*cursor != ' '))
      {
        new_argv[new_argc][word_len] = *cursor;
        cursor++;
        word_len++;
      }
      new_argv[new_argc][word_len] = '\0';
      new_argc++;
      while (*cursor == ' ')
        cursor++;
    }
    new_argv[new_argc] = NULL;

    *nb_tokens = new_argc ;
    return new_argv ;

}

static inline void __mpcomp_read_env_variables() {
  char * env ;
  int nb_threads ;

  sctk_nodebug ("__mpcomp_read_env_variables: Read env vars (MPC rank: %d)",
      sctk_get_task_rank ());

  /******* OMP_VP_NUMBER *********/
  OMP_MICROVP_NUMBER = sctk_runtime_config_get()->modules.openmp.vp;

  if ( OMP_MICROVP_NUMBER < 0 ) {
      fprintf( stderr, 
	  "Warning: Incorrect number of microVPs (OMP_MICROVP_NUMBER=<%d>) -> "
	       "Switching to default value %d\n", OMP_MICROVP_NUMBER, 0 ) ;
      OMP_MICROVP_NUMBER = 0 ;
  }
  if ( OMP_MICROVP_NUMBER > sctk_get_processor_number() ) {
      fprintf( stderr,
	  "Warning: Number of microVPs should be at most the number of cores per node: %d\n"
	  "Switching to default value %d\n", sctk_get_processor_number (), OMP_MICROVP_NUMBER ) ;
      OMP_MICROVP_NUMBER = 0 ;
  }



  /******* OMP_SCHEDULE *********/
  env = sctk_runtime_config_get()->modules.openmp.schedule;
  OMP_SCHEDULE = 1 ;	/* DEFAULT */
  if (env != NULL)
  {
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
	    fprintf (stderr,
		"Warning: Incorrect chunk size within OMP_SCHEDULE variable: <%s>\n", env ) ;
	    chunk_size = 0 ;
	  } else {
	    OMP_MODIFIER_SCHEDULE = chunk_size ;
	  }
	  break ;
	case '\0':
	  sctk_nodebug( "No chunk size\n" ) ;
	  break ;
	default:
	  fprintf (stderr,
	      "Syntax error with environment variable OMP_SCHEDULE <%s>,"
	      " should be \"static|dynamic|guided|auto[,chunk_size]\"\n", env ) ;
	  exit( 1 ) ;
      }
    }  else {
      fprintf (stderr,
	  "Warning: Unknown schedule <%s> (must be static, guided, dynamic or auto),"
	  " fallback to default schedule <%d>\n", env,
	  OMP_SCHEDULE);
    }
  }


  /******* OMP_NUM_THREADS *********/
  OMP_NUM_THREADS = sctk_runtime_config_get()->modules.openmp.nb_threads;
  if ( OMP_NUM_THREADS < 0 ) {
	  OMP_NUM_THREADS = 0 ;
  }
TODO( "If OMP_NUM_THREADS is 0, let it equal to 0 by default and handle it later" )
  if ( OMP_NUM_THREADS == 0 ) {
	  OMP_NUM_THREADS = OMP_MICROVP_NUMBER;	/* DEFAULT */
  }
  TODO( "OMP_NUM_THREADS: need to handle x,y,z,... and keep only x" )


  /******* OMP_DYNAMIC *********/
  OMP_DYNAMIC = sctk_runtime_config_get()->modules.openmp.adjustment ? 1 : 0;

  /******* OMP_PROC_BIND *********/
  OMP_DYNAMIC = sctk_runtime_config_get()->modules.openmp.proc_bind ? 1 : 0;

  /******* OMP_NESTED *********/
  OMP_NESTED = sctk_runtime_config_get()->modules.openmp.nested ? 1 : 0;

  /******* OMP_STACKSIZE *********/
  OMP_STACKSIZE = sctk_runtime_config_get()->modules.openmp.stack_size ;
  if ( OMP_STACKSIZE  == 0 ) {
	  if (sctk_is_in_fortran == 1)
		  OMP_STACKSIZE = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
	  else
		  OMP_STACKSIZE = SCTK_ETHREAD_STACK_SIZE;
  } 

  /******* OMP_WAIT_POLICY *********/
  OMP_WAIT_POLICY = sctk_runtime_config_get()->modules.openmp.wait_policy ;
  
  /******* OMP_THREAD_LIMIT *********/
  OMP_THREAD_LIMIT = sctk_runtime_config_get()->modules.openmp.thread_limit ;

  /******* OMP_MAX_ACTIVE_LEVELS *********/
  OMP_MAX_ACTIVE_LEVELS = sctk_runtime_config_get()->modules.openmp.max_active_levels ;


  /******* ADDITIONAL VARIABLES *******/

  /******* OMP_WARN_NESTED *******/
  OMP_WARN_NESTED = sctk_runtime_config_get()->modules.openmp.warn_nested ;

  /******* OMP_TREE *********/
  env = sctk_runtime_config_get()->modules.openmp.tree ;
  
  if (strlen( env) != 0)
  {
    int nb_tokens = 0 ;
    char ** tokens = NULL ;
    int i ;

    tokens = __mpcomp_tokenizer( env, &nb_tokens ) ;
    sctk_assert( tokens != NULL ) ;

#if 0
    fprintf( stderr, "OMP_TREE: Found %d token(s)\n", nb_tokens ) ;

    for ( i = 0 ; i < nb_tokens ; i++ ) {
      fprintf( stderr, "OMP_TREE\tToken %d -> <%s>\n", i, tokens[i] ) ;
    }
#endif

    /* TODO check that arguments are ok and #leaves is correct */

    OMP_TREE = (int *)sctk_malloc( nb_tokens * sizeof( int ) ) ;
    OMP_TREE_DEPTH = nb_tokens ;
    OMP_TREE_NB_LEAVES = 1 ;
    for ( i = 0 ; i < nb_tokens ; i++ ) {
      OMP_TREE[i] = atoi( tokens[ i ] ) ;
      OMP_TREE_NB_LEAVES *= OMP_TREE[i] ;
    }

#if 0
    fprintf( stderr, "OMP_TREE: tree w/ %d level(s)\n", OMP_TREE_DEPTH ) ;

    for ( i = 0 ; i < nb_tokens ; i++ ) {
      fprintf( stderr, "OMP_TREE\tLevel %d -> %d children\n", i, OMP_TREE[i] ) ;
    }
#endif
	TODO( "check the env variable OMP_TREE" )

  } else {
	  OMP_TREE = NULL ;
  }


#if MPCOMP_TASK
  /******* OMP_NEW_TASKS_DEPTH *********/
  env = getenv ("OMP_NEW_TASKS_DEPTH");
  if (env != NULL)
  {
       OMP_NEW_TASKS_DEPTH = strtol(env, NULL, 10);       
  }

  /******* OMP_UNTIED_TASKS_DEPTH *********/
  env = getenv ("OMP_UNTIED_TASKS_DEPTH");
  if (env != NULL)
  {
       OMP_UNTIED_TASKS_DEPTH = strtol(env, NULL, 10);              
  }

  /******* OMP_TASK_LARCENY_MODE *********/
  env = getenv ("OMP_TASK_LARCENY_MODE");
  if (env != NULL)
  {
       OMP_TASK_LARCENY_MODE = strtol(env, NULL, 10);              
  }

  /******* OMP_TASK_NESTING_MAX *********/
  env = getenv ("OMP_TASK_NESTING_MAX");
  if (env != NULL)
  {
       OMP_TASK_NESTING_MAX = strtol(env, NULL, 10);              
  }
#endif /* MPCOMP_TASK */

  /***** PRINT SUMMARY (only first MPI rank) ******/
  if (getenv ("MPC_DISABLE_BANNER") == NULL && sctk_process_rank == 0) {
    fprintf (stderr,
	"MPC OpenMP version %d.%d\n",
	SCTK_OMP_VERSION_MAJOR, SCTK_OMP_VERSION_MINOR);
#if MPCOMP_TASK
    fprintf (stderr, "\tTasking on\n" ) ;
#else
    fprintf (stderr, "\tTasking off\n" ) ;
#endif
    fprintf (stderr, "\tOMP_SCHEDULE %d\n", OMP_SCHEDULE);
	if ( OMP_NUM_THREADS == 0 ) {
		fprintf (stderr, "\tDefault #threads (OMP_NUM_THREADS)\n");
	} else {
		fprintf (stderr, "\tOMP_NUM_THREADS %d\n", OMP_NUM_THREADS);
	}
    fprintf (stderr, "\tOMP_DYNAMIC %d\n", OMP_DYNAMIC);
    fprintf (stderr, "\tOMP_NESTED %d\n", OMP_NESTED);
	if ( OMP_MICROVP_NUMBER == 0 ) {
		fprintf (stderr, "\tDefault #microVPs (OMP_MICROVP_NUMBER)\n" );
	} else {
		fprintf (stderr, "\t%d microVPs (OMP_MICROVP_NUMBER)\n", OMP_MICROVP_NUMBER);
	}
	if ( OMP_TREE != NULL ) {
	  int i ;
	  fprintf( stderr, "\tOMP_TREE w/ depth:%d leaves:%d, arity:[%d", 
		  OMP_TREE_DEPTH, OMP_TREE_NB_LEAVES, OMP_TREE[0] ) ;
	  for ( i = 1 ; i < OMP_TREE_DEPTH ; i++ ) {
		fprintf( stderr, ", %d", OMP_TREE[i] ) ;
	  }
	  fprintf( stderr, "]\n" ) ;
	} else {
	  fprintf( stderr, "\tOMP_TREE default\n" ) ;
	}
#if MPCOMP_MALLOC_ON_NODE
    fprintf( stderr, "\tNUMA allocation for tree nodes\n" ) ;
#endif
#if MPCOMP_COHERENCY_CHECKING
    fprintf( stderr, "\tCoherency check enabled\n" ) ;
#endif
    if ( OMP_WARN_NESTED ) {
      fprintf( stderr, "\tOMP_WARN_NESTED %d (breakpoint mpcomp_warn_nested)\n", OMP_WARN_NESTED ) ;
    } else {
      fprintf( stderr, "\tNo warning for nested parallelism\n" ) ;
    }
    TODO( "Add every env vars when printing info on OpenMP" ) 
  }
}


/* Initialization of the OpenMP runtime
   Called during the initialization of MPC
 */
void __mpcomp_init() {
  static volatile int done = 0;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  int nb_mvps;
  int task_rank ;

  /* Need to initialize the current team */
  if ( sctk_openmp_thread_tls == NULL ) {
    mpcomp_instance_t * seq_instance ;
    mpcomp_instance_t * instance ;
	mpcomp_team_t * seq_team_info ;
	mpcomp_team_t * team_info ;
    mpcomp_thread_t * t ;
    mpcomp_local_icv_t icvs;

	/* Need to initialize the whole runtime (environment variables) This
	 * section is shared by every OpenMP instances amon MPI tasks located inside
	 * the same OS process */
    sctk_thread_mutex_lock (&lock);
    if ( done == 0 ) {
      __mpcomp_read_env_variables() ;
      mpcomp_global_icvs.def_sched_var = omp_sched_static ;
      mpcomp_global_icvs.bind_var = OMP_PROC_BIND;
      mpcomp_global_icvs.stacksize_var = OMP_STACKSIZE;
      mpcomp_global_icvs.active_wait_policy_var = OMP_WAIT_POLICY;
      mpcomp_global_icvs.thread_limit_var = OMP_THREAD_LIMIT;
      mpcomp_global_icvs.max_active_levels_var = OMP_MAX_ACTIVE_LEVELS;
      mpcomp_global_icvs.nmicrovps_var = OMP_MICROVP_NUMBER ;
      mpcomp_global_icvs.warn_nested = OMP_WARN_NESTED ;
      done = 1;
    }


	/*** Initialize SEQUENTIAL information (current instance + team) ***/

    /* Initialize team information */
    seq_team_info = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
    sctk_assert( seq_team_info != NULL ) ;
    __mpcomp_team_init( seq_team_info ) ;

    /* Allocate an instance of OpenMP */
    seq_instance = (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
    sctk_assert( seq_instance != NULL ) ;
    __mpcomp_instance_init( seq_instance, 1, seq_team_info ) ;


	/*** Initialize PARALLEL information (instance + team for the next parallel
	 * region) ***/

    /* Initialize team information */
    team_info = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
    sctk_assert( team_info != NULL ) ;
    __mpcomp_team_init( team_info ) ;

	/* Get the rank of current MPI task */
	task_rank = sctk_get_task_rank();


	if ( task_rank == -1 ) {
		/* No parallel OpenMP if MPI has not been initialized yet */
		nb_mvps = 1 ;
	} else {
		/* Compute the number of cores for this task */
		sctk_get_init_vp_and_nbvp(task_rank, &nb_mvps);

		sctk_debug( "__mpcomm_init: #mvps = %d", nb_mvps ) ;

		/* Consider the env variable if between 1 and the number
		 * of cores for this task */
		if ( OMP_MICROVP_NUMBER > 0 && OMP_MICROVP_NUMBER <= nb_mvps ) {
			nb_mvps = OMP_MICROVP_NUMBER ;
		}
	}

	if ( sctk_get_local_task_rank() == 0 ) {
	  sctk_debug( "__mpcomm_init: "
	      "MPI rank=%d, process_rank=%d, local_task_rank=%d => %d mvp(s) out of %d core(s) A",
	      task_rank, sctk_get_local_process_rank(), sctk_get_local_task_rank(),
	      sctk_get_processor_number(), sctk_get_processor_number() ) ;
	} else {
	sctk_debug( "__mpcomm_init: "
	    "MPI rank=%d, process_rank=%d, local_task_rank=%d => %d mvp(s) out of %d core(s)",
	    task_rank, sctk_get_local_process_rank(), sctk_get_local_task_rank(),
	   nb_mvps, sctk_get_processor_number() ) ;
	}



    /* Allocate an instance of OpenMP */
    instance = (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
    sctk_assert( instance != NULL ) ;
    __mpcomp_instance_init( instance, nb_mvps, team_info ) ;


    /* Allocate information for the sequential region */
    t = (mpcomp_thread_t *)sctk_malloc( sizeof(mpcomp_thread_t) ) ;
    sctk_assert( t != NULL ) ;

    /* Initialize default ICVs */
    if (OMP_NUM_THREADS == 0) {
	 icvs.nthreads_var = nb_mvps;
    } else {
	 icvs.nthreads_var = OMP_NUM_THREADS;
    }
    icvs.dyn_var = OMP_DYNAMIC;
    icvs.nest_var = OMP_NESTED;
    icvs.run_sched_var = OMP_SCHEDULE;
    icvs.modifier_sched_var = OMP_MODIFIER_SCHEDULE ;

	/* Thread info initialization */
    __mpcomp_thread_init( t, icvs, seq_instance ) ;
	t->children_instance = instance ;

#if MPCOMP_TASK
    /* Ensure tasks lists depths are correct */
    OMP_UNTIED_TASKS_DEPTH = sctk_max(OMP_UNTIED_TASKS_DEPTH, OMP_NEW_TASKS_DEPTH);
    team_info->tasklist_depth[MPCOMP_TASK_TYPE_NEW] = sctk_min(instance->tree_depth, OMP_NEW_TASKS_DEPTH);
    team_info->tasklist_depth[MPCOMP_TASK_TYPE_UNTIED] = sctk_min(instance->tree_depth, OMP_UNTIED_TASKS_DEPTH);

    /* Check the validity of larceny mode */
    if (OMP_TASK_LARCENY_MODE < 0 || OMP_TASK_LARCENY_MODE >= MPCOMP_TASK_LARCENY_MODE_COUNT)
	 team_info->task_larceny_mode = MPCOMP_TASK_LARCENY_MODE_HIERARCHICAL;
    else
	 team_info->task_larceny_mode = OMP_TASK_LARCENY_MODE;

    /* Check the validity of nesting max depth value */
    if (OMP_TASK_NESTING_MAX <= 0)
	 OMP_TASK_NESTING_MAX = 8;
    team_info->task_nesting_max = OMP_TASK_NESTING_MAX;
#endif /* MPCOMP_TASK */

    /* Current thread information is 't' */
    sctk_openmp_thread_tls = t ;

    sctk_thread_mutex_unlock (&lock);

    sctk_nodebug( "__mpcomp_init: Init done..." ) ;
  }

}

void __mpcomp_exit()
{
#if MPCOMP_TASK
     __mpcomp_task_exit();
#endif /* MPCOMP_TASK */
}

void __mpcomp_instance_init( mpcomp_instance_t * instance, int nb_mvps,
	   struct mpcomp_team_s * team	) {

	sctk_nodebug( "__mpcomp_instance_init: Entering..." ) ;

	/* Assign the current team */
	instance->team = team ;

	/* TODO: act here to adapt the number of microVPs for each MPI task */

	if ( nb_mvps > 1 ){
	     hwloc_topology_t restrictedTopology, flatTopology;
		int err;

		/* Alloc memory for 'nb_mvps' microVPs */
		instance->mvps = (mpcomp_mvp_t **)sctk_malloc( nb_mvps * sizeof( mpcomp_mvp_t * ) ) ;
		sctk_assert( instance->mvps != NULL ) ;

		instance->nb_mvps = nb_mvps ;

		__mpcomp_restrict_topology(&restrictedTopology, instance->nb_mvps);
		__mpcomp_flatten_topology(restrictedTopology, &flatTopology);
		instance->topology = flatTopology;

		if ( OMP_TREE == NULL ) {
			__mpcomp_build_default_tree( instance ) ;
		} else {
			__mpcomp_build_tree( instance, OMP_TREE_NB_LEAVES, OMP_TREE_DEPTH, OMP_TREE ) ; 
		}
	} else {
		mpcomp_local_icv_t icvs ;
		/* Sequential instance and team */
		instance->mvps = (mpcomp_mvp_t **)sctk_malloc( 1 * sizeof( mpcomp_mvp_t * ) ) ;
		sctk_assert( instance->mvps != NULL ) ;

		instance->mvps[0] = (mpcomp_mvp_t *)sctk_malloc( 1 * sizeof( mpcomp_mvp_t ) ) ;
		sctk_assert( instance->mvps[0] != NULL ) ;

		instance->nb_mvps = 1 ;

		__mpcomp_thread_init( &(instance->mvps[0]->threads[0]), icvs, instance ) ;

	}

	sctk_nodebug( "__mpcomp_instance_init: Exiting..." ) ;

	/* TODO Do we need a TLS for the openmp instance for every microVPs? */
}

void in_order_scheduler( mpcomp_mvp_t * mvp ) {

  /*
     for i = 0 ; i < #threads in this mVP ; i++ )
     	if (ctx ==0) switch HLS, switch thread
     	call function of threads[i]
	done = 1
     #threads = 0
     */

  int i ;

    sctk_nodebug( "in_order_scheduler: Starting to schedule %d thread(s)", mvp->nb_threads ) ;

  for ( i = 0 ; i < mvp->nb_threads ; i++ ) {
    /* TODO handle out of order */


    sctk_openmp_thread_tls = &mvp->threads[i];

    sctk_assert( ((mpcomp_thread_t *)sctk_openmp_thread_tls)->instance != NULL ) ;
    sctk_assert( ((mpcomp_thread_t *)sctk_openmp_thread_tls)->instance->team != NULL ) ;
    sctk_assert( mvp != NULL);  
    mvp->threads[i].info.func( mvp->threads[i].info.shared ) ;
    mvp->threads[i].done = 1 ;
  }
}

/* Create contextes for non-terminated threads of the same microVP */
void expand_microthreads() {
  /*
     NEW version of fork_when_blocked

     get mpcomp_thread
     if (ctx==0 && #threads > 1)
     	get index inside the microVP
	for ( i = index+1 ; i < #threads ; i++ ) {
	  context=1
	  update func to wrapper_out_of_order_scheduler
	  create stack+ctx
	}
	context=2
     */
}

/* Function call when scheduling another thread while the current one is still alive */
void out_of_order_scheduler() {
  /* Assume 'expand_microthreads' has been called */

  /* Exit after scheduling one thread */

  /*
  i = index of current thread in current mVP
  while i < #threads
    if is_running[i]!=0 && done[i] == 0
    	swap context between i and current
	restore tls mpcomp_thread
	break ;
    i++
    */
}


/*
 * Return the maximum number of processors.
 * See OpenMP API 1.0 Section 3.2.5
 */
int
mpcomp_get_num_procs (void)
{
  mpcomp_thread_t * t ;
  __mpcomp_init ();

  return mpcomp_global_icvs.nmicrovps_var;
}


/**
  * Set or unset the dynamic adaptation of the thread number.
  * See OpenMP API 2.5 Section 3.1.7
  */
void
mpcomp_set_dynamic (int dynamic_threads)
{
  mpcomp_thread_t * t ;
  __mpcomp_init ();
  sctk_nodebug( "mpcomp_get_dynamic: entering" ) ;
  t = sctk_openmp_thread_tls;
  sctk_assert( t != NULL);
  t->info.icvs.dyn_var = dynamic_threads;
}


/**
  * Retrieve the current dynamic adaptation of the program.
  * See OpenMP API 2.5 Section 3.2.8
  */
int
mpcomp_get_dynamic (void)
{
  mpcomp_thread_t * t ;
  __mpcomp_init ();
  sctk_nodebug( "mpcomp_get_dynamic: entering" ) ;
  t = sctk_openmp_thread_tls;
  sctk_assert( t != NULL);
  return t->info.icvs.dyn_var;
}

/**
  *
  * See OpenMP API 2.5 Section 3.2.9
  */
void
mpcomp_set_nested (int nested)
{
  mpcomp_thread_t * t ;
  __mpcomp_init ();
  sctk_nodebug( "mpcomp_set_nested: entering" ) ;
  t = sctk_openmp_thread_tls;
  sctk_assert( t != NULL);
  t->info.icvs.nest_var = nested;
}

/**
  *
  * See OpenMP API 2.5 Section 3.2.10
  */
int
mpcomp_get_nested (void)
{
  mpcomp_thread_t * t ;
  __mpcomp_init ();
  sctk_nodebug( "mpcomp_get_nested: entering" ) ;
  t = sctk_openmp_thread_tls;
  sctk_assert( t != NULL);
  return t->info.icvs.nest_var;
}


/**
  *
  * See OpenMP API 3.0 Section 3.2.11
  */
void omp_set_schedule( omp_sched_t kind, int modifier ) {
  mpcomp_thread_t * t ;

  __mpcomp_init ();
  sctk_nodebug( "mpcomp_set_schedule: entering" ) ;
  t = sctk_openmp_thread_tls;
  sctk_assert( t != NULL);

  t->info.icvs.run_sched_var = kind ;
  t->info.icvs.modifier_sched_var = modifier ;
}

/**
  *
  * See OpenMP API 3.0 Section 3.2.12
  */
void omp_get_schedule( omp_sched_t * kind, int * modifier ) {
  mpcomp_thread_t * t ;

  __mpcomp_init ();
  sctk_nodebug( "mpcomp_get_chedule: entering" ) ;
  t = sctk_openmp_thread_tls;
  sctk_assert( t != NULL);
  *kind = t->info.icvs.run_sched_var ;
  *modifier = t->info.icvs.modifier_sched_var ;
}


/*
 * Check whether the current flow is located inside a parallel region or not.
 * See OpenMP API 2.5 Section 3.2.6
 */
int
mpcomp_in_parallel (void)
{
  mpcomp_thread_t * t ;

  __mpcomp_init ();
  sctk_nodebug( "mpcomp_in_parallel: entering" ) ;
  t = sctk_openmp_thread_tls;
  sctk_assert( t != NULL);
  return (t->instance->team->depth != 0);
}

/*
 * OpenMP 3.0. Returns the nesting level for the parallel block, which enclose the calling call.
 * TODO : differencier active_level et level
 */
int
mpcomp_get_level (void)
{
	not_implemented();
	/*
	mpcomp_thread_t *t;
	__mpcomp_init ();
	t = sctk_openmp_thread_tls;
	sctk_assert(t != NULL);
	sctk_debug( "mpcomp_get_level: entering %d", t->instance->team->depth);
	return t->instance->team->depth;
	*/
	return -1 ;
}

/*
 * OpenMP 3.0. Returns the nesting level for the active parallel block, which enclose the calling call.
 */
int
mpcomp_get_active_level (void)
{
	mpcomp_thread_t *t;
	__mpcomp_init ();
	t = sctk_openmp_thread_tls;
	sctk_assert(t != NULL);
	sctk_debug( "mpcomp_get_active_level: entering %d", t->instance->team->depth);
	return t->instance->team->depth;
}

/*
 * OpenMP 3.0. This function returns the thread identification number for the given nesting level of the current thread.
 * For values of level outside zero to omp_get_level -1 is returned. 
 * if level is omp_get_level the result is identical to omp_get_thread_num
 */
int 
mpcomp_get_ancestor_thread_num(int level)
{
	not_implemented();
	/*
	mpcomp_thread_t *t;
	__mpcomp_init();
	t = sctk_openmp_thread_tls;
	sctk_assert(t != NULL);
	mpcomp_instance_t *instance = t->instance;
	if (level < 0 || level > (instance->team->depth))
	{
	sctk_debug( "mpcomp_get_ancestor_thread_num (%d) = -1", level);
		return -1;
	}
	for (level = (instance->team->depth - level); level > 0; --level)
		t = &instance->root->father->instance;
	return (team id);
	*/
	return -1 ;
}

/*
 * OpenMP 3.0. This function returns the number of threads in a thread team to which either the current thread or its ancestor belongs.
 * For values of level outside zero to omp_get_level, -1 is returned.
 * if level is zero, 1 is returned, and for omp_get_level, the result is identical to omp_get_num_threads.
 */
int 
omp_get_team_size(int level)
{
	not_implemented();
	/*
	mpcomp_thread_t *t;
	__mpcomp_init();
	t = sctk_openmp_thread_tls;
	sctk_assert(t != NULL);
	mpcomp_instance_t *instance = t->instance;
	if (level < 0 || level > (instance->team->depth))
	{
	sctk_debug( "mpcomp_get_team_size (%d) = -1", level);
		return -1;
	}
	for (level = (instance->team->depth - level); level > 0; --level)
		t = &instance->root->father->instance;
	if (instance->team == NULL)
	{
	sctk_debug( "mpcomp_get_team_size (%d) = 1", level);
		return 1;
	}
	else
	{
	sctk_debug( "mpcomp_get_team_size (%d) = %d", level, instance->team->info.num_threads);
		return instance->team->info.num_threads;
	}
	*/
	return -1 ;
}

/*
 * Return the number of threads used in the current team (direct enclosing
 * parallel region).
 * If called from a sequential part of the program, this function returns 1.
 * (See OpenMP API 2.5 Section 3.2.2)
 */
int
mpcomp_get_num_threads (void)
{
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  sctk_nodebug( "mpcomp_get_num_threads: entering" ) ;

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

  return t->info.num_threads;
}

void
mpcomp_set_num_threads(int num_threads) 
{
 mpcomp_thread_t * t;

 __mpcomp_init ();

 t = sctk_openmp_thread_tls;
 sctk_assert( t != NULL);

 sctk_nodebug("[%d, %p] mpcomp_set_num_threads: entering for %d thread(s)",
		 t->rank, t, num_threads ) ;

 t->info.icvs.nthreads_var = num_threads;

}

/*
 * Return the thread number (ID) of the current thread whitin its team.
 * The master thread of each team has a thread ID equals to 0.
 * This function returns 0 if called from a serial part of the code (or from a
 * flatterned nested parallel region).
 * See OpenMP API 2.5 Section 3.2.4
 */
int
mpcomp_get_thread_num (void)
{
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  sctk_nodebug( "mpcomp_get_thread_num: entering" ) ;

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

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
  mpcomp_thread_t * t ;

  __mpcomp_init ();

  t = sctk_openmp_thread_tls ;
  sctk_assert( t != NULL ) ;

 sctk_nodebug("[%d, %p] mpcomp_get_max_threads: getting %d thread(s)",
		 t->rank, t, t->info.icvs.nthreads_var) ;

  return t->info.icvs.nthreads_var;
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

void __mpcomp_flush() 
{
/* TODO TEMP only return, but need to handle oversubscribing */
}



void __mpcomp_ordered_begin()
{
     mpcomp_thread_t *t;

     __mpcomp_init();
     
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert(t != NULL); 

     sctk_nodebug( "[%d] __mpcomp_ordered_begin: enter w/ iteration %d and team %d",
			 t->rank, t->current_ordered_iteration, t->instance->team->next_ordered_offset ) ;

     /* First iteration of the loop -> initialize 'next_ordered_offset' */
     if (t->current_ordered_iteration == t->info.loop_lb) {
	  t->instance->team->next_ordered_offset = 0;
     } else {
	  /* Do we have to wait for the right iteration? */
	  if (t->current_ordered_iteration != 
	      (t->info.loop_lb + t->info.loop_incr * 
		   t->instance->team->next_ordered_offset)) {
	       mpcomp_mvp_t *mvp;
	       
	       sctk_nodebug("__mpcomp_ordered_begin[%d]: Waiting to schedule iteration %d",
			    t->rank, t->current_ordered_iteration);
	       
	       /* Grab the corresponding microVP */
	       mvp = t->mvp;

		   while ( t->current_ordered_iteration != 
				   (t->info.loop_lb + t->info.loop_incr * 
					t->instance->team->next_ordered_offset) )
		   {
			   sctk_thread_yield();
		   }
#if 0	       
	       TODO("use correct primitives")
	       mpcomp_fork_when_blocked (my_vp, info->step);
	       
	       /* Spin while the condition is not satisfied */
	       mpcomp_macro_scheduler (my_vp, info->step);
	       while ( info->current_ordered_iteration != 
		       (info->loop_lb + info->loop_incr * team->next_ordered_offset) ) {
		    mpcomp_macro_scheduler (my_vp, info->step);
		    if ( info->current_ordered_iteration != 
			 (info->loop_lb + info->loop_incr * team->next_ordered_offset) ) {
			 sctk_thread_yield ();
		    }
	       }
#endif

	  }
     }

     sctk_nodebug( "__mpcomp_ordered_begin[%d]: Allowed to schedule iteration %d",
		   info->rank, info->current_ordered_iteration ) ;
}



void __mpcomp_ordered_end()
{
     mpcomp_thread_t *t;
     
     t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
     sctk_assert(t != NULL); 
     
     t->current_ordered_iteration += t->info.loop_incr ;
     if ( (t->info.loop_incr > 0 && t->current_ordered_iteration >= t->info.loop_b) ||
	  (t->info.loop_incr < 0 && t->current_ordered_iteration <= t->info.loop_b) ) {
	  t->instance->team->next_ordered_offset = -1 ;
     } else {
	  t->instance->team->next_ordered_offset++ ;
     }
}

void
mpcomp_warn_nested() 
{
  fprintf( stderr, "WARNING: Nested invoked (mpcomp_warn_nested)\n" ) ;
}
