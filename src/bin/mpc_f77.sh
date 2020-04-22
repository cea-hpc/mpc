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

#
# General wrapper config
#

MPC_INSTALL_PREFIX=$(mpc_cflags -p)
MPC_SHARE_DIR=$("${MPC_INSTALL_PREFIX}/bin/mpc_cflags" -s)

# Source Common function library

# shellcheck source=/dev/null
. "${MPC_SHARE_DIR}/mpc_compiler_common.sh"

# Common variables
COMPILER=$("${MPC_INSTALL_PREFIX}/bin/mpc_cflags" -fc)

#first definition of compiler flags
CFLAGS=$("${MPC_INSTALL_PREFIX}/bin/mpc_cflags")
LDFLAGS=$("${MPC_INSTALL_PREFIX}/bin/mpc_ldflags")

append_to "CFLAGS" "-fPIC"

# Notify CLI parser that we are in FORTRAN
set_fortran

parse_cli_args $@

# Here we restore MPC's build environment
. ${MPC_INSTALL_PREFIX}/bin/mpc_build_env.sh

#
# Fortran compilation phase
#

# Check if o file contains a main and patch it if required
#
# Args:
#  - $1: .o file to be scanned
#
# Args (env):
# - MPC_COMMAND_WRAPPER: command wrapper for display
#
# Output (env):
#  - MAIN_NEEDED: (yes or no) determines if the main has to be linked
#
MAIN_NEEDED="no"
scan_and_patch_o_file()
{
	#It is an object file, but is it the real main ?
	has_symbol "${1}" "MAIN__"

	if test "x${HAS_SYMBOL}" = "xyes"; then
		MAIN_NEEDED="yes"
		#This is main, rename the symbol(s)
		$MPC_COMMAND_WRAPPER rename_main_symbols "${1}"

	else
	# Ok, not a real MAIN__ symbol but what about mpc_user_main_ ? (backward compatibility)
	# In this case, we need FORTRAN_MAIN_OBJ but not to rename the main() symbol
		has_symbol "${1}" "mpc_user_main_"
		test "x${HAS_SYMBOL}" = "xyes" && MAIN_NEEDED="yes"
	fi
}


# Scan all arguments and attempt to extract/patch
#
# Args:
#  - $1: all compiler arguments
#
# Args (env):
# - MPC_COMMAND_WRAPPER: command wrapper for display (for scan_and_patch_o_file)
#
# Output (env):
#  - MAIN_NEEDED: (yes or no) determines if the main has to be linked (via scan_and_patch_o_file)
#  - SOURCE_FILES: list of source files on CLI
#  - OBJECT_FILES: list of object files on CLI (these are patched for main if required)
#  - ARGS_NO_SOURCES: other arguments for the compiler
#
SOURCE_FILES=""
OBJECT_FILES=""
ARGS_NO_SOURCES=""
scan_cli_for_obj_and_extract_sources()
{
	# First walk through objects and modify the main one and identify sourcefiles given on link command line
	for ARG in "$@"
	do
		TMP_ARG_PROCESSED=1
		ARG="$(eval echo "$ARG")"

		#Is it a file ?
		if test -f "$ARG"; then
			#Is is an object file ?
			if has_ext "${ARG}" ".o"; then

				scan_and_patch_o_file "${ARG}"
				append_to "OBJECT_FILES" "${ARG}"

				TMP_ARG_PROCESSED=0
			else

				#This is a file, but not an object... is it a Fortran source file ?
				for ext in .f .F .f90 .F90 .f08 .F08 .for .FOR .fpp .FPP .r
				do
					if has_ext "${ARG}" "${ext}"
					then
						TMP_ARG_PROCESSED=0

						# This is a Fortran source file, store it...
						append_to "SOURCE_FILES" "${ARG}"

						break
					fi
				done

			fi
		fi

		if [ ${TMP_ARG_PROCESSED} -eq 1 ]; then
			append_to "ARGS_NO_SOURCES" "${ARG}"
		fi
	done
}

# Convert and patch all source files to .o files
# Once called all source files are now .o files
# 
# Args (from ENV):
#  - MPC_COMMAND_WRAPPER: command wrapper for display (for scan_and_patch_o_file)
#  - COMPILER: the command to compile
#  - CFLAGS: compilation flags
#  - LDFLAGS: linker flags
#  - ARGS_NO_SOURCES: unprocessed compiler args
#
# Output Args:
#  - MAIN_NEEDED: should main be linked (via scan_and_patch_o_file)
#  - OBJECT_FILES: updated object file list
#  - FAKE_OBJECT_FILE_LIST: generated object files in temporary dirs
#
FAKE_OBJECT_FILE_LIST=""
compile_and_patch_source_files()
{
	# If there are some source files on command line
	if test -n "${SOURCE_FILES}"
	then
		for source_file in ${SOURCE_FILES}
		do
			TMP_OBJ_FILE=$(mktemp --suffix=.o)
			append_to "FAKE_OBJECT_FILE_LIST" "$TMP_OBJ_FILE"

			# Compile source file
			$MPC_COMMAND_WRAPPER $COMPILER $CFLAGS ${ARGS_NO_SOURCES} "${source_file}" $LDFLAGS -c -o "${TMP_OBJ_FILE}"
			
			check_rc "$?"
					
			# Check if patching is needed
			scan_and_patch_o_file "${TMP_OBJ_FILE}"
			# Append to object file list
			append_to "OBJECT_FILES" "${TMP_OBJ_FILE}"
		done
	fi
}

clean_tmp_obj_files()
{
	if test -n "${FAKE_OBJECT_FILE_LIST}"; then
		$MPC_COMMAND_WRAPPER rm -f ${FAKE_OBJECT_FILE_LIST} > /dev/null 2>&1 
	fi
}

#
# This is the main to be potentially injected in the target progam
# when MPC_DOING_LINK
#

#
# Compile the main object file
#
# Input Args (from env mostly parse_cli_args):
#  - MPC_COMMAND_WRAPPER: wrapping command 
#  - COMPILER: the command to compile
#
DID_GENERATE_MAIN="no"
compile_fortran_main()
{
	# Cflags to be used for the main file
	CFLAGS_FORTRAN_MAIN=""
	# Name of the main fortran file
	FORTRAN_MAIN="/tmp/tmp_mpc_fortran_$$.f"
	# Name of the main fortran obj
	FORTRAN_MAIN_OBJ="/tmp/tmp_mpc_fortran_$$.o"

cat <<THIS_EOF > $FORTRAN_MAIN
      subroutine mpc_user_main_
      call mpc_user_main()
      end

      program main
      call mpc_start()
      call exit(0)
      end
THIS_EOF

	DID_GENERATE_MAIN="yes"

	$MPC_COMMAND_WRAPPER $COMPILER $CFLAGS_FORTRAN_MAIN -c $FORTRAN_MAIN -o $FORTRAN_MAIN_OBJ

	rc=$?
	if test "x$rc" != "x0" ; then 
		exit $rc
	fi
}

clean_fortran_main()
{
	if test "x${DID_GENERATE_MAIN}" = "xyes"; then
		$MPC_COMMAND_WRAPPER rm -f "$FORTRAN_MAIN" "$FORTRAN_MAIN_OBJ" > /dev/null 2>&1 
	fi
}

#
# Fortran patch phase at link time
#

if [ "$MPC_DOING_LINK" = yes ] ; then

	# FROM HERE, We want to replace main() fortran symbol in each fortran file (object or source)
	# 3 cases occur
	# - at least one .o exposes a symbol MAIN -> to be replaced and FORTRAN_MAIN_OBJ has to be loaded
	# - at least one .o exposes a symbol mpc_user_main -> nothing to do but FORTRAN_MAIN_OBJ has to be loaded
	# - No .o do not provide any MAIN* symbol -> remove FORTRAN_MAIN_OBJ, nothing indicating its a program (lib?)

	scan_cli_for_obj_and_extract_sources $COMPILATION_ARGS
	compile_and_patch_source_files

	if test "x${MAIN_NEEDED}" = "xyes"; then
		compile_fortran_main
		# Append to compilation objects
		append_to "OBJECT_FILES" "$FORTRAN_MAIN_OBJ"
	fi

	# Recretate compilation args from processed files and params
	COMPILATION_ARGS="${ARGS_NO_SOURCES} ${OBJECT_FILES}"
fi

trap clean_fortran_main EXIT
trap clean_tmp_obj_files EXIT

run_compiler