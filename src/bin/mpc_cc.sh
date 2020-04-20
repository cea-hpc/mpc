#!/bin/sh
############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - PERACHE Marc marc.perache@cea.fr                                 #
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#                                                                      #
########################################################################

MPC_INSTALL_PREFIX=$(mpc_cflags -p)
MPC_SHARE_DIR=$("${MPC_INSTALL_PREFIX}/bin/mpc_cflags" -s)

# Source Common function library

# shellcheck source=/dev/null
. "${MPC_SHARE_DIR}/mpc_compiler_common.sh"

# Common variables
COMPILER=$("${MPC_INSTALL_PREFIX}/bin/mpc_cflags" -cc)

#first definition of compiler & linking flags
CFLAGS=$("${MPC_INSTALL_PREFIX}/bin/mpc_cflags")
LDFLAGS=$("${MPC_INSTALL_PREFIX}/bin/mpc_ldflags")

parse_cli_args $@

# Here we restore MPC's build environment
. $MPC_INSTALL_PREFIX/bin/mpc_build_env.sh

run_compiler