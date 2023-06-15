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
# Path to the configure script
SCRIPT=$(readlink -f "$0")
# Directory containing configure
SCRIPTPATH=$(dirname "$SCRIPT")
# Where MPC is currently being BUILT
PROJECT_BUILD_DIR=$(readlink -f "$PWD")

# Move to script directory first.
cd $SCRIPTPATH

ret=0
err()
{
	1>&2 printf "Error: $@\n"
	ret=1
}

die()
{
	err "$1"
	exit 1
}

extract_version()
{
	"$1" --version | head -n 1 | grep -E -o "[0-9]+(\.[0-9]+)*"
}

spack_tips()
{
	echo "========================="
	echo "Consider running:"
	echo "$ spack install autoconf@${ac_expect} automake@${am_expect} libtool@${lt_expect}"
	echo "$ spack load autoconf@${ac_expect} automake@${am_expect} libtool@${lt_expect}"
	echo "========================="
}


arc=$(command -v autoreconf)
ac=$(command -v autoconf)
am=$(command -v automake)
lt=$(command -v libtoolize)
ptch=$(command -v patch)

test -n "$arc" || err "No 'autoreconf' command in PATH. Please install autoconf first."
test -n "$ac" || err "No 'autoconf' command in PATH. Please install autoconf first."
test -n "$am" || err "No 'automake' command in PATH. Please install automake first."
test -n "$lt" || err "No 'libtoolize' command in PATH. Please install libtool first."
test -n "$ptch" || err "No 'patch' command in PATH. Please install patch first."

if test "$ret" = "1"; then
	exit 1
fi

if test "$ret" = "0"; then
	$arc -vi
	ret=$?
else
	die "Abort configuration due to error(s) above.\n"
fi

#
# Patching Phase
#

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

#
# Check if a patch is already applied to a given file
# Args:
#  - $1 : Magic Pattern
#
# Returns:
#  - PATCHED = yes | no
#
check_for_patch()
{
	CNT=$(grep -c "$1" "${SCRIPTPATH}/configure")

	if test "x${CNT}" = "x0"; then
		PATCHED="no"
	else
		PATCHED="yes"
	fi
}

#
# Add a comment in the configure to notify that patch is done
#
# Args:
#   - $1 : watermark to be added
#
watermark_configure()
{
	echo "# MPC has applied patch $1" >> "${SCRIPTPATH}/configure"
}

#
# Parse patch watermark
#
# Args:
#  - $1: path to patch
#
# Returns:
#  - WATERMARK: what is to be used to mark patch done
#
parse_watermark()
{
	WATERMARK=$(basename "$1" | sed "s/\.patch//g" | cut -d "-" -f 2)

	if test -z "${WATERMARK}"; then
		die "Could not parse watermark in $1"
	fi
}

#
# Apply all patches located in config/conf_patches/
#
# THEY must follow these guidelines:
#   - Named as ORDER-MAGIC.patch
#      - ORDER is the order of application 000 to 999
#      - MAGIC is BOTH the patch description and patch watermark
apply_configuration_patches()
{
	for i in "${SCRIPTPATH}"/config/conf_patches/*.patch
	do
		parse_watermark "$i"
		check_for_patch "${WATERMARK}"

		if test "x${PATCHED}" = "xno"; then
			patch -p1 < "$i" || die "Could not apply ${WATERMARK} patch"
			watermark_configure "${WATERMARK}"
			echo "PATCH: ${WATERMARK} APPLIED"
		else
			echo "PATCH: ${WATERMARK} already APPLIED"
		fi

	done
}

apply_configuration_patches

# Here is the patch description


# 001-MPC_FORTRAN_FIX_LIB_WITH_SPACE
#
# Fix for Fortran libraries in libtools as reported in
#
# https://debbugs.gnu.org/cgi/bugreport.cgi?bug=10972
# https://debbugs.gnu.org/cgi/bugreport.cgi?bug=21137
# https://github.com/open-mpi/ompi/issues/576
#

# 002-MPC_FLANG_SUPPORT
#
# Allow libtool to know flang (taken from OpenMPI)
# facing the exact same issues
#
# https://github.com/open-mpi/ompi/commit/6f8010c685d6fb0554c80cde9f131996081f7105


#
# Mark the configure as GENERATED with ./autogen.sh (if needed)
#
IS_AUTOGEN=$(tail -n 1 "${SCRIPTPATH}/configure"| grep -c "MPC_AUTOGEN_WAS_USED")

if test "x${IS_AUTOGEN}" = "x0"; then
	echo "# MPC_AUTOGEN_WAS_USED" >> "${SCRIPTPATH}/configure"
fi

# Go back to build dir
cd $PROJECT_BUILD_DIR
