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
#include <mpc_common_rank.h>

#include <sctk_alloc.h>

#include "mpc_launch.h"
#include "sctk_debug.h"
#include "mpc_common_asm.h"
#include "opa_primitives.h"

#include "mpc_topology.h"
#include <mpc_launch_pmi.h>

#include "mpc_runtime_config.h"
#include <mpc_common_types.h>

#include "main.h"


#if defined(MPC_ENABLE_DEBUG_MESSAGES)
#define SCTK_DEBUG_MODE    " Debug Messages Enabled"
#else
#define SCTK_DEBUG_MODE    ""
#endif


void mpc_launch_print_banner(bool restart)
{
	if(mpc_common_get_process_rank() == 0)
	{
		char *mpc_lang = "C/C++";

		if(mpc_common_get_flags()->is_fortran == 1)
		{
			mpc_lang = "Fortran";
		}

		if(sctk_runtime_config_get()->modules.launcher.banner)
		{
			if(mpc_common_get_flags()->checkpoint_enabled && restart)
			{
				mpc_common_debug_log("+++ Application restarting from checkpoint with the following configuration:");
			}

			mpc_common_debug_log("--------------------------------------------------------");
			mpc_common_debug_log("MPC version: %s", MPC_VERSION_STRING);
			mpc_common_debug_log("Running    : %s code", mpc_lang);
			mpc_common_debug_log("Setup      : %d tasks %d processes %d cpus",
			                     mpc_common_get_flags()->task_number,
			                     mpc_common_get_flags()->process_number,
			                     mpc_topology_get_pu_count() );
			mpc_common_debug_log("Threading  : '%s'", mpc_common_get_flags()->thread_library_kind);
			mpc_common_debug_log("Allocator  : %s %s %s", sctk_alloc_mode(), SCTK_DEBUG_MODE, mpc_common_get_flags()->checkpoint_model);
			mpc_common_debug_log("C/R        : %s", mpc_common_get_flags()->checkpoint_model);
			mpc_common_debug_log("Debug      : %s", SCTK_DEBUG_MODE);
			mpc_common_debug_log("Networks   : %s", mpc_common_get_flags()->sctk_network_description_string);
			mpc_common_debug_log("--------------------------------------------------------");
		}
	}
}

/********************
* ARGUMENT SETTERS *
********************/

void sctk_use_pthread(void)
{
	mpc_common_get_flags()->thread_library_kind = "pthread";
	mpc_common_get_flags()->thread_library_init = mpc_thread_pthread_engine_init;
}

static void sctk_use_ethread(void)
{
	mpc_common_get_flags()->thread_library_kind = "ethread";
	mpc_common_get_flags()->thread_library_init = mpc_thread_ethread_engine_init;
}

void sctk_use_ethread_mxn(void)
{
	mpc_common_get_flags()->thread_library_kind = "ethread_mxn";
	mpc_common_get_flags()->thread_library_init = mpc_thread_ethread_mxn_engine_init;
}

void sctk_use_ethread_mxn_ng(void)
{
	mpc_common_get_flags()->thread_library_kind = "ethread_mxn_ng";
	mpc_common_get_flags()->thread_library_init = mpc_thread_ethread_mxn_ng_engine_init;
}

void sctk_use_ethread_ng(void)
{
	mpc_common_get_flags()->thread_library_kind = "ethread_ng";
	mpc_common_get_flags()->thread_library_init = mpc_thread_ethread_ng_engine_init;
}

void sctk_use_pthread_ng(void)
{
	mpc_common_get_flags()->thread_library_kind = "pthread_ng";
	mpc_common_get_flags()->thread_library_init = mpc_thread_pthread_ng_engine_init;
}

static void __arg_set_verbosity(char *arg)
{
	int tmp = atoi(arg);

	if( (0 <= tmp) && (mpc_common_get_flags()->verbosity < tmp) )
	{
		mpc_common_get_flags()->verbosity = tmp;
	}
}

/****************************
* ARGUMENT PARSING HELPERS *
****************************/

static inline int __parse_arg(char *arg, void (*action)(void), char *passed_arg)
{
	if(strncmp(arg, passed_arg, strlen(arg) ) == 0)
	{
		action();
		return 0;
	}

	return 1;
}

char *___extract_argument_string_value(char *arg, char *passed_arg)
{
	size_t len = strlen(arg);

	if(!strncmp(passed_arg, arg, len) )
	{
		if( (len + 1) < strlen(passed_arg) )
		{
			return passed_arg + len + 1; /*skip = */
		}
	}

	return NULL;
}

static inline int __parse_arg_eq(char *arg,
                                 void (*action)(char *),
                                 char *passed_arg)
{
	char *value = ___extract_argument_string_value(arg, passed_arg);

	if(value)
	{
		action(value);
		return 0;
	}

	return 1;
}

/* Note these macro are only intended for the __parse_argument
 * function as they depend on the passed_arg parameter */

/* Booleans */

#define __SET_FLAG_BOOLEAN(arg, flag, value)                  \
	do                                                    \
	{                                                     \
		if(!strcmp(arg, passed_arg) )                 \
		{                                             \
			mpc_common_get_flags()->flag = value; \
			return;                               \
		}                                             \
	} while(0)

#define SET_FLAG_BOOLEAN(key, flag)                     \
	__SET_FLAG_BOOLEAN( ("--"key), flag, 1);        \
	__SET_FLAG_BOOLEAN( ("--enable-"key), flag, 1); \
	__SET_FLAG_BOOLEAN( ("--disable-"key), flag, 1);

/* Strings / Integers */

#define __SET_FLAG_CONV(arg, flag, converter)                                                   \
	do                                                                                      \
	{                                                                                       \
		{                                                                               \
			char *__value_flag = ___extract_argument_string_value(arg, passed_arg); \
			if(__value_flag)                                                        \
			{                                                                       \
				mpc_common_get_flags()->flag = converter(__value_flag);         \
				return;                                                         \
			}                                                                       \
		}                                                                               \
	} while(0)

#define SET_FLAG_STRING(arg, flag)                  \
	do {                                        \
		__SET_FLAG_CONV(arg, flag, strdup); \
	} while(0)

#define SET_FLAG_INT(arg, flag)                   \
	do {                                      \
		__SET_FLAG_CONV(arg, flag, atoi); \
	} while(0)

/* Function based handling */

#define PARSE_ARG(arg, action)                        \
	if(__parse_arg(arg, action, passed_arg) == 0) \
		return
#define PARSE_ARG_WITH_EQ(arg, action)                   \
	if(__parse_arg_eq(arg, action, passed_arg) == 0) \
		return

static inline void __parse_argument(char *passed_arg)
{
	/* Boolean flags */
	SET_FLAG_BOOLEAN("smt", enable_smt_capabilities);
	SET_FLAG_BOOLEAN("graphic-placement", enable_topology_graphic_placement);
	SET_FLAG_BOOLEAN("text-placement", enable_topology_text_placement);
	SET_FLAG_BOOLEAN("checkpoint", checkpoint_enabled);
	/* Old syntax could be removed */
	__SET_FLAG_BOOLEAN("--use-accl", enable_accelerators, 1);
	SET_FLAG_BOOLEAN("accl", enable_accelerators);

	/* String flags */
	SET_FLAG_STRING("--sctk_use_network", network_driver_name);
	SET_FLAG_STRING("--profiling", profiler_outputs);
	SET_FLAG_STRING("--launcher", launcher);

	/* Int flags */
	SET_FLAG_INT("--node-number", node_number);
	SET_FLAG_INT("--process-number", process_number);
	SET_FLAG_INT("--task-number", task_number);
	SET_FLAG_INT("--processor-number", processor_number);

	PARSE_ARG_WITH_EQ("--mpc-verbose", __arg_set_verbosity);

	/* Thread engines */
	PARSE_ARG("--use-pthread_ng", sctk_use_pthread_ng);
	PARSE_ARG("--use-pthread", sctk_use_pthread);
	PARSE_ARG("--use-ethread_mxn_ng", sctk_use_ethread_mxn_ng);
	PARSE_ARG("--use-ethread_mxn", sctk_use_ethread_mxn);
	PARSE_ARG("--use-ethread_ng", sctk_use_ethread_ng);
	PARSE_ARG("--use-ethread", sctk_use_ethread);

	sctk_warning("Argument %s Unknown\n", passed_arg);
}

/*********************
* VP START FUNCTION *
*********************/

typedef struct
{
	int    argc;
	char **argv;
	char **saved_argv;
} startup_arg_t;

static inline startup_arg_t *__startup_arg_extract_and_duplicate(startup_arg_t *input_args)
{
	startup_arg_t *ret = sctk_malloc(sizeof(startup_arg_t) );

	assume(ret);
	ret->argc = input_args->argc;

	/* We create this extra argv array
	 * to prevent the case where a (strange)
	 * program modifies the argv[i] pointers
	 * preventing them to be freed at exit */
	ret->saved_argv = (char **)sctk_malloc(ret->argc * sizeof(char *) );
	assume(ret->saved_argv);
	int i;

	for(i = 0; i < ret->argc; i++)
	{
		ret->saved_argv[i] = input_args->argv[i];
	}

	ret->argv = (char **)sctk_malloc(ret->argc * sizeof(char *) );
	assume(ret->argv);

	for(i = 0; i < ret->argc; i++)
	{
		int   j;
		int   k;
		char *tmp;
		tmp = (char *)sctk_malloc( (strlen(input_args->argv[i]) + 1) * sizeof(char) );
		assume(tmp != NULL);
		j = 0;
		k = 0;

		while(input_args->argv[i][j] != '\0')
		{
			if(memcmp(&(input_args->argv[i][j]), "@MPC_LINK_ARGS@", strlen("@MPC_LINK_ARGS@") ) != 0)
			{
				tmp[k] = input_args->argv[i][j];
				j++;
				k++;
			}
			else
			{
				j     += strlen("@MPC_LINK_ARGS@");
				tmp[k] = ' ';
				k++;
			}
		}

		tmp[k]       = input_args->argv[i][j];
		ret->argv[i] = tmp;

		/* Here we store the pointer to
		 * be able to free it later on
		 * even in case of modification */
		ret->saved_argv[i] = tmp;
	}

	ret->argv[ret->argc] = NULL;
	return ret;
}

static inline void __startup_arg_free(startup_arg_t *arg)
{
	int i;

	for(i = 0; i < arg->argc; i++)
	{
		/* Here we free using the copy
		 * of the array to be sure we have
		 * a valid pointer */
		sctk_free(arg->saved_argv[i]);
	}

	sctk_free(arg->argv);
	sctk_free(arg->saved_argv);
	sctk_free(arg);
}

/* This variable is the return code for MPC's main
 * it is initialized in mpc_launch_main */
static OPA_int_t __mpc_main_return_code;
#define MPC_INCOHERENT_RETCODE    42

static void *__mpc_mpi_task_start_function(void *parg)
{
	startup_arg_t *arg = (startup_arg_t *)parg;

	int            retcode;
	startup_arg_t *duplicate_args = __startup_arg_extract_and_duplicate(arg);

	/* In libmode there is no main */
#ifndef SCTK_LIB_MODE
	mpc_common_init_trigger("Starting Main");

	retcode = CALL_MAIN(mpc_user_main__, duplicate_args->argc, duplicate_args->argv);

	/* We need to handle the case when MPC's mains do not return the
	 * same value we then apply the following rules:
	 *
	 * - All 0 => OK
	 * - All != 0 and same value return value
	 * - All/Some != 0 return MPC err code 42 (MPC_INCOHERENT_RETCODE) and warnings for each retcode
	 */

	/* Firs check if retcodes are not already incoherent */
	if(OPA_load_int(&__mpc_main_return_code) == MPC_INCOHERENT_RETCODE)
	{
		sctk_warning("main returned %d", retcode);
	}
	else
	{
		int previous_val = OPA_swap_int(&__mpc_main_return_code, retcode);

		/* if -1 I'm the firt to return all ok */
		if(previous_val != -1)
		{
			/* Check if previous val is not different if so enter incoherent mode */
			if(previous_val != retcode)
			{
				/* Set retcodes as incoherent */
				OPA_swap_int(&__mpc_main_return_code, MPC_INCOHERENT_RETCODE);
				sctk_warning("main returned %d and a previous main returned %d", retcode, previous_val);
				sctk_warning("Retcodes are incoherent for local mains, returning 42");
			}
		}
	}

	mpc_common_init_trigger("Ending Main");
#endif  /* SCTK_LIB_MODE */
	__startup_arg_free(duplicate_args);
	return NULL;
}

static void *___auto_kill_func(void *arg)
{
	int timeout = *(int *)arg;

	if(timeout > 0)
	{
		if(sctk_runtime_config_get()->modules.launcher.banner &&
		   !mpc_common_get_flags()->is_fortran)
		{
			mpc_common_io_noalloc_fprintf(stderr, "Autokill in %ds\n", timeout);
		}

		sleep(timeout);

		if(!mpc_common_get_flags()->is_fortran)
		{
			mpc_common_io_noalloc_fprintf(stderr, "TIMEOUT reached\n");
		}

		abort();
	}

	return NULL;
}

static void __create_autokill_thread()
{
	static int auto_kill = 0;

	auto_kill = sctk_runtime_config_get()->modules.launcher.autokill;

	if(auto_kill > 0)
	{
		pthread_t pid;
		pthread_create(&pid, NULL, ___auto_kill_func, &auto_kill);
	}
}

static void __set_default_values()
{
	mpc_common_get_flags()->sctk_network_description_string = "No networking";
	mpc_common_get_flags()->checkpoint_model = "No C/R";

#ifdef SCTK_LIB_MODE
	/* In lib mode we force the pthread MODE */
	sctk_use_pthread();

	/* In LIB mode comm size is inherited from
	 * the host application (not from the launcher) */
	mpc_common_get_flags()->process_number = mpc_lowcomm_hook_size();
	mpc_common_get_flags()->task_number    = mpc_lowcomm_hook_size();
#else
	sctk_use_ethread_mxn();

	// Initializing multithreading mode
	if(sctk_runtime_config_get()->modules.launcher.thread_init.value)
	{
		sctk_runtime_config_get()->modules.launcher.thread_init.value();
	}

	mpc_common_get_flags()->task_number    = sctk_runtime_config_get()->modules.launcher.nb_task;
	mpc_common_get_flags()->process_number = sctk_runtime_config_get()->modules.launcher.nb_process;
#endif
	mpc_common_get_flags()->processor_number        = sctk_runtime_config_get()->modules.launcher.nb_processor;
	mpc_common_get_flags()->verbosity               = sctk_runtime_config_get()->modules.launcher.verbosity;
	mpc_common_get_flags()->launcher                = sctk_runtime_config_get()->modules.launcher.launcher;
	mpc_common_get_flags()->profiler_outputs        = sctk_runtime_config_get()->modules.launcher.profiling;
	mpc_common_get_flags()->enable_smt_capabilities = sctk_runtime_config_get()->modules.launcher.enable_smt;
	mpc_common_get_flags()->checkpoint_enabled      = sctk_runtime_config_get()->modules.launcher.checkpoint;
	mpc_common_get_flags()->enable_accelerators     = sctk_runtime_config_get()->modules.accelerator.enabled;
	/* force smt on MIC */
#ifdef __MIC__
	mpc_common_get_flags()->enable_smt_capabilities = 1;
#endif
}

#define LEGACY_MPCRUN_ARGUMENT_START    "--sctk-args--"
#define LEGACY_MPCRUN_ARGUMENT_END      "--sctk-args-end--"

static void __unpack_arguments()
{
	char *sctk_argument = getenv("MPC_STARTUP_ARGS");

	if(sctk_argument != NULL)
	{
		char *str1, *token;
		char *saveptr;
		int   j;

		for(j = 1, str1 = sctk_argument;; j++, str1 = NULL)
		{
			token = strtok_r(str1, " ", &saveptr);

			if(token == NULL)
			{
				break;
			}

			/* Ignore legacy argument start and end */
			if(!strcmp(token, LEGACY_MPCRUN_ARGUMENT_START) ||
			   !strcmp(token, LEGACY_MPCRUN_ARGUMENT_END) )
			{
				continue;
			}

			__parse_argument(token);
			mpc_common_debug_info("mpcrun argument %d: %s", j, token);
		}
	}
}

static void __set_mpc_common_process_number(void)
{
	if(!mpc_common_get_flags()->task_number)
	{
		fprintf(stderr, "No task number specified!\n");
		sctk_abort();
	}

	if(mpc_common_get_flags()->process_number && mpc_common_get_flags()->task_number)
	{
		if(mpc_common_get_flags()->task_number < mpc_common_get_flags()->process_number)
		{
			bad_parameter("MPI process number (-n=%d) must be at least the process number (-p=%d)",
			              mpc_common_get_flags()->task_number,
			              mpc_common_get_flags()->process_number);
		}
	}

	if(mpc_common_get_flags()->process_number > 1)
	{
		mpc_common_set_process_count(mpc_common_get_flags()->process_number);
	}
}

static void __topology_init()
{
	mpc_topology_init();

	if(mpc_common_get_flags()->processor_number > 1)
	{
		if(mpc_common_get_flags()->process_number > 1)
		{
			unsigned int cpu_detected = mpc_topology_get_pu_count();

			if(cpu_detected < mpc_common_get_flags()->processor_number)
			{
				fprintf(stderr,
				        "Processor number doesn't match number detected %d <> %d!\n",
				        cpu_detected, mpc_common_get_flags()->processor_number);
			}
		}
	}
}

void mpc_launch_init_runtime()
{
	static int __init_already_done = 0;

	if(__init_already_done)
	{
		return;
	}

	__init_already_done = 1;

	/* Start computing TSC frequency to avoid usleep in
	 * the first waitall */
	mpc_arch_tsc_freq_compute_start();

	mpc_common_init_trigger("Base Runtime Init");

	/* WARNING !! NO CONFIG before this point */

	if(sctk_runtime_config_get()->modules.debugger.mpc_bt_sig)
	{
		mpc_common_debugger_install_sig_handlers();
	}

	__create_autokill_thread();
	__set_default_values();
	__unpack_arguments();
	__set_mpc_common_process_number();

	/* As a first step initialize the PMI */
	mpc_launch_pmi_init();

	__topology_init();


	sctk_init_alloc();

	mpc_common_init_trigger("Base Runtime Init with Config");

	mpc_common_init_print();
	mpc_common_init_trigger("Base Runtime Init Done");
	mpc_launch_print_banner(0 /* not in restart mode */);
}

int mpc_launch_release_runtime()
{
	mpc_common_init_trigger("Base Runtime Finalize");
	mpc_topology_destroy();
	return 0;
}

#ifndef SCTK_LIB_MODE

int mpc_launch_main(int argc, char **argv)
{
	/* Initialize return code to -1 (as not set) */
	OPA_store_int(&__mpc_main_return_code, -1);

	mpc_launch_init_runtime();

	mpc_common_get_flags()->exename = argv[0];

	startup_arg_t arg;
	arg.argc = argc;
	arg.argv = argv;

	mpc_thread_spawn_mpi_tasks(__mpc_mpi_task_start_function, &arg);

	mpc_launch_release_runtime();

	return OPA_load_int(&__mpc_main_return_code);
}
#endif /* SCTK_LIB_MODE */
