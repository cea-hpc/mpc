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

# Common variables

set -e
MPC_CFLAGS="mpc_cflags"

CC=""
FC=""

NOPRIV="no"

FORCE_INSTALL_LEVEL="no"


for arg in "$@"
do
	case $arg in
	-cc=*)
		CC=$(echo "A$arg" | sed -e 's/^A-cc=//g')
		CC=$(command -v "${CC}" || echo "${CC}")
		;;
	-fc=*)
		FC=$(echo "A$arg" | sed -e 's/^A-fc=//g')
		FC=$(command -v "${FC}" || echo "${FC}")
		;;
	-cf=*)
		MPC_CFLAGS=$(echo "A$arg" | sed -e 's/^A-cf=//g')
		MPC_CFLAGS=$(command -v "${MPC_CFLAGS}" || echo "${MPC_CFLAGS}")
		;;
	--nopriv)
		NOPRIV="yes"
		;;
	--check)
		CHECK="yes"
		;;
	--use-install)
		FORCE_INSTALL_LEVEL="yes"
		;;
	*)
		CFLAGS="${CFLAGS} ${arg}"
		;;
	esac
done


#Extract compiler configuration and flags
# - MPC_INSTALL_PREFIX
# - MPC_SHARE_DIR
# - MPC_DEFAULT_FC_COMPILER
# - MPC_DEFAULT_C_COMPILER
# - MPC_GENERATION_DATE
# - CFLAGS
eval $("$MPC_CFLAGS" -sh -p -s -fc -cc -f -t)

test -z "$CC" && CC="$MPC_DEFAULT_C_COMPILER"
test -z "$FC" && FC="$MPC_DEFAULT_FC_COMPILER"

# shellcheck source=/dev/null
. "${MPC_SHARE_DIR}/mpc_compiler_common.sh"

if test "x${NOPRIV}" = "xno"; then
	append_to "CFLAGS" "-fmpc-privatize"
fi

fortran_config_hash()
{
	FCPATH=$(command -v ${FC})
	FCPATH=$(readlink -e ${FCPATH})
	CPATH=$(command -v ${CC})
	CPATH=$(readlink -e ${CPATH})
	CHAIN="${CPATH}${FCPATH}${NOPRIV}${MPC_INSTALL_PREFIX}"
	echo "$CHAIN" | md5sum | cut -d " " -f 1
}

create_mpc_fortran_user_home()
{
	test -d "${HOME}/.mpc/" || mkdir "${HOME}/.mpc/"
	test -d "${HOME}/.mpc/fortran/" || mkdir "${HOME}/.mpc/fortran/"
	test -d "${HOME}/.mpc/fortran/$(fortran_config_hash)" || mkdir "${HOME}/.mpc/fortran/$(fortran_config_hash)"
}

create_mpc_fortran_install_home()
{
	test -d "${MPC_INSTALL_PREFIX}/lib/fortran/" || mkdir "${MPC_INSTALL_PREFIX}/lib/fortran/"
	test -d "${MPC_INSTALL_PREFIX}/lib/fortran/$(fortran_config_hash)" || mkdir "${MPC_INSTALL_PREFIX}/lib/fortran/$(fortran_config_hash)"
}

fortran_install_path()
{
	HASH=$(fortran_config_hash)
	echo "${MPC_INSTALL_PREFIX}/lib/fortran/${HASH}"
}

fortran_user_path()
{
	HASH=$(fortran_config_hash)
	echo "${HOME}/.mpc/fortran/${HASH}"
}

fortran_modules_found()
{
	
	for FORTRAN_HOME in $(fortran_install_path) $(fortran_user_path)
	do

		if test -d "$FORTRAN_HOME" -a -f "$FORTRAN_HOME/.ts"; then
			if test "x$(cat "$FORTRAN_HOME/.ts")" = "x${MPC_GENERATION_DATE}"; then
				return
			else
				info "# Outdated fortran modules in ${FORTRAN_HOME}"
				if test -w "${FORTRAN_HOME}" -a -d "${FORTRAN_HOME}"; then
					rm -fr "${FORTRAN_HOME}"
				fi
			fi
		fi
	done

	FORTRAN_HOME=""
}

TEMPDIR=""

clean_tempdir()
{
	test -d "$TEMPDIR" && rm -fr "$TEMPDIR"
}

build_fortran_modules()
{
	fortran_modules_found

	if test -n "${FORTRAN_HOME}"; then
		return
	fi

	INSTALL_HOME="$(fortran_install_path)"
	USER_HOME="$(fortran_user_path)"

	if test "x${FORCE_INSTALL_LEVEL}" = "xyes"; then
		if test -w "${MPC_INSTALL_PREFIX}/lib/"; then
			create_mpc_fortran_install_home
			FORTRAN_HOME=${INSTALL_HOME}
		else
			die "${INSTALL_HOME} is not writable cannot install modules there"
		fi
	else
		create_mpc_fortran_user_home
		FORTRAN_HOME="${USER_HOME}"
	fi

	TEMPDIR=$(mktemp -d)
	trap clean_tempdir EXIT
	cp ${MPC_SHARE_DIR}/fortran/* "${TEMPDIR}"

	info "# Building Fortran module for $(basename ${FC})..."
	make -C "${TEMPDIR}" CC="${CC}" FC="${FC}" CFLAGS="${CFLAGS}" > "$TEMPDIR/out.log" 2>&1 || true

	if test -f ${TEMPDIR}/libmpcfortran.so -a ${TEMPDIR}/mpif.h; then
		create_mpc_fortran_user_home
		cp "${TEMPDIR}/libmpcfortran.so" "${FORTRAN_HOME}"
		cp "${TEMPDIR}/mpif.h" "${FORTRAN_HOME}"
		# Save generation date for comparison (later rebuild)
		echo "$MPC_GENERATION_DATE" > "${FORTRAN_HOME}/.ts"
	else
		cat "$TEMPDIR/out.log" >&2
		die "Could not compile fortran modules for this compiler"
	fi

}

if test "x${CHECK}" = "xyes"; then
	fortran_modules_found
else
	build_fortran_modules
fi

echo ${FORTRAN_HOME}