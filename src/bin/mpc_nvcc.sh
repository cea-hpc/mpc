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
CC=$("${MPC_INSTALL_PREFIX}/bin/mpc_cflags" -cpp)
COMPILER="nvcc --compiler-bindir $CC"

#first definition of compiler & linking flags
CFLAGS=$("${MPC_INSTALL_PREFIX}/bin/mpc_cflags")

LDFLAGS=$("${MPC_INSTALL_PREFIX}/bin/mpc_ldflags")

parse_cli_args $@

# Update compiler in case CC was altered by CLI
COMPILER="nvcc --compiler-bindir $CC"

# Add forwaders for NVCC
LDFLAGS=$(echo "$LDFLAGS" | sed -e 's/\-Wl,/-Xlinker /g' -e 's/ -B/ -Xcompiler &/g')

# Here we restore MPC's build environment
. ${MPC_INSTALL_PREFIX}/bin/mpc_build_env.sh


#
# NVCC additionnal blacklisting
#

#Used by nvcc to automatically blacklist some var patterns
list_cuda_global_vars()
{
	CUDA_FILE="/tmp/cuda_tmp_main${RANDOM}.cu"

cat<< EOF_FILE > ${CUDA_FILE}
#include <stdlib.h>

__global__
void foo(void)
{
}

int main(int argc, char ** argv)
{
	return 0;
}
EOF_FILE

	IFS="
	"

	tmp=""
	for decl in `${COMPILER} -Xcompiler -fmpc-privatize ${CUDA_FILE} -Xcompiler -B${MPC_INSTALL_PREFIX}/bin 2>&1 | grep "MPC task" | sed -e 's/.*variable //g' -e 's/ in file.*//g'`
	do
		tmp="${tmp}:${decl}"
	done

	echo "${tmp}"
	rm ${CUDA_FILE}
}

compiler_is_privatizing "${1}"

if test "x${RES}" = "xyes"; then
	append_to "CFLAGS" "-fno-priv-var=$(list_cuda_global_vars)"
fi

run_compiler