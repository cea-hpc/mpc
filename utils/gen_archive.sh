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

##################### FUNCTION ###################
safe_exec()
{
	echo "$*"
	$* || exit 1
}
##################### FUNCTION ###################
do_check()
{
	if test -d mpcframework-${VERSION}; then safe_exec rm -rf mpcframework-${VERSION}; fi
	safe_exec tar -xzf mpcframework-${VERSION}.tar.gz
	safe_exec cp -r ./MPC_Test_Suite ./mpcframework-${VERSION}/
	safe_exec cd mpcframework-${VERSION}/MPC_Test_Suite
	safe_exec rm -fr ./build/
	./mpc_validation --build --color 
	safe_exec ./mpc_validation --test --color --select=simple
}

##################### SECTION ####################
if test ! -f ./installmpc; then
	echo "Please run this script from the root directory of MPC" 1>&2
	exit 1
fi

##################### SECTION ####################
if test ! -e .git; then
	echo "This script is made to generate an archive from a Git repository !" 1>&2
	exit 1
fi

##################### SECTION ####################
VERSION=$(./utils/get_version)
echo "MPC version for this archive will be $VERSION"
test -f ./mpcframework-${VERSION}.tar.gz && echo "A file named mpcframework-${VERSION}.tar.gz already exist!" && exit 1

echo "git archive --format=tar.gz --prefix=mpcframework-${VERSION}/ HEAD > mpcframework-${VERSION}.tar.gz"
git archive --format=tar.gz --prefix=mpcframework-${VERSION}/ HEAD > mpcframework-${VERSION}.tar.gz || exit 1

echo "$(sha512sum mpcframework-${VERSION}.tar.gz)" > ./mpcframework-${VERSION}.tar.gz.digest
echo "$(sha256sum mpcframework-${VERSION}.tar.gz)" >> ./mpcframework-${VERSION}.tar.gz.digest
echo "$(md5sum mpcframework-${VERSION}.tar.gz)" >> ./mpcframework-${VERSION}.tar.gz.digest

echo "Archive sum (./mpcframework-${VERSION}.tar.gz.digest):"
cat ./mpcframework-${VERSION}.tar.gz.digest

if test "$1" = "--check"; then
	do_check
fi

echo "Done."
