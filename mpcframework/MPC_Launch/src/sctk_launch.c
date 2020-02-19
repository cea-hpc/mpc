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
/* #   - BOUHROUR Stephane stephane.bouhrour@exascale-computing.eu        # */
/* #                                                                      # */
/* ######################################################################## */
#include <mpc_config.h>


#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <mpc_runtime_config.h>

#include <mpc_common_flags.h>
#include <mpc_common_helper.h>


#if !defined(NO_INTERNAL_ASSERT)
	#define SCTK_DEBUG_MODE " Debug MODE"
#else
	#define SCTK_DEBUG_MODE ""
#endif

#include "sctk_thread.h"
#include "sctk_launch.h"
#include "sctk_debug.h"
#include "mpc_common_asm.h"


#include "mpc_topology.h"
#include <mpc_launch_pmi.h>

/* #include "sctk_daemons.h" */
/* #include "sctk_io.h" */
#include "mpc_runtime_config.h"
#include "mpc_common_types.h"

#ifdef MPC_USE_EXTLS
	#include <extls_hls.h>
#endif

#ifdef MPC_Profiler
	#include "sctk_profile_render.h"
#endif

#ifdef MPC_Accelerators
	#include "sctk_accelerators.h"
#endif

#ifdef MPC_Fault_Tolerance
	#include "sctk_ft_iface.h"
#endif

#ifdef MPC_Active_Message
	#include "arpc_common.h"
#endif

#define SCTK_START_KEYWORD "--sctk-args--"

#ifdef HAVE_ENVIRON_VAR
	extern char **environ;
#endif

#ifdef MPC_AIO_ENABLED
	#include "sctk_aio.h"
#endif

#define SCTK_LAUNCH_MAX_ARG 4096
static char *sctk_save_argument[SCTK_LAUNCH_MAX_ARG];
static int sctk_initial_argc = 0;
static int sctk_start_argc = 0;
static char **init_argument = NULL;
bool sctk_checkpoint_mode;

bool sctk_accl_support;
#define MAX_TERM_LENGTH 80
#define MAX_NAME_FORMAT 30
/* const char *sctk_store_dir = "/dev/null"; */

char *sctk_network_mode = "none";
char *sctk_checkpoint_str = "";

bool sctk_share_node_capabilities;

char *sctk_profiling_outputs;

char *get_debug_mode()
{
	return SCTK_DEBUG_MODE;
}
void format_output( char *name, char *def )
{
	char n[MAX_NAME_FORMAT];
	char *tmp;
	char *tmp_tab;
	char *old_tmp;
	int i;

	for ( i = 0; i < MAX_NAME_FORMAT; i++ )
	{
		n[i] = ' ';
	}

	n[MAX_NAME_FORMAT - 1] = '\0';
	sprintf ( n, "%s", name );
	n[strlen ( name )] = ' ';
	fprintf ( stderr, "  %s", n );

	for ( i = 0; i < MAX_NAME_FORMAT; i++ )
	{
		n[i] = ' ';
	}

	n[MAX_NAME_FORMAT - 1] = '\0';
	tmp_tab = ( char * ) sctk_malloc ( ( 1 + strlen ( def ) ) * sizeof ( char ) );
	memcpy ( tmp_tab, def, ( 1 + strlen ( def ) ) * sizeof ( char ) );
	tmp = tmp_tab;

	while ( strlen ( tmp ) > ( MAX_TERM_LENGTH - MAX_NAME_FORMAT ) )
	{
		old_tmp = tmp;
		tmp = &( tmp[MAX_TERM_LENGTH - MAX_NAME_FORMAT] );

		while ( ( tmp > old_tmp ) && ( tmp[0] != ' ' ) )
		{
			tmp--;
		}

		assume ( tmp != old_tmp );
		tmp[0] = '\0';
		fprintf ( stderr, "%s\n  %s", old_tmp, n );
		tmp++;
	}

	fprintf ( stderr, "%s", tmp );
	fprintf ( stderr, "\n" );
}

static inline int
__sctk_add_arg ( char *arg, void ( *action ) ( void ),
                 char *word )
{
	if ( strncmp ( word, arg, strlen ( arg ) ) == 0 )
	{
		action ();
		return 0;
	}

	return 1;
}

#define sctk_add_arg(arg,action) if(__sctk_add_arg(arg,action,word) == 0) return 0

static inline int
__sctk_add_arg_eq ( char *arg,
                    void ( *action ) ( char * ), char *word )
{
	if ( strncmp ( word, arg, strlen ( arg ) ) == 0 )
	{
		action ( &( word[1 + strlen ( arg )] ) );
		return 0;
	}

	return 1;
}

#define sctk_add_arg_eq(arg,action) if(__sctk_add_arg_eq(arg,action,word) == 0) return 0

static int sctk_process_nb_val;
static int sctk_processor_nb_val;


static char *sctk_launcher_mode;


/*   void */
/* sctk_set_net_val (void (*val) (int *, char ***)) */
/* { */
/*   sctk_net_val = val; */
/* } */

void sctk_use_pthread( void )
{
	mpc_common_get_flags()->thread_library_kind = "pthread";
	mpc_common_get_flags()->thread_library_init = sctk_pthread_thread_init;
}

#if 0
/* Note that we start with an agressive frequency
 * to speedup the polling during the init phase
 * we relax it after doing driver initialization */
int __polling_done = 0;

void *polling_thread( __UNUSED__ void *dummy )
{
	/* The role of this thread is to poll
	 * idle in a gentle manner in order
	 * to avoid starvation particularly
	 * during init phases (where tasks
	 * are not waiting but for example
	 * might be in a PMI barrier.
	 *
	 * Note that as polling is hierarchical
	 * the contention is limited */
	mpc_topology_bind_to_cpu( -1 );

	while ( 1 )
	{
                #ifdef MPC_Message_Passing
		sctk_network_notify_idle_message();
                #endif

		if ( __polling_done )
		{
			break;
		}
	}

	return NULL;
}
#endif

void mpc_launch_print_banner( bool restart )
{
	if ( mpc_common_get_process_rank() == 0 )
	{
		char *mpc_lang = "C/C++";

		if ( sctk_is_in_fortran == 1 )
		{
			mpc_lang = "Fortran";
		}

		if ( sctk_runtime_config_get()->modules.launcher.banner )
		{
			if ( sctk_checkpoint_mode && restart )
			{
				mpc_common_debug_log("+++ Application restarting from checkpoint with the following configuration:");
			}

                        char version_string[64];

                        if(MPC_VERSION_MINOR >= 0)
                        {
                                snprintf(version_string, 64, "version %d.%d.%d%s",
                                                             MPC_VERSION_MAJOR,
                                                             MPC_VERSION_MINOR,
                                                             MPC_VERSION_REVISION,
				                             MPC_VERSION_PRE);
                        }
                        else
                        {
                                snprintf(version_string, 64, "experimental version");
                        }

                        mpc_common_debug_log("--------------------------------------------------------");
                        mpc_common_debug_log("MPC %s in %s", version_string, mpc_lang);
                        mpc_common_debug_log("%d tasks %d processes %d cpus @ %2.2fGHz",
                                              mpc_common_get_flags()->task_number,
                                             sctk_process_nb_val,
                                             mpc_topology_get_pu_count (),
                                             sctk_atomics_get_cpu_freq() / 1000000000.0);
                        mpc_common_debug_log("%s Thread Engine", mpc_common_get_flags()->thread_library_kind);
                        mpc_common_debug_log("%s %s %s", sctk_alloc_mode (), SCTK_DEBUG_MODE, sctk_checkpoint_str);
                        mpc_common_debug_log("%s", sctk_network_mode);
                        mpc_common_debug_log("--------------------------------------------------------");



		}
	}
}

void mpc_lowcomm_rdma_window_init_ht();
void mpc_lowcomm_rdma_window_release_ht();

static void sctk_perform_initialisation ( void )
{

	/*   mkdir (sctk_store_dir, 0777); */
	if ( sctk_process_nb_val && mpc_common_get_flags()->task_number )
	{
		if ( mpc_common_get_flags()->task_number < sctk_process_nb_val )
		{
			sctk_error( "\n--process-nb=%d\n--nb-task=%d\n Nb tasks must be greater than nb processes\n Run your program with option --nb-task=(>=%d) or --nb-process=(<=%d)", sctk_process_nb_val, mpc_common_get_flags()->task_number, sctk_process_nb_val, mpc_common_get_flags()->task_number );
			abort();
		}
	}

	sctk_only_once ();

	if ( sctk_process_nb_val > 1 )
	{
		mpc_common_set_process_count( sctk_process_nb_val );
		sctk_nodebug ( "%d processes", mpc_common_get_process_count() );
	}

	/* As a first step initialize the PMI */

        mpc_launch_pmi_init();
	mpc_topology_init ();
#if defined (MPC_USE_EXTLS) && !defined(MPC_DISABLE_HLS)
	extls_hls_topology_construct();

#endif
#ifdef MPC_Accelerators
	sctk_accl_init();
#endif
#ifdef MPC_Fault_Tolerance
	sctk_ft_init();
#endif
#ifdef MPC_Active_Message
	arpc_init();
#endif

	/* Do not bind in LIB_MODE */
	TODO( "UNDERSTAND WHY IT IS FAILING" );
#if 0
#ifndef SCTK_LIB_MODE
	/* Bind the main thread to the first VP */
	const unsigned int core = 0;
	int binding = 0;
	binding = sctk_get_init_vp ( core );
	mpc_topology_bind_to_cpu ( binding );
	sctk_nodebug( "Init: thread bound to thread %d", binding );
#endif
#endif

#ifdef HAVE_HWLOC
	sctk_alloc_posix_mmsrc_numa_init_phase_numa();
#endif
	// MALP FIX...
	char *env = NULL;

	if ( ( env = getenv( "LD_PRELOAD" ) ) )
	{
		setenv( "LD_PRELOAD", "", 1 );
	}

	sctk_locate_dynamic_initializers();

	if ( env )
	{
		setenv( "LD_PRELOAD", env, 1 );
	}

	sctk_thread_init ();


#ifdef SCTK_LIB_MODE
	/* In lib mode we force the pthread MODE */
	sctk_use_pthread();
#endif

	if ( mpc_common_get_flags()->thread_library_init != NULL )
	{
		mpc_common_get_flags()->thread_library_init ();
	}
	else
	{
		fprintf ( stderr, "No multithreading mode specified!\n" );
		abort ();
	}

	sctk_init_alloc ();

	if ( sctk_processor_nb_val > 1 )
	{
		if ( sctk_process_nb_val > 1 )
		{
			int cpu_detected;
			cpu_detected = mpc_topology_get_pu_count ();

			if ( cpu_detected < sctk_processor_nb_val )
			{
				fprintf ( stderr,
				          "Processor number doesn't match number detected %d <> %d!\n",
				          cpu_detected, sctk_processor_nb_val );
			}
		}
	}

	if ( !mpc_common_get_flags()->task_number )
	{
		fprintf ( stderr, "No task number specified!\n" );
		sctk_abort ();
	}

#ifdef MPC_Message_Passing

#endif
#ifdef MPC_Profiler
	sctk_internal_profiler_init();
#endif

#if 0
	/* Start auxiliary polling thread */
	/*
	   pthread_t progress;
	   pthread_create (&progress, NULL, polling_thread, NULL);
	   */
	sctk_thread_t progress;
	sctk_thread_attr_t attr;
	sctk_thread_attr_init ( &attr );
	sctk_thread_attr_setscope ( &attr, SCTK_THREAD_SCOPE_SYSTEM );
	sctk_user_thread_create( &progress, &attr, polling_thread, NULL );
#endif

#ifdef SCTK_LIB_MODE
	sctk_net_init_task_level ( my_rank, 0 );
#endif

#if 0
	/* We passed the init phase we can stop the bootstrap polling */
	__polling_done = 1;
#endif
}

//////////////////////////////////
// OLD SCHEDULER
//
static void sctk_use_ethread ( void )
{
	mpc_common_get_flags()->thread_library_kind = "ethread";
	mpc_common_get_flags()->thread_library_init = sctk_ethread_thread_init;
}

void
sctk_use_ethread_mxn ( void )
{
	mpc_common_get_flags()->thread_library_kind = "ethread_mxn";
	mpc_common_get_flags()->thread_library_init = sctk_ethread_mxn_thread_init;
}
//////////////////////////////////
// END OLD SCHEDULER
//

//////////////////////////////////
// SCHEDULER NG
//
/********* ETHREAD MXN ************/
void sctk_use_ethread_mxn_ng( void )
{
	mpc_common_get_flags()->thread_library_kind = "ethread_mxn_ng";
	mpc_common_get_flags()->thread_library_init = sctk_ethread_mxn_ng_thread_init;
}

/********* ETHREAD ************/
void sctk_use_ethread_ng( void )
{
	mpc_common_get_flags()->thread_library_kind = "ethread_ng";
	mpc_common_get_flags()->thread_library_init = sctk_ethread_ng_thread_init;
}

/********* PTHREAD ************/
void sctk_use_pthread_ng( void )
{
	mpc_common_get_flags()->thread_library_kind = "pthread_ng";
	mpc_common_get_flags()->thread_library_init = sctk_pthread_ng_thread_init;
}
/*********  END NG ************/

static void sctk_def_directory( __UNUSED__ char *arg ) /*   sctk_store_dir = arg; */
{
}

static void ignored_argument ( char *arg )
{

}

static void sctk_def_task_nb( char *arg )
{
	int task_nb = atoi( arg );
	mpc_common_get_flags()->task_number = task_nb;
}

static void
sctk_def_process_nb ( char *arg )
{
	sctk_process_nb_val = atoi ( arg );
}


static void
sctk_def_processor_nb ( char *arg )
{
	mpc_common_get_flags()->processor_number = atoi ( arg );
}

static void
sctk_def_launcher_mode( char *arg )
{
	sctk_launcher_mode = strdup( arg );
}
char *
sctk_get_launcher_mode()
{
	return sctk_launcher_mode;
}


static void
sctk_def_enable_smt ( __UNUSED__ char *arg )
{
	mpc_common_get_flags()->enable_smt_capabilities = 1;
}

static void
sctk_def_disable_smt ( __UNUSED__ char *arg )
{
	mpc_common_get_flags()->enable_smt_capabilities = 0;
}

static void
sctk_def_text_placement( __UNUSED__ char *arg )
{
	mpc_common_get_flags()->enable_topology_text_placement = 1;
}

static void
sctk_def_graphic_placement( __UNUSED__ char *arg )
{
	mpc_common_get_flags()->enable_topology_graphic_placement = 1;
}

static void
sctk_def_share_node ( __UNUSED__ char *arg )
{
	sctk_share_node_capabilities = 1;
}

static void
sctk_checkpoint ( void )
{
	sctk_checkpoint_mode = 1;
}


static void sctk_def_accl_support( void )
{
	sctk_accl_support = 1;
}

static void sctk_set_verbosity( char *arg )
{
	int tmp = atoi( arg );

	if ( ( 0 <= tmp ) && ( mpc_common_get_flags()->verbosity < tmp ) )
	{
		mpc_common_get_flags()->verbosity = tmp;
	}
}


static void
sctk_use_network ( char *arg )
{
	mpc_common_get_flags()->network_driver_name = arg;
}


static void sctk_set_profiling( __UNUSED__ char *arg )
{
	/* fprintf(stderr,"BEFORE |%s| |%s|\n",sctk_profiling_outputs,arg); */
#ifdef MPC_Profiler
	if ( strcmp( arg, "undef" ) != 0 )
	{
		char *val;
		val = calloc( strlen( arg ) + 10, sizeof( char ) );
		memcpy( val, arg, strlen( arg ) );

		if ( sctk_profile_renderer_check_render_list( val ) )
		{
			sctk_error( "Provided profiling output syntax is not correct: %s", val );
			abort();
		}

		/* fprintf(stderr,"SET %s %s %d\n",sctk_profiling_outputs,arg,strcmp(arg,"undef")); */
		sctk_profiling_outputs = val;
	}

#endif
	/* fprintf(stderr,"AFTER %s %s\n",sctk_profiling_outputs,arg); */
}

static void
sctk_def_port_number ( char *arg )
{
	arg[0] = '\0';
}

static void
sctk_def_use_host ( char *arg )
{
	arg[0] = '\0';
}

static inline int
sctk_proceed_arg ( char *word )
{
	sctk_add_arg_eq ( "--enable-smt", sctk_def_enable_smt );
	sctk_add_arg_eq ( "--graphic-placement", sctk_def_graphic_placement );
	sctk_add_arg_eq ( "--text-placement", sctk_def_text_placement );
	sctk_add_arg_eq ( "--disable-smt", sctk_def_disable_smt );
	sctk_add_arg_eq ( "--directory", sctk_def_directory );
	sctk_add_arg_eq ( "--mpc-verbose", sctk_set_verbosity );
	sctk_add_arg ( "--use-pthread_ng", sctk_use_pthread_ng );
	sctk_add_arg ( "--use-pthread", sctk_use_pthread );
	sctk_add_arg ( "--use-ethread_mxn_ng", sctk_use_ethread_mxn_ng );
	sctk_add_arg ( "--use-ethread_mxn", sctk_use_ethread_mxn );
	sctk_add_arg ( "--use-ethread_ng", sctk_use_ethread_ng );
	sctk_add_arg ( "--use-ethread", sctk_use_ethread );
	sctk_add_arg_eq ( "--sctk_use_network", sctk_use_network );
	/*FOR COMPATIBILITY */
	sctk_add_arg_eq ( "--sctk_use_port_number", sctk_def_port_number );
	sctk_add_arg_eq ( "--sctk_use_host", sctk_def_use_host );
	sctk_add_arg_eq ( "--task-number", sctk_def_task_nb );
	sctk_add_arg_eq ( "--process-number", sctk_def_process_nb );
        /* Node NB is always coming from PMI */
	sctk_add_arg_eq ("--node-number", ignored_argument);
	sctk_add_arg_eq ( "--share-node", sctk_def_share_node );
	sctk_add_arg_eq ( "--processor-number", sctk_def_processor_nb );
	sctk_add_arg_eq ( "--profiling", sctk_set_profiling );
	sctk_add_arg_eq ( "--launcher", sctk_def_launcher_mode );
	sctk_add_arg ( "--checkpoint", sctk_checkpoint );
	sctk_add_arg( "--use-accl", sctk_def_accl_support );

	if ( strcmp( word, "--sctk-args-end--" ) == 0 )
	{
		return -1;
	}

	fprintf( stderr, "Argument %s Unknown\n", word );
	return 1;
}

void
sctk_launch_contribution ( FILE *file )
{
	int i;
	fprintf ( file, "ARGC %d\n", sctk_initial_argc );

	for ( i = 0; i < sctk_initial_argc; i++ )
	{
		fprintf ( file, "%ld %s\n", ( long ) strlen ( sctk_save_argument[i] ) + 1,
		          sctk_save_argument[i] );
	}
}

static int sctk_env_init_intern( int *argc, char ***argv )
{
	int i, j;
	char **new_argv;
	sctk_init();
	sctk_initial_argc = *argc;
	init_argument = *argv;

	for ( i = 0; i < sctk_initial_argc; i++ )
	{
		sctk_start_argc = i;

		if ( strcmp( ( *argv )[i], SCTK_START_KEYWORD ) == 0 )
		{
			sctk_start_argc = i - 1;
			break;
		}
	}

	new_argv = ( char ** )sctk_malloc( sctk_initial_argc * sizeof( char * ) );
	sctk_nodebug( "LAUNCH allocate %p of size %lu", new_argv,
	              ( unsigned long )( sctk_initial_argc * sizeof( char * ) ) );

	for ( i = 0; i <= sctk_start_argc; i++ )
	{
		new_argv[i] = ( *argv )[i];
	}

	for ( i = sctk_start_argc + 1; i < sctk_initial_argc; i++ )
	{
		if ( strcmp( ( *argv )[i], SCTK_START_KEYWORD ) == 0 )
		{
			*argc = ( *argc ) - 1;
		}
		else
		{
			if ( strcmp( ( *argv )[i], "--sctk-args-end--" ) == 0 )
			{
				*argc = ( *argc ) - 1;
			}

			if ( sctk_proceed_arg( ( *argv )[i] ) == -1 )
			{
				break;
			}

			( *argv )[i] = "";
			*argc = ( *argc ) - 1;
		}
	}

	if ( mpc_common_get_flags()->enable_smt_capabilities == 1 )
	{
		sctk_processor_nb_val *= mpc_topology_get_ht_per_core();
	}

	i++;

	for ( j = sctk_start_argc + 1; i < sctk_initial_argc; i++ )
	{
		new_argv[j] = ( *argv )[i];
		sctk_nodebug( "Copy %d %s", j, ( *argv )[i] );
		j++;
	}

	*argv = new_argv;
	sctk_nodebug( "new argc tmp %d", *argc );
	return 0;
}

int
sctk_env_init ( int *argc, char ***argv )
{

	sctk_env_init_intern ( argc, argv );

	sctk_perform_initialisation ();

	return 0;
}

int
sctk_env_exit ()
{
	mpc_topology_destroy();
#ifdef MPC_AIO_ENABLED
	mpc_common_aio_threads_release();
#endif
	sctk_leave ();
	return 0;
}

static inline char *
sctk_skip_token ( char *p )
{
	while ( isspace ( *p ) )
	{
		p++;
	}

	while ( *p && !isspace ( *p ) )
	{
		p++;
	}

	while ( isspace ( *p ) )
	{
		p++;
	}

	return p;
}

/*********************
 * VP START FUNCTION *
 *********************/

typedef struct
{
	int argc;
	char **argv;
        char **saved_argv;
} startup_arg_t;


static inline startup_arg_t * __startup_arg_extract_and_duplicate(startup_arg_t * input_args)
{
        startup_arg_t * ret = sctk_malloc(sizeof(startup_arg_t ));
        assume(ret);

        ret->argc = input_args->argc;

	/* We create this extra argv array
	 * to prevent the case where a (strange)
	 * program modifies the argv[i] pointers
	 * preventing them to be freed at exit */
        ret->saved_argv = (char**)sctk_malloc(ret->argc * sizeof(char*));
        assume(ret->saved_argv);

        int i;
        for( i = 0 ; i < ret->argc; i++)
        {
                ret->saved_argv[i] = input_args->argv[i];
        }

        ret->argv = (char**)sctk_malloc(ret->argc * sizeof(char*));
        assume(ret->argv);

        int argc = 0;

	for ( i = 0; i < ret->argc; i++ )
	{
		int j;
		int k;
		char *tmp;

		tmp =  ( char * ) sctk_malloc ( ( strlen ( input_args->argv[i] ) + 1 ) * sizeof ( char ) );
		assume( tmp != NULL );

		j = 0;
		k = 0;

		while ( input_args->argv[i][j] != '\0' )
		{
			if ( memcmp( &( input_args->argv[i][j] ), "@MPC_LINK_ARGS@", strlen ( "@MPC_LINK_ARGS@" ) ) != 0 )
			{
				tmp[k] = input_args->argv[i][j];
				j++;
				k++;
			}
			else
			{
				j += strlen ( "@MPC_LINK_ARGS@" );
				tmp[k] = ' ';
				k++;
			}
		}

		tmp[k] = input_args->argv[i][j];

		ret->argv[i] = tmp;
		/* Here we store the pointer to
		 * be able to free it later on
		 * even in case of modification */
		ret->saved_argv[i] = tmp;
	}

	ret->argv[ret->argc] = NULL;

        return ret;
}

static inline void __startup_arg_free(startup_arg_t * arg)
{
        int i;

	for ( i = 0; i < arg->argc; i++ )
	{
		/* Here we free using the copy
		 * of the array to be sure we have
		 * a valid pointer */
		sctk_free ( arg->saved_argv[i] );
	}

	sctk_free ( arg->argv );
	sctk_free ( arg->saved_argv );
        sctk_free(arg);
}

static int main_result = 0;

static void * mpc_launch_vp_start_function ( startup_arg_t *arg )
{
        startup_arg_t * duplicate_args = __startup_arg_extract_and_duplicate(arg);

	/* In libmode there is no main */
#ifndef SCTK_LIB_MODE
        mpc_common_init_trigger("Starting Main");

        #ifdef HAVE_ENVIRON_VAR
                main_result = mpc_user_main(duplicate_args->argc, duplicate_args->argv, environ );
        #else
                main_result = mpc_user_main(duplicate_args->argc, duplicate_args->argv);
        #endif

        mpc_common_init_trigger("Ending Main");
#endif /* SCTK_LIB_MODE */

        __startup_arg_free(duplicate_args);

	return NULL;
}

static int sctk_mpc_env_initialized = 0;
#ifdef SCTK_LINUX_DISABLE_ADDR_RANDOMIZE

#include <asm/unistd.h>
#include <linux/personality.h>
#define THIS__set_personality(pers) syscall(__NR_personality,pers)

static inline void
sctk_disable_addr_randomize ( int argc, char **argv )
{
	bool keep_addr_randomize, disable_addr_randomize;

	if ( sctk_mpc_env_initialized == 0 )
	{
		/* To be check for move into mprcun script */
		return;
		assume ( argc > 0 );
		keep_addr_randomize = sctk_runtime_config_get()->modules.launcher.keep_rand_addr;
		disable_addr_randomize = sctk_runtime_config_get()->modules.launcher.disable_rand_addr;

		if ( !keep_addr_randomize && disable_addr_randomize )
		{
			THIS__set_personality ( ADDR_NO_RANDOMIZE );
			execvp ( argv[0], argv );
		}

		sctk_nodebug ( "current brk %p", sbrk ( 0 ) );
	}
	else
	{
		sctk_warning( "Unable to disable addr ramdomization" );
	}
}
#else
static inline void
sctk_disable_addr_randomize ( int argc, char **argv )
{
}
#endif

static void * ___auto_kill_func ( void *arg )
{
	int timeout = *( int * )arg;

	if ( timeout > 0 )
	{
		if ( sctk_runtime_config_get()->modules.launcher.banner && !sctk_is_in_fortran )
		{
			mpc_common_io_noalloc_fprintf ( stderr, "Autokill in %ds\n", timeout );
		}

		sleep ( timeout );

		if ( !sctk_is_in_fortran )
		{
			mpc_common_io_noalloc_fprintf ( stderr, "TIMEOUT reached\n" );
		}

		abort ();
		exit ( -1 );
	}

	return NULL;
}

static void __create_autokill_thread()
{
        static int auto_kill = 0;

        auto_kill = sctk_runtime_config_get()->modules.launcher.autokill;

	if ( auto_kill > 0 )
	{
		pthread_t pid;
		pthread_create( &pid, NULL, ___auto_kill_func, &auto_kill );
	}
}


static void __set_default_values()
{

	/* Default values */
	// this function is called 2 times, one here and one with the function
	// pointer
	//"sctk_runtime_config_get()->modules.launcher.thread_init.value()"
	// below
	sctk_use_ethread_mxn();
	sctk_def_task_nb( "1" );
	sctk_def_process_nb( "1" );

	// Initializing multithreading mode
	if ( sctk_runtime_config_get()->modules.launcher.thread_init.value )
	{
		sctk_runtime_config_get()->modules.launcher.thread_init.value();
	}

	/* if(strstr(mpc_common_get_flags()->thread_library_kind, "_ng") != NULL){ */
	/*     mpc_common_get_flags()->new_scheduler_engine_enabled=1; */
	/* }else{ */
	/*     mpc_common_get_flags()->new_scheduler_engine_enabled=0; */
	/* } */
	/* Move this in a post-config function */
#ifdef SCTK_LIB_MODE
	/* In LIB mode comm size is inherited from
	 * the host application (not from the launcher) */
	sctk_process_nb_val = mpc_lowcomm_hook_size();
	mpc_common_get_flags()->task_number = mpc_lowcomm_hook_size();
#else
	mpc_common_get_flags()->task_number = sctk_runtime_config_get()->modules.launcher.nb_task;
	sctk_process_nb_val = sctk_runtime_config_get()->modules.launcher.nb_process;
#endif
	sctk_processor_nb_val = sctk_runtime_config_get()->modules.launcher.nb_processor;
	mpc_common_get_flags()->verbosity  = sctk_runtime_config_get()->modules.launcher.verbosity;
	sctk_launcher_mode = sctk_runtime_config_get()->modules.launcher.launcher;
	sctk_profiling_outputs = sctk_runtime_config_get()->modules.launcher.profiling;
	mpc_common_get_flags()->enable_smt_capabilities = sctk_runtime_config_get()->modules.launcher.enable_smt;
	sctk_share_node_capabilities = sctk_runtime_config_get()->modules.launcher.share_node;
	sctk_checkpoint_mode = sctk_runtime_config_get()->modules.launcher.checkpoint;
	sctk_accl_support =
	    sctk_runtime_config_get()->modules.accelerator.enabled;
	/* forece smt on MIC */
#ifdef __MIC__
	mpc_common_get_flags()->enable_smt_capabilities = 1;
#endif
}

static void __unpack_arguments()
{
	char *sctk_argument;
	int argc = 1;
	char **argv;
	void **tofree = NULL;
	int tofree_nb = 0;
	static int auto_kill;
	char *argv_tmp[1];
	argv = argv_tmp;
	argv[0] = "main";

	sctk_argument = getenv ( "MPC_STARTUP_ARGS" );

	if ( sctk_argument != NULL )
	{
		/*    size_t len;*/
		char *cursor;
		int i;
		char **new_argv;
		int new_argc = 0;
		new_argv = ( char ** ) sctk_malloc ( ( argc + 20 ) * sizeof ( char * ) );
		tofree = ( void ** ) sctk_malloc ( ( argc + 20 ) * sizeof ( void * ) );
		tofree[tofree_nb] = new_argv;
		tofree_nb++;
		cursor = sctk_argument;
		new_argv[new_argc] = argv[0];
		new_argc++;

		for ( i = 1; i < argc; i++ )
		{
			new_argv[new_argc] = argv[i];
			new_argc++;
		}

		while ( *cursor == ' ' )
		{
			cursor++;
		}

		while ( *cursor != '\0' )
		{
			int word_len = 0;
			new_argv[new_argc] = ( char * ) sctk_malloc ( 1024 * sizeof ( char ) );
			tofree[tofree_nb] = new_argv[new_argc];
			tofree_nb++;

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
		argc = new_argc;
		argv = new_argv;
		/*  for(i = 0; i <= argc; i++){
		    fprintf(stderr,"%d : %s\n",i,argv[i]);
		    } */
	}


	memcpy ( sctk_save_argument, argv, argc * sizeof ( char * ) );
	sctk_nodebug ( "init argc %d", argc );
	sctk_env_init ( &argc, &argv );
	sctk_nodebug ( "init argc %d", argc );

	mpc_common_get_flags()->exename = argv[0];

	sctk_free( argv );

	if ( tofree != NULL )
	{
		int i;

		for ( i = 0; i < tofree_nb; i++ )
		{
			sctk_free( tofree[i] );
		}

		sctk_free( tofree );
	}
}


static inline void __base_runtime_init()
{
        if ( sctk_mpc_env_initialized == 1 )
	{
		return;
	}

	sctk_mpc_env_initialized = 1;

        mpc_common_init_trigger("Base Runtime Init");

        /* WARNING !! NO CONFIG before this point */

        if( sctk_runtime_config_get()->modules.debugger.mpc_bt_sig )
        {
                sctk_install_bt_sig_handler();
        }

        __create_autokill_thread();
        __set_default_values();
        __unpack_arguments();

	sctk_atomics_cpu_freq_init();
        mpc_common_init_print();

        mpc_common_init_trigger("Base Runtime Init Done");

	mpc_launch_print_banner( 0 /* not in restart mode */ );
}

void sctk_init_mpc_runtime()
{
        __base_runtime_init();
}

static inline void __base_runtime_finalize()
{
#ifdef MPC_Message_Passing
        TODO("FIX THIS ASAP");
	mpc_lowcomm_rdma_window_release_ht();
#endif

#ifdef MPC_USE_EXTLS
	extls_fini();
#endif

        mpc_common_init_trigger("Base Runtime Finalize");
}

#ifndef SCTK_LIB_MODE

int sctk_launch_main ( int argc, char **argv )
{

	startup_arg_t arg;
#ifdef MPC_USE_EXTLS
	extls_init();
	extls_set_context_storage_addr((void*(*)(void))sctk_get_ctx_addr);
#endif

	/* MPC_MAKE_FORTRAN_INTERFACE is set when compiling fortran headers.
	 * To check why ? */
        TODO("See how thiscompares to MPC_CALL_ORIGINAL_MAIN");

	if ( getenv( "MPC_MAKE_FORTRAN_INTERFACE" ) != NULL )
	{
#ifdef HAVE_ENVIRON_VAR
		return mpc_user_main( argc, argv, environ );
#else
		return mpc_user_main( argc, argv );
#endif
	}

	sctk_disable_addr_randomize ( argc, argv );

	__base_runtime_init();


	sctk_nodebug ( "new argc %d", argc );
	arg.argc = argc;
	arg.argv = argv;

	mpc_thread_spawn_virtual_processors ( ( void *( * )( void * ) ) mpc_launch_vp_start_function, &arg );

	sctk_env_exit ();
	__base_runtime_finalize();



	return main_result;
}

#endif /* SCTK_LIB_MODE */

