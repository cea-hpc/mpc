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
#ifndef MPC_LAUNCH_H
#define MPC_LAUNCH_H

#include <mpc_config.h>


#ifdef __cplusplus
extern "C"
{
#endif

/**
* @brief Print MPC's Banner Only visible with -v
*
* @param restarting true if from checkpoint
*/
void mpc_launch_print_banner( bool restarting );

/**
* @brief This is the main entry point to initialize MPC
*
*/
void mpc_launch_init_runtime();

/**
* @brief Main wrapper for MPC launch (calling INIT/RELEASE)
*
* @param argc number of arguments
* @param argv Arguments
* @return int return value
*/
int mpc_launch_main( int argc, char **argv );

#ifdef __cplusplus
}
#endif

#endif /* MPC_LAUNCH_H */
