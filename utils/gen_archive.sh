#!/bin/sh

set -x
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


SCRIPTPATH=$(dirname "$(readlink -f "$0")")


##################### FUNCTION ###################
safe_exec()
{
	echo "$*"
	$* || exit 1
}
##################### FUNCTION ###################
do_check()
{
	if test -d "mpcframework-${VERSION}"; then safe_exec rm -rf "mpcframework-${VERSION}"; fi
	safe_exec tar -xzf "${FILE}"
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

if test -d ~/.mpcdep; then
	echo "Please temporarily disable your ~/.mpcdep cache to build the tarballs"
	exit 1
fi

############### ARGUMENT PARSING #########################

VERSION=""
ADD_DEP="yes"
LIGHT_FLAG=""

CHECKSUM="no"
DOCHECK="no"

help()
{
cat << EOF
Generate an archive of the MPC repository.

It must be called from MPC's root directory.

$0 [--version=x.x.x] [--light] [--no-checksum] [--check]

This script supports the following arguments:

	--version=X         Force mpc version (instead of reading in git)
	--light             Do not download tarballs
	--checksum          Generate digest files
	--check             Compile and run simple tests on the tarball
	--help              Show this help
EOF
exit 0
}


for arg in "$@"
do
	case "$arg" in
		################# HYDRA #################
		--version=*)
			VERSION=$(echo "$arg" | sed -e "s/^--version=//g")
			;;
		--checksum)
			CHECKSUM="yes"
			;;
		--check)
			DOCHECK="yes"
			;;
		--light)
			ADD_DEP="no"
			LIGHT_FLAG="-light"
			;;
		--help|-h)
			help
			;;
		*)
			echo "Invalid argument '$arg', please check your command line or get help with --help." 1>&2
			exit 1
	esac
done


if test -z "${VERSION}"; then
	VERSION=$(./utils/get_version)
fi




################ Archive GENERATION ########################

FILE="${PWD}/mpcframework-${VERSION}${LIGHT_FLAG}.tar.gz"

echo "MPC version for this archive will be $VERSION"

test -f "${FILE}" && echo "A file named ${FILE} already exist!" && exit 1

echo "git archive --format=tar.gz --prefix=mpcframework-${VERSION}${LIGHT_FLAG}/ HEAD > ${FILE}"
git archive --format=tar.gz --prefix="mpcframework-${VERSION}${LIGHT_FLAG}/" HEAD > "${FILE}" || exit 1

if test "x${ADD_DEP}" = "xyes"; then
	# Now enrich archive with deps
	TMPDIR=$(mktemp -d)
	cd "${TMPDIR}" || exit 42

	safe_exec tar xf "${FILE}"
	mkdir "./mpcframework-${VERSION}/deps/" || exit 42
	cd "./mpcframework-${VERSION}/deps/" || exit 42

	echo "Downloading dependencies.."
	safe_exec "${SCRIPTPATH}/../installmpc" --download "$@"

	echo "Inserting dependencies ..."
	cd "${TMPDIR}" || exit 42

	safe_exec tar czf "${FILE}" "./mpcframework-${VERSION}/"

	rm -fr "${TMPDIR}"
fi

if test "x${CHECKSUM}" = "xyes"; then
	sha512sum "${FILE}" > "${FILE}.digest"
	sha256sum "${FILE}" >> "${FILE}.digest"
	md5sum "${FILE}" >> "${FILE}.digest"

	echo "Archive sum (./${FILE}.digest):"
	cat "${FILE}.digest"
fi

if test "x${DOCHECK}" = "xyes"; then
	do_check
fi

echo "${FILE}"

echo "Done."
