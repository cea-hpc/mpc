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

#if defined(MPC_Message_Passing) || defined(MPC_Threads)

/* We do not need a main
 * when running in lib mode */
#ifndef SCTK_LIB_MODE
int
main (int argc, char **argv)
{
  int tmp;

#ifdef SCTK_DEBUG_MESSAGES
  fprintf(stderr, "[node/proc/ vp /task/thrd/rank] DEBUG INFO HEADER\n");
#endif

  tmp = main_c (argc, argv);
  return tmp;
}
#endif /* SCTK_LIB_MODE */

#else
	#ifdef HAVE_ENVIRON_VAR
	  int mpc_user_main (int argc , char **argv, char**envp)
	  {
		  not_available ();
		  return 0;
	  }
	#else
	  int mpc_user_main (int argc, char ** argv)
	  {
		  not_available ();
		  return 0;
	  }
	#endif
#endif
