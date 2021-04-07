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
#include "mpc_common_debug.h"
#include "mpc_topology.h"

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
#include "mpc_conf.h"
#include "mpc_thread.h"

#include "ompt.h"
/* parsing OMP_PLACES */
#include "mpcomp_places_env.h"


/*************************
 * MPC_OMP CONFIGURATION *
 *************************/

static struct mpc_omp_conf __omp_conf = { 0 };

struct mpc_omp_conf * mpc_omp_conf_get(void)
{
	return &__omp_conf;
}

static inline void __omp_conf_set_default(void)
{
	/* This is the chunk size as schedule modifier */

	__omp_conf.OMP_MAX_ACTIVE_LEVELS=0;
	__omp_conf.OMP_WARN_NESTED=0;
	snprintf(__omp_conf.places, MPC_CONF_STRING_SIZE, "cores");
	snprintf(__omp_conf.allocator, MPC_CONF_STRING_SIZE, "omp_default_mem_alloc");

	/* Schedule */
	__omp_conf.OMP_MODIFIER_SCHEDULE = 0;

	/* Threads */
	__omp_conf.OMP_NUM_THREADS = 0;
	__omp_conf.OMP_NESTED = 0;
	__omp_conf.OMP_MICROVP_NUMBER = 0;
	__omp_conf.OMP_PROC_BIND=1;
	__omp_conf.OMP_WAIT_POLICY=0;
	__omp_conf.OMP_THREAD_LIMIT=64;
	__omp_conf.OMP_DYNAMIC=0;

	/* Tasks */

	__omp_conf.omp_new_task_depth = 10;
	__omp_conf.omp_untied_task_depth = 10;
	snprintf(__omp_conf.omp_task_larceny_mode_str, MPC_CONF_STRING_SIZE, "producer");
	__omp_conf.omp_task_larceny_mode = MPCOMP_TASK_LARCENY_MODE_PRODUCER;
	__omp_conf.omp_task_nesting_max = 1000000;
	__omp_conf.mpcomp_task_max_delayed = 1024;
	__omp_conf.omp_task_use_lockfree_queue = 1;
	__omp_conf.omp_task_steal_last_stolen_list = 0;
	__omp_conf.omp_task_resteal_to_last_thief = 0;

}


static inline void __omp_conf_init(void)
{
	__omp_conf_set_default();

	mpc_conf_config_type_t *schedule = mpc_conf_config_type_init("schedule",
	                                                       PARAM("chunk", &__omp_conf.OMP_MODIFIER_SCHEDULE , MPC_CONF_INT, "This is the chunk size as schedule modifier"),
														   NULL);

	mpc_conf_config_type_t *thread = mpc_conf_config_type_init("thread",
	                                PARAM("number", &__omp_conf.OMP_NUM_THREADS , MPC_CONF_INT, "This is the number of threads OMP_NUM_THREADS"),
									PARAM("nested", &__omp_conf.OMP_NESTED , MPC_CONF_BOOL, "Is nested parallelism enabled OMP_NESTED"),
									PARAM("vpcount", &__omp_conf.OMP_MICROVP_NUMBER , MPC_CONF_INT, "Number of VP for each OMP team OMP_MICROVP_NUMBER"),
									PARAM("bind", &__omp_conf.OMP_PROC_BIND , MPC_CONF_BOOL, "Bind OMP threads to cores OMP_PROC_BIND"),
									PARAM("stacksize", &__omp_conf.OMP_STACKSIZE , MPC_CONF_INT, "Stack size for OpenMP threads OMP_STACKSIZE"),
									PARAM("waitpolicy", &__omp_conf.OMP_WAIT_POLICY , MPC_CONF_INT, "Behavior of threads while waiting OMP_WAIT_POLICY"),
									PARAM("maxthreads", &__omp_conf.OMP_THREAD_LIMIT , MPC_CONF_INT, "Max number of threads OMP_THREAD_LIMIT"),
									PARAM("dynamic", &__omp_conf.OMP_DYNAMIC , MPC_CONF_INT, "Allow dynamic adjustment of the thread number OMP_DYNAMIC"),
									NULL);

	mpc_conf_config_type_t *task = mpc_conf_config_type_init("task",
	                                PARAM("depth", &__omp_conf.omp_new_task_depth , MPC_CONF_INT, "Depth of the new tasks lists in the tree"),
	                                PARAM("untieddepth", &__omp_conf.omp_untied_task_depth , MPC_CONF_INT, "Depth of the untied tasks lists in the tree"),
									PARAM("larceny", &__omp_conf.omp_task_larceny_mode_str , MPC_CONF_STRING, "Task stealing policy (hierarchical, random, random_order, round_robin, producer, producer_order, hierarchical_random)"),
	                                PARAM("maxnesting", &__omp_conf.omp_task_nesting_max , MPC_CONF_INT, "Task max depth in task generation"),
	                                PARAM("maxdelayed", &__omp_conf.mpcomp_task_max_delayed , MPC_CONF_INT, "Max tasks in mpcomp list"),
									PARAM("lockfreequeue", &__omp_conf.omp_task_use_lockfree_queue , MPC_CONF_BOOL, "Use a lock-free queue for tasks"),
									PARAM("lastlist", &__omp_conf.omp_task_steal_last_stolen_list , MPC_CONF_BOOL, "Try to steal to same list than last successful stealing"),
									PARAM("lastthief", &__omp_conf.omp_task_resteal_to_last_thief , MPC_CONF_BOOL, "Try to steal to the last thread that stole a task to current thread"),
									NULL);

	mpc_conf_config_type_t *omp = mpc_conf_config_type_init("omp",
	                                PARAM("number", &__omp_conf.OMP_MAX_ACTIVE_LEVELS , MPC_CONF_INT, "Maximum depth of nested parallelism"),
									PARAM("nested", &__omp_conf.OMP_WARN_NESTED , MPC_CONF_BOOL, "Emit warning when entering nested parallelism"),
									PARAM("places", &__omp_conf.places , MPC_CONF_STRING, "OpenMP Places configuration OMP_PLACES"),
									PARAM("allocator", &__omp_conf.allocator , MPC_CONF_STRING, "OpenMP allocator"),
									PARAM("schedule", schedule , MPC_CONF_TYPE, "Parameters associated with OpenMP schedules"),
									PARAM("thread", thread , MPC_CONF_TYPE, "Parameters associated with OpenMP threads"),
									PARAM("task", task , MPC_CONF_TYPE, "Parameters associated with OpenMP tasks"),
									NULL);

	mpc_conf_root_config_append("mpcframework", omp, "MPC OpenMP Configuration");

}



void mpc_openmp_registration() __attribute__( (constructor) );

void mpc_openmp_registration()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("Config Sources", "MPC_OMP Init", __omp_conf_init, 32);

}

/*****************
  ****** GLOBAL VARIABLES
  *****************/

/* Schedule type */
static omp_sched_t OMP_SCHEDULE = omp_sched_static;
/* Default tree for OpenMP instances? (number of nodes per level) */
static int *OMP_TREE = NULL;

/* Number of level for the default tree (size of OMP_TREE) */
static int OMP_TREE_DEPTH = 0;
/* Total number of leaves for the tree (product of OMP_TREE elements) */
static int OMP_TREE_NB_LEAVES = 0;

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
	assert( new_argv != NULL );
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
	assert( max_depth > 1 );
	reverse_shape = ( int * ) mpcomp_alloc( sizeof( int ) * max_depth );
	assert( reverse_shape );
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
	assert( npus % ncores == 0 );

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
			assert( pu_obj );
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

			assert( core_obj );
			assert( pu_obj );
			hwloc_bitmap_or( final_cpuset, final_cpuset, pu_obj->cpuset );
		}
	}

	assert( num_mvps == hwloc_bitmap_weight( final_cpuset ) );

	if ( ( err = hwloc_topology_init( restrictedTopology ) ) )
	{
		return -1;
	}

	if ( ( err = hwloc_topology_dup( restrictedTopology, topology ) ) )
	{
		return -1;
	}

#if (HWLOC_API_VERSION < 0x00020000)
	if ( ( err = hwloc_topology_restrict( *restrictedTopology, final_cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES ) ) )
#else
	if ( ( err = hwloc_topology_restrict( *restrictedTopology, final_cpuset, 0 ) ) )
#endif
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
	assert( restrictedTopology );
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
	/* Ensure larceny mode for tasks is read from string and then locked */
	__omp_conf.omp_task_larceny_mode = mpcomp_task_parse_larceny_mode(__omp_conf.omp_task_larceny_mode_str);
	/* We lock as we wont parse future updates */
	mpc_conf_root_config_set_lock("mpcframework.omp.task.larceny", 1);

	char *env;
	mpc_common_nodebug( "__mpcomp_read_env_variables: Read env vars (MPC rank: %d)",
	              mpc_common_get_task_rank() );
	/******* OMP_VP_NUMBER *********/
	if ( __omp_conf.OMP_MICROVP_NUMBER < 0 )
	{
		fprintf(
		    stderr,
		    "Warning: Incorrect number of microVPs (OMP_MICROVP_NUMBER=<%d>) -> "
		    "Switching to default value %d\n",
		    __omp_conf.OMP_MICROVP_NUMBER, 0 );
		__omp_conf.OMP_MICROVP_NUMBER = 0;
	}

	if ( __omp_conf.OMP_MICROVP_NUMBER > mpc_topology_get_pu_count() )
	{
		mpc_common_debug_warning("Number of microVPs should be at most the number "
		         "of cores per node: %d\n"
		         "Switching to default value %d\n",
		         mpc_topology_get_pu_count(), __omp_conf.OMP_MICROVP_NUMBER );
		__omp_conf.OMP_MICROVP_NUMBER = 0;
	}

	/******* OMP_SCHEDULE *********/
	env = getenv("OMP_SCHEDULE");

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
			mpc_common_nodebug( "Remaining string for schedule: <%s>", &env[offset] );

			switch ( env[offset] )
			{
				case ',':
					mpc_common_nodebug( "There is a chunk size -> <%s>", &env[offset + 1] );
					chunk_size = atoi( &env[offset + 1] );

					if ( chunk_size <= 0 )
					{
						mpc_common_debug_warning("Incorrect chunk size within OMP_SCHEDULE "
						         "variable: <%s>\n",
						         env );
						chunk_size = 0;
					}
					else
					{
						__omp_conf.OMP_MODIFIER_SCHEDULE = chunk_size;
					}

					break;

				case '\0':
					mpc_common_nodebug( "No chunk size\n" );
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
			mpc_common_debug_warning("Unknown schedule <%s> (must be static, guided, "
			         "dynamic or auto),"
			         " fallback to default schedule <%d>\n",
			         env, OMP_SCHEDULE );
		}
	}

	/******* OMP_NUM_THREADS *********/
	env = getenv("OMP_NUM_THREADS");

	if(env)
	{
		__omp_conf.OMP_NUM_THREADS = atoi(env);
	}

	if ( __omp_conf.OMP_NUM_THREADS < 0 )
	{
		__omp_conf.OMP_NUM_THREADS = 0;
	}

	TODO( "If OMP_NUM_THREADS is 0, let it equal to 0 by default and handle it "
	      "later" )

	if ( __omp_conf.OMP_NUM_THREADS == 0 )
	{
		__omp_conf.OMP_NUM_THREADS = __omp_conf.OMP_MICROVP_NUMBER; /* DEFAULT */
	}

	TODO( "OMP_NUM_THREADS: need to handle x,y,z,... and keep only x" )

	/******* OMP_STACKSIZE *********/

	env = getenv("OMP_STACKSIZE");

	if(env)
	{
		__omp_conf.OMP_STACKSIZE = atoi(env);
	}

	if ( __omp_conf.OMP_STACKSIZE == 0 )
	{
		if ( mpc_common_get_flags()->is_fortran == 1 )
		{
			__omp_conf.OMP_STACKSIZE = SCTK_ETHREAD_STACK_SIZE_FORTRAN;
		}
		else
		{
			__omp_conf.OMP_STACKSIZE = SCTK_ETHREAD_STACK_SIZE;
		}
	}
	/******* ADDITIONAL VARIABLES *******/
	/******* OMP_MODE *******/
	env = getenv("OMP_MODE");
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
			mpc_common_debug_warning("Unknown mode <%s> (must be SIMPLE_MIXED, "
			         "ALTERNATING, OVERSUBSCRIBED_MIXED or FULLY_MIXED),"
			         " fallback to default mode <%d>\n",
			         env, OMP_MODE );
		}
	}

	/******* OMP_AFFINITY *******/

	env = getenv("OMP_AFFINITY");

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
			mpc_common_debug_warning("Unknown affinity <%s> (must be COMPACT, "
			         "SCATTER or BALANCED),"
			         " fallback to default affinity <%d>\n",
			         env, OMP_AFFINITY );
		}
	}

	OMP_AFFINITY = MPCOMP_AFFINITY_SCATTER;
	/******* OMP_TREE *********/
	env = getenv("OMP_TREE");

	if ( env != NULL && strlen(env) )
	{
		int nb_tokens = 0;
		char **tokens = NULL;
		int i;
		tokens = __mpcomp_tokenizer( env, &nb_tokens );
		assert( tokens != NULL );
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
	if ( getenv( "MPC_DISABLE_BANNER" ) == NULL &&  mpc_common_get_process_rank() == 0)
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
		         __omp_conf.omp_new_task_depth,
		         __omp_conf.omp_task_larceny_mode,
		         __omp_conf.omp_task_nesting_max,
		         __omp_conf.mpcomp_task_max_delayed,
		         __omp_conf.omp_task_use_lockfree_queue,
		         __omp_conf.omp_task_steal_last_stolen_list,
		         __omp_conf.omp_task_resteal_to_last_thief );
#ifdef MPCOMP_USE_TASKDEP
		mpc_common_debug_log("\t\tDependencies ON" );
#else
		mpc_common_debug_log("\t\tDependencies OFF" );
#endif /* MPCOMP_USE_TASKDEP */
#else
		mpc_common_debug_log("\tOpenMP 3 Tasking off");
#endif
		mpc_common_debug_log("\tOMP_SCHEDULE %d", OMP_SCHEDULE );

		if ( __omp_conf.OMP_NUM_THREADS == 0 )
		{
			mpc_common_debug_log("\tDefault #threads (OMP_NUM_THREADS)" );
		}
		else
		{
			mpc_common_debug_log("\tOMP_NUM_THREADS %d", __omp_conf.OMP_NUM_THREADS );
		}

		mpc_common_debug_log("\tOMP_DYNAMIC %d", __omp_conf.OMP_DYNAMIC );
		mpc_common_debug_log("\tOMP_NESTED %d", __omp_conf.OMP_NESTED );

		if ( __omp_conf.OMP_MICROVP_NUMBER == 0 )
		{
			mpc_common_debug_log("\tDefault #microVPs (OMP_MICROVP_NUMBER)" );
		}
		else
		{
			mpc_common_debug_log("\t%d microVPs (OMP_MICROVP_NUMBER)",
			         __omp_conf.OMP_MICROVP_NUMBER );
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

		if ( __omp_conf.OMP_WARN_NESTED )
		{
			mpc_common_debug_log("\tOMP_WARN_NESTED %d (breakpoint mpcomp_warn_nested)",
			         __omp_conf.OMP_WARN_NESTED );
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
		mpcomp_global_icvs.bind_var = __omp_conf.OMP_PROC_BIND;
		mpcomp_global_icvs.stacksize_var = __omp_conf.OMP_STACKSIZE;
		mpcomp_global_icvs.active_wait_policy_var = __omp_conf.OMP_STACKSIZE;
		mpcomp_global_icvs.thread_limit_var = __omp_conf.OMP_THREAD_LIMIT;
		mpcomp_global_icvs.max_active_levels_var = __omp_conf.OMP_MAX_ACTIVE_LEVELS;
		mpcomp_global_icvs.nmicrovps_var = __omp_conf.OMP_MICROVP_NUMBER;
		mpcomp_global_icvs.warn_nested = __omp_conf.OMP_WARN_NESTED;
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
				mpc_common_nodebug( "[%d] %s: SIMPLE_MIXED -> #mvps = %d", task_rank, __func__,
				              nb_mvps );

				/* Consider the env variable if between 1 and the number
				* of cores for this task */
				if ( __omp_conf.OMP_MICROVP_NUMBER > 0 && __omp_conf.OMP_MICROVP_NUMBER <= nb_mvps )
				{
					nb_mvps = __omp_conf.OMP_MICROVP_NUMBER;
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
	if ( __omp_conf.OMP_NUM_THREADS == 0 )
	{
		icvs.nthreads_var = nb_mvps;
	}
	else
	{
		icvs.nthreads_var = __omp_conf.OMP_NUM_THREADS;
	}

	icvs.dyn_var = __omp_conf.OMP_DYNAMIC;
	icvs.nest_var = __omp_conf.OMP_NESTED;
	icvs.run_sched_var = OMP_SCHEDULE;
	icvs.modifier_sched_var = __omp_conf.OMP_MODIFIER_SCHEDULE;
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
	assert( thread );
	assert( thread->instance );
	assert( thread->info.func );
	assert( thread->instance->team );
	loop = &( thread->info.loop_infos.loop.mpcomp_long );

	if ( thread->info.combined_pragma < 0 || thread->info.combined_pragma > 10 )
	{
		mpc_common_nodebug( stderr, "[%d/%ld] Start %s :: %ld\n", thread->mvp->global_rank, thread->rank, __func__, thread->info.combined_pragma );
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
