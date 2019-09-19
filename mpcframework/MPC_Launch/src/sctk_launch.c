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
#if defined(MPC_Message_Passing) || defined(MPC_Threads)

#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sctk_runtime_config.h>
#include "sctk.h"

#if !defined(NO_INTERNAL_ASSERT)
#define SCTK_DEBUG_MODE " Debug MODE"
#else
#define SCTK_DEBUG_MODE ""
#endif

#include "sctk_thread.h"
#include "sctk_launch.h"
#include "sctk_debug.h"
#include "sctk_config.h"
#include "sctk_asm.h"
#include "sctk_atomics.h"
#ifdef MPC_Message_Passing
	#include "sctk_inter_thread_comm.h"
	#include "sctk_low_level_comm.h"
	#ifdef MPC_USE_INFINIBAND
		#include <sctk_ib_cp.h>
	#endif /* MPC_USE_INFINIBAND */
#include "sctk_window.h"
#endif /* MPC_Message_Passing */
#include "sctk_topology.h"
#include "sctk_asm.h"
/* #include "sctk_daemons.h" */
/* #include "sctk_io.h" */
#include "sctk_runtime_config.h"
#include "sctk_bool.h"

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

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

#ifdef HAVE_ENVIRON_VAR
extern char ** environ;
#endif

#ifdef MPC_AIO_ENABLED
#include "sctk_aio.h"
#endif

#define SCTK_LAUNCH_MAX_ARG 4096
static char *sctk_save_argument[SCTK_LAUNCH_MAX_ARG];
static int sctk_initial_argc = 0;
static int sctk_start_argc = 0;
static char **init_argument = NULL;
bool sctk_restart_mode;
bool sctk_checkpoint_mode;
bool sctk_migration_mode;
bool sctk_accl_support;
#define MAX_TERM_LENGTH 80
#define MAX_NAME_FORMAT 30
/* const char *sctk_store_dir = "/dev/null"; */


char *sctk_multithreading_mode;
int sctk_new_scheduler_engine_enabled = 0;
char *sctk_network_mode = "none";
char *sctk_checkpoint_str = "";
bool sctk_enable_smt_capabilities;
bool sctk_enable_graphic_placement;
bool sctk_enable_text_placement;
bool sctk_share_node_capabilities;

double __sctk_profiling__start__sctk_init_MPC;
double __sctk_profiling__end__sctk_init_MPC;
char * sctk_profiling_outputs;

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

	for (i = 0; i < MAX_NAME_FORMAT; i++)
	{
		n[i] = ' ';
	}
	n[MAX_NAME_FORMAT - 1] = '\0';

	sprintf (n, "%s", name);
	n[strlen (name)] = ' ';

	fprintf (stderr, "  %s", n);

	for (i = 0; i < MAX_NAME_FORMAT; i++)
	{
		n[i] = ' ';
	}
	n[MAX_NAME_FORMAT - 1] = '\0';

	tmp_tab = (char *) sctk_malloc ((1 + strlen (def)) * sizeof (char));

	memcpy (tmp_tab, def, (1 + strlen (def)) * sizeof (char));

	tmp = tmp_tab;

	while (strlen (tmp) > (MAX_TERM_LENGTH - MAX_NAME_FORMAT))
	{
		old_tmp = tmp;
		tmp = &(tmp[MAX_TERM_LENGTH - MAX_NAME_FORMAT]);
		while ((tmp > old_tmp) && (tmp[0] != ' '))
		{
			tmp--;
		}
		assume (tmp != old_tmp);
		tmp[0] = '\0';
		fprintf (stderr, "%s\n  %s", old_tmp, n);
		tmp++;
	}
	fprintf (stderr, "%s", tmp);
	fprintf (stderr, "\n");
}

	static inline int
__sctk_add_arg (char *arg, void (*action) (void),
		char *word)
{
	if (strncmp (word, arg, strlen (arg)) == 0)
	{
		action ();
		return 0;
	}
	return 1;
}

#define sctk_add_arg(arg,action) if(__sctk_add_arg(arg,action,word) == 0) return 0

	static inline int
__sctk_add_arg_eq (char *arg,
		void (*action) (char *), char *word)
{
	if (strncmp (word, arg, strlen (arg)) == 0)
	{
		action (&(word[1 + strlen (arg)]));
		return 0;
	}
	return 1;
}

#define sctk_add_arg_eq(arg,action) if(__sctk_add_arg_eq(arg,action,word) == 0) return 0

static bool sctk_version_details_val;
static void (*sctk_thread_val) (void) = NULL;
static int sctk_task_nb_val;
static int sctk_process_nb_val;
static int sctk_processor_nb_val;
static int sctk_node_nb_val;
static int sctk_verbosity;
static char* sctk_launcher_mode;
/* Name of the inter-process driver to use. NULL means default driver */
static char* sctk_network_driver_name = NULL;

/*   void */
/* sctk_set_net_val (void (*val) (int *, char ***)) */
/* { */
/*   sctk_net_val = val; */
/* } */

void sctk_use_pthread(void) {
  sctk_multithreading_mode = "pthread";
  sctk_thread_val = sctk_pthread_thread_init;
}

/* Note that we start with an agressive frequency
 * to speedup the polling during the init phase
 * we relax it after doing driver initialization */
int __polling_done = 0;

void * polling_thread( __UNUSED__ void * dummy )
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
  sctk_bind_to_cpu(-1);

    while(1)
    {
        sctk_network_notify_idle_message();
        
        if( __polling_done )
			break;
    }

	return NULL;
}

void sctk_print_banner(bool restart)
{
	if (get_process_rank() == 0)
	{
		char *mpc_lang = "C/C++";
		if (sctk_is_in_fortran == 1)
		{
			mpc_lang = "Fortran";
		}

		if (sctk_runtime_config_get()->modules.launcher.banner)
		{
			if(sctk_checkpoint_mode && restart)
				fprintf (stderr, "+++ Application restarting from checkpoint with the following configuration:\n");

			if (SCTK_VERSION_MINOR >= 0)
			{
				fprintf (stderr,
						"MPC version %d.%d.%d%s %s (%d tasks %d processes %d cpus (%2.2fGHz) %s) %s %s %s %s\n",
						SCTK_VERSION_MAJOR, SCTK_VERSION_MINOR, SCTK_VERSION_REVISION,
						SCTK_VERSION_PRE, mpc_lang, sctk_task_nb_val,
						sctk_process_nb_val, sctk_get_cpu_number (),sctk_atomics_get_cpu_freq()/1000000000.0,
						sctk_multithreading_mode,
						sctk_alloc_mode (), SCTK_DEBUG_MODE, sctk_checkpoint_str, sctk_network_mode);
			}
			else
			{
				fprintf (stderr,
						"MPC experimental version %s (%d tasks %d processes %d cpus (%2.2fGHz) %s) %s %s %s %s\n",
						mpc_lang, sctk_task_nb_val, sctk_process_nb_val,
						sctk_get_cpu_number (),sctk_atomics_get_cpu_freq()/1000000000.0,  sctk_multithreading_mode,
						sctk_alloc_mode (), SCTK_DEBUG_MODE, sctk_checkpoint_str, sctk_network_mode);
			}
		}
	}
}

static void sctk_perform_initialisation (void)
{
	/*   mkdir (sctk_store_dir, 0777); */
	if(sctk_process_nb_val && sctk_task_nb_val)
	{
		if(sctk_task_nb_val < sctk_process_nb_val)
		{
			sctk_error( "\n--process-nb=%d\n--nb-task=%d\n Nb tasks must be greater than nb processes\n Run your program with option --nb-task=(>=%d) or --nb-process=(<=%d)", sctk_process_nb_val, sctk_task_nb_val, sctk_process_nb_val, sctk_task_nb_val );
			abort();
		}
	}
	sctk_only_once ();

	if (sctk_process_nb_val > 1)
	{
		mpc_common_set_process_count(sctk_process_nb_val);
		sctk_nodebug ("%d processes", get_process_count());
	}

	/* As a first step initialize the PMI */
	sctk_pmi_init();

	sctk_topology_init ();
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
#ifndef SCTK_LIB_MODE
	/* Bind the main thread to the first VP */
	const unsigned int core = 0;
	int binding = 0;
	binding = sctk_get_init_vp (core);
	sctk_bind_to_cpu (binding);
	sctk_nodebug("Init: thread bound to thread %d", binding);
#endif

#ifdef HAVE_HWLOC
	sctk_alloc_posix_mmsrc_numa_init_phase_numa();
#endif

	// MALP FIX...
	char *env = NULL;
	if( (env = getenv( "LD_PRELOAD")) )
	{
		setenv("LD_PRELOAD", "", 1 );
	}

	sctk_locate_dynamic_initializers();

	if( env )
		setenv( "LD_PRELOAD", env, 1 );

	sctk_thread_init ();

	if (sctk_version_details_val)
		sctk_set_version_details ();

#ifdef SCTK_LIB_MODE
	/* In lib mode we force the pthread MODE */
	sctk_use_pthread();
#endif

	if (sctk_thread_val != NULL)
	{
		sctk_thread_val ();
	}
	else
	{
		fprintf (stderr, "No multithreading mode specified!\n");
		abort ();
	}

	sctk_init_alloc ();

	if (sctk_processor_nb_val > 1)
	{
		if (sctk_process_nb_val > 1)
		{
			int cpu_detected;
			cpu_detected = sctk_get_cpu_number ();
			if (cpu_detected < sctk_processor_nb_val)
			{
				fprintf (stderr,
						"Processor number doesn't match number detected %d <> %d!\n",
						cpu_detected, sctk_processor_nb_val);
			}
		}
	}

#ifdef MPC_Message_Passing
	sctk_collectives_init_hook =
		*(void**)(&sctk_runtime_config_get()->modules.inter_thread_comm.collectives_init_hook.value);
	if (sctk_process_nb_val > 1){
		sctk_ptp_per_task_init(-1);
	}
#endif

	if (sctk_task_nb_val)
	{
#ifdef MPC_Message_Passing
		sctk_communicator_world_init (sctk_task_nb_val);
		sctk_communicator_self_init ();
#else
		(void) (0);
#endif
	}
	else
	{
		fprintf (stderr, "No task number specified!\n");
		sctk_abort ();
	}

#ifdef MPC_Profiler
	sctk_internal_profiler_init();
#endif

#ifdef SCTK_LIB_MODE	
	/* In LIB mode we create the task context
	 * at process level (not in an actual task ) */
	int my_rank = get_process_rank();
	sctk_ptp_per_task_init (my_rank);
#endif

	/* Start auxiliary polling thread */
	/*
	   pthread_t progress;
	   pthread_create (&progress, NULL, polling_thread, NULL);
	   */

	sctk_thread_t progress;
	sctk_thread_attr_t attr;
	sctk_thread_attr_init ( &attr );
	sctk_thread_attr_setscope ( &attr, SCTK_THREAD_SCOPE_SYSTEM );

	sctk_user_thread_create(&progress, &attr, polling_thread, NULL);


#ifdef MPC_Message_Passing
	if (sctk_process_nb_val > 1) {
#ifdef MPC_USE_INFINIBAND
		if (!sctk_network_driver_name && !sctk_ib_device_found()) {
			if (!get_process_rank())
				sctk_info("Could not locate an IB device, fallback to TCP");
			sctk_network_driver_name = "tcp";
		}
#endif
		sctk_net_init_driver(sctk_network_driver_name);
	}
	sctk_window_init_ht();
#endif

#ifdef SCTK_LIB_MODE
	sctk_net_init_task_level (my_rank, 0);
#endif

	sctk_atomics_cpu_freq_init();
	sctk_print_banner(0 /* not in restart mode */);

	sctk_flush_version ();

	/* We passed the init phase we can stop the bootstrap polling */
	__polling_done = 1;

}

static void sctk_version_details (void)
{
	sctk_version_details_val = 1;
}
//////////////////////////////////
// OLD SCHEDULER
//
static void sctk_use_ethread (void)
{
  sctk_multithreading_mode = "ethread";
  sctk_thread_val = sctk_ethread_thread_init;
}

	void
sctk_use_ethread_mxn (void)
{
          sctk_multithreading_mode = "ethread_mxn";
          sctk_thread_val = sctk_ethread_mxn_thread_init;
}
//////////////////////////////////
// END OLD SCHEDULER
//

//////////////////////////////////
// SCHEDULER NG
//
/********* ETHREAD MXN ************/
void sctk_use_ethread_mxn_ng(void) {
  sctk_multithreading_mode = "ethread_mxn_ng";
  sctk_thread_val = sctk_ethread_mxn_ng_thread_init;
}

/********* ETHREAD ************/
void sctk_use_ethread_ng(void) {
  sctk_multithreading_mode = "ethread_ng";
  sctk_thread_val = sctk_ethread_ng_thread_init;
}

/********* PTHREAD ************/
void sctk_use_pthread_ng(void) {
  sctk_multithreading_mode = "pthread_ng";
  sctk_thread_val = sctk_pthread_ng_thread_init;
}
/*********  END NG ************/

static void sctk_def_directory(__UNUSED__ char *arg) { /*   sctk_store_dir = arg; */
}

static void sctk_def_task_nb(char *arg) {
  int task_nb;
  task_nb = atoi(arg);
  sctk_task_nb_val = task_nb;
}

	static void
sctk_def_process_nb (char *arg)
{
	sctk_process_nb_val = atoi (arg);
}


void (*sctk_get_thread_val(void)) ()
{
	return sctk_thread_val;
}

	int
sctk_get_process_nb ()
{
	return sctk_process_nb_val;
}

	int
sctk_get_total_tasks_number()
{
	return sctk_task_nb_val;
}


	static void
sctk_def_processor_nb (char *arg)
{
	sctk_processor_nb_val = atoi (arg);
}

	int
sctk_get_processor_nb ()
{
	return sctk_processor_nb_val;
}

	static void
sctk_def_launcher_mode(char *arg)
{
	sctk_launcher_mode = strdup(arg);
}
	char*
sctk_get_launcher_mode()
{
	return sctk_launcher_mode;
}

	static void
sctk_def_node_nb (char *arg)
{
	sctk_node_nb_val = atoi (arg);
}

	int
sctk_get_node_nb()
{
	return sctk_node_nb_val;
}

	static void
sctk_def_enable_smt (__UNUSED__ char *arg)
{
	sctk_enable_smt_capabilities = 1;
}

	static void
sctk_def_disable_smt (__UNUSED__ char *arg)
{
	sctk_enable_smt_capabilities = 0;
}

	static void
sctk_def_text_placement(__UNUSED__ char *arg)
{
	sctk_enable_text_placement = 1;
}

	static void
sctk_def_graphic_placement(__UNUSED__ char *arg)
{
	sctk_enable_graphic_placement = 1;
}

	static void
sctk_def_share_node (__UNUSED__ char *arg)
{
	sctk_share_node_capabilities = 1;
}

	static void
sctk_checkpoint (void)
{
	sctk_checkpoint_mode = 1;
}

	static void
sctk_migration (void)
{
	sctk_checkpoint_mode = 1;
	sctk_migration_mode = 1;
}

	static void
sctk_restart (void)
{
	sctk_restart_mode = 1;
}

        static void sctk_def_accl_support(void) { sctk_accl_support = 1; }

        static void sctk_set_verbosity(char *arg) {
          int tmp = atoi(arg);
          if ((0 <= tmp) && (sctk_verbosity < tmp))
            sctk_verbosity = tmp;
}

	int
sctk_get_verbosity ()
{
	return sctk_verbosity;
}

	static void
sctk_use_network (char *arg)
{
	sctk_network_driver_name = arg;
}


static void sctk_set_profiling(__UNUSED__ char * arg )
{
	/* fprintf(stderr,"BEFORE |%s| |%s|\n",sctk_profiling_outputs,arg); */
#ifdef MPC_Profiler
	if(strcmp(arg,"undef") != 0){
		char* val;

		val = calloc(strlen(arg) + 10,sizeof(char));

		memcpy(val,arg,strlen(arg));

		if( sctk_profile_renderer_check_render_list( val ) )
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
sctk_def_port_number (char *arg)
{
	arg[0] = '\0';
}

	static void
sctk_def_use_host (char *arg)
{
	arg[0] = '\0';
}

	static inline int
sctk_proceed_arg (char *word)
{
	sctk_add_arg_eq ("--enable-smt", sctk_def_enable_smt);
	sctk_add_arg_eq ("--graphic-placement", sctk_def_graphic_placement);
	sctk_add_arg_eq ("--text-placement", sctk_def_text_placement);
	sctk_add_arg_eq ("--disable-smt", sctk_def_disable_smt);

	sctk_add_arg_eq ("--directory", sctk_def_directory);
	sctk_add_arg ("--version-details", sctk_version_details);
	sctk_add_arg_eq ("--mpc-verbose", sctk_set_verbosity);
	sctk_add_arg ("--use-pthread_ng", sctk_use_pthread_ng);
	sctk_add_arg ("--use-pthread", sctk_use_pthread);
	sctk_add_arg ("--use-ethread_mxn_ng", sctk_use_ethread_mxn_ng);
	sctk_add_arg ("--use-ethread_mxn", sctk_use_ethread_mxn);
	sctk_add_arg ("--use-ethread_ng", sctk_use_ethread_ng);
	sctk_add_arg ("--use-ethread", sctk_use_ethread);

	sctk_add_arg_eq ("--sctk_use_network", sctk_use_network);

	/*FOR COMPATIBILITY */
	sctk_add_arg_eq ("--sctk_use_port_number", sctk_def_port_number);
	sctk_add_arg_eq ("--sctk_use_host", sctk_def_use_host);

	sctk_add_arg_eq ("--task-number", sctk_def_task_nb);
	sctk_add_arg_eq ("--process-number", sctk_def_process_nb);
	sctk_add_arg_eq ("--node-number", sctk_def_node_nb);
	sctk_add_arg_eq ("--share-node", sctk_def_share_node);
	sctk_add_arg_eq ("--processor-number", sctk_def_processor_nb);

	sctk_add_arg_eq ("--profiling", sctk_set_profiling);

	sctk_add_arg_eq ("--launcher", sctk_def_launcher_mode);

	sctk_add_arg ("--checkpoint", sctk_checkpoint);
	sctk_add_arg ("--migration", sctk_migration);
	sctk_add_arg ("--restart", sctk_restart);

        sctk_add_arg("--use-accl", sctk_def_accl_support);

        if (strcmp(word, "--sctk-args-end--") == 0)
          return -1;

        fprintf(stderr, "Argument %s Unknown\n", word);
        return -1;
}

	void
sctk_launch_contribution (FILE * file)
{
	int i;
	fprintf (file, "ARGC %d\n", sctk_initial_argc);
	for (i = 0; i < sctk_initial_argc; i++)
	{
		fprintf (file, "%ld %s\n", (long) strlen (sctk_save_argument[i]) + 1,
				sctk_save_argument[i]);
	}
}

static int sctk_env_init_intern(int *argc, char ***argv) {
  int i, j;
  char **new_argv;
  sctk_init();
  sctk_initial_argc = *argc;
  init_argument = *argv;
  sctk_print_version("Init Launch", SCTK_LOCAL_VERSION_MAJOR,
                     SCTK_LOCAL_VERSION_MINOR);

  for (i = 0; i < sctk_initial_argc; i++) {
    sctk_start_argc = i;
    if (strcmp((*argv)[i], SCTK_START_KEYWORD) == 0) {
      sctk_start_argc = i - 1;
      break;
    }
  }

  new_argv = (char**)sctk_malloc(sctk_initial_argc * sizeof(char *));
  sctk_nodebug("LAUNCH allocate %p of size %lu", new_argv,
               (unsigned long)(sctk_initial_argc * sizeof(char *)));

  for (i = 0; i <= sctk_start_argc; i++) {
    new_argv[i] = (*argv)[i];
  }
  for (i = sctk_start_argc + 1; i < sctk_initial_argc; i++) {
    if (strcmp((*argv)[i], SCTK_START_KEYWORD) == 0) {
      *argc = (*argc) - 1;
    } else {
      if (strcmp((*argv)[i], "--sctk-args-end--") == 0) {
        *argc = (*argc) - 1;
      }
      if (sctk_proceed_arg((*argv)[i]) == -1) {
        break;
      }
      (*argv)[i] = "";
      *argc = (*argc) - 1;
    }
  }
  
  if( sctk_enable_smt_capabilities == 1)
  {
     sctk_processor_nb_val *= sctk_get_ht_per_core();
  }
 
  i++;
  for (j = sctk_start_argc + 1; i < sctk_initial_argc; i++) {
    new_argv[j] = (*argv)[i];
    sctk_nodebug("Copy %d %s", j, (*argv)[i]);
    j++;
  }
  *argv = new_argv;
  sctk_nodebug("new argc tmp %d", *argc);
  return 0;
}

	int
sctk_env_init (int *argc, char ***argv)
{
	sctk_env_init_intern (argc, argv);

	if (sctk_restart_mode == 1)
	{
		return 1;
	}
	else
	{
		sctk_perform_initialisation ();
	}
	return 0;
}

	int
sctk_env_exit ()
{
	sctk_topology_destroy();
#ifdef MPC_AIO_ENABLED
	sctk_aio_threads_release();
#endif
	sctk_leave ();
	return 0;
}

	static inline char *
sctk_skip_token (char *p)
{
	while (isspace (*p))
		p++;
	while (*p && !isspace (*p))
		p++;
	while (isspace (*p))
		p++;
	return p;
}

typedef struct
{
	int argc;
	char **argv;
} sctk_startup_args_t;

static int main_result = 0;

static void * run (sctk_startup_args_t * arg)
{
	int argc;
	char **argv, **argv_safe;
	int i;

	argc = arg->argc;
	argv = (char **) sctk_malloc ((argc + 1) * sizeof (char *));

	assume( argv!= NULL );

	/* We create this extra argv array
	 * to prevent the case where a (strange)
	 * program modifies the argv[i] pointers
	 * preventing them to be freed at exit */
	argv_safe =  (char **) sctk_malloc ((argc + 1) * sizeof (char *));
	assume( argv_safe != NULL );



	sctk_nodebug ("creation argv %d", argc);
	for (i = 0; i < argc; i++)
	{
		int j;
		int k;
		char *tmp;
		sctk_nodebug ("%d %s", i, arg->argv[i]);

		tmp =  (char *) sctk_malloc ((strlen (arg->argv[i]) + 1) * sizeof (char));
		assume( tmp != NULL );

		j = 0;
		k = 0;
		while (arg->argv[i][j] != '\0')
		{
			if (memcmp
					(&(arg->argv[i][j]), "@MPC_LINK_ARGS@",
					 strlen ("@MPC_LINK_ARGS@")) != 0)
			{
				tmp[k] = arg->argv[i][j];
				j++;
				k++;
			}
			else
			{
				j += strlen ("@MPC_LINK_ARGS@");
				tmp[k] = ' ';
				k++;
			}
		}
		tmp[k] = arg->argv[i][j];

		argv[i] = tmp;
		/* Here we store the pointer to
		 * be able to free it later on
		 * even in case of modification */
		argv_safe[i] = tmp;

		sctk_nodebug ("%d %s done", i, argv[i]);
	}
	sctk_nodebug ("creation argv done");

	argv[argc] = NULL;



/* In libmode there is no main */
#ifndef SCTK_LIB_MODE
	main_result = sctk_user_main (argc, argv);
#endif /* SCTK_LIB_MODE */
	
	for (i = 0; i < argc; i++)
	{
		/* Here we free using the copy
		 * of the array to be sure we have
		 * a valid pointer */  
		sctk_free (argv_safe[i]);
	}
	sctk_free (argv);
	sctk_free (argv_safe);

	return NULL;
}

static int sctk_mpc_env_initialized = 0;
#ifdef SCTK_LINUX_DISABLE_ADDR_RANDOMIZE

#include <asm/unistd.h>
#include <linux/personality.h>
#define THIS__set_personality(pers) syscall(__NR_personality,pers)

	static inline void
sctk_disable_addr_randomize (int argc, char **argv)
{
	bool keep_addr_randomize, disable_addr_randomize;
	if(sctk_mpc_env_initialized == 0){
		/* To be check for move into mprcun script */
		return;
		assume (argc > 0);
		keep_addr_randomize = sctk_runtime_config_get()->modules.launcher.keep_rand_addr;
		disable_addr_randomize = sctk_runtime_config_get()->modules.launcher.disable_rand_addr;
		if (!keep_addr_randomize && disable_addr_randomize)
		{
			THIS__set_personality (ADDR_NO_RANDOMIZE);
			execvp (argv[0], argv);
		}
		sctk_nodebug ("current brk %p", sbrk (0));
	} else {
		sctk_warning("Unable to disable addr ramdomization");
	}
}
#else
	static inline void
sctk_disable_addr_randomize (int argc, char **argv)
{

}
#endif

	static void *
auto_kill_func (void *arg)
{
	int timeout = *(int*)arg;
	if (timeout > 0)
	{
		if (sctk_runtime_config_get()->modules.launcher.banner && !sctk_is_in_fortran)
		{
			sctk_noalloc_fprintf (stderr, "Autokill in %ds\n", timeout);
		}
		sleep (timeout);

		if( !sctk_is_in_fortran )
			sctk_noalloc_fprintf (stderr, "TIMEOUT reached\n");

		abort ();
		exit (-1);
	}
	return NULL;
}

#ifdef MPC_MPI
void mpc_MPI_allocmem_adapt(char *exename);
#endif

void sctk_init_mpc_runtime()
{
	if(sctk_mpc_env_initialized == 1)
		return;

	char *sctk_argument;

	int argc = 1;
	char **argv;
	void **tofree = NULL;
	int tofree_nb = 0;
	static int auto_kill;
	char * argv_tmp[1];


	argv = argv_tmp;
	argv[0] = "main";

	sctk_mpc_env_initialized = 1;

	//load mpc configuration from XML files if not already done.
	sctk_runtime_config_init();

        // Now attach signal handlers (if allowed by the config)
        sctk_install_bt_sig_handler();

        __sctk_profiling__start__sctk_init_MPC =
            sctk_get_time_stamp_gettimeofday();

        auto_kill = sctk_runtime_config_get()->modules.launcher.autokill;
        if (auto_kill > 0) {
          pthread_t pid;
          /*       sctk_warning ("Auto kill in %s s",auto_kill); */
          pthread_create(&pid, NULL, auto_kill_func, &auto_kill);
        }

        /* Default values */
        // this function is called 2 times, one here and one with the function
        // pointer
        //"sctk_runtime_config_get()->modules.launcher.thread_init.value()"
        // below

        sctk_use_ethread_mxn();
        sctk_def_task_nb("1");
        sctk_def_process_nb("1");

        // Initializing multithreading mode
        if (sctk_runtime_config_get()->modules.launcher.thread_init.value)
          sctk_runtime_config_get()->modules.launcher.thread_init.value();

/* if(strstr(sctk_multithreading_mode, "_ng") != NULL){ */
/*     sctk_new_scheduler_engine_enabled=1; */
/* }else{ */
/*     sctk_new_scheduler_engine_enabled=0; */
/* } */

/* Move this in a post-config function */

#ifdef SCTK_LIB_MODE
  /* In LIB mode comm size is inherited from
   * the host application (not from the launcher) */
  sctk_process_nb_val = MPC_Net_hook_size();
  sctk_task_nb_val = MPC_Net_hook_size();
#else
	sctk_task_nb_val = sctk_runtime_config_get()->modules.launcher.nb_task;
	sctk_process_nb_val = sctk_runtime_config_get()->modules.launcher.nb_process;
#endif
	sctk_processor_nb_val = sctk_runtime_config_get()->modules.launcher.nb_processor;
	sctk_node_nb_val = sctk_runtime_config_get()->modules.launcher.nb_node;
	sctk_verbosity = sctk_runtime_config_get()->modules.launcher.verbosity;
	sctk_launcher_mode = sctk_runtime_config_get()->modules.launcher.launcher;
	sctk_version_details_val = sctk_runtime_config_get()->modules.launcher.vers_details;
	sctk_profiling_outputs = sctk_runtime_config_get()->modules.launcher.profiling; 
	sctk_enable_smt_capabilities = sctk_runtime_config_get()->modules.launcher.enable_smt;
	sctk_share_node_capabilities = sctk_runtime_config_get()->modules.launcher.share_node;
	sctk_restart_mode = sctk_runtime_config_get()->modules.launcher.restart;
	sctk_checkpoint_mode = sctk_runtime_config_get()->modules.launcher.checkpoint;
	sctk_migration_mode = sctk_runtime_config_get()->modules.launcher.migration;
        sctk_accl_support =
            sctk_runtime_config_get()->modules.accelerator.enabled;
/* forece smt on MIC */
#ifdef __MIC__
	sctk_enable_smt_capabilities = 1;
#endif

	/*   sctk_exception_catch (11); */

	/*   sctk_set_execuatble_name (argv[0]); */

	/*
	   if (sctk_runtime_config_get()->modules.launcher.disable_mpc)
	   {
	   sctk_pthread_thread_init ();
	   sctk_thread_init_no_mpc ();
	   return mpc_user_main (argc, argv);
	   }
	 */



	sctk_argument = getenv ("MPC_STARTUP_ARGS");

	if (sctk_argument != NULL)
	{
		/*    size_t len;*/
		char *cursor;
		int i;
		char **new_argv;
		int new_argc = 0;

		new_argv = (char **) sctk_malloc ((argc + 20) * sizeof (char *));
		tofree = (void **) sctk_malloc ((argc + 20) * sizeof (void *));

		tofree[tofree_nb] = new_argv;
		tofree_nb++;

		cursor = sctk_argument;
		new_argv[new_argc] = argv[0];
		new_argc++;

		for (i = 1; i < argc; i++)
		{
			new_argv[new_argc] = argv[i];
			new_argc++;
		}

		while (*cursor == ' ')
			cursor++;
		while (*cursor != '\0')
		{
			int word_len = 0;
			new_argv[new_argc] = (char *) sctk_malloc (1024 * sizeof (char));
			tofree[tofree_nb] = new_argv[new_argc];
			tofree_nb++;
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
		argc = new_argc;
		argv = new_argv;

		/*  for(i = 0; i <= argc; i++){
		    fprintf(stderr,"%d : %s\n",i,argv[i]);
		    } */
	}

	memcpy (sctk_save_argument, argv, argc * sizeof (char *));

	sctk_nodebug ("init argc %d", argc);
	sctk_env_init (&argc, &argv);
	sctk_nodebug ("init argc %d", argc);

#ifdef MPC_MPI
        mpc_MPI_allocmem_adapt(argv[0]);
#endif

        sctk_free(argv);

        if (tofree != NULL) {
          int i;
          for (i = 0; i < tofree_nb; i++) {
            sctk_free(tofree[i]);
          }
          sctk_free(tofree);
        }
}

void sctk_release_mpc_runtime() {
  sctk_window_release_ht();
#ifdef MPC_USE_EXTLS
  extls_fini();
#endif
}

#ifndef SCTK_LIB_MODE

int sctk_launch_main (int argc, char **argv)
{
	sctk_startup_args_t arg;

#ifdef MPC_USE_EXTLS
	extls_init();
	extls_set_context_storage_addr((void*(*)(void))sctk_get_ctx_addr);
#endif
	/* MPC_MAKE_FORTRAN_INTERFACE is set when compiling fortran headers.
	 * To check why ? */
	if (getenv("MPC_MAKE_FORTRAN_INTERFACE") != NULL) {
#ifdef HAVE_ENVIRON_VAR
		return mpc_user_main(argc, argv, environ);
#else
		return mpc_user_main(argc, argv);
#endif
	}

	sctk_disable_addr_randomize (argc,argv);
	sctk_init_mpc_runtime();

	sctk_nodebug ("new argc %d", argc);

	arg.argc = argc;
	arg.argv = argv;


	/*   do{ */
	/*     int i; */
	/*     for(i = 0; i <= argc; i++){ */
	/*       fprintf(stderr,"%d : %s\n",i,argv[i]);       */
	/*     } */
	/*   }while(0); */

	/*   sctk_exception_catch (11); */

	sctk_start_func ((void *(*)(void *)) run, &arg);
	sctk_env_exit ();

        sctk_release_mpc_runtime();

        return main_result;
}

#endif /* SCTK_LIB_MODE */

#endif
