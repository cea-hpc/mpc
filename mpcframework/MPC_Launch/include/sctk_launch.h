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
#ifndef __SCTK_LAUNCH_H_
#define __SCTK_LAUNCH_H_

#include "mpc_config.h"
#include "mpc_common_types.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C"
{
#endif
  void sctk_init_mpc_runtime();

  int sctk_env_init (int *argc, char ***argv);
  int sctk_initialisation (char *args, int *argc, char ***argv);
  int sctk_env_exit (void);
  int sctk_launch_main (int argc, char **argv);
  void sctk_launch_contribution (FILE * file);
  void mpc_launch_print_banner(bool);

#ifdef HAVE_ENVIRON_VAR
  int mpc_user_main (int, char **, char**);
#else
  int mpc_user_main (int, char **);
#endif

  char* sctk_get_launcher_mode(void);
  char *get_debug_mode();
  int sctk_get_node_nb();

  void sctk_set_net_val (void (*val) (int *, char ***));

  void mpc_start_ (void);
  void mpc_start__ (void);

#ifndef SCTK_LIB_MODE

	#ifdef HAVE_ENVIRON_VAR
	  extern int mpc_user_main__ (int, char **,char**);
	#else
	  extern int mpc_user_main__ (int, char **);
	#endif

	#if defined(MPC_Message_Passing) || defined(MPC_Threads)
	int main_c (int argc, char **argv);
	int main_fortran (int argc, char **argv);
	#endif

#endif /* SCTK_LIB_MODE */


void mpc_launch_init_runtime();

#ifdef __cplusplus
}
#endif
#endif
