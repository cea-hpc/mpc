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
/* #   - CAPRA Antoine capra@paratools.com                                # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_common_rank.h>
#include <mpc_common_flags.h>
#include "sctk_debug.h"
#include "mpc_topology.h"
#include "mpc_runtime_config.h"
#include <sys/time.h>
#include <ctype.h>

#include "mpc_launch.h"

#include "mpcomp_task.h"
#include "mpcomp_sync.h"

#include "mpcomp_sync.h"
#include "mpcomp_core.h"
#include "mpcomp_tree.h"

/* Loop declaration */
#include "mpcomp_loop.h"
#include "mpcomp_tree.h"

#include "mpc_thread.h"

#include "ompt.h"
/* parsing OMP_PLACES */
#include "mpcomp_places_env.h"

/*****************
  ****** GLOBAL VARIABLES
  *****************/

/* Schedule type */
static omp_sched_t OMP_SCHEDULE = omp_sched_static;
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
static int *OMP_TREE = NULL;
/* Number of level for the default tree (size of OMP_TREE) */
static int OMP_TREE_DEPTH = 0;
/* Total number of leaves for the tree (product of OMP_TREE elements) */
static int OMP_TREE_NB_LEAVES = 0;
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
static int OMP_WARN_NESTED = 0;
/* Hybrid MPI/OpenMP mode */
static mpcomp_mode_t OMP_MODE = MPCOMP_MODE_SIMPLE_MIXED;
/* Affinity policy */
static mpcomp_affinity_t OMP_AFFINITY = MPCOMP_AFFINITY_SCATTER;
/* OMP_PLACES */
static mpcomp_places_info_t *OMP_PLACES_LIST = NULL;

mpcomp_global_icv_t mpcomp_global_icvs;

/*****************
  ****** FUNCTIONS
  *****************/

TODO( " function __mpcomp_tokenizer is inspired from sctk_launch.c. Need to "
      "merge" )
static char **__mpcomp_tokenizer( char *string_to_tokenize, int *nb_tokens )
{
	/*    size_t len;*/
	char *cursor;
	char **new_argv;
	int new_argc = 0;
	/* TODO check sizes of every mpcomp_alloc... */
	new_argv = ( char ** ) mpcomp_alloc( 32 * sizeof( char * ) );
	sctk_assert( new_argv != NULL );
	cursor = string_to_tokenize;

	while ( *cursor == ' ' )
	{
		cursor++;
	}

	while ( *cursor != '\0' )
	{
		int word_len = 0;
		new_argv[new_argc] = ( char * ) mpcomp_alloc( 1024 * sizeof( char ) );

		while ( ( word_len < 1024 ) && ( *cursor != '\0' ) && ( *cursor != ' ' ) )
		{
			new_argv[new_argc][word_len] = *cursor;
			cursor++;
			word_len++;
		}

		new_argv[new_argc][word_len] = '\0';
		new_argc++;

		while ( *cursor == ' ' )
		{
			cursor++;
		}
	}

	new_argv[new_argc] = NULL;
	*nb_tokens = new_argc;
	return new_argv;
}

static void
__mpcomp_aux_reverse_one_dim_array( int *array, const int len )
{
	int i, j, tmp;

	for ( i = 0, j = len - 1; i < j; i++, j-- )
	{
		tmp = array[i];
		array[i] = array[j];
		array[j] = tmp;
	}
}

static int *
__mpcomp_convert_topology_to_tree_shape( hwloc_topology_t topology, int *shape_depth )
{
	int *reverse_shape;
	int i, j, reverse_shape_depth;
	const int max_depth = hwloc_topology_get_depth( topology );
	sctk_assert( max_depth > 1 );
	reverse_shape = ( int * ) mpcomp_alloc( sizeof( int ) * max_depth );
	sctk_assert( reverse_shape );
	memset( reverse_shape, 0, sizeof( int ) * max_depth );
	/* Last level size */
	reverse_shape[0] = hwloc_get_nbobjs_by_depth( topology, max_depth - 1 );
	reverse_shape_depth = 1;

	for ( i = max_depth - 2; i >= 0; i-- )
	{
		int cur_stage_num = hwloc_get_nbobjs_by_depth( topology, i );
		const int last_stage_num = reverse_shape[reverse_shape_depth - 1];

		if ( cur_stage_num == 1 )
		{
			break;
		}

		if ( cur_stage_num == last_stage_num || last_stage_num % cur_stage_num )
		{
			continue;
		}

		int num_cores_per_node = -1;
		int real_stage_num = cur_stage_num;

		for ( j = 0; j < cur_stage_num; j++ )
		{
			hwloc_obj_t cur = hwloc_get_obj_by_depth( topology, i, j );
			const int num_cores = hwloc_get_nbobjs_inside_cpuset_by_type( topology, cur->cpuset, HWLOC_OBJ_PU );

			if ( num_cores == 0 )
			{
				real_stage_num -= 1;
				continue;
			}

			if ( num_cores_per_node == -1 )
			{
				num_cores_per_node = num_cores;
			}
			else
			{
				if ( num_cores_per_node != num_cores )
				{
					break;
				}
			}
		}

		if ( real_stage_num == 1 )
		{
			break;
		}

		if ( cur_stage_num != j )
		{
			continue;
		}

		if ( real_stage_num == last_stage_num || last_stage_num % real_stage_num )
		{
			continue;
		}

		reverse_shape[reverse_shape_depth] = real_stage_num;
		reverse_shape[reverse_shape_depth - 1] /= real_stage_num;
		reverse_shape_depth++;
	}

	if ( reverse_shape_depth > 1 )
	{
		__mpcomp_aux_reverse_one_dim_array( reverse_shape, reverse_shape_depth );
	}

	*shape_depth = reverse_shape_depth;
	return reverse_shape;
}

/* MOVE FROM mpcomp_tree_bis.c */
int __mpcomp_restrict_topology_for_mpcomp( hwloc_topology_t *restrictedTopology, const int omp_threads_expected, const int *cpulist )
{
	int i, err, num_mvps;
	hwloc_topology_t topology;
	hwloc_bitmap_t final_cpuset;
	hwloc_obj_t core_obj, pu_obj;
	final_cpuset = hwloc_bitmap_alloc();
	topology = mpc_topology_get();

	const int taskRank = mpc_common_get_task_rank();
	const int taskVp = mpc_thread_get_task_placement_and_count( taskRank, &num_mvps );

	const int __UNUSED__ npus = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_PU );
	const int __UNUSED__ ncores = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_CORE );
	// Every core must have the same number of PU
	sctk_assert( npus % ncores == 0 );

	if ( omp_threads_expected > 0 && num_mvps > omp_threads_expected )
	{
		num_mvps = omp_threads_expected;
	}

	if ( cpulist )
	{
		/* If smt is enable we use cpu_id as HT_id else core_id
		 * We can iterate on OBJ_PU directly for both configuration */
		assume( omp_threads_expected == num_mvps );

		/* Restrict core_id to core in OMP_PLACES */
		for ( i = 0; i < num_mvps; i++ )
		{
			const int cur_pu_id = cpulist[i];
			pu_obj = hwloc_get_obj_by_type( topology, HWLOC_OBJ_PU, cur_pu_id );
			sctk_assert( pu_obj );
			hwloc_bitmap_or( final_cpuset, final_cpuset, pu_obj->cpuset );
		}
	}
	else /* No places */
	{
		int pu;

		for ( pu = 0; pu < num_mvps; pu++ )
		{
			if ( mpc_common_get_flags()->enable_smt_capabilities )
			{
				core_obj = hwloc_get_obj_by_type( topology, HWLOC_OBJ_PU, taskVp + pu );
				pu_obj = core_obj;
			}
			else
			{
				core_obj = hwloc_get_obj_by_type( topology, HWLOC_OBJ_CORE, taskVp + pu );
				pu_obj = hwloc_get_obj_inside_cpuset_by_type( topology, core_obj->cpuset, HWLOC_OBJ_PU, 0 );
			}

			sctk_assert( core_obj );
			sctk_assert( pu_obj );
			hwloc_bitmap_or( final_cpuset, final_cpuset, pu_obj->cpuset );
		}
	}

	sctk_assert( num_mvps == hwloc_bitmap_weight( final_cpuset ) );

	if ( ( err = hwloc_topology_init( restrictedTopology ) ) )
	{
		return -1;
	}

	if ( ( err = hwloc_topology_dup( restrictedTopology, topology ) ) )
	{
		return -1;
	}

	if ( ( err = hwloc_topology_restrict( *restrictedTopology, final_cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES ) ) )
	{
		return -1;
	}

	hwloc_obj_t prev_pu, next_pu;
	prev_pu = NULL; // Init hwloc iterator

	while ( ( next_pu = hwloc_get_next_obj_by_type( *restrictedTopology, HWLOC_OBJ_PU, prev_pu ) ) )
	{
		prev_pu = next_pu;
	}

	hwloc_bitmap_free( final_cpuset );
	return 0;
}

static hwloc_topology_t
__mpcomp_prepare_omp_task_tree_init( const int num_mvps, const int *cpus_order )
{
	int err;
	hwloc_topology_t restrictedTopology;
	err = __mpcomp_restrict_topology_for_mpcomp( &restrictedTopology, num_mvps, cpus_order );
	assert( !err );
	sctk_assert( restrictedTopology );
	return restrictedTopology;
}

static void
__mpcomp_init_omp_task_tree( const int num_mvps, int *shape, const int *cpus_order, const mpcomp_local_icv_t icvs )
{
	int i, max_depth, place_depth = 0, place_size;
	int *tree_shape;
	hwloc_topology_t omp_task_topo;
	omp_task_topo = __mpcomp_prepare_omp_task_tree_init( num_mvps, cpus_order );
	tree_shape = __mpcomp_convert_topology_to_tree_shape( omp_task_topo, &max_depth );

	if ( shape ) // Force shape
	{
		int cumul = 1;

		/* Test if PLACES arity is in tree */
		for ( i = 0; i < max_depth; i++ )
		{
			cumul *= tree_shape[i];

			if ( cumul == shape[0] )
			{
				break;
			}
		}

		/* Not found */
		if ( i == max_depth )
		{
			max_depth = 2;
			place_depth = 0;
			tree_shape = shape;
			mpc_common_debug_log("Default Tree w/ places at level %d\n", place_depth );
		}
		else
		{
			place_depth = i;
			mpc_common_debug_log("Topological Tree w/ places at level %d\n", place_depth );
		}

		place_size = shape[1];
	}
	else
	{
		place_size = 1;
	}

	_mpc_omp_tree_alloc( tree_shape, max_depth, cpus_order, place_depth, place_size, &icvs );
}

/*
 * Read environment variables for OpenMP.
 * Actually, the values are read from configuration: those values can be
 * assigned with environement variable or config file.
 * This function is called once per process
 */
static inline void __mpcomp_read_env_variables()
{
	char *env;
	sctk_nodebug( "__mpcomp_read_env_variables: Read env vars (MPC rank: %d)",
	              mpc_common_get_task_rank() );
	/******* OMP_VP_NUMBER *********/
	OMP_MICROVP_NUMBER = sctk_runtime_config_get()->modules.openmp.vp;

	if ( OMP_MICROVP_NUMBER < 0 )
	{
		fprintf(
		    stderr,
		    "Warning: Incorrect number of microVPs (OMP_MICROVP_NUMBER=<%d>) -> "
		    "Switching to default value %d\n",
		    OMP_MICROVP_NUMBER, 0 );
		OMP_MICROVP_NUMBER = 0;
	}

	if ( OMP_MICROVP_NUMBER > mpc_topology_get_pu_count() )
	{
		sctk_warning("Number of microVPs should be at most the number "
		         "of cores per node: %d\n"
		         "Switching to default value %d\n",
		         mpc_topology_get_pu_count(), OMP_MICROVP_NUMBER );
		OMP_MICROVP_NUMBER = 0;
	}

	/******* OMP_SCHEDULE *********/
	env = sctk_runtime_config_get()->modules.openmp.schedule;

	OMP_SCHEDULE = omp_sched_static; /* DEFAULT */

	if ( env != NULL )
	{
		int ok = 0;
		int offset = 0;

		if ( strncmp( env, "static", 6 ) == 0 )
		{
			OMP_SCHEDULE = omp_sched_static;
			offset = 6;
			ok = 1;
		}

		if ( strncmp( env, "dynamic", 7 ) == 0 )
		{
			OMP_SCHEDULE = omp_sched_dynamic;
			offset = 7;
			ok = 1;
		}

		if ( strncmp( env, "guided", 6 ) == 0 )
		{
			OMP_SCHEDULE = omp_sched_guided;
			offset = 6;
			ok = 1;
		}

		if ( strncmp( env, "auto", 4 ) == 0 )
		{
			OMP_SCHEDULE = omp_sched_auto;
			offset = 4;
			ok = 1;
		}

		if ( ok )
		{
			int chunk_size = 0;
			/* Check for chunk size, if present */
			sctk_nodebug( "Remaining string for schedule: <%s>", &env[offset] );

			switch ( env[offset] )
			{
				case ',':
					sctk_nodebug( "There is a chunk size -> <%s>", &env[offset + 1] );
					chunk_size = atoi( &env[offset + 1] );

					if ( chunk_size <= 0 )
					{
						sctk_warning("Incorrect chunk size within OMP_SCHEDULE "
						         "variable: <%s>\n",
						         env );
						chunk_size = 0;
					}
					else
					{
						OMP_MODIFIER_SCHEDULE = chunk_size;
					}

					break;

				case '\0':
					sctk_nodebug( "No chunk size\n" );
					break;

				default:
					fprintf( stderr,
					         "Syntax error with environment variable OMP_SCHEDULE <%s>,"
					         " should be \"static|dynamic|guided|auto[,chunk_size]\"\n",
					         env );
					exit( 1 );
			}
		}
		else
		{
			sctk_warning("Unknown schedule <%s> (must be static, guided, "
			         "dynamic or auto),"
			         " fallback to default schedule <%d>\n",
			         env, OMP_SCHEDULE );
		}
	}

	/******* OMP_NUM_THREADS *********/
	OMP_NUM_THREADS = sctk_runtime_config_get()->modules.openmp.nb_threads;

	if ( OMP_NUM_THREADS < 0 )
	{
		OMP_NUM_THREADS = 0;
	}

	TODO( "If OMP_NUM_THREADS is 0, let it equal to 0 by default and handle it "
	      "later" )

	if ( OMP_NUM_THREADS == 0 )
	{
		OMP_NUM_THREADS = OMP_MICROVP_NUMBER; /* DEFAULT */
	}

	TODO( "OMP_NUM_THREADS: need to handle x,y,z,... and keep only x" )
	/******* OMP_DYNAMIC *********/
	OMP_DYNAMIC = sctk_runtime_config_get()->modules.openmp.adjustment ? 1 : 0;
	/******* OMP_PROC_BIND *********/
	OMP_PROC_BIND = sctk_runtime_config_get()->modules.openmp.proc_bind ? 1 : 0;
	/******* OMP_NESTED *********/
	OMP_NESTED = sctk_runtime_config_get()->modules.openmp.nested ? 1 : 0;
	/******* OMP_STACKSIZE *********/
	OMP_STACKSIZE = sctk_runtime_config_get()->modules.openmp.stack_size;

	if ( OMP_STACKSIZE == 0 )
	{
		if ( mpc_common_get_flags()->is_fortran == 1 )
		{
			OMP_STACKSIZE = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
		}
		else
		{
			OMP_STACKSIZE = SCTK_ETHREAD_STACK_SIZE;
		}
	}

	/******* OMP_WAIT_POLICY *********/
	OMP_WAIT_POLICY = sctk_runtime_config_get()->modules.openmp.wait_policy;
	/******* OMP_THREAD_LIMIT *********/
	OMP_THREAD_LIMIT = sctk_runtime_config_get()->modules.openmp.thread_limit;
	/******* OMP_MAX_ACTIVE_LEVELS *********/
	OMP_MAX_ACTIVE_LEVELS =
	    sctk_runtime_config_get()->modules.openmp.max_active_levels;
	/******* ADDITIONAL VARIABLES *******/
	/******* OMP_WARN_NESTED *******/
	OMP_WARN_NESTED = sctk_runtime_config_get()->modules.openmp.warn_nested;
	/******* OMP_MODE *******/
	env = sctk_runtime_config_get()->modules.openmp.mode;
	OMP_MODE = MPCOMP_MODE_SIMPLE_MIXED;

	if ( env != NULL )
	{
		int ok = 0;

		/* Handling MPCOMP_MODE_SIMPLE_MIXED */
		if ( strncmp( env, "simple-mixed", strlen( "simple-mixed" ) ) == 0 ||
		     strncmp( env, "SIMPLE-MIXED", strlen( "SIMPLE-MIXED" ) ) == 0 ||
		     strncmp( env, "simple_mixed", strlen( "simple_mixed" ) ) == 0 ||
		     strncmp( env, "SIMPLE_MIXED", strlen( "SIMPLE_MIXED" ) ) == 0 )
		{
			OMP_MODE = MPCOMP_MODE_SIMPLE_MIXED;
			ok = 1;
		}

		/* Handling MPCOMP_MODE_OVERSUBSCRIBED_MIXED */
		if ( strncmp( env, "alternating", strlen( "alternating" ) ) == 0 ||
		     strncmp( env, "ALTERNATING", strlen( "ALTERNATING" ) ) == 0 )
		{
			OMP_MODE = MPCOMP_MODE_ALTERNATING;
			ok = 1;
		}

		/* Handling MPCOMP_MODE_ALTERNATING */
		if ( strncmp( env, "oversubscribed-mixed", strlen( "oversubscribed-mixed" ) ) ==
		     0 ||
		     strncmp( env, "OVERSUBSCRIBED-MIXED", strlen( "OVERSUBSCRIBED-MIXED" ) ) ==
		     0 ||
		     strncmp( env, "oversubscribed_mixed", strlen( "oversubscribed_mixed" ) ) ==
		     0 ||
		     strncmp( env, "OVERSUBSCRIBED_MIXED", strlen( "OVERSUBSCRIBED_MIXED" ) ) ==
		     0 )
		{
			OMP_MODE = MPCOMP_MODE_OVERSUBSCRIBED_MIXED;
			ok = 1;
		}

		/* Handling MPCOMP_MODE_FULLY_MIXED */
		if ( strncmp( env, "fully-mixed", strlen( "fully-mixed" ) ) == 0 ||
		     strncmp( env, "FULLY-MIXED", strlen( "FULLY-MIXED" ) ) == 0 ||
		     strncmp( env, "fully_mixed", strlen( "fully_mixed" ) ) == 0 ||
		     strncmp( env, "FULLY_MIXED", strlen( "FULLY_MIXED" ) ) == 0 )
		{
			OMP_MODE = MPCOMP_MODE_FULLY_MIXED;
			ok = 1;
		}

		if ( ok )
		{
		}
		else
		{
			sctk_warning("Unknown mode <%s> (must be SIMPLE_MIXED, "
			         "ALTERNATING, OVERSUBSCRIBED_MIXED or FULLY_MIXED),"
			         " fallback to default mode <%d>\n",
			         env, OMP_MODE );
		}
	}

	/******* OMP_AFFINITY *******/
	env = sctk_runtime_config_get()->modules.openmp.affinity;

	if ( env != NULL )
	{
		int ok = 0;

		/* Handling MPCOMP_AFFINITY_COMPACT */
		if ( strncmp( env, "compact", strlen( "compact" ) ) == 0 ||
		     strncmp( env, "COMPACT", strlen( "COMPACT" ) ) == 0 )
		{
			OMP_AFFINITY = MPCOMP_AFFINITY_COMPACT;
			ok = 1;
		}

		/* Handling MPCOMP_AFFINITY_SCATTER */
		if ( strncmp( env, "scatter", strlen( "scatter" ) ) == 0 ||
		     strncmp( env, "SCATTER", strlen( "SCATTER" ) ) == 0 )
		{
			OMP_AFFINITY = MPCOMP_AFFINITY_SCATTER;
			ok = 1;
		}

		/* Handling MPCOMP_AFFINITY_BALANCED */
		if ( strncmp( env, "balanced", strlen( "balanced" ) ) == 0 ||
		     strncmp( env, "BALANCED", strlen( "BALANCED" ) ) == 0 )
		{
			OMP_AFFINITY = MPCOMP_AFFINITY_BALANCED;
			ok = 1;
		}

		if ( ok )
		{
		}
		else
		{
			sctk_warning("Unknown affinity <%s> (must be COMPACT, "
			         "SCATTER or BALANCED),"
			         " fallback to default affinity <%d>\n",
			         env, OMP_AFFINITY );
		}
	}

	OMP_AFFINITY = MPCOMP_AFFINITY_SCATTER;
	/******* OMP_TREE *********/
	env = sctk_runtime_config_get()->modules.openmp.tree;

	if ( strlen( env ) != 0 )
	{
		int nb_tokens = 0;
		char **tokens = NULL;
		int i;
		tokens = __mpcomp_tokenizer( env, &nb_tokens );
		sctk_assert( tokens != NULL );
		/* TODO check that arguments are ok and #leaves is correct */
		OMP_TREE = ( int * ) mpcomp_alloc( nb_tokens * sizeof( int ) );
		OMP_TREE_DEPTH = nb_tokens;
		OMP_TREE_NB_LEAVES = 1;

		for ( i = 0; i < nb_tokens; i++ )
		{
			OMP_TREE[i] = atoi( tokens[i] );
			OMP_TREE_NB_LEAVES *= OMP_TREE[i];
		}

		TODO( "check the env variable OMP_TREE" )
	}
	else
	{
		OMP_TREE = NULL;
	}

	/***** PRINT SUMMARY (only first MPI rank) ******/
	if ( getenv( "MPC_DISABLE_BANNER" ) == NULL && mpc_common_get_task_rank() == 0 )
	{
                mpc_common_debug_log("--------------------------------------------------");
		mpc_common_debug_log(
		         "MPC OpenMP version %d.%d (Intel and Patched GCC compatibility)",
		         SCTK_OMP_VERSION_MAJOR, SCTK_OMP_VERSION_MINOR );
#if MPCOMP_TASK
		mpc_common_debug_log("\tOpenMP 3 Tasking on:\n"
                "\t\tOMP_NEW_TASKS_DEPTH=%d\n"
                "\t\tOMP_TASK_LARCENY_MODE=%d\n"
		"\t\tOMP_TASK_NESTING_MAX=%d\n"
                "\t\tOMP_TASK_MAX_DELAYED=%d\n"
                "\t\tOMP_TASK_USE_LOCKFREE_QUEUE=%d\n"
                "\t\tOMP_TASK_STEAL_LAST_STOLEN_LIST=%d\n"
                "\t\tOMP_TASK_RESTEAL_TO_LAST_THIEF=%d",
		         sctk_runtime_config_get()->modules.openmp.omp_new_task_depth,
		         sctk_runtime_config_get()->modules.openmp.omp_task_larceny_mode,
		         sctk_runtime_config_get()->modules.openmp.omp_task_nesting_max,
		         sctk_runtime_config_get()->modules.openmp.mpcomp_task_max_delayed,
		         sctk_runtime_config_get()->modules.openmp.omp_task_use_lockfree_queue,
		         sctk_runtime_config_get()->modules.openmp.omp_task_steal_last_stolen_list,
		         sctk_runtime_config_get()->modules.openmp.omp_task_resteal_to_last_thief );
#ifdef MPCOMP_USE_TASKDEP
		mpc_common_debug_log("\t\tDependencies ON" );
#else
		mpc_common_debug_log("\t\tDependencies OFF" );
#endif /* MPCOMP_USE_TASKDEP */
#else
		mpc_common_debug_log("\tOpenMP 3 Tasking off");
#endif
		mpc_common_debug_log("\tOMP_SCHEDULE %d", OMP_SCHEDULE );

		if ( OMP_NUM_THREADS == 0 )
		{
			mpc_common_debug_log("\tDefault #threads (OMP_NUM_THREADS)" );
		}
		else
		{
			mpc_common_debug_log("\tOMP_NUM_THREADS %d", OMP_NUM_THREADS );
		}

		mpc_common_debug_log("\tOMP_DYNAMIC %d", OMP_DYNAMIC );
		mpc_common_debug_log("\tOMP_NESTED %d", OMP_NESTED );

		if ( OMP_MICROVP_NUMBER == 0 )
		{
			mpc_common_debug_log("\tDefault #microVPs (OMP_MICROVP_NUMBER)" );
		}
		else
		{
			mpc_common_debug_log("\t%d microVPs (OMP_MICROVP_NUMBER)",
			         OMP_MICROVP_NUMBER );
		}

		switch ( OMP_AFFINITY )
		{
			case MPCOMP_AFFINITY_COMPACT:
				mpc_common_debug_log("\tAffinity COMPACT (fill logical cores first)" );
				break;

			case MPCOMP_AFFINITY_SCATTER:
				mpc_common_debug_log("\tAffinity SCATTER (spread over NUMA nodes)" );
				break;

			case MPCOMP_AFFINITY_BALANCED:
				mpc_common_debug_log("\tAffinity BALANCED (fill physical cores first)" );
				break;

			default:
				mpc_common_debug_log("\tAffinity Unknown" );
				break;
		}

		if ( OMP_TREE != NULL )
		{
			int i;
			mpc_common_debug_log("\tOMP_TREE w/ depth:%d leaves:%d, arity:[%d",
			         OMP_TREE_DEPTH, OMP_TREE_NB_LEAVES, OMP_TREE[0] );

			for ( i = 1; i < OMP_TREE_DEPTH; i++ )
			{
				mpc_common_debug_log(", %d", OMP_TREE[i] );
			}

			mpc_common_debug_log("]" );
		}
		else
		{
			mpc_common_debug_log("\tOMP_TREE default" );
		}

		switch ( OMP_MODE )
		{
			case MPCOMP_MODE_SIMPLE_MIXED:
				mpc_common_debug_log("\tMode for hybrid MPI+OpenMP parallelism SIMPLE_MIXED" );
				break;

			case MPCOMP_MODE_OVERSUBSCRIBED_MIXED:
				mpc_common_debug_log("\tMode for hybrid MPI+OpenMP parallelism OVERSUBSCRIBED_MIXED" );
				break;

			case MPCOMP_MODE_ALTERNATING:
				mpc_common_debug_log("\tMode for hybrid MPI+OpenMP parallelism ALTERNATING" );
				break;

			case MPCOMP_MODE_FULLY_MIXED:
				mpc_common_debug_log("\tMode for hybrid MPI+OpenMP parallelism FULLY_MIXED" );
				break;

			default:
				not_reachable();
				break;
		}

#if MPCOMP_MALLOC_ON_NODE
		mpc_common_debug_log("\tNUMA allocation for tree nodes" );
#endif
#if MPCOMP_COHERENCY_CHECKING
		mpc_common_debug_log("\tCoherency check enabled" );
#endif
#if MPCOMP_ALIGN
		mpc_common_debug_log("\tStructure field alignement" );
#endif

		if ( OMP_WARN_NESTED )
		{
			mpc_common_debug_log("\tOMP_WARN_NESTED %d (breakpoint mpcomp_warn_nested)",
			         OMP_WARN_NESTED );
		}
		else
		{
			mpc_common_debug_log("\tNo warning for nested parallelism" );
		}

#if MPCOMP_MIC
		mpc_common_debug_log("\tMIC optimizations on\n" );
#endif
#if OMPT_SUPPORT
		mpc_common_debug_log("\tOMPT Support ON\n" );
#else
		mpc_common_debug_log("\tOMPT Support OFF\n" );
#endif
		TODO( "Add every env vars when printing info on OpenMP" )
                mpc_common_debug_log("--------------------------------------------------");
	}
}

/* Initialization of the OpenMP runtime
   Called during the initialization of MPC
 */
void __mpcomp_init( void )
{
	static volatile int done = 0;
	static mpc_common_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
	int nb_mvps;
	int task_rank;

	/* Do we need to initialize the current team */
	if ( sctk_openmp_thread_tls )
	{
		return;
	}

#if OMPT_SUPPORT
	_mpc_ompt_pre_init();
#endif /* OMPT_SUPPORT */

	mpcomp_local_icv_t icvs;
	/* Need to initialize the whole runtime (environment variables) This
	* section is shared by every OpenMP instances among MPI tasks located inside
	* the same OS process */
	mpc_common_spinlock_lock( &lock );

	if ( done == 0 )
	{
		__mpcomp_read_env_variables();
		mpcomp_global_icvs.def_sched_var = omp_sched_static;
		mpcomp_global_icvs.bind_var = OMP_PROC_BIND;
		mpcomp_global_icvs.stacksize_var = OMP_STACKSIZE;
		mpcomp_global_icvs.active_wait_policy_var = OMP_WAIT_POLICY;
		mpcomp_global_icvs.thread_limit_var = OMP_THREAD_LIMIT;
		mpcomp_global_icvs.max_active_levels_var = OMP_MAX_ACTIVE_LEVELS;
		mpcomp_global_icvs.nmicrovps_var = OMP_MICROVP_NUMBER;
		mpcomp_global_icvs.warn_nested = OMP_WARN_NESTED;
		mpcomp_global_icvs.affinity = OMP_AFFINITY;
		done = 1;
	}

	/* Get the rank of current MPI task */
	task_rank = mpc_common_get_task_rank();

	if ( task_rank == -1 )
	{
		/* No parallel OpenMP if MPI has not been initialized yet */
		nb_mvps = 1;
	}
	else
	{
		/* Compute the number of microVPs according to Hybrid Mode */
		switch ( OMP_MODE )
		{
			case MPCOMP_MODE_SIMPLE_MIXED:
				/* Compute the number of cores for this task */
				mpc_thread_get_task_placement_and_count( task_rank, &nb_mvps );
				sctk_nodebug( "[%d] %s: SIMPLE_MIXED -> #mvps = %d", task_rank, __func__,
				              nb_mvps );

				/* Consider the env variable if between 1 and the number
				* of cores for this task */
				if ( OMP_MICROVP_NUMBER > 0 && OMP_MICROVP_NUMBER <= nb_mvps )
				{
					nb_mvps = OMP_MICROVP_NUMBER;
				}

				break;

			case MPCOMP_MODE_ALTERNATING:
				nb_mvps = 1;

				if ( mpc_common_get_local_task_rank() == 0 )
                                {
					nb_mvps = mpc_topology_get_pu_count();
				}

				break;

			case MPCOMP_MODE_OVERSUBSCRIBED_MIXED:
				not_implemented();
				break;

			case MPCOMP_MODE_FULLY_MIXED:
				nb_mvps = mpc_topology_get_pu_count();
				break;

			default:
				not_reachable();
				break;
		}
	}

	/* DEBUG */
	if ( mpc_common_get_local_task_rank() == 0 )
	{
		mpc_common_debug(
		    "%s: MPI rank=%d, process_rank=%d, local_task_rank=%d => %d mvp(s) "
		    "out of %d core(s) A",
		    __func__, task_rank, mpc_common_get_local_process_rank(),
		    mpc_common_get_local_task_rank(), mpc_topology_get_pu_count(),
		    mpc_topology_get_pu_count() );
	}
	else
	{
		mpc_common_debug( "%s: MPI rank=%d, process_rank=%d, local_task_rank=%d => %d "
		            "mvp(s) out of %d core(s)",
		            __func__, task_rank, mpc_common_get_local_process_rank(),
		            mpc_common_get_local_task_rank(), nb_mvps,
		            mpc_topology_get_pu_count() );
	}

	/* Update some global icvs according the number of mvps */
	if ( mpcomp_global_icvs.thread_limit_var == 0 ||
	     mpcomp_global_icvs.thread_limit_var > nb_mvps )
	{
		mpcomp_global_icvs.thread_limit_var = nb_mvps;
	}

	if ( mpcomp_global_icvs.nmicrovps_var == 0 )
	{
		mpcomp_global_icvs.nmicrovps_var = nb_mvps;
	}

	if ( mpcomp_global_icvs.max_active_levels_var == 0 )
	{
		mpcomp_global_icvs.max_active_levels_var = 1;
	}

	/* Initialize default ICVs */
	if ( OMP_NUM_THREADS == 0 )
	{
		icvs.nthreads_var = nb_mvps;
	}
	else
	{
		icvs.nthreads_var = OMP_NUM_THREADS;
	}

	icvs.dyn_var = OMP_DYNAMIC;
	icvs.nest_var = OMP_NESTED;
	icvs.run_sched_var = OMP_SCHEDULE;
	icvs.modifier_sched_var = OMP_MODIFIER_SCHEDULE;
	icvs.active_levels_var = 0;
	icvs.levels_var = 0;
#if OMPT_SUPPORT
	_mpc_ompt_post_init();
#endif /* OMPT_SUPPORT */

	int places_nb_mvps;
	int *shape, *cpus_order;
	OMP_PLACES_LIST = mpcomp_places_env_variable_parsing( nb_mvps );

	if ( OMP_PLACES_LIST )
	{
		places_nb_mvps = mpcomp_places_get_topo_info( OMP_PLACES_LIST, &shape, &cpus_order );
		assert( places_nb_mvps <= nb_mvps );
		nb_mvps = places_nb_mvps;
	}
	else
	{
		shape = NULL;
		cpus_order = NULL;
	}

	__mpcomp_init_omp_task_tree( nb_mvps, shape, cpus_order, icvs );
#if MPCOMP_TASK
	//    _mpc_task_team_info_init(team_info, instance->tree_depth);
#endif /* MPCOMP_TASK */
	mpc_common_spinlock_unlock( &lock );
}

void __mpcomp_exit( void )
{
#if MPCOMP_TASK
	//     _mpc_task_new_exit();
#endif /* MPCOMP_TASK */
}

void __mpcomp_in_order_scheduler( mpcomp_thread_t *thread )
{
	mpcomp_loop_long_iter_t *loop;
	sctk_assert( thread );
	sctk_assert( thread->instance );
	sctk_assert( thread->info.func );
	sctk_assert( thread->instance->team );
	loop = &( thread->info.loop_infos.loop.mpcomp_long );

	if ( thread->info.combined_pragma < 0 || thread->info.combined_pragma > 10 )
	{
		sctk_nodebug( stderr, "[%d/%ld] Start %s :: %ld\n", thread->mvp->global_rank, thread->rank, __func__, thread->info.combined_pragma );
	}

	/* Handle beginning of combined parallel region */
	switch ( thread->info.combined_pragma )
	{
		case MPCOMP_COMBINED_NONE:
			break;

		case MPCOMP_COMBINED_SECTION:
			__mpcomp_sections_init( thread, thread->info.nb_sections );
			break;

		case MPCOMP_COMBINED_STATIC_LOOP:
			__mpcomp_static_loop_init( thread, loop->lb, loop->b, loop->incr, loop->chunk_size );
			break;

		case MPCOMP_COMBINED_DYN_LOOP:
			__mpcomp_dynamic_loop_init( thread, loop->lb, loop->b, loop->incr, loop->chunk_size );
			break;

		case MPCOMP_COMBINED_GUIDED_LOOP:
			__mpcomp_guided_loop_begin( loop->lb, loop->b, loop->incr, loop->chunk_size, NULL, NULL );
			break;

		default:
			not_implemented();
	}

#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_implicit_task_t callback;
			callback = ( ompt_callback_implicit_task_t ) OMPT_Callbacks[ompt_callback_implicit_task];

			if ( callback )
			{
				ompt_data_t *parallel_data = &( thread->instance->team->info.ompt_region_data );
                ompt_data_t* task_data = &( thread->task_infos.current_task->ompt_task_data );

				callback( ompt_scope_begin, parallel_data, task_data, thread->instance->nb_mvps, thread->rank, ompt_task_implicit );  
			}
		}
	}

#endif /* OMPT_SUPPORT */
	thread->info.func( thread->info.shared );
#if OMPT_SUPPORT

	if ( _mpc_omp_ompt_is_enabled() )
	{
		if ( OMPT_Callbacks )
		{
			ompt_callback_implicit_task_t callback;
			callback = ( ompt_callback_implicit_task_t ) OMPT_Callbacks[ompt_callback_implicit_task];

			if ( callback )
			{
                ompt_data_t* task_data = &( thread->task_infos.current_task->ompt_task_data );

				callback( ompt_scope_end, NULL, task_data, thread->instance->nb_mvps, thread->rank, ompt_task_implicit );
			}
		}
	}

#endif /* OMPT_SUPPORT */

	/* Handle ending of combined parallel region */
	switch ( thread->info.combined_pragma )
	{
		case MPCOMP_COMBINED_NONE:
		case MPCOMP_COMBINED_SECTION:
		case MPCOMP_COMBINED_STATIC_LOOP:
			break;

		case MPCOMP_COMBINED_DYN_LOOP:
			__mpcomp_dynamic_loop_end_nowait();
			break;

		case MPCOMP_COMBINED_GUIDED_LOOP:
			__mpcomp_guided_loop_end_nowait();
			break;

		default:
			not_implemented();
			break;
	}
}
