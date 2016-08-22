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
#include "sctk_launch.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_ENVIRON_VAR
#include <stdlib.h>
#include <stdio.h>
extern char ** environ;
#endif

/* No need for a main in LIB mode */
#ifndef SCTK_LIB_MODE

#if defined(MPC_Message_Passing) || defined(MPC_Threads)
#include "mpc.h"

#if defined(WINDOWS_SYS)
#include <pthread.h>
#endif


#include <dlfcn.h>

static void run_libgfortran_symbol(char* sym)
{
    void (*ptr_func)(void) = NULL; 
    static void *handle = NULL;
      handle = dlopen("libgfortran.so",RTLD_LAZY);
      if (handle == NULL)
      {
        return;
      }
      ptr_func = dlsym(handle,sym);
      if (ptr_func == NULL)
      {
        return;
      }
    ptr_func();
    dlclose(handle);
}

/* Set up the modified libgfortran if needed */
static void local_libgfortran_init()
{
    /* init function for the modified libgfortran */
    if (sctk_is_in_fortran == 1)
    {
      run_libgfortran_symbol("_gfortran_init_units");
    }
}
/* Clear the modified libgfortran if needed */
static void local_libgfortran_close()
{
    /* init function for the modified libgfortran */
    if (sctk_is_in_fortran == 1)
    {
        run_libgfortran_symbol("_gfortran_close_units");
    }
}

static int
intern_main (int argc, char **argv)
{
  int tmp;

  /* WI4MPI hack: avoid MPC initialisation when MPC is not the target MPI */
  char * mpi_lib = getenv("TRUE_MPI_LIB");
  if(mpi_lib && ! strstr(mpi_lib, "libmpc_framework") )
  { 
	sctk_info("MPC-WI4MPI: Not being the target MPI: Disabling the runtime");
	sctk_use_pthread();
#ifdef HAVE_ENVIRON_VAR
  	return mpc_user_main__(argc, argv, environ);
#else
  	return mpc_user_main__(argc, argv);
#endif
  }

#if defined(WINDOWS_SYS)
  pthread_win32_process_attach_np ();
#endif
  tmp = sctk_launch_main (argc, argv);
#if defined(WINDOWS_SYS)
  pthread_win32_process_detach_np ();
#endif
  return tmp;
}

#ifdef HAVE_ENVIRON_VAR
int
mpc_user_main (int argc, char **argv,char** envp)
{
  int res = 0;
  /* Set up the modified libgfortran if needed */
  local_libgfortran_init();
  res = mpc_user_main__ (argc, argv,envp);
  /* Clear up the modified libgfortran if needed */
  local_libgfortran_close();
  return res;
}
#else
int
mpc_user_main (int argc, char **argv)
{
  int res = 0;
  /* Set up the modified libgfortran if needed */
  local_libgfortran_init();
  res = mpc_user_main__ (argc, argv);
  /* Clear up the modified libgfortran if needed */
  local_libgfortran_close();
  return res;
}
#endif

int
main_c (int argc, char **argv)
{
  int tmp;
  sctk_is_in_fortran = 0;
  tmp = intern_main (argc, argv);
  return tmp;
}

int
main_fortran (int argc, char **argv)
{
  sctk_is_in_fortran = 1;
  intern_main (argc, argv);
  exit (0);
  return 0;
}
#else
int
main_fortran (int argc, char **argv)
{
  int tmp;
#ifdef HAVE_ENVIRON_VAR
  mpc_user_main__ (argc, argv,environ);
#else
  mpc_user_main__ (argc, argv);
#endif
  exit (0);
  return 0;
}
#endif

void
mpc_start_ (void)
{
  char *argv[2];
  argv[0] = "unknown";
  argv[1] = NULL;
  main_fortran (1, argv);
}

void
mpc_start__ (void)
{
  char *argv[2];
  argv[0] = "unknown";
  argv[1] = NULL;
  main_fortran (1, argv);
}

#endif /* SCTK_LIB_MODE */
