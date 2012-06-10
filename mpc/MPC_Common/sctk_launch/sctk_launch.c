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
#ifdef MPC_Message_Passing
#include "sctk_inter_thread_comm.h"
#include "sctk_low_level_comm.h"
#endif
#include "sctk_topology.h"
#include "sctk_asm.h"
#include "sctk_tls.h"
/* #include "sctk_daemons.h" */
/* #include "sctk_io.h" */
#include "sctk_runtime_config.h"

#ifdef MPC_Profiler
#include "sctk_profile_render.h"
#endif

#define SCTK_START_KEYWORD "--sctk-args--"

#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

#define SCTK_LAUNCH_MAX_ARG 4096
static char *sctk_save_argument[SCTK_LAUNCH_MAX_ARG];
static int sctk_initial_argc = 0;
static int sctk_start_argc = 0;
static char **init_argument = NULL;
int sctk_restart_mode = 0;
int sctk_check_point_restart_mode = 0;
int sctk_migration_mode = 0;
#define MAX_TERM_LENGTH 80
#define MAX_NAME_FORMAT 30
char *sctk_mono_bin = "";
char *sctk_store_dir = "/dev/null";
static char topology_name[SCTK_MAX_FILENAME_SIZE];

char *sctk_multithreading_mode = "none";
char *sctk_network_mode = "none";
int sctk_enable_smt_capabilities = 0;
int sctk_share_node_capabilities = 0;

double __sctk_profiling__start__sctk_init_MPC;
double __sctk_profiling__end__sctk_init_MPC;
char * sctk_profiling_outputs = "none";


void
format_output (char *name, char *def)
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
__sctk_add_arg_eq (char *arg, char *argeq,
    void (*action) (char *), char *word)
{
  if (strncmp (word, arg, strlen (arg)) == 0)
  {
    action (&(word[1 + strlen (arg)]));
    return 0;
  }
  return 1;
}

#define sctk_add_arg_eq(arg,action) if(__sctk_add_arg_eq(arg,arg"=n",action,word) == 0) return 0

static int sctk_version_details_val = 0;
static void (*sctk_thread_val) (void) = NULL;
static void (*sctk_net_val) (char*) = NULL;
static char* sctk_net_val_arg  = "none";
static int sctk_task_nb_val = 0;
static int sctk_process_nb_val = 0;
static int sctk_processor_nb_val = 0;
static int sctk_node_nb_val = 0;
static int sctk_verbosity = 0;
static char* sctk_launcher_mode = "none";

/*   void */
/* sctk_set_net_val (void (*val) (int *, char ***)) */
/* { */
/*   sctk_net_val = val; */
/* } */

  static void
sctk_perform_initialisation (void)
{
  mkdir (sctk_store_dir, 0777);

  sctk_only_once ();

  if (sctk_process_nb_val > 1)
  {
    if (sctk_net_val != NULL)
    {
      sctk_process_number = sctk_process_nb_val;
      sctk_nodebug ("%d processes", sctk_process_number);
    }
    else
    {
      fprintf (stderr, "No network support specified\n");
      sctk_abort();
    }
  }
  sctk_topology_init ();
  #ifdef HAVE_LIBNUMA
  sctk_alloc_posix_mmsrc_numa_init_phase_numa();
  #endif
  sctk_hls_build_repository();
  sctk_thread_init ();


  if (sctk_version_details_val)
    sctk_set_version_details ();

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
    if (sctk_net_val != NULL)
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
  if (sctk_net_val != NULL){
    sctk_ptp_per_task_init(-1);
    sctk_net_init_pmi();
  }
#endif

  if (sctk_task_nb_val)
  {
#ifdef MPC_Message_Passing
    sctk_communicator_world_init (sctk_task_nb_val);
    sctk_communicator_self_init (sctk_task_nb_val);
#else
    (void) (0);
#endif
  }
  else
  {
    fprintf (stderr, "No task number specified!\n");
    sctk_abort ();
  }

#ifdef MPC_Message_Passing
  if (sctk_net_val != NULL){
    sctk_net_val (sctk_net_val_arg);
  }
#endif


  if (sctk_process_rank == 0)
  {
    char *mpc_lang = "C/C++";
    if (sctk_is_in_fortran == 1)
    {
      mpc_lang = "Fortran";
    }

    if (sctk_config_runtime_get()->modules.launcher.banner)
    {
      if (SCTK_VERSION_MINOR >= 0)
      {
        fprintf (stderr,
            "MPC version %d.%d.%d%s %s (%d tasks %d processes %d cpus %s) %s%s%s\n",
            SCTK_VERSION_MAJOR, SCTK_VERSION_MINOR, SCTK_VERSION_REVISION,
            SCTK_VERSION_PRE, mpc_lang, sctk_task_nb_val,
            sctk_process_nb_val, sctk_get_cpu_number (),
            sctk_multithreading_mode,
            sctk_alloc_mode (), SCTK_DEBUG_MODE, sctk_network_mode);
      }
      else
      {
        fprintf (stderr,
            "MPC experimental version %s (%d tasks %d processes %d cpus %s) %s%s%s\n",
            mpc_lang, sctk_task_nb_val, sctk_process_nb_val,
            sctk_get_cpu_number (), sctk_multithreading_mode,
            sctk_alloc_mode (), SCTK_DEBUG_MODE, sctk_network_mode);
      }
    }
    if (sctk_restart_mode == 1)
    {
      fprintf (stderr, "Restart Job\n");
    }
  }
  {
    FILE *topo_file = NULL;
    int max_try = 10;
    sprintf (topology_name, "%s/Process_%d_topology", sctk_store_dir,
        sctk_process_rank);
    do
    {
      topo_file = fopen (topology_name, "w");
      max_try--;
    }
    while ((topo_file == NULL) && (max_try >= 0));
    if (topo_file != NULL)
    {
      sctk_print_topology (topo_file);
      fclose (topo_file);
    }
  }
  sctk_flush_version ();
}

  static void
sctk_version_details (void)
{
  sctk_version_details_val = 1;
}

  static void
sctk_use_pthread (void)
{
  sctk_multithreading_mode = "pthread";
  sctk_thread_val = sctk_pthread_thread_init;
}

  static void
sctk_use_ethread (void)
{
  sctk_multithreading_mode = "ethread";
  sctk_thread_val = sctk_ethread_thread_init;
}

  static void
sctk_use_ethread_mxn (void)
{
  sctk_multithreading_mode = "ethread_mxn";
  sctk_thread_val = sctk_ethread_mxn_thread_init;
}

/*   static void */
/* sctk_use_mpi (void) */
/* { */
/* #ifdef MPC_Message_Passing */
/*   sctk_network_mode = "mpi"; */
/*   sctk_net_init_driver ("mpi"); */
/* #endif */
/* } */

/*   static void */
/* sctk_use_tcp (void) */
/* { */
/* #ifdef MPC_Message_Passing */
/*   sctk_network_mode = "tcp"; */
/*   sctk_net_init_driver ("tcp"); */
/* #endif */
/* } */

  static void
sctk_def_directory (char *arg)
{
  sctk_store_dir = arg;
}

  static void
sctk_def_task_nb (char *arg)
{
  int task_nb;
  task_nb = atoi (arg);
  sctk_task_nb_val = task_nb;
}

  static void
sctk_def_mono (char *arg)
{
  sctk_mono_bin = arg;
}

  static void
sctk_def_process_nb (char *arg)
{
  sctk_process_nb_val = atoi (arg);
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

  static void
sctk_def_enable_smt (char *arg)
{
  sctk_enable_smt_capabilities = 1;
}

  static void
sctk_def_share_node (char *arg)
{
  sctk_share_node_capabilities = 1;
}

  static void
sctk_checkpoint (void)
{
  sctk_nodebug ("use sctk_check_point_restart_mode");
  sctk_check_point_restart_mode = 1;
}

  static void
sctk_migration (void)
{
  sctk_check_point_restart_mode = 1;
  sctk_migration_mode = 1;
}

  static void
sctk_restart (void)
{
  sctk_restart_mode = 1;
}

  static void
sctk_set_verbosity (char *arg)
{
  int tmp =  atoi (arg);
  if( (0 <= tmp) && (sctk_verbosity < tmp) )
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
#ifdef MPC_Message_Passing
  sctk_network_mode = arg;
  sctk_net_val_arg = arg;
  sctk_net_val = sctk_net_init_driver;
#endif

#if 0
  /* if the network mode is different to none,
   * we initialize it. */
#ifdef MPC_Message_Passing
  if (strcmp(arg, "none"))
  {
    sctk_network_mode = arg;
    sctk_net_init_driver (arg);
  }
#endif
#endif
}


static void sctk_set_profiling( char * arg )
{
#ifdef MPC_Profiler
	if( sctk_profile_renderer_check_render_list( arg ) )
	{
		sctk_error( "Provided profiling output syntax is not correct: %s", arg );
		abort();
	}

	sctk_profiling_outputs = arg;
#endif
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
sctk_threat_arg (char *word)
{
  sctk_add_arg_eq ("--directory", sctk_def_directory);
  sctk_add_arg ("--version-details", sctk_version_details);
  sctk_add_arg_eq ("--mpc-verbose", sctk_set_verbosity);
  sctk_add_arg ("--use-pthread", sctk_use_pthread);
  sctk_add_arg ("--use-ethread_mxn", sctk_use_ethread_mxn);
  sctk_add_arg ("--use-ethread", sctk_use_ethread);

/*   sctk_add_arg ("--use-mpi", sctk_use_mpi); */
/*   sctk_add_arg ("--use-tcp", sctk_use_tcp); */
  sctk_add_arg_eq ("--sctk_use_network", sctk_use_network);

  /*FOR COMPATIBILITY */
  sctk_add_arg_eq ("--sctk_use_port_number", sctk_def_port_number);
  sctk_add_arg_eq ("--sctk_use_host", sctk_def_use_host);

  sctk_add_arg_eq ("--task-number", sctk_def_task_nb);
  sctk_add_arg_eq ("--processor-number", sctk_def_processor_nb);
  sctk_add_arg_eq ("--process-number", sctk_def_process_nb);
  sctk_add_arg_eq ("--node-number", sctk_def_node_nb);
  sctk_add_arg_eq ("--enable-smt", sctk_def_enable_smt);
  sctk_add_arg_eq ("--share-node", sctk_def_share_node);

  sctk_add_arg_eq ("--profiling", sctk_set_profiling);

  sctk_add_arg_eq ("--launcher", sctk_def_launcher_mode);

  sctk_add_arg ("--checkpoint", sctk_checkpoint);
  sctk_add_arg ("--migration", sctk_migration);
  sctk_add_arg ("--restart", sctk_restart);

  sctk_add_arg_eq ("--mono", sctk_def_mono);


  if (strcmp (word, "--sctk-args-end--") == 0)
    return -1;

  fprintf (stderr, "Argument %s Unknown\n", word);
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

  static int
sctk_env_init_intern (int *argc, char ***argv)
{
  int i, j;
  char **new_argv;
  sctk_init ();
  sctk_initial_argc = *argc;
  init_argument = *argv;
  sctk_print_version ("Init Launch", SCTK_LOCAL_VERSION_MAJOR,
      SCTK_LOCAL_VERSION_MINOR);

  for (i = 0; i < sctk_initial_argc; i++)
  {
    sctk_start_argc = i;
    if (strcmp ((*argv)[i], SCTK_START_KEYWORD) == 0)
    {
      sctk_start_argc = i - 1;
      break;
    }
  }

  new_argv = sctk_malloc (sctk_initial_argc * sizeof (char *));
  sctk_nodebug ("LAUNCH allocate %p of size %lu", new_argv,
      (unsigned long) (sctk_initial_argc * sizeof (char *)));


  for (i = 0; i <= sctk_start_argc; i++)
  {
    new_argv[i] = (*argv)[i];
  }
  for (i = sctk_start_argc + 1; i < sctk_initial_argc; i++)
  {
    if (strcmp ((*argv)[i], SCTK_START_KEYWORD) == 0)
    {
      *argc = (*argc) - 1;
    }
    else
    {
      if (strcmp ((*argv)[i], "--sctk-args-end--") == 0)
      {
        *argc = (*argc) - 1;
      }
      if (sctk_threat_arg ((*argv)[i]) == -1)
      {
        break;
      }
      (*argv)[i] = "";
      *argc = (*argc) - 1;
    }
  }
  i++;
  for (j = sctk_start_argc + 1; i < sctk_initial_argc; i++)
  {
    new_argv[j] = (*argv)[i];
    sctk_nodebug ("Copy %d %s", j, (*argv)[i]);
    j++;
  }
  *argv = new_argv;
  sctk_nodebug ("new argc tmp %d", *argc);
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

//int
//sctk_initialisation (char *args, int *argc, char ***argv)
//{
//  char tmp[500];
//  char *cursor;
//  char *next;
//  sctk_env_init_intern (argc, argv);
//  cursor = args;
//  while (cursor[0] != '\0')
//    {
//      next = sctk_skip_token (cursor);
//      memcpy (tmp, cursor, (size_t) next - (size_t) cursor);
//      tmp[(size_t) next - (size_t) cursor] = '\0';
//      if (sctk_threat_arg (tmp, 0) == -1)
//	break;
//      cursor = next;
//    }
//  sctk_perform_initialisation ();
//  return 0;
//}

typedef struct
{
  int argc;
  char **argv;
} sctk_startup_args_t;

static int main_result = 0;

  static void *
run (sctk_startup_args_t * arg)
{
  int argc;
  char **argv;
  int i;

  argc = arg->argc;
  argv = (char **) sctk_malloc ((argc + 1) * sizeof (char *));

  sctk_nodebug ("creation argv %d", argc);
  for (i = 0; i < argc; i++)
  {
    int j;
    int k;
    char *tmp;
    sctk_nodebug ("%d %s", i, arg->argv[i]);
    tmp =
      (char *) sctk_malloc ((strlen (arg->argv[i]) + 1) * sizeof (char));
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
    sctk_nodebug ("%d %s done", i, argv[i]);
  }
  sctk_nodebug ("creation argv done");

  argv[argc] = NULL;

  main_result = sctk_user_main (argc, argv);
  for (i = 0; i < argc; i++)
  {
    sctk_free (argv[i]);
  }
  sctk_free (argv);

  return NULL;
}

#ifdef SCTK_LINUX_DISABLE_ADDR_RADOMIZE
#include <asm/unistd.h>
#include <linux/personality.h>
#define THIS__set_personality(pers) ((long)syscall(__NR_personality,pers))

  static inline void
sctk_disable_addr_randomize (int argc, char **argv)
{
  char *disable_addr_randomize;
  assume (argc > 0);
  if (getenv ("SCTK_LINUX_KEEP_ADDR_RADOMIZE") == NULL)
  {
    disable_addr_randomize = getenv ("SCTK_LINUX_DISABLE_ADDR_RADOMIZE");
    if (disable_addr_randomize)
    {
      unsetenv ("SCTK_LINUX_DISABLE_ADDR_RADOMIZE");
      if (sctk_config_runtime_get()->modules.launcher.banner)
      {
INFO("Addr randomize disabled for large scale runs")
//        sctk_warning ("Restart execution to disable addr randomize");
      }
      THIS__set_personality (ADDR_NO_RANDOMIZE);
      execvp (argv[0], argv);
    }
    sctk_nodebug ("current brk %p", sbrk (0));
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
    if (sctk_config_runtime_get()->modules.launcher.banner)
    {
      sctk_noalloc_fprintf (stderr, "Autokill in %ds\n", timeout);
    }
    sleep (timeout);
    sctk_noalloc_fprintf (stderr, "TIMEOUT reached\n");
    abort ();
    exit (-1);
  }
  return NULL;
}

  int
sctk_launch_main (int argc, char **argv)
{
  sctk_startup_args_t arg;
  char name[SCTK_MAX_FILENAME_SIZE];
  FILE *file;
  int init_res;
  char *sctk_argument;
  char *sctk_disable_mpc;
  void **tofree = NULL;
  int tofree_nb = 0;
  int auto_kill;

  //load mpc configuration from XML files if not already done.
  sctk_runtime_config_init();

  sctk_disable_addr_randomize (argc, argv);

  __sctk_profiling__start__sctk_init_MPC = sctk_get_time_stamp_gettimeofday ();
  

  auto_kill = sctk_config_runtime_get()->modules.launcher.autokill;
  if (auto_kill > 0)
  {
    pthread_t pid;
    /*       sctk_warning ("Auto kill in %s s",auto_kill); */
    pthread_create (&pid, NULL, auto_kill_func, &auto_kill);
  }

  sctk_use_ethread_mxn ();
  sctk_def_task_nb ("1");
  sctk_def_process_nb ("1");
  /*   sctk_exception_catch (11); */

  /*   sctk_set_execuatble_name (argv[0]); */

  sctk_disable_mpc = getenv ("MPC_DISABLE");
  if (sctk_disable_mpc != NULL)
  {
    if (strcmp ("1", sctk_disable_mpc) == 0)
    {
      sctk_pthread_thread_init ();
      sctk_thread_init_no_mpc ();
      return mpc_user_main (argc, argv);
    }
  }

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
  init_res = sctk_env_init (&argc, &argv);
  sctk_nodebug ("init argc %d", argc);


  if (init_res == 1)
  {
    int i;
    int size;
    int nb_processes;
    int nb_args;
    sprintf (name, "%s/Job_description", sctk_store_dir);

    sctk_nodebug ("Perform restart from protection");

    file = fopen (name, "r");
    assume(fscanf (file, "Job with %d tasks on %d processes\n", &size,
		   &nb_processes) == 2);
    sctk_nodebug ("Previous run with params %d tasks %d processes",
        size, nb_processes);

    assume(fscanf (file, "ARGC %d\n", &nb_args) == 1);
    sctk_nodebug ("ARGC %d", nb_args);

    argv = (char **) sctk_malloc ((nb_args + 1) * sizeof (char *));
    argc = nb_args;

    for (i = 0; i < nb_args; i++)
    {
      long arg_size;
      assume(fscanf (file, "%ld ", &arg_size) == 1);
      argv[i] = (char *) sctk_malloc (arg_size * sizeof (char));
      assume(fscanf (file, "%s\n", argv[i]) == 1);
      sctk_nodebug ("Arg read %s", argv[i]);
    }
    argv[nb_args] = NULL;

    fclose (file);
    memcpy (sctk_save_argument, argv, argc * sizeof (char *));

    sctk_env_init_intern (&argc, &argv);
    sctk_perform_initialisation ();
    sctk_nodebug ("Launch environement restored");
  }
  else
  {
    sprintf (name, "%s/Job_description", sctk_store_dir);
    file = fopen (name, "r");
    if (file != NULL)
    {
      if (sctk_process_rank == 0)
      {
        fprintf (stderr,
            "%s is not clean; specify a clean directiry using --tmp_dir= or use mpc_clean %s\n",
            sctk_store_dir, sctk_store_dir);
      }
      exit (1);
    }
  }
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

  sprintf (name, "%s/Job_description", sctk_store_dir);
  remove (name);
  sprintf (name, "%s/mpcrun_args", sctk_store_dir);
  remove (name);
  sprintf (name, "%s/last_point", sctk_store_dir);
  remove (name);
  sprintf (name, "%s/Process_%d_topology", sctk_store_dir, sctk_process_rank);
  remove (name);
  sprintf (name, "%s/use_%s_%d", sctk_store_dir, sctk_get_node_name (),
      getpid ());
  remove (name);
  if (sctk_process_rank == 0)
  {
    int i;
    int j;
    i = 1;
    j = 0;
    do
    {
      j = 1;
      do
      {
        sprintf (name, "%s/communicator_%d_%d", sctk_store_dir, i, j);
        j++;
      }
      while (remove (name) == 0);
      sprintf (name, "%s/communicator_%d_%d", sctk_store_dir, i, 0);
      i++;
    }
    while (remove (name) == 0);
  }
  sprintf (name, "%s/communicators", sctk_store_dir);
  remove (name);
  rmdir (sctk_store_dir);
  sctk_free (argv);
  if (tofree != NULL)
  {
    int i;
    for (i = 0; i < tofree_nb; i++)
    {
      sctk_free (tofree[i]);
    }
    sctk_free (tofree);
  }
  /*   sctk_close_io(); */
  return main_result;
}

#endif
