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

#include "sctk_thread_generic_kind.h"

#include "sctk_debug.h"
#include "sctk_topology.h"
#include "sctk_runtime_config.h"
#include "mpcomp_internal.h"
#include <sys/time.h>
#include <ctype.h>

#include "mpcomp_tree_structs.h"
#include "mpcomp_task_utils.h"

/*****************
  ****** GLOBAL VARIABLES 
  *****************/

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
/* Should we emit a warning when nesting is used? */
static int OMP_WARN_NESTED = 0 ;
/* Hybrid MPI/OpenMP mode */
static mpcomp_mode_t OMP_MODE = MPCOMP_MODE_SIMPLE_MIXED ;
/* Affinity policy */
static mpcomp_affinity_t OMP_AFFINITY = MPCOMP_AFFINITY_BALANCED;

mpcomp_global_icv_t mpcomp_global_icvs;

/*****************
  ****** FUNCTIONS  
  *****************/

TODO(" function __mpcomp_tokenizer is inspired from sctk_launch.c. Need to merge")
static char ** 
__mpcomp_tokenizer( char * string_to_tokenize, int * nb_tokens ) 
{
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

/*
 * Read environment variables for OpenMP.
 * Actually, the values are read from configuration: those values can be
 * assigned with environement variable or config file.
 * This function is called once per process
 */
static inline 
void __mpcomp_read_env_variables() 
{
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
  OMP_PROC_BIND = sctk_runtime_config_get()->modules.openmp.proc_bind ? 1 : 0;

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

  /******* OMP_MODE *******/
  env = sctk_runtime_config_get()->modules.openmp.mode ;
  OMP_MODE = MPCOMP_MODE_SIMPLE_MIXED ;
  if ( env != NULL )
  {
    int ok = 0 ;

	/* Handling MPCOMP_MODE_SIMPLE_MIXED */
	if ( strncmp (env, "simple-mixed", strlen("simple-mixed") ) == 0 
		||
		strncmp (env, "SIMPLE-MIXED", strlen("SIMPLE-MIXED") ) == 0
		||
		strncmp (env, "simple_mixed", strlen("simple_mixed") ) == 0
		||
		strncmp (env, "SIMPLE_MIXED", strlen("SIMPLE_MIXED") ) == 0
		) 
	{
	  OMP_MODE = MPCOMP_MODE_SIMPLE_MIXED ;
      ok = 1 ;
	}

	/* Handling MPCOMP_MODE_OVERSUBSCRIBED_MIXED */
	if ( strncmp (env, "alternating", strlen("alternating") ) == 0 
		||
		strncmp (env, "ALTERNATING", strlen("ALTERNATING") ) == 0
		) 
	{
	  OMP_MODE = MPCOMP_MODE_ALTERNATING ;
      ok = 1 ;
	}

	/* Handling MPCOMP_MODE_ALTERNATING */
	if ( strncmp (env, "oversubscribed-mixed", strlen("oversubscribed-mixed") ) == 0 
		||
		strncmp (env, "OVERSUBSCRIBED-MIXED", strlen("OVERSUBSCRIBED-MIXED") ) == 0
		||
		strncmp (env, "oversubscribed_mixed", strlen("oversubscribed_mixed") ) == 0
		||
		strncmp (env, "OVERSUBSCRIBED_MIXED", strlen("OVERSUBSCRIBED_MIXED") ) == 0
		) 
	{
	  OMP_MODE = MPCOMP_MODE_OVERSUBSCRIBED_MIXED ;
      ok = 1 ;
	}

	/* Handling MPCOMP_MODE_FULLY_MIXED */
	if ( strncmp (env, "fully-mixed", strlen("fully-mixed") ) == 0 
		||
		strncmp (env, "FULLY-MIXED", strlen("FULLY-MIXED") ) == 0
		||
		strncmp (env, "fully_mixed", strlen("fully_mixed") ) == 0
		||
		strncmp (env, "FULLY_MIXED", strlen("FULLY_MIXED") ) == 0
		) 
	{
	  OMP_MODE = MPCOMP_MODE_FULLY_MIXED ;
      ok = 1 ;
	}

	if ( ok ) 
	{
	} else {
      fprintf (stderr,
	  "Warning: Unknown mode <%s> (must be SIMPLE_MIXED, ALTERNATING, OVERSUBSCRIBED_MIXED or FULLY_MIXED),"
	  " fallback to default mode <%d>\n", env,
	  OMP_MODE);
	}

  }

  /******* OMP_AFFINITY *******/
  env = sctk_runtime_config_get()->modules.openmp.affinity;
  if ( env != NULL )
  {
    int ok = 0 ;

	/* Handling MPCOMP_AFFINITY_COMPACT */
	if ( strncmp (env, "compact", strlen("compact") ) == 0 
		||
		strncmp (env, "COMPACT", strlen("COMPACT") ) == 0
		) 
	{
	  OMP_AFFINITY = MPCOMP_AFFINITY_COMPACT ;
      ok = 1 ;
	}

	/* Handling MPCOMP_AFFINITY_SCATTER */
	if ( strncmp (env, "scatter", strlen("scatter") ) == 0 
		||
		strncmp (env, "SCATTER", strlen("SCATTER") ) == 0
		) 
	{
	  OMP_AFFINITY = MPCOMP_AFFINITY_SCATTER ;
      ok = 1 ;
	}

	/* Handling MPCOMP_AFFINITY_BALANCED */
	if ( strncmp (env, "balanced", strlen("balanced") ) == 0 
		||
		strncmp (env, "BALANCED", strlen("BALANCED") ) == 0
		) 
	{
	  OMP_AFFINITY = MPCOMP_AFFINITY_BALANCED ;
      ok = 1 ;
	}

	if ( ok ) 
	{
	} else {
      fprintf (stderr,
	  "Warning: Unknown affinity <%s> (must be COMPACT, SCATTER or BALANCED),"
	  " fallback to default affinity <%d>\n", env,
	  OMP_AFFINITY );
	}

  }


  /******* OMP_TREE *********/
  env = sctk_runtime_config_get()->modules.openmp.tree ;
  
  if (strlen( env) != 0)
  {
    int nb_tokens = 0 ;
    char ** tokens = NULL ;
    int i ;

    tokens = __mpcomp_tokenizer( env, &nb_tokens ) ;
    sctk_assert( tokens != NULL ) ;


    /* TODO check that arguments are ok and #leaves is correct */

    OMP_TREE = (int *)sctk_malloc( nb_tokens * sizeof( int ) ) ;
    OMP_TREE_DEPTH = nb_tokens ;
    OMP_TREE_NB_LEAVES = 1 ;
    for ( i = 0 ; i < nb_tokens ; i++ ) {
      OMP_TREE[i] = atoi( tokens[ i ] ) ;
      OMP_TREE_NB_LEAVES *= OMP_TREE[i] ;
    }

	TODO( "check the env variable OMP_TREE" )

  } else {
	  OMP_TREE = NULL ;
  }

	/***** PRINT SUMMARY (only first MPI rank) ******/
  	if (getenv ("MPC_DISABLE_BANNER") == NULL && sctk_process_rank == 0) {
    fprintf (stderr,
	"MPC OpenMP version %d.%d (Intel and Patched GCC compatibility)\n",
	SCTK_OMP_VERSION_MAJOR, SCTK_OMP_VERSION_MINOR);
#if MPCOMP_TASK
    fprintf (stderr, "\tOpenMP 3 Tasking on\n" ) ;
#else
    fprintf (stderr, "\tOpenMP 3 Tasking off\n" ) ;
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
    fprintf( stderr, "\tAffinity " ) ;
    switch ( OMP_AFFINITY ) {
        case MPCOMP_AFFINITY_COMPACT:
          fprintf(stderr, "COMPACT (fill logical cores first)\n");
          break;
        case MPCOMP_AFFINITY_SCATTER:
          fprintf(stderr, "SCATTER (spread over NUMA nodes)\n");
          break;
        case MPCOMP_AFFINITY_BALANCED:
          fprintf(stderr, "BALANCED (fill physical cores first)\n");
          break;
        default:
            fprintf( stderr, "Unknown\n" ) ;
            break ;
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

	fprintf( stderr, "\tMode for hybrid MPI+OpenMP parallelism " ) ;
	switch ( OMP_MODE ) 
	{
	  case MPCOMP_MODE_SIMPLE_MIXED:
		fprintf( stderr, "SIMPLE_MIXED\n" ) ;
		break ;
	  case MPCOMP_MODE_OVERSUBSCRIBED_MIXED:
		fprintf( stderr, "OVERSUBSCRIBED_MIXED\n" ) ;
		break ;
	  case MPCOMP_MODE_ALTERNATING:
		fprintf( stderr, "ALTERNATING\n" ) ;
		break ;
	  case MPCOMP_MODE_FULLY_MIXED:
		fprintf( stderr, "FULLY_MIXED\n" ) ;
		break ;
	  default:
		not_reachable() ;
		break ;
	}
#if MPCOMP_MALLOC_ON_NODE
    fprintf( stderr, "\tNUMA allocation for tree nodes\n" ) ;
#endif
#if MPCOMP_COHERENCY_CHECKING
    fprintf( stderr, "\tCoherency check enabled\n" ) ;
#endif
#if MPCOMP_ALIGN
    fprintf( stderr, "\tStructure field alignement\n" ) ;
#endif
    if ( OMP_WARN_NESTED ) {
      fprintf( stderr, "\tOMP_WARN_NESTED %d (breakpoint mpcomp_warn_nested)\n", OMP_WARN_NESTED ) ;
    } else {
      fprintf( stderr, "\tNo warning for nested parallelism\n" ) ;
    }
#if MPCOMP_MIC
    fprintf( stderr, "\tMIC optimizations on\n" ) ;
#endif
    TODO( "Add every env vars when printing info on OpenMP" ) 
  }
}


/* Initialization of the OpenMP runtime
   Called during the initialization of MPC
 */
void 
__mpcomp_init() 
{
  static volatile int done = 0;
  static sctk_thread_mutex_t lock = SCTK_THREAD_MUTEX_INITIALIZER;
  int nb_mvps;
  int task_rank, id_numa;

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
      mpcomp_global_icvs.affinity = OMP_AFFINITY ;
      done = 1;
    }


	/*** Initialize SEQUENTIAL information (current instance + team) ***/

    /* Initialize team information */
    seq_team_info = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
    sctk_assert( seq_team_info != NULL ) ;
    mpcomp_team_infos_init( seq_team_info ) ;

    /* Allocate an instance of OpenMP */
    seq_instance = (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
    sctk_assert( seq_instance != NULL ) ;
    __mpcomp_instance_init( seq_instance, 1, seq_team_info ) ;


	/*** Initialize PARALLEL information (instance + team for the next parallel
	 * region) ***/

    /* Initialize team information */
    team_info = (mpcomp_team_t *)sctk_malloc( sizeof( mpcomp_team_t ) ) ;
    sctk_assert( team_info != NULL ) ;
    mpcomp_team_infos_init( team_info ) ;

	/* Get the rank of current MPI task */
	task_rank = sctk_get_task_rank();
        /* Get id numa*/
        id_numa = sctk_get_node_from_cpu(sctk_get_init_vp(task_rank));

        if (task_rank == -1) {
          /* No parallel OpenMP if MPI has not been initialized yet */
          nb_mvps = 1;
        } else {
          sctk_debug("__mpcomp_init: sctk_get_task_rank = %d", task_rank);

          /* Compute the number of microVPs according to Hybrid Mode */
          switch (OMP_MODE) {
          case MPCOMP_MODE_SIMPLE_MIXED:
            /* Compute the number of cores for this task */

            sctk_get_init_vp_and_nbvp(task_rank, &nb_mvps);

            sctk_debug("__mpcomp_init: SIMPLE_MIXED -> #mvps = %d", nb_mvps);

            /* Consider the env variable if between 1 and the number
             * of cores for this task */
            if (OMP_MICROVP_NUMBER > 0 && OMP_MICROVP_NUMBER <= nb_mvps) {
              nb_mvps = OMP_MICROVP_NUMBER;
            }
            break;
          case MPCOMP_MODE_ALTERNATING:
            nb_mvps = 1;
            if (sctk_get_local_task_rank() == 0) {
              nb_mvps = sctk_get_processor_number();
            }
            break;
          case MPCOMP_MODE_OVERSUBSCRIBED_MIXED:
            not_implemented();
            break;
          case MPCOMP_MODE_FULLY_MIXED:
            nb_mvps = sctk_get_processor_number();
            break;
          default:
            not_reachable();
            break;
          }
        }

        if (sctk_get_local_task_rank() == 0) {
          sctk_debug(
              "__mpcomp_init: "
              "MPI rank=%d, process_rank=%d, local_task_rank=%d => %d mvp(s) "
              "out of %d core(s) A",
              task_rank, sctk_get_local_process_rank(),
              sctk_get_local_task_rank(), sctk_get_processor_number(),
              sctk_get_processor_number());
    } else {
        sctk_debug( "__mpcomp_init: "
                "MPI rank=%d, process_rank=%d, local_task_rank=%d => %d "
                "mvp(s) out of %d core(s)",
                task_rank, sctk_get_local_process_rank(), 
                sctk_get_local_task_rank(),
                nb_mvps, sctk_get_processor_number() ) ;
    }

    /* Update some global icvs according the number of mvps */
    if( mpcomp_global_icvs.thread_limit_var == 0 
          || mpcomp_global_icvs.thread_limit_var > nb_mvps ) {
      mpcomp_global_icvs.thread_limit_var = nb_mvps;
    }

    if( mpcomp_global_icvs.nmicrovps_var == 0 ) {
      mpcomp_global_icvs.nmicrovps_var = nb_mvps;
    }

    if( mpcomp_global_icvs.max_active_levels_var == 0 ) {
      mpcomp_global_icvs.max_active_levels_var = 1;
    }

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
    icvs.active_levels_var = 0 ;
    icvs.levels_var = 0 ;

    /* Allocate information for the sequential region */
    t = (mpcomp_thread_t *)sctk_malloc( sizeof(mpcomp_thread_t) ) ;
    sctk_assert( t != NULL ) ;

	/* Thread info initialization */
    mpcomp_thread_infos_init( t, icvs, seq_instance, NULL ) ;

    /* Current thread information is 't' */
    sctk_openmp_thread_tls = t ;

    /* Allocate an instance of OpenMP */
    instance = (mpcomp_instance_t *)sctk_malloc( sizeof( mpcomp_instance_t ) ) ;
    sctk_assert( instance != NULL ) ;
    __mpcomp_instance_init( instance, nb_mvps, team_info ) ;

	t->children_instance = instance ;

#if MPCOMP_TASK
	mpcomp_task_team_infos_init( team_info, instance->tree_depth );
#endif /* MPCOMP_TASK */

    sctk_thread_mutex_unlock (&lock);

   // sctk_error( "__mpcomp_init: Init done..." ) ;
  }

}

void 
__mpcomp_exit()
{
#if MPCOMP_TASK
     //__mpcomp_task_exit();
#endif /* MPCOMP_TASK */
}

void 
__mpcomp_instance_init( mpcomp_instance_t * instance, int nb_mvps,
        struct mpcomp_team_s * team	) 
{

	/* TODO Field to update: 
	 * tree_depth, tree_base, tree_cumulative, topology,
	 * tree_level_size, tree_array_size, tree_array_first_rank 
	 */

	sctk_nodebug( "__mpcomp_instance_init: Entering... %p %d %p", instance, nb_mvps, team ) ;

	/* Assign the current team */
	instance->team = team ;

    /* If this instance is not sequential... */
	if ( nb_mvps > 1 ){
        hwloc_topology_t restrictedTopology, flatTopology;
		int err, i;

		/* Alloc memory for 'nb_mvps' microVPs */
		instance->mvps = (mpcomp_mvp_t **)sctk_malloc( nb_mvps * sizeof( mpcomp_mvp_t * ) ) ;
		sctk_assert( instance->mvps != NULL ) ;

		instance->nb_mvps = nb_mvps ;

        /* Restrict the global topology to the number of microVPs */
		err = __mpcomp_restrict_topology(&restrictedTopology, instance->nb_mvps);
        if ( err != 0 ) {
            sctk_error( "MPC_OpenMP Internal error in __mpcomp_restrict_topology" ) ;
            sctk_abort() ;
        }

		instance->topology = restrictedTopology ;

		if ( OMP_TREE == NULL ) {
			__mpcomp_build_default_tree( instance ) ;
		} else {
			__mpcomp_build_tree( instance, OMP_TREE_NB_LEAVES, OMP_TREE_DEPTH, OMP_TREE ) ; 
		}
	} else {
        int i ;
        int id_numa =
            sctk_get_node_from_cpu(sctk_get_init_vp(sctk_get_task_rank()));

        mpcomp_local_icv_t icvs;
        /* Sequential instance and team */
        instance->mvps =
            (mpcomp_mvp_t **)sctk_malloc(1 * sizeof(mpcomp_mvp_t *));
        sctk_assert(instance->mvps != NULL);

        instance->mvps[0] =
            (mpcomp_mvp_t *)sctk_malloc(1 * sizeof(mpcomp_mvp_t));
        sctk_assert(instance->mvps[0] != NULL);

        for ( i = 0 ; i < MPCOMP_AFFINITY_NB ; i++ ) {
            instance->mvps[0]->min_index[i] = 0 ;
        }

		instance->nb_mvps = 1 ;
		instance->root = NULL ;

		mpcomp_thread_infos_init( &(instance->mvps[0]->threads[0]), icvs, instance, sctk_openmp_thread_tls  ) ;
        }

        sctk_nodebug("__mpcomp_instance_init: Exiting...");

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
	mpcomp_thread_t * t, *cur_mvp_thread ;

	sctk_assert( mvp != NULL ) ;

	sctk_error( "in_order_scheduler: Starting to schedule %d thread(s)", mvp->nb_threads ) ;

	/* Save previous TLS */
	t = sctk_openmp_thread_tls ;
	// t can be NULL for worker thread
  // sctk_assert( t != NULL ) ;

  for ( i = 0 ; i < mvp->nb_threads ; i++ ) {
    /* TODO handle out of order */

    sctk_openmp_thread_tls = &mvp->threads[i];
    cur_mvp_thread = (mpcomp_thread_t*) sctk_openmp_thread_tls;
	 fprintf(stderr, "value ... %p\n", cur_mvp_thread );

    // hmt
    // set the KIND_MASK_OMP to the current thread
    // printf("BEFORE OMP: %d\n", sctk_thread_generic_getkind_mask_self());
    sctk_thread_generic_addkind_mask_self(KIND_MASK_OMP);
    sctk_thread_generic_set_basic_priority_self(
        sctk_runtime_config_get()->modules.scheduler.omp_basic_priority);
    sctk_thread_generic_setkind_priority_self(
        sctk_runtime_config_get()->modules.scheduler.omp_basic_priority);
    sctk_thread_generic_set_current_priority_self(
        sctk_runtime_config_get()->modules.scheduler.omp_basic_priority);
    // printf("AFTER OMP: %d\n", sctk_thread_generic_getkind_mask_self());
    // endhmt

    sctk_assert( ((mpcomp_thread_t *)sctk_openmp_thread_tls)->instance != NULL ) ;
    sctk_assert( ((mpcomp_thread_t *)sctk_openmp_thread_tls)->instance->team != NULL ) ;
    sctk_assert( mvp->threads[i].info.func != NULL ) ;

		/* Handle beginning of combined parallel region */
		switch( mvp->threads[i].info.combined_pragma ) {
			case MPCOMP_COMBINED_NONE:
				sctk_nodebug( "in_order_scheduler: BEGIN - No combined parallel" ) ;
				break ;
			case MPCOMP_COMBINED_SECTION:
				sctk_nodebug( "in_order_scheduler: BEGIN - Combined parallel/sections w/ %d section(s)",
						mvp->threads[i].info.nb_sections	) ;
				__mpcomp_sections_init( 
						&(mvp->threads[i]),
						mvp->threads[i].info.nb_sections ) ;
				break ;
			case MPCOMP_COMBINED_STATIC_LOOP:
				sctk_nodebug( "in_order_scheduler: BEGIN - Combined parallel/loop" ) ;
				__mpcomp_static_loop_init(
						&(mvp->threads[i]),
						mvp->threads[i].info.loop_lb,
						mvp->threads[i].info.loop_b,
						mvp->threads[i].info.loop_incr,
						mvp->threads[i].info.loop_chunk_size
						) ;
				break ;
			case MPCOMP_COMBINED_DYN_LOOP:
				sctk_nodebug( "in_order_scheduler: BEGIN - Combined parallel/loop" ) ;
				__mpcomp_dynamic_loop_init(
						&(mvp->threads[i]),
						mvp->threads[i].info.loop_lb,
						mvp->threads[i].info.loop_b,
						mvp->threads[i].info.loop_incr,
						mvp->threads[i].info.loop_chunk_size) ;
				break ;
			default:
				not_implemented() ;
				break ;
		}

    mvp->threads[i].info.func( mvp->threads[i].info.shared ) ;

		/* Handle ending of combined parallel region */
		switch( mvp->threads[i].info.combined_pragma ) {
			case MPCOMP_COMBINED_NONE:
				sctk_nodebug( "in_order_scheduler: END - No combined parallel" ) ;
				break ;
			case MPCOMP_COMBINED_SECTION:
				sctk_nodebug( "in_order_scheduler: END - Combined parallel/sections w/ %d section(s)",
						mvp->threads[i].info.nb_sections	) ;
				break ;
			case MPCOMP_COMBINED_STATIC_LOOP:
				sctk_nodebug( "in_order_scheduler: END - Combined parallel/loop" ) ;
				break ;
			case MPCOMP_COMBINED_DYN_LOOP:
				sctk_nodebug( "in_order_scheduler: END - Combined parallel/loop" ) ;
				__mpcomp_dynamic_loop_end_nowait(
						&(mvp->threads[i])
						) ;
				break ;
			default:
				not_implemented() ;
				break ;
		}



    mvp->threads[i].done = 1 ;
  }

	while( !mpcomp_task_all_task_executed() ) 
	{
   	__mpcomp_task_schedule( 0 );
	}

	/* Restore previous TLS */
	sctk_openmp_thread_tls = t ;
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


void 
__mpcomp_flush() 
{
/* TODO TEMP only return, but need to handle oversubscribing */
}


	void 
__mpcomp_ordered_begin()
{
	mpcomp_thread_t *t;

	__mpcomp_init();

	t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
	sctk_assert(t != NULL); 

    /* No need to check something in case of 1 thread */
    if ( t->info.num_threads == 1 ) return ;

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
			t->rank, t->current_ordered_iteration ) ;
}



void __mpcomp_ordered_end()
{
	mpcomp_thread_t *t;

	t = (mpcomp_thread_t *) sctk_openmp_thread_tls;
	sctk_assert(t != NULL); 

    /* No need to check something in case of 1 thread */
    if ( t->info.num_threads == 1 ) return ;

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
