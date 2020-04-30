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
#   - CARRIBAULT Patrick patrick.carribault@cea.fr                     #
#   - PERACHE Marc marc.perache@cea.fr                                 #
#   - JAEGER Julien julien.jaeger@cea.fr                               #
#                                                                      #
########################################################################

#
# You may run this through "make lint"
#

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")


BUILDIR=$1

if test "x$#" != "x1"; then
	echo "Usage: $0 [BUILD DIR]"
	exit 1
fi

command -v clang-tidy

if test "x$?" != "x0"; then
	echo "Linting requires clang-tidy"
	exit 1
fi

clean()
{
	echo "Cleaning UP"
	rm -fr "$TMP"
	exit 1
}


trap 'clean' INT


TMP=$(mktemp -d)

for i in $(find "${SCRIPTPATH}/../src/" -iname "*.[ch]") $(find "$1" -iname "*.[ch]")
do
	BN=$(basename "$i")
	if test ! -f  "$TMP/${BN}"; then
		ln -s "$i" "$TMP/${BN}"
	fi
done

. "$BUILDIR/src/bin/mpc_build_env.sh"


for i in $(find "${SCRIPTPATH}/../src/" -iname "*.[ch]")
do
	clang-tidy "$i" -- -I "$TMP" $(pkg-config --cflags mpcalloc || true)
done


clean
