#define _GNU_SOURCE    1
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

#include "mpc_config.h"
#include "mpc_common_debug.h"
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <mpc_launch.h>
#include <mpc_keywords.h>
#include <mpc_common_flags.h>
#include <mpc_common.h>

#if defined(WINDOWS_SYS)
#include <pthread.h>
#endif

#include "main.h"

/***********************
* COMMON MAIN WRAPPER *
***********************/

static int __main_wrapper(int argc, char **argv)
{
	int bypass_mpc_launch_main = 0;

	/* If MPC has no main wrapping
	 * directly call the original main */
    #ifndef MPC_Threads
	bypass_mpc_launch_main |= 1;
    #endif

	char *mpi_lib = getenv("TRUE_MPI_LIB");

	/* WI4MPI hack: avoid MPC initialisation when MPC is not the target MPI */
	if(mpi_lib && !strstr(mpi_lib, "libmpc_framework") )
	{
		bypass_mpc_launch_main |= 1;
	}

	/* Here we bypass all the MPC setup and call
	 * the renamed main directly using env variable */
	if(getenv("MPC_CALL_ORIGINAL_MAIN") )
	{
		bypass_mpc_launch_main |= 1;
	}

	if(bypass_mpc_launch_main)
	{
		return CALL_MAIN(mpc_user_main__, argc, argv);
	}

#if defined(WINDOWS_SYS)
	pthread_win32_process_attach_np();
#endif

	int tmp = mpc_launch_main(argc, argv);

#if defined(WINDOWS_SYS)
	pthread_win32_process_detach_np();
#endif

	return tmp;
}

/************************
* FORTRAN ENTRY-POINTS *
************************/

void mpc_start_(void)
{
	char *argv[2];

	argv[0] = "unknown";
	argv[1] = NULL;
	mpc_common_get_flags()->is_fortran = 1;
	__main_wrapper(1, argv);
	exit(0);
}

void mpc_start__(void)
{
	mpc_start_();
}

/*********************************************
* MPC'S REPLACEMENT MAIN (MAIN ENTRY POINT) *
*********************************************/


#if !defined(MPC_Threads)
/* Handle the case where the main is not being rewritten
   and provide the symbol to handle it */

	#ifdef HAVE_ENVIRON_VAR
		/**
		 * @brief Definition of the user-main
		 */
		int mpc_user_main__ (__UNUSED__ int argc, __UNUSED__ char **argv, __UNUSED__  char**envp)
		{
			not_reachable();
		}
	#else
		int mpc_user_main__ (__UNUSED__ int argc, __UNUSED__ char **argv)
		{
			not_reachable();
		}
	#endif

#else
	/* This is the replacement main */

	int main(int argc, char **argv)
	{
		mpc_common_get_flags()->is_fortran = 0;
		return __main_wrapper(argc, argv);
	}
#endif

/******************************
* LIBGFORTRAN INITIALIZATION *
******************************/

static void __search_and_call_symbol(char *sym)
{
	void ( *ptr_func )(void) = dlsym(RTLD_DEFAULT, sym);

	if(ptr_func)
	{
		ptr_func();
	}
}

/* Set up the modified libgfortran if needed */
static void __libgfortran_init()
{
	if(mpc_common_get_flags()->is_fortran)
	{
		/* init function for the modified libgfortran */
		__search_and_call_symbol("_gfortran_ap_init_units");
	}
}

/* Clear the modified libgfortran if needed */
static void __libgfortran_close()
{
	if(mpc_common_get_flags()->is_fortran)
	{
		/* init function for the modified libgfortran */
		__search_and_call_symbol("_gfortran_ap_close_units");
	}
}

void _mpc_common_debug_init() __attribute__( (constructor) );

void _mpc_common_debug_init()
{
    MPC_INIT_CALL_ONLY_ONCE

    mpc_common_init_callback_register("Config Sources", "MPC Lowcomm Config Registration", mpc_common_debug_init, 128);

}

void mpc_main_init() __attribute__( (constructor) );

void mpc_main_init()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("Starting Main", "Initialize libgfortran", __libgfortran_init, 16);

	mpc_common_init_callback_register("Ending Main", "Release libgfortran", __libgfortran_close, 16);
}
