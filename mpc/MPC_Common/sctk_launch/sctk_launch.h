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
#ifndef __SCTK_LAUNCH_H_
#define __SCTK_LAUNCH_H_

#include "sctk_config.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C"
{
#endif

  int sctk_env_init (int *argc, char ***argv);
  int sctk_initialisation (char *args, int *argc, char ***argv);
  int sctk_env_exit (void);
  int sctk_launch_main (int argc, char **argv);
  void sctk_launch_contribution (FILE * file);

  int sctk_user_main (int argc, char **argv);
  int mpc_user_main (int, char **);

  int sctk_get_process_nb (void);
  int sctk_get_processor_nb (void);
  char* sctk_get_launcher_mode(void);

  void sctk_set_net_val (void (*val) (int *, char ***));

  extern char *sctk_mono_bin;
  extern int sctk_disable_smt_capabilities;
  extern int sctk_share_node_capabilities;

  void mpc_start_ (void);
  void mpc_start__ (void);

  extern int mpc_user_main__ (int, char **);

  /*  return the number of tasks involved */
  int sctk_get_total_tasks_number();

#if defined(MPC_Message_Passing) || defined(MPC_Threads)
  int main_c (int argc, char **argv);
  int main_fortran (int argc, char **argv);
#endif

#ifdef __cplusplus
}
#endif
#endif
