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

#if defined(MPC_Allocator)
#include "sctk_iso_alloc.h"
#endif

#if defined(MPC_Message_Passing) || defined(MPC_Threads)
#include "mpc.h"

#if defined(WINDOWS_SYS)
#include <pthread.h>
#endif

static int
intern_main (int argc, char **argv)
{
  int tmp;


#if defined(MPC_Allocator)
  sctk_iso_init ();
#endif


#if defined(WINDOWS_SYS)
  pthread_win32_process_attach_np ();
#endif
  tmp = sctk_launch_main (argc, argv);
#if defined(WINDOWS_SYS)
  pthread_win32_process_detach_np ();
#endif
  return tmp;
}

int
mpc_user_main (int argc, char **argv)
{
  return mpc_user_main__ (argc, argv);
}

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
  mpc_user_main__ (argc, argv);
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
