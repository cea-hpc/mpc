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
#include <string.h>

#include "mpc_thread.h"
#include "mpc_launch.h"
#include "mpcomp_task.h"
#include "mpcomp_sync.h"
#include "mpcomp_core.h"
#include "mpcomp_tree.h"
#include "mpcomp_spinning_core.h"

/* Loop declaration */
#include "mpcomp_loop.h"
#include "mpcomp_tree.h"
#include "mpc_conf.h"
/* parsing OMP_PLACES */
#include "mpcomp_places_env.h"

#include "mpcompt_task.h"
#include "mpcompt_init.h"
#include "mpcompt_thread.h"
#include "mpcompt_frame.h"

/*************************
 * MPC_OMP CONFIGURATION *
 *************************/

static struct mpc_omp_conf_s __omp_conf = { 0 };

struct mpc_omp_conf_s * mpc_omp_conf_get(void)
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
    __omp_conf.maximum_tasks                = INT_MAX;
    __omp_conf.pqueue_new_depth             = 0;
    __omp_conf.pqueue_untied_depth          = 1;
    __omp_conf.task_recycler_capacity       = 8192;
    __omp_conf.fiber_recycler_capacity      = 256;
    __omp_conf.queue_empty_schedules        = 1;
    __omp_conf.task_depth_threshold         = 4;
    __omp_conf.task_use_fiber               = 1;
    __omp_conf.task_trace                   = 0;
    __omp_conf.task_yield_mode              = MPC_OMP_TASK_YIELD_MODE_NOOP;
    __omp_conf.task_priority_policy         = MPC_OMP_TASK_PRIORITY_POLICY_FIFO;

    /* task steal */
    __omp_conf.task_steal_last_stolen   = 0;
    __omp_conf.task_steal_last_thief    = 0;
    __omp_conf.task_larceny_mode        = MPC_OMP_TASK_LARCENY_MODE_PRODUCER;
    snprintf(__omp_conf.task_larceny_mode_str, MPC_CONF_STRING_SIZE, "producer");
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
            NULL
    );

    mpc_conf_config_type_t *task = mpc_conf_config_type_init("task",
            PARAM("maximum",                    &__omp_conf.maximum_tasks,              MPC_CONF_INT,   "Number maximum of tasks that can exists concurrently in the runtime"),
            PARAM("newdepth",                   &__omp_conf.pqueue_new_depth,           MPC_CONF_INT,   "Depth of the new tasks lists in the tree"),
            PARAM("untieddepth",                &__omp_conf.pqueue_untied_depth,        MPC_CONF_INT,   "Depth of the untied tasks lists in the tree"),
            PARAM("taskrecyclercapacity",       &__omp_conf.task_recycler_capacity,     MPC_CONF_INT,   "Task recycler capacity"),
            PARAM("fiberrecyclercapacity",      &__omp_conf.fiber_recycler_capacity,    MPC_CONF_INT,   "Task fiber recycler capacity"),
            PARAM("depththreshold",             &__omp_conf.task_depth_threshold,       MPC_CONF_INT,   "Maximum task depth before it is run undeferedly on parent's fiber"),
            PARAM("fiber",                      &__omp_conf.task_use_fiber,             MPC_CONF_BOOL,  "Enable task fiber"),
            PARAM("trace",                      &__omp_conf.task_trace,                 MPC_CONF_BOOL,  "Enable task tracing"),
            PARAM("prioritypolicy",             &__omp_conf.task_priority_policy,       MPC_CONF_INT,   "Task priority policy"),

            /* TODO : replace yield and priority policy by a string and parse it */
            PARAM("yieldmode",            &__omp_conf.task_yield_mode,          MPC_CONF_INT,       "Task yielding policy"),
            PARAM("larcenymode",          &__omp_conf.task_larceny_mode_str,    MPC_CONF_STRING,    "Task stealing policy"),
            PARAM("steallaststolen",     &__omp_conf.task_steal_last_stolen,    MPC_CONF_BOOL,      "Try to steal to same list than last successful stealing"),
            PARAM("steallastthief",      &__omp_conf.task_steal_last_thief,     MPC_CONF_BOOL,      "Try to steal to the last thread that stole a task to current thread"),
            NULL
    );

    mpc_conf_config_type_t *omp = mpc_conf_config_type_init("omp",
            PARAM("number", &__omp_conf.OMP_MAX_ACTIVE_LEVELS , MPC_CONF_INT, "Maximum depth of nested parallelism"),
            PARAM("nested", &__omp_conf.OMP_WARN_NESTED , MPC_CONF_BOOL, "Emit warning when entering nested parallelism"),
            PARAM("places", &__omp_conf.places , MPC_CONF_STRING, "OpenMP Places configuration OMP_PLACES"),
            PARAM("allocator", &__omp_conf.allocator , MPC_CONF_STRING, "OpenMP allocator"),
            PARAM("schedule", schedule , MPC_CONF_TYPE, "Parameters associated with OpenMP schedules"),
            PARAM("thread", thread , MPC_CONF_TYPE, "Parameters associated with OpenMP threads"),
            PARAM("task", task , MPC_CONF_TYPE, "Parameters associated with OpenMP tasks"),
            NULL
    );

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
static mpc_omp_mode_t OMP_MODE = MPC_OMP_MODE_SIMPLE_MIXED;
/* Affinity policy */
static mpc_omp_affinity_t OMP_AFFINITY = MPC_OMP_AFFINITY_SCATTER;
/* OMP_PLACES */
static mpc_omp_places_info_t *OMP_PLACES_LIST = NULL;
/* Tools */
static mpc_omp_tool_status_t OMP_TOOL = mpc_omp_tool_enabled;
static char* OMP_TOOL_LIBRARIES = NULL;

mpc_omp_global_icv_t mpcomp_global_icvs;

/*****************
  ****** FUNCTIONS
  *****************/

TODO(" function ___tokenizer is inspired from sctk_launch.c. Need to merge, or use 'strtok'")
static char **
___tokenizer( char *string_to_tokenize, int *nb_tokens )
{
	/*    size_t len;*/
	char *cursor;
	char **new_argv;
	int new_argc = 0;
	/* TODO check sizes of every mpcomp_alloc... */
	new_argv = ( char ** ) mpc_omp_alloc( 32 * sizeof( char * ) );
	assert( new_argv != NULL );
	cursor = string_to_tokenize;

	while ( *cursor == ' ' )
	{
		cursor++;
	}

	while ( *cursor != '\0' )
	{
		int word_len = 0;
		new_argv[new_argc] = ( char * ) mpc_omp_alloc( 1024 * sizeof( char ) );

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
___aux_reverse_one_dim_array( int *array, const int len )
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
__convert_topology_to_tree_shape( hwloc_topology_t topology, int *shape_depth )
{
	int *reverse_shape;
	int i, j, reverse_shape_depth;
	const int max_depth = hwloc_topology_get_depth( topology );
	assert( max_depth > 1 );
	reverse_shape = ( int * ) mpc_omp_alloc( sizeof( int ) * max_depth );
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
		___aux_reverse_one_dim_array( reverse_shape, reverse_shape_depth );
	}

	*shape_depth = reverse_shape_depth;
	return reverse_shape;
}

/* MOVE FROM mpcomp_tree_bis.c */
static int
__restrict_topology_for_mpc_omp( hwloc_topology_t *restrictedTopology, const int omp_threads_expected, const int *cpulist )
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
__prepare_omp_task_tree_init( const int num_mvps, const int *cpus_order )
{
	int err;
	hwloc_topology_t restrictedTopology;
	err = __restrict_topology_for_mpc_omp( &restrictedTopology, num_mvps, cpus_order );
	assert( !err );
	assert( restrictedTopology );
	return restrictedTopology;
}

static void
__init_task_tree( const int num_mvps, int *shape, const int *cpus_order )
{
	int i, max_depth, place_depth = 0, place_size;
	int *tree_shape;
	hwloc_topology_t omp_task_topo;
	omp_task_topo = __prepare_omp_task_tree_init( num_mvps, cpus_order );
	tree_shape = __convert_topology_to_tree_shape( omp_task_topo, &max_depth );

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

	_mpc_omp_tree_alloc( tree_shape, max_depth, cpus_order, place_depth, place_size );
}

/*
 * Read environment variables for OpenMP.
 * Actually, the values are read from configuration: those values can be
 * assigned with environement variable or config file.
 * This function is called once per process
 */
static inline void __read_env_variables()
{
	/* Ensure larceny mode for tasks is read from string and then locked */
	__omp_conf.task_larceny_mode = mpc_omp_task_parse_larceny_mode(__omp_conf.task_larceny_mode_str);

	/* We lock as we wont parse future updates */
	mpc_conf_root_config_set_lock("mpcframework.omp.task.larceny", 1);

	char *env;
	mpc_common_nodebug( "_mpcomp_read_env_variables: Read env vars (MPC rank: %d)", mpc_common_get_task_rank() );

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
	OMP_MODE = MPC_OMP_MODE_SIMPLE_MIXED;

	if ( env != NULL )
	{
		int ok = 0;

		/* Handling MPC_OMP_MODE_SIMPLE_MIXED */
		if ( strncmp( env, "simple-mixed", strlen( "simple-mixed" ) ) == 0 ||
		     strncmp( env, "SIMPLE-MIXED", strlen( "SIMPLE-MIXED" ) ) == 0 ||
		     strncmp( env, "simple_mixed", strlen( "simple_mixed" ) ) == 0 ||
		     strncmp( env, "SIMPLE_MIXED", strlen( "SIMPLE_MIXED" ) ) == 0 )
		{
			OMP_MODE = MPC_OMP_MODE_SIMPLE_MIXED;
			ok = 1;
		}

		/* Handling MPC_OMP_MODE_OVERSUBSCRIBED_MIXED */
		if ( strncmp( env, "alternating", strlen( "alternating" ) ) == 0 ||
		     strncmp( env, "ALTERNATING", strlen( "ALTERNATING" ) ) == 0 )
		{
			OMP_MODE = MPC_OMP_MODE_ALTERNATING;
			ok = 1;
		}

		/* Handling MPC_OMP_MODE_ALTERNATING */
		if ( strncmp( env, "oversubscribed-mixed", strlen( "oversubscribed-mixed" ) ) ==
		     0 ||
		     strncmp( env, "OVERSUBSCRIBED-MIXED", strlen( "OVERSUBSCRIBED-MIXED" ) ) ==
		     0 ||
		     strncmp( env, "oversubscribed_mixed", strlen( "oversubscribed_mixed" ) ) ==
		     0 ||
		     strncmp( env, "OVERSUBSCRIBED_MIXED", strlen( "OVERSUBSCRIBED_MIXED" ) ) ==
		     0 )
		{
			OMP_MODE = MPC_OMP_MODE_OVERSUBSCRIBED_MIXED;
			ok = 1;
		}

		/* Handling MPC_OMP_MODE_FULLY_MIXED */
		if ( strncmp( env, "fully-mixed", strlen( "fully-mixed" ) ) == 0 ||
		     strncmp( env, "FULLY-MIXED", strlen( "FULLY-MIXED" ) ) == 0 ||
		     strncmp( env, "fully_mixed", strlen( "fully_mixed" ) ) == 0 ||
		     strncmp( env, "FULLY_MIXED", strlen( "FULLY_MIXED" ) ) == 0 )
		{
			OMP_MODE = MPC_OMP_MODE_FULLY_MIXED;
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

		/* Handling MPC_OMP_AFFINITY_COMPACT */
		if ( strncmp( env, "compact", strlen( "compact" ) ) == 0 ||
		     strncmp( env, "COMPACT", strlen( "COMPACT" ) ) == 0 )
		{
			OMP_AFFINITY = MPC_OMP_AFFINITY_COMPACT;
			ok = 1;
		}

		/* Handling MPC_OMP_AFFINITY_SCATTER */
		if ( strncmp( env, "scatter", strlen( "scatter" ) ) == 0 ||
		     strncmp( env, "SCATTER", strlen( "SCATTER" ) ) == 0 )
		{
			OMP_AFFINITY = MPC_OMP_AFFINITY_SCATTER;
			ok = 1;
		}

		/* Handling MPC_OMP_AFFINITY_BALANCED */
		if ( strncmp( env, "balanced", strlen( "balanced" ) ) == 0 ||
		     strncmp( env, "BALANCED", strlen( "BALANCED" ) ) == 0 )
		{
			OMP_AFFINITY = MPC_OMP_AFFINITY_BALANCED;
			ok = 1;
		}

		if ( ok )
		{
		}
		else
		{
			/*mpc_common_debug_warning("Unknown affinity <%s> (must be COMPACT, "
			         "SCATTER or BALANCED),"
			         " fallback to default affinity <%d>\n",
			         env, OMP_AFFINITY );*/
		}
	}

	OMP_AFFINITY = MPC_OMP_AFFINITY_SCATTER;
	/******* OMP_TREE *********/
	env = getenv("OMP_TREE");

	if ( env != NULL && strlen(env) )
	{
		int nb_tokens = 0;
		char **tokens = NULL;
		int i;
		tokens = ___tokenizer( env, &nb_tokens );
		assert( tokens != NULL );
		/* TODO check that arguments are ok and #leaves is correct */
		OMP_TREE = ( int * ) mpc_omp_alloc( nb_tokens * sizeof( int ) );
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

    /******* OMP_TOOL *********/
    env = __omp_conf.omp_tool;

    if ( strlen( env ) != 0 )
    {
        if ( strncmp( env, "disabled", strlen("disabled") ) == 0 ||
             strncmp( env, "Disabled", strlen("disabled") ) == 0 ||
             strncmp( env, "DISABLED", strlen("disabled") ) == 0 )
        {
            OMP_TOOL = mpc_omp_tool_disabled;
        }
    }

//fprintf(stderr, "OMPT TOOL ENV = %s / OMP_TOOL = %s\n", env, OMP_TOOL );
//fflush(stderr);

    /******* OMP_TOOL_LIBRARIES *********/
    env = __omp_conf.omp_tool_libraries;

    if ( strlen( env ) != 0 )
    {
        OMP_TOOL_LIBRARIES = strdup( env );
    }

	/***** PRINT SUMMARY (only first MPI rank) ******/
	if ( getenv( "MPC_DISABLE_BANNER" ) == NULL &&  mpc_common_get_process_rank() == 0)
	{
        mpc_common_debug_log("--------------------------------------------------");
        mpc_common_debug_log("MPC OpenMP version %d.%d (Intel and Patched GCC compatibility)", MPC_OMP_VERSION_MAJOR, MPC_OMP_VERSION_MINOR );
        mpc_common_debug_log("\tOpenMP 3 Tasking on:\n");
        mpc_common_debug_log("\t\tmaximum tasks=%d",        __omp_conf.maximum_tasks);
        mpc_common_debug_log("\t\tnew tasks depth=%d",      __omp_conf.pqueue_new_depth);
        mpc_common_debug_log("\t\tuntied tasks depth=%d",   __omp_conf.pqueue_untied_depth);
        mpc_common_debug_log("\t\tlaceny mode=%d",          __omp_conf.task_larceny_mode);
        mpc_common_debug_log("\t\tdepth threshold=%d",      __omp_conf.task_depth_threshold);
        mpc_common_debug_log("\t\tsteal last stolen=%d",    __omp_conf.task_steal_last_stolen);
        mpc_common_debug_log("\t\tsteal last thief=%d",     __omp_conf.task_steal_last_thief);
        mpc_common_debug_log("\t\tyield mode=%d",           __omp_conf.task_yield_mode);
        mpc_common_debug_log("\t\tpriority policy=%d",      __omp_conf.task_priority_policy);
        mpc_common_debug_log("\t\ttrace=%d",                __omp_conf.task_trace);
        mpc_common_debug_log("\n");

        mpc_common_debug_log("\tTask context");
# if MPCOMP_TASK_COMPILE_CONTEXT
        mpc_common_debug_log("\t\tCompiled=yes");
        mpc_common_debug_log("\t\tEnabled=%s", MPCOMP_TASK_CONTEXT_ENABLED ? "yes" : "no");
# else
        mpc_common_debug_log("\t\tCompiled=no");
# endif 
        mpc_common_debug_log("\n");
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
			case MPC_OMP_AFFINITY_COMPACT:
				mpc_common_debug_log("\tAffinity COMPACT (fill logical cores first)" );
				break;

			case MPC_OMP_AFFINITY_SCATTER:
				mpc_common_debug_log("\tAffinity SCATTER (spread over NUMA nodes)" );
				break;

			case MPC_OMP_AFFINITY_BALANCED:
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
			case MPC_OMP_MODE_SIMPLE_MIXED:
				mpc_common_debug_log("\tMode for hybrid MPI+OpenMP parallelism SIMPLE_MIXED" );
				break;

			case MPC_OMP_MODE_OVERSUBSCRIBED_MIXED:
				mpc_common_debug_log("\tMode for hybrid MPI+OpenMP parallelism OVERSUBSCRIBED_MIXED" );
				break;

			case MPC_OMP_MODE_ALTERNATING:
				mpc_common_debug_log("\tMode for hybrid MPI+OpenMP parallelism ALTERNATING" );
				break;

			case MPC_OMP_MODE_FULLY_MIXED:
				mpc_common_debug_log("\tMode for hybrid MPI+OpenMP parallelism FULLY_MIXED" );
				break;

			default:
				not_reachable();
				break;
		}

#if MPC_OMP_MALLOC_ON_NODE
		mpc_common_debug_log("\tNUMA allocation for tree nodes" );
#endif
#if MPC_OMP_COHERENCY_CHECKING
		mpc_common_debug_log("\tCoherency check enabled" );
#endif
#if MPC_OMP_ALIGN
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

#if MPC_OMP_MIC
		mpc_common_debug_log("\tMIC optimizations on\n" );
#endif
#if OMPT_SUPPORT
		mpc_common_debug_log("\tTool support %s", OMP_TOOL ? "enabled" : "disabled" );

        if ( OMP_TOOL_LIBRARIES )
        {
		    mpc_common_debug_log("\tTool paths: %s", OMP_TOOL_LIBRARIES );
        }
#else
		mpc_common_debug_log("\tTool Support disabled" );
#endif
		TODO( "Add every env vars when printing info on OpenMP" )
                mpc_common_debug_log("--------------------------------------------------");
	}
}

/* Initialization of the OpenMP runtime
   Called during the initialization of MPC
 */
void mpc_omp_init( void )
{
	static volatile int done = 0;
	static mpc_common_spinlock_t lock = SCTK_SPINLOCK_INITIALIZER;
	int nb_mvps;
	int task_rank;

	/* Do we need to initialize the current team */
#if OMPT_SUPPORT
    if ( mpc_omp_tls
         && ( (mpc_omp_thread_t*) mpc_omp_tls )->tool_status != uninitialized )
#else
	if ( mpc_omp_tls )
#endif /* OMPT_SUPPORT */
    {
		return;
    }

	mpc_omp_local_icv_t icvs;

	/* Need to initialize the whole runtime (environment variables) This
	* section is shared by every OpenMP instances among MPI tasks located inside
	* the same OS process */
	mpc_common_spinlock_lock( &lock );

	if ( done == 0 )
	{
		__read_env_variables();
		mpcomp_global_icvs.def_sched_var = omp_sched_static;
		mpcomp_global_icvs.bind_var = __omp_conf.OMP_PROC_BIND;
		mpcomp_global_icvs.stacksize_var = __omp_conf.OMP_STACKSIZE;
		mpcomp_global_icvs.active_wait_policy_var = __omp_conf.OMP_STACKSIZE;
		mpcomp_global_icvs.thread_limit_var = __omp_conf.OMP_THREAD_LIMIT;
		mpcomp_global_icvs.max_active_levels_var = __omp_conf.OMP_MAX_ACTIVE_LEVELS;
		mpcomp_global_icvs.nmicrovps_var = __omp_conf.OMP_MICROVP_NUMBER;
		mpcomp_global_icvs.warn_nested = __omp_conf.OMP_WARN_NESTED;
		mpcomp_global_icvs.affinity = OMP_AFFINITY;
        mpcomp_global_icvs.tool = OMP_TOOL;
        mpcomp_global_icvs.tool_libraries = OMP_TOOL_LIBRARIES;
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
			case MPC_OMP_MODE_SIMPLE_MIXED:
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

			case MPC_OMP_MODE_ALTERNATING:
				nb_mvps = 1;

				if ( mpc_common_get_local_task_rank() == 0 )
                                {
					nb_mvps = mpc_topology_get_pu_count();
				}

				break;

			case MPC_OMP_MODE_OVERSUBSCRIBED_MIXED:
				not_implemented();
				break;

			case MPC_OMP_MODE_FULLY_MIXED:
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

    mpc_omp_init_seq_region();

#if OMPT_SUPPORT
	_mpc_omp_ompt_init();
#endif /* OMPT_SUPPORT */

    mpc_omp_init_initial_thread( icvs );

	int places_nb_mvps;
	int *shape, *cpus_order;
	OMP_PLACES_LIST = _mpc_omp_places_env_variable_parsing( nb_mvps );

	if ( OMP_PLACES_LIST )
	{
		places_nb_mvps = _mpc_omp_places_get_topo_info( OMP_PLACES_LIST, &shape, &cpus_order );
		assert( places_nb_mvps <= nb_mvps );
    nb_mvps = places_nb_mvps;
	}
	else
	{
		shape = NULL;
		cpus_order = NULL;
	}

	__init_task_tree( nb_mvps, shape, cpus_order );

	mpc_common_spinlock_unlock( &lock );

#if OMPT_SUPPORT
    _mpc_omp_ompt_callback_implicit_task( ompt_scope_begin, 1, 1, ompt_task_initial );
#endif /* OMPT_SUPPORT */
}

void mpc_omp_exit( void )
{
    mpc_omp_thread_t *ithread;
    mpc_omp_node_t* root_node;

    if( mpc_omp_tls ) {
        ithread = mpc_omp_tls;
        assert( ithread->rank == 0 );
        root_node = ithread->root;
        assert( root_node );

#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_implicit_task( ompt_scope_end, 0, 1, ompt_task_initial );
#endif /* OMPT_SUPPORT */

        _mpc_omp_exit_node_signal( root_node );

#if OMPT_SUPPORT
        _mpc_omp_ompt_callback_thread_end( ithread, ompt_thread_initial );
        _mpc_omp_ompt_finalize();
#endif /* OMPT_SUPPORT */

        if( mpcomp_global_icvs.tool_libraries ) {
            sctk_free( mpcomp_global_icvs.tool_libraries );
            mpcomp_global_icvs.tool_libraries = NULL;
        }
    }
}

void _mpc_omp_in_order_scheduler( mpc_omp_thread_t *thread )
{
	mpc_omp_loop_long_iter_t *loop;
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
		case MPC_OMP_COMBINED_NONE:
			break;

		case MPC_OMP_COMBINED_SECTION:
			_mpc_omp_sections_init( thread, thread->info.nb_sections );
			break;

		case MPC_OMP_COMBINED_STATIC_LOOP:
			_mpc_omp_static_loop_init( thread, loop->lb, loop->b, loop->incr, loop->chunk_size );
			break;

		case MPC_OMP_COMBINED_DYN_LOOP:
			_mpc_omp_dynamic_loop_init( thread, loop->lb, loop->b, loop->incr, loop->chunk_size );
			break;

		case MPC_OMP_COMBINED_GUIDED_LOOP:
			mpc_omp_guided_loop_begin( loop->lb, loop->b, loop->incr, loop->chunk_size, NULL, NULL );
			break;

		default:
			not_implemented();
	}

#if OMPT_SUPPORT && MPC_OMPT_HAS_FRAME_SUPPORT
    /* Record and reset current frame infos */
    mpc_omp_ompt_frame_info_t prev_frame_infos = _mpc_omp_ompt_frame_reset_infos();

    _mpc_omp_ompt_frame_set_exit( MPC_OMPT_GET_FRAME_ADDRESS );
#endif /* OMPT_SUPPORT */

	thread->info.func( thread->info.shared );

#if OMPT_SUPPORT && MPC_OMPT_HAS_FRAME_SUPPORT
    /* Restore previous frame infos */
    _mpc_omp_ompt_frame_set_infos( &prev_frame_infos );
#endif /* OMPT_SUPPORT */
}

# if MPC_MPI
#  include "mpc_mpi.h"
# endif

double
mpc_omp_timestamp(void)
{
# if MPC_MPI
    return MPI_Wtime() * 1000000;
# else /* MPC_MPI */
    double res;
#  if SCTK_WTIME_USE_GETTIMEOFDAY
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (double) tp.tv_usec + (double) tp.tv_sec * 1000000.0;
#  else /* SCTK_WTIME_USE_GETTIMEOFDAY */
    return mpc_arch_get_timestamp() * 1000000.0;
#  endif /* SCTK_WTIME_USE_GETTIMEOFDAY */
# endif /* MPC_MPI */
}
